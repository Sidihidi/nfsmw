#!/usr/bin/env python3
"""nombra_grafo.py -- nombres de Xbox 360 y PC por CODIGO IGUAL y por el grafo de referencias (LLAMANTES).

objdiff empareja por nombre, y el informe (scripts/pctall.py) se genera con `functionRelocDiffs=none`: el DESTINO de una
reubicacion no cuenta, solo la instruccion. Asi que una funcion sin nombre del original (fn_ en 360, sub_ en PC) casa
en cuanto lleva el nombre de una funcion nuestra con el mismo ESQUELETO (el codigo con el campo reubicado a cero).
tools/nombra_por_codigo.py y la ronda 12 (05bbf803) solo nombraban las que eran unicas y ademas en orden, o con el
texto identico incluidos los destinos; quedaban dos bolsas:
  - funciones cuyo codigo es igual salvo destinos SIN NOMBRE en el original (fn_/lbl_/sub_/dword_...);
  - funciones con el mismo esqueleto que VARIAS nuestras (Lerp/VU0_v3lerp, reserve/push_back de cada Vector<T>...).

Lo que hace verificable cada nombre son los destinos de las reubicaciones:
  - un destino con nombre en el original tiene que llamarse igual en el nuestro (si no, la pareja es FALSA); las
    constantes `__real@XXXXXXXX` y las cadenas `??_C@_0...` se comprueban contra los BYTES del original, porque los
    nombres de datos de la configuracion no son fiables (en 360, `__real@00000000_4` guarda 1.0f);
  - un destino sin nombre recibe EVIDENCIA: en una pareja ya aceptada, la llamada k-esima del original y la del nuestro
    van al mismo sitio (solo si las dos apuntan al principio del simbolo);
  - las anclas (mismo nombre y mismo esqueleto en los dos lados) dan la evidencia inicial; cada pareja aceptada, mas.
Una funcion sin nombre se nombra si (a) es la UNICA pareja compatible en los dos sentidos de su grupo de esqueleto
(`unica`), (b) los llamantes la nombran: toda la evidencia de su direccion dice ese nombre (`llamantes`; `secuencia`
si viene de alinear las llamadas de funciones con nombre que aun no casan), o (c) entre varias compatibles es la unica
con mas destinos confirmados, y lo es tambien en el otro sentido (`mejor`). Punto fijo, y una segunda lectura con todos
los nombres decididos tira las parejas que se contradigan. Una funcion corta sin ningun destino que la confirme ademas
tiene que caer entre sus anclas vecinas en el orden de nuestro objeto.

Diferencias entre versiones:
  - 360 (EUROPEGERMILESTONE): enlazado con LTCG; lleva copias por unidad de las funciones en linea (?Normalize@UMath
    esta dos veces byte a byte), asi que un nombre puede repetirse en unidades distintas, nunca dos veces en una.
    Aplica cambiando el nombre de lineas `fn_XXXXXXXX = .text:` de symbols.txt.
  - PC (SPEED_EXE_1_3): cada COMDAT esta una sola vez; un nombre no puede estar ya en symbols.json. Ademas de los
    sub_/nullsub_/unknown_libname_, se sustituyen los nombres que MSVC 7.1 no puede emitir (demangle de IDA
    `FEObject::FEObject`, decoracion de GCC traida de GameCube `Default__14PlayerSettings`, `nombre__0061CD50`) pero
    SOLO por el nuestro que diga la misma funcion (mismo nombre, y mismos ambitos o clase): nunca las firmas de
    biblioteca falsas de IDA (`Concurrency::details::...`). La funcion del original mide lo que dice symbols.json
    (como la compara objdiff). Aplica cambiando el campo name con sustitucion exacta de texto (no reescribe el JSON).
Solo se nombran funciones de las unidades con objeto nuestro (las SourceLists); nunca datos, nunca se crean simbolos.

    python scripts/nombra_grafo.py EUROPEGERMILESTONE|SPEED_EXE_1_3 [--tsv SALIDA.tsv] [--aplicar] [--detalle]
                                   [--anclas-malas] [--validar FRACCION] [--nuestros=DIR]

--nuestros=DIR toma nuestros objetos de DIR en vez de build/<V>/src. En 360 la medida oficial sale del enlace LTCG
(scripts/ltcg360.py): `--nuestros=build/EUROPEGERMILESTONE/ltcg/obj` nombra contra el codigo enlazado, que es el que
se compara. Ronda 39: 361 nombres (28 kB) que contra los objetos de fichero a fichero no salian; --validar 0,3 con
esos objetos: 811 de 811 bien.
Ronda 40: los destinos de datos que el troceado LTCG no sabe nombrar (ltcg_XXXXXXXX) contaban como nombre distinto del
original: eran 1.137 de las 1.291 'anclas malas' y frenaban la evidencia. Tratados como anonimos: +1.160 nombres
(110 kB); --validar 0,3: 1.310 de 1.310 bien. Los nombres falsos que quedan los corrige scripts/nombra_corrige360.py.
--anclas-malas lista las funciones YA nombradas cuyo codigo es igual al nuestro pero con un destino con nombre que
contradice al nuestro: nombres heredados que pueden estar mal (el informe las da al 100 % igual).
--validar F esconde una fraccion F de las anclas y cuenta cuantas recupera con SU nombre (no aplica nada). Ronda 13,
con 0,3: 360 904 de 905 bien (el fallo, dos destructores de interfaz con el mismo codigo), PC 2.546 de 2.546.
Ronda 13 (2026-09-30), repetido hasta que no salio nada: 360 +828 funciones (+ 3 nombres de acceso viejo que PC
confirma), PC +2.467 (de 2.475 nombres: 8 funciones de zLua con el tamano de IDA inflado por el relleno); 0 perdidas.
Repetirlo tras cada tanda: cada funcion nueva que compile igual es un nombre mas, y cada nombre nuevo es un ancla.
"""
import bisect
import collections
import glob
import json
import os
import re
import struct
import sys
from escritura import entero

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAX_NOMBRE = 250  # 360: jeff no admite `filename:` en los atributos; un nombre mas largo no se puede poner

