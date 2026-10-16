#!/usr/bin/env python3
"""trocea_como_a124.py -- parte las unidades de .text de SLUS que agrupan funciones de VARIAS unidades de la A124.

El .mdebug de la A124 da la frontera exacta de cada objeto de las bibliotecas (libc y libm de SN y de Sony, la libkernel,
cdvd, realgraph, rcmp...). En SLUS esas fronteras se heredaron por bloques y muchas unidades se quedaron con los
objetos siguientes pegados: sf_sin.c tenia 34 funciones (sinf, tanf, e_exp, kf_cos...), iopreset.c 63. Con la unidad
equivocada nuestra fuente no puede casar: el objeto de sf_tan.c nunca se compara con tanf.

Para cada unidad asmtu de SLUS (menos las SourceLists del juego) recorre sus funciones (las del troceado actual,
build/SLUS-21351/asm) y busca por NOMBRE la unidad de la A124 con la funcion del mismo nombre (solo nombres unicos en la
A124; las func_ sin nombre siguen a la anterior). Cada carrera de funciones de otra unidad de la A124 abre una unidad:
  - en su primera funcion menos el desfase que esa funcion tiene en la A124 (restos de funciones quitadas por el
    enlazador al principio del objeto, o una estatica sin nombre en SLUS como lflush), SOLO si el codigo de SLUS
    delante de la funcion es el de la A124 al principio de la unidad (palabra a palabra; en jal/j solo el codigo de
    operacion y en las de inmediato el codigo y los registros), no es todo ceros y no hay funciones con nombre en medio;
  - un hueco no nulo entre dos carreras se reparte entre las unidades de la A124 que faltan entre ellas si es,
    trozo a trozo y en orden, el final exacto de cada una (el resto de 8 bytes que deja un objeto quitado entero);
    si es el final de la unidad anterior de la A124, se queda donde esta.
Lo mismo con el principio de las unidades que ya existen (MOVER): refill.c empezaba en __srefill y su estatica lflush
se quedaba al final de puts.c.
No se crea una unidad con un nombre que SLUS ya tenga.

    python scripts/trocea_como_a124.py [--unidad REGEX] [--yml SALIDA.yml] [--aplicar]

Sin --yml ni --aplicar solo imprime. --yml escribe una copia de config/SLUS-21351/config.yml con los cortes (para
scripts/trocea_prueba.py); --aplicar la escribe en el arbol. Despues hay que volver a trocear y medir, y dar nombre a
las func_ que queden en las unidades nuevas (tools/nombra_ps2.py).
"""
import collections
import json
import os
import re
import struct
import sys

import yaml
from escritura import entero

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
A, S = 'SLES-53558-A124', 'SLUS-21351'
ELF = {S: os.path.join(RAIZ, 'orig', S, 'SLUS_213.51'), A: os.path.join(RAIZ, 'orig', A, 'NFS.ELF')}
OFF = 0xFF000  # vram = offset de fichero + 0xFF000 en las dos PS2
CODIGO = ('asmtu', 'asm', 'c', 'cpp')
rx = re.compile(r'^\s*(\S+)\s*=\s*0x([0-9A-Fa-f]+)\s*;(.*)$')
RE_NM = re.compile(r'^nonmatching (\S+), 0x([0-9A-Fa-f]+)')
RE_INS = re.compile(r'^\s*/\*\s*([0-9A-F]+)\s+([0-9A-F]{8})\s+([0-9A-F]{8})\s*\*/')
SIN_NOMBRE = re.compile(r'^(fn|sub|func|lbl|zz|j_sub|nullsub)_[0-9A-Fa-f_]+$')
_elf = {}


def lee(V, vram, n):
    if V not in _elf:
        _elf[V] = open(ELF[V], 'rb').read()
    o = vram - OFF
    return _elf[V][o:o + n]


