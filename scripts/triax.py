#!/usr/bin/env python3
"""Triaje X360: clasifica near-miss por TIPO de diff (CODIGO vs LAYOUT)."""
import json, os, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
V = 'EUROPEGERMILESTONE'
SCR = os.environ.get('SCRATCH', 'C:/Users/jferr/scratch-r21-x360')
os.makedirs(SCR, exist_ok=True)
CLI = ROOT + '/build/tools/objdiff-cli.exe'

rep = json.load(open(ROOT + '/build/versiones/EUROPEGERMILESTONE.json'))
# unidad corta -> rel
def rel_of(unit_short):
    rel = 'Speed/Indep/SourceLists/' + unit_short
    if not os.path.exists(f'{ROOT}/build/{V}/obj/{rel}.obj') and os.path.exists(f'{ROOT}/build/{V}/obj/{unit_short}.obj'):
        rel = unit_short
    return rel

quiero = {}
for u in rep.get('units', []):
    short = u['name'].split('/')[-1]
    for f in u.get('functions', []) or []:
        pct = f.get('fuzzy_match_percent', 0.0); tam = int(f.get('size', 0) or 0)
        if 80.0 <= pct < 99.999 and tam >= 100:
            quiero.setdefault(short, {})[f['name']] = (pct, tam)

def texto(e):
    return ((e or {}).get('instruction') or {}).get('formatted', '').strip()

import re
def es_offset(a, b):
    """Mismo mnemonico, solo cambia un inmediato numerico (layout)."""
    ta, tb = texto(a), texto(b)
    if not ta or not tb:
        return False
    ma = ta.split(None, 1); mb = tb.split(None, 1)
    if not ma or not mb or ma[0] != mb[0]:
        return False
    na = re.sub(r'0x[0-9a-fA-F]+|-?\d+', '#', ma[1] if len(ma) > 1 else '')
    nb = re.sub(r'0x[0-9a-fA-F]+|-?\d+', '#', mb[1] if len(mb) > 1 else '')
    return na == nb

filtro = sys.argv[1] if len(sys.argv) > 1 else None
filas = []
for short, fns in sorted(quiero.items()):
    if filtro and filtro.lower() not in short.lower():
        continue
    rel = rel_of(short)
    out = f'{SCR}/t_{short}.json'
    o1, o2 = f'{ROOT}/build/{V}/obj/{rel}.obj', f'{ROOT}/build/{V}/src/{rel}.obj'
    if not (os.path.exists(o1) and os.path.exists(o2)):
        continue
    if not os.path.exists(out):
        r = subprocess.run([CLI, 'diff', '-1', o1, '-2', o2, '-c', 'function_reloc_diffs=none',
                            '-c', 'ppc.calculatePoolRelocations=false', '-o', out, '--format', 'json'],
                           capture_output=True, text=True)
        if r.returncode != 0:
            print('FALLO', short, r.stderr[-300:]); continue
    d = json.load(open(out))
    L = {s['name']: s for s in d['left']['symbols']}
    R = {s['name']: s for s in d['right']['symbols']}
    for name, (pct, tam) in fns.items():
        a, b = L.get(name), R.get(name)
        if a is None or b is None:
            continue
        LI, RI = a.get('instructions', []), b.get('instructions', [])
        n = max(len(LI), len(RI))
        cod, lay, det = 0, 0, []
        for k in range(n):
            x = LI[k] if k < len(LI) else None
            y = RI[k] if k < len(RI) else None
            kx = (x or {}).get('diff_kind') or ''; ky = (y or {}).get('diff_kind') or ''
            if kx in ('', 'DIFF_NONE') and ky in ('', 'DIFF_NONE'):
                continue
            if es_offset(x, y):
                lay += 1
            else:
                cod += 1
                if len(det) < 4:
                    det.append((k, texto(x), texto(y)))
        filas.append((cod, lay, pct, tam, name, short, det))

filas.sort(key=lambda r: (r[0], -r[3]))
print('COD LAY   PCT%   TAM  unidad    funcion')
for cod, lay, pct, tam, name, short, det in filas[:80]:
    print(f'{cod:3d} {lay:3d} {pct:6.2f} {tam:6d} {short:12s} {name}')
    for k, ta, tb in det:
        print(f'      [{k}] {ta}   ||   {tb}')
