#!/usr/bin/env python3
"""a138_referencia.py -- el Xbox A138 (default.xbe del 8-oct-2005, MSVC 7.1 XDK 5849: el compilador de speed.exe) como
REFERENCIA de PC (ronda 81, carril XBOX-A138). Mide; no escribe nada en config/.

    python scripts/a138_referencia.py --xbe RUTA/a138_default.xbe [--informe-pc build/versiones/SPEED_EXE_1_3.json]
                                      [--informe-gc build/versiones/GOWE69.json] [--salida build/a138] [--cache]

El XBE no trae simbolos y el Milestone conserva el PUNTERO DE MARCO (push ebp/mov ebp,esp en ~53 % de los inicios;
speed.exe ~5 %), asi que se comparan dos esqueletos x86 (capstone):
  raw  : mnemonico + operandos, direcciones de imagen -> A, destinos de salto/llamada -> T;
  norm : ademas sin prologo/epilogo de marco ni push/pop de registros salvados, memoria de pila -> S, registros -> rNN.
Alineacion por ORDEN DE ENLACE: anclas de esqueleto unico en los dos lados (subsecuencia creciente), en cada hueco las
parejas de esqueleto igual y unico del hueco y despues programacion dinamica con el parecido (SequenceMatcher sobre
norm); cuatro pasadas mas para los bloques que el enlace movio. Niveles: raw (identica salvo direcciones), norm
(identica salvo marco/registros), casi (>= 0,9), parecida (>= 0,7), debil (< 0,7).

Validacion INDEPENDIENTE por cadenas (una cadena de >= 6 caracteres que referencia UNA sola funcion en cada binario
empareja esas dos): en la r81, 630 parejas: raw 97,4 %, norm 98,2 %, casi 99,3 %, parecida 98,1 %, debil 83,3 %
(los fallos raw/norm son tiras de funciones de forma identica con margen 0: codigo igual, copia corrida).

Salidas en --salida:
  gemelas_pc.tsv          cada funcion de PC con su gemela Xbox, nivel, parecido, margen y estado en el informe;
  gemelas_por_unidad.tsv  (a) por unidad de splits.json;
  b_iguales_sin_casar.tsv (b) no casan en PC (parcial o nombre sin objeto nuestro) con gemela Xbox IGUAL a speed.exe
                          (raw/norm), con el estado en GameCube por nombre cualificado y el tipo de diferencia de
                          nuestro objeto frente a speed.exe (registros, constantes, orden, longitud...);
  tres_vias.tsv           parciales con gemela Xbox DISTINTA de speed.exe: nuestro codigo igual al del Xbox o mas
                          parecido a el que a PC (= nuestra fuente es la del Milestone, o una rama que no es la de PC).
Lectura: Xbox == speed.exe prueba que esa funcion no tiene rama propia de PC frente a Xbox (puede tener una rama x86
comun, WIN32||XBOX); NO prueba lo contrario: entre las funciones que YA casan en PC, la mitad de las que tienen gemela
solo son casi/parecida por el puntero de marco.
Necesita la medida de PC (los objetos de build/SPEED_EXE_1_3/src y el informe) y capstone.
"""
import argparse
import bisect
import collections
import difflib
import glob
import json
import os
import pickle
import re
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MD = Cs(CS_ARCH_X86, CS_MODE_32)
HEX = re.compile(r'0x[0-9a-f]+')
R32 = re.compile(r'\b(e[abcd]x|e[sd]i|ebp|esp)\b')
R16 = re.compile(r'\b([abcd]x|[sd]i|bp)\b')
R8 = re.compile(r'\b([abcd][lh])\b')
STK = re.compile(r'\[(?:ebp|esp)(?: [+-] (?:0x[0-9a-f]+|\d+))?\]')
SALVA = {'ebx', 'esi', 'edi', 'ebp'}
AUTO = re.compile(r'^(sub_|nullsub_|j_|unknown_libname_|fn_)')
XLO, XHI, PLO, PHI = 0x10000, 0x540000, 0x400000, 0x9D0000
UMIN = 0.5
NIVELES = ['raw', 'norm', 'casi', 'parecida', 'debil', 'igual_fuera', '-']


