#include "postp.h"

/* Sin prototipo, como en el original: en GameCube las llamadas llevan crclr. */
extern void *memcpy();

static const UINT32 DeblockLimitValuesV1[64] = {
    30, 25, 20, 20, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10,
    9,  9,  8,  8,  7,  7,  7,  7,  6,  6,  6,  6,  5,  5,  5,  5,
    4,  4,  4,  4,  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  2,  2,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

void FilterHoriz_Simple_C(POSTPROC_INSTANCE *ppi, unsigned char *PixelPtr, INT32 LineLength,
                          INT32 *BoundingValuePtr) {
    INT32 j;
    INT32 FiltVal;
    unsigned char *LimitTable = &LimitVal_VP31[256];

    for (j = 0; j < 8; j++) {
        int UseHighVariance = 0;

        FiltVal = 3 * (PixelPtr[2] - PixelPtr[1]);

        if ((abs(PixelPtr[0] - PixelPtr[1]) > 1) || (abs(PixelPtr[2] - PixelPtr[3]) > 1))
            UseHighVariance = 1;

        if (UseHighVariance)
            FiltVal += (PixelPtr[0] - PixelPtr[3]);

        FiltVal = BoundingValuePtr[(FiltVal + 4) >> 3];

        PixelPtr[1] = LimitTable[PixelPtr[1] + FiltVal];
        PixelPtr[2] = LimitTable[PixelPtr[2] - FiltVal];

        if (!UseHighVariance) {
            FiltVal >>= 1;
            PixelPtr[0] = LimitTable[PixelPtr[0] + FiltVal];
            PixelPtr[3] = LimitTable[PixelPtr[3] - FiltVal];
        }

        PixelPtr += LineLength;
    }
}

void FilterVert_Simple_C(POSTPROC_INSTANCE *ppi, unsigned char *PixelPtr, INT32 LineLength,
                         INT32 *BoundingValuePtr) {
    INT32 j;
    INT32 FiltVal;
    unsigned char *LimitTable = &LimitVal_VP31[256];

    for (j = 0; j < 8; j++) {
        int UseHighVariance = 0;

        FiltVal = 3 * (PixelPtr[0] - PixelPtr[-LineLength]);

        if ((abs(*(PixelPtr - 2 * LineLength) - PixelPtr[-LineLength]) > 1) ||
            (abs(PixelPtr[0] - PixelPtr[LineLength]) > 1))
            UseHighVariance = 1;

        if (UseHighVariance)
            FiltVal += (*(PixelPtr - 2 * LineLength) - PixelPtr[LineLength]);

        FiltVal = BoundingValuePtr[(FiltVal + 4) >> 3];

        PixelPtr[-LineLength] = LimitTable[PixelPtr[-LineLength] + FiltVal];
        PixelPtr[0] = LimitTable[PixelPtr[0] - FiltVal];

        if (!UseHighVariance) {
            FiltVal >>= 1;
            PixelPtr[-2 * LineLength] = LimitTable[PixelPtr[-2 * LineLength] + FiltVal];
            PixelPtr[LineLength] = LimitTable[PixelPtr[LineLength] - FiltVal];
        }

        PixelPtr++;
    }
}

void SimpleDeblockFrame(POSTPROC_INSTANCE *ppi, unsigned char *SrcBuffer,
                        unsigned char *DestBuffer) {
    INT32 j, m, n;
    INT32 RowStart = 0;
    INT32 NextRow;
    INT32 FLimit;
    INT32 QIndex;
    INT32 *BoundingValuePtr;
    INT32 LineLength = 0;
    INT32 FragsAcross = ppi->HFragments;
    INT32 FragsDown = ppi->VFragments;

    QIndex = ppi->FrameQIndex;

    /* PC (speed.exe 0x80647B): el original compara contra 2, no contra 1. */
#if defined(EA_PLATFORM_WIN32)
    if (ppi->Vp3VersionNo >= 2)
#else
    if (ppi->Vp3VersionNo > 1)
#endif
        FLimit = DeblockLimitValuesV2[QIndex];
    else
        FLimit = DeblockLimitValuesV1[QIndex];

    BoundingValuePtr = SetupDeblockValueArray(ppi, FLimit);

    for (j = 0; j < 3; j++) {
        switch (j) {
        case 0:
            FragsAcross = ppi->HFragments;
            FragsDown = ppi->VFragments;
            LineLength = ppi->YStride;
            RowStart = ppi->ReconYDataOffset;
            break;
        case 1:
            FragsAcross = ppi->HFragments / 2;
            FragsDown = ppi->VFragments / 2;
            LineLength = ppi->UVStride;
            RowStart = ppi->ReconUDataOffset;
            break;
        case 2:
            FragsAcross = ppi->HFragments / 2;
            FragsDown = ppi->VFragments / 2;
            LineLength = ppi->UVStride;
            RowStart = ppi->ReconVDataOffset;
            break;
        }

        NextRow = LineLength * 8;

        memcpy(DestBuffer + RowStart, SrcBuffer + RowStart, NextRow);

        for (n = 1; n < FragsAcross; n++)
            FilterHoriz_Simple(ppi, DestBuffer + (8 * n + RowStart) - 2, LineLength,
                               BoundingValuePtr);

        RowStart += NextRow;

        for (m = 1; m < FragsDown; m++) {
            memcpy(DestBuffer + RowStart, SrcBuffer + RowStart, LineLength * 8);

            FilterVert_Simple(ppi, DestBuffer + RowStart, LineLength, BoundingValuePtr);

            for (n = 1; n < FragsAcross; n++) {
                FilterHoriz_Simple(ppi, DestBuffer + (8 * n + RowStart) - 2, LineLength, BoundingValuePtr);
                FilterVert_Simple(ppi, DestBuffer + RowStart + 8 * n, LineLength, BoundingValuePtr);
            }

            RowStart += NextRow;
        }
    }
}

