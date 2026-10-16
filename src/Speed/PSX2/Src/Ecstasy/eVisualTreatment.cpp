#include "Speed/PSX2/Src/Ecstasy/eVisualTreatment.hpp"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/PSX2/Inc/DmaE.hpp"
#include "Speed/PSX2/bWare/Src/ee/include/eestruct.h"

// psx2/src/ecstasy/eVisualTreatment.cpp de la A124: el aspecto visual de MW en PS2. Cada efecto se hace con sprites
// que copian un render target sobre otro a traves de una paleta (eAttachTargetMoveSprites), y las constantes de los
// registros ALPHA del GS son globales para poder retocarlas desde el depurador (sus valores, los del .data del ELF).

int gApplyMWVisualLook = 1;

IVisualTreatment *IVisualTreatment::Get() {
    return pVisualTreatmentPlat;
}

void OpenVisualTreatment() {
    pVisualTreatmentPlat = new ("visual treatment", 0) IVisualTreatmentPlat;
}

void CloseVisualTreatment() {
#ifndef EA_BUILD_A124
    // La version de venta espera a que el GS termine antes de liberar las paletas; la A124 no lo hace.
    eWaitUntilRenderingDone();
#endif
    delete pVisualTreatmentPlat;
    pVisualTreatmentPlat = 0;
}

// Sin nada dentro, como en GameCube; el .mdebug las pone en las lineas 106 y 110, detras de CloseVisualTreatment, y por
// eso OpenVisualTreatment llama al constructor en vez de expandirlo.
IVisualTreatmentPlat::IVisualTreatmentPlat() {
}

IVisualTreatmentPlat::~IVisualTreatmentPlat() {
}

