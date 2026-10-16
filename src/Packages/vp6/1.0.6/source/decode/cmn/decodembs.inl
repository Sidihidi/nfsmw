/* decodembs.inl -- el .mdebug de la A124 y el DWARF de GameCube dicen que el criticalpath.c de cada plataforma lo
 * incluye (D:/env/egami/vp6/dev/source/decode/ps2/../cmn/decodembs.inl en PS2, 1.0.3; gc/../cmn en GameCube, 1.0.6). */

static const int VP6_HuffTokenMinVal[12] = {0, 1, 2, 3, 4, 5, 7, 11, 19, 35, 67, 0};

static const int VP6_CoeffToHuffBand[65] = {
    -1, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3,  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
};

const unsigned char DefaultNonInterlacedScanBands[64] = {
    0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6,
    7, 7, 7, 7, 7, 8, 8, 9, 9, 9, 9, 9, 9, 10, 10, 11, 11, 11, 11, 11,
    11, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15,
};

const unsigned char DefaultInterlacedScanBands[64] = {
    0, 1, 0, 1, 1, 2, 5, 3, 2, 2, 2, 2, 4, 7, 8, 10, 9, 7, 5, 4, 2, 3,
    5, 6, 8, 9, 11, 12, 13, 12, 11, 10, 9, 7, 5, 4, 6, 7, 9, 11, 12, 12,
    13, 13, 14, 12, 11, 9, 7, 9, 11, 12, 14, 14, 14, 15, 13, 11, 13, 15, 15, 15, 15, 15,
};

const int VP6_Mode2Frame[16] = {1, 0, 1, 1, 1, 2, 2, 1, 2, 2, 0, 0, 0, 0, 0, 0};

static const unsigned int loMaskTbl_VP60[33] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000F, 0x0000001F, 0x0000003F,
    0x0000007F, 0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF, 0x00001FFF,
    0x00003FFF, 0x00007FFF, 0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
    0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
    0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

#define VP6_LOADREMAINDER(br)                                                                      \
    br->remainder = (br->position[0] << 24) + (br->position[1] << 16) + (br->position[2] << 8) +   \
                    br->position[3];                                                               \
    br->position += 4

#if VP6_VERSION >= 106
static inline
#else
static
#endif
unsigned int bitread(struct _BITREADER *br, int bits) {
    unsigned int z = 0;

    br->remainder &= loMaskTbl_VP60[br->bitsinremainder];

    if ((bits -= br->bitsinremainder) > 0) {
        z |= br->remainder << bits;
        VP6_LOADREMAINDER(br);
        bits -= 32;
    }
    return z | (br->remainder >> (br->bitsinremainder = -bits));
}

static inline unsigned int bitreadonly(struct _BITREADER *br, unsigned int bits) {
    unsigned int x = br->bitsinremainder;
    unsigned int z = (1 << x) - 1;

    z &= br->remainder;
    if (x >= bits) {
        return z >> (x - bits);
    }
    z <<= 8;
    z |= br->position[0];
    return z >> (8 + x - bits);
}

static inline void bitShift(struct _BITREADER *br, int bits) {
    br->bitsinremainder -= bits;
    if (br->bitsinremainder < 0) {
        VP6_LOADREMAINDER(br);
        br->bitsinremainder += 32;
    }
}

#if VP6_VERSION >= 106
static inline
#else
static
#endif
unsigned int bitread1(struct _BITREADER *br) {
    if (br->bitsinremainder) {
        return (br->remainder >> --br->bitsinremainder) & 1;
    }
    VP6_LOADREMAINDER(br);
    return br->remainder >> (br->bitsinremainder = 31);
}


int nDecodeBool(BOOL_CODER *br, int probability) {
    unsigned int bit = 0;
    unsigned int split;
    unsigned int bigsplit;
    int count = br->count;
    unsigned int range = br->range;
    unsigned int value = br->value;

    split = 1 + (((range - 1) * probability) >> 8);
    bigsplit = split << 24;

    if (value >= bigsplit) {
        range = range - split;
        value = value - bigsplit;
        bit = 1;
    } else {
        range = split;
    }

    while (range < 128) {
        range += range;
        value += value;
        if (--count == 0) {
            count = 8;
            value |= br->buffer[br->pos];
            br->pos++;
        }
    }

    br->count = count;
    br->value = value;
    br->range = range;
    return bit;
}

