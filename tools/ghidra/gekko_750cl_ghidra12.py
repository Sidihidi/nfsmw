#!/usr/bin/env python3
"""Add the Gekko/Broadway language (PowerPC 750CL with paired singles) to Ghidra 12.1.2.

Usage:
  python tools/ghidra/gekko_750cl_ghidra12.py <ghidra_install_dir>

Ghidra 12.1.2 has no Gekko/Broadway variant, so it cannot decode the GameCube ELF's paired-single
instructions (psq_l, psq_st, ps_*): the disassembler stops at the first one and the function body is cut
there. Ghidra's own sources had the variant at commit e2f20dab4c26180e6358f40cd7f279adcae5f166 (April 2025;
the 11.4 DEV builds of that date ship it), and it is not on master any more.

Step 1, by hand: copy these four files, unchanged, from that commit
  https://github.com/NationalSecurityAgency/ghidra/tree/e2f20dab4c26180e6358f40cd7f279adcae5f166/Ghidra/Processors/PowerPC/data/languages
into <ghidra_install_dir>/Ghidra/Processors/PowerPC/data/languages/:
  750cl.sinc  ppc_gekko_broadway.slaspec  ppc_gekko_broadway.pspec  ppc_gekko_broadway.cspec

Step 2, this script: the two files 12.1.2 changed since that commit cannot be copied, so it edits them.
  - ppc.ldefs: adds the PowerPC:BE:32:Gekko_Broadway entry of that commit.
  - ppc_common.sinc: ports only the 750CL parts of that commit (the second paired-single register bank, the
    token fields, the attach blocks, the dcbz_l pcodeop, the 12-bit displacement subtables), each inside
    @ifdef 750CL, and hides the VSX and AltiVec banks behind @ifndef 750CL. The other PowerPC languages
    compile exactly as in stock 12.1.2, and 12.1.2's own fixes in that file are kept.
  A copy of each original is kept as <file>.orig. Running the script again changes nothing.

Step 3: compile the language and import with it.
  <ghidra_install_dir>/support/sleigh <ghidra_install_dir>/Ghidra/Processors/PowerPC/data/languages/ppc_gekko_broadway.slaspec
  analyzeHeadless ... -import NFSMWRELEASE.ELF -processor PowerPC:BE:32:Gekko_Broadway -preScript SetGqr.java
  (SetGqr.java, next to this script, sets GQR0 = 0 before auto-analysis; see its header.)

Checked: on the stock 12.1.2 ppc_common.sinc (SHA-1 b103dc80...) the result is byte for byte the file used
for this project's analysis (SHA-1 b710b37f...), with which all 8,627 paired-single instructions of the
GameCube ELF decode.
"""

import io
import os
import shutil
import sys

LANG_FILES = ('750cl.sinc', 'ppc_gekko_broadway.slaspec', 'ppc_gekko_broadway.pspec', 'ppc_gekko_broadway.cspec')
COMMIT_URL = ('https://github.com/NationalSecurityAgency/ghidra/tree/e2f20dab4c26180e6358f40cd7f279adcae5f166/'
              'Ghidra/Processors/PowerPC/data/languages')

LDEFS_ENTRY = """  <language processor="PowerPC"
            endian="big"
            size="32"
            variant="Gekko/Broadway"
            version="1.5"
            slafile="ppc_gekko_broadway.sla"
            processorspec="ppc_gekko_broadway.pspec"
            manualindexfile="../manuals/PowerPC.idx"
            id="PowerPC:BE:32:Gekko_Broadway">
    <description>PowerPC 32-bit big endian (Gekko/Broadway variant)</description>
    <compiler name="default" spec="ppc_gekko_broadway.cspec" id="default"/>
    <external_name tool="gnu" name="powerpc:common"/>
    <external_name tool="IDA-PRO" name="ppc"/>
    <external_name tool="DWARF.register.mapping.file" name="ppc.dwarf"/>
  </language>
"""


def read(path):
    return io.open(path, encoding='utf-8', newline='').read()


def write(path, text):
    if not os.path.exists(path + '.orig'):
        shutil.copy2(path, path + '.orig')
        print('original kept as %s.orig' % path)
    io.open(path, 'w', encoding='utf-8', newline='').write(text)


class Editor:
    """Exact-text edits: each anchor must appear exactly once, or nothing is written."""

    def __init__(self, text):
        self.s = text
        self.n = 0

    def sub(self, old, new, label):
        c = self.s.count(old)
        if c != 1:
            sys.exit('FAILED: anchor %r found %d times (expected once). Is this the stock 12.1.2 file?' % (label, c))
        self.s = self.s.replace(old, new)
        self.n += 1

    def guard(self, block, label):
        self.sub(block, '@ifdef 750CL\n' + block + '@endif\n', label)


def patch_ldefs(path):
    s = read(path)
    if 'Gekko_Broadway' in s:
        print('ppc.ldefs: already has Gekko_Broadway')
        return
    e = Editor(s)
    e.sub('</language_definitions>', LDEFS_ENTRY + '</language_definitions>', 'end of ppc.ldefs')
    write(path, e.s)
    print('ppc.ldefs: Gekko_Broadway entry added')


