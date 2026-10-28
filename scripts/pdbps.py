#!/usr/bin/env python3
"""pdbps.py lee FICHERO.pdb SALIDA.pkl [--base VA] | clase PKL NOMBRE... | proc PKL SUBCADENA... -- lector COMPLETO
de un PDB de MSVC 8 (MSF 7.00), sin dependencias, y consultas sobre lo leido.

Amplia scripts/pdb.py (que solo lee publicos y cuenta tipos) con lo que trae un PDB de compilacion completo:
  * modulos: objeto, biblioteca, S_COMPILE2 (version del front/back end), S_ENVBLOCK (cwd, cl, cmd = OPCIONES);
  * procedimientos: S_GPROC32/S_LPROC32 con direccion, longitud, tipo, locales (S_REGREL32/S_BPREL32/S_REGISTER,
    con REGISTRO y desplazamiento), bloques y estaticos locales;
  * lineas C13 (DEBUG_S_LINES + DEBUG_S_FILECHKSMS + /names): fichero fuente y linea de cada tramo de codigo;
  * TPI: clases/estructuras/uniones/enums con sus miembros, desplazamientos, bases, vfptr, metodos y tamano.
Salida de `lee`: un pickle con {'modulos', 'procs', 'lineas', 'clases', 'enums', 'globales', 'secciones'}.
Escrito para el PDB del prototipo de NFS ProStreet de Xbox 360 (ronda 76, docs/analisis/2026-10-21_prostreet.md:
81.648 procedimientos, 168.055 locales, 13.623 clases en 3 s); vale para cualquier PDB de MSVC 7/8.

    python scripts/pdbps.py lee Juego.pdb juego.pkl                 # direcciones con base 0x82000000 (Xbox 360)
    python scripts/pdbps.py lee speed.pdb speed.pkl --base 0x400000  # PC
    python scripts/pdbps.py clase juego.pkl SNDPLAYOPTS PhysicsObject  # miembros con desplazamiento y tamano
    python scripts/pdbps.py proc juego.pkl NotifySkyLoader            # direccion, longitud, modulo, fichero y lineas

Los datos del PDB son del usuario: el pickle va fuera del repo (no lo subas a git).
"""
import argparse
import os
import pickle
import struct
import sys
import collections

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pdb import Pdb, cstr  # noqa: E402  (scripts/pdb.py, no el depurador de Python)

PRIM = {0x03: 'void', 0x10: 'char', 0x20: 'unsigned char', 0x11: 'short', 0x21: 'unsigned short', 0x12: 'long',
        0x22: 'unsigned long', 0x13: '__int64', 0x23: 'unsigned __int64', 0x40: 'float', 0x41: 'double',
        0x42: 'long double', 0x68: 'signed char', 0x69: 'unsigned char', 0x70: 'char', 0x71: 'wchar_t',
        0x72: 'short', 0x73: 'unsigned short', 0x74: 'int', 0x75: 'unsigned int', 0x76: '__int64',
        0x77: 'unsigned __int64', 0x30: 'bool', 0x31: 'bool16', 0x32: 'bool32', 0x00: 'NOTYPE', 0x08: 'HRESULT',
        0x7a: 'char16_t', 0x7b: 'char32_t'}


def numeric(b, off):
    v, = struct.unpack_from('<H', b, off)
    if v < 0x8000:
        return v, off + 2
    fmt = {0x8000: ('<b', 1), 0x8001: ('<h', 2), 0x8002: ('<H', 2), 0x8003: ('<i', 4), 0x8004: ('<I', 4),
           0x8009: ('<q', 8), 0x800a: ('<Q', 8)}.get(v)
    if fmt is None:
        return None, off + 2
    x, = struct.unpack_from(fmt[0], b, off + 2)
    return x, off + 2 + fmt[1]


