#!/usr/bin/env python3
"""nombra_elim.py -- nombres de PC (SPEED_EXE_1_3) por ELIMINACION y por PRIMER DEFINIDOR, con validacion obligatoria
y registro de procedencia.

Por que existe: scripts/nombra_grafo.py se agoto en PC (+9 en la ronda 38) dejando miles de funciones sin nombre cuyo
codigo es IGUAL al de una funcion nuestra. Le faltaban tres reglas:
  1. El enlazador de MSVC pone una COMDAT con el PRIMER objeto que la aporta (orden de enlace = orden de direcciones
     de las unidades). Una funcion nuestra solo puede estar en la unidad que la define primero (98,6 % de las anclas
     con varias definiciones lo cumplen).
  2. Una funcion nuestra cuyo nombre YA esta en symbols.json no es candidata en ningun sitio (esta en otra direccion).
  3. Los destinos de las reubicaciones se comparan por DIRECCION, no por nombre: IDA pone dos nombres a una misma
     direccion (`??3@YAXPAX@Z` y `?Free@FastMem@@...`), y por nombre parecian contradecirse.

Pool de la unidad U: funciones de nuestro U.obj (build/SPEED_EXE_1_3/src) con nombre que no esta en symbols.json y
cuyo primer definidor es U. Objetivo: funciones del U.obj troceado (build/SPEED_EXE_1_3/obj) SIN nombre que MSVC pueda
emitir (sub_, nullsub_, unknown_libname_, `Clase::Metodo` de IDA, decoraciones de GCC, nombres con la direccion).
Se agrupan por ESQUELETO (bytes con las reubicaciones a cero, el de nombra_grafo). Reglas:
  elim-1:1     el grupo tiene una funcion del original y una nuestra.
  elim-orden   el grupo tiene N y N (copias de codigo igual): se asignan en orden (direccion en el exe contra orden
               de definicion en nuestro .obj) y SOLO si cada una cae entre sus anclas vecinas. Son nombres de
               "copia ambigua": el codigo es identico, el nombre puede estar cruzado con otra copia (~3 % validado).
Filtros de cada pareja:
  - ningun destino se contradice (direcciones distintas, o una constante __real@/cadena ??_C@ que no dice lo que hay
    en el exe);
  - una funcion de menos de 16 B con reubicaciones necesita un testigo (un destino confirmado): los envoltorios
    pequenos (push; call X; ret) tienen el mismo esqueleto con cualquier X;
  - elim-orden ademas necesita estar entre sus anclas vecinas.
Despues, objdiff: se renombra en una COPIA del .obj troceado y `objdiff-cli diff` (opciones del informe) tiene que dar
100 % a cada pareja; la que no, se descarta.

VALIDACION OBLIGATORIA: antes de aplicar se esconde el 30 % de las anclas (nombre ya puesto y codigo igual) y se mide
cuantas recupera cada regla con SU nombre. Si elim-1:1 baja del 99 % o elim-orden del 95 %, no se aplica nada.

PROCEDENCIA: cada nombre aplicado se anota en config/SPEED_EXE_1_3/nombres_procedencia.tsv (direccion, nombre
anterior, nombre nuevo, regla, marca, bytes, testigos, fecha). La marca `copia-ambigua` (elim-orden) permite revisarlos
o quitarlos de golpe con --quitar, que devuelve el nombre anterior.

    python scripts/nombra_elim.py                         # informa (con validacion y objdiff), no escribe
    python scripts/nombra_elim.py --aplicar               # aplica a symbols.json y anota la procedencia
    python scripts/nombra_elim.py --aplicar --solo-1a1    # solo la regla 1:1
    python scripts/nombra_elim.py --quitar copia-ambigua  # deshace los nombres con esa marca (o --quitar elim-1:1)
    python scripts/nombra_elim.py --tsv SALIDA.tsv        # las parejas propuestas, para revisar

Requiere los .obj al dia (los troceados de build/SPEED_EXE_1_3/obj tras el ultimo cambio de symbols.json y los
nuestros compilados): mide antes con scripts/medir.py SPEED_EXE_1_3. Repetirlo tras cada ronda de PC, despues de
nombra_grafo.py: cada funcion nueva que compile igual es un nombre mas.
Ronda 38 (2026-10-07), primera pasada: ver docs/BEYOND_UPSTREAM.md (PC names by code).
Los nombres FALSOS ya puestos (esta regla solo toca funciones sin nombre) los corrige scripts/nombra_corrige.py (ronda
39), que tambien repasa las copias ambiguas (--copias). Orden de la rutina tras cada ronda de PC: medir PC,
nombra_corrige.py --aplicar, nombra_corrige.py --copias --aplicar, medir PC, nombra_grafo.py --aplicar,
nombra_elim.py --aplicar, medir PC (y repetir mientras salga algo).
"""
import bisect
import collections
import datetime
import json
import os
import random
import re
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import nombra_grafo as ng  # noqa: E402
from escritura import abre, entero  # noqa: E402

