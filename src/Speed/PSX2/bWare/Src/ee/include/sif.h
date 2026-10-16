#ifndef _SIF_H
#define _SIF_H

// Solo lo que usa el juego de sif (psx2/src/rebootiop.cpp): la DMA del EE al IOP.

typedef struct {
    unsigned int data; // offset 0x0, size 0x4
    unsigned int addr; // offset 0x4, size 0x4
    unsigned int size; // offset 0x8, size 0x4
    unsigned int mode; // offset 0xC, size 0x4
} sceSifDmaData;

#ifdef __cplusplus
extern "C" {
#endif

unsigned int sceSifSetDma(sceSifDmaData *sdd, int len);
int sceSifDmaStat(unsigned int id);
unsigned int isceSifSetDma(sceSifDmaData *sdd, int len);
int isceSifDmaStat(unsigned int id);

#ifdef __cplusplus
}
#endif

#endif
