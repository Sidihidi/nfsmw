#!/usr/bin/env python3
"""conmuta.py -- las casi exactas de PS2 que solo fallan por el ORDEN de los operandos de una operacion conmutativa.

GameCube no ve ese orden: `a * b + c` y `c + a * b` dan el mismo `fmadds`, y `a + b` y `b + a` el mismo `add`
cuando el reparto de registros coincide. La EE si: `add.s d, s, t` guarda en que orden venian los sumandos. Asi
salieron DoWheelForces (tres suspensiones, `penetration + rideheight * upness`) y PerfStats::Fetch
(`min_w + ...`) el 2026-09-23: una sola fila distinta cada una, y la forma buena la da el propio original.

Lista las funciones cuyas filas distintas son TODAS la misma instruccion conmutativa con los mismos operandos en
otro orden, y la fila de cada una, para ir al mapa de lineas del original (scratchpad lineas_a124.py, o el .s).
Usa la cache de diffs de scripts/casi.py (build/<V>/casi).

    python scripts/conmuta.py SLES-53558-A124
    python scripts/conmuta.py SLUS-21351 --unidad zPhysics
"""
import argparse
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import casi  # noqa: E402

RAIZ = casi.RAIZ
CONMUTATIVAS = {'add.s', 'mul.s', 'max.s', 'min.s', 'c.eq.s', 'addu', 'daddu', 'add', 'dadd', 'and', 'or',
                'xor', 'nor', 'mult', 'multu', 'mul', 'pmaxw', 'pminw', 'paddw', 'vadd', 'vmul'}


def partes(txt):
    m = re.match(r'(\S+)\s+(.*)', txt)
    if not m:
        return txt, []
    return m.group(1), [o.strip() for o in m.group(2).split(',')]


def es_cambio_de_orden(a, b):
    """La misma instruccion conmutativa, el mismo destino y los mismos fuentes en otro orden."""
    ma, oa = partes(a)
    mb, ob = partes(b)
    base = ma.split('.')[0] if ma.startswith(('vadd', 'vmul')) else ma
    if ma != mb or base not in CONMUTATIVAS or len(oa) != len(ob) or oa == ob:
        return False
    if ma.startswith('c.'):
        return sorted(oa) == sorted(ob)
    return oa[0] == ob[0] and sorted(oa[1:]) == sorted(ob[1:])


def main():
    p = argparse.ArgumentParser()
    p.add_argument('version')
    p.add_argument('--unidad', default=None)
    p.add_argument('--min', type=float, default=90.0)
    args = p.parse_args()

    cache = os.path.join(RAIZ, 'build', args.version, 'casi')
    os.makedirs(cache, exist_ok=True)
    with open(os.path.join(RAIZ, 'build', args.version, 'report.json'), encoding='utf-8') as fh:
        rep = json.load(fh)
    quiero = {}
    for u in rep.get('units', []):
        for f in u.get('functions', []) or []:
            pct = f.get('fuzzy_match_percent') or 0.0
            if args.min <= pct < 100.0:
                quiero.setdefault(u['name'].split('/')[-1], {})[f['name']] = (pct, int(f.get('size', 0) or 0))

    total = 0
    bytes_ = 0
    for nombre, rel in casi.unidades(args.version):
        if args.unidad and args.unidad.lower() not in nombre.lower():
            continue
        if nombre not in quiero:
            continue
        d = casi.diff_unidad(args.version, rel, cache)
        if not d:
            print('aviso: objdiff no pudo con %s' % rel, file=sys.stderr)
            continue
        izq = dict((s.get('name'), s) for s in d['left']['symbols'])
        der = dict((s.get('name'), s) for s in d['right']['symbols'])
        for sim, (pct, tam) in sorted(quiero[nombre].items()):
            a, b = izq.get(sim), der.get(sim)
            if not a or not b:
                continue
            la, lb = a.get('instructions', []), b.get('instructions', [])
            if len(la) != len(lb):
                continue
            filas, _, detalle = casi.clasifica(la, lb)
            if not detalle or not all(es_cambio_de_orden(x, y) for _, x, y in detalle):
                continue
            total += 1
            bytes_ += tam
            print('%6d B  %8.4f %%  %-16s %s' % (tam, pct, nombre, sim))
            for k, x, y in detalle:
                print('            fila %4d  original: %-34s nuestro: %s' % (k, x, y))
    print('%d funciones, %d B' % (total, bytes_))


if __name__ == '__main__':
    main()
