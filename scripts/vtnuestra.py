#!/usr/bin/env python3
"""vtnuestra.py -- nombres de SLUS por la ranura de NUESTRA vtable (la que compila nuestro objeto de SLUS, que sale
de las cabeceras que casan con GameCube), para punteros de datos a funciones sin nombre.

Para cada puntero P de SLUS a una entrada E (scripts/entradas_sin_nombre.py --tsv), las palabras vecinas de P que
apuntan a funciones CON NOMBRE en SLUS votan por (vtable nuestra V, ranura s) tales que V[k] es ese nombre en la
posicion k = s + j. Se verifica la mejor candidata ranura a ranura: acuerdos (la palabra de SLUS es la direccion del
nombre de V[k]) y desacuerdos (apunta a OTRA funcion con nombre). Se propone V[s] si hay al menos 3 acuerdos, ningun
desacuerdo y una sola candidata con ese maximo; y se compara el cuerpo de E con nuestra funcion V[s].

    python scripts/vtnuestra.py ENTRADAS.tsv [--tsv propuestas.tsv]

A diferencia de scripts/vtslots.py no exige que la A124 diga lo mismo: la A124 es un prototipo y sus vtables a veces
tienen otro orden (CameraMover: GetTarget en la ranura 0x8c, donde la de venta tiene IsHoodCamera y GetTarget en la
0x94). Tanda 6: 30 propuestas, todas con codigo IGUAL, y la cadena de destructores SFXCTL_3D*Pos corridos una clase.
"""
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
import vtslots as VT  # noqa: E402
import prueba_lote  # noqa: E402
import prueba_nombre as pn  # noqa: E402
from vtable_dtors import Imagen, ELFS  # noqa: E402

S = 'SLUS-21351'
SIN = re.compile(r'^(func|D|fuera)_')


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('-'):
        sys.exit(__doc__)
    lista = sys.argv[1]
    img = Imagen(os.path.join(RAIZ, ELFS[S]))
    ps_dir, ps_nom, _ = VT.simbolos(S)
    nuestras = VT.nuestras_vtables()
    por_nombre = collections.defaultdict(list)          # nombre -> [(vtable, ranura)]
    vt = {}
    for v, lst in nuestras.items():
        d = dict(lst)
        vt[v] = d
        for k, n in d.items():
            if n:
                por_nombre[n].append((v, k))
    pr = prueba_lote.Probador(S)

    def nombre_de(w):
        ns = [n for n in ps_dir.get(w, []) if not SIN.match(n)]
        return ns[0] if ns else None
    filas = []
    for l in open(lista, encoding='utf-8'):
        c = l.rstrip('\n').split('\t')
        if len(c) < 6:
            continue
        E, g, unidad = int(c[0], 16), c[1], c[2]
        ptrs = [int(m, 16) for m in re.findall(r'dato@(0x[0-9a-f]+)', c[5])]
        if not ptrs:
            continue
        propuestas = collections.Counter()
        detalle = {}
        for P in ptrs:
            votos = collections.Counter()
            for j in range(-48, 49):
                if j == 0:
                    continue
                try:
                    w, = struct.unpack('<I', img.lee(P + 4 * j, 4))
                except (ValueError, struct.error):
                    continue
                n = nombre_de(w)
                if not n:
                    continue
                for v, k in por_nombre.get(n, []):
                    votos[(v, k - j)] += 1
            mejores = []
            for (v, s), k in votos.most_common(12):
                if s not in vt[v] or not vt[v][s]:
                    continue
                acuerdo = desacuerdo = 0
                for kk, nn in vt[v].items():
                    if kk == s or not nn:
                        continue
                    try:
                        w, = struct.unpack('<I', img.lee(P + 4 * (kk - s), 4))
                    except (ValueError, struct.error):
                        desacuerdo += 1
                        continue
                    if nn in ps_nom and ps_nom[nn] == w:
                        acuerdo += 1
                    elif nombre_de(w):
                        desacuerdo += 1
                mejores.append((acuerdo, -desacuerdo, v, s))
            mejores.sort(reverse=True)
            if not mejores:
                continue
            a, d, v, s = mejores[0]
            empate = [m for m in mejores[1:] if m[0] == a and m[1] == d and vt[m[2]][m[3]] != vt[v][s]]
            if a >= 3 and d == 0 and not empate:
                propuestas[vt[v][s]] += 1
                detalle.setdefault(vt[v][s], []).append('%s+0x%x (%d/%d)' % (v, 4 * s, a, -d))
            else:
                detalle.setdefault('?', []).append('%s+0x%x %s (%d/%d)%s' % (v, 4 * s, vt[v][s], a, -d,
                                                                             ' EMPATE' if empate else ''))
        nombres = [n for n in propuestas]
        if len(nombres) == 1:
            n = nombres[0]
            # codigo del cuerpo contra el nuestro
            t, o = pr.objs(unidad)
            cod = 'no la tenemos'
            if g in t:
                tb, trel = t[g][0]
                x = max(a for a in pr.glabel if a <= E)
                d = E - x
                cuerpo = (tb[d:], [(r - d, q) for r, q in trel if r >= d])
                for ob in o.get(n, []):
                    cod = pn.compara(cuerpo, ob)[0] if len(ob[0]) == len(cuerpo[0]) else 'tamano %d/%d' % (
                        len(cuerpo[0]), len(ob[0]))
                    if cod == 'IGUAL':
                        break
            ya = ps_nom.get(n)
            filas.append((E, n, g, unidad, cod, '0x%x' % ya if ya is not None and ya != E else '', detalle[n]))
            print('0x%06x %-60s %-20s %s %s' % (E, n[:60], cod, ('YA EN 0x%x' % ya) if ya is not None and ya != E
                                                else '', '; '.join(detalle[n])[:120]))
        else:
            print('0x%06x %-60s %s' % (E, 'SIN DECIDIR ' + ','.join(nombres)[:48], '; '.join(
                sum(detalle.values(), []))[:160]))
    print('%d propuestas' % len(filas))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for E, n, g, unidad, cod, ya, det in filas:
                fh.write('0x%x\t%s\tvtable_nuestra\t%s\t%s\t%s\t%s\n' % (E, n, cod, ya, unidad, '; '.join(det)))


if __name__ == '__main__':
    main()
