#!/usr/bin/env python3
"""foto.py [--rama avance-AAAA-MM-DD] [--sobre <commit>] -- prepara una FOTO LIMPIA del estado actual para subirla
a GitHub en una rama propia, sin el historial local. NO sube nada: crea la rama local y dice el push.

POLITICA (decidida por el usuario el 2026-09-18): main del fork sigue a upstream; el trabajo propio se publica en
ramas avance-*, y la foto se deja LO MAS PARECIDA POSIBLE AL REPO ORIGINAL:
  - en la raiz, solo lo que tiene upstream (src, config, tools, symbols, include, orig, docs...), con nuestra
    decompilacion dentro. Fuera port/, scripts/, ghidra_scripts/, temporales y basura;
  - en docs/, solo los documentos de upstream (fuera docs/analisis, docs/congelado y los nuestros);
  - fuera tools/scratch/ (trabajo temporal de rondas).
EXCEPCION (decidida por el usuario el 2026-09-18, para compartir el trabajo con la gente de la decompilacion):
  - scripts/ entero: nuestras herramientas. Antes se quitaron de todos sus ficheros las rutas de la maquina local
    (la raiz se calcula desde el propio script y el temporal es $SCRATCH o build/scratch);
  - docs/BEYOND_UPSTREAM.md: la guia en ingles de lo que este fork aporta sobre upstream.
DNAS (decidido por el usuario el 2026-09-30): la frase de paso de BuildRegion::GetDnasPassPhrase se publica con
ceros; si queda en cualquier fichero de la foto, en cualquier forma, no se crea.

Por que es tan estricta: la primera foto (avance-2026-09-18) PUBLICO MATERIAL DEL JUEGO. port/android/app/src/
main/assets/ llevaba tres .BUN (paquetes de datos del juego), ocho .fng (la interfaz del frontend) y texturas
extraidas de los tpk (FONT_MW_TITLE, MwSplashBack...). La rama se borro de GitHub. Y el historial local tampoco se
puede subir: arrastra un 7z de 108 MB que entro por accidente el 2026-08-19 (e62ee767, quitado en 8d875e8e).

Comprobaciones antes de crear nada:
  - que el arbol no este vacio ni corto (una comprobacion sobre un arbol vacio no comprueba nada);
  - ni un fichero de mas de 100 MB (GitHub los rechaza);
  - ni formatos del juego por EXTENSION (.bun .fng .tpk .tga .dxt* .lzc .big .ast .abk .mus .asf .vp6 .xpr .bnk
    .elf .dol .iso .xex .exe .bin...) ni por FIRMA BINARIA (7z, rar, zip, ELF, PE, DDS, RIFF, Yaz0, XEX, TPL,
    BIG de EA), que no se deja enganar por un nombre;
  - ni secretos con forma de token.

OJO al elegir el padre (--sobre): la foto NO puede colgar de una foto que tuviera material prohibido, o ese commit
seguiria siendo accesible desde la rama nueva. Por defecto va sobre la ultima rama avance-* del remoto fork.

    python scripts/foto.py                        # sobre la ultima foto publicada
    python scripts/foto.py --sobre 59f184a0       # sobre el punto donde nos separamos de upstream
    python scripts/foto.py --prueba               # todas las comprobaciones, sin crear commit ni rama"""
import datetime
import json
import os
import re
import subprocess
import sys


def git(*a, entrada=None, env=None, binario=False):
    # La entrada va en BYTES. En modo texto, Python en Windows convierte '\n' en '\r\n' al escribirla, y
    # `git update-index --index-info` ignora entonces TODAS las rutas ("Ignoring path ...") y termina con
    # codigo 0: el indice queda vacio sin un solo error. Asi salio en la primera prueba un arbol vacio
    # (4b825dc6...) que ademas pasaba todas las comprobaciones, porque en un arbol vacio no hay nada prohibido.
    datos = entrada if isinstance(entrada, bytes) else (entrada.encode('utf-8') if entrada is not None else None)
    r = subprocess.run(['git'] + list(a), capture_output=True, input=datos, env=env)
    if r.returncode:
        sys.exit('git %s fallo:\n%s' % (' '.join(a[:3]), r.stderr.decode('utf-8', 'replace').strip()))
    return r.stdout if binario else r.stdout.decode('utf-8', 'replace').strip()


