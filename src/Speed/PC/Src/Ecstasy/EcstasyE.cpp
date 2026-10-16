// EcstasyE.cpp de PC: lo de la capa de Ecstasy de PC que en GameCube y PS2 esta en su EcstasyE.cpp (speed.exe
// 0x6BF3E0-0x6C0AA0: declaraciones de vertices, tablas, ventana, vistas y el constructor de ePoly).
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/PC/Src/Ecstasy/D3D9Min.hpp"
#include "Speed/PC/Src/Ecstasy/TextureInfoPlat.hpp"
#include "Speed/PC/Src/Ecstasy/eViewPlat.hpp"
#include "Speed/PC/Src/Ecstasy/eEffect.hpp"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/World/FacePixelate.hpp"
#include "Speed/Indep/Src/World/Rain.hpp"
#include "Speed/Indep/Src/World/ScreenEffects.hpp"
#include "Speed/Indep/Src/World/VisualTreatment.h"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/Indep/Src/Ecstasy/eEnvMap.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/World/Scenery.hpp"
#include "Speed/Indep/Src/World/TimeOfDay.hpp"

#include <string.h>

// El mapa de entorno del garaje en PC es un cubo: seis texturas (una por cara) que se copian en una textura cubica
// cuando han llegado todas (nombres nuestros de los globales; entre parentesis, la direccion en speed.exe).
extern IDirect3DCubeTexture9 *eFEEnvMapCubeTexture; // (0x982A64)
extern bool eFEEnvMapEnabled;                      // (0x8F945B)
static const char *EnvMapFaceNames[6] = {"XPOS", "XNEG", "YPOS", "YNEG", "ZPOS", "ZNEG"};
unsigned char NumEnvMapFacesLoaded;                // (0x93E86C)

// Las dos declaraciones de vertices fijas de PC: la de 36 bytes (posicion, normal, color y UV) y la de pantalla de
// 24 bytes (posicion transformada, color y UV) (0x6BF3E0; nombres nuestros: 0x93CB2C y 0x93CC60).
extern IDirect3DDevice9 *g_D3DDevice;
extern IDirect3DVertexDeclaration9 *eVertexDeclPositionNormalColourUV;
extern IDirect3DVertexDeclaration9 *eVertexDeclScreenColourUV;

void CreateVertexDeclarations() {
    D3DVERTEXELEMENT9 position_normal_colour_uv[] = {
        {0, 0, 2, 0, 0, 0},     // FLOAT3, POSITION
        {0, 12, 2, 0, 3, 0},    // FLOAT3, NORMAL
        {0, 24, 4, 0, 10, 0},   // D3DCOLOR, COLOR
        {0, 28, 1, 0, 5, 0},    // FLOAT2, TEXCOORD
        {0xFF, 0, 17, 0, 0, 0}, // D3DDECL_END
    };
    g_D3DDevice->CreateVertexDeclaration(position_normal_colour_uv, &eVertexDeclPositionNormalColourUV);

    D3DVERTEXELEMENT9 screen_colour_uv[] = {
        {0, 0, 3, 0, 9, 0},     // FLOAT4, POSITIONT
        {0, 16, 4, 0, 10, 0},   // D3DCOLOR, COLOR
        {0, 20, 1, 0, 5, 0},    // FLOAT2, TEXCOORD
        {0xFF, 0, 17, 0, 0, 0}, // D3DDECL_END
    };
    g_D3DDevice->CreateVertexDeclaration(screen_colour_uv, &eVertexDeclScreenColourUV);
}

TextureInfo *pTextureInfoRVMInfo = 0;
TextureInfo *pTextureInfoRVMMask = 0;
TextureInfo *pTextureInfoWhite16x16NoAlpha = 0;

// PC busca las tres por su nombre (en GameCube, RVM y RVM_MASK van con el hash escrito) (0x6BF540).
void eFixUpTablesPlat() {
    pTextureInfoRVMInfo = GetTextureInfo(bStringHash("RVM"), 0, 0);
    pTextureInfoRVMMask = GetTextureInfo(bStringHash("RVM_MASK"), 0, 0);
    pTextureInfoWhite16x16NoAlpha = GetTextureInfo(bStringHash("WHITE16X16_NOALPHA"), 0, 0);
}

// El mapa de entorno del coche en PC. Las cuatro primeras globales, como en GameCube; las siguientes son de PC (nombres
// nuestros; entre parentesis, la direccion en speed.exe).
extern bMatrix4 hack_man_matrix;       // (0x987FC0)
extern bVector3 envmap_fakeup;         // (0x901760)
extern int EnableEnvMap;               // (0x9017B8) 0 sin mapa de entorno; 1, 2 y 3, la calidad de las opciones
extern float EnvMapNearZ;              // (0x8F9010) 0.5
extern float EnvMapFarZ;               // (0x8F9014) 100
extern int EnableEnvMapFacesNum[3];    // (0x8F901C) {6, 4, 1}
extern int EnableEnvMapFaces[3][6][6]; // (0x8F9028) las caras que se dibujan en cada cuadro, por calidad
extern unsigned short EnvMapFieldOfView; // (0x987DB8)

// En PC las seis caras salen de la camara del jugador 1 sin traslacion: media vuelta en Y, el eje Y invertido y el
// giro de cada cara segun el destino de render de su vista (5 a 10); la traslacion es la posicion del mapa de entorno
// en la base de la camara. La primera parte (la base y la mirada hacia el coche, como en GameCube) sigue calculandose
// aunque ya no se use, salvo hack_man_matrix (0x6DA350).
void eEnvMap::UpdateCameras(bVector3 *viewer_world_position, bVector3 *envmap_world_position) {
    bMatrix4 camera_basis;
    bVector3 a(0.0f, 0.0f, 0.0f);
    bVector3 b(1.0f, 0.0f, 0.0f);

    eCreateLookAtMatrix(&camera_basis, a, b, envmap_fakeup);

    camera_basis.v0.w = 0.0f;
    camera_basis.v1.w = 0.0f;
    camera_basis.v2.w = 0.0f;
    camera_basis.v3.x = 0.0f;
    camera_basis.v3.y = 0.0f;
    camera_basis.v3.z = 0.0f;
    camera_basis.v3.w = 1.0f;

    bMatrix4 camera_lookat_car;

    eCreateLookAtMatrix(&camera_lookat_car, *viewer_world_position, *envmap_world_position, envmap_fakeup);

    camera_lookat_car.v0.w = 0.0f;
    camera_lookat_car.v1.w = 0.0f;
    camera_lookat_car.v2.w = 0.0f;
    camera_lookat_car.v3.x = 0.0f;
    camera_lookat_car.v3.y = 0.0f;
    camera_lookat_car.v3.z = 0.0f;
    camera_lookat_car.v3.w = 1.0f;

    hack_man_matrix = camera_lookat_car;

    bMatrix4 orientation;

    eTransposeMatrix(&orientation, &camera_basis);

    eMulMatrix(&orientation, &camera_lookat_car, &orientation);

    orientation.v0.w = 0.0f;
    orientation.v1.w = 0.0f;
    orientation.v2.w = 0.0f;
    orientation.v3.x = 0.0f;
    orientation.v3.y = 0.0f;
    orientation.v3.z = 0.0f;
    orientation.v3.w = 1.0f;

    eTransposeMatrix(&orientation, &orientation);

    int num_enable_slots = EnableEnvMapFacesNum[EnableEnvMap - 1];
    int *face_enable_state = EnableEnvMapFaces[EnableEnvMap - 1][eFrameCounter % num_enable_slots];

    float near_z = EnvMapNearZ;
    float far_z = EnvMapFarZ;

    for (int face = 0; face < 6; face++) {
        eView *view = this->Views[face];

        if (view) {
            int view_active = face_enable_state[face];

            view->SetActive(view_active != 0);

            Camera *camera = view->GetCamera();

            if (camera) {
                bMatrix4 matrix;
                bMatrix4 flip;

                bMemCpy(&matrix, eGetView(1, false)->GetCamera()->GetCameraMatrix(), sizeof(matrix));

                matrix.v3.x = 0.0f;
                matrix.v3.y = 0.0f;
                matrix.v3.z = 0.0f;
                matrix.v3.w = 1.0f;
                flip.v0.x = 1.0f;
                flip.v0.y = 0.0f;
                flip.v0.z = 0.0f;
                flip.v0.w = 0.0f;
                flip.v1.x = 0.0f;
                flip.v1.y = -1.0f;
                flip.v1.z = 0.0f;
                flip.v1.w = 0.0f;
                flip.v2.x = 0.0f;
                flip.v2.y = 0.0f;
                flip.v2.z = 1.0f;
                flip.v2.w = 0.0f;
                flip.v3.x = 0.0f;
                flip.v3.y = 0.0f;
                flip.v3.z = 0.0f;
                flip.v3.w = 1.0f;

                switch (view->GetRenderTarget0()->ID) {
                    case 5:
                        eRotateY(&matrix, &matrix, 0x8000);
                        eMulMatrix(&matrix, &matrix, &flip);
                        eRotateY(&matrix, &matrix, 0x4000);
                        break;
                    case 6:
                        eRotateY(&matrix, &matrix, 0x8000);
                        eMulMatrix(&matrix, &matrix, &flip);
                        eRotateY(&matrix, &matrix, 0xC000);
                        break;
                    case 7:
                        eRotateY(&matrix, &matrix, 0x8000);
                        eMulMatrix(&matrix, &matrix, &flip);
                        eRotateY(&matrix, &matrix, 0x8000);
                        eRotateX(&matrix, &matrix, 0xC000);
                        break;
                    case 8:
                        eRotateY(&matrix, &matrix, 0x8000);
                        eMulMatrix(&matrix, &matrix, &flip);
                        eRotateY(&matrix, &matrix, 0x8000);
                        eRotateX(&matrix, &matrix, 0x4000);
                        break;
                    case 9:
                        eRotateY(&matrix, &matrix, 0x8000);
                        eMulMatrix(&matrix, &matrix, &flip);
                        eRotateY(&matrix, &matrix, 0x8000);
                        break;
                    case 10:
                        eRotateY(&matrix, &matrix, 0x8000);
                        eMulMatrix(&matrix, &matrix, &flip);
                        break;
                    default:
                        bMemSet(&matrix, 0, sizeof(matrix));
                        break;
                }

                matrix.v3.x = -(matrix.v1.x * envmap_world_position->y + matrix.v2.x * envmap_world_position->z +
                                matrix.v0.x * envmap_world_position->x);
                matrix.v3.y = -(matrix.v1.y * envmap_world_position->y + matrix.v2.y * envmap_world_position->z +
                                matrix.v0.y * envmap_world_position->x);
                matrix.v3.z = -(matrix.v1.z * envmap_world_position->y + matrix.v2.z * envmap_world_position->z +
                                matrix.v0.z * envmap_world_position->x);

                camera->SetCameraMatrix(matrix, RealTimeElapsed);
                camera->SetRenderDash(0);
                camera->SetFocalDistance(0.0f);
                camera->SetNearZ(near_z);
                camera->SetFarZ(far_z);
                camera->SetFieldOfView(EnvMapFieldOfView);
            }
        }
    }
}

