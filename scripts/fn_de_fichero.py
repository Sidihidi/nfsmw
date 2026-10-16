#!/usr/bin/env python3
"""fn_de_fichero.py <trozo-de-ruta> [...] -- las funciones que el .mdebug de la A124 pone en un fichero fuente, en
orden de emision, con direccion, tamano y la cuenta de instrucciones y de llamadas.

Para escribir de cero un fichero de plataforma de PS2 que en nuestro arbol esta VACIO: dice que funciones lleva,
como se llaman (nombre decorado, o sea la firma), en que orden las emitio el compilador y cuanto ocupa cada una.
El cuerpo se saca aparte con `python scripts/ps2fn.py <simbolo>`, que lista sus llamadas en orden.

    python scripts/fn_de_fichero.py psx2/src/ecstasy/elighte.cpp
    python scripts/fn_de_fichero.py psx2/src/ecstasy/ --resumen
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__))).replace('\\', '/') + '/'
sys.path.insert(0, ROOT + 'scripts')
import mdebug  # noqa: E402


def funciones_por_fichero():
    mdebug.carga()
    out = {}
    actual = None
    for iss, value, st, sc in mdebug._cache['syms']:
        if st == 5:
            nom = mdebug._cadena(iss)
            if nom.endswith(('.cpp', '.c', '.cc')):
                actual = nom.lower()
        elif st in (6, 14) and actual:
            out.setdefault(actual, []).append((value, mdebug._cadena(iss)))
    return out


def tamanos():
    """direccion -> tamano, del NFS.MAP."""
    out = {}
    p = ROOT + 'orig/SLES-53558-A124/NFS.MAP'
    if not os.path.exists(p):
        return out
    for l in open(p, errors='replace'):
        m = re.match(r'^([0-9a-fA-F]{8}) ([0-9a-fA-F]{8})\s+\d+\s+(\S+)\s*$', l)
        if m:
            out[int(m.group(1), 16)] = int(m.group(2), 16)
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    resumen = '--resumen' in sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    por = funciones_por_fichero()
    tam = tamanos()
    for pat in args:
        pat = pat.lower().replace('\\', '/')
        for fichero in sorted(f for f in por if pat in f):
            fns = sorted(por[fichero])
            total = sum(tam.get(a, 0) for a, _ in fns)
            print('== %s: %d funciones, %d B' % (fichero, len(fns), total))
            if resumen:
                continue
            for a, n in fns:
                print('   0x%06x %6d B  %s' % (a, tam.get(a, 0), n))


if __name__ == '__main__':
    main()