class XBE:
    def __init__(s, p):
        d = s.d = open(p, 'rb').read()
        assert d[:4] == b'XBEH', 'no es un XBE'
        s.base = struct.unpack_from('<I', d, 0x104)[0]
        n, sh = struct.unpack_from('<II', d, 0x11C)
        s.secs = []
        for i in range(n):
            o = sh - s.base + i * 56
            fl, va, vs, ra, rs, na = struct.unpack_from('<IIIIII', d, o)
            nm = d[na - s.base:d.index(b'\0', na - s.base)].decode('latin1')
            s.secs.append((nm, va, vs, ra, rs))

    def data(s, nm):
        for x in s.secs:
            if x[0] == nm:
                return x[1], s.d[x[3]:x[3] + x[4]]

    def read(s, va, n):
        for nm, v, vs, ra, rs in s.secs:
            if v <= va < v + rs:
                return s.d[ra + va - v: ra + va - v + n]
        return b''


class PE32:
    def __init__(self, ruta):
        d = self.d = open(ruta, 'rb').read()
        pe, = struct.unpack_from('<I', d, 0x3C)
        nsec, = struct.unpack_from('<H', d, pe + 6)
        optsz, = struct.unpack_from('<H', d, pe + 20)
        self.base, = struct.unpack_from('<I', d, pe + 24 + 28)
        so = pe + 24 + optsz
        self.secs = []
        for i in range(nsec):
            nm, vs, va, rs, rp = struct.unpack_from('<8sIIII', d, so + 40 * i)
            self.secs.append((va, max(vs, rs), rp))

    def lee(self, va, n):
        rva = va - self.base
        for sva, sz, rp in self.secs:
            if sva <= rva < sva + sz:
                o = rp + rva - sva
                return self.d[o:o + n]
        return b''


def _ops(op, lo, hi):
    return HEX.sub(lambda x: 'A' if lo <= int(x.group(), 16) < hi else x.group(), op)


def esqueletos(code, va, lo, hi):
    ins = list(MD.disasm(code, va))
    while ins and ins[-1].mnemonic in ('int3', 'nop'):
        ins.pop()
    raw, calls = [], []
    for i in ins:
        m, o = i.mnemonic, i.op_str
        if (m.startswith('j') or m == 'call') and o.startswith('0x'):
            raw.append(m + ' T')
            if m == 'call':
                calls.append(int(o, 16))
        else:
            raw.append(m + ' ' + _ops(o, lo, hi))
    k0, k1 = 0, len(ins)
    if k1 >= 2 and ins[0].mnemonic == 'push' and ins[0].op_str == 'ebp' and ins[1].mnemonic == 'mov' and ins[1].op_str == 'ebp, esp':
        k0 = 2
    while k0 < k1 and ins[k0].mnemonic == 'push' and ins[k0].op_str in SALVA:
        k0 += 1
    out = []
    for idx in range(k0, k1):
        i = ins[idx]
        m, o = i.mnemonic, i.op_str
        if m == 'pop' and o in SALVA:
            j = idx + 1
            while j < k1 and (ins[j].mnemonic in ('pop', 'leave') or (ins[j].mnemonic == 'mov' and ins[j].op_str == 'esp, ebp')):
                j += 1
            if j < k1 and ins[j].mnemonic == 'ret':
                continue
        if m == 'leave' or (m == 'mov' and o == 'esp, ebp'):
            continue
        if (m.startswith('j') or m == 'call') and o.startswith('0x'):
            out.append(m + ' T')
            continue
        o = R8.sub('r8', R16.sub('r16', R32.sub('r32', _ops(STK.sub('S', o), lo, hi))))
        out.append(m + ' ' + o)
    return raw, out, calls


def funciones_xbe(x):
    tva, td = x.data('.text')
    ini = {tva}
    for k in range(16, len(td), 16):
        if td[k] != 0xCC and td[k - 1] == 0xCC:
            ini.add(tva + k)
    for k in range(len(td) - 5):
        if td[k] == 0xE8:
            t = tva + k + 5 + struct.unpack_from('<i', td, k + 1)[0]
            if tva <= t < tva + len(td):
                ini.add(t)
    for m in re.finditer(rb'(?:\xC3|\xC2..)(?=\x55\x8B\xEC)', td, re.S):
        ini.add(tva + m.end())
    for nm in ('.rdata', '.data'):
        va, b = x.data(nm)
        for k in range(0, len(b) - 3, 4):
            t = struct.unpack_from('<I', b, k)[0]
            if tva < t < tva + len(td):
                o = t - tva
                if td[o - 1] in (0xCC, 0xC3) or (o >= 3 and td[o - 3] == 0xC2):
                    ini.add(t)
    ini = sorted(ini)
    return [(a, (ini[i + 1] if i + 1 < len(ini) else tva + len(td)) - a) for i, a in enumerate(ini)]


