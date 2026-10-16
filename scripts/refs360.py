#!/usr/bin/env python3
"""refs360.py -- nombres de Xbox 360 por las REFERENCIAS de las anclas, tambien entre unidades (ronda 55).

El hueco de rota360.py: alli el nombre nuevo tiene que estar definido en NUESTRO objeto de la MISMA unidad que la
direccion del original, y solo cuentan las llamadas. En 360 hay copias de inline y plantillas que el enlace LTCG deja
en otra unidad (??4bMatrix4 en zAI, Sim::IsSplitScreen en zEAXSound) y funciones que se referencian como DATO (los
_IHandle de las interfaces COM, que se cargan con lis/addi y se registran con Add@_IList). Las gemelas (mismo codigo,
otro tipo: Find<EngineParams>/Find<RBSimpleParams>, los ctor de IDamageable/ICarAudio/INISCarControl, los _IHandle)
casan al 100 % con el nombre cambiado; destinos.py las da como OTRA.

Evidencia: cada ANCLA (funcion al 100 % en la medida oficial con el mismo tamano en build/V/ltcg/orig y ltcg/obj)
referencia, en su palabra k, al simbolo NA del original y al NB nuestro (llamada b/bl o par lis/addi o palabra).
Si NA y NB son funciones distintas (no alias ??_G/??_E ni la misma con otra letra de acceso), es un voto "la direccion
de NA se llama NB"; si son iguales, es un voto por el nombre actual. Se renombra la direccion T de NA -> NB si:
  - todos los votos sobre T dicen NB (ninguno confirma el nombre actual ni dice otro) y hay al menos --min;
  - NB existe en NUESTRO enlace (cualquier unidad) y su codigo es el del original en T (palabra a palabra, sin los
    campos que reubica el enlace);
  - NB no queda repetido: si ya lo lleva otra direccion T2, T2 tiene que cambiar en la misma pasada por esta regla
    (permutaciones y cadenas); si no, la propuesta cae y se lista aparte.
No cambia la medida (las dos casaban o la direccion no la tenia nadie); corrige la verdad de symbols.txt, que es lo
que leen destinos.py y los ports. Pasarlo despues de medir.

    python scripts/refs360.py [--min N] [--fn [--min-fn M]] [--aplicar] [--todas]
      --todas   lista tambien las propuestas que caen (y por que)
      --validar control: en las direcciones que ya casan, los votos unanimes dicen su nombre?
      --fn      ademas nombra las fn_ que los llamantes nombran con M votos o mas (2 por defecto) aunque el codigo
                aun no sea el nuestro: funciones SIN CASAR; el nombre no puede estar ya en symbols.txt
"""
import collections
import json
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if not os.path.isdir(os.path.join(RAIZ, 'config', 'EUROPEGERMILESTONE')):
    RAIZ = os.environ.get('NFSMW_RAIZ', os.getcwd())  # copia de trabajo fuera del arbol: la raiz es el directorio actual
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import nombra_grafo as ng  # noqa: E402

V = 'EUROPEGERMILESTONE'
SIMS = os.path.join(RAIZ, 'config', V, 'symbols.txt')
FN, MIN_FN = False, 2
UNIDADES = {}   # (lado, nombre) -> unidades cuyo objeto lo define (LTCG deja copias de un inline en varias)
ANONIMO = re.compile(r'^(fn|lbl|sub|func)_[0-9A-F]{8}$|^ltcg_|^__real@|^\?\?_C@|^\$|^\.|^__(save|rest)(gpr|fpr|vmx)')
RX = re.compile(r'^(\S+) = \.text:0x([0-9A-Fa-f]+);')


def indice(lado):
    out = {}
    base = os.path.join(RAIZ, 'build', V, 'ltcg', lado)
    for raiz, _, fs in os.walk(base):
        for f in fs:
            if not f.endswith('.obj'):
                continue
            try:
                fun = ng.Coff(os.path.join(raiz, f)).funciones()
            except Exception:
                continue
            for n, (c, r) in fun.items():
                out[n] = (f[:-4], tuple(c), {k: v[1] for k, v in r.items()})
                UNIDADES.setdefault((lado, n), set()).add(f[:-4])
    return out


def canon(e):
    """Las palabras sin lo que reubica el enlace: el desplazamiento de b/bl y la mitad baja de lis/addi/lwz."""
    _, c, r = e
    out = []
    for i, w in enumerate(c):
        if (w >> 26) == 18:
            w &= 0xFC000003
        elif i in r:
            w &= 0xFFFF0000
        out.append(w)
    return tuple(out)


def alias(a, b):
    if ng.equivalentes(a, b):
        return True
    return a.replace('??_G', '??_E', 1) == b.replace('??_G', '??_E', 1)


