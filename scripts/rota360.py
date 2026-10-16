#!/usr/bin/env python3
"""rota360.py -- nombres CAMBIADOS entre funciones de Xbox 360 que YA casan (ronda 53).

El hueco que dejan las otras reglas: nombra_corrige360.py no toca una funcion que ya casa al 100 % (ancla), y
nombra_permuta360.py exige que el codigo sea UNICO en la unidad. Pero con `functionRelocDiffs=none` un nombre
equivocado sobre codigo identico tambien da 100 %: los destructores de bTList<LoadedCar>/<LoadedSkin>/<LoadedWheel>/
<Car> de zWorld estaban rotados un puesto (los cuatro tienen el mismo codigo; los llaman ~CarLoader y ~World), y los
`_IHandle` de las interfaces de zPhysicsBehaviors igual. El informe no cambia, pero el nombre miente (y un port o
quien lea el arbol llamaria a la funcion equivocada).

Evidencia (la de nombra_corrige360): cada ANCLA (funcion al 100 % con su nombre) llama, en su instruccion k, a la
direccion T del original; la nuestra, en la misma instruccion, llama a B. Se renombra T -> B si:
  - todas las anclas que llaman a T dicen B (unanimidad), y T se llama hoy de otra forma (X);
  - B esta definido en nuestro objeto de la unidad de T y su codigo es el del original en T (objdiff al 100 % con un
    nombre marcador en las dos copias: la medida no cambia);
  - X y B no son el mismo destructor con otra decoracion (`??_E`/`??_G`, alias del troceado);
  - B no queda repetido en la unidad: si ya lo lleva otra direccion, esa tambien cambia por esta regla (ciclos y
    cadenas); si no, la propuesta cae;
  - hay al menos --min llamantes (2 por defecto).
No cambia la medida (las dos ya casaban); --aplicar escribe symbols.txt. Pasarlo despues de medir.

    python scripts/rota360.py [--min N] [--aplicar]
"""
import collections
import json
import os
import re
import struct
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if not os.path.isdir(os.path.join(RAIZ, 'config', 'EUROPEGERMILESTONE')):
    RAIZ = os.environ.get('NFSMW_RAIZ', os.getcwd())  # copia de trabajo fuera del arbol: la raiz es el directorio actual
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import ltcg360  # noqa: E402
from nombra_corrige360 import renombra, casadas_de, SIN_NOMBRE, BASE  # noqa: E402

V = 'EUROPEGERMILESTONE'


def alias(x, b):
    q = lambda n: re.sub(r'^\?\?_[EG]', '??_X', n)  # noqa: E731
    return q(x) == q(b)


