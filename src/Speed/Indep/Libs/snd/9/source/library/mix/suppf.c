#include <snd/sfilter.h>

// smixi.h (shared with the other mix/ units) only carries a partial view of
// sndmix, so the fields this unit needs are declared locally.

extern "C" {
void *memset(void *dst, int c, unsigned int bytes);
}

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
// PC: snd pone a cero con su asm de x87 (speed.exe 0x822050, dos argumentos); el nombre es nuestro (pc/sdsound.c).
void SNDI_memclear(void *p, int bytes);
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

// total size: 0x34
typedef struct UNPACKPFSTATE {
    SFILTERNODE sfn;                  // offset 0x0, size 0x1C
    signed char *psample;             // offset 0x1C
    int totalframes;                  // offset 0x20
    int curframe;                     // offset 0x24
    int packetinstancehandle;         // offset 0x28
    unsigned int prevframesunpacked;  // offset 0x2C
    unsigned char bits16;             // offset 0x30
    unsigned char samplechan;         // offset 0x31
    char pad[2];                      // offset 0x32
} UNPACKPFSTATE;

int SNDDRV_getmastervoice(int voice);
int SNDDRV_getsamplechan(int voice);
int SNDPKTPLAYI_voicetopackethandle(int voice);
char *SNDPKTPLAYI_get(int pktchan, int chan, int *pframes, int *pcontinuation);
void SNDPKTPLAYI_freeframes(int pktchan, int chan, int frames);

int SFILTER_unpackpf(void *pstate, int framestoup, void *psrcp, void *pdstp, int requester) {
    UNPACKPFSTATE *pups = (UNPACKPFSTATE *)pstate;
    int maxframes;
    int totalframes;
    int continuation;
    float *pdst;

    pdst = (float *)pdstp;
    if (pups->prevframesunpacked) {
        SNDPKTPLAYI_freeframes(pups->packetinstancehandle, pups->samplechan,
                               pups->prevframesunpacked);
        pups->prevframesunpacked = 0;
    }
    if (framestoup > 0) {
        do {
        if (pups->curframe >= pups->totalframes) {
            pups->psample = (signed char *)SNDPKTPLAYI_get(pups->packetinstancehandle,
                                                           pups->samplechan, &totalframes,
                                                           &continuation);
            if (pups->psample == 0) {
                if (pups->prevframesunpacked == 0) {
                    break;
                }
#if defined(EA_PLATFORM_WIN32) && defined(_MSC_VER)
                SNDI_memclear(pdst, framestoup * sizeof(float));
#else
                memset(pdst, 0, framestoup * sizeof(float));
#endif
                break;
            }
            pups->curframe = 0;
            pups->totalframes = totalframes;
        }
        maxframes = pups->totalframes - pups->curframe;
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
            decode8(maxframes, pups->psample + pups->curframe, pdst);
        }
#endif
        pdst += maxframes;
        pups->curframe += maxframes;
        framestoup -= maxframes;
        pups->prevframesunpacked += maxframes;
        } while (framestoup > 0);
    }
    return pups->prevframesunpacked;
}

void SFILTER_unpackpfinit(void *pstate, UNPACKINITPARAMS *pupip) {
    UNPACKPFSTATE *pups = (UNPACKPFSTATE *)pstate;

    pups->sfn.filterfn = SFILTER_unpackpf;
    pups->packetinstancehandle =
        SNDPKTPLAYI_voicetopackethandle(SNDDRV_getmastervoice(pupip->voice));
    pups->samplechan = SNDDRV_getsamplechan(pupip->voice);
    pups->totalframes = -1;
    pups->psample = 0;
    pups->curframe = 0;
#ifndef EA_PLATFORM_XENON
    pups->bits16 = pupip->bits16;
#endif
    pups->prevframesunpacked = 0;
}
