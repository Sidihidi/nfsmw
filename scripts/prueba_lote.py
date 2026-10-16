#!/usr/bin/env python3
"""prueba_lote.py -- scripts/prueba_nombre.py EN LOTE: para cada (direccion, nombre) de una lista compara la funcion del
troceado que empieza en esa direccion con NUESTRA funcion de ese nombre en el objeto de la misma unidad, palabra a
palabra (en las palabras reubicadas, codigo de operacion y registros en las de tipo I y solo el codigo en j/jal; si
los dos destinos de un jal tienen nombre, tienen que ser el mismo).

    python scripts/prueba_lote.py V lista.tsv [--salida resultado.tsv]

    V       SLUS-21351 o SLES-53558-A124
    lista   filas `0xDIRECCION<TAB>nombre[<TAB>...]` (la forma que acepta scripts/aplica_nombres.py)

Escribe por fila: direccion, nombre, resultado y el glabel actual de esa direccion. Resultados: IGUAL, `N palabras
distintas`, `tamano A/B` (el original mide A y la nuestra B), `sin glabel` (la direccion no es el principio de una
funcion del troceado), `no troceada` y `no esta` (nuestro objeto no define ese nombre). Al final, por stderr, la
cuenta de cada resultado.

IGUAL no prueba el nombre si otra funcion nuestra de la unidad tambien es IGUAL (codigo identico en dos clases): hace
falta otra ancla (ranura de vtable: scripts/vtslots.py; cadena: scripts/tipos_cadena.py y scripts/getkind.py; grafo
de llamadas: scripts/grafo_nombres.py).

Lee el troceado y los objetos de la raiz donde este el guion (build/<V>/asm, obj y src): copiado a una raiz de prueba
(un troceado hecho fuera del arbol), prueba ese troceado. Tambien sirve como modulo:

    import prueba_lote
    pr = prueba_lote.Probador('SLUS-21351')
    resultado, glabel, unidad = pr.prueba(0x15c908, 'bRandom__Fi')
"""
import collections
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
import prueba_nombre as pn  # noqa: E402

RE_INS = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s')


class Probador:
    """Indice de glabels del troceado actual de V y cache de las funciones de los objetos (original y nuestro)."""

    def __init__(self, V):
        self.V = V
        self.glabel = {}          # direccion -> (glabel, unidad)
        for p, unidad in pn.ficheros_asm(V):
            pendiente = None
            for l in open(p, encoding='utf-8', errors='replace'):
                g = re.match(r'\s*glabel (\S+)', l)
                if g:
                    pendiente = g.group(1)
                    continue
                m = RE_INS.match(l)
                if m and pendiente:
                    self.glabel[int(m.group(1), 16)] = (pendiente, unidad)
                    pendiente = None
        self.cache = {}

    def objs(self, unidad):
        """(funciones del original troceado, funciones nuestras) de la unidad: {nombre: [(bytes, reubicaciones)]}"""
        if unidad not in self.cache:
            base = os.path.splitext(unidad)[0] if unidad.endswith(('.cpp', '.c')) else unidad
            try:
                t = pn.funciones(os.path.join(RAIZ, 'build', self.V, 'obj', base + '.o'))
            except (OSError, ValueError):
                t = {}
            try:
                o = pn.funciones(os.path.join(RAIZ, 'build', self.V, 'src', base + '.o'))
            except (OSError, ValueError):
                o = {}
            self.cache[unidad] = (t, o)
        return self.cache[unidad]

    def prueba(self, x, nombre):
        """(resultado, glabel actual, unidad) de comparar la funcion que empieza en x con nuestra `nombre`"""
        if x not in self.glabel:
            return 'sin glabel', None, None
        g, unidad = self.glabel[x]
        t, o = self.objs(unidad)
        if g not in t:
            return 'no troceada', g, unidad
        if nombre not in o:
            return 'no esta', g, unidad
        res = []
        for b, rel in o[nombre]:
            if len(b) != len(t[g][0][0]):
                res.append('tamano %d/%d' % (len(t[g][0][0]), len(b)))
                continue
            r, dif = pn.compara(t[g][0], (b, rel))
            res.append(r)
        return ('IGUAL' if 'IGUAL' in res else res[0]), g, unidad


def main():
    args = [a for i, a in enumerate(sys.argv[1:], 1) if not a.startswith('--') and sys.argv[i - 1] != '--salida']
    if len(args) < 2:
        sys.exit(__doc__)
    V, lista = args[0], args[1]
    pr = Probador(V)
    out = []
    cuenta = collections.Counter()
    for l in open(lista, encoding='utf-8'):
        c = l.rstrip('\n').split('\t')
        if len(c) < 2 or not c[0].startswith('0x'):
            continue
        r, g, u = pr.prueba(int(c[0], 16), c[1])
        cuenta['distintas' if r[0].isdigit() else r.split()[0] if r.startswith('tamano') else r] += 1
        out.append('%s\t%s\t%s\t%s' % (c[0], c[1], r, g))
    if '--salida' in sys.argv:
        with open(sys.argv[sys.argv.index('--salida') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            fh.write('\n'.join(out) + '\n')
    else:
        print('\n'.join(out))
    print(dict(cuenta), file=sys.stderr)


if __name__ == '__main__':
    main()
