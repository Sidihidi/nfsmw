#!/usr/bin/env python3
"""ltcgdiag360.py -- el enlace LTCG de Xbox 360 como ORACULO: que fuente esta mal, comparando el original (NFS.exe)
con nuestro enlace LTCG (scripts/ltcg360.py) funcion a funcion (ronda 76).

El compilador y el enlazador son los MISMOS (14.00.2110) y LTCG es determinista: no hay que predecir que hizo, hay que
mirar en que decide distinto con nuestro programa. Cinco diagnosticos; cada uno empareja las funciones por NOMBRE
(symbols.txt del original frente al .map de nuestro enlace; solo nombres unicos en los dos lados):

  expansiones   Para cada funcion F de los dos lados, a quien llama (bl, y b fuera de la funcion = llamada de cola).
                Una llamada a G que esta en un lado y no en el otro, con G viva en los dos, es una expansion en linea
                que solo hizo un lado (o una llamada que sobra/falta en F). Agregado por G: si muchos llamantes
                difieren igual, es G la que tiene otro tamano/IL (o es un stub opaco: LTCG no puede expandirlo).
  convencion    Convencion propia de LTCG (solo con funciones internas sin direccion tomada): entradas fuera del ABI
                (lee r0/r11/r12/r14-r31/f0/f14-f31 antes de escribirlos), no-volatiles escritos sin guardar, y en los
                LLAMANTES volatiles vivos a traves de un bl (el llamante sabe que G no los toca). En los dos lados;
                comparado por nombre. Una funcion con convencion propia no esta en ninguna vtable ni se le toma la
                direccion: sirve de filtro para nombres (un nombre virtual sobre una funcion con convencion propia es
                falso o la medida esta mal).
  supervivencia El original se enlazo con /OPT:NOREF y sin ICF (lo prueba: ~30.000 funciones sin ninguna referencia,
                y copias identicas sin plegar). Lo que rinde: los nombres del original (en unidades medidas) que el
                troceado no tiene, separados en ALIAS (??_E/??_G en la misma direccion; ltcg360 elige ya el del
                original) y en los que no estan en ninguna parte, y de esos los que tienen la misma clave
                Metodo@Clase con OTRA decoracion (firma distinta: el nombre heredado de PC no empareja nunca). Ademas
                las cadenas ??_C@ nuestras que el original no tiene, y una variante /OPT:REF (--enlazar/--ref) que con
                el arbol incompleto no separa codigo muerto de llamante que falta.
  stubs         Centralidad de los stubs opacos de ltcg360.py: cuantos llamantes tienen, cuantos no casan, cuantas
                expansiones del original deforman (el original expandio G y nosotros llamamos al stub G).
  pgo           Rich header (prodid de POGO 0x73-0x76/Cvtpgd 0x77), funciones partidas (saltos sin enlace fuera de la
                funcion que no van al principio de otra) y orden de emision por unidad.

    python scripts/ltcgdiag360.py expansiones [--min-dif 2] [--n 60] [--tsv F]
    python scripts/ltcgdiag360.py convencion [--n 40] [--tsv F]
    python scripts/ltcgdiag360.py supervivencia [--enlazar] [--ref DIR] [--n 40] [--tsv F]
    python scripts/ltcgdiag360.py stubs [--n 40] [--tsv F]
    python scripts/ltcgdiag360.py pgo

Lee build/EUROPEGERMILESTONE/ltcg/{NFS_ltcg.exe,NFS_ltcg.map,stubs.txt}, orig/EUROPEGERMILESTONE/NFS.exe,
config/EUROPEGERMILESTONE/symbols.txt y build/versiones/EUROPEGERMILESTONE.json (el porcentaje de cada funcion).
"""
import argparse
import bisect
import collections
import json
import os
import re
import struct
import subprocess
import sys
import time

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import ltcg360  # noqa: E402

V = 'EUROPEGERMILESTONE'
ORIG_BASE = 0x82000000
LTCG = os.path.join(RAIZ, 'build', V, 'ltcg')
AYUDANTES = re.compile(r'^(__savegprlr|__restgprlr|__savefpr|__restfpr|__savevmx|__restvmx|__savegpr|__restgpr)')
# r1 pila, r2 reservado, r13 TLS/datos pequenos; argumentos r3-r10 y f1-f13 (los f van de 32 a 63)
GPR_ARGS = set(range(3, 11))
FPR_ARGS = set(range(33, 46))
VOLATILES = {0} | set(range(3, 13)) | {32} | set(range(33, 46))
NOVOL = set(range(14, 32)) | set(range(46, 64))
RETORNO = {3, 33}


def rn(r):
    return 'r%d' % r if r < 32 else 'f%d' % (r - 32)


# --------------------------------------------------------------------------------------------- decodificador
# Lecturas y escrituras de GPR (0-31) y FPR (32-63) de una instruccion PowerPC (con lo de 64 bits y VMX128 del Xenon).
# Para las lecturas se prefiere quedarse corto (una lectura inventada saldria como entrada fuera del ABI).

def _ra0(a):
    return set() if a == 0 else {a}


LOAD_X = {23, 55, 87, 119, 279, 311, 343, 375, 21, 53, 341, 373, 534, 790, 532, 20, 84}
LOAD_X_UPD = {55, 119, 311, 375, 53, 373}
FLOAD_X = {535: 0, 567: 1, 599: 0, 631: 1}
STORE_X = {151, 183, 215, 247, 407, 439, 149, 181, 662, 918, 660, 150, 214}
STORE_X_UPD = {183, 247, 439, 181}
FSTORE_X = {663: 0, 695: 1, 727: 0, 759: 1, 983: 0}
VEC_X = {103, 359, 231, 487, 7, 39, 71, 135, 167, 199, 6, 38, 519, 551, 647, 679, 775, 807, 903, 935,
         278, 54, 86, 1014, 246, 982, 470, 758}
LOGIC_X = {28, 60, 444, 412, 316, 476, 124, 284, 24, 536, 792, 27, 539, 794}
UNARIO_X = {824, 954, 922, 986, 26, 58}
XO_AB = {266, 10, 138, 40, 8, 136, 235, 75, 11, 233, 73, 9, 491, 459, 489, 457}
XO_A = {202, 234, 200, 232, 104}
DESCONOCIDOS = collections.Counter()


