#!/usr/bin/env python3
"""cruce_nombres.py MAPA.map SALIDA.pkl -- cruza los nombres de funcion de ProStreet (.map de Xbox 360) con los de
las cinco versiones de MW del arbol: X360 (config/EUROPEGERMILESTONE/symbols.txt, decoracion MSVC de PPC), PC
(config/SPEED_EXE_1_3/symbols.json, decoracion MSVC de x86), GC (config/GOWE69) y PS2 (SLUS-21351, SLES-53558-A124)
(decoracion de GCC 2.95, se reduce a Clase::nombre con un parser sencillo).

Comparaciones:
  * X360 contra ProStreet: nombre DECORADO identico (misma firma, misma convencion de PPC);
  * todas: nombre CUALIFICADO (Ambito::...::nombre, sin firma), con undname 0x1000 de MSVC 7.1 para las de MSVC.
Imprime los totales y el reparto por unidad de MW (X360/PC) y por objeto de ProStreet; deja un pickle para los demas.
Como modulo (lee_mapa, lee_symbols_txt, lee_splits, unidad_de, undname, gcc_cual) lo usan carbon.py,
tri_orden.py y tri_calibra.py. Pasada a scripts/ en la ronda 78 desde build/scratch_prostreet (ronda 76).
"""
import collections
import json
import os
import pickle
import re
import subprocess
import sys
import tempfile

if __name__ == '__main__' and (len(sys.argv) < 3 or sys.argv[1] in ('-h', '--help')):
    print(__doc__)
    sys.exit(0)

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def lee_mapa(ruta):
    """{va: (decorado, objeto, estatico)} de las funciones (bandera f) del .map."""
    out = {}
    estatico = False
    rx = re.compile(r'^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s+(f?)\s*(i?)\s+(.*)$')
    for ln in open(ruta, encoding='latin-1'):
        if 'Static symbols' in ln:
            estatico = True
        m = rx.match(ln)
        if not m or m.group(5) != 'f':
            continue
        va = int(m.group(4), 16)
        out.setdefault(va, (m.group(3), m.group(7).strip(), estatico, m.group(6) == 'i'))
    return out


def lee_splat(ruta):
    out = []
    rx = re.compile(r'^(\S+) = 0x([0-9A-Fa-f]+);.*type:func')
    for ln in open(ruta, encoding='utf-8', errors='replace'):
        m = rx.match(ln)
        if m:
            out.append((m.group(1), int(m.group(2), 16), 0))
    return out


def lee_symbols_txt(ruta):
    out = []
    rx = re.compile(r'^(\S+) = \.(\w+):0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)')
    for ln in open(ruta, encoding='utf-8', errors='replace'):
        m = rx.match(ln)
        if m:
            out.append((m.group(1), int(m.group(3), 16), int(m.group(4), 16)))
    return out


def lee_splits(ruta):
    """[(ini, fin, unidad)] de .text."""
    out = []
    unidad = None
    for ln in open(ruta, encoding='utf-8'):
        if ln and not ln[0].isspace() and ln.rstrip().endswith(':'):
            unidad = ln.rstrip()[:-1]
        m = re.match(r'^\s+\.text\s+start:0x([0-9A-F]+) end:0x([0-9A-F]+)', ln)
        if m and unidad:
            out.append((int(m.group(1), 16), int(m.group(2), 16), unidad))
    out.sort()
    return out


def unidad_de(splits, va):
    import bisect
    i = bisect.bisect_right([s[0] for s in splits], va) - 1
    if 0 <= i < len(splits) and splits[i][0] <= va < splits[i][1]:
        return splits[i][2]
    return '?'


