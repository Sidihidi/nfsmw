#!/usr/bin/env python3
"""Mide con pctall.py de UNO EN UNO aunque haya varios agentes trabajando.

Dos pctall a la vez reconfiguran build.ninja uno encima del otro y compilan
objetos a medias: las medidas se envenenan sin dar ningun error.  Esto pone un
cerrojo (un directorio en build/, que crearlo es atomico), mide, y lo suelta.
Mientras mide, el dueno renueva su marca cada LATIDO segundos; si la marca
lleva mas de MAXIMO minutos sin renovarse, el dueno se da por muerto y se rompe.

Ademas imprime el estado de las funciones que le pidas, con CUATRO decimales:
un 99,996 que el informe redondea a 100,00 NO esta casado (matched_code es todo
o nada), y aqui se ve.

  python scripts/medir.py SLES-53558-A124 --quien memorycard --funcs MemoryCard
  python scripts/medir.py SLES-53558-A124 SLUS-21351 GOWE69 --quien eagl4 --funcs EvalSQT
  python scripts/medir.py SLES-53558-A124 --solo-leer --funcs MemoryCard   # sin compilar
"""
import argparse
import json
import os
import subprocess
import sys
import threading
import time

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CERROJO = os.path.join(RAIZ, 'build', '.medir.lock')
DUENO = os.path.join(CERROJO, 'dueno.txt')
MAXIMO = 30  # minutos: tres versiones tardan unos 6
LATIDO = 60  # segundos entre renovaciones de la marca del dueno vivo


def soltar():
    # Nunca rmtree: solo el fichero y el directorio vacio.
    try:
        os.remove(DUENO)
    except OSError:
        pass
    try:
        os.rmdir(CERROJO)
    except OSError:
        pass


def coger(quien):
    avisado = False
    while True:
        try:
            os.mkdir(CERROJO)
            with open(DUENO, 'w') as fh:
                fh.write('%s %f\n' % (quien, time.time()))
            return
        except FileExistsError:
            try:
                with open(DUENO) as fh:
                    otro, desde = fh.read().split()
                if time.time() - float(desde) > MAXIMO * 60:
                    print('cerrojo de %s muerto (mas de %d min): lo rompo' % (otro, MAXIMO),
                          flush=True)
                    soltar()
                    continue
                if not avisado:
                    print('esperando: mide %s' % otro, flush=True)
                    avisado = True
            except (OSError, ValueError):
                pass
            time.sleep(15)


def latir(quien, parar):
    """Renueva la marca del cerrojo mientras la medida sigue viva.

    Sin esto, una medida larga (build contendido, troceado de SLUS) pasaba de MAXIMO minutos y el siguiente en
    cola la daba por muerta y medía encima: el 2026-09-22 hubo dos pctall a la vez sobre SLUS durante una hora.
    Solo renueva si el cerrojo sigue siendo suyo."""
    while not parar.wait(LATIDO):
        try:
            with open(DUENO) as fh:
                otro = fh.read().split()[0]
            if otro != quien:
                return
            with open(DUENO, 'w') as fh:
                fh.write('%s %f\n' % (quien, time.time()))
        except (OSError, ValueError, IndexError):
            return


def estado(version, filtros):
    # El informe de ESTA medida es el que genera pctall en build/versiones/: lo
    # hace el mismo con objdiff y quitando las unidades que no compilan.  El de
    # build/<V>/report.json lo escribe el paso REPORT de ninja, y si un objeto
    # no compila ninja se para antes y se queda VIEJO sin avisar (lo cazo el
    # agente de zEagl4Anim leyendo SLUS mientras zFe no compilaba).
    ruta = os.path.join(RAIZ, 'build', 'versiones', version + '.json')
    if not os.path.exists(ruta):
        ruta = os.path.join(RAIZ, 'build', version, 'report.json')
        print('  AVISO: no hay build/versiones/%s.json; leo %s, que puede estar viejo'
              % (version, os.path.relpath(ruta, RAIZ)))
    try:
        with open(ruta, encoding='utf-8') as fh:
            rep = json.load(fh)
    except (OSError, ValueError):
        print('  %s: sin informe' % version)
        return
    edad = (time.time() - os.path.getmtime(ruta)) / 60.0
    print('  (%s, de hace %.0f min)' % (os.path.relpath(ruta, RAIZ), edad))
    filas = []
    for u in rep.get('units', []):
        for f in u.get('functions', []) or []:
            if any(x in f.get('name', '') for x in filtros):
                filas.append((f.get('fuzzy_match_percent', 0.0), int(f.get('size', 0) or 0),
                              f['name'], u.get('name', '').split('/')[-1]))
    casadas = sum(1 for p, _, _, _ in filas if p == 100.0)
    print('  %s: %d funciones, %d CASADAS, %d B sin casar'
          % (version, len(filas), casadas, sum(s for p, s, _, _ in filas if p != 100.0)))
    for p, s, n, u in sorted(filas):
        marca = 'CASADA' if p == 100.0 else '      '
        print('    %s %9.4f %%  %6d B  %-14s %s' % (marca, p, s, u[:14], n[:70]))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('versiones', nargs='+')
    ap.add_argument('--quien', default=os.environ.get('USERNAME', 'anonimo'))
    ap.add_argument('--funcs', default='', help='subcadenas separadas por comas')
    ap.add_argument('--solo-leer', action='store_true', help='no compila, solo lee')
    args = ap.parse_args()
    filtros = [x for x in args.funcs.split(',') if x]

    coger(args.quien)
    parar = threading.Event()
    hilo = threading.Thread(target=latir, args=(args.quien, parar), daemon=True)
    hilo.start()
    try:
        if not args.solo_leer:
            r = subprocess.run([sys.executable, os.path.join(RAIZ, 'scripts', 'pctall.py')]
                               + args.versiones, cwd=RAIZ, capture_output=True, text=True)
            print(r.stdout.strip())
            if r.returncode != 0:
                print(r.stderr[-3000:])
        if filtros:
            for v in args.versiones:
                estado(v, filtros)
    finally:
        parar.set()
        hilo.join(5)
        soltar()


if __name__ == '__main__':
    main()
