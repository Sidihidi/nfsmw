#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/INIS.h"
#include "Speed/Indep/Src/World/FacePixelate.hpp"
#include "Speed/Indep/Src/World/Rain.hpp"
#include "Speed/Indep/Src/World/ScreenEffects.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/PSX2/Inc/DmaE.hpp"
#include "Speed/PSX2/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/PSX2/bWare/Src/ee/include/eestruct.h"

// psx2/src/ecstasy/eEnvEffectsE.cpp de la A124: los efectos de pantalla de PS2 (niebla, profundidad de campo, blur,
// bloom, smear...) hechos con paletas de 256 entradas que se suben a la VRAM. Aqui solo estan las funciones
// pequenas; los efectos grandes quedan por escribir.

// Las paletas son globales sin inicializar: sus stabs salen al final de la unidad.
u_long128 FogPalette[3][2][64];
u_long128 AlphaPalette[3][64];
u_long128 FocusPalettes[3][64];

// Sube una imagen a la VRAM por el PATH2: un paquete de registros (BITBLTBUF, TRXPOS, TRXREG, TRXDIR) y un tag REF
// que manda la imagen directamente desde donde esta.
void AttachImageDataPacketDMA(eDmaChain *dma_chain, void *image_data, unsigned int vram_address, unsigned int qwsize,
                              unsigned int psm, unsigned int tbw, unsigned int width, unsigned int height, int eop) {
    DATA16 *data_start = static_cast<DATA16 *>(eFrameMalloc(9 * sizeof(DATA16)));

    if (data_start) {
        DATA16 *data;

        data_start[0].w[0] = (eDMA_CNT << 28) | 6;
        data_start[0].w[1] = 0;
        data_start[0].w[2] = 0;
        data_start[0].w[3] = SCE_VIF1_SET_DIRECT(6, 0);

        data_start[1].d[0] = SCE_GIF_SET_TAG(4, 0, 0, 0, GIF_PACKED, 1);
        data_start[1].d[1] = SCE_GIF_PACKED_AD;

        data_start[2].d[0] = SCE_GS_SET_BITBLTBUF(0, 0, 0, vram_address, tbw, psm);
        data_start[2].d[1] = SCE_GS_BITBLTBUF;

        data_start[3].d[0] = 0;
        data_start[3].d[1] = SCE_GS_TRXPOS;

        data_start[4].d[0] = SCE_GS_SET_TRXREG(width, height);
        data_start[4].d[1] = SCE_GS_TRXREG;

        data_start[5].d[0] = 0;
        data_start[5].d[1] = SCE_GS_TRXDIR;

        data_start[6].d[0] = SCE_GIF_SET_TAG(qwsize, eop, 0, 0, GIF_IMAGE, 0);
        data_start[6].d[1] = 0;

        data = &data_start[7];
        data->w[0] = qwsize | (eDMA_REF << 28);
        data->w[1] = reinterpret_cast<unsigned int>(image_data);
        data->w[2] = 0;
        data->w[3] = SCE_VIF1_SET_DIRECT(qwsize, 0);
        data = &data_start[8];

        data->d[0] = MakeDmaTag(0, eDMA_NEXT);
        data->w[2] = 0;
        data->w[3] = 0;

        dma_chain->Add(data_start, data);
    }
}

// Reordena una paleta lineal de 256 colores en el orden de CLUT de 8 bits del GS: en cada bloque de 32 entradas se
// cambian de sitio las filas 8-15 y 16-23.
void RemapLinearIntoClut8(uint32 *output, uint32 *input) {
    for (int j = 0; j < 8; j++) {
        u_long128 input_00_03 = reinterpret_cast<u_long128 *>(input)[0];
        u_long128 input_04_07 = reinterpret_cast<u_long128 *>(input)[1];
        u_long128 input_08_11 = reinterpret_cast<u_long128 *>(input)[2];
        u_long128 input_12_15 = reinterpret_cast<u_long128 *>(input)[3];
        u_long128 input_16_19 = reinterpret_cast<u_long128 *>(input)[4];
        u_long128 input_20_23 = reinterpret_cast<u_long128 *>(input)[5];
        u_long128 input_24_27 = reinterpret_cast<u_long128 *>(input)[6];
        u_long128 input_28_31 = reinterpret_cast<u_long128 *>(input)[7];

        reinterpret_cast<u_long128 *>(output)[0] = input_00_03;
        reinterpret_cast<u_long128 *>(output)[1] = input_04_07;
        reinterpret_cast<u_long128 *>(output)[2] = input_16_19;
        reinterpret_cast<u_long128 *>(output)[3] = input_20_23;
        reinterpret_cast<u_long128 *>(output)[4] = input_08_11;
        reinterpret_cast<u_long128 *>(output)[5] = input_12_15;
        reinterpret_cast<u_long128 *>(output)[6] = input_24_27;
        reinterpret_cast<u_long128 *>(output)[7] = input_28_31;
        input += 32;
        output += 32;
    }
}

// Los ajustes del paso de la profundidad al alfa, con los valores del .data de la A124.
int diaNumSprites = 0;
unsigned int diaFrameMask = 0x3FFF;
int diaHeight = 512;
int EnableDepthIntoAlpha = 1;
int CopyPrimTME = 1;
int CopyPrim = PRIM_SPRITE | PRIM_FST;
int CopyR = 128;
int CopyG = 128;
int CopyB = 128;
int CopyA = 128;
int FogInner = 0;
float FogstartMod = 1.0f;
unsigned int alfaClear = 0xFE;

