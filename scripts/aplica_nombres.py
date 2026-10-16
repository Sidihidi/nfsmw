#!/usr/bin/env python3
"""Aplica a un symbols.txt de PS2 una o varias listas de nombres PROBADOS (direccion, nombre) y comprueba que el lote es
coherente antes de escribir nada.

    python scripts/aplica_nombres.py V lista.tsv [lista2.tsv...] [--entrada sym.txt] [--salida sym.txt] [--aplicar]
          [--comentario "texto"]

Cada fila: 0xDIRECCION<TAB>nombre<TAB>motivo[<TAB>...]. motivo `repe` (o `static`) marca un estatico que se repite en
varias unidades (`__static_initialization_and_destruction_0`...): lleva allow_duplicated:true en TODAS sus lineas. Los
nombres que ya llevan allow_duplicated en symbols.txt (MUTEX_lock__FP5MUTEX de cada SourceList...) cuentan como repe.
  - una direccion con dos nombres distintos entre las listas: se descartan las dos (se informa);
  - un nombre (no repe) para dos direcciones: se descartan;
  - si la direccion ya tiene linea `type:func`, se cambia el nombre y se conserva el resto (salvo filename:);
  - si no, se anade `nombre = 0xDIR; // type:func` al final, en un bloque con el comentario;
  - si el nombre esta en OTRA direccion que no recibe nombre en el lote, esa linea se borra (el nombre se mueve y la
    direccion vieja vuelve a ser func_; si el nombre de alli era bueno, el lote lo habria dicho);
  - un nombre de mas de 250 caracteres lleva filename:func_XXXXXXXX (splat escribe un fichero por simbolo).
Por defecto lee y escribe config/<V>/symbols.txt; sin --aplicar solo informa. Conserva LF. Despues hay que trocear y
medir (scripts/medir.py) y pasar scripts/cortes.py, scripts/restos_epilogo.py y scripts/restos_trozos.py.
"""
import collections
import os
import re
import sys
from escritura import entero

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    args = [a for i, a in enumerate(sys.argv[1:], 1)
            if not a.startswith('--') and sys.argv[i - 1] not in ('--entrada', '--salida', '--comentario')]
    V, listas = args[0], args[1:]

    def opt(n, d=None):
        return sys.argv[sys.argv.index(n) + 1] if n in sys.argv else d
    entrada = opt('--entrada', os.path.join(RAIZ, 'config', V, 'symbols.txt'))
    salida = opt('--salida', entrada)
    por_dir = collections.defaultdict(set)
    repes = set()
    for p in listas:
        for l in open(p, encoding='utf-8'):
            c = l.rstrip('\n').split('\t')
            if len(c) < 2 or not c[0].startswith('0x'):
                continue
            por_dir[int(c[0], 16)].add(c[1])
            if len(c) >= 3 and c[2] in ('repe', 'static'):
                repes.add(c[1])
    raw = open(entrada, 'rb').read().decode('utf-8')
    assert '\r\n' not in raw, 'symbols.txt con CRLF'
    lineas = raw.split('\n')
    # los nombres que ya llevan allow_duplicated (MUTEX_lock__FP5MUTEX de cada SourceList, __tcf_0...) son
    # estaticos repetidos: nunca se borran de otras unidades
    todos = {n for ns in por_dir.values() for n in ns}
    for l in lineas:
        m = re.match(r'(\S+) = 0x[0-9a-fA-F]+;.*allow_duplicated', l)
        if m and m.group(1) in todos:
            repes.add(m.group(1))
    cambios = {}
    for a, ns in por_dir.items():
        if len(ns) > 1:
            print('  CONFLICTO 0x%x: %s (se descarta)' % (a, ' / '.join(sorted(ns))))
            continue
        cambios[a] = next(iter(ns))
    uso = collections.Counter(n for n in cambios.values() if n not in repes)
    for a, n in list(cambios.items()):
        if n not in repes and uso[n] > 1:
            print('  CONFLICTO %s en %d direcciones (se descarta)' % (n, uso[n]))
            del cambios[a]
    nuevos = set(cambios.values()) - repes
    hechas, borrar = set(), []
    n_cambia = n_repe = 0
    for i, l in enumerate(lineas):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);(.*)$', l)
        if not m:
            continue
        n, a, resto = m.group(1), int(m.group(2), 16), m.group(3)
        if a in cambios and 'type:func' in resto:
            if n != cambios[a]:
                resto = re.sub(r' filename:\S+', '', resto)
                if len(cambios[a]) > 250:
                    resto += ' filename:func_%08X' % a
                if cambios[a] in repes and 'allow_duplicated' not in resto:
                    resto += ' allow_duplicated:true'
                lineas[i] = '%s = 0x%x;%s' % (cambios[a], a, resto)
                n_cambia += 1
            hechas.add(a)
        elif n in repes and a not in cambios and 'allow_duplicated' not in resto:
            lineas[i] = l.rstrip() + ' allow_duplicated:true'
            n_repe += 1
        elif n in nuevos and a not in cambios:
            borrar.append(i)
    for i in reversed(borrar):
        print('  borra  %s' % lineas[i][:100])
        del lineas[i]
    anadir = []
    for a, n in sorted(cambios.items()):
        if a in hechas:
            continue
        extra = ' filename:func_%08X' % a if len(n) > 250 else ''
        extra += ' allow_duplicated:true' if n in repes else ''
        anadir.append('%s = 0x%x; // type:func%s' % (n, a, extra))
    print('%s: %d nombres en el lote; %d lineas cambian, %d se anaden, %d se borran, %d repes marcados' % (
        V, len(cambios), n_cambia, len(anadir), len(borrar), n_repe))
    if '--aplicar' not in sys.argv:
        return
    while lineas and not lineas[-1].strip():
        lineas.pop()
    if anadir:
        com = opt('--comentario', 'nombres probados')
        lineas += [''] + ['// ' + x for x in com.split('|')] + anadir
    lineas.append('')
    entero(salida, 'wb').write('\n'.join(lineas).encode('utf-8'))
    print('escrito', salida)


if __name__ == '__main__':
    main()
