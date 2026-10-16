#include <stdio.h>
#include <string.h>

#include "pbdll.h"
#include "duck_mem.h"

BOOL VP6_StartDecoder(PB_INSTANCE **pbi, UINT32 ImageWidth, UINT32 ImageHeight) {
    *pbi = VP6_CreatePBInstance();

    (*pbi)->ScaleWidth = ImageWidth;
    (*pbi)->ScaleHeight = ImageHeight;
    (*pbi)->OutputWidth = ImageWidth;
    (*pbi)->OutputHeight = ImageHeight;
    (*pbi)->Configuration.VideoFrameWidth = ImageWidth;
    (*pbi)->Configuration.VideoFrameHeight = ImageHeight;

    (*pbi)->postproc = CreatePostProcInstance(&(*pbi)->Configuration);
    (*pbi)->quantizer = VP6_CreateQuantizer();
    (*pbi)->ProcessorFrequency = CPUFrequency;

    if (!VP6_InitFrameDetails(*pbi)) {
        VP6_DeletePBInstance(pbi);
        return FALSE;
    }

    (*pbi)->quantizer->LastQuantizerValue = 0;
    VP6_InitialiseConfiguration(*pbi);
    return TRUE;
}

void VP6_SetPbParam(PB_INSTANCE *pbi, PB_COMMAND_TYPE Command, UINT32 Parameter) {
    switch (Command) {
        case PBC_SET_CPUFREE: {
            double PixelsPerMhz = 1000.0;

            pbi->CPUFree = Parameter;

            if (PixelsPerMhz > 150)
                pbi->PostProcessingLevel = 0;
            else if (PixelsPerMhz > 100)
                pbi->PostProcessingLevel = 8;
            else if (PixelsPerMhz > 90)
                pbi->PostProcessingLevel = 4;
            else if (PixelsPerMhz > 80)
                pbi->PostProcessingLevel = 5;
            else
                pbi->PostProcessingLevel = 6;
        } break;

        case PBC_SET_ADDNOISE:
            pbi->AddNoiseMode = Parameter;
            SetAddNoiseMode(pbi->postproc, Parameter);
            break;

        case PBC_SET_REFERENCEFRAME:
            CopyFrame(pbi->postproc, (YUV_BUFFER_CONFIG *)Parameter, pbi->LastFrameRecon);
            CopyFrame(pbi->postproc, (YUV_BUFFER_CONFIG *)Parameter, pbi->GoldenFrame);
            break;

        case PBC_SET_POSTPROC:
            if (Parameter == 9) {
                VP6_SetPbParam(pbi, PBC_SET_CPUFREE, 70);
            } else {
                pbi->PostProcessingLevel = Parameter;
                pbi->CPUFree = 0;
            }
            break;

        case PBC_SET_DEINTERLACEMODE:
            pbi->DeInterlaceMode = Parameter;
            break;

        case PBC_SET_BLACKCLAMP:
            pbi->BlackClamp = Parameter;
            break;

        case PBC_SET_WHITECLAMP:
            pbi->WhiteClamp = Parameter;
            break;

        default:
            break;
    }
}

