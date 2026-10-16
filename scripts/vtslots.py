#!/usr/bin/env python3
"""vtslots.py -- nombres de SLUS anclados por la RANURA de su vtable.

Los nombres de config/SLUS-21351/symbols.txt vienen heredados de la A124 por direccion y por codigo; entre clases
hermanas con el mismo codigo (destructores, GetTypeInfo/GetTypeName de las SFXCTL_3D*...) se cruzan, y ninguna
comparacion de codigo lo ve. La vtable si: cada vtable de la A124 tiene nombre (`_vt$Clase`, con size: en
symbols.txt, de la .symtab) y sus ranuras apuntan a funciones con nombre real.

  1. La misma vtable de SLUS esta en otra direccion y sin nombre; se busca por VOTOS: cada ranura cuya funcion de SLUS
     tiene un nombre PROBADO (IGUAL a nuestra funcion de ese nombre, palabra a palabra como scripts/prueba_nombre.py)
     y de codigo UNICO en nuestra unidad vota por un origen. Con votos por nombre a secas, EAXTruck se casaba con la
     vtable de EAXCopCar porque sus nombres de SLUS estaban cruzados.
  2. Uno a uno: un origen reclamado por dos vtables se queda con la de mas votos (si empatan, ninguna).
  3. Consistencia global: cada funcion de la A124 corresponde a UNA de SLUS en todas las vtables casadas; se quitan
     las vtables que quedan en minoria en algun choque.
  4. Con el origen, ranura a ranura: el puntero de SLUS es la funcion que la A124 tiene en esa ranura. Segunda
     referencia: la vtable que compila NUESTRO objeto de SLUS (build/SLUS-21351/src, reubicaciones por ranura, las de
     simbolo de seccion resueltas por el addend), que sale de las cabeceras que casan con GameCube. Solo se propone un
     nombre si las dos dicen lo mismo y ninguna otra vtable lo contradice.

    python scripts/vtslots.py [--tsv propuestas.tsv]

Imprime las ranuras cuyo nombre actual de SLUS no es el de la ranura (vtable, desplazamiento, direccion de SLUS,
nombre actual, nombre de la A124 y el de nuestra vtable), los conflictos y el numero de propuestas. --tsv vuelca las
propuestas (`0xDIR<TAB>nombre<TAB>vtable<TAB>clase`), que se aplican con scripts/aplica_nombres.py; despues hay que
trocear y medir SLUS (scripts/medir.py) y pasar scripts/cortes.py, scripts/restos_epilogo.py y
scripts/restos_trozos.py.

Lee los troceados, los objetos y los ELF de la raiz donde este el guion; copiado a una raiz de prueba (un troceado
hecho fuera del arbol), trabaja sobre ella. La primera vuelta (tanda 5, 2026-09-23) dio 279 propuestas: 148
confirmaron el lote de 622 nombres por orden sin una sola contradiccion, y las de EAXCop, EAXAirSupport, EAXCharacter y
EAXDispatch probaron 10 de los 11 nombres de funcion que el lote movia y dieron nombre a sus 10 direcciones viejas.
"""
import collections
import glob
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
sys.path.insert(0, os.path.join(RAIZ, 'tools'))
from vtable_dtors import Imagen, ELFS  # noqa: E402
import nombra_ps2  # noqa: E402
import prueba_lote  # noqa: E402


