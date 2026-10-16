#!/usr/bin/env python3
"""attrquita.py <clase> <atributo> [atributo...] -- quita atributos que NO EXISTEN en el prototipo A124 de una clase
generada (src/Speed/Indep/Src/Generated/AttribSys/Classes/<clase>.h): pone `#ifndef EA_BUILD_A124` alrededor de su
linea del _LayoutStruct (y de su `Private _Array_X`, si es array), de sus declaraciones y de sus definiciones inline.

Es el hermano de scripts/attrfuera.py. Aquel es para atributos que la A124 SI tiene pero lee por clave, fuera del
layout; este, para los que no tiene en absoluto. La prueba de cual es cual: buscar la clave del atributo en el asm de
la A124 (`grep -rli 0x<clave> build/SLES-53558-A124/asm`). Si aparece, attrfuera.py; si no aparece nunca, este.

Comprueba cada texto antes de tocarlo y no escribe nada si algo no cuadra. Despues hay que guardar tambien los usos
del atributo fuera de la cabecera (el compilador los senala en la A124).

    python scripts/attrquita.py explosion BaseMaterial"""
import re
import sys

if len(sys.argv) < 3 or sys.argv[1] in ('-h', '--help'):
    print(__doc__)
    sys.exit(0)
clase, attrs = sys.argv[1], sys.argv[2:]
ruta = 'src/Speed/Indep/Src/Generated/AttribSys/Classes/%s.h' % clase
d = open(ruta, 'rb').read().decode('utf-8')
nl = '\r\n' if '\r\n' in d else '\n'
lineas = d.split(nl)
GUARDA, FIN = '#ifndef EA_BUILD_A124', '#endif'

rangos = []  # (primera, ultima) inclusive, en indices de linea

# 1. el layout
ini = next(i for i, l in enumerate(lineas) if l.strip() == 'struct _LayoutStruct {')
fin = next(i for i in range(ini, len(lineas)) if lineas[i].strip() == '};')
for a in attrs:
    en_layout = [i for i in range(ini, fin) if re.search(r'\b%s(\[\d+\])?;' % re.escape(a), lineas[i])]
    if len(en_layout) != 1:
        sys.exit('%s: %d lineas de layout (espero 1)' % (a, len(en_layout)))
    i = en_layout[0]
    primera = i - 1 if re.search(r'\b_Array_%s;' % re.escape(a), lineas[i - 1]) else i
    rangos.append((primera, i))

    # 2. las declaraciones: bloque contiguo de lineas `... a(...)...;` y `... SET_a(...);`
    decl = [k for k, l in enumerate(lineas)
            if re.match(r'\s+[\w:<>&, ]+\b(SET_)?%s\(.*\)( const)?;\s*$' % re.escape(a), l)
            or re.match(r'\s+[\w:<>&, ]+\bNum_%s\(\) const;\s*$' % re.escape(a), l)]
    if not decl or decl[-1] - decl[0] != len(decl) - 1:
        sys.exit('%s: declaraciones no contiguas: %s' % (a, decl))
    rangos.append((decl[0], decl[-1]))

    # 3. las definiciones: desde la primera `inline ... Gen::clase::a(` hasta la llave que cierra la ultima
    defs = [k for k, l in enumerate(lineas)
            if re.match(r'inline .*\bGen::%s::(SET_|Num_)?%s\(' % (re.escape(clase), re.escape(a)), l)]
    if not defs:
        sys.exit('%s: sin definiciones inline' % a)
    k = defs[-1]
    while lineas[k].strip() != '}':
        k += 1
    # entre la primera y la ultima no puede haber definiciones de otro atributo
    for j in range(defs[0], k + 1):
        m = re.match(r'inline .*\bGen::%s::(?:SET_|Num_)?(\w+)\(' % re.escape(clase), lineas[j])
        if m and m.group(1) != a:
            sys.exit('%s: definicion ajena en medio (%s)' % (a, lineas[j].strip()))
    rangos.append((defs[0], k))

# aplicar de abajo arriba para no mover indices
for primera, ultima in sorted(rangos, reverse=True):
    sangria = ''
    lineas[ultima + 1:ultima + 1] = [FIN]
    lineas[primera:primera] = [GUARDA]
open(ruta, 'wb').write(nl.join(lineas).encode('utf-8'))
print('%s: %d atributos quitados de la A124 (%d bloques guardados)' % (clase, len(attrs), len(rangos)))
