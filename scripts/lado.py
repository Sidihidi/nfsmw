#!/usr/bin/env python3
"""lado.py UNIDAD SIMBOLO [OBJ] [--lineas]  -- GameCube: la linea de cada instruccion, original contra nuestro.

Instruccion a instruccion, la linea del .cpp propio en el ORIGINAL (scripts/lmap.py, que funde el .s del troceador
con symbols/debug_lines.txt) y en NUESTRO objeto (la seccion .line de DWARF 1 de build/GOWE69/src/...), lado a lado,
con la diferencia de linea (original menos nuestra).

Para que sirve: un SALTO en esa diferencia es una sentencia que sobra o que falta. Asi salio el codigo muerto de
GenerateRoadNoise (ronda 39): el original tenia dos lineas con solo un `lfs` de 0,5f y 0,1f entre dos sentencias que
nosotros teniamos seguidas; era un clamp que EA escribio y no usa, y su `if` vacio parte el bloque basico.

--lineas: en vez de la tabla, el resumen por lineas. Empareja las filas por posicion y lista (1) las lineas de
nuestro .cpp cuyas instrucciones caen en VARIAS lineas del original (al original le sobra una sentencia: la huella
tipica de un `if`/clamp muerto) y al reves, y (2) los huecos entre lineas con codigo de cada lado, en orden, para
compararlos con los de un caso hermano. Mira tambien scripts/huerfanos.py.

OBJ por defecto: el .o de la unidad bajo build/GOWE69/src (la busca tambien fuera de SourceLists).
"""
import io
import os
import re
import struct
import subprocess
import sys

from elftools.elf.elffile import ELFFile
import capstone

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'build', 'GOWE69', 'src')


def obj_de(unit):
    c = os.path.join(SRC, 'Speed', 'Indep', 'SourceLists', unit + '.o')
    if os.path.exists(c):
        return c
    base = os.path.basename(unit) + '.o'
    for dp, _, fs in os.walk(SRC):
        if base in fs:
            return os.path.join(dp, base)
    raise SystemExit('no encuentro el objeto de %s bajo %s' % (unit, SRC))


def es_propio(nombre):
    return nombre.endswith(('.cpp', '.c'))


def original(unit, sym):
    """[(texto, (fichero, linea) o None)] del original, con la ultima linea propia vista."""
    out = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'lmap.py'), unit, sym],
                         capture_output=True, text=True, cwd=ROOT).stdout
    filas = []
    cur = None
    for ln in out.splitlines()[1:]:
        m = re.match(r'^([0-9A-F]{8})  (.{44})\s*(.*)$', ln)
        if not m:
            continue
        tags = re.findall(r'(\S+?\.(?:cpp|c|h|hpp|inl)):(\d+)', m.group(3))
        if tags:
            prop = [t for t in tags if es_propio(t[0])]
            t = prop[-1] if prop else tags[-1]
            cur = (t[0], int(t[1]))
        filas.append((m.group(2).strip(), cur))
    return filas


def nuestro(obj, sym):
    """Lo mismo para nuestro objeto, leyendo .debug_srcinfo/.line/.debug_sfnames (DWARF 1 de SN)."""
    with open(obj, 'rb') as f:
        e = ELFFile(f)
        cand = [x for x in e.get_section_by_name('.symtab').iter_symbols() if x.name == sym]
        if not cand:
            raise SystemExit('%s no esta en %s' % (sym, obj))
        s = cand[0]
        sec = e.get_section(s['st_shndx'])
        base, size = s['st_value'], s['st_size']
        code = sec.data()[base:base + size]
        if e.get_section_by_name('.debug_srcinfo') is None:
            raise SystemExit('%s no trae .debug_srcinfo (no es DWARF 1 de SN: mwcc o sin -g)' % obj)
        src = e.get_section_by_name('.debug_srcinfo').data()
        line = e.get_section_by_name('.line').data()
        sfn = e.get_section_by_name('.debug_sfnames').data()
    r = io.BytesIO(src)
    ent = {}
    while r.tell() < len(src):
        cu_line, cu_sfn, _, _, _ = struct.unpack('>5I', r.read(20))
        ents = []
        while True:
            lo, so = struct.unpack('>II', r.read(8))
            if so == 0xFFFFFFFF:
                fin = lo
                break
            ents.append((lo, so))
        for i, (lo, so) in enumerate(ents):
            end = fin if i == len(ents) - 1 else ents[i + 1][0]
            name = os.path.basename(sfn[cu_sfn + so:sfn.index(b'\0', cu_sfn + so)].decode())
            p = cu_line + lo
            while p < cu_line + end:
                ln_, _, ad = struct.unpack('>IHI', line[p:p + 10])
                p += 10
                ent.setdefault(ad, []).append((name, ln_))
    md = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)
    md.skipdata = True
    ins = {i.address: (i.mnemonic + ' ' + i.op_str) for i in md.disasm(code, base)}
    filas = []
    cur = None
    for ad in range(base, base + size, 4):
        if ad in ent:
            t = ent[ad]
            prop = [x for x in t if es_propio(x[0])]
            cur = prop[-1] if prop else t[-1]
        filas.append((ins.get(ad, '?'), cur))
    return filas


def por_linea(filas):
    d = {}
    for texto, l in filas:
        if l and es_propio(l[0]):
            d.setdefault(l[1], []).append(texto)
    return d


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if len(args) < 2:
        raise SystemExit(__doc__)
    unit, sym = args[0], args[1]
    obj = args[2] if len(args) > 2 else obj_de(unit)
    orig, nues = original(unit, sym), nuestro(obj, sym)
    if '--lineas' in sys.argv:
        o2n, n2o = {}, {}
        for k in range(min(len(orig), len(nues))):
            a, b = orig[k][1], nues[k][1]
            if a and b and es_propio(a[0]) and es_propio(b[0]):
                o2n.setdefault(a[1], set()).add(b[1])
                n2o.setdefault(b[1], set()).add(a[1])
        print('lineas nuestras que caen en varias del original:')
        for k in sorted(n2o):
            if len(n2o[k]) > 1:
                print('  %5d -> %s' % (k, sorted(n2o[k])))
        print('lineas del original que caen en varias nuestras:')
        for k in sorted(o2n):
            if len(o2n[k]) > 1:
                print('  %5d -> %s' % (k, sorted(o2n[k])))
        for nombre, filas in (('original', orig), ('nuestro', nues)):
            ls = sorted(por_linea(filas))
            print('%s: %d lineas con codigo; huecos %s' % (nombre, len(ls), [b - a for a, b in zip(ls, ls[1:])]))
        return
    for k in range(max(len(orig), len(nues))):
        a = orig[k] if k < len(orig) else ('', None)
        b = nues[k] if k < len(nues) else ('', None)
        la = '%s:%d' % (a[1][0][:24], a[1][1]) if a[1] else ''
        lb = '%s:%d' % (b[1][0][:24], b[1][1]) if b[1] else ''
        dif = ''
        if a[1] and b[1] and a[1][0] == b[1][0]:
            dif = '%+d' % (a[1][1] - b[1][1])
        print('%4d %-30s %-20s | %-30s %-20s %s' % (k, a[0][:30], la, b[0][:30], lb, dif))


if __name__ == '__main__':
    main()