#if VP6_VERSION >= 106
static inline
#endif
void VP6_PredictDC(struct PB_INSTANCE *pbi, BLOCK_POSITION bp, short *LastDC,
                                 BLOCK_CONTEXT *Above, BLOCK_CONTEXT *Left) {
    unsigned char Frame;
    unsigned char Count;
    int Avg;

    Frame = VP6_Mode2Frame[pbi->mbi.Mode];
    Count = 0;
    Avg = 0;

    if (Frame == Left->Frame) {
        Avg = Left->Dc;
        Count = 1;
    }

    if (Frame == Above->Frame) {
        Avg += Above->Dc;
        Count++;
    }

    if (Count == 0) {
        Avg = LastDC[Frame];
    }

    if (Count == 2) {
        Avg += (((signed short)Avg >> 15) & 1);
        Avg >>= 1;
    }

    pbi->mbi.Coeffs[bp][0] += Avg;
    LastDC[Frame] = pbi->mbi.Coeffs[bp][0];
}

static inline int VP6_ExtractTokenN(struct _BITREADER *br, struct _huffnode *hn,
                                    unsigned short *hlt) {
    struct _tokenorptr torp;
    struct HUFF_TABLE_NODE *htptr = (struct HUFF_TABLE_NODE *)hlt;
    unsigned int x = bitreadonly(br, 6);

    bitShift(br, htptr[x].length);

    if (htptr[x].flag) {
        return htptr[x].value;
    }

    torp.value = htptr[x].value;

    do {
        if (bitread1(br)) {
            torp = hn[torp.value].rightunion.right;
        } else {
            torp = hn[torp.value].leftunion.left;
        }
    } while (!torp.selector);

    return torp.value;
}

#if VP6_VERSION >= 106
static inline
#endif
unsigned char ReadHuffTokensPredictA(struct PB_INSTANCE *pbi, short *CoeffData,
                                               unsigned int Plane) {
    struct _BITREADER *br = &pbi->br3;
    int SignBit;
    unsigned int Prec;
    int EncodedCoeffs = 1;
    int token;
    int value;

    if (pbi->CurrentDcRunLen[Plane] > 0) {
        pbi->CurrentDcRunLen[Plane]--;
        Prec = 0;
    } else {
        token = VP6_ExtractTokenN(br, pbi->DcHuffTree[Plane], pbi->DcHuffLUT[Plane]);
        value = VP6_HuffTokenMinVal[token];

        if (token == 11) {
            goto Finished;
        }

        if (token == 0) {
            unsigned int val = bitread(br, 2) + 1;

            if (val == 3) {
                val = bitread(br, 2) + 3;
            } else if (val == 4) {
                if (bitread1(br)) {
                    val = bitread(br, 6) + 11;
                } else {
                    val = bitread(br, 2) + 7;
                }
            }
            pbi->CurrentDcRunLen[Plane] = val - 1;

            Prec = 0;
        } else {
            if (token <= 4) {
                SignBit = bitread1(br);
            } else if (token <= 9) {
                value += bitread(br, token - 4);
                SignBit = bitread1(br);
            } else {
                value += bitread(br, 11);
                SignBit = bitread1(br);
            }
            CoeffData[0] = (value ^ -SignBit) + SignBit;
            Prec = (value > 1) ? 2 : 1;
        }
    }

    if (pbi->CurrentAc1RunLen[Plane] > 0) {
        pbi->CurrentAc1RunLen[Plane]--;
        goto Finished;
    }

    do {
        unsigned int Band = VP6_CoeffToHuffBand[EncodedCoeffs];
        token = VP6_ExtractTokenN(br, pbi->AcHuffTree[Prec][Plane][Band],
                                  pbi->AcHuffLUT[Prec][Plane][Band]);
        value = VP6_HuffTokenMinVal[token];

        if (token == 0) {
            {
                unsigned int ZrlBand = EncodedCoeffs > 5;
                unsigned int ZrlToken = VP6_ExtractTokenN(br, pbi->ZeroHuffTree[ZrlBand],
                                                          pbi->ZeroHuffLUT[ZrlBand]);

                if (ZrlToken <= 7) {
                    EncodedCoeffs += ZrlToken;
                } else {
                    EncodedCoeffs += bitread(br, 6) + 8;
                }
            }
            Prec = 0;
            EncodedCoeffs++;
            continue;
        }

        if (token == 11) {
            if (EncodedCoeffs != 1) {
                break;
            }

            {
                unsigned int val = bitread(br, 2) + 1;

                if (val == 3) {
                    val = bitread(br, 2) + 3;
                } else if (val == 4) {
                    if (bitread1(br)) {
                        val = bitread(br, 6) + 11;
                    } else {
                        val = bitread(br, 2) + 7;
                    }
                }
                pbi->CurrentAc1RunLen[Plane] = val - 1;
            }

            break;
        }

        if (token <= 4) {
            SignBit = bitread1(br);
        } else if (token <= 9) {
            value += bitread(br, token - 4);
            SignBit = bitread1(br);
        } else {
            value += bitread(br, 11);
            SignBit = bitread1(br);
        }
        CoeffData[pbi->MergedScanOrder[EncodedCoeffs]] = (value ^ -SignBit) + SignBit;
        Prec = (value > 1) ? 2 : 1;
        EncodedCoeffs++;
    } while (EncodedCoeffs < 64);

Finished:
    return pbi->EobOffsetTable[EncodedCoeffs - 1];
}

