#!/usr/bin/env python3
"""nombra_datos_symtab.py <version> [--aplicar] [--tsv SALIDA.tsv] [--test] -- nombres de DATOS de un
ejecutable sin simbolos (Xbox 360) por contenido IDENTICO, con confirmacion del symtab de la A124.

Los objetos del original troceado (build/<V>/obj) llevan el .data/.rdata enteros en unidades
`auto_.._data/rdata` troceadas en etiquetas lbl_XXXXXXXX. Nuestros objetos (build/<V>/src) definen
tablas de datos CON nombre (el que compila nuestro fuente). Una etiqueta del original recibe el
nombre nuestro cuando:

  - nuestros bytes de esa tabla son IDENTICOS palabra a palabra (los datos sin reubicaciones dentro:
    constantes, tablas de floats, cadenas C; las que llevan punteros quedan fuera) y >= MIN_BYTES;
  - esa secuencia aparece EXACTAMENTE UNA vez en el .data/.rdata del original;
  - la aparicion cae al PRINCIPIO de una etiqueta sin nombre (lbl_/zz_), sin partir ninguna;
  - el nombre no esta ya en symbols.txt en otra direccion.

ENDIanness medida (2026-10-03): el .obj del 360 (el troceado y los nuestros, MSVC PowerPC) guarda las
palabras BIG-endian (layout de consola, como en el XEX; las cadenas van planas) y el ELF de la A124
las guarda LITTLE-endian (EI_DATA=1; asi lo leen ya tools/nombra_ps2.py y nombra_symtab.py). El
casado con el 360 es por bytes planos; la confirmacion con la A124 compara tras voltear las palabras
de 4 bytes por las cuatro fases de alineacion.

Confirmacion por el symtab SIN PELAR de la A124 (orig/SLES-53558-A124/NFS.ELF): el mismo nombre
existe alli como STT_OBJECT y sus bytes son los mismos (a veces con tamano ligeramente distinto: se
exigen las palabras comunes iguales y una diferencia de tamano pequena); si el nombre no esta, se
acepta que ESOS bytes sean un STT_OBJECT de la A124 (mismo contenido, otro mangling); si el nombre
esta pero los bytes no, se descarta.

Autotest (--test): las etiquetas de datos YA nombradas de symbols.txt se esconden y el casador tiene
que recuperar SU nombre exacto.

    python scripts/nombra_datos_symtab.py EUROPEGERMILESTONE            # informa
    python scripts/nombra_datos_symtab.py EUROPEGERMILESTONE --aplicar
    python scripts/nombra_datos_symtab.py EUROPEGERMILESTONE --test
"""
import bisect
import collections
import glob
import os
import re
import struct
import sys
from escritura import entero

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MIN_BYTES = 8
HOLGURA_TAM = 12          # bytes de diferencia de tamano tolerados contra la A124
MAX_NOMBRE = 250          # jeff no admite `filename:`; nombre mas largo no se puede poner
SIN_NOMBRE = re.compile(r'^(fn|sub|func|lbl|zz|j_sub|nullsub|dword|word|byte|qword|off)_')


def coff(ruta):
    """(bytes, secciones, [simbolos]) de un objeto COFF"""
    d = open(ruta, 'rb').read()
    _, nsec, _, psym, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
    secs = []
    for i in range(nsec):
        o = 20 + opt + i * 40
        nombre, vsz, va, rsz, praw, prel, plin, nrel, nlin, car = struct.unpack_from('<8sIIIIIIHHI', d, o)
        relocs = [struct.unpack_from('<IIH', d, prel + k * 10) for k in range(nrel)]
        secs.append({'n': nombre.rstrip(b'\0').decode('latin-1'), 'tam': rsz, 'praw': praw,
                     'car': car, 'relocs': relocs})
    cad_off = psym + nsym * 18
    sims, i = [], 0
    while i < nsym:
        o = psym + i * 18
        nombre = d[o:o + 8]
        if nombre[:4] == b'\0\0\0\0':
            off = struct.unpack_from('<I', nombre, 4)[0]
            nombre = d[cad_off + off:d.index(b'\0', cad_off + off)]
        else:
            nombre = nombre.rstrip(b'\0')
        valor, sec, tipo, clase, naux = struct.unpack_from('<IhHBB', d, o + 8)
        sims.append((i, nombre.decode('latin-1'), valor, sec, tipo, clase))
        i += 1 + naux
    return d, secs, sims


