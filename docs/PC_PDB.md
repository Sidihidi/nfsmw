# A PDB for `speed.exe` (Need for Speed: Most Wanted 2005, PC 1.3)

This guide covers a real Microsoft PDB for the retail PC executable of *Need for Speed: Most Wanted* (2005), version 1.3.
Debuggers and disassemblers load it as the executable's own matched symbol file. The guide explains:

1. what the PDB contains and how much of the game it covers;
2. how to use the ready-made PDB, in WinDbg/cdb, Visual Studio, x64dbg, IDA and Ghidra;
3. how to generate it yourself;
4. how it was derived, step by step, and why each step can be trusted;
5. what it gets wrong, or may get wrong;
6. what is not included, and why.

The PDB comes from the NFS: Most Wanted decompilation fork described in the repository's `docs/BEYOND_UPSTREAM.md`.
Everything in the kit (`nfsmw-pc-pdb-kit.zip`) is in English: file names, columns, values, messages and the tools'
help. The project itself works in Spanish, so a few repository files keep their Spanish names; the
[equivalence table](#names-in-the-kit-and-in-the-repository) lists them.

---

## Contents

- [1. What this is](#1-what-this-is)
- [2. Quick start: using the ready-made PDB](#2-quick-start-using-the-ready-made-pdb)
- [3. Generating it yourself](#3-generating-it-yourself)
- [4. How the PDB was derived](#4-how-the-pdb-was-derived)
- [5. Caveats and accuracy](#5-caveats-and-accuracy)
- [6. Legal: what is and is not included](#6-legal-what-is-and-is-not-included)
- [Appendix A: the JSON and CSV columns](#appendix-a-the-json-and-csv-columns)
- [Appendix B: name provenance values](#appendix-b-name-provenance-values)

---

## 1. What this is

### The executable it belongs to

The PDB is for exactly one executable: `speed.exe` of the PC version 1.3. It is identified by the executable's own
debug record (an `RSDS` CodeView entry in the PE debug directory), which every Microsoft-compatible debugger uses to
find and validate a PDB:

| Field | Value |
|---|---|
| PDB file name in the record | `F:\mw\Speed\PC\CDMWCollectors\NfsMWCollectorsRelease.pdb` (only the file name matters) |
| GUID | `{9B4BA937-25B1-4FFE-A718-617680C9194C}` |
| Age | `9` |
| PE timestamp | `0x438E4C8C` (2005-12-01 01:06:20 UTC) |
| Image base / entry point | `0x00400000` / `0x007C4040` |

The reference copy used to build and test the PDB:

| | Value |
|---|---|
| File size | 5,926,912 bytes |
| SHA-1 | `f5586d5950a7cdc4ed773d0021184d5d1608a41c` |
| MD5 | `8d1d88dba3b0cdbaf45751ea2f671607` |
| SHA-256 | `0c5675a08cd71fd6d31ca87e992a915054bd8b80d268bff0561d7ecc2067e342` |

What really matters is the code and data, not the whole file. These are the SHA-1 hashes of the raw section
contents:

| Section | Address | Virtual size | SHA-1 of the raw data |
|---|---|---|---|
| `.text` | `0x00401000` | `0x48E2A5` | `79f1642f0dc3b9b48363f9ad7880858840ab3468` |
| `.rdata` | `0x00890000` | `0x05971D` | `daee3acb8059653fef578ae02c0ce47597ad89f7` |
| `.data` | `0x008EA000` | `0x0DCE10` | `ee6c260cb0a6b53ed9c00a0d52f1211e56b24994` |

A second, separately installed 1.3 executable was checked too. It has the same `RSDS` record and identical `.text`,
`.rdata` and `.data`; only `.rsrc` differs. Copies like that load the PDB exactly the same way. If your executable has
the same `RSDS` record but a different `.text`, a debugger will still accept the PDB, because it only checks the GUID
and age. A no-CD or otherwise patched executable falls in that case: most addresses still line up, but the patched
bytes do not. Section [2.6](#26-checking-your-executable) shows how to check yours.

### What the PDB contains

Figures from the build of 2026-10-08, with `config/SPEED_EXE_1_3/symbols.json` as it stood at the close of round 39
of the project:

| Contents | Count |
|---|---|
| Functions in the executable (from the IDA export) | 46,131 |
| Functions with a real name (public symbols + procedure records) | 26,931 |
| Named global data symbols | 11,725 |
| Functions whose bytes match our compiled source, verified byte by byte against the executable (relocated fields masked) | 22,177 |
| Functions with a full CodeView procedure: prototype, parameters, locals and lexical blocks | 20,978 |
| Functions with source line numbers | 20,978 |
| Source lines | 99,132 |
| Source files referenced | 1,429 |
| Named types (classes, structs, enums, typedefs) from 27 units | 40,026 |
| Named functions without type information (name and size only) | 5,953 |
| Modules (one per unit of the game, plus `unassigned_NN` ranges and `_rest`) | 57 |

In practice:

- A debugger shows the original decorated MSVC names, for example
  `?FinishAction@AIActionGetUnstuck@@UAEXXZ`, shown as `AIActionGetUnstuck::FinishAction`.
- `dt`/watch windows show real class layouts, such as `bVector2`, `UMath::Vector3` or `AIVehicle`.
- Stepping maps the original machine code to the decompiled source files of the repository.
- Parameters and locals have names and types inside every function that matches byte for byte.

Names and types follow the decompiled source of the project. They are not EA's original debug information; section
[5](#5-caveats-and-accuracy) explains the difference.

### Files of the kit

`nfsmw-pc-pdb-kit.zip` contains:

| Path in the zip | What it is |
|---|---|
| `README.md` | Short introduction; points here |
| `docs/PC_PDB.md` | This guide |
| `LICENSE`, `NOTICE.md` | License (CC0 1.0, as the repository) and third-party notes |
| `SHA256SUMS.txt` | Checksums of every file of the kit |
| `symbols/NfsMWCollectorsRelease.pdb` | The PDB |
| `symbols/speed.map` | The same symbols as an MSVC linker map (`Publics by Value`, `Rva+Base`, `Lib:Object` = unit) |
| `symbols/symbols_pc.json`, `symbols/symbols_pc.csv` | One row per symbol: names, address, size, unit, source file and line, match state, name provenance ([Appendix A](#appendix-a-the-json-and-csv-columns)) |
| `symbols/lines_pc.json`, `symbols/lines_pc.csv` | Address -> source file and line for every matched function |
| `symbols/ida_import_symbols_pc.py` | IDA 7.4+ script (needs IDAPython, so not IDA Free): names, functions, comments, source lines |
| `symbols/ImportSymbolsPC.java` | Ghidra script (Java, no Jython needed): names, functions, comments, source map |
| `symbols/speed.exe.dd32` | x64dbg database: labels, comments, function ranges |
| `repo/scripts/make_pdb.py` | Generates everything in `symbols/` (English entry point) |
| `repo/scripts/simbolos_pc.py` | The engine `make_pdb.py` runs (the project's own tool, run in its English mode) |
| `repo/scripts/name_by_call_graph.py`, `name_by_elimination.py`, `name_corrections.py` | The naming tools (section [4.4](#44-the-names)) |
| `repo/config/SPEED_EXE_1_3/` | The project's inputs: `symbols.json` (names and ranges), `nombres_procedencia.tsv` (the name provenance log), `splits.json` (address ranges per unit), `config.yml` |

`repo/` mirrors the repository layout: copy it over a clone (section [3](#3-generating-it-yourself)).

### Names in the kit and in the repository

The project works in Spanish. The kit's tools are English versions of the repository's: the same code, with English
comments, help, messages and output names. The four input files keep their repository paths, because the build and
the tools look for them there.

| In the kit | In the repository | Notes |
|---|---|---|
| `scripts/make_pdb.py` | `scripts/simbolos_pc.py --english` | `make_pdb.py` only runs `simbolos_pc.py` with `--english` |
| `scripts/name_by_call_graph.py` | `scripts/nombra_grafo.py` | same code, English text |
| `scripts/name_by_elimination.py` | `scripts/nombra_elim.py` | same code, English text |
| `scripts/name_corrections.py` | `scripts/nombra_corrige.py` | same code, English text |
| `symbols_pc.json`/`.csv`, `lines_pc.json`/`.csv`, `ida_import_symbols_pc.py`, `ImportSymbolsPC.java` | `simbolos_pc.json`/`.csv`, `lineas_pc.json`/`.csv`, `ida_importa_simbolos_pc.py`, `ImportaSimbolosPC.java` | outputs of the English and the default (Spanish) mode |
| output folder `build/SPEED_EXE_1_3/symbols/` | `build/SPEED_EXE_1_3/simbolos/` | default output folder of each mode |
| `config/SPEED_EXE_1_3/nombres_procedencia.tsv` | the same file | the name provenance log ("nombres procedencia" = names provenance); kept as is |
| `config/SPEED_EXE_1_3/symbols.json`, `splits.json`, `config.yml` | the same files | |

The provenance log and the naming tools also share the raw identifiers of the naming rules (`elim-1:1`,
`copia-ambigua`, `seguro`...): both versions of the tools write the same values, so the log stays valid for both.
[Appendix B](#appendix-b-name-provenance-values) translates each one.

---

## 2. Quick start: using the ready-made PDB

You need your own copy of the game: the kit never includes the executable. Every tool below finds the PDB in one of two
ways:

- **Next to the executable.** Copy `NfsMWCollectorsRelease.pdb` into the folder that holds `speed.exe`. The name must
  stay exactly `NfsMWCollectorsRelease.pdb`, because the executable asks for that name.
- **In a symbol path.** Leave it in the kit's `symbols\` folder and add that folder to the tool's symbol path.

The source paths inside the PDB start with `C:\nfsmw\`, for example
`C:\nfsmw\src\Speed\Indep\Src\AI\AIVehicle.cpp`. If you clone the decompilation repository to `C:\nfsmw`, source-level
debugging works without any further setting. Otherwise, point the tool's source path at your clone; each section below
says how.

### 2.1 WinDbg and cdb (tested)

WinDbg and cdb come with the *Debugging Tools for Windows* (Windows SDK). Use the 32-bit (x86) or the 64-bit
debugger: both can open a 32-bit image.

Static analysis, without running the game: `-z` opens the executable as a dump image.

```
cdb -z "C:\Games\NFS Most Wanted\speed.exe" -y "C:\nfsmw-pc-pdb-kit\symbols"
```

Then, at the prompt:

```
lm v m speed                       $$ "private pdb symbols" and the PDB path
ln 0x406830                        $$ AIActionGetUnstuck::FinishAction
x speed!AIActionGetUnstuck::*      $$ every method, with prototypes
dt speed!bVector2                  $$ the struct layout
u speed!UMath::Atan2r              $$ disassembly by name
.srcpath C:\path\to\your\clone     $$ only if the clone is not at C:\nfsmw
.lines -e                          $$ enable source line support
lsa speed!UMath::Atan2r            $$ show the source around a function
```

Live debugging: start the game, attach WinDbg (`File > Attach to process`, or `windbg -pn speed.exe`), then run
`.sympath+ C:\nfsmw-pc-pdb-kit\symbols` and `.reload /f speed.exe`. `lm v m speed` should say
`(private pdb symbols)`. WinDbg finds source files under `.srcpath` by trying shorter and shorter tails of the stored
path, so any clone location works once `.srcpath` points at the clone's root.

### 2.2 Visual Studio (not tested here; uses the same DIA/dbghelp engine)

1. `Tools > Options > Debugging > Symbols`: add the kit's `symbols` folder, or put the PDB next to `speed.exe`.
2. Debug the game: either `Debug > Attach to Process...` > `speed.exe`, or open `speed.exe` itself as a project
   (`File > Open > Project/Solution`, pick the `.exe`) and press F5.
3. In `Debug > Windows > Modules`, `speed.exe` should show *Symbols loaded* and the PDB path.
4. Source: if the clone is not at `C:\nfsmw`, Visual Studio asks for the first missing file. Point it at the clone;
   it remaps the rest. A permanent mapping can be added in the solution properties, under *Debug Source Files*.

### 2.3 x64dbg (x32dbg) (not tested here)

The game is 32-bit, so use `x32dbg`.

- **PDB.** x64dbg loads PDBs through `dbghelp`. With the PDB next to `speed.exe`, or the kit's `symbols` folder in the
  symbol path (`Options > Preferences`, symbol settings), the Symbols tab shows the names once the module loads.
- **Database.** `speed.exe.dd32` holds labels, function ranges and a comment per function:
  `unit | file:line | match % | name: provenance`. Copy it into x64dbg's `x32\db\` folder before opening `speed.exe`,
  or use the database import entry of x64dbg's `File` menu. The addresses are stored as RVAs of module `speed.exe`, so
  a relocated image base still works.

### 2.4 IDA (loading not tested here; IDA Free 9.4 installation inspected)

Three ways, from most to least complete:

1. **PDB.** `File > Load file > PDB file...`, then pick `NfsMWCollectorsRelease.pdb`. IDA's PDB plugin reads names,
   prototypes and types. The kit's PDB is an MSF 7.00 file, the format both of IDA's PDB providers (`MSDIA` and its own
   `PDBIDA`) read. When `_NT_SYMBOL_PATH` is not set, IDA's default symbol path also asks Microsoft's symbol server
   (see `cfg\pdb.cfg`). To keep IDA local, set `_NT_SYMBOL_PATH` to the kit's `symbols` folder before starting IDA,
   or put the PDB next to `speed.exe`.
2. **Script.** `File > Script file...` > `symbols\ida_import_symbols_pc.py` (IDA 7.4+ with IDAPython, Python 3).
   **IDA Free has no IDAPython**: IDA Free 9.4 ships no Python plugin, so the script cannot run there. Use the PDB or
   the map file in IDA Free. The script must stay next to `symbols_pc.json` and `lines_pc.json`. It:
   - sets the decorated names;
   - creates missing functions;
   - adds a function comment (unit, `file:line`, match %, name provenance);
   - adds source line numbers, visible with `Options > General > Line prefixes`.

   Set the environment variable `NFSMW_SRC` to your clone's root before starting IDA if it is not `C:\nfsmw`.
3. **Map file.** `File > Load file > Map file...` > `speed.map`: names only.

IDA Free 9.4 has no headless mode. It refuses `-A` and `-B` ("This version of IDA does not support the batch mode")
and `-S` ("Unknown switch '-S'"). Every step above therefore goes through the interface.

### 2.5 Ghidra (tested with 12.1.2)

- **PDB.** Import `speed.exe`, then before auto-analysis use `File > Load PDB File...` with the kit's PDB. Or leave
  the *PDB Universal* analyzer on, with the PDB next to the executable or in a configured symbol repository.
  Tested headless: 26,684 functions with prototypes, 49,330 data types and the source map of 1,424 files (figures of
  an earlier run of the same day). Ghidra prints a "failed to align" warning for 181 classes; that warning comes from
  Ghidra's structure packing and is harmless.
- **Script.** `Window > Script Manager > Script Directories`, add the kit's `symbols` folder, and run
  `ImportSymbolsPC.java`. Ghidra 12 no longer ships Jython; this script is Java, so it needs nothing else. It sets
  names, creates functions, adds a plate comment per function and fills the source map (Ghidra 11.3+:
  `Window > Source Files and Transforms`). Headless:

  ```
  analyzeHeadless C:\ghidra-projects NFSMW -import speed.exe ^
      -scriptPath C:\nfsmw-pc-pdb-kit\symbols ^
      -postScript ImportSymbolsPC.java C:\nfsmw-pc-pdb-kit\symbols C:\path\to\your\clone
  ```

  The first script argument is the folder with the JSON files. The optional second argument replaces `C:\nfsmw` as
  the source root.
- **Warning.** The `ghidra-dwarf1` extension, including the patched build in the repository's `tools/ghidra/`, makes
  every PE import fail. Its `DWARF1Analyzer.canAnalyze` throws a null pointer when the program is not an ELF. Use a
  Ghidra installation without that extension for `speed.exe`.

### 2.6 Checking your executable

The fastest check uses the same `dbghelp.dll` as the debuggers. You only need Python 3 and the kit. From the kit's
folder:

```
python repo\scripts\make_pdb.py --check-only --output symbols --exe "C:\Games\NFS Most Wanted\speed.exe"
```

Expected output, abridged:

```
module: base 0x400000, type SymPdb, pdb symbols\NfsMWCollectorsRelease.pdb, unmatched False, lines True, publics True, symbols 0
names: 2065 of 2065 sampled addresses resolve to the exact symbol
lines: 2098 of 2098 sampled addresses give the expected file and line
types: 40026 named types in the PDB
locals: 292 of 305 sampled functions have parameters or locals
```

Here `type SymPdb` with `unmatched False` means the PDB is loaded and matched; the next lines count sampled names,
lines, types and locals that resolve. With the Debugging Tools installed you can also run:

```
symchk /v "C:\Games\NFS Most Wanted\speed.exe" /s C:\nfsmw-pc-pdb-kit\symbols
```

The answer should contain `PDB Matched: TRUE`, `DBG Matched: TRUE`, `Line numbers: TRUE`, `Global syms: TRUE` and
`Type Info: TRUE`, ending with `SYMCHK: PASSED + IGNORED files = 1`.

To compare your code with the reference, hash the three sections and compare with the table in section 1:

```python
import hashlib, struct, sys
d = open(sys.argv[1], 'rb').read()
pe, = struct.unpack_from('<I', d, 0x3c)
nsec, = struct.unpack_from('<H', d, pe + 6)
optsz, = struct.unpack_from('<H', d, pe + 20)
for i in range(nsec):
    o = pe + 24 + optsz + 40 * i
    name = d[o:o + 8].rstrip(b'\0').decode('latin-1')
    rsize, rptr = struct.unpack_from('<II', d, o + 16)
    print(name, hashlib.sha1(d[rptr:rptr + rsize]).hexdigest())
```

---

## 3. Generating it yourself

You only need this to rebuild the PDB from a newer state of the decompilation, or to check that the published one is
what the project's data produces. Windows 10/11 x64 is the tested platform.

### 3.1 Requirements

| Requirement | Where it comes from | Notes |
|---|---|---|
| The repository, with the decompiled source | the fork's latest snapshot branch `avance-YYYY-MM-DD` at `https://github.com/Sidihidi/nfsmw` (`avance` = progress) | The snapshots carry upstream's layout plus the fork's source, `scripts/` and docs. They carry no game data. |
| The kit's `repo/` overlay | the zip | Brings `make_pdb.py`, `simbolos_pc.py` and the naming tools, plus `config/SPEED_EXE_1_3/` as it was when the kit was made. |
| Python 3.10 or newer | python.org | The tools use only the standard library. |
| Git | git-scm.com | To clone. With the full history, the tool also reads which round set each name. |
| ninja | `python -m pip install ninja` | `python -m ninja` works without touching `PATH`. |
| MSVC 7.1 (Visual Studio .NET 2003, `cl` 13.10.3077) | downloaded by the build | `configure.py` fetches upstream's compiler package `compilers_20260903.zip` from `github.com/dbalatoni13/compilers` into `build/compilers/`; the PC compiler is `build/compilers/Win32/7.1/`. If `cl.exe` will not start for lack of `msvcr71.dll`/`msvcp71.dll`, copy both from `build/compilers/X360/14.00.2110/` into `build/compilers/Win32/7.1/`. On Linux and macOS the build runs it through wibo, which provides them. |
| delink v0.16.4 and objdiff-cli v3.8.0 | downloaded by the build into `build/tools/` | delink cuts `speed.exe` into per-unit objects; objdiff compares them with ours. |
| Your own `speed.exe` 1.3 | your installation | Copy it to `orig/SPEED_EXE_1_3/speed.exe`. Never commit or share it. |
| `lld-link`, LLVM's COFF linker (only for the `.pdb`) | see below | Not included in the kit. |
| Debugging Tools for Windows (optional) | Windows SDK installer, feature "Debugging Tools for Windows" | `symchk` and `cdb` for checking. The check built into the tool uses the `dbghelp.dll` that ships with Windows. |

**`lld-link`.** LLVM's COFF linker is the only free linker that writes PDB files, and the tool uses it to write the
PDB. The tool never downloads it. In order, it uses:

1. the path given with `--lld`, or the `LLD_LINK` environment variable;
2. a copy it made earlier in `build/SPEED_EXE_1_3/symbols/.lld/`;
3. the `ld.lld.exe` of an Android NDK, found through `ANDROID_NDK_HOME`, `ANDROID_NDK_ROOT`, `ANDROID_HOME`,
   `ANDROID_SDK_ROOT` or `%LOCALAPPDATA%\Android\Sdk\ndk\*`. It is copied as `lld-link.exe`, because LLVM picks the
   linker flavour from the program name;
4. `lld-link` from `PATH`.

Official sources:

- **LLVM releases** (`https://github.com/llvm/llvm-project/releases`). The Windows installer `LLVM-<version>-win64.exe`
  installs `C:\Program Files\LLVM\bin\lld-link.exe`. License: Apache License 2.0 with LLVM Exceptions.
- **Android NDK** (`https://developer.android.com/ndk/downloads`, or the SDK Manager of Android Studio).
  `toolchains\llvm\prebuilt\windows-x86_64\bin\ld.lld.exe` is the same LLVM linker. The NDK package comes under the
  Android SDK license terms; LLVM itself keeps its Apache-2.0-with-LLVM-exception license.

Tested: the NDK r27 and r28c linker (`LLD 19.0.1`). An official LLVM `lld-link` is the same program but has not been
tested here.

### 3.2 Commands

```bat
:: 1. the source (cloning to C:\nfsmw makes the PDB's source paths work as they are)
git clone https://github.com/Sidihidi/nfsmw.git C:\nfsmw
cd /d C:\nfsmw
git branch -r --list "origin/avance-*"         &:: pick the newest snapshot
git checkout avance-YYYY-MM-DD

:: 2. the kit's overlay: scripts and config/SPEED_EXE_1_3 (same relative paths)
xcopy /e /i /y C:\nfsmw-pc-pdb-kit\repo C:\nfsmw

:: 3. your executable
copy "C:\Games\NFS Most Wanted\speed.exe" orig\SPEED_EXE_1_3\speed.exe

:: 4. the PC build: compiles the 30 units with MSVC 7.1, splits speed.exe, writes build\SPEED_EXE_1_3\report.json
python -m pip install ninja
python configure.py --version SPEED_EXE_1_3
python -m ninja -k 0

:: 5. the symbols and the PDB, then the check
python scripts\make_pdb.py --source-root C:\nfsmw --no-history --lld "C:\Program Files\LLVM\bin\lld-link.exe" --check
```

Notes:

- The output goes to `build\SPEED_EXE_1_3\symbols\`.
- Under heavy parallel load, MSVC 7.1 sometimes fails with `C1083` ("cannot open include file") on a header that
  exists. Run `python -m ninja -k 0` again; it only redoes what failed.
- Step 2 is optional when the snapshot is at least as new as the kit and already contains the English tools. With the
  overlay, your names, and therefore your PDB, are the same as the published ones, as long as your source compiles to
  the same objects.
- `--no-history` because a snapshot branch is a single commit: its git history cannot tell which round set each name.
  The provenance log (`nombres_procedencia.tsv`) still applies, and the published `symbols_pc.json` keeps the full
  provenance.

### 3.3 Options of `make_pdb.py`

`python scripts\make_pdb.py --help` prints them. `simbolos_pc.py --english` accepts the same options, and also the
project's Spanish spellings (`--salida`, `--prueba`, `--solo-prueba`, `--sin-pdb`, `--sin-tipos`, `--sin-historia`,
`--raiz-fuente`).

| Option | Effect |
|---|---|
| (none) | Everything: map, JSON/CSV, IDA/Ghidra/x64dbg files and the PDB, into `build/SPEED_EXE_1_3/symbols/`. Source paths are absolute to the clone that generated them. |
| `--source-root ROOT` | Write neutral paths. Source paths in the PDB and in `lines_pc.json` start with `ROOT` (for example `C:\nfsmw`). `lld-link` runs with relative paths and `/pdbsourcepath:ROOT`, and the linker's environment record in the PDB is rewritten so that it holds no path of your machine. The kit was made with `--source-root C:\nfsmw`. |
| `--lld EXE` | The linker to use (see above). |
| `--output DIR` | Output folder. |
| `--exe EXE` | The executable, if it is not `orig/SPEED_EXE_1_3/speed.exe`. |
| `--check` | After generating, load the original executable with the system `dbghelp.dll` and check samples of names, lines, types and locals. |
| `--check-only` | Only the check, on files already generated in `--output`. Needs only the two scripts `make_pdb.py` and `simbolos_pc.py`, the executable and that folder. |
| `--no-pdb` | Everything except the PDB (no linker needed). |
| `--no-types` | A PDB without the CodeView of our objects: no types, prototypes or locals, only names and lines. The tool falls back to this by itself if the linker rejects the types. |
| `--no-history` | Do not walk the git history of `symbols.json`. Names logged in `nombres_procedencia.tsv` keep their provenance; the rest get none. Use it in a clone without history, such as a snapshot branch. |

### 3.4 Time and size

On a 16-thread machine:

- **PC build.** About 20 CPU-minutes for the 30 units; a few minutes of wall time. The longest unit takes about 100 s.
  Splitting the executable takes a couple of seconds.
- **`make_pdb.py`.** One to two minutes. The first run also reads every version of `symbols.json` in the git history
  (33 versions of a 28 MB file), which adds well under a minute, and caches the result in `symbols/.history.json`.
- **Output.** About 125 MB uncompressed. The PDB is 16.6 MB; the JSON, CSV and `.dd32` files make up most of the rest.

### 3.5 Verifying what you built

- `--check`, or `--check-only`, as in section [2.6](#26-checking-your-executable). The run of 2026-10-08 resolved
  2,065 of 2,065 sampled names to the exact symbol and 2,098 of 2,098 sampled lines to the expected file and line. It
  enumerated 40,026 named types and found parameters or locals in 292 of 305 sampled functions; the other 13 have none.
- `symchk /v speed.exe /s build\SPEED_EXE_1_3\symbols`: `PDB Matched: TRUE`, with lines, globals and types.
- `cdb -z orig\SPEED_EXE_1_3\speed.exe -y build\SPEED_EXE_1_3\symbols`, then `lm v m speed`, `ln`, `x` and `dt`,
  as in section [2.1](#21-windbg-and-cdb-tested).
- The tool also stops on its own if the linked layout differs from the executable's. After linking it checks that
  `.text`, `.rdata` and `.data` land at the original RVAs.

### 3.6 Re-deriving the names (optional)

The kit's `symbols.json` already holds every name. To produce names from a newer source, as the project does after
each round of decompilation, run the naming tools after a fresh PC build. Each one reports without writing unless
given `--apply`, and validates itself before applying (section [4.4](#44-the-names)). `--help` prints each tool's
documentation.

```bat
python scripts\name_corrections.py --apply                  &:: fix proven wrong names
python scripts\name_corrections.py --copies --apply         &:: review "ambiguous copy" names
python -m ninja -k 0                                        &:: re-split with the new names, new report
python scripts\name_by_call_graph.py SPEED_EXE_1_3 --apply  &:: identical code + call graph
python scripts\name_by_elimination.py --apply               &:: linker rules: first definer + elimination
python -m ninja -k 0
```

Repeat until nothing new comes out, then run `make_pdb.py` again. Every applied name is logged in
`config/SPEED_EXE_1_3/nombres_procedencia.tsv`. `python scripts\name_by_elimination.py --remove VALUE` undoes a whole
class of them, where VALUE is a raw mark or rule of the log, for example `copia-ambigua` (Appendix B).
`python scripts\name_by_call_graph.py SPEED_EXE_1_3 --bad-anchors` lists named functions whose targets contradict
each other.

The English tools were checked against the repository's originals in two ways. First, their syntax trees are
identical once strings, comments and the added English option aliases are set aside. Second, both versions were run on
two copies of the same tree, in report mode and with `--apply`, and wrote byte-identical files: reports,
`symbols.json` and the provenance log.

---

## 4. How the PDB was derived

The executable has no symbols and no debug information of its own; its `RSDS` record points to a PDB that was never
released. Everything in this PDB comes from the decompiled source of the project, compiled with the original compiler,
and from the address of each function in the executable. Nothing is copied from the executable into the PDB. The chain
is:

1. source proven on other platforms;
2. the same source compiled for PC;
3. the executable cut into units;
4. names for the executable's functions;
5. a byte-by-byte check;
6. lines and types carried over from our objects;
7. a synthetic link that reproduces the executable's layout;
8. the executable's own GUID and age stamped into the result.

### 4.1 Source proven by GameCube and the two PS2 builds

The project decompiles *Most Wanted* against five original binaries: GameCube, two PS2 builds (the A124 prototype and
the retail US Black Edition), an Xbox 360 prototype, and PC 1.3. It is measured with `objdiff` against each.

- GameCube matches at 98.6 % of the code.
- 12,382 game functions match at the same time on GameCube (SN ProDG GCC 2.95), the A124 and SLUS-21351 (EE-GCC 2.9).
  Two different compilers produce the original bytes from the same text, which is strong evidence that the source is
  what EA wrote, up to formatting.

The GameCube build ships DWARF-1 debug information. It provides function and parameter names, local variables, types
and the original source line of each instruction. The PS2 A124 ships stabs (`.mdebug`) with member offsets, access
labels and register allocation. These "oracles" shaped the source; the repository's `docs/BEYOND_UPSTREAM.md`
describes them.

### 4.2 The same source compiled for PC

The PC build of the project compiles the same source tree, unchanged except for real platform differences guarded by
`EA_PLATFORM_WIN32`. It uses the PC compiler of the time, MSVC 7.1 (Visual Studio .NET 2003, `cl` 13.10.3077), with
the flags `configure.py` sets for `SPEED_EXE_1_3`:

- `/O2` (= `/Ox /GF /Gy`, the default Release settings of that Visual Studio). `/Gy` puts each function in its own
  COMDAT, as in the original: without it, the int3 alignment padding of the next function fell inside 53 functions.
  `/GF` puts string literals in `??_C@` COMDATs, as the original does.
- `/EHsc`: C++ exceptions. The original has `__ehhandler$` funclets.
- `/Z7`: CodeView debug information inside each `.obj` instead of a separate PDB. This is what makes the next steps
  possible: every object carries, per function, its prototype, parameters, locals, the types of its unit and its line
  numbers.

Of the functions proven on GameCube and both PS2 builds that have a name on PC, 77 % also match the PC executable
byte for byte. The rest differ because of MSVC: inlining decisions, x87 ordering, PC-only constants.

### 4.3 Cutting the executable into units

`config/SPEED_EXE_1_3/symbols.json` is an IDA export of the executable: function ranges, names, segments and 104,521
relocations. It came from upstream (`dbalatoni13/nfsmw`) and the project renamed it since. `splits.json` assigns
address ranges to units. A unit is one of EA's "SourceLists" (`zAI`, `zCamera`, `zPhysics`...), and MSVC links them in
address order. Ranges with no unit of ours (C runtime, DirectX glue, middleware without PC source) are
`unassigned_NN`.

`delink` (`github.com/dbalatoni13/delink`) uses the export to cut `speed.exe` into one COFF object per unit
(`build/SPEED_EXE_1_3/obj/`), with the relocations rebuilt. `objdiff-cli` then compares each of our objects with the
cut one, function by function and **by name**. It runs with `functionRelocDiffs=none`, so relocation targets do not
count, only the instruction bytes around them. Its `report.json` gives each function a match percentage.

Because objdiff pairs by name, a function the executable has under the wrong name, or no name, never matches, however
good the source is. On PC, **naming was the bottleneck**.

### 4.4 The names

Names come from several sources, each recorded (see [Appendix B](#appendix-b-name-provenance-values)):

1. **Upstream's IDA export.** The names upstream had already recovered for `speed.exe` (`upstream`).
2. **An earlier partial PDB.** A community reconstruction with about a quarter of the symbols, read with the
   repository's `scripts/pdb.py` (`partial-pdb`, 283 names kept).
3. **Need for Speed Undercover (Wii).** Its ELF keeps symbols and shares code with *Most Wanted* (`undercover-wii`,
   82 names).
4. **Identical code plus the call graph** (`name_by_call_graph.py`, provenance `call-graph`).
   - An unnamed function of the executable takes the name of one of ours when their *skeletons* are equal. The
     skeleton is the bytes with every relocated field set to zero.
   - The relocation targets are the evidence. A named target must have the same name on both sides. Float constants
     (`__real@`) and string literals (`??_C@`) are checked against the executable's bytes. An unnamed target inherits
     the name that the already accepted pairs call there.
   - Hiding 30 % of the known names and asking for them back gave 2,546 of 2,546 on PC. It named about 2,470
     functions in its first run.
5. **The linker's own rules** (`name_by_elimination.py`, provenance `elimination-1to1`, `ambiguous-copy`, and
   confirmed variants such as `elimination-ordered+targets`). Three facts about MSVC's linker made thousands more pairs
   decidable:
   - **First definer.** An inline or template function (a COMDAT) is kept from the *first* object, in link order,
     that contributes it. One of our functions can therefore only appear in the first unit whose object defines it.
     This held for 944 of 957 known names with several definers.
   - **Elimination.** A function of ours whose name is already placed elsewhere in `symbols.json` is not a candidate
     anywhere else.
   - **Targets by address, not by name.** IDA sometimes gives one address two names (`??3@YAXPAX@Z` and
     `?Free@FastMem@@...`). Compared by name, those looked like contradictions.

   On top of these facts it applies two rules:
   - **one-to-one** (`elimination-1to1`): one function of the executable and one of ours share a skeleton.
   - **ordered** (`elimination-ordered`): N identical copies on each side. They are paired in order (address in the
     executable against definition order in our object), and a pair is kept only if it falls between its
     neighbouring anchors.

   Filters and checks:
   - A function under 16 bytes with relocations needs at least one confirmed target. A `push; call X; ret` wrapper
     has the same skeleton for every X.
   - Every pair is checked with `objdiff-cli diff` on a renamed copy of the cut object, and must score 100 %.
   - **Validation is mandatory before applying.** The tool hides 30 % of the known identical-code names and asks for
     them back. It refuses to write anything if the one-to-one rule falls below 99 % or the ordered rule below 95 %.
     Measured on the third pass: 3,998 of 3,998 and 1,454 of 1,458.
   - Every applied name goes to the provenance log (`nombres_procedencia.tsv`): address, previous name, new name,
     rule, mark, size, confirmed targets and date.
   - Ordered names carry the mark **`ambiguous-copy`**. The code is identical, but the name may be swapped with
     another copy of the same code.
   - Together with the new `zFeOverlay` unit, this added about 8,100 names in one round.
6. **Correcting wrong names** (`name_corrections.py`, round 39; provenance `fixed-by-code`, `fixed-by-callers`,
   `fixed-decoration`, `moved-by-code`, `named-by-callers`, `removed-duplicate`, `removed-false`, `copy-corrected`).
   The tools above only name *unnamed* functions, but inherited names can be wrong.
   - A name is replaced only with two kinds of proof. Code: a 1:1 skeleton pair with the same filters. Callers:
     *strong anchors*, names that match byte for byte and have a unique skeleton, call that address where our code
     calls that name. Caller evidence must be unanimous.
   - Its validation shuffles names on purpose and requires 99 % recovery.
   - It also re-examines `ambiguous-copy` names: those that targets or callers confirm become `confirmed`, and those
     the callers contradict are swapped within their group.

The figures in section [1](#1-what-this-is) count the names as they were when the kit was generated: round 39 had
already applied the correction rule once, and was still working.

### 4.5 The byte-by-byte check

The tool does not take the report's word for a match. For each function the report scores at 100 %:

- it finds the function of the same name and size in our object (`build/SPEED_EXE_1_3/src/.../zXxx.obj`);
- it takes its bytes and the offsets covered by its relocations;
- it compares the bytes with the executable's at the function's address, skipping only the relocated fields.

Only functions that pass get line numbers and CodeView in the PDB. They are marked `verified` in the JSON; there were
22,177 of the 22,179 at 100 %. For those functions, every instruction offset inside the function is the same in the
original as in our object, so an offset-relative line table or scope record carries over exactly.

### 4.6 The `RSDS` record: why it loads without touching the executable

A PE file built with debug information carries a debug-directory entry of type `IMAGE_DEBUG_TYPE_CODEVIEW`. In MSVC
7.1 executables that entry is an `RSDS` record: a GUID, an age and the PDB path the linker wrote. A debugger:

1. takes the file name from that path (`NfsMWCollectorsRelease.pdb`);
2. looks for it next to the executable and along the symbol path;
3. opens it and compares the GUID in the PDB information stream (stream 1) and the age in the DBI stream (stream 3)
   with the record;
4. accepts it as "matched" only if both are equal.

The executable is never modified. After linking, the tool writes the executable's GUID, age and timestamp into
stream 1 and the age into the DBI header. The PDB is named as the record asks.

### 4.7 Line numbers from `/Z7` objects

MSVC 7.1 does not put line numbers in CodeView (`.debug$S`). It keeps the classic COFF line numbers:

- each function symbol has an auxiliary record whose tag points to its `.bf` symbol, and the `.bf` auxiliary record
  holds the function's base line;
- the section's line-number table lists (offset, line delta) pairs for the function;
- the source file is the closest preceding `.file` symbol (storage class 103).

The tool reads those records from each of our objects. MSVC writes the paths in lower case, so the tool restores their
real case from the repository. It keeps the lines only for verified functions and writes them as C13 line blocks
(`DEBUG_S_LINES`). There were 99,132 lines in 20,978 functions.

### 4.8 Types, prototypes and locals from CodeView

For each verified function, the tool copies the records of the associative `.debug$S` section of its COMDAT:
`S_GPROC32`/`S_LPROC32` with the function's type index, then parameters, locals, blocks and labels down to the closing
`S_END`. It also copies the module-level records it can keep (`S_UDT`, `S_CONSTANT`, `S_GDATA32`/`S_LDATA32`,
`S_COMPILE2`) and the unit's type stream (`.debug$T`).

Some records point to code or data through relocations (`SECREL`/`SECTION`). Those are retargeted:

- a relocation into the function itself -> the function's symbol at its original address;
- a relocation to a global that has a public name in the executable -> that name;
- anything else -> the record is dropped, together with its whole scope, so that no record ever points to a wrong
  place.

Types are taken from the 27 units that have verified functions. The PDB lists 40,026 named types.

Named functions that are not verified (not matched yet, or matched without our object at hand) still get a minimal
procedure record: name and size, without type, parameters or lines. There are 5,953 of them. Data symbols with a name
(11,725) become publics, and globals become `S_GDATA32` when our objects provide their types.

### 4.9 The synthetic link

The tool writes a small COFF object per unit and lets LLVM's `lld-link` write the PDB:

- **The `.text` layout.** The executable's `.text` is cut into runs of consecutive functions of the same unit. Each
  run becomes a section named `.text$<address in hex>`, with 1-byte alignment, filled with `int3` (`0xCC`) and
  exactly as long as the run. The linker sorts `$` subsections by name, and fixed-width hex names sort by address.
  The sections therefore concatenate into a `.text` with every function at its original address. Gaps go to an object
  called `_rest`.
- **Data.** `.rdata` and `.data` are zero-filled sections of the original sizes in `_rest`.
- **Symbols.** Every named function and global is defined at its offset. The first occurrence of a name is public; a
  repeated name, or one the executable keeps as static, is local.
- **Debug data.** A C13 `.debug$S` per object (symbols, lines, file checksums and string table) and the unit's
  `.debug$T`. A `.debug$S` section is split before it reaches 65,535 relocations.
- **The link.** `lld-link /dll /noentry /fixed /base:0x400000 /machine:x86 /debug:full /opt:noref /opt:noicf
  /incremental:no /safeseh:no /nodefaultlib`. After linking, the tool checks that `.text`, `.rdata` and `.data`
  landed at the RVAs of the original (`0x1000`, `0x490000`, `0x4EA000`) and stops if not. The linked DLL is thrown
  away; only the PDB is kept.

Each unit becomes a module of the PDB named after it (`zAI.obj`, `zPhysics.obj`...): 57 modules. If `lld-link`
rejects a unit's types, the tool links again without types (as with `--no-types`) rather than fail.

### 4.10 Stamping and neutral paths

The tool writes the executable's identity into the PDB (section
[4.6](#46-the-rsds-record-why-it-loads-without-touching-the-executable)). With `--source-root`, the published PDB holds
no path of the machine that built it:

- source files are written as `ROOT\src\...`;
- `lld-link` runs with relative paths and `/pdbsourcepath:ROOT`, so module paths and the working directory become
  `ROOT`;
- the linker module's `S_ENVBLOCK`, which `lld` fills with its own absolute path, is rewritten in place with the same
  record length.

---

## 5. Caveats and accuracy

- **`ambiguous-copy` names.** Names placed by the ordered rule (about 5,000 in this build) are given to identical
  copies of the same code, in order. The code under the name is right. Which copy carries which name may be swapped,
  about 3 % by validation. Disassembly and decompilation are unaffected; cross-references between copies may be
  crossed. They are marked in `symbols_pc.json` (`provenance: ambiguous-copy`, `mark`), and round 39's review already
  promoted the ones the call graph confirms.
- **Inherited names can be wrong.** Names from IDA, upstream, the old partial PDB or early rounds were never proven
  one by one. A wrong name on identical code still scores 100 % in objdiff, because relocation targets do not count.
  The correction rule (`name_corrections.py`, section [4.4](#44-the-names)) fixes those it can prove.
  `name_by_call_graph.py SPEED_EXE_1_3 --bad-anchors` lists named functions whose targets contradict each other. The
  `provenance` column tells where each name came from; trust `elimination-1to1`, the `confirmed` mark and verified
  functions most.
- **Functions without types or lines.** About 6,000 named functions are not matched yet, or are matched without our
  object at hand. They have a name and a size only: no prototype, no locals, no source line. About 19,000 functions
  are still `sub_...`; they are left out of the PDB, the map and the scripts, and kept in the JSON with `real: false`.
- **Our source, not EA's.** Lines, file names, parameter and local names, and type layouts are those of the project's
  reconstruction.
  - File paths follow the original layout where the GameCube DWARF gives it (`src/Speed/Indep/Src/...`), but line
    numbers are lines of *our* files, not of EA's.
  - Type layouts are what MSVC 7.1 computes from our headers. They are confirmed indirectly, because the code that
    uses them matches byte for byte, but no original PC type information exists to compare with.
  - Types and lines of the standard library come from the MSVC 7.1 headers (`build/compilers/Win32/7.1/Include`).
- **Source paths.** All source paths start with `C:\nfsmw`. Clone there, or set your tool's source path
  (section [2](#2-quick-start-using-the-ready-made-pdb)).
- **Data sizes** are the distance to the next symbol in the same section. They are an upper bound, not the real size of
  the object.
- **`unassigned_NN` modules** are address ranges without a unit of ours: C runtime and STL of MSVC 7.1, DLL import
  thunks, DirectX glue, dirtysock, PC realcore, VP6, `zOnline`. Their names come from the IDA export and are least
  checked.
- **Regenerate after each round.** The project names and matches more functions every round. This PDB is a snapshot of
  2026-10-08 (end of round 39). Rebuilding it takes a PC build and about a minute.

---

## 6. Legal: what is and is not included

**Not included, and never will be:**

- the game executable (`speed.exe`) or any other file of the game: code, data, assets;
- compilers or SDKs. MSVC 7.1 is proprietary; the build downloads upstream's compiler package from its own mirror.
  The kit carries nothing of it;
- `lld-link`/`ld.lld` (get it from LLVM or the Android NDK, section [3.1](#31-requirements)).

**What the PDB and the other files contain:** names, addresses, sizes, types and line numbers. The names are the
decorated MSVC names of the game's functions and globals, as recovered by the project. The types and lines come from
the project's own decompiled source.

- The PDB contains **no machine code and no data of the game.** Its `.text` was filled with `int3` and its data
  sections with zeros, and a PDB does not store section contents anyway.
- This was checked on the published files. Every non-trivial 16-byte block of the executable's `.text`, `.rdata` and
  `.data` (264,164 blocks) was searched for at every offset of every file of the kit. No block of `.text` occurs.
  The only blocks of `.rdata` and `.data` that occur are identifier strings (class and RTTI names that are also
  symbol names), runs of consecutive integers, and the GUID of the `RSDS` record, which the PDB must contain to match.

**License.** The repository is released under CC0 1.0 (`LICENSE`), and so are the kit's scripts, documentation and
data. The names in `symbols.json` build on upstream's work (`dbalatoni13/nfsmw`, also CC0). *Need for Speed* and
*Most Wanted* are trademarks of Electronic Arts. This is an unofficial fan research project, not affiliated with or
endorsed by EA. You need your own legally obtained copy of the game.

---

## Appendix A: the JSON and CSV columns

`symbols_pc.json` has a `header` and `symbols` (one object per symbol). The header holds the executable's `base`,
`timestamp`, `entry_point`, `rsds` record (`guid`, `age`, `pdb`), `sections`, the `generated` date and a `summary`.
`symbols_pc.csv` has the same columns:

| Column | Meaning |
|---|---|
| `address` | Virtual address (`0x00406830`) |
| `rva` | Address minus the image base |
| `section`, `secoff` | Section name, and `section:offset` as in a map file |
| `kind` | `function` or `data` |
| `size` | Size in bytes (functions: from the IDA export; data: distance to the next symbol) |
| `name` | Decorated MSVC name, as in the PDB |
| `undecorated`, `short_name` | Undecorated name (full, then without type information), from MSVC 7.1's `undname` |
| `real` | `false` for names IDA invents (`sub_`, `dword_`, `aString`, `FID_conflict:`...) |
| `static` | The executable keeps it as a static (local) symbol |
| `unit`, `sourcelist` | Unit (`zAI`, `unassigned_03`...) and its SourceList file |
| `file`, `line` | Our source file (relative to the repository) and the first line of the function |
| `match_percent` | objdiff match percentage |
| `verified` | Bytes compared with the executable and equal (relocated fields masked) |
| `has_lines` | Has a line table in the PDB |
| `provenance` | Where the name came from ([Appendix B](#appendix-b-name-provenance-values)) |
| `mark` | Mark of the automatic naming tools: `confirmed`, `ambiguous-copy`, `decoration` |
| `round` | The round of the project that set the name (`round 39`, `window 17`, `close of round 13`, `r79`), or `earlier commit` |

`lines_pc.json` holds `root` (`C:\nfsmw`), `files` (relative paths), and `functions`. Each function has its
`address`, `size`, `file` (an index into `files`) and `lines`, a list of `[offset, line]` pairs. `lines_pc.csv` is
flat: `address`, `file`, `line`, `function`.

## Appendix B: name provenance values

The `provenance` column of `symbols_pc.json`, and the raw value it comes from. The raw values are what the project
writes in `nombres_procedencia.tsv` (rules and marks) or what the repository's Spanish output shows.

| `provenance` | Raw value | Source |
|---|---|---|
| `upstream` | `upstream` | upstream's IDA export and later upstream renames |
| `partial-pdb` | `pdb` | the earlier partial PDB, imported with the repository's `scripts/pdb.py` |
| `undercover-wii` | `undercover-wii` | *Need for Speed Undercover* Wii symbols |
| `call-graph` | `grafo` | `name_by_call_graph.py`: identical code and call graph |
| `by-code` | `codigo` | a round that named it while decompiling (identical code, by hand or by the repository's `tools/nombra_por_codigo.py`) |
| `elimination-1to1` | `elim-1:1` (shown as `elim 1:1`) | `name_by_elimination.py`, one-to-one pair |
| `ambiguous-copy` | `elim-orden` with mark `copia-ambigua` | `name_by_elimination.py`, identical copies paired in order (see caveats) |
| `elimination-ordered+targets`, `elimination-ordered+callers`, `elimination-ordered+targets+callers` | `elim-orden+destinos`, `elim-orden+llamantes`, `elim-orden+destinos+llamantes` | an ordered copy later confirmed by its targets or its callers (`name_corrections.py --copies`) |
| `copy-corrected+targets`, `copy-corrected+callers` | `copia-corregida+destinos`, `copia-corregida+llamantes` | an ordered copy whose name was swapped within its group |
| `fixed-by-code` | `corrige-codigo` | `name_corrections.py`: wrong name replaced by a 1:1 code pair |
| `fixed-by-callers` | `corrige-llamantes` | `name_corrections.py`: wrong name replaced on the callers' evidence |
| `fixed-decoration` | `corrige-decoracion` | `name_corrections.py`: same function, decoration corrected |
| `moved-by-code` | `mueve-codigo` | `name_corrections.py`: a free name moved to its real address |
| `named-by-callers` | `nombra-llamantes` | `name_corrections.py`: unnamed near-match named by its callers |
| `removed-duplicate`, `removed-false` | `quita-duplicado`, `quita-falso` | `name_corrections.py`: a false placement taken back to `sub_` |
| `uncommitted` | `sin commit` | changed in the working tree after the last commit |
| `unnamed` | `ida (sin nombre)` | no real name (`real: false`) |
| empty | | generated with `--no-history` and not in the provenance log |

Marks (`mark` column; raw value in the log): `confirmed` (`seguro`), `ambiguous-copy` (`copia-ambigua`), `decoration`
(`decoracion`). The naming tools' own reports (`--tsv`) use the raw identifiers too; in `name_by_call_graph.py` the
motive of each pair is `unica` (the only compatible pair), `llamantes` (named by its callers), `secuencia` (by the
sequence of calls) or `mejor` (the best of several by confirmed targets).