void VP6_DecodeBlock(struct PB_INSTANCE *pbi, unsigned int MBrow, unsigned int MBcol,
                     BLOCK_POSITION bp) {
    MACROBLOCK_INFO *mbi = &pbi->mbi;
    int EOBPos;

    if (pbi->UseHuffman) {
        EOBPos = ReadHuffTokensPredictA(pbi, mbi->Coeffs[bp], mbi->Plane != 0);
    } else {
        EOBPos = VP6_ReadTokensPredictA(pbi, mbi->Coeffs[bp], mbi->Plane != 0, mbi->Above,
                                        mbi->Left);
    }

    VP6_PredictDC(pbi, bp, mbi->LastDc, mbi->Above, mbi->Left);

    mbi->Above->Mode = mbi->Left->Mode = mbi->BlockMode[bp];
    mbi->Above->Dc = mbi->Left->Dc = mbi->Coeffs[bp][0];
    mbi->Above->Frame = mbi->Left->Frame = VP6_Mode2Frame[mbi->Mode];

    if (EOBPos <= 1) {
        idct[1](mbi->Coeffs[bp], pbi->quantizer->dequant_coeffs[VP6_QTableSelect[bp]],
                pbi->ReconDataBuffer);

        mbi->Coeffs[bp][0] = 0;
    } else if (EOBPos <= 10) {
        idct[9](mbi->Coeffs[bp], pbi->quantizer->dequant_coeffs[VP6_QTableSelect[bp]],
                pbi->ReconDataBuffer);

#ifdef EA_PLATFORM_PLAYSTATION2
        /* Los diez primeros coeficientes a cero con una escritura de 128 bits: `sq $0` no lo genera el
           compilador (un cero de 128 bits lo pasa por un registro con por). */
        asm __volatile__("sq $0, 0(%0)\n"
                         "sw $0, 16(%0)"
                         :
                         : "r"(mbi->Coeffs[bp])
                         : "memory");
#else
        memset(&mbi->Coeffs[bp][0], 0, 16);
        memset(&mbi->Coeffs[bp][8], 0, 8);
        memset(&mbi->Coeffs[bp][16], 0, 8);
        memset(&mbi->Coeffs[bp][24], 0, 8);
        mbi->Coeffs[bp][32] = 0;
#endif
    } else {
        idct[63](mbi->Coeffs[bp], pbi->quantizer->dequant_coeffs[VP6_QTableSelect[bp]],
                 pbi->ReconDataBuffer);

#ifdef EA_PLATFORM_PLAYSTATION2
        asm __volatile__("sq $0, 0(%0)\n"
                         "sq $0, 16(%0)\n"
                         "sq $0, 32(%0)\n"
                         "sq $0, 48(%0)\n"
                         "sq $0, 64(%0)\n"
                         "sq $0, 80(%0)\n"
                         "sq $0, 96(%0)\n"
                         "sq $0, 112(%0)"
                         :
                         : "r"(mbi->Coeffs[bp])
                         : "memory");
#else
        memset(mbi->Coeffs[bp], 0, 128);
#endif
    }

    VP6_ReconstructBlock(pbi, bp);
}

