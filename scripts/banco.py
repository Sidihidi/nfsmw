#!/usr/bin/env python3
"""Banco de pruebas: compila una SourceList FUERA del arbol, con uno o varios de sus .cpp sustituidos por copias, y
compara funciones contra el objeto ORIGINAL de build/<V>/obj con objdiff.

Solo escribe en build/banco/ (la SourceList de prueba, su preprocesado y los objetos): no toca src/ ni los objetos
que usa la medida, asi que sirve mientras otra medida tiene el candado (build/.medir.lock) y para probar variantes en
GameCube, la A124 y SLUS sin tocar el arbol. Lo que sale bien se aplica despues y se mide con medir.py.

    python scripts/banco.py V SOURCELIST REL[,REL...] COPIA[,COPIA...] SIM[,SIM...] [--diff] [--sombra=DIR]

    V          GOWE69, SLES-53558-A124 o SLUS-21351
    SOURCELIST la unidad sin extension (zFe2, zTrack...), que es la que incluye los .cpp
    REL        la ruta que la SourceList incluye, relativa a src/ (Speed/Indep/Src/...)
    COPIA      la copia modificada de ese fichero (en cualquier sitio)
    SIM        los simbolos, separados por comas, o @fichero con ellos (por comas o por lineas)
    --diff     imprime las instrucciones de cada funcion, original | nuestra
    --sombra   un directorio con la forma de src/ cuyas cabeceras tapan a las del arbol (va antes que todo -I)
    --extra    flags propios de la unidad, entre comillas (p. ej. --extra="-mps-float" para zEcstasy en GC)
    --conservar  no borra al terminar la SourceList de prueba, su preprocesado, el objeto ni los JSON
    --asm=RUTA   compila tambien con -save-temps y deja el .s (con los stabs) en RUTA (scripts/estructura.py)

Los #include "x.h" relativos de las copias se resuelven en el directorio del fichero original. Los flags son los de
las unidades de juego de configure.py (cflags_game de GC y de PS2); las unidades con flags propios (middleware,
zEcstasy en GC con -mps-float...) no estan cubiertas. Comprobacion: una copia sin cambios tiene que dar el mismo
porcentaje que el informe.
"""
import json
import os
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
B = os.path.join(RAIZ, 'build', 'banco')
os.makedirs(B, exist_ok=True)

args = [a for a in sys.argv[1:] if not a.startswith('--')]
if len(args) < 5:
    print(__doc__)
    sys.exit(2)
V, SL, SIMS = args[0], args[1], args[4].split(',')
if args[4].startswith('@'):
    # @fichero: los simbolos separados por comas o por lineas; los de una unidad entera no caben en la linea de
    # ordenes de Windows (32.767 caracteres).
    SIMS = [s.strip() for s in open(args[4][1:], encoding='utf-8').read().split() if s.strip()]
    SIMS = [s for linea in SIMS for s in linea.split(',') if s]
RELS, COPIAS = args[2].split(','), [os.path.abspath(c) for c in args[3].split(',')]
assert len(RELS) == len(COPIAS), 'tantas copias como ficheros'
DIFF = '--diff' in sys.argv
A124 = V == 'SLES-53558-A124'
GC = V == 'GOWE69'


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
if GC:
    cflags = ['-O1', '-gdwarf+', '-Wno-ctor-dtor-privacy', '-Woverloaded-virtual', '-Wno-multichar']
    cflags += incs(['src/Speed/Indep/Libs/Support/stlgc', 'src/Speed/GameCube/Libs/stl/STLport-4.5/stlport',
                    'src/Speed/GameCube/bWare/GameCube/SN/include', 'src/Packages/eathread/1.1.0/include',
                    'src/Speed/GameCube/bWare/GameCube/dolphinsdk/include', 'src/Packages',
                    'src/Speed/GameCube/bWare/GameCube/SN', 'src'])
    cflags += ['-DEA_PLATFORM_GAMECUBE', '-DEA_REGION_AMERICA', '-DGEKKO', '-D_USE_MATH_DEFINES',
               '-I', absi('build/GOWE69/include'), '-DBUILD_VERSION=0', '-DVERSION_GOWE69', '-DNDEBUG=1',
               '-mps-nodf', '-G0', '-fno-static-dtors', '-ffast-math', '-fforce-addr', '-fcse-follow-jumps',
               '-fcse-skip-blocks', '-fforce-mem', '-fgcse', '-frerun-cse-after-loop', '-fschedule-insns',
               '-fschedule-insns2', '-fexpensive-optimizations', '-frerun-loop-opt', '-fmove-all-movables',
               '-DLUA_NUMBER=float', '-DDISABLE_RAIN', '-DDEFAULT_ALLOCATOR=0', '-DUSEREALCORE',
               # las unidades de juego de GameCube se compilaron con -fkeep-inline-functions (configure.py, keep_inline_gc)
               '-fkeep-inline-functions']
    if SL != 'zBWare':
        # y con -fdefault-single-float, menos zBWare (configure.py, single_float_gc)
        cflags.append('-fdefault-single-float')
    cflags += incs(COMUNES) + ['-x', 'c++']