eEnvMap TheOnlyEnvMap; // (0x982D60)

eEnvMap *eGetEnvMap() {
    return &TheOnlyEnvMap;
}

// La parte de PC del tratamiento visual (0x240 bytes; en GameCube esta en eVisualTreatment.cpp). Nombres nuestros
// salvo la clase, la global y el constructor.
struct IVisualTreatmentPlat : public IVisualTreatment {
    IVisualTreatmentPlat();
    ~IVisualTreatmentPlat();

    void GetEffectParameters(); // (0x6D4DC0) los parametros de VisualTreatmentShader

    TextureInfo *UvesVignette;            // offset 0x1F0, size 0x4
    char Unknown1F4[8];                   // offset 0x1F4, size 0x8
    D3DXHANDLE BlackBloomIntensity;       // offset 0x1FC, size 0x4 (los parametros del efecto, con su nombre)
    D3DXHANDLE ColourBloomIntensity;      // offset 0x200, size 0x4
    D3DXHANDLE ColourBloomTint;           // offset 0x204, size 0x4
    D3DXHANDLE Desaturation;              // offset 0x208, size 0x4
    D3DXHANDLE Unknown20C;                // offset 0x20C, size 0x4
    D3DXHANDLE DetailMapIntensity;        // offset 0x210, size 0x4
    D3DXHANDLE VisualEffectBrightness;    // offset 0x214, size 0x4
    D3DXHANDLE VisualEffectRadialBlur;    // offset 0x218, size 0x4
    D3DXHANDLE VisualEffectVignette;      // offset 0x21C, size 0x4
    D3DXHANDLE RadialBlurOffset;          // offset 0x220, size 0x4
    D3DXHANDLE BlackBloomIntensity_0;     // offset 0x224, size 0x4
    D3DXHANDLE BlackBloomIntensity_1;     // offset 0x228, size 0x4
    D3DXHANDLE Desaturation_0;            // offset 0x22C, size 0x4
    D3DXHANDLE Desaturation_1;            // offset 0x230, size 0x4
    char Unknown234[0xC];                 // offset 0x234, size 0xC
};

IVisualTreatmentPlat *pVisualTreatmentPlat; // (0x982AF0)

// PC lo crea la primera vez que se pide (0x6DFAF0); GameCube, en OpenVisualTreatment.
IVisualTreatment *IVisualTreatment::Get() {
    if (!pVisualTreatmentPlat) {
        pVisualTreatmentPlat = new IVisualTreatmentPlat;
    }
    return pVisualTreatmentPlat;
}

// PC no tiene nada que cerrar: un `ret` suelto (0x6C39D0).
void CloseVisualTreatment() {}

// (0x6DC350)
IVisualTreatmentPlat::IVisualTreatmentPlat() {
    UvesVignette = GetTextureInfo(bStringHash("UVESVIGNETTE"), 1, 0);
    GetEffectParameters();
}

void IVisualTreatmentPlat::GetEffectParameters() {
    eEffect *effect = eEffects[16];

    BlackBloomIntensity = effect->pD3DXEffect->GetParameterByName(0, "BlackBloomIntensity");
    ColourBloomIntensity = effect->pD3DXEffect->GetParameterByName(0, "ColourBloomIntensity");
    ColourBloomTint = effect->pD3DXEffect->GetParameterByName(0, "ColourBloomTint");
    Desaturation = effect->pD3DXEffect->GetParameterByName(0, "Desaturation");
    DetailMapIntensity = effect->pD3DXEffect->GetParameterByName(0, "DetailMapIntensity");
    VisualEffectBrightness = effect->pD3DXEffect->GetParameterByName(0, "VisualEffectBrightness");
    VisualEffectRadialBlur = effect->pD3DXEffect->GetParameterByName(0, "VisualEffectRadialBlur");
    VisualEffectVignette = effect->pD3DXEffect->GetParameterByName(0, "VisualEffectVignette");
    RadialBlurOffset = effect->pD3DXEffect->GetParameterByName(0, "RadialBlurOffset");
    BlackBloomIntensity_0 = effect->pD3DXEffect->GetParameterByName(0, "BlackBloomIntensity_0");
    BlackBloomIntensity_1 = effect->pD3DXEffect->GetParameterByName(0, "BlackBloomIntensity_1");
    Desaturation_0 = effect->pD3DXEffect->GetParameterByName(0, "Desaturation_0");
    Desaturation_1 = effect->pD3DXEffect->GetParameterByName(0, "Desaturation_1");
}

// Las camaras de las vistas. Las cuatro primeras, como en GameCube; PC tiene una suelta mas y un array de tres (el
// orden es el de sus constructores en speed.exe, 0x888BE0-0x888C52; los dos ultimos nombres son nuestros).
Camera FlailerCamera;     // (0x9877F0)
Camera Player1Camera;     // (0x986870)
Camera Player2Camera;     // (0x986B30)
Camera Player1RVMCamera;  // (0x983E70)
Camera View8Camera;       // (0x9880E0) la de la vista 8
Camera View15Cameras[3];  // (0x986F70) la primera es la de la vista 15

// PC: sin pantalla partida (la vista 2 no tiene destino ni se activa, pero si su lluvia aunque no haya
// EnableRainIn2P); el retrovisor, el reflejo, la especular y la primera sombra llevan la lluvia de la vista 1
// (0x6C06A0).
void epInitViews(void) {
    {
        eView *view = eGetView(EVIEW_FLAILER, false);
        view->SetCamera(&FlailerCamera);
        view->SetRenderTarget0(eGetRenderTarget(2));
        view->SetActive(1);
    }
    {
        eView *view = eGetView(EVIEW_PLAYER1, false);
        view->SetCamera(&Player1Camera);
        view->SetRenderTarget0(eGetRenderTarget(0));
        view->SetActive(1);
        view->ScreenEffects = new ScreenEffectDB();
        view->ScreenEffects->SetMyView(view);
        view->Precipitation = new Rain(view, RAIN);
        view->facePixelation = new FacePixelation(view);
    }
    {
        eView *view = eGetView(EVIEW_PLAYER2, false);
        view->SetCamera(&Player2Camera);
        view->ScreenEffects = new ScreenEffectDB();
        view->ScreenEffects->SetMyView(view);
        view->Precipitation = new Rain(view, RAIN);
    }
    {
        eView *view = eGetView(EVIEW_PLAYER1_SPECULAR, false);
        view->SetCamera(&View8Camera);
        view->SetRenderTarget0(eGetRenderTarget(0));
        view->SetActive(1);
    }
    {
        eView *view = eGetView(EVIEW_PLAYER1_RVM, false);
        view->SetCamera(&Player1RVMCamera);
        view->SetRenderTarget0(eGetRenderTarget(3));
        view->Precipitation = eGetView(EVIEW_PLAYER1, false)->Precipitation;
        view->facePixelation = nullptr;
        view->SetActive(1);
    }
    {
        eView *view = eGetView(EVIEW_PLAYER1_REFLECTION, false);
        view->SetRenderTarget0(eGetRenderTarget(1));
        view->SetCamera(&Player1Camera);
        view->Precipitation = eGetView(EVIEW_PLAYER1, false)->Precipitation;
        view->SetActive(1);
    }
    {
        eView *view = eGetView(EVIEW_SHADOWMAP1, false);
        view->SetRenderTarget0(eGetRenderTarget(4));
        view->SetCamera(&View15Cameras[0]);
        view->Precipitation = eGetView(EVIEW_PLAYER1, false)->Precipitation;
        view->SetActive(1);
    }
}

// Los interruptores del culling del escenario de PC (nombres nuestros; entre parentesis, la direccion en speed.exe).
extern int eDrawWorld;                    // (0x8F9330)
extern int eRVMSceneryExcludeFlags;       // (0x982AF4) las vistas de los jugadores excluyen como el retrovisor
extern int eEnableRoadReflection;         // (0x9017D4) la vista 4, el reflejo de la carretera mojada
extern int eEnableView8;                  // (0x9017C4)
extern int eLodEnabled;                   // (0x901830)
void SetupSceneryCullInfo(eView *view, SceneryCullInfo &info, int exclude_flags); // (0x4FAE40)

// El principio del eDisplayFrame de GameCube, aparte en PC: una SceneryCullInfo por vista activa con destino activo
// (jugadores y retrovisor, las seis del mapa de entorno, el reflejo de la carretera si esta mojada, la vista 8 y la
// 15) y el culling de todas (0x6BFDD0).
void eSetupSceneryCulling(GrandSceneryCullInfo *grand_scenery_cull_info) {
    grand_scenery_cull_info->Init();
    if (eDrawWorld && TheGameFlowManager.IsInGame()) {
        for (int view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER1_RVM; view_id++) {
            eView *view = eGetView(view_id, false);
            if (view->IsActive() && view->GetRenderTarget()->IsActive()) {
                SceneryCullInfo *scenery_cull_info = grand_scenery_cull_info->AddCullInfo();
                if (view_id == EVIEW_PLAYER1_RVM || eRVMSceneryExcludeFlags) {
                    SetupSceneryCullInfo(view, *scenery_cull_info, 0x1000);
                } else {
                    SetupSceneryCullInfo(view, *scenery_cull_info, view_id == EVIEW_PLAYER1_RVM ? 0x2000 : 0x6000);
                }
            }
        }
        if (EnableEnvMap) {
            for (int view_id = EVIEW_FIRST_ENVMAP; view_id <= EVIEW_LAST_ENVMAP; view_id++) {
                eView *view = eGetView(view_id, false);
                if (view->IsActive() && view->GetRenderTarget()->IsActive()) {
                    SceneryCullInfo *scenery_cull_info = grand_scenery_cull_info->AddCullInfo();
                    if (eLodEnabled < 2) {
                        SetupSceneryCullInfo(view, *scenery_cull_info, 0x1100);
                    } else {
                        SetupSceneryCullInfo(view, *scenery_cull_info, 0x1000);
                    }
                }
            }
        }
        if (eEnableRoadReflection && eGetView(EVIEW_PLAYER1, false)->Precipitation->GetRoadDampness() > 0.01f) {
            eView *view = eGetView(EVIEW_PLAYER1_REFLECTION, false);
            if (view->IsActive() && view->GetRenderTarget()->IsActive()) {
                SceneryCullInfo *scenery_cull_info = grand_scenery_cull_info->AddCullInfo();
                SetupSceneryCullInfo(view, *scenery_cull_info, 0x800);
            }
        }
        if (eEnableView8) {
            eView *view = eGetView(EVIEW_PLAYER1_SPECULAR, false);
            if (view->IsActive()) {
                SceneryCullInfo *scenery_cull_info = grand_scenery_cull_info->AddCullInfo();
                SetupSceneryCullInfo(view, *scenery_cull_info, 0);
            }
        }
        if (eLodEnabled > 0) {
            eView *view = eGetView(EVIEW_SHADOWMAP1, false);
            if (view->IsActive() && view->GetRenderTarget()->IsActive()) {
                SceneryCullInfo *scenery_cull_info = grand_scenery_cull_info->AddCullInfo();
                SetupSceneryCullInfo(view, *scenery_cull_info, 0x4100);
            }
        }
    }
    grand_scenery_cull_info->DoCulling();
}

