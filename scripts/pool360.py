#!/usr/bin/env python3
"""pool360.py [INFORME.json] [--lista N] [--fn SUBCADENA] -- cuantas casi exactas de Xbox 360 (medida LTCG) difieren
SOLO en como cargan las constantes del pool (ronda 84, X360-B).

En los dos lados (build/EUROPEGERMILESTONE/ltcg/orig y ltcg/obj, los de la medida oficial) se quitan los `lis` (REFHI)
y `addi` (REFLO) que fijan una base del pool y las cargas desde esa base se reducen a `op dst, POOL`. Clases:
  igual     la secuencia normalizada es identica (solo cambia el direccionamiento del pool)
  orden     mismo multiconjunto (pool + otro orden de las instrucciones, que el pool arrastra)
  regs      mismo multiconjunto de mnemonicos (pool + orden + registros)
  sinpool   como regs pero con el MISMO numero de lis/addi del pool: la diferencia no es del pool
y una linea `nuestro frente al EGM` con cuantas tienen mas o menos lis/addi que el original. --fn imprime las dos
secuencias normalizadas de las funciones cuyo nombre contiene SUBCADENA.

Lo que mide (ronda 84): LTCG funde un literal con un dato de solo lectura del mismo valor que ya conoce (una tabla
static const de otra unidad) y lo carga relativo a ese dato; que dato recibe cada literal depende del programa
entero, asi que estas diferencias convergen al escribir lo que falta (docs/analisis/2026-10-22_x360b84.md).

    python scripts/pool360.py --lista 80          # desde la raiz, tras una medida de X360
"""
import collections, json, os, re, sys
import capstone
sys.path.insert(0, 'scripts')
import ltcg360
V = 'EUROPEGERMILESTONE'
MD = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)
MD.skipdata = True
CARGA = re.compile(r'^(\w+), (-?(?:0x)?[0-9a-f]+)\((r\d+)\)$')
NOESCR = ('stw', 'stb', 'sth', 'stfs', 'stfd', 'cmp', 'stv', 'dcb', 'mt', 'tw')


def funciones(ruta):
    d, secs, sims = ltcg360.lee_coff(ruta)
    out = {}
    for k, (nombre, praw, tam, rels) in enumerate(secs):
        if nombre != '.text':
            continue
        tipos = collections.defaultdict(set)
        for off, _, t in rels:
            tipos[off].add(t)
        lista = sorted((v, nm) for nm, v, s, cl in sims.values() if s == k + 1 and cl in (2, 3)
                       and not nm.startswith('.') and not nm.startswith('$'))
        inis = sorted({v for v, _ in lista}) + [tam]
        for v, nm in lista:
            fin = min(x for x in inis if x > v) if v < tam else tam
            out.setdefault(nm, (d, praw, fin - v, v, tipos))
    return out


def normaliza(f, size=None):
    d, praw, tam, v, tipos = f
    code = d[praw + v:praw + v + (size or tam)]
    pool = set()
    res = []
    global CUENTA
    CUENTA = [0, 0, 0]
    for i in MD.disasm(code, 0):
        t = tipos.get(v + i.address, set())
        ops = i.op_str
        m = i.mnemonic
        a = [x.strip() for x in ops.split(',')]
        if m == 'lis' and ltcg360.R_REFHI in t:
            pool.add(a[0]); CUENTA[0] += 1; continue
        if m == 'addi' and ltcg360.R_REFLO in t and a[1] in pool:
            pool.add(a[0]); CUENTA[1] += 1; continue
        mm = CARGA.match(ops)
        if mm and (mm.group(3) in pool or ltcg360.R_REFLO in t) and not m.startswith('st'):
            res.append('%s %s, POOL' % (m, mm.group(1))); CUENTA[2] += 1
            if mm.group(1) in pool:
                pool.discard(mm.group(1))
            continue
        if a and a[0] in pool and not m.startswith(NOESCR):
            pool.discard(a[0])
        res.append('%s %s' % (m, ops))
    return res


SENTIDO = collections.Counter()


def main():
    inf = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith('--') else 'build/versiones/%s.json' % V
    n = int(sys.argv[sys.argv.index('--lista') + 1]) if '--lista' in sys.argv else 0
    d = json.load(open(inf, encoding='utf-8'))
    cat = collections.Counter(); byt = collections.Counter(); filas = []
    for u in d['units']:
        rel = u['name'].split('/', 1)[1] + '.obj'
        on = os.path.join('build', V, 'ltcg', 'obj', rel)
        oo = os.path.join('build', V, 'ltcg', 'orig', rel)
        if not (os.path.exists(on) and os.path.exists(oo)):
            continue
        fn, fo = funciones(on), funciones(oo)
        for f in u.get('functions', []):
            p = f.get('fuzzy_match_percent')
            if p is None or p == 100.0:
                continue
            nm, size = f['name'], int(f['size'])
            if nm not in fn or nm not in fo:
                continue
            a = normaliza(fo[nm], size); ca = tuple(CUENTA)
            b = normaliza(fn[nm]); cb = tuple(CUENTA)
            while b and b[-1].startswith('.long 0'):
                b.pop()
            while a and a[-1].startswith('.long 0'):
                a.pop()
            if '--fn' in sys.argv and sys.argv[sys.argv.index('--fn') + 1] in nm:
                print(nm, p)
                import itertools
                for x, y in itertools.zip_longest(a, b, fillvalue=''):
                    print('   %-40s %s %s' % (x, '  ' if x == y else '<>', y))
            if a == b:
                c = 'igual'
            elif sorted(a) == sorted(b):
                c = 'orden'
            elif sorted(x.split()[0] for x in a) == sorted(x.split()[0] for x in b):
                c = 'regs'
            else:
                continue
            if c != 'igual' and ca[:2] == cb[:2]:
                c = 'sinpool'
            cat[c] += 1; byt[c] += size
            filas.append((c, p, size, u['name'].split('/')[-1], nm + '  %s>%s' % (ca[:2], cb[:2])))
            if c != 'sinpool':
                dl, da = cb[0] - ca[0], cb[1] - ca[1]
                SENTIDO[('mas_lis' if dl > 0 else 'menos_lis' if dl < 0 else 'igual_lis') + '/' + ('mas_addi' if da > 0 else 'menos_addi' if da < 0 else 'igual_addi')] += 1
    for c in ('igual', 'orden', 'regs', 'sinpool'):
        print('%-6s %4d funciones %7d B' % (c, cat[c], byt[c]))
    print('nuestro frente al EGM:', SENTIDO.most_common())
    for r in sorted(filas, key=lambda r: (r[0], -r[2]))[:n]:
        print('  %-6s %6.2f %5d %-12s %s' % r)


main()
