#!/usr/bin/env python3
"""deltas360.py PAREJAS.txt [--enlace DIR] -- para las parejas de parecidas360.py con los MISMOS mnemonicos (ratio
>= 0,999), las diferencias de operandos: desplazamientos d(rN) (OFF), inmediatos (IMM) y mnemonicos (MN). Al final,
los desplazamientos que cambian resumidos por clase: un corrimiento constante en una clase es un miembro de mas o de
menos en la disposicion de Xbox 360 (luego se mide con la sonda de layout).

    python scripts/deltas360.py par_zonline.txt
"""
import collections
import re
import sys

import egm360


def main():
    argv = sys.argv[1:]
    d = egm360.enlace(argv)
    E, N = egm360.egm(), egm360.nuestro(d)
    addr = {}
    for a, n, o, t in egm360.mapa(d):
        addr.setdefault(n, a)
    porclase = collections.defaultdict(collections.Counter)
    for linea in open(argv[0]):
        if linea.startswith('#') or not linea.strip():
            continue
        a, tam, r, n = linea.split(None, 3)
        n = n.strip()
        if float(r) < 0.999 or n not in addr:
            continue
        e = E.ins(int(a, 16), int(tam))
        o = N.ins(addr[n], int(tam))
        difs = []
        for x, y in zip(e, o):
            if x.mnemonic != y.mnemonic:
                difs.append(('MN', x.mnemonic, y.mnemonic))
                continue
            if x.mnemonic.startswith('b'):
                continue
            mx = re.findall(r'(-?0x[0-9a-f]+|-?\d+)\((r\d+)\)', x.op_str)
            my = re.findall(r'(-?0x[0-9a-f]+|-?\d+)\((r\d+)\)', y.op_str)
            if mx and my and mx[0][0] != my[0][0] and x.mnemonic != 'lis':
                difs.append(('OFF', mx[0][0], my[0][0]))
            elif x.op_str != y.op_str and x.mnemonic in ('li', 'cmpwi', 'cmplwi', 'addi', 'rlwinm', 'clrlwi', 'slwi',
                                                          'srwi', 'ori'):
                difs.append(('IMM', x.op_str, y.op_str))
        clase = n.split('@')[1] if n.startswith('?') and '@' in n[1:] else n[:30]
        print('%s %-60s %s' % (a, n[:60], ' '.join('%s:%s>%s' % (t[0], t[-2], t[-1]) for t in difs[:8])))
        for t in difs:
            if t[0] == 'OFF':
                porclase[clase][(t[1], t[2])] += 1
    print()
    for clase, c in porclase.items():
        print(clase, c.most_common(8))


if __name__ == '__main__':
    main()
