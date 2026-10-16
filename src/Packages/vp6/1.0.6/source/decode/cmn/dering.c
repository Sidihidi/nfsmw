#include <stdlib.h>

#include "postp.h"
#include "vputil.h"

UINT32 DeringModifierV1[64];

UINT32 DeringModifierV2[64] = {
    9, 9, 8, 8, 7, 7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2,
};

UINT32 DeringModifierV3[64] = {
    9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 7, 7,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5,
    4, 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 0, 0, 0, 0, 0,
};

INT32 SharpenModifier[64] = {
    -12, -11, -10, -10, -9, -9, -9, -9, -6, -6, -6, -6, -6, -6, -6, -6,
    -4,  -4,  -4,  -4,  -4, -4, -4, -4, -2, -2, -2, -2, -2, -2, -2, -2,
    -2,  -2,  -2,  -2,  -2, -2, -2, -2,  0,  0,  0,  0,  0,  0,  0,  0,
    0,   0,   0,   0,   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
};

#define Clamp(val) ((val) < 0 ? 0 : ((val) > 255 ? 255 : (val)))

#define DERING_PIXEL(J, DST) \
    atot = 128; \
    B = round; \
    p = curRow[rowOffset + (J)]; \
    pl = curRow[rowOffset + (J) - 1]; \
    al = LRMod[(k * 9) + (J)]; \
    atot -= al; \
    B += al * pl; \
    pu = lastRow[rowOffset + (J)]; \
    au = UDMod[(k * 8) + (J)]; \
    atot -= au; \
    B += au * pu; \
    pd = nextRow[rowOffset + (J)]; \
    ad = UDMod[((k + 1) * 8) + (J)]; \
    atot -= ad; \
    B += ad * pd; \
    pr = curRow[rowOffset + (J) + 1]; \
    ar = LRMod[(k * 9) + (J) + 1]; \
    atot -= ar; \
    B += ar * pr; \
    newVal = ((atot * p) + B) >> 7; \
    DST = Clamp(newVal);

void DeringBlockStrong_C(const POSTPROC_INSTANCE *pbi, const unsigned char *SrcPtr,
                         unsigned char *DstPtr, const INT32 Pitch, UINT32 FragQIndex,
                         UINT32 *QuantScale) {
    INT32 B, al, ar, au, ad, atot;
    INT32 High, Low;
    INT32 TmpMod;
    INT32 newVal;
    short UDMod[72], LRMod[72];
    UINT32 j, k;
    unsigned char p, pl, pr, pu, pd;
    UINT32 rowOffset = 0;
    UINT32 round = (1 << 6);
    UINT32 QValue = QuantScale[FragQIndex];
    INT32 Sharpen = SharpenModifier[FragQIndex];
    const unsigned char *Src, *curRow, *lastRow, *nextRow;
    unsigned char *dstRow;

    Src = SrcPtr;
    curRow = SrcPtr;
    lastRow = SrcPtr - Pitch;
    nextRow = SrcPtr + Pitch;
    dstRow = DstPtr;

    (void)pbi;
    Low = 0;

    High = 3 * QValue;
    if (High > 32)
        High = 32;

    for (k = 0; k < 9; k++) {
        for (j = 0; j < 8; j++) {
            TmpMod = 32 + QValue - abs(Src[j] - Src[j - Pitch]);
            if (TmpMod < -64)
                TmpMod = Sharpen;
            else if (TmpMod < Low)
                TmpMod = Low;
            else if (TmpMod > High)
                TmpMod = High;
            UDMod[(k * 8) + j] = (short)TmpMod;
        }
        Src += Pitch;
    }

    Src = SrcPtr;
    for (k = 0; k < 8; k++) {
        for (j = 0; j < 9; j++) {
            TmpMod = 32 + QValue - abs(Src[j] - Src[j - 1]);
            if (TmpMod < -64)
                TmpMod = Sharpen;
            else if (TmpMod < 0)
                TmpMod = Low;
            else if (TmpMod > High)
                TmpMod = High;
            LRMod[(k * 9) + j] = (short)TmpMod;
        }
        Src += Pitch;
    }

    for (k = 0; k < 8; k++) {
        int newPixel[8];

        DERING_PIXEL(0, newPixel[0])
        DERING_PIXEL(1, newPixel[1])
        DERING_PIXEL(2, newPixel[2])
        DERING_PIXEL(3, newPixel[3])
        DERING_PIXEL(4, newPixel[4])
        DERING_PIXEL(5, newPixel[5])
        DERING_PIXEL(6, newPixel[6])
        DERING_PIXEL(7, newPixel[7])

        dstRow[rowOffset + 0] = (signed char)newPixel[0];
        dstRow[rowOffset + 1] = (signed char)newPixel[1];
        dstRow[rowOffset + 2] = (signed char)newPixel[2];
        dstRow[rowOffset + 3] = (signed char)newPixel[3];
        dstRow[rowOffset + 4] = (signed char)newPixel[4];
        dstRow[rowOffset + 5] = (signed char)newPixel[5];
        dstRow[rowOffset + 6] = (signed char)newPixel[6];
        dstRow[rowOffset + 7] = (signed char)newPixel[7];

        rowOffset += Pitch;
    }
}

