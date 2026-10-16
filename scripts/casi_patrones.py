#!/usr/bin/env python3
"""Clasifica por PATRON las casi-exactas que saca casi.py, para empezar por lo
que ya se sabe arreglar.

Cada fila distinta se mete en un cajon:

  conmutativa   add.s/mul.s/add/addu/mul con los operandos al reves.  En la
                fuente es `a + b' contra `b + a', y es NEUTRA en GameCube: da
                el mismo PowerPC.  OJO: bMin/bMax NO lo son (medido: invertir
                bMin rompio RemoveUnwantedVehicles en GameCube).
  constante     mismo mnemonico, mismos registros, otro inmediato.
  desplaz.      carga o guarda con otro desplazamiento sobre la misma base:
                un miembro corrido.
  signo         sra contra srl, slt contra sltu, lh contra lhu...
  registro      solo cambian los registros: asignacion, lo mas caro.
  de mas/menos  una instruccion que sobra o falta.

  python scripts/casi_patrones.py fichero_de_casi.txt
"""
import collections
import re
import sys

RE_FILA = re.compile(r'\s+(\d+)\s+(\d+)\s+([\d.]+)\s+(\S+)\s+(\S+)')
RE_INS = re.compile(r'^\s*(\d+)\s+(.*?)\s*\|\s*(.*)$')
CONMUT = ('add.s', 'mul.s', 'add', 'addu', 'daddu', 'mul', 'and', 'or', 'xor',
          'fadds', 'fmuls', 'fadd', 'fmul')
SIGNO = {('sra', 'srl'), ('srl', 'sra'), ('slt', 'sltu'), ('sltu', 'slt'),
         ('slti', 'sltiu'), ('sltiu', 'slti'), ('lh', 'lhu'), ('lhu', 'lh'),
         ('lb', 'lbu'), ('lbu', 'lb'), ('srawi', 'srwi'), ('srwi', 'srawi')}


def partes(ins):
    ins = ins.strip()
    if not ins:
        return None, []
    m = ins.split(None, 1)
    ops = [o.strip() for o in m[1].split(',')] if len(m) > 1 else []
    return m[0], ops


def cajon(orig, nuestro):
    mo, oo = partes(orig)
    mn, on = partes(nuestro)
    if mo is None or mn is None:
        return 'de mas/menos'
    if (mo, mn) in SIGNO:
        return 'signo'
    if mo != mn:
        return 'otro mnemonico'
    if mo in CONMUT and len(oo) == 3 and len(on) == 3 and oo[0] == on[0] \
            and oo[1] == on[2] and oo[2] == on[1]:
        return 'conmutativa'
    if len(oo) == len(on):
        dif = [i for i in range(len(oo)) if oo[i] != on[i]]
        if len(dif) == 1:
            a, b = oo[dif[0]], on[dif[0]]
            ma = re.match(r'(-?0x[0-9a-f]+|-?\d+)\((\w+)\)$', a)
            mb = re.match(r'(-?0x[0-9a-f]+|-?\d+)\((\w+)\)$', b)
            if ma and mb and ma.group(2) == mb.group(2):
                return 'desplaz.'
            if re.match(r'-?(0x[0-9a-f]+|\d+)$', a) and re.match(r'-?(0x[0-9a-f]+|\d+)$', b):
                return 'constante'
            return 'registro'
    return 'otro'


def main():
    filas, cur = [], None
    for linea in open(sys.argv[1], encoding='utf-8', errors='replace'):
        m = RE_FILA.match(linea)
        if m:
            cur = [int(m.group(1)), int(m.group(2)), m.group(4), m.group(5), []]
            filas.append(cur)
            continue
        m = RE_INS.match(linea)
        if cur is not None and m and '|' in linea:
            cur[4].append((m.group(2), m.group(3)))

    por_cajon = collections.defaultdict(list)
    for n, tam, unidad, sim, det in filas:
        cajones = set(cajon(a, b) for a, b in det) or {'?'}
        # Una funcion va al cajon de su diferencia si es de UN solo tipo.
        clave = cajones.pop() if len(cajones) == 1 else 'mezcla'
        por_cajon[clave].append((n, tam, unidad, sim, det))

    orden = ('conmutativa', 'constante', 'signo', 'desplaz.', 'de mas/menos',
             'registro', 'otro mnemonico', 'otro', 'mezcla', '?')
    for c in orden:
        lista = por_cajon.get(c, [])
        if not lista:
            continue
        print('=== %s: %d funciones, %d B ===' % (c, len(lista), sum(x[1] for x in lista)))
        for n, tam, unidad, sim, det in sorted(lista, key=lambda x: (x[0], -x[1])):
            print('  %d %6d  %-18s %s' % (n, tam, unidad, sim[:54]))
            for a, b in det[:2]:
                print('          %-38s | %s' % (a[:38], b[:38]))
        print()


if __name__ == '__main__':
    main()
