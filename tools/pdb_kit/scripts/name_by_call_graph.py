#!/usr/bin/env python3
"""name_by_call_graph.py -- Xbox 360 and PC names by IDENTICAL CODE and by the reference graph (CALLERS).

English copy of the project's scripts/nombra_grafo.py: same code and same behaviour; only comments, documentation and
messages are translated. Identifiers inside the code, and the values it writes (rule names such as `unica`,
`llamantes`, `secuencia`, `mejor`), are kept as in the original so that both versions produce identical files.

objdiff pairs functions by name, and the progress report (scripts/pctall.py) is generated with
`functionRelocDiffs=none`: the TARGET of a relocation does not count, only the instruction. So an unnamed function of
the original (fn_ on 360, sub_ on PC) matches as soon as it carries the name of one of our functions with the same
SKELETON (the code with the relocated field set to zero). tools/nombra_por_codigo.py and round 12 only named the ones
that were unique and also in order, or whose text was identical including the targets; two groups were left:
  - functions whose code is identical except for UNNAMED targets in the original (fn_/lbl_/sub_/dword_...);
  - functions with the same skeleton as SEVERAL of ours (Lerp/VU0_v3lerp, reserve/push_back of each Vector<T>...).

What makes each name verifiable is the relocation targets:
  - a named target in the original must have the same name in ours (otherwise the pair is FALSE); the constants
    `__real@XXXXXXXX` and the strings `??_C@_0...` are checked against the BYTES of the original, because the data
    names of the configuration are not reliable (on 360, `__real@00000000_4` holds 1.0f);
  - an unnamed target receives EVIDENCE: in an already accepted pair, the k-th call of the original and of ours go to
    the same place (only if both point to the start of the symbol);
  - the anchors (same name and same skeleton on both sides) give the initial evidence; every accepted pair adds more.
An unnamed function is named if (a) it is the ONLY compatible pair, in both directions, of its skeleton group
(`unica`), (b) the callers name it: all the evidence at its address says that name (`llamantes`; `secuencia` when it
comes from aligning the calls of named functions that do not match yet), or (c) among several compatible ones it is
the only one with the most confirmed targets, and it is also the best in the other direction (`mejor`). Fixed point,
and a second reading with all the names decided drops the pairs that contradict each other. A short function without
any target confirming it must also fall between its neighbouring anchors in the order of our object.

Differences between versions:
  - 360 (EUROPEGERMILESTONE): linked with LTCG; it carries per-unit copies of inline functions (?Normalize@UMath
    appears twice, byte for byte), so a name may repeat in different units, never twice in one.
    Applies by renaming `fn_XXXXXXXX = .text:` lines of symbols.txt.
  - PC (SPEED_EXE_1_3): each COMDAT is present once; a name cannot already be in symbols.json. Besides the
    sub_/nullsub_/unknown_libname_, the names MSVC 7.1 cannot emit (IDA demangles such as `FEObject::FEObject`, GCC
    decorations brought from GameCube such as `Default__14PlayerSettings`, `name__0061CD50`) are replaced, but ONLY by
    ours when it says the same function (same name, and same scopes or class): never IDA's false library signatures
    (`Concurrency::details::...`). The original's function measures what symbols.json says (as objdiff compares it).
    Applies by changing the name field with an exact text substitution (it does not rewrite the JSON).
Only functions of the units with an object of ours (the SourceLists) are named; never data, no symbols are created.

    python scripts/name_by_call_graph.py EUROPEGERMILESTONE|SPEED_EXE_1_3 [--tsv OUT.tsv] [--apply] [--detail]
                                         [--bad-anchors] [--validate FRACTION] [--ours=DIR]
(the Spanish options of the original, --aplicar, --detalle, --anclas-malas, --validar and --nuestros=, also work)

--ours=DIR takes our objects from DIR instead of build/<V>/src. On 360 the official measurement comes from the LTCG link
(scripts/ltcg360.py): `--ours=build/EUROPEGERMILESTONE/ltcg/obj` names against the linked code, which is what gets
compared. Round 39: 361 names (28 kB) that did not come out against the file-by-file objects; --validate 0.3 with those
objects: 811 of 811 right.
--bad-anchors lists the functions ALREADY named whose code is identical to ours but with a named target that
contradicts ours: inherited names that may be wrong (the report still scores them at 100 %).
--validate F hides a fraction F of the anchors and counts how many it recovers with THEIR name (it applies nothing).
Round 13, with 0.3: 360 904 of 905 right (the failure: two interface destructors with the same code), PC 2,546 of 2,546.
Round 13 (2026-09-30), repeated until nothing came out: 360 +828 functions (+ 3 names of old access that PC confirms),
PC +2,467 (of 2,475 names: 8 zLua functions whose IDA size was inflated by padding); 0 losses.
Repeat it after each batch: every new function that compiles identically is one more name, and every new name is an
anchor.
"""
import bisect
import collections
import glob
import json
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAX_NOMBRE = 250  # 360: jeff does not accept `filename:` in the attributes; a longer name cannot be set

# COFF
PPC_REL24, PPC_REFHI, PPC_REFLO, PPC_PAIR, PPC_ADDR32 = 6, 0x10, 0x11, 0x12, 2
I386_DIR32, I386_REL32 = 6, 0x14


def _english_args():
    """English aliases of the command-line options (the Spanish ones keep working), and --help."""
    alias = {'--apply': '--aplicar', '--detail': '--detalle', '--bad-anchors': '--anclas-malas',
             '--validate': '--validar'}
    out = []
    for a in sys.argv:
        if a.startswith('--ours='):
            a = '--nuestros=' + a.split('=', 1)[1]
        out.append(alias.get(a, a))
    sys.argv[:] = out
    if '-h' in out or '--help' in out:
        print(__doc__)
        sys.exit(0)


