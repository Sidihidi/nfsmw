#!/usr/bin/env python3
"""huerfanos.py UNIDAD SIMBOLO  -- GameCube: huellas de codigo muerto del original.

Dos listas, sacadas del asm del original con su linea (scripts/lmap.py):

1. Comparaciones huerfanas: cmpw/cmpwi/cmplw/cmplwi/fcmpu/fcmpo y las instrucciones con punto (andi., mr.,
   rlwinm. ...) cuyo cr no lee ningun salto, cr* ni mfcr antes de que se vuelva a escribir o acabe el bloque. Son
   restos de un `if` cuyo cuerpo quedo vacio: jump2 borra el salto pero no la comparacion. Sigue el grafo de
   saltos (los cr2-cr4 sobreviven a las llamadas).
2. Constantes solas: lineas del .cpp propio cuyas instrucciones son SOLO cargas de constantes (lis/ori/li y lfs/lfd
   de @l). Si nada de esa linea opera con la constante, la usa otra linea (o nadie): la huella de un clamp o un `if`
   que EA escribio y no usa (GenerateRoadNoise, ronda 39). Compara con scripts/lado.py --lineas.

No toda huella es codigo muerto (una constante puede adelantarse a la linea de su declaracion); es una pista para
mirar con lado.py, no una prueba.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CMP = ('cmpw', 'cmpwi', 'cmplw', 'cmplwi', 'fcmpu', 'fcmpo')
SALTOS_SIN_CR = ('b', 'bl', 'blr', 'bctr', 'bctrl', 'blrl')


def leer(unit, sym):
    out = subprocess.run([sys.executable, os.path.join(ROOT, 'scripts', 'lmap.py'), unit, sym],
                         capture_output=True, text=True, cwd=ROOT).stdout
    ins = []
    for ln in out.splitlines()[1:]:
        m = re.match(r'^([0-9A-F]{8})  (.{44})\s*(.*)$', ln)
        if m:
            ins.append((m.group(1), m.group(2).strip(), m.group(3)))
        elif ln.strip().startswith('.L'):
            ins.append(('LABEL', ln.strip(), ''))
    return ins


def partes(t):
    op = t.split()[0] if t else ''
    a = t.split(None, 1)[1] if ' ' in t else ''
    return op, a


def crdef(t):
    op, a = partes(t)
    if op in CMP:
        m = re.match(r'(cr\d)', a)
        return m.group(1) if m else 'cr0'
    if op.endswith('.') and op != 'stwcx.':
        return 'cr0'
    return None


def cruse(t):
    op, a = partes(t)
    if op.startswith('b') and op not in SALTOS_SIN_CR:
        m = re.match(r'(cr\d)', a)
        return [m.group(1) if m else 'cr0']
    if op == 'mfcr':
        return ['cr%d' % i for i in range(8)]
    if op.startswith('cr'):
        return re.findall(r'(cr\d)', a) or ['cr0']
    return []


VOLATILES = ('cr0', 'cr1', 'cr5', 'cr6', 'cr7')


def huerfanas(ins):
    """Recorre el grafo de saltos desde cada comparacion: huerfana si ningun camino lee su cr antes de que se
    reescriba (una llamada `bl` destruye cr0, cr1 y cr5-cr7; cr2-cr4 sobreviven)."""
    pos = {}
    for i, (ad, t, _) in enumerate(ins):
        if ad == 'LABEL':
            pos[t.rstrip(':')] = i
    res = []
    for i, (ad, t, lineas) in enumerate(ins):
        c = crdef(t)
        if not c:
            continue
        pila, visto, usado = [i + 1], set(), False
        while pila and not usado:
            j = pila.pop()
            while j < len(ins) and j not in visto:
                visto.add(j)
                ad2, t2, _ = ins[j]
                if ad2 == 'LABEL':
                    j += 1
                    continue
                if c in cruse(t2):
                    usado = True
                    break
                if crdef(t2) == c:
                    break
                op, arg = partes(t2)
                if op == 'bl' and c in VOLATILES:
                    break
                if op in ('blr', 'bctr'):
                    break
                m = re.search(r'(\.L_[0-9A-F]+)', arg)
                if op.startswith('b') and op not in ('bl', 'bctrl', 'blrl') and m and m.group(1) in pos:
                    pila.append(pos[m.group(1)])
                    if op == 'b':
                        break
                j += 1
        if not usado and not resultado_leido(ins, i):
            res.append((ad, t, lineas))
    return res


def resultado_leido(ins, i):
    """andi./andis. no tienen forma sin punto: su cr0 sobra siempre que el codigo use solo el resultado (el `cmpwi`
    de un `(x & m) == m`). Solo son huella si el registro destino tampoco se lee en el resto del bloque."""
    op, a = partes(ins[i][1])
    if op not in ('andi.', 'andis.'):
        return False
    dest = a.split(',')[0].strip()
    for ad2, t2, _ in ins[i + 1:]:
        if ad2 == 'LABEL':
            return False
        op2, a2 = partes(t2)
        ops = [x.strip() for x in re.split(r'[,()]', a2) if x.strip()]
        if dest in ops[1:] or (op2.startswith(('st', 'cmp', 'mt', 'b')) and dest in ops):
            return True
        if ops and ops[0] == dest:
            return False
        if op2 in ('b', 'blr', 'bctr'):
            return False
    return False


def es_constante(t):
    op, a = partes(t)
    if op in ('lis', 'li'):
        return True
    if op == 'ori':
        return True
    if op in ('lfs', 'lfd', 'addi') and '@l' in a:
        return True
    return False


def solas(ins):
    """Lineas propias (.cpp/.c) cuyas instrucciones son todas cargas de constantes."""
    por = {}
    orden = []
    cur = None
    for ad, t, lineas in ins:
        if ad == 'LABEL':
            continue
        tags = re.findall(r'(\S+?\.(?:cpp|c|h|hpp|inl)):(\d+)', lineas)
        if tags:
            cur = tags[-1]
        if not cur or not cur[0].endswith(('.cpp', '.c')):
            continue
        k = (cur[0], int(cur[1]))
        if k not in por:
            orden.append(k)
        por.setdefault(k, []).append((ad, t))
    return [(k, por[k]) for k in orden if all(es_constante(t) for _, t in por[k])]


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    ins = leer(sys.argv[1], sys.argv[2])
    print('comparaciones huerfanas:')
    for ad, t, lineas in huerfanas(ins):
        print('  %s  %-44s %s' % (ad, t, lineas))
    print('lineas con solo constantes:')
    for (f, n), filas in solas(ins):
        print('  %s:%d  %s' % (f, n, ' ; '.join('%s %s' % (a, t) for a, t in filas)))


if __name__ == '__main__':
    main()
