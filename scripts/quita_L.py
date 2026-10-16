#!/usr/bin/env python3
"""quita_L.py -- PC: quita de un objeto COFF de x86 las etiquetas locales `$L<n>` que deja MSVC 7.1, antes de medir.

Por que: objdiff no sabe el tamano de un simbolo COFF y lo mide hasta el SIGUIENTE simbolo de la seccion. MSVC 7.1
deja en la tabla de simbolos las etiquetas internas `$L<n>` (destinos de tablas de saltos y la del int3 que va detras
de una llamada que no vuelve), y en una funcion ESTATICA (sin COMDAT propio) objdiff la corta en la primera etiqueta:
compara 18 bytes de `_luaD_growCI` en vez de 56. El exe troceado no tiene esas etiquetas. Ronda 14: es el unico caso de
todo PC (22 funciones estaticas con `$L` dentro, todas en zLua, que es C); 8 de ellas tienen el codigo igual.

El objeto que sale es EQUIVALENTE al de entrada (el enlazador produciria los mismos bytes):
  - cada reubicacion que apuntaba a una etiqueta pasa al simbolo de su SECCION, con el desplazamiento de la etiqueta
    sumado al campo (DIR32, DIR32NB, SECREL y REL32 guardan el sumando en el propio campo; SECTION no lleva
    desplazamiento);
  - se reindexan las reubicaciones de todas las secciones y los indices de simbolo de los registros auxiliares
    (definicion de funcion, .bf y externos debiles).
Si aparece algo que no sabe tratar (otro tipo de reubicacion hacia una etiqueta, una etiqueta con registros
auxiliares, la tabla de simbolos antes de los datos), no toca el objeto y lo dice.

Y los funclets de EH (ronda 50, PC50): las `$L<n>` de las secciones .text$x NO se quitan, se renombran. Son el codigo de
desenrollado de cada funcion con destructores (`lea ecx,[ebp-x]; jmp ??1Clase`), y el exe los tiene: 3.883 colocados
en config/SPEED_EXE_1_3/symbols.json con el nombre POSICIONAL `Unwind@<funcion>$<i>` (i-esimo funclet de la .text$x de
esa funcion, la que acaba en `__ehhandler$<funcion>`). El numero de `$L` cambia con cualquier cabecera; la posicion no.
nombrar_funclets() les pone ese nombre en la copia y mapeos_funclets() da los mapeos de objdiff (`.text` contra `.text$x`).

No toca objdiff ni el build: `pctall.py` lo llama para SPEED_EXE_1_3 (para_medir: funclets con nombre y sin etiquetas que
corten funciones estaticas) y escribe la copia en build/SPEED_EXE_1_3/sin_L/, que es la que mide.

    python scripts/quita_L.py ENTRADA.obj SALIDA.obj      # escribe la copia para medir (funclets y etiquetas)
    python scripts/quita_L.py --ver ENTRADA.obj           # cuenta etiquetas y funciones estaticas afectadas
"""
import re
import struct
import sys

I386_DIR32, I386_DIR32NB, I386_SECTION, I386_SECREL, I386_REL32 = 0x06, 0x07, 0x0A, 0x0B, 0x14
CON_SUMANDO = (I386_DIR32, I386_DIR32NB, I386_SECREL, I386_REL32)
# PC53 (ronda 53): tambien las etiquetas de C (`goto nextloop;` deja `$nextloop$112236`): cortaban igual las
# estaticas (HUFF_decompress de zMisc, 3.195 B identicos, salia al 64 %; 16 estaticas de spch, vp6 y zMisc).
ETIQUETA = re.compile(rb'^\$(L\d+|[A-Za-z_]\w*\$\d+)$')


class NoSe(Exception):
    pass


def _nombre(d, o, cad):
    n = d[o:o + 8]
    if n[:4] == b'\0\0\0\0':
        off, = struct.unpack_from('<I', n, 4)
        return d[cad + off:d.index(b'\0', cad + off)]
    return n.rstrip(b'\0')


