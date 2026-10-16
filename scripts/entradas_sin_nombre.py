#!/usr/bin/env python3
"""entradas_sin_nombre.py -- entradas de verdad DENTRO de funciones sin nombre de SLUS: func_ de splat que empiezan por
trozos (restos de epilogo `addiu $sp`, ranuras de retardo sueltas `addiu $v0, $v0, N`, `nop`...).

Una direccion E dentro de func_X (X < E) es una entrada si alguien entra en E (j/jal de cualquier .s o un puntero en
los datos del ELF: vtables, tablas de funciones), nadie entra en X, y [X, E) no tiene saltos.

    python scripts/entradas_sin_nombre.py [--tsv entradas.tsv]

Imprime cada entrada con quien la referencia (j/jal con la funcion que llama, o "dato@" con la direccion del puntero)
y los trozos que la preceden. El TSV es la entrada de scripts/vtnuestra.py (las referenciadas por datos) y se cruza
con scripts/grafo_nuestro.py (las referenciadas por j/jal). Tanda 6: 128 entradas, 87 desde datos.
"""
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
from nombra_symtab import ficheros_asm  # noqa: E402
from vtable_dtors import Imagen, ELFS  # noqa: E402

S = 'SLUS-21351'
RE_INS = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s+([0-9A-F]{8})\s*\*/\s+(\S+)\s*(.*)$')
SIN = re.compile(r'^func_[0-9A-F]{8}$')


def es_salto(op):
    return op.startswith(('b', 'j')) or op in ('syscall', 'eret', 'break')


def main():
    funcs = []
    entradas = collections.defaultdict(list)       # destino -> [(origen, op, funcion que llama)]
    for p, unidad in ficheros_asm(S):
        cur = None
        for l in open(p, encoding='utf-8', errors='replace'):
            g = re.match(r'\s*glabel (\S+)', l)
            if g:
                cur = [g.group(1), unidad, []]
                funcs.append(cur)
                continue
            m = RE_INS.match(l)
            if not m or cur is None:
                continue
            v = int(m.group(1), 16)
            w = int.from_bytes(bytes.fromhex(m.group(2)), 'little')
            op = m.group(3)
            cur[2].append((v, op, m.group(4)))
            if op in ('j', 'jal') and not m.group(4).startswith('.L'):
                entradas[((w & 0x3FFFFFF) << 2) | (v & 0xF0000000)].append((v, op, cur[0]))
            if re.match(r'\s*endlabel', l):
                cur = None
    img = Imagen(os.path.join(RAIZ, ELFS[S]))
    texto = [(f[2][0][0], f[2][-1][0] + 4) for f in funcs if f[2]]
    lo, hi = min(a for a, b in texto), max(b for a, b in texto)
    punteros = collections.defaultdict(list)
    for va, off, tam in img.segs:
        d = img.d[off:off + tam]
        for k in range(0, len(d) - 3, 4):
            w = struct.unpack_from('<I', d, k)[0]
            if lo <= w < hi and w & 3 == 0:
                punteros[w].append(va + k)
    filas = []
    for g, unidad, ins in funcs:
        if not SIN.match(g) or len(ins) < 3:
            continue
        x = ins[0][0]
        if entradas.get(x) or punteros.get(x):
            continue
        for i in range(1, min(len(ins), 17)):
            e = ins[i][0]
            if es_salto(ins[i - 1][1]):
                break
            refs = entradas.get(e, []) + [(p, 'dato', '') for p in punteros.get(e, [])]
            if refs:
                cuerpo = ins[i:]
                filas.append((e, g, unidad, e - x, len(cuerpo) * 4, refs, ins[:i]))
                break
    for e, g, unidad, d, tam, refs, trozos in filas:
        r = ', '.join('%s@0x%x%s' % (op, o, (' ' + q) if q else '') for o, op, q in refs[:4])
        print('0x%06x %-15s %-18s trozos %2d B cuerpo %5d B  %s%s' % (e, g, unidad.split('/')[-1][:18], d, tam, r,
                                                                   ' ...' if len(refs) > 4 else ''))
        print('         trozos: ' + ' ; '.join('%s %s' % (op, a.replace(' ', '')) for v, op, a in trozos))
    print('%d entradas dentro de func_' % len(filas))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for e, g, unidad, d, tam, refs, trozos in filas:
                fh.write('0x%x\t%s\t%s\t%d\t%d\t%s\n' % (e, g, unidad, d, tam,
                                                        ';'.join('%s@0x%x %s' % (op, o, q) for o, op, q in refs)))


if __name__ == '__main__':
    main()
