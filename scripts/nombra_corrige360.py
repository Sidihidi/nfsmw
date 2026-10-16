#!/usr/bin/env python3
"""nombra_corrige360.py -- regla de NOMBRE EQUIVOCADO para Xbox 360 sobre el enlace LTCG (la hermana de PC es
scripts/nombra_corrige.py, que trabaja con symbols.json y no sirve para 360).

nombra_grafo.py solo pone nombre a funciones SIN nombre y solo cuando el esqueleto es unico o lo confirman sus destinos.
Quedan dos bolsas que esta regla resuelve con la evidencia de los LLAMANTES:
  - nombres FALSOS en config/EUROPEGERMILESTONE/symbols.txt: con LTCG, una funcion pequena (envoltorio) se expande en su
    llamante y el nombre del envoltorio se queda en la funcion que de verdad se llama. Ejemplo (ronda 40): la funcion
    de zAI que se llamaba `stringhash32` guarda el hash en *this y devuelve this: es `UCrc32::UCrc32(const char *)`.
  - funciones sin nombre con varias candidatas del mismo esqueleto (instancias de plantilla), que solo los llamantes
    distinguen.

Evidencia: cada ANCLA (funcion que el informe LTCG da al 100 % con su nombre) llama, en su instruccion k, a una direccion
T del original (se lee el `bl` del ejecutable original); la nuestra, en la misma instruccion, llama a B (la reubicacion
de nuestro objeto troceado). Se acepta T -> B si:
  - todas las anclas que llaman a T dicen el mismo B (unanimidad), y T no es ya un ancla;
  - B esta definido en nuestro objeto de la unidad de T y no esta en otra direccion de esa unidad;
  - el CODIGO del original en T es el de nuestra B: objdiff al 100 % (con un nombre marcador en las dos copias).
Validacion (se imprime siempre): la misma evidencia unanime sobre las direcciones que YA son anclas tiene que decir su
propio nombre. Ronda 40: 1.442 bien y 8 "mal", y las 8 son alias, no errores: `??_E`/`??_G` de un mismo destructor en
la misma direccion (nuestro troceado elige uno) y un `make_empty` de dos Vector<T> con el mismo codigo.

Entradas (por defecto, las de la ultima medida oficial):
  --informe F     informe LTCG de pctall (build/versiones/EUROPEGERMILESTONE.json) para saber que casa al 100 %;
                  o --banco F con el formato {unidad: {funcion: [pct, tam]}}.
  --nuestros DIR  objetos troceados del enlace LTCG (build/EUROPEGERMILESTONE/ltcg/obj)
  --originales DIR  originales normalizados (build/EUROPEGERMILESTONE/ltcg/orig)
  --raiz DIR      donde estan config/ y orig/ (el repo)

    python scripts/nombra_corrige360.py                  # informa
    python scripts/nombra_corrige360.py --tsv SALIDA.tsv # las correcciones (direccion, nombre viejo, nuevo, unidad)
    python scripts/nombra_corrige360.py --aplicar        # cambia symbols.txt

Despues de aplicar: medir 360 (scripts/medir.py EUROPEGERMILESTONE) y volver a pasar nombra_grafo.py con
--nuestros=build/EUROPEGERMILESTONE/ltcg/obj y esta regla hasta que no salga nada.
"""
import collections
import json
import os
import re
import struct
import subprocess
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import ltcg360  # noqa: E402
from escritura import entero  # noqa: E402

V = 'EUROPEGERMILESTONE'
BASE = 0x82000000  # orig/EUROPEGERMILESTONE/NFS.exe: VA 0x82000600 = RVA 0x600
SIN_NOMBRE = re.compile(r'^(fn|lbl|sub|func)_[0-9A-F]{8}$|^ltcg_')


def opcion(nombre, defecto):
    a = sys.argv[1:]
    for i, x in enumerate(a):
        if x == '--' + nombre and i + 1 < len(a):
            return a[i + 1]
        if x.startswith('--%s=' % nombre):
            return x.split('=', 1)[1]
    return defecto


