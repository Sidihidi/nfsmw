#!/usr/bin/env python3
"""rap.py UNIDAD COPIA SIM[,SIM...] [opciones]  -- banco RAPIDO de GameCube de un solo fichero.

Compila SOLO la copia de un .cpp (precedida del prefijo de la SourceList: los #include que no son .cpp) con los flags
de juego de GameCube, y compara los simbolos con el objeto original de build/GOWE69/obj. Tarda 1-3 s por variante en
vez de los 30-60 de banco.py con la SourceList entera; lo que salga bien se confirma luego con banco.py, que es la
referencia (el contexto de la unidad entera puede cambiar plantillas y literales).

    UNIDAD   la SourceList sin extension (zEAXSound2, zWorld...)
    COPIA    la copia modificada del .cpp (en cualquier sitio)
    SIM      simbolos separados por comas

    --dir=DIR       directorio del fichero original, para sus #include relativos (va con -I)
    --prev=REL      en vez del prefijo, TODO lo que la SourceList incluye antes de REL (como en la unidad real)
    --antes=A,B     #include extra antes de la copia
    --sinprefijo    sin prefijo de la SourceList
    --rcmp          flags de rcmp (-O2, -mps-float, sin -fforce-addr...)
    --extra="..."   flags extra
    --d="-dg ..."   no compara: preprocesa y deja los volcados RTL de cc1plus (-dg, -dl, -dS...) en el directorio de
                    trabajo
    --orig=OBJ      otro objeto original
    --ninja=REL     los flags de build.ninja de src/REL (middleware): sin prefijo y contra build/GOWE69/obj/REL.o
    --diff [--solo] instrucciones original | nuestra (--solo: solo las filas distintas)

Trabaja en %TEMP%/rap (o en RAP_DIR): nunca deja temporales junto a la fuente. zEcstasy y zFoundation llevan
-mps-float como en configure.py; zOnline y zFeOverlay renombran .text a .over como configure.py. Para el middleware y
cualquier unidad con flags propios, --ninja toma los de build.ninja. Comprobacion: la copia sin cambios tiene que dar el
porcentaje del informe.

Ejemplo (variantes en un bucle):
    python scripts/rap.py zTrack copia.cpp CalculateRegionInfo__11RegionQueryP5eView10RegionTypei
        --dir=src/Speed/Indep/Src/World --prev=Speed/Indep/Src/World/WeatherMan.cpp --diff --solo
"""
import json
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = os.environ.get('RAP_DIR') or os.path.join(tempfile.gettempdir(), 'rap')
TOOLS = os.path.join(ROOT, 'build', 'compilers', 'ProDG', '3.9.3')
OBJDIFF = os.path.join(ROOT, 'objdiff-cli-windows-x86_64.exe')

# los flags de juego de GameCube de banco.py (no dependen de build.ninja, que otros regeneran)
COMUNES = ['src/Speed/Indep/Libs/allocator/1.5.0', 'src/Speed/Indep/Libs/csis/dev/include',
           'src/Packages/eathread/1.1.0/include', 'src/Speed/Indep/Libs/snd/9/include',
           'src/Speed/Indep/Libs/spch/dev/include', 'src/Speed/Indep/Libs/path/5.01.04/include',
           'src/Speed/Indep/Libs/realcore/6.24.00/include/common', 'src/Speed/Indep/Libs/endian/0.5.2/include']
PLATAFORMA = ['src/Speed/Indep/Libs/Support/stlgc', 'src/Speed/GameCube/Libs/stl/STLport-4.5/stlport',
              'src/Speed/GameCube/bWare/GameCube/SN/include', 'src/Packages/eathread/1.1.0/include',
              'src/Speed/GameCube/bWare/GameCube/dolphinsdk/include', 'src/Packages',
              'src/Speed/GameCube/bWare/GameCube/SN', 'src']
JUEGO = ['-DEA_PLATFORM_GAMECUBE', '-DEA_REGION_AMERICA', '-DGEKKO', '-D_USE_MATH_DEFINES',
         '-DBUILD_VERSION=0', '-DVERSION_GOWE69', '-DNDEBUG=1',
         '-mps-nodf', '-G0', '-fno-static-dtors', '-ffast-math', '-fforce-addr', '-fcse-follow-jumps',
         '-fcse-skip-blocks', '-fforce-mem', '-fgcse', '-frerun-cse-after-loop', '-fschedule-insns',
         '-fschedule-insns2', '-fexpensive-optimizations', '-frerun-loop-opt', '-fmove-all-movables',
         '-DLUA_NUMBER=float', '-DDISABLE_RAIN', '-DDEFAULT_ALLOCATOR=0', '-DUSEREALCORE',
         # las unidades de juego de GameCube se compilaron con -fkeep-inline-functions (configure.py)
         '-fkeep-inline-functions']