// PC calcula la matriz de fachada (la identidad girada en Z hacia donde mira la vista) pero no la guarda: solo queda
// la vista (0x6BFD00).
void SetupSceneryCullInfoPlat(eView *view, SceneryCullInfo &info) {
    bMatrix4 *world_view = &view->GetPlatInfo()->WorldViewMatrix;
    bAngle angle = bATan(world_view->v0.z, world_view->v0.x);
    bMatrix4 facade_matrix;
    bMatrix4 rotation;

    bIdentity(&facade_matrix);
    eCreateRotationZ(&rotation, angle);
    eMulMatrix(&facade_matrix, &facade_matrix, &rotation);
    info.pView = view;
}

// Como en GameCube (0x6C6770; symbols.json lo llamaba DrawXenonEmitters).
// total size: 0x20
struct ParticleSystemStats {
    int num_particles;
    int max_num_particles;
    int num_particle_textures;
    int max_num_particle_textures;
    int num_emitters;
    int max_num_emitters;
    int num_groups;
    int max_num_groups;

    ParticleSystemStats()
    { num_particles = max_num_particles = num_particle_textures = max_num_particle_textures = num_emitters = max_num_emitters = num_groups = max_num_groups = 0; }
};

static ParticleSystemStats g_ParticleStats; // (0x987B34)

void SetParticleSystemStats(int num_particles, int max_num_particles, int num_particle_textures, int max_num_particle_textures, int num_emitters,
                            int max_num_emitters, int num_groups, int max_num_groups) {
    g_ParticleStats.num_particles = num_particles;
    g_ParticleStats.max_num_particles = max_num_particles;
    g_ParticleStats.num_particle_textures = num_particle_textures;
    g_ParticleStats.max_num_particle_textures = max_num_particle_textures;
    g_ParticleStats.num_emitters = num_emitters;
    g_ParticleStats.max_num_emitters = max_num_emitters;
    g_ParticleStats.num_groups = num_groups;
    g_ParticleStats.max_num_groups = max_num_groups;
}

// Como en GameCube, sin InitSlotPoolsEx (0x6C08A0).
void InitSlotPools(void) {
    eAnimTextureSlotPool = bNewSlotPool(8, 256, "eAnimTextureSlotPool", 0);
    eAnimTextureSlotPool->ClearFlag(SLOTPOOL_FLAG_ZERO_ALLOCATED_MEMORY);
}

extern int eVisualTreatment; // (0x901828) la opcion de video "Visual Treatment" (nombre nuestro)

// (0x6C0A20; nombre nuestro) Con ella, la hora del dia usa las luces A (TimeOfDay::UpdateLighting).
bool eIsVisualTreatmentEnabled() {
    return eVisualTreatment != 0;
}

// Como en GameCube, salvo el ultimo byte: PC pone a cero los flags (0x90), no el Flailer (0x6C0A30).
ePoly::ePoly() {
    Vertices[0].x = 0.0f;
    Vertices[0].y = 0.0f;
    Vertices[0].z = 0.0f;
    Vertices[1].x = 64.0f;
    Vertices[1].y = 0.0f;
    Vertices[1].z = 0.0f;
    Vertices[2].x = 64.0f;
    Vertices[2].y = 64.0f;
    Vertices[2].z = 0.0f;
    Vertices[3].x = 0.0f;
    Vertices[3].y = 64.0f;
    Vertices[3].z = 0.0f;

    *reinterpret_cast<unsigned int *>(Colours[0]) = 0x80808080;
    *reinterpret_cast<unsigned int *>(Colours[1]) = 0x80808080;
    *reinterpret_cast<unsigned int *>(Colours[2]) = 0x80808080;
    *reinterpret_cast<unsigned int *>(Colours[3]) = 0x80808080;

    UVs[0][0] = 0.0f;
    UVs[0][1] = 0.0f;
    UVs[1][0] = 1.0f;
    UVs[1][1] = 0.0f;
    UVs[2][0] = 1.0f;
    UVs[2][1] = 1.0f;
    UVs[3][0] = 0.0f;
    UVs[3][1] = 1.0f;

    SetFlags(0);
}

void EnvMapTextureLoadedCallback(void *param) {
    TextureInfo *face_textures[6];
    char buffer[64];

    NumEnvMapFacesLoaded++;
    if (NumEnvMapFacesLoaded >= 6) {
        for (int face = 0; face < 6; face++) {
            bSPrintf(buffer, "%s_%s", FEManager::Get()->GetGaragePrefixFromType(FEManager::Get()->GetGarageType()),
                     EnvMapFaceNames[face]);
            TextureInfo *texture_info = GetTextureInfo(bStringHash(buffer), 1, 0);
            D3DLOCKED_RECT source;
            D3DLOCKED_RECT dest;

            face_textures[face] = texture_info;
            texture_info->GetPlatInfo()->pD3DTexture->LockRect(0, &source, 0, 0);
            eFEEnvMapCubeTexture->LockRect(face, 0, &dest, 0, 0);
            memcpy(dest.pBits, source.pBits, texture_info->BaseImageSize);
            texture_info->GetPlatInfo()->pD3DTexture->UnlockRect(0);
            eFEEnvMapCubeTexture->UnlockRect(face, 0);
        }
    }
}

void eInitFEEnvMapPlat() {
    char buffer[64];

    if (eFEEnvMapEnabled) {
        NumEnvMapFacesLoaded = 0;
        for (int face = 0; face < 6; face++) {
            bSPrintf(buffer, "%s_%s", FEManager::Get()->GetGaragePrefixFromType(FEManager::Get()->GetGarageType()),
                     EnvMapFaceNames[face]);
            unsigned int hash = bStringHash(buffer);
            eLoadStreamingTexture(&hash, 1, EnvMapTextureLoadedCallback, 0, 0);
        }
    }
}

void eRemoveFEEnvMapPlat() {
    char buffer[64];

    for (int face = 0; face < 6; face++) {
        if (FEManager::Get()->GetPreviousGarageType() != 0) {
            bSPrintf(buffer, "%s_%s", FEManager::Get()->GetGaragePrefixFromType(FEManager::Get()->GetPreviousGarageType()),
                     EnvMapFaceNames[face]);
            unsigned int hash = bStringHash(buffer);
            if (GetTextureInfo(hash, 0, 0)) {
                eUnloadStreamingTexture(hash);
            }
        }
        if (FEManager::Get()->GetGarageType() != 0) {
            bSPrintf(buffer, "%s_%s", FEManager::Get()->GetGaragePrefixFromType(FEManager::Get()->GetGarageType()),
                     EnvMapFaceNames[face]);
            unsigned int hash = bStringHash(buffer);
            if (GetTextureInfo(hash, 0, 0)) {
                eUnloadStreamingTexture(hash);
            }
        }
    }
}

// El vertex buffer dinamico de las particulas de PC: cuatro vertices de 24 bytes por particula, bloqueado por tramos
// con DISCARD/NOOVERWRITE (nombres nuestros; los metodos estan en 0x6C0E80-0x6C1160; el objeto, en 0x93CAF4).
// total size: 0x38
class eDynamicVertexBuffer {
  public:
    void Init(const DWORD &fvf, const unsigned int &max_particles); // (0x6C0E80)
    unsigned char *Begin(const int &num_particles);                  // (0x6C0F80) bloquea sitio para num_particles
    void Flush();                                                    // (0x6C1010) dibuja lo bloqueado

    void Unlock() {
        if (Locked && VertexBuffer) {
            VertexBuffer->Unlock();
            Locked = false;
        }
    }

    IDirect3DVertexBuffer9 *VertexBuffer; // offset 0x0, size 0x4
    IDirect3DIndexBuffer9 *IndexBuffer;   // offset 0x4, size 0x4 (seis indices por particula, fijos)
    unsigned int MaxParticles;            // offset 0x8, size 0x4
    unsigned int MaxVerts;                // offset 0xC, size 0x4
    unsigned int NextVert;                // offset 0x10, size 0x4
    unsigned int NumVerts;                // offset 0x14, size 0x4
    unsigned int StartVert;               // offset 0x18, size 0x4
    bool Additive;                        // offset 0x1C, size 0x1 (la ultima particula pedia mezcla aditiva)
    unsigned int VertexSize;              // offset 0x20, size 0x4
    bool Locked;                          // offset 0x24, size 0x1
    bool Discard;                         // offset 0x25, size 0x1
    unsigned char *Cursor;                // offset 0x28, size 0x4
    unsigned char *LockedEnd;             // offset 0x2C, size 0x4
    unsigned char *LockedData;            // offset 0x30, size 0x4
    bool Used;                            // offset 0x34, size 0x1
};

void eSetAlphaBlend(int enable, int src_blend, int dst_blend); // (0x6C6810)