V = 'SPEED_EXE_1_3'
CONFIG = os.path.join(RAIZ, 'config', V)
SYMBOLS = os.path.join(CONFIG, 'symbols.json')
PROCEDENCIA = os.path.join(CONFIG, 'nombres_procedencia.tsv')
OBJDIFF = os.path.join(RAIZ, 'build', 'tools', 'objdiff-cli.exe')
TMP = os.path.join(RAIZ, 'build', V, 'nombra_elim_tmp')
EH = ('__unwindfunclet$', '__ehhandler$', '__catch$', '__tryblock')
MARCA = {'elim-1:1': 'seguro', 'elim-orden': 'copia-ambigua'}
MIN_SIN_TESTIGO = 16
UMBRAL_VALIDACION = {'elim-1:1': 0.99, 'elim-orden': 0.95}

# nombres que MSVC 7.1 no puede emitir: marcadores de IDA, desenredados de IDA, decoraciones de GCC, con direccion
_IDA = re.compile(r'^(sub|nullsub|j_sub|j|unknown_libname|loc|jpt|SEH|lpStartAddress|lpfnCallback|StartAddress)(_|$)'
                  r'|^Unwind@|^FID_conflict:|::|\(|\s|__[0-9A-F]{8}$|_[0-9a-f]{8}$|__delink_')
_GCC = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*__(?:\d|F|C|Q\d|t\d|H\d)|^_\$_|^__ct__|^__dt__')
_DUP = re.compile(r'^\?.*@[A-Z0-9]*Z_\d+$|^\?.*_\d+_SEH$')


def valido(n):
    """True si el nombre puede salir de MSVC (decoracion `?...` o nombre de C)."""
    if _IDA.search(n) or _GCC.search(n) or _DUP.search(n):
        return False
    return n.startswith('?') or bool(re.match(r'^[A-Za-z_@][A-Za-z0-9_@$?]*$', n))


class Exe:
    """symbols.json, rangos de unidades y bytes de datos del exe (para las constantes)."""

    def __init__(self):
        d = json.load(open(SYMBOLS, encoding='utf-8'))
        self.tamanos = {}
        self.dirs = collections.defaultdict(set)
        for f in d['functions']:
            self.tamanos[f['name']] = f['end'] - f['start']
            self.dirs[f['name']].add(f['start'])
        for x in d['names']:
            self.dirs[x['name']].add(x['addr'])
        s = json.load(open(os.path.join(CONFIG, 'splits.json'), encoding='utf-8'))
        unidades = []
        self.rangos = {}
        for u, v in s.items():
            rs = v.get('function_ranges') or []
            if rs:
                unidades.append((min(r[0] for r in rs), u[:-4]))
                self.rangos[u[:-4]] = [tuple(r) for r in rs]
        self.unidades = [u for _, u in sorted(unidades)]  # orden de enlace
        pc = ng.PC.__new__(ng.PC)
        self.pc = pc
        # la memoria de datos, como nombra_grafo.PC
        import struct
        pe = open(os.path.join(RAIZ, 'orig', V, 'speed.exe'), 'rb').read()
        e = struct.unpack_from('<I', pe, 0x3c)[0]
        nsec = struct.unpack_from('<H', pe, e + 6)[0]
        opt = struct.unpack_from('<H', pe, e + 20)[0]
        base = struct.unpack_from('<I', pe, e + 24 + 28)[0]
        trozos = []
        for i in range(nsec):
            nombre, vsz, va, rsz, raw = struct.unpack_from('<8sIIII', pe, e + 24 + opt + i * 40)
            if nombre.rstrip(b'\0') in (b'.rdata', b'.data'):
                trozos.append((base + va, pe[raw:raw + min(rsz, vsz)]))
        self.memoria = ng.Memoria(trozos)

    def dir_unica(self, n):
        ds = self.dirs.get(n)
        return next(iter(ds)) if ds and len(ds) == 1 else None


