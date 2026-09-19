#!/usr/bin/env python3
"""argsusados.py <version> <unidad> <simbolo> [...] -- que registros de argumento lee una funcion de PS2 antes de
escribirlos: $a0-$a3 y $t0-$t3 (enteros, EABI de la EE) y $f12-$f19 (flotantes). Recorrido lineal hasta la primera
llamada (con su hueco de retardo): aproximado, pero basta para ver si una build usa un parametro que otra no tiene.

Para que sirve: los nombres de config/SLUS-21351/symbols.txt vienen heredados de la A124, asi que la firma mangled de
SLUS no prueba nada. Si SLUS lee un registro que la A124 no lee (AddListener de SLUS lee $a2, el `who` que la A124
no tiene), SLUS tiene ese parametro y el nombre esta mal. Un parametro que no se lee no deja rastro: sin diferencia
de registros no se puede decidir.

    python scripts/argsusados.py SLUS-21351 zSim AddListener__Q23Sim9CollisionPQ33Sim9Collision9IListenerP10HSIMABLE__
"""
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__))).replace('\\', '/') + '/'
ARGS = ['$a0', '$a1', '$a2', '$a3', '$t0', '$t1', '$t2', '$t3'] + ['$f%d' % i for i in range(12, 20)]
STORES = {'sb', 'sh', 'sw', 'sd', 'sq', 'swc1', 'sdc1', 'swl', 'swr', 'sdl', 'sdr', 'sqc2'}
READ_ALL = {'beq', 'bne', 'beql', 'bnel', 'blez', 'bgtz', 'bltz', 'bgez', 'blezl', 'bgtzl', 'bltzl', 'bgezl', 'jr',
            'mult', 'multu', 'div', 'divu', 'mult1', 'multu1', 'div1', 'divu1', 'teq', 'tne', 'mtlo', 'mthi',
            'beqz', 'bnez', 'beqzl', 'bnezl', 'mtsa'}
REG = re.compile(r'\$(?:[a-z]+\d*|\d+)')


def cuerpo(version, unidad, sym):
    for p in glob.glob(ROOT + 'build/%s/asm/**/%s.cpp.s' % (version, unidad), recursive=True):
        t = open(p, errors='replace').read()
        i = t.find('glabel %s\n' % sym)
        if i < 0:
            continue
        j = t.find('endlabel %s\n' % sym, i)
        return t[i:j].splitlines()[1:]
    return None


def usados(lineas):
    visto, leidos, corta = set(), [], False
    for l in lineas:
        l = re.sub(r'/\*.*?\*/', '', l).strip()
        if not l or l.startswith(('.', 'glabel', 'endlabel', 'nonmatching')) or l.endswith(':'):
            continue
        partes = l.split(None, 1)
        mn = partes[0]
        ops = [o.strip() for o in partes[1].split(',')] if len(partes) > 1 else []
        regs = [REG.findall(o) for o in ops]
        if mn in STORES or mn in READ_ALL or mn.startswith(('c.', 'bc1')):
            lee, escribe = sum(regs, []), []
        elif mn == 'mtc1':
            lee, escribe = regs[0], regs[1] if len(regs) > 1 else []
        elif mn == 'jalr':
            lee, escribe = sum(regs, []), ['$ra']
        elif mn in ('jal', 'j', 'b'):
            corta = True  # la instruccion siguiente (hueco de retardo) aun cuenta
            continue
        else:
            escribe, lee = (regs[0] if regs else []), sum(regs[1:], [])
        for r in lee:
            if r in ARGS and r not in visto:
                visto.add(r)
                leidos.append(r)
        for r in escribe:
            visto.add(r)
        if corta:
            break
        if mn == 'jalr':
            corta = True
    return leidos


def main():
    version, unidad = sys.argv[1], sys.argv[2]
    for sym in sys.argv[3:]:
        c = cuerpo(version, unidad, sym)
        if c is None:
            print('%-70s NO ESTA' % sym)
            continue
        print('%-70s %4d lineas  lee antes de escribir: %s' % (sym[:70], len(c), ' '.join(usados(c)) or '-'))


if __name__ == '__main__':
    main()