SIN_RCMP = ('-O1', '-mps-nodf', '-fforce-addr', '-fno-static-dtors', '-G0', '-fmove-all-movables',
            '-fkeep-inline-functions')


# las unidades de overlay: configure.py renombra su .text a .over (section_rename) y objdiff solo empareja
# simbolos de secciones con el mismo nombre
SOBRE_OVER = ('zOnline', 'zFeOverlay')


def sobre_over(obj):
    """Renombra .text a .over en la tabla de nombres de secciones (mismo largo: se parchea en sitio)."""
    from elftools.elf.elffile import ELFFile
    with open(obj, 'rb') as f:
        e = ELFFile(f)
        sh = e.get_section(e['e_shstrndx'])
        ini, fin = sh['sh_offset'], sh['sh_offset'] + sh['sh_size']
    d = bytearray(open(obj, 'rb').read())
    k = d.find(b'\x00.text\x00', ini - 1, fin)
    if k >= 0:
        d[k + 1:k + 6] = b'.over'
        open(obj, 'wb').write(bytes(d))


def flags_ninja(rel):
    """Los cflags de GameCube de un objeto tal cual los escribe configure.py en build.ninja (para el middleware y
    las unidades con flags propios). REL es la ruta del fuente relativa a src/."""
    clave = 'build build\\GOWE69\\src\\' + os.path.splitext(rel)[0].replace('/', '\\') + '.o:'
    lineas = open(os.path.join(ROOT, 'build.ninja'), encoding='utf-8').read().split('\n')
    for i, ln in enumerate(lineas):
        if ln.startswith(clave):
            for j in range(i, i + 12):
                if lineas[j].strip().startswith('cflags ='):
                    t = lineas[j].split('=', 1)[1]
                    k = j
                    while t.rstrip().endswith('$'):
                        k += 1
                        t = t.rstrip()[:-1] + ' ' + lineas[k].strip()
                    return [absi(x) if x.startswith(('src/', 'build/')) else x for x in t.split()]
    raise SystemExit('no encuentro %s en build.ninja' % clave)


def absi(p):
    return os.path.join(ROOT, p).replace('\\', '/')


def incs(lista):
    fuera = []
    for i in lista:
        fuera += ['-I', absi(i)]
    return fuera


def opcion(nombre):
    for x in sys.argv[1:]:
        if x.startswith(nombre + '='):
            return x.split('=', 1)[1]
    return None


def prefijo(unit):
    sl = open(os.path.join(ROOT, 'src', 'Speed', 'Indep', 'SourceLists', unit + '.cpp'),
              encoding='utf-8').read().split('\n')
    rel = opcion('--prev')
    pre = []
    if rel and not any(rel in ln for ln in sl):
        raise SystemExit('--prev=%s no esta en la SourceList %s' % (rel, unit))
    if rel:
        prof = 0
        for ln in sl:
            if rel in ln:
                break
            pre.append(ln)
            t = ln.strip()
            if t.startswith('#if'):
                prof += 1
            elif t.startswith('#endif'):
                prof -= 1
        pre += ['#endif'] * prof
    else:
        for ln in sl:
            if ln.startswith('#include') and ln.rstrip().endswith('.cpp"'):
                break
            pre.append(ln)
    if opcion('--antes'):
        pre += ['#include "%s"' % y for y in opcion('--antes').split(',')]
    if '--sinprefijo' in sys.argv:
        pre = []
    return pre


def diff(l, r):
    li, ri = l.get('instructions', []), r.get('instructions', [])
    for k in range(max(len(li), len(ri))):
        ea = li[k] if k < len(li) else None
        eb = ri[k] if k < len(ri) else None
        ta = ((ea or {}).get('instruction') or {}).get('formatted', '').strip()
        tb = ((eb or {}).get('instruction') or {}).get('formatted', '').strip()
        ka = (ea or {}).get('diff_kind', '') or ''
        kb = (eb or {}).get('diff_kind', '') or ''
        m = '>>>' if (ka not in ('', 'DIFF_NONE') or kb not in ('', 'DIFF_NONE')) else '   '
        if '--solo' in sys.argv and m == '   ':
            continue
        print('%s %4d %-46s | %-46s' % (m, k, ta[:46], tb[:46]))


