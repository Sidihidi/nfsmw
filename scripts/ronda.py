#!/usr/bin/env python3
"""ronda.py -- banco de VARIANTES de fuente sobre una copia CONGELADA del arbol: compila SourceLists enteras de una o
varias versiones, en paralelo, y compara cada variante con la primera, funcion a funcion.

banco.py prueba una copia contra el arbol VIVO. Con varios agentes tocando src/ a la vez, medir unidades enteras
asi mezcla sus cambios con los tuyos. ronda.py trabaja sobre una copia de src/ y de los objetos originales tomada al
empezar, y cada variante solo lleva los ficheros que cambia.

    python scripts/ronda.py --dir=DIR --congelar [VERSIONES]
        copia src/ en DIR/congelado/src, los objetos originales de las SourceLists (build/V/obj/.../SourceLists) en
        DIR/congelado/obj/V y el informe build/versiones/V.json en DIR/inicio/V.json. VERSIONES por comas; por
        defecto GOWE69,SLES-53558-A124,SLUS-21351. No pisa una copia que ya exista.

    python scripts/ronda.py --dir=DIR NOMBRE VAR[,VAR...] UNIDADES [VERSIONES] [opciones]
        compila cada UNIDAD de cada VERSION con cada VARIANTE, la compara con objdiff contra el objeto original
        congelado y guarda {funcion: [porcentaje, tamano]} en DIR/rondas/NOMBRE/V_UNIDAD_VAR.json (lo ya hecho no se
        repite). Despues resume cada variante contra la PRIMERA: las funciones que llegan al 100 % y las que lo
        pierden, con bytes. Si existe DIR/inicio/V.json, solo cuentan las funciones de ese informe.
        UNIDADES   SourceLists sin extension (zAI,zWorld2), @fichero con ellas, o `todas`
        VERSIONES  por comas; por defecto SLES-53558-A124,SLUS-21351
        --j=N      compilaciones a la vez (4)
        --tc=NOM   toolchain GameCube bajo build/compilers/ProDG (por defecto 3.9.3,
                   el oficial; 3.9.3-src y 3.9.3-v183src son los construidos desde
                   la fuente con tools/sn-gcc/build.sh, para medir builtins nuevos)
        --resumen  no compila: resume lo que ya hay en DIR/rondas/NOMBRE
        --mueve    ademas, las funciones que suben o bajan de porcentaje (todas, o las N primeras con --max=N)
        --full     guarda tambien el JSON entero de objdiff (con las instrucciones) como full_V_UNIDAD_VAR.json
        --keep     guarda los objetos compilados en DIR/rondas/NOMBRE

Una VARIANTE es un directorio DIR/variantes/VAR/ con los ficheros cambiados en su ruta relativa a src/ (por ejemplo
DIR/variantes/X/Speed/Indep/bWare/Inc/bMath.hpp). `base` es la copia congelada sin cambios, y es la referencia
natural (la primera de la lista). Como en banco.py:
  - de cada cabecera cambiada (.h, .hpp, .inl) se copia su DIRECTORIO entero a una sombra
    (DIR/sombras/NOMBRE__VAR/...) que va delante de todo -I, porque los #include relativos no pasan por -I;
  - un .cpp o .c en un directorio con sombra va en la sombra; los demas, la SourceList los incluye por ruta
    absoluta, con -I del directorio original.
Los flags son los de las unidades de juego de configure.py (los mismos que banco.py), con los propios de cada
unidad (-fno-implicit-templates de zFeOverlay, -mps-float de zEcstasy y zFoundation en GameCube) y el cambio de
seccion .text=.over de zOnline y zFeOverlay. Comprobacion: `base` tiene que dar lo mismo que el informe oficial.

Para construir una variante con cambios de texto exactos, desde Python:

    sys.path.insert(0, 'scripts')
    from ronda import variante
    variante(DIR, 'X', [('Speed/Indep/bWare/Inc/bMath.hpp', VIEJO, NUEVO), ...], base=None)

Cada VIEJO tiene que aparecer exactamente una vez; `base` es otra variante de la que partir. Se escribe con LF.
"""
import concurrent.futures as cf
import json
import os
import shutil
import subprocess
import sys
import time

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJDIFF = os.path.join(RAIZ, 'objdiff-cli-windows-x86_64.exe')
SL_REL = os.path.join('Speed', 'Indep', 'SourceLists')
VERSIONES = ['GOWE69', 'SLES-53558-A124', 'SLUS-21351']
PS2 = ['SLES-53558-A124', 'SLUS-21351']
CABECERAS = ('.h', '.hpp', '.inl')
# zOnline y zFeOverlay tienen el codigo en .over en el original (section_rename de configure.py)
OVER = ('zOnline', 'zFeOverlay')
COMUNES = ['src/Speed/Indep/Libs/allocator/1.5.0', 'src/Speed/Indep/Libs/csis/dev/include',
           'src/Packages/eathread/1.1.0/include', 'src/Speed/Indep/Libs/snd/9/include',
           'src/Speed/Indep/Libs/spch/dev/include', 'src/Speed/Indep/Libs/path/5.01.04/include',
           'src/Speed/Indep/Libs/realcore/6.24.00/include/common', 'src/Speed/Indep/Libs/endian/0.5.2/include']


