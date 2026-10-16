# SN's GameCube compiler, built from its GPL source

This is an **option**. The normal build uses the official compiler binaries in `build/compilers/` (fetched by
`configure.py`/`ninja`). With this directory, anyone with Linux or WSL can build the same compilers from SN
Systems' GPL source, with the project's two binary patches written as readable source patches, and compile the
GameCube build with them. Nothing is downloaded: the source is the copy kept in `orig/prodg/`.

| toolchain | stands in for | built from | `cc1.exe` SHA-1 | `cc1plus.exe` SHA-1 |
|---|---|---|---|---|
| `build/compilers/ProDG/3.9.3-src` | `ProDG/3.9.3` (SN BUILD v1.76, patched for `$LC`) | v1.79 + `dollar-lc` | `084152a6aa3f0e20598625dc48f19ecb1b850737` | `3c15d20b5cfccdc7e6023a0e1cfed50b797c3411` |
| `build/compilers/ProDG/3.9.3-v183src` | `ProDG/3.9.3-v183r` (the v1.83 replica used for `path`) | v1.79 + `keating-alias` + `dollar-lc` | `3537fe9fde7b1fb0afc664ce2c3488e886051914` | `afd5ca58508f40608fba5f080c1f47c4fdd5aa6d` |

Each toolchain is a copy of `ProDG/3.9.3` (driver `ngccc.exe`, preprocessor, assembler, linker) in which only
`cc1.exe` and `cc1plus.exe` are replaced. The SHA-1s are those of a build on Ubuntu 24.04 (gcc 13.3, mingw-w64
GCC 13-win32); the build is reproducible, so the same host compilers give the same bytes.

## Use

1. Run `python configure.py` and `ninja` once, so that `build/compilers/ProDG/3.9.3` exists.
2. SN's source must be in `orig/prodg/`: either extracted (`orig/prodg/NGC_GNU_SRC/NGC/gcc/...`) or as
   `orig/prodg/ProDGforNGCv393_Source_Code.zip` (the script extracts what it needs).
3. Build the two toolchains (about 3 minutes on 4 cores):
   - Linux: `bash tools/sn-gcc/build.sh`
   - Windows, from the repository root: `wsl bash tools/sn-gcc/build.sh`

   Packages (Debian/Ubuntu): `make patch python3 gcc-multilib gcc-mingw-w64-i686`.
   Options: `--compilers DIR`, `--source DIR`, `--work DIR` (default `~/.cache/nfsmw-sn-gcc`; keep it on a
   Linux file system), `--jobs N`, `--keep`, and the names of the toolchains to build (default: both).
   `bash tools/sn-gcc/build.sh --help` prints them.
4. Configure with the option and build:
   `python configure.py --gc-toolchain-source` (alias `--gc-toolchain-fuente`), then `ninja`.
   The environment variable `NFSMW_GC_TOOLCHAIN_SOURCE=1` does the same; it is the way to keep the option in
   `scripts/pctall.py` and `scripts/medir.py`, which call `configure.py` without arguments.
   Without the option (and without the variable) nothing changes. Switching either way recompiles every
   GameCube unit built with ProDG, because the command line changes.

If the option is given and a toolchain is missing, `configure.py` stops and says so. The option only affects
GameCube; the PS2, Xbox 360 and PC builds ignore it.

## Where it comes from

- **The source.** SN Systems' GPL source release for ProDG for GameCube 3.9.3,
  `ProDGforNGCv393_Source_Code.zip` (SHA-1 `01addd9bf5acbad2bf825b8ddf0a4f32db692742`), file
  `GC source code/NGC_GNU_SRC.zip`: GCC 2.95.2 with part of 2.95.3 and SN's port, "2.95.3 SN BUILD v1.79 for
  Nintendo Gamecube" (`gcc/sn_version.h`). SN's `GnuSrcNGC.txt` offers it on request, as the GPL requires; the
  same zip (same SHA-1) is on archive.org, item `sn_sys_consoles_2`, `NGC/ProDGforNGCv393_Source_Code.zip`.
  It is **not** in this repository: `orig/` is ignored by git, and the script reads it from there.
