#ifndef VPUTIL_H
#define VPUTIL_H

#include "codec_common.h"

/* vputil.c: rutinas de bloque de 8x8 y los punteros que cada plataforma
   rellena en UtilMachineSpecificConfig (uoptsystemdependant.c). vputil.c no
   incluye esta cabecera: en los dos binarios su FData va en .bss DELANTE de
   idct e idctc, y con estas declaraciones antes GCC los emitiria detras. */
extern void (*ReconIntra)(INT16 *TmpDataBuffer, UINT8 *ReconPtr, UINT16 *ChangePtr, UINT32 LineStep);
extern void (*ReconInter)(INT16 *TmpDataBuffer, UINT8 *ReconPtr, UINT8 *RefPtr, INT16 *ChangePtr, UINT32 LineStep);
extern void (*ReconInterHalfPixel2)(INT16 *TmpDataBuffer, UINT8 *ReconPtr, UINT8 *RefPtr1, UINT8 *RefPtr2,
                                    INT16 *ChangePtr, UINT32 LineStep);
extern void (*idct[65])(INT16 *InputData, INT16 *QuantMatrix, INT16 *OutputData);
extern void (*ClearSysState)(void);
extern void (*ReconBlock)(INT16 *SrcBlock, INT16 *ReconRefPtr, UINT8 *DestBlock, UINT32 LineStep);
extern void (*SubtractBlock)(UINT8 *SrcBlock, INT16 *DestPtr, UINT32 LineStep);
extern void (*UnpackBlock)(UINT8 *ReconPtr, INT16 *ReconRefPtr, UINT32 ReconPixelsPerLine);
extern void (*AverageBlock)(UINT8 *ReconPtr1, UINT8 *ReconPtr2, UINT16 *ReconRefPtr, UINT32 ReconPixelsPerLine);
extern void (*CopyBlock)(UINT8 *src, UINT8 *dest, UINT32 srcstride);
extern void (*Copy12x12)(const UINT8 *src, UINT8 *dest, UINT32 srcstride, UINT32 deststride);
extern void (*idctc[65])(INT16 *InputData, INT16 *QuantMatrix, INT16 *OutputData);
extern void (*FilterBlockBil_8)(UINT8 *ReconPtr1, UINT8 *ReconPtr2, UINT8 *ReconRefPtr, UINT32 PixelsPerLine,
                                INT32 ModX, INT32 ModY);
extern void (*FilterBlock)(UINT8 *ReconPtr1, UINT8 *ReconPtr2, UINT16 *ReconRefPtr, UINT32 PixelsPerLine,
                           INT32 ModX, INT32 ModY, BOOL UseBicubic);

extern void ClearSysState_C(void);
extern void AverageBlock_C(UINT8 *ReconPtr1, UINT8 *ReconPtr2, UINT16 *ReconRefPtr, UINT32 ReconPixelsPerLine);
extern void SubtractBlock_C(UINT8 *SrcBlock, INT16 *DestPtr, UINT32 LineStep);
extern void CopyBlock_C(UINT8 *src, UINT8 *dest, UINT32 srcstride);
extern void Copy12x12_C(const UINT8 *src, UINT8 *dest, UINT32 srcstride, UINT32 deststride);
extern void InitVPUtil(void);
extern void FilterBlock1d(UINT8 *SrcPtr, UINT16 *OutputPtr, UINT32 SrcPixelsPerLine, UINT32 PixelStep,
                          UINT32 OutputHeight, UINT32 OutputWidth, INT32 *Filter);
extern void FilterBlock2dFirstPass(UINT8 *SrcPtr, INT32 *OutputPtr, UINT32 SrcPixelsPerLine, UINT32 PixelStep,
                                   UINT32 OutputHeight, UINT32 OutputWidth, INT32 *Filter);
extern void FilterBlock2dSecondPass(INT32 *SrcPtr, UINT16 *OutputPtr, UINT32 SrcPixelsPerLine, UINT32 PixelStep,
                                    UINT32 OutputHeight, UINT32 OutputWidth, INT32 *Filter);
extern void FilterBlock2d(UINT8 *SrcPtr, UINT16 *OutputPtr, UINT32 SrcPixelsPerLine, INT32 *HFilter,
                          INT32 *VFilter);
