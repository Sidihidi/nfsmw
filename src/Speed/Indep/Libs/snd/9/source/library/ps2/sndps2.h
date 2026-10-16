#ifndef SNDPS2_H
#define SNDPS2_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// El driver de PlayStation 2 de snd 9.06.00-ps2 (library/ps2/ en el .mdebug de la A124): el EE mezcla las voces de la
// CPU principal y le pasa al IOP, en un buffer de etiquetas por DMA de la SIF, las voces del SPU y del IOP. Los tipos
// son los de los stabs de la A124, campo a campo.

#include "../cmn/sndcmn.h"
#include <eekernel.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>

// playloc de PS2 (la CPU principal es SND_PLAYLOC_MAINCPU, 4, de sndo.h)
#define SND_PLAYLOC_SPU 0x8 // nombre adivinado: el valor es del binario, el nombre no sale en ninguno
#define SND_PLAYLOC_IOPCPU 0x100 // nombre adivinado, como el anterior

enum StartMode {
    COLD = 0,
    WARM = 1,
};

// 64 bits: en los ee-gcc 2.95.3 de SN `long` es de 64 y `long long` de 128
typedef long SNDINT64;

enum tCommandPort {
    kComPort_IOP = 0,
    kComPort_EE = 1,
};

// total size: 0x14
struct SNDIOPRPCINIT {
    unsigned int ver;                     // offset 0x0, size 0x4
    unsigned int ioptoeeaddr;             // offset 0x4, size 0x4
    unsigned int dtspcmaddr;              // offset 0x8, size 0x4
    unsigned short outputratemaincpu;     // offset 0xC, size 0x2
    unsigned short outputrateiopcpu;      // offset 0xE, size 0x2
    unsigned char outputchannels;         // offset 0x10, size 0x1
    unsigned char outputlfe;              // offset 0x11, size 0x1
    unsigned char voicesiopcpu;           // offset 0x12, size 0x1
    unsigned char routespufxtoiopcpu;     // offset 0x13, size 0x1
};

// total size: 0x8
typedef struct {
    unsigned int startaddr; // offset 0x0, size 0x4
    unsigned short pitch;   // offset 0x4, size 0x2
    signed char vol[2];     // offset 0x6, size 0x2
} SNDDRVEETOSPUVOICE;

// total size: 0x10
typedef struct {
    unsigned int startaddr;     // offset 0x0, size 0x4
    unsigned short pitch;       // offset 0x4, size 0x2
    signed char drybusgain[6];  // offset 0x6, size 0x6
    signed char wetbusgain[1];  // offset 0xC, size 0x1
    signed char pad[3];         // offset 0xD, size 0x3
} SNDDRVEETOIOPVOICE;

// total size: 0x2A0
typedef struct {
    SNDINT64 wetbusbits;                  // offset 0x0, size 0x8
    SNDINT64 drybusbits;                  // offset 0x8, size 0x8
    signed char effecttype;               // offset 0x10, size 0x1
    signed char effectdelay;              // offset 0x11, size 0x1
    signed char effectfeedback;           // offset 0x12, size 0x1
    unsigned char iopcpuresamplequality;  // offset 0x13, size 0x1
    short effectvolleft;                  // offset 0x14, size 0x2
    short effectvolright;                 // offset 0x16, size 0x2
    int effectOutputLevelIop;             // offset 0x18, size 0x4
    char pad[4];                          // offset 0x1C, size 0x4
    SNDDRVEETOSPUVOICE sv[48];            // offset 0x20, size 0x180
    SNDDRVEETOIOPVOICE iv[16];            // offset 0x1A0, size 0x100
} SNDDRVEETOIOP;

// total size: 0x8
typedef struct {
    unsigned int startaddr; // offset 0x0, size 0x4
    unsigned int curaddr;   // offset 0x4, size 0x4
} SNDDRVSPUTOEEVOICE;

