#!/usr/bin/env python3
"""pc_x360.py -- comprueba que PC (MSVC 7.1) y X360 (MSVC PPC 14.00) siguen COMPILANDO unas SourceLists, con una
variante de ronda.py o con el arbol vivo. No compara nada con el original: solo dice si compila y, si no, los errores.

    python scripts/pc_x360.py --dir=DIR VAR UNIDAD[,UNIDAD...] [--solo=pc|x360]
        la copia congelada de DIR (ronda.py --congelar) con la variante DIR/variantes/VAR (o `base`): sombra de
        cabeceras y copias de .cpp, como ronda.py
    python scripts/pc_x360.py UNIDAD[,UNIDAD...] [--solo=pc|x360]
        el arbol vivo (src/) tal cual

UNIDAD es la SourceList sin extension (zAI, zWorld2...), @fichero o `todas`. Los flags son los de las unidades de
juego de configure.py para EUROPEGERMILESTONE (X360) y SPEED_EXE_1_3 (PC). Los objetos van a DIR/obra_pc (o a
build/pc_x360 sin --dir) y se borran al terminar. Sale con codigo 1 si alguna unidad no compila.
"""
import os
import shutil
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import ronda  # noqa: E402

COMUNES = ['Speed/Indep/Libs/allocator/1.5.0', 'Speed/Indep/Libs/csis/dev/include', 'Packages/eathread/1.1.0/include',
           'Speed/Indep/Libs/snd/9/include', 'Speed/Indep/Libs/spch/dev/include',
           'Speed/Indep/Libs/path/5.01.04/include', 'Speed/Indep/Libs/realcore/6.24.00/include/common',
           'Speed/Indep/Libs/endian/0.5.2/include']


def flags(plat, src, sombra, extra_i):
    """cflags_game de configure.py para X360 (EUROPEGERMILESTONE) o PC (SPEED_EXE_1_3), con SRC como src/"""
    f = ['/nologo', '/c', '/wd4996', '/wd4355', '/wd4716']
    if sombra:
        f += ['/I', sombra]
    if plat == 'pc':
        v = 'SPEED_EXE_1_3'
        f += ['/Ox', '/Z7', '/EHsc', '/I', os.path.join(RAIZ, 'build', 'compilers', 'Win32', '7.1', 'Include'),
              '/I', os.path.join(src, 'Packages'), '/I', src, '/DEA_PLATFORM_WIN32', '/D_USE_MATH_DEFINES',
              '/I', os.path.join(RAIZ, 'build', v, 'include'), '/DBUILD_VERSION=4', '/DVERSION_' + v]
        f += ['/DLUA_NUMBER=float', '/DDEFAULT_ALLOCATOR=0', '/DUSEREALCORE']
    else:
        v = 'EUROPEGERMILESTONE'
        f += ['/Og', '/Ob2', '/Oi', '/Oz', '/GF', '/EHsc',
              '/I', os.path.join(src, 'Packages', 'xenonsdk', '2.0.2135.2', 'installed', 'include', 'xbox'),
              '/I', os.path.join(src, 'Packages'), '/I', src, '/DEA_PLATFORM_XENON', '/D_USE_MATH_DEFINES',
              '/I', os.path.join(RAIZ, 'build', v, 'include'), '/DBUILD_VERSION=1', '/DVERSION_' + v]
        f += ['/DLUA_NUMBER=float', '/DMILESTONE_BUILD', '/DDEFAULT_ALLOCATOR=0', '/DUSEREALCORE']
    for x in COMUNES:
        f += ['/I', os.path.join(src, x)]
    for x in extra_i:
        f += ['/I', x]
    return f


def main():
    opts = [a for a in sys.argv[1:] if a.startswith('--')]
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    d = None
    solo = None
    for a in opts:
        if a.startswith('--dir='):
            d = os.path.abspath(a.split('=', 1)[1])
        elif a.startswith('--solo='):
            solo = a[7:]
    if (d and len(args) < 2) or (not d and len(args) < 1):
        print(__doc__)
        return 2
    if d:
        var, lista = args[0], args[1]
        src = os.path.join(ronda.congelado(d), 'src')
        sombra, copias = ronda.prepara(d, 'pc', var)
        obra = os.path.join(d, 'obra_pc')
    else:
        var, lista = 'arbol', args[0]
        src = os.path.join(RAIZ, 'src')
        sombra, copias = None, {}
        obra = os.path.join(RAIZ, 'build', 'pc_x360')
    if lista == 'todas':
        unidades = sorted(f[:-4] for f in os.listdir(os.path.join(src, ronda.SL_REL)) if f.endswith('.cpp'))
    elif lista.startswith('@'):
        unidades = [u for u in open(lista[1:], encoding='utf-8').read().replace(',', ' ').split() if u]
    else:
        unidades = lista.split(',')
    os.makedirs(obra, exist_ok=True)
    fallos = 0
    for plat in ('pc', 'x360'):
        if solo and plat != solo:
            continue
        cl = os.path.join(RAIZ, 'build', 'compilers', 'Win32', '7.1', 'cl.exe') if plat == 'pc' else \
            os.path.join(RAIZ, 'build', 'compilers', 'X360', '14.00.2110', 'cl.exe')
        for sl in unidades:
            texto = open(os.path.join(src, ronda.SL_REL, sl + '.cpp'), encoding='utf-8').read()
            extra_i = []
            for rel, copia in copias.items():
                viejo = '#include "%s"' % rel
                if texto.count(viejo) == 1:
                    texto = texto.replace(viejo, '#include "%s"' % copia)
                    extra_i.append(os.path.join(src, os.path.dirname(rel)))
            fuente = os.path.join(obra, '%s__%s__%s.cpp' % (sl, var, plat))
            open(fuente, 'w', encoding='utf-8').write(texto)
            obj = fuente[:-4] + '.obj'
            r = subprocess.run([cl] + flags(plat, src, sombra, extra_i) + ['/Fo' + obj, fuente], capture_output=True,
                               text=True, cwd=obra, errors='replace')
            errs = [x for x in (r.stdout + r.stderr).splitlines() if ' error ' in x or 'fatal error' in x]
            ok = os.path.exists(obj) and r.returncode == 0
            fallos += 0 if ok else 1
            print('%-5s %-18s %-8s %s' % (plat, sl, var, 'OK' if ok else 'FALLA (%d errores)' % len(errs)), flush=True)
            for x in errs[:12]:
                print('      ', x.replace(src, '')[:220])
            for x in (fuente, obj):
                try:
                    os.remove(x)
                except OSError:
                    pass
    if sombra and os.path.isdir(sombra):
        shutil.rmtree(sombra)
    return 1 if fallos else 0


if __name__ == '__main__':
    sys.exit(main())
