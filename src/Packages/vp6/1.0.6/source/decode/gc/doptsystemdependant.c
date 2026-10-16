#include "../cmn/postp.h"

void PostProcMachineSpecificConfig(UINT32 Version) {
    FilterHoriz = FilterHoriz_Generic;
    FilterVert = FilterVert_Generic;
    SetupBoundingValueArray = SetupBoundingValueArray_Generic;
    SetupDeblockValueArray = SetupDeblockValueArray_Generic;
    DeringBlockWeak = DeringBlockWeak_C;
    DeringBlockStrong = DeringBlockStrong_C;
    DeblockLoopFilteredBand = DeblockLoopFilteredBand_C;
    DeblockNonFilteredBand = DeblockNonFilteredBand_C;
    DeblockNonFilteredBandNewFilter = DeblockNonFilteredBandNewFilter_C;
    FilterHoriz_Simple = FilterHoriz_Simple_C;
    FilterVert_Simple = FilterVert_Simple_C;
    HorizontalLine_1_2_Scale = HorizontalLine_1_2_Scale_C;
    VerticalBand_1_2_Scale = VerticalBand_1_2_Scale_C;
    LastVerticalBand_1_2_Scale = LastVerticalBand_1_2_Scale_C;
    HorizontalLine_3_5_Scale = HorizontalLine_3_5_Scale_C;
    VerticalBand_3_5_Scale = VerticalBand_3_5_Scale_C;
    LastVerticalBand_3_5_Scale = LastVerticalBand_3_5_Scale_C;
    HorizontalLine_4_5_Scale = HorizontalLine_4_5_Scale_C;
    VerticalBand_4_5_Scale = VerticalBand_4_5_Scale_C;
    LastVerticalBand_4_5_Scale = LastVerticalBand_4_5_Scale_C;
    FilteringHoriz_8 = FilteringHoriz_8_C;
    FilteringVert_8 = FilteringVert_8_C;
    FilteringHoriz_12 = FilteringHoriz_12_C;
    FilteringVert_12 = FilteringVert_12_C;
    FastDeInterlace = CFastDeInterlace;
    ClampLevels = ClampLevels_C;
    PlaneAddNoise = PlaneAddNoise_C;
}
