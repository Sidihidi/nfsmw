// El reparto de la memoria del SPU en el driver de PS2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/sdmemman.c):
// el mismo algoritmo que el de la ARAM de GameCube (gc/saramman.c), en bloques de 64 bytes y con los limites de
// snddrv.mm; la memoria del IOP se pide al IOP.

#include "sndps2.h"

void SNDDRV_memconstrain(int *pfreeaddr, int *pfreespace) {
    int offset;

    if (*pfreeaddr < snddrv.mm.startaddr) {
        offset = snddrv.mm.startaddr - *pfreeaddr;
        *pfreeaddr = snddrv.mm.startaddr;
        *pfreespace -= offset;
    }
    if (*pfreeaddr + *pfreespace > snddrv.mm.endaddr) {
        *pfreespace = snddrv.mm.endaddr - *pfreeaddr;
    }
    if (*pfreeaddr + *pfreespace > snddrv.mm.reverbaddr) {
        *pfreespace = snddrv.mm.reverbaddr - *pfreeaddr;
    }
}

unsigned int SNDPLATFORM_memalloc(int playloc, int size) {
    int i = 0;
    int j;
    int freespace;
    int freeaddr;
    SNDDRVMEMREC *pcurrec;
    SNDDRVMEMREC *pprevrec;

    if (playloc == SND_PLAYLOC_IOPCPU) {
        return (unsigned int)sceSifAllocIopHeap(size + 64);
    }

    if (snddrv.mm.nummallocs >= sndgs.sso.set.maxbanks) {
        return 0;
    }
    size += 63;
    size >>= 6;
    if (snddrv.mm.nummallocs == 0) {
        freeaddr = snddrv.mm.startaddr;
        freespace = snddrv.mm.endaddr - snddrv.mm.startaddr;
        SNDDRV_memconstrain(&freeaddr, &freespace);
        if (size <= freespace) {
            goto success;
        }
        return 0;
    }
    for (i = 0; i < snddrv.mm.nummallocs; i++) {
        pcurrec = &snddrv.mm.pr[i];
        if (i == 0) {
            freeaddr = snddrv.mm.startaddr;
            freespace = pcurrec->addr - freeaddr;
        } else {
            pprevrec = &snddrv.mm.pr[i - 1];
            freeaddr = pprevrec->addr + pprevrec->size;
            freespace = pcurrec->addr - freeaddr;
        }
        SNDDRV_memconstrain(&freeaddr, &freespace);
        if (size <= freespace) {
            for (j = snddrv.mm.nummallocs; j > i; j--) {
                snddrv.mm.pr[j] = snddrv.mm.pr[j - 1];
            }
            goto success;
        }
    }
    pprevrec = &snddrv.mm.pr[i - 1];
    freeaddr = pprevrec->addr + pprevrec->size;
    freespace = snddrv.mm.endaddr - freeaddr;
    SNDDRV_memconstrain(&freeaddr, &freespace);
    if (size > freespace) {
        return 0;
    }
success:
    pcurrec = &snddrv.mm.pr[i];
    pcurrec->addr = freeaddr;
    pcurrec->size = size;
    snddrv.mm.nummallocs++;
    return freeaddr << 6;
}

int SNDPLATFORM_memfree(int playloc, unsigned int address) {
    int i;
    SNDDRVMEMREC *prec;

    if (playloc == SND_PLAYLOC_IOPCPU) {
        return sceSifFreeIopHeap((void *)address);
    }

    address >>= 6;
    for (i = 0; i < snddrv.mm.nummallocs; i++) {
        prec = &snddrv.mm.pr[i];
        if (prec->addr == address) {
            snddrv.mm.nummallocs--;
            for (; i < snddrv.mm.nummallocs; i++) {
                snddrv.mm.pr[i] = snddrv.mm.pr[i + 1];
            }
            return 0;
        }
    }
    return SNDERR_INV_PARAM;
}
