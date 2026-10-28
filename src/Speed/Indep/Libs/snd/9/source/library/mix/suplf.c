#include <snd/sfilter.h>

// smixi.h (shared with the other mix/ units) only carries a partial view of
// sndmix, so the fields this unit needs are declared locally.

// total size: 0x1E8 (Xenon: 0x110)
typedef struct SNDMIXSTATE {
#ifdef EA_PLATFORM_XENON
    // Xenon: decode16 va en 0x10C, detras de mix (ver smixer.c).
    char pad0[0x10C];
    void (*decode16)(int frames, short *psrc, float *pdst); // offset 0x10C
#else
    char pad0[0x1E4];
    void (*decode16)(int frames, short *psrc, float *pdst); // offset 0x1E4
#endif
} SNDMIXSTATE;

extern SNDMIXSTATE sndmix;

// r75 (PS2): ver supf.c -- el paquete PS2 (snd 9.06.00-ps2) llama directo a
// `jal decode16x87`; GC despacha por el puntero sndmix+0x1E4.
#ifdef EA_PLATFORM_PLAYSTATION2
extern "C" {
void decode16x87(int frames, short *psrc, float *pdst);
}
#endif

#ifdef EA_PLATFORM_WIN32
// PC: las muestras de 8 bits (speed.exe 0x828F10, asm de x86 con la tabla 0x9C2550); el nombre es nuestro.
void decode8(int frames, signed char *psrc, float *pdst);
#endif

// total size: 0x2C (Xenon: 0x28)
typedef struct UNPACKINITPARAMS {
    void *psample;                 // offset 0x0
    void *ploopstart;              // offset 0x4
    int numchan;                   // offset 0x8
    int totalframes;               // offset 0xC
    int sustainstart;              // offset 0x10
    int sustainend;                // offset 0x14
#ifndef EA_PLATFORM_XENON
    // Xenon no tiene bits16 (EGM, MIX_playinit 0x82617AC0: escribe voice/cpu/platformver
    // en 0x18/0x1C/0x20 y unpackgetframe en 0x24, y el struct mide 0x28).
    int bits16;                    // offset 0x18
#endif
    int voice;                     // offset 0x1C (Xenon: 0x18)
    int cpu;                       // offset 0x20 (Xenon: 0x1C)
    int platformver;               // offset 0x24 (Xenon: 0x20)
    int (*unpackgetframe)(void *); // offset 0x28 (Xenon: 0x24)
} UNPACKINITPARAMS;

// total size: 0x30
typedef struct UNPACKLFSTATE {
    SFILTERNODE sfn;            // offset 0x0, size 0x1C
    char *psample;              // offset 0x1C
    unsigned int curframe;      // offset 0x20
    unsigned int sustainstart;  // offset 0x24
    unsigned int sustainend;    // offset 0x28
    int bits16;                 // offset 0x2C
} UNPACKLFSTATE;

int SFILTER_unpacklfgetframe(void *pstate) {
    UNPACKLFSTATE *pups = (UNPACKLFSTATE *)pstate;

    return pups->curframe;
}

int SFILTER_unpacklf(void *pstate, int framestoup, void *psrcp, void *pdstp, int requester) {
    UNPACKLFSTATE *pups = (UNPACKLFSTATE *)pstate;
    int maxframes;
    float *pdst;

    pdst = (float *)pdstp;
    while (framestoup > 0) {
        maxframes = pups->sustainend - pups->curframe + 1;
        if (framestoup < maxframes) {
            maxframes = framestoup;
        }
        if (pups->bits16) {
#ifdef EA_PLATFORM_PLAYSTATION2
            decode16x87(maxframes, (short *)(pups->psample + (pups->curframe << 1)), pdst);
#else
            sndmix.decode16(maxframes, (short *)(pups->psample + (pups->curframe << 1)), pdst);
#endif
        }
#ifdef EA_PLATFORM_WIN32
        else {
            decode8(maxframes, (signed char *)(pups->psample + pups->curframe), pdst);
        }
#endif
        pdst += maxframes;
        pups->curframe += maxframes;
        framestoup -= maxframes;
        if (pups->curframe > pups->sustainend) {
            pups->curframe = pups->sustainstart;
        }
    }
    return 1;
}

void SFILTER_unpacklfinit(void *pstate, UNPACKINITPARAMS *pupip) {
    UNPACKLFSTATE *pups = (UNPACKLFSTATE *)pstate;

    pups->sfn.filterfn = SFILTER_unpacklf;
    pups->psample = (char *)pupip->psample;
    pups->curframe = 0;
    pups->sustainstart = pupip->sustainstart;
    pups->sustainend = pupip->sustainend;
#ifndef EA_PLATFORM_XENON
    pups->bits16 = pupip->bits16;
#endif
    pupip->unpackgetframe = SFILTER_unpacklfgetframe;
}
