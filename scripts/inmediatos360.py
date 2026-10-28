#!/usr/bin/env python3
"""inmediatos360.py [--ltcg DIR] [--informe JSON] [--min 97] -- casi exactas de Xbox 360 cuyo UNICO cambio frente al
original son inmediatos (ronda 83, X360-B; a scripts/ en la 84).

Compara instruccion a instruccion (mismo numero de instrucciones) cada funcion del informe LTCG con MIN <= % < 100:
solo se admiten diferencias en el inmediato de li/cmpwi/cmplwi/subfic/mulli/rlwinm/clrlwi/slwi/srwi/srawi/andi./ori/
xori con el mismo registro destino, y se ignoran los saltos y las cargas/stores con base (direcciones, el pool). Son
constantes que pueden ser de plataforma: 2 mandos frente a 4, una tabla de 100 frente a 400, un BitArray de 256 frente
a 2048 bits, flags de NewSlotPool... En la ronda 83 dio diez en veinte minutos.

    python scripts/inmediatos360.py                       # desde la raiz, tras una medida de X360

Como tamanos360.py, es una PISTA: cada constante se comprueba en el binario y va con guarda de plataforma; un
__LINE__ distinto o un nombre sobre una funcion plegada ("return 6/7/8") no es una constante de plataforma.
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tamanos360 import LTCG, MD, ORIG, ORIG_BASE, funciones_ltcg, pe, simbolos_originales  # noqa: E402

IMM = {'li', 'cmpwi', 'cmplwi', 'subfic', 'mulli', 'rlwinm', 'clrlwi', 'slwi', 'srwi', 'srawi', 'andi.', 'ori', 'xori'}
IGNORA = {'b', 'bl', 'beq', 'bne', 'blt', 'bgt', 'ble', 'bge', 'bdnz', 'lis', 'addi', 'lwz', 'stw', 'lfs', 'stfs',
          'lbz', 'stb', 'lhz', 'sth', 'lfd', 'stfd', 'lwzx', 'stwx'}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--ltcg', default=LTCG)
    ap.add_argument('--informe', default=os.path.join('build', 'versiones', 'EUROPEGERMILESTONE.json'))
    ap.add_argument('--min', type=float, default=97.0)
    a = ap.parse_args()
    egm = simbolos_originales()
    nu = funciones_ltcg(a.ltcg)
    le, ln = pe(ORIG, ORIG_BASE), pe(os.path.join(a.ltcg, 'NFS_ltcg.exe'))
    d = json.load(open(a.informe, encoding='utf-8'))
    for u in d['units']:
        for f in u.get('functions', []):
            p = f.get('fuzzy_match_percent') or 0
            nm = f['name']
            if not (a.min <= p < 100) or nm not in egm or nm not in nu:
                continue
            va, n = egm[nm]
            x = list(MD.disasm(le(va, n), va))
            y = list(MD.disasm(ln(nu[nm], n), nu[nm]))
            if len(x) != len(y):
                continue
            dif, ok = [], True
            for i, j in zip(x, y):
                if i.mnemonic != j.mnemonic:
                    ok = False
                    break
                if i.op_str == j.op_str:
                    continue
                if i.mnemonic in IMM and i.op_str.split(',')[0] == j.op_str.split(',')[0]:
                    dif.append('%s %s | %s' % (i.mnemonic, i.op_str, j.op_str))
                elif i.mnemonic not in IGNORA:
                    ok = False
                    break
            if ok and dif:
                print('%.2f %-70s %s' % (p, nm[:70], ' ; '.join(dif[:4])))


if __name__ == '__main__':
    main()
