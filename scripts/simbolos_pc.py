#!/usr/bin/env python3
"""simbolos_pc.py -- los simbolos de PC (speed.exe 1.3) como un PDB y sus primos: .map, JSON/CSV, scripts de IDA y
Ghidra, base de x64dbg y un PDB de verdad con publicos, funciones y LINEAS DE FUENTE de las funciones casadas.

    python scripts/simbolos_pc.py               # todo en build/SPEED_EXE_1_3/simbolos/
    python scripts/simbolos_pc.py --sin-pdb     # sin el PDB (no necesita el lld del NDK)
    python scripts/simbolos_pc.py --prueba      # despues, comprueba el PDB con el dbghelp.dll de Windows
    python scripts/simbolos_pc.py --solo-prueba # solo la comprobacion, sobre lo ya generado
    python scripts/simbolos_pc.py --sin-tipos   # PDB sin el CodeView (tipos, prototipos, locales) de nuestros objetos
    python scripts/simbolos_pc.py --raiz-fuente C:\\nfsmw   # rutas neutras (para publicarlo): ni la de este clon
                                                # ni la del usuario en el PDB, lineas_pc.json ni el bloque de lld
    python scripts/simbolos_pc.py --lld RUTA\\lld-link.exe  # otro enlazador (por defecto LLD_LINK, NDK o PATH)
    python scripts/simbolos_pc.py --solo-prueba --salida CARPETA --exe RUTA\\speed.exe   # probar un PDB ya hecho

Salidas: speed.map (mapa de MSVC para IDA), simbolos_pc.json/.csv, lineas_pc.json/.csv, ida_importa_simbolos_pc.py
(IDA 7.4+), ImportaSimbolosPC.java (Ghidra), speed.exe.dd32 (x64dbg) y NfsMWCollectorsRelease.pdb.

Entradas (se leen tal como esten en ese momento; el script se puede volver a pasar cuando cambien):
  config/SPEED_EXE_1_3/symbols.json            nombres y rangos de funcion (exportacion de IDA, ya renombrada)
  config/SPEED_EXE_1_3/nombres_procedencia.tsv procedencia de los nombres puestos por scripts/nombra_elim.py
  config/SPEED_EXE_1_3/splits.json             unidad (SourceList) de cada rango, para lo que el informe no trae
  build/SPEED_EXE_1_3/report.json              unidad y porcentaje de casado de cada funcion (objdiff)
  build/SPEED_EXE_1_3/src/**/*.obj             nuestros objetos de PC: ya se compilan con /Z7, y MSVC 7.1 deja las
                                               lineas como numeros de linea COFF clasicos (.bf + .file); de ahi
                                               salen el fichero fuente y la tabla de lineas de cada funcion
  orig/SPEED_EXE_1_3/speed.exe                 secciones, punto de entrada y el registro RSDS (GUID y edad)
  historia de git de symbols.json              de que ronda viene cada nombre (cache en simbolos/.historia.json)

Las lineas solo se dan para funciones al 100 % cuyo codigo, comparado byte a byte con el exe (con las reubicaciones
enmascaradas), es identico: el desplazamiento de cada linea dentro de la funcion vale tal cual en el original.

El PDB se enlaza con el lld del NDK de Android (ld.lld.exe copiado como lld-link.exe, que es el enlazador COFF de
LLVM y escribe PDB): un objeto por unidad con trozos .text$<direccion> alineados a 1 que reproducen la disposicion
exacta del exe, simbolos en sus direcciones y un .debug$S C13 con S_GPROC32 y lineas; las funciones casadas llevan
ademas los registros CodeView de nuestro objeto (prototipo, parametros, locales) y el .debug$T de su unidad, con las
reubicaciones reapuntadas a la funcion original o a un dato con nombre. Despues se le pone el GUID y la
edad del registro RSDS de speed.exe, de modo que el exe ORIGINAL, sin tocar, lo carga solo: se llama como su
registro dice, NfsMWCollectorsRelease.pdb.
"""
import argparse
import collections
import csv
import datetime
import glob
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path, PureWindowsPath

RAIZ = Path(__file__).resolve().parent.parent
VER = 'SPEED_EXE_1_3'
CFG = RAIZ / 'config' / VER
BUILD = RAIZ / 'build' / VER
EXE = RAIZ / 'orig' / VER / 'speed.exe'
UNDNAME = RAIZ / 'build' / 'compilers' / 'Win32' / '7.1' / 'undname.exe'
SIMBOLOS_JSON = 'config/SPEED_EXE_1_3/symbols.json'

# Modo ingles (--english o --lang en; scripts/make_pdb.py lo pone siempre): nombres de fichero, claves, valores,
# mensajes y scripts de IDA/Ghidra en ingles, para el kit de la comunidad. Por defecto todo sigue en espanol: el
# proyecto y sus agentes usan estos nombres.
EN = False


def L(es, en):
    """El texto en el idioma de la salida."""
    return en if EN else es


FICHEROS = {  # clave: (espanol, ingles)
    'json': ('simbolos_pc.json', 'symbols_pc.json'),
    'csv': ('simbolos_pc.csv', 'symbols_pc.csv'),
    'lineas_json': ('lineas_pc.json', 'lines_pc.json'),
    'lineas_csv': ('lineas_pc.csv', 'lines_pc.csv'),
    'ida': ('ida_importa_simbolos_pc.py', 'ida_import_symbols_pc.py'),
    'ghidra': ('ImportaSimbolosPC.java', 'ImportSymbolsPC.java'),
    'historia': ('.historia.json', '.history.json'),
    'salida': ('simbolos', 'symbols'),
    'resto': ('_resto', '_rest'),
}


def F(clave):
    return FICHEROS[clave][1 if EN else 0]


# columnas y valores de simbolos_pc.json/.csv en ingles
CAMPOS_EN = {'direccion': 'address', 'rva': 'rva', 'seccion': 'section', 'secoff': 'secoff', 'tipo': 'kind',
             'tamano': 'size', 'nombre': 'name', 'sin_decorar': 'undecorated', 'corto': 'short_name', 'real': 'real',
             'estatica': 'static', 'unidad': 'unit', 'sourcelist': 'sourcelist', 'fichero': 'file', 'linea': 'line',
             'porcentaje': 'match_percent', 'verificado': 'verified', 'con_lineas': 'has_lines',
             'procedencia': 'provenance', 'marca': 'mark', 'ronda': 'round'}
TIPO_EN = {'funcion': 'function', 'dato': 'data'}
MARCA_EN = {'seguro': 'confirmed', 'copia-ambigua': 'ambiguous-copy', 'decoracion': 'decoration'}
PROCEDENCIA_EN = {'upstream': 'upstream', 'pdb': 'partial-pdb', 'grafo': 'call-graph', 'undercover-wii': 'undercover-wii',
                  'codigo': 'by-code', 'elim 1:1': 'elimination-1to1', 'copia-ambigua': 'ambiguous-copy',
                  'ida (sin nombre)': 'unnamed', 'sin commit': 'uncommitted'}
REGLA_EN = {  # piezas de las reglas de nombres_procedencia.tsv, que van unidas con '+'
    'elim-1:1': 'elimination-1to1', 'elim-orden': 'elimination-ordered', 'destinos': 'targets',
    'llamantes': 'callers', 'nombra-llamantes': 'named-by-callers', 'corrige-codigo': 'fixed-by-code',
    'corrige-llamantes': 'fixed-by-callers', 'corrige-decoracion': 'fixed-decoration', 'mueve-codigo': 'moved-by-code',
    'quita-duplicado': 'removed-duplicate', 'quita-falso': 'removed-false', 'copia-corregida': 'copy-corrected',
    'copia-confirmada': 'copy-confirmed', 'upstream': 'upstream'}
RESUMEN_EN = {'funciones': 'functions', 'funciones con nombre': 'named functions', 'datos con nombre': 'named data',
              '100': 'at 100 %', '100 verificadas': 'at 100 % and byte-verified',
              'funciones con lineas': 'functions with lines', 'lineas': 'lines', 'ficheros fuente': 'source files'}


def procedencia_en(p):
    if p is None:
        return None
    if p in PROCEDENCIA_EN:
        return PROCEDENCIA_EN[p]
    return '+'.join(REGLA_EN.get(x, x) for x in p.split('+'))


def ronda_en(r):
    """El asunto del commit (en espanol) reducido a la ronda: 'round 39', 'window 17', 'close of round 13'..."""
    if not r:
        return None
    for rx, fmt in ((r'^Cierre de la ronda (\d+)', 'close of round %s'), (r'^Ronda (\d+)', 'round %s'),
                    (r'^Ventana (\d+)', 'window %s'), (r'^r(\d+)\b', 'r%s')):
        m = re.match(rx, r)
        if m:
            return fmt % m.group(1)
    return 'earlier commit'

# Nombres que no son del original: los que IDA inventa para lo que no conoce.
PLACEHOLDER = re.compile(
    r'^(?:(?:sub|nullsub|unknown_libname|loc|locret|off|unk|byte|word|dword|qword|tbyte|xmmword|flt|dbl|stru|asc|'
    r'jpt|def|j_sub|j_nullsub|SEH|UnwindMapEntry|FuncInfo|HandlerType|TryBlockMapEntry_ARRAY|TryBlockMapEntry|'
    r'IPToStateMapEntry|ESTypeList|algn|funcs)_[0-9A-Fa-f_]*|Catch@[0-9A-Fa-f]+|\$L\d+|entry|start|'
    # conflictos de FLIRT y thunks de IDA: no son nombres (y "[thunk]:" rompe el lector de PDB de Ghidra)
    r'(?:j_)?FID_conflict:.*|\[thunk\]:.*)$')

# Procedencia por ronda: la primera regla que case con el asunto del commit que puso el nombre actual.
REGLAS_COMMIT = [
    (re.compile(r'\bPDB\b'), 'pdb'),
    (re.compile(r'nombra_elim|reglas del enlazador'), 'elim'),
    (re.compile(r'grafo|llamantes'), 'grafo'),
    (re.compile(r'Undercover'), 'undercover-wii'),
    (re.compile(r'upstream|reconciliacion'), 'upstream'),
]


def log(*a):
    print(*a, file=sys.stderr, flush=True)


