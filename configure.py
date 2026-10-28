#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any, Dict, List

from tools.project import (
    Object,
    ProgressCategory,
    ProjectConfig,
    Platform,
    calculate_progress,
    generate_build,
    is_windows,
)
from tools.sufijo_local import resolver as resolver_sufijos

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "GOWE69",  # 0
    "EUROPEGERMILESTONE",  # 1
    "SLES-53558-A124",  # 2
    "SLUS-21351",  # 3
    "SPEED_EXE_1_3",  # 4
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--delink",
    metavar="BINARY | DIR",
    type=Path,
    help="path to delink binary (optional)",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY | DIR",
    type=Path,
    help="path to objdiff-cli binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
parser.add_argument(
    "--no-progress",
    dest="progress",
    action="store_false",
    help="disable progress calculation",
)
parser.add_argument(
    "--gc-toolchain-source",
    "--gc-toolchain-fuente",
    "--gc-toolchain-faithful",
    dest="gc_toolchain_source",
    action="store_true",
    help="GameCube: compile with the faithful replicas of SN's compilers built from their GPL source "
    "(ProDG/3.9.3-src and ProDG/3.9.3-v183src, made by tools/sn-gcc/build.sh) instead of the official "
    "binaries: the same report and DOL; the environment variable NFSMW_GC_TOOLCHAIN_SOURCE=1 does the same",
)
parser.add_argument(
    "--gc-toolchain-experiments",
    "--gc-toolchain-experimentos",
    dest="gc_toolchain_experiments",
    action="store_true",
    help="GameCube: compile with the source-built compilers plus the project's experiments "
    "(ProDG/3.9.3-src-exp and ProDG/3.9.3-v183src-exp, with -DSN_SOURCE_TOOLCHAIN); not the official "
    "report (it loses functions); NFSMW_GC_TOOLCHAIN_SOURCE=experiments does the same",
)
args = parser.parse_args()

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.delink_path = args.delink
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
config.generate_map = args.map
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
config.progress = args.progress
if not is_windows():
    config.wrapper = args.wrapper
# Don't build asm unless we're --non-matching
if not config.non_matching:
    config.asm_dir = None

# Tool versions
config.compilers_tag = "20260903"

if version_num in [0]:
    config.platform = Platform.GC_WII
    config.dtk_tag = "v1.8.32"
    config.binutils_tag = "2.42-1"
elif version_num in [1]:
    config.platform = Platform.X360
    config.dtk_tag = "v0.1.2"
    config.binutils_tag = "2.42-1"
elif version_num in [2, 3]:
    config.platform = Platform.PS2
    config.binutils_tag = "2.45"
elif version_num in [4]:
    config.platform = Platform.WIN32
    config.delink_tag = "v0.16.4"

config.objdiff_tag = "v3.8.0"  # r79: el nuestro es mas nuevo que el de upstream
config.sjiswrap_tag = "v1.2.2"

# sjiswrap segfault
if config.platform == Platform.GC_WII:
    config.wibo_tag = "1.1.0"
else:
    config.wibo_tag = "1.2.0"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"

compilers_path = (
    Path(config.compilers_path) if config.compilers_path else Path("build/compilers")
)

# Opcional (decision del usuario, 2026-09-30): GameCube con el compilador de SN hecho desde su fuente GPL
# (orig/prodg) por tools/sn-gcc/build.sh, con los parches legibles en vez de los binarios. build.sh hace la
# REPLICA FIEL (ProDG/3.9.3-src = v1.79 + $LC, mismo ensamblador que el v1.76 en las 409 unidades v1.76;
# ProDG/3.9.3-v183src = la replica v1.83 de path, + el cambio de Keating) y, aparte (decision del usuario,
# ronda 78), los EXPERIMENTOS del proyecto sobre ellas: ProDG/3.9.3-src-exp y ProDG/3.9.3-v183src-exp
# (+ frsqrte-fres, fnmsubs-minus y bsqrt-order: los builtins y el scheduling del bSqrt de bMath.hpp).
# Decision del usuario (ronda 79): --gc-toolchain-source (alias --gc-toolchain-fuente y --gc-toolchain-faithful;
# NFSMW_GC_TOOLCHAIN_SOURCE=1 o faithful) es la REPLICA FIEL, que da el informe y el DOL oficiales; los
# experimentos van aparte, con --gc-toolchain-experiments (NFSMW_GC_TOOLCHAIN_SOURCE=experiments o exp): las -exp
# con -DSN_SOURCE_TOOLCHAIN (ronda 78: GOWE69 con ellas pierde 156 funciones y cambia el DOL). Sin opcion no
# cambia nada. La variable de entorno sirve para que pctall.py/medir.py, que llaman a configure.py sin
# argumentos, la conserven.
_gc_tc_env = os.environ.get("NFSMW_GC_TOOLCHAIN_SOURCE", "")
gc_toolchain_exp = args.gc_toolchain_experiments or _gc_tc_env in ("experiments", "exp")
gc_toolchain_fiel = not gc_toolchain_exp and (args.gc_toolchain_source or _gc_tc_env not in ("", "0"))
_gc_tc_suf = "-exp" if gc_toolchain_exp else ""
if config.platform == Platform.GC_WII and (gc_toolchain_exp or gc_toolchain_fiel):
    for _tc in ("ProDG/3.9.3-src" + _gc_tc_suf, "ProDG/3.9.3-v183src" + _gc_tc_suf):
        if not (compilers_path / _tc / "cc1plus.exe").is_file():
            sys.exit(
                f"--gc-toolchain-source/-experiments: {compilers_path / _tc} is missing; build it with "
                "tools/sn-gcc/build.sh (see tools/sn-gcc/README.md)"
            )

if config.platform == Platform.GC_WII:
    config.asflags = [
        "-mgekko",
        "--strip-local-absolute",
        "-I include",
        f"-I build/{config.version}/include",
        f"--defsym BUILD_VERSION={version_num}",
    ]

    ldscript_path = Path("config") / config.version / "ldscript.ld"
    keep_list_path = Path("config") / config.version / "keep.lst"
    config.ldflags = [
        "-strip-unused-data",
        # "-report-unused",
        "-keep",
        str(keep_list_path),
        "-T",
        str(ldscript_path),
    ]

    # Optional numeric ID for decomp.me preset
    # Can be overridden in libraries or objects
    config.scratch_preset_id = 176
elif config.platform == Platform.X360:
    config.ldflags = [
        "/NODEFAULTLIB",
        "/MACHINE:PPCBE",
        "/XEX:NO",
        f"/PDB:./build/{config.version}/{config.version}.pdb",
        f"/DEBUG",
        f"/LTCG",
    ]
elif config.platform == Platform.PS2:
    config.asflags = [
        "-no-pad-sections",
        "-EL",
        "-march=5900",
        "-mabi=eabi",
        "-I include",
    ]
    ldscript_path = Path("build") / config.version / "ldscript.ld"
    config.ldflags = [
        "-EL",
        "-T",
        str(ldscript_path),
    ]  # TODO what about undefined_syms_auto.txt?
elif config.platform == Platform.WIN32:
    config.ldflags = [
        "/NODEFAULTLIB",
        f"/PDB:./build/{config.version}/{config.version}.pdb",
        f"/DEBUG",
    ]

# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = []

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
if config.platform == Platform.GC_WII:
    config.linker_version = (
        "ProDG/3.9.3-src" + _gc_tc_suf if gc_toolchain_exp or gc_toolchain_fiel else "ProDG/3.9.3"
    )

    dolphinsdk_root = "src/Speed/GameCube/bWare/GameCube/dolphinsdk"

    cflags_base_mwcc = [
        "-nodefaults",
        "-proc gekko",
        "-align powerpc",
        "-enum int",
        "-fp hardware",
        "-Cpp_exceptions off",
        # "-W all",
        "-O4,p",
        "-inline auto",
        '-pragma "cats off"',
        '-pragma "warn_notinlined off"',
        "-maxerrors 1",
        "-nosyspath",
        "-RTTI off",
        "-fp_contract off",
        "-str reuse",
        # "-i include",
        # f"-i build/{config.version}/include",
        "-multibyte",
        "-D__GEKKO__",
        f"-DVERSION={version_num}",
    ]

    # TODO move some of these to the game flags
    cflags_base_prodg = [
        "-O1",
        "-gdwarf+",
        # "-Wa,-L",
        # "-Wall",
        # "-Wreturn-type", # enable at some point
        "-Wno-ctor-dtor-privacy",  # because of AttribSys for example
        "-Woverloaded-virtual",
        "-Wno-multichar",
        "-I src/Speed/Indep/Libs/Support/stlgc",
        "-I src/Speed/GameCube/Libs/stl/STLport-4.5/stlport",
        "-I src/Speed/GameCube/bWare/GameCube/SN/include",
        "-I src/Packages/eathread/1.1.0/include",
        f"-I {dolphinsdk_root}/include",
        "-I src/Packages",
        "-I src/Speed/GameCube/bWare/GameCube/SN",
        # "-I ./",
        "-I src",
        "-DEA_PLATFORM_GAMECUBE",
        "-DEA_REGION_AMERICA",
        "-DGEKKO",
        "-D_USE_MATH_DEFINES",
        f"-I build/{config.version}/include",
        f"-DBUILD_VERSION={version_num}",
        f"-DVERSION_{config.version}",
    ]

    # PS2 path/spch: mismos includes que GC (rutas independientes de plataforma)
    cflags_path = [
        *cflags_base_prodg,
        "-I src/Packages/eathread/1.1.0/include",
        "-I src/Speed/Indep/Libs/allocator/1.5.0",
        "-I src/Speed/Indep/Libs/csis/dev/include",
        "-I src/Speed/Indep/Libs/snd/9/include",
        "-I src/Speed/Indep/Libs/path/5.01.04/include",
    ]

    # PS2 spch: los .c se compilan como C++ (mismo caso que snd en GC)
    cflags_spch = [
        *cflags_base_prodg,
        "-G0",
        "-x c++",
        "-I src/Speed/Indep/Libs/allocator/1.5.0",
        "-I src/Speed/Indep/Libs/csis/dev/include",
        "-I src/Speed/Indep/Libs/snd/9/include",
        "-I src/Speed/Indep/Libs/spch/dev/include",
    ]


    config.context_defines = [
        "EA_PLATFORM_GAMECUBE",
        "EA_REGION_AMERICA",
        "GEKKO",
        "_USE_MATH_DEFINES",
        "__SN__",
        "SN_TARGET_NGC",
    ]

    if config.non_matching:
        cflags_base_prodg.append("-DNON_MATCHING")

    if gc_toolchain_exp:
        # El compilador construido desde la fuente con los experimentos (-exp; Fase 2: __builtin_frsqrte/fres,
        # patron fnmsubs-minus y scheduling t1-antes-t2 de bSqrt): bMath.hpp lo usa
        # para el bSqrt de GameCube en vez del asm (byte-identico; el oficial lo necesita).
        cflags_base_prodg.append("-DSN_SOURCE_TOOLCHAIN")

    # Debug flags
    if args.debug:
        cflags_base_prodg.append("-DDEBUG=1")
    else:
        cflags_base_prodg.append("-DNDEBUG=1")

    cflags_game = [
        *cflags_base_prodg,
        "-mps-nodf",
        # "-mfast-cast",
        "-G0",
        # "-mstrict-align",
        # "-mno-bit-align",
        "-fno-static-dtors",
        "-ffast-math",
        # "-fno-strength-reduce",
        "-fforce-addr",
        "-fcse-follow-jumps",
        "-fcse-skip-blocks",
        "-fforce-mem",
        "-fgcse",
        "-frerun-cse-after-loop",
        "-fschedule-insns",
        "-fschedule-insns2",
        "-fexpensive-optimizations",
        "-frerun-loop-opt",
        "-fmove-all-movables",
        # "-fno-defer-pop",
        # "-fno-function-cse",
        # "-fpeephole",
        # "-fregmove",
        # "-fno-thread-jumps",
        # "-freduce-all-givs",
        # # "-fcaller-saves",
        # # "-ffloat-store",
        # # "-funroll-all-loops",
        # "-fno-sched-spec",
        # "-fno-keep-static-consts",
        # "-fno-common",
        # "-fno-argument-alias",
        # "-fno-ident",
        "-DLUA_NUMBER=float",
        "-DDISABLE_RAIN",
        "-DDEFAULT_ALLOCATOR=0",
        # path.h: PATH_vectortoreal6 y PATH_setallocator solo existen con USEREALCORE.
        "-DUSEREALCORE",
        "-I src/Speed/Indep/Libs/allocator/1.5.0",
        "-I src/Speed/Indep/Libs/csis/dev/include",
        "-I src/Packages/eathread/1.1.0/include",
        "-I src/Speed/Indep/Libs/snd/9/include",
        "-I src/Speed/Indep/Libs/spch/dev/include",
        "-I src/Speed/Indep/Libs/path/5.01.04/include",
        "-I src/Speed/Indep/Libs/realcore/6.24.00/include/common",
        "-I src/Speed/Indep/Libs/endian/0.5.2/include",
    ]

    cflags_snd = [
        *cflags_base_prodg,
        "-G0",
        "-O2",
        "-fno-strength-reduce",
        "-fno-strict-aliasing",
        "-ffast-math",
        "-mps-float",
        "-x c++",
        "-I src/Speed/Indep/Libs/csis/dev/include",
        "-I src/Speed/Indep/Libs/allocator/1.5.0",
        "-I src/Speed/Indep/Libs/snd/9/include",
    ]

    # EA audio middleware: path (music pathing) and spch (speech) libraries.
    # Identified via DWARF1 CU map from NFSMWRELEASE.ELF (C:/packages/path,
    # D:/perforce/audio/libs/SPCH); built with the SN toolchain like snd.
    # -mps-float como su hermana snd (y rcmp/vp6): caller-save.c de SN reserva con el 8 B de marco por cada FPR vivo
    # tras una llamada; son los 8 B de CreateStreamTrack y PATH_createstreamimp (medido: path 194/194; en libc,
    # compilada sin el, romperia 13 funciones con la misma huella).
    cflags_path = [
        *cflags_base_prodg,
        "-G8",
        "-O2",
        "-fno-strict-aliasing",
        "-ffast-math",
        "-mps-float",
        "-I src/Speed/Indep/Libs/allocator/1.5.0",
        "-I src/Speed/Indep/Libs/csis/dev/include",
        "-I src/Speed/Indep/Libs/snd/9/include",
        "-I src/Speed/Indep/Libs/path/5.01.04/include",
    ]

    # NOTE: despite the .c extension, these files were compiled as C++ by the
    # original build (all symbols in the base objects use GCC2 C++ mangling,
    # e.g. SPCH_GetExtVecs__Fv), so force -x c++ like the snd lib.
    cflags_spch = [
        *cflags_base_prodg,
        "-G0",
        "-O2",
        "-fno-strength-reduce",
        "-fno-strict-aliasing",
        "-ffast-math",
        "-x c++",
        "-I src/Speed/Indep/Libs/allocator/1.5.0",
        "-I src/Speed/Indep/Libs/csis/dev/include",
        "-I src/Speed/Indep/Libs/snd/9/include",
        "-I src/Speed/Indep/Libs/spch/dev/include",
    ]

    config.extra_clang_flags = [
        "-std=gnu++11",
        "-DSN_TARGET_NGC",
        "-D__SN__",
        "-D_STLP_VENDOR_EXCEPT_STD=std",
        "-DCLANGD_DAMNIT",  # used in cases where intellisense breaks
        "-D__HONOR_STD",
        "-Wno-writable-strings",
    ]

    cflags_dolphin = [
        *cflags_base_mwcc,
        f"-i {dolphinsdk_root}/include",
        f"-i {dolphinsdk_root}/include/libc",
        "-i include",
        f"-i build/{config.version}/include",
        "-char unsigned",
        "-warn pragmas",
        "-requireprotos",
        "-DSDK_REVISION=2",
        f"-ir {dolphinsdk_root}/src",
    ]

    # Tres unidades de LibSN NO son SN ProDG: son Metrowerks CodeWarrior.
    # Prueba: no llevan el simbolo `gcc2_compiled.` (presente en 100 de 105
    # unidades GCC del juego y en 0 de 95 Metrowerks), vm.s llama a __div2i y
    # _savegpr_23/24 (runtime MW; GCC emitiria __divsi3), y sus estaticos de
    # funcion van manglados `nombre$NNN` en vez de `nombre.NNN`.
    # El prologo (`stwu` antes del `mflr`) las situa en la familia 1.3-2.7.
    cflags_libsn_mw = [
        *[f for f in cflags_dolphin if f != "-inline auto"],
        "-inline off",
        "-lang=c",
    ]

    # steering.c (driver Logitech) NO comparte optimizacion con vm/vmbase:
    # medido, con -O4,s y -fp_contract on da 3.232 B y con -O4,p 1.264 B; al reves,
    # aplicar -O4,s a vm lo baja de 1.164 a 304 B y a vmbase de 1.028 a 408 B.
    # Se lee del asm sin compilar: con ,p MWCC desenrolla x8 los bucles de relleno
    # y convierte /360 en un mulhw de numero magico; con ,s emite bucle contado y
    # divw real. Y los fmadds del objetivo son -fp_contract on.
    cflags_libsn_mw_steering = [
        ("-O4,s" if f == "-O4,p" else f)
        for f in cflags_libsn_mw
    ]
    # y -inline on: el original expande funciones `static inline` (MulDiv, CookAxis,
    # SimThread_SetFilter, ZeroMem): con ellas Effect_PerformEnvelope casa y salen los
    # siete bloques `asm { }` que tenia el fichero. Con -inline auto mwcc ademas mete
    # CookValues en sus llamadores y bajan tres funciones.
    cflags_libsn_mw_steering = [
        ("-fp_contract on" if f == "-fp_contract off"
         else "-inline on" if f == "-inline off" else f)
        for f in cflags_libsn_mw_steering
    ] + ["-use_lmw_stmw on"]
    # Metrowerks library flags
    cflags_runtime = [*cflags_base_mwcc]

    cflags_odemuexi = [*cflags_base_mwcc]

    cflags_amcstub = [*cflags_base_mwcc]

    # Helper function for Dolphin libraries
    def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
        return {
            "lib": lib_name,
            "toolchain_version": "GC/1.2.5n",
            "cflags": cflags_dolphin,
            "progress_category": "sdk",
            "objects": objects,
        }

elif config.platform == Platform.X360:
    config.linker_version = "X360/14.00.2110"

    cflags_base_prodg = [
        "/nologo",
        "/c",  # compile without linking
        "/wd4996",  # get rid of string deprecation warnings for now
        "/wd4355",  # gets rid of the warning 'this' used in base member initializer
        "/wd4716",
        # "/GL",  # enable LTCG
        # "/GR",  # RTTI
        "/Og",
        # "/Os", # no
        "/Ob2",
        "/Oi",
        # "/Oy",  # maybe
        # "/Ox",  # maybe
        # "/Ou",  # enable prescheduling
        "/Oz",  # enable inline asm scheduling
        "/GF",  # Eliminate Duplicate Strings
        # "/Gy",  # maybe?
        # "/Z7",  # /Zi enables debug info (pdb), /Zd for line numbers only (pdb), /Z7 generates debug info per obj file
        # "/EHsc",  # el original no usa excepciones (ronda 9): quitado, +67 -22 en ventana 7
        # La STL de X360 tambien pide la memoria a FastMem con "STL" (lbl_82069D94 = "STL" en su .rdata): la xmemory
        # de EA va delante de las cabeceras del XDK, como en PC. Fichero a fichero cambia el inline de list::_Incsize
        # en LoadSMSInfo; sobre el enlace LTCG (la medida oficial) LoadSMSInfo sigue al 100 % y se ganan 9 (ronda 39).
        "/I src/Speed/Indep/Libs/Support/stlpc",
        "/I src/Packages/xenonsdk/2.0.2135.2/installed/include/xbox",
        "/I src/Packages",
        "/I src",
        "/DEA_PLATFORM_XENON",
        "/D_USE_MATH_DEFINES",
        # X36042: los iteradores de la STL del XDK son de UNA palabra en el original (sin _Mycont): _Tree::_Insert
        # devuelve el iterador guardando solo el nodo (`stw r27, 0(r26)`) y con el _SECURE_SCL=1 por defecto de
        # yvals.h guarda tambien el contenedor. Banco LTCG: +107 funciones (33,5 kB), y las 3 que bajan son nombres de
        # la variante comprobada (_Unchecked_iterator_tag, iterator(nodo, contenedor)) que el original no tiene.
        "/D_SECURE_SCL=0",
        # X48: bBreak de X360 llama al DebugBreak del XDK (<xtl.h>); nuestra copia del SDK no trae ObjBase.h, que pide
        # d3dx9.h, y NOD3D deja fuera las cabeceras de D3D (nadie del juego las incluye por <xtl.h>).
        "/DNOD3D",
        # X50: el proyecto del XDK define _XBOX (d3dx9.h toma XObjBase.h y no ObjBase.h con el); eMathE de Xenon
        # y bTransposeMatrix llaman al D3DX del XDK como NFS.exe (D3DXMatrixMultiply 0x82679D40...).
        "/D_XBOX",
        f"/I build/{config.version}/include",
        f"/DBUILD_VERSION={version_num}",
        f"/DVERSION_{config.version}",
    ]

    config.context_defines = [
        "EA_PLATFORM_XENON",
        "EA_REGION_GERMANY",
        "_USE_MATH_DEFINES",
        "_WIN32",
    ]

    cflags_game = [
        *cflags_base_prodg,
        "/DLUA_NUMBER=float",
        "/DMILESTONE_BUILD",
        "/DDEFAULT_ALLOCATOR=0",
        # path.h: PATH_vectortoreal6 y PATH_setallocator solo existen con USEREALCORE.
        "/DUSEREALCORE",
        "/I src/Speed/Indep/Libs/allocator/1.5.0",
        "/I src/Speed/Indep/Libs/csis/dev/include",
        "/I src/Packages/eathread/1.1.0/include",
        "/I src/Speed/Indep/Libs/snd/9/include",
        "/I src/Speed/Indep/Libs/spch/dev/include",
        "/I src/Speed/Indep/Libs/path/5.01.04/include",
        "/I src/Speed/Indep/Libs/realcore/6.24.00/include/common",
        "/I src/Speed/Indep/Libs/endian/0.5.2/include",
    ]

    cflags_snd = [
        *cflags_game,
        "/TP",
        "/I src/Speed/Indep/Libs/snd/9/include",
    ]

    # path y spch son bibliotecas nuestras (upstream no las lista): sin estos
    # cflags, `configure.py -v EUROPEGERMILESTONE` moria con NameError, igual
    # que le pasaba al bloque WIN32.
    cflags_path = [
        *cflags_game,
        "/I src/Speed/Indep/Libs/allocator/1.5.0",
        "/I src/Speed/Indep/Libs/csis/dev/include",
        "/I src/Speed/Indep/Libs/snd/9/include",
        "/I src/Speed/Indep/Libs/path/5.01.04/include",
    ]

    # r86 (X360-A): spch y snd como C++ (/TP), como en PC: sus .c no compilan como C con el allocator de EA
    # (iallocator.h abre `namespace EA`) y en GameCube ya son C++ (iSPCH_InitBanks__Fv).
    cflags_spch = [
        *cflags_game,
        "/TP",
        "/I src/Speed/Indep/Libs/allocator/1.5.0",
        "/I src/Speed/Indep/Libs/csis/dev/include",
        "/I src/Speed/Indep/Libs/snd/9/include",
        "/I src/Speed/Indep/Libs/spch/dev/include",
    ]

    config.extra_clang_flags = [
        "-std=c++03",
        "-D_WIN32",
        "-D_WCHAR_T_DEFINED",
        "-fms-extensions",
    ]
