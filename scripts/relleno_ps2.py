#!/usr/bin/env python3
"""relleno_ps2.py -- func_ de PS2 que NO son funciones: bytes de la seccion que no pertenecen a ningun simbolo.

Entre funciones de los ELF de PS2 quedan trozos sueltos (`addiu $sp, $sp, 0x10; nop`, `lw $v0, 0x14($v1); nop`...):
copias muertas de la ultima instruccion de la funcion anterior que deja el relleno de ranuras de retardo del
compilador. El mapa de enlace de la A124 (orig/SLES-53558-A124/NFS.MAP) los pone DENTRO de la contribucion .text del
objeto (no son relleno que meta el enlazador), pero fuera del tamano de todo simbolo de la .symtab: no son de ninguna
funcion. splat los trocea como func_ y cuentan en el total (decision del usuario del 14-sep). Este guion los nombra
`pad_01_<DIRECCION>_text`, como GameCube nombra los suyos (pad_00_8000348C_init, pad_01_8030EA7C_text): el nombre solo
dice que ahi no hay nada que decompilar.

Prueba de cada uno:
  A124  - ningun simbolo de la .symtab con tamano lo cubre, y el simbolo de antes tiene tamano y acaba antes;
        - no contiene ni saltos ni retornos (un trozo, no una funcion);
        - nadie lo referencia: ni j/jal de ningun .s, ni un puntero en los datos del ELF, ni su nombre en otro .s.
  SLUS  - sin .symtab: el mismo trozo (mismos bytes) a la misma distancia de la misma funcion con nombre que en la
          A124, y las mismas pruebas de contenido y referencias.
        - o, si la A124 no tiene gemelo, CONTROL DE FLUJO: el trozo es solo restos de epilogo (`addiu $sp, $sp, +N` y
          `nop`, con al menos un addiu; ninguna funcion empieza desapilando) y la funcion con nombre de delante acaba en
          un salto incondicional (`jr`, `j`, `b`) con su ranura, seguido solo de ceros hasta el trozo: no se llega
          cayendo. Mas las mismas pruebas de referencias. Asi salen func_00385B90 (12 B, detras del `b` de abort) y
          func_00385C28 (4 B, detras del `j` de UnregisterPSX2VBlankHandler).

    python scripts/relleno_ps2.py SLES-53558-A124 [--unidad SUBCADENA] [--aplicar] [-v]
    python scripts/relleno_ps2.py SLUS-21351 [--unidad SUBCADENA] [--aplicar] [-v]

Sin --unidad recorre todo el programa (y solo informa de cuantos hay). Con --aplicar anade las lineas
`pad_01_XXXXXXXX_text = 0x...; // type:func` a config/<V>/symbols.txt (LF), al final del bloque de funciones.
"""
import bisect
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
from nombra_symtab import ficheros_asm  # noqa: E402
from escritura import entero  # noqa: E402

ELFS = {'SLUS-21351': 'orig/SLUS-21351/SLUS_213.51', 'SLES-53558-A124': 'orig/SLES-53558-A124/NFS.ELF'}
A124 = 'SLES-53558-A124'
RE_INS = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s+([0-9A-F]{8})\s*\*/\s+(\S+)\s*(.*)$')


