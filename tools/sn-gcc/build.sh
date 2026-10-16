#!/usr/bin/env bash
# Build SN Systems' GameCube compiler ("GCC 2.95.3 SN BUILD v1.79") from its GPL source and install it as
# two toolchains next to the official ones:
#
#   build/compilers/ProDG/3.9.3-src      v1.79 + dollar-lc + frsqrte-fres + fnmsubs-minus + bsqrt-order      stands in for ProDG/3.9.3
#   build/compilers/ProDG/3.9.3-v183src  v1.79 + keating-alias + dollar-lc + frsqrte-fres + fnmsubs-minus + bsqrt-order  stands in for ProDG/3.9.3-v183r
#
# Each one is a copy of build/compilers/ProDG/3.9.3 (driver, preprocessor, assembler, linker) whose cc1.exe
# and cc1plus.exe are replaced by the ones built here.  Nothing is downloaded: the source is SN's GPL release
# in orig/prodg/ (see README.md).  Afterwards, `python configure.py --gc-toolchain-source` (or the variable
# NFSMW_GC_TOOLCHAIN_SOURCE=1) makes the GameCube build use them.
#
# Runs on Linux or WSL.  Needs: bash, make, patch, python3, gcc with -m32 (Debian/Ubuntu: gcc-multilib) and
# the 32-bit mingw-w64 C compiler (gcc-mingw-w64-i686).  From Windows:  wsl bash tools/sn-gcc/build.sh
#
# Usage: tools/sn-gcc/build.sh [options] [TOOLCHAIN...]
#   TOOLCHAIN          3.9.3-src and/or 3.9.3-v183src (default: both)
#   --compilers DIR    compilers directory (default: build/compilers of this repository)
#   --source DIR       SN's source: the NGC/ directory that holds gcc/ and include/
#                      (default: orig/prodg/NGC_GNU_SRC/NGC, else extracted from
#                      orig/prodg/ProDGforNGCv393_Source_Code.zip)
#   --work DIR         build directory (default: ~/.cache/nfsmw-sn-gcc; keep it on a Linux file system,
#                      /mnt/c is several times slower); what the script puts there is removed at the end
#                      unless --keep
#   --jobs N           parallel jobs (default: nproc)
#   --keep             keep the build directory (objects, logs, and the Linux generator tree)
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
COMPILERS="$ROOT/build/compilers"
DROP=""
WORK="${XDG_CACHE_HOME:-$HOME/.cache}/nfsmw-sn-gcc"
JOBS="$(nproc 2>/dev/null || echo 4)"
KEEP=0
WANT=()

die() { echo "build.sh: $*" >&2; exit 1; }

while [ $# -gt 0 ]; do
    case "$1" in
        --compilers) COMPILERS="$2"; shift 2 ;;
        --source) DROP="$2"; shift 2 ;;
        --work) WORK="$2"; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --keep) KEEP=1; shift ;;
        -h|--help) sed -n '2,/^set -euo/p' "$0" | sed '$d; s/^# \{0,1\}//'; exit 0 ;;
        3.9.3-src|3.9.3-v183src) WANT+=("$1"); shift ;;
        *) die "unknown argument '$1' (try --help)" ;;
    esac
done
[ ${#WANT[@]} -gt 0 ] || WANT=(3.9.3-src 3.9.3-v183src)

# The patches of each toolchain, after linux-host (applied to every tree).
# frsqrte-fres (PoC, agent/compiler-r1): scalar __builtin_frsqrte/__builtin_fres;
# fnmsubs-minus (F1, agent/compiler-f1): SFmode (minus c (mult a b)) -> fnmsubs,
# the -ffast-math folded form of -(a*b-c); it touches rs6000.md, so the
# generator run takes the union of all patches.
# bsqrt-order (F2, agent/compiler-f2): rs6000.c scheduling hooks so the first
# scheduler emits t1 (y0*half) before t2 (one-x*t0) in the Newton step;
# requires the named F1 pattern; compiler-proper only (no generator impact
# beyond the F1 rename, already in the union).
patches_of() {
    case "$1" in
        3.9.3-src) echo "dollar-lc frsqrte-fres fnmsubs-minus bsqrt-order" ;;
        3.9.3-v183src) echo "keating-alias dollar-lc frsqrte-fres fnmsubs-minus bsqrt-order" ;;
    esac
}

