"""bancolib.py REL COPIA ETIQ [--cl=A,B] [--opt=/O1,/Oy-] [--orig=OBJ] [--base=ETIQ]

Banco de PC de UN fichero de biblioteca (realcore, snd...): compila COPIA (en lugar de src/REL) con los flags de
bancopc.py para SPEED_EXE_1_3 cambiando /O2 por --opt (por defecto /O1,/Oy-, los de realcore) mas --cl, y la compara
con objdiff contra el objeto original troceado build/SPEED_EXE_1_3/obj/REL.obj (o --orig). Guarda
build/bancolib/<ETIQ>.json {funcion: (pct, tam)} y con --base lista ganadas/perdidas."""
import json, os, subprocess, sys
AQUI = os.path.dirname(os.path.abspath(__file__))
R = os.path.dirname(AQUI)
sys.path.insert(0, os.path.join(R, 'scripts'))
import bancopc  # noqa: E402
import quita_L  # noqa: E402

args = [a for a in sys.argv[1:] if not a.startswith('--')]
rel, copia, etiq = args
op = dict(a[2:].split('=', 1) for a in sys.argv[1:] if a.startswith('--') and '=' in a)
SAL = os.path.join(R, 'build', 'bancolib')
os.makedirs(SAL, exist_ok=True)
fl = bancopc.flags('SPEED_EXE_1_3', None, [os.path.join(R, 'src', os.path.dirname(rel))])
fl = [f for f in fl if f not in ('/O2', '/EHsc' if 'sineh' in op else '')]
fl += op.get('opt', '/O1,/Oy-').split(',')
if op.get('cl'):
    fl += op['cl'].split(',')
obj = os.path.join(SAL, etiq + '.obj')
if os.path.exists(obj):
    os.remove(obj)
cl = os.path.join(R, 'build', 'compilers', 'Win32', '7.1', 'cl.exe')
r = subprocess.run([cl] + fl + ['/Fo' + obj, os.path.abspath(copia)], capture_output=True, text=True, cwd=SAL,
                   errors='replace')
if r.returncode or not os.path.exists(obj):
    print('NO COMPILA')
    print('\n'.join([x for x in (r.stdout + r.stderr).splitlines() if 'error' in x][:20]))
    sys.exit(1)
d = open(obj, 'rb').read()
try:
    q = quita_L.para_medir(d)
    if q:
        open(obj, 'wb').write(q[0])
except quita_L.NoSe as e:
    print('quita_L:', e)
orig = op.get('orig') or os.path.join(R, 'build', 'SPEED_EXE_1_3', 'obj', os.path.splitext(rel)[0] + '.obj')
sal = os.path.join(SAL, 'd_' + etiq + '.json')
r = subprocess.run([os.path.join(R, 'build', 'tools', 'objdiff-cli.exe'), 'diff', '-1', orig, '-2', obj,
                    '-c', 'functionRelocDiffs=none', '-c', 'ppc.calculatePoolRelocations=false',
                    '-o', sal, '--format', 'json'], capture_output=True, text=True)
if r.returncode:
    print('objdiff fallo', r.stderr[-500:])
    sys.exit(1)
dd = json.load(open(sal, encoding='utf-8'))
res = {}
for s in dd['left']['symbols']:
    if s.get('kind') == 'SYMBOL_FUNCTION':
        res[s.get('name', '')] = (s.get('match_percent'), int(s.get('size') or 0))
json.dump(res, open(os.path.join(SAL, etiq + '.json'), 'w'), indent=0)
cas = [n for n, (p, t) in res.items() if p == 100.0]
print('%s: %d al 100 %% (%d B) de %d' % (etiq, len(cas), sum(res[n][1] for n in cas), len(res)))
if op.get('base'):
    b = json.load(open(os.path.join(SAL, op['base'] + '.json')))
    for n in sorted(set(res) | set(b)):
        x, y = (b.get(n) or [None])[0], (res.get(n) or [None])[0]
        if x != y:
            print('  %-70s %s -> %s' % (n[:70], x, y))
if 'todas' in op:
    for n, (p, t) in sorted(res.items()):
        print('  %-70s %s %d' % (n[:70], p, t))
