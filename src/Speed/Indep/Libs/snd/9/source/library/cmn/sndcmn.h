#ifndef SNDCMN_H
#define SNDCMN_H

#include "./slinklist.h"
#include <snd/sndo.h>

/* Entero del tamano de un puntero.  El original hace `(int)puntero` en trece
 * unidades, que solo vale con punteros de 32 bits: ptrdiff_t y size_t son
 * int y unsigned int en las cuatro versiones del juego (el codigo no cambia)
 * y crecen solos con punteros de 64 bits, sin guarda de plataforma. */
#include <stddef.h>
typedef ptrdiff_t sndptrint;
typedef size_t sndptruint;

// total size: 0xFC
// Decl: 470
struct SNDIPATCHHEADER {
    short masterranddetune;            // offset 0x0, size 0x2, Decl: 471
    signed char platformver;           // offset 0x2, size 0x1, Decl: 472
    signed char numchan;               // offset 0x3, size 0x1, Decl: 473
    short detune;                      // offset 0x4, size 0x2, Decl: 475
    short randdetune;                  // offset 0x6, size 0x2, Decl: 476
    signed char velmin;                // offset 0x8, size 0x1, Decl: 478
    signed char velmax;                // offset 0x9, size 0x1, Decl: 479
    signed char keymin;                // offset 0xA, size 0x1, Decl: 480
    signed char keymax;                // offset 0xB, size 0x1, Decl: 481
    signed char priority;              // offset 0xC, size 0x1, Decl: 483
    signed char basekey;               // offset 0xD, size 0x1, Decl: 484
    signed char releaseenvelope;       // offset 0xE, size 0x1, Decl: 485
    signed char numenvelopes;          // offset 0xF, size 0x1, Decl: 486
    signed char bendrange;             // offset 0x10, size 0x1
    signed char pan;                   // offset 0x11, size 0x1, Decl: 488
    signed char randpan;               // offset 0x12, size 0x1, Decl: 489
    unsigned char samplerep;           // offset 0x13, size 0x1, Decl: 490
    signed char vol;                   // offset 0x14, size 0x1, Decl: 491
    signed char randvol;               // offset 0x15, size 0x1, Decl: 493
    signed char fxlevel0;              // offset 0x16, size 0x1, Decl: 494
    signed char numuserdata;           // offset 0x17, size 0x1, Decl: 495
    short unsigned int rendermode;     // offset 0x18, size 0x2
    signed char initialenvvol;         // offset 0x1A, size 0x1, Decl: 496
    unsigned char lfovollength;        // offset 0x1B, size 0x1, Decl: 498
    signed char lfovolrand;            // offset 0x1C, size 0x1, Decl: 501
    unsigned char lfopitchlength;      // offset 0x1D, size 0x1, Decl: 502
    unsigned char lfopitchrand;        // offset 0x1E, size 0x1, Decl: 503
    signed char panmult;               // offset 0x1F, size 0x1, Decl: 504
    void *pvoltable;                   // offset 0x20, size 0x4, Decl: 506
    void *pbendtable;                  // offset 0x24, size 0x4
    void *penvelope;                   // offset 0x28, size 0x4, Decl: 507
    void *plfovol;                     // offset 0x2C, size 0x4, Decl: 508
    void *plfopitch;                   // offset 0x30, size 0x4, Decl: 509
    void *puserdata[4];                // offset 0x34, size 0x10, Decl: 510
    int userdatasize[4];               // offset 0x44, size 0x10, Decl: 511
    short lfopitchdepth;               // offset 0x54, size 0x2, Decl: 513
    unsigned short samplerate;         // offset 0x56, size 0x2, Decl: 514
    int totalframes;                   // offset 0x58, size 0x4, Decl: 516
    int sustainstart;                  // offset 0x5C, size 0x4, Decl: 517
    int sustainend;                    // offset 0x60, size 0x4, Decl: 518
    int *paudioramaddr;                // offset 0x64, size 0x4
    int sampleoffset[6];               // offset 0x68, size 0x18, Decl: 520
    int loopoffset[6];                 // offset 0x80, size 0x18, Decl: 522
    void *ptimestretchdata[6];         // offset 0x98, size 0x18, Decl: 525
    unsigned short timestretchsize[6]; // offset 0xB0, size 0xC, Decl: 526
    unsigned short azimuth[6];         // offset 0xBC, size 0xC, Decl: 529
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32)
    // snd 9.06.00-ps2 no lleva los libros de codigos ni los estados de bucle del ADPCM de la DSP (mide 0xCC); el PC
    // tampoco (SNDI_parsetimbre, speed.exe 0x823E83: ptimbre en +0xC8)
    void *pcodebookdata[6];            // offset 0xC8, size 0x18
    void *ploopstatedata[6];           // offset 0xE0, size 0x18
#endif
    void *ptimbre;                     // offset 0xF8 (PS2 y PC: 0xC8), size 0x4, Decl: 531
};

// total size: 0x8
// Decl: 538
typedef struct ENVELOPE {
    int duration;  // offset 0x0, size 0x4
    int targetvol; // offset 0x4, size 0x4
} ENVELOPE;

// total size: 0x80
// Decl: 545
typedef struct SCALINGTABLE {
    signed char xlate[128]; // offset 0x0, size 0x80
} SCALINGTABLE;

// total size: 0x8
// Decl: 553
typedef struct TAGGEDPATCH {
    short id;               // offset 0x0, size 0x2
    unsigned char platform; // offset 0x2, size 0x1
    unsigned char flags;    // offset 0x3, size 0x1
    int hdrsize;            // offset 0x4, size 0x4
} TAGGEDPATCH;