class Coff:
    def __init__(self, ruta):
        d = open(ruta, 'rb').read()
        self.d = d
        maquina, nsec, _, psym, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
        assert maquina in (0x01F2, 0x014C), ruta
        self.ppc = maquina == 0x01F2
        self.secciones = []
        for i in range(nsec):
            o = 20 + opt + i * 40
            nombre, vsz, va, rsz, praw, prel, plin, nrel, nlin, car = struct.unpack_from('<8sIIIIIIHHI', d, o)
            if nrel == 0xFFFF and car & 0x01000000:  # IMAGE_SCN_LNK_NRELOC_OVFL
                nrel = struct.unpack_from('<I', d, prel)[0] - 1
                prel += 10
            relocs = [struct.unpack_from('<IIH', d, prel + k * 10) for k in range(nrel)]
            self.secciones.append({'tam': rsz, 'praw': praw, 'relocs': relocs, 'car': car})
        cad = psym + nsym * 18
        self.simbolos = {}
        self.lista = []
        i = 0
        while i < nsym:
            o = psym + i * 18
            n = d[o:o + 8]
            if n[:4] == b'\0\0\0\0':
                off = struct.unpack_from('<I', n, 4)[0]
                n = d[cad + off:d.index(b'\0', cad + off)]
            else:
                n = n.rstrip(b'\0')
            valor, sec, tipo, clase, naux = struct.unpack_from('<IhHBB', d, o + 8)
            s = (n.decode('latin-1'), valor, sec, clase)
            self.simbolos[i] = s
            self.lista.append(s)
            i += 1 + naux

    def funciones(self, tamanos=None):
        """{name: (units, {position: (type, target)})}: 32-bit words on PPC, bytes on x86. With `tamanos`
        ({name: bytes}) an x86 function measures what the configuration says, as objdiff compares it: if IDA put the
        alignment int3 inside it, they are not trimmed (and it will not match ours, which does not carry them)."""
        por_sec = collections.defaultdict(list)
        for nombre, valor, sec, clase in self.lista:
            if sec <= 0 or clase not in (2, 3) or nombre.startswith(('.', '$', '@')):
                continue
            if not self.secciones[sec - 1]['car'] & 0x20:
                continue
            por_sec[sec].append((valor, nombre))
        out = {}
        paso = 4 if self.ppc else 1
        for sec, lista in por_sec.items():
            s = self.secciones[sec - 1]
            datos = self.d[s['praw']:s['praw'] + s['tam']]
            relocs = sorted((r[0], r[2], self.simbolos.get(r[1], ('?',))[0]) for r in s['relocs']
                            if not (self.ppc and r[2] == PPC_PAIR))
            offs = [r[0] for r in relocs]
            lista.sort()
            for k, (valor, nombre) in enumerate(lista):
                fin = lista[k + 1][0] if k + 1 < len(lista) else len(datos)
                if self.ppc:
                    n = (fin - valor) // 4
                    if n <= 0:
                        continue
                    pal = list(struct.unpack_from('>%dI' % n, datos, valor))
                    while pal and pal[-1] == 0:  # alignment padding between functions
                        pal.pop()
                else:
                    if tamanos and nombre in tamanos:
                        pal = list(datos[valor:min(fin, valor + tamanos[nombre])])
                    else:
                        pal = list(datos[valor:fin])
                        while pal and pal[-1] == 0xCC:  # alignment int3
                            pal.pop()
                if not pal:
                    continue
                rel = {}
                j = bisect.bisect_left(offs, valor)
                while j < len(relocs) and relocs[j][0] < valor + paso * len(pal):
                    ro, rt, rn = relocs[j]
                    rel[(ro - valor) // paso] = (rt, rn)
                    j += 1
                out[nombre] = (pal, rel)
        return out


ACCESO = {c: g for g, cs in enumerate(('AIQ', 'BJR', 'CKS', 'DLT', 'EMU', 'FNV', '012')) for c in cs}


def equivalentes(orig, nuestro):
    """The same symbol under another name: the 360 split disambiguates repeated names with `_N`
    (`__real@00000000_2`), and a header of ours that declares public what the original has private or protected only
    changes the access letter of the MSVC decoration (`?Reset@AIVehiclePid@@MAEXXZ` and `...@@UAEXXZ`: the same
    function)."""
    if orig == nuestro:
        return True
    m = re.match(r'^(.*)_\d+$', orig)
    if m and m.group(1) == nuestro:
        return True
    if len(orig) == len(nuestro) and orig.startswith('?'):
        dif = [i for i, (x, y) in enumerate(zip(orig, nuestro)) if x != y]
        if len(dif) == 1:
            i = dif[0]
            return (orig[i - 2:i] == '@@' and orig[i] in ACCESO and nuestro[i] in ACCESO
                    and ACCESO[orig[i]] == ACCESO[nuestro[i]])
    return False


def cadena_msvc(nombre):
    """Byte prefix encoded by `??_C@_0<length>@<hash>@<text>@` (None if it is not a char string)."""
    m = re.match(r'^\?\?_C@_0[A-P0-9]+@[A-P]+@(.*)@$', nombre)
    if not m:
        return None
    esp = {'0': b',', '1': b'/', '2': b'\\', '3': b':', '4': b'.', '5': b' ', '6': b'\n', '7': b'\t', '8': b"'",
           '9': b'-'}
    t = m.group(1)
    out = b''
    i = 0
    while i < len(t):
        ch = t[i]
        if ch != '?':
            out += ch.encode('latin-1')
            i += 1
        elif t[i + 1:i + 2] == '$':
            h = t[i + 2:i + 4]
            if len(h) < 2 or any(x not in 'ABCDEFGHIJKLMNOP' for x in h):
                return None
            out += bytes([(ord(h[0]) - 65) * 16 + ord(h[1]) - 65])
            i += 4
        elif t[i + 1:i + 2] in esp:
            out += esp[t[i + 1]]
            i += 2
        elif 'a' <= t[i + 1:i + 2] <= 'z':
            out += bytes([0xE1 + ord(t[i + 1]) - ord('a')])
            i += 2
        elif 'A' <= t[i + 1:i + 2] <= 'Z':
            out += bytes([0xC1 + ord(t[i + 1]) - ord('A')])
            i += 2
        else:
            return None
    return out


def msvc_partes(nombre):
    """(base name, [scopes from inner to outer]) of a simple MSVC decoration; None if it is not understood.
    `?GetAttachments@Activity@Sim@@...` -> ('GetAttachments', ['Activity', 'Sim']); `??0FEObject@@...` ->
    ('FEObject', ['FEObject']); `??1X@@` -> ('~X', ['X']); `??$BindFunction@...` -> ('BindFunction', [])."""
    if nombre.startswith('??$'):
        m = re.match(r'^\?\?\$([A-Za-z_]\w*)@', nombre)
        return (m.group(1), []) if m else None
    m = re.match(r'^\?(\?[01])?([A-Za-z_]\w*)@((?:[A-Za-z_]\w*@)*)@', nombre)
    if not m:
        m = re.match(r'^\?([A-Za-z_]\w*)@', nombre)
        return (m.group(1), None) if m else None  # scopes with templates: the name only
    base = m.group(2)
    ambitos = [x for x in m.group(3).split('@') if x]
    if m.group(1) == '?0':
        return base, [base] + ambitos
    if m.group(1) == '?1':
        return '~' + base, [base] + ambitos
    return base, ambitos


def gcc_partes(nombre):
    """(base, class or None) of a simple GCC 2.95 decoration: `Default__14PlayerSettings` ->
    ('Default', 'PlayerSettings'); `f__FPc` -> ('f', None)."""
    m = re.match(r'^([A-Za-z_]\w*?)__(\d+)([A-Za-z_]\w*)', nombre)
    if m:
        k = int(m.group(2))
        return m.group(1), m.group(3)[:k] if len(m.group(3)) >= k else None
    m = re.match(r'^([A-Za-z_]\w*?)__[FCQt]', nombre)
    return (m.group(1), None) if m else None


def mismo_nombre_de_pila(raro, nuestro):
    """A name MSVC cannot emit (IDA demangle, GCC decoration, `name__ADDRESS`) and ours say the SAME function: same
    name and same scopes (IDA), same name and class (GCC) or same name (suffix)."""
    p = msvc_partes(nuestro)
    if not p:
        return False
    base, ambitos = p
    if '::' in raro and '(' not in raro:
        partes = raro.split('::')
        ult = partes[-1].replace('__', '~', 1) if partes[-1].startswith('__') else partes[-1]
        return (ambitos is not None and len(ambitos) >= 1 and ult == base
                and partes[:-1] == list(reversed(ambitos))[-(len(partes) - 1):])
    m = re.match(r'^([A-Za-z_]\w*?)__[0-9A-F]{8}$', raro)
    if m:
        return m.group(1) == base
    g = gcc_partes(raro)
    if g:
        return g[0] == base and (g[1] is None or ambitos is None or (ambitos and g[1] == ambitos[0]))
    return False


class Memoria:
    """Data bytes of the original, to check the MSVC names that carry their contents inside."""

    def __init__(self, trozos):
        self.trozos = sorted(trozos)
        self.inicios = [t[0] for t in self.trozos]

    def lee(self, a, n):
        k = bisect.bisect_right(self.inicios, a) - 1
        if k < 0:
            return None
        ini, d = self.trozos[k]
        if a + n > ini + len(d):
            return None
        return d[a - ini:a - ini + n]

    def comprueba(self, a, nombre):
        """True/False if the name states its contents and the original has (or not) them at `a`; None if unknown."""
        m = re.match(r'^__real@([0-9a-f]{8}|[0-9a-f]{16})$', nombre)
        if m:
            esperado = bytes.fromhex(m.group(1))
            real = self.lee(a, len(esperado))
            if real is None:
                return None
            return real == esperado or real == esperado[::-1]  # 360 stores big endian; PC, little
        pref = cadena_msvc(nombre)
        if pref:
            real = self.lee(a, len(pref))
            return None if real is None else real == pref
        return None


class X360:
    V = 'EUROPEGERMILESTONE'
    PASO = 4
    MIN = 12          # bytes: below this, a unique pair without a confirming target is not named
    CORTA = 20        # bytes: below this, it must also be in order
    LLAMADAS = (PPC_REL24,)
    SIN_NOMBRE = re.compile(r'^(fn|lbl|sub|func)_([0-9A-F]{8})$')
    GUARDAR = ('__savegpr', '__restgpr', '__savefpr', '__restfpr', '__savevmx', '__restvmx')

    def __init__(self):
        self.lineas = open(os.path.join(RAIZ, 'config', self.V, 'symbols.txt'), encoding='utf-8').read().split('\n')
        self.por_dir = collections.defaultdict(list)
        self.por_nombre = collections.defaultdict(list)
        for i, l in enumerate(self.lineas):
            m = re.match(r'(\S+) = (\.\w+):0x([0-9A-Fa-f]+);(.*)$', l)
            if m:
                a = int(m.group(3), 16)
                self.por_dir[a].append((m.group(1), m.group(2), i))
                self.por_nombre[m.group(1)].append(a)
        self.texto = {a for a, es in self.por_dir.items() if any(e[1] == '.text' for e in es)}
        self.rangos = []
        unidad = None
        for l in open(os.path.join(RAIZ, 'config', self.V, 'splits.txt'), encoding='utf-8'):
            if l and not l[0].isspace() and l.rstrip().endswith(':'):
                unidad = l.strip()[:-1].rsplit('.', 1)[0]
                continue
            m = re.match(r'\s+\.text\s+start:0x([0-9A-F]+) end:0x([0-9A-F]+)', l)
            if m and unidad and unidad != 'Sections':
                self.rangos.append((int(m.group(1), 16), int(m.group(2), 16), unidad))
        self.rangos.sort()
        trozos = []
        for p in glob.glob(os.path.join(RAIZ, 'build', self.V, 'obj', 'auto_*.obj')):
            m = re.search(r'auto_\d+_([0-9A-F]{8})_(rdata|data)\.obj$', p)
            if m:
                c = Coff(p)
                s = c.secciones[0]
                trozos.append((int(m.group(1), 16), c.d[s['praw']:s['praw'] + s['tam']]))
        self.memoria = Memoria(trozos)
        for nombre, a in (('__real@c7c35000', 0x820B83EC), ('??_C@_0BA@BDHKIADM@AIGoalHeadOnRam?$AA@', 0x82034D28)):
            assert self.memoria.comprueba(a, nombre) is True, nombre  # control of the data reading

    def dir_de(self, nombre):
        m = self.SIN_NOMBRE.match(nombre)
        return int(m.group(2), 16) if m else None

    def es_funcion_sin_nombre(self, nombre):
        return nombre.startswith('fn_') and self.dir_de(nombre) is not None

    def compatible(self, f, n):
        return True

    def falso(self, a):
        return 'fn_%08X' % a

    def unidad_de(self, a):
        for ini, fin, u in self.rangos:
            if ini <= a < fin:
                return u
        return None

    def valido(self, nombre):
        return (nombre and not nombre.startswith(('.', '$', '@', '__LINE__')) and not self.SIN_NOMBRE.match(nombre)
                and len(nombre) <= MAX_NOMBRE and ' ' not in nombre)

    def esqueleto(self, pal, rel):
        out = []
        for i, w in enumerate(pal):
            r = rel.get(i)
            if r is not None:
                t = r[0]
                if t == PPC_REL24:
                    w &= 0xFC000003
                elif t == PPC_ADDR32:
                    w = 0
                else:
                    w &= 0xFFFF0000
                out.append((w, t))
            else:
                out.append((w, 0))
        return tuple(out)

    def destino(self, ts, po, ps, i, a):
        """Address of OUR symbol according to the original (a = address of the unnamed target), or None."""
        if ts in (PPC_REFHI, PPC_REFLO):
            if ps[i] & 0xFFFF:
                return None  # ours points to symbol+k
            if ts == PPC_REFLO and (po[i] & 0xFFFF) != (a & 0xFFFF):
                return None
            if ts == PPC_REFHI and (po[i] & 0xFFFF) != (((a + 0x8000) >> 16) & 0xFFFF):
                return None
            return a
        return a if ts == PPC_REL24 else None

    def dato_con_nombre(self, no, ns, ts, po, ps, i):
        """A named (data) target in the original and another in ours that states its contents: True/False by the
        bytes; None if it cannot be decided (or the other half of the lis/addi pair already decides it)."""
        dirs = [x for x in self.por_nombre.get(no, []) if x not in self.texto]
        if len(dirs) != 1 or ps[i] & 0xFFFF or self.memoria.comprueba(dirs[0], ns) is None:
            return None
        if ts == PPC_REFHI:
            return 'nada'
        if ts != PPC_REFLO:
            return None
        base = dirs[0]
        absoluta = base + (((po[i] & 0xFFFF) - base) & 0xFFFF)
        if (absoluta - base) & 0x8000:
            absoluta -= 0x10000
        return bool(self.memoria.comprueba(absoluta, ns))

    def nombre_libre(self, n, a):
        """360: the name cannot already be in the same unit (in another one it can: per-unit copies)."""
        u = self.unidad_de(a)
        return not [x for x in self.por_nombre.get(n, []) if self.unidad_de(x) == u]

    def aplicar(self, finales):
        for f, n in finales.items():
            i = [e for e in self.por_dir[self.dir_de(f)] if e[0] == f and e[1] == '.text'][0][2]
            l = self.lineas[i]
            assert l.startswith(f + ' = .text:'), l
            self.lineas[i] = n + l[len(f):]
        open(os.path.join(RAIZ, 'config', self.V, 'symbols.txt'), 'w', encoding='utf-8', newline='\n').write(
            '\n'.join(self.lineas))
        return len(finales)


class PC:
    V = 'SPEED_EXE_1_3'
    PASO = 1
    MIN = 8
    CORTA = 16
    LLAMADAS = (I386_REL32,)
    GUARDAR = ()
    # IDA placeholders: unnamed functions and data
    SIN_NOMBRE = re.compile(r'^(sub|nullsub|j_sub|unknown_libname|loc|locret|dword|byte|word|off|unk|flt|dbl|stru|'
                            r'asc|qword|xmmword|tbyte|jpt|def)_[0-9A-Fa-f]+$')
    FUNCION_SIN_NOMBRE = re.compile(r'^(sub|nullsub|j_sub|unknown_libname)_[0-9A-Fa-f]+$')

    def __init__(self):
        self.ruta = os.path.join(RAIZ, 'config', self.V, 'symbols.json')
        d = json.load(open(self.ruta, encoding='utf-8'))
        self.por_nombre = collections.defaultdict(list)
        self.funciones = {}
        self.tamanos = {}
        for f in d['functions']:
            self.por_nombre[f['name']].append(f['start'])
            self.funciones[f['name']] = f['start']
            self.tamanos[f['name']] = f['end'] - f['start']
        for x in d['names']:
            if x['addr'] not in self.por_nombre[x['name']]:
                self.por_nombre[x['name']].append(x['addr'])
        s = json.load(open(os.path.join(RAIZ, 'config', self.V, 'splits.json'), encoding='utf-8'))
        self.rangos = sorted((ini, fin, u[:-4]) for u, v in s.items() for ini, fin in v.get('function_ranges', []))
        pe = open(os.path.join(RAIZ, 'orig', self.V, 'speed.exe'), 'rb').read()
        e = struct.unpack_from('<I', pe, 0x3c)[0]
        nsec = struct.unpack_from('<H', pe, e + 6)[0]
        opt = struct.unpack_from('<H', pe, e + 20)[0]
        base = struct.unpack_from('<I', pe, e + 24 + 28)[0]
        trozos = []
        for i in range(nsec):
            nombre, vsz, va, rsz, raw = struct.unpack_from('<8sIIII', pe, e + 24 + opt + i * 40)
            if nombre.rstrip(b'\0') in (b'.rdata', b'.data'):
                trozos.append((base + va, pe[raw:raw + min(rsz, vsz)]))
        self.memoria = Memoria(trozos)

    # names MSVC cannot emit (the executable was built with MSVC 7.1): IDA demangles (`FEListBox::SetCellScale`,
    # library signatures such as `Concurrency::details::...`), GCC decorations brought from GameCube
    # (`Default__14PlayerSettings`) or names with the address glued on (`BindFunction__0061A370`). As a relocation
    # target they are worth the same as a sub_: they cannot equal ours, but their address can.
    RARO = re.compile(r'(::|__[0-9A-F]{8}$|^[A-Za-z_][A-Za-z0-9_]*__(?:\d|F|C|Q\d|t\d|H\d)|^\$SG\d+$)')

    def dir_de(self, nombre):
        if not (self.SIN_NOMBRE.match(nombre) or self.RARO.search(nombre)):
            return None
        ds = self.por_nombre.get(nombre, [])
        return ds[0] if len(ds) == 1 else None

    def es_funcion_sin_nombre(self, nombre):
        return ((bool(self.FUNCION_SIN_NOMBRE.match(nombre)) or bool(self.RARO.search(nombre)))
                and nombre in self.funciones and self.dir_de(nombre) is not None)

    def compatible(self, f, n):
        """A sub_ accepts any name; an odd name, only ours when it says the same function."""
        return bool(self.FUNCION_SIN_NOMBRE.match(f)) or mismo_nombre_de_pila(f, n)

    def falso(self, a):
        n = 'sub_%X' % a
        self.por_nombre[n] = [a]
        self.funciones[n] = a
        return n

    def unidad_de(self, a):
        k = bisect.bisect_right(self.rangos, (a, 1 << 40, '')) - 1
        if k >= 0 and self.rangos[k][0] <= a < self.rangos[k][1]:
            return self.rangos[k][2]
        return None

    def valido(self, nombre):
        return (nombre and not nombre.startswith(('.', '$', '@', '__LINE__')) and not self.SIN_NOMBRE.match(nombre)
                and ' ' not in nombre and '"' not in nombre and '\\' not in nombre)

    def esqueleto(self, pal, rel):
        b = list(pal)
        for i, (t, n) in rel.items():
            for j in range(i, min(i + 4, len(b))):
                b[j] = -1
        return (tuple(b), tuple(sorted((i, t) for i, (t, n) in rel.items())))

    def destino(self, ts, po, ps, i, a):
        k = struct.unpack_from('<i', bytes(ps[i:i + 4]))[0] if i + 4 <= len(ps) else None
        if k != 0:
            return None  # ours points to symbol+k
        return a if ts in (I386_REL32, I386_DIR32) else None

    def dato_con_nombre(self, no, ns, ts, po, ps, i):
        dirs = self.por_nombre.get(no, [])
        if len(dirs) != 1 or ts != I386_DIR32:
            return None
        k = struct.unpack_from('<i', bytes(ps[i:i + 4]))[0] if i + 4 <= len(ps) else None
        if k != 0:
            return None
        v = self.memoria.comprueba(dirs[0], ns)
        return None if v is None else bool(v)

    def nombre_libre(self, n, a):
        """PC: each COMDAT once; the name cannot be in symbols.json."""
        return n not in self.por_nombre

    def aplicar(self, finales):
        b = open(self.ruta, 'rb').read().decode('utf-8')
        cambios = 0
        for f, n in finales.items():
            # the old name may be in `functions` and in `names` (the same address): both are changed
            antes = '"name": "%s",' % json.dumps(f)[1:-1]
            if 1 <= b.count(antes) <= 2 and len(self.por_nombre.get(f, [])) == 1:
                b = b.replace(antes, '"name": "%s",' % json.dumps(n)[1:-1])
                cambios += 1
        open(self.ruta, 'wb').write(b.encode('utf-8'))
        return cambios


class Unidad:
    def __init__(self, P, nombre, po, ps):
        self.nombre = nombre
        self.fo = Coff(po).funciones(getattr(P, 'tamanos', None))
        cs = Coff(ps)
        self.fs = cs.funciones()
        self.indice = {}  # definition order in our object
        for k, s in enumerate(cs.lista):
            self.indice.setdefault(s[0], k)
        self.eo = {n: P.esqueleto(*v) for n, v in self.fo.items()}
        self.es = {n: P.esqueleto(*v) for n, v in self.fs.items()}


def main():
    _english_args()
    args = sys.argv[1:]
    posic = [a for i, a in enumerate(args) if not a.startswith('--') and (i == 0 or args[i - 1] != '--tsv')]
    V = posic[0] if posic else 'EUROPEGERMILESTONE'
    P = {'EUROPEGERMILESTONE': X360, 'SPEED_EXE_1_3': PC}[V]()
    tsv = args[args.index('--tsv') + 1] if '--tsv' in args else None
    detalle = '--detalle' in args
    base_o = os.path.join(RAIZ, 'build', V, 'obj')
    base_s = os.path.join(RAIZ, 'build', V, 'src')
    for a in args:
        if a.startswith('--nuestros='):
            base_s = os.path.join(RAIZ, a.split('=', 1)[1])
    unidades = []
    por_u = {}
    for p in sorted(glob.glob(os.path.join(base_s, '**', '*.obj'), recursive=True)):
        rel = os.path.relpath(p, base_s)
        o = os.path.join(base_o, rel)
        if os.path.exists(o):
            u = Unidad(P, rel.replace(os.sep, '/')[:-4], o, p)
            unidades.append(u)
            por_u[u.nombre] = u
    dir_de = P.dir_de

    # --validate F: hides a fraction F of the anchors (gives them a placeholder name at their address) and measures
    # whether the method gives them back THEIR name: the real precision of each motive and size, without trusting
    # intuition
    verdad = {}
    if '--validar' in args:
        import random
        random.seed(13)
        frac = float(args[args.index('--validar') + 1])
        for u in unidades:
            for n in list(u.fo):
                if n not in u.fs or u.es[n] != u.eo.get(n):
                    continue
                ds = [x for x in P.por_nombre.get(n, []) if P.unidad_de(x) == u.nombre]
                if len(ds) != 1 or random.random() >= frac:
                    continue
                falso = P.falso(ds[0])
                if falso in u.fo:
                    continue
                u.fo[falso] = u.fo.pop(n)
                u.eo[falso] = u.eo.pop(n)
                P.por_nombre[n] = [x for x in P.por_nombre[n] if x != ds[0]]
                if not P.por_nombre[n]:
                    del P.por_nombre[n]
                verdad[falso] = n
        print('validation: %d anchors hidden' % len(verdad))

    evid = collections.defaultdict(collections.Counter)  # unnamed address -> {our name: witnesses}
    asignado = {}      # (unit, name) -> address
    nombre_de = {}     # address -> decided name (accepted pairs)

    def compara(po, ro, ps, rs):
        """(contradictions, agreements, [(address, our name)])"""
        contra = acuerdo = 0
        ev = []
        del motivos[:]
        for i, (ts, ns) in rs.items():
            to, no = ro[i]
            if equivalentes(no, ns):
                acuerdo += 1
                continue
            a = dir_de(no)
            if a is None:
                if not P.valido(ns):
                    continue
                v = P.dato_con_nombre(no, ns, ts, po, ps, i)
                if v == 'nada':
                    continue
                if v is True:
                    acuerdo += 1
                else:
                    contra += 1  # named target in the original and another in ours (or different bytes)
                    motivos.append((i, no, ns, 'different name' if v is None else 'different bytes'))
                continue
            if not P.valido(ns):
                continue
            a = P.destino(ts, po, ps, i, a)
            if a is None:
                continue
            valor = P.memoria.comprueba(a, ns)
            if valor is not None:
                if valor:
                    acuerdo += 1  # the original's constant or string is the one in the name
                else:
                    contra += 1
                    motivos.append((i, no, ns, 'different contents'))
                continue
            ya = nombre_de.get(a)
            if ya is not None:
                if ya == ns:
                    acuerdo += 1
                else:
                    contra += 1
                    motivos.append((i, no, ns, 'the address is already %s' % ya))
                continue
            b = asignado.get((P.unidad_de(a), ns))
            if b is not None and b != a:
                contra += 1  # that name is already at another address of the same unit
                motivos.append((i, no, ns, 'the name is already at 0x%08X' % b))
                continue
            e = evid.get(a)
            if e and len(e) == 1:
                if ns in e:
                    acuerdo += 1  # the witnesses say this same name
                    continue
                if next(iter(e.values())) >= 2 or a in de_ancla:
                    contra += 1  # two witnesses, or an anchor, say another name
                    motivos.append((i, no, ns, 'the witnesses say %s' % dict(e)))
                    continue
            ev.append((a, ns))
        return contra, acuerdo, ev

    de_ancla = set()   # addresses with some witness that is an anchor (name already set and same skeleton)
    motivos = []       # why the last compared pair contradicts: (position, original target, ours, reason)

    def anota(ev, ancla=False):
        for a, ns in ev:
            evid[a][ns] += 1
            if ancla:
                de_ancla.add(a)

    n_anclas = 0
    anclas_malas = []
    for u in unidades:
        for n, e in u.eo.items():
            if n in u.fs and u.es[n] == e:
                n_anclas += 1
                c, _, ev = compara(u.fo[n][0], u.fo[n][1], u.fs[n][0], u.fs[n][1])
                if not c:
                    anota(ev, ancla=True)
                else:
                    anclas_malas.append((u.nombre, n, c, list(motivos)))
    print('anchors (same name and skeleton): %d; addresses with evidence: %d' % (n_anclas, len(evid)))

    # weaker witnesses: functions with the SAME name on both sides but different code. Their calls are aligned by the
    # ones that are named in the original (longest common subsequence, and at least one that is not a register
    # save/restore helper); in each gap between two aligned calls with the same number of unnamed calls in the
    # original as in ours, the k-th of the original is our k-th. They only break ties (motive `secuencia`).
    evid_sec = collections.defaultdict(collections.Counter)
    n_sec = 0

    def conocido(x):
        return dir_de(x) is None

    def alinea(lo, ls):
        """Pairs (i, j) of the longest common subsequence of the NAMED calls (unnamed ones do not align)."""
        n1, n2 = len(lo), len(ls)
        t = [[0] * (n2 + 1) for _ in range(n1 + 1)]
        for i in range(n1 - 1, -1, -1):
            for j in range(n2 - 1, -1, -1):
                if conocido(lo[i]) and equivalentes(lo[i], ls[j]):
                    t[i][j] = t[i + 1][j + 1] + 1
                else:
                    t[i][j] = max(t[i + 1][j], t[i][j + 1])
        out = []
        i = j = 0
        while i < n1 and j < n2:
            if conocido(lo[i]) and equivalentes(lo[i], ls[j]):
                out.append((i, j))
                i += 1
                j += 1
            elif t[i + 1][j] >= t[i][j + 1]:
                i += 1
            else:
                j += 1
        return out

    for u in unidades:
        for n in u.fo:
            if n not in u.fs or u.es[n] == u.eo[n]:
                continue
            ro, rs = u.fo[n][1], u.fs[n][1]
            lo = [ro[i][1] for i in sorted(ro) if ro[i][0] in P.LLAMADAS]
            ls = [rs[i][1] for i in sorted(rs) if rs[i][0] in P.LLAMADAS]
            if not lo or not ls or len(lo) * len(ls) > 40000:
                continue
            pares = alinea(lo, ls)
            if not [1 for i, _ in pares if not (P.GUARDAR and lo[i].startswith(P.GUARDAR))]:
                continue  # without a named call in common (other than register saving) there is no anchor
            # between two aligned calls, if the gap of the original only has UNNAMED targets and is as long as
            # ours, the k-th of the original is our k-th
            ev = []
            cortes = [(-1, -1)] + pares + [(len(lo), len(ls))]
            for (i0, j0), (i1, j1) in zip(cortes, cortes[1:]):
                hueco_o = lo[i0 + 1:i1]
                hueco_s = ls[j0 + 1:j1]
                if not hueco_o or len(hueco_o) != len(hueco_s) or any(conocido(x) for x in hueco_o):
                    continue
                for x, y in zip(hueco_o, hueco_s):
                    if P.valido(y):
                        ev.append((dir_de(x), y))
            if ev:
                n_sec += 1
                for a, y in ev:
                    evid_sec[a][y] += 1
    print('witnesses by call sequence: %d functions, %d addresses' % (n_sec, len(evid_sec)))

    grupos = []
    for u in unidades:
        por_esq = collections.defaultdict(lambda: ([], []))
        for n, e in u.eo.items():
            if P.es_funcion_sin_nombre(n):
                por_esq[e][0].append(n)
        for n, e in u.es.items():
            if P.valido(n) and n not in u.fo and e in por_esq:
                por_esq[e][1].append(n)
        for e, (fs_, ns_) in por_esq.items():
            if ns_:
                grupos.append((u, fs_, ns_))
    print('skeleton groups with candidates: %d (unnamed %d, ours %d)'
          % (len(grupos), sum(len(g[1]) for g in grupos), sum(len(g[2]) for g in grupos)))

    # order: neighbouring anchors in the original (address) and in our object (symbol index)
    vecinas = {}
    for u in unidades:
        lista = []
        for n in u.fo:
            if n in u.fs and u.es[n] == u.eo[n] and n in u.indice:
                ds = [x for x in P.por_nombre.get(n, []) if P.unidad_de(x) == u.nombre]
                if len(ds) == 1:
                    lista.append((ds[0], u.indice[n]))
        vecinas[u.nombre] = sorted(lista)

    def en_orden(u, f, n):
        lista = vecinas[u.nombre]
        k = bisect.bisect_left(lista, (dir_de(f), -1))
        antes = lista[k - 1][1] if k > 0 else -1
        despues = lista[k][1] if k < len(lista) else 1 << 30
        return antes < u.indice.get(n, -1) < despues

    def tam(u, f):
        return P.PASO * len(u.fo[f][0])

    aceptadas = {}  # unnamed -> (unit, name, motive, bytes, agreements, relocations)
    for vuelta in range(20):
        nuevas = 0
        for u, fs_, ns_ in grupos:
            libres_f = [f for f in fs_ if f not in aceptadas]
            libres_n = [n for n in ns_ if (u.nombre, n) not in asignado]
            if not libres_f or not libres_n:
                continue
            compat = {}
            for f in libres_f:
                po, ro = u.fo[f]
                for n in libres_n:
                    ps, rs = u.fs[n]
                    c, acu, ev = compara(po, ro, ps, rs)
                    if not c:
                        compat[(f, n)] = (acu, ev)
            elegidas = []
            for f in libres_f:
                a = dir_de(f)
                cn = [n for n in libres_n if (f, n) in compat and P.compatible(f, n)]
                if not cn:
                    continue
                e = evid.get(a)
                if e:
                    if len(e) == 1:
                        n = next(iter(e))
                        if n in cn:
                            elegidas.append((f, n, 'llamantes'))
                    continue  # split evidence: not decided
                e = evid_sec.get(a)
                if e and len(cn) > 1:
                    if len(e) == 1:
                        n = next(iter(e))
                        if n in cn and not [g for g in libres_f if g != f and evid_sec.get(dir_de(g), {}).get(n)]:
                            elegidas.append((f, n, 'secuencia'))
                    continue
                if len(cn) != 1:
                    # several compatible ones: the one with the MOST confirmed targets wins if it is the only one with
                    # that maximum, and if in the other direction this fn_ is also the best for it
                    puntos = sorted(((compat[(f, n)][0], n) for n in cn), reverse=True)
                    if puntos[0][0] == 0 or puntos[0][0] == puntos[1][0]:
                        continue
                    n = puntos[0][1]
                    rivales = [compat[(g, n)][0] for g in libres_f if g != f and (g, n) in compat and P.compatible(g, n)]
                    if rivales and max(rivales) >= puntos[0][0]:
                        continue
                    elegidas.append((f, n, 'mejor'))
                    continue
                n = cn[0]
                if len([g for g in libres_f if (g, n) in compat and P.compatible(g, n)]) != 1:
                    continue
                if tam(u, f) < P.MIN and compat[(f, n)][0] == 0:
                    continue
                if tam(u, f) < P.CORTA and compat[(f, n)][0] == 0 and not en_orden(u, f, n):
                    continue  # short and without any confirming target: at least it must be where it belongs
                elegidas.append((f, n, 'unica'))
            vistos = collections.Counter(n for _, n, _ in elegidas)
            for f, n, motivo in elegidas:
                if vistos[n] > 1 or (u.nombre, n) in asignado or not P.nombre_libre(n, dir_de(f)):
                    continue
                if not isinstance(P, X360) and n in {v[1] for v in aceptadas.values()}:
                    continue  # PC: one name, one address
                aceptadas[f] = (u.nombre, n, motivo, tam(u, f), compat[(f, n)][0], len(u.fo[f][1]))
                asignado[(u.nombre, n)] = dir_de(f)
                nombre_de[dir_de(f)] = n
                anota(compat[(f, n)][1])
                nuevas += 1
        print('pass %d: %d new pairs (total %d)' % (vuelta, nuevas, len(aceptadas)), flush=True)
        if not nuevas:
            break

    # second reading with ALL the names already decided: a pair that now contradicts another one falls (and repeat)
    while True:
        caidas = []
        for f, v in aceptadas.items():
            u = por_u[v[0]]
            c, acu, _ = compara(u.fo[f][0], u.fo[f][1], u.fs[v[1]][0], u.fs[v[1]][1])
            if c:
                caidas.append(f)
            else:
                aceptadas[f] = v[:4] + (acu,) + v[5:]
        for f in caidas:
            un, n = aceptadas[f][:2]
            del aceptadas[f]
            del asignado[(un, n)]
            del nombre_de[dir_de(f)]
        print('second reading: %d pairs fall' % len(caidas))
        if not caidas:
            break

    # what remains ambiguous (for the report)
    pend = collections.Counter()
    pend_b = collections.Counter()
    for u, fs_, ns_ in grupos:
        libres_f = [f for f in fs_ if f not in aceptadas]
        libres_n = [n for n in ns_ if (u.nombre, n) not in asignado]
        for f in libres_f:
            cn = [n for n in libres_n if not compara(u.fo[f][0], u.fo[f][1], u.fs[n][0], u.fs[n][1])[0]]
            if not cn:
                continue
            clase_ = 'one candidate' if len(cn) == 1 else 'several candidates'
            if tam(u, f) < P.MIN:
                clase_ += ' (short)'
            pend[clase_] += 1
            pend_b[clase_] += tam(u, f)
    print('pending (unnamed with compatible candidates, undecided): %s  bytes %s' % (dict(pend), dict(pend_b)))

    mot = collections.Counter(v[2] for v in aceptadas.values())
    print('FINAL: %d functions, %d B  %s' % (len(aceptadas), sum(v[3] for v in aceptadas.values()), dict(mot)))
    por_unidad = collections.Counter(v[0].split('/')[-1] for v in aceptadas.values())
    print('  by unit:', dict(por_unidad.most_common()))
    if detalle:
        conflictos = [(a, dict(e)) for a, e in evid.items() if len(e) > 1 and P.unidad_de(a)]
        print('  SourceList addresses with split evidence:', len(conflictos))
        for a, e in conflictos[:25]:
            print('    0x%08X %s' % (a, e))
    print('anchors (name ALREADY set) with targets that contradict ours: %d' % len(anclas_malas))
    if '--anclas-malas' in args:
        for un, n, c, mot in anclas_malas:
            print('  ANCHOR %s %s (%d)' % (un.split('/')[-1], n, c))
            if detalle:
                for i, no, ns, m in mot:
                    print('      +%d: original %s | ours %s | %s' % (i * P.PASO, no, ns, m))
    if tsv:
        with open(tsv, 'w', encoding='utf-8', newline='\n') as fh:
            for f, (un, n, motivo, b, acu, nrel) in sorted(aceptadas.items()):
                fh.write('%s\t%s\t%s\t%s\t%d\t%d\t%d\n' % (f, n, un.split('/')[-1], motivo, b, acu, nrel))
    if verdad:
        res = collections.Counter()
        for f, v in aceptadas.items():
            if f in verdad:
                clase_ = (v[2], '<16' if v[3] < 16 else '<32' if v[3] < 32 else '<64' if v[3] < 64 else '>=64',
                          'acu>0' if v[4] else ('no relocations' if not v[5] else 'acu0'))
                res[clase_ + ('BIEN' if v[1] == verdad[f] else 'MAL',)] += 1
        clases = sorted({k[:3] for k in res})
        print('VALIDATION: hidden %d, named %d, right %d, wrong %d'
              % (len(verdad), sum(res.values()), sum(v for k, v in res.items() if k[3] == 'BIEN'),
                 sum(v for k, v in res.items() if k[3] == 'MAL')))
        for k in clases:
            print('   %-40s right %5d  wrong %4d' % (k, res[k + ('BIEN',)], res[k + ('MAL',)]))
        if '--detalle' in args:
            for f, v in aceptadas.items():
                if f in verdad and v[1] != verdad[f]:
                    print('   WRONG %s %s: set %s, was %s' % (v[0].split('/')[-1], f, v[1][:60], verdad[f][:60]))
        return
    if '--aplicar' in args:
        print('applied', P.aplicar({f: v[1] for f, v in aceptadas.items()}))


if __name__ == '__main__':
    main()
