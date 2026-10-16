#!/usr/bin/env python3
"""tipos_cadena.py -- nombres de SLUS anclados por la CADENA del TypeInfo/StateInfo de las clases de sonido (SndBase y
CSTATE_Base).

GetTypeInfo() devuelve &s_TypeInfo (`lui v0,%hi(T); jr ra; addiu v0,v0,%lo(T)`) y GetTypeName() lee su segundo campo
(`lui v1,%hi(T+4); jr ra; lw v0,%lo(T+4)(v1)`). El TypeInfo es {ObjectID, typeName, baseTypeInfo, createObject}: la
cadena typeName es el nombre de la clase y createObject apunta a su CreateObject (o CreateState para CSTATE_Base).
Con eso cada GetTypeInfo/GetTypeName/CreateObject de SLUS tiene su clase sin fiarse de ningun nombre heredado: las
clases hermanas tienen esas funciones iguales salvo la reubicacion, y los nombres puestos por direccion se cruzaban.

    python scripts/tipos_cadena.py [--tsv propuestas.tsv]

Solo propone nombres que NUESTRO objeto de la unidad define. Imprime las funciones cuyo nombre no cuadra con la cadena
(direccion, nombre propuesto, nombre actual, resultado de scripts/prueba_nombre.py y el motivo) y los nombres que
saldrian repetidos. --tsv vuelca `0xDIR<TAB>nombre<TAB>cadena<TAB>resultado<TAB>motivo`, que acepta
scripts/aplica_nombres.py; despues, trocear y medir SLUS con scripts/medir.py.

Lee el troceado de SLUS (build/SLUS-21351/asm, obj y src) de la raiz donde este el guion. Tanda 5 (2026-09-23): parte
de los 206 nombres anclados de SLUS salieron de aqui.
"""
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
from vtable_dtors import Imagen, ELFS  # noqa: E402
import prueba_lote  # noqa: E402
import prueba_nombre as pn  # noqa: E402

V = 'SLUS-21351'
RE_INS = re.compile(r'^\s*/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s+[0-9A-F]{8}\s*\*/\s+(\S+)\s*(.*)$')


def main():
    img = Imagen(os.path.join(RAIZ, ELFS[V]))
    por_nom, por_dir = {}, collections.defaultdict(list)
    for l in open(os.path.join(RAIZ, 'config', V, 'symbols.txt'), encoding='utf-8'):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);', l)
        if m:
            por_nom.setdefault(m.group(1), int(m.group(2), 16))
            por_dir[int(m.group(2), 16)].append(m.group(1))

    def dir_de(s):
        m = re.match(r'D_([0-9A-F]{8})$', s)
        return int(m.group(1), 16) if m else por_nom.get(s)
    funcs = []                    # (ini, glabel, unidad, [(op, args)])
    for p, unidad in pn.ficheros_asm(V):
        cur = None
        for l in open(p, encoding='utf-8', errors='replace'):
            g = re.match(r'\s*glabel (\S+)', l)
            if g:
                cur = [None, g.group(1), unidad, []]
                funcs.append(cur)
                continue
            m = RE_INS.match(l)
            if m and cur is not None:
                if cur[0] is None:
                    cur[0] = int(m.group(1), 16)
                cur[3].append((m.group(2), m.group(3).replace(' ', '')))
            elif re.match(r'\s*endlabel', l):
                cur = None
    pr = prueba_lote.Probador(V)
    props = {}
    for ini, g, unidad, ins in funcs:
        if ini is None or len(ins) != 3:
            continue
        (o1, a1), (o2, a2), (o3, a3) = ins
        if o2 != 'jr' or a2 != '$ra' or o1 != 'lui':
            continue
        m1 = re.match(r'\$(v0|v1),%hi\(([^)]+)\)$', a1)
        if not m1:
            continue
        base = dir_de(m1.group(2))
        if base is None:
            continue
        s = m1.group(2)
        if o3 == 'addiu' and a3 == '$v0,$v0,%lo(' + s + ')' and m1.group(1) == 'v0':
            clase_ti, t = 'info', base
        elif o3 == 'lw' and a3 == '$v0,%lo(' + s + ')($v1)' and m1.group(1) == 'v1':
            clase_ti, t = 'name', base - 4
        else:
            continue
        try:
            oid, pnombre, pbase, pcrea = struct.unpack('<4I', img.lee(t, 16))
            nombre = img.lee(pnombre, 64).split(b'\0')[0].decode('latin-1')
        except (ValueError, struct.error, UnicodeDecodeError):
            continue
        if not re.match(r'^[A-Za-z_][A-Za-z0-9_]*$', nombre):
            continue
        c = '%d%s' % (len(nombre), nombre)
        t_, o = pr.objs(unidad)
        for tipo, nombres in (('info', ('GetTypeInfo__C' + c, 'GetStateInfo__C' + c)),
                              ('name', ('GetTypeName__C' + c, 'GetStateName__C' + c))):
            if tipo != clase_ti:
                continue
            for n in nombres:
                if n in o:
                    props[ini] = (n, g, unidad, 'cadena "%s"' % nombre)
        if clase_ti == 'info' and pcrea:
            for n in ('CreateObject__%sUi' % c, 'CreateState__%sUi' % c):
                if n in o and pcrea in pr.glabel:
                    props[pcrea] = (n, pr.glabel[pcrea][0], unidad, 'createObject del TypeInfo de "%s"' % nombre)
    filas = []
    malos = 0
    for a, (n, g, unidad, motivo) in sorted(props.items()):
        r = pr.prueba(a, n)[0]
        if g == n:
            continue
        filas.append((a, n, g, r, motivo))
        print('0x%06x %-45s ahora %-45s %-22s %s' % (a, n[:45], g[:45], r, motivo))
    uso = collections.Counter(n for a, n, g, r, m in filas)
    print('%d propuestas; nombres repetidos: %s' % (len(filas), [n for n, k in uso.items() if k > 1]))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for a, n, g, r, motivo in filas:
                fh.write('0x%x\t%s\tcadena\t%s\t%s\n' % (a, n, r, motivo))


if __name__ == '__main__':
    main()
