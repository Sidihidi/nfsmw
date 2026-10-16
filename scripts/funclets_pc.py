#!/usr/bin/env python3
"""funclets_pc.py [--ver] [--salida=DIR] -- PC: coloca y nombra en config/SPEED_EXE_1_3/symbols.json los funclets de
desenrollado de EH de MSVC 7.1 (ronda 50, PC50).

Cada funcion con destructores que pueden correr al lanzar una excepcion lleva en el objeto una seccion `.text$x` propia:
sus funclets (`$L<n>`: `lea ecx,[ebp-x]; jmp ??1Clase`, en orden) y al final `__ehhandler$<funcion>`. El enlazador pone
las .text$x de cada objeto juntas al final del .text (0x8638A0-0x883C80), y PC49 (ronda 49) ya coloco y nombro los
`__ehhandler$` con un rango .text$x por unidad en splits.json. Aqui, por cada .text$x de nuestros objetos:

  1. su `__ehhandler$F` tiene que estar en symbols.json UNA vez dentro de los rangos de su unidad;
  2. base = direccion del handler - su desplazamiento en la seccion; TODOS los bytes de la seccion iguales a los del exe
     salvo los de las reubicaciones;
  3. el i-esimo funclet se llama `Unwind@F$i` y ocupa hasta el siguiente simbolo de la seccion.

El nombre es POSICIONAL: el numero de `$L` es un contador de la unidad que cambia con cualquier cabecera (PC49 los metio
con ese numero y la medida de PC fallo entera). El prefijo `Unwind@` es el de IDA (symbols.json ya tenia
`Unwind@<direccion>` en 3.656 de ellos, en la lista `names`, que se renombra igual); el nombre de MSVC 8,
`__unwindfunclet$F$i`, no sirve: objdiff 3.8 esconde los simbolos que empiezan por `__unwind` o `__catch`.
La medida pone el mismo nombre a nuestros `$L` (scripts/quita_L.py, nombrar_funclets) y los mapea (mapeos_funclets).

Idempotente: una entrada que ya existe con el mismo inicio, fin y nombre no cambia; una `Unwind@` con otro nombre en la
misma direccion y el mismo tamano se renombra (la de IDA, o la funcion cambio sus funclets); nunca pisa una funcion que no sea `Unwind@`. Con --ver solo
cuenta. Rehacerlo tras un cambio que mueva los funclets (otro reparto de .text$x, PC49 rehecho): compilar PC y correrlo.

    python scripts/funclets_pc.py --ver
    python scripts/funclets_pc.py                    # escribe config/SPEED_EXE_1_3/symbols.json y nombres_procedencia.tsv
    python scripts/funclets_pc.py --salida=DIR       # escribe las dos copias en DIR
"""
import bisect
import collections
import datetime
import json
import os
import re
import struct
import sys
from escritura import entero

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CFG = os.path.join(RAIZ, 'config', 'SPEED_EXE_1_3')
EXE = os.path.join(RAIZ, 'orig', 'SPEED_EXE_1_3', 'speed.exe')
OBJ = os.path.join(RAIZ, 'build', 'SPEED_EXE_1_3', 'src')
NOMBRE = 'Unwind@%s$%d'


class Exe:
    def __init__(self, ruta):
        d = self.d = open(ruta, 'rb').read()
        pe, = struct.unpack_from('<I', d, 0x3C)
        nsec, = struct.unpack_from('<H', d, pe + 6)
        opt, = struct.unpack_from('<H', d, pe + 20)
        base, = struct.unpack_from('<I', d, pe + 24 + 28)
        self.secs = []
        for i in range(nsec):
            o = pe + 24 + opt + 40 * i
            vs, va, rs, ro = struct.unpack_from('<IIII', d, o + 8)
            self.secs.append((base + va, vs, ro, rs))

    def lee(self, a, n):
        for va, vs, ro, rs in self.secs:
            if va <= a < va + vs:
                off = a - va
                return self.d[ro + off:ro + off + n] if off + n <= rs else None
        return None


def secciones_x(ruta):
    """[(simbolos [(valor, nombre)], bytes, desplazamientos con reubicacion)] de cada .text$x del objeto COFF."""
    d = open(ruta, 'rb').read()
    _, nsec, _, psym, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
    cad = psym + nsym * 18

    def nombre(raw):
        if raw[:4] == b'\0\0\0\0':
            o, = struct.unpack_from('<I', raw, 4)
            return d[cad + o:d.index(b'\0', cad + o)].decode('latin-1')
        return raw.rstrip(b'\0').decode('latin-1')
    secs = []
    for i in range(nsec):
        o = 20 + opt + 40 * i
        n = d[o:o + 8].rstrip(b'\0')
        if n.startswith(b'/') and n[1:].isdigit():
            n = d[cad + int(n[1:]):d.index(b'\0', cad + int(n[1:]))]
        rsz, praw, prel, _, nrel = struct.unpack_from('<IIIIH', d, o + 16)
        secs.append((n, rsz, praw, prel, nrel))
    sims = collections.defaultdict(list)
    i = 0
    while i < nsym:
        o = psym + 18 * i
        valor, sec, tipo, clase, naux = struct.unpack_from('<IhHBB', d, o + 8)
        if sec > 0 and secs[sec - 1][0] == b'.text$x':
            n = nombre(d[o:o + 8])
            if not n.startswith('.'):
                sims[sec].append((valor, n))
        i += 1 + naux
    out = []
    for sec, l in sims.items():
        n, rsz, praw, prel, nrel = secs[sec - 1]
        rel = set()
        for r in range(nrel):
            va, = struct.unpack_from('<I', d, prel + 10 * r)
            rel.update(range(va, va + 4))
        out.append((sorted(l), d[praw:praw + rsz], rel))
    return out


