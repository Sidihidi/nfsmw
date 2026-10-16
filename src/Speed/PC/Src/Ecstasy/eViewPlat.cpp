// eViewPlat.cpp de PC: la parte de eViewPlatInterface que no necesita la estructura completa de eViewPlatInfo
// (los wrappers de FE, los poligonos del FE y las consultas de una global). El resto (Render de modelos,
// GetScreenPosition, GetVisibleState...) espera a que la corriente de datos de zPlatform fije el layout de
// eViewPlatInfo (0x1a0 por vista).
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyData.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Indep/Src/Ecstasy/eEnvMap.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/PC/Src/Ecstasy/D3D9Min.hpp"
#include "Speed/PC/Src/Ecstasy/TextureInfoPlat.hpp"
#include "Speed/PC/Src/Ecstasy/eEffect.hpp"
#include "Speed/PC/Src/Ecstasy/eViewPlat.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Ecstasy/eLight.hpp"
#include "Speed/Indep/Src/World/SimpleModelAnim.hpp"
#include "Speed/PC/Src/Ecstasy/eSolidPlat.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/World/Car.hpp"
#include "Speed/Indep/Src/World/Rain.hpp"
#include "Speed/Indep/Src/World/TimeOfDay.hpp"

// Globales de la ventana y del modo de vista de PC (nombres nuestros: el exe no trae simbolos de esta capa;
// la corriente de datos de zPlatform es la que las declara).
extern "C" int __window_width;
extern "C" int __window_height;
extern "C" int CurrentViewMode;
extern "C" int RenderingViewMode;
extern "C" int TweakerViewMode;

// El estado de render de PC que tocan los poligonos del FE (nombres nuestros; entre parentesis, la direccion en
// speed.exe).
extern eEffect *eCurrentEffect;                // (0x982C80) el efecto empezado con BeginEffect, o ninguno
extern eEffect *eFEMaskEffect;                 // (0x93DEA0) el efecto de los poligonos con mascara
extern TextureInfo *eFEMaskTexture;            // (0x982BC8) la mascara del poligono que se esta dibujando
extern IDirect3DBaseTexture9 *eFEExtraTexture; // (0x982BC4)
extern int eIsMatroxCard;                      // (0x982C04) la tarjeta es de Matrox (fabricante 0x102B)
extern int eLastRenderedTexture;               // (0x982CA0)
extern int eRenderFlags;                       // (0x93D944)
extern bool eDrawFEPolys;                      // (0x8F9358)
extern eRenderTarget *eCurrentRenderTarget;    // (0x982A20) el destino en el que se dibuja
void eSetTexture(TextureInfo *texture_info, int stage);        // (0x6C6CC0)
void eSetAlphaTest(int enable, int ref);                       // (0x6C67C0) ALPHATESTENABLE y ALPHAREF
void eSetAlphaBlend(int enable, int src_blend, int dst_blend); // (0x6C6810) ALPHABLENDENABLE, SRCBLEND y DESTBLEND
void eCommitRenderStates();                                    // (0x6C83A0) los cinco al efecto actual
void eCountPolyRendered();                                     // (0x6C6ED0)
void eSetWorldMatrix(bMatrix4 *local_to_world, int view_id);  // (0x6C8000)

// El vertice de los poligonos del FE: D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 (0x20 bytes).
struct eFEPolyVertex {
    float x;
    float y;
    float z;
    D3DCOLOR colour;
    float u;
    float v;
    float u_mask;
    float v_mask;
};

eViewPlatInfo ViewPlatInfoTable[NUM_EVIEWS];

int GetPlaneState(const bVector4 *plane, const bVector3 *point) {
    float dot = plane->z * point->z + plane->y * point->y + plane->x * point->x + plane->w;
    return dot > 0.0f;
}

// La caja de una matriz por ejes (Arvo, Graphics Gems): la traslacion mas, por cada eje, el menor y el mayor de los
// productos de la fila con el minimo y el maximo.
void TransformBound(bMatrix4 *mat, bVector3 *min, bVector3 *max) {
    float newmin[3];
    float newmax[3];
    newmin[0] = mat->v3.x;
    newmin[1] = mat->v3.y;
    newmin[2] = mat->v3.z;
    newmax[0] = mat->v3.x;
    newmax[1] = mat->v3.y;
    newmax[2] = mat->v3.z;
    float *jmin = &min->x;
    float *jmax = &max->x;
    float *jmat = &mat->v0.x;
    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
            float a = *jmin * jmat[i];
            float b = *jmax * jmat[i];

            if (a < b) {
                newmin[i] += a;
                newmax[i] += b;
            } else {
                newmin[i] += b;
                newmax[i] += a;
            }
        }
        jmin++;
        jmax++;
        jmat += 4;
    }
    min->x = newmin[0];
    min->y = newmin[1];
    min->z = newmin[2];
    max->x = newmax[0];
    max->y = newmax[1];
    max->z = newmax[2];
}

#define FE_POLY_COLOUR(c)                                                                                              \
    ((D3DCOLOR)((((c)[3] & 0xff) << 24) | (((c)[0] & 0xff) << 16) | (((c)[1] & 0xff) << 8) | ((c)[2] & 0xff)))

void eViewPlatInterface::FEBeginBatchRender(int numPolys) {}

void eViewPlatInterface::FEEndBatchRender() {}

void eViewPlatInterface::FERender(ePoly *poly, TextureInfo *texture_info, int use_previous_data) {
    Render(poly, texture_info, use_previous_data);
}

