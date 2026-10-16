#!/usr/bin/env python3
"""Nombres de SLUS por el GRAFO DE LLAMADAS de la A124 (cuyos nombres salen de la .symtab de su ELF).

Funciones testigo: las que tienen el MISMO nombre en las dos versiones, el mismo tamano y los j/jal en las mismas
posiciones. En cada testigo, el destino k-esimo de SLUS es la pareja del k-esimo de la A124; se juntan todas las
parejas (destino de SLUS -> nombres de la A124) y se informa de:
  - destinos de SLUS SIN nombre con un unico nombre de la A124 (y cuantos testigos lo dicen): nombres nuevos;
  - destinos de SLUS con un nombre DISTINTO del de la A124: nombre posiblemente mal puesto (o funciones con el mismo
    codigo en orden distinto, o una diferencia real entre versiones: la A124 es un prototipo);
  - conflictos (un destino de SLUS con varios nombres de la A124).

NINGUNA propuesta es prueba por si sola. Un testigo solo dice que la funcion de SLUS llama a esa direccion donde la
A124 llama a ese nombre; hay que probar cada cambio por codigo (scripts/prueba_nombre.py) y, si el codigo es igual en
las dos candidatas (destructores de plantilla, accesores), dejarlo: no cambia la medida y no se puede probar. Lo que
salio el 2026-09-23 de 84 discrepancias: la mitad eran pares de codigo identico (sin prueba), una docena diferencias
reales entre versiones (el nombre actual casa al 100 %), y unas diez se probaron (bRandom__Ff/Fi, AddHead/AddTail,
constructores de eventos anclados por su vtable: scripts/eventos_vtable.py).

    python scripts/grafo_nombres.py [--min-testigos 2] [--filtro subcadena] [--tsv nuevos.tsv] [--prueba]

--tsv vuelca los nombres nuevos (nombre, testigos, direccion de SLUS). --prueba compara, en cada discrepancia, nuestro
codigo con el nombre actual y con el de la A124 contra el original troceado (como scripts/prueba_nombre.py): si el
actual es IGUAL, el nombre se queda. Lee los troceados de las dos versiones (build/<V>/asm) y, con --prueba, los
objetos de SLUS (build/SLUS-21351/obj y src): tienen que ser recientes.
"""
import collections
import os
import re
import struct
import sys

from nombra_symtab import ficheros_asm

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
A, S = 'SLES-53558-A124', 'SLUS-21351'
SIN_NOMBRE = re.compile(r'^(fn|sub|func|lbl|zz|j_sub|nullsub)_[0-9A-Fa-f_]+$')
RE_INS = re.compile(r'/\* ([0-9A-Fa-f]+) ([0-9A-Fa-f]{8}) ([0-9A-Fa-f]{8}) \*/\s+(\S+)\s*(.*)')


def symtab(ruta):
    d = open(ruta, 'rb').read()
    shoff, = struct.unpack_from('<I', d, 0x20)
    shentsize, shnum, _ = struct.unpack_from('<HHH', d, 0x2e)
    secs = [struct.unpack_from('<IIIIIIIIII', d, shoff + i * shentsize) for i in range(shnum)]
    out = []
    for s in secs:
        if s[1] != 2:
            continue
        strt = secs[s[6]]
        for k in range(s[5] // 16):
            st_name, st_value, st_size, st_info, _, st_shndx = struct.unpack_from('<IIIBBH', d, s[4] + k * 16)
            b = strt[4] + st_name
            out.append((st_value, st_size, st_info, d[b:d.index(b'\0', b)].decode('latin-1')))
    return out


def simbolos(version):
    por_dir = collections.defaultdict(list)
    for l in open(os.path.join(RAIZ, 'config', version, 'symbols.txt'), encoding='utf-8', errors='replace'):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);', l)
        if m:
            por_dir[int(m.group(2), 16)].append(m.group(1))
    return por_dir