void VP6_GetYUVConfig(PB_INSTANCE *pbi, YUV_BUFFER_CONFIG *YuvConfig) {
    if (pbi->PostProcessingLevel || (pbi->Configuration.Interlaced && pbi->DeInterlaceMode)) {
        if (pbi->PostProcessBuffer == 0) {
            UINT32 size = pbi->ReconYPlaneSize + pbi->ReconUVPlaneSize * 2 + 32;

            pbi->PostProcessBufferAlloc = (YUV_BUFFER_ENTRY *)duck_malloc(pbi->Configuration.YStride + size, DMEM_GENERAL);

            pbi->PostProcessBuffer = (YUV_BUFFER_ENTRY *)(((UINT32)pbi->PostProcessBufferAlloc + 31) & ~31);
            AllocatePostProcBuffers(pbi->postproc);
        }

        PostProcess(pbi->postproc, pbi->Vp3VersionNo, pbi->FrameType, pbi->PostProcessingLevel, pbi->AvgFrameQIndex,
                    pbi->LastFrameRecon, pbi->PostProcessBuffer, (UINT8 *)pbi->FragInfo, sizeof(FRAG_INFO), 1);

        if (pbi->BlackClamp) {
            ClampLevels(pbi->postproc, pbi->BlackClamp, pbi->WhiteClamp, pbi->PostProcessBuffer,
                        pbi->PostProcessBuffer);
        }
    }

    if (pbi->Configuration.VideoFrameWidth < pbi->OutputWidth ||
        pbi->Configuration.VideoFrameHeight < pbi->OutputHeight) {
        YuvConfig->YWidth = pbi->OutputWidth + 32;
        YuvConfig->YHeight = pbi->OutputHeight + 32;
        YuvConfig->YStride = YuvConfig->YWidth;

        YuvConfig->UVWidth = YuvConfig->YWidth / 2;
        YuvConfig->UVHeight = YuvConfig->YHeight / 2;
        YuvConfig->UVStride = YuvConfig->UVWidth;

        YuvConfig->YBuffer = (char *)pbi->ScaleBuffer;
        YuvConfig->UBuffer = (char *)pbi->ScaleBuffer + YuvConfig->YWidth * YuvConfig->YHeight;
        YuvConfig->VBuffer = (char *)pbi->ScaleBuffer + YuvConfig->YWidth * YuvConfig->YHeight +
                             YuvConfig->UVWidth * YuvConfig->UVHeight;

        YuvConfig->YBufferStart = (char *)pbi->ScaleBuffer;

        if (pbi->PostProcessingLevel) {
            ScaleOrCenter(pbi->postproc, pbi->PostProcessBuffer, YuvConfig);
        } else {
            ScaleOrCenter(pbi->postproc, pbi->LastFrameRecon, YuvConfig);
        }

        YuvConfig->YBuffer += ((YuvConfig->YHeight - pbi->OutputHeight) / 2) * YuvConfig->YStride +
                              (YuvConfig->YWidth - pbi->OutputWidth) / 2;

        YuvConfig->YWidth = pbi->OutputWidth;
        YuvConfig->YHeight = pbi->OutputHeight;

        YuvConfig->UBuffer += ((YuvConfig->UVHeight - pbi->OutputHeight / 2) / 2) * YuvConfig->UVStride +
                              (YuvConfig->UVWidth - pbi->OutputWidth / 2) / 2;

        YuvConfig->VBuffer += ((YuvConfig->UVHeight - pbi->OutputHeight / 2) / 2) * YuvConfig->UVStride +
                              (YuvConfig->UVWidth - pbi->OutputWidth / 2) / 2;

        YuvConfig->UVWidth = pbi->OutputWidth / 2;
        YuvConfig->UVHeight = pbi->OutputHeight / 2;
    } else {
        YuvConfig->YWidth = pbi->Configuration.VideoFrameWidth;
        YuvConfig->YHeight = pbi->Configuration.VideoFrameHeight;
        YuvConfig->YStride = pbi->Configuration.YStride;

        YuvConfig->UVWidth = pbi->Configuration.VideoFrameWidth / 2;
        YuvConfig->UVHeight = pbi->Configuration.VideoFrameHeight / 2;
        YuvConfig->UVStride = pbi->Configuration.UVStride;

        if (pbi->PostProcessingLevel || (pbi->Configuration.Interlaced && pbi->DeInterlaceMode)) {
            YuvConfig->YBuffer = (char *)pbi->PostProcessBuffer +
                                 (pbi->ReconYDataOffset + pbi->Configuration.YStride * 48 + 48);
            YuvConfig->UBuffer = (char *)pbi->PostProcessBuffer +
                                 (pbi->ReconUDataOffset + pbi->Configuration.UVStride * 24 + 24);
            YuvConfig->VBuffer = (char *)pbi->PostProcessBuffer +
                                 (pbi->ReconVDataOffset + pbi->Configuration.UVStride * 24 + 24);

            YuvConfig->YBufferStart = (char *)pbi->PostProcessBuffer + pbi->ReconYDataOffset;
        } else {
            YuvConfig->YBuffer = (char *)pbi->LastFrameRecon +
                                 (pbi->ReconYDataOffset + pbi->Configuration.YStride * 48 + 48);
            YuvConfig->UBuffer = (char *)pbi->LastFrameRecon +
                                 (pbi->ReconUDataOffset + pbi->Configuration.UVStride * 24 + 24);
            YuvConfig->VBuffer = (char *)pbi->LastFrameRecon +
                                 (pbi->ReconVDataOffset + pbi->Configuration.UVStride * 24 + 24);

            YuvConfig->YBufferStart = (char *)pbi->LastFrameRecon + pbi->ReconYDataOffset;
        }
    }
}

int CurrentFrame = 0;

