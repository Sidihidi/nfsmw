#include "pbdll.h"
#include "duck_mem.h"

static const int NearMacroBlocks[12][2] = {
    { -1, 0 },  { 0, -1 },  { -1, -1 }, { -1, 1 },  { -2, 0 },  { 0, -2 },
    { -1, -2 }, { -2, -1 }, { -2, 1 },  { -1, 2 },  { -2, -2 }, { -2, 2 },
};

void VP6_DeleteFragmentInfo(PB_INSTANCE *pbi) {
    if (pbi->mbi.CoeffsAlloc)
        duck_free(pbi->mbi.CoeffsAlloc);
    pbi->mbi.CoeffsAlloc = 0;
    pbi->mbi.Coeffs = 0;

    if (pbi->FragInfoAlloc)
        duck_free(pbi->FragInfoAlloc);
    pbi->FragInfoAlloc = 0;
    pbi->FragInfo = 0;

    if (pbi->fc.AboveYAlloc)
        duck_free(pbi->fc.AboveYAlloc);
    pbi->fc.AboveYAlloc = 0;
    pbi->fc.AboveY = 0;

    if (pbi->fc.AboveUAlloc)
        duck_free(pbi->fc.AboveUAlloc);
    pbi->fc.AboveUAlloc = 0;
    pbi->fc.AboveU = 0;

    if (pbi->fc.AboveVAlloc)
        duck_free(pbi->fc.AboveVAlloc);
    pbi->fc.AboveVAlloc = 0;
    pbi->fc.AboveV = 0;

    if (pbi->MBInterlacedAlloc)
        duck_free(pbi->MBInterlacedAlloc);
    pbi->MBInterlacedAlloc = 0;
    pbi->MBInterlaced = 0;

    if (pbi->MBMotionVectorAlloc)
        duck_free(pbi->MBMotionVectorAlloc);
    pbi->MBMotionVectorAlloc = 0;
    pbi->MBMotionVector = 0;

    if (pbi->predictionModeAlloc)
        duck_free(pbi->predictionModeAlloc);
    pbi->predictionModeAlloc = 0;
    pbi->predictionMode = 0;
}

BOOL VP6_AllocateFragmentInfo(PB_INSTANCE *pbi) {
    VP6_DeleteFragmentInfo(pbi);

    pbi->mbi.CoeffsAlloc = (Q_LIST_ENTRY(*)[64])duck_malloc(6 * 64 * sizeof(Q_LIST_ENTRY) + 32, DMEM_GENERAL);
    if (!pbi->mbi.CoeffsAlloc) {
        VP6_DeleteFragmentInfo(pbi);
        return FALSE;
    }
    pbi->mbi.Coeffs = (Q_LIST_ENTRY(*)[64])(((UINT32)pbi->mbi.CoeffsAlloc + 31) & ~31);

    pbi->fc.AboveYAlloc = (BLOCK_CONTEXT *)duck_malloc(pbi->HFragments * sizeof(BLOCK_CONTEXT) + 160, DMEM_GENERAL);
    if (!pbi->fc.AboveYAlloc) {
        VP6_DeleteFragmentInfo(pbi);
        return FALSE;
    }
    pbi->fc.AboveY = (BLOCK_CONTEXT *)(((UINT32)pbi->fc.AboveYAlloc + 31) & ~31);

    pbi->fc.AboveUAlloc =
        (BLOCK_CONTEXT *)duck_malloc((pbi->HFragments / 2) * sizeof(BLOCK_CONTEXT) + 160, DMEM_GENERAL);
    if (!pbi->fc.AboveUAlloc) {
        VP6_DeleteFragmentInfo(pbi);
        return FALSE;
    }
    pbi->fc.AboveU = (BLOCK_CONTEXT *)(((UINT32)pbi->fc.AboveUAlloc + 31) & ~31);

    pbi->fc.AboveVAlloc =
        (BLOCK_CONTEXT *)duck_malloc((pbi->HFragments / 2) * sizeof(BLOCK_CONTEXT) + 160, DMEM_GENERAL);
    if (!pbi->fc.AboveVAlloc) {
        VP6_DeleteFragmentInfo(pbi);
        return FALSE;
    }
    pbi->fc.AboveV = (BLOCK_CONTEXT *)(((UINT32)pbi->fc.AboveVAlloc + 31) & ~31);

    pbi->MBInterlacedAlloc = (char *)duck_malloc(pbi->MacroBlocks + 32, DMEM_GENERAL);
    if (!pbi->MBInterlacedAlloc) {
        VP6_DeleteFragmentInfo(pbi);
        return FALSE;
    }
    pbi->MBInterlaced = (char *)(((UINT32)pbi->MBInterlacedAlloc + 31) & ~31);

    pbi->predictionModeAlloc = (char *)duck_malloc(pbi->MacroBlocks + 32, DMEM_GENERAL);
    if (!pbi->predictionModeAlloc) {
        VP6_DeleteFragmentInfo(pbi);
        return FALSE;
    }
    pbi->predictionMode = (char *)(((UINT32)pbi->predictionModeAlloc + 31) & ~31);

    pbi->MBMotionVectorAlloc =
        (MOTION_VECTOR *)duck_malloc(pbi->MacroBlocks * sizeof(MOTION_VECTOR) + 32, DMEM_GENERAL);
    if (!pbi->MBMotionVectorAlloc) {
        VP6_DeleteFragmentInfo(pbi);
        return FALSE;
    }
    pbi->MBMotionVector = (MOTION_VECTOR *)(((UINT32)pbi->MBMotionVectorAlloc + 31) & ~31);

    pbi->FragInfoAlloc = (FRAG_INFO *)duck_malloc(pbi->UnitFragments * sizeof(FRAG_INFO) + 32, DMEM_GENERAL);
    if (!pbi->FragInfoAlloc) {
        VP6_DeleteFragmentInfo(pbi);
        return FALSE;
    }
    pbi->FragInfo = (FRAG_INFO *)(((UINT32)pbi->FragInfoAlloc + 31) & ~31);

    return TRUE;
}