args = sys.argv[1:]
if args and args[0] in ('-h', '--help'):
    print(__doc__)
    sys.exit(0)
RAMA = 'avance-' + datetime.date.today().isoformat()
SOBRE = None
if '--rama' in args:
    RAMA = args[args.index('--rama') + 1]
if '--sobre' in args:
    SOBRE = args[args.index('--sobre') + 1]
PRUEBA = '--prueba' in args

git('fetch', 'upstream', '--quiet')
if SOBRE is None and not PRUEBA:
    git('fetch', 'fork', '--quiet')
    ramas = [l.split('refs/heads/')[1] for l in git('ls-remote', '--heads', 'fork', 'avance-*').split('\n') if l]
    if not ramas:
        sys.exit('no hay ninguna foto anterior en fork: usa --sobre <commit>')
    ultima = sorted(ramas)[-1]
    SOBRE = git('rev-parse', 'fork/' + ultima)
    print('foto anterior: fork/%s (%s)' % (ultima, SOBRE[:10]))

# 1. el filtro "como upstream"
raiz_up = set(git('ls-tree', '--name-only', 'upstream/main').split('\n'))
docs_up = set(git('ls-tree', '-r', '--name-only', 'upstream/main', '--', 'docs').split('\n'))


# la excepcion: lo que se publica aunque upstream no lo tenga
EXTRA_DIRECTORIOS = ('scripts/',)
EXTRA_FICHEROS = {'docs/BEYOND_UPSTREAM.md', 'docs/PC_PDB.md'}  # PC_PDB.md: decision del usuario 2026-10-08


def se_queda(ruta):
    if ruta.startswith('tools/scratch/'):
        return False
    if ruta in EXTRA_FICHEROS or ruta.startswith(EXTRA_DIRECTORIOS):
        return True
    if ruta.split('/', 1)[0] not in raiz_up:
        return False
    if ruta.startswith('docs/') and ruta not in docs_up:
        return False
    return True


# DNAS (decidido por el usuario el 2026-09-30): BuildRegion::GetDnasPassPhrase lleva la frase de paso de 8 bytes de
# DNAS de cada region. El repo privado la conserva (la funcion casa); la foto la publica con ceros, y esa funcion deja
# de casar en las dos PS2 solo en la foto.
DNAS_FICHERO = 'src/Speed/Indep/Src/Misc/BuildRegion.cpp'
DNAS_FUNCION = re.compile(r'GetDnasPassPhrase\(\)\s*\{.*?\n\}\r?\n', re.S)
DNAS_LISTA = re.compile(r'\{(?:\s*0x[0-9A-Fa-f]{2}\s*,){7}\s*0x[0-9A-Fa-f]{2}\s*\}')
FRASES_DNAS = []


def sin_dnas(texto):
    m = DNAS_FUNCION.search(texto)
    if not m:
        sys.exit('NO SE CREA LA FOTO: no encuentro GetDnasPassPhrase en %s para taparla' % DNAS_FICHERO)
    cuerpo = m.group(0)
    for lista in DNAS_LISTA.findall(cuerpo):
        FRASES_DNAS.append(bytes(int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})', lista)))
    nuevo, n = DNAS_LISTA.subn('{0, 0, 0, 0, 0, 0, 0, 0}', cuerpo)
    if n != 2:
        sys.exit('NO SE CREA LA FOTO: esperaba 2 frases de DNAS en %s y hay %d' % (DNAS_FICHERO, n))
    return texto[:m.start()] + nuevo + texto[m.end():]


lineas = []
fuera = {}
for l in git('ls-tree', '-r', 'HEAD').split('\n'):
    if not l:
        continue
    ruta = l.split('\t', 1)[1]
    if ruta == DNAS_FICHERO:
        modo, tipo, h = l.split('\t', 1)[0].split()
        texto = git('cat-file', 'blob', h, binario=True).decode('utf-8')
        h = git('hash-object', '-w', '--stdin', entrada=sin_dnas(texto).encode('utf-8'))
        l = '%s %s %s\t%s' % (modo, tipo, h, ruta)
    if se_queda(ruta):
        lineas.append(l)
    else:
        clave = ruta.split('/', 1)[0] if not ruta.startswith(('docs/', 'tools/')) else '/'.join(ruta.split('/')[:2])
        fuera[clave] = fuera.get(clave, 0) + 1

idx = os.path.join(os.environ.get('TEMP', '/tmp'), 'foto_limpia.idx')
if os.path.exists(idx):
    os.remove(idx)
