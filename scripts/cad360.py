#!/usr/bin/env python3
"""cad360.py -- cadenas y referencias del EGM (Xbox 360), por funcion de symbols.txt.

    python scripts/cad360.py cad TEXTO...      # VA de cada aparicion de la cadena y las funciones que la forman
    python scripts/cad360.py rango LO HI       # las cadenas de .rdata que forma (lis+addi) cada funcion de [LO, HI)
    python scripts/cad360.py refs VA...        # las funciones que forman la VA con lis + addi/lwz/stw/lbz... (12 instr.)

TEXTO admite escapes de Python (\\n, \\x00). Sirve para atribuir funciones sin nombre por la cadena que usan (el
`new ("Nombre")` de una fabrica, el printf de depuracion) y para encontrar quien lee o escribe una global.
"""
import struct
import sys

import egm360


def palabras(img):
    va, W = img.texto()
    return va, W


def refs(img, nombres, objetivos):
    tv, W = palabras(img)
    his = {}
    for t in objetivos:
        lo = t & 0xFFFF
        hi = (t >> 16) + (1 if lo & 0x8000 else 0)
        his.setdefault(hi & 0xFFFF, []).append((t, lo))
    res = {}
    for k, w in enumerate(W):
        if w >> 26 == 15 and (w >> 16) & 31 == 0:  # lis rD
            rd, imm = (w >> 21) & 31, w & 0xFFFF
            if imm in his:
                for j in range(k + 1, min(k + 13, len(W))):
                    x = W[j]
                    if x >> 26 in (14, 32, 36, 34, 38, 48, 52, 40, 44) and (x >> 16) & 31 == rd:
                        for t, lo in his[imm]:
                            if x & 0xFFFF == lo:
                                res.setdefault(t, set()).add(nombres.de(tv + 4 * j))
    return res


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    orden, args = sys.argv[1], sys.argv[2:]
    img = egm360.egm()
    nombres = egm360.Nombres(egm360.simbolos_egm())
    if orden == 'refs':
        objs = [int(a, 16) for a in args]
        r = refs(img, nombres, objs)
        for t in objs:
            print('%x: %s' % (t, ' | '.join(sorted(r.get(t, [])))))
    elif orden == 'cad':
        for t in args:
            s = t.encode().decode('unicode_escape').encode('latin-1')
            vas = []
            i = img.d.find(s)
            while i >= 0:
                for _, a, tam, rp, rs in img.secs:
                    if rp <= i < rp + rs:
                        vas.append(a + i - rp)
                i = img.d.find(s, i + 1)
            r = refs(img, nombres, vas)
            for v in vas:
                print('%r %08x: %s' % (s, v, ' | '.join(sorted(r.get(v, [])))))
    elif orden == 'rango':
        lo, hi = int(args[0], 16), int(args[1], 16)
        rdata = [(a, tam) for n, a, tam, rp, rs in img.secs if n == b'.rdata']
        cur, lis = None, {}
        datos = img.lee(lo, hi - lo)
        for k in range(0, len(datos) - 3, 4):
            a = lo + k
            w = struct.unpack_from('>I', datos, k)[0]
            f = nombres.de(a)
            if f != cur:
                cur, lis = f, {}
            if w >> 26 == 15 and (w >> 16) & 31 == 0:
                lis[(w >> 21) & 31] = (w & 0xFFFF) << 16
            elif w >> 26 == 14 and (w >> 16) & 31 in lis:
                v = (lis[(w >> 16) & 31] + ((w & 0xFFFF) ^ 0x8000) - 0x8000) & 0xFFFFFFFF
                if any(r <= v < r + t for r, t in rdata):
                    s = img.cadena(v, 80)
                    if s and len(s) >= 3 and all(32 <= c < 127 or c in (9, 10) for c in s):
                        print('%08x %-40s %r' % (a, f[:40], s.decode()))
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
