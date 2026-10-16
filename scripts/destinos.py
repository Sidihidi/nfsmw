#!/usr/bin/env python3
"""destinos.py -- funciones CASADAS que llaman (o cargan) un simbolo distinto que el original.

La medida compara con `functionRelocDiffs=none`: una funcion sale al 100 % aunque su `bl`/`jal` vaya a otra
funcion (CreateObject que construye la clase equivocada, `Slope::Regenerate` donde el original llama a
`Slope::GetValue`). El codigo maquina es el mismo; el destino no, y en el juego (y en los ports) se ejecuta otra
cosa. callcheck.py lo mira por UNIDAD y solo en GameCube y solo hacia fuera; esto lo mira por FUNCION, en GameCube y
en las dos PS2, y tambien las llamadas dentro de la misma unidad.

Para cada funcion al 100 % del informe (build/versiones/V.json) lee las reubicaciones de su rango en el objeto
original (build/V/obj/...) y en el nuestro (build/V/src/...), resuelve las que van contra el simbolo de la SECCION
(nuestro ensamblador las emite asi: addend explicito en PowerPC; en MIPS se decodifica del jal, del par %hi/%lo o
del gprel) al simbolo con nombre que haya en esa direccion, y compara destino a destino por posicion. Las que caen
dentro de la propia funcion (saltos, tablas) y las anonimas (`@123`, `$LC`, `lbl_`, `D_`, ...) no se comparan.

Clases de diferencia:
  OTRA      llamada (bl/jal/j) a una funcion que el original TAMBIEN tiene, pero no es la que llama: fallo de fuente.
  NOMBRE    llamada a un nombre que el original no tiene: es la misma funcion con otro nombre (o una que falta).
  DATO      direccion cargada (lis/addi, lui/addiu, sda/gp) de otro simbolo con nombre.
  DESPL     el mismo simbolo de datos con otro desplazamiento (otro miembro u otro elemento).
  FALTA     una llamada con nombre en un lado y ninguna reubicacion en el otro.

    python scripts/destinos.py [V ...] [--unidad U] [--clase OTRA,NOMBRE] [--tsv FICHERO]
        por defecto GOWE69 SLES-53558-A124 SLUS-21351, todas las SourceLists y todas las clases

Xbox 360 (EUROPEGERMILESTONE, ronda 54) se pide EXPLICITA y va por otro camino (compara_360): los objetos del
enlace LTCG troceado, COFF con reubicaciones contra simbolos con nombre. Alli los nombres del original NO son
fiables: una OTRA de 360 es antes un nombre falso de symbols.txt que un fallo de fuente; miralo con
scripts/llamantes360.py --dis y con la rutina de nombres (scripts/rutina360.py) antes de tocar la fuente.
"""
import argparse
import json
import os
import re
import struct
import sys
from collections import Counter

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSIONES = ['GOWE69', 'SLES-53558-A124', 'SLUS-21351']

# nombres que pone el compilador o el troceador: no dicen nada del destino
ANONIMO = re.compile(r'^(@|\$|\.|_\.tmp_|lbl_|D_|jtbl_|jumptable_|func_|fn_|LAB_|DAT_|switch_|_GLOBAL_\$|__sinit|__sdata|_SDA|gcc2_compiled|__gnu_compiled|__tcf_|_\d+_GLOBAL_)')
SUFIJO = re.compile(r'([.$]\d+)?(_8[0-9A-F]{7})?$')

PPC_LLAMADA = {10}            # R_PPC_REL24 (bl, b)
MIPS_LLAMADA = {4}            # R_MIPS_26 (jal, j)