# SHA-1 of cc1.exe and cc1plus.exe as built on Ubuntu 24.04 (mingw-w64 GCC 13-win32, gcc 13.3): the build is
# reproducible, so the same host compilers give the same bytes.  Another host compiler gives other bytes; that
# is expected, and the output must then be validated as README.md describes.
reference_of() {
    case "$1" in
        3.9.3-src) echo "084152a6aa3f0e20598625dc48f19ecb1b850737 3c15d20b5cfccdc7e6023a0e1cfed50b797c3411" ;;
        3.9.3-v183src) echo "3537fe9fde7b1fb0afc664ce2c3488e886051914 afd5ca58508f40608fba5f080c1f47c4fdd5aa6d" ;;
    esac
}

MINGW_CC="${MINGW_CC:-i686-w64-mingw32-gcc}"
missing=()
for t in make patch python3 gcc sha1sum "$MINGW_CC"; do
    command -v "$t" >/dev/null 2>&1 || missing+=("$t")
done
[ ${#missing[@]} -eq 0 ] || die "missing tools: ${missing[*]} (Debian/Ubuntu: apt install make patch python3 gcc-multilib gcc-mingw-w64-i686)"

BASE="$COMPILERS/ProDG/3.9.3"
[ -f "$BASE/ngccc.exe" ] && [ -f "$BASE/cc1plus.exe" ] || \
    die "$BASE is missing: run 'python configure.py' and 'ninja' once first (they fetch the official compilers)"

START=$SECONDS
mkdir -p "$WORK"
WORK="$(cd "$WORK" && pwd)"

printf 'int main(void){return 0;}\n' > "$WORK/m32.c"
gcc -m32 -o "$WORK/m32" "$WORK/m32.c" 2>/dev/null && "$WORK/m32" || \
    die "gcc -m32 cannot build and run a 32-bit program (Debian/Ubuntu: apt install gcc-multilib)"

# ---- SN's source ----
if [ -z "$DROP" ]; then
    DROP="$ROOT/orig/prodg/NGC_GNU_SRC/NGC"
    if [ ! -d "$DROP/gcc/cp" ]; then
        ZIP="$ROOT/orig/prodg/ProDGforNGCv393_Source_Code.zip"
        [ -f "$ZIP" ] || die "SN's source is not in orig/prodg/ (NGC_GNU_SRC/NGC or ProDGforNGCv393_Source_Code.zip)"
        echo "extracting NGC_GNU_SRC.zip from $(basename "$ZIP")"
        rm -rf "$WORK/drop"
        python3 - "$ZIP" "$WORK/drop" <<'EOF'
import io, sys, zipfile
outer = zipfile.ZipFile(sys.argv[1])
inner = [n for n in outer.namelist() if n.endswith('/NGC_GNU_SRC.zip')]
if len(inner) != 1:
    sys.exit('NGC_GNU_SRC.zip not found in ' + sys.argv[1])
zipfile.ZipFile(io.BytesIO(outer.read(inner[0]))).extractall(sys.argv[2])
EOF
        DROP="$WORK/drop/NGC_GNU_SRC/NGC"
    fi
fi
[ -d "$DROP/gcc/cp" ] && [ -d "$DROP/include" ] || die "$DROP does not look like SN's NGC/ source directory"
VER="$(tr -d '\r' < "$DROP/gcc/sn_version.h" | sed -n 's/^#define M[AI][JN]OR_VERSION_S "\(.*\)"/\1/p' | paste -sd.)"
echo "source: $DROP (SN BUILD v$VER)"
[ "$VER" = "1.79" ] || echo "warning: this recipe was validated on SN BUILD v1.79, the source says v$VER"

# One clean copy of gcc/ and include/ on the work file system: CRLF -> LF (the rtl reader rejects '\r' in
# rs6000.md) and without the CHILL front end.  prep DIR PATCH... copies it and applies linux-host and PATCH...
echo "copying the source to $WORK/base"
rm -rf "$WORK/base"; mkdir -p "$WORK/base/src"
cp -r "$DROP/gcc" "$DROP/include" "$WORK/base/src/"
rm -rf "$WORK/base/src/gcc/ch"
chmod -R u+w "$WORK/base/src"
find "$WORK/base/src" -type f \( -name '*.c' -o -name '*.h' -o -name '*.md' -o -name '*.def' -o -name '*.y' \
    -o -name '*.cc' -o -name '*.cpp' -o -name '*.in' \) -print0 | xargs -0 sed -i 's/\r$//'

prep() {
    local d="$1"; shift
    rm -rf "$d"; mkdir -p "$d"
    cp -r "$WORK/base/src" "$d/"
    local p
    for p in linux-host "$@"; do
        (cd "$d/src" && patch -s -p1 < "$HERE/patches/$p.patch") || die "patch $p does not apply"
    done
}

# ---- generated files (insn-*.c, genrtl.*, tree-check.h), with the Linux host configuration ----
# The generator run takes the union of the toolchains' patches: frsqrte-fres and
# fnmsubs-minus touch rs6000.md (F2 renames that pattern but adds no new .md),
# so insn-codes.h et al. must see them.  Patches
# confined to the compiler proper (dollar-lc: varasm.c; keating-alias: alias.c;
# bsqrt-order: rs6000.c scheduling hooks) feed no generator.
echo "generator programs (Linux, gcc -m32)"
GENPATCHES="$(for tc in "${WANT[@]}"; do patches_of "$tc"; done | tr ' ' '\n' | sort -u | tr '\n' ' ')"
prep "$WORK/gen" $GENPATCHES
cp "$HERE/Makefile.linux" "$WORK/gen/Makefile"
( cd "$WORK/gen" && make -s generated-first > build.log 2>&1 && make -s -j"$JOBS" generated >> build.log 2>&1 ) || \
    { tail -30 "$WORK/gen/build.log"; die "the generator programs failed (log: $WORK/gen/build.log)"; }
GENFILES="tree-check.h genrtl.h genrtl.c insn-attr.h insn-codes.h insn-config.h insn-flags.h insn-peep.c
 insn-opinit.c insn-recog.c insn-extract.c insn-output.c insn-emit.c insn-attrtab.c"

HOSTCC="$("$MINGW_CC" --version | head -1)"
for tc in "${WANT[@]}"; do
    read -r -a pts <<< "$(patches_of "$tc")"
    d="$WORK/$tc"
    echo "$tc: cc1.exe and cc1plus.exe (mingw-w64), patches: linux-host ${pts[*]}"
    prep "$d" "${pts[@]}"
    # The Windows build keeps SN's own host configuration (MSVC/WinNT), untouched.
    for f in config.h hconfig.h auto-host.h SNMisc.c; do
        cp "$WORK/base/src/gcc/$f" "$d/src/gcc/$f"
    done
    mkdir -p "$d/gen"
    for f in $GENFILES; do cp "$WORK/gen/obj/$f" "$d/gen/"; done
    cp "$HERE/Makefile.mingw" "$d/Makefile"
    ( cd "$d" && make -s -j"$JOBS" MINGW_CC="$MINGW_CC" all > build.log 2>&1 ) || \
        { tail -30 "$d/build.log"; die "$tc failed (log: $d/build.log)"; }
    s1="$(sha1sum "$d/cc1.exe" | cut -d' ' -f1)"
    s2="$(sha1sum "$d/cc1plus.exe" | cut -d' ' -f1)"
    echo "  cc1.exe     $s1"
    echo "  cc1plus.exe $s2"
    if [ "$s1 $s2" = "$(reference_of "$tc")" ]; then
        echo "  identical to the reference build (validated against the official compilers)"
    else
        echo "  NOT identical to the reference build (other host compilers?): validate it before trusting it"
    fi

    # Install: a copy of the official toolchain with the two compilers replaced.
    dst="$COMPILERS/ProDG/$tc"
    [ ! -L "$dst" ] || die "$dst is a link: not touching it"
    tmp="$dst.tmp.$$"
    rm -rf "$tmp"
    cp -r "$BASE" "$tmp"
    cp "$d/cc1.exe" "$d/cc1plus.exe" "$tmp/"
    cat > "$tmp/SOURCE.txt" <<EOF
Built by tools/sn-gcc/build.sh from SN's GPL source (GCC 2.95.3 SN BUILD v$VER for Nintendo Gamecube).
Patches: linux-host (build fixes; SN's own MSVC/WinNT host configuration is kept), ${pts[*]}.
Host compiler: $HOSTCC
cc1.exe     $s1
cc1plus.exe $s2
Every other file is a copy of ProDG/3.9.3.
EOF
    rm -rf "$dst"
    mv "$tmp" "$dst"
    echo "  installed in $dst"
done

if [ "$KEEP" = 0 ]; then
    # only what this script made: --work may name a directory that holds other things
    rm -rf "$WORK/base" "$WORK/gen" "$WORK/drop" "$WORK/m32" "$WORK/m32.c"
    for tc in "${WANT[@]}"; do rm -rf "${WORK:?}/$tc"; done
    rmdir "$WORK" 2>/dev/null || true
else
    echo "build directory kept: $WORK"
fi
echo "done in $((SECONDS - START)) s.  Use them with: python configure.py --gc-toolchain-source"
