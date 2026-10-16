#!/usr/bin/env python3
"""grafo_nuestro.py -- nombres de SLUS por el GRAFO DE LLAMADAS DE NUESTRO CODIGO.

Testigo: una funcion de SLUS con nombre cuyo codigo es IGUAL (scripts/prueba_nombre.py) a nuestra funcion de ese
nombre. Como el codigo es el mismo palabra a palabra, cada reubicacion del original apunta a lo mismo que la nuestra
en esa posicion: si el original llama (j/jal) o toma la direccion (%hi/%lo) de una funcion SIN nombre (func_X, o una
entrada dentro de una func_ que splat marca con alabel) y la nuestra de un nombre N, X es N. Se juntan todos los
testigos; se propone N para X si todos dicen lo mismo, N no esta ya en symbols.txt y X no tiene nombre.

    python scripts/grafo_nuestro.py [--tsv propuestas.tsv]

Imprime las propuestas (direccion, nombre, testigos, glabel actual o "(dentro)"), las que tienen el nombre ya en otra
direccion (casi siempre hermanas de codigo identico: no se tocan sin otra prueba) y los conflictos (dos nombres para una
direccion: sobrecargas que nuestro llamador elige distinto). --tsv acepta scripts/aplica_nombres.py. Es iterativo:
cada vuelta de nombres crea testigos nuevos. Tanda 6 (2026-09-23): 188 propuestas en la primera vuelta y 6 en la
segunda. Se complementa con scripts/grafo_nombres.py (testigos de la A124).
"""
import collections
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
import prueba_lote  # noqa: E402
import prueba_nombre as pn  # noqa: E402

S = 'SLUS-21351'
SIN = re.compile(r'^(func|fn|sub|lbl|D)_[0-9A-Fa-f]{8}$')
FUNC = re.compile(r'^func_([0-9A-F]{8})$')


def main():
    pr = prueba_lote.Probador(S)
    nombres_s = collections.defaultdict(list)
    for l in open(os.path.join(RAIZ, 'config', S, 'symbols.txt'), encoding='utf-8'):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);', l)
        if m:
            nombres_s[m.group(1)].append(int(m.group(2), 16))
    votos = collections.defaultdict(collections.Counter)
    testigos = collections.defaultdict(list)
    unidades = sorted({u for g, u in pr.glabel.values()})
    iguales = 0
    for u in unidades:
        t, o = pr.objs(u)
        for g, lt in t.items():
            if SIN.match(g) or g.endswith('.NON_MATCHING') or g not in o:
                continue
            tb = lt[0]
            for ob in o[g]:
                if len(ob[0]) != len(tb[0]):
                    continue
                r, dif = pn.compara(tb, ob)
                if r != 'IGUAL':
                    continue
                iguales += 1
                ro = {k: d for k, d in ob[1]}
                for k, dt in tb[1]:
                    m = FUNC.match(dt or '')
                    if not m:
                        continue
                    do = ro.get(k)
                    if not do or do.startswith(('.', '$')) or SIN.match(do):
                        continue
                    x = int(m.group(1), 16)
                    votos[x][do] += 1
                    testigos[x].append(g)
                break
    filas, conflictos, ya = [], [], []
    for x, c in sorted(votos.items()):
        if len(c) > 1:
            conflictos.append((x, c))
            continue
        (n, k), = c.items()
        if n in nombres_s:
            if x not in nombres_s[n]:
                ya.append((x, n, nombres_s[n], k))
            continue
        filas.append((x, n, k, sorted(set(testigos[x]))[:3]))
    print('testigos IGUAL: %d' % iguales)
    for x, n, k, ts in filas:
        g = pr.glabel.get(x, ('(dentro)', ''))[0]
        print('0x%06x %-70s x%-3d %-16s %s' % (x, n[:70], k, g, ', '.join(t[:30] for t in ts)))
    print('--- nombre ya en otra direccion')
    for x, n, ds, k in ya:
        print('0x%06x %-70s x%-3d YA EN %s' % (x, n[:70], k, ','.join('0x%x' % d for d in ds)))
    print('--- conflictos')
    for x, c in conflictos:
        print('0x%06x %s' % (x, dict(c)))
    print('%d propuestas, %d con el nombre ya en otra direccion, %d conflictos' % (len(filas), len(ya),
                                                                                  len(conflictos)))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for x, n, k, ts in filas:
                fh.write('0x%x\t%s\tgrafo_nuestro\t%d\t%s\n' % (x, n, k, ','.join(ts)))


if __name__ == '__main__':
    main()
