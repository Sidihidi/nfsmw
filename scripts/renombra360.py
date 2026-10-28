#!/usr/bin/env python3
"""renombra360.py LISTA.tsv [--aplicar] [--corrige] -- pone nombres a mano en config/EUROPEGERMILESTONE/symbols.txt.

Sin --corrige, cada fila es `0xDIRECCION<TAB>nombre` y solo toca lineas `fn_XXXXXXXX = .text:0x...`; si el nombre ya
estaba en otra direccion, esa vuelve a fn_ (el nombre es unico). Con --corrige, cada fila es
`0xDIRECCION<TAB>nombre_viejo<TAB>nombre_nuevo` para un nombre FALSO: solo cambia si la linea tiene exactamente el
viejo y el nuevo no esta en otra linea. Sin --aplicar solo dice lo que haria. Escribe con scripts/escritura.py
(temporal + os.replace) y conserva los finales de linea.

    python scripts/renombra360.py n_iguales.tsv --aplicar
    python scripts/renombra360.py falsos.tsv --corrige --aplicar
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from escritura import escribe  # noqa: E402

F = os.path.join('config', 'EUROPEGERMILESTONE', 'symbols.txt')


def main():
    corrige = '--corrige' in sys.argv
    filas = [l.rstrip('\r\n').split('\t') for l in open(sys.argv[1], encoding='utf-8')
             if l.strip() and not l.startswith('#')]
    lineas = open(F, encoding='utf-8', newline='').read().split('\n')
    por_dir, por_nom = {}, {}
    for i, l in enumerate(lineas):
        m = re.match(r'^(\S+) = \.text:0x([0-9A-F]+);', l)
        if m:
            por_dir[int(m.group(2), 16)] = i
            por_nom.setdefault(m.group(1), []).append(i)
    cambios = 0
    for fila in filas:
        a = int(fila[0], 16)
        i = por_dir.get(a)
        if i is None:
            print('SIN LINEA', hex(a))
            continue
        actual = lineas[i].split(' = ', 1)[0]
        if corrige:
            viejo, nuevo = fila[1], fila[2]
            if actual != viejo:
                print('NO coincide (%s): no toco' % actual, hex(a))
                continue
            if nuevo in por_nom:
                print('el nuevo ya esta en otra linea: no toco', hex(a), nuevo)
                continue
        else:
            nuevo = fila[1]
            if actual == nuevo:
                print('ya', hex(a), nuevo)
                continue
            if not actual.startswith('fn_'):
                print('NO es fn_ (%s): no toco' % actual, hex(a), nuevo)
                continue
            for j in por_nom.get(nuevo, []):
                print('  el nombre estaba en', lineas[j].split(';')[0], '-> fn_')
                aa = re.match(r'^\S+ = \.text:0x([0-9A-F]+);', lineas[j]).group(1)
                lineas[j] = 'fn_%s' % aa + lineas[j][lineas[j].index(' = '):]
        lineas[i] = nuevo + lineas[i][lineas[i].index(' = '):]
        por_nom.setdefault(nuevo, []).append(i)
        print('pone', hex(a), nuevo)
        cambios += 1
    if '--aplicar' in sys.argv and cambios:
        escribe(F, '\n'.join(lineas), encoding='utf-8', newline='')
        print('escrito:', cambios)


if __name__ == '__main__':
    main()
