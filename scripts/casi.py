#!/usr/bin/env python3
"""Triaje de las CASI-EXACTAS: cuantas instrucciones le faltan a cada una.

Una funcion al 99,9 % aporta cero bytes igual que una al 0 %: `matched_code' es
todo o nada.  Pero la que falla por dos instrucciones vale su tamano integro y
se arregla en una tarde, y la que falla por doscientas no.  Esto las separa.

El histograma por funcion (histofn.py) es CIEGO para estas: no difieren en la
mezcla de instrucciones sino en los operandos, en el orden o en un registro.
Hace falta el diff instruccion a instruccion, y objdiff da TODAS las funciones
de una unidad en una sola llamada, asi que sale barato.

  python scripts/casi.py SLES-53558-A124
  python scripts/casi.py SLES-53558-A124 --unidad zPhysics --max-dif 6
  python scripts/casi.py GOWE69 --min 95
"""
import argparse
import json
import os
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJDIFF = os.path.join(RAIZ, 'objdiff-cli-windows-x86_64.exe')


def unidades(version):
    """[(nombre, ruta relativa)] de las unidades con los dos objetos."""
    base = os.path.join(RAIZ, 'build', version)
    fuera = []
    for raiz, _, ficheros in os.walk(os.path.join(base, 'obj')):
        for f in ficheros:
            if not f.endswith('.o'):
                continue
            orig = os.path.join(raiz, f)
            nuestro = orig.replace(os.path.join(base, 'obj'),
                                   os.path.join(base, 'src'), 1)
            if os.path.exists(nuestro):
                rel = os.path.relpath(orig, os.path.join(base, 'obj'))
                fuera.append((os.path.splitext(f)[0],
                              os.path.splitext(rel)[0].replace(os.sep, '/')))
    return sorted(fuera)


def diff_unidad(version, rel, cache):
    """El JSON de objdiff con TODAS las funciones de la unidad."""
    salida = os.path.join(cache, 'u_' + rel.replace('/', '_') + '.json')
    base = os.path.join(RAIZ, 'build', version)
    objetos = [os.path.join(base, 'obj', rel + '.o'), os.path.join(base, 'src', rel + '.o')]
    # La cache solo vale si es mas nueva que los DOS objetos: sin esto servia diffs de ayer
    # y daba por abiertas funciones ya casadas (2026-09-22).
    if os.path.exists(salida) and any(os.path.getmtime(o) > os.path.getmtime(salida)
                                      for o in objetos if os.path.exists(o)):
        os.remove(salida)
    if not os.path.exists(salida):
        r = subprocess.run(
            [OBJDIFF, 'diff',
             '-1', os.path.join(base, 'obj', rel + '.o'),
             '-2', os.path.join(base, 'src', rel + '.o'),
             '-c', 'function_reloc_diffs=none',
             '-c', 'ppc.calculatePoolRelocations=false',
             '-o', salida, '--format', 'json'],
            capture_output=True, text=True)
        if r.returncode != 0 or not os.path.exists(salida):
            return None
    try:
        with open(salida, encoding='utf-8') as fh:
            return json.load(fh)
    except (OSError, ValueError):
        return None


def texto(e):
    return ((e or {}).get('instruction') or {}).get('formatted', '').strip()


def clasifica(izq, der):
    """(filas distintas, clase dominante, [(k, original, nuestro)])."""
    n = max(len(izq), len(der))
    filas, clases, detalle = 0, {}, []
    for k in range(n):
        a = izq[k] if k < len(izq) else None
        b = der[k] if k < len(der) else None
        ka = (a or {}).get('diff_kind') or ''
        kb = (b or {}).get('diff_kind') or ''
        if ka in ('', 'DIFF_NONE') and kb in ('', 'DIFF_NONE'):
            continue
        filas += 1
        etq = (ka or kb).replace('DIFF_', '')
        clases[etq] = clases.get(etq, 0) + 1
        detalle.append((k, texto(a), texto(b)))
    if not clases:
        return 0, '', []
    dom = max(clases.items(), key=lambda kv: kv[1])
    return filas, '%s x%d' % (dom[0].lower(), dom[1]), detalle


def main():
    p = argparse.ArgumentParser()
    p.add_argument('version')
    p.add_argument('--unidad', default=None)
    p.add_argument('--min', type=float, default=97.0)
    p.add_argument('--tam', type=int, default=200, help='tamano minimo')
    p.add_argument('--max-dif', type=int, default=12,
                   help='solo las que fallan por menos de tantas filas')
    p.add_argument('--n', type=int, default=40)
    p.add_argument('--detalle', type=int, default=0,
                   help='imprime hasta N filas distintas de cada una')
    args = p.parse_args()

    cache = os.path.join(RAIZ, 'build', args.version, 'casi')
    os.makedirs(cache, exist_ok=True)

    # El informe da el porcentaje; objdiff, las instrucciones.
    ruta = os.path.join(RAIZ, 'build', args.version, 'report.json')
    with open(ruta, encoding='utf-8') as fh:
        rep = json.load(fh)
    quiero = {}
    for u in rep.get('units', []):
        for f in u.get('functions', []) or []:
            pct = f.get('fuzzy_match_percent', 0.0)
            tam = int(f.get('size', 0) or 0)
            if args.min <= pct < 99.999 and tam >= args.tam:
                quiero.setdefault(u['name'].split('/')[-1], {})[f['name']] = (pct, tam)

    filas = []
    for nombre, rel in unidades(args.version):
        if args.unidad and args.unidad.lower() not in nombre.lower():
            continue
        if nombre not in quiero:
            continue
        d = diff_unidad(args.version, rel, cache)
        if not d:
            print('aviso: objdiff no pudo con %s' % rel, file=sys.stderr)
            continue
        izq = dict((s.get('name'), s) for s in d['left']['symbols'])
        der = dict((s.get('name'), s) for s in d['right']['symbols'])
        for sim, (pct, tam) in quiero[nombre].items():
            a, b = izq.get(sim), der.get(sim)
            if not a or not b:
                continue
            n, dom, det = clasifica(a.get('instructions', []),
                                    b.get('instructions', []))
            if 0 < n <= args.max_dif:
                filas.append((n, tam, pct, nombre, sim, dom, det))

    filas.sort(key=lambda f: (f[0], -f[1]))
    print('%5s %8s %8s  %-18s %s' % ('dif', 'bytes', '%', 'unidad', 'funcion'))
    for n, tam, pct, unidad, sim, dom, det in filas[:args.n]:
        print('%5d %8d %8.2f  %-18s %s' % (n, tam, pct, unidad, sim[:56]))
        if args.detalle:
            for k, izq_t, der_t in det[:args.detalle]:
                print('%*s%5d  %-40s | %s' % (10, '', k, izq_t[:40], der_t[:40]))
        else:
            print('%*s%s' % (24, '', dom))
    print('\n%d casi-exactas fallan por <=%d filas, %d bytes en total'
          % (len(filas), args.max_dif, sum(f[1] for f in filas)))


if __name__ == '__main__':
    main()
