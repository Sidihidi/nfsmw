#!/usr/bin/env python3
"""capacidades.py [SPEED_EXE_1_3|EUROPEGERMILESTONE] [--nombres] -- la capacidad de cada lista de plantilla
(UTL::FixedVector, _Storage, Listable::List...) del original frente a la nuestra, unidad a unidad.

Una vtable de FixedVector tiene AllocVectorSpace en el slot +4 (`lea eax, [ecx+0x10] ; ret 8` en x86,
`addi r3, r3, 0x10 ; blr` en PowerPC) y GetMaxCapacity en el +16 (`mov eax, N ; ret` / `li r3, N ; blr`): N es la
capacidad. En PC la vtable del .obj troceado lleva reubicaciones a sus funciones y la capacidad del original se lee
directamente; asi se vio que PC devuelve 20 donde nosotros 10 (memoria nfsmw-limites-pc-x360.md). En Xbox 360 el
troceado no trae reubicaciones en las vtables: alli la prueba son los TAMANOS de los datos (_mTable, _mLists...) de
config/EUROPEGERMILESTONE/symbols.txt y los desplazamientos (scripts/corridos.py).

    python scripts/capacidades.py                   # PC, cuenta por unidad
    python scripts/capacidades.py SPEED_EXE_1_3 --nombres
"""
import collections
import contextlib
import glob
import io
import os
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
args = [a for a in sys.argv[1:] if not a.startswith('--')]
VERSION = args[0] if args else 'SPEED_EXE_1_3'
NOMBRES = '--nombres' in sys.argv
sys.argv = ['x', VERSION, 'ninguna']  # nombra_por_codigo lee sys.argv al importarse
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
with contextlib.redirect_stdout(io.StringIO()):
    from nombra_por_codigo import Coff  # noqa: E402

PC = VERSION == 'SPEED_EXE_1_3'
ALLOC = bytes.fromhex('8d4110c20800') if PC else bytes.fromhex('386300104e800020')
SL = os.path.join('Speed', 'Indep', 'SourceLists')


def capacidad(b):
    if PC:
        return int.from_bytes(b[1:5], 'little') if b[:1] == b'\xb8' and b[5:6] == b'\xc3' else None
    if b[:2] == bytes.fromhex('3860') and b[4:8] == bytes.fromhex('4e800020'):
        return int.from_bytes(b[2:4], 'big')
    return None


def capacidades(ruta):
    c = Coff(ruta)
    funcs = {n: b for n, b, rel, pos in c.funciones()}
    out = []
    for si, s in enumerate(c.secciones):
        if s['car'] & 0x20:
            continue
        rels = dict(sorted((r[0], c.por_indice.get(r[1], (0, '?'))[1]) for r in s['relocs']))
        for off in rels:
            if not funcs.get(rels.get(off + 4, ''), b'').startswith(ALLOC):
                continue
            cap = capacidad(funcs.get(rels.get(off + 16, ''), b''))
            if cap is None:
                continue
            nombres = sorted((not x[1].startswith('??_7'), x[1]) for x in c.simbolos if x[3] == si + 1 and x[2] == off)
            out.append((cap, nombres[0][1] if nombres else '?'))
    return out


def main():
    for u in sorted(glob.glob(os.path.join(RAIZ, 'build', VERSION, 'obj', SL, '*.obj'))):
        un = os.path.basename(u)
        nuestro = os.path.join(RAIZ, 'build', VERSION, 'src', SL, un)
        o = capacidades(u)
        n_ = capacidades(nuestro) if os.path.exists(nuestro) else []
        co = collections.Counter(x[0] for x in o)
        cn = collections.Counter(x[0] for x in n_)
        if co or cn:
            print('%-22s original %-44s nuestro %s' % (un, dict(sorted(co.items())), dict(sorted(cn.items()))))
        if NOMBRES:
            for cap, nm in o:
                print('      original %5s %s' % (cap, nm[:110]))
            for cap, nm in n_:
                print('      nuestro  %5s %s' % (cap, nm[:110]))


if __name__ == '__main__':
    main()
