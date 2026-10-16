#!/usr/bin/env python3
"""patrones.py [--version V] [--min P] [unidad ...] -- agrega, sobre todas las funciones casi exactas de una
version de PS2 (por defecto la A124), los pares (opcode del original -> el nuestro) de las filas que objdiff marca
como distintas (DIFF_OP_MISMATCH y DIFF_REPLACE), y cuantas funciones comparten cada par.

Un par que se repite en cientos de funciones es una causa raiz, no una suma de near-miss:
  - `sd -> sq` / `ld -> lq` (696 y 635 funciones): la semilla no volatile de bIdentity(bMatrix4 *) dejaba un valor
    de 128 bits en un registro preservado y EE-GCC guardaba con sq el resto de la unidad (+523 SLES, +510 SLUS);
  - `c.lt.s -> c.le.s`, `beqz -> bnez`: formas de fuente que GameCube no distingue (scripts/gcmismo.py);
  - `lqc2 -> lwc1`, `pmaxw -> slt`, `mfc0 -> jal`: el original usa VU0, MMI o COP0 donde nosotros C.
Para encontrar la causa, mira en que funcion de la unidad EMPIEZA el par (orden del .s) y que tiene esa.

Las filas REPLACE son en parte ruido de alineacion: fiate de los pares con muchas funciones, no de los de pocas.
Deja el detalle por funcion en build/patrones/patrones_<V>.json.
"""
import collections
import glob
import json
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__))).replace('\\', '/') + '/'
SCR = os.path.join(ROOT, 'build', 'patrones')


def mnem(row):
    ins = row.get('instruction') or {}
    for p in ins.get('parts', []):
        if 'opcode' in p:
            return p['opcode'].get('mnemonic')
    return None


def uno(args):
    u, T, O, minp = args
    ours = O + u + '.o'
    if not os.path.exists(ours) or not os.path.exists(T + u + '.o'):
        return u, []
    if u in ('zFeOverlay', 'zOnline'):
        tmp = os.path.join(SCR, 'over_%s.o' % u)
        subprocess.run([ROOT + 'build/mips_binutils/mips-linux-gnu-objcopy.exe', '--rename-section', '.text=.over',
                        ours, tmp], check=True)
        ours = tmp
    out = os.path.join(SCR, 'diff_%s.json' % u)
    r = subprocess.run([ROOT + 'objdiff-cli-windows-x86_64.exe', 'diff', '-1', T + u + '.o', '-2', ours,
                        '-c', 'functionRelocDiffs=none', '-o', out, '--format', 'json'], capture_output=True, text=True)
    if r.returncode:
        return u, []
    d = json.load(open(out))
    os.remove(out)
    L, R = d['left']['symbols'], d['right']['symbols']
    res = []
    for s in L:
        p = s.get('match_percent')
        if p is None or p >= 100 or p < minp or not s.get('instructions'):
            continue
        ti = s.get('target_symbol')
        if not isinstance(ti, int) or ti >= len(R):
            continue
        li, ri = s['instructions'], R[ti].get('instructions') or []
        pares = collections.Counter()
        otros = 0
        for i, row in enumerate(li):
            k = row.get('diff_kind')
            if not k:
                continue
            if k in ('DIFF_OP_MISMATCH', 'DIFF_REPLACE') and i < len(ri):
                a, b = mnem(row), mnem(ri[i])
                if a and b and a != b:
                    pares[(a, b)] += 1
            elif k != 'DIFF_ARG_MISMATCH':
                otros += 1
        res.append((u, s['name'], int(s.get('size') or 0), p, pares, otros))
    return u, res


def main():
    args = sys.argv[1:]
    version, minp = 'SLES-53558-A124', 50.0
    while args and args[0].startswith('--'):
        if args[0] == '--version':
            version, args = args[1], args[2:]
        elif args[0] == '--min':
            minp, args = float(args[1]), args[2:]
        else:
            sys.exit(__doc__)
    os.makedirs(SCR, exist_ok=True)
    T = ROOT + 'build/%s/obj/Speed/Indep/SourceLists/' % version
    O = ROOT + 'build/%s/src/Speed/Indep/SourceLists/' % version
    unidades = args or sorted(os.path.basename(p)[:-2] for p in glob.glob(T + '*.o'))
    todas = []
    with ThreadPoolExecutor(4) as ex:
        for u, res in ex.map(uno, [(u, T, O, minp) for u in unidades]):
            todas.extend(res)
    por_par = collections.defaultdict(lambda: [0, 0, 0])
    solo = collections.Counter()
    for u, n, size, p, pares, otros in todas:
        for par, c in pares.items():
            e = por_par[par]
            e[0] += 1
            e[1] += c
            e[2] += size
        if len(pares) == 1 and otros == 0:
            solo[next(iter(pares))] += 1
    print('%d funciones entre %.0f y 100 %%' % (len(todas), minp))
    print('%-34s %9s %7s %10s %6s' % ('original -> nuestro', 'funciones', 'filas', 'bytes fn', 'solo'))
    for par, (f, c, b) in sorted(por_par.items(), key=lambda kv: -kv[1][0])[:45]:
        print('%-34s %9d %7d %10d %6d' % ('%s -> %s' % par, f, c, b, solo[par]))
    json.dump([(u, n, size, p, [[list(k), v] for k, v in pares.items()], otros) for u, n, size, p, pares, otros in todas],
              open(os.path.join(SCR, 'patrones_%s.json' % version), 'w'))


if __name__ == '__main__':
    main()
