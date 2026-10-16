#!/usr/bin/env python3
"""mdebug.py -- lee el .mdebug (ECOFF/MIPS) del build de PS2 (SLES-53558-A124).

El ELF de PS2 trae 84,5 MB de debug info ECOFF con los stabs de MWCC/GCC:
para cada funcion del juego hay simbolos con los PARAMETROS (`this:P29203`),
los BLOQUES anidados ($LBB/$LBE), las etiquetas de linea ($LMnnnn) y las
rutas de los ficheros en el orden de expansion inline. El codigo MIPS no se
copia, pero esa estructura ES el fuente original compartido con GameCube.

Los offsets de las tablas salen de la cabecera HDRR, que sigue el orden
estandar de IRIX. En este fichero:

    PDR   0x55f804  31957 x 52 B
    SYMR  0x6f5348  1896029 x 12 B
    ss    0x1ce6f08, 0x3797bd6 B (issMax es el TAMANO; las cadenas externas
          empiezan justo detras, en 0x547eade)
    lineas 0x55f3ac (bytes delta, ver decodifica_lineas)

Los `iss` de los simbolos son absolutos: el issBase de los 559 FDR es 0.

    python scripts/mdebug.py fn <nombre>       # estructura de una funcion
    python scripts/mdebug.py fn <nombre> -L    # + tabla de lineas
    python scripts/mdebug.py files [patron]    # rutas de fuente unicas
"""
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
ELF = os.path.join(ROOT, "orig", "SLES-53558-A124", "NFS.ELF")

def _hdrr():
    """Offsets de las tablas segun la cabecera HDRR de la seccion .mdebug."""
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
                return dict(cbLineOffset=h[4], ipdMax=h[7], cbPdOffset=h[8],
                            isymMax=h[9], cbSymOffset=h[10], issMax=h[15],
                            cbSsOffset=h[16], ifdMax=h[19], cbFdOffset=h[20])
    raise SystemExit('%s no tiene .mdebug' % ELF)


_H = _hdrr()
PDR_OFF, PDR_N = _H['cbPdOffset'], _H['ipdMax']
SYM_OFF, SYM_N = _H['cbSymOffset'], _H['isymMax']
SS_OFF, SS_SIZE = _H['cbSsOffset'], _H['issMax']
LINE_OFF = _H['cbLineOffset']
FD_OFF, FDR_N = _H['cbFdOffset'], _H['ifdMax']

ST = {0: 'nil', 1: 'global', 2: 'static', 3: 'param', 4: 'local', 5: 'label',
      6: 'proc', 7: 'block', 8: 'end', 11: 'file', 14: 'staticproc'}

_cache = {}


def carga():
    if _cache:
        return
    f = open(ELF, 'rb')
    f.seek(PDR_OFF)
    praw = f.read(PDR_N * 52)
    pdrs = []
    for i in range(PDR_N):
        o = i * 52
        # PDR canonico ecoff32 (include/coff/sym.h): adr, isym, iline, regmask,
        # regoffset, iopt, fregmask, fregoffset, frameoffset, framereg(u16),
        # pcreg(u16), lnLow, lnHigh, cbLineOffset.  El adr es RELATIVO al adr
        # del FDR.  Los PDR del codigo MW llevan lnLow=lnHigh=-1: sin lineas.
        adr, isym, iline, regmask, regoffset, iopt, fregmask, fregoffset, \
            frameoffset, framereg, pcreg, lnLow, lnHigh, cbLineOffset = \
            struct.unpack('<IiiiiiiiihhiiI'.replace(' ', ''), praw[o:o + 52])
        pdrs.append(dict(adr=adr, isym=isym, iline=iline, lnLow=lnLow,
                         lnHigh=lnHigh, cbLineOffset=cbLineOffset,
                         frameoffset=frameoffset))
    _cache['pdrs'] = pdrs
    del praw

    f.seek(SYM_OFF)
    sraw = f.read(SYM_N * 12)
    syms = []
    for i in range(SYM_N):
        o = i * 12
        iss, value, bits = struct.unpack('<III', sraw[o:o + 12])
        syms.append((iss, value, bits & 0x3f, (bits >> 6) & 0x1f))
    _cache['syms'] = syms
    del sraw

    f.seek(SS_OFF)
    ss = f.read(SS_SIZE)
    _cache['ss'] = ss
    f.close()

    # indice: nombre de funcion -> (symidx, ...)
    por_nombre = {}
    for i, (iss, value, st, sc) in enumerate(syms):
        if st == 6:
            nom = _cadena(iss)
            if nom:
                por_nombre.setdefault(nom, i)
    _cache['por_nombre'] = por_nombre


def _cadena(iss):
    ss = _cache['ss']
    if iss >= len(ss):
        return ''
    e = ss.find(b'\0', iss)
    if e < 0:
        e = len(ss)
    return ss[iss:e].decode('latin-1')


