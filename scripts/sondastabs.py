#!/usr/bin/env python3
"""sondastabs.py [--solo zSim.cpp,...] [--clase Nombre] [-j N] -- compara el LAYOUT de PS2 de TODAS las clases que
describen los stabs del .mdebug de la A124 con el nuestro: el offset de cada campo y el tamano, sondeando cada clase
desde la SourceList que la compila.

Por que los stabs y no el volcado (symbols/PS2/PS2_types.nothpp): dan el offset y el tamano en BITS de cada campo,
sin tener que adivinar el tamano de ningun tipo, y cada clase trae su nombre CALIFICADO en la firma de sus metodos
(`Q29ESimulate10StaticData` es ESimulate::StaticData). Con el volcado, alinvec.py solo podia mirar los vectores; asi
salieron SimSystem::mEventData y el mState de las conexiones de sonido, que no son vectores.

Por que desde la SourceList: es exactamente la unidad que compila el build, asi que el contexto (cabeceras, orden,
defines de version) esta completo por construccion; la sonda por cabecera suelta no alcanzaba ni las clases
definidas dentro de un .cpp ni las cabeceras que no compilan solas.

Pasos:
  1. lee las estructuras de los stabs (cache en build/sondastabs/stabs.pkl);
  2. busca donde se define cada clase en nuestro arbol y que SourceList la ve (cierre de #include);
  3. por SourceList, una sonda con `int sonda_N = offsetof-a-mano;` por campo y un sizeof por clase, con
     `#define private public`; las lineas que no compilan (miembro renombrado, clase que no existe en PS2) se
     quitan y se reintenta;
  4. informe: por clase, el PRIMER campo que no cuadra (la causa suele estar justo antes) y el tamano.

Los campos de bits, los punteros a vtable y los estaticos no se sondean. Configura SLES para leer los flags de
cada SourceList y deja configurada GOWE69 al terminar. NO se puede correr a la vez que pctall.py (los dos
reconfiguran build.ninja).

    python scripts/sondastabs.py                      # todas
    python scripts/sondastabs.py --solo zSim.cpp      # una SourceList
    python scripts/sondastabs.py --clase SimSystem,X  # solo esas clases (nombre corto o calificado)
    python scripts/sondastabs.py --plantillas         # tambien las plantillas (casi todas STL: lento)"""
import json
import os
import pickle
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.getcwd()
PY = sys.executable
V = 'SLES-53558-A124'
PARCIAL = '--clase' in sys.argv or '--solo' in sys.argv
DIR = os.path.join('build', 'sondastabs')
SL_DIR = 'src/Speed/Indep/SourceLists'
sys.path.insert(0, os.path.join(ROOT, 'scripts'))