def rw(w):
    """(lecturas, escrituras) de la instruccion w."""
    op = w >> 26
    d, a, b = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    c = (w >> 6) & 31
    if op in (14, 15):
        return _ra0(a), {d}
    if op in (7, 8, 12, 13):
        return {a}, {d}
    if op in (2, 3, 10, 11):
        return {a}, set()
    if op in (24, 25, 26, 27, 28, 29, 21):
        return {d}, {a}
    if op == 20:
        return {d, a}, {a}
    if op == 23:
        return {d, b}, {a}
    if op == 30:
        x3 = (w >> 2) & 7
        if x3 == 3:
            return {d, a}, {a}
        if x3 == 4:
            return {d, b}, {a}
        return {d}, {a}
    if op in (32, 34, 40, 42):
        return _ra0(a), {d}
    if op in (33, 35, 41, 43):
        return {a}, {d, a}
    if op in (36, 38, 44):
        return {d} | _ra0(a), set()
    if op in (37, 39, 45):
        return {d, a}, {a}
    if op == 46:
        return _ra0(a), set(range(d, 32))
    if op == 47:
        return set(range(d, 32)) | _ra0(a), set()
    if op in (48, 50):
        return _ra0(a), {32 + d}
    if op in (49, 51):
        return {a}, {32 + d, a}
    if op in (52, 54):
        return {32 + d} | _ra0(a), set()
    if op in (53, 55):
        return {32 + d, a}, {a}
    if op == 58:
        if w & 3 == 1:
            return {a}, {d, a}
        return _ra0(a), {d}
    if op == 62:
        if w & 3 == 1:
            return {d, a}, {a}
        return {d} | _ra0(a), set()
    if op in (59, 63):
        x5 = (w >> 1) & 0x1f
        if op == 59 or x5 >= 16:
            fd, fa, fb, fc = 32 + d, 32 + a, 32 + b, 32 + c
            usa = {18: {fa, fb}, 20: {fa, fb}, 21: {fa, fb}, 22: {fb}, 24: {fb}, 25: {fa, fc}, 26: {fb},
                   23: {fa, fb, fc}, 28: {fa, fb, fc}, 29: {fa, fb, fc}, 30: {fa, fb, fc}, 31: {fa, fb, fc}}
            return usa.get(x5, {fa, fb}), {fd}
        x10 = (w >> 1) & 0x3ff
        if x10 in (0, 32):
            return {32 + a, 32 + b}, set()
        if x10 in (12, 14, 15, 814, 815, 846, 40, 72, 136, 264):
            return {32 + b}, {32 + d}
        if x10 == 583:
            return set(), {32 + d}
        if x10 == 711:
            return {32 + b}, set()
        return set(), set()
    if op == 4:
        if w & 0x3F == 0x03:   # VMX128: lvx128/stvx128/lvlx128... leen rA, rB
            return _ra0(a) | {b}, set()
        return set(), set()
    if op in (5, 6, 16, 17, 18, 19):
        return set(), set()
    if op == 31:
        x10 = (w >> 1) & 0x3ff
        x9 = (w >> 1) & 0x1ff
        if x10 in LOAD_X:
            r = _ra0(a) | {b}
            return (r | {a}, {d, a}) if x10 in LOAD_X_UPD else (r, {d})
        if x10 in FLOAD_X:
            return (_ra0(a) | {b}, {32 + d} | ({a} if FLOAD_X[x10] else set()))
        if x10 in STORE_X:
            r = {d} | _ra0(a) | {b}
            return (r, {a}) if x10 in STORE_X_UPD else (r, set())
        if x10 in FSTORE_X:
            return ({32 + d} | _ra0(a) | {b}, {a} if FSTORE_X[x10] else set())
        if x10 in VEC_X:
            return _ra0(a) | {b}, set()
        if x10 in (0, 32, 4, 68):
            return {a, b}, set()
        if x10 in LOGIC_X:
            return {d, b}, {a}
        if x10 in UNARIO_X or (w >> 2) & 0x1ff == 413:   # sradi es forma XS (9 bits de xo desde el bit 2)
            return {d}, {a}
        if x9 in XO_AB:
            return {a, b}, {d}
        if x9 in XO_A:
            return {a}, {d}
        if x10 in (339, 19, 371, 83):        # mfspr mfcr mftb mfmsr
            return set(), {d}
        if x10 in (467, 144, 146, 178):      # mtspr mtcrf mtmsr mtmsrd
            return {d}, set()
        if x10 in (598, 854, 512):           # sync eieio mcrxr
            return set(), set()
        DESCONOCIDOS[x10] += 1
        return set(), {d}
    DESCONOCIDOS[1000 + op] += 1
    return set(), set()


def es_guarda(w):
    """stw/std/stfd/stfs de un registro con base r1 o r12 (prologo): GUARDA el registro, no lo usa."""
    op, d, a = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    if a not in (1, 12):
        return None
    if op in (36, 37) or (op == 62 and w & 3 in (0, 1)):
        return d
    if op in (52, 53, 54, 55):
        return 32 + d
    return None


def es_restaura(w):
    """lwz/ld/lfd con base r1 o r12/r11 (epilogo): el registro que restaura, o None."""
    op, d, a = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    if a not in (1, 11, 12):
        return None
    if op == 32 or (op == 58 and w & 3 == 0):
        return d
    if op in (48, 50):
        return 32 + d
    return None


# --------------------------------------------------------------------------------------------- los dos programas

class Programa:
    """Un ejecutable con su lista de funciones [(va, fin, nombre)] y su contenido."""

    def __init__(self, pe, base, funcs, unidad=None):
        self.pe, self.base = pe, base
        self.funcs = sorted(funcs)
        self.va = [f[0] for f in self.funcs]
        self.por_va = {f[0]: f for f in self.funcs}
        cuenta = collections.Counter(f[2] for f in self.funcs)
        self.unicos = {f[2]: f for f in self.funcs if cuenta[f[2]] == 1}
        self.unidad = unidad or {}

    def palabras(self, f):
        n = (f[1] - f[0]) // 4
        return list(struct.unpack('>%dI' % n, self.pe.lee(f[0] - self.base, 4 * n))) if n > 0 else []

    def en(self, va):
        i = bisect.bisect_right(self.va, va) - 1
        if i >= 0 and self.funcs[i][0] <= va < self.funcs[i][1]:
            return self.funcs[i]
        return None


def lee_simbolos():
    """[(va, tamano, nombre)] de las funciones de symbols.txt (las que no traen tamano llegan hasta la siguiente)."""
    out = []
    pat = re.compile(r'^(\S+) = \.text:0x([0-9A-Fa-f]+); // type:function(?: size:0x([0-9A-Fa-f]+))?')
    for linea in open(os.path.join(RAIZ, 'config', V, 'symbols.txt'), encoding='utf-8'):
        m = pat.match(linea)
        if m:
            out.append((int(m.group(2), 16), int(m.group(3), 16) if m.group(3) else 0, m.group(1)))
    return out


INFORME = os.path.join(RAIZ, 'build', 'versiones', V + '.json')


def informe():
    """{nombre: (unidad corta, porcentaje o None, tamano)} de las funciones del original en la medida oficial."""
    d = json.load(open(INFORME, encoding='utf-8'))
    out = {}
    for u in d['units']:
        corto = u['name'].split('/')[-1]
        for f in u.get('functions', []):
            out[f['name']] = (corto, f.get('fuzzy_match_percent'), int(f['size']))
    return out


def original():
    pe = ltcg360.PE(os.path.join(RAIZ, 'orig', V, 'NFS.exe'))
    sims = sorted(lee_simbolos())
    txt = pe.seccion('.text')
    t1 = ORIG_BASE + txt[1] + txt[2]
    funcs = []
    for k, (va, tam, n) in enumerate(sims):
        sig = sims[k + 1][0] if k + 1 < len(sims) else t1
        fin = va + tam if tam else sig
        funcs.append((va, min(fin, max(sig, va + 4)) if tam else sig, n))
    inf = informe()
    return Programa(pe, ORIG_BASE, funcs, {n: inf[n][0] for _, _, n in funcs if n in inf})


