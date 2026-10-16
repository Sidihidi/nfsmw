#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file/filesys.h"
#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"
#include "sbanki.h"
#include "snd/sndo.h"
#include <cstddef>

SNDBANKASYNCSTATE sndbas = {
    NULL,
    100,
    0,
};

void SNDBANKI_abortload() {
    SNDBANKI_asynccompletereads();
    SNDBANKI_asynccompletedownload();

    if (sndbas.pi->readscompleted < sndbas.pi->readsissued || sndbas.pi->processescompleted < sndbas.pi->processesissued) {
        return;
    }

    if (sndbas.pi->fileopen) {
        if (sndbas.pi->closeop == 0) {
            sndbas.pi->closeop = FILESYS_close(sndbas.pi->fhandle, sndbas.readpriority, NULL);
        }
        if (sndbas.pi->closeop != 0) {
            if (FILESYS_opstatus(sndbas.pi->closeop) != 1)
                return;
            FILESYS_completeop(sndbas.pi->closeop);
        }
    }

    if (sndbas.pi->synctaskadded) {
        iSNDserverremoveclient(SNDBANKI_asyncservice);
    }

#ifdef EA_PLATFORM_PLAYSTATION2
    if (sndbas.pi->iopcpuxferremaining != 0) {
        SNDPLATFORM_memfree(0x100, reinterpret_cast<sndptruint>(sndbas.pi->piopcpuram));
    }
#endif

    sndgs.banklist[sndbas.pi->bhandle].phdr = NULL;
    sndgs.banklist[sndbas.pi->bhandle].locked = 0;
    sndbas.pi = NULL;
}

void SNDBANKI_asyncresolve() {
    BANKVER5 *pb = sndbas.pi->pbank;
    BANKLIST *pbl = &sndgs.banklist[sndbas.pi->bhandle];
    int firstoffset = 0;
    int i = 0;

    pbl->phdr = pb;
#ifdef EA_PLATFORM_PLAYSTATION2
    pbl->piopram = sndbas.pi->piopcpuram;

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
    pbl->pspuram = sndbas.pi->pspuram;

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
}

void SNDBANKI_asynccompletereads() {
    SNDASYNCLOADBUFFER *psalb;
    int curbuf = sndbas.pi->readscompleted % 4;
    int bytesread;

    while (sndbas.pi->readsissued > sndbas.pi->readscompleted) {
        psalb = &sndbas.pi->salb[curbuf];
        if (psalb->state != 1 || FILESYS_opstatus(psalb->fileop) != 1)
            break;

        bytesread = FILESYS_completeop(psalb->fileop);
        if (bytesread > 0) {
            psalb->state = 2;
        } else {
            psalb->state = 0;
        }

        sndbas.pi->readscompleted++;
        if (++curbuf > 3) {
            curbuf = 0;
        }
    }
}

void SNDBANKI_asyncissuereads() {
    SNDASYNCLOADBUFFER *psalb;
    int curbuf = sndbas.pi->readsissued % 4;
    int maxsize = 0xFFFFFF;
    int readoffset;

    while (sndbas.pi->readscompleted > sndbas.pi->readsissued - 4) {
        readoffset = sndbas.pi->readsissued * sndbas.pi->loadbufsize;
        if (sndbas.pi->pbank != NULL) {
            maxsize = sndbas.pi->totalbanksize - readoffset;
            if (maxsize <= 0)
                break;
        }
        if (maxsize > sndbas.pi->loadbufsize) {
            maxsize = sndbas.pi->loadbufsize;
        }

        psalb = &sndbas.pi->salb[curbuf];
        if (psalb->state != 0)
            break;

        psalb->fileop = FILESYS_read(sndbas.pi->fhandle, sndbas.pi->fileoffset + readoffset, psalb->ploadbuf, maxsize, sndbas.readpriority, NULL);
        psalb->state = 1;
        sndbas.pi->readsissued++;
        if (++curbuf > 3) {
            curbuf = 0;
        }
    }
}

