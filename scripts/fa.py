#!/usr/bin/env python3
"""fa.py -- apaga una a una las guardas EA_BUILD_A124 del cuerpo de las funciones que SLUS ya casa y la A124 no.

Las guardas de la A124 se ponen con prueba en su asm, pero algunas heredadas (de upstream o de rondas viejas) son
falsas: la A124 tiene el mismo codigo que SLUS y la guarda la manda por una rama que no existe. Para cada funcion con
SLUS al 100 % y la A124 por debajo, esto busca su cuerpo en src/Speed/Indep/Src, cambia cada `#if/#ifdef/#ifndef
EA_BUILD_A124` (y, si son 2 o 3, todas a la vez) por un nombre que no esta definido, para que la A124 tome la rama de
SLUS, y mide la funcion en la A124 con banco.py. Lo que sube a 100 es candidato a quitar la guarda (despues hay que
medir las cinco versiones: la guarda puede estar haciendo falta en otra funcion del mismo fichero).

    python scripts/fa.py --lista          # solo las candidatas y sus guardas
    python scripts/fa.py [--hilos N]      # las prueba (no toca src/: las copias van a build/fa/)

Lee build/versiones/*.json (el ultimo informe). Ronda 40 (ps2casi40): 168 pruebas, 3 ganan (LoaderScenery 97,72 ->
100 entre ellas).
"""
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TMP = os.path.join(RAIZ, 'build', 'fa')


def funciones(version):
    """{simbolo: (porcentaje, tamano, unidad)} del informe de la version."""
    r = json.load(open(os.path.join(RAIZ, 'build', 'versiones', version + '.json'), encoding='utf-8'))
    o = {}
    for u in r['units']:
        for f in u.get('functions') or []:
            o[f['name']] = (f.get('fuzzy_match_percent'), int(f.get('size') or 0), u['name'].split('/')[-1])
    return o


def demangle(n):
    """(clase, metodo) de un nombre de GCC 2.9; muy simple, basta para buscar la definicion."""
    m = re.match(r'^(.+?)__(\d+)([A-Za-z_]\w*)', n)
    if m and len(m.group(3)) >= int(m.group(2)):
        cl = m.group(3)[:int(m.group(2))]
        meth = m.group(1)
        if meth.startswith('_$_'):
            return cl, '~' + cl
        if meth == '' or n.startswith('__' + m.group(2)):
            return cl, cl
        return cl, meth
    m = re.match(r'^__(\d+)([A-Za-z_]\w*)', n)
    if m:
        cl = m.group(2)[:int(m.group(1))]
        return cl, cl
    m = re.match(r'^(.+?)__Q(\d)(.*)', n)
    if m:
        rest = m.group(3)
        partes = []
        for _ in range(int(m.group(2))):
            mm = re.match(r'(\d+)', rest)
            if not mm:
                break
            n_ = int(mm.group(1))
            rest = rest[len(mm.group(1)):]
            partes.append(rest[:n_])
            rest = rest[n_:]
        return partes[-1] if partes else None, m.group(1)
    m = re.match(r'^(.+?)__F', n)
    if m:
        return None, m.group(1)
    return None, None


def lee_fuentes():
    fuera = {}
    for dp, _, fn in os.walk(os.path.join(RAIZ, 'src', 'Speed', 'Indep', 'Src')):
        for f in fn:
            if f.endswith(('.cpp', '.hpp', '.h')):
                p = os.path.join(dp, f)
                fuera[os.path.relpath(p, RAIZ).replace(os.sep, '/')] = open(
                    p, encoding='utf-8', errors='replace').read().splitlines()
    return fuera


def sourcelists():
    """{fichero incluido: [SourceLists que lo incluyen]}"""
    sl = {}
    d = os.path.join(RAIZ, 'src', 'Speed', 'Indep', 'SourceLists')
    for f in os.listdir(d):
        if f.endswith('.cpp'):
            texto = open(os.path.join(d, f), encoding='utf-8', errors='replace').read()
            for inc in re.findall(r'#include "(Speed/[^"]+\.cpp)"', texto):
                sl.setdefault(inc, []).append(f[:-4])
    return sl