void eViewPlatInterface::FERender(ePoly *poly, TextureInfo *texture_info, TextureInfo *texture_info_mask,
                                  int use_previous_data) {
    Render(poly, texture_info, texture_info_mask, use_previous_data);
}

// Un poligono del FE: de las coordenadas de pantalla de 640x480 a las de D3D, medio texel de correccion en las UV y
// un abanico de dos triangulos con el efecto actual (speed.exe 0x6D1260).
void eViewPlatInterface::Render(ePoly *poly, TextureInfo *texture_info, int use_previous_data) {
    bool stencil_write = false;
    bool stencil_test = false;
    if (eIsMatroxCard) {
        if (texture_info && (texture_info->NameHash == 0x40CDC515 || texture_info->NameHash == 0x1149C7DA)) {
            stencil_write = true;
        }
        if (poly->GetFlailer() == 'r' || poly->GetFlailer() == 's') {
            stencil_test = true;
        }
    }

    eFEPolyVertex vertices[4];
    vertices[0].x = poly->Vertices[0].x * (1.0f / 320.0f) - 1.0f;
    vertices[0].y = -(poly->Vertices[0].y * (1.0f / 240.0f) - 1.0f);
    vertices[0].z = (poly->GetFlags() & 2) ? poly->Vertices[0].z : 1.0f;
    vertices[0].colour = FE_POLY_COLOUR(poly->Colours[0]);
    vertices[0].u = poly->UVs[0][0];
    vertices[0].v = poly->UVs[0][1];
    vertices[0].u_mask = poly->UVsMask[0][0];
    vertices[0].v_mask = poly->UVsMask[0][1];
    vertices[1].x = poly->Vertices[1].x * (1.0f / 320.0f) - 1.0f;
    vertices[1].y = -(poly->Vertices[1].y * (1.0f / 240.0f) - 1.0f);
    vertices[1].z = (poly->GetFlags() & 2) ? poly->Vertices[1].z : 1.0f;
    vertices[1].colour = FE_POLY_COLOUR(poly->Colours[1]);
    vertices[1].u = poly->UVs[1][0];
    vertices[1].v = poly->UVs[1][1];
    vertices[1].u_mask = poly->UVsMask[1][0];
    vertices[1].v_mask = poly->UVsMask[1][1];
    vertices[2].x = poly->Vertices[2].x * (1.0f / 320.0f) - 1.0f;
    vertices[2].y = -(poly->Vertices[2].y * (1.0f / 240.0f) - 1.0f);
    vertices[2].z = (poly->GetFlags() & 2) ? poly->Vertices[2].z : 1.0f;
    vertices[2].colour = FE_POLY_COLOUR(poly->Colours[2]);
    vertices[2].u = poly->UVs[2][0];
    vertices[2].v = poly->UVs[2][1];
    vertices[2].u_mask = poly->UVsMask[2][0];
    vertices[2].v_mask = poly->UVsMask[2][1];
    vertices[3].x = poly->Vertices[3].x * (1.0f / 320.0f) - 1.0f;
    vertices[3].y = -(poly->Vertices[3].y * (1.0f / 240.0f) - 1.0f);
    vertices[3].z = (poly->GetFlags() & 2) ? poly->Vertices[3].z : 1.0f;
    vertices[3].colour = FE_POLY_COLOUR(poly->Colours[3]);
    vertices[3].u = poly->UVs[3][0];
    vertices[3].v = poly->UVs[3][1];
    vertices[3].u_mask = poly->UVsMask[3][0];
    vertices[3].v_mask = poly->UVsMask[3][1];

    if (texture_info) {
        if (use_previous_data != 'a') {
            vertices[0].u += 0.5f / texture_info->Width;
            vertices[0].v += 0.5f / texture_info->Height;
            vertices[1].u -= 0.5f / texture_info->Width;
            vertices[1].v += 0.5f / texture_info->Height;
            vertices[2].u -= 0.5f / texture_info->Width;
            vertices[2].v -= 0.5f / texture_info->Height;
            vertices[3].u += 0.5f / texture_info->Width;
            vertices[3].v -= 0.5f / texture_info->Height;
            if (eFEMaskTexture) {
                vertices[0].u_mask += 0.5f / eFEMaskTexture->Width;
                vertices[0].v_mask += 0.5f / eFEMaskTexture->Height;
                vertices[1].u_mask -= 0.5f / eFEMaskTexture->Width;
                vertices[1].v_mask += 0.5f / eFEMaskTexture->Height;
                vertices[2].u_mask -= 0.5f / eFEMaskTexture->Width;
                vertices[2].v_mask -= 0.5f / eFEMaskTexture->Height;
                vertices[3].u_mask += 0.5f / eFEMaskTexture->Width;
                vertices[3].v_mask -= 0.5f / eFEMaskTexture->Height;
            }
        }
        eSetTexture(texture_info, 0);
    }

    if (eFEExtraTexture) {
        eCurrentEffect->pD3DXEffect->SetTexture(eCurrentEffect->Parameters[16].Handle, eFEExtraTexture);
    }

    if (use_previous_data == 'a') {
        if (!eCurrentEffect->Unknown40) {
            g_D3DDevice->SetTextureStageState(0, 24, 0x103); // D3DTSS_TEXTURETRANSFORMFLAGS: COUNT3 | PROJECTED
            g_D3DDevice->SetFVF(0x10142);                    // XYZ | DIFFUSE | TEX1 | TEXCOORDSIZE3(0)
        } else {
            g_D3DDevice->SetTextureStageState(0, 24, 0x100); // PROJECTED
            g_D3DDevice->SetFVF(0x20142);                    // XYZ | DIFFUSE | TEX1 | TEXCOORDSIZE4(0)
        }
        D3DXMATRIXA16 identity(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                               1.0f);
        g_D3DDevice->SetTransform(16, &identity); // D3DTS_TEXTURE0
    }

    eLastRenderedTexture = 0;
    if (stencil_write) {
        g_D3DDevice->SetRenderState(52, 1); // D3DRS_STENCILENABLE
        g_D3DDevice->SetRenderState(56, 8); // D3DRS_STENCILFUNC: ALWAYS
        g_D3DDevice->SetRenderState(55, 7); // D3DRS_STENCILPASS: INCR
        eSetAlphaTest(1, 0x5F);
        eCommitRenderStates();
    }
    if (stencil_test) {
        g_D3DDevice->SetRenderState(52, 1); // D3DRS_STENCILENABLE
        g_D3DDevice->SetRenderState(56, 2); // D3DRS_STENCILFUNC: LESS
        eSetAlphaTest(0, 0x5F);
        if (poly->GetFlailer() == 'r') {
            if (eRenderFlags & 0x2000) {
                g_D3DDevice->SetRenderState(193, 0xFFB0B0B0); // D3DRS_BLENDFACTOR
                eSetAlphaBlend(1, 14, 15);                    // BLENDFACTOR, INVBLENDFACTOR
            } else {
                eSetAlphaBlend(1, 2, 1); // ONE, ZERO
            }
        } else {
            eSetAlphaBlend(1, 5, 2); // SRCALPHA, ONE
        }
        eCommitRenderStates();
    }

    if (eDrawFEPolys) {
        eCurrentEffect->pD3DXEffect->CommitChanges();
        eCurrentEffect->pD3DXEffect->BeginPass(0);
        g_D3DDevice->DrawPrimitiveUP(6, 2, vertices, sizeof(eFEPolyVertex)); // D3DPT_TRIANGLEFAN
        eCurrentEffect->pD3DXEffect->EndPass();
    }
    if (stencil_write || stencil_test) {
        g_D3DDevice->SetRenderState(52, 0);
    }
    eCountPolyRendered();
    if (use_previous_data == 'a') {
        g_D3DDevice->SetTextureStageState(0, 24, 2); // COUNT2
    }
}

