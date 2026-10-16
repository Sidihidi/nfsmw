#include "./spchi.h"

unsigned char *iSPCH_GetSampleParmAddr(VOXBANKHDR *hdr, int sampleIndex) {
    unsigned char *sampleParmData;
    int numParms;

    sampleParmData = 0;
    if (sampleIndex < hdr->numSamples) {
        numParms = hdr->parmFlags & 0x7F;
        sampleParmData = (unsigned char *)hdr + sampleIndex * (numParms + 2);
        sampleParmData += 0xE;
    }
    return sampleParmData;
}

/* `endOffset *= blockSize` en las DOS ramas (mapa de la A124: 84, 88-89 y la 90 sin codigo, la
   llave de cierre): jump2 funde las dos multiplicaciones en una (salto cruzado) DESPUES de
   sched1/sched2, asi que el bloque de los stores se planifica sin la latencia del mullw/mult. */
int iSPCH_GetSampleSizeData(VOXBANKHDR *hdr, int sampleIndex, unsigned int *sampleOffset, unsigned int *dataBytes) {
    unsigned int offset;
    int sampleSize;
    unsigned char *sampleData;
    int result;
    int numParms;
    int blockSize;
    int nextIndex;
    unsigned int endOffset;
    unsigned char *nextSampleData;

    result = 0;
    if (sampleIndex >= hdr->numSamples) {
        goto abort;
    }

    numParms = hdr->parmFlags & 0x7F;
    sampleSize = numParms + 2;
    sampleData = (unsigned char *)hdr + 0xC;
    sampleData += sampleIndex * sampleSize;
    blockSize = (hdr->blockSize + 1) << 8;
    offset = (sampleData[0] << 8) + sampleData[1];
    offset *= blockSize;

    nextSampleData = sampleData + sampleSize;
    nextIndex = sampleIndex + 1;
    if (nextIndex >= hdr->numSamples) {
        endOffset = hdr->bankBlocks * blockSize;
    } else {
        endOffset = (nextSampleData[0] << 8) + nextSampleData[1];
        endOffset *= blockSize;
    }

    *sampleOffset = offset;
    *dataBytes = endOffset - offset;
    result = 1;

abort:
    return result;
}
