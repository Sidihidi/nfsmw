#!/usr/bin/env python3
"""Arbol de bloques y locales de los stabs de una funcion de la A124: el del original o el de un objeto nuestro, y con
--diff los dos lado a lado.

Cada `$LBB`/`$LBE` abre y cierra un bloque (un ambito con declaraciones o una expansion en linea) y cada local sale
con `(reg)` si vive en registro o `(stk)` si vive en la pila. Sirve para ver que declara el original en cada ambito:
una local que falta o sobra, un bloque de mas (un if con declaraciones, una expansion que el original no hace) o una
variable que el original tiene en la pila y nosotros en un registro. Ojo: los stabs no listan las locales que el
optimizador deja sin registro, y un bloque muerto conserva sus locales aunque no genere codigo.

    python scripts/arbol.py SIMBOLO                    # el original (A124)
    python scripts/arbol.py SIMBOLO OBJETO.o           # nuestro objeto, compilado con stabs
    python scripts/arbol.py SIMBOLO OBJETO.o --diff    # diff unificado original -> nuestro

Usa relops.py para leer el .mdebug del ELF de la A124 y el de nuestro objeto.
"""
import difflib
import os
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import relops  # noqa: E402


def arbol(t, sim):
    i0 = t.idx[sim]
    out = []
    prof = 0
    for i in range(i0 + 1, t.n):
        iss, value, bits = struct.unpack_from('<III', t.raw, t.base + i * 12)
        st = bits & 0x3f
        if st == 6:
            break
        nom = t.cad(iss)
        if st == 0 and nom.startswith('$LBB'):
            out.append('  ' * prof + '{')
            prof += 1
        elif st == 0 and nom.startswith('$LBE'):
            prof -= 1
            out.append('  ' * prof + '}')
        elif st == 0 and ':' in nom:
            n, ty = nom.split(':', 1)
            kind = ty[0]
            where = 'reg' if kind in 'rP' else ('stk' if kind.isdigit() or kind == '-' else kind)
            out.append('  ' * prof + '%s(%s)' % (n, where))
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if not args:
        print(__doc__)
        return 1
    sim = args[0]
    orig = arbol(relops.original(), sim)
    if len(args) > 1:
        nuestro = arbol(relops.objeto(args[1]), sim)
        if '--diff' in sys.argv:
            for linea in difflib.unified_diff(orig, nuestro, 'original', 'nuestro', lineterm='', n=2):
                print(linea)
        else:
            print('\n'.join(nuestro))
    else:
        print('\n'.join(orig))
    return 0


if __name__ == '__main__':
    sys.exit(main())
