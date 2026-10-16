#!/usr/bin/env python3
"""tipos360.py -- nombres de Xbox 360 por la CADENA del TypeInfo de los objetos de sonido (ronda 54).

Cada clase de sonido (SndBase: CARSFX_*, SFXObj_*, SFXCTL_*) define tres funciones que apuntan a su
`SndBase::TypeInfo` estatico ({id, const char *nombre}):
    GetTypeInfo   lis r11,T@ha; addi r3,r11,T@l; blr
    GetTypeName   lis r11,T@ha; addi r11,r11,T@l; lwz r3,4(r11); blr
    CreateObject  ... lis/addi de T y `lwz r5,4(rN)` (el nombre va a AudioMemoryManager::AllocateMemory)
El nombre de la clase esta ESCRITO en el ejecutable: la palabra T+4 apunta a la cadena ("SFXCTL_3DLeftWheelPos"). Es
la prueba mas fuerte que hay en 360 (no hay RTTI): no depende de ningun nombre heredado. En zEAXSound2,
`CreateObject@CARSFX_BottomOut` (0x8220D388) pasa la cadena "SFXCTL_3DLeftWheelPos" y llama a su constructor.

Regla: la funcion del original que se llama distinto del nombre que dice su cadena (con la decoracion que tiene en
NUESTRO objeto de la unidad: `?GetTypeInfo@C@@UBAPAUTypeInfo@SndBase@@XZ`, `?GetTypeName@C@@UBAPBDXZ`,
`?CreateObject@C@@SAPAVSndBase@@I@Z`, o en los estados `?GetStateInfo@C@@UBAPAUStateInfo@CSTATE_Base@@XZ`,
`?GetStateName@C@@UBAPBDXZ`, `?CreateState@C@@SAPAVCSTATE_Base@@I@Z`; si nuestra unidad no define ninguna, no se toca)
recibe ese nombre si: no casa hoy al 100 % con el suyo; el nombre nuevo no lo lleva otra funcion de la unidad que se quede con
el (si la otra tambien cambia en esta pasada, vale; si no, la otra se ANONIMIZA cuando su propia cadena dice otra clase
o no tiene cadena, y si no, la propuesta cae). Informa tambien (sin tocar) las funciones cuya cadena contradice el
nombre y que ya casan (el nombre falso sobre codigo igual da 100 %).

Entradas: symbols.txt/splits.txt, orig/<V>/NFS.exe y la medida oficial (build/versiones/<V>.json).

    python scripts/tipos360.py [--detalle] [--aplicar]
"""
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import ltcg360  # noqa: E402
import nombra_grafo as ng  # noqa: E402
from nombra_permuta360 import casadas, ANONIMO  # noqa: E402
from escritura import entero  # noqa: E402

V = 'EUROPEGERMILESTONE'
BASE = 0x82000000
FORMAS = {'GetTypeInfo': ['?GetTypeInfo@%s@@UBAPAUTypeInfo@SndBase@@XZ', '?GetStateInfo@%s@@UBAPAUStateInfo@CSTATE_Base@@XZ'],
          'GetTypeName': ['?GetTypeName@%s@@UBAPBDXZ', '?GetStateName@%s@@UBAPBDXZ'],
          'CreateObject': ['?CreateObject@%s@@SAPAVSndBase@@I@Z', '?CreateState@%s@@SAPAVCSTATE_Base@@I@Z']}
IDENT = re.compile(rb'^[A-Za-z_][A-Za-z0-9_]{2,80}$')


