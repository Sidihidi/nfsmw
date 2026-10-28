#!/usr/bin/env python3
"""inl.py -- inlines que el ORIGINAL expande y nuestra fuente escribe a mano (ronda 86-87, carril PC-A).

    python scripts/inl.py [--informe build/versiones/SPEED_EXE_1_3.json] [--gc build/versiones/GOWE69.json]
                          [--min 80] [--max 100] [--unidades zAI,zFe2] [--gc-todas] [--todos]

Para cada funcion PARCIAL de la version (por defecto PC: parecido >= --min y < --max) cuyo nombre cualificado este al
100 % en GameCube (la fuente ya es buena en GC: lo que falla es la forma que solo ve MSVC), toma los inlines de PRIMER
nivel que el DWARF de GameCube dice que el original expande (scripts/missinline.py, cache del volcado) y busca el
nombre del metodo en el cuerpo de NUESTRA definicion (`Clase::Metodo(...) {` en src/Speed/Indep). Lista los que no
aparecen: candidatos a "el original llama al envoltorio inline y nosotros lo escribimos a mano" (en la r86:
IsCareerRaceComplete, IsImpounded, IsOnlineRace, UMath::Normalize). MSVC 7.1 suele dejar la llamada, GCC lo pliega:
por eso GC casa y PC no.

Es una LISTA DE CANDIDATOS, no un veredicto: el ruido tipico son constructores de plantilla (list<>::list), macros que
el preprocesador expande a otro nombre (SimRandom_Float), accesores con otro nombre en nuestra cabecera y cuerpos que
el original tiene en otra funcion. Cada candidata se confirma en speed.exe (la llamada, o el cuerpo expandido) y con el
mapa de lineas de GC (la linea de cabecera del inline) antes de tocar la fuente.

--gc-todas  no exige GC al 100 % (tambien funciones que en GC no casan o no se emparejan por nombre).
--todos     no filtra el ruido conocido (Get*, operator, constructores de plantilla, Message::).
Columnas: unidad, parecido, tamano, nombre cualificado, inlines que faltan. No escribe nada.
"""
import argparse
import collections
import json
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import missinline as mi  # noqa: E402

CONV = (' __thiscall ', ' __cdecl ', ' __stdcall ', ' __fastcall ')
RUIDO = ('::Get', 'UCrc32::UCrc32', '_GetSize', 'Message::', 'operator', 'Exists')
DEF = re.compile(r'^[^\n;{}]*?\b([A-Za-z_][\w:]*::~?[A-Za-z_]\w*)\s*\([^;{]*\)\s*(const\s*)?(:[^{;]*)?\{', re.M)


def cualificado(f):
    dn = f.get('metadata', {}).get('demangled_name', f['name'])
    q = dn.split('(')[0]
    for k in CONV:
        if k in q:
            q = q.split(k)[1]
    return q.strip()


def estado_gc(ruta):
    gcp = {}
    rep = json.load(open(ruta, encoding='utf-8'))
    for u in rep['units']:
        for f in u.get('functions') or []:
            gcp[cualificado(f)] = f.get('fuzzy_match_percent') or 0
    return gcp


def cuerpos(raiz):
    out = collections.defaultdict(list)
    for d, _, fs in os.walk(raiz):
        for fn in fs:
            if not fn.endswith(('.cpp', '.c', '.h', '.hpp')):
                continue
            p = os.path.join(d, fn)
            try:
                t = open(p, encoding='utf-8', errors='replace').read()
            except OSError:
                continue
            for m in DEF.finditer(t):
                i = m.end() - 1
                prof, j = 0, i
                while j < len(t):
                    if t[j] == '{':
                        prof += 1
                    elif t[j] == '}':
                        prof -= 1
                        if prof == 0:
                            break
                    j += 1
                out[m.group(1)].append((os.path.relpath(p, RAIZ), t[m.start():j + 1]))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--informe', default=os.path.join(RAIZ, 'build', 'versiones', 'SPEED_EXE_1_3.json'))
    ap.add_argument('--gc', default=os.path.join(RAIZ, 'build', 'versiones', 'GOWE69.json'))
    ap.add_argument('--min', type=float, default=80.0)
    ap.add_argument('--max', type=float, default=100.0)
    ap.add_argument('--unidades', default='')
    ap.add_argument('--gc-todas', action='store_true')
    ap.add_argument('--todos', action='store_true')
    a = ap.parse_args()

    dump, _ = mi.load_cache(False)
    por = collections.defaultdict(list)
    for f in dump['funcs']:
        por[f['name']].append(f)
    gcp = estado_gc(a.gc)
    cs = cuerpos(os.path.join(RAIZ, 'src', 'Speed', 'Indep'))
    quiero = set(x for x in a.unidades.split(',') if x)

    rep = json.load(open(a.informe, encoding='utf-8'))
    n = tot = 0
    for u in rep['units']:
        un = u['name'].replace('\\', '/').split('/')[-1]
        if quiero and un not in quiero:
            continue
        for f in u.get('functions') or []:
            p = f.get('fuzzy_match_percent') or 0
            if not (a.min <= p < a.max):
                continue
            q = cualificado(f)
            g = gcp.get(q)
            if not a.gc_todas and g != 100.0:
                continue
            fs, cu = por.get(q, []), cs.get(q, [])
            if len(fs) != 1 or len(cu) != 1:
                continue
            body = cu[0][1]
            faltan = []
            for e in fs[0]['inl']:
                nm, depth = e[0], e[1]
                if depth != 0:
                    continue
                partes = nm.split('::')
                met = partes[-1].split('(')[0]
                if not a.todos:
                    if met.startswith('~') or (len(partes) > 1 and partes[-1] == partes[-2]):
                        continue
                    if any(r in nm for r in RUIDO) or '<' in nm:
                        continue
                if not re.search(r'\b' + re.escape(met) + r'\b', body):
                    faltan.append(nm)
            if faltan:
                n += 1
                tot += int(f.get('size') or 0)
                print('%s\t%6.2f\t%5s\t%s\t%s\t%s' % (un, p, f.get('size'), q, cu[0][0].replace('\\', '/'),
                                                      ', '.join(sorted(set(faltan)))[:300]))
    print('candidatas: %d fn, %d B' % (n, tot), file=sys.stderr)


if __name__ == '__main__':
    main()