# ---------------------------------------------------------------------------------------------------------- PE y COFF
class Pe:
    def __init__(self, ruta):
        d = Path(ruta).read_bytes()
        self.d = d
        pe, = struct.unpack_from('<I', d, 0x3c)
        _, nsec, self.timestamp, _, _, optsz, _ = struct.unpack_from('<HHIIIHH', d, pe + 4)
        opt = pe + 24
        self.entrada, = struct.unpack_from('<I', d, opt + 16)
        self.base, = struct.unpack_from('<I', d, opt + 28)
        self.tam_imagen, = struct.unpack_from('<I', d, opt + 56)
        ndir, = struct.unpack_from('<I', d, opt + 92)
        dirs = [struct.unpack_from('<II', d, opt + 96 + 8 * i) for i in range(ndir)]
        self.secciones = []
        for i in range(nsec):
            o = opt + optsz + 40 * i
            nombre = d[o:o + 8].rstrip(b'\0').decode('latin-1')
            vs, va, rsz, rptr = struct.unpack_from('<IIII', d, o + 8)
            ch, = struct.unpack_from('<I', d, o + 36)
            self.secciones.append(dict(nombre=nombre, va=va, vsize=vs, rsize=rsz, rptr=rptr, ch=ch))
        self.rsds = None
        dva, dsz = dirs[6] if len(dirs) > 6 else (0, 0)
        for k in range(dsz // 28):
            o = self.rva_a_fichero(dva + 28 * k)
            _, ts, _, _, typ, size, _, ptr = struct.unpack_from('<IIHHIIII', d, o)
            if typ == 2 and d[ptr:ptr + 4] == b'RSDS':
                guid = d[ptr + 4:ptr + 20]
                edad, = struct.unpack_from('<I', d, ptr + 20)
                nombre = d[ptr + 24:ptr + size].split(b'\0')[0].decode('latin-1')
                self.rsds = dict(guid=guid, edad=edad, pdb=nombre)

    def rva_a_fichero(self, rva):
        for s in self.secciones:
            if s['va'] <= rva < s['va'] + max(s['vsize'], s['rsize']):
                return rva - s['va'] + s['rptr']
        raise ValueError(hex(rva))

    def seccion(self, va):
        """(indice desde 1, seccion) que contiene la direccion virtual va."""
        rva = va - self.base
        for i, s in enumerate(self.secciones):
            if s['va'] <= rva < s['va'] + max(s['vsize'], s['rsize']):
                return i + 1, s
        return 0, None

    def bytes_va(self, va, n):
        o = self.rva_a_fichero(va - self.base)
        return self.d[o:o + n]


def guid_texto(g):
    a, b, c = struct.unpack_from('<IHH', g, 0)
    return '{%08X-%04X-%04X-%s-%s}' % (a, b, c, g[8:10].hex().upper(), g[10:16].hex().upper())


class ObjCoff:
    """Lo que hace falta de un .obj de MSVC 7.1 compilado con /Z7: funciones con su seccion, tamano, bytes,
    reubicaciones, fichero (.file anterior) y lineas (numeros de linea COFF relativos a la .bf)."""

    def __init__(self, ruta):
        d = Path(ruta).read_bytes()
        _, nsec, _, symptr, nsym, optsz, _ = struct.unpack_from('<HHIIIHH', d, 0)
        strtab = symptr + 18 * nsym

        def cadena(o):
            return d[strtab + o:d.index(b'\0', strtab + o)].decode('latin-1')

        self.secs = []
        self.nombres_sec = []
        for i in range(nsec):
            o = 20 + optsz + 40 * i
            _, _, rsz, rptr, relptr, lnptr, nrel, nln, ch = struct.unpack_from('<IIIIIIHHI', d, o + 8)
            self.secs.append((rsz, rptr, relptr, nrel, lnptr, nln, ch))
            self.nombres_sec.append(d[o:o + 8].split(b'\0')[0].decode('latin-1'))
        self.asociadas = collections.defaultdict(list)   # seccion de codigo -> sus .debug$S asociativas
        simbolos = {}
        fichero = None
        self.funciones = collections.defaultdict(list)
        i = 0
        pendientes = []
        while i < nsym:
            o = symptr + 18 * i
            if d[o:o + 4] == b'\0\0\0\0':
                nombre = cadena(struct.unpack_from('<I', d, o + 4)[0])
            else:
                nombre = d[o:o + 8].split(b'\0')[0].decode('latin-1')
            valor, secnum, tipo, clase, naux = struct.unpack_from('<IhHBB', d, o + 8)
            aux = d[o + 18:o + 18 + 18 * naux]
            simbolos[i] = (nombre, valor, secnum, clase)
            if clase == 3 and naux == 1 and valor == 0 and secnum > 0 and nombre == self.nombres_sec[secnum - 1]:
                numero, seleccion = struct.unpack_from('<HB', aux, 12)
                if seleccion == 5 and nombre == '.debug$S':
                    self.asociadas[numero].append(secnum)
            if clase == 103:  # C_FILE
                fichero = aux.split(b'\0')[0].decode('latin-1')
            elif secnum > 0 and (tipo & 0x30) == 0x20 and clase in (2, 3):
                if naux:
                    tag, tam, lnptr, _ = struct.unpack_from('<IIII', aux, 0)
                else:   # las que genera el compilador (??_G, ??4 implicitos...) no llevan registro de funcion
                    tag, tam, lnptr = 0, self.secs[secnum - 1][0] - valor, 0
                pendientes.append(dict(nombre=nombre, sec=secnum, off=valor, tam=tam, lnptr=lnptr, tag=tag,
                                       fichero=fichero if naux else None, estatica=clase == 3))
            i += 1 + naux
        self.d = d
        self.simbolos = simbolos
        for f in pendientes:
            base = None
            bf = simbolos.get(f['tag'])
            if bf and bf[0] == '.bf':
                ob = symptr + 18 * (f['tag'] + 1)
                base, = struct.unpack_from('<H', d, ob + 4)
            lineas = []
            rsz, rptr, relptr, nrel, lnptr, nln, ch = self.secs[f['sec'] - 1]
            if f['lnptr'] and base is not None and lnptr <= f['lnptr'] < lnptr + 6 * nln:
                o = f['lnptr'] + 6
                fin = lnptr + 6 * nln
                while o < fin:
                    va, ln = struct.unpack_from('<IH', d, o)
                    if ln == 0:
                        break
                    lineas.append((va - f['off'], base + ln))
                    o += 6
            if not lineas and base is not None and f['lnptr']:
                lineas = [(0, base)]   # sin entradas: toda la funcion en la linea de su .bf
            f['lineas'] = lineas
            f['base'] = base
            self.funciones[f['nombre']].append(f)

    def codigo(self, f):
        """Bytes de la funcion y desplazamientos (relativos) que cubren las reubicaciones."""
        rsz, rptr, relptr, nrel, _, _, _ = self.secs[f['sec'] - 1]
        b = self.d[rptr + f['off']:rptr + f['off'] + f['tam']]
        mascara = set()
        for k in range(nrel):
            va, _, typ = struct.unpack_from('<IIH', self.d, relptr + 10 * k)
            if f['off'] <= va < f['off'] + f['tam']:
                for j in range(4):
                    mascara.add(va - f['off'] + j)
        return b, mascara


    def relocs(self, sec):
        rsz, rptr, relptr, nrel, _, _, _ = self.secs[sec - 1]
        return [struct.unpack_from('<IIH', self.d, relptr + 10 * k) for k in range(nrel)]

    def subsecciones(self, sec):
        """[(tipo, inicio, fin)] de un .debug$S (en MSVC 7.1 la F1 de una COMDAT lleva longitud 0: hasta el final)."""
        rsz, rptr = self.secs[sec - 1][:2]
        b = self.d[rptr:rptr + rsz]
        out = []
        o = 4
        while o + 8 <= len(b):
            k, n = struct.unpack_from('<II', b, o)
            if n == 0:
                n = len(b) - o - 8
            out.append((k, o + 8, o + 8 + n))
            o = (o + 8 + n + 3) & ~3
        return b, out

    def debug_t(self):
        for i, nombre in enumerate(self.nombres_sec):
            if nombre == '.debug$T' and not self.secs[i][6] & 0x1000:
                rsz, rptr = self.secs[i][:2]
                return self.d[rptr:rptr + rsz]
        return None

    def registros(self, sec, f_off, f_tam, externos):
        """Registros CodeView de los .debug$S de la seccion sec (o del modulo si sec es None) que se pueden llevar al
        PDB: [(bytes, [(desplazamiento, tipo, ('func', delta) | ('ext', nombre))])]. Una reubicacion que apunte dentro
        de la funcion se reescribe contra el simbolo de la funcion; una a un dato con nombre publico, contra ese
        nombre; si no se puede, se descarta el registro con todo su ambito."""
        if sec is None:
            secciones = [i + 1 for i, n in enumerate(self.nombres_sec) if n == '.debug$S' and
                         not self.secs[i][6] & 0x1000]
        else:
            secciones = self.asociadas.get(sec, [])
        out = []
        for ds in secciones:
            b, subs = self.subsecciones(ds)
            rel = sorted(self.relocs(ds))
            for k, ini, fin in subs:
                if k != 0xF1:
                    continue
                p = ini
                saltar = 0
                while p + 4 <= fin:
                    rl, rk = struct.unpack_from('<HH', b, p)
                    q = p + 2 + rl
                    if saltar:
                        if rk in ABRE:
                            saltar += 1
                        elif rk == S_END:
                            saltar -= 1
                        p = q
                        continue
                    if sec is None and rk not in (0x1108, 0x1107, 0x110D, 0x110C, 0x1116):
                        p = q   # en el modulo: tipos (S_UDT, S_CONSTANT), datos y S_COMPILE2
                        continue
                    nuevas = []
                    vale = True
                    for va, si, t in rel:
                        if not p <= va < q:
                            continue
                        nombre, valor, secnum, clase = self.simbolos.get(si, ('', 0, 0, 0))
                        if sec is not None and secnum == sec and f_off <= valor < f_off + f_tam and t in (10, 11):
                            nuevas.append((va - p, t, ('func', valor - f_off)))
                        elif nombre in externos and t in (10, 11):
                            nuevas.append((va - p, t, ('ext', nombre)))
                        else:
                            vale = False
                    if not vale:
                        if rk in ABRE:
                            saltar = 1
                        p = q
                        continue
                    out.append((b[p:q], nuevas))
                    p = q
        return out


class CoffW:
    """Escritor minimo de objetos COFF i386."""

    def __init__(self):
        self.secs = []
        self.syms = []
        self.strtab = bytearray()

    def _nombre(self, nombre, largo):
        b = nombre.encode('latin-1', 'replace')
        if len(b) <= 8:
            return b.ljust(8, b'\0')
        off = 4 + len(self.strtab)
        self.strtab += b + b'\0'
        return (b'/%d' % off).ljust(8, b'\0') if not largo else struct.pack('<II', 0, off)

    def seccion(self, nombre, datos, ch):
        self.secs.append(dict(nombre=self._nombre(nombre, False), datos=bytes(datos), ch=ch, relocs=[]))
        idx = len(self.secs)
        # simbolo de seccion (estatico) para que lld la conozca por nombre
        return idx

    def simbolo(self, nombre, valor, secnum, funcion, externo):
        self.syms.append(struct.pack('<8sIhHBB', self._nombre(nombre, True), valor, secnum, 0x20 if funcion else 0,
                                     2 if externo else 3, 0))
        return len(self.syms) - 1

    def reloc(self, secidx, off, symidx, tipo):
        self.secs[secidx - 1]['relocs'].append(struct.pack('<IIH', off, symidx, tipo))

    def bytes(self):
        n = len(self.secs)
        off = 20 + 40 * n
        cab = bytearray()
        cuerpo = bytearray()
        for s in self.secs:
            rptr = off + len(cuerpo) if s['datos'] else 0
            cuerpo += s['datos']
            relptr = off + len(cuerpo) if s['relocs'] else 0
            for r in s['relocs']:
                cuerpo += r
            if len(s['relocs']) > 0xFFFF:
                raise ValueError('demasiadas reubicaciones en una seccion')
            cab += s['nombre'] + struct.pack('<IIIIIIHHI', 0, 0, len(s['datos']), rptr, relptr, 0,
                                             len(s['relocs']), 0, s['ch'])
        symptr = off + len(cuerpo)
        cabecera = struct.pack('<HHIIIHH', 0x14C, n, 0, symptr, len(self.syms), 0, 0)
        return bytes(cabecera + cab + cuerpo + b''.join(self.syms) + struct.pack('<I', 4 + len(self.strtab)) +
                     self.strtab)


# -------------------------------------------------------------------------------------------------------- entradas
def leer_procedencia_tsv():
    out = {}
    ruta = CFG / 'nombres_procedencia.tsv'
    if not ruta.exists():
        return out
    for linea in ruta.read_text(encoding='utf-8').splitlines():
        if not linea or linea.startswith('#'):
            continue
        c = linea.split('\t')
        if len(c) < 5 or not c[0].isdigit():
            continue
        out[int(c[0])] = dict(nombre=c[2], regla=c[3], marca=c[4])
    return out


def nombres_de(simbolos):
    m = {}
    for n in simbolos.get('names', []):
        m.setdefault(n['addr'], n['name'])
    for f in simbolos['functions']:
        m[f['start']] = f['name']
    return m


def historia(cache_ruta):
    """{direccion: (nombre, asunto del commit que lo puso)} siguiendo la historia de git de symbols.json."""
    try:
        salida = subprocess.run(['git', 'log', '--reverse', '--format=%H\t%s', '--', SIMBOLOS_JSON], cwd=RAIZ,
                                capture_output=True, check=True).stdout.decode('utf-8', 'replace')
    except (OSError, subprocess.CalledProcessError) as e:
        log(L('historia: sin git (%s)', 'history: no git (%s)') % e)
        return {}, []
    commits = [tuple(l.split('\t', 1)) for l in salida.splitlines() if '\t' in l]
    cache = {}
    if cache_ruta.exists():
        try:
            cache = json.loads(cache_ruta.read_text(encoding='utf-8'))
        except ValueError:
            cache = {}
    hechos = cache.get('commits', [])
    origen = {int(k): tuple(v) for k, v in cache.get('origen', {}).items()}
    if [c[0] for c in commits[:len(hechos)]] != hechos:
        hechos, origen = [], {}
    for h, asunto in commits[len(hechos):]:
        log(L('historia: %s %s' % (h[:9], asunto[:70]), 'history: commit %s' % h[:9]))
        datos = subprocess.run(['git', 'show', '%s:%s' % (h, SIMBOLOS_JSON)], cwd=RAIZ, capture_output=True,
                               check=True).stdout
        for a, n in nombres_de(json.loads(datos)).items():
            if a not in origen or origen[a][0] != n:
                origen[a] = (n, len(hechos))
        hechos.append(h)
    cache_ruta.parent.mkdir(parents=True, exist_ok=True)
    cache_ruta.write_text(json.dumps(dict(commits=hechos, origen={str(k): v for k, v in origen.items()})),
                          encoding='utf-8')
    asuntos = {h: s for h, s in commits}
    return {a: (n, asuntos[hechos[i]]) for a, (n, i) in origen.items()}, commits


def categoria_commit(asunto):
    for rx, cat in REGLAS_COMMIT:
        if rx.search(asunto):
            return cat
    return 'codigo'


def leer_informe():
    """{va: (unidad, sourcelist, porcentaje)} y la lista de unidades con su fuente."""
    r = json.loads((BUILD / 'report.json').read_text(encoding='utf-8'))
    por_va = {}
    unidades = {}
    for u in r['units']:
        nombre = u['name'].split('/', 1)[1] if '/' in u['name'] else u['name']
        fuente = (u.get('metadata') or {}).get('source_path')
        unidades[nombre] = fuente
        for f in u.get('functions', []):
            va = int(f['metadata']['virtual_address'])
            por_va[va] = (nombre, fuente, float(f.get('fuzzy_match_percent', 0.0)), f['name'], int(f['size']))
    return por_va, unidades


def leer_splits():
    """[(inicio, fin, unidad, tipo)] de splits.json."""
    sp = json.loads((CFG / 'splits.json').read_text(encoding='utf-8'))
    rangos = []
    for obj, v in sp.items():
        unidad = obj[:-4] if obj.endswith('.obj') else obj
        for clave in ('function_ranges', 'rdata', 'data', 'bss'):
            for a, b in v.get(clave) or []:
                rangos.append((a, b, unidad, clave))
    rangos.sort()
    return rangos


def unidad_de(rangos, va):
    import bisect
    i = bisect.bisect_right(rangos, (va, 0xFFFFFFFF, '', '')) - 1
    for j in range(i, max(-1, i - 8), -1):
        a, b, u, _ = rangos[j]
        if a <= va < b:
            return u
    return None


def recasar_rutas():
    """minusculas -> ruta real relativa a la raiz (MSVC escribe las rutas de .file en minusculas)."""
    m = {}
    for base in ('src', 'build/compilers/Win32/7.1/Include'):
        for dirpath, _, ficheros in os.walk(RAIZ / base):
            for f in ficheros:
                rel = os.path.relpath(os.path.join(dirpath, f), RAIZ).replace('\\', '/')
                m[rel.lower()] = rel
    return m


def ruta_relativa(abs_minus, recase):
    raiz = str(RAIZ).replace('\\', '/').lower().rstrip('/') + '/'
    p = abs_minus.replace('\\', '/')
    if p.lower().startswith(raiz):
        rel = p[len(raiz):]
        return recase.get(rel.lower(), rel)
    return p


def desdecorar(nombres, tmpdir):
    """{decorado: (completo, solo_nombre)} con el undname de MSVC 7.1 (modo fichero, una linea por nombre)."""
    dec = sorted({n for n in nombres if n.startswith('?')})
    out = {}
    if not dec or not UNDNAME.exists():
        return out
    ent = Path(tmpdir) / 'undname_in.txt'
    ent.write_bytes(('\n'.join(dec) + '\n').encode('latin-1'))
    res = []
    for flags in ([], ['0x1000']):
        o = subprocess.run([str(UNDNAME)] + flags + [str(ent)], capture_output=True).stdout.decode('latin-1')
        lineas = o.replace('\r\n', '\n').split('\n')
        while lineas and lineas[-1] == '':
            lineas.pop()
        res.append(lineas)
    if len(res[0]) != len(dec) or len(res[1]) != len(dec):
        log(L('undname: %d/%d lineas para %d nombres, se deja sin desdecorar',
              'undname: %d/%d lines for %d names, names left decorated') % (len(res[0]), len(res[1]), len(dec)))
        return out
    for n, a, b in zip(dec, res[0], res[1]):
        out[n] = (a.strip(), b.strip())
    return out


def es_cadena_ida(pe, nombre, va):
    """Los aXxx de IDA: el nombre lo inventa IDA a partir de una cadena literal."""
    if not re.match(r'^a[A-Z0-9_?]', nombre):
        return False
    try:
        b = pe.bytes_va(va, 64)
    except ValueError:
        return False
    fin = b.find(b'\0')
    return fin >= 1 and all(32 <= c < 127 or c in (9, 10, 13) for c in b[:fin])


# --------------------------------------------------------------------------------------------------- modelo
def construir(args, tmpdir):
    pe = Pe(EXE)
    for intento in range(5):   # otro proceso puede estar reescribiendo symbols.json
        try:
            simbolos = json.loads((CFG / 'symbols.json').read_text(encoding='utf-8'))
            break
        except ValueError:
            if intento == 4:
                raise
            import time
            time.sleep(3)
    tsv = leer_procedencia_tsv()
    por_va, unidades = leer_informe()
    rangos = leer_splits()
    hist, commits = ({}, []) if args.sin_historia else historia(args.salida / F('historia'))
    recase = recasar_rutas()

    # nuestros objetos, una vez por unidad
    objs = {}
    for unidad, fuente in unidades.items():
        if not fuente:
            continue
        ruta = BUILD / 'src' / Path(fuente[4:]).with_suffix('.obj') if fuente.startswith('src/') else None
        if ruta and ruta.exists():
            try:
                objs[unidad] = ObjCoff(ruta)
            except (struct.error, ValueError, IndexError) as e:
                log(L('obj ilegible %s: %s', 'unreadable object %s: %s') % (ruta, e))
    log(L('objetos de PC leidos: %d', 'PC objects read: %d') % len(objs))

    funciones = sorted(simbolos['functions'], key=lambda f: f['start'])
    inicios = {f['start'] for f in funciones}
    lista = []
    lineas = []
    stats = collections.Counter()
    for f in funciones:
        va, nombre = f['start'], f['name']
        tam = f['end'] - f['start']
        sec, s = pe.seccion(va)
        inf = por_va.get(va)
        unidad = inf[0] if inf else unidad_de(rangos, va)
        fuente = inf[1] if inf else None
        pct = inf[2] if inf else None
        e = dict(direccion=va, tipo='funcion', nombre=nombre, tamano=tam, seccion=s['nombre'] if s else None,
                 secnum=sec, secoff=va - pe.base - s['va'] if s else None, unidad=unidad, sourcelist=fuente,
                 porcentaje=pct, real=not PLACEHOLDER.match(nombre), fichero=None, linea=None, verificado=False,
                 estatica=bool(f.get('static')))
        # fichero y lineas desde nuestro objeto
        ob = objs.get(unidad)
        if ob is not None and inf:
            clave = re.sub(r'__delink_[0-9A-Fa-f]{8}$', '', inf[3])
            candidatas = ob.funciones.get(clave, [])
            elegida = None
            for c in candidatas:
                if c['tam'] != tam:
                    continue
                b, mascara = ob.codigo(c)
                orig = pe.bytes_va(va, tam)
                if len(b) == tam and all(b[j] == orig[j] for j in range(tam) if j not in mascara):
                    elegida = c
                    e['verificado'] = True
                    break
            if elegida is None and candidatas:
                elegida = candidatas[0]
            if elegida is not None and elegida['fichero']:
                e['fichero'] = ruta_relativa(elegida['fichero'], recase)
                if args.raiz_fuente is not None and (':' in e['fichero'] or e['fichero'].startswith(('/', '\\'))):
                    # fuera del clon (no deberia pasar): sin la ruta de esta maquina
                    e['fichero'] = 'external/' + PureWindowsPath(e['fichero'].replace('/', '\\')).name
                if elegida['lineas']:
                    e['linea'] = elegida['lineas'][0][1]
                elif elegida['base']:
                    e['linea'] = elegida['base']
                if e['verificado'] and pct == 100.0:
                    e['_ob'], e['_cand'] = ob, elegida   # para llevar su CodeView (tipos y locales) al PDB
                if e['verificado'] and pct == 100.0 and elegida['lineas']:
                    lineas.append(dict(direccion=va, tamano=tam, fichero=e['fichero'],
                                       absoluto=elegida['fichero'], lineas=elegida['lineas'], nombre=nombre,
                                       unidad=unidad))
        if pct == 100.0:
            stats['100'] += 1
            stats['100 verificadas'] += e['verificado']
        lista.append(e)

    # datos con nombre (no las funciones, que ya estan)
    datos = sorted((n for n in simbolos.get('names', []) if not n.get('is_func') and n['addr'] not in inicios),
                   key=lambda n: n['addr'])
    vistos = set()
    for n in datos:
        va = n['addr']
        if va in vistos:
            continue
        vistos.add(va)
        sec, s = pe.seccion(va)
        real = not PLACEHOLDER.match(n['name']) and not es_cadena_ida(pe, n['name'], va)
        lista.append(dict(direccion=va, tipo='dato', nombre=n['name'], tamano=None, seccion=s['nombre'] if s else None,
                          secnum=sec, secoff=va - pe.base - s['va'] if s else None, unidad=unidad_de(rangos, va),
                          sourcelist=None, porcentaje=None, real=real, fichero=None, linea=None, verificado=False,
                          estatica=False))
    lista.sort(key=lambda e: (e['direccion'], e['tipo'] != 'funcion'))

    # tamano de los datos: hasta el siguiente simbolo de la misma seccion
    for i, e in enumerate(lista):
        if e['tipo'] == 'dato' and e['secnum']:
            sig = next((x['direccion'] for x in lista[i + 1:i + 50] if x['direccion'] > e['direccion']), None)
            s = pe.secciones[e['secnum'] - 1]
            fin = pe.base + s['va'] + max(s['vsize'], s['rsize'])
            e['tamano'] = min(sig or fin, fin) - e['direccion']

    # procedencia
    for e in lista:
        t = tsv.get(e['direccion'])
        if t and t['nombre'] == e['nombre']:
            e['procedencia'] = {'elim-1:1': 'elim 1:1', 'elim-orden': 'copia-ambigua'}.get(t['regla'], t['regla'])
            e['marca'] = t['marca']
            continue
        e['marca'] = None
        if not e['real']:
            e['procedencia'] = 'ida (sin nombre)'
            continue
        h = hist.get(e['direccion'])
        if h and h[0] == e['nombre']:
            e['procedencia'] = categoria_commit(h[1])
            e['ronda'] = h[1].split(':')[0][:80]
        elif hist:
            e['procedencia'] = 'sin commit'
        else:
            e['procedencia'] = None

    # nombres desdecorados
    und = desdecorar([e['nombre'] for e in lista], tmpdir)
    for e in lista:
        c, s = und.get(e['nombre'], (e['nombre'], e['nombre']))
        e['sin_decorar'] = c
        e['corto'] = s
    stats['funciones'] = len(funciones)
    stats['funciones con nombre'] = sum(1 for e in lista if e['tipo'] == 'funcion' and e['real'])
    stats['datos con nombre'] = sum(1 for e in lista if e['tipo'] == 'dato' and e['real'])
    stats['funciones con lineas'] = len(lineas)
    stats['lineas'] = sum(len(x['lineas']) for x in lineas)
    stats['ficheros fuente'] = len({x['fichero'] for x in lineas})
    return pe, lista, lineas, stats, commits


# --------------------------------------------------------------------------------------------------- salidas
def escribir_map(pe, lista, ruta):
    secs = [s for s in pe.secciones if s['nombre']]
    o = [' speed', '',
         ' Timestamp is %08x (%s)' % (pe.timestamp, datetime.datetime.fromtimestamp(pe.timestamp, datetime.timezone.utc)
                                      .strftime('%a %b %d %H:%M:%S %Y')), '',
         ' Preferred load address is %08x' % pe.base, '',
         ' Start         Length     Name                   Class']
    for i, s in enumerate(pe.secciones):
        if not s['nombre']:
            continue
        clase = 'CODE' if s['ch'] & 0x20 else 'DATA'
        o.append(' %04x:%08x %08xH %-23s %s' % (i + 1, 0, s['vsize'], s['nombre'], clase))
    o += ['', '  Address         Publics by Value              Rva+Base       Lib:Object', '']
    for e in lista:
        if not e['real'] or not e['secnum'] or e['estatica']:
            continue
        o.append(' %04x:%08x       %-26s %08x %s %s' % (
            e['secnum'], e['secoff'], e['nombre'], e['direccion'], 'f' if e['tipo'] == 'funcion' else ' ',
            (e['unidad'] or 'speed').split('/')[-1] + '.obj'))
    sec, s = pe.seccion(pe.base + pe.entrada)
    o += ['', ' entry point at        %04x:%08x' % (sec, pe.entrada - s['va']), '', ' Static symbols', '']
    for e in lista:
        if e['real'] and e['secnum'] and e['estatica']:
            o.append(' %04x:%08x       %-26s %08x %s %s' % (
                e['secnum'], e['secoff'], e['nombre'], e['direccion'], 'f' if e['tipo'] == 'funcion' else ' ',
                (e['unidad'] or 'speed').split('/')[-1] + '.obj'))
    ruta.write_text('\r\n'.join(o) + '\r\n', encoding='latin-1')


CAMPOS = ['direccion', 'rva', 'seccion', 'secoff', 'tipo', 'tamano', 'nombre', 'sin_decorar', 'corto', 'real',
          'estatica', 'unidad', 'sourcelist', 'fichero', 'linea', 'porcentaje', 'verificado', 'con_lineas',
          'procedencia', 'marca', 'ronda']


def fila(pe, e, con_lineas):
    return dict(direccion='0x%08X' % e['direccion'], rva='0x%X' % (e['direccion'] - pe.base),
                seccion=e['seccion'], secoff=('%04X:%08X' % (e['secnum'], e['secoff'])) if e['secnum'] else None,
                tipo=e['tipo'], tamano=e['tamano'], nombre=e['nombre'], sin_decorar=e['sin_decorar'],
                corto=e['corto'], real=e['real'], estatica=e['estatica'], unidad=e['unidad'],
                sourcelist=e['sourcelist'],
                fichero=e['fichero'], linea=e['linea'], porcentaje=e['porcentaje'], verificado=e['verificado'],
                con_lineas=e['direccion'] in con_lineas, procedencia=e.get('procedencia'), marca=e.get('marca'),
                ronda=e.get('ronda'))


def fila_en(r):
    """Una fila de simbolos_pc con columnas y valores en ingles."""
    r = dict(r)
    r['tipo'] = TIPO_EN.get(r['tipo'], r['tipo'])
    r['procedencia'] = procedencia_en(r['procedencia'])
    r['marca'] = MARCA_EN.get(r['marca'], r['marca'])
    r['ronda'] = ronda_en(r['ronda'])
    return {CAMPOS_EN[k]: v for k, v in r.items()}


def escribir_json_csv(pe, lista, lineas, stats, carpeta, raiz_fuente=None):
    con_lineas = {x['direccion'] for x in lineas}
    filas = [fila(pe, e, con_lineas) for e in lista]
    if EN:
        filas = [fila_en(r) for r in filas]
        cab = dict(exe='orig/SPEED_EXE_1_3/speed.exe', base='0x%08X' % pe.base, timestamp='0x%08X' % pe.timestamp,
                   entry_point='0x%08X' % (pe.base + pe.entrada),
                   rsds=dict(guid=guid_texto(pe.rsds['guid']), age=pe.rsds['edad'], pdb=pe.rsds['pdb'])
                   if pe.rsds else None,
                   generated=datetime.datetime.now().strftime('%Y-%m-%d %H:%M'),
                   sections=[dict(name=s['nombre'], va='0x%08X' % (pe.base + s['va']), size=s['vsize'])
                             for s in pe.secciones], summary={RESUMEN_EN.get(k, k): v for k, v in stats.items()})
        todo = dict(header=cab, symbols=filas)
        campos = [CAMPOS_EN[c] for c in CAMPOS]
    else:
        cab = dict(exe='orig/SPEED_EXE_1_3/speed.exe', base='0x%08X' % pe.base, timestamp='0x%08X' % pe.timestamp,
                   entrada='0x%08X' % (pe.base + pe.entrada),
                   rsds=dict(guid=guid_texto(pe.rsds['guid']), edad=pe.rsds['edad'], pdb=pe.rsds['pdb'])
                   if pe.rsds else None,
                   generado=datetime.datetime.now().strftime('%Y-%m-%d %H:%M'),
                   secciones=[dict(nombre=s['nombre'], va='0x%08X' % (pe.base + s['va']), tamano=s['vsize'])
                              for s in pe.secciones], resumen=dict(stats))
        todo = dict(cabecera=cab, simbolos=filas)
        campos = CAMPOS
    (carpeta / F('json')).write_text(json.dumps(todo, ensure_ascii=False, indent=0), encoding='utf-8')
    with open(carpeta / F('csv'), 'w', newline='', encoding='utf-8') as fh:
        w = csv.DictWriter(fh, fieldnames=campos)
        w.writeheader()
        for r in filas:
            w.writerow(r)
    ficheros = sorted({x['fichero'] for x in lineas})
    idx = {f: i for i, f in enumerate(ficheros)}
    raiz = str(RAIZ) if raiz_fuente is None else raiz_fuente
    if EN:
        lj = dict(root=raiz, files=ficheros,
                  functions=[dict(address='0x%08X' % x['direccion'], size=x['tamano'], file=idx[x['fichero']],
                                  lines=[[o, l] for o, l in x['lineas']]) for x in lineas])
    else:
        lj = dict(raiz=raiz, ficheros=ficheros,
                  funciones=[dict(direccion='0x%08X' % x['direccion'], tamano=x['tamano'], fichero=idx[x['fichero']],
                                  lineas=[[o, l] for o, l in x['lineas']]) for x in lineas])
    (carpeta / F('lineas_json')).write_text(json.dumps(lj, separators=(',', ':')), encoding='utf-8')
    with open(carpeta / F('lineas_csv'), 'w', newline='', encoding='utf-8') as fh:
        w = csv.writer(fh)
        w.writerow(['address', 'file', 'line', 'function'] if EN else ['direccion', 'fichero', 'linea', 'funcion'])
        for x in lineas:
            for o, l in x['lineas']:
                w.writerow(['0x%08X' % (x['direccion'] + o), x['fichero'], l, x['nombre']])


def comentario(e):
    partes = []
    if e['unidad']:
        partes.append(e['unidad'].split('/')[-1])
    if e['fichero']:
        partes.append('%s:%s' % (e['fichero'], e['linea']) if e['linea'] else e['fichero'])
    if e['porcentaje'] is not None:
        partes.append('%.4g %%' % e['porcentaje'])
    if e.get('procedencia'):
        partes.append(('name: %s' % procedencia_en(e['procedencia'])) if EN else ('nombre: %s' % e['procedencia']))
    return ' | '.join(partes)


def escribir_x64dbg(pe, lista, ruta):
    labels, comments, functions = [], [], []
    for e in lista:
        if not e['secnum']:
            continue
        rva = '0x%X' % (e['direccion'] - pe.base)
        if e['real']:
            labels.append(dict(module='speed.exe', address=rva, manual=True,
                               text=e['corto'] if e['nombre'].startswith('?') else e['nombre']))
        if e['tipo'] == 'funcion':
            if e['real'] or e['fichero']:
                c = comentario(e)
                if c:
                    comments.append(dict(module='speed.exe', address=rva, manual=True, text=c))
            if e['tamano'] and e['tamano'] > 0:
                functions.append(dict(module='speed.exe', start=rva, end='0x%X' % (e['direccion'] - pe.base +
                                                                                   e['tamano'] - 1),
                                      manual=True, icount=0))
    ruta.write_text(json.dumps(dict(labels=labels, comments=comments, functions=functions), indent=1),
                    encoding='utf-8')


IDA_SCRIPT = r'''# -*- coding: utf-8 -*-
"""Nombres, comentarios y lineas de fuente de NFS Most Wanted PC (speed.exe 1.3) para IDA 7.4+ (Python 3).
Generado por scripts/simbolos_pc.py. File > Script file... con simbolos_pc.json y lineas_pc.json al lado.
Pone el nombre decorado de MSVC (IDA lo muestra desdecorado), crea las funciones que falten, un comentario de
funcion con unidad, fichero:linea y % de casado, y las lineas de fuente de las funciones casadas al 100 %
(Options > General > Line prefixes / 'Source line numbers' para verlas)."""
import json
import os

import ida_bytes
import ida_funcs
import ida_kernwin
import ida_lines
import ida_name
import ida_nalt
import idc

try:
    _DIR = os.path.dirname(os.path.abspath(__file__))
except NameError:
    _DIR = ''
_RUTA = os.path.join(_DIR, 'simbolos_pc.json')
if not os.path.exists(_RUTA):
    _RUTA = ida_kernwin.ask_file(0, 'simbolos_pc.json', 'simbolos_pc.json de scripts/simbolos_pc.py')
_DIR = os.path.dirname(_RUTA)
_FUENTES = os.environ.get('NFSMW_SRC') or None   # raiz del repo si no es la de la generacion


def principal():
    datos = json.load(open(_RUTA, encoding='utf-8'))
    n = c = f = 0
    for s in datos['simbolos']:
        ea = int(s['direccion'], 16)
        if s['tipo'] == 'funcion' and s['tamano'] and (s['real'] or s['fichero']):
            if not ida_funcs.get_func(ea):
                if ida_funcs.add_func(ea, ea + s['tamano']):
                    f += 1
        if s['real']:
            if ida_name.set_name(ea, s['nombre'], ida_name.SN_NOWARN | ida_name.SN_NOCHECK | ida_name.SN_FORCE):
                n += 1
        if s['tipo'] == 'funcion' and (s['real'] or s['fichero']):
            partes = [p for p in (s['unidad'], (s['fichero'] + (':%s' % s['linea'] if s['linea'] else ''))
                                  if s['fichero'] else None,
                                  ('%.4g %%' % s['porcentaje']) if s['porcentaje'] is not None else None,
                                  ('nombre: %s' % s['procedencia']) if s['procedencia'] else None) if p]
            if partes and ida_funcs.get_func(ea):
                idc.set_func_cmt(ea, ' | '.join(partes), 0)
                c += 1
    ln = 0
    rl = os.path.join(_DIR, 'lineas_pc.json')
    if os.path.exists(rl):
        lin = json.load(open(rl, encoding='utf-8'))
        raiz = _FUENTES or lin['raiz']
        for fn in lin['funciones']:
            ea = int(fn['direccion'], 16)
            ida_lines.add_sourcefile(ea, ea + fn['tamano'], os.path.join(raiz, lin['ficheros'][fn['fichero']]))
            for off, linea in fn['lineas']:
                ida_nalt.set_source_linnum(ea + off, linea)
                ln += 1
    print('simbolos_pc: %d nombres, %d funciones nuevas, %d comentarios, %d lineas' % (n, f, c, ln))


principal()
'''

GHIDRA_SCRIPT = r'''// Nombres, comentarios y lineas de fuente de NFS Most Wanted PC (speed.exe 1.3) para Ghidra (Java: no necesita
// Jython ni PyGhidra). Generado por scripts/simbolos_pc.py; lee simbolos_pc.json y lineas_pc.json de la carpeta que
// se le pase como argumento o, si no, de la carpeta del propio script.
//   GUI: Script Manager > Script Directories > anadir esta carpeta > ImportaSimbolosPC.java
//   headless: analyzeHeadless PROY NOMBRE -process speed.exe -scriptPath CARPETA -postScript ImportaSimbolosPC.java CARPETA
// Pone el nombre decorado (el analizador "Demangler Microsoft" lo desdecora), crea las funciones que falten, un
// comentario de cabecera con unidad, fichero:linea, % de casado y procedencia del nombre, y las lineas de fuente de las
// funciones casadas en el SourceFileManager (Ghidra 11.3+; Window > Source Files and Transforms).
// @category NFSMW
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import ghidra.app.script.GhidraScript;
import ghidra.program.database.sourcemap.SourceFile;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.sourcemap.SourceFileManager;
import ghidra.program.model.symbol.SourceType;

public class ImportaSimbolosPC extends GhidraScript {

    private static JsonObject leer(File f) throws Exception {
        try (Reader r = new InputStreamReader(new FileInputStream(f), StandardCharsets.UTF_8)) {
            return JsonParser.parseReader(r).getAsJsonObject();
        }
    }

    private static String cadena(JsonObject o, String k) {
        JsonElement e = o.get(k);
        return (e == null || e.isJsonNull()) ? null : e.getAsString();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File dir = args.length > 0 ? new File(args[0]) : getSourceFile().getParentFile().getFile(false);
        JsonObject datos = leer(new File(dir, "simbolos_pc.json"));
        int nombres = 0, funciones = 0, comentarios = 0, fallos = 0;
        for (JsonElement el : datos.getAsJsonArray("simbolos")) {
            JsonObject s = el.getAsJsonObject();
            boolean real = s.get("real").getAsBoolean();
            String fichero = cadena(s, "fichero");
            if (!real && fichero == null) {
                continue;
            }
            Address a = toAddr(Long.decode(cadena(s, "direccion")));
            boolean esFuncion = "funcion".equals(cadena(s, "tipo"));
            if (esFuncion && getFunctionAt(a) == null) {
                disassemble(a);
                if (createFunction(a, null) != null) {
                    funciones++;
                }
            }
            if (real) {
                try {
                    createLabel(a, cadena(s, "nombre"), true, SourceType.IMPORTED);
                    nombres++;
                }
                catch (Exception e) {
                    fallos++;
                }
            }
            if (esFuncion) {
                List<String> partes = new ArrayList<>();
                if (cadena(s, "unidad") != null) {
                    partes.add(cadena(s, "unidad"));
                }
                if (fichero != null) {
                    String linea = cadena(s, "linea");
                    partes.add(linea != null ? fichero + ":" + linea : fichero);
                }
                if (cadena(s, "porcentaje") != null) {
                    partes.add(cadena(s, "porcentaje") + " %");
                }
                if (cadena(s, "procedencia") != null) {
                    partes.add("nombre: " + cadena(s, "procedencia"));
                }
                if (!partes.isEmpty()) {
                    setPlateComment(a, String.join(" | ", partes));
                    comentarios++;
                }
            }
        }
        int lineas = 0, rechazadas = 0;
        File fl = new File(dir, "lineas_pc.json");
        if (fl.exists()) {
            SourceFileManager sfm = currentProgram.getSourceFileManager();
            JsonObject lin = leer(fl);
            String raiz = lin.get("raiz").getAsString().replace('\\', '/');
            if (args.length > 1) {
                raiz = args[1].replace('\\', '/');   // otra copia del repositorio
            }
            List<SourceFile> ficheros = new ArrayList<>();
            for (JsonElement f : lin.getAsJsonArray("ficheros")) {
                String p = raiz + "/" + f.getAsString();
                if (!p.startsWith("/")) {
                    p = "/" + p;
                }
                SourceFile sf = new SourceFile(p);
                sfm.addSourceFile(sf);
                ficheros.add(sf);
            }
            for (JsonElement el : lin.getAsJsonArray("funciones")) {
                JsonObject fn = el.getAsJsonObject();
                long base = Long.decode(fn.get("direccion").getAsString());
                long tam = fn.get("tamano").getAsLong();
                SourceFile sf = ficheros.get(fn.get("fichero").getAsInt());
                JsonArray ls = fn.getAsJsonArray("lineas");
                for (int i = 0; i < ls.size(); i++) {
                    JsonArray par = ls.get(i).getAsJsonArray();
                    long off = par.get(0).getAsLong();
                    long fin = i + 1 < ls.size() ? ls.get(i + 1).getAsJsonArray().get(0).getAsLong() : tam;
                    if (fin <= off) {
                        continue;
                    }
                    try {
                        sfm.addSourceMapEntry(sf, par.get(1).getAsInt(), toAddr(base + off), fin - off);
                        lineas++;
                    }
                    catch (Exception e) {
                        rechazadas++;
                    }
                }
            }
        }
        println(String.format("simbolos_pc: %d nombres (%d rechazados), %d funciones nuevas, %d comentarios, " +
            "%d lineas (%d rechazadas)", nombres, fallos, funciones, comentarios, lineas, rechazadas));
    }
}
'''


IDA_SCRIPT_EN = r'''# -*- coding: utf-8 -*-
"""Names, comments and source lines of NFS Most Wanted PC (speed.exe 1.3) for IDA 7.4+ (Python 3).
Generated by scripts/simbolos_pc.py (English mode). File > Script file... with symbols_pc.json and lines_pc.json next to
this script. It sets the decorated MSVC names (IDA shows them undecorated), creates the missing functions, adds a
function comment with unit, file:line, match % and name provenance, and adds the source lines of the functions that
match at 100 % (Options > General > Line prefixes / 'Source line numbers' to see them).
Set the environment variable NFSMW_SRC to the root of your clone if it is not the root stored in lines_pc.json."""
import json
import os

import ida_funcs
import ida_kernwin
import ida_lines
import ida_name
import ida_nalt
import idc

try:
    _DIR = os.path.dirname(os.path.abspath(__file__))
except NameError:
    _DIR = ''
_PATH = os.path.join(_DIR, 'symbols_pc.json')
if not os.path.exists(_PATH):
    _PATH = ida_kernwin.ask_file(0, 'symbols_pc.json', 'symbols_pc.json from scripts/simbolos_pc.py')
_DIR = os.path.dirname(_PATH)
_SOURCES = os.environ.get('NFSMW_SRC') or None   # root of the repository, if not the one stored at generation


def main():
    data = json.load(open(_PATH, encoding='utf-8'))
    n = c = f = 0
    for s in data['symbols']:
        ea = int(s['address'], 16)
        if s['kind'] == 'function' and s['size'] and (s['real'] or s['file']):
            if not ida_funcs.get_func(ea):
                if ida_funcs.add_func(ea, ea + s['size']):
                    f += 1
        if s['real']:
            if ida_name.set_name(ea, s['name'], ida_name.SN_NOWARN | ida_name.SN_NOCHECK | ida_name.SN_FORCE):
                n += 1
        if s['kind'] == 'function' and (s['real'] or s['file']):
            parts = [p for p in (s['unit'], (s['file'] + (':%s' % s['line'] if s['line'] else ''))
                                 if s['file'] else None,
                                 ('%.4g %%' % s['match_percent']) if s['match_percent'] is not None else None,
                                 ('name: %s' % s['provenance']) if s['provenance'] else None) if p]
            if parts and ida_funcs.get_func(ea):
                idc.set_func_cmt(ea, ' | '.join(parts), 0)
                c += 1
    ln = 0
    rl = os.path.join(_DIR, 'lines_pc.json')
    if os.path.exists(rl):
        lin = json.load(open(rl, encoding='utf-8'))
        root = _SOURCES or lin['root']
        for fn in lin['functions']:
            ea = int(fn['address'], 16)
            ida_lines.add_sourcefile(ea, ea + fn['size'], os.path.join(root, lin['files'][fn['file']]))
            for off, line in fn['lines']:
                ida_nalt.set_source_linnum(ea + off, line)
                ln += 1
    print('symbols_pc: %d names, %d new functions, %d comments, %d lines' % (n, f, c, ln))


main()
'''

GHIDRA_SCRIPT_EN = r'''// Names, comments and source lines of NFS Most Wanted PC (speed.exe 1.3) for Ghidra (Java: no Jython or PyGhidra
// needed). Generated by scripts/simbolos_pc.py (English mode). It reads symbols_pc.json and lines_pc.json from the
// folder given as first argument or, by default, from the script's own folder.
//   GUI: Script Manager > Script Directories > add this folder > ImportSymbolsPC.java
//   headless: analyzeHeadless PROJECT NAME -process speed.exe -scriptPath FOLDER -postScript ImportSymbolsPC.java FOLDER [SOURCE_ROOT]
// It sets the decorated names (the "Demangler Microsoft" analyzer undecorates them), creates the missing functions,
// adds a plate comment with unit, file:line, match % and name provenance, and adds the source lines of the matched
// functions to the SourceFileManager (Ghidra 11.3+; Window > Source Files and Transforms). The optional second argument
// replaces the source root stored in lines_pc.json (the root of your clone).
// @category NFSMW
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import ghidra.app.script.GhidraScript;
import ghidra.program.database.sourcemap.SourceFile;
import ghidra.program.model.address.Address;
import ghidra.program.model.sourcemap.SourceFileManager;
import ghidra.program.model.symbol.SourceType;

public class ImportSymbolsPC extends GhidraScript {

    private static JsonObject read(File f) throws Exception {
        try (Reader r = new InputStreamReader(new FileInputStream(f), StandardCharsets.UTF_8)) {
            return JsonParser.parseReader(r).getAsJsonObject();
        }
    }

    private static String str(JsonObject o, String k) {
        JsonElement e = o.get(k);
        return (e == null || e.isJsonNull()) ? null : e.getAsString();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File dir = args.length > 0 ? new File(args[0]) : getSourceFile().getParentFile().getFile(false);
        JsonObject data = read(new File(dir, "symbols_pc.json"));
        int names = 0, functions = 0, comments = 0, failed = 0;
        for (JsonElement el : data.getAsJsonArray("symbols")) {
            JsonObject s = el.getAsJsonObject();
            boolean real = s.get("real").getAsBoolean();
            String file = str(s, "file");
            if (!real && file == null) {
                continue;
            }
            Address a = toAddr(Long.decode(str(s, "address")));
            boolean isFunction = "function".equals(str(s, "kind"));
            if (isFunction && getFunctionAt(a) == null) {
                disassemble(a);
                if (createFunction(a, null) != null) {
                    functions++;
                }
            }
            if (real) {
                try {
                    createLabel(a, str(s, "name"), true, SourceType.IMPORTED);
                    names++;
                }
                catch (Exception e) {
                    failed++;
                }
            }
            if (isFunction) {
                List<String> parts = new ArrayList<>();
                if (str(s, "unit") != null) {
                    parts.add(str(s, "unit"));
                }
                if (file != null) {
                    String line = str(s, "line");
                    parts.add(line != null ? file + ":" + line : file);
                }
                if (str(s, "match_percent") != null) {
                    parts.add(str(s, "match_percent") + " %");
                }
                if (str(s, "provenance") != null) {
                    parts.add("name: " + str(s, "provenance"));
                }
                if (!parts.isEmpty()) {
                    setPlateComment(a, String.join(" | ", parts));
                    comments++;
                }
            }
        }
        int lines = 0, rejected = 0;
        File fl = new File(dir, "lines_pc.json");
        if (fl.exists()) {
            SourceFileManager sfm = currentProgram.getSourceFileManager();
            JsonObject lin = read(fl);
            String root = lin.get("root").getAsString().replace('\\', '/');
            if (args.length > 1) {
                root = args[1].replace('\\', '/');   // another copy of the repository
            }
            List<SourceFile> files = new ArrayList<>();
            for (JsonElement f : lin.getAsJsonArray("files")) {
                String p = root + "/" + f.getAsString();
                if (!p.startsWith("/")) {
                    p = "/" + p;
                }
                SourceFile sf = new SourceFile(p);
                sfm.addSourceFile(sf);
                files.add(sf);
            }
            for (JsonElement el : lin.getAsJsonArray("functions")) {
                JsonObject fn = el.getAsJsonObject();
                long base = Long.decode(fn.get("address").getAsString());
                long size = fn.get("size").getAsLong();
                SourceFile sf = files.get(fn.get("file").getAsInt());
                JsonArray ls = fn.getAsJsonArray("lines");
                for (int i = 0; i < ls.size(); i++) {
                    JsonArray pair = ls.get(i).getAsJsonArray();
                    long off = pair.get(0).getAsLong();
                    long end = i + 1 < ls.size() ? ls.get(i + 1).getAsJsonArray().get(0).getAsLong() : size;
                    if (end <= off) {
                        continue;
                    }
                    try {
                        sfm.addSourceMapEntry(sf, pair.get(1).getAsInt(), toAddr(base + off), end - off);
                        lines++;
                    }
                    catch (Exception e) {
                        rejected++;
                    }
                }
            }
        }
        println(String.format("symbols_pc: %d names (%d rejected), %d new functions, %d comments, " +
            "%d lines (%d rejected)", names, failed, functions, comments, lines, rejected));
    }
}
'''


# --------------------------------------------------------------------------------------------------- PDB
S_END, S_GPROC32, S_LPROC32 = 0x0006, 0x1110, 0x110F
ABRE = {0x1102, 0x1103, 0x1104, 0x110F, 0x1110}   # registros que abren ambito (S_THUNK32, S_BLOCK32, S_WITH32, procs)
DEBUG_S_SYMBOLS, DEBUG_S_LINES, DEBUG_S_STRINGTABLE, DEBUG_S_FILECHKSMS = 0xF1, 0xF2, 0xF3, 0xF4
REL_SECTION, REL_SECREL = 0x000A, 0x000B
CH_TEXT = 0x60100020   # code | exec | read | align 1
CH_RDATA = 0x40100040
CH_DATA = 0xC0100040
CH_DEBUG = 0x42100040
NOMBRES_LLD = re.compile(r'^_*(load_config_used|tls_used|tls_index|ImageBase|safe_se_handler_\w+|guard_\w+|'
                         r'delayLoadHelper2|_?CTOR_LIST__|_?DTOR_LIST__|dynamic_tls_\w+)$')


def registro(tipo, cuerpo):
    cuerpo = bytes(cuerpo)
    # los registros de CodeView van alineados a 4 dentro del flujo del modulo
    pad = (-(len(cuerpo) + 4)) % 4
    cuerpo += bytes([0xF0 + k for k in range(pad, 0, -1)]) if pad else b''
    return struct.pack('<HH', len(cuerpo) + 2, tipo) + cuerpo


def subseccion(tipo, datos):
    datos = bytes(datos)
    return struct.pack('<II', tipo, len(datos)) + datos + b'\0' * ((-len(datos)) % 4)


def lld_link(explicito=None):
    """El enlazador COFF de LLVM. Por orden: --lld (o la variable LLD_LINK), la copia ya hecha en
    build/SPEED_EXE_1_3/simbolos/.lld/, el ld.lld.exe del NDK de Android copiado alli como lld-link.exe (con sus DLL),
    y por ultimo un lld-link del PATH (el de una version oficial de LLVM)."""
    explicito = explicito or os.environ.get('LLD_LINK')
    if explicito:
        p = Path(explicito)
        if not p.exists():
            raise SystemExit(L('--lld/LLD_LINK: no existe %s', '--lld/LLD_LINK: %s does not exist') % p)
        if p.name.lower().startswith('lld-link'):
            return p
        # ld.lld.exe u otro nombre: LLVM elige el modo por el nombre del ejecutable; se copia como lld-link.exe
        destino = BUILD / F('salida') / '.lld_explicit' / 'lld-link.exe'
        destino.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(p, destino)
        for dll in p.parent.glob('*.dll'):
            if dll.name.lower().startswith(('libxml2', 'libwinpthread', 'zlib', 'libzstd', 'libc++')):
                shutil.copy2(dll, destino.parent / dll.name)
        return destino
    destino = BUILD / F('salida') / '.lld' / 'lld-link.exe'
    if destino.exists():
        return destino
    candidatos = []
    for base in (os.environ.get('ANDROID_NDK_HOME'), os.environ.get('ANDROID_NDK_ROOT')):
        if base:
            candidatos += glob.glob(os.path.join(base, 'toolchains/llvm/prebuilt/*/bin/ld.lld*'))
    for sdk in (os.environ.get('ANDROID_HOME'), os.environ.get('ANDROID_SDK_ROOT'),
                os.path.join(os.environ.get('LOCALAPPDATA', ''), 'Android', 'Sdk')):
        if sdk:
            candidatos += sorted(glob.glob(os.path.join(sdk, 'ndk/*/toolchains/llvm/prebuilt/*/bin/ld.lld*')),
                                 reverse=True)
    if not candidatos:
        enpath = shutil.which('lld-link')
        return Path(enpath) if enpath else None
    origen = Path(candidatos[0])
    destino.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(origen, destino)
    for dll in origen.parent.glob('*.dll'):
        if dll.name.lower().startswith(('libxml2', 'libwinpthread', 'zlib', 'libzstd', 'libc++')):
            shutil.copy2(dll, destino.parent / dll.name)
    return destino


def ruta_fuente(fichero, raiz_fuente):
    """Ruta de un fichero fuente tal como va al PDB: absoluta a este clon, o bajo la raiz neutra de --raiz-fuente."""
    if raiz_fuente is None:
        return str(RAIZ / fichero)
    return str(PureWindowsPath(raiz_fuente) / PureWindowsPath(fichero))


def construir_pdb(pe, lista, lineas, carpeta, tmpdir, tipos=True, lld_explicito=None, raiz_fuente=None):
    lld = lld_link(lld_explicito)
    if lld is None:
        log(L('PDB: no hay lld-link (ni --lld, ni el ld.lld del NDK, ni lld-link en el PATH); se omite',
              'PDB: no lld-link found (no --lld, no Android NDK ld.lld, no lld-link on PATH); PDB skipped'))
        return None
    text_i, text = next((i + 1, s) for i, s in enumerate(pe.secciones) if s['nombre'] == '.text')
    tbase = pe.base + text['va']
    tfin = tbase + text['vsize']

    # trozos de .text por unidad: un rango continuo de una unidad va a su objeto
    RESTO = F('resto')   # objeto de lo que no es de ninguna unidad
    funcs = [e for e in lista if e['tipo'] == 'funcion' and e['secnum'] == text_i and e['tamano']]
    trozos = []  # [inicio, fin, unidad]
    for e in funcs:
        u = (e['unidad'] or RESTO).split('/')[-1]
        a, b = e['direccion'], min(e['direccion'] + e['tamano'], tfin)
        if trozos and trozos[-1][2] == u and trozos[-1][1] >= a - 16:
            trozos[-1][1] = max(trozos[-1][1], b)
        else:
            trozos.append([a, b, u])
    # quitar solapes y rellenar huecos con el objeto _resto
    final = []
    cursor = tbase
    for a, b, u in trozos:
        a = max(a, cursor)
        if b <= a:
            continue
        if a > cursor:
            final.append([cursor, a, RESTO])
        if final and final[-1][2] == u and final[-1][1] == a:
            final[-1][1] = b
        else:
            final.append([a, b, u])
        cursor = b
    if cursor < tfin:
        final.append([cursor, tfin, RESTO])
    assert sum(b - a for a, b, _ in final) == text['vsize']

    objetos = collections.OrderedDict()

    def obj(u):
        if u not in objetos:
            objetos[u] = dict(w=CoffW(), secs=[], nombres={})
        return objetos[u]

    for a, b, u in final:
        o = obj(u)
        idx = o['w'].seccion('.text$%08X' % a, b'\xCC' * (b - a), CH_TEXT)
        o['secs'].append((a, b, idx))

    # datos: .rdata y .data enteras en _resto
    resto = obj(RESTO)
    secs_datos = {}
    for i, s in enumerate(pe.secciones):
        if s['nombre'] in ('.rdata', '.data'):
            idx = resto['w'].seccion(s['nombre'], b'\0' * s['vsize'], CH_RDATA if s['nombre'] == '.rdata'
                                     else CH_DATA)
            secs_datos[i + 1] = idx

    # simbolos: el primero de cada nombre es externo (publico), los repetidos y los de lld, estaticos
    vistos = set()
    simbolo_de = {}
    externos_datos = set()
    for e in lista:
        if not e['real'] or not e['secnum']:
            continue
        nombre = e['nombre']
        externo = nombre not in vistos and not NOMBRES_LLD.match(nombre) and not e['estatica']
        vistos.add(nombre)
        if e['secnum'] == text_i:
            u = next((u for a, b, u in final if a <= e['direccion'] < b), None)
            o = objetos[u]
            a, b, idx = next(x for x in o['secs'] if x[0] <= e['direccion'] < x[1])
            si = o['w'].simbolo(nombre, e['direccion'] - a, idx, e['tipo'] == 'funcion', externo)
            simbolo_de[e['direccion']] = (u, si, externo)
        elif e['secnum'] in secs_datos:
            resto['w'].simbolo(nombre, e['secoff'], secs_datos[e['secnum']], False, externo)
            if externo:
                externos_datos.add(nombre)

    # .debug$S por unidad: el CodeView de nuestro objeto para las funciones casadas (S_GPROC32 con su tipo, parametros
    # y locales, y los S_UDT/S_CONSTANT/S_GDATA32 del modulo), un S_GPROC32 sin tipo para el resto de funciones con
    # nombre, y las lineas C13 de las casadas.
    lineas_de = {x['direccion']: x for x in lineas}
    por_unidad = collections.defaultdict(list)
    for e in lista:
        if e['tipo'] == 'funcion' and e['direccion'] in simbolo_de:
            por_unidad[simbolo_de[e['direccion']][0]].append(e)
    cuenta = collections.Counter()
    for u, es in por_unidad.items():
        w = objetos[u]['w']
        ob = next((e['_ob'] for e in es if e.get('_ob') is not None), None)
        debug_t = ob.debug_t() if (tipos and ob is not None) else None
        ext_idx = {}

        def ext(nombre):
            if nombre not in ext_idx:
                ext_idx[nombre] = w.simbolo(nombre, 0, 0, False, True)
            return ext_idx[nombre]

        syms = bytearray()
        srel = []

        def emitir(recs, func_si):
            for rb, rr in recs:
                rb = bytearray(rb)
                base = len(syms)
                for off, t, (clase, val) in rr:
                    if clase == 'func':
                        if t == REL_SECREL:
                            struct.pack_into('<I', rb, off, struct.unpack_from('<I', rb, off)[0] + val)
                        srel.append((base + off, func_si, t))
                    else:
                        srel.append((base + off, ext(val), t))
                pad = (-len(rb)) % 4
                if pad:
                    rb += bytes([0xF0 + k for k in range(pad, 0, -1)])
                    struct.pack_into('<H', rb, 0, len(rb) - 2)
                syms.extend(rb)

        if debug_t is not None:
            emitir(ob.registros(None, 0, 0, externos_datos), None)
        for e in es:
            _, si, externo = simbolo_de[e['direccion']]
            recs = None
            if debug_t is not None and e.get('_cand') is not None:
                c = e['_cand']
                recs = ob.registros(c['sec'], c['off'], c['tam'], externos_datos)
                if not any(struct.unpack_from('<H', rb, 2)[0] in (S_GPROC32, S_LPROC32) for rb, _ in recs):
                    recs = None
            if recs:
                emitir(recs, si)
                cuenta['con tipos'] += 1
                continue
            nombre = e['corto'] or e['nombre']
            m = re.match(r'^_([A-Za-z_]\w*?)(@\d+)?$', nombre)
            if m and not nombre.startswith('?'):
                nombre = m.group(1)
            fijo = struct.pack('<IIIIIII', 0, 0, 0, e['tamano'], 0, e['tamano'], 0)
            r = registro(S_GPROC32 if externo else S_LPROC32,
                         fijo + struct.pack('<IHB', 0, 0, 0) + nombre.encode('latin-1', 'replace') + b'\0')
            base = len(syms)
            srel.append((base + 4 + 28, si, REL_SECREL))
            srel.append((base + 4 + 32, si, REL_SECTION))
            syms += r + registro(S_END, b'')
            cuenta['sin tipos'] += 1

        # lineas
        xs = [lineas_de[e['direccion']] for e in es if e['direccion'] in lineas_de]
        strtab = bytearray(b'\0')
        chk = bytearray()
        chk_off = {}
        for x in xs:
            ruta = ruta_fuente(x['fichero'], raiz_fuente)
            if ruta not in chk_off:
                chk_off[ruta] = len(chk)
                chk += struct.pack('<IBB', len(strtab), 0, 0) + b'\0\0'
                strtab += ruta.encode('latin-1') + b'\0'
        subs = [(DEBUG_S_SYMBOLS, bytes(syms), srel)]
        for x in xs:
            si = simbolo_de[x['direccion']][1]
            ls = x['lineas']
            bloque = struct.pack('<III', chk_off[ruta_fuente(x['fichero'], raiz_fuente)], len(ls), 12 + 8 * len(ls))
            bloque += b''.join(struct.pack('<II', off, (ln & 0xFFFFFF) | 0x80000000) for off, ln in ls)
            subs.append((DEBUG_S_LINES, struct.pack('<IHHI', 0, 0, 0, x['tamano']) + bloque,
                         [(0, si, REL_SECREL), (4, si, REL_SECTION)]))
            cuenta['lineas'] += len(ls)
            cuenta['funciones con lineas'] += 1
        subs.append((DEBUG_S_FILECHKSMS, bytes(chk), []))
        subs.append((DEBUG_S_STRINGTABLE, bytes(strtab), []))
        # en secciones .debug$S de menos de 65535 reubicaciones (los simbolos se parten por registros enteros)
        trozos_s = []
        for tipo, datos, rels in subs:
            if tipo == DEBUG_S_SYMBOLS and len(rels) > 60000:
                inicio = 0
                actual = []
                q = 0
                while q < len(datos):
                    rl, = struct.unpack_from('<H', datos, q)
                    fin = q + 2 + rl
                    actual_r = [r for r in rels if q <= r[0] < fin]
                    if len(actual) + len(actual_r) > 60000:
                        trozos_s.append((tipo, datos[inicio:q], [(o - inicio, s_, t_) for o, s_, t_ in actual]))
                        inicio, actual = q, []
                    actual += actual_r
                    q = fin
                trozos_s.append((tipo, datos[inicio:], [(o - inicio, s_, t_) for o, s_, t_ in actual]))
            else:
                trozos_s.append((tipo, datos, rels))
        cuerpo = bytearray(struct.pack('<I', 4))
        relocs = []
        for tipo, datos, rels in trozos_s:
            if len(relocs) + len(rels) > 60000:
                idx = w.seccion('.debug$S', cuerpo, CH_DEBUG)
                for off, si, t in relocs:
                    w.reloc(idx, off, si, t)
                cuerpo, relocs = bytearray(struct.pack('<I', 4)), []
            cab = len(cuerpo) + 8
            cuerpo += subseccion(tipo, datos)
            relocs += [(cab + o_, s_, t_) for o_, s_, t_ in rels]
        idx = w.seccion('.debug$S', cuerpo, CH_DEBUG)
        for off, si, t in relocs:
            w.reloc(idx, off, si, t)
        if debug_t is not None:
            w.seccion('.debug$T', debug_t, CH_DEBUG)
            cuenta['modulos con tipos'] += 1

    # escribir objetos y enlazar
    # carpeta estable: los modulos del PDB llevan esta ruta (los objetos se borran al terminar)
    d = carpeta / 'objetos_pdb'
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True, exist_ok=True)
    rutas = []
    for u, o in objetos.items():
        r = d / ('%s.obj' % u)
        r.write_bytes(o['w'].bytes())
        rutas.append(r)
    pdb_nombre = pe.rsds['pdb'].replace('\\', '/').split('/')[-1] if pe.rsds else 'speed.pdb'
    pdb = carpeta / pdb_nombre
    exe_tmp = d / 'speed_pdb.dll'
    rsp = d / 'lld.rsp'
    if raiz_fuente is None:
        rutas_lld = ['/pdb:%s' % pdb, '/pdbaltpath:%s' % pdb_nombre, '/out:%s' % exe_tmp]
    else:
        # rutas neutras: relativas a la carpeta de los objetos; lld las completa con /pdbsourcepath en lugar del
        # directorio de trabajo (rutas de los modulos, y cwd y pdb del bloque de entorno del modulo del enlazador)
        rutas_lld = ['/pdb:%s' % pdb_nombre, '/pdbaltpath:%s' % pdb_nombre, '/out:%s' % exe_tmp.name,
                     '/pdbsourcepath:%s' % raiz_fuente]
    rsp.write_text('\n'.join(['/nologo', '/dll', '/noentry', '/fixed', '/machine:x86', '/safeseh:no',
                              '/nodefaultlib', '/base:0x%X' % pe.base,
                              '/merge:.buildid=.rdata', '/debug:full', '/opt:noref', '/opt:noicf', '/incremental:no'] +
                             rutas_lld + [r.name for r in rutas]) + '\n', encoding='utf-8')
    # con rutas relativas, cada modulo del PDB se llama como su unidad (zAI.obj...)
    p = subprocess.run([str(lld), '@%s' % rsp.name], capture_output=True, cwd=d)
    if p.returncode != 0:
        log(p.stdout.decode('latin-1')[-3000:], p.stderr.decode('latin-1')[-3000:])
        if tipos:
            log(L('PDB: lld-link fallo con los tipos de nuestros objetos; se repite sin tipos',
                  'PDB: lld-link failed with the types of our objects; linking again without types'))
            return construir_pdb(pe, lista, lineas, carpeta, tmpdir, tipos=False, lld_explicito=lld_explicito,
                                 raiz_fuente=raiz_fuente)
        raise SystemExit(L('lld-link fallo', 'lld-link failed'))
    avisos = p.stdout.decode('latin-1') + p.stderr.decode('latin-1')
    if avisos.strip():
        n = avisos.count('\n')
        log(L('lld-link: %d lineas de aviso; las primeras:', 'lld-link: %d warning lines; the first ones:') % n)
        log('\n'.join(avisos.splitlines()[:8]))
    # la disposicion tiene que ser la del original
    enl = Pe(exe_tmp)
    for s in pe.secciones:
        if s['nombre'] in ('.text', '.rdata', '.data'):
            t = next((x for x in enl.secciones if x['nombre'] == s['nombre']), None)
            if t is None or t['va'] != s['va']:
                raise SystemExit(L('PDB: %s cae en %s y no en 0x%X', 'PDB: %s lands at %s instead of 0x%X')
                                 % (s['nombre'], t and hex(t['va']), s['va']))
    if raiz_fuente is not None:
        shutil.move(str(d / pdb_nombre), str(pdb))
    poner_guid(pdb, pe)
    if raiz_fuente is not None:
        neutralizar_entorno(pdb, raiz_fuente)
    shutil.rmtree(d, ignore_errors=True)
    return dict(pdb=pdb, lineas=cuenta['lineas'], funciones=cuenta['funciones con lineas'], modulos=len(objetos),
                con_tipos=cuenta['con tipos'], sin_tipos=cuenta['sin tipos'], modulos_tipos=cuenta['modulos con tipos'],
                tipos=tipos)


