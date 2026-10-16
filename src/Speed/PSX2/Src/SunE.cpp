#include "Speed/PSX2/Inc/DmaE.hpp"
#include "Speed/PSX2/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/World/Sun.hpp"
#include "Speed/Indep/Src/World/Rain.hpp"
#include "Speed/Indep/Libs/Support/Utility/FastMem.h"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/bWare/Inc/bList.hpp"
#include "Speed/PSX2/Src/Ecstasy/TextureInfoPlat.hpp"

#include "Speed/PSX2/bWare/Src/ee/include/eestruct.h"

// psx2/src/SunE.cpp de la A124: el sol de PS2. StuffSun dibuja las capas del sol como polys en pantalla y deja cada una
// en la lista de destellos ocluidos; al final del frame RenderAll copia la profundidad alrededor del centro de cada
// destello a una textura (eCaptureDepth), la convierte en la paleta de la textura del destello
// (StuffFlareDepthIntoPalette) y dibuja el poly, de modo que la parte tapada por la escena sale transparente.

// Las define World/Sun.cpp; Sun.hpp no las declara (el SunE.cpp de GameCube hace lo mismo).
void SetCurrentSunInfo();
extern TextureInfo *SunTextures[5];

class OccludedFlare : private bTNode<OccludedFlare> {
    int mCentreX;          // offset 0x8, size 0x4
    int mCentreY;          // offset 0xC, size 0x4
    ePoly mPoly;           // offset 0x10, size 0x80
    TextureInfo *mTex;     // offset 0x90, size 0x4
    eView *mView;          // offset 0x94, size 0x4
    static bTList<OccludedFlare> sFlareList;

  public:
    USE_FASTALLOC(OccludedFlare)

    OccludedFlare(int x, int y, const ePoly &poly, TextureInfo *tex, eView *view)
        : mCentreX(x), mCentreY(y), mPoly(poly), mTex(tex), mView(view) {
        sFlareList.AddHead(this);
    }

    ~OccludedFlare() {
        Remove();
    }

    static void RenderAll(eDmaChain *dma_chain, eDmaChain *tex_chain);

    static void RemoveAll() {
        OccludedFlare *flare = sFlareList.GetHead();

        while (flare != sFlareList.EndOfList()) {
            OccludedFlare *next = flare->GetNext();

            delete flare;
            flare = next;
        }
    }

    static int CheckFlipByTexture(OccludedFlare *before, OccludedFlare *after) {
        return before->mTex <= after->mTex;
    }
};

bTList<OccludedFlare> OccludedFlare::sFlareList;
bool bResetGlare = false;
float SunPosX = 0.0f;
float SunPosY = 0.0f;
int num_sun_layers = 3;

