# Ghidra for the GameCube build

Stock Ghidra reads little of `orig/GOWE69/NFSMWRELEASE.ELF`. Two patches change that, both measured on
this project with Ghidra 12.1.2:

| | stock | patched |
|---|---:|---:|
| functions with their DWARF signature (`SourceType.IMPORTED`) | 86 | **13,824** of 13,950 importable (99.1 %) |
| structures | 11 | **4,076** (3,802 with fields) |
| enums | 2 | **2,872** |
| paired-single instructions decoded (`psq_l`, `psq_st`, `ps_*`) | 0 (disassembly stops at the first one) | **8,627 of 8,627** |

The files here:

| file | what it is |
|---|---|
| `ghidra-dwarf1-gcc-ext.patch` | patch for the ghidra-dwarf1 extension so that it reads GCC 2.9x's DWARF-1 |
| `gekko_750cl_ghidra12.py` | adds the Gekko/Broadway language (PowerPC 750CL with paired singles) to Ghidra 12.1.2 |
| `SetGqr.java` | pre-script for the import: GQR0 = 0, so that `psq_l`/`psq_st` decompile cleanly |
| `Dwarf1Run.java` | runs the DWARF-1 analyzer headless and writes a census before and after |
| `DecompilarFunciones.java`, `InfoPrograma.java` | used by `tools/ghidra_decomp.py` to decompile functions from the command line |

## 1. DWARF-1: types and signatures

The ELF carries DWARF-1 (`.debug`, 316 compile units). The
[ghidra-dwarf1](https://github.com/emoose/ghidra-dwarf1) extension reads it, but aborts at the first entry:
GCC 2.9x emits six attributes of its own, 0x8000 to 0x8050 (`AT_sf_names`, `AT_src_info`, `AT_mac_info`,
`AT_src_coords`, `AT_body_begin`, `AT_body_end`; `gcc/dwarf.h:184-189`), outside DWARF-1's user range
0x2000-0x3ff0. The patch declares them and makes an unknown attribute or tag non-fatal.

```sh
git clone https://github.com/emoose/ghidra-dwarf1
cd ghidra-dwarf1
git checkout 5b0666c          # "11.3 fixups, forward-port ElfSectionProvider from old ghidra"
git apply <nfsmw>/tools/ghidra/ghidra-dwarf1-gcc-ext.patch
GHIDRA_INSTALL_DIR=<ghidra> gradle buildExtension
unzip dist/*.zip -d <ghidra>/Ghidra/Extensions/
```

That commit builds unchanged against Ghidra 11.4 and 12.1.2. The analyzer, `DWARF1`, is not enabled by
default: in the GUI run it with *Analysis > One Shot > DWARF1*; headless, use `Dwarf1Run.java` (step 3).

## 2. Paired singles: the Gekko/Broadway language

Ghidra 12.1.2 has no Gekko/Broadway variant. Ghidra's own sources had one at commit
[`e2f20dab`](https://github.com/NationalSecurityAgency/ghidra/tree/e2f20dab4c26180e6358f40cd7f279adcae5f166/Ghidra/Processors/PowerPC/data/languages)
(April 2025; the 11.4 DEV builds of that date ship it), and it is no longer on master.

1. Copy `750cl.sinc`, `ppc_gekko_broadway.slaspec`, `ppc_gekko_broadway.pspec` and
   `ppc_gekko_broadway.cspec`, unchanged, from that commit into
   `<ghidra>/Ghidra/Processors/PowerPC/data/languages/`.
2. `python tools/ghidra/gekko_750cl_ghidra12.py <ghidra>`: it adds the language to `ppc.ldefs` and ports the
   750CL parts of that commit's `ppc_common.sinc` into 12.1.2's, each behind `@ifdef 750CL`, so that the
   other PowerPC languages compile exactly as before. It keeps `.orig` copies and is idempotent.
3. `<ghidra>/support/sleigh <ghidra>/Ghidra/Processors/PowerPC/data/languages/ppc_gekko_broadway.slaspec`

On the stock 12.1.2 files, the script's output is byte for byte the `ppc_common.sinc` and `ppc.ldefs` used
for the figures above. What was verified is that every paired single decodes; the semantics of the
s8/u8/s16/u16 quantization were not checked.

## 3. Import

```sh
analyzeHeadless <projdir> NFSMW -import orig/GOWE69/NFSMWRELEASE.ELF \
    -processor PowerPC:BE:32:Gekko_Broadway -scriptPath tools/ghidra -preScript SetGqr.java
analyzeHeadless <projdir> NFSMW -process NFSMWRELEASE.ELF -noanalysis \
    -scriptPath tools/ghidra -postScript Dwarf1Run.java dwarf1.txt
```

The first run takes about five minutes with full auto-analysis (which also resolves the `switch` jump
tables); the second about 35 seconds. `SetGqr.java` must run *before* auto-analysis. It sets GQR0 only:
NFSMW writes GQR0 five times, always with 0, while GQR1-GQR7 carry real quantization. With GQR0 fixed,
Ghidra reports the branches for other quantization types as "Removing unreachable block"; that message is
expected in functions with paired singles.

## 4. Decompiling from the command line

`python tools/ghidra_decomp.py <function|0xaddress>...` decompiles read-only from a saved project
(`ghidra_project/NFSMW` by default); `--info` summarises the program. It needs the Ghidra version that saved
the project and a JDK 21, and fails if the GUI has the project open.
