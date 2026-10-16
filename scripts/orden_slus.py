#!/usr/bin/env python3
"""Compara, unidad a unidad, el ORDEN de las funciones de SLUS-21351 con el de la A124 (cuyos nombres salen de la
.symtab) para cazar nombres heredados que cayeron en la funcion de al lado y funciones de SLUS sin nombre.

Los nombres de SLUS se heredaron de la A124 por direccion y por codigo. Donde varias funciones tienen el mismo codigo
(destructores de clases hermanas: solo cambia la vtable, y la reubicacion no se compara) el nombre puede caer en la
hermana: en zFe2 los destructores del HUD estaban corridos una clase (TurboMeter se llamaba MenuZoneTrigger, y el
MenuZoneTrigger de verdad era un func_), y en zPhysicsBehaviors los de Draw*Car. Ninguna comparacion de codigo lo ve:
el orden si.

Metodo, por unidad: anclas = nombres de SLUS que estan una sola vez en la unidad de la A124; se queda la subsecuencia
creciente mas larga (las anclas fuera de orden se informan). Entre dos anclas seguidas se cuenta cuantas funciones hay
en cada version; si la diferencia no es cero pero la suma vuelve a cero en un ancla posterior, el tramo tiene el mismo
numero de funciones en las dos y se empareja 1:1 de punta a punta: las anclas intermedias que no caen donde dice el
1:1 son nombres corridos, y las func_ del tramo reciben el nombre de la A124 de su posicion. Tambien se emparejan 1:1
los huecos iguales entre dos anclas.

NINGUNA propuesta es prueba por si sola (la A124 es un prototipo: puede tener una funcion de mas y otra de menos en
el mismo tramo). --prueba compara cada propuesta con nuestro objeto de SLUS palabra a palabra (como
scripts/prueba_nombre.py: en las reubicadas, codigo de operacion y registros; en j/jal, el nombre del destino si los
dos lo tienen); solo las IGUAL se vuelcan con --tsv, que acepta scripts/aplica_nombres.py. La ultima columna dice si
el codigo SOLO ya prueba el nombre (`unica`: una sola funcion nuestra de la unidad tiene ese codigo) o si hay N con el
mismo (`ambiguaN`: accesores, _IHandle, destructores de interfaz...), y entonces la unica prueba es el orden.

Resultado del 2026-09-23 (tanda 4): 809 IGUAL; se aplicaron las 174 `unica` (+158 funciones casadas en SLUS, 10,6 kB)
y 13 ambiguas que ademas confirma el grafo de llamadas (scripts/grafo_nombres.py). Las otras 622 (13,7 kB, casi todas
de 8 a 20 bytes) quedaron como propuesta: solo las respalda el orden.

    python scripts/orden_slus.py [UNIDAD...] [--prueba] [--tsv propuestas.tsv] [--todas]
    python scripts/orden_slus.py zFe2 --prueba
    python scripts/orden_slus.py --prueba --tsv orden.tsv            # todas las unidades comunes

--todas imprime tambien las propuestas que no son IGUAL. Lee build/<V>/asm de las dos versiones y, con --prueba,
build/SLUS-21351/obj y build/SLUS-21351/src: tienen que ser de un troceado y una compilacion recientes.
"""
import bisect
import collections
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))

A, S = 'SLES-53558-A124', 'SLUS-21351'
SIN = re.compile(r'^(func|fn|sub|lbl)_[0-9A-Fa-f]+$')
MAX_TRAMO = 60          # funciones como mucho en un tramo 1:1 entre anclas que no cuadran
RE_INS = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s')


def lee_asm(V):
    """{unidad: [(ini, fin, nombre)]} del troceado de la version (build/<V>/asm)"""
    base = os.path.join(RAIZ, 'build', V, 'asm')
    out = collections.defaultdict(list)
    for dp, dn, fn in os.walk(base):
        for f in fn:
            if not f.endswith('.s'):
                continue
            p = os.path.join(dp, f)
            unidad = os.path.relpath(p, base).replace(os.sep, '/')[:-2]
            cur = None
            for l in open(p, encoding='utf-8', errors='replace'):
                m = re.match(r'\s*glabel (\S+)', l)
                if m:
                    cur = [None, None, m.group(1)]
                    out[unidad].append(cur)
                    continue
                m = RE_INS.match(l)
                if m and cur is not None:
                    v = int(m.group(1), 16)
                    if cur[0] is None:
                        cur[0] = v
                    cur[1] = v + 4
                    continue
                if re.match(r'\s*endlabel', l):
                    cur = None
    return {u: sorted(tuple(x) for x in lst if x[0] is not None) for u, lst in out.items()}


def lis(pares):
    """subsecuencia creciente mas larga (en el primer elemento) de pares ya ordenados por el segundo"""
    idx, prev, colas = [], [None] * len(pares), []
    for k, (i, j) in enumerate(pares):
        p = bisect.bisect_left(colas, i)
        if p == len(idx):
            idx.append(k)
            colas.append(i)
        else:
            idx[p] = k
            colas[p] = i
        prev[k] = idx[p - 1] if p > 0 else None
    k = idx[-1] if idx else None
    out = []
    while k is not None:
        out.append(pares[k])
        k = prev[k]
    return out[::-1]