void StuffSun(eView *view) {
    SetCurrentSunInfo();

    SunChunkInfo *sun_info = SunInfo;

    if (!sun_info) {
        return;
    }

#ifndef EA_BUILD_A124
    // Solo SLUS: la tercera capa siempre centrada en el sol, y las capas un poco por delante de ZMax (mas abajo).
    sun_info->SunLayers[2].OffsetX = sun_info->SunLayers[2].OffsetY = 0.0f;
#endif

    eRenderTarget *Myrender_target = view->GetRenderTarget();
    eRenderTarget *render_target = MainDraw0Context;

#ifdef EA_BUILD_A124
    float zmax = render_target->ZMax;
#else
    float zmax = render_target->ZMax - 2.0f;
#endif

    float screen_widthf = static_cast<float>(render_target->ScissorW);
    float screen_heightf = static_cast<float>(render_target->ScissorH);
    float x;
    float y;
    float max_size = 0.0f;
    float main_intensity;

    {
        bMatrix4 *camera_matrix = view->GetCamera()->GetCameraMatrix();
        bVector4 position3d;
        bVector4 position2d;
        bVector4 view3d;

        GetSunPos(view, &position3d.x, &position3d.y, &position3d.z);
        position3d.w = 1.0f;

        view->GetScreenPosition(reinterpret_cast<bVector3 *>(&position2d), reinterpret_cast<bVector3 *>(&position3d));
        eMulVector(&view3d, camera_matrix, &position3d);

        x = position2d.x;
        y = position2d.y;

        if (SunPosX != 0.0f || SunPosY != 0.0f) {
            x = SunPosX;
            y = SunPosY;
        }

        for (int i = 0; i < num_sun_layers; i++) {
            SunLayer *layer = &sun_info->SunLayers[i];

            if (layer->Texture == SUNTEX_CENTER) {
                if (layer->Size > max_size) {
                    max_size = layer->Size;
                }
            }
        }

        if (view3d.z < 0.0f || x < -max_size || x > screen_widthf + max_size || y < -max_size ||
            y > screen_heightf + max_size) {
            return;
        }
    }

    float delta_center_x = 0.0f;
    float delta_center_y = 0.0f;

    main_intensity = 1.0f;

    ePoly poly;

    poly.UVs[0][0] = 0.0f; poly.UVs[0][1] = 0.0f;
    poly.UVs[1][0] = 1.0f; poly.UVs[1][1] = 0.0f;
    poly.UVs[2][0] = 1.0f; poly.UVs[2][1] = 1.0f;
    poly.UVs[3][0] = 0.0f; poly.UVs[3][1] = 1.0f;

    poly.SetFlags(poly.GetFlags() ^ 1);

    for (int i = 0; i < num_sun_layers; i++) {
        SunLayer *layer = &sun_info->SunLayers[i];
        TextureInfo *texture_info = SunTextures[layer->Texture];
        float layer_intensity = layer->IntensityScale * (1.0f - view->Precipitation->GetCloudIntensity());

        if (texture_info && layer_intensity != 0.0f) {
            float max_sweep_angle = layer->SweepAngleAmount;
            float scale_x = (x + max_size) / (screen_widthf + max_size + max_size);
            float sweep_angle = max_sweep_angle * scale_x;
            bAngle angle = layer->Angle + static_cast<int>(sweep_angle * 65536.0f);
            float rx = layer->Size * 0.5f;
            float ry = layer->Size * 0.5f;
            float angle_sin = bSin(angle);
            float angle_cos = bCos(angle);
            float dx = rx * angle_sin + ry * angle_cos;
            float dy = ry * angle_cos - rx * angle_sin;
            int32 a;

            if (layer_intensity < 0.0f) {
                a = bFloatToInt(-layer_intensity * 128.0f);
            } else {
                a = bFloatToInt(main_intensity * 128.0f * layer_intensity);
            }

            float layer_z = zmax;

            if (i == 0) {
                layer_z = 1.0f;
            }

            poly.Vertices[0].z = layer_z;
            poly.Vertices[1].z = layer_z;
            poly.Vertices[2].z = layer_z;
            poly.Vertices[3].z = layer_z;

            float lx = x + delta_center_x + layer->OffsetX;
            float ly = y + delta_center_y + layer->OffsetY;
            int32 r = layer->Colour[0];
            int32 g = layer->Colour[1];
            int32 b = layer->Colour[2];
            float scale = 1.0f;

            if (lx < 16.0f) {
                scale = UMath::Min(UMath::Max(lx - 8.0f, 0.0f) * 0.125f, scale);
            }
            if (ly < 16.0f) {
                scale = UMath::Min(UMath::Max(ly - 8.0f, 0.0f) * 0.125f, scale);
            }
            if (lx > screen_widthf - 16.0f) {
                scale = UMath::Min(UMath::Max(static_cast<float>(Myrender_target->ScissorW) - lx - 8.0f, 0.0f) * 0.125f, scale);
            }
            if (ly > screen_heightf - 16.0f) {
                scale = UMath::Min(UMath::Max(static_cast<float>(Myrender_target->ScissorH) - ly - 8.0f, 0.0f) * 0.125f, scale);
            }

            a = static_cast<int32>(a * scale);

            if (a > 0) {
                poly.Vertices[0].x = lx - dx;
                poly.Vertices[0].y = ly - dy;
                poly.Vertices[1].x = lx + dy;
                poly.Vertices[1].y = ly - dx;
                poly.Vertices[2].x = lx + dx;
                poly.Vertices[2].y = ly + dy;
                poly.Vertices[3].x = lx - dy;
                poly.Vertices[3].y = ly + dx;

                poly.Colours[0][0] = r;
                poly.Colours[0][1] = g;
                poly.Colours[0][2] = b;
                poly.Colours[0][3] = a;
                poly.Colours[1][0] = r;
                poly.Colours[1][1] = g;
                poly.Colours[1][2] = b;
                poly.Colours[1][3] = a;
                poly.Colours[2][0] = r;
                poly.Colours[2][1] = g;
                poly.Colours[2][2] = b;
                poly.Colours[2][3] = a;
                poly.Colours[3][0] = r;
                poly.Colours[3][1] = g;
                poly.Colours[3][2] = b;
                poly.Colours[3][3] = a;

                new OccludedFlare(static_cast<int>(lx), static_cast<int>(ly), poly, texture_info, view);
            }
        }
    }
}

