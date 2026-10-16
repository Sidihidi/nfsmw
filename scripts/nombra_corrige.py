#!/usr/bin/env python3
"""nombra_corrige.py -- regla de NOMBRE EQUIVOCADO para PC (SPEED_EXE_1_3), hermana de scripts/nombra_elim.py.

nombra_elim.py y nombra_grafo.py solo ponen nombre a funciones SIN nombre (sub_, marcadores de IDA, decoraciones de
GCC). Pero symbols.json trae nombres FALSOS de origen (IDA, upstream, el PDB generado, rondas viejas): una funcion que
se llama A y cuyo codigo es el de nuestra B. Mientras siga mal, ni A ni B casan. Esta regla los corrige con dos
pruebas independientes, y SOLO cuando hay una alternativa con prueba fuerte:

  corrige-codigo  La funcion del exe lleva un nombre N cuyo codigo NO es el nuestro (o un nombre que nuestros objetos
                  no definen, o un nombre REPETIDO en varias direcciones), y su ESQUELETO (bytes con las reubicaciones
                  a cero, el de nombra_grafo) es igual al de UNA sola funcion nuestra libre de la unidad, y viceversa
                  (1:1). Libre = su nombre no esta en el exe, o esta solo en direcciones de la unidad cuyo codigo lo
                  contradice. Mismas reglas del enlazador que nombra_elim (primer definidor). Filtros: ningun destino
                  de reubicacion se contradice (por DIRECCION, con los nombres ya corregidos); una funcion de menos
                  de 16 B con reubicaciones necesita un testigo; si el codigo del nombre viejo se PARECE mucho
                  (>= 0,9: una casi exacta suya) hace falta un testigo o la evidencia de los llamantes; y la evidencia
                  de los llamantes, si la hay, no puede decir otro nombre.
  mueve-codigo    Lo mismo cuando la funcion del exe no tenia nombre y el nombre libre viene de otra direccion.
  corrige-llamantes  Evidencia de LLAMANTES: cada ANCLA FUERTE (nombre que casa byte a byte con el nuestro y esqueleto
                  unico en su unidad, en el exe y en nuestro objeto) llama en la posicion i a una direccion A del exe
                  donde la nuestra llama a T. Si al menos 2 anclas fuertes distintas (o 1, si ademas el codigo de A es
                  el de T) dicen T y ninguna dice otra cosa, A se llama T. T no puede casar ya en otra direccion.
  nombra-llamantes  Una funcion SIN nombre (sub_, demangle de IDA) a la que las anclas fuertes llaman, de forma
                  unanime, con un nombre nuestro de su unidad que no esta puesto en ningun sitio, y cuyo codigo se
                  parece al nuestro (>= 0,6): una casi exacta (o una copia que el codigo solo no distingue) que asi
                  aparece en el informe.
  quita-duplicado / quita-falso  Un nombre repetido en una unidad con una sola colocacion confirmada, o un nombre
                  que se movio a su sitio de verdad: la colocacion falsa vuelve a sub_.
  corrige-decoracion  El caso particular en que el nombre viejo y el nuevo dicen la MISMA funcion (mismo nombre de pila
                  y ambito) y solo cambia la decoracion (class/struct, const, convencion de llamada, `$09` -> `$0BE` de
                  los limites de PC) o es el nombre de C de IDA sin decorar (`InitAnimCtrls` -> `?InitAnimCtrls@@YAXXZ`).
                  No es un nombre falso: es la misma funcion escrita como la compila nuestra fuente.

Punto fijo: cada correccion aceptada cambia el estado (el nombre viejo queda libre, el destino de otras parejas deja de
contradecirse) y se vuelve a evaluar hasta que no sale nada. Despues, objdiff: se renombra en una COPIA del .obj
troceado y la pareja de codigo tiene que dar 100 %; las de llamantes no se exigen al 100 % (su codigo puede ser una casi
exacta: el nombre se corrige por veracidad, no por porcentaje) pero se informa su porcentaje.

COPIAS AMBIGUAS (--copias): repasa los nombres marcados `copia-ambigua` en nombres_procedencia.tsv. Pasan a `seguro`
los que (a) los DESTINOS distinguen (la pareja puesta no tiene contradicciones y con cada alternativa del grupo de
codigo igual si las hay) o (b) los LLAMANTES fuertes confirman. Se corrige (intercambio dentro del grupo) el que los
llamantes fuertes contradicen de forma unanime con un nombre del mismo grupo. Regla `copia-confirmada` /
`copia-corregida`.

VALIDACION OBLIGATORIA antes de aplicar:
  - codigo: se esconde el 30 % de las anclas fuertes y se BARAJAN sus nombres dentro de cada unidad (cada una recibe el
    nombre de otra, un nombre falso a proposito); se mide cuantas recupera la regla con SU nombre (minimo 99 %).
  - llamantes: para las anclas fuertes con evidencia en su direccion, cuantas veces la evidencia unanime dice SU nombre
    (precision de la evidencia; minimo 99 %).

PROCEDENCIA: cada cambio va a config/SPEED_EXE_1_3/nombres_procedencia.tsv con su regla y marca (`seguro` o
`decoracion`); `nombra_elim.py --quitar REGLA` los deshace (devuelve el nombre anterior). Un cambio de nombre puede
BAJAR una funcion que hoy cuenta 100 % por un nombre falso (copias): eso es veracidad, y se informa aparte.

    python scripts/nombra_corrige.py                      # informa (validacion + objdiff), no escribe
    python scripts/nombra_corrige.py --tsv SALIDA.tsv     # las correcciones propuestas
    python scripts/nombra_corrige.py --aplicar            # aplica y anota la procedencia
    python scripts/nombra_corrige.py --copias [--aplicar] # repaso de las copias ambiguas

Despues de aplicar: medir PC (scripts/medir.py SPEED_EXE_1_3) y pasar nombra_elim.py y nombra_grafo.py (los nombres
viejos liberados pueden tener su sitio en otra direccion). Ronda 39 (2026-10-07).
"""
import collections
import datetime
import difflib
import json
import os
import random
import re
import struct
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import nombra_elim as ne  # noqa: E402
import nombra_grafo as ng  # noqa: E402

