#!/usr/bin/env python3
"""tri_calibra.py NUESTRO_LTCG.map NUESTRO_LTCG.exe [N] -- tasa de triangulacion (la de tri_orden.py) de los nombres
VERDADEROS: funciones de X360 que ya tienen nombre y cuyo nombre define nuestra fuente; muestra aleatoria de N.
Pasada a scripts/ en la ronda 78 (procedencia: docs/analisis/2026-10-21_carbonps3.md); sin rutas de la maquina.
"""
import collections
import difflib
import os
import random
import sys

from carbon import RAIZ, undname, lee_symbols_txt, NOMBRE_AUTO

from ppcsk import Exe, esqueleto  # noqa: E402
from cruce_nombres import lee_mapa  # noqa: E402

if __name__ == '__main__' and (len(sys.argv) < 3 or sys.argv[1] in ('-h', '--help')):
    print(__doc__)
    sys.exit(0)


def palabras(sk):
    return [sk[i:i + 4] for i in range(0, len(sk), 4)]


def main():
    nmap, nexe = sys.argv[1:3]
    n = int(sys.argv[3]) if len(sys.argv) > 3 else 600
    mw = Exe(os.path.join(RAIZ, 'orig', 'EUROPEGERMILESTONE', 'NFS.exe'))
    nu_exe = Exe(nexe, base_ejec=0x400000)
    nu = lee_mapa(nmap)
    vs = sorted(nu)
    nu_n = collections.defaultdict(list)
    for i, va in enumerate(vs):
        sz = min((vs[i + 1] if i + 1 < len(vs) else va + 4) - va, 0x4000)
        nu_n[nu[va][0]].append((va, sz))
    mwl = sorted((va, sz, nm) for nm, va, sz in lee_symbols_txt(os.path.join(RAIZ, 'config', 'EUROPEGERMILESTONE', 'symbols.txt')))
    tam = {va: sz for va, sz, nm in mwl}
    orden = [va for va, sz, nm in mwl]
    pos = {va: i for i, va in enumerate(orden)}
    pool = [(va, nm) for va, sz, nm in mwl if not NOMBRE_AUTO.match(nm) and len(nu_n.get(nm, [])) == 1 and sz >= 24]
    random.seed(11)
    sm = random.sample(pool, min(n, len(pool)))
    c = collections.Counter()
    for va, nm in sm:
        a, sz = nu_n[nm][0]
        b = esqueleto(nu_exe, a, sz)[0]
        while b.endswith(bytes(4)):
            b = b[:-4]
        b = palabras(b)
        if len(b) < 6:
            continue
        i = pos[va]
        vec = [v for v in orden[max(0, i - 30):i + 31] if 0.75 * len(b) <= tam[v] // 4 <= 1.25 * len(b)]
        punt = sorted(((difflib.SequenceMatcher(None, b, palabras(esqueleto(mw, v, tam[v])[0]), autojunk=False).ratio(), v)
                       for v in vec), reverse=True)[:2]
        c['n'] += 1
        if punt:
            seg = punt[1][0] if len(punt) > 1 else 0.0
            c['si' if punt[0][1] == va and punt[0][0] - seg >= 0.05 else 'no'] += 1
    print('nombres verdaderos de X360 con nuestra funcion (muestra %d): triangulada si %d, no %d (%.1f %%)' % (
        c['n'], c['si'], c['no'], 100.0 * c['si'] / max(c['n'], 1)))


if __name__ == '__main__':
    main()