// total size: 0x814
// Decl: 564
typedef struct BANKVER5 {
    int id;                    // offset 0x0, size 0x4
    unsigned char ver;         // offset 0x4, size 0x1
    char resolved;             // offset 0x5, size 0x1
    unsigned short numpatches; // offset 0x6, size 0x2
    int hdrsize;               // offset 0x8, size 0x4
    int spusize;               // offset 0xC, size 0x4
    int iopcpusize;            // offset 0x10, size 0x4
    TAGGEDPATCH *patch[512];   // offset 0x14, size 0x800
} BANKVER5;

// total size: 0xC
// Decl: 577
typedef struct BANKLIST {
    BANKVER5 *phdr;     // offset 0x0, size 0x4
#ifdef EA_PLATFORM_PLAYSTATION2
    void *piopram;      // offset 0x4, size 0x4 (snd 9.06.00-ps2: la memoria de la IOP)
#endif
    void *pspuram;      // offset 0x4 (PS2: 0x8), size 0x4
    signed char locked; // offset 0x8 (PS2: 0xC), size 0x1
    char pad[3];        // offset 0x9 (PS2: 0xD), size 0x3
} BANKLIST;

// total size: 0x4
// Decl: 587
typedef struct FXVOLUME {
    float fxLevel; // offset 0x0, size 0x4
} FXVOLUME;

// total size: 0x90
// Decl: 598
typedef struct CHANPUB {
    int handle;                       // offset 0x0, size 0x4
    short voices[6];                  // offset 0x4, size 0xC
    unsigned short patnum;            // offset 0x10, size 0x2
    short bhandle;                    // offset 0x12, size 0x2
    unsigned int frames;              // offset 0x14, size 0x4
    int sustainend;                   // offset 0x18, size 0x4
    unsigned short azimuth;           // offset 0x1C, size 0x2
    short elevation;                  // offset 0x1E, size 0x2
    unsigned short samplerate;        // offset 0x20, size 0x2
    unsigned char samplerep;          // offset 0x22, size 0x1
    unsigned char numchan;            // offset 0x23, size 0x1
    unsigned short rendermode;        // offset 0x24, size 0x2
    unsigned char patchkey;           // offset 0x26, size 0x1
    unsigned char ismaster;           // offset 0x27, size 0x1
    short masterchan;                 // offset 0x28, size 0x2
    unsigned char priority;           // offset 0x2A, size 0x1
    signed char builtinvol;           // offset 0x2B, size 0x1
    unsigned int timestamp;           // offset 0x2C, size 0x4
    float fadePer100Hz;               // offset 0x30, size 0x4
    float fadeTargetVol;              // offset 0x34, size 0x4
    float programmedVol;              // offset 0x38, size 0x4
    int envpertick;                   // offset 0x3C, size 0x4
    int envvol;                       // offset 0x40, size 0x4
    int envduration;                  // offset 0x44, size 0x4
    float finalvol;                   // offset 0x48, size 0x4
    unsigned short builtinazimuth;    // offset 0x4C, size 0x2
    unsigned char isLfe;              // offset 0x4E, size 0x1
    char pad[1];                      // offset 0x4F, size 0x1
    unsigned short azimuthOffsets[6]; // offset 0x50, size 0xC
    signed char numenvelopes;         // offset 0x5C, size 0x1
    signed char curenvelope;          // offset 0x5D, size 0x1
    signed char releaseenvelope;      // offset 0x5E, size 0x1
    signed char drylevel;             // offset 0x5F, size 0x1
    signed char progfxlevel;          // offset 0x60, size 0x1
    char pad1[3];                     // offset 0x61, size 0x3
    FXVOLUME *pFxVolume;              // offset 0x64, size 0x4
    signed char pitchbend;            // offset 0x68, size 0x1
    unsigned char vollfolength;       // offset 0x69, size 0x1
    unsigned char pitchlfolength;     // offset 0x6A, size 0x1
    unsigned char curvollfoentry;     // offset 0x6B, size 0x1
    unsigned char curpitchlfoentry;   // offset 0x6C, size 0x1
    signed char status;               // offset 0x6D, size 0x1
    unsigned short timemult;          // offset 0x6E, size 0x2
    ENVELOPE *paenvelope;             // offset 0x70, size 0x4
    SCALINGTABLE *pvoltable;          // offset 0x74, size 0x4
    SCALINGTABLE *pbendtable;         // offset 0x78, size 0x4
    SCALINGTABLE *pvollfo;            // offset 0x7C, size 0x4
    SCALINGTABLE *ppitchlfo;          // offset 0x80, size 0x4
    short pitchlfodepth;              // offset 0x84, size 0x2
    short bendrange;                  // offset 0x86, size 0x2
    short initialdetune;              // offset 0x88, size 0x2
    unsigned short detunepitch;       // offset 0x8A, size 0x2
    unsigned short progpitch;         // offset 0x8C, size 0x2
    unsigned short finalpitch;        // offset 0x8E, size 0x2
} CHANPUB;

// total size: 0x8
struct SNDIPACKETCALLBACK {
    unsigned short type; // offset 0x0, size 0x2
    unsigned short chan; // offset 0x2, size 0x2

    union {
        unsigned int frames; // offset 0x0, size 0x4
        void *psamples; // offset 0x0, size 0x4
    } u; // offset 0x4, size 0x4
};

// total size: 0x384
struct SNDPACKETSTATE {
    // Members
    int callbackindex; // offset 0x0, size 0x4
    SNDIPACKETCALLBACK spcb[96]; // offset 0x4, size 0x300
    SNDPACKETCHAN *ppc[32]; // offset 0x304, size 0x80
};

