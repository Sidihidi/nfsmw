#!/usr/bin/env python3
"""pecmp.py A.exe B.exe -- compara dos PE seccion a seccion (cabeceras de seccion, y los datos de cada una leidos por
direccion virtual en una imagen en memoria o por desplazamiento en un fichero en disco, lo que corresponda a cada uno).
Sirve para validar xexdec.py contra un .exe ya extraido del mismo enlace.

Procedencia: carril PROTOS-MW de la ronda 78 (build/scratch_protosmw/), pasado a scripts/ en la ronda 79.

    python scripts/pecmp.py imagen_de_xexdec.exe orig/EUROPEGERMILESTONE/NFS.exe
"""
import struct
import sys


def secciones(d):
    pe = struct.unpack_from('<I', d, 0x3C)[0]
    n = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    r = []
    for i in range(n):
        s = pe + 24 + opt + 40 * i
        nombre = d[s:s + 8].rstrip(b'\0').decode('latin-1')
        vs, va, rs, rp = struct.unpack_from('<IIII', d, s + 8)
        r.append((nombre, vs, va, rs, rp))
    return r


def datos(d, sec, como_imagen):
    nombre, vs, va, rs, rp = sec
    return d[va:va + rs] if como_imagen else d[rp:rp + rs]


def main():
    if len(sys.argv) != 3 or any(a in ('-h', '--help') for a in sys.argv[1:]):
        sys.exit(__doc__)
    a = open(sys.argv[1], 'rb').read()
    b = open(sys.argv[2], 'rb').read()
    sa, sb = secciones(a), secciones(b)
    print('cabeceras de seccion iguales:', sa == sb)
    total = iguales = 0
    for sec in sb:
        y = datos(b, sec, False)
        mejor = None
        for como in (True, False):
            x = datos(a, sec, como)
            if x == y:
                mejor = 'imagen' if como else 'fichero'
                break
        total += 1
        iguales += mejor is not None
        print('%-8s va=%#9x raw=%#9x tam=%#9x  %s' % (sec[0], sec[2], sec[4], sec[3], ('IGUAL (' + mejor + ')') if mejor else 'DISTINTA'))
    print('%d de %d secciones iguales' % (iguales, total))


if __name__ == '__main__':
    main()