# ---------------------------------------------------------------- 1. stabs
class Stabs:
    """Lector minimo de las cadenas stabs de GCC 2.9: tipos, y de las estructuras, bases y campos."""

    def __init__(self, s):
        self.s, self.i = s, 0

    def peek(self):
        return self.s[self.i] if self.i < len(self.s) else ''

    def num(self):
        m = re.compile(r'-?\d+').match(self.s, self.i)
        if not m:
            raise ValueError('num en %d' % self.i)
        self.i = m.end()
        return int(m.group())

    def expect(self, c):
        if self.s[self.i] != c:
            raise ValueError('esperaba %r en %d' % (c, self.i))
        self.i += 1

    def typeref(self):
        c = self.peek()
        if c == '(':
            self.i = self.s.index(')', self.i) + 1
        elif c.isdigit() or c == '-':
            self.num()
        else:
            self.typedef()
            return
        if self.peek() == '=':
            self.i += 1
            self.typedef()

    def typedef(self):
        c = self.peek()
        self.i += 1
        if c in '*&kBf':
            self.typeref()
        elif c == '@':
            if self.peek() in 'sa':
                self.i = self.s.index(';', self.i) + 1
                self.typeref()
            else:
                self.typeref()
                self.expect(',')
                self.typeref()
        elif c in 'ar':
            if c == 'a':
                self.expect('r')
            self.typeref()
            self.expect(';')
            self.num()
            self.expect(';')
            self.num()
            self.expect(';')
            if c == 'a':
                self.typeref()
        elif c == 'e':
            while self.peek() != ';':
                self.i = self.s.index(',', self.i) + 1
            self.i += 1
        elif c == 'x':
            self.i = self.s.index(':', self.i) + 1
        elif c in 'su':
            self.estructura(anidada=True)
        elif c == '#':
            if self.peek() == '#':
                self.i += 1
                self.typeref()
                self.expect(';')
            else:
                self.typeref()
                while self.peek() == ',':
                    self.i += 1
                    self.typeref()
                self.expect(';')
        elif c == '-':
            self.num()
        elif c.isdigit():
            self.i -= 1
            self.typeref()
        else:
            raise ValueError('typedef %r en %d' % (c, self.i))

    def estructura(self, anidada=False):
        tam = self.num()
        campos = []
        if self.peek() == '!':
            self.i += 1
            n = self.num()
            self.expect(',')
            for _ in range(n):
                self.i += 2
                self.num()
                self.expect(',')
                self.typeref()
                self.expect(';')
        while self.peek() and self.peek() != ';':
            j = self.s.index(':', self.i)
            nom = self.s[self.i:j]
            if self.s[j + 1:j + 2] == ':':          # empiezan los metodos
                break
            self.i = j + 1
            if self.peek() == '/':
                self.i += 2
            self.typeref()
            if self.peek() == ':':                  # estatico: nombre fisico
                self.i = self.s.index(';', self.i) + 1
                continue
            self.expect(',')
            off = self.num()
            if self.peek() == ';':                  # `$vfN:tipo,offset;`
                bits = 32
            else:
                self.expect(',')
                bits = self.num()
            self.expect(';')
            campos.append((nom, off, bits))
        if anidada:
            if self.peek() == ';':
                self.i += 1
            else:
                k = self.s.find(';;', self.i)
                self.i = (k + 2) if k >= 0 else len(self.s)
        return tam, campos


# nombres decorados de GCC 2.9 -> C++
BUILTIN = {'i': 'int', 'Ui': 'unsigned int', 'c': 'char', 'Uc': 'unsigned char', 'Sc': 'signed char',
           's': 'short', 'Us': 'unsigned short', 'l': 'long', 'Ul': 'unsigned long', 'f': 'float', 'd': 'double',
           'b': 'bool', 'v': 'void', 'x': 'long long', 'Ux': 'unsigned long long', 'w': 'wchar_t'}


def demangle_tipo(s, i):
    """tipo decorado en s[i:] -> (texto C++, j). Solo lo que sale en argumentos de plantilla."""
    pre = ''
    while i < len(s) and s[i] in 'PRC':
        pre = {'P': '*', 'R': '&', 'C': 'const '}[s[i]] + pre
        i += 1
    for k in ('Ux', 'Ui', 'Uc', 'Us', 'Ul', 'Sc', 'i', 'c', 's', 'l', 'f', 'd', 'b', 'v', 'x', 'w'):
        if s.startswith(k, i):
            base, i = BUILTIN[k], i + len(k)
            break
    else:
        base, i = demangle_nombre(s, i)
    cst = 'const ' if pre.startswith('const ') else ''
    return cst + base + (' ' + pre.replace('const ', '') if pre.replace('const ', '') else ''), i


def demangle_componente(s, i):
    if s[i] == 't':                                  # plantilla: t<len><nombre><nargs><args>
        m = re.compile(r't(\d+)').match(s, i)
        n = int(m.group(1))
        nombre = s[m.end():m.end() + n]
        i = m.end() + n
        nargs = int(s[i])
        i += 1
        args = []
        for _ in range(nargs):
            if s[i] == 'Z':
                t, i = demangle_tipo(s, i + 1)
                args.append(t)
            else:                                    # constante: <tipo><valor>, m = negativo
                _, i = demangle_tipo(s, i)
                m = re.compile(r'(m?)(\d+)').match(s, i)
                args.append(('-' if m.group(1) else '') + m.group(2))
                i = m.end()
        return '%s<%s >' % (nombre, ', '.join(args)), i
    m = re.compile(r'(\d+)').match(s, i)
    n = int(m.group(1))
    return s[m.end():m.end() + n], m.end() + n