// total size: 0x8
typedef struct SNDMEMREC {
    unsigned int addr; // offset 0x0, size 0x4
    int size;          // offset 0x4, size 0x4
} SNDMEMREC;

// total size: 0x18
typedef struct SNDMEMSTATE {
    char *pheap;          // offset 0x0, size 0x4
    SNDMEMREC *r;         // offset 0x4, size 0x4
    int heapsize;         // offset 0x8, size 0x4
    unsigned int endaddr; // offset 0xC, size 0x4
    int lowmark;          // offset 0x10, size 0x4
    int nummallocs;       // offset 0x14, size 0x4
} SNDMEMSTATE;

// total size: 0x1A
// Decl: 789
typedef struct SNDSYSCAP {
    unsigned short outputratemin;    // offset 0x0, size 0x2
    unsigned short outputratemax;    // offset 0x2, size 0x2
    unsigned char outputchannelsmin; // offset 0x4, size 0x1
    unsigned char outputchannelsmax; // offset 0x5, size 0x1
    unsigned char voicesmaincpumax;  // offset 0x6, size 0x1
    unsigned char voicesiopcpumax;   // offset 0x7, size 0x1
    unsigned char voicesspumax;      // offset 0x8, size 0x1
    unsigned char voicesds2dhwmax;   // offset 0x9, size 0x1
    unsigned char voicesdspmax;      // offset 0xA, size 0x1
    unsigned char eax;               // offset 0xB, size 0x1
    unsigned char numrendermodes;    // offset 0xC, size 0x1
    unsigned short rendermode[6];    // offset 0xE, size 0xC
} SNDSYSCAP;

// total size: 0x9C
// Decl: 796
typedef struct SNDSYSSET {
    int packetbufsize;                      // offset 0x0, size 0x4
    unsigned int randomseed;                // offset 0x4, size 0x4
    unsigned short maxbanks;                // offset 0x8, size 0x2
    unsigned short outputrate;              // offset 0xA, size 0x2
    unsigned short outputrateiopcpu;        // offset 0xC, size 0x2
    unsigned char voicesmaincpu;            // offset 0xE, size 0x1
    unsigned char voicesiopcpu;             // offset 0xF, size 0x1
    unsigned char voicesspu;                // offset 0x10, size 0x1
    unsigned char voicesds2dhw;             // offset 0x11, size 0x1
    unsigned char voicesdsp;                // offset 0x12, size 0x1
    char compatibility;                     // offset 0x13, size 0x1
    float updateperiod;                     // offset 0x14, size 0x4
    unsigned char stealequalpriorityvoices; // offset 0x18, size 0x1
    unsigned char usesse;                   // offset 0x19, size 0x1
    unsigned char useeax;                   // offset 0x1A, size 0x1
    unsigned char loadiopmodules;           // offset 0x1B, size 0x1
    unsigned char resamplequality;          // offset 0x1C, size 0x1
    unsigned char iopcpuresamplequality;    // offset 0x1D, size 0x1
    unsigned char routemaincpufxtoiopcpu;   // offset 0x1E, size 0x1
    unsigned char routespufxtoiopcpu;       // offset 0x1F, size 0x1
    unsigned char emulationsubtype;         // offset 0x20, size 0x1
    unsigned char initaram;                 // offset 0x21, size 0x1
    unsigned char maxstreams;               // offset 0x22, size 0x1
    unsigned char outputchannels;           // offset 0x23, size 0x1
    unsigned char outputlfe;                // offset 0x24, size 0x1
    unsigned char outputspdifmode;          // offset 0x25, size 0x1
    unsigned char dtsquality;               // offset 0x26, size 0x1
    unsigned char sndheapthreshold;         // offset 0x27, size 0x1
    unsigned char numrendermodes;           // offset 0x28, size 0x1
    unsigned char useASIOdriver;            // offset 0x29, size 0x1
    unsigned short rendermode[6];           // offset 0x2A, size 0xC
    unsigned int arampooladdr;              // offset 0x38, size 0x4
    int arampoolsize;                       // offset 0x3C, size 0x4
    void *fastramaddr;                      // offset 0x40, size 0x4
    int fastramsize;                        // offset 0x44, size 0x4
    char *iopmodulepath;                    // offset 0x48, size 0x4
    char *ASIOconfig;                       // offset 0x4C, size 0x4
    void *snd2asio;                         // offset 0x50, size 0x4
    unsigned short virtualspkrcfg3d[6][6];  // offset 0x54, size 0x48
} SNDSYSSET;

// total size: 0x14
// Decl: 812
typedef struct SNDSYSVEC {
    unsigned int (*aramalloc)(unsigned int); // offset 0x0, size 0x4
    void (*aramfree)(unsigned int);          // offset 0x4, size 0x4
    void (*abortmsg)(char *);                // offset 0x8, size 0x4
    void (*profileenter)(const char *);      // offset 0xC, size 0x4
    void (*profileleave)(const char *);      // offset 0x10, size 0x4
} SNDSYSVEC;

// total size: 0xCC
// Decl: 818
typedef struct SNDSYSOPTS {
    SNDSYSCAP cap; // offset 0x0, size 0x1A
    SNDSYSSET set; // offset 0x1C, size 0x9C
    SNDSYSVEC vec; // offset 0xB8, size 0x14
} SNDSYSOPTS;