- **The recipe.** [emoose/re4](https://github.com/emoose/re4), `tools/sn-gcc` (CC0 1.0): a native Linux build of
  the same source with `gcc -m32 -O2 -std=gnu89 -fpermissive -fcommon -fno-strict-aliasing -fwrapv`, the
  pregenerated parsers used as they are (no bison), CRLF converted to LF. `Makefile.linux` is its Makefile (plus a
  `generated` target and the second `varasm.c`), and `patches/linux-host.patch` is its patch, unchanged. Its other
  patch, `shipped-build-temp-flags.patch` (stack temporaries in `function.c`), is **not** used: EA's v1.76 does not
  behave like the compiler RE4 was built with, and it breaks two functions that match with the official compiler here
  (`FEScript::FEScript(FEScript &, bool)` in zFEng and `RealInput::EventQueue::AddEvent` in realcore).

## How the build works (`build.sh`)

1. `gcc/` and `include/` are copied once to the work directory, CRLF to LF (the RTL reader rejects `\r` in
   `rs6000.md`), without the CHILL front end (`gcc/ch`).
2. **Generated files, on Linux.** With `linux-host.patch` (a Linux host configuration), `gcc -m32` builds the
   generator programs and runs them on `config/rs6000/rs6000.md`: `insn-*.c`, `genrtl.*`, `tree-check.h`. They
   do not depend on the host (HOST_WIDE_INT is 32 bits on both), so no Windows program has to run. The headers
   that some generators include are made first, one at a time: RE4's recipe has a race there under `-j`.
3. **The compilers, for Windows.** mingw-w64 (i686) compiles `cc1.exe` and `cc1plus.exe` with SN's **own** host
   configuration (`config.h`, `hconfig.h`, `auto-host.h` and `SNMisc.c` exactly as SN shipped them for
   MSVC/WinNT; only the two build fixes of `linux-host.patch` remain) and the toolchain's patches. Like SN's
   `vsgcc.dsp`, `rs6000.c` is compiled with `-DSN_CPP_BUILD` for cc1plus; `varasm.c` is compiled twice so that
   only cc1plus gets `-DSN_DOLLAR_LC`. `-Wl,--no-insert-timestamp` makes the `.exe` reproducible.
4. **Install.** A copy of `ProDG/3.9.3` with the two compilers replaced, and a `SOURCE.txt` that records the
   patches, the host compiler and the SHA-1s. The script compares the SHA-1s with the reference build above.

Floating-point constant folding in this GCC is emulated (`REAL_ARITHMETIC` in `rs6000.h`), so the host's FPU
plays no part. The only difference the compilers leave in an object is their version string ("SN BUILD v1.79"
instead of "v1.76") in the DWARF producer attribute, in `.debug`.

## The patches

- **`linux-host.patch`** (emoose/re4, CC0): the i386 GNU/Linux host configuration (used here only for the
  generator programs), `host_is_japanese()` without the Windows API, and two fixes that any modern host compiler
  needs: `cp/decl.c` took the address of a block-local variable and used it after the block (cc1plus crashed on
  every virtual destructor), and `toplev.c` uses MSVC's `_MAX_PATH`.
- **`dollar-lc.patch`**: the source form of upstream's binary patch `tools/patch-toolchain.py` (profile
  `prodg393-gc-cc1plus`), which redirects six calls and three pushes of the official `cc1plus.exe` to helper code
  in a cave. Constant-pool labels become local symbols `$LCn` with `.type` and `.size`, so that every literal is a
  symbol of the object (objdiff pairs them with the original's data labels; `config/GOWE69/keep.lst` names
  them). Only cc1plus has it, as in the binary patch: the official `cc1.exe` is unpatched (checked at its six
  call sites).
- **`keating-alias.patch`**: GCC's change of 2000-06-23 by Geoffrey Keating, "alias.c
  (fixed_scalar_and_varying_struct_p): Don't examine struct vs. scalar-ness when -fno-strict-aliasing". It went
  into GCC 3.0, not into 2.95. SN's v1.79 source still applies the rule; the game's `path` library was built with
  SN BUILD v1.83 (GameCube DWARF), and this is the change that separates the two for this code.
  `scripts/replica_v183.py` writes the same thing into copies of the official binaries (three bytes: the function
  returns NULL, which is what the change amounts to with `-fno-strict-aliasing`, the only setting used here).

**Why v1.79 stands in for v1.76.** The official `cc1plus.exe` (v1.76) was linked on 2003-03-07 (PE time stamp).
What SN changed after that and up to v1.79 (2003-05-16, per the SourceSafe headers of each file) does not touch
generated code: `-mregnames`, a `vec_new` "peephole" that SN added and removed, and DWARF for nested templates.
The measurements below agree.

## Validation

- **Assembly** (round 14): every unit preprocessed once with the official `cpp`, then compiled by both `cc1`s,
  `.s` compared without the version string. v1.79 against SN's v1.76 without the `$LC` patch: 409 of 409 v1.76
  units of GameCube identical (28,163 symbols). v1.79 + `dollar-lc` against the project's `cc1plus.exe`: 409 of
  409. v1.79 + `keating-alias` (+ `dollar-lc`) against `3.9.3-v183r`: the 27 units of `path` and `rcmp` identical.
- **Whole build** (round 15, GOWE69, with and without `--gc-toolchain-source`, one after the other under one lock
  of `scripts/medir.py`): the objdiff report is the same function by function (18,387 of 18,432 functions), the
  DOL keeps SHA-1 `9619ba57c9919f95f7f2ac951a2166a3517f91e3`, and the 436 objects compiled with ProDG are
  byte-identical apart from the version string, except one whose `_GLOBAL_.N...` symbol has a suffix that changes
  with every compilation whatever the compiler (`rcmp_mad_codec_chunk_types.o`).
- **Speed**: the mingw build is faster than SN's MSVC build of 2003. One unit at a time, best of three: `zAI`
  12.7 s against 18.1 s, `zPhysics` 9.4 against 12.9, `zSim` 9.1 against 14.2, `zEAXSound2` 9.1 against 16.0;
  66 % of the time in all. Details in `docs/BEYOND_UPSTREAM.md`, "Building SN's compiler from its GPL source".

## Native Linux compilers

`make all` in `WORK/gen` (left by `build.sh --keep`; its Makefile is `Makefile.linux`) builds Linux
`cc1`/`cc1plus`: handy to debug or instrument the compiler, but the project does not use them. `ngccc.exe` is a
Windows program, and the Linux `cc1` does not read CRLF sources like SN's: a string literal split by a real
newline inside a CRLF file keeps the `\r` on Linux, where Windows' text mode drops it (one unit, `zPlatform.cpp`,
differs; 408 of 409). Running the mingw `.exe` under wibo (the project's Linux/macOS path) has not been tested.
