#include <cstddef>
#include <snd/sfilter.h>

extern "C" {
void *memset(void *dst, int c, unsigned int bytes);
}

#ifdef EA_PLATFORM_WIN32
// PC: snd pone a cero con su asm de x87 (speed.exe 0x822050, dos argumentos); el nombre es nuestro (pc/sdsound.c).
void SNDI_memclear(void *p, int bytes);
#endif

namespace Snd {

// total size: 0xA8 (eaxadecf.cpp)
struct CEAXABLKDecf {
    static void *operator new(size_t size);
    static void operator delete(void *ptr);

    CEAXABLKDecf();
    ~CEAXABLKDecf() {}
    int Feed(void *pSampleData, int sampleDataSize, int numSamples);
    int Decode(float **pDstBuf, int numSamples);

    char pad0[0xA8];
};

} // namespace Snd

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
typedef struct UNPACKXAFSTATE {
    SFILTERNODE sfn;             // offset 0x0, size 0x1C
    Snd::CEAXABLKDecf *pDecoder; // offset 0x1C
    unsigned char *psample;      // offset 0x20
    int totalframes;             // offset 0x24
    int curframe;                // offset 0x28
} UNPACKXAFSTATE;

int SFILTER_unpackxaf(void *pstate, int framestoup, void *psrcp, void *pdstp, int requester) {
    UNPACKXAFSTATE *pups = (UNPACKXAFSTATE *)pstate;
    int retVal;
    int framesdecoded;
    float *ptmpdst;

    ptmpdst = (float *)pdstp;
    if (pups->curframe >= pups->totalframes) {
        retVal = -1;
    } else {
        framesdecoded = pups->pDecoder->Decode(&ptmpdst, framestoup);
        pups->curframe += framesdecoded;
        ptmpdst += framesdecoded;
        if (framesdecoded < framestoup) {
            framestoup -= framesdecoded;
            framesdecoded += framestoup;
#if defined(EA_PLATFORM_WIN32) && defined(_MSC_VER)
            SNDI_memclear(ptmpdst, framestoup * sizeof(float));
#else
            memset(ptmpdst, 0, framestoup * sizeof(float));
#endif
        }
        retVal = framesdecoded;
    }
    return retVal;
}

int SFILTER_unpackgetframexaf(void *pstate) {
    UNPACKXAFSTATE *pups = (UNPACKXAFSTATE *)pstate;

    return pups->curframe;
}

void SFILTER_unpackxafrestore(void *pstate) {
    UNPACKXAFSTATE *pups = (UNPACKXAFSTATE *)pstate;

    delete pups->pDecoder;
}

void SFILTER_unpackxafinit(void *pstate, UNPACKINITPARAMS *pupip) {
    UNPACKXAFSTATE *pups = (UNPACKXAFSTATE *)pstate;

    pups->sfn.filterfn = SFILTER_unpackxaf;
    pups->sfn.restorefn = SFILTER_unpackxafrestore;
    pups->psample = (unsigned char *)pupip->psample;
    pups->totalframes = pupip->totalframes;
    pups->curframe = 0;
    pups->pDecoder = new Snd::CEAXABLKDecf();
    pupip->unpackgetframe = SFILTER_unpackgetframexaf;
    pups->pDecoder->Feed(pups->psample, pups->totalframes * sizeof(float), pups->totalframes);
}
