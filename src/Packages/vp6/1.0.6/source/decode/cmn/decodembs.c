#include <string.h>

#include "pbdll.h"

typedef struct {
    UINT16 MinVal;
    INT16 Length;
    UINT8 Probs[11];
} TOKENEXTRABITS;

static const TOKENEXTRABITS VP6_TokenExtraBits2[12] = {
    {0, -1, {0}},
    {1, 0, {0}},
    {2, 0, {0}},
    {3, 0, {0}},
    {4, 0, {0}},
    {5, 0, {0x9F}},
    {7, 1, {0x91, 0xA5}},
    {11, 2, {0x8C, 0x94, 0xAD}},
    {19, 3, {0x87, 0x8C, 0x9B, 0xB0}},
    {35, 4, {0x82, 0x86, 0x8D, 0x9D, 0xB4}},
    {67, 10, {0x81, 0x82, 0x85, 0x8C, 0x99, 0xB1, 0xC4, 0xE6, 0xF3, 0xFE, 0xFE}},
    {0, -1, {0}},
};

const INT32 VP6_CoeffToBand[65] = {
    -1, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    7,
};

/* La 1.0.6 (GameCube) la expande en cada llamada; la 1.0.3 (PS2) la emite aparte y la llama. */
#if VP6_VERSION >= 106
static inline int nDecodeBool128(BOOL_CODER *br)
#else
static int nDecodeBool128(BOOL_CODER *br)
#endif
{
    unsigned int bit;
    unsigned int split;
    unsigned int bigsplit;
    unsigned int count;
    unsigned int range;
    unsigned int value;

    range = br->range;
    value = br->value;
    count = br->count;
    split = (range + 1) >> 1;
    bigsplit = split << 24;

    bit = (value >= bigsplit);
    range = bit ? range - split : split;
    if (bit) {
        value = value - bigsplit;
    }

    value = value + value;
    range = range + range;
    count--;
    if (!count) {
        count = 8;
        value = value | br->buffer[br->pos];
        br->pos++;
    }

    br->count = count;
    br->value = value;
    br->range = range;
    return bit;
}

void BuildScanOrder(PB_INSTANCE *pbi, UINT8 *ScanBands) {
    UINT32 i;
    UINT32 j;
    UINT32 ScanOrderIndex;
    UINT32 MaxOffset;

    pbi->ModifiedScanOrder[0] = 0;
    ScanOrderIndex = 1;

    for (i = 0; i <= 15; i++) {
        for (j = 1; j <= 63; j++) {
            if (ScanBands[j] == i) {
                pbi->ModifiedScanOrder[ScanOrderIndex] = j;
                ScanOrderIndex++;
            }
        }
    }

    MaxOffset = 0;
    for (i = 0; i <= 63; i++) {
        if (pbi->ModifiedScanOrder[i] > MaxOffset) {
            MaxOffset = pbi->ModifiedScanOrder[i];
        }
        pbi->EobOffsetTable[i] = MaxOffset;
        if (pbi->Vp3VersionNo > 6) {
            pbi->EobOffsetTable[i] = MaxOffset + 1;
        }
    }
}

void BoolTreeToHuffCodes(UINT8 *BoolTreeProbs, UINT32 *HuffProbs) {
    UINT32 Prob;
    UINT32 Prob1;

    HuffProbs[11] = ((unsigned int)BoolTreeProbs[0] * BoolTreeProbs[1]) >> 8;
    HuffProbs[0] = ((unsigned int)BoolTreeProbs[0] * (255 - BoolTreeProbs[1])) >> 8;

    Prob = 255 - BoolTreeProbs[0];
    HuffProbs[1] = (Prob * BoolTreeProbs[2]) >> 8;

    Prob = (Prob * (255 - BoolTreeProbs[2])) >> 8;
    Prob1 = (Prob * BoolTreeProbs[3]) >> 8;
    HuffProbs[2] = (Prob1 * BoolTreeProbs[4]) >> 8;
    Prob1 = (Prob1 * (255 - BoolTreeProbs[4])) >> 8;
    HuffProbs[3] = (Prob1 * BoolTreeProbs[5]) >> 8;
    HuffProbs[4] = (Prob1 * (255 - BoolTreeProbs[5])) >> 8;

    Prob = (Prob * (255 - BoolTreeProbs[3])) >> 8;
    Prob1 = (Prob * BoolTreeProbs[6]) >> 8;
    HuffProbs[5] = (Prob1 * BoolTreeProbs[7]) >> 8;
    HuffProbs[6] = (Prob1 * (255 - BoolTreeProbs[7])) >> 8;

    Prob = (Prob * (255 - BoolTreeProbs[6])) >> 8;
    Prob1 = (Prob * BoolTreeProbs[8]) >> 8;
    HuffProbs[7] = (Prob1 * BoolTreeProbs[9]) >> 8;
    HuffProbs[8] = (Prob1 * (255 - BoolTreeProbs[9])) >> 8;

    Prob = (Prob * (255 - BoolTreeProbs[8])) >> 8;
    HuffProbs[9] = (Prob * BoolTreeProbs[10]) >> 8;
    HuffProbs[10] = (Prob * (255 - BoolTreeProbs[10])) >> 8;
}

