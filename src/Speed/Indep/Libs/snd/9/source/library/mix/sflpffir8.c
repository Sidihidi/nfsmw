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
typedef struct LPFSTATE {
    SFILTERNODE sfn; // offset 0x0, size 0x1C
    SNDFIRSTATE fir; // offset 0x1C, size 0x3C
} LPFSTATE;

void SNDI_fir8init(SNDFIRSTATE *psfs);
void SNDI_fir8(SNDFIRSTATE *pfir, int frames, void *psrcp, void *pdstp);
void calcFIRCoeffs(SNDFIRSTATE *pfir, int filtertype);

int SFILTER_lpfFIR8(void *pstate, int outputframes, void *psrcp, void *pdstp, int requester) {
    LPFSTATE *pls = (LPFSTATE *)pstate;
    int temp;

    if (pls->sfn.pfnnext) {
        temp = pls->sfn.pfnnext->filterfn(pls->sfn.pfnnext, outputframes, pdstp, psrcp,
                                          pls->sfn.requester);
        if (temp <= 0) {
            return temp;
        }
    }
    SNDI_fir8(&pls->fir, outputframes, psrcp, pdstp);
    return outputframes;
}

int SFILTER_createLPFFIR8(LPFSTATE *pls) {
    pls->sfn.pfnnext = 0; pls->sfn.pfnnext2 = 0; pls->sfn.pfnprev1 = 0; pls->sfn.pfnprev2 = 0;
    pls->sfn.clean = 0;
    pls->sfn.restorefn = 0;
    pls->sfn.filterfn = SFILTER_lpfFIR8;
    SNDI_fir8init(&pls->fir);
    return 0;
}

void SFILTER_modifyLPFFIR8(LPFSTATE *pls, int *pparams) {
    int cutofffreq;
    int samplerate;
    float leakage;

    cutofffreq = pparams[0] >> 8;
    samplerate = pparams[1] >> 8;
    leakage = 2.0f * cutofffreq / samplerate;
    pls->fir.cutHigh = leakage;
    calcFIRCoeffs(&pls->fir, 2);
}