void SNDBANKI_asyncxferhdr() {
    if (sndbas.pi->hdrxferremaining <= 0)
        return;

    SNDASYNCLOADBUFFER *psalb;
    int curbuf = sndbas.pi->processesissued % 4;
    int maxsize;

    while (sndbas.pi->processescompleted < sndbas.pi->readscompleted) {
        psalb = &sndbas.pi->salb[curbuf];
        if (sndbas.pi->hdrxferremaining < sndbas.pi->loadbufsize) {
            maxsize = sndbas.pi->hdrxferremaining;
        } else {
            maxsize = sndbas.pi->loadbufsize;
        }

        Snd::Util::MemCpy((char *)sndbas.pi->pbank + sndbas.pi->loadbufsize * sndbas.pi->processescompleted, psalb->ploadbuf, maxsize);

        sndbas.pi->hdrxferremaining -= maxsize;
        if (sndbas.pi->hdrxferremaining <= 0 && maxsize < sndbas.pi->loadbufsize) {
            sndbas.pi->dloffset = maxsize;
            return;
        }

        psalb->state = 0;
        sndbas.pi->processesissued++;
        sndbas.pi->processescompleted++;
        if (++curbuf > 3) {
            curbuf = 0;
        }
    }
}

void SNDBANKI_asynccompletedownload() {
    SNDASYNCLOADBUFFER *psalb;
    int curbuf = sndbas.pi->processescompleted % 4;

    if (sndbas.pi->processesissued <= sndbas.pi->processescompleted)
        return;

    do {
        psalb = &sndbas.pi->salb[curbuf];
#ifdef EA_PLATFORM_PLAYSTATION2
        if ((SNDPLATFORM_downloadcompleteNew(psalb->cmnRequestHandle) && psalb->cmnRequestHandle.handle > 0) == 0)
#else
        if (SNDPLATFORM_downloadcomplete(psalb->dlhandle) == 0 || psalb->dlhandle <= 0)
#endif
            break;

        psalb->state = 0;
        sndbas.pi->processescompleted++;
        if (++curbuf > 3) {
            curbuf = 0;
        }

    } while (sndbas.pi->processesissued > sndbas.pi->processescompleted);
}