extern void FilterBlock2dBil_FirstPass(UINT8 *SrcPtr, INT32 *OutputPtr, UINT32 SrcPixelsPerLine, UINT32 PixelStep,
                                       UINT32 OutputHeight, UINT32 OutputWidth, INT32 *Filter);
extern void FilterBlock1dBil_8(UINT8 *SrcPtr, UINT8 *OutputPtr, UINT32 SrcPixelsPerLine, UINT32 PixelStep,
                               UINT32 OutputHeight, UINT32 OutputWidth, INT32 *Filter);
extern void FilterBlock2dBil_SecondPass_8(INT32 *SrcPtr, UINT8 *OutputPtr, UINT32 SrcPixelsPerLine,
                                          UINT32 PixelStep, UINT32 OutputHeight, UINT32 OutputWidth,
                                          INT32 *Filter);
extern void FilterBlock2dBil_8(UINT8 *SrcPtr, UINT8 *OutputPtr, UINT32 SrcPixelsPerLine, INT32 *HFilter,
                               INT32 *VFilter);
extern void FilterBlockBil_8_C(UINT8 *ReconPtr1, UINT8 *ReconPtr2, UINT8 *ReconRefPtr, UINT32 PixelsPerLine,
                               INT32 ModX, INT32 ModY);

/* reconstruct.c */
extern void ScalarReconInterHalfPixel2(INT16 *TmpDataBuffer, UINT8 *ReconPtr, UINT8 *RefPtr1, UINT8 *RefPtr2,
                                       INT16 *ChangePtr, UINT32 LineStep);

/* Las versiones de cada plataforma (criticalpath.c e idctpart.c) que pone UtilMachineSpecificConfig */
#ifdef EA_PLATFORM_PLAYSTATION2
extern void IDct1_PS2(INT16 *InputData, INT16 *QuantMatrix, INT16 *OutputData);
extern void IDct10_PS2(INT16 *InputData, INT16 *QuantMatrix, INT16 *OutputData);
extern void IDct64_PS2(INT16 *InputData, INT16 *QuantMatrix, INT16 *OutputData);
extern void ScalarReconIntra_PS2(INT16 *TmpDataBuffer, UINT8 *ReconPtr, UINT16 *ChangePtr, UINT32 LineStep);
extern void ScalarReconInter_PS2(INT16 *TmpDataBuffer, UINT8 *ReconPtr, UINT8 *RefPtr, INT16 *ChangePtr,
                                 UINT32 LineStep);
extern void UnpackBlock_PS2(UINT8 *ReconPtr, INT16 *ReconRefPtr, UINT32 ReconPixelsPerLine);
extern void ReconBlock_PS2(INT16 *SrcBlock, INT16 *ReconRefPtr, UINT8 *DestBlock, UINT32 LineStep);
extern void FilterBlock_PS2(UINT8 *ReconPtr1, UINT8 *ReconPtr2, UINT16 *ReconRefPtr, UINT32 PixelsPerLine,
                            INT32 ModX, INT32 ModY, BOOL UseBicubic);
#else
extern void IDct1_GC(INT16 *InputData, INT16 *QuantMatrix, INT16 *OutputData);
extern void IDct10(INT16 *InputData, INT16 *QuantMatrix, INT16 *OutputData);
extern void IDct64_GC(INT16 *InputData, INT16 *QuantMatrix, INT16 *OutputData);
extern void ScalarReconIntra_GC(INT16 *TmpDataBuffer, UINT8 *ReconPtr, UINT16 *ChangePtr, UINT32 LineStep);
extern void ScalarReconInter_GC(INT16 *TmpDataBuffer, UINT8 *ReconPtr, UINT8 *RefPtr, INT16 *ChangePtr,
                                UINT32 LineStep);
extern void UnpackBlock_GC(UINT8 *ReconPtr, INT16 *ReconRefPtr, UINT32 ReconPixelsPerLine);
extern void ReconBlock_GC(INT16 *SrcBlock, INT16 *ReconRefPtr, UINT8 *DestBlock, UINT32 LineStep);
extern void FilterBlock_GC(UINT8 *ReconPtr1, UINT8 *ReconPtr2, UINT16 *ReconRefPtr, UINT32 PixelsPerLine,
                           INT32 ModX, INT32 ModY, BOOL UseBicubic);
#endif

#endif