void eDynamicVertexBuffer::Init(const DWORD &fvf, const unsigned int &max_particles) {
    IDirect3DDevice9 *device = g_D3DDevice;

    MaxParticles = max_particles;
    VertexBuffer = 0;
    NextVert = 0;
    Discard = true;
    Locked = false;
    Additive = false;
    MaxVerts = max_particles * 4;
    VertexSize = 24;
    StartVert = 0;
    Cursor = 0;
    LockedEnd = 0;
    device->CreateVertexBuffer((MaxVerts + 10) * 24, 0x208, fvf, D3DPOOL_DEFAULT, &VertexBuffer, 0); // DYNAMIC | WRITEONLY
    device->CreateIndexBuffer(max_particles * 12, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &IndexBuffer, 0);

    unsigned short *indices = 0;
    IndexBuffer->Lock(0, 0, reinterpret_cast<void **>(&indices), 0);
    unsigned short vertex = 0;
    for (unsigned int i = 0; i < max_particles; i++) {
        indices[0] = vertex + 3;
        indices[1] = vertex;
        indices[2] = vertex + 1;
        indices[3] = vertex + 1;
        indices[4] = vertex + 2;
        indices[5] = vertex + 3;
        indices += 6;
        vertex += 4;
    }
    IndexBuffer->Unlock();
}

unsigned char *eDynamicVertexBuffer::Begin(const int &num_particles) {
    Discard = true;
    unsigned int num_verts = num_particles * 4;
    if (num_verts > MaxVerts) {
        if (NextVert > 0) {
            Discard = true;
        }
        num_verts = MaxVerts;
    }
    Used = true;
    NumVerts = num_verts;
    StartVert = 0;
    unsigned char *data = 0;
    if (VertexBuffer) {
        DWORD flags = 0x1800; // NOOVERWRITE | NOSYSLOCK
        if (Discard) {
            flags = 0x2800; // DISCARD | NOSYSLOCK
            Discard = false;
            NextVert = 0;
        }
        if (VertexBuffer->Lock(NextVert * VertexSize, num_verts * VertexSize, reinterpret_cast<void **>(&data), flags) == 0) {
            StartVert = NextVert;
            NextVert += num_verts;
            Locked = true;
            Cursor = data;
            LockedData = data;
            LockedEnd = data + num_verts * 24;
        }
    }
    return data;
}

void eDynamicVertexBuffer::Flush() {
    IDirect3DDevice9 *device = g_D3DDevice;

    device->SetStreamSource(0, VertexBuffer, 0, 24);
    device->SetIndices(IndexBuffer);
    device->SetRenderState(27, 1); // D3DRS_ALPHABLENDENABLE
    if (Additive) {
        eSetAlphaBlend(1, 5, 2); // SRCALPHA, ONE
    }
    g_D3DDevice->SetRenderState(168, 7); // D3DRS_COLORWRITEENABLE: rojo, verde y azul
    int num_primitives = NumVerts / 2;
    int start_index = (StartVert / 4) * 6;
    if (num_primitives > 0) {
        eCurrentEffect->pD3DXEffect->BeginPass(0);
        eCurrentEffect->pD3DXEffect->CommitChanges();
        device->DrawIndexedPrimitive(4, 0, StartVert, NumVerts, start_index, num_primitives); // TRIANGLELIST
        eCurrentEffect->pD3DXEffect->EndPass();
    }
    NumVerts = 0;
}

extern eDynamicVertexBuffer eParticleVertexBuffer; // (0x93CAF4)
// El objeto de las particulas de PC (0x9877CC): su primer byte dice si se esta pintando; EmitterSystem.cpp le pasa cada
// particula con AddParticle (0x6C1160). Mismos nombres (nuestros) que alli.
struct eParticleRenderer {
    bool Rendering;
};
extern eParticleRenderer eParticles;
extern eEffect *eCurrentEffect;                    // (0x982C80)
void eSetTexture(TextureInfo *texture_info, int stage);       // (0x6C6CC0)
void eSetWorldMatrix(bMatrix4 *local_to_world, int view_id); // (0x6C8000)

// Como AcidFX_G.cpp de GameCube, sobre D3D9: sin escribir Z, con la matriz identidad como mundo, la textura en la
// etapa 0 y la primera pasada del efecto actual (0x6CEEE0 y 0x6DA2F0).
bool PlatStartParticleRender(eView *view, TextureInfo *texture, uint32 num_particles) {
    g_D3DDevice->SetRenderState(14, 0); // D3DRS_ZWRITEENABLE
    eParticles.Rendering = true;
    eSetWorldMatrix(eGetIdentityMatrix(), view->ID);
    eSetTexture(texture, 0);
    eParticleVertexBuffer.Begin(num_particles);
    eCurrentEffect->pD3DXEffect->BeginPass(0);
    return true;
}

void PlatEndParticleRender() {
    eParticles.Rendering = false;
    eParticleVertexBuffer.Unlock();
    eParticleVertexBuffer.Flush();
    eCurrentEffect->pD3DXEffect->EndPass();
    g_D3DDevice->SetRenderState(14, 1); // D3DRS_ZWRITEENABLE
}

// Los vectores de la vista para las particulas: las columnas de la WorldViewMatrix (0x6BCC00; en el exe va antes de
// DemoDisc_PC, en un fichero de PC que no tenemos: como GameCube lo tiene en AcidFX_G.cpp, aqui va con la vista).
void PlatGetViewVectors(eView *view, UMath::Vector3 &right, UMath::Vector3 &up, UMath::Vector3 &fwd) {
    bMatrix4 m;

    bCopy(&m, &view->GetPlatInfo()->WorldViewMatrix);
    right.x = m.v0.x;
    right.y = m.v1.x;
    right.z = m.v2.x;
    up.x = m.v0.y;
    up.y = m.v1.y;
    up.z = m.v2.y;
    fwd.x = m.v0.z;
    fwd.y = m.v1.z;
    fwd.z = m.v2.z;
}

// El pool de parametros compartidos de los efectos (0x982C84; nombre nuestro).
extern ID3DXEffectPool *g_ID3DXEffectPool;

// Los manejadores de los parametros del efecto: cada parametro con semantica se apunta en la entrada de la tabla que
// lleva el hash de esa semantica (0x6C5500).
void eEffect::GetParameters() {
    D3DXEFFECT_DESC effect_desc;

    pD3DXEffect->GetDesc(&effect_desc);
    for (UINT i = 0; i < effect_desc.Parameters; i++) {
        D3DXHANDLE handle = pD3DXEffect->GetParameter(0, i);
        D3DXPARAMETER_DESC desc;

        pD3DXEffect->GetParameterDesc(handle, &desc);
        if (desc.Semantic) {
            unsigned int hash = bStringHash(desc.Semantic);
            eEffectParameter *parameters = Parameters;
            for (int j = 0; j < NUM_EFFECT_PARAMETERS; j++) {
                if (parameters[j].NameHash == hash) {
                    parameters[j].Handle = handle;
                    break;
                }
            }
        }
    }
}

// Lo que las fuentes de PC saben de la tarjeta y de las opciones de video (nombres nuestros; entre parentesis, la
// direccion en speed.exe).
extern D3DCAPS9 eD3DCaps;           // (0x93D918)
extern unsigned int GPUVendor;      // (0x93D898)
extern int eTextureFiltering;       // (0x901818) 0 puntual, 1 bilineal, 2 anisotropo
extern int eRenderStates[5];        // (0x93DE20) ALPHATESTENABLE, ALPHAREF, ALPHABLENDENABLE, SRCBLEND y DESTBLEND

#define GPU_VENDOR_MATROX 0x102B

// Los valores de partida de los parametros del efecto: los estados de render y los filtros de textura segun la
// opcion de filtrado y la tarjeta (Matrox sin anisotropo) (0x6C5E60).
void eEffect::SetDefaults() {
    if (Parameters[11].Handle) {
        pD3DXEffect->SetInt(Parameters[11].Handle, 1);
    }
    if (Parameters[3].Handle) {
        eRenderStates[0] = 0;
        eRenderStates[1] = 0;
        eRenderStates[2] = 0;
        eRenderStates[3] = 5; // D3DBLEND_SRCALPHA
        eRenderStates[4] = 6; // D3DBLEND_INVSRCALPHA
        pD3DXEffect->SetIntArray(Parameters[3].Handle, eRenderStates, 5);
    }
    if (Parameters[5].Handle) {
        int filter;
        if (eTextureFiltering > 1 && eTextureFiltering == 2 && (eD3DCaps.TextureFilterCaps & 0x400)) {
            filter = (GPUVendor != GPU_VENDOR_MATROX) + 2;
        } else {
            filter = 2;
        }
        pD3DXEffect->SetInt(Parameters[5].Handle, filter);
    }
    if (Parameters[4].Handle) {
        int filter;
        if (eTextureFiltering > 1 && eTextureFiltering == 2 && (eD3DCaps.TextureFilterCaps & 0x4000000)) {
            filter = (GPUVendor != GPU_VENDOR_MATROX) + 2;
        } else {
            filter = 2;
        }
        pD3DXEffect->SetInt(Parameters[4].Handle, filter);
    }
    if (Parameters[6].Handle) {
        pD3DXEffect->SetInt(Parameters[6].Handle, (eTextureFiltering != 0) + 1);
    }
    if (Parameters[38].Handle) {
        pD3DXEffect->SetInt(Parameters[38].Handle, 1);
    }
    if (Parameters[7].Handle) {
        if (eTextureFiltering == 2) {
            pD3DXEffect->SetInt(Parameters[7].Handle, eD3DCaps.MaxAnisotropy);
        } else {
            pD3DXEffect->SetInt(Parameters[7].Handle, 1);
        }
    }
    if (Parameters[28].Handle) {
        pD3DXEffect->SetInt(Parameters[28].Handle, 15);
    }
    if (Index == 11) {
        pD3DXEffect->SetInt(Parameters[9].Handle, 15);
    }
}

// (0x6C5040; los destructores de las 14 clases de efecto lo llevan en linea: 0x6D64A0...)
eEffect::~eEffect() {
    delete Parameters;
    if (pD3DXEffect) {
        pD3DXEffect->OnLostDevice();
        pD3DXEffect->Release();
        pD3DXEffect = 0;
    }
    if (pVertexDecl) {
        pVertexDecl->Release();
        pVertexDecl = 0;
    }
}

// Crea (o vuelve a crear) el efecto de D3DX desde el recurso del exe (0x6C6080).
void eEffect::Load() {
    ID3DXBuffer *errors;

    if (pD3DXEffect) {
        pD3DXEffect->OnLostDevice();
        pD3DXEffect->Release();
        pD3DXEffect = 0;
    }
    D3DXCreateEffectFromResourceA(g_D3DDevice, 0, eEffectInfoTable[Index].ResourceName, 0, 0, 0, g_ID3DXEffectPool,
                                  &pD3DXEffect, &errors);
    GetParameters();
    SetDefaults();
}

