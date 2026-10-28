#!/usr/bin/env python3
"""parecidas360.py OBJETO LO HI [--min 0.85] [--enlace DIR] -- para cada fn_ del EGM en [LO, HI) busca la funcion de
nuestro enlace LTCG (del objeto OBJETO del .map, p. ej. zFe.obj) mas parecida por mnemonicos (difflib) y de tamano
parecido, entre los nombres que symbols.txt aun no usa; imprime `VA tam ratio nombre` para las de ratio >= min.

Es la entrada de deltas360.py (desplazamientos que cambian) y de iguales360.py (iguales salvo reubicaciones).

    python scripts/parecidas360.py zFe.obj 0x82272AD0 0x822958C0 --min 0.80 > par_zfe.txt
"""
import difflib
import sys

import egm360


def main():
    argv = sys.argv[1:]
    d = egm360.enlace(argv)
    mn = 0.85
    if '--min' in argv:
        i = argv.index('--min')
        mn = float(argv[i + 1])
        del argv[i:i + 2]
    objeto, lo, hi = argv[0], int(argv[1], 16), int(argv[2], 16)
    E, N = egm360.egm(), egm360.nuestro(d)
    sims = egm360.simbolos_egm()
    eg = [(a, n, t) for a, n, t in sims if lo <= a < hi]
    nu = [(a, n, t) for a, n, o, t in egm360.mapa(d) if o == objeto]
    mns = {n: [x.mnemonic for x in N.ins(a, t)] for a, n, t in nu}
    usados = set(n for _, n, _ in sims if not n.startswith('fn_'))
    for a, n, t in eg:
        if not n.startswith('fn_') or t < 16:
            continue
        e = [x.mnemonic for x in E.ins(a, t)]
        if not e:
            continue
        mejor = (0, None)
        for b, nn, t2 in nu:
            if nn in usados or abs(t2 - t) > max(32, t // 5) or not mns[nn]:
                continue
            r = difflib.SequenceMatcher(None, e, mns[nn], autojunk=False).ratio()
            if r > mejor[0]:
                mejor = (r, nn)
        if mejor[0] >= mn:
            print('%08X %5d %.3f %s' % (a, t, mejor[0], mejor[1]))


if __name__ == '__main__':
    main()