void ZerosBoolTreeToHuffCodes(UINT8 *BoolTreeProbs, UINT32 *HuffProbs) {
    UINT32 Prob;

    Prob = ((unsigned int)BoolTreeProbs[0] * BoolTreeProbs[1]) >> 8;
    HuffProbs[0] = (Prob * BoolTreeProbs[2]) >> 8;
    HuffProbs[1] = (Prob * (255 - BoolTreeProbs[2])) >> 8;

    Prob = ((unsigned int)BoolTreeProbs[0] * (255 - BoolTreeProbs[1])) >> 8;
    HuffProbs[2] = (Prob * BoolTreeProbs[3]) >> 8;
    HuffProbs[3] = (Prob * (255 - BoolTreeProbs[3])) >> 8;

    Prob = ((255u - BoolTreeProbs[0]) * BoolTreeProbs[4]) >> 8;
    Prob = (Prob * BoolTreeProbs[5]) >> 8;
    HuffProbs[4] = (Prob * BoolTreeProbs[6]) >> 8;
    HuffProbs[5] = (Prob * (255 - BoolTreeProbs[6])) >> 8;

    Prob = ((255u - BoolTreeProbs[0]) * BoolTreeProbs[4]) >> 8;
    Prob = (Prob * (255 - BoolTreeProbs[5])) >> 8;
    HuffProbs[6] = (Prob * BoolTreeProbs[7]) >> 8;
    HuffProbs[7] = (Prob * (255 - BoolTreeProbs[7])) >> 8;

    Prob = ((255u - BoolTreeProbs[0]) * (255 - BoolTreeProbs[4])) >> 8;
    HuffProbs[8] = Prob;
}

void ConvertBoolTrees(PB_INSTANCE *pbi) {
    UINT32 i;
    UINT32 Plane;
    UINT32 Band;
    INT32 Prec;

    for (Plane = 0; Plane <= 1; Plane++) {
        BoolTreeToHuffCodes(&pbi->DcProbs[Plane * 11], pbi->DcHuffProbs[Plane]);
        VP6_BuildHuffTree(pbi->DcHuffTree[Plane], pbi->DcHuffProbs[Plane], 12);
        VP6_BuildHuffLookupTable(pbi->DcHuffTree[Plane], pbi->DcHuffLUT[Plane]);
        VP6_CreateCodeArray(pbi->DcHuffTree[Plane], 0, pbi->DcHuffCode[Plane],
                            pbi->DcHuffLength[Plane], 0, 0);
    }

    for (i = 0; i <= 1; i++) {
        ZerosBoolTreeToHuffCodes(pbi->ZeroRunProbs[i], pbi->ZeroHuffProbs[i]);
        VP6_BuildHuffTree(pbi->ZeroHuffTree[i], pbi->ZeroHuffProbs[i], 9);
        VP6_BuildHuffLookupTable(pbi->ZeroHuffTree[i], pbi->ZeroHuffLUT[i]);
        VP6_CreateCodeArray(pbi->ZeroHuffTree[i], 0, pbi->ZeroHuffCode[i], pbi->ZeroHuffLength[i],
                            0, 0);
    }

    for (Prec = 0; Prec <= 2; Prec++) {
        for (Plane = 0; Plane <= 1; Plane++) {
            for (Band = 0; Band <= 5; Band++) {
                BoolTreeToHuffCodes(&pbi->AcProbs[Plane * 198 + Prec * 66 + Band * 11],
                                    pbi->AcHuffProbs[Prec][Plane][Band]);
                VP6_BuildHuffTree(pbi->AcHuffTree[Prec][Plane][Band],
                                  pbi->AcHuffProbs[Prec][Plane][Band], 12);
                VP6_BuildHuffLookupTable(pbi->AcHuffTree[Prec][Plane][Band],
                                         pbi->AcHuffLUT[Prec][Plane][Band]);
                VP6_CreateCodeArray(pbi->AcHuffTree[Prec][Plane][Band], 0,
                                    pbi->AcHuffCode[Prec][Plane][Band],
                                    pbi->AcHuffLength[Prec][Plane][Band], 0, 0);
            }
        }
    }
}