// total size: 0x244
// Decl: 825
typedef struct SNDGLOBALSTATE {
    SNDSYSOPTS sso;                                          // offset 0x0, size 0xCC
    SNDSYSSET prevset;                                       // offset 0xCC, size 0x9C
    signed char installed;                                   // offset 0x168, size 0x1
    volatile signed char incritical;                         // offset 0x169, size 0x1
    signed char numserverclients100hz;                       // offset 0x16A, size 0x1
    signed char numserverclients;                            // offset 0x16B, size 0x1
    short voicestotal;                                       // offset 0x16C, size 0x2
    signed char numuserdataclients;                          // offset 0x16E, size 0x1
    unsigned char cputypes;                                  // offset 0x16F, size 0x1
    volatile unsigned int audiotick;                         // offset 0x170, size 0x4
    void (*serverclient100hz[6])();                          // offset 0x174, size 0x18
    void (*serverclient[6])();                               // offset 0x18C, size 0x18
    void (*userdataclient[4])(SNDUSERDATACBINFO *);          // offset 0x1A4, size 0x10
    void (*aemsrestore)();                                   // offset 0x1B4, size 0x4
    int (*aemsstopmodulebanks)();                            // offset 0x1B8, size 0x4
    int (*aemsstreamrestore)();                              // offset 0x1BC, size 0x4
    int (*eventrestore)();                                   // offset 0x1C0, size 0x4
    int (*bankremove)(int);                                  // offset 0x1C4, size 0x4
    int (*midirestore)();                                    // offset 0x1C8, size 0x4
    int (*streamrestore)();                                  // offset 0x1CC, size 0x4
    int (*aemsstreampurge)();                                // offset 0x1D0, size 0x4
    CHANPUB *chan;                                           // offset 0x1D4, size 0x4
    BANKLIST *banklist;                                      // offset 0x1D8, size 0x4
    SNDMEMSTATE *mm;                                         // offset 0x1DC, size 0x4
    unsigned short srcchancfg3d[6][6];                       // offset 0x1E0, size 0x48
    void (*profileenter)();                                  // offset 0x228, size 0x4
    void (*profileleave)();                                  // offset 0x22C, size 0x4
    volatile unsigned int servertick;                        // offset 0x230, size 0x4
    volatile short currenttimerhz;                           // offset 0x234, size 0x2
    volatile short servicesskipped;                          // offset 0x236, size 0x2
    char aborted;                                            // offset 0x238, size 0x1
    char logmixing;                                          // offset 0x239, size 0x1
    void (*logcallbackfn)(int, int, unsigned short, void *); // offset 0x23C, size 0x4
    char debugpad[2];                                        // offset 0x240, size 0x2
} SNDGLOBALSTATE;

// total size: 0x8
// Decl: 761
typedef struct SNDLINKNODE {
    SNDLINKNODE *pnext; // offset 0x0, size 0x4
    SNDLINKNODE *pprev; // offset 0x4, size 0x4
} SNDLINKNODE;

// total size: 0xC
// Decl: 767
typedef struct SNDLINKLIST {
    SNDLINKNODE *phead; // offset 0x0, size 0x4
    SNDLINKNODE *ptail; // offset 0x4, size 0x4
    int items;          // offset 0x8, size 0x4
} SNDLINKLIST;

void SNDAEMSI_timerupdate(void *pClientData);

namespace Snd {

// total size: 0x10
// Decl: 1244
typedef struct VariableTimerClient {
    CListDNode ln;               // offset 0x0, size 0x8
    void (*pClientFunc)(void *); // offset 0x8, size 0x4
    void *pClientData;           // offset 0xC, size 0x4
} VariableTimerClient;

extern float gVariableTimerPeriod;
extern int gMaxFxBuses;
extern CListDStack gVariableTimerList;
extern float gMasterVol;
extern const signed char gChannelToVoiceIndexLut[6][6];
extern unsigned char gTotalOutputChannels;
extern float gSpeakerPositions[5];
extern const unsigned char gAzimuthFoldDownLut[4][6][6];

// total size: 0x1
// Decl: 1268
struct Util {
    static void ReallocBuf(void **buffer, int *curSize, int newMinSize, int sizeofDataType);

    static unsigned short Az360To65536(float azimuth) {
        return (unsigned short)(azimuth * (65536.0f / 360.0f));
    }

    static float Az65536To360(unsigned short azimuth) {
        return (float)azimuth * (360.0f / 65536.0f);
    }

    static void FastVol(struct CHANPUB *pVoice) {
        pVoice->finalvol = pVoice->programmedVol * gMasterVol;
    }

    static void AddVariableTimerClient(VariableTimerClient *pClient) {
        gVariableTimerList.Push(&pClient->ln);
    }

    static void RemoveVariableTimerClient(VariableTimerClient *pClient) {
        gVariableTimerList.Remove(&pClient->ln);
    }

    static void *MemCpy(void *pDst, const void *pSrc, unsigned int bytes);
    static void SetDefaultAzimuths(CHANPUB *pVoice);
};

// total size: 0x28
// Decl: 64
typedef struct SNDSTREAMREQUEST {
    SNDLINKNODE node;                        // offset 0x0, size 0x8
    volatile int streamrequestid;            // offset 0x8, size 0x4
    volatile int sndrequesthandle;           // offset 0xC, size 0x4
    volatile unsigned int avgdatarate;       // offset 0x10, size 0x4
    volatile unsigned int currentframe;      // offset 0x14, size 0x4
    volatile int totalframes;                // offset 0x18, size 0x4
    volatile unsigned int outstandingframes; // offset 0x1C, size 0x4
    volatile int holdtime;                   // offset 0x20, size 0x4
    unsigned char continuation;              // offset 0x24, size 0x1
    volatile unsigned char isgeneric;        // offset 0x25, size 0x1
    char pad[2];                             // offset 0x26, size 0x2
} SNDSTREAMREQUEST;

// total size: 0x4
typedef struct SNDSTREAMFXVOLUME {
    float fxLevel; // offset 0x0, size 0x4
} SNDSTREAMFXVOLUME;

// total size: 0x18
typedef struct Fader {
    VariableTimerClient variableTimerClient; // offset 0x0, size 0x10
    float targetVol;                         // offset 0x10, size 0x4
    float incrementPerUpdate;                // offset 0x14, size 0x4
} Fader;

// total size: 0x24
typedef struct StreamSourceChannelState {
    float azimuth;               // offset 0x0, size 0x4
    unsigned char isDirect;      // offset 0x4, size 0x1
    unsigned char sourceChannel; // offset 0x5, size 0x1
    char pad[2];                 // offset 0x6, size 0x2
    float vol;                   // offset 0x8, size 0x4
    Fader fader;                 // offset 0xC, size 0x18
} StreamSourceChannelState;

static inline void StreamVolSetState(StreamSourceChannelState *pSourceChannelState, float vol) {
    pSourceChannelState->vol = vol;
    if (pSourceChannelState->fader.incrementPerUpdate != 0.0f) {
        Util::RemoveVariableTimerClient(&pSourceChannelState->fader.variableTimerClient);
        pSourceChannelState->fader.incrementPerUpdate = 0.0f;
    }
}

}; // namespace Snd