# COFF
PPC_REL24, PPC_REFHI, PPC_REFLO, PPC_PAIR, PPC_ADDR32 = 6, 0x10, 0x11, 0x12, 2
I386_DIR32, I386_REL32 = 6, 0x14


class Coff:
    def __init__(self, ruta):
        d = open(ruta, 'rb').read()
        self.d = d
        maquina, nsec, _, psym, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
        assert maquina in (0x01F2, 0x014C), ruta
        self.ppc = maquina == 0x01F2
        self.secciones = []
        for i in range(nsec):
            o = 20 + opt + i * 40
            nombre, vsz, va, rsz, praw, prel, plin, nrel, nlin, car = struct.unpack_from('<8sIIIIIIHHI', d, o)
            if nrel == 0xFFFF and car & 0x01000000:  # IMAGE_SCN_LNK_NRELOC_OVFL
                nrel = struct.unpack_from('<I', d, prel)[0] - 1
                prel += 10
            relocs = [struct.unpack_from('<IIH', d, prel + k * 10) for k in range(nrel)]
            self.secciones.append({'tam': rsz, 'praw': praw, 'relocs': relocs, 'car': car})
        cad = psym + nsym * 18
        self.simbolos = {}
        self.lista = []
        i = 0
        while i < nsym:
            o = psym + i * 18
            n = d[o:o + 8]
            if n[:4] == b'\0\0\0\0':
                off = struct.unpack_from('<I', n, 4)[0]
                n = d[cad + off:d.index(b'\0', cad + off)]
            else:
                n = n.rstrip(b'\0')
            valor, sec, tipo, clase, naux = struct.unpack_from('<IhHBB', d, o + 8)
            s = (n.decode('latin-1'), valor, sec, clase)
            self.simbolos[i] = s
            self.lista.append(s)
            i += 1 + naux

    def funciones(self, tamanos=None):
        """{nombre: (unidades, {posicion: (tipo, destino)})}: palabras de 32 bits en PPC, bytes en x86. Con `tamanos`
        ({nombre: bytes}) una funcion x86 mide lo que dice la configuracion, como la compara objdiff: si IDA le puso
        dentro los int3 de alineacion, no se recortan (y no casara con la nuestra, que no los lleva)."""
        por_sec = collections.defaultdict(list)
        for nombre, valor, sec, clase in self.lista:
            if sec <= 0 or clase not in (2, 3) or nombre.startswith(('.', '$', '@')):
                continue
            if not self.secciones[sec - 1]['car'] & 0x20:
                continue
            por_sec[sec].append((valor, nombre))
        out = {}
        paso = 4 if self.ppc else 1
        for sec, lista in por_sec.items():
            s = self.secciones[sec - 1]
            datos = self.d[s['praw']:s['praw'] + s['tam']]
            relocs = sorted((r[0], r[2], self.simbolos.get(r[1], ('?',))[0]) for r in s['relocs']
                            if not (self.ppc and r[2] == PPC_PAIR))
            offs = [r[0] for r in relocs]
            lista.sort()
            for k, (valor, nombre) in enumerate(lista):
                fin = lista[k + 1][0] if k + 1 < len(lista) else len(datos)
                if self.ppc:
                    n = (fin - valor) // 4
                    if n <= 0:
                        continue
                    pal = list(struct.unpack_from('>%dI' % n, datos, valor))
                    while pal and pal[-1] == 0:  # relleno de alineacion entre funciones
                        pal.pop()
                else:
                    if tamanos and nombre in tamanos:
                        pal = list(datos[valor:min(fin, valor + tamanos[nombre])])
                    else:
                        pal = list(datos[valor:fin])
                        while pal and pal[-1] == 0xCC:  # int3 de alineacion
                            pal.pop()
                if not pal:
                    continue
                rel = {}
                j = bisect.bisect_left(offs, valor)
                while j < len(relocs) and relocs[j][0] < valor + paso * len(pal):
                    ro, rt, rn = relocs[j]
                    rel[(ro - valor) // paso] = (rt, rn)
                    j += 1
                out[nombre] = (pal, rel)
        return out


ACCESO = {c: g for g, cs in enumerate(('AIQ', 'BJR', 'CKS', 'DLT', 'EMU', 'FNV', '012')) for c in cs}


def equivalentes(orig, nuestro):
    """El mismo simbolo con otro nombre: el troceado de 360 desambigua los repetidos con `_N` (`__real@00000000_2`),
    y una cabecera nuestra que declara public lo que el original tiene private o protected solo cambia la letra de
    acceso de la decoracion de MSVC (`?Reset@AIVehiclePid@@MAEXXZ` y `...@@UAEXXZ`: la misma funcion)."""
    if orig == nuestro:
        return True
    m = re.match(r'^(.*)_\d+$', orig)
    if m and m.group(1) == nuestro:
        return True
    if len(orig) == len(nuestro) and orig.startswith('?'):
        dif = [i for i, (x, y) in enumerate(zip(orig, nuestro)) if x != y]
        if len(dif) == 1:
            i = dif[0]
            return (orig[i - 2:i] == '@@' and orig[i] in ACCESO and nuestro[i] in ACCESO
                    and ACCESO[orig[i]] == ACCESO[nuestro[i]])
    return False


def cadena_msvc(nombre):
    """Prefijo de bytes que codifica `??_C@_0<long>@<hash>@<texto>@` (None si no es una cadena de char)."""
    m = re.match(r'^\?\?_C@_0[A-P0-9]+@[A-P]+@(.*)@$', nombre)
    if not m:
        return None
    esp = {'0': b',', '1': b'/', '2': b'\\', '3': b':', '4': b'.', '5': b' ', '6': b'\n', '7': b'\t', '8': b"'",
           '9': b'-'}
    t = m.group(1)
    out = b''
    i = 0
    while i < len(t):
        ch = t[i]
        if ch != '?':
            out += ch.encode('latin-1')
            i += 1
        elif t[i + 1:i + 2] == '$':
            h = t[i + 2:i + 4]
            if len(h) < 2 or any(x not in 'ABCDEFGHIJKLMNOP' for x in h):
                return None
            out += bytes([(ord(h[0]) - 65) * 16 + ord(h[1]) - 65])
            i += 4
        elif t[i + 1:i + 2] in esp:
            out += esp[t[i + 1]]
            i += 2
        elif 'a' <= t[i + 1:i + 2] <= 'z':
            out += bytes([0xE1 + ord(t[i + 1]) - ord('a')])
            i += 2
        elif 'A' <= t[i + 1:i + 2] <= 'Z':
            out += bytes([0xC1 + ord(t[i + 1]) - ord('A')])
            i += 2
        else:
            return None
    return out


def msvc_partes(nombre):
    """(nombre base, [ambitos de dentro a fuera]) de una decoracion de MSVC sencilla; None si no se entiende.
    `?GetAttachments@Activity@Sim@@...` -> ('GetAttachments', ['Activity', 'Sim']); `??0FEObject@@...` ->
    ('FEObject', ['FEObject']); `??1X@@` -> ('~X', ['X']); `??$BindFunction@...` -> ('BindFunction', [])."""
    if nombre.startswith('??$'):
        m = re.match(r'^\?\?\$([A-Za-z_]\w*)@', nombre)
        return (m.group(1), []) if m else None
    m = re.match(r'^\?(\?[01])?([A-Za-z_]\w*)@((?:[A-Za-z_]\w*@)*)@', nombre)
    if not m:
        m = re.match(r'^\?([A-Za-z_]\w*)@', nombre)
        return (m.group(1), None) if m else None  # ambitos con plantillas: solo el nombre
    base = m.group(2)
    ambitos = [x for x in m.group(3).split('@') if x]
    if m.group(1) == '?0':
        return base, [base] + ambitos
    if m.group(1) == '?1':
        return '~' + base, [base] + ambitos
    return base, ambitos


def gcc_partes(nombre):
    """(base, clase o None) de una decoracion de GCC 2.95 sencilla: `Default__14PlayerSettings` ->
    ('Default', 'PlayerSettings'); `f__FPc` -> ('f', None)."""
    m = re.match(r'^([A-Za-z_]\w*?)__(\d+)([A-Za-z_]\w*)', nombre)
    if m:
        k = int(m.group(2))
        return m.group(1), m.group(3)[:k] if len(m.group(3)) >= k else None
    m = re.match(r'^([A-Za-z_]\w*?)__[FCQt]', nombre)
    return (m.group(1), None) if m else None


def mismo_nombre_de_pila(raro, nuestro):
    """Un nombre que MSVC no puede emitir (demangle de IDA, decoracion de GCC, `nombre__DIRECCION`) y el nuestro dicen
    la MISMA funcion: mismo nombre y mismos ambitos (IDA), mismo nombre y clase (GCC) o mismo nombre (sufijo)."""
    p = msvc_partes(nuestro)
    if not p:
        return False
    base, ambitos = p
    if '::' in raro and '(' not in raro:
        partes = raro.split('::')
        ult = partes[-1].replace('__', '~', 1) if partes[-1].startswith('__') else partes[-1]
        return (ambitos is not None and len(ambitos) >= 1 and ult == base
                and partes[:-1] == list(reversed(ambitos))[-(len(partes) - 1):])
    m = re.match(r'^([A-Za-z_]\w*?)__[0-9A-F]{8}$', raro)
    if m:
        return m.group(1) == base
    g = gcc_partes(raro)
    if g:
        return g[0] == base and (g[1] is None or ambitos is None or (ambitos and g[1] == ambitos[0]))
    return False


class Memoria:
    """Bytes de datos del original, para comprobar los nombres de MSVC que llevan el contenido dentro."""

    def __init__(self, trozos):
        self.trozos = sorted(trozos)
        self.inicios = [t[0] for t in self.trozos]

    def lee(self, a, n):
        k = bisect.bisect_right(self.inicios, a) - 1
        if k < 0:
            return None
        ini, d = self.trozos[k]
        if a + n > ini + len(d):
            return None
        return d[a - ini:a - ini + n]

    def comprueba(self, a, nombre):
        """True/False si el nombre dice su contenido y el original lo tiene (o no) en `a`; None si no se sabe."""
        m = re.match(r'^__real@([0-9a-f]{8}|[0-9a-f]{16})$', nombre)
        if m:
            esperado = bytes.fromhex(m.group(1))
            real = self.lee(a, len(esperado))
            if real is None:
                return None
            return real == esperado or real == esperado[::-1]  # 360 guarda big endian; PC, little
        pref = cadena_msvc(nombre)
        if pref:
            real = self.lee(a, len(pref))
            return None if real is None else real == pref
        return None


class X360:
    V = 'EUROPEGERMILESTONE'
    PASO = 4
    MIN = 12          # bytes: por debajo, una unica sin destino que la confirme no se nombra
    CORTA = 20        # bytes: por debajo, ademas tiene que estar en orden
    LLAMADAS = (PPC_REL24,)
    SIN_NOMBRE = re.compile(r'^(fn|lbl|sub|func)_([0-9A-F]{8})$')
    GUARDAR = ('__savegpr', '__restgpr', '__savefpr', '__restfpr', '__savevmx', '__restvmx')

    def __init__(self):
        self.lineas = open(os.path.join(RAIZ, 'config', self.V, 'symbols.txt'), encoding='utf-8').read().split('\n')
        self.por_dir = collections.defaultdict(list)
        self.por_nombre = collections.defaultdict(list)
        for i, l in enumerate(self.lineas):
            m = re.match(r'(\S+) = (\.\w+):0x([0-9A-Fa-f]+);(.*)$', l)
            if m:
                a = int(m.group(3), 16)
                self.por_dir[a].append((m.group(1), m.group(2), i))
                self.por_nombre[m.group(1)].append(a)
        self.texto = {a for a, es in self.por_dir.items() if any(e[1] == '.text' for e in es)}
        self.rangos = []
        unidad = None
        for l in open(os.path.join(RAIZ, 'config', self.V, 'splits.txt'), encoding='utf-8'):
            if l and not l[0].isspace() and l.rstrip().endswith(':'):
                unidad = l.strip()[:-1].rsplit('.', 1)[0]
                continue
            m = re.match(r'\s+\.text\s+start:0x([0-9A-F]+) end:0x([0-9A-F]+)', l)
            if m and unidad and unidad != 'Sections':
                self.rangos.append((int(m.group(1), 16), int(m.group(2), 16), unidad))
        self.rangos.sort()
        trozos = []
        for p in glob.glob(os.path.join(RAIZ, 'build', self.V, 'obj', 'auto_*.obj')):
            m = re.search(r'auto_\d+_([0-9A-F]{8})_(rdata|data)\.obj$', p)
            if m:
                c = Coff(p)
                s = c.secciones[0]
                trozos.append((int(m.group(1), 16), c.d[s['praw']:s['praw'] + s['tam']]))
        self.memoria = Memoria(trozos)
        for nombre, a in (('__real@c7c35000', 0x820B83EC), ('??_C@_0BA@BDHKIADM@AIGoalHeadOnRam?$AA@', 0x82034D28)):
            assert self.memoria.comprueba(a, nombre) is True, nombre  # control de la lectura de datos

    def dir_de(self, nombre):
        m = self.SIN_NOMBRE.match(nombre)
        return int(m.group(2), 16) if m else None

    def es_funcion_sin_nombre(self, nombre):
        return nombre.startswith('fn_') and self.dir_de(nombre) is not None

    def compatible(self, f, n):
        return True

    def falso(self, a):
        return 'fn_%08X' % a

    def unidad_de(self, a):
        for ini, fin, u in self.rangos:
            if ini <= a < fin:
                return u
        return None

    # Los destinos de datos que el troceado LTCG (scripts/ltcg360.py) no sabe nombrar salen como ltcg_XXXXXXXX (o
    # ltcg_lo_XXXXXXXX): son anonimos, no un nombre que contradiga al del original.
    SIN_NOMBRE_LTCG = re.compile(r'^ltcg_(lo_)?[0-9A-F]{8}$')

    def valido(self, nombre):
        return (nombre and not nombre.startswith(('.', '$', '@', '__LINE__')) and not self.SIN_NOMBRE.match(nombre)
                and not self.SIN_NOMBRE_LTCG.match(nombre) and len(nombre) <= MAX_NOMBRE and ' ' not in nombre)

    def esqueleto(self, pal, rel):
        out = []
        for i, w in enumerate(pal):
            r = rel.get(i)
            if r is not None:
                t = r[0]
                if t == PPC_REL24:
                    w &= 0xFC000003
                elif t == PPC_ADDR32:
                    w = 0
                else:
                    w &= 0xFFFF0000
                out.append((w, t))
            else:
                out.append((w, 0))
        return tuple(out)

    def destino(self, ts, po, ps, i, a):
        """Direccion del simbolo NUESTRO segun el original (a = direccion del destino sin nombre), o None."""
        if ts in (PPC_REFHI, PPC_REFLO):
            if ps[i] & 0xFFFF:
                return None  # el nuestro apunta a simbolo+k
            if ts == PPC_REFLO and (po[i] & 0xFFFF) != (a & 0xFFFF):
                return None
            if ts == PPC_REFHI and (po[i] & 0xFFFF) != (((a + 0x8000) >> 16) & 0xFFFF):
                return None
            return a
        return a if ts == PPC_REL24 else None

    def dato_con_nombre(self, no, ns, ts, po, ps, i):
        """Destino con nombre (de datos) en el original y otro en el nuestro que dice su contenido: True/False por los
        bytes; None si no se puede decidir (o ya lo decide la otra mitad del par lis/addi)."""
        dirs = [x for x in self.por_nombre.get(no, []) if x not in self.texto]
        if len(dirs) != 1 or ps[i] & 0xFFFF or self.memoria.comprueba(dirs[0], ns) is None:
            return None
        if ts == PPC_REFHI:
            return 'nada'
        if ts != PPC_REFLO:
            return None
        base = dirs[0]
        absoluta = base + (((po[i] & 0xFFFF) - base) & 0xFFFF)
        if (absoluta - base) & 0x8000:
            absoluta -= 0x10000
        return bool(self.memoria.comprueba(absoluta, ns))

    def nombre_libre(self, n, a):
        """360: el nombre no puede estar ya en la misma unidad (en otra si: copias por unidad)."""
        u = self.unidad_de(a)
        return not [x for x in self.por_nombre.get(n, []) if self.unidad_de(x) == u]

    def aplicar(self, finales):
        for f, n in finales.items():
            i = [e for e in self.por_dir[self.dir_de(f)] if e[0] == f and e[1] == '.text'][0][2]
            l = self.lineas[i]
            assert l.startswith(f + ' = .text:'), l
            self.lineas[i] = n + l[len(f):]
        entero(os.path.join(RAIZ, 'config', self.V, 'symbols.txt'), 'w', encoding='utf-8', newline='\n').write(
            '\n'.join(self.lineas))
        return len(finales)


class PC:
    V = 'SPEED_EXE_1_3'
    PASO = 1
    MIN = 8
    CORTA = 16
    LLAMADAS = (I386_REL32,)
    GUARDAR = ()
    # marcadores de IDA: funciones y datos sin nombre
    SIN_NOMBRE = re.compile(r'^(sub|nullsub|j_sub|unknown_libname|loc|locret|dword|byte|word|off|unk|flt|dbl|stru|'
                            r'asc|qword|xmmword|tbyte|jpt|def)_[0-9A-Fa-f]+$')
    FUNCION_SIN_NOMBRE = re.compile(r'^(sub|nullsub|j_sub|unknown_libname)_[0-9A-Fa-f]+$')

    def __init__(self):
        self.ruta = os.path.join(RAIZ, 'config', self.V, 'symbols.json')
        d = json.load(open(self.ruta, encoding='utf-8'))
        self.por_nombre = collections.defaultdict(list)
        self.funciones = {}
        self.tamanos = {}
        for f in d['functions']:
            self.por_nombre[f['name']].append(f['start'])
            self.funciones[f['name']] = f['start']
            self.tamanos[f['name']] = f['end'] - f['start']
        for x in d['names']:
            if x['addr'] not in self.por_nombre[x['name']]:
                self.por_nombre[x['name']].append(x['addr'])
        s = json.load(open(os.path.join(RAIZ, 'config', self.V, 'splits.json'), encoding='utf-8'))
        self.rangos = sorted((ini, fin, u[:-4]) for u, v in s.items() for ini, fin in v.get('function_ranges', []))
        pe = open(os.path.join(RAIZ, 'orig', self.V, 'speed.exe'), 'rb').read()
        e = struct.unpack_from('<I', pe, 0x3c)[0]
        nsec = struct.unpack_from('<H', pe, e + 6)[0]
        opt = struct.unpack_from('<H', pe, e + 20)[0]
        base = struct.unpack_from('<I', pe, e + 24 + 28)[0]
        trozos = []
        for i in range(nsec):
            nombre, vsz, va, rsz, raw = struct.unpack_from('<8sIIII', pe, e + 24 + opt + i * 40)
            if nombre.rstrip(b'\0') in (b'.rdata', b'.data'):
                trozos.append((base + va, pe[raw:raw + min(rsz, vsz)]))
        self.memoria = Memoria(trozos)

    # nombres que MSVC no puede emitir (el ejecutable es de MSVC 7.1): demangle de IDA (`FEListBox::SetCellScale`,
    # firmas de biblioteca como `Concurrency::details::...`), decoracion de GCC traida de GameCube
    # (`Default__14PlayerSettings`) o con la direccion pegada (`BindFunction__0061A370`). Como destino de una
    # reubicacion valen lo mismo que un sub_: no pueden coincidir con el nuestro, pero su direccion si.
    RARO = re.compile(r'(::|__[0-9A-F]{8}$|^[A-Za-z_][A-Za-z0-9_]*__(?:\d|F|C|Q\d|t\d|H\d)|^\$SG\d+$)')

    def dir_de(self, nombre):
        if not (self.SIN_NOMBRE.match(nombre) or self.RARO.search(nombre)):
            return None
        ds = self.por_nombre.get(nombre, [])
        return ds[0] if len(ds) == 1 else None

    def es_funcion_sin_nombre(self, nombre):
        return ((bool(self.FUNCION_SIN_NOMBRE.match(nombre)) or bool(self.RARO.search(nombre)))
                and nombre in self.funciones and self.dir_de(nombre) is not None)

    def compatible(self, f, n):
        """Un sub_ admite cualquier nombre; un nombre raro, solo el nuestro que diga la misma funcion."""
        return bool(self.FUNCION_SIN_NOMBRE.match(f)) or mismo_nombre_de_pila(f, n)

    def falso(self, a):
        n = 'sub_%X' % a
        self.por_nombre[n] = [a]
        self.funciones[n] = a
        return n

    def unidad_de(self, a):
        k = bisect.bisect_right(self.rangos, (a, 1 << 40, '')) - 1
        if k >= 0 and self.rangos[k][0] <= a < self.rangos[k][1]:
            return self.rangos[k][2]
        return None

    def valido(self, nombre):
        return (nombre and not nombre.startswith(('.', '$', '@', '__LINE__')) and not self.SIN_NOMBRE.match(nombre)
                and ' ' not in nombre and '"' not in nombre and '\\' not in nombre)

    def esqueleto(self, pal, rel):
        b = list(pal)
        for i, (t, n) in rel.items():
            for j in range(i, min(i + 4, len(b))):
                b[j] = -1
        return (tuple(b), tuple(sorted((i, t) for i, (t, n) in rel.items())))

    def destino(self, ts, po, ps, i, a):
        k = struct.unpack_from('<i', bytes(ps[i:i + 4]))[0] if i + 4 <= len(ps) else None
        if k != 0:
            return None  # el nuestro apunta a simbolo+k
        return a if ts in (I386_REL32, I386_DIR32) else None

    def dato_con_nombre(self, no, ns, ts, po, ps, i):
        dirs = self.por_nombre.get(no, [])
        if len(dirs) != 1 or ts != I386_DIR32:
            return None
        k = struct.unpack_from('<i', bytes(ps[i:i + 4]))[0] if i + 4 <= len(ps) else None
        if k != 0:
            return None
        v = self.memoria.comprueba(dirs[0], ns)
        return None if v is None else bool(v)

    def nombre_libre(self, n, a):
        """PC: cada COMDAT una vez; el nombre no puede estar en symbols.json."""
        return n not in self.por_nombre

    def aplicar(self, finales):
        b = open(self.ruta, 'rb').read().decode('utf-8')
        cambios = 0
        from nombra_elim import vetados  # noqa: E402 (nombra_elim importa este modulo)
        vet = vetados()
        for f, n in finales.items():
            if any(a in vet for a in self.por_nombre.get(f, [])):
                print('  VETADA', f[:80], '->', n[:80])
                continue
            # el nombre viejo puede estar en `functions` y en `names` (la misma direccion): se cambian los dos
            antes = '"name": "%s",' % json.dumps(f)[1:-1]
            if 1 <= b.count(antes) <= 2 and len(self.por_nombre.get(f, [])) == 1:
                b = b.replace(antes, '"name": "%s",' % json.dumps(n)[1:-1])
                cambios += 1
        entero(self.ruta, 'wb').write(b.encode('utf-8'))
        return cambios


class Unidad:
    def __init__(self, P, nombre, po, ps):
        self.nombre = nombre
        self.fo = Coff(po).funciones(getattr(P, 'tamanos', None))
        cs = Coff(ps)
        self.fs = cs.funciones()
        self.indice = {}  # orden de definicion en nuestro objeto
        for k, s in enumerate(cs.lista):
            self.indice.setdefault(s[0], k)
        self.eo = {n: P.esqueleto(*v) for n, v in self.fo.items()}
        self.es = {n: P.esqueleto(*v) for n, v in self.fs.items()}


def main():
    args = sys.argv[1:]
    posic = [a for i, a in enumerate(args) if not a.startswith('--') and (i == 0 or args[i - 1] != '--tsv')]
    V = posic[0] if posic else 'EUROPEGERMILESTONE'
    P = {'EUROPEGERMILESTONE': X360, 'SPEED_EXE_1_3': PC}[V]()
    tsv = args[args.index('--tsv') + 1] if '--tsv' in args else None
    detalle = '--detalle' in args
    base_o = os.path.join(RAIZ, 'build', V, 'obj')
    base_s = os.path.join(RAIZ, 'build', V, 'src')
    for a in args:
        if a.startswith('--nuestros='):
            base_s = os.path.join(RAIZ, a.split('=', 1)[1])
    unidades = []
    por_u = {}
    for p in sorted(glob.glob(os.path.join(base_s, '**', '*.obj'), recursive=True)):
        rel = os.path.relpath(p, base_s)
        o = os.path.join(base_o, rel)
        if os.path.exists(o):
            u = Unidad(P, rel.replace(os.sep, '/')[:-4], o, p)
            unidades.append(u)
            por_u[u.nombre] = u
    dir_de = P.dir_de

    # --validar F: esconde una fraccion F de las anclas (les pone un nombre de marcador en su direccion) y mide si el
    # metodo les devuelve SU nombre: la precision real de cada motivo y tamano, sin fiarse de la intuicion
    verdad = {}
    if '--validar' in args:
        import random
        random.seed(13)
        frac = float(args[args.index('--validar') + 1])
        for u in unidades:
            for n in list(u.fo):
                if n not in u.fs or u.es[n] != u.eo.get(n):
                    continue
                ds = [x for x in P.por_nombre.get(n, []) if P.unidad_de(x) == u.nombre]
                if len(ds) != 1 or random.random() >= frac:
                    continue
                falso = P.falso(ds[0])
                if falso in u.fo:
                    continue
                u.fo[falso] = u.fo.pop(n)
                u.eo[falso] = u.eo.pop(n)
                P.por_nombre[n] = [x for x in P.por_nombre[n] if x != ds[0]]
                if not P.por_nombre[n]:
                    del P.por_nombre[n]
                verdad[falso] = n
        print('validacion: %d anclas escondidas' % len(verdad))

    evid = collections.defaultdict(collections.Counter)  # direccion sin nombre -> {nombre nuestro: testigos}
    asignado = {}      # (unidad, nombre) -> direccion
    nombre_de = {}     # direccion -> nombre decidido (parejas aceptadas)

    def compara(po, ro, ps, rs):
        """(contradicciones, coincidencias, [(direccion, nombre nuestro)])"""
        contra = acuerdo = 0
        ev = []
        del motivos[:]
        for i, (ts, ns) in rs.items():
            to, no = ro[i]
            if equivalentes(no, ns):
                acuerdo += 1
                continue
            a = dir_de(no)
            if a is None:
                if not P.valido(ns):
                    continue
                v = P.dato_con_nombre(no, ns, ts, po, ps, i)
                if v == 'nada':
                    continue
                if v is True:
                    acuerdo += 1
                else:
                    contra += 1  # destino con nombre en el original y otro en el nuestro (o bytes distintos)
                    motivos.append((i, no, ns, 'nombre distinto' if v is None else 'bytes distintos'))
                continue
            if not P.valido(ns):
                continue
            a = P.destino(ts, po, ps, i, a)
            if a is None:
                continue
            valor = P.memoria.comprueba(a, ns)
            if valor is not None:
                if valor:
                    acuerdo += 1  # la constante o la cadena del original es la del nombre
                else:
                    contra += 1
                    motivos.append((i, no, ns, 'contenido distinto'))
                continue
            ya = nombre_de.get(a)
            if ya is not None:
                if ya == ns:
                    acuerdo += 1
                else:
                    contra += 1
                    motivos.append((i, no, ns, 'la direccion ya es %s' % ya))
                continue
            b = asignado.get((P.unidad_de(a), ns))
            if b is not None and b != a:
                contra += 1  # ese nombre ya esta en otra direccion de la misma unidad
                motivos.append((i, no, ns, 'el nombre ya esta en 0x%08X' % b))
                continue
            e = evid.get(a)
            if e and len(e) == 1:
                if ns in e:
                    acuerdo += 1  # los testigos dicen este mismo nombre
                    continue
                if next(iter(e.values())) >= 2 or a in de_ancla:
                    contra += 1  # dos testigos, o un ancla, dicen otro nombre
                    motivos.append((i, no, ns, 'los testigos dicen %s' % dict(e)))
                    continue
            ev.append((a, ns))
        return contra, acuerdo, ev

    de_ancla = set()   # direcciones con algun testigo que es un ancla (nombre ya puesto y mismo esqueleto)
    motivos = []       # por que contradice la ultima pareja comparada: (posicion, destino original, nuestro, motivo)

    def anota(ev, ancla=False):
        for a, ns in ev:
            evid[a][ns] += 1
            if ancla:
                de_ancla.add(a)

    n_anclas = 0
    anclas_malas = []
    for u in unidades:
        for n, e in u.eo.items():
            if n in u.fs and u.es[n] == e:
                n_anclas += 1
                c, _, ev = compara(u.fo[n][0], u.fo[n][1], u.fs[n][0], u.fs[n][1])
                if not c:
                    anota(ev, ancla=True)
                else:
                    anclas_malas.append((u.nombre, n, c, list(motivos)))
    print('anclas (mismo nombre y esqueleto): %d; direcciones con evidencia: %d' % (n_anclas, len(evid)))

    # testigos mas debiles: funciones con el MISMO nombre en los dos lados pero codigo distinto. Se alinean sus llamadas
    # por las que tienen nombre en el original (subsecuencia comun mas larga, y al menos una que no sea de guardar o
    # restaurar registros); en cada hueco entre dos alineadas con el mismo numero de llamadas sin nombre en el original
    # que en el nuestro, la k-esima del original es la k-esima nuestra. Solo desempatan (motivo `secuencia`).
    evid_sec = collections.defaultdict(collections.Counter)
    n_sec = 0

    def conocido(x):
        return dir_de(x) is None

    def alinea(lo, ls):
        """Parejas (i, j) de la subsecuencia comun mas larga de las llamadas con NOMBRE (las sin nombre no alinean)."""
        n1, n2 = len(lo), len(ls)
        t = [[0] * (n2 + 1) for _ in range(n1 + 1)]
        for i in range(n1 - 1, -1, -1):
            for j in range(n2 - 1, -1, -1):
                if conocido(lo[i]) and equivalentes(lo[i], ls[j]):
                    t[i][j] = t[i + 1][j + 1] + 1
                else:
                    t[i][j] = max(t[i + 1][j], t[i][j + 1])
        out = []
        i = j = 0
        while i < n1 and j < n2:
            if conocido(lo[i]) and equivalentes(lo[i], ls[j]):
                out.append((i, j))
                i += 1
                j += 1
            elif t[i + 1][j] >= t[i][j + 1]:
                i += 1
            else:
                j += 1
        return out

    for u in unidades:
        for n in u.fo:
            if n not in u.fs or u.es[n] == u.eo[n]:
                continue
            ro, rs = u.fo[n][1], u.fs[n][1]
            lo = [ro[i][1] for i in sorted(ro) if ro[i][0] in P.LLAMADAS]
            ls = [rs[i][1] for i in sorted(rs) if rs[i][0] in P.LLAMADAS]
            if not lo or not ls or len(lo) * len(ls) > 40000:
                continue
            pares = alinea(lo, ls)
            if not [1 for i, _ in pares if not (P.GUARDAR and lo[i].startswith(P.GUARDAR))]:
                continue  # sin una llamada con nombre en comun (que no sea de guardar registros) no hay ancla
            # entre dos llamadas alineadas, si el hueco del original solo tiene destinos SIN nombre y mide lo mismo
            # que el nuestro, la k-esima del original es la k-esima nuestra
            ev = []
            cortes = [(-1, -1)] + pares + [(len(lo), len(ls))]
            for (i0, j0), (i1, j1) in zip(cortes, cortes[1:]):
                hueco_o = lo[i0 + 1:i1]
                hueco_s = ls[j0 + 1:j1]
                if not hueco_o or len(hueco_o) != len(hueco_s) or any(conocido(x) for x in hueco_o):
                    continue
                for x, y in zip(hueco_o, hueco_s):
                    if P.valido(y):
                        ev.append((dir_de(x), y))
            if ev:
                n_sec += 1
                for a, y in ev:
                    evid_sec[a][y] += 1
    print('testigos por secuencia de llamadas: %d funciones, %d direcciones' % (n_sec, len(evid_sec)))

    grupos = []
    for u in unidades:
        por_esq = collections.defaultdict(lambda: ([], []))
        for n, e in u.eo.items():
            if P.es_funcion_sin_nombre(n):
                por_esq[e][0].append(n)
        for n, e in u.es.items():
            if P.valido(n) and n not in u.fo and e in por_esq:
                por_esq[e][1].append(n)
        for e, (fs_, ns_) in por_esq.items():
            if ns_:
                grupos.append((u, fs_, ns_))
    print('grupos de esqueleto con candidatas: %d (sin nombre %d, nuestras %d)'
          % (len(grupos), sum(len(g[1]) for g in grupos), sum(len(g[2]) for g in grupos)))

    # orden: anclas vecinas en el original (direccion) y en nuestro objeto (indice de simbolo)
    vecinas = {}
    for u in unidades:
        lista = []
        for n in u.fo:
            if n in u.fs and u.es[n] == u.eo[n] and n in u.indice:
                ds = [x for x in P.por_nombre.get(n, []) if P.unidad_de(x) == u.nombre]
                if len(ds) == 1:
                    lista.append((ds[0], u.indice[n]))
        vecinas[u.nombre] = sorted(lista)

    def en_orden(u, f, n):
        lista = vecinas[u.nombre]
        k = bisect.bisect_left(lista, (dir_de(f), -1))
        antes = lista[k - 1][1] if k > 0 else -1
        despues = lista[k][1] if k < len(lista) else 1 << 30
        return antes < u.indice.get(n, -1) < despues

    def tam(u, f):
        return P.PASO * len(u.fo[f][0])

    aceptadas = {}  # sin nombre -> (unidad, nombre, motivo, bytes, coincidencias, reubicaciones)
    for vuelta in range(20):
        nuevas = 0
        for u, fs_, ns_ in grupos:
            libres_f = [f for f in fs_ if f not in aceptadas]
            libres_n = [n for n in ns_ if (u.nombre, n) not in asignado]
            if not libres_f or not libres_n:
                continue
            compat = {}
            for f in libres_f:
                po, ro = u.fo[f]
                for n in libres_n:
                    ps, rs = u.fs[n]
                    c, acu, ev = compara(po, ro, ps, rs)
                    if not c:
                        compat[(f, n)] = (acu, ev)
            elegidas = []
            for f in libres_f:
                a = dir_de(f)
                cn = [n for n in libres_n if (f, n) in compat and P.compatible(f, n)]
                if not cn:
                    continue
                e = evid.get(a)
                if e:
                    if len(e) == 1:
                        n = next(iter(e))
                        if n in cn:
                            elegidas.append((f, n, 'llamantes'))
                    continue  # evidencia partida: no se decide
                e = evid_sec.get(a)
                if e and len(cn) > 1:
                    if len(e) == 1:
                        n = next(iter(e))
                        if n in cn and not [g for g in libres_f if g != f and evid_sec.get(dir_de(g), {}).get(n)]:
                            elegidas.append((f, n, 'secuencia'))
                    continue
                if len(cn) != 1:
                    # varias compatibles: gana la que tiene MAS destinos confirmados si es la unica con ese maximo,
                    # y si en el otro sentido esta fn_ tambien es la mejor para ella
                    puntos = sorted(((compat[(f, n)][0], n) for n in cn), reverse=True)
                    if puntos[0][0] == 0 or puntos[0][0] == puntos[1][0]:
                        continue
                    n = puntos[0][1]
                    rivales = [compat[(g, n)][0] for g in libres_f if g != f and (g, n) in compat and P.compatible(g, n)]
                    if rivales and max(rivales) >= puntos[0][0]:
                        continue
                    elegidas.append((f, n, 'mejor'))
                    continue
                n = cn[0]
                if len([g for g in libres_f if (g, n) in compat and P.compatible(g, n)]) != 1:
                    continue
                if tam(u, f) < P.MIN and compat[(f, n)][0] == 0:
                    continue
                if tam(u, f) < P.CORTA and compat[(f, n)][0] == 0 and not en_orden(u, f, n):
                    continue  # corta y sin ningun destino que la confirme: que al menos este donde toca
                elegidas.append((f, n, 'unica'))
            vistos = collections.Counter(n for _, n, _ in elegidas)
            for f, n, motivo in elegidas:
                if vistos[n] > 1 or (u.nombre, n) in asignado or not P.nombre_libre(n, dir_de(f)):
                    continue
                if not isinstance(P, X360) and n in {v[1] for v in aceptadas.values()}:
                    continue  # PC: un nombre, una direccion
                aceptadas[f] = (u.nombre, n, motivo, tam(u, f), compat[(f, n)][0], len(u.fo[f][1]))
                asignado[(u.nombre, n)] = dir_de(f)
                nombre_de[dir_de(f)] = n
                anota(compat[(f, n)][1])
                nuevas += 1
        print('vuelta %d: %d parejas nuevas (total %d)' % (vuelta, nuevas, len(aceptadas)), flush=True)
        if not nuevas:
            break

    # segunda lectura con TODOS los nombres ya decididos: una pareja que ahora contradice a otra se cae (y se repite)
    while True:
        caidas = []
        for f, v in aceptadas.items():
            u = por_u[v[0]]
            c, acu, _ = compara(u.fo[f][0], u.fo[f][1], u.fs[v[1]][0], u.fs[v[1]][1])
            if c:
                caidas.append(f)
            else:
                aceptadas[f] = v[:4] + (acu,) + v[5:]
        for f in caidas:
            un, n = aceptadas[f][:2]
            del aceptadas[f]
            del asignado[(un, n)]
            del nombre_de[dir_de(f)]
        print('segunda lectura: %d parejas caen' % len(caidas))
        if not caidas:
            break

    # lo que queda ambiguo (para el informe)
    pend = collections.Counter()
    pend_b = collections.Counter()
    for u, fs_, ns_ in grupos:
        libres_f = [f for f in fs_ if f not in aceptadas]
        libres_n = [n for n in ns_ if (u.nombre, n) not in asignado]
        for f in libres_f:
            cn = [n for n in libres_n if not compara(u.fo[f][0], u.fo[f][1], u.fs[n][0], u.fs[n][1])[0]]
            if not cn:
                continue
            clase_ = 'una candidata' if len(cn) == 1 else 'varias candidatas'
            if tam(u, f) < P.MIN:
                clase_ += ' (corta)'
            pend[clase_] += 1
            pend_b[clase_] += tam(u, f)
    print('pendientes (sin nombre con candidatas compatibles sin decidir): %s  bytes %s' % (dict(pend), dict(pend_b)))

    mot = collections.Counter(v[2] for v in aceptadas.values())
    print('FINALES: %d funciones, %d B  %s' % (len(aceptadas), sum(v[3] for v in aceptadas.values()), dict(mot)))
    por_unidad = collections.Counter(v[0].split('/')[-1] for v in aceptadas.values())
    print('  por unidad:', dict(por_unidad.most_common()))
    if detalle:
        conflictos = [(a, dict(e)) for a, e in evid.items() if len(e) > 1 and P.unidad_de(a)]
        print('  direcciones de SourceLists con evidencia partida:', len(conflictos))
        for a, e in conflictos[:25]:
            print('    0x%08X %s' % (a, e))
    print('anclas (nombre YA puesto) con destinos que contradicen al nuestro: %d' % len(anclas_malas))
    if '--anclas-malas' in args:
        for un, n, c, mot in anclas_malas:
            print('  ANCLA %s %s (%d)' % (un.split('/')[-1], n, c))
            if detalle:
                for i, no, ns, m in mot:
                    print('      +%d: original %s | nuestro %s | %s' % (i * P.PASO, no, ns, m))
    if tsv:
        with open(tsv, 'w', encoding='utf-8', newline='\n') as fh:
            for f, (un, n, motivo, b, acu, nrel) in sorted(aceptadas.items()):
                fh.write('%s\t%s\t%s\t%s\t%d\t%d\t%d\n' % (f, n, un.split('/')[-1], motivo, b, acu, nrel))
    if verdad:
        res = collections.Counter()
        for f, v in aceptadas.items():
            if f in verdad:
                clase_ = (v[2], '<16' if v[3] < 16 else '<32' if v[3] < 32 else '<64' if v[3] < 64 else '>=64',
                          'acu>0' if v[4] else ('sin reubicaciones' if not v[5] else 'acu0'))
                res[clase_ + ('BIEN' if v[1] == verdad[f] else 'MAL',)] += 1
        clases = sorted({k[:3] for k in res})
        print('VALIDACION: escondidas %d, nombradas %d, bien %d, mal %d'
              % (len(verdad), sum(res.values()), sum(v for k, v in res.items() if k[3] == 'BIEN'),
                 sum(v for k, v in res.items() if k[3] == 'MAL')))
        for k in clases:
            print('   %-40s bien %5d  mal %4d' % (k, res[k + ('BIEN',)], res[k + ('MAL',)]))
        if '--detalle' in args:
            for f, v in aceptadas.items():
                if f in verdad and v[1] != verdad[f]:
                    print('   MAL %s %s: puso %s, era %s' % (v[0].split('/')[-1], f, v[1][:60], verdad[f][:60]))
        return
    if '--aplicar' in args:
        print('aplicados', P.aplicar({f: v[1] for f, v in aceptadas.items()}))


if __name__ == '__main__':
    main()
