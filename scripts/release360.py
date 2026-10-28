#!/usr/bin/env python3
"""release360.py -- el prototipo XENON 07.42 (config EuropeRelease, la maqueta de la venta) como ORACULO de config para
Xbox 360: que cambia entre Release y Milestone (nuestro EGM) en las mismas funciones (ronda 79, carril X360-79).

El 07.42 no tiene simbolos: no da nombres por si mismo. Lo que da es la otra configuracion del mismo programa, del mismo
dia y con el mismo compilador. Si una funcion del EGM tiene el mismo esqueleto en Release, su codigo no depende de
MILESTONE_BUILD; si NUESTRO enlace LTCG es identico al de Release y distinto del EGM, a nuestra fuente le falta lo de
Milestone (guardas EA_BUILD_A124 que eran de las dos builds milestone, Juice, nombres de asignacion...).

La imagen del 07.42 se saca del DEFAULT.XEX de desarrollo con scripts/xexdec.py (solo claves de desarrollo). Se lee
como imagen en memoria (secciones por direccion virtual), base de ejecucion 0x82000000.

    python scripts/release360.py mapa XENON.exe [--pkl F]
        trocea el .text del EGM y del 07.42 con la MISMA regla (inicios ciertos: .pdata, destinos de bl, punteros de
        .rdata/.data; y corte tras blr sin salto interno pendiente), esqueleto de cada tramo (scripts/ppcsk.py) y
        alineacion por orden con anclas unicas (LIS) y difflib entre anclas. Calibra contra symbols.txt.
        Por defecto guarda build/EUROPEGERMILESTONE/release360.pkl
    python scripts/release360.py oraculo [--tsv F]
        cada funcion de symbols.txt: igual / casi / sin pareja en Release, cruzado con su porcentaje de la medida
        oficial (build/versiones/EUROPEGERMILESTONE.json). Tabla por (nombre|fn_) x estado x porcentaje.
    python scripts/release360.py tres [--tsv F] [--n 40]
        las funciones con nombre que no casan: esqueleto NUESTRO (build/EUROPEGERMILESTONE/ltcg) frente al EGM y al
        07.42. nuestro==Release!=EGM => falta codigo de Milestone en la fuente (o en una funcion que expande).
    python scripts/release360.py cadenas [--tsv F]
        cadenas de .rdata/.data: las del EGM que el 07.42 no tiene son de Milestone; las que tampoco tiene nuestro
        enlace son codigo de Milestone sin escribir (FALTA), con su direccion en el EGM.
    python scripts/release360.py expansiones [--n 45]
        en las funciones emparejadas 'casi', llamadas que solo hace Milestone o solo Release, agregadas por destino:
        un destino con muchos llamantes que Release expande y Milestone llama tiene codigo de Milestone dentro.
    python scripts/release360.py via [--tsv F]
        nombres para fn_ del EGM por la via del 07.42: si nuestra funcion y la del 07.42 tienen la misma forma, la
        llamada k del 07.42 es la llamada k nuestra; el destino se lleva al EGM por el mapa. Control: las llamadas cuyo
        destino ya tiene nombre en el EGM (porcentaje de acierto). Solo propone (TSV), no escribe symbols.txt.
        --directo aplica la misma regla al EGM contra nuestro enlace (sin el 07.42): en la ronda 79 dio casi todo lo
        mismo (el 07.42 solo aporta las funciones que nuestra fuente tiene como Release).

Todas menos `mapa` leen el pickle de `mapa` (--pkl); --ltcg DIR lee otra copia del enlace LTCG. El exe del 07.42 va por argumento en `mapa` y queda apuntado en
el pickle. Resultados de la ronda 79: docs/analisis/2026-10-21_x36079.md.
"""
import argparse
import bisect
import collections
import difflib
import hashlib
import json
import os
import pickle
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import ppcsk  # noqa: E402

