#!/usr/bin/env python3
"""Destructores de PS2 anclados por su VTABLE, en todo el troceado de una version.

En el EE-GCC de PS2 la vtable de una clase lleva 12 bytes a cero y despues pares (puntero, ajuste); el primer puntero
es el destructor. El destructor guarda en el objeto su propia vtable (un par %hi/%lo), asi que una funcion que carga
una vtable cuyo primer puntero es ella misma ES el destructor de esa clase; y la clase la dicen los nombres de los
demas metodos de esa vtable (Update__11SpeedometerP7IPlayer...), que tienen codigo distinto por clase y los nombra
bien la comparacion de codigo. Los destructores de clases hermanas, en cambio, son iguales salvo la vtable (que es una
reubicacion y no se compara), y ahi un nombre heredado por direccion puede caer en la hermana: paso en zFe2 (HUD) y en
zPhysicsBehaviors (Draw*Car).

La clase de una vtable sale de su HUELLA: la lista ordenada de los nombres de sus metodos (los propios y los heredados
sin redefinir), comparada con la de cada clase de la A124 (nombres de la .symtab). Solo se propone si encaja con una
sola clase: una derivada que no redefine nada tiene la misma huella que su base y no se decide. Si el glabel de splat
empieza 8 o 16 bytes antes de la entrada que dice la vtable (un trozo suelto delante, ver scripts/restos_trozos.py),
se nombra la entrada de verdad.

Informa de cada destructor cuyo nombre no cuadra con la clase de su vtable, y de cada func_ que es el destructor de una
clase con nombre. Control: en la A124 (nombres de la .symtab) tiene que salir 0.

    python scripts/vtable_dtors.py SLES-53558-A124          # control
    python scripts/vtable_dtors.py SLUS-21351 [--tsv cambios.tsv]

--tsv vuelca (direccion, nombre); se aplican con scripts/aplica_nombres.py y se mide. El 2026-09-23 salieron 127 en
SLUS (los del HUD de zFe2, contenedores UTL::Vector/FixedVector cruzados en zPhysics, zMain y zEAXSound, pantallas de
zFe...): +95 funciones casadas, sin ninguna perdida.
"""
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ELFS = {'SLUS-21351': 'orig/SLUS-21351/SLUS_213.51', 'SLES-53558-A124': 'orig/SLES-53558-A124/NFS.ELF'}
RE_INS = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s+[0-9A-F]{8}\s*\*/\s+(\S+)\s*(.*)$')
RE_HILO = re.compile(r'%(?:hi|lo)\(([^)+]+?)(?:\s*\+\s*(0x[0-9A-Fa-f]+|\d+))?\)')
SIN = re.compile(r'^(func|fn|sub|lbl)_[0-9A-Fa-f]+$')


class Imagen:
    """Bytes del ELF original por direccion virtual (segmentos PT_LOAD)."""

    def __init__(self, ruta):
        self.d = open(ruta, 'rb').read()
        phoff, = struct.unpack_from('<I', self.d, 0x1C)
        phentsize, phnum = struct.unpack_from('<HH', self.d, 0x2A)
        self.segs = []
        for i in range(phnum):
            p = struct.unpack_from('<IIIIIIII', self.d, phoff + i * phentsize)
            if p[0] == 1:
                self.segs.append((p[2], p[1], p[4]))

    def lee(self, v, n):
        for va, off, tam in self.segs:
            if va <= v < va + tam:
                return self.d[off + v - va:off + v - va + n]
        raise ValueError(hex(v))


def clase_de(nombre):
    """clase de un nombre mangled de metodo o destructor (11Infractions, Q23Sim6Entity...), o None"""
    m = re.match(r'^(?:_\$_|[A-Za-z_][A-Za-z0-9_]*?__C?)(Q(\d)((?:\d+[A-Za-z_][A-Za-z0-9_]*)+)|\d+[A-Za-z_][A-Za-z0-9_]*)',
                 nombre)
    if not m:
        return None
    s = m.group(1)
    if s[0] == 'Q':
        n, resto, out = int(s[1]), s[2:], s[:2]
        for _ in range(n):
            k = re.match(r'\d+', resto)
            if not k:
                return None
            ln = int(k.group(0))
            out += resto[:len(k.group(0)) + ln]
            resto = resto[len(k.group(0)) + ln:]
        return out
    k = re.match(r'\d+', s).group(0)
    return s[:len(k) + int(k)]


