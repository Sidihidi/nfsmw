#!/usr/bin/env python3
"""trocea360.py -- rehace el troceado de Xbox 360 (config/EUROPEGERMILESTONE/splits.txt) por ESQUELETO, sin mover lo
que ya casa. Metodo de la ronda 41 (x36041), subido aqui en la ronda 42.

Por que: el ejecutable de 360 se enlazo con LTCG POR FICHERO (369 objetos en el Rich header), no por SourceList. Las
"unidades" de 360 son rangos que hay que deducir, y al entrar codigo nuevo hay funciones ya escritas que caen en la
unidad equivocada o en trozos auto_ que no se miden.

Pasos (todos fuera del arbol salvo `aplica`):

  etiquetas DIR     cada funcion del original (troceado de DIR/build/<V>/obj: unidades y auto_) recibe la unidad
                    nuestra cuyo esqueleto (palabras con los campos reubicados a cero) es el UNICO que coincide.
                    Lo nuestro: build/<V>/ltcg/obj (troceado LTCG) y build/<V>/src (fichero a fichero) del arbol,
                    mas --extra=DIR,... (objetos de unidades que el arbol aun no compila para 360).
                    Escribe DIR/etiquetas.tsv: direccion, tamano, trozo actual, etiqueta (unidad, '*' ambigua,
                    '-' sin pareja), nombre.
  splits DIR SALIDA tramos por unidad a partir de las etiquetas. Las ANCLAS (lo que ya casa al 100 % en el informe
                    oficial, o en --anclas=JSON de `banco`) se quedan en su unidad pase lo que pase; un tramo corto
                    (menos de 4 funciones y 600 B) no abre unidad salvo que sea frontend; la frontera es la primera
                    funcion propia de la unidad siguiente, ajustada al principio de una funcion de symbols.txt; y
                    un .pdata por cada rango de .text (si se juntan, jeff da "overlaps"). Solo rehace las regiones
                    del juego (R1, R3) y zFeOverlay (R2); el resto se conserva tal cual, y zDebug y zOnline
                    recortan lo nuevo. --sin=R1,R2,R3 deja una region como esta.
  prueba DIR SPLITS [SYMBOLS]
                    prepara una raiz de prueba en DIR (config, orig y herramientas enlazados) y trocea el original
                    con jeff segun SPLITS: DIR/build/<V>/obj. No toca el arbol.
  banco ETIQ --raiz=DIR [--extra=zA] [--base=ETIQ]
                    enlace LTCG de nuestro arbol (IL de build/<V>/ltcg/il) troceado y comparado contra los
                    originales de DIR, con la normalizacion oficial (pool por valor, tablas por destino). Escribe
                    DIR/b/ETIQ.json = {unidad: {funcion: [pct, tam]}}. Sin --raiz, contra el arbol: es el control.
  nombra DIR ETIQ   nombra_grafo y nombra_corrige360 sobre la raiz de prueba, con nuestros objetos del banco ETIQ.
  compara DIR A B   ganadas y perdidas entre dos bancos (una funcion que cambia de unidad y casa en la otra no es
                    perdida).
  aplica DIR        copia DIR/config/<V>/splits.txt y symbols.txt al arbol (solo con el candado de medir.py libre).

Vuelta tipica (unos 5 min):
    python scripts/trocea360.py prueba D config/EUROPEGERMILESTONE/splits.txt   # raiz = arbol actual
    python scripts/trocea360.py banco ctl --raiz=D                                # control
    python scripts/trocea360.py etiquetas D
    python scripts/trocea360.py splits D D/splits_nuevo.txt --anclas=D/b/ctl.json
    python scripts/trocea360.py prueba D D/splits_nuevo.txt
    python scripts/trocea360.py banco n1 --raiz=D --base=ctl
    python scripts/trocea360.py nombra D n1 && python scripts/trocea360.py prueba D D/config/.../splits.txt ...
y medir.py EUROPEGERMILESTONE tras `aplica`.
"""
import bisect
import collections
import glob
import json
import os
import re
import shutil
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
V = 'EUROPEGERMILESTONE'
FRONT = {'zFe', 'zFe2', 'zFEng', 'zFeOverlay'}
MINP, MINB, GRANDE = 4, 600, 20
# regiones del juego que se rehacen (medidas en la ronda 41): lo de fuera (render de Xenon, Live, bibliotecas, CRT)
# no tiene fuente que case y se conserva
REGIONES = {'R1': (0x820F0000, 0x82470310), 'R3': (0x8249A6C0, 0x825720BC)}
FEOV = (0x82591B30, 0x825C3358)   # R2: zFeOverlay, entero
CONSERVA = {'zDebug', 'zOnline'}
NUEVAS = (('zEagl4Anim', 'zDynamics'), ('zFeOverlay', 'zFe2'))   # unidad nueva: va detras de esta


