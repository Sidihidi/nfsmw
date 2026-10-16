#!/usr/bin/env python3
"""inlines.py -- que inline de cabecera tiene la culpa de las casi-exactas de la A124.

Nuestros objetos de PS2 llevan .mdebug, asi que cada funcion dice que lineas de que cabeceras expande. Si una linea
de un inline la expanden muchas funciones y NINGUNA casa (y varias se quedan en el 97-99,9 %), el sospechoso es el
inline, no las funciones: se arregla en un sitio y casan todas. Asi salieron, el 2026-09-22:

    bMath.hpp  bAbs(int)      10 funciones, 0 al 100 %: el original hace `negu` + `pmaxw` (MMI), no un salto
    bMath.hpp  bMin(int)      44 funciones, 0 al 100 %: el original da `pminw c, b, a`, con los operandos al reves
    AnimUtil.h FloatToInt     34 funciones, 0 al 100 %: el original convierte siempre en $f4 (asm de EA)

    python scripts/inlines.py global [MIN]              (cabecera, linea) con MIN funciones o mas (12 por defecto),
                                                        de menos a mas casadas
    python scripts/inlines.py usos CABECERA L0 [L1]     las funciones que expanden esas lineas, con su porcentaje

Las lineas son las de NUESTRA cabecera cuando se compilo el objeto (el .o de build/SLES-53558-A124/src), no las de
ahora ni las del original. El porcentaje es el de build/versiones/SLES-53558-A124.json. Ojo con dos trampas: una
linea sin ninguna funcion casada puede ser tambien la de un inline que solo usan funciones grandes (bIdentity,
bCopy de matrices: nada casa alli por otras razones), y las funciones sin porcentaje ("-") son copias fuera de linea
que el original no tiene (las de UTLVector.h), que no suman nada.
"""
import collections
import json
import os
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import relops  # noqa: E402

V = 'SLES-53558-A124'


def informe():
    rep = json.load(open(os.path.join(RAIZ, 'build', 'versiones', V + '.json'), encoding='utf-8'))
    pct = {}
    for u in rep['units']:
        for f in u.get('functions', []) or []:
            pct[f['name']] = (f.get('fuzzy_match_percent'), int(f.get('size', 0) or 0), u['name'].split('/')[-1])
    return pct


def expansiones():
    """{(cabecera, linea): {funcion}} de nuestros objetos, sin el fichero principal de cada funcion"""
    por = collections.defaultdict(set)
    for o in relops.nuestros_objetos():
        tb = relops.objeto(o)
        if tb is None:
            continue
        for s in tb.idx:
            lms = tb.lms(s)
            if not lms:
                continue
            princ = next((f for f, l, a in lms if f.lower().endswith(('.cpp', '.c'))), None)
            for f, l, a in lms:
                if f != princ and f.endswith(('.h', '.hpp', '.inl')):
                    por[(f.split('/')[-1], l)].add(s)
    return por


def cmd_global(minimo):
    pct = informe()
    filas = []
    for (cab, l), fs in expansiones().items():
        if len(fs) < minimo:
            continue
        ok = sum(1 for s in fs if (pct.get(s) or (None,))[0] == 100.0)
        casi = sum(1 for s in fs if 97.0 <= ((pct.get(s) or (None,))[0] or 0) < 100.0)
        filas.append((ok / len(fs), -casi, cab, l, len(fs), ok, casi))
    filas.sort()
    for _, _, cab, l, n, ok, casi in filas[:80]:
        print('%-30s %5d  funciones=%4d  al 100 %%=%4d  entre 97 y 100=%4d' % (cab[:30], l, n, ok, casi))


def cmd_usos(cab, l0, l1):
    pct = informe()
    fs = set()
    for (c, l), s in expansiones().items():
        if c == cab and l0 <= l <= l1:
            fs |= s
    filas = sorted(fs, key=lambda s: -((pct.get(s) or (None,))[0] or -1))
    for s in filas:
        p, tam, u = pct.get(s) or (None, 0, '?')
        print('%8s %6s  %-14s %s' % ('%.2f' % p if p is not None else '-', tam, u, s))
    print('%d funciones; %d al 100 %%' % (len(filas), sum(1 for s in filas if (pct.get(s) or (None,))[0] == 100.0)))


def main():
    if len(sys.argv) >= 2 and sys.argv[1] == 'global':
        cmd_global(int(sys.argv[2]) if len(sys.argv) > 2 else 12)
    elif len(sys.argv) >= 4 and sys.argv[1] == 'usos':
        l0 = int(sys.argv[3])
        cmd_usos(sys.argv[2], l0, int(sys.argv[4]) if len(sys.argv) > 4 else l0)
    else:
        print(__doc__)
        sys.exit(2)


if __name__ == '__main__':
    main()
