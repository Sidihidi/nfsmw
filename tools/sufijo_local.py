"""Resuelve los mapeos de objdiff cuyo nombre NUESTRO lleva un sufijo que pone el compilador.

GCC 2.95 nombra las clases locales de una funcion con un contador de etiquetas (`var_labelno`, gcc/varasm.c):
`Compare__...IconSortPCvT1.26770`. El numero cambia en cuanto cambia cualquier declaracion anterior de la unidad,
asi que un mapeo escrito a mano en OBJDIFF_SYMBOL_MAPPINGS de configure.py caduca en cada tanda (el de IconSort
caduco siete veces en una semana). Con un `*` al final del nombre nuestro, este modulo lo busca en el objeto recien
compilado y pone el numero que tenga ahora.

    "Compare__...IconSortPCvT1.35326": "Compare__...IconSortPCvT1.*"

Solo cuentan los simbolos DEFINIDOS del objeto (tabla de simbolos ELF o COFF): un nombre que solo aparece como
referencia no se empareja (paso con las clases locales de FEPlayerCarDB: nuestro objeto las llama pero no las emite).
Se resuelve solo si hay exactamente UN candidato; si no, el mapeo se deja y se avisa (la funcion saldra sin
emparejar, que es lo honrado).

Dos formas sin sufijo numerico final que la version 1 no contemplaba (r58, thunks de FEPlayerCarDB en zFe2):

  1. ee-gcc 2.95 de PS2 emite esos mismos nombres SIN el contador final
     (`Callback__...0_5FinesRC14FECareerRecord`, no `.35757`): un `X.*` o `X.NNNNN`
     cuyo candidato sea exactamente `X` se resuelve a `X`.
  2. Los mapeos de GameCube se cuelan en los objdiff.json de PS2 (tools/project.py
     conserva `symbol_mappings` entre configuraciones) y alla la clave `.35757` no
     existe en el objeto del objetivo: el mapeo no empareja NADA. En vez de avisar
     para siempre, se retira (y `automatico` lo vuelve a crear si vuelve a hacer
     falta). La purga va ANTES que la resolucion: al reves, la caida de (1) fijaba
     el valor al nombre PS2 `X` y esa basura volvia a GameCube (donde la clave vive
     y el valor ya no resolvia a nada) -- ocho Callback de zFe2 se quedaron al 0%
     asi en un report.json regenerado.
"""
import os
import re
import struct

# `Nombre.12345` o `Nombre$12345`: el sufijo numerico que pone el contador de etiquetas
_SUFIJO = re.compile(r'^(.*[.$])(\d{3,6})$')
_cache = {}