typedef int STREAMHANDLE;
typedef int STREAMREQUESTID;

// total size: 0x214 (Xenon 0x208: 4 B de hueco delante y SNDSAMPLEATTR/SNDPLAYOPTS mas cortos;
// SNDSTRM_purge de 360 en 0x8260F488 deja los campos de cabecera 4 B mas alla y los de detras de
// newsa 4 B mas aca: streamhandle/shandle/packetinstancehandle/state/fromtap/cursf/newsf en
// 0x4/0x8/0xC/0x14/0x15/0x18/0x1C, cursa en 0x20 (0x64), newsa en 0x84 (0x64) y las listas de
// peticiones en 0x114/0x120 con parsingrequest en 0x12C)
typedef struct SNDSTREAMCHANNEL {
#ifdef EA_PLATFORM_XENON
    char pad0[4];                                        // offset 0x0, size 0x4 (Xenon: hueco delante de streamhandle)
#endif
    STREAMHANDLE streamhandle;                           // offset 0x0, size 0x4 (Xenon 0x4)
    volatile int shandle;                                // offset 0x4, size 0x4 (Xenon 0x8)
    volatile int packetinstancehandle;                   // offset 0x8, size 0x4 (Xenon 0xC)
    volatile int lastsndrequesthandle;                   // offset 0xC, size 0x4 (Xenon 0x10)
    volatile signed char state;                          // offset 0x10, size 0x1 (Xenon 0x14)
    volatile signed char fromtap;                        // offset 0x11, size 0x1 (Xenon 0x15)
    unsigned char useOldAzimuthBehaviour;                // offset 0x12, size 0x1 (Xenon 0x16)
    char pad[1];                                         // offset 0x13, size 0x1 (Xenon 0x17)
    SNDSAMPLEFORMAT cursf;                               // offset 0x14, size 0x4 (Xenon 0x18)
    SNDSAMPLEFORMAT newsf;                               // offset 0x18, size 0x4 (Xenon 0x1C)
    SNDSAMPLEATTR cursa;                                 // offset 0x1C, size 0x68 (Xenon 0x20, size 0x64)
    SNDSAMPLEATTR newsa;                                 // offset 0x84, size 0x68 (Xenon 0x84, size 0x64)
    SNDPLAYOPTS po;                                      // offset 0xEC, size 0x18 (Xenon 0xE8, size 0x10)
    Snd::SNDSTREAMFXVOLUME *pFxVolume;                   // offset 0x104, size 0x4 (Xenon 0xF8)
    SNDFILTERDEF filter[1];                              // offset 0x108, size 0x18 (Xenon 0xFC)
    SNDLINKLIST allocatedrequests;                       // offset 0x120, size 0xC (Xenon 0x114)
    SNDLINKLIST freerequests;                            // offset 0x12C, size 0xC (Xenon 0x120)
    volatile Snd::SNDSTREAMREQUEST *parsingrequest;      // offset 0x138, size 0x4 (Xenon 0x12C)
    Snd::StreamSourceChannelState sourceChannelState[6]; // offset 0x13C, size 0xD8 (Xenon 0x130)
} SNDSTREAMCHANNEL;

// total size: 0x80
typedef struct SNDSTREAMSTATE {
    SNDSTREAMCHANNEL *pssc[32]; // offset 0x0, size 0x80
} SNDSTREAMSTATE;

extern SNDGLOBALSTATE sndgs; // Decl: 861

// PC (speed.exe 1.3): las funciones de la API de snd empiezan comprobando que el controlador esta instalado
// (`cmp byte [sndgs.installed], 0`; sin el, -10). GameCube y PS2 no lo hacen.
#ifdef EA_PLATFORM_WIN32
#define SNDI_CHECKINSTALLED()    \
    if (sndgs.installed == 0) {  \
        return SNDERR_NO_DRIVER; \
    }
#else
#define SNDI_CHECKINSTALLED()
#endif

// Los tipos de las transferencias DMA; en GameCube estan en el DWARF de todas las unidades de cmn/, pero solo las
// usa PS2 (SNDPLATFORM_downloadNew y la carga de bancos por la IOP).
// total size: 0x8
typedef struct CmnRequestHandle {
    int handle;           // offset 0x0, size 0x4
    void *requestDetails; // offset 0x4, size 0x4
} CmnRequestHandle;

typedef enum DmaPriority {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2
} DmaPriority;

typedef enum DmaTransferType {
    ASYNC = 0,
    SYNC = 1
} DmaTransferType;

#include "sndenum.h"