# ---------------------------------------------------------------------------------------------------------- variantes

def congelado(d):
    return os.path.join(d, 'congelado')


def lee(d, ruta, base=None):
    """el texto (LF) de src/RUTA en la variante BASE, o en la copia congelada"""
    if base:
        p = os.path.join(d, 'variantes', base, ruta)
        if os.path.exists(p):
            return open(p, 'rb').read().decode('utf-8').replace('\r\n', '\n')
    return open(os.path.join(congelado(d), 'src', ruta), 'rb').read().decode('utf-8').replace('\r\n', '\n')


def variante(d, nombre, cambios, base=None):
    """crea DIR/variantes/NOMBRE a partir de la copia congelada (o de la variante BASE) con cambios de texto exactos:
    [(ruta relativa a src, VIEJO, NUEVO)]; cada VIEJO tiene que aparecer una sola vez"""
    dst = os.path.join(d, 'variantes', nombre)
    if os.path.exists(dst):
        shutil.rmtree(dst)
    textos = {}
    if base:
        bd = os.path.join(d, 'variantes', base)
        for r, _, fs in os.walk(bd):
            for f in fs:
                rel = os.path.relpath(os.path.join(r, f), bd).replace('\\', '/')
                textos[rel] = lee(d, rel, base)
    for ruta, viejo, nuevo in cambios:
        if ruta not in textos:
            textos[ruta] = lee(d, ruta, base)
        n = textos[ruta].count(viejo)
        assert n == 1, '%s: %d apariciones de %r' % (ruta, n, viejo[:120])
        textos[ruta] = textos[ruta].replace(viejo, nuevo)
    for ruta, t in textos.items():
        p = os.path.join(dst, ruta)
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, 'wb') as fh:
            fh.write(t.encode('utf-8'))
    return dst


def cambios(d, var):
    """rutas relativas a src (con /) de los ficheros de la variante"""
    if var == 'base':
        return []
    vd = os.path.join(d, 'variantes', var)
    assert os.path.isdir(vd), 'no existe la variante %s' % vd
    fuera = []
    for r, _, fs in os.walk(vd):
        for f in fs:
            fuera.append(os.path.relpath(os.path.join(r, f), vd).replace('\\', '/'))
    return sorted(fuera)


def prepara(d, etiqueta, var):
    """(directorio de sombra o None, {ruta relativa de .cpp/.c: copia absoluta}) de una variante"""
    rels = cambios(d, var)
    dirs = sorted({os.path.dirname(r) for r in rels if r.lower().endswith(CABECERAS)})
    sombra = None
    if dirs:
        sombra = os.path.join(d, 'sombras', etiqueta + '__' + var)
        if os.path.exists(sombra):
            shutil.rmtree(sombra)
        for sd in dirs:
            src = os.path.join(congelado(d), 'src', sd)
            dst = os.path.join(sombra, sd)
            os.makedirs(dst, exist_ok=True)
            # solo los ficheros del directorio: los #include relativos de un nivel bastan
            for f in os.listdir(src):
                if os.path.isfile(os.path.join(src, f)):
                    shutil.copyfile(os.path.join(src, f), os.path.join(dst, f))
    copias = {}
    for r in rels:
        orig = os.path.join(d, 'variantes', var, r)
        if os.path.dirname(r) in dirs:
            shutil.copyfile(orig, os.path.join(sombra, r))
        else:
            copias[r] = orig.replace('\\', '/')
    return sombra, copias


