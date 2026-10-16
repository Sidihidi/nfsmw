#!/usr/bin/env python3
"""compara_informes.py -- compara dos informes de objdiff (report.json de pctall o de scripts/informe_prueba.py)
funcion a funcion: las casadas (100 %) que se ganan y las que se pierden, con bytes, y los totales de cada uno.

    python scripts/compara_informes.py ANTES.json DESPUES.json [--todas] [--tsv salida.tsv]

Cada funcion se identifica por (nombre, unidad, n-esima aparicion del nombre en la unidad por direccion): los estaticos
repetidos (__static_initialization_and_destruction_0, __tcf_0...) se cuentan por unidad, asi que anadir uno en otra
unidad no desplaza a los demas (con la clave global salian perdidas falsas). Sin --todas lista 60 de cada.
"""
import collections
import json
import sys


def carga(p):
    d = json.load(open(p))
    out = {}
    for u in sorted(d['units'], key=lambda x: x['name']):
        cuenta = collections.Counter()
        un = u['name'].split('/')[-1]
        for f in sorted(u.get('functions', []) or [], key=lambda x: int(x.get('address', 0) or 0)):
            n = f['name']
            # (nombre, unidad, n-esima aparicion en la unidad): los estaticos repetidos se cuentan por unidad
            k = (n, un, cuenta[n])
            cuenta[n] += 1
            out[k] = (u['name'], int(f.get('size', 0)), f.get('fuzzy_match_percent'))
    return d['measures'], out


def casada(v):
    return v is not None and v[2] is not None and v[2] >= 100.0


def main():
    a, b = sys.argv[1], sys.argv[2]
    ma, fa = carga(a)
    mb, fb = carga(b)
    for m, n in ((ma, a), (mb, b)):
        print('%-60s %s/%s B  %s/%s fn' % (n[-60:], m.get('matched_code'), m.get('total_code'),
                                           m.get('matched_functions'), m.get('total_functions')))
    ganadas = [(k, fb[k]) for k in fb if casada(fb[k]) and not casada(fa.get(k))]
    perdidas = [(k, fa[k]) for k in fa if casada(fa[k]) and not casada(fb.get(k))]
    print('ganadas %d (%d B), perdidas %d (%d B)' % (len(ganadas), sum(v[1] for k, v in ganadas),
                                                     len(perdidas), sum(v[1] for k, v in perdidas)))
    lim = None if '--todas' in sys.argv else 60
    print('--- perdidas')
    for k, v in sorted(perdidas, key=lambda x: x[1][0])[:lim]:
        print('  - %-70s %-45s %5d  ahora %s' % (k[0][:70], v[0][-45:], v[1], fb.get(k, (None, None, 'NO ESTA'))[2]))
    print('--- ganadas')
    for k, v in sorted(ganadas, key=lambda x: x[1][0])[:lim]:
        print('  + %-70s %-45s %5d  antes %s' % (k[0][:70], v[0][-45:], v[1], fa.get(k, (None, None, 'NO ESTABA'))[2]))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8') as fh:
            for s, lst in (('+', ganadas), ('-', perdidas)):
                for k, v in lst:
                    fh.write('%s\t%s\t%s\t%d\n' % (s, k[0], v[0], v[1]))


if __name__ == '__main__':
    main()