namespace Snd {

extern void (*gMutexLockFn)();
extern void (*gMutexUnlockFn)();

// TODO: move?
class Hal {
  public:
    static int SetPresetFx(void *pData, FxPreset fxPreset);

    static int SetPresetFxEx(void *pData, void *pFxPresetData);

    static unsigned int SetCustomFx(void *pData, void *pFxDefinition);

    static unsigned int Reset(void *pData);

    static int SetFxOutputLevel(void *pData, Channel outputChannel);

    static void SetVol(int voice);

    static void SetDry(int voice);

    static void SetPan(int voice);

  private:
    static void SetVolInternal(int voice); // guess
};

// TODO: move?
extern FoldDownTarget gFoldDownTarget;

}; // namespace Snd

// Decl: 1041
inline int SNDI_ftoiround(float val) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // snd 9.06.00-ps2 (sndcmn.h:1690 y 1699 en el .mdebug de la A124, con las locales tmp y result): elige el medio
    // redondeo por el signo y suma una sola vez. La suma y la conversion son asm de EA (sndcmn.h:1699): add.s,
    // cvt.w.s a $f4 y mfc1 en la misma linea, $f4 en 8 de 8 expansiones, como SNDI_ftoitruncpositive
    float tmp;
    int result;

    tmp = val < 0.0f ? -0.5f : 0.5f;
    asm volatile("add.s %1, %1, %2\n\tcvt.w.s $f4, %1\n\tmfc1 %0, $f4" : "=r"(result), "+f"(tmp) : "f"(val) : "$f4");
    return result;
#elif defined(EA_PLATFORM_WIN32) && defined(_MSC_VER) // otro compilador toma el C de abajo
    // asm de EA (speed.exe): fld + fistp, que redondea con el modo de la FPU. La copia fuera de linea de 0x820056
    // (51 d9442408 db1c24 8b0424 59 c3) es byte a byte la de este bloque; MSVC 7.1 sin /QIfist convierte con __ftol2.
    int result;

    __asm fld val
    __asm fistp result
    return result;
#else
    int result;

    if (val >= 0.0f) {
        result = static_cast<int>(val + 0.5f);
    } else {
        result = static_cast<int>(val - 0.5f);
    }

    return result;
#endif
}

inline int SNDI_ftoitruncpositive(float val) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // asm de EA (sndcmn.h:1845 del original): cvt.w.s a $f4 y mfc1, $f4 en 29 de 29 expansiones (el C da $f4 en 8 de
    // 540), como el ftoi.h de sdriver
    int result;

    asm volatile("cvt.w.s $f4, %1\n\tmfc1 %0, $f4" : "=r"(result) : "f"(val) : "$f4");
    return result;
#else
    return SNDI_ftoiround(val - 0.5f);
#endif
}

inline int SNDI_ftoiroundpositive(float val) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // asm de EA (sndcmn.h:1794 del original): el mismo add.s + cvt.w.s a $f4 + mfc1 que SNDI_ftoiround
    float tmp;
    int result;

    tmp = 0.5f;
    asm volatile("add.s %1, %1, %2\n\tcvt.w.s $f4, %1\n\tmfc1 %0, $f4" : "=r"(result), "+f"(tmp) : "f"(val) : "$f4");
    return result;
#else
    int result = static_cast<int>(val + 0.5f);

    return result;
#endif
}

// 1202
inline int SNDI_ftoifast(float val) {
#ifdef EA_PLATFORM_PLAYSTATION2
    int result; // el mismo asm de EA que SNDI_ftoitruncpositive

    asm volatile("cvt.w.s $f4, %1\n\tmfc1 %0, $f4" : "=r"(result) : "f"(val) : "$f4");
    return result;
#else
    return SNDI_ftoiround(val);
#endif
}

// 1610
static inline int SNDI_clipint32(int val, int minval, int maxval) {
    if (val < minval)
        return minval;
    if (val > maxval)
        return maxval;
    return val;
}

static inline float SNDI_clipf(float val, float minval, float maxval) {
    if (val < minval)
        return minval;
    if (val > maxval)
        return maxval;
    return val;
}

// sst.c
SNDSTREAMCHANNEL *SNDSTRMI_getstreamptr(int sndstreamhandle);

// sstgetrp.c
Snd::SNDSTREAMREQUEST *SNDSTRMI_getrequestptr(int sndrequesthandle);

// salloc.c
int SNDVOICEI_get(int handle);
int SNDVOICEI_alloc(int voicesneeded, int priority, int *phandle, int minvoicerange, int maxvoicerange);
void SNDVOICEI_free(int voice);

// SNDI_sin.c / SNDI_cos.c
float SNDI_sin(float x);
float SNDI_cos(float x);

// srrange.c
int randrange(int range);

// spantoaz.c
int SNDI_pantoazimuth(int pan);

// srender.c
int SNDI_validrendermode(int *pindex, SNDIPATCHHEADER *pph);

// gc/snddrv.c: la API de plataforma que las unidades cmn/ llaman
void SNDPLATFORM_getvoicerange(int playloc, int *minvoicerange, int *maxvoicerange);
int SNDPLATFORM_playtimbre(SNDIPATCHHEADER *pph, void *psampledata, int voice, int timemult,
                           int lowpasscutoff, int highpasscutoff);
int SNDPLATFORM_packetplaycreate(int packetinstancehandle, void *pmem);
int SNDPLATFORM_packetplaydestroy(int packetinstancehandle);

// sbadd.c
void SNDBANKI_userdatacallback(SNDIPATCHHEADER *pph, int shandle, int type);

// sballoc.c
TAGGEDPATCH *SNDBANKI_getppatch(BANKVER5 *pb, int patnum);

// spatkey.c
int iSNDpatchkey(int chan, int *psetchan);