// Suelta los 31 efectos de D3DX y el pool, para volver a cargarlos (0x6C5DA0).
void eReleaseEffects() {
    for (int i = 0; i <= NUM_EFFECTS - 1; i++) {
        eEffect *effect = eEffects[i];
        if (effect->pD3DXEffect) {
            effect->pD3DXEffect->OnLostDevice();
            effect->pD3DXEffect->Release();
            effect->pD3DXEffect = 0;
        }
    }
    if (g_ID3DXEffectPool) {
        g_ID3DXEffectPool->Release();
        g_ID3DXEffectPool = 0;
    }
}

// Antes de reiniciar el dispositivo: los efectos sueltan lo que tienen en video (0x6C5D70; eResetDevice la expande).
void eLoseEffects() {
    for (int i = 0; i < NUM_EFFECTS; i++) {
        eEffects[i]->pD3DXEffect->OnLostDevice();
    }
}

// Borra los efectos cargados y el pool (0x6C5D20; al salir del juego).
void eDeleteEffects() {
    for (int i = 0; i < NUM_EFFECTS; i++) {
        if (eEffects[i]->pD3DXEffect) {
            delete eEffects[i];
            eEffects[i] = 0;
        }
    }
    if (g_ID3DXEffectPool) {
        g_ID3DXEffectPool->Release();
        g_ID3DXEffectPool = 0;
    }
}

// Despues de reiniciar el dispositivo: los efectos recuperan su estado y sus parametros (0x6C5DF0; eResetDevice la
// expande y nadie mas la llama).
void eResetEffects() {
    for (int i = 0; i < NUM_EFFECTS; i++) {
        eEffects[i]->pD3DXEffect->OnResetDevice();
        eEffects[i]->GetParameters();
    }
}

extern int eNumLods;          // (0x982C3C) los LOD de efecto que se usan (las opciones)
extern int EnableCarEnvMap;   // (0x901840)
extern int EnableCarEnvMap2;  // (0x901844)

// Crea los efectos que falten y les pone la tecnica: los que tienen LOD, la del LOD mas alto que exista; si el efecto
// del coche es el de "car_no_env_fix", sin mapa de entorno (0x6D7500).
void eLoadEffects() {
    if (!g_ID3DXEffectPool) {
        D3DXCreateEffectPool(&g_ID3DXEffectPool);
    }
    for (int i = 0; i <= NUM_EFFECTS - 1; i++) {
        eEffect *effect = eEffects[i];
        D3DXTECHNIQUE_DESC desc;

        if (!effect->pD3DXEffect) {
            effect->Load();
        }
        switch (effect->Index) {
            case 0:
            case 1:
            case 3:
            case 4:
            case 5:
            case 18: {
                int lod = eNumLods;
                while (lod >= 0) {
                    if (effect->LodTechniques[lod] != -1) {
                        break;
                    }
                    lod--;
                }
                if (lod >= 0) {
                    effect->TechniqueIndex = effect->LodTechniques[lod];
                    effect->Technique = effect->pD3DXEffect->GetTechnique(effect->TechniqueIndex);
                    effect->Unknown40 = effect->LodFlags[lod] > 0;
                    effect->CurrentLod = lod;
                    effect->pD3DXEffect->SetTechnique(effect->Technique);
                } else {
                    effect->Technique = 0;
                }
                break;
            }
            default:
                if (effect->Technique) {
                    effect->Technique = effect->pD3DXEffect->GetTechnique(effect->TechniqueIndex);
                    effect->pD3DXEffect->SetTechnique(effect->Technique);
                }
                break;
        }
        switch (effect->Index) {
            case 0:
            case 1:
            case 3:
            case 4:
            case 5:
            case 18:
                effect->pD3DXEffect->GetTechniqueDesc(effect->Technique, &desc);
                effect->NumPasses = desc.Passes;
                break;
        }
        if (effect->Index == 4 && bStrCmp(desc.Name, "car_no_env_fix") == 0) {
            EnableCarEnvMap = 0;
            EnableEnvMap = 0;
            EnableCarEnvMap2 = 0;
        }
    }
}

extern int eRenderPass;                   // (0x8FAE6C)
extern int eAntialiasEnabled;             // (0x8F921C)
extern int __multisamplequality;          // (0x901808)
void eSetDefaultAlphaBlend();             // (0x6C6880)

// Los estados de partida del dispositivo: Z, alfa, sin luces ni niebla, y por cada etapa de textura los filtros de la
// opcion de filtrado (como eEffect::SetDefaults) y la etapa sin operaciones de color (0x6CFCE0).
void eSetDefaultRenderStates() {
    g_D3DDevice->SetRenderState(7, 1);    // D3DRS_ZENABLE
    g_D3DDevice->SetRenderState(14, 1);   // D3DRS_ZWRITEENABLE
    g_D3DDevice->SetRenderState(23, 4);   // D3DRS_ZFUNC, D3DCMP_LESSEQUAL
    g_D3DDevice->SetRenderState(25, 5);   // D3DRS_ALPHAFUNC, D3DCMP_GREATER
    g_D3DDevice->SetRenderState(29, 1);   // D3DRS_SPECULARENABLE
    g_D3DDevice->SetRenderState(137, 0);  // D3DRS_LIGHTING
    g_D3DDevice->SetRenderState(28, 0);   // D3DRS_FOGENABLE
    eRenderPass = 2;
    g_D3DDevice->SetRenderState(168, 15); // D3DRS_COLORWRITEENABLE
    for (DWORD i = 0; i < eD3DCaps.MaxTextureBlendStages; i++) {
        int filter;
        if (eTextureFiltering > 1 && eTextureFiltering == 2 && (eD3DCaps.TextureFilterCaps & 0x400)) {
            filter = (GPUVendor != GPU_VENDOR_MATROX) + 2;
        } else {
            filter = 2;
        }
        g_D3DDevice->SetSamplerState(i, 6, filter); // D3DSAMP_MINFILTER
        if (eTextureFiltering > 1 && eTextureFiltering == 2 && (eD3DCaps.TextureFilterCaps & 0x4000000)) {
            filter = (GPUVendor != GPU_VENDOR_MATROX) + 2;
        } else {
            filter = 2;
        }
        g_D3DDevice->SetSamplerState(i, 5, filter); // D3DSAMP_MAGFILTER
        g_D3DDevice->SetSamplerState(i, 7, (eTextureFiltering != 0) + 1); // D3DSAMP_MIPFILTER
        if (eTextureFiltering == 2) {
            g_D3DDevice->SetSamplerState(i, 10, eD3DCaps.MaxAnisotropy); // D3DSAMP_MAXANISOTROPY
        } else {
            g_D3DDevice->SetSamplerState(i, 10, 1);
        }
    }
    if (eAntialiasEnabled && __multisamplequality > 0) {
        g_D3DDevice->SetRenderState(161, 1); // D3DRS_MULTISAMPLEANTIALIAS
    } else {
        g_D3DDevice->SetRenderState(161, 0);
    }
    for (DWORD i = 0; i < eD3DCaps.MaxTextureBlendStages; i++) {
        g_D3DDevice->SetTextureStageState(i, 1, 1);  // D3DTSS_COLOROP, D3DTOP_DISABLE
        g_D3DDevice->SetTextureStageState(i, 4, 1);  // D3DTSS_ALPHAOP, D3DTOP_DISABLE
        g_D3DDevice->SetTextureStageState(i, 24, 0); // D3DTSS_TEXTURETRANSFORMFLAGS
        g_D3DDevice->SetTextureStageState(i, 11, i); // D3DTSS_TEXCOORDINDEX
    }
    eEffects[0]->pD3DXEffect->SetInt(eEffects[0]->Parameters[9].Handle, 7);
    eSetDefaultAlphaBlend();
}

// El cuadro de PC (0x6E7220). Nombres nuestros de lo que el exe no nombra; entre parentesis, la direccion.
extern bool eCreateTimeOfDayPending;      // (0x901AC8) empieza a 1

// La hora del dia de PC (0x6BF290; la pide el primer cuadro).
void eCreateTimeOfDay() {
    TimeOfDayPointer = new TimeOfDay;
}

extern int eReloadEffectsPending;         // (0x982B14)
extern bool eDisplaySettingsChanged;      // (0x982C39)
struct eResolutionTable {
    void GetResolution(int *width, int *height); // (0x6C27D0)
};
extern eResolutionTable eResolutions;     // (0x983E60)
extern int eResolutionMode;               // (0x90181C) el modo de video de las opciones (0..5)

// Tarjetas que piden otro valor (fabricante, dispositivo y el valor), acabada en fabricante 0 (0x8FAF20). Nombres
// nuestros: el arranque (0x6E69F0) lo usa para la calidad del multimuestreo.
struct eGPUScaleEntry {
    int Vendor;
    int Device;
    float Scale;
};
extern eGPUScaleEntry eGPUScaleTable[];

// El valor de la tarjeta, o 1 si no esta en la tabla (0x6CB190).
float eGetGPUScale(int vendor, int device) {
    for (int i = 0; eGPUScaleTable[i].Vendor != 0; i++) {
        if (eGPUScaleTable[i].Vendor == vendor && eGPUScaleTable[i].Device == device) {
            return eGPUScaleTable[i].Scale;
        }
    }
    return 1.0f;
}

// Si la tarjeta esta en la tabla (0x6CB1E0).
bool eIsGPUInScaleTable(int vendor, int device) {
    for (int i = 0; eGPUScaleTable[i].Vendor != 0; i++) {
        if (eGPUScaleTable[i].Vendor == vendor && eGPUScaleTable[i].Device == device) {
            return true;
        }
    }
    return false;
}