def analiza(fa, fs):
    """-> (propuestas {j: (nombre, motivo)}, anclas fuera de orden [(j, nombre)], huecos desiguales)"""
    nom_a = collections.defaultdict(list)
    for i, f in enumerate(fa):
        nom_a[f[2]].append(i)
    cand = [(nom_a[f[2]][0], j) for j, f in enumerate(fs) if not SIN.match(f[2]) and len(nom_a.get(f[2], [])) == 1]
    anc = lis(cand)
    en_orden = set(anc)
    fuera = [(j, fs[j][2]) for i, j in cand if (i, j) not in en_orden]
    prop = {}
    desiguales = 0
    bordes = [(-1, -1)] + anc + [(len(fa), len(fs))]
    k = 0
    while k < len(bordes) - 1:
        (i0, j0), (i1, j1) = bordes[k], bordes[k + 1]
        if (j1 - j0) == (i1 - i0):
            for d in range(1, i1 - i0):
                j = j0 + d
                prop.setdefault(j, (fa[i0 + d][2], 'hueco' if SIN.match(fs[j][2]) else 'distinto'))
            k += 1
            continue
        desiguales += 1
        # buscar un ancla posterior (cerca) donde el total vuelva a cuadrar: tramo 1:1 de punta a punta, que solo se
        # acepta si los tamanos casan (todos los renombrados y el 80 % del tramo): si no, es una diferencia real
        # entre versiones (la A124 tiene funciones de red, de depuracion... que SLUS no).
        for m in range(k + 2, min(len(bordes), k + 8)):
            (ib, jb) = bordes[m]
            if (jb - j0) != (ib - i0):
                continue
            n = ib - i0 - 1
            if n <= 0 or n > MAX_TRAMO:
                break
            tam = [fa[i0 + d][1] - fa[i0 + d][0] == fs[j0 + d][1] - fs[j0 + d][0] for d in range(1, n + 1)]
            ren = [d for d in range(1, n + 1) if fs[j0 + d][2] != fa[i0 + d][2]]
            if sum(tam) < 0.8 * n or not all(tam[d - 1] for d in ren):
                break
            for d in ren:
                j = j0 + d
                motivo = 'corrido' if not SIN.match(fs[j][2]) else 'tramo'
                prop[j] = (fa[i0 + d][2], motivo)
            k = m
            break
        else:
            k += 1
            continue
        if bordes[k] == (i0, j0):
            k += 1
    # solo las que cambian algo
    prop = {j: v for j, v in prop.items() if fs[j][2] != v[0]}
    return prop, fuera, desiguales


def main():
    args = [a for i, a in enumerate(sys.argv[1:], 1) if not a.startswith('--') and sys.argv[i - 1] != '--tsv']
    prueba = '--prueba' in sys.argv
    todas = '--todas' in sys.argv
    ua, us = lee_asm(A), lee_asm(S)
    unidades = sorted(set(ua) & set(us))
    if args:
        unidades = [u for u in unidades if any(u.endswith('/' + x) or u.endswith('/' + x + '.cpp') or u == x
                                               for x in args)]
    if prueba:
        import prueba_nombre as PN
    nombres_s = collections.Counter(f[2] for lst in us.values() for f in lst)
    filas = []
    tot = collections.Counter()
    for u in unidades:
        fa, fs = ua[u], us[u]
        prop, fuera, desig = analiza(fa, fs)
        if not prop and not fuera:
            continue
        t = o = None
        if prueba:
            base = u[:-4] if u.endswith('.cpp') else (u[:-2] if u.endswith('.c') else u)
            pt = os.path.join(RAIZ, 'build', S, 'obj', base + '.o')
            po = os.path.join(RAIZ, 'build', S, 'src', base + '.o')
            t = PN.funciones(pt) if os.path.exists(pt) else {}
            o = PN.funciones(po) if os.path.exists(po) else {}
        cab = False
        por_tam = collections.defaultdict(list)
        if prueba:
            for nn, lst in o.items():
                for x in lst:
                    por_tam[len(x[0])].append((nn, x))
        for j, (n, motivo) in sorted(prop.items()):
            actual = fs[j][2]
            est = ''
            unica = ''
            if prueba:
                if actual in t and n in o and len(t[actual]) == 1:
                    est = PN.compara(t[actual][0], o[n][0])[0]
                    if est == 'IGUAL':
                        # cuantas funciones NUESTRAS de la unidad tienen ese mismo codigo: si es una, el codigo
                        # solo ya prueba el nombre; si son varias, la prueba es el orden
                        iguales = {nn for nn, x in por_tam[len(t[actual][0][0])]
                                   if PN.compara(t[actual][0], x)[0] == 'IGUAL'}
                        unica = 'unica' if len(iguales) == 1 else 'ambigua%d' % len(iguales)
                else:
                    est = 'no esta en el nuestro' if n not in o else 'sin comparar'
            tot[motivo] += 1
            tot['IGUAL'] += est == 'IGUAL'
            tot[unica or 'x'] += bool(unica)
            if est == 'IGUAL' or todas or not prueba:
                if not cab:
                    print('== %s: %d propuestas, %d anclas fuera de orden, %d huecos desiguales' % (
                        u, len(prop), len(fuera), desig))
                    cab = True
                print('  0x%06x %5x %-40s -> %-50s %-8s %s %s%s' % (fs[j][0], fs[j][1] - fs[j][0], actual[:40],
                                                                  n[:50], motivo, est, unica,
                                                                  '  (nombre repetido en SLUS)'
                                                                  if nombres_s.get(n) else ''))
            if est == 'IGUAL':
                filas.append((fs[j][0], n, motivo, u, unica))
        if todas:
            for j, n in fuera[:10]:
                print('  fuera de orden: 0x%06x %s' % (fs[j][0], n[:70]))
    print('propuestas: %s' % dict(tot))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for a, n, motivo, u, unica in filas:
                fh.write('0x%x\t%s\t%s\t%s\t%s\n' % (a, n, motivo, u, unica))
        print('volcadas %d (solo IGUAL)' % len(filas))


if __name__ == '__main__':
    main()
