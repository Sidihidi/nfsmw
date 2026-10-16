#!/usr/bin/env python3

###
# Renames ELF sections of a relocatable object in place.
#
# Some translation units were compiled into a non-default output section by the
# original build (the GameCube overlay, `.over`, is the case in this project:
# see the `*zFEOverlay.o(.over .rodata)` line in config/GOWE69/ldscript.ld).
# dtk therefore extracts the target object with its code in `.over`, while our
# compiler emits plain `.text`.  objdiff pairs symbols per section, so without
# this step every function of such a unit is invisible to the progress report.
#
# The rename is a pure byte patch of `.shstrtab`: only names of the same length
# are accepted, so no offset, size or index in the object moves.  The relocation
# section that belongs to a renamed section (`.rela<name>` / `.rel<name>`) is
# renamed along with it.
#
# Usage:
#   python3 tools/rename_section.py <object.o> <old>=<new> [<old>=<new> ...]
#   python3 tools/rename_section.py <object.o> .text=.over -o <output.o>
###

import argparse
import struct
import sys
from pathlib import Path
from typing import Dict, List, Tuple


def parse_renames(specs: List[str]) -> List[Tuple[str, str]]:
    renames: List[Tuple[str, str]] = []
    for spec in specs:
        # Allow a single argument to carry several comma separated entries.
        for part in spec.split(","):
            part = part.strip()
            if not part or "=" not in part or part.startswith("sym:"):
                continue
            old, new = part.split("=", 1)
            if len(old) != len(new):
                sys.exit(
                    f"Section names must have the same length: {old!r} vs {new!r}"
                )
            renames.append((old, new))
    return renames


def parse_aligns(specs: List[str]) -> List[Tuple[str, int]]:
    # Entries of the form NAME@ALIGN (e.g. `.data@1`), mixed with the renames in
    # the same argument list: patch sh_addralign of that section. The GameCube
    # overlay units of the original carry NO `.data` at all, while ours has an
    # empty one at alignment 8: ngcld bumps the location counter for it and the
    # `.data` of the following object (zFEOverlay, right behind zOnline in the
    # link) lands 4 bytes high, breaking the DOL. Alignment 1 on an empty
    # section is a no-op for the placement.
    aligns: List[Tuple[str, int]] = []
    for spec in specs:
        for part in spec.split(","):
            part = part.strip()
            if not part or "=" in part or "@" not in part:
                continue
            name, align = part.split("@", 1)
            try:
                value = int(align, 0)
            except ValueError:
                sys.exit(f"Invalid alignment {align!r} in {part!r}")
            if value < 1 or (value & (value - 1)) != 0:
                sys.exit(f"Alignment must be a positive power of two: {part!r}")
            aligns.append((name, value))
    return aligns


