#include <string.h>

#include "pbdll.h"
#include "vputil.h"
#include "duck_mem.h"

BOOL VP6_ModeUsesMC[10] = { FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE };

static UINT32 VP6_DCQuantScaleP[64];
UINT32 CPUFrequency;

void VP6_DeleteTmpBuffers(PB_INSTANCE *pbi) {
    if (pbi->ReconDataBufferAlloc)
        duck_free(pbi->ReconDataBufferAlloc);
    if (pbi->LoopFilteredBlockAlloc)
        duck_free(pbi->LoopFilteredBlockAlloc);
    if (pbi->TmpDataBufferAlloc)
        duck_free(pbi->TmpDataBufferAlloc);
    if (pbi->TmpReconBufferAlloc)
        duck_free(pbi->TmpReconBufferAlloc);
    if (pbi->ScaleBufferAlloc)
        duck_free(pbi->ScaleBufferAlloc);

    pbi->ReconDataBufferAlloc = 0;
    pbi->LoopFilteredBlockAlloc = 0;
    pbi->TmpDataBufferAlloc = 0;
    pbi->TmpReconBufferAlloc = 0;
    pbi->ScaleBufferAlloc = 0;

    pbi->ReconDataBuffer = 0;
    pbi->LoopFilteredBlock = 0;
    pbi->TmpDataBuffer = 0;
    pbi->TmpReconBuffer = 0;
    pbi->ScaleBuffer = 0;
}

BOOL VP6_AllocateTmpBuffers(PB_INSTANCE *pbi) {
    VP6_DeleteTmpBuffers(pbi);

    pbi->ReconDataBufferAlloc = (INT16 *)duck_malloc(64 * sizeof(INT16) + 32, DMEM_GENERAL);
    if (!pbi->ReconDataBufferAlloc)
        goto Exit;
    pbi->ReconDataBuffer = (INT16 *)(((UINT32)pbi->ReconDataBufferAlloc + 31) & ~31);

    pbi->TmpDataBufferAlloc = (INT16 *)duck_malloc(64 * sizeof(INT16) + 32, DMEM_GENERAL);
    if (!pbi->TmpDataBufferAlloc)
        goto Exit;
    pbi->TmpDataBuffer = (INT16 *)(((UINT32)pbi->TmpDataBufferAlloc + 31) & ~31);

    pbi->LoopFilteredBlockAlloc = (UINT8 *)duck_malloc(256 + 32, DMEM_GENERAL);
    if (!pbi->LoopFilteredBlockAlloc)
        goto Exit;
    pbi->LoopFilteredBlock = (UINT8 *)(((UINT32)pbi->LoopFilteredBlockAlloc + 31) & ~31);

    pbi->TmpReconBufferAlloc = (INT16 *)duck_malloc(64 * sizeof(INT16) + 32, DMEM_GENERAL);
    if (!pbi->TmpReconBufferAlloc)
        goto Exit;
    pbi->TmpReconBuffer = (INT16 *)(((UINT32)pbi->TmpReconBufferAlloc + 31) & ~31);

    return TRUE;

Exit:
    VP6_DeleteTmpBuffers(pbi);
    return FALSE;
}

void VP6_DeletePBInstance(PB_INSTANCE **pbi) {
    if (*pbi) {
        VP6_DeleteTmpBuffers(*pbi);
        VP6_DeleteQuantizer(&(*pbi)->quantizer);
        DeletePostProcInstance(&(*pbi)->postproc);
    }

    duck_free(*pbi);
    *pbi = 0;
}

PB_INSTANCE *VP6_CreatePBInstance(void) {
    PB_INSTANCE *pbi;
    CONFIG_TYPE ConfigurationInit;
    int pbi_size = sizeof(PB_INSTANCE);

    memset(&ConfigurationInit, 0, sizeof(ConfigurationInit));
    ConfigurationInit.HFragPixels = 8;
    ConfigurationInit.VFragPixels = 8;

    pbi = (PB_INSTANCE *)duck_malloc(pbi_size, DMEM_GENERAL);
    if (!pbi)
        return 0;

    memset(pbi, 0, pbi_size);
    pbi->Configuration = ConfigurationInit;

    if (!VP6_AllocateTmpBuffers(pbi)) {
        duck_free(pbi);
        return 0;
    }

    pbi->CPUFree = 70;
    pbi->idct = idct;

    memset(pbi->DcProbs, 0, sizeof(pbi->DcProbs));
    memset(pbi->AcProbs, 0, sizeof(pbi->AcProbs));

    return pbi;
}

void VP6_VPInitLibrary(void) {
    int i;

    CPUFrequency = VP6_GetProcessorFrequency();

    VP6_DMachineSpecificConfig();

    for (i = 0; i < 64; i++) {
        INT32 dcScale = VP6_DcQuant[i] / 2 + 2;
        VP6_DCQuantScaleP[i] = dcScale;
    }

    InitPostProcessing(VP6_DCQuantScaleP, VP6_DCQuantScaleP, VP6_DCQuantScaleP, 7);

    InitVPUtil();
}

void VP6_VPDeInitLibrary(void) {
}