def undname(nombres):
    """{decorado: cualificado} con undname 0x1000 (solo nombre) de MSVC 7.1."""
    exe = os.path.join(RAIZ, 'build', 'compilers', 'Win32', '7.1', 'undname.exe')
    nombres = sorted(set(n for n in nombres if n.startswith('?')))
    out = {}
    with tempfile.TemporaryDirectory() as td:
        f = os.path.join(td, 'n.txt')
        for i in range(0, len(nombres), 20000):
            trozo = nombres[i:i + 20000]
            open(f, 'w', encoding='latin-1', newline='\n').write('\n'.join(trozo) + '\n')
            r = subprocess.run([exe, '0x1000', f], capture_output=True)
            lineas = r.stdout.decode('latin-1').replace('\r', '').split('\n')
            lineas = [x for x in lineas if x.strip() != '' or True][:len(trozo)]
            for a, b in zip(trozo, lineas):
                out[a] = b.strip()
    return out


def gcc_cual(n):
    """Cualificado a partir de una decoracion de GCC 2.95: f__5Clase..., f__Q24Attr8Instance..., __5Clase (ctor),
    _$_5Clase (dtor). None si no se entiende."""
    m = re.match(r'^(_\$_|__)(\d+)([A-Za-z_]\w*)', n)
    if m and (m.group(1) == '_$_' or n.startswith('__') and m.group(2)):
        k = int(m.group(2))
        cl = m.group(3)[:k]
        return '%s::%s%s' % (cl, '~' if m.group(1) == '_$_' else '', cl)
    m = re.match(r'^(_\$_|__)Q(\d)(.*)$', n)
    if m:
        ambs, r = _q(int(m.group(2)), m.group(3))
        if ambs:
            cl = ambs[-1].split('<')[0]
            return '::'.join(ambs) + '::' + ('~' if m.group(1) == '_$_' else '') + cl
    m = re.match(r'^([A-Za-z_]\w*?)__(\d+)([A-Za-z_]\w*)', n)
    if m:
        k = int(m.group(2))
        return '%s::%s' % (m.group(3)[:k], m.group(1))
    m = re.match(r'^([A-Za-z_]\w*?)__Q(\d)(.*)$', n)
    if m:
        ambs, r = _q(int(m.group(2)), m.group(3))
        if ambs:
            return '::'.join(ambs) + '::' + m.group(1)
    m = re.match(r'^([A-Za-z_]\w*?)__F', n)
    if m:
        return m.group(1)
    return None


def _q(k, s):
    ambs = []
    if s.startswith('_'):
        s = s[1:]
    for _ in range(k):
        m = re.match(r'^(\d+)(.*)$', s)
        if not m:
            return None, s
        ln = int(m.group(1))
        ambs.append(m.group(2)[:ln])
        s = m.group(2)[ln:]
    return ambs, s