void VP6_DecodeMacroBlock(struct PB_INSTANCE *pbi, unsigned int MBrow, unsigned int MBcol) {
    unsigned int MBPointer;
    int NextBlock;

    if (pbi->Configuration.Interlaced) {
        unsigned char prob = pbi->probInterlaced;

        if (MBcol > 3) {
            if (pbi->mbi.Interlaced) {
                prob = prob - (prob >> 1);
            } else {
                prob = prob + ((256 - prob) >> 1);
            }
        }

        pbi->mbi.Interlaced = nDecodeBool(&pbi->br, prob);
    } else {
        pbi->mbi.Interlaced = 0;
    }

    if (pbi->FrameType == 0) {
        pbi->mbi.Mode = CODE_INTRA;
    } else {
        VP6_decodeModeAndMotionVector(pbi, MBrow, MBcol);
    }

    if (pbi->mbi.Interlaced == 0) {
        NextBlock = 8;
        pbi->mbi.CurrentReconStride = pbi->Configuration.YStride;
    } else {
        NextBlock = 1;
        pbi->mbi.CurrentReconStride = pbi->Configuration.YStride * 2;
    }

    pbi->mbi.MvShift = 2;
#if defined(EA_PLATFORM_WIN32)
    /* PC (speed.exe 0x807DC3): MvModMask se guarda antes que FrameReconStride. */
    pbi->mbi.MvModMask = 3;
    pbi->mbi.FrameReconStride = pbi->Configuration.YStride;
    pbi->mbi.LastDc = pbi->fc.LastDcY;
#else
    pbi->mbi.FrameReconStride = pbi->Configuration.YStride;
    pbi->mbi.LastDc = pbi->fc.LastDcY;
    pbi->mbi.MvModMask = 3;
#endif
    pbi->mbi.Plane = 0;
    pbi->mbi.SourceY = MBrow * 16;
    pbi->mbi.SourceX = MBcol * 16;
    MBPointer = pbi->ReconYDataOffset + pbi->mbi.SourceY * pbi->Configuration.YStride +
                pbi->mbi.SourceX;
    pbi->mbi.Recon = MBPointer;
    pbi->mbi.Above = &pbi->fc.AboveY[MBcol * 2];
    pbi->mbi.Left = &pbi->fc.LeftY[0];
    VP6_DecodeBlock(pbi, MBrow, MBcol, TOP_LEFT_Y_BLOCK);

    pbi->mbi.Recon += 8;
    pbi->mbi.Above = &pbi->fc.AboveY[MBcol * 2 + 1];
    pbi->mbi.Left = &pbi->fc.LeftY[0];
    pbi->mbi.SourceX += 8;
    VP6_DecodeBlock(pbi, MBrow, MBcol, TOP_RIGHT_Y_BLOCK);

    MBPointer += NextBlock * pbi->Configuration.YStride; pbi->mbi.Recon = MBPointer;
    pbi->mbi.Above = &pbi->fc.AboveY[MBcol * 2];
    pbi->mbi.Left = &pbi->fc.LeftY[1];
    pbi->mbi.SourceX -= 8;
    pbi->mbi.SourceY += NextBlock;
    VP6_DecodeBlock(pbi, MBrow, MBcol, BOTTOM_LEFT_Y_BLOCK);

    pbi->mbi.Recon += 8;
    pbi->mbi.Above = &pbi->fc.AboveY[MBcol * 2 + 1];
    pbi->mbi.Left = &pbi->fc.LeftY[1];
    pbi->mbi.SourceX += 8;
    VP6_DecodeBlock(pbi, MBrow, MBcol, BOTTOM_RIGHT_Y_BLOCK);

    pbi->mbi.FrameReconStride = pbi->Configuration.UVStride;
    pbi->mbi.CurrentReconStride = pbi->Configuration.UVStride;
    pbi->mbi.SourceY = MBrow * 8;
    pbi->mbi.SourceX = MBcol * 8;
    pbi->mbi.MvShift = 3;
    pbi->mbi.MvModMask = 7;

    pbi->mbi.Recon = pbi->ReconUDataOffset + pbi->mbi.SourceY * pbi->Configuration.UVStride +
                     pbi->mbi.SourceX;
    pbi->mbi.Above = &pbi->fc.AboveU[MBcol];
    pbi->mbi.Left = &pbi->fc.LeftU;
    pbi->mbi.LastDc = pbi->fc.LastDcU;
    pbi->mbi.Plane = 1;
    VP6_DecodeBlock(pbi, MBrow, MBcol, U_BLOCK);

    pbi->mbi.Above = &pbi->fc.AboveV[MBcol];
    pbi->mbi.Left = &pbi->fc.LeftV;
    pbi->mbi.Recon = pbi->ReconVDataOffset + pbi->mbi.SourceY * pbi->mbi.CurrentReconStride +
                     pbi->mbi.SourceX;
    pbi->mbi.LastDc = pbi->fc.LastDcV;
    pbi->mbi.Plane = 2;
    VP6_DecodeBlock(pbi, MBrow, MBcol, V_BLOCK);
}
