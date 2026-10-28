#!/usr/bin/env python3
"""patrones_pc.py -- formas de MSVC 7.1 que decide la FUENTE, buscadas en las funciones PARCIALES de PC (speed.exe
contra nuestro objeto). Ronda 85 (PC-A): junta patrones.py y tabla86.py de la ronda 84.

    python scripts/patrones_pc.py [--informe build/versiones/SPEED_EXE_1_3.json] [--lista] [--tabla [N]] [--min P]

Familias (GCC de GC/PS2 y el MSVC de X360 dan lo mismo con las dos formas; MSVC 7.1 no):
  setcc        speed.exe `xor eax,eax; test/cmp; setCC al` donde el nuestro no lleva el xor (bool de una comparacion
               explicita `return x != 0;` frente a `return x;` o if/return true/false)
  setcc_inv    al reves (el nuestro lleva el xor y speed.exe no)
  cola         speed.exe `call F; ret` donde el nuestro `jmp F` (o al reves)
  uno_exe      alineado: speed.exe `cmp X,1; jl/jge` contra nuestro `test X,X; jle/jg` (la fuente era `< 1`/`>= 1`)
  uno_nuestro  al reves (la fuente era `<= 0`/`> 0`)
  orden        alineado: dos instrucciones contiguas de memoria (mov M,..) intercambiadas (orden de asignaciones, p. ej.
               un inline de cabecera: StartFadeIn en la r84; en la r85, 13 funciones de PC por esta lista)
  constante    alineado: speed.exe `cmp X,K; jle` contra nuestro `cmp X,K+1; jl` (o al reves, y los pares jg/jge,
               jbe/jb, ja/jae): la fuente era `> K` y no `>= K+1` (BustedOverlayScreen en la r85)
  sin_signo    alineado, tras un `test`/`cmp` con cero: speed.exe `jbe`/`ja` contra nuestro `je`/`jne` (la fuente era
               `x > 0`/`x <= 0` con x sin signo o un unsigned short/char promovido, y nosotros `x != 0`/`x == 0`;
               GCC lo pliega igual: FastMem::Init y InitBuffersAsRequired de FnDeltaF1/F3 en la r87)
  sin_signo_inv al reves (speed.exe `je`/`jne` y el nuestro `jbe`/`ja`)
--tabla: la tabla 'speed.exe -> nuestro' de las sustituciones alineadas (mismo numero de instrucciones) en las funciones
con UNA o DOS filas distintas (las que una forma de la fuente puede cerrar), con forma de operando (registro -> R,
inmediato -> I salvo 0/1, memoria -> M). N = filas de la tabla (40).
Lee orig/SPEED_EXE_1_3/speed.exe, config/SPEED_EXE_1_3/symbols.json y los .obj de build/SPEED_EXE_1_3/src; capstone.
No escribe nada.
"""
import argparse
import collections
import difflib
import json
import os
import re
import struct

import capstone

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
REG = r'\b(e?[abcd]x|e?[sd]i|e?[sb]p|[abcd][lh])\b'


class PE32:
    def __init__(self, ruta):
        d = open(ruta, 'rb').read()
        self.d = d
        pe, = struct.unpack_from('<I', d, 0x3C)
        nsec, = struct.unpack_from('<H', d, pe + 6)
        optsz, = struct.unpack_from('<H', d, pe + 20)
        self.base, = struct.unpack_from('<I', d, pe + 24 + 28)
        so = pe + 24 + optsz
        self.secs = []
        for i in range(nsec):
            _, vs, va, rs, rp = struct.unpack_from('<8sIIII', d, so + 40 * i)
            self.secs.append((va, max(vs, rs), rp))

    def lee(self, va, n):
        rva = va - self.base
        for sva, sz, rp in self.secs:
            if sva <= rva < sva + sz:
                o = rp + rva - sva
                return self.d[o:o + n]
        return b''


def simbolos_coff(ruta):
    """{nombre: bytes} de las funciones (tipo 0x20) definidas en un .obj COFF (de su seccion, desde su valor)."""
    d = open(ruta, 'rb').read()
    _, _, _, symptr, nsym, optsz, _ = struct.unpack_from('<HHIIIHH', d, 0)
    strtab = symptr + 18 * nsym
    out = {}
    i = 0
    while i < nsym:
        o = symptr + 18 * i
        raw = d[o:o + 8]
        val, sec, typ, _, naux = struct.unpack_from('<IhHBB', d, o + 8)
        if raw[:4] == b'\0\0\0\0':
            off, = struct.unpack_from('<I', raw, 4)
            s = d[strtab + off:d.index(b'\0', strtab + off)].decode('latin-1')
        else:
            s = raw.rstrip(b'\0').decode('latin-1')
        if sec > 0 and typ == 0x20:
            so = 20 + optsz + 40 * (sec - 1)
            rs, rp = struct.unpack_from('<II', d, so + 16)
            out[s] = d[rp + val: rp + rs]
        i += 1 + naux
    return out


