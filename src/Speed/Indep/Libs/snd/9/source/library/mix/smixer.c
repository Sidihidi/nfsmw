#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"
#include <snd/sfilter.h>

// Layout reconstructed from build/GOWE69/asm and the DWARF dump of this TU.
// smixi.h (shared with the other mix/ units) carries a simplified view of the
// same object, so the full definitions live here.

typedef struct MIXCONFIG {
    int outputrate;                  // offset 0x0
    void (*chanovercallback)(int);   // offset 0x4
    unsigned char inputchannels;     // offset 0x8
    unsigned char outputchannels;    // offset 0x9
    unsigned char outputinterleaved; // offset 0xA
} MIXCONFIG;

typedef struct UNPACKINITPARAMS {
    void *psample;                 // offset 0x0
    void *ploopstart;              // offset 0x4
    int numchan;                   // offset 0x8
    int totalframes;               // offset 0xC
    int sustainstart;              // offset 0x10
    int sustainend;                // offset 0x14
    int bits16;                    // offset 0x18
    int voice;                     // offset 0x1C
    int cpu;                       // offset 0x20
    int platformver;               // offset 0x24
    int (*unpackgetframe)(void *); // offset 0x28
} UNPACKINITPARAMS;

typedef void UNPACKINITFN(void *pstate, UNPACKINITPARAMS *pparams);

// total size: 0x1834
typedef struct TIMESTRETCHSTATE {
    SFILTERNODE sfn; // offset 0x0
    char pad[0x1834 - 0x1C];
} TIMESTRETCHSTATE;

// total size: 0x3C
typedef struct RSFSTATE {
    SFILTERNODE sfn; // offset 0x0
    char pad[0x3C - 0x1C];
} RSFSTATE;

// total size: 0x28
typedef struct LPFRCSTATE {
    SFILTERNODE sfn; // offset 0x0
    char pad[0x28 - 0x1C];
} LPFRCSTATE;

// total size: 0x58
typedef struct HPFSTATE {
    SFILTERNODE sfn; // offset 0x0
    char pad[0x58 - 0x1C];
} HPFSTATE;

#ifdef EA_PLATFORM_PLAYSTATION2
// total size: 0x1C
typedef struct FT16STATE {
    SFILTERNODE sfn; // offset 0x0
} FT16STATE;
#else
// total size: 0x1C
typedef struct FT24_32STATE {
    SFILTERNODE sfn; // offset 0x0
} FT24_32STATE;
#endif

// total size: 0x5C
typedef struct SNDMIXVOICE {
    char status;                   // offset 0x0
    signed char gainchanged;       // offset 0x1
    short pad;                     // offset 0x2
    float drybusgainprev[6];       // offset 0x4
    float drybusgain[6];           // offset 0x1C
    float *wetbusgainprev;         // offset 0x34
    float *wetbusgain;             // offset 0x38
    float lastmixed;               // offset 0x3C
    SFILTERNODE *filterhead;       // offset 0x40
    int (*unpackgetframe)(void *); // offset 0x44
    void *pups;                    // offset 0x48
    TIMESTRETCHSTATE *ptss;        // offset 0x4C
    RSFSTATE *prss;                // offset 0x50
    LPFRCSTATE *plps;              // offset 0x54
    HPFSTATE *phps;                // offset 0x58
} SNDMIXVOICE;

// total size: 0x8 (PS2: 0xC)
typedef struct SNDMIXFXSTATE {
#ifdef EA_PLATFORM_PLAYSTATION2
    float **inbufUnaligned; // offset 0x0
#endif
    float **inbuf; // offset 0x0 (PS2: 0x4)
    int timeout;   // offset 0x4 (PS2: 0x8)
} SNDMIXFXSTATE;