// Con mascara: el efecto de mascara mientras se dibuja el poligono, y despues vuelve el que hubiera (0x6D1930).
void eViewPlatInterface::Render(ePoly *poly, TextureInfo *texture_info, TextureInfo *texture_info_mask,
                                int use_previous_data) {
    eFEMaskTexture = texture_info_mask;
    eEffect *previous_effect = eCurrentEffect;
    if (eCurrentEffect) {
        eCurrentEffect->EndEffect();
    }
    eCurrentEffect = eFEMaskEffect;
    eCurrentEffect->BeginEffect();
    eCurrentEffect->pD3DXEffect->SetTexture(eCurrentEffect->Parameters[29].Handle, eFEMaskTexture->GetPlatInfo()->pD3DTexture);
    texture_info->AlphaBlendType = 1;
    Render(poly, texture_info, use_previous_data);
    eFEMaskEffect->EndEffect();
    eCurrentEffect = 0;
    if (previous_effect) {
        eCurrentEffect = previous_effect;
        previous_effect->BeginEffect();
    }
    eFEMaskTexture = 0;
}

// Un poligono con matriz (0x6D1A40): sin correccion de medio texel ni mascara.
void eViewPlatInterface::Render(ePoly *poly, TextureInfo *texture_info, bMatrix4 *local_to_world,
                                int use_previous_data) {
    if (!local_to_world) {
        local_to_world = &eMathIdentityMatrix;
    }

    eFEPolyVertex vertices[4];
    vertices[0].x = poly->Vertices[0].x;
    vertices[0].y = poly->Vertices[0].y;
    vertices[0].z = poly->Vertices[0].z;
    vertices[0].colour = FE_POLY_COLOUR(poly->Colours[0]);
    vertices[0].u = poly->UVs[0][0];
    vertices[0].v = poly->UVs[0][1];
    vertices[1].x = poly->Vertices[1].x;
    vertices[1].y = poly->Vertices[1].y;
    vertices[1].z = poly->Vertices[1].z;
    vertices[1].colour = FE_POLY_COLOUR(poly->Colours[1]);
    vertices[1].u = poly->UVs[1][0];
    vertices[1].v = poly->UVs[1][1];
    vertices[2].x = poly->Vertices[2].x;
    vertices[2].y = poly->Vertices[2].y;
    vertices[2].z = poly->Vertices[2].z;
    vertices[2].colour = FE_POLY_COLOUR(poly->Colours[2]);
    vertices[2].u = poly->UVs[2][0];
    vertices[2].v = poly->UVs[2][1];
    vertices[3].x = poly->Vertices[3].x;
    vertices[3].y = poly->Vertices[3].y;
    vertices[3].z = poly->Vertices[3].z;
    vertices[3].colour = FE_POLY_COLOUR(poly->Colours[3]);
    vertices[3].u = poly->UVs[3][0];
    vertices[3].v = poly->UVs[3][1];

    if (texture_info) {
        eSetTexture(texture_info, 0);
    }
    eSetWorldMatrix(local_to_world, eCurrentRenderTarget->ViewID);
    if (eFEExtraTexture) {
        eCurrentEffect->pD3DXEffect->SetTexture(eCurrentEffect->Parameters[16].Handle, eFEExtraTexture);
    }
    if (eDrawFEPolys) {
        eCurrentEffect->pD3DXEffect->CommitChanges();
        eLastRenderedTexture = 0;
        eCurrentEffect->pD3DXEffect->BeginPass(0);
        g_D3DDevice->DrawPrimitiveUP(6, 2, vertices, sizeof(eFEPolyVertex)); // D3DPT_TRIANGLEFAN
        eCurrentEffect->pD3DXEffect->EndPass();
    }
    eCountPolyRendered();
}

