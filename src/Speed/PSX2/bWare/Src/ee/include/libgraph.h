#ifndef _LIBGRAPH_H
#define _LIBGRAPH_H

// Solo lo que usa el juego de libgraph (psx2/src/platform.cpp y psx2/src/ecstasy/EcstasyE.cpp).

#define SCE_GS_NOINTERLACE 0
#define SCE_GS_INTERLACE 1
#define SCE_GS_FIELD 1
#define SCE_GS_FRAME 0
#define SCE_GS_NTSC 2
#define SCE_GS_PAL 3

// total size: 0x10 (stabs de la A124); lo usa el driver de sonido de PS2 (sdriver.c: el modo de video para el
// periodo del temporizador)
typedef struct {
    short sceGsInterMode;     // offset 0x0, size 0x2
    short sceGsOutMode;       // offset 0x2, size 0x2
    short sceGsFFMode;        // offset 0x4, size 0x2
    short sceGsVersion;       // offset 0x6, size 0x2
    int (*sceGsVSCfunc)(int); // offset 0x8, size 0x4
    int sceGsVSCid;           // offset 0xC, size 0x4
} sceGsGParam;

#ifdef __cplusplus
extern "C" {
#endif

int sceGsSyncVCallback(int (*func)(int));
void sceGsResetPath(void);
void sceGsResetGraph(short mode, short inter, short omode, short ffmode);
sceGsGParam *sceGsGetGParam(void);

#ifdef __cplusplus
}
#endif

#endif
