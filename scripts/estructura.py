#!/usr/bin/env python3
"""estructura.py -- compara la ESTRUCTURA de stabs de una funcion (sus locales y parametros con la clase de
almacenamiento, y sus bloques) en nuestro .s de PS2 contra la del original: el .mdebug de la A124 (scripts/mdebug.py).

Lo que mira el compilador y no se ve en el codigo final: un parametro de inline que el original modifica (`a:r` en el
original contra nuestra `c:r` en bMin/bMax de PS2), un inline que el original llama y nosotros no (la `temp:r` de
bFloatToInt), una local que el original declara y nosotros no, el ambito de una declaracion (el `iter` de un `for`).
De aqui salieron FindHeliPoly y GetLoadingPriority (diario ps2a12475).

    python scripts/estructura.py FICHERO.s FUNC [FUNC...] [-q]
    python scripts/estructura.py FICHERO.s @lista.txt -q

    FICHERO.s  nuestro ensamblador con stabs: scripts/banco.py ... --asm=FICHERO.s (compila con -save-temps)
    FUNC       el nombre MANGLED (exacto, como en el .mdebug y en el informe)
    -q         una linea por funcion: tokens del original y del nuestro, diferencias de NOMBRE (+ nuestro, - original)
               y de BLOQUES, y las `netas` (las de nombre sin el orden: lo que de verdad falta o sobra). Sin -q, el
               diff entero de cada funcion.

Tokens: `nombre:clase` con clase r (registro), P (parametro en registro), m (memoria/pila), V/S/G... (estaticos y
globales, la letra del stab); `{` y `}` son los bloques ($LBB/$LBE en el original, stabn 192/224 en el nuestro).
Los nombres de una diferencia se leen con cuidado: el orden de los bloques de VU0 o de `_GLOBAL_` cambia sin que
cambie el codigo. scripts/stabs_unidades.py lo pasa por todas las casi exactas de unas unidades.
"""
import difflib
import os
import re
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mdebug  # noqa: E402


def tok(nombre, clase):
    c = clase[:1]
    c = 'r' if c == 'r' else ('P' if c in 'Pp' else ('m' if (c.isdigit() or c == '(') else c))
    return '%s:%s' % (nombre, c)


def corta(lista):
    """Hasta que el bloque mas externo se cierra y empieza otra cosa (un parametro P a profundidad 0)."""
    prof = 0
    visto = False
    for k, t in enumerate(lista):
        if t == '{':
            prof += 1
            visto = True
        elif t == '}':
            prof -= 1
        elif visto and prof == 0 and t.endswith(':P'):
            return lista[:k]
        elif visto and prof == 0 and k > 0 and lista[k - 1] == '}' and t.endswith(':V'):
            return lista[:k]
    return lista


def nuestro(txt, fn):
    """Los tokens de FUNC en el texto de nuestro .s, o None si no esta."""
    i = txt.find('.stabs "%s:F' % fn)
    if i < 0:
        i = txt.find('.stabs "%s:f' % fn)
    if i < 0:
        return None
    j = txt.find('.stabs "",36,0,0,$Lscope', i)
    out = []
    for linea in txt[i:j].splitlines()[1:]:
        m = re.match(r'\s*\.stabs "([^":]*):([^"]*)"', linea)
        if m:
            out.append(tok(m.group(1), m.group(2)))
        elif re.match(r'\s*\.stabn 192,', linea):
            out.append('{')
        elif re.match(r'\s*\.stabn 224,', linea):
            out.append('}')
    return out


def original(fn):
    """Los tokens de FUNC en el .mdebug de la A124 (nombre EXACTO), o None si no esta."""
    mdebug.carga()
    i0 = mdebug._cache['por_nombre'].get(fn)
    if i0 is None:
        return None
    syms = mdebug._cache['syms']
    out = []
    prof = 0
    for j in range(i0, min(i0 + 4000, mdebug.SYM_N)):
        jiss, jval, jst, jsc = syms[j]
        if jst == 8 and jval == j - i0 - 1 and prof == 0:
            break
        if jst in (5, 6):
            continue
        if jst in (0, 2, 3, 4):
            nom = mdebug._cadena(jiss)
            if nom.startswith('$LBB'):
                out.append('{')
            elif nom.startswith('$LBE'):
                out.append('}')
            elif jst in (0, 2) and ':' in nom and len(nom) < 120 and not re.search(r'\s', nom.split(':', 1)[0]):
                # los stabs van en simbolos stNil/stStatic (los stParam/stLocal son ECOFF de los .s de GNU)
                a, b = nom.split(':', 1)
                out.append(tok(a, b.split()[0] if b.split() else ''))
    return corta(out)


def compara(txt, fn):
    """(original, nuestro, diff, diferencias de nombre, diferencias de bloque); None si falta en alguno."""
    o, n = original(fn), nuestro(txt, fn)
    if o is None or n is None:
        return None
    d = list(difflib.unified_diff(o, n, 'original', 'nuestro', lineterm='', n=2))
    cuerpo = [x for x in d if x[:1] in '+-' and not x.startswith(('+++', '---'))]
    nom = [x for x in cuerpo if x[1:] not in ('{', '}')]
    bl = [x for x in cuerpo if x[1:] in ('{', '}')]
    return o, n, d, nom, bl


def resumen(txt, fn):
    r = compara(txt, fn)
    if r is None:
        return '%-60s no esta (%s)' % (fn[:60], 'original' if original(fn) is None else 'nuestro')
    o, n, d, nom, bl = r
    return '%-60s orig %3d nuestro %3d  nombres %2d bloques %2d  netas: %s  | %s' % (
        fn[:60], len(o), len(n), len(nom), len(bl), ' '.join(netas(o, n)) or '-', ' '.join(nom)[:150])


def netas(o, n):
    """Las diferencias de nombre SIN el orden: lo que el original declara y nosotros no (-) y al reves (+). Un
    `+result:r -result:r` del diff ordenado es solo orden; aqui se va."""
    co = Counter(x for x in o if x not in ('{', '}'))
    cn = Counter(x for x in n if x not in ('{', '}'))
    out = []
    for k, c in sorted((co - cn).items()):
        out += ['-' + k] * c
    for k, c in sorted((cn - co).items()):
        out += ['+' + k] * c
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    if len(args) < 2:
        print(__doc__)
        sys.exit(2)
    q = '-q' in sys.argv
    txt = open(args[0], errors='replace').read()
    fns = []
    for a in args[1:]:
        if a.startswith('@'):
            fns += [x for x in open(a[1:], encoding='utf-8').read().split() if x]
        else:
            fns.append(a)
    for fn in fns:
        if q:
            print(resumen(txt, fn))
            continue
        r = compara(txt, fn)
        if r is None:
            print(resumen(txt, fn))
            continue
        o, n, d, nom, bl = r
        print('\n'.join(d) or '%s: iguales' % fn)
        print('orig', len(o), 'nuestro', len(n))


if __name__ == '__main__':
    main()
