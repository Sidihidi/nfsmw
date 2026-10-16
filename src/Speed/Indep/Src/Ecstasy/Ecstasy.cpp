#include "Ecstasy.hpp"
#include "EcstasyE.hpp"
#include "Speed/Indep/Src/Animation/AnimEngineManager.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FEngFont.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Misc/Profiler.hpp"
#include "Speed/Indep/Src/World/Scenery.hpp"
#include "Speed/Indep/Src/World/ScreenEffects.hpp"
#include "Speed/Indep/Src/World/SimpleModelAnim.hpp"
#include "Speed/Indep/Src/World/Sun.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Texture.hpp"
#include "eEnvMapE.hpp"
#include "eLight.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/World/WDbgCollisionDraw.h"
#endif

SlotPool *ePolySlotPool = 0;
bool WaitUntilRenderingDoneDisabled = false;
bool WaitForFrameBufferSwapDisabled = false;

// EAXSound2
float renderModifier = 1.0f;
#ifdef EA_BUILD_A124
uint32 numCopsActive;
#else
extern TextureInfo *SunTextures[5];
// y los tres van al REVES de como estaban escritos (View, Total, Cherry)
uint32 numCopsActiveView;
uint32 numCopsActiveTotal;
uint32 numCopsActiveCherry;
#endif
uint8 *FrameMemoryBuffer[2];
uint32 FrameMemoryBufferSize = 0;
uint8 *CurrentBufferStart = 0;
uint8 *CurrentBufferPos = 0;
uint8 *CurrentBufferEnd = 0;

int FrameMemoryBufferAmountUsed[2];
//
uint32 FrameMallocAllocNum = 0;
uint32 FrameMallocFailed = 0;
uint32 FrameMallocFailAmount = 0;
//
uint32 FrameMallocMaxFailAmount = 0;

int Eframelargest = 0;
int Eframecurrent = 0;

TextureInfo *OtherEcstacyTextures[30];
unsigned int OtherEcstacyTextures_name_hash[30];

uint32 numOtherTex = 0;

// Los contadores de poligonos por tipo de dibujo (la tabla de nombres, del .data de la A124).
unsigned int PolyCounts[NUM_POLY_COUNT_TYPES];
unsigned int ModelCounts[NUM_POLY_COUNT_TYPES];
char PolyCountNames[NUM_POLY_COUNT_TYPES][32] = {
    "MAIN_CAR",     "REFL_CAR",     "RVMI_CAR",     "ENVM_SCENERY", "MAIN_SCENERY",
    "REFL_SCENERY", "RVMI_SCENERY", "MAIN_WRLDMDL", "REFL_WRLDMDL", "RVMI_WRLDMDL",
};

void eAllocateFrameMallocBuffers(uint32 total_size /* r3 */);

int eInitEngine() {
    eAllocateFrameMallocBuffers(0xE1000);
    ePolySlotPool = bNewSlotPool(0x94, 0x20, "ePolySlotPool", 0);
    ePolySlotPool->ClearFlag(SLOTPOOL_FLAG_ZERO_ALLOCATED_MEMORY);
    eInitModels();
    eInitTextures();
    eInitSolids();
    epInitViews();
    elInit();
    eInitEnvMap();
    InitNFSAnimEngine();
    SimpleModelAnim::Init();
    return eInitEnginePlat();
}

// STRIPPED
int eCloseEngine() {}

