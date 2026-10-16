#!/usr/bin/env python3
"""Catalogo de los bloques VU0 del original: cada expansion en linea de
UVectorMath.hpp, con su forma exacta y cuantas veces sale.

Un bloque es una racha de instrucciones de COP2 (lqc2/sqc2/qmtc2/qmfc2 y las
`v*') sin nada por medio salvo el relleno que el planificador mete.  Como cada
inline de VU0 se expande a una racha de estas, el catalogo ES la lista de los
cuerpos que hay que escribir, sacada del binario y no de la memoria.

  python scripts/vu0bloques.py SLES-53558-A124              # todas las unidades
  python scripts/vu0bloques.py SLES-53558-A124 --unidad zDynamics
  python scripts/vu0bloques.py SLES-53558-A124 --lado src   # los nuestros
"""
import argparse
import collections
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import histofn

RE_INSN = re.compile(r'^\s*([0-9a-f]+):\s+([a-z][a-z0-9_.]*)\s*(.*)$')
COP2 = re.compile(r'^(v[a-z]|lqc2|sqc2|qmtc2|qmfc2|cfc2|ctc2)')

# Las direcciones y los registros de proposito general cambian en cada sitio;
# la FORMA no.  Se normaliza a mnemonico + registros de VU0.
RE_VF = re.compile(r'\$(vf\d+|ACC|Q|I|R)')


def forma(mnem, ops):
    vf = RE_VF.findall(ops)
    return mnem + (' ' + ','.join(vf) if vf else '')


def main():
    p = argparse.ArgumentParser()
    p.add_argument('version')
    p.add_argument('--unidad', default=None)
    p.add_argument('--lado', default='obj', choices=('obj', 'src'))
    p.add_argument('--n', type=int, default=30)
    p.add_argument('--min', type=int, default=2, help='instrucciones minimas')
    args = p.parse_args()

    objdump = os.path.join(histofn.RAIZ,
                           histofn.OBJDUMP.get(args.version, histofn.PPC))
    bloques = collections.Counter()
    ejemplo = {}
    for unidad, orig, nuestro in histofn.objetos(args.version, args.unidad):
        ruta = orig if args.lado == 'obj' else nuestro
        salida = subprocess.run([objdump, '-d', '--no-show-raw-insn', ruta],
                                capture_output=True, text=True).stdout
        actual, hueco = [], 0
        for linea in salida.splitlines():
            m = RE_INSN.match(linea)
            if not m:
                continue
            dir_, mnem, ops = m.group(1), m.group(2), m.group(3)
            if COP2.match(mnem):
                actual.append(forma(mnem, ops))
                hueco = 0
            elif actual:
                # El planificador mete hasta dos instrucciones por medio.
                hueco += 1
                if hueco > 2:
                    if len(actual) >= args.min:
                        k = ' | '.join(actual)
                        bloques[k] += 1
                        ejemplo.setdefault(k, '%s %s' % (unidad, dir_))
                    actual, hueco = [], 0
        if len(actual) >= args.min:
            bloques[' | '.join(actual)] += 1

    print('%6s  %s' % ('veces', 'forma'))
    for k, n in bloques.most_common(args.n):
        print('%6d  %s' % (n, k))
        print('        (%s)' % ejemplo.get(k, '?'))
    print('\n%d formas distintas, %d bloques' % (len(bloques), sum(bloques.values())))


if __name__ == '__main__':
    main()