DELINK = re.compile(r'^(.*)__delink_([0-9A-F]{8})$')
SUB = re.compile(r'^(?:sub|nullsub|j_sub|unknown_libname)_([0-9A-F]+)$')
PARECIDO_MAX = 0.9
PARECIDO_CASI = 0.6
MIN_LLAMANTES = 2
UMBRAL = {'codigo': 0.99, 'llamantes': 0.99}


def base(o):
    m = DELINK.match(o)
    return (m.group(1), int(m.group(2), 16)) if m else (o, None)


def parecido(a, b):
    sa, sb = a[0], b[0]
    if sa == sb:
        return 1.0
    if max(len(sa), len(sb)) > 3000:
        return min(len(sa), len(sb)) / max(len(sa), len(sb)) * 0.5
    return difflib.SequenceMatcher(None, sa, sb, autojunk=False).ratio()


def misma_funcion(viejo, nuevo):
    """True si los dos nombres dicen la misma funcion con otra decoracion (o el nombre de C de IDA sin decorar)."""
    if ng.equivalentes(viejo, nuevo):
        return True
    p = ng.msvc_partes(nuevo)
    if not p:
        return nuevo.lstrip('_') == viejo.lstrip('_')
    if not viejo.startswith('?'):
        return ng.mismo_nombre_de_pila(viejo, nuevo) or viejo == p[0] or viejo.lstrip('_') == p[0]
    q = ng.msvc_partes(viejo)
    if not q:
        return False
    if p[0] != q[0]:
        return False
    if p[1] is None or q[1] is None:
        return True
    return p[1] == q[1]


class Estado:
    """Nombres actuales del exe por direccion (lista functions) y sus colocaciones; se actualiza con cada cambio."""

    def __init__(self, exe):
        d = json.load(open(ne.SYMBOLS, encoding='utf-8'))
        self.exe = exe
        self.nombre = {f['start']: f['name'] for f in d['functions']}
        self.fin = {f['start']: f['end'] for f in d['functions']}
        self.dirs = exe.dirs  # {nombre: set(direcciones)}: functions + names (se modifica)
        self.desnombradas = set()  # direcciones a las que esta regla quito un nombre falso (vuelven a sub_)

    def renombra(self, a, nuevo):
        viejo = self.nombre[a]
        self.dirs[viejo].discard(a)
        if not self.dirs[viejo]:
            del self.dirs[viejo]
        self.dirs.setdefault(nuevo, set()).add(a)
        self.nombre[a] = nuevo


class U:
    """Una unidad: funciones del original con su direccion, y las nuestras."""

    def __init__(self, u, estado):
        self.u = u
        self.nombre = u.nombre
        self.fo, self.fs, self.eo, self.es = u.fo, u.fs, u.eo, u.es
        rs = estado.exe.rangos[u.nombre]
        dentro = lambda a: any(i <= a < f for i, f in rs)  # noqa: E731
        self.dir = {}
        delink = collections.defaultdict(set)
        for o in self.fo:
            b, a = base(o)
            if a is not None:
                self.dir[o] = a
                delink[b].add(a)
        for o in self.fo:
            if o in self.dir:
                continue
            ds = [a for a in estado.dirs.get(o, ()) if dentro(a) and a not in delink.get(o, ())]
            if len(ds) == 1:
                self.dir[o] = ds[0]
        self.obj_de = {a: o for o, a in self.dir.items()}
        cuenta_o = collections.Counter(self.eo.values())
        cuenta_s = collections.Counter(self.es.values())
        self.unico_o = {o for o, e in self.eo.items() if cuenta_o[e] == 1}
        self.unico_s = {n for n, e in self.es.items() if cuenta_s[e] == 1}
        self.dentro = dentro