void eViewPlatInterface::FERender(ePoly *poly, TextureInfo *texture_info, bMatrix4 *local_to_world,
                                  int use_previous_data, float bbRad) {
    Render(poly, texture_info, local_to_world, use_previous_data);
}

void eViewPlatInterface::Render(ePoly *poly, TextureInfo *texture_info, bMatrix4 *local_to_world,
                                int use_previous_data, float bbRad) {
    Render(poly, texture_info, local_to_world, use_previous_data);
}

int eViewPlatInterface::GetPixelWidth() {
    return __window_width;
}

int eViewPlatInterface::GetPixelHeight() {
    return __window_height;
}

eViewPlatInfo *eViewPlatInterface::GimmeMyViewPlatInfo(int view_id) {
    return &ViewPlatInfoTable[view_id];
}

// Como en GameCube, con la media ventana de PC en vez de 256 (0x6BE9F0).
float CalculateH(unsigned short alpha) {
    unsigned short beta = alpha >> 1;
    float tan = bTan(beta);
    if (tan != 0.0f) {
        return (__window_width >> 1) / tan;
    }
    return 10.0f;
}

eVisibleState eViewPlatInterface::GetVisibleStateSB(const bVector3 *aabb_min, const bVector3 *aabb_max,
                                                    bMatrix4 *local_world) {
    bVector3 min = *aabb_min;
    bVector3 max = *aabb_max;

    if (local_world) {
        TransformBound(local_world, &min, &max);
    }

    eViewPlatInfo *plat_info = GetPlatInfo();
    bVector3 centre((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f);
    bVector3 diag = max - centre;
    unsigned int partialFlag = 0;

    for (int clip_plane = 1; clip_plane <= 6; clip_plane++) {
        bVector4 *plane = &plat_info->ClippingPlanes[clip_plane - 1];
        float np = bAbs(plane->x) * diag.x + (bAbs(plane->y) * diag.y + bAbs(plane->z) * diag.z);
        float mp = centre.x * plane->x + centre.y * plane->y + centre.z * plane->z + plane->w;

        if (mp + np < 0.0f) {
            return EVISIBLESTATE_NOT;
        }
        if (mp - np < 0.0f) {
            partialFlag = 1;
        }
    }

    if (partialFlag) {
        return EVISIBLESTATE_PARTIAL;
    }
    return EVISIBLESTATE_FULL;
}

eVisibleState eViewPlatInterface::GetVisibleStateSB(const bVector3 *position, bMatrix4 *local_world) {
    eViewPlatInfo *plat_info = GetPlatInfo();

    for (int i = 0; i <= 5; i++) {
        if (!GetPlaneState(&plat_info->ClippingPlanes[i], position)) {
            return EVISIBLESTATE_NOT;
        }
    }
    return EVISIBLESTATE_FULL;
}

// De mundo a pixeles con la matriz de recorte y el tamano de la ventana (en GameCube, eRotTransPers a 640x480).
void eViewPlatInterface::GetScreenPosition(bVector3 *screen_position, const bVector3 *world_position) {
    eViewPlatInfo *plat_info = GetPlatInfo();
    bVector4 position(world_position->x, world_position->y, world_position->z, 1.0f);
    bVector4 clip;

    eMulVector(&clip, &plat_info->WorldClipMatrix, &position);
    float inv_w = 1.0f / clip.w;
    clip.x *= inv_w;
    clip.y *= inv_w;
    clip.z *= inv_w;
    screen_position->x = (clip.x + 1.0f) * __window_width * 0.5f;
    screen_position->y = (clip.y - 1.0f) * __window_height * -0.5f;
    screen_position->z = clip.z;
}

void eWaitUntilRenderingDone() {}

TextureInfo *eGetRenderTargetTextureInfo(int32 name_hash) {
    return 0;
}

EVIEWMODE eGetCurrentViewMode() {
    return static_cast<EVIEWMODE>(CurrentViewMode);
}

// El tamano de la pantalla de PC es el de la ventana (0x6BCDA0 y 0x6BCDB0).
int eGetScreenWidth() {
    return __window_width;
}

int eGetScreenHeight() {
    return __window_height;
}

// Los destinos de render de PC (0x93DAB8) y su consulta (0x6BD3A0), que epInitViews expande.
eRenderTarget eRenderTargets[11];

eRenderTarget *eGetRenderTarget(int32 render_target) {
    return &eRenderTargets[render_target];
}

void eFlushRenderList(); // (0x6E2F50) ordena y dibuja la lista de mallas del cuadro

// Como en GameCube, mas el borrado de PC: los destinos 1, 3 y 5 a 10 borran color y Z, el 2 solo el color y el resto
// tambien el stencil; el viewport es el del destino entero y no queda efecto empezado (0x6BD3B0).
void eSetCurrentRenderTarget(eRenderTarget *render_target, int clear) {
    eFlushRenderList();
    g_D3DDevice->SetRenderTarget(0, render_target->D3DSurface);
    g_D3DDevice->SetDepthStencilSurface(render_target->D3DDepthSurface);
    if (clear) {
        int id = render_target->ID;
        if ((id >= 5 && id <= 10) || id == 1 || id == 3) {
            g_D3DDevice->Clear(0, 0, 3, 0xFF000000, 1.0f, 0); // TARGET | ZBUFFER
        } else if (id == 2) {
            g_D3DDevice->Clear(0, 0, 1, 0xFF000000, 1.0f, 0); // TARGET
        } else {
            g_D3DDevice->Clear(0, 0, 7, 0xFF000000, 1.0f, 0); // TARGET | ZBUFFER | STENCIL
        }
    }
    D3DVIEWPORT9 viewport;
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = render_target->FrameWidth;
    viewport.Height = render_target->FrameHeight;
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    g_D3DDevice->SetViewport(&viewport);
    eCurrentRenderTarget = render_target;
    eCurrentEffect = 0;
}

// Por numero de destino (0x6CEF80).
void eSetCurrentRenderTarget(int render_target, int clear) {
    eSetCurrentRenderTarget(eGetRenderTarget(render_target), clear);
}

// El render de modelos de PC (nombres nuestros de lo que el exe no nombra; entre parentesis, la direccion).
extern int eOcclusionEnabled;           // (0x982B38) se prueba ademas cada caja contra los oclusores
extern bMatrix4 *eOcclusionMatrix;      // (0x982D04) la matriz con la que se pasan las cajas a los oclusores
extern unsigned int eSkipTextureHash;   // (0x982D40) las mallas con esta textura no se dibujan
extern int eNumMeshesRendered;          // (0x982B90)
extern int eNumModelsRendered;          // (0x982B94)
extern int eNumEffectChanges;           // (0x982B98)
extern int eMeshesPerModelHistogram[8]; // (0x9828A0) cuantos modelos dibujaron 0..6 mallas, y 7 o mas
extern int eEffectChangesHistogram[8];  // (0x93DE38) cuantos modelos cambiaron 0..6 veces de efecto, y 7 o mas
bool eOccluderTest(bVector3 *bbox_min, bVector3 *bbox_max); // (0x6D7A40) falso si la caja esta tapada
eLightMaterial *elGetDefaultLightMaterial();

void eRenderMesh(eMeshEntry *mesh, eSolid *solid, uint32 flags, eEffect *effect, TextureInfo **textures,
                 bMatrix4 *local_to_world, eLightContext *light_context, eLightMaterial *light_material,
                 bMatrix4 *blending_matricies); // (0x6E0AB0) apunta la malla en la lista del cuadro

// Como Render(eModel) de GameCube, pero malla a malla: cada malla de eSolidPlatInfo lleva su caja, sus cinco texturas,
// su material de luz y su efecto. Dos modelos se eligen por el efecto del cielo (0xED85BA82 y 0xBE43EDBB); en el
// destino 4 y en los que no son el 0 el efecto se cambia por su version sin niebla (0x6DA9B0).
void eViewPlatInterface::Render(eModel *model, bMatrix4 *local_to_world, eLightContext *light_context, uint32 flags,
                                bMatrix4 *blending_matricies) {
    eView *view = (eView *)this;
    uint32 name_hash = model->GetNameHash();

    if (name_hash == 0x90F70174) {
        return;
    }
    if (name_hash == 0xED85BA82) {
        if (!eEffects[19]->Unknown40) {
            return;
        }
    } else if (name_hash == 0xBE43EDBB) {
        if (eEffects[19]->Unknown40) {
            return;
        }
    }

    eSolid *solid = model->GetSolid();
    if (!solid) {
        return;
    }
    eSolidPlatInfo *solid_plat_info = solid->GetPlatInfo();
    if (!solid_plat_info->IndexBuffer) {
        return;
    }

    SimpleModelAnim::Animate(model, solid, local_to_world);

    if (eOcclusionEnabled) {
        bVector3 bbox_min;
        bVector3 bbox_max;
        bMatrix4 local_occlusion;

        solid->GetBoundingBox(&bbox_min, &bbox_max);
        D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX *>(&local_occlusion), reinterpret_cast<D3DXMATRIX *>(local_to_world),
                           reinterpret_cast<D3DXMATRIX *>(eOcclusionMatrix));
        TransformBound(&local_occlusion, &bbox_min, &bbox_max);
        if (!eOccluderTest(&bbox_min, &bbox_max)) {
            return;
        }
    }

    TextureInfo **replaced_textures[128];
    if (model->GetReplacementTextureTable()) {
        model->ApplyReplacementTextureTable(replaced_textures);
    }

    int num_meshes_rendered = 0;
    int num_effect_changes = 0;
    int last_effect = -1;
    for (int i = 0; i < solid_plat_info->NumMeshes; i++) {
        eMeshEntry *mesh = &solid_plat_info->MeshEntryTable[i];
        eEffect *effect = mesh->pEffect;

        if (!(flags & 4) && !(mesh->Flags & 1)) {
            if (!GetVisibleStateSB(reinterpret_cast<bVector3 *>(mesh->BBoxMin), reinterpret_cast<bVector3 *>(mesh->BBoxMax),
                                   local_to_world)) {
                continue;
            }
        }
        if (eOcclusionEnabled) {
            bVector3 bbox_min(mesh->BBoxMin[0], mesh->BBoxMin[1], mesh->BBoxMin[2]);
            bVector3 bbox_max(mesh->BBoxMax[0], mesh->BBoxMax[1], mesh->BBoxMax[2]);
            bMatrix4 local_occlusion;

            D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX *>(&local_occlusion),
                               reinterpret_cast<D3DXMATRIX *>(local_to_world), reinterpret_cast<D3DXMATRIX *>(eOcclusionMatrix));
            TransformBound(&local_occlusion, &bbox_min, &bbox_max);
            if (!eOccluderTest(&bbox_min, &bbox_max)) {
                continue;
            }
        }

        if (eCurrentRenderTarget->ID == 4) {
            if (effect->Index == 4) {
                effect = eEffects[23];
            }
            if (effect->Index == 0) {
                effect = eEffects[8];
            } else if (effect->Index == 3 || effect->Index == 1) {
                effect = eEffects[30];
            } else if (effect->Index == 5) {
                effect = eEffects[8];
            }
        } else if (eCurrentRenderTarget->ID != 0 && effect->Index != 4) {
            if (effect->Index == 0) {
                effect = eEffects[8];
            } else if (effect->Index == 3 || effect->Index == 1) {
                effect = eEffects[30];
            }
        }
        if (flags & 0x20000) {
            if (effect->Index == 0 || effect->Index == 8) {
                effect = eEffects[22];
            }
        }

        TextureInfo *textures[5];
        bool missing_texture = false;
        for (int t = 0; t < 5; t++) {
            textures[t] = solid->pTextureTable[mesh->TextureIndex[t]].pTextureInfo;
            missing_texture |= textures[t] == 0;
        }

        if (missing_texture || !effect->Technique) {
            if (effect->Index != 5) {
                continue;
            }
            effect = eEffects[0];
        }
        if (textures[0]->NameHash == eSkipTextureHash) {
            continue;
        }

        num_meshes_rendered++;
        if (effect->Index != last_effect) {
            num_effect_changes++;
            last_effect = effect->Index;
        }

        eLightMaterial *light_material;
        if (mesh->LightMaterialIndex != 0xFF) {
            light_material = solid->LightMaterialTable[mesh->LightMaterialIndex].LightMaterial;
        } else {
            light_material = elGetDefaultLightMaterial();
        }
        if (effect->Index == 4 && !light_context) {
            light_context = view->WorldLightContext;
        }
        eRenderMesh(mesh, solid, flags, effect, textures, local_to_world, light_context, light_material, blending_matricies);
    }

    eNumMeshesRendered += num_meshes_rendered;
    eNumEffectChanges += num_effect_changes;
    if (num_meshes_rendered > 0) {
        eNumModelsRendered++;
    }
    if (num_meshes_rendered < 7) {
        eMeshesPerModelHistogram[num_meshes_rendered]++;
    } else {
        eMeshesPerModelHistogram[7]++;
    }
    if (num_effect_changes < 7) {
        eEffectChangesHistogram[num_effect_changes]++;
    } else {
        eEffectChangesHistogram[7]++;
    }
    model->RestoreReplacementTextureTable(replaced_textures);
}

