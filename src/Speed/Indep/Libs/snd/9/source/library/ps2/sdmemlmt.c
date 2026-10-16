// Los limites de la memoria del SPU que reparte el driver de PS2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/
// sdmemlmt.c), en bloques de 64 bytes; el principio no puede bajar de 0x141 (lo que ocupa el driver) ni el final pasar
// de 0x8000 (los 2 MB del SPU2).

#include "sndps2.h"

int SNDPLATFORM_memlimits(int startaddr, int endaddr) {
    if (startaddr == -1) {
        startaddr = 0x141;
    } else {
        startaddr = (startaddr + 63) >> 6;
    }
    if (startaddr < 0x141) {
        return SNDERR_INV_PARAM;
    }
    snddrv.mm.startaddr = startaddr;

    if (endaddr == -1) {
        endaddr = 0x8000;
    } else {
        endaddr = (endaddr + 63) >> 6;
    }
    if (endaddr > 0x8000) {
        return SNDERR_INV_PARAM;
    }
    snddrv.mm.endaddr = endaddr;
    return 0;
}
