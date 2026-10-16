#include <string.h>

#include "pbdll.h"

inline UINT32 VP6_bitread(BOOL_CODER *br, int bits) {
    UINT32 z = 0;
    int bit;

    for (bit = bits - 1; bit >= 0; bit--) {
        z |= (VP6_DecodeBool128(br) << bit);
    }

    return z;
}

void InitHeaderBuffer(FRAME_HEADER *Header, unsigned char *Buffer) {
    Header->buffer = Buffer;
    Header->value = (Buffer[0] << 24) + (Buffer[1] << 16) + (Buffer[2] << 8) + Buffer[3];
    Header->bits_available = 32;
    Header->pos = 4;
}

UINT32 ReadHeaderBits(FRAME_HEADER *Header, UINT32 BitsRequired) {
    UINT32 pos = Header->pos;
    UINT32 available = Header->bits_available;
    UINT32 value = Header->value;
    UINT8 *Buffer = Header->buffer + pos;
    UINT32 RetVal = 0;

    if (available < BitsRequired) {
        RetVal = value >> (32 - available);
        BitsRequired -= available;
        value = (Buffer[0] << 24) + (Buffer[1] << 16) + (Buffer[2] << 8) + Buffer[3];
        RetVal <<= BitsRequired;
        pos += 4;
        available = 32;
    }

    RetVal |= value >> (32 - BitsRequired);
    Header->value = value << BitsRequired;
    Header->bits_available = available - BitsRequired;
    Header->pos = pos;

    return RetVal;
}

