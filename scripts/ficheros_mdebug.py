#!/usr/bin/env python3
"""ficheros_mdebug.py [unidad ...] -- para cada unidad (SourceList) de la A124: los ficheros .cpp/.c que el .mdebug
pone en su rango de codigo, en orden de enlace, y los que nuestra SourceList NO incluye (si existen en el arbol, con
cuantas lineas: 0 = esqueleto vacio).

Como: el fichero de cada funcion es la etiqueta de fichero (.cpp/.c) mas reciente antes de su stProc en la tabla de
simbolos del .mdebug; los rangos salen de config/SLES-53558-A124/config.yml (vram = start - 0x1000 + 0x100000 en el
segmento principal).

Encontro que la zOnline de la A124 (116 kB) tenia el subsistema online en el arbol y la SourceList vacia: +501
funciones. Las pistas buenas son ficheros que existen con contenido; los de 0 lineas son trabajo nuevo.
"""
import collections
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__))).replace('\\', '/') + '/'
sys.path.insert(0, ROOT + 'scripts')
import mdebug  # noqa: E402


def rangos_unidades():
    marcas = []
    bloque = None  # segmento en forma de bloque: `- start: 0x...` seguido de `type:` y `name:`
    for l in open(ROOT + 'config/SLES-53558-A124/config.yml'):
        m = re.search(r'\[0x([0-9A-Fa-f]+), (\w+|\.\w+)(?:, ([^\]]+))?\]', l)
        if m:
            marcas.append((int(m.group(1), 16), (m.group(3) or '').strip() if m.group(2) in ('asmtu', 'asm') else None))
            bloque = None
            continue
        m2 = re.search(r'start: 0x([0-9a-fA-F]+)', l)
        if m2:
            bloque = {'start': int(m2.group(1), 16)}
            marcas.append((bloque['start'], None))
            continue
        if bloque is not None:
            mt = re.search(r'^\s+type: (\S+)', l)
            mn = re.search(r'^\s+name: (\S+)', l)
            if mt:
                bloque['type'] = mt.group(1)
            if mn:
                bloque['name'] = mn.group(1)
            if 'type' in bloque and 'name' in bloque:
                if bloque['type'] in ('asmtu', 'asm'):
                    marcas[-1] = (bloque['start'], bloque['name'])
                bloque = None
    marcas.sort(key=lambda m: m[0])
    out = []
    for i, (a, n) in enumerate(marcas):
        if n and n.startswith('Speed/Indep/SourceLists/') and i + 1 < len(marcas):
            out.append((n.split('/')[-1], a - 0x1000 + 0x100000, marcas[i + 1][0] - 0x1000 + 0x100000))
    return out


def main():
    pedidas = set(a if a.endswith('.cpp') else a + '.cpp' for a in sys.argv[1:])
    mdebug.carga()
    funcs = []
    actual = None
    for iss, value, st, sc in mdebug._cache['syms']:
        if st == 5:
            nom = mdebug._cadena(iss)
            if nom.endswith(('.cpp', '.c', '.cc')):
                actual = nom
        elif st in (6, 14):
            funcs.append((value, actual))
    funcs.sort()
    arbol = {p.lower(): p for p in subprocess.run(['git', 'ls-files', 'src'], capture_output=True, text=True,
                                                   cwd=ROOT).stdout.split()}
    for u, lo, hi in rangos_unidades():
        if pedidas and u not in pedidas:
            continue
        incl = set()
        sl = ROOT + 'src/Speed/Indep/SourceLists/' + u
        if os.path.exists(sl):
            for l in open(sl, errors='replace'):
                m = re.match(r'\s*#include\s+"([^"]+)"', l)
                if m:
                    incl.add(('src/' + m.group(1)).lower())
        por = collections.OrderedDict()
        for v, f in funcs:
            if lo <= v < hi and f:
                por[f] = por.get(f, 0) + 1
        print('== %s (0x%x-0x%x): %d ficheros' % (u, lo, hi, len(por)))
        for f, n in por.items():
            r = 'src/' + f.lower().split('/mw/', 1)[-1]
            if r in incl:
                marca = 'incluido'
            elif r in arbol:
                marca = 'FALTA en la SourceList (%d lineas)' % sum(1 for _ in open(ROOT + arbol[r], errors='replace'))
            else:
                marca = 'no existe'
            print('   %4d fn  %-66s %s' % (n, r[4:70], marca))


if __name__ == '__main__':
    main()
