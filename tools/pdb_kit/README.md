# PC PDB kit builder

Builds `dist/nfsmw-pc-pdb-kit.zip`, the community kit for the PC `speed.exe` PDB (see `docs/PC_PDB.md`).

- `scripts/`: the English entry point (`make_pdb.py`) and English copies of the naming tools
  (`name_by_call_graph.py`, `name_by_elimination.py`, `name_corrections.py`). They are translations of
  `scripts/nombra_grafo.py`, `scripts/nombra_elim.py` and `scripts/nombra_corrige.py`: same code, English help,
  comments and messages. When those scripts change, update these copies too.
- `plantilla/`: the kit's `README.md` and `NOTICE.md`.
- `genera_con_cerrojo.py OUT`: regenerates the symbols under the `scripts/medir.py` lock (no PC rebuild meanwhile),
  with `simbolos_pc.py --english --source-root C:\nfsmw`.
- `arma_kit.py OUT_ENGLISH`: assembles the zip from that output, these scripts and the repo inputs.
- `comprueba_kit.py FOLDER EXE`: privacy check (Windows user name, user folder, clone root and anything listed in
  the `NFSMW_PRIVADO` environment variable) and check that no 16-byte block of the executable's code or data occurs
  in any file of the kit.