elif config.platform == Platform.PS2:
    config.linker_version = "PS2/ee-gcc2.9-991111"

    cflags_base_prodg = [
        "-O2",
        "-g2",
        "-Wa,-L",  # Keep compiler-generated $LC* local object symbols
        # "-Wall",
        "-Wno-ctor-dtor-privacy",  # because of AttribSys for example
        "-I src/Speed/Indep/Libs/Support/stlps2",
        "-I src/Speed/PSX2/Libs/stl/gpp",
        "-I src/Speed/PSX2/bWare/src/ee/include",
        "-I src/Speed/PSX2/bWare/src/ee/gcc/ee/include",
        "-I src/Speed/PSX2/bWare/src/ee/gcc/lib/gcc-lib/ee/2.9-ee-991111/include",
        "-I src/Speed/PSX2/bWare/src/ee/gcc/ee",
        "-I src/Speed/PSX2/bWare/src/ee/gcc/lib/gcc-lib/ee/2.9-ee-991111",
        "-I src/Packages",
        "-I src",
        "-DEA_PLATFORM_PLAYSTATION2",  # TODO rename to PS2
        # EA_BUILD_A124 y MILESTONE_BUILD son del PROTOTIPO alpha 124 (SLES-53558-A124,
        # version_num 2). SLUS-21351 es la version DE VENTA y no es ninguna de las dos:
        # su ELF no tiene `bMalloc__FiPCcii` -- el asignador de cuatro argumentos, con
        # fichero y linea, que solo existe en builds milestone -- y el de SLES si.
        *(["-DEA_BUILD_A124"] if version_num == 2 else []),
        # Region: SLUS-21351 es la version americana (su BuildRegion::IsAmerica devuelve 1) y la A124 es un
        # prototipo de la europea SLES-53558 (su IsEurope devuelve 1). Solo BuildRegion.cpp mira estas macros.
        *(["-DEA_REGION_EUROPE"] if version_num == 2 else ["-DEA_REGION_AMERICA"]),
        "-D_NOTHREADS",  # TODO is this necessary?
        # r77 (PS2): `_STL` es el espacio de nombres propio con el que esta
        # configurado el STLport de GameCube, y el codigo compartido escribe
        # `_STL::sort(...)` en 49 sitios. En PS2 la STL es la de SGI y va en el
        # espacio global: COMPROBADO contra el binario, el ELF de PS2 no tiene ni
        # una ocurrencia de `_STL`. Definiendolo vacio, `_STL::sort` queda en
        # `::sort`, que es lo que la STL de PS2 ofrece. Cero cambios en fuente y
        # cero efecto en GameCube, que no pasa por este bloque.
        "-D_STL=",
        f"-I build/{config.version}/include",
        f"-DBUILD_VERSION={version_num}",
        # f"-DVERSION_{config.version}", # TODO it's broken because of the dash?
    ]

    # PS2 path/spch: mismos includes que GC (rutas independientes de plataforma)
    # path (SN 2.95.3-136) lleva -G0, -ffast-math y -fno-exceptions/-fno-rtti como
    # el resto del middleware. Medido una a una: -G0 +10.460 B, -ffast-math +788 B
    # y sin excepciones +612 B en SLES (+8.860, +788 y +612 en SLUS).
    # y -mvu0-use-vf0-vf31 como snd de PS2: sube el umbral de loop.c para sacar invariantes de bucles con llamadas
    # (PATH_event; medido por el agente de bibliotecas de PS2 de la ronda 14).
    cflags_path = [
        *cflags_base_prodg,
        "-G0",
        "-ffast-math",
        "-fno-exceptions",
        "-fno-rtti",
        "-mvu0-use-vf0-vf31",
        "-I src/Packages/eathread/1.1.0/include",
        "-I src/Speed/Indep/Libs/allocator/1.5.0",
        "-I src/Speed/Indep/Libs/csis/dev/include",
        "-I src/Speed/Indep/Libs/snd/9/include",
        "-I src/Speed/Indep/Libs/path/5.01.04/include",
    ]

    # PS2 spch: los .c se compilan como C++ (mismo caso que snd en GC)
    cflags_spch = [
        *cflags_base_prodg,
        "-G0",
        "-x c++",
        "-I src/Speed/Indep/Libs/allocator/1.5.0",
        "-I src/Speed/Indep/Libs/csis/dev/include",
        "-I src/Speed/Indep/Libs/snd/9/include",
        "-I src/Speed/Indep/Libs/spch/dev/include",
    ]

    config.context_defines = [
        "EA_PLATFORM_PLAYSTATION2",
        *(["EA_REGION_EUROPE"] if version_num == 2 else ["EA_REGION_AMERICA"]),
        *(["EA_BUILD_A124"] if version_num == 2 else []),
        "_NOTHREADS",
    ]

    # Debug flags
    # TODO
    # if args.debug:
    # cflags_base.append("-DDEBUG=1")
    # else:
    #     cflags_base.append("-DNDEBUG=1")

    cflags_game = [
        *cflags_base_prodg,
        "-G0",
        "-ffast-math",
        "-fno-exceptions",
        "-fno-rtti",
        # "-funaligned-pointers",
        # "-funaligned-struct-hack",
        # "-fsched-interblock",
        # "-fsched-spec",
        # "-fsched-spec-load-dangerous",
        # "-fedge-sm",
        # "-fedge-lm",
        # "-fedge-lcm",
        # "-fforce-addr",
        # "-fcse-follow-jumps",
        # "-fcse-skip-blocks",
        # "-fforce-mem",
        # "-fgcse",
        # "-fstrength-reduce",
        # "-frerun-cse-after-loop",
        # "-fschedule-insns",
        # "-fschedule-insns2",
        # "-fexpensive-optimizations",
        # "-frerun-loop-opt",
        # "-fmove-all-movables",
        # "-fregmove",
        # "-fcaller-saves",
        "-DLUA_NUMBER=float",
        # Solo el prototipo A124: ver la nota de EA_BUILD_A124 mas arriba.
        *(["-DMILESTONE_BUILD"] if version_num == 2 else []),
        "-DDEFAULT_ALLOCATOR=0",
        # path.h: PATH_vectortoreal6 y PATH_setallocator solo existen con USEREALCORE.
        "-DUSEREALCORE",
        "-I src/Speed/Indep/Libs/allocator/1.5.0",
        "-I src/Speed/Indep/Libs/csis/dev/include",
        "-I src/Packages/eathread/1.1.0/include",
        "-I src/Speed/Indep/Libs/snd/9/include",
        "-I src/Speed/Indep/Libs/spch/dev/include",
        "-I src/Speed/Indep/Libs/path/5.01.04/include",
        "-I src/Speed/Indep/Libs/realcore/6.24.00/include/common",
        "-I src/Speed/Indep/Libs/endian/0.5.2/include",
    ]

    # snd 9.06.00-ps2 se compilo con los 32 registros de la VU0 asignables (-mvu0-use-vf0-vf31; el juego usa
    # vf0-vf15). La opcion no cambia ninguna instruccion por si sola: sube el numero de registros libres, y con el
    # el umbral de loop.c para sacar invariantes de los bucles con llamadas (constantes de una instruccion y partes
    # altas de direcciones que el original saca y sin ella se quedan dentro). Medido en las 134 unidades de snd de
    # las dos PS2: +6 funciones, ninguna perdida (sdriver.c: SNDDRV_thread y SNDDRV_updatevoices; SNDSTRMI_service,
    # readsamples de mtdecf, SFILTER_unpackxalf y SFILTER_unpackxapf).
    cflags_snd = [
        *cflags_game,
        "-mvu0-use-vf0-vf31",
        "-x c++",
        "-I src/Speed/Indep/Libs/csis/dev/include",
        "-I src/Speed/Indep/Libs/allocator/1.5.0",
        "-I src/Speed/Indep/Libs/snd/9/include",
    ]

    config.extra_clang_flags = [
        "-std=gnu++98",
        "-DCLANGD_DAMNIT",
        "-D__HONOR_STD",
        "-D__STL_MEMBER_TEMPLATE_KEYWORD",
        "-U_MIPS_SIM",
        "-U __mips",
        "-D__mips=3",
        "-D__mips_eabi",
        "-DR5900",
        "-D_R5900",
        "-D__mips_single_float",
        "-D__builtin_next_arg(x)=((void *)0)",
        "-D__builtin_args_info(x)=1",
        "-msoft-float",
    ]
elif config.platform == Platform.WIN32:
    config.linker_version = "Win32/7.1"

    cflags_base_prodg = [
        "/nologo",
        "/c",  # compile without linking
        "/wd4996",  # get rid of string deprecation warnings for now
        "/wd4355",  # gets rid of the warning 'this' used in base member initializer
        "/wd4716",  # functions not decompiled yet have empty bodies, as on Xbox 360
        # /O2 = /Ox + /GF + /Gy (el Release por defecto de Visual Studio .NET 2003). Con /Ox a secas 53 funciones salian
        # con el int3 de alineacion de la siguiente dentro de su tamano (sin /Gy van todas en una sola .text) y 4 con
        # las cadenas fuera de sus COMDAT ??_C@; con /O2, +57 y ninguna perdida (banco de todas las unidades).
        "/O2",
        "/Z7",  # /Zi enables debug info (pdb), /Zd for line numbers only (pdb), /Z7 generates debug info per obj file
        "/EHsc",  # enable exception handling (and extern C notthrow?)
        # La STL de PC pide la memoria a FastMem con la etiqueta "STL" (como stlgc y stlps2): la xmemory de EA va
        # delante de las cabeceras del compilador (src/Speed/Indep/Libs/Support/stlpc/xmemory). X360 tambien (su bloque).
        "/I src/Speed/Indep/Libs/Support/stlpc",
        f"/I {compilers_path / config.linker_version / 'Include'}",
        "/I src/Packages",
        "/I src",
        "/DEA_PLATFORM_WIN32",
        "/D_USE_MATH_DEFINES",
        f"/I build/{config.version}/include",
        f"/DBUILD_VERSION={version_num}",
        f"/DVERSION_{config.version}",
    ]

    config.context_defines = [
        "EA_PLATFORM_WIN32",
        "EA_REGION_AMERICA",
        "_USE_MATH_DEFINES",
        "_WIN32",
    ]

    cflags_game = [
        *cflags_base_prodg,
        "/DLUA_NUMBER=float",
        "/DDEFAULT_ALLOCATOR=0",
        # path.h: PATH_vectortoreal6 y PATH_setallocator solo existen con USEREALCORE.
        "/DUSEREALCORE",
        "/I src/Speed/Indep/Libs/allocator/1.5.0",
        "/I src/Speed/Indep/Libs/csis/dev/include",
        "/I src/Packages/eathread/1.1.0/include",
        "/I src/Speed/Indep/Libs/snd/9/include",
        "/I src/Speed/Indep/Libs/spch/dev/include",
        "/I src/Speed/Indep/Libs/path/5.01.04/include",
        "/I src/Speed/Indep/Libs/realcore/6.24.00/include/common",
        "/I src/Speed/Indep/Libs/endian/0.5.2/include",
    ]

    # PC41: snd de PC con /O1 y como C++ (/TP), como spch: con /O1 /TP casan por codigo 103 funciones de snd (mapa por
    # fichero de cmn y mix); con el /O2 del juego, 16. Las unidades de PC son las de config/SPEED_EXE_1_3.
    # PC51: sin /EHsc. El tramo de snd del exe (0x819000-0x829000) no tiene ni un marco de excepciones (cero
    # `mov fs:[0]`) y SFILTER_unpackmtfinit/mtpfinit/mtlfinit (new CMTBLKDecf) solo casan sin la limpieza del new.
    # r80 (decision del usuario): /Oi. El exe expande strlen/strcpy/memcpy en linea (el rep movsd de MIX_initreverb,
    # SNDSTRMI_parseheader); banco de las 108 unidades de snd de PC (sdsound.c incluida): +1, 0 perdidas.
    cflags_snd = [
        *[f for f in cflags_game if f not in ("/O2", "/EHsc")],
        "/O1",
        "/Oi",
        "/TP",
        "/I src/Speed/Indep/Libs/snd/9/include",
    ]

    # r79: las bibliotecas path y spch son nuestras; upstream no las tiene en su
    # lista, asi que su bloque WIN32 no definia estos cflags y
    # `configure.py -v SPEED_EXE_1_3` moria con NameError.
    # PC40: path de PC con /Oy- (marco de pila): con /O2 /Oy- casan 81 funciones de path por codigo; con /O2 a secas,
    # 10, y con /O1, 9. Compila desde la ronda 40, cuando eathread tuvo su backend de Win32 (pathi.h:532).
    # PC51: sin /EHsc, como snd: el tramo de path (0x828F90-0x8309F0) no tiene marcos de excepciones y PATHI_init
    # (0x829C50, 206 B) solo casa sin la limpieza del new.
    cflags_path = [
        *[f for f in cflags_game if f != "/EHsc"],
        "/Oy-",
        "/I src/Speed/Indep/Libs/allocator/1.5.0",
        "/I src/Speed/Indep/Libs/csis/dev/include",
        "/I src/Speed/Indep/Libs/snd/9/include",
        "/I src/Speed/Indep/Libs/path/5.01.04/include",
    ]

    # PC40: spch de PC con /O1 y como C++ (/TP), como snd: con /O1 casan por codigo 37 funciones de spch y 103 de snd;
    # con el /O2 del juego, 8 y 16. En GameCube ya son C++ (iSPCH_InitBanks__Fv, SNDI_cos__Ff).
    cflags_spch = [
        *[f for f in cflags_game if f != "/O2"],
        "/O1",
        "/TP",
        "/I src/Speed/Indep/Libs/allocator/1.5.0",
        "/I src/Speed/Indep/Libs/csis/dev/include",
        "/I src/Speed/Indep/Libs/snd/9/include",
        "/I src/Speed/Indep/Libs/spch/dev/include",
    ]

    config.extra_clang_flags = [
        "-std=c++98",
        "-D_WIN32",
        "-D_WCHAR_T_DEFINED",
        "-fms-extensions",
    ]

cflags_libc = [*cflags_base_prodg, "-O2"]


def solo_gc(*flags):
    """Opciones de ProDG que solo existen en GameCube (EE-GCC y MSVC las rechazan)."""
    return list(flags) if config.platform == Platform.GC_WII else []


def solo_ps2(*flags):
    """Opciones que solo se pasan en PS2."""
    return list(flags) if config.platform == Platform.PS2 else []


def solo_win32(*flags):
    """Opciones u objetos que solo existen en WIN32 (PC)."""
    return list(flags) if config.platform == Platform.WIN32 else []


def solo_a124(*objs):
    """Unidades que solo enlaza el prototipo SLES-53558-A124 (dirtysock no esta en SLUS)."""
    return list(objs) if config.version == "SLES-53558-A124" else []


# El juego de PS2 se compilo con registros VU0 asignables: los stabs de la A124 dan
# locales `uint128` (t, d, o, vfq...) en los registros 79-83, que son vf0-vf4, y
# EE-GCC solo reparte esos registros con la restriccion "j" y esta opcion (sin ella
# el "j" aborta con "fixed or forbidden register ... spilled for class VU_REGS").
# La opcion sola, sin tocar la fuente, ya cambia el reparto del codigo normal:
# medido en banco, +6 funciones en zAI y +2 en zWorld, ninguna perdida.
# Ronda 75: con los 32 (-mvu0-use-vf0-vf31, como snd, path y zEagl4Anim). El umbral de loop.c es
# (1 + registros no fijos) con llamadas en el bucle: 79 con vf15 y 95 con vf31 (medido en los volcados -dL).
# Con vf15 el original sacaba del bucle invariantes que nosotros no (la familia de Checkout, UIQRBrief::Setup,
# ReadMessageResponseTags y ClipGeneral; walk_all_paths, CompositeSkin32...): medido unidad a unidad en las dos
# PS2, +25 funciones en la A124 y +28 en SLUS. Ningun N intermedio (19, 23, 27-30) da todas.
vu0_ps2 = solo_ps2("-mvu0-use-vf0-vf31")
# Ronda 76 (decision del usuario): tambien zSpeech, zTrack y zWorld. Sus tres funciones del borde del umbral
# eran fuente corta: BuildHoleMovements tenia una inicializacion muerta de menos (vuelve al 100 %);
# FindHeliPoly y VerifySampleIntegrity quedan pendientes (diario ps2a12475).

# Las unidades del juego de GameCube se compilaron con -fkeep-inline-functions: el DWARF de las 33 SourceLists trae
# una copia fuera de linea (Range 0xFFFFFFFF, estripada al enlazar) de TODAS sus static inline (217 de 217 en zFe2:
# GXEnd, OSSetGQR2...; ninguna en las bibliotecas). Sin la opcion GCC 2.95 no emite los overrides virtuales de las
# clases locales (las 16 funciones de FEPlayerCarDB en zFe2, 624 B).
keep_inline_gc = solo_gc("-fkeep-inline-functions")

# Y con -fdefault-single-float, opcion de SN ("Any real constant will default to type float rather than double"): un
# literal sin sufijo es float. Prueba (GCD44, ronda 44): GenericNISControlScenario::HandleEventMessage carga con lfd
# el double 0x3EB0C6F7A0000000 = (double)1e-6f donde las dos PS2 tienen el 1e-6 exacto, y AssignCopsInCircle,
# OnTask (AIPursuit), ICEManager::GetSlope, GetGameCompletionStats, OnScreenRain, QueueWeatherStream y
# GetWooshSample cargan un double en las PS2 y operan en float en GameCube. NO es -fshort-double: el DWARF de las 33
# unidades da double de 8 B (struct __exception con arg2 en 0x10, bSymbolInfoPacket de 0x58). zBWare no la lleva:
# _bOutput carga el 0.1 double exacto. Medido en banco: cero funciones distintas en las 33 unidades.
single_float_gc = solo_gc("-fdefault-single-float")


# PS2: el middleware NO se compilo con el ee-gcc del juego. El .mdebug del NFS.ELF
# de SLES-53558-A124 guarda el directorio de compilacion de cada fichero: el juego
# sale de mw\Speed (2.9-ee-991111b, el de config.linker_version) y las bibliotecas
# de EA de sus paquetes PlayStation2SN (anotados abajo). Firma: guardan s0-s7/fp
# con `sq` y ra con `sd` (1.863 de 1.864; el otro es decodexac, ensamblador a
# mano). De los cinco ee-gcc de SN que tenemos solo el 2.95.3 SN BUILD 1.36 guarda
# asi: 2.95.2-273a/274 y 2.95.3-107/114 guardan tambien ra con `sq`. Medido con
# los flags de antes en las 189 unidades de biblioteca de SLES que compilan con
# todos: 991111 19.676 B, 273a/274/107/114 24.984 B y 136 68.700 B (en las 119 de
# SLUS: 41.512, 44.808 y 75.364 B). Con el 136 y los flags de abajo, SLES pasa de
# 23,31 % a 24,72 % y SLUS de 25,53 % a 26,71 %, sin tocar una SourceList.
# OJO: en los 2.95.3 de SN `long long` es de 128 bits (16 B) y `long`, de 64. El
# original usa `long` para 64 bits (FILESYS_readlarge__FiUlPvUliT2): donde nuestra
# fuente pone `long long`, el compilador aborta con "unsupported wide integer
# operation" salvo con -frelax-128 (opcion de SN, abajo por fichero), y lo que
# devuelve o guarda esos 64 bits no puede casar hasta que la fuente use `long`.
PS2_TOOLCHAIN = {
    "rcmp": "PS2/ee-gcc2.95.3-136",  # PlayStation2SN 2.8.1-8
    "eathread": "PS2/ee-gcc2.95.3-136",  # PlayStation2SN 2.8.1-8 (aun sin unidades en PS2)
    "realcore": "PS2/ee-gcc2.95.3-136",  # PlayStation2SN 2.8.1-9
    "spch": "PS2/ee-gcc2.95.3-136",  # PlayStation2SN 2.8.1-9 (con spchcsis.cpp)
    "vp6": "PS2/ee-gcc2.95.3-136",  # PlayStation2SN 2.8.1-9 (PS2 es 1.0.3: ver vp6_ps2)
    "snd": "PS2/ee-gcc2.95.3-136",  # PlayStation2SN 3.0.2-1
    "realmemcard": "PS2/ee-gcc2.95.3-136",  # PlayStation2SN 3.0.2-1 (PS2 es 3.04.00: ver rmc_ps2)
    # PlayStation2SN 3.0.2-4: la replica de scripts/replica_v183.py (ver GC_TOOLCHAIN)
    "path": "PS2/ee-gcc2.95.3-136-v183r",
    "csis": "PS2/ee-gcc2.95.3-136",  # PlayStation2SN 2.8.1-9
    # La libc de SN (stdio, qsort, atexit...): el prologo guarda los s con sq y ra con sd. Se ensamblo con el
    # Ps2EeAs de SN y no con el as de GNU: la replica de scripts/replica_snas.py (los bucles cortos a 6
    # instrucciones y no a 7), con "ee_as": "Ps2EeAs" en sus libs de C (ee-gcc -S y Ps2EeAs).
    "libc_sn": "PS2/ee-gcc2.95.3-136-snas",
    # dirtysock de PS2: el prologo guarda los s con sq y ra con sd (la firma del middleware, ver arriba)
    "dirtysock": "PS2/ee-gcc2.95.3-136",
}


