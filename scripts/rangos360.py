#!/usr/bin/env python3
"""rangos360.py -- a que unidad nuestra se parece cada rango .text de splits.txt de Xbox 360 (ronda 52).

El troceado de 360 se dedujo (ronda 41) y tiene cortes mal puestos: un rango que el original tiene en una unidad pero
cuyas funciones son, byte a byte (esqueleto), las de OTRA unidad nuestra. Ejemplos de la ronda 52: 0x821FB1E8-
0x821FC960 de zEAXSound era EAXCar.cpp (zEAXSound2); zFoundation desde 0x82171638 era el principio de zCamera.

  python scripts/rangos360.py
      por cada rango: funciones con esqueleto conocido, cuantas son de la propia unidad y de cuales otras. Lista los
      rangos pequenos (< 64 kB) y los que se parecen mas a otra unidad.
  python scripts/rangos360.py uno UNIDAD A0 A1 OTRA
      detalle de [A0,A1) de UNIDAD: porcentaje de cada funcion hoy, si nuestro objeto de UNIDAD la define y con que
      funciones de OTRA comparte esqueleto. Antes de mover un rango: los llamantes (scripts/llamantes360.py) y la
      .pdata va con su texto.
Lee build/versiones/EUROPEGERMILESTONE.json (medida LTCG) y los objetos de build/EUROPEGERMILESTONE/{obj,ltcg/obj}.
"""
import collections
import glob
import json
import os
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if not os.path.isdir(os.path.join(RAIZ, 'config', 'EUROPEGERMILESTONE')):
    RAIZ = os.environ.get('NFSMW_RAIZ', os.getcwd())  # copia de trabajo fuera del arbol: la raiz es el directorio actual
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import nombra_grafo as ng  # noqa: E402

V = 'EUROPEGERMILESTONE'
SL = os.path.join('Speed', 'Indep', 'SourceLists')


def todos():
    P = ng.X360()
    por = collections.defaultdict(set)
    for p in sorted(glob.glob(os.path.join(RAIZ, 'build', V, 'ltcg', 'obj', '**', '*.obj'), recursive=True)):
        u = os.path.basename(p)[:-4]
        for n, v in ng.Coff(p).funciones().items():
            if len(v[0]) >= 6:
                por[P.esqueleto(*v)].add(u)
    for p in sorted(glob.glob(os.path.join(RAIZ, 'build', V, 'obj', 'Speed', '**', '*.obj'), recursive=True)):
        u = os.path.basename(p)[:-4]
        fo = ng.Coff(p).funciones()
        for a, b in [(a, b) for a, b, x in P.rangos if x.endswith('/' + u)]:
            c = collections.Counter()
            n = 0
            for nm, v in fo.items():
                ds = [d for d in P.por_nombre.get(nm, []) if a <= d < b]
                if not ds and P.dir_de(nm) is not None and a <= P.dir_de(nm) < b:
                    ds = [P.dir_de(nm)]
                if not ds:
                    continue
                n += 1
                for x in por.get(P.esqueleto(*v), set()):
                    c[x] += 1
            if (b - a) < 0x10000 or (c and c.most_common(1)[0][0] != u):
                print('%-18s %08X-%08X %6X fns=%4d propia=%4d  otras=%s' % (
                    u, a, b, b - a, n, c.get(u, 0), [(k, v) for k, v in c.most_common(4) if k != u]))


def uno(u, a0, a1, v):
    P = ng.X360()
    rep = json.load(open(os.path.join(RAIZ, 'build', 'versiones', V + '.json')))
    pct = {}
    for x in rep['units']:
        for f in x.get('functions') or []:
            pct[(x['name'].split('/')[-1], f['name'])] = f.get('fuzzy_match_percent')
    fo = ng.Coff(os.path.join(RAIZ, 'build', V, 'obj', SL, u + '.obj')).funciones()
    su = ng.Coff(os.path.join(RAIZ, 'build', V, 'ltcg', 'obj', SL, u + '.obj')).funciones()
    sv = ng.Coff(os.path.join(RAIZ, 'build', V, 'ltcg', 'obj', SL, v + '.obj')).funciones()
    ev = collections.defaultdict(list)
    for n, x in sv.items():
        ev[P.esqueleto(*x)].append(n)
    filas = []
    for nm, x in fo.items():
        ds = [d for d in P.por_nombre.get(nm, []) if a0 <= d < a1]
        if not ds and P.dir_de(nm) is not None and a0 <= P.dir_de(nm) < a1:
            ds = [P.dir_de(nm)]
        if ds:
            filas.append((ds[0], 4 * len(x[0]), nm, pct.get((u, nm)), nm in su, ev.get(P.esqueleto(*x), [])))
    for d, t, nm, p, ens, cv in sorted(filas):
        print('%08X %4d %-60s %-7s %-8s %s' % (d, t, nm[:60], p, 'nuestroU' if ens else '',
                                               ('%s:' % v) + ','.join(c[:40] for c in cv[:2]) if cv else ''))


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'uno':
        uno(sys.argv[2], int(sys.argv[3], 16), int(sys.argv[4], 16), sys.argv[5])
    else:
        todos()
