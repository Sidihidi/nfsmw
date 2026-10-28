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

// total size: 0xC
struct MTFSTATE {
    unsigned int shiftreg;  // offset 0x0
    unsigned int bufframes; // offset 0x4
    int doWholeInit;        // offset 0x8
};

// total size: 0xD68 (mtdecf.cpp)
struct CMTBLKDecf {
    static void *operator new(size_t size);
    static void operator delete(void *ptr);

    CMTBLKDecf();
    ~CMTBLKDecf() {}
    int Feed(void *pSampleData, int sampleDataSize, int numSamples);
    int Decode(float **pDstBuf, int numSamples);
    MTFSTATE GetState();
    void SetState(MTFSTATE *pstate, int ignoresamples);
    void SetCodecVersion(int codecversion);

    char pad0[0xD68];
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
typedef struct UNPACKMTFSTATE {
    SFILTERNODE sfn;           // offset 0x0, size 0x1C
    unsigned int totalframes;  // offset 0x1C
    unsigned int curframe;     // offset 0x20
    int platformver;           // offset 0x24
    Snd::CMTBLKDecf *pDecoder; // offset 0x28
} UNPACKMTFSTATE;

int SFILTER_unpackgetframemtf(void *pstate) {
    UNPACKMTFSTATE *pups = (UNPACKMTFSTATE *)pstate;

    return pups->curframe;
}

int SFILTER_unpackmtf(void *pstate, int framestoup, void *psrcp, void *pdstp, int requester) {
    UNPACKMTFSTATE *pups = (UNPACKMTFSTATE *)pstate;
    float *pdst;
    int retVal;
    int framesdecoded;

    pdst = (float *)pdstp;
    if (pups->curframe >= pups->totalframes) {
        retVal = -1;
    } else {
        framesdecoded = pups->pDecoder->Decode(&pdst, framestoup);
        pups->curframe += framesdecoded;
        pdst += framesdecoded;
        if (framesdecoded < framestoup) {
            framestoup -= framesdecoded;
#if defined(EA_PLATFORM_WIN32) && defined(_MSC_VER)
            SNDI_memclear(pdst, framestoup * sizeof(float));
#else
            memset(pdst, 0, framestoup * sizeof(float));
#endif
        }
        retVal = 1;
    }
    return retVal;
}

void SFILTER_unpackmtfrestore(void *pstate) {
    UNPACKMTFSTATE *pups = (UNPACKMTFSTATE *)pstate;

    delete pups->pDecoder;
}

void SFILTER_unpackmtfinit(void *pstate, UNPACKINITPARAMS *pupip) {
    UNPACKMTFSTATE *pups = (UNPACKMTFSTATE *)pstate;
    Snd::MTFSTATE mts;

    pups->sfn.filterfn = SFILTER_unpackmtf;
    pups->totalframes = pupip->totalframes;
    pups->curframe = 0;
    pups->platformver = pupip->platformver;
    pupip->unpackgetframe = SFILTER_unpackgetframemtf;
    pups->sfn.restorefn = SFILTER_unpackmtfrestore;
    pups->pDecoder = new Snd::CMTBLKDecf();
    mts = pups->pDecoder->GetState();
    mts.doWholeInit = 1;
    pups->pDecoder->SetState(&mts, 0);
#ifdef EA_PLATFORM_XENON
    // Xenon (EGM 0x8264E760) da siempre el codec 1, sin mirar platformver.
    pups->pDecoder->SetCodecVersion(1);
#else
#ifdef EA_PLATFORM_WIN32
    // PC solo da el codec 0 a platformver <= 1 (speed.exe 0x826C9F: cmp [ebx+0x24], 1; jg). Las voces de PC
    // (copspeech, platformver 2) llevan el byte de marca por trama del codec 1.
    if (pups->platformver <= 1) {
#else
    if (pups->platformver <= 2) {
#endif
        pups->pDecoder->SetCodecVersion(0);
    } else {
        pups->pDecoder->SetCodecVersion(1);
    }
#endif
    pups->pDecoder->Feed(pupip->psample, pups->totalframes * sizeof(float), pups->totalframes);
}
