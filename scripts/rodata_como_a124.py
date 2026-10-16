#!/usr/bin/env python3
"""rodata_como_a124.py -- fronteras de .rodata de SLUS por CONTENIDO: busca la .rodata de cada unidad de la A124 dentro de
un trozo anonimo de la .rodata de SLUS.

La A124 trae la .rodata partida por objeto (su .mdebug); en SLUS las bibliotecas (vp6, csis, realgraph, realcore, libc,
libm...) caen en un trozo `[0x409570, .rodata]` sin nombre. Los comienzos que votaban las referencias de las funciones
(config_6) no dan los finales. Aqui cada unidad de la A124 se convierte en un patron de palabras: las que apuntan dentro
del programa de la A124 (vtables, tablas de saltos) son comodines, que en SLUS la direccion es otra; el resto (cadenas,
constantes, ceros) tiene que ser igual; los ceros del final son relleno y no cuentan.

  1. ANCLAS: las unidades cuyo patron casa en UN solo sitio de SLUS (alineado a 4). Las de menos de 16 bytes con
     contenido no se buscan solas: no prueban nada.
  2. VECINAS: desde cada ancla, hacia delante y hacia atras, la unidad siguiente (anterior) en el orden de la A124 va
     justo detras (delante), a la distancia que marca el tamano de la A124, si su patron casa ALLI. Asi se colocan las
     que se repiten (la cadena "Assert: %s:%i (%s)" de cada unidad de realgraph) y las pequenas.
  3. Lo que queda entre dos unidades colocadas es un trozo anonimo.

    python scripts/rodata_como_a124.py [--desde VRAM_SLUS] [--hasta VRAM_SLUS] [--unidad REGEX] [--base YML] [--yml SALIDA]

--base: el config.yml de SLUS que da el orden del .text (y sobre el que se escribe); por defecto el del arbol.

Por defecto analiza el primer trozo anonimo de .rodata de SLUS. Con --yml escribe una copia de config/SLUS-21351/config.yml
con los cortes (para scripts/trocea_prueba.py).
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from trocea_como_a124 import lee, subsegmentos, OFF, A, S  # noqa: E402
from escritura import entero  # noqa: E402

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MIN_CONTENIDO = 16


def subsegmentos_de(yml):
    import yaml
    d = yaml.safe_load(open(yml, encoding='utf-8'))
    subs = []
    for s in d['segments'][0]['subsegments']:
        if isinstance(s, list):
            subs.append((s[0], s[1] if len(s) > 1 else None, s[2] if len(s) > 2 else None))
        else:
            subs.append((s.get('start'), s.get('type'), s.get('name')))
    return subs


def rodata_units(V):
    subs = subsegmentos(V)
    out = []
    for i, (st, ty, nm) in enumerate(subs):
        if ty == '.rodata':
            fin = subs[i + 1][0] if i + 1 < len(subs) else None
            out.append((st + OFF, fin + OFF if fin is not None else None, nm))
    return out


def es_puntero_a124(w):
    return 0x100000 <= w < 0x6A0000


def patron(b):
    n = len(b) // 4
    ws = struct.unpack('<%dI' % n, b[:n * 4])
    pat = [None if es_puntero_a124(w) else w for w in ws]
    while pat and pat[-1] == 0:
        pat.pop()
    return pat


def casa_en(pat, region, base, pos):
    i = (pos - base) // 4
    if i < 0 or (pos - base) % 4 or i + len(pat) > len(region) // 4:
        return False
    ws = struct.unpack_from('<%dI' % len(pat), region, 4 * i)
    return all(w is None or w == x for w, x in zip(pat, ws))


def busca(pat, region, base):
    n = len(region) // 4
    ws = struct.unpack('<%dI' % n, region[:n * 4])
    primero = next((i for i, w in enumerate(pat) if w is not None), None)
    if primero is None:
        return []
    res = []
    for i in range(0, n - len(pat) + 1):
        if ws[i + primero] == pat[primero] and all(w is None or ws[i + j] == w for j, w in enumerate(pat)):
            res.append(base + 4 * i)
    return res


def coloca(desde, hasta, filtro=None, avisa=print, subs_s=None):
    region = lee(S, desde, hasta - desde)
    au = [(st, fin, nm) for st, fin, nm in rodata_units(A)
          if nm and fin and not nm.startswith('Speed/Indep/SourceLists/') and (not filtro or re.search(filtro, nm))]
    info = []
    for st, fin, nm in au:
        pat = patron(lee(A, st, fin - st))
        info.append(dict(nm=nm, st=st, tam=fin - st, pat=pat, cont=sum(4 for w in pat if w)))
    # rangos de .text de cada unidad de SLUS (del config que da el orden) para las tablas sin contenido
    rango_s = {}
    if subs_s is not None:
        for i, (st, ty, nm) in enumerate(subs_s):
            if ty in ('asmtu', 'asm', 'c', 'cpp') and nm and i + 1 < len(subs_s):
                rango_s[nm] = (st + OFF, subs_s[i + 1][0] + OFF)
    texto = (0x100000, 0x4BE880)   # .text de SLUS

    def vale(j, q):
        """el patron j casa en q; si no tiene contenido propio (vtable, tabla de saltos), sus punteros de SLUS
        tienen que ir todos al .text y alguno al .text de la propia unidad"""
        u = info[j]
        if not u['pat'] or not casa_en(u['pat'], region, desde, q):
            return False
        if u['cont'] >= 4:
            return True
        r = rango_s.get(u['nm'])
        if not r:
            return False
        i0 = (q - desde) // 4
        ws = struct.unpack_from('<%dI' % len(u['pat']), region, 4 * i0)
        ptrs = [x for w, x in zip(u['pat'], ws) if w is None]
        return bool(ptrs) and all(texto[0] <= x < texto[1] for x in ptrs) and any(r[0] <= x < r[1] for x in ptrs)

    puesto = {}   # indice en au -> posicion en SLUS
    for k, u in enumerate(info):
        if u['cont'] < MIN_CONTENIDO:
            continue
        pos = busca(u['pat'], region, desde)
        if len(pos) == 1:
            puesto[k] = pos[0]
    anclas = sorted(puesto.items())
    # vecinas en el orden de la A124, a la distancia del tamano de la A124
    cambio = True
    while cambio:
        cambio = False
        for k, p in sorted(puesto.items()):
            for d in (1, -1):
                j = k + d
                if j < 0 or j >= len(info) or j in puesto:
                    continue
                pj = p + info[k]['tam'] if d == 1 else p - info[j]['tam']
                if not (desde <= pj < hasta) or pj in puesto.values():
                    continue
                if vale(j, pj):
                    puesto[j] = pj
                    cambio = True
    # orden de enlace de SLUS: la .rodata va en el orden del .text, que en SLUS no siempre es el de la A124 (realcore
    # input, la libkernel). Detras de cada unidad colocada, la siguiente en el .text de SLUS que tenga .rodata en la
    # A124, si su patron casa entre su final y 24 bytes mas alla (relleno de alineacion).
    if subs_s is not None:
        orden_s = [nm for st, ty, nm in subs_s if ty in ('asmtu', 'asm', 'c', 'cpp')]
        idx = {u['nm']: k for k, u in enumerate(info)}
        cambio = True
        while cambio:
            cambio = False
            ocupado = sorted((p, p + 4 * len(info[k]['pat'])) for k, p in puesto.items())
            for k, p in sorted(puesto.items(), key=lambda x: x[1]):
                nm = info[k]['nm']
                if nm not in orden_s:
                    continue
                i = orden_s.index(nm) + 1
                while i < len(orden_s) and (orden_s[i] not in idx or not info[idx[orden_s[i]]]['pat']):
                    i += 1
                if i >= len(orden_s):
                    continue
                j = idx[orden_s[i]]
                if j in puesto:
                    continue
                fin_p = (p + 4 * len(info[k]['pat']) + 3) & ~3
                sitios = [q for q in range(fin_p, fin_p + 28, 4) if vale(j, q)
                          and not any(a < q + 4 * len(info[j]['pat']) and q < b for a, b in ocupado)]
                if len(sitios) == 1:
                    puesto[j] = sitios[0]
                    cambio = True
                    break
    fuera = sorted((p, k) for k, p in puesto.items())
    res = []
    for p, k in fuera:
        u = info[k]
        fin_u = p + 4 * len(u['pat'])
        res.append((p, fin_u, u['nm'], k in dict(anclas)))
    for (a, fa, na, x), (b, fb, nb, y) in zip(res, res[1:]):
        if fa > b:
            avisa('SOLAPE %s [%x, %x) con %s [%x)' % (na, a, fa, nb, b))
    return res


def main():
    su = rodata_units(S)
    anon = [(st, fin) for st, fin, nm in su if nm is None]
    desde = int(sys.argv[sys.argv.index('--desde') + 1], 16) if '--desde' in sys.argv else anon[0][0]
    hasta = int(sys.argv[sys.argv.index('--hasta') + 1], 16) if '--hasta' in sys.argv else anon[0][1]
    filtro = sys.argv[sys.argv.index('--unidad') + 1] if '--unidad' in sys.argv else None
    print('SLUS [%x, %x) %d B' % (desde, hasta, hasta - desde))
    base = sys.argv[sys.argv.index('--base') + 1] if '--base' in sys.argv else None
    subs_s = subsegmentos_de(base) if base else subsegmentos(S)
    res = coloca(desde, hasta, filtro, subs_s=subs_s)
    nombres_s = {nm for st, fin, nm in su if nm}
    for p, f, nm, ancla in res:
        print('%s %-70s SLUS [%x, %x) off %x%s' % ('A' if ancla else 'v', nm[-70:], p, f, p - OFF,
                                                  '  (YA EN SLUS)' if nm in nombres_s else ''))
    print('%d unidades (%d anclas)' % (len(res), sum(1 for r in res if r[3])))
    if '--yml' in sys.argv:
        escribe(res, desde, hasta, nombres_s, sys.argv[sys.argv.index('--yml') + 1], base)


def escribe(res, desde, hasta, nombres_s, destino, origen=None):
    p = origen or os.path.join(RAIZ, 'config', S, 'config.yml')
    t = open(p, 'rb').read().decode('utf-8')
    assert '\r\n' not in t
    ancla = '    - [0x%06x, .rodata]\n' % (desde - OFF)
    assert t.count(ancla) == 1, ancla
    lineas = []
    cur = desde
    for a, fa, nm, x in res:
        if nm in nombres_s or a < cur:
            continue
        if a > cur:
            lineas.append('    - [0x%06x, .rodata]\n' % (cur - OFF))
        lineas.append('    - [0x%06x, .rodata, %s]\n' % (a - OFF, nm))
        cur = (fa + 3) & ~3
    if cur < hasta:
        lineas.append('    - [0x%06x, .rodata]\n' % (cur - OFF))
    nota = ('    # .rodata de las bibliotecas por CONTENIDO (scripts/rodata_como_a124.py): la de cada unidad de la A124,\n'
            '    # con los punteros como comodines, casa en un solo sitio de SLUS o justo al lado de una que ya casa;\n'
            '    # lo que no se prueba se queda sin nombre.\n')
    t = t.replace(ancla, nota + ''.join(lineas))
    entero(destino, 'wb').write(t.encode('utf-8'))
    print('escrito', destino, len(lineas), 'lineas')


if __name__ == '__main__':
    main()