void VP6_ConfigureEntropyDecoder(PB_INSTANCE *pbi, UINT8 FrameType) {
    UINT32 i;
    UINT32 j;
    UINT32 Plane;
    UINT32 Band;
    INT32 Prec;
    UINT8 PrecNonZero;
    UINT8 LastProb[11];

    memset(LastProb, 128, 11);

    for (Plane = 0; Plane <= 1; Plane++) {
        for (i = 0; i <= 10; i++) {
            if (nDecodeBool(&pbi->br, VP6_DcUpdateProbs[Plane][i])) {
                LastProb[i] = VP6_bitread(&pbi->br, 7) << 1;
                LastProb[i] += (LastProb[i] == 0);
                pbi->DcProbs[Plane * 11 + i] = LastProb[i];
            } else if (FrameType == 0) {
                pbi->DcProbs[Plane * 11 + i] = LastProb[i];
            }
        }
    }

    if (FrameType == 0) {
        memcpy(pbi->ZeroRunProbs, ZeroRunProbDefaults, sizeof(pbi->ZeroRunProbs));
    }

    if (nDecodeBool(&pbi->br, 128)) {
        for (i = 1; i <= 63; i++) {
            if (nDecodeBool(&pbi->br, ScanBandUpdateProbs[i])) {
                pbi->ScanBands[i] = VP6_bitread(&pbi->br, 4);
            }
        }
        BuildScanOrder(pbi, pbi->ScanBands);
    }

    for (i = 0; i <= 1; i++) {
        for (j = 0; j <= 13; j++) {
            if (nDecodeBool(&pbi->br, ZrlUpdateProbs[i][j])) {
                pbi->ZeroRunProbs[i][j] = VP6_bitread(&pbi->br, 7) << 1;
                pbi->ZeroRunProbs[i][j] += (pbi->ZeroRunProbs[i][j] == 0);
            }
        }
    }

    for (Prec = 0; Prec <= 2; Prec++) {
        for (Plane = 0; Plane <= 1; Plane++) {
            for (Band = 0; Band <= 5; Band++) {
                for (i = 0; i <= 10; i++) {
                    if (nDecodeBool(&pbi->br, VP6_AcUpdateProbs[Prec][Plane][Band][i])) {
                        LastProb[i] = VP6_bitread(&pbi->br, 7) << 1;
                        LastProb[i] += (LastProb[i] == 0);
                        pbi->AcProbs[Plane * 198 + Prec * 66 + Band * 11 + i] = LastProb[i];
                    } else if (FrameType == 0) {
                        pbi->AcProbs[Plane * 198 + Prec * 66 + Band * 11 + i] = LastProb[i];
                    }
                }
            }
        }
    }

    VP6_ConfigureContexts(pbi);
}

void VP6_ResetLeftContext(PB_INSTANCE *pbi) {
    memset(&pbi->fc.LeftY[0], 0, sizeof(BLOCK_CONTEXT));
    memset(&pbi->fc.LeftY[1], 0, sizeof(BLOCK_CONTEXT));
    memset(&pbi->fc.LeftU, 0, sizeof(BLOCK_CONTEXT));
    memset(&pbi->fc.LeftV, 0, sizeof(BLOCK_CONTEXT));

    pbi->fc.LeftY[0].Mode = -1;
    pbi->fc.LeftY[1].Mode = -1;
    pbi->fc.LeftU.Mode = -1;
    pbi->fc.LeftV.Mode = -1;
    pbi->fc.LeftY[0].Frame = 4;
    pbi->fc.LeftY[1].Frame = 4;
    pbi->fc.LeftU.Frame = 4;
    pbi->fc.LeftV.Frame = 4;
}

