#!/usr/bin/env python3
"""anonimiza360.py -- nombres REPETIDOS dentro de una unidad de Xbox 360 sin ninguna copia casada -> fn_ (ronda 52).

En 360 un nombre puede estar en varias unidades (LTCG deja una copia de cada funcion en linea en cada unidad que la
usa: tCubic2D en zCamera y en zMisc), pero nunca dos veces en la misma. Si un nombre esta dos o mas veces en una
unidad y el informe no lo da al 100 % en ella, ninguna de las direcciones se puede defender: todas vuelven a fn_ y la
rutina de nombres (nombra_grafo.py, nombra_corrige360.py) decide por codigo y llamantes. Ejemplo de la ronda 52:
VU0_v3sub cuatro veces en zAI, GetNode, Scale, simsurface, RemoveListener.

Lee el porcentaje de build/versiones/EUROPEGERMILESTONE.json (la medida LTCG): pasalo justo despues de medir.

    python scripts/anonimiza360.py [--aplicar]
"""
import collections
import json
import os
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if not os.path.isdir(os.path.join(RAIZ, 'config', 'EUROPEGERMILESTONE')):
    RAIZ = os.environ.get('NFSMW_RAIZ', os.getcwd())  # copia de trabajo fuera del arbol: la raiz es el directorio actual
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import nombra_grafo as ng  # noqa: E402
from escritura import entero  # noqa: E402

V = 'EUROPEGERMILESTONE'


def porcentajes():
    rep = json.load(open(os.path.join(RAIZ, 'build', 'versiones', V + '.json')))
    pct = {}
    for u in rep['units']:
        for f in u.get('functions') or []:
            pct[(u['name'].split('/')[-1], f['name'])] = f.get('fuzzy_match_percent')
    return pct


def main():
    P = ng.X360()
    pct = porcentajes()
    cambios = []
    for n, ds in P.por_nombre.items():
        pu = collections.defaultdict(list)
        for d in ds:
            if d in P.texto:
                u = P.unidad_de(d)
                if u:
                    pu[u].append(d)
        for u, l in pu.items():
            if len(l) > 1 and pct.get((u.split('/')[-1], n)) != 100.0:
                cambios += [(d, n) for d in l]
    for d, n in sorted(cambios):
        print('%08X %s' % (d, n[:90]))
    print('repetidos en una unidad sin copia casada: %d direcciones' % len(cambios))
    if '--aplicar' in sys.argv and cambios:
        lin = P.lineas
        for d, n in cambios:
            i = [e for e in P.por_dir[d] if e[0] == n and e[1] == '.text'][0][2]
            assert lin[i].startswith(n + ' = .text:0x%08X;' % d)
            lin[i] = 'fn_%08X' % d + lin[i][len(n):]
        entero(os.path.join(RAIZ, 'config', V, 'symbols.txt'), 'w', encoding='utf-8', newline='\n').write('\n'.join(lin))
        print('aplicado', len(cambios))


if __name__ == '__main__':
    main()
