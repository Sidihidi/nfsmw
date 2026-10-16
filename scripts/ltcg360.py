#!/usr/bin/env python3
"""ltcg360.py [--ninja FICHERO] [--hilos N] [--fallidos A,B] [--solo-trocear] -- la medida de Xbox 360 sobre un
enlace LTCG del arbol entero, como se hizo el original.

El ejecutable de 360 (EUROPEGERMILESTONE) se enlazo con generacion de codigo en el enlazador (cl /GL + link /LTCG):
su Rich header lleva 369 objetos `Utc1400_LTCG_CPP` (prodid 0x72) build 2110, y nuestro link 8.00.2110 solo escribe
ese prodid cuando enlaza con /LTCG. LTCG expande funciones entre unidades, especializa parametros y reparte registros
segun el programa entero; compilando unidad a unidad hay funciones que no pueden salir. Esto las reproduce:

  1. IL: cada unidad que build.ninja compila con la regla `msvc` se vuelve a compilar con los MISMOS flags y /GL en
     build/EUROPEGERMILESTONE/ltcg/il/. Solo las que cambiaron (el objeto de ninja es mas nuevo que el IL).
  2. Stubs: lo que nuestro arbol aun no define (funciones sin escribir, la CRT y el XDK, que no tenemos) se define en
     un objeto COFF escrito aqui (ltcg/stubs.obj), SIN /GL: opaco para LTCG, que lo trata como una llamada externa.
     `/FORCE:UNRESOLVED` no sirve con /LTCG (LNK1120 fatal). La lista se guarda (ltcg/stubs.txt) y se completa
     enlazando otra vez mientras queden LNK2001/LNK2019.
  3. Enlace: link /LTCG /XEX:NO /NODEFAULTLIB /FORCE:MULTIPLE /OPT:NOREF /MAP (ltcg/NFS_ltcg.exe y .map).
  4. Troceado: el .text enlazado vuelve a un objeto COFF por unidad (ltcg/obj/<misma ruta que build/<V>/src>), con
     el objeto de cada funcion que dice el .map y sus reubicaciones: REL24 para cada b/bl que sale de la funcion,
     REFHI+PAIR / REFLO para los pares HIGHADJ/LOW de .reloc y ADDR32 para los HIGHLOW. Los campos reubicados van a
     cero, como en un objeto de cl. objdiff los compara con build/<V>/obj como siempre.

  5. Normalizacion (decision del usuario, 2026-10-07): en los dos lados, las cargas del pool de constantes se comparan
     por VALOR y las tablas de saltos por DESTINO (ver `Normalizacion`). Los originales normalizados van a
     ltcg/orig/ (misma ruta que build/<V>/obj) y son el target_path del informe LTCG; build/<V>/obj no se toca.

pctall.py lo llama para EUROPEGERMILESTONE: la medida oficial (build/versiones/EUROPEGERMILESTONE.json) sale de
ltcg/obj y la de fichero a fichero queda como diagnostico (EUROPEGERMILESTONE-fichero.json).

Limites, medidos el 2026-10-07: el orden de las funciones dentro de una unidad lo decide LTCG con el programa entero
(no se reproduce: se compara por nombre); y con el arbol incompleto LTCG especializa de mas, porque no ve a los
llamantes que faltan (209 funciones / 14 kB casaban fichero a fichero y no con LTCG; 3.190 / 350 kB al reves).

    python scripts/ltcg360.py                       # con build.ninja configurado para EUROPEGERMILESTONE
    python scripts/ltcg360.py --solo-trocear        # rehace solo el troceado del ejecutable que ya haya
"""
import argparse
import collections
import os
import re
import struct
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor

V = 'EUROPEGERMILESTONE'
CL = os.path.join('build', 'compilers', 'X360', '14.00.2110', 'cl.exe')
LINK = os.path.join('build', 'compilers', 'X360', '14.00.2110', 'link.exe')
PPC = 0x01F2
R_ADDR32, R_REL24, R_REFHI, R_REFLO, R_PAIR = 0x0002, 0x0006, 0x0010, 0x0011, 0x0012


def rutas(raiz):
    base = os.path.join(raiz, 'build', V, 'ltcg')
    return {'base': base, 'il': os.path.join(base, 'il'), 'obj': os.path.join(base, 'obj'),
            'exe': os.path.join(base, 'NFS_ltcg.exe'), 'map': os.path.join(base, 'NFS_ltcg.map'),
            'log': os.path.join(base, 'link.log'), 'stubs': os.path.join(base, 'stubs.txt'),
            'stubobj': os.path.join(base, 'stubs.obj'), 'rsp': os.path.join(base, 'link.rsp'),
            'orig': os.path.join(base, 'orig')}


# --------------------------------------------------------------------------------------------- build.ninja

