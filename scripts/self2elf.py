#!/usr/bin/env python3
"""self2elf.py EBOOT.BIN EBOOT.elf -- saca el ELF de un SELF de PS3 SIN CIFRAR (fself de depuracion): lee la cabecera
SCE, comprueba en la tabla de segmentos (section info) que nada va cifrado ni comprimido y copia el ELF entero, que
en un fself va tal cual a partir de header_len. Si algo va cifrado, para.
Pasada a scripts/ en la ronda 78 (procedencia: docs/analisis/2026-10-21_carbonps3.md); sin rutas de la maquina.
"""
import struct
import sys

if __name__ == '__main__' and (len(sys.argv) < 3 or sys.argv[1] in ('-h', '--help')):
    print(__doc__)
    sys.exit(0)


def main():
    d = open(sys.argv[1], 'rb').read()
    magic, ver, krev, htype, meta_off, hdr_len, data_len = struct.unpack('>4sIHHIQQ', d[:0x20])
    if magic != b'SCE\0':
        raise SystemExit('no es un SELF')
    ext = struct.unpack('>QQQQQQQQ', d[0x20:0x60])
    elf_off, phdr_off, secinfo_off = ext[2], ext[3], ext[5]
    phnum, = struct.unpack('>H', d[elf_off + 0x38:elf_off + 0x3A])
    for i in range(phnum):
        off, size, comp, _, _, enc = struct.unpack('>QQIIII', d[secinfo_off + 32 * i:secinfo_off + 32 * i + 32])
        if enc == 1 or comp == 2:
            raise SystemExit('segmento %d cifrado (%d) o comprimido (%d): no se puede sacar en claro' % (i, enc, comp))
    if d[hdr_len:hdr_len + 4] != b'\x7fELF':
        raise SystemExit('no hay ELF en claro en header_len=0x%X' % hdr_len)
    open(sys.argv[2], 'wb').write(d[hdr_len:hdr_len + data_len])
    print('key revision 0x%X, header_len 0x%X, ELF de %d B' % (krev, hdr_len, data_len))


if __name__ == '__main__':
    main()