static BOOL LoadFrameHeader(PB_INSTANCE *pbi) {
    UINT8 DctQMask;
    FRAME_HEADER *Header = &pbi->Header;
    int RetVal = 1;

    pbi->FrameType = ReadHeaderBits(Header, 1);
    DctQMask = ReadHeaderBits(Header, 6);
    pbi->MultiStream = (UINT8)ReadHeaderBits(Header, 1);

    if (pbi->FrameType == 0) {
        pbi->Vp3VersionNo = ReadHeaderBits(Header, 5);
        pbi->VpProfile = ReadHeaderBits(Header, 2);

        if (pbi->Vp3VersionNo > 7) {
            return 0;
        }

        VP6_InitQTables(pbi->quantizer, pbi->Vp3VersionNo);

        pbi->Configuration.Interlaced = (UINT8)ReadHeaderBits(Header, 1);

        if (pbi->MultiStream || pbi->VpProfile == 0) {
            VP6_StartDecode(&pbi->br, pbi->Header.buffer + 4);
            pbi->Buff2Offset = ReadHeaderBits(Header, 16);
        } else {
            VP6_StartDecode(&pbi->br, pbi->Header.buffer + 2);
        }

        SetPPInterlacedMode(pbi->postproc, pbi->Configuration.Interlaced);

        if (pbi->Configuration.Interlaced) {
            SetDeInterlaceMode(pbi->postproc, pbi->DeInterlaceMode);
        }

        {
            UINT32 HFragments;
            UINT32 VFragments;
            UINT32 HOldScaled;
            UINT32 VOldScaled;
            UINT32 HNewScaled;
            UINT32 VNewScaled;
            UINT32 OutputHFragments;
            UINT32 OutputVFragments;

            VFragments = (UINT8)VP6_bitread(&pbi->br, 8) * 2;
            HFragments = (UINT8)VP6_bitread(&pbi->br, 8) * 2;
            OutputVFragments = (UINT8)VP6_bitread(&pbi->br, 8) * 2;
            OutputHFragments = (UINT8)VP6_bitread(&pbi->br, 8) * 2;

            if (pbi->Configuration.HRatio == 0) {
                pbi->Configuration.HRatio = 1;
            }

            if (pbi->Configuration.VRatio == 0) {
                pbi->Configuration.VRatio = 1;
            }

            HOldScaled = pbi->Configuration.HScale * pbi->HFragments * 8 / pbi->Configuration.HRatio;
            VOldScaled = pbi->Configuration.VScale * pbi->VFragments * 8 / pbi->Configuration.VRatio;

            pbi->Configuration.ExpandedFrameWidth = OutputHFragments * 8;
            pbi->Configuration.ExpandedFrameHeight = OutputVFragments * 8;

            if (VFragments >= OutputVFragments) {
                pbi->Configuration.VRatio = pbi->Configuration.VScale = 1;
            } else if (VFragments * 5 >= OutputVFragments * 4) {
                pbi->Configuration.VScale = 5;
                pbi->Configuration.VRatio = 4;
            } else if (VFragments * 5 >= OutputVFragments * 3) {
                pbi->Configuration.VScale = 5;
                pbi->Configuration.VRatio = 3;
            } else {
                pbi->Configuration.VScale = 2;
                pbi->Configuration.VRatio = 1;
            }

            if (HFragments >= OutputHFragments) {
                pbi->Configuration.HRatio = pbi->Configuration.HScale = 1;
            } else if (HFragments * 5 >= OutputHFragments * 4) {
                pbi->Configuration.HScale = 5;
                pbi->Configuration.HRatio = 4;
            } else if (HFragments * 5 >= OutputHFragments * 3) {
                pbi->Configuration.HScale = 5;
                pbi->Configuration.HRatio = 3;
            } else {
                pbi->Configuration.HScale = 2;
                pbi->Configuration.HRatio = 1;
            }

            HNewScaled = pbi->Configuration.HScale * HFragments * 8 / pbi->Configuration.HRatio;
            VNewScaled = pbi->Configuration.VScale * VFragments * 8 / pbi->Configuration.VRatio;

            pbi->ScaleWidth = HNewScaled;
            pbi->ScaleHeight = VNewScaled;

            pbi->Configuration.ScalingMode = VP6_bitread(&pbi->br, 2);

            if (VFragments != pbi->VFragments || HFragments != pbi->HFragments) {
                pbi->Configuration.VideoFrameWidth = HFragments * 8;
                pbi->Configuration.VideoFrameHeight = VFragments * 8;
                VP6_InitFrameDetails(pbi);
            }

            if (pbi->ScaleBuffer && (HOldScaled != HNewScaled || VOldScaled != VNewScaled)) {
                memset(pbi->ScaleBuffer, 0, (pbi->OutputWidth + 32) * (pbi->OutputHeight + 32));
                memset(pbi->ScaleBuffer + (pbi->OutputWidth + 32) * (pbi->OutputHeight + 32), 128,
                       (pbi->OutputWidth + 32) * (pbi->OutputHeight + 32) / 2);
            }
        }

        if (pbi->VpProfile) {
            if (VP6_DecodeBool(&pbi->br, 128)) {
                pbi->PredictionFilterMode = 2;
                pbi->PredictionFilterVarThresh = VP6_bitread(&pbi->br, 5) * 32;
                pbi->PredictionFilterMvSizeThresh = VP6_bitread(&pbi->br, 3);
            } else if (VP6_DecodeBool(&pbi->br, 128)) {
                pbi->PredictionFilterMode = 1;
            } else {
                pbi->PredictionFilterMode = 0;
            }
        }
    } else {
        if (pbi->MultiStream || pbi->VpProfile == 0) {
            VP6_StartDecode(&pbi->br, pbi->Header.buffer + 3);
            pbi->Buff2Offset = ReadHeaderBits(Header, 16);
        } else {
            VP6_StartDecode(&pbi->br, pbi->Header.buffer + 1);
        }

        pbi->RefreshGoldenFrame = VP6_DecodeBool(&pbi->br, 128);

        if (pbi->VpProfile) {
            pbi->UseLoopFilter = VP6_DecodeBool(&pbi->br, 128);

            if (pbi->UseLoopFilter) {
                pbi->UseLoopFilter = (pbi->UseLoopFilter << 1) | VP6_DecodeBool(&pbi->br, 128);
            }
        }
    }

    pbi->UseHuffman = VP6_DecodeBool(&pbi->br, 128);
    pbi->quantizer->FrameQIndex = DctQMask;
    pbi->quantizer->ThisFrameQuantizerValue = pbi->quantizer->QThreshTable[DctQMask];
    VP6_UpdateQ(pbi->quantizer, pbi->Vp3VersionNo);

    return RetVal;
}

BOOL VP6_LoadFrame(PB_INSTANCE *pbi) {
    int RetVal = 1;

    if (LoadFrameHeader(pbi) == 0) {
        RetVal = 0;
    }

    return RetVal;
}
