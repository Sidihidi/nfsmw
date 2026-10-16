#!/usr/bin/env python3
"""quita_solapadas.py ENTRADA.json SALIDA.json -- quita de symbols.json de PC las reubicaciones falsas que la
exportacion de IDA pone UN byte antes de otra al mismo destino (1.748 pares). En `mov dword ptr [reg+disp8], imm32`
(C7 4x dd ii ii ii ii) la buena esta en el imm32 y la falsa en el disp8: objdiff pone a cero los 4 bytes de la falsa y
el original sale como `mov dword ptr [reg], 0x0`, con lo que la funcion no casa aunque el codigo sea identico.

Dos reubicaciones de 4 bytes solapadas son imposibles en codigo valido, asi que se quita la de delante cuando la
de un byte despues tiene el mismo destino y el mismo tamano. Primera pasada (2026-09-23): 105.814 -> 104.066
reubicaciones y +242 funciones de PC (+56.060 B), 0 perdidas. Pasarlo tras traer nombres o reubicaciones de
upstream (su symbols.json viene de la misma exportacion).

    python scripts/quita_solapadas.py config/SPEED_EXE_1_3/symbols.json config/SPEED_EXE_1_3/symbols.json
"""
import json
import sys

from escritura import abre

d = json.load(open(sys.argv[1], encoding='utf-8'))
rel = d['relocations']
por_dir = {}
for r in rel:
    por_dir.setdefault(r['addr'], []).append(r)
quitar = set()
for r in rel:
    sig = por_dir.get(r['addr'] + 1)
    if sig and any(s['target'] == r['target'] and s['size'] == r['size'] for s in sig):
        quitar.add(id(r))
nuevo = [r for r in rel if id(r) not in quitar]
print('reubicaciones %d -> %d (quitadas %d)' % (len(rel), len(nuevo), len(rel) - len(nuevo)))
d['relocations'] = nuevo
with abre(sys.argv[2], 'w', encoding='utf-8', newline='\n') as fh:
    json.dump(d, fh, indent=2, ensure_ascii=False)
