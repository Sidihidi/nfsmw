#!/usr/bin/env python3
"""nombra_vtable360.py -- nombres de Xbox 360 por las VTABLES (ronda 42).

Las funciones virtuales diminutas (getters de 8 B: `lfs f1, 0x34(r3); blr`) tienen el mismo codigo que muchas otras:
ni nombra_grafo (esqueleto unico) ni nombra_corrige360 (llamadas `bl`; una virtual se llama con bctrl) ni
nombra_permuta360 (codigo exacto unico en la unidad) pueden decidir su nombre, y los heredados estan a veces
cambiados: el original pone 0x82457A20 en el hueco de IInductable::InductionSpool de EngineRacer y 0x82457A18 en el de
GetInductionPSI, y symbols.txt los tenia al reves (el codigo lo confirma: 0x82457A20 lee mSpool).

Metodo:
  1. Vtables emparejadas: en cada funcion que se llama igual en el original y en lo nuestro y tiene la misma longitud,
     el `lis` que en lo nuestro carga `??_7Clase@@6B...@` carga en el original la direccion de SU vtable (constructores
     y destructores). Solo parejas unanimes y biyectivas.
  2. Hueco a hueco: la palabra k de nuestra vtable (nuestro enlace LTCG: el .map da el nombre) frente a la palabra k
     de la del original (el ejecutable). Si la funcion del original en ese hueco se llama distinto, se le pone el
     nombre nuestro cuando (a) todos los huecos que la apuntan dicen el mismo nombre, (b) su codigo es EXACTAMENTE el
     de la nuestra (esqueleto: campos reubicados a cero, pool y tablas normalizados), (c) nuestra unidad la define y
     (d) el nombre no lo lleva ya otra funcion de la unidad que case, o esa tambien cambia (como nombra_permuta360).
  3. Control de cada vtable: los huecos ya nombrados tienen que coincidir con los nuestros en su mayoria (si no, la
     vtable de 360 es otra: version distinta de la clase) -- se descarta la vtable entera.

    python scripts/nombra_vtable360.py [--banco=DIR/b/ETIQ] [--raiz=DIR] [--aplicar] [--detalle] [--vt=SUBCADENA]
Sin --banco usa la medida oficial (ronda 54): build/<V>/ltcg/NFS_ltcg.{exe,map}, build/<V>/ltcg/{orig,obj} y
build/versiones/<V>.json, como nombra_permuta360. Con --banco, un banco conservado (scripts/trocea360.py banco ETIQ
--conservar): su NFS.exe, NFS.map, {orig,obj} y ETIQ.json.
"""
import bisect
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import ltcg360  # noqa: E402
import nombra_grafo as ng  # noqa: E402
from nombra_permuta360 import casadas, ANONIMO  # noqa: E402
from escritura import entero  # noqa: E402

V = 'EUROPEGERMILESTONE'
ORIG_BASE = 0x82000000
REFHI = 0x10
MIN_ACUERDO = 0.5


def opt(nombre, defecto=None):
    for a in sys.argv[1:]:
        if a.startswith('--%s=' % nombre):
            return a.split('=', 1)[1]
    return defecto


