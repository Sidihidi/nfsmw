#!/usr/bin/env python3
"""todas.py V ETIQ [opciones de bancopc] [--unidades=A,B] [--hilos=N] [--par] -- bancopc.py en todas las unidades de V
(o las pedidas), en paralelo; resume ganadas/perdidas frente a --base.

--par: para cada unidad compila primero el control (sin sombra, etiqueta c_ETIQ) y justo despues la variante, y
compara con ese control: asi un cambio del arbol o de los objetos del original entre medias (otros agentes) no se
cuela como ganancia o perdida."""
import concurrent.futures
import os
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
AQUI = os.path.dirname(os.path.abspath(__file__))
args = [a for a in sys.argv[1:] if not a.startswith('--')]
v, etiq = args[0], args[1]
opts = [a for a in sys.argv[1:] if a.startswith('--') and not a.startswith(('--unidades=', '--hilos=', '--par'))]
unidades = None
hilos = 4
par = '--par' in sys.argv
for a in sys.argv[1:]:
    if a.startswith('--unidades='):
        unidades = a.split('=', 1)[1].split(',')
    elif a.startswith('--hilos='):
        hilos = int(a.split('=', 1)[1])
if unidades is None:
    unidades = sorted(f[:-4] for f in os.listdir(os.path.join(RAIZ, 'build', v, 'obj', 'Speed', 'Indep', 'SourceLists'))
                      if f.endswith('.obj'))


def corre(argumentos):
    for intento in range(4):
        r = subprocess.run([sys.executable, os.path.join(AQUI, 'bancopc.py')] + argumentos, capture_output=True,
                           text=True, errors='replace')
        out = r.stdout + r.stderr
        if 'C1083' not in out:
            break
    return out


def uno(u):
    if par:
        control = 'c_' + etiq
        out1 = corre([v, u, control])
        resto = [o for o in opts if not o.startswith('--base=')] + ['--base=' + control]
        return u, corre([v, u, etiq] + resto) if 'NO COMPILA' not in out1 else out1
    return u, corre([v, u, etiq] + opts)


with concurrent.futures.ThreadPoolExecutor(hilos) as ex:
    for u, out in ex.map(uno, unidades):
        print(out.rstrip(), flush=True)
