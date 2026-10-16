#!/usr/bin/env python3
"""guardas_ps2.py [--aplicar] [fichero...] -- busca bloques `#ifdef EA_BUILD_A124 / #else / #endif` en los que las
dos ramas son el MISMO codigo con la comparacion al reves (o los operandos cambiados de sitio), y cambia el guarda
a `#ifdef EA_PLATFORM_PLAYSTATION2`.

Por que: esas ramas no distinguen el prototipo del juego final, distinguen el EE de PowerPC. Se escribieron
midiendo solo la A124, pero SLUS-21351 se compila con el mismo EE-GCC, asi que le vale la misma forma; GameCube se
queda con la rama #else y no cambia ni un byte. Los guardas que separan CODIGO de verdad (el online del
prototipo, una clase con otro tamano) no los toca: solo pasan dos formas medidas contra el original, el ternario
con la comparacion complementaria y las ramas cambiadas (`X == 0 ? A : B` frente a `X != 0 ? B : A`, que es lo
que separa `movn` de `movz` en el EE) y el mismo texto con solo el operador de comparacion cambiado.

    python scripts/guardas_ps2.py                 # informe
    python scripts/guardas_ps2.py --aplicar       # y los cambia
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CMP = re.compile(r'(!=|==|>=|<=|<|>)')


COMPL = {'==': '!=', '!=': '==', '<': '>=', '>=': '<', '<=': '>', '>': '<='}
TERN = re.compile(r'^return(.+?)(==|!=|>=|<=|<|>)(.+?)\?(.+?):(.+?);$')


def limpia(texto):
    t = re.sub(r'//.*', '', texto)
    t = re.sub(r'/\*.*?\*/', '', t, flags=re.S)
    return re.sub(r'\s+', '', t)


def misma_forma(uno, otro):
    """Solo dos formas, las dos comprobadas contra el original:
       - `return X OP Y ? A : B;` frente a `return X ~OP Y ? B : A;` (la comparacion al reves y las ramas
         cambiadas de sitio), que es lo que separa movn de movz en el EE;
       - el mismo texto con solo el operador de comparacion cambiado."""
    a, b = limpia(uno), limpia(otro)
    if not a or not b or a == b:
        return False
    ma, mb = TERN.match(a), TERN.match(b)
    if ma and mb:
        return (ma.group(1) == mb.group(1) and ma.group(3) == mb.group(3) and
                COMPL.get(ma.group(2)) == mb.group(2) and
                ma.group(4) == mb.group(5) and ma.group(5) == mb.group(4))
    return CMP.sub('OP', a) == CMP.sub('OP', b)


def bloques(lineas):
    """Devuelve (i_ifdef, i_else, i_endif) de cada bloque #ifdef EA_BUILD_A124 ... #else ... #endif sin anidar."""
    out = []
    i = 0
    while i < len(lineas):
        if re.match(r'\s*#\s*ifdef\s+EA_BUILD_A124\s*$', lineas[i]):
            prof = 0
            j = i + 1
            els = None
            while j < len(lineas):
                if re.match(r'\s*#\s*if', lineas[j]):
                    prof += 1
                elif re.match(r'\s*#\s*endif', lineas[j]):
                    if prof == 0:
                        break
                    prof -= 1
                elif prof == 0 and re.match(r'\s*#\s*else\s*$', lineas[j]):
                    els = j
                j += 1
            if j < len(lineas) and els is not None:
                out.append((i, els, j))
                i = j
        i += 1
    return out


def main():
    args = [a for a in sys.argv[1:] if a != '--aplicar']
    aplicar = '--aplicar' in sys.argv[1:]
    if args:
        ficheros = args
    else:
        r = subprocess.run(['git', 'ls-files', 'src'], capture_output=True, text=True, cwd=ROOT)
        ficheros = [f for f in r.stdout.split() if f.endswith(('.cpp', '.hpp', '.h'))]
    total = 0
    for f in ficheros:
        p = os.path.join(ROOT, f)
        try:
            crudo = open(p, 'rb').read().decode('utf-8', 'replace')
        except OSError:
            continue
        if 'EA_BUILD_A124' not in crudo:
            continue
        eol = '\r\n' if '\r\n' in crudo else '\n'
        lineas = crudo.replace('\r\n', '\n').split('\n')
        cambios = []
        for a, e, z in bloques(lineas):
            uno, otro = lineas[a + 1:e], lineas[e + 1:z]
            if not uno or not otro or len(uno) > 8 or len(otro) > 8:
                continue
            if not misma_forma('\n'.join(uno), '\n'.join(otro)):
                continue
            cambios.append(a)
        if not cambios:
            continue
        total += len(cambios)
        print('%-72s %d bloque(s)' % (f, len(cambios)))
        for a in cambios:
            print('      %s   |   %s' % (lineas[a + 1].strip()[:52], lineas[a + 3 if a + 3 < len(lineas) else a + 1].strip()[:52]))
        if aplicar:
            for a in cambios:
                lineas[a] = lineas[a].replace('EA_BUILD_A124', 'EA_PLATFORM_PLAYSTATION2')
            open(p, 'wb').write(eol.join(lineas).encode('utf-8'))
    print('--- %d bloques de pura forma%s' % (total, ' (aplicados)' if aplicar else ''))


if __name__ == '__main__':
    main()
