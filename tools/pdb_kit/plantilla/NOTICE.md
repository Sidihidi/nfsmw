# Notices

## License of this kit

Everything in this kit is released under CC0 1.0 Universal (see `LICENSE`), the license of the NFS: Most Wanted
decompilation repository it comes from. That covers the scripts, the documentation, the configuration data and the
generated symbol files.

## Where the data comes from

- `repo/config/SPEED_EXE_1_3/symbols.json` started as an IDA export of `speed.exe` made by the upstream decompilation
  project (`https://github.com/dbalatoni13/nfsmw`, CC0 1.0). The fork renamed it since, as described in
  `docs/PC_PDB.md`. Its `meta.input_file` field still shows the path of the machine where upstream made the export;
  it is kept as upstream published it.
- Names, types, parameter and local names, and line numbers in the PDB come from the decompiled source of the project,
  compiled with Microsoft Visual C++ 7.1. Types and line numbers of the C++ standard library refer to the compiler's
  own headers (`build/compilers/Win32/7.1/Include/...`): their names, member layouts and line numbers only. No header
  file, library or other part of the compiler is included.
- The PDB file was written by LLVM's `lld-link` (Apache License 2.0 with LLVM Exceptions) from synthetic objects. No
  part of `lld-link` is included.

## Not included

- No file of the game: no executable, no data, no assets. The PDB and the other files contain no machine code or data
  of the game. Section 6 of `docs/PC_PDB.md` explains how that was checked.
- No compiler, SDK or linker.

## Trademarks

*Need for Speed* and *Most Wanted* are trademarks of Electronic Arts Inc. This is an unofficial research project, not
affiliated with or endorsed by Electronic Arts. Microsoft, Visual C++ and Windows are trademarks of Microsoft
Corporation.
