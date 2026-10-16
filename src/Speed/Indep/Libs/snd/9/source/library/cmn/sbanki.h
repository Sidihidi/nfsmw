#ifndef SND_CMN_BANK_H
#define SND_CMN_BANK_H

#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file/filesys.h"
#include "Speed/Indep/Libs/snd/9/extern/aemsdef.h"
#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"

// total size: 0x10 (PS2: 0x18)
struct SNDASYNCLOADBUFFER {
    FILEOP fileop; // offset 0x0, size 0x4
    int dlhandle; // offset 0x4, size 0x4
#ifdef EA_PLATFORM_PLAYSTATION2
    CmnRequestHandle cmnRequestHandle; // offset 0x8, size 0x8
#endif
    char * ploadbuf; // offset 0x8 (PS2: 0x10), size 0x4
    signed char state; // offset 0xC (PS2: 0x14), size 0x1
    char pad[3]; // offset 0xD (PS2: 0x15), size 0x3
};

// total size: 0x8C (PS2: 0xB8)
struct SNDBANKASYNCINSTANCE {
    bool synctaskadded; // offset 0x0, size 0x1
    bool aborting; // offset 0x4, size 0x1
    bool fileopen; // offset 0x8, size 0x1
    bool failgracefully; // offset 0xC, size 0x1
    FILEOP openop; // offset 0x10, size 0x4
    FILEOP closeop; // offset 0x14, size 0x4
    BANKVER5 *pbank; // offset 0x18, size 0x4
    void *(*mallocfn)(int); // offset 0x1C, size 0x4
    int fileoffset; // offset 0x20, size 0x4
    int fhandle; // offset 0x24, size 0x4
    int dloffset; // offset 0x28, size 0x4
    int totalbanksize; // offset 0x2C, size 0x4
    int hdrxferremaining; // offset 0x30, size 0x4
#ifdef EA_PLATFORM_PLAYSTATION2
    // snd 9.06.00-ps2: la parte del banco que va a la memoria de la IOP
    char *piopcpuram; // offset 0x34, size 0x4
    int iopcpudownloaded; // offset 0x38, size 0x4
    int iopcpuxferremaining; // offset 0x3C, size 0x4
#endif
    char *pspuram; // offset 0x34 (PS2: 0x40), size 0x4
    int spudownloaded; // offset 0x38 (PS2: 0x44), size 0x4
    int spuxferremaining; // offset 0x3C (PS2: 0x48), size 0x4
    short unsigned int loadbufsize; // offset 0x40 (PS2: 0x4C), size 0x2
    short unsigned int bhandle; // offset 0x42 (PS2: 0x4E), size 0x2
    short unsigned int readsissued; // offset 0x44 (PS2: 0x50), size 0x2
    short unsigned int readscompleted; // offset 0x46 (PS2: 0x52), size 0x2
    short unsigned int processesissued; // offset 0x48 (PS2: 0x54), size 0x2
    short unsigned int processescompleted; // offset 0x4A (PS2: 0x56), size 0x2
    SNDASYNCLOADBUFFER salb[4]; // offset 0x4C (PS2: 0x58), size 0x40 (PS2: 0x60)
};

// total size: 0xC
struct SNDBANKASYNCSTATE {
    SNDBANKASYNCINSTANCE *pi; // offset 0x0, size 0x4
    int readpriority; // offset 0x4, size 0x4
    int lasterror; // offset 0x8, size 0x4
};

// total size: 0x14 (PS2: 0x18)
struct SNDBANKASYNCMEMSTATE {
    char loading; // offset 0x0, size 0x1
    char pad; // offset 0x1, size 0x1
    short unsigned int bhandle; // offset 0x2, size 0x2
    int dlhandle; // offset 0x4, size 0x4
    CmnRequestHandle cmnRequestHandle; // offset 0x8, size 0x8
#ifdef EA_PLATFORM_PLAYSTATION2
    int iopcpudownloaded; // offset 0x10, size 0x4
#endif
    int spudownloaded; // offset 0x10 (PS2: 0x14), size 0x4
};

void SNDBANKI_abortload();
void SNDBANKI_asyncresolve();
void SNDBANKI_asynccompletereads();
void SNDBANKI_asyncissuereads();
void SNDBANKI_asyncxferhdr();
void SNDBANKI_asynccompletedownload();
void SNDBANKI_asyncissuedownloads();
void SNDBANKI_asyncprocess();
void SNDBANKI_asyncserver();
void SNDBANKI_asyncservice();
int SNDBANK_asyncloadi(char *filename, int fileoffset, void *pmem, int memsize, void *(* mallocfn)(int), bool failgracefully);

#ifdef __cplusplus
extern "C" {
#endif

int SNDBANK_asyncloadz(char *filename, int fileoffset, void *pmem, int memsize, void * (* mallocfn)(int));
int SNDBANK_asyncload(char *filename, int fileoffset, void *pmem, int memsize, void * (* mallocfn)(int));

#ifdef __cplusplus
}
#endif

#endif
