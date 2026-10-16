#!/usr/bin/env python3
"""mdebug_lineas.py -- EL MAPA FDR/PDR/$LM del .mdebug del ELF de PS2 (SLES-53558-A124).

Para cada funcion del juego: la lista (direccion -> (fichero de fuente, linea)) que
el .mdebug lleva. Es el oraculo de lineas absolutas por instruccion de la ronda 27:
cierra la vigilancia de los nop "sueltos" (forense del asm, seccion 3.1) juzgando
cada nop por evidencia: un nop con cambio de linea EXACTAMENTE en su direccion es
una sentencia del fuente original; un nop que hereda la linea de la instruccion
anterior es relleno del compilador.

FORMATO (calibrado esta ronda contra include/coff/sym.h de binutils 2.15 y contra
el escritor gas/ecoff.c:ecoff_build_lineno; verificado con una sonda compilada con
el ee-gcc 2.9-991111 real del arbol y contra las cadenas de los 559 FDR):

  HDRR  (0x60 B)   cbLineOffset/cbLine, cbPdOffset/ipdMax, cbSymOffset/isymMax,
                   cbSsOffset/issMax, cbFdOffset/ifdMax...
  FDR   (72 B)     adr, rss, issBase, cbSs, isymBase, csym, ilineBase, cline,
                   ioptBase, copt, ipdFirst(u16), cpd(u16), iauxBase, caux,
                   rfdBase, crfd, flags, cbLineOffset, cbLine
                   (18 x u32; el orden es el canonnico ecoff32, NO el del doc
                   anterior: lo que alli se leia como issLim/cbLineOffset eran
                   cbSs/cbLineOffset mal alineados)
  PDR   (52 B)     adr, isym, iline, regmask, regoffset, iopt, fregmask,
                   fregoffset, frameoffset, framereg(u16), pcreg(u16),
                   lnLow, lnHigh, cbLineOffset
                   -- adr es RELATIVO al adr del FDR (el absoluto suma FDR.adr)
                   -- los PDR del codigo MW llevan lnLow=lnHigh=-1: sin lineas;
                      SOLO los 4 ficheros GNU .s (crt0.s, snd smtflt, klib.s,
                      tlbtrap.s) tienen tabla delta (1112 B en total)
  SYMR  (12 B)     iss, value, st(6b), sc(5b), index(18b = bits>=14)
  $LM   (st=5)     value = direccion ABSOLUTA, index = LINEA de fuente
  FILE  (st=5)     nombre con ruta: cambia el fichero de contexto de los $LM que
                   siguen (el flujo de simbolos va por trozos de fichero, con el
                   orden de expansion de los inline)
  delta (GNU .s)   nibble bajo = (n de instrucciones de la entrada)-1, nibble alto
                   = delta de linea: 1..7 suma, 9..15 resta (d-16), 8 = forma de
                   3 bytes con int16 big-endian a continuacion. Linea inicial =
                   PDR.lnLow del procedimiento.

USO:
    python scripts/mdebug_lineas.py fn <patron>        # marcadores y tabla de una funcion
    python scripts/mdebug_lineas.py addr 0xDIR         # linea efectiva de una direccion
    python scripts/mdebug_lineas.py nop 0xDIR [...]    # veredicto de nop(s)
    python scripts/mdebug_lineas.py mapa --json build/mdebug_lineas.json
    python scripts/mdebug_lineas.py cobertura          # estadisticas por region
    python scripts/mdebug_lineas.py fdr [patron]       # los 559 ficheros objeto
    python scripts/mdebug_lineas.py test               # autotest (ver abajo)

AUTOTEST (`test`): cadenas FDR exactas contra isymMax/ipdMax/iauxMax/ilineMax,
delta del crt0 dentro de [lnLow,lnHigh], marcadores de bTransposeMatrix y de las
funciones de una sola sentencia asm, control negativo del modulo .s sin stabs, y
consistencia linea-efectiva. Todo contra el ELF; si falta orig/, salta con aviso.
"""
import bisect
import json
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
ELF = os.path.join(ROOT, "orig", "SLES-53558-A124", "NFS.ELF")

