#!/usr/bin/env python3
"""Histograma por FUNCION: que mnemonicos y que destinos de llamada sobran o
faltan en cada funcion que no casa.

La leccion 67 conto mnemonicos por unidad y delato ramas de plataforma vacias.
Llevado a la funcion, el mismo recuento dice de un vistazo si a una casi-exacta
le falta una llamada, le sobra una multiplicacion o cambia un tipo de salto,
sin leer el desensamblado entero.

Cuatro normalizaciones, cada una descubierta por un falso positivo:

  1. Los rotulos locales se llaman distinto en cada lado ($LC875 contra
     lbl_803DECF0, .text+0x15cfc contra un nombre): se cuentan todos en el
     mismo cubo `(local)', que ademas esta fuera del recuento por defecto
     porque nuestro .o lleva reubicaciones que el original ya trae resueltas.

  2. Una llamada a una funcion del MISMO objeto no deja reubicacion en nuestro
     .o (el ensamblador resuelve el salto) y si la deja en el original, que
     viene de trocear el binario.  `_vfwrite' salia como `faltan 13' y esta
     llamado 13 veces.  Se descartan los destinos definidos en el objeto.

  3. Lo mismo con los DATOS, y mucho peor: el ensamblador reduce la referencia
     a `.data+0x3d4' y el original la nombra.  `cFEng::mInstance' salia con
     831 referencias de menos en zFe, todas falsas.

  4. Con -g el objeto lleva las etiquetas de linea ($LM3549) como simbolos y
     objdump abre un bloque en cada una: `UpdateHud' salia con 15 instrucciones
     de 1.653 y 533 `lw' "de menos".  Son continuacion, no funciones.  Ademas
     cada funcion se corta por su tamano del symtab, que es lo que separa una
     funcion de la siguiente cuando a la siguiente le falta el simbolo.

Uso:
  python scripts/histofn.py GOWE69                      # peores del informe
  python scripts/histofn.py SLES-53558-A124 --unidad zSim
  python scripts/histofn.py GOWE69 --simbolo eProject__FP8bVector3PC8bVector3
  python scripts/histofn.py GOWE69 --busca PSMTX44Copy  # quien llama de mas
"""
import argparse
import collections
import json
import os
import re
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PPC = 'build/ppc_binutils/powerpc-eabi-objdump.exe'
MIPS = 'build/mips_binutils/mips-linux-gnu-objdump.exe'
OBJDUMP = {
    'GOWE69': PPC,
    'EUROPEGERMILESTONE': PPC,
    'SLUS-21351': MIPS,
    'SLES-53558-A124': MIPS,
    'SPEED_EXE_1_3': MIPS,
}

RE_FUNC = re.compile(r'^([0-9a-f]+) <(.+)>:')
# SIN grupo para los bytes crudos: siempre se desensambla con
# --no-show-raw-insn, y ese grupo se comia los mnemonicos de una sola letra que
# ademas son digitos hexadecimales.  `b d92c <$L23297>' contaba `d92c' como
# mnemonico y perdia el `b'.
RE_INSN = re.compile(r'^\s*([0-9a-f]+):\s+([a-z][a-z0-9_.+-]*)')
RE_RELOC = re.compile(r'^\s*([0-9a-f]+):\s+R_\w+\s+(\S+)')
RE_ETIQUETA = re.compile(r'^[$.]L')
RE_SIMB = re.compile(r'^([0-9a-f]+)\s+\S*\s+F\s+\S+\s+([0-9a-f]+)\s+(\S+)')

RE_LOCAL = re.compile(r'^(?:\$L\w*\d+|\.L\w+|lbl_[0-9A-Fa-f]+|func_[0-9A-Fa-f]+|'
                      r'\.(?:text|data|rodata|rdata|sdata2?|sbss2?|bss|ctors|'
                      r'dtors|over|lit4|lit8)(?:\+0x[0-9a-f]+)?)$')