def leer(d):
    maquina, nsec, _, psym, nsym, opt, _ = struct.unpack_from('<HHIIIHH', d, 0)
    if maquina != 0x014C:
        raise NoSe('no es x86')
    secs = []
    for i in range(nsec):
        o = 20 + opt + i * 40
        _, _, _, rsz, praw, prel, _, nrel, _, car = struct.unpack_from('<8sIIIIIIHHI', d, o)
        primero = 0
        if nrel == 0xFFFF and car & 0x01000000:  # IMAGE_SCN_LNK_NRELOC_OVFL: la primera entrada lleva la cuenta
            nrel, = struct.unpack_from('<I', d, prel)
            primero = 1
        secs.append({'cab': o, 'rsz': rsz, 'praw': praw, 'prel': prel, 'nrel': nrel, 'primero': primero, 'car': car})
    cad = psym + nsym * 18
    sims = []  # (indice, nombre, valor, seccion, tipo, clase, naux)
    i = 0
    while i < nsym:
        o = psym + i * 18
        valor, sec, tipo, clase, naux = struct.unpack_from('<IhHBB', d, o + 8)
        sims.append((i, _nombre(d, o, cad), valor, sec, tipo, clase, naux))
        i += 1 + naux
    return secs, psym, nsym, sims


def etiquetas_en_estaticas(d):
    """Cuantas etiquetas $L caen DENTRO de una funcion estatica (las que cortan la funcion en objdiff)."""
    secs, psym, nsym, sims = leer(d)
    por_sec = {}
    for i, n, v, sec, tipo, clase, naux in sims:
        if sec > 0 and not n.startswith(b'.'):
            por_sec.setdefault(sec, []).append((v, 0 if ETIQUETA.match(n) else 1, n, clase, tipo))
    n = 0
    fs = set()
    for sec, l in por_sec.items():
        l.sort()
        dueno = None  # la funcion en la que cae cada etiqueta: la ultima que empieza antes
        for v, es_fn, nm, cl, t in l:
            if es_fn:
                dueno = (nm, cl) if (t >> 4) == 2 else None
            elif dueno and dueno[1] == 3 and v > 0:
                n += 1
                fs.add(dueno[0])
    return n, sorted(x.decode('latin-1') for x in fs)


def quitar(d):
    d = bytearray(d)
    secs, psym, nsym, sims = leer(d)
    for s in secs:
        if s['praw'] and s['praw'] + s['rsz'] > psym or s['nrel'] and s['prel'] + 10 * (s['nrel'] + s['primero']) > psym:
            raise NoSe('la tabla de simbolos no esta al final')
    # simbolo de seccion: el primero con el nombre de la seccion, clase STATIC, valor 0 y registro auxiliar
    sim_seccion = {}
    for i, n, v, sec, tipo, clase, naux in sims:
        if sec > 0 and clase == 3 and v == 0 and naux >= 1 and n.startswith(b'.') and sec not in sim_seccion:
            sim_seccion[sec] = i
    quitar_idx = {}
    for i, n, v, sec, tipo, clase, naux in sims:
        if ETIQUETA.match(n) and clase == 3 and sec > 0:
            if naux:
                raise NoSe('etiqueta %s con registros auxiliares' % n.decode())
            if sec not in sim_seccion:
                raise NoSe('seccion %d sin simbolo de seccion' % sec)
            quitar_idx[i] = (sec, v)
    if not quitar_idx:
        return None
    # nuevo indice de cada simbolo que se queda
    nuevo = {}
    k = 0
    for i, n, v, sec, tipo, clase, naux in sims:
        if i in quitar_idx:
            continue
        for a in range(1 + naux):
            nuevo[i + a] = k + a
        k += 1 + naux
    total = k
    # reubicaciones: redirigir las de etiquetas y reindexar todas
    for s in secs:
        for r in range(s['nrel']):
            o = s['prel'] + 10 * (r + s['primero'])
            off, idx, tipo = struct.unpack_from('<IIH', d, o)
            if idx in quitar_idx:
                sec, v = quitar_idx[idx]
                if tipo in CON_SUMANDO:
                    p = s['praw'] + off
                    viejo, = struct.unpack_from('<i', d, p)
                    struct.pack_into('<i', d, p, viejo + v)
                elif tipo != I386_SECTION:
                    raise NoSe('reubicacion de tipo 0x%X hacia una etiqueta' % tipo)
                idx = sim_seccion[sec]
            struct.pack_into('<I', d, o + 4, nuevo[idx])
    # registros auxiliares con indices de simbolo
    for i, n, v, sec, tipo, clase, naux in sims:
        if not naux:
            continue
        a = psym + (i + 1) * 18
        campos = []
        if clase == 105:                                   # externo debil: TagIndex
            campos = [0]
        elif (tipo >> 4) == 2 and clase in (2, 3) and sec > 0:  # definicion de funcion: TagIndex y siguiente funcion
            campos = [0, 12]
        elif clase == 101 and n == b'.bf':                  # .bf: siguiente funcion
            campos = [12]
        for c in campos:
            x, = struct.unpack_from('<I', d, a + c)
            if x:
                if x in quitar_idx:
                    raise NoSe('un registro auxiliar apunta a una etiqueta')
                struct.pack_into('<I', d, a + c, nuevo.get(x, 0))
    # la tabla nueva: los simbolos que se quedan, con sus auxiliares, y la tabla de cadenas detras
    tabla = bytearray()
    for i, n, v, sec, tipo, clase, naux in sims:
        if i in quitar_idx:
            continue
        tabla += d[psym + i * 18:psym + (i + 1 + naux) * 18]
    cadenas = d[psym + nsym * 18:]
    struct.pack_into('<I', d, 12, total)
    return bytes(d[:psym] + tabla + cadenas), len(quitar_idx)