// Ancho y alto del modo de video elegido (0x6C27D0).
void eResolutionTable::GetResolution(int *width, int *height) {
    switch (eResolutionMode) {
    case 0:
        *width = 640;
        *height = 480;
        break;
    case 1:
        *width = 800;
        *height = 600;
        break;
    case 2:
        *width = 1024;
        *height = 768;
        break;
    case 3:
        *width = 1280;
        *height = 960;
        break;
    case 4:
        *width = 1280;
        *height = 1024;
        break;
    case 5:
        *width = 1600;
        *height = 1200;
        break;
    }
}
extern int eResolutionIndex;              // (0x901818)
extern int eRefreshRates[];               // (0x9829FC)
void InitWindow(int width, int height, int refresh_rate); // (0x6BFAB0)
extern int SaveStreamAnim;                // (0x982BC0)
extern int eStreamAnimMode;               // (0x982BB8)
extern int eStreamAnimPending;            // (0x982BBC)
void MaybeChangeViewMode();
extern int eShadowDetail;                 // (0x9017F4) la calidad de las sombras de las opciones
extern int eShadowsEnabled;               // (0x982B34)
extern int eShadowsEnabled2;              // (0x9A3A34)
extern int eShadowMapQuality;             // (0x982AE0)
void UpdateEnvironmentMapCameras();
void elBeginFrame();
void eSetupView8(eView *view, int flag);  // (0x6D0B00)
extern float RVMNearZ;                    // (0x901AC4) 0.1
extern int eDepthOfFieldEnabled;          // (0x9017F0) la opcion de profundidad de campo
extern bool eDepthOfFieldActive;          // (0x982B24)
extern bool eDepthOfFieldActive2;         // (0x982B25)
bool GiveTheMoviePlayerBandwidth();
extern int ExitTheGameFlag;
void ___eDisplayFrame();                  // (0x6DE300)
int eGetAverageBatchSize();               // (0x6C6F00)
extern int eScreenshotPending;            // (0x982BF8)
extern int eMovieFramePending;            // (0x9885C4)
void eMovieFrameDone();                   // nullsub_36
void eDisplayFrameDone();                 // nullsub_35
void ePostDisplay();
void eSwapFrameMallocBuffers();
void eSwapDynamicFrameMemory();
extern int eLastRenderedTexture;          // (0x982CA0)
extern int eLastRenderedTexture2;         // (0x982CA4)
// El resplandor de PC: la intensidad del efecto de pantalla SE_GLARE de la vista del jugador (o una fija), suavizada
// cuadro a cuadro, reparte dos valores enteros entre su minimo y su maximo (nombres nuestros; entre parentesis, la
// direccion).
extern bool eGlareForced;          // (0x982C70)
extern float eGlareForcedIntensity; // (0x8F9B58)
extern float eGlareMin;            // (0x8F9460)
extern float eGlareMax;            // (0x8F9464)
extern float eGlareSmoothing;      // (0x9018C0)
extern float eGlareIntensity;      // (0x988544)
extern int eGlareMinA;             // (0x8F947C)
extern int eGlareMaxA;             // (0x8F9480)
extern int eGlareValueA;           // (0x8F9478)
extern int eGlareMinB;             // (0x8F9484)
extern int eGlareMaxB;             // (0x8F9488)
extern int eGlareValueB;           // (0x8F9470)

// (0x6C37F0)
void eUpdateGlare(float intensity) {
    eGlareIntensity = (1.0f - eGlareSmoothing) * eGlareIntensity + eGlareSmoothing * intensity;
    eGlareValueA = (int)((eGlareMaxA - eGlareMinA) * eGlareIntensity + eGlareMinA);
    eGlareValueB = (int)((eGlareMaxB - eGlareMinB) * eGlareIntensity + eGlareMinB);
}

void eRenderFrameEnd(); // (0x6C3870) (ver PROPUESTAS de PCSPLIT44: casa sola, pero eDisplayFrame la expande)

// La pasada de pantalla del cuadro (nombres nuestros; entre parentesis, la direccion): al empezar y al acabar el cuadro,
// el destino 0 entero con las matrices a la identidad y el efecto de los poligonos del FE puesto; al acabar, ademas, un
// rotulo si falla la comprobacion de la proteccion de copia, y el FE.
extern eEffect *eFEEffect;            // (0x93DE9C) eEffects[9]
extern eEffect *eFEMaskEffect;        // (0x93DEA0)
extern int eCopyProtectionKey;        // (0x901794)
extern bool eCopyProtectionWarning;   // (0x919738)
extern IDirect3DBaseTexture9 *eCopyProtectionTexture; // (0x982A7C)
short eCopyProtectionCheck(int a, int b); // (0x6E6380)
void eFlushRenderList();
void eSetWorldMatrix(bMatrix4 *local_to_world, int view_id);
void eSetAlphaTest(int enable, int ref);
void eSetAlphaBlend(int enable, int src_blend, int dst_blend);
void eCommitRenderStates();

// (0x6E6E40)
void eRenderFrame(int begin) {
    D3DXMATRIXA16 identity;
    D3DVIEWPORT9 viewport;

    eSetDefaultRenderStates();
    eFlushRenderList();
    g_D3DDevice->SetRenderTarget(0, eRenderTargets[0].D3DSurface);
    g_D3DDevice->SetDepthStencilSurface(eRenderTargets[0].D3DDepthSurface);
    viewport.Width = eRenderTargets[0].FrameWidth;
    viewport.Height = eRenderTargets[0].FrameHeight;
    viewport.X = 0;
    viewport.Y = 0;
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    g_D3DDevice->SetViewport(&viewport);

    eEffect *effect = eFEEffect;
    eCurrentRenderTarget = &eRenderTargets[0];
    eCurrentEffect = 0;
    effect->pD3DXEffect->SetInt(effect->Parameters[28].Handle, 15);
    eCurrentEffect = effect;
    effect->BeginEffect();
    D3DXMatrixIdentity(&identity);
    eViewPlatInfo *plat_info = eViews[0].GetPlatInfo();
    bMemCpy(&plat_info->WorldViewMatrix, &identity, sizeof(bMatrix4));
    D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX *>(&plat_info->WorldClipMatrix),
                       reinterpret_cast<D3DXMATRIX *>(&plat_info->WorldViewMatrix),
                       reinterpret_cast<D3DXMATRIX *>(&plat_info->ViewScreenMatrix));
    plat_info = eViews[0].GetPlatInfo();
    bMemCpy(&plat_info->ViewScreenMatrix, &identity, sizeof(bMatrix4));
    D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX *>(&plat_info->WorldClipMatrix),
                       reinterpret_cast<D3DXMATRIX *>(&plat_info->WorldViewMatrix),
                       reinterpret_cast<D3DXMATRIX *>(&plat_info->ViewScreenMatrix));
    eSetWorldMatrix(reinterpret_cast<bMatrix4 *>(&identity), 0);
    effect->EndEffect();

    eEffect *mask_effect = eFEMaskEffect;
    eCurrentEffect = mask_effect;
    mask_effect->BeginEffect();
    eSetWorldMatrix(reinterpret_cast<bMatrix4 *>(&identity), 0);
    mask_effect->EndEffect();

    eCurrentEffect = effect;
    effect->BeginEffect();
    g_D3DDevice->SetRenderState(14, 0); // D3DRS_ZWRITEENABLE
    g_D3DDevice->SetRenderState(7, 0);  // D3DRS_ZENABLE
    if (!begin && eCopyProtectionCheck(eCopyProtectionKey, eCopyProtectionKey) == 0x188) {
        if (eCopyProtectionWarning) {
            ePoly poly;
            poly.Vertices[0].x = 450.0f;
            poly.Vertices[0].y = 19.0f;
            poly.Vertices[1].x = 190.0f;
            poly.Vertices[1].y = 19.0f;
            poly.Vertices[2].x = 190.0f;
            poly.Vertices[2].y = 89.0f;
            poly.Vertices[3].x = 450.0f;
            poly.Vertices[3].y = 89.0f;
            for (int j = 0; j < 4; j++) {
                for (int i = 0; i < 4; i++) {
                    poly.Colours[i][j] = 0xFF;
                }
            }
            effect->pD3DXEffect->SetTexture(effect->Parameters[13].Handle, eCopyProtectionTexture);
            g_D3DDevice->SetSamplerState(0, 1, 3); // D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP
            g_D3DDevice->SetSamplerState(0, 2, 3); // D3DSAMP_ADDRESSV
            eSetAlphaTest(0, 1);
            eSetAlphaBlend(1, 5, 6);
            eCommitRenderStates();
            eViewPlatInterface::Render(&poly, 0, 0);
            g_D3DDevice->SetSamplerState(0, 1, 1); // D3DTADDRESS_WRAP
            g_D3DDevice->SetSamplerState(0, 2, 1);
        }
        FEManager::Get()->Render();
    }
    effect->EndEffect();
    eCurrentEffect = 0;
    g_D3DDevice->SetRenderState(7, 1);
    g_D3DDevice->SetRenderState(14, 1);
}

// El raton de PC: la posicion del cursor en la ventana, la rueda y tres botones con su flanco (0x987F50; nombres
// nuestros). Los botones y la rueda los lee la entrada (0x628CD0...).
int MouseGetWheel();      // (0x628D00)
bool MouseLeftButton();   // (0x628CD0)
bool MouseRightButton();  // (0x628CE0)
bool MouseMiddleButton(); // (0x628CF0)

// total size: 0x2C
class eMouse {
  public:
    void UpdateCursorPos(); // (0x6C9B60)
    int Update();           // (0x6C9C80)

    int Unknown0;           // offset 0x0, size 0x4
    int X;                  // offset 0x4, size 0x4
    int Y;                  // offset 0x8, size 0x4
    int PrevX;              // offset 0xC, size 0x4
    int PrevY;              // offset 0x10, size 0x4
    char Unknown14[8];      // offset 0x14, size 0x8
    int Wheel;              // offset 0x1C, size 0x4
    bool LeftDown;          // offset 0x20, size 0x1
    bool LeftPressed;       // offset 0x21, size 0x1
    bool LeftReleased;      // offset 0x22, size 0x1
    bool RightDown;         // offset 0x23, size 0x1
    bool RightPressed;      // offset 0x24, size 0x1
    bool RightReleased;     // offset 0x25, size 0x1
    bool MiddleDown;        // offset 0x26, size 0x1
    bool MiddlePressed;     // offset 0x27, size 0x1
    bool MiddleReleased;    // offset 0x28, size 0x1
};
extern eMouse TheMouse;     // (0x987F50)

// El raton en cada cuadro: el cursor, la rueda y, por boton, si se acaba de pulsar o de soltar (0x6C9C80).
int eMouse::Update() {
    UpdateCursorPos();
    Wheel = MouseGetWheel();
    bool left = MouseLeftButton();
    LeftPressed = !LeftDown & left;
    LeftReleased = !left & LeftDown;
    LeftDown = left;
    bool right = MouseRightButton();
    RightPressed = !RightDown & right;
    RightReleased = !right & RightDown;
    RightDown = right;
    bool middle = MouseMiddleButton();
    MiddlePressed = !MiddleDown & middle;
    MiddleReleased = !middle & MiddleDown;
    MiddleDown = middle;
    return 0;
}

