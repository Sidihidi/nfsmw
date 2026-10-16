#include "postp.h"

/* Sin prototipo, como en el original: en GameCube cada llamada a memcpy lleva
   crclr delante. */
extern void *memcpy();
extern void *memset();

void UpdateUMVBorder(POSTPROC_INSTANCE *pbi, UINT8 *DestReconPtr) {
    INT32 i;
    INT32 PlaneHeight;
    UINT8 *SrcPtr1, *SrcPtr2;
    UINT8 *DestPtr1, *DestPtr2;
    UINT32 Border = pbi->MVBorder;
    INT32 PlaneStride = pbi->YStride;

    PlaneStride = pbi->YStride;
    PlaneHeight = pbi->VFragments * 8;

    SrcPtr1 = DestReconPtr + pbi->ReconYDataOffset;
    SrcPtr2 = SrcPtr1 + 8 * pbi->HFragments - 1;
    DestPtr1 = SrcPtr1 - Border;
    DestPtr2 = SrcPtr2 + 1;

    for (i = 0; i < PlaneHeight; i++) {
        memset(DestPtr1, SrcPtr1[0], Border);
        memset(DestPtr2, SrcPtr2[0], Border);
        SrcPtr1 += PlaneStride;
        SrcPtr2 += PlaneStride;
        DestPtr1 += PlaneStride;
        DestPtr2 += PlaneStride;
    }

    SrcPtr1 = DestReconPtr + Border * PlaneStride;
    SrcPtr2 = SrcPtr1 + (pbi->VFragments * 8 * PlaneStride) - PlaneStride;
    DestPtr1 = DestReconPtr;
    DestPtr2 = SrcPtr2 + PlaneStride;

    for (i = 0; i < (INT32)Border; i++) {
        memcpy(DestPtr1, SrcPtr1, PlaneStride);
        memcpy(DestPtr2, SrcPtr2, PlaneStride);
        DestPtr1 += PlaneStride;
        DestPtr2 += PlaneStride;
    }

    PlaneStride = pbi->UVStride;
    PlaneHeight = pbi->VFragments * 4;

    SrcPtr1 = DestReconPtr + pbi->ReconUDataOffset;
    SrcPtr2 = SrcPtr1 + 4 * pbi->HFragments - 1;
    DestPtr1 = SrcPtr1 - Border / 2;
    DestPtr2 = SrcPtr2 + 1;

    for (i = 0; i < PlaneHeight; i++) {
        memset(DestPtr1, SrcPtr1[0], Border / 2);
        memset(DestPtr2, SrcPtr2[0], Border / 2);
        SrcPtr1 += PlaneStride;
        SrcPtr2 += PlaneStride;
        DestPtr1 += PlaneStride;
        DestPtr2 += PlaneStride;
    }

    SrcPtr1 = DestReconPtr + pbi->ReconUDataOffset - Border / 2;
    SrcPtr2 = SrcPtr1 + (pbi->VFragments * 4 * PlaneStride) - PlaneStride;
    DestPtr1 = SrcPtr1 - Border / 2 * PlaneStride;
    DestPtr2 = SrcPtr2 + PlaneStride;

    for (i = 0; i < (INT32)(Border / 2); i++) {
        memcpy(DestPtr1, SrcPtr1, PlaneStride);
        memcpy(DestPtr2, SrcPtr2, PlaneStride);
        DestPtr1 += PlaneStride;
        DestPtr2 += PlaneStride;
    }

    SrcPtr1 = DestReconPtr + pbi->ReconVDataOffset;
    SrcPtr2 = SrcPtr1 + 4 * pbi->HFragments - 1;
    DestPtr1 = SrcPtr1 - Border / 2;
    DestPtr2 = SrcPtr2 + 1;

    for (i = 0; i < PlaneHeight; i++) {
        memset(DestPtr1, SrcPtr1[0], Border / 2);
        memset(DestPtr2, SrcPtr2[0], Border / 2);
        SrcPtr1 += PlaneStride;
        SrcPtr2 += PlaneStride;
        DestPtr1 += PlaneStride;
        DestPtr2 += PlaneStride;
    }

    SrcPtr1 = DestReconPtr + pbi->ReconVDataOffset - Border / 2;
    SrcPtr2 = SrcPtr1 + (pbi->VFragments * 4 * PlaneStride) - PlaneStride;
    DestPtr1 = SrcPtr1 - Border / 2 * PlaneStride;
    DestPtr2 = SrcPtr2 + PlaneStride;

    for (i = 0; i < (INT32)(Border / 2); i++) {
        memcpy(DestPtr1, SrcPtr1, PlaneStride);
        memcpy(DestPtr2, SrcPtr2, PlaneStride);
        DestPtr1 += PlaneStride;
        DestPtr2 += PlaneStride;
    }
}

void CopyFrame(POSTPROC_INSTANCE *pbi, YUV_BUFFER_CONFIG *b, UINT8 *DestReconPtr) {
    int row;
    unsigned char *source;
    unsigned char *dest;

    dest = DestReconPtr + pbi->ReconYDataOffset;
    source = (unsigned char *)b->YBuffer;

    for (row = 0; row < b->YHeight; row++) {
        memcpy(dest, source, b->YWidth);
        source += b->YStride;
        dest += pbi->YStride;
    }

    dest = DestReconPtr + pbi->ReconUDataOffset;
    source = (unsigned char *)b->UBuffer;

    for (row = 0; row < b->UVHeight; row++) {
        memcpy(dest, source, b->UVWidth);
        source += b->UVStride;
        dest += pbi->UVStride;
    }

    dest = DestReconPtr + pbi->ReconVDataOffset;
    source = (unsigned char *)b->VBuffer;

    for (row = 0; row < b->UVHeight; row++) {
        memcpy(dest, source, b->UVWidth);
        source += b->UVStride;
        dest += pbi->UVStride;
    }

    UpdateUMVBorder(pbi, DestReconPtr);
}
