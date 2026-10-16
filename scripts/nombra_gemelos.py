#!/usr/bin/env python3
"""nombra_gemelos.py <version> [--aplicar] [--validar F] [unidad...] -- nombres de fn_ del X360 por
GEMELO COMDAT ya nombrado en otra unidad.

El X360 va enlazado con LTCG: cada SourceList lleva SUS PROPIAS copias de las funciones inline y
plantillas, byte a byte iguales a las de otras unidades. Las rondas 12/13 nombraron contra NUESTRO
objeto (tools/nombra_por_codigo.py, scripts/nombra_grafo.py) y sus reglas de unicidad (un nombre
nuestro sin casar, una pareja por grupo) dejan fuera a las copias cuyo gemelo YA esta nombrado en
otra unidad: 1 nombre nuestro frente a N copias del original.

Este casador es original-contra-original, dentro del MISMO binario: un fn_ recibe el nombre de la
funcion YA NOMBRADA (por rondas verificadas) cuyo esqueleto (codigo con las palabras reubicadas
reducidas a su opcode, el mismo de nombra_grafo) es el mismo, con estas garantias:

  - el esqueleto coincide con UN SOLO nombre distinto en todo el binario (las colisiones
    multi-unidad tipo Lerp/VU0_v3lerp de la ronda 12 quedan fuera);
  - ese nombre no esta ya en la unidad (un nombre, como mucho, una vez por unidad);
  - si hay varias copias del fn_ en la unidad, SOLO LA PRIMERA (direccion menor) se nombra;
  - los DESTINOS con nombre de las reubicaciones del fn_ no contradicen a los del gemelo nombrado
    (cada destino con nombre del fn_ aparece en la misma posicion en alguna copia del gemelo);
  - NUESTRO objeto de la unidad DEFINE ese nombre con el MISMO esqueleto (la copia que compila
    nuestro fuente de esa unidad es identica: tercera copia de corroboracion);
  - el nombre cabe (<= 250: jeff no admite `filename:`).

    python scripts/nombra_gemelos.py EUROPEGERMILESTONE            # informa
    python scripts/nombra_gemelos.py EUROPEGERMILESTONE --aplicar
    python scripts/nombra_gemelos.py EUROPEGERMILESTONE --validar 0.5
"""
import collections
import glob
import os
import re
import struct
import sys
from escritura import entero

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MIN_BYTES = 20
MAX_NOMBRE = 250
SIN_NOMBRE = re.compile(r'^(fn|sub|func|lbl|zz|j_sub|nullsub)_[0-9A-Fa-f_]+$')


