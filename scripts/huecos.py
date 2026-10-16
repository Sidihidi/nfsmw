#!/usr/bin/env python3
"""Censo de cuerpos que mienten: funciones que compilan y no hacen nada.

`bMin'/`bMax' (leccion 57), `bSqrt' (leccion 67) y las diez de VU0 eran todas
el mismo fallo: la rama de la plataforma existe, esta vacia, compila sin una
sola advertencia y devuelve basura.  Y no se ve en un diff, porque lo que cuelga
de ellas GCC lo borra: la funcion simplemente no esta.

La navaja es que una funcion que DEVUELVE algo y tiene el cuerpo vacio no puede
ser correcta nunca.  Un accesor vacio de tipo `void' si puede serlo (de 597 en
GameCube el original solo expandia 225, y 222 eran constructores o destructores),
asi que esos se marcan aparte y no se cuentan como hueco.

  python scripts/huecos.py                  # solo los seguros
  python scripts/huecos.py --todos          # tambien los `void' y los TODO
"""
import argparse
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# `tipo nombre(args) {}' en una linea, o con el `{}' en la siguiente.
RE_VACIA = re.compile(
    r'^[ \t]*(?:inline\s+|static\s+|virtual\s+|explicit\s+)*'
    r'([A-Za-z_][\w:<>,\s\*&]*?)\s+'
    r'(~?[A-Za-z_]\w*(?:::\w+)?)\s*'
    r'\(([^;{]*)\)\s*(?:const\s*)?\{\s*\}\s*$')

# Un cuerpo que es un unico `return <constante>;' con un TODO al lado.
RE_TODO = re.compile(r'return\s+(?:0|0\.0f|0\.0|false|true|NULL|nullptr)\s*;'
                     r'\s*(?://|/\*)\s*TODO', re.I)

VOIDS = ('void', 'void*', 'void *')

# `operator new'/`operator delete' reenvian al asignador del juego y se
# escriben aparte; ademas parten mal (el nombre saldria `new').
RE_ALLOC = re.compile(r'\boperator\s+(new|delete)\b')

# Los demas operadores SI entran: `FEVector3 operator+(...) const {}' es un
# hueco igual que cualquier otro, y excluirlos por llevar la palabra
# `operator' dejaba fuera los cinco de FEVector3.
RE_OPERADOR = re.compile(
    r'^[ \t]*(?:inline\s+|static\s+|virtual\s+|explicit\s+)*'
    r'([A-Za-z_][\w:<>,\s\*&]*?)\s*'
    r'\boperator\s*([-+*/%^&|~!=<>]+|\[\]|\(\)|->)\s*'
    r'\([^;{]*\)\s*(?:const\s*)?\{\s*\}\s*$')


# Codigo de terceros que no decompilamos: STL, SDK, paquetes.
FUERA = ('/stl/', '/stlport/', '/Packages/', '/stlps2/', '/gpp/',
         '/gcc/', '/newlib', '/include/xbox/', '/msl/')


def es_ctor_dtor(tipo, nombre):
    """Un constructor o un destructor no tiene tipo de retorno propio.

    El nombre del tipo es la ultima palabra: `explicit vector' es el
    constructor de `vector', no una funcion que devuelva `explicit'.
    """
    if nombre.startswith('~'):
        return True
    ultima = tipo.replace('*', ' ').replace('&', ' ').split()
    return bool(ultima) and ultima[-1].split('::')[-1] == nombre


def cuenta_usos(nombres):
    """Cuantas veces se LLAMA a cada nombre en todo el arbol."""
    usos = dict.fromkeys(nombres, 0)
    patron = re.compile(r'\b(' + '|'.join(re.escape(n) for n in nombres)
                        + r')\s*\(') if nombres else None
    if patron is None:
        return usos
    for raiz, _, ficheros in os.walk(os.path.join(RAIZ, 'src')):
        for f in ficheros:
            if not f.endswith(('.h', '.hpp', '.cpp', '.c', '.inl')):
                continue
            ruta = os.path.join(raiz, f)
            rel = os.path.relpath(ruta, RAIZ).replace(os.sep, '/')
            if any(x in '/' + rel for x in FUERA):
                continue
            try:
                with open(ruta, encoding='utf-8', errors='replace') as fh:
                    for m in patron.finditer(fh.read()):
                        usos[m.group(1)] += 1
            except OSError:
                pass
    return usos


