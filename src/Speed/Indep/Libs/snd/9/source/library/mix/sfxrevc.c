#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"
#include "snd/sfilter.h"
#include <cstring>

// ---------------------------------------------------------------------------
// Local declarations.  smixi.h / slinklist.h are shared with units that already
// match, so everything this unit needs that is missing from them is declared
// here instead.
// ---------------------------------------------------------------------------


// total size: 0x38
struct MIXFXFILTHEADER {
    char id[2];                   // offset 0x0
    unsigned char ver;            // offset 0x2
    char pad1;                    // offset 0x3
    char totalfilesize[4];        // offset 0x4
    char numfiltersstruct[6][4];  // offset 0x8
    char numconnectsstruct[6][4]; // offset 0x20
};

// total size: 0x84
struct BUSNODE_INSTANCE {
    CListNode ln;               // offset 0x0
    MIXFXFILTHEADER headerInfo; // offset 0x4
    SFILTERNODE **pfilterlist;  // offset 0x3C
    void **phead;               // offset 0x40
    int lastoutputchannels;     // offset 0x44
    int validstructoutputs;     // offset 0x48
    int indexToSrcModule;       // offset 0x4C
    float outputLevelcur[6];    // offset 0x50
    float outputLevelnew[6];    // offset 0x68
    int outputLevelChanged;     // offset 0x80
};

// total size: 0xC
struct MIXFXCONNECTION {
    char node1ID[4];           // offset 0x0
    char node2ID[4];           // offset 0x4
    unsigned char node1output; // offset 0x8
    unsigned char node2input;  // offset 0x9
    char pad[2];               // offset 0xA
};

// total size: 0x14
struct MIXFXFILTERMOD {
    unsigned char type; // offset 0x0
    unsigned char head; // offset 0x1
    char pad[2];        // offset 0x2
    char params[4][4];  // offset 0x4
};

// total size: 0x4
struct MIXFXMODSOURCE {
    unsigned char type; // offset 0x0
    unsigned char head; // offset 0x1
    char pad[2];        // offset 0x2
};

// total size: 0x8
struct MIXFXMODLOWRC {
    unsigned char type; // offset 0x0
    unsigned char head; // offset 0x1
    char pad[2];        // offset 0x2
    char params[2][2];  // offset 0x4
};

// total size: 0x4
struct MIXFXMODLOWFIR {
    unsigned char type; // offset 0x0
    unsigned char head; // offset 0x1
    char param[2];      // offset 0x2
};

// total size: 0x4
struct MIXFXMODHIGHFIR {
    unsigned char type; // offset 0x0
    unsigned char head; // offset 0x1
    char param[2];      // offset 0x2
};

// total size: 0x8
struct MIXFXMODBANDFIR {
    unsigned char type; // offset 0x0
    unsigned char head; // offset 0x1
    char pad[2];        // offset 0x2
    char params[2][2];  // offset 0x4
};

// total size: 0x8
struct MIXFXMODRESON {
    unsigned char type; // offset 0x0
    unsigned char head; // offset 0x1
    char params[3][2];  // offset 0x2
};

// total size: 0x4
struct MIXFXMODAMP {
    unsigned char type; // offset 0x0
    unsigned char head; // offset 0x1
    char param[2];      // offset 0x2
};

// total size: 0x8
struct MIXFXMODECHO {
    unsigned char type; // offset 0x0
    unsigned char head; // offset 0x1
    char params[2][2];  // offset 0x2
    char pad[2];        // offset 0x6
};

// total size: 0xC
struct LPFRC {
    float accumulator; // offset 0x0
    float leakage;     // offset 0x4
    float inverse;     // offset 0x8
};

// total size: 0x3C; en PS2 (snd 9.06.00-ps2) 0x100, con los coeficientes para la VU0 delante (ver sfir8.c)
struct SNDFIRSTATE {
#ifdef EA_PLATFORM_PLAYSTATION2
    float VU0coef[12][4] __attribute__((aligned(16))); // offset 0x0
#endif
    float history[8]; // offset 0x0 (PS2: 0xC0)
    float coef[5];    // offset 0x20 (PS2: 0xE0)
    float cutLow;     // offset 0x34 (PS2: 0xF4)
    float cutHigh;    // offset 0x38 (PS2: 0xF8)
#ifdef EA_PLATFORM_PLAYSTATION2
    signed char resetVU0coef; // offset 0xFC
    char pad[3];              // offset 0xFD
#endif
};

// total size: 0x20
struct SRCSTATE {
    SFILTERNODE sfn; // offset 0x0
    float *pdata;    // offset 0x1C
};

// total size: 0x28
struct LPFRCSTATE {
    SFILTERNODE sfn; // offset 0x0
    LPFRC lpf;       // offset 0x1C
};

