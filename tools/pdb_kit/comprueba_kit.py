"""comprueba_kit.py CARPETA EXE -- privacidad y ausencia de bytes del juego en todos los ficheros de CARPETA.

1. Privacidad: busca (sin mayusculas, en ASCII y en UTF-16LE) el usuario de Windows, users\\, users/, la carpeta
   del usuario, la raiz del clon y lo que se pase en NFSMW_PRIVADO (lista separada por comas: email, nombre...).
2. Datos del juego: trocea .text, .rdata y .data del exe en bloques de 16 B alineados (con al menos 6 valores de
   byte distintos) y busca cada bloque en cualquier desplazamiento de cada fichero (prefiltro de 8 B con numpy).
"""
import collections
import os
import struct
import sys
from pathlib import Path

import numpy as np

carpeta, exe = Path(sys.argv[1]), Path(sys.argv[2])
home = str(Path.home())
raiz = str(Path(__file__).resolve().parents[2])
patrones = [os.environ.get('USERNAME', ''), 'users\\', 'users/', home, home.replace('\\', '/'), raiz,
            raiz.replace('\\', '/'), 'appdata']
patrones += [x for x in os.environ.get('NFSMW_PRIVADO', '').split(',') if x]
patrones = [x.lower() for x in patrones if x]

d = exe.read_bytes()
pe, = struct.unpack_from('<I', d, 0x3c)
_, nsec, _, _, _, optsz, _ = struct.unpack_from('<HHIIIHH', d, pe + 4)
bloques = {}
for i in range(nsec):
    o = pe + 24 + optsz + 40 * i
    nombre = d[o:o + 8].rstrip(b'\0').decode('latin-1')
    vs, va, rsz, rptr = struct.unpack_from('<IIII', d, o + 8)
    if nombre not in ('.text', '.rdata', '.data'):
        continue
    s = d[rptr:rptr + rsz]
    for k in range(0, len(s) - 15, 16):
        c = s[k:k + 16]
        if len(set(c)) >= 6:
            bloques.setdefault(c, (nombre, 0x400000 + va + k))
print('bloques de 16 B no triviales del exe: %d (%s)' % (len(bloques), dict(collections.Counter(v[0] for v in bloques.values()))))
claves = np.unique(np.frombuffer(b''.join(c[:8] for c in bloques), dtype='<u8'))

total_priv = 0
total_juego = collections.Counter()
for f in sorted(p for p in carpeta.rglob('*') if p.is_file()):
    datos = f.read_bytes()
    baja = datos.lower()
    priv = []
    for pat in patrones:
        for enc in ('latin-1', 'utf-16-le'):
            b = pat.lower().encode(enc)
            n = baja.count(b)
            if n:
                priv.append('%s[%s]=%d' % (pat, enc[:5], n))
    total_priv += len(priv)
    hits = collections.Counter()
    ejemplos = []
    if len(datos) >= 16:
        for r in range(8):
            m = (len(datos) - r) // 8
            vals = np.frombuffer(datos[r:r + 8 * m], dtype='<u8')
            cand = np.nonzero(np.isin(vals, claves))[0]
            for i in cand:
                p = r + 8 * int(i)
                c = datos[p:p + 16]
                if c in bloques:
                    sec, va = bloques[c]
                    texto = len(c.rstrip(bytes(1))) >= 8 and all(32 <= x < 127 for x in c.rstrip(bytes(1)))
                    w = struct.unpack('<4I', c)
                    serie = all(w[j + 1] - w[j] == w[1] - w[0] for j in range(3)) and 0 < w[1] - w[0] <= 16
                    clase = 'texto' if texto else ('serie' if serie else 'BINARIO')
                    texto = texto or serie
                    hits[sec + ' (' + clase + ')'] += 1
                    if not texto or len(ejemplos) < 2:
                        ejemplos.append('%s@0x%X en +0x%X: %r' % (sec, va, p, c))
    total_juego.update(hits)
    print('%-45s %10d B  privacidad: %s  bloques del exe: %s' % (
        f.relative_to(carpeta), len(datos), ', '.join(priv) or 'limpio', dict(hits) or 'ninguno'))
    for e in ejemplos:
        print('    ', e)
print('TOTAL privacidad: %d patrones encontrados; bloques del exe: %s' % (total_priv, dict(total_juego) or 'ninguno'))