EXT_SRC = ('.cpp', '.hpp', '.h', '.c', '.inl', '.s', '.ii', '.cxx', '.inc')

_cache = {}


# ---------------------------------------------------------------- tablas base

def _secciones():
    """Secciones con codigo del ELF: [(nombre, vaddr, offset_fichero, tamano)]."""
    with open(ELF, 'rb') as f:
        e = f.read(0x34)
        shoff = struct.unpack_from('<I', e, 0x20)[0]
        shentsize, shnum, shstrndx = struct.unpack_from('<HHH', e, 0x2E)
        f.seek(shoff)
        sh = f.read(shentsize * shnum)
        # s = (nombre, tipo, flags, addr, offset, size)
        secs = [struct.unpack_from('<IIIIIIIIII', sh, i * shentsize) for i in range(shnum)]
        f.seek(secs[shstrndx][4])
        nombres = f.read(secs[shstrndx][5])
        out = []
        for s in secs:
            nombre = nombres[s[0]:nombres.index(b'\0', s[0])].decode('latin-1')
            if s[5] and (s[2] & 0x4 or nombre == '.vutext'):
                out.append((nombre, s[3], s[4], s[5]))
    return out


def _hdrr():
    with open(ELF, 'rb') as f:
        e = f.read(0x34)
        shoff = struct.unpack_from('<I', e, 0x20)[0]
        shentsize, shnum, shstrndx = struct.unpack_from('<HHH', e, 0x2E)
        f.seek(shoff)
        sh = f.read(shentsize * shnum)
        secs = [struct.unpack_from('<IIIIIIIIII', sh, i * shentsize) for i in range(shnum)]
        f.seek(secs[shstrndx][4])
        nombres = f.read(secs[shstrndx][5])
        for s in secs:
            if nombres[s[0]:nombres.index(b'\0', s[0])] == b'.mdebug':
                f.seek(s[4])
                h = struct.unpack('<HH23I', f.read(0x60))
                return dict(ilineMax=h[2], cbLine=h[3], cbLineOffset=h[4],
                            ipdMax=h[7], cbPdOffset=h[8], isymMax=h[9],
                            cbSymOffset=h[10], iauxMax=h[13], issMax=h[15],
                            cbSsOffset=h[16], ifdMax=h[19], cbFdOffset=h[20])
    raise SystemExit('%s no tiene .mdebug' % ELF)


