#ifndef PBDLL_H
#define PBDLL_H

#include "codec_common.h"
#include "boolhuff.h"
#include "huffman.h"

typedef struct LineEq {
    INT32 M;
    INT32 C;
} LINE_EQ;

/* TokenEntropy.c */
extern const UINT8 VP6_DcUpdateProbs[2][11];
extern const UINT8 VP6_AcUpdateProbs[3][2][6][11];
extern const UINT8 ScanBandUpdateProbs[64];
extern const UINT8 ZrlUpdateProbs[2][14];
extern const UINT8 ZeroRunProbDefaults[2][14];

#include "codec_common_interface.h"

typedef struct PB_INSTANCE *xPB_INST;

typedef enum {
    PBC_SET_POSTPROC,
    PBC_SET_CPUFREE,
    PBC_MAX_PARAM,
    PBC_SET_TESTMODE,
    PBC_SET_PBSTRUCT,
    PBC_SET_BLACKCLAMP,
    PBC_SET_WHITECLAMP,
    PBC_SET_REFERENCEFRAME,
    PBC_SET_DEINTERLACEMODE,
    PBC_SET_ADDNOISE
} PB_COMMAND_TYPE;

/* El descodificador solo ve el postproceso a traves de este puntero opaco. */
typedef struct POSTPROC_INSTANCE *POSTPROC_INST;

#include "quantize.h"

typedef struct RAW_BUFFER {
    UINT32 pos;
    INT32 byte_bit_offset;
    UINT32 DataBlock;
    UINT8 *Buffer;
} RAW_BUFFER;

typedef enum {
    CODE_INTER_NO_MV = 0,
    CODE_INTRA = 1,
    CODE_INTER_PLUS_MV = 2,
    CODE_INTER_NEAREST_MV = 3,
    CODE_INTER_NEAR_MV = 4,
    CODE_USING_GOLDEN = 5,
    CODE_GOLDEN_MV = 6,
    CODE_INTER_FOURMV = 7,
    CODE_GOLD_NEAREST_MV = 8,
    CODE_GOLD_NEAR_MV = 9,
    DO_NOT_CODE = 16
} CODING_MODE;

typedef struct {
    unsigned int FragCodingMode : 4;
    int MVectorX : 8;
    int MVectorY : 8;
} FRAG_INFO;

typedef struct _DCINFO {
    Q_LIST_ENTRY dc;
    short frame;
} DCINFO;

typedef enum {
    TOP_LEFT_Y_BLOCK = 0,
    TOP_RIGHT_Y_BLOCK = 1,
    BOTTOM_LEFT_Y_BLOCK = 2,
    BOTTOM_RIGHT_Y_BLOCK = 3,
    U_BLOCK = 4,
    V_BLOCK = 5
} BLOCK_POSITION;

typedef struct {
    UINT8 Token;
    CODING_MODE Mode;
    UINT16 Frame;
    Q_LIST_ENTRY Dc;
    UINT8 unused[3];
} BLOCK_CONTEXT;

typedef struct {
    BLOCK_CONTEXT LeftY[2];
    BLOCK_CONTEXT LeftU;
    BLOCK_CONTEXT LeftV;
    BLOCK_CONTEXT *AboveY;
    BLOCK_CONTEXT *AboveU;
    BLOCK_CONTEXT *AboveV;
    BLOCK_CONTEXT *AboveYAlloc;
    BLOCK_CONTEXT *AboveUAlloc;
    BLOCK_CONTEXT *AboveVAlloc;
    Q_LIST_ENTRY LastDcY[4];
    Q_LIST_ENTRY LastDcU[4];
    Q_LIST_ENTRY LastDcV[4];
} FRAME_CONTEXT;

typedef UINT8 TOKENBUFFER[256];

