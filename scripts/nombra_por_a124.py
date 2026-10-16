#!/usr/bin/env python3
"""nombra_por_a124.py -- nombres para las func_ de SLUS copiando los de la MISMA unidad de la A124 cuando el codigo es
el mismo.

Las bibliotecas (libc, libm, libkernel, realcore, realgraph...) son el mismo objeto en las dos PS2. En SLUS muchas
estaticas se quedaron sin nombre (lflush de refill.c, las ranuras de InterfaceImp...) y objdiff empareja por NOMBRE:
nuestro objeto las define y no casan. Para cada funcion sin nombre de una unidad de SLUS (menos las SourceLists del
juego, donde la firma puede cambiar entre las dos builds) busca en la unidad del mismo nombre de la A124 las funciones
con nombre del mismo tamano y el mismo codigo (palabra a palabra; en jal/j solo el codigo de operacion y en las de
inmediato el codigo y los registros: las reubicaciones cambian). Propone el nombre si hay UNA sola, el nombre no esta
ya en esa unidad de SLUS y ninguna otra func_ de la unidad tiene el mismo codigo.

    python scripts/nombra_por_a124.py [DIR_TROCEADO_SLUS] [--tsv SALIDA]

DIR_TROCEADO_SLUS: un troceado de scripts/trocea_prueba.py (su asm/ y unidades.json); por defecto el del arbol
(build/SLUS-21351). La A124 siempre es la del arbol. Solo informa; los nombres se pasan a mano (o con --tsv a
scripts/aplica_nombres.py) a config/SLUS-21351/symbols.txt.
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from trocea_como_a124 import lee, mismo_codigo, SIN_NOMBRE, RE_NM, RE_INS  # noqa: E402

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
A, S = 'SLES-53558-A124', 'SLUS-21351'


def por_unidad(asm_dir, nombres):
    out = {}
    for nm in nombres:
        p = os.path.join(asm_dir, nm + '.s')
        if not os.path.exists(p):
            continue
        fns = []
        pend = None
        for l in open(p, encoding='utf-8', errors='replace'):
            m = RE_NM.match(l)
            if m:
                pend = [m.group(1), int(m.group(2), 16)]
                continue
            if pend:
                m = RE_INS.match(l)
                if m:
                    fns.append((int(m.group(2), 16), pend[1], pend[0]))
                    pend = None
        out[nm] = fns
    return out


def unidades_de(d):
    if os.path.exists(os.path.join(d, 'unidades.json')):
        us = json.load(open(os.path.join(d, 'unidades.json')))
        base = os.path.join(d, 'asm').replace('\\', '/')
        out = []
        for s in us:
            s = s.replace('\\', '/')
            rel = s[s.index('/asm/') + 5:] if '/asm/' in s else os.path.relpath(s, base).replace('\\', '/')
            out.append(rel.removesuffix('.s'))
        return out
    return [u['name'] for u in json.load(open(os.path.join(d, 'config.json')))['units']]


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    ds = os.path.abspath(args[0]) if args else os.path.join(RAIZ, 'build', S)
    da = os.path.join(RAIZ, 'build', A)
    us = [u for u in unidades_de(ds) if not u.startswith('Speed/Indep/SourceLists/')]
    fs = por_unidad(os.path.join(ds, 'asm'), us)
    fa = por_unidad(os.path.join(da, 'asm'), [u for u in unidades_de(da) if u in fs])
    props = []
    for u, lst in sorted(fs.items()):
        if u not in fa:
            continue
        ya = {n for a, t, n in lst if not SIN_NOMBRE.match(n)}
        cand_a = [(a, t, n) for a, t, n in fa[u] if not SIN_NOMBRE.match(n) and n not in ya]
        sin = [(a, t, n) for a, t, n in lst if SIN_NOMBRE.match(n)]
        for a, t, n in sin:
            cod = lee(S, a, t)
            igual = [x for x in cand_a if x[1] == t and mismo_codigo(cod, lee(A, x[0], t))]
            if len(igual) != 1:
                continue
            otras = [y for y in sin if y is not (a, t, n) and y[1] == t and y[0] != a
                     and mismo_codigo(lee(S, y[0], t), cod)]
            if otras:
                continue
            props.append((u, a, n, igual[0][2], t))
    for u, a, n, nombre, t in props:
        print('%-60s %08x %-16s -> %s (%d B)' % (u[-60:], a, n, nombre, t))
    print('%d nombres' % len(props))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8') as fh:
            for u, a, n, nombre, t in props:
                fh.write('%s\t0x%x\t%s\t%d\n' % (nombre, a, u, t))


if __name__ == '__main__':
    main()
