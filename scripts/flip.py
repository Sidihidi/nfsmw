#!/usr/bin/env python3
"""flip.py -- prueba a convertir las guardas EA_BUILD_A124 de una funcion en guardas de PS2.

Una guarda `#ifdef/#ifndef EA_BUILD_A124` dice "solo la A124" (o "todas menos la A124"); a veces lo que la A124 tiene
de distinto lo tienen tambien SLUS (las dos PS2) y la guarda buena es EA_PLATFORM_PLAYSTATION2. Esto cambia cada
guarda del cuerpo de la funcion (desde LINEA hasta su llave de cierre, y N lineas antes) por EA_PLATFORM_PLAYSTATION2,
en todas las combinaciones (16 como mucho), y mide SIM con banco.py. No toca src/: las copias van a build/flip/.

    python scripts/flip.py REL LINEA UNIDAD SIM [--antes=N] [--v=A|S|G]

    REL     el fichero relativo a src/ (Speed/Indep/Src/...)
    LINEA   la primera linea de la funcion
    UNIDAD  la SourceList que lo incluye
    SIM     el simbolo que se mide
    --v     version: S (SLUS, por defecto), A (la A124) o G (GameCube)
"""
import itertools
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TMP = os.path.join(RAIZ, 'build', 'flip')


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if len(args) < 4:
        print(__doc__)
        sys.exit(2)
    rel, linea, unidad, sim = args[0], int(args[1]), args[2], args[3]
    antes = 0
    version = 'SLUS-21351'
    for a in sys.argv[1:]:
        if a.startswith('--antes='):
            antes = int(a[len('--antes='):])
        if a.startswith('--v='):
            version = {'A': 'SLES-53558-A124', 'S': 'SLUS-21351', 'G': 'GOWE69'}[a[len('--v='):]]
    texto = open(os.path.join(RAIZ, 'src', rel.replace('/', os.sep)), newline='', encoding='utf-8').read()
    nl = '\r\n' if '\r\n' in texto else '\n'
    lineas = texto.split(nl)
    i0 = linea - 1
    nivel, empezado, i1 = 0, False, i0
    for i in range(i0, len(lineas)):
        nivel += lineas[i].count('{') - lineas[i].count('}')
        if '{' in lineas[i]:
            empezado = True
        if empezado and nivel <= 0:
            i1 = i
            break
    guardas = [i for i in range(max(0, i0 - antes), i1 + 1)
               if re.match(r'\s*#\s*(ifdef|ifndef)\s+EA_BUILD_A124\b', lineas[i])]
    print('guardas:', [(g + 1, lineas[g].strip()) for g in guardas])
    os.makedirs(TMP, exist_ok=True)

    def prueba(combo):
        nuevas = lineas[:]
        for g in combo:
            nuevas[g] = nuevas[g].replace('EA_BUILD_A124', 'EA_PLATFORM_PLAYSTATION2')
        etiqueta = 'f' + ''.join(str(guardas.index(g)) for g in combo) if combo else 'f_'
        copia = os.path.join(TMP, etiqueta + '_' + os.path.basename(rel))
        open(copia, 'w', newline='', encoding='utf-8').write(nl.join(nuevas))
        r = subprocess.run([sys.executable, os.path.join(RAIZ, 'scripts', 'banco.py'), version, unidad, rel, copia, sim],
                           cwd=RAIZ, capture_output=True, text=True, errors='replace')
        os.remove(copia)
        res = [l for l in r.stdout.splitlines() if '%' in l and 'size=' in l]
        return combo, (res[0] if res else r.stdout[-300:] + r.stderr[-500:])

    combos = []
    for n in range(0, len(guardas) + 1):
        combos += list(itertools.combinations(guardas, n))
    with ThreadPoolExecutor(4) as ex:
        for combo, res in ex.map(prueba, combos[:16]):
            print([g + 1 for g in combo], res)
    try:
        os.rmdir(TMP)
    except OSError:
        pass


if __name__ == '__main__':
    main()
