"""rapido.py (cwd = raiz del repo) V UNIDAD REL COPIA SIM[,SIM] [--antes=REL,REL] [--sombra=DIR] -- banco minimo de PC/X360: una SourceList
que solo incluye (las --antes y) COPIA, contra el objeto original de UNIDAD; imprime el porcentaje de los SIM (por
subcadena). Para variantes de una funcion; lo bueno se confirma con bancopc.py de la unidad entera."""
import json, os, subprocess, sys, uuid
args = [a for a in sys.argv[1:] if not a.startswith('--')]
v, unidad, rel, copia, sims = args
op = dict(a[2:].split('=', 1) for a in sys.argv[1:] if a.startswith('--') and '=' in a)
et = 'r' + uuid.uuid4().hex[:8]
os.makedirs(os.path.join('build', 'bancopc'), exist_ok=True)
lista = os.path.join('build', 'bancopc', 'lista_%s.cpp' % et)
inc = [r for r in op.get('antes', '').split(',') if r]
with open(lista, 'w') as fh:
    for r in inc:
        fh.write('#include "%s"\n' % r)
    fh.write('#include "%s"\n' % rel)
cmd = [sys.executable, 'scripts/bancopc.py', v, unidad, et, '--lista=' + lista, '--sust=%s=%s' % (rel, copia)]
if 'sombra' in op:
    cmd.append('--sombra=' + op['sombra'])
r = subprocess.run(cmd, capture_output=True, text=True)
os.remove(lista)
p = 'build/bancopc/%s_%s_%s.json' % (v, unidad, et)
if not os.path.exists(p):
    print(r.stdout[-1500:])
    sys.exit(1)
d = json.load(open(p))
os.remove(p)
for s in sims.split(','):
    for k, x in d.items():
        if s in k:
            print('%-8s %s %s' % (x[0], x[1], k[:100]))