typedef struct {
    Q_LIST_ENTRY (*CoeffsAlloc)[64];
    Q_LIST_ENTRY (*Coeffs)[64];
    CODING_MODE Mode;
    CODING_MODE BlockMode[6];
    MOTION_VECTOR Mv[6];
    MOTION_VECTOR NearestInterMVect;
    MOTION_VECTOR NearInterMVect;
    INT32 NearestMvIndex;
    MOTION_VECTOR NearestGoldMVect;
    MOTION_VECTOR NearGoldMVect;
    INT32 NearestGMvIndex;
    UINT32 MBrow;
    UINT32 MBcol;
    BLOCK_POSITION bp;
    UINT32 Source;
    UINT32 SourceY;
    UINT32 SourceX;
    INT32 CurrentSourceStride;
    UINT32 Recon;
    INT32 CurrentReconStride;
    UINT32 Plane;
    INT32 MvShift;
    INT32 MvModMask;
    INT32 FrameSourceStride;
    INT32 FrameReconStride;
    UINT32 SourcePtr[6];
    UINT32 ReconPtr[6];
    BLOCK_CONTEXT *Above;
    BLOCK_CONTEXT *Left;
    Q_LIST_ENTRY *LastDc;
    INT32 Interlaced;
} MACROBLOCK_INFO;

typedef struct FRAME_HEADER {
    UINT8 *buffer;
    UINT32 value;
    INT32 bits_available;
    UINT32 pos;
} FRAME_HEADER;

typedef struct _BITREADER {
    int bitsinremainder;
    UINT32 remainder;
    unsigned char *position;
} BITREADER;

