#!/usr/bin/env python3
"""int3_noreturn.py ENTRADA.json SALIDA.json -- en symbols.json de PC, mete dentro de la funcion el `int3` que MSVC 7.1
pone detras de una llamada a una funcion `__declspec(noreturn)` cuando esa llamada es lo ultimo de la funcion.

La exportacion de IDA termina la funcion en la llamada (no vuelve) y deja fuera ese 0xCC, que parece relleno de
alineacion pero lo emite el compilador: nuestro cl.exe 7.1 lo pone igual (los `_Xlen`/`_Xran` de std::vector acaban en
`call __CxxThrowException@8` + `int3`), y objdiff sacaba esas funciones al 95-99 % solo por esa fila de mas en
nuestro lado. Solo se tocan las llamadas a funciones que las cabeceras de MSVC 7.1 declaran noreturn
(_CxxThrowException en ehdata/eh.h, exit en stdlib.h), y solo si el byte siguiente del exe es 0xCC y no pisa la
funcion de despues. Es idempotente: una funcion ya ampliada acaba en 0xCC y no en un `call`.

Primera pasada (2026-09-24): 117 funciones (115 _CxxThrowException, 2 exit).

Con --splits, ademas mueve en splits.json la frontera de unidad que corte una funcion (delink la rechaza): una funcion
ampliada acababa justo donde terminaba zEAXSound2 y empezaba zEcstasy, en el byte del int3 (relleno sin funcion). Se
edita el texto del fichero, que tiene un formato propio, y es idempotente.

    python scripts/int3_noreturn.py config/SPEED_EXE_1_3/symbols.json config/SPEED_EXE_1_3/symbols.json \\
        --splits config/SPEED_EXE_1_3/splits.json
"""
import bisect
import json
import os
import re
import struct
import sys
from escritura import abre, entero

NORETURN = ('__CxxThrowException@8', '_exit')
EXE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'orig', 'SPEED_EXE_1_3', 'speed.exe')

d = json.load(open(sys.argv[1], encoding='utf-8'))
exe = open(EXE, 'rb').read()
pe, = struct.unpack_from('<I', exe, 0x3c)
nsec, = struct.unpack_from('<H', exe, pe + 6)
opt, = struct.unpack_from('<H', exe, pe + 20)
base, = struct.unpack_from('<I', exe, pe + 24 + 28)
secciones = []
for i in range(nsec):
    vsize, va, _, raw = struct.unpack_from('<IIII', exe, pe + 24 + opt + i * 40 + 8)
    secciones.append((base + va, vsize, raw))


def byte_en(direccion):
    for inicio, tam, raw in secciones:
        if inicio <= direccion < inicio + tam:
            return raw + direccion - inicio
    return None


destinos = set()
for x in d['names']:
    if x['name'] in NORETURN:
        destinos.add(x['addr'])
for f in d['functions']:
    if f['name'] in NORETURN:
        destinos.add(f['start'])
inicios = sorted(f['start'] for f in d['functions'])
ampliadas = 0
for f in d['functions']:
    fin = f['end']
    o = byte_en(fin - 5)
    ob = byte_en(fin)
    if o is None or ob is None or exe[o] != 0xE8 or exe[ob] != 0xCC:
        continue
    destino = (fin + struct.unpack_from('<i', exe, o + 1)[0]) & 0xffffffff
    if destino not in destinos:
        continue
    i = bisect.bisect_right(inicios, fin)
    if i < len(inicios) and inicios[i] <= fin:
        continue
    f['end'] = fin + 1
    ampliadas += 1
print('funciones ampliadas con su int3: %d' % ampliadas)
with abre(sys.argv[2], 'w', encoding='utf-8', newline='\n') as fh:
    json.dump(d, fh, indent=2, ensure_ascii=False)


def arregla_fronteras(ruta, funciones):
    """Mueve al final de la funcion cada fin de rango de unidad que la corte, y el comienzo de la unidad siguiente si
    empezaba en el mismo byte."""
    texto = open(ruta, encoding='utf-8', newline='').read()
    unidades = json.loads(texto)
    rangos = [(a, b) for u in unidades.values() for a, b in u.get('function_ranges', [])]
    inicios_f = sorted((f['start'], f['end']) for f in funciones)
    movidos = 0
    for a, b in rangos:
        i = bisect.bisect_left(inicios_f, (b, -1)) - 1
        if i < 0:
            continue
        f_ini, f_fin = inicios_f[i]
        if not (a <= f_ini < b < f_fin):
            continue
        # el fin del rango y, si existe, el comienzo de la unidad siguiente: el mismo numero en el texto
        esperadas = 1 + sum(1 for x, _ in rangos if x == b)
        patron = re.compile(r'(?<![0-9])%d(?![0-9])' % b)
        assert len(patron.findall(texto)) == esperadas, (b, esperadas)
        texto = patron.sub(str(f_fin), texto)
        movidos += 1
        print('frontera %#x -> %#x' % (b, f_fin))
    entero(ruta, 'w', encoding='utf-8', newline='').write(texto)
    print('fronteras de unidad movidas: %d' % movidos)


if '--splits' in sys.argv:
    arregla_fronteras(sys.argv[sys.argv.index('--splits') + 1], d['functions'])
