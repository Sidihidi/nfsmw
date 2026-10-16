#ifndef POSTP_H
#define POSTP_H

#include "codec_common.h"

/* Tipos y funciones del postproceso (postproc.c, deblock.c, dering.c,
   loopfilter.c, simpledeblocker.c, scale.c, borders.c, clamp.c y el
   doptsystemdependant.c de cada plataforma). Los stabs de la A124 prueban que
   estas unidades no ven pbdll.h: su YUV_BUFFER_CONFIG no tiene YBufferStart y
   su xPB_INST apunta a POSTPROC_INSTANCE, no a PB_INSTANCE. */

typedef struct {
    int YWidth;
    int YHeight;
    int YStride;
    int UVWidth;
    int UVHeight;
    int UVStride;
    char *YBuffer;
    char *UBuffer;
    char *VBuffer;
} YUV_BUFFER_CONFIG;

typedef enum {
    MAINTAIN_ASPECT_RATIO = 0,
    SCALE_TO_FIT = 1,
    CENTER = 2,
    OTHER = 3
} SCALE_MODE;

typedef struct {
    INT32 Vp3VersionNo;
    INT32 FrameType;
    INT32 PostProcessingLevel;
    INT32 FrameQIndex;
    UINT8 *LastFrameRecon;
    UINT8 *PostProcessBuffer;
    UINT8 *FragInfo;
    UINT32 FragInfoElementSize;
    UINT32 FragInfoCodedMask;
    INT32 *FragQIndex;
    INT32 *FragmentVariances;
    UINT8 *FragDeblockingFlag;
    INT32 *BoundingValuePtr;
    INT32 *FiltBoundingValue;
    INT32 *DeblockValuePtr;
    INT32 *DeblockBoundingValue;
    CONFIG_TYPE Configuration;
    UINT32 ReconYDataOffset;
    UINT32 ReconUDataOffset;
    UINT32 ReconVDataOffset;
    UINT32 YPlaneFragments;
    UINT32 UVPlaneFragments;
    UINT32 UnitFragments;
    UINT32 HFragments;
    UINT32 VFragments;
    INT32 YStride;
    INT32 UVStride;
    INT32 *FiltBoundingValueAlloc;
    INT32 *DeblockBoundingValueAlloc;
    INT32 *FragQIndexAlloc;
    INT32 *FragmentVariancesAlloc;
    UINT8 *FragDeblockingFlagAlloc;
    UINT32 MVBorder;
    UINT8 *IntermediateBufferAlloc;
    UINT8 *IntermediateBuffer;
    UINT32 DeInterlaceMode;
    UINT32 AddNoiseMode;
} POSTPROC_INSTANCE;

typedef POSTPROC_INSTANCE *xPB_INST;

