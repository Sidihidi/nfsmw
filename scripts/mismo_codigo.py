#!/usr/bin/env python3
"""mismo_codigo.py -- compara funcion a funcion los BYTES de codigo de dos objetos ELF (por simbolo STT_FUNC con
tamano): cuantas son iguales, cuales distintas y cuales estan solo en uno.

    python scripts/mismo_codigo.py A.o B.o [--todas]

Sirve para saber si un cambio de fuente es neutro en un objeto que objdiff no puede medir, o para comparar dos
compilaciones nuestras entre si (antes/despues). Compara bytes crudos: en MIPS las reubicaciones REL llevan el
sumando dentro de la instruccion, asi que un desplazamiento distinto en .rodata cuenta como distinto (a proposito).
Sin --todas ensena las 30 primeras distintas y las 10 primeras de cada lado que faltan en el otro.
"""
import sys

from elftools.elf.elffile import ELFFile


def funciones(ruta):
    """{nombre: bytes} de las funciones con tamano de un objeto"""
    fuera = {}
    with open(ruta, 'rb') as fh:
        elf = ELFFile(fh)
        secs = {i: s for i, s in enumerate(elf.iter_sections())}
        datos = {}
        for s in elf.get_section_by_name('.symtab').iter_symbols():
            if s['st_info']['type'] != 'STT_FUNC' or s['st_size'] == 0:
                continue
            idx = s['st_shndx']
            if not isinstance(idx, int):
                continue
            if idx not in datos:
                datos[idx] = secs[idx].data()
            fuera[s.name] = datos[idx][s['st_value']:s['st_value'] + s['st_size']]
    return fuera


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if len(args) != 2:
        print(__doc__)
        return 2
    todas = '--todas' in sys.argv
    a, b = funciones(args[0]), funciones(args[1])
    iguales = sum(1 for k in a if k in b and a[k] == b[k])
    distintas = sorted(k for k in a if k in b and a[k] != b[k])
    solo_a = sorted(set(a) - set(b))
    solo_b = sorted(set(b) - set(a))
    print('iguales %d  distintas %d  solo en A %d  solo en B %d' % (iguales, len(distintas), len(solo_a),
                                                                   len(solo_b)))
    for k in (distintas if todas else distintas[:30]):
        print('   distinta', k)
    for k in (solo_a if todas else solo_a[:10]):
        print('   solo A', k)
    for k in (solo_b if todas else solo_b[:10]):
        print('   solo B', k)
    return 0


if __name__ == '__main__':
    sys.exit(main())
