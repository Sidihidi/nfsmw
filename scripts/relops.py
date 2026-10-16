#!/usr/bin/env python3
"""relops.py -- el oraculo de stl_relops.h para PS2.

La STL de SGI de EE-GCC define operator!=, >, <= y >= como plantillas GLOBALES (stl_relops.h, que entra con
stl_algobase.h, <list>, <utility>...). Con los dos operandos del MISMO tipo enum, bool o clase, la plantilla es
coincidencia exacta y gana al operador del lenguaje: el codigo cambia (el valor se calcula y luego se salta, y GCC
ya no pliega `x != A && x != B` al rango sin signo). GameCube no las tiene, asi que alli las dos formas dan lo mismo.

El .mdebug de la A124 marca cada expansion: una etiqueta FILE .../stl_relops.h seguida de $LM con la linea 38 (!=),
43 (>), 48 (<=) o 53 (>=). Nuestros objetos de PS2 llevan tambien .mdebug, asi que se puede comparar funcion a funcion.

    python scripts/relops.py lineas SIM[,SIM...] [OBJ]   cada expansion con la linea del fichero principal que la
                                                         precede, en el original y en nuestro objeto (por defecto se
                                                         busca en build/SLES-53558-A124/src)
    python scripts/relops.py difieren [N]                funciones donde el original y nosotros no expandimos los
                                                         mismos operadores (tipos de operador distintos primero)

Cuidado: el planificador puede partir una expansion en dos, asi que los RECUENTOS meten ruido; lo fiable es que un
operador aparezca en un lado y no en el otro, y la linea. Que el original NO use la plantilla donde nosotros si puede
ser un == invertido, otro tipo (int en vez del enum), un switch, o que la STL aun no estuviera incluida en ese punto
de la unidad (nuestra zFEng no la incluye y la del original si).
"""
import collections
import glob
import os
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import mdebug  # noqa: E402

OPS = {37: '!=', 38: '!=', 42: '>', 43: '>', 47: '<=', 48: '<=', 52: '>=', 53: '>='}


def seccion_mdebug(ruta):
    """(bytes, desplazamiento, tamano) de la seccion .mdebug de un ELF"""
    b = open(ruta, 'rb').read()
    shoff = struct.unpack_from('<I', b, 0x20)[0]
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', b, 0x2E)
    secs = [struct.unpack_from('<IIIIIIIIII', b, shoff + i * shentsize) for i in range(shnum)]
    tab = secs[shstrndx]
    for s in secs:
        if b[tab[4] + s[0]:b.index(b'\0', tab[4] + s[0])] == b'.mdebug':
            return b, s[4], s[5]
    return b, None, None


class Tabla:
    """la tabla de simbolos (SYMR) de un .mdebug, con un indice de procedimientos por nombre"""

    def __init__(self, raw, base, n, ss):
        self.raw, self.base, self.n, self.ss = raw, base, n, ss
        self.idx = {}
        for i in range(n):
            iss, value, bits = struct.unpack_from('<III', raw, base + i * 12)
            if bits & 0x3f == 6:
                self.idx.setdefault(self.cad(iss), i)

    def cad(self, iss):
        e = self.ss.find(b'\0', iss)
        return self.ss[iss:e].decode('latin-1') if 0 <= iss < len(self.ss) else ''

    def lms(self, sim):
        """[(fichero, linea, direccion)] de los $LM de una funcion"""
        i0 = self.idx.get(sim)
        if i0 is None:
            return None
        fuera = []
        fichero = ''
        for i in range(i0 + 1, self.n):
            iss, value, bits = struct.unpack_from('<III', self.raw, self.base + i * 12)
            st = bits & 0x3f
            if st == 6:
                break
            if st != 5:
                continue
            nom = self.cad(iss)
            if nom.startswith('$LM'):
                fuera.append((fichero, bits >> 12, value))
            elif nom.endswith(('.cpp', '.h', '.hpp', '.c', '.inl')) or '/src/' in nom or '\\' in nom:
                fichero = nom.replace('\\', '/')
        return fuera


def expansiones(lms):
    """(fichero principal, [(operador, linea del principal que la precede)])"""
    principal = None
    for f, l, a in lms:
        if f.lower().endswith(('.cpp', '.c')):
            principal = f
            break
    res = []
    ultima = None
    dentro = False
    for f, l, a in lms:
        if f.endswith('stl_relops.h'):
            if l in OPS and (not dentro or (res and res[-1][0] != OPS[l])):
                res.append((OPS[l], ultima))
            dentro = True
            continue
        dentro = False
        if f == principal:
            ultima = l
    return principal, res