def main():
    raiz = os.path.abspath(opt('raiz', RAIZ))
    x = ng.X360.__new__(ng.X360)
    if opt('banco'):
        banco = os.path.abspath(opt('banco'))
        exe, mapa_f, med = os.path.join(banco, 'NFS.exe'), os.path.join(banco, 'NFS.map'), banco + '.json'
        orig_d, obj_d = os.path.join(banco, 'orig'), os.path.join(banco, 'obj')
    else:   # la medida oficial: el enlace LTCG de pctall/medir y su troceado
        r = ltcg360.rutas(RAIZ)
        exe, mapa_f, orig_d, obj_d = r['exe'], r['map'], r['orig'], r['obj']
        med = os.path.join(RAIZ, 'build', 'versiones', V + '.json')
    pct = casadas(med)
    sims = os.path.join(raiz, 'config', V, 'symbols.txt')
    lineas = open(sims, encoding='utf-8').read().split('\n')
    nombres_en = collections.defaultdict(list)   # direccion -> [(nombre, indice)] (.text)
    dir_de = {}                                    # nombre de dato -> direccion
    for i, l in enumerate(lineas):
        m = re.match(r'^(\S+) = (\.\w+):0x([0-9A-Fa-f]+);', l)
        if m:
            a = int(m.group(3), 16)
            if m.group(2) == '.text':
                nombres_en[a].append((m.group(1), i))
            else:
                dir_de.setdefault(m.group(1), a)
    rangos = []
    u = None
    for l in open(os.path.join(raiz, 'config', V, 'splits.txt'), encoding='utf-8'):
        m = re.match(r'^(\S.*)\.cpp:', l)
        if m:
            u = m.group(1).split('/')[-1]
            continue
        m = re.match(r'\s+\.text\s+start:0x(\w+) end:0x(\w+)', l)
        if m and u:
            rangos.append((int(m.group(1), 16), int(m.group(2), 16), u))
    rangos.sort()

    def unidad_de(a):
        i = bisect.bisect_right([r[0] for r in rangos], a) - 1
        return rangos[i][2] if i >= 0 and a < rangos[i][1] else None

    def dir_dato(n):
        m = re.match(r'^(lbl|fn|data)_([0-9A-F]{8})$', n)
        return int(m.group(2), 16) if m else dir_de.get(n)

    # nuestro enlace
    pe_n = ltcg360.PE(exe)
    mapa = ltcg360.lee_mapa(mapa_f)
    fun_n = collections.defaultdict(list)
    sim_rva = {}
    todos = sorted(set(r for r, n, ob, f, e in mapa))
    for rva, n, ob, f, est in mapa:
        if f:
            fun_n[rva].append(n)
        sim_rva.setdefault(n, rva)
    pe_o = ltcg360.PE(os.path.join(RAIZ, 'orig', V, 'NFS.exe'))

    # 1. vtables emparejadas
    ev = collections.defaultdict(collections.Counter)
    objs = {}
    for un in sorted(pct):
        po = os.path.join(orig_d, 'Speed', 'Indep', 'SourceLists', un + '.obj')
        pn = os.path.join(obj_d, 'Speed', 'Indep', 'SourceLists', un + '.obj')
        if not (os.path.exists(po) and os.path.exists(pn)):
            continue
        co = ng.Coff(po)
        fo, fn = co.funciones(), ng.Coff(pn).funciones()
        # direccion de cada funcion del original: por nombre y unidad
        objs[un] = (fo, fn)
        for nombre in fo.keys() & fn.keys():
            (po_, ro), (pn_, rn) = fo[nombre], fn[nombre]
            if len(po_) == len(pn_):
                parejas = [(rn[k], ro[k]) for k in rn if k in ro]
            else:   # longitud distinta (Construct con el constructor expandido): los `lis` en orden, si son tantos
                ho = [ro[k] for k in sorted(ro) if ro[k][0] == REFHI]
                hn = [rn[k] for k in sorted(rn) if rn[k][0] == REFHI]
                parejas = list(zip(hn, ho)) if len(ho) == len(hn) else []
            for r, q in parejas:
                if r[0] == REFHI and len(r) > 1 and r[1].startswith('??_7') and q[0] == REFHI:
                    a = dir_dato(q[1] if len(q) > 1 else '')
                    if a:
                        ev[r[1]][a] += 1
    pares = {}
    usados = collections.Counter()
    for vt, c in ev.items():
        if len(c) == 1:
            a = list(c)[0]
            pares[vt] = a
            usados[a] += 1
    pares = {vt: a for vt, a in pares.items() if usados[a] == 1 and vt in sim_rva}
    print('vtables emparejadas por constructor: %d (de %d con evidencia)' % (len(pares), len(ev)))

    # 1b. por CONTENIDO (constructor expandido en todos sus llamantes): las palabras de nuestra vtable cuyos nombres
    # ya estan en el original en una sola direccion se buscan en el .rdata del original; vale la posicion unica
    # donde coinciden al menos dos huecos y los que la contradicen (nombre heredado distinto) no pasan de la mitad
    dir_fun = collections.defaultdict(set)
    for a, ns in nombres_en.items():
        for nm, _ in ns:
            dir_fun[nm].add(a)
    _, rva_rd, vs_rd, pr_rd, rs_rd = pe_o.seccion('.rdata')
    rd = pe_o.d[pr_rd:pr_rd + rs_rd]
    pos = collections.defaultdict(list)
    for i in range(0, len(rd) - 3, 4):
        w = struct.unpack_from('>I', rd, i)[0]
        if 0x820F0000 <= w < 0x83000000:
            pos[w].append(ORIG_BASE + rva_rd + i)
    ya_o = set(pares.values())
    por_contenido = 0
    candidatas = {}
    for vt, r0 in sim_rva.items():
        if not vt.startswith('??_7') or vt in pares:
            continue
        i = bisect.bisect_right(todos, r0)
        n = min(((todos[i] if i < len(todos) else r0 + 4) - r0) // 4, 512)
        wn = struct.unpack('>%dI' % n, pe_n.lee(r0, 4 * n))
        slots = []
        for k in range(n):
            nn = fun_n.get(wn[k] - pe_n.base)
            if not nn:
                break
            ds = set().union(*(dir_fun.get(m, set()) for m in nn))
            slots.append(ds if len(ds) == 1 else None)
        votos = collections.Counter()
        for k, ds in enumerate(slots):
            if ds:
                for h in pos.get(next(iter(ds)), []):
                    votos[h - 4 * k] += 1
        buenas = []
        for ini, v in votos.items():
            if v < 2 or ini in ya_o:
                continue
            malos = 0
            for k, ds in enumerate(slots):
                if ds:
                    w = struct.unpack('>I', pe_o.lee(ini + 4 * k - ORIG_BASE, 4))[0]
                    if w not in ds and nombres_en.get(w) and not ANONIMO.match(nombres_en[w][0][0]):
                        malos += 1
            if 2 * malos <= v:
                buenas.append(ini)
        if len(buenas) == 1:
            candidatas[vt] = buenas[0]
    usadas = collections.Counter(candidatas.values())
    for vt, a in candidatas.items():
        if usadas[a] == 1:
            pares[vt] = a
            por_contenido += 1
    print('vtables emparejadas por contenido: %d' % por_contenido)

    # 2. hueco a hueco
    voto = collections.defaultdict(collections.Counter)   # direccion del original -> nombres nuestros
    descartadas = 0
    acuerdo_total = [0, 0]
    def mismo_codigo(ao, viejo, nuevo):
        un = unidad_de(ao)
        if un not in objs:
            return False
        fo, fn = objs[un]
        return viejo in fo and nuevo in fn and x.esqueleto(*fo[viejo]) == x.esqueleto(*fn[nuevo])

    for vt, a in sorted(pares.items()):
        r0 = sim_rva[vt]
        i = bisect.bisect_right(todos, r0)
        r1 = todos[i] if i < len(todos) else r0 + 4
        n = min((r1 - r0) // 4, 512)
        wn = struct.unpack('>%dI' % n, pe_n.lee(r0, 4 * n))
        wo = struct.unpack('>%dI' % n, pe_o.lee(a - ORIG_BASE, 4 * n))
        huecos = []
        bien = mal = 0
        for k in range(n):
            nn = fun_n.get(wn[k] - pe_n.base)
            if not nn or not (0x82000000 <= wo[k] < 0x83000000):
                break
            ya = [nm for nm, _ in nombres_en.get(wo[k], [])]
            if ya and not ANONIMO.match(ya[0]):
                if ya[0] in nn:
                    bien += 1
                elif not mismo_codigo(wo[k], ya[0], nn[0]):
                    mal += 1   # un nombre heredado distinto con el MISMO codigo es justo lo que se corrige
            huecos.append((wo[k], nn))
        acuerdo_total[0] += bien
        acuerdo_total[1] += mal
        if opt('vt') and opt('vt') in vt:
            print('VT %s -> 0x%08X bien %d mal %d' % (vt, a, bien, mal))
            for ao, nn in huecos:
                print('   0x%08X %-50s <- %s' % (ao, (nombres_en.get(ao) or [('?', 0)])[0][0][:50], nn[0][:60]))
        if bien + mal == 0 or bien < MIN_ACUERDO * (bien + mal):
            descartadas += 1
            continue
        for ao, nn in huecos:
            if len(nn) == 1:
                voto[ao][nn[0]] += 1
            else:
                voto[ao]['<varios>'] += 1
    print('huecos ya nombrados: %d iguales, %d distintos; vtables descartadas por desacuerdo: %d' % (
        acuerdo_total[0], acuerdo_total[1], descartadas))

    # control: en las vtables aceptadas, los huecos cuya funcion del original YA casa al 100 % con su nombre (anclas)
    val = collections.Counter()
    for ao, c in voto.items():
        ya = nombres_en.get(ao, [])
        un = unidad_de(ao)
        if not ya or un not in pct or pct[un].get(ya[0][0]) != 100.0 or len(c) != 1 or '<varios>' in c:
            continue
        ok_ = list(c)[0] == ya[0][0]
        val['bien' if ok_ else 'distinta'] += 1
        if not ok_ and '--detalle' in sys.argv:
            print('   VAL 0x%08X %s <- %s' % (ao, ya[0][0][:70], list(c)[0][:70]))
    print('validacion (huecos de anclas con voto unanime): %s' % dict(val))

    # 3. propuestas
    prop = collections.defaultdict(dict)   # unidad -> {nombre viejo: (dir, nombre nuevo, indice)}
    for ao, c in voto.items():
        if len(c) != 1:
            continue
        nuevo = list(c)[0]
        if nuevo == '<varios>' or ANONIMO.match(nuevo):
            continue
        ya = nombres_en.get(ao, [])
        if not ya or ya[0][0] == nuevo:
            continue
        viejo, idx = ya[0]
        un = unidad_de(ao)
        if un not in objs:
            continue
        fo, fn = objs[un]
        if viejo not in fo or nuevo not in fn or pct[un].get(viejo) == 100.0:
            continue
        if x.esqueleto(*fo[viejo]) != x.esqueleto(*fn[nuevo]):
            continue
        if pct[un].get(nuevo) == 100.0:
            continue
        prop[un][viejo] = (ao, nuevo, idx)
    cambios = []
    for un, p in prop.items():
        fo = objs[un][0]
        nuevos = {v[1] for v in p.values()}
        cambio = True
        while cambio:   # el nombre nuevo no puede quedar repetido en la unidad
            cambio = False
            for viejo, (ao, nuevo, idx) in list(p.items()):
                if nuevo in fo and nuevo not in p:
                    del p[viejo]
                    cambio = True
        dup = collections.Counter(v[1] for v in p.values())
        for viejo, (ao, nuevo, idx) in p.items():
            if dup[nuevo] == 1:
                cambios.append((un, ao, viejo, nuevo, idx))
    for un, ao, viejo, nuevo, idx in sorted(cambios):
        print('%-18s 0x%08X %-55s -> %s' % (un, ao, viejo[:55], nuevo[:90]))
    print('por vtable: %d nombres' % len(cambios))
    if '--aplicar' in sys.argv and cambios:
        for un, ao, viejo, nuevo, idx in cambios:
            assert lineas[idx].startswith(viejo + ' = .text:'), lineas[idx]
            lineas[idx] = nuevo + lineas[idx][len(viejo):]
        entero(sims, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
        print('aplicados %d' % len(cambios))
    return 0


if __name__ == '__main__':
    sys.exit(main())
