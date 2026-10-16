#!/usr/bin/env python3
"""name_corrections.py -- WRONG NAME rule for PC (SPEED_EXE_1_3), sibling of name_by_elimination.py.

English copy of the project's scripts/nombra_corrige.py: same code and same behaviour; only comments, documentation
and messages are translated. Identifiers inside the code, and the values it writes to config/SPEED_EXE_1_3/symbols.json
and config/SPEED_EXE_1_3/nombres_procedencia.tsv (the rule names below and the marks `seguro` and `decoracion`), are
kept as in the original: both versions write identical files and share the same provenance log. docs/PC_PDB.md
translates every value.

name_by_elimination.py and name_by_call_graph.py only give names to UNNAMED functions (sub_, IDA placeholders, GCC
decorations). But symbols.json carries FALSE names from its sources (IDA, upstream, the earlier PDB, old rounds): a
function called A whose code is that of our B. While it stays wrong, neither A nor B matches. This rule fixes them with
two independent kinds of proof, and ONLY when there is an alternative with strong proof:

  corrige-codigo  The exe function carries a name N whose code is NOT ours (or a name our objects do not define, or a
                  name REPEATED at several addresses), and its SKELETON (bytes with the relocations set to zero, the one
                  of name_by_call_graph) equals that of exactly ONE free function of ours in the unit, and vice versa
                  (1:1). Free = its name is not in the exe, or is only at addresses of the unit whose code contradicts
                  it. Same linker rules as name_by_elimination (first definer). Filters: no relocation target
                  contradicts (by ADDRESS, with the names already corrected); a function under 16 B with relocations
                  needs a witness; if the code of the old name is very SIMILAR (>= 0.9: a near-match of its own) a
                  witness or the callers' evidence is required; and the callers' evidence, if any, cannot say another
                  name.
  mueve-codigo    The same when the exe function had no name and the free name comes from another address.
  corrige-llamantes  CALLERS' evidence: each STRONG ANCHOR (a name that matches ours byte for byte and has a unique
                  skeleton in its unit, both in the exe and in our object) calls, at position i, an exe address A where
                  ours calls T. If at least 2 different strong anchors (or 1, if the code at A is also that of T) say T
                  and none says anything else, A is called T. T cannot already match at another address.
  nombra-llamantes  An UNNAMED function (sub_, IDA demangle) that the strong anchors call, unanimously, with a name of
                  ours from its unit that is not set anywhere, and whose code resembles ours (>= 0.6): a near-match (or
                  a copy that code alone cannot tell apart) that thereby shows up in the report.
  quita-duplicado / quita-falso  A name repeated in one unit with a single confirmed placement, or a name that moved
                  to its real place: the false placement goes back to sub_.
  corrige-decoracion  The special case where the old and the new name say the SAME function (same plain name and
                  scope) and only the decoration changes (class/struct, const, calling convention, `$09` -> `$0BE` of
                  the PC limits) or it is IDA's undecorated C name (`InitAnimCtrls` -> `?InitAnimCtrls@@YAXXZ`). It is
                  not a false name: it is the same function written as our source compiles it.

Fixed point: every accepted correction changes the state (the old name becomes free, the target of other pairs stops
contradicting) and everything is evaluated again until nothing comes out. Then objdiff: the names are changed in a COPY
of the split .obj and the code pair must give 100 %; caller pairs are not required to be at 100 % (their code may be a
near-match: the name is fixed for truthfulness, not for the percentage) but their percentage is reported.

AMBIGUOUS COPIES (--copies): reviews the names marked `copia-ambigua` in nombres_procedencia.tsv. These become `seguro`:
(a) those the TARGETS distinguish (the pair set has no contradictions, and each alternative of the identical-code group
does), or (b) those the strong CALLERS confirm. The one the strong callers contradict unanimously with a name of the
same group is corrected (swap within the group). Rule `copia-confirmada` / `copia-corregida`.

MANDATORY VALIDATION before applying:
  - code: 30 % of the strong anchors are hidden and their names SHUFFLED within each unit (each one receives the name
    of another, a deliberately false name); the script measures how many the rule recovers with THEIR name (99 %
    minimum).
  - callers: for the strong anchors with evidence at their address, how often the unanimous evidence says THEIR name
    (precision of the evidence; 99 % minimum).

PROVENANCE: every change goes to config/SPEED_EXE_1_3/nombres_procedencia.tsv with its rule and mark (`seguro` or
`decoracion`); `name_by_elimination.py --remove RULE` undoes them (restores the previous name). A name change may LOWER
a function that scores 100 % today because of a false name (copies): that is truthfulness, and it is reported apart.

    python scripts/name_corrections.py                      # reports (validation + objdiff), writes nothing
    python scripts/name_corrections.py --tsv OUT.tsv        # the proposed corrections
    python scripts/name_corrections.py --apply              # applies and logs the provenance
    python scripts/name_corrections.py --copies [--apply]   # review of the ambiguous copies
(the Spanish options of the original, --aplicar and --copias, also work)

After applying: rebuild PC (python -m ninja) and run name_by_elimination.py and name_by_call_graph.py (the old names
that were freed may have their place at another address). Round 39 (2026-10-07).
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
import name_by_elimination as ne  # noqa: E402
import name_by_call_graph as ng  # noqa: E402

DELINK = re.compile(r'^(.*)__delink_([0-9A-F]{8})$')
SUB = re.compile(r'^(?:sub|nullsub|j_sub|unknown_libname)_([0-9A-F]+)$')
PARECIDO_MAX = 0.9
PARECIDO_CASI = 0.6
MIN_LLAMANTES = 2
UMBRAL = {'codigo': 0.99, 'llamantes': 0.99}


def _english_args():
    """English aliases of the command-line options (the Spanish ones keep working), and --help."""
    alias = {'--apply': '--aplicar', '--copies': '--copias'}
    sys.argv[:] = [alias.get(a, a) for a in sys.argv]
    if '-h' in sys.argv or '--help' in sys.argv:
        print(__doc__)
        sys.exit(0)


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
    """True if both names say the same function with another decoration (or IDA's undecorated C name)."""
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
    """Current names of the exe by address (the functions list) and their placements; updated with every change."""

    def __init__(self, exe):
        d = json.load(open(ne.SYMBOLS, encoding='utf-8'))
        self.exe = exe
        self.nombre = {f['start']: f['name'] for f in d['functions']}
        self.fin = {f['start']: f['end'] for f in d['functions']}
        self.dirs = exe.dirs  # {name: set(addresses)}: functions + names (modified in place)
        self.desnombradas = set()  # addresses from which this rule removed a false name (back to sub_)

    def renombra(self, a, nuevo):
        viejo = self.nombre[a]
        self.dirs[viejo].discard(a)
        if not self.dirs[viejo]:
            del self.dirs[viejo]
        self.dirs.setdefault(nuevo, set()).add(a)
        self.nombre[a] = nuevo


class U:
    """A unit: functions of the original with their address, and ours."""

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
    """{address: (unit, obj, name)}: matches byte for byte and its skeleton is unique in the exe and in ours."""
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
    """{address: Counter(our name -> distinct strong anchors that call it there)}."""
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
    """(contradictions, witnesses), like name_by_elimination.destinos but with the names already corrected."""
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
    """(pool, freed) of the unit."""
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
                continue  # work for name_by_elimination
            if o not in u.unico_o or n not in u.unico_s:
                continue  # copies of identical code in the unit: a 1:1 between free ones proves nothing (--copies)
            a = u.dir[o]
            contra, testigos = destinos(est, u.fo[o], u.fs[n])
            if contra:
                continue
            tam = len(u.fo[o][0])
            if tam < ne.MIN_SIN_TESTIGO and u.fo[o][1] and not testigos:
                continue
            c = ev.get(a)
            if c and (len(c) != 1 or n not in c):
                continue  # the callers say something else
            par = parecido(u.eo[o], u.es[b]) if e == 'contradice' else 0.0
            if par >= PARECIDO_MAX and not testigos and not c:
                continue
            regla = 'mueve-codigo' if e == 'sinnombre' else 'corrige-codigo'
            out.append(propuesta(u, o, est, n, regla, tam, testigos + (c[n] if c else 0), par, True))
    return out


def propone_llamantes(us, est, primero, ev, fuertes):
    """Named addresses whose name contradicts the unanimous evidence of the strong anchors."""
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
            continue  # unnamed: not a correction
        if est.dirs.get(n):
            continue  # the name is set at another address: it will move once it is free (next pass)
        igual = n in u.fs and u.es[n] == u.eo[o]
        if k < MIN_LLAMANTES and not igual:
            continue
        if primero.get(n) is not None and primero.get(n) != u.nombre:
            continue  # the COMDAT goes with its first definer
        out.append(propuesta(u, o, est, n, 'corrige-llamantes', len(u.fo[o][0]), k, 1.0 if igual else 0.0, igual))
    return out


def propone_casi(us, est, primero, ev, fuertes):
    """nombra-llamantes: an UNNAMED function (sub_, IDA demangle, GCC decoration) whose code is not identical to ours
    (that is why name_by_elimination/name_by_call_graph do not touch it) but which the strong anchors call
    unanimously with a name of ours from its unit that is not set anywhere. It is a near-match: the name makes it
    visible in the report. The code must resemble ours (>= PARECIDO_CASI)."""
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
    """A name repeated at several addresses of the SAME unit (a unit is a single linked .cpp: a name cannot be there
    twice) with ONE confirmed placement (code identical to ours, or the callers): the others are false and lose the
    name (back to sub_)."""
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
    """A new name only once and an address only once per pass."""
    usado_n = collections.Counter(p['nuevo'] for p in props)
    usado_a = collections.Counter(p['a'] for p in props)
    return [p for p in props if usado_n[p['nuevo']] == 1 and usado_a[p['a']] == 1]


def punto_fijo(exe, est, us, primero, llamantes=True, verificar=False):
    """Passes until nothing comes out. With `verificar`, each pass goes through objdiff (with all the changes of the
    unit so far): a CODE pair that does not give 100 % is undone and not proposed again."""
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
        # the same pair can come out by code and by callers: only once (code first)
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
            # a freed name leaves the addresses where it was wrongly set: they go back to sub_ (and may receive
            # their real name in another pass)
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
                print('    dropped by objdiff:', hex(p['a']), p['viejo'][:60], '->', p['nuevo'][:60], p['pct'])
            nuevas = [p for p in nuevas if p not in malas]
        aceptadas += nuevas
        print('  pass %d: %d corrections' % (vuelta + 1, len(nuevas)))
    return aceptadas


def valida_codigo(fraccion=0.3, semilla=11):
    """Hides 30 % of the anchors by shuffling their names within the unit (each one receives the name of another) and
    measures how many come back with THEIR name."""
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
    """{address: percentage} with objdiff on a copy of each unit's split .obj with ALL the names changed in it so far
    (a swap is only visible with both sides renamed). In the split, an address may carry two symbols (the one of
    `functions` and the one of `names`): the one of `functions` is renamed."""
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
            print('objdiff failed in', un, r.stderr[-300:])
        for q in (t, sal):
            if os.path.exists(q):
                os.remove(q)
    try:
        os.rmdir(ne.TMP)
    except OSError:
        pass
    return out


def main():
    _english_args()
    args = sys.argv[1:]
    if '--copias' in args:
        return copias('--aplicar' in args, args[args.index('--tsv') + 1] if '--tsv' in args else None)
    print('CODE VALIDATION (hidden anchors with shuffled names):')
    n, bien, mal, malas, fuera = valida_codigo()
    tasa_c = bien / (bien + mal) if bien + mal else 1.0
    print('  hidden %d  recovered right %d  wrong %d  accuracy %.4f (minimum %.2f); changes outside the hidden ones %d'
          % (n, bien, mal, tasa_c, UMBRAL['codigo'], fuera))
    for x in malas[:10]:
        print('    WRONG', x)
    print('CALLERS VALIDATION (unanimous evidence at the address of each strong anchor):')
    b2, m2, malas2 = valida_llamantes()
    tasa_l = b2 / (b2 + m2) if b2 + m2 else 1.0
    print('  right %d  wrong %d  accuracy %.4f (minimum %.2f)' % (b2, m2, tasa_l, UMBRAL['llamantes']))
    for x in malas2[:10]:
        print('    WRONG', x)

    exe, est, us, primero = carga()
    viejos = dict(est.nombre)
    print('REAL PASS')
    acept = punto_fijo(exe, est, us, primero, verificar=True)
    # the result is the final state: per address, the name in symbols.json against the current one (an address may have
    # gone through sub_ in one pass and received its name in another); the rule is the one of the last step
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
        cnt[(p['regla'], 'changes')] += 1
        cnt[(p['regla'], 'B')] += p['tam']
        if p['antes'] == 'casa':
            cnt[(p['regla'], 'at 100 % today with the old name')] += 1
        if p['pct'] == 100.0:
            cnt[(p['regla'], 'objdiff 100 with the new name')] += 1
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
        print('NOT APPLIED: the validation does not reach the minimum')
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
    print('APPLIED %d of %d changes in %s' % (len(filas), len(finales), ne.SYMBOLS))
    return 0


def consistentes(finales, viejos):
    """Removes the corrections that, without the dropped ones, would leave a name at two addresses."""
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
        print('  removed for being left dangling:', len(malas))
        finales = [p for p in finales if p not in malas]


def anota(filas_nuevas):
    filas = ne.lee_procedencia()
    filas += filas_nuevas
    ne.escribe_procedencia(filas)


def copias(aplicar, tsv):
    """Review of the `copia-ambigua` names: confirmed by targets or by callers; corrected (swap within the group of
    identical code) when the targets or the callers contradict the name and point to another one of the group."""
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
            res['no longer there'] += 1
            continue
        u, o = por_dir[a]
        if n not in u.fs:
            res['without ours'] += 1
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
            res['confirmed by ' + motivo] += 1
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
            res['contradicted by ' + otro[1]] += 1
            cambios.append({'k': k, 'a': a, 'n': n, 'm': otro[0], 'motivo': otro[1], 'u': u, 'o': o})
            salida.append((u.nombre.split('/')[-1], 'contradicha', otro[1], '0x%X' % a, n, otro[0]))
            continue
        res['still ambiguous'] += 1
    for x in sorted(res):
        print(' ', x, res[x])
    # each swap puts m at a: m must be left free (not set, or leaving its place in another swap)
    while True:
        sale = set(x['n'] for x in cambios)
        entra = collections.Counter(x['m'] for x in cambios)
        ok = [x for x in cambios if entra[x['m']] == 1 and (not est.dirs.get(x['m']) or x['m'] in sale)]
        if len(ok) == len(cambios):
            break
        cambios = ok
    print('  consistent swaps: %d' % len(cambios))
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
        # two steps so as not to overwrite names: first to a unique temporary name, then to the final one
        tmp = [(x['a'], x['n'], '%s__corrige_%08X' % (x['n'], x['a'])) for x in cambios]
        ne.aplica(tmp)
        fin = [(x['a'], '%s__corrige_%08X' % (x['n'], x['a']), x['m']) for x in cambios]
        hechos = set(ne.aplica(fin))
        for x in cambios:
            if (x['a'], '%s__corrige_%08X' % (x['n'], x['a']), x['m']) in hechos:
                filas.append([str(x['a']), x['n'], x['m'], 'copia-corregida+' + x['motivo'], 'seguro', filas[x['k']][5],
                              '0', fecha])
    ne.escribe_procedencia(filas)
    print('marks changed to seguro (confirmed): %d; swaps applied: %d' % (len(nuevas_marcas), len(cambios)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
