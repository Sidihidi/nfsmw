#ifndef _SIFCMD_H
#define _SIFCMD_H

// Solo lo que usa el driver de sonido de PS2 (snd 9.06.00-ps2, sdriver.c): los comandos del IOP al EE. Los campos y
// tamanos son los de los stabs de la A124 (sceSifCmdHdr 16 B, sceSifCmdData 12 B).

typedef struct {
    unsigned int psize : 8;  // offset 0x0
    unsigned int dsize : 24; // offset 0x1
    unsigned int daddr;      // offset 0x4, size 0x4
    unsigned int fcode;      // offset 0x8, size 0x4
    unsigned int opt;        // offset 0xC, size 0x4
} sceSifCmdHdr;

typedef void (*sceSifCmdHandler)(void *, void *);

typedef struct {
    sceSifCmdHandler func; // offset 0x0, size 0x4
    void *data;            // offset 0x4, size 0x4
    void *gp;              // offset 0x8, size 0x4
} sceSifCmdData;

#ifdef __cplusplus
extern "C" {
#endif

sceSifCmdData *sceSifSetCmdBuffer(sceSifCmdData *db, int size);
void sceSifAddCmdHandler(unsigned int pos, sceSifCmdHandler handler, void *data);
void sceSifExitCmd(void); // psx2/src/platform.cpp, antes de lanzar una demo
unsigned int sceSifSendCmd(unsigned int cid, void *packet, int packet_size, void *src_extra, void *dest_extra,
                           int size_extra); // snd 9.06.00-ps2 (dma.cpp)

#ifdef __cplusplus
}
#endif

#endif
