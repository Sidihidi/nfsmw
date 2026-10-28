#!/usr/bin/env python3
"""lt360.py --dir DIR [--copia] [--sombra=DIR_INCLUDE ...] [UNIDAD ...] -- enlace LTCG PRIVADO de Xbox 360.

Para iterar sobre el enlace LTCG (la medida oficial de EUROPEGERMILESTONE, ver ltcg360.py) sin gastar una medida ni
tocar build/: DIR guarda una copia del IL del enlace oficial (il/, lib/, stubs.obj). Las unidades dadas (zOnline,
zFe...: SourceLists) se recompilan con /GL en DIR/il con las MISMAS cflags que les da configure.py (se ejecuta
configure.py hasta antes de escribir build.ninja, para no depender de como este configurado build.ninja ahora), y
se enlaza DIR/il + DIR/lib + DIR/stubs.obj en el orden del link.rsp oficial a DIR/ltcg/NFS_ltcg.exe y .map.
No coge el candado de medida: solo lee build/EUROPEGERMILESTONE/ltcg/ (con --copia) y escribe en DIR.

  --copia          rehace DIR/il, DIR/lib y DIR/stubs.obj desde build/EUROPEGERMILESTONE/ltcg/ (tras una medida)
  --sombra=DIR     /I DIR delante de las demas (una copia de una cabecera con un cambio a probar)
  --cflags         imprime las cflags de cada unidad y sale
  --juego          recompila todas las SourceLists z* del enlace (una cabecera comun); --hilos N (4)

Luego cmplt360.py compara funciones de ese enlace con el EGM (--enlace DIR/ltcg). Desde la raiz del repo:

    python scripts/lt360.py --dir D:/tmp/lt --copia
    python scripts/lt360.py --dir D:/tmp/lt zOnline zFe
    python scripts/cmplt360.py parejas.tsv -v --enlace D:/tmp/lt/ltcg
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

V = 'EUROPEGERMILESTONE'
CL = os.path.join('build', 'compilers', 'X360', '14.00.2110', 'cl.exe')
LINK = os.path.join('build', 'compilers', 'X360', '14.00.2110', 'link.exe')
LANG = ('/TP', '/TC', '/Tp', '/Tc')


def cflags_configure(raiz, unidades, libs=()):
    """{unidad: cflags} con las listas de configure.py (libreria + extra_cflags del objeto, mas /TP como project.py).
    `libs`: rutas de objetos de bibliotecas (Speed/Indep/Libs/..., Packages/...), con la ruta como clave."""
    src = open(os.path.join(raiz, 'configure.py'), encoding='utf-8').read()
    corte = src.rfind('\nif args.mode == "configure":')
    assert corte > 0, 'configure.py: no encuentro el final (if args.mode == "configure")'
    g = {'__name__': '__lt360__', '__file__': os.path.join(raiz, 'configure.py')}
    argv, cwd, path = sys.argv, os.getcwd(), list(sys.path)
    try:
        sys.argv = ['configure.py', '--version', V]
        os.chdir(raiz)
        sys.path.insert(0, raiz)
        exec(compile(src[:corte], 'configure.py', 'exec'), g)
    finally:
        sys.argv, sys.path[:] = argv, path
        os.chdir(cwd)
    out = {}
    for lib in g['config'].libs:
        for obj in lib.get('objects', []):
            nombre = obj.name.replace('\\', '/')
            base = os.path.splitext(os.path.basename(nombre))[0]
            if nombre in libs:
                base = nombre
            elif base not in unidades or not nombre.startswith('Speed/Indep/SourceLists/'):
                continue
            cf = obj.options.get('cflags') or lib.get('cflags') or []
            extra = list(obj.options.get('extra_cflags') or [])
            if not any(f in LANG for f in list(cf) + extra):
                extra.insert(0, '/TC' if nombre.endswith('.c') else '/TP')
            out[base] = (' '.join(list(cf) + extra), 'src/' + nombre)
    return out


def copia(raiz, d):
    of = os.path.join(raiz, 'build', V, 'ltcg')
    for sub in ('il', 'lib'):
        if os.path.isdir(os.path.join(d, sub)):
            shutil.rmtree(os.path.join(d, sub))
        shutil.copytree(os.path.join(of, sub), os.path.join(d, sub))
    shutil.copy2(os.path.join(of, 'stubs.obj'), os.path.join(d, 'stubs.obj'))
    shutil.copy2(os.path.join(of, 'stubs.txt'), os.path.join(d, 'stubs.txt'))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--dir', required=True)
    ap.add_argument('--copia', action='store_true')
    ap.add_argument('--cflags', action='store_true')
    ap.add_argument('--sombra', action='append', default=[])
    ap.add_argument('--hilos', type=int, default=4)
    ap.add_argument('--juego', action='store_true', help='todas las SourceLists z* del link.rsp oficial')
    ap.add_argument('--lib', action='append', default=[],
                    help='objeto de biblioteca de configure.py (Speed/Indep/Libs/... o Packages/...; repetible, o '
                         '@fichero con una ruta por linea): se compila con /GL en DIR/il y se enlaza como unidad '
                         '(delante de lib/); si su nombre base es el de una de lib/ (csis), esa sale del enlace')
    ap.add_argument('--sust', action='append', default=[],
                    help='REL=COPIA: la unidad o --lib REL (su ruta de configure.py) se compila desde COPIA (una '
                         'variante del .cpp; sus #include relativos se buscan junto a COPIA)')
    ap.add_argument('unidades', nargs='*')
    a = ap.parse_args()
    raiz = os.getcwd()
    d = os.path.abspath(a.dir)
    os.makedirs(d, exist_ok=True)
    if a.copia:
        copia(raiz, d)
        print('copia del enlace oficial en %s' % d)
    if not os.path.isdir(os.path.join(d, 'il')):
        sys.exit('%s/il no existe: --copia primero' % d)
    if a.juego:
        rsp = open(os.path.join(raiz, 'build', V, 'ltcg', 'link.rsp')).read().replace('\\', '/').split()
        a.unidades = [os.path.splitext(os.path.basename(x.strip('"')))[0] for x in rsp if '/ltcg/il/z' in x]
    libs = []
    for x in a.lib:
        libs += [l.strip() for l in open(x[1:])] if x.startswith('@') else [x]
    libs = [l.replace('\\', '/') for l in libs if l and not l.startswith('#')]
    fl = cflags_configure(raiz, set(a.unidades), set(libs)) if a.unidades or libs else {}
    for u in libs:
        if u not in fl:
            sys.exit('%s: configure.py no lo lista para %s' % (u, V))
    bases = [os.path.splitext(os.path.basename(u))[0] for u in libs]
    if len(set(bases)) != len(bases):
        sys.exit('dos --lib con el mismo nombre de objeto: el .map solo da el nombre base')
    a.unidades = list(a.unidades) + libs
    sust = dict(s.split('=', 1) for s in a.sust)
    for u in a.unidades:
        if u not in fl:
            sys.exit('%s: configure.py no tiene Speed/Indep/SourceLists/%s para %s' % (u, u, V))
    if a.cflags:
        for u in a.unidades:
            print('%s: %s' % (u, fl[u][0]))
        return
    def compila(u):
        t = time.time()
        cf, fuente = fl[u]
        fuente = sust.get(u, fuente)
        sombra = ' '.join('/I "%s"' % s for s in a.sombra)
        base = os.path.splitext(os.path.basename(u))[0]
        cmd = '"%s" %s %s /GL /Fo"%s" %s' % (os.path.join(raiz, CL), sombra, cf,
                                             os.path.join(d, 'il', base + '.obj'), fuente.replace('/', '\\'))
        r = subprocess.run(cmd, capture_output=True, text=True, errors='replace', cwd=raiz)
        errs = [x for x in (r.stdout + r.stderr).splitlines() if ': error' in x or 'fatal error' in x]
        if u in libs and not errs and r.returncode == 0:
            # el COFF sin /GL dice que define (el IL no lo dice): eso no puede ser stub. Un stub que la biblioteca
            # tambien define rompe el enlace LTCG con LNK1223 (".pdata contributions") antes de avisar con LNK4006.
            os.makedirs(os.path.join(d, 'coff'), exist_ok=True)
            subprocess.run(cmd.replace(' /GL /Fo"%s"' % os.path.join(d, 'il', base + '.obj'),
                                       ' /Fo"%s"' % os.path.join(d, 'coff', base + '.obj')),
                           capture_output=True, text=True, errors='replace', cwd=raiz)
        return u, errs, r.returncode, time.time() - t

    malos = 0
    with ThreadPoolExecutor(max(1, a.hilos)) as ex:
        for u, errs, rc, dt in ex.map(compila, a.unidades):
            print('%s: %d errores, %.0f s' % (u, len(errs), dt))
            if errs or rc:
                print('\n'.join(errs[:10]))
                malos += 1
    if malos:
        sys.exit(1)
    orden = [x.strip().strip('"').replace('\\', '/') for x in open(os.path.join(raiz, 'build', V, 'ltcg', 'link.rsp'))]
    orden = [p[p.index('/ltcg/') + len('/ltcg/'):] for p in orden]
    # las --lib van detras de las unidades del enlace oficial y delante de lib/ (y la de lib/ con su nombre, fuera)
    nuevas = ['il/%s.obj' % b for b in bases if 'il/%s.obj' % b not in orden]
    orden = [p for p in orden if not (p.startswith('lib/') and os.path.splitext(p[4:])[0] in bases)]
    k = next((i for i, p in enumerate(orden) if not p.startswith('il/')), len(orden))
    orden = orden[:k] + nuevas + orden[k:]
    with open(os.path.join(d, 'link.rsp'), 'w') as fh:
        for p in orden:
            fh.write('"%s"\n' % os.path.join(d, p).replace('\\', '/'))
    os.makedirs(os.path.join(d, 'ltcg'), exist_ok=True)
    exe, mp = os.path.join(d, 'ltcg', 'NFS_ltcg.exe'), os.path.join(d, 'ltcg', 'NFS_ltcg.map')
    # los stubs del enlace oficial; lo que la fuente nueva deja sin resolver se anade (como ltcg360.enlaza)
    sys.path.insert(0, os.path.join(raiz, 'scripts'))
    import ltcg360
    stubs_txt = os.path.join(d, 'stubs.txt')
    if not os.path.exists(stubs_txt):
        shutil.copy2(os.path.join(raiz, 'build', V, 'ltcg', 'stubs.txt'), stubs_txt)
    stubs = {}
    for linea in open(stubs_txt, encoding='latin-1'):
        k, _, n = linea.rstrip('\n').partition(' ')
        if n:
            stubs[n] = k
    definidos = set()
    for b in bases:
        p = os.path.join(d, 'coff', b + '.obj')
        if os.path.exists(p):
            definidos |= ltcg360.definidos_coff(p)
    if definidos & set(stubs):
        print('%d stubs que ya definen las --lib, fuera' % len(definidos & set(stubs)))
        stubs = {n: k for n, k in stubs.items() if n not in definidos}
        ltcg360.escribe_stubs(os.path.join(d, 'stubs.obj'), stubs)
    for vuelta in range(4):
        for f in (exe, mp):
            if os.path.exists(f):
                os.remove(f)
        t = time.time()
        r = subprocess.run([os.path.join(raiz, LINK), '/nologo', '/LTCG', '/XEX:NO', '/NODEFAULTLIB', '/FORCE:MULTIPLE',
                            '/OPT:NOREF', '/ENTRY:main', '/MAP:' + mp, '/OUT:' + exe,
                            '@' + os.path.join(d, 'link.rsp')], cwd=raiz, capture_output=True, text=True,
                           errors='replace')
        log = r.stdout + r.stderr
        open(os.path.join(d, 'link.log'), 'w', encoding='utf-8').write(log)
        nuevos = {n: k for n, k in ltcg360.sin_resolver(log).items() if n not in stubs}
        # lo que la fuente nueva ya define sobra en stubs.obj (con /FORCE:MULTIPLE ganaria el stub)
        sobran = set(re.findall(r'^stubs\.obj : warning LNK4006: .*? \((\S+)\) already defined in', log, re.M))
        print('enlace %.0f s, exe %s, %d sin resolver nuevos, %d stubs de sobra' %
              (time.time() - t, os.path.exists(exe), len(nuevos), len(sobran)))
        if sobran:
            print('  stubs que sobran: ' + ', '.join(sorted(sobran)[:10]))
            for n in sobran:
                stubs.pop(n, None)
        if os.path.exists(exe) and not nuevos and not sobran:
            break
        if not nuevos and not sobran:
            for x in log.splitlines():
                if 'fatal' in x or 'error' in x:
                    print(x[:200])
            sys.exit(1)
        if nuevos:
            print('  stubs nuevos: ' + ', '.join(sorted(nuevos)[:10]))
        stubs.update(nuevos)
        ltcg360.escribe_stubs(os.path.join(d, 'stubs.obj'), stubs)
        with open(stubs_txt, 'w', encoding='latin-1') as fh:
            for n in sorted(stubs):
                fh.write('%s %s\n' % (stubs[n], n))
    if not os.path.exists(exe):
        sys.exit(1)


if __name__ == '__main__':
    main()