// Dos paquetes GS: el primero dibuja dos sprites de 16x16 pixeles sobre el centro del destello en el contexto alfa de
// la pantalla (el segundo con ZTST GEQUAL) y el segundo copia ese cuadrado a la textura intermedia 9 en cuatro tiras.
void eCaptureDepth(eRenderTarget *screenTarget, eDmaChain *dma_chain, int xPos, int yPos) {
    uint32 *data_start;
    int dx0;
    int dy0;
    eRenderTarget *offscreenTarget;

    data_start = static_cast<uint32 *>(eFrameMalloc(17 * sizeof(u_long128)));
    if (data_start) {
        reinterpret_cast<u_long *>(data_start)[0] = MakeDmaTagA(screenTarget->GetDrawContextData(DRAW_CONTEXT_ALPHA), 14, eDMA_REF);
        data_start[2] = SCE_VIF1_SET_FLUSHE(0);
        data_start[3] = SCE_VIF1_SET_DIRECT(14, 0);
        reinterpret_cast<u_long *>(data_start)[2] = MakeDmaTag(15, eDMA_CNT);
        data_start[6] = SCE_VIF1_SET_FLUSHE(0);
        data_start[7] = SCE_VIF1_SET_DIRECT(15, 0);
        reinterpret_cast<u_long *>(data_start)[4] = MakeGifTag(14, 1, 0, 0, GIF_PACKED, 1);
        reinterpret_cast<u_long *>(data_start)[5] = SCE_GIF_PACKED_AD;
        reinterpret_cast<u_long *>(data_start)[6] = SCE_GS_SET_FRAME(screenTarget->FrameAddress, screenTarget->FrameWidth >> 6, 0, 0xFFFFFF);
        reinterpret_cast<u_long *>(data_start)[7] = SCE_GS_FRAME_1;
        reinterpret_cast<u_long *>(data_start)[8] = SCE_GS_SET_ZBUF(screenTarget->ZBufferAddress, screenTarget->ZBufferMode, 1);
        reinterpret_cast<u_long *>(data_start)[9] = SCE_GS_ZBUF_1;
        reinterpret_cast<u_long *>(data_start)[10] = SCE_GS_SET_ALPHA(0, 2, 2, 1, 0x80);
        reinterpret_cast<u_long *>(data_start)[11] = SCE_GS_ALPHA_1;
        reinterpret_cast<u_long *>(data_start)[12] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
        reinterpret_cast<u_long *>(data_start)[13] = SCE_GS_TEST_1;
        reinterpret_cast<u_long *>(data_start)[14] = SCE_GS_SET_XYOFFSET(0, 0);
        reinterpret_cast<u_long *>(data_start)[15] = SCE_GS_XYOFFSET_1;

        dx0 = xPos - 8;
        dy0 = yPos - 8;

        reinterpret_cast<u_long *>(data_start)[16] = PRIM_SPRITE;
        reinterpret_cast<u_long *>(data_start)[17] = SCE_GS_PRIM;
        reinterpret_cast<u_long *>(data_start)[18] = SCE_GS_SET_RGBAQ(0, 0, 0, 0, 0x3F800000);
        reinterpret_cast<u_long *>(data_start)[19] = SCE_GS_RGBAQ;
        reinterpret_cast<u_long *>(data_start)[20] = SCE_GS_SET_XYZ(dx0 << 4, dy0 << 4, 0);
        reinterpret_cast<u_long *>(data_start)[21] = SCE_GS_XYZ2;
        reinterpret_cast<u_long *>(data_start)[22] = SCE_GS_SET_XYZ((xPos + 8) << 4, (yPos + 8) << 4, 0);
        reinterpret_cast<u_long *>(data_start)[23] = SCE_GS_XYZ2;
        reinterpret_cast<u_long *>(data_start)[24] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2);
        reinterpret_cast<u_long *>(data_start)[25] = SCE_GS_TEST_1;
        reinterpret_cast<u_long *>(data_start)[26] = PRIM_SPRITE;
        reinterpret_cast<u_long *>(data_start)[27] = SCE_GS_PRIM;
        reinterpret_cast<u_long *>(data_start)[28] = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
        reinterpret_cast<u_long *>(data_start)[29] = SCE_GS_RGBAQ;
        reinterpret_cast<u_long *>(data_start)[30] = SCE_GS_SET_XYZ(dx0 << 4, dy0 << 4, 0);
        reinterpret_cast<u_long *>(data_start)[31] = SCE_GS_XYZ2;
        reinterpret_cast<u_long *>(data_start)[32] = SCE_GS_SET_XYZ((xPos + 8) << 4, (yPos + 8) << 4, 0);
        reinterpret_cast<u_long *>(data_start)[33] = SCE_GS_XYZ2;

        dma_chain->Add(data_start, data_start + 4);

        data_start = static_cast<uint32 *>(eFrameMalloc(35 * sizeof(u_long128)));
        if (data_start) {
            offscreenTarget = eGetRenderTarget(9);

            reinterpret_cast<u_long *>(data_start)[0] = MakeDmaTag(33, eDMA_CNT);
            data_start[2] = SCE_VIF1_SET_FLUSHE(0);
            data_start[3] = SCE_VIF1_SET_DIRECT(33, 0);
            reinterpret_cast<u_long *>(data_start)[2] = MakeGifTag(32, 1, 0, 0, GIF_PACKED, 1);
            reinterpret_cast<u_long *>(data_start)[3] = SCE_GIF_PACKED_AD;
            reinterpret_cast<u_long *>(data_start)[4] = SCE_GS_SET_FRAME(offscreenTarget->FrameAddress, offscreenTarget->FrameWidth >> 6, 0, 0xFFFFFF);
            reinterpret_cast<u_long *>(data_start)[5] = SCE_GS_FRAME_1;
            reinterpret_cast<u_long *>(data_start)[6] = SCE_GS_SET_ZBUF(offscreenTarget->ZBufferAddress, offscreenTarget->ZBufferMode, 1);
            reinterpret_cast<u_long *>(data_start)[7] = SCE_GS_ZBUF_1;
            reinterpret_cast<u_long *>(data_start)[8] = SCE_GS_SET_TEX0(screenTarget->FrameAddress << 5, screenTarget->FrameWidth >> 6, 0, 9, 9, 1, 1, 0, 0,
                                      0, 0, 0);
            reinterpret_cast<u_long *>(data_start)[9] = SCE_GS_TEX0_1;
            reinterpret_cast<u_long *>(data_start)[10] = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
            reinterpret_cast<u_long *>(data_start)[11] = SCE_GS_TEX1_1;
            reinterpret_cast<u_long *>(data_start)[12] = 0;
            reinterpret_cast<u_long *>(data_start)[13] = SCE_GS_TEXFLUSH;
            reinterpret_cast<u_long *>(data_start)[14] = SCE_GS_SET_XYOFFSET(0, 0);
            reinterpret_cast<u_long *>(data_start)[15] = SCE_GS_XYOFFSET_1;
            reinterpret_cast<u_long *>(data_start)[16] = SCE_GS_SET_ALPHA(0, 2, 2, 1, 0x80);
            reinterpret_cast<u_long *>(data_start)[17] = SCE_GS_ALPHA_1;
            reinterpret_cast<u_long *>(data_start)[18] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
            reinterpret_cast<u_long *>(data_start)[19] = SCE_GS_TEST_1;
            reinterpret_cast<u_long *>(data_start)[20] = SCE_GS_SET_CLAMP(2, 2, dx0, dx0 + 16, dy0, dy0 + 16);
            reinterpret_cast<u_long *>(data_start)[21] = SCE_GS_CLAMP_1;
            reinterpret_cast<u_long *>(data_start)[22] = PRIM_SPRITE | PRIM_TME | PRIM_FST;
            reinterpret_cast<u_long *>(data_start)[23] = SCE_GS_PRIM;
            reinterpret_cast<u_long *>(data_start)[24] = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
            reinterpret_cast<u_long *>(data_start)[25] = SCE_GS_RGBAQ;
            reinterpret_cast<u_long *>(data_start)[26] = SCE_GS_SET_UV((dx0 + 1) << 4, (dy0 + 1) << 4);
            reinterpret_cast<u_long *>(data_start)[27] = SCE_GS_UV;
            reinterpret_cast<u_long *>(data_start)[28] = SCE_GS_SET_XYZ(0, 0, 0);
            reinterpret_cast<u_long *>(data_start)[29] = SCE_GS_XYZ2;
            reinterpret_cast<u_long *>(data_start)[30] = SCE_GS_SET_UV((dx0 + 17) << 4, (dy0 + 17) << 4);
            reinterpret_cast<u_long *>(data_start)[31] = SCE_GS_UV;
            reinterpret_cast<u_long *>(data_start)[32] = SCE_GS_SET_XYZ(128, 128, 0);
            reinterpret_cast<u_long *>(data_start)[33] = SCE_GS_XYZ2;
            reinterpret_cast<u_long *>(data_start)[34] = SCE_GS_SET_TEX0(offscreenTarget->FrameAddress << 5, offscreenTarget->FrameWidth >> 6, 0, 9, 9, 1, 1, 0,
                                       0, 0, 0, 0);
            reinterpret_cast<u_long *>(data_start)[35] = SCE_GS_TEX0_1;
            reinterpret_cast<u_long *>(data_start)[36] = 0;
            reinterpret_cast<u_long *>(data_start)[37] = SCE_GS_TEXFLUSH;
            reinterpret_cast<u_long *>(data_start)[38] = SCE_GS_SET_CLAMP(2, 2, 0, 8, 0, 8);
            reinterpret_cast<u_long *>(data_start)[39] = SCE_GS_CLAMP_1;
            reinterpret_cast<u_long *>(data_start)[40] = PRIM_SPRITE | PRIM_TME | PRIM_FST;
            reinterpret_cast<u_long *>(data_start)[41] = 0;
            reinterpret_cast<u_long *>(data_start)[42] = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
            reinterpret_cast<u_long *>(data_start)[43] = SCE_GS_RGBAQ;
            reinterpret_cast<u_long *>(data_start)[44] = SCE_GS_SET_UV(16, 16);
            reinterpret_cast<u_long *>(data_start)[45] = SCE_GS_UV;
            reinterpret_cast<u_long *>(data_start)[46] = SCE_GS_SET_XYZ(256, 0, 0);
            reinterpret_cast<u_long *>(data_start)[47] = SCE_GS_XYZ2;
            reinterpret_cast<u_long *>(data_start)[48] = SCE_GS_SET_UV(144, 144);
            reinterpret_cast<u_long *>(data_start)[49] = SCE_GS_UV;
            reinterpret_cast<u_long *>(data_start)[50] = SCE_GS_SET_XYZ(320, 64, 0);
            reinterpret_cast<u_long *>(data_start)[51] = SCE_GS_XYZ2;
            reinterpret_cast<u_long *>(data_start)[52] = 0;
            reinterpret_cast<u_long *>(data_start)[53] = SCE_GS_TEXFLUSH;
            reinterpret_cast<u_long *>(data_start)[54] = SCE_GS_SET_CLAMP(2, 2, 16, 20, 0, 4);
            reinterpret_cast<u_long *>(data_start)[55] = SCE_GS_CLAMP_1;
            reinterpret_cast<u_long *>(data_start)[56] = PRIM_SPRITE | PRIM_TME | PRIM_FST;
            reinterpret_cast<u_long *>(data_start)[57] = 0;
            reinterpret_cast<u_long *>(data_start)[58] = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
            reinterpret_cast<u_long *>(data_start)[59] = SCE_GS_RGBAQ;
            reinterpret_cast<u_long *>(data_start)[60] = SCE_GS_SET_UV(272, 16);
            reinterpret_cast<u_long *>(data_start)[61] = SCE_GS_UV;
            reinterpret_cast<u_long *>(data_start)[62] = SCE_GS_SET_XYZ(512, 0, 0);
            reinterpret_cast<u_long *>(data_start)[63] = SCE_GS_XYZ2;
            reinterpret_cast<u_long *>(data_start)[64] = SCE_GS_SET_UV(336, 80);
            reinterpret_cast<u_long *>(data_start)[65] = SCE_GS_UV;
            reinterpret_cast<u_long *>(data_start)[66] = SCE_GS_SET_XYZ(544, 32, 0);
            reinterpret_cast<u_long *>(data_start)[67] = SCE_GS_XYZ2;

            dma_chain->Add(data_start);
        }
    }
}