# GameCube: path es "GNU C++ 2.95.3 SN BUILD v1.83" en el DWARF (paquete GameCubeSN 3.9.3-4 de EA) y el resto
# v1.76 o anterior; en PS2 path es PlayStation2SN 3.0.2-4. Esos compiladores no son publicos. Lo que los separa de
# los nuestros es el cambio de GCC del 2000-06-23 (alias.c: fixed_scalar_and_varying_struct_p no mira struct
# frente a escalar con -fno-strict-aliasing), que SN adopto entre v1.79 (su fuente GPL aun aplica la regla) y
# v1.83. scripts/replica_v183.py lo reproduce en copias de los oficiales, con SHA-1 comprobado. Medido: path +11
# en GameCube y +7 en cada PS2 sin perdidas; en las bibliotecas v1.76 la misma replica pierde (-59 en GameCube).
GC_TOOLCHAIN = {
    "path": (
        "ProDG/3.9.3-v183src" + _gc_tc_suf if gc_toolchain_exp or gc_toolchain_fiel else "ProDG/3.9.3-v183r"
    ),
}


def toolchain_de(lib):
    """toolchain_version de una biblioteca: en PS2, el ee-gcc de SN de su paquete; en GameCube, la replica de v1.83
    para path."""
    if config.platform == Platform.PS2:
        return PS2_TOOLCHAIN.get(lib, config.linker_version)
    if config.platform == Platform.GC_WII:
        return GC_TOOLCHAIN.get(lib, config.linker_version)
    return config.linker_version


# Las replicas se generan en local desde las toolchains oficiales (nada se descarga).
if config.platform in (Platform.GC_WII, Platform.PS2):
    import subprocess as _subprocess

    _subprocess.run([sys.executable, "scripts/replica_v183.py"], check=True)
    if config.platform == Platform.PS2:
        _subprocess.run([sys.executable, "scripts/replica_snas.py"], check=True)


# vp6: sin -fmove-all-movables ni -fforce-addr, y con -mps-float en vez de
# -mps-nodf (ver la nota del bloque de la biblioteca). En PS2 si lleva -G0, como
# el resto del middleware de SN, y VP6_VERSION=103 (ver vp6_ps2).
# PCM3: en PC el middleware (vp6, realcore, realmemcard y rcmp) NO va con el /O2
# del juego: va a /O1 con marco de pila (/Oy-), como eathread. Medido fuera del
# arbol contra los unassigned_* (pc3_lote2): a /O1 /Oy- casan las funciones con
# marco y sin marco de la misma unidad (el dtor de EventQueue sin marco y su
# operator= con el); a /O1 a secas faltan las de marco, y con el /O2 no casa
# casi nada. vp6 lleva ademas /Oi (HorizontalLine_Copy expande memcpy a
# rep movsd; sin el llama a memcpy) y /Dinline=__inline (DFrameR.c y
# decodembs.c usan el `inline` de C99, que MSVC 7.1 en C no conoce; medido: el
# define no cambia el objeto de las unidades C++ de la biblioteca). Estos
# bloques solo existen en WIN32: en GC/PS2 las bibliotecas conservan los cflags
# de siempre. PCM6: las unidades que MSVC no compilaba entran con puertas
# WIN32 minimas medidas en banco (150 fn / 5.501 B al 100 %): defines por
# objeto (filesys strcasecmp/__builtin_alloca + /FImalloc.h, abortmsg y
# printstr vsnprintf, memcard_taskmanager strncasecmp; en sus Object) y
# puertas en la fuente (hlafile/avplayer __attribute__, rcmp_mad_codec y
# rcmp_vp6_codec constante con salto de linea). Sigue excluida locale.cpp de
# realmemcard: con __declspec(align) compila, pero GetString (344 B) no tiene
# pareja en el exe (solo casan GetWstrLength 25 B y el setter, ambiguo).
if config.platform == Platform.WIN32:
    cflags_vp6 = [f for f in cflags_game if f != "/O2"] + [
        "/O1",
        "/Oy-",
        "/Oi",
        "/Dinline=__inline",
    ]
    cflags_realcore_win32 = [f for f in cflags_game if f != "/O2"] + ["/O1", "/Oy-"]
    # Ronda 61: /EHsc heredado del juego sale (el original no usa excepciones) y entran /Oi y /GX-:
    # con ello interfaceimp cierra 7/7 y memcard_interface 39/40 (CreateInstance, el ctor de GameInfo
    # y ~MemcardInterface necesitan /GX-; el ctor de GameInfo, /Oi por el memcpy expandido) y no se
    # rompe nada de impl/taskmanager/memvectors/utilities (medido TU a TU por pcx36060).
    cflags_realmemcard_win32 = [f for f in cflags_game if f not in ("/O2", "/EHsc")] + [
        "/O1",
        "/Oy-",
        "/Oi",
        "/GX-",
        "/I src/Packages/realmemcard/3.04.01-layer2/include/common",
    ]
    cflags_rcmp_win32 = [f for f in cflags_game if f != "/O2"] + ["/O1", "/Oy-"]
    # r86 (PC-B): dirtysock de PC (DirtySDK en C, como en la A124): /O2 del juego sin /EHsc. Medido contra el tramo
    # de dirtysock del exe (0x834576-0x85D380) con las fuentes escritas del asm de la A124: con /O2 casan 18 funciones
    # de lobbyapilist/lobbyapiutil/lobbyapiasync; con /O1 o /Oy- el parecido baja (0,75 -> 0,52/0,65).
    # r87 (PC-B): y /GS: _LobbyApiCreate, _LobbyApiAsyncMoveRoom, _BuddyApiRoster... de speed.exe llaman a
    # @__security_check_cookie@4 (las funciones con un char[] en la pila); el juego no lo lleva. Banco: ninguna baja.
    cflags_dirtysock_win32 = [f for f in cflags_game if f != "/EHsc"] + ["/GS", "/I src/Speed/Indep/Libs/dirtysock/include"]
else:
    cflags_vp6 = [
        f
        for f in cflags_game
        if f not in ("-G0", "-fmove-all-movables", "-fforce-addr", "-mps-nodf")
    ] + ["-O2", *solo_gc("-mps-float"), *solo_ps2("-G0", "-DVP6_VERSION=103")]
    cflags_realcore_win32 = cflags_realmemcard_win32 = cflags_rcmp_win32 = cflags_dirtysock_win32 = []
    # X59: en Xbox 360 la capa comun de realmemcard entra en el enlace LTCG con los flags del juego (la 360 tiene
    # BootupCheckParams::Clear suelto en 0x825E8E50 y expandido en el constructor de MemoryCard, 0x822B0B60).
    if config.platform == Platform.X360:
        cflags_realmemcard_win32 = [
            *cflags_game,
            "/I src/Packages/realmemcard/3.04.01-layer2/include/common",
            # Ronda 61 (pcx36061): memcard_taskmanager llama strncasecmp, que es la funcion de la libc
            # (src/libc/strncasecmp.c); en MSVC no existe y sin este define la TU no compila (medido A/B:
            # el objeto cae y el numero no se mueve, asi que no es la fuente de ninguna perdida).
            "/Dstrncasecmp=_strnicmp",
        ]


# vp6 en PS2: las dos PS2 llevan la 1.0.3 del paquete, no la 1.0.6 de GameCube. Lo
# dicen los stabs de la A124 (PB_INSTANCE sin OtherFrameRecon: 0x4954 bytes) y el
# codigo de las dos: VP6_UseScratchpad escribe ScratchpadStart/End en 0x494C/0x4950
# tanto en la A124 (0x4C0980) como en SLUS (0x477350). Las unidades se llaman como
# en el .mdebug, Packages/vp6/1.0.3/..., y se compilan del arbol 1.0.6 con
# VP6_VERSION=103, que activa las diferencias de version en la fuente. Lo que solo
# existe en PS2 (scratchpad.c y los *systemdependant.c de ps2/) vive en 1.0.6/.../ps2.
def vp6_ps2(unidad, fuente=None):
    return Object(
        NonMatching,
        "Packages/vp6/1.0.3/source/decode/" + unidad,
        source="Packages/vp6/1.0.6/source/decode/" + (fuente or unidad),
    )


# realmemcard en PS2: las dos PS2 llevan la 3.04.00 del paquete (Packages/realmemcard/3.04.00-layer2 en el .mdebug)
# y GameCube la 3.04.01. Las unidades se llaman como en el .mdebug y se compilan del arbol 3.04.01 con
# REALMEMCARD_VERSION=30400, que activa las diferencias de version en la fuente (firmas sin el nombre del tipo,
# NAME_FILE, SetBootFlow...); lo que es de la plataforma (TitleId de cadena, puerto y ranura, directorios) va con
# EA_PLATFORM_PLAYSTATION2. Los stabs de la A124 dan cada firma y cada campo.
def rmc_ps2(unidad):
    return Object(
        NonMatching,
        "Packages/realmemcard/3.04.00-layer2/source/lib/" + unidad,
        source="Packages/realmemcard/3.04.01-layer2/source/lib/" + unidad,
    )


# eathread va a -O2 como el resto de bibliotecas de EA: el static-init de
# eathread_thread (32 AtomicInt vacios) sale como li/mtctr/bdnz solo a -O2.
cflags_eathread = [*[f for f in cflags_game if f != "-O1"], "-O2"]

Matching = True  # Object matches and should be linked
NonMatching = False  # Object does not match and should not be linked
Equivalent = (
    config.non_matching
)  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


def RenameSectionsFor(versions: tuple[str], renames: tuple[tuple[str]]):
    return renames if config.version in versions else ()


# snd enlaza en dos bloques ordenados cada uno por ruta: el grande (sexithndl.c ..
# supxapf.cpp) y otro de siete unidades, de sgparse.cpp a SNDI_findprime.c, que es
# otro archivo de biblioteca con sus propias opciones. SNDI_root1x.c solo sale con
# las locales del DWARF (p1..p7) sin -ffast-math, que asocia las multiplicaciones;
# las otras seis dan el mismo objeto con y sin la opcion (medido).
cflags_snd_nofastmath = [f for f in cflags_snd if f != "-ffast-math"]

if config.platform != Platform.PS2:
    config.warn_missing_config = True