void DeringBlockWeak_C(const POSTPROC_INSTANCE *pbi, const unsigned char *SrcPtr,
                       unsigned char *DstPtr, const INT32 Pitch, UINT32 FragQIndex,
                       UINT32 *QuantScale) {
    INT32 B, al, ar, au, ad, atot;
    INT32 High, Low;
    INT32 newVal;
    INT32 TmpMod;
    short UDMod[72], LRMod[72];
    UINT32 j, k;
    unsigned char p, pl, pr, pu, pd;
    UINT32 rowOffset = 0;
    UINT32 round = (1 << 6);
    UINT32 QValue = QuantScale[FragQIndex];
    INT32 Sharpen = SharpenModifier[FragQIndex];
    const unsigned char *Src = SrcPtr;
    const unsigned char *curRow = SrcPtr;
    const unsigned char *lastRow = SrcPtr - Pitch;
    const unsigned char *nextRow = SrcPtr + Pitch;
    unsigned char *dstRow = DstPtr;

    (void)pbi;

    Low = 0;
    High = 3 * QValue;
    if (High > 24)
        High = 24;

    for (k = 0; k < 9; k++) {
        for (j = 0; j < 8; j++) {
            TmpMod = 32 + QValue - 2 * abs(Src[j] - Src[j - Pitch]);
            if (TmpMod < -64)
                TmpMod = Sharpen;
            else if (TmpMod < Low)
                TmpMod = Low;
            else if (TmpMod > High)
                TmpMod = High;
            UDMod[(k * 8) + j] = (short)TmpMod;
        }
        Src += Pitch;
    }

    Src = SrcPtr;
    for (k = 0; k < 8; k++) {
        for (j = 0; j < 9; j++) {
            TmpMod = 32 + QValue - 2 * abs(Src[j] - Src[j - 1]);
            if (TmpMod < -64)
                TmpMod = Sharpen;
            else if (TmpMod < Low)
                TmpMod = Low;
            else if (TmpMod > High)
                TmpMod = High;
            LRMod[(k * 9) + j] = (short)TmpMod;
        }
        Src += Pitch;
    }

    for (k = 0; k < 8; k++) {
        for (j = 0; j < 8; j++) {
            atot = 128;
            B = round;
            p = curRow[rowOffset + j];

            pl = curRow[rowOffset + j - 1];
            al = LRMod[(k * 9) + j];
            atot -= al;
            B += al * pl;

            pu = lastRow[rowOffset + j];
            au = UDMod[(k * 8) + j];
            atot -= au;
            B += au * pu;

            pd = nextRow[rowOffset + j];
            ad = UDMod[((k + 1) * 8) + j];
            atot -= ad;
            B += ad * pd;

            pr = curRow[rowOffset + j + 1];
            ar = LRMod[(k * 9) + j + 1];
            atot -= ar;
            B += ar * pr;

            newVal = ((atot * p) + B) >> 7;

            dstRow[rowOffset + j] = (signed char)Clamp(newVal);
        }

        rowOffset += Pitch;
    }
}