def demangle_nombre(s, i):
    if s[i] == 'Q':
        if s[i + 1] == '_':
            m = re.compile(r'Q_(\d+)_').match(s, i)
            n, i = int(m.group(1)), m.end()
        else:
            n, i = int(s[i + 1]), i + 2
        partes = []
        for _ in range(n):
            p, i = demangle_componente(s, i)
            partes.append(p)
        return '::'.join(partes), i
    return demangle_componente(s, i)


def calificado(nombre, cuerpo):
    """El nombre calificado sale del operator= implicito (`__as::...;:...RC<clase>;`), que toda clase C++ tiene."""
    m = re.search(r'__as::[^;]*;:[^;]*?RC((?:Q|t|\d)[^;]*);', cuerpo)
    if m:
        try:
            q, _ = demangle_nombre(m.group(1), 0)
            if q.split('::')[-1].split('<')[0] == nombre.split('<')[0]:
                return q
        except (ValueError, IndexError, AttributeError):
            pass
    return None


def lee_stabs():
    import mdebug
    with open(mdebug.ELF, 'rb') as f:
        f.seek(mdebug.SS_OFF)
        ss = f.read(mdebug.SS_SIZE + 0x2000000)
    clases = {}
    for m in re.finditer(rb'(?<=\0)([A-Za-z_][\w<>,:\s\*&]*?):T[t]?(\d+)=s(?=\d)', ss):
        nombre = m.group(1).decode('latin-1')
        fin = ss.index(b'\0', m.end())
        s = ss[m.end():fin].decode('latin-1')
        p = fin
        while s.endswith('\\'):
            q = ss.index(b'\0', p + 1)
            s = s[:-1] + ss[p + 1:q].decode('latin-1')
            p = q
        try:
            tam, campos = Stabs(s).estructura()
        except (ValueError, IndexError):
            continue
        q = calificado(nombre, s) or nombre
        # el puntero a vtable se llama `$vf<numero de tipo>`, y el numero cambia en cada unidad
        huella = (tam, tuple((re.sub(r'^\$vf\d+$', '$vf', n), o, b) for n, o, b in campos))
        clases.setdefault(q, {}).setdefault(huella, 0)
        clases[q][huella] += 1
    return clases


# ---------------------------------------------------------------- 2. donde vive cada clase
def indice_definiciones():
    """nombre corto -> ficheros de nuestro arbol que lo DEFINEN (class/struct X ... {)."""
    pat = re.compile(r'^\s*(?:template\s*<[^>]*>\s*)?(?:class|struct)\s+(?:\w+\s+)?(\w+)\s*(?::[^;{]*)?\{', re.M)
    idx = {}
    for base in ('src', 'include'):
        for r, _, ff in os.walk(base):
            if 'GameCube' in r or 'XENON' in r.upper() or 'PSX2' in r.upper():
                pass
            for f in ff:
                if f.endswith(('.h', '.hpp', '.cpp', '.hh')):
                    p = os.path.join(r, f).replace(os.sep, '/')
                    try:
                        t = open(p, encoding='utf-8', errors='replace').read()
                    except OSError:
                        continue
                    for m in pat.finditer(t):
                        idx.setdefault(m.group(1), set()).add(p)
    return idx


def cierre_includes(sl, incdirs):
    """Ficheros que una SourceList compila de verdad en PS2: los marcadores de linea de su preprocesado (el que
    tools/ee_gcc_pp.py deja en build/<V>/pp), que ya tienen los #if evaluados. zPlatform, por ejemplo, no incluye
    NADA en PS2. Si no hay preprocesado, el cierre de #include "..." por texto, por exceso."""
    pp = os.path.join('build', V, 'pp', SL_DIR, sl)
    if os.path.exists(pp):
        vistos = set()
        with open(pp, encoding='utf-8', errors='replace') as f:
            for l in f:
                if l.startswith('# '):
                    m = re.match(r'# \d+ "([^"]+)"', l)
                    if m:
                        vistos.add(os.path.normpath(m.group(1).replace('\\\\', '/')).replace(os.sep, '/'))
        return vistos
    vistos, pila = set(), [os.path.join(SL_DIR, sl).replace(os.sep, '/')]
    pat = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
    while pila:
        p = pila.pop()
        if p in vistos:
            continue
        vistos.add(p)
        try:
            t = open(p, encoding='utf-8', errors='replace').read()
        except OSError:
            continue
        for inc in pat.findall(t):
            for d in [os.path.dirname(p)] + incdirs:
                c = os.path.normpath(os.path.join(d, inc)).replace(os.sep, '/')
                if os.path.isfile(c):
                    pila.append(c)
                    break
    return vistos