/* postproc.c */
extern UINT8 LimitVal_VP31[768];
extern void (*FilteringVert_12)(UINT32 QValue, UINT8 *Src, INT32 Pitch);
extern void (*FilteringHoriz_12)(UINT32 QValue, UINT8 *Src, INT32 Pitch);
extern void (*FilteringVert_8)(UINT32 QValue, UINT8 *Src, INT32 Pitch);
extern void (*FilteringHoriz_8)(UINT32 QValue, UINT8 *Src, INT32 Pitch);
extern void (*VerticalBand_4_5_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void (*LastVerticalBand_4_5_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void (*VerticalBand_3_5_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void (*LastVerticalBand_3_5_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void (*HorizontalLine_1_2_Scale)(const unsigned char *source, unsigned int sourceWidth,
                                        unsigned char *dest, unsigned int destWidth);
extern void (*HorizontalLine_3_5_Scale)(const unsigned char *source, unsigned int sourceWidth,
                                        unsigned char *dest, unsigned int destWidth);
extern void (*HorizontalLine_4_5_Scale)(const unsigned char *source, unsigned int sourceWidth,
                                        unsigned char *dest, unsigned int destWidth);
extern void (*VerticalBand_1_2_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void (*LastVerticalBand_1_2_Scale)(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void (*FilterHoriz_Simple)(xPB_INST ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void (*FilterVert_Simple)(xPB_INST ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void (*DeringBlockWeak)(xPB_INST pbi, const UINT8 *SrcPtr, UINT8 *DstPtr, INT32 Pitch,
                               UINT32 FragQIndex, UINT32 *QuantScale);
extern void (*DeringBlockStrong)(xPB_INST pbi, const UINT8 *SrcPtr, UINT8 *DstPtr, INT32 Pitch,
                                 UINT32 FragQIndex, UINT32 *QuantScale);
extern void (*DeblockLoopFilteredBand)(xPB_INST pbi, UINT8 *SrcPtr, UINT8 *DesPtr, UINT32 PlaneLineStep,
                                       UINT32 FragsAcross, UINT32 StartFrag, UINT32 *QuantScale);
extern void (*DeblockNonFilteredBand)(xPB_INST pbi, UINT8 *SrcPtr, UINT8 *DesPtr, UINT32 PlaneLineStep,
                                      UINT32 FragsAcross, UINT32 StartFrag, UINT32 *QuantScale);
extern void (*DeblockNonFilteredBandNewFilter)(xPB_INST pbi, UINT8 *SrcPtr, UINT8 *DesPtr,
                                               UINT32 PlaneLineStep, UINT32 FragsAcross, UINT32 StartFrag,
                                               UINT32 *QuantScale);
extern INT32 *(*SetupBoundingValueArray)(xPB_INST ppi, INT32 FLimit);
extern INT32 *(*SetupDeblockValueArray)(xPB_INST pbi, INT32 FLimit);
extern void (*FilterHoriz)(xPB_INST ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void (*FilterVert)(xPB_INST ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void (*ClampLevels)(xPB_INST pbi, INT32 BlackClamp, INT32 WhiteClamp, UINT8 *Src, UINT8 *Dst);
extern void (*FastDeInterlace)(UINT8 *SrcPtr, UINT8 *DstPtr, INT32 Width, INT32 Height, INT32 Stride);
extern void (*PlaneAddNoise)(UINT8 *Start, UINT32 Width, UINT32 Height, INT32 Pitch, int q);

extern void InitPostProcessing(UINT32 *DCQuantScaleV2p, UINT32 *DCQuantScaleUVp, UINT32 *DCQuantScaleV1p,
                               UINT32 Version);
extern void DeletePostProcBuffers(POSTPROC_INSTANCE *ppi);
extern INT32 AllocatePostProcBuffers(POSTPROC_INSTANCE *ppi);
extern void ChangePostProcConfiguration(POSTPROC_INSTANCE *ppi, CONFIG_TYPE *ConfigurationInit);
extern POSTPROC_INSTANCE *CreatePostProcInstance(CONFIG_TYPE *ConfigurationInit);
extern void DeletePostProcInstance(POSTPROC_INSTANCE **ppi);
extern void SetPPInterlacedMode(POSTPROC_INSTANCE *ppi, int Interlaced);
extern void SetDeInterlaceMode(POSTPROC_INSTANCE *ppi, int DeInterlaceMode);
extern void SetAddNoiseMode(POSTPROC_INSTANCE *ppi, int AddNoiseMode);
extern void UpdateFragQIndex(POSTPROC_INSTANCE *ppi);
extern void PlaneAddNoise_C(UINT8 *Start, UINT32 Width, UINT32 Height, INT32 Pitch, int q);
extern void PostProcess(POSTPROC_INSTANCE *ppi, INT32 Vp3VersionNo, INT32 FrameType, INT32 PostProcessingLevel,
                        INT32 FrameQIndex, UINT8 *LastFrameRecon, UINT8 *PostProcessBuffer, UINT8 *FragInfo,
                        UINT32 FragInfoElementSize, UINT32 FragInfoCodedMask);

/* deblock.c */
extern UINT32 DeblockLimitValuesVp4[64];
extern UINT32 DeblockLimitValuesVp5[64];
extern UINT32 DeblockLimitValuesVp6[64];
extern UINT32 *DCQuantScaleV2;
extern UINT32 *DCQuantScaleUV;
extern UINT32 *DCQuantScaleV1;
extern UINT32 *DeblockLimitValuesV2;
extern INT32 *SetupDeblockValueArray_Generic(POSTPROC_INSTANCE *pbi, INT32 FLimit);
extern void DeblockLoopFilteredBand_C(POSTPROC_INSTANCE *pbi, UINT8 *SrcPtr, UINT8 *DesPtr, UINT32 PlaneLineStep,
                                      UINT32 FragsAcross, UINT32 StartFrag, UINT32 *QuantScale);
extern void DeblockNonFilteredBand_C(POSTPROC_INSTANCE *pbi, UINT8 *SrcPtr, UINT8 *DesPtr, UINT32 PlaneLineStep,
                                     UINT32 FragsAcross, UINT32 StartFrag, UINT32 *QuantScale);
extern void DeblockNonFilteredBandNewFilter_C(POSTPROC_INSTANCE *pbi, UINT8 *SrcPtr, UINT8 *DesPtr,
                                              UINT32 PlaneLineStep, UINT32 FragsAcross, UINT32 StartFrag,
                                              UINT32 *QuantScale);
extern void DeblockFrame(POSTPROC_INSTANCE *pbi, UINT8 *SourceBuffer, UINT8 *DestinationBuffer);
extern void DeblockFrameInterlaced(POSTPROC_INSTANCE *pbi, UINT8 *SourceBuffer, UINT8 *DestinationBuffer);

/* dering.c */
extern UINT32 DeringModifierV1[64];
extern void DeringBlockStrong_C(const POSTPROC_INSTANCE *pbi, const UINT8 *SrcPtr, UINT8 *DstPtr, const INT32 Pitch,
                                UINT32 FragQIndex, UINT32 *QuantScale);
extern void DeringBlockWeak_C(const POSTPROC_INSTANCE *pbi, const UINT8 *SrcPtr, UINT8 *DstPtr, const INT32 Pitch,
                              UINT32 FragQIndex, UINT32 *QuantScale);
extern void DeringFrame(POSTPROC_INSTANCE *pbi, UINT8 *Src, UINT8 *Dst);
extern void DeringFrameInterlaced(POSTPROC_INSTANCE *pbi, UINT8 *Src, UINT8 *Dst);

/* loopfilter.c */
extern UINT32 LoopFilterLimitValuesV1[64];
extern UINT32 LoopFilterLimitValuesVp4[64];
extern UINT32 LoopFilterLimitValuesVp5[64];
extern UINT32 LoopFilterLimitValuesVp6[64];
extern UINT32 *LoopFilterLimitValuesV2;
extern INT32 *SetupBoundingValueArray_Generic(POSTPROC_INSTANCE *ppi, INT32 FLimit);
extern void FilterHoriz_Generic(POSTPROC_INSTANCE *ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void FilterVert_Generic(POSTPROC_INSTANCE *ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void FilteringHoriz_8_C(UINT32 QValue, UINT8 *Src, INT32 Pitch);
extern void FilteringVert_8_C(UINT32 QValue, UINT8 *Src, INT32 Pitch);
extern void FilteringHoriz_12_C(UINT32 QValue, UINT8 *Src, INT32 Pitch);
extern void FilteringVert_12_C(UINT32 QValue, UINT8 *Src, INT32 Pitch);

/* simpledeblocker.c */
extern void FilterHoriz_Simple_C(POSTPROC_INSTANCE *ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void FilterVert_Simple_C(POSTPROC_INSTANCE *ppi, UINT8 *PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void SimpleDeblockFrame(POSTPROC_INSTANCE *ppi, UINT8 *SrcBuffer, UINT8 *DestBuffer);

/* scale.c */
extern void HorizontalLine_4_5_Scale_C(const unsigned char *source, unsigned int sourceWidth, unsigned char *dest,
                                       unsigned int destWidth);
extern void VerticalBand_4_5_Scale_C(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void LastVerticalBand_4_5_Scale_C(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void HorizontalLine_3_5_Scale_C(const unsigned char *source, unsigned int sourceWidth, unsigned char *dest,
                                       unsigned int destWidth);
extern void VerticalBand_3_5_Scale_C(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void LastVerticalBand_3_5_Scale_C(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void HorizontalLine_1_2_Scale_C(const unsigned char *source, unsigned int sourceWidth, unsigned char *dest,
                                       unsigned int destWidth);
extern void VerticalBand_1_2_Scale_C(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void LastVerticalBand_1_2_Scale_C(unsigned char *dest, unsigned int destPitch, unsigned int destWidth);
extern void ScaleOrCenter(POSTPROC_INSTANCE *ppi, UINT8 *FrameBuffer, YUV_BUFFER_CONFIG *YuvConfig);

/* borders.c */
extern void UpdateUMVBorder(POSTPROC_INSTANCE *pbi, UINT8 *DestReconPtr);
extern void CopyFrame(POSTPROC_INSTANCE *pbi, YUV_BUFFER_CONFIG *b, UINT8 *DestReconPtr);

/* clamp.c y DeInterlace.c */
extern void ClampLevels_C(POSTPROC_INSTANCE *pbi, INT32 BlackClamp, INT32 WhiteClamp, UINT8 *Src, UINT8 *Dst);
extern void CFastDeInterlace(UINT8 *SrcPtr, UINT8 *DstPtr, INT32 Width, INT32 Height, INT32 Stride);

#endif