def carga():
    exe = ne.Exe()
    unidades = [u for u in (ne.Unidad(exe, n) for n in exe.unidades) if u.ok]
    est = Estado(exe)
    us = [U(u, est) for u in unidades]
    primero = {}
    for u in unidades:
        for n in u.fs:
            primero.setdefault(n, u.nombre)
    return exe, est, us, primero


def estado_de(u, est, o):
    a = u.dir.get(o)
    if a is None:
        return None, None
    b = est.nombre.get(a)
    if b is None:
        return None, None
    if not ne.valido(b):
        return 'sinnombre', b
    if b in u.fs:
        return ('casa' if u.eo[o] == u.es[b] else 'contradice'), b
    return 'ajeno', b


def anclas_fuertes(us, est):
    """{direccion: (unidad, obj, nombre)}: casa byte a byte y su esqueleto es unico en el exe y en el nuestro."""
    out = {}
    for u in us:
        for o in u.fo:
            e, b = estado_de(u, est, o)
            if e == 'casa' and o in u.unico_o and b in u.unico_s:
                out[u.dir[o]] = (u, o, b)
    return out


def direccion(est, no):
    b, a = base(no)
    if a is not None:
        return a
    m = SUB.match(no)
    if m:
        return int(m.group(1), 16)
    ds = est.dirs.get(no)
    return next(iter(ds)) if ds and len(ds) == 1 else None


def evidencia(us, est, fuertes, nuestras):
    """{direccion: Counter(nombre nuestro -> anclas fuertes distintas que lo llaman ahi)}."""
    ev = collections.defaultdict(collections.Counter)
    for a0, (u, o, b) in fuertes.items():
        ro, rs = u.fo[o][1], u.fs[b][1]
        ps = u.fs[b][0]
        vistos = set()
        for i, (t, no) in ro.items():
            if t not in (ng.I386_REL32, ng.I386_DIR32):
                continue
            m = rs.get(i)
            if not m or m[1] not in nuestras:
                continue
            k = struct.unpack_from('<i', bytes(ps[i:i + 4]))[0] if i + 4 <= len(ps) else None
            if k != 0:
                continue
            a = direccion(est, no)
            if a is None or a not in est.nombre or (a, m[1]) in vistos:
                continue
            vistos.add((a, m[1]))
            ev[a][m[1]] += 1
    return ev


def destinos(est, vo, vs):
    """(contradicciones, testigos), como nombra_elim.destinos pero con los nombres ya corregidos."""
    ro, rs = vo[1], vs[1]
    contra = testigos = 0
    for i, (t, no) in ro.items():
        m = rs.get(i)
        if m is None:
            contra += 1
            continue
        ns = m[1]
        if ng.equivalentes(no, ns):
            testigos += 1
            continue
        k = vs[0][i:i + 4]
        if len(k) == 4 and any(k):
            continue
        ao = direccion(est, no)
        ds = est.dirs.get(ns)
        as_ = next(iter(ds)) if ds and len(ds) == 1 else None
        if ao is not None and as_ is not None:
            if ao == as_:
                testigos += 1
            else:
                contra += 1
            continue
        if ao is not None:
            v = est.exe.memoria.comprueba(ao, ns)
            if v is True:
                testigos += 1
            elif v is False:
                contra += 1
    return contra, testigos


def libres(u, est, primero):
    """(pool, liberados) de la unidad."""
    pool = set(n for n in u.fs if n not in est.dirs and primero.get(n) == u.nombre and ne.valido(n)
               and not n.startswith(ne.EH) and len(n) <= 2000)
    coloc = collections.defaultdict(list)
    for o in u.fo:
        e, b = estado_de(u, est, o)
        if e:
            coloc[b].append(e)
    lib = set()
    for b, es in coloc.items():
        if b in u.fs and primero.get(b) == u.nombre and len(est.dirs.get(b, ())) == len(es) \
                and all(e == 'contradice' for e in es):
            lib.add(b)
    return pool, lib