// Copia un canal de 8 bits (el verde o el alfa) de un render target de 32 bits sobre el otro canal: lee la imagen como
// PSMCT16 en columnas de 8 pixeles desplazadas y escribe con la mascara de FRAME, en dos mitades de altura.
void CopyGreenOrAlphaChannels(int srcVramAddress, int destVramAddress, eDmaChain *dma_chain, int initialU,
                              int initialX, int frameWidth, int frameHeight) {
    int num_sprites = frameWidth / 8;
    int dmatag_qwsize = 1;

    int usprite_qwsize = num_sprites * 2 + 7;

    int lsprite_qwsize = num_sprites * 2 + 7;
    int total_qwsize = dmatag_qwsize + 12 + usprite_qwsize + lsprite_qwsize;

    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(total_qwsize * sizeof(u_long128)));

    uint32 *data_setup = data_start + dmatag_qwsize * 4;

    uint32 *data_lsprites = data_start + (dmatag_qwsize + 12 + usprite_qwsize) * 4;

    int target_address;

    if (data_start) {
        *reinterpret_cast<u_long *>(data_start) = MakeDmaTag(total_qwsize - 1, eDMA_NEXT);
        data_start[2] = SCE_VIF1_SET_FLUSHE(0);
        data_start[3] = SCE_VIF1_SET_DIRECT(total_qwsize - 1, 0);

        reinterpret_cast<u_long *>(data_setup)[0] = MakeGifTag(11, 1, 0, 0, GIF_PACKED, 1);
        reinterpret_cast<u_long *>(data_setup)[1] = SCE_GIF_PACKED_AD;

        reinterpret_cast<u_long *>(data_setup)[2] = 0;
        reinterpret_cast<u_long *>(data_setup)[3] = SCE_GS_TEXFLUSH;
        reinterpret_cast<u_long *>(data_setup)[4] = 0;
        reinterpret_cast<u_long *>(data_setup)[5] = SCE_GS_DTHE;
        reinterpret_cast<u_long *>(data_setup)[6] = 0;
        reinterpret_cast<u_long *>(data_setup)[7] = SCE_GS_FBA_1;
        reinterpret_cast<u_long *>(data_setup)[8] = SCE_GS_SET_TEXA(0, 0, 0x80);
        reinterpret_cast<u_long *>(data_setup)[9] = SCE_GS_TEXA;
        reinterpret_cast<u_long *>(data_setup)[10] = 0;
        reinterpret_cast<u_long *>(data_setup)[11] = SCE_GS_XYOFFSET_1;
        reinterpret_cast<u_long *>(data_setup)[12] = SCE_GS_SET_ZBUF(0, PSMZ16S, 1);
        reinterpret_cast<u_long *>(data_setup)[13] = SCE_GS_ZBUF_1;
        reinterpret_cast<u_long *>(data_setup)[14] = SCE_GS_SET_TEST(ATE_ON, ATST_NEVER, 0, AFAIL_FB_ONLY, DATE_OFF, 0, ZTE_ON, ZTST_ALWAYS);
        reinterpret_cast<u_long *>(data_setup)[15] = SCE_GS_TEST_1;
        reinterpret_cast<u_long *>(data_setup)[16] = SCE_GS_SET_ALPHA(2, 2, 2, 0, 128);
        reinterpret_cast<u_long *>(data_setup)[17] = SCE_GS_ALPHA_1;
        reinterpret_cast<u_long *>(data_setup)[18] = SCE_GS_SET_TEX1(1, 0, 0, 0, 0, 0, 0);
        reinterpret_cast<u_long *>(data_setup)[19] = SCE_GS_TEX1_1;
        reinterpret_cast<u_long *>(data_setup)[20] = SCE_GS_SET_SCISSOR(0, 511, 0, 511);
        reinterpret_cast<u_long *>(data_setup)[21] = SCE_GS_SCISSOR_1;
        reinterpret_cast<u_long *>(data_setup)[22] = SCE_GS_SET_CLAMP(1, 1, 0, 0, 0, 0);
        reinterpret_cast<u_long *>(data_setup)[23] = SCE_GS_CLAMP_1;

        target_address = srcVramAddress * 32;

        {
            // Detras del tag de DMA (1 quadword) y del bloque de registros (12).
            u_long *data = reinterpret_cast<u_long *>(data_start + 13 * 4);
            int sx0 = initialU;
            int dx0 = initialX;

            *data++ = MakeGifTag(5, 0, 0, 0, GIF_PACKED, 1);
            *data++ = SCE_GIF_PACKED_AD;
            *data++ = 0;
            *data++ = SCE_GS_TEXFLUSH;
            *data++ = SCE_GS_SET_FRAME(destVramAddress, frameWidth / 64, PSMCT16, 0x3FFF);
            *data++ = SCE_GS_FRAME_1;
            *data++ = SCE_GS_SET_TEX0(target_address, frameWidth / 64, PSMCT16, 9, 10, 1, 1, 0, 0, 0, 0, 0);
            *data++ = SCE_GS_TEX0_1;
            *data++ = PRIM_SPRITE | PRIM_TME | PRIM_FST;
            *data++ = SCE_GS_PRIM;
            *data++ = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
            *data++ = SCE_GS_RGBAQ;
            *data++ = MakeGifTag(num_sprites, 0, 0, 0, GIF_REGLIST, 4);
            *data++ = SCE_GS_UV | (SCE_GS_XYZ2 << 4) | (SCE_GS_UV << 8) | (SCE_GS_XYZ2 << 12);

            for (int i = 0; i < num_sprites; i++) {
                *data++ = SCE_GS_SET_UV(((sx0 << 4) + 8) & 0xFFFF, ((0 << 4) + 8) & 0xFFFF);
                *data++ = SCE_GS_SET_XYZ((dx0 << 4) & 0xFFFF, (0 << 4) & 0xFFFF, 0);
                *data++ = SCE_GS_SET_UV((((sx0 + 8) << 4) + 8) & 0xFFFF, ((frameHeight << 4) + 8) & 0xFFFF);
                *data++ = SCE_GS_SET_XYZ(((dx0 + 8) << 4) & 0xFFFF, (frameHeight << 4) & 0xFFFF, 0);

                sx0 += 16;
                dx0 += 16;
            }
        }

        {
            u_long *data = reinterpret_cast<u_long *>(data_lsprites);
            int sx0 = initialU;
            int dx0 = initialX;

            destVramAddress += (frameWidth * (frameHeight >> 1)) / 2048;
            target_address += (frameWidth * (frameHeight >> 1)) / 64;

            *data++ = MakeGifTag(5, 0, 0, 0, GIF_PACKED, 1);
            *data++ = SCE_GIF_PACKED_AD;
            *data++ = 0;
            *data++ = 0xF; // la segunda mitad no vacia la cache de texturas: escribe en la direccion 0xF, que no hace nada
            *data++ = SCE_GS_SET_FRAME(destVramAddress, frameWidth / 64, PSMCT16, 0x3FFF);
            *data++ = SCE_GS_FRAME_1;
            *data++ = SCE_GS_SET_TEX0(target_address, frameWidth / 64, PSMCT16, 9, 10, 1, 1, 0, 0, 0, 0, 0);
            *data++ = SCE_GS_TEX0_1;
            *data++ = PRIM_SPRITE | PRIM_TME | PRIM_FST;
            *data++ = SCE_GS_PRIM;
            *data++ = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
            *data++ = SCE_GS_RGBAQ;
            *data++ = MakeGifTag(num_sprites, 1, 0, 0, GIF_REGLIST, 4);
            *data++ = SCE_GS_UV | (SCE_GS_XYZ2 << 4) | (SCE_GS_UV << 8) | (SCE_GS_XYZ2 << 12);

            for (int i = 0; i < num_sprites; i++) {
                *data++ = SCE_GS_SET_UV(((sx0 << 4) + 8) & 0xFFFF, ((0 << 4) + 8) & 0xFFFF);
                *data++ = SCE_GS_SET_XYZ((dx0 << 4) & 0xFFFF, (0 << 4) & 0xFFFF, 0);
                *data++ = SCE_GS_SET_UV((((sx0 + 8) << 4) + 8) & 0xFFFF, ((frameHeight << 4) + 8) & 0xFFFF);
                *data++ = SCE_GS_SET_XYZ(((dx0 + 8) << 4) & 0xFFFF, (frameHeight << 4) & 0xFFFF, 0);

                sx0 += 16;
                dx0 += 16;
            }
        }

        dma_chain->Add(data_start);
    }
}

