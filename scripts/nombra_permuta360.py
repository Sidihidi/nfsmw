#!/usr/bin/env python3
"""nombra_permuta360.py -- nombres CAMBIADOS entre funciones de una misma unidad de Xbox 360 (ronda 42).

Los nombres heredados de 360 (de PC, de upstream o de rondas viejas) estan a veces permutados entre funciones del mismo
tamano y la misma forma: `EngineRacer::GetInductionPSI` del original lee el campo 0x30 (mSpool, visto desde IInductable)
y `InductionSpool` lee 0x34 (mPSI); los getters de rueda de SuspensionSimple/Traffic/Trailer/Spline; el destructor de
bTList<RigidBody> que vacia TheSimpleBodies. objdiff empareja por nombre, asi que las dos salen al 99,5 % aunque las dos
esten escritas bien. nombra_grafo no lo arregla (solo nombra funciones sin nombre) y nombra_corrige360 solo mira los
llamantes.

Regla: en cada unidad, una funcion del original que NO casa con su nombre recibe el nombre B de una funcion nuestra si
  - su codigo es EXACTAMENTE el de B (palabra a palabra, con los campos reubicados a cero, ya normalizados pool y
    tablas: es lo que compara la medida), y B es la UNICA nuestra de la unidad con ese codigo;
  - los destinos de las reubicaciones con nombre real (no fn_/lbl_/ltcg_/__real@/cadenas) son los mismos en los dos
    lados (el destructor de la lista lleva su global);
  - B no lo lleva ya una funcion del original que case (en 360 un nombre no se repite dentro de una unidad), y ninguna
    otra funcion del original reclama B;
  - la funcion del original que hoy se llama B (si la hay) tambien recibe nombre nuevo por esta misma regla: nunca
    se deja un nombre repetido en la unidad.
  - CICLOS (A<->B, A->B->C->A: cada funcion del ciclo casa exactamente con el nombre de la siguiente) se aceptan de
    cualquier tamano: la prueba va en los dos sentidos. CADENAS abiertas (la ultima toma un nombre que la unidad del
    original no tenia) solo si el nombre nuevo es la MISMA funcion con otra decoracion: mismo metodo y misma clase
    (parametros long/int, struct/class, const, virtual, ??_G/??_E de la misma clase). Medido sobre las anclas: si se
    esconde la nuestra de verdad, en 663 de 12.000 hay OTRA nuestra unica con el mismo codigo (ICopMgr/ITrafficMgr,
    los constructores de mensajes, Attach/Detach): el codigo solo no prueba un nombre distinto.
Control (ronda 42): la vtable de EngineRacer en el original pone 0x82457A20 en el hueco de InductionSpool y 0x82457A18
en el de GetInductionPSI (IInductable.h), al reves que los nombres heredados; el codigo dice lo mismo.

Entradas: un banco LTCG conservado (scripts/trocea360.py banco ETIQ --conservar: DIR/b/ETIQ/{orig,obj} y
DIR/b/ETIQ.json) o, sin --banco, la medida oficial (build/<V>/ltcg/{orig,obj} y build/versiones/<V>.json).

    python scripts/nombra_permuta360.py [--banco=DIR/b/ETIQ] [--raiz=DIR] [--aplicar]
"""
import collections
import json
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import nombra_grafo as ng  # noqa: E402
from escritura import entero  # noqa: E402

V = 'EUROPEGERMILESTONE'
ANONIMO = re.compile(r'^(fn|lbl|sub|func)_[0-9A-F]{8}$|^ltcg_|^__real@|^\?\?_C@|^\$|^\.')


def opt(nombre, defecto=None):
    for a in sys.argv[1:]:
        if a.startswith('--%s=' % nombre):
            return a.split('=', 1)[1]
    return defecto


def misma_funcion(a, b):
    """Mismo metodo y misma clase con sus ambitos y argumentos de plantilla (todo lo de antes del primer `@@`): solo
    cambia la decoracion de la firma. Vector<unsigned>::end y Vector<ISndAttachable *>::end NO son la misma."""
    def clave(n):
        n = n.replace('??_G', '??_E', 1) if n.startswith('??_G') else n
        return n.split('@@', 1)[0] if '@@' in n else None
    return clave(a) is not None and clave(a) == clave(b)


def casadas(ruta):
    d = json.load(open(ruta, encoding='utf-8'))
    out = collections.defaultdict(dict)
    if 'units' in d:
        for u in d['units']:
            for f in u.get('functions', []):
                out[u['name'].split('/')[-1]][f['name']] = f.get('fuzzy_match_percent')
    else:
        for u, fs in d.items():
            for f, (p, t) in fs.items():
                out[u][f] = p
    return out