def lee_obj(ruta):
    """{decorado: bytes} de un COFF nuestro: DIR32 -> 0x500000 (sale A), REL32 -> 0 (sale T)."""
    d = open(ruta, 'rb').read()
    mach, nsec, _, symptr, nsym, optsz, _ = struct.unpack_from('<HHIIIHH', d, 0)
    secs = []
    for i in range(nsec):
        o = 20 + optsz + 40 * i
        nm, vs, va, rs, rp, relp, lnp, nrel, nln, ch = struct.unpack_from('<8sIIIIIIHHI', d, o)
        secs.append((rs, rp, relp, nrel))
    strtab = symptr + 18 * nsym
    por = collections.defaultdict(list)
    i = 0
    while i < nsym:
        o = symptr + 18 * i
        raw = d[o:o + 8]
        val, sec, typ, cls, naux = struct.unpack_from('<IhHBB', d, o + 8)
        if raw[:4] == b'\0\0\0\0':
            off, = struct.unpack_from('<I', raw, 4)
            nm = d[strtab + off:d.index(b'\0', strtab + off)].decode('latin-1')
        else:
            nm = raw.rstrip(b'\0').decode('latin-1')
        if sec > 0 and typ == 0x20:
            por[sec].append((val, nm))
        i += 1 + naux
    out = {}
    for sec, lst in por.items():
        rs, rp, relp, nrel = secs[sec - 1]
        b = bytearray(d[rp:rp + rs])
        for k in range(nrel):
            rva, si, ty = struct.unpack_from('<IIH', d, relp + 10 * k)
            if rva + 4 <= len(b):
                b[rva:rva + 4] = struct.pack('<I', 0x500000) if ty == 6 else b'\0\0\0\0'
        lst.sort()
        for j, (val, nm) in enumerate(lst):
            out.setdefault(nm, bytes(b[val:(lst[j + 1][0] if j + 1 < len(lst) else rs)]))
    return out


def carga(xbe, cache):
    if cache and os.path.exists(cache):
        return pickle.load(open(cache, 'rb'))
    x = XBE(xbe)
    pc = PE32(os.path.join(RAIZ, 'orig', 'SPEED_EXE_1_3', 'speed.exe'))
    fx = funciones_xbe(x)
    pcj = json.load(open(os.path.join(RAIZ, 'config', 'SPEED_EXE_1_3', 'symbols.json'), encoding='utf-8'))['functions']
    fp = sorted((f['start'], f['end'] - f['start'], f['name']) for f in pcj)
    D = {'fx': fx, 'fp': fp, 'rx': {}, 'nx': {}, 'cx': {}, 'rp': {}, 'np': {}, 'cp': {}}
    for a, z in fx:
        D['rx'][a], D['nx'][a], D['cx'][a] = esqueletos(x.read(a, z), a, XLO, XHI)
    for a, z, n in fp:
        D['rp'][a], D['np'][a], D['cp'][a] = esqueletos(pc.lee(a, z), a, PLO, PHI)
    if cache:
        pickle.dump(D, open(cache, 'wb'))
    return D


def lis(pares):
    pv = [p for _, p in pares]
    tails, prev, ti = [], [-1] * len(pv), []
    for i, v in enumerate(pv):
        j = bisect.bisect_left(tails, v)
        if j == len(tails):
            tails.append(v)
            ti.append(i)
        else:
            tails[j] = v
            ti[j] = i
        prev[i] = ti[j - 1] if j else -1
    k, out = (ti[-1] if ti else -1), []
    while k >= 0:
        out.append(pares[k])
        k = prev[k]
    return out[::-1]