# ---------------------------------------------------------------------------------------------------------- compilar

def flags(d, v):
    """los flags de las unidades de juego (cflags_game de configure.py, los de banco.py) sobre la copia congelada"""
    def absc(p):
        return os.path.join(congelado(d), p).replace('\\', '/')

    def incs(lista):
        fuera = []
        for i in lista:
            fuera += ['-I', absc(i)]
        return fuera

    if v == 'GOWE69':
        f = ['-O1', '-gdwarf+', '-Wno-ctor-dtor-privacy', '-Woverloaded-virtual', '-Wno-multichar']
        f += incs(['src/Speed/Indep/Libs/Support/stlgc', 'src/Speed/GameCube/Libs/stl/STLport-4.5/stlport',
                   'src/Speed/GameCube/bWare/GameCube/SN/include', 'src/Packages/eathread/1.1.0/include',
                   'src/Speed/GameCube/bWare/GameCube/dolphinsdk/include', 'src/Packages',
                   'src/Speed/GameCube/bWare/GameCube/SN', 'src'])
        f += ['-DEA_PLATFORM_GAMECUBE', '-DEA_REGION_AMERICA', '-DGEKKO', '-D_USE_MATH_DEFINES',
              '-I', os.path.join(RAIZ, 'build', 'GOWE69', 'include').replace('\\', '/'), '-DBUILD_VERSION=0',
              '-DVERSION_GOWE69', '-DNDEBUG=1',
              '-mps-nodf', '-G0', '-fno-static-dtors', '-ffast-math', '-fforce-addr', '-fcse-follow-jumps',
              '-fcse-skip-blocks', '-fforce-mem', '-fgcse', '-frerun-cse-after-loop', '-fschedule-insns',
              '-fschedule-insns2', '-fexpensive-optimizations', '-frerun-loop-opt', '-fmove-all-movables',
              '-DLUA_NUMBER=float', '-DDISABLE_RAIN', '-DDEFAULT_ALLOCATOR=0', '-DUSEREALCORE',
              # las unidades de juego de GameCube se compilaron con -fkeep-inline-functions (configure.py, keep_inline_gc)
              '-fkeep-inline-functions']
    else:
        a124 = v == 'SLES-53558-A124'
        f = ['-O2', '-g2', '-Wa,-L', '-Wno-ctor-dtor-privacy']
        f += incs(['src/Speed/Indep/Libs/Support/stlps2', 'src/Speed/PSX2/Libs/stl/gpp',
                   'src/Speed/PSX2/bWare/src/ee/include', 'src/Speed/PSX2/bWare/src/ee/gcc/ee/include',
                   'src/Speed/PSX2/bWare/src/ee/gcc/lib/gcc-lib/ee/2.9-ee-991111/include',
                   'src/Speed/PSX2/bWare/src/ee/gcc/ee',
                   'src/Speed/PSX2/bWare/src/ee/gcc/lib/gcc-lib/ee/2.9-ee-991111', 'src/Packages', 'src'])
        f += ['-DEA_PLATFORM_PLAYSTATION2'] + (['-DEA_BUILD_A124'] if a124 else []) + ['-D_NOTHREADS', '-D_STL=']
        f += ['-DEA_REGION_EUROPE'] if a124 else ['-DEA_REGION_AMERICA']
        f += ['-I', os.path.join(RAIZ, 'build', v, 'include').replace('\\', '/')]
        f += ['-DBUILD_VERSION=%d' % (2 if a124 else 3)]
        f += ['-G0', '-ffast-math', '-fno-exceptions', '-fno-rtti', '-DLUA_NUMBER=float', '-mvu0-use-vf0-vf15']
        f += (['-DMILESTONE_BUILD'] if a124 else []) + ['-DDEFAULT_ALLOCATOR=0', '-DUSEREALCORE']
    f += incs(COMUNES) + ['-x', 'c++']
    return f


