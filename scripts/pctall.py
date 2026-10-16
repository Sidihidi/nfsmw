#!/usr/bin/env python3
"""pctall.py [--solo-informe] [VERSION ...] -- el porcentaje de TODAS las versiones, no solo de GameCube.

Es la medida del metodo de upstream: un informe de objdiff por version, con codigo real que compile en
todas. `ninja progress` y `scripts/estado.py` solo leen `build/GOWE69/report.json`, y eso deja sin
comprobar lo unico que distingue codigo real de un match apoyado en andamios -- que la MISMA fuente
compile tambien con el compilador de otra plataforma.

    python scripts/pctall.py                     # las cinco versiones: configura, compila y mide
    python scripts/pctall.py --solo-informe      # no compila: mide los objetos que ya haya
    python scripts/pctall.py SLUS-21351          # una sola

Para cada version: `configure.py --version V`, `ninja -k 0` (sigue aunque falle alguna unidad), informe de
objdiff con las opciones del proyecto, y una linea con codigo casado, funciones y CUANTOS OBJETOS NO
COMPILAN. Al terminar deja configurada GOWE69 otra vez. Salida: `build/versiones/<V>.json` (informe) y
`<V>.log` (build).

LA CIFRA NO VALE SI NO ES "0 objetos no compilan". Tres trampas medidas, todas tapadas aqui:

  1. objdiff cuenta al 100 % una unidad marcada `complete` SIN compararla: la que no tiene objeto base y
     tambien la que lo tiene y no casa. `complete` sale del flag Matching de configure.py, que es un hecho
     de GameCube (objeto identico al byte); en PS2 contaba como casadas 124 funciones de la A124 y 261 de
     SLUS que no lo estaban (sndfxbus.o de PS2 ni siquiera define dos de ellas). Aqui TODAS las unidades
     pasan a `complete: false`, como en upstream, donde todas son NonMatching; y la que no tiene objeto o
     es de un build que fallo pierde ademas `base_path`.
  2. Si el TROCEADO falla, `configure.py` no reescribe `objdiff.json` y queda el de la version anterior:
     se acaba midiendo una version como si fuera otra. Aqui se comprueba que las rutas del objdiff.json
     son de esta version antes de medir.
  3. `configure.py --version V` puede terminar OK y no emitir ni una regla de compilacion (si falta
     `build/<V>/config.json`, `build_config` queda en None y no se genera nada, sin aviso).

En PC se mide una copia sin las etiquetas `$L` de MSVC de los objetos que las tienen dentro de una funcion estatica
(scripts/quita_L.py; objdiff cortaba ahi la funcion): build/SPEED_EXE_1_3/sin_L/. El objeto del build no se toca.
En esa copia los funclets de EH (`$L` de las .text$x) llevan su nombre posicional `Unwind@<funcion>$<i>`, el de
symbols.json, y sus mapeos (y los de `_$E<n>` y `__ehhandler$`, que configure.py pudo retirar al resolverlos contra el
objeto de ANTES de compilar) se anaden aqui, antes de resolver los sufijos (quita_L.mapeos_funclets; ronda 50).

Xbox 360 (EUROPEGERMILESTONE) se mide sobre un ENLACE LTCG, como se hizo el original (su Rich header: 369 objetos
`Utc1400_LTCG_CPP`): scripts/ltcg360.py recompila las unidades con /GL, enlaza con /LTCG y vuelve a trocear el
ejecutable en un objeto por unidad (build/EUROPEGERMILESTONE/ltcg/obj/). Ese es el informe oficial,
`build/versiones/EUROPEGERMILESTONE.json`. La medida fichero a fichero de antes queda como diagnostico en
`EUROPEGERMILESTONE-fichero.json` (otra linea en la salida). Si el enlace LTCG falla, la version sale como fallida:
no se cae en silencio a la medida fichero a fichero. En el informe LTCG los dos lados van normalizados (decision del
usuario, 2026-10-07): cargas del pool de constantes por VALOR y tablas de saltos por DESTINO; el original
normalizado esta en build/EUROPEGERMILESTONE/ltcg/orig/ (ltcg360.Normalizacion).

Y dos detalles de entorno: Xbox 360 trocea con `jeff.exe` (la variante de dtk para .xex), no con dtk; y
`cl.exe` da C1083 ("no abre una cabecera que existe") con mucha carga en paralelo, asi que ninja se
relanza hasta tres veces y solo recompila lo que fallo.
"""
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'tools'))
from sufijo_local import automatico as sufijos_automaticos, resolver as resolver_sufijos  # noqa: E402
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import quita_L  # noqa: E402
import ltcg360  # noqa: E402

