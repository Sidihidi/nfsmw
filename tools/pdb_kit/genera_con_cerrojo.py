"""Genera, con el cerrojo de scripts/medir.py (ningun pctall reconstruye PC mientras tanto) y sobre el MISMO estado:
  A: simbolos_pc.py de HEAD (el de antes del modo ingles), modo por defecto, --raiz-fuente C:\\nfsmw
  B: simbolos_pc.py actual, modo por defecto, --raiz-fuente C:\\nfsmw          (A == B: el defecto no cambia)
  C: simbolos_pc.py actual, --english --source-root C:\\nfsmw --check          (el kit)
"""
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path

REPO = str(Path(__file__).resolve().parents[2])
sys.path.insert(0, REPO + r'\scripts')
import medir  # noqa: E402

QUIEN = 'PDBKIT'
base = Path(sys.argv[1])
historia = base / 'historia.json'
for d in ('A', 'B', 'C'):
    p = base / d
    if p.exists():
        shutil.rmtree(p)
    p.mkdir(parents=True)
shutil.copy2(historia, base / 'A' / '.historia.json')
shutil.copy2(historia, base / 'B' / '.historia.json')
shutil.copy2(historia, base / 'C' / '.history.json')
viejo = subprocess.run(['git', 'show', 'HEAD:scripts/simbolos_pc.py'], cwd=REPO, capture_output=True,
                       check=True).stdout.decode('utf-8')
(base / 'simbolos_pc_HEAD.py').write_text(viejo, encoding='utf-8')
lanza_viejo = ("import sys; src=open(r'%s', encoding='utf-8').read(); sys.argv=['simbolos_pc.py']+sys.argv[1:]; "
               "exec(compile(src, r'%s\\scripts\\simbolos_pc.py', 'exec'), "
               "{'__name__': '__main__', '__file__': r'%s\\scripts\\simbolos_pc.py'})"
               % (base / 'simbolos_pc_HEAD.py', REPO, REPO))
print('cogiendo el cerrojo...', flush=True)
medir.coger(QUIEN)
parar = threading.Event()
hilo = threading.Thread(target=medir.latir, args=(QUIEN, parar), daemon=True)
hilo.start()
try:
    print('cerrojo cogido', time.strftime('%H:%M:%S'), flush=True)
    for nombre, orden in (
            ('A', [sys.executable, '-c', lanza_viejo, '--salida', str(base / 'A'), '--raiz-fuente', 'C:\\nfsmw']),
            ('B', [sys.executable, REPO + r'\scripts\simbolos_pc.py', '--salida', str(base / 'B'), '--raiz-fuente',
                   'C:\\nfsmw']),
            ('C', [sys.executable, REPO + r'\scripts\simbolos_pc.py', '--english', '--output', str(base / 'C'),
                   '--source-root', 'C:\\nfsmw', '--check'])):
        r = subprocess.run(orden, cwd=REPO)
        print(nombre, 'codigo', r.returncode, time.strftime('%H:%M:%S'), flush=True)
finally:
    parar.set()
    hilo.join(5)
    medir.soltar()