class Tipos:
    def __init__(self, tpi):
        ver, hsz, self.tmin, self.tmax, tbytes = struct.unpack_from('<IIIII', tpi, 0)
        self.rec = {}
        off = hsz
        i = self.tmin
        while off + 4 <= len(tpi) and i < self.tmax:
            ln, kind = struct.unpack_from('<HH', tpi, off)
            self.rec[i] = (kind, tpi[off + 4:off + 2 + ln])
            off += 2 + ln
            i += 1
        self._cls_por_nombre = {}
        for ti, (k, r) in self.rec.items():
            if k in (0x1504, 0x1505, 0x1506, 0x1507):
                d = self.udt(ti)
                if d and not d['fwd']:
                    self._cls_por_nombre.setdefault(d['name'], ti)

    def udt(self, ti):
        k, r = self.rec[ti]
        if k in (0x1504, 0x1505):
            count, prop, field, derived, vshape = struct.unpack_from('<HHIII', r, 0)
            size, o = numeric(r, 16)
            name = cstr(r, o)
            return {'kind': 'class' if k == 0x1504 else 'struct', 'count': count, 'prop': prop, 'field': field,
                    'size': size, 'name': name, 'fwd': bool(prop & 0x80), 'vshape': vshape}
        if k == 0x1506:
            count, prop, field = struct.unpack_from('<HHI', r, 0)
            size, o = numeric(r, 8)
            return {'kind': 'union', 'count': count, 'prop': prop, 'field': field, 'size': size,
                    'name': cstr(r, o), 'fwd': bool(prop & 0x80)}
        if k == 0x1507:
            count, prop, utype, field = struct.unpack_from('<HHII', r, 0)
            return {'kind': 'enum', 'count': count, 'prop': prop, 'field': field, 'size': None, 'utype': utype,
                    'name': cstr(r, 12), 'fwd': bool(prop & 0x80)}
        return None

    def resuelve(self, ti):
        """De una referencia adelantada a la definicion completa (por nombre)."""
        if ti in self.rec:
            d = self.udt(ti)
            if d and d['fwd']:
                return self._cls_por_nombre.get(d['name'], ti)
        return ti

    def nombre(self, ti, prof=0):
        if prof > 12:
            return '?'
        if ti < 0x1000:
            base = PRIM.get(ti & 0xFF, 'prim%#x' % (ti & 0xFF))
            return base + ('*' if (ti >> 8) & 7 else '')
        if ti not in self.rec:
            return 'T%#x' % ti
        k, r = self.rec[ti]
        if k == 0x1002:
            ut, attr = struct.unpack_from('<II', r, 0)
            mode = (attr >> 5) & 7
            return self.nombre(ut, prof + 1) + ('&' if mode == 1 else '*')
        if k == 0x1001:
            ut, mod = struct.unpack_from('<IH', r, 0)
            return ('const ' if mod & 1 else '') + ('volatile ' if mod & 2 else '') + self.nombre(ut, prof + 1)
        if k == 0x1503:
            et, it = struct.unpack_from('<II', r, 0)
            size, o = numeric(r, 8)
            esz = self.tam(et)
            n = size // esz if esz else size
            return '%s[%s]' % (self.nombre(et, prof + 1), n)
        if k in (0x1504, 0x1505, 0x1506, 0x1507):
            return self.udt(ti)['name']
        if k == 0x1205:
            t, ln, pos = struct.unpack_from('<IBB', r, 0)
            return '%s:%d@%d' % (self.nombre(t, prof + 1), ln, pos)
        if k == 0x1008:
            rt, cc, attr, np, al = struct.unpack_from('<IBBHI', r, 0)
            return '%s(*)(%s)' % (self.nombre(rt, prof + 1), self.args(al, prof + 1))
        if k == 0x1009:
            rt, cls, this, cc, attr, np, al, adj = struct.unpack_from('<IIIBBHIi', r, 0)
            return '%s(%s::*)(%s)' % (self.nombre(rt, prof + 1), self.nombre(cls, prof + 1), self.args(al, prof + 1))
        return 'K%#x' % k

    def args(self, al, prof=0):
        if al not in self.rec or self.rec[al][0] != 0x1201:
            return '?'
        r = self.rec[al][1]
        n, = struct.unpack_from('<I', r, 0)
        return ', '.join(self.nombre(x, prof + 1) for x in struct.unpack_from('<%dI' % n, r, 4))

    def tam(self, ti):
        if ti < 0x1000:
            if (ti >> 8) & 7:
                return 4
            b = ti & 0xFF
            return {0x03: 0, 0x10: 1, 0x20: 1, 0x68: 1, 0x69: 1, 0x70: 1, 0x30: 1, 0x11: 2, 0x21: 2, 0x72: 2,
                    0x73: 2, 0x71: 2, 0x12: 4, 0x22: 4, 0x74: 4, 0x75: 4, 0x40: 4, 0x32: 4, 0x13: 8, 0x23: 8,
                    0x76: 8, 0x77: 8, 0x41: 8, 0x42: 8}.get(b, 0)
        if ti not in self.rec:
            return 0
        k, r = self.rec[ti]
        if k == 0x1002:
            return 4
        if k == 0x1001:
            return self.tam(struct.unpack_from('<I', r, 0)[0])
        if k == 0x1503:
            return numeric(r, 8)[0]
        if k in (0x1504, 0x1505, 0x1506):
            t2 = self.resuelve(ti)
            return self.udt(t2)['size']
        if k == 0x1507:
            return self.tam(self.udt(ti)['utype'])
        return 0

    def campos(self, fl):
        """Lista de entradas de un LF_FIELDLIST (sigue LF_INDEX)."""
        out = []
        vistos = set()
        while fl in self.rec and fl not in vistos:
            vistos.add(fl)
            k, r = self.rec[fl]
            if k != 0x1203:
                break
            off = 0
            sig = None
            while off + 2 <= len(r):
                if r[off] >= 0xF0:
                    off += max(1, r[off] & 0x0F)
                    continue
                lk, = struct.unpack_from('<H', r, off)
                o = off + 2
                if lk == 0x150D:  # LF_MEMBER
                    attr, t = struct.unpack_from('<HI', r, o)
                    val, o = numeric(r, o + 6)
                    nm = cstr(r, o)
                    out.append(('member', nm, val, t, attr))
                    o += len(nm.encode('latin-1')) + 1
                elif lk == 0x1400:  # LF_BCLASS
                    attr, t = struct.unpack_from('<HI', r, o)
                    val, o = numeric(r, o + 6)
                    out.append(('base', None, val, t, attr))
                elif lk in (0x1401, 0x1402):
                    attr, t, vbpt = struct.unpack_from('<HII', r, o)
                    v1, o = numeric(r, o + 10)
                    v2, o = numeric(r, o)
                    out.append(('vbase', None, v1, t, attr))
                elif lk == 0x1409:  # LF_VFUNCTAB
                    pad, t = struct.unpack_from('<HI', r, o)
                    out.append(('vfptr', None, 0, t, 0))
                    o += 6
                elif lk == 0x150E:  # LF_STMEMBER
                    attr, t = struct.unpack_from('<HI', r, o)
                    nm = cstr(r, o + 6)
                    out.append(('static', nm, None, t, attr))
                    o += 6 + len(nm.encode('latin-1')) + 1
                elif lk == 0x150F:  # LF_METHOD
                    cnt, ml = struct.unpack_from('<HI', r, o)
                    nm = cstr(r, o + 6)
                    out.append(('method', nm, cnt, ml, 0))
                    o += 6 + len(nm.encode('latin-1')) + 1
                elif lk == 0x1511:  # LF_ONEMETHOD
                    attr, t = struct.unpack_from('<HI', r, o)
                    o += 6
                    mprop = (attr >> 2) & 7
                    vofs = None
                    if mprop in (4, 6):
                        vofs, = struct.unpack_from('<I', r, o)
                        o += 4
                    nm = cstr(r, o)
                    out.append(('onemethod', nm, vofs, t, attr))
                    o += len(nm.encode('latin-1')) + 1
                elif lk == 0x1510:  # LF_NESTTYPE
                    pad, t = struct.unpack_from('<HI', r, o)
                    nm = cstr(r, o + 6)
                    out.append(('nested', nm, None, t, 0))
                    o += 6 + len(nm.encode('latin-1')) + 1
                elif lk == 0x1502:  # LF_ENUMERATE
                    attr, = struct.unpack_from('<H', r, o)
                    val, o = numeric(r, o + 2)
                    nm = cstr(r, o)
                    out.append(('enumerate', nm, val, None, attr))
                    o += len(nm.encode('latin-1')) + 1
                elif lk == 0x1404:  # LF_INDEX
                    pad, sig = struct.unpack_from('<HI', r, o)
                    o += 6
                else:
                    out.append(('?%#x' % lk, None, None, None, None))
                    break
                off = o
            if sig is None:
                break
            fl = sig
        return out