def main():
    x = ng.X360.__new__(ng.X360)
    banco = opt('banco')
    raiz = os.path.abspath(opt('raiz', RAIZ))
    if banco:
        orig_d, obj_d, pct = os.path.join(banco, 'orig'), os.path.join(banco, 'obj'), casadas(banco + '.json')
    else:
        orig_d = os.path.join(RAIZ, 'build', V, 'ltcg', 'orig')
        obj_d = os.path.join(RAIZ, 'build', V, 'ltcg', 'obj')
        pct = casadas(os.path.join(RAIZ, 'build', 'versiones', V + '.json'))
    sims = os.path.join(raiz, 'config', V, 'symbols.txt')
    lineas = open(sims, encoding='utf-8').read().split('\n')
    rangos = collections.defaultdict(list)
    u = None
    for l in open(os.path.join(raiz, 'config', V, 'splits.txt'), encoding='utf-8'):
        m = re.match(r'^(\S.*)\.cpp:', l)
        if m:
            u = m.group(1).split('/')[-1]
            continue
        m = re.match(r'\s+\.text\s+start:0x(\w+) end:0x(\w+)', l)
        if m and u:
            rangos[u].append((int(m.group(1), 16), int(m.group(2), 16)))
    linea_de = collections.defaultdict(list)   # nombre -> [(dir, indice de linea)]
    for i, l in enumerate(lineas):
        m = re.match(r'^(\S+) = \.text:0x([0-9A-Fa-f]+); // type:function', l)
        if m:
            linea_de[m.group(1)].append((int(m.group(2), 16), i))

    def destinos(rel):
        return {k: r[1] for k, r in rel.items() if len(r) > 1 and not ANONIMO.match(r[1] or '')}
    cambios = []   # (unidad, dir, viejo, nuevo, indice)
    tipo = collections.Counter()
    for un in sorted(pct):
        po = os.path.join(orig_d, 'Speed', 'Indep', 'SourceLists', un + '.obj')
        pn = os.path.join(obj_d, 'Speed', 'Indep', 'SourceLists', un + '.obj')
        if not (os.path.exists(po) and os.path.exists(pn)):
            continue
        fo, fn = ng.Coff(po).funciones(), ng.Coff(pn).funciones()
        por_esq = collections.defaultdict(list)
        for n, (pal, rel) in fn.items():
            por_esq[x.esqueleto(pal, rel)].append(n)
        ok = {n for n, p in pct[un].items() if p == 100.0}
        prop, fuerte = {}, {}
        for o, (pal, rel) in fo.items():
            if o in ok or o not in pct[un] or ANONIMO.match(o):
                continue
            cand = [n for n in por_esq.get(x.esqueleto(pal, rel), []) if n != o]
            if len(cand) != 1 or cand[0] in ok:
                continue
            b = cand[0]
            do, dn = destinos(rel), destinos(fn[b][1])
            if any(do[k] != dn[k] for k in do.keys() & dn.keys()):
                continue
            prop[o] = b
            fuerte[o] = misma_funcion(o, b)
        dup = collections.Counter(prop.values())
        prop = {o: b for o, b in prop.items() if dup[b] == 1}

        def cierra():   # si B ya es el nombre de una funcion del original, esa tambien tiene que cambiar
            cambio = True
            while cambio:
                cambio = False
                for o, b in list(prop.items()):
                    if b in fo and b not in prop:
                        del prop[o]
                        cambio = True
        cierra()

        def en_ciclo(o):
            v, k = prop.get(o), 0
            while v is not None and v != o and k < len(prop):
                v, k = prop.get(v), k + 1
            return v == o
        ciclo = {o for o in prop if en_ciclo(o)}
        for o in list(prop):
            if o not in ciclo and not fuerte[o]:
                del prop[o]
        cierra()
        for o, b in prop.items():
            tipo['ciclo' if o in ciclo else 'cadena'] += 1
            sitios = [(a, i) for a, i in linea_de.get(o, []) if any(r0 <= a < r1 for r0, r1 in rangos[un])]
            if len(sitios) != 1:
                continue
            cambios.append((un, sitios[0][0], o, b, sitios[0][1]))
    for un, a, o, b, i in sorted(cambios):
        print('%-18s 0x%08X %-60s -> %s' % (un, a, o[:60], b[:90]))
    print('permutaciones: %d nombres %s' % (len(cambios), dict(tipo)))
    if '--aplicar' in sys.argv and cambios:
        for un, a, o, b, i in cambios:
            assert lineas[i].startswith(o + ' = .text:'), lineas[i]
            lineas[i] = b + lineas[i][len(o):]
        entero(sims, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
        print('aplicados %d' % len(cambios))
    return 0


if __name__ == '__main__':
    sys.exit(main())
