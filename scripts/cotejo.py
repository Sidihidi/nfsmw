#!/usr/bin/env python3
"""cotejo.py <Clase> [fichero] -- pone lado a lado los campos de una clase en la A124 (stabs del .mdebug, via la cache
de sondastabs.py) y los miembros de nuestra declaracion, para decir QUE miembro sobra o falta.

sondastabs.py dice que una clase esta descolocada y cual es el primer campo que no cuadra; esto dice por que. Casi
siempre es un miembro que la A124 todavia no tenia (AIPursuit::mAllowStatsToAccumulate, PVehicle::mCacheName,
CubicCameraMover::fIgnoreSetSnapNextTimer...) o uno que tenia y luego se quito. Los huecos (relleno delante de un
campo) marcan alineaciones.

Sin fichero, busca el que define la clase. Compara por NOMBRE y en orden (difflib): un miembro renombrado sale como
una pareja borrado/insertado.

    python scripts/cotejo.py CubicCameraMover
    python scripts/cotejo.py SoundConn::Pkt_Car_Service src/Speed/Indep/Src/EAXSound/SoundConn.h"""
import difflib
import os
import pickle
import re
import subprocess
import sys

if len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help'):
    print(__doc__)
    sys.exit(0)
q = sys.argv[1]
cache = os.path.join('build', 'sondastabs', 'stabs.pkl')
if not os.path.exists(cache):
    sys.exit('falta %s: corre antes scripts/sondastabs.py' % cache)
clases = pickle.load(open(cache, 'rb'))
if q not in clases:
    parecidas = [k for k in clases if k.split('::')[-1] == q.split('::')[-1]]
    sys.exit('%s no esta en los stabs. Con ese nombre corto: %s' % (q, parecidas[:10]))
for (tam, campos), n in clases[q].items():
    print('%s en la A124: tamano 0x%x (en %d unidades)' % (q, tam, n))
    fin = None
    for nom, off, bits in campos:
        hueco = (off - fin) // 8 if fin is not None and off > fin and (off - fin) >= 8 else 0
        print('   0x%04x %5d bits  %-32s%s' % (off // 8, bits, nom, ('  <- %d B de relleno delante' % hueco) if hueco else ''))
        fin = off + bits
if len(clases[q]) > 1:
    sys.exit('dos layouts distintos con ese nombre: no se coteja')
(tam, campos), = clases[q].keys()
orig = [n for n, o, b in campos if n and not n.startswith('$')]

corto = q.split('::')[-1]
fich = sys.argv[2] if len(sys.argv) > 2 else None
if not fich:
    r = subprocess.run(['git', 'grep', '-lE', r'(class|struct)\s+(\w+\s+)?%s\b[^;]*\{' % re.escape(corto), '--', 'src'],
                       capture_output=True, text=True)
    cands = [l for l in r.stdout.split('\n') if l]
    if not cands:
        sys.exit('no encuentro donde se define %s' % corto)
    fich = cands[0]
t = open(fich, encoding='utf-8', errors='replace').read()
m = re.search(r'\b(?:class|struct)\s+(?:\w+\s+)?' + re.escape(corto) + r'\b[^;{]*\{', t)
if not m:
    sys.exit('no encuentro %s en %s' % (corto, fich))
i, prof = m.end(), 1
while prof and i < len(t):
    prof += {'{': 1, '}': -1}.get(t[i], 0)
    i += 1
plano, prof = [], 0
for ch in t[m.end():i - 1]:
    if ch == '{':
        prof += 1
    elif ch == '}':
        prof -= 1
        if prof == 0:
            plano.append(';')      # un cuerpo inline cierra su declaracion
    elif prof == 0:
        plano.append(ch)
nuestros = []
for decl in re.sub(r'//[^\n]*', '', ''.join(plano)).split(';'):
    decl = re.sub(r'^\s*#[^\n]*\n', '', decl, flags=re.M)
    decl = re.sub(r'^\s*(public|private|protected)\s*:', '', decl.strip()).strip()
    if not decl or '(' in decl or re.match(r'(typedef|friend|enum|using|static|template|class|struct)\b', decl):
        continue
    mm = re.search(r'(\w+)\s*(\[[^\]]*\])*\s*(:\s*\d+)?$', decl)
    if mm:
        nuestros.append(mm.group(1))
print('\nnuestra declaracion: %s' % fich)
sm = difflib.SequenceMatcher(a=orig, b=nuestros, autojunk=False)
iguales = True
for op, a0, a1, b0, b1 in sm.get_opcodes():
    if op == 'equal':
        continue
    iguales = False
    print('  %-8s A124 %s  |  nuestro %s' % (op, orig[a0:a1], nuestros[b0:b1]))
if iguales:
    print('  mismos miembros y en el mismo orden (si no cuadra, es tamano o alineacion)')
print('(%d campos en la A124, %d en el nuestro)' % (len(orig), len(nuestros)))