REG_PPC = {**{i: 'r%d' % i for i in range(32)}}


def lee_modulo(pdb, sym, c13, nombres_str):
    """Devuelve (compile, env, objname, procs, lineas) de un flujo de modulo."""
    comp, env, objname = None, None, None
    procs = []
    pila = []
    off = 4
    while off + 4 <= len(sym):
        ln, kind = struct.unpack_from('<HH', sym, off)
        if ln < 2:
            break
        r = sym[off + 4:off + 2 + ln]
        here = off
        off += 2 + ln
        try:
            if kind == 0x1101:
                objname = cstr(r, 4)
            elif kind == 0x1116:  # S_COMPILE2
                flags, mach, fa, fb, fc, ba, bb, bc = struct.unpack_from('<IH6H', r, 0)
                comp = {'lang': flags & 0xFF, 'flags': flags >> 8, 'machine': mach,
                        'fe': '%d.%02d.%d' % (fa, fb, fc), 'be': '%d.%02d.%d' % (ba, bb, bc), 'ver': cstr(r, 18)}
            elif kind == 0x113C:  # S_COMPILE3
                flags, mach = struct.unpack_from('<IH', r, 0)
                v = struct.unpack_from('<8H', r, 6)
                comp = {'lang': flags & 0xFF, 'flags': flags >> 8, 'machine': mach,
                        'fe': '%d.%02d.%d.%d' % v[:4], 'be': '%d.%02d.%d.%d' % v[4:], 'ver': cstr(r, 22)}
            elif kind == 0x113D:  # S_ENVBLOCK
                o = 1
                kv = []
                while o < len(r) and r[o] != 0:
                    s = cstr(r, o)
                    kv.append(s)
                    o += len(s.encode('latin-1')) + 1
                env = dict(zip(kv[0::2], kv[1::2]))
            elif kind in (0x110F, 0x1110):  # S_LPROC32 / S_GPROC32
                pPar, pEnd, pNext, plen, ds, de, ti, poff, seg, fl = struct.unpack_from('<6IIIHB', r, 0)
                p = {'name': cstr(r, 35), 'seg': seg, 'off': poff, 'len': plen, 'type': ti, 'global': kind == 0x1110,
                     'locals': [], 'blocks': 0, 'statics': [], 'end': pEnd, 'dbg': (ds, de)}
                procs.append(p)
                pila.append(('proc', p, pEnd))
            elif kind == 0x1103:  # S_BLOCK32
                pPar, pEnd = struct.unpack_from('<II', r, 0)
                if pila:
                    for e in reversed(pila):
                        if e[0] == 'proc':
                            e[1]['blocks'] += 1
                            break
                pila.append(('block', None, pEnd))
            elif kind == 0x0006:  # S_END
                if pila:
                    pila.pop()
            elif kind in (0x1111, 0x110B, 0x1106, 0x110C, 0x1112):
                cur = None
                for e in reversed(pila):
                    if e[0] == 'proc':
                        cur = e[1]
                        break
                if cur is None:
                    continue
                prof = sum(1 for e in pila if e[0] == 'block')
                if kind == 0x1111:
                    o, ti, reg = struct.unpack_from('<IIH', r, 0)
                    cur['locals'].append(('regrel', cstr(r, 10), ti, reg, struct.unpack('<i', struct.pack('<I', o))[0], prof))
                elif kind == 0x110B:
                    o, ti = struct.unpack_from('<iI', r, 0)
                    cur['locals'].append(('bprel', cstr(r, 8), ti, None, o, prof))
                elif kind == 0x1106:
                    ti, reg = struct.unpack_from('<IH', r, 0)
                    cur['locals'].append(('reg', cstr(r, 6), ti, reg, None, prof))
                elif kind in (0x110C, 0x1112):
                    ti, o, sg = struct.unpack_from('<IIH', r, 0)
                    cur['statics'].append((cstr(r, 10), ti, sg, o))
        except (struct.error, ValueError, IndexError):
            pass
    # lineas C13
    lineas = []  # (seg, off, cb, fichero, [(delta, linea)])
    fich = {}
    o = 0
    subs = []
    while o + 8 <= len(c13):
        t, ln = struct.unpack_from('<II', c13, o)
        subs.append((t, c13[o + 8:o + 8 + ln]))
        o += 8 + ((ln + 3) & ~3)
    for t, d in subs:
        if t == 0xF4:
            q = 0
            while q + 6 <= len(d):
                noff, cb, ty = struct.unpack_from('<IBB', d, q)
                fich[q] = nombres_str(noff)
                q = (q + 6 + cb + 3) & ~3
    for t, d in subs:
        if t == 0xF2 and len(d) >= 12:
            offc, segc, flg, cbc = struct.unpack_from('<IHHI', d, 0)
            q = 12
            while q + 12 <= len(d):
                fid, nl, cbb = struct.unpack_from('<III', d, q)
                ls = []
                for j in range(nl):
                    lo, lf = struct.unpack_from('<II', d, q + 12 + 8 * j)
                    ls.append((lo, lf & 0xFFFFFF))
                lineas.append((segc, offc, cbc, fich.get(fid, '?%d' % fid), ls))
                q += cbb if cbb else 12 + 8 * nl
    return comp, env, objname, procs, lineas


