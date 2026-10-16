#!/usr/bin/env python3
"""Nombres de los eventos de zMain (clases `E*`: EBailPursuit, EShowSMS...) ANCLADOS por su cadena, para las
versiones de PS2. Cada evento tiene vtable propia y un `GetEventName` que devuelve el nombre de la clase:

    GetEventName__C12EBailPursuit:  lui $v0, %hi(s); jr $ra; addiu $v0, $v0, %lo(s)     s = "EBailPursuit"

Con la cadena se sabe de que clase es cada vtable (cabecera de 12 bytes a cero y pares (delta, puntero); se para en
la cabecera de la vtable siguiente), y de ahi, sin fiarse de ningun nombre heredado:
  - el primer puntero (+12) es el destructor `_$_<clase>`;
  - el puntero que devuelve la cadena es `GetEventName__C<clase>`;
  - la otra funcion que carga la vtable (%hi/%lo en el troceado) es el constructor: se le pone el nombre que define
    NUESTRO objeto de la unidad (la firma de esa version; si no hay uno solo, no se toca).
Validado en la A124, donde los nombres salen de la .symtab: 163 vtables ancladas, 0 nombres que no cuadran. En SLUS
(nombres heredados de la A124 por patron, y los eventos tienen codigo casi identico) destapo el 2026-09-23 131
nombres: 115 funciones sin nombre y 16 nombres cruzados entre clases.

Informa de cada nombre que no cuadra y de si nuestro codigo es identico al original (palabra a palabra; en las
reubicadas, codigo de operacion + rs + rt). Con --aplicar reescribe config/<V>/symbols.txt: cambia el nombre de la
direccion, anade las que no tenian linea y borra el nombre de la direccion vieja si se mueve. Despues: trocear y medir
con scripts/medir.py, y pasar scripts/cortes.py y scripts/restos_epilogo.py.

    python scripts/eventos_vtable.py SLES-53558-A124       # control: tiene que salir 0
    python scripts/eventos_vtable.py SLUS-21351
    python scripts/eventos_vtable.py SLUS-21351 --aplicar

Lee build/<V>/asm, build/<V>/obj y build/<V>/src (un troceado y una compilacion recientes).
"""
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
import nombra_ps2  # noqa: E402
from nombra_symtab import ficheros_asm  # noqa: E402
from escritura import entero  # noqa: E402

ELFS = {'SLUS-21351': 'orig/SLUS-21351/SLUS_213.51', 'SLES-53558-A124': 'orig/SLES-53558-A124/NFS.ELF'}
UNIDAD = 'Speed/Indep/SourceLists/zMain'
RE_INS = re.compile(r'^\s*/\*\s*([0-9A-F]+)\s+([0-9A-F]{8})\s+([0-9A-F]{8})\s*\*/\s+(\S+)\s*(.*)$')
RE_HILO = re.compile(r'%(?:hi|lo)\(([^)+]+?)(?:\s*\+\s*(0x[0-9A-Fa-f]+|\d+))?\)')


class Imagen:
    """Bytes del ELF original por direccion virtual (segmentos PT_LOAD)."""

    def __init__(self, ruta):
        self.d = open(ruta, 'rb').read()
        phoff, = struct.unpack_from('<I', self.d, 0x1C)
        phentsize, phnum = struct.unpack_from('<HH', self.d, 0x2A)
        self.segs = []
        for i in range(phnum):
            p = struct.unpack_from('<IIIIIIII', self.d, phoff + i * phentsize)
            if p[0] == 1:
                self.segs.append((p[2], p[1], p[4]))

    def lee(self, v, n):
        for va, off, tam in self.segs:
            if va <= v < va + tam:
                return self.d[off + v - va:off + v - va + n]
        raise ValueError(hex(v))


def simbolos(ruta):
    por_nom = {}
    for l in open(ruta, encoding='utf-8'):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);(.*)$', l.rstrip('\n'))
        if m:
            por_nom.setdefault(m.group(1), []).append((int(m.group(2), 16), m.group(3)))
    return por_nom


def funciones(ruta):
    out = {}
    if os.path.exists(ruta):
        for nombre, b, rel, orden in nombra_ps2.Elf(ruta).funciones():
            out.setdefault(nombre, (b, rel))
    return out


