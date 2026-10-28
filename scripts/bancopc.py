#!/usr/bin/env python3
"""bancopc.py -- banco de pruebas de PC (MSVC 7.1) y Xbox 360 (MSVC PPC 14.00): compila una SourceList FUERA del
arbol, con cabeceras de sombra y copias de .cpp, y la compara con el objeto original troceado.

    python bancopc.py V UNIDAD ETIQUETA [--sombra=DIR] [--sust=REL=COPIA[,REL=COPIA]] [--base=ETIQ] [--clase=A,B]
                      [--cl=OPCION[,OPCION]] [--conservar]

    V         SPEED_EXE_1_3 o EUROPEGERMILESTONE
    UNIDAD    la SourceList sin extension (zFe, zFe2...)
    ETIQUETA  nombre de la corrida: guarda build/bancopc/<V>_<UNIDAD>_<ETIQUETA>.json con {funcion: (pct, tam)}
    --sombra  directorio con forma de src/ cuyas cabeceras van antes que /I src (el directorio ENTERO de cada
              cabecera tocada, porque los #include relativos no pasan por /I)
    --sust    REL (ruta que incluye la SourceList, relativa a src/) sustituida por COPIA
    --base    compara con la corrida ETIQ: ganadas, perdidas, y bytes
    --clase   ademas lista las funciones de esas clases con su porcentaje
    --cl      opciones de cl.exe que se anaden al final (p. ej. --cl=/Gy)
    --lista   otra SourceList (una copia de src/Speed/Indep/SourceLists/UNIDAD.cpp con otros #include)

Sin sombra ni sust es la corrida de control: tiene que dar lo mismo que el informe.
"""
import json
import os
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
AQUI = os.path.dirname(os.path.abspath(__file__))
BANCO = os.path.join(RAIZ, 'build', 'bancopc')
os.makedirs(BANCO, exist_ok=True)
COMUNES = ['Speed/Indep/Libs/allocator/1.5.0', 'Speed/Indep/Libs/csis/dev/include', 'Packages/eathread/1.1.0/include',
           'Speed/Indep/Libs/snd/9/include', 'Speed/Indep/Libs/spch/dev/include',
           'Speed/Indep/Libs/path/5.01.04/include', 'Speed/Indep/Libs/realcore/6.24.00/include/common',
           'Speed/Indep/Libs/endian/0.5.2/include']
STLPC = os.path.join(RAIZ, 'src', 'Speed', 'Indep', 'Libs', 'Support', 'stlpc')


