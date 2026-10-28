"""carbon.py -- utilidades comunes: carga del pickle del DWARF, simbolos del ELF de PS3 y nombres de MW.

Las rutas del prototipo se pasan por variable de entorno CARBON_DIR (carpeta con EBOOT.elf y dw.pkl) o argumento.
Modulo de orden.py, tri_orden.py y tri_calibra.py (no se ejecuta solo).
Pasada a scripts/ en la ronda 78 (procedencia: docs/analisis/2026-10-21_carbonps3.md); sin rutas de la maquina.
"""
import collections
import json
import os
import pickle
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
from cruce_nombres import lee_symbols_txt, lee_splat, lee_splits, unidad_de, undname, gcc_cual  # noqa: E402,F401


def dir_carbon():
    d = os.environ.get('CARBON_DIR')
    if not d:
        raise SystemExit('falta CARBON_DIR (carpeta con EBOOT.elf y dw.pkl)')
    return d


def carga_dw(d=None):
    return pickle.load(open(os.path.join(d or dir_carbon(), 'dw.pkl'), 'rb'))


def nombre_cu(c):
    n = c['name'].replace('\\', '/')
    if '/SourceLists/' in n:
        return n.split('/')[-1]
    m = re.search(r'packages/([^/]+)/([^/]+)/', n, re.I)
    if m:
        return '%s %s' % (m.group(1), m.group(2))
    m = re.search(r'Libs/([^/]+)/([^/]+)/', n)
    if m:
        return '%s %s' % (m.group(1), m.group(2))
    return '/'.join(n.split('/')[-3:])


def simbolos_elf(d=None):
    """{va: [(nombre_sin_punto, tamano, bind)]} de las funciones .text (simbolos con punto del ABI PPC64)."""
    from elftools.elf.elffile import ELFFile
    f = ELFFile(open(os.path.join(d or dir_carbon(), 'EBOOT.elf'), 'rb'))
    st = f.get_section_by_name('.symtab')
    out = {}
    for s in st.iter_symbols():
        if s['st_info']['type'] == 'STT_FUNC' and s.name.startswith('.') and s['st_shndx'] != 'SHN_UNDEF':
            out.setdefault(s['st_value'], []).append((s.name[1:], s['st_size'], s['st_info']['bind']))
    return out


def sin_plantilla(q):
    """Quita los argumentos de plantilla <...> (anidados) y espacios: para comparar MSVC con GCC."""
    if q is None:
        return None
    out = []
    prof = 0
    i = 0
    while i < len(q):
        c = q[i]
        if q.startswith('operator', i):
            # operator<, operator<<, operator<= ... no son plantillas
            m = re.match(r'operator\s*(\(\)|\[\]|<<=|>>=|<<|>>|<=|>=|->\*|->|[-+*/%^&|~!=<>,]=?|new|delete)?', q[i:])
            if prof == 0:
                out.append(m.group(0))
            i += len(m.group(0))
            continue
        if c == '<':
            prof += 1
        elif c == '>':
            prof -= 1
        elif prof == 0 and c != ' ':
            out.append(c)
        i += 1
    return ''.join(out)


NOMBRE_AUTO = re.compile(r'^(fn|lbl|sub|func|nullsub|unknown_libname|j)_[0-9A-Fa-f]+|^(sub|nullsub|j)_')


def nombres_mw():
    """{version: [(nombre_decorado, va, tam, cualificado)]} de las funciones con nombre de MW."""
    cfg = os.path.join(RAIZ, 'config')
    x = [s for s in lee_symbols_txt(os.path.join(cfg, 'EUROPEGERMILESTONE', 'symbols.txt')) if not NOMBRE_AUTO.match(s[0])]
    pcj = json.load(open(os.path.join(cfg, 'SPEED_EXE_1_3', 'symbols.json'), encoding='utf-8'))['functions']
    pc = [(f['name'], f['start'], f['end'] - f['start']) for f in pcj if not NOMBRE_AUTO.match(f['name'])]
    gc = [s for s in lee_symbols_txt(os.path.join(cfg, 'GOWE69', 'symbols.txt')) if not NOMBRE_AUTO.match(s[0])]
    slus = [s for s in lee_splat(os.path.join(cfg, 'SLUS-21351', 'symbols.txt')) if not NOMBRE_AUTO.match(s[0])]
    a124 = [s for s in lee_splat(os.path.join(cfg, 'SLES-53558-A124', 'symbols.txt')) if not NOMBRE_AUTO.match(s[0])]
    und = undname([s[0] for s in x] + [re.sub(r'__[0-9A-F]{8}$', '', s[0]) for s in pc])
    out = {}
    for k, lista, msvc in (('X360', x, True), ('PC', pc, True), ('GC', gc, False), ('SLUS', slus, False),
                           ('A124', a124, False)):
        res = []
        for n, va, sz in lista:
            if msvc:
                n2 = re.sub(r'__[0-9A-F]{8}$', '', n)
                if n2.startswith('?'):
                    q = und.get(n2, n2)
                elif '__' in n2 and gcc_cual(n2):
                    q = gcc_cual(n2)  # nombres al estilo GCC que trae el symbols.json de PC
                else:
                    q = n2  # "Clase::metodo" ya cualificado o funcion C
            else:
                q = gcc_cual(n)
                if q is None and re.match(r'^[A-Za-z_]\w*$', n):
                    q = n  # funcion C
            res.append((n, va, sz, q))
        out[k] = res
    return out


if __name__ == "__main__":
    print(__doc__)
