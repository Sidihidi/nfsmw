# NFS: Most Wanted (2005) PC 1.3: a PDB for `speed.exe`

This kit holds a real Microsoft PDB for the retail PC executable of *Need for Speed: Most Wanted* (2005), version
1.3, the tools that generate it, and the project data it is generated from. It comes from the NFS: Most Wanted
decompilation fork (`https://github.com/Sidihidi/nfsmw`).

The executable's own debug record asks for a file called `NfsMWCollectorsRelease.pdb`, with GUID
`{9B4BA937-25B1-4FFE-A718-617680C9194C}` and age 9. This PDB carries that GUID and age, so debuggers load it as a
**matched** PDB, and the executable needs no patch.

What the PDB contains:

- 26,931 named functions and 11,725 named globals;
- 20,978 functions with prototype, parameters, locals and source lines (99,132 lines in 1,429 files);
- 40,026 named types.

It holds no code or data of the game, and the kit includes no game files: you need your own copy.

**Full guide: [`docs/PC_PDB.md`](docs/PC_PDB.md)**. It covers what is in the PDB, how to use it in every tool, how to
regenerate it, how it was derived, and its caveats.

## Quick start

1. Copy `symbols/NfsMWCollectorsRelease.pdb` next to your `speed.exe`, or add the `symbols` folder to your debugger's
   symbol path.
2. Open or attach to `speed.exe` in WinDbg/cdb, Visual Studio or x32dbg. For static analysis:
   `cdb -z speed.exe -y <kit>\symbols`, then `lm v m speed`, `ln 0x406830`, `dt speed!bVector2`.
3. IDA: `File > Load file > PDB file`, or run `symbols/ida_import_symbols_pc.py` (needs IDAPython, which IDA Free
   lacks; there, use the PDB or `symbols/speed.map`). Ghidra: `File > Load PDB File`, or
   run `symbols/ImportSymbolsPC.java`. x32dbg: copy `symbols/speed.exe.dd32` into `x32\db\`.
4. Source lines point to `C:\nfsmw\src\...`. Clone the decompilation repository there, or set your tool's source path
   to your clone.

To check that your executable matches, run this from the kit folder with Python 3; it uses Windows' `dbghelp.dll`:

```
python repo\scripts\make_pdb.py --check-only --output symbols --exe "C:\path\to\speed.exe"
```

It should report `type SymPdb`, `unmatched False`, and all sampled names and lines resolved.

## Layout

| Path | Contents |
|---|---|
| `docs/PC_PDB.md` | The guide |
| `symbols/` | The PDB, `speed.map`, `symbols_pc.json`/`.csv`, `lines_pc.json`/`.csv`, IDA and Ghidra scripts, x64dbg database |
| `repo/scripts/` | `make_pdb.py` (generates `symbols/`), the engine it runs (`simbolos_pc.py`), and the naming tools `name_by_call_graph.py`, `name_by_elimination.py`, `name_corrections.py` |
| `repo/config/SPEED_EXE_1_3/` | Inputs: `symbols.json`, `nombres_procedencia.tsv`, `splits.json`, `config.yml` |
| `LICENSE`, `NOTICE.md` | CC0 1.0, and third-party notes |
| `SHA256SUMS.txt` | Checksums of every file of the kit |

`repo/` mirrors the repository layout. To regenerate, copy it over a clone of the decompilation and follow section 3
of the guide.

## Names in the kit and in the repository

The decompilation project works in Spanish. Everything in this kit is in English. A few files keep their repository
names because the build and the tools look for them there, or because they are the project's own tool:

| In the kit | In the repository | Notes |
|---|---|---|
| `repo/scripts/make_pdb.py` | `scripts/simbolos_pc.py --english` | `make_pdb.py` runs the project's `simbolos_pc.py` in its English mode; both files are in the kit |
| `repo/scripts/name_by_call_graph.py` | `scripts/nombra_grafo.py` | same code, English comments, help and messages |
| `repo/scripts/name_by_elimination.py` | `scripts/nombra_elim.py` | same code, English comments, help and messages |
| `repo/scripts/name_corrections.py` | `scripts/nombra_corrige.py` | same code, English comments, help and messages |
| `symbols/symbols_pc.*`, `lines_pc.*`, `ida_import_symbols_pc.py`, `ImportSymbolsPC.java` | `simbolos_pc.*`, `lineas_pc.*`, `ida_importa_simbolos_pc.py`, `ImportaSimbolosPC.java` | outputs of the English and the default (Spanish) mode |
| `repo/config/SPEED_EXE_1_3/nombres_procedencia.tsv` | the same path | the name provenance log ("names provenance"); the raw rule and mark values it holds are translated in Appendix B of the guide |
| `repo/config/SPEED_EXE_1_3/symbols.json`, `splits.json`, `config.yml` | the same paths | |

Generated on 2026-10-08 with `simbolos_pc.py --english --source-root C:\nfsmw --check` (what
`make_pdb.py --source-root C:\nfsmw --check` runs), from the project's state at the close of round 39.
