#!/usr/bin/env python3
"""informe_prueba.py -- informe de objdiff de un troceado hecho con scripts/trocea_prueba.py: objetivo = DIR/obj, base =
nuestros objetos de build/<V>/src (o de --base DIRBASE, con la misma forma), todas las unidades `complete: false` y las
mismas opciones que scripts/pctall.py (functionRelocDiffs=none, ppc.calculatePoolRelocations=false).

    python scripts/informe_prueba.py V DIR [--base DIRBASE] [--extra-base DIREXTRA]

--extra-base: un directorio con objetos nuestros para las unidades que el arbol aun no compila para V (se usa si el
de --base no existe). En la tanda 6 las unidades de realcore nuevas de SLUS se probaron con los objetos de la A124
(build/SLES-53558-A124/src), que en esa biblioteca son identicos byte a byte a los de SLUS.
Deja DIR/objdiff.json y DIR/report.json e imprime la linea de medida. Sin los mapeos de sufijo de configure.py: para
comparar, usar dos informes hechos con esta misma herramienta (scripts/compara_informes.py).
"""
import json
import os
import subprocess
import sys
from pathlib import PurePosixPath

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJDIFF = os.path.join(RAIZ, 'build', 'tools', 'objdiff-cli.exe')


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    V, D = args[0], os.path.abspath(args[1])

    def opt(n):
        return sys.argv[sys.argv.index(n) + 1] if n in sys.argv else None
    base = os.path.abspath(opt('--base')) if opt('--base') else os.path.join(RAIZ, 'build', V, 'src')
    extra = os.path.abspath(opt('--extra-base')) if opt('--extra-base') else None
    unidades = json.load(open(os.path.join(D, 'unidades.json')))
    asm = os.path.join(D, 'asm').replace('\\', '/')
    units = []
    sin_base = 0
    for s in unidades:
        s = s.replace('\\', '/')
        rel = s[s.index('/asm/') + 5:] if '/asm/' in s else os.path.relpath(s, asm).replace('\\', '/')
        name = rel.removesuffix('.s')                      # como config.json: Speed/.../zAI.cpp
        objrel = rel.removesuffix('.s').removesuffix('.cpp').removesuffix('.c') + '.o'
        u = {'name': 'main/' + str(PurePosixPath(name).with_suffix('')),
             # objdiff estropea las rutas absolutas de Windows ("C:Users\..."): todo relativo al proyecto
             'target_path': os.path.relpath(os.path.join(D, 'obj', objrel), D).replace(os.sep, '/'),
             'metadata': {'complete': False}}
        srcobj = str(PurePosixPath(name).with_suffix('.o'))
        for b in (base, extra):
            if b and os.path.exists(os.path.join(b, srcobj)):
                u['base_path'] = os.path.relpath(os.path.join(b, srcobj), D).replace(os.sep, '/')
                break
        else:
            sin_base += 1
        units.append(u)
    cfg = {'min_version': '2.0.0-beta.5', 'build_target': False, 'units': units, 'progress_categories': []}
    json.dump(cfg, open(os.path.join(D, 'objdiff.json'), 'w', encoding='utf-8'), indent=1)
    rep = os.path.join(D, 'report.json')
    if os.path.exists(rep):
        os.remove(rep)
    r = subprocess.run([OBJDIFF, 'report', 'generate', '-p', D, '-o', rep, '-c', 'functionRelocDiffs=none',
                        '-c', 'ppc.calculatePoolRelocations=false'], capture_output=True, text=True, errors='replace')
    if not os.path.exists(rep):
        print('SIN INFORME', r.stdout[-2000:], r.stderr[-2000:])
        sys.exit(1)
    m = json.load(open(rep))['measures']
    print('%s %.4f %% codigo (%s/%s B), %s/%s funciones; %d unidades, %d sin base' % (
        V, m.get('matched_code_percent', 0), m.get('matched_code'), m.get('total_code'), m.get('matched_functions'),
        m.get('total_functions'), len(units), sin_base))


if __name__ == '__main__':
    main()