config.warn_missing_source = False
config.libs = [
    {
        "lib": "Game",
        "toolchain_version": config.linker_version,
        "cflags": [*cflags_game, *vu0_ps2, *keep_inline_gc, *single_float_gc],
        "host": False,
        "progress_category": "game",  # str | List[str]
        "objects": [
            Object(NonMatching, "Speed/Indep/SourceLists/zAI.cpp"),
            # r76b: CWorldAnimEntity::Init 97,98 % (912/912 B) sin la barrera.
            Object(NonMatching, "Speed/Indep/SourceLists/zAnim.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zAttribSys.cpp"),
            # r76b: bList::Sort 94,54 % (172/172 B). 13 formas medidas; el .lreg da
            # los numeros de allocno_compare y ninguna forma de fuente los cambia.
            # bWare sin -fdefault-single-float: el 0.1 de _bOutput es el double exacto (single_float_gc).
            Object(NonMatching, "Speed/Indep/SourceLists/zBWare.cpp", cflags=[*cflags_game, *vu0_ps2, *keep_inline_gc]),
            Object(NonMatching, "Speed/Indep/SourceLists/zCamera.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zComms.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zDebug.cpp"),
            # r76b: Geometry::SphereVsBox 98,49 % (1896/1896 B).
            Object(NonMatching, "Speed/Indep/SourceLists/zDynamics.cpp"),
            # PS2: zEagl4Anim con los 32 registros de la VU0 (-mvu0-use-vf0-vf31, como snd): FnPoseBlender::Eval y
            # EvalSQT solo sacan del bucle sus invariantes con el umbral de loop.c de vf31 (+2 en cada PS2).
            Object(
                NonMatching,
                "Speed/Indep/SourceLists/zEagl4Anim.cpp",
                cflags=cflags_game,
                extra_cflags=[*solo_ps2("-mvu0-use-vf0-vf31"), *keep_inline_gc, *single_float_gc],
            ),
            Object(NonMatching, "Speed/Indep/SourceLists/zEAXSound.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zEAXSound2.cpp"),
            Object(
                NonMatching,
                "Speed/Indep/SourceLists/zEcstasy.cpp",
                # El original usa ps_mul/ps_madds0/psq_l/psq_st explicitos en
                # epCalculateLocalDirectionalPOS16 y en media eDataRender::Render:
                # sin -mps-float el mode(PS) es ilegal y esas ~8 kB no se pueden
                # escribir. Medido: no mueve ni un byte de lo que ya casa.
                cflags=cflags_game,
                extra_cflags=[*solo_gc("-mps-float"), *vu0_ps2, *keep_inline_gc, *single_float_gc],
            ),
            Object(NonMatching, "Speed/Indep/SourceLists/zFe.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zFe2.cpp"),
            # r76b: ProcessPadsForPackage 99,91 % (3080/3080 B) y Update 98,25 %
            # (764/776). La barrera escondia una SENTENCIA que faltaba: con
            # `Pressed = Held = Released = 0;` detras de Mask/JoyMask, como dice el
            # mapa de lineas, se pasa de 33 filas a 14 sin un solo asm.
            Object(NonMatching, "Speed/Indep/SourceLists/zFEng.cpp"),
            Object(
                NonMatching,
                "Speed/Indep/SourceLists/zFoundation.cpp",
                # Unity TU original con -fforce-addr; los VU0_* requieren -mps-float
                # (paired singles via inline asm) y -fvtable-thunks=1.
                cflags=cflags_game,
                extra_cflags=[*solo_gc("-mps-float", "-fvtable-thunks=1"), *vu0_ps2, *keep_inline_gc, *single_float_gc],
            ),
            Object(NonMatching, "Speed/Indep/SourceLists/zGameModes.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zGameplay.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zLua.cpp"),
            # r76b: ConvertWheelRotation 98,84 % (172/172) y PollDevice 99,93 %
            # (1348/1348). Todas las secciones ALLOC y la tabla de simbolos siguen
            # identicas -- solo cambian 10 instrucciones, todas de numero de
            # registro -- pero matched_code es todo-o-nada. DOS de los tres andamios
            # sostenian un 100 % FALSO: metian notas de linea que el original no
            # tiene (cruce .line contra debug_lines.txt).
            Object(NonMatching, "Speed/Indep/SourceLists/zMain.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zMisc.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zMiscSmall.cpp"),
            # r27: segunda unidad de pool puro. Tres piezas: (1) el corte le robaba a
            # zPhysics los primeros 36 B de su .bss (statics value/_.tmp de funciones
            # inline, acreditados por el DWARF del original) -- rango reclamado en
            # splits.txt y pad en keep.lst; (2) MissionEdit.cpp es TODO file-static con
            # las firmas de la A124 (AIEntityCreated lleva 4 parametros): el DWARF de
            # GOWE69 lista las siete emitidas y descartadas (low_pc 0xFFFFFFFF) y
            # ninguna otra unidad del juego las declara; (3) los $LC del pool van a
            # keep.lst (mecanismo zOnline de la r26) y el .data@1 neutraliza el .data
            # vacio. El symrename devuelve el nombre del _GLOBAL_.I que la instanciacion
            # explicita de __node_alloc le roba al TU.
            Object(
                Matching,
                "Speed/Indep/SourceLists/zMission.cpp",
                section_rename=(
                    ".data@1,"
                    "sym:_GLOBAL_.I._S_round_up__Q24_STLt12__node_alloc2b0i0Ui=_GLOBAL_.I.DrawMissionComponents__Fv"
                ),
            ),
            Object(NonMatching, "Speed/Indep/SourceLists/zPhysics.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zPhysicsBehaviors.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zPlatform.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zRender.cpp"),
            # r76b: QuickGame::CreateCars 99,75 % (1912/1908). Lo que falta es UNA
            # instruccion `mr r3,r11`, ranura de sched1 en expand_call.
            Object(NonMatching, "Speed/Indep/SourceLists/zSim.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zSpeech.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zTrack.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zWorld.cpp"),
            Object(NonMatching, "Speed/Indep/SourceLists/zWorld2.cpp"),
            # Overlay units: the original build linked these two objects
            # through *zOnline.o(.over .rodata) / *zFEOverlay.o(.over .rodata),
            # so their code section is .over and not .text (see
            # config/GOWE69/ldscript.ld and the .over ranges in splits.txt).
            # r26: el pool comun de 96 B ya sale de fuente (GAMECUBE|ruta de bware.hpp|bad_alloc|%f,%f,%f|
            # %f,%f,%f,%f|STL) y trypromo da DOL exacto. El .data@1 neutraliza el .data VACIO de nuestro objeto
            # (el original no lo tiene): a alineacion 8 ngcld desplaza el .data de zFEOverlay 4 B y rompe el DOL.
            Object(
                Matching,
                "Speed/Indep/SourceLists/zOnline.cpp",
                section_rename=(
                    ".text=.over,.data@1,"
                    # la instanciacion explicita de __node_alloc roba el nombre del _GLOBAL_.I:
                    # el original lo llama __OSBusClock (la ultima variable no-estatica del TU, os.h:34).
                    # Con la sonda de parseo de stlgc/stl/_alloc.h la instanciacion implicita de
                    # allocate va primera en la cola y el nombre robado es allocate, no _S_round_up.
                    "sym:_GLOBAL_.I.allocate__Q24_STLt12__node_alloc2b0i0Ui=_GLOBAL_.I.__OSBusClock"
                ),
            ),
            # r76b: CustomizeParts::Setup 99,84 % (1708/1708) y
            # CustomizeMain::NotificationMessage 96,61 % (808/808). El pin de
            # vinyl_group_number era HONESTO (la local existe y vive en r21), pero
            # no compila fuera de GameCube. Y el 100 % de NotificationMessage era
            # falso en mapa de lineas: 78 notas contra las 72 del original, y las
            # seis de mas justo donde estaban los andamios; sin ellos, 74.
            Object(
                NonMatching,
                "Speed/Indep/SourceLists/zFeOverlay.cpp",
                section_rename=".text=.over",
                # r48: emitiamos 96 simbolos de mas (7.184 B) que el original no
                # tiene, y el 90 % no los referencia nadie. La bandera deja de
                # emitir las instanciaciones IMPLICITAS de plantilla, que es
                # exactamente lo que son. Medido: .over 148.408 -> 142.528, cero
                # funciones distintas y cero simbolos perdidos.
                extra_cflags=["-fno-implicit-templates"],
            ),
        ],
    },
    {
        "lib": "rcmp",
        "toolchain_version": toolchain_de("rcmp"),
        # PCM3: en WIN32, el bloque /O1 /Oy- de arriba; en GC/PS2, los de la lista.
        "cflags": cflags_rcmp_win32 if config.platform == Platform.WIN32 else [
            # Medido: esta biblioteca NO va con los flags del juego. -O2 (el bucle
            # de NEW_tBigYUVSwizzlerTexture solo sale con mtctr/bdnz a -O2),
            # -mps-float en vez de -mps-nodf (con nodf los guardados FPR salen
            # psq_st en vez de stfd), -fno-force-addr (idctcompute pasa de 63% a
            # 100%: el original rematerializa lis/addi en cada llamada en vez de
            # izar la direccion a un salvado) y SIN -fno-static-dtors (sin eso no
            # se emiten los _GLOBAL_.D. ni la rama destructora).
            # Y SIN -G0: rcmp_vp6_codec, maddec y maddeca direccionan sus
            # globales pequenos por @sda21 en el original (30+ accesos), el
            # delator de la seccion 1 del playbook. Medido: quitarlo vale
            # +188 B y una funcion (el ctor de VP6_CODEC_INTERNAL) y no
            # regresa ninguna de las 13 unidades de la biblioteca.
            # Y SIN -fmove-all-movables, igual que vp6: iza fuera del bucle el
            # `li 0` condicional de AV_PLAYER::GetRCMPChunk a un salvado, que el
            # original deja dentro. Medido: +540 B y una funcion, cero regresion
            # en las 13 unidades y los dos objetos Matching siguen byte a byte.
            *[f for f in cflags_game
              if f not in ("-O1", "-mps-nodf", "-fforce-addr", "-fno-static-dtors", "-G0",
                           "-fmove-all-movables")],
            "-O2",
            *solo_gc("-mps-float"),
            "-fno-force-addr",
            "-fno-implement-inlines",
            # PS2 (SN 2.95.3-136) SI lleva -G0: el original no direcciona nada por
            # $gp. Medido en SLES: +156 B (audioplayer); en SLUS, +156 B.
            *solo_ps2("-G0"),
        ],
        "host": False,
        "progress_category": "libs",  # str | List[str]
        "objects": [
            Object(NonMatching, "egami/rcmp/dev/source/decoder/cmn/rcmpbase.cpp"),
            Object(NonMatching, "egami/rcmp/dev/source/decoder/cmn/rcmp2real.cpp"),
            Object(NonMatching, "egami/rcmp/dev/source/decoder/cmn/rcmp_mad_codec.cpp"),
            Object(
                Matching,
                "egami/rcmp/dev/source/decoder/cmn/rcmp_mad_codec_chunk_types.cpp",
            ),
            Object(
                Matching,
                "egami/rcmp/dev/source/decoder/cmn/rcmp_vp6_codec_chunk_types.cpp",
            ),
            Object(Matching, "egami/rcmp/dev/source/decoder/cmn/maddec.cpp"),
            Object(Matching, "egami/rcmp/dev/source/decoder/cmn/maddeca.cpp"),
            Object(Matching, "egami/rcmp/dev/source/decoder/cmn/madidct.cpp"),
            Object(NonMatching, "egami/rcmp/dev/source/decoder/cmn/rcmp_vp6_codec.cpp"),
            # r76: barrera fuera. GetFirstFrame 98,78 % (656/656 B).
            # av/ sin -ffast-math: el original no reasocia las multiplicaciones por
            # constante (AV_PLAYER::IsTimeForDecode multiplica por 0.001f y luego por
            # el frame rate; con -ffast-math GCC lo reordena). decoder/ si lo lleva:
            # MAD_CODEC_INTERNAL::DecodeChunk baja sin el.
            # PS2: -frelax-128 porque AV_MS_TIMER calcula en `long long` (128 bits en
            # el ee-gcc de SN; ver PS2_TOOLCHAIN). Sin ella no compila; con ella,
            # 56 -> 716 B y 2 -> 10 funciones.
            Object(
                NonMatching,
                "egami/rcmp/dev/source/av/cmn/avplayer.cpp",
                extra_cflags=["-fno-fast-math", *solo_ps2("-frelax-128")],
            ),
            Object(Matching, "egami/rcmp/dev/source/av/cmn/avsubtitle.cpp", extra_cflags=["-fno-fast-math"]),
            Object(NonMatching, "egami/rcmp/dev/source/av/cmn/audioplayer.cpp", extra_cflags=["-fno-fast-math"]),
            Object(NonMatching, "egami/rcmp/dev/source/vd/gc/bigyuvswizzler.cpp"),
            Object(NonMatching, "egami/rcmp/dev/source/vd/gc/bigswizzler.cpp"),
            # PS2: el analizador de cabeceras MPEG-2 del codec de Sony (decoder/cmn/getbits.cpp y gethdr.cpp en el
            # .mdebug de la A124; GameCube no los tiene) y los trozos que reconoce.
            *solo_ps2(
                Object(NonMatching, "egami/rcmp/dev/source/decoder/cmn/getbits.cpp"),
                Object(NonMatching, "egami/rcmp/dev/source/decoder/cmn/gethdr.cpp"),
                Object(NonMatching, "egami/rcmp/dev/source/decoder/ps2/rcmp_ps2_sony_codec_chunk_types.cpp"),
            ),
        ],
    },
    {
        "lib": "vp6",
        "toolchain_version": toolchain_de("vp6"),
        # vp6 no se compilo con -fmove-all-movables ni con -fforce-addr: quitarlos
        # lleva vputil.c y pb_globals.c a mas match y no regresa ninguna unidad.
        # Y va con -mps-float, no con -mps-nodf, igual que rcmp: con nodf los
        # guardados FPR salen psq_st en vez de stfd y el marco encoge 0x10.
        # Ninguna de las 25 unidades regresa; postproc pasa de 62,91% a 85,31%
        # (gaussian 112 B y PlaneAddNoise_C 620 B, imposibles con nodf).
        # criticalpath usa psq_l/psq_st con registros de cuantificacion, que el
        # propio compilador rechaza "without -mps-float enabled".
        "cflags": cflags_vp6,
        "host": False,
        "progress_category": "libs",  # str | List[str]
        "objects": [
            Object(NonMatching, "Packages/vp6/1.0.6/source/decode/cmn/allocator.cpp"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/duck_mem.cpp"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/boolhuff.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/clamp.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/TokenEntropy.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/gc/DSystemDependant.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/gc/doptsystemdependant.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/gc/uoptsystemdependant.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/DeInterlace.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/decodemv.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/recon.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/pb_globals.c", extra_cflags=["-fno-common"]),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/reconstruct.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/loopfilter.c", extra_cflags=["-fno-common"]),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/vputil.c", extra_cflags=["-fno-common"]),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/postproc.c", extra_cflags=["-fno-common"]),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/scale.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/FrameIni.c"),
            Object(NonMatching, "Packages/vp6/1.0.6/source/decode/cmn/vfwpbdll_if.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/DFrameR.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/Huffman.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/borders.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/decodemode.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/decodembs.c"),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/deblock.c", extra_cflags=["-fno-common"]),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/dering.c", extra_cflags=["-fno-common"]),
            Object(Matching, "Packages/vp6/1.0.6/source/decode/cmn/idctpart.c"),
            Object(
                Matching,
                "Packages/vp6/1.0.6/source/decode/cmn/quantize.c",
                cflags=[*cflags_vp6, "-fno-common"],
            ),
            Object(NonMatching, "Packages/vp6/1.0.6/source/decode/cmn/simpledeblocker.c"),
            # r76: despineada en la r75 (9 pines). VP6_DecodeBlock y
            # VP6_PredictFilteredBlock ya no reproducen el objeto (163 B de diferencia
            # en .text, mismo tamano); el enlace usa el extraido.
            # r25: DOL OK. Los 4 B que mide de mas el extraido son padding de
            # alineacion del corte (gap_05_80412774_rodata), no dato: el linker
            # los rellena solo al enlazar el nuestro (trypromo verificado).
            Object(Matching, "Packages/vp6/1.0.6/source/decode/gc/criticalpath.c"),
            # PC: el mismo haz de .inl de cmn en su propia unidad (speed.exe
            # 0x806F24-0x80855B), sin las rutinas MMX/SSE2 de la plataforma, que
            # no tienen fuente en el arbol. Medido en bancopc (MSVC 7.1, los
            # cflags_vp6 de WIN32): casan byte a byte VP6_DecodeBool/128,
            # bitread/bitread1/bitreadonly/bitShift, nDecodeBool,
            # VP6_ExtractTokenN, VP6_DecodeBlockMode, VP6_DecodeMode,
            # VP6_ReconstructBlock y VP6_DecodeMacroBlock.
            *solo_win32(
                Object(NonMatching, "Packages/vp6/1.0.6/source/decode/pc/criticalpath.c"),
            ),
            # PS2 (1.0.3), en el orden de la A124.
            *solo_ps2(
                vp6_ps2("ps2/scratchpad.c"),
                vp6_ps2("cmn/allocator.cpp"),
                vp6_ps2("cmn/pb_globals.c"),
                vp6_ps2("cmn/postproc.c"),
                vp6_ps2("cmn/quantize.c"),
                vp6_ps2("cmn/simpledeblocker.c"),
                vp6_ps2("cmn/vfwpbdll_if.c"),
                vp6_ps2("cmn/vputil.c"),
                vp6_ps2("ps2/doptsystemdependant.c"),
                vp6_ps2("ps2/DSystemDependant.c"),
                vp6_ps2("ps2/uoptsystemdependant.c"),
                vp6_ps2("cmn/duck_mem.cpp"),
                vp6_ps2("cmn/boolhuff.c"),
                vp6_ps2("cmn/borders.c"),
                vp6_ps2("cmn/clamp.c"),
                vp6_ps2("cmn/deblock.c"),
                vp6_ps2("cmn/decodembs.c"),
                vp6_ps2("cmn/decodemode.c"),
                vp6_ps2("cmn/decodemv.c"),
                vp6_ps2("cmn/DeInterlace.c"),
                vp6_ps2("cmn/dering.c"),
                vp6_ps2("cmn/DFrameR.c"),
                vp6_ps2("cmn/FrameIni.c"),
                vp6_ps2("cmn/Huffman.c"),
                vp6_ps2("cmn/loopfilter.c"),
                vp6_ps2("cmn/reconstruct.c"),
                vp6_ps2("cmn/scale.c"),
                vp6_ps2("cmn/TokenEntropy.c"),
                # El criticalpath de PS2 comparte los .inl de cmn con el de GameCube y lleva los suyos de
                # ps2/ (filtro, IDCT y reconstruccion en MMI, el asm del original).
                vp6_ps2("ps2/criticalpath.c"),
                vp6_ps2("cmn/recon.c"),
            ),
        ],
    },
    {
        "lib": "snd",
        "toolchain_version": toolchain_de("snd"),
        "cflags": cflags_snd,
        "host": False,
        "progress_category": "libs",  # str | List[str]
        "objects": [
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/saems.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/saemsamb.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/saemsmbf.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/saemsmbm.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/saemstimupdt.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/salloc.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sattrdef.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sbadd.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sballoc.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sbasync.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sbasyncm.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sbhdrcpy.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sbhdrsze.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sbplay.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sbremove.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sbvalid.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/scheckpo.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sclnt100.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sctrldry.cpp"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sdata.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sfxlevel.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/slowpass.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/smemcpy.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/smemdis.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/smemlmt.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/smemlu.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/smemman.c"),
            # NOTE: upstream lo marca Matching, pero compila a 0x25c frente a los
            # 0x210 del original ("kinda match") y desplaza todo el .text
            # posterior, rompiendo el sha1 del DOL. NonMatching hasta que case.
            Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/cmn/sndfxbus.cpp"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/spatkey.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/spitch.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sgetpvol.c"),
            Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/gc/sscalcfx.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/gc/ssdfx.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/gc/sdspmix.c"),
            Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/cmn/seffect.c"),
            Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/cmn/sevent.cpp"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfxrevc.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sgetdata.c"),
            Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/cmn/sinitdts.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/smixer.c"),
            # r76: despineada. SNDDRV_audiocallback 99,56 % (400/400 B): el DWARF no
            # tiene `mixSource` y r4 es el parametro `context`. Tampoco tiene bloques
            # lexicos, asi que se fueron tambien las llaves que daban ambito al pin.
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/gc/snddrv.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/spktplay.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/spktctoh.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/splysdef.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/spoutlat.c"),
            # r76: barrera fuera. iSNDrandom 98,33 % (260/264 B).
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/srandom.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sreallocbuf.cpp"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/srender.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sresopat.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sserver.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/ssine.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sst.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sst3dpos.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstautov.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstcrtap.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstfxlev.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstgetrp.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstgetpv.c"),
            Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/cmn/ssthighp.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/ssthold.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstlowp.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstop.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstovrhd.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstpmult.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstqmem.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstqreqi.c"),
            Object(
                Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstrmdry.cpp"
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstrstat.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstsetgl.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sststat.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/ssttmul.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstvol.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/ssys.cpp"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/ssysinit.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/ssysserv.c"),
            Object(
                Matching,
                "Speed/Indep/Libs/snd/9/source/library/cmn/ssysveccsismutex.cpp",
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/stagpat.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/stimemul.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/stimerem.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/stpparse.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/stretch.c"),
            Object(
                Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/svecreal.cpp"
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/svol.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sx87d16.c"),
            Object(
                Matching,
                "Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/coda.cpp",
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/s3dlow.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/gc/saramman.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/satospkr.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sclcptch.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sctlfilt.c"),
            Object(
                Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sdownmix.cpp"
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfamplf.c"),
            Object(
                Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfbpffir8.c"
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfecho.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfft24.c"),
            Object(
                Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfhpffir8.c"
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfilter.c"),
            # r76: los DOS asm fuera. calcFIRCoeffs 89,25 % (940/936 B). Hay formas
            # sin asm mejores ya medidas (95-97 %) pero exigen subir la escritura
            # `halfLen = 4`, y eso puede cambiar que coeficiente se resta: pendiente
            # de comprobar la equivalencia antes de adoptarlas.
            Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/mix/sfir.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfir8.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sflpf.c"),
            Object(
                Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sflpffir8.c"
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfmixer.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfreson.c"),
            # r76: despineada. SFILTER_splitter 99,53 % (256/256 B): el DWARF no tiene
            # `copySource` ni `copySize`; faltan las 3 cargas de argumento, que rs6000
            # emite r3->r4->r5 y el objetivo deja r3 para el final.
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfsplit.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfsrc.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sgettag.c"),
            Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/mix/shipass.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sinit16.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sinitut.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sinitxa.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/slib.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/slinklst.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/slinkmix.c"),
            Object(
                Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/smemhigh.cpp"
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/smixc.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/smixfram.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/smixhip.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/smixlowp.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/smixptch.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/smixtmul.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/SNDI_cos.c"),
            Object(
                Matching,
                "Speed/Indep/Libs/snd/9/source/library/cmn/SNDI_mult16.c",
                cflags=cflags_snd_nofastmath,
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/snd/9/source/library/cmn/SNDI_root1x.c",
                cflags=cflags_snd_nofastmath,
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/SNDI_sin.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sover.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/spantoaz.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/spat2hdr.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/srrange.c"),
            Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/cmn/sstopall.c"),
            Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/cmn/ssysreal.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/supf.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/suplf.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/supmutf.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/supmutlf.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/supmutpf.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/suppf.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/supxaf.cpp"),
            Object(
                Matching, "Speed/Indep/Libs/snd/9/source/library/mix/supxalf.cpp"
            ),
            Object(
                Matching, "Speed/Indep/Libs/snd/9/source/library/mix/supxapf.cpp"
            ),
            # PS2: decodexac va en ensamblador de EA (eaxadecf_ps2.s, ronda 18); GC sigue con el C.
            *solo_gc(
                Object(
                    Matching,
                    "Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/eaxadecf.cpp",
                    cflags=cflags_snd_nofastmath,
                ),
            ),
            *solo_ps2(
                Object(
                    Matching,
                    "Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/eaxadecf.cpp",
                    source="Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/eaxadecf_ps2.s",
                ),
            ),
            # PCM3: en PC va el C como en GameCube, con los flags de snd de PC
            # (/O1 /TP): casan sus 4 funciones de speed.exe (0x828B03-0x828CE9).
            *solo_win32(
                Object(
                    NonMatching,
                    "Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/eaxadecf.cpp",
                    cflags=cflags_snd_nofastmath,
                ),
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/mtdecf.cpp",
                cflags=cflags_snd_nofastmath,
            ),
            Object(
                Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sexithndl.c"
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/sfrsf.c"),
            Object(
                Matching,
                "Speed/Indep/Libs/snd/9/source/library/cmn/sgparse.cpp",
                cflags=cflags_snd_nofastmath,
            ),
            Object(
                MatchingFor("GOWE69"),
                "Speed/Indep/Libs/snd/9/source/library/cmn/SNDI_findprime.c",
                cflags=cflags_snd_nofastmath,
            ),
            Object(
                MatchingFor("GOWE69"),
                "Speed/Indep/Libs/snd/9/source/library/cmn/sbpatinf.c",
            ),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/scrsfl.c", cflags=cflags_snd_nofastmath),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/smixfilt.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/mix/smixvec.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sformat.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/soutputmap.cpp"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/sprofvoc.c"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/cmn/author.cpp"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/mpeghufftables.cpp"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/mpegl3base.cpp"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/setmemcpy.cpp"),
            # coda music codec extras (identified from DWARF1 CU map):
            # PPCMtdec (0x80311BE4) and lbmpeg.cpp _GLOBAL_ ctor (0x80366298)
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/mtdecppc.cpp"),
            Object(Matching, "Speed/Indep/Libs/snd/9/source/library/extern/coda/cmn/lbmpeg.cpp"),
            # PS2: el driver de snd 9.06.00-ps2 (library/ps2/sdriver.c en el .mdebug de la A124; la unidad de
            # config/ se llama cmn/sdriver.c). Usa sifcmd.h, sceSifCheckStatRpc y sceGsGetGParam del SDK del arbol.
            *solo_ps2(
                Object(
                    NonMatching,
                    "Speed/Indep/Libs/snd/9/source/library/cmn/sdriver.c",
                    source="Speed/Indep/Libs/snd/9/source/library/ps2/sdriver.c",
                ),
            ),
            # PS2: el resto de la plataforma de snd 9.06.00-ps2 (library/ps2/*.c, cmn/dma.cpp y mix/sfft16.c en el
            # .mdebug de la A124; config/ los llama cmn/<fichero>). smemcpy.cpp y svecreal6.cpp son en PS2 los
            # cmn/smemcpy.c y cmn/svecreal.cpp de GameCube (smemcpy lleva en PS2 el ensamblador de EA con las MMI).
            *solo_ps2(
                *[
                    Object(
                        NonMatching,
                        f"Speed/Indep/Libs/snd/9/source/library/cmn/{f}.c",
                        source=f"Speed/Indep/Libs/snd/9/source/library/ps2/{f}.c",
                    )
                    for f in (
                        "sd3dpos",
                        "sdasync",
                        "sdfilter",
                        "sdfx",
                        "sdgetcf",
                        "sdmaee",
                        "sdmemlmt",
                        "sdmemlu",
                        "sdmemman",
                        "sdoutlat",
                        "sdpacket",
                        "sdplapat",
                        "sdrespat",
                    )
                ],
                Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/cmn/dma.cpp"),
                Object(
                    NonMatching,
                    "Speed/Indep/Libs/snd/9/source/library/cmn/sfft16.c",
                    source="Speed/Indep/Libs/snd/9/source/library/mix/sfft16.c",
                ),
                Object(
                    NonMatching,
                    "Speed/Indep/Libs/snd/9/source/library/cmn/smemcpy.cpp",
                    source="Speed/Indep/Libs/snd/9/source/library/cmn/smemcpy.c",
                ),
                Object(
                    NonMatching,
                    "Speed/Indep/Libs/snd/9/source/library/cmn/svecreal6.cpp",
                    source="Speed/Indep/Libs/snd/9/source/library/cmn/svecreal.cpp",
                ),
            ),
        ],
    },
    {
        # EA path music library (C:/packages/path/5.01.04) - .text 0x80374530..0x8037D268
        "lib": "path",
        "toolchain_version": toolchain_de("path"),
        # -fno-implement-inlines: barrido unidad a unidad (r30 §2.1);
        # condicion de la promocion de pathreal (532 -> 496 = objetivo).
        "cflags": [*cflags_path, *([] if config.platform == Platform.WIN32 else ["-fno-implement-inlines"])],
        "host": False,
        "progress_category": "libs",  # str | List[str]
        "objects": [
            Object(
                Matching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathcontrol.cpp"
            ),
            Object(Matching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathevent.cpp"
            ),
            Object(NonMatching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathinit.cpp"
            ),
            Object(Matching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathreal.cpp"),
            # r79: sin el primer de "%s  %s" (literal muerto de origen desconocido).
            Object(NonMatching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathreal6.cpp"),
            Object(Matching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathserv.cpp"),
            # r76: barrera fuera. CreateStreamTrack 99,85 % (404/404 B). r25: ya al
            # 100 % (code+data), DOL verificado con trypromo.
            Object(Matching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathsnd.cpp"),
            Object(NonMatching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathtrack.cpp"
            ),
            # r79: sin el primer de "%s  %s" (literal muerto de origen desconocido).
            Object(NonMatching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathvol.cpp"),
            Object(NonMatching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathaction.cpp"
            ),
            Object(NonMatching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathbank.cpp"),
            Object(
                Matching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathdebug.cpp"
            ),
            # r76: despineada en la r75 (8 pines). PATHI_calcwaitbeat ya no reproduce
            # el objeto (77 B de diferencia en .text, mismo tamano).
            Object(
                NonMatching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathnode.cpp"
            ),
            Object(Matching, "Speed/Indep/Libs/path/5.01.04/source/cmn/pathrand.cpp"),
        ],
    },
    {
        # EA speech library (D:/perforce/audio/libs/SPCH) - .text 0x8037D26C..0x80381654
        "lib": "spch",
        "toolchain_version": toolchain_de("spch"),
        "cflags": cflags_spch,
        "host": False,
        "progress_category": "libs",  # str | List[str]
        "objects": [
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/spchbank.c"),
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/spchdata.c"),
            # r25: promovida reclamando su .data (0x80451F00-08, los statics
            # gAddEventStatus/gSPCHPlayStatus) del comodin auto_06_80451F00.
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/spchevnt.c"),
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/spchinit.c"),
            # r25: promovida reclamando su .data (multiple_80451F08) y su .bss
            # (gEventChoice_804CC900) del comodin.
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/spchpick.c"),
            # r76: barrera fuera. iSPCH_EACrandom 98,33 % (260/264 B).
            # r25: los 3 simbolos que "sobran" (iSPCH_AddRandToQueue y cia) no
            # desplazan nada: DOL OK con trypromo.
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/spchrand.c"),
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/spchrslv.c"),
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/spchrule.c"),
            # r76: las dos barreras fuera. iSPCH_GetSampleSizeData 87,65 % (136/136 B).
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/spchsamp.c"),
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/sptick.c"),
            Object(Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/sputil.c"),
            Object(
                Matching, "Speed/Indep/Libs/spch/dev/source/library/cmn/spchcsis.cpp"
            ),
        ],
    },
    {
        "lib": "realcore",
        "toolchain_version": toolchain_de("realcore"),
        # NOTE: sin -fno-static-dtors: las TU realcore con objetos globales
        # polimorficos (filesys, dvd_device, hd_device) SI emiten _GLOBAL_.D.
        # PS2 (SN 2.95.3-136) va con -G0 y no -G8: el original no direcciona nada
        # por $gp. Medido: +900 B en SLES y +180 B en SLUS.
        # PCM3: en WIN32, el bloque /O1 /Oy- de arriba (cflags_realcore_win32).
        "cflags": cflags_realcore_win32 if config.platform == Platform.WIN32 else [f for f in cflags_game if f not in ("-G0", "-fforce-addr", "-fno-static-dtors")]
        + [
            "-G0" if config.platform == Platform.PS2 else "-G8",
            "-fstrength-reduce",
            "-fno-implement-inlines",
            # GameCube compila realcore a -O2: FILEOPERATION::AddToQueue en la forma del DWARF solo casa asi
            # (expand_end_loop solo enrolla el bucle con -O2), y las otras 42 unidades salen iguales a -O1 y a -O2.
            *solo_gc("-O2"),
        ],
        "host": False,
        "progress_category": "libs",  # str | List[str]
        "objects": [
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/cmn/device.cpp",
            ),
            # FILEOPERATION guarda sus 64 bits en int64_t/uint64_t (los tipos de los stabs;
            # en el ee-gcc de SN `long long` es de 128 bits): ya no hace falta
            # -frelax-128 (ventana 4, medido igual).
            Object(
                NonMatching,  # r76: despineada (AddToQueue 252->248 B)
                "Speed/Indep/Libs/realcore/6.24.00/source/file/cmn/filesys.cpp",
                # PCM6 (WIN32): los nombres POSIX por sus de MSVC + el _alloca
                # declarado; 34 fn al 100 % en banco.
                extra_cflags=solo_win32(
                    "/Dstrcasecmp=_stricmp", "/D__builtin_alloca=_alloca", "/FImalloc.h",
                    # r82 (PC-A): /Oi. FILEOPERATION::SetName (0x7ED087) expande strlen en linea; el banco de
                    # filesys da 40/40 con y sin /Oi y SetName casa con el.
                    "/Oi",
                ),
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/file/cmn/filesysopts.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/file/cmn/syncfile.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/file/cmn/hlafile.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/file/cmn/hlsfile.cpp",
            ),
            # r76: degradada para que su objeto EXTRAIDO exporte `gCurRead`, que el
            # snddrv extraido referencia 10 veces. En el original ese dato NO era
            # static (un static de otra unidad no se puede referenciar desde C); en
            # symbols.txt pasa a scope:global. Nuestro fuente se queda como estaba:
            # quitarle el `static` sube la alineacion de SECCION de .bss de 8 a 32 y
            # corre 16 B el .bss del DOL (medido con shift.py).
            Object(
                NonMatching,
                "Speed/Indep/Libs/realcore/6.24.00/source/file/gc/dvd_device.cpp",
            ),
            # r79: sin el relleno de .bss con __attribute__((section)).
            Object(
                NonMatching,
                "Speed/Indep/Libs/realcore/6.24.00/source/file/gc/hd_device.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/cmn/event.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/cmn/effect.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/cmn/itimer.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/cmn/memvectors.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/cmn/eventqueue.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/cmn/effectimp.cpp",
            ),
            Object(NonMatching, "Speed/Indep/Libs/realcore/6.24.00/source/input/cmn/interface.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/cmn/interfaceimp.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/gc/gc_device.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/gc/gc_effect.cpp",
            ),
            Object(NonMatching, "Speed/Indep/Libs/realcore/6.24.00/source/input/gc/gc_interface.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/input/gc/gc_pad.cpp",
            ),
            Object(
                NonMatching,
                "Speed/Indep/Libs/realcore/6.24.00/source/file/cmn/bigfile.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/file/cmn/filesys_c.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/file/cmn/filesys_cc.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/std/cmn/locale.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/file/gc/device.cpp",
            ),
            Object(NonMatching, "Speed/Indep/Libs/realcore/6.24.00/source/system/cmn/systemvars.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/system/cmn/timer.cpp",
            ),
            Object(NonMatching, "Speed/Indep/Libs/realcore/6.24.00/source/system/cmn/systask.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/system/gc/threads.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/system/gc/signals.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/debug/cmn/printstr.cpp",
                extra_cflags=solo_win32("/Dvsnprintf=_vsnprintf"),  # PCM6
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/debug/cmn/abortmsg.cpp",
                extra_cflags=solo_win32("/Dvsnprintf=_vsnprintf"),  # PCM6
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/std/cmn/memclear.cpp",
            ),
            Object(
                NonMatching, "Speed/Indep/Libs/realcore/6.24.00/source/std/cmn/exit.cpp"
            ),
            Object(NonMatching, "Speed/Indep/Libs/realcore/6.24.00/source/system/gc/timerthread.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/std/gc/memfill.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/std/gc/memcopy.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/system/cmn/mutex.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/system/cmn/exit.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/system/gc/initvblt.cpp",
            ),
            Object(NonMatching, "Speed/Indep/Libs/realcore/6.24.00/source/system/gc/inittmr.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/debug/cmn/printvstr.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/debug/cmn/debugger.cpp",
            ),
            Object(
                Matching,
                "Speed/Indep/Libs/realcore/6.24.00/source/debug/gc/printdrv.cpp",
            ),
            # PS2: la plataforma de realcore 6.24.00 (source/*/ps2/*.cpp en el .mdebug de la A124). addtimer y
            # ps2_initmtap solo los enlaza la A124; addtimer vive en system/cmn/ y config/ lo llama ps2/.
            *solo_ps2(
                *[
                    Object(NonMatching, f"Speed/Indep/Libs/realcore/6.24.00/source/{f}.cpp")
                    for f in (
                        "system/ps2/threads",
                        "system/ps2/signals",
                        "system/ps2/inittmr",
                        "system/ps2/videomode",
                        "std/cmn/memfill",
                        "std/cmn/memcopy",
                        "file/ps2/host0_device",
                        "file/ps2/cddvd_device",
                        "input/ps2/ps2_initmtap",
                        "input/ps2/ps2_effect",
                        "input/ps2/ps2_interface",
                        "input/ps2/ps2_pad",
                        "input/ps2/ps2_device",
                    )
                ],
                Object(
                    NonMatching,
                    "Speed/Indep/Libs/realcore/6.24.00/source/system/ps2/addtimer.cpp",
                    source="Speed/Indep/Libs/realcore/6.24.00/source/system/cmn/addtimer.cpp",
                ),
            ),
        ],
    },
    {
        "lib": "realmemcard",
        "toolchain_version": toolchain_de("realmemcard"),
        # PCM3: en WIN32, el bloque /O1 /Oy- de arriba; en GC/PS2, los de la lista.
        "cflags": cflags_realmemcard_win32 if config.platform in (Platform.WIN32, Platform.X360) else [
            # Esta biblioteca NO lleva -G0 (sus estaticos viven en .sdata y se
            # leen con @sda21) ni -fforce-addr (accede a los globales sin
            # materializar la direccion en un registro salvado).
            # Y TAMPOCO -fno-static-dtors: sin el, el static-init de gc_interface
            # emite la rama destructora y pasa de 84,29% a 86,67% CON LA COLA BYTE
            # A BYTE. Medido en las 15 unidades de la biblioteca: cero regresiones.
            *[f for f in cflags_game
              if f not in ("-G0", "-fforce-addr", "-fno-static-dtors")],
            # Y se compilo con wchar_t de 16 bits y sin signo: sus simbolos
            # manglan `Uw` (unsigned wchar_t), no `w`.
            "-fshort-wchar",
            # Lleva reduccion de fuerza: sus bucles sobre arrays avanzan
            # punteros en vez de indexar con lwzx/stwx.
            "-fstrength-reduce",
            "-I src/Packages/realmemcard/3.04.01-layer2/include/common",
            "-fno-implement-inlines",
            # En PS2, -G0 como el resto del middleware de SN, y la 3.04.00 (ver rmc_ps2).
            *solo_ps2("-G0", "-DREALMEMCARD_VERSION=30400"),
        ],
        "host": False,
        "progress_category": "libs",  # str | List[str]
        "objects": [
            Object(
                Matching,
                "Packages/realmemcard/3.04.01-layer2/source/lib/cmn/memcard_interface.cpp",
            ),
            Object(
                Matching,
                "Packages/realmemcard/3.04.01-layer2/source/lib/cmn/memcard_interface_impl.cpp",
            ),
            # r25: promovida reclamando su .rodata (0x80414400-08) del comodin.
            Object(Matching, "Packages/realmemcard/3.04.01-layer2/source/lib/cmn/memcard_utilities.cpp",
            ),
            Object(
                Matching,
                "Packages/realmemcard/3.04.01-layer2/source/lib/cmn/interfaceimp.cpp",
            ),
            Object(
                Matching,
                "Packages/realmemcard/3.04.01-layer2/source/lib/gc/gc_blockcalculator.cpp",
            ),
            Object(
                Matching,
                "Packages/realmemcard/3.04.01-layer2/source/lib/gc/gc_memcard_taskmanager.cpp",
            ),
            Object(Matching, "Packages/realmemcard/3.04.01-layer2/source/lib/cmn/memcard_memvectors.cpp",
            ),
            Object(
                Matching,
                "Packages/realmemcard/3.04.01-layer2/source/lib/cmn/memcard_taskmanager.cpp",
                extra_cflags=solo_win32("/Dstrncasecmp=_strnicmp"),  # PCM6
            ),
            # gc_driver va a -O2 (el resto de la biblioteca, a -O1). ReadFile declara `readHead`
            # dentro del cuerpo del `while`, como dice el DWARF (bloque 0x8038ED14-0x8038EDF0: justo
            # el cuerpo rotado), y GCC solo rota un bucle con notas de bloque a -O2 (stmt.c,
            # expand_end_loop). Medido: -O2 en toda la biblioteca solo pierde TrcSingletonAssert
            # (gc_interface conserva una comparacion muerta, que es de -O1).
            Object(
                Matching,
                "Packages/realmemcard/3.04.01-layer2/source/lib/gc/gc_driver.cpp",
                extra_cflags=solo_gc("-O2"),
            ),
            Object(NonMatching, "Packages/realmemcard/3.04.01-layer2/source/lib/gc/gc_interface.cpp",
            ),
            Object(NonMatching, "Packages/realmemcard/3.04.01-layer2/source/lib/gc/public.cpp",
            ),
            Object(
                Matching,
                "Packages/realmemcard/3.04.01-layer2/source/lib/gc/trctasks.cpp",
            ),
            Object(NonMatching, "Packages/realmemcard/3.04.01-layer2/source/lib/gc/tasks.cpp",
            ),
            Object(Matching, "Packages/realmemcard/3.04.01-layer2/source/lib/cmn/locale.cpp"),
            Object(Matching, "Packages/realmemcard/3.04.01-layer2/source/lib/gc/gc_memcard_interface_impl.cpp"),
            # PS2 (3.04.00), en el orden de la A124.
            *solo_ps2(
                rmc_ps2("cmn/locale.cpp"),
                rmc_ps2("cmn/memcard_interface.cpp"),
                rmc_ps2("cmn/memcard_interface_impl.cpp"),
                rmc_ps2("cmn/memcard_memvectors.cpp"),
                rmc_ps2("cmn/memcard_taskmanager.cpp"),
                rmc_ps2("ps2/ps2_memcard_interface_impl.cpp"),
                rmc_ps2("ps2/ps2_memcard_taskmanager.cpp"),
                rmc_ps2("ps2/ps2_sjis.cpp"),
                rmc_ps2("cmn/memcard_utilities.cpp"),
                rmc_ps2("ps2/ps2_initmtap.cpp"),
                rmc_ps2("ps2/ps2_interface.cpp"),
                rmc_ps2("ps2/ps2_internaltasks.cpp"),
                rmc_ps2("ps2/ps2_public.cpp"),
                rmc_ps2("ps2/ps2_tasks.cpp"),
                rmc_ps2("ps2/ps2_trctasks.cpp"),
                rmc_ps2("cmn/interfaceimp.cpp"),
                rmc_ps2("ps2/ps2_blockcalculator.cpp"),
            ),
        ],
    },
]


def ps2_libc(fuente, carpeta="libc/"):
    """Una unidad de Speed/PSX2/bware/sce/ee/gcc/ee compilada de la fuente de GameCube (src/libc)."""
    return Object(NonMatching, "Speed/PSX2/bware/sce/ee/gcc/ee/" + fuente, source=carpeta + fuente)


if config.platform == Platform.PS2:
    config.libs.extend(
        [
            {
                # csis (C:/audio/libs/csis/dev en el .mdebug), la misma fuente que GameCube. El cerrojo de
                # PS2 es un semaforo del kernel con dueno y cuenta (gMutexOwner, gMutexLocks y gMutexHandle
                # en los stabs de la A124). Flags: los de GameCube (sin -fforce-addr, que PS2 no lleva).
                "lib": "csis",
                "toolchain_version": toolchain_de("csis"),
                "cflags": [*cflags_game, "-fno-implement-inlines"],
                "host": False,
                "progress_category": "libs",
                "objects": [
                    Object(NonMatching, "Speed/Indep/Libs/csis/dev/source/library/cmn/csis.cpp"),
                ],
            },
            {
                # eathread de PS2 (r50): el backend source/ps2 escrito del asm troceado
                # de la A124 y la SLUS (las dos traen las mismas unidades). Como el resto
                # del middleware de SN: -G0 y -O2. El .cpp lleva -x c++ implicito.
                "lib": "eathread",
                "toolchain_version": toolchain_de("eathread"),
                "cflags": [
                    *cflags_base_prodg,
                    "-G0",
                    "-O2",
                    "-I src/Packages/eathread/1.1.0/include",
                ],
                "host": False,
                "progress_category": "libs",
                "objects": [
                    Object(
                        NonMatching,
                        "Packages/eathread/1.1.0/source/eathread_semaphore.cpp",
                        extra_cflags=["-g0"],
                    ),
                    Object(
                        NonMatching,
                        "Packages/eathread/1.1.0/source/eathread_thread.cpp",
                        extra_cflags=["-g0"],
                    ),
                ],
            },
            {
                # dirtysock (libdirtysock.a en el mapa de la A124, d:\...\indep\libs\dirtysock):
                # el middleware de red de EA, en C, con el ee-gcc de SN del middleware
                # (prologo con sq para los s y sd para ra). Solo enlaza el prototipo: SLUS
                # no tiene la biblioteca. hlbudapi.c escrito de su asm (ronda 43).
                # Sin -g (ronda 44, medido): el original no trae ni un $Lscope (nm: 0) y con
                # el -g2 de PS2 GCC saca la direccion de las funciones por su alias local
                # ($Lscope103 en vez de _HLBOpCallback). El -g0 va el ultimo y anula -g2.
                "lib": "dirtysock",
                "toolchain_version": toolchain_de("dirtysock"),
                "cflags": [
                    *cflags_base_prodg,
                    "-G0",
                    "-O2",
                    "-x c",
                    "-I src/Speed/Indep/Libs/dirtysock/include",
                ],
                "host": False,
                "progress_category": "libs",
                "objects": [
                    *solo_a124(
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/dirtysock/source/library/ps2/final/hlbudapi.c",
                            extra_cflags=["-g0"],
                        ),
                        # r45: buddyapi.c escrito de su asm (tanda 1: formas medida
                        # del asm; el .mdebug no trae esta lib).
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/dirtysock/source/library/ps2/final/buddyapi.c",
                            extra_cflags=["-g0"],
                        ),
                        # r48: lobbyapi.c y weboffer.c escritos de su asm (tanda 1).
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/dirtysock/source/library/ps2/final/lobbyapi.c",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/dirtysock/source/library/ps2/final/weboffer.c",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/dirtysock/source/library/ps2/final/lobbytagfield.c",
                            extra_cflags=["-g0"],
                        ),
                        # r11-decomp: lobbyapilist.c (6 fn al 100 %) y lobbyapiutil.c (1 fn) escritas
                        # de su asm (banco r52, informe 2026-10-13_m2-dirtysock).
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/dirtysock/source/library/ps2/final/lobbyapilist.c",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/dirtysock/source/library/ps2/final/lobbyapiutil.c",
                            extra_cflags=["-g0"],
                        ),
                        # retomada dirtysock-2: lobbyapiasync.c escrita de su asm
                        # (14/20 fn al 100 % en banco r52; informe 2026-10-13_dirtysock-2).
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/dirtysock/source/library/ps2/final/lobbyapiasync.c",
                            extra_cflags=["-g0"],
                        ),
                    ),
                ],
            },
            {
                # juice 1.5.2a (Speed/Indep/Libs/juice en el mapa de la A124): la biblioteca de
                # automatizacion de EA, en C++. Al contrario que el middleware de SN, se compilo
                # con el ee-gcc 2.9-ee-991111 del juego (guarda los s con sd, no con sq; con el
                # 136 JuiceMutex no casa una funcion) y con RTTI (los __tf llaman a __rtti_user),
                # sin excepciones (con ellas, el Unlock de JuiceMutex pierde la llamada en cola a
                # SignalSema: 73 %) y sin -G0 (el __ti, la guarda del estatico local de
                # JuiceRandom y sCurrentNetExtention van por $gp desde .sdata). Sin -g como
                # dirtysock: el original no trae ni un $Lscope. Solo enlaza el prototipo: en SLUS
                # el bloque de Juice queda anonimo, sin trocear.
                "lib": "juice",
                "toolchain_version": config.linker_version,
                "cflags": [
                    *cflags_base_prodg,
                    "-O2",
                    "-fno-exceptions",
                ],
                "host": False,
                "progress_category": "libs",
                "objects": [
                    *solo_a124(
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceUtil.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceMutex.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceRandom.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceNetwork.cpp",
                            extra_cflags=["-g0"],
                        ),
                        # ronda 2 (informe 2026-10-13_p2juice2): Processor (la base, con el
                        # vptr a +0x0C del ABI viejo) y los gestores derivados pequenos,
                        # escritos de su asm modulo a modulo. JuiceMonitoring queda a 8/9
                        # (el __tf de MonitoredVariable no se emite sin un ctor que el
                        # original no tiene) y JuiceExceptionPS2 a 3/6 (LocateCaller
                        # 96,1 %, GetExceptionType 95,2 %); las demas cierran enteras.
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceProcessor.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceOnlinePlayMgr.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceRemoteFileManager.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceDebugSystem.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceMonkey.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceRPCManager.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceScreenshotMgr.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceErrorManager.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceLogging.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceMonitoring.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuicePadConfigManager.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuicePad.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceCoverage.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceExceptionPS2.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceExtension.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceComChannel.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceThreadManager.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceSystemManager.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceScripting.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceCom.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/Juice.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceReplayManager.cpp",
                            extra_cflags=["-g0"],
                        ),
                        Object(
                            NonMatching,
                            "Speed/Indep/Libs/juice/1.5.2a/source/ps2/JuiceDefaultScripting.cpp",
                            extra_cflags=["-g0"],
                        ),
                    ),
                ],
            },
            {
                # La libc de SN para PS2 (Speed/PSX2/bware/sce/ee/gcc/ee): el mismo newlib 1.8 de SN que la de
                # GameCube (src/libc), con las cabeceras de newlib del EE y el ee-gcc 2.95.3 de SN. Las
                # funciones de cadena optimizadas con MMI (strcpy, memcpy...) no son de aqui. SN la compilo con las
                # funciones internas de GCC (el PAD de vfprintf copia sus 16 B con ldl/sdl: memcpy en linea) y con
                # wchar_t de 16 bits (-fshort-wchar: las L"..." de vfwprintf son de 2 B por caracter).
                "lib": "libc_sn",
                "toolchain_version": toolchain_de("libc_sn"),
                "ee_as": "Ps2EeAs",
                "cflags": [*cflags_libc, "-G0", "-fshort-wchar"],
                "host": False,
                "progress_category": "libc",
                "objects": [
                    ps2_libc("fclose.c"),
                    ps2_libc("fflush.c"),
                    ps2_libc("findfp.c"),
                    ps2_libc("fopen.c"),
                    ps2_libc("fprintf.c"),
                    ps2_libc("fseek.c"),
                    ps2_libc("fwalk.c"),
                    ps2_libc("makebuf.c"),
                    # lflush y __srefill: en GameCube el troceado llama wsetup a esa unidad.
                    Object(NonMatching, "Speed/PSX2/bware/sce/ee/gcc/ee/refill.c", source="libc/wsetup.c"),
                    ps2_libc("sprintf.c"),
                    ps2_libc("stdio.c"),
                    ps2_libc("vprintf.c"),
                    ps2_libc("vsprintf.c"),
                    # vfprintf.c con itoa.c dentro, y otra vez con INTEGER_ONLY (vfprintf_1.c): _vfiprintf_r.
                    ps2_libc("vfprintf.c"),
                    Object(NonMatching, "Speed/PSX2/bware/sce/ee/gcc/ee/vfiprintf.c", source="libc/vfprintf_1.c"),
                    ps2_libc("atexit.c", ""),
                    ps2_libc("qsort.c", ""),
                    ps2_libc("strcasecmp.c"),
                    ps2_libc("strcoll.c"),
                    ps2_libc("strcspn.c"),
                    ps2_libc("locale.c"),
                    ps2_libc("closer.c"),
                    ps2_libc("fstatr.c"),
                    ps2_libc("lseekr.c"),
                    ps2_libc("openr.c"),
                    ps2_libc("readr.c"),
                    ps2_libc("writer.c"),
                    ps2_libc("math_support.c"),
                ],
            },
            {
                # La libm es la de la toolchain de Sony (/usr/local/sce/ee/gcc/build/ee/newlib/libm en el
                # .mdebug de la A124): newlib tal cual, compilado con el ee-gcc 2.9-ee-991111 del juego
                # (guarda los s con sd). Las que llevan div.s no casan: su ensamblador rellena con dos nop
                # delante de cada div.s y el nuestro no.
                "lib": "libm_sce",
                "toolchain_version": config.linker_version,
                "cflags": [*cflags_libc, "-G0", "-fno-builtin"],
                "host": False,
                "progress_category": "libc",
                "objects": [
                    ps2_libc("sf_ceil.c"),
                    ps2_libc("sf_cos.c"),
                    ps2_libc("sf_fabs.c"),
                    ps2_libc("sf_floor.c"),
                    ps2_libc("sf_isnan.c"),
                    ps2_libc("sf_sin.c"),
                    ps2_libc("sf_tan.c"),
                    ps2_libc("e_exp.c"),
                    ps2_libc("ef_rem_pio2.c"),
                    ps2_libc("kf_cos.c"),
                    ps2_libc("kf_rem_pio2.c"),
                    ps2_libc("kf_sin.c"),
                    ps2_libc("sf_scalbn.c"),
                    ps2_libc("sf_copysign.c"),
                ],
            },
        ]
    )

# r36: runtime de la toolchain de Sony en PS2 (fuente publica tal cual, bajo Speed/PSX2/bWare/Src/ee/gcc/).
# - libgcc.a de gcc 2.9-ee-991111: los stabs del original dicen ../../src/gcc/libgcc2.c compilado desde
#   /usr/local/sce/ee/gcc/build/gcc, y el libgcc.a de build/compilers casa al byte con el del juego. En src/gcc/ va la
#   fuente de gcc; en objdir/ (el directorio de build de gcc y newlib; `build/` lo ignora git) lo que genera su build
#   (tconfig.h, tm.h, fp-bit.c, dp-bit.c) y un fichero por objeto que el Makefile saca de un mismo fuente con
#   -DL<nombre> o -DDEFINE_<x> (tools/ee_gcc_pp.py preprocesa por ruta de fuente: dos objetos del mismo fuente se
#   pisarian el .i). _eh lleva -fexceptions, como LIB2FUNCS_EH en el Makefile de gcc (sin el, este cc1 aborta en
#   __rethrow, jump.c:3791).
# - libm.a (newlib 1.8.1 de la toolchain): -O2 -g, NEWLIB_CFLAGS (-DMALLOC_ALIGNMENT=16 -fno-builtin) y -G0.
# - libc.a: la de SN (objetos .obj en el NFS.MAP), compilada de newlib 1.8.1 con el ee-gcc 2.95.3 de SN como
#   libc_sn, con --enable-newlib-mb (MB_CAPABLE) y las variantes de mallocr.c (-DDEFINE_x) del Makefile de newlib.
#   Lo que SN anadio o retoco (snprintf, el cerrojo de malloc, _tolower en strlwr/strncasecmp, wchar_t de 16 bits)
#   va en sn/libc.
RT_EE = "Speed/PSX2/bWare/Src/ee/gcc/"


def ps2_rt(unidad, fuente, *extra):
    """Una unidad Speed/PSX2/bware/sce/ee/gcc/<unidad> del runtime de PS2, con su fuente bajo RT_EE."""
    return Object(
        NonMatching,
        "Speed/PSX2/bware/sce/ee/gcc/" + unidad + ".c",
        source=RT_EE + fuente,
        extra_cflags=list(extra),
    )


if config.platform == Platform.PS2:
    config.libs.extend(
        [
            {
                # LIBGCC2_CFLAGS e INCLUDES del Makefile de gcc (CROSS, INTERNAL_CFLAGS, TARGET_LIBGCC2_CFLAGS -G 0).
                "lib": "libgcc_ee",
                "toolchain_version": config.linker_version,
                "cflags": [
                    "-O2",
                    "-DCROSS_COMPILE",
                    "-DIN_GCC",
                    "-DHAIFA",
                    "-DUSE_CPPLIB=1",
                    "-DMULTIBYTE_CHARS=1",
                    "-g",
                    "-I src/" + RT_EE + "lib/gcc-lib/ee/2.9-ee-991111/include",
                    "-G0",
                    "-g1",
                    "-DIN_LIBGCC2",
                    "-D__GCC_FLOAT_NOT_NEEDED",
                    "-Dinhibit_libc",
                    "-I src/" + RT_EE + "objdir/gcc",
                    "-I src/" + RT_EE + "src/gcc",
                    "-I src/" + RT_EE + "src/gcc/config",
                    "-I src/" + RT_EE + "include",
                ],
                "host": False,
                "progress_category": "libc",
                "objects": [
                    ps2_rt("__main", "objdir/gcc/__main.c"),
                    ps2_rt("_divdi3", "objdir/gcc/_divdi3.c"),
                    ps2_rt("_eh", "objdir/gcc/_eh.c", "-fexceptions"),
                    ps2_rt("_fixdfdi", "objdir/gcc/_fixdfdi.c"),
                    ps2_rt("_fixsfdi", "objdir/gcc/_fixsfdi.c"),
                    ps2_rt("_fixunsdfdi", "objdir/gcc/_fixunsdfdi.c"),
                    ps2_rt("_fixunssfdi", "objdir/gcc/_fixunssfdi.c"),
                    ps2_rt("_floatdidf", "objdir/gcc/_floatdidf.c"),
                    ps2_rt("_floatdisf", "objdir/gcc/_floatdisf.c"),
                    # _moddi3, _udivdi3 y _umoddi3: el original reserva 16-32 B mas de pila para la union del
                    # __udivmoddi4 en linea; es del compilador (el libgcc.a de build/compilers si casa).
                    ps2_rt("_moddi3", "objdir/gcc/_moddi3.c"),
                    ps2_rt("_muldi3", "objdir/gcc/_muldi3.c"),
                    ps2_rt("_pure", "objdir/gcc/_pure.c"),
                    ps2_rt("_udivdi3", "objdir/gcc/_udivdi3.c"),
                    ps2_rt("_umoddi3", "objdir/gcc/_umoddi3.c"),
                    ps2_rt("dp-bit", "objdir/gcc/dp-bit.c"),
                    ps2_rt("fp-bit", "objdir/gcc/fp-bit.c"),
                    ps2_rt("frame", "src/gcc/frame.c"),
                    # La parte C++ de libgcc.a (cp/Make-lang.in); solo la A124 la enlaza.
                    ps2_rt("tinfo", "src/gcc/cp/tinfo.cc"),
                    ps2_rt("tinfo2", "src/gcc/cp/tinfo2.cc"),
                    ps2_rt("exception", "src/gcc/cp/exception.cc", "-fexceptions"),
                ],
            },
            {
                # Las que no casan llevan div.s: el original no lo pone en ranura de retardo y lo precede de dos
                # nop, cosa que ni este cc1 ni este as hacen; e_sqrt y s_rint, la pila de mas de _moddi3.
                "lib": "libm_newlib",
                "toolchain_version": config.linker_version,
                "cflags": [
                    "-O2",
                    "-g",
                    "-G0",
                    "-fno-builtin",
                    "-DMALLOC_ALIGNMENT=16",
                    "-I src/" + RT_EE + "src/newlib/libm/common",
                    "-I src/" + RT_EE + "ee/include",
                    "-I src/" + RT_EE + "lib/gcc-lib/ee/2.9-ee-991111/include",
                ],
                "host": False,
                "progress_category": "libc",
                "objects": [
                    ps2_rt("ee/" + f.split("/")[1], "src/newlib/libm/" + f + ".c")
                    for f in [
                        "math/w_exp",
                        "math/w_log",
                        "math/w_sqrt",
                        "math/wf_acos",
                        "math/wf_asin",
                        "math/wf_atan2",
                        "math/wf_cosh",
                        "math/wf_exp",
                        "math/wf_fmod",
                        "math/wf_log",
                        "math/wf_log10",
                        "math/wf_pow",
                        "math/wf_sinh",
                        "math/e_log",
                        "math/e_sqrt",
                        "math/ef_acos",
                        "math/ef_asin",
                        "math/ef_atan2",
                        "math/ef_cosh",
                        "math/ef_exp",
                        "math/ef_fmod",
                        "math/ef_log",
                        "math/ef_log10",
                        "math/ef_pow",
                        "math/ef_sinh",
                        "math/ef_sqrt",
                        "math/kf_tan",
                        "math/sf_atan",
                        "math/sf_tanh",
                        "math/s_isnan",
                        "common/s_finite",
                        "common/s_matherr",
                        "common/s_rint",
                        "common/sf_expm1",
                        "common/sf_finite",
                    ]
                ],
            },
            {
                # Ensamblada con el Ps2EeAs de SN (la replica -snas): los nop de bucle corto y de alineacion
                # que separaban mprec, strtod, strtol, exit, strstr, fvwrite y mallocr eran del ensamblador.
                # Flags de SN como libc_sn (funciones internas y -fshort-wchar).
                "lib": "libc_sn_newlib",
                "toolchain_version": toolchain_de("libc_sn"),
                "ee_as": "Ps2EeAs",
                "cflags": [*cflags_libc, "-G0", "-fshort-wchar", "-DMALLOC_ALIGNMENT=16", "-DMB_CAPABLE"],
                "host": False,
                "progress_category": "libc",
                "objects": [
                    *[
                        ps2_rt("ee/" + f.split("/")[1], "src/newlib/libc/" + f + ".c")
                        for f in [
                            "errno/errno",
                            "stdio/puts",
                            "stdlib/atoi",
                            "stdlib/bsearch",
                            "stdlib/rand",
                            "string/strerror",
                            "string/u_strerr",
                            "ctype/_tolower",
                            "reent/sbrkr",
                            "stdio/flags",
                            "stdlib/mprec",
                            "stdlib/strtod",
                            "stdlib/strtol",
                            "stdlib/strtoul",
                            "stdlib/exit",
                            "string/strstr",
                            "stdio/fvwrite",
                        ]
                    ],
                    ps2_rt(
                        "ee/mallocr",
                        "src/newlib/libc/stdlib/mallocr.c",
                        "-DINTERNAL_NEWLIB",
                        "-DDEFINE_MALLOC",
                    ),
                    ps2_rt("ee/freer", "objdir/ee/newlib/libc/stdlib/freer.c", "-DINTERNAL_NEWLIB"),
                    ps2_rt("ee/callocr", "objdir/ee/newlib/libc/stdlib/callocr.c", "-DINTERNAL_NEWLIB"),
                    # Lo que SN escribio o retoco (sn/libc): sacado del binario, al estilo de newlib. mbtowc_r es el
                    # de newlib con el wchar_t de 16 bits de SN; wcslen y wmemchr, desenrolladas a mano;
                    # vfwprintf/vfwiprintf, su printf ancho.
                    *[
                        ps2_rt("ee/" + f, "sn/libc/" + f + ".c")
                        for f in [
                            "snprintf",
                            "malloc",
                            "mlock",
                            "strlwr",
                            "strncasecmp",
                            "wcscpy",
                            "wmemcpy",
                            "wsetup",
                            "wcslen",
                            "wmemchr",
                            "mbtowc_r",
                            "vfwprintf",
                            "vfwiprintf",
                        ]
                    ],
                ],
            },
            {
                # r36 cadenas: las funciones de cadena de la libc de SN en PS2 son el asm de Cygnus de newlib
                # (newlib/libc/machine/r5900/*.S, MMI y quadword), el mismo instruccion a instruccion; van copiadas
                # sin cambios (asm escrito en asm en origen: docs/asm-de-ea.md). Se preprocesan con ee-gcc -E y se
                # ensamblan con el ensamblador de SN, Ps2EeAs (tools/ee_as_S.py): su `dli` es el del original y el
                # del as de GNU no. El Ps2EeAs que tenemos (1.9.25) alarga los bucles de 6 instrucciones o menos
                # a 7, y el que ensamblo la libc de SN los alargaba a 6: la toolchain de libc_sn es la replica
                # de scripts/replica_snas.py y con ella casan las 13.
                # setjmp no entra: el original guarda con sq/swc1 y este setjmp.S (newlib 1.8.1) con sw.
                "lib": "libc_sn_r5900",
                "toolchain_version": toolchain_de("libc_sn"),
                "cflags": ["-Wa,-G0"],
                "host": False,
                "progress_category": "libc",
                "objects": [
                    ps2_rt("ee/" + f, "src/newlib/libc/machine/r5900/" + f + ".S")
                    for f in [
                        "memchr",
                        "memcmp",
                        "memcpy",
                        "memmove",
                        "memset",
                        "strcat",
                        "strchr",
                        "strcmp",
                        "strcpy",
                        "strlen",
                        "strncat",
                        "strncmp",
                        "strncpy",
                    ]
                ],
            },
        ]
    )


if config.platform == Platform.GC_WII:
    config.libs.extend(
        [
            {
                "lib": "libsn",
                "toolchain_version": config.linker_version,
                # SN library code is gcc2-compiled (SN ProDG toolchain),
                # NOT Metrowerks: ngccc rejects the mwcc-style flags.
                # MEDIDO: comparte casi todo con cflags_libc pero SI lleva -G0.
                # sndvd emite lis @ha/@l donde sin -G0 sacamos @sda21: vale
                # +348 B y 3 funciones. Tiene que ser una lista PROPIA: meter
                # -G0 en cflags_libc cuesta -13.032 B y 33 funciones en libc
                # (sf_atan -528, sf_fmod -456, s_floor -348, fopen -324...).
                "cflags": [*cflags_libc, "-G0"],
                "host": False,
                "progress_category": "libs",  # str | List[str]
                "objects": [
                    Object(MatchingFor("GOWE69"), "LibSN/crt0.s"),
                    Object(NonMatching, "LibSN/cvtll.c"),
                    Object(NonMatching, "LibSN/debug.c"),
                    Object(NonMatching, "LibSN/dummy.c"),
                    Object(NonMatching, "LibSN/fileserver.c"),
                    Object(NonMatching, "LibSN/FSasync.c"),
                    Object(NonMatching, "LibSN/inituser.c"),
                    Object(NonMatching, "LibSN/ppcdown.c"),
                    Object(NonMatching, "LibSN/prof.c"),
                    Object(NonMatching, "LibSN/proview.c"),
                    # DSIHandler 97,07 % (620/628 B). Asm original que se queda: el
                    # vector DSIExcHandler y los mtspr del DABR (SPR de supervisor).
                    Object(NonMatching, "LibSN/sndvd.c"),
                    Object(NonMatching, "LibSN/tealeaf.c"),
                    Object(NonMatching, "LibSN/tors.c"),
                    # SN SDK blocks identified from symbols (no DWARF CUs):
                    # MetroTRK proper 0x8030EA7C-0x80311BE4 (licensed SN code),
                    # Logitech steering driver 0x8031F5D8-0x80321810,
                    # SN virtual memory 0x8039B258-0x8039BD38 + VMBASE 0x803A3398-0x803A4004
                    # MetroTRK nunca fue C: va como .s, igual que pro2D4.s.
                    Object(Matching, "LibSN/metrotrk.c", source="LibSN/metrotrk.s"),
                    Object(Matching, "LibSN/syscalls.c"),
                ],
            },
            {
                "lib": "libsn_mw",
                "toolchain_version": "GC/2.7",
                "cflags": cflags_libsn_mw,
                "host": False,
                "progress_category": "libs",
                "objects": [
                    Object(Matching, "LibSN/vm.c"),
                    Object(Matching, "LibSN/VMPageReplacement.c"),
                    Object(Matching, "LibSN/VMMapping.c"),
                    # __VMBASESetupExceptionHandlers: 99,16 % en C (mwcc lleva el
                    # temporal de la instruccion por r0); el 100 % era asm.
                    Object(NonMatching, "LibSN/vmbase.c"),
                ],
            },
            {
                "lib": "libsn_mw_steering",
                # Medido (barrido MWCC 1.3-2.7 contra el objeto): con GC/2.7 Effect_Init da
                # 45,55 %; con GC/2.0p1 da 97,10 % y ninguna de las 31 funciones al 100 %
                # se mueve (.text 97,50 -> 99,06). SimThread_Init baja 95,0 -> 93,40 (no
                # estaba cerrada). La familia 1.3-2.7 del prologo ya lo acotaba.
                "toolchain_version": "GC/2.0p1",
                "cflags": cflags_libsn_mw_steering,
                "host": False,
                "progress_category": "libs",
                "objects": [
                    Object(NonMatching, "LibSN/steering.c"),
                ],
            },
            {
                # RealShape y RealFont (librealshapez.a / librealfontz.a). MEDIDO: con
                # los cflags de `misc` estas siete unidades dan 83,2%; con estos tres
                # cambios dan 4.644/4.644 B y 34 de 34 funciones, 100% las siete.
                #   -G0 SOBRA: MemObject::sAllocator y RealFontOld::gFontDriver se leen
                #     con @sda21(r0) en UNA instruccion; con -G0 salen lis+lwz @l.
                #     Cuesta fontcreate ENTERA (136 B) y 284 B de base.
                #   -fforce-addr SOBRA: el objetivo emite `lbzx r3,r9,r3` (base primero)
                #     y con el flag sale `lbzx r3,r3,r9`. Cuesta GetDepth y Shape::Create.
                #   -fstrength-reduce FALTA: findkern16 avanza dos punteros (addi 4)
                #     donde sin el sintetizamos slwi+lhzx. Cuesta 64 B.
                # NO se puede arreglar `misc` en bloque: de sus 272 objetos solo 8 tienen
                # fuente, y csis.cpp CAE de 81,58% a 48,71% con estos flags (-1.884 B).
                # Los hermanos sin fuente todavia (shptype, swizzlesize, fontdraw,
                # fontdrawf, fontnull) van aqui cuando se escriban.
                "lib": "realgraph",
                "toolchain_version": config.linker_version,
                "cflags": [
                    *[f for f in cflags_game if f not in ("-G0", "-fforce-addr")],
                    "-fstrength-reduce",
                ],
                "host": False,
                "progress_category": "libs",
                "objects": [
                    Object(Matching, "allocator.cpp"),
                    Object(Matching, "shpdestroy.cpp"),
                    Object(NonMatching, "model.cpp"),
                    Object(Matching, "fontchar.cpp"),
                    Object(Matching, "fontcreate.cpp"),
                    Object(Matching, "fontkern.cpp"),
                    Object(NonMatching, "creates.cpp"),
                    Object(NonMatching, "cluttype.cpp"),
                ],
            },
            {
                "lib": "misc",
                "toolchain_version": config.linker_version,
                "cflags": cflags_game,
                "host": False,
                "progress_category": "libs",  # str | List[str]
                "objects": [
                    Object(Matching, "crt2D1.tmp", source="LibSN/crt2D1.c", cflags=[*cflags_libc, "-G0"]),
                    # r76: barrera fuera. __cvt_fp2unsigned 94,44 % (68/72 B).
                    Object(NonMatching, "ppc2D2.tmp", source="LibSN/ppc2D2.c", cflags=[*cflags_libc, "-G0"]),
                    # r39: __va_arg is MW, unlike the preceding GCC bridges.
                    # Separate objects reproduce the original DOL; see r39-runtime.md.
                    Object(
                        Matching,
                        "Runtime.PPCEABI.H/__va_arg.c",
                        toolchain_version="GC/1.2.5n",
                        cflags=[*cflags_runtime, "-char unsigned", "-lang=c"],
                    ),
                    Object(NonMatching, "fil2D3.tmp"),
                    Object(Matching, "pro2D4.tmp", source="LibSN/pro2D4.s"),
                    Object(NonMatching, "inituser.c"),
                    Object(NonMatching, "pro2D9.tmp"),
                    Object(NonMatching, "tea2Da.tmp"),
                    # 2104/2104 B en C, sin asm. Sigue NonMatching: sus datos viven en
                    # los comodines auto_06_804394D0_data y auto_07_804B4DE8_bss.
                    Object(NonMatching, "FSasync.c", source="LibSN/FSasync.c", cflags=[*cflags_libc, "-G0"]),
                    Object(NonMatching, "sndvd.c"),
                    # This newlib unit uses the libc flags, not the generic
                    # game fallback (-G0/-fforce-addr changes its SDA loads).
                    Object(Matching, "atexit.c", cflags=cflags_libc),
                    Object(Matching, "qsort.c"),
                    Object(Matching, "sn_malloc.c"),
                    Object(NonMatching, "allsrc.c"),
                    Object(NonMatching, "audioplayer.cpp"),
                    Object(NonMatching, "device_cmn.cpp"),
                    Object(NonMatching, "rendercontext.cpp"),
                    Object(NonMatching, "rendercontext_cmn.cpp"),
                    Object(NonMatching, "state.cpp"),
                    Object(NonMatching, "tar.cpp"),
                    Object(NonMatching, "tevstage.cpp"),
                    Object(NonMatching, "texturerc_cmn.cpp"),
                    Object(NonMatching, "viewport.cpp"),
                    Object(NonMatching, "viewport_cmn.cpp"),
                    Object(NonMatching, "profiler_cmn.cpp"),
                    Object(NonMatching, "singledraw.cpp"),
                    Object(NonMatching, "rendermethod.cpp"),
                    Object(NonMatching, "texturerc.cpp"),
                    Object(NonMatching, "pcode.cpp"),
                    Object(NonMatching, "locatbig.cpp"),
                    Object(NonMatching, "filedev.cpp"),
                    Object(NonMatching, "fixdmult.cpp"),
                    Object(NonMatching, "gc_driver.cpp"),
                    Object(NonMatching, "memvectors.cpp"),
                    Object(NonMatching, "interfaceimp.cpp"),
                    Object(NonMatching, "lib/cmn/locale.cpp"),
                    Object(NonMatching, "inittmr.cpp"),
                    Object(NonMatching, "initvblt.cpp"),
                    Object(NonMatching, "mutex2.cpp"),
                    Object(NonMatching, "printn3.cpp"),
                    Object(NonMatching, "memcopy.cpp"),
                    Object(NonMatching, "fontdraw.cpp"),
                    Object(NonMatching, "fontdrawf.cpp"),
                    Object(NonMatching, "fontnull.cpp"),
                    Object(NonMatching, "loccore.cpp"),
                    Object(NonMatching, "ustrcpy.cpp"),
                    Object(NonMatching, "shptype.cpp"),
                    Object(NonMatching, "swizzlesize.cpp"),
                    # csis va sin -fforce-addr, como las otras bibliotecas de EA (rcmp, vp6,
                    # realmemcard, realcore, realgraph). Medido: con el flag, Init, Subscribe,
                    # Unsubscribe y los dos Alloc del adaptador no casan (52/57); sin el y con
                    # los campos de NullAllocatorFlags en el orden de TagValuePair, 57/57.
                    Object(
                        NonMatching,
                        "Speed/Indep/Libs/csis/dev/source/library/cmn/csis.cpp",
                        cflags=[*[f for f in cflags_game if f != "-fforce-addr"], "-fno-implement-inlines"],
                    ),
                    Object(NonMatching, "dummy.c"),
                    Object(NonMatching, "prodg_fixes.cpp"),
                    Object(NonMatching, "codec/cmn/refsize.cpp"),
                ],
            },
            DolphinLib(
                "base",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/base/PPCArch.c",
                    ),
                ],
            ),
            DolphinLib(
                "ar",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ar/ar.c",
                    ),
                ],
            ),
            DolphinLib(
                "arq",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ar/arq.c",
                    ),
                ],
            ),
            DolphinLib(
                "ax",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ax/AXAlloc.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ax/AXAux.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ax/AXCL.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ax/AX.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ax/AXOut.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ax/AXSPB.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ax/AXVPB.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ax/AXComp.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ax/DSPCode.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ax/AXProf.c",
                    ),
                ],
            ),
            DolphinLib(
                "card",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDBios.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDBlock.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDDir.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDCheck.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDOpen.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDMount.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDNet.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDFormat.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDCreate.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDRead.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDWrite.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDDelete.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDStat.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDStatEx.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDUnlock.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/card/CARDRdwr.c",
                    ),
                ],
            ),
            DolphinLib(
                "dsp",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dsp/dsp.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dsp/dsp_debug.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dsp/dsp_task.c",
                    ),
                ],
            ),
            DolphinLib(
                "os",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OS.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSAlarm.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSAlloc.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSArena.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSAudioSystem.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSCache.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSContext.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSError.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSExec.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSFont.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSInterrupt.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSLink.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSMemory.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSMessage.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSMutex.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSReset.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSResetSW.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSRtc.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSSemaphore.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSSync.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSThread.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSTime.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSUtf.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/OSReboot.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/os/__ppc_eabi_init.c",
                    ),
                ],
            ),
            DolphinLib(
                "db",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/db/db.c",
                    ),
                ],
            ),
            DolphinLib(
                "mtx",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/mtx/mtx.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/mtx/mtxvec.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/mtx/mtx44.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/mtx/vec.c",
                    ),
                ],
            ),
            DolphinLib(
                "dvd",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dvd/dvdfs.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dvd/dvd.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dvd/dvdqueue.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dvd/dvderror.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dvd/dvdidutils.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dvd/dvdFatal.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dvd/fstload.c",
                        extra_cflags=["-char signed"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/dvd/dvdlow.c",
                        extra_cflags=["-char signed"],
                    ),
                ],
            ),
            DolphinLib(
                "vi",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/vi/vi.c",
                    ),
                ],
            ),
            DolphinLib(
                "demo",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/demo/DEMOPad.c",
                    ),
                ],
            ),
            DolphinLib(
                "pad",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/pad/Padclamp.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/pad/Pad.c",
                    ),
                ],
            ),
            DolphinLib(
                "ai",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/ai/ai.c",
                    ),
                ],
            ),
            DolphinLib(
                "gx",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXInit.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXFrameBuf.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXAttr.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXFifo.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXMisc.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXGeometry.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXDisplayList.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXLight.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXTexture.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXBump.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXTev.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXPixel.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXTransform.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/gx/GXPerf.c",
                    ),
                ],
            ),
            DolphinLib(
                "exi",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/exi/EXIBios.c",
                        extra_cflags=["-O4,p", "-schedule off"],
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/exi/EXIUart.c",
                    ),
                ],
            ),
            DolphinLib(
                "si",
                [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/si/SIBios.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/si/SISamplingRate.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/si/SISteering.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/si/SISteeringXfer.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/si/SISteeringAuto.c",
                    ),
                ],
            ),
            {
                "lib": "OdemuExi2",
                # MEDIDO: es Metrowerks de la familia GC/1.0-1.2.5n, no ProDG.
                # El discriminador: MWCC 1.0-1.2.5n emite `addi rD,rS,0` como
                # copia de registro y 1.3-2.7 emiten `mr`; DebuggerDriver.s
                # tiene 34 `addi rX,rY,0x0` y EXIBios.s (SDK 1.2.5n) 94.
                # Con config.linker_version (ProDG) el ngccc rechaza los flags
                # de MWCC con "unknown command line option '-nodefaults'".
                "toolchain_version": "GC/1.2.5n",
                "cflags": cflags_dolphin,
                "progress_category": "sdk",  # str | List[str]
                "objects": [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/OdemuExi2/src/DebuggerDriver.c",
                    ),
                ],
            },
            {
                # The original ELF identifies two more TUs, built with 1.2.5.
                "lib": "OdemuExi2Amc",
                "toolchain_version": "GC/1.2.5",
                "cflags": cflags_dolphin,
                "progress_category": "sdk",
                "objects": [
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/OdemuExi2/src/AmcExi2Comm.c",
                    ),
                    Object(
                        Matching,
                        "Speed/GameCube/bWare/GameCube/OdemuExi2/src/AmcExi.c",
                    ),
                ],
            },
            {
                "lib": "amcstubs",
                "mw_version": config.linker_version,
                "cflags": cflags_amcstub,
                "host": False,
                "progress_category": "sdk",  # str | List[str]
                "objects": [
                    Object(
                        NonMatching,
                        "Speed/GameCube/bWare/GameCube/amcstubs/src/amcExi2/AmcExi.c",
                    ),
                    Object(
                        NonMatching,
                        "Speed/GameCube/bWare/GameCube/dolphinsdk/src/amcExi2/AmcExi2Comm.c",
                    ),
                ],
            },
            {
                "lib": "libc",
                "toolchain_version": config.linker_version,
                "cflags": cflags_libc,
                "progress_category": "libc",  # str | List[str]
                "objects": [
                    Object(Matching, "libc/vprintf.c"),
                    Object(Matching, "libc/vsprintf.c"),
                    Object(Matching, "libc/memcpy.c"),
                    Object(Matching, "libc/memmove.c"),
                    Object(Matching, "libc/memset.c"),
                    Object(Matching, "libc/strcasecmp.c"),
                    Object(Matching, "libc/strchr.c"),
                    Object(Matching, "libc/strcmp.c"),
                    Object(Matching, "libc/strcpy.c"),
                    Object(Matching, "libc/strlen.c"),
                    Object(Matching, "libc/strncmp.c"),
                    Object(Matching, "libc/strncpy.c"),
                    # vfprintf.c empieza en strround (0x803152A0, el STT_FILE `vfprintf.c'): la compilacion con
                    # coma flotante lleva delante las rutinas de itoa.c (#include "itoa.c"), que no es una unidad.
                    Object(NonMatching, "libc/vfprintf.c"),
                    Object(Matching, "libc/ctype_.c"),
                    Object(Matching, "libc/impure.c"),
                    Object(NonMatching, "libc/vfprintf_1.c"),
                    Object(Matching, "libc/isdigit.c"),
                    Object(Matching, "libc/isspace.c"),
                    Object(Matching, "libc/locale.c"),
                    Object(Matching, "libc/_tolower.c"),
                    Object(Matching, "libc/math_support.c"),
                    Object(Matching, "libc/mbtowc_r.c"),
                    Object(Matching, "libc/libgcc2.c"),
                    Object(Matching, "libc/memchr.c"),
                    Object(Matching, "libc/libgcc2_1.c"),
                    Object(Matching, "libc/libgcc2_2.c"),
                    Object(Matching, "libc/libgcc2_3.c"),
                    Object(Matching, "libc/libgcc2_4.c"),
                    Object(Matching, "libc/libgcc2_5.c"),
                    Object(Matching, "libc/libgcc2_6.c"),
                    Object(Matching, "libc/libgcc2_7.c"),
                    Object(Matching, "libc/libgcc2_8.c"),
                    Object(Matching, "libc/libgcc2_9.c"),
                    Object(Matching, "libc/libgcc2_10.c", extra_cflags=["-G0"]),
                    # MEDIDO r19: -msafe-sda manda el POOL DE CONSTANTES del
                    # compilador a .sdata con @sda21 (UNA instruccion), que es
                    # donde lo tiene el original; sin el, el sesgo
                    # 0x4330000080000000 de cada conversion int->double cae en
                    # .rodata y cuesta un `lis @ha` de mas por conversion.
                    # -mstrict-align da BLKmode a los arrays de 8 B y con ello
                    # el orden `lfsx rD, base, indice` del objetivo.
                    # libm es el newlib de SN tal cual (static const, literales):
                    # -msafe-sda pone su pool donde el original, -fno-builtin
                    # deja fabs/fabsf como llamada (newlib se compila asi) y
                    # -G1024 da @sda21 a las tablas de 16..792 B que el original
                    # lee por @sda21 (two_over_pi de ef_rem_pio2 mide 792 B).
                    Object(Matching, "libc/e_pow.c", extra_cflags=["-msafe-sda", "-mstrict-align", "-G1024", "-fno-builtin"]),
                    Object(Matching, "libc/e_sqrt.c"),
                    Object(Matching, "libc/s_fabs.c"),
                    Object(Matching, "libc/ef_pow.c", extra_cflags=["-msafe-sda", "-mstrict-align", "-fno-builtin"]),
                    Object(Matching, "libc/ef_sqrt.c"),
                    # MEDIDO: -fno-expensive-optimizations lleva estas tres de
                    # ~93% a 100% (58 -> 55 instrucciones, 35 -> 32). Va POR
                    # OBJETO a proposito: el mismo flag tira 14 unidades de esta
                    # misma biblioteca de 100% a 0% (e_acos, e_sqrt, s_floor,
                    # math_support, memcmp, libgcc2_3/6/8/9...).
                    Object(
                        Matching,
                        "libc/sf_cos.c",
                        extra_cflags=["-fno-expensive-optimizations", "-msafe-sda"],
                    ),
                    Object(Matching, "libc/sf_fabs.c"),
                    Object(
                        Matching,
                        "libc/sf_sin.c",
                        extra_cflags=["-fno-expensive-optimizations", "-msafe-sda"],
                    ),
                    Object(Matching, "libc/s_scalbn.c", extra_cflags=["-msafe-sda"]),
                    Object(Matching, "libc/sf_scalbn.c", extra_cflags=["-msafe-sda"]),
                    Object(Matching, "libc/kf_cos.c", extra_cflags=["-msafe-sda"]),
                    Object(Matching, "libc/kf_sin.c", extra_cflags=["-msafe-sda"]),
                    Object(Matching, "libc/ef_rem_pio2.c", extra_cflags=["-msafe-sda", "-mstrict-align", "-G1024", "-fno-builtin"]),
                    Object(Matching, "libc/s_copysign.c"),
                    Object(Matching, "libc/sf_copysign.c"),
                    Object(Matching, "libc/kf_rem_pio2.c", extra_cflags=["-msafe-sda", "-mstrict-align", "-G1024"]),
                    Object(Matching, "libc/sf_floor.c", extra_cflags=["-msafe-sda"]),
                    Object(Matching, "libc/e_acos.c", extra_cflags=["-msafe-sda"]),
                    Object(NonMatching, "libc/e_atan2.c"),
                    Object(NonMatching, "libc/s_atan.c"),
                    Object(Matching, "libc/ef_atan2.c", extra_cflags=["-msafe-sda", "-fno-builtin"]),
                    Object(NonMatching, "libc/vfscanf.c"),
                    Object(Matching, "libc/strtod2.c"),
                    Object(Matching, "libc/sn_buf.cpp"),
                    Object(Matching, "libc/tolower.c"),
                    Object(NonMatching, "libc/puts.c"),
                    Object(NonMatching, "libc/atoi.c"),
                    Object(Matching, "libc/bsearch.c"),
                    Object(Matching, "libc/memcmp.c"),
                    Object(Matching, "libc/strstr.c"),
                    Object(NonMatching, "libc/strtol.c"),
                    Object(NonMatching, "libc/fvwrite.c"),
                    Object(Matching, "libc/wsetup.c"),
                    Object(Matching, "libc/fflush.c"),
                    Object(Matching, "libc/fopen.c"),
                    Object(Matching, "libc/fwalk.c"),
                    Object(Matching, "libc/makebuf.c"),
                    Object(Matching, "libc/stdio.c"),
                    Object(Matching, "libc/closer.c"),
                    Object(Matching, "libc/fstatr.c"),
                    Object(Matching, "libc/lseekr.c"),
                    Object(Matching, "libc/readr.c"),
                    Object(Matching, "libc/writer.c"),
                    Object(Matching, "libc/s_sin.c", extra_cflags=["-msafe-sda"]),
                    Object(Matching, "libc/sf_atan.c", extra_cflags=["-msafe-sda", "-G1024", "-fno-builtin"]),
                    Object(
                        Matching,
                        "libc/sf_tan.c",
                        extra_cflags=["-fno-expensive-optimizations", "-msafe-sda"],
                    ),
                    Object(Matching, "libc/k_cos.c", extra_cflags=["-msafe-sda"]),
                    Object(Matching, "libc/k_sin.c", extra_cflags=["-msafe-sda"]),
                    Object(NonMatching, "libc/k_tan.c"),
                    Object(Matching, "libc/e_rem_pio2.c", extra_cflags=["-msafe-sda", "-mstrict-align", "-G1024", "-fno-builtin"]),
                    # -msafe-sda solo (el pool a .sdata): cierra al 100%
                    Object(Matching, "libc/kf_tan.c", extra_cflags=["-msafe-sda", "-G1024", "-fno-builtin"]),
                    Object(Matching, "libc/k_rem_pio2.c", extra_cflags=["-msafe-sda", "-mstrict-align", "-G1024"]),
                    Object(Matching, "libc/s_floor.c", extra_cflags=["-msafe-sda"]),
                    # SN libc TUs identified from the gcc2/newlib block
                    # 0x80312C10-0x8031F4C0 and 0x8039BD38-0x8039D230
                    Object(Matching, "libc/fclose.c"),
                    Object(Matching, "libc/fprintf.c"),
                    Object(Matching, "libc/fseek.c"),
                    Object(Matching, "libc/printf.c"),
                    Object(Matching, "libc/sprintf.c"),
                    Object(Matching, "libc/strcat.c"),
                    Object(Matching, "libc/strcoll.c"),
                    Object(Matching, "libc/strcspn.c"),
                    Object(Matching, "libc/strncasecmp.c"),
                    Object(Matching, "libc/strncat.c"),
                    # newlib la escribe en ensamblador (libc/machine/powerpc/setjmp.S).
                    Object(Matching, "libc/setjmp.c", source="libc/setjmp.s"),
                    Object(Matching, "libc/rand.c"),
                    Object(Matching, "libc/openr.c"),
                    Object(Matching, "libc/flags.c"),
                    Object(Matching, "libc/sf_asin.c", extra_cflags=["-msafe-sda", "-fno-builtin"]),
                    Object(Matching, "libc/sf_cosh.c", extra_cflags=["-msafe-sda", "-fno-builtin"]),
                    Object(Matching, "libc/sf_exp.c", extra_cflags=["-msafe-sda", "-mstrict-align"]),
                    Object(Matching, "libc/sf_fmod.c"),
                    # -msafe-sda solo (el pool a .sdata): cierra al 100%
                    Object(Matching, "libc/sf_log.c", extra_cflags=["-msafe-sda"]),
                    Object(Matching, "libc/sf_log10.c", extra_cflags=["-msafe-sda"]),
                    Object(Matching, "libc/sf_sinh.c", extra_cflags=["-msafe-sda", "-fno-builtin"]),
                    Object(Matching, "libc/sf_ceil.c", extra_cflags=["-msafe-sda"]),
                    Object(Matching, "libc/sf_isnan.c"),
                    Object(Matching, "libc/sf_tanh.c", extra_cflags=["-msafe-sda", "-fno-builtin"]),
                    Object(Matching, "libc/sf_expm1.c", extra_cflags=["-msafe-sda", "-mstrict-align"]),
                    Object(Matching, "libc/strrchr.c"),
                    Object(Matching, "libc/wcscat.c"),
                    Object(Matching, "libc/wcscpy.c"),
                    Object(Matching, "libc/wcslen.c"),
                    Object(Matching, "libc/e_exp.c", extra_cflags=["-msafe-sda", "-mstrict-align", "-G1024"]),
                ],
            },
            {
                "lib": "eathread",
                "toolchain_version": toolchain_de("eathread"),
                "cflags": cflags_eathread,
                "progress_category": "libs",
                "objects": [
                    Object(
                        Matching,
                        "Packages/eathread/1.1.0/source/eathread_semaphore.cpp",
                    ),
                    Object(
                        Matching,
                        "Packages/eathread/1.1.0/source/eathread_thread.cpp",
                    ),
                ],
            },
        ]
    )