def main():
    mapa, salida = sys.argv[1], sys.argv[2]
    ps = lee_mapa(mapa)
    print('ProStreet: %d funciones en el .map (%d estaticas, %d con nombre C++ decorado)' % (
        len(ps), sum(1 for v in ps.values() if v[2]), sum(1 for v in ps.values() if v[0].startswith('?'))))
    cfg = os.path.join(RAIZ, 'config')
    x = [s for s in lee_symbols_txt(os.path.join(cfg, 'EUROPEGERMILESTONE', 'symbols.txt'))]
    xn = [s for s in x if not re.match(r'^(fn|lbl|sub)_[0-9A-F]+$', s[0])]
    spl = lee_splits(os.path.join(cfg, 'EUROPEGERMILESTONE', 'splits.txt'))
    pcj = json.load(open(os.path.join(cfg, 'SPEED_EXE_1_3', 'symbols.json'), encoding='utf-8'))['functions']
    pcn = [f for f in pcj if not re.match(r'^(sub_|nullsub_|unknown_libname_|j_)', f['name'])]
    gc = [s for s in lee_symbols_txt(os.path.join(cfg, 'GOWE69', 'symbols.txt')) if not s[0].startswith(('fn_', 'func_'))]
    slus = [s for s in lee_splat(os.path.join(cfg, 'SLUS-21351', 'symbols.txt')) if not s[0].startswith(('fn_', 'func_'))]
    a124 = [s for s in lee_splat(os.path.join(cfg, 'SLES-53558-A124', 'symbols.txt')) if not s[0].startswith(('fn_', 'func_'))]
    print('MW X360: %d funciones, %d con nombre; PC: %d, %d con nombre; GC %d, SLUS %d, A124 %d con nombre' % (
        len(x), len(xn), len(pcj), len(pcn), len(gc), len(slus), len(a124)))
    und = undname([v[0] for v in ps.values()] + [s[0] for s in xn] + [f['name'] for f in pcn])
    def q(n):
        return und.get(n, n)
    ps_dec = collections.defaultdict(list)
    ps_q = collections.defaultdict(list)
    for va, (n, obj, est, inl) in ps.items():
        ps_dec[n].append(va)
        ps_q[q(n)].append(va)
    # X360 decorado y cualificado
    res = {}
    por_unidad = collections.defaultdict(lambda: [0, 0, 0])
    x_dec = x_q = 0
    for n, va, sz in xn:
        u = unidad_de(spl, va)
        por_unidad[u][0] += 1
        if n in ps_dec:
            x_dec += 1
            por_unidad[u][1] += 1
        if q(n) in ps_q:
            x_q += 1
            por_unidad[u][2] += 1
    print('\nX360 con nombre: %d; con homonimo DECORADO en ProStreet: %d (%.1f %%); CUALIFICADO: %d (%.1f %%)' % (
        len(xn), x_dec, 100.0 * x_dec / len(xn), x_q, 100.0 * x_q / len(xn)))
    print('  por unidad de MW (con nombre / decorado igual / cualificado igual):')
    for u, (a, b, c) in sorted(por_unidad.items(), key=lambda t: -t[1][0]):
        print('   %-55s %6d %6d %6d' % (u.replace('Speed/Indep/SourceLists/', ''), a, b, c))
    pc_q = sum(1 for f in pcn if q(f['name']) in ps_q)
    print('\nPC con nombre: %d; cualificado igual en ProStreet: %d (%.1f %%)' % (len(pcn), pc_q, 100.0 * pc_q / len(pcn)))
    for nombre, lista in (('GC', gc), ('SLUS', slus), ('A124', a124)):
        cu = [gcc_cual(s[0]) for s in lista]
        ok = sum(1 for c in cu if c and c in ps_q)
        ent = sum(1 for c in cu if c)
        print('%s: %d con nombre, %d entendidos, %d con homonimo cualificado en ProStreet (%.1f %%)' % (
            nombre, len(lista), ent, ok, 100.0 * ok / max(ent, 1)))
    # por objeto de ProStreet: cuantas de sus funciones tienen homonimo en MW (X360 o PC)
    mw_q = set(q(n) for n, _, _ in xn) | set(q(f['name']) for f in pcn) | \
        set(c for c in (gcc_cual(s[0]) for s in gc + slus + a124) if c)
    po = collections.defaultdict(lambda: [0, 0])
    for va, (n, obj, est, inl) in ps.items():
        k = obj.split(':')[0] if ':' in obj and not obj.startswith('CIL') else obj
        po[k][0] += 1
        if q(n) in mw_q:
            po[k][1] += 1
    print('\nProStreet por objeto (funciones / con homonimo cualificado en alguna version de MW):')
    tot = [0, 0]
    for k, (a, b) in sorted(po.items(), key=lambda t: -t[1][0])[:70]:
        print('   %-60s %6d %6d  %5.1f %%' % (k[:60], a, b, 100.0 * b / a))
    for a, b in po.values():
        tot[0] += a
        tot[1] += b
    print('   TOTAL %d / %d' % tuple(tot))
    pickle.dump({'ps': ps, 'und': und}, open(salida, 'wb'))


if __name__ == '__main__':
    main()