else:
    cflags = ['-O2', '-g2', '-Wa,-L', '-Wno-ctor-dtor-privacy']
    cflags += incs(['src/Speed/Indep/Libs/Support/stlps2', 'src/Speed/PSX2/Libs/stl/gpp',
                    'src/Speed/PSX2/bWare/src/ee/include', 'src/Speed/PSX2/bWare/src/ee/gcc/ee/include',
                    'src/Speed/PSX2/bWare/src/ee/gcc/lib/gcc-lib/ee/2.9-ee-991111/include',
                    'src/Speed/PSX2/bWare/src/ee/gcc/ee', 'src/Speed/PSX2/bWare/src/ee/gcc/lib/gcc-lib/ee/2.9-ee-991111',
                    'src/Packages', 'src'])
    cflags += ['-DEA_PLATFORM_PLAYSTATION2'] + (['-DEA_BUILD_A124'] if A124 else []) + ['-D_NOTHREADS', '-D_STL=']
    # Region, como configure.py: la A124 es un prototipo de la europea SLES-53558 y SLUS es la americana.
    cflags += ['-DEA_REGION_EUROPE'] if A124 else ['-DEA_REGION_AMERICA']
    cflags += ['-I', absi('build/%s/include' % V), '-DBUILD_VERSION=%d' % (2 if A124 else 3)]
    cflags += ['-G0', '-ffast-math', '-fno-exceptions', '-fno-rtti', '-DLUA_NUMBER=float']
    # vu0_ps2 de configure.py: desde la ronda 76 todas las unidades de juego de PS2 llevan los 32 registros VU0
    # (decision del usuario); con vf15, que es lo que habia aqui, el banco media otra linea de ordenes que la medida.
    cflags += ['-mvu0-use-vf0-vf31']
    cflags += (['-DMILESTONE_BUILD'] if A124 else []) + ['-DDEFAULT_ALLOCATOR=0', '-DUSEREALCORE']
    cflags += incs(COMUNES) + ['-x', 'c++']

for a in sys.argv[1:]:
    if a.startswith('--sombra='):
        cflags[0:0] = ['-I', os.path.abspath(a.split('=', 1)[1]).replace('\\', '/')]
    elif a.startswith('--extra='):
        # flags propios de la unidad en configure.py (zEcstasy y zFoundation en GC llevan -mps-float, etc.)
        cflags[-2:-2] = a.split('=', 1)[1].split()
for rel in RELS:
    d = absi('src/' + os.path.dirname(rel))
    if d not in cflags:
        cflags[-2:-2] = ['-I', d]

texto = open(os.path.join(RAIZ, 'src', 'Speed', 'Indep', 'SourceLists', SL + '.cpp'), encoding='utf-8').read()
for rel, copia in zip(RELS, COPIAS):
    viejo = '#include "%s"' % rel
    assert texto.count(viejo) == 1, 'la SourceList no incluye %s' % rel
    texto = texto.replace(viejo, '#include "%s"' % copia.replace('\\', '/'))
etiqueta = '+'.join(os.path.splitext(os.path.basename(c))[0] for c in COPIAS)[:80]
# La version va en el nombre: dos banco a la vez con la misma SourceList y la misma copia en dos versiones
# escribian la misma SourceList de prueba y daban errores de compilacion falsos.
# Y el PID: dos banco a la vez con una copia del MISMO nombre en la misma version y unidad se pisaban el objeto y
# daban el mismo resultado para dos variantes distintas (diario ps2a12475). El nombre de la SourceList de prueba no
# cambia el codigo: los _GLOBAL_ de las unidades de juego llevan el nombre del primer simbolo global, no el fichero.
fuente = '%s__%s_%s_%d.cpp' % (SL, etiqueta, V, os.getpid())
open(os.path.join(B, fuente), 'w', encoding='utf-8').write(texto)
obj = os.path.join(B, fuente[:-4] + '.o')
JSONS = []
ASM = None
for a in sys.argv[1:]:
    if a.startswith('--asm='):
        ASM = os.path.abspath(a.split('=', 1)[1])
        cflags[-2:-2] = ['-save-temps']
# -save-temps deja el .s en el directorio del preprocesado (ee_gcc_pp.py compila alli)
S_TEMP = os.path.join(B, 'pp', fuente[:-4] + '.s')