# PC40: eathread de PC. EA::Thread con su backend de Win32 (Packages/eathread/1.1.0/source/win32). La biblioteca va
# con /O1 /Oy- (tamano y marco de pila): asi salen byte a byte las 19 funciones del semaforo de speed.exe
# (0x834D82-0x834FE9); con el /O2 del juego no casa ninguna.
if config.platform == Platform.WIN32:
    config.libs.append(
        {
            "lib": "eathread",
            "toolchain_version": config.linker_version,
            "cflags": [f for f in cflags_base_prodg if f != "/O2"]
            + ["/O1", "/Oy-", "/I src/Packages/eathread/1.1.0/include"],
            "progress_category": "libs",
            "objects": [
                Object(NonMatching, "Packages/eathread/1.1.0/source/eathread_semaphore.cpp"),
            ],
        }
    )

# PC41: csis de PC. La misma fuente con el cerrojo de Win32 (CreateMutexA, WaitForSingleObject, ReleaseMutex). Con /O1
# casan por codigo 15 funciones sin nombre de speed.exe (0x830EAB-0x831D15) y System::Init, Lock y Unlock byte a byte;
# con /O2, 4.
if config.platform == Platform.WIN32:
    config.libs.append(
        {
            "lib": "csis",
            "toolchain_version": config.linker_version,
            "cflags": [f for f in cflags_game if f != "/O2"] + ["/O1"],
            "progress_category": "libs",
            "objects": [
                Object(NonMatching, "Speed/Indep/Libs/csis/dev/source/library/cmn/csis.cpp"),
            ],
        }
    )