void ePreDisplay() {
    TickSFX();
    RenderSunAsFlare();
    SimpleModelAnim::Update();
    DoTunnelBloom(eGetView(1, false));
    DoTunnelBloom(eGetView(2, false));
#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32)
// La A124 tintea las dos vistas sin mirar la lluvia; speed.exe tambien (dos DoTinting incondicionales).
// Las demas versiones solo la primera, y unicamente si no llueve.
    DoTinting(eGetView(1, false));
    DoTinting(eGetView(2, false));
#else
    if (!IsRainDisabled()) {
        DoTinting(eGetView(1, false));
    }
#endif
    UpdateLightFlareParameters();
#ifdef EA_BUILD_A124
    numCopsActive = eGetView(1, false)->NumCopsInView + eGetView(2, false)->NumCopsInView;
    eGetView(1, false)->NumCopsInView = 0;
    eGetView(2, false)->NumCopsInView = 0;

#else
    numCopsActiveView = eGetView(1, false)->NumCopsInView;
    numCopsActiveTotal = eGetView(1, false)->NumCopsTotal;
    numCopsActiveCherry = eGetView(1, false)->NumCopsCherry;
    eGetView(1, false)->NumCopsInView = 0;
    eGetView(1, false)->NumCopsTotal = 0;
    eGetView(1, false)->NumCopsCherry = 0;
    renderModifier = 1.0f - numCopsActiveCherry / 5.0f;
    if (renderModifier < 0.25f) {
        renderModifier = 0.25f;
    }
#endif
}

void ePostDisplay() {
    ProfileNode profile_node;
    UpdateTextureAnimations();
    UpdateAllScreenEFX();
    eResestLightFlarePool();
    if (QueryFlushAccumulationBuffer()) {
        AccumulationBufferFlushed();
    }
}

void eNotifyTextureLoading(TexturePack *texture_pack, TextureInfo *texture_info, bool loading) {
    eSolidNotifyTextureLoading(texture_pack, texture_info, loading);
    FEngFontNotifyTextureLoading(texture_pack, loading);
}

void eFixUpTables() {
    if (eDisableFixUpTables == 0) {
        if (eDirtySolids != 0 || eDirtyTextures != 0) {
            eFixUpTablesPlat();
            eDirtySolids = 0;
            eDirtyTextures = 0;
            eDirtyAnimations = 0;
        }
    }
}

void eAllocateFrameMallocBuffers(uint32 total_size) {
    uint32 buffer_size = (total_size >> 1) & 0x7FFFFFF0;

    FrameMemoryBuffer[0] = new ("Ecstacy::eFrameMallocBuffer0", 0) uint8[buffer_size];
    FrameMemoryBuffer[1] = new ("Ecstacy::eFrameMallocBuffer1", 0) uint8[buffer_size];

    FrameMemoryBufferSize = buffer_size;
    CurrentBufferPos = FrameMemoryBuffer[0];
    CurrentBufferEnd = FrameMemoryBuffer[0] + buffer_size;
    FrameMallocAllocNum = 0;
}

// STRIPPED
void eFreeFrameMallocBuffers() {}

void eSwapFrameMallocBuffers() {
    uint8 *buffer0;
    uint8 *buffer1;  // r3
    int amount_used; // r4

    if (FrameMallocFailed != 0 && FrameMallocFailAmount > FrameMallocMaxFailAmount) {
        FrameMallocMaxFailAmount = FrameMallocFailAmount;
    }

    buffer0 = FrameMemoryBuffer[0];
    buffer1 = FrameMemoryBuffer[1];

    FrameMallocFailed = 0;
    FrameMallocFailAmount = 0;

    amount_used = CurrentBufferPos - buffer0;
    if (amount_used > Eframelargest) {
        Eframelargest = amount_used;
    }

    Eframecurrent = amount_used;
    FrameMemoryBufferAmountUsed[1] = FrameMemoryBufferAmountUsed[0];
    FrameMemoryBufferAmountUsed[0] = amount_used;
#ifdef EA_PLATFORM_WIN32
    // w12c6: el original guarda [0] antes que [1]
    FrameMemoryBuffer[0] = buffer1;
    FrameMemoryBuffer[1] = buffer0;
#else
    FrameMemoryBuffer[1] = buffer0;
    FrameMemoryBuffer[0] = buffer1;
#endif
    CurrentBufferStart = buffer1;
    CurrentBufferPos = buffer1;
    CurrentBufferEnd = FrameMemoryBufferSize + buffer1;
    FrameMallocAllocNum = 0;
}

