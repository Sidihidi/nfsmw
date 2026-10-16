#include <string.h>
#include <math.h>

#include "pbdll.h"
#include "vputil.h"

UINT32 Var16Point(UINT8 *DataPtr, INT32 SourceStride) {
    UINT32 i;
    UINT32 XSum = 0;
    UINT32 XXSum = 0;
    UINT8 *DiffPtr;

    DiffPtr = DataPtr;

    for (i = 0; i < 8; i += 2) {
        XSum += DiffPtr[0];
        XXSum += DiffPtr[0] * DiffPtr[0];
        XSum += DiffPtr[2];
        XXSum += DiffPtr[2] * DiffPtr[2];
        XSum += DiffPtr[4];
        XXSum += DiffPtr[4] * DiffPtr[4];
        XSum += DiffPtr[6];
        XXSum += DiffPtr[6] * DiffPtr[6];

        DiffPtr += (SourceStride << 1);
    }

    return ((XXSum << 4) - XSum * XSum) >> 8;
}

void VP6_PredictFiltered(PB_INSTANCE *pbi, UINT8 *SrcPtr, INT32 mx, INT32 my) {
    INT32 mVx;
    INT32 mVy;
    INT32 ReconIndex;
    MACROBLOCK_INFO *mbi;
    UINT8 *TempBuffer;
    INT32 BoundaryX;
    INT32 BoundaryY;

    mbi = &pbi->mbi;
    TempBuffer = pbi->LoopFilteredBlock;

    if (mx > 0)
        mVx = mx >> mbi->MvShift;
    else
        mVx = -(-mx >> mbi->MvShift);

    if (my > 0)
        mVy = my >> mbi->MvShift;
    else
        mVy = -(-my >> mbi->MvShift);

    ReconIndex = mbi->FrameReconStride * mVy + mVx - 2 * mbi->CurrentReconStride - 2;

    Copy12x12(SrcPtr + ReconIndex, TempBuffer, mbi->CurrentReconStride, 16);

    BoundaryX = (8 - (mVx & 7)) & 7;
    BoundaryY = (8 - (mVy & 7)) & 7;

    if (BoundaryX)
        FilteringHoriz_12(pbi->quantizer->FrameQIndex, TempBuffer + (BoundaryX + 2), 16);

    if (BoundaryY)
        FilteringVert_12(pbi->quantizer->FrameQIndex, TempBuffer + (BoundaryY * 16 + 32), 16);
}