def main():
    a = [x for x in sys.argv[1:] if not x.startswith('--')]
    if len(a) < 3:
        raise SystemExit(__doc__)
    unit, copia, sims = a[0], os.path.abspath(a[1]), a[2].split(',')
    os.makedirs(W, exist_ok=True)
    cflags = ['-O1', '-gdwarf+', '-Wno-ctor-dtor-privacy', '-Woverloaded-virtual', '-Wno-multichar']
    cflags += incs(PLATAFORMA) + JUEGO + ['-I', absi('build/GOWE69/include')] + incs(COMUNES)
    if '--rcmp' in sys.argv:
        cflags = [c for c in cflags if c not in SIN_RCMP] + ['-O2', '-mps-float', '-fno-force-addr',
                                                             '-fno-implement-inlines', '-fno-fast-math']
    if unit in ('zEcstasy', 'zFoundation'):
        cflags.append('-mps-float')
    if unit != 'zBWare' and '--rcmp' not in sys.argv:
        cflags.append('-fdefault-single-float')  # configure.py, single_float_gc
    if opcion('--ninja'):
        cflags = flags_ninja(opcion('--ninja'))
    extra = (opcion('--extra') or '').split()
    dumps = (opcion('--d') or '').split()
    inc = ['-I', opcion('--dir')] if opcion('--dir') else []
    env = dict(os.environ, SN_NGC_PATH=TOOLS)
    tag = os.path.splitext(os.path.basename(copia))[0]
    envol = os.path.join(W, tag + '_w.cpp')
    with open(envol, 'w', encoding='utf-8') as f:
        pre = [] if opcion('--ninja') else prefijo(unit)
        f.write('\n'.join(pre) + '\n#include "%s"\n' % copia.replace('\\', '/'))
    obj = os.path.join(W, tag + '.o')
    if os.path.exists(obj):
        os.remove(obj)
    ngccc = os.path.join(TOOLS, 'ngccc.exe')
    if dumps:
        ii = os.path.join(W, tag + '.i')
        p = subprocess.run([ngccc] + inc + cflags + extra + ['-E', '-o', ii, envol], env=env,
                           capture_output=True, text=True)
        if p.returncode:
            raise SystemExit(p.stderr[-3000:])
        keep = [f for f in cflags + extra if re.match(r'^-(O|f|m|G|g|W|std=|ansi|pedantic)', f)]
        p = subprocess.run([os.path.join(TOOLS, 'cc1plus.exe')] + keep + dumps +
                           ['-quiet', '-o', os.path.join(W, tag + '.s'), ii],
                           cwd=W, env=env, capture_output=True, text=True)
        print(p.stderr[-2000:])
        print('volcados en', W)
        return
    p = subprocess.run([ngccc] + inc + cflags + extra + ['-c', '-o', obj, envol], env=env,
                       capture_output=True, text=True)
    if p.returncode or not os.path.exists(obj):
        raise SystemExit(p.stdout[-3000:] + p.stderr[-3000:])
    if unit in SOBRE_OVER:
        sobre_over(obj)
    if opcion('--ninja') and not opcion('--orig'):
        sys.argv.append('--orig=' + os.path.join(ROOT, 'build', 'GOWE69', 'obj',
                                                 os.path.splitext(opcion('--ninja'))[0] + '.o'))
    orig = opcion('--orig') or os.path.join(ROOT, 'build', 'GOWE69', 'obj', 'Speed', 'Indep', 'SourceLists',
                                            unit + '.o')
    out = os.path.join(W, tag + '.json')
    subprocess.run([OBJDIFF, 'diff', '-1', orig, '-2', obj, '-c', 'function_reloc_diffs=none',
                    '-c', 'ppc.calculatePoolRelocations=false', '-o', out, '--format', 'json'],
                   capture_output=True, text=True)
    d = json.load(open(out))
    # con un mismo nombre puede haber una referencia sin cuerpo: manda la que trae instrucciones
    izq, der = {}, {}
    for lado, dic in (('left', izq), ('right', der)):
        for s in d[lado]['symbols']:
            n = s.get('name')
            if n not in dic or (s.get('instructions') and not dic[n].get('instructions')):
                dic[n] = s
    for sim in sims:
        l, r = izq.get(sim), der.get(sim)
        if l is None or r is None:
            print(sim, 'no esta', l is None, r is None)
            continue
        print('%-60s %s%% size=%s/%s' % (sim[:60], r.get('match_percent'), l.get('size'), r.get('size')))
        if '--diff' in sys.argv:
            diff(l, r)
    for x in (out, obj, envol):
        if os.path.exists(x):
            os.remove(x)


if __name__ == '__main__':
    main()