void DeringFrame(POSTPROC_INSTANCE *pbi, unsigned char *Src, unsigned char *Dst) {
    UINT32 Block;
    UINT32 col, row;
    UINT32 BlocksAcross, BlocksDown;
    UINT32 *QuantScale;
    UINT32 LineLength;
    INT32 Thresh1, Thresh2, Thresh3, Thresh4;
    unsigned char *SrcPtr, *DestPtr;
    INT32 Quality = pbi->FrameQIndex;

    /* PC (speed.exe 0x8057AF/0x805BE3): MSVC 7.1 no canonicaliza `> 4` a `>= 5`
     * como GCC, y el original compara contra 5 y contra 2. */
#if defined(EA_PLATFORM_WIN32)
    if (pbi->Vp3VersionNo >= 5) {
#else
    if (pbi->Vp3VersionNo > 4) {
#endif
        Thresh1 = 384;
        Thresh2 = 2304;
        Thresh3 = 2880;
        Thresh4 = 5760;
    } else {
        Thresh1 = 2048;
        Thresh2 = 30720;
        Thresh3 = 92160;
        Thresh4 = 122880;
    }

#if defined(EA_PLATFORM_WIN32)
    if (pbi->Vp3VersionNo >= 5)
        QuantScale = DeringModifierV3;
    else if (pbi->Vp3VersionNo >= 2)
        QuantScale = DeringModifierV2;
    else
        QuantScale = DeringModifierV1;
#else
    if (pbi->Vp3VersionNo > 4)
        QuantScale = DeringModifierV3;
    else if (pbi->Vp3VersionNo > 1)
        QuantScale = DeringModifierV2;
    else
        QuantScale = DeringModifierV1;
#endif

    BlocksAcross = pbi->HFragments;
    BlocksDown = pbi->VFragments;

    SrcPtr = Src + pbi->ReconYDataOffset;
    DestPtr = Dst + pbi->ReconYDataOffset;
    LineLength = pbi->YStride;

    Block = 0;

    for (row = 0; row < BlocksDown; row++) {
        for (col = 0; col < BlocksAcross; col++) {
            int Variance = pbi->FragmentVariances[Block];

            if (pbi->PostProcessingLevel > 5 && Variance > Thresh3) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                if ((col > 0 && pbi->FragmentVariances[Block - 1] > Thresh4) ||
                    (col + 1 < BlocksAcross && pbi->FragmentVariances[Block + 1] > Thresh4) ||
                    (row + 1 < BlocksDown &&
                     pbi->FragmentVariances[Block + BlocksAcross] > Thresh4) ||
                    (row > 0 && pbi->FragmentVariances[Block - BlocksAcross] > Thresh4)) {
                    DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,
                                      Quality, QuantScale);
                    DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,
                                      Quality, QuantScale);
                }
            } else if (Variance > Thresh2) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh1) {
                DeringBlockWeak(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                QuantScale);
            } else {
                CopyBlock(SrcPtr + 8 * col, DestPtr + 8 * col, LineLength);
            }

            Block++;
        }

        SrcPtr += 8 * LineLength;
        DestPtr += 8 * LineLength;
    }

    BlocksAcross = BlocksAcross / 2;
    BlocksDown = BlocksDown / 2;
    LineLength = LineLength / 2;
    SrcPtr = Src + pbi->ReconUDataOffset;
    DestPtr = Dst + pbi->ReconUDataOffset;

    for (row = 0; row < BlocksDown; row++) {
        for (col = 0; col < BlocksAcross; col++) {
            int Variance = pbi->FragmentVariances[Block];

#if defined(EA_PLATFORM_WIN32)
            if (pbi->Vp3VersionNo < 5)
#else
            if (pbi->Vp3VersionNo <= 4)
#endif
                Quality = pbi->FragQIndex[Block];

            if (pbi->PostProcessingLevel > 5 && Variance > Thresh4) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh2) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh1) {
                DeringBlockWeak(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                QuantScale);
            } else {
                CopyBlock(SrcPtr + 8 * col, DestPtr + 8 * col, LineLength);
            }

            Block++;
        }

        SrcPtr += 8 * LineLength;
        DestPtr += 8 * LineLength;
    }

    SrcPtr = Src + pbi->ReconVDataOffset;
    DestPtr = Dst + pbi->ReconVDataOffset;

    for (row = 0; row < BlocksDown; row++) {
        for (col = 0; col < BlocksAcross; col++) {
            int Variance = pbi->FragmentVariances[Block];

#if defined(EA_PLATFORM_WIN32)
            if (pbi->Vp3VersionNo < 5)
#else
            if (pbi->Vp3VersionNo <= 4)
#endif
                Quality = pbi->FragQIndex[Block];

            if (pbi->PostProcessingLevel > 5 && Variance > Thresh4) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh2) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh1) {
                DeringBlockWeak(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                QuantScale);
            } else {
                CopyBlock(SrcPtr + 8 * col, DestPtr + 8 * col, LineLength);
            }

            Block++;
        }

        SrcPtr += 8 * LineLength;
        DestPtr += 8 * LineLength;
    }
}

