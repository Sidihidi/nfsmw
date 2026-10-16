#!/usr/bin/env python3
"""yc_renumera.py UNIDAD VIEJO.obj NUEVO.obj [--ver] [--salida=DIR] -- PC: pone al dia en
config/SPEED_EXE_1_3/symbols.json los nombres `_$E<n>` de una unidad cuando un cambio de fuente mueve el contador
(ronda 50, PC50).

MSVC 7.1 numera los inicializadores dinamicos (`_$E<n>`, .text$yc) y sus destructores de atexit (.text$yd) con un
contador de la unidad, y PC49 (ronda 49) nombro con ese numero las funciones del exe que les corresponden. Si un cambio
anade o quita un inicializador antes (otra cabecera delante, un global nuevo), los de detras cambian de numero, el mapeo
de objdiff deja de casar y la unidad pierde esas funciones. Este script compara el objeto de la unidad ANTES del cambio
con el de DESPUES (los dos de nuestro cl, p. ej. dos corridas de scripts/bancopc.py con --conservar): empareja cada
`_$E` viejo con el nuevo por seccion, tamano, bytes (sin los de las reubicaciones) y destinos de sus reubicaciones (con
los `_$E` normalizados), en orden; y renombra en symbols.json las funciones de esa unidad que lleven un nombre viejo.
Si una clave tiene distinto numero de funciones antes y despues, ese grupo no se toca (lo dice).

    python scripts/yc_renumera.py zPlatform build/bancopc/SPEED_EXE_1_3_zPlatform_ctl.obj \\
           build/bancopc/SPEED_EXE_1_3_zPlatform_nuevo.obj --ver
"""
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
E = re.compile(r'^_\$E\d+$')


def inicializadores(ruta):
    """[(clave, nombre)] de las funciones de las secciones .text$yc/.text$yd, en orden del objeto."""
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
        rsz, praw, prel, _, nrel = struct.unpack_from('<IIIIH', d, o + 16)
        secs.append((d[o:o + 8].rstrip(b'\0').decode('latin-1'), rsz, praw, prel, nrel))
    sims = {}
    porsec = collections.defaultdict(list)
    i = 0
    while i < nsym:
        o = psym + 18 * i
        valor, sec, tipo, clase, naux = struct.unpack_from('<IhHBB', d, o + 8)
        n = nombre(d[o:o + 8])
        sims[i] = n
        if sec > 0 and secs[sec - 1][0] in ('.text$yc', '.text$yd') and E.match(n):
            porsec[sec].append((valor, n))
        i += 1 + naux
    out = []
    for sec in sorted(porsec):
        nm, rsz, praw, prel, nrel = secs[sec - 1]
        code = bytearray(d[praw:praw + rsz])
        destinos = []
        for r in range(nrel):
            va, si, typ = struct.unpack_from('<IIH', d, prel + 10 * r)
            code[va:va + 4] = b'\0\0\0\0'
            destinos.append((va, '_$E' if E.match(sims[si]) else sims[si]))
        l = sorted(porsec[sec])
        for k, (v, n) in enumerate(l):
            fin = l[k + 1][0] if k + 1 < len(l) else rsz
            clave = (nm, fin - v, bytes(code[v:fin]), tuple(t for va, t in destinos if v <= va < fin))
            out.append((clave, n))
    return out


def renombres(viejo, nuevo, avisa=print):
    a, b = collections.defaultdict(list), collections.defaultdict(list)
    for clave, n in inicializadores(viejo):
        a[clave].append(n)
    for clave, n in inicializadores(nuevo):
        b[clave].append(n)
    out = {}
    for clave, ns in a.items():
        ms = b.get(clave, [])
        if len(ms) != len(ns):
            avisa('grupo %s de %d B: %d antes y %d despues, no se toca (%s)' % (clave[0], clave[1], len(ns), len(ms),
                                                                              ' '.join(ns)))
            continue
        for n, m in zip(ns, ms):
            if n != m:
                out[n] = m
    return out


def main():
    args = [x for x in sys.argv[1:] if not x.startswith('--')]
    if len(args) != 3:
        print(__doc__)
        return 2
    unidad, viejo, nuevo = args
    ver = '--ver' in sys.argv
    sal = next((x.split('=', 1)[1] for x in sys.argv if x.startswith('--salida=')), CFG)
    ren = renombres(viejo, nuevo)
    raw = open(os.path.join(CFG, 'symbols.json'), 'rb').read()
    sy = json.loads(raw.decode('utf-8'))
    crlf = b'\r\n' in raw[:200]

    def vuelca(x):
        t = json.dumps(x, indent=2)
        return (t.replace('\n', '\r\n') if crlf else t).encode('utf-8')
    assert vuelca(sy) == raw, 'symbols.json no esta en el formato de siempre (indent 2): no se reescribe'
    sp = json.load(open(os.path.join(CFG, 'splits.json'), encoding='utf-8'))
    rangos = sp['Speed/Indep/SourceLists/%s.obj' % unidad]['function_ranges']
    cambios = []
    for f in sy['functions']:
        if E.match(f['name']) and any(lo <= f['start'] < hi for lo, hi in rangos) and f['name'] in ren:
            cambios.append((f['start'], f['name'], ren[f['name']]))
            f['name'] = ren[f['name']]
    porad = {a: m for a, n, m in cambios}
    for x in sy['names']:
        if x['addr'] in porad and E.match(x['name']):
            x['name'] = porad[x['addr']]
    for a, n, m in sorted(cambios):
        print('%X  %s -> %s' % (a, n, m))
    print('%s: %d renombres en symbols.json (de %d en el objeto)' % (unidad, len(cambios), len(ren)))
    if ver or not cambios:
        return 0
    os.makedirs(sal, exist_ok=True)
    entero(os.path.join(sal, 'symbols.json'), 'wb').write(vuelca(sy))
    pt = os.path.join(CFG, 'nombres_procedencia.tsv')
    t = open(pt, 'rb').read().decode('utf-8')
    nl = '\r\n' if '\r\n' in t else '\n'
    if not t.endswith(nl):
        t += nl
    hoy = datetime.date.today().isoformat()
    t += nl.join('\t'.join([str(a), n, m, 'yc-renumera', 'seguro', '0', '1', hoy]) for a, n, m in sorted(cambios)) + nl
    entero(os.path.join(sal, 'nombres_procedencia.tsv'), 'wb').write(t.encode('utf-8'))
    print('escrito en', sal)
    return 0


if __name__ == '__main__':
    sys.exit(main())
