import json, subprocess, sys, os
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
SCR = os.environ.get('SCRATCH', os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'build', 'scratch'))
os.makedirs(SCR, exist_ok=True)
# La version se elige con V=... (por defecto GameCube, que es como
# nacio esto). Para PS2: V=SLES-53558-A124 python scripts/fndiff.py ...
V=os.environ.get('V','GOWE69')
unit=sys.argv[1]; sym=sys.argv[2]
lim=int(sys.argv[3]) if len(sys.argv)>3 else 2000
# El nombre suelto es una SourceList; con '/' es una ruta relativa a
# build/{V}/{obj,src} -- hacen falta para las 184 unidades que NO son
# SourceLists (LibSN/steering, libc/*, Libs/snd/*...).
# Nombre suelto -> SourceList; con '/' -> ruta relativa. Y si la SourceList no
# existe, el nombre suelto ES la ruta (hay unidades en la raiz: FSasync, vmbase).
rel = unit if '/' in unit else f'Speed/Indep/SourceLists/{unit}'
# X360 y PC usan .obj de MSVC, no .o; el binario vive en build/tools.
EXT = '.obj' if V in ('EUROPEGERMILESTONE', 'SPEED_EXE_1_3') else '.o'
CLI = f'{ROOT}/build/tools/objdiff-cli.exe'
if not os.path.exists(CLI):
    CLI = f'{ROOT}/objdiff-cli-windows-x86_64.exe'
if not os.path.exists(f'{ROOT}/build/{V}/obj/{rel}{EXT}') and os.path.exists(f'{ROOT}/build/{V}/obj/{unit}{EXT}'):
    rel = unit
out=f'{SCR}/d_{V}_{unit.replace(chr(47),"_")}.json'
if os.environ.get('REUSE')!='1' or not os.path.exists(out):
    r=subprocess.run([CLI,'diff',
      '-1',f'{ROOT}/build/{V}/obj/{rel}{EXT}',
      '-2',f'{ROOT}/build/{V}/src/{rel}{EXT}',
      '-c','function_reloc_diffs=none','-c','ppc.calculatePoolRelocations=false','-o',out,'--format','json',sym],capture_output=True,text=True)
    if r.returncode!=0:
        print(r.stdout[-2000:]); print(r.stderr[-2000:]); sys.exit(1)
d=json.load(open(out))
def get(side):
    for s in d[side]['symbols']:
        if s.get('name')==sym: return s
    return None
A=get('left'); B=get('right')
if A is None or B is None:
    print('not found', A is None, B is None); sys.exit(1)
LI=A.get('instructions',[]); RI=B.get('instructions',[])
# el aviso de truncamiento iba al FINAL, detras de 2.000 lineas: nadie lo veia
# porque todo el mundo corta con head. Va en la cabecera.
_n=max(len(LI),len(RI))
_tr=('  *** TRUNCADO a %d de %d instrucciones: repite con  fndiff.py %s %s %d ***'
     % (lim,_n,unit,sym,_n+100)) if _n>lim else ''
print('# %s  target=%s%%  ours=%s%%  size=%s/%s%s'%(sym,A.get('match_percent'),B.get('match_percent'),A.get('size'),B.get('size'),_tr))
def fmt(e):
    if e is None: return ('','')
    i=e.get('instruction') or {}
    return (i.get('formatted','').strip(), e.get('diff_kind','') or '')
n=max(len(LI),len(RI))
for k in range(min(n,lim)):
    a=LI[k] if k<len(LI) else None
    b=RI[k] if k<len(RI) else None
    ta,ka=fmt(a); tb,kb=fmt(b)
    kk=''
    if ka not in ('','DIFF_NONE'): kk+=ka.replace('DIFF_','')
    if kb not in ('','DIFF_NONE'): kk+='/'+kb.replace('DIFF_','')
    mark='>>>' if kk else '   '
    print('%s %4d %-48s | %-48s %s'%(mark,k,ta[:48],tb[:48],kk))
if n>lim: print('...truncated',n)