def main():
    a = sys.argv[1:]
    minimo = int(a[a.index('--min') + 1]) if '--min' in a else 2
    nuestros = os.path.join(RAIZ, 'build', V, 'ltcg', 'obj')
    originales = os.path.join(RAIZ, 'build', V, 'ltcg', 'orig')
    res = casadas_de(None, os.path.join(RAIZ, 'build', 'versiones', V + '.json'))
    objdiff = os.path.join(RAIZ, 'build', 'tools', 'objdiff-cli.exe')
    sims = os.path.join(RAIZ, 'config', V, 'symbols.txt')
    lineas = open(sims, encoding='utf-8').read().split('\n')
    nombres_en = collections.defaultdict(list)
    dirs_de = collections.defaultdict(list)
    for i, l in enumerate(lineas):
        m = re.match(r'(\S+) = \.text:0x([0-9A-Fa-f]+);', l)
        if m:
            d = int(m.group(2), 16)
            nombres_en[d].append((m.group(1), i))
            dirs_de[m.group(1)].append(d)
    rangos = []
    u = None
    for l in open(os.path.join(RAIZ, 'config', V, 'splits.txt'), encoding='utf-8'):
        if l and not l[0].isspace() and l.rstrip().endswith(':'):
            u = l.strip()[:-1].rsplit('/', 1)[-1].rsplit('.', 1)[0]
            continue
        m = re.match(r'\s+\.text\s+start:0x([0-9A-F]+) end:0x([0-9A-F]+)', l)
        if m and u and u != 'Sections':
            rangos.append((int(m.group(1), 16), int(m.group(2), 16), u))

    def unidad_de(d):
        for x, y, un in rangos:
            if x <= d < y:
                return un
        return None
    pe = ltcg360.PE(os.path.join(RAIZ, 'orig', V, 'NFS.exe'))

    def palabra(d):
        return struct.unpack('>I', pe.lee(d - BASE, 4))[0]

    def obj_de(base, un):
        return os.path.join(base, 'Speed', 'Indep', 'SourceLists', un + '.obj')

    nuestro = {}
    for un in res:
        p = obj_de(nuestros, un)
        if not os.path.exists(p):
            continue
        _, secs, ss = ltcg360.lee_coff(p)
        _, _, tm, rels = secs[0]
        defs = sorted((v, n) for n, v, s, cl in ss.values() if s == 1 and cl in (2, 3) and not n.startswith('.'))
        fun = {}
        for k, (v, n) in enumerate(defs):
            fun.setdefault(n, (v, defs[k + 1][0] if k + 1 < len(defs) else tm))
        nuestro[un] = (fun, {off: ss[isim][0] for off, isim, t in rels if t == ltcg360.R_REL24})

    anclas_dir = {}
    for un, fs in res.items():
        for n, pct in fs.items():
            if pct == 100.0:
                for d in dirs_de.get(n, []):
                    if unidad_de(d) == un:
                        anclas_dir[d] = n
    evid = collections.defaultdict(collections.Counter)
    for un, fs in res.items():
        if un not in nuestro:
            continue
        fun, r = nuestro[un]
        for n, pct in fs.items():
            if pct != 100.0 or n not in fun:
                continue
            ds = [d for d in dirs_de.get(n, []) if unidad_de(d) == un]
            if len(ds) != 1:
                continue
            v, fin = fun[n]
            for off in range(v, fin, 4):
                if off not in r or SIN_NOMBRE.match(r[off]):
                    continue
                w = palabra(ds[0] + off - v)
                if w >> 26 != 18:
                    continue
                li = w & 0x03FFFFFC
                if li & 0x02000000:
                    li -= 0x04000000
                t = li if w & 2 else ds[0] + off - v + li
                if t in nombres_en:
                    evid[t][r[off]] += 1

    prop = {}   # t -> (b, x, un, n)
    for t, c in evid.items():
        if t not in anclas_dir or len(c) != 1:
            continue
        b = next(iter(c))
        x = anclas_dir[t]
        un = unidad_de(t)
        if b == x or alias(x, b) or c[b] < minimo or un not in nuestro or b not in nuestro[un][0]:
            continue
        if b in [n for n, _ in nombres_en[t]]:
            continue
        prop[t] = (b, x, un, c[b])
    if '--detalle' in a:
        for t, (b, x, un, n) in sorted(prop.items()):
            print('PROP %08X %-12s %-50s -> %-50s %d' % (t, un, x[:50], b[:50], n))
    # punto fijo: B libre en la unidad, o su duena tambien cambia
    acept = dict(prop)
    while True:
        malos = set()
        cnt = collections.Counter((v[2], v[0]) for v in acept.values())
        for t, (b, x, un, n) in acept.items():
            if cnt[(un, b)] > 1:
                malos.add(t)
            for h in dirs_de.get(b, []):
                if h != t and unidad_de(h) == un and h not in acept:
                    malos.add(t)
        if not malos:
            break
        for t in malos:
            acept.pop(t)
    # control de codigo: objdiff al 100 % de T contra nuestra B
    ok = {}
    tmp = os.path.join(RAIZ, 'build', V, 'ltcg', 'rota360')
    os.makedirs(tmp, exist_ok=True)
    por_u = collections.defaultdict(list)
    for t, v in acept.items():
        por_u[v[2]].append(t)
    for un, ts in por_u.items():
        co, cn = {}, {}
        for t in ts:
            b, x = acept[t][0], acept[t][1]
            mk = 'ROTA360_%08X' % t
            if len([d for d in dirs_de[x] if unidad_de(d) == un]) == 1:
                co[x] = mk
            cn[b] = mk
        to, tn, sal = (os.path.join(tmp, f) for f in ('o.obj', 'n.obj', 'd.json'))
        open(to, 'wb').write(renombra(open(obj_de(originales, un), 'rb').read(), co))
        open(tn, 'wb').write(renombra(open(obj_de(nuestros, un), 'rb').read(), cn))
        subprocess.run([objdiff, 'diff', '-1', to, '-2', tn, '-c', 'functionRelocDiffs=none',
                        '-c', 'ppc.calculatePoolRelocations=false', '-o', sal, '--format', 'json'],
                       capture_output=True, text=True)
        pct = {s.get('name'): s.get('match_percent') for s in json.load(open(sal, encoding='utf-8'))['left']['symbols']}
        for t in ts:
            if pct.get('ROTA360_%08X' % t) == 100.0:
                ok[t] = acept[t]
        for f in (to, tn, sal):
            os.remove(f)
    os.rmdir(tmp)
    # tras el control, otra vez el punto fijo (una que cae puede dejar a otra con el nombre repetido)
    while True:
        malos = {t for t, (b, x, un, n) in ok.items()
                 if any(h != t and unidad_de(h) == un and h not in ok for h in dirs_de.get(b, []))}
        if not malos:
            break
        for t in malos:
            ok.pop(t)
    print('anclas con llamantes unanimes que dicen otro nombre: %d; sin repetir en la unidad: %d; codigo 100 %%: %d'
          % (len(prop), len(acept), len(ok)))
    for t, (b, x, un, n) in sorted(ok.items(), key=lambda kv: (kv[1][2], kv[0])):
        print('  %-18s 0x%08X %-55s -> %s  (%d llamantes)' % (un, t, x[:55], b[:80], n))
    if '--aplicar' in a and ok:
        for t, (b, x, un, n) in ok.items():
            i = [i for nm, i in nombres_en[t] if nm == x][0]
            assert lineas[i].startswith(x + ' = .text:'), lineas[i]
            lineas[i] = b + lineas[i][len(x):]
        open(sims, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
        print('aplicados %d' % len(ok))


if __name__ == '__main__':
    main()