env = dict(os.environ, GIT_INDEX_FILE=idx)
git('update-index', '--index-info', entrada='\n'.join(lineas) + '\n', env=env)
arbol = git('write-tree', env=env)
os.remove(idx)

# 2. comprobaciones
en_arbol = git('ls-tree', '-r', '-l', arbol).split('\n')
if len([l for l in en_arbol if l]) != len(lineas):
    sys.exit('NO SE CREA LA FOTO: el arbol tiene %d ficheros y se le pidieron %d' % (len(en_arbol), len(lineas)))

PROHIBIDA = re.compile(r'\.(bun|fng|tpk|tga|dxt[1-5]|lzc|big|ast|abk|mus|asf|vp6|xpr|bnk|sng|elf|dol|iso|xex|exe|bin|'
                       r'7z|rar)$', re.I)
FIRMAS = [(b'7z\xbc\xaf\x27\x1c', '7z'), (b'Rar!', 'rar'), (b'\x7fELF', 'ELF'), (b'MZ', 'ejecutable PE'),
          (b'DDS ', 'textura DDS'), (b'RIFF', 'RIFF'), (b'Yaz0', 'Yaz0'), (b'XEX2', 'XEX'),
          (b'\x00\x20\xaf\x30', 'textura TPL'), (b'BIGF', 'BIG de EA'), (b'SCHl', 'audio de EA')]