def main():
    global FN, MIN_FN
    aplicar = '--aplicar' in sys.argv
    FN = '--fn' in sys.argv
    MIN_FN = int(sys.argv[sys.argv.index('--min-fn') + 1]) if '--min-fn' in sys.argv else 2
    minimo = int(sys.argv[sys.argv.index('--min') + 1]) if '--min' in sys.argv else 1
    lineas = open(SIMS, encoding='utf-8').read().split('\n')
    dir_de, nom_de = {}, {}
    for x in lineas:
        m = RX.match(x)
        if m:
            a = int(m.group(2), 16)
            dir_de.setdefault(m.group(1), a)
            nom_de[a] = m.group(1)
    rep = json.load(open(os.path.join(RAIZ, 'build', 'versiones', V + '.json'), encoding='utf-8'))
    anclas = set(f['name'] for u in rep['units'] for f in u.get('functions', []) or []
                 if f.get('fuzzy_match_percent') == 100.0)
    O, N = indice('orig'), indice('obj')
    votos = collections.defaultdict(collections.Counter)   # direccion -> {nombre: votos}
    quien = collections.defaultdict(list)
    for f in anclas:
        if f not in O or f not in N or len(O[f][1]) != len(N[f][1]):
            continue
        ro, rn = O[f][2], N[f][2]
        for k in set(ro) & set(rn):
            na, nb = ro[k], rn[k]
            if na == f or nb == f or na not in dir_de or ANONIMO.match(nb) or na.startswith('??_C@'):
                continue
            if nb not in N:   # solo funciones nuestras (los datos no estan en N)
                continue
            t = dir_de[na]
            if alias(na, nb):
                votos[t][nom_de[t]] += 1
            else:
                votos[t][nb] += 1
                quien[t].append('%s+0x%x' % (f[:50], 4 * k))
    if '--validar' in sys.argv:
        # control de la regla de --fn: en las direcciones cuyo nombre ya casa al 100 % (verdad conocida), cuantas veces
        # los votos unanimes con MIN_FN o mas dicen ese nombre y cuantas otro
        bien = mal = 0
        for t, c in votos.items():
            if nom_de[t] in anclas and len(c) == 1 and sum(c.values()) >= MIN_FN:
                if nom_de[t] in c:
                    bien += 1
                else:
                    mal += 1
                    print('  validar: %08X %s <- %s' % (t, nom_de[t][:60], list(c)[0][:60]))
        print('validar (%d votos o mas): %d bien, %d mal' % (MIN_FN, bien, mal))
    prop, caidas, sin_casar = {}, [], set()
    for t, c in votos.items():
        actual = nom_de[t]
        nuevos = [n for n in c if n != actual]
        if not nuevos:
            continue
        if len(c) != 1:
            caidas.append((t, actual, dict(c), 'votos repartidos'))
            continue
        nb = nuevos[0]
        if c[nb] < minimo:
            caidas.append((t, actual, dict(c), 'menos de --min'))
            continue
        if actual in anclas and O.get(actual, ('?',))[0] not in UNIDADES.get(('obj', nb), set()):
            # la direccion ya casa en la medida con su nombre: solo se cambia si el nombre nuevo tambien esta en NUESTRO
            # objeto de esa unidad (ronda 55: _Xlen<WTrigger> con PAU/PAV; zEAXSound2 define la PAU y el voto venia de
            # otra unidad con la PAV: el renombre perdia la funcion)
            caidas.append((t, actual, dict(c), 'casa y el nombre nuevo no esta en nuestra unidad'))
            continue
        if actual in O and canon(O[actual]) != canon(N[nb]):
            # --fn: una fn_ (sin nombre) que los llamantes nombran con --min-fn votos o mas, aunque nuestro codigo aun
            # no sea el suyo (funcion sin casar: el nombre la empareja en la medida y en destinos.py)
            if not (FN and ANONIMO.match(actual) and c[nb] >= MIN_FN and nb not in dir_de):
                caidas.append((t, actual, dict(c), 'codigo distinto'))
                continue
            sin_casar.add(t)
        prop[t] = nb
    # repetidos: NB ya en otra direccion que no cambia
    while True:
        malas = []
        for t, nb in prop.items():
            t2 = dir_de.get(nb)
            if t2 is not None and t2 != t and t2 not in prop:
                malas.append(t)
        destinos = collections.Counter(prop.values())
        malas += [t for t, nb in prop.items() if destinos[nb] > 1]
        if not malas:
            break
        for t in set(malas):
            caidas.append((t, nom_de[t], {prop[t]: -1}, 'el nombre nuevo queda repetido'))
            del prop[t]
    for t in sorted(prop):
        print('%08X  %-60s -> %s %s  [%s]' % (t, nom_de[t][:60], prop[t][:80], '(sin casar)' if t in sin_casar else '',
                                            ', '.join(quien[t][:3])))
    if '--todas' in sys.argv:
        for t, actual, c, porque in sorted(caidas):
            print('  cae %08X %-50s %s: %s' % (t, actual[:50], porque, {k[:50]: v for k, v in c.items()}))
    print('refs360: %d renombres, %d de ellos fn_ sin casar (%d propuestas caen)' % (len(prop), len(sin_casar), len(caidas)))
    if aplicar and prop:
        for i, x in enumerate(lineas):
            m = RX.match(x)
            if m and int(m.group(2), 16) in prop:
                lineas[i] = prop[int(m.group(2), 16)] + x[len(m.group(1)):]
        tmp = SIMS + '.refs360.tmp'   # escritura atomica: con el disco lleno no deja symbols.txt a medias
        with open(tmp, 'w', encoding='utf-8', newline='\n') as fh:
            fh.write('\n'.join(lineas))
        os.replace(tmp, SIMS)
        print('aplicados %d' % len(prop))
    return 0


if __name__ == '__main__':
    sys.exit(main())