void DeringFrameInterlaced(POSTPROC_INSTANCE *pbi, unsigned char *Src, unsigned char *Dst) {
    UINT32 Block;
    UINT32 col, row;
    UINT32 BlocksAcross, BlocksDown;
    UINT32 LineLength;
    UINT32 *QuantScale;
    INT32 Thresh1, Thresh2, Thresh3, Thresh4;
    unsigned char *SrcPtr, *DestPtr;
    INT32 Quality = pbi->FrameQIndex;

#if defined(EA_PLATFORM_WIN32)
    if (pbi->Vp3VersionNo >= 5) {
#else
    if (pbi->Vp3VersionNo > 4) {
#endif
        Thresh1 = 384;
        Thresh2 = 2304;
        Thresh3 = 2880;
        Thresh4 = 5760;
    } else {
        Thresh1 = 2048;
        Thresh2 = 30720;
        Thresh3 = 92160;
        Thresh4 = 122880;
    }

#if defined(EA_PLATFORM_WIN32)
    if (pbi->Vp3VersionNo >= 5)
        QuantScale = DeringModifierV3;
    else if (pbi->Vp3VersionNo >= 2)
        QuantScale = DeringModifierV2;
    else
        QuantScale = DeringModifierV1;

    /* PC: mismo orden de asignaciones que en DeringFrame (speed.exe 0x805BE3). */
    BlocksAcross = pbi->HFragments;
    BlocksDown = pbi->VFragments / 2;
    SrcPtr = Src + pbi->ReconYDataOffset;
    DestPtr = Dst + pbi->ReconYDataOffset;
    LineLength = pbi->YStride * 2;
    Block = 0;
#else
    if (pbi->Vp3VersionNo > 4)
        QuantScale = DeringModifierV3;
    else if (pbi->Vp3VersionNo > 1)
        QuantScale = DeringModifierV2;
    else
        QuantScale = DeringModifierV1;

    Block = 0;
    BlocksDown = pbi->VFragments / 2;
    BlocksAcross = pbi->HFragments;
    LineLength = pbi->YStride * 2;
    SrcPtr = Src + pbi->ReconYDataOffset;
    DestPtr = Dst + pbi->ReconYDataOffset;
#endif

    for (row = 0; row < BlocksDown; row++) {
        for (col = 0; col < BlocksAcross; col++) {
            int Variance = pbi->FragmentVariances[Block];

            if (pbi->PostProcessingLevel > 5 && Variance > Thresh3) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                if ((col > 0 && pbi->FragmentVariances[Block - 1] > Thresh4) ||
                    (col + 1 < BlocksAcross && pbi->FragmentVariances[Block + 1] > Thresh4) ||
                    (row + 1 < BlocksDown &&
                     pbi->FragmentVariances[Block + BlocksAcross] > Thresh4) ||
                    (row > 0 && pbi->FragmentVariances[Block - BlocksAcross] > Thresh4)) {
                    DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,
                                      Quality, QuantScale);
                    DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,
                                      Quality, QuantScale);
                }
            } else if (Variance > Thresh2) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh1) {
                DeringBlockWeak(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                QuantScale);
            } else {
                CopyBlock(SrcPtr + 8 * col, DestPtr + 8 * col, LineLength);
            }

            Block++;
        }

        SrcPtr += 8 * LineLength;
        DestPtr += 8 * LineLength;
    }

    SrcPtr = Src + pbi->ReconYDataOffset + pbi->YStride;
    DestPtr = Dst + pbi->ReconYDataOffset + pbi->YStride;

    for (row = 0; row < BlocksDown; row++) {
        for (col = 0; col < BlocksAcross; col++) {
            int Variance = pbi->FragmentVariances[Block];

            if (pbi->PostProcessingLevel > 5 && Variance > Thresh3) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                if ((col > 0 && pbi->FragmentVariances[Block - 1] > Thresh4) ||
                    (col + 1 < BlocksAcross && pbi->FragmentVariances[Block + 1] > Thresh4) ||
                    (row + 1 < BlocksDown &&
                     pbi->FragmentVariances[Block + BlocksAcross] > Thresh4) ||
                    (row > 0 && pbi->FragmentVariances[Block - BlocksAcross] > Thresh4)) {
                    DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,
                                      Quality, QuantScale);
                    DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength,
                                      Quality, QuantScale);
                }
            } else if (Variance > Thresh2) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh1) {
                DeringBlockWeak(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                QuantScale);
            } else {
                CopyBlock(SrcPtr + 8 * col, DestPtr + 8 * col, LineLength);
            }

            Block++;
        }

        SrcPtr += 8 * LineLength;
        DestPtr += 8 * LineLength;
    }

    BlocksAcross = BlocksAcross / 2;
    LineLength = LineLength / 4;
    SrcPtr = Src + pbi->ReconUDataOffset;
    DestPtr = Dst + pbi->ReconUDataOffset;

    for (row = 0; row < BlocksDown; row++) {
        for (col = 0; col < BlocksAcross; col++) {
            int Variance = pbi->FragmentVariances[Block];

            if (pbi->PostProcessingLevel > 5 && Variance > Thresh4) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh2) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh1) {
                DeringBlockWeak(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                QuantScale);
            } else {
                CopyBlock(SrcPtr + 8 * col, DestPtr + 8 * col, LineLength);
            }

            Block++;
        }

        SrcPtr += 8 * LineLength;
        DestPtr += 8 * LineLength;
    }

    SrcPtr = Src + pbi->ReconVDataOffset;
    DestPtr = Dst + pbi->ReconVDataOffset;

    for (row = 0; row < BlocksDown; row++) {
        for (col = 0; col < BlocksAcross; col++) {
            int Variance = pbi->FragmentVariances[Block];

            if (pbi->PostProcessingLevel > 5 && Variance > Thresh4) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh2) {
                DeringBlockStrong(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                  QuantScale);
            } else if (Variance > Thresh1) {
                DeringBlockWeak(pbi, SrcPtr + 8 * col, DestPtr + 8 * col, LineLength, Quality,
                                QuantScale);
            } else {
                CopyBlock(SrcPtr + 8 * col, DestPtr + 8 * col, LineLength);
            }

            Block++;
        }

        SrcPtr += 8 * LineLength;
        DestPtr += 8 * LineLength;
    }
}
