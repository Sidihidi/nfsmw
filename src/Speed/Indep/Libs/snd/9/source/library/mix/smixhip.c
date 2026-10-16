#include "../cmn/sndcmn.h"
#include <snd/sfilter.h>
#include "smixi.h"

// total size: 0x3C (sfir8.c)
// En PS2 (snd 9.06.00-ps2) mide 0x100: los stabs de la A124 le ponen delante los coeficientes para la VU0,
// alineados a 16 (por eso la `fir` de LPFSTATE/HPFSTATE/BPFSTATE cae en 0x20), y detras resetVU0coef.
typedef struct SNDFIRSTATE {
#ifdef EA_PLATFORM_PLAYSTATION2
    float VU0coef[12][4] __attribute__((aligned(16))); // offset 0x0, size 0xC0
#endif
    float history[8]; // offset 0x0, size 0x20 (PS2: 0xC0)
    float coef[5];    // offset 0x20, size 0x14 (PS2: 0xE0)
    float cutLow;     // offset 0x34 (PS2: 0xF4)
    float cutHigh;    // offset 0x38 (PS2: 0xF8)
#ifdef EA_PLATFORM_PLAYSTATION2
    signed char resetVU0coef; // offset 0xFC
    char pad[3];              // offset 0xFD
#endif
} SNDFIRSTATE;

// total size: 0x58 (sfhpffir8.c)
typedef struct HPFSTATE {
    SFILTERNODE sfn; // offset 0x0, size 0x1C
    SNDFIRSTATE fir; // offset 0x1C, size 0x3C
} HPFSTATE;

int SFILTER_createHPFFIR8(HPFSTATE *phs);
void SFILTER_modifyHPFFIR8(HPFSTATE *phs, int *pparams);

extern "C" {
void SFILTER_add(SFILTERNODE **pphead, SFILTERNODE *pnew);
void SFILTER_remove(SFILTERNODE **pphead, SFILTERNODE *pcur);
}

extern "C" void MIX_sethighpass(int voice, int cutofffreq) {
    MIXVOICE *psmv;
    int params[2];

    psmv = &sndmix.voices[voice];
    if (cutofffreq > 0) {
        if (psmv->phpf == 0) {
            psmv->phpf = (HPFSTATE *)SNDMEMI_allocz(sizeof(HPFSTATE));
            psmv->phpf->sfn.restorefn = 0;
            psmv->phpf->sfn.priority = 0x50;
            psmv->phpf->sfn.requester = 0;
            SFILTER_createHPFFIR8(psmv->phpf);
            SFILTER_add(&psmv->pfilterhead, (SFILTERNODE *)psmv->phpf);
        }
        params[0] = cutofffreq << 8;
        params[1] = sndgs.sso.set.outputrate << 8;
        SFILTER_modifyHPFFIR8(psmv->phpf, params);
    } else {
        if (psmv->phpf != 0) {
            SFILTER_remove(&psmv->pfilterhead, (SFILTERNODE *)psmv->phpf);
            SNDMEMI_free(psmv->phpf);
            psmv->phpf = 0;
        }
    }
}
