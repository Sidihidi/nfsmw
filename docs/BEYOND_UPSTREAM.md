# NFS: Most Wanted decompilation — what this fork adds beyond upstream

This document is a guide to everything the `Sidihidi/nfsmw` fork adds on top of
upstream ([dbalatoni13/nfsmw](https://github.com/dbalatoni13/nfsmw)): the
discoveries, the source changes (file by file where a file carries a finding),
the tools, and — for each of them — **why** it was done, **which data** it comes
from and **how** that data was obtained. It is written for the other people
working on the decompilation, so that anything useful here can be reused,
checked or upstreamed.

State of this document: **2026-09-30** (first written 2026-09-18). The fork branched from upstream at
`59f184a0` ("5% linked! (#123)") and carries about 1,890 commits on top of it. Upstream's later work is not merged
as history: it is integrated hunk by hunk, each hunk measured, the latest being `9ca26bc1` ("zTrack work (#144)").
Compare against `59f184a0` when a diff looks surprising.

---

## At a glance

Five builds are compiled and measured, each against its own original binary:

| build | platform / compiler | upstream `fd0c355` (2026-09-16) | this fork (2026-10-16) |
|---|---|---|---|
| `GOWE69` | GameCube, SN ProDG GCC 2.95.3 | 55.55 % code, 11,074 functions | **98.70 %** code, 18,391 / 18,432 functions (data 50.83 %) |
| `SLES-53558-A124` | PS2 prototype "A124", EE-GCC 2.9-ee-991111 | 17.31 % | **78.58 %**, 21,452 / 25,358 |
| `SLUS-21351` | PS2 retail (Black Edition), EE-GCC 2.9 | 18.09 % | **80.97 %**, 18,795 / 22,003 |
| `EUROPEGERMILESTONE` | Xbox 360 milestone, MSVC (PPC) | 0.68 % | **21.89 %**, 21,236 / 57,575 (LTCG link) |
| `SPEED_EXE_1_3` | PC 1.3, MSVC 7.1 | 6.54 % | **41.80 %**, 31,352 / 54,172 |

Of the 41 GameCube entries still open, one is not a function: `pad_00_8000348C_init` is 20 bytes of alignment added
when the ELF was converted to a DOL, outside every ELF section; it stays in the count rather than being dropped or
mapped to another symbol. The `path` library was built with SN ProDG v1.83, which nobody publishes; the fork now
replicates it (see "Replicating a compiler that is not public") and, with `-mps-float` like its sibling `snd`, all 194
of its functions match.

How the numbers are taken, so that they are comparable:

- One `objdiff-cli` report per build, with the same options upstream uses
  (`functionRelocDiffs=none`, `ppc.calculatePoolRelocations=false`). Every unit
  counts as non-matching: the `Matching` flag in `configure.py` is ignored
  (`scripts/pctall.py` marks every unit incomplete; objdiff scores a `complete`
  unit at 100 % without comparing it, which until 2026-09-18 added 124 and 261
  functions that do not match to the two PS2 figures).
- A figure is only valid if the run also says **"0 objects fail to compile"**
  (objdiff otherwise counts a `complete` unit without an object as 100 %).
  `scripts/pctall.py` measures the five builds and prints that control.
- Upstream's GameCube figure was reproduced locally with upstream's own
  `configure.py` at `fd0c355` and matches decomp.dev to the decimal; the other
  upstream figures are decomp.dev's per-version numbers at the same commit.
- GameCube control: the linked DOL must keep SHA-1
  `9619ba57c9919f95f7f2ac951a2166a3517f91e3` unless a change is meant to move
  it.

## Ground rules this fork follows

These rules explain the shape of many changes described below.

1. **Real, portable code.** A real 80 % that compiles on every platform is worth
   more than a 100 % that only compiles on GameCube. There is **no
   compiler-steering inline asm** left in the game code (register pins, empty
   barriers and asm aliases were all removed). The inline asm that remains is
   what the binary proves EA wrote: hardware that C cannot produce — PS2 VU0
   macros, MMI and COP0, GameCube paired-single, GQR and cache instructions,
   `frsqrte`/`fres`/`fsel`/`fabs`, the SN and Dolphin runtime — and the few FPU
   sequences no C form can reproduce (lesson 90: `bSqrt` on GameCube,
   `min.s`/`max.s`/`abs.s` and the float-to-int conversions on PS2), plus
   toolchain headers. When it is not obvious whether the original was asm, the
   line map settles it: an asm statement is one or a few source lines over the
   whole instruction sequence (see the line map, below). Four assembler sources
   remain, all runtime code: SN's `crt0.s`, the MetroTRK debugger stub
   (`metrotrk.s`, hand-written assembler in the original), the PPC register
   save/restore helpers (`pro2D4.s`) and newlib's `setjmp.s`. The matching
   devices of earlier phases (dead "primer" functions, placement attributes,
   joined literals, invented locals) were removed too; what that cost is listed
   at the end, under "Open fronts". Many functions keep a comment block with the
   experiments that were measured (what was tried and what it gave), so the
   next person does not repeat them.
2. **Differences between builds are proven, not guessed.** A member, a function
   or an alignment that differs between versions is guarded with the platform or
   version define (`EA_PLATFORM_*`, `EA_BUILD_A124` for the PS2 prototype,
   `MILESTONE_BUILD`, …) only when a binary shows the difference: DWARF, the
   A124 stabs, a type dump, or the offsets the original code uses on another
   platform. Guards remove a statement, a sub-condition or an `if` header; code
   blocks are not duplicated.
3. **Find the common cause, not the function.** A wrong member offset breaks
   every function that touches the class; a wrong compiler flag breaks a whole
   library. Most of the gains listed here came from one fix that moved dozens or
   hundreds of functions.
4. **Confirm with an oracle before editing, measure all five builds after.** A
   layout change is checked first with a compiler probe (the compiler is asked
   for `sizeof`/`offsetof` with the real flags), then the five builds are
   measured and the DOL control is checked, and the commit carries the evidence.

## How this document is organised

1. **Data sources in the original binaries** — the "oracles": what debug data
   each version ships, where it lives and which tool reads it.
2. **Compiler and linker knowledge** — measured behaviours of GCC 2.95 / EE-GCC
   2.9 / the linker that decide whether code matches.
3. **Traps** — the places where a tool or a metric lies.
4. **Working method.**
5. **Build and measurement changes, and the tool catalogue.**
6. **The source tree, area by area and file by file.**
7. **Open fronts** — things that are diagnosed but not done.

## Where the work is published

The fork's `main` follows upstream. The work is published as **clean snapshot
branches** `avance-YYYY-MM-DD`: one commit each, whose parent is upstream's
`59f184a0`, with upstream's root layout plus `scripts/` and this guide, and
**no game data**. The local history is not pushed: it contains a large binary
that was committed by mistake early on.

## Contents

- [Knowledge: oracles, compiler behaviour, traps and method](#knowledge-oracles-compiler-behaviour-traps-and-method)
  - [Data sources in the original binaries (the oracles)](#data-sources-in-the-original-binaries-the-oracles)
    - [GameCube DWARF-1 (`.debug`, 92 MB, 316 compile units)](#gamecube-dwarf-1-debug-92-mb-316-compile-units)
    - [GameCube `.debug_aranges`: unit boundaries (`scripts/aranges.py`)](#gamecube-debug_aranges-unit-boundaries-scriptsarangespy)
    - [GameCube `.debug_pubnames` (`scripts/pubnames.py`)](#gamecube-debug_pubnames-scriptspubnamespy)
    - [GameCube line map (`symbols/debug_lines.txt`, 525,964 entries)](#gamecube-line-map-symbolsdebug_linestxt-525964-entries)
    - [GameCube symbol table](#gamecube-symbol-table)
    - [PS2 A124 `.mdebug` (stabs)](#ps2-a124-mdebug-stabs)
    - [PS2 A124 `NFS.MAP` and type dump](#ps2-a124-nfsmap-and-type-dump)
    - [Stripped builds and names](#stripped-builds-and-names)
    - [The compiler's own source, and Ghidra (`orig/prodg/NGC_GNU_SRC/NGC/gcc/`)](#the-compilers-own-source-and-ghidra-origprodgngc_gnu_srcngcgcc)
  - [Compiler and linker knowledge](#compiler-and-linker-knowledge)
    - [Control flow](#control-flow)
    - [Expressions and folding](#expressions-and-folding)
    - [Scheduling and register allocation](#scheduling-and-register-allocation)
    - [Locals, blocks and frames](#locals-blocks-and-frames)
    - [Inline functions, templates and emission order](#inline-functions-templates-and-emission-order)
    - [Data placement, constant pool and linker](#data-placement-constant-pool-and-linker)
    - [Flags and compilers](#flags-and-compilers)
    - [Metrowerks (`mwcceppc`) units](#metrowerks-mwcceppc-units)
    - [PS2 (EE-GCC)](#ps2-ee-gcc)
  - [Traps: where tools lie](#traps-where-tools-lie)
    - [objdiff](#objdiff)
    - [Build and staleness](#build-and-staleness)
    - [Our own measurement tools](#our-own-measurement-tools)
    - [Reading and setup](#reading-and-setup)
  - [Working method](#working-method)
    - [How we measure](#how-we-measure)
    - [The cycle](#the-cycle)
    - [What a commit carries](#what-a-commit-carries)
- [Build, measurement and tools](#build-measurement-and-tools)
  - [Build and measurement changes](#build-and-measurement-changes)
    - [Five builds from one `configure.py`](#five-builds-from-one-configurepy)
    - [Per-library compilers and flags](#per-library-compilers-and-flags)
    - [Replicating a compiler that is not public: SN v1.83 for `path`](#replicating-a-compiler-that-is-not-public-sn-v183-for-path)
    - [Building SN's compiler from its GPL source](#building-sns-compiler-from-its-gpl-source)
    - [How EA's inline asm is proven: the compiler as judge](#how-eas-inline-asm-is-proven-the-compiler-as-judge)
    - [The EE-GCC two-step wrapper (`tools/ee_gcc_pp.py`)](#the-ee-gcc-two-step-wrapper-toolsee_gcc_pppy)
    - [Section renaming (`section_rename`)](#section-renaming-section_rename)
    - [Build-graph fixes (`tools/project.py`)](#build-graph-fixes-toolsprojectpy)
    - [Small changes to upstream tools](#small-changes-to-upstream-tools)
    - [objdiff options and name mappings](#objdiff-options-and-name-mappings)
    - [Measuring five builds: `scripts/pctall.py`](#measuring-five-builds-scriptspctallpy)
    - [Xbox 360 is measured on an LTCG link: `scripts/ltcg360.py`](#xbox-360-is-measured-on-an-ltcg-link-scriptsltcg360py)
    - [`config/<version>/`](#configversion)
  - [Tool catalogue](#tool-catalogue)
    - [Measurement and progress](#measurement-and-progress)
    - [Building and fast iteration](#building-and-fast-iteration)
    - [Diagnosing a function or a unit](#diagnosing-a-function-or-a-unit)
    - [Register allocation and scheduling](#register-allocation-and-scheduling)
    - [PS2 layout probes](#ps2-layout-probes)
    - [Readers of the original's debug data](#readers-of-the-originals-debug-data)
    - [GameCube splits, symbols, keep list and data](#gamecube-splits-symbols-keep-list-and-data)
    - [GameCube link and DOL checks](#gamecube-link-and-dol-checks)
    - [Publishing](#publishing)
    - [Ghidra](#ghidra)
    - [Xbox 360 and PC names](#xbox-360-and-pc-names)
    - [A PDB for the PC executable (`scripts/simbolos_pc.py`)](#a-pdb-for-the-pc-executable-scriptssimbolos_pcpy)
    - [Build infrastructure in `tools/`](#build-infrastructure-in-tools)
    - [Android port experiment (`port/`)](#android-port-experiment-port)
  - [Documentation added](#documentation-added)
- [The source tree, area by area and file by file](#the-source-tree-area-by-area-and-file-by-file)
  - [Events, messages, Lua, input, the main loop and the attribute system (`src/Speed/Indep/Src/{Generated,Interfaces,Input,Main,Debug,Lua,Gamemodes}/`, `src/Speed/Indep/Tools/`)](#events-messages-lua-input-the-main-loop-and-the-attribute-system-srcspeedindepsrcgeneratedinterfacesinputmaindebugluagamemodes-srcspeedindeptools)
  - [Frontend UI and FEng (`src/Speed/Indep/Src/Frontend/`, `src/Speed/Indep/Src/FEng/`)](#frontend-ui-and-feng-srcspeedindepsrcfrontend-srcspeedindepsrcfeng)
  - [Sound, speech and the EA audio middleware](#sound-speech-and-the-ea-audio-middleware)
  - [Game logic: AI, physics, simulation, gameplay, camera and animation](#game-logic-ai-physics-simulation-gameplay-camera-and-animation)
    - [AI (`src/Speed/Indep/Src/AI/`)](#ai-srcspeedindepsrcai)
    - [Physics (`src/Speed/Indep/Src/Physics/`)](#physics-srcspeedindepsrcphysics)
    - [Simulation (`src/Speed/Indep/Src/Sim/`)](#simulation-srcspeedindepsrcsim)
    - [Gameplay (`src/Speed/Indep/Src/Gameplay/`)](#gameplay-srcspeedindepsrcgameplay)
    - [Camera (`src/Speed/Indep/Src/Camera/`)](#camera-srcspeedindepsrccamera)
    - [Animation (`src/Speed/Indep/Src/Animation/`)](#animation-srcspeedindepsrcanimation)
  - [World, renderer, animation, misc and online](#world-renderer-animation-misc-and-online)
    - [World: collision, grid and road network (`src/Speed/Indep/Src/World/Common/`, `World/W*.h`)](#world-collision-grid-and-road-network-srcspeedindepsrcworldcommon-worldwh)
    - [World: cars, streaming, scenery and effects (`src/Speed/Indep/Src/World/`)](#world-cars-streaming-scenery-and-effects-srcspeedindepsrcworld)
    - [Ecstasy renderer core (`src/Speed/Indep/Src/Ecstasy/`)](#ecstasy-renderer-core-srcspeedindepsrcecstasy)
    - [Render connections (`src/Speed/Indep/Src/Render/`)](#render-connections-srcspeedindepsrcrender)
    - [EAGL4Anim animation runtime (`src/Speed/Indep/Src/EAGL4Anim/`)](#eagl4anim-animation-runtime-srcspeedindepsrceagl4anim)
    - [Misc (`src/Speed/Indep/Src/Misc/`)](#misc-srcspeedindepsrcmisc)
    - [Online (`src/Speed/Indep/Src/Online/`)](#online-srcspeedindepsrconline)
  - [Platform code: GameCube (`src/Speed/GameCube/`)](#platform-code-gamecube-srcspeedgamecube)
  - [Platform code: PS2, Xbox 360 and PC (`src/Speed/PSX2/`, `Xenon/`, `PC/`)](#platform-code-ps2-xbox-360-and-pc-srcspeedpsx2-xenon-pc)
  - [bWare (`src/Speed/Indep/bWare/`)](#bware-srcspeedindepbware)
  - [Support library (`src/Speed/Indep/Libs/Support/`)](#support-library-srcspeedindeplibssupport)
  - [Library compilers and flags (`configure.py`)](#library-compilers-and-flags-configurepy)
  - [RealCore (`src/Speed/Indep/Libs/realcore/`)](#realcore-srcspeedindeplibsrealcore)
  - [Middleware packages (`src/Packages/`)](#middleware-packages-srcpackages)
  - [rcmp movie player (`src/egami/`)](#rcmp-movie-player-srcegami)
  - [SN ProDG runtime (`src/LibSN/`)](#sn-prodg-runtime-srclibsn)
  - [C runtime: libc, libm, libgcc2 (`src/libc/`, `src/Runtime.PPCEABI.H/`, root `src/*.c`)](#c-runtime-libc-libm-libgcc2-srclibc-srcruntimeppceabih-root-srcc)
  - [RealShape and RealFont (root `src/*.cpp`, `src/realshape.hpp`)](#realshape-and-realfont-root-srccpp-srcrealshapehpp)
  - [SourceLists (`src/Speed/Indep/SourceLists/`)](#sourcelists-srcspeedindepsourcelists)
- [Open fronts (diagnosed, not done)](#open-fronts-diagnosed-not-done)
  - [PS2 prototype (A124): classes whose layout still differs](#ps2-prototype-a124-classes-whose-layout-still-differs)
  - [Leftovers that break the ground rules: removed](#leftovers-that-break-the-ground-rules-removed)
  - [FPU asm: removed, proven to be EA's, restored (2026-09-23)](#fpu-asm-removed-proven-to-be-eas-restored-2026-09-23)
  - [Other builds](#other-builds)

---

# Knowledge: oracles, compiler behaviour, traps and method

Everything below was measured on this project (GameCube `GOWE69` unless stated), with the figures
recorded when the finding was made. Paths are relative to the repository root; tools live in
`scripts/` and `tools/`. Where a finding was later corrected, only the correction is given.

## Data sources in the original binaries (the oracles)

The rule: nothing the binary can tell you is guessed.

| Version | File | What it carries | Readers |
|---|---|---|---|
| GameCube `GOWE69` (final) | `orig/GOWE69/NFSMWRELEASE.ELF` | full symtab, 577 `STT_FILE`, DWARF-1 (`.debug`, `.line`, `.debug_aranges`, `.debug_pubnames`, `.debug_srcinfo`, `.debug_sfnames`) | `dwarf1.py`, `orac.py`, `plan.py`, `fuse.py`, `lmap.py`, `aranges.py`, `pubnames.py`, `rangechk.py`, `dwarfmap.py` |
| PS2 `SLES-53558-A124` (alpha 124, milestone) | `orig/SLES-53558-A124/NFS.ELF`, `NFS.MAP` | ECOFF `.mdebug` with stabs; linker map | `mdebug.py`, `sondastabs.py`, `cotejo.py`, `ps2map.py`, `ps2fn.py` |
| PS2 `SLUS-21351` (retail) | `orig/SLUS-21351/SLUS_213.51.ELF` | stripped | symbol presence, diff against A124 |
| Xbox 360 `EUROPEGERMILESTONE` (21 Oct 2005) | `orig/EUROPEGERMILESTONE/NFS.exe`, `.xex` | stripped PPC PE | `tools/nombra_por_codigo.py`, `x360ref.py` |
| PC `SPEED_EXE_1_3` | `orig/SPEED_EXE_1_3/speed.exe` | nothing; names from an IDA export | `config/SPEED_EXE_1_3/symbols.json` |
| Xbox A138 (8 Oct 2005, x86) | disc image, not in the repo | assert `__FILE__` strings | a name list in the fork's unpublished notes |

### GameCube DWARF-1 (`.debug`, 92 MB, 316 compile units)

- Producer `GNU C++ 2.95.3 SN BUILD v1.76 for Nintendo Gamecube` (288 units; 15 say v1.83, 13 v1.72);
  each unit carries EA's path (`D:/mw/Speed/...`).
- Per function: parameters and locals with name, type and **register or stack slot**
  (`float vert_comp; // f30`), lexical blocks with address ranges, and the **inline expansion tree with
  address ranges**. Totals: 643,106 inline expansions (nesting up to 9), 19,677 anonymous blocks,
  521,509 local declarations (40,256 with a register). Per type: members with offsets, enum values.
- Readers: `scripts/dwarf1.py fn <name|address>` parses `.debug` (format checked against `dwarfout.c`);
  `scripts/orac.py` greps the rendered dump `symbols/mw_dwarfdump.nothpp` (`dtk dwarf dump`, 105 MB)
  in ~1 s; `scripts/plan.py <unit> "Class::Method"` interleaves target asm, source line, inline/block
  open-close marks and each local's register, which turns writing into transcription;
  `scripts/dwbody.py` diffs locals, blocks and inlines against ours; `scripts/missinline.py` ranks
  inlines the original expands and we lack; `tools/dwarf-compare.py` scores DWARF similarity per
  function (in uiWorldMap only 10 of 36 functions were 100 % on DWARF, almost all 100 % on code).
- Reading rules that paid: declare the listed locals in the listed order; the expansion count is
  evidence (one `Min` listed = one call); locals at function scope instead of nested mean early
  returns; an accessor missing from the tree means the member was touched directly.
- What it does not say:
  1. A method without a body has no DIE (`output_decl` skips `DECL_INITIAL == NULL`): pure and
     declared-only methods never appear, and neither does vtable order.
  2. Enumerators print signed (`0xFFFFFFFF` as `-1`); the sign comes from `cmplwi` in the code. There
     are no namespaces: same-named types from different namespaces look identical.
  3. Compiler temporaries are not listed; a local the DWARF omits can still be required (three
     measured cases); a missing `// Labels` entry means unnamed, not absent.
  4. `bool` is annotated as size 1 but occupies 4 bytes (finding 47).
  5. 59 functions fail to render in the dump (`Failed to process tag`): absent from the dump is not
     absent from the game. Weak copies stripped by the linker are listed with `Range: 0xFFFFFFFF`.
  6. When dump and mangled symbol disagree on a signature (`int`/`long`), the symbol wins. SN's
     precompiled libc and the Metrowerks units have no DWARF.
  7. Array dimensions print reversed: Dolphin's `Mtx` (`[3][4]`) shows as `[4][3]` and mpg123's
     `slen[2][16]` as `[16][2]`. `ePoly::UVs` read from the dump as `[2][4]` was `[4][2]` on every
     platform; take dimensions from the PS2 stabs or from the code.
- Type order inside a unit is an oracle too: classes with inline bodies first (definition order), each
  followed by the new types its fields use, then everything unreferenced in declaration order. A
  body-less type appears in every unit that includes its header: the same unit set as an inline whose
  header the line map names means the same header. Compare with `dtk dwarf dump` of our object.

### GameCube `.debug_aranges`: unit boundaries (`scripts/aranges.py`)

- Per unit (`dwarfout.c:7344-7385`): DIE offset, an (address, size) pair for every public tentative
  (COMMON) variable, then four pairs for text, data, rodata and bss. 316 units: the compiler's own
  `splits.txt`. Mandatory control: 299 of 300 `.text` starts equal `splits.txt` to the byte.
- It added 62 `.rodata`, 42 `.data` and 20 `.bss` boundaries, and 25 of the 71 `auto_*` filler ranges
  start where a unit starts. No entries exist for `.sdata/.sdata2/.sbss/.ctors/.init`. Sizes are object
  sizes (the link strips): take unit ends from `AT_low_pc/AT_high_pc` of the `TAG_compile_unit` DIE
  (0 overlaps instead of 140). A known boundary is not yet a usable one (finding 44).

### GameCube `.debug_pubnames` (`scripts/pubnames.py`)

- Maps 21,431 public addresses (93.1 % of `.text`) to their unit and yielded 344 real names missing
  from `config/GOWE69/symbols.txt`; each can turn a 0 % into a match.
- The string is `DECL_NAME` (undecorated) and 21 % of DIE offsets are skewed (`next_pubname_number`
  not incremented): join by address, never by name.

### GameCube line map (`symbols/debug_lines.txt`, 525,964 entries)

- `address -> EA file:line`, extracted from `.line` by `tools/dwarf1_gcc_line_info.py`; read with
  `tools/line_lookup.py`, `scripts/lmap.py`, `scripts/fuse.py`, `scripts/plan.py`.
- It gives statement boundaries (instructions on line N vs N+1 separate `p[n].f` from
  `q = &p[n]; q->f`), missing statements (line gaps between anchors), the header holding an inline
  body, a unit's real file order, and the signature of an inline written by hand (GCC attributes an
  inline's parameter set-up to the parameter's declaration line in the callee). It tells C from
  original asm: asm shows 2-4 lines over hundreds of bytes, C dozens.
- Our objects are built with `-gdwarf+` and carry `.line` too. Crossing both gives statement order,
  which the DWARF lacks: it rejected a "100 %" with an extra `return` per branch (the original has no
  epilogue note) and found two swapped statement pairs. Align `.line` and `nm` offsets by best match.
- Note rules (`cp/semantics.c`, `final.c`): a note for each `{`, each `if` twice (keyword and closing
  `)`), `return`, `break`, expressions and initialized declarations; none for `case` or `}`; `final.c`
  deletes a note followed by another when no line in between has one; the closing-brace note exists
  only if the function has a `return`. A `#line N "F.cpp"` bench compares variants note by note in ~1 s.
- Caveats: the old reader dropped the tail notes of the last file segment in 267 units (21,119 notes;
  reader fixed, file not regenerated yet); 111,977 lines carry 9-digit addresses with a delta of
  exactly 0xFFFFFFFF, a sentinel (probably stripped code), not an address.

### GameCube symbol table

- 577 `STT_FILE` symbols, each followed by its file's locals: the real attribution of statics, strings
  and literals (`scripts/rangechk.py` checks `splits.txt`; `scripts/dwarfmap.py` attributes globals
  from the DWARF). Their names are the SourceLists (`zGameplay.cpp`, `zLua.cpp`): the original was
  built as unity SourceLists, as PS2's map also shows.
- Caveats: globals follow all locals and inherit the last `STT_FILE`; a `STT_FILE` can be an amalgam
  (`allsrc.c` is `steering.c`). Local statics carry GCC's counter (`statInfo.695`, finding 33).

### PS2 A124 `.mdebug` (stabs)

- 84.5 MB of ECOFF debug. The data is in the SYMR table (1,896,029 entries), not in the file
  descriptors: only 4 of 559 FDRs carry line numbers (hand asm), so PS2 has no line map.
- Per function (`scripts/mdebug.py fn <mangled>`): parameters (`this:P7566`), locals with type and
  register (`distance:r14` = `$f14`), `$LBB/$LBE` ranges of blocks and inlines, and the `FILE` +
  `$LMnnnn` sequence giving inline expansion order. It covers middleware built without `-g` on GC.
- Struct layouts: every class with offset and size of each field in bits, and its qualified name in
  the signature of the implicit `operator=` (`RCQ29ESimulate10StaticData`); type numbers are per unit.
  `scripts/sondastabs.py` probes every class from the SourceList that compiles it, with the version's
  real flags (2,520 match, 13 do not); `scripts/cotejo.py <Class>` sets A124 fields beside ours.
- Transfer rule: GameCube is `-O1`, PS2 `-O2`. Locals, inlines, order and layout transfer; codegen does
  not. Where both agree, the source form is confirmed by two compilers.
- The stabs do not record `volatile`: the A124 ELF has no `B` type descriptor at all, and a variable that
  is volatile in the GameCube DWARF (`FrameCounter`) appears as plain `int32`. On PS2 the proof of a
  volatile is the binary: a re-read whose value is not used, two loads in a row, or data an interrupt
  handler touches.
- Caveats: an alpha, so late members and statements are missing (the `EA_BUILD_A124` guards); covers
  `speed/indep`, not `GameCube/Src`. The `.mdebug` names the toolchains (game `GNU as v2.9-ee-991111b`,
  middleware SN packages per file): read it before hunting for compilers.

### PS2 A124 `NFS.MAP` and type dump

- `NFS.MAP` (29,142 symbols): address, size and owning object (`...\sourcelists\zai.cpp.obj`,
  `librealshapez.a(shpcreate.cpp)`). It gives that build's emission order
  (`scripts/ps2map.py`), where a vtable or destructor is defined (proof of a `#pragma implementation`),
  library membership (six "sourceless" objects were RealShape and RealFont) and independent order
  evidence for doubtful names.
- `symbols/PS2/PS2_types.nothpp` (stdump of `NFS.ELF`): types in declaration order, 7,506
  `/* vtable[N] */` slot annotations (GameCube's DWARF has none), typedefs GameCube loses (`Key`) and
  parameter names. Base offsets are in bits, members in bytes; prefer the stabs for layout.

### Stripped builds and names

- SLUS-21351 (retail): diffs against A124 show where the alpha is not representative; symbol presence
  classifies builds (only SLES has the milestone allocator `bMalloc__FiPCcii`).
- Xbox 360: `symbols.txt` named 4,709 of 57,465 functions. `tools/nombra_por_codigo.py` names a
  function whose bytes equal one of our MSVC-compiled object (unique both ways, in anchor order or
  confirmed by a named relocation target): +580 names (1.33 -> 1.85 %); on PC +1,909 (6.75 -> 9.16 %).
  Rerun after each batch. `scripts/x360ref.py` finds a function's X360 twin through its string
  literals: constants, conditions and statement order from a second compiler.
- PC names come from an IDA export; only MSVC-mangled names pair (upstream's list gave +716). The
  x86 Xbox A138 is useless for codegen, but its assert strings give 113 source paths and 149 exact
  `Class::Method` names. X360 and PC compile the same source with MSVC, so a GCC-only construct
  (`case a ... b`) cannot be EA's source for code those builds contain.
- Hash constants: `FEHash` (`src/Speed/Indep/Src/FEng/FEList.cpp`: `h = 0xFFFFFFFF`, then
  `h += h << 5; h += c` per character) and `bStringHash` (`src/Speed/Indep/bWare/Src/Strings.cpp`) are
  in the tree, so a proposed name is checkable: of 152 in upstream's Carbon-derived `ScriptHashes.hpp`,
  143 verified, 6 with spaces for underscores, 9 did not (dropped). Never edit hash-table files.

### The compiler's own source, and Ghidra (`orig/prodg/NGC_GNU_SRC/NGC/gcc/`)

- SN's GCC 2.95.3: read behaviour instead of inferring it (`dwarfout.c`, `cp/decl2.c`, `cp/pt.c`,
  `varasm.c`, `cse.c`, `haifa-sched.c`, `local-alloc.c`, `global.c`, `jump.c`, `alias.c`,
  `config/rs6000/rs6000.h`). SN's `cc1` has options missing from `--help` (its `target_switches`
  table: `safe-sda`, `sndata=`, `fast-cast`, `align-section-names`).
- RTL dumps: the `ngccc` driver swallows `-d`. Preprocess, then run
  `build/compilers/ProDG/3.9.3/cc1plus.exe` with the unit's cflags plus `-dg -dl` (`.greg`: allocno
  order and final `reg_renumber`; `.lreg`: local_alloc) or `-dR -fsched-verbose-5`; ~22 s per SourceList.
- Micro-bench: `ngccc -S` on a one-line `.cpp` that includes the file, with the unit's flags and
  `SN_NGC_PATH` set, reproduces a function in 1-4 s (`scripts/bench.py` for SourceLists).
- `tools/ghidra_decomp.py` decompiles the GameCube ELF headless after two patches, both in `tools/ghidra/`
  with their steps: DWARF-1 support for GCC's vendor attributes (86 -> 13,824 signatures) and the
  Gekko/Broadway processor (the default PowerPC target cuts each function at its first paired single). Its C normalizes semantics (seven `&&` for one `or.`):
  compare forms in the asm, and check member names against offsets.

## Compiler and linker knowledge

Target unless stated: SN ProDG 3.9.3 (`ngccc`, GCC 2.95.3 PowerPC), `-O1` with scheduling and GCSE,
`-gdwarf+`, linked by `ngcld -strip-unused-data`.

### Control flow

1. **`switch` or `if` chain: read the first compare.** GCC 2.95 always canonicalizes `x >= C` to
   `x > C-1` and `x < C` to `x <= C-1`; only `expand_case` emits the bound as written, so
   `cmpwi r,0x4f; blt` comes from a `switch`. Rewriting the chain as a `switch` reproduces the whole
   tree, jump targets included. The index must be unsigned or the tree's `cmplwi` is missing.
2. **The switch tree is reproducible.** Pivot `i = (n_cases + n_ranges + 1) / 2`. Adjacent cases
   sharing a label merge before `balance_case_nodes`; an empty `case X: break;` moves the pivot only if
   its label differs, even when its compare vanishes (value above the maximum). Same value set with a
   different tree means the original had an empty case. Bodies come out in source order; `default:`
   first with `case 0:` attached roots the tree at node 0.
3. **`||` versus separate `if`s.** Each `if (X) return false;` emits its own `li r3,0`;
   `if (A || B || C) return false;` emits one. Symptom: 4 bytes too many and one inserted `li`
   (`ShouldDoSimplePhysics` 98.36 -> 100 % with one line). `a || b` gives `or.` only with both operands
   already 0/1: write `|`.
4. **Defensive code the original lacked.** N extra instructions and zero missing is not scheduling: it
   is a check GCC cannot prove away across inlines, e.g. `if (p == 0) p = "";` after an accessor that
   already returns `""`. Fourteen such sites were worth 2,456 B.
5. **The branch you write as the body decides block layout.** `if (A) { X; return; } Y;` emits Y as
   the fall-through and X at the end; to get X inline, write the other branch as the body. Several
   `if` blocks that all jump to the function tail were one `else if` chain.
6. **Three loop shapes.** `while`/`for` jump to a bottom test; a test at top and bottom with no initial
   `b` is a hand-written `if (c) { do { ... } while (c); }`. A backward `goto` is not a loop to
   `loop_optimize` (no `NOTE_INSN_LOOP_BEG`), so invariants stay inside. `lwz; addi -1; stw; lwz;
   cmpwi -1` is `while (x--)` on a volatile.
7. **Cross-jumping.** For identical `case` bodies the merge direction follows source distance
   (adjacent: the earlier leaf jumps forward; separated: the later jumps back), not label order. For
   duplicated tails outside a `switch`, how each block ends decides whether a copy merges, and the
   position of the shared label decides the direction.

### Expressions and folding

8. **Canonicalizations make some sweeps pointless.** Float `X > C` and `C < X`, pointer `i + base` and
   `base + i`, `i <= 6` and `i < 7` give identical objects. `|` chains are not canonicalized: operand
   order decides the tree and the accumulating register (`UnlockPalette` 92.7 -> 99.4 % by sweeping 24
   permutations). `(a + C) + b` is not reassociated either: source order decides who carries the `C`.
9. **`-ffast-math` reassociates constants.** `a * C1 * f(b)` with `f(b) = b*C2` becomes
   `a * ((b*C2)*C1)`; only a named temporary holding `a*C1` blocks it. If a DWARF-faithful form matches
   only without `-ffast-math`, check whether the unit belongs to a group with other flags (snd links in
   two blocks, one of them without it).
10. **A `lis sym@ha` hoisted above a `bl` means a conversion.** `expand_assignment` evaluates a bare
    `CALL_EXPR` before the left-hand side, so the `lis` comes after the call; an implicit conversion
    wraps the call in `NOP_EXPR` and the `lis` comes first, in a saved register. Check the declared
    type of the `extern` (`DataLoadCB` matched with `int` for `unsigned int`, as the DWARF says).
11. **Address forms.** `lis; lwz sym@l(r9)` is direct access; `lis; addi sym@l; lwz 0(r9)` means the
    source materializes the address (`&` or a pointer local). A target that puts the base first in an
    `add` or indexed store indexes a member array: declare the array inside the struct.
12. **Opcodes reveal types.** Three `lwz/stw` are a same-type struct assignment; three `lfs/stfs` a
    member-wise constructor/`operator=`, i.e. a temporary in the source. Branch-free `xor; subfic 0;
    adde` is integer `==`. `crclr cr1eq` before a call means no prototype in that file, its absence a
    prototype (a new `#include <cstring>` removed the one before `_bOutput`'s `memset`).
13. **Inline nesting.** In-class bodies compile in declaration order: an inline calling one declared
    later in the class emits `bl`. GCC 2.9 does not inline at a third level (`ctorA -> ctorB -> Clear()`
    calls `Clear`). `char x[2]` is HImode and gets a stack slot only when its address is taken.

### Scheduling and register allocation

14. **Store runs are rotated, and the rotation inverts.** For independent member stores the scheduler
    emits the last statement's store first, `E = [S(n), S(1) ... S(n-1)]` (rotations of 1 and 2 seen).
    Derotating the target predicts source order without compiling (all 13 store relocations of
    `WRoadNetwork::Init` at once). Stores to one base reorder; to different bases they do not.
15. **`rank_for_schedule` has five levels; level 2 (`INSN_REG_WEIGHT`) acts only in `sched1`.** So
    `sched1` and `sched2` can order a pair oppositely, and `sched2` sees what `sched1` left, not the
    source. Ties go to the insn with more dependents before LUID; PRE insertions sit before the jump
    with the highest LUID, so a tie against them is a ceiling. `regmove` can steal a `REG_DEAD`: read
    the `.combine`/`.regmove`/`.sched`/`.sched2` dumps instead of sweeping source forms.
16. **Live lengths are computed after `sched1`.** `REG_LIVE_LENGTH` is recomputed after `sched1` and
    `local_alloc` runs after it, so ranges read off the final `.s` mislead. Moving independent
    statements within one basic block changes no live length (seven forms identical).
17. **Allocation order is computable.** `allocno_compare` orders by
    `floor_log2(n_refs) * n_refs * size / live_length` (ties by creation order); the steps at 8, 16 and
    32 references are real (a dead reassignment dropping a local from 16 to 14 refs cost it r30).
    Callee-saved registers go in descending order (r31..r14, f31..f14), and `find_reg` distributes by
    `local_reg_n_refs / local_reg_live_length`. Loop depth multiplies `n_refs`: `do { } while (0)`
    macros are not neutral, newlib's bare `{ }` blocks are.
18. **`local_alloc` gives r30, `global_alloc` gives r31.** A single-basic-block function never uses
    r31: if the target saves one register with `stw r31` and we save two with `stmw r30`, the source
    lacks a basic block, not a local. `stmw rN` starts at the lowest saved register used, not at the
    number of live values: count registers and holes. A frame 8 B too large and a register mismatch
    are often one problem; compute `vars = frame - 8 - 4*nGPR - 8*nFPR`.
19. **FPR outputs of an inline asm are assigned deterministically.** `REG_ALLOC_ORDER`
    (`rs6000.h:932`) starts f0, f13, f12...: with N outputs the last takes f0 and the others go down
    from f13; the return value is always operand 0 and takes f1; with an `"f"` input in f1 the outputs
    need `"=&f"` or the argument is overwritten. EA's original paired-single code thus needs one asm
    with `"=f"` operands and no pins (103 pins -> 0, 41 of 41 functions identical).
20. **Register permutations are usually a symptom.** 36 register-only rows of
    `AIPursuit::UpdateFormation` vanished after three structural fixes; of 371 near-misses only 4 %
    were allocator ceilings. Levers without asm: one more reference (`if (A) x = K; else if (C) x = K;`
    for `if (A && B) x = K;`, 62 rows -> 0); one variable reassigned where the target chains values
    through one register; `float` instead of `const float` for constant locals (a const one is
    propagated to its use); splitting a declaration from its assignment.
21. **An empty loop is a `sched2` barrier in legal C.** `do { } while (0);` leaves
    `NOTE_INSN_LOOP_BEG/END`, which `sched_analyze_insn` treats like an `ASM_INPUT`. It replaces a pure
    ordering barrier (byte-identical), not one that hides a value from CSE, and it raises `n_refs`
    through loop depth.
22. **An aggregate view breaks an aliasing conflict.** `alias.c` (`fixed_scalar_and_varying_struct_p`)
    assumes a fixed-address scalar and a varying-address struct field do not alias. Declaring the same
    symbol as an aggregate (`extern unsigned int milliseconds_agg[];`) sets `MEM_IN_STRUCT_P` and
    changes the order at zero instructions (`Libs/path`: 14 scaffolds -> 4), if the declaration is
    plausible EA code.
23. **What `asm` does in GCC 2.95** (diagnostic knowledge; forbidden in our committed code). Every asm
    without outputs is marked volatile, a full barrier, even `asm("" : : "r"(x))`; a selective one needs
    a live output tied with `"0"`. A non-volatile asm on a long-lived value adds one reference and one
    unit of live length at zero bytes. `register T x asm("rN")` is the only construct that reaches
    `local_alloc`, is silently ignored if `&x` is taken, and is a hard error in EE-GCC and MSVC. Ask the
    DWARF first: 11 of 15 pins in matched units propped up a local the original never had (9) or
    duplicated a parameter (2).

### Locals, blocks and frames

24. **Rewrite against the DWARF; don't patch one local at a time.** A local we have and the DWARF lacks
    is a hint: removing single locals made seven functions worse, while rewriting
    `FindConditionBranchTarget` against the dump (two locals, no label, no block) went 65 -> 100 % at
    the first compile. Adding missing DWARF locals in DWARF order usually helps
    (`TrackCopCameraMover::Update` 95.2 -> 96.9 %). Stack slots follow first use; declared locals never
    reuse temporary slots; a lexical block of dead code still reserves frame.
25. **Empty lexical blocks.** The original has 90 blocks without instructions. Extra braces emit no
    block; only a type tag inside (`{ enum { _tag }; ... }`) emits one, at zero instructions.

### Inline functions, templates and emission order

26. **In-class body = inline = no symbol.** A body inside the class is expanded in callers and emits no
    symbol; outside the class it emits a weak symbol and callers keep `bl` (moving template bodies
    inside cost 18 functions). Exceptions: a virtual with an in-class body is emitted (the vtable needs
    it), and a polymorphic class whose key method is defined in the TU is exported with all members out
    of line. C twin: `static` is not inlined at `-O2`, `static inline` is (`qsort` 37 -> 100 %).
27. **The key method gates the vtable.** `_vt.` and the out-of-line copies of inline virtuals are
    emitted only in the TU defining the first non-inline virtual: write it first when opening a class
    (+38 functions in `PVehicle`). A virtual declared non-pure and defined nowhere leaves the class
    without a key method, so no TU emits the vtable and the link fails on `_vt.*`. The original had it
    pure: its slot in the extracted object relocates to `__pure_virtual` (`scripts/ghostref.py`).
28. **Missing template instances usually mean a missing caller.** 271 of 453 `UTL::Vector` symbols were
    MISSING while every emitted instance matched; they appear when callers are written. Hundreds of
    zeros usually hang from one unwritten mother function (`BindMessagesToLua`, 7,700 B, brought 211
    functions at the first compile). An undefined template static member switches off its whole
    instantiation, but it pays only for container types with a vtable, needs `= T()` (without it the
    definition is COMMON and emits nothing) and is per unit.
29. **Emission order is decided by the point of use.** `finish_file` (`cp/decl2.c`) drains separate
    queues in passes: template bodies from `pending_templates` (queued once, at first request), vtables
    from `walk_globals`. An inline template is instantiated at the parse point only if its first use is
    inside a function (`in_function_p()`, `cp/pt.c`), otherwise at the end. Ordinary functions follow
    definition order, templates and inlines parse order, so `inline` is a position: one keyword on an
    out-of-class definition placed 68 functions. Parse order predicts `.text` order far better than the
    call graph (Kendall tau +0.95 vs +0.42; 64 % of zPhysics's functions call nothing).
30. **Vtables come out in reverse order of class completion** (`walk_globals` walks a list built by
    prepending). A class completes where its header is first parsed; its bodies come out where its
    `.cpp` is: separate the two triggers. Check with `-E` in seconds (inverted vtable order must equal
    class order: 30 of 30 in zEagl4Anim, whose 26 misplaced vtables went to 0). Includes used only for
    pointers or references move classes.
31. **`#pragma interface`/`implementation`.** The pair emits all the header's vtables in the first
    `finish_file` pass (before static tables), plus vtables the original lacks; `interface` alone leaves
    the vtable undefined. If other units' DWARF lists the class's inline destructor with
    `Range: 0xFFFFFFFF` (stripped weak copies), there was no pragma. Where it is real (vtable and
    destructor global in one unit only, as PS2's map shows for `ITrafficCenter`), it stays.
32. **`DECL_COMDAT` inlines come out one pass late.** `wrapup_global_declarations` waits for
    `TREE_SYMBOL_REFERENCED || !DECL_COMDAT`; an in-class inline called only by another inline of its
    class is referenced once that caller is written, so it lands a pass later, thousands of bytes away.
    A class with a known interface (pragma pair scoped to it) goes through `maybe_make_one_only`, which
    does not set `DECL_COMDAT`: its inlines come out in the first pass, in source order.
33. **Function-local classes.** GCC 2.9 emits the vtable of a class declared inside a function but not
    its virtual bodies (they stay undefined: a ceiling, 624 B in zFe2). Their names carry a `.NNNNN`
    suffix from `var_labelno`: +3 per function body parsed before (hidden `__FUNCTION__`,
    `__PRETTY_FUNCTION__`, `__func__`), +1 per local static; types and prototypes do not move it. The
    original's local statics carry the same counter, so comparing suffixes tells whether a header has
    too many or too few bodies. Normalize `name.NNN` when diffing objects.
34. **Names for free.** `_GLOBAL_.I.<X>` takes the name of the first global the unit emits: reorder
    definitions, not the initializer (44 B in zLua). An explicit instantiation (`template class
    ListNode<Foo>;`) emits `.globl` instead of `.weak` in the same `finish_file` slot, moving no byte:
    the defence against finding 38 (226,583 B in zPhysics).
34a. **A synthesized `operator=` is inlined only below a size threshold.** cc1plus 2.95 builds it without
    `DECL_INLINE` and expands it only while the function stays under `3 * max_insns` uids (240). Line notes
    count, so a header that spreads one statement over four lines can push it over: `elRotateLightContext`'s
    `*dst = *src;` was 244 uids and GameCube kept the call. The GameCube line map puts the four loads of
    `bCopy(bVector4)` on one line (`bMath.hpp:1106`) and the four stores of `bFill` on another (1040, 350 call
    sites, none on 1041-1043); written that way the copy is expanded and the function matches on all five builds.
    When GameCube expands a synthesized method and we do not, count uids before reordering anything.

### Data placement, constant pool and linker

35. **The initializer decides the section.** `int x = 0;` goes to `.data`, `int x;` to COMMON/`.bss`,
    whatever the value (`= {0}` put 26,624 B of zeros in `.data`). An explicit specialization of a
    static data member without initializer is only a declaration.
36. **A real definition steals a COMMON symbol.** If the original object has a symbol as COMMON and ours
    defines it, ours wins (GLOBAL beats COMMON, and beats WEAK even when linked later) and everything
    between the old and new places shifts: `= {}` on two symbols of 640 B moved 916,004 B. Symptom:
    `.data` surplus and `.bss` deficit of equal size. A header that defines globals does this everywhere.
37. **`.bss` order.** `.bss` is emitted at the end of the unit (a file-scope asm lands before all
    variables). Anonymous holes have no source lever, but named C++ symbols (a namespace-scope pointer
    is not tentative in C++) follow the order of their first declaration: an `extern` before the
    `#include` that first declares them fixed `sserver`.
38. **Ownership change.** A symbol our object defines and the original unit's object does not, also
    defined by another object, is taken from the first in link order; everything between moves by its
    size. Invisible to size, order and function diffs; measure (growth of the linked section) - (growth
    of our contribution) with `scripts/movidos.py`. One 120-byte template method poisoned three units.
39. **The constant pool.** Entries are written in creation order (`force_const_mem`: statement order)
    and unreferenced ones dropped (`varasm.c`); the pool is per function (one constant, a different
    label per function) and is dumped just before each function, parse-time strings first. `cc1plus`
    clears its table before the deferred `finish_file` pass, so a literal used by a late template is
    interned twice (the original has `"STL"` twice too). Any new literal, a float constant included,
    renumbers every later `$LC`.
40. **Stripping leaves corpses.** `-strip-unused-data` removes `size & ~(sh_addralign - 1)` of each
    dead symbol, with the input section's alignment (set by its most demanding member): at 8, an
    8-byte literal vanishes, a 4-byte one survives, others leave a 1-7 byte tail (`"ssis"` of
    `"Attrib::Gen::chassis"`); at 4, a dead `int` vanishes. Conversely, a dead string of ours whose tail
    is absent from the original was never emitted there: our unit includes a header the original did
    not (the DWARF shows the type as declaration-only).
41. **The 32-byte window.** The linker script aligns `.data`, `.bss`, `.sdata` and `.sdata2` to 32, so
    `rodata +8` means "8 to 39 bytes too many" and anything in `[-31, 0]` is closed. A `+8` in zPhysics
    shifted three sections and made 273,472 B look different.
42. **The linker strips unreferenced code.** Stripping is per symbol, so extra `.text` (template copies,
    out-of-line accessors) never reaches the DOL: swapping in an object with +18,908 B of `.text` gave
    exactly the original linked `.text` size, and A/B pairs gave the same DOL hash. `-gdwarf+` disables
    `.gnu.linkonce` in ProDG 3.8.1 and later; the original kept one copy per instantiation. What moves
    the DOL is `.rodata`, `.data` and `.sdata`.
43. **`linked` versus `matched`.** `matched` is objdiff's per-function match; `linked` is how much of the
    DOL is built from our objects. A NonMatching unit links its extracted object, so the DOL stays
    identical whatever its source; one byte too many in a linked unit breaks it (8 extra bytes in
    FSasync moved 513,077 B in 1,960 ranges). Diagnose on the linked ELF (`scripts/linkdelta.py`,
    `dolwhere.py`), never on object sizes. Units are independent; only shared headers and ownership
    changes couple them.
44. **A boundary is not a placement.** `auto_*` fillers are placed by address, named units by link
    order: an orphan range can go to its owner only if it lies between the unit's neighbours in link
    order. Degrading a unit to NonMatching is not neutral either: dead data only our object emitted is
    stripped (needs `keep.lst`), a `static` of ours the original exported leaves undefined references,
    and section alignment may differ (`.bss` 32 vs 8: `align:` in `splits.txt`); check all three with
    `scripts/antesdegradar.py`.

### Flags and compilers

45. **The ProDG version is not a lever.** `madidct` built with ProDG 3.9.3, 3.8.1 and 3.7 gives the same
    SHA-1 of `.text` and `.data`; codegen is frozen from v1.46 to v1.76. A different compiler is:
    `__va_arg` (Metrowerks `Runtime.PPCEABI.H`), cut by the splitter into a GCC object, sat at 58 % for
    eight rounds until it got its own object and `mwcceppc`.
46. **Flags per library, read from the target.** Written code with a low score suspects flags or
    compiler before source, tested in both directions. `-msafe-sda` iff the target loads its pool with
    `@sda21` rather than `@ha/@l` (per object, not per library); `-fno-builtin` where the original calls
    `fabs`; `-G<n>` works, but `T x[] = {...}` without a size is never small data. A missing flag looks
    like scaffolding (16 asm in `sf_log10.c` were `-msafe-sda`). Toggling one pass flag (`-fno-gcse`) on
    the real file names the guilty pass. `-MMD` omits `<...>` headers; build with `-MD`. Known libraries
    match when transcribed: newlib libm (29 units), SN stdio = newlib 1.8 with three edits (13 of 13).
47. **`bool` is pointer-sized.** SN's `cc1plus` defines `BOOL_TYPE_SIZE` as `POINTER_SIZE` unless
    `-fbyte-bool` (`cp/decl.c:189-191`): consecutive `bool` members sit 4 B apart on GameCube and in the
    A124 layouts, although DWARF and stabs annotate size 1.

### Metrowerks (`mwcceppc`) units

48. **The GCC catalogue does not apply** (`steering`, `OdemuExi2`, `Packages`). MWCC folds away
    `register`, split assignment chains, declaration order inside a block or macro and redundant
    temporaries. It is moved by function-level declaration order, assignment order inside a branch,
    expression and control-flow shape and local types. Acting pragmas: `scheduling`, `peephole`,
    `optimize_for_size`, `use_lmw_stmw`, `opt_common_subs`; six more are accepted and inert (an unknown
    pragma does warn). GCC register pins are syntax errors there.

### PS2 (EE-GCC)

49. **Alignment is per member, not per type.** The A124 aligns many members to 16 with padding before
    them; aligning `UMath::Vector4` as a type broke more than it fixed (the original copies it unaligned
    with `ldl/ldr`). Size is proof: `sizeof` rounds up to the alignment, so a 0x5c class holding a
    `Matrix4` shows `Matrix4` is not type-aligned. Align the member (`ALIGNVEC` if X360 also aligns it,
    `PS2ALIGN16` if not), non-vectors included (`SimSystem::mEventData`, two `mState` pointers, 18
    `Pkt_*` classes). Struct locals are unaffected: every BLKmode local gets `BIGGEST_ALIGNMENT`, 16 on
    the R5900.
50. **Ask the compiler for the layout.** Compile a probe with the version's real flags that stores
    `sizeof`, `__alignof__` and base/member offsets in `int` globals, and read them from the `.s`
    (`scripts/sonda.py`); compare with the stabs. A missing 16-byte alignment on `UTL::COM::Object` was
    worth +71/+80 functions (SLES/SLUS). Heuristics mislead: of 71 members flagged by the alignment
    scanner, 14 were real.
51. **Members added after the prototype.** One member the A124 lacks shifts the whole class by 4 B and
    breaks every function touching it (163 near-misses in zFe). Guard it with `#ifndef EA_BUILD_A124`,
    give its initializer the same guard, and turn each use into an early exit so the A124 falls through
    to the prototype's path; never duplicate bodies (+119/+89). Generated attribute layouts differ per
    attribute: the A124 reads some arrays by key (`GetAttributePointer(hash, i)`), not from the layout
    (`scripts/attrfuera.py`).
52. **VU0 field masks.** Inline VU0 asm without a mask assembles as `.xyzw`; the original writes `.xyz`,
    `.x`, `.xz`. Take the mask from the diff: a table of our opcode -> original opcode over
    `DIFF_OP_MISMATCH` rows is 99-100 % consistent (`vmul.xyzw -> vmul.xyz` 238 times,
    `vmulax.xyzw -> vmulax.x` 179). One mnemonic takes different masks per routine (dot product `.x`,
    matrix times vector `.xyz`), and `VU0_v4scale`'s `.xyzw` is right. It is EA's own asm, so correcting
    it is legitimate (+112/+104, then +36).
53. **Two EE compilers.** The game was built with `ee-gcc 2.9-ee-991111`; the middleware with SN EE-GCC
    from `PlayStation2SN` packages, which saves s-registers with `sq` (128-bit) where 991111 uses `sd`;
    SN `ee-gcc 2.95.3-136` reproduces it (the only build saving `ra` with `sd`). There `long long` is
    128-bit (`-frelax-128` for its two users), while `long` is 64-bit on the EE. The EE driver
    preprocesses to a temporary `.ii`, so `#pragma implementation` never matches the file name:
    `tools/ee_gcc_pp.py` compiles in two steps keeping the name.
54. **Platform leaks.** STLport's `_STL` namespace exists only in the GameCube build (the retail PS2 ELF
    has no `_STL`): `-D_STL=` for PS2. Overlay units need the `.text=.over` rename in every platform
    branch of the build (+217/+197 PS2 functions, no source change). The retail build is not a
    milestone build: giving it the prototype's defines cost 268 functions.
55. **Same function, another signature.** objdiff pairs by mangled name, so a parameter added after the
    prototype leaves a function at 0 % whatever its body: the listener name of
    `Sim::Collision::AddListener`, a `bool` in half the frontend, `unsigned long` against
    `unsigned long long`. `scripts/firmas.py` lists functions both sides define under the same
    `Class::method` with different signatures (24.7 kB in the A124). Guard the parameter in the
    declaration, the definition and every argument, as with a member; a `const` method takes
    `#ifndef EA_BUILD_A124 const #endif`; a 64-bit typedef becomes `uint64` (`unsigned long long` on
    GameCube, `unsigned long` on the EE, which is what DWARF and stabs say). Where the parameter carries
    logic the prototype does differently, leave it: a hybrid of two versions is worse than a 0.
56. **Retail PS2 names come from the prototype.** `config/SLUS-21351/symbols.txt` carries the A124's
    mangled names, so a SLUS signature proves nothing: SLUS's `AddListener` reads the listener name
    (`daddu $s2, $a2`) and stores it in a 16-byte node, although its name says otherwise.
    `scripts/argsusados.py` lists the argument registers a PS2 function reads before writing them;
    where SLUS reads one the A124 does not, the parameter exists and the name is wrong (21 corrected,
    12 of them now at 100 %). An unused parameter leaves no trace: without a register difference, keep
    the name. Guards for the prototype stay `EA_BUILD_A124`, never all of PS2, unless SLUS's code proves
    it.
57. **An empty platform branch returns garbage.** `bMin`/`bMax(float)` had an empty PS2 branch: GameCube
    used its `fsel`, and both PS2 builds returned whatever was in `$f0`, so GCC dropped every computation
    that depended on them (the A124 has 1,193 `min.s`/`max.s`). With `-ffast-math`, EE-GCC turns a plain
    conditional into `min.s`/`max.s`; `a > b ? b : a` and `a < b ? b : a` give the original's operand
    order. `scripts/sinreturn.py` compiles the PS2 units with `-Wreturn-type` and lists such sites; most
    are empty accessors the original never expands, the ones that matter are header inlines with a body
    on another platform.
58. **One pair of opcodes in hundreds of functions is one cause.** `scripts/patrones.py` adds up the
    differing rows of every near-miss function of a PS2 build as (original opcode -> ours) pairs. At the
    top were `sd -> sq` and `ld -> lq` (696 and 635 functions): we saved s-registers with 128 bits. Not
    the flags (a small file with the same flags gives `sd`) but the unit: from one function on, EE-GCC
    kept saving with `sq`. That function had two `bIdentity(bMatrix4 *)`, whose MMI seed asm (no inputs,
    not volatile) GCC merged across calls, keeping a 128-bit value in a saved register. Making the seed
    `volatile` was worth +523 (A124) and +510 (SLUS) functions. It is not exactly the original, which
    hoists the seed yet never merges it; our compiler cannot do both.
59. **Forms GameCube cannot tell apart.** GCC 2.95 for PowerPC compiles `if (x >= c) return; y = 1;`
    and `if (x < c) y = 1;` alike, so a 100 % GameCube function does not prove its comparisons; the EE
    emits `c.le.s` for one and `c.lt.s` for the other, and the orientation of an `if` (`beqz` against
    `bnez`) the same way. The prototype says which form EA wrote. Rewrite the common source (no
    `#ifdef`) and demand an identical GameCube object function by function (`scripts/gcmismo.py`). In two
    passes, 120 functions; both raised all four non-GameCube builds, which is the proof. Caveats: at -O1
    GameCube moves the `then` of a last `if (x) return A; return B;` to the end, so early returns are not
    always neutral there; the DWARF's nested blocks and the line map decide.
60. **An empty SourceList can hide code you already have.** The A124's `zOnline` (116 kB) is the online
    subsystem; our SourceList was empty, like upstream's, because GameCube strips all of it at link time,
    while `Src/Online/` already held 8,000 lines nobody compiled. `scripts/ficheros_mdebug.py` lists the
    source files of every A124 unit in link order from the `.mdebug` and flags the ones a SourceList
    lacks: +501 functions. The retail PS2 build has no network objects, so the online headers are
    `EA_BUILD_A124`, not all of PS2 (doing it for PS2 cost SLUS three 100 % functions).
61. **On PS2 the cheapest bytes are names, not code.** objdiff pairs by mangled name, so a target function
    with no name can never match however exact our code is; 6,738 of SLUS's 21,555 functions (31 %, 814 kB)
    had none, and 3,263 of the A124's. `tools/nombra_ps2.py` is the MIPS/ELF twin of
    `tools/nombra_por_codigo.py`: per unit it compares the split original object with ours and names a
    function when the words match whole and uniquely both ways, comparing only the opcode on relocated words
    but demanding that named relocation targets agree, and checking that good proposals grow in order on both
    sides. Two differences from the COFF one: `--aplicar` has to ADD lines, because splat's `func_XXXXXXXX`
    names are not in `symbols.txt`, and a template name over 250 characters breaks the split, so it carries
    `filename:func_XXXXXXXX`. Worth +704 SLUS and +407 A124 in one pass (36.47 -> 39.06 %, 34.95 -> 36.39 %),
    all at 100 % the moment they are named; a second pass after re-splitting found 12 and 5 more. Re-run it
    after every batch. Its relocation check used to take splat's `D_XXXXXXXX` and `jtbl_` labels for real
    names, so it rejected every pair where our object points at the global by its true name; fixed, it named
    228 more SLUS functions, all matching. With it, the call graph and the A124 as a reference (identical code
    inside the same unit), one round took SLUS from 45.31 % to 50.62 % (+2,098 functions) without touching
    code. Two checks keep it honest: a name must sit on a real entry point (one landed on the target of a
    `beq` inside another function and cut it), and unit boundaries in `config.yml` move with the names (46 of
    them had the first functions of a library in the previous unit).
62. **The VU0 in the original is two families; do not mix them.** `UMath` (`UVectorMath.hpp`) loads the
    destination too (three `lqc2`) and masks `.xyz`, to keep its `w`: those are the majority (210 `vadd.xyz`,
    224 `vsub.xyz`, 289 `vmulx.xyz`, 216 dot products, 134 scale-adds, 80 `vopmula`/`vopmsub`) and they were
    already written, correct down to `vsqrt` and `qmtc2.ni`. bWare (`bMath.hpp`, `bVector3`) uses two loads
    and `.xyzw`; that family was missing (bSub 60 sites, bNormalize 67, bAdd 46, bScale 33, bScaleAdd 26).
    Counting `lqc2` (6,966) and calling it all pending work overstates it sixfold: the mask tells you which
    library a block belongs to. Validate a block by compiling a standalone file with `ee-gcc` and diffing the
    disassembly before touching the tree; the `"o"` operand folds the offset (`lqc2 $vf2,16(a0)`) exactly like
    the original, and plain `asm` is not CSEd.
63. **A save convention no compiler in the tree reproduces is a ceiling, not a root cause.** SLUS's top
    near-miss pair was `lq -> ld` / `sq -> sd` in 210 and 187 functions, and 212 of the 225 sit in one unit,
    `zWorld2`. It is not the code: the original saves with `sq` an `s0` that only holds a 32-bit pointer, and
    2.9-991111 cannot do that (measured: it uses `sq` only for the register that really holds a 128-bit value
    and `sd` for the rest of the same function). Of the six EE compilers here, 2.9 saves everything with `sd`,
    2.95.2-273a/-274 and 2.95.3-107/-114 save everything with `sq`, and only 2.95.3-136 gives the original's
    mix (`sq` for s-registers, `sd` for `ra`, with `-frelax-128`) but swaps the frame slots; compiling the
    whole unit with it is far worse (56 of 315 at 100 % against 168 of 408). So those ~400 differences are a
    toolchain ceiling: leave them out of the root-cause tables instead of paying for them again.
64. **`movn` against `movz` is the orientation of the ternary.** On the EE `X != 0 ? -1 : N` compiles to
    `movz` and `X == 0 ? N : -1` to `movn`, whichever way the `if` is written; PowerPC does not distinguish
    them, which is why the form that matches PS2 sits behind an `#ifdef`. Guards written while only measuring
    the A124 say `EA_BUILD_A124`, but SLUS is the same EE-GCC and wants the same form:
    `scripts/guardas_ps2.py` finds the blocks whose two branches are the same code with the comparison
    complemented and the arms swapped and turns the guard into `EA_PLATFORM_PLAYSTATION2`, leaving GameCube on
    the `#else` untouched. Only 3 of 1,212 `EA_BUILD_A124` sites qualified, which is the point: the rest
    separate real code.
65. **The PS2 platform files are empty, and writing them lands first time.** Every file under
    `src/Speed/PSX2/` is zero lines and no SourceList includes them: 545 functions the original has and we
    never compiled, with the biggest hole in `zEcstasy` (187 kB, 13.9 % in SLUS). Four oracles make
    writing them transcription rather than guesswork, and the first five files landed 31 of 37 functions
    at 100 %: `scripts/fn_de_fichero.py` lists the functions the `.mdebug` puts in a source file, in
    emission order, with the size from `NFS.MAP` (address 0xffffffff means declared but never emitted, so
    not work); `scripts/clase_stabs.py` gives a class's size, every field's name and offset and every
    method's mangled arguments, which is what the DWARF gives on GameCube; `scripts/ps2dis.py`
    disassembles by MAP range, because the `$L` labels make objdump cut in the wrong place; and the
    GameCube twin (`src/Speed/GameCube/Src/Ecstasy/eMathE.cpp` and friends) is the C logic, which
    transplants unchanged. Only the VU0/MMI blocks are PS2-specific. Signatures come from the mangled
    name, where `T0` means "the type of argument 0" (`eRotateX__FP8bMatrix4T0Us` is `(bMatrix4*,
    bMatrix4*, bAngle)`); this binutils' c++filt does not demangle GNU v2.
66. **Field types: anchor the numbers, never guess them.** The type number a stab gives a field cannot be
    resolved from the text (it is an ECOFF auxiliary-table index; searching for a `<n>=` definition
    returns FALSE names, because the number is file-local and another class reuses it). But it is
    consistent, so anchoring a handful names them everywhere: the basic ones fall out of comparing the
    dump of `eSolid` with the declaration already in the tree (Version is uint8, Flags uint16, NameHash
    uint32, NumPolys int16, NumBones int8, AABBMinX float), and the rest from `eRenderTarget` and
    `eViewPlatInfo` against the GameCube twin. That is how `eRenderTarget` (45 fields, 2,704 B) was
    written; the size checking out on its own -- 0xA88 of fields rounded to 16 by the aligned member gives
    the 0xA90 the stabs state -- is the proof the layout is right. A class of 1 B with no fields is not a
    mistake: `eStripData` is a view over the strip's quadword block, and its 15 methods come from the same
    stab.
67. **The instruction histogram finds the empty platform branches, and says when there are none left.**
    `bSqrt` had an empty PS2 branch: it returned an uninitialised local, so every square root in bWare
    gave garbage on both PS2 builds and GCC deleted whatever depended on it -- the same trap as
    `bMin`/`bMax` (lesson 57), and invisible in a diff because the function simply is not there. Counting
    mnemonics makes it obvious: the original has 96 `sqrt.s` and we emitted 34, all of them from
    `VU0_sqrt`. The fix is one line of C, `sqrtf(x)`. (An earlier version of this lesson put it in asm
    because `sqrtf` lets the scheduler fill a branch delay slot with the `sqrt.s` and the original does
    not; measured later, that asm bought exactly ONE 12-byte function per PS2 build, so it was forcing
    a scheduler decision and went back to C -- see lesson 69.) Worth +5 SLES and +4 SLUS with 177
    functions improving, nearly all of it from filling the empty branch. Run
    the histogram over every unit and the remaining gaps name themselves: `lq`/`sq` (the toolchain ceiling
    of lesson 63), `.word` (jump tables), `syscall` (the SDK), and 172 `madda.s` that live in exactly two
    hand-written asm routines. That is the useful negative result: no other systemic hole is left in
    bWare's maths.
68. **A non-void function with an empty body is always a hole, and there were 258 of them in headers.**
    Lesson 67 found `bSqrt` by counting mnemonics per unit. The same count per FUNCTION
    (`scripts/histofn.py`) turns a 5,000-instruction diff into one line: what is missing is 873 `lqc2` and
    314 `sqc2`, and what is left over is 927 `mul.s` and 737 `sub.s`. That named the cause without reading
    any disassembly: ten inline functions in `UVectorMath.hpp` -- the PS2 vector maths -- had the body
    `{}` or `return 0.0f`. They compile without a warning, return garbage, and GCC deletes whatever
    depends on them, so the callers are not wrong, they are ABSENT. 184 functions covering 200 kB hung off
    those ten. The forms are not guessed: `scripts/vu0bloques.py` walks the original, collects the runs of
    COP2 instructions and counts them, and the catalogue IS the list of bodies to write -- `vmulx.xyz` 179
    times, `vadd.xyz` 100, `vsub.xyz` 98, the dot product 88, scale-add 82 -- each one matching,
    instruction for instruction, the `Vector3` twin that was already written. `SphereVsBox` goes from
    19.71 % to 81.00 %. The razor generalises: a function that RETURNS something and has an empty body can
    never be right, while an empty `void` accessor often is (of 597 on GameCube the original only expanded
    225, and 222 were constructors or destructors). `scripts/huecos.py` sweeps for exactly that and still
    finds 248 in headers, where each one poisons every caller, plus 148 more in .cpp files that only lose
    themselves.

    Getting the count right took four normalisations, each one a measured false positive: local labels are
    named differently on each side (`$LC875` against `lbl_803DECF0`), a call to a function in the SAME
    object leaves no relocation in ours because the assembler resolves it (`_vfwrite` looked like 13
    missing calls and is called 13 times), the same happens with DATA and is far worse (`cFEng::mInstance`
    showed 831 missing references in `zFe`, every one of them false), and with `-g` the object carries the
    line labels `$LM3549` as symbols, so objdump opens a block at each one and `UpdateHud` came out with
    15 instructions out of 1,653.

    The empty body is only the first of three shapes, and the histogram finds all three the same way. The
    second is a function written as a COMPOSITION of others: `VU0_v4lengthxyz` was
    `VU0_sqrt(VU0_v4lengthsquarexyz(a))` and `VU0_v4unit` was `VU0_rsqrt(...)` then a scale. They compile
    and compute the right number, but they force the value out of VU0 and back in -- `qmfc2`, `mtc1`,
    `swc1`, plus a real call to `VU0_rsqrt`, which is out of line -- and the original never leaves VU0
    half way. Its form is one fused block, and the block for `lengthxyz` is the most repeated in the whole
    game: 123 appearances, with 52 for `distancexyz` and 27 for `unitxyz`. The third is a function that
    FORWARDS to another subsystem: all three `bMulMatrix` called Ecstasy's `eMulMatrix`/`eMulVector`, and
    the original carries the VU0 inside. Transcribing them from the binary closed all three exactly, and
    the three `vnop` in the two vector ones are not decoration -- in macro mode you must wait for the
    accumulate chain before reading the destination register, and the matrix one does not need them
    because the next three columns already do that waiting.

    One erratum of the catalogue, which changes no conclusion: `vsqrt` comes out as `c2 0x203bd` because
    this objdump cannot decode it, so it never appears in the printed forms. `vrsqrt` does decode. Between
    the three shapes the PS2 VU0 gap went from 1,979 instructions in 184 functions to 1,486 in 169, and
    what is left is no longer in the headers: it is in `zEagl4Anim`, `zWorld` and `zCamera`, where each
    function has its own hand-written VU0.

69. **Before keeping an asm block, measure its C alternative.** Every asm added or inherited has to pass
    one test: write the C version and measure the affected builds. Identical to the byte means the asm was
    scaffolding: `VU0_sqrt` copied `sqrt.s; nop; nop; mov.s $fD,$fD` from the binary -- the latency wait a
    COMPILER inserts, not something a person types -- and `sqrtf()` came out exactly the same, so it
    bought nothing. Losing a few bytes means the asm is forcing a scheduler decision: `bSqrt` with
    `sqrtf()` loses one 12-byte function per PS2 build because the asm kept the root out of a delay slot,
    and that is not a reason for asm either. What stays is what C cannot express at all: VU0 macro mode
    (`lqc2`, `vmul`, `vrsqrt`...), the MMI instructions (`pminw`/`pmaxw`) and COP0 (the cycle counter),
    which EE-GCC 2.9 has no intrinsics for -- and each of those lives only in the PS2 branch of a header
    whose other platforms are plain C. A plain FPU or integer instruction never justifies asm: if the
    original has it and we do not, the difference is in the shape of the source. One asm that DID need
    fixing rather than removing: `bGetTicker` read the COP0 counter without `volatile`, so GCC deleted the
    read wherever the value was unused (`UnloadChunks`, for a trace it never prints) while the original
    keeps it. Adding `volatile` -- which a hardware counter needs anyway, since every read differs -- was
    worth +23 SLES and +19 SLUS.

70. **Where a method lives, and whether a feature exists at all, are both in the prototype's dump.** The
    A124 `.mdebug` records, per unit, every out-of-line function AND every inline defined in the .cpp (at
    address `ffffffff`: `cFrontendDatabase::IsDirty`, `GetRandomRace`), but not the inlines of headers
    (`IsDDay` and `IsCareerMode` never appear). So a method missing from the records of the unit that
    would define it is a header inline in the prototype: `IsFinalEpicChase` and
    `GManager::GetHasPendingSMS` were, their callers expand the body, and making them inline under
    `EA_BUILD_A124` fixed `MemoryCard::Tick` and `ShouldDoAutoSave`. The absence of a whole class is
    proved by a string: every event returns its name from `GetEventName`, and "EReloadGame" is not in the
    prototype's ELF while the retail one has it. Reloading the world with the race restarted --
    `EReloadGame`, `Game_ReloadWorld`, the Lua binding "ReloadWorld", `GManager::PreBeginGameplay` and the
    member `mRestartEventHash` -- came after the prototype, and the right guard is the whole feature, not
    the member: the prototype's `GManager` constructor differed only in the `sw zero, 0x30C` that
    initialised it (1,720 B). The same with `MemoryCard`: in the prototype it went from 8 of 42 functions
    to 38 of 41 once `m_bCancelNextAutoSave` (prototype only) and `m_bAutoLoadDone` (both PS2 builds) left
    the class together with every call site, and on SLUS to 26 of 26. The three that remain differ only in
    the `__LINE__` passed to `new` (1079 in the prototype's file, 762 in ours): the same statement at
    another line of another file, which no one source can satisfy for both builds.

71. **On PS2, `enum > enum` is a function call.** The SGI STL that ships with EE-GCC defines `operator!=`,
    `>`, `<=` and `>=` as GLOBAL templates in `stl_relops.h`, which arrives with `stl_algobase.h`,
    `<list>`, `<utility>`... When both operands have the same enum (or bool, or class) type the template
    is an exact match and beats the built-in operator, so the code changes: the result is computed as a
    value and then tested (`slti`+`xori`+`beqz` instead of `blez`), and GCC no longer folds
    `x != A && x != B` into an unsigned range check. GameCube's STLport has no such globals, so there both
    shapes compile the same -- which is why the tree was full of the shape PS2 does not want: comparisons
    against int literals or `static_cast<int>`, and the folded ranges themselves (`static_cast<unsigned>
    (Status - LOADED) > 1`, marked "TODO weird"). The A124 `.mdebug` records every expansion: a `FILE`
    label for `stl_relops.h` followed by `$LM` entries whose index is line 38 (`!=`), 43 (`>`), 48 (`<=`)
    or 53 (`>=`), and our own PS2 objects carry an `.mdebug` too, so `scripts/relops.py` compares both
    sides function by function and gives the source line of each expansion. The fixes are always the
    natural source: the enum's own constant (`mCountdown > RACE_COUNTDOWN_NUMBER_GO`, `type >=
    GRace::kRaceType_NumTypes`), the two inequalities instead of the folded range, and -- where the
    original uses NO template -- `==` chains, early returns, `<` on both sides, or even the enum against
    plain integer literals (`SFXCTL_AccelTrans::UpdateTRQ` only matches with `!= 0`, `>= 0` and `<= 4`: the
    tidy version with the enum's constants is the one that drags the template in). The oracle decides. The same oracle showed
    three globals typed as `int` that the DWARF declares as enums (`PrecullerMode`, `mToggleCarList`,
    `ReverbZoneCrossMap`) and two units whose STL never gets included in our tree although the original's
    did (`zFEng`, `zEagl4Anim`), so their comparisons cannot use the template until we find the header
    that brought it in. `scripts/banco.py` compiles a SourceList outside the tree with some of its files
    replaced and diffs the result for GameCube, the A124 and SLUS, which is how each change was proved not
    to move GameCube before it touched the tree.

72. **When a whole family of near-misses shares one inline, fix the inline.** Our PS2 objects' `.mdebug`
    says which header lines each function expands, so `scripts/inlines.py global` can rank every
    (header, line) by how many of the functions that expand it match. A line expanded by dozens of
    functions of which NONE matches, while several sit at 97-99.9 %, points at the inline, not at the
    functions. The first pass found three in the PS2 math. `bAbs(int)` is MMI in the original: `negu`
    and `pmaxw c, a, -a`, which EE-GCC never generates from C (it gives `slt`/`movn` or a `bgez`
    branch), so it now has the same kind of asm as `bMin` and `bMax`. `bMin(int)` had its asm operands
    the wrong way round: the original is `pminw c, b, a` in every direct call and in every `bClamp`,
    while GameCube, whose C form is `b < a ? b : a`, already matched with the same call sites -- so the
    callers were right and the intrinsic was wrong. And `EAGL4Anim::FloatToInt` converts into `$f4` in 49
    of the 49 conversions of `zEagl4Anim`, from seven different source registers: no register allocator
    does that, so it is EA's asm with the register written in it. Each was measured with `banco.py
    --sombra` on the three builds before it touched the tree (GameCube cannot move: the changes are in
    the PS2 branch). The same report points at what is still open: the prototype's `FNEW` passes
    `__LINE__`, and 23 FEng functions sit at 99.9x % because their `Clone` is not on the line its
    `Decl:` comment gives.

73. **The PS2 game was built with VU0 registers the compiler can allocate.** Our VU0 inlines wrote their
    `vf` registers by hand, but the original's are not fixed: `bLength` of a `bVector3` loads into `vf1`
    and sums into `vf2` in one function, and the other way round in the next. The A124 stabs explain it:
    the inlines have 128-bit locals (`t`, `d`, `o`, `vfq`) whose register numbers are 79-83, and EE-GCC
    numbers `vf0`-`vf4` exactly so. EE-GCC only puts a local there through the `"j"` asm constraint, and
    only with `-mvu0-use-vf0-vfN`: without the option the constraint aborts with "fixed or forbidden
    register ... spilled for class VU_REGS". The option was missing from our PS2 flags, and on its own,
    before any source change, it already moves ordinary code: in the bench it gave +6 functions in zAI,
    +3 in zPhysics, +2 in zWorld and +1 in zEcstasy, and cost none. `bLength` rewritten with 128-bit
    locals (`u_long128`) and `"j"` operands, one asm statement per step so the scheduler can slide other instructions
    between the load and the multiply as the original does, closed five more in the two sound units. The
    rest of `bMath.hpp` followed the same way, every step its own statement as the original's line table
    has them: +43 A124 and +48 SLUS functions. The old forms also wrote `vf1`-`vf4` without declaring them,
    so the scheduler could slide one of those blocks between two statements of another inline and clobber
    its register: wrong code, not just a mismatch. `bIdentity`'s MMI seed needs no asm at all: a 128-bit
    constant in C (`u_long128 t = 0x3f800000`) gives `por` + `lui` per identity and lets the compiler hoist it
    between the stores like the original. `UVectorMath.hpp` is the opposite case, lesson 62's other family:
    across more than a thousand expansions each `VU0_*` uses the same registers every time and the stabs give
    it no VU0 locals, so its `vf` stay written in the asm (`"j"` on four of them lost 39 functions). Porting
    that family also found a real bug: `VU0_v3addscale` computed a + b·s where the original computes
    (a + b)·s, so `UMath::Mean` returned a + b/2. `uint128` in the stabs is the compiler's own name for the
    type; the source uses `u_long128`, from the SDK header that `bMath.hpp` already includes.

74. **A milestone build pins every allocation to its line.** The A124 defines `MILESTONE_BUILD`, so `FNEW`
    passes `__FILE__` and `__LINE__` and a function that allocates only matches with each `FNEW` on the
    original's line. The lines come from three places: the immediates of the `FEngMalloc` calls, the A124's
    `.mdebug` and GameCube's `debug_lines.txt`. Twenty-seven FEng files were re-laid out so every function
    starts on the original's prologue line (the line of its `{`, or of the `:` in constructors) and every
    statement with an `FNEW` sits on its line, filling the gaps with blank lines: the same tokens and
    comments, checked file by file, and +33 A124 functions. Two side effects. The `Decl:` comments of some
    FEng headers do not come from GameCube's DWARF and contradict the line map; the code follows the line
    map. And `clang-format` would join the blank lines and split the one-line methods, so
    `src/Speed/Indep/Src/FEng/.clang-format` disables it there.
75. **When GameCube already matches, its line map still holds the order PS2 needs.** GameCube's scheduler
    hides statement order that the EE does not: `MoviePlayer`'s constructor matched on GameCube with its
    member stores in another order, while GameCube's `debug_lines.txt` and the A124's `.mdebug` both put
    them in one line (an initializer list) with `m_DefaultMemDir` before the two function pointers; written
    that way it matches all three. The same file had three more forms PowerPC cannot tell apart: `(x &
    0x1FFC) << 17` and bWare's own `BMEMORY_ALIGNMENT_OFFSET(x)`, `((x >> 2) & 0x7ff) << 19` (one `rlwinm`
    on GameCube, `sra`/`andi`/`sll` on the EE); `sound ? ON : OFF` against `!sound ? OFF : ON`, which
    decides which of two constant 1s the EE materialises first for its `movz`/`movn`; and a `goto` chain
    against the condition GameCube's line map puts on one line, which is the only form that also matches PC.
    With those and the PS2 code the file lacked (two static bridges that switch the stack out of the EE
    scratchpad, a 40-entry volume table per PS2 build, no `GamecubeMaybeAllocateFromCarLoader`), the whole
    of `MoviePlayer.cpp` matches on both PS2 builds.
76. **The register a stab gives an inline's parameter says which argument is which.** EAGL4's
    `QuatMult(a, b, result)` matched on GameCube, and on both PS2 builds `AlignRootQ` differed only in the
    order of its first two instructions, the two addresses passed in. The A124's stabs put `a` in the
    register that holds `&sqt[4]` and `b` in the one that holds `mAlignQ`: the original calls
    `QuatMult(sqt, mAlignQ, result)` with a body that mirrors ours, and our header had the parameters' roles
    swapped (and named `q0`/`q1`, where GameCube's DWARF says `a`/`b`). The same arithmetic with the
    original's roles and the four calls' arguments swapped matches both PS2 builds; GameCube is unchanged.
77. **EE-GCC does not expand an inline whose body is a tail call, and the callee names the wrapper.**
    `UMath::Rotate(Vector3, Vector4, Vector3)` and `RotateTranslate(Vector3, Vector4, Vector3, Vector3)` are
    one-line wrappers that end in a call, so the original keeps a 16-byte copy of each in the units that
    use them, and the A124's symbol table names the one `Constraint::React` calls. Our `React` called
    `VU0_v3quatrotate_xlate` directly; with the wrapper (the overload GameCube's DWARF lists) it matches on
    all three builds, and the A124 gains the copy. SLUS's inherited `symbols.txt` had `Rotate` 16 B late, on
    top of `RotateTranslate`; objdiff scored it 100 % because both bodies are the same two instructions
    with the target masked. The same class had its base in GameCube's DWARF (`struct Quaternion : public
    Vector4`): with it, a dozen `reinterpret_cast`s went away and `Constraint::Resolve` and `Rotate` matched
    once written with the DWARF's locals.
78. **Loads the compiler reuses across inline calls are C, not asm.** `WWorldMath::PtDir4` (a 2D cross
    product) was plain C in our tree, and neither PS2 build had a single `vmsubz` where the originals have
    35 and 31. Its stabs give six `u_long128` locals in VU0 registers, and in `InTri` the second and third
    calls reuse vectors the first one loaded, even when the load came from another source line. Written as
    asm, our loads were repeated in every call (GCC keeps an asm's source position in the expression it
    compares for reuse); written as plain `u_long128` loads, which the compiler places in VU0 registers
    itself, they are reused exactly as in the original. Only the arithmetic is asm. With that form the VU0 block of `FindFaceInTriStrip` matches instruction for instruction on both
    builds. The same functions also showed that the EE keeps the source order of a commutative `+`
    (`cp.x + v / 128.0f`, not the other way round), which GameCube's `fmadds` hides.
79. **A single instruction two slots out of place can be a statement split in two.**
    `WorldMap::AddRoadBlocks` passed `img_num++` inside a call; GameCube's line map puts the increment on its
    own line after the call, and written that way it matches all three builds. The same shape (one register
    copy moved by two slots) is still open in `Speech::EventHistory::Init`, inside `UTL::Vector::insert`.
80. **A flag the original only ever switches one way is an `if`, not a comparison.** Four PS2 near-misses
    had `flag = a < b;` inside a block where the flag starts false (or true): `ResetCar::TrackState`,
    `AIActionHeliPursuit::StraightLinePursuit`, `RoadblockFlow::Setup` and spchpick's
    `iSPCH_ChooseSentenceIteratively`. The EE gives it away: the original either moves the constant under a
    branch-likely (`bc1tl` with `addiu 1` in its slot) or keeps the old value with a `movz`/`movn`, where the
    comparison form assigns both values. GameCube turns both forms into the same `cror`/`mfcr`, so its code
    cannot decide, but its line map can: `ResetCar` has both comparisons on one line, so the original is
    `if (a < b && c < d) flag = true;`. All four match on every build that has them.
81. **The line map counts a constructor's initializers.** GCC gives every member initializer the line of the
    constructor's opening brace, so the lines between the declaration and that brace are the initializer list,
    one per line. `SuspensionTraffic`'s constructor starts on line 342 and all its zero stores sit on line 354:
    eleven initializers, where our list had seven and assigned four members (`mRB`, `mRBComplex`, `mInput`,
    `mNumWheelsOnGround`) in the body. GameCube matched both ways; the A124 matches only with the eleven, in
    declaration order. Other statement orders came out of the same map on the same day (the best candidate's
    five assignments in `WRoadNav::FindClosestSegmentInd`, 2,752 B; `race_route_bogus` before `Reverse()` in
    `AStarSearch::Service`), and two ternaries (`pGoalNode`, and `nState`/`pSolution` when the node pool runs
    out) that the map writes on one line with both values.
82. **A literal GameCube hides, PS2 prints.** `CAnimCtrl::AdvanceAnimTime` scored 100 % on GameCube while
    dividing two delays by 3; the original divides by 30 (they were multiplied by 30.0f a few lines earlier).
    On GameCube the constant lives in the pool (`0x3d088889`, 1/30) and objdiff does not compare its value;
    on the EE it is a `lui`/`ori` immediate, and both PS2 builds showed it as a four-instruction near-miss.
    It was a behaviour bug, not a matching detail. PS2 near-misses whose only difference is a float immediate
    point at a wrong literal on every build.
83. **The A124 can be an older program, not only older headers.** Its path finder stores `AStarNode` as
    pointers and floats with a virtual destructor (0x20 bytes, from the stabs) where the retail builds pack five
    `short`s; its pool holds 2,048 nodes instead of 3,072 (`MAX_ASTAR_NODES`, a name GameCube's DWARF lists);
    its `AStarSearch` constructor always adds the node in the opposite direction, checks the goal segment even
    with no path and never clears a half-found GPS path; `WRoadNav::Reverse` has no exception for racers; and
    it has no `DRIVER_NIS` (the stabs jump from `DRIVER_NONE = 4` to `DRIVER_REMOTE = 6`), so its scene cars
    use `DRIVER_NONE` and `ai_behaviors` has eight rows. Written as `EA_BUILD_A124` variants, ten functions
    of the A124's path finder and road navigator matched, plus `NISActivity::PrepareVehicles`, and none of
    the other builds moved.
84. **A dead block keeps its locals in the stabs and its space in the frame.** The A124's
    `AStarSearch::Service` has thirty source lines without code between two statements: the stabs still list
    their locals (`int handles[2]`, `const WRoadNode *nodes[2]`, a `bVector3` and a `UMath::Vector3`) in
    blocks whose `$LBB`/`$LBE` share one address, and the frame is 0x20 bytes bigger than ours for them.
    Without the body that ceiling stays (98.6 %). The converse also holds: a local the optimiser left
    without a register does not appear in the stabs at all, so its absence proves nothing.
85. **`while` and `if` + `do`/`while` are one loop on GameCube, two on the EE.** Our `Service` copied the
    path in `if (a && b) { do { ... } while (...); }`, which GameCube matches; EE-GCC then tests `b` at the
    top and jumps into the loop, while the A124 tests both at the bottom with a branch-likely. Written as
    `while (a && b) { ... }`, it matches the A124 and still matches GameCube.
86. **GameCube's `.data` keeps the place of globals that lost their names.** Between `bPathFinderPrints`
    and `ASTAR_METRIC_SCALE` GameCube has three unnamed words; the DWARF lists `bAdmissibilityPrints`,
    `AStarNodeHighWater` and `AStarSearchHighWater` there, and the A124's globals name all three. Declared in
    the DWARF's order, our `.data` has the original's layout.
87. **PC and Xbox 360 raise the game's limits.** Fifty-nine PC functions of `AIPursuit` sat at 99 % with every
    member shifted by +0x28, then +0x50: each `IVehicle::List` is 0x60 bytes on PC, not 0x38. The proof is in the
    list vtables of the split PC objects: the `GetMaxCapacity` they point to returns 20 where we returned 10 (`mov
    eax, 0x14`). Across all units, every vehicle list of 10 is 20 on PC, the vehicle cache 18 is 28, and the model
    limit 434 is 2296. Xbox 360 has the same limits: its `IncNumCopsDestroyed` reads every member 0x28 further, its
    code uses 2296 wherever we used 434, and the data sizes in its own symbol file agree (`_mTable` 0x60, `_mLists`
    0x3C0). The declarations take `#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)`; template names that
    carry the capacity (`$09` = 10, `$0BE@` = 20) are renamed in the PC and Xbox 360 symbol files.
88. **Upstream's PC alignment fix, with the macro that also aligns on Xbox 360.** Upstream makes `ALIGN_16` align
    for real on PC and moves the `UMath::Vector3` members to `PS2ALIGN16`, which is empty on PC *and* on Xbox 360.
    On Xbox 360 those members are aligned (`WRoadNav::GetOccludedPosition`, `AITarget::TrackInternal` and others
    only match with that layout), so here they take `ALIGNVEC`: aligned on PS2 and Xbox 360, empty on GameCube and
    PC. GameCube, PS2 and Xbox 360 compile exactly as before.
89. **A compiler-numbered name is paired by pattern, not by number.** GCC 2.95 names a local class after a label
    counter (`Compare__...IconSortPCvT1.26770`); the number changes with any earlier declaration in the unit, and
    the mapping for it went stale seven times in a week. `tools/sufijo_local.py` resolves `*` patterns against the
    object just built, and only against *defined* symbols: sixteen local classes that looked like mere names turned
    out to be referenced but never emitted by our object.
90. **An FPU `asm` stays only when the binary proves EA wrote it.** Every FPU `asm` was taken out and each was
    rewritten with the C form that loses least; then each was put back only with proof that no C form can give the
    original's code. `bSqrt` on GameCube: with `-ffast-math`, GCC 2.95 rewrites `-(a*b - c)` before it can match
    `fnmsubs` (twelve forms, all five instructions per step instead of four), and the original's 269 `fnmsubs` all
    sit next to an `frsqrte`. PS2 `min.s`/`max.s`/`abs.s`: the original never puts them in a branch delay slot (0
    of 1,465), our C does (204 of 1,448); that is the scheduling barrier of an `asm`. `FloatToInt` converts in
    `$f4` 100 % of the time, where compiled C uses `$f4` 1.9 %. Those are EA's own `asm` and they stay, clearly
    marked; the loose `mfc1` we had written in `bScale` did not have that proof and stays in C.
91. **On the EE a field's type decides the order of loads and stores.** A callback parameter stored as `uint32` or
    as `void *` gives different orders through type-based alias analysis: the A124's streaming API takes `uint32`
    and the retail builds take pointers.
92. **An inline wrapper with a cast breaks its caller's tail call on the EE.** Written as out-of-line functions,
    the caller keeps its tail call, as in the original.
93. **GameCube's compiler inverts `if (x) return a; return b;` when a destructor runs** (`jump.c`, the "if (foo)
    bar; else break;" rewrite); EE-GCC does not. The PS2 twin shows the order the source had.
94. **The original never calls a message's destructor on PS2.** It calls `~Hermes::Message` directly, 127 times in
    the A124: the message classes have implicit destructors. Written as `~MXxx() {}`, EE-GCC emits them out of line
    and every sender calls them.
95. **GameCube's game units were built with `-fkeep-inline-functions`.** In the DWARF of all 33 SourceLists every
    `static inline` (217 of 217 in `zFe2`: `GXEnd`, `OSSetGQR2`...) has an out-of-line copy with range 0xFFFFFFFF,
    emitted by the compiler and stripped by the linker; the libraries have none. Without the option GCC 2.95 never
    emits the virtual overrides of local classes: the sixteen destructors and `Callback`s of `FEPlayerCarDB`'s local
    classes were referenced from their vtables but never defined, whatever the source said.
96. **The PS2 twin keeps what GameCube folds.** `-ffast-math`, `-fdelete-null-pointer-checks` and CSE remove code on
    GameCube that still changes its schedule; the same function in the A124 or SLUS still has it. The camera movers
    add `hcomp` to `Look` (folded to nothing on GameCube, yet without it the `stfs` order changes); the keyboard
    screen indexes `HeldButtons[i]` inside `case 4`; `CreateCars` keeps an `if (career)`. The other way round, a
    block only GameCube has (the headlight glass of `UpdateLightStateTextures`) is absent from both PS2 builds and
    PC, and takes `#ifdef EA_PLATFORM_GAMECUBE`. Two "irreducible" GameCube functions fell this way.
97. **PC was built with `/O2`, not `/Ox`.** `/O2` is `/Ox` plus `/GF` and `/Gy`, the Visual Studio .NET 2003 Release
    default. Without `/Gy` every function shares one `.text` and objdiff sizes ours up to the next symbol, alignment
    `int3`s included; `/GF` puts string literals in `??_C@` COMDATs as the original does. A bench over every PC unit
    gave 57 more matching functions and no losses. Xbox 360 is not settled: `/Gy` there gains 57 and loses 7,
    because its compiler puts more than the function in each COMDAT.
98. **MSVC 7.1 puts an `int3` after a call that does not return.** A function that ends in `call
    __CxxThrowException@8` or `call _exit` is followed by `0xCC`, and our compiler emits that byte as part of the
    function; IDA's export ends the function at the call. `scripts/int3_noreturn.py` adds the byte to those 117
    functions in the PC symbol file (`_luaD_throw` is left alone: Lua does not declare it `noreturn`).
99. **Upstream's DWARF line extractor dropped the last source entry of each unit.** `tools/dwarf1_gcc_line_info.py`
    read one line of the final `.debug_srcinfo` entry instead of reading up to its end: `symbols/debug_lines.txt`
    was missing 21,119 lines in 267 of 316 units, which left some library functions without a line map. The
    fixed tool reproduces the old file exactly and only adds lines, apart from one line of the committed file that
    had been edited by hand (`0x0x8005CCB8` where the ELF says `0x8005C308`).
100. **In FEng files a new comment line moves `FNEW`'s `__LINE__`.** Three comment lines in `FEngine.cpp` shifted
     the rest of the file and three A124 functions stopped matching. Comments there go at the end of an existing
     line; the file keeps its line count.
101. **Both PS2 builds allocate STL containers from `gFastMem`.** Their `stl_alloc.h` is EA's: a `chunky_alloc`
     whose `allocate` is `gFastMem.Alloc(n, "STL")`, expanded into every `_M_insert` and `_M_insert_aux`, and whose
     `deallocate` is an out-of-line function (`deallocate__t12chunky_alloc1b0PvUi`, with its parameters in the
     stabs). One header gave 38 A124 and 35 SLUS functions.
102. **Whether an `asm` is `volatile` is part of the proof.** `bMin`/`bMax` of float on PS2 are EA's `min.s`/`max.s`,
     but not volatile: the volatile version loses functions, the plain one with the local `f` the stabs list wins 39
     and 43. The A124's float-to-int inline (`bMath.hpp` lines 72-78) converts in place in 232 of 265 expansions,
     where compiled C does it in 12 of 739. vp6 writes 128-bit zeros with `sq $0`, which EE-GCC never emits (it goes
     through `por`), and `DEBUG_break` is `break 0, 6`. Each was taken with its proof and measured over every unit.
103. **The Xbox 360 build was linked with whole-program optimisation and without exceptions.** Functions such as
     `bStrLen` and `bStringHash` have no call anywhere in the executable (they were inlined across units) and
     `list::_Incsize` uses the constant 1 instead of its parameter; only 67 CRT functions carry exception data, where
     our objects have 2,498. Compiling unit by unit cannot reproduce all of it.
104. **A re-read of `p->field` right after storing it can depend on the number of pseudos, not on the source.** In
     GCC 2.95's CSE the store is hashed before `this` gets a quantity, and the load only finds it when both land in
     the same bucket (31 of them): adding unrelated code to a function makes the re-read appear or vanish.
105. **An empty `~X() {}` is not the implicit destructor on the EE.** Declared, it keeps its vtable and is emitted
     out of line with a body; the original's implicit one is `j base; nop` (8 bytes). Removing 81 empty destructors
     of option classes matched them.
106. **Code identical to ours is still unmatched while it has the wrong name.** 1,108 PC functions had code identical
     to ours; 153 of them carried a false name from IDA or from upstream's symbol file (`Class::Method`,
     `Method__7Class`) and were renamed by unique code identity inside their unit.
107. **One 128-bit value in a saved register changes a whole unit.** In SLUS `zWorld2`, 133 near-misses saved `s0`
     with `sd` where the original uses `sq`. EE-GCC saves a callee-saved register with `sq` for the rest of the unit
     once a 128-bit value has lived in it; in the original that starts in `FindFaceInTriStrip`, which keeps
     `PtDir4`'s result in `s0`. Ours extracted it with an `asm` `qmfc2.ni`; the original uses the `mtc1` of
     `bVU0Float::operator float` and leaves the `qmfc2` to the compiler, so the GPR holds 128 bits. One change:
     +195 functions.
108. **A pragma one compiler honours and another ignores can be the original layout.** `CARP::Trigger` is
     packed on GameCube and on MSVC, while PS2 reads its bit-field with `lw` + `srl`. `#pragma pack(1)` gives exactly
     that: ProDG and MSVC honour it, EE-GCC 2.9 ignores it (`sizeof` 9 against 12 in a probe). No platform guard.
109. **Shapes the EE tells apart and GameCube does not.** A `bool` accessor against the mask it wraps
     (`IsQuickRaceMode()` against `GetGameMode() & 4`: `sra`+`andi 1` against `andi mask`); `a && b` with an early
     return against two `if`s; a boolean computed in one statement (the GameCube line map puts `li 0` and `mfcr` on
     one line); and the order of addends (`a + b*c` against `b*c + a`). GameCube compiles each pair identically, so
     only the PS2 twin decides; in round 16 these shapes matched several hundred PS2 functions across areas.
110. **A sibling game's symbols can name ours.** The Wii build of NFS Undercover (internally "NFS MW2") ships its full
     symbol table and relocations: 27,941 named functions, 12,871 with the same name as in MW. Its named call graph,
     matched against ours and confirmed by objdiff on the code (never by the graph alone), named 82 PC and 9 SLUS
     functions at 100 %. The symbols come from the user's own disc and stay outside the repository.
111. **The A124 prototype differs more than members.** `UCOM`'s `_IList` has no out-of-line destructor there (93
     functions, 64 of them Lua message bindings), and its online code, online screens and PS2 pad driver are
     versions SLUS does not have. A version guard with the A124's own asm as proof is the honest form.
112. **PC's platform layer lives in zPlatform.** 1,255 functions of the PC exe between zPhysicsBehaviors and zSim are
     the Direct3D 9 platform layer (texture records of 0x20 bytes, solid platform info, PC chunk loaders); the
     game calls 37 IDirect3DDevice9 methods, 27 of ID3DXEffect and 32 effects with 132 shader programs.

## Traps: where tools lie

Every entry cost at least half a round. The rule behind all of them: demand from a measurement a
control that has to change.

### objdiff

- It masks relocated operands and call targets: a function can read 100 % while calling a symbol that
  does not exist or loading `1e-05f` for `1e-06f`. Check targets, literals and constants separately
  (`scripts/audit.py`).
- Pass both `-c functionRelocDiffs=none -c ppc.calculatePoolRelocations=false` to `diff`; without the
  second it invents pool relocations (48,348 B of finished functions read 94-99.9 %).
- It pairs symbols by name and by section: same size and 0 % is almost always a name (address-suffixed
  splitter names; `symbol_mappings` is ignored by `diff` and `report`), and a unit emitting `.text`
  where the target has `.over` reads 0 %.
- The data percentage measures names (`lbl_<addr>` vs `$LC`), not bytes: `scripts/datacmp.py` found
  63.8 % of SourceList data already equal where objdiff said ~0 %.
- `matched_code` is all-or-nothing per function; the fuzzy score gives partial credit and can rise
  while exact instructions fall. Count exact rows (`scripts/fndiff.py`).
- `REPLACE`/`INSERT`/`DELETE` rows are alignment artefacts of sequences of different length (a
  "pattern" of 460 PS2 epilogues with `ld` replaced was nothing); only `OP_MISMATCH` and
  `ARG_MISMATCH` at the same position are signals. Both lists are padded: count rows by kind.
- A unit marked `complete` counts 100 % without being compared: without a base object, and also with one
  that does not match. Our `Matching` flags are GameCube facts; on PS2 they counted 124 (A124) and 261
  (SLUS) functions that do not match, two of which our object does not even define. Measure with every
  unit incomplete (`scripts/pctall.py` does), as upstream's all-NonMatching configuration does. The trap
  is still there in the build log: the `Progress:` block of `build/versiones/<V>.log` is ninja's own
  REPORT step, with the flags on, and it read 11,878 SLUS functions when pctall counted 11,637. The
  figure is the line pctall prints, whose report is `build/versiones/<V>.json`. A compiler label
  (`$L121558`) used as a function name aborts the whole report, and only after the next re-split.
- A stale `report.json` has no `functions` key; the field is `fuzzy_match_percent`; `size` is a string.

### Build and staleness

- `build_direct.py` can print `1 ok` and leave an object not built from the current source (parallel
  builds: "Could not open output file" keeps the old `.o`); `--help` compiles everything. Seal objects
  with a hash of the ALLOC sections (`.line` moves with any edit).
- The link uses objects on disk, so a stale `.o` certifies old code; `main.dol` can be days old, and a
  failed build leaves the old DOL and report in place. Relink before believing "DOL OK".
- Ninja graph holes: `splits.txt`, `symbols.txt` and `keep.lst` were not dependencies (fixed), and
  `-MMD` missed `<...>` headers (now `-MD`). Symptom: a short DOL with no error; compare timestamps.
- The PS2 split did not rerun when `symbols.txt` changed: its wrapper never wrote the depfile the rule
  declares, and splat's cache keys each segment on its config and bytes, not on symbols ("0 split, 305
  cached", old names in every `.s`). `tools/splat_to_config_json.py` now writes the depfile and splits
  without the cache (under two minutes). To force a split, delete `build/<V>/config.json`.
- `configure.py --version V` can finish OK and emit no rules (missing `build/<V>/config.json`), and a
  failed split keeps the previous version's `objdiff.json`: a 0 % that is "nothing compiled", or one
  version measured as another.
- `keep.lst` rots silently: `$LC` numbers shift when a literal is added earlier; moving a boundary
  renames `gap_` padding to `pad_`; an entry naming no symbol is silence. Run `scripts/lcfix.py` and
  `keepchk.py`, and read every `FALLO` line (they check values, not addresses).
- The same happens to function-local statics: the `_name.N` of `DECLARE_CONTAINER_TYPE` (`UStandard.h`) at the
  start of the `.data` of zMission and zOnline are zero, unreferenced and kept only by name in `keep.lst`, and
  `N` comes from a counter global to cc1plus. One new inline body in a shared header (a `UTransform` constructor
  in `UVectorMath.h`) moved it by 3 and the DOL came out 32 B short with no function changed. Those entries are
  now `# @data UNIT OFFSET` directives that `scripts/lcfix.py zMission zOnline` resolves by position; run it
  after any change to a shared header. A single CRLF line in `keep.lst` had blinded `lcfix.py` for eight rounds.
- A `Matching` flag survives the source change that justified it: a promoted unit whose source
  regressed moved half the DOL while the report said all was well.
- `scripts/frozen.py` fingerprints include `.debug`/`.line` sizes: "changed" with identical code.

### Our own measurement tools

- `linkdelta.py` compares sizes: 0 is not close (zSpeech at `rodata -8` had 83 of 703 functions
  misplaced), and `.text +0` can be two errors cancelling.
- `dolwhere.py` counts ranges, not bytes (183,693 reported for 15,369 real) and blames the nearest of
  our symbols; `vtord.py` and `textorder.py` compare objects, so they count symbols the linker strips.
- `movidos.py`'s subtraction mixes stolen bulk, stripped code and padding: decide ownership per symbol
  with the linker's rules. `aranges.py` sizes are object sizes; its `--dir` sees no holes off `.text`.
- Censuses by name merge overloads (597 "empty accessors" were 3 real ones), `regmap.py` pairs locals
  by position and type and can invent permutations, and the alignment scanner had 57 false positives
  of 71. Confirm against the DWARF or the stabs.
- The permuter's score can rise while opcodes fall, and its winners can be wrong code (three `madidct`
  permutations swapped operands at 53-57 %). Verify semantics, run `fndiff` on the full unit, and
  reject small gains that pull the source away from the line map.
- Ghidra's C is not the source form: compare against the asm and our object, not our source text.
- In PS2 units built with SN's 2.95.3 that have no initialised global, the static initialiser is
  `_GLOBAL_$I$src_<file>.cpp` plus six characters from GCC's `append_random_chars`, which this build
  takes from the process id: three identical compiles of `pathreal.cpp` gave three names. A name in
  `symbols.txt` copied from one of our builds matches until that object is rebuilt, so its "loss" in a
  measurement is not a regression. `tools/sufijo_local.py` pairs them with the original's by unit and kind; the six
  characters are letters *and* digits (a pattern with letters only missed five after a full PS2 rebuild).

### Reading and setup

- A shadow header via `-I` is ignored when the includer uses a relative `#include "..."`: put an
  `#error` in the shadow and demand a failure. A control that cannot fail (`s = s;`) proves nothing.
- A negative delta in a progress series is a measurement error. A unit at 0 % in one version and fine
  in another is a measurement problem first (name, section, defines).
- A finding in one unit is not a front until counted in the others (three false fronts in one round).
  Old vetoes expire: 12 of 55 were broken, mostly because N forms of the wrong statement had been
  swept. Check prior attempts with `scripts/previo.py`.
- Environment: Git Bash heredocs lose backslashes (write such scripts to files); `grep -c $'\r'` counts
  every line; `subprocess` with `text=True` adds `\r` to stdin (`git update-index` then ignores every
  path with exit code 0); scripts with a hard-coded repository root diff the main tree's objects from a
  worktree; EE-GCC and the 2005 `cl.exe` fail on paths over 260 characters. Git Bash also turns an argument like
  `--cl=/Gy` into a Windows path before a native program sees it: `cl.exe` takes it for an object file, compiles
  without the option and the bench reports no change. Pass compiler options with a dash (`-Gy`).
- EE-GCC's dependency files came from `DEPENDENCIES_OUTPUT`, which in GCC 2.95's preprocessor leaves out everything
  included with `<...>`: changing the PS2 STL left six units per build with old objects and the measurement came out
  short. The rule now sets `SUNPRO_DEPENDENCIES`, which lists every header (the same fix `-MD` was for ProDG).

## Working method

Since 2026-09-17 the fork measures and decides like upstream. The rule, from the project owner: **an
80 % that is real and portable is worth more than a 100 % held up by register pins that only compiles
on GameCube.** The percentage is not the goal; real code that compiles in all five versions is.

- **Zero scaffolding asm.** No register pins, no barriers, no hand-written `.rodata` blocks, no
  `__asm__("lbl_...")` aliases: a pin is a hard error in EE-GCC and MSVC. Asm that stays is EA's own
  (paired singles, VU0 intrinsics in `bMath.hpp`/`UVectorMath.hpp`, `mfdec`/`mtdec`, GQR setup),
  toolchain headers, and functions the line map shows were asm in the original. EA's asm may be
  corrected (VU0 masks); ours is removed.
- **Upstream's forbidden list:** assigning registers by hand; accessing struct members by pointer
  offset; invented strings or floats; random renames; implementing the same code twice behind include
  guards or per-compiler `#ifdef`; redeclaring used functions at the top of each file; casting a
  variable to its own type.
- **Our plausibility criterion** for replacing scaffolding: only constructs EA could have written. No
  `#line`, no `__attribute__((aligned))` or `section`, no dead "primer" functions that place strings.
  Content trimmed by the original linker is written with the DWARF's name and type, zero-initialized
  and annotated. A doubtful name needs independent order evidence (PS2 map, use in the tree, adjacent
  member); adjacency in the DWARF alone is not enough.
- **The `Matching` flag is ignored.** It is configuration state, not a measurement.

### How we measure

`python scripts/pctall.py` configures, builds (`ninja -k 0`) and writes one objdiff report per version
(`GOWE69`, `SLES-53558-A124`, `SLUS-21351`, `EUROPEGERMILESTONE`, `SPEED_EXE_1_3`); `--solo-informe`
measures existing objects. A figure counts only if its line says **0 objects fail to compile**; the
script closes the three measured traps (a `complete` unit without object counted 100 %, a stale
`objdiff.json` from another version, a `configure.py` that emitted no rules). Run on a worktree of
upstream's main branch, the same measurement reproduces upstream's published GameCube figure to the
decimal. A unit is done when its object matches and the same source compiles in all five versions,
compiling first. Near-misses are worth their full size.

### The cycle

1. **Measure all five versions** (`pctall.py`): the baseline for every change.
2. **Census the gap** (`scripts/censohueco.py`): bytes of 80-100 % functions per unit, and the column
   that finds common causes, "at 100 % on GameCube". If most PS2 near-misses of a unit are 100 % on
   GameCube, the source is right and a platform cause (compiler, layout, flags) is waiting: 243 of
   zAI's 259 PS2 near-misses were VU0 masks.
3. **Diff the units with the largest gap** (`scripts/diffunidad.py <V> <units>`).
4. **Read only the rows that mean something**: `DIFF_OP_MISMATCH` and `DIFF_ARG_MISMATCH` at the same
   position. `scripts/tabla_op.py` builds the "we emit -> original emits" opcode table (a 99-100 %
   mapping is a direct correction); `scripts/desplazamientos.py` finds loads that differ only in
   displacement, i.e. layout faults, grouped by class.
5. **Confirm the cause with an oracle before editing**: `scripts/sonda.py`, `sondastabs.py` and
   `cotejo.py` for layout; `missinline.py` for inlines; `dwarf1.py`/`mdebug.py` for locals, registers
   and inline order; the original object's disassembly for what the prototype did in a branch. If no
   oracle can decide the form, nothing is written: note it and move on.
6. **Fix the minimum.** Remove each part of a multi-part change to check it is needed; edit line by
   line after checking the text, never by blind pattern replacement.
7. **Measure all five again and check the GameCube control**: relink and compare `build/GOWE69/main.dol`
   with SHA-1 `9619ba57c9919f95f7f2ac951a2166a3517f91e3`, which must not move unless intended.

Before a function: read prior attempts (`scripts/previo.py`); triage by type of difference, not by
percentage (`scripts/triaje.py`: register-only, one local missing, extra code, structure); look one
layer up (a shared header, a missing mother function, a sister at 100 % to copy). Classes written from
the DWARF and line map (`scripts/plan.py`) mostly match at the first compile. A big gain that stalls at
two or three rows means the wrong layer. Never leave a function half written: it lowers its neighbours.

Measurement faults are worth as much as code: a missing section rename, the prototype's defines on the
retail build, PC names and a label used as a function name were each worth hundreds of functions. Upstream
code is adopted per function after A/B: it matched no GameCube function we did not, but about 500 PS2
functions (frontend, zPhysicsBehaviors) where its form matches and ours did not.

### What a commit carries

One cause per commit, with its evidence: the per-version before/after lines (percentage, functions,
"0 objects fail to compile"), the DOL SHA-1, the oracle output that justified the change (probe
offsets, opcode table, stabs fields), and what was tried and discarded, with the reason. Agents do not
edit `keep.lst`, `configure.py`, `config/` or shared headers: they propose, and each proposal is
applied, rejected or deferred with a reason between batches. Reference result (round 81): SLES
24.72 -> 28.07 %, SLUS 26.71 -> 32.33 %, PC 9.16 -> 9.99 %, Xbox 360 1.89 -> 1.93 %, GameCube 97.14 %
unchanged, with no new asm, the DOL untouched and 0 objects failing in every version.

---

# Build, measurement and tools

## Build and measurement changes

The build keeps upstream's decomp-toolkit layout: `configure.py` declares every object and its
flags, and `tools/project.py` turns that into `build.ninja` and `objdiff.json`. The fork changed
both for three reasons: every build must compile and be measured honestly, each library must be
compiled the way EA compiled it, and ninja must rebuild whatever changed. Most flag choices in
`configure.py` carry a comment (in Spanish) with the measured effect in bytes and functions; those
comments are the evidence and stay next to the flag.

### Five builds from one `configure.py`

Upstream already lists the five versions. These are the build-level changes that give each version
its own defines and let all five compile with zero failed objects; the source-side portability work
is covered in the source-tree section.

- **Per-version defines.** `EA_BUILD_A124` and `MILESTONE_BUILD` were defined for both PS2 builds.
  They now apply only to version index 2, the A124 prototype, in the compiler flags and in
  `context_defines`. Why: `SLUS-21351` is the retail build. Its ELF has no `bMalloc__FiPCcii`, the
  four-argument allocator with file and line that exists only in milestone builds, and the A124 ELF
  has it. Separating them gave SLUS +268 functions (+83,296 B) and left the other four builds
  unchanged. The only source change it needed was an inconsistent guard in our tree: a member's
  initializer was guarded differently from the member.
- **`-D_STL=` on PS2.** On GameCube, STLport puts the STL in namespace `_STL`, and shared code
  writes `_STL::sort(...)` in 49 places. The PS2 ELF has no `_STL` symbol, because there the SGI
  STL lives in the global namespace. Defining `_STL` as empty turns those calls into `::sort`
  without touching the source and without any effect on GameCube. Thirteen files later moved to
  `std::`, as upstream does, because MSVC has no `_STL` either.
- **`-DUSEREALCORE` on every platform.** `path.h` declares `PATH_vectortoreal6` and
  `PATH_setallocator` only under it.
- **`/wd4716` on PC**, as the Xbox 360 block already had: functions that are not decompiled yet
  have empty bodies, and MSVC treats C4716 ("must return a value") as an error.
- **`cflags_path` and `cflags_spch` in the Xbox 360 and PC blocks.** The fork added the `path` and
  `spch` libraries. Without these lists, `configure.py -v EUROPEGERMILESTONE` and
  `-v SPEED_EXE_1_3` stopped with a `NameError`.
- **`solo_gc()` and `solo_ps2()`** wrap platform-only options. EE-GCC and MSVC reject ProDG options
  such as `-mps-float` and `-fvtable-thunks=1`. SN-only options such as `-frelax-128` exist only on
  PS2.
- objdiff is pinned to v3.8.0 (upstream: v3.7.0).

### Per-library compilers and flags

**Why:** the middleware had been compiled with the game's flags (`cflags_game`) or with a copy of
`snd`'s. When a library's source is written and it still matches poorly, the first suspects are the
flags or the compiler, not the source. Each flag was measured in both directions over the whole
library. A flag that helps some units and hurts others is set on those objects only.
**How:** each library entry sets its own `cflags` and `toolchain_version`, and single objects
override them with `cflags=` or `extra_cflags=`.

GameCube (SN ProDG), main cases:

| library / unit | change | evidence |
|---|---|---|
| `rcmp` | `-O2`, `-mps-float` instead of `-mps-nodf`, `-fno-force-addr`, `-fno-implement-inlines`; no `-G0`, `-fno-static-dtors` or `-fmove-all-movables` | `mtctr/bdnz` loops appear only at -O2; FPR saves use `stfd`, not `psq_st`; the target uses `@sda21` accesses |
| `vp6` (new library) | `-O2`, `-mps-float`, no `-fmove-all-movables`/`-fforce-addr`, `-fno-common` on some objects | `postproc` 62.91 % → 85.31 %; `criticalpath` uses `psq_l/psq_st`, which the compiler rejects without `-mps-float` |
| `path`, `spch` (new) | `path`: its own list (`-G8`, `-O2`, `-ffast-math`, `-fno-implement-inlines`) without the `-G0` and `-fno-strength-reduce` copied from `snd`; `spch`: `-x c++` | dropping the copied flags was worth 1,852 B and 10 functions (`pathcontrol` 34.3 % → 100 %); the symbols of `spch`'s `.c` files are C++-mangled (`SPCH_GetExtVecs__Fv`) |
| `realcore` | `-G8`, `-fstrength-reduce`, `-fno-implement-inlines`; no `-fforce-addr`; static destructors kept | units with global polymorphic objects emit `_GLOBAL_.D.` |
| `realmemcard` | `-fshort-wchar`, `-fstrength-reduce`; no `-G0`, `-fforce-addr` or `-fno-static-dtors` | the symbols mangle `Uw` (unsigned 16-bit `wchar_t`) |
| `realgraph` (new: RealShape/RealFont) | no `-G0` or `-fforce-addr`; `-fstrength-reduce` | the units go from 83.2 % to 100 % |
| `snd` | a second block of seven units without `-ffast-math` | the link order shows a second library archive; `SNDI_root1x.c` only matches with the DWARF's locals when that reassociation is off, and the other six give the same object either way |
| `libsn` | the libc flags plus `-G0` | the target uses `lis @ha/@l`; without `-G0` we emit `@sda21` |
| `libc` / libm | `-O2`; per object `-msafe-sda`, `-mstrict-align`, `-G1024`, `-fno-builtin`, and `-fno-expensive-optimizations` on three objects | newlib 1.8 compiled as-is: pool in `.sdata`, tables read through `@sda21`, `fabs` as a call; `-fno-expensive-optimizations` would drop 14 other units of the library from 100 % to 0 %, hence per object |
| `eathread` | `-O2` | the static initializer comes out as `li/mtctr/bdnz` only at -O2 |
| `zEcstasy`, `zFoundation` | `-mps-float` (and `-fvtable-thunks=1`), through `solo_gc` | the original uses explicit paired-single intrinsics |
| `zFeOverlay` | `-fno-implicit-templates` | we emitted 96 implicit template instantiations that the original does not have |

Some objects in what had been treated as SN code were built with **Metrowerks**:

- `libsn_mw` (`vm.c`, `VMPageReplacement.c`, `VMMapping.c`, `vmbase.c`) and `libsn_mw_steering`
  (the Logitech wheel driver, `steering.c`) use MWCC `GC/2.7`. The evidence: these objects have no
  `gcc2_compiled.` symbol (it is in 100 of 105 GCC units and in 0 of 95 Metrowerks ones), they call
  `__div2i` and `_savegpr_23/24`, and their function statics are mangled `name$NNN`. The asm shows
  counted loops, a real `divw` and `fmadds`, so `steering.c` needs `-O4,s`, `-fp_contract on` and
  `-use_lmw_stmw on`.
- `OdemuExi2` uses `GC/1.2.5n`: MWCC 1.0–1.2.5n emits `addi rD,rS,0` as a register copy, and 1.3
  and later emit `mr`. Two more AMC units found in the ELF go to `OdemuExi2Amc` with `GC/1.2.5`.
- `Runtime.PPCEABI.H/__va_arg.c` was split out of an object of GCC runtime bridges and uses
  `GC/1.2.5n`.

The PS2 middleware uses a different compiler from the game:

- `PS2_TOOLCHAIN` and `toolchain_de(lib)` compile the EA libraries (rcmp, eathread, realcore, spch,
  vp6, snd, realmemcard, path) with **SN's ee-gcc 2.95.3 build 1.36**. The game itself keeps
  `2.9-ee-991111`. Why: the A124 `.mdebug` records each file's build directory. The game comes from
  `mw\Speed` (991111b), and the libraries come from EA's `PlayStation2SN` packages (2.8.1-8,
  2.8.1-9, 3.0.2-1, 3.0.2-4). The library code saves s0–s7/fp with `sq` and `ra` with `sd`. Of the
  five SN ee-gcc builds available, only 2.95.3-136 does that. Without touching a SourceList, SLES
  went from 23.31 % to 24.72 % and SLUS from 25.53 % to 26.71 %. Per-library flags were then
  measured on top: `-G0` for path, realcore and rcmp (the original addresses nothing through `$gp`),
  `-ffast-math -fno-exceptions -fno-rtti` for path, and `-O2` everywhere.
- Trap: in those compilers `long long` is 128 bits and `long` is 64, and EA's code uses `long` for
  64 bits. `filesys.cpp` and `avplayer.cpp` compute in `long long` and only compile with SN's
  `-frelax-128`. Of the 17 functions that got worse with the new compiler (607 improved), six were
  `filesys.cpp` accessors at 100 % that had only matched because `long long` is 8 bytes in the
  game's compiler; they need the source to use `long`.
- These compilers are not in the compilers archive that `configure.py` downloads (same tag as
  upstream). The fork uses decomp.me's Windows builds, installed under `build/compilers/PS2/`.
  `tools/project.py` maps the five SN ee-gcc versions to decomp.me compiler ids.

- `snd` on PS2 is compiled with `-mvu0-use-vf0-vf31` (the game uses `-mvu0-use-vf0-vf15`). The option changes no
  instruction by itself: with more free registers, loop.c's threshold for hoisting invariants out of loops that
  contain calls goes up, and the original hoists constants and address high parts that we left inside. Measured on
  the 134 `snd` units of each PS2 build: +6 functions, none lost.

### Replicating a compiler that is not public: SN v1.83 for `path`

**Why:** the GameCube DWARF says `GNU C++ 2.95.3 SN BUILD v1.76` for 288 units, `v1.72` for 13 (`rcmp`) and `v1.83`
for 15: the `path` music library, from EA's package `GameCubeSN\3.9.3-4-non-proxy`. On PS2 the `.mdebug` gives
`PlayStation2SN\3.0.2-4` for `path` and 3.0.2-1 or older for the rest. Neither compiler is public (decomp.me and
archive.org stop at v1.76 and ee-gcc build 1.36). ProDG 3.7, 3.8.1 and 3.9.3 give byte-identical objects on the
game, so the version alone explained nothing, except for `path`.
**What differs:** one real GCC change. GCC's ChangeLog of 2000-06-23 (Geoffrey Keating): "alias.c
(fixed_scalar_and_varying_struct_p): Don't examine struct vs. scalar-ness when -fno-strict-aliasing." It went into
GCC 3.0, not into the 2.95 branch. SN's GPL source release of v1.79 (`orig/prodg/`) still applies the rule, so SN took
the change between v1.79 and v1.83. With `-fno-strict-aliasing` (2.95's default, and the only setting used here) the
change is the same as the function always returning NULL.
**How:** `scripts/replica_v183.py` copies the official toolchains to `build/compilers/ProDG/3.9.3-v183r` and
`build/compilers/PS2/ee-gcc2.95.3-136-v183r` and writes `xor eax,eax; ret` over the first three bytes of that function
in `cc1` and `cc1plus`, which it finds by its body. It checks the SHA-1 of every file before and after, and writes
nothing if one does not match. `configure.py` runs it and compiles `path` only with these toolchains
(`GC_TOOLCHAIN` and `PS2_TOOLCHAIN`).
**Evidence:** positive, `path` goes from 182 to 193 of 194 functions on GameCube and gains 6 on each PS2; the three
`*(unsigned long *)&` casts that `pathserv.cpp` used to dodge the rule can be removed, after which the function
matches on the three builds and the GameCube DOL still links byte for byte. Negative, the same replica applied to the
v1.76 libraries loses 59 functions on GameCube and 28 on each PS2: it only fits the units the DWARF marks v1.83.

### Replicating SN's assembler for its PS2 libc

**Why:** SN's PS2 libc (newlib 1.8.1 built by SN, and the R5900 string functions written in assembly in
`newlib/libc/machine/r5900/`) was assembled with SN's own assembler, Ps2EeAs, not with GNU as: `dli` comes out the
way Ps2EeAs expands it. The Ps2EeAs we have (1.9.25.758) still differs in one thing: both pad short loops (the R5900
short-loop erratum) with `nop` before the backward branch, but ours pads them to 7 instructions and the one that built
the game's libc padded them to 6.
**How:** `scripts/replica_snas.py` copies `build/compilers/PS2/ee-gcc2.95.3-136` to
`build/compilers/PS2/ee-gcc2.95.3-136-snas` and changes the two constants of that check in `ee/bin/Ps2EeAs.exe`
(6 to 5, two bytes), checking the SHA-1 before and after. `configure.py` runs it and gives the copy to the three SN
libc libraries (`PS2_TOOLCHAIN["libc_sn"]`). Their C goes through `ee-gcc -S` and that Ps2EeAs
(`tools/ee_gcc_pp.py --as`, rule `ee-gcc_snas`); the `.S` files through `ee-gcc -E` and Ps2EeAs (`tools/ee_as_S.py`).
**Evidence:** the 13 `r5900` string functions match on both PS2 builds (3 of 13 with the official assembler), and the
C units gain 16 functions on the A124 and 15 on SLUS (`mprec`, `_strtod_r`, `_strtol_r`, `_strtoul_r`, `exit`,
`strstr`, `__sfvwrite`, `_malloc_r`, `__sfp`), with none lost.

### Building SN's compiler from its GPL source

SN's GPL source release of its GameCube compiler (`orig/prodg/`, "SN BUILD v1.79") builds with emoose/re4's recipe
(`tools/sn-gcc` in that project: `-m32 -std=gnu89 -fpermissive -fcommon -fno-strict-aliasing`, CRLF to LF, the
pregenerated parsers, a use-after-scope fix in `cp/decl.c`) as a Linux binary and, with mingw-w64 and SN's own
MSVC/WinNT host configuration unchanged, as a Windows `.exe`. Against the official v1.76 it gives the same assembly on
409 of 409 v1.76 units (28,163 symbols). Two things came out of it:
- the `cc1plus.exe` in `build/compilers/ProDG/3.9.3` is not SN's as shipped: it carries the binary `$LC` patch of
  `tools/patch-toolchain.py`, which can be written as a source patch;
- v1.79 plus Keating's alias.c change gives the same objects as the three-byte replica above on the 27 units of `path`
  and `rcmp`, and the 8 bytes of stack frame left in `path`'s `CreateStreamTrack` were not the compiler at all but
  `-mps-float`: SN's `caller-save.c` reserves an 8-byte slot per FPR live across a call with it. `path` (like its
  sibling `snd`) now has the flag on GameCube and is 194/194.

**Using it (optional).** The normal build keeps the official binaries. `tools/sn-gcc/` holds the recipe, so that
anyone with Linux or WSL can make the GameCube compilers from the source instead, with both binary patches written as
readable source patches: `Makefile.linux` and `Makefile.mingw`, `patches/linux-host.patch` (emoose/re4's, CC0),
`patches/dollar-lc.patch` (the `$LC` labels of `tools/patch-toolchain.py`, in cc1plus only, like the binary patch),
`patches/keating-alias.patch` (the v1.83 change that `scripts/replica_v183.py` writes as three bytes), and a
`README.md` with the origin of each piece. Nothing is downloaded: the source is read from `orig/prodg/` (extracted, or
the zip) and is not copied into the repository.

1. `python configure.py` and `ninja` once, so that `build/compilers/ProDG/3.9.3` exists.
2. `bash tools/sn-gcc/build.sh` on Linux, or `wsl bash tools/sn-gcc/build.sh` from the repository root on Windows
   (packages: `make patch python3 gcc-multilib gcc-mingw-w64-i686`; about 3 minutes on 4 cores). It builds the
   generator programs with `gcc -m32`, then `cc1.exe`/`cc1plus.exe` with mingw-w64 and SN's own host configuration,
   and installs two toolchains, each a copy of `ProDG/3.9.3` in which only those two files are replaced:
   `build/compilers/ProDG/3.9.3-src` (v1.79 + `dollar-lc`, in place of `ProDG/3.9.3`) and
   `build/compilers/ProDG/3.9.3-v183src` (v1.79 + `keating-alias` + `dollar-lc`, in place of `ProDG/3.9.3-v183r`
   for `path`). The build is reproducible: the script prints the SHA-1s and says whether they are those of the
   reference build (Ubuntu 24.04, mingw-w64 GCC 13-win32).
3. `python configure.py --gc-toolchain-source` (alias `--gc-toolchain-fuente`), then `ninja`. The environment
   variable `NFSMW_GC_TOOLCHAIN_SOURCE=1` does the same, and is how the option survives `scripts/pctall.py` and
   `scripts/medir.py`, which run `configure.py` without arguments. Without either, nothing changes. If a `-src`
   toolchain is missing, `configure.py` stops and says how to build it. Switching recompiles every ProDG unit of
   GameCube (their command line changes).

Measured on GOWE69 (2026-09-30), the two builds one after the other under a single lock of `scripts/medir.py`, so
that the tree could not change in between:
- the objdiff report is the same function by function: 18,387 of 18,432 functions and 3,891,328 of 3,946,048 bytes
  of code matched with and without the option, and no function has another percentage or size;
- the DOL links to SHA-1 `9619ba57c9919f95f7f2ac951a2166a3517f91e3` both ways (`build/GOWE69/ok` written);
- the 436 objects compiled with ProDG are byte-identical once the version string is set aside ("SN BUILD v1.79"
  for "v1.76", only in the DWARF producer), except `rcmp_mad_codec_chunk_types.o`, whose `_GLOBAL_.N...` symbol
  carries a four-letter suffix that changes with every compilation (it comes from the process id), whatever the
  compiler;
- time: the whole `scripts/pctall.py GOWE69` run (configure, rebuild of the 436 ProDG units with ninja's default
  parallelism, link, report) took 157 s with the option and 188 s without, and the compilations add up to 1,074 s
  against 1,536 s; other jobs shared the machine. One unit at a time, best of three, alternating the compilers:
  `zAI` 12.7 s against 18.1 s, `zPhysics` 9.4 against 12.9, `zSim` 9.1 against 14.2, `zEAXSound2` 9.1 against
  16.0, and the same for small units (0.3 to 0.6 s): 41 s against 62 s in all (66 %), with identical objects;
- with the option, `configure.py` prints "Missing scratch compiler mapping" once per unit (441 lines), because
  `tools/project.py` maps only `ProDG/3.9.3` to a decomp.me compiler; the units then lack their decomp.me scratch
  settings in `objdiff.json`, which the report does not use.

### How EA's inline asm is proven: the compiler as judge

The fork keeps inline asm only where the binary proves EA wrote it. With the compilers built from source (SN's for
GameCube, and Sony/Cygnus's `ee-gcc 2.9-ee-991111` for the PS2 game, which reproduces the shipped one on 99.4 % of the
game's functions once Sony's VU0 register bank is rebuilt from the tables in the official `.exe`), every asm statement
was classified: 277 of 378 need an instruction no pattern of the machine description emits (VU0 arithmetic, MMI, COP0,
`ei`/`di`, `frsqrte`, `fres`...), and 101 use an instruction the compiler has, but in a form it cannot produce: never
in a delay slot (`reorg.c`'s `stop_search_p` never puts an asm there: 0 of 1,371 in the original, against 9 of about
200 for the same instructions coming from C), a fixed register reload would not pick (`cvt.w.s` into `$f4` in 29 of 29
expansions against 8 of 540 in C), a line of its own in the line map, and the locals the stabs or the DWARF give the
block. Seventeen C forms were measured against the asm: all of them lose functions, none gains. One caveat: the R5900
compiler does emit `nop` before the backward branch of loops of six instructions or fewer
(`mips_r5900_lengthen_loops`), so not every `nop` is EA's.

Added on 2026-10-01 with the same tests: `SNDI_ftoiround`/`SNDI_ftoiroundpositive` in `sndcmn.h` (`add.s` and
`cvt.w.s` into `$f4` on one source line, in 8 of 8 expansions), `_HackFpToSi`/`_HackFpToUi` in the PS2
`Platform.cpp` (they have no procedure in the `.mdebug`, and `HackPatchFloatToInt` copies their 16 bytes as data
over libgcc's `fptosi`/`fptoui`), and the `nop` lines of `VRAMView` and `eWaitUntilRenderingDone` (eight source
lines of eight `nop` each, like `WaitUntilVSync`).

### The EE-GCC two-step wrapper (`tools/ee_gcc_pp.py`)

**Why:** the `2.9-ee-991111` driver preprocesses into a temporary `.ii` whose line markers name the
real source. cc1plus therefore sees two file names and rejects every `#pragma implementation`
("can only appear at top-level"). The pragma is real in the original: the A124 linker map places
`ITrafficCenter`'s destructor and vtable in `zcamera.cpp.obj`. It could not be dropped, and it
blocked whole SourceLists (zCamera) on PS2.
**How:** the ninja `ee-gcc` rule calls
`python tools/ee_gcc_pp.py build/<V>/pp <ee-gcc> -- <flags> -c -o <out> <src>`. The wrapper first
preprocesses to `build/<V>/pp/<source>`. It then compiles that file as `(c++-)cpp-output` from that
directory, under the same relative name that the line markers use. `DEPENDENCIES_OUTPUT` applies
only to the preprocessing step. The wrapper deletes the output first, so a failed compile cannot
leave the previous object behind to be measured as if it came from the current source. Before this,
stale objects had inflated the PS2 figure. The result has the same `.text`, `.data`, `.rodata` and
relocations as a one-step compile (checked on `zAI.o`). No other compiler was needed for the game
code: the A124 binary names `2.9-ee-991111b`, the compiler the project already used.

### Section renaming (`section_rename`)

**Why:** the original linked `zOnline` and `zFeOverlay` into the overlay section `.over`
(`*zFEOverlay.o(.over .rodata)` in `config/GOWE69/ldscript.ld`). The target objects therefore carry
their code in `.over`, on GameCube and in both PS2 builds, while our compilers emit `.text`. objdiff
pairs symbols per section, so both units read as entirely unmatched. On GameCube the fix took
zFeOverlay from 0.38 % to 9.98 % (7 → 97 functions). On PS2 it made visible 217 (SLES) and 197
(SLUS) functions that already matched.
**How:** set `Object(..., section_rename=".text=.over")`. `tools/project.py` then selects the
`prodg_rename` or `ee-gcc_rename` rule. The rule compiles, then runs
`tools/rename_section.py -q $out .text=.over`. The script patches `.shstrtab` in place and renames
`.rela.text` along with `.text`. It accepts only names of the same length, so no offset moves. The
rules are separate so that the plain `prodg` command line stays byte-identical and no other object
is rebuilt. The renamed object is the build output itself, so the same object serves objdiff and,
once the unit is marked `Matching`, the link, whose script expects `.over`. After the fork point
upstream added an alternative, an objdiff-only renamed copy (`section_renames`). The fork took that
code over with upstream's `project.py` but does not use it, and its command line no longer matches
the rewritten `rename_section.py` (`<object> <old>=<new> ...`).

### Build-graph fixes (`tools/project.py`)

Three holes let ninja keep objects built from old inputs. The failure was silent: a broken DOL, or
an object that no longer compiled still being linked and measured.

- ngccc uses `-MD` instead of `-MMD`. GCC 2.95's preprocessor leaves every `<...>` include out of
  `-MMD` depfiles. After an edit to `snd/sndo.h`, three snd units were still linked from objects
  that no longer compiled.
- The split step now depends on `splits.txt` and `symbols.txt` (their paths are read from
  `config.yml`), not only on `config.yml`. It also declares no depfile: dtk never writes one, and the
  declared one kept the edge permanently dirty, in an endless configure → split loop.
- The `-keep` list (`config/GOWE69/keep.lst`) is an implicit input of the link, because it decides
  what survives `-strip-unused-data`.

Two more fixes. delink is configured independently of dtk: with `--dtk`, the PC split lost its
delink path, and the branch that builds delink from source pointed at `dtk.exe`. The PS2
target-object path now replaces only the `src` path component. A plain string replace turned
`sfsrc.c` into `sfobj.o`, and ninja could not build the graph.

### Small changes to upstream tools

- `tools/download_tool.py` skips the download when the tool directory is already populated, so
  parallel processes can share it.
- `tools/delink_to_config_json.py` starts delink with native path separators. Windows cannot start
  a relative path written with `/`.
- `tools/hasher.py` reads sources as Latin-1, because one literal contains the `™` byte. It strips
  comments before it scans `#include`s, because a quoted path in a comment broke the build. It also
  emits `#pragma once` under `EA_PRAGMA_ONCE_SUPPORTED`.

### objdiff options and name mappings

- The measuring scripts (`measure.py`, `fndiff.py`, `unitscore.py`, `libdiff.py`,
  `audit_zeros.py`, `missingcalls.py` and others) call `objdiff-cli diff` with both options that
  `report generate` uses: `-c functionRelocDiffs=none -c ppc.calculatePoolRelocations=false`.
  Without the second option, `diff` invents relocations for constant-pool accesses, and
  byte-identical functions read 94–99.9 %. This hid 48,348 B in 28 units, and agents were sent to
  "fix" functions that were already finished. `diffunidad.py` (mostly used for MIPS) and
  `litcheck.py` still pass only the first option.
- After every configure, `configure.py` writes `OBJDIFF_SYMBOL_MAPPINGS` into `objdiff.json`, which
  is git-ignored. The mappings cover three symbols whose names differ between the two sides: an
  address suffix added by the splitter, a local-class counter, and a renamed static initializer.
  Measured caveat: `objdiff-cli diff` and `report generate` do not apply them, so
  `scripts/measure.py` applies them itself.

### Measuring five builds: `scripts/pctall.py`

**Why:** `ninja progress` and the earlier scripts read only `build/GOWE69/report.json`. That cannot
tell real code from a match propped up by scaffolding. Only compiling the same source with the
other platforms' compilers can. Since 2026-09-17 the fork measures the way upstream does: one
objdiff report per build, no asm, and the `Matching` flag ignored as a metric. The flag still
decides which objects the GameCube link uses.

**How:** for each version (all five, or the ones named), the script:

1. runs `configure.py --version V`, passing the tool paths that already exist (Xbox 360 splits with
   `jeff.exe`, dtk's XEX variant);
2. runs `ninja -k 0`, retried up to three times because MSVC's `cl.exe` fails intermittently with
   C1083 under heavy parallel load;
3. on PC only, writes a copy of each object whose static functions contain MSVC 7.1 `$L<n>` labels
   (jump-table targets and the `int3` after a call that does not return) to
   `build/SPEED_EXE_1_3/sin_L/`, without those labels (`scripts/quita_L.py`). objdiff does not know
   the size of a COFF symbol and measures up to the next symbol, so it cut `_luaD_growCI` at 18 bytes
   instead of 56. Relocations that pointed at a label move to the section symbol with the offset
   added, so the copy links to the same bytes. The split exe has no such labels. The build object is
   not touched, and `scripts/bancopc.py` does the same;
4. runs `objdiff-cli report generate` with the project options.

The output is `build/versiones/<V>.json` and `<V>.log`, plus one line per build: matched code %,
bytes, functions, complete units, **the number of objects that fail to compile**, and units without
an object. At the end the script configures GOWE69 again. `--solo-informe` measures the existing
objects without compiling.

**Why the control is "0 objects fail to compile":** there are three ways to measure wrong, and all
three have happened:

1. objdiff counts a unit marked `complete` with no base object as 100 %. A build where half the
   tree failed showed a very high figure. pctall drops `base_path` and sets `complete: false` for
   every unit whose object is missing or failed.
2. If the split fails, `configure.py` leaves the previous version's `objdiff.json` in place. pctall
   checks that the paths in `objdiff.json` belong to the version being measured.
3. `configure.py --version V` can succeed and emit no compile rule at all when
   `build/<V>/config.json` is missing.

A figure counts only if its line says that 0 objects fail to compile. On GameCube, the SHA-1 of the
linked DOL (`config/GOWE69/build.sha1`) is a second control.

### Xbox 360 is measured on an LTCG link: `scripts/ltcg360.py`

**Why:** the Xbox 360 executable was linked with whole-program optimisation (`cl /GL` + `link /LTCG`).
Its Rich header lists 369 objects with product id 0x72 (`Utc1400_LTCG_CPP`), build 2110, plus the
linker `Linker800` with the same build. The XEX lists `D3D9LTCG` among its static libraries. The
compiler in `build/compilers/X360/14.00.2110` (cl 14.00.2110, link 8.00.2110) is that exact build.
Linking two small test files with it writes 0x72 only when `/LTCG` is used; without `/GL` the same
objects come out as 0x6e (`Utc1400_CPP`). LTCG inlines across units, propagates constant arguments
and allocates registers for the whole program. A unit compiled on its own therefore cannot produce
those functions. In the original, 333 of 672 cross-unit callees that carry a name (`bStringHash`,
`bStrLen`, `operator new(size, name, line)`, `Sim::GetTime`…) are never called from another unit.

**How:** for `EUROPEGERMILESTONE`, `scripts/pctall.py` runs `scripts/ltcg360.py` after ninja.

1. Every unit that `build.ninja` compiles with the `msvc` rule is compiled again with the same flags
   plus `/GL`, into `build/EUROPEGERMILESTONE/ltcg/il/`. A unit is only recompiled when ninja's
   object is newer than its IL object.
2. The tree does not define everything the program calls: functions not decompiled yet, and the CRT
   and XDK libraries, which are not available. Each missing symbol is defined in a small COFF object
   that the script writes itself, without `/GL`. LTCG cannot see into it and treats each one as an
   ordinary external call. Every stub function gets its own address, so that the split can tell the
   targets apart. `/FORCE:UNRESOLVED` does not work with `/LTCG` (LNK1120 stays fatal). The list of
   missing symbols is kept in `ltcg/stubs.txt` and extended by relinking while LNK2001/LNK2019 remain.
   Some of them, such as the `__savegprlr_N` helpers, only appear after code generation.
3. The link is `link /LTCG /XEX:NO /NODEFAULTLIB /FORCE:MULTIPLE /OPT:NOREF /ENTRY:main /MAP`. It
   takes about 1.5 minutes for the whole tree; a run whose stub list is already complete links once.
4. The linked `.text` is split back into one COFF object per unit (`build/EUROPEGERMILESTONE/ltcg/obj/`,
   same relative paths as `build/EUROPEGERMILESTONE/src/`). Each function goes to the object the
   linker map assigns it. Its relocations are rebuilt: `REL24` for every `b`/`bl` that leaves the
   function, `REFHI`+`PAIR`/`REFLO` for the `.reloc` HIGHADJ/LOW pairs, `ADDR32` for HIGHLOW. The
   relocated fields are zeroed, as in an object from `cl`.
5. objdiff compares those objects with the split original, as for every other build. The result is
   the official `build/versiones/EUROPEGERMILESTONE.json`. The per-unit measurement is still reported
   as a diagnostic in `EUROPEGERMILESTONE-fichero.json` and on its own output line.
6. Both sides are normalised in the same way before the comparison (class `Normalizacion` in
   `scripts/ltcg360.py`; a change of METHOD, decided by the user on 2026-10-07). LTCG places `.rdata`
   constants and jump tables for the whole program, so identical code used to come out different:
   - **Constant pool, compared by value.** After `lis rT, base@ha; addi rB, rT, base@l`, a load such
     as `lfs fX, d(rB)` has a displacement that depends on where LTCG put each constant. On both sides
     `d` is replaced by a number that identifies the value read from that side's own executable (size
     and bytes; `orig/EUROPEGERMILESTONE/NFS.exe` for the original). One table of numbers serves both
     sides and is one-to-one, so a different value gives a different instruction and the function
     still fails. Only non-update loads (`lwz lbz lhz lha lfs lfd`) qualify. The base must come from a
     relocated `lis`/`addi` in the same function, with no write to that register in between, and the
     address must fall in `.rdata`. A `lwz` that reads an address of the image (a vtable or a table of
     pointers such as `luaT_typenames`) is not a constant and is left alone.
   - **Jump tables, compared by destination.** Each table word is an absolute address inside the
     function. On both sides it is replaced by `b destination`, relative to the word itself, and objdiff
     compares a branch by its target inside the function. A bare value would be drawn as `<illegal>`,
     which objdiff treats as equal without comparing it. Our side takes the HIGHLOW entries of `.reloc`
     that point inside the function. The original has no relocations, so the words taken are the
     consecutive ones after a `bctr` that are aligned addresses inside the same function.
   - Anything that cannot be resolved is left as it was and keeps failing. The normalised originals go
     to `build/EUROPEGERMILESTONE/ltcg/orig/` and are the report's `target_path`. `build/<V>/obj` is not
     touched, and the per-unit diagnostic and the other four versions do not change.

   **Controls.** Before and after on the same link: +114 functions (38,348 bytes), 0 lost. A negative
   control used a copy of the linked executable with one constant changed on purpose (the first `lfs`
   of `RigidBody::ResolveWorldCollision`, 0.0 to 1.0). That function stops matching, and so do the
   other 78 that load the same constant. The same copy also had the first jump-table entry of
   `luaV_execute` pointed at the next case, and `luaV_execute` stops matching too.

Check of the split: in every unit, objdiff's 100 % agreed with byte identity under the
`nombra_por_codigo` normalisation in 4,795 of 4,796 named functions. The one exception was a function
that objdiff scored lower.

**Results (2026-10-07):**

- Per-unit compile: 3.49 % of the code, 4,617 functions.
- LTCG link: 4.28 % (360,336 bytes, 4,800 functions) before any renaming.
- LTCG link with 334 new names: **4.68 %** (394,120 bytes, 5,134 functions).
- Round 39: the STL with EA's allocator (`stlpc/xmemory`, also on Xbox 360), 4.77 % (5,209). Then 330 more
  names from `scripts/nombra_grafo.py EUROPEGERMILESTONE --nuestros=<LTCG objects>` (`--validar 0.3`: 843 of
  843 right), 5,539 functions. Then the normalisation of step 6, which adds 114 more:
  **5.53 %** (465,096 bytes, 5,654 functions).
- Round 40: `zSpeech` compiles for Xbox 360 and gets its own unit (`.text 0x824B65DC-0x824ECD50`; its 223 kB had
  been split into `zTrack`), `bMemCpy` becomes the CRT `memcpy` on Xbox 360 (the executable has 140 calls to the CRT
  `memcpy` and no `bMemCpy`), and 1,869 names: **7.92 %** (666,528 bytes, 7,592 functions). Linking `zSpeech` costs
  three small functions (80 bytes) that LTCG now specialises or moves: one gains a caller with constant arguments,
  one loses an `if` because every caller passes NULL, and a COMDAT moves to the unit that defines it first.
- Round 41: the Xbox 360 split redone. The LTCG link is per file (369 objects in the Rich header), so units are
  not given and have to be deduced: each original function was assigned to the only unit of ours with the same
  skeleton, ranges were built per unit keeping everything that already matched in place, and one `.pdata` range
  was emitted per `.text` range. It found `zEagl4Anim` inside `zDynamics`, a `zFeOverlay` unit, `zFe`/`zFe2`/`zFEng`
  crossed, and `zMain`, `zLua`, `zGameplay` and `zFoundation` almost entirely in unmeasured `auto_` pieces. With
  the names run to a fixed point: **13.48 %** (1,134,600 bytes, 12,344 functions), +4,671 functions without new
  code. The Xbox 360 front end is the console one, not PC's: turning its `WIN32` guards into `WIN32||XENON` is
  neutral or worse.
- Round 42: the split method became `scripts/trocea360.py` (`etiquetas`, `splits`, `prueba`, `banco`, `nombra`,
  `compara`, `aplica`). Two new naming tools: `scripts/nombra_vtable360.py` pairs each of our vtables with the
  original's (by the `lis` of the constructor, or by `.rdata` content) and names a slot only on a unanimous vote with
  identical code (2,906 of 2,937 known anchors right); `scripts/nombra_permuta360.py` fixes names swapped inside a
  unit (closed cycles only). The XDK's STL was built without checked iterators: `/D_SECURE_SCL=0` makes
  `_Tree::_Insert` return a one-word iterator as the original does (+107 functions). **17.62 %** (15,153 functions).
- Round 48 (owner's decisions applied): `/D_SECURE_SCL=0` re-measured by full A/B (all 30 units recompiled with
  `--cl=/D_SECURE_SCL=1`, which wins as the last `/D` on the MSVC command line): without it the link loses **1,135
  functions / 258,216 B** and gains 3 checked-iterator helpers of 128 B with no caller — the flag is a fact of the
  original XDK, now worth 10x what round 42 measured. The 18 AIVehicle vtables that round 47 left pending were
  re-verified with its own criterion (unique address, 1:1, full slot agreement with the r46/47 function names as
  anchors, zero contradictions) and the bag turned out to be **25** — the whole family (AIVehicle, Pid, Perp,
  Racecar, Pursuit, CopCar, Empty, Human, Helicopter, Traffic); the 4 base-class ones agree with PC's own symbols,
  the other 21 exist nowhere else. Applied as label renames, invisible to the link (bank +0/−0), plus the cascade
  tail `SetFlags@tCubic2D` (+1 fn surviving LTCG). **17.72 %** (15,260 functions).

The new names come from `tools/nombra_por_codigo.py --nuestros=build/EUROPEGERMILESTONE/ltcg/obj`. A
name was applied only when its code fingerprint is unique on both sides of the unit: no other
original function and no other function of ours has the same normalised code. Of 13,372 unnamed
candidates, 2,714 had code identical to one of ours. Most were ambiguous:

- 1,934 matched several of our functions;
- 103 names were proposed for more than one address;
- 229 were out of order and had no named call target in common;
- 45 had a twin in the original.

334 names remained, and all 334 score 100 % in the report.

**Limits:**

- LTCG decides with the whole program. With most of the game not written yet, it sees fewer callers
  and specialises more: about 90 functions that matched per unit (8 kB) come out different. For
  example, `bMatchNameWithWildcard` receives its first argument as a constant, because every caller
  in our tree passes the same string.
- About 110 small COMDAT functions (5 kB) are emitted once, in the first unit that uses them. The
  original has a copy in a later unit.
- The function order inside a unit follows LTCG's own order and is not reproduced. The comparison
  is by name.
- The `C4744` warning on `cSTICH_PlayBack::mQueuedSampleList` is the compiler's: two test files with
  a static data member array of a class template reproduce it. It is not a difference in the source.
- `Attrib::Alloc` is `inline __declspec(noinline)` on Xbox 360. It was a non-inline definition in
  every unit, which the whole-program link rejected as a duplicate. The original has one copy.
- PC is not LTCG. `speed.exe`'s Rich header has no 0x64 (`Utc1310_LTCG_CPP`, measured with the
  7.1 compiler); all its objects are build 3077 without LTCG. Its per-unit measurement stands.
### `config/<version>/`

These are the kinds of change; the data itself is not described here.

- **`GOWE69/splits.txt`** grows from 380 to 547 units:
  - many boundaries are taken from, or checked against, the original ELF's `.debug_aranges`,
    `STT_FILE` symbols and `.debug_pubnames`;
  - data ranges (`.rodata`, `.data`, `.bss`, `.sdata`, `.sdata2`, `.ctors`) are claimed by the
    units that own them, so those units can link;
  - it adds the `.over` ranges, and `align:N` where a section's alignment differs;
  - ranges are split where the splitter had merged code from different compilers.

  Two backups, `splits.txt.bak` and `splits.txt.bak_r9`, are committed.
- **`GOWE69/symbols.txt`**: splitter-made names (address-suffixed statics, `$LC<address>` pool
  labels) are replaced by real names or plain `lbl_` labels as units stopped naming DOL addresses.
  Sizes are corrected, and one symbol that another object references is global.
- **`GOWE69/keep.lst`** is the `-keep` list of `-strip-unused-data`, adjusted to our objects. Each
  entry that names a compiler pool label (`$LCn`) carries a directive with the string it must name.
  `scripts/lcfix.py` checks and re-resolves those entries. Comments say why each group must survive.
- **`GOWE69/config.yml`** has `block_relocations` for 15 `lis/ori` pairs that dtk read as pointers
  (chunk IDs and CRCs). Those pairs otherwise made two instructions mismatch forever (8 functions,
  1,676 B). Obsolete `add_relocations` entries are removed.
- **`EUROPEGERMILESTONE/symbols.txt`** has about 2,500 names merged address by address from the
  `xbox_stuff` branch (its code was not taken), plus names recovered by
  `tools/nombra_por_codigo.py`. Duplicate labels are removed.
- **`SPEED_EXE_1_3/symbols.json`** is rebuilt on upstream's file plus the MSVC-decorated names
  recovered from our compiled code. It keeps no demangled IDA text and no duplicate names.
- **`SLUS-21351/build.sha1`** is added.

## Tool catalogue

All scripts are Python 3, run from the repository root as `python scripts/<name>.py`. Their names, docstrings and output are in Spanish; `python scripts/indice.py` writes `docs/HERRAMIENTAS.md`, an index of the first docstring line of each (the fork's copy is not in the snapshot). They compute the repository root from their own location and write temporary files to `$SCRATCH` or `build/scratch`.

Unless a version is given, they work on GameCube and read:

- the extracted original objects in `build/GOWE69/obj/` and ours in `build/GOWE69/src/`;
- the original ELF, `orig/GOWE69/NFSMWRELEASE.ELF`;
- the line map, `symbols/debug_lines.txt`;
- the DWARF dump `symbols/mw_dwarfdump.nothpp`, made with `dtk dwarf dump` as upstream's README describes.

The most important tools have a second sentence.

### Measurement and progress

- `pctall.py`: the five-build measurement described above. It gives the figure that counts.
- `estado.py`: the GameCube front on one screen. It corrects two figures: objdiff's data % is really a proxy for how many units link, and the unit ceiling is 544, not 618, because 74 entries are splitter `auto_*` wildcards. It lists every non-matching function and what blocks each unit.
- `censo.py`: code not written yet, per unit and by bytes. It compares the splitter's function list with our symbol table, without objdiff, in seconds. A missing function written from scratch usually matches at the first compile, while polishing a 99 % near-miss can take 200 variants.
- `censohueco.py`: the recoverable PS2 gap per unit, from pctall's reports. It counts the bytes of functions at 80–100 %, how many of them are 100 % on GameCube, and how many are near-misses in both PS2 builds. If nearly all are 100 % on GameCube, the cause is the platform, not the source: 243 of 259 zAI near-misses led to the VU0 field masks.
- `measure.py`: independent per-unit measurement from the original and current objects. It sums the bytes of 100 % functions, applies the `objdiff.json` name mappings, and supports snapshots and `--cmp`.
- `pctsnap.py`: a per-function percentage snapshot and diff. It catches a function that falls from 95 % to 75 % without moving any `matched_code` byte.
- `unitmap.py`: per-unit percentage and total from `build/GOWE69/report.json`, with snapshot comparison.
- `unitscore.py`: (no docstring) a one-line summary of a SourceList: functions, matched bytes and near-misses at 70–99 %, with an optional name filter.
- `pct_ecs.py`: (no docstring) per-function match of one SourceList, filtered by name substrings. It marks missing symbols.
- `audit_zeros.py`: a per-unit objdiff audit of symbols of the original that we do not emit (MISS) and of near-misses at 90 % and 70 %.
- `loss.py`: ranks by bytes lost instead of by bad instructions, from `fndiff.py`'s cached diff. `miss` lists symbols absent from our object.
- `cheap.py`: (no docstring) near-misses at 60–100 % from the same cached diff, ranked by differing instructions per byte.
- `serie.py`: the real closure rate per work round, rebuilt from the round reports in `docs/analisis/`.
- `frozen.py`: freezes a unit's objdiff diff, without line numbers, as a SHA-256 in `docs/congelado/`, and checks it later. It catches code changes that do not move the percentage. The fingerprint also covers the `.debug` and `.line` sizes, so a "changed" verdict can come from debug info alone: confirm with `tools/fnobj.py`.
- `censorancios.py`: recompiles every promoted unit to a temporary directory and compares the ALLOC sections, relocations and symbol table with the object on disk, to find stale objects.
- `tools/sufijo_local.py`: keeps objdiff name mappings valid when our name carries a compiler label number
  (`...IconSort.26725`): a `*` pattern in `configure.py` is resolved against the object just built, only among
  defined symbols, and `pctall.py` also pairs other local classes whose names differ only in that number.
- `nombres_pc_upstream.py UP.json [--aplicar]`: takes a PC name from upstream's `symbols.json` only where our
  compiled PC objects define it and the current name is not defined (+931 PC functions the first time).
- `censoasm.py`: an exact census of the GCC inline asm left in the tree, with comments stripped. It classifies each use as a register pin, a scheduler barrier, real instructions, hand-placed data or a name alias.
- `auditaportable.py`: a census of what keeps the code from being portable decompilation: GNU asm and attributes, statement expressions, `typeof` and builtins. It also flags heuristic candidates for upstream's forbidden patterns: member access by pointer offset, and casts to the type a variable already has.
- `auditecho.py`: audits the "ceilings" documented in the playbook. Using the mnemonic multiset, it separates real allocator or scheduler walls from functions that still lack source.

### Building and fast iteration

- `build_direct.py`: compiles units by calling ngccc directly, in parallel, with each unit's cflags from `build.ninja`. It bypasses ninja, which loops regenerating its manifest when several processes touch `objdiff.json`. Two warnings. First, `--help` does not print help: it compiles every unit. Second, with several writers it can report "ok" for an object that does not match the source, so seal objects with a SHA-1.
- `bench.py`: one work cycle in 1.4 s instead of 34. It compiles the SourceList prologue plus the smallest prefix of files that one `.cpp` needs, with the unit's exact flags. It checks fidelity by comparing function sizes with the real unit object, adding earlier files until they agree, then measures with objdiff. The percentage agrees with the full unit to the fifth decimal. Headers are swept on scratch copies, so other people's builds are not disturbed.
- `sprobe.py`: compiles one `.cpp` of a unit to assembly with the unit's real flags, in 1–4 s.
- `tamfn.py`: compiles one file with its real flags and compares each function's size with the target, without ninja or `report.json`.
- `libdiff.py`: compiles and measures one middleware unit without ninja. It works even for a unit not yet registered in `configure.py`, taking the flags from a sibling unit. It shows per-function match and a side-by-side diff.
- `mn_repro.py`: compiles one `.cpp` with a unit's exact flags into scratch and keeps the object.
- `mn_forms.py`: applies a JSON list of alternative source forms at one spot, compiles and measures each, and always restores the file.
- `mn_sweepflags.py`: adds one flag per compile and prints a summarised diff of one function.
- `mn_diff.py`: diffs a unit's target function against any object file.
- `s_zlua.py`: compiles the prefix of `zLua.cpp`, up to a given include, to assembly.
- `zcmp.py`: compares that `.s` with the original's, function by function.
- `scratchclean.py`: deletes regenerable build and dump artefacts from the scratch directory. It is a dry run by default, and it never deletes sources, notes, or files cited in `src/`, `docs/` or `scripts/`. A full disk reads as "the variant changes nothing".
- `indice.py`: regenerates `docs/HERRAMIENTAS.md` from the scripts' docstrings, grouped by family.
- `mapaplaybook.py`: regenerates `docs/PLAYBOOK-MAPA.md`, the section → line-range map, so the playbook can be read by range. `--check` reports whether the map is stale.

### Diagnosing a function or a unit

- `previo.py`: shows what has already been tried on a function and with what result, from the experiment notes kept in source comments. Run it first.
- `triaje.py`: classifies each near-miss of a unit by the kind of difference:
  - registers only: permuter work;
  - missing instructions: a missing local or statement;
  - extra instructions: defensive code;
  - opcode changes: structure.
- `triage.py`: a second classifier. It compares the normalised instruction multiset (without registers, literals or immediates) to find code that is really missing or extra. It also measures how far a moved row travels: 20 rows or fewer is scheduler jitter, more than 60 is a real moved block.
- `fncmp.py`: lists which functions of a unit are not identical, opening the two objects directly, so it also works for middleware units without a SourceList. It gives detail per function.
- `fndiff.py`: (no docstring) side-by-side objdiff rows of one function, with both options. It caches the unit diff for `loss.py` and `cheap.py`.
- `audit.py`: proves that a function matches instead of trusting the percentage. It checks the bytes against the ELF at the address in `symbols.txt`, the relative branch targets and conditions, and the relocation types, addends and symbols.
- `callcheck.py`: the general detector of "the 100 % that lies". It finds calls to the wrong symbol, which objdiff with `functionRelocDiffs=none` does not compare: `__builtin_new` instead of `__builtin_vec_new`, a member declared in the wrong namespace, the wrong overload.
- `undefcheck.py`: finds every `REL24` call in our objects to a symbol that does not exist in the original ELF. The cause is bad mangling, a wrong namespace or overload, or a method that was declared and never written.
- `missingcalls.py`: finds functions where the target calls X and ours never does: a missing statement, or an inline that the original called out of line.
- `mnemo.py`: the mnemonic multiset of each near-miss. The same multiset means allocator or scheduler; a delta such as `stfs +1, lfs +1` names the missing statement.
- `arboljump.py`: compares the jump tree (each compared constant plus the branch that follows), which `mnemo.py` cannot see.
- `casetree.py`: compares the sequence of constants a switch compares, to find hidden `case`s or a moved pivot.
- `swsweep.py`: sweeps the tree for two switch patterns: shared case bodies that the original duplicates, and the inverted last leaf.
- `diffunidad.py`: writes the full objdiff JSON diff of whole units, for any version, into `build/diffs/<V>/` (tens of MB each). The pattern tools read it.
- `tabla_op.py`: a table of "what we emit → what the original emits" for same-position opcode mismatches, with how consistent each mapping is. A 99–100 % mapping is a direct fix. It found the PS2 VU0 field masks (`vmul.xyzw` → `vmul.xyz`, ...).
- `casi.py V [--unidad U] [--max-dif N] [--detalle N]`: triages near-misses by the number of differing rows, with the rows themselves. A function at 99.9 % adds no bytes (`matched_code` is all or nothing) but one that misses by two rows is worth its whole size.
- `banco.py V SOURCELIST REL COPY SYMBOLS [--diff] [--sombra=DIR]`: compiles a SourceList outside the tree with one or more of its `.cpp` files replaced by copies (and optionally a shadow directory of headers), and compares the named functions with the original. It only writes to `build/banco/` and cleans up, so it works while another measurement holds the lock.
- `ronda.py`: a bench of source variants over a frozen copy of the tree: whole SourceLists of one or several builds, in parallel, each variant compared function by function with the first. Several agents changing `src/` at once cannot mix into it.
- `mismo_codigo.py A.o B.o`: compares two ELF objects function by function, raw bytes (so a different `.rodata` offset counts): whether a change is neutral in an object objdiff cannot measure.
- `llamadas.py`: which functions call (or load the address of) a symbol, and how often, in our objects or in the originals of some SourceLists.
- `pc_x360.py`: checks that PC and Xbox 360 still compile some SourceLists, from the live tree or a `ronda.py` variant.
- `litcheck.py`: compares, per function, the multiset of constant-pool values with the target's. objdiff cannot see which constant an `lfs` loads. Its first pass flagged four functions, and all four had a wrong literal.
- `litpos.py`: the same check position by position, which catches two constants swapped with each other.
- `strseq.py`: compares the strings each function references by content. objdiff treats an `addi` to one string and an `addi` to another as identical, and NFSMW hashes names at runtime, so a wrong string fails silently.
- `reladdr.py`: gives the address each `HA16/LO16` relocation points to (containing symbol plus delta), to catch a reference to another copy of the same string.
- `relocfantasma.py`: finds functions that fail only because the splitter read a `lis/ori` constant as a pointer. The cure is a `block_relocations` entry in `config.yml`.
- `phantom.py`: the retired sweep for phantom `R_PPC_NONE` differences, whose cause was the missing `ppc.calculatePoolRelocations=false`. It is kept and reports zero.
- `deadreg.py`: finds accessors with an empty body (`{}` in a non-void method) by their signature in the asm: an instruction that reads a register nothing writes.
- `missinline.py`: ranks the inlines that the original expands and our tree lacks, or has with an empty body, by how many non-matching functions use them. It also prints per-function inline trees from the DWARF dump.
- `symtabdiff.py`: diffs the ELF symbol tables of target and ours. A missing definition points at a template instantiation that was switched off.
- `globalini.py`: compares each unit's `_GLOBAL_.I.<name>`. GCC 2.9 names it after the first global with an initializer, and a tentative definition never wins, so a mismatch is a 0 % that is only a name.
- `ghostref.py`: finds symbols we reference that never existed in the original, for example a non-pure virtual function that is never defined: the class has no key method, so GCC 2.9 emits no vtable.
- `stmtorder.py`: finds the source order of a block of stores to the same base, by trying orders and measuring each with objdiff.
- `derotate.py`: undoes the scheduler's rotation of runs of stores and proposes the statement order from `symbols/debug_lines.txt`.
- `pool2lit.py`: replaces `extern const float lbl_XXXX` references with the literal values. Measure the result file by file.
- `rodata.py`: (no docstring) prints the hex and float value of each symbol matching a regex in an ELF object.
- `tools/fnobj.py`: compares two objects function by function with `b`/`bl` displacements masked, because removing a function shifts every later branch. `--orig` checks the changed functions against the original.
- `tools/abobj.py`: A/B of two builds of a unit when a change may add or remove strings. `.text` and its relocations must stay identical, and the `.rodata` string set must not move away from the original.
- `tools/fnasm.py`: dumps the original's disassembly of named functions of a unit, from an objdiff diff.
- `corridos.py VERSION UNIT...`: for PC or Xbox 360, per class, the functions whose member offsets differ from the
  original's by a constant: a member of another size on that platform. One `objdiff-cli diff` per unit. It found
  the PC/Xbox 360 list limits (lesson 87).
- `capacidades.py [VERSION] [--nombres]`: the capacity of every template list (`FixedVector`, `Listable::List`)
  in the original against ours, read from the `GetMaxCapacity` their vtables point to.
- `bancopc.py VERSION UNIT LABEL [--sombra=DIR] [--sust=REL=COPY] [--base=LABEL] [--cl=OPTION]`: the PC and Xbox 360
  bench. It compiles one SourceList outside the tree, with shadow headers and substituted `.cpp` files, compares it
  with the split original and reports gains and losses against another run; `--cl` appends compiler options (with
  a dash: lesson 97 was found this way). The control run matches the official report. `todas.py` runs it over every
  unit in parallel (`--par` pairs each variant with a fresh control); `calor.py` and `corr.py` read its kept diffs
  for member offsets shifted across classes; `sondapc.py` asks the PC or Xbox 360 compiler for a class's size and
  member offsets; `pp_igual.py` proves a GameCube/PS2 change neutral by preprocessing with and without it.

### Register allocation and scheduling

- `regmap.py`: compares, local by local, the register the original compiler gave each variable (from the DWARF dump) with ours (from `dtk dwarf dump` on our object), and names the cause: an extra or missing local, or one living in another scope. It pairs by position and type and can invent permutations, so confirm with the DWARF before acting on it.
- `permuter.py`: searches automatically for the matching source form. It generates equivalent variants of one function, compiles them to assembly, scores them against the original and keeps the best (hill climbing). With `--guided`, `regmap.py`'s verdict chooses the transformations. It also offers annealing, whole-unit sweeps, and seeding from a hand-written body.
- `rtldump.py`: produces GCC RTL dumps (`.lreg`, `.greg`, `.combine`, ...) of a unit or a single `.cpp`, with the unit's flags plus extra ones. They are read against the SN GCC 2.95.3 source, kept locally under `orig/prodg/NGC_GNU_SRC/`.
- `lreg.py`: the compiler's own allocno table (`n_refs`, `live_length`) for functions stuck on register permutations.
- `alloc.py`: the real allocno table from `.greg`: only the pseudos that reach `global_alloc`, already in `allocno_compare` order, with the size factor. It corrects three measured errors of `lreg.py`.
- `regalloc.py`: dumps the allocno table of our code for any unit or library.
- `schedtrace.py`: the scheduler trace, cycle by cycle, with the ready list and functional unit, from `-fsched-verbose`.
- `pines.py`: a census of register pins (`register X asm("rN")`) and whether each one matters.

### PS2 layout probes

- `sondastabs.py`: compares the PS2 layout of every class described by the A124 `.mdebug` stabs with ours. The stabs give the offset and size of every field in bits, with qualified names. The script probes each class from the SourceList that compiles it and reports the first field that does not fit. First full run: 2,414 classes fit and 107 do not.
- `sonda.py`: asks the compiler for the real layout of a class (`sizeof`, alignment, offsets) with a version's real flags. It uses a probe file of `int sonda_*` globals and reads them from the `.s`. Compare the result with `symbols/PS2/PS2_types.nothpp`.
- `cotejo.py`: puts the A124 fields of a class next to our declaration and says which member is extra or missing, usually one that the A124 did not have yet.
- `desplazamientos.py`: groups loads and stores that differ only in displacement. A misplaced member breaks every function that touches the class.
- `alinvec.py`: finds vector members that are aligned to 16 in the PS2 original but not in our source, from the type dump. `sondastabs.py` and `cotejo.py` now also cover non-vector members.
- `attrfuera.py`: for the A124 only, moves the given attributes out of a generated AttribSys class's `_LayoutStruct` and makes their accessors read by key, as that build does. It was worth +31 SLES functions.
- `firmas.py [--version V]`: functions the original defines under a mangled name our object lacks, while ours defines the same `Class::method` with another signature. objdiff pairs by name, so these read 0 % until the signature matches.
- `argsusados.py <version> <unit> <symbol>`: the argument registers a PS2 function reads before writing them. It decides whether SLUS has a parameter its inherited A124 name lacks.
- `sinreturn.py [--slus]`: compiles the PS2 units with `-Wreturn-type` and lists non-void functions that can end without a `return`, most reached first.
- `patrones.py [--version V]`: adds up the differing rows of every near-miss function of a PS2 build as opcode pairs (original -> ours) and counts the functions that share each pair. A pair in hundreds of functions is one root cause.
- `gcmismo.py <tag> <unit> [...]`: compiles GameCube units from the working tree and compares them, function by function with relocations, to the objects of the last GameCube build. It guards source-form changes that only PS2 can see.
- `ficheros_mdebug.py [unit ...]`: the source files of every A124 unit in link order, from the `.mdebug`, and whether each SourceList includes them.
- `banco.py V SOURCELIST REL[,REL] COPY[,COPY] SYM[,SYM]|@FILE [--diff] [--sombra=DIR] [--conservar]`: compiles a game SourceList outside the tree (in `build/banco/`) with some of its files replaced by copies, and diffs the given functions against the original object of GameCube, the A124 or SLUS. It touches neither `src/` nor the objects a measurement uses, so variants can be proved on the three builds while another measurement holds the lock. `@FILE` reads the symbols from a file (a whole unit does not fit in a Windows command line); one objdiff call covers them all, so a 500-function unit takes seconds. The overlay units (`zOnline`, `zFeOverlay`) get their `.text` renamed to `.over` as the build does. It deletes its SourceList copy, preprocessed file, object and JSON when it ends (`--conservar` keeps them): runs from several agents once filled the disk with 19.5 GB. A `--sombra` directory must hold the whole header directory, not one header: a header another header includes by a relative path is read from the tree.
- `inlines.py global [MIN] | usos HEADER L0 [L1]`: which header inline is to blame for a family of A124 near-misses (lesson 72). `global` ranks every (header, line) that our PS2 objects expand by the fraction of those functions that match; `usos` lists the functions that expand some lines, with their percentage.
- `relops.py lineas SYM[,SYM] | difieren`: the `stl_relops.h` oracle (lesson 71). `lineas` gives, for the original and for our object, every expansion of the global `operator!=`, `>`, `<=` or `>=` with the source line it follows; `difieren` lists the functions where the two sides do not expand the same operators.

### PS2 names

objdiff pairs functions by name, so every `func_XXXXXXXX` in a PS2 `symbols.txt` is a function that cannot score, whatever our code does. SLUS has no symbol table: most of its names were inherited from the A124 by address, and several landed on the wrong bytes.

- `tools/nombra_ps2.py`: proposes names for unnamed PS2 functions whose code equals, with relocations masked, a function our object defines. Every proposal still needs a proof.
- `nombra_symtab.py`: the names the A124's ELF symbol table has and `symbols.txt` lacks, mostly statics; it checks each address is a real entry before adding it.
- `prueba_nombre.py V 0xADDR NAME [NAME...]`: the exact proof of a name, word by word, comparing registers in relocated words too (`bRandom__Fi` and `bRandom__Ff` differ only in the seed's register).
- `grafo_nombres.py`: names for SLUS from the A124's call graph, through functions with the same name, size and call positions in both builds. No proposal is a proof on its own.
- `eventos_vtable.py`: the event classes of `zMain` anchored by the string their `GetEventName` returns, and from it their vtable, destructor and constructor.
- `restos_epilogo.py` and `restos_trozos.py`: names sitting on the 8-byte leftovers (an epilogue's `addiu $sp`, or one loose instruction and a `nop`) in front of their function; they move the name to the entry every caller jumps to.
- `cortes.py V`: names that fall inside a function (on a branch target), which split it in two. Run it after every batch of names.
- `aplica_nombres.py V list.tsv [--aplicar]`: applies proven `(address, name, reason)` lists to a PS2 `symbols.txt`, and first checks the batch is consistent: an address with two names or a name with two addresses is dropped, and a name that already sits elsewhere is moved (the old address goes back to `func_`). Statics that repeat across units get `allow_duplicated` on every line.
- `orden_slus.py`: compares, unit by unit, the order of SLUS's functions with the A124's (named by its `.symtab`). Where sibling functions have the same code (destructors that differ only in their vtable), an inherited name can land on the neighbour; code comparison cannot see it, order can.
- `vtable_dtors.py`: anchors PS2 destructors by their vtable (an EE-GCC vtable's first entry is the destructor, and the destructor stores its own vtable), and names the class from the other methods in that vtable.

### Readers of the original's debug data

GameCube DWARF-1 and line map:

- `dwarf1.py`: the DWARF-1 oracle. It parses the 92 MB `.debug` of `NFSMWRELEASE.ELF`, emitted by "GNU C++ 2.95.3 SN BUILD v1.76", for locals with name, type and register, lexical blocks, and the inline tree. The format was verified against the producer's `dwarfout.c`.
- `orac.py`: the same data by grep over the `dtk dwarf dump` output, in about 1 s instead of about 30 s: functions, locals and registers, inlines, structs.
- `libdwarf.py`: the DWARF of a middleware unit rendered as C, with prototypes, parameter and local registers, and structs. The SourceList tools do not cover libraries.
- `dwbody.py`: diffs the DWARF body (locals, inline tree, blocks) of the original and ours, per function. One pass found structural differences in 13 of 32 near-misses.
- `dwblocks.py`: compares block and inline ranges, normalised to the start of the function.
- `dwarfmap.py`: gives the owning unit of every data symbol, global or local, from the DWARF.
- `pubnames.py`: maps every public symbol to its compile unit from `.debug_pubnames` and `.debug`.
- `aranges.py`: the authoritative unit boundaries from `.debug_aranges`: 316 units with four ranges each, with the ends taken from each compile unit's low/high pc. Control: 299 of 300 `.text` starts coincide with `splits.txt`. It found over a hundred boundaries that `splits.txt` lacked, mostly in `.rodata`, `.data` and `.bss`.
- `lmap.py`: shows the target's asm with the source line of each instruction next to it, from `symbols/debug_lines.txt`.
- `fuse.py`: fuses the original disassembly with EA's line map. With it, 58 of 60 GManager functions were written at 100 %.
- `plan.py`: a reconstruction plan for one function: asm, source line, and the DWARF inline tree, blocks and locals, per instruction.
- `zdump.py`: the original disassembly of several zLua symbols, fused with line info.
- `tools/zlua_annot.py`: annotates zLua asm with DWARF line info.
- `tools/zcam_audit.py`: prints the first address of each Camera source file inside the zCamera range, from the line map (a file-order audit).
- `tools/dwarfclase.py`: DWARF similarity of all functions of the given classes: parameter and local names, blocks, local constants, inlines. It batches upstream's `tools/dwarf-compare.py` and sorts by similarity.

PS2 and Xbox 360:

- `mdebug.py`: reads the `.mdebug` (ECOFF) of the A124 `NFS.ELF` from its SYMR table. Per function it gives locals with type and register, the `$LBB`/`$LBE` range of each inline, and the expansion order.
- `arbol.py SYMBOL [OBJ] [--diff]`: the tree of blocks and locals in the A124's stabs for one function (register or stack for each local), ours next to it with `--diff`. It shows what the original declares in each scope; see lesson 84 for what the stabs leave out.
- `ps2fn.py`: shows what a function does in the A124 build: its symbol in `NFS.MAP` and its ordered list of calls. The MIPS code is read, not copied.
- `ps2map.py`: reads the A124 linker map. Per object, it sorts symbols by address, which gives the emission order of that build.
- `x360ref.py`: a GameCube ↔ Xbox 360 cross-reference over the decrypted basefile: where a string is, which function contains it, and that function's asm.

### GameCube splits, symbols, keep list and data

- `checksplits.py`: validates `splits.txt` before re-splitting and reports every problem at once, such as overlaps and ranges that end inside a symbol. The splitter stops at the first one.
- `rangechk.py`: checks `splits.txt` boundaries against the ELF's 577 `STT_FILE` entries, which say which unit owns each local symbol.
- `claimdata.py`: claims for a unit the data that the splitter left without an owner, by finding our bytes, already correct, in the unclaimed data.
- `claimlbl.py`: reads each unit's `.rodata` address from objdiff's `$LCn` ↔ `lbl_` pairing instead of searching for bytes.
- `claimrange.py`: proposes, from `symbols.txt` names, the `splits.txt` ranges that a library unit must claim, and checks for collisions.
- `frontcoste.py`: for each boundary that `aranges.py` proposes, shows what would change owner (symbols, non-zero bytes) and at what cost.
- `prefijochk.py`: checks that no `.rodata` range contains two unit prefixes. The bWare prefix string marks the start of each unit's `.rodata`, and the ELF has 31 of them.
- `prefijotu.py`: lists, in order, the shared vocabulary of strings that a unit lacks.
- `keepchk.py`: checks that every `object:symbol` entry of `keep.lst` exists. A stale entry is stripped silently.
- `lcfix.py`: re-resolves the `$LCn` entries of `keep.lst` by content, and the `# @data UNIT OFFSET` ones by position in `.data`. The number `n` shifts when a literal is added earlier in the unit, and then the wrong string is stripped silently. Read its `FALLO` lines, not only the `CORRIGE` ones.
- `lcmap.py`: pairs the `$LCn` pool that cc1plus emits with the extracted object's `lbl_XXXXXXXX` labels, relocation by relocation.
- `lcpool.py`: shows the `$LC` pool that we emit and the original object does not have.
- `deadstr.py`: finds strings that we emit and the target has, but the linker strips. `-strip-unused-data` removes most of a dead symbol (`size & ~7` bytes in 8-aligned sections).
- `deadlink.py`: the same question, answered by the linker. It links twice, with and without keeping all our `.rodata`, and compares.
- `dupdata.py`: finds data written by hand in an `asm()` that the compiler also emits.
- `dupstr.py`: finds strings that we emit more times than the target, by content, and marks the copies that `keep.lst` forces.
- `rodatagaps.py`: maps the dead-data gaps between live literals, paired by relocation.
- `rodorden.py`: shows the permutation of a unit's `.rodata`, by strings.
- `residuo.py`: measures how much of the target's `.rodata` is leftovers: the tails (`size % 8` bytes) of symbols that the linker stripped.
- `datadiff.py`: diffs a data section byte by byte, with resynchronisation, to show which strings and constants are missing or extra.
- `datacmp.py`: measures how much data is already right, by bytes. objdiff's data percentage pairs by name and reads 0 % for units whose data is correct.
- `seccdiff.py`: compares the size of each of our sections with the extracted object, separating SourceLists from libraries.
- `extrasym.py`: lists the symbols we emit that the extracted object lacks. It separates instantiations that the original emitted once in another unit, which are stripped at link and harmless, from the rest.
- `vtdup.py`: lists vtables and other named data that we emit in several units (classes without a key method).
- `mangfix.py`: re-resolves the mangled aliases of local classes. Their numeric suffix comes from a per-unit declaration counter and drifts.
- `tools/harvest_strings.py`: harvests `.rodata` strings and links each one to its consumer, a function or a named `.data` pointer. It writes `tools/string_map.csv`, `tools/string_globals.csv` and `tools/string_map_summary.json`. It closed the "asset names" idea with a measured negative: the game hashes names at runtime.
- `tools/zlua_str.py`: reads strings from the DOL and prints the Lua bind-call sequence of an address range.
- `tools/ucom_gap_report.txt`: a saved report of the unmatched symbols that the DWARF attributes to `UCOM.h` and `UTLVector.h`.

### GameCube link and DOL checks

These tools served the period when the metric was `linked`: how much of the DOL is built from our objects. They are still how a GameCube link is checked.

- `linkdelta.py`: links the whole project, replacing one unit at a time, and compares the resulting ELF's sections with the base link. Object sizes mislead, because `-strip-unused-data` removes whatever is unreferenced: zLua's object had +18,908 B of `.text` and linked identically.
- `trypromo.py`: tests a promotion without touching the tree. It rebuilds the link's object list from `build.ninja`, swaps in our objects, links to scratch, and compares the DOL with the original.
- `dolwhere.py`: links with a promoted unit and lists the address ranges where the DOL differs, with the symbol at each.
- `dolrod.py`: diffs a linked section inside a unit's window: what the linker kept, not what the object has.
- `desplaza.py`: measures the real displacement that a unit imposes on the rest of the link, from the addresses of the symbols that follow it. The linker aligns the start of `.data`, `.bss`, `.sdata` and `.sdata2` to 32, so a size comparison can hide up to 31 B.
- `movidos.py`: lists which symbols land at another address when a unit is linked in.
- `stripmap.py`: splits each unit's DOL deficit into strip damage and data that is really missing. It links three times.
- `stripped.py`: lists data that the original emitted and the linker discarded. Such data must be emitted and left unreferenced.
- `gapchk.py`: finds hand-written gaps with `.size` that `-strip-unused-data` removes whole.
- `refs.py`: shows which of a unit's extra symbols are referenced, and from where. Unreferenced ones are stripped.
- `undlist.py`: lists the undefined symbols that block a promotion, grouped by symbol and by the object that references them.
- `promote.py`: the pre-check before marking a unit `Matching`, done by reading both ELFs. It checks the section set, symbol mismatches, and relocations to address-suffixed symbols (promote the whole cluster or none of it).
- `promodist.py`: shows what each promotion costs and how many bytes it unlocks.
- `antesdegradar.py`: shows what demoting a unit to `NonMatching` will change: section size, section alignment, undefined symbols, dead data.
- `permorden.py`: gives the exact permutation of a unit's `.text` emission order, as the longest increasing subsequence plus cycles.
- `reorden.py`: a counterfactual: would the unit match if it emitted in the target's order?
- `textorder.py`: compares the order of `.text` functions, which objdiff does not see. A unit at 100 % can still link a different DOL.
- `vtord.py`: shows the emission order of a unit's vtables side by side, following the walk that `finish_file` does.
- `parseord.py`: measures the class parse order with `-E`, in seconds. Vtables are emitted in the reverse order of class completion.
- `vtable_audit.py`: audits vtable size and slot order against the original's objects, and names the slots from the PS2 type dump.

### Publishing

- `foto.py`: prepares a clean snapshot branch, `avance-YYYY-MM-DD`, of the current tree without the local history. It pushes nothing. The snapshot keeps upstream's root layout and upstream's documents, plus `scripts/` and this guide. `port/`, `ghidra_scripts/`, `tools/scratch/` and the fork's own documents in `docs/` stay out. It refuses:
  - files over 100 MB;
  - game formats, detected by extension and by binary signature;
  - text files that embed a binary: more than 4 KiB with more than 4,096 hex byte literals (`0x..,`), unless upstream has the same file;
  - token-like secrets;
  - local machine paths.

  `--prueba` runs every check without creating anything. The checks are strict because two snapshots had to be deleted: the first published game files from `port/`, and a later one the game's `globala.bun` resource bundle, embedded in the source tree as a C array (it is now only declared, see the platform section).

### Ghidra

Stock Ghidra reads little of the GameCube ELF. Two patches change that; both are in `tools/ghidra/`, whose `README.md` gives the steps (measured with Ghidra 12.1.2):

- `tools/ghidra/ghidra-dwarf1-gcc-ext.patch`: for the ghidra-dwarf1 extension (emoose's fork, commit `5b0666c`). GCC 2.9x emits six DWARF-1 attributes of its own (0x8000–0x8050), outside the user range, and the extension aborted at the first entry; the patch declares them and makes unknown attributes and tags non-fatal. Functions with their DWARF signature go from 86 to 13,824 (of 13,950 importable), structures from 11 to 4,076, enums from 2 to 2,872.
- `tools/ghidra/gekko_750cl_ghidra12.py`: adds the Gekko/Broadway language (PowerPC 750CL with paired singles) to Ghidra 12.1.2, which does not ship it. Four language files are copied unchanged from Ghidra's own sources at commit `e2f20dab` (April 2025; no longer on master); the script adds the `ppc.ldefs` entry and ports the 750CL parts of that commit's `ppc_common.sinc` behind `@ifdef 750CL`, so the other PowerPC languages compile as in stock 12.1.2. On the stock files its output is byte for byte the one this project's analysis used, with which all 8,627 paired-single instructions of the ELF decode; without it, the disassembly stops at the first one and the function is cut there.
- `tools/ghidra/SetGqr.java`, an import pre-script that sets GQR0 = 0 (NFSMW only ever writes 0 to it) so that `psq_l`/`psq_st` decompile without a branch per quantization type, and `tools/ghidra/Dwarf1Run.java`, which runs the DWARF-1 analyzer headless (it is not enabled by default) and writes a census before and after.
- `tools/ghidra_decomp.py`: decompiles functions headless and read-only from the Ghidra project of the GameCube ELF (`ghidra_project/NFSMW`, not versioned). It accepts a name, a GCC 2 unmangled name, or an address. `--info` summarises the program and searches by substring. It needs the Ghidra version that saved the project (12.1.2 here) and a JDK 21, and it fails if the GUI has the project open. It runs `tools/ghidra/DecompilarFunciones.java` and `tools/ghidra/InfoPrograma.java`.
- `ghidra_scripts/` (in the fork's working tree, not in the published snapshot) keeps the exporters of the analysis described in `docs/GHIDRA_NFSMW.md`: `ExportOpen.java` and `ExportDecomp.java` bulk-decompile an `addr,size,label` list (ExportOpen also writes the listing with hex words and a coverage summary), `ExportSymbols.java` exports the symbol table as CSV, `ExportWalls.java` and `ExportWalls2.java` dump callers, callees, references and decompilation for a fixed list of 28 of the hardest functions (the "walls"), and `DumpInfo.java` prints a program summary.

### Xbox 360 and PC names

- `quita_solapadas.py IN OUT`: removes from the PC symbol file the IDA relocations that sit one byte before a real
  one with the same target (1,748 of them, all in `mov dword ptr [reg+disp8], imm32`); objdiff zeroed four bytes of
  the original for each.
- `int3_noreturn.py IN OUT`: adds to the PC symbol file the `int3` MSVC emits after a final call to a `noreturn`
  function (lesson 98).
- `pdb.py FILE.pdb [resumen|publicos|modulos|json OUT]`: a dependency-free reader of Microsoft PDB files (MSF 7.00):
  streams, DBI, sections, modules and public symbols, to bring PC names from a PDB.
- `dtors_por_vtable.py`: names SLUS destructors from the vtables that point to them (55 in its first pass).
- `nombra_grafo.py EUROPEGERMILESTONE|SPEED_EXE_1_3 [--aplicar] [--validar F] [--anclas-malas]`: names by code AND by
  the call graph. The report compares with `functionRelocDiffs=none`, so a relocation's target does not count: an
  unnamed original function matches as soon as it carries the name of one of ours with the same skeleton (the code
  with relocated fields zeroed). The targets are used as evidence instead: a named target must agree, `__real@` and
  `??_C@` targets are checked against the original's bytes, and an unnamed target takes the name the accepted pairs
  call there, which also breaks COMDAT ties (the `reserve`/`push_back` of each `Vector<T>`). Hiding 30 % of the known
  names and asking for them back gave 904/905 on Xbox 360 and 2,546/2,546 on PC; its first run named 828 functions on
  Xbox 360 and 2,475 on PC. The other side of `functionRelocDiffs=none`: a WRONG name on identical code also scores
  100 %; `--anclas-malas` lists names whose targets contradict each other. Re-run it after each batch.
- `nombra_elim.py [--aplicar] [--solo-1a1] [--quitar MARK] [--tsv OUT]`: names PC functions by code, using the
  linker's own rules. It looks at each unnamed function of the executable. Here "unnamed" means a name MSVC 7.1 cannot
  emit: `sub_`, an IDA demangle, a GCC decoration imported from GameCube, or a name with an address glued on. It pairs
  that function with one of ours that has the same skeleton (the bytes with the relocated fields zeroed). The candidate
  must come from the SAME unit, and in that unit it must be the only one left after elimination. Three rules make the
  difference with `nombra_grafo.py`:
  - **First-definer rule.** MSVC places a COMDAT with the first object, in link order, that contributes it. An inline
    or template function of ours can therefore only be in the first unit whose object defines it. This held for 944
    of 957 known names with several definers.
  - **Elimination.** A function of ours whose name is already in `symbols.json` is not a candidate anywhere.
  - **Relocation targets compared by address.** IDA gives one address two names (`??3@YAXPAX@Z` and
    `?Free@FastMem@@QAEXPAXIPBD@Z`), so comparing by name reported false contradictions.

  It applies two rules:
  - `elim-1:1`: one original function and one of ours share the skeleton.
  - `elim-orden`: N and N identical copies. They are assigned in order: address in the executable against definition
    order in our object. A pair is kept only when the function lies between its neighbouring anchors.

  It also has these filters and checks:
  - A function under 16 bytes that has relocations needs at least one confirmed target. A `push; call X; ret` wrapper
    has the same skeleton for any X.
  - Each pair is checked with `objdiff-cli diff` on a renamed COPY of the split object, and must score 100 %.
  - Validation is mandatory. It hides 30 % of the known identical-code names and asks for them back. It refuses to
    apply if `elim-1:1` falls under 99 % or `elim-orden` under 95 %. The first run got 2,755 of 2,756 and 359 of 375.
  - Every applied name is logged in `config/SPEED_EXE_1_3/nombres_procedencia.tsv`: address, previous name, new name,
    rule, mark, size, confirmed targets and date.
  - The `elim-orden` names carry the mark `copia-ambigua`. The code is identical, but the name may be swapped with
    another copy of the same code. `--quitar copia-ambigua` restores all of them at once.

  First runs (2026-10-07):
  - **First pass:** 2,110 + 5,195 names, PC 13,106 → 20,411 functions (+266 kB, 23.4 → 29.6 % of code).
  - **Second pass, after the `zFeOverlay` unit below:** 510 + 214 more, of which 373 are in `zFeOverlay`; with the
    unit, PC reaches 21,141 functions (30.4 % of code).
  - **Third pass:** 22 + 29 more.
  - **Validation on the third pass:** 3,998 of 3,998 and 1,454 of 1,458.

  A wrong name on identical code still scores 100 % (the report uses `functionRelocDiffs=none`). That is why the
  provenance log exists. Re-run the script after `nombra_grafo.py` each batch.
- `nombra_corrige360.py [--aplicar]`: the Xbox 360 counterpart of `nombra_corrige.py`. It fixes wrong names from the
  callers' evidence and requires objdiff at 100 % on the LTCG objects (validation: 1,446 right, 10 `??_E`/`??_G`
  aliases or folded code). On Xbox 360, `nombra_grafo.py` treats the `ltcg_XXXXXXXX` targets of the LTCG split as
  anonymous: they were 1,137 of the 1,291 "bad anchors".
- `tablas_pc.py`: IDA left the jump tables MSVC places after a `switch` outside the function. Putting them back in
  331 functions added 18,219 bytes and 2,337 relocations to the comparison, and 34 functions reached 100 % without
  touching code.
- EA's libraries are not built with the game's `/O2` on PC. `eathread` (its Win32 back end, 19 of 19 functions)
  uses `/O1 /Oy-`, `path` uses `/O2 /Oy-` (+110 functions) and `spch` and `snd` use `/O1` as C++ (`/TP`; +37 for
  `spch`). `path` compiles for PC once `eathread` has its Win32 back end. Round 41 added `snd` as 49 PC units
  (`/O1 /TP`, +95 functions) and `csis` with its Win32 lock (`CreateMutexA`, `WaitForSingleObject`,
  `ReleaseMutex`; +44), and the PC front-end renderer on D3D9 (`eViewPlatInterface::Render` with mask and stencil,
  `GetVisibleStateSB`, `GetScreenPosition`, `ePoly`, the cube environment map and PC lights). Round 42 added the
  DirectSound driver of `snd` (`library/pc/sdsound.c`, 15 of 19 functions; the `DSOUND_*` names are ours, no build
  carries symbols for it) and showed that PC uses EA's STL where GameCube uses UTL: the `lightmaterials` map is a
  `std::map` whose allocator pushes the string "STL".
- `lado.py`, `huerfanos.py` and `rap.py` (GameCube): the line of each instruction, ours against the original
  (`--lineas` lists the gaps, to compare with a sibling block elsewhere in the source); comparisons no branch reads,
  following the branch graph; and a one-function compile with the real flags from `build.ninja`.
- `fa.py`, `flip.py` and `ldlr.py` (PS2): switch off each `EA_BUILD_A124` guard of a function body and measure the
  A124 (several guards inherited from other branches turned out to be false); find guards that belong to both PS2
  builds; and find members whose alignment shows as `ldl`/`ldr` pairs.
- PC now has the `zFeOverlay` unit. The executable links it after `zWorld2`, at 0x7A1760–0x7C3F39, inside what
  `splits.json` called `unassigned_02`…`_07`; those four units keep their `.rdata` slices. Our SourceList builds with
  MSVC unchanged. `zOnline` stays unassigned: its PC code (lobby, client and server) has no source in this tree.
- `nombres_pc_upstream.py` now reads upstream's `symbols.json`, which writes addresses as hexadecimal strings. Before,
  it matched nothing and reported "0 of 0" without an error.
- Member access is part of the name on PC and Xbox 360: MSVC encodes public/protected/private in the decorated name
  and GCC does not. Putting back the original access labels in 45 headers (witnesses: upstream's PC names and the
  A124 stabs, which record access per member and agree on 335 of 366) gained 153 PC functions and left GameCube and PS2
  byte-identical. The GameCube DWARF does not record member access (ProDG's `dwarfout.c` only writes it for bases).
- `relleno_ps2.py V [--unidad U] [--aplicar]`: names `pad_01_<addr>_text` the PS2 `func_` that are not functions (dead
  copies of the previous function's last instruction, outside every sized symbol and referenced by nobody). They still
  count in the total; the name only says there is nothing to decompile.

- `tools/nombra_por_codigo.py`: recovers function names in the Xbox 360 and PC executables, which have no symbols. For each unit, an original function without a name (`fn_`/`sub_`) takes the name of one of ours when their bytes are identical, unique in both directions, and in order. For relocated words it compares only the opcode on PowerPC, and it ignores those bytes on x86. `--aplicar` writes the names to `config/<V>/symbols.txt`. Why: objdiff pairs by name, so those builds hit a naming ceiling, not a code ceiling. The first passes took X360 from 1.33 % to 1.85 % and PC from 6.75 % to 9.16 %. It skips compiler-internal labels (`$L…`), one of which once broke a whole report. Re-run it after each batch of decompiled code.

### A PDB for the PC executable (`scripts/simbolos_pc.py`)

`python scripts/simbolos_pc.py [--prueba] [--sin-pdb] [--sin-tipos] [--sin-historia] [--raiz-fuente RAIZ]
[--lld EXE] [--exe EXE] [--english]` turns what the project knows about `speed.exe` 1.3 into symbol files that debuggers and
disassemblers read. Everything goes to `build/SPEED_EXE_1_3/simbolos/`, which is not committed. It reads the tree as
it is; re-run it after each batch. The full guide, for users of the PDB as well as for whoever regenerates it, is
`docs/PC_PDB.md` (in the working tree, and inside the PC PDB kit, `nfsmw-pc-pdb-kit.zip`).

Inputs:
- `config/SPEED_EXE_1_3/symbols.json`: names and function ranges.
- `config/SPEED_EXE_1_3/nombres_procedencia.tsv`, plus the git history of `symbols.json`: where each name came from.
- `build/SPEED_EXE_1_3/report.json`, and `splits.json` for what the report lacks: the unit and match percentage of
  each function.
- Our PC objects. They are already built with `/Z7`, and MSVC 7.1 stores line numbers there as classic COFF line
  records (`.bf` base line plus `.file` symbols), not as CodeView.

Line tables and CodeView are taken only from functions at 100 % whose bytes equal the executable's, with relocated
fields masked. The code is identical, so each line's offset inside the function is valid in the original.

Output files:

| File | Contents | Use |
|---|---|---|
| `NfsMWCollectorsRelease.pdb` | publics, a procedure per named function, and lines. Matched functions also get the CodeView of our objects: prototype, parameters, locals, and the types, `S_UDT` and globals of 27 units. | Debuggers load it by itself (below). |
| `speed.map` | MSVC linker map: `Publics by Value`, `Rva+Base`, `Lib:Object` = unit | IDA: File > Load file > Map file |
| `simbolos_pc.json`, `.csv` | one row per symbol | scripts, spreadsheets |
| `lineas_pc.json`, `.csv` | address → source file and line of the matched functions | tools without PDB support |
| `ida_importa_simbolos_pc.py` | names, missing functions, comments, source lines (`add_sourcefile`/`set_source_linnum`) | IDA 7.4+: File > Script file |
| `ImportaSimbolosPC.java` | the same for Ghidra, in Java (no Jython or PyGhidra needed); lines go to the source map (11.3+) | Script Manager, or `analyzeHeadless … -postScript ImportaSimbolosPC.java <folder>` |
| `speed.exe.dd32` | x64dbg database: labels, function ranges, comments | copy to x64dbg's `db/` folder |

Each row of `simbolos_pc.json` holds:
- the decorated name, the undecorated name and the short name (from `build/compilers/Win32/7.1/undname.exe`);
- the address, RVA, section, `section:offset` and size;
- the unit, the SourceList and our source file and first line;
- the match percentage and whether the bytes were verified;
- where the name came from: `pdb`, `upstream`, `grafo`, `codigo`, `elim 1:1`, `copia-ambigua` or another rule of the
  log, with the round that set it.

IDA invents names for what it does not know (`sub_`, `dword_`, `aString`, `FID_conflict:`…). Rows with those names
are kept but marked `real: false`, and they are left out of the map, the scripts and the PDB.

How the PDB is built. Nothing is downloaded: the Android NDK's `ld.lld.exe` is copied as `lld-link.exe` (LLVM's COFF
linker, which writes PDBs) and given synthetic objects:
- one object per unit, whose `.text$<address>` pieces have 1-byte alignment and reproduce the executable's layout
  exactly. The script checks the RVAs of `.text`, `.rdata` and `.data` after linking;
- symbols at the original addresses;
- a C13 `.debug$S` with line blocks;
- the `.debug$T` and the CodeView records of our objects. Relocations are retargeted to the original function, or to
  a named global. Records that cannot be retargeted are dropped, together with their scope.

The PDB then receives the GUID and age of the executable's own `RSDS` record (`{9B4BA937-…}`, age 9,
`NfsMWCollectorsRelease.pdb`), so the original executable needs no patch. An installed copy of the game was checked
too: its `speed.exe` has the same record and the same `.text`, `.rdata` and `.data`, and only `.rsrc` differs. Put the
PDB next to the executable or in the symbol path. Debuggers that use `dbghelp` (WinDbg, x64dbg, Visual Studio) then
load it as a matched PDB. IDA can load it with File > Load file > PDB file; that path was not tested here.

First run (2026-10-07):
- 26,684 named functions and 11,725 named data symbols.
- 21,851 functions at 100 %, of which 21,849 are byte-verified.
- 20,660 functions with lines, 96,694 lines in 1,424 files.
- 20,660 functions with prototype and locals; 40,105 named types.
- `symchk` reports it matched with lines, globals and types.
- `cdb -z speed.exe -y build\SPEED_EXE_1_3\simbolos` resolves names, prototypes and `dt` types.
- `--prueba` drives the system `dbghelp.dll` against the original executable. It resolved 2,046 of 2,046 sampled names
  and 2,066 of 2,066 sampled lines to the expected file and line.
- Ghidra 12.1.2 headless, PDB analyzer only: 26,684 functions with prototypes, 49,330 data types and the source map
  of 1,424 files. 181 classes trip its "failed to align" structure warning.
- Ghidra 12.1.2 headless with `ImportaSimbolosPC.java` and no PDB: 38,429 names (2 rejected), 26,644 functions created,
  96,619 source-map lines.

The `ghidra-dwarf1` extension from `tools/ghidra/` makes every PE import fail (`DWARF1Analyzer.canAnalyze` throws a
null pointer when the program is not ELF). Use a Ghidra without it for the PC executable.

Without options, source paths in the PDB are absolute to the clone that generated it; `lineas_pc.json` keeps them
relative. For a PDB meant to be shared, `--raiz-fuente C:\nfsmw` writes every source path under that neutral root,
runs `lld-link` with relative paths and `/pdbsourcepath`, and rewrites in place the linker's `S_ENVBLOCK`, where
`lld` stores its own absolute path. The result holds no path of the machine that built it. `--lld` names the linker
(otherwise `LLD_LINK`, then the Android NDK, then `lld-link` on `PATH`, for example from an official LLVM release),
and `--exe` points at an executable outside `orig/`. With `--solo-prueba`, the script only needs itself, the
executable and the generated folder.

`--english` (or `--lang en`) switches the whole output to English, for the community kit:
- file names: `symbols_pc.json`/`.csv`, `lines_pc.json`/`.csv`, `ida_import_symbols_pc.py`, `ImportSymbolsPC.java`;
- columns and values, including the name provenance (`call-graph`, `elimination-1to1`, `ambiguous-copy`, `confirmed`...);
- messages, help and the IDA/Ghidra scripts;
- the default folder, `build/SPEED_EXE_1_3/symbols/`.

Every option also has an English alias (`--output`, `--check`, `--check-only`, `--no-pdb`, `--no-types`,
`--no-history`, `--source-root`). Without `--english`, the output, the names and the options are the same as before.
`--check-only` reads either language.

The kit is all English. It holds the PDB and the other outputs generated with `--english --source-root C:\nfsmw`, and
the inputs from `config/SPEED_EXE_1_3/`, which keep their paths. Its scripts are:
- `make_pdb.py`, which runs `simbolos_pc.py --english`, and `simbolos_pc.py` itself;
- `name_by_call_graph.py`, `name_by_elimination.py` and `name_corrections.py`: copies of `nombra_grafo.py`,
  `nombra_elim.py` and `nombra_corrige.py` with English comments, help, messages and option aliases.

The copies were checked two ways:
- their syntax trees equal the originals once strings, comments and the alias function are set aside;
- run on two identical copies of the tree, perturbed so that the tools had work (1,829 names removed or swapped),
  both versions wrote byte-identical reports, `symbols.json` and provenance log.

The kit and its English scripts live outside the repository. The kit holds nothing of the game and no compiler. Every non-trivial 16-byte block of the
executable's `.text`, `.rdata` and `.data` was searched for in every file of the kit. No `.text` block occurs. The
only `.rdata`/`.data` blocks that occur are identifier strings that are also symbol names, runs of consecutive
integers, and the `RSDS` GUID.

### Build infrastructure in `tools/`

- `tools/project.py`, `tools/ee_gcc_pp.py` and `tools/rename_section.py`: see "Build and measurement changes".
- `tools/download_tool.py`, `tools/delink_to_config_json.py` and `tools/hasher.py`: small fixes, see above.
- `tools/sn-gcc/`: the recipe that builds SN's GameCube compiler from its GPL source, with the `$LC` and
  alias patches as readable `.patch` files. See "Building SN's compiler from its GPL source".
- `tools/zlua_build.py`: builds `zLua.o` on its own with the command from `build.ninja` and prints the tail of the compiler output.
- `tools/dwarf-compare.py` is upstream's and is unchanged. The fork uses it through `tools/dwarfclase.py`, after generating `symbols/Dwarf/` with upstream's `tools/split_dwarf_info.py`.

### Android port experiment (`port/`)

An early experiment, separate from the decompilation build:

- `port/android/` is a Gradle/NDK app for arm64 that compiles decompiled code with clang. It started with `UMath` as a probe. It then built EA's `snd` library and drives the real game mixer (`MIX_audioslice`) through AAudio. 135 of the library's 139 units compile; the four left out are the GameCube DSP driver and codecs with PowerPC inline asm. The app also has small readers for the frontend packages (FEng), fonts and textures, drawn with a GLES2 quad renderer. `tests/` holds `static_assert` layout checks: the mixer definition used by the 64-bit port (`smixport.h`) must stay consistent with `smixer.c` and, compiled for 32 bits, must give the offsets measured on the GameCube asm. `tools/` holds font and texture extractors and a writer for a minimal FEng package.
- `port/zzdata/` has Python readers for the GameCube disc's asset archives: the `ZDIR.BIN` index, the `ZZDATA` volumes, and EA's JDLZ and HUFF decompressors, ported from the decompiled `LZCompress.cpp`.

- Since then (state on 2026-10-01): a GX display-list interpreter draws the game's own car and streamed track on
  GLES2; the game's frontend (FEManager, its packages, fonts and strings) runs in the app; the game's vehicle physics
  drives on the streamed track with collisions; and the app reads the game files from the user's own PC install
  through the game's file manager, converting the attribute vault at load time. The riskiest part of a full port is
  LP64: the data files keep 4-byte pointers. A generator derives each structure's 32-bit layout from the GameCube
  DWARF and the A124 stabs, asks clang for the 64-bit one and emits checked converters; it reproduces the
  hand-written ones byte for byte. Nothing under `src/` is edited for the port.
- State on 2026-10-16: there are two ports, Android (arm64) and PS Vita (`port/vita/`, vitasdk, vitaGL). Both build
  the game from `src/` as the PC variant (`EA_PLATFORM_WIN32`), so the PC decompilation feeds them directly: the PC
  controls screen and input device, the PC time-of-day lighting, the PC rules for EA-XA and MicroTalk audio and the PC
  sound-bank slot sizes all come from decompiled PC code. Middleware threads and file I/O use the GameCube back end
  over small platform shims. On Android, Career mode runs from the prologue to the safe house with saving, and the
  game is driven with touch controls or a pad. On the Vita, quick races run near 60 fps on real hardware, and in the
  Vita3K emulator Career mode runs from the prologue into the second race. Remaining work per area is tracked in the
  fork's port status document.

The `assets/` folders under `port/` hold files extracted from the game for local tests. They are game data, and `port/` is not part of published snapshots.

## Documentation added

The fork's documents are in Spanish, under `docs/`. Upstream's own documents (`getting_started.md`, `dependencies.md`, `splits.md`, `symbols.md`, ...) are unchanged. The documents below live in the fork's working tree; the published snapshot branches carry only upstream's documents and this guide (see `foto.py`).

- `INDICE.md`: the entry point. It lists the five builds and the five documents to read first, where each piece of original data lives and which tool reads it, the work-cycle commands, the five most expensive traps, and how the documentation is organised.
- `METODO.md`: how progress has been measured and decided since 2026-09-17: no asm, `Matching` ignored as a metric, one report per build, `pctall.py` and its "0 objects fail to compile" control, and what counts as done. Its note that the PS2 branch still shares the prototype's defines is older than the fix described above.
- `PROCEDIMIENTO.md`: how to find what is missing by looking for the common cause, not the function. The steps: measure the five builds, locate the gap (`censohueco.py`), take per-unit diffs, read only the opcode and operand mismatches (`tabla_op.py`, `desplazamientos.py`), confirm with an oracle (`sonda.py`, `sondastabs.py`, `cotejo.py`, `missinline.py`, `dwarf1.py`/`mdebug.py`), make the minimal fix, then measure and commit with the evidence.
- `TRAMPAS.md`: where each tool or metric lies. It has 31 measured entries, each with the case that exposed it, under one rule: demand of every measurement a control that has to change.
- `DATOS-DEBUG.md`: a map of the debug data about the original source in each binary. It covers the GameCube ELF's DWARF-1, the PS2 A124 `.mdebug`, PS2 retail, the Xbox A138, the Xbox 360 milestone and the project's own configs, saying what each contains and which tool reads it. It also lists the compilers needed for reproduction.
- `PLAYBOOK.md`: the codegen manual for SN ProDG / GCC 2.9 on this target, with the source patterns that decide a match and the tools (6,242 lines, about 436 kB). Read it by line range through `PLAYBOOK-MAPA.md`.
- `PLAYBOOK-MAPA.md`: the generated map from each section of `PLAYBOOK.md` to its line range (`scripts/mapaplaybook.py`).
- `HERRAMIENTAS.md`: the generated index of `scripts/`, by family, from the first docstring line of each script (`scripts/indice.py`). It covers all 138 scripts.
- `ESTRATEGIA.md`: the closing strategy of the `linked` (GameCube DOL) era. It is historical: the method still applies, but its figures are frozen.
- `GHIDRA_NFSMW.md`: the Ghidra 12.1.2 headless analysis of the GameCube ELF: where the project and scripts are, the symbol export, and what Ghidra adds to the other oracles.
- `X360_EXTRACTION.md`: the verified procedure for obtaining the decrypted Xbox 360 basefile that the dtk v0.1.2 split needs.
- `PS3_PKG_EXTRACTION.md`: the pipeline used to extract a PS2-Classic ISO of a US retail PS2 version (SLUS-21267) from a PS3 PKG. Its helper scripts live in `tools/scratch/`.
- `PC_PDB.md` (in English): the guide to the PDB for the PC executable. It covers what the PDB contains, how to use
  it in WinDbg, Visual Studio, x64dbg, IDA and Ghidra, how to regenerate it, how it was derived, and its caveats. It
  ships inside the PC PDB kit.
- `docs/congelado/`: 95 JSON files, one per frozen unit, each with the SHA-256 of that unit's normalised objdiff diff. `scripts/frozen.py` writes and checks them. Debug-info changes alter the fingerprint, so many are stale.
- `docs/analisis/`: 655 files, about 11 MB. It holds the per-round analysis reports, JSON censuses and dumps. Treat it as a diary: consult it, but take figures from `pctall.py`. `INDICE.md` flags two reports as refuted.

---

# The source tree, area by area and file by file

This part walks the source tree. Each area opens with what it covers, which
units compile it and how it compares with upstream, then lists its **notable
files**, the ones that carry a finding, with up to four points each: **what**
was added or changed, **why**, the **evidence** (which original data says so)
and **how** it was obtained or measured. An **Everything else** list closes each
area, so that every changed file is at least named.

Conventions used in this part:

- **Compare against the fork point.** Each snapshot branch is one commit whose
  parent is upstream's `59f184a0`, so `git diff 59f184a0 HEAD -- <path>` shows
  exactly what this fork added. `git diff upstream/main` also shows upstream's
  newer commits in reverse, and where both trees reconstructed the same files
  independently it shows two reconstructions side by side.
- **Names of the oracles.** "DWARF" is the GameCube DWARF-1, "line map" is
  `symbols/debug_lines.txt`, "A124 stabs" is the `.mdebug` of the PS2
  prototype and "type dump" is `symbols/PS2/PS2_types.nothpp`; the first part
  of this document describes each of them.
- **Figures.** Per-unit figures are objdiff's, taken the way upstream takes
  them (one report per build, the `Matching` flag ignored), on 2026-09-18 for
  this fork and at `fd0c355` for upstream. Per-function percentages quoted
  from the history are the value when the change was made.

---

## Events, messages, Lua, input, the main loop and the attribute system (`src/Speed/Indep/Src/{Generated,Interfaces,Input,Main,Debug,Lua,Gamemodes}/`, `src/Speed/Indep/Tools/`)

These directories feed three large GameCube units, `zMain` (events, Hermes messages, event sequencer, scheduler,
input, `Generated/AttribSys/codegen.cpp`, `Generated/QueryDefs.cpp`), `zLua` and `zAttribSys`, plus the small
`zGameModes` and `zDebug`; the `Interfaces/` headers are included almost everywhere. 532 files differ from
`upstream/main` (457 under `Generated/`, 22 `Interfaces/`, 16 `Input/`, 12 `Lua/`, 10 `Tools/`, 9 `Main/`,
4 `Debug/`, 2 `Gamemodes/`; 7 are new), and most of the volume is code written where upstream still has empty
`.cpp` files: all 161 event classes and the event registry, the Lua bindings of 65 Hermes messages, the whole game
side of `zLua`, the GameCube input devices, the scheduler and the event-sequencer engine. The rest corrects headers
against the original's debug data (member order, pure virtuals and vtable order, allocation names, the file and
position of each definition), adds the first `EA_BUILD_A124` layout variants of generated attribute classes, and
keeps only hash names that reproduce their value. `Generated/Hash.hpp` and `Input/ActionRef.h` differ only because
upstream changed them after the fork point, and several interface and input headers were rewritten on both sides,
so diff against the fork point (upstream's `59f184a0`) to see this fork's changes alone; note also that
`tools/event_class_generator.py` and `tools/attrib_generator.py` are identical to upstream's, so regenerating
`Generated/` would revert the fixes below.

### Notable files

#### Events and Hermes messages

- **`Generated/Events/EventDefs.cpp`** — What: the event registry, empty upstream. `RegisterEvent::LookupEvent`,
  `ResolveEvent` and `GetLuaBinding` search a 161-entry key table with `SearchPackedBinaryTree` (an implicit binary
  tree: the children of node *i* are 2*i*+1 and 2*i*+2) and index three parallel callback tables; `NULL` marks the
  46 events without Lua binding or resolver, and `ECollision`, which has no make-event callback.
  `EventManager::BindMessagesToLua` (0x1E14 B) builds 65 `Hermes::Handler::Create(M*::HandleMessage_LuaBinding,
  UCrc32(0x20D60DBF), 0)` in an array initializer, then `reserve` plus a `push_back` loop. Evidence: every key is
  `Attrib::StringHash32` (lookup2, initval 0xABCDEF00) of the class name (`EScheduleEvent` → 0x88281A34; all 161
  check out); the message order is the order of the `bl M*::_GetKind` calls in the asm, and an unrolled 0x18-byte
  copy block gives away the aggregate initializer. The `M*.h` includes are sorted case-insensitively
  (`MEnteringGameplay` before `MEnterPostRaceFlow`), the order that reproduces the original's emission order; an
  ASCII sort does not.
- **`Generated/Events/E*.cpp` (161 files)** — What: every event class implemented (upstream: empty files and a
  stub `E911Call.cpp`): constructor, destructor, `GetEventName() const`, `E*_MakeEvent_Callback` (reads the event's
  `StaticData` and `gEventDynamicData`) and, for the 115 script-callable events, `E*_MakeEvent_LuaBinding` (checks
  `lua_gettop`, converts with `lua_to*`) and `E*_ResolveEvent_Callback`. The game logic is in the destructors:
  `Event::operator new` allocates in a linear buffer and `EventManager::RunEvents` deletes the queued events in
  order. Evidence/How: transcribed from the asm with the GameCube DWARF (member order; locals with registers, whose
  declaration order decides allocation; the inline tree, which says which accessor or overload the original called)
  and `symbols/debug_lines.txt` for statement boundaries. Indirect calls are named from their vtable offset,
  8 × (index + 2), counting the interface's own virtuals after `IUnknown`'s destructor from 0. Two idioms read in
  the A124 dump: `UMath::Vector3 v = {a, b, c};` compiles to a 12-byte memset, three stores and a copy; a bool
  chosen per branch is two explicit calls, not one call with `!x`.
- **`Generated/Events/E*.hpp` (161 files)** — What: `GetEventName() const override` everywhere (the symbols are
  `GetEventName__C8E911Call`, …), prototypes for the Lua binding and resolver, and member order taken from the
  DWARF where the generated offset comments were wrong (duplicated or shifted): `EMomentStrm` puts
  `fPosition`/`fVector`/`fVelocity`/`fhSimable` before `fAttribStrm`/`fAttribKey` (0x44 B); `EAIEngineRev` and
  `EGTriggerInternal` start with `fhSimable` and their constructors take the parameters in that order; offsets fixed
  in `EBecomePursuitCar`, `EDamageLights`, `EResetSystem`, `EStopObjectEffect` and `EWakeObject`. Why: member order
  is what makes the constructors match.
- **`Main/Event.h`, `Main/Common/Event.cpp`** — What: `Event::GetEventName() const = 0`; `Event::StaticData` is
  fast-allocated; `EventManager::EmbedField` is a member template with a body (copies a string or a
  `Hermes::Message` into the event buffer, 16-byte aligned, and grows `fEventSize`); `EventsQueued()` returns
  `gDeletionPoint < gCreationPoint` (upstream compares the other way round); the `ProfileNode` in `RunEvents` has
  no name string. Evidence: the ELF has `EmbedField__H1ZCc_12EventManagerP5EventPX01_PX01` and its
  `Hermes::Message` twin, not two overloads (before, every event with such a payload called a symbol that does not
  exist); GCC 2.95 emits a literal passed to an inline even when unused, and `"EventManager::RunEvents"` is not in
  the original's data.
- **`Generated/Messages/M*.h` (65 of 66)** — What: inline `HandleMessage_LuaBinding` (wraps the message in a
  `LuaMessageDeliveryInfo`, calls `LuaPostOffice::RouteMessage`) and `BuildMessageTable` (a Lua table keyed by the
  field name without its `f`). Conversions read from the asm: float/int/unsigned → `lua_pushnumber`, bool →
  `lua_pushboolean`, `const char *` → `lua_pushstring`, `Vector3`/`Vector4` → `lua_pushnil`, pointers and
  `HSIMABLE` → `lua_newuserdata` + `LuaBindery::AttachMetatable(<class name>)`. Why: `BindMessagesToLua` is the root
  that references them; once it existed, 65 `HandleMessage_LuaBinding`, 65 `BuildMessageTable` and 65
  `Hermes::Handler::StaticHandler<M>::Call` instances were emitted. Evidence: the DWARF lists these (and
  `PostMessage_LuaBinding`, `BindToLua`) as inline members of every message, and the line map puts the bodies in
  `M*.h`, so they live in the header.
- **`Main/Common/EventSequencer.cpp`, `Main/EventSequencer.h`, `Main/EventDynamicData.h`** — What:
  `EventSequencer::Engine` written (the 23 `IEngine` overrides and the destructor; its `System` array is taken from
  `gFastMem` as `"EventSequencerSystems"`), `IEngine`/`IContext` pure, and upstream's logic corrected against the
  asm: `System::Complete` ends with `gEndAction` (upstream `gStopAction`); `ProcessStimulus` invokes the stimulus
  when the system is idle and queues it while in action (`QUEUE_SHALLOW` with an empty queue, `QUEUE_ALLOW` with a
  free slot), looking the filtered stimulus up in the end state; `ExecuteFilter` indexes the `QueryDesc` array and
  passes `mQueryName`, static data + `mDataOffset`, `mCount` and `&queryValues[mIndex]` to `Query::Invoke`; the
  stimulus and action ids are static-initialised with `StringToID("create")`, `StringToID("trigger")`, …; the chunk loaders
  are registered for `BCHUNK_CARP_EVENT_SEQUENCES`/`BCHUNK_CARP_EVENT_SEQUENCE` (0x8003B810/0x3B811, the values the
  loader compares; upstream used 0x8003B72C). `EventDynamicData` gets its own header, as in the original
  (`d:/mw/speed/indep/src/main/eventdynamicdata.h` is in its file list). Evidence: `scripts/vtable_audit.py` showed
  the original `_vt.*` slots pointing to `Engine::*` and ours to undefined `IEngine::*`.
- **`Main/Common/Scheduler.cpp`, `Main/Scheduler.h`** — What: the scheduler, empty upstream. `Scheduler` creates
  the once-per-game-loop, sim-rate, half-rate and quarter-rate schedules; `Run` turns real time into ticks with a
  carried fraction (`mTickBank`; full-speed mode asks for 4), the catch-up is capped at 6 ticks per frame (2 with
  `LOCK_TO_30`), and the event queue is drained after each tick; `Schedule` keeps task buckets (`AddTask`,
  `RemoveTask`, `EmptiestBucket`, `RunTasks`). The header gains the `TaskRecord` constructor, the four `Schedule_*`
  subclasses (0x2C) and fast-allocated lists.
- **`Main/GC/VirtualMemory.cpp`** — `VirtualMemory` (0x901C B: 1,024 physical and 2,048 virtual page records)
  from the DWARF, its constructor and `fgInstance`. `fgStats` and `fgDebugDisplay` are declared but not defined: the
  DWARF gives them address 0xFFFFFFFF (removed by the linker), so a definition would add `.bss` the original lacks.
- **`Main/ScratchPtr.h`** — `IMPLEMENT_SCRATCHPTR` defines the explicit specializations without `= {}`. With the
  initializer GCC 2.95 emits a real `.data` definition, which took the symbol from the COMMON block the original
  uses and shifted everything after it in the DOL; without it the specialization is only a declaration.

#### Lua (`Lua/`)

Upstream has the five game-side `.cpp` files of `zLua` empty. The original's address map gives their link order
(Runtime, PostOffice, Bindery, Attributes, GameHooks; the A124 links them in the same order) and sizes
(GameHooks 115 functions / 21,560 B, Bindery 96 / 16,552 B, Runtime 23 / 6,348 B). Within each file the
definitions follow the original's emission order, which GCC 2.95 takes from parse and definition order.

- **`Lua/LuaRuntime.cpp`, `Lua/LuaRuntime.h`** — What: the Lua memory manager (`Alloc`/`Free`/`Realloc` over a
  pool with 16 small-block free chains and a map of large free blocks, emergency allocations, a heap snapshot and a
  reset deferred while a message is being delivered, `BeginDelivery`/`EndDelivery`), `PackIdentifier`/
  `UnpackIdentifier`, `SerializeTable` (0x4A0 B), `DeserializeTable` (0x378 B), `DumpStack`, `HandleGlobalIndex`,
  `LuaPanic` and the C hooks
  `LuaRealloc`/`LuaFree` installed with `luaM_setallocator`. Evidence: A124 stabs, GameCube DWARF and asm; the
  DWARF's `Labels:` section names the `goto LabelDeserializeFailed` target, and a lexical block that ends before
  the last statement of a loop body marks a `for` increment. How: `LuaRuntime::mObj` is defined after `LuaRealloc`
  because GCC 2.9 names the static initializer after the unit's first global (`_GLOBAL_.I.LuaRealloc`).
- **`Lua/LuaBindery.cpp`, `Lua/LuaBindery.h`** — What: the C++-to-Lua binding framework: `Parameter<T>` and
  `ReturnValue<T>` specializations (bool, int, float, double, unsigned, `const char *`, `GRuntimeInstance *`,
  `GRaceStatus *`, `ISimable *` through a `HSIMABLE` userdata), function and method thunks with the target pointer
  in an upvalue, `BindSingleton`, the metatable and global-table helpers, and `BindToGameCode` (0x119C B), which
  registers every hook. How: the thunk families are uniform in the original (all 22 `BindVoidFunction` instances
  are 116 B, all 5 `BindVoidMethod` 132 B), so the framework was designed once and every instance followed; the
  template instance names fix each hook's signature (e.g. `BindFunction<ISimable *, int>`).
- **`Lua/LuaGameHooks.cpp`, `Lua/LuaGameHooks.h` (new)** — What: the script-callable game functions
  (`Activity_*`, `Audio_*`, `Bin_*`, `Camera_*`, `Debug_*`, `Game_*`, `HUD_*`, `MiniMap_*`, `NIS_Play`, …). Three
  uses of APIs the A124 lacks are under `#ifndef EA_BUILD_A124` (`IPlayer::IsLocal`,
  `ICopMgr::SetAllBustedTimersToZero`, `GRaceStatus::SetWarpWhenInFreeRoam`). Evidence: DWARF (locals; the
  `_type_ShuffleVector` container tag visible in the mangled `reserve`/`push_back`), A124 stabs, asm;
  `gGPSDestination` is file-static because the original has it as a local 12-byte `.bss` symbol. Finding:
  `OnlineRacer` is an empty class on GameCube (DWARF `total size: 0x1`, no members): the online layer is compiled
  out and its accessors return constants; the tell is a discarded call result followed by `li rN,0`.
- **`Lua/LuaAttributes.cpp`, `Lua/LuaAttributes.h`, `Lua/LuaPostOffice.cpp`, `Lua/LuaPostOffice.h`** — What:
  attribute access from scripts through metatables on `GRuntimeInstance` and `LuaAttribArrayInfo`
  (`__index`/`__newindex`/`__eq`), per-type push functions, `BuildAttributeTypeTable`, `PushAttributeValue`,
  `SetAttributeValue`; `IMessageFilterContext` and `LuaMessageDeliveryInfo` (0x34 B, `UTL::COM::Object` plus the
  interface), `RouteMessage`, `RegisterHandler`/`UnregisterHandler`. These two were the first files written from
  the A124 dump; three shape findings from LuaAttributes: `lua_pushboolean` needs an explicit `? 1 : 0`, the null
  guard of `IsSameInstance` is `(a != NULL && b == NULL) || (b != NULL && a == NULL)`, and `LuaAttribAccessInfo` is
  filled in an order that is not its declaration order.
- **`Lua/source/lua.h`, `Lua/source/lauxlib.h`** — What: `extern "C"` guards, and `lua_userdatalen` declared (it
  is defined in `lapi.c`). Why: `symbols.txt` has the Lua API only unmangled (`lua_gettop`, `lua_settop`, …, 135
  `lua*_` symbols and no mangled one); without the guards, C++ callers such as `GHandler.cpp` called names like
  `lua_gettop__FP9lua_State` that do not exist.

#### Attribute system (`Generated/AttribSys/`, `Tools/AttribSys/`, `Tools/EventSys/`, `Tools/Inc/`)

- **`Generated/AttribSys/Classes/*.h` (all 59 non-`_hash` headers)** — What: three fixes to the generated
  template. `USE_ATTRIB_ALLOC("Attrib::Gen::<class>")`: both `"tires"` and `"Attrib::Gen::tires"` are in the A124
  ELF, and the long one is the one referenced. `Modify()` calls
  `ModifyInternal(ClassKey(), key, LocalAttribCount() + space)`, like `Attrib::Instance::Modify`; with upstream's
  body `LocalAttribCount()` was evaluated in the caller, and "an
  argument computed in the caller instead of inside the inline" is the general sign that a generated body lacks a
  term. `GenerateUniqueKey()` calls `GUKeyInternal(ClassKey(), name, registerName)`; upstream's calls itself.
  `ClassKey()` stays the header inline as upstream has it: the out-of-line `bl ClassKey` of the original comes from
  constructing by key (`Attrib::Gen::X v(key, 0, nullptr)`), not from a hand-written `FindCollection`.
- **A124 variants in `pursuitlevels.h`, `camerainfo.h`, `emittergroup.h`, `ecar.h`, `chassis.h`, `turbosfx.h`,
  `speech.h`, `smackable.h`, `engineaudio.h`** — What: attributes that are not in the A124's `_LayoutStruct` are
  removed from it under `#ifndef EA_BUILD_A124`, and their accessors use the by-key macros there: `cops`
  (the A124 pursuitlevels layout starts at `heliLOSdistance`, 0x50 earlier), camerainfo's eight arrays (only
  `CollectionName` is laid out), emittergroup `Emitters`/`IntensityRanges`/`FarClip`, ecar `ReflectionOffset`/
  `KitWheelOffsetRear`/`KitWheelOffsetFront`, `RENDER_MOTION`, `ChargeTime`, `BackTime`, `IsWooshable`
  (`Ginsu_ACL_Neg_L_RPM` in engineaudio came from the `ps2_fixes` branch). The choice is per attribute: engine,
  tires, transmission and others keep their arrays in the layout. Evidence: `scripts/sondastabs.py` (layouts from
  the A124 `.mdebug` stabs) flagged nine `_LayoutStruct`s; the A124 asm reads by key (`GetAdjustedCopCounts` calls
  `GetAttributePointer(0xA9811B93, i)` for `cops(i)`). How: `scripts/attrfuera.py <class> <attrs…>` writes the
  guards. Open: `speechtune` is reordered in the A124 and needs a second layout.
- **`Generated/AttribSys/Classes/{frontend,emitterdata,engineaudio,smackable,speech,rigidbodyspecs,camerainfo}.h`**
  — These seven keep the accessor bodies inside the class (the form of upstream's generator before its last
  update; upstream now defines them out of class) and carry offset comments corrected against the DWARF: a ProDG
  `bool` field takes 4 bytes although the DWARF says size 1, so every annotation after the first bool was short
  (`FECarRecord::Default` reads the bool at 0x58 with `lwz`, and `frontend::UnlockedAt` is at 0x5C, not 0x59).
- **`Generated/AttribSys/codegen.cpp`** — What: upstream's `// TODO _TypeHandler structs` filled in:
  `Attrib::DefaultDataArea` (a 2 KB static), the seven `*_TypeHandler` classes (`Retain`/`Clone`/`Clean`/`Release`,
  cleaning each record's `RefSpec` members: `Reaction`, `StreamMoment`, `mSurface` and `mEffect`, …), their
  singletons, the id table that `TypeDesc::Lookup` searches as an implicit binary tree, and `TypeDesc::NameToType`.
  Evidence: the ids are `Attrib::StringHash32` of the type names (`Attrib::RefSpec` → 0x2B936EB7,
  `AICollisionReactionRecord` → 0xAA229CD7; all seven check out); member names from the DWARF. The comments in
  `Lookup` record why it reads the global count three times and splits `(id < t) + 1` into two locals.
- **`Tools/AttribSys/Runtime/AttribSys.h`, `Common/AttribHashMap.h`, `Common/AttribDatabase.cpp`,
  `Common/AttribPrivate.h`, `Common/Attribute.cpp`, `Common/AttribSupport.cpp`** — What: definitions moved to the
  file the line map gives: `Array`, `Node` and `HashMap` to AttribHashMap.h (attribhashmap.h:44-215 and 385-466);
  `ClassTable`, `TypeDescPtrVec`, `TypeTable`, `CollectionList`, `ClassList`, `DatabaseLoadData` and
  `DatabasePrivate` into AttribDatabase.cpp (attribdatabase.cpp:36 and 62), with `DatabasePrivate`'s static helpers
  after its constructor and destructor; `~Attribute` to attribute.cpp:35, between the three-argument constructor
  and `operator=`. Why: GCC 2.95 emits inline and template code in parse/definition order, so the file decides where
  code lands (the original emits `~ClassTable` before `~TypeTable`, and instantiates the `std::find` of
  `QueueForDelete` only after the constructor's and destructor's template instances). Also:
  `USE_ATTRIB_ALLOC(name)` takes the literal, with no `ENABLE_IN_MILESTONE` branch (the dead branch doubled the
  operators' RTL and GCC 2.95 stopped inlining destructors that use them), and every
  allocation passes the name the original passes (`Attrib::Collection`, `ClassPrivate`, `ClassTable`, `TypeTable`,
  `DatabasePrivate`, `CollectionHashMap`, `HashMapTable`, `HashMap`, `Array`, `Vault`, `ExportManager`, `RefSpec`),
  read from the A124 ELF by following each function's relocations to its `.rodata`. Bug fixed:
  `DatabaseExportPolicy::IsReferenced` reads `GetExportData(index)` (upstream `GetExportType`).
- **`Tools/AttribSys/Runtime/AttribSys.h` (API)** — `Attribute::Get<T>` and `TAttrib<T>::Get` defined out of the
  class (in the class they would be implicitly inline, and the original calls them: zAI's object defines the
  `bool`, `UMath::Vector4` and `GCollectionKey` instances), with a ternary body (the original tests with `mr.`, not
  `cmpwi`); `Attribute::Set<T>`, `TAttrib::Set`, `Instance::AddAndSet`/`GetOrClone`, `TypeDesc::GetName`,
  `RefSpec` `operator new`/`delete`/`==`/`!=`, `Blob`'s members (its copy constructor is declared on
  purpose: it makes GCC 2.9 keep a `Blob` local addressable in the frame). `Attribute::GetElementPointer` is public
  because the original inlines it into `GRaceCustom::SetAttribute` and `CreateRaceActivity` (attribsys.h:276-279
  in the line map). `ATTRIB_CODEGEN_CHECKEDGETLAYOUT` is marked in the header as invented.
- **`Tools/AttribSys/Runtime/VecHashMap64.h`** — What: `GetNextValidIndex`, `GetPtrAtIndex`,
  `GetPtrAtValidIndex` and `DeleteIndex` added (upstream's header does not have them; callers need them), and
  `UpdateSearchLength` restructured: the second case is an `else if` that assigns `freeIndex`, with `targetIndex` a
  local initialised from the parameter. Evidence: the asm (`mr r28,r26`); the one change closed five
  `Cache::Flush*`/`TossSample` functions in the sound units and is neutral in `zAttribSys` and `zMisc`.
- **`Tools/EventSys/Runtime/Common/eventsysdata.h`** — `QueryDesc` moves into `namespace CARP`: the only symbol
  that names it is `Q24CARP9QueryDesc`, and at global scope the `SearchPackedBinaryTree` instances mangled wrong.
- **`Tools/Inc/ConversionUtil.hpp`** — `ConversionUtil::RightToLeftVector3/4` and `RightToLeftMatrix4` templates
  (with `Copy4`/`Scale3`/`Make3`/`Make4`), whose names and template arguments are in the A124 function dump
  (`symbols/PS2/PS2_functions.nothpp`) and whose bodies follow the GameCube DWARF; `LB2KG`. `KPH2MPS` stays
  `x / 3.6f`, with the measurement in a comment: the 17 uses of 0x3E8E38E4 are all in `zAI`'s `.rodata`, which
  calls the macro, while the 10 of 0x3E8E392E (`0.27778f`) sit in `.over` and the frontend's `.rodata`, where the
  source multiplies by hand.

#### Interfaces (`Interfaces/`)

- **`Interfaces/IFengHud.h`, `Interfaces/IMenuZoneTrigger.h` (new)** — What: this fork and upstream reconstructed
  `IHud` and its 25 HUD element interfaces independently; here `_IHandle`, owner constructor and virtual destructor
  are written out, and every method but `IGenericMessage::RequestGenericMessage` is pure (all slots of
  `_vt.11ITachometer` and `_vt.4IHud` are `__pure_virtual`; GCC 2.9 emits `_IHandle__X` and the interface
  destructor only for a fully abstract class, and with non-pure methods the tree emitted 2 of the game's 97
  `_IHandle` symbols). Differences from upstream with evidence: `ITachometerDrag` has no virtual besides
  the destructor (`_vt.14DragTachometer.15ITachometerDrag` is 0x18 B, `_vt.11ITachometer` with five methods is
  0x40; the A124 stabs agree; upstream declares five); `IMenuZoneTrigger` has the ten methods of the GameCube vtable
  (`_vt.15MenuZoneTrigger.16IMenuZoneTrigger`, 0x68 B), while the A124 stabs list
  `EnterTriggerForAutoSave`/`ExitTriggerForAutoSave` instead of `RequestCingularLogo` (upstream declares all
  twelve). `IAutoSaveIcon` comes from the GameCube DWARF plus the A124's out-of-line copies.
- **`Interfaces/IListener.h`, `Interfaces/SimActivities/ITrafficCenter.h`** —
  `Sim::Collision::IListener::OnCollision` and `ITrafficCenter::GetTrafficBasis` are pure. Why: declared non-pure
  and defined nowhere, the classes had no key method, GCC 2.9 emitted their vtables in no unit, and `zCamera` could
  not link. Evidence: the ELF's
  `_vt.Q33Sim9Collision9IListener` (24 B) and `_vt.14ITrafficCenter` (32 B) have `__pure_virtual` in those slots,
  and `GetTrafficBasis__14ITrafficCenter…` does not exist. ITrafficCenter.h is `#pragma interface`; the zCamera
  SourceList carries the matching `#pragma implementation`, where the original defines the vtable and destructor.
- **`Interfaces/Simables/IVehicle.h`, `SimEntities/IPlayer.h`, `SimModels/ISceneryModel.h`** — Signatures from the
  mangled names and vtables: `IVehicle::IsGlareOn` is not const (`IsGlareOn__8PVehicleQ29VehicleFX2ID`),
  `GetPosition` returns `const UMath::Vector3 &`, and `GetCacheName` is under `#ifndef EA_BUILD_A124` (the A124
  vtable ends at `GetPerformance`); `IPlayer::SetHud(ePlayerHudType)` replaces upstream's `SetHud() const`, which made
  `LocalPlayer::SetHud` a new virtual with an extra slot; `ISceneryModel` has the A124 stabs' vtable order and
  `GetSceneryTransform(Matrix4 &) const`, `GetSpawnerID() const`, `IsExcluded(unsigned) const`
  (`GetSceneryTransform__C12SceneryModelRQ25UMath7Matrix4`, …), so that `SceneryModel`'s methods really override
  them. `IPlayer`'s virtuals are pure, as are those of `IEntity`, `IGameState`, `INIS`, `INISLISTENER`,
  `IPlaceableScenery`, `ITriggerableModel`, `IFeedback` and `ISteeringWheel`.
- **`Interfaces/Simables/IRigidBody.h`** — Includes ICollisionBody.h, IBody.h, IRBVehicle.h and ISimpleBody.h.
  Evidence: in the A124 stabs the types `HCOLLISIONGEOM`, `ICollisionBody`, `IBody`, `IRBVehicle`, `ISimpleBody`,
  `IRigidBody` always appear together and in that order behind unrelated parents, and on GameCube they occur in the
  same 21 units; with the includes, `zAI` emits `find<ISimpleBody **>` just before `find<IRigidBody **>`, as the
  original does.
- **`Interfaces/Simables/IEffects.h`, `Simables/IExplodeable.h`, `SimEntities/IOnlinePlayer.h` (empty upstream),
  `SimEntities/IRecordablePlayer.h` (new)** — `IEffects` and `IEffectsVehicle` with
  the vtable order of the A124 stabs, `IExplodeable::OnExplosion`, `IOnlinePlayer` (its three methods from the
  A124 stabs; its `_IHandle` is in the GameCube binary and `Game_AddPlayer` queries it), and the
  `IRecordablePlayer` skeleton (`Listable<IRecordablePlayer, 8>`, from DWARF and stabs).

#### Input (`Input/`)

- **`Input/GC/InputDeviceGC.cpp`, `Input/Common/iomodule.cpp`, `Input/Common/SteeringWheelDevice.cpp`,
  `Input/SteeringWheelDevice.h`** — What: written from empty files: the GameCube `GameDevice` (devices enumerated
  through realcore 6.24's RealInput, the pad data block, rumble through `InputEffectState`, the `IFeedback`
  overrides, a scalar description table ended by an entry with a `NULL` name, which is what `Initialize` tests),
  `IOModule` (creation, polling, unplug detection, `IsJoystickTypeWheel`) and the Logitech `SteeringWheelDevice` (force
  feedback through `LGWheels`, wheel and pedal conversions). Allocations carry the names the original passes
  (`new ("LGWheel", 0)`, `::new ("GameDevice::SteeringWheelDevice", 0)`: `UTL::COM::Object` declares its own
  `operator new`, so without `::` GCC 2.95 would not consider the global one).
- **`Input/Common/actionqueue.cpp`, `Input/InputDefParser.h`, `Input/Common/action.cpp`** — What: upstream's
  "unfinished" `FetchCurrentValues` and `IO_UpdateFromDevice` completed (dead-zone ramp per mapping, press/release
  thresholds at 0.5, and an analog update dropped while another mapping of the same action reads higher);
  `InputMapEntry::HasChanged()` is `|Current − Previous| > 0.000001f` (upstream: `CurrentValue < 0 // ?`);
  `getActionIDString` spelled as in `getActionIDString__F8ActionID`; `GameActionToStringTable` has no
  `MAX_ACTIONID` row: the original table (1,112 B) ends at `ACTION_FLUSH` (0x89) and the sentinel.

#### Hash names, game modes, debug and queries

- **`Generated/LanguageHashes.hpp`, `Generated/FEngHash/FEHash_{FeBusted,FeWorldMapQuickList,FeArrayScroller,
  UI_DebugCarCustomize}.hpp`** — What: upstream's lists (marked "from Carbon and not fully accurate") reduced to
  names that reproduce their value with `FEHash`/`bStringHash`: 5,357 of 5,370 language names (the hashed string
  has no `LANGUAGE_` prefix: `ALL_CHARACTERS` → 0x11124417), FeBusted 127/131, FeWorldMapQuickList 52/61,
  FeArrayScroller 33/49, UI_DebugCarCustomize 28/28; FEng object names such as `__BUTTON_1__` hash `BUTTON_1`. Each
  header says how many names it dropped; no value had two candidate names. These headers back the named constants
  used across the frontend (see that section).
- **`Gamemodes/EmotionManager.cpp` (empty upstream), `Gamemodes/EmotionManager.hpp` (new)** — What: the emotion subsystem
  (`EmotionalSummary`, `EmotionalResponse`, `EmotionManager` and their string tables). The GameCube binary keeps
  only the unit's static initializer, `_GLOBAL_.I.aEmotionalSummaryTypeStrings`, and the tables' strings; the DWARF
  still describes the discarded objects (address 0xFFFFFFFF), which gives the table names
  (`aEmotionalResponseTriggerStrings`, not `…TypeStrings`) and lists items not reconstructed (`g_EmotionDebug`,
  `EM_Trigger_EmotionalResponse`, `SMALL_FLOAT`, `TIME_RATE_CHANGE`, `VALUE_RATE_CHANGE`). The DOL has five phase
  names and CamelCase triggers, and no `"Created"`, so that slot is `NULL`.
- **`Debug/Common/DebugGraph.cpp` (empty upstream), `Debug/Common/DebugGraph.hpp` (new)** — `DebugGraph` and
  `DebugGraphDataSet` with the DWARF layouts. Only the static initializer `_GLOBAL_.I.__10DebugGraphPcfffiffb`
  survives in the GameCube build; its name says the `DebugGraph` constructor is the first global the unit emits.
- **`Generated/QueryDefs.cpp`** — `Query::Invoke`, `LookupQueryFunc`, `ResolveStaticData`, `ByteSwapStaticData` and
  the one query, `QSimTime` (returns `Sim::GetTime()`), with a one-entry key table (0xBF50744E =
  `Attrib::StringHash32("QSimTime")`).

### Everything else

- `Generated/Events/`: forward declarations (`WTrigger`, `GRuntimeInstance`) and qualified types as in the mangled
  names (`CARP::Trigger`, `CARP::EventList`, `CARP::CollisionObject`, `GInfractionManager::InfractionType`,
  `struct GRacerInfo`); `EShowResults.hpp` takes `FERESULTTYPE` from FEPkg_PostRace.hpp instead of redefining it;
  `EVehicleDestroyed.cpp` skips the `IPlayer::IsLocal` test on the A124; `EPlayObjectEffect.cpp` calls the inline
  `OrthoInverse` wrapper the DWARF lists; `EPlayRaceMovie.cpp` includes `MNotifyMovieFinished.h` where the original
  parses it.
- `Generated/Messages/`: `MNotifySpeechStatus` uses `Speech::ScheduledSpeechEvent`;
  `MSetCopAutoSpawnMode::fAutoSpawnMode` is declared `int` (the DWARF says `bool`; in this ProDG build both take
  4 bytes).
- `Generated/AttribSys/GenericAccessor.h`: no longer includes attribuserinclude.h and drops an empty
  `MODEL(TAttrib &)` stub. `Generated/TexturePacks/TexEnumGen_ParticleTextures.cpp`: `GetNumParticleTextures()`
  returns 46 (taken from an upstream pull request that is not in `upstream/main`). `Generated/Hash.hpp`: upstream
  renamed its `STRINGHASH_CE_*` block after the fork point; this tree has the older names.
- `Interfaces/`: `RacePreparationInfo()` zero-fills itself with `bMemSet`; `IPerpetrator::MaxiumumHeat` is declared
  without an initializer, as in the DWARF, where it has no address (upstream gives `= 10`);
  `ICopMgr::AreCopsEnabled()` returns `!mDisableCops` (the swapped slots of `SetAllBustedTimersToZero` and
  `GetLockoutTimeRemaining` are now fixed on both sides); `IEntity::SetPosition` takes a const reference;
  `IExplosion.h` includes UVector.h. Upstream's later `NIS_MAX_RACERS`/`NIS_MAX_COPS` in INIS.h are not merged.
- `Input/`: `SteeringWheelDevice` is declared only in SteeringWheelDevice.h, whose constructor sets the index and
  flags and passes `this` as owner; `FFBTypes.h` gets `EffectBinary::Clear`/`Run` and `InputEffectState`'s
  `Mag_right` (0xC) and `Enabled` (0x10), with `Push` taking a `RealInput::Effect *`; `InputDevice.h`'s default
  virtuals return `false`/`nullptr` instead of falling off the end, plus `GetDeviceIndex()`;
  `ActionQueue::IsRequired()` also requires `mPort >= 0`, and the `IO_*` methods are public. `ActionRef.h` differs
  only by upstream's later edit.
- `Lua/`: portability edits for MSVC and EE-GCC (`std::swap` instead of `_STL::swap`, `static_cast` for attribute
  keys). `Game_AddPlayer`'s start marker is constructed by key (`Attrib::Gen::gameplay x(matched_key, 0,
  NULL)`), which gives the original's out-of-line `ClassKey` call; it used to call a declaration by mangled name.
- `Debug/Common/DebugDraw.cpp`, `DebugDraw.h` (new): imported from the `bit_of_ecstasyy` branch. It is the code
  of the Xbox 360 build's `DebugDraw` class (`DebugDraw::Quad`, `ArrowFlat`, `Triangle`, … are in its symbol
  table), which the GameCube build does not have; no SourceList compiles it yet.
- `Tools/AttribSys/Runtime/Common/AttribClass.cpp` drops an unused include; `AttribSupport.cpp` loses
  `~Attribute`, which moved to Attribute.cpp (see above).

---

## Frontend UI and FEng (`src/Speed/Indep/Src/Frontend/`, `src/Speed/Indep/Src/FEng/`)

This area is the game's user interface: the FEng runtime (packages, objects, scripts, list boxes; SourceList `zFEng`) and the Frontend built on it (HUD, menu screens, databases, memory card, movies; `zFe`, `zFe2` and the overlay unit `zFeOverlay`). 361 files differ from upstream/main (284 in `Frontend/`, 77 in `FEng/`; about +26.9k/−24.0k lines) because both trees filled this area in independently after the merge-base, each adding about 72k lines. This fork's side was transcribed against the GameCube DWARF-1, the line map and the extracted original objects, and later took upstream's bodies wherever those match on PS2 without regressing GameCube. On GameCube (objdiff; this fork on 2026-09-18, upstream at `fd0c355`) this fork matches 341/343 functions of zFEng (upstream 319), 913/921 of zFe (846), 1,288/1,307 of zFe2 (1,201) and 465/467 of zFeOverlay (428). That is about 215 functions and 121 kB of code that only this fork matches, against 2 functions (236 B) that only upstream matches. This fork is also ahead in these units on both PS2 builds, although upstream still matches several dozen PS2 functions this fork does not. Beyond function bodies, this fork adds data definitions at the original section and address, so all four units link into the rebuilt DOL; hash names verified by recomputing the hash; A124 layout guards read from the prototype's type dump; and MemoryCard wide-string bridges that build on all five targets.

### Notable files

- **`src/Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp`, `.../FEngHashes/FEHash_Credits.hpp`** — **What:** a constant is kept only if hashing its name reproduces its value. Six script messages hash with a space instead of `_` (`"SORT UP"`, `"LEAVE ALL"`, `"ENABLE INPUT"`, `"NOT READY"`, `"SORT DOWN"`, `"BACK DELAY"`). Nine upstream names that do not verify under any spelling are dropped (`EXITCOMPLETE`, `ENTERCOMPLETE`, `INITCOMPLETE`, `HEADERTEXT`, `HEADERTEXT2`, `BACKBUTTON`, `SELECTBUTTON`, `LISTUP`, `LISTDOWN`), so their uses stay numeric. About 700 frontend literals were then replaced by verified names; this fork uses `LANGUAGE_*` about 290 times, upstream about 50. **Why:** upstream marks its list "from Carbon, not accurate to MW". **Evidence/How:** `FEHash()` from `FEng/FEList.cpp` (`h = 0xFFFFFFFF; h += h << 5; h += c`) was run over several spellings of each name. The same check was applied to `Generated/LanguageHashes.hpp` (names hash without the `LANGUAGE_` prefix; 5,357 of 5,370 verify) and to the `Generated/FEngHash/FEHash_*.hpp` headers the frontend includes. Hash tables such as `Generated/Hash.hpp` and `FEHash_FeBonusCards.hpp`, and all comments, were left alone.
- **`.../customize/CarCustomize.cpp`, `.../customize/FECustomize.cpp`, `Frontend/Careers/UnlockSystem.cpp`, `Frontend/Database/FEDatabase.cpp`, `Frontend/HUD/FeRadarDetector.cpp`, `Frontend/HUD/FeMinimapStreamer.cpp`, `.../quickrace/uiQRCarSelect.cpp`, `.../career/uiRepSheetBounty.cpp`** (under `src/Speed/Indep/Src/`) — **What:** these globals are `extern`, header-defined or missing upstream; this fork defines each one in the file and order where the original object has it. CarCustomize.cpp: the 14 `g_pCustomize*Pkg` and flag globals, `CustomizeShoppingCart::pParentPkg = ""`, the `CustomizeHUDTex*Resources` arrays and `CustomizeDecals::CurrentDecalLocation = 0x501`. `gTradeInFactor = 0.25f` goes in FECustomize.cpp: it sits in zFe2's `.rodata` at 0x803E89B0 and is undefined in the extracted zFeOverlay.o. `TheUnlockData[57]` and `gMaxPartLevels` are tentative definitions, the last two symbols of zFe2's `.bss`. `DriveConfigs`/`HudConfigs` are `static const Attrib::Key[5][2]` tables of `Attrib::Hash::controller::key_*` (`.rodata` 0x803EA794/0x803EA7BC). Also `TWK_RadarDetectorMinThreshold = 0.1f` (the first symbol of zFe2's `.data`), `RadarDetector::mStaticRange = 0.0f`, `gChoppedMiniMapManager = nullptr`, `gPlayerNum` and `theMarker`. **Why:** the units link into a DOL that must equal the original. **Evidence:** `config/GOWE69/symbols.txt`, the symbol tables of the extracted objects in `build/GOWE69/obj` (defined vs. undefined), and ELF values. **How:** GCC 2.95 placement rules: an explicit `= 0` goes to `.data`, while no initializer goes to COMMON/`.bss`, which is emitted last.
- **`src/Speed/Indep/Src/FEng/FEKeyTypes.cpp` (new), `FEng/FETypeNode.cpp`, `FEng/FEScript.cpp`, `SourceLists/zFEng.cpp`** — **What:** `FEKeyTypeSize[7]` and `FEKeyInterpValid[7][5]` move from FETypeNode.cpp into a new FEKeyTypes.cpp, which zFEng includes. They and `FETrackOffsets[11]` get an `extern const` declaration before their definition, and `FEKeyInterpDefault` is no longer emitted. **Why:** in C++ a namespace-scope `const` has internal linkage, and GCC 2.95 defers it to the end of `.rodata`. The extracted zFEng.o has these symbols GLOBAL at unit offsets +0x138 (0x803EA920) and +0x428, and `FEKeyInterpDefault` has no address (0xFFFFFFFF). **Evidence:** DWARF declaration records (FEKeyTypes.cpp lines 11/21/32), the extracted object, symbols.txt.
- **`src/Speed/Indep/Src/FEng/FEEvent.cpp`, `FEList.cpp`, `FEObject.cpp`, `FEPackage.cpp`, `FEKeyTrack.cpp`, `FEngStandard.cpp`, `src/Speed/Indep/Src/Frontend/RaceStarter.cpp`** — **What:** upstream's `// STRIPPED` definitions, mostly empty bodies, are commented out or deleted: `FEEventList::Insert/AddEvent/FindEvent/Delete/SortEvents`, `FELowerCase`, `FEStrInStr`, `FEObject::SetPivot/SetSize`, `FEPackage::FindObjectByName` and `FEKeyTrack::ComputeSize`; `CloseFEngMemoryPool`, `FEngFree`, `FEngMemMove` and `FEngStrCmp`; 18 RaceStarter setup functions. **Why:** each one emits code the original objects do not contain; even an empty body is a 4-byte `blr`. For FEng, none of the 526 extracted objects references them. RaceStarter keeps only `AddTrafficCars` and `AddCopDriverInfos`, which exist as 8-byte functions in the A124 ELF.
- **`src/Speed/Indep/Src/FEng/FEObjectCallback.h`, `src/Speed/Indep/SourceLists/zFe.cpp`** — **What:** `#pragma interface` in the header, and `#pragma implementation "FEObjectCallback.h"` at the top of zFe's SourceList. **Why/Evidence:** the original zFEng and zFe2 import `_vt.16FEObjectCallback` and `_._16FEObjectCallback`, and only zFe defines them, as GLOBAL rather than weak symbols. That is the signature of GCC's interface/implementation pair.
- **`src/Speed/Indep/Src/Frontend/MenuScreens/Common/feArrayScrollerMenu.{hpp,cpp}`** — **What:** `bool bShouldPlaySound` (the first member of `ArrayScroller`), its initializer and assignments, and `ArrayScrollerMenu::NotifySoundMessage` are wrapped in `#ifndef EA_BUILD_A124`. **Why:** the A124 `ArrayScroller` starts with `slots` at 0x00 and is 0xB8 bytes. With the member, every list screen, including subclasses such as `PostRacePursuitScreen` and `uiRapSheetUS`, was 4 bytes off on the prototype. The function does not exist there either; on A124, calls to it resolve to `MenuScreen::NotifySoundMessage`. **Evidence:** `symbols/PS2/PS2_types.nothpp` and the A124 symbols. **How:** `scripts/desplazamientos.py` finds loads and stores that differ only in offset, and `scripts/sondastabs.py` checks every class layout against the `.mdebug` stabs.
- **`src/Speed/Indep/Src/Frontend/MenuScreens/Safehouse/customize/CarCustomize.{hpp,cpp}`** — **What:** `CustomizationScreen::bNeedsRefresh` and `Timer ScrollTime` (0x1DC/0x1E0) are guarded out for A124, together with the scroll-throttled part preview (0.3 s, tick message 0xC98356BA). On the prototype the three `RefreshHeader`s therefore call `PreviewPart` unconditionally. The file also holds the data definitions listed above, and ten more GameCube matches, e.g. `FEShoppingCartItem::DrawPartName` (3,144 B: a `switch` on the part, repeated `GetBuyingPart()` reads, duplicated printf calls instead of format locals), `CustomizeNumbers::NotificationMessage` and `CustomizeMeter::Draw`. **Evidence:** the A124 type dump (`CustomizationScreen` is 0x1DC bytes and ends at `DisplayHelper`). In the A124 code of its four users there is no 0.3f constant, no RealTimer read and no tick message.
- **`src/Speed/Indep/Src/Frontend/HUD/FeAutoSaveIcon.{cpp,hpp}`, `.../HUD/FeOnlineHudSupport.{cpp,hpp}`** — **What:** PS2-only code transcribed from the A124 ELF: `AutoSaveIcon::RequestAutoSaveIcon` (0x00288128, 0xC B); the in-class `IsAutoSaveIconShowing` (`jr ra; lw v0,0x44(a0)`); `OnlineHUDSupport::DisplayGenericMessage(const char *)` (0x002855D0, 0x44 B; `FEHASH_FLASHER2`, `GenericMessage_Priority_1`). `pPackageName` becomes `const char *`, as in the GameCube DWARF. Upstream's empty constructor and `Update` stubs are dropped: the real functions are 0xAC/0xEC and 0x4C/0x1B4 bytes. **Evidence:** `orig/SLES-53558-A124/NFS.ELF`, `NFS.MAP`.
- **`src/Speed/Indep/Src/Frontend/MenuScreens/Loading/FESplashScreen.cpp`, `.../Frontend/Database/FEDatabase.cpp`** — **What/Why:** the `TheTrackStreamer.IsPermFileLoading()` check is compiled out for A124, whose TrackStreamer has no such method. `VideoSettings::Default` uses `FEScale = 0.84f` only on GameCube; the DOL has 0.84f and the A124 has 1.0f.
- **`src/Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.cpp`, `MemoryCardImp.hpp`, `MemoryCardHelper.hpp`** — **What:** RealMemCard is built with `-fshort-wchar`, so its wide-string methods are mangled `Uw`, while the game uses plain `wchar_t` (`w`). As in the original, the game unit defines `extern "C"` bridges named with the `w` symbols, taking `void *` parameters and forwarding to the `Uw` versions: `Delete__Q211RealmcIface16MemcardInterfacePCcPCw`, `__Q211RealmcIface8GameInfoPCwUibT3` and `Load__…PCwPCQ211RealmcIface9TitleInfo`. They are defined only on GameCube and PS2, because MSVC's `wchar_t` is already 16-bit. No `__asm__` alias is left, zFe builds on all five targets, and the bridges match on GameCube. Other changes: `MemoryCardImp::gEntryType[3]` is defined at `.data` 0x8041BBB8 (undefined in zPlatform). Package names come from the DOL (`MC_Main_GC.fng`, `InGame_MC_Main_GC.fng`). Upstream's `MemoryCard.fng`, `IG_Pause.fng` and `AutoSaveIcon.fng` are not in the DOL. The subset of RealMemCard 3.04.01 that the game uses is declared, with `IGameInterface`'s virtuals in the library's vtable order. **Evidence:** the bridge entries in the GameCube DWARF, the same bridges in the PS2 map, and the DOL strings.
- **`src/Speed/Indep/Src/Frontend/FECarViewer.{cpp,hpp}`, `src/Speed/Indep/SourceLists/zFeOverlay.cpp`** — **What:** the seven `CarViewer` statics (`FindWhichScreenToUpdate`, `SetRideInfo`, `CancelCarLoad`, `GetRideInfo`, `HideAllCars`, `ShowAllCars`, `ShowCarScreen`), `CarViewer::haveLoadedOnce`, `TopOrFullScreenRide` and `TopOrFullScreenLoadingReason` move from FEPkg_GarageMain.cpp into FECarViewer.cpp, which is empty upstream. zFeOverlay now includes FECarLoader.cpp and FECarViewer.cpp. **Evidence:** `symbols/debug_lines.txt` attributes this code (0x803A69xx) to `frontend/FECarViewer.cpp`, and symbols.txt puts the data at `.bss` 0x804AB0D4 (0x310 B), `.bss` 0x804AB5A8 (common) and `.data` 0x80439110.
- **`src/Speed/Indep/Src/FEng/FEngine.cpp`** — **What:** five more GameCube matches: `ProcessResponses`, `UpdateMouseState`, `RenderGroup`, `UnloadLibraryPackage` and `~FEPackageCommand`. The pad block of `ProcessPadsForPackage` is now a `switch (i)`; that function reaches 99.9 %. **How:** `ProcessResponses`'s 24-node case tree needed an empty `case 0x104:` and one more case outside the emitted range. Both were found by compiling candidate case sets with the SN compiler's `-S` and comparing the `cmpw` constants. Its QueueMessage calls are written once per branch, because the target duplicates their argument setup. **Evidence:** `ProcessPadsForPackage` keeps uncanonicalized bounds (`cmplwi 0x7; bge`, `cmplwi 0x4; blt`). Only `expand_case` emits these; an if-chain folds to `>C-1`.
- **`src/Speed/Indep/Src/FEng/FEKeyInterpLinear.cpp`, `FEng/FEMath.cpp`, `FEng/FEObject.cpp`** — **How:** `FELerpQuaternion` (1,308 B) is `q * *pOffset`, not the reverse. This was read off the sign of the three cross terms (swapped `fmsubs` factors), and its four `stfs` are a rotation of the written order. `FEInterpLinear` has its `1.0f` branch first, which gives the target's constant-pool order. `FEQuaternion::GetMatrix` fills the matrix column by column, as the order of the zero stores shows. `FEObject::SetupMoveToTracks` case 1 is an integer subtraction through `FEGenericVal`'s `i32*` operator, with no 0x4330 int→float conversion.
- **`src/Speed/Indep/Src/FEng/FECodeListBox.cpp`, `FEng/FEPackageReader.cpp`, `FEng/FEMessageResponse.cpp`** — **How:** `FECodeListBox::ScrollSelection` (2,436 B) follows the DWARF lexical blocks: loop indices are declared once in the outer scope and reused by sibling loops. `ReadObjectChunk`, `ReadScriptTags` and `ProcessCodeListBoxTag` match once the loop shape lets GCC hoist the chunk-ID constants to the preheader. A `while (true)` + `continue` loop prevents that hoist; the size stays the same and only the loop-head address differs. `FEMessageResponse::FindConditionBranchTarget` uses exactly the two locals the DWARF lists: `Nest` (r10) and `Result` (r4, shared with the parameter).
- **`src/Speed/Indep/Src/Frontend/Careers/UnlockSystem.cpp`** — **What:** 26 more GameCube matches: the `Is*Unlocked` and `IsBackroomAvailable` family of the Career, QuickRace and Online unlockers, `DoesCategoryHaveNewUnlock`, `MapCarPartToUnlockable` and `FEMarkerManager::AwardMarker`. **How:** The original accumulates with bitwise OR on a `bool` (`answer |= (cond);`, `answer = answer | f();`), never with `||`. The starting value comes from `static inline bool AllThingsUnlocked()`. `DoesCategoryHaveNewUnlock` is a `switch` whose case order is the body emission order, which can be told from the first `lbz` offset of each body.
- **`src/Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.cpp`, `.../MemCard/uiMemcard.cpp`, `.../MemCard/uiMemcardBase.cpp`** — **What:** switch trees rebuilt. `MenuScreen::BaseNotifySound` (3,784 B) matches with its ~140 cases ordered by the target's `li r3,K` sequence, because case bodies are emitted in source order. Empty `case` labels that share the `default` body emit no code, but they are still tree nodes and move the pivots to the original's values (e.g. 0x35F8620B → 0x3A2BE557 in `UIMemcardBase::NotificationMessage`). **How:** a re-implementation of GCC's `balance_case_nodes`, plus brute-force compiles of candidate case sets with the SN compiler (`-S`, under a second each) to compare the `cmpw` sequence.
- **`src/Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionWidgets.cpp`** — **What:** 25 option `Act` handlers now match (`POGauges`, `GODamage`, `AOCarVol`, `COVibration`, …). **How:** the original ends each handler with `Update(data)` and, unlike upstream, does not call `Draw()` afterwards. It also reads the player index into a local first.
- **`src/Speed/Indep/Src/Frontend/FEngFont.cpp`** — **What:** ten more matches, among them `FEngFont::RenderString` (1,572 B), the constructor, `GetTextHeight`, `GetLineWidth`, `GetCharacterWidth` and `HandleJoyEventTexture`. **How:** the inlined `PrintCharacter` must compute `(mU + 1) + mWidth`. GCC 2.95 does not reassociate `(a + C) + b`, and that operand order decided the float-register allocation that the diff showed as a register permutation.
- **`src/Speed/Indep/Src/Frontend/HUD/FEPkg_Hud.cpp`, `.../HUD/FeMinimap.cpp`, `.../HUD/FeLeaderBoard.cpp`, `.../HUD/FeTimeExtension.cpp`, `.../Frontend/FEngRender.cpp`** — **What:** `HudResourceManager::LoadedCustomHudTexturePackCallback` hashes `"TACH_FILL_%2.2d"`, `"TURBO_LINES_%2.2d"`, `"TACH_NEEDLE_%2.2d"` and `"TURBO_NEEDLE_%2.2d"`. These strings are in the DOL; upstream's `CUSTOMHUD_*` formats are not. `FEngHud::Update` and `cFEngRender::RenderObject` use `ProfileNode`'s default constructor. GCC 2.95 emits a string literal passed to an inline even when the inline ignores it, and neither `"FEngHud::Update"` nor `"TODO"` is in the DOL. GameCube matches: `Minimap::SetupMinimap/Update/UpdateElementArt`, `TimeExtension::Update` (1,436 B), `LeaderBoard::Update` (1,360 B, which needs `int n = mNumRacers > 1 ? mNumRacers : 0;` after the `if`) and `cFEngRender::RenderMultiImage`.
- **`.../quickrace/uiQRCarSelect.cpp`, `.../customize/CustomizeManager.cpp`, `.../quickrace/uiQRChallengeSeries.cpp`, `.../career/uiRepSheetRaceEvents.cpp`, `.../career/uiRepSheetMilestones.cpp`** (under `src/Speed/Indep/Src/Frontend/MenuScreens/Safehouse/`) — **What:** about 22 more GameCube matches, including `UIQRCarSelect::NotificationMessage/RefreshHeader/SetupForPlayer/ScrollCars/GetFilterType`, `QRCarSelectBustedManager::MaybeAddImpoundBox`, `CarCustomizeManager::IsCategoryNew/GetCarPartList/GetCartTotal/CanTradeIn` and `UISafehouseRaceSheet::RefreshHeader`. **How:** `scripts/plan.py --dwarf-only` gives the literal plan of a handler: the chain of mode accessors (`IsCarLotMode()`, `IsQuickRaceMode()`, …) instead of a cached `mode` local; each `const unsigned long FEObj_<NAME>` in its own anonymous block; the two-argument `FEngSetInvisible(pkg, hash)` inline; nested call arguments (`FEDatabase->FillY(c, FEDatabase->GetX(...))`), which keep the global pointer in a saved register as the target does.
- **`src/Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPKg_PostRace.cpp`, `.../Common/feUIWidgetMenu.cpp`, `.../InGame/uiWorldMap.cpp`** — **How:** `PostRaceResultsScreen::SetupResults` searches with the bare call in the loop preheader and a post-increment: `r = &Get().GetRacerInfo(i); while (r->GetRanking() != rank) r = &Get().GetRacerInfo(i++);`. `UIWidgetMenu::SetInitialOption` uses `for (;;) { if (!w) break; ... }` and re-tests the same `w` after the loop, which gives the target's `cr7`. uiWorldMap.cpp adds seven matches: `WorldMap::NotificationMessage`, `AddCops`, `UpdateCursor`, `SnapCursor`, `PanToCursor`, `ClampToMapBounds`, `SetupEvent`.

### Everything else

- **Method (all):** objdiff reports per version from `scripts/pctall.py`; DWARF similarity from `tools/dwarf-compare.py`; DWARF-1 function records from `scripts/dwarf1.py fn` and `scripts/plan.py --dwarf-only`; A124 stabs and class layouts from `scripts/mdebug.py` and `scripts/sondastabs.py`; link checks against the DOL with `scripts/linkdelta.py` and `scripts/fncmp.py`; empty inline bodies from `scripts/missinline.py`.
- **`FEng/` (77 files):** More GameCube matches: `FEPackage::UpdateObject`, the `FEPackage` constructor, the `FEObject` copy constructor, `FEListBox::ScrollSelection`, `FEObjectSorter<1024>::SortObjects` and `FEQuaternion::operator*`. About 45 headers differ mainly because this fork keeps the full DWARF `Decl: speed/indep/src/feng/X.h:N` comments, which upstream shortened to `:N`. Some empty accessor bodies are filled in (the `FEList.h` family, `FEAnimImage::GetImageData`), and `FENode::GetName` returns `char *const` as in the DWARF. This does not change code generation. Notes in `FEButtonMap.cpp`, `FECodeListBox.cpp`, `FEObject.cpp`, `FEPackageReader.cpp` and `FEKeyInterpNone.cpp` explain the include and definition order that reproduces the target's emission order of deferred inlines.
- **`Frontend/` root (30 files):** Matches: `FERenderObject::ClipGeneral` (the `i < 4 && num_verts` condition goes in the loop header, not in a `break`), `cFEngJoyInput::HandleJoy/CheckUnplugged`, `FEManager::IsOkayToRequestPauseSimulation`, `FEPackageData::GetNameHash/ScreenFactory`, `FEPackageManager::GetBasePkgName`, `UnicodeFile::Next`, `RaceStarter::StartSkipFERace` and `MoviePlayer_Play`. New `Event.hpp`, which only re-exports `Main/Event.h` and four generated events.
- **`Frontend/Careers`, `Frontend/Database` (10 files):** Matches: `cFrontendDatabase::GetMilestoneIconHash/Default`, `CareerSettings::LoadRaceData` and `FECarRecord::GetReleaseFromImpoundCost`. `FEDatabase`, `gFEDatabase` and `g_MaxSongs` are now defined. `FECustomizationRecord`'s `operator new`/`delete` use `gFastMem`; they are empty upstream.
- **`Frontend/FEngInterfaces`, `Frontend/Localization` (17 files):** Matches: `FEngGet2DExtentsForMouse`, `cFEngGameInterface::LoadResources`, `cFEng::FindPackage`, `SearchForString` and `WideCharHistogram::PackString/UnpackString`. The `WideCharHistogram` pair stores the second byte through a pointer (`add` + `stb`, no `extsb`). `FEngInterfaceFEObjects.hpp` notes the DWARF register of every parameter.
- **`Frontend/HUD` (54 files):** mostly parallel transcriptions of the same classes. Besides the entries above, `NitrousGauge::SetNos` and `HudResourceManager::LoadedCustomHudTexturesCallback` match.
- **`Frontend/MenuScreens/Common` (22 files):** Matches: `CTextScroller::SetText/WordWrapAddLines/FindCR`, the `IconScroller` constructor (13 stores in a fixed order, because sched1 groups stores by register weight), `IconPanel::AnimateSelected`, `UIWidgetMenu::Scroll`, `ArrayScroller::ClearData/ForceSelectionOnScreen`, `feDialogScreen::NotifySoundMessage` and `TwoStageSlider::Draw`. `feScrollerina.hpp` records which helpers are inline: the ELF has no out-of-line `GetSelectedSlotIndex`, `MoveNext` or `MovePrev`. `CTextScroller.hpp` is this fork's own header for a class defined in `feWidget.cpp`.
- **`Frontend/MenuScreens/InGame`, `Loading`, `Career`, root (49 files):** Matches: `PhotoFinishScreen::Setup`, the `SillyTextureStreamerManager` constructor, `CustomTuningScreen::NotificationMessage/Setup`, `PauseMenu::NotificationMessage`, `FEKeyboard::UpdateStringVisual/GetLetterMap`, `uiSMSMessage::NotificationMessage`, `SplashScreen::NotificationMessage`, `EngageEventDialog::NotificationMessage` and `InitChyron`. `TuningSlider.{cpp,hpp}` is this fork's own split of that class, with its DWARF members (`Negative`, `Positive`, `Title`, `HelpBlurb`, `Min`, `Max`, `Current`, `Increment`, a 4-byte `bool bActive`). The line map places the class in `CustomTuning.cpp`, which `#include`s the split file.
- **`Frontend/MenuScreens/Safehouse/*` (86 files):** besides the entries above, these match: `GarageMainScreen::HandleTick/HandleJoyEvents`, `UIMain::NotificationMessage`, `uiRepSheetBounty::NotificationMessage`, `FEMarkerSelection::NotificationMessage`, `uiRepSheetRivalStreamer::TexturesLoadedCallback`, `UIOptionsScreen::NotificationMessage`, `DebugCarCustomizeScreen::NotificationMessage`, the `Showcase` and `UITrackMapStreamer` constructors, and `UIQRTrackSelect::ScrollTracks`.
- **`Frontend/MemoryCard`, `Frontend/MenuScreens/MemCard` (12 files):** these match: `MemcardCallbacks::Failed/SetAutosaveDone/CardChecked/SetMonitorDone`, `IJoyHelper::EmulateMemoryCardLibrary`, the `MemoryCard` constructor, `UIMemcardList/UIMemcardBase/UIMemcardMain::NotificationMessage` and `MemcardEnter`.
- **Where this fork is still behind upstream:** **GameCube:** only upstream matches `MyMutex::CreateInstance` and `MyThread::CreateInstance`. The original calls `memset` without a prototype (`crclr cr1eq`); upstream reproduces that by value-initializing the member in the init list (`mMutex()`, `mThreadData()`), while this fork calls `memset` explicitly. **GameCube:** 16 local-class helpers inside `FEPlayerCarDB` accessors, plus `IGenericMessage::_IHandle`, show 0 %: their names embed a per-unit counter (`...0_6Bounty.35689`) that this fork's build does not reproduce. Remaining near misses include `FEngine::Update` (98.3 %), `CustomizeMain::NotificationMessage` (96.6 %), `ArrayScroller::ScrollVer` (96.3 %) and `FEKeyboard::ToggleCapsLock` (91.1 %). **A124:** upstream has `EA_BUILD_A124` guards that this fork lacks, in `UnlockSystem.cpp` (`HasBeatenCareer`), `FEDatabase.*` (`TryAwardDemoMarker`, SMS flag values), `VehicleDB.hpp`, `feMinimap.hpp`, `uiRepSheetMain/RaceEvents/Milestones`, `uiEATraxJukebox`, `FELoadingTips` and `MemoryCardHelper.hpp`. **Names:** MemoryCard allocations still use `BNEW`, although the DOL contains upstream's labels (`"MemcardThread"`, `"MemcardMutex"`, `"Memcard Msg"`, `"Save Buffer"`, `"Load Buffer"`, `"Locale buffer"`). Upstream also names more constants (`FEObj_*`, `__PAD_LEFT__`, `RealmcIface::ID_*`), and this fork calls the DWARF's `FEngMemoryPoolHighwaterWarning` `FEngMemoryPoolUnknown`. **Header case:** the merge-base's lower-case `FEng/fengine.h` and `feimage.h` are renamed to upstream's `FEngine.h` and `FEImage.h`, and the includes spell each file name as it is on disk.

---

## Sound, speech and the EA audio middleware

Paths: `src/Speed/Indep/Src/EAXSound/` (units `zEAXSound`, `zEAXSound2`), `src/Speed/Indep/Src/Speech/`
(unit `zSpeech`, together with the four speech files of `EAXSound/Stream/`), and four EA libraries under
`src/Speed/Indep/Libs/`: `snd/9` (mixer, streams, AEMS, GameCube DSP driver, `coda` decoders), `path/5.01.04`
(interactive music), `spch` (speech event database and sentence picker) and `csis` (the interface registry
behind every `Csis::` handle). Against `upstream/main`, 157 files differ in `EAXSound/`, 29 in `Speech/`,
132 in `snd/`, 21 in `path/`, 13 in `spch/` and 4 in `csis/`. Upstream has headers but no code for `Speech/`,
`path/` and `csis/`, and nothing of `spch/`: all of that is written here. In `snd/` the fork completes six
empty or partial files and adds 109 files to the 50 upstream has. `EAXSound/` was almost empty at the fork
point and has been reconstructed independently in both trees since, so `git diff upstream/main` shows two
reconstructions side by side (diff against the fork point `59f184a0` to see only this fork's part: 148 files,
+18,756 lines).

Matched code per unit, measured with objdiff as upstream measures ("here" = this tree on 2026-09-18,
"upstream" = upstream's own `configure.py` at `fd0c355`, reproduced locally):

| unit / library | GameCube here | GameCube upstream | PS2 A124 here / upstream | PS2 SLUS here / upstream |
|---|---|---|---|---|
| `zSpeech` | 99.22 % (700 / 703 fn) | 0 % (0) | 40.94 / 0 % | 47.91 / 0 % |
| `zEAXSound` | 98.95 % (772 / 773) | 95.89 % (761) | 39.77 / 37.23 % | 41.34 / 34.36 % |
| `zEAXSound2` | 97.28 % (923 / 930) | 85.78 % (903) | 52.49 / 40.96 % | 49.08 / 43.63 % |
| `snd` (150 units) | 98.81 % (498 / 500) | 25.53 % (145) | 33.00 / 1.36 % | 13.83 / 0.52 % |
| `path` (14 units) | 77.94 % (181 / 194) | not built | 54.59 / 0 % | 47.02 / 0 % |
| `spch` (12 units) | 92.31 % (106 / 110) | not built | 28.34 / 0 % | not configured |
| `csis` (1 unit) | 81.56 % (52 / 57) | empty file | 0 / 0 % | 0 / 0 % |

On GameCube this tree matches 1,082 functions of `zSpeech`, `snd` and the two `zEAXSound` units that upstream
does not (700 + 351 + 31; the last 31 are 24.2 kB), plus the 339 of `path`, `spch` and `csis`, which upstream
does not build; upstream matches none that this tree lacks. On PS2 upstream matches two small `zEAXSound`
functions that this tree does not (`stSndDataLoadParams::Clear`, `UTL::Vector<EAX_HeliState *, 16>::push_back`).
Xbox 360: `zEAXSound` 2.12 % / `zEAXSound2` 13.96 % here against 0.48 / 3.10 % upstream; PC (measured here
only): `zEAXSound` 11.63 %, `zEAXSound2` 10.51 %, `zSpeech` 6.11 %. `matched_code` counts whole functions
only, which is why `path` and `csis` read
78–82 % with 91–93 % of their functions matched (`PATHI_serviceaction` alone is 3,252 B at 99.32 %).

Evidence used in this area: GameCube DWARF-1 (`symbols/mw_dwarfdump.nothpp`; `scripts/dwarf1.py`, and
`scripts/libdwarf.py`, which serves a middleware unit's types, prototypes and locals as compilable C), the line
map `symbols/debug_lines.txt` (unit membership, link order, statement order), `.debug_aranges`,
`.debug_pubnames` and the DWARF-1 CU map (units missing from `splits.txt`), `config/GOWE69/symbols.txt` and the
ELF symbol table (mangling, symbol scope, vtable slots), the original disassembly (`build/GOWE69/asm`), the A124
type dump, `.mdebug` stabs and disassembly for PS2 layouts (`scripts/sondastabs.py`, `scripts/cotejo.py`,
`scripts/desplazamientos.py`), `NFS.MAP` for PS2 placement, and three unmerged upstream branches used as raw
material (`eaxsound-clean`, `csis`, `ps2_fixes`), each checked against the DWARF before anything was kept.
Changes were checked on GameCube against the byte-identical DOL, and `scripts/audit.py` (bytes, branch
targets, relocation targets and addends, literal values) was run where objdiff's 100 % could hide a wrong
relocation.

### Notable files

**zSpeech: the police-radio speech system (`Speech/` and `EAXSound/Stream/`)**

- **`src/Speed/Indep/Src/Speech/SoundAI.cpp`, `SoundAI.h`** — What: the `SoundAI` director (2,695 lines;
  empty upstream). Source: started from upstream's unmerged `eaxsound-clean` branch, then corrected against
  DWARF-1, the line map and `symbols.txt`. How: `switch` trees rebuilt with GCC 2.95's pivot rule
  (`balance_case_nodes` pivots on the `(cases + ranges + 1) / 2`-th node; a `case` above the maximum emits
  nothing but still moves the pivot, so `UpdateStateMachines` needs `case kTerminal: break;`), integer
  `else if` chains turned back into switches (`GetRandomCop`, `GetRandomActiveCop`), split guards merged into
  the original single conditions, and 51 bodies reordered to the original's emission order. No function left
  below 100 %.
- **`src/Speed/Indep/Src/Speech/CsisSpeechEvents.h`** (new, 2,527 lines) **and `EAXCharacter.cpp`** — What:
  the 139 Csis speech events: one parameter struct each (from the DWARF), `InterfaceId`/`FunctionHandle`
  pairs, and `Csis::CacheHandlesEvents()`. Why: with the handles only declared, the unit's static initializer
  lost its 139 constructor calls, and a `static inline` stub returning `RESULT_OK` hid ~3,000 B of
  `Speech::Manager::Init2`. How: the handles are defined in `EAXCharacter.cpp` (the unit's sixth file); the
  body of `CacheHandlesEvents` was recovered from the original disassembly by reading its
  `lis rX, _4Csis.g<Event>Handle@ha` sequence in order.
- **`src/Speed/Indep/Src/EAXSound/Stream/SpeechManager.cpp`** (+ `SpeechManager.hpp`, `SpeechModule.hpp`,
  `GameSpeech.hpp`, `NISSFXModule.hpp`) — What: `Speech::Manager`, `Speech::Module`, `GameSpeech`,
  `SED_NISSFX` and the unit's 71 STL instantiations (2,406 lines; a 3-line stub upstream). Source:
  `eaxsound-clean`, fixed here: `SPEECH_BANK` lives in `namespace Speech` (the ELF mangles
  `PQ26Speech11SPEECH_BANK`; the branch had it global plus two hand-mangled `extern "C"` shims), wrong
  constants and relocation targets found by checking relocations (one was a `UCrc32` of the wrong message
  name), and 12 defensive `if (p == 0) p = "";` after accessors that already normalize (absent in the
  original). Note: `debug_lines.txt` splits this code over four files; here `GameSpeech.cpp`,
  `SpeechModule.cpp` and `NISSFXModule.cpp` are empty SourceList entries and everything is in `SpeechManager.cpp`.
- **`src/Speed/Indep/Src/Speech/EAXCop.cpp`, `EAXDispatch.cpp`, `EAXDispatch.h`** — What: the cop and
  dispatcher voices (1,140 + 357 lines), written in this fork (the branch's versions use `int` Csis fields and
  names that do not match the DWARF-generated structs). Evidence: `RBUpdate`/`RBReply` take
  `signed char true_false` (the ELF mangles `Sc`); `EVReply`, `VehicleVinyls` and `VehicleCustomPaint` are
  gone because the ELF's 19 contiguous `EAXDispatch` symbols do not include them. Left: `PursuitEscalation`
  98.44 %, `BreakAway` 95.75 %.
- **`src/Speed/Indep/Src/Speech/SpeechFlow.cpp/.h`, `PursuitFlow.cpp`, `StrategyFlow.cpp`,
  `RoadblockFlow.cpp`, `MusicFlow.cpp`, `Observer.cpp`** (+ headers) — What: the state machines that script
  the radio chatter (~3,200 lines; empty upstream). How: `SpeechFlow`'s constructor is out of class because
  every derived constructor calls `__Q26Speech10SpeechFlow`; `StrategyFlow::Waiting` got its three hidden
  `case` labels from the pivot arithmetic instead of by trial; constants imported wrong from the branch were
  fixed (six in `MusicFlow`, two of them swapped; two in `Observer::AssessOutcome`). Left:
  `RoadblockFlow::Setup` 95.87 %: in the RTL dumps two identical virtual calls tie at every level of
  `rank_for_schedule`, and the original resolves the tie in the opposite order.
- **`src/Speed/Indep/Src/Speech/SoundAI.h`, `EAXCop.h`, `EAXCharacter.h`, `Observer.h`** — PS2 layout, from
  layout probes against the A124 type dump and stabs (`desplazamientos.py` found this family reading members
  16–20 B late): the A124 `VoiceUsage` has six voiceIDs (no `cs_Alpine` battalion) and `SoundAI` has no
  `mTimeSinceLastChase` — `SyncPursuit` and `EAXCop::ReinitiatePursuit` recompute
  `WorldTimer - mT_sinceLastPursuit` (×1/4000) in the A124 disassembly, so `GetTimeSinceLastChase()` computes
  it on A124 and returns the member elsewhere. `ALIGNVEC` on `EAXCharacter::mPos`, `EAXCop::mTgtOffset` and
  `Observer::mGasStationPos`/`mFwPlayer`/`mFwRoad`. Afterwards every member sits at its A124 offset
  (`sizeof(SoundAI)` 0x2d0). The one A124 value not read from the binary (`t_lockout = 0`) is marked as such.

**`snd` 9: EA's sound library (`Libs/snd/9/`)**

- **`source/library/cmn/sndcmn.h`, `cmn/slinklist.h`, `include/snd/sndo.h`** — What: the declarations the
  new units need. `sndcmn.h` gains the private prototypes (`SNDVOICEI_*`, `SNDPLATFORM_*`, `SNDPKTPLAYI_*`,
  `SNDI_gettag` with `SNDTAGINFO` …), the public `SNDPKTPLAY_*` and `STREAM_*` C API, the globals
  `gSpeakerPositions`, `gTotalOutputChannels`, `gAzimuthFoldDownLut`, and real bodies for
  `Util::Az360To65536`/`Az65536To360` (empty upstream); `slinklist.h` gains `CListNode`, `CListStack`,
  `CListQueue` and a real `CListDStack::Reset()` (empty upstream); `sndo.h` gains the missing `SNDSTRM_*`
  entry points (`overheadtap`, `createtap`, `queuerequestid`, `timemult`, `drylevel`, `fxlevel`,
  `setgreedylevel`) — the rest of its diff is reordering. Evidence: DWARF prototypes; the ELF shows the public
  C API (`SNDPKTPLAY_*`, `SNDAEMS_*`, `STREAM_*`, `MIX_play`) unmangled although the library is compiled
  `-x c++`, while the internal `SNDPKTPLAYI_*` is mangled (calling a C symbol with C++ linkage still matches
  every instruction but does not link; defining it so leaves objdiff's side empty, 0 %). One documented
  deviation: `SNDPKTPLAY_framesoutstanding` stays `unsigned int` although its DWARF says `int`, because
  `sststat.c` divides it unsigned in the original.
- **`source/library/mix/`** (43 new files: `smixer.c`, `sfxrevc.c`, `stretch.c`, `sfir.c`, `sup*`, `sf*`,
  `smix*`) — What: software mixer, filters, FX buses/reverb, time-stretch. Source: DWARF types via
  `libdwarf.py` (hoists anonymous structs, orders them topologically, reverses array dimensions) and
  `build/GOWE69/asm`. How: 28 units written in one pass; 15 closed with `scripts/stmtorder.py`, which relies
  on the scheduler reordering stores to the same base but keeping stores to different bases in order. Also:
  `x *= C * y` is not `x = x * C * y` under `-ffast-math`; `sfxrevc`'s `tablecopyto[a - 1][b - 1][i][0]`
  folds the `-1`s into the relocation addend (`tablecopyto-252`), which objdiff does not compare. Left:
  `calcFIRCoeffs` 89.25 % (its header comment keeps the measured experiments).
- **`mix/smixport.h`** and the `sndptrint` typedef in `cmn/sndcmn.h` — What: LP64 support for the fork's
  Android port experiment (kept in the fork's working tree, not in the published snapshot). Why: `sndmix` is one global seen through four declarations (the full one in
  `smixer.c`/`sfxrevc.c`, a padded partial one in `smixi.h`, two-field views in `supf`/`suplf`/`suppf` and in
  `gc/snddrv.c`); byte-count padding does not grow with 8-byte pointers, so `sinit16.c` wrote an init function
  where `smixer.c` reads a filter. `smixport.h` gives one padding-free definition with both name vocabularies
  in anonymous unions and is used only under `__ANDROID__` (checked by `static_assert` tests in the
  port). The 25 `(int)pointer` casts in 13 units became `sndptrint` (`int` on GameCube and PS2:
  identical `.text`).
- **`gc/snddrv.c`** (a 5-line stub upstream, 1,725 lines here) **+ new `gc/sdspmix.c`, `gc/saramman.c`,
  `gc/ssdfx.c`** — What: the GameCube layer: AX voices and audio callback, DSP mixing, ARAM allocator, DSP
  effects. Evidence: DWARF locals — `SNDDRV_audiocallback` has `auxA` in r31, the parameter `context` in r4 and
  no lexical blocks, which ruled out locals that had been invented. Note: `dolphin/arq.h` cannot be included
  under GCC 2.95 C++ (typedef clash with `ar.h`), so `ARQRequest` is declared locally.
- **`cmn/sst.c`** (6 → 757 lines), **`spktplay.c`, `spat2hdr.c`, `sserver.c`** (stubs or partial upstream)
  **and 55 new `cmn/` units** (`stagpat.c`, `stpparse.c`, `ssys.cpp`, `ssysinit.c`, `satospkr.c`, `sst*.c` …) — What:
  streams, packet playback, patch parsing, system init and service. Data placement recorded in the sources:
  the `.bss` order of namespace-scope PODs follows the *first declaration* of each name, so `sserver.c` and
  `ssysinit.c` declare one global before including `sndcmn.h`; `static unsigned char freekey` goes to `.bss`
  without an initializer and to `.data` with `= 0`, and the original has it in `.bss` in `ssysinit` but in
  `.data` in `stagpat`; `lastTick` is a function-local static of `sserver.c` whose explicit `= 0` puts it in
  `.data`, as in the original.
- **`cmn/SNDI_root1x.c`** (and `cflags_snd_nofastmath`) — snd links as two blocks, each sorted by path; the
  second (`sgparse.cpp`, `SNDI_mult16.c`, `SNDI_root1x.c`, `eaxadecf.cpp`, `mtdecf.cpp`, `scrsfl.c`,
  `SNDI_findprime.c`) is a separate archive built without `-ffast-math`: `SNDI_rootof1plusx` with the DWARF
  locals `p1..p7` matches only without it, `SNDI_sin` (first block) needs it, and the other six are identical
  either way.
- **`extern/coda/cmn/eaxadecf.cpp`, `mtdecf.cpp`** (+ `coda.cpp`, `setmemcpy.cpp`, `mpeg*.cpp`, `lbmpeg.cpp`,
  `mtdecppc.cpp`) — What: the `coda` decoders `CEAXABLKDecf` (EA-XA) and `CMTBLKDecf`. How: `eaxadecf.cpp` did
  not exist; written from the DWARF (`CEAXABLKDecf` 0xA8, `EAXAVARSF` 0x90, `MXAPACKETF` 0x14 with every field)
  and from `EAXSound/Ginsu/ginsudata.cpp`, which carries the same XA algorithm and tables: 9/9 functions plus
  its tables. `mtdecppc.cpp` and `lbmpeg.cpp` were found through the DWARF-1 CU map; `PPCMtdec` (the SN
  profiler's decrementer) stays inline asm because it is `mfdec`/`mtdec` hardware code.
- **`cmn/saemstimupdt.c`** — GameCube-only inline-asm bridge for the AEMS timer clients, and asm in the
  original too: the target parks LR in r22 inside the loop, moves r20/r21 around the `blrl` and saves
  r20–r31, which no C call produces, and the line map puts that whole sequence on one source line (line 73,
  29 lines after the previous statement), the signature of an asm statement. The other platforms use
  upstream's plain C call.
- **`cmn/author.cpp`, `cmn/sndfxbus.cpp`, `cmn/srandom.c`** — `author.cpp` is a data-only CU (DWARF-1 has one
  variable, `sndlibauthor`, the build stamp of library version 9.06.00); `sndfxbus.cpp`'s `.rodata` is 8-aligned in the
  original and 4 in ours, which only a dummy `double` reproduced; the dummy is gone and the unit is NonMatching;
  `iSNDrandom` is the same multi-word add-with-carry generator as spch's `iSPCH_EACrandom`, and both stop at
  98.33 %.

**`path` 5.01.04, `spch` and `csis` (`Libs/path/`, `Libs/spch/`, `Libs/csis/`)**

- **`path/5.01.04/source/cmn/pathi.h`** (new, ~690 lines) — What: the library's private header:
  `PATHTRACK`, `PATHEVENT`, `PATHACTION` with its 16 unions, the `PATHFINDNODE` bitfields and family, six
  enums, `namespace Path` with ~20 globals, ~90 prototypes and the inline helpers (`PATHI_getevent`,
  `PATHI_getnode`, `PATHI_gettrackinfo` …). Why: upstream's 14 `.cpp` files are empty and this header did not
  exist. Source: DWARF-1 (the CU map names `C:/packages/path`) via `libdwarf.py`; `_4Path.pfstate` in
  `symbols.txt` is a namespace global (a class static would be `pfstate__4Path`).
- **`path/5.01.04/include/path/path.h`, `IPathTrack.h`, `IPathToSnd.h`, `IPathToReal.h`, `PathToSnd.h`,
  `PathToReal.h`** — What: headers corrected. `IPathTrack`'s accessors had empty bodies (`GetVolume() {}` …)
  and now return their members, as do `PathTrackSnd::UpdateStatus` (`mUpdateStatus = 1`) and `ModifyHold`
  (`return -1`); the virtuals the base does not implement are pure (`= 0`), as in the original. `path.h`
  declares the public `PATH_*` API (51 prototypes against upstream's 18) with the original's `USEREALCORE`
  switch (without it `PATH_setallocator` and `PATH_vectortoreal5/6` are error macros), so the game is compiled
  with `-DUSEREALCORE`. `PathToReal6::TaskService(void *, int)` follows its mangled name
  (`TaskService__11PathToReal6Pvi`). `IPathToSnd::sndimp` is in `.sdata` on GameCube (explicit section; the
  game uses `-G0`) and in `.data` on PS2 (`NFS.MAP`). `IPathToReal.h` is `#pragma interface`, paired with
  `#pragma implementation` in `pathreal.cpp`.
- **`path/.../pathsnd.cpp`, `pathreal.cpp`, `pathreal6.cpp`, `pathevent.cpp`** — What: the snd back end
  (`PathToSnd`, `PathTrackSnd*`) and the realcore file back end. Source: upstream's unmerged `csis` branch,
  merged by hand and measured function by function: `pathreal*.cpp` and `IPathToReal.h` whole, `pathsnd.cpp`
  without its private `extern "C"` block (it uses `snd/sndo.h`), `pathevent.cpp` keeping this tree's
  `PATH_clearallevents`/`PATHI_seteventfilter` (the branch's scored lower); the branch's `voiceID & 0x3f`
  masks in `PATH_addmapfile` are not in the target and were removed. The `PATH_AGG(x)` aggregate view (a
  `__typeof__` cast that changed alias analysis and emitted no code) was removed too: the `PathTrackSnd`
  constructor went from 100 to 88.57 % and `PathTrackSndStream::CheckStatus` to 98.81 %.
- **`path/.../pathaction.cpp`, `pathnode.cpp`, `pathbank.cpp`, `pathtrack.cpp`, `pathinit.cpp`,
  `pathvol.cpp`** (+ `pathcontrol`, `pathserv`, `pathrand`, `pathdebug`) — What: the music engine, from
  scratch. Findings: the event-queue loops run 17 times over a 16-entry array (`mtctr 0x11`), an original bug
  kept as is; a `switch` fall-through was written in the source as a duplicated body (GCC cross-jumps it);
  `lastwhile`/`lastendif` are statics of `PATHI_serviceaction` (DWARF); the original stores `0xBEDFACED` before
  setting up `LoadFile`'s arguments; ours schedules it later, and `PATHI_loadbankdata` is at 87.78 % since a
  `*(volatile int *)` store that forced the order was removed. Left, all at 100 % while the library used asm
  aliases, pins or barriers:
  `PATHI_serviceaction` 99.32 %, `CreateStreamTrack` 99.85 %, `PATHI_calcwaitbeat` 98.51 %, `PATHI_queuenode`
  97.54 %, `PathTrackSndBank::Play` 94.06 %, `PATHI_clearalleventfilters` 93.04 %, `PATHI_inittrack` 83.19 %,
  `PATH_addmapfile` 82.50 %, `PATHI_getmastertrack` 66.51 %, `PATH_setallocator` 43.44 %.
- **`spch/dev/source/library/cmn/spchi.h`** + 11 sources (all new) — What: the speech library (`SPCH_*`,
  `iSPCH_*`): banks, events, rules, random, sentence and sample picking. Evidence: CU map
  (`D:/perforce/audio/libs/SPCH`), DWARF types and locals; the `.c` files are built as C++ because every
  symbol is GCC-2 mangled (`SPCH_GetExtVecs__Fv`). Findings: `(x + 7) / 8` and `(x + 7) >> 3` emit the same
  `srawi` but different RTL, which changes allocation for the whole function (4 functions); five functions
  `spchcsis.cpp` calls are mangled in the original (declared `extern "C"`, they matched "100 %" while calling
  symbols that do not exist); `gAddEventStatus`, `gSPCHPlayStatus`, `gEventChoice` and the power-of-4 table
  `multiple` are file statics (local scope in `symbols.txt`); `enum SPCH_Rules {}` is empty in the DWARF too;
  the audit caught `iSPCH_InitBanks` writing
  `gNumBanks` where the original writes `gUniqueBankHandle` while objdiff said 100 %. Left:
  `iSPCH_ChooseSamples` 98.0 %, `iSPCH_MakeSampleRequests` 98.32 %, `iSPCH_EACrandom` 98.33 %,
  `iSPCH_GetSampleSizeData` 87.65 %.
- **`spch/dev/source/library/cmn/spchdata.c`** (new unit) — What: a data-only unit (zero-length `.text` in
  `.debug_aranges`; 48 B of `.data`, 520 B of `.bss`) that defines the 20 globals the other eleven units
  declare `extern`. Evidence: `.debug_aranges` and `.debug_pubnames` list it; `splits.txt` did not. How: the
  ten `.data` symbols carry an explicit `= 0` and the ten `.bss` ones none (under `-x c++` any initializer,
  even zero, moves a definition to `.data`); the file does not include `spchi.h`, whose `extern`s would move
  `gEventDats` to the start of `.bss`.
- **`csis/dev/source/library/cmn/csis.cpp`** (empty upstream, 653 lines), **`csisi.h`**, new
  **`csis/dev/source/extern/csisdef.h`**, **`include/csis/csis.h`** — What: `FunctionHandle`/`ClassHandle`/
  `GlobalVariableHandle` (`Set`/`SetFast`/`Valid`), `System` (init, lock, subscribe, allocation), `Function`,
  `Class`, `GlobalVariable`, `ClassData` and the `IAllocator`/`ICoreAllocator` adaptors; `csisdef.h` holds
  `FunctionDesc`, `SystemDesc`, `GlobalVariableDesc` from the DWARF. How: `Subscribe` and `Unsubscribe` call
  the real `gSystems.Push/Remove` inline (the DWARF lists only `pinterface`, `pSystemDesc`, `i` and an unsigned
  `ptr`) instead of a hand-expanded copy; both have the exact size (98.96 / 99.02 %), and what is left is cse's
  base choice inside the shared `slinklist.h`. `InterfaceId` in `csis.h` got a struct tag so MSVC accepts
  forward declarations. objdiff's data figure is 112 B low because this unit emits the weak `IAllocator`
  vtable copy that the original also emitted and the linker dropped (DWARF ranges `0xFFFFFFFF`).
- **Per-library compiler flags (`configure.py`)** — GameCube: `snd` keeps upstream's `-O2 -G0
  -fno-strength-reduce -fno-strict-aliasing -ffast-math -mps-float -x c++` plus the no-fast-math group above;
  `path` is `-O2 -G8 -fno-strict-aliasing -ffast-math -fno-implement-inlines` (its first version had copied
  `-G0 -fno-strength-reduce` from snd; dropping them gave 1,852 B and 10 functions with no source change, while
  the same change costs `spch` 9,152 B — each library is measured in both directions); `spch` is `-O2 -G0
  -fno-strength-reduce -fno-strict-aliasing -ffast-math -x c++`; `csis.cpp`, in the `misc` library, uses the
  game flags plus `-fno-implement-inlines`. PS2: the `.mdebug` names each file's EA package (PlayStation2SN
  2.8.1-9 for spch and csis, 3.0.2-1 for snd, 3.0.2-4 for path), whose SN ee-gcc saves the `s` registers with
  `sq`; snd, spch and path are therefore built with SN ee-gcc 2.95.3-136 (`PS2_TOOLCHAIN`), the only one of
  the available SN builds that also saves `ra` with `sd`. `csis.cpp` is not, and matches nothing on PS2 yet.

**EAXSound (`Src/EAXSound/`)**

- **`CARSFX/SFXObj_Pathfinder.cpp`** (818 → 1,364 lines) **+ new `CARSFX/SFXObj_PFEATrax.hpp`,
  `sfxctl/SFXCTL_NISReving.cpp`, `sfxctl/SFXCTL_MasterVol.cpp`, `States/Managers/STATEMGR_AICar.cpp`** —
  What: the functions upstream deferred because they read frontend state (its markers: "wait for the merge of
  zFE", "TODO UpdateNIS after we've merged FE"): EA Trax playlist and ambience (`SFXObj_PFEATrax::UpdateInGame`,
  `GenNextMusicTrackID`, `TestToAmbience`, `ProcessUpdate`, `UpdateAmbience`, `GenMusicType`, `GenEATraxState`;
  `TestToLicensed` 99.09 %), `InitializeEATrax`, `SFXCTL_Physics::UpdateNIS` (99.96 %),
  `SFXCTL_MasterVol::UpdateParams`, `CSTATEMGR_AICar::QueueSlots`, `EAXSound::GetCurMusicVolume`. Why: this
  tree has the frontend decompiled. Evidence: DWARF locals (`bool NoCops = FEDatabase->IsQuickRaceMode()` in
  `QueueSlots`, `newzone`/`peaxcar_0` in `UpdateAmbience`).
- **`States/Managers/STATEMGR_CarState.cpp`** — `CSTATEMGR_CarState::ResolveCarBanks` (7,508 B; upstream
  97.39 %) at 100 %: two changes that only work together — `found = true` placed before the declaration of
  the zero-initialized `EngToCarStruct` (which changes the register cse reuses) and the rule loops written
  `for (; n >= 0; --n)` with `goto BeginRuleN` restarts. Measured on the way: one forward `goto` out of a scope
  with a destructor gets its cleanup pad inline; two are cross-jumped and the last copy survives.
- **`Ginsu/ginsusynth.cpp`, `Ginsu/ginsudata.cpp`** — `GinsuSynthesis::HandlePacketRelease` (1,936 B) closed
  by one initializer, `float cycle = 0.0f;` (found by `scripts/permuter.py` guided by `scripts/regmap.py`);
  `GinsuSynthData::SampleToCycle` (upstream 84.91 %) is a single `for (;;)` with `if / else if / else break`
  (upstream: two nested `while (true)`), and `FrequencyToSample`/`CycleToSample` use early returns instead of
  a single `samp` exit. The A124 variants (packet size, `samplerep`, no null check) come from `ps2_fixes`.
  Left: `BindToData` 95.87 %.
- **`Dynamic_Mixer/NFSMixMapState.cpp`, `sfxctl/SFXCTL_Physics.cpp`, `sfxctl/SFXCTL_Engine.cpp`,
  `CARSFX/CARSFX_Skids.cpp`, `CARSFX/SFXObj_MomentStrm.cpp`, `CARSFX/CARSFX_Turbo.cpp`, `SFX_Common.cpp`,
  `EAXFrontEnd.cpp`, `EAXSndUtil.cpp`, `EAXTunerCar.cpp`** — near-misses closed here that are below 100 %
  upstream: `NFSMixMapState::InitializeMasterChannels`, `CreateEvtMixCtls`, `CreateMixCtls`,
  `InitializeSubChannels`; `SFXCTL_Physics::UpdateMixerOutputs` (1,212 B); `SFXCTL_Engine::UpdateRPM`;
  `CARSFX_Skids::ProcessUpdate`; `SFXObj_MomentStrm::ShouldStreamPlay`; `CARSFX_Turbo::PlaySpl`;
  `SFX_Common::MsgPlayMiscSound`; `EAXCommon::Play`; `Slope::Initialize`; `EAXTunerCar::UpdateRotation`.
  Patterns recorded in these files: `bClamp(x, A, B)` is not two `if`s (the inlined MIN/MAX keep the bound in a
  shared register on the left of `cmpw`); a non-`const` local divisor (`DistanceRolloffRange = 44.0f`) keeps
  `-ffast-math` from rewriting `x / C` as `x * (1 / C)`; `UMath::Max(a, b)` evaluates left to right.
- **`RealStream/src/stream.cpp`** — realcore's stream reader:
  `restartstream` (1,000 B) at 100 %; `startnextrequest` in upstream's four-case form and `static`
  (`symbols.txt` gives it local scope); compile-time size checks `REQUESTSTRUCT` 0x124, `TAPSTRUCT` 0x10,
  `FILTERSTRUCT` 0xC and `STREAMHEADER` 0x18C on GameCube / 0x180 on PS2 (A124 stabs; the PS2 `MUTEX` is 0x10).
- **`SoundConn.h`, `SoundConn.cpp`** — PS2/Xbox 360 layout (upstream has none of it): every `Pkt_*` class is
  `ALIGNVEC` (A124 packet sizes are multiples of 16; on Xbox 360 `Pkt_Effect_Open::Size()` returns 0x20
  against 0x18 on GameCube), and `CarSoundConn::mState`/`HeliSoundConn::mState` sit at 0x20 on PS2 and Xbox
  360 (0x18/0x14 on GameCube). `Pkt_Car_Service::mWheelZforce[4]` does not exist in the A124 (its stabs go
  from `mWheelLoad` to `mWheelOnGround`) and is guarded together with its setter and every use.
- **PS2 A124 support across `EAXSound/` and `Speech/`** (127 `EA_BUILD_A124` conditionals in 43 files and 54
  `ALIGNVEC` members in 21 headers; upstream/main: 8 conditionals in 2 files, no `ALIGNVEC`) — Source: the 64
  `EAXSound/` files of upstream's unmerged `ps2_fixes` branch, merged in batches with the DOL checked after
  each. Of the branch's 16 files that did not merge cleanly, only the `ALIGNVEC`s and the A124 variants of
  `NFSMixMap`, `NFSMixMapState` and `STATEMGR_AICar` were taken, with this tree's code in the `#else`: the
  branch's `#else` is Black Edition's code, not "every other build" — in `NFSMixMap` it sets
  `m_fDeltaTimeRatio` to `0.0f` where the GameCube DOL needs `1.0f`. Nine of its member guards were rejected
  because this tree uses those members in code the branch has not written — a member is guarded together with
  all its uses or not at all (two of them, `cs_Alpine` and `mTimeSinceLastChase`, were later guarded that way,
  see `SoundAI` above). Added here: guards on the uses of members the headers already excluded
  (`m_bPause_MainFNG` in `EAXSound.cpp`; `m_bClearSkipUpdate`, `m_bPathFAILED`, `m_FEPreviewEvent` … in
  `SFXObj_Pathfinder.cpp`) and the `SoundAI`/`SoundConn` layouts above.
- **Reconciliation with `eaxsound-clean` and with upstream's names** — the branch's 94 content hunks were
  judged by their identifiers against the DWARF: 88 kept this tree's side (in 76 the branch's names are absent
  from the DWARF and ours present, in 12 neither is), and of the 6 where the branch agreed with the DWARF, 4
  local names were adopted (`numskids`, `NoCops`, `newzone`/`peaxcar_0`, `fRelativeVel`); the branch's rename
  of `eSNDDATATYPE` to `EAXSND_DT_*` was rejected (the DWARF has `SDT_*`). In `sfxctl/`, 40 literals took
  upstream's constant names (`REDLINING_RPM`, `MAX_COMPRESSION_DURATION` …) only where our DWARF dump lists the
  `static const` and our DOL-matching code confirms the value; 17 upstream constants were left out because
  their values differ, 25 more because the value is too common to attribute.

### Everything else

- `EAXSound/CARSFX/` (33 files): `ALIGNVEC` and A124 guards on the SFX objects (`CARSFX_WindNoise.hpp` alone
  has ten aligned vectors: final, start, current and base position of each side plus the two deltas;
  `SFXObj_Collision`, `SFXObj_MomentStrm`, `SFXObj_Woosh` …), DWARF local names. `CARSFX_WindWeather.cpp` is in
  no SourceList (the compiled `CARSFX_WindWeather` code is in `CARSFX_WindNoise.cpp` in both trees).
- `EAXSound/sfxctl/` (26 files): the near-misses and named constants above, A124 guards, `ALIGNVEC` in
  `SFXCTL_Engine`, `SFXCTL_Tunnel`, `SFXCTL_Wheel` and `sfxctl_helicopter`.
- `EAXSound/States/`, `States/Managers/` (16 files): `CSTATEMGR_AICar::EnterWorld` writes `bMin(4, sum)`
  because this tree's `bMin` compares `b < a` (upstream flipped both body and call sites; doing that here
  breaks 52 other functions); A124 conditionals.
- `EAXSound/Stream/` (11 files): the speech files above; `EAXS_StreamChannel`, `EAXS_StreamManager` and
  `SndStrmWrapper` are this tree's own reconstruction (same functions matched as upstream).
- `EAXSound/` root (47 files): an independent reconstruction whose differences from upstream are mostly
  organization — sound enums in `EAXSoundEnums.hpp` instead of `EAXSOund.hpp`, `EAXCommon.hpp`, a partial
  `NFSMixMaster.hpp` view, `Csis.hpp` reduced to a forward to `csis/csis.h`; `EAXAemsManager.h` is upstream's
  file unchanged. Portability work brought these files to the forms upstream already uses (Dolphin calls
  behind bMath inlines, `Sound::Context` case labels in `DestroyEAXCar`, typed `std::vector` iterators),
  leaving the GameCube objects unchanged.
- `EAXSound/Dynamic_Mixer/`, `Ginsu/`, `SND_GEN/`, `SimStates/`, `UG/`, `Data/` (20 files): the mixer-state
  and Ginsu work above, A124 variants, `ALIGNVEC` in `EAX_HeliState`.
- Upstream-only in this area: `EAXEnvironment.cpp/.hpp` (added upstream after the fork point; not in this
  tree's `zEAXSound` SourceList) and the `CacheHandlesMAIN_AEMS`/`CacheHandlesTURBO` inlines in `SND_GEN/`
  (this tree writes the same `Set` calls directly in `EAXAemsManager.cpp`).
- File and include case: directories and file names follow upstream's spelling (`RealStream/`,
  `Dynamic_Mixer/`), and the includes this fork added spell each file as it is on disk. The mismatches left are
  upstream's own (for example `SND_GEN/copspeech.hpp` in `Misc/MWAttribUserTypes.h` for the file
  `SND_GEN/COPSPEECH.hpp`); the build tolerates them because wibo resolves the Windows compiler's paths without
  regard to case.
- `Speech/` headers and small files: `MiscSpeech.h` is the `MiscSpeech` class of statics the DWARF shows
  (upstream's 560-line catch-all header is replaced; its enums live in `COPSPEECH.hpp`); `SpeechCache.cpp`,
  `EAXAirSupport.cpp`, `SpeechFlow.cpp` written; `SpeechContainers.cpp/.h` keeps three empty destructors that
  are in the DWARF, but is in no SourceList.
- `Libs/snd/9/` (132 files): `cmn/` 55 new and 21 modified — five stubs completed, the headers,
  `saemstimupdt.c` and `sndfxbus.cpp` above, `SNDCTRL_getprogvol`/`SNDSTRM_getprogvol` scaling the volume by
  `127.0f` before `SNDI_ftoifast` (91 % upstream, 100 % here), and `(int)pointer` → `sndptrint` in the rest
  of upstream's files; `mix/` 43 new; `gc/` 3 new plus `snddrv.c`; `extern/coda/` 8 new.
- `Libs/path/5.01.04/` (21 files): 14 sources and `pathi.h` written, 6 public headers corrected.
- `Libs/spch/dev/` (13 new files) and `Libs/csis/dev/` (4 files), as above.

---

## Game logic: AI, physics, simulation, gameplay, camera and animation

Paths: `src/Speed/Indep/Src/{AI,Physics,Sim,Gameplay,Camera,Animation}/`. These six directories are
compiled by eight units (their SourceLists are in `src/Speed/Indep/SourceLists/`). At the fork point most
of `Gameplay/`, `Camera/`, `Physics/Common/` and `Physics/Dynamics/` were empty `.cpp` files, and upstream
still leaves them empty or outside its SourceLists: upstream's `zGameplay.cpp` and `zDynamics.cpp` include
no file, its `zPhysics.cpp` includes 3 of the 14 files and its `zCamera.cpp` 6 of 28. `AI/`,
`Physics/Behaviors/` and `Animation/` were already started, and both trees have reconstructed them since,
so `git diff upstream/main` there shows two independent reconstructions side by side (diff against the
fork point `59f184a0` to see only what was added here).

Matched code per unit, measured with objdiff exactly as upstream measures ("here" is this tree on
2026-09-18; "upstream" is upstream's own `configure.py` at `fd0c355` run locally, which reproduces
decomp.dev on GameCube but totals slightly less on PS2: 16.25 / 17.29 % against 17.31 / 18.09 %):

| unit | directory | GameCube here | GameCube upstream | PS2 A124 here / upstream | PS2 SLUS here / upstream |
|---|---|---|---|---|---|
| `zAI` | `AI/` | 99.38 % (1,029 / 1,030 fn) | 96.60 % (994) | 46.91 / 34.34 % | 53.13 / 29.69 % |
| `zPhysics` | `Physics/`, `Physics/Common/` | 99.02 % (717 / 718) | 1.76 % (11) | 66.99 / 1.33 % | 66.23 / 1.35 % |
| `zPhysicsBehaviors` | `Physics/Behaviors/` | 99.64 % (1,119 / 1,120) | 93.48 % (1,112) | 55.49 / 40.32 % | 58.31 / 43.90 % |
| `zDynamics` | `Physics/Dynamics/` | 93.46 % (40 / 41) | 0 % (0) | 10.04 / 0 % | 10.18 / 0 % |
| `zSim` | `Sim/` | 96.48 % (399 / 402) | 69.99 % (317) | 44.93 / 37.92 % | 59.48 / 41.91 % |
| `zGameplay` | `Gameplay/` | 96.73 % (763 / 768) | 0 % (0) | 58.09 / 0 % | 59.27 / 0 % |
| `zCamera` | `Camera/` | 92.86 % (447 / 453) | 3.27 % (24) | 18.59 / 0.69 % | 20.87 / 0.67 % |
| `zAnim` | `Animation/` | 97.84 % (314 / 315) | 94.58 % (308) | 36.23 / 32.28 % | 30.79 / 26.31 % |

On GameCube this tree matches 2,062 functions of these units that upstream does not, and upstream matches
none that this tree lacks. On PS2 upstream matches ten that this tree does not, all in the retail build
(listed under AI, Simulation and Animation). On Xbox 360 the eight units are at 0.1–8.6 % here against
0–4.4 % upstream; there and on PC much of the limit is function names, not code. Nineteen GameCube
functions remain below 100 % in these units; each area lists them.

Evidence comes from GameCube DWARF-1 (`symbols/mw_dwarfdump.nothpp`, `scripts/dwarf1.py`; "DWARF line N"
below is a line of that dump), the line map (`symbols/debug_lines.txt`, fused with the original
disassembly by `scripts/fuse.py`; `scripts/plan.py` adds the inline tree), the GameCube ELF symbol table
and vtables (`scripts/symtabdiff.py`, `scripts/ghostref.py`), the A124 stabs (`scripts/sondastabs.py`,
`scripts/cotejo.py`), its type dump (`symbols/PS2/PS2_types.nothpp`, `scripts/sonda.py`) and
disassembly, and Xbox 360 decorated names. Four mechanisms recur in these files:

1. **The vtable gate.** GCC 2.95 emits `_vt.<Class>` and the out-of-line copies of in-class virtuals only
   in the unit that defines the class's first non-inline virtual (its key method), so writing a class
   starts there (`PVehicle::Launch`, `~PhysicsObject`, `CubicCameraMover::IsHoodCamera`,
   `~CDActionTrackCar`). A virtual declared, not pure and never defined leaves the vtable undefined: the
   ELF has `__pure_virtual` in those slots of `_vt.11AIAvoidable`, `_vt.14ITrafficCenter` and
   `_vt.Q33Sim9Collision9IListener`, so the three methods are `= 0` here.
2. **Template static data needs an initializer.** `Listable<T,N>::_mTable` and similar members defined
   without `= T()` go to COMMON, and GCC then emits neither the vtable nor the methods of that
   instantiation (about 195 functions in `zPhysics`). The interface tables no `.cpp` owns (the line map
   gives them no file) are defined at the top of `SourceLists/zPhysics.cpp`, in construction order.
3. **PS2 alignment is per member, never per type.** `ALIGNVEC` aligns to 16 on PS2 and Xbox 360,
   `PS2ALIGN16` on PS2 only; the X360 offset decides which. The original copies `UMath::Vector4` with
   `ldl`/`ldr`, and the A124's `SmackableParams` (0x5c) and `XenonEffectDef` (0x58) contain a
   `UMath::Matrix4`, so neither type is aligned: upstream aligns both types, this tree aligns the members
   that have padding in front of them in the A124.
4. **Members the A124 prototype lacks** are guarded with `#ifndef EA_BUILD_A124` together with every use
   (a sub-condition, an `if` header or an early exit rather than a duplicated block), each use checked
   against the prototype's code. `EA_BUILD_A124` is defined for `SLES-53558-A124` only; upstream also defines it
   for the retail `SLUS-21351`.

### AI (`src/Speed/Indep/Src/AI/`)

At the fork point `AIPursuit.cpp` and `AIVehicle.cpp` were partly written and `AIMath.cpp`,
`AIRoadBlock.cpp`, `AIRoadBlockSetups.cpp`, `AISpawnManager.cpp` and `Gps.cpp` were empty; this tree and
upstream then completed `zAI` independently. On GameCube 35 functions (7.6 kB) match only here:
`AICopManager::UpdatePursuits` (2,516 B), `Gps::Render` (1,808 B, commented out upstream) and template
instances the original emits in this unit, such as `_STL::find<Sim::IEntity **>`. On PS2 the gap (182
functions in the A124, 226 in SLUS) comes mostly from the layout work below. Five `zAI` functions took
upstream's form after measuring it equal or better (`AIVehicle::OnGasBrake` with
`in_shock ^ in_neutral`, `AISpawnManager::GetBasePosition` with `eGetView(1, false)`, `~AIPursuit`
leaving the reset to `~GroundSupportRequest`…).

#### Notable files

- **`AI/AIPursuit.h`, `AI/Common/AIPursuit.cpp`** — What: `mAllowStatsToAccumulate` (GameCube offset
  0xD0) is under `#ifndef EA_BUILD_A124` together with its seven uses (constructor, `OnDetached`,
  `IncNumCopsDestroyed`, `TrackVehicleCounts`, two in `OnTask`, `NotifyCopDamaged`); `mLastKnownLocation`
  is `ALIGNVEC`. Why: every A124 access past `mHeatScale` was 4 bytes off. Evidence: the A124 stabs put
  `mInFormationTimer` right after `mHeatScale`, and the prototype accumulates pursuit statistics always
  (`NotifyCopDamaged` increments unconditionally, `IncNumCopsDestroyed` has no early return). How:
  `sondastabs.py` flagged the class, `cotejo.py` named the member, the A124 disassembly settled each use.
  `AssignClosestOffsets` (1,684 B) is at 99.67 %: it reached 100 % only with an inline-asm barrier, now
  removed; its comment records why (GCC's `combine` fuses a copy into `or.`, the original compares a
  temporary, and `--x > 0` is excluded by how `expand_increment` works) and every form measured.
- **`AI/Common/AIVehicle.cpp`** — What: `road_walker` (class, `walk_road`, `walk_all_paths`,
  `evaluate_end`), `path_spot` and `AIVehicle::UpdateRoads` written from DWARF line 93888 (`road_walker`,
  0x9C) and `fuse.py`; they pulled in 13 `_STL::sort` instantiations. PS2: `direction`, `futurepoint`,
  `targetpoint` and `startpoint` are `ALIGNVEC`; the class is local to the `.cpp`, so only a probe run in
  the context of `SourceLists/zAI.cpp` sees it. Xbox 360: `WRoadLane::GetBits`/`GetBitsSigned` are back in
  their header, because MSVC cannot inline them from here (three `WRoadProfile` lane functions went to 100 %).
- **`AI/Gps.cpp`** — What: class `Gps` (`Sim::Activity`, `Singleton<Gps>`, 0x388, DWARF line 115431) is
  declared in the `.cpp`, as in the original (the line map puts its `USE_FASTALLOC` inlines at
  `Gps.cpp:17`); 12 of 12 functions including `Render`. Its position in the SourceList comes from `.bss`:
  the singleton's instance pointer follows `AISpawnManager`'s statics. The unit's static initializer
  (9,380 B) matched once `Tweak_ForceGPSArrowTo` got its original initial value: the float-register
  rotation there was the shadow of one wrong constant, not an allocator limit.
- **`AI/AIRoadBlock.h`, `AI/Common/AIRoadBlock.cpp`, `AI/Common/AIRoadBlockSetups.cpp`** — What: the
  `AIRoadBlock` class rebuilt from DWARF line 43420 (`Sim::Activity` + `IRoadBlock`, 0xB4, 12 members);
  the header only had the setup structs. 27 of 27 functions matched at the first compile. The two roadblock
  candidate tables are built at run time into `.bss`, so their contents were recovered by simulating the
  original static initializer (a small interpreter of `li`/`lis`/`addi`/`lfs`/`lwz`/`stw`/`stfs`). PS2:
  `mRoadBlockDir` and `mPerpCheatPoint` are `ALIGNVEC`.
- **`AI/Activities/AICopManager.hpp`, `AICopManager.cpp`** — Access specifiers follow the Xbox 360 binary:
  MSVC encodes `private`/`public` in decorated names, so eight methods (`RemoveActiveCopVehicle`,
  `SpawnPursuitCar`, `SpawnCopCarNow`, `SpawnVehicleBehindTarget`, `GetSpawnPositionAheadOfTarget`,
  `GetAvailableCopVehicleByClass`/`ByName`, `GetActiveCopVehicleFromOutOfView`) are private; GCC does not
  mangle access, so GameCube and PS2 do not move. An explicit instantiation of
  `_STL::find<EAX_HeliState **>` (GameCube and PS2 only) provides the copy the original unit contains. The
  class sits in a header of this tree that only its `.cpp` includes; the original has no such header
  (DWARF file table), and upstream now declares it in the `.cpp`.
- **`AI/AIVehicle.h`, `AIVehicleHelicopter.h`, `AIVehiclePursuit.h`, `Actions/AIActionRace.cpp`,
  `Actions/AIActionTraffic.cpp`** — PS2 layout: `AIVehicleHuman::vMomentPosition`,
  `AIVehicleHelicopter::mLookAtPosition`/`mLastPlaceHeliSawPerp`, `AIVehiclePursuit::mInPositionOffset`
  and `AIActionRace::mLastFindPosition` are `ALIGNVEC`; `AIActionTraffic::mNavMatrix` is `PS2ALIGN16`
  (8 bytes of padding). They came from `alinvec.py --verifica` (14 real cases among 71 candidates;
  aligning the other 57, which already matched, would have broken them) and from `sondastabs.py`; the
  probe afterwards matches byte for byte (tree-wide batch: SLES +132 / SLUS +145 functions).
  `AIActionRace::mPerpetrator` and `CheckSpeedTraps` do not exist in the A124 and are guarded; there
  `ComputePotentials` asks the AI for the perpetrator interface (`GetAI()->QueryInterface`) instead of
  reading the member. The rest of the A124 version (`mBottleTime`, another `GetPotentialNOS`, an older
  NOS block in `Update`) is still open (see Open fronts).

#### Everything else

- `Actions/`: the 16 action files exist in both trees; here `AIActionPursuitOffRoad::ShouldDoIt` and its
  four `Update*` are private (Xbox 360 names).
- `Common/`: `AIMath.cpp` (5 functions) and `AISpawnManager.cpp` (9, including `CheckSpawnPosition` and
  `RefreshSpawnData` once `WRoadNetwork.h` exposed its accessors) written from empty, in the link order
  AIPursuit, AIRoadBlock, AISpawnManager, AIMath, Gps; `AIGoal.cpp` defines `~AIGoalStaticRoadBlock` out of
  line (the only `AIGoal*` destructor the original emits out of line); `AITarget::TrackInternal` is private
  (Xbox 360); `AIPursuit::EvenOutOffsets` uses typed iterators, so it compiles with MSVC.
- PS2 retail: two `WCollisionTri` vector helpers instantiated in `zAI` (`_M_insert_aux`,
  `__uninitialized_copy_aux`) match in upstream's SLUS build and not here; this tree aligns
  `WCollisionTri`'s points to 16 following the A124 stabs.
- Left on GameCube: `AIPursuit::AssignClosestOffsets` (99.67 %).

### Physics (`src/Speed/Indep/Src/Physics/`)

`Physics/Common/` (PVehicle, PhysicsObject, Smackable, SmokeableInfo, Explosion, Bounds, Behavior,
VehicleBehaviors), `PhysicsUpgrades.cpp`, `SmackableTrigger.cpp`, `PhysicsTunings.cpp` and the three
`Physics/Dynamics/` files were empty at the fork point and are not compiled upstream. They are written
here, and `SourceLists/zPhysics.cpp` lists the 14 files in the link order read from the line map
(SmackableTrigger, Explosion, PVehicle, PhysicsObject, Smackable, Wheel, VehicleBehaviors, VehicleSystem,
Behavior, Bounds, SmokeableInfo, PhysicsUpgrades, PhysicsInfo, PhysicsTunings). `Physics/Behaviors/` was
already at 93 % upstream; there the changes are corrections, the static initializer and the PS2 layout.

#### Notable files

- **`Physics/Common/PVehicle.cpp`** — What: the vehicle class (121 symbols) from an empty file. It is
  declared in the `.cpp`, as in the original: the line map puts all its inline bodies at
  `PVehicle.cpp:293-389`, and `PVehicle.h` only holds `VehicleParams` (its `VPF_*` flag values come from
  the DWARF). The key method is `Launch()`; defining it emitted `_vt.8PVehicle` and about 40 in-class
  virtuals. `IVehicle.h` had two wrong signatures (`IsGlareOn` is not `const`; `GetPosition` returns
  `const &`). PS2: the A124 has no vehicle-cache name at all — no `mCacheName` (its stabs end the class at
  `mPerformanceValid`, 0x280), no constructor parameter (its mangled constructor ends in `Performance`),
  no `IVehicle::GetCacheName` (its `IVehicle` vtable ends in `GetPerformance`) — so all three are guarded;
  `mLocalVel` is `ALIGNVEC` (A124: 0x200, with 12 bytes of padding after `mWheelsOnGround`).
- **`Physics/Common/PhysicsObject.cpp`, `Physics/PhysicsObject.h`** — What: written in the function order
  of the original `.s`; the 20 in-class virtuals of the header only appear once `~PhysicsObject()` is
  defined. `UTL::COM::IUnknown` places `_mCOMObject` at +0 and the vptr at +4. Left:
  `PhysicsObject::PhysicsObject(const char *, ...)` at 99.24 % (1,436 B); it was 100 % only with inline asm.
- **`Physics/Common/Smackable.cpp`, `Physics/Smackable.h`, `Physics/SmackableTrigger.cpp`** — What:
  written from empty. `UTL::Collections::Listable<T,N>::Sort` became a member template: the ELF instantiates
  `std::sort` with a `const T *` predicate for `Smackable` and a `T *` one for `ITrafficCenter` (13
  functions). `Smackable::Smackable` (3,120 B) matches with `Behavior *rbbehavior = NULL;` in its `else`
  block: the DWARF lists exactly `rbbehavior // r11` and `rbparams // r1+0x60` there, and the target's
  `li r11,0` is that initialization, reused by CSE for an argument. The zero-byte asm statements and the
  invented local that had forced the match were removed; the unused `UCrc32 smack_class` stays because the
  original has it (r1+0x58).
- **`Physics/Common/SmokeableInfo.cpp`, `Physics/SmokeableInfo.hpp`** — What: `HeirarchyModel`,
  `SceneryModel` and `PlaceableScenery` from an empty file. Evidence used: GCC 2.95 emits the out-of-line
  copies of in-class inlines in a block at the end of the unit (from 0x8022C000 in `zPhysics`), so a
  virtual whose address falls there was defined inside its class; the copy carries the line of its point
  of use. PS2: `HeirarchyModel::mTriggerAvoid` is `ALIGNVEC`, which also places `SceneryModel` and
  `PlaceableScenery`.
- **`Physics/PhysicsInfo.cpp`, `Physics/PhysicsInfo.hpp`** — What: from 9 kB to 35 kB. `PerfStats`,
  `PerfLevel`, `PerformanceMaps`, `TheStockCars` and `PerformanceWeights` were rebuilt from the DWARF,
  which unlocked `ComputePerformance`, `EstimatePerformance`, `PerfStats::Fetch`,
  `ComputeAccelerationTable` and `Physics::Info::Init`; 25 of the first 26 matched at the first compile.
  The DWARF iterator types (`_STL::_Const_traits` against `_Nonconst_traits`) tell `const_iterator` from
  `iterator`, which decides whether the iterator lives in a register and the frame size.
- **`Physics/PhysicsUpgrades.cpp`, `Physics/PhysicsUpgrades.hpp`** — What: written from empty; the
  header's API was completed (for example `SetLevel` returns `bool`, and `GetPercent`, `Validate`,
  `RemovePart`, `MatchPerformance` were missing). `MatchPerformance` needed plain
  float literals (declared as `extern const float` labels, GCSE hoisted the `lis` into two predecessors)
  and a ternary for its range, because the DWARF shows no intermediate local.
- **`Physics/Common/Explosion.cpp`, `Bounds.cpp`, `Behavior.cpp`, `Physics/Explosion.h`, `Bounds.h`,
  `Behavior.h`** — What: `Explosion.cpp` matched 18 of 19 at the first compile from the PS2 type dump;
  `CollisionGeometry::CreateJoint` (3,708 B) was written with `plan.py --dwarf-only` and `fuse.py`, 926 of
  927 instructions at the first compile. `Behavior.h` gains the constructor, `Pause` and `SimulateTask`,
  and `BehaviorSpecsPtr(ISimable *, int)` looks its key up instead of passing 0.
- **`Physics/Dynamics/Collision.cpp`, `Geometry.cpp`, `Articulation.cpp`, `Collision.h`, `Inertia.h`,
  `Articulation.h`** — What: `zDynamics` from zero. The inlines the original expands were rebuilt from the
  line map (`inertia.h:50/51`, `collision.h:414/429`: `Moment::GetLeverArm` returning `UVector3` by value,
  `GetInverseWorldInertia`, `Tensor::AddPointMass`, `ParallelAxis`); they were located because GCC
  attributes the setup of an inline's parameters to the callee's declaration line. Float constants are
  plain literals with the values read from the ELF: declaring them as `extern const float` labels changed
  register allocation (`Friction::GetForce` only reached 100 % with the literal). PS2:
  `Articulation::Constraint::mOrientation` is `ALIGNVEC`. Left: `Geometry::SphereVsBox` 98.49 % (1,896 B),
  100 % only with an asm barrier before; the comment keeps the measurements.
- **`Physics/Behaviors/SuspensionRacer.cpp`** — What: two behaviour fixes objdiff cannot see.
  `DoHumanSteering` indexes `SteerInputRemapTables[steer_remapping - 1]` (the selector is 1-based; the
  drift value 4 would overflow a 0-based array of four), and `SteeringRangeCoeffTable` is built from
  `SteeringInputRangeData`. Evidence: `&a[i - 1]` folds into the relocation addend, which objdiff does not
  compare, so upstream's `[steer_remapping]` also scores 100 %; the DWARF gives `SteerInputRemapTables[4]`
  at its address and all 125 instructions match byte for byte. `DoWheelForces` is at 100 % (upstream
  99.95 %).
- **`Physics/Behaviors/RigidBody.h`, `SimpleChopper.cpp`, `SuspensionSpline.cpp`, `SoundCar.cpp`,
  `Physics/Wheel.h`** — PS2 layout: `RigidBody::mInvWorldTensor` (8 bytes of padding) and
  `SuspensionSpline::mNISPosition` (12) are `PS2ALIGN16` `Matrix4` members;
  `RigidBody::Primitive::mOrientation`, the six `Vector3` members of `SimpleChopper` (16-byte stride
  0xa0–0xf0 in the A124, 12 on GameCube) and `Wheel::mNormal`/`mIntegral` (`Wheel` is 0xD0 with
  `mIntegral` at 0xC0 in the PS2 dump) are `ALIGNVEC`; aligning the `Vector4` type instead fixed the
  tyres but cost 16 functions in `zWorld2` alone. The A124's `SoundConn::Pkt_Car_Service` has no
  `mWheelZforce[4]` (stabs: `mWheelLoad`, then `mWheelOnGround`), so `SoundCar` does not call
  `SetWheelZForce` there. `SimpleChopper::SetTorqueToMatchPitchAndRoll` keeps the four locals the DWARF
  lists (clamp the acceleration, then copy it into the angle), where a 100 % version without them existed.
- **`Physics/Behaviors/SuspensionTraffic.cpp`** — Code as upstream plus the real profile name, and an
  analysis of `Tire::UpdateLoaded` (97.21 %) from `cc1plus` RTL dumps: PRE hoists the `lis` of `1.0f` to
  where the target has it, `cse2` undoes it (a `HIGH` costs 0 against 1 for the pseudo) and
  `update_equiv_regs` sinks it; DWARF locals and inline tree equal the original's, so it is not the source.

#### Everything else

- `Physics/Behaviors/`: the unit's static initializer (4,464 B) matches (upstream 45 %). The cause was how
  a static array member of a class template is defined: `template <> T X<A>::m[N] = {}` unrolls the
  constructor calls, `template <> T X<A>::m[N];` is only a declaration (undefined symbol), and
  `T X<A>::m[N];` defines it and emits the constructor loop the original has.
  `SuspensionSimple::DoWheelForces` and `Tire::UpdateLoaded`, `SuspensionSpline::SetNISPosition` and
  `DamageRacer::CanDamageVisuals` (reads `FEDatabase->GetGameplaySettings()->Damage`) are at 100 %;
  `SuspensionSpline` calls `OrthoInverse` through a static inline because the line map attributes the call
  to `UMath.h:680`.
- `Physics/Common/`: `Wheel::Wheel`, `Reset`, `InitPosition`; `VehicleSystem.cpp`, `VehicleBehaviors.cpp`,
  `PhysicsTunings.cpp`.
- Left on GameCube: `PhysicsObject::PhysicsObject` (99.24 %), `Geometry::SphereVsBox` (98.49 %),
  `SuspensionTraffic::Tire::UpdateLoaded` (97.21 %).

### Simulation (`src/Speed/Indep/Src/Sim/`)

`zSim` is at 70 % upstream and 96.48 % here. The additions are functions absent from the tree
(`LocalPlayer::UpdateHud` 7,176 B and `DoRadar` 1,944 B, most of `NISActivity`, `QuickGame::IsStateDone`,
`CanSimulate` and `CreateCars`, `OBB::BoxVsBox`/`SphereVsBox`, `Util_GenerateMatrix`, `Entity::Attach`),
the static data that makes the garbage collectors' template instances appear, the unit's static
initializer (1,956 B, 12.86 % upstream) and the PS2 layout of `SimSystem`.

#### Notable files

- **`Sim/Common/Simulation.cpp`** — What: `SimSystem` (declared in this `.cpp`) gets `PS2ALIGN16` on
  `mEventData` (`ESimulate::StaticData`) and `ALIGNVEC` on `mCameras[2]`. Evidence: in the A124,
  `mEventData` sits at 0x40 right after `mState` (0x34); the stabs list no field in between and the
  original constructor never writes 0x38–0x3c, so it is padding; Xbox 360 keeps it at 0x30 like GameCube,
  hence the PS2-only macro. How: `sondastabs.py` run from `zSim.cpp`; afterwards 11 of 11 fields and the
  size (0xb0) match. Also: `SimTask::mNextHandle` starts at 1 (value read from the target object's `.data`).
- **`Sim/Simulation.h`** — What: includes `IPlayer.h`, `IOnlinePlayer.h` and `IEntity.h`; the methods of
  `ITimeManager` and `IStateManager` are pure. Evidence: the DWARF type order of `zAI` (ISimable, IPlayer,
  IOnlinePlayer, IEntity, SimRandom) and the A124 stabs (the three always consecutive in the 24 units that
  define `IPlayer`, followed by SimRandom in 22). Result: `zAI` emits `_STL::find<Sim::IEntity **>` right
  after `find<IPlayer **>`, as the original.
- **`Sim/Entities/LocalPlayer.cpp`, `LocalPlayer.hpp`** — What: `UpdateHud` (1,794 instructions over
  source lines 312–780) and `DoRadar` transcribed from the line map with the DWARF locals. The
  initializer of `mLastPursuit` now carries the same guard as the member
  (`#ifndef EA_PLATFORM_PLAYSTATION2`); keyed on `EA_BUILD_A124` it initialised a field the retail PS2
  build does not have.
- **`Sim/Activities/QuickGame.cpp`, `QuickGame.h`** — What: `CarBuilder` (0x1B4: `Key` 0x0, `Flags` 0x4,
  `Customization` 0x8, `Performance` 0xC, `mCustomization` 0x10, `mPerformance` 0x1A8) existed nowhere and
  was written from the code that uses it. The declaration order of its two in-class `Match` overloads
  decides whether GCC inlines one into the other (an in-class body is compiled in declaration order and can
  only be inlined into a later one); with `Match(float)` first, `Match(const Performance &)` keeps its
  symbol as in the original. Left: `CreateCars` 99.75 % (one `mr` in the temporary
  `WCollisionMgr(0, 3)`) and `OnManageTime` 94.01 % (the statement order that fixes its literal-pool
  order changes its code).
- **`Sim/Activities/NISActivity.cpp`** — What: `PrepareVehicles`, `Load`, `UpdateLoading`,
  `UpdatePlaying`, `Release`, `Play` and `ResetEvents` written (about 21 kB).
- **`Sim/Common/SimEntity.cpp`** — `Sim::Entity::Entity` passes `this` to `IEntity` on every platform (99.86 %
  on GameCube; a GNU statement expression that reached 100 % was removed). `Entity::Attach` and
  `GarbageNode<Sim::Entity, 8>::_mCollector` (with initializer) are new.

#### Everything else

- `Common/OBB.cpp`: `BoxVsBox` (from `fuse.py`, first compile), `SphereVsBox`, `CheckOBBOverlap` (a bare
  `fabs` is the builtin, `fmr`+`fcmpu`+`cror`+`bso`+`fneg` is `VU0_fabs`; both occur here). `Util.cpp`:
  `Util_GenerateMatrix`. `SimSurface.cpp`: `GetParentSurface`.
- Headers: `ConnStatus` moved to `SimConn.h`; accessors that were empty `{}` placeholders return their
  member (`IsRendering`, `IsSimulating`, `SimCollisionMap::Type`); `SimCollisionMap` has its constructor;
  `OnDraw` is pure; `Sim::Update`, `CanSpawnSimpleRigidBody` and `CanSpawnRigidBody` are declared.
- Retail PS2: `LocalPlayer::GetSettings` (48 B) matches in upstream's SLUS build and is at 99.92 % here.
- Left on GameCube: `QuickGame::CreateCars` (99.75 %), `QuickGame::OnManageTime` (94.01 %),
  `Sim::Entity::Entity` (99.86 %).

### Gameplay (`src/Speed/Indep/Src/Gameplay/`)

All 17 `.cpp` files were empty at the fork point and upstream's `zGameplay.cpp` still includes none of
them, so the unit is at 0 % upstream on every platform; here it is at 763 of 768 functions on GameCube,
674 on the A124 and 638 on the retail PS2. The files were transcribed from the line map fused with the
original disassembly (`fuse.py`: 32 of the first 36 `GManager` functions matched at the first compile),
with the DWARF for inline trees and locals; the recurring blockers were support types the tree lacked
(`GObjectIterator<T>`, the `Attrib::Vault` export API, `PackedDecimal`) and headers with wrong accessors.

#### Notable files

- **`Gameplay/GManager.cpp`, `GManager.h`** — What: 86 kB from an empty file. PS2: the A124 has no "free
  roam from the safe house": `mFreeRoamFromSafeHouseStartMarker` and `mStartFreeRoamFromSafeHouse` are
  guarded together with their initializers, the save and load blocks, the reset, the assignment in
  `BeginGameplay` and the choice in `GetRespawnMarker`; the two in-class setters keep an empty body there,
  so their callers need no change. Evidence: the A124 dump puts `mStartFreeRoamPursuit` right after
  `mFreeRoamStartMarker`, and the prototype's code differs exactly there (`SaveGameplayData` 4 `bMemCpy`
  against 5, `LoadGameplayData` 3 against 4, `ResetAllGameplayData` 55 instructions against 58,
  `BeginGameplay` 96/98, `GetRespawnMarker` 71/73): its save format has no garage marker. The virtual
  `GetCacheName` is absent too (the A124 vtable is the destructor plus the two `IVehicleCache` methods).
  `mRestartEventHash`, also absent, is left in: it is the last member and only changes the size (0x320
  against 0x310). How: `desplazamientos.py`, a probe against the A124 dump, the A124 disassembly. Left:
  `AllocateInstanceMap` 98.07 %, `GetStrippedNameKey` 99.6 %, and the 16-byte comparator class local to
  `GatherVisibleIcons`, whose symbol ends in a compiler counter suffix, so objdiff cannot pair it by name.
- **`Gameplay/GRaceDatabase.cpp`, `GRaceDatabase.h`** — What: 96 functions (`GRaceParameters`,
  `GRaceCustom`, `GRaceBin`, the race index). The 55 getters of `GRaceParameters` call `EnsureLoaded()`
  out of line because the original defines it after them (its line 955, the getters 892–949): it is an
  `inline` definition placed after the getters in the `.cpp`; as a header inline all 55 differed.
  `PackedDecimal` (`Misc/PackedDecimal.h`) was rebuilt from the DWARF. `GRaceCustom::SetCopDensity`
  exists only in the A124, with an empty body (`jr ra; nop` in its ELF). Left:
  `GRaceParameters::GenerateIndex` 98.96 %.
- **`Gameplay/GRaceStatus.cpp`, `GRaceStatus.h`** — What: written from empty. `GRacerInfo` has an inline
  constructor `{ ClearAll(); }` (the array construction loop calls it). PS2: the A124 has no split times
  and no DNF (`mSplitTimes`, `mSplitRankings`, `mDNF`, guarded upstream already); here their setters,
  getters, reset loops and uses are guarded too — one unguarded accessor broke 18 of the 34 PS2 units —
  and so is the `IsFarLoadingInProgress()` block.
- **`Gameplay/GTrigger.cpp`, `GTrigger.h`, `GMarker.h`** — `GTrigger::mDirection` and
  `GMarker::mPosition`/`mDirection` are `ALIGNVEC`. `GTrigger::GTrigger` (2,544 B) is at 99.58 %: a
  register pin and a barrier were removed after measuring the object identical without them.
  `GTrigger::ShowIcon` calls the inline `GIcon::Show()` and the original emits `bl SetFlag__5GIconUi`:
  GCC 2.95 at `-O1` does not inline an inline called from inside another inline.
- **`Gameplay/GRuntimeInstance.h`, `GRuntimeInstance.cpp`** — The original's `USE_FASTALLOC` has seven
  operators including placement `new(size, void *)`: placement new on such a class has no null check,
  the global one does. Applied to this header only.
- **`Gameplay/GVault.cpp`, `GObjectBlock.cpp` (+ `GVault.h`, `GObjectBlock.h`)** — Written from empty
  (28 and 24 functions). The two headers belong to this tree (the original's file table has none); they
  share the declarations between `GManager.cpp`, `GRaceDatabase.cpp` and `GRaceStatus.cpp`.

#### Everything else

- `GCharacter.cpp` from zero; `GActivity::GetIsRunning` returns `mRunning` without normalising;
  `GMilestone::GetIsAvailable` is a range test (states 2..3), not two equalities; `SavedTimerInfo` is
  0x20 bytes (the 0x24 in the header comment is wrong); `GSpeedTrap::IsFlagSet`/`IsFlagClear` are only
  declared in the class and defined `inline` late in `GSpeedTrap.cpp`, so the header's `GetIs*` accessors
  call them out of line, as the original does; `GSpeedTrap::Reset` multiplies by `0.27778f`, not by the
  `KPH2MPS` macro (a division by 3.6f, different bits); `GIcon::CreateGeometry` does not call
  `SetCastsShadow` in the A124; `GInfractionManager::ReportResistingArrest` exists again (upstream's form);
  `GHandler`, `GMarker`, `GState`, `GTimer` written.
- Open on PS2: the A124 has 15 icon types instead of 17 (see Open fronts).
- Left on GameCube: the five functions named above (`GManager` three, `GRaceDatabase` one, `GTrigger` one).

### Camera (`src/Speed/Indep/Src/Camera/`)

At the fork point 22 of the 28 `.cpp` files of `zCamera` were empty (the seven `CDAction*`, all nine
movers and six of the eight `ICE*`), and upstream still compiles six files (3.27 %). All 28 are written
here (447 of 453, 92.86 %) and follow the line map's order (TrackCar and TrackCop come before Rearview,
Showcase and the ICE files). `SourceLists/zCamera.cpp` starts with
`#pragma implementation "ITrafficCenter.h"`: the original defines that interface's vtable and
destructor, as globals, in this unit. On PS2 every `CDAction*` member was 16 bytes early until
`UTL::COM::Object` got `PS2ALIGN16` (in `UCOM.h`): a probe gave `CameraAI::Action` 0x18 against 0x30 in
the dump (SLES +71 / SLUS +80 functions).

#### Notable files

- **`Camera/Movers/Cubic.cpp`, `Camera/CameraMover.hpp`** — What: `CubicCameraMover` from zero, declared
  in `CameraMover.hpp` (the line map puts its inline virtuals at `CameraMover.hpp:969-984`); key method
  `IsHoodCamera`; `CubicPovData` (0xB0, DWARF line 330296) and `aCubicPovTables[7]`. PS2: the A124 has no
  `fIgnoreSetSnapNextTimer` (stabs: `tLastGearChange`, then `vSavedEye`), which was also the unexplained
  +16 on `vCameraImpcat`. In the prototype `SetSnapNext` is in-class (no symbol;
  `CDActionDrive::MessageJumpCut` stores `bSnapNext` at 0xB8 directly), the constructor has no `else`
  branch, and `Update` neither counts the timer down nor uses it around `CameraAccelCurve`; all of it is
  guarded. Left: the unit's static initializer at 99.84 % (3,604 B): six rows around
  `HydraulicsLookAngle`; `-fno-schedule-insns2` and `lreg.py` show three values rotating among three
  local-alloc registers, and no reordering or source form tried so far moves them (the comment keeps the
  measurements).
- **`Camera/CameraAI.cpp`, `CameraAI.hpp`** — What: `CameraAI::Director` is flat with its vptr at 0x2C4
  (DWARF line 97844); the `DirectorData` base it used to have forced 8-byte alignment (its `StringKey`
  holds a `uint64_t`) and moved every member by 8. `Action::GetName()` returns
  `const Attrib::StringKey &` (read at the call site, no sret buffer). `IDebugWatchCar` is declared at the
  end of `CameraAI.hpp` (its inlines are at `CameraAI.hpp:147` in the line map) with a pure
  `GetSimable()` (vtable 0x20, not 0x18). An
  explicit instantiation of `_STL::find<IVehicle *const *>` (GameCube and PS2) gives the copy the original
  unit contains. Left: the copy constructor of `_Storage<CameraAI::Director *, 2>` (98.08 %), which the
  original calls out of line from `CameraAI::Shutdown`.
- **`Camera/Actions/CDAction*.cpp`** — What: the seven camera actions from empty files.
  `ITrafficCenter::GetTrafficBasis` and `Sim::Collision::IListener::OnCollision` are pure (the ELF has
  `__pure_virtual` in both vtables and no `GetTrafficBasis__14ITrafficCenter` symbol); before that no
  vtable was emitted for the five `ITrafficCenter` users. `~CDActionTrackCar` is defined for the same
  reason (about 1.4 kB per class). `CDActionDebugWatchCar` does not inherit `IAttachable` (the symbol
  table has no `_vt.<class>.11IAttachable`). The `ConversionUtil` helpers (`Copy4`, `Scale3`, `Make3`,
  `Make4`, `RightToLeft*`) are new.
- **`Camera/ICE/`** — What: `ICEMover` (layout at DWARF line 324389, bodies from 340475), `ICEAnchor`,
  `ICEData`, `ICEPoint`, `ICEManager`, `ICEReplay`, `ICEOverlays`, `ICEAnimScene`, `ICEMath.hpp`. From the
  evidence: `ICEData` has `unsigned char fSimSpeed[2]` at 0x80 and is 0x84 bytes (the code multiplies by
  0x84); `ICESingleKey` (0x3C, in five units of the DWARF) is one side of an `ICEData` key;
  `ICEAnchor::mNumWheels`/`mIsNosEngaged` are `int`; `ICE::Cubic1D`/`Cubic3D` are the same code as
  `tCubic1D`/`tCubic3D` in `Misc/Spline.cpp`; `ICE::ATan` has a default second argument.
  `ICEMover::Update` (3,868 B) matched once the source followed the DWARF (a single local
  `int frame; // r8`, and a `SignedMod` inline that returns instead of modifying its argument), after a
  blind permuter had spent 1,606 compilations on it without progress. PS2:
  `ICEMover::mHybridToWorld`/`vSmoothCarFwd` are `ALIGNVEC`, `ICEAnchor::mGeomRot` is `PS2ALIGN16`
  (class 0xA0). Left: `ICEManager::LoadCameraShakes` 96.67 % after removing an invented `guard` variable
  and its barrier (the DWARF has no such local; the line map puts the target's two zeros on the `for`).
- **`Camera/CameraMover.cpp`** — What: `CameraAnchor` lives here (upstream's header computed
  `GetVelocityMagnitude` with a square root; the original returns `mVelMag`, which alone took
  `TerrainVelocityNoise` from 80 % to 98 %); `tAverage<T>::tAverage(int)` is defined, which emits the
  `tAverage<bVector3>` vtable; `AverageBase::Recalculate` is declared in `Misc/Table.hpp` and defined
  here, empty: it is the key method, and the target `zMisc` object imports `_vt.11AverageBase` as
  undefined. PS2 and Xbox 360: `mWPos` is at 0x30 (GameCube 0x28) once `WCollisionTri`'s points are
  `ALIGNVEC`.
- **`Camera/Movers/`** — `SelectCar`, `TrackCar`, `TrackCop`, `DebugWorld`, `Showcase`, `Rearview`,
  `CopView` and `Still` written. `SelectCar` uses pi as `3.1415926f` (0x40490FDA), not bMath's `M_PI`
  (0x40490FDB); objdiff scored the wrong constant 100 %. `TrackCar.hpp`, `Showcase.hpp`, `Rearview.hpp`
  and `DebugWorld.hpp` were added by this tree; the original's file table has none of them (nor
  `TrackCop.hpp`), and it has a `movers/cubic.hpp` that neither tree has.

#### Everything else

- `Camera::SetLetterBox` stores `LB_height` (an empty accessor upstream); `CameraInfo.hpp` gains the
  `POVTypes` enum. Data symbols missing from the whole tree (the `Pov*NoiseScale` tables in `Cubic.cpp`,
  `Tweak_EnableICEAuthoring`/`Tweak_ForceICEReplay` in `CameraMover.cpp`) were found with
  `symtabdiff.py` and their values read from the target object.
- Left on GameCube, besides the three named above: `TrackCarCameraMover::Update` 99.63 %,
  `TrackCopCameraMover::Update` 99.16 % (two barriers and an invented `dst` removed),
  `DebugWorldCameraMover::Update` 99.96 %.

### Animation (`src/Speed/Indep/Src/Animation/`)

`zAnim` was reconstructed in both trees (97.84 % here, 94.58 % upstream). Upstream's later "zAnim fixes"
were merged function by function: eight clean merges and five whole files were taken because they tie,
and three files were kept because upstream's version lost a function here
(`AnimEntity_BasicCharacter.cpp` `RenderEffects` 100 → 99.97 %, `AnimPlayer.cpp` `CAnimPlayer::Load`
100 → 96.69 %, `AnimScene.cpp` `LoaderAnimSceneData` 100 → 95.79 %).

#### Notable files

- **`Animation/AnimPlayer.cpp`, `AnimPlayer.hpp`** — `CAnimPlayer::Load` (496 B) matches with the
  `goto done` moved out of the fallback `if` (the target re-enters the `gAnimLoader_MemPointer` check);
  `CAnimPlayer::Unload` takes upstream's form (`TheCarLoader.FreeUserMemory`, which also emits
  `CarLoader::FreeUserMemory`).
- **`Animation/AnimEntity_BasicCharacter.cpp`** — `RenderEffects` at 100 % (upstream 99.97 %); the two
  versions differ, among other things, in which `shadow_poly.UVs` elements receive the constants.
- **`Animation/AnimScene.cpp`, `AnimScene.hpp`, `AnimProperty.hpp`** — `LoaderAnimSceneData` at 100 %;
  `CAnimProperty` (0x14, `bTNode` base) sits in `AnimProperty.hpp`, a header of this tree (not in the
  original's file table), which is why upstream's `AnimScene.hpp` cannot be taken whole.

#### Everything else

- Retail PS2: seven functions match in upstream's SLUS build and not here (`CAnimEngineManager::Init`,
  `InitAnimCtrls`, `InitAnimBankSlotPool`, `LoaderWorldAnimDirectoryData` and three
  `EndianSwapEntityData`). They sit under `EA_BUILD_A124` guards inherited from upstream (slot pools
  without `GetVirtualMemoryAllocParams()`, `EAGL4Anim::Init(0xC800, true)`, no endian swaps), whose
  prototype branch the retail build also follows; upstream defines `EA_BUILD_A124` for SLUS, this tree
  no longer does.
- `AnimCandidates.hpp`: `GetSpecialCarList` has its loop instead of an empty `{}` placeholder.
- Left on GameCube: `CWorldAnimEntity::Init` (97.98 %, 912 B; 100 % only with an asm scaffold, removed).

---

## World, renderer, animation, misc and online

This part covers `src/Speed/Indep/Src/{World,Ecstasy,Render,EAGL4Anim,Misc,Online}`, which the SourceLists
`zWorld2`, `zWorld`, `zTrack`, `zEcstasy`, `zRender`, `zEagl4Anim`, `zMisc`, `zMiscSmall` and `zMission` compile.
`zOnline` compiles nothing.

**Read `git diff upstream/main HEAD` in these directories with care.** Upstream kept working on `World/` and
`Misc/` after the merge base `59f184a0`: it ran a `this->` pass over zWorld/zWorld2/zTrack, added the zWorld2
match improvements, and merged PR #141, which rewrote most of zMisc (`Point.cpp`, `QueuedFile.cpp`, `bFile.cpp`,
`LZCompress.cpp`, `Table.hpp`, …). A large share of that diff is therefore upstream's newer code that this fork
has not taken. Everything below covers only lines the fork added since the merge base
(`git diff 59f184a0 HEAD`). Upstream made no change after the merge base in `EAGL4Anim/` or `Render/`, and
only small ones in `Ecstasy/` and `Online/` (`Ecstasy.hpp`, `EcstasyData.hpp`, `Texture.hpp`, `OnlineCfg.hpp`).

Short names for the sources used below:
- *DWARF*: the GameCube DWARF-1 (`symbols/mw_dwarfdump.nothpp`, `scripts/dwarf1.py`).
- *line map*: `symbols/debug_lines.txt`, which gives the source line of each original instruction.
- *A124 stabs*: the `.mdebug` of the PS2 prototype (`scripts/mdebug.py`, `scripts/sondastabs.py`).
- *symbols.txt*: `config/GOWE69/symbols.txt`.

Matched functions per unit, measured with objdiff using upstream's method (upstream at `fd0c355`, fork on
2026-09-18):

| unit | GameCube upstream | GameCube fork | PS2 A124 upstream | PS2 A124 fork |
|---|---|---|---|---|
| `zWorld2` (`World/Common`, collision headers) | 349/357 | 353/357 | 329/557 | 367/557 |
| `zWorld` (cars, render glue) | 565/582 | 576/582 | 320/897 | 327/897 |
| `zTrack` (streaming, scenery, effects) | 244/259 | 255/259 | 180/354 | 223/354 |
| `zEcstasy` | 340/539 | 531/539 | 244/960 | 273/960 |
| `zRender` | not built | 6/6 | not built | 3/6 |
| `zEagl4Anim` | 165/318 | 314/318 | 145/392 | 236/392 |
| `zMisc` / `zMiscSmall` / `zMission` | 450/450, 7/8, not built | 450/450, 8/8, 2/2 | 332/734, 6/8, — | 450/734, 6/8, 2/2 |

### World: collision, grid and road network (`src/Speed/Indep/Src/World/Common/`, `World/W*.h`)

Unit `zWorld2`. Upstream improved this unit after the merge base too. The fork took upstream's file whole wherever
it measured no worse in this tree: `WTrigger.cpp/.h` (which brought `CheckCollideSRB` to 100 %), `WCollider.cpp`,
`WWorldPos.cpp` and `WWorldMath.h`. It kept its own version where upstream's lost functions: `WRoadNetwork.cpp`,
`WCollisionMgr.cpp`, `WPathFinder.cpp`, `DamageZones.cpp`. The fork's work here is a few large functions and
several layout and emission fixes that reach beyond this unit.

#### Notable files
- **`src/Speed/Indep/Src/World/WCollisionAssets.h`** — What: include `CARP.h` + `Common/WGridNode.h` instead of
  `WGridManagedDynamicElem.h`, which was included twice. Why: the in-class `Init()`/`Shutdown()` call
  `fgManagedDynamicElemList.clear()`. With that list type visible, every unit including the header instantiated
  the 120-B `_List_base<...>::clear`: five of our objects defined it against one original object, and
  zGameplay/zMisc/zPhysics took zSim's symbol. Evidence: the original zWorld2 object has `clear` undefined. How: a
  probe unit showed the trigger is a *use* written in the header. Effect: 1,447,438 fewer differing DOL bytes in
  three units.
- **`src/Speed/Indep/Src/World/Common/WRoadNetwork.cpp`** — What: `HolePunchAvoidables` (2,980 B) rewritten.
  - Upstream calls `bCross(&cookie.Forward, &cookie_to_avoidable)`. The reversed operands flip the sign of
    `avoidable_offset`, which is a behaviour bug, not only a mismatch. Evidence: the operand order of the
    target's `fmuls`/`fmsubs` pair; fixing it took the diff from 44 rows to 9.
  - `extra_width = offset_change * (1.0f - 0.8f)`: the pool holds `0x3E4CCCCC`, which only that folded
    subtraction produces. The line map places the statement, and the DWARF shows `approach_time` reassigned
    twice.
  - Result: 97.18 % with no pins (upstream's body: 94.92 %); `CookieTrailCurvature` 99.66 % (upstream: 95.94 %).
  - The original reserves a guard and a destructor thunk (`__tcf_3`) in `GetSegmentPointIntersect` for a
    function-local static inside code the optimiser removed; its name is unknown, so it is not written, and
    the next static's thunk is `__tcf_3` here instead of `__tcf_4` (an invented `static USpline` was removed).
- **`src/Speed/Indep/Src/World/Common/WGrid.cpp`** — What: `FindNodes` (3,456 B, 93.01 → 100 %), written as an
  early return plus five walking loops, each a guard and a hand-written `do { } while`. How: the mnemonic-count
  delta (`scripts/mnemo.py`: `cmpw+4, b+4, addi+2, subi+2, bge+2, ble+2`) predicted the five loops and their
  direction before compiling. Each guard must repeat the loop condition word for word, or `thread_jumps` cannot
  redirect it.
- **`src/Speed/Indep/Src/World/WorldConn.h`** — What: `ALIGNVEC` on the six `Pkt_Body_*`/`Pkt_Effect_*` classes
  and `PS2ALIGN16` on the three `Pkt_Body_*::mMatrix` members. Evidence: the A124 stabs and X360 `Size()` (see
  `Render/`). Upstream aligned `UMath::Matrix4` as a type on PS2, which the A124 refutes: `SmackableParams`
  contains a `Matrix4` and is 0x5C, which is not a multiple of 16. Also, `DECLARE_WORLDPACKET` now names the
  real dispatchers (`World_UpdateBody`, `World_OneShotEffect`, `WorldEffectConn`) where upstream uses
  `WorldBodyConn` for all six. Three of those came from auditing relocation targets in functions objdiff
  already scored at 100 %. Effect: SLES +17 / SLUS +8 for the packet
  change (measured together with two A124 member guards elsewhere), then +13 / +15 for the matrices.
- **`src/Speed/Indep/Src/World/WCollisionTri.h`** — What: `ALIGNVEC` on `fPt0/1/2`, which makes
  `WCollisionTri` and `WWorldPos` 16-aligned on PS2/X360. Evidence: the A124 stabs put every `WWorldPos` member
  on a multiple of 16 (`CameraMover::mWPos` 0x30; `CarRenderInfo::mWorldPos` 0x50, with 8 empty bytes before it),
  and X360 builds CameraMover's at 0x30 where GameCube uses 0x28. How: `sondastabs.py` probes every class layout
  from its SourceList; its first pass matched 2,414 classes and not 107. Effect of the batch: SLES +62,
  SLUS +71, X360 +8.
- **`src/Speed/Indep/Src/World/WCollision.h`** — `WCollisionArticle::GetSurface` keeps `fStripsSize +
  sizeof(WCollisionArticle)` in a named intermediate, which stops GCC from hoisting `this+0x10`. This closed
  `GetTriList`, `GetBarrierNormal` and `GetBarrierList` (6,480 B in four functions).
- **`src/Speed/Indep/Src/World/Common/WCollisionMgr.cpp`**, **`WGridManagedDynamicElem.cpp`** — What: file-local
  `static inline` `OrthoInverseInline`/`OrthoInverseWrap` replace an `#ifdef EA_BUILD_A124` split at three call
  sites; the DWARF lists an inline `OrthoInverse(Matrix4 &)` expanded there. `FindFaceInCInst` keeps
  `float dist = faceY;` (upstream's form measures 99.18 %). `GetClosestIntersectingBarrier` stores `&*bIter`
  in a pointer so that MSVC's iterator compiles; upstream removes the body on Xenon instead.
- **`src/Speed/Indep/Src/World/DamageZones.cpp`** — keeps `#include ".../AttribHash.h"`, which upstream dropped.
  Without it `Attrib::Gen::world::ClassKey` is not emitted, yet the original has it at the end of zWorld2.
- **`src/Speed/Indep/Src/World/Common/WWorldMath.cpp`** — `IntersectCircle` computes
  `pow2(x2 - x1) + pow2(y2 - y1)` before the in-place `-=` updates; none of the 16 orderings of those updates
  matched. Now 100 %.

#### Everything else
- `WRoadNetwork.h`: bodies for `SetRaceFilterValid(bool)` and `GetSegmentStamp()`. In `WRoadNetwork.cpp`,
  `GetSegmentTrafficLaneInd` takes upstream's form (SLES 89.76 → 100 %), while `InitLaneOffset` keeps the
  merge-base `bMin` order (upstream's measures 99.21 %).
- `WRoadElem.h`: `WRoadLane::GetBits/GetBitsSigned` are back in the header, as upstream has them. MSVC cannot
  expand an `inline` defined in another `.cpp`; with them in the header, X360 `WRoadProfile::GetNumTrafficLanes`
  and `GetNthTrafficLane*` go from 23–45 % to 100 %.
- `WWorldPos.h`: a default `WWorldPos()` for `PVehicle::SetVehicleOnGround`. Known deviation: the DWARF has only
  `WWorldPos(float)`.
- `WPathFinder.cpp`: keeps `BIND_ACTIVITY_FACTORY(PathFinder)` (upstream's file drops the static initializer to
  93.99 %) and `ProfileNode profile_node;` (no invented `"TODO"`). Open point: the SourceList includes this file
  after `WRoadNetwork.cpp`, which fixes the static-init order, but the original `.text` has
  `AStarSearch`/`PathFinder` first. Upstream keeps the `.text` order and moves the binding instead.
- Only upstream's newer changes: `WCollisionAssets.cpp`, `WCollisionPack.cpp`, `WWorld.cpp`, `WGrid.h`,
  `WGridNode.*`, `WCollider.h`, `WCollisionMgr.h`, `WCollisionPack.h`, `WCollisionSTL.h`, `WSurfaceTypes.h`,
  `WWorld.h`, `DamageZones.h`.

### World: cars, streaming, scenery and effects (`src/Speed/Indep/Src/World/`)

Units `zWorld` and `zTrack`. The fork's reconciliation with upstream merged upstream's `VehiclePartDamage.cpp` and
`ParameterMaps.cpp`, and parts of `Skids.cpp`, `World.cpp` and `TrackPath.cpp`. Most of the PS2-prototype guards
come from upstream's side branch `ps2_fixes`. The fork's own work is the largest functions, strings and constants
recovered from the original `.rodata`/`.data`, and statement order taken from the line map.

#### Notable files
- **`src/Speed/Indep/Src/World/CarRender.cpp`**
  - `CarRenderInfo::Render` (11,132 B, the largest function in the game) went from 98.88 to 100 %. A free
    `static inline bSlerp(qdest, q1, q2, t)`, which the DWARF names, replaces the member `Slerp`, and the part
    dispatch is a `switch` with `default:` first. It has been at 99.91 % since the splitter labels
    `lbl_8040AD3C/4C` became plain `1.0f`/`0.0f` and a GCC case range became four labels for MSVC.
  - `RenderFlaresOnCar` (2,908 B) reaches 100 % once `(A&&B)||(C&&D)` is split into `if`/`else if`. The cc1plus
    `.greg` dump showed that the split gives the `@ha` pseudo an eighth reference, and that reference settles
    the allocation.
  - The line map puts all of `UpdateWheelYRenderOffset` on one line, so its body is a single statement.
- **`src/Speed/Indep/Src/World/CarLoader.cpp`** — What: the invented `"TODO"` name of
  `ProfileNode profile_node("TODO", 0)` is dropped at all 13 sites (`"TODO"` never occurs in `main.dol`). The
  real `Begin()` names `"Remove Duplicates"`, `"Defragment Textures"` and `"Defragment Solids"` were each
  located by their neighbours in the original zWorld `.rodata`. `SetMemoryPoolSize` gets the sponge loop the
  DWARF still describes (its own block with `n`, and an inline
  `bMalloc(…, "SpongeAllocation", 0, (CarLoaderMemoryPoolNumber & 0xF) | 0x2000)`), which emits no code because
  `CarLoaderMemorySpongeNumAllocs == 0`. `DefragmentPool` is at 92.47 % without three
  helper locals the DWARF lacks (they gave 99.27 %).
- **`src/Speed/Indep/Src/World/TrackStreamer.cpp`**, **`.hpp`**
  - `static inline TrackMPH2MPS(mph) { return mph / 2.237f; }`, and `SceneryMPH2MPS` in `Scenery.cpp`. The ELF
    loads 1/2.237f (`0x3EE4E0C2`), not the `0.44703001f` of `ConversionUtil.hpp`, and the DWARF names a second,
    non-const `MPH2MPS(float)`. Changing the shared header instead broke 23 functions in other units.
  - `bBitTable::ClearTable` is inline after the constructor; the target keeps an out-of-line copy.
  - `HandleLoading` is at 100 % with no asm, once its `||` of exclusive tests is written as if/else-if.
  - `GetLoadingPriority` puts its three products on the lines the line map gives.
  - The literal audit found the `SwitchZones` threshold `30.0f` (it was `0.1f`) and `ForceHoleFillerMethod = -1`.
  - The A124 guards (`PermFile*`, `CurrentZoneFarLoad`, `CompressedSize`) come from `ps2_fixes`, applied by hand.
    With them zTrack compiles for the A124.
- **`src/Speed/Indep/Src/World/OnlineManager.hpp`**, **`.cpp`** — What: 73 of the 88 method declarations from
  upstream's branch `bit_of_ecstasyy`, filtered by the compiler; only declarations, because its data members
  would move the layout. The fork adds the in-class `OnlineManager() { InitQuantizers(); }` that the DWARF shows.
  Together with a `Sim::SubSystem` constructor fix it closed zWorld's static initializer (11,168 B; zWorld 70.17
  → 77.11 %). `inline void OnlineManager::InitQuantizers() {}` sits in the `.cpp`: defined out of line, it
  would become the first `.text` symbol and shift 497 functions by 4 B.
- **`src/Speed/Indep/Src/World/Scenery.cpp`**, **`.hpp`** — What: globals that upstream only declares are defined
  with their original values and in `.data` order: `ForceAllSceneryDetailLevels` at 0x80436000,
  `EnvMapShadowExtraHeight = 2.0f`, `CurrentZoneNumber = -1`, and the Seeulator flags at 0x80436020–2C.
  `GrandSceneryCullInfo`'s constructor is placed so that GCC emits the out-of-line `Init` the target has. The
  rest of the diff is `ps2_fixes` guards.
- **`src/Speed/Indep/Src/World/CarSkin.cpp`** — `CompositeSkin(SkinCompositeParams *)` is at 100 % (+2,128 B).
  The line map puts the colour store after the division (line 771 against 768), and the DWARF gives `int x` in
  `r0`.
- **`src/Speed/Indep/Src/World/TrackPath.cpp`** — `FindZone` without its register pin: 99.85 %. Comparing our
  objects' `.line` notes with the line map found two statement pairs in reverse order. A variant that reached
  100 % by adding `return`s was rejected: the original has one return and no epilogue line note.
- **`src/Speed/Indep/Src/World/VehiclePartDamage.h`**, **`WorldModel.hpp`** (PS2) — `ALIGNVEC` on
  `linearAcc_ch`, `linearVel_ch` and `angularVel_ch`. The A124 type dump has them at 0x90/0xA0/0xB0, a 16-byte
  stride against GameCube's 12; they are part of a ten-member change (SLES +46 / SLUS +50). `SetCastsShadow` gets
  the same `#ifndef EA_BUILD_A124` guard as the member it sets.

#### Everything else
- `ScreenEffects.cpp/.hpp`: mostly the A124 palette and tunnel code from `ps2_fixes`. The fork's own changes are
  the glare divisor `len / 17.0f` (it was `1.7f`) and the definitions `GlareFalloff = 0.02f`,
  `GlareFallon = 0.04f`, `TUNHEIGHT = 10.0f`, `FACflush` and `debugflash`.
- `EventManager.cpp`: `emAddHandler` returns early on `function == nullptr || stream_mask == 0`; the merge-base
  logic was wrong. `CarRenderConn.cpp`: `UpdateParts` is at 100 % without the `#pragma scheduling
  off/reset` that wrapped it (SN's GCC ignores that pragma).
- `Sun.cpp`: its globals are initialized to 0 so that they land in `.data`, and `WorldLightDirectionVector`
  moves to `Ecstasy/eLight.cpp`. `RaceParameters.hpp`: bodies for `IsBurnout`, `IsShortTrackRace`,
  `IsDriftPhysics`, `IsBurnoutPhysics`. `NeuQuant.cpp`: `learn()` indexes with `i`, not `j`.
  `VisualTreatment.cpp`: statement shapes. `World.cpp`: `rotMat` starts from `kIdentity`.
- The `ProfileNode "TODO"` removal also covers `CarInfo.cpp`, `Clans.cpp`, `Skids.cpp`, `WorldModel.cpp`,
  `SkyRender.cpp` and `CarRenderConn.cpp`.
- Only `ps2_fixes` guards: `TrackInfo.cpp`, `TrackPositionMarker.cpp`, `TrackPath.hpp`, `VisibleSection.hpp`,
  `Clans.cpp`, `Skids.cpp`. Only upstream's newer changes: `CarLoader.hpp`, `Clans.hpp`, `Skids.hpp`, `Sun.hpp`,
  `FacePixelate.hpp`, `HeliSheet.cpp`, `SmackableRender.cpp`, `SpaceNode.*`, `Track.*`,
  `VehicleFragmentConn.cpp`, `VehicleRenderConn.*`, `WeatherMan.cpp`, `rain.cpp`.

### Ecstasy renderer core (`src/Speed/Indep/Src/Ecstasy/`)

Unit `zEcstasy`. On GameCube, 340 of its 539 functions match in upstream and 531 in the fork. Part of the base
comes from upstream side branches: `EmitterSystem.cpp/.h`, `Texture.cpp`, `eModel.cpp` and `Ecstasy.cpp` from
`bit_of_ecstasyy`, and parts of `Ecstasy.hpp`/`EcstasyData.hpp` from `pr-117`. The fork wrote about 2,000 lines on
top of that base. Two data rules run through the unit:
- Zero-valued globals get an explicit `= 0`. GCC 2.95 only puts a global in `.data` when it has an initializer,
  and symbols.txt shows the original has them there.
- Objects with constructors are declared in the order in which the original static initializer builds them.

#### Notable files
- **`src/Speed/Indep/Src/Ecstasy/eLight.cpp`**, **`eLight.hpp`**
  - Functions: `LoaderLights` (1,400 B), `elSetupLights` (1,252 B), `UpdateLightFlareParameters` (reads the
    `light_flares_cg` collections by key; the names are checked in `symbols/vlt.txt`) and `eRenderLightFlare`
    (3,356 B, absent upstream). `eRenderLightFlare` was transcribed with `plan.py` and matched at its first
    compile.
  - Data, with the values from the ELF: `SingleLightFlareParameters[22]` (0x8041A6B0, 22 × 0x2C), the flare
    texture hashes and twelve `eShaperLightRig` presets. Declared in the original order, they close the static
    initializer (4,544 B), which had been at 24.45 %.
  - Header, from the DWARF: `LightFlareParameters` (0x2C), `eDynamicLight` (0x68) and `eDynamicLightPack` (0x34);
    EE-GCC needs the complete type to destroy `bTList<eDynamicLightPack>`, so this is what lets zEcstasy compile
    on PS2. The line map attributes the static-init code to `eLight.hpp:856–863`, which places the
    `eShaperLightRig` constructors. `LIGHT_INVALID = 0xFFFFFFFF`: the asm compares it with `cmplwi`.
- **`src/Speed/Indep/Src/Ecstasy/eSolid.cpp`** — What: `InternalLoaderSolidChunks` (1,576 B; a stub upstream).
  It walks the `SOLID_INFO` … `MESH_INFO_CONTAINER` chunks, byte-swaps them and flags duplicate solids. Also
  normal smoothing (`eSolid::SmoothNormals`, and `eSmoothNormals` as a k-way merge) and texture-fixup bookkeeping
  (`RebuildSolidTextureFixupInfo`, `eSolidNotifyTextureLoading/Moving`). Evidence: the line map (`eSolid.cpp:616`,
  `:928`) and symbols.txt, which places the unit's globals at 0x8041A610–20.
- **`src/Speed/Indep/Src/Ecstasy/EmitterSystem.cpp`**, **`.h`** — `bit_of_ecstasyy`'s version, plus:
  - Data: the `.data` block in the original order (`EmitterRandomSeed = 0x23985727`, slot pools) and the
    function statics `random_seed = 0x69247365` and `aseed = 0xFEED0123`.
  - Functions: `HandleFXTriggers` (452 B). `SpawnParticles` (2,780 B) is closed by an `(unsigned int)` cast
    before the int→float conversion, and by a local copy read only in the guard, which stops `thread_jumps` from
    merging the re-test.
  - Constructors: stores in descending offset order in `smVector3`/`EmitterParticle` mean the source used a
    chained assignment `a = b = c = 0` (464 B).
  - `Render` has been at 82 % since a local absent from the DWARF was removed; its notes block is out of date.
- **`src/Speed/Indep/Src/Ecstasy/Texture.cpp`**, **`Texture.hpp`**
  - New: `InternalLoadTexturePackHeaderChunks` (768 B).
  - Two upstream bugs fixed. `UpdateTextureAnimations` never advanced the animation; it now uses
    `base_entry[(i + current_frame) % num_frames]`. `TexturePack::GetTextureDataSize` called itself.
  - Names from the originals: the callback name is spelled as in the GameCube/A124/SLUS symbol tables, and
    `new ("TextureInfo", 0)` takes its name from `.rodata`.
- **`src/Speed/Indep/Src/Ecstasy/eStreamingPack.cpp`** — fixes to upstream's logic:
  - `LoadStreamingEntry` keeps the minimum offset instead of overwriting `ChunkByteOffset`.
  - `CreateStreamingPack` always reads `min(size, 0x8000)` header bytes.
  - The `TestLoadStreamingEntry` condition `(Flags & 0x10) == 0 && RefCount == 0` was decoded from GCC's merged
    load (`lwz 0x10` + `and. 0x10FFFF`).
  - A zero-trip `for (n = 0; n < 0; n++)` reproduces a DWARF block with an empty range.

#### Everything else
- `Ecstasy.cpp` defines `ePolySlotPool`, `WaitUntilRenderingDoneDisabled` and `CurrentBuffer*`, which upstream
  uses but never defines; its buffer names use EA's spelling `"Ecstacy::eFrameMallocBuffer0/1"`.
- `Ecstasy.hpp`: `eSolid::EndianSwap`, and `ePoly` flag accessors guarded because the A124 lacks `flags`.
  `ePoly` follows the PS2 stabs: `float UVs[4][2]` on every platform and `short UVsMask[4][2]` on PS2; on PS2
  `GetFlags`/`GetFlailer` read `Vertices[0].pad`/`Vertices[1].pad`. The GameCube bytes do not change (the DWARF
  dump prints the dimensions reversed), and the users of `UVs` were re-indexed in the same change.
- PS2 `EcstasyE.cpp`: the counters and flags the VBlank and VIF interrupts touch are `volatile`, each with a
  re-read in the binary as proof; `VRAMView` and `eWaitUntilRenderingDone` keep EA's `nop` lines (see
  "How EA's inline asm is proven").
- `EcstasyData.hpp`: `EndianSwap` bodies and `eDamageVertex`.
- `eModel.cpp`: `NotifySolidUnloader`, at line map `:702`.
- `eSprites.hpp`: `eSpriteManager` (0x1C).
- `elSetupLights` keeps a zero-trip `{ eLightQuery light_query; }` block. Its DWARF range is empty, but the
  original frame still reserves its 0xD0 bytes.
- The `zEcstasy` SourceList gains `eLightE`, `eEnvMapE`, `eMatrixE`, `ePShader`, `eVShader`, `eVisualTreatment`
  and `TexEnumGen_ParticleTextures.cpp`, found by auditing the line map against `splits.txt`.
- `elRotateLightContext` is at 73.14 %: without a dead `static inline` that copied an `eDynamicLightContext`
  (removed as a primer), GCC calls the synthesized `operator=` out of line instead of inlining it. The original
  inlines it, so something before it used that copy, but the DWARF shows nothing that did.

### Render connections (`src/Speed/Indep/Src/Render/`)

Upstream's `zRender` SourceList is empty, so upstream builds no object for it. This fork compiles the unit's three
files, and all six GameCube functions match. It also fixes the render packets' layout for PS2 and Xbox 360.

#### Notable files
- **`src/Speed/Indep/Src/Render/RenderConn.h`** — What: `ALIGNVEC` goes on each packet class (`Pkt_Car_*`,
  `Pkt_Heli_*`, `Pkt_VehicleFragment_*`, `Pkt_Smackable_*`), not on `Sim::Packet`. Evidence: in the A124 stabs,
  `Sim::Packet` is 4 bytes, yet every packet's size is a multiple of 16 even when its fields end earlier:
  `Pkt_Effect_Open`'s fields end at 0x18 and it measures 0x20. X360's `Pkt_Effect_Open::Size()` returns 0x20;
  on GameCube the class is 0x18. How: `sondastabs.py`. The same fix covers all 18 `Pkt_*` classes in
  `RenderConn.h`, `SoundConn.h` and `WorldConn.h`, and all 18 then match the probe.
- **`src/Speed/Indep/Src/Render/Common/RRandom.h`**, **`.cpp`** — `RRandom::Fixed/IntRange/FloatRange`. Its two
  statics take the values the DOL's `.data` holds: `fastRandom = 0xF874AF01` at 0x80435768, `randSeed = 1` at
  0x8043576C.
- **`src/Speed/Indep/Src/Render/Common/RenderConn.cpp`**, **`ColourConversion.cpp/.h`** —
  `RenderConn::InitServices`, `RestoreServices`, `UpdateLoading` and `UpdateServices`, which forward to the
  smackable, car, emitter and animation services; and `ColourConvertXBoxToPS2`, which swaps the red and blue
  bytes.

### EAGL4Anim animation runtime (`src/Speed/Indep/Src/EAGL4Anim/`)

In upstream most `.cpp` files here are empty and most accessors are `{}` placeholders. Upstream has not touched
this directory since the merge base, so everything below is fork work. This fork writes unit `zEagl4Anim` almost
completely: about 5,100 lines in 24 files that were empty or nearly empty, and about 600 in the ELF loader. On
GameCube that takes the unit from 165 to 314 of 318 functions. The method: `fuse.py`/`plan.py` combined the DWARF
(member offsets, locals with their registers, the inline-expansion tree) with the original asm and the line map,
and objdiff checked each function. No compiler-steering asm is left, only comments recording measured
experiments. The vtable-order rule was found in this unit (see the last entry).

#### Notable files
- **`src/Speed/Indep/Src/EAGL4Anim/FnDeltaQ.cpp`**, **`FnDeltaQFast.cpp`**, **`FnDeltaSingleQ.cpp`** (and
  `DeltaQ.h`, `DeltaQFast.h`, `DeltaSingleQ.h`)
  - What: the three compressed-quaternion channels, about 30 KB: `EvalSQT`/`EvalSQTMasked` (~5 KB each),
    `UpdateNextQs`, `Add/SubDelta(Mask)` and the header dequantisation.
  - How: `FnDeltaF1.cpp`/`FnDeltaF3.cpp`, from the same family and already at 100 %, served as a literal
    template.
  - `DeltaQDelta::DeQuantize7Bit/8Bit` exist because `-ffast-math` reassociates `a*(b*K)` unless the product
    sits inside an inline call.
  - `DeltaSingleQ.h` loses `GetConstBoneIdx`/`GetConstPhysical`: the DWARF has no such methods.
- **`src/Speed/Indep/Src/EAGL4Anim/FnRunBlender.cpp`**, **`FnTurnBlender.cpp`**, **`FnPoseBlender.cpp`**,
  **`FnStatelessQ.cpp`**, **`FnStatelessF3.cpp`**, **`FnPoseAnim.cpp`**, **`RawStateChan.cpp`**,
  **`RawPoseChannel.cpp`**, **`PhaseChan.cpp`**, **`Skeleton.cpp`**, **`FnCycle.cpp`**
  - What: blenders, raw channels and skeleton helpers written from empty files. `AnimUtil.h` gains `QuatMult`,
    `QuatRotate`, `QuatTransformPoint`, `EulToQuat` and `LinearBlendF3`.
  - How: the line map attributes an inlined call's argument setup to the callee's opening line, which is how
    the inlines were identified.
  - Fix: upstream declared `EvalPhase(float) override`. `override` is an empty macro in `src/types.h`, so under
    GCC 2.95 this silently added a new virtual. The real signature is `EvalPhase(float, PhaseValue &)`.
- **`src/Speed/Indep/Src/EAGL4Anim/eagl4supportdlopen.cpp`**, **`.h`** — What: `DynamicLoader::Resolve` and
  `Initialize` (2,352 B), plus `htotus`/`htotul` and `ELFAddr`. How: `htotul` must be one expression (a
  separate local shifted with `<<=` moves the `slwi`; 1,300 B), and `ELFAddr` needs the fast guard
  `if (offset < mDataLen)` first. The GCC-only `case a ... b` ranges later became plain labels so that MSVC
  compiles the loader. The size stayed exact; GameCube dropped from 100 to 99.22 %, which was accepted.
- **`src/Speed/Indep/Src/EAGL4Anim/MemoryPoolManager.cpp`**, **`.h`** — What: defines `EAGL4Anim::MatrixMultiply`,
  a pointer in `.data` at 0x80417160 initialized to `MtxMult`. It was the only undefined symbol that kept the
  unit from linking. Evidence: symbols.txt places it between `gFreeListSize` and `gMemoryPool`, and its ELF bytes
  `0x800A3D78` are the address of `MtxMult__FPQ25EAGL49TransformPCQ25EAGL49TransformT1`. `NewBlockAux` keeps
  only the DWARF's locals (`idx` r4, `r` r3) and is at 88.57 %.
- **`src/Speed/Indep/Src/EAGL4Anim/eagl4supportbase.cpp`**, **`system.cpp`**, **`SystemCmn.cpp`** — The allocator
  pointers follow the symbols.txt order (`gEAGL4ANIM_Malloc/Free` at 0x804170F4/F8, then `EAGL4Malloc/Free`);
  upstream had the pairs swapped. New files: `MtxMult` at global scope, and `Initializer::InitInternal` (pool,
  factory, `"AnimationBank"` registration). Both are added to the SourceList, together with
  `FnEventBlender.cpp` and `eagl4runtimetransform.cpp`, which upstream's list also lacks.
- **`src/Speed/Indep/Src/EAGL4Anim/eagl4runtimetransform.cpp`**, **`.h`** — What: the `.cpp` written from empty
  (`PostMult`, `ExtractQuatTrans`, `BuildSQT`, `MultMatrix`). `EAGL4::Transform` keeps `ATTRIBUTE_ALIGN(32)` on
  GameCube and uses `ALIGN_16` on PS2. Evidence: in the A124 stabs `FnPoseBlender::mAlignMatrix` is at 0x30,
  with 4 bytes of padding before it. How: `sondastabs.py`.
- **`src/Speed/Indep/Src/EAGL4Anim/FnEventBlender.cpp`** — `EulF3`/`QuatF4`/`TranF3` and their `*Interp` variants
  are `inline` even though `RawPoseChannel::InitAnimMemoryMap` stores their addresses in tables. GCC 2.9 then
  emits them from the deferred queue at the end of the unit, where the original has them. Without `inline` they
  come out at the parse point (differing DOL bytes 93,279 → 85,362).
- **Class shape across the headers** (`DeltaChan.h`, `PhaseChan.h`, `FnPoseBlender.h`, `FnRunBlender.h`,
  `FnTurnBlender.h`, `FnAnimFactory.h`)
  - Destructors that the original synthesizes (no `stw _vt` on entry) are not declared.
  - An in-class `operator delete` declared before the destructor is inlined; declared after it, it is emitted.
  - Some virtual bodies moved out of the class as `inline` definitions in the `.cpp`
    (`FnCompoundChannel::GetAttributes/Eval`, `FnCycle::Eval*`, `FnDeltaChan::GetLength`), which puts them where
    the original emits them.
- **Vtable order** (`CompoundChannel.h`, `MemoryPoolManager.cpp`, `FnAnimMemoryMap.cpp`, `RawEventChannel.cpp`,
  `eagl4AnimBank.cpp`)
  - Finding: GCC 2.9 emits a unit's vtables in the *reverse* order of class completion, because `walk_globals`
    walks a list built by prepending. A class is completed where its header is first parsed; its bodies are
    emitted where its `.cpp` is.
  - How: preprocess with the real flags (`-E`, seconds) and compare with `vtord.py`. This took the unit from 26
    of 31 misplaced vtables to 0.
  - What stays in this fork: `CompoundChannel.h` no longer includes `PhaseChan.h`; two `.cpp` files include the
    `Fn*.h` of the class they must complete; the include order in `MemoryPoolManager.cpp` and
    `eagl4AnimBank.cpp`; and `FnCycle.cpp` after `MemoryPoolManager.cpp` in the SourceList.
  - The other SourceList reorders were dropped when SourceLists were normalized to upstream's layout; objdiff
    does not measure emission order.

#### Everything else
- Static data that the original static initializer builds: the `"FPS"`/`AT_UNSIGNEDCHAR`/`ID_FPS` entry of
  `mReservedAttributeMetaData`, `mScratchBuffers[3]` with a zeroing constructor, and `AttributeId()` defaulting
  to `ID_INVALID`.
- Accessor bodies filled from the DWARF layouts in `RawStateChan.h`, `RawLinearChannel.h`, `RawPoseChannel.h`,
  `StatelessF3/Q.h`, `PhaseChan.h` and `PosePalette.h`.
- A slot-by-slot vtable audit removed `FnDeltaQ`'s `EvalWeights`/`EvalVel2D` overrides.
- `FnTurnBlender.cpp`: `printf` strings with a raw newline inside them (GCC 2.95 accepts that, MSVC does not)
  now use `\n`.

### Misc (`src/Speed/Indep/Src/Misc/`)

Units `zMisc`, `zMiscSmall` and `zMission`. Upstream's PR #141 rewrote most of zMisc after the merge base and
matched every GameCube function. 17 Misc files are now identical to upstream's, and the rest of #141 went
through a compiler filter in which value-returning `{}` stubs became declarations, `// STRIPPED` stubs became
comments, and invented `"TODO"` names absent from the DOL were dropped. The remaining differences are mostly
upstream's newer code. The fork also replaced its own old hand-written data blocks with real C: 33 string
literals and 86 float constants read back from the DOL. That is why `Timer.cpp`, `Main.cpp`, `Rumble.cpp` and
`Spline.cpp` contain literals such as `0.016666668f`.

#### Notable files
- **`src/Speed/Indep/Src/Misc/Config.cpp`**, **`Config.h`** (zMiscSmall)
  - What: 58 configuration globals defined in C, in the original `.data` order (0x8041ED7C–0x8041EEEC), with
    DWARF types and DOL values. About 20 of them are missing upstream (`bRumbleEnabled`, `EnableParticleSystem`,
    `DebugCameraNearPlane = 0.5f`, …).
  - `SkipFEPlayerCar` is `"bmwm3gtre46"` per the DOL bytes; upstream has `"bwm3gtre46"`.
  - `UnlockAllThings`, `SkipCareerIntro` and `SkipDDayRaces` are `bool`, per the DWARF.
  - `SkipFEPrintPerformances` stays uninitialized because its DWARF address is 0xFFFFFFFF.
  - `int SHAPE_clut = 0;` comes first because GameCube's static initializer is named `_GLOBAL_.I.SHAPE_clut`.
- **`src/Speed/Indep/Src/Misc/Hermes.h`**, **`Hermes.cpp`** — What: `StaticHandler<MessageT>` gets its handler
  pointer at offset 0 and a real `Call`, and `Create` stores the pointer. Upstream's `Call` is empty and its
  `Create` never stores the pointer. Evidence: the allocation names come from the original `.rodata`
  (`"Hermes::System"`, `"Hermes PortMessage"`); `"Attrib::vechashmap"` was found by following an A124
  relocation. Effect, with `EventManager::BindMessagesToLua`: zMain 46.80 → 69.73 %, +210 functions across 65
  message types.
- **`src/Speed/Indep/Src/Misc/Table.hpp`** — What: the operand order of `TableBase::GetIndex`, a `tTable<T>`
  constructor with an inline `GetValue`, and a new `tAverage<T>` ring buffer used by `CubicCameraMover`.
  `AverageBase::Recalculate()` is non-inline and defined in `Camera/CameraMover.cpp`, which makes it the key
  method, so the vtable is emitted in zCamera as symbols.txt shows (0x80083450).
- **`src/Speed/Indep/Src/Misc/Spline.hpp`** — `tCubic1D/2D/3D` stay here; #141's move to `Point.hpp` was not
  taken. The fork adds constructors such as `tCubic1D(short type, float dur)`, and setters. The DWARF expands
  these constructors in the camera movers and in `IconScroller`.
- **`src/Speed/Indep/Src/Misc/GameFlow.cpp`**, **`.hpp`**
  - Bodies for `IsGameFlowLoadingGame()`/`IsGameFlowPaused()`; the DWARF shows 21 and 27 inline expansions.
  - `RegionLoader::LoadHandler` is `inline`, at its line-map position.
  - `LoadGlobalAChunks` aligns from `bin_globala_bun - 1`: the target aligns from the byte before the symbol.
  - `IsPermFileLoading()` is guarded with `#ifndef EA_BUILD_A124`; the A124 lacks it.
- **`src/Speed/Indep/Src/Misc/Platform.h`** — `VMStats` (0x1C) and `VMStatsManager` (0x54), with the DWARF
  layouts. Their inline constructors are placed so that they call `Init` out of line, matching GameCube's
  `Init__7VMStats` and `gVMStatsManager_FE/LS/IG`. This replaces asm aliases.
- **`src/Speed/Indep/Src/Misc/PackedDecimal.h`**, **`FixedPoint.hpp`** — What:
  `FloatingPoint<...>::GetNormalizedLower/Upper`, and `PackedDecimal` with `short mExp : 5; short mMan : 11;` and
  `operator float()`. `FixedPoint.hpp` comes from upstream's `dev` branch, plus a default constructor.
  Deliberate deviation: the DWARF makes `PackedDecimal` `FloatingPoint<short,10,3,5,11>`, but the template form
  shifted `$LC` literal numbering in 19 units.
- **`src/Speed/Indep/Src/Misc/TestHooks/JuiceHooks/*`** — the Juice test-automation hooks.
  - Imported from `bit_of_ecstasyy`; upstream/main has 0-byte placeholders.
  - Compiled only under `MILESTONE_BUILD`: A124 and Xbox 360, whose symbol tables carry these names. GameCube's
    and PC's do not.
  - Fixes on top of the branch: a duplicate `ICountdown` that broke PS2 zMisc was removed; the real
    `MoviePlayer` header is used; `GetRawName()` became `GetName()` (`mQueueName` at 0x27C).
- **`src/Speed/Indep/Src/Misc/MissionEdit.cpp`**, **`.hpp`** (zMission, new) — eight empty debug and editor hooks
  (`DrawMissionComponents`, `AIEntityCreated/Destroyed`, `SendLuaOutput` ×2, `IsLuaOutputEnabled`,
  `SendToolIMMessage` ×2). Evidence: the A124 function list has exactly these, 8 B each. On GameCube only
  `_GLOBAL_.I.DrawMissionComponents__Fv` survives.

#### Everything else
- `bMemoryOverloads.cpp`: documents that the global `operator new/delete` are emitted as GCC 2.9's
  `__builtin_new`/`__builtin_delete` family (the ELF has no `__nw__FUi`); the milestone overloads come from
  `ps2_fixes`.
- `bFile.cpp`: `REALCORE_FILESYS_CXX_LINKAGE`, so the `FILESYS_*` calls use C++ linkage, as in GameCube's
  symbols. `DetectMicropause` takes two parameters, since `"%s - %s"` is not in `.rodata`.
- `QueuedFile.cpp`, `Rumble.cpp`: methods defined `inline` at their line-map positions.
- `SpeedChunks.hpp`: chunk IDs from `symbols/bchunks.txt`.
- `MWAttribUserTypes.h`: enums from upstream side branches; only the `AxlePair()` constructor is the fork's.
- `Joylog::Save` uses the DWARF local `total_size`; `IAttribAllocator` methods are pure virtual.
- SLUS: the 4-argument `bMalloc(size, name, line, params)` exists only in milestone builds (the A124 has
  `bMalloc__FiPCcii`; SLUS-21351 does not). The named allocations in `LoadTempPermChunks`/`HighAttribAlloc`
  (upstream's form) cost SLUS functions only while it was wrongly compiled as a milestone; it no longer is.
- `GameFlow.cpp`, `Main.cpp`, `ResourceLoader.cpp` and `SpeedChunks.cpp` define the 32 globals zMisc used and
  nobody defined since the hand-written data blocks went away (package names, code-overlay settings,
  `CarLoaderPoolSizes`, `ChunkNameTable`, …): every value read from the DOL, in the original `.data` order.
  Upstream's duplicate definitions in `FEPkg_Hud.cpp` swap two of the HUD names.

### Online (`src/Speed/Indep/Src/Online/`)

In upstream/main this directory holds empty `.cpp` stubs plus `OnlineCfg.hpp`. This fork carries about 10,800 lines of
lobby, buddy, network, voice and in-game client/server code (`Lobby*`, `BuddyCore`, `ConnectionCore`,
`NetworkCore`, `SmartBitstream`, `WebOfferUG2`, `InGame/*`).
- **This code was not written in the fork.** It was imported from upstream's side branch `bit_of_ecstasyy`, which
  is not merged into upstream/main, and filtered through the compiler on the way in.
- **None of these `.cpp` files is compiled by any version.** On GameCube the online layer is disabled; `zOnline`
  is a 0x7C-byte overlay whose SourceList is empty, as in upstream.
- Only `OnlineCfg.hpp` (included through `FEDatabase.hpp`) and `InGame/OnlineRacer.h` reach a compiler. Treat
  `Src/Online/*.cpp` as unverified source.

#### Notable files
- **`src/Speed/Indep/Src/Online/OnlineCfg.hpp`** — What: `OnlineRaceParameters` in its final 0x10 layout
  (`EventHash` 0x0, `NumLaps` 0x4, `TrackDirection` 0x5, `CopDensity` 0x6, `float TimeOfDay` 0x8, `bool IsLapKO`
  0xC); the branch had the 0xC layout of the A124 alpha. Evidence: every definition of the type in the DWARF,
  which places its declaration at `database/RaceDB.hpp:157`. The type is defined both here and in
  `Frontend/Database/RaceDB.hpp`, as in the original; a shared `ONLINERACEPARAMETERS_DEFINED` guard stops the
  two definitions from colliding.
- **`src/Speed/Indep/Src/Online/InGame/OnlineRacer.h`** (new) — the DWARF gives `OnlineRacer` `total size: 0x1`
  and no members, and `GetPersona()` returns `NULL`: on GameCube the class is a stub. `Lua/LuaGameHooks.cpp`
  uses it (`Game_AddPlayer`).
- **Import filter** (whole directory) — a small script compiled the directory with `-k 0`, dropped the imported files that
  failed, and repeated until clean (two passes). It dropped `VoiceCore.hpp`, so `VoiceCore.cpp` has no header.
  One-line `{}` bodies became declarations, because in the DWARF-derived headers `{}` means "body not shown"
  (11 in `WebOfferUG2.hpp`, plus `ListSimConditions` and `PossiblyDoDataCorruption`). Local prototypes were
  replaced by the real headers.
- Related, outside this directory: `World/OnlineManager.hpp` (73 of the branch's 88 declarations) and
  `Misc/TestHooks/JuiceHooks/*` come from the same branch.

---

## Platform code: GameCube (`src/Speed/GameCube/`)

Many of upstream's GameCube `Src/` files are empty placeholders and its
`zPlatform` SourceList includes nothing. This fork fills the two GameCube-only unity
units, `zEcstasy` (12 files in `Src/Ecstasy/`) and `zPlatform` (18 files), in the
original's file order (`symbols/file_names.txt`), working from the GameCube
DWARF (types, locals with registers, inline trees), the line map
`symbols/debug_lines.txt`, the splitter's `.s` and the ELF symbols. The core of
`eMathE`/`eMatrixE`/`eViewPlat`/`eStrip`/`eSolidPlat` and a first `EcstasyE.cpp`
came from upstream's unmerged `pr-117` branch (August 2026) and were extended.
Both units stay NonMatching; `zEcstasy` gets `-mps-float` for SN's
paired-single builtins.

### Notable files
- **`Src/Ecstasy/EcstasyE.cpp`** (1,036 → 4,606 lines) — `MaybeChangeViewMode`
  (100 %), `SetScreenBuffers` (ten named render targets), the
  `eViewPlatInterface::Render` family, sky, motion blur, EURGB60/PAL checks; the
  4,544 B static initialiser rebuilt from twenty globals with type, ELF value and
  order. The bus clock is the SDK's `OSTicksToMilliseconds(OSGetTime())`, which
  reads `__OSBusClock` exactly as the original does (a declared `e_OSBusClock`
  with no definition was removed).
- **`Src/Ecstasy/EcstasyEx.cpp`** (983 → 2,848 lines) — render state, horizon
  fog, `eDataRender::Render` (5,980 B), texture buckets (moved here because the
  line map puts them here); written with 139 SN paired-single builtins
  (`__builtin_psq_l`… on `float __attribute__((mode(PS)))`) rather than asm.
- **`Src/Logitech/*`** (7 `.cpp` + 4 new headers) — `Force` 0x100, `Wheels`
  0x880, `LGWheels` 0x16E4 from the DWARF; where the stores contradict the dump
  the code follows the stores (`Playing`/`EffectID` are 4-byte `[4][8]` arrays at
  `channel*32+effect*4`); `LGForceType` values come from each `StopXxx`'s
  `li r5,N`; the driver API (`LGOpen`, in `LibSN/steering.c`) is `extern "C"`.
- **`Src/MemoryCard/MemoryCardImp.cpp`** — the real class from Indep's
  `MemoryCardImp.hpp`, no asm; `MemoryCardHelper.hpp` is not included because
  it put the file name into `.rodata`, which the original object lacks.
- **`Src/globala_bun.cpp`** — only declares `bin_globala_bun[89716]`, the
  global `.bun` resource bundle the DOL embeds. Its bytes are game data and are
  not distributed; `zPlatform` stays NonMatching, so the link takes them from
  the original object.
- **`bWare/GameCube/OdemuExi2/src/*.c`** (new) — the splitter's one object was
  three units: MWCC `GC/1.2.5n` for `DebuggerDriver.c`, `GC/1.2.5` for the two
  Amc files (the split point, 0x803465D0, is where failures change). Matching.
- **`dolphinsdk/src/os/OSUtf.c`** — the SJIS/UCS tables with the DOL's real
  pointers (upstream `{0}`): `STT_FILE` gives 0x8043C2E0 to it and the `.data`
  is exactly 70,208 B with 133 relocations.

### Everything else
- `Src/Ecstasy/`: `ePShader`/`eVShader` (18 TEV recipes, vertex formats; order
  from the line map), the pr-117 core (`eMathE`, `eMatrixE`, `eViewPlat`,
  `eStrip`, `eSolidPlat`; this fork reorders `eProject` by the DWARF), `eLightE`,
  `eEnvMapE`, `eVisualTreatment`, `TextureInfoPlat`, `xSprites`. `Src/`:
  `Platform_G.cpp` (3 → 564 lines: init, reset and DVD-error handling),
  `SunE`, `AcidFX_G`, `xSparks`, `JoyE`, `Movie_GC`, `G.hpp`, `DemoDisc_G`.
  `GXVert.h`: `GXWGFifo` placed with SN's `__attribute__((address(…)))`.
- Remaining asm: EA platform code kept as such (GQR writes `mtspr 914–919`,
  `fres`, `dcbt`, `fsel`, paired-single `eMulVector`/`eConvertToGX34`; the DWARF
  shows locals `t0..t6`/`FP0..FP11` in their registers) and pr-117's statements
  in `eViewPlat.cpp`, where `TransformBound` moves data through fixed stack
  slots with 16 one-instruction `psq_l`/`psq_st`.
- Upstream-only: the per-platform `eSolidPlat.hpp` (This fork keeps
  `eStripEntry`/`eSolidPlatInfo` in Indep's `EcstasyData.hpp`).

## Platform code: PS2, Xbox 360 and PC (`src/Speed/PSX2/`, `Xenon/`, `PC/`)

Most differences here are newer upstream commits (the `eSolidPlat.hpp` headers,
`xSparks.h` for Xenon/PC, clangd-only declarations in the PS2 STL headers).

### Notable files
- **`PSX2/Src/Online/PS2Isp.cpp`, `PS2Isp.hpp`, `PlatformNetworkCore.cpp`** —
  byte-identical to upstream's `bit_of_ecstasyy` branch, imported with the
  Online subsystem (the A124 lists these paths and symbols); the `.cpp` files
  are in no SourceList, so nothing compiles them.
- **`{PSX2,Xenon,PC}/Src/Ecstasy/eLightPlat.hpp`** — `PlatConvertColor`
  declared because common `eLight.hpp` calls it; no PS2 body, since the
  GameCube version's byte swap assumes big-endian.
- **`PSX2/bWare/Src/ee/gcc/ee/include/stdio.h`** — `snprintf`/`vsnprintf`
  prototypes (the PS2 libc has them, per `NFS.MAP`).

## bWare (`src/Speed/Indep/bWare/`)

Sixteen files carry this fork's work (`bChunk.*`, `bSlotPool.*` differ only by
newer upstream commits). EA's intrinsics in `bMath.hpp` (GameCube
`frsqrte`/`fsel`/`fabs`, PS2 `lqc2`/`sqc2`/`pexew`…) are the same 20 statements
as upstream.

### Notable files
- **`Src/bPrintf.cpp`** — `_bOutput` (5,180 B) matches byte for byte. Its body
  was **imported from the matt-lacerda/nfsmw fork, branch `zbware-complete`
  (CC0-1.0)**, after checking that fork cites the same target SHA-1. It follows
  the original where upstream differs: rounding on `number * 10.0 >= 5.0`
  (upstream `0.5`), `%v` vectors as `"[%*.*f,%*.*f]"`. `memset` is declared
  without prototype (the target emits `crclr cr1eq` before each call), which is
  why `bWare.hpp` includes `<cstring>` only on Win32.
- **`Inc/bMath.hpp`** — GameCube `bAbs` initialises `float f_abs = 0.0f;`: an
  uninitialised local whose only write is an asm output carries `= 0.0f` in the
  original (the dead `lis` survives and CSE reuses it); 5,212 B in two units,
  while the same change on `bMin`/`bMax` cost 10,636 B and was not made.
  `bVector2/3/4` members follow DWARF order.
- **`Src/bMemory.cpp`, `SpeedScript.cpp`** — plain literals. Four macros that
  joined two messages into one literal, to reproduce the splitter's single
  symbol, were removed.
- **`Src/bMath.cpp`, `bVector.cpp`, `bQuaternion.cpp`** — functions the linker
  dropped keep a body only where it follows their DWARF record (`bToPolar`,
  `bLength(const bQuaternion *)`, `bDistBetween(const bVector4 *, …)`,
  `bGetPolyArea`); `bOldATan` and `bFastATan`, whose bodies contradicted it,
  are only named. `bSinTable[513] = {0}` is the dropped table with its DWARF
  name and type; `Slerp` uses the original's 0.05f.

### Everything else
- `bList.*` (inline order from the DWARF), `Strings.*`, `Espresso.hpp`,
  `bMemory.hpp`, `bDebug.cpp`, `bMatrix.cpp`.

## Support library (`src/Speed/Indep/Libs/Support/`)

38 files differ (8 new): PS2 fidelity, the GameCube zFoundation unit and
portability. Each layout change was first checked with a **compiler probe** — a
file built with the real PS2 flags that stores `sizeof`/`alignof`/`offsetof` in
globals — against the A124 stabs, then measured on every build.

### Notable files
- **`Utility/UVectorMath.hpp`** (EA's PS2 VU0 macros, also upstream) — explicit
  field masks on 57 instructions: without one the assembler emits `.xyzw`, the
  original writes `.xyz`/`.x`/`.xz`, and functions sat at 99.99 % for that
  alone. The masks come from a table "our opcode → original opcode" over zAI's
  `DIFF_OP_MISMATCH` rows, 99–100 % consistent (`vmul`→`.xyz` 238, `vmulax`→`.x`
  179, `vsub`→`.xyz` 133…). The rule is semantic: three-component operations
  `.xyz`, dot-product accumulation lane `.x`, matrix×vector routines `.xyz`,
  `vmul.xz` only in `VU0_v3lengthxz`; `VU0_v4scale` keeps `.xyzw`. SLES
  26.51 → 27.87 %, SLUS 30.57 → 32.02 %.
- **`Utility/UTypes.h`** — `UMath::Matrix4` is **no longer type-aligned** on
  PS2 (upstream `struct PS2ALIGN16 Matrix4`, from Carbon): in the A124
  `SmackableParams` is 0x5c and `XenonEffectDef` 0x58 with a `Matrix4` inside,
  impossible with a 16-aligned type since `sizeof` rounds up to the alignment.
  `PS2ALIGN16` goes on the padded members (`USpline::fSplineMat`,
  `Pkt_Body_*::mMatrix`…); locals are unaffected (every local struct gets the
  R5900's 16-byte `BIGGEST_ALIGNMENT`). `Vector4` is not type-aligned on
  PS2/X360 either (the original copies it with `ldl`/`ldr`).
- **`Utility/UDefs.h`** — `ALIGNVEC` is `__declspec(align(16))` on Xbox 360: the
  X360 original reads `RigidBody` vectors at 0x30/0x4C, not 0x28/0x44 (338
  zPhysicsBehaviors functions sat at 95–99 %). Rule for members: aligned on X360
  too → `ALIGNVEC`; PS2 only → `PS2ALIGN16`.
- **`Utility/UCOM.h` + new `UCOM.cpp`** — `class PS2ALIGN16 Object`
  (`IUnknown` already was): every `CDAction*` read its members 16 B early
  (`lw 0x58` against `0x68`); the probe gave `sizeof(CameraAI::Action)` 0x18
  against the ELF's 0x30. SLES +71, SLUS +80 functions. `Factory::CreateInstance`
  moved out of class: an in-class body is implicitly inline in GCC 2.9 and
  would not emit the original's standalone symbols.
- **`Utility/UVectorMath.cpp`** (105 → 839 lines) — GameCube-only paired-single
  `VU0_*`. The DWARF lists float temporaries `t0..tN` with registers and no C
  use, so each function is one `asm` with them as `"=f"` outputs and GCC picks
  the registers (`REG_ALLOC_ORDER` starts f0, f13, f12…: with N outputs the last
  gets f0, the rest count down). 41 functions byte-identical, no pins.
  Upstream's portable `VU0_ExtractX/Y/ZAxis3FromQuat`, `VU0_quattom4` and
  `VU0_MATRIX4_mult` are compiled by the other builds (a merge had nested them
  inside the GameCube block).
- **`Utility/UVectorMathCPU.hpp`** (PC, Xbox 360, also upstream) —
  `VU0_v3scaleadd` computed `a + b * s`; the original computes `a * s + b` on
  GameCube (`ps_muls0` on `a`, then `ps_add` with `b`) and on PS2 (`vmulx` on
  `a`, then `vadd` with `b`). It was a logic bug in the CPU builds' physics,
  found when the Android port's vehicle misbehaved. With the right form
  `UMath::ScaleAdd`, `RigidBody::DistributeMass` and `RigidBody::Accelerate`
  match on PC, and the five `GetWheelCenterPos` of the suspensions on Xbox 360. `VU0_v3crossprod` had the same kind
  of bug: it wrote `dest.x` before reading `a.x`, and `SuspensionRacer` calls `Cross(c, forwardNormal, c)`; computed
  into a temporary (as GameCube and PS2 read everything before writing) the `RigidBody` functions that expand it match
  on PC and Xbox 360.

### Everything else
- New zFoundation units: `ExpressionEvaluator.cpp` (the CARP expression VM,
  with an original bug where `func3f`/`func4f` pass one argument), `Carp.cpp`,
  `StringHash.cpp`, `StringRegistry.*`, `USpline.cpp`, `UStringToNumber.cpp`,
  `UCollections.cpp`, `UFoundationBody.cpp`; also `UGroup.cpp`, `FastMem.cpp`,
  `UMath.*`, `UTLVector.h`, `SAP.h`, `UListable.h`, `CARP.h`.

## Library compilers and flags (`configure.py`)

Upstream builds realcore, realmemcard, rcmp and eathread with the game's flags
(`-O1 -mps-nodf -G0 -fno-static-dtors -fforce-addr -fmove-all-movables …`) and
libc with the bare base; it does not list vp6, realgraph or the Metrowerks
units. The criterion was *source written, percentage low* (suspect flags or the
compiler before the source), with each flag measured in both directions.

| library | compiler | GameCube flags, relative to the game's |
|---|---|---|
| `libc` | ProDG ngccc | `-O2`; per object `-msafe-sda`, `-fno-builtin`, `-G1024`, `-mstrict-align` |
| `libsn` | ngccc | libc flags + `-G0` (on libc itself `-G0` costs 13,032 B) |
| `libsn_mw`, `libsn_mw_steering` | MWCC `GC/2.7` | Dolphin flags, `-inline off -lang=c`; steering `-O4,s -fp_contract on -use_lmw_stmw on` |
| `__va_arg`, `OdemuExi2` | MWCC `GC/1.2.5n` | runtime / Dolphin flags |
| `realgraph` | ngccc | − `-G0`, − `-fforce-addr`, + `-fstrength-reduce` |
| `vp6` | ngccc | `-O2`, `-mps-float`, − `-G0`, − `-fforce-addr`, − `-fmove-all-movables` |
| `rcmp` | ngccc | `-O2`, `-mps-float`, `-fno-force-addr`, − `-G0`, − `-fno-static-dtors`, − `-fmove-all-movables` |
| `realcore` | ngccc | `-G8`, − `-fforce-addr`, − `-fno-static-dtors`, + `-fstrength-reduce -fno-implement-inlines` |
| `realmemcard` | ngccc | as realcore without `-G8`, + `-fshort-wchar` (symbols mangle `Uw`) |
| `eathread` | ngccc | `-O2` |

Telltales: `@sda21` on small globals means no `-G0`; FPR saves as `stfd`, not
`psq_st`, mean `-mps-float`; an address rematerialised at each call means no
`-fforce-addr`; emitted `_GLOBAL_.D.` means no `-fno-static-dtors`. **On PS2 the
EA middleware uses another compiler**, SN's `ee-gcc 2.95.3` build 1.36, not the
game's `2.9-ee-991111b`: the A124 `.mdebug` records each file's build directory
(`PlayStation2SN` packages), and of five candidates only build 1.36 saves
s0–s7/fp with `sq` and ra with `sd`, as 1,863 of 1,864 library functions do.
There `long long` is 128-bit; files still using it get `-frelax-128`.

## RealCore (`src/Speed/Indep/Libs/realcore/`)

Upstream has the `6.24.00/` tree (paths below are under it) as placeholders:
all 50 files under `source/` are empty. This fork writes 65 files (~172 KB; 33 of 44
objects flagged Matching) from the GameCube DWARF and line map; `locale`,
`filesys_c`, `eventqueue`, `effectimp`, `effect` and several input headers came
first from upstream's `csis` branch. No asm; `aligned(8/32)` only where the
extracted `.bss` shows the padding, and 661 bare `//` lines keep inline bodies
on their original line numbers (the padding comments cite the line map).

### Notable files
- **`source/file/cmn/filesys.cpp`** (2,018 lines, NonMatching) — the
  asynchronous file system (per-device operation queues on a worker thread, the
  `FILESYS_*` API, devices, search paths), from DWARF sizes (FILEOPERATION 0x48,
  FILEDEVICE 0x3D4), the original `__FILE__` with line tags 312/432 and
  `AddToQueue`'s DWARF locals.
- **`include/common/realcore/file/filesys.h`** — the prototypes sit in an
  `extern "C"` block a unit can switch off (`REALCORE_FILESYS_CXX_LINKAGE`,
  `REALCORE_FILESYS_IMPLEMENTATION`): the original has **both linkages** — snd
  calls the unmangled names, realcore's own units the mangled ones
  (`FILESYS_close__FiiPv`), with the shims `filesys_c.cpp`/`filesys_cc.cpp`.
- **`source/file/gc/dvd_device.*`, `hd_device.cpp`** — the `"dvd:"` driver over
  the Dolphin DVD API (`gCurRead` `aligned(32)`: the extracted object puts it at
  `.bss`+0xE0 after 28 B of padding, giving the target's 33,088 B) and the
  `"hd:"` driver over SN's PC file-server calls, under their real names
  (`PCopen`, `PCread`… exported by `metrotrk`).
- **`include/common/realcore/input.h`** and `input/*` — a guarded
  `#pragma interface "input.h"`: the eleven `_vt.Q29RealInput*` vtables sit in
  an unattributed `.rodata` gap, so our objects must not emit them;
  `interface.cpp` lifts the guard and emits only `Interface`'s. A backward
  `goto` loop has no `NOTE_INSN_LOOP_BEG`: `EnumerateDevices` 87.56 → 100 %.
- **`include/common/realcore/system.h`, `source/system/cmn/mutex.cpp`** —
  `MUTEX` is 0x10 on PS2 (A124 debug data) and 0x1C elsewhere; the PS2 mutex is
  a semaphore-based `MutexPrivate`. The header also declares the print API with
  the DWARF's names (`PRINTCHANNEL` 0–14, `PRINT_USER_CHANNEL` 32…).
- **`include/common/realcore/std/list.h`, `impl/std/list.inl`** —
  `ListSingle<T>` and its iterator, padded onto the line map's lines.

### Everything else
- `file/` (`hlafile`, `filesysopts`, `syncfile`, `hlsfile`, `filesys_c`/`_cc`,
  `gc/device.cpp`), `std/` (`locale`, `memclear`, `gc/memcopy`, `gc/memfill`,
  moved to the original's `std/gc/`), `debug/`, `system/` (`systemvars` also
  holds `RealFontOld::gFontDriver` and `RealShape::MemObject::sAllocator`).
  Top-level `Libs/realcore/include/common/realcore/system.h`: a copy that came
  with realstream from upstream's `eaxsound-clean` branch.
- Not in this fork: one shared `FILESYSOPTS` (the 0x38 layout is declared in four
  units) or any model of realcore 6.21 (`6.21.00/…/getm.inl` is empty).

## Middleware packages (`src/Packages/`)

Upstream has eathread, no vp6 at all, and four realmemcard files added after
the fork point. This fork adds vp6 (30 sources, 26 flagged Matching), realmemcard (21
new files) and eathread fixes, in the original build paths of
`symbols/file_names.txt` (`d:/packages/vp6/1.0.6`,
`c:/packages/realmemcard/3.04.01-layer2`). Types come from the GameCube DWARF
through `scripts/libdwarf.py`, which turns the dump into compilable C (it
produced `PB_INSTANCE`, 0x4958 B, in a second); `scripts/libdiff.py --lines`
shows the target with each instruction's source line. Only GameCube has these
units: PS2 ships other versions (vp6 1.0.3, realmemcard 3.04.00). Upstream's
`csis` branch decompiled the same libraries independently; in a unit-by-unit A/B
ours led (e.g. `gc_driver` 99.91 % vs 97.43 %) or theirs did not compile here.

### Notable files
- **`vp6/1.0.6/source/decode/gc/criticalpath.c`** — the VP6 hot path in C plus
  seven functions that are **entirely Gekko paired-single assembler in the
  original** and stay so (`IDct64_GC`, `ReconBlock_GC`… ~490 `ps_*`/`psq_*`
  lines; the DWARF has locals `FP0..FP8`, `t0..tN` with no C use). NonMatching
  since its register pins were removed. `cmn/vfwpbdll_if.c` keeps EA's GQR code
  (`MTGQR`, `OSInitFastCast`).
- **vp6 C units** (`deblock`, `decodembs`, `dering`, `postproc`, `scale`…) — the
  `memcpy`/`memset` prototype decides per unit whether GCC expands the builtin
  (`unsigned int` expands, `unsigned long` conflicts and calls), and seven units
  need `-fno-common` because the original allocates their globals.
- **realmemcard** — **first imported from upstream's `csis` branch** (whole
  subtree; six files still identical), then extended: new
  `impl/gc_interface_impl.h` (`GCMessage`/`GCInterface` moved there because the
  DWARF shows `struct GCMessage` in exactly six units, which removed stray
  `.rodata` from four others), `gc/gc_driver.cpp` (1,050 lines, written here),
  `memcard_taskmanager.cpp` (+3,016 B from eight fixes read off the line map),
  and two literal errors of the import fixed. Functions whose DWARF range is
  `0xFFFFFFFF` (inline copies the linker dropped) were not written.
- **`eathread/1.1.0/source/gc/eathread_thread_gc.cpp`** — the DWARF's code
  (`gThreadDynamicData`, `AllocateThreadDynamicData`) replaces a hand-made
  static initialiser; the static-init comes out right only at `-O2`. Upstream's
  `char[832][32] = {0}` put 26,624 B of zeros in `.data`. New
  **`include/eathread/ps2/eathread_semaphore_ps2.h`** (A124 types) lets the 13
  `path` units compile on PS2.

### Everything else
- vp6: the rest of `cmn/` (`idctpart`/`quantize`/`clamp` written from scratch,
  `allocator.cpp` over `EA::Allocator`) and `gc/` (the idct/recon function
  tables). realmemcard: `memcard_interface_impl.h`, `driver.h`, `gc_driver.h`,
  `gc_blockcalculator.*`, `locale.cpp`, `memcard_utilities.*`.

## rcmp movie player (`src/egami/`)

Upstream has nothing here. This fork adds 13 sources (~4,300 lines) under the
original `d:/env/egami/rcmp/dev`: EA's RCMP container and player
(`AV_PLAYER`), the VP6 and MAD codec wrappers, the MAD video decoder and the GX
swizzlers, written here (the csis A/B above covers rcmp too). rcmp is an
**older build**: its compile units say `GNU C++ 2.95.3 SN BUILD v1.72` (the
game: v1.76) and its line map expands `D:/packages/realcore/6.21.00/…/getm.inl`.

### Notable files
- **`av/cmn/avplayer.cpp`** — `AV_PLAYER` (0x94) with a local `FILESYSOPTS` of
  **0x34** ending at `DiscType`: the realcore 6.21 layout rcmp was built against
  (6.24's is 0x38, with `mErrorRetryCount`).
- **`decoder/cmn/rcmp_vp6_codec.cpp`, `rcmp_mad_codec.cpp`** — `CODEC`,
  `CHUNK`, `DECODER`; giving `CODEC` a key method removed a duplicate 64 B
  vtable but cost 728 B and four functions, so it was reverted.
- **`decoder/cmn/madidct.cpp`** — NonMatching in plain C: the target keeps
  `src[6]` in CTR and a carry in LR, the reverse of GCC's choice; ProDG 3.9.3,
  3.8.1 and 3.7 give the same `.text`, ruling out the compiler version.

### Everything else
- `rcmpbase`, `maddec`, `maddeca`, `rcmp_*_chunk_types`, `bigyuvswizzler`
  (`aligned(32)`: the DWARF says 0x640, the members sum to 0x62C),
  `bigswizzler`, `audioplayer`, `avsubtitle`.

## SN ProDG runtime (`src/LibSN/`)

Upstream has `crt0.s` and twelve empty placeholders. This fork writes twelve units
(ten C, two assembler) and gives each its **real compiler**. Three are
Metrowerks, not ProDG GCC: no `gcc2_compiled.` marker (present in 100 of 105
GCC units of the game, 0 of 95 Metrowerks ones), `vm` calls `__div2i` and
`_savegpr_23/24` (GCC would call `__divsi3`), statics are mangled `name$NNN`,
not `name.NNN`, and `stwu` before `mflr` means CodeWarrior 1.3–2.7.

### Notable files
- **`steering.c`** (47 KB) — the Logitech wheel driver. Its `STT_FILE` is
  `allsrc.c`, an amalgamated build identified by the 27 locals behind it
  (`HandlePedals`, `CookValues`…). MWCC flags read off the target: with
  `-O4,p` MWCC unrolls fill loops ×8 and turns `/360` into a magic `mulhw`; the
  target has counted loops, a real `divw` and contracted `fmadds`. No DWARF.
  Still has seven one-instruction MWCC `asm {}` (`extsb`, `fsubs`, `fctiwz`…).
- **`vm.c`, `VMMapping.c`, `VMPageReplacement.c`, `vmbase.c`** — the ARAM
  virtual-memory library (`STT_FILE` `VMBase.c`). `vmbase.c` keeps what has no
  C form: six whole-function `asm` routines (TLB invalidation, SDR1 setup, the
  DSI/ISI exception handlers) and inline `tlbie`/`mtsr`/`icbi` blocks;
  `VMMapping.c` keeps a three-instruction asm block (`mr`/`addis`/`stw`).
- **`sndvd.c`** — the ProDG Target Manager DVD emulation (traps the DI
  registers with the DABR); keeps `mtspr 1013` and the `DSIExcHandler` vector
  as file-scope `__asm__`, annotated as transcribed (it reads r1/r2 of the
  interrupted program and ends in tail branches GCC 2.9 never emits).
- **`FSasync.c`, `ppc2D2.c`** — the ELF gives `FSasync.c` its own `STT_FILE`
  inside the splitter's `ppc2D2` (2,104/2,104 B in C); `ppc2D2.c` keeps, in one
  file-scope `__asm__`, the seven 4-byte tail branches SN aliases onto libgcc.
- **`metrotrk.s`** — MetroTRK, **kept as assembler** because it was never C:
  9,916 of 11,988 B read callee-saved registers without saving them or use
  `rfi`/`mtmsr`/`mtspr`, addresses are built with `lis @h`+`ori @l` (seven MWCC
  versions and ProDG 3.9.3 all emit `@ha`/`addi`), and no DWARF line falls in it.
- **`pro2D4.s`** (`_savegpr_*`/`_restgpr_*`), **`crt0.s`** (+64 B `.data`). That
  `.data` and metrotrk's `.bss` had been given to zFeOverlay by `splits.txt`;
  counting which objects reference each label moved them.

### Everything else
- `crt2D1.c` (libgcc2 `L__main`), `syscalls.c` (UART and host-call I/O); the
  other ten placeholders stay empty.

## C runtime: libc, libm, libgcc2 (`src/libc/`, `src/Runtime.PPCEABI.H/`, root `src/*.c`)

Upstream has no `src/libc/`. SN's C library ships **without DWARF**, so each
unit was **transcribed from the public source it was built from** — newlib 1.8,
newlib's fdlibm, gcc-2.95 `libgcc2.c` — and only SN's changes were rebuilt; 110
of 118 objects have source. **"libc" is two libraries**, told apart by one line
of the target `.s`: a constant pool loaded with `@sda21` means precompiled by
SN, `lis @ha`+`@l` means built by the game's toolchain (SN's `floor` is 348 B,
the game's `sn_floor` 372 B: exactly six extra `lis`).

### Notable files
- **stdio (`fopen.c`, `stdio.c`, `fflush.c`, `wsetup.c`, `fseek.c`, `local.h`,
  `sn_buf.cpp`…)** — newlib 1.8 with three SN deltas: `__sinit`/`__sfp` renamed
  `_sn_sinit`/`_sn_sfp`, no `sfmoreglue`/`_free_r`, ten FILEs with preallocated
  buffers. SN's real `sys/reent.h` was already in the tree (`sizeof(FILE)` 0x60,
  `__sglue` at +0x1D8); `STT_FILE` locals say which unit owns each static. Nine
  of thirteen units matched on the first compile.
- **libm (36 `e_*`/`ef_*`/`k_*`/`kf_*`/`s_*`/`sf_*` units)** — newlib's fdlibm
  **verbatim** (`static const`, literals) instead of an emulation of the
  constant layout, with real ngccc options per object: `-msafe-sda` (the pool
  goes to `.sdata` via `@sda21` in first-use order, the original's),
  `-fno-builtin` (`fabs`/`sqrt` stay calls), `-G1024` for tables >8 B. Found on
  the way: `T[] = {…}` is never small data for this ngccc (write `T[13]`), and
  `while (n--)` folds to `--n != -1`, so `sf_fmod.c` keeps `(i = n--) != 0`.
- **`vfprintf.c` + `vfprintf_1.c`** — newlib `vfprintf` with SN's output over a
  static 128 B buffer and a thousands separator (`%'d`), compiled again with
  `INTEGER_ONLY` for `_vfiprintf_r`; NonMatching since their register pins were
  removed (same instructions, other registers).
- **`math_support.c`** — `sn_floor`/`sn_fmod`/`sn_log`/`sn_log10`, the fdlibm
  routines the *game* compiles for SN's `fftoa` (`itoa.c`).
- **`libgcc2*.c`** — one `L_*` routine per unit; `__clz_tab` declared as in
  SN's own `longlong.h:1259` (from the ProDG GCC sources in
  `orig/prodg/NGC_GNU_SRC/`); `libgcc2_10.c` runs up to the `new_eh_context`
  string that survives after the linker drops the rest of SN's `L_eh` block.
- **`src/Runtime.PPCEABI.H/__va_arg.c`** — Metrowerks' `__va_arg`: the splitter
  put it in `ppc2D2.tmp` among GCC bridges, where ten source variants stalled at
  58.49 %; its own object with MWCC `GC/1.2.5n` gives 244 B exact.
- **Root `src/qsort.c`, `atexit.c`, `sn_malloc.c`** — newlib's `qsort` and
  `atexit`; ProDG's public `malloc`/`free`/… are OSPanic traps that print LR−4,
  with a one-instruction `mflr` asm (this GCC's `__builtin_return_address(0)`
  reloads LR from the stack).

### Everything else
- `setjmp.s` (newlib's `setjmp.S`, assembler in newlib too), `strtod2.c` (the
  Tcl/Sprite `strtod` SN ships under that name), `itoa.c`, `mbtowc_r.c`,
  `locale.c`, `impure.c`, `ctype_.c`, `mem*`/`str*`/`wcs*`, the `printf`
  family. No source yet: `e_atan2`, `s_atan`, `vfscanf`, `puts`, `atoi`,
  `strtol`, `fvwrite`, `k_tan`.
- `src/types.h` only adds an `__ANDROID__` branch from an Android port test.

## PS2 runtime: libgcc, newlib and SN's libc (`src/Speed/PSX2/bWare/Src/ee/gcc/`)

What the PS2 game links from its toolchain, per the A124's `NFS.MAP`, built from the public sources of that
toolchain (each file keeps its copyright and licence header; `README.md` in that folder lists origins and licences).

### Notable files
- **`src/gcc/` and `objdir/gcc/`** — libgcc and the C++ runtime (`tinfo`, `exception`, `_eh`) of GCC
  2.9-ee-991111. `objdir/` reproduces what the gcc build generates (`tconfig.h`, `tm.h`, `fp-bit.c`/`dp-bit.c`
  and `#define L_x` + `#include libgcc2.c` wrappers). `_eh` needs `-fexceptions`, as `LIB2FUNCS_EH` in gcc's
  Makefile passes it; without it this cc1 aborts in `__rethrow`.
- **`src/newlib/`** — newlib 1.8.1 libm and libc as Sony's toolchain shipped them.
- **`src/newlib/libc/machine/r5900/*.S`** — the 13 string functions (`memchr` … `strncpy`) are Cygnus' hand-written
  R5900 assembly (MMI, quadword), copied unchanged: the original matches it instruction for instruction. Rule for
  this fork: assembly that was written as assembly at the source enters as is, with its licence (see
  `docs/asm-de-ea.md`). Preprocessed with `ee-gcc -E` and assembled with SN's Ps2EeAs (`tools/ee_as_S.py`).
  `setjmp` is not there: the game's saves with `sq`/`swc1`, a later newlib.
- **`sn/libc/`** — what SN added or changed in its libc, written from the binary in newlib style: `snprintf`,
  `malloc`/`mlock`, the wide printf (`vfwprintf`, `vfwiprintf`), `wcslen`/`wmemchr` (hand-unrolled), `mbtowc_r`
  with SN's 16-bit `wchar_t`.
- **`src/libc/vfprintf.c` and `itoa.c`** — shared with GameCube; the PS2 deltas (FILE without `_data2`/`_flags2`,
  64-bit `long` for `quad_t`/`u_long`, little-endian `EXTRACT_WORDS`) go under `EA_PLATFORM_PLAYSTATION2`.

### How SN built it (measured)
- With GCC's builtins on (the `PAD` table of `vfprintf` is copied inline with `ldl`/`sdl`) and `-fshort-wchar`
  (the `L"..."` strings of the wide printf are 2 bytes per character): no `-fno-builtin` in its blocks.
- C compiled to `.s` with `ee-gcc -S` and assembled with an older Ps2EeAs than ours: see "Replicating SN's
  assembler for its PS2 libc".
- Left: `fftoa`/`wfftoa` (scheduling), `sn_fmod` 99.3 %, and `copy_reg` of `_eh` and `_moddi3`, which the
  distributed cc1 does not reproduce: the cc1 that built `libgcc.a` was another one.

## RealShape and RealFont (root `src/*.cpp`, `src/realshape.hpp`)

Eight new units (upstream lists them under the splitter's names as sourceless
objects of `misc`): two EA middleware libraries the tree did not have,
`librealshapez.a` and `librealfontz.a`, identified by **symbol multiset against
the PS2 `NFS.MAP`** and confirmed by `__FILE__` strings of the Xbox alpha 138
build (`creates.cpp` = `shpcreate.cpp`, `model.cpp` = `shpelement.cpp`,
`font*.cpp` = `oldfont*.cpp`); `STT_FILE` renamed `base.cpp` to `allocator.cpp`.
There is no DWARF for them: they were written from the splitter's `.s`, and
`realshape.hpp` from the disassembly and the mangled names.

### Notable files
- **All eight + `realshape.hpp`** — own library `realgraph` (flags in the table
  above: `MemObject::sAllocator` is read with `@sda21(r0)`, the target emits
  `lbzx r3,r9,r3` base first, `findkern16` walks two pointers): 83.2 % →
  4,644/4,644 B for the seven units of the time; `misc`'s flags could not
  change wholesale (`csis.cpp` would drop from 81.58 % to 48.71 %). `creates.cpp`
  defines `gTexelTypeToBpp` with the DOL's values, so three units are flagged
  NonMatching (their extracted objects use the splitter's data).

## SourceLists (`src/Speed/Indep/SourceLists/`)

The ELF's `STT_FILE` symbols are named `zGameplay.cpp`, `zLua.cpp`,
`zMain.cpp`…: the original was built as these unity units. Upstream's lists are
partial (`zMain` 8 includes, `zCamera` 6, `zPhysics` 3; `zGameplay`,
`zPlatform`, `zSpeech`, `zDynamics`, `zRender` empty). This fork rebuilds them from
the binary: file-to-unit assignment and **link order** come from crossing
`symbols/debug_lines.txt` (address → original `file:line`) with the unit ranges
of `config/GOWE69/splits.txt`, validated by regenerating lists already fixed by
hand plus seven already right (identical include by include). The first audit
found 17 of 33 lists wrong (148 files, 3,543 original functions): order decides
addresses (intra-unit calls carry no relocation), and a missing file silently
drops out of the count (zEAXSound listed 13 of its 35 files). The final lists
are plain `#include "X.cpp"` sequences like upstream's, without the old
matching scaffolding (dead "primer" functions, pool-ordering includes);
upstream's `#pragma warning(disable : 4716)` is `/wd4716` in `configure.py`.

### Notable files
- **`zMain.cpp`** — 8 → 176 includes (~150 `Generated/Events/E*.cpp`,
  `QueryDefs.cpp`, `AttribSys/codegen.cpp`…), with `VirtualMemory.cpp` and
  `InputDeviceGC.cpp` behind `EA_PLATFORM_GAMECUBE` so PS2 and MSVC compile it.
- **`zPhysics.cpp`** — 3 → 14 `.cpp`, preceded by the static tables of the
  simulation interfaces (`IMPLEMENT_LISTABLE(...)`, `ListableSet<…>::_mLists`,
  `Instanceable<…>::_mList`) in construction order: they open this unit's
  `.bss` in the original, but `debug_lines` gives them no file.
- **`zCamera.cpp`, `zFe.cpp`** — keep `#pragma implementation
  "ITrafficCenter.h"`/`"FEObjectCallback.h"`: those headers are
  `#pragma interface`, and the original defines their vtable and global
  destructor in exactly this unit (without it `_._14ITrafficCenter` is lost).
- **`zLua.cpp`** — the C++ glue (`LuaRuntime`, `LuaBindery`…) before the
  `extern "C" {}` block of Lua 5.0 files; the unit's first global names its
  `_GLOBAL_.I.<symbol>`, so definition order matters.

### Everything else
- Completed: `zPlatform` (0 → 18) and `zEcstasy` (GameCube files under
  `#ifdef EA_PLATFORM_GAMECUBE`), `zCamera` (6 → 28), `zGameplay` (0 → 17),
  `zSpeech` (0 → 17), `zFoundation` (7 → 15), `zEagl4Anim`, `zDynamics`,
  `zRender`, `zFeOverlay`, `zFEng`, `zGameModes`, `zMission`, `zDebug`; `zFe2`
  adds `FeAutoSaveIcon.cpp` only under `EA_BUILD_A124`.
- Upstream has, this fork lacks: its later `EAXEnvironment.cpp` (zEAXSound),
  A124-only `FELocalizer`/`FEMemStream` (zFEng), `FeOnlineHudSupport` (zFe2).

---

# Open fronts (diagnosed, not done)

Everything below has been diagnosed with evidence but is not in the tree yet.
It is listed so that nobody has to rediscover it.

## PS2 prototype (A124): classes whose layout still differs

The full-layout probe (`scripts/sondastabs.py`, see the tool catalogue) compares
every class described in the A124 stabs with ours: **2,520 classes match field
by field and 13 do not**. The generated events with one parameter fewer
(`EPause`, `EShowResults`, `EFadeScreenOn`, `MNotifyOnlineRaceOver`), the
`speechtune` attribute layout, the sound controllers and some thirty frontend,
world and gameplay classes are fixed with version guards (lesson 51). The 13
left are older versions of their class, where guarding members one by one would
produce a hybrid of two versions:

- **`GManager`.** The A124 has 15 icon types, not 17: there is no
  `GIcon::kType_SpeedTrapInRace` or `kType_AreaUnlock`, and `kType_Checkpoint`
  is 14. `mSpeedTrapRaceIconsShown` and `mRestartEventHash` do not exist, and
  it has members the final game dropped (`mBinVaultInSlot`, `mRaceVaultInSlot`,
  `mKeyToInstanceMap`, `mIcons`). `UpdateIconVisibility`, the `GTrigger`
  constructor, `FeMinimap` and `uiWorldMap` follow the 15 types.
- **`AIActionRace`.** No `mBottleTime`. The A124's `GetPotentialNOS` is a
  different function, `bool GetPotentialNOS(float speed, bool was_on)`, and so
  is `GetPotentialAcceleration` (a lerp between two scale tables, no drag case).
- **The rival texture streamer.** `uiRepSheetRivalStreamer` is 0x44 bytes with
  another design (texture-pack flags and three hashes instead of bins). The five
  screens that embed it (`uiRepSheetRivalBio`, `uiSafehouseRegionUnlock`,
  `uiRepSheetMain`, `uiRepSheetRival`, `Showcase`) are 8 bytes off because of
  it; `uiRepSheetMain` and `uiRepSheetRival` also lack the "defeated" images.
- **`MemoryCard`.** Three different layouts: GameCube, the A124 and the retail
  PS2 (e.g. `m_MemOp` at 0x94 and `m_pIMemcard` at 0x15c in `SLUS-21351`).
- **`Speech::GameSpeech`.** Its pending list is a heap-allocated list walked
  node by node, not a vector.
- **`FEKeyboard`, `FEMarkerSelection`, `LoadingTips`, `EAXAemsManager`.** Older
  versions with other members and logic (`LoadingTips` picks the tip screen
  from the race database, with none of the final game's test screens;
  `EAXAemsManager` differs in its loading state).

## Leftovers that break the ground rules: removed

Earlier phases left matching devices in the tree: a dead "primer" function, a scheduling pragma, a dummy
`double` for section alignment, joined literals, invented locals and statics, a `volatile` cast and a
`__typeof__` view that steered the optimiser, a declaration by mangled name and a GNU statement expression.
All were removed on 2026-09-18 and their loose ends closed, measuring the five builds before and after. What
it cost on GameCube (the DOL stays byte-identical: `sndfxbus.cpp`, whose 8-byte `.rodata` alignment only the
dummy `double` reproduced, is now NonMatching, and the link takes the original object):

| function (unit) | before | after | what was removed |
|---|---:|---:|---|
| `elRotateLightContext` (zEcstasy) | 100 % | 73.14 % | a dead `static inline` that copied an `eDynamicLightContext` |
| `CarLoader::DefragmentPool` (zWorld) | 99.27 % | 92.47 % | three helper locals the DWARF does not list |
| `PATHI_loadbankdata` (path) | 100 % | 87.78 % | a `*(volatile int *)&fileop` store |
| `PathTrackSnd::PathTrackSnd` (path) | 100 % | 88.57 % | the `PATH_AGG(x)` aggregate view |
| `PathTrackSndStream::CheckStatus` (path) | 100 % | 98.81 % | the same |
| `Sim::Entity::Entity` (zSim) | 100 % | 99.86 % | a GNU statement expression |
| `__tcf_4` (zWorld2) | 100 % | 0 % | an invented function-local `static` in dead code |

GameCube goes from 18,298 to 18,292 functions (97.14 to 97.07 %). On PS2 the same `path` functions lose a
little and `DefragmentPool` gains (A124 64.42 to 70.11 %); on Xbox 360 and PC nothing is lost.

Removed at no cost: the `#pragma scheduling off/reset` around `CarRenderConn::UpdateParts` (SN's GCC ignores
it); the joined literals of `bMemory.cpp` and `SpeedScript.cpp`; `ClassKey__Q36Attrib3Gen8gameplay()` declared
by its mangled name (constructing `Attrib::Gen::gameplay` by key gives the same out-of-line call); the bodies of
`bOldATan` and `bFastATan`, stripped functions whose DWARF records contradicted them; and the undefined
`e_OSBusClock` (the SDK's `OSTicksToMilliseconds` reads `__OSBusClock` exactly as the original). Closed at the
same time: the 32 globals zMisc used and nobody defined, now defined with the DOL's values in the original
`.data` order; and upstream's portable `VU0_*` functions in `UVectorMath.cpp`, which a merge had nested inside
the GameCube block (Xbox 360 and PC now compile `VU0_MATRIX4_mult`, at 93 and 99 %).

Kept on purpose, because they follow the evidence: `bDistBetween(const bVector4 *, ...)`, `bGetPolyArea`,
`bToPolar` and `bLength(const bQuaternion *)`, stripped functions whose bodies follow their DWARF records; and
`bSinTable[513] = {0}`, a table the linker dropped, written with its DWARF name and type and a zero initialiser.

Open: what the original used, before `elRotateLightContext`, that synthesized `eDynamicLightContext::operator=`
early enough to be inlined there.

**Measurement note.** The same batch showed that `scripts/pctall.py` was not fully ignoring the `Matching`
flag: objdiff scores a unit marked `complete` at 100 % without comparing it, and our flags are GameCube facts.
On PS2 that counted 124 (A124) and 261 (SLUS) functions that do not match, mostly in `snd`, `realcore`, `spch`,
`path` and `rcmp`; two of them do not even exist in our `sndfxbus.o`. `pctall.py` now marks every unit
incomplete, as upstream's all-NonMatching configuration does, and the figures in this document are the
corrected ones.

## FPU asm: removed, proven to be EA's, restored (2026-09-23)

Every FPU `asm` in the tree was taken out on 2026-09-23 and replaced with the C form that loses least, measured
over every unit on a frozen tree: `bSqrt`'s `fmuls`/`fnmsubs` on GameCube, the PS2 `VU0_floatmin`/`VU0_floatmax`/
`VU0_fabs` (`min.s`, `max.s`, `abs.s`), `FloatToInt` (`cvt.w.s` in `$f4`), `VU0_FloatToInt` (`$f8` and two `nop`), the
`nop` in `bCopy(bMatrix4)` and the loose `mfc1` of `bScale`/`bScaleAdd`. What it cost:

| build | functions lost | bytes | where |
|---|---:|---:|---|
| GameCube | 86 | 77,856 | every function that expands `bSqrt`: zEAXSound, zCamera, zEAXSound2, zWorld, zEcstasy... |
| A124 | 168 | 119,172 | zPhysicsBehaviors 77, zAI 25, zPhysics 16, zCamera 9... |
| SLUS | 172 | 114,396 | zPhysicsBehaviors 79, zAI 32, zPhysics 16, zCamera 9... |

A dedicated round then tried to win them back with C that EA could have written and proved it cannot be done
(lesson 90), so the maintainer restored exactly the `asm` that the binary proves EA wrote. The loose `mfc1` of
`bScale`/`bScaleAdd`, which was ours, stays in C through `bVU0Float`. The PS2 `bAbs(float)` had the same fingerprint
as `VU0_fabs` and was measured the same way: in C it loses 21 A124 and 18 SLUS functions, as EA's `abs.s` it loses
none, so it takes the `asm` too. `bMin` and `bMax` of float gain 4 and lose 10-11 per build with `min.s`/`max.s`,
so they stay in C.

The C form of each of EA's `asm` is kept next to it, for the day a source form is found that gives the same code:

| `asm` | where | C form | cost of the C form |
|---|---|---|---|
| `fmuls`/`fnmsubs` of `bSqrt` (GameCube) | `bWare/Inc/bMath.hpp` | `t1 = y0 * half; t2 = -(x * t0 - one);` (four orders tried) | 86 GameCube functions |
| `min.s`/`max.s`/`abs.s` of `VU0_floatmin`/`VU0_floatmax`/`VU0_fabs` (PS2) | `Libs/Support/Utility/UVectorMath.h` | `a > b ? b : a`, `a < b ? b : a`, `fabsf(a)` | about 148 A124 and 152 SLUS functions |
| `abs.s` of `bAbs(float)` (PS2) | `bWare/Inc/bMath.hpp` | `fabsf(a)` | 21 A124, 18 SLUS |
| `cvt.w.s` in `$f4` of `FloatToInt` (PS2) | `Src/EAGL4Anim/AnimUtil.h` | `static_cast<int>(f)` | with the next two rows, 19 A124 and 20 SLUS |
| `cvt.w.s` in `$f8` and two `nop` of `VU0_FloatToInt` (PS2) | `Libs/Support/Utility/UVectorMath.h` | `static_cast<int>(f)` | (above) |
| the `nop` of `bCopy(bMatrix4)` (PS2) | `bWare/Inc/bMath.hpp` | none: C cannot emit a lone `nop` | (above) |
| `min.s`/`max.s` of `bMin`/`bMax(float)`, not volatile (PS2) | `bWare/Inc/bMath.hpp` | `a > b ? b : a`, `a < b ? b : a` | 39 A124, 43 SLUS |
| `cvt.w.s` + `mfc1` of the float-to-int inline, lines 72-78 (PS2; name not recorded) | `bWare/Inc/bMath.hpp` | `static_cast<int>(x)` | 26 A124, 27 SLUS |
| `sq $0` for 128-bit zeros in vp6 (PS2) | `vp6/1.0.6/source/decode/cmn/decodembs.inl` | `memset` | 1 per PS2 build |
| `break 0, 6` of `DEBUG_break` (PS2) | `realcore/6.24.00/source/debug/cmn/debugger.cpp` | none | 1 per PS2 build |

## Other builds

- **Xbox 360 and PC are limited by names, not only by code.** Their symbol
  files name a small fraction of the functions. `tools/nombra_por_codigo.py`
  names an original function when our compiled object has a byte-identical,
  unique function in the same order; re-running it after each batch of new code
  keeps adding names (it took X360 from 1.33 % to 1.85 % and PC from 6.75 % to
  9.16 % the first time).
- **GameCube** is at 18,292 of 18,432 functions. The 140 left (116 kB) are
  mostly near-misses: 74 of them (95 kB) are already above 97 % similarity, and
  the largest single one is `CarRenderInfo::Render` (11 kB at 99.9 %). Other
  large ones are SN's own `vfprintf` rewrite (no public source), VP6's
  `VP6_DecodeBlock`, and `zCamera`'s static initialiser.