// total size: 0x8
typedef struct {
    unsigned int startaddr; // offset 0x0, size 0x4
    int curframe;           // offset 0x4, size 0x4
} SNDDRVIOPTOEEVOICE;

// total size: 0x240
typedef struct {
    unsigned int seqnumtop;       // offset 0x0, size 0x4
    int iopcpu;                   // offset 0x4, size 0x4
    char pad[52];                 // offset 0x8, size 0x34
    SNDDRVSPUTOEEVOICE sv[48];    // offset 0x3C, size 0x180
    SNDDRVIOPTOEEVOICE iv[16];    // offset 0x1BC, size 0x80
    unsigned int seqnumbottom;    // offset 0x23C, size 0x4
} SNDDRVIOPTOEE;

// total size: 0x4
struct tCmdHandlerData {
    char pad[4]; // offset 0x0, size 0x4
};

// Los mensajes de la SIF van alineados a 16: los stabs los dan de 0x20 y 0x30 con los campos acabando en 0x14 y 0x24
// total size: 0x20
struct tCommandData {
    sceSifCmdHdr Hdr; // offset 0x0, size 0x10
    int cmd;          // offset 0x10, size 0x4
} __attribute__((aligned(16)));

// total size: 0x30
struct DMAMSG {
    sceSifCmdHdr Hdr;       // offset 0x0, size 0x10
    int cmd;                // offset 0x10, size 0x4
    int handle;             // offset 0x14, size 0x4
    unsigned int psrc;      // offset 0x18, size 0x4
    unsigned int pdst;      // offset 0x1C, size 0x4
    unsigned short length;  // offset 0x20, size 0x2
    short priority;         // offset 0x22, size 0x2
} __attribute__((aligned(16)));

// total size: 0x20
struct DMACOMPLETEMSG {
    sceSifCmdHdr Hdr; // offset 0x0, size 0x10
    int cmd;          // offset 0x10, size 0x4
} __attribute__((aligned(16)));

// Las etiquetas del buffer que viaja al IOP: cada bloque va precedido de su tipo (un int).
// total size: 0xC
struct SNDIOPTAGHEADER {
    unsigned int audiotick;  // offset 0x0, size 0x4
    unsigned int tagbufsize; // offset 0x4, size 0x4
    char pad[4];             // offset 0x8, size 0x4
};

// total size: 0xC
struct SNDIOPTAGMIXBUF {
    unsigned short bufsize; // offset 0x0, size 0x2
    char pad[10];           // offset 0x2, size 0xA
};

// total size: 0xC
struct SNDIOPTAGFXBUF {
    char pad[12]; // offset 0x0, size 0xC
};

// total size: 0x4
struct SNDIOPTAGVOICECMD {
    int datasize; // offset 0x0, size 0x4
};

// total size: 0xC
struct SNDIOPTAGDTSBUF {
    char pad[12]; // offset 0x0, size 0xC
};

// total size: 0xC
struct SNDIOPTAGFOOTER {
    unsigned int audiotick; // offset 0x0, size 0x4
    char pad[8];            // offset 0x4, size 0x8
};

// total size: 0x4
typedef struct {
    unsigned short addr; // offset 0x0, size 0x2
    unsigned short size; // offset 0x2, size 0x2
} SNDDRVMEMREC;

// total size: 0xC
typedef struct {
    unsigned short nummallocs; // offset 0x0, size 0x2
    unsigned short startaddr;  // offset 0x2, size 0x2
    unsigned short endaddr;    // offset 0x4, size 0x2
    unsigned short reverbaddr; // offset 0x6, size 0x2
    SNDDRVMEMREC *pr;          // offset 0x8, size 0x4
} SNDDRVMEMSTATE;

// total size: 0x20
typedef struct {
    float balances[6];       // offset 0x0, size 0x18
    unsigned int sampleaddr; // offset 0x18, size 0x4
    short prevlvol;          // offset 0x1C, size 0x2
    short prevrvol;          // offset 0x1E, size 0x2
} SNDDRVVOICE;