# ---------------------------------------------------------------- 3. sondear
def flags_por_sl():
    cache = os.path.join(DIR, 'flags.json')
    if os.path.exists(cache):
        return json.load(open(cache))
    subprocess.run([PY, 'configure.py', '--version', V, '--dtk', 'build/tools/dtk.exe',
                    '--objdiff', 'build/tools/objdiff-cli.exe'], capture_output=True)
    try:
        crudo = open('build.ninja', encoding='utf-8', errors='replace').read().split('\n')
    finally:
        subprocess.run([PY, 'configure.py'], capture_output=True)
    # juntar las continuaciones `$`: las unidades con varias salidas parten la linea `build` en dos
    ln, acum = [], ''
    for l in crudo:
        if l.rstrip().endswith('$'):
            acum += l.rstrip()[:-1] + ' '
            continue
        ln.append(acum + (l.strip() if acum else l))
        acum = ''
    res = {}
    for i, l in enumerate(ln):
        m = re.match(r'build build\\' + re.escape(V) + r'\\src\\Speed\\Indep\\SourceLists\\(\w+)\.o\b.*?:\s*ee-gcc', l)
        if not m:
            continue
        for j in range(i + 1, min(i + 12, len(ln))):
            if ln[j].strip().startswith('cflags ='):
                res[m.group(1) + '.cpp'] = ln[j].split('=', 1)[1].split()
                break
    os.makedirs(DIR, exist_ok=True)
    json.dump(res, open(cache, 'w'))
    return res


def incdirs_de(flags):
    out, i = [], 0
    while i < len(flags):
        if flags[i] == '-I' and i + 1 < len(flags):
            out.append(flags[i + 1].replace('\\', '/'))
            i += 2
            continue
        if flags[i].startswith('-I') and len(flags[i]) > 2:
            out.append(flags[i][2:].replace('\\', '/'))
        i += 1
    return out


def compila(sl, flags, lineas, grupo):
    """Compila la sonda de una SourceList; quita las lineas con error y reintenta. -> {n: valor}, quitadas, error

    El texto de la SourceList va DENTRO de la sonda, no incluido: zFe y zCamera abren con un `#pragma
    implementation`, que GCC solo acepta en el fichero principal. Y un error quita la CLASE entera (grupo[n]):
    tras una plantilla invalida GCC 2.9 no informa de las lineas siguientes, y quitando linea a linea hacian
    falta decenas de vueltas."""
    cc = os.path.normpath('build/compilers/PS2/ee-gcc2.9-991111/bin/ee-gcc.exe')
    # con --clase/--solo el nombre lleva el PID: varias sondas parciales pueden correr a la vez sin pisarse
    nombre = 'sonda_' + sl.replace('.cpp', '') + ('_%d' % os.getpid() if PARCIAL else '')
    src, out = os.path.join(DIR, nombre + '.cpp'), os.path.join(DIR, nombre + '.s')
    cuerpo = open(os.path.join(SL_DIR, sl), encoding='utf-8', errors='replace').read().split('\n')
    cab = ['#define private public', '#define protected public'] + [l.rstrip('\r') for l in cuerpo]
    vivas = dict(enumerate(lineas))
    quitadas = set()
    for intento in range(40):
        texto = cab + [vivas[n] for n in sorted(vivas)]
        pos = {len(cab) + 1 + k: n for k, n in enumerate(sorted(vivas))}
        open(src, 'w').write('\n'.join(texto) + '\n')
        if os.path.exists(out):
            os.remove(out)
        r = subprocess.run([PY, os.path.normpath('tools/ee_gcc_pp.py'), os.path.normpath('build/%s/pp' % V), cc,
                            '--'] + flags + ['-x', 'c++', '-S', '-o', out, src], capture_output=True, text=True,
                           errors='replace')
        if r.returncode == 0 and os.path.exists(out):
            s = open(out, encoding='utf-8', errors='replace').read()
            val = {}
            for m in re.finditer(r'^sonda_(\d+):\s*\n\s*\.word\s+(-?\w+)', s, re.M):
                v = m.group(2)
                val[int(m.group(1))] = int(v, 16) if v.startswith('0x') else int(v)
            return val, quitadas, None
        malas = set()
        for l in (r.stderr or '').split('\n'):
            m = re.search(re.escape(nombre) + r'\.cpp:(\d+): (?!warning)', l)
            if m and int(m.group(1)) in pos:
                malas.add(pos[int(m.group(1))])
        if not malas:
            err = [l for l in (r.stderr or '').split('\n') if l.strip() and 'warning' not in l][:6]
            return {}, quitadas, err
        clases_malas = {grupo[n] for n in malas}
        for n in [k for k in vivas if grupo[k] in clases_malas]:
            vivas.pop(n)
            quitadas.add(n)
    return {}, quitadas, ['demasiados reintentos']