def raiz_fichero(rel):
    """`UVectorMathGC.hpp' y `UVectorMathCPU.hpp' son el mismo fichero."""
    base = os.path.splitext(os.path.basename(rel))[0]
    for suf in ('GC', 'CPU', 'PS2', 'PSX2', 'E', 'X', '_G', '_PS2', '_X360',
                '_PC', 'Plat', 'Old'):
        if base.endswith(suf) and len(base) > len(suf) + 2:
            base = base[:-len(suf)]
            break
    return base.lower()


def hermanas(huecos):
    """{(fichero, nombre): [sitios donde ESE nombre si tiene cuerpo]}.

    Es lo que separa el arreglo de un minuto de la decompilacion de verdad: si
    la variante de otra plataforma, la sobrecarga de al lado o la version por
    puntero ya estan escritas, el cuerpo se copia de ahi.  Los diez de VU0 y
    los diecinueve de bMath y FEMath salieron todos asi.

    MEDIDO: emparejar solo por NOMBRE da 248 de 248 y es mentira -- `GetSize'
    existe en cuarenta clases sin relacion ninguna.  Vale como hermana la que
    esta en el MISMO fichero (la sobrecarga de al lado, la version por puntero)
    o en una variante de plataforma del mismo fichero.  Lo demas es un nombre
    que coincide, no una hermana.
    """
    nombres = set(n for _, _, _, n in huecos)
    fuera = dict(((rel, n), []) for rel, _, _, n in huecos)
    if not nombres:
        return fuera
    # Una definicion con cuerpo: abre llave y NO la cierra vacia en la linea.
    # El lookahead va SIN `$': sin re.M el `$' solo casa al final del fichero,
    # asi que la propia funcion vacia se contaba a si misma como hermana y
    # salian 248 de 248.  Basta con que tras la llave no venga otra llave.
    patron = re.compile(r'\b(' + '|'.join(re.escape(n) for n in nombres)
                        + r')\s*\([^;{]*\)\s*(?:const\s*)?\{(?!\s*\})')
    for raiz, _, ficheros in os.walk(os.path.join(RAIZ, 'src')):
        for f in ficheros:
            if not f.endswith(('.h', '.hpp', '.cpp', '.c', '.inl')):
                continue
            ruta = os.path.join(raiz, f)
            rel = os.path.relpath(ruta, RAIZ).replace(os.sep, '/')
            if any(x in '/' + rel for x in FUERA):
                continue
            try:
                with open(ruta, encoding='utf-8', errors='replace') as fh:
                    texto = fh.read()
            except OSError:
                continue
            mia = raiz_fichero(rel)
            corto = os.path.basename(rel)
            for m in patron.finditer(texto):
                for clave in fuera:
                    if clave[1] != m.group(1):
                        continue
                    if raiz_fichero(clave[0]) != mia:
                        continue
                    if corto not in fuera[clave]:
                        fuera[clave].append(corto)
    return fuera


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--todos', action='store_true')
    p.add_argument('--raiz', default='src')
    args = p.parse_args()

    seguros, voids, todos_todo = [], [], []
    for raiz, _, ficheros in os.walk(os.path.join(RAIZ, args.raiz)):
        for f in ficheros:
            if not f.endswith(('.h', '.hpp', '.cpp', '.c', '.inl')):
                continue
            ruta = os.path.join(raiz, f)
            rel = os.path.relpath(ruta, RAIZ).replace('\\', '/')
            if any(x in '/' + rel for x in FUERA):
                continue
            try:
                with open(ruta, encoding='utf-8', errors='replace') as fh:
                    lineas = fh.readlines()
            except OSError:
                continue
            for i, linea in enumerate(lineas, 1):
                if RE_TODO.search(linea):
                    todos_todo.append((rel, i, linea.strip()))
                    continue
                # `void *operator new(...)' parte mal: el nombre saldria `new'
                # y contaria los 1.240 `new (' del arbol.  Y un operador que
                # reenvia al asignador del juego se escribe aparte.  Pero
                # excluir TODA linea con `operator' era un punto ciego: los
                # cinco operadores vacios de FEVector3 no salian en el censo.
                if RE_ALLOC.search(linea):
                    continue
                m = RE_OPERADOR.match(linea.rstrip())
                if m:
                    tipo, nombre = m.group(1).strip(), 'operator' + m.group(2)
                    if tipo in VOIDS or tipo.endswith('void'):
                        voids.append((rel, i, nombre))
                    else:
                        seguros.append((rel, i, tipo, nombre))
                    continue
                m = RE_VACIA.match(linea.rstrip())
                if not m:
                    continue
                tipo, nombre = m.group(1).strip(), m.group(2).strip()
                if es_ctor_dtor(tipo, nombre) or tipo in ('class', 'struct',
                                                          'else', 'do', 'try',
                                                          'return', 'case'):
                    continue
                if tipo in VOIDS or tipo.endswith('void'):
                    voids.append((rel, i, nombre))
                else:
                    seguros.append((rel, i, tipo, nombre))

    # Los de cabecera se meten en linea en CADA llamante y envenenan a todos;
    # los de .cpp solo se pierden a si mismos.  Se ordenan por llamadas.
    cab = [x for x in seguros if x[0].endswith(('.h', '.hpp', '.inl'))]
    cpp = [x for x in seguros if not x[0].endswith(('.h', '.hpp', '.inl'))]
    usos = cuenta_usos(set(n for _, _, _, n in cab))
    herm = hermanas(cab)
    # Primero las que tienen hermana escrita: esas se copian, las otras no.
    cab.sort(key=lambda x: (not herm.get((x[0], x[3])), -usos.get(x[3], 0)))
    con = sum(1 for x in cab if herm.get((x[0], x[3])))
    print('=== NO-VOID VACIA EN CABECERA: %d (envenena a cada llamante) ==='
          % len(cab))
    print('    %d tienen HERMANA escrita (se copia el idioma); %d no.'
          % (con, len(cab) - con))
    for rel, i, tipo, nombre in cab[:60]:
        h = herm.get((rel, nombre)) or []
        print('  %5d usos  %-58s  %s %s' % (usos.get(nombre, 0),
                                            '%s:%d' % (rel, i), tipo, nombre))
        if h:
            print('              hermana en: %s' % ', '.join(h[:4]))
    if len(cab) > 60:
        print('  ... y %d mas' % (len(cab) - 60))
    print()
    print('=== NO-VOID VACIA EN .cpp: %d (se pierde solo a si misma) ==='
          % len(cpp))

    print('\n=== `return <constante>; // TODO\': %d ===' % len(todos_todo))
    for rel, i, txt in sorted(todos_todo)[:60 if not args.todos else None]:
        print('  %s:%d  %s' % (rel, i, txt[:90]))
    if not args.todos and len(todos_todo) > 60:
        print('  ... y %d mas (--todos)' % (len(todos_todo) - 60))

    if args.todos:
        print('\n=== `void\' con cuerpo vacio: %d (casi todos LEGITIMOS) ==='
              % len(voids))
        for rel, i, nombre in sorted(voids):
            print('  %s:%d  %s' % (rel, i, nombre))
    else:
        print('\n(%d funciones `void\' vacias; casi todas legitimas, --todos)'
              % len(voids))


if __name__ == '__main__':
    main()