def limpiar():
    """Cada corrida dejaba la SourceList de prueba, su preprocesado (pp/) y el objeto: con decenas de corridas de
    varios agentes llenaron el disco (19,5 GB en build/banco el 2026-09-23 y todas las compilaciones fallando)."""
    if '--conservar' in sys.argv:
        return
    for q in [os.path.join(B, fuente), obj, os.path.join(B, 'pp', fuente), S_TEMP] + JSONS:
        try:
            os.remove(q)
        except OSError:
            pass


if os.path.exists(obj):
    os.remove(obj)
if GC:
    prodg = os.path.join(RAIZ, 'build', 'compilers', 'ProDG', '3.9.3')
    r = subprocess.run([os.path.join(prodg, 'ngccc.exe')] + cflags + ['-c', '-o', obj, fuente], cwd=B,
                       capture_output=True, text=True, env=dict(os.environ, SN_NGC_PATH=prodg))
else:
    cc = os.path.join(RAIZ, 'build', 'compilers', 'PS2', 'ee-gcc2.9-991111', 'bin', 'ee-gcc.exe')
    r = subprocess.run([sys.executable, os.path.join(RAIZ, 'tools', 'ee_gcc_pp.py'), 'pp', cc, '--'] + cflags
                       + ['-c', '-o', obj, fuente], cwd=B, capture_output=True, text=True)
if r.returncode or not os.path.exists(obj):
    print(r.stdout[-3000:], r.stderr[-3000:])
    limpiar()
    sys.exit(1)
if ASM:
    if os.path.exists(S_TEMP):
        os.replace(S_TEMP, ASM)
    else:
        print('--asm: no se genero %s' % S_TEMP)
# zOnline y zFeOverlay: el original tiene su codigo en .over en las tres versiones (configure.py les pone
# section_rename), y objdiff empareja por seccion: sin el cambio de nombre todas sus funciones salian None%.
if SL in ('zOnline', 'zFeOverlay'):
    subprocess.run([sys.executable, os.path.join(RAIZ, 'tools', 'rename_section.py'), '-q', obj, '.text=.over'],
                   check=True)

orig = os.path.join(RAIZ, 'build', V, 'obj', 'Speed', 'Indep', 'SourceLists', SL + '.o')
# Una sola llamada a objdiff para todos los simbolos: sin simbolo, el modo de una pasada saca todas las funciones con
# su porcentaje y sus instrucciones (0,3 s para zDynamics). Antes era una llamada por simbolo, y una unidad entera
# (538 funciones de zWorld2) tardaba mas de diez minutos por corrida. Con la etiqueta de la copia en el nombre: dos
# banco a la vez con copias distintas escribian el mismo JSON.
out = os.path.join(B, 'd_%s_%s_%d.json' % (V, etiqueta[:40], os.getpid()))
JSONS.append(out)
r = subprocess.run([os.path.join(RAIZ, 'objdiff-cli-windows-x86_64.exe'), 'diff', '-1', orig, '-2', obj,
                    '-c', 'function_reloc_diffs=none', '-c', 'ppc.calculatePoolRelocations=false',
                    '-o', out, '--format', 'json'], capture_output=True, text=True)
if r.returncode:
    print('objdiff fallo', r.stderr[-500:])
    limpiar()
    sys.exit(1)
d = json.load(open(out))
IZQ = {}
DER = {}
for s in d['left']['symbols']:
    IZQ.setdefault(s.get('name'), s)
for s in d['right']['symbols']:
    DER.setdefault(s.get('name'), s)
for sim in SIMS:
    a, b = IZQ.get(sim), DER.get(sim)
    if a is None or b is None:
        print('%-50s no esta (%s)' % (sim, 'original' if a is None else 'nuestro'))
        continue
    print('%-50s %s%%  size=%s/%s' % (sim, b.get('match_percent'), a.get('size'), b.get('size')))
    if DIFF:
        LI, RI = a.get('instructions', []), b.get('instructions', [])
        for k in range(max(len(LI), len(RI))):
            ea = LI[k] if k < len(LI) else None
            eb = RI[k] if k < len(RI) else None
            ta = ((ea or {}).get('instruction') or {}).get('formatted', '').strip()
            tb = ((eb or {}).get('instruction') or {}).get('formatted', '').strip()
            ka = (ea or {}).get('diff_kind', '') or ''
            kb = (eb or {}).get('diff_kind', '') or ''
            marca = '>>>' if (ka not in ('', 'DIFF_NONE') or kb not in ('', 'DIFF_NONE')) else '   '
            print('%s %4d %-46s | %-46s' % (marca, k, ta[:46], tb[:46]))

limpiar()
