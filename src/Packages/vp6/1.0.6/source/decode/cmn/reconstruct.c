#include "codec_common.h"

void SatUnsigned8(UINT8 *ResultPtr, INT16 *DataBlock, UINT32 ResultLineStep, UINT32 DataLineStep) {
    INT32 i;

    for (i = 0; i < 8; i++) {
        ResultPtr[0] = (DataBlock[0] < 0) ? 0 : ((DataBlock[0] > 255) ? 255 : (UINT8)DataBlock[0]);
        ResultPtr[1] = (DataBlock[1] < 0) ? 0 : ((DataBlock[1] > 255) ? 255 : (UINT8)DataBlock[1]);
        ResultPtr[2] = (DataBlock[2] < 0) ? 0 : ((DataBlock[2] > 255) ? 255 : (UINT8)DataBlock[2]);
        ResultPtr[3] = (DataBlock[3] < 0) ? 0 : ((DataBlock[3] > 255) ? 255 : (UINT8)DataBlock[3]);
        ResultPtr[4] = (DataBlock[4] < 0) ? 0 : ((DataBlock[4] > 255) ? 255 : (UINT8)DataBlock[4]);
        ResultPtr[5] = (DataBlock[5] < 0) ? 0 : ((DataBlock[5] > 255) ? 255 : (UINT8)DataBlock[5]);
        ResultPtr[6] = (DataBlock[6] < 0) ? 0 : ((DataBlock[6] > 255) ? 255 : (UINT8)DataBlock[6]);
        ResultPtr[7] = (DataBlock[7] < 0) ? 0 : ((DataBlock[7] > 255) ? 255 : (UINT8)DataBlock[7]);

        ResultPtr += ResultLineStep;
        DataBlock += DataLineStep;
    }
}

void ScalarReconInterHalfPixel2(INT16 *TmpDataBuffer, UINT8 *ReconPtr, UINT8 *RefPtr1, UINT8 *RefPtr2,
                                INT16 *ChangePtr, UINT32 LineStep) {
    UINT32 i;
    INT16 *TmpDataPtr = TmpDataBuffer;

    for (i = 0; i < 8; i++) {
        TmpDataPtr[0] = (INT16)(ChangePtr[0] + ((RefPtr1[0] + RefPtr2[0]) >> 1));
        TmpDataPtr[1] = (INT16)(ChangePtr[1] + ((RefPtr1[1] + RefPtr2[1]) >> 1));
        TmpDataPtr[2] = (INT16)(ChangePtr[2] + ((RefPtr1[2] + RefPtr2[2]) >> 1));
        TmpDataPtr[3] = (INT16)(ChangePtr[3] + ((RefPtr1[3] + RefPtr2[3]) >> 1));
        TmpDataPtr[4] = (INT16)(ChangePtr[4] + ((RefPtr1[4] + RefPtr2[4]) >> 1));
        TmpDataPtr[5] = (INT16)(ChangePtr[5] + ((RefPtr1[5] + RefPtr2[5]) >> 1));
        TmpDataPtr[6] = (INT16)(ChangePtr[6] + ((RefPtr1[6] + RefPtr2[6]) >> 1));
        TmpDataPtr[7] = (INT16)(ChangePtr[7] + ((RefPtr1[7] + RefPtr2[7]) >> 1));

        ChangePtr += 8;
        TmpDataPtr += 8;
        RefPtr1 += LineStep;
        RefPtr2 += LineStep;
    }

    SatUnsigned8(ReconPtr, TmpDataBuffer, LineStep, 8);
}
