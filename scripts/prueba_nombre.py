#!/usr/bin/env python3
"""Prueba EXACTA de uno o varios nombres para una funcion del original de PS2: compara la funcion del troceado
(build/<V>/obj, que lleva las reubicaciones de splat) que empieza en la direccion dada con nuestras funciones de esos
nombres en el objeto de la misma unidad (build/<V>/src), palabra a palabra.

En las palabras reubicadas se compara el codigo de operacion Y los registros (rs, rt) en las de tipo I, y solo el
codigo en j/jal; si los dos destinos de un jal tienen nombre, tienen que ser el mismo. Enmascarar solo el codigo de
operacion (como tools/nombra_ps2.py) esconde diferencias reales: bRandom__Fi y bRandom__Ff de SLUS solo se distinguen
en el registro de la semilla (`lui $a1` / `lui $a0` con %hi), y con esa mascara salian las dos "iguales".

    python scripts/prueba_nombre.py V 0xDIRECCION NOMBRE [NOMBRE...]
    python scripts/prueba_nombre.py SLUS-21351 0x15c908 bRandom__Fi bRandom__Ff

Imprime, por nombre: IGUAL, el numero de palabras distintas (y cuales, si son pocas) o la diferencia de tamano, y los
destinos de las reubicaciones de las dos. IGUAL no prueba el nombre si otra funcion nuestra tambien es IGUAL (codigo
identico en dos clases): entonces hace falta otra ancla (vtable, cadena, grafo de llamadas).
"""
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
import nombra_ps2  # noqa: E402
from nombra_symtab import ficheros_asm  # noqa: E402

RE_INS = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s')


def funcion_en(V, x):
    """(nombre del glabel, unidad) del troceado que empieza en x"""
    marca = '%08X' % x
    for p, unidad in ficheros_asm(V):
        txt = open(p, encoding='utf-8', errors='replace').read()
        if marca not in txt:
            continue
        lineas = txt.split('\n')
        for i, l in enumerate(lineas):
            m = RE_INS.match(l)
            if not m or m.group(1) != marca:
                continue
            for atras in lineas[i - 1::-1]:
                g = re.match(r'\s*glabel (\S+)', atras)
                if g:
                    return g.group(1), unidad
                if RE_INS.match(atras):
                    return None, None            # la direccion no es la primera instruccion de su funcion
    return None, None


def funciones(ruta):
    """{nombre: [(bytes, [(desplazamiento, destino)])]} de las funciones de un objeto. En nuestros objetos, un j/jal
    a una funcion de la misma seccion se reubica contra el SIMBOLO DE SECCION (destino ''), con el desplazamiento
    dentro de la palabra: se resuelve aqui al nombre de la funcion que empieza ahi, para poder comparar destinos
    (FEngSetVisible__FPCcUi y FEngSetInvisible__FPCcUi solo se distinguen en el destino del `j` final)."""
    elf = nombra_ps2.Elf(ruta)
    en_seccion = {}
    for nombre, valor, sec, tipo, tam in elf.simbolos:
        if tipo == 2 and nombre:
            en_seccion.setdefault((sec, valor), nombre)
    out = {}
    for nombre, b, rel, (sec, inicio) in elf.funciones():
        resuelto = []
        for r, d in rel:
            if not d and r + 4 <= len(b):
                w, = struct.unpack_from('<I', b, r)
                if (w >> 26) in (2, 3):
                    d = en_seccion.get((sec, (w & 0x3FFFFFF) << 2), '')
            resuelto.append((r, d))
        out.setdefault(nombre, []).append((b, resuelto))
    return out


def compara(t, o):
    (tb, trel), (ob, orel) = t, o
    if len(tb) != len(ob):
        return 'tamano %d/%d' % (len(ob), len(tb)), []
    n = len(tb) // 4
    wt, wo = struct.unpack('<%dI' % n, tb[:n * 4]), struct.unpack('<%dI' % n, ob[:n * 4])
    rt, ro = {r // 4: d for r, d in trel}, {r // 4: d for r, d in orel}
    dif = []
    for k in range(n):
        if k in rt or k in ro:
            op = wt[k] >> 26
            m = 0xFC000000 if op in (2, 3) else 0xFFFF0000
            if (wt[k] & m) != (wo[k] & m):
                dif.append(k)
            elif (op in (2, 3) and k in rt and k in ro and rt[k] != ro[k] and ro[k] and not ro[k].startswith('.')
                  and not re.match(r'(func|D)_[0-9A-F]{8}$', rt[k])):
                dif.append(k)
        elif wt[k] != wo[k]:
            dif.append(k)
    return ('IGUAL' if not dif else '%d palabras distintas' % len(dif)), [(k, wt[k], wo[k]) for k in dif]


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    V, x, nombres = sys.argv[1], int(sys.argv[2], 16), sys.argv[3:]
    glabel, unidad = funcion_en(V, x)
    if not glabel:
        sys.exit('0x%x no es el principio de una funcion del troceado de %s' % (x, V))
    base = os.path.splitext(unidad)[0] if unidad.endswith(('.cpp', '.c')) else unidad
    t = funciones(os.path.join(RAIZ, 'build', V, 'obj', base + '.o')).get(glabel)
    if not t:
        sys.exit('%s no esta en el objeto troceado (hay que volver a ensamblar: scripts/medir.py)' % glabel)
    nuestras = funciones(os.path.join(RAIZ, 'build', V, 'src', base + '.o'))
    print('%s 0x%x %s (%d B) [%s]' % (V, x, glabel, len(t[0][0]), unidad))
    print('   destinos del original: %s' % ', '.join(sorted({d for r, d in t[0][1]})))
    for n in nombres:
        for b, rel in nuestras.get(n, []):
            res, dif = compara(t[0], (b, rel))
            print('  %-70s %s' % (n[:70], res))
            print('      destinos nuestros: %s' % ', '.join(sorted({d or '(seccion)' for r, d in rel})))
            for k, a, c in dif[:5]:
                print('      palabra %d: original %08x nuestra %08x' % (k, a, c))
        if n not in nuestras:
            print('  %-70s no esta en nuestro objeto' % n[:70])


if __name__ == '__main__':
    main()