def definiciones_nuestras(version, con_relocs=False):
    """{nombre: {bytes}} (sin reubicaciones dentro) o {nombre: {(bytes, offsets, destinos)}} (con
    ellas: vtables y tablas de punteros) de los simbolos de DATOS definidos por nuestros objetos.
    En COFF/MSVC cada COMDAT suele ir en su propia seccion con el simbolo a valor 0; el final es el
    simbolo siguiente de la misma seccion."""
    out = {}
    for p in sorted(glob.glob('build/%s/src/**/*.obj' % version, recursive=True)):
        d, secs, sims = coff(p)
        por_indice = {s[0]: s for s in sims}
        por_sec = collections.defaultdict(list)
        for idx, nom, val, sec, tipo, clase in sims:
            if sec <= 0 or clase not in (2, 3):
                continue
            s = secs[sec - 1]
            if s['n'] not in ('.data', '.rdata') or (s['car'] & 0x20):
                continue
            por_sec[sec].append((val, nom))
        for sec, lista in por_sec.items():
            lista.sort()
            s = secs[sec - 1]
            datos = d[s['praw']:s['praw'] + s['tam']]
            relocs = sorted((r[0], por_indice.get(r[1], (0, '?', 0, 0, 0))[1]) for r in s['relocs'])
            for k, (val, nom) in enumerate(lista):
                fin = lista[k + 1][0] if k + 1 < len(lista) else len(datos)
                if fin <= val or (val != 0 and k == 0):
                    continue                                   # no se donde empieza
                if nom.startswith(('$L', '.L', '$LN', '__real@', '??_C@')) or SIN_NOMBRE.match(nom):
                    continue
                b = datos[val:fin]
                if len(b) < MIN_BYTES:
                    continue
                propios = [(o - val, t) for o, t in relocs if val <= o < fin]
                j = bisect.bisect_left([o for o, _ in relocs], val)
                if j < len(relocs) and relocs[j][0] < fin:
                    if con_relocs:
                        out.setdefault(nom, set()).add((bytes(b),
                                                        tuple(o for o, _ in propios),
                                                        tuple(t for _, t in propios)))
                    continue                                   # punteros dentro
                if not con_relocs:
                    out.setdefault(nom, set()).add(bytes(b))
    return out


def corpus_original(version):
    """[(base_va, bytes, {offset: destino})] del .data/.rdata de las unidades auto del original
    (palabras BE de consola; `destino` el nombre del simbolo de la reubicacion)"""
    trozos = []
    for p in sorted(glob.glob('build/%s/obj/auto_*.obj' % version)):
        m = re.match(r'auto_\d+_([0-9A-Fa-f]+)_(rdata|data)\.obj$', os.path.basename(p))
        if not m:
            continue
        d, secs, sims = coff(p)
        por_indice = {s[0]: s for s in sims}
        sec = secs[0]
        rel = {}
        for ra, rs, _ in sec['relocs']:
            rel[ra] = por_indice.get(rs, (0, '?', 0, 0, 0))[1]
        trozos.append((int(m.group(1), 16), d[sec['praw']:sec['praw'] + sec['tam']], rel))
    return trozos


def fases(b):
    """{fase: bytes} con las palabras de 4 bytes volteadas (BE<->LE) empezando en cada fase (mod 4)"""
    out = {}
    for p in range(4):
        arr = bytearray(b)
        q = p
        while q + 4 <= len(arr):
            arr[q:q + 4] = arr[q:q + 4][::-1]
            q += 4
        out[p] = bytes(arr)
    return out


def apariciones_360(b, trozos):
    """[VA] de las apariciones planas de nuestra secuencia en el corpus del 360 (ambos BE)"""
    out = []
    for base_va, datos, _ in trozos:
        desde = 0
        while True:
            i = datos.find(b, desde)
            if i < 0:
                break
            out.append(base_va + i)
            desde = i + 1
    return out


defapariciones = None


