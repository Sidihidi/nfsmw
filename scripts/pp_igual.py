#!/usr/bin/env python3
"""pp_igual.py V UNIDAD[,UNIDAD] --sombra=DIR [--sust=REL=COPIA,...] -- prueba de identidad para GameCube y PS2:
preprocesa la SourceList con el arbol y con la sombra (mismos flags que scripts/banco.py) y compara el texto sin
marcas de linea ni lineas en blanco, con la ruta de la sombra normalizada a src/. Si sale IGUAL, el compilador ve
exactamente los mismos tokens en las mismas lineas (__LINE__ incluido): el objeto es identico por construccion.

    V: GOWE69, SLES-53558-A124, SLUS-21351 (o 'consolas' para las tres)
"""
import os
import re
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
AQUI = os.path.dirname(os.path.abspath(__file__))
TMP = os.path.join(RAIZ, 'build', 'pp_igual')
os.makedirs(TMP, exist_ok=True)


def absi(p):
    return os.path.join(RAIZ, p).replace('\\', '/')


def incs(lista):
    fuera = []
    for i in lista:
        fuera += ['-I', absi(i)]
    return fuera


COMUNES = ['src/Speed/Indep/Libs/allocator/1.5.0', 'src/Speed/Indep/Libs/csis/dev/include',
           'src/Packages/eathread/1.1.0/include', 'src/Speed/Indep/Libs/snd/9/include',
           'src/Speed/Indep/Libs/spch/dev/include', 'src/Speed/Indep/Libs/path/5.01.04/include',
           'src/Speed/Indep/Libs/realcore/6.24.00/include/common', 'src/Speed/Indep/Libs/endian/0.5.2/include']


def cflags_de(V):
    A124 = V == 'SLES-53558-A124'
    if V == 'GOWE69':
        cflags = ['-O1', '-gdwarf+', '-Wno-ctor-dtor-privacy', '-Woverloaded-virtual', '-Wno-multichar']
        cflags += incs(['src/Speed/Indep/Libs/Support/stlgc', 'src/Speed/GameCube/Libs/stl/STLport-4.5/stlport',
                        'src/Speed/GameCube/bWare/GameCube/SN/include', 'src/Packages/eathread/1.1.0/include',
                        'src/Speed/GameCube/bWare/GameCube/dolphinsdk/include', 'src/Packages',
                        'src/Speed/GameCube/bWare/GameCube/SN', 'src'])
        cflags += ['-DEA_PLATFORM_GAMECUBE', '-DEA_REGION_AMERICA', '-DGEKKO', '-D_USE_MATH_DEFINES',
                   '-I', absi('build/GOWE69/include'), '-DBUILD_VERSION=0', '-DVERSION_GOWE69', '-DNDEBUG=1',
                   '-mps-nodf', '-G0', '-fno-static-dtors', '-ffast-math',
                   '-DLUA_NUMBER=float', '-DDISABLE_RAIN', '-DDEFAULT_ALLOCATOR=0', '-DUSEREALCORE']
        cflags += incs(COMUNES) + ['-x', 'c++']
    else:
        cflags = ['-O2', '-g2', '-Wno-ctor-dtor-privacy']
        cflags += incs(['src/Speed/Indep/Libs/Support/stlps2', 'src/Speed/PSX2/Libs/stl/gpp',
                        'src/Speed/PSX2/bWare/src/ee/include', 'src/Speed/PSX2/bWare/src/ee/gcc/ee/include',
                        'src/Speed/PSX2/bWare/src/ee/gcc/lib/gcc-lib/ee/2.9-ee-991111/include',
                        'src/Speed/PSX2/bWare/src/ee/gcc/ee', 'src/Speed/PSX2/bWare/src/ee/gcc/lib/gcc-lib/ee/2.9-ee-991111',
                        'src/Packages', 'src'])
        cflags += ['-DEA_PLATFORM_PLAYSTATION2'] + (['-DEA_BUILD_A124'] if A124 else []) + ['-D_NOTHREADS', '-D_STL=']
        cflags += ['-DEA_REGION_EUROPE'] if A124 else ['-DEA_REGION_AMERICA']
        cflags += ['-I', absi('build/%s/include' % V), '-DBUILD_VERSION=%d' % (2 if A124 else 3)]
        cflags += ['-G0', '-ffast-math', '-fno-exceptions', '-fno-rtti', '-DLUA_NUMBER=float']
        cflags += ['-mvu0-use-vf0-vf15']
        cflags += (['-DMILESTONE_BUILD'] if A124 else []) + ['-DDEFAULT_ALLOCATOR=0', '-DUSEREALCORE']
        cflags += incs(COMUNES) + ['-x', 'c++']
    return cflags