_orig = None


def original():
    global _orig
    if _orig is None:
        f = open(mdebug.ELF, 'rb')
        f.seek(mdebug.SYM_OFF)
        raw = f.read(mdebug.SYM_N * 12)
        f.seek(mdebug.SS_OFF)
        ss = f.read(mdebug.SS_SIZE)
        f.close()
        _orig = Tabla(raw, 0, mdebug.SYM_N, ss)
    return _orig


_objs = {}


def objeto(ruta):
    if ruta not in _objs:
        b, off, tam = seccion_mdebug(ruta)
        if off is None:
            _objs[ruta] = None
        else:
            h = struct.unpack_from('<HH23I', b, off)
            isymMax, cbSymOffset, issMax, cbSsOffset = h[9], h[10], h[15], h[16]
            base = 0
            if cbSymOffset + isymMax * 12 > len(b) or cbSsOffset + issMax > len(b):
                base = off  # en un objeto las posiciones pueden ser de la seccion y no del fichero
            _objs[ruta] = Tabla(b, base + cbSymOffset, isymMax, b[base + cbSsOffset:base + cbSsOffset + issMax])
    return _objs[ruta]


def nuestros_objetos():
    return glob.glob(os.path.join(RAIZ, 'build', 'SLES-53558-A124', 'src', '**', '*.o'), recursive=True)


def informe(sim, objs):
    out = []
    lms = original().lms(sim)
    if lms is None:
        return '%s: no esta en el original' % sim
    princ, exp = expansiones(lms)
    out.append('== %s   original %s: %s' % (sim, os.path.basename(princ or '?'),
                                            ', '.join('%s@%s' % (o, l) for o, l in exp) or '(ninguna)'))
    for o in objs:
        t = objeto(o)
        if t is None:
            continue
        lms2 = t.lms(sim)
        if lms2 is None:
            continue
        princ2, exp2 = expansiones(lms2)
        ruta = None
        for c in ([princ2] if princ2 and os.path.isabs(princ2) else
                  [os.path.join(RAIZ, princ2 or ''), os.path.join(RAIZ, 'src', princ2 or '')]):
            if princ2 and os.path.isfile(c):
                ruta = c
                break
        texto = open(ruta, encoding='utf-8', errors='replace').read().split('\n') if ruta else []
        out.append('   nuestro %s:' % (os.path.relpath(ruta, RAIZ) if ruta else '? (%s)' % princ2))
        for op, l in exp2:
            out.append('     %-3s @%-5s %s' % (op, l, texto[l - 1].strip()[:120] if texto and l else ''))
        if not exp2:
            out.append('     (ninguna)')
        break
    return '\n'.join(out)


def difieren(n):
    """funciones cuyas expansiones difieren entre el original y nuestros objetos de la A124"""
    t = original()
    orig = {}
    for sim in t.idx:
        lms = t.lms(sim)
        if lms and any(f.endswith('stl_relops.h') for f, l, a in lms):
            orig[sim] = collections.Counter(op for op, l in expansiones(lms)[1])
    nuestro = {}
    for o in nuestros_objetos():
        tb = objeto(o)
        if tb is None:
            continue
        for sim in tb.idx:
            lms = tb.lms(sim)
            if lms and any(f.endswith('stl_relops.h') for f, l, a in lms):
                nuestro[sim] = collections.Counter(op for op, l in expansiones(lms)[1])
    filas = []
    for sim in set(orig) | set(nuestro):
        a, b = orig.get(sim, collections.Counter()), nuestro.get(sim, collections.Counter())
        if a == b:
            continue
        fuerte = set(a) != set(b)
        filas.append((not fuerte, sim, dict(a - b), dict(b - a)))
    filas.sort()
    for debil, sim, falta, sobra in filas[:n]:
        print('%-6s falta=%-24s sobra=%-24s %s' % ('debil' if debil else 'FUERTE', falta, sobra, sim))
    print('%d funciones difieren (%d con un operador que solo esta en un lado)'
          % (len(filas), sum(1 for f in filas if not f[0])))


if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in ('lineas', 'difieren'):
        print(__doc__)
        sys.exit(2)
    if sys.argv[1] == 'lineas':
        objs = [sys.argv[3]] if len(sys.argv) > 3 else nuestros_objetos()
        for s in sys.argv[2].split(','):
            print(informe(s, objs))
    else:
        difieren(int(sys.argv[2]) if len(sys.argv) > 2 else 400)