def compara(t, o):
    """IGUAL o la cuenta de palabras distintas entre la funcion troceada t y la nuestra o, cada una (bytes, rel)"""
    (tb, trel), (ob, orel) = t, o
    if len(tb) != len(ob):
        return 'tamano %d/%d' % (len(ob), len(tb))
    n = len(tb) // 4
    wt, wo = struct.unpack('<%dI' % n, tb[:n * 4]), struct.unpack('<%dI' % n, ob[:n * 4])
    rt, ro = {r // 4 for r, _ in trel}, {r // 4 for r, _ in orel}
    dif = 0
    for k in range(n):
        if k in rt or k in ro:
            m = 0xFC000000 if (wt[k] >> 26) in (2, 3) else 0xFFFF0000
            dif += (wt[k] & m) != (wo[k] & m)
        else:
            dif += wt[k] != wo[k]
    return 'IGUAL' if not dif else '%d palabras distintas' % dif


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if not args or args[0] not in ELFS:
        sys.exit(__doc__)
    V = args[0]
    img = Imagen(os.path.join(RAIZ, ELFS[V]))
    ruta_sym = os.path.join(RAIZ, 'config', V, 'symbols.txt')
    por_nom = simbolos(ruta_sym)
    ini, ins, cargas = {}, {}, collections.defaultdict(set)
    for p, unidad in ficheros_asm(V):
        cur = None
        for l in open(p, encoding='utf-8', errors='replace'):
            m = re.match(r'\s*glabel (\S+)', l)
            if m:
                cur = m.group(1)
                continue
            mi = RE_INS.match(l)
            if not (mi and cur):
                continue
            v = int(mi.group(2), 16)
            if cur not in ini:
                ini[cur] = v
                ins[v] = []
            if len(ins[ini[cur]]) < 3:
                ins[ini[cur]].append((mi.group(4), mi.group(5).replace(' ', '')))
            for sym, add in RE_HILO.findall(mi.group(5)):
                if re.match(r'D_[0-9A-F]{8}$', sym):
                    a = int(sym[2:], 16)
                elif sym in por_nom:
                    a = por_nom[sym][0][0]
                else:
                    continue
                cargas[cur].add(a + (int(add, 0) if add else 0))
    nombre_en = {v: n for n, v in ini.items()}

    def cadena(fdir):
        i = ins.get(fdir, [])
        if len(i) < 3 or [x[0] for x in i] != ['lui', 'jr', 'addiu']:
            return None
        m = re.search(r'%hi\(([^)]+)\)', i[0][1])
        s = m.group(1) if m else ''
        a = int(s[2:], 16) if re.match(r'D_[0-9A-F]{8}$', s) else (por_nom[s][0][0] if s in por_nom else None)
        if a is None:
            return None
        t = img.lee(a, 64).split(b'\0')[0]
        return t.decode('ascii') if re.match(rb'^E[A-Za-z0-9_]+$', t) else None

    vtables = {}
    for f, adrs in cargas.items():
        for a in adrs:
            if a in vtables:
                continue
            try:
                ws = struct.unpack('<16I', img.lee(a, 64))
            except (ValueError, struct.error):
                continue
            if ws[0] or ws[1] or ws[2]:
                continue
            for k in range(3, 16, 2):
                if ws[k] == 0:
                    break                           # (0, 0): la cabecera de la vtable siguiente
                c = cadena(ws[k])
                if c:
                    vtables[a] = (c, ws[k], ws[3])
                    break
    nuestras = funciones(os.path.join(RAIZ, 'build', V, 'src', UNIDAD + '.o'))
    troceadas = funciones(os.path.join(RAIZ, 'build', V, 'obj', UNIDAD + '.o'))
    cambios = {}
    for vt, (clase, fget, fdtor) in sorted(vtables.items()):
        k = '%d%s' % (len(clase), clase)
        cambios.setdefault(fget, 'GetEventName__C' + k)
        cambios.setdefault(fdtor, '_$_' + k)
        ctors = [n for n in nuestras if n.startswith('__' + k) and not n[len(k) + 2:len(k) + 3].isdigit()]
        for f, adrs in cargas.items():
            if vt in adrs and ini[f] not in (fdtor, fget):
                if len(ctors) == 1:
                    cambios.setdefault(ini[f], ctors[0])
                elif nombre_en.get(ini[f], '').startswith('__' + k):
                    cambios.setdefault(ini[f], nombre_en[ini[f]])
    assert len(set(cambios.values())) == len(cambios), 'un nombre para dos direcciones'
    malos = cuenta = 0
    for a, n in sorted(cambios.items()):
        actual = nombre_en.get(a, '?')
        if actual == n:
            continue
        malos += 1
        est = compara(troceadas[actual], nuestras[n]) if actual in troceadas and n in nuestras else 'sin comparar'
        cuenta += est == 'IGUAL'
        print('0x%06x %-44s -> %-50s %s' % (a, actual[:44], n[:50], est))
    print('%s: %d vtables de evento ancladas; %d nombres no cuadran (%d con nuestro codigo identico)'
          % (V, len(vtables), malos, cuenta))
    if '--aplicar' not in sys.argv or not malos:
        return
    raw = open(ruta_sym, 'rb').read().decode('utf-8')
    assert '\r\n' not in raw, 'symbols.txt tiene que ser LF'
    lineas, hechas, borrar = raw.split('\n'), set(), []
    for i, l in enumerate(lineas):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);(.*)$', l)
        if not m:
            continue
        n, a, resto = m.group(1), int(m.group(2), 16), m.group(3)
        if a in cambios and 'type:func' in resto:
            lineas[i] = '%s = 0x%x;%s' % (cambios[a], a, resto)
            hechas.add(a)
        elif n in cambios.values() and a not in cambios:
            borrar.append(i)                         # el nombre se va a su sitio
    for i in reversed(borrar):
        del lineas[i]
    while lineas and not lineas[-1].strip():
        lineas.pop()
    nuevas = ['%s = 0x%x; // type:func' % (n, a) for a, n in sorted(cambios.items()) if a not in hechas]
    if nuevas:
        lineas += ['', '// Eventos de zMain anclados por la cadena de GetEventName (scripts/eventos_vtable.py).'] + nuevas
    lineas.append('')
    entero(ruta_sym, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
    print('escrito %s: %d cambiadas, %d anadidas, %d borradas' % (os.path.relpath(ruta_sym, RAIZ), len(hechas),
                                                                 len(nuevas), len(borrar)))


if __name__ == '__main__':
    main()