def rename_symbols(data: bytearray, renames: List[Tuple[str, str]]) -> int:
    # Entradas "sym:OLD=NEW": renombran un SIMBOLO reescriendo su cadena en .strtab IN SITU.
    # NEW debe caber en el hueco de OLD (len(NEW) <= len(OLD)): se escribe NEW + NUL sobre el
    # principio de la cadena vieja y el resto del slot queda como relleno — ningun offset se mueve.
    # Es lo que arregla el _GLOBAL_.I.<nombre> de zOnline: la instanciacion explicita de
    # __node_alloc roba el nombre del envoltorio (pasa a _GLOBAL_.I.allocate__...) y el
    # emparejado de objdiff lo pierde; el original lo llama _GLOBAL_.I.__OSBusClock.
    if data[:4] != b"ELF":
        sys.exit("Not an ELF file")
    if data[4] != 1:
        sys.exit("Only 32-bit ELF objects are supported")
    endian = ">" if data[5] == 2 else "<"

    (shoff,) = struct.unpack_from(endian + "I", data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from(endian + "3H", data, 0x2E)
    if shoff == 0 or shnum == 0:
        sys.exit("Object has no section headers")
    (shstrtab_off,) = struct.unpack_from(
        endian + "I", data, shoff + shstrndx * shentsize + 16
    )

    # localizar .strtab
    strtab_off = None
    for i in range(shnum):
        name_off = struct.unpack_from(endian + "I", data, shoff + i * shentsize)[0]
        start = shstrtab_off + name_off
        end = data.index(bytes([0]), start)
        if data[start:end].decode("ascii", "replace") == ".strtab":
            (strtab_off,) = struct.unpack_from(endian + "I", data, shoff + i * shentsize + 16)
            break
    if strtab_off is None:
        sys.exit("Object has no .strtab")

    targets = dict(renames)
    hits = 0
    for i in range(shnum):
        if struct.unpack_from(endian + "I", data, shoff + i * shentsize + 4)[0] != 2:
            continue  # solo SHT_SYMTAB
        symoff, symcount = struct.unpack_from(endian + "II", data, shoff + i * shentsize + 16)
        for j in range(symcount // 16):
            (name_off,) = struct.unpack_from(endian + "I", data, symoff + j * 16)
            start = strtab_off + name_off
            end = data.index(bytes([0]), start)
            name = data[start:end].decode("ascii", "replace")
            new_name = targets.get(name)
            if new_name is None:
                continue
            if len(new_name) > len(name):
                sys.exit(f"Symbol rename does not fit: {new_name!r} > {name!r}")
            data[start : start + len(new_name) + 1] = new_name.encode("ascii") + bytes([0])
            hits += 1
    return hits


def align_sections(data: bytearray, aligns: List[Tuple[str, int]]) -> int:
    if data[:4] != b"\x7fELF":
        sys.exit("Not an ELF file")
    if data[4] != 1:
        sys.exit("Only 32-bit ELF objects are supported")
    endian = ">" if data[5] == 2 else "<"

    (shoff,) = struct.unpack_from(endian + "I", data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from(endian + "3H", data, 0x2E)
    if shoff == 0 or shnum == 0:
        sys.exit("Object has no section headers")
    (shstrtab_off,) = struct.unpack_from(
        endian + "I", data, shoff + shstrndx * shentsize + 16
    )

    hits = 0
    for i in range(shnum):
        (name_off,) = struct.unpack_from(endian + "I", data, shoff + i * shentsize)
        start = shstrtab_off + name_off
        end = data.index(b"\0", start)
        name = data[start:end].decode("ascii", "replace")
        for target, value in aligns:
            if name != target:
                continue
            struct.pack_into(endian + "I", data, shoff + i * shentsize + 32, value)
            hits += 1
    return hits


def rename_sections(data: bytearray, renames: List[Tuple[str, str]]) -> int:
    if data[:4] != b"\x7fELF":
        sys.exit("Not an ELF file")
    if data[4] != 1:
        sys.exit("Only 32-bit ELF objects are supported")
    endian = ">" if data[5] == 2 else "<"

    (shoff,) = struct.unpack_from(endian + "I", data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from(endian + "3H", data, 0x2E)
    if shoff == 0 or shnum == 0:
        sys.exit("Object has no section headers")
    (shstrtab_off,) = struct.unpack_from(
        endian + "I", data, shoff + shstrndx * shentsize + 16
    )

    # A renamed section drags its relocation section along.
    targets: Dict[str, str] = {}
    for old, new in renames:
        targets[old] = new
        targets[".rela" + old] = ".rela" + new
        targets[".rel" + old] = ".rel" + new

    hits = 0
    for i in range(shnum):
        (name_off,) = struct.unpack_from(endian + "I", data, shoff + i * shentsize)
        start = shstrtab_off + name_off
        end = data.index(b"\0", start)
        name = data[start:end].decode("ascii", "replace")
        new_name = targets.get(name)
        if new_name is None:
            continue
        # Same length by construction, so this cannot disturb any other name
        # that shares this suffix in the string table.
        data[start : start + len(new_name)] = new_name.encode("ascii")
        hits += 1
    return hits


def main() -> None:
    parser = argparse.ArgumentParser(description="Rename ELF sections in place.")
    parser.add_argument("object", type=Path, help="relocatable object to patch")
    parser.add_argument(
        "renames", nargs="+", metavar="OLD=NEW", help="section rename, e.g. .text=.over"
    )
    parser.add_argument(
        "-o", "--output", type=Path, help="write here instead of patching in place"
    )
    parser.add_argument(
        "-q", "--quiet", action="store_true", help="do not print what was renamed"
    )
    args = parser.parse_args()

    renames = parse_renames(args.renames)
    aligns = parse_aligns(args.renames)
    symrenames = [
        (part.split(":", 1)[1].split("=", 1)[0], part.split(":", 1)[1].split("=", 1)[1])
        for spec in args.renames
        for part in spec.split(",")
        if part.strip().startswith("sym:")
    ]
    if not renames and not aligns and not symrenames:
        sys.exit("No renames or alignments given")
    data = bytearray(args.object.read_bytes())
    hits = rename_sections(data, renames)
    hits += align_sections(data, aligns)
    hits += rename_symbols(data, symrenames)
    (args.output or args.object).write_bytes(data)
    if not args.quiet:
        pairs = ", ".join(f"{old} -> {new}" for old, new in renames)
        pairs += "".join(f", {name} @ {value}" for name, value in aligns)
        print(f"{args.object}: patched {hits} section(s) ({pairs})")


if __name__ == "__main__":
    main()