def propuesta(u, o, est, nuevo, regla, tam, testigos, par, codigo):
    a = u.dir[o]
    viejo = est.nombre[a]
    e, _ = estado_de(u, est, o)
    if regla in ('corrige-codigo', 'corrige-llamantes') and misma_funcion(viejo, nuevo) \
            and len(est.dirs.get(viejo, ())) <= 1:
        regla = 'corrige-decoracion'
    return {'u': u, 'o': o, 'a': a, 'nuevo': nuevo, 'regla': regla, 'viejo': viejo, 'tam': tam,
            'testigos': testigos, 'par': par, 'antes': e, 'codigo': codigo, 'pct': None, 'vuelta': 0}


def propone_codigo(us, est, primero, ev):
    out = []
    for u in us:
        pool, lib = libres(u, est, primero)
        cand = pool | lib
        g = collections.defaultdict(lambda: ([], []))
        for n in cand:
            g[u.es[n]][1].append(n)
        for o in u.fo:
            e, b = estado_de(u, est, o)
            if e in ('sinnombre', 'contradice', 'ajeno') and u.eo[o] in g:
                g[u.eo[o]][0].append((o, e, b))
        for _, (os_, ns) in g.items():
            if len(os_) != 1 or len(ns) != 1:
                continue
            (o, e, b), n = os_[0], ns[0]
            if e == 'sinnombre' and n in pool and u.dir[o] not in est.desnombradas:
                continue  # trabajo de nombra_elim
            if o not in u.unico_o or n not in u.unico_s:
                continue  # copias de codigo igual en la unidad: el 1:1 entre libres no prueba nada (--copias)
            a = u.dir[o]
            contra, testigos = destinos(est, u.fo[o], u.fs[n])
            if contra:
                continue
            tam = len(u.fo[o][0])
            if tam < ne.MIN_SIN_TESTIGO and u.fo[o][1] and not testigos:
                continue
            c = ev.get(a)
            if c and (len(c) != 1 or n not in c):
                continue  # los llamantes dicen otra cosa
            par = parecido(u.eo[o], u.es[b]) if e == 'contradice' else 0.0
            if par >= PARECIDO_MAX and not testigos and not c:
                continue
            regla = 'mueve-codigo' if e == 'sinnombre' else 'corrige-codigo'
            out.append(propuesta(u, o, est, n, regla, tam, testigos + (c[n] if c else 0), par, True))
    return out


def propone_llamantes(us, est, primero, ev, fuertes):
    """Direcciones con nombre cuyo nombre contradice la evidencia unanime de las anclas fuertes."""
    por_dir = {}
    for u in us:
        for o, a in u.dir.items():
            por_dir[a] = (u, o)
    out = []
    for a, c in ev.items():
        if len(c) != 1 or a not in por_dir or a in fuertes:
            continue
        n, k = next(iter(c.items()))
        u, o = por_dir[a]
        viejo = est.nombre[a]
        if ng.equivalentes(viejo, n) or not ne.valido(viejo):
            continue  # sin nombre: no es una correccion
        if est.dirs.get(n):
            continue  # el nombre esta puesto en otra direccion: se movera cuando quede libre (siguiente vuelta)
        igual = n in u.fs and u.es[n] == u.eo[o]
        if k < MIN_LLAMANTES and not igual:
            continue
        if primero.get(n) is not None and primero.get(n) != u.nombre:
            continue  # la COMDAT va con su primer definidor
        out.append(propuesta(u, o, est, n, 'corrige-llamantes', len(u.fo[o][0]), k, 1.0 if igual else 0.0, igual))
    return out


def propone_casi(us, est, primero, ev, fuertes):
    """nombra-llamantes: una funcion SIN nombre (sub_, demangle de IDA, decoracion de GCC) cuyo codigo no es igual al
    nuestro (por eso nombra_elim/nombra_grafo no la tocan) pero a la que las anclas fuertes llaman de forma unanime
    con un nombre nuestro de su unidad que no esta puesto en ningun sitio. Es una casi exacta: el nombre la hace
    visible en el informe. El codigo tiene que parecerse al nuestro (>= PARECIDO_CASI)."""
    por_dir = {}
    for u in us:
        for o, a in u.dir.items():
            por_dir[a] = (u, o)
    out = []
    for a, c in ev.items():
        if len(c) != 1 or a not in por_dir or a in fuertes:
            continue
        n, k = next(iter(c.items()))
        u, o = por_dir[a]
        viejo = est.nombre.get(a)
        if viejo is None or ne.valido(viejo) or est.dirs.get(n):
            continue
        if n not in u.fs or primero.get(n) != u.nombre:
            continue
        par = parecido(u.eo[o], u.es[n])
        if par < PARECIDO_CASI:
            continue
        out.append(propuesta(u, o, est, n, 'nombra-llamantes', len(u.fo[o][0]), k, par, False))
    return out