static int VP6_DecodeFrameToYUV_internal(PB_INSTANCE *pbi, char *VideoBufferPtr, unsigned int ByteCount,
                                         UINT32 ImageWidth, UINT32 ImageHeight) {
    unsigned char *tmp;

    pbi->CurrentFrameSize = ByteCount;

    InitHeaderBuffer(&pbi->Header, (unsigned char *)VideoBufferPtr);

    if (!VP6_LoadFrame(pbi)) {
        return -1;
    }

    if (pbi->MultiStream || !pbi->VpProfile) {
        if (pbi->UseHuffman) {
            pbi->br3.bitsinremainder = 0;
            pbi->br3.remainder = 0;
            pbi->br3.position = (UINT8 *)VideoBufferPtr + pbi->Buff2Offset;
        } else {
            VP6_StartDecode(&pbi->br2, (UINT8 *)VideoBufferPtr + pbi->Buff2Offset);
        }
    }

    VP6_DecodeFrameMbs(pbi);

    /* el cuadro recien descodificado pasa a ser el ultimo */
    tmp = pbi->LastFrameRecon;
    pbi->LastFrameRecon = pbi->ThisFrameRecon;
#if VP6_VERSION >= 106
    pbi->ThisFrameRecon = pbi->OtherFrameRecon ? pbi->OtherFrameRecon : tmp;
    pbi->OtherFrameRecon = 0;
#else
    pbi->ThisFrameRecon = tmp;
#endif

    UpdateUMVBorder(pbi->postproc, pbi->LastFrameRecon);

    if (pbi->FrameType == 0 || pbi->RefreshGoldenFrame) {
#if VP6_VERSION >= 106
        /* la 1.0.6 intercambia punteros; la 1.0.3 copia el cuadro entero */
        pbi->OtherFrameRecon = pbi->GoldenFrame;
        pbi->GoldenFrame = pbi->LastFrameRecon;
#else
        memcpy(pbi->GoldenFrame, pbi->LastFrameRecon, pbi->ReconYPlaneSize + 2 * pbi->ReconUVPlaneSize);
#endif
    }

    if (pbi->FrameType == 0) {
        pbi->AvgFrameQIndex = pbi->quantizer->FrameQIndex;
    } else {
        pbi->AvgFrameQIndex = (2 + 3 * pbi->AvgFrameQIndex + pbi->quantizer->FrameQIndex) / 4;
    }

    if (pbi->br.pos > pbi->CurrentFrameSize) {
        FILE *f = fopen("badframes.stt", "a");
        fprintf(f, "%8d %8d %8d \n", CurrentFrame, pbi->br.pos, pbi->CurrentFrameSize);
        fclose(f);
    }

    CurrentFrame++;

    return 0;
}

#ifdef EA_PLATFORM_GAMECUBE
/* Los registros de cuantificacion (GQR) de Gekko: el codec los usa con psq_l/psq_st y
   hay que devolverlos como estaban. */
#define MFGQR(n) ({ UINT32 v_; asm("mfspr %0,GQR" #n : "=b"(v_)); v_; })
#define MTGQR(n, v) asm volatile("mtspr GQR" #n ",%0" : : "b"(v))

static __inline void OSInitFastCast(void) {
    asm volatile("li 3,4\n\toris 3,3,4\n\tmtspr GQR2,3\n\t"
                 "li 3,5\n\toris 3,3,5\n\tmtspr GQR3,3\n\t"
                 "li 3,6\n\toris 3,3,6\n\tmtspr GQR4,3\n\t"
                 "li 3,7\n\toris 3,3,7\n\tmtspr GQR5,3"
                 :
                 :
                 : "r3");
}
#endif

int VP6_DecodeFrameToYUV(PB_INSTANCE *pbi, char *VideoBufferPtr, unsigned int ByteCount, UINT32 ImageWidth,
                         UINT32 ImageHeight) {
    int result;
#ifdef EA_PLATFORM_GAMECUBE
    UINT32 _gqrStates[5];

    _gqrStates[0] = MFGQR(2);
    _gqrStates[1] = MFGQR(3);
    _gqrStates[2] = MFGQR(4);
    _gqrStates[3] = MFGQR(5);
    _gqrStates[4] = MFGQR(6);

    OSInitFastCast();
    MTGQR(6, 0x39053905);

    result = VP6_DecodeFrameToYUV_internal(pbi, VideoBufferPtr, ByteCount, ImageWidth, ImageHeight);

    MTGQR(6, _gqrStates[4]);
    MTGQR(5, _gqrStates[3]);
    MTGQR(4, _gqrStates[2]);
    MTGQR(3, _gqrStates[1]);
    MTGQR(2, _gqrStates[0]);
#elif defined(EA_PLATFORM_PLAYSTATION2)
    if (pbi->ScratchpadEnd) {
        /* Con el scratchpad asignado, la pila de la descodificacion tambien va alli:
           se cambia $sp a su final y se llama con los argumentos tal como llegaron. */
        asm volatile("sw    $sp, 8(%1)\n\t"
                     "addu  $sp, %1, $0\n\t"
                     "jal   VP6_DecodeFrameToYUV_internal\n\t"
                     "lw    $sp, 8($sp)\n\t"
                     "sw    $2, %0"
                     : "=m"(result)
                     : "r"(pbi->ScratchpadEnd - 16));
    } else {
        result = VP6_DecodeFrameToYUV_internal(pbi, VideoBufferPtr, ByteCount, ImageWidth, ImageHeight);
    }
#else
    result = VP6_DecodeFrameToYUV_internal(pbi, VideoBufferPtr, ByteCount, ImageWidth, ImageHeight);
#endif

    return result;
}

BOOL VP6_StopDecoder(PB_INSTANCE **pbi) {
    if (*pbi) {
        VP6_DeleteQuantizer(&(*pbi)->quantizer);
        DeletePostProcInstance(&(*pbi)->postproc);
        VP6_DeleteFragmentInfo(*pbi);
        VP6_DeleteFrameInfo(*pbi);
        VP6_DeletePBInstance(pbi);
    }

    return TRUE;
}
