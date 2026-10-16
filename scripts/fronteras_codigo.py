#!/usr/bin/env python3
"""fronteras_codigo.py -- fronteras de CODIGO de SLUS corridas: para cada unidad de SLUS con pareja en la A124, la
primera funcion con nombre de la unidad de la A124 se busca en symbols.txt de SLUS; si esta ANTES del comienzo actual
de la unidad (dentro de la anterior), la frontera esta tarde y esa funcion no puede casar con nuestro objeto.

    python scripts/fronteras_codigo.py [SYMBOLS_SLUS]

Imprime unidad, comienzo actual (start del ELF), la primera funcion de la A124 con su direccion en SLUS y el start
propuesto (vram - 0xff000). Tanda 6: zEAXSound (el constructor de AudioMemoryManager), pathaction, cddvd_device,
filesys_c y filesys_cc. Las unidades que SLUS tenia fundidas en otra (syncfile, systask, event, eventqueue...) se ven
mejor con scripts/cruzaver.py.
"""
import collections
import json
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
A, S = 'SLES-53558-A124', 'SLUS-21351'


def unidades_asm(V):
    cfg = json.load(open(os.path.join(RAIZ, 'build', V, 'config.json')))
    out = {}
    for u in cfg['units']:
        p = os.path.join(RAIZ, 'build', V, 'asm', u['name'] + '.s')
        if not os.path.exists(p):
            continue
        fs = []
        g = None
        for l in open(p, encoding='utf-8', errors='replace'):
            m = re.match(r'\s*glabel (\S+)', l)
            if m:
                g = m.group(1)
                continue
            m = re.match(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s', l)
            if m and g:
                fs.append((int(m.group(1), 16), g))
                g = None
            if l.startswith('.section') and '.text' not in l and '.over' not in l and fs:
                break
        out[u['name']] = fs
    return out


def main():
    symf = sys.argv[1] if len(sys.argv) > 1 else os.path.join(RAIZ, 'config', S, 'symbols.txt')
    sym = collections.defaultdict(list)
    for l in open(symf, encoding='utf-8'):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);', l)
        if m:
            sym[m.group(1)].append(int(m.group(2), 16))
    ua, us = unidades_asm(A), unidades_asm(S)
    # comienzo de cada unidad de SLUS: su primera instruccion
    ini_s = {u: fs[0][0] for u, fs in us.items() if fs}
    orden = sorted(ini_s, key=ini_s.get)
    for i, u in enumerate(orden):
        if u not in ua or not ua[u]:
            continue
        # primera funcion con nombre de la A124
        nombradas = [(a, g) for a, g in ua[u] if not re.match(r'(func|fn)_[0-9A-F]{8}$', g)]
        if not nombradas:
            continue
        a0, g0 = nombradas[0]
        es_primera = ua[u][0][1] == g0
        dirs = sym.get(g0, [])
        if len(dirs) != 1:
            continue
        x = dirs[0]
        prev = orden[i - 1] if i else None
        if x < ini_s[u] and prev and ini_s[prev] <= x:
            print('%-60s start 0x%06x  primera de la A124 %s en 0x%x (vram) -> start 0x%06x %s  [en %s]' % (
                u[-60:], ini_s[u] - 0xff000, g0[:50], x, x - 0xff000, '' if es_primera else '(no es la 1a de la A124)',
                prev.split('/')[-1]))


if __name__ == '__main__':
    main()