typedef struct PB_INSTANCE {
    MACROBLOCK_INFO mbi;
    FRAME_CONTEXT fc;
    QUANTIZER *quantizer;
    INT32 CodedBlockIndex;
    UINT8 *DataOutputInPtr;
    FRAG_INFO *FragInfo;
    FRAG_INFO *FragInfoAlloc;
    BOOL_CODER br;
    BOOL_CODER br2;
    BITREADER br3;
    UINT8 Vp3VersionNo;
    UINT8 VpProfile;
    UINT32 PostProcessingLevel;
    UINT32 ProcessorFrequency;
    UINT32 CPUFree;
    UINT8 FrameType;
    CONFIG_TYPE Configuration;
    UINT32 CurrentFrameSize;
    UINT32 YPlaneSize;
    UINT32 UVPlaneSize;
    UINT32 VFragments;
    UINT32 HFragments;
    UINT32 UnitFragments;
    UINT32 YPlaneFragments;
    UINT32 UVPlaneFragments;
    UINT32 ReconYPlaneSize;
    UINT32 ReconUVPlaneSize;
    UINT32 YDataOffset;
    UINT32 UDataOffset;
    UINT32 VDataOffset;
    UINT32 ReconYDataOffset;
    UINT32 ReconUDataOffset;
    UINT32 ReconVDataOffset;
    UINT32 MacroBlocks;
    UINT32 MBRows;
    UINT32 MBCols;
    UINT32 ScaleWidth;
    UINT32 ScaleHeight;
    UINT32 OutputWidth;
    UINT32 OutputHeight;
    YUV_BUFFER_ENTRY *ThisFrameRecon;
    YUV_BUFFER_ENTRY *ThisFrameReconAlloc;
    YUV_BUFFER_ENTRY *GoldenFrame;
    YUV_BUFFER_ENTRY *GoldenFrameAlloc;
    YUV_BUFFER_ENTRY *LastFrameRecon;
    YUV_BUFFER_ENTRY *LastFrameReconAlloc;
    YUV_BUFFER_ENTRY *PostProcessBuffer;
    YUV_BUFFER_ENTRY *PostProcessBufferAlloc;
    YUV_BUFFER_ENTRY *ScaleBuffer;
    YUV_BUFFER_ENTRY *ScaleBufferAlloc;
#if VP6_VERSION >= 106
    YUV_BUFFER_ENTRY *OtherFrameRecon; /* la 1.0.3 (PS2) no lo tiene */
#endif
    Q_LIST_ENTRY *quantized_list;
    INT16 *ReconDataBuffer;
    INT16 *ReconDataBufferAlloc;
    UINT8 FragCoefEOB;
    INT16 *TmpReconBuffer;
    INT16 *TmpReconBufferAlloc;
    INT16 *TmpDataBuffer;
    INT16 *TmpDataBufferAlloc;
    UINT8 *LoopFilteredBlockAlloc;
    UINT8 *LoopFilteredBlock;
    void (**idct)(INT16 *InputData, INT16 *QuantMatrix, INT16 *OutputData);
    POSTPROC_INST postproc;
    TOKENBUFFER LastToken;
    CODING_MODE LastMode;
    UINT8 DcProbs[22];
    UINT8 AcProbs[396];
    UINT8 DcNodeContexts[2][3][5];
    UINT8 ZeroRunProbs[2][14];
    UINT8 MergedScanOrder[64];
    UINT8 ModifiedScanOrder[64];
    UINT8 EobOffsetTable[64];
    UINT8 ScanBands[64];
    UINT8 MBModeProb[11];
    UINT8 BModeProb[11];
    UINT8 PredictionFilterMode;
    UINT8 PredictionFilterMvSizeThresh;
    UINT32 PredictionFilterVarThresh;
    BOOL RefreshGoldenFrame;
    UINT8 Inter00Prob;
    UINT32 AvgFrameQIndex;
    BOOL testMode;
    UINT32 mvNearOffset[16];
    int probInterlaced;
    char *MBInterlaced;
    char *predictionMode;
    MOTION_VECTOR *MBMotionVector;
    char *MBInterlacedAlloc;
    char *predictionModeAlloc;
    MOTION_VECTOR *MBMotionVectorAlloc;
    UINT8 MvSignProbs[2];
    UINT8 IsMvShortProb[2];
    UINT8 MvShortProbs[2][7];
    UINT8 MvQPelProbs[2];
    UINT8 MvHalfPixelProbs[2];
    UINT8 MvLowBitProbs[2];
    UINT8 MvSizeProbs[2][8];
    UINT8 probXmitted[4][2][10];
    UINT8 probModeSame[4][10];
    UINT8 probMode[4][10][9];
    UINT32 maxTimePerFrame;
    UINT32 thisDecodeTime;
    UINT32 avgDecodeTime;
    UINT32 avgPPTime[10];
    UINT32 avgBlitTime;
    BOOL MultiStream;
    UINT32 DcHuffCode[2][12];
    UINT8 DcHuffLength[2][12];
    UINT32 DcHuffProbs[2][12];
    HUFF_NODE DcHuffTree[2][12];
    UINT32 AcHuffCode[3][2][6][12];
    UINT8 AcHuffLength[3][2][6][12];
    UINT32 AcHuffProbs[3][2][6][12];
    HUFF_NODE AcHuffTree[3][2][6][12];
    UINT32 ZeroHuffCode[2][14];
    UINT8 ZeroHuffLength[2][14];
    UINT32 ZeroHuffProbs[2][14];
    HUFF_NODE ZeroHuffTree[2][14];
    UINT16 DcHuffLUT[2][64];
    UINT16 AcHuffLUT[3][2][6][64];
    UINT16 ZeroHuffLUT[2][64];
    RAW_BUFFER HuffBuffer;
    FRAME_HEADER Header;
    UINT32 Buff2Offset;
    BOOL UseHuffman;
    INT32 CurrentDcRunLen[2];
    INT32 CurrentAc1RunLen[2];
    UINT8 UseLoopFilter;
    UINT32 DrCutOff;
    UINT32 DrThresh[256];
    UINT32 BlackClamp;
    UINT32 WhiteClamp;
    UINT32 DeInterlaceMode;
    UINT32 AddNoiseMode;
    char *ScratchpadStart;
    char *ScratchpadEnd;
} PB_INSTANCE;

/* criticalpath.c de cada plataforma y decodembs.c */
extern const int VP6_Mode2Frame[16];
extern const INT32 VP6_CoeffToBand[65];
extern const UINT8 DefaultNonInterlacedScanBands[64];
extern const UINT8 DefaultInterlacedScanBands[64];

/* Tablas de modo de decodemode.c, decodembs.c y criticalpath.c */
typedef struct _modeContext {
    UINT8 left;
    UINT8 above;
    UINT8 last;
} MODE_CONTEXT;

typedef struct _htorp {
#ifdef __BIG_ENDIAN__
    unsigned char value : 7;
    unsigned char selector : 1;
#else
    unsigned char selector : 1;
    unsigned char value : 7;
#endif
} torp;