def alinea(D):
    xs = [a for a, z in D['fx']]
    ps = [a for a, z, n in D['fp']]

    def anclas():
        pares = set()
        for k in ('r', 'n'):
            hx, hp = collections.defaultdict(list), collections.defaultdict(list)
            for a, s in D[k + 'x'].items():
                if len(s) >= 8:
                    hx[tuple(s)].append(a)
            for a, s in D[k + 'p'].items():
                if len(s) >= 8:
                    hp[tuple(s)].append(a)
            pares |= {(hx[q][0], hp[q][0]) for q in hx if len(hx[q]) == 1 and len(hp.get(q, ())) == 1}
        cx = collections.Counter(a for a, b in pares)
        cp = collections.Counter(b for a, b in pares)
        return sorted((a, b) for a, b in pares if cx[a] == 1 and cp[b] == 1)

    def dp(gx, gp):
        sc = {}
        lx = [len(D['nx'][a]) for a in gx]
        for j, b in enumerate(gp):
            sb = D['np'][b]
            lb = len(sb)
            if not lb:
                continue
            m = difflib.SequenceMatcher(None, autojunk=False)
            m.set_seq2(sb)
            for i, a in enumerate(gx):
                la = lx[i]
                if not la or la > 2 * lb + 2 or lb > 2 * la + 2:
                    continue
                m.set_seq1(D['nx'][a])
                if m.real_quick_ratio() >= UMIN and m.quick_ratio() >= UMIN:
                    r = m.ratio()
                    if r >= UMIN:
                        sc[i, j] = r
        fila, col = collections.defaultdict(list), collections.defaultdict(list)
        for (i, j), r in sc.items():
            fila[i].append((r, j))
            col[j].append((r, i))
        nx, np_ = len(gx), len(gp)
        V = [[0.0] * (np_ + 1) for _ in range(nx + 1)]
        B = [bytearray(np_ + 1) for _ in range(nx + 1)]
        for i in range(1, nx + 1):
            Vi, Vp, Bi = V[i], V[i - 1], B[i]
            fi = {j: r for r, j in fila.get(i - 1, ())}
            for j in range(1, np_ + 1):
                best, bb = Vp[j], 1
                if Vi[j - 1] > best:
                    best, bb = Vi[j - 1], 2
                r = fi.get(j - 1)
                if r is not None and Vp[j - 1] + r - UMIN > best:
                    best, bb = Vp[j - 1] + r - UMIN, 3
                Vi[j], Bi[j] = best, bb
        out, i, j = [], nx, np_
        while i > 0 and j > 0:
            b = B[i][j]
            if b == 3:
                r = sc[i - 1, j - 1]
                seg = max([v for v, jj in fila[i - 1] if jj != j - 1] + [v for v, ii in col[j - 1] if ii != i - 1] + [0.0])
                out.append((gx[i - 1], gp[j - 1], r, r - seg))
                i, j = i - 1, j - 1
            elif b == 1:
                i -= 1
            else:
                j -= 1
        return out[::-1]

    pares, usados = {}, set()

    def pon(a, b, r=1.0, m=1.0):
        nivel = 'raw' if D['rx'][a] == D['rp'][b] else ('norm' if D['nx'][a] == D['np'][b] else
                                                          ('casi' if r >= 0.9 else ('parecida' if r >= 0.7 else 'debil')))
        pares[a] = (b, nivel, r, m)
        usados.add(b)

    def rellena(L, libx, libp, cerca):
        bordes = [(xs[0] - 1, ps[0] - 1)] + L + [(xs[-1] + 1, ps[-1] + 1)]
        for (x1, p1), (x2, p2) in zip(bordes, bordes[1:]):
            if cerca and (x2 - x1 > 0x20000 or p2 - p1 > 0x40000):
                continue
            gx = [a for a in libx[bisect.bisect_right(libx, x1):bisect.bisect_left(libx, x2)] if a not in pares]
            gp = [b for b in libp[bisect.bisect_right(libp, p1):bisect.bisect_left(libp, p2)] if b not in usados]
            if not gx or not gp:
                continue
            hx, hp = collections.defaultdict(list), collections.defaultdict(list)
            for a in gx:
                if len(D['nx'][a]) >= 3:
                    hx[tuple(D['nx'][a])].append(a)
            for b in gp:
                if len(D['np'][b]) >= 3:
                    hp[tuple(D['np'][b])].append(b)
            loc = lis(sorted((hx[q][0], hp[q][0]) for q in hx if len(hx[q]) == 1 and len(hp.get(q, ())) == 1))
            for a, b in loc:
                pon(a, b)
            sub = [(x1, p1)] + loc + [(x2, p2)]
            for (y1, q1), (y2, q2) in zip(sub, sub[1:]):
                hx_ = [a for a in gx if y1 < a < y2]
                hp_ = [b for b in gp if q1 < b < q2]
                if hx_ and hp_ and len(hx_) * len(hp_) <= 4000000:
                    for a, b, r, m in dp(hx_, hp_):
                        pon(a, b, r, m)
    L = lis(anclas())
    for a, b in L:
        pon(a, b)
    rellena(L, xs, ps, False)
    for _ in range(4):
        libx = [a for a in xs if a not in pares]
        libp = [b for b in ps if b not in usados]
        sx, sp = set(libx), set(libp)
        L2 = lis([(a, b) for a, b in anclas() if a in sx and b in sp])
        if not L2:
            break
        antes = len(pares)
        for a, b in L2:
            pon(a, b)
        rellena(L2, libx, libp, True)
        if len(pares) - antes < 20:
            break
    return pares