// Las capturas de pantalla de PC: la imagen de la cadena de intercambio, copiada a un destino que se puede bloquear y
// grabada como TGA (nombres nuestros; entre parentesis, la direccion).
extern IDirect3DSurface9 *eBackBufferSurface; // (0x982A28)
extern int StreamAnimInterval;                // (0x8F9350) cuadros entre dos imagenes de la animacion
extern int StreamAnimFrame;                   // (0x8F9354)
extern int eStreamAnimCountdown;              // (0x982BB0)
extern int eScreenshotNumber;                 // (0x982BB4)
extern int NumberOfStreamAnim;
void eWriteScreenshot(D3DSURFACE_DESC *desc, D3DLOCKED_RECT *locked_rect, const char *filename); // (0x6C0450)
int bFileExists(const char *filename);

// (0x6C05E0)
void eTakeScreenshot(const char *filename) {
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT locked_rect;
    D3DSURFACE_DESC desc;

    if (g_D3DDevice->CreateRenderTarget(__window_width, __window_height, D3DFMT_A8R8G8B8, 0, 0, 1, &surface, 0) == D3D_OK &&
        g_D3DDevice->StretchRect(eBackBufferSurface, 0, surface, 0, 0) == D3D_OK &&
        surface->LockRect(&locked_rect, 0, 16) == D3D_OK) { // D3DLOCK_READONLY
        surface->GetDesc(&desc);
        if (desc.Format >= D3DFMT_A8R8G8B8 && desc.Format <= D3DFMT_X8R8G8B8) {
            eWriteScreenshot(&desc, &locked_rect, filename);
        }
        surface->UnlockRect();
        surface->Release();
    }
}

// La captura pedida y la animacion de capturas (una imagen cada StreamAnimInterval cuadros) (0x6C0910).
void eUpdateScreenshot() {
    char filename[128];

    if (eStreamAnimMode == 2) {
        if (NumberOfStreamAnim < 1) {
            NumberOfStreamAnim = 1;
        }
        if (StreamAnimInterval < 1) {
            StreamAnimInterval = 1;
        }
        if (--eStreamAnimCountdown <= 0) {
            eStreamAnimCountdown = StreamAnimInterval;
            bSPrintf(filename, "StreamAnim%06d.tga", ++StreamAnimFrame);
            eTakeScreenshot(filename);
        }
        if (StreamAnimFrame == NumberOfStreamAnim - 1) {
            SaveStreamAnim = 0;
        }
    } else {
        StreamAnimFrame = -1;
        eStreamAnimCountdown = 0;
    }
    if (eStreamAnimMode == 1) {
        do {
            bSPrintf(filename, "Screenshot%03d.tga", eScreenshotNumber);
            eScreenshotNumber++;
        } while (bFileExists(filename));
        eTakeScreenshot(filename);
        eStreamAnimPending = 0;
    }
}

// El reinicio del dispositivo (nombres nuestros de lo que el exe no nombra; entre parentesis, la direccion).
void eReleaseDeviceTextures();                     // (0x6D2100)
// Las texturas de pantalla completa de los efectos de PC (nombres nuestros; entre parentesis, la direccion).
extern IDirect3DResource9 *eScreenTexturesA[2];    // (0x93DE60)
extern IDirect3DResource9 *eScreenTexturesB[2];    // (0x93DEF8)
extern IDirect3DResource9 *eScreenTexturesC[4];    // (0x982990)
extern IDirect3DResource9 *eScreenTexturesD[4];    // (0x982964)
extern IDirect3DResource9 *eScreenTextureE;        // (0x982960)
extern IDirect3DResource9 *eScreenTextureF;        // (0x93E7BC)
extern IDirect3DResource9 *eScreenTextureG;        // (0x93E7B8)
extern IDirect3DResource9 *eScreenTextureH;        // (0x982C64)
extern IDirect3DResource9 *eScreenTextureI;        // (0x982C68) se suelta con la H, sin mirarla
void eReleaseScreenTexturesPlat();                 // (0x6C3A40) vacia en PC

// Suelta las texturas de pantalla completa (antes de reiniciar el dispositivo y al salir) (0x6C3360).
void eReleaseScreenTextures() {
    for (int i = 0; i < 2; i++) {
        if (eScreenTexturesA[i]) {
            eScreenTexturesA[i]->Release();
        }
        if (eScreenTexturesB[i]) {
            eScreenTexturesB[i]->Release();
        }
        eScreenTexturesA[i] = 0;
        eScreenTexturesB[i] = 0;
    }
    for (int i = 0; i < 4; i++) {
        if (eScreenTexturesC[i]) {
            eScreenTexturesC[i]->Release();
            eScreenTexturesC[i] = 0;
        }
        if (eScreenTexturesD[i]) {
            eScreenTexturesD[i]->Release();
            eScreenTexturesD[i] = 0;
        }
    }
    if (eScreenTextureE) {
        eScreenTextureE->Release();
        eScreenTextureE = 0;
    }
    if (eScreenTextureF) {
        eScreenTextureF->Release();
        eScreenTextureF = 0;
    }
    if (eScreenTextureG) {
        eScreenTextureG->Release();
        eScreenTextureG = 0;
    }
    if (eScreenTextureH) {
        eScreenTextureH->Release();
        eScreenTextureH = 0;
        eScreenTextureI->Release();
        eScreenTextureI = 0;
    }
    eReleaseScreenTexturesPlat();
}
extern IDirect3DSurface9 *eSavedRenderTargetSurface; // (0x982A24)
extern IDirect3DSurface9 *eDepthStencilSurface;      // (0x982A2C)
// Las superficies y texturas de los destinos de render de PC (nombres nuestros; entre parentesis, la direccion).
extern IDirect3DResource9 *eRenderTargetResources[6]; // (0x982A4C)
extern IDirect3DResource9 *eRenderTargetResourceA;    // (0x982A30)
extern IDirect3DResource9 *eRenderTargetResourceB;    // (0x982A78)
extern IDirect3DResource9 *eRenderTargetResourceC;    // (0x982A68)
extern IDirect3DResource9 *eRenderTargetResourceD;    // (0x982A6C)
extern IDirect3DResource9 *eRenderTargetResourceE;    // (0x982A70)
extern IDirect3DResource9 *eRenderTargetResourceF;    // (0x982A74)
extern IDirect3DResource9 *eRenderTargetResourceG;    // (0x982A84)
extern IDirect3DResource9 *eRenderTargetResourceH;    // (0x982A80)
extern IDirect3DResource9 *eRenderTargetResourceI;    // (0x982A7C)
extern IDirect3DResource9 *eRenderTargetResourceJ;    // (0x982CF0)
extern IDirect3DResource9 *eRenderTargetResourceK;    // (0x982CEC)
extern IDirect3DResource9 *eRenderTargetResourceL;    // (0x982CF8)
extern IDirect3DResource9 *eRenderTargetResourceM;    // (0x982CF4)

// Suelta las superficies de los destinos de render (antes de reiniciar el dispositivo y al salir); el destino 0 suelta
// su superficie de profundidad, que es la de la cadena de intercambio (0x6BD620).
void eReleaseRenderTargetSurfaces() {
    eReleaseScreenTexturesPlat();
    for (int i = 0; i < 11; i++) {
        eRenderTarget *render_target = &eRenderTargets[i];
        render_target->ID = (TARGET_ID)i;
        if (i == 0) {
            render_target->D3DDepthSurface->Release();
        }
    }
    for (int i = 0; i < 6; i++) {
        if (eRenderTargetResources[i]) {
            eRenderTargetResources[i]->Release();
            eRenderTargetResources[i] = 0;
        }
    }
    if (eRenderTargetResourceA) {
        eRenderTargetResourceA->Release();
        eRenderTargetResourceA = 0;
    }
    if (eSavedRenderTargetSurface) {
        eSavedRenderTargetSurface->Release();
        eSavedRenderTargetSurface = 0;
    }
    if (eBackBufferSurface) {
        eBackBufferSurface->Release();
        eBackBufferSurface = 0;
    }
    if (eDepthStencilSurface) {
        eDepthStencilSurface->Release();
        eDepthStencilSurface = 0;
    }
    if (eRenderTargetResourceB) {
        eRenderTargetResourceB->Release();
        eRenderTargetResourceB = 0;
    }
    if (eRenderTargetResourceC) {
        eRenderTargetResourceC->Release();
        eRenderTargetResourceC = 0;
    }
    if (eRenderTargetResourceD) {
        eRenderTargetResourceD->Release();
        eRenderTargetResourceD = 0;
    }
    if (eRenderTargetResourceE) {
        eRenderTargetResourceE->Release();
        eRenderTargetResourceE = 0;
    }
    if (eRenderTargetResourceF) {
        eRenderTargetResourceF->Release();
        eRenderTargetResourceF = 0;
    }
    if (eRenderTargetResourceG) {
        eRenderTargetResourceG->Release();
        eRenderTargetResourceG = 0;
    }
    if (eRenderTargetResourceH) {
        eRenderTargetResourceH->Release();
        eRenderTargetResourceH = 0;
    }
    if (eRenderTargetResourceI) {
        eRenderTargetResourceI->Release();
        eRenderTargetResourceI = 0;
    }
    if (eRenderTargetResourceJ) {
        eRenderTargetResourceJ->Release();
        eRenderTargetResourceJ = 0;
    }
    if (eRenderTargetResourceK) {
        eRenderTargetResourceK->Release();
        eRenderTargetResourceK = 0;
    }
    if (eRenderTargetResourceL) {
        eRenderTargetResourceL->Release();
        eRenderTargetResourceL = 0;
    }
    if (eRenderTargetResourceM) {
        eRenderTargetResourceM->Release();
        eRenderTargetResourceM = 0;
    }
}

extern int eLodEnabled;                              // (0x901830)
extern bool eRenderTargetsCreated;                   // (0x982BAC)
void eCreateLodTargets();                            // (0x6C8770)
void eCreateScreenTextures();                        // (0x6C2E60)
void eCreateRenderTargetsA();                        // (0x6BD4B0)
void eCreateRenderTargetsB();                        // (0x6BCDC0)
void eCreateRenderTargetsC();                        // (0x6BCFA0)
void eInitRenderTargets();                           // (0x6BD0D0) los 11 destinos
void eCreateRenderTargetSurfacesPlat();              // (0x6C3A30) vacia en PC

