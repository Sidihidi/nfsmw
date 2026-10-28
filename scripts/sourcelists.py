#!/usr/bin/env python3
"""sourcelists.py ELF SALIDA.txt -- reconstruye las SourceLists de Carbon PS3 con .debug_macinfo: los ficheros que
cada CU z*.cpp incluye DIRECTAMENTE (profundidad 1), en orden, con su linea del #include.
Pasada a scripts/ en la ronda 78 (procedencia: docs/analisis/2026-10-21_carbonps3.md); sin rutas de la maquina.
"""
import sys

from elftools.elf.elffile import ELFFile

from macinfo import lee

if __name__ == '__main__' and (len(sys.argv) < 3 or sys.argv[1] in ('-h', '--help')):
    print(__doc__)
    sys.exit(0)


def main():
    f = ELFFile(open(sys.argv[1], 'rb'))
    dw = f.get_dwarf_info()
    mac = f.get_section_by_name('.debug_macinfo').data()
    out = open(sys.argv[2], 'w', encoding='utf-8', newline='\n')
    for cu in dw.iter_CUs():
        a = cu.get_top_DIE().attributes
        nm = a['DW_AT_name'].value.decode('latin-1')
        if 'DW_AT_macro_info' not in a:
            continue
        lp = dw.line_program_for_CU(cu)
        dirs = [d.decode('latin-1') for d in lp['include_directory']]
        files = []
        for fe in lp['file_entry']:
            n = fe.name.decode('latin-1')
            if fe.dir_index and fe.dir_index <= len(dirs):
                n = dirs[fe.dir_index - 1].rstrip('/' + chr(92)) + '/' + n
            files.append(n)
        prof = 0
        out.write('== %s\n' % nm)
        for t, ln, txt, _ in lee(mac, a['DW_AT_macro_info'].value, files):
            if t == 'S':
                prof += 1
                if prof == 2:
                    out.write('  %5d %s\n' % (ln, txt))
            elif t == 'E':
                prof -= 1
    out.close()


if __name__ == '__main__':
    main()