bool useAlphaMult = false;
int whichTexture = 0;
uint32 SUNmask = 0x00FFFFFF;

// Pinta la textura 9 sobre la paleta de la textura del destello (FRAME en PaletteVramAddress con la mascara SUNmask)
// y vuelve a enlazar el contexto de dibujo solido de la pantalla.
void StuffFlareDepthIntoPalette(eRenderTarget *screenTarget, eDmaChain *dma_chain, const TextureInfo *texture_info) {
    uint32 *data_start;
    uint32 *data;
    eRenderTarget *offscreenTarget;

    data_start = static_cast<uint32 *>(eFrameMalloc(16 * sizeof(u_long128)));
    if (data_start) {
        offscreenTarget = eGetRenderTarget(9);

        reinterpret_cast<u_long *>(data_start)[0] = MakeDmaTag(14, eDMA_CNT);
        data_start[2] = SCE_VIF1_SET_FLUSHE(0);
        data_start[3] = SCE_VIF1_SET_DIRECT(14, 0);
        reinterpret_cast<u_long *>(data_start)[2] = MakeGifTag(13, 1, 0, 0, GIF_PACKED, 1);
        reinterpret_cast<u_long *>(data_start)[3] = SCE_GIF_PACKED_AD;
        reinterpret_cast<u_long *>(data_start)[4] = 0;
        reinterpret_cast<u_long *>(data_start)[5] = SCE_GS_TEXFLUSH;
        reinterpret_cast<u_long *>(data_start)[6] = SCE_GS_SET_TEX0(offscreenTarget->FrameAddress << 5, offscreenTarget->FrameWidth >> 6,
                                                                    0, 9, 9, 1, 1, 0, 0, 0, 0, 0);
        reinterpret_cast<u_long *>(data_start)[7] = SCE_GS_TEX0_1;
        reinterpret_cast<u_long *>(data_start)[8] =
            SCE_GS_SET_FRAME(const_cast<TextureInfo *>(texture_info)->GetPlatInfo()->PaletteVramAddress / 32, 1, 0, SUNmask);
        reinterpret_cast<u_long *>(data_start)[9] = SCE_GS_FRAME_1;
        reinterpret_cast<u_long *>(data_start)[10] = SCE_GS_SET_XYOFFSET(0, 0);
        reinterpret_cast<u_long *>(data_start)[11] = SCE_GS_XYOFFSET_1;
        reinterpret_cast<u_long *>(data_start)[12] = SCE_GS_SET_ZBUF(screenTarget->ZBufferAddress, screenTarget->ZBufferMode, 1);
        reinterpret_cast<u_long *>(data_start)[13] = SCE_GS_ZBUF_1;
        reinterpret_cast<u_long *>(data_start)[14] = SCE_GS_SET_CLAMP(2, 2, 32, 34, 0, 2);
        reinterpret_cast<u_long *>(data_start)[15] = SCE_GS_CLAMP_1;
        reinterpret_cast<u_long *>(data_start)[16] = PRIM_SPRITE | PRIM_TME | PRIM_FST;
        reinterpret_cast<u_long *>(data_start)[17] = SCE_GS_PRIM;
        reinterpret_cast<u_long *>(data_start)[18] = SCE_GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000);
        reinterpret_cast<u_long *>(data_start)[19] = SCE_GS_RGBAQ;
        reinterpret_cast<u_long *>(data_start)[20] = SCE_GS_SET_UV(528, 16);
        reinterpret_cast<u_long *>(data_start)[21] = SCE_GS_UV;
        reinterpret_cast<u_long *>(data_start)[22] = SCE_GS_SET_XYZ(0, 0, 0);
        reinterpret_cast<u_long *>(data_start)[23] = SCE_GS_XYZ2;
        reinterpret_cast<u_long *>(data_start)[24] = SCE_GS_SET_UV(528, 16);
        reinterpret_cast<u_long *>(data_start)[25] = SCE_GS_UV;

        data = data_start + 52;
        if (const_cast<TextureInfo *>(texture_info)->GetPlatInfo()->PixelStorageMode == PSMT8) {
            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(256, 256, 0);
        } else {
            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(256, 16, 0);
        }
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
        data += 4;
        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEST_1;

        dma_chain->Add(data_start);

        data_start = static_cast<uint32 *>(eFrameMalloc(2 * sizeof(u_long128)));
        if (data_start) {
            reinterpret_cast<u_long *>(data_start)[0] =
                MakeDmaTagA(screenTarget->GetDrawContextData(DRAW_CONTEXT_SOLID), 14, eDMA_REF);
            data_start[2] = SCE_VIF1_SET_FLUSHE(0);
            data_start[3] = SCE_VIF1_SET_DIRECT(14, 0);
            reinterpret_cast<u_long *>(data_start)[2] = MakeDmaTag(0, eDMA_NEXT);
            data_start[6] = SCE_VIF1_SET_NOP(0);
            data_start[7] = SCE_VIF1_SET_NOP(0);

            dma_chain->Add(data_start, data_start + 4);
        }
    }
}

