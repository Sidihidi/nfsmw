#!/usr/bin/env python3
"""escapa360.py VA [--egm] [--enlace DIR] -- en nuestro enlace LTCG (o en el EGM con --egm), las funciones que forman
la direccion VA con lis+addi (la direccion "escapa": se pasa o se guarda) y las que escriben en VA+off (0..0x200) con
lis + stb/sth/stw/stfs/stfd, por funcion.

LTCG solo especializa un parametro a una global (this = &TheRaceParameters, this = VoiceCore::mInstance) si sabe
todas las escrituras; comparar los dos lados dice si la diferencia esta en quien escribe o en quien forma la direccion.

    python scripts/escapa360.py 0x82CB2140 --egm
"""
import collections
import struct
import sys

import egm360


def main():
    argv = sys.argv[1:]
    d = egm360.enlace(argv)
    es_egm = '--egm' in argv
    argv = [a for a in argv if a != '--egm']
    V = int(argv[0], 16)
    if es_egm:
        img = egm360.egm()
        nombres = egm360.Nombres(egm360.simbolos_egm())
    else:
        img = egm360.nuestro(d)
        nombres = egm360.Nombres([(a, n) for a, n, o, t in egm360.mapa(d)])
    hi = (V + 0x8000) >> 16
    forma, escribe = collections.Counter(), collections.Counter()
    va, W = img.texto()
    lis = {}
    for k, w in enumerate(W):
        op = w >> 26
        rt, ra, imm = (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
        simm = imm - 0x10000 if imm & 0x8000 else imm
        a = va + 4 * k
        if op == 15 and ra == 0:
            lis[rt] = (imm, a)
            continue
        if ra in lis and a - lis[ra][1] < 64 and lis[ra][0] == hi:
            ea = ((hi << 16) + simm) & 0xFFFFFFFF
            f = nombres.de(a)
            if op == 14 and ea == V:
                forma[f] += 1
            elif op in (38, 44, 36, 52, 54) and V <= ea < V + 0x200:  # stb sth stw stfs stfd
                escribe[(f, ea - V)] += 1
        if rt in lis and op not in (36, 38, 44, 52, 54, 32, 34, 40, 48):
            if op != 14 or rt != ra:
                lis.pop(rt, None)
    print('FORMAN la direccion (lis+addi):')
    for f, n in sorted(forma.items(), key=lambda x: -x[1]):
        print('  %3d %s' % (n, f[:110]))
    print('ESCRIBEN en +off (lis + st):')
    for (f, off), n in sorted(escribe.items()):
        print('  +0x%x %3d %s' % (off, n, f[:100]))


if __name__ == '__main__':
    main()