class Elf:
    def __init__(self, ruta):
        d = open(ruta, 'rb').read()
        self.d = d
        e = '<' if d[5] == 1 else '>'
        self.e = e
        self.mips = struct.unpack_from(e + 'H', d, 18)[0] == 8
        shoff, = struct.unpack_from(e + 'I', d, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from(e + 'HHH', d, 0x2e)
        self.secs = [struct.unpack_from(e + 'IIIIIIIIII', d, shoff + i * shentsize) for i in range(shnum)]
        ss = self.secs[shstrndx]
        self.secnom = [self._cad(ss[4] + s[0]) for s in self.secs]
        self.syms = []
        for s in self.secs:
            if s[1] != 2:
                continue
            strtab = self.secs[s[6]]
            for k in range(s[5] // 16):
                n, v, t, info, otro, shndx = struct.unpack_from(e + 'IIIBBH', d, s[4] + k * 16)
                self.syms.append((self._cad(strtab[4] + n), v, t, info & 0xF, info >> 4, shndx))
            break
        # simbolos con nombre por seccion: {sec: {valor: [indices]}}
        self.porval = {}
        self.pornombre = {}
        for i, (n, v, t, ty, b, sh) in enumerate(self.syms):
            if sh == 0 or sh >= 0xff00 or ty in (3, 4) or not n:
                continue
            self.porval.setdefault(sh, {}).setdefault(v, []).append(i)
            if ty == 2 or (b in (1, 2) and t):
                self.pornombre.setdefault(n, i)
        self._rel = {}
        self._orden = {}

    def _cad(self, o):
        f = self.d.index(b'\0', o)
        return self.d[o:f].decode('latin-1')

    def relocs(self, sec):
        """[(off, tipo, symidx, addend)] de una seccion, con el addend implicito de MIPS resuelto."""
        if sec in self._rel:
            return self._rel[sec]
        e, d = self.e, self.d
        out = []
        for s in self.secs:
            if s[1] not in (4, 9) or s[7] != sec:
                continue
            rela = s[1] == 4
            ent = s[9] or (12 if rela else 8)
            datos = self.secs[sec]
            pend = {}  # HI16 pendientes por simbolo
            ultimo_hi = {}
            vistos = {}
            for k in range(s[5] // ent):
                if rela:
                    off, info, add = struct.unpack_from(e + 'IIi', d, s[4] + k * ent)
                else:
                    off, info = struct.unpack_from(e + 'II', d, s[4] + k * ent)
                    add = None
                ty = info & 0xFF
                si = info >> 8
                if add is None:
                    ins, = struct.unpack_from(e + 'I', d, datos[4] + off)
                    if ty == 4:
                        add = (ins & 0x3FFFFFF) << 2
                    elif ty == 5:
                        pend.setdefault(si, []).append(len(out))
                        ultimo_hi[si] = ins & 0xFFFF
                        v = vistos.setdefault(si, [])
                        if ins & 0xFFFF in v:
                            v.remove(ins & 0xFFFF)
                        v.insert(0, ins & 0xFFFF)
                        out.append([off, ty, si, None])
                        continue
                    elif ty == 6:
                        lo = ins & 0xFFFF
                        lo = lo - 0x10000 if lo & 0x8000 else lo
                        if pend.get(si):
                            add = (ultimo_hi.get(si, 0) << 16) + lo
                            for j in pend.pop(si):
                                out[j][3] = add
                        else:
                            # %lo que reutiliza un %hi anterior (el registro base ya cargado): de los %hi vistos
                            # contra ese simbolo, el que cae al principio de un simbolo con nombre
                            add = None
                            for h in vistos.get(si, []):
                                if (h << 16) + lo in self.porval.get(self.syms[si][5], {}):
                                    add = (h << 16) + lo
                                    break
                            if add is None:
                                add = (ultimo_hi.get(si, 0) << 16) + lo
                    elif ty == 7:
                        add = ins & 0xFFFF
                        add = add - 0x10000 if add & 0x8000 else add
                    elif ty == 2:
                        add = ins
                    else:
                        add = 0
                out.append([off, ty, si, add])
        out = [tuple(x) for x in out]
        self._rel[sec] = out
        return out

    def destino(self, si, add):
        """(nombre, seccion, direccion, desplazamiento desde el simbolo) del destino; nombre '' si es anonimo.

        Contra el simbolo de la seccion se busca el simbolo con nombre que hay en esa direccion, o el anterior mas
        cercano de la misma seccion (y entonces el desplazamiento no es cero)."""
        n, v, t, ty, b, sh = self.syms[si]
        if ty != 3 and n:  # no es el simbolo de la seccion
            if sh == 0 or sh >= 0xff00:
                return n, sh, None, add or 0
            return n, sh, v + (add or 0), add or 0
        if add is None:
            return '', sh, None, 0
        tabla = self.porval.get(sh, {})
        if sh not in self._orden:
            self._orden[sh] = sorted(tabla)
        orden = self._orden[sh]
        import bisect
        k = bisect.bisect_right(orden, add) - 1
        if k < 0:
            return '', sh, add, 0
        base = orden[k]
        mejor = ''
        for i in tabla[base]:
            nn, vv, tt, tyy, bb, _ = self.syms[i]
            if base != add and not (tt and base <= add < base + tt):
                continue  # el anterior no llega hasta aqui
            if ANONIMO.match(nn):
                if not mejor:
                    mejor = nn
                continue
            if not mejor or ANONIMO.match(mejor) or bb in (1, 2):
                mejor = nn
        return mejor, sh, add, add - base

    def direccion(self, nombre):
        """(seccion, valor) de un simbolo definido por nombre, o None."""
        i = self.pornombre.get(nombre)
        if i is None:
            for j, x in enumerate(self.syms):
                if x[0] == nombre and 0 < x[5] < 0xff00:
                    i = j
                    break
        if i is None:
            return None
        return self.syms[i][5], self.syms[i][1]


def ruta_objs(v, unidad):
    base = os.path.join(RAIZ, 'build', v)
    rel = unidad.split('/', 1)[1] + '.o' if '/' in unidad else unidad + '.o'
    return os.path.join(base, 'obj', rel), os.path.join(base, 'src', rel)


def norm(n):
    return SUFIJO.sub('', n)


def nombres_original(v):
    """{nombre: direccion} de config/V/symbols.txt (la direccion None si no la tiene)."""
    out = {}
    rx = re.compile(r'^\s*(\S+)\s*=\s*(?:\.?\w+:)?(0x[0-9A-Fa-f]+)')
    with open(os.path.join(RAIZ, 'config', v, 'symbols.txt'), encoding='utf-8', errors='replace') as fh:
        for l in fh:
            m = rx.match(l)
            if m:
                out[m.group(1)] = int(m.group(2), 16)
            elif '=' in l:
                out.setdefault(l.split('=', 1)[0].strip(), None)
    return out


def compara_funcion(eo, en, nombre, todos):
    io = eo.pornombre.get(nombre)
    inn = en.pornombre.get(nombre)
    if io is None or inn is None:
        return None
    _, vo, to, _, _, so = eo.syms[io]
    _, vn, tn, _, _, sn = en.syms[inn]
    if not to or to != tn:
        return None
    llamada = MIPS_LLAMADA if eo.mips else PPC_LLAMADA

    def lista(elf, sec, v0, tam):
        r = {}
        for off, ty, si, add in elf.relocs(sec):
            if not (v0 <= off < v0 + tam):
                continue
            nom, sh, val, des = elf.destino(si, add)
            if sh == sec and val is not None and v0 <= val < v0 + tam:
                continue  # salto o tabla dentro de la propia funcion
            r.setdefault(off - v0, []).append((ty, nom, sh, val, des))
        return r

    def misma_direccion(x, y):
        """El nombre x del original, puesto en NUESTRO objeto con su desplazamiento, cae donde nuestro y (o al reves):
        es la misma direccion con otro nombre (el final de un array, un miembro de una estructura)."""
        d = en.direccion(x[1])
        if d and y[3] is not None and d[0] == y[2] and d[1] + x[4] == y[3]:
            return True
        d = eo.direccion(y[1])
        if d and x[3] is not None and d[0] == x[2] and d[1] + y[4] == x[3]:
            return True
        # con las direcciones del original (symbols.txt): el final de un array es el principio del siguiente
        ax, ay = todos.get(x[1]), todos.get(y[1])
        if ax is not None and ay is not None and ax + x[4] == ay + y[4]:
            return True
        return False

    ro = lista(eo, so, vo, to)
    rn = lista(en, sn, vn, tn)
    difs = []
    for off in sorted(set(ro) | set(rn)):
        a = [x for x in ro.get(off, []) if x[1] and not ANONIMO.match(x[1])]
        b = [x for x in rn.get(off, []) if x[1] and not ANONIMO.match(x[1])]
        if not a and not b:
            continue
        if a and b:
            na, nb = norm(a[0][1]), norm(b[0][1])
            if na == nb:
                if a[0][4] != b[0][4] and a[0][0] not in llamada:
                    difs.append(('DESPL', off, '%s+0x%x' % (a[0][1], a[0][4]), '%s+0x%x' % (b[0][1], b[0][4])))
                continue
            if misma_direccion(a[0], b[0]):
                continue
            if a[0][0] in llamada:
                clase = 'OTRA' if b[0][1] in todos else 'NOMBRE'
            else:
                clase = 'DATO'
            difs.append((clase, off, '%s+0x%x' % (a[0][1], a[0][4]) if a[0][4] else a[0][1],
                         '%s+0x%x' % (b[0][1], b[0][4]) if b[0][4] else b[0][1]))
        elif a and not ro.get(off) == rn.get(off):
            # el original nombra un destino y el nuestro no tiene reubicacion (o es anonima)
            if off in rn or a[0][0] not in llamada:
                continue  # anonimo en la misma posicion, o un %ha que el troceador no emparejo
            difs.append(('FALTA', off, a[0][1], '-'))
        elif b and off not in ro and b[0][0] in llamada:
            difs.append(('FALTA', off, '-', b[0][1]))
    return difs


X360 = 'EUROPEGERMILESTONE'
COFF_LLAMADA = 6          # IMAGE_REL_PPC_REL24
ANONIMO_360 = re.compile(r'^(fn|lbl|sub|func)_[0-9A-F]{8}$|^ltcg_|^__real@|^\?\?_C@|^\$|^\.|^__(save|rest)(gpr|fpr|vmx)')


def compara_360(rep, unidades, todas):
    """Xbox 360 (ronda 54): la medida oficial sale del enlace LTCG troceado (scripts/ltcg360.py), asi que se comparan
    build/<V>/ltcg/orig (el original troceado con symbols.txt) y build/<V>/ltcg/obj (nuestro enlace troceado). Las
    reubicaciones de esos COFF van contra simbolos con nombre (sin desplazamiento): b/bl que salen de la funcion
    (REL24), pares lis/addi (REFHI/REFLO) y palabras (ADDR32). CUIDADO: en 360 los nombres del ORIGINAL son heredados y
    muchos son falsos; una diferencia suele ser un nombre falso de symbols.txt antes que un fallo de fuente (por eso
    van aparte las clases que el resto de la rutina de nombres de 360 ya corrige: ver scripts/rutina360.py)."""
    sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
    import nombra_grafo as ng
    todos = nombres_original(X360)
    base = os.path.join(RAIZ, 'build', X360, 'ltcg')
    filas, cuenta, nfun = [], Counter(), 0
    for u in rep['units']:
        corto = u['name'].rsplit('/', 1)[-1]
        if unidades and corto not in unidades:
            continue
        rel = os.path.join('Speed', 'Indep', 'SourceLists', corto + '.obj')
        po, pn = os.path.join(base, 'orig', rel), os.path.join(base, 'obj', rel)
        if not (os.path.exists(po) and os.path.exists(pn)):
            continue
        fs = [f['name'] for f in u.get('functions', []) if todas or f.get('fuzzy_match_percent') == 100.0]
        if not fs:
            continue
        fo, fn = ng.Coff(po).funciones(), ng.Coff(pn).funciones()
        for f in fs:
            if f not in fo or f not in fn or ANONIMO_360.match(f):
                continue
            (pa, ra), (pb, rb) = fo[f], fn[f]
            if len(pa) != len(pb):
                continue
            nfun += 1
            for k in sorted(set(ra) | set(rb)):
                a, b = ra.get(k), rb.get(k)
                na = a[1] if a and a[0] != ng.PPC_PAIR and not ANONIMO_360.match(a[1] or '') else ''
                nb = b[1] if b and b[0] != ng.PPC_PAIR and not ANONIMO_360.match(b[1] or '') else ''
                if not na and not nb:
                    continue
                if na and nb:
                    if ng.equivalentes(na, nb) or norm(na) == norm(nb):
                        continue
                    if na.replace('??_G', '??_E', 1) == nb.replace('??_G', '??_E', 1):
                        continue   # ??_G/??_E de la misma clase: el troceado de 360 los tiene como alias
                    if a[0] == COFF_LLAMADA:
                        clase = 'OTRA' if nb in todos else 'NOMBRE'
                    else:
                        clase = 'DATO'
                elif a and b:
                    continue   # anonimo en un lado y con nombre en el otro: el original aun no lo nombra
                elif (a or b)[0] != COFF_LLAMADA:
                    continue
                else:
                    clase = 'FALTA'
                cuenta[clase] += 1
                filas.append((X360, corto, f, clase, 4 * k, na or '-', nb or '-'))
    print('%s: %d funciones comparadas; %s' % (X360, nfun, dict(cuenta)), file=sys.stderr)
    return filas


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('versiones', nargs='*')
    ap.add_argument('--unidad', action='append', default=[])
    ap.add_argument('--clase', default='')
    ap.add_argument('--tsv')
    ap.add_argument('--todas', action='store_true', help='tambien las funciones que no estan al 100 %%')
    a = ap.parse_args()
    vs = a.versiones or VERSIONES
    clases = set(a.clase.split(',')) if a.clase else None
    filas = []
    for v in vs:
        rep = json.load(open(os.path.join(RAIZ, 'build', 'versiones', v + '.json')))
        if v == X360:
            filas += [r for r in compara_360(rep, a.unidad, a.todas) if not clases or r[3] in clases]
            continue
        todos = nombres_original(v)
        cuenta = Counter()
        nfun = 0
        for u in rep['units']:
            nu = u['name']
            corto = nu.rsplit('/', 1)[-1]
            if a.unidad and corto not in a.unidad:
                continue
            po, pn = ruta_objs(v, nu)
            if not (os.path.exists(po) and os.path.exists(pn)):
                continue
            fs = [f['name'] for f in u.get('functions', [])
                  if a.todas or f.get('fuzzy_match_percent') == 100.0]
            if not fs:
                continue
            try:
                eo, en = Elf(po), Elf(pn)
            except Exception as ex:  # objeto COFF o roto
                print('%s %s: %s' % (v, corto, ex), file=sys.stderr)
                continue
            for f in fs:
                if ANONIMO.match(f):
                    continue
                difs = compara_funcion(eo, en, f, todos)
                if difs is None:
                    continue
                nfun += 1
                for clase, off, o, n in difs:
                    if clases and clase not in clases:
                        continue
                    cuenta[clase] += 1
                    filas.append((v, corto, f, clase, off, o, n))
        print('%s: %d funciones comparadas; %s' % (v, nfun, dict(cuenta)), file=sys.stderr)
    if a.tsv:
        with open(a.tsv, 'w', encoding='utf-8', newline='\n') as fh:
            for r in filas:
                fh.write('%s\t%s\t%s\t%s\t0x%x\t%s\t%s\n' % r)
    else:
        for r in filas:
            print('%s\t%s\t%s\t%s\t0x%x\toriginal=%s\tnuestro=%s' % r)


if __name__ == '__main__':
    main()
