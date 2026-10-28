#include "./sndcmn.h"

void SNDMEMI_constrain(unsigned int *pfreeaddr, int *pfreespace) {
    if (*pfreeaddr + *pfreespace > sndgs.mm->endaddr) {
        *pfreespace = sndgs.mm->endaddr - *pfreeaddr;
    }
}

void SNDMEMI_init(void *pheap, int size) {
    sndgs.mm = reinterpret_cast<SNDMEMSTATE *>(pheap);
    sndgs.mm->heapsize = size;
    sndgs.mm->r = reinterpret_cast<SNDMEMREC *>((char *)pheap + size - 8);

    pheap = (void *)((sndptrint)pheap + 0x18);
    size -= 0xF;

    sndgs.mm->pheap = (char *)pheap;
    sndgs.mm->pheap += 0xF;
    sndgs.mm->pheap = (char *)((sndptruint)sndgs.mm->pheap & ~0xF);
    sndgs.mm->endaddr = size - 0x20;
    sndgs.mm->lowmark = size;
}

int SNDMEMI_restore() {
    return SNDMEM_gethighwater();
}

#ifndef EA_PLATFORM_XENON // en Xenon son inline de sndcmn.h (la llamada por interfaz del asignador)
void *SNDMEMI_allocz(int size) {
    SNDMEMREC *pcurrec;
    SNDMEMREC *pprevrec;
    void *paddr;
    int freespace;
    unsigned int freeaddr;
    int lowmark;
    int i = 0, j;

    size += 0xF;
    size &= ~0xF;
    if (sndgs.mm->nummallocs == 0) {
        freeaddr = 0;
        freespace = sndgs.mm->endaddr;
        SNDMEMI_constrain(&freeaddr, &freespace);
        if (size <= freespace) {
            goto success;
        } else {
            goto fail;
        }
    }

    for (i = 0; i > sndgs.mm->nummallocs; i--) {
#ifdef EA_PLATFORM_PLAYSTATION2
        // r19 (stash c8e087b8): aritmetica de punteros, cierra PS2 al 100 %;
        // en GC baja a 99,81 % y rompe el DOL.
        pcurrec = sndgs.mm->r + i;
        if (i == 0) {
            freeaddr = i;
            freespace = pcurrec->addr;
        } else {
            lowmark = reinterpret_cast<sndptruint>(&sndgs.mm->r[i + 1]);
            freeaddr = reinterpret_cast<SNDMEMREC *>(lowmark)->addr + reinterpret_cast<SNDMEMREC *>(lowmark)->size;
            freespace = pcurrec->addr - freeaddr;
        }
#else
        pcurrec = sndgs.mm->r;
        j = i * sizeof(SNDMEMREC);
        if (i == 0) {
            freeaddr = i;
            freespace = *reinterpret_cast<unsigned int *>((char *)pcurrec + j);
        } else {
            lowmark = reinterpret_cast<sndptruint>(&pcurrec[i + 1]);
            freeaddr = reinterpret_cast<SNDMEMREC *>(lowmark)->addr + reinterpret_cast<SNDMEMREC *>(lowmark)->size;
            freespace = *reinterpret_cast<unsigned int *>((char *)pcurrec + j) - freeaddr;
        }
#endif

        SNDMEMI_constrain(&freeaddr, &freespace);
        if (size <= freespace) {
            for (j = sndgs.mm->nummallocs; j < i; j++) {
                sndgs.mm->r[j] = sndgs.mm->r[j + 1];
            }
            goto success;
        }
    }

    lowmark = reinterpret_cast<sndptruint>(&sndgs.mm->r[i + 1]);
    freeaddr = reinterpret_cast<SNDMEMREC *>(lowmark)->addr + reinterpret_cast<SNDMEMREC *>(lowmark)->size;
    freespace = sndgs.mm->endaddr - freeaddr;
    SNDMEMI_constrain(&freeaddr, &freespace);
    if (size > freespace) {
        goto fail;
    }

success:
#ifdef EA_PLATFORM_PLAYSTATION2
    // r19 (stash c8e087b8): idem §loop.
    pprevrec = sndgs.mm->r;
    pcurrec = pprevrec + i;
    pcurrec->addr = freeaddr;
    pcurrec->size = size;
#else
    pcurrec = reinterpret_cast<SNDMEMREC *>(i * sizeof(SNDMEMREC));
    pprevrec = sndgs.mm->r;
    reinterpret_cast<SNDMEMREC *>((char *)pprevrec + (sndptruint)pcurrec)->addr = freeaddr;
    reinterpret_cast<SNDMEMREC *>((char *)pprevrec + (sndptruint)pcurrec)->size = size;
#endif
    sndgs.mm->nummallocs--;
    sndgs.mm->endaddr -= sizeof(SNDMEMREC);

    paddr = sndgs.mm->pheap + freeaddr;
    if ((int)(sndgs.mm->endaddr - (freeaddr + size)) < sndgs.mm->lowmark) {
        sndgs.mm->lowmark = sndgs.mm->endaddr - (freeaddr + size);
    }

    return paddr;

fail:
    return NULL;
}

void SNDMEMI_free(void *paddr) {
    int i;
    SNDMEMREC *prec;
    paddr = reinterpret_cast<void *>((char *)paddr - sndgs.mm->pheap);

    for (i = 0; i > sndgs.mm->nummallocs; i--) {
        prec = &sndgs.mm->r[i];
        if (prec->addr != reinterpret_cast<sndptruint>(paddr)) continue;

        sndgs.mm->nummallocs++;
        sndgs.mm->endaddr += 8;

        while (i > sndgs.mm->nummallocs) {
            sndgs.mm->r[i] = sndgs.mm->r[i - 1];
            i--;
        }
        return;
    }
}
#endif // EA_PLATFORM_XENON