# Se ejecuta desde la raiz del repo (o de un worktree); la salida va a <repo>/build/versiones/.
ROOT = os.getcwd().replace(chr(92), '/') + '/'
S = ROOT + 'build/versiones/'
TODAS = ['GOWE69', 'SLES-53558-A124', 'SLUS-21351', 'EUROPEGERMILESTONE', 'SPEED_EXE_1_3']


def sin_etiquetas(u):
    """PC: objdiff corta una funcion ESTATICA de COFF en la primera etiqueta `$L` que MSVC deja en la tabla de simbolos
    (el exe troceado no las tiene). Si el objeto nuestro tiene etiquetas dentro de una funcion estatica, se mide una
    copia equivalente sin ellas (scripts/quita_L.py) en build/SPEED_EXE_1_3/sin_L/; el objeto del build no se toca.
    En la misma copia, los funclets de EH (`$L` de las .text$x) con su nombre posicional (quita_L.para_medir)."""
    bp = u['base_path'].replace(chr(92), '/')
    datos = open(os.path.join(ROOT, bp), 'rb').read()
    try:
        r = quita_L.para_medir(datos)
    except quita_L.NoSe as e:
        print('quita_L: %s se mide con etiquetas (%s)' % (bp, e))
        return
    if r is None:
        return
    copia = bp.replace('build/SPEED_EXE_1_3/src/', 'build/SPEED_EXE_1_3/sin_L/', 1)
    if copia == bp:
        return
    os.makedirs(os.path.dirname(os.path.join(ROOT, copia)), exist_ok=True)
    open(os.path.join(ROOT, copia), 'wb').write(r[0])
    u['base_path'] = copia

if any(a in ('-h', '--help') for a in sys.argv[1:]):
    print(__doc__)
    sys.exit(0)

os.makedirs(S, exist_ok=True)
args = [a for a in sys.argv[1:] if not a.startswith('--')]
solo_informe = '--solo-informe' in sys.argv
versiones = args or TODAS


def corre(cmd, log=None):
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, errors='replace')
    if log:
        open(log, 'w', encoding='utf-8').write(r.stdout + r.stderr)
    return r


def herramientas(v):
    """Rutas de las herramientas ya presentes, para que ninja no las vuelva a descargar. Xbox 360 trocea con
    jeff (la variante de dtk para xex), no con dtk."""
    out = []
    dtk = 'jeff.exe' if v == 'EUROPEGERMILESTONE' else 'dtk.exe'
    for opcion, fichero in (('--dtk', dtk), ('--objdiff', 'objdiff-cli.exe'), ('--sjiswrap', 'sjiswrap.exe'),
                            ('--delink', 'delink.exe')):
        p = 'build/tools/' + fichero
        if os.path.exists(ROOT + p):
            out += [opcion, p]
    return out


def informe(cfg, rep):
    """objdiff.json con `cfg` y su informe en `rep`; devuelve sus medidas o None."""
    json.dump(cfg, open(ROOT + 'objdiff.json', 'w', encoding='utf-8'), indent=2)
    if os.path.exists(rep):
        os.remove(rep)
    corre([ROOT + 'build/tools/objdiff-cli.exe', 'report', 'generate',
           '-c', 'functionRelocDiffs=none', '-c', 'ppc.calculatePoolRelocations=false', '-o', rep])
    if not os.path.exists(rep):
        return None
    return json.load(open(rep))['measures']


def linea(m, fallos, sin_objeto):
    return '%.2f %% codigo (%s/%s B), %s/%s funciones, %s unidades completas; %d objetos no compilan, %d unidades sin objeto' % (
        float(m.get('matched_code_percent', 0)), m.get('matched_code'), m.get('total_code'),
        m.get('matched_functions'), m.get('total_functions'), m.get('complete_units'), len(fallos), sin_objeto)