// total size: 0x10
typedef struct {
    unsigned int handle;     // offset 0x0, size 0x4
    unsigned int psrc;       // offset 0x4, size 0x4
    unsigned int pdst;       // offset 0x8, size 0x4
    unsigned short length;   // offset 0xC, size 0x2
    unsigned char priority;  // offset 0xE, size 0x1
    char pad;                // offset 0xF, size 0x1
} SNDDMAMSG;

// total size: 0x1DC
typedef struct {
    unsigned int curhandle;        // offset 0x0, size 0x4
    signed char outstanding;       // offset 0x4, size 0x1
    signed char transferringslot;  // offset 0x5, size 0x1
    signed char active;            // offset 0x6, size 0x1
    char pad;                      // offset 0x7, size 0x1
    unsigned int transferedat;     // offset 0x8, size 0x4
    SNDDMAMSG msg[28];             // offset 0xC, size 0x1C0
    sceSifDmaData sdd[1];          // offset 0x1CC, size 0x10
} SNDDMAMANAGER;

// total size: 0x1B80
// Mide 0x1B80 en los stabs y sus campos llegan a 0x1B44: la estructura va alineada a 64 bytes (la linea de cache;
// el buffer de la RPC y las zonas que escribe la DMA del IOP empiezan en multiplos de 64).
typedef struct {
    unsigned char rpcsendbuf[2048] __attribute__((aligned(64))); // offset 0x0, size 0x800
    SNDDRVIOPTOEE ioptoee;          // offset 0x800, size 0x240
    SNDDRVIOPTOEE ioptoeedma;       // offset 0xA40, size 0x240
    sceSifClientData rpccd;         // offset 0xC80, size 0x28
    SNDDRVIOPTOEE *pioptoee;        // offset 0xCA8, size 0x4
    SNDDRVIOPTOEE *pioptoeedma;     // offset 0xCAC, size 0x4
    SNDDRVEETOIOP *peetoiop;        // offset 0xCB0, size 0x4
    SNDIOPTAGHEADER *psith;         // offset 0xCB4, size 0x4
    SNDIOPTAGFOOTER *psitf;         // offset 0xCB8, size 0x4
    SNDDMAMANAGER dm;               // offset 0xCBC, size 0x1DC
    SNDDRVVOICE voice[96];          // offset 0xE98, size 0xC00
    SNDDRVMEMSTATE mm;              // offset 0x1A98, size 0xC
    int serversema;                 // offset 0x1AA4, size 0x4
    int vblhandlerid;               // offset 0x1AA8, size 0x4
    char pad;                       // offset 0x1AAC, size 0x1
    signed char installed;          // offset 0x1AAD, size 0x1
    unsigned short scratchpadsize;  // offset 0x1AAE, size 0x2
    short framestomix;              // offset 0x1AB0, size 0x2
    unsigned short tagindex;        // offset 0x1AB2, size 0x2
    unsigned char *ptagbuf;         // offset 0x1AB4, size 0x4
    unsigned int startaddrsync;     // offset 0x1AB8, size 0x4
    void *pmixbuf[6];               // offset 0x1ABC, size 0x18
    unsigned char *palloceddtsbuf;  // offset 0x1AD4, size 0x4
    unsigned char *porigdtsmixbuf;  // offset 0x1AD8, size 0x4
    void *pdtsmixbuf[6];            // offset 0x1ADC, size 0x18
    unsigned char *pdtsbuf;         // offset 0x1AF4, size 0x4
    void *pdtspermbuf;              // offset 0x1AF8, size 0x4
    void *pdtstempbuf;              // offset 0x1AFC, size 0x4
    sceSifDmaData sifdma;           // offset 0x1B00, size 0x10
    // el cerrojo lo comparten el hilo del driver y los del juego: el original lee y escribe estos tres siempre en
    // memoria y nunca en la ranura de retardo de una llamada (SNDI_mutex*), como un volatile
    volatile int mutex;             // offset 0x1B10, size 0x4
    volatile int mutexowner;        // offset 0x1B14, size 0x4
    volatile int mutexlocks;        // offset 0x1B18, size 0x4
    unsigned int SP_d8_madr;        // offset 0x1B1C, size 0x4
    unsigned int SP_d8_sadr;        // offset 0x1B20, size 0x4
    unsigned int SP_d8_qwc;         // offset 0x1B24, size 0x4
    unsigned int SP_d9_madr;        // offset 0x1B28, size 0x4
    unsigned int SP_d9_tadr;        // offset 0x1B2C, size 0x4
    unsigned int SP_d9_sadr;        // offset 0x1B30, size 0x4
    unsigned int SP_d9_qwc;         // offset 0x1B34, size 0x4
    void *SP_buff;                  // offset 0x1B38, size 0x4
    float curFxOutputLevel;         // offset 0x1B3C, size 0x4
    float newFxOutputLevel;         // offset 0x1B40, size 0x4
} SNDDRV;