typedef struct _hnode {
    torp left;
    torp right;
} HNODE;

typedef enum _MODETYPE {
    MACROBLOCK,
    NONEAREST_MACROBLOCK,
    NONEAR_MACROBLOCK,
    BLOCK
} MODETYPE;

/* decodemode.c y decodemv.c */
extern const UINT8 VP6_BaselineXmittedProbs[4][2][10];
extern const UINT8 DefaultMvShortProbs[2][7];
extern const UINT8 DefaultMvLongProbs[2][8];
extern const UINT8 DefaultIsShortProbs[2];
extern const UINT8 DefaultSignProbs[2];

/* postproc.c, borders.c, scale.c, clamp: la parte que usa el descodificador */
extern POSTPROC_INST CreatePostProcInstance(CONFIG_TYPE *ConfigurationInit);
extern void DeletePostProcInstance(POSTPROC_INST *ppi);
extern void InitPostProcessing(UINT32 *DCQuantScaleV2p, UINT32 *DCQuantScaleUVp, UINT32 *DCQuantScaleV1p,
                               UINT32 Version);
extern INT32 AllocatePostProcBuffers(POSTPROC_INST ppi);
extern void ChangePostProcConfiguration(POSTPROC_INST ppi, CONFIG_TYPE *ConfigurationInit);
extern void SetPPInterlacedMode(POSTPROC_INST ppi, int Interlaced);
extern void SetDeInterlaceMode(POSTPROC_INST ppi, int DeInterlaceMode);
extern void SetAddNoiseMode(POSTPROC_INST ppi, int AddNoiseMode);
extern void PostProcess(POSTPROC_INST ppi, INT32 Vp3VersionNo, INT32 FrameType, INT32 PostProcessingLevel,
                        INT32 FrameQIndex, UINT8 *LastFrameRecon, UINT8 *PostProcessBuffer, UINT8 *FragInfo,
                        UINT32 FragInfoElementSize, UINT32 FragInfoCodedMask);
extern void UpdateUMVBorder(POSTPROC_INST pbi, UINT8 *DestReconPtr);
extern void CopyFrame(POSTPROC_INST pbi, YUV_BUFFER_CONFIG *b, UINT8 *DestReconPtr);
extern void ScaleOrCenter(POSTPROC_INST ppi, UINT8 *FrameBuffer, YUV_BUFFER_CONFIG *YuvConfig);
extern void (*ClampLevels)(POSTPROC_INST pbi, INT32 BlackClamp, INT32 WhiteClamp, UINT8 *Src, UINT8 *Dst);
extern void (*FilteringHoriz_12)(UINT32 QValue, UINT8 *Src, INT32 Pitch);
extern void (*FilteringVert_12)(UINT32 QValue, UINT8 *Src, INT32 Pitch);

/* pb_globals.c */
extern BOOL VP6_ModeUsesMC[10];
extern UINT32 CPUFrequency;
extern void VP6_DeleteTmpBuffers(PB_INSTANCE *pbi);
extern BOOL VP6_AllocateTmpBuffers(PB_INSTANCE *pbi);
extern void VP6_DeletePBInstance(PB_INSTANCE **pbi);
extern PB_INSTANCE *VP6_CreatePBInstance(void);
extern void VP6_VPInitLibrary(void);
extern void VP6_VPDeInitLibrary(void);

/* vfwpbdll_if.c */
extern BOOL VP6_StartDecoder(PB_INSTANCE **pbi, UINT32 ImageWidth, UINT32 ImageHeight);
extern void VP6_SetPbParam(PB_INSTANCE *pbi, PB_COMMAND_TYPE Command, UINT32 Parameter);
extern void VP6_GetYUVConfig(PB_INSTANCE *pbi, YUV_BUFFER_CONFIG *YuvConfig);
extern int VP6_DecodeFrameToYUV(PB_INSTANCE *pbi, char *VideoBufferPtr, unsigned int ByteCount, UINT32 ImageWidth,
                                UINT32 ImageHeight);
extern BOOL VP6_StopDecoder(PB_INSTANCE **pbi);

