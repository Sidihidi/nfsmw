#!/usr/bin/env python3
"""Assemble a PS2 `.S` file the way SN Systems built its libc: preprocess it with EE-GCC
(`ee-gcc -E`) and assemble the result with SN's assembler, Ps2EeAs.

Usage: ee_as_S.py [--wrapper <program>] <ee-gcc> <Ps2EeAs> -- <flags...> -o <out> <source.S>

Flags starting with `-Wa,` go to the assembler (comma-separated, as with gcc); the rest go to the
preprocessor. The preprocessed file is written next to the object (<out>.pp.s). `--wrapper` runs
both Windows programs through it (wibo/wine outside Windows).

Why Ps2EeAs and not the GNU assembler of the toolchain: the original objects expand `dli` the way
Ps2EeAs does (ori/dsll chains); the GNU as of 2.9-ee-991111 uses lui/ori and does not match.
"""

import os
import subprocess
import sys


def main() -> int:
    argv = sys.argv[1:]
    wrapper = []
    if argv[:1] == ["--wrapper"] and len(argv) > 1:
        wrapper = [argv[1]]
        argv = argv[2:]
    if "--" not in argv or argv.index("--") != 2:
        print(__doc__, file=sys.stderr)
        return 2
    ee_gcc, ps2eeas = argv[0], argv[1]
    args = argv[3:]
    if "-o" not in args or len(args) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    out_index = args.index("-o")
    output = args[out_index + 1]
    source = args[-1]
    flags = [a for i, a in enumerate(args[:-1]) if i not in (out_index, out_index + 1)]
    as_flags = []
    pp_flags = []
    for a in flags:
        if a.startswith("-Wa,"):
            as_flags += [f for f in a[4:].split(",") if f]
        else:
            pp_flags.append(a)

    # A failed build must not leave the previous object behind.
    if os.path.exists(output):
        os.remove(output)
    os.makedirs(os.path.dirname(output) or ".", exist_ok=True)
    preprocessed = output + ".pp.s"
    result = subprocess.run(wrapper + [ee_gcc, "-E", *pp_flags, "-o", preprocessed, source])
    if result.returncode:
        return result.returncode
    result = subprocess.run(wrapper + [ps2eeas, *as_flags, "-o", output, preprocessed])
    if result.returncode:
        if os.path.exists(output):
            os.remove(output)
        return result.returncode
    return 0 if os.path.exists(output) else 1


if __name__ == "__main__":
    sys.exit(main())
