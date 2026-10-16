#!/usr/bin/env python3
"""trocea_prueba.py -- trocea una version de PS2 FUERA del arbol: copia su config.yml (o el que se le pase) cambiando
todas las rutas de salida a DIR, ejecuta splat sin cache, ensambla cada .s con el as del proyecto (las mismas opciones
que configure.py) y deja en DIR/unidades.json la lista de unidades.

    python scripts/trocea_prueba.py V DIR [--yml config.yml] [--symbols symbols.txt] [--sin-ensamblar]
                                          [--solo-ensamblar]

No escribe nada en el arbol: asm, obj, ldscript, undefined_*_auto y el yml van a DIR; los macro.inc/labels.inc son los
del arbol (include/), solo se leen. Lo usa scripts/raiz_prueba.py; un troceado de SLUS tarda unos 90 s.
"""
import concurrent.futures
import json
import os
import subprocess
import sys
import time

import yaml

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
AS = os.path.join(RAIZ, 'build', 'mips_binutils', 'mips-linux-gnu-as.exe')
ASFLAGS = ['-no-pad-sections', '-EL', '-march=5900', '-mabi=eabi', '-I', os.path.join(RAIZ, 'include')]


def prepara(V, D, yml=None, symbols=None):
    yml = yml or os.path.join(RAIZ, 'config', V, 'config.yml')
    cfg = yaml.safe_load(open(yml, encoding='utf-8'))
    o = cfg['options']
    o['base_path'] = RAIZ.replace('\\', '/')
    o['target_path'] = 'orig/%s/%s' % (V, os.path.basename(o['target_path']))
    for k, v in (('asm_path', 'asm'), ('build_path', 'obj'), ('src_path', 'src'),
                 ('ld_script_path', 'ldscript.ld'), ('undefined_funcs_auto_path', 'undefined_funcs_auto.txt'),
                 ('undefined_syms_auto_path', 'undefined_syms_auto.txt'), ('elf_path', 'elf.elf')):
        o[k] = os.path.join(D, v).replace('\\', '/')
    o['generate_asm_macros_files'] = False
    o['cache_path'] = os.path.join(D, '.splache').replace('\\', '/')
    if symbols:
        o['symbol_addrs_path'] = os.path.abspath(symbols).replace('\\', '/')
    else:
        sp = o['symbol_addrs_path']
        sp = sp if isinstance(sp, list) else [sp]
        o['symbol_addrs_path'] = [os.path.join(RAIZ, p).replace('\\', '/') for p in sp]
    os.makedirs(D, exist_ok=True)
    out = os.path.join(D, 'config.yml')
    yaml.safe_dump(cfg, open(out, 'w', encoding='utf-8'), sort_keys=False, width=200)
    return out


def trocea(V, D, yml):
    """splat en un proceso aparte (su estado es global); devuelve la lista de unidades (nombre, .s)"""
    guion = os.path.join(D, '_split.py')
    open(guion, 'w', encoding='utf-8').write(
        'import sys, json\n'
        'sys.setrecursionlimit(10000)\n'
        'import splat.scripts.split as split\n'
        'from pathlib import Path\n'
        'split.main([Path(%r)], modes="all", verbose=False, use_cache=False)\n'
        'u = []\n'
        'for e in split.linker_writer.entries:\n'
        '    if e.segment.type[0] == "." or e.object_path is None:\n'
        '        continue\n'
        '    u.append(str(e.src_paths[0]))\n'
        'json.dump(sorted(set(u)), open(%r, "w"))\n' % (yml, os.path.join(D, 'unidades.json')))
    t = time.time()
    r = subprocess.run([sys.executable, guion], cwd=RAIZ, capture_output=True, text=True, errors='replace')
    open(os.path.join(D, 'split.log'), 'w', encoding='utf-8').write(r.stdout + '\n' + r.stderr)
    print('split: codigo %d, %.0f s' % (r.returncode, time.time() - t))
    if r.returncode:
        print(r.stderr[-3000:])
        sys.exit(1)
    return json.load(open(os.path.join(D, 'unidades.json')))


def ensambla_uno(args):
    s, o = args
    os.makedirs(os.path.dirname(o), exist_ok=True)
    r = subprocess.run([AS, *ASFLAGS, '-o', o, s], capture_output=True, text=True, errors='replace')
    return s, r.returncode, r.stderr[-800:]


def ensambla(D):
    base = os.path.join(D, 'asm')
    trabajos = []
    for dp, dn, fn in os.walk(base):
        for f in fn:
            if not f.endswith('.s'):
                continue
            s = os.path.join(dp, f)
            rel = os.path.relpath(s, base).replace(os.sep, '/')
            # como tools/splat_to_config_json.py: .removesuffix(".s").removesuffix(".cpp").removesuffix(".c")
            nombre = rel.removesuffix('.s').removesuffix('.cpp').removesuffix('.c')
            trabajos.append((s, os.path.join(D, 'obj', nombre + '.o')))
    t = time.time()
    malos = 0
    with concurrent.futures.ThreadPoolExecutor(12) as ex:
        for s, rc, err in ex.map(ensambla_uno, trabajos):
            if rc:
                malos += 1
                print('AS FALLA', s, err)
    print('ensamblados %d (%d fallan), %.0f s' % (len(trabajos), malos, time.time() - t))


if __name__ == '__main__':
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    V, D = args[0], os.path.abspath(args[1])

    def opt(n):
        return sys.argv[sys.argv.index(n) + 1] if n in sys.argv else None
    if '--solo-ensamblar' not in sys.argv:
        yml = prepara(V, D, opt('--yml'), opt('--symbols'))
        trocea(V, D, yml)
    if '--sin-ensamblar' not in sys.argv:
        ensambla(D)