// total size: 0x58
struct LPFSTATE {
    SFILTERNODE sfn; // offset 0x0
    SNDFIRSTATE fir; // offset 0x1C
};

// total size: 0x58
struct HPFSTATE {
    SFILTERNODE sfn; // offset 0x0
    SNDFIRSTATE fir; // offset 0x1C
};

// total size: 0x58
struct BPFSTATE {
    SFILTERNODE sfn; // offset 0x0
    SNDFIRSTATE fir; // offset 0x1C
};

// total size: 0x40
struct RESONSTATE {
    SFILTERNODE sfn; // offset 0x0
    int samplerate;  // offset 0x1C
    float resonfreq; // offset 0x20
    float bw;        // offset 0x24
    float r;         // offset 0x28
    float costheta;  // offset 0x2C
    float a0;        // offset 0x30
    float xn;        // offset 0x34
    float ynminus1;  // offset 0x38
    float ynminus2;  // offset 0x3C
};

// total size: 0x20
struct AMPLFSTATE {
    SFILTERNODE sfn; // offset 0x0
    float gain;      // offset 0x1C
};

// total size: 0x24
struct MIXERSTATE {
    SFILTERNODE sfn;   // offset 0x0
    float *ptempbuf;   // offset 0x1C
    int tempbufframes; // offset 0x20
};

// total size: 0x34; en PS2 (snd 9.06.00-ps2) 0x40: fblevels alineado a 16, en 0x30
struct ECHOSTATE {
    SFILTERNODE sfn;           // offset 0x0
    float *pdelaybuf;          // offset 0x1C
    float *punaligneddelaybuf; // offset 0x20
    int sampleindex;           // offset 0x24
    int bufsamples;            // offset 0x28
#ifdef EA_PLATFORM_PLAYSTATION2
    float fblevels __attribute__((aligned(16))); // offset 0x30
#else
    float fblevels;            // offset 0x2C
#endif
    float fbneglevels;         // offset 0x30 (PS2: 0x34)
};

// total size: 0x28
struct SPLITSTATE {
    SFILTERNODE sfn; // offset 0x0
    void *pdatabuf;  // offset 0x1C
    int sizebuf;     // offset 0x20
    short mustempty; // offset 0x24
    short isempty;   // offset 0x26
};

#ifdef EA_PLATFORM_PLAYSTATION2
// total size: 0x1C
struct FT16STATE {
    SFILTERNODE sfn; // offset 0x0
};
#else
// total size: 0x1C
struct FT24_32STATE {
    SFILTERNODE sfn; // offset 0x0
};
#endif

// total size: 0xC
struct MIXCONFIG {
    int outputrate;                  // offset 0x0
    void (*chanovercallback)(int);   // offset 0x4
    unsigned char inputchannels;     // offset 0x8
    unsigned char outputchannels;    // offset 0x9
    unsigned char outputinterleaved; // offset 0xA
};

typedef void UNPACKINITFN(void *, void *);

// total size: 0x1E8; en PS2 0x1EC (ver smixer.c)
struct SNDMIXSTATE {
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
    void *smv;                               // offset 0x1DC (PS2: 0x1E4)
    void (*mix)(int, float, void *, void *); // offset 0x1E0 (PS2: 0x1E8)
#ifndef EA_PLATFORM_PLAYSTATION2
    void (*decode16)(int, short *, float *); // offset 0x1E4
#endif
};

// total size: 0x8; en PS2 0xC, con los buferes sin alinear delante
struct SNDMIXFXSTATE {
#ifdef EA_PLATFORM_PLAYSTATION2
    float **inbufUnaligned; // offset 0x0
#endif
    float **inbuf; // offset 0x0 (PS2: 0x4)
    int timeout;   // offset 0x4 (PS2: 0x8)
};

extern SNDMIXSTATE sndmix;

extern "C" {
void MIXI_interpolatemix(float gain, float gaintarget, float *psrc, float *pdst);
void SFILTER_connect(SFILTERNODE *pnode1, SFILTERNODE *pnode2, int node1output, int node2input);
}

void SFILTER_initSOURCE(SRCSTATE *psrc, void *pdata);
void SFILTER_createSOURCE(SRCSTATE *psrc);
void SFILTER_createLPFRC(LPFRCSTATE *plpfrc);
void SFILTER_modifyLPFRC(LPFRCSTATE *plpfrc, int *pparams);
void SFILTER_createLPFFIR8(LPFSTATE *plpf);
void SFILTER_modifyLPFFIR8(LPFSTATE *plpf, int *pparams);
void SFILTER_createHPFFIR8(HPFSTATE *phpf);
void SFILTER_modifyHPFFIR8(HPFSTATE *phpf, int *pparams);
void SFILTER_createBPFFIR8(BPFSTATE *pbpf);
void SFILTER_modifyBPFFIR8(BPFSTATE *pbpf, int *pparams);
void SFILTER_createRESON(RESONSTATE *pres);
void SFILTER_modifyRESON(RESONSTATE *pres, int *pparams);
void SFILTER_createAMPLF(AMPLFSTATE *pamp);
void SFILTER_modifyAMPLF(AMPLFSTATE *pamp, int *pparams);
void SFILTER_createECHO(ECHOSTATE *pecho);
void SFILTER_modifyECHO(ECHOSTATE *pecho, int *pparams);
void SFILTER_createMIX(MIXERSTATE *pmix);
void SFILTER_createSPLIT(SPLITSTATE *psplit);

