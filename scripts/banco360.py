#!/usr/bin/env python3
"""banco360.py --dir DIR UNIDAD[,UNIDAD] --fn F[,F] [--sombra REL=COPIA ...] [--il OBJ,...] [--bisec I0:I1] [--egm]
-- banco del enlace LTCG de Xbox 360 con SOLO unas unidades (ronda 85, X360-B; de solo.py/variante.py/bisec.py de
la ronda 84).

Compila con /GL las SourceLists dadas (las cflags de configure.py, como scripts/lt360.py) y las enlaza con /LTCG
SOLAS: lo que falta lo ponen stubs propios (ltcg360.escribe_stubs) hasta que no queda nada sin resolver. Tarda
10-40 s y reproduce las decisiones que LTCG toma dentro de esas unidades (que constante se carga de un objeto y
cual es un literal, que se saca del bucle, la fusion de un literal con un dato de solo lectura del mismo valor);
lo que depende del programa entero (donde cae cada dato, la base comun del pool) no: para eso, lt360.py.
Desensambla las funciones cuyo nombre contiene F, con cada carga anotada (direccion, seccion y valor), cuenta los
inicializadores dinamicos (??__E) y, con --egm, saca tambien la funcion del original (symbols.txt) anotada igual.

  --sombra REL=COPIA  COPIA hace de src/REL (una cabecera o un .cpp con un cambio a probar; va delante de las demas
                      rutas de include, y la carpeta del original detras para sus includes relativos). Repetible.
  --il OBJ,...        IL ya compilados que se enlazan tambien (p. ej. copias de build/EUROPEGERMILESTONE/ltcg/il)
  --fuente COPIA      la (unica) UNIDAD se compila desde COPIA en vez de su SourceList (con la carpeta de la
                      SourceList en los include): un #include o una guarda de la SourceList cambiados
  --bisec I0:I1       la (unica) UNIDAD se compila solo con los .cpp de indices [I0, I1) de su SourceList (las
                      cabeceras de delante siempre): biseccion de que unidad cambia una decision de LTCG
  --egm               la funcion del EGM (orig/EUROPEGERMILESTONE/NFS.exe) al lado

Ejemplo (ronda 85: por que Util_InitTrigTable relee 2*pi en cada vuelta):

    python scripts/banco360.py --dir D:/tmp/b1 zSim --fn Util_InitTrigTable --egm \\
        --sombra Speed/Indep/Src/Sim/Common/Util.cpp=D:/tmp/Util_variante.cpp
"""
import argparse
import os
import re
import shutil
import struct
import subprocess
import sys

import capstone

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ltcg360  # noqa: E402
import lt360  # noqa: E402

V = 'EUROPEGERMILESTONE'
MD = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)
CARGA = re.compile(r'(-?(?:0x)?[0-9a-f]+)\((r\d+)\)$')


def anota(pe, base, codigo, va0):
    """Lineas de desensamblado; las cargas/guardados con base fijada por lis(/addi) llevan direccion, seccion y,
    si es lfs, el valor."""
    his, out = {}, []
    for ins in MD.disasm(codigo, va0):
        ops = [x.strip() for x in ins.op_str.split(',')]
        nota = ''
        if ins.mnemonic == 'lis':
            his[ops[0]] = (int(ops[1], 0) << 16) & 0xffffffff
        elif ins.mnemonic == 'addi' and len(ops) == 3 and ops[1] in his:
            his[ops[0]] = (his[ops[1]] + int(ops[2], 0)) & 0xffffffff
        m = CARGA.match(ops[-1]) if ops and ins.mnemonic[:1] in ('l', 's') and ins.mnemonic != 'lis' else None
        if m and m.group(2) in his:
            va = (his[m.group(2)] + int(m.group(1), 0)) & 0xffffffff
            rva = va - base
            sec = [s[0] for s in pe.secs if s[1] <= rva < s[1] + max(s[2], 1)]
            val = ''
            if ins.mnemonic == 'lfs':
                val = '%.8g' % struct.unpack('>f', pe.lee(rva, 4))[0]
            nota = '  ; %08X %s %s' % (va, sec[0] if sec else '?', val)
        out.append('   %08X %-8s %-26s%s' % (ins.address, ins.mnemonic, ins.op_str, nota))
    return out