def ins(b, va):
    out = []
    for x in MD.disasm(b, va):
        out.append((x.mnemonic, x.op_str))
        if x.mnemonic == 'ret' and x.address + x.size - va >= len(b.rstrip(b'\xcc')):
            break
    return out


def setcc(l):
    return sum(1 for i in range(len(l) - 2)
               if l[i] == ('xor', 'eax, eax') and l[i + 1][0] in ('test', 'cmp') and l[i + 2][0].startswith('set'))


def setcc_sin(l):
    return sum(1 for i in range(1, len(l))
               if l[i][0].startswith('set') and l[i][1] == 'al' and l[i - 1][0] in ('test', 'cmp')
               and not (i >= 2 and l[i - 2] == ('xor', 'eax, eax')))


def cola(l):
    return sum(1 for i in range(len(l) - 1) if l[i][0] == 'call' and l[i + 1][0] == 'ret')


def es_cmp1(l, i):
    return i + 1 < len(l) and l[i][0] == 'cmp' and l[i][1].endswith(', 1') and l[i + 1][0] in ('jl', 'jge')


def es_test(l, i):
    return i + 1 < len(l) and l[i][0] == 'test' and l[i + 1][0] in ('jle', 'jg')


def uno(le, lo):
    """pares alineados cmp 1/jl|jge contra test/jle|jg, en los dos sentidos."""
    sm = difflib.SequenceMatcher(None, [m for m, _ in le], [m for m, _ in lo], autojunk=False)
    n1 = n2 = 0
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag != 'replace':
            continue
        for k in range(min(i2 - i1, j2 - j1)):
            i, j = i1 + k, j1 + k
            if es_cmp1(le, i) and es_test(lo, j):
                n1 += 1
            if es_cmp1(lo, j) and es_test(le, i):
                n2 += 1
    return n1, n2


PAR_K = {('jle', 'jl'), ('jg', 'jge'), ('jbe', 'jb'), ('ja', 'jae'), ('jl', 'jle'), ('jge', 'jg'), ('jb', 'jbe'),
         ('jae', 'ja')}


def inm(op):
    m = re.search(r', (-?0x[0-9a-f]+|-?\d+)$', op)
    return int(m.group(1), 0) if m else None


def constante(le, lo):
    """alineado: speed.exe `cmp X,K; jle` contra nuestro `cmp X,K+1; jl` (y los demas pares: `> K` frente a
    `>= K+1`, con o sin signo). Cuenta las parejas."""
    sm = difflib.SequenceMatcher(None, [m for m, _ in le], [m for m, _ in lo], autojunk=False)
    n = 0
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag not in ('replace', 'equal'):
            continue
        for k in range(min(i2 - i1, j2 - j1)):
            i, j = i1 + k, j1 + k
            if i + 1 >= len(le) or j + 1 >= len(lo) or le[i][0] != 'cmp' or lo[j][0] != 'cmp':
                continue
            a, b = inm(le[i][1]), inm(lo[j][1])
            if a is None or b is None or abs(a - b) != 1 or le[i][1].split(',')[0] != lo[j][1].split(',')[0]:
                continue
            if (le[i + 1][0], lo[j + 1][0]) in PAR_K:
                n += 1
    return n


PAR_U = {('jbe', 'je'), ('ja', 'jne')}


def con_cero(x):
    return x[0] == 'test' or (x[0] == 'cmp' and (x[1].endswith(', 0') or re.search(r', (e?[a-d]x|e?[sd]i|[a-d]l)$',
                                                                                  x[1]) is not None))


def sin_signo(le, lo):
    """alineado: speed.exe `jbe`/`ja` tras test/cmp contra el `je`/`jne` nuestro (n1) y al reves (n2)."""
    sm = difflib.SequenceMatcher(None, [m for m, _ in le], [m for m, _ in lo], autojunk=False)
    n1 = n2 = 0
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag != 'replace':
            continue
        for k in range(min(i2 - i1, j2 - j1)):
            i, j = i1 + k, j1 + k
            if i == 0 or j == 0 or le[i - 1][0] != lo[j - 1][0] or not con_cero(le[i - 1]):
                continue
            if (le[i][0], lo[j][0]) in PAR_U:
                n1 += 1
            elif (lo[j][0], le[i][0]) in PAR_U:
                n2 += 1
    return n1, n2


def sinsaltos(l):
    return [(m, re.sub(r'0x[0-9a-f]{3,}', 'A', o)) for m, o in l]


def orden(le, lo):
    """mismo largo: posiciones con dos instrucciones contiguas intercambiadas, una al menos escribe en memoria."""
    if len(le) != len(lo):
        return 0
    a, b = sinsaltos(le), sinsaltos(lo)
    n = 0
    i = 0
    while i < len(a) - 1:
        if a[i] != b[i] and a[i] == b[i + 1] and a[i + 1] == b[i] and \
                any(x[0] == 'mov' and x[1].split(',')[0].strip().endswith(']') for x in (a[i], a[i + 1])):
            n += 1
            i += 2
        else:
            i += 1
    return n


