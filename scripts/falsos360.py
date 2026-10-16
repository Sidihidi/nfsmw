#!/usr/bin/env python3
"""falsos360.py -- nombres FALSOS del original de Xbox 360, por CODIGO (ronda 52, X52; pasado a scripts/ en la 53).

Un nombre X del original (no fn_) es falso cuando su esqueleto (codigo con los campos reubicados a cero) no es el de
nuestra X de la unidad, pero si el de UNA sola funcion nuestra Y de la misma unidad que no casa con nada del original,
y el original no tiene otra funcion sin casar con ese esqueleto en la unidad. Se le pone el nombre Y.
  - Con --varias, entre varias Y candidatas se queda con la unica que tiene destinos (reubicaciones con nombre) que la
    confirman y ninguno en contra, y ninguna rival del original puntua igual o mejor.
  - Y no puede quedar repetido en la unidad del original: si ya lo lleva otra funcion, esa tambien tiene que cambiar
    (punto fijo); si no, la propuesta cae. --diag lista las que caen y por que.
  - Minimo --min bytes (16 por defecto): los getters de 8 B tienen el mismo codigo en clases sin relacion.
  - --estricto tira las que tengan algun destino en contra.
Ejemplos de la ronda 52: el `SetCarAnimationPositions` de X360 (0x150 B) es `InitCarAnimStatesFromNIS`;
`?GetTick@Sim` era `WorldBodyConn::FetchData`; `NotifySkyUnloader` era `DebugWorld::Service`.

Usa nuestros objetos del enlace LTCG (build/EUROPEGERMILESTONE/ltcg/obj, los que mide el informe) y el original
troceado (build/EUROPEGERMILESTONE/obj). Tras aplicar: la rutina de nombres de X360 y medir.

    python scripts/falsos360.py [--varias] [--min B] [--estricto] [--diag] [--tsv F] [--aplicar]
"""
import collections
import glob
import os
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if not os.path.isdir(os.path.join(RAIZ, 'config', 'EUROPEGERMILESTONE')):
    RAIZ = os.environ.get('NFSMW_RAIZ', os.getcwd())  # copia de trabajo fuera del arbol: la raiz es el directorio actual
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import nombra_grafo as ng  # noqa: E402
from escritura import entero  # noqa: E402

V = 'EUROPEGERMILESTONE'


