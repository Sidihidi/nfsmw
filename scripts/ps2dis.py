#!/usr/bin/env python3
"""ps2dis.py <simbolo|0xdireccion> [...] -- desensambla una funcion de la A124 por RANGO (direccion y tamano del
NFS.MAP), no por los limites que dan los simbolos del ELF.

Hace falta porque el ELF de la A124 tiene etiquetas internas (`$L...`) y funciones sin nombre: objdump corta donde
no debe y una funcion sale partida o pegada a la siguiente. El mapa trae el tamano exacto.

    python scripts/ps2dis.py eMulMatrix__FP8bMatrix4T0T0
    python scripts/ps2dis.py 0x218728
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__))).replace('\\', '/') + '/'
ELF = ROOT + 'orig/SLES-53558-A124/NFS.ELF'
MAP = ROOT + 'orig/SLES-53558-A124/NFS.MAP'
OD = ROOT + 'build/mips_binutils/mips-linux-gnu-objdump.exe'


def mapa():
    """simbolo -> (direccion, tamano) y direccion -> (simbolo, tamano)."""
    por_nombre, por_dir = {}, {}
    for l in open(MAP, errors='replace'):
        m = re.match(r'^([0-9a-fA-F]{8}) ([0-9a-fA-F]{8})\s+\d+\s+(\S+)\s*$', l)
        if m:
            a, t, n = int(m.group(1), 16), int(m.group(2), 16), m.group(3)
            if t and not n.endswith(('.obj', '.lib')) and '\\' not in n:
                por_nombre[n] = (a, t)
                por_dir[a] = (n, t)
    return por_nombre, por_dir


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    por_nombre, por_dir = mapa()
    for pedido in sys.argv[1:]:
        if pedido.startswith('0x'):
            a = int(pedido, 16)
            n, t = por_dir.get(a, (pedido, 0x100))
        elif pedido in por_nombre:
            n = pedido
            a, t = por_nombre[pedido]
        else:
            cands = [k for k in por_nombre if pedido in k]
            if not cands:
                print('== %s: no esta en el mapa' % pedido)
                continue
            n = sorted(cands)[0]
            a, t = por_nombre[n]
        r = subprocess.run([OD, '-d', '--no-show-raw-insn', '--start-address=0x%x' % a,
                            '--stop-address=0x%x' % (a + t), ELF], capture_output=True, text=True)
        print('== %s  0x%x  %d B' % (n, a, t))
        for l in r.stdout.split('\n'):
            if re.match(r'^\s*[0-9a-f]+:\s', l):
                dirn, ins = l.split(':', 1)
                print('   %s  %s' % (dirn.strip(), re.sub(r'\s+', ' ', ins.strip())))


if __name__ == '__main__':
    main()