def mismo_codigo(a, b):
    """dos trozos de codigo MIPS iguales salvo reubicaciones: jal/j por codigo de operacion; lui por codigo y registro;
    una instruccion de inmediato (addiu, lw, sw...) por codigo y registros SOLO si su base es un registro cargado con
    lui antes (el %lo de una direccion) o $gp; el resto, palabra exacta (los desplazamientos de campo cuentan: los
    accesores de 8 bytes solo se distinguen por ellos). Todo ceros no prueba nada."""
    if len(a) != len(b) or not a.strip(b'\0'):
        return False
    n = len(a) // 4
    lui = set()
    for x, y in zip(struct.unpack('<%dI' % n, a), struct.unpack('<%dI' % n, b)):
        op = x >> 26
        if x == y:
            if op == 0x0F:
                lui.add((x >> 16) & 31)
            continue
        if op != y >> 26 or op == 0:
            return False
        if op in (2, 3):
            continue
        if op == 0x0F:
            if (x >> 16) & 31 != (y >> 16) & 31:
                return False
            lui.add((x >> 16) & 31)
            continue
        if (x >> 16) != (y >> 16):
            return False
        if (x >> 21) & 31 not in lui and (x >> 21) & 31 != 28:
            return False
    return True


def simbolos(V):
    out = []
    for l in open(os.path.join(RAIZ, 'config', V, 'symbols.txt'), encoding='utf-8'):
        m = rx.match(l)
        if m and 'type:func' in m.group(3):
            out.append((int(m.group(2), 16), m.group(1)))
    out.sort()
    return out


def subsegmentos(V):
    d = yaml.safe_load(open(os.path.join(RAIZ, 'config', V, 'config.yml'), encoding='utf-8'))
    subs = []
    for s in d['segments'][0]['subsegments']:
        if isinstance(s, list):
            subs.append((s[0], s[1] if len(s) > 1 else None, s[2] if len(s) > 2 else None))
        else:
            subs.append((s.get('start'), s.get('type'), s.get('name')))
    return subs


def unidades(V):
    subs = subsegmentos(V)
    out = []
    for i, (st, ty, nm) in enumerate(subs):
        if ty in CODIGO:
            fin = subs[i + 1][0] if i + 1 < len(subs) else None
            out.append((st + OFF, fin + OFF if fin is not None else None, nm))
    return out


def funciones_asm(V):
    """[(ini, tam, nombre)] de todas las funciones del troceado actual (con las func_ de splat)"""
    cfg = json.load(open(os.path.join(RAIZ, 'build', V, 'config.json'), encoding='utf-8'))
    out = []
    for u in cfg['units']:
        p = os.path.join(RAIZ, 'build', V, 'asm', u['name'] + '.s')
        if not os.path.exists(p):
            continue
        pend = None
        for l in open(p, encoding='utf-8', errors='replace'):
            m = RE_NM.match(l)
            if m:
                pend = [m.group(1), int(m.group(2), 16)]
                continue
            if pend:
                m = RE_INS.match(l)
                if m:
                    out.append((int(m.group(2), 16), pend[1], pend[0]))
                    pend = None
    out.sort()
    return out


def reparte(hueco, entre, ini_a):
    """trozos (desplazamiento, unidad) si el hueco es, en orden, el final de cada unidad de `entre` (o de parte de
    ellas; las que no dejan nada se saltan); None si no"""
    memo = {}

    def rec(p, k):
        if p == len(hueco):
            return []
        if k == len(entre) or (p, k) in memo:
            return None
        for n in range(8, len(hueco) - p + 1, 8):
            ust, ufin = ini_a[entre[k]]
            if n <= ufin - ust and lee(A, ufin - n, n) == hueco[p:p + n]:
                r = rec(p + n, k + 1)
                if r is not None:
                    return [(p, entre[k])] + r
        r = rec(p, k + 1)
        memo[(p, k)] = r
        return r
    return rec(0, 0)


