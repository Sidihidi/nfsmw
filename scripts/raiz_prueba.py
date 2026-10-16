#!/usr/bin/env python3
"""raiz_prueba.py -- raiz FALSA para probar un symbols.txt (y un config.yml) de SLUS sin tocar el arbol ni el candado.

    python scripts/raiz_prueba.py DIR SYMBOLS [--yml config.yml] [--sin-trocear] [--con-a124]
    python scripts/raiz_prueba.py DIR --quitar-union          # SIEMPRE antes de borrar DIR

Monta en DIR lo que leen las herramientas de nombres, que sacan su raiz de __file__:
  - DIR/scripts: copia de scripts/*.py; DIR/tools/nombra_ps2.py: copia;
  - DIR/orig/SLUS-21351/SLUS_213.51: copia del ELF; DIR/config/SLUS-21351: el symbols.txt candidato (y el
    config.yml, si se pasa --yml);
  - DIR/build/SLUS-21351: el troceado de scripts/trocea_prueba.py con esos ficheros (asm, obj, config.json);
  - DIR/build/SLUS-21351/src: UNION (junction) a nuestros objetos del arbol, que no se copian;
  - con --con-a124, uniones a build/, config/ y orig/ de SLES-53558-A124 (solo lectura), para las herramientas que
    comparan con la A124 (scripts/vtslots.py, grafo_nombres.py, cruzaver.py, datos_ref.py).
Las uniones se quitan con `rmdir` (nunca con un borrado recursivo, que borraria el destino): --quitar-union.

Despues, por ejemplo:
    python DIR/scripts/prueba_lote.py SLUS-21351 lista.tsv          (IGUAL / distintas de cada nombre)
    python DIR/scripts/cortes.py SLUS-21351, restos_epilogo.py, restos_trozos.py
    python scripts/informe_prueba.py SLUS-21351 DIR/build/SLUS-21351  (informe de objdiff, como pctall)
    python scripts/compara_informes.py ANTES.json DIR/build/SLUS-21351/report.json
Tanda 6 (2026-09-23): con esto se probaron 345 nombres y 21 fronteras de SLUS (+120 funciones previstas, +121 medidas).
"""
import glob
import json
import os
import shutil
import subprocess
import sys

REAL = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
V, A = 'SLUS-21351', 'SLES-53558-A124'


def uniones(D):
    return [os.path.join(D, 'build', V, 'src'), os.path.join(D, 'build', A), os.path.join(D, 'config', A),
            os.path.join(D, 'orig', A)]


def quitar_union(D):
    for u in uniones(D):
        if os.path.lexists(u):
            # rmdir de cmd quita la union sin tocar su destino
            subprocess.run(['cmd', '/c', 'rmdir', u], check=True)
        assert not os.path.lexists(u), u


def une(enlace, destino):
    os.makedirs(os.path.dirname(enlace), exist_ok=True)
    subprocess.run(['cmd', '/c', 'mklink', '/J', enlace, destino], check=True, capture_output=True)


def main():
    args = [a for i, a in enumerate(sys.argv[1:], 1) if not a.startswith('--') and sys.argv[i - 1] != '--yml']
    if not args:
        sys.exit(__doc__)
    D = os.path.abspath(args[0])
    if '--quitar-union' in sys.argv:
        quitar_union(D)
        return
    sym = os.path.abspath(args[1])
    yml = os.path.abspath(sys.argv[sys.argv.index('--yml') + 1]) if '--yml' in sys.argv else None
    quitar_union(D)
    os.makedirs(os.path.join(D, 'scripts'), exist_ok=True)
    for p in glob.glob(os.path.join(REAL, 'scripts', '*.py')):
        shutil.copy2(p, os.path.join(D, 'scripts'))
    os.makedirs(os.path.join(D, 'tools'), exist_ok=True)
    shutil.copy2(os.path.join(REAL, 'tools', 'nombra_ps2.py'), os.path.join(D, 'tools'))
    os.makedirs(os.path.join(D, 'orig', V), exist_ok=True)
    if not os.path.exists(os.path.join(D, 'orig', V, 'SLUS_213.51')):
        shutil.copy2(os.path.join(REAL, 'orig', V, 'SLUS_213.51'), os.path.join(D, 'orig', V))
    os.makedirs(os.path.join(D, 'config', V), exist_ok=True)
    if os.path.abspath(os.path.join(D, 'config', V, 'symbols.txt')) != sym:
        shutil.copy2(sym, os.path.join(D, 'config', V, 'symbols.txt'))
    if yml and os.path.abspath(os.path.join(D, 'config', V, 'config.yml')) != yml:
        shutil.copy2(yml, os.path.join(D, 'config', V, 'config.yml'))
    B = os.path.join(D, 'build', V)
    if '--sin-trocear' not in sys.argv:
        for sub in ('asm', 'obj'):
            p = os.path.join(B, sub)
            if os.path.isdir(p):
                shutil.rmtree(p)
        extra = ['--yml', yml] if yml else []
        r = subprocess.run([sys.executable, os.path.join(REAL, 'scripts', 'trocea_prueba.py'), V, B, '--symbols',
                            os.path.join(D, 'config', V, 'symbols.txt')] + extra, capture_output=True, text=True)
        print(r.stdout.strip()[-2000:], r.stderr.strip()[-2000:])
        if r.returncode:
            sys.exit(1)
    u = json.load(open(os.path.join(B, 'unidades.json')))
    units = []
    for s in u:
        s = s.replace('\\', '/')
        units.append({'name': s[s.index('/asm/') + 5:][:-2]})
    json.dump({'units': units}, open(os.path.join(B, 'config.json'), 'w'))
    une(os.path.join(B, 'src'), os.path.join(REAL, 'build', V, 'src'))
    if '--con-a124' in sys.argv:
        for sub in ('build', 'config', 'orig'):
            une(os.path.join(D, sub, A), os.path.join(REAL, sub, A))
    print('raiz lista:', D)


if __name__ == '__main__':
    main()