def main():
    pe = ltcg360.PE(os.path.join(RAIZ, 'orig', V, 'NFS.exe'))
    pct = casadas(os.path.join(RAIZ, 'build', 'versiones', V + '.json'))
    sims = os.path.join(RAIZ, 'config', V, 'symbols.txt')
    lineas = open(sims, encoding='utf-8').read().split('\n')
    rangos = []
    u = None
    for l in open(os.path.join(RAIZ, 'config', V, 'splits.txt'), encoding='utf-8'):
        m = re.match(r'^(\S.*)\.cpp:', l)
        if m:
            u = m.group(1).split('/')[-1]
            continue
        m = re.match(r'\s+\.text\s+start:0x(\w+) end:0x(\w+)', l)
        if m and u:
            rangos.append((int(m.group(1), 16), int(m.group(2), 16), u))

    def unidad_de(a):
        for a0, a1, un in rangos:
            if a0 <= a < a1:
                return un
        return None
    fun = []
    for i, l in enumerate(lineas):
        m = re.match(r'^(\S+) = \.text:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)', l)
        if m:
            fun.append((int(m.group(2), 16), int(m.group(3), 16), m.group(1), i))
    cadena_de = {}

    def cadena(t):
        if t in cadena_de:
            return cadena_de[t]
        c = None
        try:
            p = struct.unpack('>I', pe.lee(t + 4 - BASE, 4))[0]
            if 0x82000000 <= p < 0x83000000:
                s = pe.lee(p - BASE, 96).split(b'\0')[0]
                if IDENT.match(s):
                    c = s.decode()
        except Exception:
            c = None
        cadena_de[t] = c
        return c

    def tipo(a, tam):
        """(forma, T) si la funcion es una de las tres con un TypeInfo T, o None."""
        n = tam // 4
        if n < 3 or n > 80:
            return None
        w = struct.unpack('>%dI' % n, pe.lee(a - BASE, 4 * n))
        regs = {}   # registro -> direccion cargada con lis/addi
        for k, x in enumerate(w):
            op = x >> 26
            rd, ra, imm = (x >> 21) & 31, (x >> 16) & 31, x & 0xFFFF
            simm = imm - 0x10000 if imm & 0x8000 else imm
            if op == 15 and ra == 0:      # lis
                regs[rd] = ('hi', (simm << 16) & 0xFFFFFFFF)
            elif op == 14 and ra in regs and regs[ra][0] == 'hi':   # addi
                t = (regs[ra][1] + simm) & 0xFFFFFFFF
                if n == 3 and k == 1 and rd == 3 and w[2] == 0x4E800020 and cadena(t):
                    return ('GetTypeInfo', t)
                regs[rd] = ('dir', t)
            elif op == 32 and ra in regs and regs[ra][0] == 'dir' and simm == 4:   # lwz rX, 4(rA)
                t = regs[ra][1]
                if cadena(t):
                    if n == 4 and k == 2 and rd == 3 and w[3] == 0x4E800020:
                        return ('GetTypeName', t)
                    if rd == 5:
                        return ('CreateObject', t)
        return None

    obj_d = os.path.join(RAIZ, 'build', V, 'ltcg', 'obj', 'Speed', 'Indep', 'SourceLists')
    cache = {}

    def nuestros(un):
        if un not in cache:
            p = os.path.join(obj_d, un + '.obj')
            cache[un] = set(ng.Coff(p).funciones()) if os.path.exists(p) else set()
        return cache[un]
    propuestas = {}   # indice -> (unidad, dir, viejo, nuevo)
    contradice_100 = []
    por_unidad = collections.defaultdict(dict)   # unidad -> {nombre: [indices]}
    for a, tam, nombre, i in fun:
        un = unidad_de(a)
        if un:
            por_unidad[un].setdefault(nombre, []).append(i)
    vistos = collections.Counter()
    dice = {}   # indice -> nombre que dice su cadena
    for a, tam, nombre, i in fun:
        r = tipo(a, tam)
        if not r:
            continue
        forma, t = r
        clase = cadena(t)
        vistos[forma] += 1
        posibles = [f % clase for f in FORMAS[forma]]
        if nombre in posibles or nombre == '?GetStaticTypeInfo@%s@@SAPAUTypeInfo@SndBase@@XZ' % clase:   # la estatica es igual
            dice[i] = nombre
            continue
        un = unidad_de(a)
        if not un:
            continue
        nuestras = [x for x in posibles if x in nuestros(un)]   # la decoracion real: la que define nuestra unidad
        if len(nuestras) != 1:
            vistos['sin nombre nuestro'] += 1
            continue
        nuevo = dice[i] = nuestras[0]
        if pct.get(un, {}).get(nombre) == 100.0:
            contradice_100.append((un, a, nombre, nuevo))
            continue
        propuestas[i] = (un, a, nombre, nuevo)
    # el nombre nuevo no puede quedar repetido en la unidad
    anonimiza = {}
    cambio = True
    while cambio:
        cambio = False
        for i, (un, a, viejo, nuevo) in list(propuestas.items()):
            otros = [k for k in por_unidad[un].get(nuevo, []) if k != i and k not in propuestas]
            otros_bien = [k for k in otros if dice.get(k) == nuevo]
            if otros_bien or sum(1 for p in propuestas.values() if p[0] == un and p[3] == nuevo) > 1:
                del propuestas[i]
                cambio = True
                continue
            for k in otros:
                ak = int(re.match(r'^\S+ = \.text:0x([0-9A-Fa-f]+);', lineas[k]).group(1), 16)
                if pct.get(un, {}).get(nuevo) == 100.0:   # la otra ya casa con el nombre: no se toca
                    del propuestas[i]
                    cambio = True
                    break
                anonimiza[k] = (un, ak, nuevo, 'fn_%08X' % ak)
    print('funciones con TypeInfo: %s' % dict(vistos))
    for un, a, viejo, nuevo in sorted(contradice_100):
        print('YA CASA (no se toca) %-12s 0x%08X %s  (la cadena dice %s)' % (un, a, viejo[:60], nuevo[:60]))
    cambios = sorted(list(propuestas.items()) + list(anonimiza.items()), key=lambda x: x[1][1])
    for i, (un, a, viejo, nuevo) in cambios:
        print('%-12s 0x%08X %-55s -> %s' % (un, a, viejo[:55], nuevo[:80]))
    print('por TypeInfo: %d nombres (%d anonimizados)' % (len(cambios), len(anonimiza)))
    if '--aplicar' in sys.argv and cambios:
        for i, (un, a, viejo, nuevo) in cambios:
            assert lineas[i].startswith(viejo + ' = .text:'), lineas[i]
            lineas[i] = nuevo + lineas[i][len(viejo):]
        entero(sims, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
        print('aplicados %d' % len(cambios))
    return 0


if __name__ == '__main__':
    sys.exit(main())
