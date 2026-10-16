#!/usr/bin/env python3
"""llamantes360.py -- quien llama (b/bl directos) a cada funcion de un rango del original de Xbox 360 (ronda 52).

Para decidir a que unidad va un rango sin trocear o mal cortado: por cada funcion de symbols.txt en [A0,A1), cuantas
llamadas recibe desde cada unidad de splits.txt ('-' = sin unidad) y los tres primeros llamantes. Con --dis VA [TAM]
desensambla el original (capstone) con los nombres de symbols.txt en los destinos y en las cargas lis/addi.

    python scripts/llamantes360.py A0 A1
    python scripts/llamantes360.py --dis VA [TAM]
"""
import bisect
import collections
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if not os.path.isdir(os.path.join(RAIZ, 'config', 'EUROPEGERMILESTONE')):
    RAIZ = os.environ.get('NFSMW_RAIZ', os.getcwd())  # copia de trabajo fuera del arbol: la raiz es el directorio actual
V = 'EUROPEGERMILESTONE'
BASE = 0x82000000


def exe():
    d = open(os.path.join(RAIZ, 'orig', V, 'NFS.exe'), 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    secs = []
    o = pe + 24 + opt
    for _ in range(nsec):
        nm = d[o:o + 8].rstrip(b'\0')
        vs, va, rs, rp = struct.unpack_from('<IIII', d, o + 8)
        secs.append((nm, BASE + va, vs, rs, rp))
        o += 40
    return d, secs


def simbolos():
    funcs = []
    nombres = {}
    for l in open(os.path.join(RAIZ, 'config', V, 'symbols.txt'), encoding='utf-8', errors='replace'):
        m = re.match(r'(\S+) = \.(\w+):0x([0-9A-Fa-f]+);(?:.*size:0x([0-9A-Fa-f]+))?', l)
        if not m:
            continue
        a = int(m.group(3), 16)
        nombres.setdefault(a, m.group(1))
        if m.group(2) == 'text' and 'type:function' in l and m.group(4):
            funcs.append((a, int(m.group(4), 16), m.group(1)))
    funcs.sort()
    return funcs, nombres


def unidades():
    spl = []
    cur = None
    for l in open(os.path.join(RAIZ, 'config', V, 'splits.txt'), encoding='utf-8'):
        if l.strip().endswith(':') and not l[0].isspace() and not l.startswith('Sections'):
            cur = l.strip()[:-1].split('/')[-1].rsplit('.', 1)[0]
        m = re.match(r'\s+\.text\s+start:0x([0-9A-F]+) end:0x([0-9A-F]+)', l)
        if m and cur:
            spl.append((int(m.group(1), 16), int(m.group(2), 16), cur))
    return spl


def llamantes(a0, a1):
    d, secs = exe()
    funcs, _ = simbolos()
    st = [x[0] for x in funcs]

    def fn(a):
        i = bisect.bisect_right(st, a) - 1
        return funcs[i] if i >= 0 and a < funcs[i][0] + funcs[i][1] else None
    spl = unidades()

    def unit(a):
        for s, e, u in spl:
            if s <= a < e:
                return u
        return '-'
    t0, tb = [(va, d[rp:rp + rs]) for nm, va, vs, rs, rp in secs if nm == b'.text'][0]
    calls = collections.defaultdict(list)
    for k in range(0, len(tb) - 3, 4):
        w = struct.unpack_from('>I', tb, k)[0]
        if w >> 26 == 18:
            li = w & 0x03FFFFFC
            if li & 0x02000000:
                li -= 0x04000000
            a = t0 + k
            dest = li if w & 2 else a + li
            f = fn(a)
            if f and f[0] <= dest < f[0] + f[1]:
                continue
            calls[dest].append(a)
    for s, z, n in funcs:
        if a0 <= s < a1:
            cs = calls.get(s, [])
            us = collections.Counter(unit(c) for c in cs)
            ej = ', '.join('%s' % (fn(c)[2][:40] if fn(c) else '%08X' % c) for c in cs[:3])
            print('%08X %4X %-48s %s | %s' % (s, z, n[:48], dict(us), ej))


def dis(va, n):
    import capstone
    d, secs = exe()
    funcs, names = simbolos()
    if n is None:
        n = next((z for s, z, _ in funcs if s == va), 0x100)

    def off(a):
        for nm, sva, vs, rs, rp in secs:
            if sva <= a < sva + max(vs, rs):
                return rp + a - sva
    print('==', names.get(va, '?'))
    md = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)
    md.skipdata = True
    hi = {}
    for i in md.disasm(d[off(va):off(va) + n], va):
        s = i.op_str
        extra = ''
        if i.mnemonic in ('bl', 'b') or (i.mnemonic.startswith('b') and s.startswith('0x')):
            t = int(s.split(',')[-1].strip(), 16)
            if t in names:
                extra = '  ; ' + names[t]
        if i.mnemonic == 'lis':
            rr, v = s.split(', ')
            hi[rr] = int(v, 0) & 0xFFFF
        if i.mnemonic in ('addi', 'lwz', 'lfs', 'stw', 'stfs', 'lbz', 'lhz', 'stb', 'sth', 'lfd'):
            p = s.split(', ')
            if i.mnemonic == 'addi' and len(p) == 3 and p[1] in hi:
                t = ((hi[p[1]] << 16) + int(p[2], 0)) & 0xFFFFFFFF
                extra = '  ; %08X %s' % (t, names.get(t, ''))
            elif len(p) == 2 and '(' in p[1]:
                disp, rb = p[1].rstrip(')').split('(')
                if rb in hi:
                    t = ((hi[rb] << 16) + int(disp, 0)) & 0xFFFFFFFF
                    extra = '  ; %08X %s' % (t, names.get(t, ''))
        if va != i.address and i.address in names:
            print('-- %s' % names[i.address])
        print('%08x  %-7s %s%s' % (i.address, i.mnemonic, s, extra))


if __name__ == '__main__':
    if sys.argv[1] == '--dis':
        dis(int(sys.argv[2], 16), int(sys.argv[3], 16) if len(sys.argv) > 3 else None)
    else:
        llamantes(int(sys.argv[1], 16), int(sys.argv[2], 16))
