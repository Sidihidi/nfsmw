#!/usr/bin/env python3
"""nombres_pc_upstream.py -- trae nombres de PC (SPEED_EXE_1_3) del symbols.json de upstream, decidiendo por lo que
DEFINEN nuestros objetos de PC compilados: objdiff empareja por nombre, asi que un nombre solo vale si nuestro .obj
lo tiene.

En cada direccion donde upstream y nosotros nombramos distinto: se toma el de upstream si nuestros objetos lo definen
y el nuestro no (el nuestro era un marcador: sub_, unknown_libname, Clase::_unk, una firma de GameCube); se deja el
nuestro si los dos existen, si ninguno existe, o si el de upstream ya esta en otra direccion nuestra. Nunca quita un
nombre que emparejaba, asi que no puede bajar nada.

    git show upstream/main:config/SPEED_EXE_1_3/symbols.json > up.json
    python scripts/nombres_pc_upstream.py up.json            # informa
    python scripts/nombres_pc_upstream.py up.json --aplicar  # escribe config/SPEED_EXE_1_3/symbols.json
Despues, medir PC con scripts/medir.py SPEED_EXE_1_3. Primera pasada (2026-09-23, upstream a90a40c4): +931 funciones.
"""
import collections
import glob
import json
import os
import re
import sys

R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(R, 'tools'))
from nombra_por_codigo import Coff  # noqa: E402
from escritura import abre  # noqa: E402

up = json.load(open(sys.argv[1], encoding='utf-8'))


def _entero(x):
    """Upstream escribe ahora las direcciones en hexadecimal ("0x401000"); antes eran enteros. Sin convertir, ninguna
    direccion coincidia y el script decia "0 de 0" sin error (ronda 38)."""
    return int(x, 16) if isinstance(x, str) else x


for _f in up.get('functions', []):
    _f['start'] = _entero(_f['start'])
    if _f.get('end') is not None:
        _f['end'] = _entero(_f['end'])
for _f in up.get('names', []):
    if 'addr' in _f:
        _f['addr'] = _entero(_f['addr'])
ruta_nuestra = os.path.join(R, 'config', 'SPEED_EXE_1_3', 'symbols.json')
ours = json.load(open(ruta_nuestra, encoding='utf-8'))

definidos = set()
for obj in glob.glob(os.path.join(R, 'build', 'SPEED_EXE_1_3', 'src', '**', '*.obj'), recursive=True):
    try:
        c = Coff(obj)
    except Exception as e:
        print('aviso: no leo', obj, e)
        continue
    for nombre, _b, _rel, _pos in c.funciones():
        definidos.add(nombre)
print('funciones definidas en nuestros objetos de PC:', len(definidos))

PLACE = re.compile(r'^(sub_|nullsub_|j_|loc_|unknown_libname|fn_)')
u = {f['start']: f['name'] for f in up.get('functions', [])}
por_dir = {f['start']: f for f in ours.get('functions', [])}
por_nombre = collections.defaultdict(list)
for f in ours.get('functions', []):
    por_nombre[f['name']].append(f['start'])

tomar, ambos, choque, nada = [], [], [], 0
for a, nu in u.items():
    f = por_dir.get(a)
    if f is None or f['name'] == nu:
        continue
    no = f['name']
    nu_ok = nu in definidos
    no_ok = no in definidos
    if nu_ok and not no_ok:
        otros = [x for x in por_nombre.get(nu, []) if x != a]
        if otros:
            choque.append((a, no, nu, otros))
        else:
            tomar.append((a, no, nu))
    elif nu_ok and no_ok:
        ambos.append((a, no, nu))
    else:
        nada += 1

print('se toman de upstream:', len(tomar))
print('los dos nombres existen en nuestros objetos (se deja el nuestro):', len(ambos))
print('el de upstream ya esta en otra direccion nuestra (se deja):', len(choque))
print('ni uno ni otro en nuestros objetos (sin efecto, se deja):', nada)
for a, no, nu in tomar[:15]:
    print('  %x  %-50s -> %s' % (a, no[:50], nu[:80]))

if '--aplicar' in sys.argv:
    for a, no, nu in tomar:
        por_dir[a]['name'] = nu
    with abre(ruta_nuestra, 'w', encoding='utf-8', newline='\n') as fh:
        json.dump(ours, fh, indent=2, ensure_ascii=False)
    print('escrito', ruta_nuestra)