// STRIPPED
int eGetSizeofFrameMallocBuffers(int *pamount_used /* r3 */) {}

void SetupSceneryCullInfo(eView *view /* r30 */, SceneryCullInfo &info /* r29 */, int exclude_flags /* r31 */) {
    EVIEWMODE view_mode; // r3
    int32 view_id;       // r11

    view_mode = eGetCurrentViewMode();
    exclude_flags |= 0x10;
    view_id = view->GetID();
    if (view_id == 1 || view_id == 2)
        exclude_flags |= 2;
    if (view_mode == 4 || view_mode == 3)
        exclude_flags |= 1;
    // PS2: el original compara con view_id - 20 (rango [20,25]) y no tiene el test de 0x1000 de abajo:
    // en SLUS es addiu -20 + sltu sin andi/movn extra (88,19 % con la forma de GC, 100 % alli: no tocar).
#ifdef EA_PLATFORM_PLAYSTATION2
    if (view_id >= 20 && view_id <= 25)
#elif defined(EA_PLATFORM_WIN32) // speed.exe: vistas 18..23 (cmp 0x12 / cmp 0x17)
    if (view_id >= 18 && view_id <= 23)
#else
    if (view_id >= 16 && view_id <= 21)
#endif
        exclude_flags |= 0x100;

    if (GRaceStatus::Exists() && GRaceStatus::Get().GetPlayMode() == 1)
        exclude_flags |= 4;
    if (exclude_flags & 0x1800)
#ifdef EA_PLATFORM_WIN32 // speed.exe anade tambien 0x8000000 (or 0x8000040)
        exclude_flags |= 0x8000040;
#else
        exclude_flags |= 0x40;
#endif
#ifndef EA_PLATFORM_PLAYSTATION2
    if (exclude_flags & 0x1000)
        exclude_flags |= 2;
#endif
    if (view_id == 3)
        exclude_flags |= 0x20;

    SetupSceneryCullInfoPlat(view, info);
    info.ExcludeFlags = exclude_flags;
}

void eSwapDynamicFrameMemory() {}

// STRIPPED
void eTagHeadlightCallback(SceneryDrawInfo *info) {
    info->SceneryInst->Rotation[1] = 512;
}

// STRIPPED
void RenderMWDebugTopology(eView *view /* r3 */) {
#ifdef EA_BUILD_A124
    // La A124 empieza aqui el cuadro del dibujo de colisiones de depuracion.
    WDbgCollisionDraw::Get().StartFrame();
#endif
};

// STRIPPED
void eDebugRender(eView *view /* r4 */) {}

// STRIPPED
void eAddOtherEcstacyTexture(uint32 name_hash /* r3 */, TextureInfo *tex /* r4 */) {};

struct TextureInfo *eGetOtherEcstacyTexture(uint32 name_hash) {
    if (numOtherTex < 0)
        return NULL;

    for (uint32 i = 0; i < numOtherTex; i++) {
        if (OtherEcstacyTextures_name_hash[i] == name_hash) {
            return OtherEcstacyTextures[i];
        }
    }

    return nullptr;
}

// STRIPPED
void eRemoveOtherEcstacyTexture(uint32 name_hash /* r3 */) {}

bool eIsWidescreen() {
    return false;
} // Unlocks fullscreen on GameCube for free?

// STRIPPED
void AddPolyCount(polyCountEnum type /* r3 */, struct eModel *model /* r4 */) {}
void ClearPolyCounts() {
    for (int i = 0; i < NUM_POLY_COUNT_TYPES; i++) {
        ModelCounts[i] = 0;
        PolyCounts[i] = 0;
    }
}

int GetPolyCount(polyCountEnum type /* r3 */) {
    return PolyCounts[type];
}

int GetModelCount(polyCountEnum type /* r3 */) {
    return ModelCounts[type];
}

char *GetPCType(polyCountEnum type /* r3 */) {
    return PolyCountNames[type];
}
