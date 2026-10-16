#!/usr/bin/env python3
"""restos_trozos.py [VERSION] [--aplicar] -- nombres de config/<V>/symbols.txt puestos sobre TROZOS SUELTOS de 8 bytes
que hay delante de su funcion, y que por eso no pueden casar.

Entre funciones de los ELF de PS2 quedan trozos de 8 bytes: una instruccion suelta y un `nop` (`swc1 $f0, 0($a1)`,
`daddu $v0, $a0, $zero`, `addiu $sp, $sp, 0x30`...). scripts/restos_epilogo.py solo reconoce los `addiu $sp, $sp, +N`
y la ranura de un `jr $ra`; los demas se quedaron dentro de la funcion con el nombre encima. En SLUS los nombres se
heredaron de la A124 por direccion y caian sobre el trozo: UpdateTime__10CAnimScenef estaba 8 bytes antes de su
entrada, y todos sus llamadores saltan a la entrada de verdad (splat hasta pone ahi una `alabel`).

La prueba es de REFERENCIAS, no de patron: se mueve el nombre de A a A+d (d = 8, 16, 24 o 32) solo si
  - alguien entra en A+d: un j/jal de cualquier .s, o un puntero en los datos del ELF (vtables, tablas);
  - nadie entra en A: ni j/jal ni puntero (si lo hay, A es la entrada y el nombre esta bien);
  - lo que hay en [A, A+d) no tiene ni un salto y acaba en `nop` (un trozo, no codigo de la funcion);
  - A+d no tiene ya nombre en symbols.txt.
Control: en la A124, cuyos nombres salen de la .symtab, tiene que salir 0 (2026-09-23: 0).

    python scripts/restos_trozos.py SLES-53558-A124            # control
    python scripts/restos_trozos.py SLUS-21351                 # informa
    python scripts/restos_trozos.py SLUS-21351 --aplicar

Despues: trocear y medir la version (scripts/medir.py), y pasar scripts/cortes.py.
"""
import collections
import os
import re
import struct
import sys

from nombra_symtab import ficheros_asm
from escritura import entero

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ELFS = {'SLUS-21351': 'orig/SLUS-21351/SLUS_213.51', 'SLES-53558-A124': 'orig/SLES-53558-A124/NFS.ELF'}
RE_INS = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s+([0-9A-F]{8})\s*\*/\s+(\S+)\s*(.*)$')
SALTOS = ('b', 'j')


def es_salto(op):
    return op.startswith(SALTOS) or op in ('syscall', 'eret', 'break')


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    V = args[0] if args else 'SLUS-21351'
    if V not in ELFS:
        sys.exit(__doc__)
    funcs = []                                   # [inicio, nombre, [(dir, op)]]
    entradas = collections.Counter()             # destinos de j/jal
    for p, unidad in ficheros_asm(V):
        cur = None
        for l in open(p, encoding='utf-8', errors='replace'):
            m = re.match(r'\s*glabel (\S+)', l)
            if m:
                cur = [None, m.group(1), []]
                funcs.append(cur)
                continue
            m = RE_INS.match(l)
            if not m:
                continue
            v, w, op = int(m.group(1), 16), int.from_bytes(bytes.fromhex(m.group(2)), 'little'), m.group(3)
            if op in ('j', 'jal'):
                entradas[((w & 0x3FFFFFF) << 2) | (v & 0xF0000000)] += 1
            if cur is not None:
                if cur[0] is None:
                    cur[0] = v
                if len(cur[2]) < 10:
                    cur[2].append((v, op))
            if re.match(r'\s*endlabel', l):
                cur = None
    funcs = [f for f in funcs if f[0] is not None]
    lo = min(f[0] for f in funcs)
    hi = max(f[2][-1][0] for f in funcs if f[2]) + 4
    # punteros a codigo en los datos: secciones PROGBITS con SHF_ALLOC y sin SHF_EXECINSTR (.rodata, .data, .sdata...).
    # El ELF tiene un solo segmento para todo, asi que van por secciones; en .text una instruccion como
    # `sll $v0, $s0, 2` (0x00101080) pareceria un puntero.
    d = open(os.path.join(RAIZ, ELFS[V]), 'rb').read()
    shoff, = struct.unpack_from('<I', d, 0x20)
    shentsize, shnum, _ = struct.unpack_from('<HHH', d, 0x2e)
    for i in range(shnum):
        _, sh_type, sh_flags, sh_addr, sh_off, sh_size = struct.unpack_from('<IIIIII', d, shoff + i * shentsize)
        if sh_type != 1 or not sh_flags & 2 or sh_flags & 4 or not sh_addr:
            continue
        for w in struct.unpack_from('<%dI' % (sh_size // 4), d, sh_off):
            if lo <= w < hi and not w & 3:
                entradas[w] += 1
    ruta = os.path.join(RAIZ, 'config', V, 'symbols.txt')
    raw = open(ruta, 'rb').read().decode('utf-8')
    assert '\r\n' not in raw, 'symbols.txt tiene que ser LF'
    lineas = raw.split('\n')
    por_dir, por_nom = collections.defaultdict(list), collections.defaultdict(list)
    for i, l in enumerate(lineas):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);(.*)$', l)
        if m:
            por_dir[int(m.group(2), 16)].append(m.group(1))
            por_nom[m.group(1)].append((int(m.group(2), 16), i, m.group(3)))
    mover = []
    for a, nombre, ins in funcs:
        if nombre.startswith(('func_', 'D_')) or entradas.get(a) or len(por_nom.get(nombre, [])) != 1:
            continue
        if 'type:func' not in por_nom[nombre][0][2] or por_nom[nombre][0][0] != a:
            continue
        for dd in (8, 16, 24, 32):
            trozo = [op for v, op in ins if v < a + dd]
            if len(trozo) != dd // 4 or any(es_salto(op) for op in trozo):
                break
            if trozo[-1] == 'nop' and entradas.get(a + dd) and not por_dir.get(a + dd):
                mover.append((a, a + dd, nombre, entradas[a + dd], trozo))
                break
    for a, b, nombre, refs, trozo in mover:
        print('  0x%06x -> 0x%06x %-70s (%d referencias; trozo: %s)' % (a, b, nombre[:70], refs, ' '.join(trozo)))
    print('%s: %d nombres sobre trozos sueltos' % (V, len(mover)))
    if '--aplicar' in sys.argv and mover:
        for a, b, nombre, refs, trozo in mover:
            i = por_nom[nombre][0][1]
            m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);(.*)$', lineas[i])
            lineas[i] = '%s = 0x%x;%s' % (nombre, b, m.group(3))
        entero(ruta, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
        print('escrito', os.path.relpath(ruta, RAIZ))


if __name__ == '__main__':
    main()