// total size: 0x1E8
// En PS2 (snd 9.06.00-ps2, stabs de la A124) mide 0x1EC: pfxwetmix y fxaddiop detras de fxadd, el filtro de
// salida es el de 16 bits (ft16s) y no hay decode16 (supf/suplf/suppf llaman directo a decode16x87).
typedef struct SNDMIXSTATE {
    MIXCONFIG mc;                            // offset 0x0
    unsigned short outputframeincr;          // offset 0xC
    unsigned char resamplequality;           // offset 0xE
    char pad[1];                             // offset 0xF
    void (*fxadd)(int);                      // offset 0x10
#ifdef EA_PLATFORM_PLAYSTATION2
    void *pfxwetmix;                         // offset 0x14
    void (*fxaddiop)(int);                   // offset 0x18
#endif
    int *wetbusactive;                       // offset 0x14 (PS2: 0x1C)
    UNPACKINITFN *filterunpackinit[6][3];    // offset 0x18 (PS2: 0x20)
    int filterunpacksize[6][3];              // offset 0x60 (PS2: 0x68)
    float *punalignedtempbuf[2];             // offset 0xA8 (PS2: 0xB0)
    float *ptempbuf[2];                      // offset 0xB0 (PS2: 0xB8)
    float *punalignedmixbuf[6];              // offset 0xB8 (PS2: 0xC0)
    float *pmixbuf[6];                       // offset 0xD0 (PS2: 0xD8)
    float stoppedvoiceaccdry[6];             // offset 0xE8 (PS2: 0xF0)
    float *stoppedvoiceaccwet;               // offset 0x100 (PS2: 0x108)
    SFILTERNODE *outputfilterhead[6];        // offset 0x104 (PS2: 0x10C)
    LPFRCSTATE *plps[6];                     // offset 0x11C (PS2: 0x124)
#ifdef EA_PLATFORM_PLAYSTATION2
    FT16STATE ft16s[6];                      // offset 0x13C
#else
    FT24_32STATE ft24s[6];                   // offset 0x134
#endif
    SNDMIXVOICE *smv;                        // offset 0x1DC (PS2: 0x1E4)
    void (*mix)(int, float, void *, void *); // offset 0x1E0 (PS2: 0x1E8)
#ifndef EA_PLATFORM_PLAYSTATION2
    void (*decode16)(int, short *, float *); // offset 0x1E4
#endif
} SNDMIXSTATE;

namespace Snd {

void CODASetNew(void *(*pfn)(unsigned int size));
void CODASetDelete(void (*pfn)(void *pmem));

namespace Coda {

// El DWARF del original da la firma exacta: `static enum Result SetMemCpy(void
// *(*)(void *, const void *, unsigned int))`, con un Result de este ambito
// reducido al unico valor que se usa. Nosotros la declarabamos devolviendo
// (ver setmemcpy.cpp, que es quien la define)
enum Result {
    RESULT_OK = 0,
};

struct System {
    static Result SetMemCpy(void *(*pfn)(void *pdst, const void *psrc, unsigned int size));
};

} // namespace Coda
} // namespace Snd

extern "C" {

void MIXI_interpolatemix(float gain, float gaintarget, float *psrc, float *pdst);
void SNDMIX_setmasterlowpass(float leakage);
void MIX_create(MIXCONFIG *pmc);
void MIX_destroy();
void MIX_reset();
void MIX_playinit(int voice, int samplerep, int playtype, void *psample, void *ploopstart, void *ptsdata, int numchan, int totalframes, int sustainstart, int sustainend, void *pcoeftable, void *ploopstate, int platformver, int requester);
void MIX_play(int voice);
void MIX_stop(int voice);
void SNDMIX_setdrygain(int voice, int channel, float gain);
void SNDMIX_setwetgain(int voice, int bus, float gain);
void MIX_audioslice(void **poutput, int frames);

void MIX_createFxglobals();
void MIX_destroyFxglobals();
void MIXI_initunpack16();
void MIXI_initunpackxa();
void MIXI_initunpackmt();
void SFILTER_add(SFILTERNODE **pphead, SFILTERNODE *pnew);
void mixc(int count, float vol, void *psrc, void *pdst);
void decode16x87(int n, short *psrc, float *pdst);
void *memset(void *p, int c, unsigned int n);

extern SNDMIXFXSTATE sndfx;
}

#ifdef EA_PLATFORM_PLAYSTATION2
void SFILTER_ft16init(FT16STATE *ps);
#else
void SFILTER_ft24_32init(FT24_32STATE *ps);
#endif
int SFILTER_timestretchinit(TIMESTRETCHSTATE *ptss, unsigned char *psideband, int voice);

SNDMIXSTATE sndmix;
#ifdef EA_PLATFORM_PLAYSTATION2
unsigned int SNDMIXfastramfree;
void (*sndmixopenlog)();
#endif
void (*sndmixwritelog)(short **, int);
#ifdef EA_PLATFORM_PLAYSTATION2
void (*sndmixcloselog)();
#endif

