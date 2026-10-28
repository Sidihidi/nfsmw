#!/usr/bin/env python3
"""macinfo.py ELF [CU_SUBCADENA ...] -- .debug_macinfo (DWARF 2) por unidad de compilacion.

Para cada CU imprime los #define de la LINEA DE ORDENES (los que van antes del primer DW_MACINFO_start_file, linea 0)
y cuenta los del resto. Con --todo y una subcadena de CU imprime todos los #define con fichero y linea.
Pasada a scripts/ en la ronda 78 (procedencia: docs/analisis/2026-10-21_carbonps3.md); sin rutas de la maquina.
"""
import sys

from elftools.elf.elffile import ELFFile

if __name__ == '__main__' and (len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help')):
    print(__doc__)
    sys.exit(0)


def uleb(b, i):
    r = sh = 0
    while True:
        c = b[i]
        i += 1
        r |= (c & 0x7F) << sh
        sh += 7
        if not c & 0x80:
            return r, i


def cstr(b, i):
    j = b.index(b'\0', i)
    return b[i:j].decode('latin-1'), j + 1


def lee(mac, off, files):
    """[(tipo, linea, texto|fichero, pila_de_ficheros)] desde off hasta el 0 final."""
    out = []
    pila = []
    i = off
    while True:
        t = mac[i]
        i += 1
        if t == 0:
            break
        if t in (1, 2):
            ln, i = uleb(mac, i)
            txt, i = cstr(mac, i)
            out.append(('D' if t == 1 else 'U', ln, txt, pila[-1] if pila else None))
        elif t == 3:
            ln, i = uleb(mac, i)
            fi, i = uleb(mac, i)
            nm = files[fi - 1] if 0 < fi <= len(files) else '#%d' % fi
            pila.append(nm)
            out.append(('S', ln, nm, None))
        elif t == 4:
            if pila:
                pila.pop()
            out.append(('E', 0, None, None))
        elif t == 255:
            _, i = uleb(mac, i)
            _, i = cstr(mac, i)
        else:
            raise ValueError('macinfo tipo %d en %x' % (t, i))
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    todo = '--todo' in sys.argv
    f = ELFFile(open(args[0], 'rb'))
    dw = f.get_dwarf_info()
    mac = f.get_section_by_name('.debug_macinfo').data()
    filtros = args[1:]
    for cu in dw.iter_CUs():
        top = cu.get_top_DIE()
        a = top.attributes
        nm = a['DW_AT_name'].value.decode('latin-1')
        if filtros and not any(x.lower() in nm.lower() for x in filtros):
            continue
        if 'DW_AT_macro_info' not in a:
            print('%s: sin macinfo' % nm)
            continue
        lp = dw.line_program_for_CU(cu)
        dirs = [d.decode('latin-1') for d in lp['include_directory']]
        files = []
        for fe in lp['file_entry']:
            n = fe.name.decode('latin-1')
            if fe.dir_index and fe.dir_index <= len(dirs):
                n = dirs[fe.dir_index - 1].rstrip('/\\') + '/' + n
            files.append(n)
        ev = lee(mac, a['DW_AT_macro_info'].value, files)
        cmd = []
        ns = 0
        for e in ev:
            if e[0] == 'S':
                ns += 1
                if ns > 1:
                    break
                continue
            if e[0] in 'DU' and e[1] <= 1:
                cmd.append(('-D' if e[0] == 'D' else '-U') + e[2])
        nd = sum(1 for e in ev if e[0] == 'D')
        nf = sum(1 for e in ev if e[0] == 'S')
        print('== %s: %d define, %d ficheros incluidos' % (nm, nd, nf))
        incorporados = [c for c in cmd if c.startswith('-D__') or c.startswith('-D_') and c[3:4].isupper() and 'EA' not in c]
        print('   linea de ordenes (%d, sin %d predefinidos de GCC con __):' % (len(cmd), len(incorporados)))
        for c in cmd:
            if c not in incorporados:
                print('     ' + c)
        if todo:
            for e in ev:
                if e[0] in 'DU':
                    print('   %s %s:%d %s' % (e[0], e[3], e[1], e[2]))


if __name__ == '__main__':
    main()