def extra_unidad(v, sl):
    """los flags propios de la unidad en configure.py"""
    if sl == 'zFeOverlay':
        return ['-fno-implicit-templates'] + (['-fdefault-single-float'] if v == 'GOWE69' else [])
    sf = ['-fdefault-single-float'] if v == 'GOWE69' and sl != 'zBWare' else []  # configure.py, single_float_gc
    if v == 'GOWE69' and sl == 'zFoundation':
        return ['-mps-float', '-fvtable-thunks=1'] + sf
    if v == 'GOWE69' and sl == 'zEcstasy':
        return ['-mps-float'] + sf
    return sf


def compila(d, nombre, v, sl, var, sombra, copias, tc='3.9.3'):
    """(objeto, None) o (None, errores)"""
    obra = os.path.join(d, 'obra')
    os.makedirs(obra, exist_ok=True)
    fuente = '%s__%s__%s__%s.cpp' % (sl, nombre, var, v)
    texto = open(os.path.join(congelado(d), 'src', SL_REL, sl + '.cpp'), encoding='utf-8').read()
    extra = []
    for rel, copia in copias.items():
        viejo = '#include "%s"' % rel
        if texto.count(viejo) != 1:
            continue  # esta SourceList no lo incluye
        texto = texto.replace(viejo, '#include "%s"' % copia)
        extra += ['-I', os.path.join(congelado(d), 'src', os.path.dirname(rel)).replace('\\', '/')]
    open(os.path.join(obra, fuente), 'w', encoding='utf-8').write(texto)
    obj = os.path.join(obra, fuente[:-4] + '.o')
    if os.path.exists(obj):
        os.remove(obj)
    f = flags(d, v)
    if sombra:
        f = ['-I', sombra.replace('\\', '/')] + f
    f[-2:-2] = extra + extra_unidad(v, sl)
    if v == 'GOWE69':
        prodg = os.path.join(RAIZ, 'build', 'compilers', 'ProDG', tc)
        r = subprocess.run([os.path.join(prodg, 'ngccc.exe')] + f + ['-c', '-o', obj, fuente], cwd=obra,
                           capture_output=True, text=True, env=dict(os.environ, SN_NGC_PATH=prodg))
    else:
        cc = os.path.join(RAIZ, 'build', 'compilers', 'PS2', 'ee-gcc2.9-991111', 'bin', 'ee-gcc.exe')
        r = subprocess.run([sys.executable, os.path.join(RAIZ, 'tools', 'ee_gcc_pp.py'), 'pp', cc, '--'] + f
                           + ['-c', '-o', obj, fuente], cwd=obra, capture_output=True, text=True)
    for x in (os.path.join(obra, fuente), os.path.join(obra, 'pp', fuente)):
        try:
            os.remove(x)
        except OSError:
            pass
    if r.returncode or not os.path.exists(obj):
        lineas = (r.stdout + r.stderr).splitlines()
        err = [x for x in lineas if 'warning' not in x and 'In file included' not in x and
               not x.strip().startswith('from ') and 'In function' not in x and 'In method' not in x]
        return None, ' | '.join(err[-25:])[-4000:]
    if sl in OVER:
        subprocess.run([sys.executable, os.path.join(RAIZ, 'tools', 'rename_section.py'), '-q', obj,
                        '.text=.over'], check=True)
    return obj, None


def mide(d, nombre, v, sl, var, prep, full=False, keep=False, tc='3.9.3'):
    ronda = os.path.join(d, 'rondas', nombre)
    out = os.path.join(ronda, '%s_%s_%s.json' % (v, sl, var))
    if os.path.exists(out):
        return 'ya'
    sombra, copias = prep[var]
    obj, err = compila(d, nombre, v, sl, var, sombra, copias, tc)
    if obj is None:
        return 'FALLO ' + err
    orig = os.path.join(congelado(d), 'obj', v, sl + '.o')
    js = os.path.join(ronda, '_d_%s_%s_%s.json' % (v, sl, var))
    r = subprocess.run([OBJDIFF, 'diff', '-1', orig, '-2', obj, '-c', 'function_reloc_diffs=none',
                        '-c', 'ppc.calculatePoolRelocations=false', '-o', js, '--format', 'json'],
                       capture_output=True, text=True)
    if r.returncode:
        return 'objdiff fallo ' + r.stderr[-300:]
    dd = json.load(open(js))
    res = {}
    for s in dd['right']['symbols']:
        nm, mp = s.get('name'), s.get('match_percent')
        if s.get('kind') == 'SYMBOL_FUNCTION' and nm and mp is not None:
            res[nm] = (mp, int(s.get('size', 0) or 0))
    json.dump(res, open(out, 'w'))
    if full:
        os.replace(js, os.path.join(ronda, 'full_%s_%s_%s.json' % (v, sl, var)))
    else:
        os.remove(js)
    if keep:
        os.replace(obj, os.path.join(ronda, '%s_%s_%s.o' % (v, sl, var)))
    else:
        try:
            os.remove(obj)
        except OSError:
            pass
    return 'ok'


