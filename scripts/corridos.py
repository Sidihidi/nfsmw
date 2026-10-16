#!/usr/bin/env python3
"""corridos.py VERSION UNIDAD [UNIDAD...] -- por clase, las funciones de PC o Xbox 360 cuyos desplazamientos de
miembro difieren del original en una cantidad constante: un miembro de otro tamano en esa plataforma.

Solo mira funciones del MISMO tamano en los dos lados y por debajo del 100 %, compara los accesos `[reg+0xN]` (x86) o
`0xN(rN)` (PowerPC) instruccion a instruccion y cuenta, por clase, cuanto se corre cada acceso. Una clase con muchas
funciones y la misma diferencia (+0x28 en todas) tiene un miembro de otro tamano ANTES de esos accesos; si hay dos
saltos (+0x28 y luego +0x50) son dos miembros. Asi se encontraron los limites de PC/360 (AIPursuit: cada
IVehicle::List de 0x60 en vez de 0x38; memoria nfsmw-limites-pc-x360.md).

Una sola llamada a `objdiff-cli diff` por unidad (saca todas las funciones; el nombre va en `name`); el JSON queda en
build/<VERSION>/corridos_<UNIDAD>.json por si hace falta mirar una funcion a mano.

    python scripts/corridos.py SPEED_EXE_1_3 zAI zFe
    python scripts/corridos.py EUROPEGERMILESTONE zAI
"""
import collections
import json
import os
import re
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
X86 = re.compile(r'\[(?:e[a-d]x|e[sd]i|ebp)\+0x([0-9a-f]+)\]')
PPC = re.compile(r'\b0x([0-9a-f]+)\(r(?:[3-9]|[12][0-9]|3[01])\)')


def clase_de(nombre):
    m = re.match(r'\?[^@]+@([A-Za-z_][A-Za-z0-9_]*)@@', nombre)
    return m.group(1) if m else None


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    version = sys.argv[1]
    rx = X86 if version == 'SPEED_EXE_1_3' else PPC
    for unidad in sys.argv[2:]:
        sl = os.path.join('Speed', 'Indep', 'SourceLists', unidad + '.obj')
        objetivo = os.path.join(RAIZ, 'build', version, 'obj', sl)
        nuestro = os.path.join(RAIZ, 'build', version, 'src', sl)
        salida = os.path.join(RAIZ, 'build', version, 'corridos_%s.json' % unidad)
        subprocess.run([os.path.join(RAIZ, 'build', 'tools', 'objdiff-cli.exe'), 'diff', '-1', objetivo, '-2', nuestro,
                        '-o', salida, '--format', 'json', 'x'], capture_output=True)
        d = json.load(open(salida, encoding='utf-8'))
        lados = {}
        for lado in ('left', 'right'):
            m = {}
            for s in d[lado]['symbols']:
                if s.get('instructions'):
                    m[s.get('name', '')] = (s.get('match_percent'), s.get('size'),
                                            [i.get('instruction', {}).get('formatted', '') for i in s['instructions']])
            lados[lado] = m
        por_clase = collections.defaultdict(collections.Counter)
        funciones = collections.defaultdict(list)
        for nombre, (pct, tam, ins) in lados['left'].items():
            if nombre not in lados['right'] or pct is None or pct >= 100:
                continue
            _, tam2, ins2 = lados['right'][nombre]
            if tam != tam2 or len(ins) != len(ins2):
                continue
            difs = set()
            for a, b in zip(ins, ins2):
                oa = [int(x, 16) for x in rx.findall(a)]
                ob = [int(x, 16) for x in rx.findall(b)]
                if len(oa) == len(ob):
                    difs.update(x - y for x, y in zip(oa, ob) if x != y)
            clase = clase_de(nombre)
            if clase and difs:
                for dd in difs:
                    por_clase[clase][dd] += 1
                funciones[clase].append(nombre)
        print('== %s %s' % (version, unidad))
        for clase, cnt in sorted(por_clase.items(), key=lambda kv: -sum(kv[1].values()))[:30]:
            top = ', '.join('%+#x x%d' % (k, v) for k, v in cnt.most_common(4))
            print('  %-34s %3d fn  %s' % (clase[:34], len(funciones[clase]), top))


if __name__ == '__main__':
    main()