class Unidad:
    def __init__(self, exe, nombre):
        self.nombre = nombre
        po = os.path.join(RAIZ, 'build', V, 'obj', nombre + '.obj')
        ps = os.path.join(RAIZ, 'build', V, 'src', nombre + '.obj')
        self.ok = os.path.exists(po) and os.path.exists(ps)
        if not self.ok:
            return
        self.po, self.ps = po, ps
        self.fo = ng.Coff(po).funciones(exe.tamanos)
        cs = ng.Coff(ps)
        self.fs = cs.funciones()
        self.indice = {}
        for k, s in enumerate(cs.lista):
            if s[0] in self.fs:
                self.indice.setdefault(s[0], k)
        self.eo = {n: exe.pc.esqueleto(*v) for n, v in self.fo.items()}
        self.es = {n: exe.pc.esqueleto(*v) for n, v in self.fs.items()}
        rs = exe.rangos[nombre]
        self.va = {}
        for n in self.fo:
            ds = [a for a in exe.dirs.get(n, ()) if any(i <= a < f for i, f in rs)]
            self.va[n] = ds[0] if len(ds) == 1 else None


def destinos(exe, vo, vs):
    """(contradicciones, testigos) de las reubicaciones de una pareja con el mismo esqueleto."""
    ro, rs = vo[1], vs[1]
    contra = testigos = 0
    for i, (t, no) in ro.items():
        m = rs.get(i)
        if m is None:
            contra += 1
            continue
        ns = m[1]
        if ng.equivalentes(no, ns):
            testigos += 1
            continue
        k = vs[0][i:i + 4]
        if len(k) == 4 and any(k):
            continue  # el nuestro apunta a simbolo+k: no se decide
        ao, as_ = exe.dir_unica(no), exe.dir_unica(ns)
        if ao is not None and as_ is not None:
            if ao == as_:
                testigos += 1
            else:
                contra += 1
            continue
        if ao is not None:
            v = exe.memoria.comprueba(ao, ns)
            if v is True:
                testigos += 1
            elif v is False:
                contra += 1
    return contra, testigos


def corre(exe, unidades, escondidas):
    """[(unidad, regla, orig, nuestro, bytes, testigos)]. escondidas: {unidad: {nombre}} anclas tratadas como sin
    nombre (validacion)."""
    nombres_exe = set(exe.dirs)
    for u, ns in escondidas.items():
        nombres_exe -= ns
    primero = {}
    for u in unidades:
        for n in u.fs:
            primero.setdefault(n, u.nombre)
    out = []
    for u in unidades:
        esc = escondidas.get(u.nombre, set())
        # anclas: nombre en los dos lados (sin las escondidas), para la compuerta de orden
        anclas = sorted((u.va[n], u.indice[n]) for n in u.fo if n in u.fs and n not in esc and u.va.get(n) is not None
                        and n in u.indice)

        def en_orden(o, n):
            a = u.va.get(o)
            if a is None:
                return False
            k = bisect.bisect_left(anclas, (a, -1))
            antes = [x[1] for x in anclas[max(0, k - 2):k]]
            despues = [x[1] for x in anclas[k:k + 2]]
            i = u.indice.get(n, -1)
            return (not antes or i > min(antes)) and (not despues or i < max(despues))

        pool = [n for n in u.fs if n not in nombres_exe and primero.get(n) == u.nombre and valido(n)
                and not n.startswith(EH) and len(n) <= 2000]
        g = collections.defaultdict(lambda: ([], []))
        for n in pool:
            g[u.es[n]][1].append(n)
        for o in u.fo:
            if (not valido(o) or o in esc) and u.eo[o] in g and u.va.get(o) is not None:
                g[u.eo[o]][0].append(o)
        for e, (os_, ns) in g.items():
            if not os_ or not ns or len(os_) != len(ns):
                continue
            if len(os_) == 1:
                parejas, regla = [(os_[0], ns[0])], 'elim-1:1'
            else:
                a = sorted(os_, key=lambda x: u.va[x])
                b = sorted(ns, key=lambda x: u.indice.get(x, 1 << 30))
                parejas, regla = list(zip(a, b)), 'elim-orden'
            for o, n in parejas:
                contra, testigos = destinos(exe, u.fo[o], u.fs[n])
                if contra:
                    continue
                tam = len(u.fo[o][0])
                if tam < MIN_SIN_TESTIGO and u.fo[o][1] and not testigos:
                    continue
                if regla == 'elim-orden' and not en_orden(o, n):
                    continue
                out.append((u.nombre, regla, o, n, tam, testigos))
    return out