def carga():
    """Lee HDRR+FDR+PDR+SYMR+ss y construye el mapa de marcadores $LM/FILE."""
    if _cache:
        return
    if not os.path.exists(ELF):
        raise SystemExit('falta %s (extrae la A124 primero)' % ELF)
    H = _hdrr()
    f = open(ELF, 'rb')

    # FDR: 72 B, 18 x u32, orden canonico ecoff32 (include/coff/sym.h)
    f.seek(H['cbFdOffset'])
    fraw = f.read(H['ifdMax'] * 72)
    fdrs = []
    for i in range(H['ifdMax']):
        v = struct.unpack_from('<18I', fraw, i * 72)
        fdrs.append(dict(adr=v[0], rss=v[1], issBase=v[2], cbSs=v[3],
                         isymBase=v[4], csym=v[5], ilineBase=v[6], cline=v[7],
                         ipdFirst=v[10] & 0xFFFF, cpd=v[10] >> 16,
                         iauxBase=v[11], caux=v[12], rfdBase=v[13], crfd=v[14],
                         flags=v[15], cbLineOffset=v[16], cbLine=v[17]))
    del fraw

    # PDR: 52 B canonicos; adr relativo al FDR
    f.seek(H['cbPdOffset'])
    praw = f.read(H['ipdMax'] * 52)
    pdrs = []
    for i in range(H['ipdMax']):
        o = i * 52
        adr, isym, iline, rm, ro, iopt, fm, fo, foff, fr, pc, \
            lnLow, lnHigh, cbl = struct.unpack('<IiiiiiiiihhiiI'.replace(' ', ''), praw[o:o + 52])
        pdrs.append(dict(adr=adr, isym=isym, iline=iline, lnLow=lnLow,
                         lnHigh=lnHigh, cbLineOffset=cbl, frameoffset=foff))
    del praw

    # ss y SYMR
    f.seek(H['cbSsOffset'])
    ss = f.read(H['issMax'])
    f.seek(H['cbSymOffset'])
    sraw = f.read(H['isymMax'] * 12)
    f.close()

    syms = []
    for i in range(H['isymMax']):
        o = i * 12
        iss, value, bits = struct.unpack_from('<III', sraw, o)
        syms.append((iss, value, bits & 0x3F, (bits >> 6) & 0x1F, bits >> 14))
    del sraw

    def cadena(iss):
        e = ss.find(b'\0', iss)
        return ss[iss:e if e >= 0 else len(ss)].decode('latin-1')

    # ---- el mapa: recorrido del flujo de simbolos
    # marcadores $LM -> (dir_absoluta, ifichero, linea); FILE cambia el contexto.
    ficheros = []            # lista de rutas unicas (orden de aparicion)
    idx_fich = {}
    marcadores = []          # (dir, ifich, linea) EN ORDEN DE FLUJO
    funcs_stabs = []         # (dir, nombre, ifichero_de_contexto)
    cur_file = -1
    es_fuente = lambda n: ('/' in n or '\\' in n) and not n.startswith('$')
    for iss, value, st, sc, index in syms:
        if st == 5:
            n = cadena(iss)
            if n.startswith('$LM'):
                # index = linea de fuente; value = direccion. Los value=-1 son
                # expansiones inline sin codigo (71.754, todas en cabeceras):
                # ruido del linker, fuera del mapa.
                if index and value != 0xFFFFFFFF:
                    marcadores.append((value, cur_file, index))
                continue
            if es_fuente(n) or n.endswith(EXT_SRC):
                if n not in idx_fich:
                    idx_fich[n] = len(ficheros)
                    ficheros.append(n)
                cur_file = idx_fich[n]
                continue
        elif st in (6, 14) and value != 0xFFFFFFFF:
            funcs_stabs.append((value, cadena(iss), cur_file))

    # ---- esqueleto de funciones: .symtab del ELF (con tamanos) + stProc de stabs
    # (el symtab cubre TODO, incluidos los .s sin stabs; los stProc aportan el
    #  nombre cfront del MW y el fichero de contexto. El superbenderasm.obj de
    #  0x47cbe0 no tiene ninguno de los dos: es un hueco entre funciones.)
    f = open(ELF, 'rb')
    e = f.read(0x34)
    shoff = struct.unpack_from('<I', e, 0x20)[0]
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', e, 0x2E)
    f.seek(shoff)
    sh = f.read(shentsize * shnum)
    secs_sh = [struct.unpack_from('<IIIIIIIIII', sh, i * shentsize) for i in range(shnum)]
    f.seek(secs_sh[shstrndx][4])
    shnombres = f.read(secs_sh[shstrndx][5])
    symtab = strtab = None
    for s in secs_sh:
        nom = shnombres[s[0]:shnombres.index(b'\0', s[0])].decode('latin-1')
        if nom == '.symtab':
            symtab = (s[4], s[5])
        elif nom == '.strtab':
            strtab = (s[4], s[5])
    f.seek(strtab[0])
    strs = f.read(strtab[1])
    f.seek(symtab[0])
    symraw = f.read(symtab[1])
    f.close()
    base = {}
    for i in range(symtab[1] // 16):
        o = i * 16
        nom, val, size, info, other, shndx = struct.unpack_from('<IIIBBH', symraw, o)
        if (info & 0xF) == 2 and val and size:
            e2 = strs.find(b'\0', nom)
            base[val] = dict(addr=val, fin=val + size,
                             nombre=strs[nom:e2].decode('latin-1'), ifich=-1)

    def region(addr):
        for nombre, va, off, size in _secciones():
            if va <= addr < va + size:
                return (nombre, va, va + size)
        return (None, None, None)

    for addr, nom, ifi in funcs_stabs:
        b = base.get(addr)
        if b is None:
            base[addr] = dict(addr=addr, fin=None, nombre=nom, ifich=ifi)
        else:
            b['nombre_stabs'] = nom
            b['ifich'] = ifi
    funciones = sorted(base.values(), key=lambda d: d['addr'])
    for i, fu in enumerate(funciones):
        if fu['fin'] is None:
            fin = funciones[i + 1]['addr'] if i + 1 < len(funciones) else 0xFFFFFFFF
            _, r0, r1 = region(fu['addr'])
            if r1 is not None:
                fin = min(fin, r1)
            fu['fin'] = fin
        fu.setdefault('nombre_stabs', '')

    _cache.update(H=H, fdrs=fdrs, pdrs=pdrs, syms=syms, ss=ss, cadena=cadena,
                  ficheros=ficheros, marcadores=marcadores, funciones=funciones,
                  secs=_secciones())
    _indices()


def _indices():
    """Indice dir->funcion y, por funcion, sus marcadores (orden de flujo)."""
    funciones = _cache['funciones']
    marcadores = _cache['marcadores']
    dirs = [f['addr'] for f in funciones]
    por_dir = {f['addr']: i for i, f in enumerate(funciones)}
    por_nombre = {}
    for i, f in enumerate(funciones):
        por_nombre.setdefault(f['nombre'], i)
        if f.get('nombre_stabs'):
            por_nombre.setdefault(f['nombre_stabs'], i)
    # marcadores ordenados por direccion (estable: conserva el orden de flujo en empates)
    orden = sorted(range(len(marcadores)), key=lambda i: (marcadores[i][0], i))
    m_orden = [marcadores[i] for i in orden]
    m_dirs = [m[0] for m in m_orden]
    # limites de funcion sobre la lista ordenada
    m_ini = []
    j = 0
    for f in funciones:
        while j < len(m_dirs) and m_dirs[j] < f['addr']:
            j += 1
        m_ini.append(j)
    _cache.update(dirs=dirs, por_dir=por_dir, por_nombre=por_nombre,
                  m_orden=m_orden, m_dirs=m_dirs, m_ini=m_ini)


# ---------------------------------------------------------------- consultas

def funcion_de(addr):
    """La funcion que contiene addr (o None)."""
    carga()
    dirs = _cache['dirs']
    i = bisect.bisect_right(dirs, addr) - 1
    if i >= 0 and addr < _cache['funciones'][i]['fin']:
        return i
    return None


def marcadores_de_ifunc(i):
    carga()
    f = _cache['funciones'][i]
    j = _cache['m_ini'][i]
    fin = _cache['m_ini'][i + 1] if i + 1 < len(_cache['funciones']) else len(_cache['m_dirs'])
    out = []
    while j < len(_cache['m_dirs']) and _cache['m_dirs'][j] < f['fin']:
        out.append(_cache['m_orden'][j])
        j += 1
    return out


def linea_en(addr):
    """(ifich, linea, exacto): ultimo marcador <= addr; exacto = hay marcador
    en esa direccion (la linea empieza ahi)."""
    carga()
    j = bisect.bisect_right(_cache['m_dirs'], addr) - 1
    if j < 0:
        return None
    a, ifi, ln = _cache['m_orden'][j]
    return ifi, ln, (a == addr)


def veredicto_nop(addr):
    """(linea_propia, linea, linea_previa): True si hay cambio de linea que
    EMPIEZA exactamente en addr (sentencia propia del fuente); False si hereda
    la linea de la instruccion anterior (relleno del compilador)."""
    carga()
    m = _cache['m_orden']
    d = _cache['m_dirs']
    j = bisect.bisect_right(d, addr) - 1
    # linea efectiva en addr: ultimo marcador <= addr
    if j < 0:
        return None
    en_addr = None
    k = j
    while k >= 0 and d[k] == d[j]:
        k -= 1
    # m[j] ya es el ultimo (orden de flujo estable) con dir <= addr
    linea = m[j][2]
    linea_prev = None
    if d[j] == addr:
        en_addr = True
        # linea previa = ultimo marcador con dir < addr
        if k >= 0:
            linea_prev = m[k][2]
    else:
        en_addr = False
    propia = en_addr and (linea_prev is None or linea_prev != linea)
    return propia, linea, linea_prev


# ------------------------------------------------- parte GNU (tablas delta)

def decodifica_delta(ifdr, num_pdr=None):
    """Decodifica la tabla delta de los FDR con cbLine>0 (los .s de GNU).
    Devuelve [(dir_absoluta, linea)] de todos sus PDR (o solo del PDR pedido)."""
    carga()
    fdr = _cache['fdrs'][ifdr]
    H = _cache['H']
    if not fdr['cbLine']:
        return []
    with open(ELF, 'rb') as f:
        f.seek(H['cbLineOffset'] + fdr['cbLineOffset'])
        tabla = f.read(fdr['cbLine'])
    pdrs = _cache['pdrs'][fdr['ipdFirst']:fdr['ipdFirst'] + fdr['cpd']]
    out = []
    for i, p in enumerate(pdrs):
        if num_pdr is not None and i != num_pdr:
            continue
        fin = pdrs[i + 1]['adr'] if i + 1 < len(pdrs) else None
        palabras = (fin - p['adr']) >> 2 if fin else None
        addr = fdr['adr'] + p['adr']
        linea = p['lnLow']
        o = p['cbLineOffset']
        n = 0
        while o < len(tabla):
            b = tabla[o]
            o += 1
            d = b >> 4
            if d:
                if d == 8:
                    if o + 1 >= len(tabla):
                        break
                    delta = struct.unpack_from('>h', tabla, o)[0]
                    o += 2
                else:
                    delta = d - 16 if d > 8 else d
                linea += delta
            for _ in range((b & 0xF) + 1):
                out.append((addr, linea))
                addr += 4
                n += 1
            if palabras is not None and n >= palabras:
                break
    return out


def funciones_gnu():
    """[(nombre_fdr, ifdr)] de los FDR con tabla delta (la parte GNU-as)."""
    carga()
    return [(_cache['cadena'](f['rss']), i) for i, f in enumerate(_cache['fdrs']) if f['cbLine']]


# ---------------------------------------------------------------- lectura .text

def palabra(addr):
    """Instruccion (u32) del ELF en la direccion virtual dada."""
    carga()
    for nombre, va, off, size in _cache['secs']:
        if va <= addr < va + size:
            with open(ELF, 'rb') as f:
                f.seek(off + (addr - va))
                return struct.unpack('<I', f.read(4))[0]
    return None


# ---------------------------------------------------------------- CLI

def _fmt_marcs(i, limite=60):
    f = _cache['funciones'][i]
    fich = _cache['ficheros']
    out = ['===== %s  0x%x..0x%x  [%s]' % (
        f['nombre'], f['addr'], f['fin'],
        fich[f['ifich']] if f['ifich'] >= 0 else '?')]
    prev = None
    for a, ifi, ln in marcadores_de_ifunc(i)[:limite]:
        marca = '' if prev == ln else '   <-- cambia'
        out.append('  0x%08x  linea %-5d %s%s' % (a, ln, fich[ifi] if ifi >= 0 else '?', marca))
        prev = ln
    return '\n'.join(out)


def cmd_fn(patron):
    carga()
    n = 0
    for i, f in enumerate(_cache['funciones']):
        if patron.lower() in f['nombre'].lower():
            print(_fmt_marcs(i))
            n += 1
            if n >= 6:
                print('... (mas coincidencias cortadas)')
                break
    if not n:
        print('sin funciones que casen %r' % patron)


def cmd_addr(arg):
    carga()
    addr = int(arg, 16) if arg.startswith('0x') else int(arg)
    i = funcion_de(addr)
    if i is None:
        print('0x%x: sin funcion' % addr)
        return
    r = linea_en(addr)
    fich = _cache['ficheros']
    print('0x%x  %s  linea %d  (%s)' % (addr, _cache['funciones'][i]['nombre'], r[1],
                                        fich[r[0]] if r[0] >= 0 else '?'))


def cmd_nop(args):
    carga()
    for arg in args:
        addr = int(arg, 16) if arg.startswith('0x') else int(arg)
        i = funcion_de(addr)
        w = palabra(addr)
        fn = _cache['funciones'][i]['nombre'] if i is not None else '?'
        v = veredicto_nop(addr)
        if v is None:
            print('0x%x sin marcadores previos' % addr)
            continue
        propia, linea, prev = v
        fich = _cache['ficheros']
        ifi = linea_en(addr)[0]
        print('0x%08x  word=0x%08x  %-40s  linea %d (previa %s)  %s' % (
            addr, w if w is not None else 0, fn[:40], linea,
            str(prev) if prev is not None else '-',
            'LINEA PROPIA (sentencia del fuente)' if propia else
            'hereda linea (compilador/relleno)'))


def cmd_cobertura():
    carga()
    from collections import Counter
    secs = _cache['secs']
    marc = Counter(m[0] for m in _cache['marcadores'])
    tot = Counter()
    for nombre, va, off, size in secs:
        n = sum(v for a, v in marc.items() if va <= a < va + size)
        print('%-28s 0x%08x..0x%08x  %7d marcadores / %7d instr' % (
            nombre, va, va + size, n, size // 4))
    print('funciones con stabs: %d  |  marcadores $LM: %d' % (
        len(_cache['funciones']), len(_cache['marcadores'])))
    print('parte GNU con tabla delta: %s' % (', '.join(n for n, _ in funciones_gnu()),))


def cmd_fdr(patron=None):
    carga()
    cad = _cache['cadena']
    for i, f in enumerate(_cache['fdrs']):
        n = cad(f['rss'])
        if patron and patron.lower() not in n.lower():
            continue
        print('%3d  0x%08x  sym %7d+%7d  pd %6d+%4d  line %4d+%4d  %s' % (
            i, f['adr'], f['isymBase'], f['csym'], f['ipdFirst'], f['cpd'],
            f['ilineBase'], f['cline'], n))


def cmd_mapa(ruta):
    carga()
    fich = _cache['ficheros']
    out = {'ficheros': fich, 'funciones': []}
    for i, f in enumerate(_cache['funciones']):
        ms = marcadores_de_ifunc(i)
        if not ms:
            continue
        out['funciones'].append({
            'n': f['nombre'], 'a': f['addr'], 'fin': f['fin'],
            'ifich': f['ifich'],
            'm': [[a, ln, ifi] for a, ifi, ln in ms]})
    with open(ruta, 'w') as g:
        json.dump(out, g, separators=(',', ':'))
    print('%d funciones con marcadores -> %s' % (len(out['funciones']), ruta))


# ---------------------------------------------------------------- autotest

def cmd_test():
    """Autotest del mapa: cadenas FDR, delta calibrada, patrones conocidos."""
    carga()
    H = _cache['H']
    fdrs = _cache['fdrs']
    fallos = []

    def ck(cond, msg):
        print(('  OK  ' if cond else 'FALLO') + ' ' + msg)
        if not cond:
            fallos.append(msg)

    print('[1] cadenas FDR contra los maximos de la HDRR')
    ck(fdrs[0]['isymBase'] == 0 and
       all(fdrs[i + 1]['isymBase'] == fdrs[i]['isymBase'] + fdrs[i]['csym']
           for i in range(len(fdrs) - 1)) and
       fdrs[-1]['isymBase'] + fdrs[-1]['csym'] == H['isymMax'],
       'isymBase+csym encadena y cierra en isymMax=%d' % H['isymMax'])
    ck(all(fdrs[i + 1]['ipdFirst'] == fdrs[i]['ipdFirst'] + fdrs[i]['cpd']
           for i in range(len(fdrs) - 1)) and
       fdrs[-1]['ipdFirst'] + fdrs[-1]['cpd'] == H['ipdMax'],
       'ipdFirst+cpd encadena y cierra en ipdMax=%d' % H['ipdMax'])
    ck(fdrs[0]['iauxBase'] == 0 and
       all(fdrs[i + 1]['iauxBase'] == fdrs[i]['iauxBase'] + fdrs[i]['caux']
           for i in range(len(fdrs) - 1)) and
       fdrs[-1]['iauxBase'] + fdrs[-1]['caux'] == H['iauxMax'],
       'iauxBase+caux encadena y cierra en iauxMax=%d' % H['iauxMax'])
    ck(sum(f['cline'] for f in fdrs) == H['ilineMax'],
       'suma de cline == ilineMax=%d (tabla delta solo en .s GNU)' % H['ilineMax'])

    print('[2] tabla delta calibrada (crt0.s de la A124)')
    gnu = funciones_gnu()
    ck(len(gnu) == 4, 'hay exactamente 4 FDR con delta: %s' % ([n for n, _ in gnu],))
    ifdr = next(i for n, i in gnu if 'crt0' in n)
    ent = decodifica_delta(ifdr, 0)  # solo el PDR0 (el __start del crt0)
    ck(bool(ent), 'crt0 PDR0 decodifica %d entradas' % len(ent))
    p0 = _cache['pdrs'][_cache['fdrs'][ifdr]['ipdFirst']]
    ok_rango = all(p0['lnLow'] <= l <= p0['lnHigh'] for _, l in ent)
    ck(ok_rango, 'lineas del PDR0 del crt0 dentro de [%d,%d]' % (p0['lnLow'], p0['lnHigh']))
    ck(all(ent[i][0] < ent[i + 1][0] for i in range(len(ent) - 1)),
       'direcciones monotonas en el crt0')

    print('[3] patrones conocidos del juego ($LM)')
    pn = _cache['por_nombre']
    # a) funcion de C normal: decenas de lineas
    for nom, mini in (('bTransposeMatrix', 8), ('bMemCpy', 8)):
        k = next((i for n, i in pn.items() if n.startswith(nom)), None)
        ms = marcadores_de_ifunc(k) if k is not None else []
        ck(len({l for _, _, l in ms}) >= mini,
           '%s: %d lineas distintas (>= %d)' % (nom, len({l for _, _, l in ms}), mini))
    # b) funcion de UNA sentencia asm: 1-2 lineas
    k = next((i for n, i in pn.items() if 'NetMonitorSyscall__Fiiii' in n), None)
    ms = marcadores_de_ifunc(k) if k is not None else []
    ck(len({l for _, _, l in ms}) <= 3,
       'NetMonitorSyscall__Fiiii: %d lineas distintas (<= 3, asm de una sentencia)'
       % len({l for _, _, l in ms}))
    # c) control negativo: el hueco del superbenderasm.obj (0x47cbe0) no tiene
    #    ni symtab ni stabs: ninguna funcion del mapa lo cubre
    ck(funcion_de(0x47CBE0) is None,
       '0x47cbe0 (superbenderasm.obj) es hueco: sin funcion en el mapa')
    # d) linea efectiva coherente
    r = linea_en(0x15dce0)  # bTransposeMatrix + 8
    ck(r is not None and r[2], 'linea_en da marcador exacto en 0x15dce0 (linea %s)' % (r[1],))

    print('[4] cobertura')
    from collections import Counter
    marc = Counter(m[0] for m in _cache['marcadores'])
    n_text = sum(v for a, v in marc.items() if 0x100000 <= a < 0x534ce8)
    ck(n_text > 400000, 'marcadores $LM en .text: %d' % n_text)

    if fallos:
        print('\n%d FALLOS' % len(fallos))
        sys.exit(1)
    print('\nautotest completo: OK')


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    cmd = sys.argv[1]
    if cmd == 'fn':
        cmd_fn(sys.argv[2] if len(sys.argv) > 2 else '')
    elif cmd == 'addr':
        cmd_addr(sys.argv[2])
    elif cmd == 'nop':
        cmd_nop(sys.argv[2:])
    elif cmd == 'mapa':
        cmd_mapa(sys.argv[3] if len(sys.argv) > 3 and sys.argv[2] == '--json'
                 else 'build/mdebug_lineas.json')
    elif cmd == 'cobertura':
        cmd_cobertura()
    elif cmd == 'fdr':
        cmd_fdr(sys.argv[2] if len(sys.argv) > 2 else None)
    elif cmd == 'test':
        cmd_test()
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