extern "C" void SNDMIXI_modlapifxadd(int frames);
void SNDMIXI_restorefx2(int fxBusIndex, BUSNODE_INSTANCE *pBusNodeInstance);
void SNDMIXI_initfx(int samplerate, void *pFxFile, int fxBusIndex, BUSNODE_INSTANCE *pBusNodeInstance);

/* El objeto original alinea .data y .bss a 8; sin ello el enlazador coloca
 * esta unidad en otro hueco y desplaza SNDDRV_dolbypl2balances. */
CListStack *fxBusesLinkList;

signed char tablecopyto[6][6][6][6] = {
    {
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, 2, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, 2, 3, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, 2, 3, 4, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, 2, 3, 4, 5, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
    },
    {
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {3, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, 2, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {4, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, 2, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {4, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
    },
    {
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {3, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {2, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {4, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {2, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {4, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
    },
    {
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
    },
    {
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
    },
    {
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
        {
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
            {-1, -1, -1, -1, -1, -1},
        },
    },
};

unsigned char mapchannel[6][6][6] = {
    {
        {1, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0},
    },
    {
        {0, 0, 0, 0, 0, 0},
        {1, 2, 0, 0, 0, 0},
        {1, 3, 0, 0, 0, 0},
        {1, 3, 0, 0, 0, 0},
        {1, 4, 0, 0, 0, 0},
        {1, 4, 0, 0, 0, 0},
    },
    {
        {0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},
        {1, 2, 3, 0, 0, 0},
        {1, 0, 3, 0, 0, 0},
        {2, 1, 4, 0, 0, 0},
        {2, 1, 4, 0, 0, 0},
    },
    {
        {0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},
        {1, 2, 3, 4, 0, 0},
        {1, 3, 4, 5, 0, 0},
        {1, 3, 4, 5, 0, 0},
    },
    {
        {0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},
        {1, 2, 3, 4, 5, 0},
        {1, 2, 3, 4, 5, 0},
    },
    {
        {0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},
        {1, 2, 3, 4, 5, 6},
    },
};

// Los tamanos de los estados de filtro: en PS2 LPF/HPF/BPF miden 0x120 y ECHO 0x40 (el .data de la A124)
int filtersizetable[10] = { sizeof(SRCSTATE),  sizeof(LPFRCSTATE), sizeof(LPFSTATE),   sizeof(HPFSTATE),
                            sizeof(BPFSTATE),  sizeof(RESONSTATE), sizeof(AMPLFSTATE), sizeof(ECHOSTATE),
                            sizeof(MIXERSTATE), sizeof(SPLITSTATE) };

int filtermodsizetable[10] = { 4, 8, 4, 4, 8, 8, 4, 8, 4, 4 };

SNDMIXFXSTATE sndfx;

MIXFXFILTERMOD *modlist;
MIXFXCONNECTION *conlist;

static int fxGlobalsCreated = 0;

#ifdef EA_PLATFORM_PLAYSTATION2
extern "C" void MIX_setwetbuffer(void *pbuf) {
    sndmix.pfxwetmix = pbuf;
}
#endif

extern "C" void *MIX_getwetbuffer(int fxBusIndex) {
    return sndfx.inbuf[fxBusIndex];
}

int remap(int validstructoutputs, int wantedoutputchannels, int validheadvalue) {
    int temp;

    temp = mapchannel[validstructoutputs - 1][wantedoutputchannels - 1][validheadvalue - 1];

    return temp;
}

extern "C" void SNDMIXI_modlapifxadd(int frames) {
    float *psrc;
    float *pdst;
    int numfilts;
    int i;
    int j;
    int k;
    int processed[6] = { 0 };
    signed char *copyindex;
    signed char *pcopyto;
    BUSNODE_INSTANCE *pBusNodeInstance;
    CListNode *pNextNode;

    sndfx.timeout++;
    psrc = sndmix.ptempbuf[0];
    pdst = sndmix.ptempbuf[1];
#ifdef EA_PLATFORM_PLAYSTATION2
    if (sndfx.timeout >= 3000) {
        return;
    }
#endif

    for (k = 0; k < Snd::gMaxFxBuses; k++) {
        if (!fxBusesLinkList[k].IsEmpty()) {
            pBusNodeInstance = (BUSNODE_INSTANCE *)fxBusesLinkList[k].GetHead();
            while (pBusNodeInstance != NULL) {
                pNextNode = pBusNodeInstance->ln.GetNext();
                memset(processed, 0, sizeof(processed));
                numfilts = SNDI_getb(
                    pBusNodeInstance->headerInfo.numfiltersstruct[pBusNodeInstance->validstructoutputs - 1], 4
                );
                if (numfilts) {
                    SFILTER_initSOURCE(
                        (SRCSTATE *)pBusNodeInstance->pfilterlist[pBusNodeInstance->indexToSrcModule],
                        sndfx.inbuf[k]
                    );
                    for (i = 0; i < sndmix.mc.outputchannels; i++) {
                        if (!processed[i] && pBusNodeInstance->phead[i + 1] != NULL) {
                            ((SFILTERNODE *)pBusNodeInstance->phead[i + 1])
                                ->filterfn(pBusNodeInstance->phead[i + 1], frames, psrc, pdst, i + 1);
                            if (pBusNodeInstance->outputLevelChanged == 1) {
                                MIXI_interpolatemix(
                                    pBusNodeInstance->outputLevelcur[i], pBusNodeInstance->outputLevelnew[i], pdst,
                                    sndmix.pmixbuf[i]
                                );
                                sndmix.mix(
                                    frames - 16, pBusNodeInstance->outputLevelnew[i], pdst + 16,
                                    sndmix.pmixbuf[i] + 16
                                );
                            } else {
                                sndmix.mix(frames, pBusNodeInstance->outputLevelnew[i], pdst, sndmix.pmixbuf[i]);
                            }
                            processed[i] = 1;
                            pcopyto = &tablecopyto[pBusNodeInstance->validstructoutputs - 1][sndmix.mc.outputchannels - 1][0][0];
                            copyindex = pcopyto + i * 6;
                            for (j = 0; j < sndmix.mc.outputchannels; j++) {
                                if (*copyindex >= 0) {
                                    if (pBusNodeInstance->outputLevelChanged == 1) {
                                        MIXI_interpolatemix(
                                            pBusNodeInstance->outputLevelcur[i], pBusNodeInstance->outputLevelnew[i],
                                            pdst, sndmix.pmixbuf[*copyindex]
                                        );
                                        sndmix.mix(
                                            frames - 16, pBusNodeInstance->outputLevelnew[i], pdst + 16,
                                            sndmix.pmixbuf[*copyindex] + 16
                                        );
                                    } else {
                                        sndmix.mix(
                                            frames, pBusNodeInstance->outputLevelnew[j], pdst,
                                            sndmix.pmixbuf[*copyindex]
                                        );
                                    }
                                    processed[*copyindex] = 1;
                                }

                                copyindex++;
                            }
                            if (pBusNodeInstance->outputLevelChanged == 1) {
                                pBusNodeInstance->outputLevelcur[i] = pBusNodeInstance->outputLevelnew[i];
                            }
                        }
                    }
                }
                pBusNodeInstance->outputLevelChanged = 0;
                pBusNodeInstance = (BUSNODE_INSTANCE *)pNextNode;
            }
            memset(sndfx.inbuf[k], 0, frames * sizeof(float));
        }
    }
}

void SNDMIXI_restorefx2(int fxBusIndex, BUSNODE_INSTANCE *pBusNodeInstance) {
    int numfilt;
    int i;
    int j;

    if (sndmix.wetbusactive[fxBusIndex]) {
        numfilt = SNDI_getb(
            pBusNodeInstance->headerInfo.numfiltersstruct[pBusNodeInstance->lastoutputchannels - 1], 4
        );
        if (numfilt) {
            for (i = 0; i < numfilt; i++) {
                if (pBusNodeInstance->pfilterlist[i]->restorefn != NULL) {
                    pBusNodeInstance->pfilterlist[i]->restorefn(pBusNodeInstance->pfilterlist[i]);
                    pBusNodeInstance->pfilterlist[i]->restorefn = NULL;
                }
                if (pBusNodeInstance->pfilterlist[i] != NULL) {
                    SNDMEMI_free(pBusNodeInstance->pfilterlist[i]);
                    pBusNodeInstance->pfilterlist[i] = NULL;
                }
            }
        }
        if (pBusNodeInstance->pfilterlist != NULL) {
            SNDMEMI_free(pBusNodeInstance->pfilterlist);
            pBusNodeInstance->pfilterlist = NULL;
        }
        for (i = 0; i < 6; i++) {
            if (pBusNodeInstance->phead[i] != NULL) {
                pBusNodeInstance->phead[i] = NULL;
            }
        }
        if (pBusNodeInstance->phead != NULL) {
            SNDMEMI_free(pBusNodeInstance->phead);
            pBusNodeInstance->phead = NULL;
        }
#ifdef EA_PLATFORM_PLAYSTATION2
        memset(sndfx.inbuf[fxBusIndex], 0, 0x800);
#else
        memset(sndfx.inbuf[fxBusIndex], 0, 0x280);
#endif
    }
}

void SNDMIXI_initfx(int samplerate, void *pFxFile, int fxBusIndex, BUSNODE_INSTANCE *pBusNodeInstance) {
    char *pcurModule;
    MIXFXCONNECTION *pcurcon;
    SRCSTATE *psrc;
    LPFRCSTATE *plpfrc;
    LPFSTATE *plpf;
    HPFSTATE *phpf;
    BPFSTATE *pbpf;
    RESONSTATE *pres;
    AMPLFSTATE *pamp;
    MIXERSTATE *pmix;
    ECHOSTATE *pecho;
    SPLITSTATE *psplit;
    MIXFXMODLOWRC *plowrcmod;
    MIXFXMODLOWFIR *plowfirmod;
    MIXFXMODHIGHFIR *phighfirmod;
    MIXFXMODBANDFIR *pbandfirmod;
    MIXFXMODRESON *presonmod;
    MIXFXMODAMP *ampmod;
    MIXFXMODECHO *pechomod;
    int params[4];
    int i;
    int j;
    int k;
    int numfilt;
    unsigned char tempHeadVal;

    pBusNodeInstance->phead = (void **)SNDMEMI_allocz(7 * sizeof(void *));
    memset(pBusNodeInstance->phead, 0, 7 * sizeof(void *));

    k = pBusNodeInstance->validstructoutputs - 1;
    numfilt = SNDI_getb(pBusNodeInstance->headerInfo.numfiltersstruct[k], 4);
    pBusNodeInstance->pfilterlist = (SFILTERNODE **)SNDMEMI_allocz(numfilt * sizeof(SFILTERNODE *));
    pcurModule = (char *)modlist;

    for (i = 0; i < numfilt; i++) {
        pBusNodeInstance->pfilterlist[i] = (SFILTERNODE *)SNDMEMI_allocz(filtersizetable[((MIXFXMODSOURCE *)pcurModule)->type]);
        memset(pBusNodeInstance->pfilterlist[i], 0, filtersizetable[((MIXFXMODSOURCE *)pcurModule)->type]);
        memset(params, 0, sizeof(params));

        switch (((MIXFXMODSOURCE *)pcurModule)->type) {
        case 0:
            psrc = (SRCSTATE *)pBusNodeInstance->pfilterlist[i];
            SFILTER_createSOURCE(psrc);
            break;

        case 1:
            plowrcmod = (MIXFXMODLOWRC *)pcurModule;
            plpfrc = (LPFRCSTATE *)pBusNodeInstance->pfilterlist[i];
            params[0] = SNDI_getb(plowrcmod->params[0], 2) << 8;
            params[1] = samplerate << 8;
            params[2] = SNDI_getb(plowrcmod->params[1], 2);
            SFILTER_createLPFRC(plpfrc);
            SFILTER_modifyLPFRC(plpfrc, params);
            break;

        case 2:
            plowfirmod = (MIXFXMODLOWFIR *)pcurModule;
            plpf = (LPFSTATE *)pBusNodeInstance->pfilterlist[i];
            params[0] = SNDI_getb(plowfirmod->param, 2) << 8;
            params[1] = samplerate << 8;
            SFILTER_createLPFFIR8(plpf);
            SFILTER_modifyLPFFIR8(plpf, params);
            break;

        case 3:
            phighfirmod = (MIXFXMODHIGHFIR *)pcurModule;
            phpf = (HPFSTATE *)pBusNodeInstance->pfilterlist[i];
            params[0] = SNDI_getb(phighfirmod->param, 2) << 8;
            params[1] = samplerate << 8;
            SFILTER_createHPFFIR8(phpf);
            SFILTER_modifyHPFFIR8(phpf, params);
            break;

        case 4:
            pbandfirmod = (MIXFXMODBANDFIR *)pcurModule;
            pbpf = (BPFSTATE *)pBusNodeInstance->pfilterlist[i];
            params[0] = SNDI_getb(pbandfirmod->params[0], 2) << 8;
            params[1] = SNDI_getb(pbandfirmod->params[1], 2) << 8;
            params[2] = samplerate << 8;
            SFILTER_createBPFFIR8(pbpf);
            SFILTER_modifyBPFFIR8(pbpf, params);
            break;

        case 5:
            presonmod = (MIXFXMODRESON *)pcurModule;
            pres = (RESONSTATE *)pBusNodeInstance->pfilterlist[i];
            params[0] = SNDI_getb(presonmod->params[0], 2) << 8;
            params[1] = samplerate << 8;
            params[2] = SNDI_getb(presonmod->params[1], 2) << 8;
            params[3] = SNDI_getb(presonmod->params[2], 2);
            SFILTER_createRESON(pres);
            SFILTER_modifyRESON(pres, params);
            break;

        case 6:
            ampmod = (MIXFXMODAMP *)pcurModule;
            pamp = (AMPLFSTATE *)pBusNodeInstance->pfilterlist[i];
            params[0] = SNDI_getb(ampmod->param, 2);
            SFILTER_createAMPLF(pamp);
            SFILTER_modifyAMPLF(pamp, params);
            break;

        case 7:
            pechomod = (MIXFXMODECHO *)pcurModule;
            pecho = (ECHOSTATE *)pBusNodeInstance->pfilterlist[i];
            params[0] = SNDI_getb(pechomod->params[0], 2);
            params[1] = samplerate << 8;
            params[2] = SNDI_getb(pechomod->params[1], 2) << 8;
            SFILTER_createECHO(pecho);
            SFILTER_modifyECHO(pecho, params);
            break;

        case 8:
            pmix = (MIXERSTATE *)pBusNodeInstance->pfilterlist[i];
            SFILTER_createMIX(pmix);
            break;

        case 9:
            psplit = (SPLITSTATE *)pBusNodeInstance->pfilterlist[i];
            SFILTER_createSPLIT(psplit);
            break;
        }

        if (((MIXFXMODSOURCE *)pcurModule)->head != 0) {
            if (((MIXFXMODSOURCE *)pcurModule)->type == 9) {
                tempHeadVal = ((MIXFXMODSOURCE *)pcurModule)->head;
                pBusNodeInstance->phead[remap(
                    pBusNodeInstance->validstructoutputs, sndmix.mc.outputchannels, tempHeadVal & 0xF
                )] = pBusNodeInstance->pfilterlist[i];
                pBusNodeInstance->phead[remap(
                    pBusNodeInstance->validstructoutputs, sndmix.mc.outputchannels, tempHeadVal >> 4
                )] = pBusNodeInstance->pfilterlist[i];
            } else {
                pBusNodeInstance->phead[remap(
                    pBusNodeInstance->validstructoutputs, sndmix.mc.outputchannels, ((MIXFXMODSOURCE *)pcurModule)->head
                )] = pBusNodeInstance->pfilterlist[i];
            }
        }

        pcurModule += filtermodsizetable[((MIXFXMODSOURCE *)pcurModule)->type];
    }

    pcurcon = conlist;
    for (i = 0; i < SNDI_getb(pBusNodeInstance->headerInfo.numconnectsstruct[k], 4); i++) {
        SFILTER_connect(
            pBusNodeInstance->pfilterlist[SNDI_getb(pcurcon->node1ID, 4)],
            pBusNodeInstance->pfilterlist[SNDI_getb(pcurcon->node2ID, 4)], pcurcon->node1output,
            pcurcon->node2input
        );
        pcurcon++;
    }

    sndfx.timeout = 3000;
#ifdef EA_PLATFORM_PLAYSTATION2
    memset(sndfx.inbuf[fxBusIndex], 0, 0x800);
#else
    memset(sndfx.inbuf[fxBusIndex], 0, 0x280);
#endif

    pcurModule = (char *)modlist;
    pBusNodeInstance->indexToSrcModule = 0;
    while (((MIXFXMODSOURCE *)pcurModule)->type != 0) {
        pcurModule += filtermodsizetable[((MIXFXMODSOURCE *)pcurModule)->type];
        pBusNodeInstance->indexToSrcModule++;
    }

    for (i = 0; i < 6; i++) {
        pBusNodeInstance->outputLevelcur[i] = 1.0f;
        pBusNodeInstance->outputLevelnew[i] = 1.0f;
    }
    pBusNodeInstance->outputLevelChanged = 0;
}

extern "C" void MIX_createFxglobals() {
    int memAllocSize;

#ifdef EA_PLATFORM_PLAYSTATION2
    memAllocSize = Snd::gMaxFxBuses * sizeof(float *) + Snd::gMaxFxBuses * sizeof(int) +
                   Snd::gMaxFxBuses * sizeof(float) + Snd::gMaxFxBuses * sizeof(float *);
#else
    memAllocSize = Snd::gMaxFxBuses * sizeof(float *) + Snd::gMaxFxBuses * sizeof(int) +
                   Snd::gMaxFxBuses * sizeof(float);
#endif
    sndfx.inbuf = (float **)SNDMEMI_allocz(memAllocSize);
    sndmix.wetbusactive = (int *)(sndfx.inbuf + Snd::gMaxFxBuses);
    sndmix.stoppedvoiceaccwet = (float *)(sndmix.wetbusactive + Snd::gMaxFxBuses);
#ifdef EA_PLATFORM_PLAYSTATION2
    sndfx.inbufUnaligned = (float **)(sndmix.stoppedvoiceaccwet + Snd::gMaxFxBuses);
#endif
    SNDI_memclear(sndfx.inbuf, memAllocSize);

    fxBusesLinkList = (CListStack *)SNDMEMI_allocz(Snd::gMaxFxBuses * sizeof(CListStack));
    SNDI_memclear(fxBusesLinkList, Snd::gMaxFxBuses * sizeof(CListStack));

    fxGlobalsCreated = 1;
}

extern "C" void MIX_destroyFxglobals() {
    int i;
    BUSNODE_INSTANCE *pBusNodeInstance;
    CListNode *pNextNode;

    if (fxGlobalsCreated) {
        SNDMEMI_free(sndfx.inbuf);
        for (i = 0; i < Snd::gMaxFxBuses; i++) {
            if (!fxBusesLinkList[i].IsEmpty()) {
                pBusNodeInstance = (BUSNODE_INSTANCE *)fxBusesLinkList[i].GetHead();
                while (pBusNodeInstance != NULL) {
                    pNextNode = pBusNodeInstance->ln.GetNext();
                    SNDMEMI_free(pBusNodeInstance);
                    pBusNodeInstance = (BUSNODE_INSTANCE *)pNextNode;
                }
            }
        }
        SNDMEMI_free(fxBusesLinkList);
        fxGlobalsCreated = 0;
    }
}

#ifdef EA_PLATFORM_PLAYSTATION2
extern "C" void MIX_inbufInit(int fxBusIndex) {
    sndfx.inbufUnaligned[fxBusIndex] = (float *)SNDMEMI_allocz(0x840);
    sndfx.inbuf[fxBusIndex] = (float *)(((sndptruint)sndfx.inbufUnaligned[fxBusIndex] + 0x3F) >> 6 << 6);
}

extern "C" void MIX_inbufRestore(int fxBusIndex) {
    sndmix.wetbusactive[fxBusIndex] = 0;
    if (sndfx.inbuf[fxBusIndex] != NULL) {
        SNDMEMI_free(sndfx.inbufUnaligned[fxBusIndex]);
        sndfx.inbufUnaligned[fxBusIndex] = NULL;
        sndfx.inbuf[fxBusIndex] = NULL;
    }
}
#endif

extern "C" unsigned int MIX_initreverb(int outputrate, int fxBusIndex, void *pFxFile) {
    char *pcur;
    int i;
    int j;
    int k;
    int temp;
    unsigned int retVal;
    BUSNODE_INSTANCE *pBusNodeInstance;

    if (fxBusesLinkList[fxBusIndex].IsEmpty()) {
#ifdef EA_PLATFORM_PLAYSTATION2
        sndfx.inbufUnaligned[fxBusIndex] = (float *)SNDMEMI_allocz(0x840);
        sndfx.inbuf[fxBusIndex] = (float *)(((sndptruint)sndfx.inbufUnaligned[fxBusIndex] + 0x3F) >> 6 << 6);
#else
        sndfx.inbuf[fxBusIndex] = (float *)SNDMEMI_allocz(0x280);
#endif
    }

    pBusNodeInstance = (BUSNODE_INSTANCE *)SNDMEMI_allocz(sizeof(BUSNODE_INSTANCE));
    if (pBusNodeInstance == NULL) {
        retVal = 0;
        return retVal;
    }

    fxBusesLinkList[fxBusIndex].Push(&pBusNodeInstance->ln);
    Snd::Util::MemCpy(&pBusNodeInstance->headerInfo, pFxFile, sizeof(MIXFXFILTHEADER));
    pcur = (char *)pFxFile + sizeof(MIXFXFILTHEADER);

    j = sndmix.mc.outputchannels - 1;
    if (SNDI_getb(pBusNodeInstance->headerInfo.numfiltersstruct[j], 4) != 0) {
        pBusNodeInstance->validstructoutputs = sndmix.mc.outputchannels;
    } else {
        while (SNDI_getb(pBusNodeInstance->headerInfo.numfiltersstruct[j], 4) == 0) {
            j--;
        }
        pBusNodeInstance->validstructoutputs = j + 1;
    }
    pBusNodeInstance->lastoutputchannels = pBusNodeInstance->validstructoutputs;

    for (i = 0; i < j; i++) {
        temp = SNDI_getb(pBusNodeInstance->headerInfo.numfiltersstruct[i], 4);
        for (k = 0; k < temp; k++) {
            pcur += filtermodsizetable[((MIXFXFILTERMOD *)pcur)->type];
        }
        temp = SNDI_getb(pBusNodeInstance->headerInfo.numconnectsstruct[i], 4);
        pcur += temp * sizeof(MIXFXCONNECTION);
    }

    modlist = (MIXFXFILTERMOD *)pcur;
    temp = SNDI_getb(pBusNodeInstance->headerInfo.numfiltersstruct[j], 4);
    for (k = 0; k < temp; k++) {
        pcur += filtermodsizetable[((MIXFXFILTERMOD *)pcur)->type];
    }
    conlist = (MIXFXCONNECTION *)pcur;

    SNDMIXI_initfx(outputrate, pFxFile, fxBusIndex, pBusNodeInstance);

    sndmix.fxadd = SNDMIXI_modlapifxadd;
    sndmix.wetbusactive[fxBusIndex] = 1;

    retVal = (sndptruint)pBusNodeInstance;

    return retVal;
}

extern "C" unsigned int MIX_restorereverb(int fxBusIndex, unsigned int instanceHandle) {
    CListNode *pPrevNode;
    CListNode *pNextNode;
    int nodeFound = 0;
    BUSNODE_INSTANCE *pBusNodeInstance = (BUSNODE_INSTANCE *)instanceHandle;

    SNDMIXI_restorefx2(fxBusIndex, pBusNodeInstance);

    pPrevNode = fxBusesLinkList[fxBusIndex].GetHead();
    pNextNode = pPrevNode->GetNext();
    if (pNextNode == NULL) {
        if (pPrevNode == &pBusNodeInstance->ln) {
            fxBusesLinkList[fxBusIndex].Reset();
            nodeFound = 1;
        }
    } else {
        while (pNextNode != NULL && nodeFound == 0) {
            if (pPrevNode == &pBusNodeInstance->ln) {
                fxBusesLinkList[fxBusIndex].Pop();
                nodeFound = 1;
            } else if (pNextNode == &pBusNodeInstance->ln) {
                pPrevNode->SetNext(pNextNode->GetNext());
                nodeFound = 1;
            } else {
                pPrevNode = pNextNode;
                pNextNode = pNextNode->GetNext();
            }
        }
    }

    if (nodeFound) {
        SNDMEMI_free(pBusNodeInstance);
    }

    if (fxBusesLinkList[fxBusIndex].IsEmpty()) {
        sndmix.wetbusactive[fxBusIndex] = 0;
        if (sndfx.inbuf[fxBusIndex] != NULL) {
#ifdef EA_PLATFORM_PLAYSTATION2
            SNDMEMI_free(sndfx.inbufUnaligned[fxBusIndex]);
            sndfx.inbufUnaligned[fxBusIndex] = NULL;
#else
            SNDMEMI_free(sndfx.inbuf[fxBusIndex]);
#endif
            sndfx.inbuf[fxBusIndex] = NULL;
        }
    }

    return 0;
}

#ifdef EA_PLATFORM_PLAYSTATION2
// Pasa el bus de efectos al bufer de la IOP (pfxwetmix): escala por 127, a entero y deja el de entrada a cero. El
// asm de la VU0 es el del original (vf1-vf5 y los registros de trabajo del binario de la A124).
extern "C" void SNDMIXI_fxadd(int frames) {
    if (sndmix.pfxwetmix != NULL) {
        asm __volatile__(".set noreorder\n"
                         "move $4, %0\n"
                         "move $5, %1\n"
                         "move $6, %2\n"
                         "li $8, 0x7f\n"
                         "sll $4, $4, 2\n"
                         "qmtc2.ni $8, vf1\n"
                         "add $4, $4, $5\n"
                         "vitof0.xyzw vf1, vf1\n"
                         "1:\n"
                         "lqc2 vf2, 0x0($5)\n"
                         "lqc2 vf3, 0x10($5)\n"
                         "lqc2 vf4, 0x20($5)\n"
                         "lqc2 vf5, 0x30($5)\n"
                         "vmulx.xyzw vf2, vf2, vf1x\n"
                         "sq $0, 0x0($5)\n"
                         "vmulx.xyzw vf3, vf3, vf1x\n"
                         "sq $0, 0x10($5)\n"
                         "vmulx.xyzw vf4, vf4, vf1x\n"
                         "sq $0, 0x20($5)\n"
                         "vmulx.xyzw vf5, vf5, vf1x\n"
                         "sq $0, 0x30($5)\n"
                         "addi $5, $5, 0x40\n"
                         "addi $6, $6, 0x40\n"
                         "vftoi0.xyzw vf2, vf2\n"
                         "vftoi0.xyzw vf3, vf3\n"
                         "vftoi0.xyzw vf4, vf4\n"
                         "vftoi0.xyzw vf5, vf5\n"
                         "sqc2 vf2, -0x40($6)\n"
                         "sqc2 vf3, -0x30($6)\n"
                         "sqc2 vf4, -0x20($6)\n"
                         "bne $4, $5, 1b\n"
                         "sqc2 vf5, -0x10($6)\n"
                         ".set reorder"
                         :
                         : "r"(frames), "r"(sndfx.inbuf[0]), "r"(sndmix.pfxwetmix)
                         : "$4", "$5", "$6", "$8", "memory");
    }
}
#endif