def normaliza(destino):
    # Todos van al mismo cubo: solo interesa CUANTOS, no como se llaman.
    return '(local)' if RE_LOCAL.match(destino) else destino


def simbolos(objdump, ruta):
    """({funcion: tamano}, {todo simbolo DEFINIDO en el objeto}).

    El conjunto de definidos lleva tambien los datos: nuestro ensamblador
    reduce a `.data+0x3d4' una referencia a un simbolo que el objeto define, y
    el original --que viene de reensamblar el troceado-- lo nombra.  Sin esto
    `cFEng::mInstance' salia con 831 referencias de menos en zFe y era mentira.
    """
    salida = subprocess.run([objdump, '-t', ruta],
                            capture_output=True, text=True).stdout
    tam, definidos = {}, set()
    for linea in salida.splitlines():
        partes = linea.split('\t')
        if len(partes) != 2:
            continue
        izq = partes[0].split()
        der = partes[1].split(None, 1)
        if len(izq) < 2 or len(der) != 2:
            continue
        seccion, nombre = izq[-1], der[1].strip()
        if seccion.startswith('*'):          # *UND*, *ABS*, *COM*
            continue
        definidos.add(nombre)
        if 'F' in izq[1:-1]:
            try:
                tam[nombre] = int(der[0], 16)
            except ValueError:
                pass
    return tam, definidos


def desensambla(objdump, ruta):
    """({nombre: Counter de mnemonicos y destinos}, conjunto de definidos)."""
    if not os.path.exists(ruta):
        return {}, set()
    tam, definidos = simbolos(objdump, ruta)
    salida = subprocess.run([objdump, '-dr', '--no-show-raw-insn', ruta],
                            capture_output=True, text=True).stdout
    fn, actual, fin = {}, None, None
    for linea in salida.splitlines():
        m = RE_FUNC.match(linea)
        if m:
            nombre = m.group(2)
            # Compilando con -g el objeto lleva las etiquetas de linea ($LM3549)
            # como simbolos y objdump abre un bloque en cada una: `UpdateHud'
            # salia con 15 instrucciones de 1.653.  Son continuacion.
            if RE_ETIQUETA.match(nombre):
                continue
            actual = collections.Counter()
            fn[nombre] = actual
            # Sin tamano en el symtab no se puede acotar: se cuenta entera.
            t = tam.get(nombre, 0)
            fin = int(m.group(1), 16) + t if t else None
            continue
        if actual is None:
            continue
        reloc = RE_RELOC.match(linea)
        m = reloc or RE_INSN.match(linea)
        if not m:
            continue
        if fin is not None and int(m.group(1), 16) >= fin:
            actual = None
            continue
        if reloc:
            actual['-> ' + normaliza(m.group(2))] += 1
        else:
            actual[m.group(2)] += 1
    return fn, definidos


def objetos(version, unidad):
    base = os.path.join(RAIZ, 'build', version)
    pares = []
    for raiz, _, ficheros in os.walk(os.path.join(base, 'obj')):
        for f in ficheros:
            if not f.endswith('.o'):
                continue
            if unidad and unidad.lower() not in f.lower():
                continue
            orig = os.path.join(raiz, f)
            nuestro = orig.replace(os.path.join(base, 'obj'),
                                   os.path.join(base, 'src'), 1)
            if os.path.exists(nuestro):
                pares.append((os.path.splitext(f)[0], orig, nuestro))
    return sorted(pares)


def informe(version):
    """{simbolo: (porcentaje, tamano, unidad)} del report.json al dia."""
    ruta = os.path.join(RAIZ, 'build', version, 'report.json')
    if not os.path.exists(ruta):
        return {}
    with open(ruta, encoding='utf-8') as fh:
        datos = json.load(fh)
    fuera = {}
    for unidad in datos.get('units', []):
        nombre = unidad.get('name', '')
        for f in (unidad.get('measures', {}) or {}).get('functions', []) or \
                 unidad.get('functions', []) or []:
            fuera[f.get('name', '')] = (f.get('fuzzy_match_percent', 0.0),
                                        int(f.get('size', 0) or 0), nombre)
    return fuera


