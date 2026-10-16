#!/usr/bin/env python3
"""orden360.py -- nombres de Xbox 360 por el ORDEN de emision dentro de la unidad (ronda 54).

LTCG decide el orden de las funciones con el programa entero y no lo reproducimos en grande (ltcg360.py), pero en
pequeno si: entre dos funciones que se llaman igual en el original y en lo nuestro, las que salen en medio suelen ser
las mismas y en el mismo orden. En zWorld, entre `EndOfList` y `Remove@bTNode` de bTList<ParameterAccessor>, el
original tiene [fn_ (24 B), AddHead, AddTail, Remove, fn_ (4), fn_ (4), fn_ (24)] y lo nuestro [GetHead, AddHead,
AddTail, Remove, ??0bTNode, ??1bTNode, GetNext]: el mismo codigo hueco a hueco. Ninguna otra regla nombra esas
funciones: GetHead y GetNext tienen el MISMO codigo (`lwz; cmplwi; addi -4; bnelr; li 0; blr`), un constructor vacio
es un `blr` como otros cien, y nadie las llama con `bl`.

Regla, unidad a unidad (original: direcciones de symbols.txt dentro de su rango de splits.txt; nuestro: el orden del
objeto troceado de nuestro enlace LTCG, que es el orden del .map):
  1. Anclas: nombres que estan una sola vez en cada lado; se quedan las de la subsecuencia creciente mas larga (las
     que estan en el mismo orden relativo en los dos lados).
  2. Entre dos anclas consecutivas, el hueco del original y el nuestro tienen que tener el MISMO numero de funciones y
     el MISMO codigo hueco a hueco (esqueleto de nombra_grafo: campos reubicados a cero). Si no, el hueco no se toca.
  3. En un hueco que cuadra, cada funcion del original cuyo nombre no es el nuestro recibe el nuestro si: la del
     original no casa hoy al 100 % con su nombre, el nuestro no es anonimo (fn_/ltcg_/cadenas) y el nombre nuevo no lo
     lleva otra funcion de la unidad del original que se quede con el (si la otra tambien cambia en esta pasada, vale).
  4. GEMELAS: si dentro del hueco dos de las nuestras tienen el mismo codigo, ninguna de las dos se nombra (el orden
     entre gemelas no es fiable). Control (--validar=0|1|2 esconde un tercio de las anclas y mira si el hueco las
     acierta), 2026-10-16: sin gemelas 3.487 bien y 0 mal; las gemelas fallaban 4 de 30 (los Handler/MemberHandler
     de zEAXSound, GetModel const/no const), por eso van fuera salvo --gemelas.
  4b. BORDES: si el hueco no cuadra entero (otro numero de funciones u otro codigo en medio), valen las funciones
     pegadas a cada ancla mientras el codigo siga siendo igual (prefijo y sufijo). Control con las anclas escondidas:
     unas 700 mas por tercio, todas bien (0 mal). --sin-bordes lo apaga.
  5. Anclas: por defecto, funciones al 100 % con su nombre; --anclas-nombre acepta las que solo coinciden en el
     nombre. --min-hueco N: solo huecos de al menos N funciones.

Entradas: la medida oficial (build/<V>/ltcg/{orig,obj} y build/versiones/<V>.json), como nombra_permuta360.

    python scripts/orden360.py [--unidad=U] [--detalle] [--anclas-nombre] [--gemelas] [--sin-bordes] [--validar=K] [--tsv=F] [--aplicar]
"""
import bisect
import collections
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import nombra_grafo as ng  # noqa: E402
from nombra_permuta360 import casadas, ANONIMO  # noqa: E402
from escritura import entero  # noqa: E402

V = 'EUROPEGERMILESTONE'


def opt(nombre, defecto=None):
    for a in sys.argv[1:]:
        if a.startswith('--%s=' % nombre):
            return a.split('=', 1)[1]
    return defecto


def lis(pares):
    """pares [(i_original, j_nuestro)] ordenados por i: la subsecuencia de j estrictamente creciente mas larga."""
    if not pares:
        return []
    colas, idx, prev = [], [], [None] * len(pares)
    for k, (_, j) in enumerate(pares):
        p = bisect.bisect_left(colas, j)
        if p == len(colas):
            colas.append(j)
            idx.append(k)
        else:
            colas[p] = j
            idx[p] = k
        prev[k] = idx[p - 1] if p > 0 else None
    out, k = [], idx[-1]
    while k is not None:
        out.append(pares[k])
        k = prev[k]
    return out[::-1]


