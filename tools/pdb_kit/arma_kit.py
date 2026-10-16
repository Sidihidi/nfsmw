"""arma_kit.py SALIDA_INGLES -- arma dist/nfsmw-pc-pdb-kit.zip (todo en ingles) con lo generado por
simbolos_pc.py --english --source-root C:\\nfsmw, los scripts en ingles de en/scripts y los del repo."""
import hashlib
import shutil
import sys
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
AQUI = Path(__file__).resolve().parent
simbolos = Path(sys.argv[1])
kit = AQUI / 'kit' / 'nfsmw-pc-pdb-kit'
if kit.exists():
    shutil.rmtree(kit)
(kit / 'symbols').mkdir(parents=True)
for f in ('NfsMWCollectorsRelease.pdb', 'speed.map', 'symbols_pc.json', 'symbols_pc.csv', 'lines_pc.json',
          'lines_pc.csv', 'ida_import_symbols_pc.py', 'ImportSymbolsPC.java', 'speed.exe.dd32'):
    shutil.copy2(simbolos / f, kit / 'symbols' / f)
(kit / 'repo' / 'scripts').mkdir(parents=True)
for f in ('make_pdb.py', 'name_by_call_graph.py', 'name_by_elimination.py', 'name_corrections.py'):
    shutil.copy2(AQUI / 'scripts' / f, kit / 'repo' / 'scripts' / f)
shutil.copy2(REPO / 'scripts' / 'simbolos_pc.py', kit / 'repo' / 'scripts' / 'simbolos_pc.py')
for rel in ('config/SPEED_EXE_1_3/symbols.json', 'config/SPEED_EXE_1_3/nombres_procedencia.tsv',
            'config/SPEED_EXE_1_3/splits.json', 'config/SPEED_EXE_1_3/config.yml'):
    (kit / 'repo' / rel).parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(REPO / rel, kit / 'repo' / rel)
(kit / 'docs').mkdir()
shutil.copy2(REPO / 'docs' / 'PC_PDB.md', kit / 'docs' / 'PC_PDB.md')
shutil.copy2(REPO / 'LICENSE', kit / 'LICENSE')
shutil.copy2(AQUI / 'plantilla' / 'README.md', kit / 'README.md')
shutil.copy2(AQUI / 'plantilla' / 'NOTICE.md', kit / 'NOTICE.md')

ficheros = sorted(p for p in kit.rglob('*') if p.is_file())
lineas = []
for p in ficheros:
    lineas.append('%s  %s' % (hashlib.sha256(p.read_bytes()).hexdigest(), p.relative_to(kit).as_posix()))
(kit / 'SHA256SUMS.txt').write_text('\n'.join(lineas) + '\n', encoding='utf-8', newline='\n')

destino = REPO / 'dist' / 'nfsmw-pc-pdb-kit.zip'
destino.parent.mkdir(exist_ok=True)
if destino.exists():
    destino.unlink()
with zipfile.ZipFile(destino, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for p in sorted(q for q in kit.rglob('*') if q.is_file()):
        z.write(p, 'nfsmw-pc-pdb-kit/' + p.relative_to(kit).as_posix())
print('zip:', destino, destino.stat().st_size)
with zipfile.ZipFile(destino) as z:
    for i in z.infolist():
        print('%12d %12d  %s' % (i.file_size, i.compress_size, i.filename))