# ---------------------------------------------------------------------------------------------------------- resumen

_INF = {}


def funciones_informe(d, v, sl):
    """nombres de las funciones de la unidad en DIR/inicio/V.json, o None si no hay informe"""
    if v not in _INF:
        p = os.path.join(d, 'inicio', v + '.json')
        m = None
        if os.path.exists(p):
            m = {}
            for u in json.load(open(p, encoding='utf-8'))['units']:
                m[u['name'].split('/')[-1]] = {f['name'] for f in (u.get('functions', []) or [])}
        _INF[v] = m
    return None if _INF[v] is None else _INF[v].get(sl, set())


def resumen(d, nombre, variantes, unidades, versiones, mueve=False, maximo=0):
    ronda = os.path.join(d, 'rondas', nombre)
    ref = variantes[0]
    for var in variantes[1:]:
        tot = {}
        detalle = []
        movidas = []
        for v in versiones:
            g = p = bg = bp = 0
            for sl in unidades:
                if not os.path.exists(os.path.join(congelado(d), 'obj', v, sl + '.o')):
                    continue
                a = os.path.join(ronda, '%s_%s_%s.json' % (v, sl, ref))
                b = os.path.join(ronda, '%s_%s_%s.json' % (v, sl, var))
                if not (os.path.exists(a) and os.path.exists(b)):
                    detalle.append('   %s %s: falta' % (v, sl))
                    continue
                r0, r1 = json.load(open(a)), json.load(open(b))
                ofi = funciones_informe(d, v, sl)
                if ofi is not None:
                    r0 = {k: x for k, x in r0.items() if k in ofi}
                    r1 = {k: x for k, x in r1.items() if k in ofi}
                gana = sorted(k for k in r1 if r1[k][0] == 100.0 and r0.get(k, (0,))[0] != 100.0)
                pierde = sorted(k for k in r0 if r0[k][0] == 100.0 and r1.get(k, (0,))[0] != 100.0)
                g += len(gana)
                p += len(pierde)
                bg += sum(r1[k][1] for k in gana)
                bp += sum(r0[k][1] for k in pierde)
                if gana or pierde:
                    detalle.append('   %-16s %-12s +%d -%d  %s%s' % (
                        v[:16], sl, len(gana), len(pierde), ' '.join('+' + k for k in gana),
                        ' ' + ' '.join('-%s(%.2f)' % (k, r1.get(k, (0,))[0]) for k in pierde)))
                if mueve:
                    for k in set(r0) & set(r1):
                        dif = r1[k][0] - r0[k][0]
                        if abs(dif) > 1e-6:
                            movidas.append((v, dif, sl, k, r0[k][0], r1[k][0], r1[k][1]))
            tot[v] = (g, p, bg, bp)
        print('%-22s vs %-10s %s' % (var, ref, '  '.join('%s: +%d -%d (+%d B -%d B)' % (v[:9], g, p, bg, bp)
                                                          for v, (g, p, bg, bp) in tot.items())))
        for ln in detalle:
            print(ln)
        if mueve:
            for v in versiones:
                suben = sorted((x for x in movidas if x[0] == v and x[1] > 0), key=lambda x: -x[1])
                bajan = sorted((x for x in movidas if x[0] == v and x[1] < 0), key=lambda x: x[1])
                print('   == %s: suben %d, bajan %d' % (v, len(suben), len(bajan)))
                for etq, lista in (('baja', bajan), ('sube', suben)):
                    for x in (lista[:maximo] if maximo else lista):
                        print('      %s %8.3f  %-16s %-70s %7.3f -> %7.3f (%d B)' % (etq, x[1], x[2], x[3], x[4],
                                                                                   x[5], x[6]))