# PC42: el driver de DirectSound de snd de PC (speed.exe 0x820063-0x820D4C; library/pc/sdsound.c, nombre de
# fichero nuestro). Los flags de snd de PC (/O1 /TP) con /Ob1: sin el, /O1 expande DSOUND_stopthread en DSOUND_close
# y el exe la llama.
if config.platform == Platform.WIN32:
    config.libs.append(
        {
            "lib": "sndpc",
            "toolchain_version": config.linker_version,
            "cflags": [*cflags_snd, "/Ob1"],
            "progress_category": "libs",
            "objects": [
                Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/pc/sdsound.c"),
                # r82: el driver de plataforma de PC (0x81EBF9-0x81F337), con nombres neutros donde no hay fuente
                Object(NonMatching, "Speed/Indep/Libs/snd/9/source/library/pc/snddrv.c"),
            ],
        }
    )

if config.platform == Platform.WIN32:
    # r86 (PC-B): dirtysock de PC (el DirtySDK de speed.exe, "Oct 19 2005"), con las fuentes de la A124 (escritas de su
    # asm). Las cuatro que casan contra el tramo de dirtysock del exe (0x834576-0x85D380): 20 funciones al 100 %;
    # lobbyapi.c sin sus cuatro arrays de relleno de tamano cero (MSVC no los admite en mitad de una struct: C2229).
    # lobbytagfield compila pero el TagField de PC es otra version (ninguna al 100 %).
    # r90 (PC-B): lobbytagfield dada de alta; emparejada por XREF con el tramo 0x83A0F0-0x83CBEB del exe
    # (dentro de unassigned_23). 3 funciones a 0 diferencias contra el exe sin LTCG
    # (_TagFieldFindIdx 86 B, _TagFieldGetNumber 98 B, _TagFieldGetFlags 59 B).
    # r87 (PC-B): buddyapi/hlbudapi sin su relleno de tamano cero y weboffer con WebOfferUpdate declarada (decision del
    # usuario de la ronda 86); emparejadas por orden y tamano con el tramo de dirtysock del exe.
    config.libs.append(
        {
            "lib": "dirtysock",
            "toolchain_version": config.linker_version,
            "cflags": cflags_dirtysock_win32,
            "host": False,
            "progress_category": "libs",
            "objects": [
                Object(NonMatching, "Speed/Indep/Libs/dirtysock/source/library/ps2/final/lobbyapi.c"),
                Object(NonMatching, "Speed/Indep/Libs/dirtysock/source/library/ps2/final/lobbyapilist.c"),
                Object(NonMatching, "Speed/Indep/Libs/dirtysock/source/library/ps2/final/lobbyapiutil.c"),
                Object(NonMatching, "Speed/Indep/Libs/dirtysock/source/library/ps2/final/lobbyapiasync.c"),
                Object(NonMatching, "Speed/Indep/Libs/dirtysock/source/library/ps2/final/buddyapi.c"),
                Object(NonMatching, "Speed/Indep/Libs/dirtysock/source/library/ps2/final/hlbudapi.c"),
                Object(NonMatching, "Speed/Indep/Libs/dirtysock/source/library/ps2/final/weboffer.c"),
                Object(NonMatching, "Speed/Indep/Libs/dirtysock/source/library/ps2/final/lobbytagfield.c"),
            ],
        }
    )