def indice(version):
    """[(nombre, inicio, tamano, [(k, op, destino)], unidad)] de los glabel del troceado"""
    fns = []
    for p, unidad in ficheros_asm(version):
        cur = None
        for l in open(p, encoding='utf-8', errors='replace'):
            m = re.match(r'\s*glabel (\S+)', l)
            if m:
                cur = [m.group(1), None, 0, [], unidad]
                fns.append(cur)
                continue
            m = RE_INS.search(l)
            if m and cur is not None:
                v = int(m.group(2), 16)
                if cur[1] is None:
                    cur[1] = v
                cur[2] = v + 4 - cur[1]
                if m.group(4) in ('jal', 'j'):
                    w = int.from_bytes(bytes.fromhex(m.group(3)), 'little')
                    cur[3].append(((v - cur[1]) // 4, m.group(4), ((w & 0x3FFFFFF) << 2) | (v & 0xF0000000)))
            if re.match(r'\s*endlabel', l):
                cur = None
    return [f for f in fns if f[1] is not None]


def main():
    mint = int(sys.argv[sys.argv.index('--min-testigos') + 1]) if '--min-testigos' in sys.argv else 2
    filtro = sys.argv[sys.argv.index('--filtro') + 1] if '--filtro' in sys.argv else None
    fs, fa = indice(S), indice(A)
    nom_a = collections.defaultdict(set)
    for v, t, info, n in symtab(os.path.join(RAIZ, 'orig', A, 'NFS.ELF')):
        if n and (info & 0xf) == 2 and not n.startswith(('gcc2', '__gnu')):
            nom_a[v].add(n)
    for v, ns in simbolos(A).items():
        nom_a[v] |= {n for n in ns if not SIN_NOMBRE.match(n)}
    por_dir_s = simbolos(S)
    a_por_nombre = collections.defaultdict(list)
    for n, v, t, js, u in fa:
        for x in ({n} if not SIN_NOMBRE.match(n) else set()) | nom_a.get(v, set()):
            a_por_nombre[x].append((v, t, js))
    s_por_nombre = collections.defaultdict(list)
    for n, v, t, js, u in fs:
        if not SIN_NOMBRE.match(n):
            s_por_nombre[n].append((v, t, js))
    pares = collections.defaultdict(collections.Counter)
    testigos = collections.defaultdict(list)
    usadas = 0
    for n, ls in s_por_nombre.items():
        la = a_por_nombre.get(n)
        if not la or len(ls) != 1 or len(la) != 1:
            continue
        (vs, ts, jss), (va, ta, jsa) = ls[0], la[0]
        if ts != ta or [(k, op) for k, op, d in jss] != [(k, op) for k, op, d in jsa]:
            continue
        usadas += 1
        for (k, op, ds), (_, _, da) in zip(jss, jsa):
            for x in nom_a.get(da, ()):
                pares[ds][x] += 1
            if nom_a.get(da):
                testigos[ds].append(n)
    print('funciones testigo (mismo nombre, tamano y posiciones de j/jal): %d' % usadas)
    nuevos, discrepan, conflictos = [], [], []
    for ds, c in pares.items():
        actual = [x for x in por_dir_s.get(ds, []) if not SIN_NOMBRE.match(x)]
        top = c.most_common()
        mx = top[0][1]
        cands = sorted(x for x, k in top if k == mx)      # varios nombres de la A124 en una direccion: alias
        if filtro and not any(filtro in x for x in cands + actual):
            continue
        if sum(k for x, k in top if k != mx):
            conflictos.append((ds, top, actual))
        elif not actual:
            if mx >= mint:
                nuevos.append((ds, cands, mx, testigos[ds][:4]))
        elif not set(actual) & set(cands):
            discrepan.append((ds, actual, cands, mx, testigos[ds][:4]))
    print('\n== destinos de SLUS sin nombre (>= %d testigos): %d' % (mint, len(nuevos)))
    for ds, cands, mx, t in sorted(nuevos):
        print('  0x%06x  %-60s x%d  (%s)' % (ds, ' | '.join(cands)[:60], mx, ', '.join(x[:30] for x in t)))
    print('\n== destinos de SLUS con nombre DISTINTO al de la A124: %d' % len(discrepan))
    prueba = '--prueba' in sys.argv
    objetos = {}
    en_s = {v: (n, u) for n, v, t, js, u in fs}
    if prueba:
        import prueba_nombre as pn

        def funciones(ruta):
            if ruta not in objetos:
                objetos[ruta] = pn.funciones(ruta) if os.path.exists(ruta) else {}
            return objetos[ruta]
    for ds, na, cands, mx, t in sorted(discrepan):
        print('  0x%06x  SLUS %-40s A124 %-40s x%d (%s)' % (ds, ','.join(na)[:40], ' | '.join(cands)[:40], mx,
                                                            ', '.join(x[:30] for x in t)))
        if not prueba:
            continue
        # nuestro codigo con el nombre actual y con el propuesto, contra el original troceado en esa direccion
        if ds not in en_s:
            continue
        glabel, unidad = en_s[ds]
        base = os.path.splitext(unidad)[0] if unidad.endswith(('.cpp', '.c')) else unidad
        tro = funciones(os.path.join(RAIZ, 'build', S, 'obj', base + '.o')).get(glabel)
        nuestras = funciones(os.path.join(RAIZ, 'build', S, 'src', base + '.o'))
        for n in na + cands:
            res = [pn.compara(tro[0], o)[0] for o in nuestras.get(n, [])] if tro else []
            print('        %-6s %-70s %s' % ('actual' if n in na else 'A124', n[:70], ', '.join(res) or 'no esta'))
    print('\n== conflictos: %d' % len(conflictos))
    for ds, top, na in sorted(conflictos)[:60]:
        print('  0x%06x  SLUS %-30s %s' % (ds, ','.join(na)[:30], top[:4]))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for ds, cands, mx, t in sorted(nuevos):
                if len(cands) == 1:
                    fh.write('%s\t%d\t0x%x\n' % (cands[0], mx, ds))


if __name__ == '__main__':
    main()