def propone_duplicados(us, est, ev):
    """Un nombre repetido en varias direcciones de una MISMA unidad (una unidad es un solo .cpp de enlace: un nombre
    no puede estar dos veces) con UNA colocacion confirmada (codigo igual al nuestro, o los llamantes): las demas son
    falsas y pierden el nombre (vuelven a sub_)."""
    out = []
    for u in us:
        coloc = collections.defaultdict(list)
        for o, a in u.dir.items():
            if a in est.nombre:
                coloc[est.nombre[a]].append(o)
        for b, os_ in coloc.items():
            if len(os_) < 2 or not ne.valido(b) or b not in u.fs:
                continue
            conf, resto = [], []
            for o in os_:
                c = ev.get(u.dir[o])
                ok = u.eo[o] == u.es[b] or (c and len(c) == 1 and b in c)
                (conf if ok else resto).append(o)
            if len(conf) != 1:
                continue
            for o in resto:
                c = ev.get(u.dir[o])
                if c and b in c:
                    continue
                out.append(propuesta(u, o, est, 'sub_%X' % u.dir[o], 'quita-duplicado', len(u.fo[o][0]),
                                     0, 0.0, False))
    return out


def unicas(props):
    """Un nombre nuevo solo una vez y una direccion solo una vez por vuelta."""
    usado_n = collections.Counter(p['nuevo'] for p in props)
    usado_a = collections.Counter(p['a'] for p in props)
    return [p for p in props if usado_n[p['nuevo']] == 1 and usado_a[p['a']] == 1]


def punto_fijo(exe, est, us, primero, llamantes=True, verificar=False):
    """Vueltas hasta que no sale nada. Con `verificar`, cada vuelta pasa por objdiff (con todos los cambios de la
    unidad hasta ese momento): una pareja de CODIGO que no da 100 % se deshace y no se vuelve a proponer."""
    nuestras = set()
    for u in us:
        nuestras.update(u.fs)
    viejos = dict(est.nombre)
    rechazadas = set()
    aceptadas = []
    for vuelta in range(20):
        fuertes = anclas_fuertes(us, est)
        ev = evidencia(us, est, fuertes, nuestras)
        props = propone_codigo(us, est, primero, ev)
        if llamantes:
            props += propone_llamantes(us, est, primero, ev, fuertes)
            props += propone_casi(us, est, primero, ev, fuertes)
        # la misma pareja puede salir por codigo y por llamantes: una sola vez (la de codigo primero)
        vistas = set()
        props = [p for p in props if (p['a'], p['nuevo']) not in rechazadas
                 and not ((p['a'], p['nuevo']) in vistas or vistas.add((p['a'], p['nuevo'])))]
        nuevas = unicas(props)
        if llamantes and not nuevas:
            nuevas = unicas([p for p in propone_duplicados(us, est, ev) if (p['a'], p['nuevo']) not in rechazadas])
        if not nuevas:
            break
        for p in nuevas:
            p['vuelta'] = vuelta + 1
            # un nombre liberado sale de las direcciones donde estaba mal puesto: vuelven a sub_ (y pueden recibir
            # su nombre de verdad en otra vuelta)
            p['quitas'] = []
            for a2 in sorted(est.dirs.get(p['nuevo'], ())):
                if a2 != p['a'] and a2 in est.nombre:
                    p['quitas'].append((a2, est.nombre[a2]))
                    est.renombra(a2, 'sub_%X' % a2)
                    est.desnombradas.add(a2)
            est.renombra(p['a'], p['nuevo'])
            if p['nuevo'].startswith('sub_'):
                est.desnombradas.add(p['a'])
        if verificar:
            tocadas = {p['u'].nombre: p['u'] for p in nuevas}
            pct = verifica(list(tocadas.values()), viejos, est)
            malas = []
            for p in nuevas:
                p['pct'] = pct.get(p['a'])
                if p['codigo'] and p['pct'] != 100.0:
                    malas.append(p)
            for p in malas:
                est.renombra(p['a'], p['viejo'])
                for a2, v2 in p['quitas']:
                    est.renombra(a2, v2)
                rechazadas.add((p['a'], p['nuevo']))
                print('    descartada por objdiff:', hex(p['a']), p['viejo'][:60], '->', p['nuevo'][:60], p['pct'])
            nuevas = [p for p in nuevas if p not in malas]
        aceptadas += nuevas
        print('  vuelta %d: %d correcciones' % (vuelta + 1, len(nuevas)))
    return aceptadas


