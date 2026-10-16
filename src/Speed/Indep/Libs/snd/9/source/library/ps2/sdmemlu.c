// El mayor hueco libre de la memoria del SPU en el driver de PS2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/
// sdmemlu.c): el mismo recorrido que el de la ARAM de GameCube (gc/saramman.c), hasta la zona de la reverberacion.

#include "sndps2.h"

int SNDPLATFORM_memlargestunused(int *paddr) {
    int i;
    int largest = 0;
    int freespace;
    int freeaddr;
    SNDDRVMEMREC *pcurrec;
    SNDDRVMEMREC *pprevrec;

    if (snddrv.mm.nummallocs == 0) {
        freeaddr = snddrv.mm.startaddr;
        freespace = snddrv.mm.endaddr - snddrv.mm.startaddr;
        SNDDRV_memconstrain(&freeaddr, &freespace);
        *paddr = freeaddr;
        largest = freespace;
        goto success;
    }
    for (i = 0; i < snddrv.mm.nummallocs; i++) {
        pcurrec = &snddrv.mm.pr[i];
        if (i == 0) {
            freeaddr = pcurrec->addr;
            freespace = pcurrec->addr - 0x141;
            SNDDRV_memconstrain(&freeaddr, &freespace);
            if (freespace > largest) {
                *paddr = freeaddr;
                largest = freespace;
            }
        } else {
            pprevrec = &snddrv.mm.pr[i - 1];
            freeaddr = pprevrec->addr + pprevrec->size;
            freespace = pcurrec->addr - freeaddr;
            SNDDRV_memconstrain(&freeaddr, &freespace);
            if (freespace > largest) {
                *paddr = freeaddr;
                largest = freespace;
            }
        }
    }
    pprevrec = &snddrv.mm.pr[i - 1];
    freeaddr = pprevrec->addr + pprevrec->size;
    freespace = snddrv.mm.reverbaddr - freeaddr;
    SNDDRV_memconstrain(&freeaddr, &freespace);
    if (freespace > largest) {
        *paddr = freeaddr;
        largest = freespace;
    }
success:
    *paddr <<= 6;
    return largest << 6;
}