def elf(version):
    d = open(os.path.join(RAIZ, ELFS[version]), 'rb').read()
    shoff, = struct.unpack_from('<I', d, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', d, 0x2e)
    secs = [struct.unpack_from('<IIIIIIIIII', d, shoff + i * shentsize) for i in range(shnum)]
    return d, secs, shstrndx


def nombre_seccion(d, secs, shstrndx, i):
    o = secs[shstrndx][4] + secs[i][0]
    return d[o:d.index(b'\0', o)].decode()


def rangos_symtab(d, secs):
    """[(inicio, fin)] de los simbolos con tamano en secciones de codigo; y {inicio: tamano} de todos."""
    rangos = []
    todos = {}
    for s in secs:
        if s[1] != 2:
            continue
        for k in range(s[5] // 16):
            _, st_value, st_size, st_info, _, st_shndx = struct.unpack_from('<IIIBBH', d, s[4] + k * 16)
            if st_shndx < len(secs) and secs[st_shndx][2] & 4 and st_value:
                todos[st_value] = max(todos.get(st_value, 0), st_size)
                if st_size:
                    rangos.append((st_value, st_value + st_size))
    return sorted(rangos), todos


def lee(version):
    """funciones de splat: [nombre, unidad, [(dir, palabra, op, args)]], destinos de j/jal y lineas que citan cada
    nombre fuera de su propia definicion."""
    funcs = []
    entradas = collections.Counter()
    citas = collections.Counter()
    for p, unidad in ficheros_asm(version):
        cur = None
        for l in open(p, encoding='utf-8', errors='replace'):
            m = re.match(r'\s*glabel (\S+)', l)
            if m:
                cur = [m.group(1), unidad, []]
                funcs.append(cur)
                continue
            if re.match(r'\s*(endlabel|nonmatching|alabel|\.size|\.type)\b', l):
                if l.strip().startswith('endlabel'):
                    cur = None
                continue
            for n in re.findall(r'\b(func_[0-9A-F]{8})\b', l):
                citas[n] += 1
            m = RE_INS.match(l)
            if not m:
                continue
            v, w, op = int(m.group(1), 16), int.from_bytes(bytes.fromhex(m.group(2)), 'little'), m.group(3)
            if op in ('j', 'jal'):
                entradas[((w & 0x3FFFFFF) << 2) | (v & 0xF0000000)] += 1
            if cur is not None:
                cur[2].append((v, w, op, m.group(4)))
    return [f for f in funcs if f[2]], entradas, citas


def punteros(d, secs, lo, hi, entradas):
    for s in secs:
        _, sh_type, sh_flags, sh_addr, sh_off, sh_size = s[:6]
        if sh_type != 1 or not sh_flags & 2 or sh_flags & 4 or not sh_addr:
            continue
        for w in struct.unpack_from('<%dI' % (sh_size // 4), d, sh_off):
            if lo <= w < hi and not w & 3:
                entradas[w] += 1


def es_salto(op):
    return op.startswith(('b', 'j')) or op in ('syscall', 'eret', 'break')


def es_incondicional(op, x):
    """Salto sin vuelta ni condicion: jr (retorno o tabla), j, b (beq $zero,$zero) y eret."""
    return op in ('jr', 'j', 'b', 'eret') or (op == 'beq' and x.replace(' ', '').startswith('$zero,$zero,'))


def es_resto_de_epilogo(ins):
    """Solo `addiu $sp, $sp, +N` (N > 0) y `nop`, con al menos un addiu."""
    hay = False
    for _, _, op, x in ins:
        if op == 'nop':
            continue
        m = re.match(r'\$sp,\$sp,(0x[0-9A-Fa-f]+|\d+)$', x.replace(' ', ''))
        if op == 'addiu' and m and int(m.group(1), 0) > 0:
            hay = True
            continue
        return False
    return hay


def palabra(d, secs, v):
    for s in secs:
        if s[2] & 4 and s[3] <= v < s[3] + s[5]:
            return struct.unpack_from('<I', d, s[4] + v - s[3])[0]
    return None


def tras_salto(d, secs, ant, a):
    """La funcion de delante acaba en un salto incondicional mas su ranura (lo que siga dentro de ella, nop), y de su
    final al trozo solo hay ceros: al trozo no se llega cayendo."""
    ins = ant[2]
    k = len(ins) - 1
    while k >= 0 and ins[k][2] == 'nop':
        k -= 1
    if k < 0:
        return False
    if not (es_incondicional(ins[k][2], ins[k][3]) or (k >= 1 and es_incondicional(ins[k - 1][2], ins[k - 1][3]))):
        return False
    fin = ins[-1][0] + 4
    if not fin <= a <= fin + 12:
        return False
    return all(palabra(d, secs, v) == 0 for v in range(fin, a, 4))


def candidatos(version, filtro):
    d, secs, shstrndx = elf(version)
    funcs, entradas, citas = lee(version)
    lo = min(f[2][0][0] for f in funcs)
    hi = max(f[2][-1][0] for f in funcs) + 4
    punteros(d, secs, lo, hi, entradas)
    out = []
    motivos = collections.Counter()
    previa = {}
    for k, f in enumerate(funcs):
        previa[f[0]] = funcs[k - 1] if k else None
    for k, (n, unidad, ins) in enumerate(funcs):
        if not n.startswith('func_') or filtro not in unidad:
            continue
        a = ins[0][0]
        b = ins[-1][0] + 4
        if any(es_salto(op) for _, _, op, _ in ins):
            motivos['tiene saltos o retorno'] += 1
            continue
        if entradas.get(a) or citas.get(n):
            motivos['referenciado'] += 1
            continue
        ant = funcs[k - 1] if k else None
        if ant is None or ant[1] != unidad or ant[0].startswith('func_'):
            motivos['sin funcion con nombre delante en la unidad'] += 1
            continue
        out.append((n, unidad, a, b, ins, ant))
    return out, motivos, (d, secs, shstrndx)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    V = args[0] if args else A124
    filtro = sys.argv[sys.argv.index('--unidad') + 1] if '--unidad' in sys.argv else ''
    if V not in ELFS:
        sys.exit(__doc__)
    cand, motivos, (d, secs, shstrndx) = candidatos(V, filtro)
    buenos = []
    if V == A124:
        rangos, todos = rangos_symtab(d, secs)
        inicios = [r[0] for r in rangos]
        for n, unidad, a, b, ins, ant in cand:
            k = bisect.bisect_right(inicios, b - 1) - 1
            if any(rangos[j][0] < b and rangos[j][1] > a for j in range(max(0, k - 50), k + 1)):
                motivos['lo cubre un simbolo de la .symtab'] += 1
                continue
            a_ant = ant[2][0][0]
            if not todos.get(a_ant) or a_ant + todos[a_ant] > a:
                motivos['el simbolo de delante no tiene tamano'] += 1
                continue
            buenos.append((n, unidad, a, b, ins))
    else:
        # SLUS: el mismo trozo que un hueco probado de la A124, a la misma distancia de la misma funcion con nombre
        ca, _, (da, sa, xa) = candidatos(A124, filtro)
        rangos, todos = rangos_symtab(da, sa)
        inicios = [r[0] for r in rangos]
        huecos_a = {}
        for n, unidad, a, b, ins, ant in ca:
            k = bisect.bisect_right(inicios, b - 1) - 1
            if any(rangos[j][0] < b and rangos[j][1] > a for j in range(max(0, k - 50), k + 1)):
                continue
            a_ant = ant[2][0][0]
            if not todos.get(a_ant) or a_ant + todos[a_ant] > a:
                continue
            huecos_a[(ant[0], a - a_ant)] = [w for _, w, _, _ in ins]
        for n, unidad, a, b, ins, ant in cand:
            clave = (ant[0], a - ant[2][0][0])
            if huecos_a.get(clave) == [w for _, w, _, _ in ins]:
                buenos.append((n, unidad, a, b, ins))
                continue
            if es_resto_de_epilogo(ins) and tras_salto(d, secs, ant, a):
                print('  sin gemelo en la A124, por control de flujo: %s detras de %s' % (n, ant[0]))
                buenos.append((n, unidad, a, b, ins))
                continue
            motivos['sin el mismo hueco en la A124'] += 1
    for m, c in motivos.most_common():
        print('  descartados (%s): %d' % (m, c))
    idx_text = [i for i in range(len(secs)) if nombre_seccion(d, secs, shstrndx, i) == '.text'][0]
    print('%s: %d huecos que no son funcion (%d B)%s' % (V, len(buenos), sum(b - a for _, _, a, b, _ in buenos),
                                                        ' en ' + filtro if filtro else ''))
    if '-v' in sys.argv:
        for n, unidad, a, b, ins in buenos:
            print('  %s %-60s %s' % (n, unidad[-60:], ' | '.join('%s %s' % (op, x) for _, _, op, x in ins)))
    if '--aplicar' in sys.argv and buenos:
        ruta = os.path.join(RAIZ, 'config', V, 'symbols.txt')
        raw = open(ruta, 'rb').read().decode('utf-8')
        assert '\r\n' not in raw, 'symbols.txt tiene que ser LF'
        ya = set(re.findall(r'^\S+ = 0x([0-9a-f]+);', raw, re.M))
        nuevas = []
        for n, unidad, a, b, ins in buenos:
            seccion = None
            for i, s in enumerate(secs):
                if s[2] & 4 and s[3] <= a < s[3] + s[5]:
                    seccion = (i, nombre_seccion(d, secs, shstrndx, i))
            if seccion is None or '%x' % a in ya:
                continue
            nuevas.append('pad_%02d_%08X_%s = 0x%x; // type:func' % (seccion[0], a, seccion[1].lstrip('.'), a))
        if not raw.endswith('\n'):
            raw += '\n'
        raw += '// Huecos sin funcion entre funciones (scripts/relleno_ps2.py): no hay nada que decompilar.\n'
        raw += '\n'.join(nuevas) + '\n'
        entero(ruta, 'w', encoding='utf-8', newline='\n').write(raw)
        print('anadidas %d lineas a %s (seccion .text = %d)' % (len(nuevas), os.path.relpath(ruta, RAIZ), idx_text))


if __name__ == '__main__':
    main()
