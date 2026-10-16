#!/usr/bin/env python3
"""ldlr.py -- funciones de PS2 donde nuestro objeto copia con ldl/ldr/sdl/sdr y el original no (o al reves).

EE-GCC copia un struct que solo sabe alineado a 4 con pares ldl/ldr y sdl/sdr; si el original lo copia con ld/sd
(o con lq/sq) es que el tipo, o el miembro, estaba alineado a 8 o a 16 y a nosotros nos falta esa alineacion. Es la
huella de una alineacion por miembro que falta (memoria "La alineacion por miembro"). Cuenta esas cuatro
instrucciones en cada funcion de build/V/src (nuestro) y de build/V/obj (el original) y lista las que no coinciden,
las de mas diferencia primero.

    python scripts/ldlr.py SLES-53558-A124
    python scripts/ldlr.py SLUS-21351
"""
import os
import struct
import sys

from elftools.elf.elffile import ELFFile

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# codigos de operacion MIPS: ldl 0x1A, ldr 0x1B, sdl 0x2C, sdr 0x2D
OPS = {0x1A, 0x1B, 0x2C, 0x2D}


def funciones(p):
    """{simbolo: (numero de ldl/ldr/sdl/sdr, tamano)}"""
    e = ELFFile(open(p, 'rb'))
    st = e.get_section_by_name('.symtab')
    fuera = {}
    secciones = {}
    for s in st.iter_symbols():
        if s['st_info']['type'] != 'STT_FUNC' or not s['st_size']:
            continue
        sh = s['st_shndx']
        if not isinstance(sh, int):
            continue
        if sh not in secciones:
            secciones[sh] = e.get_section(sh).data()
        d = secciones[sh][s['st_value']:s['st_value'] + s['st_size']]
        n = 0
        for i in range(0, len(d) - 3, 4):
            if (struct.unpack('<I', d[i:i + 4])[0] >> 26) in OPS:
                n += 1
        fuera[s.name] = (n, s['st_size'])
    return fuera


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(2)
    v = sys.argv[1]
    base = os.path.join(RAIZ, 'build', v)
    res = []
    for dp, _, fn in os.walk(os.path.join(base, 'obj', 'Speed', 'Indep', 'SourceLists')):
        for f in fn:
            if not f.endswith('.o'):
                continue
            orig = os.path.join(dp, f)
            nuestro = orig.replace(os.path.join(base, 'obj'), os.path.join(base, 'src'))
            if not os.path.exists(nuestro):
                continue
            try:
                a = funciones(orig)
                b = funciones(nuestro)
            except Exception:
                continue
            for k, (n, tam) in a.items():
                if k in b and b[k][0] != n:
                    res.append((b[k][0] - n, f, k, tam, n, b[k][0]))
    for r in sorted(res, key=lambda x: -abs(x[0]))[:120]:
        print('%+4d %-22s orig=%3d nuestro=%3d %5d %s' % (r[0], r[1], r[4], r[5], r[3], r[2][:90]))


if __name__ == '__main__':
    main()
