#!/usr/bin/env python3
"""Nombres que caen DENTRO de una funcion en el troceado de una version de PS2: un simbolo con nombre en la direccion A
no es una entrada si la funcion de splat anterior (la que acaba justo en A) tiene saltos locales (.Lxxxxxxxx) a
direcciones >= A, o si la funcion que empieza en A salta hacia atras dentro de la anterior. Entonces el nombre parte
una funcion por una etiqueta de rama (paso una vez en SLUS con un nombre heredado por direccion que caia sobre el
destino de un beq), y ni la funcion partida ni la de despues pueden casar.

Hay que pasarlo despues de cada lote de nombres, sobre el troceado NUEVO (medir.py o el troceado con el candado).

    python scripts/cortes.py SLUS-21351
    python scripts/cortes.py SLES-53558-A124 --solo-nuevos config_viejo/symbols.txt   # solo los nombres nuevos

Sin --solo-nuevos informa de todos. Solo informa: los casos se arreglan a mano en config/<V>/symbols.txt.
"""
import os
import re
import sys

from nombra_symtab import ficheros_asm

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RE_INS = re.compile(r'^\s*/\*\s*([0-9A-F]+)\s+([0-9A-F]{8})\s+([0-9A-F]{8})\s*\*/\s+(\S+)\s*(.*)$')
RE_L = re.compile(r'\.L([0-9A-F]{8})')


def funciones(version):
    """[inicio, fin, nombre, [(origen, destino) de saltos locales]] de los .s del troceado actual de la version"""
    fns = []
    for p, unidad in ficheros_asm(version):
        cur = None
        for l in open(p, encoding='utf-8', errors='replace'):
            m = re.match(r'\s*glabel (\S+)', l)
            if m:
                cur = [None, None, m.group(1), []]
                fns.append(cur)
                continue
            m = RE_INS.match(l)
            if m and cur is not None:
                v = int(m.group(2), 16)
                if cur[0] is None:
                    cur[0] = v
                cur[1] = v + 4
                cur[3] += [(v, int(t, 16)) for t in RE_L.findall(m.group(5))]
            elif re.match(r'\s*endlabel', l):
                cur = None
    return sorted(f for f in fns if f[0] is not None)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if not args:
        sys.exit(__doc__)
    version = args[0]
    viejos = None
    if '--solo-nuevos' in sys.argv:
        viejos = set()
        for l in open(sys.argv[sys.argv.index('--solo-nuevos') + 1], encoding='utf-8', errors='replace'):
            m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);', l)
            if m:
                viejos.add((m.group(1), int(m.group(2), 16)))
    fns = funciones(version)
    malos = 0
    for k in range(1, len(fns)):
        a0, a1, n, saltos = fns[k]
        p0, p1, pn, psaltos = fns[k - 1]
        if p1 != a0 or n.startswith('func_'):
            continue
        if viejos is not None and (n, a0) in viejos:
            continue
        adelante = [t for o, t in psaltos if t >= a0]
        atras = [t for o, t in saltos if p0 <= t < a0]
        if adelante or atras:
            malos += 1
            print('0x%06x %-60s corta a %-40s (saltos a %s)' % (a0, n[:60], pn[:40],
                                                               ', '.join('0x%x' % t for t in (adelante + atras)[:3])))
    print('%s: nombres que parten una funcion: %d' % (version, malos))


if __name__ == '__main__':
    main()
