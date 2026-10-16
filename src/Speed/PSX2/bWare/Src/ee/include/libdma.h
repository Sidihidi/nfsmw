#ifndef _LIB_DMA_H_
#define _LIB_DMA_H_

// Solo lo que usa el juego de libdma (psx2/src/ecstasy/EcstasyE.cpp). Los layouts de sceDmaTag, sceDmaEnv y
// sceDmaChan son los de los stabs de la A124.

#include <eetypes.h>
#include <eeregs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _sceDmaTag {
    u_short qwc;
    u_char mark;
    u_char id;
    struct _sceDmaTag *next;
    u_int p[2];
} sceDmaTag __attribute__((aligned(16)));

typedef struct {
    u_char sts;
    u_char std;
    u_char mfd;
    u_char rcycle;
    u_short express;
    u_short notify;
    u_short sqwc;
    u_short tqwc;
    void *rbadr;
    u_int rbmsk;
} sceDmaEnv;

typedef struct {
    tD_CHCR chcr;
    u_int p0[3];
    void *madr;
    u_int p1[3];
    u_int qwc;
    u_int p2[3];
    sceDmaTag *tadr;
    u_int p3[3];
    void *as0;
    u_int p4[3];
    void *as1;
    u_int p5[3];
    u_int p6[4];
    u_int p7[4];
    void *sadr;
    u_int p8[3];
} sceDmaChan;

int sceDmaReset(int mode);
sceDmaChan *sceDmaGetChan(int id);
void sceDmaSend(sceDmaChan *d, void *tag);
int sceDmaSync(sceDmaChan *d, int mode, int timeout);

#ifdef __cplusplus
}
#endif

#endif