def valida(exe, unidades, fraccion=0.3, semilla=7):
    random.seed(semilla)
    esc = {}
    for u in unidades:
        cand = [n for n in u.fo if n in u.fs and valido(n) and u.eo[n] == u.es[n] and u.va.get(n) is not None]
        esc[u.nombre] = set(x for x in cand if random.random() < fraccion)
    res = corre(exe, unidades, esc)
    cnt = collections.Counter()
    malas = []
    for un, regla, o, n, t, w in res:
        if o in esc[un]:
            ok = ng.equivalentes(o, n)
            cnt[(regla, ok)] += 1
            if not ok:
                malas.append((un, regla, o, n))
    return cnt, malas, sum(len(x) for x in esc.values())


def renombra_coff(datos, cambios):
    """Bytes del COFF con los simbolos de `cambios` {viejo: nuevo} renombrados (cadena nueva al final)."""
    import struct
    d = bytearray(datos)
    _, nsec, _, psym, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
    cad = psym + nsym * 18
    tam_cad = struct.unpack_from('<I', d, cad)[0]
    assert cad + tam_cad == len(d), 'la tabla de cadenas no es lo ultimo del fichero'
    extra = bytearray()
    hechos = set()
    i = 0
    while i < nsym:
        o = psym + i * 18
        n = bytes(d[o:o + 8])
        if n[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', n, 4)[0]
            nombre = bytes(d[cad + off:d.index(b'\0', cad + off)])
        else:
            nombre = n.rstrip(b'\0')
        nombre = nombre.decode('latin-1')
        if nombre in cambios and nombre not in hechos:
            off = tam_cad + len(extra)
            extra += cambios[nombre].encode('latin-1') + b'\0'
            struct.pack_into('<II', d, o, 0, off)
            hechos.add(nombre)
        i += 1 + d[o + 17]
    d += extra
    struct.pack_into('<I', d, cad, tam_cad + len(extra))
    return bytes(d), hechos


def verifica_objdiff(unidades, parejas):
    """{(unidad, orig): porcentaje} con objdiff sobre una copia renombrada del .obj troceado."""
    os.makedirs(TMP, exist_ok=True)
    por_u = collections.defaultdict(dict)
    for un, regla, o, n, t, w in parejas:
        por_u[un][o] = n
    um = {u.nombre: u for u in unidades}
    out = {}
    for un, cambios in por_u.items():
        u = um[un]
        datos, hechos = renombra_coff(open(u.po, 'rb').read(), cambios)
        t = os.path.join(TMP, os.path.basename(un) + '.obj')
        sal = os.path.join(TMP, os.path.basename(un) + '.json')
        open(t, 'wb').write(datos)
        r = subprocess.run([OBJDIFF, 'diff', '-1', t, '-2', u.ps, '-c', 'functionRelocDiffs=none', '-o', sal,
                            '--format', 'json'], capture_output=True, text=True)
        if r.returncode == 0:
            d = json.load(open(sal, encoding='utf-8'))
            pct = {s.get('name'): s.get('match_percent') for s in d['left']['symbols']
                   if s.get('kind') == 'SYMBOL_FUNCTION'}
            for o, n in cambios.items():
                out[(un, o)] = pct.get(n) if o in hechos else None
        else:
            print('objdiff fallo en', un, r.stderr[-300:])
        for q in (t, sal):
            if os.path.exists(q):
                os.remove(q)
    try:
        os.rmdir(TMP)
    except OSError:
        pass
    return out


def lee_procedencia():
    if not os.path.exists(PROCEDENCIA):
        return []
    filas = []
    for l in open(PROCEDENCIA, encoding='utf-8'):
        if l.startswith('#') or not l.strip():
            continue
        filas.append(l.rstrip('\n').split('\t'))
    return filas


CABECERA = ('# Nombres de PC puestos por scripts/nombra_elim.py (no los borres a mano: usa --quitar MARCA).\n'
            '# direccion\tnombre_anterior\tnombre_nuevo\tregla\tmarca\tbytes\ttestigos\tfecha\n')


def escribe_procedencia(filas):
    with abre(PROCEDENCIA, 'w', encoding='utf-8', newline='\n') as f:
        f.write(CABECERA)
        for r in filas:
            f.write('\t'.join(r) + '\n')


_ENTRADA = re.compile(r'"(start|addr)": (\d+),(\s*"end": \d+,)?\s*"name": "((?:[^"\\]|\\.)*)"')


def nombres_por_direccion(texto):
    """{direccion: nombre} de la lista `functions` (lo que compara objdiff)."""
    out = {}
    for m in _ENTRADA.finditer(texto):
        if m.group(1) == 'start':
            out[int(m.group(2))] = json.loads('"%s"' % m.group(4))
    return out


def aplica(cambios):
    """cambios [(direccion, viejo, nuevo)] -> [los aplicados]. Una sola pasada por el texto: cambia el nombre en la
    entrada de `functions` (start) y en la de `names` (addr) de esa direccion SOLO si alli esta el nombre viejo, asi
    que es idempotente (si ya tiene el nuevo, no hace nada). No reescribe el JSON: el resto del fichero queda igual."""
    texto = open(SYMBOLS, 'rb').read().decode('utf-8')
    por_dir = {a: (viejo, nuevo) for a, viejo, nuevo in cambios}
    trozos, ultimo, hechos = [], 0, set()
    for m in _ENTRADA.finditer(texto):
        a = int(m.group(2))
        if a not in por_dir:
            continue
        viejo, nuevo = por_dir[a]
        if json.loads('"%s"' % m.group(4)) != viejo:
            continue
        trozos.append(texto[ultimo:m.start(4)])
        trozos.append(json.dumps(nuevo)[1:-1])
        ultimo = m.end(4)
        if m.group(1) == 'start':
            hechos.add(a)
    trozos.append(texto[ultimo:])
    entero(SYMBOLS, 'wb').write(''.join(trozos).encode('utf-8'))
    return [(a, v, n) for a, v, n in cambios if a in hechos]


def quitar(marca):
    filas = lee_procedencia()
    deshacer = [(int(r[0]), r[2], r[1]) for r in filas if marca in (r[3], r[4])]
    hechos = set(aplica(deshacer))
    quedan = [r for r in filas if not (marca in (r[3], r[4]) and (int(r[0]), r[2], r[1]) in hechos)]
    escribe_procedencia(quedan)
    print('deshechos %d de %d nombres con la marca %s' % (len(hechos), len(deshacer), marca))


def main():
    if '--quitar' in sys.argv:
        quitar(sys.argv[sys.argv.index('--quitar') + 1])
        return 0
    exe = Exe()
    unidades = [u for u in (Unidad(exe, n) for n in exe.unidades) if u.ok]
    print('unidades con los dos objetos:', len(unidades))
    cnt, malas, n_esc = valida(exe, unidades)
    ok = True
    print('VALIDACION (30 %% de las anclas escondidas: %d)' % n_esc)
    for regla in ('elim-1:1', 'elim-orden'):
        b, m = cnt[(regla, True)], cnt[(regla, False)]
        tasa = b / (b + m) if b + m else 1.0
        print('  %-11s bien %5d  mal %4d  acierto %.4f (minimo %.2f)' % (regla, b, m, tasa, UMBRAL_VALIDACION[regla]))
        if tasa < UMBRAL_VALIDACION[regla]:
            ok = False
    for x in malas[:10]:
        print('    MAL', x[0], x[1], x[2][:60], '->', x[3][:60])
    res = corre(exe, unidades, {})
    if '--solo-1a1' in sys.argv:
        res = [r for r in res if r[1] == 'elim-1:1']
    pct = verifica_objdiff(unidades, res)
    finales = [r for r in res if pct.get((r[0], r[2])) == 100.0]
    c = collections.Counter()
    for r in res:
        k = 'objdiff 100' if pct.get((r[0], r[2])) == 100.0 else 'descartada por objdiff'
        c[(r[1], k)] += 1
        c[(r[1], k, 'B')] += r[4]
    for k in sorted(c, key=str):
        print(' ', k, c[k])
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8') as f:
            for r in finales:
                f.write('\t'.join(map(str, r)) + '\n')
    if '--aplicar' not in sys.argv:
        return 0
    if not ok:
        print('NO SE APLICA: la validacion no llega al minimo')
        return 1
    um = {u.nombre: u for u in unidades}
    cambios = [(um[r[0]].va[r[2]], r[2], r[3]) for r in finales]
    hechos = aplica(cambios)
    fecha = datetime.date.today().isoformat()
    meta = {(um[r[0]].va[r[2]], r[2], r[3]): r for r in finales}
    filas = lee_procedencia()
    for a, viejo, nuevo in hechos:
        r = meta[(a, viejo, nuevo)]
        filas.append([str(a), viejo, nuevo, r[1], MARCA[r[1]], str(r[4]), str(r[5]), fecha])
    escribe_procedencia(filas)
    print('APLICADOS %d de %d nombres en %s; procedencia en %s' % (len(hechos), len(cambios), SYMBOLS, PROCEDENCIA))
    return 0


if __name__ == '__main__':
    sys.exit(main())