def plan(sy, sp, exe, avisa=print):
    """[(direccion, tamano, nombre, objeto)] de los funclets que se pueden colocar, y la cuenta de lo que no."""
    por_nombre = collections.defaultdict(list)
    for f in sy['functions']:
        por_nombre[re.sub(r'__delink_[0-9A-F]{8}$', '', f['name'])].append(f['start'])
    out = []
    c = collections.Counter()
    for obj, v in sorted(sp.items()):
        if obj.startswith('unassigned'):
            continue
        ruta = os.path.join(OBJ, obj.replace('/', os.sep))
        if not os.path.exists(ruta):
            continue
        rangos = v.get('function_ranges') or []
        for sims, code, rel in secciones_x(ruta):
            if not sims[-1][1].startswith('__ehhandler$'):
                c['sin_handler'] += 1
                continue
            fl = [x for x in sims if x[1].startswith('$L')]
            if not fl:
                continue
            eoff, eh = sims[-1]
            cands = [a for a in por_nombre.get(eh, []) if any(lo <= a < hi for lo, hi in rangos)]
            if len(cands) != 1:
                c['handler_fuera_de_la_unidad'] += len(fl)
                continue
            base = cands[0] - eoff
            b = exe.lee(base, len(code))
            if b is None or any(code[j] != b[j] for j in range(len(code)) if j not in rel):
                c['bytes_distintos'] += len(fl)
                continue
            f = eh[len('__ehhandler$'):]
            for i, (val, n) in enumerate(fl):
                fin = sims[sims.index((val, n)) + 1][0]
                out.append((base + val, fin - val, NOMBRE % (f, i), obj))
    return out, c


def main():
    ver = '--ver' in sys.argv
    sal = next((a.split('=', 1)[1] for a in sys.argv if a.startswith('--salida=')), CFG)
    raw = open(os.path.join(CFG, 'symbols.json'), 'rb').read()
    sy = json.loads(raw.decode('utf-8'))
    crlf = b'\r\n' in raw[:200]

    def vuelca(d):
        t = json.dumps(d, indent=2)
        return (t.replace('\n', '\r\n') if crlf else t).encode('utf-8')
    assert vuelca(sy) == raw, 'symbols.json no esta en el formato de siempre (indent 2): no se reescribe'
    sp = json.load(open(os.path.join(CFG, 'splits.json'), encoding='utf-8'))
    pl, c = plan(sy, sp, Exe(EXE))
    fs = sy['functions']
    starts = [f['start'] for f in fs]
    nombres = {}
    for x in sy['names']:
        nombres.setdefault(x['addr'], []).append(x)
    nuevas, renombradas, iguales, choques = [], [], 0, 0
    viejo = {}
    for a, tam, n, obj in pl:
        i = bisect.bisect_left(starts, a)
        if i < len(starts) and starts[i] == a:
            f = fs[i]
            if f['name'] == n and f['end'] == a + tam:
                iguales += 1
            elif f['name'].startswith('Unwind@') and f['end'] == a + tam:  # el de IDA o uno posicional viejo
                viejo[a] = f['name']
                f['name'] = n
                renombradas.append(a)
            else:
                choques += 1
            continue
        if (i > 0 and fs[i - 1]['end'] > a) or (i < len(fs) and fs[i]['start'] < a + tam):
            choques += 1
            continue
        nuevas.append({'start': a, 'end': a + tam, 'name': n, 'thunk': False, 'lib': False, 'static': False,
                       'public': True, 'thunk_target': None})
        viejo[a] = next((x['name'] for x in nombres.get(a, []) if x['name'].startswith('Unwind@')), '(hueco)')
    # la lista `names` (IDA): el `Unwind@<direccion>` de cada funclet colocado pasa a su nombre (si no, delink saca dos
    # simbolos en la misma direccion)
    nuevo_de = {a: n for a, tam, n, obj in pl}
    n_names = 0
    for x in sy['names']:
        n = nuevo_de.get(x['addr'])
        if n and x['name'] != n and x['name'].startswith('Unwind@'):
            x['name'] = n
            n_names += 1
    print('funclets colocados: %d (%d B); ya al dia %d, nuevos %d, renombrados %d, en `names` %d, choques %d'
          % (len(pl), sum(p[1] for p in pl), iguales, len(nuevas), len(renombradas), n_names, choques))
    print('sin colocar:', dict(c))
    if ver or not (nuevas or renombradas or n_names):
        return 0
    sy['functions'] = sorted(fs + nuevas, key=lambda f: f['start'])
    for x, y in zip(sy['functions'], sy['functions'][1:]):
        assert x['end'] <= y['start'], (x, y)
    os.makedirs(sal, exist_ok=True)
    entero(os.path.join(sal, 'symbols.json'), 'wb').write(vuelca(sy))
    pt = os.path.join(CFG, 'nombres_procedencia.tsv')
    t = open(pt, 'rb').read().decode('utf-8')
    nl = '\r\n' if '\r\n' in t else '\n'
    hoy = datetime.date.today().isoformat()
    filas = ['\t'.join([str(a), viejo[a], n, 'eh-funclet', 'seguro', str(tam), '1', hoy])
             for a, tam, n, obj in pl if a in viejo]
    if filas:
        if not t.endswith(nl):
            t += nl
        t += nl.join(filas) + nl
    entero(os.path.join(sal, 'nombres_procedencia.tsv'), 'wb').write(t.encode('utf-8'))
    print('escrito en %s (%d filas de procedencia)' % (sal, len(filas)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