void MoveGtoA(int srcVramAddress, int destVramAddress, eDmaChain *dma_chain, int width, int height) {
    CopyGreenOrAlphaChannels(srcVramAddress, destVramAddress, dma_chain, 0, 8, width, height);
}

float debugColourGAIN = 0.5f;

// El bloom negro: una paleta de 256 entradas cuyo alfa sigue la curva BlackBloomCurve, que se sube a la VRAM y con la
// que se dibuja el render target reducido (leido como PSMT8H) sobre la pantalla en un sprite.
void IVisualTreatmentPlat::ApplyBlackBloom(eRenderTarget *screenTarget, eRenderTarget *downSample, eDmaChain *dma_chain) {
    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(19 * sizeof(u_long128)));
    uint32 curve[256];
    int paletteAddr;
    int baseOFFSET;
    int frame_page_address;
#ifndef EA_BUILD_A124
    int dx4;
#endif
    int dw4;
    int dh4;

    if (!data_start) {
        return;
    }

    for (int i = 0; i < 256; i++) {
        float blackCurve = GetValueFromSpline(i * (1.0f / 256.0f), &BlackBloomCurve);
        int alpha;

        // Como en ApplyPulseBrightness, el binario solo dice que bEqual compara dos constantes iguales.
        if (!bEqual(0.0f, 0.0f, 0.01f)) {
            alpha = 128;
        } else {
            alpha = static_cast<int>(((1.0f - BlackBloomIntensity) + blackCurve * BlackBloomIntensity) * 128.0f);
        }
        if (alpha > 255) {
            alpha = 255;
        }

        curve[i] = (alpha << 24) | 0x808080;
    }

    RemapLinearIntoClut8(reinterpret_cast<uint32 *>(BlackBloomPalette), curve);

    paletteAddr = eVRAM.GetArea(VRAM_RAMP_PALETTE_START, 1);
    AttachImageDataPacketDMA(dma_chain, BlackBloomPalette, paletteAddr / 256, 64, 0, 1, 16, 16, 1);