def forma(m, o):
    o = re.sub(r'\[[^\]]*\]', 'M', o)
    o = re.sub(REG, 'R', o)
    o = re.sub(r'\b0x[0-9a-f]+\b|\b[2-9]\b', 'I', o)
    return m + ' ' + o


def parciales(rep, va_de, minimo):
    for u in rep['units']:
        nombre = u['name'].split('/', 1)[1] if u['name'].startswith('main/') else u['name']
        ruta = os.path.join(RAIZ, 'build', 'SPEED_EXE_1_3', 'src', nombre.replace('/', os.sep) + '.obj')
        parc = [f for f in u.get('functions', []) if f.get('fuzzy_match_percent') not in (None, 100.0)
                and f.get('fuzzy_match_percent', 0) > minimo and f['name'] in va_de]
        if not parc or not os.path.exists(ruta):
            continue
        try:
            so = simbolos_coff(ruta)
        except Exception as e:  # noqa: BLE001
            print('no leo', ruta, e)
            continue
        for f in parc:
            if f['name'] in so:
                yield nombre.split('/')[-1], f, so[f['name']]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--informe', default=os.path.join(RAIZ, 'build', 'versiones', 'SPEED_EXE_1_3.json'))
    ap.add_argument('--lista', action='store_true')
    ap.add_argument('--tabla', nargs='?', const=40, type=int)
    ap.add_argument('--min', type=float, default=0.0, help='fuzzy minimo (la tabla usa 50 si no se da)')
    a = ap.parse_args()
    exe = PE32(os.path.join(RAIZ, 'orig', 'SPEED_EXE_1_3', 'speed.exe'))
    sy = json.load(open(os.path.join(RAIZ, 'config', 'SPEED_EXE_1_3', 'symbols.json'), encoding='utf-8'))
    va_de = {}
    for f in sy['functions']:
        va_de.setdefault(f['name'], (f['start'], f['end']))
    rep = json.load(open(a.informe, encoding='utf-8'))
    if a.tabla:
        cnt = collections.Counter()
        ej = collections.defaultdict(list)
        for _, f, b in parciales(rep, va_de, a.min or 50.0):
            n = f['name']
            s, e = va_de[n]
            le = sinsaltos(ins(exe.lee(s, e - s), s))
            lo = sinsaltos(ins(b, 0))
            if len(le) != len(lo):
                continue
            dif = [(x, y) for x, y in zip(le, lo)
                   if x[0] != y[0] or (x[1] != y[1] and not x[0].startswith('j') and x[0] != 'call')]
            if not dif or len(dif) > 2:
                continue
            k = tuple((forma(*x), forma(*y)) for x, y in dif)
            cnt[k] += 1
            ej[k].append((int(f.get('size') or 0), n))
        for k, c in cnt.most_common(a.tabla):
            print(c, sum(t for t, _ in ej[k]), ' || '.join('%s -> %s' % p for p in k))
            for t, n in sorted(ej[k], reverse=True)[:4]:
                print('      %5d %s' % (t, n[:100]))
        return
    res = collections.defaultdict(list)
    for uni, f, b in parciales(rep, va_de, a.min):
        n = f['name']
        s, e = va_de[n]
        le = ins(exe.lee(s, e - s), s)
        lo = ins(b, 0)
        fila = (int(f.get('size') or 0), uni, f['fuzzy_match_percent'], n)
        if setcc(le) > setcc(lo) and setcc_sin(lo) >= setcc_sin(le):
            res['setcc'].append(fila)
        if setcc(lo) > setcc(le) and setcc_sin(le) >= setcc_sin(lo):
            res['setcc_inv'].append(fila)
        if cola(le) != cola(lo):
            res['cola'].append(fila)
        n1, n2 = uno(le, lo)
        if n1:
            res['uno_exe'].append(fila)
        if n2:
            res['uno_nuestro'].append(fila)
        if orden(le, lo):
            res['orden'].append(fila)
        if constante(le, lo):
            res['constante'].append(fila)
        n1, n2 = sin_signo(le, lo)
        if n1:
            res['sin_signo'].append(fila)
        if n2:
            res['sin_signo_inv'].append(fila)
    for k in ('setcc', 'setcc_inv', 'cola', 'uno_exe', 'uno_nuestro', 'orden', 'constante', 'sin_signo',
              'sin_signo_inv'):
        v = res.get(k, [])
        print('== %s: %d fn, %d B' % (k, len(v), sum(x[0] for x in v)))
        if a.lista:
            for x in sorted(v, reverse=True):
                print('  %5d %-18s %8.3f %s' % (x[0], x[1][:18], x[2], x[3][:110]))


if __name__ == '__main__':
    main()