void MIXI_interpolateto0(float *acc, float *pdst) {
    pdst[0] += *acc * 0.941176474f;
    pdst[1] += *acc * 0.882352948f;
    pdst[2] += *acc * 0.823529422f;
    pdst[3] += *acc * 0.764705896f;
    pdst[4] += *acc * 0.70588237f;
    pdst[5] += *acc * 0.647058845f;
    pdst[6] += *acc * 0.588235319f;
    pdst[7] += *acc * 0.529411793f;
    pdst[8] += *acc * 0.470588237f;
    pdst[9] += *acc * 0.411764711f;
    pdst[10] += *acc * 0.352941185f;
    pdst[11] += *acc * 0.294117659f;
    pdst[12] += *acc * 0.235294119f;
    pdst[13] += *acc * 0.176470593f;
    pdst[14] += *acc * 0.117647059f;
    pdst[15] += *acc * 0.0588235296f;
    *acc = 0.0f;
}

void MIXI_interpolatemix(float gain, float gaintarget, float *psrc, float *pdst) {
    float gainincr = (gaintarget - gain) * 0.0588235296f;

    pdst[0] += psrc[0] * (gain + gainincr * 1.0f);
    pdst[1] += psrc[1] * (gain + gainincr * 2.0f);
    pdst[2] += psrc[2] * (gain + gainincr * 3.0f);
    pdst[3] += psrc[3] * (gain + gainincr * 4.0f);
    pdst[4] += psrc[4] * (gain + gainincr * 5.0f);
    pdst[5] += psrc[5] * (gain + gainincr * 6.0f);
    pdst[6] += psrc[6] * (gain + gainincr * 7.0f);
    pdst[7] += psrc[7] * (gain + gainincr * 8.0f);
    pdst[8] += psrc[8] * (gain + gainincr * 9.0f);
    pdst[9] += psrc[9] * (gain + gainincr * 10.0f);
    pdst[10] += psrc[10] * (gain + gainincr * 11.0f);
    pdst[11] += psrc[11] * (gain + gainincr * 12.0f);
    pdst[12] += psrc[12] * (gain + gainincr * 13.0f);
    pdst[13] += psrc[13] * (gain + gainincr * 14.0f);
    pdst[14] += psrc[14] * (gain + gainincr * 15.0f);
    pdst[15] += psrc[15] * (gain + gainincr * 16.0f);
}

void SNDMIX_setmasterlowpass(float leakage) {
}

int SNDMIXI_volramp(SNDMIXVOICE *psmv) {
    int samplesdecoded;
    int i;

    samplesdecoded = psmv->filterhead->filterfn(psmv->filterhead, 16, sndmix.ptempbuf[0], sndmix.ptempbuf[1], 0);

    if (samplesdecoded <= 0) {
        for (i = 0; i < Snd::gMaxFxBuses; i++) {
            psmv->wetbusgainprev[i] = psmv->wetbusgain[i];
        }

        for (i = 0; i < sndmix.mc.outputchannels; i++) {
            psmv->drybusgainprev[i] = psmv->drybusgain[i];
        }

        return 0;
    }

    psmv->lastmixed = sndmix.ptempbuf[1][15];

    for (i = 0; i < Snd::gMaxFxBuses; i++) {
        if (sndmix.wetbusactive[i]) {
            if (psmv->wetbusgainprev[i] != psmv->wetbusgain[i]) {
                MIXI_interpolatemix(psmv->wetbusgainprev[i], psmv->wetbusgain[i], sndmix.ptempbuf[1], sndfx.inbuf[i]);
                psmv->wetbusgainprev[i] = psmv->wetbusgain[i];
            } else if (psmv->wetbusgain[i] != 0.0f) {
                sndmix.mix(16, psmv->wetbusgain[i], sndmix.ptempbuf[1], sndfx.inbuf[i]);
            }
        }
    }

    for (i = 0; i < sndmix.mc.outputchannels; i++) {
        if (psmv->drybusgainprev[i] != psmv->drybusgain[i]) {
            MIXI_interpolatemix(psmv->drybusgainprev[i], psmv->drybusgain[i], sndmix.ptempbuf[1], sndmix.pmixbuf[i]);
            psmv->drybusgainprev[i] = psmv->drybusgain[i];
        } else if (psmv->drybusgain[i] != 0.0f) {
            sndmix.mix(16, psmv->drybusgain[i], sndmix.ptempbuf[1], sndmix.pmixbuf[i]);
        }
    }

    return 16;
}