FUNCLET = 'Unwind@%s$%d'


def _nombre_seccion(d, cab, cad):
    n = d[cab:cab + 8].rstrip(b'\0')
    if n.startswith(b'/') and n[1:].isdigit():  # nombre largo: desplazamiento en la tabla de cadenas
        off = int(n[1:])
        return d[cad + off:d.index(b'\0', cad + off)]
    return n


def funclets(d):
    """Los funclets de desenrollado de EH (`$L<n>` de las secciones .text$x) con su nombre POSICIONAL:
    [(indice del simbolo, nombre viejo, nombre nuevo)]. Cada .text$x de MSVC 7.1 es la de UNA funcion F: sus funclets
    en orden y `__ehhandler$F` al final; el i-esimo funclet se llama `Unwind@F$i`. El numero de `$L` es un contador
    de la unidad que cambia con cualquier cabecera; la posicion no. El nombre: el prefijo `Unwind@` de IDA (el que ya
    tenian en symbols.json, `Unwind@<direccion>`); el de MSVC 8, `__unwindfunclet$F$i`, no vale: objdiff 3.8 esconde
    los simbolos que empiezan por `__unwind` y `__catch` y no entrarian en el informe (ni en el total)."""
    secs, psym, nsym, sims = leer(d)
    cad = psym + nsym * 18
    por_sec = {}
    for k, s in enumerate(secs, 1):
        if _nombre_seccion(d, s['cab'], cad) == b'.text$x':
            por_sec[k] = []
    for i, n, v, sec, tipo, clase, naux in sims:
        if sec in por_sec and not n.startswith(b'.'):
            por_sec[sec].append((v, i, n, clase))
    out = []
    for sec, l in por_sec.items():
        l.sort()
        if not l or not l[-1][2].startswith(b'__ehhandler$'):
            continue
        f = l[-1][2][len(b'__ehhandler$'):].decode('latin-1')
        k = 0
        for v, i, n, clase in l[:-1]:
            if ETIQUETA.match(n) and clase == 3:
                out.append((i, n, (FUNCLET % (f, k)).encode('latin-1')))
                k += 1
    return out


def nombrar_funclets(d):
    """Copia del objeto con los funclets de EH renombrados (funclets()) y con tipo de funcion (0x20). Solo cambian
    los nombres: las reubicaciones van por indice de simbolo. Los nombres nuevos se anaden al final de la tabla de
    cadenas. Devuelve (bytes, n) o None si no hay funclets."""
    lista = funclets(d)
    if not lista:
        return None
    d = bytearray(d)
    secs, psym, nsym, sims = leer(d)
    for s in secs:
        if s['praw'] and s['praw'] + s['rsz'] > psym or s['nrel'] and s['prel'] + 10 * (s['nrel'] + s['primero']) > psym:
            raise NoSe('la tabla de simbolos no esta al final')
    cad = psym + nsym * 18
    tam, = struct.unpack_from('<I', d, cad)
    if cad + tam != len(d):
        raise NoSe('hay datos detras de la tabla de cadenas')
    extra = bytearray()
    for i, viejo, nuevo in lista:
        o = psym + i * 18
        struct.pack_into('<II', d, o, 0, tam + len(extra))
        struct.pack_into('<H', d, o + 14, 0x20)
        extra += nuevo + b'\0'
    struct.pack_into('<I', d, cad, tam + len(extra))
    return bytes(d) + bytes(extra), len(lista)