malos, blobs = [], []
for l in en_arbol:
    p = l.split(None, 4)
    if len(p) < 5:
        continue
    tam, ruta, h = p[3], p[4], p[2]
    blobs.append((h, ruta))
    if tam.isdigit() and int(tam) > 100 * 1024 * 1024:
        malos.append('mas de 100 MB: %s (%d MB)' % (ruta, int(tam) // 1048576))
    if PROHIBIDA.search(ruta):
        malos.append('formato prohibido: %s' % ruta)
    if ruta == 'Need' or ruta.endswith('/Need'):
        malos.append('el 7z de 108 MB: %s' % ruta)
# firma binaria de todos los blobs, con un solo proceso
# Y los datos incrustados COMO TEXTO: la foto avance-2026-09-18-limpia publico src/Speed/GameCube/Src/globala_bun.inc,
# los 89.716 bytes del paquete de recursos global escritos como `0x..,` en un fichero de texto, que ni la extension ni
# la firma binaria veian. Se rechaza todo texto con mas de 4 KB de bytes en hexadecimal que no sea identico a un
# fichero de upstream (las tablas de los SDK, como axart3d.c, vienen de alli).
blobs_upstream = {l.split()[2] for l in git('ls-tree', '-r', 'upstream/main').split('\n') if l}
HEX = re.compile(rb'0x[0-9a-fA-F]{2}\s*,')
r = subprocess.run(['git', 'cat-file', '--batch'], input=('\n'.join(h for h, _ in blobs) + '\n').encode(),
                   capture_output=True)
datos, pos, i = r.stdout, 0, 0
while pos < len(datos) and i < len(blobs):
    fin = datos.index(b'\n', pos)
    cab = datos[pos:fin].split()
    n = int(cab[2])
    cuerpo = datos[fin + 1:fin + 1 + min(n, 8)]
    firma = next((nom for f, nom in FIRMAS if cuerpo.startswith(f)), None)
    # el zip del gradle wrapper es una herramienta de compilacion, no material del juego; aun asi aqui no hay
    if firma:
        malos.append('firma de %s: %s' % (firma, blobs[i][1]))
    if n > 4096 and blobs[i][0] not in blobs_upstream:
        nhex = len(HEX.findall(datos[fin + 1:fin + 1 + n]))
        if nhex > 4096:
            malos.append('datos incrustados como texto (%d bytes en hexadecimal): %s' % (nhex, blobs[i][1]))
    pos = fin + 1 + n + 1
    i += 1
SECRETO = r'(ghp_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{30,}|sk-[A-Za-z0-9]{32,}|AKIA[0-9A-Z]{16}|-----BEGIN [A-Z ]*PRIVATE KEY-----)'
r = subprocess.run(['git', 'grep', '-lIE', SECRETO, arbol], capture_output=True)
malos += ['posible secreto: %s' % l.split(':', 1)[-1] for l in r.stdout.decode('utf-8', 'replace').split('\n') if l]
# rutas de la maquina local en lo que se publica: no son secretos, pero rompen los scripts en otra maquina y
# publican el usuario local. Se buscan en los ficheros de la EXCEPCION, que son los que no venian de upstream.
r = subprocess.run(['git', 'grep', '-lIiE', r'(Users|home)[\\/][A-Za-z0-9_.-]+[\\/](Desktop|AppData)', arbol, '--']
                   + list(EXTRA_DIRECTORIOS) + sorted(EXTRA_FICHEROS), capture_output=True)
malos += ['ruta de la maquina local: %s' % l.split(':', 1)[-1]
          for l in r.stdout.decode('utf-8', 'replace').split('\n') if l]
# la frase de DNAS no puede quedar en ningun fichero publicado, en ninguna forma
if not FRASES_DNAS:
    malos.append('no se tapo la frase de DNAS (%s no esta en HEAD o cambio de forma)' % DNAS_FICHERO)
for f in FRASES_DNAS:
    patrones = [',[[:space:]]*'.join('0x%02x' % b for b in f), f.hex(), '[[:space:]]'.join('%02x' % b for b in f)]
    r = subprocess.run(['git', 'grep', '-lIiE', '|'.join(patrones), arbol], capture_output=True)
    malos += ['frase de DNAS sin tapar: %s' % l.split(':', 1)[-1]
              for l in r.stdout.decode('utf-8', 'replace').split('\n') if l]
if malos:
    print('NO SE CREA LA FOTO:')
    for m in malos:
        print('   ' + m)
    sys.exit(1)
extra = [l for l in lineas if se_queda(l.split('\t', 1)[1]) and
         (l.split('\t', 1)[1] in EXTRA_FICHEROS or l.split('\t', 1)[1].startswith(EXTRA_DIRECTORIOS))]
if PRUEBA:
    print('PRUEBA: la foto se podria crear. %d ficheros, sin nada prohibido (extension, firma binaria, tamano,'
          ' secretos, rutas locales) y con la frase de DNAS tapada (%d frases).' % (len(lineas), len(FRASES_DNAS)))
    print('de la excepcion: %d ficheros (%s)' % (len(extra), ', '.join(list(EXTRA_DIRECTORIOS) + sorted(EXTRA_FICHEROS))))
    faltan = sorted(f for f in EXTRA_FICHEROS if not any(l.endswith('\t' + f) for l in lineas))
    if faltan:
        print('AVISO: aun no existen en HEAD: %s' % ', '.join(faltan))
    print('fuera de la foto, por no estar en upstream:')
    for k, n in sorted(fuera.items(), key=lambda x: -x[1])[:14]:
        print('   %5d  %s' % (n, k))
    sys.exit(0)

# 3. el commit
estado = ''
for v in ('GOWE69', 'SLES-53558-A124', 'SLUS-21351', 'EUROPEGERMILESTONE', 'SPEED_EXE_1_3'):
    p = os.path.join('build', 'versiones', v + '.json')
    if os.path.exists(p):
        m = json.load(open(p)).get('measures', {})
        estado += '    %-20s %6.2f %%  %s funciones\n' % (v, float(m.get('matched_code_percent', 0)),
                                                     m.get('matched_functions'))
msg = ('Avance de la decompilacion hasta el %s\n\n'
       'Foto del estado del proyecto en un solo commit, sin el historial local y con la misma estructura que el\n'
       'repo original: solo lo que upstream tiene en su raiz y en docs/, con nuestra decompilacion dentro.\n'
       'main sigue a upstream; esta rama es la linea de trabajo propia.\n\n'
       'Ultima medida (objdiff por version):\n\n%s'
       % (datetime.date.today().isoformat(), estado or '    (sin informes en build/versiones)\n'))
commit = git('commit-tree', arbol, '-p', SOBRE, entrada=msg)
git('branch', '-f', RAMA, commit)
print('foto creada: rama %s = %s, sobre %s' % (RAMA, commit[:10], SOBRE[:10]))
print('ficheros: %d, sin nada prohibido (extension, firma binaria, tamano, secretos).' % len(lineas))
print('fuera de la foto, por no estar en upstream:')
for k, n in sorted(fuera.items(), key=lambda x: -x[1])[:14]:
    print('   %5d  %s' % (n, k))
print('\nPara publicarla (lo decide una persona):\n    git push fork %s' % RAMA)