def valida(D, P, xbe):
    x = XBE(xbe)
    pc = PE32(os.path.join(RAIZ, 'orig', 'SPEED_EXE_1_3', 'speed.exe'))
    imp = re.compile(rb'[\x20-\x7e]{6,}\0')
    xsec = [(va, va + len(b), b) for va, b in (x.data('.rdata'), x.data('.data'))]

    def xs(v):
        for lo, hi, b in xsec:
            if lo <= v < hi:
                m = imp.match(b, v - lo)
                return m.group()[:-1] if m else None

    def ps(v):
        if 0x8A0000 <= v < PHI:
            m = imp.match(pc.lee(v, 256) or b'')
            return m.group()[:-1] if m else None

    def refs(lee, funcs, cad):
        out = collections.defaultdict(set)
        for a, z in funcs:
            b = lee(a, z) or b''
            for k in range(len(b) - 3):
                s = cad(struct.unpack_from('<I', b, k)[0])
                if s:
                    out[s].add(a)
        return out
    rx = refs(x.read, D['fx'], xs)
    rp = refs(pc.lee, [(a, z) for a, z, n in D['fp']], ps)
    verdad = collections.defaultdict(set)
    for s, fa in rx.items():
        if len(fa) == 1 and len(rp.get(s, ())) == 1:
            verdad[next(iter(fa))].add(next(iter(rp[s])))
    c = collections.defaultdict(lambda: [0, 0])
    for a, v in verdad.items():
        if len(v) == 1:
            k = P[a][1] if a in P else '-'
            c[k][0] += 1
            c[k][1] += a in P and P[a][0] in v
    return c