def valida_codigo(fraccion=0.3, semilla=11):
    """Esconde el 30 % de las anclas barajando sus nombres dentro de la unidad (cada una recibe el nombre de otra) y
    mide cuantas vuelven con SU nombre."""
    exe, est, us, primero = carga()
    random.seed(semilla)
    verdad = {}
    por_u = collections.defaultdict(list)
    for u in us:
        for o in sorted(u.fo):
            e, b = estado_de(u, est, o)
            if e == 'casa' and len(est.dirs.get(b, ())) == 1 and random.random() < fraccion:
                por_u[u.nombre].append((u.dir[o], b))
    for un, ls in por_u.items():
        if len(ls) < 2:
            continue
        nombres = [b for a, b in ls]
        rot = nombres[1:] + nombres[:1]
        for (a, b), falso in zip(ls, rot):
            verdad[a] = b
            est.nombre[a] = falso
            est.dirs.pop(b, None)
        for (a, b), falso in zip(ls, rot):
            est.dirs[falso] = {a}
    acept = punto_fijo(exe, est, us, primero, llamantes=False)
    bien = mal = 0
    malas = []
    for p in acept:
        if p['a'] in verdad:
            if verdad[p['a']] == p['nuevo']:
                bien += 1
            else:
                mal += 1
                malas.append((p['u'].nombre, hex(p['a']), p['viejo'][:80], p['nuevo'][:80], verdad[p['a']][:80]))
    fuera = sum(1 for p in acept if p['a'] not in verdad)
    return len(verdad), bien, mal, malas, fuera


def valida_llamantes():
    exe, est, us, primero = carga()
    nuestras = set()
    for u in us:
        nuestras.update(u.fs)
    fuertes = anclas_fuertes(us, est)
    ev = evidencia(us, est, fuertes, nuestras)
    bien = mal = 0
    malas = []
    for a, (u, o, b) in fuertes.items():
        c = ev.get(a)
        if not c:
            continue
        if len(c) == 1 and ng.equivalentes(b, next(iter(c))):
            bien += 1
        else:
            mal += 1
            malas.append((u.nombre, hex(a), b, dict(c)))
    return bien, mal, malas


def verifica(unidades, viejos, est):
    """{direccion: porcentaje} con objdiff sobre una copia del .obj troceado de cada unidad con TODOS los nombres
    cambiados hasta ahora en ella (un intercambio solo se ve con los dos lados renombrados). En el troceado una
    direccion puede llevar dos simbolos (el de `functions` y el de `names`): se renombra el de `functions`."""
    out = {}
    os.makedirs(ne.TMP, exist_ok=True)
    for u in unidades:
        un = u.nombre
        lugar = {}
        for s in ng.Coff(u.u.po).lista:
            lugar.setdefault(s[0], (s[1], s[2]))
        cambios = {}
        destino = {}
        for o, a in u.dir.items():
            if a not in est.nombre or est.nombre[a] == viejos.get(a):
                continue
            v = viejos[a]
            sim = v if v in lugar and lugar[v] == lugar.get(o) else o
            cambios[sim] = est.nombre[a]
            destino[a] = (sim, est.nombre[a])
        if not cambios:
            continue
        datos, hechos = ne.renombra_coff(open(u.u.po, 'rb').read(), cambios)
        t = os.path.join(ne.TMP, os.path.basename(un) + '.obj')
        sal = os.path.join(ne.TMP, os.path.basename(un) + '.json')
        open(t, 'wb').write(datos)
        r = subprocess.run([ne.OBJDIFF, 'diff', '-1', t, '-2', u.u.ps, '-c', 'functionRelocDiffs=none', '-o', sal,
                            '--format', 'json'], capture_output=True, text=True)
        if r.returncode == 0:
            d = json.load(open(sal, encoding='utf-8'))
            pct = {s.get('name'): s.get('match_percent') for s in d['left']['symbols']
                   if s.get('kind') == 'SYMBOL_FUNCTION'}
            for a, (sim, n) in destino.items():
                out[a] = pct.get(n) if sim in hechos else None
        else:
            print('objdiff fallo en', un, r.stderr[-300:])
        for q in (t, sal):
            if os.path.exists(q):
                os.remove(q)
    try:
        os.rmdir(ne.TMP)
    except OSError:
        pass
    return out


