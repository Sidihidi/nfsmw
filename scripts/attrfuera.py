#!/usr/bin/env python3
"""attrfuera.py <clase> <atributo> [atributo...] -- saca atributos del _LayoutStruct de una clase generada
(src/Speed/Indep/Src/Generated/AttribSys/Classes/<clase>.h) SOLO en la A124, y hace que sus accesores lean por
CLAVE, como hace el original.

En la A124 algunos atributos no estan en el layout de su clase: los stabs de pursuitlevels empiezan en
heliLOSdistance (0x0) sin el array `cops`, y GetAdjustedCopCounts lee `cops(i)` como
GetAttributePointer(0xA9811B93, i) y `Num_cops()` como Get(0xA9811B93).GetLength(). No es de todos los arrays (en
engine, tires, transmission... siguen dentro): es por atributo, y lo dice `sondastabs.py`/`cotejo.py`.

Para cada atributo:
  - en _LayoutStruct, `#ifndef EA_BUILD_A124` alrededor de su linea (y de su `Private _Array_X`, si es array);
  - cada accesor que usa la macro de layout pasa, en la A124, a la macro por clave con la clave y el tipo de su
    propio ATTRIB_CODEGEN_GETATTRIB(TIPO, CLAVE):
        GETLAYOUTINDEXED -> GETVALUEINDEXED        CHECKEDGETLAYOUTINDEXED -> CHECKEDGETVALUEINDEXED
        GETLAYOUTLENGTH  -> GETLENGTH              SETLAYOUTINDEXED        -> SETVALUEINDEXED
        GETLAYOUT        -> GETVALUE               CHECKEDGETLAYOUT        -> CHECKEDGETVALUE
        SETLAYOUT        -> SETVALUE
Los accesores pueden estar fuera de la clase (`inline bool Gen::clase::X(...)`, pursuitlevels) o dentro
(`bool X(...)`, camerainfo); se respeta la sangria de cada uno. Comprueba cada texto antes de tocarlo y no escribe
nada si algo no cuadra. Despues, `sondastabs.py --clase`.

    python scripts/attrfuera.py camerainfo STIFFNESS LATOFFSET LAG FOV HEIGHT ANGLE TILTING SELECTABLE"""
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

# 1. el layout
ini = next(i for i, l in enumerate(lineas) if l.strip() == 'struct _LayoutStruct {')
fin = next(i for i in range(ini, len(lineas)) if lineas[i].strip() == '};')
guardar = set()
for a in attrs:
    hit = [i for i in range(ini, fin) if re.match(r'\s+\S.*\b%s\s*(\[\d+\])?\s*;' % re.escape(a), lineas[i])
           and '_Array_' not in lineas[i]]
    if len(hit) != 1:
        sys.exit('FALLO: %s aparece %d veces en el _LayoutStruct de %s' % (a, len(hit), clase))
    guardar.add(hit[0])
    guardar.update(i for i in range(ini, fin) if re.match(r'\s+Private _Array_%s\s*;' % re.escape(a), lineas[i]))
nuevas = []
for i, l in enumerate(lineas):
    if i in guardar and (i - 1) not in guardar:
        nuevas.append('#ifndef EA_BUILD_A124')
    nuevas.append(l)
    if i in guardar and (i + 1) not in guardar:
        nuevas.append('#endif')
d = nl.join(nuevas)

# 2. los accesores
CAMBIO = {
    'GETLAYOUTINDEXED': lambda t, k, a: 'ATTRIB_CODEGEN_GETVALUEINDEXED(%s, %s, %s)' % (t, k, a[2]),
    'CHECKEDGETLAYOUTINDEXED': lambda t, k, a: 'ATTRIB_CODEGEN_CHECKEDGETVALUEINDEXED(%s, %s, %s, %s)' % (t, k, a[1],
                                                                                                        a[2]),
    'GETLAYOUTLENGTH': lambda t, k, a: 'ATTRIB_CODEGEN_GETLENGTH(%s)' % k,
    'SETLAYOUTINDEXED': lambda t, k, a: 'ATTRIB_CODEGEN_SETVALUEINDEXED(%s, %s, %s, %s)' % (t, k, a[1], a[2]),
    'GETLAYOUT': lambda t, k, a: 'ATTRIB_CODEGEN_GETVALUE(%s, %s)' % (t, k),
    'CHECKEDGETLAYOUT': lambda t, k, a: 'ATTRIB_CODEGEN_CHECKEDGETVALUE(%s, %s, %s)' % (t, k, a[1]),
    'SETLAYOUT': lambda t, k, a: 'ATTRIB_CODEGEN_SETVALUE(%s, %s, %s)' % (t, k, a[1]),
}
MACRO = re.compile(r'^([ \t]+)ATTRIB_CODEGEN_(GETLAYOUTINDEXED|CHECKEDGETLAYOUTINDEXED|GETLAYOUTLENGTH|'
                   r'SETLAYOUTINDEXED|GETLAYOUT|CHECKEDGETLAYOUT|SETLAYOUT)\(([^;]*)\);', re.M)
total = 0
for a in attrs:
    m = re.search(r'(?:inline bool Gen::%s::|\bbool )%s\(TAttrib<([^>]+)> &result\) const \{\s*'
                  r'ATTRIB_CODEGEN_GETATTRIB\(([^,]+), (0x[0-9a-fA-F]+)\);' % (re.escape(clase), re.escape(a)), d)
    if not m:
        sys.exit('FALLO: no encuentro el GETATTRIB de %s::%s (clave y tipo)' % (clase, a))
    tipo, clave = m.group(2).strip(), m.group(3)
    cuenta = [0]

    def cambia(mm, a=a, tipo=tipo, clave=clave):
        s, macro, args = mm.group(1), mm.group(2), [x.strip() for x in mm.group(3).split(',')]
        # el campo es el segundo argumento en GETLAYOUTINDEXED y el primero en las demas
        campo = args[1] if macro == 'GETLAYOUTINDEXED' else args[0]
        if campo != a:
            return mm.group(0)
        cuenta[0] += 1
        return ('#ifdef EA_BUILD_A124' + nl + s + CAMBIO[macro](tipo, clave, args) + ';' + nl + '#else' + nl
                + mm.group(0) + nl + '#endif')
    d = MACRO.sub(cambia, d)
    if not cuenta[0]:
        sys.exit('FALLO: ningun accesor de layout para %s::%s' % (clase, a))
    print('  %-24s clave %s  tipo %-28s %d accesores' % (a, clave, tipo, cuenta[0]))
    total += cuenta[0]
open(ruta, 'wb').write(d.encode('utf-8'))
print('ok %s: %d atributos fuera del layout en la A124, %d accesores' % (ruta, len(attrs), total))
