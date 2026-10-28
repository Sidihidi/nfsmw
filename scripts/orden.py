#!/usr/bin/env python3
"""orden.py VERSION SALIDA.tsv [--kmax=N] -- nombra funciones sin nombre de MW (X360 o PC) por ORDEN DE EMISION
dentro de un fichero fuente, usando las lineas del DWARF de Carbon PS3.

Para cada hueco de MW entre dos funciones con nombre consecutivas A y B (k funciones sin nombre entre ellas, k<=kmax)
cuyos homonimos en Carbon estan definidos en el MISMO fichero .cpp con linea(A) < linea(B), los candidatos son las
funciones de Carbon de ese fichero con linea entre las dos y cuyo nombre MW (esa version) no tiene. Si hay
exactamente k, se proponen en orden.

Validacion (sobre las funciones de MW que YA tienen nombre):
  * presente: se esconde una funcion X con nombre cuyas vecinas son anclas del mismo fichero; acierto si el unico
    candidato es X;
  * ausente: igual, pero quitando ademas X de los candidatos de Carbon (la funcion de MW no esta en Carbon o Carbon
    la tiene con otro nombre): toda propuesta es un error. Con las dos tasas se estima la precision esperada en las
    sin nombre (como en build/scratch_prostreet/parecido.py).
Pasada a scripts/ en la ronda 78 (procedencia: docs/analisis/2026-10-21_carbonps3.md); sin rutas de la maquina.
"""
import bisect
import collections
import json
import os
import re
import sys

from carbon import carga_dw, nombres_mw, NOMBRE_AUTO, RAIZ, lee_symbols_txt

if __name__ == '__main__' and (len(sys.argv) < 3 or sys.argv[1] in ('-h', '--help')):
    print(__doc__)
    sys.exit(0)


def funciones_mw(ver):
    """[(va, nombre, cualificado o None)] ordenadas por direccion, con y sin nombre."""
    cfg = os.path.join(RAIZ, 'config')
    nm = {t[0]: t[3] for t in nombres_mw()[ver]}
    out = []
    if ver in ('X360', 'GC'):
        for n, va, sz in lee_symbols_txt(os.path.join(cfg, 'EUROPEGERMILESTONE' if ver == 'X360' else 'GOWE69', 'symbols.txt')):
            out.append((va, n, nm.get(n) if not NOMBRE_AUTO.match(n) else None))
    else:
        for f in json.load(open(os.path.join(cfg, 'SPEED_EXE_1_3', 'symbols.json'), encoding='utf-8'))['functions']:
            n = f['name']
            out.append((f['start'], n, nm.get(n) if not NOMBRE_AUTO.match(n) else None))
    out.sort()
    return out


def indice_carbon(d):
    """{cualificado: [(fichero, linea)]} de las definiciones en .cpp/.c, y {fichero: [(linea, cualificado)]}."""
    porq = collections.defaultdict(set)
    porf = collections.defaultdict(set)
    for f in d['funcs']:
        if not f['q'] or not f['file'] or not f['line']:
            continue
        fi = f['file'].replace(chr(92), '/').lower()
        if not re.search(r'\.(cpp|c)$', fi):
            continue
        porq[f['q']].add((fi, f['line']))
        porf[fi].add((f['line'], f['q']))
    return porq, {k: sorted(v) for k, v in porf.items()}


def huecos(fns, porq, porf, nombrados, kmax, quitar=None, solo=None):
    """Genera (i_A, i_B, fichero, [indices sin nombre], [candidatos]) para los huecos resolubles."""
    anclas = [i for i, t in enumerate(fns) if t[2] and t[2] in porq and len(porq[t[2]]) == 1]
    for a, b in zip(anclas, anclas[1:]):
        k = b - a - 1
        if k < 1 or k > kmax:
            continue
        if any(fns[j][2] for j in range(a + 1, b)) and solo is None:
            continue
        (fa, la), = porq[fns[a][2]]
        (fb, lb), = porq[fns[b][2]]
        if fa != fb or la >= lb:
            continue
        cand = []
        for ln, q in porf[fa]:
            if la < ln < lb and q not in nombrados and q != quitar and q not in cand:
                cand.append(q)
        yield a, b, fa, list(range(a + 1, b)), cand


