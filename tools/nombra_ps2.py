#!/usr/bin/env python3
"""nombra_ps2.py <version> [unidad...] [--aplicar] -- recupera nombres de funciones de las builds de PS2 comparando,
unidad a unidad, el objeto del original troceado (build/<V>/obj) con nuestro objeto compilado (build/<V>/src).

Es la version MIPS/ELF de tools/nombra_por_codigo.py (Xbox 360 y PC, COFF): misma regla, una funcion del original
sin nombre (func_/fn_/sub_...) recibe el nombre de una nuestra cuando sus palabras coinciden enteras y de forma
UNICA en los dos sentidos; en las palabras con reubicacion (jal, %hi/%lo, gp-rel) solo se exige el codigo de
operacion, y los destinos con nombre real de las dos tienen que coincidir. Ademas, las propuestas buenas crecen a la
vez en el original y en nuestro objeto (subsecuencia creciente mas larga); las que quedan fuera de ese orden solo
valen si comparten al menos un destino con nombre.

Por que importa: objdiff empareja por NOMBRE. En SLUS-21351, 6.738 funciones del original (31 %, 814 kB) no tienen
nombre, asi que no pueden casar aunque nuestro codigo sea identico.

Solo informa; con --aplicar reescribe los nombres en config/<V>/symbols.txt. Despues hay que volver a trocear
(el troceado ya depende de symbols.txt) y medir con scripts/pctall.py <V>.

    python tools/nombra_ps2.py SLUS-21351
    python tools/nombra_ps2.py SLUS-21351 zAI zSim --aplicar
"""
import bisect
import collections
import glob
import os
import re
import struct
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'scripts'))
from escritura import entero

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MIN_PALABRAS = 5
SIN_NOMBRE = re.compile(r'^(fn|sub|func|lbl|zz|j_sub|nullsub)_[0-9A-Fa-f_]+$')


