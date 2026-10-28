#!/usr/bin/env python3
"""tamanos360.py [--ltcg DIR] -- tamanos de `new`/bMalloc que difieren entre el original de Xbox 360 y nuestro
enlace LTCG (ronda 83, X360-B; a scripts/ en la 84).

Para cada funcion con el mismo nombre en config/EUROPEGERMILESTONE/symbols.txt y en el .map del enlace LTCG
(scripts/ltcg360.py), recoge los `li r3, N` (N >= 0x20) seguidos, en las 4 instrucciones siguientes, de un `bl` a una
funcion que se llama ??2*, *Alloc*, *Malloc* o *new*, e imprime las funciones donde la lista de N difiere: una clase
o una tabla de otro tamano en Xenon (JuiceStatsDB de 0x2C4, la allocation_table de 3072, MAX_CAR_PART_MODELS 500...).

    python scripts/tamanos360.py                     # desde la raiz, tras una medida de X360 (pctall/medir)

Lo que da es una PISTA: el tamano de X360 se confirma en el binario (quien lee los miembros nuevos) antes de tocar
una cabecera, y va con #ifdef EA_PLATFORM_XENON y la direccion del original en un comentario.
"""
import argparse
import os
import re
import struct

import capstone

ORIG = os.path.join('orig', 'EUROPEGERMILESTONE', 'NFS.exe')
ORIG_BASE = 0x82000000  # el PE del xex reubicado: VA 0x82000600 = RVA 0x600 (como scripts/ltcg360.py)
SIMBOLOS = os.path.join('config', 'EUROPEGERMILESTONE', 'symbols.txt')
LTCG = os.path.join('build', 'EUROPEGERMILESTONE', 'ltcg')
MD = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)


def pe(path, base=None):
    """lee(va, n) de un PE de PowerPC (base: la del original, o la preferida del propio PE)."""
    d = open(path, 'rb').read()
    p = struct.unpack('<I', d[0x3c:0x40])[0]
    ns = struct.unpack('<H', d[p + 6:p + 8])[0]
    osz = struct.unpack('<H', d[p + 20:p + 22])[0]
    b = struct.unpack('<I', d[p + 24 + 28:p + 24 + 32])[0] if base is None else base
    secs = []
    for i in range(ns):
        o = p + 24 + osz + 40 * i
        vs, va, rs, rp = struct.unpack('<IIII', d[o + 8:o + 24])
        secs.append((b + va, vs, rp))

    def lee(v, n):
        for a, vs, rp in secs:
            if a <= v < a + vs:
                return d[rp + v - a:rp + v - a + n]
        return b''
    return lee


def simbolos_originales():
    """{nombre: (va, tamano)} de las funciones de symbols.txt de X360."""
    out = {}
    for linea in open(SIMBOLOS, encoding='utf-8'):
        m = re.match(r'^(\S+) = \.text:0x([0-9A-F]+); // type:function size:0x([0-9A-F]+)', linea)
        if m:
            out[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    return out


def funciones_ltcg(ltcg):
    """{nombre: va} de las funciones del .map de nuestro enlace LTCG."""
    out = {}
    for linea in open(os.path.join(ltcg, 'NFS_ltcg.map'), encoding='latin-1'):
        c = linea.split()
        if len(c) >= 4 and c[3] == 'f' and re.match(r'^[0-9a-f]{8}$', c[2]):
            out[c[1]] = int(c[2], 16)
    return out


def tamanos(lee, va, n, nombres, rx=re.compile(r'(\?\?2|Alloc|Malloc|alloc|new)')):
    ins = list(MD.disasm(lee(va, n), va))
    out = []
    for i, x in enumerate(ins):
        if x.mnemonic == 'li' and x.op_str.startswith('r3, '):
            try:
                v = int(x.op_str.split(', ')[1], 0)
            except ValueError:
                continue
            if v < 0x20:
                continue
            for y in ins[i + 1:i + 5]:
                if y.mnemonic == 'bl':
                    if rx.search(nombres.get(int(y.op_str, 0), '')):
                        out.append(v)
                    break
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--ltcg', default=LTCG, help='directorio del enlace LTCG (NFS_ltcg.exe/.map)')
    a = ap.parse_args()
    egm = simbolos_originales()
    egm_por_va = {v[0]: k for k, v in egm.items()}
    nu = funciones_ltcg(a.ltcg)
    nu_por_va = {v: k for k, v in nu.items()}
    le, ln = pe(ORIG, ORIG_BASE), pe(os.path.join(a.ltcg, 'NFS_ltcg.exe'))
    for nm, (va, n) in sorted(egm.items(), key=lambda kv: kv[1][0]):
        if nm not in nu or nm.startswith('fn_'):
            continue
        x, y = tamanos(le, va, n, egm_por_va), tamanos(ln, nu[nm], n, nu_por_va)
        if x and x != y:
            print('%08X %-70s original %s nuestro %s' % (va, nm[:70], [hex(v) for v in x], [hex(v) for v in y]))


if __name__ == '__main__':
    main()
