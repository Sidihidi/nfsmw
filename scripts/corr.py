#!/usr/bin/env python3
"""corr.py V ETIQ [UNIDAD...] -- corridos.py sobre los JSON de objdiff que deja bancopc.py --conservar
(banco/d_<V>_<UNIDAD>_<ETIQ>.json): por clase, funciones con desplazamientos corridos una cantidad constante."""
import collections
import glob
import json
import os
import re
import sys

BANCO = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'build', 'bancopc')
X86 = re.compile(r'\[(?:e[a-d]x|e[sd]i|ebp)\+0x([0-9a-f]+)\]')
PPC = re.compile(r'\b0x([0-9a-f]+)\(r(?:[3-9]|[12][0-9]|3[01])\)')


def clase_de(nombre):
    m = re.match(r'\?[^@]+@([A-Za-z_][A-Za-z0-9_]*)@@', nombre)
    return m.group(1) if m else None


v, etiq = sys.argv[1], sys.argv[2]
rx = X86 if v == 'SPEED_EXE_1_3' else PPC
unidades = sys.argv[3:]
rutas = sorted(glob.glob(os.path.join(BANCO, 'd_%s_*_%s.json' % (v, etiq))))
for ruta in rutas:
    u = os.path.basename(ruta)[len('d_%s_' % v):-len('_%s.json' % etiq)]
    if unidades and u not in unidades:
        continue
    d = json.load(open(ruta, encoding='utf-8'))
    lados = {}
    for lado in ('left', 'right'):
        m = {}
        for s in d[lado]['symbols']:
            if s.get('instructions'):
                m[s.get('name', '')] = (s.get('match_percent'), s.get('size'),
                                        [(i.get('instruction') or {}).get('formatted', '') for i in s['instructions']])
        lados[lado] = m
    por_clase = collections.defaultdict(collections.Counter)
    funciones = collections.defaultdict(list)
    for nombre, (pct, tam, ins) in lados['left'].items():
        if nombre not in lados['right'] or pct is None or pct >= 100:
            continue
        _, tam2, ins2 = lados['right'][nombre]
        if len(ins) != len(ins2):
            continue
        difs = set()
        for a, b in zip(ins, ins2):
            oa = [int(x, 16) for x in rx.findall(a)]
            ob = [int(x, 16) for x in rx.findall(b)]
            if len(oa) == len(ob):
                difs.update(x - y for x, y in zip(oa, ob) if x != y)
        clase = clase_de(nombre)
        if clase and difs:
            for dd in difs:
                por_clase[clase][dd] += 1
            funciones[clase].append(nombre)
    print('== %s %s' % (v, u))
    for clase, cnt in sorted(por_clase.items(), key=lambda kv: -sum(kv[1].values()))[:25]:
        top = ', '.join('%+#x x%d' % (k, n) for k, n in cnt.most_common(4))
        print('  %-34s %3d fn  %s' % (clase[:34], len(funciones[clase]), top))