// La configuracion del mezclador de la CPU principal (mix/smixer.c)
// total size: 0xC
struct MIXCONFIG {
    int outputrate;                  // offset 0x0, size 0x4
    void (*chanovercallback)(int);   // offset 0x4, size 0x4
    unsigned char inputchannels;     // offset 0x8, size 0x1
    unsigned char outputchannels;    // offset 0x9, size 0x1
    unsigned char outputinterleaved; // offset 0xA, size 0x1
};

extern SNDDRV snddrv;

// El encoder de DTS (una biblioteca aparte que el juego registra si la usa)
extern int (*sndpdtsquerymemsize)(int, int, int *, int *);
extern int (*sndpdtsinit)(int, int, void *);
extern int (*sndpdtsencode)(void **, unsigned char *, void *);

namespace Snd {

extern int gVariableTimerTick; // cmn/sserver.c

// total size: 0x20
struct DmaRequest {
    CListNode linkNode;           // offset 0x0, size 0x4
    int playloc;                  // offset 0x4, size 0x4
    unsigned int pSrc;            // offset 0x8, size 0x4
    unsigned int pDst;            // offset 0xC, size 0x4
    int length;                   // offset 0x10, size 0x4
    unsigned int handle;          // offset 0x14, size 0x4
    DmaPriority priority;         // offset 0x18, size 0x4
    DmaTransferType transferType; // offset 0x1C, size 0x4
};

// total size: 0x8
struct DmaRequestHandle {
    unsigned int handle;  // offset 0x0, size 0x4
    DmaRequest *pRequest; // offset 0x4, size 0x4
};

// cmn/dma.cpp: las transferencias DMA de la SIF hacia el IOP (a la memoria del IOP, o a la del SPU pasando por un
// buffer del IOP), en tres colas por prioridad que vacia un hilo
struct Dma {
  public:
    static Result Init();
    static Result Restore();
    static DmaRequestHandle QueueRequest(int playloc, unsigned int pSrc, unsigned int pDst, int length,
                                         DmaPriority priority, DmaTransferType transferType);
    static int IsComplete(DmaRequestHandle requestHandle);
    static int GetMaxDMALength();
    static void CompletionHandler(unsigned int handle);

  private:
    static void DmaThread(void *data);

    // los stabs los dan como static int de la clase (sin direccion): constantes; las que el codigo no usa no se sabe
    // cuanto valen y no estan
    static const int MIN_FREELIST_ENTRIES = 5;
    static const int DM_MAX_LENGTH = 0x1000;
    static const int NUM_LISTS = 3;
    static const int MAX_DMA_REQUESTS = 28;
    static const int DMA_THREAD_STACK_SIZE = 0x800;

