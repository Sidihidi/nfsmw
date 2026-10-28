#include "../cmn/sndcmn.h"
#include <snd/sfilter.h>
#include "smixi.h"

// total size: 0x3C (sfrsf.c); en PS2 0x38: snd 9.06.00-ps2 no lleva el puntero rsf
typedef struct RSFSTATE {
    SFILTERNODE sfn; // offset 0x0, size 0x1C
#ifdef EA_PLATFORM_PLAYSTATION2
    char pad0[0x38 - 0x1C];
#else
    char pad0[0x3C - 0x1C];
#endif
} RSFSTATE;

void SFILTER_rsfinit(RSFSTATE *prs, int cputypes, int method);
void SFILTER_rsfsetpitch(RSFSTATE *prs, int pitchmult);

extern "C" {
void SFILTER_add(SFILTERNODE **pphead, SFILTERNODE *pnew);
}

extern "C" void MIX_setpitch(int voice, int pitchmult) {
    MIXVOICE *psmv;
    int method;

    psmv = &sndmix.voices[voice];
    method = 0;
#ifdef EA_PLATFORM_PLAYSTATION2
    if (pitchmult > 0x20000) {
        pitchmult = 0x20000;
    }
#else
    if (pitchmult > 0x40000) {
        pitchmult = 0x40000;
    }
#endif
    if (psmv->prsf == 0) {
        psmv->prsf = (RSFSTATE *)SNDMEMI_allocz(sizeof(RSFSTATE));
        if (sndmix.platformver >= 0x32) {
            method = 1;
        }
        psmv->prsf->sfn.restorefn = 0;
        psmv->prsf->sfn.priority = 0xa0;
        psmv->prsf->sfn.requester = 0;
        SFILTER_rsfinit(psmv->prsf, sndgs.cputypes, method);
        SFILTER_add(&psmv->pfilterhead, (SFILTERNODE *)psmv->prsf);
    }
    SFILTER_rsfsetpitch(psmv->prsf, pitchmult);
}
