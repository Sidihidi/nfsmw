#!/usr/bin/env python3
"""stabs_unidades.py -- pasa scripts/estructura.py por todas las funciones casi exactas de unas unidades de PS2:
compila cada unidad tal como esta en el arbol con scripts/banco.py --asm (los stabs de nuestro .s) y compara cada
funcion con MIN <= porcentaje < MAX contra el .mdebug de la A124. Una linea por funcion (ver estructura.py -q).

    python scripts/stabs_unidades.py V MIN MAX unidad [unidad...] [--hilos=N] [--patron=TOKENS]
    python scripts/stabs_unidades.py SLES-53558-A124 90 100 zAI zPhysics zEAXSound
    python scripts/stabs_unidades.py SLES-53558-A124 90 100 --todas --patron=-temp:r,+c:r

    V         SLES-53558-A124 (el oraculo es su .mdebug) o SLUS-21351 (la venta, contra los stabs de la A124:
              vale para la forma, no para lo que la A124 aun no tenia)
    --todas   todas las unidades de juego (Speed/Indep/SourceLists) del informe
    --patron  solo las funciones cuyas diferencias NETAS de nombre (sin el orden) contienen alguno de esos tokens
              (+ nuestro, - original): `-temp:r` es un bFloatToInt que el original expande y nosotros no; `-a:r +c:r`, un
              inline cuyo parametro el original modifica y nosotros copiamos a una local
    --hilos   compilaciones a la vez (4)

Los porcentajes salen del informe OFICIAL (build/versiones/V.json, el de scripts/medir.py) o, si no esta, de
build/V/report.json. Los .s van a build/banco/stabs/ y se borran al terminar (--conservar los deja).
"""
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import estructura  # noqa: E402

args = [a for a in sys.argv[1:] if not a.startswith('--')]
if len(args) < 3:
    print(__doc__)
    sys.exit(2)
V, MIN, MAX = args[0], float(args[1]), float(args[2])
uds = args[3:]
HILOS = 4
PATRON = None
for a in sys.argv[1:]:
    if a.startswith('--hilos='):
        HILOS = int(a.split('=', 1)[1])
    elif a.startswith('--patron='):
        PATRON = [x for x in a.split('=', 1)[1].split(',') if x]
CONSERVAR = '--conservar' in sys.argv

rep = os.path.join(RAIZ, 'build', 'versiones', V + '.json')
if not os.path.exists(rep):
    rep = os.path.join(RAIZ, 'build', V, 'report.json')
r = json.load(open(rep, encoding='utf-8'))
casi = {}
for u in r['units']:
    if '/SourceLists/' not in u['name']:
        continue
    un = u['name'].split('/')[-1]
    if '--todas' in sys.argv or un in uds:
        casi[un] = [(f['name'], f.get('fuzzy_match_percent', 0) or 0, int(f.get('size', 0)))
                    for f in u.get('functions', []) if MIN <= (f.get('fuzzy_match_percent', 0) or 0) < MAX]
casi = {k: v for k, v in casi.items() if v}
D = os.path.join(RAIZ, 'build', 'banco', 'stabs')
os.makedirs(D, exist_ok=True)

# zFeOverlay lleva -fno-implicit-templates en configure.py; las demas unidades de juego, los flags de banco.py
EXTRA = {'zFeOverlay': '-fno-implicit-templates'}


def uno(un):
    sl = os.path.join(RAIZ, 'src', 'Speed', 'Indep', 'SourceLists', un + '.cpp')
    if not os.path.exists(sl):
        return '%-12s sin SourceList' % un
    texto = open(sl, encoding='utf-8').read()
    incs = re.findall(r'^#include "([^"]+\.(?:cpp|c))"', texto, re.M)
    incs = [i for i in incs if texto.count('#include "%s"' % i) == 1 and os.path.exists(os.path.join(RAIZ, 'src', i))]
    if not incs:
        return '%-12s sin fuentes' % un
    # banco.py necesita sustituir un fichero: el primero, por el mismo (la unidad tal como esta en el arbol)
    rel = incs[0]
    s = os.path.join(D, '%s_%s_%d.s' % (un, V, os.getpid()))
    cmd = [sys.executable, os.path.join(AQUI, 'banco.py'), V, un, rel, os.path.join(RAIZ, 'src', rel), 'x',
           '--asm=' + s]
    if un in EXTRA:
        cmd.append('--extra=' + EXTRA[un])
    p = subprocess.run(cmd, capture_output=True, text=True, cwd=RAIZ)
    if not os.path.exists(s):
        return '%-12s NO COMPILA: %s' % (un, (p.stdout + p.stderr)[-300:].replace('\n', ' '))
    txt = open(s, errors='replace').read()
    out = []
    for name, pct, size in casi[un]:
        q = estructura.resumen(txt, name)
        # el patron mira las diferencias NETAS (sin el orden), no el diff ordenado
        netas = q.split('netas:', 1)[1].split('|', 1)[0].split() if 'netas:' in q else []
        if PATRON and not any(t in netas for t in PATRON):
            continue
        out.append('%-12s %6.2f %5d %s' % (un, pct, size, q))
    if not CONSERVAR:
        os.remove(s)
    return '\n'.join(out) if out else '%-12s (nada)' % un


estructura.mdebug.carga()  # una sola vez, antes de los hilos
with ThreadPoolExecutor(HILOS) as ex:
    for res in ex.map(uno, sorted(casi)):
        print(res, flush=True)