// Como en GameCube: PC no tiene pantalla partida, pero el modo de dos jugadores sigue aqui (0x6CFA10).
void eUpdateViewMode(void) {
    eView *eview_player1 = eGetView(EVIEW_PLAYER1, false);
    eView *eview_player2 = eGetView(EVIEW_PLAYER2, false);
    eView *eview_player1_rvm = eGetView(EVIEW_PLAYER1_RVM, false);

    if (eview_player1->GetCameraMover() != nullptr && eview_player2->GetCameraMover() != nullptr) {
        CurrentViewMode = EVIEWMODE_TWOH;
    } else if (eview_player1->GetCameraMover() != nullptr && eview_player1_rvm->GetCameraMover() != nullptr &&
               !eview_player1->GetCameraMover()->OutsidePOV()) {
        CurrentViewMode = EVIEWMODE_ONE_RVM;
    } else if (eview_player1->GetCameraMover() != nullptr) {
        CurrentViewMode = EVIEWMODE_ONE;
    } else {
        CurrentViewMode = EVIEWMODE_NONE;
    }
}

// El retrovisor de PC se puede apagar: la funcion de 0x595D40 (fuera de esta unidad; nombre nuestro) mira la opcion
// del jugador, la pausa del FE y si el HUD de carrera esta puesto.
bool IsRearViewMirrorAllowed(int view_id);
// Del coche del FE (CarRender.cpp; no hay cabecera que los declare).
extern bTList<FrontEndRenderingCar> FrontEndRenderingCarList;
int IsAnyFrontEndCars();
CameraMover *cmp; // (0x93DA4C) el nombre es el de IDA; en GameCube es una local sin uso

