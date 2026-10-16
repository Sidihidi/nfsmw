#!/usr/bin/env python3
"""datos_ref.py -- fronteras de .data/.bss de SLUS por REFERENCIAS de codigo, emparejadas con la A124.

Cada funcion con el mismo nombre y el mismo numero de instrucciones en la A124 y en SLUS referencia sus datos (%hi,
%lo, %gp_rel) en las mismas instrucciones; eso empareja direcciones de datos de la A124 (con nombre, de la .symtab)
con las de SLUS. Para cada subsegmento de datos de la A124 de las unidades pedidas, su comienzo en SLUS = direccion de
SLUS de una variable referenciada - su desplazamiento dentro del subsegmento en la A124; todas las variables de la
unidad tienen que votar el mismo comienzo.

    python scripts/datos_ref.py [SUBCADENA_DE_UNIDAD] [--tipos .rodata,.data,.bss] [--tsv votos.tsv]

    SUBCADENA  por defecto /snd/ (en Git Bash, una que empiece por / hay que pasarla con MSYS_NO_PATHCONV=1: si no,
               Bash la convierte en una ruta de Windows y no casa con nada)
    --tipos    secciones a mirar (por defecto .data,.bss; .rodata tambien vale: vram = start + 0xff000)
    --tsv      vuelca una fila por subsegmento con votos: tipo, unidad, vram y tamano en la A124, comienzo mas votado
               en SLUS, sus votos y los del segundo, y el comienzo actual

Imprime, por tipo (.data y .bss) y por unidad de la A124 que contiene la subcadena: su vram y tamano en la A124, el
comienzo actual en config/SLUS-21351/config.yml (o `-` si SLUS no tiene esa frontera) y los tres comienzos mas votados
en SLUS. En .data la vram es start + 0xff000 (el segmento principal de las dos PS2); en .bss se usa la vram del
subsegmento. Solo informa: las fronteras se escriben a mano en config.yml, se trocea y se mide SLUS.

Tanda 5 (2026-09-23): las fronteras de .data/.bss de las 53 unidades de snd fundidas en SLUS (12 .data nuevas, 9 .bss
nuevas y 5 corregidas). Lee los troceados y config.json de las dos versiones de la raiz donde este el guion.
"""
import collections
import json
import os
import re
import sys

import yaml

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
A, S = 'SLES-53558-A124', 'SLUS-21351'
RE_INS = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s+[0-9A-F]{8}\s*\*/\s+(\S+)\s*(.*)$')
RE_REF = re.compile(r'%(hi|lo|gp_rel)\(([^)+]+?)(?:\s*\+\s*(0x[0-9A-Fa-f]+|\d+))?\)')


def simbolos(raiz, V):
    d = {}
    for l in open(os.path.join(raiz, 'config', V, 'symbols.txt'), encoding='utf-8'):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);', l)
        if m:
            d.setdefault(m.group(1), int(m.group(2), 16))
    return d


def funciones(raiz, V, filtro):
    """{nombre: [(op, [(tipo, direccion)])]} de las funciones de las unidades que contienen filtro"""
    cfg = json.load(open(os.path.join(raiz, 'build', V, 'config.json'), encoding='utf-8'))
    sym = simbolos(raiz, V)
    out = {}
    for u in cfg['units']:
        if filtro not in u['name']:
            continue
        p = os.path.join(raiz, 'build', V, 'asm', u['name'] + '.s')
        if not os.path.exists(p):
            continue
        cur = None
        for l in open(p, encoding='utf-8', errors='replace'):
            g = re.match(r'\s*glabel (\S+)', l)
            if g:
                cur = []
                out.setdefault(g.group(1), []).append(cur)
                continue
            m = RE_INS.match(l)
            if m and cur is not None:
                refs = []
                for tipo, s, add in RE_REF.findall(m.group(3)):
                    mm = re.match(r'D_([0-9A-F]{8})$', s)
                    a = int(mm.group(1), 16) if mm else sym.get(s)
                    if a is not None:
                        refs.append((tipo, a + (int(add, 0) if add else 0)))
                cur.append((m.group(2), refs))
            elif re.match(r'\s*endlabel', l):
                cur = None
    return out


def subsegs(V):
    cfg = yaml.safe_load(open(os.path.join(RAIZ, 'config', V, 'config.yml'), encoding='utf-8'))
    out = []
    for seg in cfg['segments']:
        if not isinstance(seg, dict):
            continue
        for s in seg.get('subsegments', []) or []:
            if isinstance(s, list):
                out.append({'start': s[0], 'type': s[1], 'name': s[2] if len(s) > 2 else None})
            elif isinstance(s, dict):
                out.append(dict(s))
    return out


def main():
    args = [a for i, a in enumerate(sys.argv[1:], 1) if not a.startswith('--') and sys.argv[i - 1] not in (
        '--tipos', '--tsv')]
    filtro = args[0] if args else '/snd/'
    tipos = sys.argv[sys.argv.index('--tipos') + 1].split(',') if '--tipos' in sys.argv else ['.data', '.bss']
    tsv = open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') \
        if '--tsv' in sys.argv else None
    fa, fs = funciones(RAIZ, A, filtro), funciones(RAIZ, S, filtro)
    pares = collections.defaultdict(collections.Counter)      # dir A124 -> {dir SLUS: veces}
    for n, la in fa.items():
        if n not in fs or len(la) != 1 or len(fs[n]) != 1:
            continue
        a, s = la[0], fs[n][0]
        if len(a) != len(s):
            continue
        for (opa, ra), (ops, rs) in zip(a, s):
            if opa != ops or len(ra) != len(rs):
                continue
            for (ta, xa), (ts, xs) in zip(ra, rs):
                if ta == ts:
                    pares[xa][xs] += 1
    # subsegmentos de datos de la A124 de las unidades del filtro, con su vram
    for tipo in tipos:
        sa = [s for s in subsegs(A) if s.get('type') == tipo and not s.get('linker_section_order')]
        ss = [s for s in subsegs(S) if s.get('type') == tipo and not s.get('linker_section_order')]
        print('=== %s' % tipo)
        for i, s in enumerate(sa):
            if not s['name'] or filtro not in s['name']:
                continue
            ini = s.get('vram', s['start'] + 0xff000) if tipo == '.bss' else s['start'] + 0xff000
            fin_rom = sa[i + 1]['start'] if i + 1 < len(sa) else None
            fin = ini + (fin_rom - s['start']) if fin_rom else None
            votos = collections.Counter()
            for xa, c in pares.items():
                if ini <= xa < (fin or ini):
                    for xs, k in c.items():
                        votos[xs - (xa - ini)] += k
            actual = [x for x in ss if x['name'] == s['name']]
            act = ('0x%x' % (actual[0].get('vram', actual[0]['start'] + 0xff000) if tipo == '.bss'
                             else actual[0]['start'] + 0xff000)) if actual else '-'
            print('%-45s A124 vram 0x%x tam 0x%-5x SLUS actual %-9s votos %s' % (
                s['name'].split('library/')[-1], ini, (fin - ini) if fin else 0, act,
                ', '.join('0x%x:%d' % v for v in votos.most_common(3))))
            if tsv and votos:
                (mejor, k1), = votos.most_common(1)
                k2 = votos.most_common(2)[1][1] if len(votos) > 1 else 0
                tsv.write('%s\t%s\t0x%x\t0x%x\t0x%x\t%d\t%d\t%s\n' % (
                    tipo, s['name'], ini, (fin - ini) if fin else 0, mejor, k1, k2, act))


if __name__ == '__main__':
    main()