#ifdef EA_BUILD_A124
    baseOFFSET = screenTarget->ScissorY / 4;
#else
    // La version de venta ya no corre la base del FRAME: dibuja el sprite en la posicion del scissor.
    baseOFFSET = screenTarget->ScissorY;
#endif
    frame_page_address = screenTarget->FrameAddress;

    if (screenTarget->RelativeToFrameBuffer) {
        frame_page_address = MainDraw0Context->FrameAddress;
    }

#ifndef EA_BUILD_A124
    dx4 = screenTarget->ScissorX;
#endif
    dw4 = screenTarget->ScissorW;
    dh4 = screenTarget->ScissorH;

    sceGifPackAd *data = reinterpret_cast<sceGifPackAd *>(data_start);

    reinterpret_cast<u_long *>(data_start)[0] = MakeDmaTag(17, eDMA_CNT);
    data_start[2] = SCE_VIF1_SET_FLUSHE(0);
    data_start[3] = SCE_VIF1_SET_DIRECT(17, 0);
    data++;

    data->DATA = MakeGifTag(16, 1, 0, 0, GIF_PACKED, 1);
    data->ADDR = SCE_GIF_PACKED_AD;
    data++;

    data->DATA = 0;
    data->ADDR = SCE_GS_TEXFLUSH;
    data++;

    data->DATA = 0;
    data->ADDR = SCE_GS_XYOFFSET_1;
    data++;

    data->DATA = SCE_GS_SET_ALPHA(1, 2, 0, 2, 128);
    data->ADDR = SCE_GS_ALPHA_1;
    data++;

    data->DATA = SCE_GS_SET_SCISSOR(screenTarget->ScissorX, screenTarget->ScissorX + screenTarget->ScissorW,
                                    screenTarget->ScissorY, screenTarget->ScissorY + screenTarget->ScissorH);
    data->ADDR = SCE_GS_SCISSOR_1;
    data++;

    data->DATA = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
    data->ADDR = SCE_GS_TEST_1;
    data++;

#ifdef EA_BUILD_A124
    data->DATA = SCE_GS_SET_FRAME(frame_page_address + baseOFFSET, screenTarget->FrameWidth >> 6, 0, 0xFF000000);
#else
    data->DATA = SCE_GS_SET_FRAME(frame_page_address, screenTarget->FrameWidth >> 6, 0, 0xFF000000);
#endif
    data->ADDR = SCE_GS_FRAME_1;
    data++;

    data->DATA = SCE_GS_SET_ZBUF(screenTarget->ZBufferAddress, screenTarget->ZBufferMode, 1);
    data->ADDR = SCE_GS_ZBUF_1;
    data++;

    data->DATA = SCE_GS_SET_TEX0(downSample->FrameAddress << 5, downSample->FrameWidth >> 6, PSMT8H, 9, 9, 1, 1,
                                 paletteAddr / 256, 0, 0, 0, 1);
    data->ADDR = SCE_GS_TEX0_1;
    data++;

    data->DATA = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
    data->ADDR = SCE_GS_TEX1_1;
    data++;

    data->DATA = SCE_GS_SET_CLAMP(1, 1, 0, 0, 0, 0);
    data->ADDR = SCE_GS_CLAMP_1;
    data++;

    data->DATA = PRIM_SPRITE | PRIM_TME | PRIM_ABE | PRIM_FST;
    data->ADDR = SCE_GS_PRIM;
    data++;

    data->DATA = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
    data->ADDR = SCE_GS_RGBAQ;
    data++;

    data->DATA = 0;
    data->ADDR = SCE_GS_UV;
    data++;

#ifdef EA_BUILD_A124
    data->DATA = 0;
#else
    data->DATA = SCE_GS_SET_XYZ(dx4 << 4, baseOFFSET << 4, 0);