def nuestro(exe=None, mapa=None):
    exe = exe or os.path.join(LTCG, 'NFS_ltcg.exe')
    mapa = mapa or os.path.join(LTCG, 'NFS_ltcg.map')
    pe = ltcg360.PE(exe)
    txt = pe.seccion('.text')
    t0, t1 = txt[1], txt[1] + txt[2]
    sims = ltcg360.lee_mapa(mapa)
    vistos = {}
    for rva, n, ob, f, est in sims:
        if f and t0 <= rva < t1 and (rva not in vistos or (vistos[rva][3] and not est)):
            vistos[rva] = (rva, n, ob, est)
    lista = sorted(vistos.values())
    funcs, unidad = [], {}
    for k, (rva, n, ob, est) in enumerate(lista):
        fin = lista[k + 1][0] if k + 1 < len(lista) else t1
        funcs.append((pe.base + rva, pe.base + fin, n))
        unidad.setdefault(n, ob[:-4] if ob.lower().endswith('.obj') else ob)
    prog = Programa(pe, pe.base, funcs, unidad)
    prog.mapa = mapa
    return prog


def stubs():
    out = {}
    p = os.path.join(LTCG, 'stubs.txt')
    if os.path.exists(p):
        for linea in open(p, encoding='latin-1'):
            k, _, n = linea.rstrip('\n').partition(' ')
            if n:
                out[n] = k
    return out


def destino(w, va):
    """Destino de un b/bl (opcode 18) o None."""
    if w >> 26 != 18:
        return None
    li = w & 0x03FFFFFC
    if li & 0x02000000:
        li -= 0x04000000
    return (li if w & 2 else va + li) & 0xFFFFFFFF


def llamadas(prog, f, ws=None):
    """[(indice de palabra, va destino, es_cola)] de los bl y de los b que salen de la funcion."""
    ws = ws if ws is not None else prog.palabras(f)
    out = []
    for i, w in enumerate(ws):
        t = destino(w, f[0] + 4 * i)
        if t is None:
            continue
        if w & 1:
            out.append((i, t, False))
        elif not (f[0] <= t < f[1]):
            out.append((i, t, True))
    return out


def nombre_en(prog, va):
    f = prog.por_va.get(va)
    return f[2] if f else None


# --------------------------------------------------------------------------------------------- 1. expansiones

def callees(prog, f):
    c = collections.Counter()
    for _, t, _ in llamadas(prog, f):
        n = nombre_en(prog, t)
        if n and not AYUDANTES.match(n):
            c[n] += 1
    return c


def cmd_expansiones(a):
    O, N = original(), nuestro()
    inf = informe()
    st = stubs()
    comunes = sorted(set(O.unicos) & set(N.unicos))
    porg = collections.defaultdict(lambda: {'orig_expande': [], 'nos_expande': [], 'iguales': 0})
    funcs_dif = 0
    for n in comunes:
        co, cn = callees(O, O.unicos[n]), callees(N, N.unicos[n])
        dif = False
        for g in set(co) | set(cn):
            if g not in O.unicos or g not in N.unicos:
                continue
            if co[g] == cn[g]:
                porg[g]['iguales'] += 1
            elif co[g] < cn[g]:
                porg[g]['orig_expande'].append((n, cn[g] - co[g]))
                dif = True
            else:
                porg[g]['nos_expande'].append((n, co[g] - cn[g]))
                dif = True
        funcs_dif += dif
    filas = []
    for g, d in porg.items():
        oe, ne = d['orig_expande'], d['nos_expande']
        if len(oe) + len(ne) < a.min_dif:
            continue
        bytes_ll = sum(inf.get(x, ('', None, 0))[2] for x, _ in oe + ne
                       if (inf.get(x, ('', None, 0))[1] or 0) < 100)
        fo, fn = O.unicos[g], N.unicos[g]
        filas.append((bytes_ll, g, len(oe), len(ne), d['iguales'], fo[1] - fo[0], fn[1] - fn[0],
                      inf.get(g, ('', None, 0))[1], g in st, O.unidad.get(g, '?'), oe, ne))
    filas.sort(reverse=True)
    print('expansiones: %d funciones con el mismo nombre en los dos lados; %d con alguna llamada distinta; '
          '%d llamadas (G) con >= %d llamantes distintos' % (len(comunes), funcs_dif, len(filas), a.min_dif))
    print('  O>N = el original la expandio (o no la llama) y nosotros la llamamos; N>O = al reves')
    print('%9s %5s %5s %5s %6s %6s %7s %5s  %-10s %s' % ('B llam<100', 'O>N', 'N>O', 'igual', 'tamO', 'tamN', '%G',
                                                         'stub', 'unidad', 'G'))
    for fila in filas[:a.n]:
        b, g, oe, ne, ig, to, tn, p, esst, u = fila[:10]
        print('%9d %5d %5d %5d %6d %6d %7s %5s  %-10s %s' % (b, oe, ne, ig, to, tn,
                                                             '-' if p is None else '%.1f' % p, 'S' if esst else '',
                                                             u, g[:110]))
    if a.tsv:
        with open(a.tsv, 'w', encoding='utf-8') as fh:
            fh.write('bytes_llamantes_no100\tO>N\tN>O\tiguales\ttamO\ttamN\tpctG\tstub\tunidad\tG\tllamantes_O>N\t'
                     'llamantes_N>O\n')
            for fila in filas:
                b, g, oe, ne, ig, to, tn, p, esst, u, l1, l2 = fila
                fh.write('%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%s\t%s\t%s\n' % (
                    b, oe, ne, ig, to, tn, '' if p is None else '%.2f' % p, 'S' if esst else '', u, g,
                    ' '.join(x for x, _ in l1), ' '.join(x for x, _ in l2)))
    return filas


# --------------------------------------------------------------------------------------------- 2. convencion

