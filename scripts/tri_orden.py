#!/usr/bin/env python3
"""tri_orden.py ORDEN_X360.tsv NUESTRO_LTCG.map NUESTRO_LTCG.exe SALIDA.tsv -- contrasta las propuestas de orden.py
para X360 con dos pruebas independientes del orden:

  * existe_en: en que otras versiones de MW tiene ya ese nombre cualificado una funcion (GC, SLUS, A124, PC): prueba
    de que la funcion EXISTE fuera de linea en MW (la trampa de ProStreet era el nombre que MW no tiene);
  * parecido_nuestro: si nuestra fuente define ese nombre (nuestro enlace LTCG), SequenceMatcher entre el esqueleto
    (ppcsk) de la fn_ de MW y el de nuestra funcion; control: el mismo parecido contra la propuesta de OTRA fila
    del mismo tamano aproximado (si el control da lo mismo, el parecido no discrimina).
Pasada a scripts/ en la ronda 78 (procedencia: docs/analisis/2026-10-21_carbonps3.md); sin rutas de la maquina.
"""
import collections
import csv
import difflib
import os
import random
import sys

from carbon import RAIZ, nombres_mw, undname, lee_symbols_txt

from ppcsk import Exe, esqueleto  # noqa: E402
from cruce_nombres import lee_mapa  # noqa: E402

if __name__ == '__main__' and (len(sys.argv) < 5 or sys.argv[1] in ('-h', '--help')):
    print(__doc__)
    sys.exit(0)


def palabras(sk):
    return [sk[i:i + 4] for i in range(0, len(sk), 4)]


def main():
    tsv, nmap, nexe, salida = sys.argv[1:5]
    mw = Exe(os.path.join(RAIZ, 'orig', 'EUROPEGERMILESTONE', 'NFS.exe'))
    nu_exe = Exe(nexe, base_ejec=0x400000)
    nu = lee_mapa(nmap)
    vs = sorted(nu)
    und = undname([nu[v][0] for v in vs])
    nu_q = collections.defaultdict(list)
    for i, va in enumerate(vs):
        n = nu[va][0]
        sz = min((vs[i + 1] if i + 1 < len(vs) else va + 4) - va, 0x4000)
        nu_q[und.get(n, n)].append((va, sz, n))
    mwl = sorted((va, sz) for n, va, sz in lee_symbols_txt(os.path.join(RAIZ, 'config', 'EUROPEGERMILESTONE', 'symbols.txt')))
    tam = dict(mwl)
    orden = [va for va, sz in mwl]
    pos = {va: i for i, va in enumerate(orden)}
    mwn = nombres_mw()
    existe = collections.defaultdict(set)
    for ver in ('GC', 'SLUS', 'A124', 'PC'):
        for t in mwn[ver]:
            if t[3]:
                existe[t[3]].add(ver)
    filas = list(csv.DictReader(open(tsv, encoding='utf-8'), delimiter='\t'))

    def sk_mw(va):
        return palabras(esqueleto(mw, va, tam[va])[0])

    def sk_nu(q):
        if q not in nu_q or len(nu_q[q]) != 1:
            return None
        va, sz, n = nu_q[q][0]
        sk = esqueleto(nu_exe, va, sz)[0]
        while sk.endswith(bytes(4)):
            sk = sk[:-4]
        return palabras(sk)

    random.seed(7)
    est = collections.Counter()
    out = []
    nuestros = [r for r in filas if sk_nu(r['propuesta_carbon'])]
    for r in filas:
        va = int(r['va'], 16)
        q = r['propuesta_carbon']
        ex = ','.join(sorted(existe.get(q, ())))
        a = sk_mw(va)
        b = sk_nu(q)
        par = ctl = None
        if b:
            par = difflib.SequenceMatcher(None, a, b, autojunk=False).ratio()
            otros = [o for o in nuestros if o is not r and 0.7 <= len(sk_nu(o['propuesta_carbon'])) / max(len(a), 1) <= 1.4]
            if otros:
                o = random.choice(otros)
                ctl = difflib.SequenceMatcher(None, a, sk_nu(o['propuesta_carbon']), autojunk=False).ratio()
        tri = ''
        if b and par is not None:
            i = pos[va]
            vec = [v for v in orden[max(0, i - 30):i + 31] if 0.75 * len(b) <= tam[v] // 4 <= 1.25 * len(b)]
            punt = sorted(((difflib.SequenceMatcher(None, b, sk_mw(v), autojunk=False).ratio(), v) for v in vec),
                          reverse=True)[:2]
            if punt:
                seg = punt[1][0] if len(punt) > 1 else 0.0
                tri = 'si' if punt[0][1] == va and punt[0][0] - seg >= 0.05 else 'no'
                est['triangulada_' + tri] += 1
            # control: el nombre de OTRA fila (de tamano parecido) en esta misma fn_
            otros = [o for o in nuestros if o is not r and 0.75 * len(a) <= len(sk_nu(o['propuesta_carbon'])) <= 1.25 * len(a)]
            if otros:
                bo = sk_nu(random.choice(otros)['propuesta_carbon'])
                vec = [v for v in orden[max(0, i - 30):i + 31] if 0.75 * len(bo) <= tam[v] // 4 <= 1.25 * len(bo)]
                punt = sorted(((difflib.SequenceMatcher(None, bo, sk_mw(v), autojunk=False).ratio(), v) for v in vec),
                              reverse=True)[:2]
                if punt:
                    seg = punt[1][0] if len(punt) > 1 else 0.0
                    est['control_triangulada_' + ('si' if punt[0][1] == va and punt[0][0] - seg >= 0.05 else 'no')] += 1
        est['total'] += 1
        est['existe_en_mw' if ex else 'no_existe_en_otra_version'] += 1
        if par is not None:
            est['con_nuestro'] += 1
            est['parecido>=0.7' if par >= 0.7 else 'parecido<0.7'] += 1
            if ctl is not None:
                est['control>=0.7' if ctl >= 0.7 else 'control<0.7'] += 1
        out.append((r['va'], r['nombre_mw'], q, r['k'], str(tam[va]), ex,
                    '' if par is None else '%.3f' % par, '' if ctl is None else '%.3f' % ctl, tri, r['fichero']))
    for k in sorted(est):
        print('  %-28s %5d' % (k, est[k]))
    acept = [o for o in out if o[5] and o[6] and float(o[6]) >= 0.7]
    print('  existe en otra version Y parecido nuestro >= 0.7: %d funciones, %d B' % (len(acept), sum(int(o[4]) for o in acept)))
    acept = [o for o in out if o[5] and o[8] == 'si']
    print('  existe en otra version Y triangulada (la mejor de +-30 vecinas, margen 0.05): %d funciones, %d B' % (
        len(acept), sum(int(o[4]) for o in acept)))
    acept = [o for o in out if o[8] == 'si']
    print('  triangulada (con o sin otra version): %d funciones, %d B' % (len(acept), sum(int(o[4]) for o in acept)))
    with open(salida, 'w', encoding='utf-8', newline='\n') as f:
        f.write('va\tnombre_mw\tpropuesta_carbon\tk\ttam\texiste_en\tparecido_nuestro\tcontrol\ttriangulada\tfichero\n')
        for o in out:
            f.write('\t'.join(o) + '\n')


if __name__ == '__main__':
    main()