#endif
    data->ADDR = SCE_GS_XYZ2;
    data++;

    data->DATA = SCE_GS_SET_UV(downSample->ScissorW << 4, downSample->ScissorH << 4);
    data->ADDR = SCE_GS_UV;
    data++;

#ifdef EA_BUILD_A124
    data->DATA = SCE_GS_SET_XYZ(dw4 << 4, dh4 << 4, 0);
#else
    data->DATA = SCE_GS_SET_XYZ((dx4 + dw4) << 4, (baseOFFSET + dh4) << 4, 0);
#endif
    data->ADDR = SCE_GS_XYZ2;

    dma_chain->Add(data_start);
}

int tcc = 1;
int tfx = 0;
int tw = 9;
int th = 9;
float colorAlpha = 128.0f;
int cbA = 1;
int cbB = 2;
int cbC = 0;
int cbD = 1;
int cbFix = 0x80;
int QuickTint = 0;
#ifndef EA_BUILD_A124
// Solo en la version de venta (en su .data, entre QuickTint y qtintA); SLUS no trae simbolos: el nombre es nuestro.
int ColorBloomTintOnly = 0;
#endif

// El bloom de color: un tinte de la pantalla sobre si misma (salvo con QuickTint) y despues el render target reducido,
// leido como PSMT8H a traves de una paleta cuyo alfa sigue la curva ColourBloomCurve, sumado encima.
void IVisualTreatmentPlat::ApplyColorBloom(eRenderTarget *screenTarget, eRenderTarget *downSample, eDmaChain *dma_chain) {
    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(16 * sizeof(u_long128)));
    int baseOFFSET;
    int frame_page_address;
    float cbInt;
    int r;
    int g;
    int b;
    uint32 tintA;
    uint32 tintB;
    uint32 tintC;
    uint32 tintD;
    uint32 tintFix;
    uint32 curve[256];
    int paletteAddr;
#ifndef EA_BUILD_A124
    int dx4;
#endif
    int dw4;
    int dh4;

    if (!data_start) {
        return;
    }

#ifdef EA_BUILD_A124
    baseOFFSET = screenTarget->ScissorY / 4;
#endif
    frame_page_address = screenTarget->FrameAddress;

    if (screenTarget->RelativeToFrameBuffer) {
        frame_page_address = MainDraw0Context->FrameAddress;
    }
#ifdef EA_BUILD_A124
    frame_page_address += baseOFFSET;
#endif

    // Como en ApplyPulseBrightness, el binario solo dice que bEqual compara dos constantes iguales.
    if (!bEqual(0.0f, 0.0f, 0.01f)) {
        cbInt = 0.0f;
    } else {
        cbInt = ColourBloomIntensity;
    }

    r = static_cast<int>(ColourBloomTint.x * 255.0f);
    g = static_cast<int>(ColourBloomTint.y * 255.0f);
    b = static_cast<int>(ColourBloomTint.z * 255.0f);

    tintA = 0;
    tintB = 2;
    tintC = 2;
    tintD = 1;
    tintFix = static_cast<int>(cbInt * colorAlpha);

    if (!QuickTint) {
        eAttachTargetMoveSprites(dma_chain, screenTarget, screenTarget, 8, 8, 8, 8,
                                 tintA | (tintB << 2) | (tintC << 4) | (tintD << 6), tintFix, 1, r, g, b, 0, 0,
                                 0x30000);
    }

#ifndef EA_BUILD_A124
    // La version de venta puede quedarse solo con el tinte.
    if (ColorBloomTintOnly) {
        return;
    }
#endif

    for (int i = 0; i < 256; i++) {
        int alpha = static_cast<int>(GetValueFromSpline(i * (1.0f / 256.0f), &ColourBloomCurve) * cbInt * colorAlpha);

        if (alpha > 255) {
            alpha = 255;
        }

        curve[i] = (alpha << 24) | 0x808080;
    }

    RemapLinearIntoClut8(reinterpret_cast<uint32 *>(ColorBloomPalette), curve);

    paletteAddr = eVRAM.GetArea(VRAM_RAMP_PALETTE_START, 1);
    AttachImageDataPacketDMA(dma_chain, ColorBloomPalette, paletteAddr / 256, 64, 0, 1, 16, 16, 1);

