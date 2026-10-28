#!/usr/bin/env python3
"""ppcsk.py EXE_A VA_A EXE_B VA_B [TAM_B] [--tam-a N] [--base-a VA] [--base-b VA] -- esqueleto de funciones
PowerPC de Xbox 360 SIN reubicaciones (exe contra exe): dice si dos funciones de dos ejecutables enlazados por separado
son el mismo codigo salvo direcciones.

Dos ejecutables enlazados por separado no comparten direcciones; lo que si comparten (si el codigo es el mismo) es todo
lo demas. Normalizacion de cada palabra:
  * b/bl absolutos-relativos (opcode 18) que salen de la funcion: campo LI a cero; el destino se guarda como LLAMADA;
    los saltos dentro de la funcion se quedan (son relativos y la forma de la funcion los fija);
  * lis rD,X (addis rD,0,X): inmediato a cero, rD queda marcado como base de direccion;
  * forma D (addi, lwz, stw, lfs, lfd, lbz, lhz, sth, stb, stfs, stfd, ...) y forma DS (ld/std) cuya base es un
    registro marcado por un lis previo de la misma funcion: desplazamiento a cero (es la mitad baja de la direccion);
  * el resto, tal cual (desplazamientos de miembro desde r3/r31..., constantes, mascaras de rlwinm).
Se usa en los dos lados igual, asi que el exceso de mascara es simetrico. Como modulo: `Exe(ruta, base_ejec)` lee un
PE de Xbox 360 en claro (el basefile del xex, docs/X360_EXTRACTION.md, o nuestro NFS_ltcg.exe con base 0x400000) y
`esqueleto(exe, va, tam)` da (bytes, llamadas, referencias lis/lo). Escrito para cruzar MW con el prototipo de
ProStreet (ronda 76, docs/analisis/2026-10-21_prostreet.md).

    python scripts/ppcsk.py orig/EUROPEGERMILESTONE/NFS.exe 0x821651F0 OTRO.exe 0x82190000 --tam-a 28
    python scripts/ppcsk.py orig/EUROPEGERMILESTONE/NFS.exe 0x821651F0 build/EUROPEGERMILESTONE/ltcg/NFS_ltcg.exe 0x46EDC0 --tam-a 28 --base-b 0x400000
"""
import struct

DFORM = {14, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55}
DSFORM = {58, 62}


class Exe:
    """Un PE de Xbox 360 en claro (basefile del xex). Direcciones de ejecucion (0x82xxxxxx)."""

    def __init__(self, ruta, base_ejec=0x82000000):
        d = open(ruta, 'rb').read()
        self.d = d
        pe, = struct.unpack_from('<I', d, 0x3C)
        nsec, = struct.unpack_from('<H', d, pe + 6)
        optsz, = struct.unpack_from('<H', d, pe + 20)
        self.imagebase, = struct.unpack_from('<I', d, pe + 24 + 28)
        self.secs = []
        so = pe + 24 + optsz
        for i in range(nsec):
            nm, vs, va, rs, rp = struct.unpack_from('<8sIIII', d, so + 40 * i)
            self.secs.append((nm.rstrip(b'\0').decode('latin-1'), va, vs, rp, rs))
        self.base = base_ejec

    def lee(self, va, n):
        rva = va - self.base
        for nm, sva, vs, rp, rs in self.secs:
            if sva <= rva < sva + max(vs, rs):
                o = rp + rva - sva
                return self.d[o:o + n]
        return b''

    def seccion(self, nombre):
        for nm, sva, vs, rp, rs in self.secs:
            if nm == nombre:
                return self.base + sva, vs
        return None

    def u32(self, va):
        b = self.lee(va, 4)
        return struct.unpack('>I', b)[0] if len(b) == 4 else None

    def f32(self, va):
        b = self.lee(va, 4)
        return struct.unpack('>f', b)[0] if len(b) == 4 else None


def esqueleto(exe, va, size, valores=False):
    """(bytes del esqueleto, [destinos de llamada], [direcciones referenciadas lis/lo])."""
    raw = exe.lee(va, size)
    n = len(raw) // 4
    ws = struct.unpack('>%dI' % n, raw[:4 * n])
    out = []
    calls = []
    marcados = set()
    hi = {}
    refs = []
    for i, w in enumerate(ws):
        op = w >> 26
        if op == 18:
            li = w & 0x03FFFFFC
            if li & 0x02000000:
                li -= 0x04000000
            aa = (w >> 1) & 1
            dest = li if aa else va + 4 * i + li
            if not (va <= dest < va + size):
                calls.append(dest)
                w &= 0xFC000003
        elif op == 15:
            rd, ra = (w >> 21) & 31, (w >> 16) & 31
            if ra == 0:
                marcados.add(rd)
                hi[rd] = w & 0xFFFF
                w &= 0xFFFF0000
        elif op in DFORM or op in DSFORM:
            ra = (w >> 16) & 31
            if ra in marcados and ra != 1:
                lo = w & 0xFFFF
                if op in DSFORM:
                    lo &= 0xFFFC
                if ra in hi:
                    a = ((hi[ra] << 16) + (lo - 0x10000 if lo & 0x8000 else lo)) & 0xFFFFFFFF
                    refs.append((op, a))
                w &= 0xFFFF0000 | (3 if op in DSFORM else 0)
                if op == 14:
                    marcados.discard((w >> 21) & 31)  # addi: rD ya es la direccion entera (lo que siga son miembros)
        out.append(w)
    return struct.pack('>%dI' % len(out), *out), calls, refs


def main():
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.split(chr(10))[0])
    ap.add_argument('exe_a')
    ap.add_argument('va_a')
    ap.add_argument('exe_b')
    ap.add_argument('va_b')
    ap.add_argument('tam_b', nargs='?', help='tamano en B (por defecto, el de A)')
    ap.add_argument('--tam-a', required=True, help='tamano en bytes de la funcion de A')
    ap.add_argument('--base-a', default='0x82000000')
    ap.add_argument('--base-b', default='0x82000000')
    a = ap.parse_args()
    ea, eb = Exe(a.exe_a, int(a.base_a, 0)), Exe(a.exe_b, int(a.base_b, 0))
    ta = int(a.tam_a, 0)
    tb = int(a.tam_b, 0) if a.tam_b else ta
    sa, ca, ra = esqueleto(ea, int(a.va_a, 0), ta)
    sb, cb, rb = esqueleto(eb, int(a.va_b, 0), tb)
    n = min(len(sa), len(sb)) // 4
    dif = [i for i in range(n) if sa[4 * i:4 * i + 4] != sb[4 * i:4 * i + 4]]
    print('%s: %d palabras en A, %d en B, %d distintas; llamadas %d / %d' % (
        'IGUAL' if sa == sb else 'DISTINTO', len(sa) // 4, len(sb) // 4, len(dif), len(ca), len(cb)))
    for i in dif[:40]:
        print('  +0x%03X  %s  %s' % (4 * i, sa[4 * i:4 * i + 4].hex(), sb[4 * i:4 * i + 4].hex()))


if __name__ == '__main__':
    main()