// spat2hdr.c
void SNDI_patchtohdr(void *pbank, TAGGEDPATCH *ptp, SNDSAMPLEFORMAT *pssf, SNDSAMPLEATTR *pssa, SNDSAMPLEDESC *pssd, unsigned char *isgeneric);

// smemman.c
#ifdef EA_PLATFORM_XENON
// Xenon: el snd no tiene heap propio; SNDMEMI_allocz/SNDMEMI_free son la llamada por interfaz
// del asignador del nucleo y entran en linea en cada sitio del EGM (MIX_create 0x82617580,
// MIX_destroy 0x82617938, ??1GlobalFxProcessor 0x82615580, SNDI_patchtohdr 0x8262DB38):
// asignador en el campo +0x20 del nucleo de 0x82A7F9F0, Alloc en vtable[1] con
// (obj, size, 0, 1, 0x10, 0) y Free en vtable[3] con (obj, ptr, 0). smemman.c no se compila.
namespace EA {
namespace Thread {
class Mutex;
}
}

struct XenonSndAllocVt {
    void *slot0;                     // +0x0
    void *(*Alloc)(void *pself, unsigned size, unsigned a, unsigned b, unsigned c, unsigned d); // +0x4
    void *slot2;                     // +0x8
    void (*Free)(void *pself, void *ptr, unsigned a); // +0xC
};

struct XenonSndAllocator {
    XenonSndAllocVt *vt;
};

struct XenonUnknown82A7F9F0_t {
    char pad000[0x20];               // 0x00-0x1F (sin probar)
    XenonSndAllocator *allocator;    // 0x20
    char pad024[0x30];               // 0x24-0x53 (sin probar)
    void (*enterFn)();               // 0x54
    void (*leaveFn)();               // 0x58
    EA::Thread::Mutex *mutex;        // 0x5C
};

extern XenonUnknown82A7F9F0_t *XenonUnknown82A7F9F0;

inline void SNDMEMI_free(void *paddr) {
    XenonSndAllocator *palloc = XenonUnknown82A7F9F0->allocator;
    palloc->vt->Free(palloc, paddr, 0);
}

inline void *SNDMEMI_allocz(int size) {
    XenonSndAllocator *palloc = XenonUnknown82A7F9F0->allocator;
    return palloc->vt->Alloc(palloc, (unsigned)size, 0, 1, 0x10, 0);
}
#else
void SNDMEMI_free(void *paddr);
void *SNDMEMI_allocz(int size);
#endif

// sbvalid.c
int SNDBANKI_valid(int bhandle);

// sgetpvol.c
int SNDCTRL_getprogvol(int shandle);

// sstgetpv.c
int SNDSTRM_getprogvol(int sndstreamhandle);

// stimerem.c
int SNDtimeremaining(int shandle);

// srandom.c
unsigned int iSNDrandom();

// sgetdata.c
int SNDI_getb(void *pdata, int numbytes);

// sgettag.c
// total size: 0x14
typedef struct SNDTAGINFO {
    unsigned char *p;      // offset 0x0
    int tag;               // offset 0x4
    int size;              // offset 0x8
    unsigned char *pdata;   // offset 0xC
    unsigned int datasize; // offset 0x10
} SNDTAGINFO;
int SNDI_gettag(SNDTAGINFO *pti);

// spktplay.c
extern SNDPACKETSTATE sndpps;

// spktctoh.c
int SNDPKTPLAYI_voicetopackethandle(int voice);

// snddrv.c
int SNDPLATFORM_stop(int voice);
int SNDPLATFORM_filteradd(int voice, SNDFILTERDEF *psfd);
int SNDPLATFORM_asyncresolvetimbre(SNDIPATCHHEADER *pph, char *pdata, int *pfirstoffset);
int SNDPLATFORM_setpitch(int voice);
int SNDPLATFORM_timemult(int voice, int timemult);
int SNDPLATFORM_setfxlevel(int voice, int bus);
void SNDPLATFORM_lowpass(int voice, int cutofffreq);
void SNDPLATFORM_highpass(int voice, int cutofffreq);
int SNDPLATFORM_getcurframe(int voice);
unsigned int SNDPLATFORM_memalloc(int playloc, int size);
int SNDPLATFORM_memfree(int playloc, unsigned int addr);
int SNDPLATFORM_memlimits(int startaddr, int endaddr);
int SNDPLATFORM_memlargestunused(int *paddr);
int SNDPLATFORM_download(int playloc, void *psrc, void *pdst, int size);
int SNDPLATFORM_downloadcomplete(int dlhandle);
#ifdef EA_PLATFORM_PLAYSTATION2
CmnRequestHandle SNDPLATFORM_downloadNew(int playloc, void *psrc, void *pdst, int size, DmaPriority priority,
                                         DmaTransferType transferType);
int SNDPLATFORM_downloadcompleteNew(CmnRequestHandle cmnRequestHandle);
#endif

// ssine.c
int iSNDsin(int angle);

// svol.c
void iSNDcalcvol(int chan);

// ssysserv.c
void iSNDserveraddclient(void (*pfunc)(void));
void iSNDserverremoveclient(void (*pfunc)(void));

// sballoc.c
int SNDBANKI_alloc();

// sresopat.c
int SNDBANKI_asyncresolvepatch(int playloc, TAGGEDPATCH *ptp, char *pdata, int *pfirstoffset);

// stpparse.c
int SNDI_parsetimbre(void **pptp, SNDIPATCHHEADER *pph);

// scheckpo.c
void SNDI_checkplayopts(SNDPLAYOPTS *pspo);

// stagpat.c
int SNDBANKI_playpatch(void *psampledata, TAGGEDPATCH *ptp, int bhandle, int patnum, struct SNDPLAYOPTS *pspo);

