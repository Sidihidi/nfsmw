#!/usr/bin/env python3
"""build_recomp_iso.py -- construye build/GOWE69/NFSMW_RECOMP.iso inyectando
nuestro main.dol en una copia EXACTA de la ISO original.

OJO (r77): el hueco del DOL va de 0x20300 al OFFSET DEL FST (0x475100), NO es
una longitud. La receta anterior rellenaba con ceros hasta 0x20300+0x475100 y
BORRABA el FST (y 140KB de datos detras), con lo que el juego no podia abrir
ningun fichero del DVD: arrancaba leyendo basura y moria con
"unable to resolve read address". El FST y todo lo que sigue al hueco deben
quedar INTACTOS.

  python scripts/build_recomp_iso.py            # usa la ISO USA del repo
  python scripts/build_recomp_iso.py <orig.iso> # otra fuente
"""
import os
import shutil
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_SRC = os.path.join(ROOT, "Need For Speed - Most Wanted (USA)",
                           "Need for Speed - Most Wanted (USA).iso")
DST = os.path.join(ROOT, "build", "GOWE69", "NFSMW_RECOMP.iso")
DOL = os.path.join(ROOT, "build", "GOWE69", "main.dol")
DOL_OFF = 0x20300


def fst_offset(iso_path):
    with open(iso_path, "rb") as f:
        f.seek(0x424)
        return struct.unpack(">I", f.read(4))[0]


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SRC
    fst_off = fst_offset(src)
    dol = open(DOL, "rb").read()
    hole = fst_off - DOL_OFF
    if len(dol) > hole:
        sys.exit("ERROR: main.dol (%d B) no cabe en el hueco 0x%x..0x%x (%d B)"
                 % (len(dol), DOL_OFF, fst_off, hole))
    print("DOL %d B en hueco %d B (margen %d B); FST en 0x%x queda intacto"
          % (len(dol), hole, hole - len(dol), fst_off))
    shutil.copyfile(src, DST)
    with open(DST, "r+b") as f:
        f.seek(DOL_OFF)
        f.write(dol)
        f.write(b"\0" * (hole - len(dol)))
        f.seek(0x424)
        check = struct.unpack(">I", f.read(4))[0]
        assert check == fst_off
    print("OK: %s (%d B)" % (DST, os.path.getsize(DST)))


if __name__ == "__main__":
    main()