void *SNDI_New(unsigned int s) {
    return SNDMEMI_allocz(s);
}

void SNDI_Delete(void *p) {
    SNDMEMI_free(p);
}

void MIX_create(MIXCONFIG *pmc) {
#ifdef EA_PLATFORM_PLAYSTATION2
    unsigned int fastramaddr;
#endif
    int i = 0;
#ifdef EA_PLATFORM_PLAYSTATION2
    int pingPongBufSize = 0x1058;
#else
    int pingPongBufSize = 0xABC;
#endif

    Snd::CODASetNew(SNDI_New);
    Snd::CODASetDelete(SNDI_Delete);
    Snd::Coda::System::SetMemCpy(Snd::Util::MemCpy);

    sndmix.mc = *pmc;

    SNDSYS_entercritical();

#ifdef EA_PLATFORM_PLAYSTATION2
    // En PS2 los buferes van primero a la memoria rapida (el scratchpad) que den las opciones del sistema.
    fastramaddr = (unsigned int)sndgs.sso.set.fastramaddr;
    SNDMIXfastramfree = sndgs.sso.set.fastramsize;
#endif

    MIX_createFxglobals();

    for (i = 0; i < Snd::gMaxFxBuses; i++) {
        sndmix.wetbusactive[i] = 0;
    }

    for (i = 0; i < 2; i++) {
#ifdef EA_PLATFORM_PLAYSTATION2
        if (SNDMIXfastramfree >= pingPongBufSize) {
            SNDMIXfastramfree -= pingPongBufSize;
            sndmix.punalignedtempbuf[i] = (float *)fastramaddr;
            sndmix.ptempbuf[i] = (float *)fastramaddr;
            fastramaddr += pingPongBufSize;
        } else {
            sndmix.punalignedtempbuf[i] = (float *)SNDMEMI_allocz(pingPongBufSize);
            sndmix.ptempbuf[i] = sndmix.punalignedtempbuf[i];
        }
#else
        sndmix.punalignedtempbuf[i] = (float *)SNDMEMI_allocz(pingPongBufSize);
        sndmix.ptempbuf[i] = sndmix.punalignedtempbuf[i] + 2;
#endif

        while (((sndptrint)sndmix.ptempbuf[i] & 0x3F) != 0) {
            sndmix.ptempbuf[i] += 1;
        }
    }

    for (i = 0; i < sndmix.mc.outputchannels; i++) {
#ifdef EA_PLATFORM_PLAYSTATION2
        int bufsize = 0x840;

        if (SNDMIXfastramfree >= bufsize) {
            SNDMIXfastramfree -= bufsize;
            sndmix.punalignedmixbuf[i] = (float *)fastramaddr;
            sndmix.pmixbuf[i] = (float *)fastramaddr;
            fastramaddr += bufsize;
        } else {
            sndmix.punalignedmixbuf[i] = (float *)SNDMEMI_allocz(bufsize);
            sndmix.pmixbuf[i] = sndmix.punalignedmixbuf[i];
        }
#else
        int bufsize = 0x2C0;

        sndmix.punalignedmixbuf[i] = (float *)SNDMEMI_allocz(bufsize);
        sndmix.pmixbuf[i] = sndmix.punalignedmixbuf[i];
#endif

        while (((sndptrint)sndmix.pmixbuf[i] & 0x3F) != 0) {
            sndmix.pmixbuf[i] += 1;
        }
    }

    if (sndmix.mc.inputchannels != 0) {
        char *ptemp;

        sndmix.smv = (SNDMIXVOICE *)SNDMEMI_allocz(sndmix.mc.inputchannels * sizeof(SNDMIXVOICE) + (Snd::gMaxFxBuses * sizeof(float)) * sndmix.mc.inputchannels + (Snd::gMaxFxBuses * sizeof(float)) * sndmix.mc.inputchannels);
        memset(sndmix.smv, 0, sndmix.mc.inputchannels * sizeof(SNDMIXVOICE) + (Snd::gMaxFxBuses * sizeof(float)) * sndmix.mc.inputchannels + (Snd::gMaxFxBuses * sizeof(float)) * sndmix.mc.inputchannels);

        ptemp = (char *)sndmix.smv + sndmix.mc.inputchannels * sizeof(SNDMIXVOICE);

        for (i = 0; i < sndmix.mc.inputchannels; i++) {
            sndmix.smv[i].wetbusgainprev = (float *)ptemp;
            ptemp += Snd::gMaxFxBuses * sizeof(float);
            sndmix.smv[i].wetbusgain = (float *)ptemp;
            ptemp += Snd::gMaxFxBuses * sizeof(float);
        }
    }

    SNDSYS_leavecritical();

    MIXI_initunpack16();
    MIXI_initunpackxa();
    MIXI_initunpackmt();

    MIX_reset();

    for (i = 0; i < sndmix.mc.outputchannels; i++) {
        sndmix.outputfilterhead[i] = 0;
#ifdef EA_PLATFORM_PLAYSTATION2
        SFILTER_ft16init(&sndmix.ft16s[i]);
        sndmix.ft16s[i].sfn.priority = 0;
        sndmix.ft16s[i].sfn.requester = 0;
        SFILTER_add(&sndmix.outputfilterhead[i], &sndmix.ft16s[i].sfn);
        memset(sndmix.pmixbuf[i], 0, 0x800);
#else
        SFILTER_ft24_32init(&sndmix.ft24s[i]);
        sndmix.ft24s[i].sfn.priority = 0;
        sndmix.ft24s[i].sfn.requester = 0;
        SFILTER_add(&sndmix.outputfilterhead[i], &sndmix.ft24s[i].sfn);
        memset(sndmix.pmixbuf[i], 0, 0x280);
#endif
    }
}