V = 'EUROPEGERMILESTONE'
EGM = os.path.join(RAIZ, 'orig', V, 'NFS.exe')
SIMS = os.path.join(RAIZ, 'config', V, 'symbols.txt')
INFORME = os.path.join(RAIZ, 'build', 'versiones', V + '.json')
LTCG = os.path.join(RAIZ, 'build', V, 'ltcg')
PKL = os.path.join(RAIZ, 'build', V, 'release360.pkl')
AYUDANTES = ('__save', '__rest')


class ExeImg(ppcsk.Exe):
    """Una imagen en memoria (la salida de xexdec.py): las secciones se leen por direccion virtual."""

    def __init__(self, ruta, base=0x82000000):
        super().__init__(ruta, base)
        self.secs = [(nm, va, vs, va, max(vs, rs)) for nm, va, vs, rp, rs in self.secs]


def texto(exe):
    va, vs = exe.seccion('.text')
    d = exe.lee(va, vs)
    return va, struct.unpack('>%dI' % (len(d) // 4), d[:len(d) // 4 * 4])


def ciertos(exe, va, ws):
    """inicios seguros: .pdata, destinos de bl y punteros alineados de .rdata/.data al .text."""
    s = set()
    pv, ps = exe.seccion('.pdata')
    pd = exe.lee(pv, ps)
    fin = va + 4 * len(ws)
    for i in range(0, len(pd) - 7, 8):
        b, = struct.unpack_from('>I', pd, i)
        if va <= b < fin:
            s.add(b)
    for i, w in enumerate(ws):
        if w >> 26 == 18 and (w & 1):
            li = w & 0x03FFFFFC
            if li & 0x02000000:
                li -= 0x04000000
            t = li if (w >> 1) & 1 else va + 4 * i + li
            if va <= t < fin:
                s.add(t)
    for nm in ('.rdata', '.data'):
        dv, dsz = exe.seccion(nm)
        dd = exe.lee(dv, dsz)
        for o in range(0, len(dd) - 3, 4):
            p, = struct.unpack_from('>I', dd, o)
            if va <= p < fin and not p & 3:
                s.add(p)
    return s


def trocea(va, ws, cs):
    """cortes en los inicios ciertos y tras un blr sin salto interno pendiente (saltando los ceros de relleno)."""
    starts = []
    cur = None
    maxt = 0
    for i, w in enumerate(ws):
        a = va + 4 * i
        if cur is None:
            if w == 0:
                continue
            cur = maxt = a
            starts.append(a)
        elif a in cs:
            cur = maxt = a
            starts.append(a)
        op = w >> 26
        if op == 16 and not (w & 1):
            bd = w & 0xFFFC
            if bd & 0x8000:
                bd -= 0x10000
            t = bd if (w >> 1) & 1 else a + bd
            maxt = max(maxt, t)
        elif op == 18 and not (w & 1):
            li = w & 0x03FFFFFC
            if li & 0x02000000:
                li -= 0x04000000
            t = li if (w >> 1) & 1 else a + li
            if t not in cs and t > maxt and t - a < 0x10000:
                maxt = t
        if w == 0x4E800020 and maxt <= a:
            cur = None
    return starts


def tramos(exe):
    va, ws = texto(exe)
    st = trocea(va, ws, ciertos(exe, va, ws))
    fin = va + 4 * len(ws)
    r = []
    for k, s in enumerate(st):
        e = st[k + 1] if k + 1 < len(st) else fin
        j = (e - va) // 4
        while j > (s - va) // 4 + 1 and ws[j - 1] == 0:
            j -= 1
        r.append((s, va + 4 * j - s))
    return r


def lee_simbolos():
    rx = re.compile(r'^(\S+) = \.text:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)')
    out = []
    for l in open(SIMS, encoding='utf-8'):
        m = rx.match(l)
        if m:
            out.append((int(m.group(2), 16), int(m.group(3), 16), m.group(1)))
    return sorted(out)


def lis(pares):
    js = [j for _, j in pares]
    tails, tidx, prev = [], [], [-1] * len(js)
    for t, j in enumerate(js):
        p = bisect.bisect_left(tails, j)
        if p == len(tails):
            tails.append(j); tidx.append(t)
        else:
            tails[p] = j; tidx[p] = t
        prev[t] = tidx[p - 1] if p else -1
    out = []
    t = tidx[-1] if tidx else -1
    while t >= 0:
        out.append(pares[t]); t = prev[t]
    return out[::-1]


def cmd_mapa(a):
    egm, xen = ppcsk.Exe(EGM), ExeImg(a.xenon)
    sims = lee_simbolos()
    se, sx = tramos(egm), tramos(xen)
    ss, pe = set(x for x, _, _ in sims), set(x for x, _ in se)
    print('EGM: %d tramos; inicios de symbols.txt hallados %d de %d (%.1f %%); tramos que son inicio %.1f %%' % (
        len(se), len(ss & pe), len(ss), 100.0 * len(ss & pe) / len(ss), 100.0 * len(ss & pe) / len(pe)))
    print('XENON: %d tramos' % len(sx))
    ke = [hashlib.sha1(ppcsk.esqueleto(egm, x, n)[0]).digest()[:10] for x, n in se]
    kx = [hashlib.sha1(ppcsk.esqueleto(xen, x, n)[0]).digest()[:10] for x, n in sx]
    ce, cx = collections.Counter(ke), collections.Counter(kx)
    posx = {k: j for j, k in enumerate(kx) if cx[k] == 1}
    anc = lis([(i, posx[k]) for i, k in enumerate(ke) if ce[k] == 1 and k in posx])
    mapa = {i: (j, 'igual') for i, j in anc}
    for (i0, j0), (i1, j1) in zip([(-1, -1)] + anc, anc + [(len(ke), len(kx))]):
        x, y = ke[i0 + 1:i1], kx[j0 + 1:j1]
        if not x or not y or len(x) * len(y) > 4e6:
            continue
        for tag, a0, a1, b0, b1 in difflib.SequenceMatcher(None, x, y, autojunk=False).get_opcodes():
            if tag == 'equal' or (tag == 'replace' and a1 - a0 == b1 - b0):
                for k in range(a1 - a0):
                    mapa[i0 + 1 + a0 + k] = (j0 + 1 + b0 + k, 'igual' if tag == 'equal' else 'casi')
    c = collections.Counter(v[1] for v in mapa.values())
    print('anclas %d; tramos del EGM emparejados: igual %d, casi %d, de %d' % (len(anc), c['igual'], c['casi'], len(ke)))
    with open(a.pkl, 'wb') as f:
        pickle.dump({'se': se, 'sx': sx, 'mapa': mapa, 'sims': sims, 'xenon': os.path.abspath(a.xenon)}, f)
    print('escrito', a.pkl)


class Mapa:
    def __init__(self, pkl):
        if os.path.getmtime(SIMS) > os.path.getmtime(pkl):
            # Ronda 80: el pickle guarda los nombres de symbols.txt de cuando se hizo `mapa`; con uno viejo, `via`
            # vuelve a proponer fn_ que ya tienen nombre.
            print('AVISO: %s es mas nuevo que %s: rehaz `mapa` antes de fiarte de los fn_' % (SIMS, pkl),
                  file=sys.stderr)
        d = pickle.load(open(pkl, 'rb'))
        self.se, self.sx, self.mapa, self.sims = d['se'], d['sx'], d['mapa'], d['sims']
        self.egm, self.xen = ppcsk.Exe(EGM), ExeImg(d['xenon'])
        self.ini = {x: i for i, (x, n) in enumerate(self.se)}
        self.nom = {x: nm for x, n, nm in self.sims}
        self.sa = [x for x, _, _ in self.sims]
        self.xs = [x for x, _ in self.sx]
        self.inv = {self.sx[j][0]: self.se[i][0] for i, (j, e) in self.mapa.items()}

    def estado(self, x, n):
        """('igual'|'casi'|'sin'|'sin_tramo', va en el 07.42, tamano en el 07.42)"""
        i = self.ini.get(x)
        if i is None:
            return 'sin_tramo', 0, 0
        k, ests = i, []
        while k < len(self.se) and self.se[k][0] < x + n:
            m = self.mapa.get(k)
            ests.append(m[1] if m else 'sin')
            k += 1
        if i not in self.mapa:
            return 'sin', 0, 0
        j = self.mapa[i][0]
        jj = min(j + len(ests) - 1, len(self.sx) - 1)
        xa = self.sx[j][0]
        est = 'igual' if all(e == 'igual' for e in ests) else 'casi'
        return est, xa, self.sx[jj][0] + self.sx[jj][1] - xa

    def nombre_egm(self, t):
        k = bisect.bisect_right(self.sa, t) - 1
        return self.sims[k][2] if k >= 0 else '?'

    def nombre_xen(self, t):
        k = bisect.bisect_right(self.xs, t) - 1
        if k < 0:
            return '?'
        return self.nom.get(self.inv.get(self.sx[k][0], -1), 'XEN_%08X' % self.sx[k][0])


def informe():
    out = {}
    if os.path.exists(INFORME):
        for u in json.load(open(INFORME, encoding='utf-8'))['units']:
            for f in u.get('functions', []):
                out[f['name']] = (u['name'].split('/')[-1], f.get('fuzzy_match_percent'))
    return out


def cmd_oraculo(a):
    m, inf = Mapa(a.pkl), informe()
    t, b = collections.Counter(), collections.Counter()
    filas = []
    for x, n, nm in m.sims:
        est, xa, xn = m.estado(x, n)
        u, p = inf.get(nm, ('-', None))
        filas.append((nm, x, n, est, xa, p, u))
        c = 'sin_medir' if p is None else ('100' if p == 100.0 else ('>=90' if p >= 90 else '<90'))
        q = 'fn_' if nm.startswith('fn_') else 'nombre'
        t[(q, est, c)] += 1; b[(q, est, c)] += n
    for q in ('nombre', 'fn_'):
        print('==', q)
        for est in ('igual', 'casi', 'sin', 'sin_tramo'):
            print('  %-9s %s' % (est, '  '.join('%s: %d fn %d B' % (c, t[(q, est, c)], b[(q, est, c)])
                                               for c in ('100', '>=90', '<90', 'sin_medir'))))
    if a.tsv:
        with open(a.tsv, 'w', encoding='utf-8') as f:
            f.write('nombre\tva\ttam\testado\tva_0742\tpct\tunidad\n')
            for r in filas:
                f.write('%s\t%#x\t%d\t%s\t%#x\t%s\t%s\n' % r)


def nuestro():
    rx = re.compile(r'^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s+(f?)')
    nm, fa, porn = {}, [], collections.defaultdict(list)
    for ln in open(os.path.join(LTCG, 'NFS_ltcg.map'), encoding='latin-1'):
        r = rx.match(ln)
        if r and r.group(5) == 'f':
            va = int(r.group(4), 16)
            nm.setdefault(va, r.group(3)); fa.append(va); porn[r.group(3)].append(va)
    fa = sorted(set(fa))
    return ppcsk.Exe(os.path.join(LTCG, 'NFS_ltcg.exe'), 0x400000), nm, fa, porn


def palabras(exe, x, n):
    raw = exe.lee(x, n)
    ws = list(struct.unpack('>%dI' % (len(raw) // 4), raw[:len(raw) // 4 * 4]))
    while len(ws) > 1 and ws[-1] == 0:
        ws.pop()
    s, c, r = ppcsk.esqueleto(exe, x, 4 * len(ws))
    return list(struct.unpack('>%dI' % (len(s) // 4), s)), c


def cmd_tres(a):
    m, inf = Mapa(a.pkl), informe()
    nos, nmO, fa, porn = nuestro()
    out = []
    for x, n, nm in m.sims:
        p = inf.get(nm, ('-', None))[1]
        if nm.startswith('fn_') or p in (None, 100.0) or len(porn.get(nm, [])) != 1:
            continue
        est, xa, xn = m.estado(x, n)
        if est not in ('igual', 'casi'):
            continue
        ov = porn[nm][0]
        q = bisect.bisect_right(fa, ov)
        E, _ = palabras(m.egm, x, n)
        X, _ = palabras(m.xen, xa, xn)
        O, _ = palabras(nos, ov, (fa[q] - ov) if q < len(fa) else 64)
        rE = difflib.SequenceMatcher(None, O, E, autojunk=False).ratio()
        rX = difflib.SequenceMatcher(None, O, X, autojunk=False).ratio()
        out.append((nm, inf[nm][0], est, p, n, 4 * len(X), 4 * len(O), rE, rX, O == X, O == E))
    out.sort(key=lambda r: r[8] - r[7], reverse=True)
    print('funciones %d; nuestro == Release: %d; mas cerca de Release (> 0,05): %d; mas cerca del EGM: %d' % (
        len(out), sum(r[9] for r in out), sum(1 for r in out if r[8] - r[7] > 0.05),
        sum(1 for r in out if r[7] - r[8] > 0.05)))
    for r in out[:a.n]:
        print('%-58s %-10s %-5s %6.2f EGM %5d XEN %5d NUESTRO %5d  pEGM %.3f pXEN %.3f %s' % (
            r[0][:58], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], 'nuestro=XEN' if r[9] else ''))
    if a.tsv:
        with open(a.tsv, 'w', encoding='utf-8') as f:
            f.write('nombre\tunidad\testado\tpct\ttamEGM\ttamXEN\ttamNUESTRO\tparecidoEGM\tparecidoXEN\tnuestro==XEN\t'
                    'nuestro==EGM\n')
            for r in out:
                f.write('%s\t%s\t%s\t%.2f\t%d\t%d\t%d\t%.3f\t%.3f\t%s\t%s\n' % r)


def cadenas(exe):
    r, pos = collections.Counter(), {}
    for nm in ('.rdata', '.data'):
        va, vs = exe.seccion(nm)
        d = exe.lee(va, vs)
        for c in re.finditer(rb'[\x20-\x7e\t\n]{5,}\x00', d):
            if c.start() % 4 and d[c.start() - 1] != 0:
                continue
            s = c.group()[:-1]
            r[s] += 1
            pos.setdefault(s, va + c.start())
    return r, pos


def cmd_cadenas(a):
    m = Mapa(a.pkl)
    E, pE = cadenas(m.egm)
    X, _ = cadenas(m.xen)
    O, _ = cadenas(ppcsk.Exe(os.path.join(LTCG, 'NFS_ltcg.exe'), 0x400000))
    M = [s for s in E if s not in X]
    falta = sorted([s for s in M if s not in O], key=lambda s: pE[s])
    print('EGM %d, 07.42 %d, nuestro %d; solo Milestone %d; de esas nos faltan %d' % (len(E), len(X), len(O), len(M),
                                                                                      len(falta)))
    if a.tsv:
        with open(a.tsv, 'w', encoding='utf-8') as f:
            for s in falta:
                f.write('FALTA\t%#x\t%s\n' % (pE[s], s.decode('latin-1').replace('\n', '\\n').replace('\t', '\\t')))


def cmd_expansiones(a):
    m, inf = Mapa(a.pkl), informe()
    solo_m, solo_r, ej = collections.Counter(), collections.Counter(), {}
    for i, (j, e) in m.mapa.items():
        x, n = m.se[i]
        if e != 'casi' or x not in m.nom:
            continue
        xa, xn = m.sx[j]
        ce = collections.Counter(y for y in map(m.nombre_egm, ppcsk.esqueleto(m.egm, x, n)[1])
                                 if not y.startswith(AYUDANTES))
        cx = collections.Counter(y for y in map(m.nombre_xen, ppcsk.esqueleto(m.xen, xa, xn)[1])
                                 if not y.startswith(AYUDANTES + ('XEN_',)))
        for g in ce - cx:
            solo_m[g] += 1; ej.setdefault(g, m.nom[x])
        for g in cx - ce:
            solo_r[g] += 1
    print('== llamadas solo en MILESTONE, por destino (n llamantes, porcentaje del destino, un llamante):')
    for g, k in solo_m.most_common(a.n):
        print('  %4d  %-70s %s  %s' % (k, g[:70], inf.get(g, ('', '-'))[1], ej[g][:50]))
    print('== llamadas solo en RELEASE, por destino:')
    for g, k in solo_r.most_common(a.n // 2):
        print('  %4d  %-70s %s' % (k, g[:70], inf.get(g, ('', '-'))[1]))


def cmd_via(a):
    m = Mapa(a.pkl)
    nos, nmO, fa, porn = nuestro()
    props, ctrl, ej = collections.defaultdict(collections.Counter), collections.Counter(), {}
    for x, n, nm in m.sims:
        if nm.startswith('fn_') or len(porn.get(nm, [])) != 1:
            continue
        est, xa, xn = m.estado(x, n)
        if est not in ('igual', 'casi') and not a.directo:
            continue
        ov = porn[nm][0]
        q = bisect.bisect_right(fa, ov)
        O, cO = palabras(nos, ov, (fa[q] - ov) if q < len(fa) else 64)
        if a.directo:
            # la misma regla con el EGM en lugar del 07.42 (lo que el 07.42 aporta es la diferencia entre los dos)
            X, cX = palabras(m.egm, x, n)
        else:
            X, cX = palabras(m.xen, xa, xn)
        if len(cX) != len(cO) or (X != O and difflib.SequenceMatcher(None, X, O, autojunk=False).ratio() < 0.95):
            continue
        for t, o in zip(cX, (nmO.get(t, '?') for t in cO)):
            if o == '?' or o.startswith(AYUDANTES):
                continue
            if a.directo:
                e = t if t in m.nom else None
            else:
                k = bisect.bisect_right(m.xs, t) - 1
                if k < 0 or m.sx[k][0] != t:
                    continue
                e = m.inv.get(t)
            if e is None:
                continue
            ne = m.nom.get(e, '?')
            if ne.startswith('fn_'):
                props[e][o] += 1; ej.setdefault(e, nm)
            else:
                ctrl['coincide' if ne == o else 'distinto'] += 1
    usados = set(m.nom.values())
    libres = {e: c for e, c in props.items() if len(c) == 1 and list(c)[0] not in usados}
    tot = max(1, sum(ctrl.values()))
    print('control (destinos ya con nombre en el EGM): coinciden %d, distintos %d (%.2f %%)' % (
        ctrl['coincide'], ctrl['distinto'], 100.0 * ctrl['coincide'] / tot))
    print('fn_ con propuesta %d; unanimes con nombre libre %d' % (len(props), len(libres)))
    if a.tsv:
        with open(a.tsv, 'w', encoding='utf-8') as f:
            f.write('va\tnombre\tvotos\tllamante\n')
            for e, c in sorted(libres.items()):
                o, v = list(c.items())[0]
                f.write('%#x\t%s\t%d\t%s\n' % (e, o, v, ej[e]))


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help'):
        sys.exit(__doc__)
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    sub = ap.add_subparsers(dest='cmd')
    p = sub.add_parser('mapa'); p.add_argument('xenon'); p.add_argument('--pkl', default=PKL)
    for nombre in ('oraculo', 'tres', 'cadenas', 'expansiones', 'via'):
        p = sub.add_parser(nombre)
        p.add_argument('--pkl', default=PKL)
        p.add_argument('--tsv')
        p.add_argument('--n', type=int, default=40)
        p.add_argument('--ltcg', help='carpeta con NFS_ltcg.exe/.map (por defecto build/EUROPEGERMILESTONE/ltcg)')
        p.add_argument('--directo', action='store_true', help='via: EGM contra nuestro, sin pasar por el 07.42')
    a = ap.parse_args()
    if getattr(a, 'ltcg', None):
        global LTCG
        LTCG = a.ltcg
    {'mapa': cmd_mapa, 'oraculo': cmd_oraculo, 'tres': cmd_tres, 'cadenas': cmd_cadenas,
     'expansiones': cmd_expansiones, 'via': cmd_via}[a.cmd](a)


if __name__ == '__main__':
    main()