def main():
    args = sys.argv[1:]
    if args and args[0] in ('-h', '--help'):
        print(__doc__)
        return
    solo = args[args.index('--solo') + 1].split(',') if '--solo' in args else None
    solo_clase = set(args[args.index('--clase') + 1].split(',')) if '--clase' in args else None
    plantillas = '--plantillas' in args
    hilos = int(args[args.index('-j') + 1]) if '-j' in args else 6
    os.makedirs(DIR, exist_ok=True)

    cache = os.path.join(DIR, 'stabs.pkl')
    if os.path.exists(cache):
        clases = pickle.load(open(cache, 'rb'))
    else:
        clases = lee_stabs()
        pickle.dump(clases, open(cache, 'wb'))
    flags = flags_por_sl()
    sls = sorted(flags)
    incdirs = incdirs_de(flags[sls[0]])
    cierres = {sl: cierre_includes(sl, incdirs) for sl in sls}
    idx = indice_definiciones()

    # 3. que sondear y desde donde
    porsl = {}
    carga = {sl: 0 for sl in sls}
    texto = {}
    sin_def, sin_sl, ambiguas = [], [], []
    for q, huellas in sorted(clases.items()):
        corto = q.split('::')[-1].split('<')[0]
        if solo_clase and corto not in solo_clase and q not in solo_clase:
            continue
        if len(huellas) > 1:                          # dos layouts con el mismo nombre sin calificar
            ambiguas.append(q)
            continue
        if '<' in q and not plantillas:               # casi todas STL o sin instanciar: con --plantillas
            continue
        (tam, campos), = huellas.keys()
        defs = idx.get(corto)
        if defs and '::' in q:                        # A::B: el fichero que define B tiene que nombrar A
            fuera = q.split('::')[-2].split('<')[0]
            for p in list(defs):
                if p not in texto:
                    texto[p] = open(p, encoding='utf-8', errors='replace').read()
            defs = {p for p in defs if re.search(r'\b%s\b' % re.escape(fuera), texto[p])}
        if not defs:
            sin_def.append(q)
            continue
        cand = [sl for sl in sls if cierres[sl] & defs]
        if solo:
            cand = [sl for sl in cand if sl in solo]
        if not cand:
            sin_sl.append(q)
            continue
        sl = min(cand, key=lambda x: carga[x])        # repartir: una sonda enorme tarda mas en cada reintento
        carga[sl] += 1 + len(campos)
        porsl.setdefault(sl, []).append((q, tam, campos))

    def trabajo(sl):
        lineas, meta, grupo = [], [], []
        for k, (q, tam, campos) in enumerate(porsl[sl]):
            lineas.append('int sonda_%d = sizeof(%s);' % (len(lineas), q))
            meta.append((q, None, tam))
            grupo.append(k)
            for nom, off, bits in campos:
                if not nom or nom.startswith('$') or off % 8 or bits % 8:
                    continue
                lineas.append('int sonda_%d = (int)&((%s *)0x1000)->%s - 0x1000;' % (len(lineas), q, nom))
                meta.append((q, nom, off // 8))
                grupo.append(k)
        val, quitadas, err = compila(sl, flags[sl], lineas, grupo)
        return sl, meta, val, quitadas, err

    resultados = []
    with ThreadPoolExecutor(hilos) as ex:
        for sl, meta, val, quitadas, err in ex.map(trabajo, sorted(porsl)):
            resultados.append((sl, meta, val, quitadas, err))
            print('  %-24s %5d sondas, %4d quitadas%s' % (sl, len(meta), len(quitadas),
                                                          ('  NO COMPILA: ' + ' | '.join(err)[:200]) if err else ''),
                  flush=True)

    # 4. informe
    malas, buenas, fuera = {}, 0, []
    for sl, meta, val, quitadas, err in resultados:
        porclase = {}
        for n, (q, nom, orig) in enumerate(meta):
            if n not in val:
                continue
            porclase.setdefault(q, []).append((nom, orig, val[n]))
        for q, datos in porclase.items():
            mal = [d for d in datos if d[1] != d[2]]
            if mal:
                malas[q] = (sl, datos, mal)
            else:
                buenas += 1
        fuera += [(sl, q) for q in sorted({meta[n][0] for n in quitadas} - set(porclase))]
        if err:
            fuera += [(sl, q) for q in sorted({m[0] for m in meta} - set(porclase) - {meta[n][0] for n in quitadas})]

    def primero_mal(datos):
        """el PRIMER campo que no cuadra (la causa suele estar justo antes); el sizeof solo si todos cuadran"""
        campos = [d for d in datos if d[0] is not None]
        return next((d for d in campos if d[1] != d[2]), None) or next(d for d in datos if d[1] != d[2])

    # las corridas con filtro no pisan el informe de la pasada completa
    sufijo = '_parcial' if (solo or solo_clase) else ''
    informe = os.path.join(DIR, 'informe%s.txt' % sufijo)
    with open(informe, 'w', encoding='utf-8') as f:
        for q, (sl, datos, mal) in sorted(malas.items(), key=lambda x: (x[1][0], x[0])):
            p = primero_mal(datos)
            tam = next((d for d in datos if d[0] is None), None)
            f.write('%-18s %-50s primer campo mal: %-26s orig 0x%03x nuestro 0x%03x  (%d mal de %d)%s\n' % (
                sl, q[:50], p[0] or 'sizeof', p[1], p[2], len(mal), len(datos),
                ('  sizeof orig 0x%x nuestro 0x%x' % (tam[1], tam[2])) if tam and tam[1] != tam[2] else ''))
    with open(os.path.join(DIR, 'no_sondeadas%s.txt' % sufijo), 'w', encoding='utf-8') as f:
        for sl, q in fuera:
            f.write('%-18s %s\n' % (sl, q))
    print('\nclases que cuadran: %d' % buenas)
    print('clases con algun campo mal: %d  (detalle en %s)' % (len(malas), informe))
    print('asignadas pero sin compilar en la sonda: %d  (lista en %s)' % (len(fuera),
                                                                         os.path.join(DIR, 'no_sondeadas%s.txt' % sufijo)))
    print('sin definicion en nuestro arbol: %d; sin SourceList que las vea: %d; con dos layouts en los stabs: %d'
          % (len(sin_def), len(sin_sl), len(ambiguas)))
    for q, (sl, datos, mal) in sorted(malas.items(), key=lambda x: -len(x[1][2]))[:40]:
        p = primero_mal(datos)
        print('   %-16s %-44s %-24s orig 0x%03x nuestro 0x%03x  (%d/%d)' % (
            sl, q[:44], (p[0] or 'sizeof')[:24], p[1], p[2], len(mal), len(datos)))


if __name__ == '__main__':
    main()
