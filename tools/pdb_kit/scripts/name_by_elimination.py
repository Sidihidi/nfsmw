#!/usr/bin/env python3
"""name_by_elimination.py -- PC (SPEED_EXE_1_3) names by ELIMINATION and by FIRST DEFINER, with mandatory validation
and a provenance log.

English copy of the project's scripts/nombra_elim.py: same code and same behaviour; only comments, documentation and
messages are translated. Identifiers inside the code, and the values it writes to config/SPEED_EXE_1_3/symbols.json and
config/SPEED_EXE_1_3/nombres_procedencia.tsv (rule names `elim-1:1` and `elim-orden`, marks `seguro` and
`copia-ambigua`, the header line of the log), are kept as in the original: both versions write identical files and
share the same provenance log. docs/PC_PDB.md translates every value.

Why it exists: scripts/nombra_grafo.py (English copy: name_by_call_graph.py) was exhausted on PC (+9 in round 38),
leaving thousands of unnamed functions whose code is IDENTICAL to one of ours. It lacked three rules:
  1. MSVC's linker places a COMDAT with the FIRST object that contributes it (link order = address order of the
     units). One of our functions can only be in the unit that defines it first (98.6 % of the anchors with several
     definitions follow it).
  2. A function of ours whose name is ALREADY in symbols.json is not a candidate anywhere (it is at another address).
  3. Relocation targets are compared by ADDRESS, not by name: IDA gives one address two names (`??3@YAXPAX@Z` and
     `?Free@FastMem@@...`), and by name they seemed to contradict each other.

Pool of unit U: functions of our U.obj (build/SPEED_EXE_1_3/src) with a name that is not in symbols.json and whose
first definer is U. Target: functions of the split U.obj (build/SPEED_EXE_1_3/obj) WITHOUT a name MSVC could emit
(sub_, nullsub_, unknown_libname_, IDA's `Class::Method`, GCC decorations, names with the address).
They are grouped by SKELETON (bytes with the relocations set to zero, the one of nombra_grafo). Rules:
  elim-1:1     the group has one function of the original and one of ours.
  elim-orden   the group has N and N (copies of identical code): they are assigned in order (address in the exe
               against definition order in our .obj) and ONLY if each one falls between its neighbouring anchors.
               These are "ambiguous copy" names: the code is identical, the name may be swapped with another copy
               (~3 % by validation).
Filters of each pair:
  - no target contradicts (different addresses, or an __real@ constant / ??_C@ string that does not say what is in
    the exe);
  - a function under 16 B with relocations needs a witness (a confirmed target): small wrappers (push; call X; ret)
    have the same skeleton for any X;
  - elim-orden also needs to be between its neighbouring anchors.
Then objdiff: the names are changed in a COPY of the split .obj and `objdiff-cli diff` (the report's options) must give
100 % to each pair; a pair that does not is dropped.

MANDATORY VALIDATION: before applying, 30 % of the anchors (name already set and identical code) are hidden and the
script measures how many each rule recovers with THEIR name. If elim-1:1 falls below 99 % or elim-orden below 95 %,
nothing is applied.

PROVENANCE: every applied name is logged in config/SPEED_EXE_1_3/nombres_procedencia.tsv (address, previous name, new
name, rule, mark, bytes, witnesses, date). The mark `copia-ambigua` (elim-orden) allows reviewing them or removing them
all at once with --remove, which restores the previous name.

    python scripts/name_by_elimination.py                          # reports (with validation and objdiff), writes nothing
    python scripts/name_by_elimination.py --apply                  # applies to symbols.json and logs the provenance
    python scripts/name_by_elimination.py --apply --only-1to1      # only the 1:1 rule
    python scripts/name_by_elimination.py --remove copia-ambigua   # undoes the names with that mark (or --remove elim-1:1)
    python scripts/name_by_elimination.py --tsv OUT.tsv            # the proposed pairs, for review
(the Spanish options of the original, --aplicar, --solo-1a1 and --quitar, also work)

It needs up-to-date objects (the split ones in build/SPEED_EXE_1_3/obj after the last change of symbols.json, and ours
compiled): rebuild PC first (python -m ninja). Repeat it after each PC round, after name_by_call_graph.py: every new
function that compiles identically is one more name.
Round 38 (2026-10-07), first pass: see docs/PC_PDB.md.
FALSE names already set (this rule only touches unnamed functions) are fixed by name_corrections.py (round 39), which
also reviews the ambiguous copies (--copies). Routine order after each PC round: rebuild PC,
name_corrections.py --apply, name_corrections.py --copies --apply, rebuild PC, name_by_call_graph.py SPEED_EXE_1_3
--apply, name_by_elimination.py --apply, rebuild PC (and repeat while something comes out).
"""
import bisect
import collections
import datetime
import json
import os
import random
import re
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import name_by_call_graph as ng  # noqa: E402