def cualificado(dem):
    """'public: virtual void __thiscall A::B(int)' o 'A::B(int)' -> 'A::B'."""
    if not dem:
        return None
    i = dem.find('(')
    t = dem[:i] if i > 0 else dem
    return t.split(' ')[-1] if ' ' in t and not t.endswith('operator') else t


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--xbe', required=True)
    ap.add_argument('--informe-pc', default=os.path.join(RAIZ, 'build', 'versiones', 'SPEED_EXE_1_3.json'))
    ap.add_argument('--informe-gc', default=os.path.join(RAIZ, 'build', 'versiones', 'GOWE69.json'))
    ap.add_argument('--salida', default=os.path.join(RAIZ, 'build', 'a138'))
    ap.add_argument('--cache', action='store_true', help='guarda/reusa los esqueletos en --salida/carga.pkl')
    a = ap.parse_args()
    os.makedirs(a.salida, exist_ok=True)
    D = carga(a.xbe, os.path.join(a.salida, 'carga.pkl') if a.cache else None)
    P = alinea(D)
    print('funciones Xbox %d, PC %d, parejas %d' % (len(D['fx']), len(D['fp']), len(P)))
    print('validacion por cadenas unicas (nivel: parejas, bien):',
          {k: '%d/%d' % (v[1], v[0]) for k, v in sorted(valida(D, P, a.xbe).items())})
    sp = json.load(open(os.path.join(RAIZ, 'config', 'SPEED_EXE_1_3', 'splits.json'), encoding='utf-8'))
    rangos = sorted((ini, fin, u) for u, v in sp.items() for ini, fin in v.get('function_ranges', []))
    rini = [r[0] for r in rangos]

    def unidad(va):
        i = bisect.bisect_right(rini, va) - 1
        return rangos[i][2] if i >= 0 and rangos[i][0] <= va < rangos[i][1] else '-'
    p2 = {v[0]: (xa,) + v[1:] for xa, v in P.items()}
    hx = set(tuple(s) for s in D['nx'].values() if len(s) >= 3)
    est = {}
    for u in json.load(open(a.informe_pc))['units']:
        for f in u.get('functions', []):
            md = f.get('metadata', {})
            est[int(md['virtual_address'])] = (f['name'], f.get('fuzzy_match_percent'), cualificado(md.get('demangled_name')) or f['name'].lstrip('_'))
    gc = collections.defaultdict(list)
    if os.path.exists(a.informe_gc):
        for u in json.load(open(a.informe_gc))['units']:
            for f in u.get('functions', []):
                gc[cualificado(f.get('metadata', {}).get('demangled_name')) or f['name']].append(f.get('fuzzy_match_percent'))
    nuestro = {}
    for p in glob.glob(os.path.join(RAIZ, 'build', 'SPEED_EXE_1_3', 'src', '**', '*.obj'), recursive=True):
        for k, v in lee_obj(p).items():
            nuestro.setdefault(k, v)
    pc = PE32(os.path.join(RAIZ, 'orig', 'SPEED_EXE_1_3', 'speed.exe'))
    tab = collections.defaultdict(lambda: collections.defaultdict(lambda: [0, 0]))
    cruce = collections.defaultdict(lambda: [0, 0])
    fg, fb, ft = [], [], []
    for va, z, n in D['fp']:
        u = unidad(va)
        if va in p2:
            xa, nv, r, m = p2[va]
        else:
            xa, nv, r, m = None, ('igual_fuera' if len(D['np'][va]) >= 3 and tuple(D['np'][va]) in hx else '-'), 0.0, 0.0
        tab[u][nv][0] += 1
        tab[u][nv][1] += z
        e = est.get(va)
        st = 'fuera_del_informe' if e is None else ('casa' if e[1] == 100.0 else (
            ('sub_' if AUTO.match(e[0]) else 'nombre_sin_objeto') if e[1] is None else 'parcial'))
        cruce[nv, st][0] += 1
        cruce[nv, st][1] += z
        fg.append('0x%X\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%.3f\t%.3f' % (va, z, n, u, st, '' if not e or e[1] is None else '%.1f' % e[1],
                                                                   '' if xa is None else '0x%X' % xa, nv, r, m))
        if st not in ('parcial', 'nombre_sin_objeto') or xa is None:
            continue
        g = gc.get(e[2])
        sgc = 'no_en_gc' if not g else ('gc_varias' if len(g) > 1 else ('gc_100' if g[0] == 100.0 else ('gc_sin' if g[0] is None else 'gc_%.0f' % g[0])))
        if nv in ('raw', 'norm'):
            tipo = ''
            if st == 'parcial' and n in nuestro:
                rn = esqueletos(nuestro[n], 0, PLO, PHI)[0]
                rp = D['rp'][va]
                tipo = 'longitud %+d' % (len(rn) - len(rp))
                if len(rn) == len(rp):
                    cs = set()
                    for t1, t2 in zip(rn, rp):
                        if t1 != t2:
                            m1, o1 = (t1.split(' ', 1) + [''])[:2]
                            m2, o2 = (t2.split(' ', 1) + [''])[:2]
                            cs.add('otra' if m1 != m2 else ('registros' if re.sub(r'\b(?:e?[abcd]x|[abcd][lh]|e?[sd]i|e?[sb]p)\b', 'R', o1) ==
                                                            re.sub(r'\b(?:e?[abcd]x|[abcd][lh]|e?[sd]i|e?[sb]p)\b', 'R', o2) else
                                                            ('constantes' if re.sub(r'\b(?:0x[0-9a-f]+|\d+)\b', 'N', o1) ==
                                                             re.sub(r'\b(?:0x[0-9a-f]+|\d+)\b', 'N', o2) else 'otra')))
                    tipo = ('identico_esqueleto' if not cs else ('orden' if sorted(rn) == sorted(rp) else '+'.join(sorted(cs))))
            fb.append((z, u, va, n, st, '' if e[1] is None else '%.1f' % e[1], xa, nv, sgc, tipo))
        elif st == 'parcial' and n in nuestro:
            nn = esqueletos(nuestro[n], 0, PLO, PHI)[1]
            spc = difflib.SequenceMatcher(None, nn, D['np'][va], autojunk=False).ratio()
            sxb = 1.0 if nn == D['nx'][xa] else difflib.SequenceMatcher(None, nn, D['nx'][xa], autojunk=False).ratio()
            c = 'nuestro_igual_xbox' if sxb == 1.0 else ('mas_cerca_xbox' if sxb >= spc + 0.05 else ('mas_cerca_pc' if spc >= sxb + 0.05 else 'igual_de_cerca'))
            ft.append((c, z, u, va, n, e[1], xa, nv, spc, sxb, sgc))
    with open(os.path.join(a.salida, 'gemelas_pc.tsv'), 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('va_pc\ttam\tnombre\tunidad\testado\tfuzzy\tva_xbox\tnivel\tparecido\tmargen\n' + '\n'.join(fg) + '\n')
    with open(os.path.join(a.salida, 'gemelas_por_unidad.tsv'), 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('unidad\tfunciones\tbytes\t' + '\t'.join('%s_fn\t%s_B' % (k, k) for k in NIVELES) + '\n')
        for u in sorted(tab, key=lambda u: -sum(v[1] for v in tab[u].values())):
            t = tab[u]
            fh.write('%s\t%d\t%d\t' % (u, sum(v[0] for v in t.values()), sum(v[1] for v in t.values())) +
                     '\t'.join('%d\t%d' % tuple(t[k]) for k in NIVELES) + '\n')
    fb.sort(key=lambda o: -o[0])
    with open(os.path.join(a.salida, 'b_iguales_sin_casar.tsv'), 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('tam\tunidad\tva_pc\tnombre\testado\tfuzzy\tva_xbox\tnivel\tgc\ttipo_diferencia\n')
        for o in fb:
            fh.write('%d\t%s\t0x%X\t%s\t%s\t%s\t0x%X\t%s\t%s\t%s\n' % o)
    ft.sort(key=lambda o: (['nuestro_igual_xbox', 'mas_cerca_xbox', 'igual_de_cerca', 'mas_cerca_pc'].index(o[0]), -o[1]))
    with open(os.path.join(a.salida, 'tres_vias.tsv'), 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('clase\ttam\tunidad\tva_pc\tnombre\tfuzzy\tva_xbox\tnivel_gemela\tnuestro_vs_pc\tnuestro_vs_xbox\tgc\n')
        for o in ft:
            fh.write('%s\t%d\t%s\t0x%X\t%s\t%.1f\t0x%X\t%s\t%.3f\t%.3f\t%s\n' % o)
    nt = sum(v[0] for v in cruce.values())
    bt = sum(v[1] for v in cruce.values())
    print('(a) funciones de PC por nivel de gemela Xbox (fn, B):')
    for k in NIVELES:
        f_, b_ = sum(cruce[k, s][0] for s in ('casa', 'parcial', 'nombre_sin_objeto', 'sub_', 'fuera_del_informe')), \
            sum(cruce[k, s][1] for s in ('casa', 'parcial', 'nombre_sin_objeto', 'sub_', 'fuera_del_informe'))
        print('  %-12s %6d (%4.1f %%) %8d B (%4.1f %%)   casa %d, parcial %d, nombre_sin_objeto %d, sub_ %d' % (
            k, f_, 100.0 * f_ / nt, b_, 100.0 * b_ / bt, cruce[k, 'casa'][0], cruce[k, 'parcial'][0], cruce[k, 'nombre_sin_objeto'][0], cruce[k, 'sub_'][0]))
    cb = collections.Counter()
    for o in fb:
        cb[o[4], o[8] == 'gc_100'] += 1
    print('(b) no casan y Xbox == speed.exe: %d fn, %d B; parciales %d (GC al 100 %%: %d), nombre sin objeto %d' % (
        len(fb), sum(o[0] for o in fb), cb['parcial', True] + cb['parcial', False], cb['parcial', True],
        cb['nombre_sin_objeto', True] + cb['nombre_sin_objeto', False]))
    ct = collections.Counter(o[0] for o in ft)
    bt3 = collections.Counter()
    for o in ft:
        bt3[o[0]] += o[1]
    print('tres vias (parciales con gemela Xbox distinta de PC):', {k: '%d fn %d B' % (ct[k], bt3[k]) for k in ct})


if __name__ == '__main__':
    main()