def egm(fns):
    pe = ltcg360.PE(os.path.join('orig', V, 'NFS.exe'))
    base = ltcg360.ORIG_BASE
    for linea in open(os.path.join('config', V, 'symbols.txt'), encoding='utf-8'):
        m = re.match(r'(\S+) = \.text:0x([0-9A-F]+); // type:function size:0x([0-9A-F]+)', linea)
        if m and any(f in m.group(1) for f in fns):
            va, n = int(m.group(2), 16), int(m.group(3), 16)
            print('EGM', m.group(1), hex(va), n)
            print('\n'.join(anota(pe, base, pe.lee(va - base, n), va)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--dir', required=True)
    ap.add_argument('unidades')
    ap.add_argument('--fn', required=True)
    ap.add_argument('--sombra', action='append', default=[])
    ap.add_argument('--il', default='')
    ap.add_argument('--bisec', default='')
    ap.add_argument('--fuente', default='')
    ap.add_argument('--egm', action='store_true')
    a = ap.parse_args()
    raiz = os.getcwd()
    d = os.path.abspath(a.dir)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    unis = a.unidades.split(',')
    fns = a.fn.split(',')
    inc = []
    if a.sombra:
        som = os.path.join(d, 'sombra')
        dirs = []
        for s in a.sombra:
            rel, copia = s.split('=', 1)
            destino = os.path.join(som, rel)
            os.makedirs(os.path.dirname(destino), exist_ok=True)
            shutil.copy(copia, destino)
            dirs.append(os.path.dirname(os.path.join(raiz, 'src', rel)))
        inc = ['/I', som] + sum([['/I', x] for x in dirs], [])
    fl = lt360.cflags_configure(raiz, set(unis))
    ils = []
    for u in unis:
        if u not in fl:
            sys.exit('%s: configure.py no tiene Speed/Indep/SourceLists/%s para %s' % (u, u, V))
        cf, fuente = fl[u]
        if a.fuente:
            assert len(unis) == 1, '--fuente con una sola unidad'
            inc += ['/I', os.path.dirname(os.path.join(raiz, fuente))]
            fuente = a.fuente
        if a.bisec:
            assert len(unis) == 1, '--bisec con una sola unidad'
            i0, i1 = [int(x) for x in a.bisec.split(':')]
            lineas = open(os.path.join(raiz, fuente), encoding='utf-8').read().splitlines()
            cpps = [x for x in lineas if x.startswith('#include') and x.strip().endswith('.cpp"')]
            resto = [x for x in lineas if not (x.startswith('#include') and x.strip().endswith('.cpp"'))]
            fuente = os.path.join(d, u + '_bisec.cpp')
            open(fuente, 'w', encoding='utf-8').write('\n'.join(resto + cpps[i0:i1]) + '\n')
            inc += ['/I', os.path.dirname(os.path.join(raiz, fl[u][1]))]
            print('bisec %s [%d, %d) de %d: %s' % (u, i0, i1, len(cpps), ', '.join(
                x.split('/')[-1].rstrip('"') for x in cpps[i0:i1][:4])))
        o = os.path.join(d, u + '.obj')
        cmd = [os.path.join(raiz, lt360.CL)] + inc + cf.split() + ['/GL', '/Fo' + o, fuente.replace('/', '\\')]
        r = subprocess.run(cmd, capture_output=True, text=True, errors='replace', cwd=raiz)
        errs = [x for x in (r.stdout + r.stderr).splitlines() if ': error' in x or 'fatal error' in x]
        if errs or r.returncode:
            sys.exit('%s NO COMPILA:\n%s' % (u, '\n'.join(errs[:6])))
        ils.append(o)
    ils += [x for x in a.il.split(',') if x]
    n = os.path.join(d, 'b')
    stubs = {'main': 'F'}
    for vuelta in range(10):
        ltcg360.escribe_stubs(n + '_stubs.obj', stubs)
        p = subprocess.run([os.path.join(raiz, lt360.LINK), '/nologo', '/LTCG', '/XEX:NO', '/NODEFAULTLIB',
                            '/FORCE:MULTIPLE', '/OPT:NOREF', '/ENTRY:main', '/MAP:' + n + '.map', '/OUT:' + n + '.exe']
                           + ils + [n + '_stubs.obj'], capture_output=True, text=True, errors='replace')
        nuevos = {k: v for k, v in ltcg360.sin_resolver(p.stdout).items() if k not in stubs}
        if not nuevos:
            break
        stubs.update(nuevos)
    open(n + '.log', 'w', encoding='utf-8').write(p.stdout)
    if not os.path.exists(n + '.exe'):
        sys.exit('el enlace fallo (ver %s.log)' % n)
    pe = ltcg360.PE(n + '.exe')
    m = ltcg360.lee_mapa(n + '.map')
    print('enlace rc %d, %d stubs; inicializadores dinamicos (??__E): %d; __real: %d' % (
        p.returncode, len(stubs), sum(1 for x in m if x[1].startswith('??__E')),
        sum(1 for x in m if x[1].startswith('__real'))))
    fs = sorted({(r, nm) for r, nm, o, f, e in m if f})
    for i, (r, nm) in enumerate(fs):
        if any(f in nm for f in fns):
            fin = fs[i + 1][0] if i + 1 < len(fs) else r + 0x100
            print('NUESTRO', nm, hex(r + pe.base), fin - r)
            print('\n'.join(anota(pe, pe.base, pe.lee(r, min(fin - r, 0x1000)), r + pe.base)))
    if a.egm:
        egm(fns)


if __name__ == '__main__':
    main()
