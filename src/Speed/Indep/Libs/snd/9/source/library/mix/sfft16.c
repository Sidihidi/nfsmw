// El paso de muestras en coma flotante a enteros de 16 bits del mezclador de PS2 (C:/packages/snd/9.06.00-ps2/source/
// library/mix/sfft16.c): recortadas a [-32768, 32767] y empaquetadas con la VU0 y las MMI, de 16 en 16 (lqc2, vmaxx,
// vminix, vftoi0, qmfc2, ppach y sq en el binario: instrucciones que el compilador no genera).

#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"
#include <snd/sfilter.h>

// total size: 0x1C
typedef struct FT16STATE {
    SFILTERNODE sfn; // offset 0x0
} FT16STATE;

int SFILTER_ft16(void *pstate, int frames, void *psrcp, void *pdstp, int requester) {
    int temp;

    if (((FT16STATE *)pstate)->sfn.pfnnext) {
        temp = ((FT16STATE *)pstate)
                   ->sfn.pfnnext->filterfn(((FT16STATE *)pstate)->sfn.pfnnext, frames, pdstp, psrcp,
                                           ((FT16STATE *)pstate)->sfn.requester);
        if (temp <= 0) {
            return temp;
        }
    }

    asm __volatile__(".set noreorder\n"
                     "move $4, %0\n"
                     "move $5, %1\n"
                     "move $6, %2\n"
                     "li $8, -32768\n"
                     "li $9, 32767\n"
                     "qmtc2 $8, vf1\n"
                     "qmtc2 $9, vf2\n"
                     "vitof0.xyzw vf1, vf1\n"
                     "vitof0.xyzw vf2, vf2\n"
                     "1:\n"
                     "lqc2 vf3, 0x0($5)\n"
                     "lqc2 vf4, 0x10($5)\n"
                     "lqc2 vf5, 0x20($5)\n"
                     "lqc2 vf6, 0x30($5)\n"
                     "vmaxx.xyzw vf3, vf3, vf1x\n"
                     "vmaxx.xyzw vf4, vf4, vf1x\n"
                     "vmaxx.xyzw vf5, vf5, vf1x\n"
                     "vmaxx.xyzw vf6, vf6, vf1x\n"
                     "vminix.xyzw vf3, vf3, vf2x\n"
                     "vminix.xyzw vf4, vf4, vf2x\n"
                     "vminix.xyzw vf5, vf5, vf2x\n"
                     "vminix.xyzw vf6, vf6, vf2x\n"
                     "vftoi0.xyzw vf3, vf3\n"
                     "vftoi0.xyzw vf4, vf4\n"
                     "vftoi0.xyzw vf5, vf5\n"
                     "vftoi0.xyzw vf6, vf6\n"
                     "addi $4, $4, -16\n"
                     "addi $5, $5, 0x40\n"
                     "qmfc2 $7, vf3\n"
                     "qmfc2 $8, vf4\n"
                     "qmfc2 $9, vf5\n"
                     "qmfc2 $10, vf6\n"
                     "ppach $7, $8, $7\n"
                     "ppach $9, $10, $9\n"
                     "sq $7, 0x0($6)\n"
                     "sq $9, 0x10($6)\n"
                     "bgtzl $4, 1b\n"
                     "addi $6, $6, 0x20\n"
                     ".set reorder"
                     :
                     : "r"(frames), "r"(psrcp), "r"(pdstp)
                     : "$4", "$5", "$6", "$7", "$8", "$9", "$10", "memory");

    return frames;
}

void SFILTER_ft16init(FT16STATE *ps) {
    ps->sfn.filterfn = SFILTER_ft16;
    ps->sfn.restorefn = 0;
}