def main():
    a = sys.argv[1:]
    minimo = int(a[a.index('--min') + 1]) if '--min' in a else 16
    P = ng.X360()
    base_o = os.path.join(RAIZ, 'build', V, 'obj')
    base_s = os.path.join(RAIZ, 'build', V, 'ltcg', 'obj')
    prop = {}
    presentes = {}
    for p in sorted(glob.glob(os.path.join(base_s, '**', '*.obj'), recursive=True)):
        rel = os.path.relpath(p, base_s)
        o = os.path.join(base_o, rel)
        if not os.path.exists(o):
            continue
        un = rel.replace(os.sep, '/')[:-4]
        fo = ng.Coff(o).funciones()
        fs = ng.Coff(p).funciones()
        eo = {n: P.esqueleto(*v) for n, v in fo.items()}
        es = {n: P.esqueleto(*v) for n, v in fs.items()}
        presentes[un] = set(fo)
        casadas_o = {n for n in eo if es.get(n) == eo[n]}
        por_esq_s = collections.defaultdict(list)
        for n, e in es.items():
            if n not in casadas_o:
                por_esq_s[e].append(n)
        por_esq_o = collections.defaultdict(list)
        for n, e in eo.items():
            if n not in casadas_o:
                por_esq_o[e].append(n)

        def puntua(n, y):
            ac = co = 0
            mot = []
            po, ro = fo[n]
            ps, rs = fs[y]
            for i, (ts, nsn) in rs.items():
                if i not in ro:
                    continue
                to, no = ro[i]
                if ng.equivalentes(no, nsn):
                    ac += 1
                    continue
                if P.dir_de(no) is not None or no.startswith(('lbl_', 'ltcg_')) or not P.valido(nsn) \
                        or not P.valido(no):
                    continue
                v = P.dato_con_nombre(no, nsn, ts, po, ps, i)
                if v == 'nada':
                    continue
                if v is True:
                    ac += 1
                    continue
                co += 1
                mot.append('%s|%s' % (no[:50], nsn[:50]))
            return ac, co, mot

        for n, e in eo.items():
            if n in casadas_o or P.es_funcion_sin_nombre(n) or n.startswith('auto_'):
                continue
            if len(fo[n][0]) * 4 < minimo:
                continue
            ds = [x for x in P.por_nombre.get(n, []) if P.unidad_de(x) == un and x in P.texto]
            if len(ds) != 1:
                continue
            ys = [y for y in por_esq_s.get(e, []) if y != n and len(y) <= ng.MAX_NOMBRE]
            if not ys:
                continue
            unico = len(ys) == 1 and len(por_esq_o[e]) == 1
            if not unico and '--varias' not in a:
                continue
            if not unico:
                pun = {y: puntua(n, y) for y in ys}
                buenos = sorted(((v[0], y) for y, v in pun.items() if v[1] == 0), reverse=True)
                if not buenos or buenos[0][0] == 0 or (len(buenos) > 1 and buenos[1][0] == buenos[0][0]):
                    continue
                y = buenos[0][1]
                rivales = [m for m in por_esq_o[e] if m != n and (P.es_funcion_sin_nombre(m) or m not in es or
                                                                  es[m] != e)]
                if any(o[1] == 0 and o[0] >= buenos[0][0] for o in (puntua(m, y) for m in rivales)):
                    continue
                ys = [y]
            y = ys[0]
            ac, co, mot = puntua(n, y)
            prop[(un, n)] = (y, ds[0], len(fo[n][0]) * 4, ac, co, mot)
    # punto fijo: Y libre en la unidad del original, o su duena tambien cambia
    acept = dict(prop)
    caidas = {}
    while True:
        salen = set(acept)
        malos = {}
        for k, (y, ad, t, ac, co, mot) in acept.items():
            if y in presentes[k[0]] and (k[0], y) not in salen:
                malos[k] = 'el nombre nuevo ya lo lleva otra funcion de la unidad'
        cnt = collections.Counter((k[0], v[0]) for k, v in acept.items())
        for k, v in acept.items():
            if cnt[(k[0], v[0])] > 1:
                malos[k] = 'dos van al mismo nombre'
        if not malos:
            break
        for k, m in malos.items():
            acept.pop(k, None)
            caidas[k] = m
    if '--estricto' in a:
        for k in [k for k, v in acept.items() if v[4]]:
            acept.pop(k)
            caidas[k] = 'destino en contra (--estricto)'
    print('propuestas %d, aceptadas %d (%d B)' % (len(prop), len(acept), sum(v[2] for v in acept.values())))
    out = sorted((v[1], k[1], v[0], k[0], v[2]) for k, v in acept.items())
    for ad, x, y, u, t in out:
        v = acept[(u, x)]
        print('%08X %4d %-12s %-60s -> %-70s +%d -%d %s' % (ad, t, u.split('/')[-1], x[:60], y[:70], v[3], v[4],
                                                           ' ; '.join(v[5][:2])))
    if '--diag' in a:
        for k, m in sorted(caidas.items(), key=lambda kv: prop[kv[0]][1]):
            v = prop[k]
            print('CAE %08X %4d %-12s %-55s -> %-55s %s' % (v[1], v[2], k[0].split('/')[-1], k[1][:55], v[0][:55], m))
    if '--tsv' in a:
        with open(a[a.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as f:
            for ad, x, y, u, t in out:
                f.write('%08X\t%s\t%s\t%s\t%d\n' % (ad, x, y, u, t))
    if '--aplicar' in a and out:
        lin = P.lineas
        for ad, x, y, u, t in out:
            i = [e for e in P.por_dir[ad] if e[0] == x and e[1] == '.text'][0][2]
            assert lin[i].startswith(x + ' = .text:0x%08X;' % ad), lin[i]
            lin[i] = y + lin[i][len(x):]
        entero(os.path.join(RAIZ, 'config', V, 'symbols.txt'), 'w', encoding='utf-8', newline='\n').write('\n'.join(lin))
        print('aplicado', len(out))


if __name__ == '__main__':
    main()
