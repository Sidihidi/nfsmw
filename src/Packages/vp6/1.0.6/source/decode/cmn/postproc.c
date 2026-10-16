#include "postp.h"
#include "duck_mem.h"

#include <stdlib.h>
#include <math.h>

/* Asi las declara esta unidad y no con los prototipos de <string.h>: en
   GameCube memcpy se llama sin prototipo (lleva crclr delante) y el memset de
   CreatePostProcInstance no se expande como incorporado. */
extern void *memset(void *, int, unsigned long);
extern void *memcpy();

UINT8 LimitVal_VP31[768];

void (*FilteringVert_12)(UINT32 QValue, UINT8 *Src, INT32 Pitch);
void (*FilteringHoriz_12)(UINT32 QValue, UINT8 *Src, INT32 Pitch);
void (*FilteringVert_8)(UINT32 QValue, UINT8 *Src, INT32 Pitch);
void (*FilteringHoriz_8)(UINT32 QValue, UINT8 *Src, INT32 Pitch);
void (*VerticalBand_4_5_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
void (*LastVerticalBand_4_5_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
void (*VerticalBand_3_5_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
void (*LastVerticalBand_3_5_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
void (*HorizontalLine_1_2_Scale)(const unsigned char *source, unsigned int sourceWidth, unsigned char *dest,
                                 unsigned int destWidth);
void (*HorizontalLine_3_5_Scale)(const unsigned char *source, unsigned int sourceWidth, unsigned char *dest,
                                 unsigned int destWidth);
void (*HorizontalLine_4_5_Scale)(const unsigned char *source, unsigned int sourceWidth, unsigned char *dest,
                                 unsigned int destWidth);
void (*VerticalBand_1_2_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
void (*LastVerticalBand_1_2_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
void (*FilterHoriz_Simple)(xPB_INST ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
void (*FilterVert_Simple)(xPB_INST ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
void (*DeringBlockWeak)(xPB_INST pbi, const UINT8 *SrcPtr, UINT8 *DstPtr, INT32 Pitch, UINT32 FragQIndex,
                        UINT32 *QuantScale);
void (*DeringBlockStrong)(xPB_INST pbi, const UINT8 *SrcPtr, UINT8 *DstPtr, INT32 Pitch, UINT32 FragQIndex,
                          UINT32 *QuantScale);
void (*DeblockLoopFilteredBand)(xPB_INST pbi, UINT8 *SrcPtr, UINT8 *DesPtr, UINT32 PlaneLineStep,
                                UINT32 FragsAcross, UINT32 StartFrag, UINT32 *QuantScale);
void (*DeblockNonFilteredBand)(xPB_INST pbi, UINT8 *SrcPtr, UINT8 *DesPtr, UINT32 PlaneLineStep,
                               UINT32 FragsAcross, UINT32 StartFrag, UINT32 *QuantScale);
void (*DeblockNonFilteredBandNewFilter)(xPB_INST pbi, UINT8 *SrcPtr, UINT8 *DesPtr, UINT32 PlaneLineStep,
                                        UINT32 FragsAcross, UINT32 StartFrag, UINT32 *QuantScale);
INT32 *(*SetupBoundingValueArray)(xPB_INST ppi, INT32 FLimit);
INT32 *(*SetupDeblockValueArray)(xPB_INST pbi, INT32 FLimit);
void (*FilterHoriz)(xPB_INST ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
void (*FilterVert)(xPB_INST ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
void (*ClampLevels)(xPB_INST pbi, INT32 BlackClamp, INT32 WhiteClamp, UINT8 *Src, UINT8 *Dst);
void (*FastDeInterlace)(UINT8 *SrcPtr, UINT8 *DstPtr, INT32 Width, INT32 Height, INT32 Stride);
void (*PlaneAddNoise)(UINT8 *Start, UINT32 Width, UINT32 Height, INT32 Pitch, int q);

void InitPostProcessing(UINT32 *DCQuantScaleV2p, UINT32 *DCQuantScaleUVp, UINT32 *DCQuantScaleV1p,
                        UINT32 Version) {
    int i;

    for (i = 0; i < 768; i++) {
        int x = i - 256;
        LimitVal_VP31[i] = (x < 0) ? 0 : ((x > 255) ? 255 : x);
    }

    DCQuantScaleV2 = DCQuantScaleV2p;
    DCQuantScaleUV = DCQuantScaleUVp;
    DCQuantScaleV1 = DCQuantScaleV1p;

    for (i = 0; i < 64; i++) {
        DeringModifierV1[i] = DCQuantScaleV1p[i];
    }

    if (Version > 5) {
        LoopFilterLimitValuesV2 = LoopFilterLimitValuesVp6;
        DeblockLimitValuesV2 = DeblockLimitValuesVp6;
    } else if (Version > 4) {
        LoopFilterLimitValuesV2 = LoopFilterLimitValuesVp5;
        DeblockLimitValuesV2 = DeblockLimitValuesVp5;
    } else {
        LoopFilterLimitValuesV2 = LoopFilterLimitValuesVp4;
        DeblockLimitValuesV2 = DeblockLimitValuesVp4;
    }

    PostProcMachineSpecificConfig(Version);
}

void DeletePostProcBuffers(POSTPROC_INSTANCE *ppi) {
    if (ppi->IntermediateBufferAlloc) {
        duck_free(ppi->IntermediateBufferAlloc);
    }
    ppi->IntermediateBufferAlloc = 0;
    ppi->IntermediateBuffer = 0;

    if (ppi->FiltBoundingValueAlloc) {
        duck_free(ppi->FiltBoundingValueAlloc);
    }
    ppi->FiltBoundingValueAlloc = 0;
    ppi->FiltBoundingValue = 0;

    if (ppi->DeblockBoundingValueAlloc) {
        duck_free(ppi->DeblockBoundingValueAlloc);
    }
    ppi->DeblockBoundingValueAlloc = 0;
    ppi->DeblockBoundingValue = 0;

    if (ppi->FragQIndexAlloc) {
        duck_free(ppi->FragQIndexAlloc);
    }
    ppi->FragQIndexAlloc = 0;
    ppi->FragQIndex = 0;

    if (ppi->FragmentVariancesAlloc) {
        duck_free(ppi->FragmentVariancesAlloc);
    }
    ppi->FragmentVariancesAlloc = 0;
    ppi->FragmentVariances = 0;

    if (ppi->FragDeblockingFlagAlloc) {
        duck_free(ppi->FragDeblockingFlagAlloc);
    }
    ppi->FragDeblockingFlagAlloc = 0;
    ppi->FragDeblockingFlag = 0;
}

int AllocatePostProcBuffers(POSTPROC_INSTANCE *ppi) {
    DeletePostProcBuffers(ppi);

    ppi->IntermediateBufferAlloc = duck_malloc(
        (ppi->YStride * (ppi->Configuration.VideoFrameHeight + ppi->MVBorder * 2) * 3) / 2 + 32, 0);
    if (ppi->IntermediateBufferAlloc == 0) {
        DeletePostProcBuffers(ppi);
        return 0;
    }
    ppi->IntermediateBuffer = (UINT8 *)(((UINT32)ppi->IntermediateBufferAlloc + 31) & ~31);

    ppi->FiltBoundingValueAlloc = duck_malloc(256 * 8 + 32, 0);
    if (ppi->FiltBoundingValueAlloc == 0) {
        DeletePostProcBuffers(ppi);
        return 0;
    }
    ppi->FiltBoundingValue = (int *)(((UINT32)ppi->FiltBoundingValueAlloc + 31) & ~31);

    ppi->DeblockBoundingValueAlloc = duck_malloc(256 * 8 + 32, 0);
    if (ppi->DeblockBoundingValueAlloc == 0) {
        DeletePostProcBuffers(ppi);
        return 0;
    }
    ppi->DeblockBoundingValue = (int *)(((UINT32)ppi->DeblockBoundingValueAlloc + 31) & ~31);

    ppi->FragQIndexAlloc = duck_malloc(ppi->UnitFragments * sizeof(int) + 32, 0);
    if (ppi->FragQIndexAlloc == 0) {
        DeletePostProcBuffers(ppi);
        return 0;
    }
    ppi->FragQIndex = (int *)(((UINT32)ppi->FragQIndexAlloc + 31) & ~31);

    ppi->FragmentVariancesAlloc = duck_malloc(ppi->UnitFragments * sizeof(int) + 32, 0);
    if (ppi->FragmentVariancesAlloc == 0) {
        DeletePostProcBuffers(ppi);
        return 0;
    }
    ppi->FragmentVariances = (int *)(((UINT32)ppi->FragmentVariancesAlloc + 31) & ~31);

    ppi->FragDeblockingFlagAlloc = duck_malloc(ppi->UnitFragments + 32, 0);
    if (ppi->FragDeblockingFlagAlloc == 0) {
        DeletePostProcBuffers(ppi);
        return 0;
    }
    ppi->FragDeblockingFlag = (UINT8 *)(((UINT32)ppi->FragDeblockingFlagAlloc + 31) & ~31);

    return 1;
}

void ChangePostProcConfiguration(POSTPROC_INSTANCE *ppi, CONFIG_TYPE *ConfigurationInit) {
    memcpy((void *)&ppi->Configuration, (void *)ConfigurationInit, sizeof(CONFIG_TYPE));

    ppi->HFragments = ppi->Configuration.VideoFrameWidth >> 3;
    ppi->VFragments = ppi->Configuration.VideoFrameHeight >> 3;
    ppi->YStride = ppi->Configuration.YStride;
    ppi->UVStride = ppi->Configuration.UVStride;
    ppi->YPlaneFragments = ppi->HFragments * ppi->VFragments;
    ppi->UVPlaneFragments = ppi->YPlaneFragments / 4;
    ppi->UnitFragments = ppi->YPlaneFragments + 2 * ppi->UVPlaneFragments;
    ppi->MVBorder = (ppi->YStride - 8 * ppi->HFragments) / 2;

    ppi->ReconYDataOffset = ppi->MVBorder * ppi->YStride + ppi->MVBorder;
    ppi->ReconYDataOffset = ppi->MVBorder * ppi->YStride + ppi->MVBorder;
    ppi->ReconUDataOffset =
        (ppi->YStride * (ppi->Configuration.VideoFrameHeight + ppi->MVBorder * 2)) +
        ppi->MVBorder / 2 * ppi->UVStride + ppi->MVBorder / 2;
    ppi->ReconVDataOffset =
        (ppi->YStride * (ppi->Configuration.VideoFrameHeight + ppi->MVBorder * 2)) +
        (ppi->UVStride * (ppi->Configuration.VideoFrameHeight / 2 + ppi->MVBorder)) +
        ppi->MVBorder / 2 * ppi->UVStride + ppi->MVBorder / 2;
}

POSTPROC_INSTANCE *CreatePostProcInstance(CONFIG_TYPE *ConfigurationInit) {
    POSTPROC_INSTANCE *ppi;

    ppi = duck_malloc(sizeof(POSTPROC_INSTANCE), DMEM_GENERAL);
    if (!ppi)
        return 0;

    memset(ppi, 0, sizeof(POSTPROC_INSTANCE));

    ChangePostProcConfiguration(ppi, ConfigurationInit);

    ppi->AddNoiseMode = 1;

    return ppi;
}

void DeletePostProcInstance(POSTPROC_INSTANCE **ppi) {
    if (*ppi) {
        DeletePostProcBuffers(*ppi);
        duck_free(*ppi);
        *ppi = 0;
    }
}

void SetPPInterlacedMode(POSTPROC_INSTANCE *ppi, int Interlaced) {
    ppi->Configuration.Interlaced = Interlaced;
}

void SetDeInterlaceMode(POSTPROC_INSTANCE *ppi, int DeInterlaceMode) {
    ppi->DeInterlaceMode = DeInterlaceMode;
}

void SetAddNoiseMode(POSTPROC_INSTANCE *ppi, int AddNoiseMode) {
    ppi->AddNoiseMode = AddNoiseMode;
}

void UpdateFragQIndex(POSTPROC_INSTANCE *ppi) {
    unsigned int i;
    int FrameQIndex = ppi->FrameQIndex;

    for (i = 0; i < ppi->UnitFragments; i++) {
        if (ppi->FragInfo[i * ppi->FragInfoElementSize] & ppi->FragInfoCodedMask) {
            ppi->FragQIndex[i] = FrameQIndex;
        }
    }
}

double gaussian(double sigma, double mu, double x) {
    return 1 / (sigma * sqrt(6.2831853)) * (exp(-(x - mu) * (x - mu) / (2 * sigma * sigma)));
}

void PlaneAddNoise_C(UINT8 *Start, UINT32 Width, UINT32 Height, int Pitch, int q) {
    unsigned int i, j;
    char BlackClamp[16];
    char WhiteClamp[16];
    char BothClamp[16];
    char CharDist[300];
    char Rand[2048];
    double sigma;

    sigma = 1 + .8 * (63 - q) / 63.0;

    {
        double i;
        int next, j;

        next = 0;

        for (i = -32; i < 32; i++) {
            int a = (int)(.5 + 256 * gaussian(sigma, 0, i));

            if (a) {
                for (j = 0; j < a; j++) {
                    CharDist[next + j] = (char)i;
                }
                next = next + j;
            }
        }

        for (next = next; next < 256; next++) {
            CharDist[next] = 0;
        }
    }

    for (i = 0; i < 2048; i++) {
        Rand[i] = CharDist[rand() & 0xff];
    }

    for (i = 0; i < 16; i++) {
        BlackClamp[i] = -CharDist[0];
        WhiteClamp[i] = -CharDist[0];
        BothClamp[i] = -2 * CharDist[0];
    }

    for (i = 0; i < Height; i++) {
        UINT8 *Pos = Start + i * Pitch;
        char *Ref = &Rand[rand() & 0xff];

        for (j = 0; j < Width; j++) {
            if (Pos[j] < -CharDist[0]) {
                Pos[j] = -CharDist[0];
            }

            if (Pos[j] > 255 - CharDist[0]) {
                Pos[j] = 255 - CharDist[0];
            }

            Pos[j] += Ref[j];
        }
    }
}

void PostProcess(POSTPROC_INSTANCE *ppi, INT32 Vp3VersionNo, INT32 FrameType,
                 INT32 PostProcessingLevel, INT32 FrameQIndex, UINT8 *LastFrameRecon,
                 UINT8 *PostProcessBuffer, UINT8 *FragInfo, UINT32 FragInfoElementSize,
                 UINT32 FragInfoCodedMask) {
    UINT32 ReconUVPlaneSize;

    ppi->Vp3VersionNo = Vp3VersionNo;
    ppi->FrameType = FrameType;
    ppi->PostProcessingLevel = PostProcessingLevel;
    ppi->FrameQIndex = FrameQIndex;
    ppi->LastFrameRecon = LastFrameRecon;
    ppi->PostProcessBuffer = PostProcessBuffer;
    ppi->FragInfo = FragInfo;
    ppi->FragInfoElementSize = FragInfoElementSize;
    ppi->FragInfoCodedMask = FragInfoCodedMask;

    switch (ppi->PostProcessingLevel) {
    case 8:
        UpdateFragQIndex(ppi);

        if (ppi->Vp3VersionNo > 1) {
            if (ppi->Configuration.Interlaced && ppi->DeInterlaceMode) {
                SimpleDeblockFrame(ppi, ppi->LastFrameRecon, ppi->IntermediateBuffer);

                ReconUVPlaneSize = ppi->VFragments * (ppi->YStride + ppi->YStride);
                memcpy(ppi->PostProcessBuffer + ppi->ReconUDataOffset,
                       ppi->IntermediateBuffer + ppi->ReconUDataOffset, ReconUVPlaneSize);
                memcpy(ppi->PostProcessBuffer + ppi->ReconVDataOffset,
                       ppi->IntermediateBuffer + ppi->ReconVDataOffset, ReconUVPlaneSize);

                FastDeInterlace(ppi->IntermediateBuffer + ppi->ReconYDataOffset,
                                ppi->PostProcessBuffer + ppi->ReconYDataOffset,
                                ppi->HFragments * 8, ppi->VFragments * 8, ppi->YStride);
            } else {
                SimpleDeblockFrame(ppi, ppi->LastFrameRecon, ppi->PostProcessBuffer);
            }
        } else {
            DeblockFrame(ppi, ppi->LastFrameRecon, ppi->PostProcessBuffer);
        }
        break;

    case 5:
    case 6:
        if (ppi->Vp3VersionNo <= 4) {
            UpdateFragQIndex(ppi);
        } else if (ppi->Configuration.Interlaced) {
            if (ppi->DeInterlaceMode == 0) {
                DeblockFrameInterlaced(ppi, ppi->LastFrameRecon, ppi->PostProcessBuffer);
                UpdateUMVBorder(ppi, ppi->PostProcessBuffer);
                DeringFrameInterlaced(ppi, ppi->PostProcessBuffer, ppi->PostProcessBuffer);
            } else {
                DeblockFrameInterlaced(ppi, ppi->LastFrameRecon, ppi->IntermediateBuffer);
                UpdateUMVBorder(ppi, ppi->IntermediateBuffer);
                DeringFrameInterlaced(ppi, ppi->IntermediateBuffer, ppi->IntermediateBuffer);

                ReconUVPlaneSize = ppi->VFragments * (ppi->YStride + ppi->YStride);
                memcpy(ppi->PostProcessBuffer + ppi->ReconUDataOffset,
                       ppi->IntermediateBuffer + ppi->ReconUDataOffset, ReconUVPlaneSize);
                memcpy(ppi->PostProcessBuffer + ppi->ReconVDataOffset,
                       ppi->IntermediateBuffer + ppi->ReconVDataOffset, ReconUVPlaneSize);

                FastDeInterlace(ppi->IntermediateBuffer + ppi->ReconYDataOffset,
                                ppi->PostProcessBuffer + ppi->ReconYDataOffset,
                                ppi->HFragments * 8, ppi->VFragments * 8, ppi->YStride);
            }
            break;
        }

        DeblockFrame(ppi, ppi->LastFrameRecon, ppi->PostProcessBuffer);
        UpdateUMVBorder(ppi, ppi->PostProcessBuffer);
        DeringFrame(ppi, ppi->PostProcessBuffer, ppi->PostProcessBuffer);

        if (ppi->AddNoiseMode) {
            PlaneAddNoise(ppi->PostProcessBuffer + ppi->ReconYDataOffset, ppi->HFragments * 8,
                          ppi->VFragments * 8, ppi->YStride, FrameQIndex);
        }
        break;

    case 7:
        if (ppi->Vp3VersionNo > 4) {
            if (ppi->Configuration.Interlaced) {
                if (ppi->DeInterlaceMode) {
                    DeblockFrameInterlaced(ppi, ppi->LastFrameRecon, ppi->IntermediateBuffer);

                    ReconUVPlaneSize = ppi->VFragments * (ppi->YStride + ppi->YStride);
                    memcpy(ppi->PostProcessBuffer + ppi->ReconUDataOffset,
                           ppi->IntermediateBuffer + ppi->ReconUDataOffset, ReconUVPlaneSize);
                    memcpy(ppi->PostProcessBuffer + ppi->ReconVDataOffset,
                           ppi->IntermediateBuffer + ppi->ReconVDataOffset, ReconUVPlaneSize);

                    FastDeInterlace(ppi->IntermediateBuffer + ppi->ReconYDataOffset,
                                    ppi->PostProcessBuffer + ppi->ReconYDataOffset,
                                    ppi->HFragments * 8, ppi->VFragments * 8, ppi->YStride);
                } else {
                    DeblockFrameInterlaced(ppi, ppi->LastFrameRecon, ppi->PostProcessBuffer);
                }
                break;
            }
        } else {
            UpdateFragQIndex(ppi);
        }

        DeblockFrame(ppi, ppi->LastFrameRecon, ppi->PostProcessBuffer);
        UpdateUMVBorder(ppi, ppi->PostProcessBuffer);
        DeringFrame(ppi, ppi->PostProcessBuffer, ppi->PostProcessBuffer);
        break;

    case 4:
        if (ppi->Vp3VersionNo > 4) {
            if (ppi->Configuration.Interlaced) {
                if (ppi->DeInterlaceMode == 0) {
                    DeblockFrameInterlaced(ppi, ppi->LastFrameRecon, ppi->PostProcessBuffer);
                } else {
                    DeblockFrameInterlaced(ppi, ppi->LastFrameRecon, ppi->IntermediateBuffer);

                    ReconUVPlaneSize = ppi->VFragments * (ppi->YStride + ppi->YStride);
                    memcpy(ppi->PostProcessBuffer + ppi->ReconUDataOffset,
                           ppi->IntermediateBuffer + ppi->ReconUDataOffset, ReconUVPlaneSize);
                    memcpy(ppi->PostProcessBuffer + ppi->ReconVDataOffset,
                           ppi->IntermediateBuffer + ppi->ReconVDataOffset, ReconUVPlaneSize);

                    FastDeInterlace(ppi->IntermediateBuffer + ppi->ReconYDataOffset,
                                    ppi->PostProcessBuffer + ppi->ReconYDataOffset,
                                    ppi->HFragments * 8, ppi->VFragments * 8, ppi->YStride);
                }
                break;
            }
        } else {
            UpdateFragQIndex(ppi);
        }

        DeblockFrame(ppi, ppi->LastFrameRecon, ppi->PostProcessBuffer);
        break;

    case 1:
        UpdateFragQIndex(ppi);
        break;

    case 0:
        if (ppi->Configuration.Interlaced && ppi->DeInterlaceMode) {
            ReconUVPlaneSize = ppi->VFragments * (ppi->YStride + ppi->YStride);
            memcpy(ppi->PostProcessBuffer + ppi->ReconUDataOffset,
                   ppi->LastFrameRecon + ppi->ReconUDataOffset, ReconUVPlaneSize);
            memcpy(ppi->PostProcessBuffer + ppi->ReconVDataOffset,
                   ppi->LastFrameRecon + ppi->ReconVDataOffset, ReconUVPlaneSize);

            FastDeInterlace(ppi->LastFrameRecon + ppi->ReconYDataOffset,
                            ppi->PostProcessBuffer + ppi->ReconYDataOffset,
                            ppi->HFragments * 8, ppi->VFragments * 8, ppi->YStride);
        }
        break;

    default:
        DeblockFrame(ppi, ppi->LastFrameRecon, ppi->PostProcessBuffer);
        UpdateUMVBorder(ppi, ppi->PostProcessBuffer);
        DeringFrame(ppi, ppi->PostProcessBuffer, ppi->PostProcessBuffer);
        break;
    }
}
