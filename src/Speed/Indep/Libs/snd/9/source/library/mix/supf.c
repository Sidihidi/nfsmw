#include <snd/sfilter.h>

// smixi.h (shared with the other mix/ units) only carries a partial view of
// sndmix, so the fields this unit needs are declared locally.

extern "C" {
void *memset(void *dst, int c, unsigned int bytes);
}

// r75 (PS2): el paquete PS2 del snd (C:/packages/snd/9.06.00-ps2, segun el
// .mdebug) llama DIRECTO al decodificador: `jal decode16x87` con simbolo C
// sin manglar. El de GC (C:/MyWork/packages/snd/9) despacha por el puntero
// del campo sndmix+0x1E4 (R_PPC_ADDR16_* sndmix+0x1e4 en su .o original).
#ifdef EA_PLATFORM_PLAYSTATION2
extern "C" {
void decode16x87(int frames, short *psrc, float *pdst);
}
#endif

#ifdef EA_PLATFORM_WIN32
// PC: snd pone a cero con su asm de x87 (speed.exe 0x822050, dos argumentos); el nombre es nuestro (pc/sdsound.c).
void SNDI_memclear(void *p, int bytes);
// PC: las muestras de 8 bits (speed.exe 0x828F10, asm de x86 con la tabla 0x9C2550); el nombre es nuestro.
void decode8(int frames, signed char *psrc, float *pdst);
#endif

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

// total size: 0x2C
typedef struct UNPACKFSTATE {
    SFILTERNODE sfn;          // offset 0x0, size 0x1C
    char *psample;            // offset 0x1C
    unsigned int curframe;    // offset 0x20
    unsigned int totalframes; // offset 0x24
    int bits16;               // offset 0x28
} UNPACKFSTATE;

int SFILTER_unpackfgetframe(void *pstate) {
    UNPACKFSTATE *pups = (UNPACKFSTATE *)pstate;

    return pups->curframe;
}

int SFILTER_unpackf(void *pstate, int framestoup, void *psrcp, void *pdstp, int requester) {
    UNPACKFSTATE *pups = (UNPACKFSTATE *)pstate;
    int midframe;
    int startframe;
    float *pdst;

    startframe = pups->curframe;
    if (startframe >= pups->totalframes) {
        return -1;
    }
    pups->curframe = startframe + framestoup;
    if (pups->curframe < pups->totalframes) {
        if (pups->bits16) {
#ifdef EA_PLATFORM_PLAYSTATION2
            decode16x87(framestoup, (short *)(pups->psample + (startframe << 1)), (float *)pdstp);
#else
            sndmix.decode16(framestoup, (short *)(pups->psample + (startframe << 1)), (float *)pdstp);
#endif
        }
#ifdef EA_PLATFORM_WIN32
        else {
            decode8(framestoup, (signed char *)(pups->psample + startframe), (float *)pdstp);
        }
#endif
    } else {
        midframe = pups->totalframes - startframe;
        if (pups->bits16) {
#ifdef EA_PLATFORM_PLAYSTATION2
            decode16x87(midframe, (short *)(pups->psample + (startframe << 1)), (float *)pdstp);
#else
            sndmix.decode16(midframe, (short *)(pups->psample + (startframe << 1)), (float *)pdstp);
#endif
        }
#ifdef EA_PLATFORM_WIN32
        else {
            decode8(midframe, (signed char *)(pups->psample + startframe), (float *)pdstp);
        }
#endif
        pdst = (float *)pdstp + midframe;
#if defined(EA_PLATFORM_WIN32) && defined(_MSC_VER)
        SNDI_memclear(pdst, (framestoup - midframe) * sizeof(float));
#else
        memset(pdst, 0, (framestoup - midframe) * sizeof(float));
#endif
    }
    return 1;
}

void SFILTER_unpackfinit(void *pupsin, UNPACKINITPARAMS *pupip) {
    UNPACKFSTATE *pups = (UNPACKFSTATE *)pupsin;

    pups->sfn.filterfn = SFILTER_unpackf;
    pups->psample = (char *)pupip->psample;
    pups->curframe = 0;
    pups->totalframes = pupip->totalframes;
#ifndef EA_PLATFORM_XENON
    pups->bits16 = pupip->bits16;
#endif
    pupip->unpackgetframe = SFILTER_unpackfgetframe;
    pups->sfn.restorefn = 0;
}
