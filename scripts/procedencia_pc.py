#!/usr/bin/env python3
"""procedencia_pc.py -- las filas que le faltan a config/SPEED_EXE_1_3/nombres_procedencia.tsv, sacadas de git log.

El tsv solo tiene los nombres que pusieron los scripts que lo escriben (nombra_elim, nombra_corrige, funclets_pc, gcpc,
barre_bib...) y los carriles que anotaron su regla. Los que se pusieron a mano o con otra herramienta desde el 7-oct
(el documento de procedencia de la ronda 87 contaba 850) no tienen fila. Este script recorre los commits que tocaron
config/SPEED_EXE_1_3/symbols.json desde --desde (por defecto 2026-10-07), del mas viejo al mas nuevo, y para cada
funcion cuyo nombre VIGENTE puso uno de esos commits y que no tiene fila vigente en el tsv, propone una fila:

    direccion  nombre_anterior  nombre_nuevo  regla=git-<carril del asunto>  marca=<commit corto>  bytes  0  fecha

(<carril del asunto> es la primera palabra del asunto del commit sin los dos puntos, p. ej. `pca87`, `x360b87`; si el
asunto no empieza por un carril, sus dos primeras palabras: `carril-m3`, `ventana-r39`, `config-ronda`). Los nombres que siguen siendo de IDA/upstream (anteriores a --desde) no se
tocan: el documento de procedencia ya los cuenta por commit.

    python scripts/procedencia_pc.py [--desde AAAA-MM-DD] [--aplicar] [--lista]

Sin --aplicar solo cuenta (y con --lista imprime las filas). Con --aplicar anade las filas al final del tsv con
scripts/escritura.py. Idempotente: una segunda pasada no encuentra nada que anadir.
"""
import argparse
import json
import os
import re
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
from escritura import escribe  # noqa: E402

SYM = 'config/SPEED_EXE_1_3/symbols.json'
TSV = 'config/SPEED_EXE_1_3/nombres_procedencia.tsv'
RE_START = re.compile(r'^[ +-]\s*"start": (\d+),')
RE_NAME = re.compile(r'^([+-])\s*"name": "(.*)",\s*$')


def git(*a):
    return subprocess.run(['git'] + list(a), cwd=RAIZ, capture_output=True, encoding='utf-8', errors='replace').stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--desde', default='2026-10-07')
    ap.add_argument('--aplicar', action='store_true')
    ap.add_argument('--lista', action='store_true')
    a = ap.parse_args()

    sy = json.load(open(os.path.join(RAIZ, SYM), encoding='utf-8'))
    actual = {f['start']: (f['name'], f.get('end', f['start']) - f['start']) for f in sy['functions']}

    tsv_txt = open(os.path.join(RAIZ, TSV), encoding='utf-8', newline='').read()
    con_fila = set()
    for l in tsv_txt.splitlines():
        if l.startswith('#') or not l.strip():
            continue
        c = l.split('\t')
        if len(c) >= 3 and c[0].isdigit():
            con_fila.add((int(c[0]), c[2]))

    commits = git('log', '--reverse', '--since=' + a.desde, '--format=%H\t%ad\t%s', '--date=short', '--', SYM)
    puesto = {}  # direccion -> (nombre puesto, nombre anterior, commit, fecha, carril)
    for linea in commits.splitlines():
        h, fecha, asunto = linea.split('\t', 2)
        m = re.match(r'([A-Za-z]+[0-9]+[a-z]?)\b', asunto)
        if m:
            carril = m.group(1)
        else:  # "Carril M3 (PC)...", "Ventana r39: ...", "Config ronda 61": las dos primeras palabras
            pal = [re.sub(r'[^0-9a-z]', '', p.lower()) for p in asunto.split()[:2]]
            carril = '-'.join(p for p in pal if p) or 'git'
        diff = git('show', '--format=', '-U3', h, '--', SYM)
        addr = None
        quitado = {}
        for l in diff.splitlines():
            if l.startswith('@@'):
                addr = None
                continue
            ms = RE_START.match(l)
            if ms:
                addr = int(ms.group(1))
                continue
            mn = RE_NAME.match(l)
            if mn and addr is not None:
                if mn.group(1) == '-':
                    quitado[addr] = mn.group(2)
                else:
                    puesto[addr] = (mn.group(2), quitado.get(addr, ''), h[:9], fecha, carril)
    filas = []
    for addr, (nom, ant, h, fecha, carril) in sorted(puesto.items()):
        if addr not in actual or actual[addr][0] != nom:
            continue  # lo cambio despues otro commit o el arbol
        if (addr, nom) in con_fila:
            continue
        filas.append('%d\t%s\t%s\tgit-%s\t%s\t%d\t0\t%s' % (addr, ant, nom, carril, h, actual[addr][1], fecha))
    print('nombres vigentes puestos desde %s por commits: %d; sin fila en el tsv: %d (%d B)' % (
        a.desde, sum(1 for ad, v in puesto.items() if ad in actual and actual[ad][0] == v[0]), len(filas),
        sum(int(f.split('\t')[5]) for f in filas)))
    if a.lista:
        for f in filas:
            print(f)
    if a.aplicar and filas:
        nl = '\r\n' if '\r\n' in tsv_txt else '\n'
        nuevo = tsv_txt
        if not nuevo.endswith(nl):
            nuevo += nl
        nuevo += '# procedencia_pc.py: nombres puestos por commits desde %s sin fila (regla git-<carril>, marca = commit)%s' % (
            a.desde, nl)
        nuevo += nl.join(filas) + nl
        escribe(os.path.join(RAIZ, TSV), nuevo.encode('utf-8'))
        print('escrito', TSV)


if __name__ == '__main__':
    main()