def firma(b, rel):
    """codigo normalizado como lo compara scripts/prueba_nombre.py: en las palabras reubicadas, codigo de operacion y
    registros (tipo I) o solo el codigo (j/jal)"""
    n = len(b) // 4
    ws = struct.unpack('<%dI' % n, b[:n * 4])
    marcadas = {r // 4 for r, d in rel}
    out = []
    for k, w in enumerate(ws):
        if k in marcadas:
            out.append(w & (0xFC000000 if (w >> 26) in (2, 3) else 0xFFFF0000))
        else:
            out.append(w)
    return tuple(out)

A, S = 'SLES-53558-A124', 'SLUS-21351'
SIN = re.compile(r'^(func|fn|sub|lbl)_[0-9A-Fa-f]+$')


def simbolos(V):
    por_dir, por_nom, tam = collections.defaultdict(list), {}, {}
    for l in open(os.path.join(RAIZ, 'config', V, 'symbols.txt'), encoding='utf-8'):
        m = re.match(r'(\S+) = 0x([0-9a-fA-F]+);(.*)', l)
        if not m:
            continue
        a = int(m.group(2), 16)
        por_dir[a].append(m.group(1))
        por_nom.setdefault(m.group(1), a)
        t = re.search(r'size:0x([0-9a-fA-F]+)', m.group(3))
        if t:
            tam[m.group(1)] = int(t.group(1), 16)
    return por_dir, por_nom, tam


def texto_de(ruta):
    """(ini, fin) de .text (y overlays de codigo) del ELF"""
    img = Imagen(ruta)
    return img


def lee_obj(p):
    """simbolos [(nombre, valor, seccion, tipo, tam)], datos de secciones y reubicaciones con addend resuelto:
    {seccion: [(offset, nombre destino)]}; las de simbolo de seccion se resuelven a la funcion de ese desplazamiento"""
    d = open(p, 'rb').read()
    if d[:4] != b'\x7fELF':
        return None
    shoff, = struct.unpack_from('<I', d, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', d, 0x2e)
    secs = [struct.unpack_from('<IIIIIIIIII', d, shoff + i * shentsize) for i in range(shnum)]
    def cad(base, off):
        return d[base + off:d.index(b'\0', base + off)].decode('latin-1')
    syms = []
    for s in secs:
        if s[1] != 2:
            continue
        stro = secs[s[6]][4]
        for k in range(s[5] // 16):
            st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from('<IIIBBH', d, s[4] + k * 16)
            syms.append((cad(stro, st_name), st_value, st_shndx, st_info & 0xf, st_size))
    # funcion por (seccion, desplazamiento)
    fn_en = {}
    for n, v, sec, t, tam in syms:
        if t == 2 and n:
            fn_en.setdefault((sec, v), n)
    rel = collections.defaultdict(list)
    for s in secs:
        if s[1] not in (9, 4):
            continue
        paso = 12 if s[1] == 4 else 8
        destino_sec = s[7]
        dsec = secs[destino_sec]
        for k in range(s[5] // paso):
            r_offset, r_info = struct.unpack_from('<II', d, s[4] + k * paso)
            idx, tipo = r_info >> 8, r_info & 0xff
            if idx >= len(syms):
                continue
            n, v, ssec, t, tam = syms[idx]
            if t == 3 or not n:            # simbolo de seccion: el addend esta en la palabra (REL)
                if tipo != 2:              # solo R_MIPS_32
                    continue
                add, = struct.unpack_from('<I', d, dsec[4] + r_offset)
                n = fn_en.get((ssec, v + add), '')
            rel[destino_sec].append((r_offset, n))
    return syms, rel


def nuestras_vtables():
    """{nombre de vtable: [(indice de palabra, nombre destino)]} de los objetos de SLUS"""
    out = {}
    for p in glob.glob(os.path.join(RAIZ, 'build', S, 'src', '**', '*.o'), recursive=True):
        r = lee_obj(p)
        if not r:
            continue
        syms, rel = r
        for nombre, valor, sec, tipo, tamano in syms:
            if not nombre.startswith('_vt$') or tipo != 1 or not sec:
                continue
            rr = [(o - valor, n) for o, n in rel.get(sec, []) if valor <= o < valor + tamano]
            out.setdefault(nombre, sorted((o // 4, n) for o, n in rr))
    return out


def main():
    ia, isl = Imagen(os.path.join(RAIZ, ELFS[A])), Imagen(os.path.join(RAIZ, ELFS[S]))
    pa_dir, pa_nom, pa_tam = simbolos(A)
    ps_dir, ps_nom, _ = simbolos(S)
    # palabras de datos de SLUS: valor -> posiciones (solo valores que son direcciones con nombre)
    funcs_s = set(ps_dir)
    indice = collections.defaultdict(list)
    for va, off, tamseg in isl.segs:
        d = isl.d[off:off + tamseg]
        for k in range(0, len(d) - 3, 4):
            w = struct.unpack_from('<I', d, k)[0]
            if w in funcs_s:
                indice[w].append(va + k)
    nuestras = nuestras_vtables()
    # votos de confianza: solo cuentan las ranuras cuya funcion de SLUS tiene un nombre PROBADO por codigo (IGUAL a
    # nuestra funcion de ese nombre) y cuyo codigo es UNICO en nuestra unidad. Los destructores de clases hermanas y
    # los GetTypeInfo/GetTypeName de las SFXCTL_3D* son iguales salvo una reubicacion: con votos por nombre, una vtable
    # de la A124 se casaba con la de la hermana si los nombres de SLUS estaban cruzados.
    pr = prueba_lote.Probador(S)
    firmas = {}

    def firma_unica(unidad, nombre):
        if unidad not in firmas:
            t, o = pr.objs(unidad)
            grupos = collections.Counter()
            por_nombre = {}
            for n, lst in o.items():
                for b, rel in lst:
                    k = firma(b, rel)
                    grupos[k] += 1
                    por_nombre[n] = k
            firmas[unidad] = (grupos, por_nombre)
        grupos, por_nombre = firmas[unidad]
        return nombre in por_nombre and grupos[por_nombre[nombre]] == 1
    confianza = {}

    def fiable(a):
        if a not in confianza:
            ok = False
            for n in ps_dir.get(a, []):
                r, g, u = pr.prueba(a, n)
                if r == 'IGUAL' and firma_unica(u, n):
                    ok = True
            confianza[a] = ok
        return confianza[a]
    # 1) origen en SLUS de cada vtable de la A124, por votos
    origen = {}
    sin_origen = 0
    for vt, tam in sorted(pa_tam.items()):
        if not vt.startswith('_vt$'):
            continue
        a0 = pa_nom[vt]
        try:
            ws = struct.unpack('<%dI' % (tam // 4), ia.lee(a0, tam))
        except (ValueError, struct.error):
            continue
        ranuras = [(i, w, pa_dir[w][0]) for i, w in enumerate(ws) if w and w in pa_dir]
        votos = collections.Counter()
        for i, w, n in ranuras:
            if n in ps_nom and ps_nom[n] in indice and fiable(ps_nom[n]):
                for pos in indice[ps_nom[n]]:
                    votos[pos - 4 * i] += 1
        conocidas = sum(1 for i, w, n in ranuras if n in ps_nom and ps_nom[n] in indice and fiable(ps_nom[n]))
        if not votos:
            sin_origen += 1
            continue
        (orig, v1), = votos.most_common(1)
        v2 = votos.most_common(2)[1][1] if len(votos) > 1 else 0
        # el origen bueno tiene estrictamente mas votos que cualquier otro (una vtable hermana comparte las ranuras
        # heredadas y pierde en las redefinidas) y al menos la mitad de las ranuras con nombre conocido en SLUS
        if v1 < 2 or v2 >= v1 or v1 * 2 < conocidas:
            sin_origen += 1
            continue
        origen[vt] = (orig, v1, ws, ranuras, a0)
    # 2) uno a uno: un origen de SLUS reclamado por dos vtables de la A124 se queda con la de mas votos (si empatan,
    #    ninguna)
    por_orig = collections.defaultdict(list)
    for vt, (orig, v1, ws, ranuras, a0) in origen.items():
        por_orig[orig].append((v1, vt))
    descartadas = []
    for orig, lst in por_orig.items():
        if len(lst) > 1:
            lst.sort(reverse=True)
            fuera = lst[1:] if lst[0][0] > lst[1][0] else lst
            for v, vt in fuera:
                descartadas.append(vt)
                del origen[vt]
    # 3) consistencia global: cada funcion de la A124 tiene UNA funcion de SLUS y al reves en todas las vtables
    #    casadas. Una vtable casada con la hermana equivocada (nombres cruzados en SLUS entre clases hermanas: paso
    #    con EAXTruck/EAXCopCar) empareja una funcion heredada de la base con la redefinida de la hermana, y choca con
    #    la base y con las demas derivadas. Se quitan las vtables que quedan en minoria en algun choque.
    lectura = {}
    for vt, (orig, v1, ws, ranuras, a0) in list(origen.items()):
        try:
            wsl = struct.unpack('<%dI' % len(ws), isl.lee(orig, len(ws) * 4))
        except (ValueError, struct.error):
            del origen[vt]
            continue
        lectura[vt] = wsl
    while True:
        a_s = collections.defaultdict(collections.Counter)
        s_a = collections.defaultdict(collections.Counter)
        for vt, (orig, v1, ws, ranuras, a0) in origen.items():
            wsl = lectura[vt]
            for i, w, n in ranuras:
                a_s[w][wsl[i]] += 1
                s_a[wsl[i]][w] += 1
        malas = set()
        for vt, (orig, v1, ws, ranuras, a0) in origen.items():
            wsl = lectura[vt]
            for i, w, n in ranuras:
                for tabla, clave, valor in ((a_s, w, wsl[i]), (s_a, wsl[i], w)):
                    c = tabla[clave]
                    if len(c) > 1:
                        mejor, vm = c.most_common(1)[0]
                        empate = sum(1 for x in c.values() if x == vm) > 1
                        if empate or valor != mejor:
                            malas.add(vt)
        if not malas:
            break
        for vt in malas:
            descartadas.append(vt + '(choque)')
            del origen[vt]
    propuestas = collections.defaultdict(set)      # dir de SLUS -> {nombre}
    motivos = collections.defaultdict(list)
    informe = []
    for vt in sorted(origen):
        orig, v1, ws, ranuras, a0 = origen[vt]
        tam = len(ws) * 4
        try:
            wsl = struct.unpack('<%dI' % (tam // 4), isl.lee(orig, tam))
        except (ValueError, struct.error):
            continue
        # misma forma: los ceros en los mismos sitios
        if any((x == 0) != (y == 0) for x, y in zip(ws, wsl)):
            informe.append('FORMA %s: A124 0x%x, SLUS 0x%x: distinta (se salta)' % (vt, a0, orig))
            continue
        nuestra = dict(nuestras.get(vt, []))
        for i, w, n in ranuras:
            ws_ = wsl[i]
            actual = ps_dir.get(ws_, ['func_%08X' % ws_])
            nu = nuestra.get(i)
            if n in actual:
                continue
            informe.append('%-40s +0x%03x SLUS 0x%06x %-45s A124 %-45s nuestra %s' % (
                vt[:40], i * 4, ws_, ','.join(actual)[:45], n[:45], nu if nu else '-'))
            if nu == n:
                propuestas[ws_].add(n)
                motivos[ws_].append(vt)
    print('\n'.join(informe))
    print('vtables sin origen en SLUS: %d; descartadas por origen compartido: %s' % (sin_origen, ', '.join(descartadas)))
    buenas = {a: next(iter(ns)) for a, ns in propuestas.items() if len(ns) == 1}
    for a, ns in sorted(propuestas.items()):
        if len(ns) > 1:
            print('CONFLICTO 0x%x: %s' % (a, ', '.join(sorted(ns))))
    # un nombre para dos direcciones: fuera
    uso = collections.Counter(buenas.values())
    for a, n in sorted(buenas.items()):
        if uso[n] > 1:
            print('NOMBRE EN VARIAS 0x%x: %s' % (a, n))
    buenas = {a: n for a, n in buenas.items() if uso[n] == 1}
    print('propuestas (A124 y nuestra de acuerdo): %d' % len(buenas))
    if '--tsv' in sys.argv:
        with open(sys.argv[sys.argv.index('--tsv') + 1], 'w', encoding='utf-8', newline='\n') as fh:
            for a, n in sorted(buenas.items()):
                fh.write('0x%x\t%s\tvtable\t%s\n' % (a, n, motivos[a][0]))


if __name__ == '__main__':
    main()
