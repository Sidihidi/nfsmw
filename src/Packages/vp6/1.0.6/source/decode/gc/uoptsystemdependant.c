#include "../cmn/vputil.h"

void fillidctconstants(void) {
}

void UtilMachineSpecificConfig(void) {
    int i;

    for (i = 0; i <= 64; i++) {
        if (i <= 1) {
            idct[i] = IDct1_GC;
        } else if (i <= 10) {
            idct[i] = IDct10;
        } else {
            idct[i] = IDct64_GC;
        }
    }

    for (i = 0; i <= 64; i++) {
        if (i <= 1) {
            idctc[i] = IDct1_GC;
        } else if (i <= 10) {
            idct[i] = IDct10; /* asi en el binario de GameCube: el segundo bucle escribe idct, no idctc */
        } else {
            idctc[i] = IDct64_GC;
        }
    }

    ClearSysState = ClearSysState_C;
    ReconIntra = ScalarReconIntra_GC;
    ReconInter = ScalarReconInter_GC;
    ReconInterHalfPixel2 = ScalarReconInterHalfPixel2;
    AverageBlock = AverageBlock_C;
    UnpackBlock = UnpackBlock_GC;
    ReconBlock = ReconBlock_GC;
    SubtractBlock = SubtractBlock_C;
    CopyBlock = CopyBlock_C;
    Copy12x12 = Copy12x12_C;
    FilterBlockBil_8 = FilterBlockBil_8_C;
    FilterBlock = FilterBlock_GC;
}