def unidades_ninja(ruta):
    """[(objeto de ninja, fuente, cflags)] de las reglas `msvc` que compilan en build/<V>/src."""
    texto = open(ruta, encoding='utf-8', errors='replace').read().replace('$\r\n', '').replace('$\n', '')
    out = []
    bloque = None
    for linea in texto.splitlines():
        if linea.startswith('build '):
            m = re.match(r'build\s+(\S+):\s+msvc\s+(\S+)', linea)
            bloque = None
            if m and m.group(1).replace('\\', '/').startswith('build/%s/src/' % V):
                bloque = [m.group(1), m.group(2), None]
                out.append(bloque)
        elif bloque is not None and linea.startswith('  cflags = '):
            bloque[2] = re.sub(r'\s+', ' ', linea[len('  cflags = '):]).strip()
        elif not linea.startswith('  '):
            bloque = None
    return [(o.replace('\\', '/'), s.replace('\\', '/'), c) for o, s, c in out if c]


def nombre_il(obj):
    """build/<V>/src/Speed/Indep/SourceLists/zAI.obj -> zAI.obj (el .map de link.exe solo da el nombre base)."""
    return os.path.basename(obj)


def compila_il(raiz, unidades, fallidos, hilos, avisa):
    r = rutas(raiz)
    os.makedirs(r['il'], exist_ok=True)
    nombres = [nombre_il(o) for o, _, _ in unidades]
    assert len(set(nombres)) == len(nombres), 'dos unidades con el mismo nombre de objeto: %s' % nombres

    def una(u):
        obj, src, cflags = u
        il = os.path.join(r['il'], nombre_il(obj))
        ninja_obj = os.path.join(raiz, obj)
        if os.path.normpath(obj) in fallidos or not os.path.exists(ninja_obj):
            if os.path.exists(il):
                os.remove(il)
            return obj, 'sin objeto de ninja'
        if os.path.exists(il) and os.path.getmtime(il) >= os.path.getmtime(ninja_obj):
            return obj, None
        if os.path.exists(il):
            os.remove(il)
        cmd = '"%s" %s /GL /Fo%s %s' % (os.path.join(raiz, CL), cflags, il, src.replace('/', '\\'))
        for _ in range(3):  # C1083 con mucha carga, como en pctall
            p = subprocess.run(cmd, cwd=raiz, capture_output=True, text=True, errors='replace')
            if p.returncode == 0 and os.path.exists(il):
                return obj, None
            if 'C1083' not in p.stdout:
                break
        errs = [x for x in p.stdout.splitlines() if 'error' in x]
        return obj, 'no compila con /GL: ' + ' | '.join(errs[:2])

    malos = {}
    with ThreadPoolExecutor(hilos) as ex:
        for obj, err in ex.map(una, unidades):
            if err:
                malos[obj] = err
                avisa('ltcg360: %s: %s' % (obj, err))
    return [u for u in unidades if u[0] not in malos], malos


# --------------------------------------------------------------------------------------------- COFF

def escribe_coff(ruta, secciones, simbolos):
    """secciones: [(nombre, datos, [(offset, indice de simbolo, tipo)], caracteristicas)];
    simbolos: [(nombre, valor, seccion 1.., tipo, clase)]. Cabeceras little-endian (los datos, big-endian, van tal
    cual). Sin simbolos auxiliares: el indice de un simbolo es su posicion."""
    cadenas = bytearray(b'\0\0\0\0')
    tabla = bytearray()
    for nombre, valor, sec, tipo, clase in simbolos:
        b = nombre.encode('latin-1')
        if len(b) <= 8:
            n = b.ljust(8, b'\0')
        else:
            n = struct.pack('<II', 0, len(cadenas))
            cadenas += b + b'\0'
        tabla += n + struct.pack('<IhHBB', valor, sec, tipo, clase, 0)
    struct.pack_into('<I', cadenas, 0, len(cadenas))
    cab = 20 + 40 * len(secciones)
    pos = cab
    cuerpos = bytearray()
    hdrs = bytearray()
    for nombre, datos, rels, car in secciones:
        p_datos = pos
        cuerpos += datos
        pos += len(datos)
        p_rel = pos if rels else 0
        for off, idx, tipo in rels:
            cuerpos += struct.pack('<IIH', off, idx, tipo)
        pos += 10 * len(rels)
        nrel = len(rels)
        if nrel > 0xFFFF:
            raise ValueError('mas de 65535 reubicaciones en una seccion')
        hdrs += nombre.encode().ljust(8, b'\0') + struct.pack('<IIIIIIHHI', 0, 0, len(datos), p_datos, p_rel, 0,
                                                              nrel, 0, car)
    d = struct.pack('<HHIIIHH', PPC, len(secciones), 0, pos, len(simbolos), 0, 0) + hdrs + cuerpos + tabla + cadenas
    tmp = ruta + '.tmp'
    open(tmp, 'wb').write(d)
    os.replace(tmp, ruta)


