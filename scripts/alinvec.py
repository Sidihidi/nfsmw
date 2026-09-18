#!/usr/bin/env python3
"""alinvec.py -- miembros vectoriales que en el PS2 original estan ALINEADOS A 16 y en nuestro fuente no llevan
ninguna alineacion. Recorre el volcado de tipos de la A124 (symbols/PS2/PS2_types.nothpp) y marca los miembros
de tipo Vector/bVector/Matrix que caen en un desplazamiento multiplo de 16 CON RELLENO DELANTE (el miembro
anterior acaba antes): esa es la huella de una alineacion forzada por miembro. Despues busca la declaracion en
src/ y dice si ya esta alineada.

La alineacion es por MIEMBRO, no por tipo: el original copia UMath::Vector4 sin alinear (ldl/ldr), y alinear
el tipo entero rompe mas de lo que arregla. Por eso se mira miembro a miembro. Asi salieron PVehicle::mLocalVel,
los tres bVector3 de VehiclePartDamageBehaviour y los seis Vector3 de SimpleChopper.

Dos cosas que la primera version hacia mal y dieron 165 "FALTA" inflados:
  - Cuenta como alineado CUALQUIER forma: ALIGNVEC, ALIGN_16 (types.h, que SI alinea en PS2: WRoadNav lo usa
    en sus 20 vectores y la sonda lo da identico al original), PS2ALIGN16 o el atributo directo.
  - Una declaracion solo vale si esta en un fichero que declara tambien ESA clase: el nombre de un miembro se
    repite entre clases (position, direction...) y casar solo por nombre daba la clase equivocada.
Y aun asi la heuristica falla mucho: el tamano de los miembros NO vectoriales lo estima, asi que tras un struct
grande ve un "relleno" que no existe. Por eso existe --verifica, que no se fia de nada: compila una sonda por
cabecera con los flags reales de SLES y compara el desplazamiento de cada FALTA con el del original. En la r82,
de los candidatos de las cabeceras que compilan, 57 eran falsos positivos y solo 14 reales. Poner ALIGNVEC a
todos a ciegas habria roto 57 layouts correctos.

    python scripts/alinvec.py              # los candidatos
    python scripts/alinvec.py --todos      # tambien los ya alineados
    python scripts/alinvec.py --verifica   # confirma con la sonda: REAL / OK (falso positivo)"""
import os
import re
import sys

TODOS = '--todos' in sys.argv
if any(a in ('-h', '--help') for a in sys.argv[1:]):
    print(__doc__)
    sys.exit(0)

VEC = re.compile(r'\b(b?Vector[234]|Matrix4|bMatrix4|Quaternion|bQuaternion)\b')
TAM = {'Vector2': 8, 'bVector2': 8, 'Vector3': 12, 'bVector3': 12, 'Vector4': 16, 'bVector4': 16,
       'Matrix4': 64, 'bMatrix4': 64, 'Quaternion': 16, 'bQuaternion': 16}
MIEMBRO = re.compile(r'/\* (0x[0-9a-f]+) \*/ (.+?)\s+\**(\w+)(\[(\d+)\])?;')
DECL = re.compile(r'\b(?:UMath::)?(b?Vector[234]|Matrix4|bMatrix4)\s+(\w+)(\[\d+\])?;')
ALINEADO = re.compile(r'ALIGNVEC|ALIGN_16|PS2ALIGN16|aligned\s*\(\s*16')

# 1. candidatos en el volcado: (clase, miembro, desplazamiento, tipo)
cand = []
clase, prev_fin = None, None
for l in open('symbols/PS2/PS2_types.nothpp', encoding='utf-8', errors='replace'):
    m = re.match(r'^struct (\S+)', l)
    if m:
        clase, prev_fin = m.group(1), None
        continue
    m = MIEMBRO.search(l)
    if not (m and clase):
        continue
    off, ty, nom = int(m.group(1), 16), m.group(2).strip(), m.group(3)
    v = VEC.search(ty)
    if v and off % 16 == 0 and prev_fin is not None and prev_fin < off:
        cand.append((clase, nom, off, v.group(1)))
    n = int(m.group(5)) if m.group(5) else 1
    prev_fin = off + TAM.get(v.group(1), 4) * n if v else off + 4 * n   # aproximado para lo que no es vector

# 2. las declaraciones en src/, con su fichero para poder exigir la clase
decl = {}
textos = {}
for r, _, ff in os.walk('src'):
    for f in ff:
        if not f.endswith(('.h', '.hpp', '.cpp')):
            continue
        p = os.path.join(r, f).replace(os.sep, '/')
        try:
            txt = open(p, encoding='utf-8', errors='replace').read()
        except Exception:
            continue
        textos[p] = txt
        for i, l in enumerate(txt.split('\n'), 1):
            m = DECL.search(l)
            if m:
                decl.setdefault(m.group(2), []).append((p, i, bool(ALINEADO.search(l))))


