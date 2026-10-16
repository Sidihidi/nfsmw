#!/usr/bin/env python3
"""getkind.py -- los `_GetKind` de los mensajes de Hermes de SLUS (MEnterFreeRoam...) anclados por su CADENA:

    static UCrc32 _GetKind() { static UCrc32 k(stringhash32("MEnterFreeRoam")); return k; }

El cuerpo es igual en todas las clases salvo la cadena y los estaticos (reubicaciones), asi que los nombres heredados
por direccion se cruzaban y ninguna comparacion de codigo los distingue. Busca en el troceado de SLUS las funciones
cortas que llaman a stringhash32__FPCc con `lui/addiu $a0` de una cadena del ELF, y propone `_GetKind__<n><cadena>` si
nuestro objeto de la unidad lo define.

    python scripts/getkind.py [--tsv propuestas.tsv]

Imprime direccion, nombre propuesto, nombre actual y el resultado de scripts/prueba_nombre.py. --tsv vuelca
`0xDIR<TAB>nombre<TAB>cadena<TAB>resultado`, que acepta scripts/aplica_nombres.py; despues, trocear y medir SLUS con
scripts/medir.py. Lee el troceado de la raiz donde este el guion.
"""
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
from vtable_dtors import Imagen, ELFS  # noqa: E402
import prueba_lote  # noqa: E402
import prueba_nombre as pn  # noqa: E402

V = 'SLUS-21351'
RE_INS = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s+[0-9A-F]{8}\s*\*/\s+(\S+)\s*(.*)$')


def main():
    img = Imagen(os.path.join(RAIZ, ELFS[V]))
    pr = prueba_lote.Probador(V)
    filas = []
    for p, unidad in pn.ficheros_asm(V):
        cur = None
        fns = []
        for l in open(p, encoding='utf-8', errors='replace'):
            g = re.match(r'\s*glabel (\S+)', l)
            if g:
                cur = [None, g.group(1), []]
                fns.append(cur)
                continue
            m = RE_INS.match(l)
            if m and cur is not None:
                if cur[0] is None:
                    cur[0] = int(m.group(1), 16)
                cur[2].append((m.group(2), m.group(3).replace(' ', '')))
        t, o = pr.objs(unidad)
        for ini, g, ins in fns:
            if not any(op == 'jal' and a == 'stringhash32__FPCc' for op, a in ins) or len(ins) > 40:
                continue
            his = {}
            cad = None
            for op, a in ins:
                m = re.match(r'\$a0,%hi\((D_[0-9A-F]{8})\)$', a)
                if op == 'lui' and m:
                    his[m.group(1)] = True
                m = re.match(r'\$a0,\$a0,%lo\((D_[0-9A-F]{8})\)$', a)
                if op == 'addiu' and m and m.group(1) in his:
                    x = int(m.group(1)[2:], 16)
                    try:
                        cad = img.lee(x, 80).split(b'\0')[0].decode('latin-1')
                    except ValueError:
                        cad = None
            if not cad or not re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', cad):
                continue
            n = '_GetKind__%d%s' % (len(cad), cad)
            if n not in o or n == g:
                continue
            r = pr.prueba(ini, n)[0]
            filas.append((ini, n, g, r))
            print('0x%06x %-40s ahora %-40s %s' % (ini, n, g, r))
    print('%d propuestas' % len(filas))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for a, n, g, r in filas:
                fh.write('0x%x\t%s\tcadena\t%s\n' % (a, n, r))


if __name__ == '__main__':
    main()
