#!/usr/bin/env python3
"""Nombres de funcion que la .symtab del ELF de la A124 tiene y config/SLES-53558-A124/symbols.txt no: casi todos
ESTATICOS (`static` de C/C++, `__tcf_N`, `__static_initialization_and_destruction_0`, los MUTEX_* inline de cada
SourceList...). objdiff empareja por NOMBRE: mientras el original los llame `func_XXXXXXXX`, no casan aunque nuestro
objeto defina la funcion identica.

La direccion sale de la propia .symtab (exacta), pero se comprueba antes de anadirla:
  - que sea una ENTRADA: no cae dentro del rango (st_size) de otra funcion de la .symtab, y en el troceado actual o
    hay ya un `glabel` en esa direccion o la funcion de splat que la contiene solo lleva delante un resto de epilogo
    (`addiu $sp, $sp, +N` / `nop`) o trozos sueltos de 8 bytes (una instruccion y un `nop`), sin saltos, y ningun
    salto local cruza la direccion (si no, cortaria una funcion por una etiqueta de rama);
  - que la direccion no tenga ya nombre en symbols.txt;
  - los nombres repetidos (un estatico por unidad) llevan `allow_duplicated:true` en TODAS sus lineas, tambien la que
    ya estaba, y solo si cada aparicion cae en una unidad distinta (dos iguales en un .s no ensamblan);
  - un nombre de mas de 250 bytes lleva `filename:func_XXXXXXXX` (splat escribe un fichero por simbolo).
Las funciones de tamano 0 (etiquetas de asm) solo entran si splat ya las trocea como funcion; y lo mismo las rutinas
de asm escrito a mano que la .symtab declara OBJECT o NOTYPE (globales, tamano 0: bDoWithStack, InvalidateICache...).

Resultado del 2026-09-23: 947 nombres (la mayoria estaticos) mas 10 rutinas de asm; la A124 paso de 15.941 a 16.416
funciones casadas (+467 de estos nombres, +30,6 kB) sin perder ninguna. Tras aplicarlo, `.symtab` y `.mdebug` quedan
agotados: las ~1.900 func_ que quedan son restos de epilogo y codigo sin simbolo en ninguna de las dos tablas.

Informa por unidad de cuantas casarian al instante: nuestro objeto (build/<V>/src) define ese nombre y su codigo es
identico al del original troceado (build/<V>/obj), comparando como tools/nombra_ps2.py (en las palabras reubicadas,
solo el codigo de operacion). Con --aplicar anade las lineas al final de symbols.txt, en un bloque por unidad. Despues
hay que trocear y medir con scripts/medir.py, y pasar scripts/cortes.py y scripts/restos_epilogo.py.

    python scripts/nombra_symtab.py                 # informa
    python scripts/nombra_symtab.py --tsv f.tsv     # y vuelca la lista (direccion, tamano, nombre, unidad, estado)
    python scripts/nombra_symtab.py --aplicar
    python scripts/nombra_symtab.py --tamanos [--aplicar]   # size: donde splat no corta como la .symtab

Solo la A124 tiene .symtab con nombres (el ELF de SLUS esta estripado). Lee el troceado actual: build/<V>/asm y
build/<V>/obj tienen que ser de un troceado reciente (medir.py los rehace).
"""
import bisect
import collections
import json
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
import nombra_ps2  # noqa: E402
from escritura import entero  # noqa: E402

V = 'SLES-53558-A124'
ELF = os.path.join(RAIZ, 'orig', V, 'NFS.ELF')
SYM = os.path.join(RAIZ, 'config', V, 'symbols.txt')
RE_INS = re.compile(r'^\s*/\*\s*([0-9A-F]+)\s+([0-9A-F]{8})\s+([0-9A-F]{8})\s*\*/\s+(\S+)\s*(.*)$')
RE_L = re.compile(r'\.L([0-9A-F]{8})')
ILEGALES = set('<>:"/\\|?*')


