#!/usr/bin/env python3
"""Comparacion REAL por bytes (con reubicaciones enmascaradas) para X360.

objdiff/capstone imprime a veces el mismo byte de salto de forma distinta
en cada lado (bdnzf lt, <sim> vs bne cr6, <dir>): el % miente. Aqui manda
el byte: se enmascaran los sitios con reubicacion COFF y se compara.
"""
import json, os, struct, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
V = 'EUROPEGERMILESTONE'
SCR = os.environ.get('SCRATCH', 'C:/Users/jferr/scratch-r21-x360')

# tamano enmascarado por tipo de reubicacion PPC (IMAGE_REL_PPC_*)
TAM = {0x0001: 2, 0x0002: 4, 0x0003: 2, 0x0004: 4, 0x0005: 2, 0x0006: 4,
       0x0007: 4, 0x0008: 4, 0x0009: 2, 0x000A: 2, 0x000B: 4, 0x000C: 4,
       0x000D: 4, 0x000E: 2, 0x000F: 4, 0x0010: 4, 0x0011: 4, 0x0012: 2,
       0x0013: 4, 0x0014: 4}

def seccion(d, nsec, szopt, idx):
    off = 20 + szopt + (idx - 1) * 40
    se = d[off:off + 40]
    vsize, vaddr, rawsz, rawptr, relptr, lnptr, nrel, nln, flags = struct.unpack('<IIIIIIHHI', se[8:40])
    return vaddr, rawptr, rawsz, relptr, nrel

def simbolo(d, want):
    mach, nsec, ts, ptrSym, nsym, szopt, chr = struct.unpack('<HHIIIHH', d[:20])
    strs = d[ptrSym + nsym * 18:]
    for i in range(nsym):
        e = d[ptrSym + i * 18:ptrSym + i * 18 + 18]
        nm, val, scn, typ, cls, aux = struct.unpack('<8sIhHBB', e[:18])
        if nm[:4] == b'\x00\x00\x00\x00':
            off = struct.unpack('<I', nm[4:])[0]
            end = strs.find(b'\x00', off)
            name = strs[off:end].decode('utf-8', 'replace')
        else:
            name = nm.rstrip(b'\x00').decode('utf-8', 'replace')
        if name == want and scn > 0:
            vaddr, rawptr, rawsz, relptr, nrel = seccion(d, nsec, szopt, scn)
            fn_off = val - vaddr
            start = rawptr + fn_off
            rels = []
            for j in range(nrel):
                r = d[relptr + j * 10:relptr + j * 10 + 10]
                rv, rsi, rty = struct.unpack('<IIH', r)
                rels.append((rv - vaddr - fn_off, TAM.get(rty, 4)))
            return start, rels
    return None, None

def cuerpo(path, sym, tam):
    try:
        d = open(path, 'rb').read()
    except OSError:
        return None
    start, rels = simbolo(d, sym)
    if start is None:
        return None
    raw = d[start:start + tam]
    buf = bytearray(raw)
    for off, t in rels:
        if 0 <= off < tam:
            for k in range(min(t, tam - off)):
                buf[off + k] = 0
    return bytes(raw), bytes(buf)

def rel_deUnidad(short):
    for cand in ('Speed/Indep/SourceLists/' + short, short):
        if os.path.exists(os.path.join(ROOT, 'build', V, 'obj', cand + '.obj')):
            return cand
    return None

if __name__ == '__main__':
    rep = json.load(open(os.path.join(ROOT, 'build', 'versiones', 'EUROPEGERMILESTONE.json')))
    cands = []
    for u in rep.get('units', []):
        short = u['name'].split('/')[-1]
        for f in u.get('functions', []) or []:
            pct = f.get('fuzzy_match_percent', 0.0); tam = int(f.get('size', 0) or 0)
            if 80.0 <= pct < 99.999 and tam >= 100:
                cands.append((pct, tam, f['name'], short))
    cands.sort(key=lambda r: (-r[0], -r[1]))
    solo = sys.argv[1] if len(sys.argv) > 1 else None
    n = 0
    for pct, tam, sym, short in cands:
        if solo and solo not in sym:
            continue
        rel = rel_deUnidad(short.replace('.obj', ''))
        if rel is None:
            continue
        ra = cuerpo(os.path.join(ROOT, 'build', V, 'obj', rel + '.obj'), sym, tam)
        rb = cuerpo(os.path.join(ROOT, 'build', V, 'src', rel + '.obj'), sym, tam)
        if ra is None or rb is None:
            print('SIN-SIMBOLO %s' % sym); continue
        (araw, amsk), (braw, bmsk) = ra, rb
        if amsk == bmsk:
            print('FANTASMA %.2f %6d %s [%s]' % (pct, tam, sym, rel)); n += 1
            continue
        # FANTASMA-REL: las unicas palabras distintas tienen los mismos bytes
        # crudos (reubicacion espuria de un solo lado, p. ej. el salto a si
        # mismo que jeff reubica). Si el crudo tambien difiere, es real.
        dif = [i // 4 for i in range(0, min(len(amsk), len(bmsk)), 4)
               if amsk[i:i + 4] != bmsk[i:i + 4]]
        if dif and all(araw[i * 4:i * 4 + 4] == braw[i * 4:i * 4 + 4] for i in dif):
            print('FANTASMA-REL %.2f %6d %s [%s] filas %s' % (pct, tam, sym, rel, dif)); n += 1
        elif len(sys.argv) > 2 and sys.argv[2] in sym:
            print('DIF %d/%d %s' % (len(amsk), len(bmsk), sym))
            for i in range(0, min(len(amsk), len(bmsk)), 4):
                if amsk[i:i + 4] != bmsk[i:i + 4]:
                    print('  fila %d: %s || %s%s' % (i // 4, amsk[i:i + 4].hex(), bmsk[i:i + 4].hex(),
                          '  (crudo igual)' if araw[i:i + 4] == braw[i:i + 4] else ''))
    print('fantasmas:', n)
