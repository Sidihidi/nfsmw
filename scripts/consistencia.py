#!/usr/bin/env python3
"""consistencia.py -- scripts/prueba_nombre.py sobre TODAS las funciones con nombre del troceado de SLUS: cuenta las
IGUAL y las distintas, y lista aparte las que SOLO difieren en el destino con nombre de algun j/jal (grafo de
llamadas): o el nombre de la funcion o el de su destino esta mal (o el llamador es otra version de la funcion).

    python scripts/consistencia.py [--tsv salida.tsv]

Imprime la cuenta ({'IGUAL': N, 'grafo': N, 'distinta': N, 'tamano': N}). --tsv vuelca una fila por funcion que no
es IGUAL: direccion, nombre y resultado; las de grafo, una fila por destino con la palabra, el destino del original y
el nuestro. Sirve para comparar dos symbols.txt candidatos (antes y despues de un lote de nombres) sin medir: las
`grafo` que aparecen o desaparecen dicen que nombre se ha movido bien o mal.

Lee el troceado (build/SLUS-21351/asm y obj) y nuestros objetos (build/SLUS-21351/src) de la raiz donde este el guion.
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
import prueba_lote  # noqa: E402

V = 'SLUS-21351'
SIN = re.compile(r'^(func|fn|sub|lbl|D)_[0-9A-Fa-f]{8}$')


def main():
    pr = prueba_lote.Probador(V)
    cuenta = collections.Counter()
    filas = []
    for x, (g, unidad) in sorted(pr.glabel.items()):
        if SIN.match(g):
            continue
        t, o = pr.objs(unidad)
        if g not in t or g not in o:
            continue
        tb = t[g][0]
        mejor = None
        for ob in o[g]:
            r, dif = pn.compara(tb, ob)
            if r == 'IGUAL':
                mejor = ('IGUAL', [])
                break
            if mejor is None:
                mejor = (r, dif, ob)
        if mejor[0] == 'IGUAL':
            cuenta['IGUAL'] += 1
            continue
        r = mejor[0]
        if r.startswith('tamano'):
            cuenta['tamano'] += 1
            filas.append('0x%x\t%s\t%s' % (x, g, r))
            continue
        dif, ob = mejor[1], mejor[2]
        rt = {k // 4: d for k, d in tb[1]}
        ro = {k // 4: d for k, d in ob[1]}
        grafo = [(k, rt.get(k), ro.get(k)) for k, a, b in dif if (a >> 26) in (2, 3) and (a & 0xFC000000) ==
                 (b & 0xFC000000) and rt.get(k) and ro.get(k)]
        if len(grafo) == len(dif):
            cuenta['grafo'] += 1
            for k, a, b in grafo:
                filas.append('0x%x\t%s\tgrafo\t%d\t%s\t%s' % (x, g, k, a, b))
        else:
            cuenta['distinta'] += 1
            filas.append('0x%x\t%s\t%s' % (x, g, r))
    print(dict(cuenta))
    if '--tsv' in sys.argv:
        open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n').write('\n'.join(filas) + '\n')


if __name__ == '__main__':
    main()
