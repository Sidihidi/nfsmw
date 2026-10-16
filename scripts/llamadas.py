#!/usr/bin/env python3
"""llamadas.py -- que funciones llaman (o citan) a un simbolo en unos objetos ELF, y cuantas veces: cuenta las
reubicaciones contra ese simbolo por funcion (R_MIPS_26 de un jal en PS2, R_PPC_REL24 de un bl en GameCube, y tambien
las de una direccion que se carga).

    python scripts/llamadas.py OBJ[,OBJ...] SIMBOLO
        objetos por ruta (admiten comodines: build/SLES-53558-A124/src/**/*.o)
    python scripts/llamadas.py UNIDAD[,UNIDAD...] SIMBOLO --orig [--v=V[,V...]]
        los objetos ORIGINALES de esas SourceLists (build/V/obj/Speed/Indep/SourceLists/UNIDAD.o; `todas` para
        todas), por defecto de las dos PS2

Sirve para ver si un inline sale fuera de linea (una llamada que el original no tiene) o quien usa una funcion.
Solo cuenta reubicaciones contra el simbolo por nombre: una llamada local ya resuelta por el ensamblador (sin
reubicacion) o una contra el simbolo de la seccion no aparece.
"""
import collections
import glob
import os
import sys

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def llamadas(ruta, simbolo):
    """Counter {funcion que llama: veces} de un objeto"""
    fuera = collections.Counter()
    with open(ruta, 'rb') as fh:
        elf = ELFFile(fh)
        symtab = elf.get_section_by_name('.symtab')
        syms = list(symtab.iter_symbols())
        funcs = collections.defaultdict(list)  # seccion -> [(inicio, fin, nombre)]
        for s in syms:
            if s['st_info']['type'] == 'STT_FUNC' and s['st_size'] > 0:
                funcs[s['st_shndx']].append((s['st_value'], s['st_value'] + s['st_size'], s.name))
        for sec in elf.iter_sections():
            if not isinstance(sec, RelocationSection):
                continue
            destino = sec['sh_info']
            for r in sec.iter_relocations():
                if syms[r['r_info_sym']].name != simbolo:
                    continue
                off = r['r_offset']
                nombre = '?'
                for a, b, n in funcs.get(destino, []):
                    if a <= off < b:
                        nombre = n
                        break
                fuera[nombre] += 1
    return fuera


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if len(args) != 2:
        print(__doc__)
        return 2
    sim = args[1]
    versiones = ['SLES-53558-A124', 'SLUS-21351']
    for a in sys.argv[1:]:
        if a.startswith('--v='):
            versiones = a[4:].split(',')
    rutas = []
    if '--orig' in sys.argv:
        for v in versiones:
            sld = os.path.join(RAIZ, 'build', v, 'obj', 'Speed', 'Indep', 'SourceLists')
            for u in args[0].split(','):
                if u == 'todas':
                    rutas += sorted(glob.glob(os.path.join(sld, '*.o')))
                else:
                    rutas.append(os.path.join(sld, u + '.o'))
    else:
        for x in args[0].split(','):
            rutas += sorted(glob.glob(x, recursive=True))
    for r in rutas:
        if not os.path.exists(r):
            continue
        c = llamadas(r, sim)
        if c:
            nombre = os.path.relpath(r, RAIZ) if os.path.abspath(r).startswith(RAIZ) else r
            print('== %s: %d llamadas en %d funciones' % (nombre, sum(c.values()), len(c)))
            for n, k in sorted(c.items()):
                print('   %3d  %s' % (k, n))
    return 0


if __name__ == '__main__':
    sys.exit(main())
