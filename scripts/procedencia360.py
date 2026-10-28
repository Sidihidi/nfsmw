#!/usr/bin/env python3
"""procedencia360.py [--fuente ETIQUETA=TSV ...] [--ver] -- el registro de procedencia de los nombres de Xbox 360
(config/EUROPEGERMILESTONE/nombres_procedencia.tsv), como el de PC (ronda 88, X360-A).

Una fila por funcion de .text con nombre en config/EUROPEGERMILESTONE/symbols.txt (las fn_ no): direccion, nombre,
tamano, fuente, detalle, commit y fecha. La fuente sale, por orden de prioridad:
  1. de los TSV de --fuente (ETIQUETA=RUTA; RUTA con `va<TAB>nombre[<TAB>...]`, cabecera opcional): la fila cuenta si
     la direccion y el nombre son los de hoy (si el nombre cambio despues, manda el commit que lo cambio);
  2. del commit que puso la linea (`git blame` de symbols.txt): `upstream-prostreet-vt` para ec590bfc1 (upstream,
     nombres de ProStreet por version tracking, ene-2026), `upstream` para los commits de otros autores,
     `reconciliacion-upstream` (922210cc9), `upstream-xbox_stuff` (0a3210fe2) y `carril` para los nuestros (el detalle
     lleva el asunto del commit); `sin-commit` para lo que aun no esta en git.
`git blame` da el ULTIMO commit que toco la linea: los troceados de las rondas 40-42 reescribieron lineas enteras, asi
que lo de upstream y lo de septiembre son minimos (docs/analisis/2026-10-23_procedencia_pc_x360.md, 2.2).

    python scripts/procedencia360.py --fuente xenon0742-via-r79=build/rescate_x360_0742/x36079/nombres_via79.tsv
"""
import argparse
import collections
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from escritura import escribe  # noqa: E402

SYM = 'config/EUROPEGERMILESTONE/symbols.txt'
SALIDA = 'config/EUROPEGERMILESTONE/nombres_procedencia.tsv'
FIJOS = {'ec590bfc1': 'upstream-prostreet-vt', '922210cc9': 'reconciliacion-upstream',
         '0a3210fe2': 'upstream-xbox_stuff'}
NUESTROS = {'jferr'}


def blame():
    """{numero de linea (1..): (commit corto, autor, fecha, asunto)}"""
    p = subprocess.run(['git', 'blame', '--line-porcelain', SYM], capture_output=True)
    if p.returncode:
        sys.exit('git blame fallo: %s' % p.stderr.decode('utf-8', 'replace')[-300:])
    info, out, cur, linea = {}, {}, None, 0
    for raw in p.stdout.split(b'\n'):
        t = raw.decode('utf-8', 'replace')
        m = re.match(r'^([0-9a-f]{40}) \d+ (\d+)', t)
        if m:
            cur, linea = m.group(1), int(m.group(2))
            info.setdefault(cur, {})
            continue
        if cur is None:
            continue
        if t.startswith('author '):
            info[cur]['autor'] = t[7:]
        elif t.startswith('author-time '):
            import time
            info[cur]['fecha'] = time.strftime('%Y-%m-%d', time.gmtime(int(t[12:])))
        elif t.startswith('summary '):
            info[cur]['asunto'] = t[8:]
        elif t.startswith('\t'):
            d = info[cur]
            out[linea] = (cur, d.get('autor', ''), d.get('fecha', ''), d.get('asunto', ''))
    return out


def fuentes(pares):
    """{(va, nombre): (etiqueta, detalle)} de los TSV de --fuente (el primero que da la pareja manda)."""
    r = {}
    for par in pares:
        etiq, ruta = par.split('=', 1)
        for l in open(ruta, encoding='utf-8', errors='replace'):
            c = l.rstrip('\n').split('\t')
            if len(c) < 2 or not re.match(r'^0x[0-9a-fA-F]+$', c[0]):
                continue
            det = ' '.join(x for x in c[2:] if x)[:200]
            r.setdefault((int(c[0], 16), c[1]), (etiq, det))
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--fuente', action='append', default=[])
    ap.add_argument('--ver', action='store_true')
    a = ap.parse_args()
    ext = fuentes(a.fuente)
    bl = blame()
    filas, cuenta, bytes_ = [], collections.Counter(), collections.Counter()
    for n, l in enumerate(open(SYM, encoding='utf-8').read().split('\n'), 1):
        m = re.match(r'^(\S+) = \.text:0x([0-9A-F]+);.*?size:0x([0-9A-F]+)', l)
        if not m or m.group(1).startswith('fn_'):
            continue
        nom, va, tam = m.group(1), int(m.group(2), 16), int(m.group(3), 16)
        commit, autor, fecha, asunto = bl.get(n, ('0' * 40, '', '', ''))
        sin = commit == '0' * 40 or autor == 'Not Committed Yet'
        corto = 'sin-commit' if sin else commit[:9]
        if (va, nom) in ext:
            fuente, det = ext[(va, nom)]
        elif sin:
            fuente, det = 'sin-commit', ''
        elif corto in FIJOS:
            fuente, det = FIJOS[corto], asunto[:120]
        elif autor not in NUESTROS:
            fuente, det = 'upstream', '%s: %s' % (autor, asunto[:100])
        else:
            fuente, det = 'carril', asunto[:120]
        filas.append('0x%08X\t%s\t%d\t%s\t%s\t%s\t%s' % (va, nom, tam, fuente, det.replace('\t', ' '), corto,
                                                         '' if sin else fecha))
        cuenta[fuente] += 1
        bytes_[fuente] += tam
    cab = ('# Procedencia de los nombres de funcion de Xbox 360 (symbols.txt); la escribe scripts/procedencia360.py.\n'
           '# direccion\tnombre\tbytes\tfuente\tdetalle\tcommit\tfecha\n')
    for f, c in cuenta.most_common():
        print('%-28s %6d %9d B' % (f, c, bytes_[f]))
    print('total %d' % len(filas))
    if not a.ver:
        escribe(SALIDA, cab + '\n'.join(filas) + '\n', encoding='utf-8', newline='\n')
        print('escrito', SALIDA)


if __name__ == '__main__':
    main()
