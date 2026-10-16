#!/usr/bin/env python3
"""gcmismo.py <TAG> <unidad> [...] -- compila la SourceList de GameCube desde el arbol de trabajo (ngccc con los
cflags de build.ninja, solo lectura) a build/gcmismo/<unidad><TAG>.o y la compara con el objeto de
build/GOWE69/src/.../<unidad>.o, que tiene que ser el de HEAD (el ultimo ninja de GameCube): toda funcion que no
salga identica (bytes Y reubicaciones) se lista, y sale con codigo 1 si hay alguna.

Para que sirve: hay formas de fuente que GameCube no distingue y PS2 si (`x >= c` frente a `x < c`, la orientacion
de un if). La A124 dice cual era la del original; esto exige que GameCube no cambie al escribirla. build.ninja tiene
que estar configurado para GOWE69 (lo esta salvo mientras corre scripts/pctall.py).

    python scripts/gcmismo.py _yo zCamera zAI
"""
import json
import os
import re
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'build', 'gcmismo')


def reglas(ninja, unit):
    head = 'build build\\GOWE69\\src\\Speed\\Indep\\SourceLists\\%s.o: ' % unit
    i = ninja.find(head)
    if i < 0:
        sys.exit('sin regla para ' + unit)
    rest = ninja[i + len(head):]
    rule = rest.split()[0]
    vars_ = {}
    for l in rest.split('\n')[1:]:
        m = re.match(r'  (\w+) = (.*)$', l)
        if not m:
            break
        vars_[m.group(1)] = m.group(2)
    return rule, vars_


def uno(ninja, tag, u):
    rule, v = reglas(ninja, u)
    tv = v['toolchain_version']
    out = os.path.join(OUT, u + tag + '.o')
    if os.path.exists(out):
        os.remove(out)
    env = dict(os.environ)
    env['SN_NGC_PATH'] = os.path.join(ROOT, 'build', 'compilers', tv)
    args = [os.path.join(ROOT, 'build', 'compilers', tv, 'ngccc.exe')] + shlex.split(v['cflags'], posix=True) + \
        ['-c', '-o', out, os.path.normpath('src/Speed/Indep/SourceLists/%s.cpp' % u)]
    r = subprocess.run(args, cwd=ROOT, env=env, capture_output=True, text=True, errors='replace')
    if rule == 'prodg_rename' and os.path.exists(out) and 'section_rename' in v:
        subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'rename_section.py'), '-q', out] +
                       v['section_rename'].split(), cwd=ROOT)
    if not os.path.exists(out):
        return u, None, (r.stderr or r.stdout)[-800:]
    base = os.path.join(ROOT, 'build', 'GOWE69', 'src', 'Speed', 'Indep', 'SourceLists', u + '.o')
    js = os.path.join(OUT, u + tag + '.json')
    subprocess.run([os.path.join(ROOT, 'objdiff-cli-windows-x86_64.exe'), 'diff', '-1', base, '-2', out, '-o', js,
                    '--format', 'json'], capture_output=True, text=True, cwd=ROOT)
    d = json.load(open(js))
    os.remove(js)
    malos = []
    R = d['right']['symbols']
    for s in d['left']['symbols']:
        if s.get('kind') != 'SYMBOL_FUNCTION':
            continue
        ti = s.get('target_symbol')
        o = R[ti] if isinstance(ti, int) and ti < len(R) else None
        if o is None:
            malos.append((s['name'], 'desaparece'))
        elif s.get('match_percent') != 100.0 or s.get('size') != o.get('size'):
            malos.append((s['name'], '%s%% %s->%s B' % (s.get('match_percent'), s.get('size'), o.get('size'))))
    return u, malos, ''


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    tag = sys.argv[1]
    os.makedirs(OUT, exist_ok=True)
    ninja = open(os.path.join(ROOT, 'build.ninja'), errors='replace').read().replace('$\n', '')
    if 'config\\GOWE69\\' not in ninja[:4000]:
        sys.exit('build.ninja no esta configurado para GOWE69 (pctall en marcha?): espera y reintenta')
    fallo = False
    with ThreadPoolExecutor(3) as ex:
        for u, malos, err in ex.map(lambda u: uno(ninja, tag, u), sys.argv[2:]):
            if malos is None:
                print('%-18s NO COMPILA EN GAMECUBE\n%s' % (u, err))
                fallo = True
            elif malos:
                fallo = True
                print('%-18s %d funciones DISTINTAS en GameCube:' % (u, len(malos)))
                for n, why in malos[:30]:
                    print('    %-70s %s' % (n[:70], why))
            else:
                print('%-18s GameCube identico' % u)
    sys.exit(1 if fallo else 0)


if __name__ == '__main__':
    main()