def opt(nombre, defecto=None):
    for a in sys.argv[1:]:
        if a.startswith('--%s=' % nombre):
            return a.split('=', 1)[1]
    return defecto


def corto(u):
    return u.split('/')[-1].rsplit('.', 1)[0]


def lee_splits(ruta):
    cab, unidades, u = [], collections.OrderedDict(), None
    for l in open(ruta, encoding='utf-8'):
        m = re.match(r'^(\S.*\.cpp):', l)
        if m:
            u = m.group(1)
            unidades[u] = []
            continue
        if u is None:
            cab.append(l)
            continue
        m = re.match(r'\s+(\.\w+)\s+start:0x(\w+) end:0x(\w+)', l)
        if m:
            unidades[u].append((m.group(1), int(m.group(2), 16), int(m.group(3), 16)))
    return cab, unidades


def lee_symbols(ruta):
    por_nombre = collections.defaultdict(list)
    for m in re.finditer(r'^(\S+) = \.text:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)',
                         open(ruta, encoding='utf-8').read(), re.M):
        por_nombre[m.group(1)].append((int(m.group(2), 16), int(m.group(3), 16)))
    return por_nombre


# ------------------------------------------------------------------------------------------------- .pdata
class Pdata:
    def __init__(self):
        import ltcg360
        pe = ltcg360.PE(os.path.join(RAIZ, 'orig', V, 'NFS.exe'))
        n, va, vs, pr, rs = pe.seccion('.pdata')
        self.ent = [struct.unpack_from('>I', pe.d, pr + 8 * i)[0] for i in range(vs // 8)]
        self.base = 0x82000000 + va

    def rango(self, a, b):
        i0 = bisect.bisect_left(self.ent, a)
        i1 = bisect.bisect_left(self.ent, b)
        return (self.base + 8 * i0, self.base + 8 * i1) if i1 > i0 else None


# ------------------------------------------------------------------------------------------------- etiquetas
def etiquetas(d):
    import nombra_grafo as ng
    x = ng.X360.__new__(ng.X360)
    _, unidades = lee_splits(os.path.join(d, 'config', V, 'splits.txt'))
    spl = [(a, b, corto(u)) for u, rs in unidades.items() for s, a, b in rs if s == '.text']
    obj = os.path.join(d, 'build', V, 'obj')

    def fun_dirs(path, t):
        c = ng.Coff(path)
        out = {}
        if t.startswith('auto'):
            rs = [(int(re.search(r'_([0-9A-F]{8})_text', t).group(1), 16), None)]
        else:
            rs = sorted((a, b) for a, b, uu in spl if uu == t)
        f = c.funciones()
        for nombre, valor, sec, clase in c.lista:
            if nombre in f and nombre not in out:
                off = valor
                for a, b in rs:
                    if b is None or off < b - a:
                        out[nombre] = (a + off, f[nombre])
                        break
                    off -= (b - a)
        return out
    orig = []
    trozos = [os.path.basename(p)[:-4] for p in glob.glob(os.path.join(obj, 'auto_*_text.obj'))]
    for t in trozos + sorted(set(u for a, b, u in spl)):
        p = os.path.join(obj, t + '.obj') if t.startswith('auto') else \
            os.path.join(obj, 'Speed', 'Indep', 'SourceLists', t + '.obj')
        if not os.path.exists(p):
            continue
        for n, (a, (pal, rel)) in fun_dirs(p, t).items():
            orig.append((a, len(pal) * 4, t, n, x.esqueleto(pal, rel) if len(pal) >= 5 else None))
    orig.sort()
    nuestro = collections.defaultdict(set)
    fuentes = glob.glob(os.path.join(RAIZ, 'build', V, 'ltcg', 'obj', 'Speed', 'Indep', 'SourceLists', '*.obj'))
    fuentes += glob.glob(os.path.join(RAIZ, 'build', V, 'src', 'Speed', 'Indep', 'SourceLists', '*.obj'))
    for e in [x for x in opt('extra', '').split(',') if x]:
        fuentes += glob.glob(os.path.join(e, '*.obj'))
    for p in fuentes:
        un = os.path.basename(p)[:-4]
        for n, (pal, rel) in ng.Coff(p).funciones().items():
            if len(pal) >= 5:
                nuestro[x.esqueleto(pal, rel)].add(un)
    cuenta = collections.Counter()
    with open(os.path.join(d, 'etiquetas.tsv'), 'w', newline='\n') as f:
        for a, s, t, n, e in orig:
            us = nuestro.get(e) if e else None
            lab = list(us)[0] if us and len(us) == 1 else ('*' if us else '-')
            cuenta['unica' if lab not in '*-' else lab] += 1
            f.write('%08X\t%d\t%s\t%s\t%s\n' % (a, s, t, lab, n))
    print('etiquetas: %d funciones del original; %s' % (len(orig), dict(cuenta)))


# ------------------------------------------------------------------------------------------------- anclas
def anclas(ruta):
    """{unidad corta: set(funcion)} de lo que casa al 100 %: un banco ({u: {f: [pct, tam]}}) o un informe oficial."""
    d = json.load(open(ruta, encoding='utf-8'))
    out = collections.defaultdict(set)
    if 'units' in d:
        for u in d['units']:
            for f in u.get('functions', []):
                if f.get('fuzzy_match_percent') == 100.0:
                    out[corto(u['name'])].add(f['name'])
    else:
        for u, fs in d.items():
            for f, (pct, tam) in fs.items():
                if pct == 100.0:
                    out[u].add(f)
    return out


# ------------------------------------------------------------------------------------------------- splits
def splits(d, salida):
    sin = set(x for x in opt('sin', '').split(',') if x)
    cab, unidades = lee_splits(opt('base-splits', os.path.join(RAIZ, 'config', V, 'splits.txt')))
    por_nombre = lee_symbols(opt('base-symbols', os.path.join(RAIZ, 'config', V, 'symbols.txt')))
    pts = {}
    for l in open(os.path.join(d, 'etiquetas.tsv'), encoding='utf-8'):
        a, s, t, lab, n = l.rstrip('\n').split('\t')
        if lab not in '-*':
            pts[int(a, 16)] = (int(a, 16), int(s), lab, False)
    rangos_u = collections.defaultdict(list)
    for u, rs in unidades.items():
        for sec, a, b in rs:
            if sec == '.text':
                rangos_u[corto(u)].append((a, b))
    na = 0
    for u, fs in anclas(opt('anclas', os.path.join(RAIZ, 'build', 'versiones', V + '.json'))).items():
        for f in fs:
            for a, sz in por_nombre.get(f, []):
                if any(x0 <= a < x1 for x0, x1 in rangos_u[u]):
                    pts[a] = (a, sz, u, True)
                    na += 1
    print('anclas: %d' % na)
    funs = sorted(x for v in por_nombre.values() for x in v)
    ini = [a for a, s in funs]

    def ajusta(b):   # b dentro de una funcion de symbols.txt -> el principio de esa funcion
        i = bisect.bisect_right(ini, b) - 1
        if i >= 0 and ini[i] < b < ini[i] + funs[i][1]:
            return ini[i]
        return b
    pts = [pts[a] for a in sorted(pts)]

    def runs(p):
        out = []
        for a, s, lab, f in p:
            if out and out[-1]['u'] == lab:
                o = out[-1]
                o['b'] = a + s
                o['n'] += 1
                o['by'] += s
                o['p'].append((a, s, lab, f))
                o['f'] |= f
            else:
                out.append({'u': lab, 'a': a, 'b': a + s, 'n': 1, 'by': s, 'p': [(a, s, lab, f)], 'f': f})
        return out

    def segmenta(r0, r1):
        p = [x for x in pts if r0 <= x[0] < r1]
        while True:
            r = runs(p)
            grandes = collections.Counter(x['u'] for x in r if x['n'] >= GRANDE)

            def vale(x):
                if x['n'] >= GRANDE or x['f']:
                    return True
                if x['n'] < MINP and x['by'] < MINB:
                    return False
                if x['u'] in FRONT or x['u'] == 'zFoundation':
                    return True
                return grandes[x['u']] == 0 and x['u'] != 'zMain'
            malos = [i for i, x in enumerate(r) if not vale(x)]
            if not malos:
                break
            i = min(malos, key=lambda i: (r[i]['n'], r[i]['by']))
            p = [q for j, x in enumerate(r) if j != i for q in x['p']]
        segs = []
        for i, x in enumerate(r):
            a = r0 if i == 0 else ajusta(x['a'])
            b = r1 if i == len(r) - 1 else ajusta(r[i + 1]['a'])
            segs.append((x['u'], a, b))
        return segs
    nuevos = []
    for k, (r0, r1) in REGIONES.items():
        if k not in sin:
            nuevos += segmenta(r0, r1)
    if 'R2' not in sin:
        nuevos.append(('zFeOverlay',) + FEOV)
    cubierto = [(a, b) for _, a, b in nuevos]

    def resta(trozos, quitar):
        for ca, cb in quitar:
            nt = []
            for x0, x1 in trozos:
                if cb <= x0 or ca >= x1:
                    nt.append((x0, x1))
                    continue
                if x0 < ca:
                    nt.append((x0, ca))
                if cb < x1:
                    nt.append((cb, x1))
            trozos = nt
        return trozos
    texto = collections.defaultdict(list)
    for u, rs in unidades.items():
        for sec, a, b in rs:
            if sec != '.text':
                continue
            if corto(u) in CONSERVA:
                texto[corto(u)].append((a, b))
            else:
                texto[corto(u)] += resta([(a, b)], cubierto)
    for u, a, b in nuevos:
        texto[u] += resta([(a, b)], [r for v in CONSERVA for r in texto[v]])
    for u in texto:   # juntar contiguos
        j = []
        for a, b in sorted(x for x in texto[u] if x[1] > x[0]):
            if j and j[-1][1] == a:
                j[-1] = (j[-1][0], b)
            else:
                j.append((a, b))
        texto[u] = j
    todo = sorted((a, b, u) for u in texto for a, b in texto[u])
    for i in range(1, len(todo)):
        assert todo[i][0] >= todo[i - 1][1], (todo[i - 1], todo[i])
    orden = [corto(u) for u in unidades]
    ruta = {corto(u): u for u in unidades}
    for extra, despues in NUEVAS:
        if texto.get(extra) and extra not in orden:
            orden.insert(orden.index(despues) + 1, extra)
            ruta[extra] = 'Speed/Indep/SourceLists/%s.cpp' % extra
    pd = Pdata()
    bloques = []
    for u in orden:
        t = texto.get(u, [])
        if not t:
            print('sin rangos:', u)
            continue
        bl = '%s:\n' % ruta[u]
        for x0, x1 in sorted(x for x in (pd.rango(a, b) for a, b in t) if x):
            bl += '\t.pdata      start:0x%08X end:0x%08X\n' % (x0, x1)
        for x0, x1 in t:
            bl += '\t.text       start:0x%08X end:0x%08X\n' % (x0, x1)
        bloques.append(bl)
    with open(salida, 'w', newline='\n') as f:
        f.write(''.join(cab) + '\n'.join(bloques))
    viejo = {corto(u): [(a, b) for s, a, b in rs if s == '.text'] for u, rs in unidades.items()}
    for u in orden:
        if texto.get(u, []) != viejo.get(u, []):
            print('%-18s %8d B -> %8d B  %s' % (u, sum(b - a for a, b in viejo.get(u, [])),
                                                sum(b - a for a, b in texto.get(u, [])),
                                                ' '.join('%08X-%08X' % x for x in texto.get(u, []))))


# ------------------------------------------------------------------------------------------------- prueba
def prueba(d, spl, sym=None):
    d = os.path.abspath(d)
    for sub in ('config/' + V, 'orig/' + V, 'build/tools', 'build/' + V, 'scripts', 'b'):
        os.makedirs(os.path.join(d, sub), exist_ok=True)
    for rel in ('orig/%s/NFS.exe' % V, 'build/tools/objdiff-cli.exe'):
        dst = os.path.join(d, rel)
        if not os.path.exists(dst):
            os.link(os.path.join(RAIZ, rel), dst)
    for s in ('ltcg360.py', 'nombra_grafo.py', 'nombra_corrige360.py', 'escritura.py'):
        shutil.copy(os.path.join(RAIZ, 'scripts', s), os.path.join(d, 'scripts', s))
    if os.path.abspath(spl) != os.path.join(d, 'config', V, 'splits.txt'):
        shutil.copy(spl, os.path.join(d, 'config', V, 'splits.txt'))
    sym = sym or os.path.join(RAIZ, 'config', V, 'symbols.txt')
    if os.path.abspath(sym) != os.path.join(d, 'config', V, 'symbols.txt'):
        shutil.copy(sym, os.path.join(d, 'config', V, 'symbols.txt'))
    yml = open(os.path.join(RAIZ, 'config', V, 'config.yml'), encoding='utf-8').read()
    rel = os.path.relpath(d, RAIZ).replace('\\', '/')
    yml = yml.replace('symbols: config/', 'symbols: %s/config/' % rel).replace('splits: config/', 'splits: %s/config/' % rel)
    open(os.path.join(d, 'config.yml'), 'w', newline='\n').write(yml)
    sal = os.path.join(d, 'split')
    shutil.rmtree(sal, ignore_errors=True)
    r = subprocess.run([os.path.join(RAIZ, 'build', 'tools', 'jeff.exe'), 'xex', 'split', rel + '/config.yml',
                        rel + '/split'], cwd=RAIZ, capture_output=True, text=True, errors='replace')
    open(os.path.join(d, 'jeff.log'), 'w').write(r.stdout + r.stderr)
    if r.returncode:
        print('jeff fallo:\n' + '\n'.join((r.stdout + r.stderr).splitlines()[-8:]))
        return 1
    o = os.path.join(d, 'build', V, 'obj')
    shutil.rmtree(o, ignore_errors=True)
    shutil.move(os.path.join(sal, 'obj'), o)
    shutil.rmtree(sal, ignore_errors=True)
    print('prueba: troceado en %s' % o)
    return 0


# ------------------------------------------------------------------------------------------------- banco
def banco(etiq):
    import ltcg360
    objdiff = os.path.join(RAIZ, 'build', 'tools', 'objdiff-cli.exe')
    raiz_o = os.path.abspath(opt('raiz', RAIZ))
    salida = os.path.join(raiz_o if raiz_o != RAIZ else os.path.join(RAIZ, 'build', V, 'trocea360'), 'b')
    os.makedirs(salida, exist_ok=True)
    oil = os.path.join(RAIZ, 'build', V, 'ltcg', 'il')
    base_il = sorted(f[:-4] for f in os.listdir(oil) if f.endswith('.obj'))
    extra = [x for x in opt('extra', '').split(',') if x]
    todas = sorted(set(base_il) | set(extra))
    # --compila=todas|zA,zB --cl=-DX=1,...: recompila esas unidades con opciones de cl de mas (prueba de flags)
    cl_extra = [x for x in opt('cl', '').split(',') if x]
    recomp = opt('compila', '')
    recomp = todas if recomp == 'todas' else [x for x in recomp.split(',') if x]
    extra = sorted(set(extra) | set(recomp))
    trab = os.path.join(salida, etiq)
    shutil.rmtree(trab, ignore_errors=True)
    il = os.path.join(trab, 'il')
    os.makedirs(il)
    for u in base_il:
        if u not in extra:
            os.link(os.path.join(oil, u + '.obj'), os.path.join(il, u + '.obj'))
    shutil.copy(os.path.join(RAIZ, 'build', V, 'ltcg', 'stubs.txt'), os.path.join(trab, 'stubs.txt'))
    if extra:
        import bancopc
        cl = os.path.join(RAIZ, 'build', 'compilers', 'X360', '14.00.2110', 'cl.exe')

        def compila(u):
            r = subprocess.run([cl] + bancopc.flags(V, None, []) + cl_extra + ['/TP', '/GL', '/Fo' + os.path.join(
                il, u + '.obj'), os.path.join(RAIZ, 'src', 'Speed', 'Indep', 'SourceLists', u + '.cpp')],
                capture_output=True, text=True, cwd=RAIZ, errors='replace')
            return u, r.returncode
        with ThreadPoolExecutor(4) as ex:
            for u, rc in ex.map(compila, extra):
                if rc:
                    print('NO COMPILA', u)
                    return 1
    r0, dc0 = ltcg360.rutas, ltcg360.definidos_coff

    def rutas(raiz):
        return {'base': trab, 'il': il, 'obj': os.path.join(trab, 'obj'), 'exe': os.path.join(trab, 'NFS.exe'),
                'map': os.path.join(trab, 'NFS.map'), 'log': os.path.join(trab, 'link.log'),
                'stubs': os.path.join(trab, 'stubs.txt'), 'stubobj': os.path.join(trab, 'stubs.obj'),
                'rsp': os.path.join(trab, 'link.rsp'), 'orig': os.path.join(trab, 'orig')}

    def definidos(ruta):   # el objeto de fichero de una unidad extra puede estar solo en la raiz de prueba
        if not os.path.exists(ruta):
            ruta = os.path.join(raiz_o, os.path.relpath(ruta, RAIZ))
        return dc0(ruta)
    ltcg360.rutas, ltcg360.definidos_coff = rutas, definidos
    tmp = os.path.join(trab, 'tmp')
    os.makedirs(tmp, exist_ok=True)
    os.environ['TMP'] = os.environ['TEMP'] = tmp
    ils = [('build/%s/src/Speed/Indep/SourceLists/%s.obj' % (V, u), None, None) for u in todas]
    try:
        if not ltcg360.enlaza(RAIZ, ils, print):
            print('enlace fallo')
            return 1
        ltcg360.trocea(raiz_o, ils, print)
    finally:
        ltcg360.rutas, ltcg360.definidos_coff = r0, dc0

    def diff(u):
        orig = os.path.join(trab, 'orig', 'Speed', 'Indep', 'SourceLists', u + '.obj')
        mio = os.path.join(trab, 'obj', 'Speed', 'Indep', 'SourceLists', u + '.obj')
        if not os.path.exists(orig):
            return u, {}
        sal = os.path.join(trab, 'd_%s.json' % u)
        p = subprocess.run([objdiff, 'diff', '-1', orig, '-2', mio, '-c', 'functionRelocDiffs=none',
                            '-c', 'ppc.calculatePoolRelocations=false', '-o', sal, '--format', 'json'],
                           capture_output=True, text=True)
        if p.returncode:
            print('objdiff fallo', u, p.stderr[-300:])
            return u, {}
        dd = json.load(open(sal, encoding='utf-8'))
        os.remove(sal)
        return u, {s.get('name', ''): [s.get('match_percent'), int(s.get('size') or 0)]
                   for s in dd['left']['symbols'] if s.get('kind') == 'SYMBOL_FUNCTION'}
    todo = {}
    with ThreadPoolExecutor(4) as ex:
        for u, out in ex.map(diff, todas):
            todo[u] = out
    json.dump(todo, open(os.path.join(salida, etiq + '.json'), 'w'))
    n = sum(1 for u in todo for f, (p, t) in todo[u].items() if p == 100.0)
    b = sum(t for u in todo for f, (p, t) in todo[u].items() if p == 100.0)
    print('%s: %d funciones al 100 %% (%d B)' % (etiq, n, b))
    if '--conservar' not in sys.argv:
        shutil.rmtree(trab, ignore_errors=True)
    if opt('base'):
        compara(salida, opt('base'), etiq)
    return 0


def compara(dirb, a, b):
    A = json.load(open(os.path.join(dirb, a + '.json')))
    B = json.load(open(os.path.join(dirb, b + '.json')))

    def casadas(D):
        return {(u, f): t for u, fs in D.items() for f, (p, t) in fs.items() if p == 100.0}
    ca, cb = casadas(A), casadas(B)
    gan = {k: t for k, t in cb.items() if k not in ca}
    per = {k: t for k, t in ca.items() if k not in cb}
    nb = {f for (u, f) in cb}
    mov = {k for k in per if k[1] in nb}
    print('%s -> %s: +%d (%d B)  -%d (%d B)  [de las perdidas, %d casan en otra unidad]' % (
        a, b, len(gan), sum(gan.values()), len(per), sum(per.values()), len(mov)))
    print('  ganadas por unidad', dict(collections.Counter(u for u, f in gan)))
    print('  perdidas por unidad', dict(collections.Counter(u for u, f in per)))
    for k in sorted(per):
        if k not in mov:
            print('   PIERDE', k[0], k[1][:110])


# ------------------------------------------------------------------------------------------------- nombra
def nombra(d, etiq):
    """nombra_grafo y nombra_corrige360 (copias en DIR/scripts: trabajan sobre DIR/config) hasta punto fijo."""
    nuestros = os.path.join(d, 'b', etiq, 'obj')
    if not os.path.isdir(nuestros):
        print('falta %s (banco con --conservar)' % nuestros)
        return 1
    banco_json = os.path.join(d, 'b', etiq + '.json')
    for paso in range(6):
        total = 0
        r = subprocess.run([sys.executable, 'scripts/nombra_grafo.py', V, '--nuestros=' + nuestros, '--aplicar'],
                           cwd=d, capture_output=True, text=True, errors='replace')
        lin = [x for x in r.stdout.splitlines() if 'aplicados' in x or 'FINALES' in x]
        print('nombra_grafo:', ' | '.join(lin[-2:]) or r.stderr[-300:])
        m = re.search(r'FINALES: (\d+)', r.stdout)
        total += int(m.group(1)) if m else 0
        # la regla de nombre equivocado necesita lo que casa: el banco ETIQ (lo nombrado ahora aun no es ancla)
        r = subprocess.run([sys.executable, 'scripts/nombra_corrige360.py', '--banco=' + banco_json,
                            '--nuestros=' + nuestros, '--originales=' + os.path.join(d, 'b', etiq, 'orig'),
                            '--raiz=' + d, '--aplicar'], cwd=d, capture_output=True, text=True, errors='replace')
        lin = [x for x in r.stdout.splitlines() if x.startswith(('validacion', 'correcciones', 'aplicados'))]
        print('nombra_corrige360:', ' | '.join(lin) or r.stderr[-300:])
        m = re.search(r'aplicados (\d+)', r.stdout)
        total += int(m.group(1)) if m else 0
        if not total:
            break
    return 0


def aplica(d):
    for f in ('splits.txt', 'symbols.txt'):
        a = os.path.join(d, 'config', V, f)
        b = os.path.join(RAIZ, 'config', V, f)
        if open(a, 'rb').read() != open(b, 'rb').read():
            if '--ver' in sys.argv:
                print('distinto:', f)
            else:
                shutil.copy(a, b)
                print('aplicado:', f)
        else:
            print('igual:', f)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if not args:
        print(__doc__)
        return 1
    c = args[0]
    if c == 'etiquetas':
        return etiquetas(os.path.abspath(args[1]))
    if c == 'splits':
        return splits(os.path.abspath(args[1]), args[2])
    if c == 'prueba':
        return prueba(args[1], args[2], args[3] if len(args) > 3 else None)
    if c == 'banco':
        return banco(args[1])
    if c == 'compara':
        return compara(os.path.join(os.path.abspath(args[1]), 'b'), args[2], args[3])
    if c == 'nombra':
        return nombra(os.path.abspath(args[1]), args[2])
    if c == 'aplica':
        return aplica(os.path.abspath(args[1]))
    print(__doc__)
    return 1


if __name__ == '__main__':
    sys.exit(main() or 0)