def definidos_coff(ruta):
    """Simbolos externos definidos de un objeto COFF de cl (para no poner stub a lo que ya existe)."""
    d = open(ruta, 'rb').read()
    if len(d) < 20 or struct.unpack_from('<H', d, 0)[0] != PPC:
        return set()
    _, nsec, _, psym, nsym, _, _ = struct.unpack_from('<HHIIIHH', d, 0)
    cad = psym + nsym * 18
    out = set()
    i = 0
    while i < nsym:
        o = psym + i * 18
        n = d[o:o + 8]
        if n[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', n, 4)[0]
            n = d[cad + off:d.index(b'\0', cad + off)]
        else:
            n = n.rstrip(b'\0')
        _, sec, _, clase, naux = struct.unpack_from('<IhHBB', d, o + 8)
        if sec > 0 and clase == 2:
            out.add(n.decode('latin-1'))
        i += 1 + naux
    return out


# --------------------------------------------------------------------------------------------- stubs y enlace

def es_dato(texto):
    """Del texto de un LNK2001: '"int Foo" (?Foo@@3HA)' es un dato; con convencion de llamada sin '(' delante es una
    funcion; un nombre C sin forma desmangada se trata como funcion."""
    m = re.match(r'"(.*)" \((\S+)\)$', texto)
    if not m:
        return False
    dem = m.group(1)
    return re.search(r'(?<![(])__(cdecl|thiscall|stdcall|fastcall|clrcall)\b', dem) is None


def sin_resolver(log):
    out = {}
    for linea in log.splitlines():
        m = re.search(r'error LNK20(?:01|19): unresolved external symbol (.*?)(?: referenced in .*)?$', linea)
        if not m:
            continue
        t = m.group(1).strip()
        m2 = re.match(r'"(.*)" \((\S+)\)$', t)
        nombre = m2.group(2) if m2 else t
        out[nombre] = 'D' if es_dato(t) else 'F'
    return out


def escribe_stubs(ruta, stubs):
    """Un objeto con cada stub definido: las funciones son un `blr` propio (4 B cada una, direcciones distintas para
    que el troceado sepa a quien llama cada bl) y los datos 16 B de .bss cada uno."""
    funcs = sorted(n for n, k in stubs.items() if k == 'F')
    datos = sorted(n for n, k in stubs.items() if k == 'D')
    texto = struct.pack('>I', 0x4E800020) * max(1, len(funcs))
    simbolos = [(n, 4 * i, 1, 0x20, 2) for i, n in enumerate(funcs)]
    simbolos += [(n, 16 * i, 2, 0, 2) for i, n in enumerate(datos)]
    secs = [('.text', texto, [], 0x60500020),
            ('.bss', b'', [], 0xC0500080)]
    # .bss: SizeOfRawData es el tamano, sin datos en el fichero
    escribe_coff(ruta, secs, simbolos)
    d = bytearray(open(ruta, 'rb').read())
    struct.pack_into('<II', d, 20 + 40 + 16, 16 * max(1, len(datos)), 0)
    open(ruta, 'wb').write(d)


def enlaza(raiz, ils, avisa, max_vueltas=6):
    r = rutas(raiz)
    stubs = {}
    if os.path.exists(r['stubs']):
        for linea in open(r['stubs'], encoding='latin-1'):
            k, _, n = linea.rstrip('\n').partition(' ')
            if n:
                stubs[n] = k
    # lo que nuestros objetos (los de ninja, COFF normales) ya definen no lleva stub
    nuestros = set()
    for obj, _, _ in ils:
        nuestros |= definidos_coff(os.path.join(raiz, obj))
    stubs = {n: k for n, k in stubs.items() if n not in nuestros}
    stubs.setdefault('main', 'F')
    for vuelta in range(max_vueltas):
        escribe_stubs(r['stubobj'], stubs)
        with open(r['rsp'], 'w') as fh:
            for obj, _, _ in ils:
                fh.write('"%s"\n' % os.path.join(r['il'], nombre_il(obj)))
            fh.write('"%s"\n' % r['stubobj'])  # el ultimo: con /FORCE:MULTIPLE gana la definicion nuestra
        for f in (r['exe'], r['map']):
            if os.path.exists(f):
                os.remove(f)
        t = time.time()
        p = subprocess.run([os.path.join(raiz, LINK), '/nologo', '/LTCG', '/XEX:NO', '/NODEFAULTLIB',
                            '/FORCE:MULTIPLE', '/OPT:NOREF', '/ENTRY:main', '/MAP:' + r['map'], '/OUT:' + r['exe'],
                            '@' + r['rsp']], cwd=raiz, capture_output=True, text=True, errors='replace')
        log = p.stdout + p.stderr
        open(r['log'], 'w', encoding='utf-8').write(log)
        nuevos = {n: k for n, k in sin_resolver(log).items() if n not in stubs}
        avisa('ltcg360: enlace %d: %.0f s, %d sin resolver nuevos' % (vuelta + 1, time.time() - t, len(nuevos)))
        if os.path.exists(r['exe']) and os.path.exists(r['map']) and not nuevos:
            with open(r['stubs'], 'w', encoding='latin-1') as fh:
                for n in sorted(stubs):
                    fh.write('%s %s\n' % (stubs[n], n))
            return True
        if not nuevos:
            avisa('ltcg360: el enlace fallo sin simbolos nuevos que definir; ver %s' % r['log'])
            return False
        stubs.update(nuevos)
    return False


# --------------------------------------------------------------------------------------------- troceado

class PE:
    def __init__(self, ruta):
        d = open(ruta, 'rb').read()
        self.d = d
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        o = pe + 24
        self.base = struct.unpack_from('<I', d, o + 28)[0]
        nsec = struct.unpack_from('<H', d, pe + 6)[0]
        optsz = struct.unpack_from('<H', d, pe + 20)[0]
        self.secs = []
        for i in range(nsec):
            s = o + optsz + i * 40
            n, vs, va, rs, pr = struct.unpack_from('<8sIIII', d, s)
            self.secs.append((n.rstrip(b'\0').decode('latin-1'), va, vs, pr, rs))
        self.dirs = [struct.unpack_from('<II', d, o + 96 + i * 8) for i in range(16)]

    def lee(self, rva, n):
        for _, va, vs, pr, rs in self.secs:
            if va <= rva < va + max(vs, rs):
                off = rva - va
                b = self.d[pr + off:pr + min(off + n, rs)]
                return b + b'\0' * (n - len(b))
        return b'\0' * n

    def seccion(self, nombre):
        return [s for s in self.secs if s[0] == nombre][0]

    def relocs(self):
        """{rva: (tipo, parametro)}; HIGHADJ (4) lleva detras una entrada con la mitad baja del destino."""
        rva, size = self.dirs[5]
        b = self.lee(rva, size)
        out = {}
        off = 0
        while off + 8 <= size:
            page, bs = struct.unpack_from('<II', b, off)
            if bs < 8:
                break
            k, n = 0, (bs - 8) // 2
            while k < n:
                e = struct.unpack_from('<H', b, off + 8 + 2 * k)[0]
                t = e >> 12
                if t == 4:
                    out[page + (e & 0xfff)] = (4, struct.unpack_from('<H', b, off + 8 + 2 * (k + 1))[0])
                    k += 2
                    continue
                if t:
                    out[page + (e & 0xfff)] = (t, 0)
                k += 1
            off += bs
        return out


def lee_mapa(ruta):
    """[(rva, nombre, objeto, es_funcion, estatico)] de todos los simbolos del .map de link.exe."""
    out = []
    estatico = False
    base = 0
    pat = re.compile(r'^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(f\s+)?(i\s+)?(\S+)\s*$')
    for linea in open(ruta, encoding='latin-1'):
        if 'Preferred load address is' in linea:
            base = int(linea.split()[-1], 16)
        elif linea.strip().startswith('Static symbols'):
            estatico = True
        m = pat.match(linea)
        if m and int(m.group(2), 16) >= base:
            out.append((int(m.group(2), 16) - base, m.group(1), m.group(5).split(':')[-1], bool(m.group(3)),
                        estatico))
    return out


def sext16(x):
    return x - 0x10000 if x & 0x8000 else x


def trocea(raiz, ils, avisa, normalizar=True):
    """Un objeto COFF por unidad con las funciones del ejecutable enlazado. Devuelve {objeto de ninja: troceado}.
    Con `normalizar`, tambien escribe los originales normalizados (ver normaliza_originales) y aplica la misma
    normalizacion a lo nuestro: cargas del pool por VALOR y tablas de saltos por DESTINO."""
    r = rutas(raiz)
    pe = PE(r['exe'])
    rel = pe.relocs()
    txt = pe.seccion('.text')
    t0, t1 = txt[1], txt[1] + txt[2]
    simbolos = lee_mapa(r['map'])
    en = {}
    for rva, n, ob, f, est in simbolos:
        if rva not in en or (en[rva][1] and not est):
            en[rva] = (n, est)
    funcs = sorted((rva, n, ob, est) for rva, n, ob, f, est in simbolos if f and t0 <= rva < t1)
    # quita duplicados de direccion (alias): el primero publico
    vistos = {}
    for x in funcs:
        if x[0] not in vistos or (vistos[x[0]][3] and not x[3]):
            vistos[x[0]] = x
    funcs = sorted(vistos.values())
    por_obj = {}
    for k, (rva, n, ob, est) in enumerate(funcs):
        fin = funcs[k + 1][0] if k + 1 < len(funcs) else t1
        por_obj.setdefault(ob.lower(), []).append((rva, fin, n, est))
    norm = None
    if normalizar:
        norm = Normalizacion(raiz, pe, rel)
        for obj, _, _ in ils:
            for rva, fin, n, est in por_obj.get(nombre_il(obj).lower(), []):
                norm.nuestra(rva, fin)
        norm.originales([obj for obj, _, _ in ils])
        norm.asigna()
    out = {}
    os.makedirs(r['obj'], exist_ok=True)
    total = 0
    for obj, _, _ in ils:
        lista = por_obj.get(nombre_il(obj).lower(), [])
        destino = os.path.join(r['obj'], os.path.relpath(os.path.join(raiz, obj), os.path.join(raiz, 'build', V,
                                                                                                'src')))
        os.makedirs(os.path.dirname(destino), exist_ok=True)
        escribe_unidad(destino, lista, pe, rel, en, norm)
        out[obj] = os.path.relpath(destino, raiz).replace('\\', '/')
        total += len(lista)
    avisa('ltcg360: troceado: %d funciones en %d objetos' % (total, len(out)))
    if norm:
        norm.escribe_originales(r['orig'])
        avisa('ltcg360: normalizacion: %s' % norm.resumen())
    return out


# --------------------------------------------------------------------------------------------- normalizacion
#
# Decision del usuario (2026-10-07). LTCG coloca las constantes de .rdata y las tablas de saltos segun el programa
# entero, y eso no se reproduce: hasta aqui un codigo igual salia distinto por dos artefactos de la medida.
#  - Pool de constantes: `lis rT, base@ha; addi rB, rT, base@l` y despues `lfs fX, d(rB)`. El desplazamiento d depende
#    de donde quedo cada constante. Se compara por VALOR: en los dos lados (el original troceado por jeff y nuestro
#    enlace) d se sustituye por un numero que identifica el valor leido en SU ejecutable (tamano y bytes); la tabla
#    de numeros es una sola para los dos lados y es inyectiva. Si el valor difiere, el numero difiere y la funcion
#    sigue fallando. Solo cargas sin actualizacion (lwz lbz lhz lha lfs lfd) con la base fijada por un lis/addi
#    reubicado de la misma funcion, sin que el registro se haya escrito entre medias, y que caen en .rdata; un lwz
#    que lee una direccion de la propia imagen (vtable, tabla de punteros) no es una constante y no se toca.
#  - Tablas de saltos: las palabras de la tabla (direcciones absolutas dentro de la funcion) se sustituyen en los
#    dos lados por `b destino`, relativo a la propia palabra: objdiff compara el DESTINO dentro de la funcion (un
#    valor suelto lo pinta como <illegal> y no lo compara: el control negativo lo demostro). Lo nuestro: las HIGHLOW de .reloc que caen
#    dentro de la funcion. El original (sin reubicaciones): las palabras seguidas tras un `bctr` que son una
#    direccion alineada dentro de la propia funcion.
# Lo que no se resuelve se queda como estaba (y sigue fallando). Las otras versiones no pasan por aqui.

ORIG_BASE = 0x82000000  # orig/EUROPEGERMILESTONE/NFS.exe es el PE del xex reubicado: VA 0x82000600 = RVA 0x600
CARGAS = {32: 4, 34: 1, 40: 2, 42: 2, 48: 4, 50: 8}  # lwz lbz lhz lha lfs lfd
BCTR = 0x4E800420
# opcode 31 (forma X): las logicas y desplazamientos escriben rA; las de comparar, guardar y cache no escriben GPR
X31_RA = {28, 60, 444, 412, 316, 476, 124, 284, 24, 536, 792, 824, 26, 954, 922, 27, 539, 794, 413, 986, 58}
X31_NADA = {0, 32, 4, 68, 151, 183, 215, 247, 407, 439, 663, 695, 727, 759, 918, 662, 150, 54, 86, 246, 278, 470,
            982, 1014, 598, 854, 149, 181, 21}


def escritos(w):
    """GPR que la instruccion w puede escribir (por exceso: mejor perder una base que inventarla)."""
    op, d, a = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    if op in (32, 34, 40, 42, 7, 8, 12, 13, 14, 15, 58):
        return {d}
    if op in (33, 35, 41, 43):
        return {d, a}
    if op == 46:
        return set(range(d, 32))
    if op in (37, 39, 45, 49, 51, 53, 55):
        return {a}
    if op in (36, 38, 44, 47, 48, 50, 52, 54, 62, 10, 11, 3, 2, 59, 63, 4, 16):
        return set()
    if op in (20, 21, 23, 24, 25, 26, 27, 28, 29, 30):
        return {a}
    if op in (18, 19):
        return set(range(0, 1)) | set(range(3, 13)) if w & 1 else set()
    if op == 31:
        xo = (w >> 1) & 0x3FF
        if xo in X31_NADA:
            return set()
        if xo in X31_RA:
            return {a}
        return {d}
    return {d, a}


def salto(rel):
    """Una entrada de tabla como `b destino` relativo a la propia palabra: objdiff compara el destino de un salto
    dentro de la funcion (un valor suelto sale como <illegal> y lo da por igual, que no compara nada)."""
    return 0x48000000 | (rel & 0x03FFFFFC)


def cargas_de_pool(ws, hi_idx, lo_idx):
    """{indice de palabra: (tamano, direccion absoluta, opcode)} de las cargas desde una base fijada con lis/addi."""
    hi, base, out = {}, {}, {}
    for i, w in enumerate(ws):
        op, d, a = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        if i in hi_idx and op == 15 and a == 0:
            hi[d] = (w & 0xFFFF) << 16
            base.pop(d, None)
            continue
        if i in lo_idx and op == 14 and a in hi:
            v = (hi[a] + sext16(w & 0xFFFF)) & 0xFFFFFFFF
            if d != a:
                hi.pop(d, None)
            base[d] = v
            continue
        if op in CARGAS and a in base and i not in hi_idx and i not in lo_idx:
            out[i] = (CARGAS[op], (base[a] + sext16(w & 0xFFFF)) & 0xFFFFFFFF, op)
        for x in escritos(w):
            hi.pop(x, None)
            base.pop(x, None)
    return out


def lee_coff(ruta):
    """(datos, secciones [(nombre, praw, tam, [(off, isim, tipo)])], simbolos {indice: (nombre, valor, sec, clase)})"""
    d = open(ruta, 'rb').read()
    _, nsec, _, psym, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
    secs = []
    for i in range(nsec):
        o = 20 + opt + i * 40
        nombre, _, _, rsz, praw, prel, _, nrel, _, _ = struct.unpack_from('<8sIIIIIIHHI', d, o)
        rels = [struct.unpack_from('<IIH', d, prel + k * 10) for k in range(nrel)]
        secs.append((nombre.rstrip(b'\0').decode('latin-1'), praw, rsz, rels))
    cad = psym + nsym * 18
    sims = {}
    i = 0
    while i < nsym:
        o = psym + i * 18
        n = d[o:o + 8]
        if n[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', n, 4)[0]
            n = d[cad + off:d.index(b'\0', cad + off)]
        else:
            n = n.rstrip(b'\0')
        valor, sec, _, clase, naux = struct.unpack_from('<IhHBB', d, o + 8)
        sims[i] = (n.decode('latin-1'), valor, sec, clase)
        i += 1 + naux
    return d, secs, sims


class Normalizacion:
    def __init__(self, raiz, pe, rel):
        self.raiz, self.pe, self.rel = raiz, pe, rel
        rd = pe.seccion('.rdata')
        self.rd = (rd[1], rd[1] + rd[2])
        self.orig_pe = PE(os.path.join(raiz, 'orig', V, 'NFS.exe'))
        ro = self.orig_pe.seccion('.rdata')
        self.ro = (ro[1], ro[1] + ro[2])
        self.carga_n = {}   # rva de la palabra: clave del valor
        self.tabla_n = {}   # rva de la palabra: desplazamiento del destino
        self.carga_o = {}   # (obj original, desplazamiento en el fichero): clave
        self.tabla_o = {}   # (obj original, desplazamiento en el fichero): desplazamiento del destino
        self.datos_o = {}
        self.ids = {}
        self.n = {'pool_nuestro': 0, 'pool_original': 0, 'tablas_nuestro': 0, 'tablas_original': 0,
                  'pool_fuera_de_rdata': 0, 'punteros': 0}
        fin = max(va + max(vs, rs) for _, va, vs, _, rs in pe.secs)
        self.img_n = (pe.base, pe.base + fin)
        fin = max(va + max(vs, rs) for _, va, vs, _, rs in self.orig_pe.secs)
        self.img_o = (ORIG_BASE, ORIG_BASE + fin)

    def _valor(self, pe, rango, rva, tam, op, imagen):
        """(tamano, bytes) del dato, o None si no es una constante: fuera de .rdata, o un lwz que lee una DIRECCION
        de la propia imagen (vtables, tablas de punteros como luaT_typenames: no se comparan por valor)."""
        if not (rango[0] <= rva and rva + tam <= rango[1]):
            return None
        b = pe.lee(rva, tam)
        if op == 32 and imagen[0] <= struct.unpack('>I', b)[0] < imagen[1]:
            self.n['punteros'] += 1
            return False
        return (tam, b)

    def nuestra(self, rva, fin):
        n = (fin - rva) // 4
        ws = list(struct.unpack('>%dI' % n, self.pe.lee(rva, 4 * n)))
        hi_idx = {i for i in range(n) if self.rel.get(rva + 4 * i, (0,))[0] == 4}
        lo_idx = {i for i in range(n) if self.rel.get(rva + 4 * i, (0,))[0] == 2}
        for i, (tam, va, op) in cargas_de_pool(ws, hi_idx, lo_idx).items():
            v = self._valor(self.pe, self.rd, va - self.pe.base, tam, op, self.img_n)
            if not v:
                self.n['pool_fuera_de_rdata'] += v is None
                continue
            self.carga_n[rva + 4 * i] = v
        for i in range(n):
            if self.rel.get(rva + 4 * i, (0,))[0] == 3:
                dest = ws[i] - self.pe.base
                if rva <= dest < fin:
                    self.tabla_n[rva + 4 * i] = salto(dest - rva - 4 * i)

    def originales(self, objs):
        """Analiza los objetos de jeff (build/<V>/obj) de esas unidades."""
        for obj in objs:
            rel_o = os.path.relpath(os.path.join(self.raiz, obj), os.path.join(self.raiz, 'build', V, 'src'))
            ruta = os.path.join(self.raiz, 'build', V, 'obj', rel_o)
            if not os.path.exists(ruta):
                continue
            d, secs, sims = lee_coff(ruta)
            self.datos_o[rel_o] = ruta
            for k, (nombre, praw, tam, rels) in enumerate(secs):
                if nombre != '.text':
                    continue
                sec = k + 1
                lista = sorted({(v, nm) for nm, v, s, cl in sims.values() if s == sec and cl in (2, 3)
                                and not (cl == 3 and nm.startswith('.'))})
                # direccion absoluta de la seccion: por los nombres con direccion (fn_/lbl_XXXXXXXX)
                bases = collections.Counter()
                for v, nm in lista:
                    m = re.match(r'(?:fn|lbl)_([0-9A-Fa-f]{8})$', nm)
                    if m:
                        bases[int(m.group(1), 16) - v] += 1
                if not bases:
                    continue
                base = bases.most_common(1)[0][0]
                tipos = collections.defaultdict(set)
                for off, _, t in rels:
                    tipos[off].add(t)
                inicios = sorted({v for v, _ in lista})
                for j, v in enumerate(inicios):
                    fin = inicios[j + 1] if j + 1 < len(inicios) else tam
                    n = (fin - v) // 4
                    if n <= 0:
                        continue
                    ws = list(struct.unpack_from('>%dI' % n, d, praw + v))
                    hi_idx = {i for i in range(n) if R_REFHI in tipos.get(v + 4 * i, ())}
                    lo_idx = {i for i in range(n) if R_REFLO in tipos.get(v + 4 * i, ())}
                    for i, (t, va, op) in cargas_de_pool(ws, hi_idx, lo_idx).items():
                        val = self._valor(self.orig_pe, self.ro, va - ORIG_BASE, t, op, self.img_o)
                        if not val:
                            self.n['pool_fuera_de_rdata'] += val is None
                            continue
                        self.carga_o[(rel_o, praw + v + 4 * i)] = val
                    a0, a1 = base + v, base + fin
                    i = 0
                    while i < n:
                        if ws[i] == BCTR:
                            j2 = i + 1
                            while j2 < n and a0 <= ws[j2] < a1 and ws[j2] % 4 == 0 and not tipos.get(v + 4 * j2):
                                self.tabla_o[(rel_o, praw + v + 4 * j2)] = salto(ws[j2] - a0 - 4 * j2)
                                j2 += 1
                            i = j2
                        else:
                            i += 1

    def asigna(self):
        claves = sorted(set(self.carga_n.values()) | set(self.carga_o.values()))
        if len(claves) >= 0xFFFF:
            raise ValueError('normalizacion: %d valores distintos no caben en 16 bits' % len(claves))
        self.ids = {c: k + 1 for k, c in enumerate(claves)}

    def palabra_nuestra(self, rva_palabra, w):
        """La palabra normalizada, o None si no se toca."""
        if rva_palabra in self.carga_n:
            self.n['pool_nuestro'] += 1
            return (w & 0xFFFF0000) | self.ids[self.carga_n[rva_palabra]]
        if rva_palabra in self.tabla_n:
            self.n['tablas_nuestro'] += 1
            return self.tabla_n[rva_palabra]
        return None

    def escribe_originales(self, dir_orig):
        for rel_o, ruta in sorted(self.datos_o.items()):
            d = bytearray(open(ruta, 'rb').read())
            for (r, off), clave in self.carga_o.items():
                if r == rel_o:
                    w = struct.unpack_from('>I', d, off)[0]
                    struct.pack_into('>I', d, off, (w & 0xFFFF0000) | self.ids[clave])
                    self.n['pool_original'] += 1
            for (r, off), desp in self.tabla_o.items():
                if r == rel_o:
                    struct.pack_into('>I', d, off, desp)
                    self.n['tablas_original'] += 1
            destino = os.path.join(dir_orig, rel_o)
            os.makedirs(os.path.dirname(destino), exist_ok=True)
            tmp = destino + '.tmp'
            open(tmp, 'wb').write(bytes(d))
            os.replace(tmp, destino)

    def resumen(self):
        return ', '.join('%s %d' % kv for kv in sorted(self.n.items())) + ', valores distintos %d' % len(self.ids)


def ruta_original(raiz, target_path):
    """El original normalizado (build/<V>/ltcg/orig/...) que corresponde a target_path (build/<V>/obj/...)."""
    rel_o = os.path.relpath(os.path.join(raiz, target_path), os.path.join(raiz, 'build', V, 'obj'))
    p = os.path.join(rutas(raiz)['orig'], rel_o)
    return os.path.relpath(p, raiz).replace('\\', '/') if os.path.exists(p) else None


def escribe_unidad(destino, lista, pe, rel, en, norm=None):
    datos = bytearray()
    rels = []
    simbolos = []
    indice = {}

    def sim(nombre, defin=None):
        if nombre in indice and (defin is None or simbolos[indice[nombre]][2] != 0):
            return indice[nombre]
        indice[nombre] = len(simbolos)
        if defin is None:
            simbolos.append((nombre, 0, 0, 0x20, 2))
        else:
            simbolos.append((nombre, defin[0], 1, 0x20, 3 if defin[1] else 2))
        return indice[nombre]

    def nombre_de(va):
        rva = va - pe.base
        if rva in en:
            return en[rva][0]
        return 'ltcg_%08X' % va

    # primero las definiciones, para que las referencias internas apunten a ellas
    off = 0
    for rva, fin, n, est in lista:
        sim(n, (off, est))
        off += fin - rva
    for rva, fin, n, est in lista:
        base_off = len(datos)
        b = bytearray(pe.lee(rva, fin - rva))
        nw = len(b) // 4
        for i in range(nw):
            a = rva + 4 * i
            w = struct.unpack_from('>I', b, 4 * i)[0]
            o = base_off + 4 * i
            nuevo = norm.palabra_nuestra(a, w) if norm else None
            if nuevo is not None:  # carga del pool por valor o entrada de tabla por destino: sin reubicacion
                struct.pack_into('>I', b, 4 * i, nuevo)
                continue
            if a in rel:
                t, param = rel[a]
                if t == 4:  # HIGHADJ: lis con la mitad alta ajustada
                    va = ((w & 0xFFFF) << 16) + sext16(param)
                    idx = sim(nombre_de(va & 0xFFFFFFFF))
                    rels += [(o, idx, R_REFHI), (o, 0, R_PAIR)]
                    w &= 0xFFFF0000
                elif t == 2:  # LOW: busca el lis de su registro base hacia atras
                    ra = (w >> 16) & 31
                    hi = None
                    for j in range(i - 1, -1, -1):
                        aj = rva + 4 * j
                        wj = struct.unpack_from('>I', b, 4 * j)[0]
                        if aj in rel and rel[aj][0] == 4 and (wj >> 26) == 15 and ((wj >> 21) & 31) == ra:
                            hi = wj & 0xFFFF
                            break
                    if hi is None:
                        for j in range(nw):
                            aj = rva + 4 * j
                            if aj in rel and rel[aj][0] == 4 and rel[aj][1] == (w & 0xFFFF):
                                hi = struct.unpack_from('>I', b, 4 * j)[0] & 0xFFFF
                                break
                    va = ((hi or 0) << 16) + sext16(w & 0xFFFF)
                    idx = sim(nombre_de(va & 0xFFFFFFFF) if hi is not None else 'ltcg_lo_%08X' % (pe.base + a))
                    rels.append((o, idx, R_REFLO))
                    w &= 0xFFFF0000
                elif t == 3:  # HIGHLOW: una direccion de 32 bits dentro del codigo (tablas de salto)
                    rels.append((o, sim(nombre_de(w)), R_ADDR32))
                    w = 0
            elif w >> 26 == 18:
                li = w & 0x03FFFFFC
                if li & 0x02000000:
                    li -= 0x04000000
                dest = li if w & 2 else a + li
                if not (rva <= dest < fin):
                    rels.append((o, sim(nombre_de(pe.base + dest)), R_REL24))
                    w &= 0xFC000003
            struct.pack_into('>I', b, 4 * i, w)
        datos += b
    escribe_coff(destino, [('.text', bytes(datos), rels, 0x60500020)], simbolos)


# --------------------------------------------------------------------------------------------- todo

def generar(raiz, ninja='build.ninja', fallidos=(), hilos=4, solo_trocear=False, avisa=print):
    """El paso entero. Devuelve ({objeto de ninja: objeto troceado}, {objeto: motivo}) o (None, motivo)."""
    r = rutas(raiz)
    os.makedirs(r['base'], exist_ok=True)
    unidades = unidades_ninja(os.path.join(raiz, ninja))
    if not unidades:
        return None, 'build.ninja no tiene unidades msvc de %s' % V
    fallidos = {os.path.normpath(f) for f in fallidos}
    if solo_trocear and os.path.exists(r['exe']):
        ils = [u for u in unidades if os.path.exists(os.path.join(r['il'], nombre_il(u[0])))]
        malos = {u[0]: 'sin IL' for u in unidades if u not in ils}
    else:
        t = time.time()
        ils, malos = compila_il(raiz, unidades, fallidos, hilos, avisa)
        avisa('ltcg360: IL de %d unidades (%d sin objeto) en %.0f s' % (len(ils), len(malos), time.time() - t))
        if not ils or not enlaza(raiz, ils, avisa):
            return None, 'el enlace LTCG fallo (ver %s)' % os.path.relpath(r['log'], raiz)
    return trocea(raiz, ils, avisa), malos


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--ninja', default='build.ninja')
    ap.add_argument('--hilos', type=int, default=4)
    ap.add_argument('--fallidos', default='', help='objetos de ninja que fallaron, separados por comas')
    ap.add_argument('--solo-trocear', action='store_true')
    a = ap.parse_args()
    raiz = os.getcwd()
    res, malos = generar(raiz, a.ninja, [x for x in a.fallidos.split(',') if x], a.hilos, a.solo_trocear)
    if res is None:
        print('ltcg360: ' + malos)
        return 1
    for o, m in sorted(malos.items()):
        print('ltcg360: sin objeto LTCG: %s (%s)' % (o, m))
    return 0


if __name__ == '__main__':
    sys.exit(main())
