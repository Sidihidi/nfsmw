#!/usr/bin/env python3
"""casi360.py -- las CASI-EXACTAS de Xbox 360 en la medida OFICIAL (enlace LTCG), por unidad y por PATRON.

Como scripts/casi.py (GameCube/PS2), pero con los objetos de la medida de 360 (scripts/ltcg360.py):
build/EUROPEGERMILESTONE/ltcg/orig (el original troceado y normalizado) contra build/EUROPEGERMILESTONE/ltcg/obj
(nuestro enlace LTCG troceado), y el porcentaje de build/versiones/EUROPEGERMILESTONE.json. objdiff con las mismas
opciones que pctall (functionRelocDiffs=none, ppc.calculatePoolRelocations=false).

Cada fila distinta se resume en un PATRON:
  conm             mismo mnemonico y los dos operandos de fuente cambiados de orden (fmuls/fadds/or...: `a * b`
                   frente a `b * a`; MSVC los emite en el orden de la fuente)
  reg              mismo mnemonico, solo cambian registros (reparto)
  imm              mismo mnemonico, cambia un inmediato o un desplazamiento
  op A>B           otra instruccion en la misma fila (A del original, B la nuestra)
  +B / -A          una instruccion de mas (nuestra) o de menos (del original): suele arrastrar un desplazamiento
y la funcion se clasifica por su patron dominante. La tabla de patrones dice cuantas funciones y bytes tiene cada uno:
el patron comun de muchas casi exactas es el que merece una forma de fuente (o una regla del enlace LTCG).

    python scripts/casi360.py [--min 95] [--tam 0] [--unidad U] [--max-dif N] [--n 40] [--detalle N]
    python scripts/casi360.py --unidades            # solo la tabla por unidad (bytes al 95-99,99 %)
    --guardar deja los JSON de objdiff en build/EUROPEGERMILESTONE/ltcg/casi (medio GB para todas las unidades)
"""
import argparse
import collections
import json
import os
import re
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
V = 'EUROPEGERMILESTONE'
OBJDIFF = os.path.join(RAIZ, 'build', 'tools', 'objdiff-cli.exe')
BASE = os.path.join(RAIZ, 'build', V, 'ltcg')
REG = re.compile(r'\b(r\d+|f\d+|cr\d|v\d+)\b')
NUM = re.compile(r'-?0x[0-9a-fA-F]+|-?\b\d+\b')


def diff_unidad(corto, cache, guardar=False):
    rel = os.path.join('Speed', 'Indep', 'SourceLists', corto + '.obj')
    po, pn = os.path.join(BASE, 'orig', rel), os.path.join(BASE, 'obj', rel)
    if not (os.path.exists(po) and os.path.exists(pn)):
        return None
    salida = os.path.join(cache, corto + '.json')
    if os.path.exists(salida) and any(os.path.getmtime(o) > os.path.getmtime(salida) for o in (po, pn)):
        os.remove(salida)
    if not os.path.exists(salida):
        r = subprocess.run([OBJDIFF, 'diff', '-1', po, '-2', pn, '-c', 'functionRelocDiffs=none',
                            '-c', 'ppc.calculatePoolRelocations=false', '-o', salida, '--format', 'json'],
                           capture_output=True, text=True)
        if r.returncode != 0 or not os.path.exists(salida):
            return None
    try:
        with open(salida, encoding='utf-8') as fh:
            d = json.load(fh)
    except (OSError, ValueError):
        d = None
    if not guardar and os.path.exists(salida):
        os.remove(salida)  # medio GB entre todas las unidades: sin --guardar no se queda en disco
    return d


def texto(e):
    return ((e or {}).get('instruction') or {}).get('formatted', '').strip()


def patron(a, b):
    if not a:
        return '+' + b.split()[0]
    if not b:
        return '-' + a.split()[0]
    ma, mb = a.split()[0], b.split()[0]
    if ma != mb:
        return 'op %s>%s' % (ma, mb)
    if REG.sub('R', a) == REG.sub('R', b):
        oa = [x.strip() for x in a.split(None, 1)[1].split(',')] if ' ' in a else []
        ob = [x.strip() for x in b.split(None, 1)[1].split(',')] if ' ' in b else []
        if len(oa) >= 3 and oa[0] == ob[0] and oa[1:3] == ob[2:0:-1] and oa[3:] == ob[3:]:
            return 'conm'  # operandos conmutados (a*b frente a b*a): el orden de la fuente en MSVC
        return 'reg'
    if NUM.sub('N', a) == NUM.sub('N', b):
        return 'imm'
    return 'otro ' + ma


