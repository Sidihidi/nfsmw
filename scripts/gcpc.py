"""gcpc.py -- nombres de PC con la decoracion de GCC 2.95 (Metodo__5Clasef) o la forma de IDA (Clase::Metodo): los
empareja por (clase, metodo) con los simbolos MSVC de NUESTRO objeto de la MISMA unidad (segun splits.json) y propone
el nombre decorado de MSVC (ronda 75: 91 renombres, 0 perdidas; ronda 76: revision a mano de los #tam).

    python scripts/gcpc.py [--tam=0.1] [--aplicar]

Criterio: (clase, metodo) unico en nuestro objeto de esa unidad, unico entre los nombres del exe de esa unidad, el
nombre MSVC no esta ya en symbols.json, y |tam_exe - tam_nuestro| <= tam * max (por defecto 0: igual de tamano; un
nombre suelto de IDA, sin clase, nunca pasa del 5 %). Lo que no cumple el tamano sale como `#tam` y NO se aplica:
hay que mirarlo a mano (en la 76, de 20: 17 eran el mismo metodo con codigo de PC distinto -sobre todo lo online,
`OnlineManager` fuera de linea-, 1 era `bIdentity` con nombre de constructor y 1 el `??_E` de `FEMinList` con el del
destructor). Necesita los objetos de PC compilados (build/SPEED_EXE_1_3/src). Con --aplicar escribe symbols.json y
anota cada cambio en nombres_procedencia.tsv (regla `gcpc`), con scripts/escritura.py.
"""
import bisect
import json
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
from escritura import escribe  # noqa: E402

APLICAR = '--aplicar' in sys.argv
TOL = 0.0
for a in sys.argv[1:]:
    if a.startswith('--tam='):
        TOL = float(a.split('=', 1)[1])


def gc_clave(n):
    """(clases, metodo) de un nombre de GCC 2.95 o de IDA; None si no se entiende."""
    if n.startswith('j_') or n.startswith('nullsub') or n.startswith('sub_'):
        return None
    n = re.sub(r'__[0-9A-F]{8}$', '', n)  # sufijo de direccion de IDA
    if re.match(r'^[A-Za-z]\w*$', n) and '__' not in n and not re.match(
            r'^(sub|nullsub|j|unknown|loc|off|dword|byte|word|flt|dbl|asc|str|stru|unk|lpData|BOOL|UINT|SEH|Unwind)_', n):
        return ('*',), n  # nombre suelto de IDA: cualquier clase (solo con tamano igual)
    if '::' in n and '(' not in n:
        partes = n.split('::')
        if any(not re.match(r'^~?[A-Za-z_]\w*$', p) for p in partes):
            return None
        met = partes[-1]
        clases = tuple(partes[:-1])
        if met.startswith('_') and met[1:] == clases[-1]:
            met = '~' + clases[-1]  # dtor de IDA: Clase::_Clase
        return clases, met
    m = re.match(r'^(_\._)?([A-Za-z_]\w*?)?__(F|Q\d|\d+)(.*)$', n)
    if not m:
        return None
    dtor, met, tipo, resto = m.group(1), m.group(2) or '', m.group(3), m.group(4)
    if tipo == 'F':
        if not met:
            return None
        return (), met
    s = tipo + resto
    clases = []
    if s.startswith('Q'):
        k = int(s[1])
        s = s[2:]
        for _ in range(k):
            mm = re.match(r'^(\d+)', s)
            if not mm:
                return None
            ln = int(mm.group(1))
            s = s[len(mm.group(1)):]
            clases.append(s[:ln])
            s = s[ln:]
    else:
        mm = re.match(r'^(\d+)', s)
        ln = int(mm.group(1))
        s = s[len(mm.group(1)):]
        clases.append(s[:ln])
    if dtor:
        return tuple(clases), '~' + clases[-1]
    if not met:
        return tuple(clases), clases[-1]  # ctor
    if met.startswith('__'):
        return None  # operadores: fuera
    return tuple(clases), met


def ms_clave(n):
    if not n.startswith('?'):
        return None
    if n.startswith('??0') or n.startswith('??1'):
        m = re.match(r'^\?\?([01])((?:[A-Za-z_]\w*@)+)@', n)
        if not m:
            return None
        partes = m.group(2).rstrip('@').split('@')
        clases = tuple(reversed(partes))
        return clases, (clases[-1] if m.group(1) == '0' else '~' + clases[-1])
    if n.startswith('??'):
        return None
    m = re.match(r'^\?([A-Za-z_]\w*)@((?:[A-Za-z_]\w*@)*)@', n)
    if not m:
        return None
    partes = [p for p in m.group(2).split('@') if p]
    return tuple(reversed(partes)), m.group(1)