def decodifica_lineas(pdr):
    """Tabla delta de lineas del PDR (formato calibrado en la ronda 27 contra
    gas/ecoff.c:ecoff_build_lineno de binutils y una sonda del ee-gcc real):

      nibble bajo  = (numero de instrucciones de la entrada) - 1
      nibble alto  = delta de linea: 1..7 suma, 9..15 resta (d-16) y 8 es la
                     forma de 3 bytes con un int16 big-endian a continuacion

    La linea inicial es pdr['lnLow']; el cbLineOffset del PDR es relativo al
    cbLineOffset del FDR (los 4 ficheros GNU .s: crt0, snd smtflt59, klib,
    tlbtrap).  Los PDR del codigo MW no tienen tabla (devuelve []).
    Para el mapa completo de lineas usar scripts/mdebug_lineas.py."""
    fdrs = _cache.get('fdrs')
    if fdrs is None:
        # FDRs perezosos: solo hacen falta para la parte GNU
        fd = open(ELF, 'rb')
        fd.seek(FD_OFF)
        fraw = fd.read(FDR_N * 72)
        fd.close()
        fdrs = [struct.unpack_from('<18I', fraw, i * 72) for i in range(FDR_N)]
        _cache['fdrs'] = fdrs
    # localizar el FDR del PDR por su indice en la lista
    idx = None
    for k, p in enumerate(_cache['pdrs']):
        if p is pdr:
            idx = k
            break
    if idx is None:
        return []
    base = 0
    for v in fdrs:
        ipd, cpd = v[10] & 0xFFFF, v[10] >> 16
        if ipd <= idx < ipd + cpd and v[16]:
            base = v[16]
            break
    f2 = open(ELF, 'rb')
    f2.seek(LINE_OFF + base + pdr['cbLineOffset'])
    raw = f2.read(4096)
    f2.close()
    out = []
    line = pdr['lnLow']
    addr = pdr['adr']
    # fin del procedimiento: el adr del siguiente PDR del mismo FDR
    fin = None
    for k2 in range(idx + 1, PDR_N):
        if _cache['pdrs'][k2]['adr'] != 0xFFFFFFFF:
            fin = _cache['pdrs'][k2]['adr']
            break
    o = 0
    while o < len(raw):
        b = raw[o]
        o += 1
        d = b >> 4
        if d:
            if d == 8:
                if o + 1 >= len(raw):
                    break
                line += struct.unpack_from('>h', raw, o)[0]
                o += 2
            else:
                line += d - 16 if d > 8 else d
        for _ in range((b & 0xF) + 1):
            if fin is not None and addr >= fin:
                return out
            out.append((addr, line))
            addr += 4
        if line > pdr['lnHigh'] + 8 or len(out) > 4000:
            break
    return out


def cmd_fn(nm, con_lineas=False):
    carga()
    pn = _cache['por_nombre']
    claves = [k for k in pn if nm.lower() in k.lower()]
    if not claves:
        print('no hay funciones que casen %r' % nm)
        return
    syms = _cache['syms']
    for k in claves[:3]:
        i0 = pn[k]
        iss, value, st, sc = syms[i0]
        print('===== %s  @0x%x' % (k, value))
        # el PDR asociado
        pdr = None
        for p in _cache['pdrs']:
            if p['isym'] == i0:
                pdr = p
                break
        if pdr:
            print('  lineas %d..%d (irline %d)  frame %d' % (
                pdr['lnLow'], pdr['lnHigh'], pdr['irline'], pdr['frameoffset']))
        # volcar los simbolos de la funcion hasta el stEnd que la cierra
        prof = 0
        fichero = ''
        for j in range(i0, min(i0 + 4000, SYM_N)):
            jiss, jval, jst, jsc = syms[j]
            nom = _cadena(jiss)
            if jst == 8 and jval == j - i0 - 1 and prof == 0:
                break
            if jst == 6:
                continue
            if jst == 5:  # labels: lineas y ficheros
                if nom.startswith('$LM'):
                    print('    %-8s 0x%08x' % (nom, jval))
                elif nom.endswith(('.cpp', '.h', '.hpp', '.c', '.inl')) or '/src/' in nom:
                    fichero = nom
                    print('    FILE     %s' % nom)
                continue
            if jst in (0, 2, 3, 4):
                if nom.startswith('$LB'):
                    print('    %-14s 0x%x' % (nom, jval))
                elif ':' in nom and len(nom) < 120:
                    print('    %-14s %-8s val=%-6d %s' % (
                        ST.get(jst, jst), '', jval, nom))
        if con_lineas and pdr:
            print('  --- lineas (PDR) ---')
            for a, l in decodifica_lineas(pdr)[:80]:
                print('    0x%08x  linea %d' % (a, l))


def cmd_files(pat):
    carga()
    vistos = set()
    for iss, value, st, sc in _cache['syms']:
        if st == 5:
            nom = _cadena(iss)
            if '/src/' in nom or nom.endswith(('.cpp', '.h', '.hpp')):
                if nom not in vistos:
                    vistos.add(nom)
    for v in sorted(vistos):
        if not pat or pat.lower() in v.lower():
            print(v)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(0)
    cmd = sys.argv[1]
    if cmd == 'fn':
        cmd_fn(sys.argv[2] if len(sys.argv) > 2 else '', '-L' in sys.argv)
    elif cmd == 'files':
        cmd_files(sys.argv[2] if len(sys.argv) > 2 else None)
    else:
        print(__doc__)