def main():
    args = sys.argv[1:]
    if '--copias' in args:
        return copias('--aplicar' in args, args[args.index('--tsv') + 1] if '--tsv' in args else None)
    print('VALIDACION codigo (anclas escondidas con los nombres barajados):')
    n, bien, mal, malas, fuera = valida_codigo()
    tasa_c = bien / (bien + mal) if bien + mal else 1.0
    print('  escondidas %d  recuperadas bien %d  mal %d  acierto %.4f (minimo %.2f); cambios fuera de las escondidas %d'
          % (n, bien, mal, tasa_c, UMBRAL['codigo'], fuera))
    for x in malas[:10]:
        print('    MAL', x)
    print('VALIDACION llamantes (evidencia unanime en la direccion de cada ancla fuerte):')
    b2, m2, malas2 = valida_llamantes()
    tasa_l = b2 / (b2 + m2) if b2 + m2 else 1.0
    print('  bien %d  mal %d  acierto %.4f (minimo %.2f)' % (b2, m2, tasa_l, UMBRAL['llamantes']))
    for x in malas2[:10]:
        print('    MAL', x)

    exe, est, us, primero = carga()
    viejos = dict(est.nombre)
    print('PASADA REAL')
    acept = punto_fijo(exe, est, us, primero, verificar=True)
    # el resultado es el estado final: por direccion, el nombre de symbols.json contra el de ahora (una direccion puede
    # haber pasado por sub_ en una vuelta y recibir su nombre en otra); la regla es la del ultimo paso
    ultimo = {}
    for p in acept:
        ultimo[p['a']] = p
    finales = []
    for a in sorted(a for a in est.nombre if est.nombre[a] != viejos.get(a)):
        p = ultimo.get(a)
        f = {'a': a, 'viejo': viejos[a], 'nuevo': est.nombre[a]}
        if p is not None and p['nuevo'] == est.nombre[a]:
            f.update({k: p[k] for k in ('u', 'regla', 'tam', 'testigos', 'par', 'antes', 'pct', 'vuelta')})
        else:
            f.update({'u': None, 'regla': 'quita-falso', 'tam': est.fin.get(a, a) - a, 'testigos': 0, 'par': 0.0,
                      'antes': 'contradice', 'pct': None, 'vuelta': 0})
        if f['regla'] == 'corrige-decoracion' and not misma_funcion(f['viejo'], f['nuevo']):
            f['regla'] = 'corrige-codigo' if f['par'] < 1 else 'corrige-llamantes'
        finales.append(f)
    cnt = collections.Counter()
    for p in finales:
        cnt[(p['regla'], 'cambios')] += 1
        cnt[(p['regla'], 'B')] += p['tam']
        if p['antes'] == 'casa':
            cnt[(p['regla'], 'hoy al 100 % con el nombre viejo')] += 1
        if p['pct'] == 100.0:
            cnt[(p['regla'], 'objdiff 100 con el nombre nuevo')] += 1
    for k in sorted(cnt, key=str):
        print(' ', k, cnt[k])
    if '--tsv' in args:
        with open(args[args.index('--tsv') + 1], 'w', encoding='utf-8') as f:
            for p in finales:
                un = p['u'].nombre.split('/')[-1] if p['u'] else '-'
                f.write('\t'.join(map(str, (un, p['regla'], '0x%X' % p['a'], p['viejo'], p['nuevo'], p['tam'],
                                            p['testigos'], '%.2f' % p['par'], p['antes'], p['pct'], p['vuelta'])))
                        + '\n')
    if '--aplicar' not in args:
        return 0
    if tasa_c < UMBRAL['codigo'] or tasa_l < UMBRAL['llamantes']:
        print('NO SE APLICA: la validacion no llega al minimo')
        return 1
    hechos = set(ne.aplica([(p['a'], p['viejo'], p['nuevo']) for p in finales]))
    fecha = datetime.date.today().isoformat()
    filas = []
    for p in finales:
        if (p['a'], p['viejo'], p['nuevo']) in hechos:
            marca = 'decoracion' if p['regla'] == 'corrige-decoracion' else 'seguro'
            filas.append([str(p['a']), p['viejo'], p['nuevo'], p['regla'], marca, str(p['tam']), str(p['testigos']),
                          fecha])
    anota(filas)
    print('APLICADOS %d de %d cambios en %s' % (len(filas), len(finales), ne.SYMBOLS))
    return 0


def consistentes(finales, viejos):
    """Quita las correcciones que, sin las descartadas, dejarian un nombre en dos direcciones."""
    while True:
        nombre = dict(viejos)
        dirs = collections.defaultdict(set)
        for a, n in nombre.items():
            dirs[n].add(a)
        malas = []
        for p in finales:
            if p['nuevo'] in dirs and dirs[p['nuevo']] and not p['nuevo'].startswith('sub_'):
                malas.append(p)
            dirs[nombre[p['a']]].discard(p['a'])
            nombre[p['a']] = p['nuevo']
            dirs[p['nuevo']].add(p['a'])
        if not malas:
            return finales
        print('  quitadas por quedar colgadas:', len(malas))
        finales = [p for p in finales if p not in malas]


def anota(filas_nuevas):
    filas = ne.lee_procedencia()
    filas += filas_nuevas
    ne.escribe_procedencia(filas)