    static CListQueue sListQueued[NUM_LISTS];
    static CListQueue sListFree;
    static DmaRequest sRequestBuff[MAX_DMA_REQUESTS];
    static int sDmaActive;
    static volatile unsigned int sTransferedAt;
    static int sHandleSeqNum;
    static unsigned int sLastQueueID;
    static int sDmaThreadSema;
    static int sDmaThreadID;
    static char sDmaThreadStack[DMA_THREAD_STACK_SIZE];
    static sceSifDmaData sSpuDmaDataTemp;
};

} // namespace Snd

// sdriver.c
void SNDDRV_savescratchpad();
void SNDDRV_restorescratchpad();
void SNDDRV_mixvoicefree(int voice);
void SNDDRV_updatevoices();
void SNDDRV_iopinterrupt(void *p, void *q);
void SNDDRV_strcat(char *pdst, char *psrc);
void SNDDRV_resettagbuf();
int SNDDRV_vblankcb(int ca);
void SNDDRV_thread(void *data);
int SNDPLATFORM_outputcaps();
int SNDPLATFORM_outputset();
int SNDPLATFORM_init(StartMode startMode);
int SNDPLATFORM_restore(StartMode startMode);
void iSNDserve();
void SNDI_mutexalloc();
void SNDI_mutexfree();
void SNDI_mutexlock();
void SNDI_mutexunlock();
int SNDPLATFORM_ModeSetUp(SNDSYSOPTS *pSndSysOpts, Snd::OutputMode outputmode);

// sdmadrv.c: el scratchpad con la DMA
void SNDDMAI_ToScratchPad(unsigned int *psrc, unsigned int *pdst, unsigned int qwc, int mode);
void SNDDMAI_FromScratchPad(unsigned int *psrc, unsigned int *pdst, unsigned int qwc, int mode);

// cmn/sserver.c
void SNDSYSI_timerservice(int ticks);
void SNDSYSI_variabletimerservice();

// cmn/spktplay.c
void SNDPKTPLAYI_flushcallbackdata();

// cmn/satospkr.c
void SNDI_aztospkrvol(int azimuth, float *balance);

extern "C" {

void SNDDRV_rpc(int command, void *pdata, int datasize);
void SNDDRV_setspufxmem(int size);
void SNDDRV_memconstrain(int *pfreeaddr, int *pfreespace); // sdmemman.c

// mix/: el mezclador de la CPU principal
extern unsigned int SNDMIXfastramfree;
void MIX_create(MIXCONFIG *pmc);
void MIX_destroy();
void MIX_audio(void **poutput, int frames);
void MIX_playinit(int voice, int samplerep, int playtype, void *psample, void *ploopstart, void *ptsdata, int numchan,
                  int totalframes, int sustainstart, int sustainend, void *pcoeftable, void *ploopstate,
                  int platformver, int requester);
void MIX_play(int voice);
int MIX_getframe(int voice);
void MIX_filteradd(int voice, int channel, SNDFILTERDEF *psfd);
unsigned int MIX_initreverb(int outputrate, int fxBusIndex, void *pFxFile);
unsigned int MIX_restorereverb(int fxBusIndex, unsigned int instanceHandle);
void MIX_inbufInit(int fxBusIndex);
void MIX_inbufRestore(int fxBusIndex);
void SNDMIXI_fxadd(int frames);
void MIX_stop(int voice);
void MIX_setpitch(int voice, int pitch);
void MIX_settimemult(int voice, int timemult);
void MIX_setlowpass(int voice, float leakage);
void MIX_sethighpass(int voice, int cutofffreq);
void MIX_setresamplequality(int quality);
void MIX_setwetbuffer(void *pbuf);
void SNDMIX_setdrygain(int voice, int channel, float gain);
void SNDMIX_setwetgain(int voice, int bus, float gain);

void *memset(void *p, int c, unsigned int n);
}

#endif