def symtab(ruta):
    """[(valor, tam, info, nombre, shndx)] de la .symtab de un ELF de 32 bits"""
    d = open(ruta, 'rb').read()
    shoff, = struct.unpack_from('<I', d, 0x20)
    shentsize, shnum, _ = struct.unpack_from('<HHH', d, 0x2e)
    secs = [struct.unpack_from('<IIIIIIIIII', d, shoff + i * shentsize) for i in range(shnum)]
    out = []
    for s in secs:
        if s[1] != 2:
            continue
        strt = secs[s[6]]
        for k in range(s[5] // 16):
            st_name, st_value, st_size, st_info, _, st_shndx = struct.unpack_from('<IIIBBH', d, s[4] + k * 16)
            b = strt[4] + st_name
            out.append((st_value, st_size, st_info, d[b:d.index(b'\0', b)].decode('latin-1'), st_shndx))
    return out


def ficheros_asm(version):
    """[(ruta, unidad)] de los .s del troceado ACTUAL: los de las unidades de build/<V>/config.json. Con glob sobre
    build/<V>/asm entran .s viejos de troceados anteriores (asm/data/rodata.rodata.s de SLUS es del 16-sep y tiene un
    `glabel func_001018F8` sobre datos) y confunden a quien los recorra."""
    cfg = json.load(open(os.path.join(RAIZ, 'build', version, 'config.json'), encoding='utf-8'))
    out = []
    for u in cfg['units']:
        p = os.path.join(RAIZ, 'build', version, 'asm', u['name'] + '.s')
        if os.path.exists(p):
            out.append((p, u['name']))
    return out


def lee_asm(version):
    """[(ini, fin, nombre, unidad, [(origen, destino) de saltos locales], [primeras instrucciones])] de splat"""
    fns = []
    for p, unidad in ficheros_asm(version):
        cur = None
        for l in open(p, encoding='utf-8', errors='replace'):
            m = re.match(r'\s*glabel (\S+)', l)
            if m:
                cur = [None, None, m.group(1), unidad, [], []]
                fns.append(cur)
                continue
            m = RE_INS.match(l)
            if m and cur is not None:
                v = int(m.group(2), 16)
                if cur[0] is None:
                    cur[0] = v
                cur[1] = v + 4
                cur[4] += [(v, int(t, 16)) for t in RE_L.findall(m.group(5))]
                if len(cur[5]) < 64:
                    cur[5].append((v, m.group(4), m.group(5).replace(' ', '')))
            elif re.match(r'\s*endlabel', l):
                cur = None
    return sorted(tuple(f) for f in fns if f[0] is not None)


def simbolos():
    """{dir: [nombres]}, {nombre: [(dir, resto)]} de symbols.txt"""
    por_dir, por_nom = {}, {}
    for l in open(SYM, encoding='utf-8'):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);(.*)$', l.rstrip('\n'))
        if m:
            a = int(m.group(2), 16)
            por_dir.setdefault(a, []).append(m.group(1))
            por_nom.setdefault(m.group(1), []).append((a, m.group(3)))
    return por_dir, por_nom


def es_salto(ins):
    return ins[1].startswith(('b', 'j')) or ins[1] in ('syscall', 'eret')


def solo_restos(antes):
    """Lo que splat deja delante de una funcion cuando no sabe donde empieza: restos de epilogo (`addiu $sp, $sp, +N`
    y `nop`) y otros trozos sueltos de 8 bytes (una instruccion y un `nop`). Sin un solo salto, y acabando en `nop`."""
    return bool(antes) and antes[-1][1] == 'nop' and not any(es_salto(x) for x in antes)


_objs = {}


def funciones_obj(ruta):
    if ruta not in _objs:
        out = {}
        if os.path.exists(ruta):
            for nombre, b, rel, orden in nombra_ps2.Elf(ruta).funciones():
                out.setdefault(nombre, []).append((b, rel))
        _objs[ruta] = out
    return _objs[ruta]


