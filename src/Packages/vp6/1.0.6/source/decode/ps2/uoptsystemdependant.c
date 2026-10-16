#include "../cmn/vputil.h"

void fillidctconstants(void) {
}

void UtilMachineSpecificConfig(void) {
    int i;

    for (i = 0; i <= 64; i++) {
        if (i <= 1) {
            idct[i] = IDct1_PS2;
        } else if (i <= 10) {
            idct[i] = IDct10_PS2;
        } else {
            idct[i] = IDct64_PS2;
        }
    }

    for (i = 0; i <= 64; i++) {
        if (i <= 1) {
            idctc[i] = IDct1_PS2;
        } else if (i <= 10) {
            idctc[i] = IDct10_PS2;
        } else {
            idctc[i] = IDct64_PS2;
        }
    }

    ClearSysState = ClearSysState_C;
    ReconIntra = ScalarReconIntra_PS2;
    ReconInter = ScalarReconInter_PS2;
    ReconInterHalfPixel2 = ScalarReconInterHalfPixel2;
    AverageBlock = AverageBlock_C;
    UnpackBlock = UnpackBlock_PS2;
    ReconBlock = ReconBlock_PS2;
    SubtractBlock = SubtractBlock_C;
    CopyBlock = CopyBlock_C;
    Copy12x12 = Copy12x12_C;
    FilterBlockBil_8 = FilterBlockBil_8_C;
    FilterBlock = FilterBlock_PS2;
}