def lee_troceado(V):
    """[(ini, nombre, unidad, [direcciones %hi/%lo cargadas])] y {nombre: dir} de los .s y symbols.txt"""
    por_nom = {}
    for l in open(os.path.join(RAIZ, 'config', V, 'symbols.txt'), encoding='utf-8', errors='replace'):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);', l)
        if m:
            por_nom.setdefault(m.group(1), int(m.group(2), 16))
    base = os.path.join(RAIZ, 'build', V, 'asm')
    fns = []
    for dp, dn, fn in os.walk(base):
        for f in fn:
            if not f.endswith('.s'):
                continue
            p = os.path.join(dp, f)
            unidad = os.path.relpath(p, base).replace(os.sep, '/')[:-2]
            cur = None
            for l in open(p, encoding='utf-8', errors='replace'):
                m = re.match(r'\s*glabel (\S+)', l)
                if m:
                    cur = [None, m.group(1), unidad, set()]
                    fns.append(cur)
                    continue
                m = RE_INS.match(l)
                if m and cur is not None:
                    if cur[0] is None:
                        cur[0] = int(m.group(1), 16)
                    for s, add in RE_HILO.findall(m.group(3)):
                        a = int(s[2:], 16) if re.match(r'D_[0-9A-F]{8}$', s) else por_nom.get(s)
                        if a is not None:
                            cur[3].add(a + (int(add, 0) if add else 0))
                elif re.match(r'\s*endlabel', l):
                    cur = None
    return [f for f in fns if f[0] is not None], por_nom


def vtables_propias(V):
    """[(ini del destructor, nombre actual, unidad, (nombres de los metodos de su vtable, en orden))], {nombre: dir}"""
    img = Imagen(os.path.join(RAIZ, ELFS[V]))
    fns, por_nom = lee_troceado(V)
    nombre_en = {}
    for f in fns:
        nombre_en.setdefault(f[0], f[1])
    for n, a in por_nom.items():
        nombre_en.setdefault(a, n)
    out = []
    for ini, nombre, unidad, cargas in fns:
        for a in sorted(cargas):
            try:
                ws = struct.unpack('<32I', img.lee(a, 128))
            except (ValueError, struct.error):
                continue
            # el destructor puede empezar 8 o 16 bytes despues del glabel de splat: un trozo suelto de la funcion
            # anterior (ver scripts/restos_trozos.py) se queda delante con el func_; entonces se nombra la entrada
            # de verdad, la que dice la vtable, y el nombre actual del glabel no cuenta
            if ws[0] or ws[1] or ws[2] or not (ws[3] == ini or (ini < ws[3] <= ini + 16 and SIN.match(nombre))):
                continue
            metodos = []
            for k in range(5, 32, 2):
                if ws[k] == 0:
                    break
                metodos.append(nombre_en.get(ws[k], 'func_%08X' % ws[k]))
            if ws[3] == ini:
                out.append((ini, nombre, unidad, tuple(metodos)))
            else:
                out.append((ws[3], nombre_en.get(ws[3], 'func_%08X' % ws[3]), unidad, tuple(metodos)))
            break
    return out, por_nom


def encaja(ms, ma):
    """la vtable de SLUS (ms) tiene los mismos metodos que la de la A124 (ma): mismo largo y, donde SLUS tiene nombre,
    el mismo nombre; al menos un metodo con nombre"""
    if len(ms) != len(ma):
        return False
    con_nombre = [(s, a) for s, a in zip(ms, ma) if not SIN.match(s)]
    return bool(con_nombre) and all(s == a for s, a in con_nombre)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    V = args[0] if args else 'SLUS-21351'
    # la huella de cada clase sale de la A124, cuyos nombres son de la .symtab: la lista de metodos de su vtable (los
    # suyos y los que hereda sin redefinir). Una vtable de V es de la clase X si su lista encaja con la de X y con la
    # de ninguna otra clase (dos clases con la misma lista, p. ej. una derivada que no redefine nada, no se deciden).
    va, _ = vtables_propias('SLES-53558-A124')
    huella = collections.defaultdict(set)
    for ini, nombre, unidad, metodos in va:
        if nombre.startswith('_$_'):
            # las plantillas (t11FixedVector3Z...) no se parsean: cuentan con su nombre entero, para que una
            # huella compartida con otra clase salga ambigua y no se proponga
            huella[metodos].add(clase_de(nombre) or nombre)
    vs, por_nom = vtables_propias(V) if V != 'SLES-53558-A124' else (va, None)
    if por_nom is None:
        _, por_nom = lee_troceado(V)
    filas = []
    for ini, nombre, unidad, metodos in vs:
        actual = (clase_de(nombre) or nombre) if nombre.startswith('_$_') else None
        cand = set()
        for ma, clases in huella.items():
            if encaja(metodos, ma):
                cand |= clases
        if len(cand) != 1 or actual in cand:
            continue
        c = cand.pop()
        nuevo = c if c.startswith('_$_') else '_$_' + c
        filas.append((ini, nombre, nuevo, unidad, sum(1 for m in metodos if not SIN.match(m)), por_nom.get(nuevo)))
    revisados = len(vs)
    for ini, nombre, nuevo, unidad, votos, otra in sorted(filas):
        print('0x%06x %-44s -> %-40s %d metodos  %s%s' % (ini, nombre[:44], nuevo[:40], votos, unidad.split('/')[-1],
                                                          ('  (el nombre esta ahora en 0x%x)' % otra) if otra else ''))
    print('%s: %d destructores con vtable propia, %d no cuadran' % (V, revisados, len(filas)))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for ini, nombre, nuevo, unidad, votos, otra in sorted(filas):
                fh.write('0x%x\t%s\tvtable\t%s\n' % (ini, nuevo, unidad))


if __name__ == '__main__':
    main()