def para_medir(d):
    """El objeto que se mide en PC: funclets de EH con nombre posicional y sin las etiquetas `$L` que cortan las
    funciones estaticas. Devuelve (bytes, descripcion) o None si no hay nada que cambiar."""
    hecho = []
    r = nombrar_funclets(d)
    if r:
        d = r[0]
        hecho.append('%d funclets' % r[1])
    if etiquetas_en_estaticas(d)[0]:
        r = quitar(d)
        if r:
            d = r[0]
            hecho.append('%d etiquetas quitadas' % r[1])
    return (d, ', '.join(hecho)) if hecho else None


MAPEADOS_PC = re.compile(r'^(_\$E\d+|__ehhandler\$.+|Unwind@\?.*\$\d+)$')


def mapeos_funclets(raiz):
    """{unidad de objdiff: {nombre en el objetivo: nombre nuestro}} de los nombres de PC que viven en otra seccion que
    la nuestra, de config/SPEED_EXE_1_3/symbols.json, por el rango de splits.json en que caen: los funclets de EH con
    nombre posicional (`Unwind@F$i`, nuestro .text$x), los `__ehhandler$` (.text$x) y los `_$E<n>` (.text$yc/$yd).
    Hace falta el mapeo explicito: objdiff solo empareja por nombre dentro de la MISMA seccion (`.text` del exe troceado
    frente a la nuestra). Un nombre repetido lleva en el troceado el sufijo `__delink_<DIR>` desde la segunda vez.
    configure.py ya pone los de `_$E` y `__ehhandler$` (objdiff_pc_symbol_mappings), pero tools/sufijo_local.py los
    resuelve contra el objeto que haya ANTES de compilar: un `_$E<n>` renumerado (scripts/yc_renumera.py) o un funclet
    (que el objeto del build lleva como `$L`) no estan en el y se retiran. pctall.py los vuelve a poner DESPUES de
    compilar y de nombrar_funclets()."""
    import json
    import os
    cfg = os.path.join(raiz, 'config', 'SPEED_EXE_1_3')
    with open(os.path.join(cfg, 'symbols.json'), encoding='utf-8') as f:
        funcs = json.load(f)['functions']
    with open(os.path.join(cfg, 'splits.json'), encoding='utf-8') as f:
        splits = json.load(f)
    rangos = sorted((lo, hi, obj) for obj, v in splits.items() for lo, hi in v.get('function_ranges') or [])
    out = {}
    vistos = set()
    for f in sorted(funcs, key=lambda f: f['start']):
        n = f['name']
        repetido = n in vistos
        vistos.add(n)
        if not MAPEADOS_PC.match(n):
            continue
        for lo, hi, obj in rangos:
            if lo <= f['start'] < hi:
                if obj.endswith('.obj'):
                    clave = '%s__delink_%08X' % (n, f['start']) if repetido else n
                    out.setdefault('main/' + obj[:-len('.obj')], {})[clave] = n
                break
    return out


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == '--ver':
        for p in sys.argv[2:]:
            n, fs = etiquetas_en_estaticas(open(p, 'rb').read())
            print('%s: %d etiquetas dentro de %d funciones estaticas' % (p, n, len(fs)))
            for f in fs:
                print('   ', f)
        return 0
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    try:
        r = para_medir(open(sys.argv[1], 'rb').read())
    except NoSe as e:
        print('no se toca %s: %s' % (sys.argv[1], e))
        return 1
    if r is None:
        print('%s: sin etiquetas ni funclets' % sys.argv[1])
        return 0
    open(sys.argv[2], 'wb').write(r[0])
    print('%s: %s' % (sys.argv[2], r[1]))
    return 0


if __name__ == '__main__':
    sys.exit(main())