def main():
    ver, salida = sys.argv[1], sys.argv[2]
    kmax = 3
    extra = {}
    for x in sys.argv[3:]:
        if x.startswith('--kmax='):
            kmax = int(x[7:])
        if x.startswith('--extra='):
            # nombres ya aceptados (TSV con va y nombre_carbon): pasan a ser anclas de la siguiente vuelta
            import csv
            for r in csv.DictReader(open(x[8:], encoding='utf-8'), delimiter='	'):
                extra[int(r['va'], 16)] = r['nombre_carbon']
    d = carga_dw()
    porq, porf = indice_carbon(d)
    fns = [(va, n, extra.get(va, q)) for va, n, q in funciones_mw(ver)]
    nombrados = {t[2] for t in fns if t[2]}
    sin = sum(1 for t in fns if not t[2])
    print('%s: %d funciones, %d sin nombre; anclas con definicion unica en un .cpp de Carbon: %d' % (
        ver, len(fns), sin, sum(1 for t in fns if t[2] and t[2] in porq and len(porq[t[2]]) == 1)))
    # validacion: esconder cada funcion con nombre que tenga anclas a los dos lados
    pres_ok = pres_mal = pres_n = aus_prop = 0
    for i in range(1, len(fns) - 1):
        x = fns[i][2]
        if not x or x not in porq or len(porq[x]) != 1:
            continue
        a, b = fns[i - 1][2], fns[i + 1][2]
        if not (a and b and a in porq and b in porq and len(porq[a]) == 1 and len(porq[b]) == 1):
            continue
        (fa, la), = porq[a]
        (fb, lb), = porq[b]
        (fx, lx), = porq[x]
        if not (fa == fb == fx and la < lx < lb):
            # la verdadera no cae entre sus vecinas (otro fichero u otro orden): cuenta como caso del hueco igual
            pass
        if fa != fb or la >= lb:
            continue
        pres_n += 1
        cand = [q for ln, q in porf[fa] if la < ln < lb and (q not in nombrados or q == x)]
        cand = list(dict.fromkeys(cand))
        if len(cand) == 1:
            if cand[0] == x:
                pres_ok += 1
            else:
                pres_mal += 1
        cand2 = [q for q in cand if q != x]
        if len(cand2) == 1:
            aus_prop += 1
    print('validacion PRESENTE: %d huecos de 1 con anclas del mismo fichero; propuestas %d: %d bien, %d mal (precision %.1f %%; aceptacion %.1f %%)' % (
        pres_n, pres_ok + pres_mal, pres_ok, pres_mal, 100.0 * pres_ok / max(pres_ok + pres_mal, 1),
        100.0 * (pres_ok + pres_mal) / max(pres_n, 1)))
    print('validacion AUSENTE: con la verdadera fuera de Carbon, propuestas (todas falsas): %d de %d (%.1f %%)' % (
        aus_prop, pres_n, 100.0 * aus_prop / max(pres_n, 1)))
    a1 = (pres_ok + pres_mal) / max(pres_n, 1)
    p1 = pres_ok / max(pres_ok + pres_mal, 1)
    a0 = aus_prop / max(pres_n, 1)
    # objetivo
    props = []
    nh = collections.Counter()
    for a, b, fa, idx, cand in huecos(fns, porq, porf, nombrados, kmax):
        nh[len(idx)] += 1
        if len(cand) == len(idx):
            for j, q in zip(idx, cand):
                props.append((fns[j][0], fns[j][1], q, fa.split('/speed/')[-1], fns[a][1], fns[b][1], len(idx)))
    tot_h = sum(nh.values())
    acc = len({(p[4], p[5]) for p in props}) / max(tot_h, 1)
    print('objetivo: %d huecos sin nombre (k<=%d) entre anclas del mismo fichero %s; resueltos (k candidatos): %d huecos, %d funciones' % (
        tot_h, kmax, dict(sorted(nh.items())), len({(p[4], p[5]) for p in props}), len(props)))
    # estimacion de la fraccion presente pi con acc = pi*a1 + (1-pi)*a0 (aproximacion con huecos de 1)
    h1 = [p for p in props if p[6] == 1]
    acc1 = len(h1) / max(nh[1], 1)
    if a1 > a0:
        pi = min(max((acc1 - a0) / (a1 - a0), 0.0), 1.0)
        prec = pi * a1 * p1 / max(pi * a1 + (1 - pi) * a0, 1e-9)
        print('huecos de 1: aceptacion %.1f %% -> fraccion presente estimada %.2f, PRECISION ESPERADA %.1f %%' % (
            100 * acc1, pi, 100 * prec))
    with open(salida, 'w', encoding='utf-8', newline='\n') as f:
        f.write('va\tnombre_mw\tpropuesta_carbon\tfichero\tancla_antes\tancla_despues\tk\n')
        for p in props:
            f.write('0x%08X\t%s\t%s\t%s\t%s\t%s\t%d\n' % p)


if __name__ == '__main__':
    main()