// PC: solo uno o ninguno, sin pantalla partida; el retrovisor se dibuja en su propio destino (el 3) y el de la
// vista 4 es de 320x240. En el FE, la vista 8 se activa con el coche del jugador que no es un reflejo (0x6CFAD0).
void MaybeChangeViewMode() {
    eView *viewp1;
    eView *viewr1;
    eView *view4;
    eView *view8;
    eRenderTarget *targetp1;
    eRenderTarget *targetr1;
    eRenderTarget *target4;
    int screen_width;
    int screen_height;

    if (TweakerViewMode != EVIEWMODE_NONE) {
        if (RenderingViewMode != TweakerViewMode) {
            RenderingViewMode = TweakerViewMode;
        }
    } else {
        if (RenderingViewMode != CurrentViewMode) {
            RenderingViewMode = CurrentViewMode;
        }
    }

    viewp1 = eGetView(EVIEW_PLAYER1, false);
    viewr1 = eGetView(EVIEW_PLAYER1_RVM, false);
    view4 = eGetView(EVIEW_PLAYER1_REFLECTION, false);
    view8 = eGetView(EVIEW_PLAYER1_SPECULAR, false);

    if (CurrentViewMode == EVIEWMODE_ONE) {
        cmp = viewp1->GetCameraMover();

        if (viewp1->GetCameraMover() && viewr1->GetCameraMover() && !viewp1->GetCameraMover()->OutsidePOV() &&
            IsRearViewMirrorAllowed(1)) {
            CurrentViewMode = EVIEWMODE_ONE_RVM;
        }
    } else if (CurrentViewMode == EVIEWMODE_ONE_RVM) {
        if (!viewr1->GetCameraMover() || viewp1->GetCameraMover()->OutsidePOV() || !IsRearViewMirrorAllowed(1)) {
            CurrentViewMode = EVIEWMODE_ONE;
        }
    }

    targetp1 = viewp1->GetRenderTarget0();
    targetr1 = viewr1->GetRenderTarget0();
    target4 = view4->GetRenderTarget0();
    screen_width = __window_width;
    screen_height = __window_height;

    switch (RenderingViewMode) {
        case EVIEWMODE_ONE:
            targetp1->FrameWidth = screen_width;
            targetp1->FrameHeight = screen_height;
            targetp1->SetActive(1);
            viewp1->SetActive(1);
            targetr1->SetActive(0);
            viewr1->SetActive(0);
            target4->FrameWidth = 320;
            target4->FrameHeight = 240;
            target4->SetActive(1);
            view4->SetActive(1);
            view8->SetActive(1);
            break;
        case EVIEWMODE_ONE_RVM:
            targetp1->FrameWidth = screen_width;
            targetp1->FrameHeight = screen_height;
            targetp1->SetActive(1);
            viewp1->SetActive(1);
            targetr1->SetActive(1);
            viewr1->SetActive(1);
            target4->FrameWidth = 320;
            target4->FrameHeight = 240;
            target4->SetActive(1);
            view4->SetActive(1);
            view8->SetActive(1);
            break;
        case EVIEWMODE_NONE:
            targetp1->SetActive(0);
            viewp1->SetActive(0);
            targetr1->SetActive(0);
            viewr1->SetActive(0);
            target4->SetActive(0);
            view4->SetActive(0);
            view8->SetActive(0);
            break;
    }

    if (!TheGameFlowManager.IsInGame() && IsAnyFrontEndCars()) {
        for (FrontEndRenderingCar *car = FrontEndRenderingCarList.GetHead(); car != FrontEndRenderingCarList.EndOfList();
             car = car->GetNext()) {
            if (!car->Reflection && car->ViewID == EVIEW_PLAYER1) {
                view8->SetActive(car->GetRenderInfo() != nullptr);
            }
        }
    }
}