void eRemoveOccludedFlares() {
    OccludedFlare::RemoveAll();
}

int eRenderOccludedFlares(eDmaChain *dma_chain, eDmaChain *tex_chain) {
    OccludedFlare::RenderAll(dma_chain, tex_chain);
    OccludedFlare::RemoveAll();
    return 0;
}

uint32 splatpal = 1;

void OccludedFlare::RenderAll(eDmaChain *dma_chain, eDmaChain *tex_chain) {
    TextureInfo *lastTex;
    eActiveTexture *last_active_texture;
    eActiveTexture *first_active_texture;

    sFlareList.Sort(CheckFlipByTexture);

    lastTex = 0;
    last_active_texture = 0;
    first_active_texture = 0;

    for (OccludedFlare *flare = sFlareList.GetHead(); flare != sFlareList.EndOfList(); flare = flare->GetNext()) {
        TextureInfo *tex = flare->mTex;
        eView *view;

        first_active_texture = TextureDownloader.GetFirstTexture();
        if (tex != lastTex && tex) {
            if (last_active_texture) {
                AttachAllPolysNoTexUpload(first_active_texture, last_active_texture, dma_chain, tex_chain);
            }
            eGetActiveTexture(tex);
            TextureDownloader.AddTexturesSorted(&AlphaTextureList);
            lastTex = tex;
            TextureDownloader.AddTexturesSorted(&SolidTextureList);
            first_active_texture = TextureDownloader.GetFirstTexture();
            last_active_texture = TextureDownloader.DownloadDoubleBuffered(dma_chain, tex_chain, true);
        }

        view = flare->mView;
        if (view && view->IsActive()) {
            eRenderTarget *renderTarget = MainDraw0Context;
            eRenderTarget *viewTarget = view->GetRenderTarget();

            if (tex) {
                int xPos = flare->mCentreX;
                int yPos = flare->mCentreY + viewTarget->ScissorY;

                if (xPos >= renderTarget->ScissorX - 8 && xPos <= renderTarget->ScissorW + 8 &&
                    yPos >= renderTarget->ScissorY - 8 && yPos <= renderTarget->ScissorY + renderTarget->ScissorH + 8) {
                    eCaptureDepth(renderTarget, dma_chain, xPos, yPos);
                    if (splatpal) {
                        StuffFlareDepthIntoPalette(renderTarget, dma_chain, tex);
                    }
                    view->Render(&flare->mPoly, tex, 0);
                }
            }
        }
    }

    if (lastTex) {
        AttachAllPolysNoTexUpload(first_active_texture, last_active_texture, dma_chain, tex_chain);
    }
}