def patch_common(path):
    s = read(path)
    if '750CL' in s:
        print('ppc_common.sinc: already patched')
        return
    e = Editor(s)

    # 1. the VSX register bank does not exist on the 750CL (paired singles overlay the FPRs)
    e.sub('define register offset=0x4000 size=16 \n',
          '@ifndef 750CL\ndefine register offset=0x4000 size=16 \n', 'guard VSX bank, open')
    e.sub('      vs48  vs49  vs50  vs51  vs52  vs53  vs54  vs55  vs56  vs57  vs58  vs59  vs60  vs61  vs62  vs63 \n'
          '    ];\n',
          '      vs48  vs49  vs50  vs51  vs52  vs53  vs54  vs55  vs56  vs57  vs58  vs59  vs60  vs61  vs62  vs63 \n'
          '    ];\n@endif\n\n', 'guard VSX bank, close')

    # 2. the psN_1 register bank: the second slot of each paired single
    anchor = '\t_ f16 _ f17 _ f18 _ f19 _ f20 _ f21 _ f22 _ f23 _ f24 _ f25 _ f26 _ f27 _ f28 _ f29 _ f30 _ f31 ];\n@endif\n'
    e.sub(anchor, anchor + """
@ifdef 750CL
define register offset=0x11000 size=8 [
         ps0_1 ps1_1  ps2_1  ps3_1  ps4_1  ps5_1  ps6_1  ps7_1
         ps8_1 ps9_1  ps10_1 ps11_1 ps12_1 ps13_1 ps14_1 ps15_1
        ps16_1 ps17_1 ps18_1 ps19_1 ps20_1 ps21_1 ps22_1 ps23_1
        ps24_1 ps25_1 ps26_1 ps27_1 ps28_1 ps29_1 ps30_1 ps31_1];
@endif
""", 'psN_1 register bank')

    # 3. token fields (guarded in step 9)
    e.sub('\tfD=(21,25)\n',
          '\tfD=(21,25)\n\tps0A=(16,20)\n\tps1A=(16,20)\n\tps0B=(11,15)\n\tps1B=(11,15)\n'
          '\tps0C=(6,10)\n\tps1C=(6,10)\n\tps0D=(21,25)\n\tps1D=(21,25)\n', 'fields psNA..psND')
    e.sub('\tfS=(21,25)\n', '\tfS=(21,25)\n\tps0S=(21,25)\n\tps1S=(21,25)\n\tps1T=(21,25)\n', 'fields psNS, ps1T')
    e.sub('\tIMM=(11,15)\n', '\tIMM=(11,15)\n\n\tI=(12,14)\n\tIX=(7,9)\n', 'fields I, IX')
    e.sub('\tSIMM_DS=(2,15) signed\n', '\tSIMM_PS=(0,11) signed\n\tSIMM_DS=(2,15) signed\n', 'field SIMM_PS')
    e.sub('\tWC=(21,22)\n', '\tW=(15,15)\n\tWX=(10,10)\n\tWC=(21,22)\n', 'fields W, WX')
    e.sub('\tXOP_1_5=(1,5)\n', '\tXOP_1_5=(1,5)\n\tXOP_1_6=(1,6)\n', 'field XOP_1_6')   # used by psq_lx/psq_stx

    # 4. the VSX 'T' attach
    e.sub('attach variables [ T ]\n', '@ifndef 750CL\nattach variables [ T ]\n', 'guard T attach, open')
    e.sub('      vs16  vs17  vs18  vs19  vs20  vs21  vs22  vs23  vs24  vs25  vs26  vs27  vs28  vs29  vs30  vs31 \n'
          '    ];\n',
          '      vs16  vs17  vs18  vs19  vs20  vs21  vs22  vs23  vs24  vs25  vs26  vs27  vs28  vs29  vs30  vs31 \n'
          '    ];\n@endif\n', 'guard T attach, close')

    # 5. the 750CL attach blocks, right after the fA/fB/fC/fD/fS/fT attach
    anchor = ('                   f16 f17 f18 f19 f20 f21 f22 f23\n'
              '                   f24 f25 f26 f27 f28 f29 f30 f31 ];\n')
    e.sub(anchor, anchor + """
@ifdef 750CL

# fX are mirrored to psX, so we simplify and just use the fX registers as the psX_0 registers.
# This is also how the actual hardware behaves.
attach variables [ ps0D ps0B ps0A ps0C ps0S ]
                 [ f0  f1  f2  f3  f4  f5  f6  f7
                   f8  f9  f10 f11 f12 f13 f14 f15
                   f16 f17 f18 f19 f20 f21 f22 f23
                   f24 f25 f26 f27 f28 f29 f30 f31 ];

attach variables [ ps1D ps1B ps1A ps1C ps1S ps1T ]
                 [  ps0_1  ps1_1  ps2_1  ps3_1  ps4_1  ps5_1  ps6_1  ps7_1
                    ps8_1  ps9_1 ps10_1 ps11_1 ps12_1 ps13_1 ps14_1 ps15_1
                   ps16_1 ps17_1 ps18_1 ps19_1 ps20_1 ps21_1 ps22_1 ps23_1
                   ps24_1 ps25_1 ps26_1 ps27_1 ps28_1 ps29_1 ps30_1 ps31_1 ];

# GQR0-GQR7
attach variables [I IX]
\t[spr390 spr391 spr392 spr393 spr394 spr395 spr396 spr397];
@endif
""", '750CL attach blocks')

    # 6. the AltiVec / VSX attach region
    e.sub('attach variables [vrDR vrAR vrBR vrSR vrCR]\n',
          '@ifndef 750CL\nattach variables [vrDR vrAR vrBR vrSR vrCR]\n', 'guard AltiVec attach, open')
    i = e.s.find('attach variables vrC_8_15 [')
    if i < 0:
        sys.exit('FAILED: no "attach variables vrC_8_15 [" in ppc_common.sinc')
    j = e.s.find('\n', i)
    e.s = e.s[:j + 1] + '@endif # @ifndef 750CL\n' + e.s[j + 1:]
    e.n += 1

    # 7. the pcodeop of dcbz_l
    e.sub('define pcodeop dataCacheBlockClearToZero;\n',
          'define pcodeop dataCacheBlockClearToZero;\ndefine pcodeop dataCacheBlockSetToZeroLocked;\n',
          'pcodeop dcbz_l')

    # 8. the 12-bit displacement address subtables of psq_l / psq_st
    e.sub('dPlusRaAddress: SIMM(A)\t\t\t\t\tis SIMM & A\t\t\t\t{tmp:$(REGISTER_SIZE) = A+SIMM; export tmp;  }\n',
          'dPlusRaAddress: SIMM(A)\t\t\t\t\tis SIMM & A\t\t\t\t{tmp:$(REGISTER_SIZE) = A+SIMM; export tmp;  }\n'
          'dPlusRaOrZeroAddressPS: SIMM_PS(RA_OR_ZERO) is SIMM_PS & RA_OR_ZERO { tmp:4 = RA_OR_ZERO+SIMM_PS; export tmp; }\n'
          'dPlusRaAddressPS:       SIMM_PS(A)          is SIMM_PS & A          { tmp:4 = A+SIMM_PS; export tmp; }\n',
          'PS address subtables')

    # 9. everything added above that other PowerPC languages must not see
    e.guard('\tps0A=(16,20)\n\tps1A=(16,20)\n\tps0B=(11,15)\n\tps1B=(11,15)\n'
            '\tps0C=(6,10)\n\tps1C=(6,10)\n\tps0D=(21,25)\n\tps1D=(21,25)\n', 'guard psNA..psND')
    e.guard('\tps0S=(21,25)\n\tps1S=(21,25)\n\tps1T=(21,25)\n', 'guard psNS, ps1T')
    e.guard('\n\tI=(12,14)\n\tIX=(7,9)\n', 'guard I, IX')
    e.guard('\tSIMM_PS=(0,11) signed\n', 'guard SIMM_PS')
    e.guard('\tW=(15,15)\n\tWX=(10,10)\n', 'guard W, WX')
    e.guard('\tXOP_1_6=(1,6)\n', 'guard XOP_1_6')
    e.guard('define pcodeop dataCacheBlockSetToZeroLocked;\n', 'guard dcbz_l')
    e.guard('dPlusRaOrZeroAddressPS: SIMM_PS(RA_OR_ZERO) is SIMM_PS & RA_OR_ZERO { tmp:4 = RA_OR_ZERO+SIMM_PS; export tmp; }\n'
            'dPlusRaAddressPS:       SIMM_PS(A)          is SIMM_PS & A          { tmp:4 = A+SIMM_PS; export tmp; }\n',
            'guard PS address subtables')

    write(path, e.s)
    print('ppc_common.sinc: %d edits' % e.n)


def main():
    if len(sys.argv) != 2 or sys.argv[1] in ('-h', '--help'):
        print(__doc__)
        sys.exit(0 if len(sys.argv) == 2 else 1)
    lang = os.path.join(sys.argv[1], 'Ghidra', 'Processors', 'PowerPC', 'data', 'languages')
    if not os.path.isfile(os.path.join(lang, 'ppc_common.sinc')):
        sys.exit('no Ghidra PowerPC languages in %s' % lang)
    missing = [f for f in LANG_FILES if not os.path.isfile(os.path.join(lang, f))]
    if missing:
        sys.exit('copy these files first, unchanged, from %s\ninto %s:\n  %s' % (COMMIT_URL, lang, '\n  '.join(missing)))
    patch_ldefs(os.path.join(lang, 'ppc.ldefs'))
    patch_common(os.path.join(lang, 'ppc_common.sinc'))
    print('now compile: %s %s' % (os.path.join(sys.argv[1], 'support', 'sleigh'),
                                  os.path.join(lang, 'ppc_gekko_broadway.slaspec')))


if __name__ == '__main__':
    main()