# ---------------------------------------------------------------------------------------------------------- congelar

def congelar(d, versiones):
    cg = congelado(d)
    if os.path.exists(os.path.join(cg, 'src')):
        print('ya hay una copia congelada en', cg, '(borrala a mano para tomar otra)')
        return 1
    os.makedirs(cg, exist_ok=True)
    shutil.copytree(os.path.join(RAIZ, 'src'), os.path.join(cg, 'src'))
    for v in versiones:
        dst = os.path.join(cg, 'obj', v)
        os.makedirs(dst, exist_ok=True)
        sld = os.path.join(RAIZ, 'build', v, 'obj', SL_REL)
        for f in sorted(os.listdir(sld)):
            if f.endswith('.o'):
                shutil.copyfile(os.path.join(sld, f), os.path.join(dst, f))
        inf = os.path.join(RAIZ, 'build', 'versiones', v + '.json')
        if os.path.exists(inf):
            os.makedirs(os.path.join(d, 'inicio'), exist_ok=True)
            shutil.copyfile(inf, os.path.join(d, 'inicio', v + '.json'))
    head = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=RAIZ, capture_output=True, text=True).stdout.strip()
    sucio = subprocess.run(['git', 'status', '--short', 'src'], cwd=RAIZ, capture_output=True, text=True).stdout
    with open(os.path.join(cg, 'HEAD.txt'), 'w', encoding='utf-8') as fh:
        fh.write(head + '\n' + sucio)
    print('congelado %s (HEAD %s%s) con %s' % (cg, head[:8], ', src con cambios' if sucio.strip() else '',
                                              ', '.join(versiones)))
    return 0


def main():
    opts = [a for a in sys.argv[1:] if a.startswith('--')]
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    d = None
    j, maximo = 4, 0
    tc = '3.9.3'
    for a in opts:
        if a.startswith('--dir='):
            d = os.path.abspath(a.split('=', 1)[1])
        elif a.startswith('--j='):
            j = int(a[4:])
        elif a.startswith('--max='):
            maximo = int(a[6:])
        elif a.startswith('--tc='):
            tc = a[5:]
    if d is None:
        print(__doc__)
        return 2
    if '--congelar' in opts:
        return congelar(d, args[0].split(',') if args else VERSIONES)
    if len(args) < 3:
        print(__doc__)
        return 2
    nombre, variantes = args[0], args[1].split(',')
    if args[2] == 'todas':
        unidades = sorted({f[:-2] for v in VERSIONES if os.path.isdir(os.path.join(congelado(d), 'obj', v))
                           for f in os.listdir(os.path.join(congelado(d), 'obj', v)) if f.endswith('.o')})
    elif args[2].startswith('@'):
        unidades = [u for u in open(args[2][1:], encoding='utf-8').read().replace(',', ' ').split() if u]
    else:
        unidades = args[2].split(',')
    versiones = args[3].split(',') if len(args) > 3 else PS2
    os.makedirs(os.path.join(d, 'rondas', nombre), exist_ok=True)
    if '--resumen' not in opts:
        prep = {var: prepara(d, nombre, var) for var in variantes}
        t0 = time.time()
        trabajos = [(v, sl, var) for sl in unidades for v in versiones for var in variantes
                    if os.path.exists(os.path.join(congelado(d), 'obj', v, sl + '.o'))]
        with cf.ThreadPoolExecutor(j) as ex:
            futuros = {ex.submit(mide, d, nombre, v, sl, var, prep, '--full' in opts, '--keep' in opts, tc): (v, sl, var)
                       for v, sl, var in trabajos}
            for fu in cf.as_completed(futuros):
                st = fu.result()
                if st not in ('ok', 'ya'):
                    print('%s %s %s: %s' % (futuros[fu] + (st,)), flush=True)
        print('compilado en %.0f s' % (time.time() - t0), flush=True)
        # las sombras solo hacen falta mientras se compila
        for var in variantes:
            s = os.path.join(d, 'sombras', nombre + '__' + var)
            if os.path.isdir(s):
                shutil.rmtree(s)
    resumen(d, nombre, variantes, unidades, versiones, '--mueve' in opts, maximo)
    return 0


if __name__ == '__main__':
    sys.exit(main())