def clasifica(izq, der):
    n = max(len(izq), len(der))
    pats, detalle = collections.Counter(), []
    for k in range(n):
        a = izq[k] if k < len(izq) else None
        b = der[k] if k < len(der) else None
        ka = (a or {}).get('diff_kind') or ''
        kb = (b or {}).get('diff_kind') or ''
        if ka in ('', 'DIFF_NONE') and kb in ('', 'DIFF_NONE'):
            continue
        ta, tb = texto(a), texto(b)
        if not ta and not tb:
            continue
        pats[patron(ta, tb)] += 1
        detalle.append((k, ta, tb))
    return pats, detalle


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--min', type=float, default=95.0)
    p.add_argument('--tam', type=int, default=0)
    p.add_argument('--unidad', default=None)
    p.add_argument('--max-dif', type=int, default=1000000)
    p.add_argument('--n', type=int, default=40)
    p.add_argument('--detalle', type=int, default=0)
    p.add_argument('--unidades', action='store_true')
    p.add_argument('--tsv', default=None)
    p.add_argument('--guardar', action='store_true', help='deja los diff de objdiff en ltcg/casi')
    args = p.parse_args()

    rep = json.load(open(os.path.join(RAIZ, 'build', 'versiones', V + '.json'), encoding='utf-8'))
    quiero, por_u = {}, collections.Counter()
    nfu = collections.Counter()
    for u in rep.get('units', []):
        corto = u['name'].rsplit('/', 1)[-1]
        for f in u.get('functions', []) or []:
            pct = f.get('fuzzy_match_percent', 0.0)
            tam = int(f.get('size', 0) or 0)
            if args.min <= pct < 100.0 and tam >= args.tam:
                quiero.setdefault(corto, {})[f['name']] = (pct, tam)
                por_u[corto] += tam
                nfu[corto] += 1
    print('%-22s %6s %9s' % ('unidad', 'fn', 'bytes'))
    for corto, b in por_u.most_common(25):
        print('%-22s %6d %9d' % (corto, nfu[corto], b))
    print('%-22s %6d %9d  (al %.0f-99,99 %%)\n' % ('TOTAL', sum(nfu.values()), sum(por_u.values()), args.min))
    if args.unidades:
        return 0

    cache = os.path.join(BASE, 'casi')
    os.makedirs(cache, exist_ok=True)
    filas = []
    for corto in sorted(quiero):
        if args.unidad and args.unidad.lower() != corto.lower():
            continue
        d = diff_unidad(corto, cache, args.guardar)
        if not d:
            print('aviso: objdiff no pudo con %s' % corto, file=sys.stderr)
            continue
        izq = {s.get('name'): s for s in d['left']['symbols']}
        der = {s.get('name'): s for s in d['right']['symbols']}
        for sim, (pct, tam) in quiero[corto].items():
            a, b = izq.get(sim), der.get(sim)
            if not a or not b:
                continue
            pats, det = clasifica(a.get('instructions', []), b.get('instructions', []))
            n = sum(pats.values())
            if 0 < n <= args.max_dif:
                filas.append((n, tam, pct, corto, sim, pats, det))

    if not args.guardar and os.path.isdir(cache) and not os.listdir(cache):
        os.rmdir(cache)
    filas.sort(key=lambda f: (f[0], -f[1]))
    print('%5s %7s %7s  %-18s %s' % ('dif', 'bytes', '%', 'unidad', 'funcion'))
    for n, tam, pct, corto, sim, pats, det in filas[:args.n]:
        print('%5d %7d %7.2f  %-18s %s' % (n, tam, pct, corto, sim[:70]))
        print('%24s%s' % ('', ', '.join('%s x%d' % kv for kv in pats.most_common(4))))
        for k, ta, tb in det[:args.detalle]:
            print('%26d  %-38s | %s' % (k, ta[:38], tb[:38]))

    # patron dominante de cada funcion y patrones totales
    dom = collections.defaultdict(lambda: [0, 0])
    tot = collections.defaultdict(lambda: [0, 0, 0])
    for n, tam, pct, corto, sim, pats, det in filas:
        k = pats.most_common(1)[0][0]
        k = k if k in ('reg', 'imm', 'conm') else re.sub(r'^(op|otro) ', r'\1 ', k)
        dom[k][0] += 1
        dom[k][1] += tam
        for x, c in pats.items():
            tot[x][0] += 1
            tot[x][1] += tam
            tot[x][2] += c
    print('\nPatron dominante (funciones, bytes):')
    for k, (nf, b) in sorted(dom.items(), key=lambda kv: -kv[1][1])[:20]:
        print('  %-24s %5d %9d' % (k, nf, b))
    print('\nPatron presente (funciones, bytes de esas funciones, filas):')
    for k, (nf, b, c) in sorted(tot.items(), key=lambda kv: -kv[1][1])[:25]:
        print('  %-24s %5d %9d %7d' % (k, nf, b, c))
    print('\n%d casi exactas con <=%d filas distintas, %d bytes' % (len(filas), args.max_dif, sum(f[1] for f in filas)))
    if args.tsv:
        with open(args.tsv, 'w', encoding='utf-8', newline='\n') as fh:
            for n, tam, pct, corto, sim, pats, det in filas:
                fh.write('%s\t%s\t%d\t%d\t%.3f\t%s\n' % (corto, sim, n, tam, pct,
                                                       ';'.join('%s=%d' % kv for kv in pats.most_common())))
    return 0


if __name__ == '__main__':
    sys.exit(main())