void MIX_destroy() {
    int i;

    SNDSYS_entercritical();

    MIX_destroyFxglobals();

    if (sndmix.smv != 0) {
        SNDMEMI_free(sndmix.smv);
        sndmix.smv = 0;
    }

    for (i = 0; i < 2; i++) {
#ifdef EA_PLATFORM_PLAYSTATION2
        // lo que salio de la memoria rapida (0x70000000, el scratchpad) no se libera
        if (((sndptruint)sndmix.punalignedtempbuf[i] & 0x70000000) == 0x70000000) {
            sndmix.punalignedtempbuf[i] = 0;
        } else
#endif
        if (sndmix.punalignedtempbuf[i] != 0) {
            SNDMEMI_free(sndmix.punalignedtempbuf[i]);
            sndmix.punalignedtempbuf[i] = 0;
        }
    }

    for (i = 0; i < sndmix.mc.outputchannels; i++) {
#ifdef EA_PLATFORM_PLAYSTATION2
        if (((sndptruint)sndmix.punalignedmixbuf[i] & 0x70000000) == 0x70000000) {
            sndmix.punalignedmixbuf[i] = 0;
        } else
#endif
        if (sndmix.punalignedmixbuf[i] != 0) {
            SNDMEMI_free(sndmix.punalignedmixbuf[i]);
            sndmix.punalignedmixbuf[i] = 0;
        }
    }

    SNDMIX_setmasterlowpass(2.0f);

    SNDSYS_leavecritical();
}

void MIX_reset() {
    sndmix.mix = mixc;
#ifdef EA_PLATFORM_PLAYSTATION2
    sndmix.outputframeincr = 1024;
#else
    sndmix.decode16 = decode16x87;
    sndmix.outputframeincr = 320;
#endif
}

#ifdef EA_PLATFORM_PLAYSTATION2
extern "C" void MIX_setresamplequality(int quality) {
    sndmix.resamplequality = quality;
}
#endif

