#!/usr/bin/env python3
"""restos_epilogo.py [VERSION] [--aplicar] -- simbolos CON NOMBRE de config/<V>/symbols.txt que empiezan antes de su
funcion: en la ranura de retardo del `jr $ra` de la funcion anterior, o sobre restos de epilogo sueltos
(`addiu $sp, $sp, +N` y `nop`, 8 bytes cada uno), o sobre pares sueltos (instruccion; `nop`) que siguen a un
`endlabel` + `nop` y llegan hasta el prologo (`addiu $sp, $sp, -N`): restos de la cola de la funcion anterior
(`sw v0, 0x58(v1); nop` antes de `_$_16ShoppingCartItem` en SLUS, ronda 55). EE-GCC no emite `nop` entre dos
instrucciones de una entrada (el R5900 no tiene ranura de carga), asi que un par (x; nop) antes del prologo no es
codigo de la funcion.

La prueba es de control de flujo: si la instruccion justo ANTES del simbolo es `jr $ra`, la primera del simbolo es la
ranura de retardo de otra funcion (`addiu v0, v0, 0`, `daddu v0, zero, zero`, `addiu sp, sp, +N`...), no una entrada.
Y ninguna funcion empieza desapilando. La entrada real es la primera instruccion despues de la ranura, de los `nop` de
relleno y de los restos. En SLUS lo probo GetLocaleString, cuya direccion guarda el constructor de MemoryCard.
Pasa en SLUS porque sus nombres se heredaron por direccion; en la A124 las direcciones salen exactas del .mdebug.

Solo mueve un simbolo si ningun otro simbolo ocupa ya la direccion nueva (bMemoryFindAllocation en SLUS ERA el resto
entero, mal nombrado). Los `pad_` se saltan: SON el relleno (scripts/relleno_ps2.py), y en la ventana 15 los 36 de SLUS
que proponia mover lo eran. Sin --aplicar solo informa. Despues hay que medir la version (el troceado depende de
symbols.txt) y comprobar que no se pierde nada.

    python scripts/restos_epilogo.py SLUS-21351
    python scripts/restos_epilogo.py SLUS-21351 --aplicar
"""
import glob
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
args = [a for a in sys.argv[1:] if not a.startswith('--')]
V = args[0] if args else 'SLUS-21351'
APLICAR = '--aplicar' in sys.argv
RE_INS = re.compile(r'^\s*/\*\s*([0-9A-F]+)\s+([0-9A-F]+)\s+[0-9A-F]+\s*\*/\s+(\S+)\s*(.*)$')


def es_resto_sp(ins):
    return ins[1] == 'addiu' and ins[2].startswith('$sp,$sp,0x')


def es_prologo(ins):
    return ins[1] == 'addiu' and ins[2].startswith('$sp,$sp,-')


SALTOS = ('jr', 'j', 'jal', 'jalr', 'b', 'bal', 'beq', 'bne', 'beqz', 'bnez', 'beql', 'bnel', 'blez', 'bgtz',
          'bltz', 'bgez', 'bc1t', 'bc1f', 'eret')


def pares_hasta_prologo(cuerpo):
    """Bytes de pares (x; nop) desde la entrada hasta el prologo, o 0. `cuerpo` solo llega al endlabel."""
    if not cuerpo or es_prologo(cuerpo[0]):
        return 0
    k = 0
    while k + 1 < len(cuerpo) and not es_prologo(cuerpo[k]):
        if cuerpo[k + 1][1] != 'nop' or cuerpo[k][1] in SALTOS:
            return 0
        k += 2
    if k and k < len(cuerpo) and es_prologo(cuerpo[k]) and cuerpo[k][0] == cuerpo[0][0] + 4 * k:
        return 4 * k
    return 0


