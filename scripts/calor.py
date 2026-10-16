#!/usr/bin/env python3
"""calor.py V ETIQ [--min=N] [--unidad=U] -- los desplazamientos corridos mas repetidos en TODAS las funciones (no por
clase): para cada funcion no casada con el mismo numero de instrucciones en los dos lados, los pares (nuestro,
original) de [reg+X] que difieren; cuenta en cuantas funciones sale cada par y cuantos bytes suman esas funciones.
Un par que sale en muchas funciones es un miembro corrido de una estructura muy usada (EAX_CarState, FEScrollBar...),
aunque las funciones sean de clases distintas."""
import collections
import glob
import json
import os
import re
import sys

BANCO = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'build', 'bancopc')
X86 = re.compile(r'\[(?:e[a-d]x|e[sd]i|ebp)\+0x([0-9a-f]+)\]')
PPC = re.compile(r'\b0x([0-9a-f]+)\(r(?:[3-9]|[12][0-9]|3[01])\)')

v, etiq = sys.argv[1], sys.argv[2]
rx = X86 if v == 'SPEED_EXE_1_3' else PPC
minimo = 3
solo = None
for a in sys.argv[3:]:
    if a.startswith('--min='):
        minimo = int(a[6:])
    elif a.startswith('--unidad='):
        solo = a[9:].split(',')
pares = collections.defaultdict(set)
tam = {}
pct = {}
for ruta in sorted(glob.glob(os.path.join(BANCO, 'd_%s_*_%s.json' % (v, etiq)))):
    u = os.path.basename(ruta)[len('d_%s_' % v):-len('_%s.json' % etiq)]
    if solo and u not in solo:
        continue
    d = json.load(open(ruta, encoding='utf-8'))
    R = {s.get('name', ''): s for s in d['right']['symbols'] if s.get('instructions')}
    for s in d['left']['symbols']:
        n = s.get('name', '')
        p = s.get('match_percent')
        if not s.get('instructions') or n not in R or p is None or p >= 100:
            continue
        ia = [(i.get('instruction') or {}).get('formatted', '') for i in s['instructions']]
        ib = [(i.get('instruction') or {}).get('formatted', '') for i in R[n]['instructions']]
        if len(ia) != len(ib):
            continue
        clave = (u, n)
        tam[clave] = int(s.get('size') or 0)
        pct[clave] = p
        for a, b in zip(ia, ib):
            oa = [int(x, 16) for x in rx.findall(a)]
            ob = [int(x, 16) for x in rx.findall(b)]
            if len(oa) == len(ob):
                for x, y in zip(oa, ob):
                    if x != y:
                        pares[(y, x - y)].add(clave)
filas = sorted(pares.items(), key=lambda kv: -len(kv[1]))
for (nuestro, delta), fns in filas:
    if len(fns) < minimo:
        break
    ej = sorted(fns, key=lambda c: -pct[c])[:3]
    print('%#6x %+#6x  %3d fn %6d B   %s' % (nuestro, delta, len(fns), sum(tam[c] for c in fns),
                                         ' | '.join('%s %.1f' % (c[1][:50], pct[c]) for c in ej)))
