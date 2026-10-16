#!/usr/bin/env python3
"""dtors_por_vtable.py -- nombres de SLUS para los destructores que la vtable entera ancla, tambien los que no guardan
su propia vtable.

scripts/vtable_dtors.py reconoce un destructor porque carga la vtable cuyo primer puntero es el mismo. Los destructores
implicitos de EE-GCC no la cargan: si la clase no declara `~X() {}` el destructor sintetizado es `j ~Base; nop` (8 B),
y esos quedaban sin nombre, casi siempre metidos en un func_ que empieza por restos de epilogo (scripts/restos_*.py).

Aqui la prueba es la vtable. La huella de cada clase sale de la A124 (`_vt$Clase = dir; // size:N` de su symbols.txt,
nombres de la .symtab): la lista ordenada de los metodos de su vtable principal. En SLUS se barren los datos (desde el
final de .text) buscando la forma de vtable de EE-GCC: 12 bytes a cero y despues pares (puntero a codigo, 0). Una
vtable de SLUS es de la clase X si tiene tantas ranuras como la de X y, en cada ranura donde SLUS ya tiene nombre, el
mismo nombre, con al menos dos nombres o uno que sea un metodo de la propia X (React__7OMAudio... para OMAudio). Si
encaja con una sola clase, su primer puntero es el destructor de X.

Se proponen (--tsv) solo los nombres que no estan ya en otra direccion de symbols.txt y que no salen dos veces: una
derivada que no redefine nada tiene la huella de su base (MenuScreen, ArrayDatum...) y esas se listan pero no se
proponen.

    python scripts/dtors_por_vtable.py [--tsv propuestas.tsv]

Las propuestas se aplican con scripts/aplica_nombres.py; despues hay que trocear y medir SLUS (scripts/medir.py) y pasar
scripts/cortes.py, scripts/restos_epilogo.py y scripts/restos_trozos.py. Tanda 7 (2026-09-24): 55 propuestas, casi todas
destructores implicitos de las opciones del frontend (OMAudio, VOTimeOfDay, pm_*, PM*, C*...).
"""
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
from vtable_dtors import Imagen  # noqa: E402

SIN = re.compile(r'^(func|fn|sub|lbl|D)_[0-9A-Fa-f]+$')
# SLUS: .text de 0x100000 a 0x4be880 (detras van el microcodigo de VU y los datos) y .over (zOnline y zFeOverlay) de
# 0x510380 a 0x536718, como en config/SLUS-21351/config.yml.
TEXTO = ((0x100000, 0x4be880), (0x510380, 0x536718))
DATOS = 0x4be880


def es_texto(p):
    return any(a <= p < b for a, b in TEXTO)


def simbolos(V):
    por_dir, por_nom, tam = {}, {}, {}
    for l in open(os.path.join(RAIZ, 'config', V, 'symbols.txt'), encoding='utf-8', errors='replace'):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);(.*)', l)
        if m:
            a = int(m.group(2), 16)
            por_dir.setdefault(a, m.group(1))
            por_nom.setdefault(m.group(1), a)
            k = re.search(r'size:0x([0-9a-fA-F]+)', m.group(3))
            if k:
                tam[m.group(1)] = int(k.group(1), 16)
    return por_dir, por_nom, tam


def glabels(V):
    """{direccion: nombre} de los glabel de los .s troceados (tambien los func_)"""
    out = {}
    ins = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s')
    base = os.path.join(RAIZ, 'build', V, 'asm')
    for dp, dn, fn in os.walk(base):
        for f in fn:
            if not f.endswith('.s'):
                continue
            cur = None
            for l in open(os.path.join(dp, f), encoding='utf-8', errors='replace'):
                m = re.match(r'\s*glabel (\S+)', l)
                if m:
                    cur = m.group(1)
                    continue
                if cur:
                    m = ins.match(l)
                    if m:
                        out[int(m.group(1), 16)] = cur
                        cur = None
    return out


def huellas():
    """{(metodo0, metodo1, ...): {clase}} de las vtables principales de la A124"""
    ia = Imagen(os.path.join(RAIZ, 'orig', 'SLES-53558-A124', 'NFS.ELF'))
    a_dir, a_nom, a_tam = simbolos('SLES-53558-A124')
    h = collections.defaultdict(set)
    for n, sz in a_tam.items():
        if not n.startswith('_vt$') or '$' in n[4:]:
            continue   # solo la vtable principal (no las de las bases secundarias, _vt$A$B)
        try:
            ws = struct.unpack('<%dI' % (sz // 4), ia.lee(a_nom[n], sz))
        except (ValueError, struct.error):
            continue
        if ws[0] or ws[1] or ws[2]:
            continue
        h[tuple(a_dir.get(ws[k], 'func_%08X' % ws[k]) for k in range(3, len(ws), 2))].add(n[4:])
    return h


def main():
    huella = huellas()
    img = Imagen(os.path.join(RAIZ, 'orig', 'SLUS-21351', 'SLUS_213.51'))
    s_dir, s_nom, _ = simbolos('SLUS-21351')
    nombre_en = glabels('SLUS-21351')
    nombre_en.update(s_dir)
    va, off, tam = img.segs[0]
    buf = img.lee(DATOS, va + tam - DATOS)
    n = len(buf) // 4
    ws = struct.unpack('<%dI' % n, buf[:n * 4])
    props = []
    i = 0
    while i < n - 5:
        if ws[i] == 0 and ws[i + 1] == 0 and ws[i + 2] == 0 and es_texto(ws[i + 3]) and ws[i + 4] == 0:
            met = []
            k = i + 3
            while k < n and es_texto(ws[k]) and ws[k - 1] == 0:
                met.append(ws[k])
                k += 2
            nombres = tuple(nombre_en.get(p, 'func_%08X' % p) for p in met)
            cand = set()
            for ma in huella:
                if len(ma) != len(nombres) or not ma[0].startswith('_$_'):
                    continue
                con = [(s, a) for s, a in zip(nombres[1:], ma[1:]) if not SIN.match(s)]
                propio = '__' + ma[0][3:]   # _$_7OMAudio -> __7OMAudio: un metodo de la propia clase
                if all(s == a for s, a in con) and (len(con) >= 2 or any(propio in s for s, a in con)):
                    cand.add(ma[0])
            if len(cand) == 1:
                props.append((met[0], nombre_en.get(met[0], '-'), cand.pop(), DATOS + 4 * i, len(met)))
            i = k - 1   # la vtable siguiente puede empezar justo detras del ultimo puntero
        else:
            i += 1
    malos = [x for x in props if x[1] != x[2]]
    usados = collections.Counter(x[2] for x in malos)
    for p, a, nn, v, m in sorted(malos):
        otra = s_nom.get(nn)
        print('0x%06x %-34s -> %-34s vt 0x%06x %2d ranuras%s%s' % (
            p, a[:34], nn[:34], v, m, ('  (ya en 0x%x)' % otra) if otra is not None else '',
            '  (REPETIDO)' if usados[nn] > 1 else ''))
    print('%d vtables casadas, %d con el destructor mal o sin nombre' % (len(props), len(malos)))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for p, a, nn, v, m in sorted(malos):
                if usados[nn] == 1 and s_nom.get(nn) is None:
                    fh.write('0x%x\t%s\tvtable\t-\n' % (p, nn))


if __name__ == '__main__':
    main()
