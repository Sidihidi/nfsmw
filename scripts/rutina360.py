#!/usr/bin/env python3
"""rutina360.py -- la rutina COMPLETA de nombres de Xbox 360 en un solo script (ronda 54).

Los nombres de symbols.txt de EUROPEGERMILESTONE son heredados (de PC, de upstream, de IDA y de rondas viejas) y una
parte es falsa: objdiff empareja por nombre, asi que un nombre falso sobre codigo que ya escribimos bien da <100 %.
Cada herramienta corrige una clase de error distinta; aqui van todas, en el orden en que se probaron (rondas 42-53),
siempre con la version EXPLICITA y contra la medida OFICIAL de 360 (el enlace LTCG de scripts/ltcg360.py:
build/EUROPEGERMILESTONE/ltcg/{NFS_ltcg.exe,NFS_ltcg.map,orig,obj} y build/versiones/EUROPEGERMILESTONE.json).

  1. nombra_grafo.py EUROPEGERMILESTONE --nuestros=build/EUROPEGERMILESTONE/ltcg/obj
                              funciones SIN nombre (fn_) por esqueleto unico y por el grafo de llamadas
  2. nombra_corrige360.py     nombres falsos que no casan: los llamantes unanimes (anclas al 100 %) dicen el bueno
  3. nombra_permuta360.py     nombres cambiados entre funciones de la unidad con el mismo codigo (ciclos y cadenas)
  4. nombra_vtable360.py      virtuales diminutas por el hueco de la vtable (la nuestra frente a la del original)
  5. tipos360.py              GetTypeInfo/GetTypeName/CreateObject de sonido por la CADENA de su TypeInfo (ronda 54)
  6. orden360.py              funciones entre dos anclas en el mismo orden y con el mismo codigo (ronda 54)
  7. ctor360.py --anonimizar  constructores y CreateObject por la vtable que escriben (ronda 53)
  8. falsos360.py --varias    nombre falso por codigo (la funcion del original con el nombre no es la nuestra)
  9. anonimiza360.py          quita nombres que el codigo contradice y nadie reclama (vuelven a fn_)
 10. rota360.py --min 1       nombres rotados entre funciones que YA casan (no cambia la medida, corrige la verdad)
 11. refs360.py --fn          nombres por las referencias de las anclas, tambien entre unidades y por DATO (los
                              _IHandle, los ctor de interfaz, Find/Fetch<T>); y las fn_ que dos anclas o mas nombran
                              aunque aun no casen (ronda 55)

Todas leen la ULTIMA medida oficial: lo que una pasada renombra no se ve hasta medir otra vez. Por eso la rutina va a
vueltas: pasada -> medir.py EUROPEGERMILESTONE -> pasada ... hasta una pasada sin cambios.

    python scripts/rutina360.py                         # informe: cada paso sin --aplicar (cada uno ve el arbol de
                                                        # partida, asi que las cifras no se suman)
    python scripts/rutina360.py --aplicar               # una pasada, cada paso con --aplicar
    python scripts/rutina360.py --aplicar --medir --quien NOMBRE [--vueltas N]
                                                        # pasada + medir.py (con su candado) hasta 0 cambios
    --logs=DIR    guarda la salida entera de cada paso (DIR/v<vuelta>_<paso>.log)
    --sin=PASO,.. salta pasos (grafo, corrige, permuta, vtable, tipos, orden, ctor, falsos, anonimiza, rota, refs)

Cuenta los cambios de cada paso como lineas distintas de symbols.txt antes/despues (vale para todos los pasos).
Nunca toca otra version: si el symbols.txt de otra cambia mientras corre, lo avisa (sera otra sesion).
"""
import argparse
import os
import subprocess
import time
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
V = 'EUROPEGERMILESTONE'
SIMS = os.path.join(RAIZ, 'config', V, 'symbols.txt')
PASOS = [
    ('grafo', ['nombra_grafo.py', V, '--nuestros=' + os.path.join('build', V, 'ltcg', 'obj')]),
    ('corrige', ['nombra_corrige360.py']),
    ('permuta', ['nombra_permuta360.py']),
    ('vtable', ['nombra_vtable360.py']),
    ('tipos', ['tipos360.py']),
    ('orden', ['orden360.py']),
    ('ctor', ['ctor360.py', '--anonimizar']),
    ('falsos', ['falsos360.py', '--varias']),
    ('anonimiza', ['anonimiza360.py']),
    ('rota', ['rota360.py', '--min', '1']),
    ('refs', ['refs360.py', '--fn']),
]
RESUMEN = {'grafo': 'FINALES', 'ctor': 'ctor/dtor/factoria', 'orden': 'por orden', 'tipos': 'por TypeInfo',
           'refs': 'refs360:'}   # la linea de resumen de cada paso (si no, la ultima)
OTRAS = ['GOWE69', 'SLES-53558-A124', 'SLUS-21351', 'SPEED_EXE_1_3']


def lineas():
    with open(SIMS, encoding='utf-8') as fh:
        return fh.read().split('\n')