def definidos(ruta):
    """Nombres de los simbolos definidos (no las referencias) de un objeto ELF de 32 bits o COFF."""
    clave = (ruta, os.path.getmtime(ruta))
    if clave in _cache:
        return _cache[clave]
    d = open(ruta, 'rb').read()
    out = set()
    if d[:4] == b'\x7fELF':
        e = '<' if d[5] == 1 else '>'
        shoff, = struct.unpack_from(e + 'I', d, 0x20)
        shentsize, shnum = struct.unpack_from(e + 'HH', d, 0x2e)
        secs = [struct.unpack_from(e + 'IIIIIIIIII', d, shoff + i * shentsize) for i in range(shnum)]
        for s in secs:
            if s[1] != 2:  # SHT_SYMTAB
                continue
            strtab = secs[s[6]]
            for k in range(s[5] // 16):
                nombre, valor, tam, info, otro, shndx = struct.unpack_from(e + 'IIIBBH', d, s[4] + k * 16)
                if shndx == 0:
                    continue
                fin = d.index(b'\0', strtab[4] + nombre)
                out.add(d[strtab[4] + nombre:fin].decode('latin-1'))
    else:
        _, nsec, _, psym, nsym, _, _ = struct.unpack_from('<HHIIIHH', d, 0)
        cadenas = psym + nsym * 18
        i = 0
        while i < nsym:
            o = psym + i * 18
            nombre = d[o:o + 8]
            if nombre[:4] == b'\0\0\0\0':
                off, = struct.unpack_from('<I', nombre, 4)
                nombre = d[cadenas + off:d.index(b'\0', cadenas + off)]
            else:
                nombre = nombre.rstrip(b'\0')
            _, seccion, _, _, naux = struct.unpack_from('<IhHBB', d, o + 8)
            if seccion > 0:
                out.add(nombre.decode('latin-1'))
            i += 1 + naux
    _cache[clave] = out
    return out


def _por_prefijo(nombres):
    out = {}
    for n in nombres:
        m = _SUFIJO.match(n)
        if m:
            out.setdefault(m.group(1), set()).add(n)
    return out


def resolver(unidad, raiz='.', avisa=print):
    """Pone al dia los mapeos cuyo nombre nuestro lleva sufijo de compilador: los `*` y tambien los ya resueltos
    que el objeto recien compilado ya no define (el numero cambio). Los mapeos cuya clave no existe en el objeto
    del objetivo (restos de otra version) se retiran. Devuelve True si cambio algo."""
    mapeos = unidad.get('symbol_mappings') or {}
    base = unidad.get('base_path')
    ruta = os.path.join(raiz, base) if base else None
    if not mapeos or not ruta or not os.path.isfile(ruta):
        return False
    nuestros = None
    suyos_obj = None
    cambio = False
    for objetivo, valor in list(mapeos.items()):
        if not isinstance(valor, str):
            continue
        # Los restos de otra version se retiran ANTES de resolver: la clave lleva el contador de GameCube
        # (`X.35757`) y el objetivo de PS2 no la define. Con el orden viejo la caida a nombre sin sufijo
        # "arreglaba" el valor contra el objeto de PS2 (`X.35757 => X`) y esa basura volvia a GameCube
        # por el preserve de tools/project.py, donde la clave SI existe y el valor ya no casa con nada.
        if _SUFIJO.match(objetivo):
            if suyos_obj is None:
                suyos_obj = _definidos_objetivo(unidad, raiz)
            if suyos_obj and objetivo not in suyos_obj:
                del mapeos[objetivo]
                cambio = True
                continue
        if nuestros is None:
            nuestros = definidos(ruta)
        if valor in nuestros:
            continue
        if valor.endswith('*'):
            prefijo = valor[:-1]
        else:
            m = _SUFIJO.match(valor)
            # sin contador que quitar (`X.35757 => X` corrompido): los hermanos `X.NNNNN` del propio
            # nombre son el unico candidato posible
            prefijo = m.group(1) if m else valor + '.'
        hallados = sorted(n for n in nuestros if n.startswith(prefijo) and n[len(prefijo):].isdigit())
        if not hallados and prefijo[-1:] in '.$' and prefijo[:-1] in nuestros:
            hallados = [prefijo[:-1]]
        if len(hallados) == 1:
            if mapeos[objetivo] != hallados[0]:
                mapeos[objetivo] = hallados[0]
                cambio = True
            continue
        if not hallados and not valor.endswith('*'):
            # ni el valor ni su forma con/sin sufijo estan en nuestro objeto: objdiff
            # ignora ese mapeo (eathread lo prueba: 100 % por nombre con el mapeo
            # colgando). Se retira en silencio: los de `configure.py` se reanaden
            # solos en cada configuracion, y `automatico` rehace los suyos.
            del mapeos[objetivo]
            cambio = True
            continue
        avisa('sufijo_local: %s: %d simbolos definidos casan con %s* en %s; el mapeo se queda como esta'
              % (unidad.get('name'), len(hallados), prefijo, base))
    return cambio


def _definidos_objetivo(unidad, raiz):
    """Simbolos definidos del objeto del OBJETIVO, o set vacio si no se pudo leer (nunca se retira nada a ciegas)."""
    t = unidad.get('target_path')
    rt = os.path.join(raiz, t) if t else None
    if not rt or not os.path.isfile(rt):
        return set()
    try:
        return definidos(rt)
    except Exception:
        return set()


def emparejar(unidad, raiz='.', avisa=print):
    """resolver + automatico sobre una unidad: la pasada completa que necesita objdiff.json.

    configure.py (apply_objdiff_symbol_mappings) solo llama a `resolver` y solo para las unidades con mapeos
    escritos a mano, y `automatico` solo lo llamaba scripts/pctall.py: los 16 thunks de FEPlayerCarDB de zFe2
    (clases locales de VehicleDB.cpp) se quedaban al 0 % en cada report.json regenerado hasta que alguien
    volvia a correr pctall. tools/project.py llama a esto al escribir objdiff.json, que es donde nacen los
    mapeos de TODAS las versiones."""
    cambio = resolver(unidad, raiz, avisa)
    automatico(unidad, raiz)
    return cambio


def automatico(unidad, raiz='.'):
    """Empareja las clases locales del objetivo con las nuestras aunque el sufijo no sea el mismo: mismo nombre salvo
    el numero final, solo simbolos DEFINIDOS y un solo candidato en cada lado. Solo empareja nombres; objdiff sigue
    comparando el codigo. Devuelve el numero de mapeos anadidos."""
    t, b = unidad.get('target_path'), unidad.get('base_path')
    if not t or not b:
        return 0
    rt, rb = os.path.join(raiz, t), os.path.join(raiz, b)
    if not os.path.isfile(rt) or not os.path.isfile(rb):
        return 0
    suyos_def = definidos(rb)
    objetivo = _por_prefijo(n for n in definidos(rt) if '.0_' in n or '$0_' in n)
    nuestro = _por_prefijo(n for n in suyos_def if '.0_' in n or '$0_' in n)
    mapeos = unidad.get('symbol_mappings') or {}
    nuevos = 0
    for prefijo, nombres in objetivo.items():
        suyos = nuestro.get(prefijo)
        if len(nombres) != 1 or not suyos or len(suyos) != 1:
            continue
        a, n = next(iter(nombres)), next(iter(suyos))
        if a == n or a in mapeos or a in suyos_def:
            continue
        mapeos[a] = n
        nuevos += 1
    # Los constructores y destructores de estaticos del middleware de PS2 (GCC 2.95.3): `_GLOBAL_$I$` + la ruta del
    # fichero EN EL DISCO DE QUIEN COMPILO + seis letras aleatorias (memoria nfsmw-global-sufijo-aleatorio.md). Ni la
    # ruta ni el sufijo pueden coincidir: se emparejan por unidad y por tipo ($I$ o $D$) cuando hay uno en cada lado.
    t_glob = _globales(definidos(rt))
    n_glob = _globales(suyos_def)
    for tipo, suyos in t_glob.items():
        mios = n_glob.get(tipo)
        if len(suyos) != 1 or not mios or len(mios) != 1:
            continue
        a, n = next(iter(suyos)), next(iter(mios))
        if a == n or a in mapeos or a in suyos_def:
            continue
        mapeos[a] = n
        nuevos += 1
    if nuevos:
        unidad['symbol_mappings'] = mapeos
    return nuevos


# append_random_chars de GCC 2.95 elige los seis caracteres entre letras Y cifras (c3Njaaa, d0kaaa...)
_GLOBAL = re.compile(r'^_GLOBAL_\$([ID])\$.*\.(?:cpp|cc|c)[A-Za-z0-9]{6}$')


def _globales(nombres):
    out = {}
    for n in nombres:
        m = _GLOBAL.match(n)
        if m:
            out.setdefault(m.group(1), set()).add(n)
    return out