// La fuerza de las sombras de PC con el sol (nombres nuestros; entre parentesis, la direccion en speed.exe): la luz
// directa crece con la altura del sol entre eShadowSunLow y eShadowSunHigh y se apaga con la nubosidad; la fuerza va de
// eShadowStrengthMin a eShadowStrengthMax con ella. La lee el render de PC (el alfa de la sombra del coche y otras).
float eShadowStrengthMin = 0.1f;   // (0x9018E4)
float eShadowStrengthMax = 0.9f;   // (0x9018E8)
float eShadowSunHigh = 0.677f;     // (0x9018EC)
float eShadowSunLow = 0.14f;       // (0x9018F0)
float eSunShadowAmount = 1.0f;     // (0x8FAE64) de 0 (sol bajo o cielo cubierto) a 1
float eShadowStrength = 1.0f;      // (0x8FAE68)

// (0x6C83D0) La llama el cuadro de PC justo despues de mover la hora del dia; la nubosidad es la de la vista 1.
void eUpdateShadowStrength() {
    float sun_height = TimeOfDayPointer->SunDirection.z - eShadowSunLow;
    float cloud_intensity = eGetView(EVIEW_PLAYER1, false)->Precipitation->GetCloudIntensity();
    float sun_range = eShadowSunHigh - eShadowSunLow;
    if (sun_height < 0.0f) {
        sun_height = 0.0f;
    }
    if (sun_height > sun_range) {
        sun_height = sun_range;
    }
    sun_height /= sun_range;
    eSunShadowAmount = sun_height * (1.0f - cloud_intensity);
    eShadowStrength = (eShadowStrengthMax - eShadowStrengthMin) * eSunShadowAmount + eShadowStrengthMin;
}

// PC no tiene sombras de la consola: el tamano nominal es 1024x1024.
int GetShadowWidth() {
    return 1024;
}

int GetShadowHeight() {
    return 1024;
}

unsigned int RCMP_GetMaxFramesOutStanding() {
    return 2;
}

// Las matrices de la vista de PC (nombres nuestros de lo que el exe no nombra; entre parentesis, la direccion).
extern float eZBias;              // (0x8F9210) el sesgo de Z de WorldScreenMatrix