def coff(ruta):
    d = open(ruta, 'rb').read()
    _, nsec, _, psym, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
    secs = []
    for i in range(nsec):
        o = 20 + opt + i * 40
        nombre, vsz, va, rsz, praw, prel, plin, nrel, nlin, car = struct.unpack_from('<8sIIIIIIHHI', d, o)
        relocs = [struct.unpack_from('<IIH', d, prel + k * 10) for k in range(nrel)]
        secs.append({'n': nombre.rstrip(b'\0').decode('latin-1'), 'praw': praw, 'tam': rsz,
                     'car': car, 'relocs': relocs})
    cad = psym + nsym * 18
    sims, i = [], 0
    while i < nsym:
        o = psym + i * 18
        nombre = d[o:o + 8]
        if nombre[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', nombre, 4)[0]
            nombre = d[cad + off:d.index(b'\0', cad + off)]
        else:
            nombre = nombre.rstrip(b'\0')
        valor, sec, tipo, clase, naux = struct.unpack_from('<IhHBB', d, o + 8)
        sims.append((i, nombre.decode('latin-1'), valor, sec, clase))
        i += 1 + naux
    return d, secs, {s[0]: s for s in sims}


def funciones(ruta):
    """[(nombre, bytes, [(offset, destino)])] de las funciones con codigo del objeto"""
    d, secs, por_indice = coff(ruta)
    por_sec = collections.defaultdict(list)
    for _, nom, val, sec, clase in por_indice.values():
        if sec <= 0 or clase not in (2, 3):
            continue
        s = secs[sec - 1]
        if not (s['car'] & 0x20) or s['n'] != '.text':
            continue
        por_sec[sec].append((val, nom))
    out = []
    for sec, lista in por_sec.items():
        lista.sort()
        s = secs[sec - 1]
        datos = d[s['praw']:s['praw'] + s['tam']]
        for k, (val, nom) in enumerate(lista):
            fin = lista[k + 1][0] if k + 1 < len(lista) else len(datos)
            if fin <= val or nom.startswith(('$L', '.L', '$LN')):
                continue
            rel = [(r - val, por_indice.get(rs, (0, '?', 0, 0, 0))[1])
                   for r, rs, _ in s['relocs'] if val <= r < fin]
            out.append((nom, datos[val:fin], rel))
    return out


def esqueleto(b, rel):
    """(opcode,0) en las palabras reubicadas; (palabra,1) en el resto (nombra_grafo)"""
    n = len(b) // 4
    w = struct.unpack('>%dI' % n, b[:n * 4])
    m = {r // 4 for r, _ in rel}
    return tuple((x >> 26, 0) if i in m else (x, 1) for i, x in enumerate(w))


def destinos(rel):
    """{posicion de palabra: nombre} de los destinos CON nombre real"""
    return {r // 4: n for r, n in rel
            if n and not SIN_NOMBRE.match(n) and not n.startswith(('$', '.', '__real@', '??_C@'))}


def unidades(version, cuales=None):
    out = []
    for p in sorted(glob.glob('build/%s/obj/Speed/Indep/SourceLists/*.obj' % version)):
        u = os.path.basename(p)[:-4]
        if cuales and u not in cuales:
            continue
        out.append((u, p))
    return out


def main():
    argv = sys.argv[1:]
    args = [a for a in argv if not a.startswith('--')]
    if '--validar' in argv:
        args = [a for a in args if a != argv[argv.index('--validar') + 1]]
    version = args[0] if args else 'EUROPEGERMILESTONE'
    cuales = set(args[1:]) or None
    aplicar = '--aplicar' in argv
    validar = float(argv[argv.index('--validar') + 1]) if '--validar' in argv else 0.0

    uns = unidades(version, cuales)
    orig = {u: funciones(p) for u, p in uns}
    nuestros = {}
    for u, p in uns:
        sp = p.replace('/obj/', '/src/')
        nuestros[u] = collections.defaultdict(set)
        if os.path.exists(sp):
            for nom, b, rel in funciones(sp):
                if len(b) >= MIN_BYTES:
                    nuestros[u][nom].add(esqueleto(b, rel))

    # esqueleto -> nombres nombrados + destinos de cada copia
    esq_nombres = collections.defaultdict(set)
    esq_dests = collections.defaultdict(list)
    for u in orig:
        for nom, b, rel in orig[u]:
            if SIN_NOMBRE.match(nom) or len(b) < MIN_BYTES:
                continue
            e = esqueleto(b, rel)
            esq_nombres[e].add(nom)
            esq_dests[e].append(destinos(rel))

    if validar:
        import random
        random.seed(20261003)
        todas = sorted({n for s in esq_nombres.values() for n in s})
        escondidas = {n for n in todas if random.random() < validar}
        esq_nombres_v = collections.defaultdict(set)
        for e, nombres in esq_nombres.items():
            esq_nombres_v[e] = nombres - escondidas
    else:
        esq_nombres_v, escondidas = esq_nombres, set()

    propuestas = {}        # fn_ -> (unidad, nombre)
    cuenta = collections.Counter()
    for u in orig:
        nombres_en_u = {nom for nom, b, rel in orig[u] if not SIN_NOMBRE.match(nom)}
        vistos = set()
        pares = sorted(orig[u], key=lambda x: id(x))  # orden de direccion (funciones van en orden)
        for nom, b, rel in pares:
            if len(b) < MIN_BYTES or not SIN_NOMBRE.match(nom):
                continue
            e = esqueleto(b, rel)
            nombres = {n for n in esq_nombres_v.get(e, ()) if n not in nombres_en_u and n not in vistos}
            if len(nombres) != 1:
                cuenta['esqueleto ambiguo o sin gemelo' if nombres else 'sin gemelo nombrado'] += 1
                continue
            n = next(iter(nombres))
            if len(n) > MAX_NOMBRE:
                cuenta['nombre demasiado largo'] += 1
                continue
            d1 = destinos(rel)
            if not all(any(d0.get(k) == v for d0 in esq_dests[e]) for k, v in d1.items()):
                cuenta['destinos que contradicen al gemelo'] += 1
                continue
            if e not in nuestros[u].get(n, ()):          # nuestro objeto de la unidad, mismo esqueleto
                cuenta['nuestro objeto no lo define igual'] += 1
                continue
            vistos.add(n)
            propuestas[nom] = (u, n)
            cuenta['propuesta'] += 1

    print('unidades: %d; propuestas: %d' % (len(uns), len(propuestas)))
    for k, c in cuenta.most_common():
        if k != 'propuesta':
            print('   fuera %-38s %d' % (k, c))
    por_unidad = collections.Counter(u for u, n in propuestas.values())
    for u, c in por_unidad.most_common():
        print('   %-28s %d' % (u, c))
    if validar:
        bien = sum(1 for f, (u, n) in propuestas.items() if n in escondidas)
        # propuestas sobre direcciones de escondidas con OTRO nombre = colisiones
        dirs_escondidas = {}
        for u in orig:
            for nom, b, rel in orig[u]:
                if nom in escondidas:
                    dirs_escondidas[nom] = nom
        mal = sum(1 for f, (u, n) in propuestas.items() if f in dirs_escondidas and dirs_escondidas[f] != n)
        print('VALIDACION (escondidas %d de %d): recuperadas con SU nombre %d; sobre una escondida con OTRO nombre %d'
              % (len(escondidas), len(escondidas) + len([n for s in esq_nombres.values() for n in s]) - len(set().union(*esq_nombres.values())), bien, mal))
        return
    for f, (u, n) in sorted(propuestas.items())[:15]:
        print('   %s -> %s (%s)' % (f, n[:70], u))
    if not aplicar or not propuestas:
        return
    ruta = 'config/%s/symbols.txt' % version
    raw = open(ruta, 'rb').read().decode('utf-8')
    assert '\r\n' not in raw, 'symbols.txt tiene que ser LF'
    lineas = raw.split('\n')
    hechos = 0
    for i, l in enumerate(lineas):
        m = re.match(r'(\S+)( = \.text:0x[0-9A-Fa-f]+;.*)$', l)
        if m and m.group(1) in propuestas:
            lineas[i] = propuestas[m.group(1)][1] + m.group(2)
            hechos += 1
    entero(ruta, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
    print('aplicados %d nombres' % hechos)


if __name__ == '__main__':
    main()