def bloques(f, ws):
    """Bloques basicos: [(i0, i1, sucesores, llamadas en el bloque)]. bcctr sin enlace: sucesores = los bloques que
    nadie mas alcanza (las entradas de la tabla de saltos)."""
    n = len(ws)
    lideres = {0}
    for i, w in enumerate(ws):
        op = w >> 26
        if op in (16, 18) and not w & 1:
            t = destino(w, f[0] + 4 * i) if op == 18 else None
            if op == 16:
                bd = w & 0xFFFC
                if bd & 0x8000:
                    bd -= 0x10000
                t = (bd if w & 2 else f[0] + 4 * i + bd) & 0xFFFFFFFF
            if t is not None and f[0] <= t < f[1]:
                lideres.add((t - f[0]) // 4)
            lideres.add(i + 1)
        elif op == 19 and ((w >> 1) & 0x3ff) in (16, 528) and not w & 1:
            lideres.add(i + 1)
    lideres = sorted(x for x in lideres if x < n)
    bl = []
    for k, i0 in enumerate(lideres):
        i1 = lideres[k + 1] if k + 1 < len(lideres) else n
        bl.append([i0, i1, set(), False])
    idx = {b[0]: j for j, b in enumerate(bl)}
    tabla = []
    for j, b in enumerate(bl):
        w = ws[b[1] - 1]
        i = b[1] - 1
        op = w >> 26
        sigue = True
        if op == 18 and not w & 1:
            t = destino(w, f[0] + 4 * i)
            if f[0] <= t < f[1]:
                b[2].add(idx.get((t - f[0]) // 4))
            sigue = False
        elif op == 16 and not w & 1:
            bd = w & 0xFFFC
            if bd & 0x8000:
                bd -= 0x10000
            t = (bd if w & 2 else f[0] + 4 * i + bd) & 0xFFFFFFFF
            if f[0] <= t < f[1]:
                b[2].add(idx.get((t - f[0]) // 4))
            if (w >> 21) & 0x14 == 0x14:
                sigue = False
        elif op == 19 and not w & 1:
            x = (w >> 1) & 0x3ff
            if x == 16 and (w >> 21) & 0x14 == 0x14:
                sigue = False
            elif x == 528:
                sigue = False
                tabla.append(j)
        if sigue and j + 1 < len(bl):
            b[2].add(j + 1)
        b[2].discard(None)
    if tabla:
        alc, pila = {0}, [0]
        while pila:
            for s in bl[pila.pop()][2]:
                if s not in alc:
                    alc.add(s)
                    pila.append(s)
        resto = [j for j in range(len(bl)) if j not in alc]
        for j in tabla:
            bl[j][2].update(resto)
    return bl


def sin_tablas(f, ws):
    """Las palabras de las tablas de saltos (tras un bctr, direcciones alineadas dentro de la propia funcion) a cero:
    son datos, y una direccion 0x822Fxxxx se lee como un lwz."""
    ws = list(ws)
    for i, w in enumerate(ws):
        if w == 0x4E800420:
            j = i + 1
            while j < len(ws) and f[0] <= ws[j] < f[1] and ws[j] % 4 == 0:
                ws[j] = 0
                j += 1
    return ws


def analiza(prog, f, ayudante):
    """{'entradas': regs leidos antes de escribirse (fuera de guardas), 'sin_guardar': no volatiles escritos sin
    guardarse, 'vivos': [(va destino, regs volatiles vivos tras la llamada)]}."""
    ws = sin_tablas(f, prog.palabras(f))
    if not ws:
        return None
    bl = bloques(f, ws)
    guardados, escritos = set(), set()
    uso, defi, ins = [], [], []
    for b in bl:
        u, dd, lista = set(), set(), []
        for i in range(b[0], b[1]):
            w = ws[i]
            r, e = rw(w)
            g = es_guarda(w)
            if g is not None and g in NOVOL and g not in dd:
                guardados.add(g)
                r = r - {g}
            rest = es_restaura(w)
            t = destino(w, f[0] + 4 * i)
            llamada = False
            if t is not None and (w & 1 or not (f[0] <= t < f[1])):
                nom = ayudante.get(t)
                if nom:
                    m = re.match(r'__(save|rest)(gprlr|fpr|gpr|vmx)_(\d+)', nom)
                    if m and m.group(2) in ('gprlr', 'gpr', 'fpr'):
                        base = 32 if m.group(2) == 'fpr' else 0
                        regs = set(range(base + int(m.group(3)), base + 32))
                        if m.group(1) == 'save':
                            guardados |= regs - dd
                            r, e = {12} if m.group(2) == 'gprlr' else set(), set()
                        else:
                            r, e = set(), regs | {12}
                            rest = 'todos'
                    else:
                        r, e = set(), set()
                else:
                    llamada = True
                    r, e = set(), set(VOLATILES)
            if rest is None:
                escritos |= (e & NOVOL)
            r = set(x for x in r if x not in (1, 2, 13))
            lista.append((r, e, llamada, t))
            u |= (r - dd)
            dd |= e
        uso.append(u)
        defi.append(dd)
        ins.append(lista)
    vivo_in = [set() for _ in bl]
    cambio = True
    while cambio:
        cambio = False
        for j in range(len(bl) - 1, -1, -1):
            out = set()
            for s in bl[j][2]:
                out |= vivo_in[s]
            nuevo = uso[j] | (out - defi[j])
            if nuevo != vivo_in[j]:
                vivo_in[j] = nuevo
                cambio = True
    vivos = []
    for j, b in enumerate(bl):
        out = set()
        for s in b[2]:
            out |= vivo_in[s]
        for r, e, llamada, t in reversed(ins[j]):
            if llamada:
                v = out & VOLATILES - RETORNO
                if v:
                    vivos.append((t, v))
            out = r | (out - e)
    return {'entradas': vivo_in[0] if bl else set(), 'sin_guardar': escritos - guardados, 'vivos': vivos}


def rareza(res):
    if res is None:
        return set(), set()
    raras = res['entradas'] - GPR_ARGS - FPR_ARGS
    return raras, res['sin_guardar']


def convencion_de(prog):
    ayud = {f[0]: f[2] for f in prog.funcs if AYUDANTES.match(f[2])}
    por = {}
    callee_vivos = collections.defaultdict(list)
    for f in prog.funcs:
        if AYUDANTES.match(f[2]):
            continue
        res = analiza(prog, f, ayud)
        if res is None:
            continue
        por[f[2]] = res
        for t, v in res['vivos']:
            callee_vivos[t].append((f[2], v))
    return por, callee_vivos


def vtable_refs(prog, img0, img1, secciones=('.rdata', '.data')):
    """Direcciones de funcion que aparecen como palabra alineada en .rdata/.data (vtables, tablas de punteros)."""
    starts = set(prog.va)
    out = collections.Counter()
    for nom in secciones:
        try:
            s = prog.pe.seccion(nom)
        except IndexError:
            continue
        b = prog.pe.lee(s[1], s[2])
        for k in range(0, len(b) - 3, 4):
            x = struct.unpack_from('>I', b, k)[0]
            if x in starts:
                out[x] += 1
    return out


def cmd_convencion(a):
    O, N = original(), nuestro()
    inf = informe()
    t = time.time()
    po, vo = convencion_de(O)
    pn, vn = convencion_de(N)
    print('convencion: analizadas %d funciones del original y %d nuestras en %.0f s' % (len(po), len(pn),
                                                                                         time.time() - t))
    if DESCONOCIDOS:
        print('  (opcodes sin tabla, tratados como "escribe rD": %s)' % ', '.join(
            '%s x%d' % (('op%d' % (k - 1000)) if k >= 1000 else 'x31:%d' % k, n)
            for k, n in DESCONOCIDOS.most_common(8)))

    def resumen(prog, por, vivos, etiqueta):
        ent = {n for n, r in por.items() if rareza(r)[0]}
        sg = {n for n, r in por.items() if rareza(r)[1]}
        callee = {nombre_en(prog, t) for t in vivos if nombre_en(prog, t)}
        print('  %-9s entradas fuera del ABI %5d | no volatiles sin guardar %5d | llamadas con volatiles vivos: '
              '%d llamantes -> %d funciones' % (etiqueta, len(ent), len(sg),
                                                len({x for l in vivos.values() for x, _ in l}), len(callee)))
        regs = collections.Counter()
        for n in ent:
            regs.update(rn(x) for x in rareza(por[n])[0])
        print('            registros de entrada: %s' % ', '.join('%s %d' % kv for kv in regs.most_common(10)))
        regs = collections.Counter()
        for l in vivos.values():
            for _, v in l:
                regs.update(rn(x) for x in v)
        print('            vivos a traves de la llamada: %s' % ', '.join('%s %d' % kv for kv in regs.most_common(10)))
        return ent, sg, callee

    eo, so, co = resumen(O, po, vo, 'original')
    en_, sn, cn = resumen(N, pn, vn, 'nuestro')
    propia_o = eo | so | co
    propia_n = en_ | sn | cn
    # control: una funcion en una vtable/tabla de punteros no puede tener ENTRADAS fuera del ABI ni dejar no volatiles
    # sin guardar (la llaman por puntero). Los volatiles vivos a traves de una llamada directa SI pueden darse: LTCG
    # sabe que registros toca la llamada aunque tambien se la llame por puntero (reparto entre procedimientos).
    vt = vtable_refs(O, ORIG_BASE, ORIG_BASE + 0x2000000)
    en_tabla = {n for n in O.unicos if O.unicos[n][0] in vt}
    malas = sorted((eo | so) & en_tabla)
    print('  control: %d de %d funciones con entradas fuera del ABI o no volatiles sin guardar estan en una tabla de '
          'punteros (deberia ser 0: %s); con volatiles vivos en sus llamantes y en tabla: %d' % (
              len(malas), len(eo | so), ', '.join(malas[:6]), len(co & en_tabla)))
    comunes = set(O.unicos) & set(N.unicos)
    solo_o = sorted((n for n in comunes if n in propia_o and n not in propia_n),
                    key=lambda n: -(inf.get(n, ('', 0, 0))[2]))
    solo_n = sorted((n for n in comunes if n in propia_n and n not in propia_o),
                    key=lambda n: -(inf.get(n, ('', 0, 0))[2]))
    ambas = [n for n in comunes if n in propia_o and n in propia_n]
    print('  con el mismo nombre en los dos lados: %d; convencion propia en los dos %d, solo en el original %d, '
          'solo en la nuestra %d' % (len(comunes), len(ambas), len(solo_o), len(solo_n)))
    fn_o = [f for f in O.funcs if f[2].startswith('fn_') and f[2] in propia_o]
    print('  fn_ del original con convencion propia (internas, sin direccion tomada, llamadas directas): %d' % len(fn_o))
    # nombres de metodos virtuales (aparecen en vtables nuestras) puestos sobre funciones con convencion propia
    vtn = vtable_refs(N, N.base, N.base + 0x2000000)
    virt = {f[2] for f in N.funcs if f[0] in vtn}
    sosp = sorted(n for n in propia_o if n in virt and n in O.unicos and O.unicos[n][0] not in vt)
    print('  SOSPECHOSOS: %d nombres del original que en nuestro enlace estan en una vtable/tabla y en el original '
          'llevan convencion propia (nombre falso, o el original no la tiene en tabla):' % len(sosp))
    for n in sosp[:a.n]:
        print('    %-8s %6s %s' % (O.unidad.get(n, '?'), '-' if inf.get(n, ('', None))[1] is None else
                                   '%.1f' % inf[n][1], n[:110]))

    def detalle(lista, titulo, prog_a, por_a, viv_a):
        print('  %s (por tamano):' % titulo)
        for n in lista[:a.n]:
            r = por_a.get(n)
            ent, sg = rareza(r)
            f = prog_a.unicos[n]
            vv = set()
            for _, v in viv_a.get(f[0], []):
                vv |= v
            print('    %-8s %6s %5d  entra[%s] sin_guardar[%s] vivos_en_llamantes[%s]  %s' % (
                O.unidad.get(n, '?'), '-' if inf.get(n, ('', None))[1] is None else '%.1f' % inf[n][1],
                f[1] - f[0], ' '.join(map(rn, sorted(ent))), ' '.join(map(rn, sorted(sg))),
                ' '.join(map(rn, sorted(vv))), n[:90]))
    detalle(solo_o, 'solo en el ORIGINAL', O, po, vo)
    detalle(solo_n, 'solo en la NUESTRA', N, pn, vn)
    if a.tsv:
        with open(a.tsv, 'w', encoding='utf-8') as fh:
            fh.write('lado\tnombre\tunidad\tpct\ttam\tentradas\tsin_guardar\tvivos_en_llamantes\n')
            for lado, prog, por, viv, prop in (('O', O, po, vo, propia_o), ('N', N, pn, vn, propia_n)):
                for n in sorted(prop):
                    f = prog.unicos.get(n)
                    if not f:
                        continue
                    ent, sg = rareza(por.get(n))
                    vv = set()
                    for _, v in viv.get(f[0], []):
                        vv |= v
                    p = inf.get(n, ('', None, 0))[1]
                    fh.write('%s\t%s\t%s\t%s\t%d\t%s\t%s\t%s\n' % (
                        lado, n, prog.unidad.get(n, ''), '' if p is None else '%.2f' % p, f[1] - f[0],
                        ' '.join(map(rn, sorted(ent))), ' '.join(map(rn, sorted(sg))), ' '.join(map(rn, sorted(vv)))))
    return po, pn


# --------------------------------------------------------------------------------------------- 3. supervivencia

def enlaza_variante(destino_dir, opciones):
    """Enlaza los IL de ltcg360 (build/<V>/ltcg/il, el orden de link.rsp) con otras /OPT."""
    os.makedirs(destino_dir, exist_ok=True)
    rsp = os.path.join(destino_dir, 'link.rsp')
    with open(os.path.join(LTCG, 'link.rsp')) as fh, open(rsp, 'w') as out:
        out.write(fh.read())
    t = time.time()
    p = subprocess.run([os.path.join(RAIZ, ltcg360.LINK), '/nologo', '/LTCG', '/XEX:NO', '/NODEFAULTLIB',
                        '/FORCE:MULTIPLE'] + opciones + ['/ENTRY:main', '/MAP:' + os.path.join(destino_dir, 'NFS.map'),
                                                         '/OUT:' + os.path.join(destino_dir, 'NFS.exe'), '@' + rsp],
                       cwd=RAIZ, capture_output=True, text=True, errors='replace')
    open(os.path.join(destino_dir, 'link.log'), 'w', encoding='utf-8').write(p.stdout + p.stderr)
    print('supervivencia: enlace %s en %.0f s (codigo %d)' % (' '.join(opciones), time.time() - t, p.returncode))
    return p.returncode == 0


def clave(n):
    """Lo que comparten un nombre de PC (x86) y uno de X360: '?Metodo@Clase' sin la convencion ni los tipos."""
    return n.split('@@')[0] if n.startswith('?') else n


def otras_versiones():
    """Claves de los nombres de funcion de PC (symbols.json): lo que PC tambien tiene."""
    out = set()
    p = os.path.join(RAIZ, 'config', 'SPEED_EXE_1_3', 'symbols.json')
    if os.path.exists(p):
        d = json.load(open(p, encoding='utf-8'))
        for x in d.get('functions', []):
            out.add(clave(x['name']))
    return out


def cadenas_ausentes(N, O):
    """{funcion nuestra: [cadenas]} de las que leen (lis/addi reubicados) una cadena ??_C@ de nuestro .rdata que no
    esta en ninguna parte del original (.rdata/.data): o la funcion no esta en el original (codigo muerto que el
    enlace quito con sus cadenas) o su fuente es otra."""
    sims = ltcg360.lee_mapa(N.mapa)
    cad = {}
    for rva, n, ob, f, est in sims:
        if n.startswith('??_C@_0'):
            b = N.pe.lee(rva, 256)
            z = b.find(bytes(1))
            if z > 3:
                cad[N.base + rva] = b[:z]
    orig = b''.join(O.pe.lee(s[1], s[2]) for s in O.pe.secs if s[0] in ('.rdata', '.data'))
    # los __FILE__ (FNEW/BNEW de milestone) son rutas de nuestro arbol: el original las tiene como
    # e:\mw\speed\indep\src\... y no dicen nada del codigo
    faltan = {va: t for va, t in cad.items() if t not in orig and not re.search(rb'[.](cpp|hpp|h|c)$', t)}
    rel = N.pe.relocs()
    out = collections.defaultdict(list)
    for f in N.funcs:
        for va in range(f[0], f[1], 4):
            r = rel.get(va - N.base)
            if r and r[0] == 4:
                w = struct.unpack('>I', N.pe.lee(va - N.base, 4))[0]
                t = (((w & 0xFFFF) << 16) + sext16(r[1])) & 0xFFFFFFFF
                if t in faltan:
                    out[f[2]].append(faltan[t].decode('latin-1'))
    return out, len(cad), len(faltan)


def sext16(x):
    return x - 0x10000 if x & 0x8000 else x


def sin_referencias(prog):
    """Funciones a las que nada se refiere: ni b/bl, ni lis/addi con su direccion, ni una palabra de .rdata/.data."""
    inicios = set(prog.va)
    ref = collections.Counter(vtable_refs(prog, 0, 0))
    for f in prog.funcs:
        hi = {}
        for i, w in enumerate(prog.palabras(f)):
            t = destino(w, f[0] + 4 * i)
            if t is not None and t in inicios and t != f[0]:
                ref[t] += 1
            op, d, ra = w >> 26, (w >> 21) & 31, (w >> 16) & 31
            if op == 15 and ra == 0:
                hi[d] = (w & 0xFFFF) << 16
            elif op == 14 and ra in hi:
                v = (hi[ra] + sext16(w & 0xFFFF)) & 0xFFFFFFFF
                if v in inicios:
                    ref[v] += 1
    return [f for f in prog.funcs if ref[f[0]] == 0 and not f[2].startswith('__')]


def inlines_de_mapa(mapa):
    """Nombres que el .map de link.exe marca 'f i': copias fuera de linea de funciones inline."""
    out = set()
    pat = re.compile(r'^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+[0-9a-f]{8}\s+f\s+i\s+')
    for linea in open(mapa, encoding='latin-1'):
        m = pat.match(linea)
        if m:
            out.add(m.group(1))
    return out


def cmd_supervivencia(a):
    ref = a.ref or os.path.join(LTCG, 'ref')
    if a.enlazar and not enlaza_variante(ref, ['/OPT:REF', '/OPT:NOICF']):
        return 1
    O = original()
    N = nuestro()
    # 0. el original se enlazo SIN /OPT:REF: guarda las copias fuera de linea que nadie llama
    huer = sin_referencias(O)
    print('supervivencia: el original tiene %d funciones (%d B) sin NINGUNA referencia (copias fuera de linea de '
          'inline como acosf, MPH2MPS, Vector3Make...): se enlazo con /OPT:NOREF, como ltcg360. Lo que sobrevive en '
          'el original es lo que alguna unidad EMITIO, no lo que se usa.' % (len(huer), sum(f[1] - f[0] for f in huer)))
    # 1. copias fuera de linea de inline (f i en nuestro .map): una copia que el original no tiene = alguna unidad
    #    nuestra no expandio (o tomo la direccion) donde todas las del original si; al reves, el original no la expandio
    inl = inlines_de_mapa(N.mapa)
    nombres_o0 = {f[2] for f in O.funcs}
    tam0 = {f[2]: f[1] - f[0] for f in N.funcs}
    solo_n = sorted((n for n in inl if n not in nombres_o0 and n in tam0), key=lambda n: -tam0[n])
    con_o = [n for n in inl if n in nombres_o0]
    print('  copias fuera de linea de inline en nuestro enlace: %d; con nombre en el original: %d; sin el: %d '
          '(%d B; el original pudo dejarla sin nombre (fn_) o no emitirla)' % (
              len(inl), len(con_o), len(solo_n), sum(tam0[n] for n in solo_n)))
    # 2. lo que el original tiene (con nombre, en una unidad medida) y nuestro enlace no tiene en ninguna parte
    nuestros0 = {f[2] for f in N.funcs}
    falta = [f for f in O.funcs if not f[2].startswith('fn_') and f[2] not in nuestros0
             and not O.unidad.get(f[2], 'auto').startswith('auto')]
    print('  funciones del original CON nombre en unidades medidas que el troceado no tiene: %d (%d B)' % (
              len(falta), sum(f[1] - f[0] for f in falta)))
    # alias: el mismo codigo con otro nombre en la misma direccion (??_E/??_G); el troceado escribe un nombre por
    # direccion y prefiere el que usa el original
    en_mapa = {n for rva, n, ob, f, est in ltcg360.lee_mapa(N.mapa) if f}
    alias = [f for f in falta if f[2] in en_mapa]
    nada = [f for f in falta if f[2] not in en_mapa]
    porclave = collections.defaultdict(set)
    for n in en_mapa:
        porclave[clave(n)].add(n)
    firma = [(f, porclave[clave(f[2])]) for f in nada if f[2].startswith('?') and clave(f[2]) in porclave]
    print('    %d son ALIAS de otro nombre en nuestro enlace; %d (%d B) no estan en ninguna parte: codigo que falta, una '
          'inline que expandimos en todas partes, o el mismo metodo con OTRA decoracion (%d: firma distinta)' % (
              len(alias), len(nada), sum(f[1] - f[0] for f in nada), len(firma)))
    cu = collections.Counter(O.unidad.get(f[2], '?') for f in nada)
    print('    por unidad: %s' % ', '.join('%s %d' % kv for kv in cu.most_common(12)))
    for f, ns in firma[:a.n]:
        print('    firma %-10s %5d O %s\n%22sN %s' % (O.unidad.get(f[2], '?'), f[1] - f[0], f[2][:100], '',
                                                     ' | '.join(sorted(ns))[:100]))
    R = nuestro(os.path.join(ref, 'NFS.exe'), os.path.join(ref, 'NFS.map'))
    inf = informe()
    st = stubs()
    pc = otras_versiones()
    nombres_o = {f[2] for f in O.funcs}
    vivas_r = {f[2] for f in R.funcs}
    todas_n = {f[2] for f in N.funcs if f[2] not in st and not AYUDANTES.match(f[2])}
    quitadas = todas_n - vivas_r
    conservadas = todas_n & vivas_r
    # 1. el original tiene la funcion (por nombre) y REF nos la quita: un llamante (o una referencia) que falta
    falta_llamante = sorted((n for n in quitadas if n in nombres_o), key=lambda n: -inf.get(n, ('', 0, 0))[2])
    # 2. la conservamos con REF y el original no tiene ese nombre: o el nombre del original es fn_ (sin nombrar),
    #    o el original la elimino (codigo muerto, una referencia de mas en nuestra fuente)
    sin_nombre_o = sorted(n for n in conservadas if n not in nombres_o)
    tam = {f[2]: f[1] - f[0] for f in N.funcs}
    print('supervivencia: nuestras %d funciones (sin stubs); /OPT:REF quita %d (%d B) y conserva %d' % (
        len(todas_n), len(quitadas), sum(tam[n] for n in quitadas), len(conservadas)))
    print('  quitadas por REF y PRESENTES en el original por nombre: %d (%d B) -> el original las llama/referencia '
          'desde codigo que nos falta' % (len(falta_llamante), sum(tam[n] for n in falta_llamante)))
    cu = collections.Counter(N.unidad.get(n, '?') for n in falta_llamante)
    print('    por unidad: %s' % ', '.join('%s %d' % kv for kv in cu.most_common(12)))
    print('  conservadas por REF y SIN nombre en el original: %d (%d B); de ellas con nombre en PC: %d' % (
        len(sin_nombre_o), sum(tam[n] for n in sin_nombre_o), sum(1 for n in sin_nombre_o if clave(n) in pc)))
    # para las sin nombre: hay en el original una fn_ de la misma unidad con el mismo tamano? (si no, candidata a
    # codigo muerto en el original)
    fn_por_tam = collections.defaultdict(set)
    for f in O.funcs:
        if f[2].startswith('fn_'):
            fn_por_tam[(O.unidad.get(f[2], '?'), f[1] - f[0])].add(f[2])
    muertas = []
    for n in sin_nombre_o:
        u, t = N.unidad.get(n, '?'), tam[n]
        if not any(fn_por_tam.get((u, t + d)) for d in range(-16, 20, 4)):
            muertas.append(n)
    print('  de esas, SIN ninguna fn_ del original en su unidad con tamano parecido (+-16 B): %d (%d B) -> '
          'candidatas a referencia de mas / codigo muerto en el original' % (len(muertas),
                                                                             sum(tam[n] for n in muertas)))
    cu = collections.Counter(N.unidad.get(n, '?') for n in muertas)
    print('    por unidad: %s' % ', '.join('%s %d' % kv for kv in cu.most_common(12)))
    for n in sorted(muertas, key=lambda n: -tam[n])[:a.n]:
        print('    %-12s %6d %s %s' % (N.unidad.get(n, '?'), tam[n], 'PC' if clave(n) in pc else '  ', n[:110]))
    # cadenas: lo mas fiable con el arbol incompleto (no depende de nombres)
    aus, ncad, nfal = cadenas_ausentes(N, O)
    vivas = {n for n in aus if n in vivas_r}
    print('  cadenas: %d literales ??_C@ nuestros, %d NO estan en el original; las leen %d funciones (%d B), %d de '
          'ellas vivas con /OPT:REF' % (ncad, nfal, len(aus), sum(tam.get(n, 0) for n in aus), len(vivas)))
    for n in sorted(aus, key=lambda n: -tam.get(n, 0))[:a.n]:
        p = inf.get(n, ('', None, 0))[1]
        print('    %-12s %6d %6s %s %s  <- %s' % (N.unidad.get(n, '?'), tam.get(n, 0), '-' if p is None else '%.1f' % p,
                                              'REF' if n in vivas else '   ', n[:80],
                                              ' | '.join(sorted(set(aus[n])))[:120]))
    if a.tsv:
        with open(a.tsv, 'w', encoding='utf-8') as fh:
            fh.write('clase\tunidad\ttam\tpc\tnombre\n')
            for n in falta_llamante:
                fh.write('falta_llamante\t%s\t%d\t%s\t%s\n' % (N.unidad.get(n, ''), tam[n],
                                                               'PC' if clave(n) in pc else '', n))
            for n in muertas:
                fh.write('muerta_en_original\t%s\t%d\t%s\t%s\n' % (N.unidad.get(n, ''), tam[n],
                                                                    'PC' if clave(n) in pc else '', n))
            for n in sorted(aus):
                fh.write('cadena_ausente\t%s\t%d\t%s\t%s\t%s\n' % (N.unidad.get(n, ''), tam.get(n, 0),
                                                                 'REF' if n in vivas else '', n,
                                                                 ' | '.join(sorted(set(aus[n])))))
    return 0


# --------------------------------------------------------------------------------------------- 4. stubs

def cmd_stubs(a):
    O, N = original(), nuestro()
    inf = informe()
    st = stubs()
    # los ayudantes de guardar/restaurar registros son opacos en los dos lados (no se expanden): fuera
    stub_va = {f[0]: f[2] for f in N.funcs if f[2] in st and not AYUDANTES.match(f[2])}
    llam = collections.defaultdict(set)
    for f in N.funcs:
        if f[2] in st:
            continue
        for _, t, _ in llamadas(N, f):
            if t in stub_va:
                llam[stub_va[t]].add(f[2])
    # expansiones del original deformadas: el original no llama al stub G desde F, y nosotros si
    deform = collections.Counter()
    for g in llam:
        if g not in O.unicos:
            continue
        for fnm in llam[g]:
            if fnm in O.unicos:
                co = callees(O, O.unicos[fnm])
                if co[g] == 0:
                    deform[g] += 1
    # reparto entre procedimientos (IPRA): llamantes F en los que el ORIGINAL guarda volatiles vivos a traves de la
    # llamada a G (sabe que G no los toca). Un stub es opaco: LTCG supone que lo machaca todo y F no puede casar.
    _, viv_o = convencion_de(O)
    ipra = collections.Counter()
    for g in llam:
        fo = O.unicos.get(g)
        if fo:
            ipra[g] = len({x for x, _ in viv_o.get(fo[0], []) if x in llam[g]})
    filas = []
    for g, ls in llam.items():
        no100 = [x for x in ls if (inf.get(x, ('', None, 0))[1] or 0) < 100]
        b = sum(inf.get(x, ('', None, 0))[2] for x in no100)
        to = O.unicos[g][1] - O.unicos[g][0] if g in O.unicos else 0
        u = O.unidad.get(g, '')
        filas.append((b, g, len(ls), len(no100), deform[g], to, u, ipra[g]))
    filas.sort(reverse=True)
    juego = [f for f in filas if f[5]]
    print('stubs: %d en stubs.txt (%d funciones, %d datos); %d funciones stub con llamantes nuestros' % (
        len(st), sum(1 for k in st.values() if k == 'F'), sum(1 for k in st.values() if k == 'D'), len(llam)))
    print('  con nombre en el original (codigo del juego o de bibliotecas que el original tiene): %d; '
          'el resto son nombres que el original no tiene (CRT/XDK con otro nombre, o mal nombrados)' % len(juego))
    print('  deform = el original no llama a G desde ese llamante (la expandio); ipra = el original deja registros '
          'volatiles vivos a traves de la llamada a G (reparto entre procedimientos): con un stub opaco no puede ser')
    print('  total: deform %d llamantes, ipra %d llamantes' % (sum(f[4] for f in filas), sum(f[7] for f in filas)))
    print('%9s %6s %6s %6s %5s %6s  %-12s %s' % ('B llam<100', 'llam', '<100', 'deform', 'ipra', 'tamO', 'unidad',
                                                   'stub'))
    for b, g, nl, n1, de, to, u, ip in filas[:a.n]:
        print('%9d %6d %6d %6d %5d %6d  %-12s %s' % (b, nl, n1, de, ip, to, u, g[:100]))
    if a.tsv:
        with open(a.tsv, 'w', encoding='utf-8') as fh:
            fh.write('bytes_llamantes_no100\tllamantes\tno100\tdeforma\tipra\ttam_original\tunidad\tstub\tllamantes\n')
            for b, g, nl, n1, de, to, u, ip in filas:
                fh.write('%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\n' % (b, nl, n1, de, ip, to, u, g,
                                                                ' '.join(sorted(llam[g]))))
    return 0


# --------------------------------------------------------------------------------------------- 5. pgo

def rich(ruta):
    d = open(ruta, 'rb').read()
    i = d.find(b'Rich', 0, 0x1000)
    if i < 0:
        return []
    key = struct.unpack_from('<I', d, i + 4)[0]
    j, out = i - 4, []
    while j > 0:
        x = struct.unpack_from('<I', d, j - 4)[0] ^ key
        n = struct.unpack_from('<I', d, j)[0] ^ key
        if x == 0x536E6144:
            break
        out.append((x >> 16, x & 0xffff, n))
        j -= 8
    return [x for x in reversed(out) if x != (0, 0, 0)]


PRODID = {0x6d: 'Utc1400_C', 0x6e: 'Utc1400_CPP', 0x71: 'Utc1400_LTCG_C', 0x72: 'Utc1400_LTCG_CPP',
          0x73: 'Utc1400_POGO_I_C', 0x74: 'Utc1400_POGO_I_CPP', 0x75: 'Utc1400_POGO_O_C', 0x76: 'Utc1400_POGO_O_CPP',
          0x77: 'Cvtpgd1400', 0x78: 'Linker800', 0x7b: 'Implib800', 0x7d: 'Masm800', 0x01: 'Import0'}


def cmd_pgo(a):
    for etiqueta, ruta in (('original', os.path.join(RAIZ, 'orig', V, 'NFS.exe')),
                           ('nuestro', os.path.join(LTCG, 'NFS_ltcg.exe'))):
        r = rich(ruta)
        print('Rich header %s: %s' % (etiqueta, ', '.join('%s(0x%02x) b%d x%d' % (PRODID.get(p, '?'), p, b, n)
                                                          for p, b, n in r)))
        pogo = [x for x in r if 0x73 <= x[0] <= 0x77]
        print('  objetos POGO (instrumentado/optimizado con perfil) o Cvtpgd: %s' % (pogo or 'NINGUNO'))
    O = original()
    inicios = set(O.va)
    partidas, saltos = collections.Counter(), 0
    for f in O.funcs:
        if AYUDANTES.match(f[2]):
            continue
        ws = O.palabras(f)
        for i, w in enumerate(ws):
            op = w >> 26
            if op == 16 and not w & 1:
                bd = w & 0xFFFC
                if bd & 0x8000:
                    bd -= 0x10000
                t = (bd if w & 2 else f[0] + 4 * i + bd) & 0xFFFFFFFF
            elif op == 18 and not w & 1:
                t = destino(w, f[0] + 4 * i)
            else:
                continue
            if f[0] <= t < f[1]:
                continue
            saltos += 1
            if t not in inicios:
                partidas[f[2]] += 1
    print('original: %d saltos sin enlace que salen de su funcion; %d van a mitad de otra funcion '
          '(%d funciones): con BBT/PGO (bloques frios aparte) serian miles' % (
              saltos, sum(partidas.values()), len(partidas)))
    for n, k in partidas.most_common(8):
        print('    %3d  %s' % (k, n[:100]))
    # /OPT:ICF: con el plegado de COMDAT identicos no quedan dos cuerpos iguales en el ejecutable
    cuerpos = collections.defaultdict(list)
    for f in O.funcs:
        if f[1] - f[0] >= 32 and not AYUDANTES.match(f[2]):
            ws = O.palabras(f)
            cuerpos[tuple(w if w >> 26 != 18 else (w & 3) for w in ws)].append(f[2])
    rep = [v for v in cuerpos.values() if len(v) > 1]
    print('ICF: %d grupos de funciones (>= 32 B) con el cuerpo identico en el original, %d copias de mas: sin '
          '/OPT:ICF (con ICF serian 0). El mayor: %d copias (%s...)' % (
              len(rep), sum(len(v) - 1 for v in rep), max(map(len, rep)) if rep else 0,
              (max(rep, key=len)[0] if rep else '')[:60]))
    # orden de emision: las funciones de cada unidad (splits.txt) en rangos contiguos
    rangos = collections.Counter()
    u_actual, cambios = None, 0
    for f in O.funcs:
        u = O.unidad.get(f[2])
        if u and u != u_actual:
            cambios += 1
            rangos[u] += 1
            u_actual = u
    print('orden de emision: %d tramos de unidad para %d unidades (1 tramo por unidad = emision por objeto, sin '
          'reordenar por perfil); las de mas tramos: %s' % (cambios, len(rangos), ', '.join(
              '%s %d' % kv for kv in rangos.most_common(6))))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--ltcg', default='', help='otro directorio con NFS_ltcg.exe, NFS_ltcg.map y stubs.txt (una copia '
                    'del enlace, para no leerlo mientras medir.py lo rehace)')
    ap.add_argument('--informe', default='', help='otro informe de objdiff de X360 (por defecto build/versiones)')
    sub = ap.add_subparsers(dest='cmd', required=True)
    p = sub.add_parser('expansiones')
    p.add_argument('--min-dif', type=int, default=2)
    p.add_argument('--n', type=int, default=60)
    p.add_argument('--tsv', default='')
    p = sub.add_parser('convencion')
    p.add_argument('--n', type=int, default=40)
    p.add_argument('--tsv', default='')
    p = sub.add_parser('supervivencia')
    p.add_argument('--enlazar', action='store_true')
    p.add_argument('--ref', default='')
    p.add_argument('--n', type=int, default=40)
    p.add_argument('--tsv', default='')
    p = sub.add_parser('stubs')
    p.add_argument('--n', type=int, default=40)
    p.add_argument('--tsv', default='')
    sub.add_parser('pgo')
    a = ap.parse_args()
    global LTCG, INFORME
    if a.ltcg:
        LTCG = os.path.abspath(a.ltcg)
    if a.informe:
        INFORME = os.path.abspath(a.informe)
    r = {'expansiones': cmd_expansiones, 'convencion': cmd_convencion, 'supervivencia': cmd_supervivencia,
            'stubs': cmd_stubs, 'pgo': cmd_pgo}[a.cmd](a)
    return r if isinstance(r, int) else 0


if __name__ == '__main__':
    sys.exit(main())
