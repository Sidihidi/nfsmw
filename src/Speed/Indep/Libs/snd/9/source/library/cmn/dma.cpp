// La cola de transferencias DMA del EE al IOP de snd 9.06.00-ps2 (C:/packages/snd/9.06.00-ps2/source/library/cmn/
// dma.cpp): las peticiones se trocean en bloques de DM_MAX_LENGTH, se encolan por prioridad y un hilo las pasa a la
// SIF de una en una; al IOP se le avisa con un comando para que copie al SPU las que van alli.

#include "../ps2/sndps2.h"

namespace Snd {

CListQueue Dma::sListQueued[Dma::NUM_LISTS];
CListQueue Dma::sListFree;
DmaRequest Dma::sRequestBuff[Dma::MAX_DMA_REQUESTS];
int Dma::sDmaActive;
volatile unsigned int Dma::sTransferedAt;
int Dma::sHandleSeqNum;
unsigned int Dma::sLastQueueID;
int Dma::sDmaThreadSema;
int Dma::sDmaThreadID;
char Dma::sDmaThreadStack[Dma::DMA_THREAD_STACK_SIZE];
sceSifDmaData Dma::sSpuDmaDataTemp;

Result Dma::Init() {
    static SemaParam semaParam;
    static ThreadParam threadParam;
    int i;

    sSpuDmaDataTemp.addr = (unsigned int)sceSifAllocIopHeap(DM_MAX_LENGTH);
    sSpuDmaDataTemp.mode = 0;

    for (i = 0; i < MAX_DMA_REQUESTS; i++) {
        sListFree.PushTail(&sRequestBuff[i].linkNode);
    }

    sDmaActive = 0;
    sLastQueueID = 0;
    sHandleSeqNum = 1;

    semaParam.maxCount = 253;
    semaParam.initCount = 0;
    sDmaThreadSema = CreateSema(&semaParam);
    if (sDmaThreadSema >= 0) {
        threadParam.entry = DmaThread;
        threadParam.stack = sDmaThreadStack;
        threadParam.stackSize = DMA_THREAD_STACK_SIZE;
        threadParam.initPriority = 5;
        threadParam.gpReg = &_gp;
        threadParam.option = 0;
        sDmaThreadID = CreateThread(&threadParam);
        if (sDmaThreadID >= 0) {
            StartThread(sDmaThreadID, 0);
            return RESULT_OK;
        }
    }
    return (Result)-6;
}

Result Dma::Restore() {
    ThreadParam threadParam;
    int i;

    for (i = 0; i < NUM_LISTS; i++) {
        sListQueued[i].Reset();
    }
    sListFree.Reset();

    ReferThreadStatus(sDmaThreadID, &threadParam);
    if (threadParam.status == THS_DORMANT) {
        DeleteThread(sDmaThreadID);
    } else {
        TerminateThread(sDmaThreadID);
        DeleteThread(sDmaThreadID);
    }

    DeleteSema(sDmaThreadSema);
    sceSifFreeIopHeap((void *)sSpuDmaDataTemp.addr);
    return RESULT_OK;
}

DmaRequestHandle Dma::QueueRequest(int playloc, unsigned int pSrc, unsigned int pDst, int length,
                                   DmaPriority priority, DmaTransferType transferType) {
    DmaRequestHandle requestHandle;
    unsigned int handle;
    int numTransfers;
    int curlength;
    int i;

    numTransfers = length / DM_MAX_LENGTH + 1;
    if (length % DM_MAX_LENGTH == 0) {
        numTransfers--;
    }

    for (i = 0; i < numTransfers; i++) {
        if (transferType == SYNC) {
            while (sListFree.GetEntries() < MIN_FREELIST_ENTRIES) {
                if (!sDmaActive) {
                    SemaParam sp;

                    ReferSemaStatus(sDmaThreadSema, &sp);
                    if (sp.currentCount < sp.maxCount) {
                        SignalSema(sDmaThreadSema);
                    }
                }
            }
        }

        curlength = DM_MAX_LENGTH;
        if (length < DM_MAX_LENGTH) {
            curlength = length;
        }

        if (curlength & 0x3F) {
            curlength += 64;
            curlength &= 0xFFFFFFC0;
        }

        DI();

        handle = (priority << 24) | sHandleSeqNum;
        sHandleSeqNum++;
        if (sHandleSeqNum > 0xFFFFFF) {
            sHandleSeqNum = 1;
        }

        DmaRequest *pRequest = (DmaRequest *)sListFree.Pop();

        pRequest->playloc = playloc;
        pRequest->pSrc = pSrc;
        pRequest->pDst = pDst;
        pRequest->length = curlength;
        pRequest->priority = priority;
        pRequest->transferType = transferType;
        pRequest->handle = handle;

        sListQueued[priority].PushTail(&pRequest->linkNode);

        EI();

        requestHandle.pRequest = pRequest;
        requestHandle.handle = handle;

        pSrc += curlength;
        pDst += curlength;
        length -= curlength;
    }

    SemaParam sp;

    ReferSemaStatus(sDmaThreadSema, &sp);
    if (sp.currentCount < sp.maxCount) {
        SignalSema(sDmaThreadSema);
    }

    return requestHandle;
}

int Dma::IsComplete(DmaRequestHandle requestHandle) {
    int isComplete;

    if (requestHandle.pRequest == 0) {
        return 1;
    }

    if (requestHandle.handle == requestHandle.pRequest->handle) {
        isComplete = 0;
        if (sndgs.audiotick - sTransferedAt > 500 || sndgs.audiotick - sTransferedAt < 0) {
            SemaParam sp;

            ReferSemaStatus(sDmaThreadSema, &sp);
            if (sp.currentCount < sp.maxCount) {
                SignalSema(sDmaThreadSema);
            }
        }
    } else {
        isComplete = 1;
    }
    return isComplete;
}

void Dma::CompletionHandler(unsigned int handle) {
    DmaRequest *pRequest;
    int priority;
    SemaParam sp;

    priority = handle >> 24;

    pRequest = (DmaRequest *)sListQueued[priority].GetHead();
    pRequest->handle = 0;
    sListQueued[priority].Pop();
    sListFree.PushTail(&pRequest->linkNode);

    sDmaActive = 0;

    iReferSemaStatus(sDmaThreadSema, &sp);
    if (sp.currentCount < sp.maxCount) {
        iSignalSema(sDmaThreadSema);
    }
}

void Dma::DmaThread(void *data) {
    DmaRequest *pRequest;
    static sceSifDmaData dmaData;
    static DMAMSG commandData;
    int i;

    while (1) {
        do {
            WaitSema(sDmaThreadSema);
        } while (sDmaActive);

        for (i = NUM_LISTS - 1; i >= 0; i--) {
            if (!sListQueued[i].IsEmpty()) {
                pRequest = (DmaRequest *)sListQueued[i].GetHead();
                dmaData.data = pRequest->pSrc;
                dmaData.size = pRequest->length;
                dmaData.mode = 0;
                dmaData.addr = pRequest->pDst;
                if (pRequest->playloc == SND_PLAYLOC_SPU) {
                    dmaData.addr = sSpuDmaDataTemp.addr;
                }

                FlushCache(0);
                sTransferedAt = sndgs.audiotick;
                sLastQueueID = sceSifSetDma(&dmaData, 1);

                sDmaActive = 1;

                if (pRequest->playloc == SND_PLAYLOC_IOPCPU) {
                    commandData.Hdr.psize = sizeof(DMAMSG);
                    commandData.cmd = 4;
                    commandData.handle = pRequest->handle;
                } else if (pRequest->playloc == SND_PLAYLOC_SPU) {
                    commandData.Hdr.psize = sizeof(DMAMSG);
                    commandData.cmd = 1;
                    commandData.handle = pRequest->handle;
                    commandData.psrc = sSpuDmaDataTemp.addr;
                    commandData.pdst = pRequest->pDst;
                    commandData.length = pRequest->length;
                    commandData.priority = pRequest->priority;
                }

                sceSifSendCmd(0, &commandData, sizeof(DMAMSG), 0, 0, 0);
                break;
            }
        }
    }
}

} // namespace Snd