V = 'SPEED_EXE_1_3'
CONFIG = os.path.join(RAIZ, 'config', V)
SYMBOLS = os.path.join(CONFIG, 'symbols.json')
PROCEDENCIA = os.path.join(CONFIG, 'nombres_procedencia.tsv')
OBJDIFF = os.path.join(RAIZ, 'build', 'tools', 'objdiff-cli.exe')
TMP = os.path.join(RAIZ, 'build', V, 'nombra_elim_tmp')
EH = ('__unwindfunclet$', '__ehhandler$', '__catch$', '__tryblock')
MARCA = {'elim-1:1': 'seguro', 'elim-orden': 'copia-ambigua'}
MIN_SIN_TESTIGO = 16
UMBRAL_VALIDACION = {'elim-1:1': 0.99, 'elim-orden': 0.95}

# names MSVC 7.1 cannot emit: IDA placeholders, IDA demangles, GCC decorations, names with the address
_IDA = re.compile(r'^(sub|nullsub|j_sub|j|unknown_libname|loc|jpt|SEH|lpStartAddress|lpfnCallback|StartAddress)(_|$)'
                  r'|^Unwind@|^FID_conflict:|::|\(|\s|__[0-9A-F]{8}$|_[0-9a-f]{8}$|__delink_')
_GCC = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*__(?:\d|F|C|Q\d|t\d|H\d)|^_\$_|^__ct__|^__dt__')
_DUP = re.compile(r'^\?.*@[A-Z0-9]*Z_\d+$|^\?.*_\d+_SEH$')


def _english_args():
    """English aliases of the command-line options (the Spanish ones keep working), and --help."""
    alias = {'--apply': '--aplicar', '--only-1to1': '--solo-1a1', '--remove': '--quitar'}
    sys.argv[:] = [alias.get(a, a) for a in sys.argv]
    if '-h' in sys.argv or '--help' in sys.argv:
        print(__doc__)
        sys.exit(0)


def valido(n):
    """True if the name can come out of MSVC (a `?...` decoration or a C name)."""
    if _IDA.search(n) or _GCC.search(n) or _DUP.search(n):
        return False
    return n.startswith('?') or bool(re.match(r'^[A-Za-z_@][A-Za-z0-9_@$?]*$', n))


class Exe:
    """symbols.json, unit ranges and data bytes of the exe (for the constants)."""

    def __init__(self):
        d = json.load(open(SYMBOLS, encoding='utf-8'))
        self.tamanos = {}
        self.dirs = collections.defaultdict(set)
        for f in d['functions']:
            self.tamanos[f['name']] = f['end'] - f['start']
            self.dirs[f['name']].add(f['start'])
        for x in d['names']:
            self.dirs[x['name']].add(x['addr'])
        s = json.load(open(os.path.join(CONFIG, 'splits.json'), encoding='utf-8'))
        unidades = []
        self.rangos = {}
        for u, v in s.items():
            rs = v.get('function_ranges') or []
            if rs:
                unidades.append((min(r[0] for r in rs), u[:-4]))
                self.rangos[u[:-4]] = [tuple(r) for r in rs]
        self.unidades = [u for _, u in sorted(unidades)]  # link order
        pc = ng.PC.__new__(ng.PC)
        self.pc = pc
        # the data memory, as in name_by_call_graph.PC
        import struct
        pe = open(os.path.join(RAIZ, 'orig', V, 'speed.exe'), 'rb').read()
        e = struct.unpack_from('<I', pe, 0x3c)[0]
        nsec = struct.unpack_from('<H', pe, e + 6)[0]
        opt = struct.unpack_from('<H', pe, e + 20)[0]
        base = struct.unpack_from('<I', pe, e + 24 + 28)[0]
        trozos = []
        for i in range(nsec):
            nombre, vsz, va, rsz, raw = struct.unpack_from('<8sIIII', pe, e + 24 + opt + i * 40)
            if nombre.rstrip(b'\0') in (b'.rdata', b'.data'):
                trozos.append((base + va, pe[raw:raw + min(rsz, vsz)]))
        self.memoria = ng.Memoria(trozos)

    def dir_unica(self, n):
        ds = self.dirs.get(n)
        return next(iter(ds)) if ds and len(ds) == 1 else None


