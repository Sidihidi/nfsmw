#!/usr/bin/env python3
"""iguales360.py PAREJAS.txt... [--casi N] [--enlace DIR] -- de las parejas de parecidas360.py cuya fn_ del EGM sigue
sin nombre, las IGUALES a la nuestra salvo reubicaciones (lis y los addi/lwz/stw... que usan su registro, destinos de
b/bl) y cuyo nombre no esta en symbols.txt ni sale dos veces: imprime `0xVA<TAB>nombre<TAB>tam`, la entrada de
renombra360.py. Con --casi N, en vez de eso, las que difieren en <= N instrucciones, con el diff (lo que falta).

    python scripts/iguales360.py par_zfe.txt par_zonline.txt > n_iguales.tsv
    python scripts/iguales360.py par_zfe.txt --casi 6
"""
import collections
import difflib
import re
import sys

import egm360


def norm(img, va, n):
    r = []
    lis = set()
    for x in img.ins(va, n):
        m, op = x.mnemonic, x.op_str
        if m == 'lis':
            lis.add(op.split(',')[0])
            r.append('lis ' + op.split(',')[0])
            continue
        ops = [t.strip() for t in op.split(',')]
        if m.startswith('b') and m not in ('blr', 'bctr', 'bctrl', 'blrl'):
            r.append(m)
            continue
        if (len(ops) >= 2 and any(ops[-1].endswith('(%s)' % rr) for rr in lis)) or \
                (m == 'addi' and len(ops) == 3 and ops[1] in lis):
            r.append(m + ' ' + ops[0] + ' RELOC')
            continue
        r.append(m + ' ' + op)
    return r


def main():
    argv = sys.argv[1:]
    d = egm360.enlace(argv)
    casi = None
    if '--casi' in argv:
        i = argv.index('--casi')
        casi = int(argv[i + 1])
        del argv[i:i + 2]
    E, N = egm360.egm(), egm360.nuestro(d)
    addr = {}
    for a, n, o, t in egm360.mapa(d):
        addr.setdefault(n, a)
    usados, nomdir = {}, {}
    for a, n, t in egm360.simbolos_egm():
        usados[n] = a
        nomdir[a] = n
    cand = []
    for f in argv:
        for linea in open(f):
            if linea.startswith('#') or not linea.strip():
                continue
            a, tam, ra, n = linea.split(None, 3)
            n, a, tam = n.strip(), int(a, 16), int(tam)
            if not nomdir.get(a, '').startswith('fn_') or n not in addr:
                continue
            e, o = norm(E, a, tam), norm(N, addr[n], tam)
            if casi is not None:
                if e and o and e != o:
                    dd = [x for x in difflib.ndiff(e, o) if x[:1] in '+-']
                    if len(dd) <= casi:
                        print('0x%08X %5d %2d %s | %s' % (a, tam, len(dd), n[:70], ' ; '.join(dd)[:200]))
                continue
            if n not in usados and e and e == o:
                cand.append((a, n, tam))
    cuenta = collections.Counter(n for _, n, _ in cand)
    for a, n, tam in cand:
        if cuenta[n] == 1:
            print('0x%08X\t%s\t%d' % (a, n, tam))
        else:
            print('# repetido 0x%08X %s' % (a, n), file=sys.stderr)


if __name__ == '__main__':
    main()
