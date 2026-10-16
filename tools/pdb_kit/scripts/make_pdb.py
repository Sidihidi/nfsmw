#!/usr/bin/env python3
"""make_pdb.py -- English entry point of the PC PDB tool of the NFS: Most Wanted decompilation.

It runs scripts/simbolos_pc.py (the project's own tool, which must sit next to this file) in its English mode: English
file names (symbols_pc.json/.csv, lines_pc.json/.csv, ida_import_symbols_pc.py, ImportSymbolsPC.java, speed.map,
speed.exe.dd32, NfsMWCollectorsRelease.pdb), English columns and values, English messages and help. All options of
simbolos_pc.py are accepted; the main ones:

    python scripts/make_pdb.py                                   # everything, into build/SPEED_EXE_1_3/symbols/
    python scripts/make_pdb.py --source-root C:\\nfsmw --check    # neutral source paths, then check with dbghelp
    python scripts/make_pdb.py --no-pdb                          # everything except the PDB (no linker needed)
    python scripts/make_pdb.py --lld "C:\\Program Files\\LLVM\\bin\\lld-link.exe"
    python scripts/make_pdb.py --check-only --output DIR --exe PATH\\speed.exe   # check a PDB that is already built

Run `python scripts/make_pdb.py --help` for the full list. The guide is docs/PC_PDB.md.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import simbolos_pc  # noqa: E402

if __name__ == '__main__':
    sys.argv[0] = 'make_pdb.py'
    simbolos_pc.main(['--english'] + sys.argv[1:])
