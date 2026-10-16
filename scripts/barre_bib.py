#!/usr/bin/env python3
"""barre_bib.py -- PC (speed.exe 1.3): funciones de las bibliotecas con bytes identicos a las nuestras fuera de su
unidad o con otro nombre (la "veta" de PC51 en snd, generalizada a todas las bibliotecas).

    python scripts/barre_bib.py [--lib PREFIJO ...] [--juego] [--min N] [--margen N] [--salida F.tsv] [--aplicar]
                                [--regla R] [--respeta-decorados]

Para cada objeto NUESTRO de PC ya compilado (build/SPEED_EXE_1_3/src/**/*.obj, con los flags de configure.py; hace
falta una medida de PC antes) de una biblioteca (por defecto todas las de src/Speed/Indep/Libs y src/Packages), busca
cada funcion de >= N bytes (12 por defecto) en el tramo del exe de esa biblioteca (del primer al ultimo rango de sus
unidades en splits.json, mas un margen) con las reubicaciones enmascaradas. Una funcion que sale UNA sola vez y cuyo
nombre en symbols.json no es el nuestro es candidata:

    direccion  bytes  nuestro_nombre  nombre_actual  unidad_duena  unidad_nuestra  accion

accion: `nombrar` (la direccion ya es de nuestra unidad: solo el nombre), `mover` (esta en un unassigned_*: se pasa el
rango a nuestra unidad y se nombra), `une` (IDA corto la funcion: los trozos de dentro, con nombres de FLIRT como
`?_Get_deleter@_Ref_count_base...` o sub_, no los llama ni los referencia nadie; se quitan y la funcion llega a su
final), `parte` (solo se informa: la funcion de symbols.json es mas larga que la nuestra; casi siempre es el bloque
catch de la propia funcion, que nuestro objeto lleva aparte, asi que no se parte: ronda 52, 4 de 4), o por que se
salta: `otra-unidad` (la direccion es de otra unidad: frontera mal puesta, a mano), `pisa` (se
monta sobre otra funcion), `tamano` (otro tamano que no se arregla solo: un trozo de dentro tiene referencias o sale
de la unidad), `nombre-en-uso` (nuestro nombre ya esta en otra direccion: o lo tiene bien otra copia o hay que
quitarlo a mano), `cruza` (el rango cruza el final del unassigned), `ambigua` (la misma direccion la reclaman
varios objetos o varios nombres: una funcion en linea o estatica que sale en varias unidades, o sobrecargas con el
mismo codigo).

--aplicar escribe symbols.json (functions y names), splits.json y nombres_procedencia.tsv (regla `barre-bib` o la de
--regla, marca `seguro`) SOLO con `nombrar`, `mover`, `une` (solo dentro de nuestra unidad), `nombrar-comdat` y
`mover-comdat`. Las `-comdat` son funciones en
linea con un solo nombre que salen en varias de nuestras unidades: son de la que tiene la direccion dentro de un
rango suyo, o de la unidad cuyo rango acaba justo donde empieza el hueco sin unidad en que caen (el enlazador pone la
copia del primer objeto que la define en la parte de ese objeto); si no se puede decir, siguen `ambigua`.
Comprueba antes que el JSON se reescribe byte a byte igual.
Sin --aplicar no escribe nada en config/. La medida de PC (scripts/medir.py) confirma despues lo que casa.

--juego anade las unidades de juego (src/Speed/Indep/SourceLists), cada una en su propio tramo: con ellas sale sobre
todo `nombrar` (una funcion nuestra con el codigo identico a una sin nombre, o con un nombre de IDA o de GameCube, de
la misma unidad). Lo que el enlazador pliega (/OPT:ICF: varias funciones nuestras con el mismo codigo en una sola
direccion) sale `ambigua` y no se toca. Codigo identico con las reubicaciones enmascaradas no prueba el nombre si las
reubicaciones van a otro sitio (dos envolturas con la misma forma y distinto destino): despues de --aplicar, mide y
devuelve a su nombre anterior lo que no llegue al 100 % (--respeta-decorados deja ademas los nombres decorados de
MSVC que ya esten puestos).

Diferencias con el barre_snd.py de PC51: usa los objetos que ya compila la medida (los flags reales de cada
biblioteca, sin copiarlos aqui), vale para cualquier biblioteca y no corta los nombres. No crea unidades nuevas (un
fichero sin unidad en config.yml no tiene objeto): esas siguen siendo trabajo a mano.
"""
import datetime
import glob
import json
import os
import re
import struct
import sys
from escritura import entero

