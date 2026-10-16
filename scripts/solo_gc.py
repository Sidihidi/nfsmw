#!/usr/bin/env python3
"""Funciones cuyo UNICO cuerpo esta dentro de un `#ifdef EA_PLATFORM_GAMECUBE'.

La tercera forma del fallo de la leccion 68, generalizada.  `VU0_v4add' y otras
trece estaban asi: `UMath.h' las llama, `UVectorMath.h' las declara y el unico
cuerpo que existe esta dentro del bloque de GameCube de `UVectorMath.cpp',
escrito con los paired-singles de Gekko.  En PS2 salen como salto a un simbolo
indefinido, y no se ve porque de PS2 comparamos objetos y no enlazamos
ejecutable: el enlazador nunca llega a quejarse.

  python scripts/solo_gc.py
  python scripts/solo_gc.py --guarda EA_PLATFORM_PLAYSTATION2   # al reves
"""
import argparse
import os
import re

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

FUERA = ('/stl/', '/stlport/', '/Packages/', '/stlps2/', '/gpp/', '/gcc/',
         '/newlib', '/include/xbox/', '/msl/')

# Una definicion: `tipo nombre(args) {' al principio de linea, sin `;'.
RE_DEF = re.compile(
    r'^[A-Za-z_][\w:<>,\s\*&]*?\b(\w+)\s*\([^;{]*\)\s*(?:const\s*)?\{')
RE_IF = re.compile(r'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$')


def regiones(lineas, guarda):
    """Para cada linea, si esta dentro de un bloque exclusivo de `guarda'."""
    pila, fuera = [], []
    for linea in lineas:
        m = RE_IF.match(linea)
        if m:
            que, resto = m.group(1), m.group(2)
            if que in ('if', 'ifdef', 'ifndef'):
                positivo = (guarda in resto
                            and que != 'ifndef'
                            and not re.search(r'!\s*defined\s*\(\s*' + guarda,
                                              resto))
                pila.append(positivo)
            elif que in ('elif', 'else'):
                if pila:
                    # Tras un `#else' de un bloque de la guarda, ya NO estamos
                    # dentro; y tras el `#else' de otro bloque, tampoco entramos.
                    pila[-1] = False
            elif que == 'endif' and pila:
                pila.pop()
            fuera.append(any(pila))
            continue
        fuera.append(any(pila))
    return fuera


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--guarda', default='EA_PLATFORM_GAMECUBE')
    args = p.parse_args()

    dentro, fuera_de = {}, {}
    for raiz, _, ficheros in os.walk(os.path.join(RAIZ, 'src')):
        for f in ficheros:
            if not f.endswith(('.h', '.hpp', '.cpp', '.c', '.inl')):
                continue
            ruta = os.path.join(raiz, f)
            rel = os.path.relpath(ruta, RAIZ).replace(os.sep, '/')
            if any(x in '/' + rel for x in FUERA):
                continue
            # Un fichero que ya vive en un directorio de plataforma no cuenta:
            # ahi el `#ifdef' es redundante, no un agujero.
            if '/GameCube/' in '/' + rel or '/PSX2/' in '/' + rel:
                continue
            try:
                with open(ruta, encoding='utf-8', errors='replace') as fh:
                    lineas = fh.readlines()
            except OSError:
                continue
            marcas = regiones(lineas, args.guarda)
            for i, linea in enumerate(lineas):
                m = RE_DEF.match(linea)
                if not m:
                    continue
                nombre = m.group(1)
                destino = dentro if marcas[i] else fuera_de
                destino.setdefault(nombre, []).append('%s:%d' % (rel, i + 1))

    solo = sorted(n for n in dentro if n not in fuera_de)
    print('Funciones con cuerpo SOLO dentro de #ifdef %s: %d'
          % (args.guarda, len(solo)))
    for n in solo:
        print('  %-36s %s' % (n, dentro[n][0]))


if __name__ == '__main__':
    main()
