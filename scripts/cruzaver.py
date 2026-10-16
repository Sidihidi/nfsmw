#!/usr/bin/env python3
"""cruzaver.py -- nombres de SLUS por CODIGO IDENTICO AL DE LA A124: para cada funcion sin nombre de SLUS (func_, o
entrada dentro de una func_ tras trozos) busca las funciones de la A124 con el mismo codigo (palabras iguales; en las
reubicadas, codigo de operacion y registros como scripts/prueba_nombre.py). Propone el nombre de la A124 (de la
.symtab) si hay UNA sola funcion de la A124 con ese codigo en todo el ejecutable y el nombre no esta ya en symbols.txt
de SLUS.

    python scripts/cruzaver.py [--min 4] [--tsv propuestas.tsv] [--entradas entradas.tsv]

Sirve donde no tenemos fuente (SDK, libm, partes de realcore y realinput) y para ver que unidades de la A124 estan
fundidas en otra en SLUS (la unidad de la A124 de cada propuesta sale en la ultima columna). Tanda 6: 90 propuestas.
"""
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
import prueba_nombre as pn  # noqa: E402
from nombra_symtab import ficheros_asm  # noqa: E402

A, S = 'SLES-53558-A124', 'SLUS-21351'
SIN = re.compile(r'^(func|fn|sub|lbl)_[0-9A-Fa-f]{8}$')


def firma(b, rel):
    n = len(b) // 4
    ws = struct.unpack('<%dI' % n, b[:n * 4])
    marc = {r // 4 for r, d in rel}
    return tuple(w & (0xFC000000 if (w >> 26) in (2, 3) else 0xFFFF0000) if k in marc else w
                 for k, w in enumerate(ws))


def objs(V):
    out = {}
    for p, unidad in ficheros_asm(V):
        base = os.path.splitext(unidad)[0] if unidad.endswith(('.cpp', '.c')) else unidad
        try:
            out[unidad] = pn.funciones(os.path.join(RAIZ, 'build', V, 'obj', base + '.o'))
        except (OSError, ValueError):
            pass
    return out


def main():
    minw = int(sys.argv[sys.argv.index('--min') + 1]) if '--min' in sys.argv else 4
    entradas = {}
    if '--entradas' in sys.argv:
        for l in open(sys.argv[sys.argv.index('--entradas') + 1], encoding='utf-8'):
            c = l.split('\t')
            if c[0].startswith('0x'):
                entradas[c[1]] = int(c[0], 16)          # glabel -> entrada de verdad
    nombres_s = set()
    for l in open(os.path.join(RAIZ, 'config', S, 'symbols.txt'), encoding='utf-8'):
        m = re.match(r'(\S+) = 0x', l)
        if m:
            nombres_s.add(m.group(1))
    oa, os_ = objs(A), objs(S)
    # indice de la A124: firma -> [nombres]
    ia = collections.defaultdict(list)
    for u, fs in oa.items():
        for n, lst in fs.items():
            if n.endswith('.NON_MATCHING'):
                continue
            for b, rel in lst:
                if len(b) >= 4 * minw:
                    ia[firma(b, rel)].append((n, u))
    filas = []
    for u, fs in os_.items():
        for g, lst in fs.items():
            if not SIN.match(g):
                continue
            b, rel = lst[0]
            x = int(g[5:], 16)
            if g in entradas:
                d = entradas[g] - x
                b, rel = b[d:], [(r - d, q) for r, q in rel if r >= d]
                x += d
            if len(b) < 4 * minw:
                continue
            cands = ia.get(firma(b, rel), [])
            nombres = sorted({n for n, ua in cands if not SIN.match(n)})
            if len(cands) == 1 and len(nombres) == 1 and nombres[0] not in nombres_s:
                filas.append((x, nombres[0], g, u, cands[0][1], len(b)))
    for x, n, g, u, ua, t in sorted(filas):
        print('0x%06x %-70s %5d B %-16s %s | A124 %s' % (x, n[:70], t, g, u.split('/')[-1], ua.split('/')[-1]))
    print('%d propuestas' % len(filas))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for x, n, g, u, ua, t in sorted(filas):
                fh.write('0x%x\t%s\tcodigo_a124\t%s\t%s\n' % (x, n, u, ua))


if __name__ == '__main__':
    main()