def diferencias(a, b, definidos, locales):
    """Lo que sobra (+) o falta (-) en b respecto de a, ya limpio."""
    dif = collections.Counter()
    for k in set(a) | set(b):
        if k.startswith('-> '):
            if k == '-> (local)' and not locales:
                continue
            # Una llamada dentro del mismo objeto no deja reubicacion en el
            # nuestro: la diferencia seria siempre falsa.
            if k[3:] in definidos:
                continue
        d = b.get(k, 0) - a.get(k, 0)
        if d:
            dif[k] = d
    return dif


def pinta(nombre, pct, tam, dif, lineas):
    cab = '%-58s' % nombre[:58]
    if pct is not None:
        cab += '  %6.2f %%  %5d B' % (pct, tam)
    print(cab)
    orden = sorted(dif.items(), key=lambda kv: (-abs(kv[1]), kv[0]))
    for k, d in orden[:lineas]:
        print('    %+4d  %s%s' % (d, k, '   sobra' if d > 0 else '   falta'))
    if len(orden) > lineas:
        print('    ... y %d mas' % (len(orden) - lineas))


def main():
    p = argparse.ArgumentParser()
    p.add_argument('version')
    p.add_argument('--unidad', default=None)
    p.add_argument('--simbolo', default=None)
    p.add_argument('--busca', default=None,
                   help='solo funciones donde difiera este mnemonico o destino')
    p.add_argument('--min', type=float, default=1.0, help='%% minimo a mirar')
    p.add_argument('--max', type=float, default=99.999, help='%% maximo')
    p.add_argument('--n', type=int, default=25)
    p.add_argument('--lineas', type=int, default=8)
    p.add_argument('--resumen', action='store_true',
                   help='suma las diferencias de TODAS las funciones')
    p.add_argument('--locales', action='store_true',
                   help='cuenta tambien los rotulos locales')
    args = p.parse_args()

    objdump = os.path.join(RAIZ, OBJDUMP.get(args.version, PPC))
    pcts = informe(args.version)
    if not pcts:
        print('aviso: report.json sin funciones; no filtro por porcentaje',
              file=sys.stderr)

    candidatas = []
    for unidad, orig, nuestro in objetos(args.version, args.unidad):
        a, def_a = desensambla(objdump, orig)
        b, def_b = desensambla(objdump, nuestro)
        definidos = def_a & def_b
        for nombre in sorted(set(a) & set(b)):
            if args.simbolo and args.simbolo not in nombre:
                continue
            pct, tam, _ = pcts.get(nombre, (None, 0, unidad))
            if pcts and not args.simbolo:
                if pct is None or not (args.min <= pct <= args.max):
                    continue
            dif = diferencias(a[nombre], b[nombre], definidos, args.locales)
            if not dif:
                continue
            if args.busca and not any(args.busca in k for k in dif):
                continue
            candidatas.append((tam, pct if pct is not None else 0.0,
                               nombre, unidad, dif))

    if args.resumen:
        total = collections.Counter()
        cuantas = collections.Counter()
        for tam, pct, nombre, unidad, dif in candidatas:
            for k, d in dif.items():
                total[k] += d
                cuantas[k] += 1
        print('%8s  %6s  %s' % ('neto', 'fn', 'mnemonico o destino'))
        for k, d in sorted(total.items(), key=lambda kv: -abs(kv[1]))[:40]:
            print('%+8d  %6d  %s' % (d, cuantas[k], k))
        print()
        print('%d funciones comparadas con diferencias' % len(candidatas))
        return

    candidatas.sort(key=lambda c: -c[0])
    for tam, pct, nombre, unidad, dif in candidatas[:args.n]:
        pinta('%s  [%s]' % (nombre, unidad), pct, tam, dif, args.lineas)
    print('\n%d funciones con histograma distinto (muestro %d)'
          % (len(candidatas), min(len(candidatas), args.n)))


if __name__ == '__main__':
    main()