void MIX_playinit(int voice, int samplerep, int playtype, void *psample, void *ploopstart, void *ptsdata, int numchan, int totalframes, int sustainstart, int sustainend, void *pcoeftable, void *ploopstate, int platformver, int requester) {
    UNPACKINITPARAMS upip;
    SNDMIXVOICE *psmv = &sndmix.smv[voice];
    int upindex = -1;
    int unpackType = 0;
    int err;

    psmv->filterhead = 0;
    psmv->pups = 0;
    psmv->prss = 0;
    psmv->plps = 0;
    psmv->phps = 0;
    psmv->ptss = 0;

    upip.bits16 = 1;
    upip.unpackgetframe = 0;

#ifdef EA_PLATFORM_PLAYSTATION2
    if (samplerep == 8) {
#else
    if (samplerep == 7) {
#endif
        upindex = 0;
    } else if (samplerep == 10) {
        upindex = 1;
    } else if (samplerep == 4) {
        upindex = 2;
    } else if (samplerep == 22) {
        upindex = 3;
    } else if (samplerep == 16) {
        upindex = 4;
    } else if (samplerep == 23) {
        upindex = 5;
    }

    if (playtype == 1) {
        if (sustainend > 0) {
            unpackType = 1;
        }
    } else if (playtype == 0) {
        unpackType = 2;
    }

    if (sndmix.filterunpackinit[upindex][unpackType] != 0) {
        psmv->pups = SNDMEMI_allocz(sndmix.filterunpacksize[upindex][unpackType]);

        upip.psample = psample;
        upip.ploopstart = ploopstart;
        upip.numchan = numchan;
        upip.platformver = platformver;
        upip.totalframes = totalframes;
        upip.sustainstart = sustainstart;
        upip.sustainend = sustainend;
        upip.voice = voice;
        upip.cpu = sndgs.cputypes;

        ((SFILTERNODE *)psmv->pups)->restorefn = 0;
        ((SFILTERNODE *)psmv->pups)->priority = 240;
        ((SFILTERNODE *)psmv->pups)->requester = requester;

        sndmix.filterunpackinit[upindex][unpackType](psmv->pups, &upip);

        psmv->unpackgetframe = upip.unpackgetframe;

        SFILTER_add(&psmv->filterhead, (SFILTERNODE *)psmv->pups);
    }

    if (ptsdata != 0) {
        if (playtype != 0) {
            voice = -1;
        }

        psmv->ptss = (TIMESTRETCHSTATE *)SNDMEMI_allocz(sizeof(TIMESTRETCHSTATE));
        psmv->ptss->sfn.restorefn = 0;
        psmv->ptss->sfn.priority = 200;
        psmv->ptss->sfn.requester = requester;

        err = SFILTER_timestretchinit(psmv->ptss, (unsigned char *)ptsdata, voice);

        SFILTER_add(&psmv->filterhead, &psmv->ptss->sfn);
    }

    psmv->status = 1;
}

void MIX_play(int voice) {
    SNDMIXVOICE *psmv = &sndmix.smv[voice];
    int i;

    psmv->gainchanged = 0;
    psmv->lastmixed = 0.0f;

    for (i = 0; i < Snd::gMaxFxBuses; i++) {
        psmv->wetbusgainprev[i] = psmv->wetbusgain[i];
    }

    for (i = 0; i < sndmix.mc.outputchannels; i++) {
        psmv->drybusgainprev[i] = psmv->drybusgain[i];
    }

    psmv->status = 2;
}

void MIXI_stop(int voice) {
    MIX_stop(voice);
    sndmix.mc.chanovercallback(voice);
}

void MIX_stop(int voice) {
    SNDMIXVOICE *psmv = &sndmix.smv[voice];
    SFILTERNODE *psfn;
    int i;

    for (i = 0; i < Snd::gMaxFxBuses; i++) {
        sndmix.stoppedvoiceaccwet[i] += psmv->wetbusgainprev[i] * psmv->lastmixed;
    }

    for (i = 0; i < sndmix.mc.outputchannels; i++) {
        sndmix.stoppedvoiceaccdry[i] += psmv->drybusgainprev[i] * psmv->lastmixed;
    }

    do {
        if (psmv->filterhead->restorefn != 0) {
            psmv->filterhead->restorefn(psmv->filterhead);
        }

        psfn = psmv->filterhead->pfnnext;
        SNDMEMI_free(psmv->filterhead);
        psmv->filterhead = psfn;
    } while (psfn != 0);

    psmv->status = 0;
}

void SNDMIX_setdrygain(int voice, int channel, float gain) {
    sndmix.smv[voice].drybusgain[channel] = gain;
    sndmix.smv[voice].gainchanged = 1;
}

void SNDMIX_setwetgain(int voice, int bus, float gain) {
    sndmix.smv[voice].wetbusgain[bus] = gain;
    sndmix.smv[voice].gainchanged = 1;
}

void MIX_audioslice(void **poutput, int frames) {
    SNDMIXVOICE *psmv;
    int framesnotramped;
    int framesramped;
    int framestomix;
    int voice;
    int i;

    for (i = 0; i < Snd::gMaxFxBuses; i++) {
        if (sndmix.wetbusactive[i]) {
            if (sndmix.stoppedvoiceaccwet[i] != 0.0f) {
                MIXI_interpolateto0(&sndmix.stoppedvoiceaccwet[i], sndfx.inbuf[i]);
            }
        }
    }

    for (i = 0; i < sndmix.mc.outputchannels; i++) {
        memset(sndmix.pmixbuf[i], 0, frames * sizeof(float));

        if (sndmix.stoppedvoiceaccdry[i] != 0.0f) {
            MIXI_interpolateto0(&sndmix.stoppedvoiceaccdry[i], sndmix.pmixbuf[i]);
        }
    }

    for (voice = 0; voice < sndmix.mc.inputchannels; voice++) {
        psmv = &sndmix.smv[voice];

        if (psmv->status != 2) {
            continue;
        }

        if (psmv->gainchanged) {
            framesramped = SNDMIXI_volramp(psmv);
            psmv->gainchanged = 0;
            framesnotramped = frames - framesramped;
        } else {
            framesramped = 0;
            framesnotramped = frames;
        }

        if (framesnotramped == 0) {
            continue;
        }

        framestomix = psmv->filterhead->filterfn(psmv->filterhead, framesnotramped, sndmix.ptempbuf[0], sndmix.ptempbuf[1], 0);

        if (framestomix < 0) {
            MIXI_stop(voice);
        } else if (framestomix != 0) {
            psmv->lastmixed = sndmix.ptempbuf[1][framestomix - 1];

            for (i = 0; i < sndmix.mc.outputchannels; i++) {
                if (psmv->drybusgain[i] != 0.0f) {
                    sndmix.mix(framestomix, psmv->drybusgain[i], sndmix.ptempbuf[1], sndmix.pmixbuf[i] + framesramped);
                }
            }

            for (i = 0; i < Snd::gMaxFxBuses; i++) {
                if (sndmix.wetbusactive[i]) {
                    if (psmv->wetbusgain[i] != 0.0f) {
                        sndmix.mix(framestomix, psmv->wetbusgain[i], sndmix.ptempbuf[1], sndfx.inbuf[i] + framesramped);
                        sndfx.timeout = 0;
                    }
                }
            }
        }
    }

    if (sndmix.fxadd != 0) {
        sndmix.fxadd(frames);
    }
#ifdef EA_PLATFORM_PLAYSTATION2
    if (sndmix.fxaddiop != 0) {
        sndmix.fxaddiop(frames);
    }
#endif

    for (i = 0; i < sndmix.mc.outputchannels; i++) {
        if (sndmix.plps[i] != 0) {
            sndmix.plps[i]->sfn.filterfn(sndmix.plps[i], frames, sndmix.pmixbuf[i], sndmix.ptempbuf[0], 0);
            sndmix.outputfilterhead[i]->filterfn(sndmix.outputfilterhead[i], frames, sndmix.ptempbuf[0], poutput[i], 0);
        } else {
            sndmix.outputfilterhead[i]->filterfn(sndmix.outputfilterhead[i], frames, sndmix.pmixbuf[i], poutput[i], 0);
        }
    }

#ifdef EA_PLATFORM_PLAYSTATION2
    if (sndgs.logmixing || sndgs.logcallbackfn != 0) {
#else
    if (sndgs.logcallbackfn != 0) {
#endif
        sndmixwritelog((short **)poutput, frames);
    }
}

#ifdef EA_PLATFORM_PLAYSTATION2
extern "C" void MIX_audio(void **poutputpassed, int frames) {
    void *poutput[6];
    int i;

    for (i = 0; i < sndmix.mc.outputchannels; i++) {
        poutput[i] = poutputpassed[i];
    }

    while (frames > 0) {
        MIX_audioslice(poutput, frames > 512 ? 512 : frames);

        for (i = 0; i < sndmix.mc.outputchannels; i++) {
            poutput[i] = (char *)poutput[i] + sndmix.outputframeincr;
        }

        frames -= 512;
    }
}
#endif