void VP6_DeleteFrameInfo(PB_INSTANCE *pbi) {
    if (pbi->ThisFrameReconAlloc)
        duck_free(pbi->ThisFrameReconAlloc);
    if (pbi->GoldenFrameAlloc)
        duck_free(pbi->GoldenFrameAlloc);
    if (pbi->LastFrameReconAlloc)
        duck_free(pbi->LastFrameReconAlloc);
    if (pbi->PostProcessBufferAlloc)
        duck_free(pbi->PostProcessBufferAlloc);

    pbi->ThisFrameReconAlloc = 0;
    pbi->GoldenFrameAlloc = 0;
    pbi->LastFrameReconAlloc = 0;

    pbi->ThisFrameRecon = 0;
    pbi->GoldenFrame = 0;
    pbi->LastFrameRecon = 0;
    pbi->PostProcessBufferAlloc = 0;
}

BOOL VP6_AllocateFrameInfo(PB_INSTANCE *pbi, unsigned int FrameSize) {
    FrameSize += 32;

    VP6_DeleteFrameInfo(pbi);

    pbi->ThisFrameReconAlloc =
        (YUV_BUFFER_ENTRY *)duck_malloc(pbi->Configuration.YStride + FrameSize, DMEM_GENERAL);
    if (!pbi->ThisFrameReconAlloc) {
        VP6_DeleteFrameInfo(pbi);
        return FALSE;
    }
    pbi->ThisFrameRecon = (YUV_BUFFER_ENTRY *)(((UINT32)pbi->ThisFrameReconAlloc + 31) & ~31);

    pbi->GoldenFrameAlloc = (YUV_BUFFER_ENTRY *)duck_malloc(pbi->Configuration.YStride + FrameSize, DMEM_GENERAL);
    if (!pbi->GoldenFrameAlloc) {
        VP6_DeleteFrameInfo(pbi);
        return FALSE;
    }
    pbi->GoldenFrame = (YUV_BUFFER_ENTRY *)(((UINT32)pbi->GoldenFrameAlloc + 31) & ~31);

    pbi->LastFrameReconAlloc =
        (YUV_BUFFER_ENTRY *)duck_malloc(pbi->Configuration.YStride + FrameSize, DMEM_GENERAL);
    if (!pbi->LastFrameReconAlloc) {
        VP6_DeleteFrameInfo(pbi);
        return FALSE;
    }
    pbi->LastFrameRecon = (YUV_BUFFER_ENTRY *)(((UINT32)pbi->LastFrameReconAlloc + 31) & ~31);

    return TRUE;
}