def declara_la_clase(p, clase):
    corto = clase.split('::')[-1].split('<')[0]
    return re.search(r'\b(class|struct)\s+' + re.escape(corto) + r'\b', textos.get(p, '')) is not None


VERIFICA = '--verifica' in sys.argv
faltan = ya = sin = 0
a_verificar = {}
for clase, nom, off, ty in sorted(set(cand)):
    ds = [d for d in decl.get(nom, []) if declara_la_clase(d[0], clase)]
    if not ds:
        sin += 1
        continue
    if all(a for _, _, a in ds):
        ya += 1
        if TODOS:
            print('  ya      %-34s %-26s 0x%03x' % (clase[:34], nom, off))
        continue
    faltan += 1
    p, i, _ = ds[0]
    if not VERIFICA:
        print('  FALTA   %-34s %-26s 0x%03x  %s:%d' % (clase[:34], nom, off, p.replace('src/Speed/Indep/', ''), i))
    elif p.endswith(('.h', '.hpp')) and '/GameCube/' not in p:
        a_verificar.setdefault(p[len('src/'):], []).append((clase, nom, off))
print('\n%d candidatos: %d sin alinear, %d ya alineados, %d sin declaracion en un fichero de su clase'
      % (len(set(cand)), faltan, ya, sin))
if not VERIFICA:
    print('Cada FALTA se confirma con --verifica antes de tocarlo.')
    sys.exit(0)

# --verifica: una sonda por cabecera (juntas no compilan), configurando SLES una sola vez
import subprocess
PY = sys.executable
subprocess.run([PY, 'configure.py', '--version', 'SLES-53558-A124', '--dtk', 'build/tools/dtk.exe',
                '--objdiff', 'build/tools/objdiff-cli.exe'], capture_output=True)
ln = open('build.ninja', encoding='utf-8', errors='replace').read().split('\n')
flags = None
for i, l in enumerate(ln):
    if l.startswith('build build\\SLES-53558-A124') and '\\SourceLists\\zAI.o: ee-gcc' in l:
        for j in range(i, i + 12):
            if ln[j].strip().startswith('cflags ='):
                t, k = ln[j].split('=', 1)[1], j
                while t.rstrip().endswith('$'):
                    k += 1
                    t = t.rstrip()[:-1] + ' ' + ln[k].strip()
                flags = t.split()
                break
        break
cc = os.path.normpath('build/compilers/PS2/ee-gcc2.9-991111/bin/ee-gcc.exe')
reales, oks, fallan = [], [], []
try:
    for cab, ms in sorted(a_verificar.items()):
        src, out = os.path.join('build', 'sonda_alinvec.cpp'), os.path.join('build', 'sonda_alinvec.s')
        l = ['#define private public', '#define protected public', '#include "%s"' % cab]
        l += ['int sonda_%03d = (int)&((%s *)0x1000)->%s - 0x1000;' % (n, c, m) for n, (c, m, o) in enumerate(ms)]
        open(src, 'w').write('\n'.join(l) + '\n')
        if os.path.exists(out):
            os.remove(out)
        subprocess.run([PY, os.path.normpath('tools/ee_gcc_pp.py'), os.path.normpath('build/SLES-53558-A124/pp'),
                        cc, '--'] + flags + ['-x', 'c++', '-S', '-o', out, src], capture_output=True)
        if not os.path.exists(out):
            fallan.append(cab)
            continue
        s = open(out, encoding='utf-8', errors='replace').read()
        val = {int(m.group(1)): (int(m.group(2), 16) if m.group(2).startswith('0x') else int(m.group(2)))
               for m in re.finditer(r'^sonda_(\d+):\s*\n\s*\.word\s+(-?\w+)', s, re.M)}
        for n, (c, m, o) in enumerate(ms):
            if n in val:
                (reales if val[n] != o else oks).append((c, m, o, val[n], cab))
finally:
    subprocess.run([PY, 'configure.py'], capture_output=True)
print('\nREALES (%d): nuestro desplazamiento distinto del original' % len(reales))
for c, m, o, n, cab in reales:
    tipo = 'nos falta relleno: ALIGNVEC' if n < o else 'nos SOBRA algo antes: otra causa'
    print('   %-26s %-24s orig 0x%03x  nuestro 0x%03x  %+4d  %s' % (c[:26], m, o, n, n - o, tipo))
print('OK, falsos positivos: %d.   Cabeceras que no compilan sueltas: %d' % (len(oks), len(fallan)))
