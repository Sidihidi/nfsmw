#!/usr/bin/env python3
"""sondapc.py V CABECERA CLASE [miembro ...] [--sombra=DIR] [--todos] -- pregunta al compilador de PC o de 360 el tamano de
CLASE y el desplazamiento de sus miembros (con la sombra, si se da). Truco: `sizeof(Mostrar<N>)` de una plantilla sin
definir da un error C2027 que imprime N.

--todos: saca los miembros de datos de la declaracion de la clase en la cabecera (lineas `tipo nombre;`).
"""
import os
import re
import subprocess
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import bancopc  # noqa: E402

RAIZ = bancopc.RAIZ


def miembros_de(cabecera, clase, sombra):
    ruta = os.path.join(sombra, cabecera) if sombra and os.path.exists(os.path.join(sombra, cabecera)) else \
        os.path.join(RAIZ, 'src', cabecera)
    t = open(ruta, encoding='utf-8', errors='replace').read()
    m = re.search(r'\b(class|struct)\s+%s\b[^;{]*\{' % re.escape(clase), t)
    if not m:
        return []
    fin = re.compile(r'^\};', re.M).search(t, m.end())
    cuerpo = t[m.end():fin.start() if fin else len(t)]
    # fuera las directivas y las cabeceras alternativas de la clase (#if ... class X : A<20> { #else ... #endif)
    cuerpo = '\n'.join(x for x in cuerpo.split('\n') if not x.strip().startswith('#')
                       and not re.match(r'\s*(class|struct)\s+%s\b' % re.escape(clase), x))
    # quitar cuerpos de funciones en linea
    limpio = ''
    prof = 0
    for ch in cuerpo:
        if ch == '{':
            prof += 1
            continue
        if ch == '}':
            prof -= 1
            continue
        if prof == 0:
            limpio += ch
    out = []
    for linea in limpio.split('\n'):
        s = linea.split('//')[0].strip()
        if not s.endswith(';') or '(' in s or s.startswith(('typedef', 'friend', 'using', 'enum', 'static', 'return')):
            continue
        mm = re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*(\[[^\]]*\])*\s*;$', s)
        if mm and mm.group(1) not in out:
            out.append(mm.group(1))
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    v, cabecera, clase = args[0], args[1], args[2]
    sombra = None
    for a in sys.argv[1:]:
        if a.startswith('--sombra='):
            sombra = os.path.abspath(a.split('=', 1)[1])
    miembros = args[3:]
    if '--todos' in sys.argv:
        miembros = miembros_de(cabecera, clase, sombra)
    exprs = ['sizeof(%s)' % clase] + [m[1:] if m.startswith('=') else '(int)&(((%s *)0)->%s)' % (clase, m)
                                      for m in miembros]
    fuente = os.path.join(bancopc.BANCO, 'sonda_%s_%s.cpp' % (v, clase.replace(':', '_')))
    lineas = ['#define private public', '#define protected public', '#include "%s"' % cabecera,
              'template <int N> struct Mostrar;']
    for k, e in enumerate(exprs):
        lineas.append('int sonda_%d = sizeof(Mostrar<%s>); // %d' % (k, e, k))
    open(fuente, 'w', encoding='utf-8').write('\n'.join(lineas) + '\n')
    cl = os.path.join(RAIZ, 'build', 'compilers', 'Win32', '7.1', 'cl.exe') if v == 'SPEED_EXE_1_3' else \
        os.path.join(RAIZ, 'build', 'compilers', 'X360', '14.00.2110', 'cl.exe')
    obj = fuente[:-4] + '.obj'
    r = subprocess.run([cl] + bancopc.flags(v, sombra, []) + ['/Fo' + obj, fuente], capture_output=True, text=True,
                       cwd=bancopc.BANCO, errors='replace')
    salida = r.stdout + r.stderr
    valores = {}
    actual = None
    for linea in salida.splitlines():
        m = re.search(r'\((\d+)\)\s*:\s*error C2027:.*Mostrar<(-?\d+|N)>', linea)
        if m:
            actual = int(m.group(1))
            if m.group(2) != 'N':
                valores[actual] = int(m.group(2))
                actual = None
            continue
        m = re.search(r'^\s*N=(-?\d+)', linea)
        if m and actual is not None:
            valores[actual] = int(m.group(1))
            actual = None
    base = 4  # la primera expresion va en la linea 5
    for k, e in enumerate(exprs):
        n = valores.get(base + 1 + k)
        nombre = 'sizeof' if k == 0 else miembros[k - 1]
        print('  %-40s %s' % (nombre, ('%#x' % n) if n is not None else '?'))
    if not valores:
        print('\n'.join(x for x in salida.splitlines() if 'error' in x)[:3000])
    for q in (fuente, obj):
        try:
            os.remove(q)
        except OSError:
            pass


if __name__ == '__main__':
    main()
