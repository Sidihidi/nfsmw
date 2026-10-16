#include "Speed/Indep/Libs/snd/9/source/library/cmn/sbanki.h"
#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"
#include "snd/sndo.h"
#include <cstddef>

SNDBANKASYNCMEMSTATE sndbams;

void SNDBANKI_asyncloadmemresolve() {
    BANKLIST *pbl = &sndgs.banklist[sndbams.bhandle];
    BANKVER5 *pb = pbl->phdr;

    int firstoffset;
    int i;
#ifdef EA_PLATFORM_PLAYSTATION2
    if (pb->iopcpusize != 0) {
        firstoffset = 0;

        for (i = 0; i < pb->numpatches; i++) {
            if (pb->patch[i] != NULL) {
                SNDBANKI_asyncresolvepatch(0x100, reinterpret_cast<TAGGEDPATCH *>((sndptrint)&pb->patch[i] + (sndptrint)pb->patch[i]), (char *)pbl->piopram,
                                           &firstoffset);
            }
        }
    }
#endif
    if (pb->spusize != 0) {
        firstoffset = 0;

        for (i = 0; i < pb->numpatches; i++) {
            if (pb->patch[i] != NULL) {
#ifdef EA_PLATFORM_PLAYSTATION2
                SNDBANKI_asyncresolvepatch(8, reinterpret_cast<TAGGEDPATCH *>((sndptrint)&pb->patch[i] + (sndptrint)pb->patch[i]), (char *)pbl->pspuram,
                                           &firstoffset);
#else
                SNDBANKI_asyncresolvepatch(0x200, reinterpret_cast<TAGGEDPATCH *>((sndptrint)&pb->patch[i] + (sndptrint)pb->patch[i]), (char *)pbl->pspuram,
                                           &firstoffset);
#endif
            }
        }
    }

    pbl->locked = 0;
    sndbams.loading = 0;
}

void SNDBANKI_asyncloadmem100hz() {
#ifdef EA_PLATFORM_PLAYSTATION2
    if (!SNDPLATFORM_downloadcompleteNew(sndbams.cmnRequestHandle) && sndbams.cmnRequestHandle.handle > 0)
        return;
#else
    if (SNDPLATFORM_downloadcomplete(sndbams.dlhandle) == 0) {
        if (sndbams.dlhandle > 0) {
            return;
        }
    }
#endif

    BANKLIST *pbl = &sndgs.banklist[sndbams.bhandle];
    BANKVER5 *pb = pbl->phdr;
    int xfersize;
#ifdef EA_PLATFORM_PLAYSTATION2
    if (pbl->piopram != NULL) {
        xfersize = pb->iopcpusize - sndbams.iopcpudownloaded;
        if (xfersize != 0) {
            if (xfersize > 0x18000) {
                xfersize = 0x18000;
            }

            sndbams.cmnRequestHandle = SNDPLATFORM_downloadNew(0x100, (char *)pb + pb->hdrsize + sndbams.iopcpudownloaded,
                                                               (char *)pbl->piopram + sndbams.iopcpudownloaded, xfersize, LOW, ASYNC);
            sndbams.iopcpudownloaded += xfersize;

            return;
        }
    }
#endif
    if (pbl->pspuram != NULL) {
        xfersize = pb->spusize - sndbams.spudownloaded;
        if (xfersize != 0) {
            if (xfersize > 0x10000) {
                xfersize = 0x10000;
            }

#ifdef EA_PLATFORM_PLAYSTATION2
            sndbams.cmnRequestHandle = SNDPLATFORM_downloadNew(8, (char *)pb + pb->hdrsize + pb->iopcpusize + sndbams.spudownloaded,
                                                               (char *)pbl->pspuram + sndbams.spudownloaded, xfersize, LOW, ASYNC);
#else
            sndbams.dlhandle = SNDPLATFORM_download(0x200, (char *)pb + pb->hdrsize + pb->iopcpusize + sndbams.spudownloaded,
                                                    (char *)pbl->pspuram + sndbams.spudownloaded, xfersize);
#endif
            sndbams.spudownloaded += xfersize;

            return;
        }
    }

    SNDBANKI_asyncloadmemresolve();
    SNDSYS_remove100hzclient(SNDBANKI_asyncloadmem100hz);
}

int SNDBANK_asyncloadmem(int *pbhandle, void *pbank) {
    BANKLIST *pbl;
    BANKVER5 *pb;
    int rc = 0;

    sndgs.bankremove = SNDbankremove;

    SNDSYS_entercritical();

    *pbhandle = SNDBANKI_alloc();
    sndbams.bhandle = *pbhandle;

    pbl = &sndgs.banklist[*pbhandle];
    pbl->phdr = reinterpret_cast<BANKVER5 *>(pbank);

    pb = pbl->phdr;

#ifdef EA_PLATFORM_PLAYSTATION2
    pbl->piopram = NULL;
    if (pb->iopcpusize != 0) {
        pbl->piopram = (char *)SNDPLATFORM_memalloc(0x100, pb->iopcpusize + 0x30);
        if (pbl->piopram == NULL) {
            pbl->phdr = NULL;
            rc = -6;
            goto abort;
        }
    }
#endif
    pbl->pspuram = NULL;
    if (pb->spusize != 0) {
#ifdef EA_PLATFORM_PLAYSTATION2
        pbl->pspuram = (char *)SNDPLATFORM_memalloc(8, pb->spusize + 0x30);
#else
        pbl->pspuram = (char *)SNDPLATFORM_memalloc(0x200, pb->spusize + 0x30);
#endif
        if (pbl->pspuram == NULL) {
            pbl->phdr = NULL;
            rc = -6;
            goto abort;
        }
    }
    if (pb->spusize != 0 || pb->iopcpusize != 0) {
        pbl->locked = 1;
        sndbams.loading = 1;
#ifdef EA_PLATFORM_PLAYSTATION2
        sndbams.iopcpudownloaded = 0;
#endif
        sndbams.spudownloaded = 0;
        rc = 7;
        SNDSYS_add100hzclient(SNDBANKI_asyncloadmem100hz);
        goto abort;
    }

    SNDBANKI_asyncloadmemresolve();
    rc = 8;

abort:
    SNDSYS_leavecritical();
    return rc;
}

int SNDBANK_asyncloadmemdone() {
    return sndbams.loading == 0;
}