def tabla_nombres(pdb):
    """El flujo /names (tabla de cadenas de los ficheros fuente)."""
    s1 = pdb.stream(1)
    ver, sig, age = struct.unpack_from('<III', s1, 0)
    o = 28
    blen, = struct.unpack_from('<I', s1, o)
    buf = s1[o + 4:o + 4 + blen]
    o += 4 + blen
    size, cap = struct.unpack_from('<II', s1, o)
    o += 8
    nw, = struct.unpack_from('<I', s1, o)
    present = struct.unpack_from('<%dI' % nw, s1, o + 4)
    o += 4 + 4 * nw
    nd, = struct.unpack_from('<I', s1, o)
    o += 4 + 4 * nd
    mapa = {}
    for i in range(cap):
        if present[i // 32] >> (i % 32) & 1:
            k, v = struct.unpack_from('<II', s1, o)
            o += 8
            mapa[cstr(buf, k)] = v
    st = pdb.stream(mapa['/names']) if '/names' in mapa else b''
    if not st:
        return lambda x: '?', mapa
    mg, vv, bl = struct.unpack_from('<III', st, 0)
    sb = st[12:12 + bl]
    return (lambda x: cstr(sb, x) if x < len(sb) else '?'), mapa


def lee_modinfo(pdb):
    h, s = pdb.dbi()
    off, fin = 64, 64 + h['ModInfoSize']
    out = []
    while off < fin:
        flags, symstream, symbytes, c11, c13, nsrc = struct.unpack_from('<HHIIIH', s, off + 4 + 28)
        sc = struct.unpack_from('<HHIIIHH', s, off + 4)
        p = off + 64
        e1 = s.index(b'\0', p)
        nombre = s[p:e1].decode('latin-1')
        e2 = s.index(b'\0', e1 + 1)
        obj = s[e1 + 1:e2].decode('latin-1')
        off = (e2 + 1 + 3) & ~3
        out.append({'name': nombre, 'lib': obj, 'stream': symstream, 'sym': symbytes, 'c11': c11, 'c13': c13})
    return out


def lee(ruta, salida, base=0x82000000):
    pdb = Pdb(ruta)
    secs = pdb.secciones()

    def va(seg, off):
        return base + secs[seg - 1][1] + off if 0 < seg <= len(secs) else None

    nstr, mapa = tabla_nombres(pdb)
    mods = lee_modinfo(pdb)
    procs = []
    lineas = []
    import time
    t0 = time.time()
    for mi, m in enumerate(mods):
        if mi % 100 == 0:
            print('modulo', mi, round(time.time() - t0, 1), flush=True)
        if m['stream'] == 0xFFFF:
            continue
        st = pdb.stream(m['stream'])
        sym = st[:m['sym']]
        c13 = st[m['sym'] + m['c11']:m['sym'] + m['c11'] + m['c13']]
        comp, env, objname, ps, ls = lee_modulo(pdb, sym, c13, nstr)
        m['compile'], m['env'], m['objname'] = comp, env, objname
        m['nprocs'] = len(ps)
        for p in ps:
            p['va'] = va(p['seg'], p['off'])
            p['mod'] = mi
            procs.append(p)
        for (sg, of, cb, f, l) in ls:
            lineas.append((va(sg, of), cb, f, l, mi))
    print('tipos...', round(time.time() - t0, 1), flush=True)
    tipos = Tipos(pdb.stream(2))
    print('tipos leidos', round(time.time() - t0, 1), flush=True)
    clases = {}
    enums = {}
    for ti in tipos.rec:
        d = tipos.udt(ti)
        if not d or d['fwd']:
            continue
        if d['kind'] == 'enum':
            enums.setdefault(d['name'], [(c[1], c[2]) for c in tipos.campos(d['field']) if c[0] == 'enumerate'])
            continue
        if d['name'] in clases:
            continue
        cs = tipos.campos(d['field'])
        miembros = []
        for c in cs:
            if c[0] == 'member':
                miembros.append((c[2], c[1], tipos.nombre(c[3]), tipos.tam(c[3])))
            elif c[0] in ('base', 'vbase'):
                miembros.append((c[2], '<base>', tipos.nombre(c[3]), tipos.tam(c[3])))
            elif c[0] == 'vfptr':
                miembros.append((0, '<vfptr>', 'void**', 4))
        metodos = [c[1] for c in cs if c[0] in ('method', 'onemethod')]
        estaticos = [(c[1], tipos.nombre(c[3])) for c in cs if c[0] == 'static']
        clases[d['name']] = {'kind': d['kind'], 'size': d['size'], 'members': miembros, 'methods': metodos,
                             'statics': estaticos, 'ti': ti}
    # tipos de los procs y locales en texto
    for p in procs:
        p['typestr'] = tipos.nombre(p['type']) if p['type'] else ''
        p['locals'] = [(k, n, tipos.nombre(t), r, o, pr) for (k, n, t, r, o, pr) in p['locals']]
        p['statics'] = [(n, tipos.nombre(t)) for (n, t, sg, o) in p['statics']]
    globales = []
    for kind, r in pdb.simbolos_globales():
        if kind in (0x110D, 0x110C):
            ti, o, sg = struct.unpack_from('<IIH', r, 0)
            globales.append((cstr(r, 10), va(sg, o), tipos.nombre(ti), kind == 0x110D))
    out = {'modulos': mods, 'procs': procs, 'lineas': lineas, 'clases': clases, 'enums': enums,
           'globales': globales, 'secciones': secs, 'flujos_con_nombre': mapa}
    pickle.dump(out, open(salida, 'wb'))
    print('modulos %d, procs %d, tramos de lineas %d, clases %d, enums %d, globales %d' % (
        len(mods), len(procs), len(lineas), len(clases), len(enums), len(globales)))


def _consulta_clase(d, nombres):
    for q in nombres:
        cs = [n for n in d['clases'] if n == q] or [n for n in d['clases'] if q in n][:10]
        if not cs:
            print('%s: no esta en el PDB' % q)
        for n in cs:
            c = d['clases'][n]
            print('%s %s  size 0x%X' % (c['kind'], n, c['size']))
            for off, nm, ty, sz in sorted(c['members'], key=lambda m: m[0]):
                print('   +0x%-4X %-32s %-40s 0x%X' % (off, nm, ty[:40], sz or 0))


def _consulta_proc(d, subcadenas):
    lin = {}
    for va, cb, f, ls, mi in d['lineas']:
        if va is not None:
            lin.setdefault(va, (f, sorted(set(l for o, l in ls if l))))
    for q in subcadenas:
        for p in d['procs']:
            if p['va'] and q in p['name']:
                f, ls = lin.get(p['va'], ('', []))
                mod = d['modulos'][p['mod']]['name'].replace(chr(92), '/').split('/')[-1]
                print('0x%08X %6d  %-60s %-24s %s %s-%s' % (p['va'], p['len'], p['name'][:60], mod,
                      f.replace(chr(92), '/').split('/')[-1] if f else '?', ls[0] if ls else '', ls[-1] if ls else ''))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split(chr(10))[0])
    sub = ap.add_subparsers(dest='orden', required=True)
    a = sub.add_parser('lee', help='lee FICHERO.pdb y escribe SALIDA.pkl')
    a.add_argument('pdb')
    a.add_argument('salida')
    a.add_argument('--base', default='0x82000000', help='base de carga (0x82000000 Xbox 360, 0x400000 PC)')
    c = sub.add_parser('clase', help='miembros de las clases (nombre exacto o subcadena)')
    c.add_argument('pkl')
    c.add_argument('nombres', nargs='+')
    p = sub.add_parser('proc', help='procedimientos cuyo nombre contiene la subcadena')
    p.add_argument('pkl')
    p.add_argument('subcadenas', nargs='+')
    args = ap.parse_args()
    if args.orden == 'lee':
        lee(args.pdb, args.salida, int(args.base, 0))
        return
    d = pickle.load(open(args.pkl, 'rb'))
    if args.orden == 'clase':
        _consulta_clase(d, args.nombres)
    else:
        _consulta_proc(d, args.subcadenas)


if __name__ == '__main__':
    main()