def pp(V, fuente, sombra, extra):
    cflags = cflags_de(V)
    if sombra:
        cflags[0:0] = ['-I', sombra.replace('\\', '/')]
    for d in extra:
        cflags[-2:-2] = ['-I', d]
    salida = fuente + '.' + V + '.i'
    if V == 'GOWE69':
        prodg = os.path.join(RAIZ, 'build', 'compilers', 'ProDG', '3.9.3')
        r = subprocess.run([os.path.join(prodg, 'ngccc.exe')] + cflags + ['-E', '-o', salida, fuente], cwd=TMP,
                           capture_output=True, text=True, env=dict(os.environ, SN_NGC_PATH=prodg))
    else:
        cc = os.path.join(RAIZ, 'build', 'compilers', 'PS2', 'ee-gcc2.9-991111', 'bin', 'ee-gcc.exe')
        r = subprocess.run([cc] + cflags + ['-E', '-o', salida, fuente], cwd=TMP, capture_output=True, text=True)
    if r.returncode or not os.path.exists(salida):
        print('   fallo el preprocesado', (r.stdout + r.stderr)[-1500:])
        return None
    t = open(salida, encoding='utf-8', errors='replace').read()
    os.remove(salida)
    return t


def normal(t, sombra):
    s1 = sombra.replace('\\', '/')
    s2 = sombra.replace('/', '\\')
    src = absi('src')
    t = t.replace(s1, src).replace(s2, src.replace('/', '\\'))
    t = t.replace(s1.replace('/', '\\\\'), src.replace('/', '\\\\'))
    fuera = []
    for linea in t.split('\n'):
        if re.match(r'^\s*#\s*(\d+|line)\b', linea) or not linea.strip():
            continue
        fuera.append(linea.rstrip())
    return fuera


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    vs = ['GOWE69', 'SLES-53558-A124', 'SLUS-21351'] if args[0] == 'consolas' else args[0].split(',')
    unidades = args[1].split(',')
    sombra, sust = None, []
    for a in sys.argv[1:]:
        if a.startswith('--sombra='):
            sombra = os.path.abspath(a.split('=', 1)[1])
        elif a.startswith('--sust='):
            for par in a.split('=', 1)[1].split(','):
                rel, copia = par.split('=', 1)
                sust.append((rel, os.path.abspath(copia)))
    malos = 0
    for u in unidades:
        texto = open(os.path.join(RAIZ, 'src', 'Speed', 'Indep', 'SourceLists', u + '.cpp'), encoding='utf-8').read()
        t2 = texto
        extra = []
        sust_u = list(sust)
        if sombra:
            for rel in re.findall(r'#include "([^"]+)"', texto):
                if os.path.isfile(os.path.join(sombra, rel)) and not any(r == rel for r, _ in sust_u):
                    sust_u.append((rel, os.path.join(sombra, rel)))
        for rel, copia in sust_u:
            viejo = '#include "%s"' % rel
            if t2.count(viejo) != 1:
                continue
            t2 = t2.replace(viejo, '#include "%s"' % copia.replace('\\', '/'))
            d = absi('src/' + os.path.dirname(rel))
            if d not in extra:
                extra.append(d)
        f1 = os.path.join(TMP, '%s__arbol.cpp' % u)
        f2 = os.path.join(TMP, '%s__sombra.cpp' % u)
        open(f1, 'w', encoding='utf-8').write(texto)
        open(f2, 'w', encoding='utf-8').write(t2)
        for V in vs:
            a = pp(V, f1, None, extra)
            b = pp(V, f2, sombra, extra)
            if a is None or b is None:
                malos += 1
                continue
            na, nb = normal(a, sombra), normal(b, sombra)
            if na == nb:
                print('%-16s %-18s IGUAL (%d lineas de codigo)' % (V, u, len(na)))
            elif ' '.join(na).split() == ' '.join(nb).split():
                print('%-16s %-18s MISMOS TOKENS (cambia solo el reparto en lineas)' % (V, u))
            else:
                malos += 1
                import difflib
                dif = list(difflib.unified_diff(na, nb, lineterm='', n=1))
                print('%-16s %-18s DISTINTO (%d lineas de diff)' % (V, u, len(dif)))
                for x in dif[:40]:
                    print('     ', x[:200])
        for f in (f1, f2):
            os.remove(f)
    sys.exit(1 if malos else 0)


if __name__ == '__main__':
    main()