filas = []
try:
    for v in versiones:
        r = corre([sys.executable, 'configure.py', '--version', v, *herramientas(v)])
        if r.returncode:
            filas.append((v, 'configure fallo', r.stderr[-200:]))
            continue
        if not solo_informe:
            for intento in range(3):
                b = corre([sys.executable, '-m', 'ninja', '-k', '0'], S + v + '.log')
                if 'FAILED:' not in b.stdout or 'C1083' not in b.stdout:
                    break
        log = open(S + v + '.log', encoding='utf-8', errors='replace').read() if os.path.exists(S + v + '.log') else ''
        fallos = re.findall(r'FAILED: (?:\[code=\d+\] )?(\S+)', log)
        fallidos = {os.path.normpath(f) for f in fallos}
        if any(f.endswith('config.json') for f in fallos):
            filas.append((v, 'fallo el troceado (config.json): no se mide con objetos de otro build', ''))
            continue
        pj = ROOT + 'objdiff.json'
        cfg = json.load(open(pj, encoding='utf-8'))
        propio = 'build/%s/' % v
        if not any(propio in (u.get('target_path', '') + u.get('base_path', '')).replace(chr(92), '/')
                   for u in cfg.get('units', [])):
            filas.append((v, 'objdiff.json no es de esta version (fallo el troceado o configure)',
                          ' '.join(f.split('/')[-1] for f in fallos[:12])))
            continue
        sin_objeto = 0
        funclets = quita_L.mapeos_funclets(ROOT) if v == 'SPEED_EXE_1_3' else {}
        for u in cfg.get('units', []):
            u.setdefault('metadata', {})['complete'] = False      # el metodo de upstream: Matching no cuenta
            bp = u.get('base_path')
            if bp and (not os.path.exists(os.path.join(ROOT, bp)) or os.path.normpath(bp) in fallidos):
                del u['base_path']
                sin_objeto += 1
            if v == 'SPEED_EXE_1_3' and u.get('base_path'):
                sin_etiquetas(u)                                   # PC: sin las etiquetas $L (scripts/quita_L.py)
                if u['name'] in funclets:                          # y los mapeos de .text$x/$yc/$yd, con el objeto ya hecho
                    u['symbol_mappings'] = dict(u.get('symbol_mappings') or {}, **funclets[u['name']])
            resolver_sufijos(u, ROOT)                              # los `*` de configure.py, con el objeto recien hecho
            sufijos_automaticos(u, ROOT)                           # y las demas clases locales con otro numero
        if v == 'EUROPEGERMILESTONE':
            # diagnostico: la medida fichero a fichero de siempre
            m = informe(cfg, S + v + '-fichero.json')
            if m:
                filas.append((v + ' (fichero)', linea(m, fallos, sin_objeto) + ' [diagnostico, sin LTCG]', ''))
            # la oficial: el enlace LTCG troceado por unidad (scripts/ltcg360.py)
            avisos = []
            res, motivo = ltcg360.generar(ROOT, 'build.ninja', fallidos, solo_trocear=solo_informe,
                                          avisa=avisos.append)
            if res is None:
                filas.append((v, 'el enlace LTCG fallo: %s' % motivo, ' | '.join(avisos[-3:])))
                continue
            sin_objeto = 0
            for u in cfg.get('units', []):
                bp = u.get('base_path')
                if not bp:
                    continue
                nuevo = res.get(bp.replace(chr(92), '/'))
                if nuevo and os.path.exists(os.path.join(ROOT, nuevo)):
                    u['base_path'] = nuevo
                    # el original con la misma normalizacion (pool por valor, tablas por destino): ltcg360.py
                    tn = ltcg360.ruta_original(ROOT, u['target_path']) if u.get('target_path') else None
                    if tn:
                        u['target_path'] = tn
                    resolver_sufijos(u, ROOT)
                    sufijos_automaticos(u, ROOT)
                else:
                    del u['base_path']
                    sin_objeto += 1
        m = informe(cfg, S + v + '.json')
        if not m:
            filas.append((v, 'sin informe', ''))
            continue
        filas.append((v, linea(m, fallos, sin_objeto) + (' [LTCG]' if v == 'EUROPEGERMILESTONE' else ''),
                      ' '.join(f.split('/')[-1] for f in fallos[:12])))
finally:
    corre([sys.executable, 'configure.py'])

for v, res, extra in filas:
    print('%-20s %s' % (v, res))
    if extra:
        print('%-20s   %s' % ('', extra))