if config.platform == Platform.X360:
    config.libs.append(
        {
            "lib": "Game",
            "toolchain_version": config.linker_version,
            "cflags": cflags_game,
            "host": False,
            "progress_category": "sdk",  # str | List[str]
            "objects": [
                Object(NonMatching, "Speed/Xenon/link_fix.cpp"),
            ],
        }
    )
    # r86 (X360-A): csis como unidad de X360 (hasta aqui solo IL en ltcg360.BIBLIOTECAS, ronda 76). Los flags del
    # juego, como la copia de BIBLIOTECAS (la de PC lleva /O1: en Xenon no hay prueba de otros).
    config.libs.append(
        {
            "lib": "csis",
            "toolchain_version": config.linker_version,
            "cflags": [*cflags_game, "/I src/Speed/Indep/Libs/csis/dev/source/library/cmn"],
            "progress_category": "libs",
            "objects": [
                Object(NonMatching, "Speed/Indep/Libs/csis/dev/source/library/cmn/csis.cpp"),
            ],
        }
    )
    # r86 (X360-A): el semaforo de EAThread con su backend de Xenon (source/xenon, del EGM 0x82669258-0x8266976C).
    config.libs.append(
        {
            "lib": "eathread",
            "toolchain_version": config.linker_version,
            "cflags": cflags_game,
            "progress_category": "libs",
            "objects": [
                Object(NonMatching, "Packages/eathread/1.1.0/source/eathread_semaphore.cpp"),
                # r87 (X360-A): el mutex de EAThread de Xenon (source/xenon, EGM 0x82656070-0x82656370), el cerrojo de path.
                Object(NonMatching, "Packages/eathread/1.1.0/source/eathread_mutex.cpp"),
                # r88 (X360-A): los hilos de EAThread de Xenon (source/xenon, EGM 0x82654D18-0x82656070).
                Object(NonMatching, "Packages/eathread/1.1.0/source/eathread_thread.cpp"),
            ],
        }
    )