/* DFrameR.c */
extern void InitHeaderBuffer(FRAME_HEADER *Header, unsigned char *Buffer);
extern UINT32 ReadHeaderBits(FRAME_HEADER *Header, UINT32 BitsRequired);
extern BOOL VP6_LoadFrame(PB_INSTANCE *pbi);
extern UINT32 VP6_bitread(BOOL_CODER *br, int bits);

/* FrameIni.c */
extern void VP6_DeleteFragmentInfo(PB_INSTANCE *pbi);
extern BOOL VP6_AllocateFragmentInfo(PB_INSTANCE *pbi);
extern void VP6_DeleteFrameInfo(PB_INSTANCE *pbi);
extern BOOL VP6_AllocateFrameInfo(PB_INSTANCE *pbi, unsigned int FrameSize);
extern BOOL VP6_InitFrameDetails(PB_INSTANCE *pbi);
extern void VP6_InitialiseConfiguration(PB_INSTANCE *pbi);

/* decodembs.c, decodemode.c, decodemv.c, TokenEntropy.c */
extern void BuildScanOrder(PB_INSTANCE *pbi, UINT8 *ScanBands);
extern void VP6_ConfigureEntropyDecoder(PB_INSTANCE *pbi, UINT8 FrameType);
extern void VP6_ResetLeftContext(PB_INSTANCE *pbi);
extern void VP6_ResetAboveContext(PB_INSTANCE *pbi);
extern UINT8 VP6_ReadTokensPredictA(PB_INSTANCE *pbi, INT16 *CoeffData, UINT32 Plane, BLOCK_CONTEXT *Above,
                                    BLOCK_CONTEXT *Left);
extern void VP6_DecodeFrameMbs(PB_INSTANCE *pbi);
extern void VP6_BuildModeTree(PB_INSTANCE *pbi);
extern int VP6_decodeModeDiff(PB_INSTANCE *pbi);
extern void VP6_DecodeModeProbs(PB_INSTANCE *pbi);
extern void VP6_ConfigureMvEntropyDecoder(PB_INSTANCE *pbi, UINT8 FrameType);
extern void VP6_ConfigureContexts(PB_INSTANCE *pbi);

/* recon.c */
extern UINT32 Var16Point(UINT8 *DataPtr, INT32 SourceStride);
extern void VP6_PredictFiltered(PB_INSTANCE *pbi, UINT8 *SrcPtr, INT32 mx, INT32 my);

/* criticalpath.c de cada plataforma */
extern void VP6_PredictDC(PB_INSTANCE *pbi, BLOCK_POSITION bp, Q_LIST_ENTRY *LastDC, BLOCK_CONTEXT *Above,
                          BLOCK_CONTEXT *Left);
extern UINT8 ReadHuffTokensPredictA(PB_INSTANCE *pbi, INT16 *CoeffData, UINT32 Plane);
extern void VP6_DecodeBlock(PB_INSTANCE *pbi, UINT32 MBrow, UINT32 MBcol, BLOCK_POSITION bp);
extern void VP6_DecodeMacroBlock(PB_INSTANCE *pbi, UINT32 MBrow, UINT32 MBcol);
extern CODING_MODE VP6_DecodeBlockMode(PB_INSTANCE *pbi);
extern CODING_MODE VP6_DecodeMode(PB_INSTANCE *pbi, CODING_MODE lastmode, UINT32 type);
extern void VP6_decodeModeAndMotionVector(PB_INSTANCE *pbi, UINT32 MBrow, UINT32 MBcol);
extern void VP6_decodeMotionVector(PB_INSTANCE *pbi, MOTION_VECTOR *mv, CODING_MODE Mode);
extern void VP6_FindNearestandNextNearest(PB_INSTANCE *pbi, UINT32 MBrow, UINT32 MBcol, UINT8 Frame, int *type);
extern void VP6_PredictFilteredBlock(PB_INSTANCE *pbi, INT16 *OutputPtr, BLOCK_POSITION bp);
extern void VP6_ReconstructBlock(PB_INSTANCE *pbi, BLOCK_POSITION bp);

#endif