// sclcptch.c
void iSNDcalcpitch(int chan /* r3 */);

// sserver.c
void SNDSYSI_100hzserver();

#ifdef __cplusplus
extern "C" {
#endif

/* Poner a cero: en PC snd tiene su rutina (SNDI_memclear, speed.exe 0x822050, asm de x87 de EA con `fst qword`; la
 * llaman 40 sitios de snd y ninguno llama a memset); GameCube y PS2 usan memset. Solo donde el exe de PC llama a
 * 0x822050: donde PC pone `rep stosd`, el original escribe memset. Con enlace de C (snd es C); pc/sdsound.c la
 * vuelve a declarar sin especificacion y conserva este enlace. Su cuerpo es asm de x86: fuera de MSVC, memset. */
#if defined(EA_PLATFORM_WIN32) && defined(_MSC_VER)
void SNDI_memclear(void *p, int bytes);
#else
#define SNDI_memclear(p, bytes) memset((p), 0, (bytes))
#endif

// s3dlow.c
int SND3dpos(STREAMHANDLE shandle, int azimuth, int elevation);

// saemsamb.c
int SNDAEMS_addmodulebank(void *pBank, char *streamFileName, int streamFileNameOffset, void *(*mallocCb)(void *, int, int));

// saemsmbf.c
int SNDAEMS_asyncloadmodulebank(char *moduleBankFileName, int moduleBankFileOffset, char *streamFileName, int streamFileOffset, void *pMem,
                                int memSize, void *(*mallocCb)(int));
int SNDAEMS_asyncloadmodulebankdone();

// saemsmbm.c
int SNDAEMS_asyncloadmodulebankmem(void *pModuleBank, char *streamFileName, int streamFileOffset, void *(*mallocCb)(int));
int SNDAEMS_asyncloadmodulebankmemdone();

// sattrdef.c
int SND_attrsetdef(SNDSAMPLEATTR *pssa);

// sbadd.c
int SNDbankadd(int *pbhandle, void *pbank);

// sbasync.c
int SNDBANK_asyncload(char *filename, int fileoffset, void *pmem, int memsize, void *(*mallocfn)(int));
int SNDBANK_asyncdone();

// sbasyncm.c
int SNDBANK_asyncloadmem(int *pbhandle, void *pbank);
int SNDBANK_asyncloadmemdone();

// sbhdrsze.c
int SNDbankheadersize(int bhandle);

// sbhdrcpy.c
int SNDbankheadercopy(void *pmem, int bhandle);

// sbplay.c
int SNDBANK_play(int bhandle, int patnum, SNDPLAYOPTS *pspo);

// sbvalid.c
int SNDbankremove(int bhandle);

// sclnt100.c
void SNDSYS_add100hzclient(void (*client)(void));
void SNDSYS_remove100hzclient(void (*client)(void));

// sctrldry.c
int SNDCTRL_drylevel(int shandle, int level);

// sctlfilt.c
int SNDCTRL_filteradd(int shandle, SNDFILTERDEF *psfd);

// stimemul.c
int SNDCTRL_timemult(int shandle, int timemult);

// sfxlevel.c
int SNDfxlevel(int shandle, int bus, int level);

// slowpass.c
int SNDCTRL_lowpass(int shandle, int lowpasscutoff);

// smemlmt.c
int SNDmemlimits(int startaddr, int endaddr);

// smemlu.c
int SNDmemlargestunused(int *paddr);

// filesys_c
int FILESYS_opstatus(int ophandle);

// sstop.c
int SNDstop(int shandle);

// spitch.c
int SNDpitchmult(int shandle, int pitchmult);

// svol.c
int SNDvol(int shandle, int vol);

// smemhigh.cpp
int SNDMEM_gethighwater();

// sover.c (enlace C)
int SNDover(int shandle);

// stream (biblioteca externa): en el ELF conviven STREAM_gettable__Fi (codigo
// del juego, 0x800B89D4) y STREAM_gettable (la biblioteca, 0x80381814). Las de
// aqui son las de la biblioteca, con enlace C.
int STREAM_overhead(int maxrequests, int holdtime, int isgeneric);
unsigned int STREAM_gettable(int streamhandle);

// spktplay.c: la API publica SNDPKTPLAY_* sale sin manglar en el ELF; solo la
// interna SNDPKTPLAYI_* lleva mangling de C++.
int SNDPKTPLAY_overhead(int maxpackets);
int SNDPKTPLAY_create(void (*preleasefunc)(void *, void *), void (*pframesfunc)(int, int, void *),
                      void *pclientdata, void *pmem, int memsize);
int SNDPKTPLAY_destroy(int packetinstancehandle);
int SNDPKTPLAY_start(int packetinstancehandle, SNDSAMPLEFORMAT *pssf, SNDSAMPLEATTR *pssa,
                     SNDPLAYOPTS *pspo);
int SNDPKTPLAY_stop(int packetinstancehandle);
int SNDPKTPLAY_submit(int packetinstancehandle, SNDPACKET *psp);
int SNDPKTPLAY_submitspace(int packetinstancehandle);
// OJO: el volcado DWARF de spktplay.c la define devolviendo `int`, pero la
// DECLARACION que ve el resto de la biblioteca es `unsigned int`: sststat.c
// hace `framesoutstanding(...) * 1000 / samplerate` y el original divide SIN
// signo. Ponerla `int` aqui rompe sststat.c (224 B, medido).
unsigned int SNDPKTPLAY_framesoutstanding(int packetinstancehandle);
int SNDPKTPLAY_purge(int packetinstancehandle, int starthandle, int endhandle);

#ifdef __cplusplus
}
#endif

#endif