// Las superficies de la cadena de intercambio y todos los destinos de render (al arrancar y tras reiniciar el
// dispositivo) (0x6BD7C0).
void eCreateRenderTargetSurfaces() {
    eSavedRenderTargetSurface = 0;
    g_D3DDevice->GetBackBuffer(0, 0, 0, &eBackBufferSurface); // D3DBACKBUFFER_TYPE_MONO
    g_D3DDevice->GetDepthStencilSurface(&eDepthStencilSurface);
    if (eLodEnabled > 0) {
        eCreateLodTargets();
    }
    eCreateScreenTextures();
    eCreateRenderTargetsA();
    eCreateRenderTargetsB();
    eCreateRenderTargetsC();
    eInitRenderTargets();
    eCreateRenderTargetSurfacesPlat();
    eRenderTargetsCreated = true;
}
void eInitFEEnvMapPlat();
extern IDirect3DResource9 *eDeviceResources[4];    // (0x93E03C) cuatro recursos de video sueltos
extern D3DPRESENT_PARAMETERS g_D3DPresentParameters; // (0x93E82C)
// Las consultas de oclusion de D3D (0x93E018; nombres nuestros).
// total size: 0x34
struct eOcclusionQueries {
    void Init(); // (0x6C00D0)

    int Unknown0;                  // offset 0x0, size 0x4
    int Unknown4;                  // offset 0x4, size 0x4
    float Unknown8;                // offset 0x8, size 0x4
    float UnknownC;                // offset 0xC, size 0x4
    bool Unknown10;                // offset 0x10, size 0x1
    int Unknown14;                 // offset 0x14, size 0x4
    float Unknown18;               // offset 0x18, size 0x4
    int Unknown1C;                 // offset 0x1C, size 0x4
    int Unknown20;                 // offset 0x20, size 0x4
    IDirect3DQuery9 *Queries[4];   // offset 0x24, size 0x10
};
extern eOcclusionQueries eOcclusion;               // (0x93E018)

// Los valores de partida y las cuatro consultas de oclusion (las que no se pueden crear quedan a cero) (0x6C00D0).
void eOcclusionQueries::Init() {
    Unknown0 = 0;
    Unknown4 = 1;
    Unknown8 = 0.5f;
    UnknownC = 0.9999f;
    Unknown10 = false;
    Unknown14 = 0;
    Unknown18 = 1.0f;
    Unknown1C = 0;
    Unknown20 = 0;
    for (int i = 0; i < 4; i++) {
        if (g_D3DDevice->CreateQuery(9, &Queries[i]) != D3D_OK) { // D3DQUERYTYPE_OCCLUSION
            Queries[i] = 0;
        }
    }
}

// Reinicia el dispositivo de D3D (al perderlo o al cambiar de modo): suelta todo lo que esta en video, Reset, y si
// falla prueba tres veces y luego resoluciones mas bajas; despues lo vuelve a crear todo (0x6DB0D0).
int eResetDevice() {
    eReleaseDeviceTextures();
    eLoseEffects();
    eReleaseRenderTargetSurfaces();
    for (int i = 0; i < 4; i++) {
        if (eDeviceResources[i]) {
            eDeviceResources[i]->Release();
            eDeviceResources[i] = 0;
        }
    }
    eReleaseScreenTextures();
    eParticleVertexBuffer.Unlock();
    eParticleVertexBuffer.Flush();
    if (eParticleVertexBuffer.VertexBuffer) {
        eParticleVertexBuffer.VertexBuffer->Release();
    }

    g_D3DDevice->Reset(&g_D3DPresentParameters);
    HRESULT result = g_D3DDevice->Reset(&g_D3DPresentParameters);
    if (result != D3D_OK) {
        int width;
        int height;

        if (__multisamplequality > 0) {
            __multisamplequality = 0;
        }
        eResolutions.GetResolution(&width, &height);
        InitWindow(width, height, eRefreshRates[eResolutionIndex]);
        for (int tries = 0; tries < 3; tries++) {
            result = g_D3DDevice->Reset(&g_D3DPresentParameters);
            if (result == D3D_OK) {
                break;
            }
        }
        if (result != D3D_OK) {
            for (int index = eResolutionIndex; index != 0; index = eResolutionIndex) {
                eResolutionIndex = --index;
                eResolutions.GetResolution(&width, &height);
                InitWindow(width, height, eRefreshRates[index]);
                result = g_D3DDevice->Reset(&g_D3DPresentParameters);
                if (result == D3D_OK) {
                    break;
                }
            }
            if (result != D3D_OK) {
                return result;
            }
        }
    }

    eOcclusion.Init();
    eCreateRenderTargetSurfaces();
    eInitFEEnvMapPlat();
    eParticleVertexBuffer.Init(0, 0x400);
    eSetDefaultRenderStates();
    eResetEffects();
    return result;
}

void eDisplayFrame() {
    eCurrentEffect = 0;
    if (eCreateTimeOfDayPending) {
        eCreateTimeOfDay();
        eCreateTimeOfDayPending = false;
    }
    if (eReloadEffectsPending) {
        eReleaseEffects();
        eLoadEffects();
        if (pVisualTreatmentPlat) {
            pVisualTreatmentPlat->GetEffectParameters();
        }
        eReloadEffectsPending = 0;
    }
    if (eDisplaySettingsChanged) {
        int width;
        int height;
        eReleaseEffects();
        eLoadEffects();
        if (pVisualTreatmentPlat) {
            pVisualTreatmentPlat->GetEffectParameters();
        }
        eResolutions.GetResolution(&width, &height);
        InitWindow(width, height, eRefreshRates[eResolutionIndex]);
        eResetDevice();
        eDisplaySettingsChanged = false;
    }

    eStreamAnimMode = 0;
    if (SaveStreamAnim) {
        eStreamAnimMode = 2;
        eStreamAnimPending = 0;
    } else if (eStreamAnimPending) {
        eStreamAnimMode = 1;
    }

    MaybeChangeViewMode();

    switch (eShadowDetail) {
        case 0:
            eShadowsEnabled = 1;
            eShadowsEnabled2 = 1;
            eShadowMapQuality = 2;
            break;
        case 1:
            eShadowsEnabled = 1;
            eShadowsEnabled2 = 1;
            eShadowMapQuality = 1;
            break;
        case 2:
            eShadowsEnabled = 1;
            eShadowsEnabled2 = 1;
            eShadowMapQuality = 0;
            break;
        case 3:
            eShadowsEnabled = 0;
            eShadowsEnabled2 = 0;
            eShadowMapQuality = 0;
            break;
    }

    if (EnableEnvMap) {
        UpdateEnvironmentMapCameras();
    }
    elBeginFrame();

    for (int view_id = EVIEW_FIRST; view_id <= EVIEW_LAST; view_id++) {
        eView *view = &eViews[view_id];
        eViewPlatInfo *plat_info = view->GetPlatInfo();
        if (view_id == EVIEW_PLAYER1_SPECULAR) {
            eSetupView8(view, 0);
        }
        if (view->Active || view_id >= EVIEW_FIRST_ENVMAP) {
            if (view->ID == EVIEW_PLAYER1_RVM) {
                plat_info->CalculateViewMatricies(view, RVMNearZ, 15000.0f, 0.0f, true);
            } else {
                plat_info->CalculateViewMatricies(view, 0.0f, 15000.0f, 0.0f, true);
            }
        }
    }
    eSetDefaultRenderStates();

    // La profundidad de campo de la camara del jugador 1, en el efecto del mundo.
    if (eDepthOfFieldEnabled) {
        Camera *camera = eViews[EVIEW_PLAYER1].pCamera;
        float focal_distance = camera->GetFocalDistance();
        float depth_of_field = camera->GetDepthOfField();
        if (focal_distance != 0.0f || depth_of_field != 0.0f) {
            if (focal_distance < 0.0f) {
                focal_distance = 0.0f;
            }
            if (depth_of_field < 0.0f) {
                depth_of_field = 8.0f;
            }
            eDepthOfFieldActive = true;
            eEffects[0]->pD3DXEffect->SetFloat(eEffects[0]->Parameters[36].Handle, 0.0009765625f);
            float range = depth_of_field * 0.00048828125f;
            float focus = focal_distance * 0.0009765625f;
            float dof_scale = depth_of_field * 0.01953125;
            float scale = 1.0f / dof_scale;
            eEffects[0]->pD3DXEffect->SetFloat(eEffects[0]->Parameters[18].Handle, scale);
            eEffects[0]->pD3DXEffect->SetFloat(eEffects[0]->Parameters[19].Handle, -((focus + range) * scale));
            eEffects[0]->pD3DXEffect->SetFloat(eEffects[0]->Parameters[20].Handle, (focus - range) * scale);
        } else {
            eDepthOfFieldActive = false;
        }
    } else {
        eDepthOfFieldActive = false;
    }
    if (eDepthOfFieldActive) {
        eDepthOfFieldActive2 = true;
    } else {
        eDepthOfFieldActive2 = false;
    }

    HRESULT result = g_D3DDevice->TestCooperativeLevel();
    if (result == D3DERR_DEVICENOTRESET) {
        result = eResetDevice();
    }
    if (result == D3D_OK) {
        g_D3DDevice->BeginScene();
        eRenderPass = 2;
        eRenderFrame(1);
        if (!GiveTheMoviePlayerBandwidth()) {
            for (int view_id = EVIEW_FIRST_PLAYER; view_id <= EVIEW_LAST_PLAYER; view_id++) {
                eView *view = eGetView(view_id, false);
                if (view->Active) {
                    view->SetupWorldLightContext();
                }
            }
            if (!ExitTheGameFlag) {
                eSetDefaultRenderStates();
            }
            ___eDisplayFrame();
            eRenderFrameEnd();
        }
        eRenderFrame(0);
        g_D3DDevice->EndScene();
    }

    eGetAverageBatchSize();
    if (eScreenshotPending) {
        eTakeScreenshot(0);
        eScreenshotPending = 0;
    }
    eUpdateScreenshot();
    if (g_D3DDevice->Present(0, 0, 0, 0) == D3DERR_DEVICELOST &&
        g_D3DDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
        eResetDevice();
    }
    if (eMovieFramePending) {
        eMovieFrameDone();
        eMovieFramePending = 0;
    }
    eDisplayFrameDone();
    ePostDisplay();
    g_D3DDevice->Clear(0, 0, 7, 0, 1.0f, 0); // TARGET | ZBUFFER | STENCIL
    eSwapFrameMallocBuffers();
    eSwapDynamicFrameMemory();
    eLastRenderedTexture = 0;
    eLastRenderedTexture2 = 0;
    eFrameCounter++;
    TheMouse.Update();
}