def flags(v, sombra, extra_i):
    src = os.path.join(RAIZ, 'src')
    f = ['/nologo', '/c', '/wd4996', '/wd4355', '/wd4716']
    if v == 'SPEED_EXE_1_3':
        f += ['/O2', '/Z7', '/EHsc']  # los de configure.py (PC es /O2 desde la ventana 6)
        if sombra:
            f += ['/I', sombra]
        f += ['/I', STLPC]  # la xmemory de EA delante de las del compilador (como configure.py)
        f += ['/I', os.path.join(RAIZ, 'build', 'compilers', 'Win32', '7.1', 'Include'),
              '/I', os.path.join(src, 'Packages'), '/I', src, '/DEA_PLATFORM_WIN32', '/D_USE_MATH_DEFINES',
              '/I', os.path.join(RAIZ, 'build', v, 'include'), '/DBUILD_VERSION=4', '/DVERSION_' + v]
        f += ['/DLUA_NUMBER=float', '/DDEFAULT_ALLOCATOR=0', '/DUSEREALCORE']
    else:
        f += ['/Og', '/Ob2', '/Oi', '/Oz', '/GF']  # los de configure.py: sin /EHsc desde la ventana 7
        if sombra:
            f += ['/I', sombra]
        f += ['/I', STLPC]  # la xmemory de EA tambien en X360 (configure.py, ronda 39)
        f += ['/I', os.path.join(src, 'Packages', 'xenonsdk', '2.0.2135.2', 'installed', 'include', 'xbox'),
              '/I', os.path.join(src, 'Packages'), '/I', src, '/DEA_PLATFORM_XENON', '/D_USE_MATH_DEFINES',
              '/D_SECURE_SCL=0',  # como configure.py (ronda 42): iteradores de una palabra en el original
              '/DNOD3D',  # como configure.py (ronda 48): <xtl.h> sin D3D en bDebug.cpp
              '/D_XBOX',  # como configure.py (ronda 50): d3dx9.h toma XObjBase.h (eMathE, bTransposeMatrix)
              '/I', os.path.join(RAIZ, 'build', v, 'include'), '/DBUILD_VERSION=1', '/DVERSION_' + v]
        f += ['/DLUA_NUMBER=float', '/DMILESTONE_BUILD', '/DDEFAULT_ALLOCATOR=0', '/DUSEREALCORE']
    for x in COMUNES:
        f += ['/I', os.path.join(src, x)]
    for x in extra_i:
        f += ['/I', x]
    return f


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if len(args) < 3:
        print(__doc__)
        return 2
    v, unidad, etiqueta = args
    sombra, sust, base, clases, cl_extra = None, [], None, [], []
    for a in sys.argv[1:]:
        if a.startswith('--sombra='):
            sombra = os.path.abspath(a.split('=', 1)[1])
        elif a.startswith('--sust='):
            for par in a.split('=', 1)[1].split(','):
                rel, copia = par.split('=', 1)
                sust.append((rel, os.path.abspath(copia)))
        elif a.startswith('--base='):
            base = a.split('=', 1)[1]
        elif a.startswith('--clase='):
            clases = a.split('=', 1)[1].split(',')
        elif a.startswith('--cl='):
            cl_extra = a.split('=', 1)[1].split(',')
    lista = os.path.join(RAIZ, 'src', 'Speed', 'Indep', 'SourceLists', unidad + '.cpp')
    for a in sys.argv[1:]:
        if a.startswith('--lista='):
            lista = os.path.abspath(a.split('=', 1)[1])
    texto = open(lista, encoding='utf-8').read()
    extra_i = []
    if sombra:
        # toda .cpp de la SourceList que tenga copia en la sombra se compila desde la sombra: asi sus #include
        # relativos ven las cabeceras de la sombra y no las del arbol
        import re as _re
        for rel in _re.findall(r'#include "([^"]+)"', texto):
            if os.path.isfile(os.path.join(sombra, rel)) and not any(r == rel for r, _ in sust):
                # solo las que DIFIEREN del arbol (ronda 87): una cabecera copiada tal cual que la SourceList incluye
                # dos veces (UStandard.h y UCollections.h en zPhysics) no se sustituye y no rompe la corrida
                try:
                    igual = open(os.path.join(sombra, rel), 'rb').read() == \
                        open(os.path.join(RAIZ, 'src', rel), 'rb').read()
                except OSError:
                    igual = False
                if not igual:
                    sust.append((rel, os.path.join(sombra, rel)))
    for rel, copia in sust:
        viejo = '#include "%s"' % rel
        if texto.count(viejo) == 0:
            continue
        assert texto.count(viejo) == 1, 'la SourceList incluye dos veces %s' % rel
        texto = texto.replace(viejo, '#include "%s"' % copia.replace('\\', '/'))
        d = os.path.join(RAIZ, 'src', os.path.dirname(rel))
        if d not in extra_i:
            extra_i.append(d)
    fuente = os.path.join(BANCO, '%s_%s_%s.cpp' % (v, unidad, etiqueta))
    open(fuente, 'w', encoding='utf-8').write(texto)
    obj = fuente[:-4] + '.obj'
    if os.path.exists(obj):
        os.remove(obj)
    cl = os.path.join(RAIZ, 'build', 'compilers', 'Win32', '7.1', 'cl.exe') if v == 'SPEED_EXE_1_3' else \
        os.path.join(RAIZ, 'build', 'compilers', 'X360', '14.00.2110', 'cl.exe')
    r = subprocess.run([cl] + flags(v, sombra, extra_i) + cl_extra + ['/Fo' + obj, fuente], capture_output=True, text=True,
                       cwd=BANCO, errors='replace')
    if r.returncode or not os.path.exists(obj):
        errs = [x for x in (r.stdout + r.stderr).splitlines() if 'error' in x]
        print('NO COMPILA:')
        for x in errs[:25]:
            print('  ', x[:250])
        os.remove(fuente)
        return 1
    if v == 'SPEED_EXE_1_3':
        # como pctall.py: sin las etiquetas $L que cortan las funciones estaticas en objdiff y con los funclets de EH
        # con su nombre posicional (scripts/quita_L.py; sin los mapeos de pctall, control y prueba por igual)
        sys.path.insert(0, AQUI)
        import quita_L
        datos = open(obj, 'rb').read()
        try:
            r = quita_L.para_medir(datos)
            if r:
                open(obj, 'wb').write(r[0])
        except quita_L.NoSe as e:
            print('quita_L: se mide con etiquetas (%s)' % e)
    orig = os.path.join(RAIZ, 'build', v, 'obj', 'Speed', 'Indep', 'SourceLists', unidad + '.obj')
    salida = os.path.join(BANCO, 'd_%s_%s_%s.json' % (v, unidad, etiqueta))
    r = subprocess.run([os.path.join(RAIZ, 'build', 'tools', 'objdiff-cli.exe'), 'diff', '-1', orig, '-2', obj,
                        '-c', 'functionRelocDiffs=none', '-c', 'ppc.calculatePoolRelocations=false',
                        '-o', salida, '--format', 'json'], capture_output=True, text=True)
    if r.returncode:
        print('objdiff fallo', r.stderr[-800:])
        return 1
    d = json.load(open(salida, encoding='utf-8'))
    res = {}
    for s in d['left']['symbols']:
        if s.get('kind') != 'SYMBOL_FUNCTION':
            continue
        n = s.get('name', '')
        res[n] = (s.get('match_percent'), int(s.get('size') or 0))
    json.dump(res, open(os.path.join(BANCO, '%s_%s_%s.json' % (v, unidad, etiqueta)), 'w'), indent=0)
    casadas = [n for n, (p, t) in res.items() if p == 100.0]
    print('%s %s %s: %d funciones al 100 %% (%d B) de %d' % (v, unidad, etiqueta, len(casadas),
                                                           sum(res[n][1] for n in casadas), len(res)))
    if base:
        b = json.load(open(os.path.join(BANCO, '%s_%s_%s.json' % (v, unidad, base))))
        gan = [n for n in res if res[n][0] == 100.0 and (b.get(n) or [None])[0] != 100.0]
        per = [n for n in b if b[n][0] == 100.0 and (res.get(n) or [None])[0] != 100.0]
        print('  frente a %s: +%d (%d B)  -%d (%d B)' % (base, len(gan), sum(res[n][1] for n in gan), len(per),
                                                        sum(b[n][1] for n in per)))
        for n in sorted(per):
            print('   PIERDE %-90s %s -> %s' % (n[:90], b[n][0], (res.get(n) or [None])[0]))
        for n in sorted(gan):
            print('   gana   %-90s %s' % (n[:90], (b.get(n) or [None])[0]))
        if '--mejoras' in sys.argv:
            for n in sorted(res):
                p = res[n][0] or 0
                q = (b.get(n) or [0])[0] or 0
                if p != q and p != 100.0 and q != 100.0:
                    print('   %s %-86s %8.3f -> %8.3f' % ('mejor ' if p > q else 'peor  ', n[:86], q, p))
    for c in clases:
        for n in sorted(res):
            if ('@%s@@' % c) in n:
                print('   %8.3f %5d %s' % (res[n][0] or 0, res[n][1], n[:100]))
    if '--conservar' not in sys.argv:
        for q in (fuente, obj, salida):
            try:
                os.remove(q)
            except OSError:
                pass
    return 0


if __name__ == '__main__':
    sys.exit(main())