def cortes(filtro=None, avisa=print):
    ua = unidades(A)
    orden_a = {nm: i for i, (st, fin, nm) in enumerate(ua)}
    por_orden = [nm for st, fin, nm in ua]
    ini_a = {nm: (st, fin) for st, fin, nm in ua}
    sa = simbolos(A)
    sa_set = set(sa)
    cuenta = collections.Counter(n for a, n in sa)
    a_de = {}
    for a, n in sa:
        if cuenta[n] != 1:
            continue
        for st, fin, nm in ua:
            if st <= a and (fin is None or a < fin):
                a_de[n] = (nm, a - st)
                break
    us = unidades(S)
    nombres_s = {nm for st, fin, nm in us}
    fa = funciones_asm(S)
    nuevos = []  # (offset de fichero, nombre, nota)
    mover = []   # (offset viejo, offset nuevo, nombre)
    for st, fin, nm in us:
        if nm.startswith('Speed/Indep/SourceLists/'):
            continue
        if filtro and not re.search(filtro, nm):
            continue
        fns = [(a, t, n) for a, t, n in fa if st <= a and (fin is None or a < fin)]
        if not fns:
            continue
        carreras = []   # [clave, [(a, t, n)], desfase de la primera con nombre]
        for a, t, n in fns:
            u = a_de.get(n) if not SIN_NOMBRE.match(n) else None
            clave = u[0] if u else None
            if carreras and (clave is None or clave == carreras[-1][0] or carreras[-1][0] is None and clave == nm):
                if carreras[-1][0] is None and clave is not None:
                    carreras[-1][0], carreras[-1][2] = clave, u[1]
                carreras[-1][1].append((a, t, n))
            else:
                carreras.append([clave, [(a, t, n)], u[1] if u else 0])
        # el principio de la propia unidad: la primera funcion con nombre y su desfase en la A124
        c0 = carreras[0]
        if c0[0] == nm and nm in ini_a:
            pn = next((x for x in c0[1] if not SIN_NOMBRE.match(x[2])), None)
            k, D = (pn[0] - st, c0[2]) if pn else (0, 0)
            if pn and k < D:
                ua0 = ini_a[nm][0]
                antes = [x for x in fa if pn[0] - D <= x[0] < st]
                if mismo_codigo(lee(S, pn[0] - D, D), lee(A, ua0, D)) and all(SIN_NOMBRE.match(x[2]) for x in antes):
                    mover.append((st - OFF, pn[0] - D - OFF, nm))
                elif k and mismo_codigo(lee(S, st, k), lee(A, ua0 + D - k, k)) or not k:
                    pass    # le falta el principio del objeto (quitado por el enlazador): empieza donde debe
                else:
                    avisa('AVISO %s: empieza %d B antes de su primera funcion y en la A124 %d; no cuadra' % (nm, k, D))
        if len(carreras) < 2:
            continue
        if carreras[0][0] not in (None, nm):
            avisa('AVISO %s: su primera funcion %s es de %s en la A124; no se toca' % (nm, carreras[0][1][0][2],
                                                                                     carreras[0][0]))
            continue
        ords = [orden_a[c[0]] for c in carreras if c[0] in orden_a]
        if ords != sorted(ords):
            avisa('AVISO %s: las unidades de la A124 no van en el mismo orden en SLUS (%s)' % (
                nm, ' '.join(os.path.basename(c[0]) for c in carreras if c[0])))
        previa = carreras[0]
        for clave, lst, desf in carreras[1:]:
            if clave == nm:
                previa = [clave, lst, desf]
                continue
            if clave in nombres_s:
                avisa('AVISO %s ya es una unidad de SLUS: no se crea otra en %x' % (clave, lst[0][0]))
                previa = [clave, lst, desf]
                continue
            a0 = lst[0][0]
            ini = a0
            if desf:
                ua0 = ini_a[clave][0]
                antes = [x for x in previa[1] if x[0] >= a0 - desf]
                # las funciones con nombre de en medio tienen que tener ese nombre en la A124 en el mismo sitio
                # (una estatica repetida como setD4_CHCR de ipuinit.c)
                if mismo_codigo(lee(S, a0 - desf, desf), lee(A, ua0, desf)) and \
                        all(SIN_NOMBRE.match(x[2]) or (ua0 + x[0] - (a0 - desf), x[2]) in sa_set for x in antes):
                    ini = a0 - desf
                elif lee(S, a0 - desf, desf).strip(b'\0'):
                    avisa('AVISO %s: los %d B de la A124 delante de %s no estan en SLUS; corte en la funcion'
                          % (clave, desf, lst[0][2]))
            # hueco entre la ultima funcion con nombre de la carrera previa y esta
            ult = [x for x in previa[1] if not SIN_NOMBRE.match(x[2]) and x[0] < ini]
            if ult and previa[0] in orden_a and clave in orden_a:
                finprev = (ult[-1][0] + ult[-1][1] + 7) & ~7
                hueco = lee(S, finprev, ini - finprev) if finprev < ini else b''
                if hueco.strip(b'\0'):
                    i0, i1 = orden_a[previa[0]], orden_a[clave]
                    entre = por_orden[i0 + 1:i1] if i0 < i1 else []
                    pst, pfin = ini_a[previa[0]]
                    if lee(A, pfin - len(hueco), len(hueco)) == hueco:
                        pass    # el mismo final que la unidad anterior en la A124: es suyo
                    else:
                        r = reparte(hueco, entre, ini_a) if entre else None
                        if r:
                            for p, u in r:
                                if u in nombres_s:
                                    avisa('AVISO resto de %s en %x, pero SLUS ya tiene esa unidad' % (u, finprev + p))
                                else:
                                    nuevos.append((finprev + p - OFF, u, 'resto'))
                        else:
                            avisa('hueco de %d B en %x (antes de %s; %d unidades de la A124 en medio) sin dueno'
                                  % (len(hueco), finprev, os.path.basename(clave), len(entre)))
            nuevos.append((ini - OFF, clave, '%d fn: %s' % (len(lst), ', '.join(n for a, t, n in lst)[:120])))
            previa = [clave, lst, desf]
    return nuevos, mover