// Pasa la profundidad al alfa de la pantalla: lee el z-buffer (PSMZ16) como textura y lo copia en columnas de 8 pixeles
// desplazadas sobre un render target de 16 bits, en dos mitades de altura; borra el alfa de la pantalla con alfaClear y
// copia el resultado encima solo en el alfa.
int eAttachDepthIntoAlpha(eDmaChain *dma_chain) {
    if (!EnableDepthIntoAlpha) {
        return 0;
    }

    eRenderTarget *draw_target = MainDraw0Context;
    eRenderTarget *disp_target = MainDisp0Context;
    int num_sprites = diaNumSprites;

    if (!num_sprites) {
        num_sprites = 64;
    }

    int dmatag_qwsize = 1;
    int usprite_qwsize = num_sprites * 2 + 7;
    int lsprite_qwsize = num_sprites * 2 + 7;
    int total_qwsize = dmatag_qwsize + 12 + usprite_qwsize + lsprite_qwsize + 21 + 40;

    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(total_qwsize * sizeof(u_long128)));
    uint32 *data_setup = data_start + dmatag_qwsize * 4;
    uint32 *data_lsprites = data_start + (dmatag_qwsize + 12 + usprite_qwsize) * 4;
    uint32 *data_csprites = data_start + (dmatag_qwsize + usprite_qwsize + 12 + lsprite_qwsize) * 4;
    uint32 *data_asprites = data_start + (dmatag_qwsize + 12 + usprite_qwsize + lsprite_qwsize + 21) * 4;
    int sprite_prim;

    if (!data_start) {
        return 0;
    }

    *reinterpret_cast<u_long *>(data_start) = MakeDmaTag(total_qwsize - 1, eDMA_NEXT);
    data_start[2] = SCE_VIF1_SET_FLUSHE(0);
    data_start[3] = SCE_VIF1_SET_DIRECT(total_qwsize - 1, 0);

    reinterpret_cast<u_long *>(data_setup)[0] = MakeGifTag(11, 0, 0, 0, GIF_PACKED, 1);
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

    sprite_prim = CopyPrim;

    if (CopyPrimTME) {
        sprite_prim |= PRIM_TME;
    }

    {
        // La mitad de arriba: el z-buffer, leido como PSMZ16 en columnas de 8 pixeles, a un render target de 16 bits.
        u_long *data = reinterpret_cast<u_long *>(data_start + 13 * 4);
        int frame_page_address = draw_target->FrameAddress;
        int sx0 = 0;
        int dx0 = 8;
        int h;

        *data++ = MakeGifTag(5, 0, 0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;
        *data++ = SCE_GS_TEXFLUSH;
        *data++ = SCE_GS_SET_FRAME(frame_page_address, 8, PSMCT16, diaFrameMask);
        *data++ = SCE_GS_FRAME_1;
        *data++ = SCE_GS_SET_TEX0(draw_target->ZBufferAddress * 32, 8, PSMZ16, 9, 10, 1, 1, 0, 0, 0, 0, 0);
        *data++ = SCE_GS_TEX0_1;
        *data++ = sprite_prim;
        *data++ = SCE_GS_PRIM;
        *data++ = SCE_GS_SET_RGBAQ(CopyR, CopyG, CopyB, CopyA, 0x3F800000);
        *data++ = SCE_GS_RGBAQ;
        *data++ = MakeGifTag(num_sprites, 0, 0, 0, GIF_REGLIST, 4);
        *data++ = SCE_GS_UV | (SCE_GS_XYZ2 << 4) | (SCE_GS_UV << 8) | (SCE_GS_XYZ2 << 12);

        h = diaHeight;

        for (int i = 0; i < num_sprites; i++) {
            *data++ = SCE_GS_SET_UV(((sx0 << 4) + 8) & 0xFFFF, ((0 << 4) + 8) & 0xFFFF);
            *data++ = SCE_GS_SET_XYZ((dx0 << 4) & 0xFFFF, (0 << 4) & 0xFFFF, 0);
            *data++ = SCE_GS_SET_UV((((sx0 + 8) << 4) + 8) & 0xFFFF, ((h << 4) + 8) & 0xFFFF);
            *data++ = SCE_GS_SET_XYZ(((dx0 + 8) << 4) & 0xFFFF, (h << 4) & 0xFFFF, 0);

            sx0 += 16;
            dx0 += 16;
        }
    }

    {
        // La mitad de abajo, 256 lineas mas alla en el z-buffer y en el render target.
        u_long *data = reinterpret_cast<u_long *>(data_lsprites);
        int sx0 = 0;
        int dx0 = 8;
        int h;

        *data++ = MakeGifTag(5, 0, 0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;
        *data++ = 0xF;
        *data++ = SCE_GS_SET_FRAME(draw_target->FrameAddress + 64, 8, PSMCT16, diaFrameMask);
        *data++ = SCE_GS_FRAME_1;
        *data++ = SCE_GS_SET_TEX0(draw_target->ZBufferAddress * 32 + 2048, 8, PSMZ16, 9, 10, 1, 1, 0, 0, 0, 0, 0);
        *data++ = SCE_GS_TEX0_1;
        *data++ = sprite_prim;
        *data++ = SCE_GS_PRIM;
        *data++ = SCE_GS_SET_RGBAQ(CopyR, CopyG, CopyB, CopyA, 0x3F800000);
        *data++ = SCE_GS_RGBAQ;
        *data++ = MakeGifTag(num_sprites, 0, 0, 0, GIF_REGLIST, 4);
        *data++ = SCE_GS_UV | (SCE_GS_XYZ2 << 4) | (SCE_GS_UV << 8) | (SCE_GS_XYZ2 << 12);

        h = diaHeight;

        for (int i = 0; i < num_sprites; i++) {
            *data++ = SCE_GS_SET_UV(((sx0 << 4) + 8) & 0xFFFF, ((0 << 4) + 8) & 0xFFFF);
            *data++ = SCE_GS_SET_XYZ((dx0 << 4) & 0xFFFF, (0 << 4) & 0xFFFF, 0);
            *data++ = SCE_GS_SET_UV((((sx0 + 8) << 4) + 8) & 0xFFFF, ((h << 4) + 8) & 0xFFFF);
            *data++ = SCE_GS_SET_XYZ(((dx0 + 8) << 4) & 0xFFFF, (h << 4) & 0xFFFF, 0);

            sx0 += 16;
            dx0 += 16;
        }
    }

    {
        // Borra el alfa de la pantalla con alfaClear (la mascara de FRAME solo deja pasar el alfa).
        u_long *data = reinterpret_cast<u_long *>(data_csprites);
        int dx0 = 0;
        int h;

        *data++ = MakeGifTag(3, 0, 0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = SCE_GS_SET_FRAME(disp_target->FrameAddress, 8, 0, 0xFFFFFF);
        *data++ = SCE_GS_FRAME_1;
        *data++ = PRIM_SPRITE;
        *data++ = SCE_GS_PRIM;
        *data++ = SCE_GS_SET_RGBAQ(0, 0, 0, alfaClear, 0x3F800000);
        *data++ = SCE_GS_RGBAQ;
        *data++ = MakeGifTag(16, 0, 0, 0, GIF_REGLIST, 2);
        *data++ = SCE_GS_XYZ2 | (SCE_GS_XYZ2 << 4);

        h = diaHeight;

        for (int i = 0; i < 16; i++) {
            *data++ = SCE_GS_SET_XYZ((dx0 << 4) & 0xFFFF, (0 << 4) & 0xFFFF, 0);
            *data++ = SCE_GS_SET_XYZ(((dx0 + 32) << 4) & 0xFFFF, (h << 4) & 0xFFFF, 0);

            dx0 += 32;
        }
    }

    {
        // Y copia encima la imagen de 16 bits leida como PSMCT32, que solo escribe el alfa donde pasa la prueba de z.
        u_long *data = reinterpret_cast<u_long *>(data_asprites);
        int dx0 = 0;
        int h;

        *data++ = MakeGifTag(6, 0, 0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = SCE_GS_SET_FRAME(disp_target->FrameAddress, 8, 0, 0xFFFFFF);
        *data++ = SCE_GS_FRAME_1;
        *data++ = SCE_GS_SET_ZBUF(draw_target->ZBufferAddress, draw_target->ZBufferMode, 1);
        *data++ = SCE_GS_ZBUF_1;
        *data++ = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, ZTE_ON, ZTST_GEQUAL);
        *data++ = SCE_GS_TEST_1;
        *data++ = SCE_GS_SET_TEX0(draw_target->FrameAddress * 32, 8, PSMCT32, 9, 9, 1, 1, 0, 0, 0, 0, 0);
        *data++ = SCE_GS_TEX0_1;
        *data++ = sprite_prim;
        *data++ = SCE_GS_PRIM;
        *data++ = SCE_GS_SET_RGBAQ(CopyR, CopyG, CopyB, CopyA, 0x3F800000);
        *data++ = SCE_GS_RGBAQ;
        *data++ = MakeGifTag(16, 1, 0, 0, GIF_REGLIST, 4);
        *data++ = SCE_GS_UV | (SCE_GS_XYZ2 << 4) | (SCE_GS_UV << 8) | (SCE_GS_XYZ2 << 12);

        h = diaHeight;

        for (int i = 0; i < 16; i++) {
            *data++ = SCE_GS_SET_UV(((dx0 << 4) + 8) & 0xFFFF, ((0 << 4) + 8) & 0xFFFF);
            *data++ = SCE_GS_SET_XYZ((dx0 << 4) & 0xFFFF, (0 << 4) & 0xFFFF, 0xFFFF);
            *data++ = SCE_GS_SET_UV((((dx0 + 32) << 4) + 8) & 0xFFFF, ((h << 4) + 8) & 0xFFFF);
            *data++ = SCE_GS_SET_XYZ(((dx0 + 32) << 4) & 0xFFFF, (h << 4) & 0xFFFF, 0xFFFF);

            dx0 += 32;
        }
    }

    dma_chain->Add(data_start);

    return 1;
}

uint32 *GetFogPalette(eView *view, int buffer) {
    int palette_index = view->GetID() - 1;

    if (palette_index < 0 || palette_index > 3) {
        return 0;
    }
    return reinterpret_cast<uint32 *>(FogPalette[palette_index][buffer]);
}

uint32 *GetAlphaPalette(eView *view) {
    int palette_index = view->GetID() - 1;

    if (palette_index < 0 || palette_index > 3) {
        return 0;
    }
    return reinterpret_cast<uint32 *>(AlphaPalette[palette_index]);
}

enum FogPalleteModes {
    FOG_NORMAL_P = 0,
    FOG_NIGHT_P = 1,
    FOG_DEPTH_P = 2,
    FOG_SINGLE_P = 3,
    FOG_VOLUME_P = 4,
    FOG_HEADLIGHT_P = 5,
    FOG_NUM_MODES = 6,
};

// FogVolumeType (los tipos de niebla de eAttachFog) esta en EcstasyE.hpp: tambien lo usa eDisplayFrame.

// La mezcla de cada modo de paleta (el ALPHA del GS) y sus valores de inicio, fin y color; los dos ultimos modos
// van a cero en el .data de la A124.
struct FogModeInfo {
    uint8 AlphaA;
    uint8 AlphaB;
    uint8 AlphaC;
    uint8 AlphaD;
    uint8 AlphaF;
    float Start;
    float End;
    float R;
    float G;
    float B;
    float A;
};

FogModeInfo FogModeInfos[FOG_NUM_MODES] = {
    {0, 1, 0, 1, 128, 244.0f, 255.0f, 128.0f, 128.0f, 192.0f, 32.0f},
    {1, 0, 0, 2, 128, 192.0f, 255.0f, 128.0f, 128.0f, 128.0f, 192.0f},
    {0, 1, 0, 1, 128, 0.0f, 255.0f, 128.0f, 128.0f, 128.0f, 128.0f},
    {0, 1, 0, 1, 128, 0.0f, 255.0f, 128.0f, 128.0f, 128.0f, 128.0f},
};

FogPalleteModes FogMode1 = FOG_NORMAL_P;
FogPalleteModes FogMode2 = FOG_NORMAL_P;
int DrawDistantFog = 1;

float FogPower = 4.0f;
float fogVPstart = 255.0f;
float fogVPend = 0.0f;
float headpower = 4.0f;

// La rampa de alfa de la niebla: solo se rehace si cambian el sesgo o la paleta.
void CreateAlphaPalette(uint32 *palette, float bias) {
    static float oBias = 9999.0f;
    static uint32 *oPalette = 0;

    if (oBias == bias && palette == oPalette) {
        return;
    }
    oBias = bias;
    oPalette = palette;

    uint32 *tp = palette;

    for (int i = 0; i < 256; i++) {
        int alf = static_cast<unsigned int>(i * bias);

        if (alf > 255) {
            alf = 255;
        }
        *tp++ = (alf << 24) | 0x808080;
    }
}

// n elevado a p con los bits del flotante: n^p ~ p * bits(n) + (1 - p) * bits(1). El original lo tiene en linea en
// DmaE.hpp (lineas 147-154, con las locales `output' y `magic' de los stabs); el nombre es nuestro.
// asm de EA (aprobado el 2026-10-02), como en indep_fpow: cvt.s.w sobre el max.s y cvt.w.s al devolver, sin pasar por
// un registro entero.
inline float FastPow(float n, float p) {
    float magic = FloatAsInteger(1.0f) - p * 1065353216.0f;

    n = bMax(n, 0.01f);

    float output;
    asm("cvt.s.w %0, %1" : "=f"(output) : "f"(n));
    output *= p;
    output += magic;

    asm("cvt.w.s %0, %1" : "=f"(output) : "f"(output));
    return output;
}

extern int FogControlOverRide;

// La paleta de niebla de la vista: una rampa de alfa (de start a end, con el modo de la vista si la niebla esta
// forzada) y el color de la niebla, en el orden de la CLUT de 8 bits.
void CreateFogPalette(eView *view, uint32 *palette, int fog_palette_mode, uint32 colour, float power, float start,
                      float end) {
    float fog_r;
    float fog_g;
    float fog_b;
    float fog_a;
    unsigned int tpalette[256];

    if (!view || !palette) {
        return;
    }

    fog_r = colour & 0xFF;
    fog_g = (colour >> 8) & 0xFF;
    fog_b = (colour >> 16) & 0xFF;
    fog_a = power * 128.0f;

    if (FogControlOverRide) {
        fog_palette_mode = view->GetID() == EVIEW_PLAYER1 ? FogMode1 : FogMode2;
    }

    switch (fog_palette_mode) {
    default:
    case FOG_NORMAL_P:
        for (int i = 0; i < 256; i++) {
            float fi = i;
            float t;

            if (fi < start) {
                t = 0.0f;
            } else if (fi < end) {
                t = (fi - start) / (end - start);
            } else {
                t = 1.0f;
            }

            t = FastPow(t, FogPower);

            int cr = bFloatToInt(fog_r) & 0xFF;
            int cg = bFloatToInt(fog_g) & 0xFF;
            int cb = bFloatToInt(fog_b) & 0xFF;

            tpalette[255 - i] = (bFloatToInt(t * fog_a) << 24) | (cb << 16) | (cg << 8) | cr;
        }
        break;

    case FOG_NIGHT_P:
        for (int i = 0; i < 256; i++) {
            float fi = i;
            float t = (fi - start) / (end - start);

            if (fi < start) {
                t = 0.0f;
            } else if (end < fi) {
                t = 1.0f;
            }
            t = 1.0f - t;

            tpalette[255 - i] = bFloatToInt(t * fog_a) << 24;
        }
        break;

    case FOG_DEPTH_P:
        for (int i = 0; i < 256; i++) {
            tpalette[i] = 0x80000000 | (i << 16) | (i << 8) | i;
        }
        break;

    case FOG_SINGLE_P:
        bMemSet(tpalette, 0, sizeof(tpalette));
        tpalette[bFloatToInt(fogVPstart)] = 0x80000080;
        tpalette[bFloatToInt(fogVPend)] = 0x80000080;
        break;

    case FOG_VOLUME_P:
        for (int i = 0; i < 256; i++) {
            float t;

            if (i < start) {
                t = 1.0f;
            } else {
                t = 0.0f;
            }

            int cr = bFloatToInt(fog_r) & 0xFF;
            int cg = bFloatToInt(fog_g) & 0xFF;
            int cb = bFloatToInt(fog_b) & 0xFF;

            tpalette[i] = (bFloatToInt(t * fog_a) << 24) | (cb << 16) | (cg << 8) | cr;
        }
        break;

    case FOG_HEADLIGHT_P:
        for (int i = 0; i < 256; i++) {
            float fi = i;
            float near_z = end;
            float far_z = start;
            int reversed = 0;
            float t;

            if (far_z < near_z) {
                far_z = near_z;
                near_z = start;
                reversed = 1;
            }

            if (fi < far_z && near_z < fi) {
                far_z -= near_z;
                if (reversed) {
                    t = (far_z - fi) / far_z;
                } else {
                    t = (fi - near_z) / far_z;
                }
                t *= headpower;
                if (1.0f < t) {
                    t = 1.0f;
                }
            } else {
                t = 0.0f;
            }

            int cr = bFloatToInt(fog_r) & 0xFF;
            int cg = bFloatToInt(fog_g) & 0xFF;
            int cb = bFloatToInt(fog_b) & 0xFF;

            tpalette[i] = (bFloatToInt(t * fog_a) << 24) | (cb << 16) | (cg << 8) | cr;
        }
        break;
    }

    RemapLinearIntoClut8(palette, tpalette);
}

int FogMeshResX = 8;
int FogMeshResY = 8;

extern FogQuery FogInfo;

// La niebla de un punto (x, y) de la malla girada Rotation alrededor del centro de la pantalla, segun su distancia a
// (xref, yref): 128 dentro de FogInner y despues cae con FogFalloff.
uint32 eFogAtDistant(int x, int y, int xref, int yref, bAngle Rotation, eView *view) {
    static float CosRot = 0.0f;
    static float SinRot = 0.0f;
    static bAngle lastRotation = 999;

    if (Rotation != lastRotation) {
        lastRotation = Rotation;
        CosRot = bCos(Rotation);
        SinRot = bSin(Rotation);
    }

    int nx = x - view->GetRenderTarget()->ScissorW / 2;
    int ny = y - (view->GetRenderTarget()->ScissorH / 2 + view->GetRenderTarget()->ScissorY);

    x = nx * CosRot - ny * SinRot;
    y = nx * SinRot + ny * CosRot;

    x += view->GetRenderTarget()->ScissorW / 2;
    y += view->GetRenderTarget()->ScissorH / 2 + view->GetRenderTarget()->ScissorY;

    // El original expande el valor absoluto con saltos (el ABS_EXPR de GCC), no con el pmaxw de bAbs; abs() de
    // stdlib.h sale aqui como llamada, asi que va el builtin.
    int dx = __builtin_abs(xref - x);
    int dy = __builtin_abs(yref - y);

    dx = dx * FogInfo.FogFalloffX;
    dy = dy * FogInfo.FogFalloffY;

    int retv;

    if (dy < dx) {
        retv = dx + (dy >> 2);
    } else {
        retv = dy + (dx >> 2);
    }

    if (retv < FogInner) {
        return 128;
    }

    retv = 128 - static_cast<int>(retv * FogInfo.FogFalloff);

    if (retv < 0) {
        retv = 0;
    }
    return retv;
}

// En WeatherMan.cpp.
void getHorizonColour(eView *view, int x, int y, uint32 *r, uint32 *g, uint32 *b, uint32 *a);

// La malla de la niebla: FogMeshResX columnas de tiras de triangulos, cada vertice con el color del horizonte
// (HORIZON_FOG) o con el alfa de eFogAtDistant.
uint32 *eBuildFogMesh(int xpos, int ypos, bAngle rotation, uint32 *data, eView *view, FogVolumeType fogType) {
    eRenderTarget *render_target = view->GetRenderTarget();
    int dx = render_target->ScissorX;
    int dy = render_target->ScissorY;
    int xstep = render_target->ScissorW / FogMeshResX;
    int ystep = render_target->ScissorH / FogMeshResY;
    int i;
    int j;

    for (i = 0; i < FogMeshResX; i++) {
        int dyy = dy;
        uint32 r = 128;
        uint32 g = 128;
        uint32 b = 128;

        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV((dx << 4) + 8, (dyy << 4) + 8);
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
        data += 4;

        if (fogType == HORIZON_FOG) {
            uint32 alph = 128;

            getHorizonColour(view, dx, dyy, &r, &g, &b, &alph);
            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_RGBAQ(r, g, b, alph, 0x3F800000);
        } else {
            reinterpret_cast<u_long *>(data)[0] =
                SCE_GS_SET_RGBAQ(r, g, b, eFogAtDistant(dx, dyy, xpos, ypos, rotation, view), 0x3F800000);
        }
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
        data += 4;

        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(dx << 4, dyy << 4, 0);
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ3;
        data += 4;

        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV(((dx + xstep) << 4) + 8, (dyy << 4) + 8);
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
        data += 4;

        if (fogType == HORIZON_FOG) {
            uint32 alph = 128;

            getHorizonColour(view, dx + xstep, dyy, &r, &g, &b, &alph);
            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_RGBAQ(r, g, b, alph, 0x3F800000);
        } else {
            reinterpret_cast<u_long *>(data)[0] =
                SCE_GS_SET_RGBAQ(r, g, b, eFogAtDistant(dx + xstep, dyy, xpos, ypos, rotation, view), 0x3F800000);
        }
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
        data += 4;

        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ((dx + xstep) << 4, dyy << 4, 0);
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ3;
        data += 4;

        for (j = 0; j < FogMeshResY; j++) {
            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV((dx << 4) + 8, ((dyy + ystep) << 4) + 8);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
            data += 4;

            if (fogType == HORIZON_FOG) {
                uint32 alph = 128;

                getHorizonColour(view, dx, (dyy + ystep), &r, &g, &b, &alph);
                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_RGBAQ(r, g, b, alph, 0x3F800000);
            } else {
                reinterpret_cast<u_long *>(data)[0] =
                    SCE_GS_SET_RGBAQ(r, g, b, eFogAtDistant(dx, (dyy + ystep), xpos, ypos, rotation, view), 0x3F800000);
            }
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(dx << 4, (dyy + ystep) << 4, 0);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV(((dx + xstep) << 4) + 8, ((dyy + ystep) << 4) + 8);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
            data += 4;

            if (fogType == HORIZON_FOG) {
                uint32 alph = 128;

                getHorizonColour(view, dx + xstep, (dyy + ystep), &r, &g, &b, &alph);
                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_RGBAQ(r, g, b, alph, 0x3F800000);
            } else {
                reinterpret_cast<u_long *>(data)[0] =
                    SCE_GS_SET_RGBAQ(r, g, b, eFogAtDistant(dx + xstep, (dyy + ystep), xpos, ypos, rotation, view), 0x3F800000);
            }
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ((dx + xstep) << 4, (dyy + ystep) << 4, 0);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
            data += 4;

            dyy += ystep;
        }
        dx += xstep;
    }
    return data;
}

float hmul = 1000.0f;

// El smear (estela de velocidad): los ajustes van por vista (dos jugadores), con los valores del .data de la A124.
int DrawSmear = 1;
int SmearInvert = 0;
uint8 Smearalpha_a = 0;
uint8 Smearalpha_b = 1;
uint8 Smearalpha_c = 0;
uint8 Smearalpha_d = 1;
uint8 Smearalpha_f = 0x80;
uint8 SmearalphaDS_a = 0;
uint8 SmearalphaDS_b = 1;
uint8 SmearalphaDS_c = 0;
uint8 SmearalphaDS_d = 1;
uint8 SmearalphaDS_f = 0x80;
float SmearBiasBASE = 2.0f;
float SmearBiasNOS = 8.0f;
int NOStimer = 15;
int SmearStage0 = 1;
int SmearStage1 = 1;
int SmearStage2 = 1;
int num_smears = 6;
int SmearSubStageEnable[2][6] = {{1, 1, 1, 1, 0, 0}, {1, 1, 1, 1, 0, 0}};
float SmearDistances[2][6] = {{2.3f, 6.4f, 8.8f, 11.4f, 22.0f, 40.0f}, {2.3f, 6.4f, 8.8f, 11.4f, 22.0f, 40.0f}};
float SmearAlphas[2][6] = {{50.0f, 35.0f, 28.0f, 17.0f, 8.0f, 4.0f}, {50.0f, 35.0f, 28.0f, 17.0f, 8.0f, 4.0f}};
int Rsub = -5;
int Gsub = -5;
int Bsub = -28;
float SmearDistModNOS = 4.0f;
int SmearR = 130;
int SmearG = 130;
int SmearB = 130;
float SmearScreenSize = 100.0f;
int SmearPrint = 0;
int SmearScreenPrint = 0;
int SmearScreenPrintX = 30;
int SmearScreenPrintY = 30;
float SmearMaxAlphaScale = 1.0f;
float SmearMinSpeed = 0.1f;
float SmearMaxSpeed = 25.0f;
float SmearAlphaScale = 0.0f;
int clearMe = 0;
int clearMe2 = 0;

float dropRez = 60.0f;
float Dropz = 16777215.0f;
float DropDiv = 4.0f;
float DropAm = 0.01f;
float shiftX = 4.0f;
float shiftY = 5.0f;
int DropCol = 0x80909090;
float DropColShift = 16.0f;

// Una gota de lluvia en pantalla: primero la mascara de la gota y despues, encima, un trozo de la pantalla
// desplazado (shiftX/shiftY por la cantidad) y aclarado con DropColShift.
void RainDrop(eView *view, float x, float y, float amount, float size) {
    float v = view->GetRenderTarget()->ScissorY / 512.0f;
    float RshiftX;
    float RshiftY;
    float dRez;

    RshiftX = shiftX * amount;
    RshiftY = shiftY * amount;
    dRez = size * dropRez;

    ePoly rainPoly;
    int Bcol;

    rainPoly.Vertices[0].x = x; rainPoly.Vertices[0].y = y; rainPoly.Vertices[0].z = Dropz; rainPoly.Vertices[0].pad = 0.0f;
    rainPoly.Vertices[1].x = x + dRez; rainPoly.Vertices[1].y = y; rainPoly.Vertices[1].z = Dropz; rainPoly.Vertices[1].pad = 0.0f;
    rainPoly.Vertices[2].x = x + dRez; rainPoly.Vertices[2].y = y + dRez; rainPoly.Vertices[2].z = Dropz; rainPoly.Vertices[2].pad = 0.0f;
    rainPoly.Vertices[3].x = x; rainPoly.Vertices[3].y = y + dRez; rainPoly.Vertices[3].z = Dropz; rainPoly.Vertices[3].pad = 0.0f;

    *reinterpret_cast<uint32 *>(rainPoly.Colours[0]) = 0x80808080;
    *reinterpret_cast<uint32 *>(rainPoly.Colours[1]) = 0x80808080;
    *reinterpret_cast<uint32 *>(rainPoly.Colours[2]) = 0x80808080;
    *reinterpret_cast<uint32 *>(rainPoly.Colours[3]) = 0x80808080;

    view->Render(&rainPoly, pTextureInfoRainDropMask, 0);

    Bcol = static_cast<int>(DropColShift * amount) + 128;
    DropCol = Bcol | (Bcol << 8) | (Bcol << 16) | (Bcol << 24);

    *reinterpret_cast<uint32 *>(rainPoly.Colours[0]) = DropCol;
    *reinterpret_cast<uint32 *>(rainPoly.Colours[1]) = DropCol;
    *reinterpret_cast<uint32 *>(rainPoly.Colours[2]) = DropCol;
    *reinterpret_cast<uint32 *>(rainPoly.Colours[3]) = DropCol;

    // El original escribe tambien un 1.0f justo detras de UVs (la primera palabra de UVsMask, linea 1705).
    rainPoly.UVs[0][0] = (RshiftX + x) / 512.0f; rainPoly.UVs[0][1] = v + (RshiftY + y) / 512.0f;
    rainPoly.UVs[1][0] = (RshiftX + x + dRez) / 512.0f; rainPoly.UVs[1][1] = v + (RshiftY + y) / 512.0f;
    rainPoly.UVs[2][0] = (RshiftX + x + dRez) / 512.0f; rainPoly.UVs[2][1] = v + (RshiftY + y + dRez) / 512.0f;
    rainPoly.UVs[3][0] = (RshiftX + x) / 512.0f; rainPoly.UVs[3][1] = v + (RshiftY + y + dRez) / 512.0f; rainPoly.UVs[4][0] = 1.0f;

    view->Render(&rainPoly, MainDraw0Context->GetTextureInfo(1), 0);
}

// La niebla de cada vista: la paleta de la niebla de distancia se rehace con FogInfo y se sube a la VRAM, y despues
// va la malla de eBuildFogMesh, centrada en el punto del horizonte que la camara tiene delante.
int eAttachFog(eDmaChain *dma_chain, uint32 palette_base_vram_address, FogVolumeType fogType) {
    eRenderTarget *disp_target = MainDisp0Context;

    for (int view_id = 1; view_id < 3; view_id++) {
        eView *view = eGetView(view_id, false);
        eRenderTarget *render_target = view->GetRenderTarget0();
        uint32 *fog_palette = GetFogPalette(view, fogType != DISTANT_FOG);

        if (view->IsActive() && render_target->IsActive() && fog_palette) {
            eSetCurrentRenderTarget(render_target);

            if (fogType == DISTANT_FOG) {
                if (FogInfo.CalculateFogInfo(view, 0)) {
                    CreateFogPalette(view, fog_palette, FOG_NORMAL_P, FogInfo.DistFogColour, FogInfo.DistFogPower,
                                     FogInfo.DistFogStart, 255.0f);
                }
            }

            if (!DrawDistantFog) {
                return 0;
            }

            int palette_vaddress = palette_base_vram_address + (view_id - 1) * 1024;
            AttachImageDataPacketDMA(dma_chain, fog_palette, palette_vaddress / 256, 64, 0, 1, 16, 16, 1);

            uint32 *data_start =
                static_cast<uint32 *>(eFrameMalloc((FogMeshResX * (FogMeshResY + 1) * 7 + 12) * sizeof(u_long128)));
            uint32 *data = data_start;

            if (!data_start) {
                return 0;
            }

            int fog_mode;

            if (view->GetID() == EVIEW_PLAYER1) {
                fog_mode = FogMode1;
            } else {
                fog_mode = FogMode1;
            }
            FogModeInfo *fog_info = &FogModeInfos[fog_mode];

            uint8 alpha_a = fog_info->AlphaA;
            uint8 alpha_b = fog_info->AlphaB;
            uint8 alpha_c = fog_info->AlphaC;
            uint8 alpha_d = fog_info->AlphaD;
            uint8 alpha_f = fog_info->AlphaF;

            *reinterpret_cast<u_long *>(data) = MakeDmaTagA(render_target->GetDrawContextData(0), 14, eDMA_REF);
            data += 2;
            *data++ = SCE_VIF1_SET_FLUSHE(0);
            *data++ = SCE_VIF1_SET_DIRECT(14, 0);

            *reinterpret_cast<u_long *>(data) = MakeDmaTag(FogMeshResX * 6 * (FogMeshResY + 1) + 10, eDMA_CNT);
            data += 2;
            *data++ = SCE_VIF1_SET_FLUSHE(0);
            *data++ = SCE_VIF1_SET_DIRECT(FogMeshResX * 6 * (FogMeshResY + 1) + 10, 0);

            reinterpret_cast<u_long *>(data)[0] = MakeGifTag(FogMeshResX * 6 * (FogMeshResY + 1) + 9, 1, 0, 0, GIF_PACKED, 1);
            reinterpret_cast<u_long *>(data)[1] = SCE_GIF_PACKED_AD;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] =
                SCE_GS_SET_TEX0(disp_target->FrameAddress << 5, 8, PSMT8H, 9, 9, 1, 0, palette_vaddress / 256, 0, 0, 0, 2);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEX0_1;
            data += 4;
            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEX1_1;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_CLAMP(1, 1, 0, 0, 0, 0);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_CLAMP_1;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_ALPHA(alpha_a, alpha_b, alpha_c, alpha_d, alpha_f);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_ALPHA_1;
            data += 4;
            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEST_1;
            data += 4;
            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_ZBUF(0, PSMZ16S, 1);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_ZBUF_1;
            data += 4;
            reinterpret_cast<u_long *>(data)[0] = 0;
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYOFFSET_1;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = PRIM_TRISTRIP | PRIM_IIP | PRIM_TME | PRIM_ABE | PRIM_FST;
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_PRIM;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
            data += 4;

            bVector3 horizonV = *view->GetCamera()->GetPosition();
            bVector3 horizonV2 = *view->GetCamera()->GetDirection();
            bVector3 horizonV3;
            bVector2 normScreen;

            horizonV3.x = horizonV.x + horizonV2.x * hmul;
            horizonV3.y = horizonV.y + horizonV2.y * hmul;
            horizonV3.z = 0.0f;

            view->GetCamera()->ProjectNormalized(&horizonV3, &normScreen);

            data = eBuildFogMesh(static_cast<int>(normScreen.x * (render_target->ScissorW / 2)) + render_target->ScissorW / 2,
                                 static_cast<int>(normScreen.y * (render_target->ScissorH / 2) + render_target->ScissorY) +
                                     render_target->ScissorH / 2,
                                 0, data, view, fogType);
            dma_chain->Add(data_start, data_start + 4);
        }
    }
    return 1;
}

uint32 *GetFocusPalette(eView *view) {
    int palette_index = view->GetID() - 1;

    if (palette_index < 0 || palette_index > 3) {
        return 0;
    }
    return reinterpret_cast<uint32 *>(FocusPalettes[palette_index]);
}

// El indice de la paleta de enfoque se mete en los bits bajos de la mantisa de la z de pantalla minima de ZRanges[1].
float MapPaletteIndexToScreenZ(int palette_index) {
    return IntegerAsFloat(FloatAsInteger(ZRanges[1][0]) | ((palette_index & 0xFF) << 12));
}

// La z de vista de una z de pantalla, con la proyeccion de ZRanges[1].
float MapScreenZToViewZ(float nearz, float farz, float screen_z) {
    float zmin = ZRanges[1][0];
    float zmax = ZRanges[1][1];

    float cz = (-zmax * nearz + zmin * farz) / (farz - nearz);
    float az = farz * nearz * (zmax - zmin) / (farz - nearz);

    return az / (screen_z - cz);
}

// La profundidad de campo: los ajustes, con los valores del .data de la A124.
int DrawDepthOfField = 1;
int FocusPrintPalette = 0;
float FocusAlphaMin = 0.0f;
float FocusAlphaMax = 128.0f;
float FocalDistance = 0.0f;
float DepthOfField = 0.0f;
int JitterBlend = 64;
int JitterOffsets[5][2] = {{0, 0}, {32, 32}, {32, 0}, {32, 32}, {0, 32}};
int AddaA = 0;
int AddaB = 0;
int AddaC = 0;
int AddaD = 1;
int AddaF = 128;
int FocusA = 0;
int FocusB = 1;
int FocusC = 1;
int FocusD = 1;
int FocusF = 128;
int dofNumPasses = 2;
int dofOffsetX = 0x4008;
int dofOffsetY = 0x4008;
int dofNumTransitionSlots = 24;

// La paleta de enfoque de una vista: alfa minimo en la franja enfocada (alrededor de la distancia focal de la camara o
// de FocalDistance), maximo fuera, y una rampa de dofNumTransitionSlots entradas en cada borde.
int CreateFocusPalette(eView *view, uint32 *palette) {
    if (!view || !palette) {
        return 0;
    }

    float camera_fz = view->FarZ;
    float camera_nz = view->NearZ;
    float focal_distance = view->GetCamera()->GetFocalDistance();
    float camera_dof = view->GetCamera()->GetDepthOfField();
    float dist[256];
    int num_transition_slots = dofNumTransitionSlots;
    if (FocalDistance != 0.0f) focal_distance = FocalDistance;
    float depth_of_field = camera_dof;
    if (DepthOfField != 0.0f) depth_of_field = DepthOfField;
    float alpha_min = FocusAlphaMin;
    float alpha_max = FocusAlphaMax;
    float alpha_range = alpha_max - alpha_min;
    unsigned int tpalette[256];

    if (depth_of_field == 0.0f && focal_distance == 0.0f) {
        return 0;
    }

    for (int pi = 0; pi < 256; pi++) {
        float screen_z = MapPaletteIndexToScreenZ(pi);

        dist[255 - pi] = MapScreenZToViewZ(camera_nz, camera_fz, screen_z);
    }

    float prev_distance = -3.402823466e+38f;
    float in_focus_start = focal_distance - depth_of_field * 0.5f;
    float in_focus_end = focal_distance + depth_of_field * 0.5f;
    int slot_in_focus_start = 0;
    int slot_in_focus_end = 255;

    for (int i = 0; i < 256; i++) {
        float slot_max_dist = dist[i];

        if (prev_distance < in_focus_start && in_focus_start <= slot_max_dist) {
            slot_in_focus_start = i;
        }

        if (prev_distance < in_focus_end && in_focus_end <= slot_max_dist) {
            slot_in_focus_end = i;
        }

        prev_distance = slot_max_dist;
    }

    uint32 min_colour = (bFloatToInt(alpha_min) << 24) | 0x808080;
    uint32 max_colour = (bFloatToInt(alpha_max) << 24) | 0x808080;

    for (int i = 0; i < 256; i++) {
        if (i >= slot_in_focus_start && i <= slot_in_focus_end) {
            tpalette[255 - i] = min_colour;
        } else {
            tpalette[255 - i] = max_colour;
        }
    }

    for (int i = 1; i < num_transition_slots; i++) {
        int slot = slot_in_focus_start - i;

        if (static_cast<unsigned int>(slot) < 256) {
            float t = static_cast<float>(i) / static_cast<float>(num_transition_slots);
            float a = alpha_min + t * alpha_range;

            tpalette[255 - slot] = (bFloatToInt(a) << 24) | 0x808080;
        }
    }

    for (int i = 1; i < num_transition_slots; i++) {
        int slot = slot_in_focus_end + i;

        if (static_cast<unsigned int>(slot) < 256) {
            float t = static_cast<float>(i) / static_cast<float>(num_transition_slots);
            float a = alpha_min + t * alpha_range;

            tpalette[255 - slot] = (bFloatToInt(a) << 24) | 0x808080;
        }
    }

    if (FocusPrintPalette) {
        for (int i = 0; i < 256; i++) {
            MapPaletteIndexToScreenZ(i);
        }
        FocusPrintPalette = 0;
    }

    RemapLinearIntoClut8(palette, tpalette);

    return 1;
}

// La profundidad de campo: sube la paleta de enfoque de cada vista, pasa la imagen de cada vista por su paleta (el
// alfa sale de la z) a la memoria de muestras, la emborrona a media resolucion con dofNumPasses pasadas desplazadas
// (JitterOffsets) y la vuelve a mezclar sobre la pantalla con el alfa de la paleta.
int eAttachDepthOfField(eDmaChain *dma_chain, uint32 vram_sample_buffer, uint32 vram_palette_base) {
    int view_id;
    int any_depth_of_field;
    eRenderTarget *draw_target;
    eRenderTarget *disp_target;
    int32 frame_width;
    int32 frame_height;

    if (!DrawDepthOfField) {
        return 0;
    }

    vram_sample_buffer = (vram_sample_buffer + 8191) & ~8191;
    any_depth_of_field = 0;

    for (view_id = 1; view_id < 3; view_id++) {
        eView *view = eGetView(view_id, false);

        if (view->IsActive()) {
            uint32 *palette = GetFocusPalette(view);

            if (palette) {
                any_depth_of_field |= CreateFocusPalette(view, palette);

                if (any_depth_of_field) {
                    int palette_vaddress = vram_palette_base + (view_id - 1) * 1024;

                    AttachImageDataPacketDMA(dma_chain, palette, palette_vaddress / 256, 64, 0, 1, 16, 16, 1);
                }
            }
        }
    }

    if (!any_depth_of_field) {
        return 0;
    }

    draw_target = MainDraw0Context;
    disp_target = MainDisp0Context;
    frame_width = draw_target->FrameWidth;
    frame_height = draw_target->FrameHeight;

    for (view_id = 1; view_id < 3; view_id++) {
        eView *view = eGetView(view_id, false);

        if (view->IsActive()) {
            eRenderTarget *render_target = view->GetRenderTarget();
            uint32 *palette = GetFocusPalette(view);
            u_long *data_start = static_cast<u_long *>(eFrameMalloc(50 * sizeof(u_long128)));
            u_long *data = data_start;

            if (palette && data_start) {
                int palette_vaddress = vram_palette_base + (view_id - 1) * 1024;

                *data++ = MakeDmaTag(49, eDMA_CNT);
                reinterpret_cast<uint32 *>(data)[0] = SCE_VIF1_SET_FLUSHE(0);
                reinterpret_cast<uint32 *>(data)[1] = SCE_VIF1_SET_DIRECT(49, 0);
                data++;

                *data++ = MakeGifTag(15, 0, 0, 0, GIF_PACKED, 1);
                *data++ = SCE_GIF_PACKED_AD;

                *data++ = 0;
                *data++ = SCE_GS_TEXFLUSH;
                *data++ = 0;
                *data++ = SCE_GS_DTHE;
                *data++ = 0;
                *data++ = SCE_GS_FBA_1;
                *data++ = SCE_GS_SET_TEXA(0x80, 0, 0x80);
                *data++ = SCE_GS_TEXA;
                *data++ = SCE_GS_SET_TEX1(1, 0, 0, 0, 0, 0, 0);
                *data++ = SCE_GS_TEX1_1;
                *data++ = 0;
                *data++ = SCE_GS_XYOFFSET_1;
                *data++ = SCE_GS_SET_ZBUF(0, PSMZ16S, 1);
                *data++ = SCE_GS_ZBUF_1;
                *data++ = SCE_GS_SET_TEST(1, 0, 0, 1, 0, 0, 1, 1);
                *data++ = SCE_GS_TEST_1;
                *data++ = SCE_GS_SET_ALPHA(AddaA, AddaB, AddaC, AddaD, AddaF);
                *data++ = SCE_GS_ALPHA_1;
                *data++ = SCE_GS_SET_SCISSOR(0, 1023, 0, 1023);
                *data++ = SCE_GS_SCISSOR_1;
                *data++ = SCE_GS_SET_CLAMP(1, 1, 0, 0, 0, 0);
                *data++ = SCE_GS_CLAMP_1;
                *data++ = SCE_GS_SET_FRAME(draw_target->FrameAddress, frame_width >> 6, 0, 0);
                *data++ = SCE_GS_FRAME_1;
                *data++ = SCE_GS_SET_TEX0(disp_target->FrameAddress << 5, frame_width >> 6, PSMT8H, 9, 9, 1, 0,
                                          palette_vaddress / 256, 0, 0, 0, 2);
                *data++ = SCE_GS_TEX0_1;
                *data++ = PRIM_SPRITE | PRIM_TME | PRIM_ABE | PRIM_FST;
                *data++ = SCE_GS_PRIM;
                *data++ = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
                *data++ = SCE_GS_RGBAQ;

                int dx = render_target->ScissorX;
                int dy = render_target->ScissorY;
                int dh = render_target->ScissorH;

                *data++ = MakeGifTag(16, 1, 0, 0, GIF_REGLIST, 4);
                *data++ = SCE_GS_UV | (SCE_GS_XYZ2 << 4) | (SCE_GS_UV << 8) | (SCE_GS_XYZ2 << 12);

                for (int i = 0; i < 16; i++) {
                    *data++ = SCE_GS_SET_UV(dx << 4, dy << 4);
                    *data++ = SCE_GS_SET_XYZ(dx << 4, dy << 4, 0);
                    *data++ = SCE_GS_SET_UV((dx + 32) << 4, (dy + dh) << 4);
                    *data++ = SCE_GS_SET_XYZ((dx + 32) << 4, (dy + dh) << 4, 0);

                    dx += 32;
                }

                dma_chain->Add(data_start);
            }
        }
    }

    int num_passes = dofNumPasses;
    u_long *data_start = static_cast<u_long *>(eFrameMalloc((num_passes * 32 + 71) * sizeof(u_long128)));
    u_long *data = data_start;

    if (!data_start) {
        return 0;
    }

    *data++ = MakeDmaTag(num_passes * 32 + 70, eDMA_CNT);
    reinterpret_cast<uint32 *>(data)[0] = SCE_VIF1_SET_FLUSHE(0);
    reinterpret_cast<uint32 *>(data)[1] = SCE_VIF1_SET_DIRECT(num_passes * 32 + 70, 0);
    data++;

    *data++ = MakeGifTag(15, 0, 0, 0, GIF_PACKED, 1);
    *data++ = SCE_GIF_PACKED_AD;

    *data++ = 0;
    *data++ = SCE_GS_TEXFLUSH;
    *data++ = 0;
    *data++ = SCE_GS_DTHE;
    *data++ = 1;
    *data++ = SCE_GS_FBA_1;
    *data++ = SCE_GS_SET_TEXA(0x80, 0, 0x80);
    *data++ = SCE_GS_TEXA;
    *data++ = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
    *data++ = SCE_GS_TEX1_1;
    *data++ = 0;
    *data++ = SCE_GS_XYOFFSET_1;
    *data++ = SCE_GS_SET_ZBUF(0, PSMZ16S, 1);
    *data++ = SCE_GS_ZBUF_1;
    *data++ = SCE_GS_SET_TEST(1, 0, 0, 1, 0, 0, 1, 1);
    *data++ = SCE_GS_TEST_1;
    *data++ = SCE_GS_SET_ALPHA(0, 2, 2, 1, JitterBlend);
    *data++ = SCE_GS_ALPHA_1;
    *data++ = SCE_GS_SET_SCISSOR(0, 511, 0, 511);
    *data++ = SCE_GS_SCISSOR_1;
    *data++ = SCE_GS_SET_CLAMP(1, 1, 0, 0, 0, 0);
    *data++ = SCE_GS_CLAMP_1;
    *data++ = SCE_GS_SET_FRAME(vram_sample_buffer / 8192, frame_width >> 7, 0, 0);
    *data++ = SCE_GS_FRAME_1;
    *data++ = SCE_GS_SET_TEX0(draw_target->FrameAddress << 5, frame_width >> 6, PSMCT32, 9, 9, 0, 1, 0, 0, 0, 0, 0);
    *data++ = SCE_GS_TEX0_1;
    *data++ = PRIM_SPRITE;
    *data++ = SCE_GS_PRIM;
    *data++ = SCE_GS_SET_RGBAQ(0, 0, 0, 0, 0x3F800000);
    *data++ = SCE_GS_RGBAQ;

    {
        int x = 0;
        int h = frame_height >> 1;

        *data++ = MakeGifTag(8, 0, 0, 0, GIF_REGLIST, 2);
        *data++ = SCE_GS_XYZ2 | (SCE_GS_XYZ2 << 4);

        for (int i = 0; i < 8; i++) {
            *data++ = SCE_GS_SET_XYZ(x << 4, 0, 0);
            *data++ = SCE_GS_SET_XYZ((x + 32) << 4, h << 4, 0);

            x += 32;
        }
    }

    *data++ = MakeGifTag(2, 0, 0, 0, GIF_PACKED, 1);
    *data++ = SCE_GIF_PACKED_AD;
    *data++ = PRIM_SPRITE | PRIM_TME | PRIM_ABE | PRIM_FST;
    *data++ = SCE_GS_PRIM;
    *data++ = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
    *data++ = SCE_GS_RGBAQ;

    *data++ = MakeGifTag(num_passes * 16, 0, 0, 0, GIF_REGLIST, 4);
    *data++ = SCE_GS_UV | (SCE_GS_XYZ2 << 4) | (SCE_GS_UV << 8) | (SCE_GS_XYZ2 << 12);

    for (int pass = 0; pass < num_passes; pass++) {
        int dx = JitterOffsets[pass][0];
        int dy = JitterOffsets[pass][1];
        int x = 0;

        for (int i = 0; i < 16; i++) {
            *data++ = SCE_GS_SET_UV((x << 4) + dx, dy);
            *data++ = SCE_GS_SET_XYZ((x >> 1) << 4, 0, 0);
            *data++ = SCE_GS_SET_UV(((x + 32) << 4) + dx, (frame_height << 4) + dy);
            *data++ = SCE_GS_SET_XYZ(((x + 32) >> 1) << 4, (frame_height >> 1) << 4, 0);

            x += 32;
        }
    }

    {
        int offset_x = dofOffsetX;
        int offset_y = dofOffsetY;

        *data++ = MakeGifTag(7, 0, 0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;
        *data++ = SCE_GS_TEXFLUSH;
        *data++ = SCE_GS_SET_FRAME(draw_target->FrameAddress, frame_width >> 6, 0, 0);
        *data++ = SCE_GS_FRAME_1;
        *data++ = SCE_GS_SET_TEX0(vram_sample_buffer / 256, frame_width >> 7, PSMCT32, 8, 8, 1, 1, 0, 0, 0, 0, 0);
        *data++ = SCE_GS_TEX0_1;
        *data++ = SCE_GS_SET_ALPHA(FocusA, FocusB, FocusC, FocusD, FocusF);
        *data++ = SCE_GS_ALPHA_1;
        *data++ = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        *data++ = SCE_GS_XYOFFSET_1;
        *data++ = PRIM_SPRITE | PRIM_TME | PRIM_ABE | PRIM_FST;
        *data++ = SCE_GS_PRIM;
        *data++ = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
        *data++ = SCE_GS_RGBAQ;

        *data++ = MakeGifTag(16, 1, 0, 0, GIF_REGLIST, 4);
        *data++ = SCE_GS_UV | (SCE_GS_XYZ2 << 4) | (SCE_GS_UV << 8) | (SCE_GS_XYZ2 << 12);

        int x = 0;

        for (int i = 0; i < 16; i++) {
            *data++ = SCE_GS_SET_UV((x >> 1) << 4, 0);
            *data++ = SCE_GS_SET_XYZ((x << 4) + 16384, 16384, 0);
            *data++ = SCE_GS_SET_UV(((x + 32) >> 1) << 4, (frame_height >> 1) << 4);
            *data++ = SCE_GS_SET_XYZ(((x + 32) << 4) + 16384, (frame_height << 4) + 16384, 0);

            x += 32;
        }
    }

    dma_chain->Add(data_start);

    return 1;
}

// La pixelacion de una cara (FacePixelation): el trozo de pantalla del rectangulo se reduce a un dieciseisavo y se
// vuelve a pintar ampliado, sin filtrar, para que salga en bloques.
void eAttachPIXELATE(eView *view, eDmaChain *dma_chain, eDmaChain *tex_chain) {
    FacePixelation *pixelation = view->facePixelation;

    if (pixelation && pixelation->IsEnabled()) {
        float screenX;
        float screenY;
        float width;
        float height;

        pixelation->GetData(&screenX, &screenY, &width, &height);

        screenX -= width * 0.5f;
        screenY -= height * 0.5f;

        eRenderTarget *rtP = view->GetRenderTarget();

        eGetRenderTarget(TARGET_QUARTER_32);
        eGetRenderTarget(TARGET_EIGTH_32);
        eRenderTarget *rtP2 = eGetRenderTarget(TARGET_SIXTEENTH_32);
        eGetRenderTarget(TARGET_THIRTYSECOND_32);

        float u0 = screenX / rtP->ScissorW;
        float u1 = (screenX + width) / rtP->ScissorW;

        float v0 = screenY / rtP->ScissorH;
        float v1 = (screenY + height) / rtP->ScissorH;

        float numrows = rtP->ScissorH / 32.0f;

        float verror = static_cast<int>(64.0f / numrows);

        verror = verror * numrows / rtP2->ScissorH;

        TextureInfoPlatInfo *plat_info = MainDraw0Context->GetTextureInfo(0)->GetPlatInfo();
        plat_info->pActiveTexture = 0;
        plat_info->MMin = 0;
        plat_info->MMag = 0;

        plat_info = rtP2->GetTextureInfo(0)->GetPlatInfo();
        plat_info->pActiveTexture = 0;
        int mmin = plat_info->MMin;
        int mmag = plat_info->MMag;
        plat_info->MMin = 0;
        plat_info->MMag = 0;

        float x = screenX;
        float y = screenY;
        float w = width;
        float h = height;

        v0 *= verror;
        v1 *= verror;

        eAttachDownsample(dma_chain, rtP2, rtP, 1);

        ePoly btoapoly;

        btoapoly.SetFlags(0);

        btoapoly.UVs[0][0] = u0; btoapoly.UVs[0][1] = v0;
        btoapoly.UVs[1][0] = u1; btoapoly.UVs[1][1] = v0;
        btoapoly.UVs[2][0] = u1; btoapoly.UVs[2][1] = v1;
        btoapoly.UVs[3][0] = u0; btoapoly.UVs[3][1] = v1;

        btoapoly.Vertices[0].x = x; btoapoly.Vertices[0].y = y;
        btoapoly.Vertices[1].x = x + w; btoapoly.Vertices[1].y = y;
        btoapoly.Vertices[2].x = x + w; btoapoly.Vertices[2].y = y + h;
        btoapoly.Vertices[3].x = x; btoapoly.Vertices[3].y = y + h;

        btoapoly.Colours[0][0] = 128; btoapoly.Colours[0][1] = 128; btoapoly.Colours[0][2] = 128; btoapoly.Colours[0][3] = 130;
        btoapoly.Colours[1][0] = 128; btoapoly.Colours[1][1] = 128; btoapoly.Colours[1][2] = 128; btoapoly.Colours[1][3] = 130;
        btoapoly.Colours[2][0] = 128; btoapoly.Colours[2][1] = 128; btoapoly.Colours[2][2] = 128; btoapoly.Colours[2][3] = 130;
        btoapoly.Colours[3][0] = 128; btoapoly.Colours[3][1] = 128; btoapoly.Colours[3][2] = 128; btoapoly.Colours[3][3] = 130;

        eSetCurrentRenderTarget(rtP);

        btoapoly.Vertices[0].z = rtP->ZMax;
        btoapoly.Vertices[1].z = rtP->ZMax;
        btoapoly.Vertices[2].z = rtP->ZMax;
        btoapoly.Vertices[3].z = rtP->ZMax;

        view->Render(&btoapoly, rtP2->GetTextureInfo(0), 0);

        AttachSolidPolys(dma_chain, tex_chain);

        plat_info->MMin = mmin;
        plat_info->MMag = mmag;
    }
}


// El blur y el bloom: los ajustes, con los valores del .data de la A124.
int bloomDS = 128;
int bloomDSa = 128;
int bloomDSmult = 128;
int bloomDSmulta = 128;
int downA = 0;
int downB = 2;
int downC = 2;
int downD = 1;
int downF = 71;
int fupA = 0;
int fupB = 2;
int fupC = 2;
int fupD = 1;
int fupF = 128;
int fupRGB = 128;
int fupAlf = 128;
int feupA = 0;
int feupB = 1;
int feupC = 2;
int feupD = 1;
int feupF = 128;
int feupRGB = 70;
int feupAlf = 128;
int bloomclear = 0;
int blooming3 = 1;
int ofu = 0;
int ofv = 0;
int Gx = 0;
int Gy = 0;
int Gu = 0;
int Gv = 0;
int b3 = 15;
uint32 skybaref = 5;

// El blur de la vista: primero pinta la mascara de alfa de la pantalla (sin alfa en todo y despues la de alfa a la z
// INz), la reduce a un cuarto, un octavo y un dieciseisavo con pasadas desplazadas b3 pixeles y despues mezcla el
// resultado sobre la pantalla con la cantidad que pide intensity.
void eAttachBlur(eView *view, eDmaChain *dma_chain, eDmaChain *tex_chain, float intensity, float INz, int bloom) {
    if (intensity == 0.0f) {
        return;
    }
    if (!blooming3) {
        return;
    }

    eRenderTarget *rtP = view->GetRenderTarget();
    TextureInfo *JustAlphamask = GetTextureInfo(bStringHash("ALLALPHA"), 0, 0);
    TextureInfo *NoAlphamask = GetTextureInfo(bStringHash("DEFAULTALPHA"), 0, 0);

    float w = rtP->ScissorW;
    float h = rtP->ScissorH;

    ePoly btoapoly;

    btoapoly.SetFlags(0);

    btoapoly.UVs[0][0] = 0.0f; btoapoly.UVs[0][1] = 0.0f;
    btoapoly.UVs[1][0] = 1.0f; btoapoly.UVs[1][1] = 0.0f;
    btoapoly.UVs[2][0] = 1.0f; btoapoly.UVs[2][1] = 1.0f;
    btoapoly.UVs[3][0] = 0.0f; btoapoly.UVs[3][1] = 1.0f;

    btoapoly.Vertices[0].x = 0.0f; btoapoly.Vertices[0].y = 0.0f;
    btoapoly.Vertices[1].x = w; btoapoly.Vertices[1].y = 0.0f;
    btoapoly.Vertices[2].x = w; btoapoly.Vertices[2].y = h;
    btoapoly.Vertices[3].x = 0.0f; btoapoly.Vertices[3].y = h;

    btoapoly.Colours[0][0] = 128; btoapoly.Colours[0][1] = 128; btoapoly.Colours[0][2] = 128; btoapoly.Colours[0][3] = 130;
    btoapoly.Colours[1][0] = 128; btoapoly.Colours[1][1] = 128; btoapoly.Colours[1][2] = 128; btoapoly.Colours[1][3] = 130;
    btoapoly.Colours[2][0] = 128; btoapoly.Colours[2][1] = 128; btoapoly.Colours[2][2] = 128; btoapoly.Colours[2][3] = 130;
    btoapoly.Colours[3][0] = 128; btoapoly.Colours[3][1] = 128; btoapoly.Colours[3][2] = 128; btoapoly.Colours[3][3] = 130;

    eSetCurrentRenderTarget(rtP);

    btoapoly.Vertices[0].z = rtP->ZMax;
    btoapoly.Vertices[1].z = rtP->ZMax;
    btoapoly.Vertices[2].z = rtP->ZMax;
    btoapoly.Vertices[3].z = rtP->ZMax;

    view->Render(&btoapoly, NoAlphamask, 0);
    AttachSolidPolys(dma_chain, tex_chain);

    btoapoly.SetFlags(2);
    btoapoly.SetFlailer(1);

    btoapoly.Vertices[0].z = INz;
    btoapoly.Vertices[1].z = INz;
    btoapoly.Vertices[2].z = INz;
    btoapoly.Vertices[3].z = INz;

    view->Render(&btoapoly, JustAlphamask, 0);
    AttachSolidPolys(dma_chain, tex_chain);

    uint32 testy = SCE_GS_SET_TEST(1, 5, skybaref, 0, 0, 0, 1, 1);

    eRenderTarget *rtP0 = eGetRenderTarget(TARGET_QUARTER_32);
    eRenderTarget *rtP1 = eGetRenderTarget(TARGET_EIGTH_32);
    eRenderTarget *rtP2 = eGetRenderTarget(TARGET_SIXTEENTH_32);
    eRenderTarget *rtP3 = eGetRenderTarget(TARGET_THIRTYSECOND_32);

    eClearRenderTarget(rtP0, dma_chain, 255, 255, 255, 255);
    eClearRenderTarget(rtP1, dma_chain, 255, 255, 255, 255);
    eClearRenderTarget(rtP2, dma_chain, 255, 255, 255, 255);
    eClearRenderTarget(rtP3, dma_chain, 255, 255, 255, 255);

    eAttachTargetMoveSprites(dma_chain, rtP0, rtP, Gx, Gy, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA, downF,
                             1, bloomDS, bloomDS, bloomDS, bloomDSa, 0, testy);

    eAttachTargetMoveSprites(dma_chain, rtP1, rtP0, -b3, -b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDS, bloomDS, bloomDS, bloomDSa, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP1, rtP0, -b3, b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDSmult, bloomDSmulta, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP1, rtP0, b3, -b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDSmult, bloomDSmulta, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP1, rtP0, b3, b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDSmult, bloomDSmulta, 0, testy);

    eAttachTargetMoveSprites(dma_chain, rtP2, rtP1, -b3, -b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDS, bloomDS, bloomDS, bloomDSa, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP2, rtP1, -b3, b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDS, bloomDSmulta, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP2, rtP1, b3, -b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDS, bloomDSmulta, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP2, rtP1, b3, b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDS, bloomDSmulta, 0, testy);

    if (bloomclear) {
        eClearRenderTarget(rtP, dma_chain, 255, 255, 255, 128);
    }

    int32 Amount2Blend = intensity * fupF;

    if (!bloom) {
        eAttachTargetMoveSprites(dma_chain, rtP, rtP2, 0, 0, ofu, ofv, (feupD << 6) | (feupC << 4) | (feupB << 2) | feupA,
                                 Amount2Blend, 1, feupRGB, feupRGB, feupRGB, feupAlf, 0, 0x30000);
    } else {
        eAttachTargetMoveSprites(dma_chain, rtP, rtP2, 0, 0, ofu, ofv, (fupD << 6) | (fupC << 4) | (fupB << 2) | fupA,
                                 Amount2Blend, 1, fupRGB, fupRGB, fupRGB, 128, 0, 0x30000);
    }
}

int New3d = 0;

// El bloom: como el blur, pero la mascara de alfa de la segunda pasada puede ser el poligono que deja el efecto de
// destello (SE_GLARE) en el ScreenEffectDB de la vista (use3D), y solo mezcla de vuelta con los ajustes fup.
void eAttachBloom4(eView *view, eDmaChain *dma_chain, eDmaChain *tex_chain, float intensity, float INz, int use3D) {
    if (intensity == 0.0f) {
        return;
    }
    if (!blooming3) {
        return;
    }

    eRenderTarget *rtP = view->GetRenderTarget();
    TextureInfo *JustAlphamask = GetTextureInfo(bStringHash("ALLALPHA"), 0, 0);
    TextureInfo *NoAlphamask = GetTextureInfo(bStringHash("DEFAULTALPHA"), 0, 0);

    float w = rtP->ScissorW;
    float h = rtP->ScissorH;

    ePoly btoapoly;

    btoapoly.SetFlags(0);

    if (!use3D) {
        btoapoly.UVs[0][0] = 0.0f; btoapoly.UVs[0][1] = 0.0f;
        btoapoly.UVs[1][0] = 1.0f; btoapoly.UVs[1][1] = 0.0f;
        btoapoly.UVs[2][0] = 1.0f; btoapoly.UVs[2][1] = 1.0f;
        btoapoly.UVs[3][0] = 0.0f; btoapoly.UVs[3][1] = 1.0f;

        btoapoly.Vertices[0].x = 0.0f; btoapoly.Vertices[0].y = 0.0f;
        btoapoly.Vertices[1].x = w; btoapoly.Vertices[1].y = 0.0f;
        btoapoly.Vertices[2].x = w; btoapoly.Vertices[2].y = h;
        btoapoly.Vertices[3].x = 0.0f; btoapoly.Vertices[3].y = h;

        btoapoly.Colours[0][0] = 128; btoapoly.Colours[0][1] = 128; btoapoly.Colours[0][2] = 128; btoapoly.Colours[0][3] = 130;
        btoapoly.Colours[1][0] = 128; btoapoly.Colours[1][1] = 128; btoapoly.Colours[1][2] = 128; btoapoly.Colours[1][3] = 130;
        btoapoly.Colours[2][0] = 128; btoapoly.Colours[2][1] = 128; btoapoly.Colours[2][2] = 128; btoapoly.Colours[2][3] = 130;
        btoapoly.Colours[3][0] = 128; btoapoly.Colours[3][1] = 128; btoapoly.Colours[3][2] = 128; btoapoly.Colours[3][3] = 130;

        eSetCurrentRenderTarget(rtP);

        btoapoly.Vertices[0].z = rtP->ZMax;
        btoapoly.Vertices[1].z = rtP->ZMax;
        btoapoly.Vertices[2].z = rtP->ZMax;
        btoapoly.Vertices[3].z = rtP->ZMax;

        view->Render(&btoapoly, NoAlphamask, 0);
        AttachSolidPolys(dma_chain, tex_chain);

        btoapoly.SetFlags(2);
        btoapoly.SetFlailer(1);

        btoapoly.Vertices[0].z = INz;
        btoapoly.Vertices[1].z = INz;
        btoapoly.Vertices[2].z = INz;
        btoapoly.Vertices[3].z = INz;

        view->Render(&btoapoly, JustAlphamask, 0);
    } else {
        btoapoly.UVs[0][0] = 0.0f; btoapoly.UVs[0][1] = 0.0f;
        btoapoly.UVs[1][0] = 1.0f; btoapoly.UVs[1][1] = 0.0f;
        btoapoly.UVs[2][0] = 1.0f; btoapoly.UVs[2][1] = 1.0f;
        btoapoly.UVs[3][0] = 0.0f; btoapoly.UVs[3][1] = 1.0f;

        btoapoly.Vertices[0].x = 0.0f; btoapoly.Vertices[0].y = 0.0f;
        btoapoly.Vertices[1].x = w; btoapoly.Vertices[1].y = 0.0f;
        btoapoly.Vertices[2].x = w; btoapoly.Vertices[2].y = h;
        btoapoly.Vertices[3].x = 0.0f; btoapoly.Vertices[3].y = h;

        btoapoly.Colours[0][0] = 128; btoapoly.Colours[0][1] = 128; btoapoly.Colours[0][2] = 128; btoapoly.Colours[0][3] = 130;
        btoapoly.Colours[1][0] = 128; btoapoly.Colours[1][1] = 128; btoapoly.Colours[1][2] = 128; btoapoly.Colours[1][3] = 130;
        btoapoly.Colours[2][0] = 128; btoapoly.Colours[2][1] = 128; btoapoly.Colours[2][2] = 128; btoapoly.Colours[2][3] = 130;
        btoapoly.Colours[3][0] = 128; btoapoly.Colours[3][1] = 128; btoapoly.Colours[3][2] = 128; btoapoly.Colours[3][3] = 130;

        eSetCurrentRenderTarget(rtP);

        btoapoly.Vertices[0].z = rtP->ZMax;
        btoapoly.Vertices[1].z = rtP->ZMax;
        btoapoly.Vertices[2].z = rtP->ZMax;
        btoapoly.Vertices[3].z = rtP->ZMax;

        view->Render(&btoapoly, NoAlphamask, 0);
        AttachSolidPolys(dma_chain, tex_chain);

        ScreenEffectDB *SE_db = view->ScreenEffects;

        btoapoly.Vertices[0].x = SE_db->GetDATA(SE_GLARE, 2);
        btoapoly.Vertices[1].x = SE_db->GetDATA(SE_GLARE, 5);
        btoapoly.Vertices[3].x = SE_db->GetDATA(SE_GLARE, 8);
        btoapoly.Vertices[2].x = SE_db->GetDATA(SE_GLARE, 11);

        btoapoly.Vertices[0].y = SE_db->GetDATA(SE_GLARE, 3);
        btoapoly.Vertices[1].y = SE_db->GetDATA(SE_GLARE, 6);
        btoapoly.Vertices[3].y = SE_db->GetDATA(SE_GLARE, 9);
        btoapoly.Vertices[2].y = SE_db->GetDATA(SE_GLARE, 12);

        btoapoly.Vertices[0].z = SE_db->GetDATA(SE_GLARE, 4);
        btoapoly.Vertices[1].z = SE_db->GetDATA(SE_GLARE, 7);
        btoapoly.Vertices[3].z = SE_db->GetDATA(SE_GLARE, 10);
        btoapoly.Vertices[2].z = SE_db->GetDATA(SE_GLARE, 13);

        view->Render(&btoapoly, JustAlphamask, eGetIdentityMatrix(), 0, 0.0f);
    }
    AttachSolidPolys(dma_chain, tex_chain);

    uint32 testy = SCE_GS_SET_TEST(1, 5, skybaref, 0, 0, 0, 1, 1);

    eRenderTarget *rtP0 = eGetRenderTarget(TARGET_QUARTER_32);
    eRenderTarget *rtP1 = eGetRenderTarget(TARGET_EIGTH_32);
    eRenderTarget *rtP2 = eGetRenderTarget(TARGET_SIXTEENTH_32);
    eRenderTarget *rtP3 = eGetRenderTarget(TARGET_THIRTYSECOND_32);

    eClearRenderTarget(rtP0, dma_chain, 255, 255, 255, 255);
    eClearRenderTarget(rtP1, dma_chain, 255, 255, 255, 255);
    eClearRenderTarget(rtP2, dma_chain, 255, 255, 255, 255);
    eClearRenderTarget(rtP3, dma_chain, 255, 255, 255, 255);

    eAttachTargetMoveSprites(dma_chain, rtP0, rtP, Gx, Gy, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA, downF,
                             1, bloomDS, bloomDS, bloomDS, bloomDSa, 0, testy);

    eAttachTargetMoveSprites(dma_chain, rtP1, rtP0, -b3, -b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDS, bloomDS, bloomDS, bloomDSa, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP1, rtP0, -b3, b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDSmult, bloomDSmulta, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP1, rtP0, b3, -b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDSmult, bloomDSmulta, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP1, rtP0, b3, b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDSmult, bloomDSmulta, 0, testy);

    eAttachTargetMoveSprites(dma_chain, rtP2, rtP1, -b3, -b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDS, bloomDS, bloomDS, bloomDSa, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP2, rtP1, -b3, b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDS, bloomDSmulta, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP2, rtP1, b3, -b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDS, bloomDSmulta, 0, testy);
    eAttachTargetMoveSprites(dma_chain, rtP2, rtP1, b3, b3, Gu, Gv, (downD << 6) | (downC << 4) | (downB << 2) | downA,
                             downF, 1, bloomDSmult, bloomDSmult, bloomDS, bloomDSmulta, 0, testy);

    if (bloomclear) {
        eClearRenderTarget(rtP, dma_chain, 255, 255, 255, 128);
    }

    float Amount2Blend = intensity * fupF;

    eAttachTargetMoveSprites(dma_chain, rtP, rtP2, 0, 0, ofu, ofv, (fupD << 6) | (fupC << 4) | (fupB << 2) | fupA,
                             static_cast<int>(Amount2Blend), 1, fupRGB, fupRGB, fupRGB, 128, 0, 0x30000);
}

float GlobAlphaScale = 1.2f;
unsigned int SmearRGBs[2][6][3];

// La estela de velocidad de cada vista: primero pasa la imagen por una paleta de alfa a un cuarto de pantalla
// (SmearStage0) y despues la vuelve a pintar varias veces, desplazada en la direccion en que se mueve la camara
// (SmearStage2).
int eAttachSmear2(eDmaChain *dma_chain, unsigned int palette_base_vram_address, float multiplier) {
    if (!DrawSmear) {
        return 0;
    }
    if (INIS::Exists()) {
        return 0;
    }

    static int NOStime[2] = {0, 0};
    static int NOSfade[2] = {0, 0};
    int NOSon[2] = {0, 0};
    float SmearBias[2] = {2.0f, 2.0f};
    float SmearDistMod[2] = {2.0f, 2.0f};

    eRenderTarget *disp_target = MainDisp0Context;
    eRenderTarget *draw_target = MainDraw0Context;

    for (int view_id = 1; view_id < 3; view_id++) {
        int SmIndex = view_id - 1;
        eView *view = eGetView(view_id, false);
        eRenderTarget *render_target = view->GetRenderTarget();

        if (view->IsActive() && render_target->IsActive()) {
            CameraMover *cameraMover = view->GetCameraMover();
            int IsThereNOS = 0;

            if (cameraMover) {
                CameraAnchor *cameraAnchor = cameraMover->GetAnchor();

                if (cameraAnchor) {
                    IsThereNOS = cameraAnchor->IsNosEngaged();
                }
            }

            if (IsThereNOS) {
                if (NOSfade[SmIndex] == 0) {
                    NOSfade[SmIndex] = 1;
                    NOStime[SmIndex] = 0;
                }
                NOSon[SmIndex] = 1;
                SmearSubStageEnable[SmIndex][4] = NOSon[SmIndex];
                SmearSubStageEnable[SmIndex][5] = NOSon[SmIndex];
            } else if (NOSfade[SmIndex] != 0) {
                NOSfade[SmIndex] = 2;
                NOSon[SmIndex] = 1;
            } else {
                SmearSubStageEnable[SmIndex][4] = 0;
                SmearSubStageEnable[SmIndex][5] = 0;
                SmearBias[SmIndex] = SmearBiasBASE;
            }

            if (NOSfade[SmIndex] == 1) {
                NOStime[SmIndex] += 2;
                if (NOStime[SmIndex] >= NOStimer) {
                    NOStime[SmIndex] = NOStimer;
                    NOSfade[SmIndex] = 3;
                }
            } else if (NOSfade[SmIndex] == 2) {
                if (--NOStime[SmIndex] <= 0) {
                    NOStime[SmIndex] = 0;
                    NOSfade[SmIndex] = 0;
                }
            }

            if (NOSon[SmIndex]) {
                SmearBias[SmIndex] =
                    static_cast<float>(NOStime[SmIndex]) * (SmearBiasNOS - SmearBiasBASE) / static_cast<float>(NOStimer) +
                    SmearBiasBASE;
                SmearDistMod[SmIndex] =
                    static_cast<float>(NOStime[SmIndex]) * (SmearDistModNOS - 2.0f) / static_cast<float>(NOStimer) + 2.0;
                SmearAlphas[SmIndex][4] = static_cast<float>(NOStime[SmIndex]) * 8.0 / static_cast<float>(NOStimer);
                SmearAlphas[SmIndex][5] = static_cast<float>(NOStime[SmIndex]) * 4.0 / static_cast<float>(NOStimer);

                for (int smear_substage = 0; smear_substage < num_smears; smear_substage++) {
                    SmearRGBs[SmIndex][smear_substage][0] = SmearR - smear_substage * Rsub;
                    SmearRGBs[SmIndex][smear_substage][1] = SmearG - smear_substage * Gsub;
                    SmearRGBs[SmIndex][smear_substage][2] = SmearB - smear_substage * Bsub;
                }
            }

            if (SmearStage0) {
                uint32 *alpha_palette = GetAlphaPalette(view);

                CreateAlphaPalette(alpha_palette, SmearBias[SmIndex]);

                int palette_vaddress = palette_base_vram_address + SmIndex * 1024;
                AttachImageDataPacketDMA(dma_chain, alpha_palette, palette_vaddress / 256, 64, 0, 1, 16, 16, 1);

                uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(76 * sizeof(u_long128)));
                uint32 *data = data_start;

                if (!data_start) {
                    return 0;
                }

                uint8 alpha_a = SmearalphaDS_a;
                uint8 alpha_b = SmearalphaDS_b;
                uint8 alpha_c = SmearalphaDS_c;
                uint8 alpha_d = SmearalphaDS_d;
                uint8 alpha_f = SmearalphaDS_f;

                *reinterpret_cast<u_long *>(data) =
                    MakeDmaTagA(eGetRenderTarget(TARGET_QUARTER_32)->GetDrawContextData(0), 14, eDMA_REF);
                data += 2;
                *data++ = SCE_VIF1_SET_FLUSHE(0);
                *data++ = SCE_VIF1_SET_DIRECT(14, 0);

                *reinterpret_cast<u_long *>(data) = MakeDmaTag(74, eDMA_CNT);
                data += 2;
                *data++ = SCE_VIF1_SET_FLUSHE(0);
                *data++ = SCE_VIF1_SET_DIRECT(74, 0);

                reinterpret_cast<u_long *>(data)[0] = MakeGifTag(73, 1, 0, 0, GIF_PACKED, 1);
                reinterpret_cast<u_long *>(data)[1] = SCE_GIF_PACKED_AD;
                data += 4;

                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_TEX0(disp_target->FrameAddress << 5, 8, PSMT8H, 9, 9, 1, 1,
                                                                      palette_vaddress / 256, 0, 0, 0, 2);
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEX0_1;
                data += 4;
                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEX1_1;
                data += 4;

                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_CLAMP(1, 1, 0, 0, 0, 0);
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_CLAMP_1;
                data += 4;

                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_ALPHA(alpha_a, alpha_b, alpha_c, alpha_d, alpha_f);
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_ALPHA_1;
                data += 4;
                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEST_1;
                data += 4;
                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_ZBUF(0, PSMZ16S, 1);
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_ZBUF_1;
                data += 4;
                reinterpret_cast<u_long *>(data)[0] = 0;
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYOFFSET_1;
                data += 4;

                int dx = render_target->ScissorX;
                int dy = render_target->ScissorY;
                int dw = render_target->ScissorW;
                int dh = render_target->ScissorH;

                reinterpret_cast<u_long *>(data)[0] = PRIM_SPRITE | PRIM_TME | PRIM_FST;
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_PRIM;
                data += 4;

                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);

                reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
                data += 4;

                dw /= 16;

                for (int i = 0; i < 16; i++) {
                    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV((dx << 4) + 8, (dy << 4) + 8);
                    reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
                    data += 4;

                    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(dx << 3, dy << 3, 0);
                    reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
                    data += 4;

                    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV(((dx + dw) << 4) + 8, ((dy + dh) << 4) + 8);
                    reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
                    data += 4;

                    reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ((dx + dw) << 3, (dy + dh) << 3, 0);
                    reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
                    data += 4;

                    dx += dw;
                }

                dma_chain->Add(data_start, data_start + 4);
            }
        }
    }

    if (SmearStage1) {
        eRenderTarget *rtP = MainDraw0Context;
        eRenderTarget *rtP0 = eGetRenderTarget(TARGET_QUARTER_24);

        eClearRenderTarget(rtP0, dma_chain, 255, 255, 255, 255);
        eAttachDownsample(dma_chain, rtP0, rtP, 0);
    }

    if (clearMe) {
        eClearRenderTarget(MainDraw0Context, dma_chain, 255, 255, 255, 128);
    }
    if (clearMe2) {
        return 0;
    }

    for (int view_id = 1; view_id < 3; view_id++) {
        if (eGetView(view_id, false)->IsActive() && eGetView(view_id, false)->GetRenderTarget0()->IsActive()) {
            int SmIndex = view_id - 1;

            if (SmearStage2) {
                bVector3 camera_world_velocity = *eGetView(view_id, false)->GetCamera()->GetVelocityPosition();
                float camera_speed = bLength(&camera_world_velocity);
                bVector4 camera_view_velocity;

                camera_view_velocity.x = camera_world_velocity.x;
                camera_view_velocity.y = camera_world_velocity.y;
                camera_view_velocity.z = camera_world_velocity.z;
                camera_view_velocity.w = 0.0f;

                eMulVector(&camera_view_velocity, eGetView(view_id, false)->GetCamera()->GetCameraMatrix(),
                           &camera_view_velocity);

                float min_speed = SmearMinSpeed;
                float max_speed = SmearMaxSpeed;
                float alpha_scale;

                camera_speed *= multiplier;

                if (camera_speed <= min_speed) {
                    alpha_scale = 0.0f;
                } else if (camera_speed >= max_speed) {
                    alpha_scale = 1.0f;
                } else {
                    alpha_scale = (camera_speed - min_speed) / (max_speed - min_speed);
                }
                alpha_scale *= GlobAlphaScale;

                if (alpha_scale > SmearMaxAlphaScale) {
                    alpha_scale = SmearMaxAlphaScale;
                }
                if (SmearAlphaScale != 0.0f) {
                    alpha_scale = SmearAlphaScale;
                }

#ifdef EA_BUILD_A124
                // Solo la A124 escribe estos valores en pantalla; SLUS ya no tiene el bloque.
                if (SmearScreenPrint) {
                    ScreenPrintf(SmearScreenPrintX, SmearScreenPrintY, "ViewV : %7.3f,%7.3f,%7.3f\n", camera_view_velocity.x,
                                 camera_view_velocity.y, camera_view_velocity.z);
                    ScreenPrintf(SmearScreenPrintX, SmearScreenPrintY + 30, "Speed : %7.3f\n", camera_speed);
                    ScreenPrintf(SmearScreenPrintX, SmearScreenPrintY + 45, "AlphaS: %7.3f\n", alpha_scale);
                }
#endif

                bVector4 s0;
                bVector4 s2;
                bVector4 camera_view_velocityN;

                bNormalize(&camera_view_velocityN, &camera_view_velocity);

                if (SmearInvert || camera_view_velocityN.z < 0.0f) {
                    s0.x = camera_view_velocityN.x + camera_view_velocityN.z;
                    s0.y = camera_view_velocityN.y + camera_view_velocityN.z;

                    s2.x = camera_view_velocityN.x - camera_view_velocityN.z;
                    s2.y = camera_view_velocityN.y - camera_view_velocityN.z;
                } else {
                    s0.x = -(camera_view_velocityN.x + camera_view_velocityN.z);
                    s0.y = -(camera_view_velocityN.y + camera_view_velocityN.z);

                    s2.x = camera_view_velocityN.z - camera_view_velocityN.x;
                    s2.y = camera_view_velocityN.z - camera_view_velocityN.y;
                }

                for (int smear_substage = num_smears - 1; smear_substage > 0; smear_substage--) {
                    if (SmearSubStageEnable[SmIndex][smear_substage]) {
                        uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(76 * sizeof(u_long128)));
                        uint32 *data = data_start;

                        if (!data_start) {
                            return 0;
                        }

                        int UseSmearR = SmearR;
                        int UseSmearG = SmearG;
                        int UseSmearB = SmearB;

                        if (NOSon[SmIndex]) {
                            UseSmearR = SmearRGBs[SmIndex][smear_substage][0];
                            UseSmearG = SmearRGBs[SmIndex][smear_substage][1];
                            UseSmearB = SmearRGBs[SmIndex][smear_substage][2];
                        }

                        uint8 alpha_a = Smearalpha_a;
                        uint8 alpha_b = Smearalpha_b;
                        uint8 alpha_c = Smearalpha_c;
                        uint8 alpha_d = Smearalpha_d;
                        uint8 alpha_f = Smearalpha_f;

                        eRenderTarget *src_target = eGetRenderTarget(TARGET_QUARTER_24);

                        *reinterpret_cast<u_long *>(data) =
                            MakeDmaTagA(eGetView(view_id, false)->GetRenderTarget0()->GetDrawContextData(0), 14, eDMA_REF);
                        data += 2;
                        *data++ = SCE_VIF1_SET_FLUSHE(0);
                        *data++ = SCE_VIF1_SET_DIRECT(14, 0);

                        *reinterpret_cast<u_long *>(data) = MakeDmaTag(74, eDMA_CNT);
                        data += 2;
                        *data++ = SCE_VIF1_SET_FLUSHE(0);
                        *data++ = SCE_VIF1_SET_DIRECT(74, 0);

                        reinterpret_cast<u_long *>(data)[0] = MakeGifTag(73, 1, 0, 0, GIF_PACKED, 1);
                        reinterpret_cast<u_long *>(data)[1] = SCE_GIF_PACKED_AD;
                        data += 4;

                        reinterpret_cast<u_long *>(data)[0] =
                            SCE_GS_SET_TEX0(src_target->FrameAddress << 5, 4, PSMCT32, 8, 8, 1, 0, 0, 0, 0, 0, 2);
                        reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEX0_1;
                        data += 4;
                        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
                        reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEX1_1;
                        data += 4;

                        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_CLAMP(1, 1, 0, 0, 0, 0);
                        reinterpret_cast<u_long *>(data)[1] = SCE_GS_CLAMP_1;
                        data += 4;

                        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_ALPHA(alpha_a, alpha_b, alpha_c, alpha_d, alpha_f);
                        reinterpret_cast<u_long *>(data)[1] = SCE_GS_ALPHA_1;
                        data += 4;
                        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
                        reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEST_1;
                        data += 4;
                        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_ZBUF(0, PSMZ16S, 1);
                        reinterpret_cast<u_long *>(data)[1] = SCE_GS_ZBUF_1;
                        data += 4;
                        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYOFFSET(16392, 16392);
                        reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYOFFSET_1;
                        data += 4;

                        int stage_alpha = bFloatToInt(SmearAlphas[SmIndex][smear_substage] * alpha_scale);

                        reinterpret_cast<u_long *>(data)[0] = PRIM_SPRITE | PRIM_IIP | PRIM_TME | PRIM_ABE | PRIM_FST;
                        reinterpret_cast<u_long *>(data)[1] = SCE_GS_PRIM;
                        data += 4;

                        reinterpret_cast<u_long *>(data)[0] =
                            SCE_GS_SET_RGBAQ(UseSmearR, UseSmearG, UseSmearB, stage_alpha, 0x3F800000);
                        reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
                        data += 4;

                        float smear_distance = -SmearDistances[SmIndex][smear_substage] * SmearDistMod[SmIndex];

                        if (eGetCurrentViewMode() == EVIEWMODE_TWOH) {
                            smear_distance *= 0.5f;
                        }

                        int s0x = src_target->ScissorX << 4;
                        int s0y = src_target->ScissorY << 4;
                        int s2x = (src_target->ScissorX + src_target->ScissorW - 1) << 4;
                        int s2y = (src_target->ScissorY + src_target->ScissorH - 1) << 4;

                        eRenderTarget *render_target = eGetView(view_id, false)->GetRenderTarget0();

                        if (render_target->ScissorH == render_target->FrameHeight >> 1) {
                            if (SmIndex == 0) {
                                s0x = 0;
                                s0y = 0;
                                s2x = 4064;
                                s2y = eCurrentVideoMode == MODE_NTSC ? 1776 : 2032;
                            } else {
                                s0x = 0;
                                s0y = eCurrentVideoMode == MODE_NTSC ? 1792 : 2048;
                                s2x = 4080;
                            }
                        }

                        int d0x = (render_target->ScissorX << 4) - bFloatToInt(s0.x * smear_distance * 16.0f);
                        int d0y = (render_target->ScissorY << 4) - bFloatToInt(s0.y * smear_distance * 16.0f);
                        int d2x = ((render_target->ScissorX + render_target->ScissorW - 1) << 4) -
                                  bFloatToInt(s2.x * smear_distance * 16.0f);
                        int d2y = ((render_target->ScissorY + render_target->ScissorH - 1) << 4) -
                                  bFloatToInt(s2.y * smear_distance * 16.0f);

                        int sx = s0x;
                        int sw = bAbs(s2x - sx) >> 4;
                        int sh = bAbs(s2y - s0y);

                        int dx = d0x + 16384;
                        int dy = d0y + 16384;

                        int dw = bAbs(d2x - d0x) >> 4;
                        int dh = bAbs(d2y - d0y);

                        for (int i = 0; i < 16; i++) {
                            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV(sx & 0xffff, s0y & 0xffff);
                            reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
                            data += 4;

                            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(dx, dy, 0);
                            reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
                            data += 4;

                            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV((sx + sw) & 0xffff, (s0y + sh) & 0xffff);
                            reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
                            data += 4;

                            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(dx + dw, dy + dh, 0);
                            reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
                            data += 4;

                            sx += sw;
                            dx += dw;
                        }

                        dma_chain->Add(data_start, data_start + 4);
                    }
                }
            }
        }
    }
    return 1;
}
