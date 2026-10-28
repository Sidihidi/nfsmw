#!/usr/bin/env python3
"""egm360.py -- lo comun de las herramientas de Xbox 360 que comparan el EGM con nuestro enlace LTCG.

No es una orden: lo importan parecidas360.py, deltas360.py, iguales360.py, cad360.py, escapa360.py y cmplt360.py.
Todo se lee desde la raiz del repo:

  - el EGM: orig/EUROPEGERMILESTONE/NFS.exe (base 0x82000000) y config/EUROPEGERMILESTONE/symbols.txt;
  - nuestro enlace LTCG: build/EUROPEGERMILESTONE/ltcg/NFS_ltcg.exe y .map (los deja scripts/ltcg360.py en cada
    medida). --enlace DIR toma otro (por ejemplo la copia de un scratch: DIR/NFS_ltcg.exe y DIR/NFS_ltcg.map).
"""
import bisect
import os
import re
import struct

import capstone

EGM = os.path.join('orig', 'EUROPEGERMILESTONE', 'NFS.exe')
SYMBOLS = os.path.join('config', 'EUROPEGERMILESTONE', 'symbols.txt')
ENLACE = os.path.join('build', 'EUROPEGERMILESTONE', 'ltcg')
MD = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)


def enlace(argv):
    """El directorio del enlace LTCG (--enlace DIR en argv; lo quita de argv)."""
    if '--enlace' in argv:
        i = argv.index('--enlace')
        d = argv[i + 1]
        del argv[i:i + 2]
        return d
    return ENLACE


class Imagen:
    """Un PE de Xbox 360 en memoria: secciones con su VA y lectura por VA."""

    def __init__(self, ruta, base=None):
        self.d = open(ruta, 'rb').read()
        d = self.d
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        ns = struct.unpack_from('<H', d, pe + 6)[0]
        osz = struct.unpack_from('<H', d, pe + 20)[0]
        self.base = base if base is not None else struct.unpack_from('<I', d, pe + 24 + 28)[0]
        self.secs = []
        for i in range(ns):
            o = pe + 24 + osz + 40 * i
            nombre = d[o:o + 8].rstrip(b'\0')
            vs, va, rs, rp = struct.unpack_from('<IIII', d, o + 8)
            self.secs.append((nombre, va + self.base, max(vs, rs), rp, rs))

    def off(self, va):
        for _, a, tam, rp, rs in self.secs:
            if a <= va < a + tam:
                return va - a + rp
        return None

    def lee(self, va, n):
        o = self.off(va)
        return self.d[o:o + n] if o is not None else b''

    def ins(self, va, n):
        return list(MD.disasm(self.lee(va, n), va))

    def cadena(self, va, maximo=200):
        o = self.off(va)
        if o is None:
            return None
        e = self.d.find(b'\0', o, o + maximo)
        return self.d[o:e] if e >= 0 else None

    def texto(self):
        """(va, palabras) de la seccion .text."""
        for nombre, a, tam, rp, rs in self.secs:
            if nombre.startswith(b'.text'):
                return a, struct.unpack_from('>%dI' % (rs // 4), self.d, rp)
        return None, ()


def egm():
    return Imagen(EGM, 0x82000000)


def simbolos_egm(prefijo='.text'):
    """[(va, nombre, tam)] de symbols.txt (tam 0 si no lo dice)."""
    r = []
    for linea in open(SYMBOLS, encoding='utf-8'):
        m = re.match(r'^(\S+) = (\.\w+):0x([0-9A-F]+);(.*)$', linea)
        if m and m.group(2).startswith(prefijo):
            t = re.search(r'size:0x([0-9A-F]+)', m.group(4))
            r.append((int(m.group(3), 16), m.group(1), int(t.group(1), 16) if t else 0))
    r.sort()
    return r


def mapa(directorio):
    """[(va, nombre, objeto, tam)] de las funciones de NFS_ltcg.map (tam hasta el simbolo siguiente)."""
    filas = []
    for linea in open(os.path.join(directorio, 'NFS_ltcg.map'), encoding='latin-1'):
        c = linea.split()
        if len(c) >= 4 and re.match(r'^000[1-9]:[0-9a-f]{8}$', c[0]) and re.match(r'^[0-9a-f]{8}$', c[2]):
            filas.append((int(c[2], 16), c[1], c[-1]))
    filas.sort()
    r = []
    for i, (a, n, o) in enumerate(filas):
        r.append((a, n, o, (filas[i + 1][0] - a) if i + 1 < len(filas) else 4))
    return r


def nuestro(directorio):
    return Imagen(os.path.join(directorio, 'NFS_ltcg.exe'))


class Nombres:
    """Busqueda del simbolo que contiene una VA."""

    def __init__(self, filas):
        self.filas = sorted((a, n) for a, n, *_ in filas)
        self.ks = [a for a, _ in self.filas]

    def de(self, va):
        i = bisect.bisect_right(self.ks, va) - 1
        return self.filas[i][1] if i >= 0 else '?'