def aplica(nuevos, mover, destino):
    p = os.path.join(RAIZ, 'config', S, 'config.yml')
    t = open(p, 'rb').read().decode('utf-8')
    assert '\r\n' not in t
    lineas = t.split('\n')
    rxl = re.compile(r'^    - \[0x([0-9a-fA-F]+), (asmtu|asm|c|cpp), ([^\]]+)\]\s*$')
    pos = []
    for i, l in enumerate(lineas):
        m = rxl.match(l)
        if m:
            pos.append((int(m.group(1), 16), i, m.group(3)))
    ya = {o for o, i, n in pos}
    for viejo, nuevo, nm in mover:
        i = next(i for o, i, n in pos if o == viejo and n == nm)
        lineas[i] = '    - [0x%06x, asmtu, %s]' % (nuevo, nm)
    inserta = collections.defaultdict(list)
    for off, nm, nota in sorted(nuevos):
        if off in ya:
            print('ya hay un corte en %x: %s no se anade' % (off, nm))
            continue
        prev = max((o, i) for o, i, n in pos if o < off)
        inserta[prev[1]].append('    - [0x%06x, asmtu, %s]' % (off, nm))
    out = []
    for i, l in enumerate(lineas):
        out.append(l)
        out.extend(inserta.get(i, []))
    entero(destino, 'wb').write('\n'.join(out).encode('utf-8'))
    print('escrito %s (%d cortes nuevos, %d movidos)' % (destino, sum(len(v) for v in inserta.values()), len(mover)))


def main():
    filtro = sys.argv[sys.argv.index('--unidad') + 1] if '--unidad' in sys.argv else None
    nuevos, mover = cortes(filtro)
    for viejo, nuevo, nm in mover:
        print('  MOVER %s de %x a %x' % (nm, viejo, nuevo))
    for off, nm, nota in sorted(nuevos):
        print('  [0x%06x, asmtu, %s]  # %s' % (off, nm, nota))
    print('%d cortes, %d movidos' % (len(nuevos), len(mover)))
    if '--yml' in sys.argv:
        aplica(nuevos, mover, sys.argv[sys.argv.index('--yml') + 1])
    elif '--aplicar' in sys.argv:
        aplica(nuevos, mover, os.path.join(RAIZ, 'config', S, 'config.yml'))


if __name__ == '__main__':
    main()