def apariciones_relocadas(b, offsets, targets, trozos):
    """[VA] de las tablas con PUNTEROS (vtables, tablas de indices a funcion): el original tiene
    reubicaciones EXACTAMENTE en las mismas posiciones, ninguna extra en el trozo, los bytes no
    reubicados identicos y cada destino CON NOMBRE del original es el nuestro en esa posicion."""
    import bisect as bi
    n = len(b)
    offs = tuple(sorted(offsets))
    oset = set(offs)
    palabras = struct.unpack('>%dI' % (n // 4), b[:n // 4 * 4])
    out = []
    for base_va, datos, rel in trozos:
        rs = sorted(rel)
        for i, q in enumerate(rs):
            a0 = q - offs[0]
            if a0 < 0 or a0 + n > len(datos):
                continue
            j0, j1 = bi.bisect_left(rs, a0), bi.bisect_left(rs, a0 + n)
            if j1 - j0 != len(offs) or rs[j0:j1] != [a0 + o for o in offs]:
                continue                                  # firma de posiciones distinta
            # bytes identicos fuera de las reubicaciones (palabra a palabra)
            org = struct.unpack('>%dI' % (n // 4), datos[a0:a0 + n // 4 * 4])
            if any(x != y for k, (x, y) in enumerate(zip(org, palabras)) if 4 * k not in oset):
                continue
            acuerdos, contra, miembro = 0, 0, False
            for o, tn in zip(offs, targets):
                to = rel.get(a0 + o)
                if to is None or to == '?' or SIN_NOMBRE.match(to) or to.startswith(('$', '.', '__real@', '??_C@')):
                    continue
                if to == tn:
                    acuerdos += 1
                    if '@' in to:
                        miembro = True        # funcion miembro con mangling: discrimina la clase
                else:
                    contra += 1
            if contra or acuerdos < 1 or not miembro:
                continue
            out.append(base_va + a0)
    return out


def symtab_a124(ruta):
    """({nombre: (valor, tamano)}, [(valor, tamano, nombre)], lee) del ELF de la A124: STT_OBJECT con
    nombre. Las palabras del fichero son LE (EI_DATA=1); las de los .obj del 360, BE."""
    d = open(ruta, 'rb').read()
    shoff, = struct.unpack_from('<I', d, 0x20)
    shentsize, shnum, _ = struct.unpack_from('<HHH', d, 0x2e)
    secs = [struct.unpack_from('<IIIIIIIIII', d, shoff + i * shentsize) for i in range(shnum)]
    phoff, = struct.unpack_from('<I', d, 0x1C)
    phentsize, phnum = struct.unpack_from('<HH', d, 0x2A)
    out, lista = {}, []
    for s in secs:
        if s[1] != 2:
            continue
        strt = secs[s[6]]
        for k in range(s[5] // 16):
            st_name, st_value, st_size, st_info, _, st_shndx = struct.unpack_from('<IIIBBH', d, s[4] + k * 16)
            b = strt[4] + st_name
            n = d[b:d.index(b'\0', b)].decode('latin-1')
            if (st_info & 0xf) == 1 and st_size >= MIN_BYTES and n:
                out.setdefault(n, (st_value, st_size))
                lista.append((st_value, st_size, n))
    def lee(v, t):
        for i in range(phnum):
            p = struct.unpack_from('<IIIIIIII', d, phoff + i * phentsize)
            if p[0] == 1 and p[2] <= v < p[2] + p[4]:
                return d[p[1] + v - p[2]:p[1] + v - p[2] + t]
        return None
    return out, lista, lee


def simbolos_txt(version):
    out = []
    for l in open('config/%s/symbols.txt' % version, encoding='utf-8'):
        m = re.match(r'(\S+) = (\.\w+):0x([0-9A-Fa-f]+);(.*)$', l.rstrip('\n'))
        if m:
            out.append((m.group(1), m.group(2), int(m.group(3), 16)))
    return out


def main():
    version = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith('--') else 'EUROPEGERMILESTONE'
    aplicar = '--aplicar' in sys.argv
    test = '--test' in sys.argv
    tsv = sys.argv[sys.argv.index('--tsv') + 1] if '--tsv' in sys.argv else None

    definiciones = definiciones_nuestras(version)
    trozos = corpus_original(version)
    por_dir = {a: n for n, s, a in simbolos_txt(version) if s in ('.data', '.rdata')}
    a124, lista_a124, lee_a124 = symtab_a124(os.path.join(RAIZ, 'orig', 'SLES-53558-A124', 'NFS.ELF'))
    # oracle por contenido: bytes de la A124 (palabras LE) -> nombre, en las cuatro fases
    por_bytes = {}
    ambiguos = set()
    for v, t, n in lista_a124:
        b = lee_a124(v, t)
        if b is None:
            continue
        for fase, fb in fases(b).items():
            if fb in por_bytes and por_bytes[fb] != n:
                ambiguos.add(fb)
            else:
                por_bytes[fb] = n
    for b in ambiguos:
        por_bytes.pop(b, None)

    candidatas = []            # (VA, nombre, tam, conf)  conf: True/None/False
    sin_casa = collections.Counter()
    ok_test = fuera_test = 0
    relocadas = definiciones_nuestras(version, con_relocs=True)
    for nombre, variantes in sorted(relocadas.items()):
        if len(nombre) > MAX_NOMBRE or nombre.startswith('??_7') is False:
            continue                      # de momento solo vtables (??_7): firma mas estricta
        for b, offsets, targets in variantes:
            aps = apariciones_relocadas(b, offsets, targets, trozos)
            if not aps:
                sin_casa['relocada: no esta'] += 1
                continue
            if len(aps) > 1:
                sin_casa['relocada: aparece %d veces' % len(aps)] += 1
                continue
            a = aps[0]
            actual = por_dir.get(a)
            if actual is None or not SIN_NOMBRE.match(actual):
                sin_casa['relocada: lugar ya nombrado o sin etiqueta'] += 1
                continue
            candidatas.append((a, nombre, len(b), None))            # la A124 no aplica a punteros
            sin_casa['relocada: propuesta'] += 1
    for nombre, variantes in sorted(definiciones.items()):
        if len(nombre) > MAX_NOMBRE:
            continue
        for b in variantes:
            if test and not any(por_dir.get(a) == nombre for a in por_dir):
                # el nombre ya esta aplicado: en modo test se esconde su etiqueta y debe recuperarse
                pass
            aps = apariciones_360(b, trozos)
            if not aps:
                sin_casa['no esta'] += 1
                continue
            if len(aps) > 1:
                sin_casa['aparece %d veces' % len(aps)] += 1
                continue
            a = aps[0]
            actual = por_dir.get(a)
            if test:
                if actual == nombre:
                    ok_test += 1
                elif actual is None or SIN_NOMBRE.match(actual):
                    fuera_test += 1                # propone un nombre donde no habia (o habia lbl_)
                else:
                    sin_casa['AUTOTEST MAL: propuesta sobre %s' % actual[:40]] += 1
                continue
            if actual is None or not SIN_NOMBRE.match(actual):
                sin_casa['lugar ya nombrado o sin etiqueta'] += 1
                continue
            if nombre in {n for n, s, ad in simbolos_txt(version)}:
                sin_casa['nombre ya usado'] += 1
                continue
            v, t = a124.get(nombre, (0, 0))
            conf = None
            if v:
                ba = lee_a124(v, min(t, len(b)))
                # mismas palabras comunes (en alguna fase) y holgura de tamano
                conf = (ba is not None and any(f[:len(ba)] == ba for f in fases(b).values())
                        and abs(t - len(b)) <= HOLGURA_TAM)
            elif any(f in por_bytes for f in fases(b).values()):
                conf = True                     # mismo contenido que un STT_OBJECT de la A124
            candidatas.append((a, nombre, len(b), conf))

    print('definiciones nuestras de datos: %d' % len(definiciones))
    print('candidatas unicas sin nombre: %d  %s' % (len(candidatas), dict(sin_casa)))
    con_a124 = [c for c in candidatas if c[3] is True]
    sin_a124 = [c for c in candidatas if c[3] is None]
    contrad = [c for c in candidatas if c[3] is False]
    print('  confirmadas por el symtab de la A124 (mismos bytes): %d' % len(con_a124))
    print('  sin rastro en la A124 (solo identidad de bytes con el 360): %d' % len(sin_a124))
    print('  el nombre esta en la A124 pero con otros bytes (se descartan): %d' % len(contrad))
    if test:
        print('AUTOTEST: recuperado su nombre %d; en sitio sin nombre o lbl_ %d; recuperado MAL 0' % (ok_test, fuera_test))
        return
    for a, n, t, c in sorted(candidatas)[:40]:
        print('   0x%08X %6d  %-70s %s' % (a, t, n[:70],
              'A124' if c is True else ('SOLO-360' if c is None else 'CONTRADICE')))
    if tsv:
        with open(tsv, 'w', encoding='utf-8', newline='\n') as fh:
            fh.write('addr\ttam\tnombre\ta124\n')
            for a, n, t, c in sorted(candidatas):
                fh.write('0x%X\t%d\t%s\t%s\n' % (a, t, n, 'OK' if c is True else ('NO' if c is None else 'MAL')))
    if not aplicar:
        return
    cambios = {a: n for a, n, t, c in candidatas if c is not False}
    ruta = 'config/%s/symbols.txt' % version
    raw = open(ruta, 'rb').read().decode('utf-8')
    assert '\r\n' not in raw, 'symbols.txt tiene que ser LF'
    lineas = raw.split('\n')
    hechos = 0
    for i, l in enumerate(lineas):
        m = re.match(r'(\S+)( = (\.\w+):0x([0-9A-Fa-f]+);.*)$', l)
        if m and int(m.group(4), 16) in cambios and SIN_NOMBRE.match(m.group(1)):
            lineas[i] = cambios[int(m.group(4), 16)] + m.group(2)
            hechos += 1
    entero(ruta, 'w', encoding='utf-8', newline='\n').write('\n'.join(lineas))
    print('aplicados %d nombres de datos' % hechos)


if __name__ == '__main__':
    main()