def main():
    x = ng.X360.__new__(ng.X360)
    orig_d = os.path.join(RAIZ, 'build', V, 'ltcg', 'orig', 'Speed', 'Indep', 'SourceLists')
    obj_d = os.path.join(RAIZ, 'build', V, 'ltcg', 'obj', 'Speed', 'Indep', 'SourceLists')
    pct = casadas(os.path.join(RAIZ, 'build', 'versiones', V + '.json'))
    sims = os.path.join(RAIZ, 'config', V, 'symbols.txt')
    lineas = open(sims, encoding='utf-8').read().split('\n')
    rangos = collections.defaultdict(list)
    u = None
    for l in open(os.path.join(RAIZ, 'config', V, 'splits.txt'), encoding='utf-8'):
        m = re.match(r'^(\S.*)\.cpp:', l)
        if m:
            u = m.group(1).split('/')[-1]
            continue
        m = re.match(r'\s+\.text\s+start:0x(\w+) end:0x(\w+)', l)
        if m and u:
            rangos[u].append((int(m.group(1), 16), int(m.group(2), 16)))
    texto = []   # (dir, nombre, indice de linea)
    for i, l in enumerate(lineas):
        m = re.match(r'^(\S+) = \.text:0x([0-9A-Fa-f]+); // type:function', l)
        if m:
            texto.append((int(m.group(2), 16), m.group(1), i))
    texto.sort()
    solo = opt('unidad')
    anclas_nombre = '--anclas-nombre' in sys.argv
    min_hueco = int(opt('min-hueco', '1'))
    validar = int(opt('validar')) if opt('validar') is not None else None
    unico = '--gemelas' not in sys.argv
    bordes = '--sin-bordes' not in sys.argv
    cambios = []
    est = collections.Counter()
    for un in sorted(pct):
        if solo and un != solo:
            continue
        po, pn = os.path.join(orig_d, un + '.obj'), os.path.join(obj_d, un + '.obj')
        if not (os.path.exists(po) and os.path.exists(pn) and un in rangos):
            continue
        fo, fn = ng.Coff(po).funciones(), ng.Coff(pn).funciones()
        O = [(a, n, i) for a, n, i in texto if any(a0 <= a < a1 for a0, a1 in rangos[un]) and n in fo]
        N = list(fn.keys())
        co = collections.Counter(n for _, n, _ in O)
        cn = collections.Counter(N)
        pos_n = {n: j for j, n in enumerate(N)}
        cand = []
        for i, (a, n, _) in enumerate(O):
            if co[n] == 1 and cn.get(n) == 1 and not ANONIMO.match(n):
                if anclas_nombre or pct[un].get(n) == 100.0:
                    cand.append((i, pos_n[n]))
        anc = lis(cand)
        if validar is not None:   # control: se esconden las anclas k % 3 == validar y se mira si el hueco las acierta
            escondidas = {anc[k][0] for k in range(len(anc)) if k % 3 == validar}
            anc = [p for p in anc if p[0] not in escondidas]
        est['anclas'] += len(anc)
        nombre_en_o = collections.defaultdict(list)
        for i, (a, n, _) in enumerate(O):
            nombre_en_o[n].append(i)
        prop = {}   # i del original -> nombre nuevo
        for (i0, j0), (i1, j1) in zip(anc, anc[1:]):
            go, gn = O[i0 + 1:i1], N[j0 + 1:j1]
            if not go or not gn or len(go) < min_hueco:
                continue
            igual = [x.esqueleto(*fo[n]) == x.esqueleto(*fn[m]) for (_, n, _), m in zip(go, gn)]
            if len(go) == len(gn) and all(igual):
                est['huecos que cuadran'] += 1
                pares = list(range(len(go)))
                parejas = [(k, k) for k in pares]
            elif bordes:   # huecos distintos: solo lo pegado a cada ancla mientras el codigo sea igual
                est['huecos con bordes'] += 1
                pre = 0
                while pre < min(len(go), len(gn)) and x.esqueleto(*fo[go[pre][1]]) == x.esqueleto(*fn[gn[pre]]):
                    pre += 1
                suf = 0
                while (suf < min(len(go), len(gn)) - pre
                       and x.esqueleto(*fo[go[-1 - suf][1]]) == x.esqueleto(*fn[gn[-1 - suf]])):
                    suf += 1
                parejas = [(k, k) for k in range(pre)] + [(len(go) - 1 - k, len(gn) - 1 - k) for k in range(suf)]
            else:
                est['huecos distintos' if len(go) != len(gn) else 'huecos con otro codigo'] += 1
                continue
            esq = [x.esqueleto(*fn[m]) for m in gn]
            repes = collections.Counter(esq)
            for ko, kn in parejas:
                a, n, idx = go[ko]
                m = gn[kn]
                if unico and repes[esq[kn]] > 1:   # gemelas de codigo en el mismo hueco
                    continue
                if validar is not None:
                    if i0 + 1 + ko in escondidas:
                        est['control bien' if n == m else 'control MAL'] += 1
                        if n != m and '--detalle' in sys.argv:
                            print('   MAL %s 0x%08X %s <- %s' % (un, a, n[:60], m[:60]))
                    continue
                if n == m or ANONIMO.match(m) or pct[un].get(n) == 100.0:
                    continue
                if '--detalle' in sys.argv:
                    print('   %s 0x%08X %4d B %-50s -> %s' % (un, a, 4 * len(fo[n][0]), n[:50], m[:70]))
                prop[i0 + 1 + ko] = m
        # el nombre nuevo no puede quedar repetido en la unidad
        cambio = True
        while cambio:
            cambio = False
            nuevos = collections.Counter(prop.values())
            for i, m in list(prop.items()):
                otros = [k for k in nombre_en_o.get(m, []) if k != i and k not in prop]
                if nuevos[m] > 1 or otros:
                    del prop[i]
                    cambio = True
        for i, m in sorted(prop.items()):
            a, n, idx = O[i]
            cambios.append((un, a, n, m, idx))
    for un, a, n, m, idx in cambios:
        print('%-18s 0x%08X %-55s -> %s' % (un, a, n[:55], m[:90]))
    print('estadistica: %s' % dict(est))
    print('por orden: %d nombres' % len(cambios))
    if opt('tsv'):
        with open(opt('tsv'), 'w', encoding='utf-8') as fh:
            for un, a, n, m, idx in cambios:
                fh.write('%08X\t%s\t%s\t%s\n' % (a, n, m, un))
    if '--aplicar' in sys.argv and cambios:
        for un, a, n, m, idx in cambios:
            assert lineas[idx].startswith(n + ' = .text:'), lineas[idx]
            lineas[idx] = m + lineas[idx][len(n):]
        entero(sims, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
        print('aplicados %d' % len(cambios))
    return 0


if __name__ == '__main__':
    sys.exit(main())
