#ifndef _SIFRPC_H
#define _SIFRPC_H

// Solo lo que usa el juego de sifrpc: psx2/src/rebootiop.cpp y el monitor de red de SuperBender
// (indep/bware/src/psx2/SuperBenderPSX2.cpp). Los layouts, de los stabs de la A124.

typedef void (*sceSifEndFunc)(void *);

typedef struct _sif_rpc_data {
    void *paddr;       // offset 0x0, size 0x4
    unsigned int pid;  // offset 0x4, size 0x4
    int tid;           // offset 0x8, size 0x4
    unsigned int mode; // offset 0xC, size 0x4
} sceSifRpcData;

typedef struct _sif_client_data {
    struct _sif_rpc_data rpcd;     // offset 0x0, size 0x10
    unsigned int command;          // offset 0x10, size 0x4
    void *buff;                    // offset 0x14, size 0x4
    void *gp;                      // offset 0x18, size 0x4
    sceSifEndFunc func;            // offset 0x1C, size 0x4
    void *para;                    // offset 0x20, size 0x4
    struct _sif_serve_data *serve; // offset 0x24, size 0x4
} sceSifClientData;

#ifdef __cplusplus
extern "C" {
#endif

void sceSifInitRpc(unsigned int mode);
int sceSifBindRpc(sceSifClientData *client, unsigned int request, unsigned int mode);
int sceSifCallRpc(sceSifClientData *client, unsigned int fno, unsigned int mode, void *send, int ssize, void *receive, int rsize,
                  sceSifEndFunc end_func, void *end_para);
int sceSifCheckStatRpc(sceSifRpcData *cd); // el driver de sonido de PS2 (snd 9.06.00-ps2, sdriver.c)
void *sceSifAllocIopHeap(unsigned int size); // el monton del IOP: snd 9.06.00-ps2 (dma.cpp, sdmemman.c)
int sceSifFreeIopHeap(void *addr);

#ifdef __cplusplus
}
#endif

#endif