RAIZ = os.environ.get('BARRE_RAIZ') or os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
V = 'SPEED_EXE_1_3'
EXE = os.path.join(RAIZ, 'orig', V, 'speed.exe')
CFG = os.path.join(RAIZ, 'config', V)
OBJS = os.path.join(RAIZ, 'build', V, 'src')


def arg(nombre, defecto=None, varios=False):
    vals = [a.split('=', 1)[1] for a in sys.argv[1:] if a.startswith(nombre + '=')]
    if varios:
        return vals or defecto
    return vals[-1] if vals else defecto


def seccion_text(ruta):
    d = open(ruta, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3C)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
    for i in range(nsec):
        o = pe + 24 + opt + 40 * i
        if d[o:o + 8].rstrip(b'\0') == b'.text':
            vs, va, rs, ro = struct.unpack_from('<IIII', d, o + 8)
            return base + va, d[ro:ro + rs]
    raise SystemExit('speed.exe sin .text')


def funciones_coff(ruta):
    """{nombre: (bytes, posiciones reubicadas)} de las funciones de un COFF x86."""
    d = open(ruta, 'rb').read()
    _, nsec, _, symp, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
    secs = []
    for i in range(nsec):
        o = 20 + opt + 40 * i
        vs, va, rs, rp, relp, lnp, nrel, nln, chr_ = struct.unpack_from('<IIIIIIHHI', d, o + 8)
        rels = [struct.unpack_from('<I', d, relp + 10 * j)[0] for j in range(nrel)]
        secs.append((d[rp:rp + rs], rels))
    strt = symp + 18 * nsym
    porsec = {}
    i = 0
    while i < nsym:
        o = symp + 18 * i
        nm = d[o:o + 8]
        if nm[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', nm, 4)[0]
            nombre = d[strt + off:d.index(b'\0', strt + off)].decode('latin1')
        else:
            nombre = nm.rstrip(b'\0').decode('latin1')
        val, sec, typ, cls, naux = struct.unpack_from('<IhHBB', d, o + 8)
        if sec > 0 and typ == 0x20:
            porsec.setdefault(sec, []).append((val, nombre))
        i += 1 + naux
    out = {}
    for sec, lista in porsec.items():
        raw, rels = secs[sec - 1]
        lista.sort()
        for k, (val, nombre) in enumerate(lista):
            fin = lista[k + 1][0] if k + 1 < len(lista) else len(raw)
            mascara = set()
            for r in rels:
                if val <= r < fin:
                    mascara.update(range(r - val, r - val + 4))
            out[nombre] = (raw[val:fin], mascara)
    return out


def patron(codigo, mascara):
    return b''.join((b'.' if i in mascara else re.escape(codigo[i:i + 1])) for i in range(len(codigo)))


def main():
    minimo = int(arg('--min', '12'))
    margen = int(arg('--margen', '0x2000'), 0)
    libs = arg('--lib', None, varios=True)
    juego = '--juego' in sys.argv
    respeta = '--respeta-decorados' in sys.argv
    aplicar = '--aplicar' in sys.argv
    regla = arg('--regla', 'barre-bib')
    salida = arg('--salida')

    va_text, text = seccion_text(EXE)
    sy_p = os.path.join(CFG, 'symbols.json')
    sp_p = os.path.join(CFG, 'splits.json')
    sy_raw = open(sy_p, 'rb').read()
    sp_raw = open(sp_p, 'rb').read()
    sy = json.loads(sy_raw.decode('utf-8'))
    sp = json.loads(sp_raw.decode('utf-8'))
    funcs = sy['functions']
    porad = {f['start']: f for f in funcs}
    nombres = {}
    for f in funcs:
        nombres.setdefault(f['name'], f['start'])
    rangos = sorted((a, b, k) for k, v in sp.items() for a, b in v.get('function_ranges', []))
    referidas = set(r['target'] for r in sy.get('relocations', []))
    for i in range(len(text) - 5):
        if text[i] in (0xE8, 0xE9):
            referidas.add(va_text + i + 5 + struct.unpack_from('<i', text, i + 1)[0])

    def frontera(f, a, fin, du):
        # IDA corto la funcion (un trozo final con un nombre de FLIRT o un sub_ que nadie llama) o junto dos
        e = f['end']
        if e < fin:
            dentro = [x for x in funcs if e <= x['start'] < fin]
            if any(x['end'] > fin or x['start'] in referidas for x in dentro) or dueno(fin - 1) != du:
                return 'tamano', None
            return 'une', dentro
        resto = fin
        while resto < e and text[resto - va_text] == 0xCC:
            resto += 1
        return 'parte', resto

    def dueno(a):
        for x, y, k in rangos:
            if x <= a < y:
                return k
        return None

    objs = sorted(glob.glob(os.path.join(OBJS, '**', '*.obj'), recursive=True))
    grupos = {}
    for o in objs:
        rel = os.path.relpath(o, OBJS).replace('\\', '/')
        m = re.match(r'(Speed/Indep/Libs/[^/]+|Packages/[^/]+)/', rel)
        if m:
            lib = m.group(1)
        elif juego and rel.startswith('Speed/Indep/SourceLists/'):
            lib = rel  # cada unidad de juego es su propio grupo: se busca en su propio tramo
        else:
            continue
        if libs and not any(rel.startswith(p) or lib.startswith(p) for p in libs):
            continue
        grupos.setdefault(lib, []).append((rel, o))

    filas = []
    for lib, lista in sorted(grupos.items()):
        rs = [r for rel, _ in lista for r in sp.get(rel, {}).get('function_ranges', [])]
        if not rs:
            print('%-40s sin rangos en splits.json: se salta' % lib)
            continue
        lo = max(va_text, min(a for a, b in rs) - margen)
        hi = min(va_text + len(text), max(b for a, b in rs) + margen)
        mem = text[lo - va_text:hi - va_text]
        n_lib = 0
        for rel, o in lista:
            for nombre, (codigo, mascara) in funciones_coff(o).items():
                if (len(codigo) < minimo or nombre.startswith('$') or nombre.startswith('??_C')
                        or nombre.startswith('__ehhandler') or nombre.startswith('_$E')):
                    continue
                hits = [lo + m.start() for m in re.finditer(patron(codigo, mascara), mem, re.S)]
                if len(hits) != 1:
                    continue
                a = hits[0]
                f = porad.get(a)
                actual = f['name'] if f else '(sin funcion)'
                if actual == nombre:
                    continue
                fin = a + len(codigo)
                du = dueno(a) or '(sin unidad)'
                if nombres.get(nombre, a) != a:
                    accion = 'nombre-en-uso'
                elif f and f['end'] != fin:
                    accion = frontera(f, a, fin, du)[0]
                elif any(x['start'] < fin and x['end'] > a and x['start'] != a
                         for x in funcs if x['start'] < fin + 0x1000 and x['end'] > a - 0x1000):
                    accion = 'pisa'
                elif du == rel:
                    accion = 'nombrar'
                elif du.startswith('unassigned_'):
                    x, y = next((x, y) for x, y in sp[du]['function_ranges'] if x <= a < y)
                    accion = 'mover' if fin <= y else 'cruza'
                else:
                    accion = 'otra-unidad'
                filas.append((a, len(codigo), nombre, actual, du, rel, accion))
                n_lib += 1
        print('%-40s %3d objetos, tramo %X-%X: %d candidatas' % (lib, len(lista), lo, hi, n_lib))

    # una direccion que reclaman varios objetos (una funcion en linea o estatica que sale en varias unidades) o con
    # varios nombres (sobrecargas con el mismo codigo) no se sabe de quien es: `ambigua`
    pordir = {}
    for f in filas:
        pordir.setdefault(f[0], set()).add((f[2], f[5]))

    def resuelve(f):
        # Una funcion en linea con UN solo nombre que sale en varias de nuestras unidades: el enlazador se queda con
        # la copia del primer objeto que la define y la pone en la parte de ese objeto. Es de la unidad que la
        # reclama si la direccion cae dentro de un rango de esa unidad, o si cae en un hueco sin unidad que empieza
        # justo donde acaba un rango de esa unidad (`nombrar`/`mover`, con la regla `...-comdat` en la procedencia).
        quienes = pordir[f[0]]
        if len(quienes) == 1:
            return f
        if len(set(n for n, u in quienes)) != 1 or f[6] in ('nombre-en-uso', 'tamano', 'pisa', 'cruza'):
            return f[:6] + ('ambigua',)
        unidades = set(u for n, u in quienes)
        du = f[4]
        if du in unidades:
            return f[:5] + (du, 'nombrar-comdat')
        if du.startswith('unassigned_'):
            x, y = next((x, y) for x, y in sp[du]['function_ranges'] if x <= f[0] < y)
            if f[0] + f[1] <= y:
                previa = [u for u in unidades if any(b == x for a, b in sp.get(u, {}).get('function_ranges', []))]
                if len(previa) == 1:
                    return f[:5] + (previa[0], 'mover-comdat')
        return f[:6] + ('ambigua',)
    filas = [resuelve(f) for f in filas]
    vistos = set()
    filas = [f for f in filas if not (f[:3] in vistos or vistos.add(f[:3]))]
    filas.sort()
    texto = ''.join('%X\t%d\t%s\t%s\t%s\t%s\t%s\n' % f for f in filas)
    if salida:
        open(salida, 'w', encoding='utf-8').write('direccion\tbytes\tnuestro\tactual\tduena\tnuestra\taccion\n' + texto)
    cuenta = {}
    for f in filas:
        cuenta[f[6]] = cuenta.get(f[6], 0) + 1
    print('acciones:', ', '.join('%s %d' % kv for kv in sorted(cuenta.items())) or 'ninguna')

    if not aplicar:
        return 0
    # el JSON tiene que reescribirse igual byte a byte (formato de configure.py)
    for raw, d in ((sy_raw, sy), (sp_raw, sp)):
        t = json.dumps(json.loads(raw.decode('utf-8')), indent=2)
        if (t.replace('\n', '\r\n') if b'\r\n' in raw[:300] else t).encode('utf-8') != raw:
            raise SystemExit('el JSON no se reescribe igual: no se aplica nada')
    ACEPTADAS = ('nombrar', 'mover', 'nombrar-comdat', 'mover-comdat', 'une')
    procs = []
    usados = set()
    hechas = set()
    for a, tam, nombre, actual, du, rel, accion in filas:
        if accion not in ACEPTADAS or nombre in usados or a in hechas:
            continue
        if respeta and actual.startswith('?'):
            continue  # un nombre decorado de MSVC ya puesto no se toca con --respeta-decorados
        fin = a + tam
        if accion in ('mover', 'mover-comdat'):
            u = sp[du]['function_ranges']
            k = next(i for i, (x, y) in enumerate(u) if x <= a < y)
            x, y = u[k]
            u[k:k + 1] = [r for r in ([x, a], [fin, y]) if r[1] > r[0]]
            sp[rel]['function_ranges'].append([a, fin])
            sp[rel]['function_ranges'].sort()
        f = porad.get(a)
        if accion in ('une', 'parte'):
            if du != rel:
                continue  # solo dentro de nuestra unidad
            tipo, extra = frontera(f, a, fin, du)
            if tipo != accion:
                continue
            if accion == 'une':
                quita = set(x['start'] for x in extra)
                funcs[:] = [x for x in funcs if x['start'] not in quita]
                sy['names'] = [x for x in sy['names'] if not (x['addr'] in quita and x.get('is_func'))]
            else:
                e = f['end']
                if extra < e:
                    i = next(i for i, x in enumerate(funcs) if x['start'] > a)
                    funcs.insert(i, {"start": extra, "end": e, "name": 'sub_%X' % extra, "thunk": False,
                                     "lib": False, "static": False, "public": True, "thunk_target": None})
            f['end'] = fin
        if f:
            f['name'] = nombre
        else:
            i = next(i for i, x in enumerate(funcs) if x['start'] > a)
            funcs.insert(i, {"start": a, "end": fin, "name": nombre, "thunk": False, "lib": False, "static": False,
                             "public": True, "thunk_target": None})
        for x in sy['names']:
            if x['addr'] == a and x['name'] != nombre:
                x['name'] = nombre
        usados.add(nombre)
        hechas.add(a)
        procs.append((a, actual, nombre, tam, regla + ('-comdat' if accion.endswith('-comdat') else '')
                      + ('-' + accion if accion in ('une', 'parte') else '')))
    for raw, d, p in ((sy_raw, sy, sy_p), (sp_raw, sp, sp_p)):
        t = json.dumps(d, indent=2)
        entero(p, 'wb').write((t.replace('\n', '\r\n') if b'\r\n' in raw[:300] else t).encode('utf-8'))
    q = os.path.join(CFG, 'nombres_procedencia.tsv')
    pr = open(q, newline='').read()
    nl = '\r\n' if '\r\n' in pr else '\n'
    hoy = datetime.date.today().isoformat()
    for a, viejo, nombre, tam, r in procs:
        pr += '%d\t%s\t%s\t%s\tseguro\t%d\t0\t%s%s' % (a, viejo, nombre, r, tam, hoy, nl)
    entero(q, 'w', newline='').write(pr)
    print('aplicadas', len(procs))
    return 0


if __name__ == '__main__':
    sys.exit(main())