class Elf:
    """ELF32 little-endian (MIPS). Solo lo que hace falta: secciones, simbolos y reubicaciones."""

    def __init__(self, ruta):
        d = open(ruta, 'rb').read()
        self.d = d
        if d[:4] != b'\x7fELF':
            raise ValueError('no es ELF: ' + ruta)
        shoff, = struct.unpack_from('<I', d, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from('<HHH', d, 0x2e)
        self.secciones = []
        for i in range(shnum):
            o = shoff + i * shentsize
            nombre, tipo, flags, addr, off, tam, link, info, _, entsize = struct.unpack_from('<IIIIIIIIII', d, o)
            self.secciones.append(dict(nombre_off=nombre, tipo=tipo, flags=flags, addr=addr, off=off, tam=tam,
                                       link=link, info=info, entsize=entsize))
        shstr = self.secciones[shstrndx]
        for s in self.secciones:
            s['nombre'] = self._cadena(shstr['off'], s['nombre_off'])
        self.simbolos = []          # (nombre, valor, seccion, tipo)
        for s in self.secciones:
            if s['tipo'] != 2:      # SHT_SYMTAB
                continue
            str_off = self.secciones[s['link']]['off']
            for k in range(s['tam'] // 16):
                o = s['off'] + k * 16
                st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from('<IIIBBH', d, o)
                self.simbolos.append((self._cadena(str_off, st_name), st_value, st_shndx, st_info & 0xf, st_size))
        self.relocs = collections.defaultdict(list)   # seccion -> [(offset, nombre del destino)]
        for s in self.secciones:
            if s['tipo'] not in (9, 4):               # SHT_REL / SHT_RELA
                continue
            paso = 12 if s['tipo'] == 4 else 8
            simtab = [x[0] for x in self.simbolos]
            for k in range(s['tam'] // paso):
                o = s['off'] + k * paso
                r_offset, r_info = struct.unpack_from('<II', d, o)
                idx = r_info >> 8
                self.relocs[s['info']].append((r_offset, simtab[idx] if idx < len(simtab) else ''))

    def _cadena(self, base, off):
        fin = self.d.index(b'\0', base + off)
        return self.d[base + off:fin].decode('latin-1')

    def funciones(self):
        """[(nombre, bytes, reubicaciones relativas, orden)] de las funciones de secciones de codigo."""
        por_sec = collections.defaultdict(list)
        for nombre, valor, sec, tipo, tam in self.simbolos:
            if not nombre or sec == 0 or sec >= len(self.secciones):
                continue
            if tipo != 2:                              # STT_FUNC
                continue
            s = self.secciones[sec]
            if not (s['flags'] & 0x4):                 # SHF_EXECINSTR
                continue
            por_sec[sec].append((valor, nombre, tam))
        out = []
        for sec, lista in por_sec.items():
            lista.sort()
            s = self.secciones[sec]
            datos = self.d[s['off']:s['off'] + s['tam']]
            relocs = sorted(self.relocs.get(sec, []))
            for k, (valor, nombre, tam) in enumerate(lista):
                fin = valor + tam if tam else (lista[k + 1][0] if k + 1 < len(lista) else len(datos))
                if fin <= valor or fin > len(datos):
                    continue
                rel = [(r - valor, destino) for r, destino in relocs if valor <= r < fin]
                out.append((nombre, datos[valor:fin], rel, (sec, valor)))
        return out


def destinos(rel):
    """{indice de palabra: nombre del destino} para los destinos con nombre real."""
    out = {}
    for r, destino in rel:
        if not destino or destino.startswith(('.', '$', 'lbl_', 'L')) or SIN_NOMBRE.match(destino):
            continue
        # los datos que splat no sabe nombrar (D_XXXXXXXX, jtbl_) tampoco son nombres: si no, se rechaza la pareja
        # cuando nuestro objeto apunta al global con su nombre verdadero (228 nombres de SLUS el 2026-09-23)
        if re.match(r'^(D|jtbl)_[0-9A-Fa-f]{8}$', destino):
            continue
        out[r // 4] = destino
    return out


def normaliza(b, rel):
    """Palabras, con las reubicadas reducidas a su codigo de operacion."""
    n = len(b) // 4
    palabras = list(struct.unpack('<%dI' % n, b[:n * 4]))
    marcadas = {r // 4 for r, _ in rel}
    return tuple((w >> 26, None) if i in marcadas else (w, 0) for i, w in enumerate(palabras)), marcadas


def coinciden(a, b, ma, mb, da=None, db=None):
    if len(a) != len(b):
        return False
    if da and db:
        for k, nombre in da.items():
            if k in db and db[k] != nombre:
                return False
    for i, (x, y) in enumerate(zip(a, b)):
        if i in ma or i in mb:
            if (x[0] if i in ma else x[0] >> 26) != (y[0] if i in mb else y[0] >> 26):
                return False
        elif x != y:
            return False
    return True


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if not args:
        sys.exit(__doc__)
    version, unidades_pedidas = args[0], args[1:]
    aplicar = '--aplicar' in sys.argv
    os.chdir(ROOT)
    ya_nombrados = set()
    ruta_sym = 'config/%s/symbols.txt' % version
    for l in open(ruta_sym, encoding='utf-8', errors='replace'):
        m = re.match(r'(\S+) = ', l)
        if m:
            ya_nombrados.add(m.group(1))

    propuestas, evidencia, en_orden, resumen = {}, {}, set(), []
    base = 'build/%s/obj' % version
    for p in sorted(glob.glob(base + '/**/*.o', recursive=True)):
        rel = os.path.relpath(p, base).replace(os.sep, '/')
        if unidades_pedidas and not any(u in rel for u in unidades_pedidas):
            continue
        nuestro = 'build/%s/src/%s' % (version, rel)
        if not os.path.exists(nuestro):
            continue
        t, n = Elf(p), Elf(nuestro)
        nuestras = []
        for nombre, b, rel_, orden in n.funciones():
            # las etiquetas internas del compilador ($L..., .L...) no son nombres de funcion: ponerle una a una
            # funcion del objetivo rompe el informe entero de objdiff, y en silencio
            if nombre.startswith(('$L', '.L', '$LN', '__LINE__')):
                continue
            clave, marcas = normaliza(b, rel_)
            if len(clave) >= MIN_PALABRAS:
                nuestras.append((nombre, clave, marcas, destinos(rel_), orden))
        por_tam = collections.defaultdict(list)
        for x in nuestras:
            por_tam[len(x[1])].append(x)
        nombres_objetivo = {s[0] for s in t.simbolos}
        orden_nuestro = {x[0]: x[4] for x in nuestras}
        candidatas = halladas = 0
        en_unidad = []
        for nombre, b, rel_, orden_t in t.funciones():
            if not SIN_NOMBRE.match(nombre):
                if nombre in orden_nuestro:
                    en_unidad.append((orden_t, orden_nuestro[nombre], None))
                continue
            clave, marcas = normaliza(b, rel_)
            if len(clave) < MIN_PALABRAS:
                continue
            candidatas += 1
            dt = destinos(rel_)
            iguales = [x[0] for x in por_tam.get(len(clave), []) if coinciden(clave, x[1], marcas, x[2], dt, x[3])]
            iguales = [x for x in set(iguales) if x not in nombres_objetivo and x not in ya_nombrados]
            if len(iguales) == 1:
                halladas += 1
                propuestas.setdefault(nombre, set()).add(iguales[0])
                suya = [x for x in por_tam[len(clave)] if x[0] == iguales[0]][0]
                evidencia[nombre] = (len(clave), sum(1 for k, v in dt.items() if suya[3].get(k) == v))
                en_unidad.append((orden_t, suya[4], nombre))
        en_unidad.sort()
        colas, padre, idx_colas = [], [None] * len(en_unidad), []
        for i, (_, o, _) in enumerate(en_unidad):
            j = bisect.bisect_left(colas, o)
            if j == len(colas):
                colas.append(o)
                idx_colas.append(i)
            else:
                colas[j] = o
                idx_colas[j] = i
            padre[i] = idx_colas[j - 1] if j > 0 else None
        k = idx_colas[-1] if idx_colas else None
        while k is not None:
            if en_unidad[k][2] is not None:
                en_orden.add(en_unidad[k][2])
            k = padre[k]
        resumen.append((rel, candidatas, halladas, len(nuestras)))

    for rel, c, h, nn in resumen:
        print('%-55s sin nombre %5d  nombradas %5d  (nuestras %d)' % (rel, c, h, nn))
    uso = collections.Counter(next(iter(v)) for v in propuestas.values() if len(v) == 1)
    finales = {k: next(iter(v)) for k, v in propuestas.items() if len(v) == 1 and uso[next(iter(v))] == 1}
    fuera = [k for k in finales if k not in en_orden and evidencia[k][1] == 0]
    print('fuera de orden y sin destino coincidente (descartadas):', len(fuera))
    for k in fuera:
        del finales[k]
    print('propuestas unicas:', len(finales), 'de', len(propuestas))
    print('  con al menos un destino con nombre coincidente:',
          sum(1 for k in finales if evidencia[k][1] > 0))
    tams = sorted(evidencia[k][0] for k in finales)
    if tams:
        print('  palabras: min %d, mediana %d, max %d' % (tams[0], tams[len(tams) // 2], tams[-1]))
    for k in list(finales)[:15]:
        print('  ', k, '->', finales[k])

    if aplicar and finales:
        # dos casos: el nombre viejo esta en symbols.txt (se sustituye) o lo invento el troceador al no encontrar
        # ninguno (`func_<vram>`, que no esta en el fichero): entonces se ANADE la linea con esa direccion.
        t = open(ruta_sym, encoding='utf-8').read()
        lineas = t.split('\n')
        cambios = 0
        for i, l in enumerate(lineas):
            m = re.match(r'(\S+)( = .*)$', l)
            if m and m.group(1) in finales:
                lineas[i] = finales[m.group(1)] + m.group(2)
                cambios += 1
                del finales[m.group(1)]
        nuevas = []
        for viejo, nuevo in sorted(finales.items()):
            m = re.match(r'^(?:fn|sub|func)_([0-9A-Fa-f]+)$', viejo)
            if not m:
                continue
            # splat escribe cada simbolo en un fichero con su nombre: por encima de 250 bytes hay que darle uno
            # corto o falla el troceado entero (los nombres de plantilla pasan de 400)
            extra = ' filename:%s' % viejo if len(nuevo) > 250 else ''
            nuevas.append('%s = 0x%x; // type:func%s' % (nuevo, int(m.group(1), 16), extra))
        if nuevas:
            while lineas and not lineas[-1].strip():
                lineas.pop()
            lineas += ['', '// Nombres recuperados por codigo (tools/nombra_ps2.py): la funcion del original no tenia',
                       '// nombre y nuestro objeto define una identica palabra a palabra.'] + nuevas + ['']
        entero(ruta_sym, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
        print('aplicados %d (sustituidos) + %d (anadidos)' % (cambios, len(nuevas)))


if __name__ == '__main__':
    main()