BOOL VP6_InitFrameDetails(PB_INSTANCE *pbi) {
    UINT32 i;
    int FrameSize;

    if (pbi->CPUFree > 0) {
        VP6_SetPbParam(pbi, PBC_SET_CPUFREE, pbi->CPUFree);
    }

    pbi->YPlaneSize =
        pbi->Configuration.VideoFrameWidth * pbi->Configuration.VideoFrameHeight;
    pbi->UVPlaneSize = pbi->YPlaneSize / 4;
    pbi->HFragments = pbi->Configuration.VideoFrameWidth / pbi->Configuration.HFragPixels;
    pbi->VFragments = pbi->Configuration.VideoFrameHeight / pbi->Configuration.VFragPixels;
    pbi->YPlaneFragments = pbi->VFragments * pbi->HFragments;
    pbi->UnitFragments = pbi->YPlaneFragments * 3;
    pbi->UnitFragments /= 2;
    pbi->UVPlaneFragments = pbi->YPlaneFragments / 4;

    pbi->Configuration.YStride = pbi->Configuration.VideoFrameWidth + 96;
    pbi->Configuration.UVStride = pbi->Configuration.YStride / 2;

    pbi->ReconYPlaneSize =
        pbi->Configuration.YStride * (pbi->Configuration.VideoFrameHeight + 96);
    pbi->ReconUVPlaneSize = pbi->ReconYPlaneSize / 4;

    FrameSize = pbi->ReconYPlaneSize + 2 * pbi->ReconUVPlaneSize;

    pbi->YDataOffset = 0;
    pbi->UDataOffset = pbi->YPlaneSize;
    pbi->VDataOffset = pbi->YPlaneSize + pbi->UVPlaneSize;
    pbi->ReconYDataOffset = 0;
    pbi->ReconUDataOffset = pbi->ReconYPlaneSize;
    pbi->ReconVDataOffset = pbi->ReconYPlaneSize + pbi->ReconUVPlaneSize;

    pbi->MBRows = 6 + (pbi->Configuration.VideoFrameHeight / 16) +
                  ((pbi->Configuration.VideoFrameHeight % 16) ? 1 : 0);
    pbi->MBCols = 6 + (pbi->Configuration.VideoFrameWidth / 16) +
                  ((pbi->Configuration.VideoFrameWidth % 16) ? 1 : 0);
    pbi->MacroBlocks = pbi->MBRows * pbi->MBCols;

    for (i = 0; i < 12; i++) {
        pbi->mvNearOffset[i] = NearMacroBlocks[i][0] * pbi->MBCols + NearMacroBlocks[i][1];
    }

    ChangePostProcConfiguration(pbi->postproc, &pbi->Configuration);

    if (!VP6_AllocateFragmentInfo(pbi))
        return FALSE;

    if (!VP6_AllocateFrameInfo(pbi, FrameSize)) {
        VP6_DeleteFragmentInfo(pbi);
        return FALSE;
    }

    if (pbi->ScaleBuffer == 0 && pbi->OutputWidth &&
        (pbi->Configuration.VideoFrameWidth != pbi->OutputWidth ||
         pbi->Configuration.VideoFrameHeight != pbi->OutputHeight)) {
        pbi->ScaleBufferAlloc = (YUV_BUFFER_ENTRY *)duck_malloc(
            32 + 3 * (pbi->OutputWidth + 32) * (pbi->OutputHeight + 32) / 2, DMEM_GENERAL);
        pbi->ScaleBuffer = (YUV_BUFFER_ENTRY *)(((UINT32)pbi->ScaleBufferAlloc + 31) & ~31);
    }

    return TRUE;
}

void VP6_InitialiseConfiguration(PB_INSTANCE *pbi) {
    pbi->Configuration.HFragPixels = 8;
    pbi->Configuration.VFragPixels = 8;
}