def distintas(a, b):
    if len(a) != len(b):   # ninguna herramienta anade ni quita lineas; si pasa, cuenta la diferencia grosera
        return abs(len(a) - len(b)) + sum(1 for x, y in zip(a, b) if x != y)
    return sum(1 for x, y in zip(a, b) if x != y)


def huellas():
    out = {}
    for v in OTRAS:
        p = os.path.join(RAIZ, 'config', v, 'symbols.txt')
        if os.path.exists(p):
            out[v] = os.path.getmtime(p)
    return out


def pasada(args, vuelta):
    total = 0
    for nombre, orden in PASOS:
        if nombre in args.sin:
            continue
        antes = lineas()
        crudo = open(SIMS, 'rb').read()
        cmd = [sys.executable, os.path.join(RAIZ, 'scripts', orden[0])] + orden[1:]
        if args.aplicar:
            cmd.append('--aplicar')
        for intento in range(3):
            r = subprocess.run(cmd, cwd=RAIZ, capture_output=True, encoding='utf-8', errors='replace')
            salida = (r.stdout or '') + (r.stderr or '')
            # Windows: symbols.txt abierto por otro proceso (antivirus, otra lectura) da OSError al escribir; la
            # herramienta no escribe nada y se puede repetir (cada paso recalcula desde el symbols.txt actual)
            if r.returncode == 0 or 'OSError' not in salida or lineas() != antes:
                break
            print('  %-10s OSError al escribir symbols.txt: lo repito en 10 s' % nombre, flush=True)
            time.sleep(10)
        despues = lineas()
        if r.returncode != 0 or len(despues) != len(antes):
            # ronda 55: con el disco lleno un paso dejo symbols.txt VACIO (open('w') trunca antes de escribir).
            # Ninguna herramienta anade ni quita lineas: si cambia el numero, se devuelve el fichero de antes del paso.
            tmp = SIMS + '.rutina360.tmp'
            with open(tmp, 'wb') as fh:
                fh.write(crudo)
            os.replace(tmp, SIMS)
            print('  %-10s ERROR (codigo %d, %d -> %d lineas): symbols.txt devuelto al de antes del paso'
                  % (nombre, r.returncode, len(antes), len(despues)), flush=True)
            print(salida[-2000:])
            sys.exit(1)
        if args.logs:
            os.makedirs(args.logs, exist_ok=True)
            with open(os.path.join(args.logs, 'v%d_%s.log' % (vuelta, nombre)), 'w', encoding='utf-8') as fh:
                fh.write(salida)
        n = distintas(antes, lineas())
        total += n
        ult = [l for l in salida.strip().split('\n') if l.strip()]
        res = [l for l in ult if nombre in RESUMEN and RESUMEN[nombre] in l] or ult[-1:] or ['(sin salida)']
        print('  %-10s %4d lineas  | %s' % (nombre, n, res[0][:110]), flush=True)
        if r.returncode != 0:
            print('  ERROR en %s (codigo %d):\n%s' % (nombre, r.returncode, salida[-2000:]))
            sys.exit(1)
    return total


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--aplicar', action='store_true')
    ap.add_argument('--medir', action='store_true', help='medir.py %s tras cada pasada con cambios' % V)
    ap.add_argument('--quien', default=os.environ.get('USERNAME', 'rutina360'))
    ap.add_argument('--vueltas', type=int, default=4)
    ap.add_argument('--logs', default='')
    ap.add_argument('--sin', default='')
    args = ap.parse_args()
    args.sin = set(x for x in args.sin.split(',') if x)
    if args.medir and not args.aplicar:
        ap.error('--medir solo tiene sentido con --aplicar')
    h0 = huellas()
    for vuelta in range(1, (args.vueltas if args.medir else 1) + 1):
        print('vuelta %d%s' % (vuelta, '' if args.aplicar else ' (informe, sin aplicar)'), flush=True)
        n = pasada(args, vuelta)
        print('vuelta %d: %d lineas de symbols.txt cambiadas' % (vuelta, n), flush=True)
        h1 = huellas()
        if h1 != h0:   # aviso: con varias sesiones a la vez otra puede estar tocando su version (nunca es esta rutina)
            print('AVISO: ha cambiado el symbols.txt de %s mientras corria la rutina (otra sesion?)'
                  % ', '.join(v for v in h1 if h1[v] != h0.get(v)))
            h0 = h1
        if not args.medir or n == 0:
            break
        r = subprocess.run([sys.executable, os.path.join(RAIZ, 'scripts', 'medir.py'), V, '--quien', args.quien],
                           cwd=RAIZ, capture_output=True, encoding='utf-8', errors='replace')
        print('\n'.join(l for l in (r.stdout or '').split('\n') if V in l or 'Matched' in l or 'funciones' in l)
              or (r.stdout or '')[-800:], flush=True)
        if r.returncode != 0:
            print('ERROR en medir.py:\n%s' % ((r.stderr or '')[-2000:]))
            return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
