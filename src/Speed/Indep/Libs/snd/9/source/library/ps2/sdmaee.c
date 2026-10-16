// Las copias entre la memoria principal y el scratchpad por DMA en el driver de PS2 (C:/packages/snd/9.06.00-ps2/
// source/library/ps2/sdmaee.c): el canal 9 lleva al scratchpad y el 8 lo trae; con el bit 0x10000 de flags se espera a
// que termine.

#include "sndps2.h"
#include <eeregs.h>

void SNDDMAI_ToScratchPad(u_int *src, u_int *dst, u_int qwords, int flags) {
    while (DGET_D9_CHCR() & D_CHCR_STR_M) {
    }

    DPUT_D9_MADR((u_int)src);
    DPUT_D9_SADR((u_int)dst);
    DPUT_D9_QWC(qwords);

    DPUT_D9_CHCR(D_CHCR_STR_M);

    if (flags & 0x10000) {
        while (DGET_D9_CHCR() & D_CHCR_STR_M) {
        }
    }
}

void SNDDMAI_FromScratchPad(u_int *src, u_int *dst, u_int qwords, int flags) {
    while (DGET_D8_CHCR() & D_CHCR_STR_M) {
    }

    DPUT_D8_MADR((u_int)dst);
    DPUT_D8_SADR((u_int)src);
    DPUT_D8_QWC(qwords);

    DPUT_D8_CHCR(D_CHCR_STR_M);

    if (flags & 0x10000) {
        while (DGET_D8_CHCR() & D_CHCR_STR_M) {
        }
    }
}