def estado(unidad, nombre, glabel):
    """IGUAL (casaria al instante), DISTINTA, NO_DEFINE (nuestro objeto no la tiene) o SIN_OBJ"""
    base = os.path.splitext(unidad)[0] if unidad.endswith(('.cpp', '.c')) else unidad
    nuestras = funciones_obj(os.path.join(RAIZ, 'build', V, 'src', base + '.o')).get(nombre)
    if nuestras is None:
        return 'NO_DEFINE' if os.path.exists(os.path.join(RAIZ, 'build', V, 'src', base + '.o')) else 'SIN_OBJ'
    obj = funciones_obj(os.path.join(RAIZ, 'build', V, 'obj', base + '.o')).get(glabel) if glabel else None
    if not obj:
        return 'DEFINE'
    tb, trel = obj[0]
    ca, ma = nombra_ps2.normaliza(tb, trel)
    for b, rel in nuestras:
        cb, mb = nombra_ps2.normaliza(b, rel)
        if nombra_ps2.coinciden(ca, cb, ma, mb):
            return 'IGUAL'
    return 'DISTINTA'


def iguales_con_tamano(unidad, nombre, v, tam):
    """True si nuestra funcion `nombre` mide `tam` y es identica a los bytes del ELF original en [v, v+tam) (en las
    palabras que reubicamos: codigo de operacion + rs + rt, o solo el codigo en j/jal)"""
    base = os.path.splitext(unidad)[0] if unidad.endswith(('.cpp', '.c')) else unidad
    nuestras = funciones_obj(os.path.join(RAIZ, 'build', V, 'src', base + '.o')).get(nombre) or []
    d = open(ELF, 'rb').read()
    phoff, = struct.unpack_from('<I', d, 0x1C)
    phentsize, phnum = struct.unpack_from('<HH', d, 0x2A)
    orig = None
    for i in range(phnum):
        p = struct.unpack_from('<IIIIIIII', d, phoff + i * phentsize)
        if p[0] == 1 and p[2] <= v < p[2] + p[4]:
            orig = d[p[1] + v - p[2]:p[1] + v - p[2] + tam]
    for b, rel in nuestras:
        if orig is None or len(b) != tam:
            continue
        n = tam // 4
        wo, wn = struct.unpack('<%dI' % n, orig), struct.unpack('<%dI' % n, b)
        marc = {r // 4 for r, _ in rel}
        if all(wo[k] == wn[k] or (k in marc and (wo[k] ^ wn[k]) & (0xFC000000 if wo[k] >> 26 in (2, 3) else 0xFFFF0000)
                                   == 0) for k in range(n)):
            return True
    return False


def tamanos(aplicar):
    """--tamanos: funciones con nombre cuyo tamano en el troceado no es el de la .symtab. splat corta por `jr $ra` y
    por el siguiente simbolo: se come el codigo suelto que sigue a una funcion que acaba en `b`/`j`, y parte en dos
    las que tienen un `jr $ra` a mitad. spimdisasm respeta `size:` en una funcion (IDct64_PS2 ya lo lleva). Se anade
    solo donde no puede costar nada: nuestra funcion es identica con el tamano de la .symtab (entonces casa), o
    nuestro objeto no la define y el troceado la hace MAS LARGA (codigo suelto detras). Nunca a una que ya casa con el
    tamano de splat (MapLoopToVolume: sus 16 bytes de mas son un caso del switch que nuestro codigo pone en otro
    sitio)."""
    st = {}
    for v, s, info, n, sh in symtab(ELF):
        if (info & 0xf) == 2 and s > 0:
            st.setdefault(v, (s, n))
    por_dir, por_nom = simbolos()
    cambios = {}
    for f in lee_asm(V):
        if f[2].startswith('func_') or f[0] not in st or st[f[0]][1] != f[2]:
            continue
        tam, t_splat = st[f[0]][0], f[1] - f[0]
        if tam == t_splat or any('size:' in r for a, r in por_nom.get(f[2], []) if a == f[0]):
            continue
        base = os.path.splitext(f[3])[0] if f[3].endswith(('.cpp', '.c')) else f[3]
        define = f[2] in funciones_obj(os.path.join(RAIZ, 'build', V, 'src', base + '.o'))
        if iguales_con_tamano(f[3], f[2], f[0], tam):
            motivo = 'nuestra funcion es identica con ese tamano'
        elif not define and t_splat > tam:
            motivo = 'no la definimos y splat se come %d B de detras' % (t_splat - tam)
        else:
            print('  deja  0x%06x %-60s splat %d, .symtab %d' % (f[0], f[2][:60], t_splat, tam))
            continue
        cambios[f[0]] = (f[2], tam)
        print('  size  0x%06x %-60s splat %d -> %d: %s' % (f[0], f[2][:60], t_splat, tam, motivo))
    if not aplicar or not cambios:
        return
    raw = open(SYM, 'rb').read().decode('utf-8')
    assert '\r\n' not in raw, 'symbols.txt tiene que ser LF'
    lineas = raw.split('\n')
    for i, l in enumerate(lineas):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);(.*)$', l)
        if m and int(m.group(2), 16) in cambios and cambios[int(m.group(2), 16)][0] == m.group(1):
            lineas[i] = l.rstrip() + ' size:0x%X' % cambios[int(m.group(2), 16)][1]
    entero(SYM, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
    print('size: en %d funciones' % len(cambios))


def main():
    aplicar = '--aplicar' in sys.argv
    if '--tamanos' in sys.argv:
        return tamanos(aplicar)
    st = [x for x in symtab(ELF) if (x[2] & 0xf) == 2 and x[3]]          # STT_FUNC con nombre
    por_dir, por_nom = simbolos()
    fns = lee_asm(V)
    inis = [f[0] for f in fns]
    rangos = sorted((v, v + s, n) for v, s, i, n, sh in st if s > 0)

    def unidad_de(v):
        j = bisect.bisect_right(inis, v) - 1
        return fns[j] if j >= 0 and fns[j][0] <= v < fns[j][1] else None

    candidatas, rechazo = [], collections.Counter()
    for v, s, info, n, sh in sorted(st):
        if v in por_dir or any(a == v for a, _ in por_nom.get(n, [])):
            continue                                                       # ya nombrada
        f = unidad_de(v)
        motivo = None
        k = bisect.bisect_right(rangos, (v, 1 << 40)) - 1
        dentro = [r for r in rangos[max(0, k - 64):k + 1] if r[0] < v < r[1]]
        if f is None:
            motivo = 'sin asm'
        elif f[0] != v:
            antes = [x for x in f[5] if x[0] < v]
            if dentro:
                motivo = 'dentro de %s' % dentro[0][2]
            elif len(antes) != (v - f[0]) // 4 or not solo_restos(antes):
                motivo = 'parte %s y lo de delante no son restos' % f[2]
            elif [1 for o, t in f[4] if o < v <= t or t < v <= o]:
                motivo = 'parte %s y un salto local la cruza' % f[2]
        elif s == 0 and not f[2].startswith('func_'):
            motivo = 'tamano 0 sobre %s' % f[2]
        elif dentro and s > 0:
            motivo = 'dentro de %s' % dentro[0][2]
        if any(c in ILEGALES for c in n):
            motivo = 'caracteres que splat no admite'
        if motivo:
            rechazo[motivo.split(' ')[0]] += 1
            print('  fuera 0x%06x %-60s %s' % (v, n[:60], motivo))
            continue
        candidatas.append((v, s, n, f[3], f[2] if f[0] == v else None))
    # rutinas de asm escrito a mano (bmiscasm.s, superbenderasm.s): la .symtab las declara OBJECT o NOTYPE, globales
    # y de tamano 0; solo entran si splat ya las trocea como funcion sin nombre
    vistas = {c[0] for c in candidatas}
    for v, s, info, n, sh in sorted(symtab(ELF)):
        if (info & 0xf) not in (0, 1) or info >> 4 != 1 or s or not n or v in por_dir or v in vistas:
            continue
        f = unidad_de(v)
        if f and f[0] == v and f[2].startswith('func_') and not any(c in ILEGALES for c in n) and n not in por_nom:
            candidatas.append((v, s, n, f[3], f[2]))
            vistas.add(v)

    # repetidos: todas las apariciones (nuevas y viejas) en unidades distintas
    apariciones = collections.defaultdict(list)
    for v, s, n, u, g in candidatas:
        apariciones[n].append((v, u))
    for n in list(apariciones):
        for a, _ in por_nom.get(n, []):
            f = unidad_de(a)
            apariciones[n].append((a, f[3] if f else None))
    repes = {n for n, l in apariciones.items() if len(l) > 1}
    malas = {n for n in repes if len({u for _, u in apariciones[n]}) != len(apariciones[n])}
    for n in sorted(malas):
        print('  fuera %s: se repite dentro de una misma unidad' % n)
    candidatas = [c for c in candidatas if c[2] not in malas]

    por_unidad = collections.defaultdict(list)
    cuenta = collections.Counter()
    bytes_iguales = 0
    filas = []
    for v, s, n, u, g in candidatas:
        e = estado(u, n, g)
        cuenta[e] += 1
        if e == 'IGUAL':
            bytes_iguales += s
        por_unidad[u].append((v, s, n, e))
        filas.append((v, s, n, u, e))
    print('.symtab: %d funciones con nombre; %d candidatas nuevas (%d con nombre repetido); fuera %s'
          % (len(st), len(candidatas), sum(1 for c in candidatas if c[2] in repes), dict(rechazo)))
    print('estado frente a nuestros objetos: %s; casarian al instante %d funciones, %d B'
          % (dict(cuenta), cuenta['IGUAL'], bytes_iguales))
    for u in sorted(por_unidad, key=lambda x: -len(por_unidad[x])):
        lst = por_unidad[u]
        print('  %-70s %4d  iguales %4d' % (u[-70:], len(lst), sum(1 for x in lst if x[3] == 'IGUAL')))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for v, s, n, u, e in filas:
                fh.write('0x%x\t%d\t%s\t%s\t%s\n' % (v, s, n, u, e))
    if not aplicar or not candidatas:
        return

    raw = open(SYM, 'rb').read().decode('utf-8')
    assert '\r\n' not in raw, 'symbols.txt tiene que ser LF'
    lineas = raw.split('\n')
    # allow_duplicated en las lineas viejas de los nombres que ahora se repiten
    for i, l in enumerate(lineas):
        m = re.match(r'(\S+) = 0x[0-9a-fA-F]+;(.*)$', l)
        if m and m.group(1) in repes and 'allow_duplicated' not in m.group(2):
            lineas[i] = l.rstrip() + ' allow_duplicated:true'
    while lineas and not lineas[-1].strip():
        lineas.pop()
    lineas += ['', '// Nombres de la .symtab del ELF de la A124 que faltaban (scripts/nombra_symtab.py): casi todos',
               '// estaticos; la direccion es la de la .symtab, comprobada como entrada de funcion.']
    for u in sorted(por_unidad, key=lambda x: min(y[0] for y in por_unidad[x])):
        lineas += ['/' * 67, '// ' + u, '/' * 67]
        for v, s, n, e in sorted(por_unidad[u]):
            extra = ' filename:func_%08X' % v if len(n) > 250 else ''
            extra += ' allow_duplicated:true' if n in repes else ''
            lineas.append('%s = 0x%x; // type:func%s' % (n, v, extra))
    lineas.append('')
    entero(SYM, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
    print('anadidas %d lineas a %s' % (len(candidatas), os.path.relpath(SYM, RAIZ)))


if __name__ == '__main__':
    main()
