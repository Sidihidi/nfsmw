// Las descargas a la memoria del SPU y del IOP en el driver de PS2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/
// sdasync.c): van a la cola de DMA de la SIF (cmn/dma.cpp) y el asa comun lleva la de la cola.

#include "sndps2.h"

CmnRequestHandle SNDPLATFORM_downloadNew(int playloc, void *psrc, void *pdst, int size, DmaPriority priority,
                                         DmaTransferType transferType) {
    CmnRequestHandle cmnRequestHandle;
    Snd::DmaRequestHandle requestHandle;

    requestHandle = Snd::Dma::QueueRequest(playloc, (unsigned int)psrc, (unsigned int)pdst, size, priority,
                                           transferType);

    cmnRequestHandle.handle = requestHandle.handle;
    cmnRequestHandle.requestDetails = requestHandle.pRequest;
    return cmnRequestHandle;
}

int SNDPLATFORM_downloadcompleteNew(CmnRequestHandle cmnRequestHandle) {
    Snd::DmaRequestHandle requestHandle;

    requestHandle.handle = cmnRequestHandle.handle;
    requestHandle.pRequest = (Snd::DmaRequest *)cmnRequestHandle.requestDetails;

    return Snd::Dma::IsComplete(requestHandle);
}