class Unidad:
    def __init__(self, exe, nombre):
        self.nombre = nombre
        po = os.path.join(RAIZ, 'build', V, 'obj', nombre + '.obj')
        ps = os.path.join(RAIZ, 'build', V, 'src', nombre + '.obj')
        self.ok = os.path.exists(po) and os.path.exists(ps)
        if not self.ok:
            return
        self.po, self.ps = po, ps
        self.fo = ng.Coff(po).funciones(exe.tamanos)
        cs = ng.Coff(ps)
        self.fs = cs.funciones()
        self.indice = {}
        for k, s in enumerate(cs.lista):
            if s[0] in self.fs:
                self.indice.setdefault(s[0], k)
        self.eo = {n: exe.pc.esqueleto(*v) for n, v in self.fo.items()}
        self.es = {n: exe.pc.esqueleto(*v) for n, v in self.fs.items()}
        rs = exe.rangos[nombre]
        self.va = {}
        for n in self.fo:
            ds = [a for a in exe.dirs.get(n, ()) if any(i <= a < f for i, f in rs)]
            self.va[n] = ds[0] if len(ds) == 1 else None


def destinos(exe, vo, vs):
    """(contradictions, witnesses) of the relocations of a pair with the same skeleton."""
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
            continue  # ours points to symbol+k: cannot decide
        ao, as_ = exe.dir_unica(no), exe.dir_unica(ns)
        if ao is not None and as_ is not None:
            if ao == as_:
                testigos += 1
            else:
                contra += 1
            continue
        if ao is not None:
            v = exe.memoria.comprueba(ao, ns)
            if v is True:
                testigos += 1
            elif v is False:
                contra += 1
    return contra, testigos


def corre(exe, unidades, escondidas):
    """[(unit, rule, orig, ours, bytes, witnesses)]. escondidas: {unit: {name}} anchors treated as unnamed
    (validation)."""
    nombres_exe = set(exe.dirs)
    for u, ns in escondidas.items():
        nombres_exe -= ns
    primero = {}
    for u in unidades:
        for n in u.fs:
            primero.setdefault(n, u.nombre)
    out = []
    for u in unidades:
        esc = escondidas.get(u.nombre, set())
        # anchors: name on both sides (without the hidden ones), for the order gate
        anclas = sorted((u.va[n], u.indice[n]) for n in u.fo if n in u.fs and n not in esc and u.va.get(n) is not None
                        and n in u.indice)

        def en_orden(o, n):
            a = u.va.get(o)
            if a is None:
                return False
            k = bisect.bisect_left(anclas, (a, -1))
            antes = [x[1] for x in anclas[max(0, k - 2):k]]
            despues = [x[1] for x in anclas[k:k + 2]]
            i = u.indice.get(n, -1)
            return (not antes or i > min(antes)) and (not despues or i < max(despues))

        pool = [n for n in u.fs if n not in nombres_exe and primero.get(n) == u.nombre and valido(n)
                and not n.startswith(EH) and len(n) <= 2000]
        g = collections.defaultdict(lambda: ([], []))
        for n in pool:
            g[u.es[n]][1].append(n)
        for o in u.fo:
            if (not valido(o) or o in esc) and u.eo[o] in g and u.va.get(o) is not None:
                g[u.eo[o]][0].append(o)
        for e, (os_, ns) in g.items():
            if not os_ or not ns or len(os_) != len(ns):
                continue
            if len(os_) == 1:
                parejas, regla = [(os_[0], ns[0])], 'elim-1:1'
            else:
                a = sorted(os_, key=lambda x: u.va[x])
                b = sorted(ns, key=lambda x: u.indice.get(x, 1 << 30))
                parejas, regla = list(zip(a, b)), 'elim-orden'
            for o, n in parejas:
                contra, testigos = destinos(exe, u.fo[o], u.fs[n])
                if contra:
                    continue
                tam = len(u.fo[o][0])
                if tam < MIN_SIN_TESTIGO and u.fo[o][1] and not testigos:
                    continue
                if regla == 'elim-orden' and not en_orden(o, n):
                    continue
                out.append((u.nombre, regla, o, n, tam, testigos))
    return out