def copias(aplicar, tsv):
    """Repaso de los nombres `copia-ambigua`: confirmados por destinos o por llamantes; corregidos (intercambio dentro
    del grupo de codigo igual) cuando los destinos o los llamantes contradicen el nombre y senalan a otro del grupo."""
    exe, est, us, primero = carga()
    nuestras = set()
    for u in us:
        nuestras.update(u.fs)
    fuertes = anclas_fuertes(us, est)
    ev = evidencia(us, est, fuertes, nuestras)
    por_dir = {}
    for u in us:
        for o, a in u.dir.items():
            por_dir[a] = (u, o)
    filas = ne.lee_procedencia()
    res = collections.Counter()
    salida = []
    nuevas_marcas = {}
    cambios = []
    for k, r in enumerate(filas):
        if r[4] != 'copia-ambigua':
            continue
        a, n = int(r[0]), r[2]
        if a not in por_dir or est.nombre.get(a) != n:
            res['ya no esta'] += 1
            continue
        u, o = por_dir[a]
        if n not in u.fs:
            res['sin la nuestra'] += 1
            continue
        alt = [m for m in u.fs if m != n and u.es[m] == u.eo[o]]
        contra, testigos = destinos(est, u.fo[o], u.fs[n])
        c = ev.get(a)
        motivo = None
        if contra == 0 and alt and all(destinos(est, u.fo[o], u.fs[m])[0] > 0 for m in alt):
            motivo = 'destinos'
        if contra == 0 and c and len(c) == 1 and n in c:
            motivo = (motivo + '+llamantes') if motivo else 'llamantes'
        if motivo:
            res['confirmada por ' + motivo] += 1
            nuevas_marcas[k] = motivo
            salida.append((u.nombre.split('/')[-1], 'confirmada', motivo, '0x%X' % a, n, r[5]))
            continue
        otro = None
        if c and len(c) == 1:
            m = next(iter(c))
            if m in alt and (c[m] >= MIN_LLAMANTES or destinos(est, u.fo[o], u.fs[m])[0] == 0):
                otro = (m, 'llamantes')
        if otro is None and contra:
            buenas = [m for m in alt if destinos(est, u.fo[o], u.fs[m])[0] == 0
                      and destinos(est, u.fo[o], u.fs[m])[1] > 0]
            if len(buenas) == 1:
                otro = (buenas[0], 'destinos')
        if otro:
            res['contradicha por ' + otro[1]] += 1
            cambios.append({'k': k, 'a': a, 'n': n, 'm': otro[0], 'motivo': otro[1], 'u': u, 'o': o})
            salida.append((u.nombre.split('/')[-1], 'contradicha', otro[1], '0x%X' % a, n, otro[0]))
            continue
        res['sigue ambigua'] += 1
    for x in sorted(res):
        print(' ', x, res[x])
    # cada intercambio pone m en a: m tiene que quedar libre (no puesto, o saliendo de su sitio en otro intercambio)
    while True:
        sale = set(x['n'] for x in cambios)
        entra = collections.Counter(x['m'] for x in cambios)
        ok = [x for x in cambios if entra[x['m']] == 1 and (not est.dirs.get(x['m']) or x['m'] in sale)]
        if len(ok) == len(cambios):
            break
        cambios = ok
    print('  intercambios coherentes: %d' % len(cambios))
    if tsv:
        with open(tsv, 'w', encoding='utf-8') as f:
            for x in salida:
                f.write('\t'.join(map(str, x)) + '\n')
    if not aplicar:
        return 0
    fecha = datetime.date.today().isoformat()
    for k, motivo in nuevas_marcas.items():
        filas[k][4] = 'seguro'
        filas[k][3] = filas[k][3] + '+' + motivo
    if cambios:
        # dos pasos para no pisar nombres: primero a un nombre temporal unico, luego al definitivo
        tmp = [(x['a'], x['n'], '%s__corrige_%08X' % (x['n'], x['a'])) for x in cambios]
        ne.aplica(tmp)
        fin = [(x['a'], '%s__corrige_%08X' % (x['n'], x['a']), x['m']) for x in cambios]
        hechos = set(ne.aplica(fin))
        for x in cambios:
            if (x['a'], '%s__corrige_%08X' % (x['n'], x['a']), x['m']) in hechos:
                filas.append([str(x['a']), x['n'], x['m'], 'copia-corregida+' + x['motivo'], 'seguro', filas[x['k']][5],
                              '0', fecha])
    ne.escribe_procedencia(filas)
    print('marcas cambiadas a seguro: %d; intercambios aplicados: %d' % (len(nuevas_marcas), len(cambios)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