def trabajos():
    a = funciones('SLES-53558-A124')
    s = funciones('SLUS-21351')
    cands = [(k, a[k][0] or 0.0, a[k][1], a[k][2]) for k in a
             if k in s and s[k][0] == 100.0 and (a[k][0] or 0) < 100.0]
    fuentes = lee_fuentes()
    sl = sourcelists()
    fuera = []
    for k, p, tam, unidad in sorted(cands, key=lambda x: -x[2]):
        cl, meth = demangle(k)
        if not meth:
            continue
        rx = re.compile((re.escape(cl) + r'::' if cl else r'(?<![:\w])') + re.escape(meth) + r'\s*\(')
        sitios = [(f, i) for f, lineas in fuentes.items() for i, l in enumerate(lineas)
                  if rx.search(l) and not l.rstrip().endswith(';')]
        for f, i0 in sitios[:2]:
            if not f.endswith('.cpp'):
                continue
            texto = fuentes[f]
            nivel, empezado, guardas = 0, False, []
            for i in range(i0, min(len(texto), i0 + 600)):
                if re.match(r'\s*#\s*(if|ifdef|ifndef|elif)\b.*EA_BUILD_A124', texto[i]):
                    guardas.append(i)
                nivel += texto[i].count('{') - texto[i].count('}')
                if '{' in texto[i]:
                    empezado = True
                if empezado and nivel <= 0:
                    break
            rel = f[len('src/'):]
            if guardas and unidad in sl.get(rel, []):
                fuera.append((k, p, tam, unidad, rel, guardas))
    return fuera


def prueba(arg):
    k, unidad, rel, combo, etiqueta = arg
    texto = open(os.path.join(RAIZ, 'src', rel), newline='', encoding='utf-8').read()
    nl = '\r\n' if '\r\n' in texto else '\n'
    lineas = texto.split(nl)
    for g in combo:
        lineas[g] = lineas[g].replace('EA_BUILD_A124', 'EA_BUILD_A124_FA')
    copia = os.path.join(TMP, etiqueta + '_' + os.path.basename(rel))
    open(copia, 'w', newline='', encoding='utf-8').write(nl.join(lineas))
    r = subprocess.run([sys.executable, os.path.join(RAIZ, 'scripts', 'banco.py'), 'SLES-53558-A124', unidad, rel,
                        copia, k], cwd=RAIZ, capture_output=True, text=True, errors='replace')
    os.remove(copia)
    res = [l for l in r.stdout.splitlines() if '%' in l and 'size=' in l]
    return k, combo, res[0][-40:] if res else 'ERROR'


def main():
    lista = trabajos()
    if '--lista' in sys.argv:
        for k, p, tam, unidad, rel, guardas in lista:
            print('%6.2f %5d %-12s %-50s %s %s' % (p, tam, unidad, k[:50], rel, [g + 1 for g in guardas]))
        return
    hilos = 8
    if '--hilos' in sys.argv:
        hilos = int(sys.argv[sys.argv.index('--hilos') + 1])
    os.makedirs(TMP, exist_ok=True)
    jobs = []
    for k, p, tam, unidad, rel, guardas in lista:
        combos = [(g,) for g in guardas]
        if 1 < len(guardas) <= 3:
            combos.append(tuple(guardas))
        for c in combos:
            jobs.append((k, unidad, rel, c, 'fa%d' % len(jobs)))
    with ThreadPoolExecutor(hilos) as ex:
        for k, c, res in ex.map(prueba, jobs):
            print('%-60s %s %s' % (k[:60], [x + 1 for x in c], res), flush=True)
    try:
        os.rmdir(TMP)
    except OSError:
        pass


if __name__ == '__main__':
    main()