#ifndef EA_BUILD_A124
    // La version de venta dibuja el sprite en la posicion del scissor en vez de correr la base del FRAME.
    dx4 = screenTarget->ScissorX;
    baseOFFSET = screenTarget->ScissorY;
#endif
    dw4 = screenTarget->ScissorW;
    dh4 = screenTarget->ScissorH;

    sceGifPackAd *data = reinterpret_cast<sceGifPackAd *>(data_start);

    reinterpret_cast<u_long *>(data_start)[0] = MakeDmaTag(15, eDMA_CNT);
    data_start[2] = SCE_VIF1_SET_FLUSHE(0);
    data_start[3] = SCE_VIF1_SET_DIRECT(15, 0);
    data++;

    data->DATA = MakeGifTag(14, 1, 0, 0, GIF_PACKED, 1);
    data->ADDR = SCE_GIF_PACKED_AD;
    data++;

    data->DATA = 0;
    data->ADDR = SCE_GS_TEXFLUSH;
    data++;

    data->DATA = SCE_GS_SET_SCISSOR(screenTarget->ScissorX, screenTarget->ScissorX + screenTarget->ScissorW,
                                    screenTarget->ScissorY, screenTarget->ScissorY + screenTarget->ScissorH);
    data->ADDR = SCE_GS_SCISSOR_1;
    data++;

    data->DATA = 0;
    data->ADDR = SCE_GS_XYOFFSET_1;
    data++;

    data->DATA = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
    data->ADDR = SCE_GS_TEST_1;
    data++;

    data->DATA = SCE_GS_SET_FRAME(frame_page_address, screenTarget->FrameWidth >> 6, 0, 0xFF000000);
    data->ADDR = SCE_GS_FRAME_1;
    data++;

    data->DATA = SCE_GS_SET_ZBUF(screenTarget->ZBufferAddress, screenTarget->ZBufferMode, 1);
    data->ADDR = SCE_GS_ZBUF_1;
    data++;

    data->DATA = SCE_GS_SET_ALPHA(cbA, cbB, cbC, cbD, cbFix);
    data->ADDR = SCE_GS_ALPHA_1;
    data++;

    data->DATA = SCE_GS_SET_TEX0(downSample->FrameAddress << 5, downSample->FrameWidth >> 6, PSMT8H, tw, th, tcc, tfx,
                                 paletteAddr / 256, 0, 0, 0, 1);
    data->ADDR = SCE_GS_TEX0_1;
    data++;

    data->DATA = PRIM_SPRITE | PRIM_TME | PRIM_ABE | PRIM_FST;
    data->ADDR = SCE_GS_PRIM;
    data++;

    if (QuickTint) {
        data->DATA = SCE_GS_SET_RGBAQ(r, g, b, 0x80, 0x3F800000);
    } else {
        data->DATA = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
    }
    data->ADDR = SCE_GS_RGBAQ;
    data++;

    data->DATA = 0;
    data->ADDR = SCE_GS_UV;
    data++;

#ifdef EA_BUILD_A124
    data->DATA = 0;
#else
    data->DATA = SCE_GS_SET_XYZ(dx4 << 4, baseOFFSET << 4, 0);
#endif
    data->ADDR = SCE_GS_XYZ2;
    data++;

    data->DATA = SCE_GS_SET_UV(downSample->ScissorW << 4, downSample->ScissorH << 4);
    data->ADDR = SCE_GS_UV;
    data++;

#ifdef EA_BUILD_A124
    data->DATA = SCE_GS_SET_XYZ(dw4 << 4, dh4 << 4, 0);
#else
    data->DATA = SCE_GS_SET_XYZ((dx4 + dw4) << 4, (baseOFFSET + dh4) << 4, 0);
#endif
    data->ADDR = SCE_GS_XYZ2;

    dma_chain->Add(data_start);
}

uint32 qtintA = 1;
uint32 qtintB = 0;
uint32 qtintC = 2;
uint32 qtintD = 1;
uint32 qtintFix = 0x80;