void VP6_ResetAboveContext(PB_INSTANCE *pbi) {
    UINT32 i;

    for (i = 0; i < pbi->HFragments + 8; i++) {
        pbi->fc.AboveY[i].Mode = -1;
        pbi->fc.AboveY[i].Frame = 4;
        pbi->fc.AboveY[i].Dc = 0;
        pbi->fc.AboveY[i].Token = 0;
    }

    for (i = 0; i < pbi->HFragments / 2 + 8; i++) {
        pbi->fc.AboveU[i].Mode = -1;
        pbi->fc.AboveU[i].Frame = 4;
        pbi->fc.AboveU[i].Token = 0;
        pbi->fc.AboveU[i].Dc = 0;
        pbi->fc.AboveV[i].Mode = -1;
        pbi->fc.AboveV[i].Frame = 4;
        pbi->fc.AboveV[i].Token = 0;
        pbi->fc.AboveV[i].Dc = 0;
    }

    /* PC (speed.exe 0x800899): el original compara contra 6, no contra 5. */
#if defined(EA_PLATFORM_WIN32)
    if (pbi->Vp3VersionNo < 6) {
#else
    if (pbi->Vp3VersionNo <= 5) {
#endif
        pbi->fc.AboveU[1].Mode = 0;
        pbi->fc.AboveU[1].Frame = 0;
        pbi->fc.AboveV[1].Mode = 0;
        pbi->fc.AboveV[1].Frame = 0;
    }

    pbi->fc.LastDcY[0] = 0;
    pbi->fc.LastDcU[0] = 128;
    pbi->fc.LastDcV[0] = 128;

    for (i = 1; i <= 2; i++) {
        pbi->fc.LastDcY[i] = 0;
        pbi->fc.LastDcU[i] = 0;
        pbi->fc.LastDcV[i] = 0;
    }
}

UINT8 VP6_ReadTokensPredictA(PB_INSTANCE *pbi, INT16 *CoeffData, UINT32 Plane, BLOCK_CONTEXT *Above,
                             BLOCK_CONTEXT *Left) {
    INT32 token;
    BOOL_CODER *br;
    UINT8 EncodedCoeffs;
    UINT8 *BaselineProbsPtr;
    UINT8 *ContextProbsPtr;
    UINT8 PrecTokenIndex;
    INT32 SignBit;
    INT32 BitsCount;
    UINT8 *AcProbsPtr;
    UINT32 *TransIndex;
    INT32 value;
    UINT32 ZeroRunCount;
    UINT8 *ZeroRunProbPtr;
    BOOL ThisTokeNonZero;

    TransIndex = pbi->quantizer->transIndex;
    AcProbsPtr = &pbi->AcProbs[Plane * 198];

    if (pbi->MultiStream || pbi->VpProfile == 0) {
        br = &pbi->br2;
    } else {
        br = &pbi->br;
    }

    BaselineProbsPtr = &pbi->DcProbs[Plane * 11];
    ContextProbsPtr = pbi->DcNodeContexts[Plane][Left->Token + Above->Token];

    if (!nDecodeBool(br, ContextProbsPtr[0])) {
        PrecTokenIndex = 0;
        Left->Token = 0;
        Above->Token = 0;
    } else {
        Left->Token = 1;
        Above->Token = 1;
        if (nDecodeBool(br, ContextProbsPtr[2])) {
            PrecTokenIndex = 2;
            if (nDecodeBool(br, ContextProbsPtr[3])) {
                if (nDecodeBool(br, BaselineProbsPtr[6])) {
                    if (nDecodeBool(br, BaselineProbsPtr[8])) {
                        token = nDecodeBool(br, BaselineProbsPtr[10]) + 9;
                    } else {
                        token = nDecodeBool(br, BaselineProbsPtr[9]) + 7;
                    }
                } else {
                    token = nDecodeBool(br, BaselineProbsPtr[7]) + 5;
                }
                value = VP6_TokenExtraBits2[token].MinVal;
                BitsCount = VP6_TokenExtraBits2[token].Length;
                do {
                    value += nDecodeBool(br, VP6_TokenExtraBits2[token].Probs[BitsCount])
                             << BitsCount;
                    BitsCount--;
                } while (BitsCount >= 0);
                SignBit = nDecodeBool128(br);
                CoeffData[0] = (value ^ -SignBit) + SignBit;
            } else {
                if (nDecodeBool(br, ContextProbsPtr[4])) {
                    token = nDecodeBool(br, BaselineProbsPtr[5]) + 3;
                } else {
                    token = 2;
                }
                SignBit = nDecodeBool128(br);
                CoeffData[0] = (token ^ -SignBit) + SignBit;
            }
        } else {
            PrecTokenIndex = 1;
            SignBit = nDecodeBool128(br);
            CoeffData[0] = (1 ^ -SignBit) + SignBit;
        }
    }

    for (EncodedCoeffs = 1; EncodedCoeffs <= 63;) {
        BaselineProbsPtr = &AcProbsPtr[PrecTokenIndex * 66 + VP6_CoeffToBand[EncodedCoeffs] * 11];

        if (EncodedCoeffs > 1 && PrecTokenIndex == 0) {
            ThisTokeNonZero = 1;
        } else {
            ThisTokeNonZero = nDecodeBool(br, BaselineProbsPtr[0]);
        }

        if (!ThisTokeNonZero) {
            if (!nDecodeBool(br, BaselineProbsPtr[1])) {
                EncodedCoeffs++;
                break;
            }

            PrecTokenIndex = 0;
            ZeroRunProbPtr = pbi->ZeroRunProbs[0];
            if (EncodedCoeffs > 5) {
                ZeroRunProbPtr = pbi->ZeroRunProbs[1];
            }

            if (!nDecodeBool(br, ZeroRunProbPtr[0])) {
                if (!nDecodeBool(br, ZeroRunProbPtr[1])) {
                    ZeroRunCount = nDecodeBool(br, ZeroRunProbPtr[2]) + 1;
                } else {
                    ZeroRunCount = nDecodeBool(br, ZeroRunProbPtr[3]) + 3;
                }
            } else if (!nDecodeBool(br, ZeroRunProbPtr[4])) {
                if (!nDecodeBool(br, ZeroRunProbPtr[5])) {
                    ZeroRunCount = nDecodeBool(br, ZeroRunProbPtr[6]) + 5;
                } else {
                    ZeroRunCount = nDecodeBool(br, ZeroRunProbPtr[7]) + 7;
                }
            } else {
                ZeroRunCount = nDecodeBool(br, ZeroRunProbPtr[8]);
                ZeroRunCount += nDecodeBool(br, ZeroRunProbPtr[9]) << 1;
                ZeroRunCount += nDecodeBool(br, ZeroRunProbPtr[10]) << 2;
                ZeroRunCount += nDecodeBool(br, ZeroRunProbPtr[11]) << 3;
                ZeroRunCount += nDecodeBool(br, ZeroRunProbPtr[12]) << 4;
                ZeroRunCount += nDecodeBool(br, ZeroRunProbPtr[13]) << 5;
                ZeroRunCount += 9;
            }

            EncodedCoeffs = EncodedCoeffs + ZeroRunCount;
        } else {
            if (nDecodeBool(br, BaselineProbsPtr[2])) {
                PrecTokenIndex = 2;
                if (nDecodeBool(br, BaselineProbsPtr[3])) {
                    if (nDecodeBool(br, BaselineProbsPtr[6])) {
                        if (nDecodeBool(br, BaselineProbsPtr[8])) {
                            token = nDecodeBool(br, BaselineProbsPtr[10]) + 9;
                        } else {
                            token = nDecodeBool(br, BaselineProbsPtr[9]) + 7;
                        }
                    } else {
                        token = nDecodeBool(br, BaselineProbsPtr[7]) + 5;
                    }
                    value = VP6_TokenExtraBits2[token].MinVal;
                    BitsCount = VP6_TokenExtraBits2[token].Length;
                    do {
                        value += nDecodeBool(br, VP6_TokenExtraBits2[token].Probs[BitsCount])
                                 << BitsCount;
                        BitsCount--;
                    } while (BitsCount >= 0);
                    SignBit = nDecodeBool128(br);
                    CoeffData[TransIndex[pbi->ModifiedScanOrder[EncodedCoeffs]]] =
                        (value ^ -SignBit) + SignBit;
                } else {
                    if (nDecodeBool(br, BaselineProbsPtr[4])) {
                        token = nDecodeBool(br, BaselineProbsPtr[5]) + 3;
                    } else {
                        token = 2;
                    }
                    SignBit = nDecodeBool128(br);
                    CoeffData[TransIndex[pbi->ModifiedScanOrder[EncodedCoeffs]]] =
                        (token ^ -SignBit) + SignBit;
                }
            } else {
                PrecTokenIndex = 1;
                SignBit = nDecodeBool128(br);
                CoeffData[TransIndex[pbi->ModifiedScanOrder[EncodedCoeffs]]] =
                    (1 ^ -SignBit) + SignBit;
            }
            EncodedCoeffs++;
        }
    }

    EncodedCoeffs--;
    return pbi->EobOffsetTable[EncodedCoeffs];
}

void VP6_DecodeFrameMbs(PB_INSTANCE *pbi) {
    UINT32 MBrow;
    UINT32 MBcol;
    UINT32 MBRows;
    UINT32 MBCols;
    INT32 i;

    MBRows = pbi->MBRows;
    MBCols = pbi->MBCols;

    if (pbi->FrameType) {
        VP6_DecodeModeProbs(pbi);
        VP6_ConfigureMvEntropyDecoder(pbi, pbi->FrameType);
        pbi->LastMode = 0;
    } else {
        memcpy(pbi->probXmitted, VP6_BaselineXmittedProbs, sizeof(pbi->probXmitted));
        memcpy(pbi->IsMvShortProb, DefaultIsShortProbs, sizeof(pbi->IsMvShortProb));
        memcpy(pbi->MvShortProbs, DefaultMvShortProbs, sizeof(pbi->MvShortProbs));
        memcpy(pbi->MvSignProbs, DefaultSignProbs, sizeof(pbi->MvSignProbs));
        memcpy(pbi->MvSizeProbs, DefaultMvLongProbs, sizeof(pbi->MvSizeProbs));

        memset(pbi->MBModeProb, 128, 11);
        memset(pbi->BModeProb, 128, 11);
        memset(pbi->predictionMode, 1, pbi->MacroBlocks);

        if (pbi->Configuration.Interlaced == 1) {
            memcpy(pbi->ScanBands, DefaultInterlacedScanBands, sizeof(pbi->ScanBands));
        } else {
            memcpy(pbi->ScanBands, DefaultNonInterlacedScanBands, sizeof(pbi->ScanBands));
        }

        BuildScanOrder(pbi, pbi->ScanBands);
    }

    VP6_ConfigureEntropyDecoder(pbi, pbi->FrameType);

    for (i = 0; i <= 63; i++) {
        pbi->MergedScanOrder[i] = pbi->quantizer->transIndex[pbi->ModifiedScanOrder[i]];
    }

    if (pbi->UseHuffman) {
        ConvertBoolTrees(pbi);
    }

    if (pbi->Configuration.Interlaced == 1) {
        pbi->probInterlaced = (UINT8)VP6_bitread(&pbi->br, 8);
    }

    VP6_ResetAboveContext(pbi);

    memset(pbi->mbi.Coeffs, 0, 768);

    pbi->CurrentDcRunLen[0] = 0;
    pbi->CurrentDcRunLen[1] = 0;
    pbi->CurrentAc1RunLen[0] = 0;
    pbi->CurrentAc1RunLen[1] = 0;

    for (MBrow = 3; MBrow < MBRows - 3; MBrow++) {
        VP6_ResetLeftContext(pbi);
        for (MBcol = 3; MBcol < MBCols - 3; MBcol++) {
            VP6_DecodeMacroBlock(pbi, MBrow, MBcol);
        }
    }
}
