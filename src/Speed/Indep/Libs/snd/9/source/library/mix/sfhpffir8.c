#include <snd/sfilter.h>

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

// total size: 0x58
typedef struct HPFSTATE {
    SFILTERNODE sfn; // offset 0x0, size 0x1C
    SNDFIRSTATE fir; // offset 0x1C, size 0x3C
} HPFSTATE;

void SNDI_fir8init(SNDFIRSTATE *psfs);
void SNDI_fir8(SNDFIRSTATE *pfir, int frames, void *psrcp, void *pdstp);
void calcFIRCoeffs(SNDFIRSTATE *pfir, int filtertype);

int SFILTER_hpfFIR8(void *pstate, int frames, void *psrcp, void *pdstp, int requester) {
    HPFSTATE *phs = (HPFSTATE *)pstate;
    int temp;

    if (phs->sfn.pfnnext) {
        temp = phs->sfn.pfnnext->filterfn(phs->sfn.pfnnext, frames, pdstp, psrcp,
                                          phs->sfn.requester);
        if (temp <= 0) {
            return temp;
        }
    } else {
        temp = frames;
    }
    SNDI_fir8(&phs->fir, temp, psrcp, pdstp);
    return temp;
}

int SFILTER_createHPFFIR8(HPFSTATE *phs) {
    phs->sfn.pfnnext = 0; phs->sfn.pfnnext2 = 0; phs->sfn.pfnprev1 = 0; phs->sfn.pfnprev2 = 0;
    phs->sfn.clean = 0;
    phs->sfn.restorefn = 0;
    phs->sfn.filterfn = SFILTER_hpfFIR8;
    SNDI_fir8init(&phs->fir);
    return 0;
}

void SFILTER_modifyHPFFIR8(HPFSTATE *phs, int *pparams) {
    int cutofffreq;
    int samplerate;

    cutofffreq = pparams[0] >> 7;
    samplerate = pparams[1] >> 8;
    phs->fir.cutLow = (float)cutofffreq / (float)samplerate;
    calcFIRCoeffs(&phs->fir, 3);
}