void IVisualTreatmentPlat::ApplyDetailMap(eRenderTarget *screenTarget, eRenderTarget *detailMap, eDmaChain *dma_chain) {
    if (DetailMapIntensity < 0.001f) {
        return;
    }

    eAttachDownsample(dma_chain, detailMap, screenTarget, 0);

    uint32 tintFix = static_cast<int>(DetailMapIntensity * 255.0f);

    if (tintFix > 255) {
        tintFix = 255;
    }

    eAttachTargetMoveSprites(dma_chain, screenTarget, detailMap, 8, 8, 8, 8,
                             qtintA | (qtintB << 2) | (qtintC << 4) | (qtintD << 6), tintFix, 1, 128, 128, 128, 128,
                             0, 0x30000);
}

uint32 TtintA = 0;
uint32 TtintB = 2;
uint32 TtintC = 2;
uint32 TtintD = 1;
uint32 Tr = 0x80;
uint32 Tg = 0x80;
uint32 Tb = 0x80;
uint32 Ta = 0x80;

void IVisualTreatmentPlat::ApplyPulseBrightness(eRenderTarget *screenTarget, eDmaChain *dma_chain) {
    if (PulseBrightness < 0.01f) {
        return;
    }

    int alpha;

    // El binario solo dice que los dos primeros argumentos son constantes iguales: bAbs recibe 0.0f.
    if (bEqual(0.0f, 0.0f, 0.01f)) {
        if (PulseBrightness < 1.0f) {
            alpha = static_cast<int>(PulseBrightness * 255.0f);
        } else {
            alpha = 255;
        }
    } else {
        alpha = 0;
    }

    uint32 tintA = TtintA;
    uint32 tintB = TtintB;
    uint32 tintC = TtintC;
    uint32 tintD = TtintD;

    uint32 r = Tr;
    uint32 g = Tg;
    uint32 b = Tb;
    uint32 a = Ta;

    eAttachTargetMoveSprites(dma_chain, screenTarget, screenTarget, 8, 8, 8, 8,
                             tintA | (tintB << 2) | (tintC << 4) | (tintD << 6), alpha, 1, r, g, b, a, 0, 0x30000);
}

long rbZ = 0xFFFFFF;
int rbW = 512;
int rbU0 = 0;
int rbV0 = 0;
int rbU1 = 128;
int rbV1 = 128;
int rbTBW = 64;
int rbAlpha = 128;

