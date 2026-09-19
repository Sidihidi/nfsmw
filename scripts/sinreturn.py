#!/usr/bin/env python3
"""sinreturn.py [--slus] [zX.cpp ...] -- compila SourceLists de PS2 con -Wreturn-type y lista las funciones no-void
que pueden terminar sin `return` ("control reaches end of non-void function"), una vez por sitio, con las unidades
que lo ven. No toca build/: escribe en build/sinreturn/.

Por que: bMin/bMax(float) de bMath.hpp tenian la rama de PS2 VACIA; el GameCube usa su `fsel` y nadie lo notaba,
pero en PS2 las dos devolvian basura en 1.193 min.s/max.s del original. La mayoria de los avisos son accesores
vacios que el original nunca expande (inofensivos); los que importan son inline de cabecera vistos por muchas
unidades y con cuerpo en otra plataforma.

Usa los flags de la A124 que deja scripts/sondastabs.py (build/sondastabs/flags.json); con --slus les quita
EA_BUILD_A124 y MILESTONE_BUILD. Sin unidades, todas las del fichero de flags.
"""
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)


def main():
    args = sys.argv[1:]
    slus = '--slus' in args
    args = [a for a in args if a != '--slus']
    flags = json.load(open(os.path.join('build', 'sondastabs', 'flags.json')))
    cc = os.path.normpath('build/compilers/PS2/ee-gcc2.9-991111/bin/ee-gcc.exe')
    base = os.path.join('build', 'sinreturn', 'slus' if slus else 'a124')
    os.makedirs(base, exist_ok=True)
    comun = next(iter(flags.values()))

    def fl(sl):
        f = list(flags.get(sl, comun))
        if slus:
            f = [x.replace('SLES-53558-A124', 'SLUS-21351') for x in f if x not in ('-DEA_BUILD_A124', '-DMILESTONE_BUILD')]
        return f

    def uno(sl):
        src = os.path.normpath('src/Speed/Indep/SourceLists/' + sl)
        out = os.path.join(base, sl.replace('.cpp', '.s'))
        r = subprocess.run([sys.executable, os.path.normpath('tools/ee_gcc_pp.py'), os.path.join(base, 'pp_' + sl[:-4]), cc,
                            '--'] + fl(sl) + ['-S', '-Wreturn-type', '-o', out, src], capture_output=True, text=True,
                           errors='replace')
        lineas = (r.stderr or '').split('\n')
        sitios = []
        for i, l in enumerate(lineas):
            if 'control reaches end of non-void' in l or 'no return statement' in l:
                ctx = next((lineas[j] for j in range(i - 1, max(i - 4, -1), -1)
                            if 'In function' in lineas[j] or 'In method' in lineas[j]), '')
                m = re.match(r'(.*?):(\d+):', l)
                sitios.append(((m.group(1), int(m.group(2))) if m else (l, 0), ctx.strip()))
        return sl, r.returncode, sitios

    unidades = args or sorted(flags)
    vistos = {}
    with ThreadPoolExecutor(4) as ex:
        for sl, rc, sitios in ex.map(uno, unidades):
            for (f, n), ctx in sitios:
                vistos.setdefault((f, n), (ctx, set()))[1].add(sl[:-4])
            print('%-22s rc=%d  %d avisos' % (sl, rc, len(sitios)), flush=True)
    print()
    for (f, n), (ctx, us) in sorted(vistos.items(), key=lambda kv: (-len(kv[1][1]), kv[0])):
        print('%s:%d  %s  [%d unidades]' % (f, n, ctx[-100:], len(us)))
    print('%d sitios distintos' % len(vistos))


if __name__ == '__main__':
    main()