candidatos = {}
for ruta in glob.glob(os.path.join(RAIZ, 'build', V, 'asm', '**', '*.s'), recursive=True):
    lineas = open(ruta, encoding='utf-8', errors='replace').read().split('\n')
    anterior = None  # ultima instruccion vista (de la funcion anterior)
    for i, l in enumerate(lineas):
        mi = RE_INS.match(l)
        if mi:
            anterior = (int(mi.group(2), 16), mi.group(3), mi.group(4).replace(' ', ''))
            continue
        m = re.match(r'\s*glabel (\S+)', l)
        if not m:
            continue
        nombre = m.group(1)
        ins = []
        cuerpo = []  # las instrucciones del simbolo, hasta su endlabel (para los pares)
        for k in range(i + 1, min(i + 120, len(lineas))):
            if lineas[k].startswith(('endlabel', 'glabel')):
                break
            mk = RE_INS.match(lineas[k])
            if mk:
                cuerpo.append((int(mk.group(2), 16), mk.group(3), mk.group(4).replace(' ', '')))
        for k in range(i + 1, min(i + 40, len(lineas))):
            mk = RE_INS.match(lineas[k])
            if mk:
                ins.append((int(mk.group(2), 16), mk.group(3), mk.group(4).replace(' ', '')))
            if len(ins) == 14:
                break
        if nombre.startswith(('func_', 'D_', 'pad_')) or len(ins) < 3:
            continue
        base = ins[0][0]
        j = 0
        en_ranura = anterior is not None and anterior[1] == 'jr' and anterior[2] == '$ra' and anterior[0] == base - 4
        if en_ranura:
            j = 1  # la ranura de retardo de la funcion anterior
        # restos de epilogo (addiu sp,sp,+N / nop) y relleno
        while j < len(ins):
            if ins[j][1] == 'nop':
                j += 1
            elif es_resto_sp(ins[j]) and j + 1 < len(ins) and ins[j + 1][1] == 'nop':
                j += 2
            else:
                break
        if j and j < len(ins) and (en_ranura or es_resto_sp(ins[0])) and ins[j][0] == base + 4 * j:
            candidatos[nombre] = (base, 4 * j)
        elif anterior is not None and anterior[1] == 'nop' and anterior[0] == base - 4:
            salto = pares_hasta_prologo(cuerpo)
            if salto:
                candidatos[nombre] = (base, salto)

P = os.path.join(RAIZ, 'config', V, 'symbols.txt')
s = open(P, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'
lineas = s.split(nl)
por_dir = {}
indice = {}
for i, l in enumerate(lineas):
    m = re.match(r'(\S+) = 0x([0-9A-Fa-f]+);', l)
    if m:
        por_dir.setdefault(int(m.group(2), 16), []).append(m.group(1))
        indice.setdefault(m.group(1), []).append(i)

movidos = 0
for nombre, (dire, salto) in sorted(candidatos.items(), key=lambda x: x[1][0]):
    if len(indice.get(nombre, [])) != 1:
        print('salto %s: %d lineas en symbols.txt' % (nombre, len(indice.get(nombre, []))))
        continue
    if por_dir.get(dire + salto):
        print('salto %s: 0x%x ya es %s' % (nombre, dire + salto, ', '.join(por_dir[dire + salto])))
        continue
    i = indice[nombre][0]
    m = re.match(r'(\S+) = 0x([0-9A-Fa-f]+);(.*)$', lineas[i])
    if not m or int(m.group(2), 16) != dire:
        print('salto %s: symbols.txt no dice 0x%x' % (nombre, dire))
        continue
    lineas[i] = '%s = 0x%x;%s' % (nombre, dire + salto, m.group(3))
    movidos += 1
print('%s: %d simbolos con nombre empiezan antes de su funcion; %d se mueven' % (V, len(candidatos), movidos))
if APLICAR and movidos:
    # escritura segura: si el disco se llena a medias, symbols.txt queda como estaba (leccion de la ronda 55)
    tmp = P + '.tmp'
    with open(tmp, 'w', encoding='utf-8', newline='') as f:
        f.write(nl.join(lineas))
    os.replace(tmp, P)
    print('escrito', os.path.relpath(P, RAIZ))