void SNDBANKI_asyncissuedownloads() {
#ifdef EA_PLATFORM_PLAYSTATION2
    if (sndbas.pi->iopcpuxferremaining <= 0 && sndbas.pi->spuxferremaining <= 0)
        return;
#else
    if (sndbas.pi->spuxferremaining <= 0)
        return;
#endif

    SNDASYNCLOADBUFFER *psalb;
    int curbuf = sndbas.pi->processescompleted % 4;
    int xfersize;

    if (sndbas.pi->processesissued >= sndbas.pi->processescompleted + 4)
        return;

    do {
        psalb = &sndbas.pi->salb[curbuf];
        if (psalb->state != 2)
            break;
#ifdef EA_PLATFORM_PLAYSTATION2
        if (sndbas.pi->iopcpuxferremaining <= 0 && sndbas.pi->spuxferremaining <= 0) {
#else
        if (sndbas.pi->spuxferremaining <= 0) {
#endif
            psalb->state = 0;
            break;
        }

#ifdef EA_PLATFORM_PLAYSTATION2
        if (sndbas.pi->iopcpuxferremaining > 0) {
            xfersize = sndbas.pi->loadbufsize - sndbas.pi->dloffset;
            if (xfersize > sndbas.pi->iopcpuxferremaining) {
                xfersize = sndbas.pi->iopcpuxferremaining;
            }

            psalb->cmnRequestHandle = SNDPLATFORM_downloadNew(0x100, &psalb->ploadbuf[sndbas.pi->dloffset],
                                                              &sndbas.pi->piopcpuram[sndbas.pi->iopcpudownloaded], xfersize, LOW, ASYNC);
            psalb->state = 3;
            sndbas.pi->iopcpudownloaded += xfersize;
            sndbas.pi->iopcpuxferremaining -= xfersize;
            sndbas.pi->dloffset += xfersize;
        }

        if (sndbas.pi->spuxferremaining > 0 && sndbas.pi->iopcpuxferremaining <= 0 && sndbas.pi->dloffset < sndbas.pi->loadbufsize) {
#else
        if (sndbas.pi->dloffset < sndbas.pi->loadbufsize) {
#endif
            xfersize = sndbas.pi->loadbufsize - sndbas.pi->dloffset;
            if (xfersize > sndbas.pi->spuxferremaining) {
                xfersize = sndbas.pi->spuxferremaining;
            }

#ifdef EA_PLATFORM_PLAYSTATION2
            psalb->cmnRequestHandle = SNDPLATFORM_downloadNew(8, &psalb->ploadbuf[sndbas.pi->dloffset],
                                                              &sndbas.pi->pspuram[sndbas.pi->spudownloaded], xfersize, LOW, ASYNC);
#else
            psalb->dlhandle =
                SNDPLATFORM_download(0x200, &psalb->ploadbuf[sndbas.pi->dloffset], &sndbas.pi->pspuram[sndbas.pi->spudownloaded], xfersize);
#endif
            psalb->state = 3;
            sndbas.pi->spudownloaded += xfersize;
            sndbas.pi->spuxferremaining -= xfersize;
        }

        if (psalb->state == 3) {
            sndbas.pi->dloffset = 0;
            sndbas.pi->processesissued++;
        }

        if (++curbuf > 3) {
            curbuf = 0;
        }
    } while (sndbas.pi->processesissued < sndbas.pi->processescompleted + 4);
}

void SNDBANKI_asyncprocess() {
    SNDBANKI_asynccompletereads();
    SNDBANKI_asyncxferhdr();
    SNDBANKI_asynccompletedownload();
    SNDBANKI_asyncissuedownloads();
    SNDBANKI_asyncissuereads();
}

void SNDBANKI_asyncserver() {
    SNDBANKI_asyncprocess();

    if (sndbas.pi->hdrxferremaining > 0)
        return;
#ifdef EA_PLATFORM_PLAYSTATION2
    if (sndbas.pi->iopcpuxferremaining > 0)
        return;
#endif
    if (sndbas.pi->spuxferremaining > 0)
        return;

    if (sndbas.pi->closeop == 0) {
        sndbas.pi->closeop = FILESYS_close(sndbas.pi->fhandle, sndbas.readpriority, NULL);
    }

    if (sndbas.pi->processesissued != sndbas.pi->processescompleted)
        return;

    if (FILESYS_opstatus(sndbas.pi->closeop) == 1) {
        FILESYS_completeop(sndbas.pi->closeop);
        SNDSYS_remove100hzclient(SNDBANKI_asyncserver);
        SNDBANKI_asyncresolve();
        sndbas.pi = NULL;
    }
}

void SNDBANKI_asyncservice() {
    SNDASYNCLOADBUFFER *psalb;
    int opstatus;

    if (sndbas.pi->aborting) {
        SNDBANKI_abortload();
        return;
    }
    if (sndbas.pi->openop != 0) {
        opstatus = FILESYS_opstatus(sndbas.pi->openop);
        if (opstatus != 1) {
            if (opstatus == 0)
                return;

            FILESYS_completeop(sndbas.pi->openop);
            sndbas.lasterror = -19;
            sndbas.pi->aborting = true;
            SNDBANKI_abortload();
            return;
        }

        sndbas.pi->fhandle = FILESYS_completeop(sndbas.pi->openop);
        sndbas.pi->openop = 0;
        sndbas.pi->fileopen = true;

        SNDBANKI_asyncissuereads();
    }

    psalb = &sndbas.pi->salb[0];
    if (FILESYS_opstatus(psalb->fileop) != 1)
        return;

    FILESYS_completeop(psalb->fileop);
    psalb->state = 2;

    sndbas.pi->readscompleted++;
    sndbas.pi->pbank = reinterpret_cast<BANKVER5 *>(psalb->ploadbuf);
    sndbas.pi->hdrxferremaining = sndbas.pi->pbank->hdrsize;
    sndbas.pi->totalbanksize = sndbas.pi->pbank->hdrsize;
#ifdef EA_PLATFORM_PLAYSTATION2
    sndbas.pi->iopcpuxferremaining = sndbas.pi->pbank->iopcpusize;
    sndbas.pi->totalbanksize = sndbas.pi->totalbanksize + sndbas.pi->pbank->iopcpusize;
#endif
    sndbas.pi->spuxferremaining = sndbas.pi->pbank->spusize;
    sndbas.pi->totalbanksize = sndbas.pi->totalbanksize + sndbas.pi->pbank->spusize;

    sndbas.pi->pbank = reinterpret_cast<BANKVER5 *>(sndbas.pi->mallocfn(sndbas.pi->hdrxferremaining));
    if (sndbas.pi->pbank == NULL) {
        sndbas.pi->aborting = true;
        sndbas.lasterror = -6;
        return;
    }

#ifdef EA_PLATFORM_PLAYSTATION2
    if (sndbas.pi->iopcpuxferremaining != 0) {
        sndbas.pi->piopcpuram = reinterpret_cast<char *>(SNDPLATFORM_memalloc(0x100, sndbas.pi->iopcpuxferremaining + 0x40));
        if (sndbas.pi->piopcpuram == NULL) {
            sndbas.pi->aborting = true;
            sndbas.lasterror = -6;
            return;
        }
    } else {
        sndbas.pi->piopcpuram = NULL;
    }
#endif

    if (sndbas.pi->spuxferremaining != 0) {
#ifdef EA_PLATFORM_PLAYSTATION2
        sndbas.pi->pspuram = reinterpret_cast<char *>(SNDPLATFORM_memalloc(8, sndbas.pi->spuxferremaining + 0x40));
#else
        sndbas.pi->pspuram = reinterpret_cast<char *>(SNDPLATFORM_memalloc(0x200, sndbas.pi->spuxferremaining + 0x40));
#endif
        if (sndbas.pi->pspuram == NULL) {
            sndbas.pi->aborting = true;
            sndbas.lasterror = -6;
            return;
        }
    } else {
        sndbas.pi->pspuram = NULL;
    }

    iSNDserverremoveclient(SNDBANKI_asyncservice);
    sndbas.pi->synctaskadded = false;
    SNDBANKI_asyncprocess();
    SNDSYS_add100hzclient(SNDBANKI_asyncserver);
}

int SNDBANK_asyncloadi(char *filename, int fileoffset, void *pmem, int memsize, void *(*mallocfn)(int), bool failgracefully) {
    SNDASYNCLOADBUFFER *psalb;
    BANKLIST *pbanklist;
    int i;

    sndbas.pi = reinterpret_cast<SNDBANKASYNCINSTANCE *>(pmem);
    sndbas.lasterror = 0;

    sndbas.pi->aborting = false;
    sndbas.pi->fileopen = false;
    sndbas.pi->failgracefully = failgracefully;
    sndbas.pi->mallocfn = mallocfn;
    sndbas.pi->fileoffset = fileoffset;
    sndbas.pi->dloffset = 0;
    sndbas.pi->totalbanksize = 0;
    sndbas.pi->hdrxferremaining = 0;
#ifdef EA_PLATFORM_PLAYSTATION2
    sndbas.pi->iopcpudownloaded = 0;
#endif
    sndbas.pi->spudownloaded = 0;
    sndbas.pi->pbank = NULL;
    sndbas.pi->readsissued = 0;
    sndbas.pi->readscompleted = 0;
    sndbas.pi->processesissued = 0;
    sndbas.pi->processescompleted = 0;
    sndbas.pi->closeop = 0;
    sndbas.pi->loadbufsize = (unsigned int)(memsize - sizeof(SNDBANKASYNCINSTANCE) - 0x140) >> 2;

    if (sndbas.pi->loadbufsize > 0x6000) {
        sndbas.pi->loadbufsize = 0x6000;
    }
    sndbas.pi->loadbufsize &= ~0x3F;
    sndbas.pi->openop = FILESYS_open(filename, 1, sndbas.readpriority, NULL);

    for (i = 0; i < 4; i++) {
        psalb = &sndbas.pi->salb[i];
        psalb->dlhandle = -1;
#ifdef EA_PLATFORM_PLAYSTATION2
        psalb->cmnRequestHandle.handle = -1;
#endif
        psalb->state = 0;
        psalb->ploadbuf = (char *)(((sndbas.pi->loadbufsize + 0x40) * i + (sndptrint)pmem + sizeof(SNDBANKASYNCINSTANCE) + 0x80) & ~0x3F);
    }

    for (i = 0; i < sndgs.sso.set.maxbanks; i++) {
        pbanklist = &sndgs.banklist[i];
        if (pbanklist->phdr == NULL) {
            pbanklist->locked = 1;
            pbanklist->phdr = (BANKVER5 *)1;
            sndbas.pi->bhandle = i;
            break;
        }
    }

    iSNDserveraddclient(SNDBANKI_asyncservice);
    sndbas.pi->synctaskadded = true;
    SNDBANKI_asyncservice();

    if (sndbas.pi == NULL) {
        return sndbas.lasterror;
    } else {
        return sndbas.pi->bhandle;
    }
}

int SNDBANK_asyncload(char *filename, int fileoffset, void *pmem, int memsize, void *(*mallocfn)(int)) {
    return SNDBANK_asyncloadi(filename, fileoffset, pmem, memsize, mallocfn, false);
}

int SNDBANK_asyncdone() {
    if (sndbas.pi == NULL) {
        if (sndbas.lasterror != 0) {
            return sndbas.lasterror;
        }
        return 1;
    }
    return 0;
}