def renombra(d, cambios):
    """Copia del COFF con otros nombres de simbolo (reescribe la tabla de cadenas)."""
    d = bytearray(d)
    _, nsec, _, psym, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
    cad = psym + nsym * 18
    tam_cad = struct.unpack_from('<I', d, cad)[0]
    tabla = bytes(d[cad:cad + tam_cad])
    nuevas = bytearray(tabla)
    i = 0
    while i < nsym:
        o = psym + i * 18
        n = bytes(d[o:o + 8])
        if n[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', n, 4)[0]
            nombre = tabla[off:tabla.index(b'\0', off)]
        else:
            nombre = n.rstrip(b'\0')
        s = nombre.decode('latin-1')
        if s in cambios:
            nuevo = cambios[s].encode('latin-1')
            if len(nuevo) <= 8:
                d[o:o + 8] = nuevo.ljust(8, b'\0')
            else:
                d[o:o + 8] = b'\0\0\0\0' + struct.pack('<I', len(nuevas))
                nuevas += nuevo + b'\0'
        i += 1 + d[o + 17]
    struct.pack_into('<I', nuevas, 0, len(nuevas))
    return bytes(d[:cad]) + bytes(nuevas) + bytes(d[cad + tam_cad:])


def casadas_de(args_banco, args_informe):
    """{unidad: {funcion: pct}} (unidad = nombre corto de la SourceList)."""
    if args_banco:
        b = json.load(open(args_banco, encoding='utf-8'))
        return {u: {f: v[0] for f, v in fs.items()} for u, fs in b.items()}
    d = json.load(open(args_informe, encoding='utf-8'))
    out = {}
    for u in d['units']:
        un = u['name'].split('/')[-1]
        out[un] = {f['name']: f.get('fuzzy_match_percent') for f in u.get('functions', [])}
    return out


def main():
    raiz = os.path.abspath(opcion('raiz', RAIZ))
    nuestros = os.path.abspath(opcion('nuestros', os.path.join(RAIZ, 'build', V, 'ltcg', 'obj')))
    originales = os.path.abspath(opcion('originales', os.path.join(RAIZ, 'build', V, 'ltcg', 'orig')))
    res = casadas_de(opcion('banco', None), opcion('informe', os.path.join(RAIZ, 'build', 'versiones', V + '.json')))
    objdiff = os.path.join(RAIZ, 'build', 'tools', 'objdiff-cli.exe')

    sims = os.path.join(raiz, 'config', V, 'symbols.txt')
    lineas = open(sims, encoding='utf-8').read().split('\n')
    nombres_en = collections.defaultdict(list)
    dirs_de = collections.defaultdict(list)
    for i, l in enumerate(lineas):
        m = re.match(r'(\S+) = \.text:0x([0-9A-Fa-f]+);', l)
        if m:
            a = int(m.group(2), 16)
            nombres_en[a].append((m.group(1), i))
            dirs_de[m.group(1)].append(a)
    rangos = []
    u = None
    for l in open(os.path.join(raiz, 'config', V, 'splits.txt'), encoding='utf-8'):
        if l and not l[0].isspace() and l.rstrip().endswith(':'):
            u = l.strip()[:-1].rsplit('/', 1)[-1].rsplit('.', 1)[0]
            continue
        m = re.match(r'\s+\.text\s+start:0x([0-9A-F]+) end:0x([0-9A-F]+)', l)
        if m and u and u != 'Sections':
            rangos.append((int(m.group(1), 16), int(m.group(2), 16), u))

    def unidad_de(a):
        for x, y, un in rangos:
            if x <= a < y:
                return un
        return None
    pe = ltcg360.PE(os.path.join(raiz, 'orig', V, 'NFS.exe'))

    def palabra(a):
        return struct.unpack('>I', pe.lee(a - BASE, 4))[0]

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

    anclas_dir = set()
    for un, fs in res.items():
        for n, pct in fs.items():
            if pct == 100.0:
                anclas_dir.update(a for a in dirs_de.get(n, []) if unidad_de(a) == un)

    evid = collections.defaultdict(collections.Counter)
    for un, fs in res.items():
        if un not in nuestro:
            continue
        fun, r = nuestro[un]
        for n, pct in fs.items():
            if pct != 100.0 or n not in fun:
                continue
            ds = [a for a in dirs_de.get(n, []) if unidad_de(a) == un]
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

    bien = mal = 0
    for t, c in evid.items():
        if t in anclas_dir and len(c) == 1:
            if next(iter(c)) in [x for x, _ in nombres_en[t]]:
                bien += 1
            else:
                mal += 1
    print('validacion (evidencia unanime sobre anclas): bien %d, distinta %d (alias ??_E/??_G y codigo plegado)' % (
        bien, mal))

    por_u = collections.defaultdict(list)
    for t, c in evid.items():
        if len(c) != 1 or t in anclas_dir:
            continue
        b = next(iter(c))
        viejos = [x for x, _ in nombres_en[t]]
        un = unidad_de(t)
        if b in viejos or un not in nuestro or b not in nuestro[un][0]:
            continue
        if [a for a in dirs_de.get(b, []) if unidad_de(a) == un and a != t]:
            continue
        por_u[un].append((t, b, viejos, c[b]))
    ok = []
    tmp = os.path.join(RAIZ, 'build', V, 'ltcg', 'corrige360')
    os.makedirs(tmp, exist_ok=True)
    for un, lista in sorted(por_u.items()):
        co, cn = {}, {}
        for t, b, viejos, _ in lista:
            mk = 'CORRIGE360_%08X' % t
            for v in viejos:
                if len([a for a in dirs_de[v] if unidad_de(a) == un]) == 1:
                    co[v] = mk
            cn[b] = mk
        to, tn, sal = (os.path.join(tmp, x) for x in ('o.obj', 'n.obj', 'd.json'))
        open(to, 'wb').write(renombra(open(obj_de(originales, un), 'rb').read(), co))
        open(tn, 'wb').write(renombra(open(obj_de(nuestros, un), 'rb').read(), cn))
        subprocess.run([objdiff, 'diff', '-1', to, '-2', tn, '-c', 'functionRelocDiffs=none',
                        '-c', 'ppc.calculatePoolRelocations=false', '-o', sal, '--format', 'json'],
                       capture_output=True, text=True)
        pct = {s.get('name'): s.get('match_percent') for s in json.load(open(sal, encoding='utf-8'))['left']['symbols']}
        for t, b, viejos, nt in lista:
            if pct.get('CORRIGE360_%08X' % t) == 100.0:
                ok.append((t, b, viejos, un, nt))
        for f in (to, tn, sal):
            os.remove(f)
    os.rmdir(tmp)
    viejos_con_nombre = [x for x in ok if not all(SIN_NOMBRE.match(v) for v in x[2])]
    print('correcciones (codigo 100 %%): %d (%d cambian un nombre puesto, %d nombran un fn_)' % (
        len(ok), len(viejos_con_nombre), len(ok) - len(viejos_con_nombre)))
    for t, b, viejos, un, nt in sorted(ok, key=lambda x: (x[3], x[0])):
        print('  %-18s 0x%08X %-40s -> %s  (%d llamantes)' % (un, t, ','.join(viejos)[:40], b[:90], nt))
    tsv = opcion('tsv', None)
    if tsv:
        with open(tsv, 'w', encoding='utf-8') as fh:
            for t, b, viejos, un, nt in ok:
                fh.write('0x%08X\t%s\t%s\t%s\t%d\n' % (t, ','.join(viejos), b, un, nt))
    if '--aplicar' in sys.argv:
        for t, b, viejos, un, nt in ok:
            primero = True
            for v, i in nombres_en[t]:
                assert lineas[i].startswith(v + ' = .text:'), lineas[i]
                if primero:
                    lineas[i] = b + lineas[i][len(v):]
                    primero = False
        entero(sims, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
        print('aplicados %d' % len(ok))
    return 0


if __name__ == '__main__':
    sys.exit(main())
