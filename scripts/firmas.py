#!/usr/bin/env python3
"""firmas.py [--version V] [unidad ...] -- funciones del original que nuestro objeto no define con ese nombre mangled
pero SI con el mismo Clase::metodo y otra firma.

objdiff empareja por nombre mangled (GCC 2.95): un parametro de mas o de menos, un `const` de mas o un tipo distinto
dejan la funcion a 0 % aunque el cuerpo case. En la A124 salieron 24,7 kB asi (el `who` de
Sim::Collision::AddListener, `char *` frente a `const char *`, `unsigned long` frente a `unsigned long long`...).

Compara build/<V>/obj/.../<u>.o (el original troceado) con build/<V>/src/.../<u>.o (el nuestro); por defecto
V = SLES-53558-A124. Sin unidades recorre todas. Imprime por unidad las parejas (original | nuestras) con el tamano
del original.

OJO con SLUS-21351: sus nombres vienen heredados de la A124, asi que una pareja en SLUS puede ser un nombre mal
puesto en config/SLUS-21351/symbols.txt y no una firma distinta. scripts/argsusados.py lo decide por el asm.
"""
import glob
import os
import re
import subprocess
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__))).replace('\\', '/') + '/'
NM = ROOT + 'build/mips_binutils/mips-linux-gnu-nm.exe'


def funciones(obj):
    r = subprocess.run([NM, '-S', '--defined-only', obj], capture_output=True, text=True, errors='replace')
    out = {}
    for l in r.stdout.splitlines():
        p = l.split()
        # el troceado define cada funcion dos veces: `X` y `X.NON_MATCHING`
        if len(p) == 4 and p[2] in 'Tt' and not p[3].endswith('.NON_MATCHING'):
            out[p[3]] = int(p[1], 16)
    return out


def nombre_con_longitud(s, i):
    m = re.match(r'\d+', s[i:])
    if not m:
        return None, i
    n = int(m.group(0))
    j = i + len(m.group(0))
    return s[j:j + n], j + n


def clase(s, i):
    """clase calificada que empieza en s[i]; devuelve (texto, fin) o (None, i)"""
    if i < len(s) and s[i] == 'Q':
        if s[i + 1] == '_':
            m = re.match(r'Q_(\d+)_', s[i:])
            if not m:
                return None, i
            k, j = int(m.group(1)), i + len(m.group(0))
        else:
            k, j = int(s[i + 1]), i + 2
        partes = []
        for _ in range(k):
            if j < len(s) and s[j] == 't':
                return None, i  # plantilla: no se intenta
            nom, j = nombre_con_longitud(s, j)
            if nom is None:
                return None, i
            partes.append(nom)
        return '::'.join(partes), j
    if i < len(s) and s[i].isdigit():
        return nombre_con_longitud(s, i)
    return None, i


def clave(m):
    """Clase::metodo sin parametros, o None si no se sabe leer"""
    if m.startswith('_$_'):
        c, _ = clase(m, 3)
        return None if c is None else c + '::~'
    for k in range(0, len(m) - 2):
        if m[k:k + 2] != '__' or (k > 0 and m[k - 1] == '_'):
            continue
        nombre, i = m[:k], k + 2
        if i < len(m) and m[i] == 'C':
            i += 1
        if i < len(m) and m[i] == 'F':
            return nombre or None
        c, j = clase(m, i)
        if c is not None:
            return c + '::' + (nombre or '<ctor>')
    return None


def main():
    args = sys.argv[1:]
    version = 'SLES-53558-A124'
    if args[:1] == ['--version']:
        version, args = args[1], args[2:]
    T = ROOT + 'build/%s/obj/Speed/Indep/SourceLists/' % version
    O = ROOT + 'build/%s/src/Speed/Indep/SourceLists/' % version
    unidades = args or sorted(os.path.basename(p)[:-2] for p in glob.glob(T + '*.o'))
    total = 0
    for u in unidades:
        if not os.path.exists(T + u + '.o') or not os.path.exists(O + u + '.o'):
            continue
        t, o = funciones(T + u + '.o'), funciones(O + u + '.o')
        nuestras = defaultdict(list)
        for n in set(o) - set(t):
            k = clave(n)
            if k:
                nuestras[k].append(n)
        filas = []
        for n in sorted(set(t) - set(o)):
            k = clave(n)
            if k and k in nuestras:
                filas.append((t[n], n, nuestras[k]))
        if filas:
            b = sum(f[0] for f in filas)
            total += b
            print('== %s: %d funciones, %d B' % (u, len(filas), b))
            for s, n, ns in sorted(filas, reverse=True):
                print('   %6d  %-70s | %s' % (s, n, ' '.join(ns)))
    print('TOTAL %d B' % total)


if __name__ == '__main__':
    main()