def poner_guid(ruta, pe):
    """GUID y edad del RSDS de speed.exe en el flujo de informacion (1) y la edad en el DBI (3)."""
    d = bytearray(Path(ruta).read_bytes())
    bs, _, _, ndir, _, bm = struct.unpack_from('<6I', d, 32)
    nb = (ndir + bs - 1) // bs
    dirbloques = struct.unpack_from('<%dI' % nb, d, bm * bs)
    dirdata = b''.join(bytes(d[b * bs:(b + 1) * bs]) for b in dirbloques)[:ndir]
    n, = struct.unpack_from('<I', dirdata, 0)
    tams = struct.unpack_from('<%dI' % n, dirdata, 4)
    off = 4 + 4 * n
    bloques = []
    for t in tams:
        k = 0 if t == 0xFFFFFFFF else (t + bs - 1) // bs
        bloques.append(struct.unpack_from('<%dI' % k, dirdata, off))
        off += 4 * k

    def escribir(stream, pos, datos):
        for i, byte in enumerate(datos):
            p = pos + i
            d[bloques[stream][p // bs] * bs + p % bs] = byte

    escribir(1, 4, struct.pack('<II', pe.timestamp, pe.rsds['edad']) + pe.rsds['guid'])
    escribir(3, 8, struct.pack('<I', pe.rsds['edad']))
    Path(ruta).write_bytes(bytes(d))


def neutralizar_entorno(ruta, raiz_fuente):
    """Con --raiz-fuente: el S_ENVBLOCK que lld escribe en el modulo "* Linker *" lleva la ruta absoluta del propio
    lld-link (LLVM la saca de GetModuleFileName, /pdbsourcepath no la toca) y la linea de ordenes. Se reescribe en su
    sitio, con la misma longitud de registro: exe = solo el nombre, y cualquier ruta de este clon o de la carpeta del
    usuario cambiada por la raiz neutra. El resto del registro queda a cero (la lista de cadenas acaba en la vacia)."""
    d = bytearray(Path(ruta).read_bytes())
    bs, _, _, ndir, _, bm = struct.unpack_from('<6I', d, 32)
    nb = (ndir + bs - 1) // bs
    dirbloques = struct.unpack_from('<%dI' % nb, d, bm * bs)
    dirdata = b''.join(bytes(d[b * bs:(b + 1) * bs]) for b in dirbloques)[:ndir]
    n, = struct.unpack_from('<I', dirdata, 0)
    tams = struct.unpack_from('<%dI' % n, dirdata, 4)
    off = 4 + 4 * n
    privadas = sorted({str(RAIZ), str(RAIZ).replace('\\', '/'), str(Path.home()), str(Path.home()).replace('\\', '/')},
                      key=len, reverse=True)
    cambios = 0
    for s, t in enumerate(tams):
        k = 0 if t == 0xFFFFFFFF else (t + bs - 1) // bs
        bloques = struct.unpack_from('<%dI' % k, dirdata, off)
        off += 4 * k
        if not k:
            continue
        datos = bytearray(b''.join(bytes(d[b * bs:(b + 1) * bs]) for b in bloques)[:t])
        p = datos.find(b'\x3d\x11\x00cwd\x00')
        if p < 2:
            continue
        ini = p - 2
        fin = ini + 2 + struct.unpack_from('<H', datos, ini)[0]
        cadenas = bytes(datos[p + 3:fin]).split(b'\0')
        if b'' in cadenas:
            cadenas = cadenas[:cadenas.index(b'')]
        nuevas = []
        for i in range(0, len(cadenas) - 1, 2):
            clave, valor = cadenas[i].decode('latin-1'), cadenas[i + 1].decode('latin-1')
            if clave == 'exe':
                valor = PureWindowsPath(valor.replace('/', '\\')).name
            for priv in privadas:
                valor = re.sub(re.escape(priv), lambda m_: raiz_fuente, valor, flags=re.I)
            nuevas += [clave, valor]
        cuerpo = b''.join(x.encode('latin-1') + b'\0' for x in nuevas) + b'\0'
        if len(cuerpo) > fin - (p + 3):
            raise SystemExit(L('PDB: el bloque de entorno neutro no cabe en el registro de lld',
                                 'PDB: the neutral environment block does not fit in the lld record'))
        datos[p + 3:fin] = cuerpo + b'\0' * (fin - (p + 3) - len(cuerpo))
        for i, byte in enumerate(datos):
            d[bloques[i // bs] * bs + i % bs] = byte
        cambios += 1
    Path(ruta).write_bytes(bytes(d))
    if not cambios:
        log(L('PDB: no se encontro el bloque de entorno de lld (S_ENVBLOCK)', 'PDB: lld environment block (S_ENVBLOCK) not found'))


# --------------------------------------------------------------------------------------------------- prueba
def prueba(carpeta, pe, lista, lineas):
    """Carga speed.exe ORIGINAL con el dbghelp.dll de Windows y la carpeta del PDB como ruta de simbolos."""
    import ctypes
    from ctypes import wintypes
    dbg = ctypes.WinDLL('dbghelp.dll')

    class IMAGEHLP_MODULEW64(ctypes.Structure):
        _fields_ = [('SizeOfStruct', wintypes.DWORD), ('BaseOfImage', ctypes.c_uint64),
                    ('ImageSize', wintypes.DWORD), ('TimeDateStamp', wintypes.DWORD),
                    ('CheckSum', wintypes.DWORD), ('NumSyms', wintypes.DWORD), ('SymType', ctypes.c_int),
                    ('ModuleName', ctypes.c_wchar * 32), ('ImageName', ctypes.c_wchar * 256),
                    ('LoadedImageName', ctypes.c_wchar * 256), ('LoadedPdbName', ctypes.c_wchar * 256),
                    ('CVSig', wintypes.DWORD), ('CVData', ctypes.c_wchar * 780), ('PdbSig', wintypes.DWORD),
                    ('PdbSig70', ctypes.c_byte * 16), ('PdbAge', wintypes.DWORD), ('PdbUnmatched', wintypes.BOOL),
                    ('DbgUnmatched', wintypes.BOOL), ('LineNumbers', wintypes.BOOL), ('GlobalSymbols', wintypes.BOOL),
                    ('TypeInfo', wintypes.BOOL), ('SourceIndexed', wintypes.BOOL), ('Publics', wintypes.BOOL),
                    ('MachineType', wintypes.DWORD), ('Reserved', wintypes.DWORD)]

    class SYMBOL_INFOW(ctypes.Structure):
        _fields_ = [('SizeOfStruct', wintypes.ULONG), ('TypeIndex', wintypes.ULONG), ('Reserved', ctypes.c_uint64 * 2),
                    ('Index', wintypes.ULONG), ('Size', wintypes.ULONG), ('ModBase', ctypes.c_uint64),
                    ('Flags', wintypes.ULONG), ('Value', ctypes.c_uint64), ('Address', ctypes.c_uint64),
                    ('Register', wintypes.ULONG), ('Scope', wintypes.ULONG), ('Tag', wintypes.ULONG),
                    ('NameLen', wintypes.ULONG), ('MaxNameLen', wintypes.ULONG), ('Name', ctypes.c_wchar * 2000)]

    class IMAGEHLP_LINEW64(ctypes.Structure):
        _fields_ = [('SizeOfStruct', wintypes.DWORD), ('Key', ctypes.c_void_p), ('LineNumber', wintypes.DWORD),
                    ('FileName', ctypes.c_wchar_p), ('Address', ctypes.c_uint64)]

    dbg.SymInitializeW.argtypes = [wintypes.HANDLE, wintypes.LPCWSTR, wintypes.BOOL]
    dbg.SymLoadModuleExW.argtypes = [wintypes.HANDLE, wintypes.HANDLE, wintypes.LPCWSTR, wintypes.LPCWSTR,
                                     ctypes.c_uint64, wintypes.DWORD, ctypes.c_void_p, wintypes.DWORD]
    dbg.SymLoadModuleExW.restype = ctypes.c_uint64
    dbg.SymGetModuleInfoW64.argtypes = [wintypes.HANDLE, ctypes.c_uint64, ctypes.POINTER(IMAGEHLP_MODULEW64)]
    dbg.SymFromAddrW.argtypes = [wintypes.HANDLE, ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint64),
                                 ctypes.POINTER(SYMBOL_INFOW)]
    dbg.SymGetLineFromAddrW64.argtypes = [wintypes.HANDLE, ctypes.c_uint64, ctypes.POINTER(wintypes.DWORD),
                                          ctypes.POINTER(IMAGEHLP_LINEW64)]
    dbg.SymSetOptions(0x2 | 0x10 | 0x80000000)   # UNDNAME | LOAD_LINES | DEBUG
    h = wintypes.HANDLE(0x4E46534D)
    if not dbg.SymInitializeW(h, str(carpeta), False):
        raise SystemExit(L('SymInitialize fallo', 'SymInitialize failed'))
    base = dbg.SymLoadModuleExW(h, None, str(EXE), None, pe.base, pe.tam_imagen, None, 0)
    info = IMAGEHLP_MODULEW64()
    info.SizeOfStruct = ctypes.sizeof(IMAGEHLP_MODULEW64)
    dbg.SymGetModuleInfoW64(h, base, ctypes.byref(info))
    tipos = {0: 'SymNone', 1: 'SymCoff', 2: 'SymCv', 3: 'SymPdb', 4: 'SymExport', 5: 'SymDeferred', 6: 'SymSym',
             7: 'SymDia', 8: 'SymVirtual'}
    out = [L('modulo: base 0x%X, tipo %s, pdb %s, sin casar %s, lineas %s, publicos %s, simbolos %d',
             'module: base 0x%X, type %s, pdb %s, unmatched %s, lines %s, publics %s, symbols %d') % (
        base, tipos.get(info.SymType, info.SymType), info.LoadedPdbName, bool(info.PdbUnmatched),
        bool(info.LineNumbers), bool(info.Publics), info.NumSyms)]
    # muestra: nombres
    reales = [e for e in lista if e['real'] and e['tipo'] == 'funcion' and not e['estatica']]
    paso = max(1, len(reales) // 2000)
    ok = mal = 0
    ejemplos = []
    fallos = []
    for e in reales[::paso]:
        si = SYMBOL_INFOW()
        si.SizeOfStruct = 88   # sizeof(SYMBOL_INFOW) con Name[1]
        si.MaxNameLen = 2000
        desp = ctypes.c_uint64()
        if dbg.SymFromAddrW(h, e['direccion'], ctypes.byref(desp), ctypes.byref(si)) and desp.value == 0:
            ok += 1
            if len(ejemplos) < 4:
                ejemplos.append('  0x%08X %s' % (e['direccion'], si.Name))
        else:
            mal += 1
            if len(fallos) < 12:
                fallos.append(L('  falla 0x%08X %s', '  fails 0x%08X %s') % (e['direccion'], e.get('nombre', '')))
    out.append(L('nombres: %d de %d direcciones de muestra resuelven a un simbolo exacto',
                 'names: %d of %d sampled addresses resolve to the exact symbol') % (ok, ok + mal))
    out += ejemplos + fallos
    ok = mal = 0
    ejemplos = []
    paso = max(1, len(lineas) // 2000)
    for x in lineas[::paso]:
        off, ln = x['lineas'][len(x['lineas']) // 2]
        li = IMAGEHLP_LINEW64()
        li.SizeOfStruct = ctypes.sizeof(IMAGEHLP_LINEW64)
        desp = wintypes.DWORD()
        if dbg.SymGetLineFromAddrW64(h, x['direccion'] + off, ctypes.byref(desp), ctypes.byref(li)) and \
                li.LineNumber == ln and li.FileName and li.FileName.replace('\\', '/').lower().endswith(
                    x['fichero'].lower()):
            ok += 1
            if len(ejemplos) < 4:
                ejemplos.append('  0x%08X %s:%d' % (x['direccion'] + off, li.FileName, li.LineNumber))
        else:
            mal += 1
    out.append(L('lineas: %d de %d direcciones de muestra dan el fichero y la linea esperados',
                 'lines: %d of %d sampled addresses give the expected file and line') % (ok, ok + mal))
    out += ejemplos
    # tipos y locales (solo si el PDB lleva el CodeView de nuestros objetos)
    CB = ctypes.WINFUNCTYPE(wintypes.BOOL, ctypes.POINTER(SYMBOL_INFOW), wintypes.ULONG, ctypes.c_void_p)
    dbg.SymEnumTypesW.argtypes = [wintypes.HANDLE, ctypes.c_uint64, CB, ctypes.c_void_p]
    dbg.SymEnumSymbolsW.argtypes = [wintypes.HANDLE, ctypes.c_uint64, wintypes.LPCWSTR, CB, ctypes.c_void_p]
    dbg.SymSetScopeFromAddr.argtypes = [wintypes.HANDLE, ctypes.c_uint64]
    ntipos = [0]

    def cuenta_tipos(si, tam, ctx):
        ntipos[0] += 1
        return True

    cb_tipos = CB(cuenta_tipos)
    dbg.SymEnumTypesW(h, base, cb_tipos, None)
    out.append(L('tipos: %d tipos con nombre en el PDB', 'types: %d named types in the PDB') % ntipos[0])
    con_locales = 0
    ejemplos = []
    for x in lineas[::max(1, len(lineas) // 300)]:
        nombres = []

        def junta(si, tam, ctx):
            nombres.append(si.contents.Name)
            return True

        cb = CB(junta)
        if dbg.SymSetScopeFromAddr(h, x['direccion']):
            dbg.SymEnumSymbolsW(h, 0, None, cb, None)
        if nombres:
            con_locales += 1
            if len(ejemplos) < 3:
                ejemplos.append('  0x%08X: %s' % (x['direccion'], ', '.join(nombres[:8])))
    out.append(L('locales: %d de %d funciones de muestra con parametros o locales',
                 'locals: %d of %d sampled functions have parameters or locals') % (
        con_locales, len(lineas[::max(1, len(lineas) // 300)])))
    out += ejemplos
    dbg.SymCleanup(h)
    return out


# --------------------------------------------------------------------------------------------------- main
DESCRIPCION_EN = ('Symbols of the PC executable (speed.exe 1.3) as a real PDB and companion files: an MSVC .map, '
                  'JSON/CSV, IDA and Ghidra scripts, an x64dbg database, and a PDB with publics, procedures, types '
                  'and source lines of the matched functions.')


def quiere_ingles(argv):
    """--english, --lang en o --lang=en (se mira antes de montar el analizador, para la ayuda)."""
    for i, a in enumerate(argv):
        if a == '--english' or a == '--lang=en' or (a == '--lang' and i + 1 < len(argv) and argv[i + 1] == 'en'):
            return True
    return False


def leer_generado(carpeta):
    """(lista, lineas) para --solo-prueba desde lo generado, en espanol o en ingles (lo que haya en la carpeta)."""
    if (carpeta / FICHEROS['json'][1]).exists() and not (carpeta / FICHEROS['json'][0]).exists():
        d = json.loads((carpeta / FICHEROS['json'][1]).read_text(encoding='utf-8'))
        lista = [dict(direccion=int(s['address'], 16), real=s['real'], tipo=TIPO_ES.get(s['kind'], s['kind']),
                      estatica=s['static'], nombre=s['name']) for s in d['symbols']]
        lj = json.loads((carpeta / FICHEROS['lineas_json'][1]).read_text(encoding='utf-8'))
        lineas = [dict(direccion=int(f['address'], 16), fichero=lj['files'][f['file']],
                       lineas=[tuple(x) for x in f['lines']]) for f in lj['functions']]
        return lista, lineas
    d = json.loads((carpeta / FICHEROS['json'][0]).read_text(encoding='utf-8'))
    lista = [dict(direccion=int(s['direccion'], 16), real=s['real'], tipo=s['tipo'], estatica=s['estatica'],
                  nombre=s['nombre'])
             for s in d['simbolos']]
    lj = json.loads((carpeta / FICHEROS['lineas_json'][0]).read_text(encoding='utf-8'))
    lineas = [dict(direccion=int(f['direccion'], 16), fichero=lj['ficheros'][f['fichero']],
                   lineas=[tuple(x) for x in f['lineas']]) for f in lj['funciones']]
    return lista, lineas


TIPO_ES = {v: k for k, v in TIPO_EN.items()}


def opcion(es, en):
    """Los dos nombres de una opcion, con el del idioma de la salida primero (asi sale en la ayuda)."""
    return (en, es) if EN else (es, en)


def main(argv=None):
    global EN, EXE
    argv = sys.argv[1:] if argv is None else argv
    EN = quiere_ingles(argv)
    ap = argparse.ArgumentParser(description=DESCRIPCION_EN if EN else __doc__.split('\n\n')[0])
    ap.add_argument(*opcion('--salida', '--output'), dest='salida', metavar=L('CARPETA', 'DIR'), type=Path,
                    default=BUILD / F('salida'),
                    help=L('carpeta de salida (por defecto build/SPEED_EXE_1_3/simbolos)',
                           'output folder (default build/SPEED_EXE_1_3/symbols)'))
    ap.add_argument(*opcion('--sin-pdb', '--no-pdb'), dest='sin_pdb', action='store_true',
                    help=L('no enlazar el PDB', 'do not link the PDB (no linker needed)'))
    ap.add_argument(*opcion('--sin-historia', '--no-history'), dest='sin_historia', action='store_true',
                    help=L('no recorrer la historia de git (procedencia)',
                           'do not walk the git history of symbols.json (name provenance by round)'))
    ap.add_argument(*opcion('--prueba', '--check'), dest='prueba', action='store_true',
                    help=L('comprobar el PDB con dbghelp.dll al terminar',
                           'check the PDB with the Windows dbghelp.dll when done'))
    ap.add_argument(*opcion('--solo-prueba', '--check-only'), dest='solo_prueba', action='store_true',
                    help=L('solo comprobar lo ya generado', 'only check the files already generated in the output folder'))
    ap.add_argument(*opcion('--sin-tipos', '--no-types'), dest='sin_tipos', action='store_true',
                    help=L('PDB sin los tipos ni las locales de nuestros objetos',
                           'PDB without the types, prototypes and locals of our objects (names and lines only)'))
    ap.add_argument(*opcion('--raiz-fuente', '--source-root'), dest='raiz_fuente', metavar=L('RAIZ', 'ROOT'), default=None,
                    help=L('raiz neutra (p. ej. C:\\nfsmw) para las rutas de fuente del PDB y de lineas_pc.json, y '
                           'para las rutas que lld escribe en el PDB; sin ella van las absolutas de este clon',
                           'neutral root (e.g. C:\\nfsmw) for the source paths in the PDB and lines_pc.json, and for '
                           'the paths lld writes into the PDB; without it, the absolute paths of this clone are used'))
    ap.add_argument('--lld', metavar='EXE', default=None,
                    help=L('lld-link.exe (o ld.lld.exe) a usar; por defecto LLD_LINK, el NDK de Android o el PATH',
                           'lld-link.exe (or ld.lld.exe) to use; default: LLD_LINK, the Android NDK, or PATH'))
    ap.add_argument('--exe', type=Path, default=None,
                    help=L('speed.exe 1.3 original (por defecto orig/SPEED_EXE_1_3/speed.exe); con --solo-prueba '
                           'basta este script, el exe y la carpeta --salida con lo generado',
                           'the original speed.exe 1.3 (default orig/SPEED_EXE_1_3/speed.exe); with --check-only, '
                           'only this script, the executable and the --output folder are needed'))
    ap.add_argument('--english', action='store_true',
                    help=L('salida en ingles: nombres de fichero, columnas, valores, mensajes y scripts de IDA/Ghidra',
                           'English output: file names, columns, values, messages and IDA/Ghidra scripts'))
    ap.add_argument('--lang', choices=('es', 'en'), default=None, help=L('idioma de la salida (es por defecto)',
                                                                         'output language (es by default)'))
    args = ap.parse_args(argv)
    if args.exe is not None:
        EXE = args.exe.resolve()
    if args.raiz_fuente is not None:
        args.raiz_fuente = str(PureWindowsPath(args.raiz_fuente.replace('/', '\\'))).rstrip('\\') or '\\'
    args.salida.mkdir(parents=True, exist_ok=True)
    if args.solo_prueba:
        pe = Pe(EXE)
        lista, lineas = leer_generado(args.salida)
        for l in prueba(args.salida, pe, lista, lineas):
            log(l)
        return
    with tempfile.TemporaryDirectory(prefix='simbolos_pc_') as tmp:
        pe, lista, lineas, stats, commits = construir(args, tmp)
        escribir_map(pe, lista, args.salida / 'speed.map')
        escribir_json_csv(pe, lista, lineas, stats, args.salida, args.raiz_fuente)
        escribir_x64dbg(pe, lista, args.salida / 'speed.exe.dd32')
        (args.salida / F('ida')).write_text(IDA_SCRIPT_EN if EN else IDA_SCRIPT, encoding='utf-8')
        (args.salida / F('ghidra')).write_text(GHIDRA_SCRIPT_EN if EN else GHIDRA_SCRIPT, encoding='utf-8')
        viejo = args.salida / 'ghidra_importa_simbolos_pc.py'
        if viejo.exists():
            viejo.unlink()
        res = None
        if not args.sin_pdb:
            res = construir_pdb(pe, lista, lineas, args.salida, tmp, tipos=not args.sin_tipos, lld_explicito=args.lld,
                                raiz_fuente=args.raiz_fuente)
    for k in ('funciones', 'funciones con nombre', 'datos con nombre', '100', '100 verificadas',
              'funciones con lineas', 'lineas', 'ficheros fuente'):
        log('%-26s %d' % (RESUMEN_EN[k], stats[k]) if EN else '%-22s %d' % (k, stats[k]))
    if res:
        log(L('PDB %s: %d modulos (%d con tipos), %d funciones con tipo y locales, %d sin tipo, %d con lineas, '
              '%d lineas; GUID %s edad %d',
              'PDB %s: %d modules (%d with types), %d functions with type and locals, %d without type, %d with lines, '
              '%d lines; GUID %s age %d') % (res['pdb'], res['modulos'], res['modulos_tipos'], res['con_tipos'],
                                             res['sin_tipos'], res['funciones'], res['lineas'],
                                             guid_texto(pe.rsds['guid']), pe.rsds['edad']))
    if args.prueba:
        for l in prueba(args.salida, pe, lista, lineas):
            log(l)


if __name__ == '__main__':
    main()
