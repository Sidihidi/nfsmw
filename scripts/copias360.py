#!/usr/bin/env python3
"""copias360.py -- funciones del original de Xbox 360 sin casar cuyo codigo solo esta en OTRA unidad nuestra (ronda 52).

LTCG deja en el original una copia de cada funcion en linea en cada unidad que la usa (tCubic en zCamera y zMisc,
StringKey en zAnim y zCamera...). Nuestro enlace LTCG deja UNA sola (la COMDAT va a la primera unidad que la usa), asi
que las demas copias del original no pueden casar con la medida por unidad. Esto las cuenta (minimo --min bytes, 16
por defecto): las de un solo nombre posible y las de varios. Contar estas copias en la medida es un cambio de criterio
(propuesta 1 de X52): lo decide el usuario; este script solo mide la bolsa.

    python scripts/copias360.py [--min B] [--lista N]
"""
import collections
import glob
import os
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if not os.path.isdir(os.path.join(RAIZ, 'config', 'EUROPEGERMILESTONE')):
    RAIZ = os.environ.get('NFSMW_RAIZ', os.getcwd())  # copia de trabajo fuera del arbol: la raiz es el directorio actual
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import nombra_grafo as ng  # noqa: E402

V = 'EUROPEGERMILESTONE'


def main():
    a = sys.argv[1:]
    minimo = int(a[a.index('--min') + 1]) if '--min' in a else 16
    lista = int(a[a.index('--lista') + 1]) if '--lista' in a else 30
    P = ng.X360()
    base_o = os.path.join(RAIZ, 'build', V, 'obj')
    base_s = os.path.join(RAIZ, 'build', V, 'ltcg', 'obj')
    U = {}
    for p in sorted(glob.glob(os.path.join(base_s, '**', '*.obj'), recursive=True)):
        rel = os.path.relpath(p, base_s)
        o = os.path.join(base_o, rel)
        if not os.path.exists(o):
            continue
        fo = ng.Coff(o).funciones()
        fs = ng.Coff(p).funciones()
        U[os.path.basename(rel)[:-4]] = ({n: P.esqueleto(*v) for n, v in fo.items()},
                                         {n: P.esqueleto(*v) for n, v in fs.items()}, fo)
    glob_s = collections.defaultdict(set)
    for u, (eo, es, fo) in U.items():
        for n, e in es.items():
            glob_s[e].add((u, n))
    tot = collections.Counter()
    tb = collections.Counter()
    con_nombre = cnb = 0
    ej = []
    for u, (eo, es, fo) in U.items():
        locales = set(es.values())
        for n, e in eo.items():
            if es.get(n) == e or e in locales:
                continue
            otros = glob_s.get(e)
            if not otros:
                continue
            t = len(fo[n][0]) * 4
            if t < minimo:
                continue
            nombres = {x for _, x in otros}
            k = 'un nombre' if len(nombres) == 1 else 'varios'
            tot[k] += 1
            tb[k] += t
            if not P.es_funcion_sin_nombre(n):
                con_nombre += 1
                cnb += t
            if len(ej) < lista and len(nombres) == 1:
                ej.append((u, n[:40], t, sorted(otros)[:2]))
    for k in tot:
        print('%-10s %5d funciones %8d B' % (k, tot[k], tb[k]))
    print('de ellas ya con nombre en el original: %d (%d B)' % (con_nombre, cnb))
    for x in ej:
        print(x)


if __name__ == '__main__':
    main()