// Sin camara, las dos primeras a la identidad. Con camara: la de la camara, la proyeccion de D3D (Z de 0 a 1) con el
// aspecto del destino (las vistas 1, 2, 4 y 5 lo estiran un 15 %; la 8 y la 9 lo fijan), la de recorte y, si se piden,
// los seis planos de recorte normalizados y la de pantalla con el sesgo de Z (0x6CF400).
void eViewPlatInfo::CalculateViewMatricies(eView *view, float force_near_z, float force_far_z, float force_screen_far_z,
                                           bool clipping_planes) {
    eZBias = -0.000033333333f;
    Camera *camera = view->pCamera;
    if (!camera) {
        bIdentity(&WorldViewMatrix);
        bIdentity(&ViewScreenMatrix);
        return;
    }

    bCopy(&WorldViewMatrix, camera->GetCameraMatrix());
    float near_z = camera->GetNearZ();
    float far_z;
    if (view->ID != EVIEW_PLAYER1) {
        far_z = camera->GetFarZ();
    }
    if (force_near_z != 0.0) {
        near_z = force_near_z;
    }
    if (force_far_z != 0.0) {
        far_z = force_far_z;
    }

    eRenderTarget *render_target = view->GetRenderTarget();
    float aspect = (float)render_target->FrameHeight / render_target->FrameWidth;
    if (view->ID == EVIEW_PLAYER1 || view->ID == EVIEW_PLAYER1_REFLECTION || view->ID == EVIEW_PLAYER2 ||
        view->ID == EVIEW_PLAYER2_REFLECTION) {
        aspect *= 1.15f;
    }
    if (view->ID == EVIEW_PLAYER1_SPECULAR || view->ID == EVIEW_PLAYER2_SPECULAR) {
        aspect = 0.6002401f;
    }

    bAngle half_fov = (bAngle)(camera->GetFieldOfView() * 0.5f);
    float x_scale = 1.0f / (bSin(half_fov) / bCos(half_fov));
    bAngle half_fov_y = (bAngle)(camera->GetFieldOfView() * aspect * 0.5f);
    float y_scale = 1.0f / (bSin(half_fov_y) / bCos(half_fov_y));

    bIdentity(&ViewScreenMatrix);
    ViewScreenMatrix.v0.x = x_scale;
    ViewScreenMatrix.v2.w = 1.0f;
    ViewScreenMatrix.v3.w = 0.0f;
    float q = far_z / (far_z - near_z);
    ViewScreenMatrix.v1.y = -y_scale;
    ViewScreenMatrix.v2.z = q;
    ViewScreenMatrix.v3.z = -(q * near_z);
    view->H = CalculateH(camera->GetFieldOfView());

    WorldViewMatrix.v0.w = 0.0f;
    WorldViewMatrix.v1.w = 0.0f;
    WorldViewMatrix.v2.w = 0.0f;
    D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX *>(&WorldClipMatrix), reinterpret_cast<D3DXMATRIX *>(&WorldViewMatrix),
                       reinterpret_cast<D3DXMATRIX *>(&ViewScreenMatrix));

    if (clipping_planes) {
        for (int i = 0; i <= 5; i++) {
            switch (i) {
                case 0:
                    ClippingPlanes[0].x = WorldClipMatrix.v0.w + WorldClipMatrix.v0.x;
                    ClippingPlanes[0].y = WorldClipMatrix.v1.w + WorldClipMatrix.v1.x;
                    ClippingPlanes[0].z = WorldClipMatrix.v2.w + WorldClipMatrix.v2.x;
                    ClippingPlanes[0].w = WorldClipMatrix.v3.w + WorldClipMatrix.v3.x;
                    break;
                case 1:
                    ClippingPlanes[1].x = WorldClipMatrix.v0.w - WorldClipMatrix.v0.x;
                    ClippingPlanes[1].y = WorldClipMatrix.v1.w - WorldClipMatrix.v1.x;
                    ClippingPlanes[1].z = WorldClipMatrix.v2.w - WorldClipMatrix.v2.x;
                    ClippingPlanes[1].w = WorldClipMatrix.v3.w - WorldClipMatrix.v3.x;
                    break;
                case 2:
                    ClippingPlanes[2].x = WorldClipMatrix.v0.w + WorldClipMatrix.v0.y;
                    ClippingPlanes[2].y = WorldClipMatrix.v1.w + WorldClipMatrix.v1.y;
                    ClippingPlanes[2].z = WorldClipMatrix.v2.w + WorldClipMatrix.v2.y;
                    ClippingPlanes[2].w = WorldClipMatrix.v3.w + WorldClipMatrix.v3.y;
                    break;
                case 3:
                    ClippingPlanes[3].x = WorldClipMatrix.v0.w - WorldClipMatrix.v0.y;
                    ClippingPlanes[3].y = WorldClipMatrix.v1.w - WorldClipMatrix.v1.y;
                    ClippingPlanes[3].z = WorldClipMatrix.v2.w - WorldClipMatrix.v2.y;
                    ClippingPlanes[3].w = WorldClipMatrix.v3.w - WorldClipMatrix.v3.y;
                    break;
                case 4:
                    ClippingPlanes[i].x = WorldClipMatrix.v0.z;
                    ClippingPlanes[i].y = WorldClipMatrix.v1.z;
                    ClippingPlanes[i].z = WorldClipMatrix.v2.z;
                    ClippingPlanes[i].w = WorldClipMatrix.v3.z;
                    break;
                case 5:
                    ClippingPlanes[5].x = WorldClipMatrix.v0.w - WorldClipMatrix.v0.z;
                    ClippingPlanes[5].y = WorldClipMatrix.v1.w - WorldClipMatrix.v1.z;
                    ClippingPlanes[5].z = WorldClipMatrix.v2.w - WorldClipMatrix.v2.z;
                    ClippingPlanes[5].w = WorldClipMatrix.v3.w - WorldClipMatrix.v3.z;
                    break;
            }
            float inv_length = 1.0f / bLength(reinterpret_cast<bVector3 *>(&ClippingPlanes[i]));
            ClippingPlanes[i].x *= inv_length;
            ClippingPlanes[i].y *= inv_length;
            ClippingPlanes[i].z *= inv_length;
            ClippingPlanes[i].w *= inv_length;
        }

        bCopy(&WorldScreenMatrix, &ViewScreenMatrix);
        WorldScreenMatrix.v2.z += eZBias;
        D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX *>(&Unknown100), reinterpret_cast<D3DXMATRIX *>(&WorldViewMatrix),
                           reinterpret_cast<D3DXMATRIX *>(&WorldScreenMatrix));
    }
}