// El blur radial: la mascara RADIALMASKALPHA (una textura de 8 bits con su paleta) se sube a la VRAM y se dibuja sobre
// la pantalla en 16 tiras de 32 pixeles que solo escriben el alfa, y despues un sprite que mezcla la pantalla con ese
// alfa. Devuelve si se ha dibujado.
unsigned int IVisualTreatmentPlat::Radialblur(eRenderTarget *screenTarget, eDmaChain *dma_chain) {
    uint32 *data_start;
    eRenderTarget *disp_target;
    int frame_pageAddress;
    eRenderTarget *draw_target;
    int zbuffer_page_address;
    int zbuffer_mode;
    TextureInfo *radialmask;
    int textureAddr;
    int paletteAddr;
    uint32 *data;
    int dy4;
    int rbH;

    // Como en ApplyPulseBrightness, el binario solo dice que bEqual compara dos constantes iguales.
    if (!bEqual(0.0f, 0.0f, 0.01f)) {
        rbAlpha = 0;
    } else if (RadialBlur < 1.0f) {
        rbAlpha = static_cast<int>(RadialBlur * 255.0f);
    } else {
        rbAlpha = 255;
    }

    if (!rbAlpha) {
        return 0;
    }

    data_start = static_cast<uint32 *>(eFrameMalloc(50 * sizeof(u_long128)));

    if (!data_start) {
        return 0;
    }

    disp_target = MainDisp0Context;
    frame_pageAddress = disp_target->FrameAddress;
    draw_target = MainDraw0Context;
    zbuffer_page_address = draw_target->ZBufferAddress;
    zbuffer_mode = draw_target->ZBufferMode;

    radialmask = GetTextureInfo(bStringHash("RADIALMASKALPHA"), 0, 0);

    textureAddr = eVRAM.GetArea(VRAM_128_A, 1);
    paletteAddr = eVRAM.GetArea(VRAM_RAMP_PALETTE_START, 1);

    AttachImageDataPacketDMA(dma_chain, radialmask->ImageData, textureAddr / 256,
                             (radialmask->Width * radialmask->Height) / 16, 0, rbTBW >> 6, 64, 64, 1);
    AttachImageDataPacketDMA(dma_chain, radialmask->PaletteData, paletteAddr / 256, 64, 0, 1, 16, 16, 1);

    dy4 = screenTarget->ScissorY;
    rbH = screenTarget->ScissorH;

    data = data_start;

    *reinterpret_cast<u_long *>(data) = MakeDmaTag(49, eDMA_CNT);
    data[2] = SCE_VIF1_SET_FLUSHE(0);
    data[3] = SCE_VIF1_SET_DIRECT(49, 0);
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = MakeGifTag(48, 1, 0, 0, GIF_PACKED, 1);
    reinterpret_cast<u_long *>(data)[1] = SCE_GIF_PACKED_AD;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = 0;
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEXFLUSH;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = 0;
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYOFFSET_1;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_SCISSOR(screenTarget->ScissorX, screenTarget->ScissorX + screenTarget->ScissorW,
                                                             screenTarget->ScissorY, screenTarget->ScissorY + screenTarget->ScissorH);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_SCISSOR_1;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEST_1;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_ZBUF(zbuffer_page_address, zbuffer_mode, 1);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_ZBUF_1;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_ALPHA(2, 2, 2, 0, 128);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_ALPHA_1;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_FRAME(frame_pageAddress, 8, 0, 0xFFFFFF);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_FRAME_1;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = PRIM_SPRITE;
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_PRIM;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_RGBAQ(0, 0, 0, 0xFE, 0x3F800000);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
    data += 4;

    {
        int dx0 = 0;

        for (int i = 0; i < 16; i++) {
            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ((dx0 << 4) & 0xFFFF, (dy4 << 4) & 0xFFFF, 0);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(((dx0 + 32) << 4) & 0xFFFF, ((dy4 + rbH) << 4) & 0xFFFF, 0);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
            data += 4;

            dx0 += 32;
        }
    }

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_TEX0(textureAddr / 256, radialmask->Width >> 6, PSMT8, radialmask->ShiftWidth,
                                                          radialmask->ShiftHeight, 1, 0, paletteAddr / 256, 0, 0, 0, 1);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEX0_1;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = PRIM_SPRITE | PRIM_TME | PRIM_FST;
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_PRIM;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, rbAlpha, 0x3F800000);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV(rbU0, rbV0);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(0 << 4, dy4 << 4, rbZ);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV(rbU1 << 4, rbV1 << 4);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
    data += 4;

    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(rbW << 4, (dy4 + rbH) << 4, rbZ);
    reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;

    dma_chain->Add(data_start);

    return 1;
}

void IVisualTreatmentPlat::RenderMWVisualLook(eView *view, eDmaChain *dma_chain, eDmaChain *tex_chain) {
    if (gApplyMWVisualLook) {
        eRenderTarget *render_target = view->GetRenderTarget();
        eRenderTarget *quarterTarget = eGetRenderTarget(9);

        eAttachTargetMoveSprites(dma_chain, quarterTarget, render_target, 0, 0, 0, 0, 100, 128, 0, 128, 128, 128, 128,
                                 0, 0x30000);

        MoveGtoA(quarterTarget->FrameAddress, quarterTarget->FrameAddress, dma_chain, quarterTarget->ScissorW,
                 quarterTarget->ScissorH);

        ApplyColorBloom(render_target, quarterTarget, dma_chain);

        ApplyBlackBloom(render_target, quarterTarget, dma_chain);

        ApplyDetailMap(render_target, quarterTarget, dma_chain);

        ApplyPulseBrightness(render_target, dma_chain);
    }
}
