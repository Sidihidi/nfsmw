#!/usr/bin/env python3
"""restos_sin_nombre.py -- funciones SIN NOMBRE de SLUS (func_) que empiezan por restos de epilogo: `addiu $sp, $sp, +N`
y `nop`, 8 bytes cada uno. La entrada de verdad esta detras de los restos, y por eso ni scripts/restos_epilogo.py
(solo mueve nombres que ya existen) ni tools/nombra_ps2.py (compara la funcion entera, con los restos dentro) las
nombran.

Para cada una se salta los restos (hasta cuatro) y se compara lo que queda, palabra a palabra como
scripts/prueba_nombre.py, con TODAS nuestras funciones de la unidad del mismo tamano. Se propone el nombre en la
entrada de verdad (direccion + restos) si una sola nuestra es IGUAL y ese nombre no esta ya en symbols.txt: codigo
unico, la misma prueba que tools/nombra_ps2.py. Las que tienen varias iguales (accesores, destructores de hermanas)
se cuentan y se dejan: necesitan otra ancla (scripts/vtslots.py, scripts/grafo_nombres.py).

    python scripts/restos_sin_nombre.py [--tsv propuestas.tsv] [--todas]

Imprime las propuestas (entrada, nombre, glabel actual y bytes de restos) y cuantas se quedan fuera por ambiguas.
--todas lista ademas, una por linea, TODAS las func_ con restos delante y por que no se nombran (ambigua, sin igual:
nuestro codigo no esta escrito o no casa). --tsv vuelca `0xDIR<TAB>nombre<TAB>restos<TAB>unidad`, que acepta
scripts/aplica_nombres.py; al nombrar la entrada, splat corta los restos en una func_ aparte de 8 o 16 bytes. Despues,
trocear y medir SLUS (scripts/medir.py) y pasar scripts/cortes.py.

Lee el troceado de SLUS de la raiz donde este el guion. Tanda 5 (2026-09-23): 377 func_ con restos delante; las de
codigo unico se nombraron entonces.
"""
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
import prueba_lote  # noqa: E402
import prueba_nombre as pn  # noqa: E402

V = 'SLUS-21351'
SIN = re.compile(r'^func_([0-9A-F]{8})$')


def es_resto(w0, w1):
    # addiu $sp, $sp, +N (N > 0) seguido de nop
    return (w0 >> 16) == 0x27BD and 0 < (w0 & 0xFFFF) < 0x8000 and w1 == 0


def main():
    pr = prueba_lote.Probador(V)
    nombres = set()
    for l in open(os.path.join(RAIZ, 'config', V, 'symbols.txt'), encoding='utf-8'):
        m = re.match(r'(\S+) = 0x', l)
        if m:
            nombres.add(m.group(1))
    filas = []
    ambiguas = 0
    todas = []                                  # (entrada, glabel, unidad, bytes de restos, tamano, estado)
    for x, (g, unidad) in sorted(pr.glabel.items()):
        if not SIN.match(g):
            continue
        t, o = pr.objs(unidad)
        if g not in t:
            continue
        tb, trel = t[g][0]
        n = len(tb) // 4
        ws = struct.unpack('<%dI' % n, tb[:n * 4])
        d = 0
        while d + 2 <= n and d < 8 and es_resto(ws[d], ws[d + 1]):
            d += 2
        if d == 0 or d >= n:
            continue
        cuerpo = (tb[4 * d:], [(r - 4 * d, s) for r, s in trel if r >= 4 * d])
        iguales = []
        for nombre, lst in o.items():
            for ob in lst:
                if len(ob[0]) != len(cuerpo[0]):
                    continue
                if pn.compara(cuerpo, ob)[0] == 'IGUAL':
                    iguales.append(nombre)
                    break
        if len(iguales) != 1:
            if len(iguales) > 1:
                ambiguas += 1
                estado = 'ambigua: ' + ', '.join(sorted(iguales)[:4]) + (' ...' if len(iguales) > 4 else '')
            else:
                estado = 'sin igual'
            todas.append((x + 4 * d, g, unidad, 4 * d, len(cuerpo[0]), estado))
            continue
        nombre = iguales[0]
        if nombre in nombres:
            todas.append((x + 4 * d, g, unidad, 4 * d, len(cuerpo[0]), 'igual a %s, que ya esta en symbols.txt' % nombre))
            continue
        todas.append((x + 4 * d, g, unidad, 4 * d, len(cuerpo[0]), 'propuesta: ' + nombre))
        filas.append((x + 4 * d, nombre, g, unidad))
        print('0x%06x %-70s (%s + %d B de restos) %s' % (x + 4 * d, nombre[:70], g, 4 * d, unidad.split('/')[-1]))
    if '--todas' in sys.argv:
        print('--- todas las func_ con restos delante: entrada, glabel, unidad, restos, tamano sin restos, estado')
        for e, g, unidad, d, tam, estado in todas:
            print('0x%06x %s %-18s %2d B %6d B  %s' % (e, g, unidad.split('/')[-1][:18], d, tam, estado))
        print('%d func_ con restos delante' % len(todas))
    uso = collections.Counter(f[1] for f in filas)
    filas = [f for f in filas if uso[f[1]] == 1 or f[1].startswith(('__tcf_', '_GLOBAL_', '__static_'))]
    print('%d propuestas (%d con varias nuestras iguales, fuera)' % (len(filas), ambiguas))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for a, nombre, g, unidad in filas:
                fh.write('0x%x\t%s\trestos\t%s\n' % (a, nombre, unidad))


if __name__ == '__main__':
    main()
