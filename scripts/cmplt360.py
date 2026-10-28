#!/usr/bin/env python3
"""cmplt360.py PAREJAS.tsv [-v] [--enlace DIR] -- compara funciones de nuestro enlace LTCG con el EGM, sin objdiff:
filas `nombre_decorado<TAB>0xVA_EGM`. Normaliza los pares lis + addi/lwz/stw... de direcciones absolutas, los destinos
de b/bl (por NOMBRE cuando los dos lados lo tienen; si no, 'T') y los saltos relativos. `==` marca las iguales; -v
imprime el diff de las demas.

Sirve para iterar sobre un enlace LTCG privado (ltcg360.py en un directorio aparte, o una copia de il/ con una unidad
recompilada) sin gastar una medida: lo que sale `==` aqui casa en la medida salvo el valor del pool y las tablas.

    python scripts/cmplt360.py voz.tsv -v --enlace D:/scratch/ltcg
"""
import difflib
import re
import sys

import egm360


def norm(ins, noms, comunes):
    out, lis = [], {}
    for x in ins:
        m, op = x.mnemonic, x.op_str
        if m == 'lis':
            r = op.split(',')[0]
            lis[r] = True
            out.append('lis %s, H' % r)
            continue
        if m in ('b', 'bl') or (m.startswith('b') and re.search(r'0x[0-9a-f]+$', op) and
                                m not in ('blr', 'bctr', 'bctrl')):
            t = re.search(r'0x([0-9a-f]+)$', op)
            if t and m in ('b', 'bl'):
                nm = noms.get(int(t.group(1), 16))
                op = '<%s>' % nm if nm and not nm.startswith('fn_') and nm in comunes else 'T'
            elif t:
                op = re.sub(r'0x[0-9a-f]+$', '+%d' % (int(t.group(1), 16) - x.address), op)
        mm = re.match(r'^(\w+), (-?0x[0-9a-f]+|-?\d+)\((\w+)\)$', op)
        if mm and mm.group(3) in lis:
            op = '%s, L(%s)' % (mm.group(1), mm.group(3))
        mm = re.match(r'^(\w+), (\w+), (-?0x[0-9a-f]+|-?\d+)$', op)
        if m == 'addi' and mm and mm.group(2) in lis:
            op = '%s, %s, L' % (mm.group(1), mm.group(2))
        dst = op.split(',')[0] if op else ''
        if m not in ('stw', 'stb', 'sth', 'stfs', 'stfd', 'std', 'cmpwi', 'cmplwi', 'cmpw', 'cmplw', 'addi') and \
                dst in lis:
            del lis[dst]
        out.append(m + ' ' + op)
    return out


def main():
    argv = sys.argv[1:]
    d = egm360.enlace(argv)
    v = '-v' in argv
    argv = [a for a in argv if a != '-v']
    E, N = egm360.egm(), egm360.nuestro(d)
    sims = egm360.simbolos_egm()
    enom = {a: n for a, n, t in sims}
    esz = {a: t for a, n, t in sims}
    comunes = set(enom.values())
    nnom, nsz, ndir = {}, {}, {}
    for a, n, o, t in egm360.mapa(d):
        nnom[a] = n
        nsz[a] = t
        ndir.setdefault(n, a)
    tot = [0, 0]
    for linea in open(argv[0]):
        if not linea.strip() or linea.startswith('#'):
            continue
        nm, va = linea.split()[:2]
        va = int(va, 16)
        a = ndir.get(nm)
        if a is None:
            print('   --  sin %s en nuestro map' % nm)
            continue
        e = norm(E.ins(va, esz.get(va, 0)), enom, comunes)
        o = norm(N.ins(a, nsz[a]), nnom, comunes)
        while o and o[-1].startswith('.word'):
            o.pop()
        r = difflib.SequenceMatcher(None, e, o, autojunk=False).ratio()
        igual = e == o
        tot[0] += 1
        tot[1] += igual
        print('%s %5.1f%%  %4d/%4d B  %08X %s' % ('==' if igual else '  ', r * 100, nsz[a], esz.get(va, 0), va,
                                                nm[:90]))
        if v and not igual:
            for x in list(difflib.unified_diff(e, o, 'egm', 'nuestro', n=1, lineterm=''))[2:]:
                print('        ' + x)
    print('iguales %d de %d' % (tot[1], tot[0]))


if __name__ == '__main__':
    main()
