#!/usr/bin/env python3
"""tablas_pc.py ENTRADA.json SALIDA.json [--ver] -- mete en su funcion las tablas de saltos que la exportacion de IDA
dejo fuera en symbols.json de PC.

MSVC 7.1 pone las tablas de un `switch` detras del codigo de la funcion, en la misma COMDAT: primero la de destinos
(dwords, `jmp dword ptr [reg*4 + T]`) y despues, si el switch es de dos niveles, la de indices (bytes,
`mov/movzx r, byte ptr [reg + B]`). Nuestro objeto las lleva dentro de la funcion. La exportacion de IDA a veces
termina la funcion en el ultimo `ret` y deja las tablas fuera, sin reubicaciones en sus entradas: objdiff compara una
funcion mas corta que la nuestra y no casa aunque el codigo sea identico. Las 54 funciones de PC con tabla que casaban
la tenian dentro. Ronda 40 (PC40): 331 funciones, +18.219 B, 2.337 reubicaciones; +34 funciones al 100 % sin tocar
codigo y otras tantas casi exactas que dejan de esconderse detras de la tabla.

Para cada funcion con un acceso a tabla que cae entre su fin y el principio de la siguiente:
  - el fin pasa al final de la ultima tabla (comprobado contra el relleno int3 que precede a la siguiente funcion);
  - cada entrada de la tabla de destinos recibe su reubicacion OFF32 (destino = el valor de la entrada), si falta.
Las entradas de destinos tienen que apuntar dentro de la funcion; si algo no cuadra la funcion se deja como esta.

    python scripts/tablas_pc.py config/SPEED_EXE_1_3/symbols.json config/SPEED_EXE_1_3/symbols.json
"""
import json
import os
import struct
import sys

import capstone
from capstone import x86

from escritura import abre

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def secciones(exe):
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    nsec = struct.unpack_from('<H', exe, pe + 6)[0]
    optsz = struct.unpack_from('<H', exe, pe + 20)[0]
    base = struct.unpack_from('<I', exe, pe + 24 + 28)[0]
    out = []
    for i in range(nsec):
        o = pe + 24 + optsz + 40 * i
        va, rsz, rp = struct.unpack_from('<III', exe, o + 12)
        out.append((base + va, rsz, rp))
    return out


def main():
    entrada, salida = sys.argv[1], sys.argv[2]
    ver = '--ver' in sys.argv
    d = json.load(open(entrada, encoding='utf-8'))
    exe = open(os.path.join(RAIZ, 'orig', 'SPEED_EXE_1_3', 'speed.exe'), 'rb').read()
    secs = secciones(exe)

    def leer(a, n):
        for va, sz, rp in secs:
            if va <= a and a + n <= va + sz:
                return exe[rp + a - va:rp + a - va + n]
        return None

    funcs = d['functions']
    starts = [f['start'] for f in funcs]
    rels = d['relocations']
    rel_dirs = set(r['addr'] for r in rels)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    nuevas = []
    cambiadas = 0
    bytes_mas = 0
    rechazadas = []
    for k, f in enumerate(funcs):
        a, fin = f['start'], f['end']
        sig = starts[k + 1] if k + 1 < len(starts) else fin
        if sig <= fin or f.get('thunk'):
            continue
        codigo = leer(a, fin - a)
        if not codigo:
            continue
        tablas_d, tablas_b = set(), set()
        for ins in md.disasm(codigo, a):
            for op in ins.operands:
                if op.type != x86.X86_OP_MEM:
                    continue
                m = op.mem
                disp = m.disp & 0xffffffff
                if not (fin <= disp < sig):
                    continue
                if ins.mnemonic == 'jmp' and m.scale == 4 and m.index != 0 and m.base == 0:
                    tablas_d.add(disp)
                elif ins.mnemonic in ('mov', 'movzx') and op.size == 1 and (m.base != 0) != (m.index != 0) and m.scale == 1:
                    tablas_b.add(disp)
        if not tablas_d:
            continue
        # tras la ultima tabla solo puede haber el relleno int3 hasta el siguiente multiplo de 16
        cola = leer(fin, sig - fin)

        def es_relleno(p):
            q = min(sig, (p + 15) & ~15)
            return all(c == 0xCC for c in cola[p - fin:q - fin])
        entradas = []
        ok = True
        fin_d = fin
        for t in sorted(tablas_d):
            p = t
            n = 0
            while p + 4 <= sig and p not in tablas_b:
                v = struct.unpack_from('<I', cola, p - fin)[0]
                if not (a <= v < fin):
                    break
                entradas.append((p, v))
                p += 4
                n += 1
            if n == 0:
                ok = False
            fin_d = max(fin_d, p)
        nuevo_fin = fin_d
        ultima = max(tablas_d | tablas_b)
        if ultima in tablas_b:
            # la ultima es de indices: llega hasta el relleno (sus valores son indices pequenos, nunca 0xCC)
            p = ultima + 1
            while p < sig and not (cola[p - fin] == 0xCC and es_relleno(p)):
                p += 1
            nuevo_fin = max(fin_d, p)
        if not es_relleno(nuevo_fin):
            ok = False  # algo mas que tablas antes del relleno
        if not ok:
            rechazadas.append((f['name'], hex(a)))
            continue
        cambiadas += 1
        bytes_mas += nuevo_fin - fin
        f['end'] = nuevo_fin
        for p, v in entradas:
            if p not in rel_dirs:
                nuevas.append({'addr': p, 'type': 'OFF32', 'size': 4, 'target': v})
                rel_dirs.add(p)
    rels.extend(nuevas)
    rels.sort(key=lambda r: r['addr'])
    print('funciones con tablas metidas: %d (+%d B); reubicaciones nuevas: %d; rechazadas: %d'
          % (cambiadas, bytes_mas, len(nuevas), len(rechazadas)))
    for n, x in rechazadas[:40]:
        print('  rechazada', x, n[:100])
    if not ver:
        with abre(salida, 'w', encoding='utf-8', newline='\n') as fh:
            json.dump(d, fh, indent=2, ensure_ascii=False)


if __name__ == '__main__':
    main()