def coff_funcs(path):
    b = open(path, 'rb').read()
    _, nsec, _, symptr, nsym = struct.unpack_from('<HHIII', b, 0)
    optsz = struct.unpack_from('<H', b, 16)[0]
    secs = []
    for i in range(nsec):
        o = 20 + optsz + 40 * i
        secs.append((b[o:o + 8].rstrip(b'\0'), struct.unpack_from('<I', b, o + 16)[0],
                     struct.unpack_from('<I', b, o + 36)[0]))
    strtab = symptr + 18 * nsym
    out = {}
    i = 0
    while i < nsym:
        o = symptr + 18 * i
        raw = b[o:o + 8]
        if raw[:4] == b'\0\0\0\0':
            p = strtab + struct.unpack_from('<I', raw, 4)[0]
            name = b[p:b.index(b'\0', p)].decode(errors='replace')
        else:
            name = raw.rstrip(b'\0').decode(errors='replace')
        val, sec, typ, cls, aux = struct.unpack_from('<IhHBB', b, o + 8)
        if sec > 0 and (typ >> 4) == 2 and cls == 2 and secs[sec - 1][0].startswith(b'.text'):
            out[name] = secs[sec - 1][1] - val if val == 0 else None
        i += 1 + aux
    return out


def main():
    sj = json.load(open(os.path.join(RAIZ, 'config', 'SPEED_EXE_1_3', 'symbols.json'), encoding='utf-8'))
    sp = json.load(open(os.path.join(RAIZ, 'config', 'SPEED_EXE_1_3', 'splits.json'), encoding='utf-8'))
    ya = set(f['name'] for f in sj['functions'])
    rangos = []
    for unidad, v in sp.items():
        for a, z in v.get('function_ranges', []):
            rangos.append((a, z, unidad))
    rangos.sort()
    ini = [r[0] for r in rangos]

    def unidad_de(a):
        i = bisect.bisect_right(ini, a) - 1
        if i >= 0 and rangos[i][0] <= a < rangos[i][1]:
            return rangos[i][2]
        return None

    exe = {}
    for f in sj['functions']:
        if f['name'].startswith('?'):
            continue
        k = gc_clave(f['name'])
        if not k:
            continue
        u = unidad_de(f['start'])
        if not u:
            continue
        exe.setdefault((u, k), []).append(f)
    objs = {}
    for (u, k), fs in exe.items():
        if u not in objs:
            p = os.path.join(RAIZ, 'build', 'SPEED_EXE_1_3', 'src', u)
            objs[u] = {}
            if os.path.exists(p):
                for n, tam in coff_funcs(p).items():
                    kk = ms_clave(n)
                    if kk:
                        objs[u].setdefault(kk, []).append((n, tam))
    n = 0
    propuestas = []
    for (u, k), fs in sorted(exe.items(), key=lambda x: x[1][0]['start']):
        if k[0] == ('*',):
            cand = [c for kk, cs in objs.get(u, {}).items() if kk[1] == k[1] for c in cs]
        else:
            cand = objs.get(u, {}).get(k, [])
        if len(fs) != 1 or len(cand) != 1:
            continue
        f = fs[0]
        nombre, tam = cand[0]
        if nombre in ya:
            continue
        te = f['end'] - f['start']
        tol = min(TOL, 0.05) if k[0] == ('*',) else TOL
        if tam is None or abs(te - tam) > tol * max(te, tam):
            print('#tam\t%d\t%s\t%s\t%d\t%s' % (f['start'], f['name'], nombre, te, tam))
            continue
        print('%d\t%s\t%s\t%d\t%s' % (f['start'], f['name'], nombre, te, u.split('/')[-1]))
        propuestas.append((f['start'], f['name'], nombre, te))
        n += 1
    print('# %d propuestas' % n, file=sys.stderr)
    if APLICAR and propuestas:
        aplica(propuestas)


def aplica(propuestas):
    """Renombra en symbols.json (edicion textual: conserva el formato) y anota la procedencia."""
    import datetime
    pcj = os.path.join(RAIZ, 'config', 'SPEED_EXE_1_3', 'symbols.json')
    proc = os.path.join(RAIZ, 'config', 'SPEED_EXE_1_3', 'nombres_procedencia.tsv')
    s = open(pcj, newline='', encoding='utf-8').read()
    p = open(proc, newline='', encoding='utf-8').read()
    hoy = datetime.date.today().isoformat()
    filas = []
    for addr, viejo, nuevo, tam in propuestas:
        v = '"name": "%s",' % viejo
        out, pos, k = [], 0, 0
        for m in re.finditer(re.escape(v), s):
            ant = s[max(0, m.start() - 120):m.start()]
            if ('"start": %d,' % addr) in ant or ('"addr": %d,' % addr) in ant:
                out.append(s[pos:m.start()])
                out.append('"name": "%s",' % nuevo)
                pos = m.end()
                k += 1
        out.append(s[pos:])
        if k:
            s = ''.join(out)
            filas.append('%d\t%s\t%s\tgcpc\tseguro\t%d\t0\t%s' % (addr, viejo, nuevo, tam, hoy))
    if not filas:
        print('nada que aplicar')
        return
    escribe(pcj, s, newline='', encoding='utf-8')
    nl = '\r\n' if '\r\n' in p else '\n'
    if not p.endswith(nl):
        p += nl
    escribe(proc, p + nl.join(filas) + nl, newline='', encoding='utf-8')
    print('APLICADOS %d nombres' % len(filas))


if __name__ == '__main__':
    main()