def valida(exe, unidades, fraccion=0.3, semilla=7):
    random.seed(semilla)
    esc = {}
    for u in unidades:
        cand = [n for n in u.fo if n in u.fs and valido(n) and u.eo[n] == u.es[n] and u.va.get(n) is not None]
        esc[u.nombre] = set(x for x in cand if random.random() < fraccion)
    res = corre(exe, unidades, esc)
    cnt = collections.Counter()
    malas = []
    for un, regla, o, n, t, w in res:
        if o in esc[un]:
            ok = ng.equivalentes(o, n)
            cnt[(regla, ok)] += 1
            if not ok:
                malas.append((un, regla, o, n))
    return cnt, malas, sum(len(x) for x in esc.values())


def renombra_coff(datos, cambios):
    """COFF bytes with the symbols of `cambios` {old: new} renamed (new string at the end)."""
    import struct
    d = bytearray(datos)
    _, nsec, _, psym, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
    cad = psym + nsym * 18
    tam_cad = struct.unpack_from('<I', d, cad)[0]
    assert cad + tam_cad == len(d), 'the string table is not the last thing in the file'
    extra = bytearray()
    hechos = set()
    i = 0
    while i < nsym:
        o = psym + i * 18
        n = bytes(d[o:o + 8])
        if n[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', n, 4)[0]
            nombre = bytes(d[cad + off:d.index(b'\0', cad + off)])
        else:
            nombre = n.rstrip(b'\0')
        nombre = nombre.decode('latin-1')
        if nombre in cambios and nombre not in hechos:
            off = tam_cad + len(extra)
            extra += cambios[nombre].encode('latin-1') + b'\0'
            struct.pack_into('<II', d, o, 0, off)
            hechos.add(nombre)
        i += 1 + d[o + 17]
    d += extra
    struct.pack_into('<I', d, cad, tam_cad + len(extra))
    return bytes(d), hechos


def verifica_objdiff(unidades, parejas):
    """{(unit, orig): percentage} with objdiff on a renamed copy of the split .obj."""
    os.makedirs(TMP, exist_ok=True)
    por_u = collections.defaultdict(dict)
    for un, regla, o, n, t, w in parejas:
        por_u[un][o] = n
    um = {u.nombre: u for u in unidades}
    out = {}
    for un, cambios in por_u.items():
        u = um[un]
        datos, hechos = renombra_coff(open(u.po, 'rb').read(), cambios)
        t = os.path.join(TMP, os.path.basename(un) + '.obj')
        sal = os.path.join(TMP, os.path.basename(un) + '.json')
        open(t, 'wb').write(datos)
        r = subprocess.run([OBJDIFF, 'diff', '-1', t, '-2', u.ps, '-c', 'functionRelocDiffs=none', '-o', sal,
                            '--format', 'json'], capture_output=True, text=True)
        if r.returncode == 0:
            d = json.load(open(sal, encoding='utf-8'))
            pct = {s.get('name'): s.get('match_percent') for s in d['left']['symbols']
                   if s.get('kind') == 'SYMBOL_FUNCTION'}
            for o, n in cambios.items():
                out[(un, o)] = pct.get(n) if o in hechos else None
        else:
            print('objdiff failed in', un, r.stderr[-300:])
        for q in (t, sal):
            if os.path.exists(q):
                os.remove(q)
    try:
        os.rmdir(TMP)
    except OSError:
        pass
    return out


def lee_procedencia():
    if not os.path.exists(PROCEDENCIA):
        return []
    filas = []
    for l in open(PROCEDENCIA, encoding='utf-8'):
        if l.startswith('#') or not l.strip():
            continue
        filas.append(l.rstrip('\n').split('\t'))
    return filas


# the header of the provenance log is kept in Spanish, byte for byte as the original writes it: both versions share
# the file. Its columns: address, previous name, new name, rule, mark, bytes, witnesses, date.
CABECERA = ('# Nombres de PC puestos por scripts/nombra_elim.py (no los borres a mano: usa --quitar MARCA).\n'
            '# direccion\tnombre_anterior\tnombre_nuevo\tregla\tmarca\tbytes\ttestigos\tfecha\n')


def escribe_procedencia(filas):
    with open(PROCEDENCIA, 'w', encoding='utf-8', newline='\n') as f:
        f.write(CABECERA)
        for r in filas:
            f.write('\t'.join(r) + '\n')


_ENTRADA = re.compile(r'"(start|addr)": (\d+),(\s*"end": \d+,)?\s*"name": "((?:[^"\\]|\\.)*)"')


def nombres_por_direccion(texto):
    """{address: name} of the `functions` list (what objdiff compares)."""
    out = {}
    for m in _ENTRADA.finditer(texto):
        if m.group(1) == 'start':
            out[int(m.group(2))] = json.loads('"%s"' % m.group(4))
    return out


def aplica(cambios):
    """cambios [(address, old, new)] -> [the applied ones]. A single pass over the text: it changes the name in the
    `functions` entry (start) and in the `names` entry (addr) of that address ONLY if the old name is there, so it is
    idempotent (if it already has the new one, it does nothing). It does not rewrite the JSON: the rest of the file
    stays the same."""
    texto = open(SYMBOLS, 'rb').read().decode('utf-8')
    por_dir = {a: (viejo, nuevo) for a, viejo, nuevo in cambios}
    trozos, ultimo, hechos = [], 0, set()
    for m in _ENTRADA.finditer(texto):
        a = int(m.group(2))
        if a not in por_dir:
            continue
        viejo, nuevo = por_dir[a]
        if json.loads('"%s"' % m.group(4)) != viejo:
            continue
        trozos.append(texto[ultimo:m.start(4)])
        trozos.append(json.dumps(nuevo)[1:-1])
        ultimo = m.end(4)
        if m.group(1) == 'start':
            hechos.add(a)
    trozos.append(texto[ultimo:])
    open(SYMBOLS, 'wb').write(''.join(trozos).encode('utf-8'))
    return [(a, v, n) for a, v, n in cambios if a in hechos]


def quitar(marca):
    filas = lee_procedencia()
    deshacer = [(int(r[0]), r[2], r[1]) for r in filas if marca in (r[3], r[4])]
    hechos = set(aplica(deshacer))
    quedan = [r for r in filas if not (marca in (r[3], r[4]) and (int(r[0]), r[2], r[1]) in hechos)]
    escribe_procedencia(quedan)
    print('undone %d of %d names with the mark %s' % (len(hechos), len(deshacer), marca))


def main():
    _english_args()
    if '--quitar' in sys.argv:
        quitar(sys.argv[sys.argv.index('--quitar') + 1])
        return 0
    exe = Exe()
    unidades = [u for u in (Unidad(exe, n) for n in exe.unidades) if u.ok]
    print('units with both objects:', len(unidades))
    cnt, malas, n_esc = valida(exe, unidades)
    ok = True
    print('VALIDATION (30 %% of the anchors hidden: %d)' % n_esc)
    for regla in ('elim-1:1', 'elim-orden'):
        b, m = cnt[(regla, True)], cnt[(regla, False)]
        tasa = b / (b + m) if b + m else 1.0
        print('  %-11s right %5d  wrong %4d  accuracy %.4f (minimum %.2f)' % (regla, b, m, tasa,
                                                                              UMBRAL_VALIDACION[regla]))
        if tasa < UMBRAL_VALIDACION[regla]:
            ok = False
    for x in malas[:10]:
        print('    WRONG', x[0], x[1], x[2][:60], '->', x[3][:60])
    res = corre(exe, unidades, {})
    if '--solo-1a1' in sys.argv:
        res = [r for r in res if r[1] == 'elim-1:1']
    pct = verifica_objdiff(unidades, res)
    finales = [r for r in res if pct.get((r[0], r[2])) == 100.0]
    c = collections.Counter()
    for r in res:
        k = 'objdiff 100' if pct.get((r[0], r[2])) == 100.0 else 'dropped by objdiff'
        c[(r[1], k)] += 1
        c[(r[1], k, 'B')] += r[4]
    for k in sorted(c, key=str):
        print(' ', k, c[k])
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8') as f:
            for r in finales:
                f.write('\t'.join(map(str, r)) + '\n')
    if '--aplicar' not in sys.argv:
        return 0
    if not ok:
        print('NOT APPLIED: the validation does not reach the minimum')
        return 1
    um = {u.nombre: u for u in unidades}
    cambios = [(um[r[0]].va[r[2]], r[2], r[3]) for r in finales]
    hechos = aplica(cambios)
    fecha = datetime.date.today().isoformat()
    meta = {(um[r[0]].va[r[2]], r[2], r[3]): r for r in finales}
    filas = lee_procedencia()
    for a, viejo, nuevo in hechos:
        r = meta[(a, viejo, nuevo)]
        filas.append([str(a), viejo, nuevo, r[1], MARCA[r[1]], str(r[4]), str(r[5]), fecha])
    escribe_procedencia(filas)
    print('APPLIED %d of %d names in %s; provenance in %s' % (len(hechos), len(cambios), SYMBOLS, PROCEDENCIA))
    return 0


if __name__ == '__main__':
    sys.exit(main())