# # Custom build step for hashing
# config.custom_build_rules = [
#     {
#         "name": "hashgen",
#         "command": f"$python tools/hasher.py $in $out",
#         "description": "HASH $out",
#     }
# ]

# # Compile steps to automatically generate the headers containing hashes (e.g. BINHASH)
# precompile_steps = []

# sourcelist_files: list[Path] = [
#     Path("src") / object.name for object in config.libs[0]["objects"]
# ]

# for src_path in sourcelist_files:
#     gen_header = Path(
#         str(src_path).replace("SourceLists", "Src/Generated/Hashes/")
#     ).with_suffix(".h")
#     precompile_steps.append(
#         {
#             "rule": "hashgen",
#             "inputs": str(src_path),
#             "outputs": str(gen_header),
#         }
#     )

# config.custom_build_steps = {"pre-compile": precompile_steps}


# Optional callback to adjust link order. This can be used to add, remove, or reorder objects.
# This is called once per module, with the module ID and the current link order.
#
# For example, this adds "dummy.c" to the end of the DOL link order if configured with --non-matching.
# "dummy.c" *must* be configured as a Matching (or Equivalent) object in order to be linked.
def link_order_callback(module_id: int, objects: List[str]) -> List[str]:
    # Don't modify the link order for matching builds
    if not config.non_matching:
        return objects
    if module_id == 0:  # DOL
        return objects + ["dummy.c"]
    return objects


# Uncomment to enable the link order callback.
# config.link_order_callback = link_order_callback


# Optional extra categories for progress tracking
# Adjust as desired for your project
config.progress_categories = [
    ProgressCategory("game", "Game Code"),
    ProgressCategory("sdk", "SDK Code"),
    ProgressCategory("libc", "STD Code"),
    ProgressCategory("libs", "Library Code"),
]
config.progress_each_module = args.verbose

# objdiff empareja simbolos por NOMBRE, y el splitter desambigua los simbolos
# locales homonimos anadiendoles la direccion (`_803906C0`). El nuestro sale con
# el nombre pelado que emite GCC, asi que sin este mapeo objdiff no los empareja
# y la funcion se lee al 0% aunque el codigo case. `tools/project.py` conserva
# `symbol_mappings` de un objdiff.json existente, pero objdiff.json esta en
# .gitignore: hay que reponerlo en cada `configure`.
OBJDIFF_SYMBOL_MAPPINGS: Dict[str, Dict[str, str]] = {
    "main/Packages/eathread/1.1.0/source/eathread_thread": {
        "_GLOBAL_.I.AllocateThreadDynamicData__Q22EA6Threadv":
            "GlobalThreadDynamicDataInit",
    },
    "main/Packages/realmemcard/3.04.01-layer2/source/lib/gc/gc_interface": {
        "__static_initialization_and_destruction_0_803906C0": "__static_initialization_and_destruction_0",
    },
}

# This generated name is version-local. DWARF identifies both thunks as the
# destructor of GetSegmentCurveStep's static USpline; all 44 relocated bytes
# and its registration agree with the original (2026-09-22_tanda2.md).
OBJDIFF_GOWE69_SYMBOL_MAPPINGS: Dict[str, Dict[str, str]] = {
    "main/Speed/Indep/SourceLists/zWorld2": {"__tcf_4": "__tcf_3"},
    # r36e: los bytes de este comparador son IDENTICOS (16 B, sin relocs en el
    # cuerpo y con las dos referencias del llamador correctas); lo unico que
    # cambia es el sufijo de clase local, emitido por var_labelno
    # (gcc/varasm.c), que cuenta las etiquetas anteriores de la unidad. El
    # nuestro cambia con cualquier cabecera que zGameplay incluya (caduco siete
    # veces entre el 2026-09-09 y el 2026-09-23), asi que va con `*`:
    # tools/sufijo_local.py pone el numero del objeto recien compilado
    # (configure.py y scripts/pctall.py).
    "main/Speed/Indep/SourceLists/zGameplay": {
        "Compare__Q38GManager48GatherVisibleIcons__8GManagerPP5GIconP7IPlayer.0_8IconSortPCvT1.35326":
            "Compare__Q38GManager48GatherVisibleIcons__8GManagerPP5GIconP7IPlayer.0_8IconSortPCvT1.*",
    },
    # datos-gc 2026-10-13 (receta de 2026-10-13_datos-gc-1.md §7): las guardas _.tmp_N de los
    # estaticos locales solo emparejan por nombre exacto, y el sufijo .M (var_labelno) nuestro
    # nunca coincide con el del slice. Con el contador _.tmp_ de la TU ya reproducido (el N es
    # el de la misma funcion en los dos lados, medido contra el DWARF de la CU), el `*` resuelve
    # nuestro sufijo actual (tools/sufijo_local.py). zSim: _13.._16 = ConnectionClass/SType de
    # Pkt_Effect_Send/Pkt_Effect_Open (WorldConn), _20 = el `r` de Sim::GetRandom.
    "main/Speed/Indep/SourceLists/zSim": {
        "_.tmp_13.19646": "_.tmp_13.*",
        "_.tmp_14.19654": "_.tmp_14.*",
        "_.tmp_15.19665": "_.tmp_15.*",
        "_.tmp_16.19673": "_.tmp_16.*",
        "_.tmp_20.25454": "_.tmp_20.*",
    },
    # zSpeech: _18 tune (CanPlayback), _19 requests_in_queue (GameSpeech::Update), _24/_25
    # fw0/startpos (Assess180), _26/_27 speed0/t_brake_start (AssessBraking), _35 t_currdir
    # (SoundAI::CalcPlayerDirection).
    # zPhysicsBehaviors: _10/_11 Car_Open, _14/_15 Heli_Open, _18/_19 VehicleFragment_Open
    # (RenderConn) y _46/_47 Car_Open, _50/_51 Heli_Open (SoundConn).
    "main/Speed/Indep/SourceLists/zPhysicsBehaviors": {
        "_.tmp_10.18891": "_.tmp_10.*",
        "_.tmp_11.18899": "_.tmp_11.*",
        "_.tmp_14.19007": "_.tmp_14.*",
        "_.tmp_15.19015": "_.tmp_15.*",
        "_.tmp_18.19057": "_.tmp_18.*",
        "_.tmp_19.19065": "_.tmp_19.*",
        "_.tmp_46.37037": "_.tmp_46.*",
        "_.tmp_47.37045": "_.tmp_47.*",
        "_.tmp_50.37153": "_.tmp_50.*",
        "_.tmp_51.37161": "_.tmp_51.*",
    },
    # zWorld: _21/_22 Pkt_Car_Service, _25/_26 Pkt_Heli_Service, _29/_30
    # Pkt_VehicleFragment_Service, _35/_36 Pkt_Smackable_Service (RenderConn), _45
    # sCarWorldPosition, _46 TheParameterMapsManager, _47 AutoParameterAccessors,
    # _48/_49/_50 tCs/tCd/tI (CalcPartRotation).
    "main/Speed/Indep/SourceLists/zWorld": {
        "_.tmp_21.29914": "_.tmp_21.*",
        "_.tmp_22.29922": "_.tmp_22.*",
        "_.tmp_25.30030": "_.tmp_25.*",
        "_.tmp_26.30038": "_.tmp_26.*",
        "_.tmp_29.30080": "_.tmp_29.*",
        "_.tmp_30.30088": "_.tmp_30.*",
        "_.tmp_35.30143": "_.tmp_35.*",
        "_.tmp_36.30151": "_.tmp_36.*",
        "_.tmp_45.31752": "_.tmp_45.*",
        "_.tmp_46.36152": "_.tmp_46.*",
        "_.tmp_47.36160": "_.tmp_47.*",
        "_.tmp_48.36849": "_.tmp_48.*",
        "_.tmp_49.36851": "_.tmp_49.*",
        "_.tmp_50.36853": "_.tmp_50.*",
    },
    # zMain: _5/_6 Pkt_Body_Send (WorldConn), _23 SmackableParams, _32/_33
    # MNotifyEngineBlown/MEnterRaceOverFlow, _37 ExplosionParams, _39 MRestartRace, _41
    # MEnterFreeRoam, _50 MEnterPostRaceFlow, _58/_59 MNotifyCellCall*, _66
    # MNotifyOnlineRaceOver, _74/_75 MNotifySimTick/MNotifySpeechStatus, _80
    # MPlayerEnterPursuit, _82 MPursuitOver, _83 MQuitToFrontEnd, _87/_88
    # MSpawnCop/MSpawnTraffic, _95 instance (IOModule::GetIOModule).
    "main/Speed/Indep/SourceLists/zMain": {
        "_.tmp_5.12647": "_.tmp_5.*",
        "_.tmp_6.12655": "_.tmp_6.*",
        "_.tmp_23.29118": "_.tmp_23.*",
        "_.tmp_32.30965": "_.tmp_32.*",
        "_.tmp_33.30997": "_.tmp_33.*",
        "_.tmp_37.39128": "_.tmp_37.*",
        "_.tmp_39.39648": "_.tmp_39.*",
        "_.tmp_41.40340": "_.tmp_41.*",
        "_.tmp_50.41432": "_.tmp_50.*",
        "_.tmp_58.41664": "_.tmp_58.*",
        "_.tmp_59.41690": "_.tmp_59.*",
        "_.tmp_66.41914": "_.tmp_66.*",
        "_.tmp_74.42194": "_.tmp_74.*",
        "_.tmp_75.42232": "_.tmp_75.*",
        "_.tmp_80.42404": "_.tmp_80.*",
        "_.tmp_82.42462": "_.tmp_82.*",
        "_.tmp_83.42494": "_.tmp_83.*",
        "_.tmp_87.42616": "_.tmp_87.*",
        "_.tmp_88.42672": "_.tmp_88.*",
        "_.tmp_95.54981": "_.tmp_95.*",
    },
    # zPhysics: _4.._9 los TypeName de VehicleBehaviors, _10/_11 Pkt_Body_Open
    # (WorldConn), _24 temp_record (PVehicle::Construct), _25 tunings (GetTunings),
    # _40/_41 Pkt_Smackable_Open (RenderConn). El pad inicial de 36 B (r27) no tiene
    # simbolos en el ELF: queda penalizando, medido en el diario de la ronda.
    "main/Speed/Indep/SourceLists/zPhysics": {
        "_.tmp_4.13006": "_.tmp_4.*",
        "_.tmp_5.13017": "_.tmp_5.*",
        "_.tmp_6.13028": "_.tmp_6.*",
        "_.tmp_7.13039": "_.tmp_7.*",
        "_.tmp_8.13050": "_.tmp_8.*",
        "_.tmp_9.13061": "_.tmp_9.*",
        "_.tmp_10.23296": "_.tmp_10.*",
        "_.tmp_11.23304": "_.tmp_11.*",
        "_.tmp_24.28291": "_.tmp_24.*",
        "_.tmp_25.28623": "_.tmp_25.*",
        "_.tmp_40.31920": "_.tmp_40.*",
        "_.tmp_41.31928": "_.tmp_41.*",
    },
    # zTrack: _14 = el estatico de DoTunnelBloom (ScreenEffects.cpp).
    "main/Speed/Indep/SourceLists/zTrack": {
        "_.tmp_14.27615": "_.tmp_14.*",
    },
    # zFe: _18 MNotifyRaceAbandoned, _19 el `scale` de FixedPoint::GetScale, _20
    # MFlowReadyForOutro, _24/_25 los estaticos de MemoryCard::Init.
    "main/Speed/Indep/SourceLists/zFe": {
        "_.tmp_18.27407": "_.tmp_18.*",
        "_.tmp_19.29590": "_.tmp_19.*",
        "_.tmp_20.29613": "_.tmp_20.*",
        "_.tmp_24.35754": "_.tmp_24.*",
        "_.tmp_25.35757": "_.tmp_25.*",
    },
    # zAI: _0 VehicleParams, _3 MSetTrafficSpeed, _11.._20 los mensajes de policia
    # (MUnspawnCop..MControlPathfinder), _34 patrolgoal (AICopManager::UpdatePatrols),
    # _35 MJackKnife, _37 MPerpBusted, _38/_39 car_hash/heli_hash (AIPerpVehicle::Update),
    # _40 variation (AIVehiclePursuit::UpdateSiren), _41 MNotifyPursuitLength.
    "main/Speed/Indep/SourceLists/zAI": {
        # El splitter renombro el value de VehicleParams con sufijo de direccion
        # (value.10688_80456560) y asi no empareja por relocs con el nuestro; el sufijo
        # nuestro es el var_labelno actual (sufijo_local avisa si deriva: `value.*` seria
        # ambiguo con los otros seis value de la unidad).
        "value.10688_80456560": "value.5796",
        "_.tmp_0.10689": "_.tmp_0.*",
        "_.tmp_3.16708": "_.tmp_3.*",
        "_.tmp_11.18594": "_.tmp_11.*",
        "_.tmp_12.18632": "_.tmp_12.*",
        "_.tmp_13.18664": "_.tmp_13.*",
        "_.tmp_14.18696": "_.tmp_14.*",
        "_.tmp_15.18728": "_.tmp_15.*",
        "_.tmp_16.18766": "_.tmp_16.*",
        "_.tmp_17.18798": "_.tmp_17.*",
        "_.tmp_18.18830": "_.tmp_18.*",
        "_.tmp_19.18874": "_.tmp_19.*",
        "_.tmp_20.18906": "_.tmp_20.*",
        "_.tmp_34.31354": "_.tmp_34.*",
        "_.tmp_35.32499": "_.tmp_35.*",
        "_.tmp_37.35275": "_.tmp_37.*",
        "_.tmp_38.35784": "_.tmp_38.*",
        "_.tmp_39.35786": "_.tmp_39.*",
        "_.tmp_40.36215": "_.tmp_40.*",
        "_.tmp_41.37061": "_.tmp_41.*",
    },
    "main/Speed/Indep/SourceLists/zSpeech": {
        "_.tmp_18.25790": "_.tmp_18.*",
        "_.tmp_19.26706": "_.tmp_19.*",
        "_.tmp_24.29387": "_.tmp_24.*",
        "_.tmp_25.29389": "_.tmp_25.*",
        "_.tmp_26.29401": "_.tmp_26.*",
        "_.tmp_27.29403": "_.tmp_27.*",
        "_.tmp_35.30961": "_.tmp_35.*",
    },
    # datos-gc 2026-10-14, misma receta: zLua _15 MNISComplete, _16
    # MNotifySpeedTrap, _17 MNotifyRacePlacement, _18 MNotifyKnockedOut, _26
    # MNotifyChallengePassed, _27 MNotifyFinished (las seis guardas duenyas del
    # slice; el contador se reproduce con el prefijo GC de zLua.cpp).
    "main/Speed/Indep/SourceLists/zLua": {
        "_.tmp_15.29731": "_.tmp_15.*",
        "_.tmp_16.29763": "_.tmp_16.*",
        "_.tmp_17.29807": "_.tmp_17.*",
        "_.tmp_18.29851": "_.tmp_18.*",
        "_.tmp_26.30182": "_.tmp_26.*",
        "_.tmp_27.30214": "_.tmp_27.*",
    },
}

# r46: `asd2` ya no existe. Era el trozo que el troceador cortaba del final de
# gc_interface --- los dos `_GLOBAL_.I./.D.` y sus entradas de .ctors/.dtors ---
# y su rango esta fundido en gc_interface.cpp en splits.txt. Nuestros simbolos
# se llaman ya igual que los del objetivo, asi que no hacen falta mapeos: el
# unico que sigue haciendo falta es el del static-init con sufijo, que esta
# arriba en OBJDIFF_SYMBOL_MAPPINGS.
OBJDIFF_SHARED_SOURCES: Dict[str, str] = {}
OBJDIFF_SHARED_SOURCE_MAPPINGS: Dict[str, Dict[str, str]] = {}

# PC (PC49, ronda 49): los inicializadores dinamicos de MSVC 7.1 (`_$E<n>`, secciones .text$yc/.text$yd) y los
# `__ehhandler$<funcion>` de EH (.text$x; los funclets `$L<n>` NO: su numero cambia con cualquier cabecera) van al final
# del .text, en bloques por unidad, y cada SourceList tiene su rango ahi (config/SPEED_EXE_1_3/splits.json). El numero
# `<n>` se repite de una unidad a otra: delink deja el nombre pelado en la PRIMERA funcion (por direccion) y anade
# `__delink_<DIR>` a las demas. Sin este mapeo (de todas, tambien las peladas) objdiff no las empareja con el
# `_$E<n>` de nuestro objeto.
import re as _re_pc

_PC_E_DELINK = _re_pc.compile(r"^(_\$E\d+|__ehhandler\$.+?)(?:__delink_[0-9A-F]{8})?$")


def objdiff_pc_symbol_mappings() -> Dict[str, Dict[str, str]]:
    with open(os.path.join("config", "SPEED_EXE_1_3", "symbols.json"), "r", encoding="utf-8") as r:
        funcs = json.load(r)["functions"]
    with open(os.path.join("config", "SPEED_EXE_1_3", "splits.json"), "r", encoding="utf-8") as r:
        splits = json.load(r)
    rangos = sorted((lo, hi, obj) for obj, v in splits.items() for lo, hi in v.get("function_ranges") or [])
    out: Dict[str, Dict[str, str]] = {}
    vistos = set()
    for f in sorted(funcs, key=lambda f: f["start"]):
        nombre = f["name"]
        repetido = nombre in vistos
        vistos.add(nombre)
        if not nombre.startswith(("_$E", "__ehhandler$")):
            continue
        for lo, hi, obj in rangos:
            if lo <= f["start"] < hi:
                if obj.endswith(".obj"):
                    unidad = "main/" + obj[: -len(".obj")]
                    # tambien el pelado: objdiff solo empareja por nombre dentro de la MISMA seccion (.text frente a
                    # nuestro .text$yc); el mapeo explicito no mira la seccion
                    objetivo = "%s__delink_%08X" % (nombre, f["start"]) if repetido else nombre
                    out.setdefault(unidad, {})[objetivo] = nombre
                break
    return out


def apply_objdiff_symbol_mappings() -> None:
    path = Path("objdiff.json")
    if not path.is_file():
        return
    with open(path, "r", encoding="utf-8") as r:
        data = json.load(r)
    changed = False
    units_by_name = {unit["name"]: unit for unit in data.get("units", [])}
    pc_mappings = objdiff_pc_symbol_mappings() if config.version == "SPEED_EXE_1_3" else {}
    for unit in data.get("units", []):
        name = unit.get("name")
        extra = dict(OBJDIFF_SYMBOL_MAPPINGS.get(name, {}))
        # PC49: los `_$E<n>` de PC solo con PC; con otra version se quitan (project.py conserva los mapeos).
        extra.update(pc_mappings.get(name, {}))
        viejos = unit.get("symbol_mappings") or {}
        sobran = [t for t, b in viejos.items() if _PC_E_DELINK.match(t) and _PC_E_DELINK.match(t).group(1) == b
                  and pc_mappings.get(name, {}).get(t) != b]
        if sobran:
            for t in sobran:
                del viejos[t]
            if viejos:
                unit["symbol_mappings"] = viejos
            else:
                unit.pop("symbol_mappings", None)
            changed = True
        gc_mappings = OBJDIFF_GOWE69_SYMBOL_MAPPINGS.get(name, {})
        if config.version == "GOWE69":
            extra.update(gc_mappings)
        elif gc_mappings:
            # project.py preserves mappings between configurations. Remove
            # only our GC associations, not unrelated user mappings.
            mappings = dict(unit.get("symbol_mappings") or {})
            for target, base in gc_mappings.items():
                actual = mappings.get(target)
                if actual == base or (base.endswith("*") and isinstance(actual, str)
                                      and actual.startswith(base[:-1])):
                    del mappings[target]
                    changed = True
            if mappings:
                unit["symbol_mappings"] = mappings
            else:
                unit.pop("symbol_mappings", None)
        donor_name = OBJDIFF_SHARED_SOURCES.get(name) if config.version == "GOWE69" else None
        if donor_name:
            donor = units_by_name.get(donor_name, {})
            base_path = donor.get("base_path")
            source_path = donor.get("metadata", {}).get("source_path")
            if not base_path or not source_path:
                raise RuntimeError(f"Missing objdiff shared source: {donor_name}")
            if unit.get("base_path") != base_path:
                unit["base_path"] = base_path
                changed = True
            metadata = unit.setdefault("metadata", {})
            if metadata.get("source_path") != source_path:
                metadata["source_path"] = source_path
                changed = True
            extra.update(OBJDIFF_SHARED_SOURCE_MAPPINGS[name])
        if not extra:
            continue
        mappings = unit.get("symbol_mappings") or {}
        if any(mappings.get(k) != v for k, v in extra.items()):
            mappings.update(extra)
            unit["symbol_mappings"] = mappings
            changed = True
        # Los `*` (sufijos que pone el compilador) se resuelven con el objeto que
        # haya ahora; scripts/pctall.py los vuelve a resolver tras compilar.
        if resolver_sufijos(unit):
            changed = True
    if changed:
        with open(path, "w", encoding="utf-8") as w:
            json.dump(data, w, indent=2)


if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
    apply_objdiff_symbol_mappings()
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
