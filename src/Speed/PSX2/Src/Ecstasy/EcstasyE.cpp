#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyData.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Indep/Src/Ecstasy/eStrip.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/Ecstasy/eLight.hpp"
#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Debug/Common/DebugDraw.h"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/HUD/FEPkg_Hud.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/FEPkg_GarageMain.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#include "Speed/Indep/Src/Frontend/MoviePlayer/MoviePlayer.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Misc/Platform.h"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/World/CarRender.hpp"
#include "Speed/Indep/Src/World/Clans.hpp"
#include "Speed/Indep/Src/World/FacePixelate.hpp"
#include "Speed/Indep/Src/World/Rain.hpp"
#include "Speed/Indep/Src/World/Scenery.hpp"
#include "Speed/Indep/Src/World/ScreenEffects.hpp"
#include "Speed/Indep/Src/World/SimpleModelAnim.hpp"
#include "Speed/Indep/Src/World/SkyRender.hpp"
#include "Speed/Indep/Src/World/Sun.hpp"
#include "Speed/Indep/Src/World/TimeOfDay.hpp"
#include "Speed/Indep/Src/World/VisualTreatment.h"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/PSX2/Inc/DmaE.hpp"
#include "Speed/PSX2/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/PSX2/Src/Ecstasy/XSprites.hpp"
#include "Speed/PSX2/Src/Ecstasy/eStripData.hpp"
#include "Speed/PSX2/Src/Ecstasy/eViewPlat.hpp"
#include "Speed/PSX2/Src/Ecstasy/eVisualTreatment.hpp"
#include "Speed/PSX2/bWare/Src/ee/include/eekernel.h"
#include "Speed/PSX2/bWare/Src/ee/include/eestruct.h"
#include "Speed/PSX2/bWare/Src/ee/include/libdma.h"
#include "Speed/PSX2/bWare/Src/ee/include/libdev.h"
#include "Speed/PSX2/bWare/Src/ee/include/libgraph.h"

extern SlotPool *eAnimTextureSlotPool;
extern unsigned int vu1MicroCodeStart[];














































































bool MovieIsPlaying() {
    bool MovieIsPlaying = false;

    if (gMoviePlayer) {
        MovieIsPlaying = gMoviePlayer->GetStatus() == 5;
    }
    return MovieIsPlaying;
}


// Los globales van en el orden de sus stabs (el mismo que el de .data en el objeto de la A124) y con sus valores.
IVisualTreatmentPlat *pVisualTreatmentPlat;
int Path3TextureTransferEnabled = 1;
int VUmemEnd = 1012;
int debuggy = 634;
u_long128 *currTex;
u_long128 *currDMA;
int oldOFFSET_REFLECT_DOT_NORM;
int STTSslop[4] = {0, 12, 12, 20};
int XYZslop[8] = {0, 12, 16, 24};
int StripLit = 1;
eTransferRectQueue SunTRECT;
eRenderStageQueue RenderStageQueue0;
eRenderStageQueue RenderStageQueue1;
eRenderStageQueue *ActiveRenderStageQueue;
eRenderStageQueue *IdleRenderStageQueue;
int LockTo30FPS;
bFix FloatingVBlankTimeMin = 0xD0000;
bFix FloatingVBlankTimeMax = 0x140000;
uint32 eFrameCounter;
uint32 PreviousVBLCount;
volatile int eVBlankCounter; // los cuatro los tocan las interrupciones (VBlank y VIF1): volatiles
volatile int RenderQueueExecuting;
volatile int WaitingForBufferSwap;
volatile int CurrentHalfOffset;
int CurrentRenderStage = -1;
sceDmaChan *dmaGif;
sceDmaChan *dmaVif0;
sceDmaChan *dmaVif1;
sceDmaChan *dmaToSPR;
sceDmaChan *dmaFromSPR;
VIDEO_MODE eCurrentVideoMode = static_cast<VIDEO_MODE>(-1);
int PreviousVIFHandlerID = -1;
SlotPool *ActiveTextureSlotPool;
bTList<eActiveTexture> SolidTextureList;
bTList<eActiveTexture> AlphaTextureList;
TextureInfo *pTextureInfoRainDropMask;
TextureInfo *pTextureInfoRVMInfo;
TextureInfo *pTextureInfoRVMMask;
TextureInfo *pTextureInfoSolidGrey;
TextureInfo *pTextureInfoFilterGrey;
TextureInfo *pTextureInfoShadowMap1;
TextureInfo *pTextureInfoRadialBlur;
TextureInfo *pTextureInfoSpecularFalloff;
TextureInfo *pTextureInfoSpecularSpot;
TextureInfo *pTextureInfoHeadlights;
TextureInfo *pTextureInfoCarSelectEnvMap;
TextureInfo *pTextureInfoSmallEnvMap;
TextureInfo *pTextureInfoCarElectric;
TextureInfo *pTextureInfoTEST0;
TextureInfo *pTextureInfoTEST1;
TextureInfo *pTextureInfoGlossyWindow;
int ScreenDisplayOffsetsXY[3][2] = {{56, 27}};
int ScreenWidth = 512;
int ScreenHeight = 448;
short ScreenDepth;
BufferParams BufferParameters[5][2] = {
    {{0, 0, 0, 0, 0, 652, 50, 0, 0, 4, 0, 2560, 0, 0, 652, 50, 0, 1, 4, 0, 2560, 0, 128},
     {0, 0, 0, 0, 0, 652, 50, 0, 0, 4, 0, 2560, 0, 0, 652, 50, 0, 1, 4, 0, 2560, 0, 128}},
    {{0, 0, 8, 8, 1, 652, 50, 0, 0, 4, 0, 2560, 448, 1, 652, 50, 0, 1, 4, 0, 2560, 448, 128},
     {0, 0, 8, 8, 1, 652, 50, 0, 0, 4, 0, 2560, 448, 1, 652, 50, 0, 1, 4, 0, 2560, 448, 128}},
    {{0, 0, 8, 8, 1, 652, 50, 0, 0, 4, 0, 2560, 448, 1, 652, 50, 0, 1, 4, 0, 2560, 448, 128},
     {0, 0, 8, 8, 1, 652, 50, 0, 0, 4, 0, 2560, 448, 1, 652, 50, 0, 1, 4, 0, 2560, 448, 128}},
};
BufferParams *CurrentBufferParameters;
float NTSCAspectRatio3dx = 1.0f;
float NTSCAspectRatio3dxWide = 0.71f;
float NTSCAspectRatio3dy = 1.1f;
float NTSCAspectRatio2dx = 0.8f;
float NTSCAspectRatio2dxWide = 0.6f;
float NTSCAspectRatio2dy = 0.93333334f;
float PALAspectRatio3dx = 1.0f;
float PALAspectRatio3dxWide = 0.71f;
float PALAspectRatio3dy = 1.365f;
float PALAspectRatio2dx = 0.8f;
float PALAspectRatio2dy = 1.0666667f;
float PALAspectRatio2dxWide = 0.6f;
int EnvironmentMappingEnable = 1;
int MinimalEnvironmentMappingEnable;
// El ALPHA del contexto 2 que carga el microcodigo (AttachMicroCode), para tocarlo desde el depurador.
int32 egAlphaA;
int32 egAlphaB = 2;
int32 egAlphaC = 1;
int32 egAlphaD = 1;
int32 egAlphaF = 16;
int EnableEnvMap = 1;
int EnableTexturing = 1;
int DrawWireframe;
int DrawWorld = 1;
int DumpDMAChains;
float TexAnimOriginalTextureWidth;
float TexAnimOriginalTextureHeight;
float TexAnimScrollSpeedS;
float TexAnimScrollSpeedT;
TextureScrollType TexAnimScrollType = TEXSCROLL_SMOOTH;
float TexAnimScrollTimeStep = 1.0f;
float DrawNormalsScale = 1.0f;
// Los parametros de la raiz cuadrada inversa rapida del microcodigo (AttachMicroCode).
float fpRSQRT_Pow = -0.5f;
float fpRSQRT_Max = 0.95f;
float fpRSQRT_PowScale = 1.0f;
float fpRSQRT_MaxScale = 0.9405f;
float BackgroundNearZ = 0.7f;
float GetFaceFarZ[6] = {64.0f, 64.0f, 0.0f, 64.0f, 64.0f, 64.0f};
eRenderTargetMatrixPack DefaultMatrixPack;
float AvgCpuTime;
float AvgGpuTime;
Camera FlailerCamera;
Camera Player1Camera;
Camera Player2Camera;
Camera Player1RVMCamera;
Camera testCam;
TextureInfo RenderTargetTextureInfos[24][2];
float ClipGBVolumeScaleXNORM = 0.5f;
float ClipGBVolumeScaleXNWIDE = 0.71f;
float ClipGBVolumeScaleXDisp = 0.5f;
float ClipGBVolumeScaleX = 0.5f;
float ClipGBVolumeScaleY = 0.909f;
float CenterX = 2048.0f;
float CenterY = 2048.0f;
uint32 vurandom;
int CurrentZWriteState = 1;
unsigned int debuggy0;
inline void eProfileTime::Start() { Time = bGetTicker(); }
inline void eProfileTime::End() { Time = bGetFixTickerDifference(Time, bGetTicker()); TotalTime += Time; }
unsigned int debuggy1;
unsigned int debuggy2;
int UpdateMicroCode;
int glowstage0;
int cleara;
int clearb;
int clearc;
int cleard;
int clearf;
eRenderTarget RenderTargets[24];
eFrameTiming FrameTimingStage0;
eFrameTiming FrameTimingStage1;
eFrameTiming FrameTimingStage2;
int WaitBufferSwapTime;
int VifTime;
bFix LastVBlankTime;
float PrevCpuTime[16];
float PrevGpuTime[16];
uint32 *es_data_start;
uint32 *es_data_bottom;
uint32 *es_data_end;
uint32 *es_data_vertex;
uint32 *es_data_vertex_top;
unsigned int *es_data_colour;
uint32 *es_data_uv;
int es_uv_flip;
int es_vertex_count;
int es_qwsize;
TextureInfo *es_texture_info;
bMatrix4 *es_local_to_world;
void *GiganticPSX2HackDMAPtr;
void *GiganticPSX2HackTEXPtr;
u_long128 SentinelDmaTags[5][2];






















































































// Apunta un XYOFFSET de un paquete para que el fin de etapa lo parchee con el medio pixel del fotograma.
inline void eRenderStageQueue::AddPatchXYOffset(u_long *xyoffset) {
    ePatchOffset *patch_offset = static_cast<ePatchOffset *>(eFrameMalloc(sizeof(ePatchOffset)));

    if (patch_offset) {
        PatchXYOffsetList.AddTail(patch_offset);
        patch_offset->OriginalValue = *xyoffset;
        patch_offset->PatchOffset = xyoffset;
    }
}





























































































































































































































































































































































































































































































































































int eInitEnginePlat() {
    eDmaInit();
    eInitTextureDownloader();
    ActiveTextureSlotPool = bNewSlotPool(sizeof(eActiveTexture), 400, "ActiveTextureSlotPool", 0);
    ActiveTextureSlotPool->ClearFlag(SLOTPOOL_FLAG_ZERO_ALLOCATED_MEMORY);
    SetScreenBuffers();
    dmaGif = sceDmaGetChan(SCE_DMA_GIF);
    dmaVif0 = sceDmaGetChan(SCE_DMA_VIF0);
    dmaVif1 = sceDmaGetChan(SCE_DMA_VIF1);
    dmaToSPR = sceDmaGetChan(SCE_DMA_toSPR);
    dmaFromSPR = sceDmaGetChan(SCE_DMA_fromSPR);
    dmaGif->chcr.TTE = 0;
    dmaVif0->chcr.TTE = 1;
    dmaVif1->chcr.TTE = 1;
    dmaToSPR->chcr.TTE = 0;
    dmaFromSPR->chcr.TTE = 0;
    *GIF_MODE = 4;
    DownloadVUCode();
    TestSort();
    DmaTest();
    VRAMView();
    bMemSet(&DefaultMatrixPack, 0, sizeof(DefaultMatrixPack));
    eAnimTextureSlotPool = bNewSlotPool(8, 256, "eAnimTextureSlotPool", 0);
    eAnimTextureSlotPool->ClearFlag(SLOTPOOL_FLAG_ZERO_ALLOCATED_MEMORY);
    return 1;
}





























































void RemoveVIFHandler() {
    if (PreviousVIFHandlerID != -1) {
        RemoveIntcHandler(INTC_VIF1, PreviousVIFHandlerID);
        PreviousVIFHandlerID = -1;
    }
}


int eSetDisplaySystem(int vide_mode) {
    if (eCurrentVideoMode == vide_mode) {
        return 1;
    }
    eCurrentVideoMode = static_cast<VIDEO_MODE>(vide_mode);
    eWaitUntilRenderingDone();
    sceGsResetPath();
    sceDmaReset(1);
    sceDevVu0Reset();
    if (vide_mode == MODE_NTSC) {
        ScreenWidth = 512;
        ScreenHeight = 448;
        sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FIELD);
    } else {
        ScreenWidth = 512;
        ScreenHeight = 512;
        sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_PAL, SCE_GS_FIELD);
    }
    RegisterPSX2VBlankHandler();
    DisableIntc(INTC_VIF1);
    RemoveVIFHandler();
    PreviousVIFHandlerID = AddIntcHandler(INTC_VIF1, Vif1InteruptHandler, 0);
    if (PreviousVIFHandlerID < 0) {
        return 0;
    }
    EnableIntc(INTC_VIF1);
    SetScreenBuffers();
    *GIF_MODE = 4;
    return 1;
}

























































void epInitViews() {
    {
        eView *view = eGetView(1, false);
        view->SetRenderTarget(eGetRenderTarget(1), 0);
        view->SetCamera(&Player1Camera);
        view->ScreenEffects = new ("ScreenEffectDB", 0) ScreenEffectDB();
        view->ScreenEffects->SetMyView(view);
        view->Precipitation = new ("Rain", 0) Rain(view, RAIN);
#ifdef EA_BUILD_A124
        view->facePixelation = new (__FILE__, __LINE__) FacePixelation(view);
#else
        view->facePixelation = new FacePixelation(view);
#endif
    }
    {
        eView *view = eGetView(2, false);
        view->SetRenderTarget(eGetRenderTarget(2), 0);
        view->SetCamera(&Player2Camera);
        view->ScreenEffects = new ("ScreenEffectDB", 0) ScreenEffectDB();
        view->ScreenEffects->SetMyView(view);
        view->Precipitation = new ("Rain", 0) Rain(view, RAIN);
    }
    {
        eView *view = eGetView(0, false);
        view->SetRenderTarget(eGetRenderTarget(0), 0);
        view->SetCamera(&FlailerCamera);
        view->ScreenEffects = eGetView(1, false)->ScreenEffects;
        view->SetActive(1);
    }
    {
        eView *view = eGetView(3, false);
        view->SetRenderTarget(eGetRenderTarget(3), 0);
        view->SetCamera(&Player1RVMCamera);
#ifndef EA_BUILD_A124
        view->Precipitation = eGetView(1, false)->Precipitation;
#endif
        view->SetActive(0);
    }
    {
        eView *view = eGetView(4, false);
        view->SetRenderTarget(eGetRenderTarget(4), 0);
        view->Precipitation = eGetView(1, false)->Precipitation;
        view->SetCamera(&Player1Camera);
        view->SetActive(1);
    }
    {
        eView *view = eGetView(5, false);
        view->SetRenderTarget(eGetRenderTarget(5), 0);
        view->Precipitation = eGetView(2, false)->Precipitation;
        view->SetCamera(&Player2Camera);
        view->SetActive(0);
    }
    {
        eView *view = eGetView(8, false);
        view->SetRenderTarget(eGetRenderTarget(6), 0);
        view->Precipitation = eGetView(1, false)->Precipitation;
        view->SetCamera(&Player1Camera);
        view->SetActive(1);
    }
    {
        eView *view = eGetView(9, false);
        view->SetRenderTarget(eGetRenderTarget(7), 0);
        view->Precipitation = eGetView(2, false)->Precipitation;
        view->SetCamera(&Player2Camera);
        view->SetActive(0);
    }
    {
        eView *view = eGetView(10, false);
        view->SetRenderTarget(eGetRenderTarget(8), 0);
        view->SetCamera(&Player1Camera);
        view->SetActive(1);
    }
}

int eGetZWriteState() {
    return CurrentZWriteState;
}

int eSetZWriteState(int on_off) {
    int previous_state = CurrentZWriteState;

    CurrentZWriteState = on_off != 0;
    return previous_state;
}

void DownloadVUCode() {
    debuggy0 = 0x1100E7A0;
    debuggy1 = 0x1100E7B0;
    debuggy2 = 0x1100E7C0;

    eWaitUntilRenderingDone();

    CPU::IntsOff();
    DMAC::SyncVif1Dma();

    sceDevVu1Pause();
    sceDevVu1Reset();

    uint32 start_time = bGetTicker();
    while (bGetTickerDifference(start_time, bGetTicker()) < 20.0f) {
    }

    uint32 *code = reinterpret_cast<uint32 *>(0x11008000);
    uint32 *data = reinterpret_cast<uint32 *>(0x1100C000);

    for (int c = 0; c < 4096; c++) {
        *code++ = 0;
    }
    for (int d = 0; d < 4096; d++) {
        *data++ = 0xDEADBEEF;
    }

    sceDmaSend(dmaVif1, vu1MicroCodeStart);
    DMAC::SyncVif1Dma();

    start_time = bGetTicker();
    while (bGetTickerDifference(start_time, bGetTicker()) < 20.0f) {
    }

    CPU::IntsOn();
}

void PatchVUCode() {}

void eFixUpTablesPlat() {
    pTextureInfoRVMInfo = GetTextureInfo(0xDBD4, 0, 0);
    pTextureInfoRVMMask = GetTextureInfo(0xCF29D0DF, 0, 0);
    pTextureInfoSolidGrey = GetTextureInfo(0x32283A11, 0, 0);
    pTextureInfoFilterGrey = GetTextureInfo(0x02C8711C, 0, 0);
    pTextureInfoShadowMap1 = GetTextureInfo(0xBAD886D4, 0, 0);
    pTextureInfoRadialBlur = GetTextureInfo(0xAB9A6E21, 0, 0);

    pTextureInfoSpecularFalloff = GetTextureInfo(0x0184D1F8, 0, 0);
    pTextureInfoSpecularSpot = GetTextureInfo(0xB9AB56E4, 0, 0);

    pTextureInfoHeadlights = GetTextureInfo(bStringHash("HEADLIGHTSPSX"), 0, 0);

#ifdef EA_BUILD_A124
    if (TheGameFlowManager.IsInGame()) {
        pTextureInfoRainDropMask = GetTextureInfo(bStringHash("RAINDROPMASK"), 1, 0);
    }
#else
    pTextureInfoRainDropMask = GetTextureInfo(bStringHash("RAINDROPMASK"), 0, 0);
#endif

    pTextureInfoSmallEnvMap = GetTextureInfo(0x7B5EDE7F, 0, 0);
    pTextureInfoCarElectric = GetTextureInfo(0xEA24E29A, 0, 0);

    pTextureInfoTEST0 = GetTextureInfo(bStringHash("SKY_SUNSET_A"), 0, 0);
    pTextureInfoTEST1 = GetTextureInfo(bStringHash("SKY_MIDDAY_A"), 0, 0);

    pTextureInfoGlossyWindow = GetTextureInfo(bStringHash("WINDOWREFLECTION"), 0, 0);
}

void EnvMapTextureLoadedCallback(int32 param) {
    char envMapTexture[64];

    bSPrintf(envMapTexture, "%s_%s", FEManager::Get()->GetGaragePrefixFromType(FEManager::Get()->GetGarageType()),
             "CARSELECTENVMAP");
    pTextureInfoCarSelectEnvMap = GetTextureInfo(bStringHash(envMapTexture), 0, 0);
}

void eInitFEEnvMapPlat() {
    char envMapTexture[64];

    bSPrintf(envMapTexture, "%s_%s", FEManager::Get()->GetGaragePrefixFromType(FEManager::Get()->GetGarageType()),
             "CARSELECTENVMAP");
    eLoadStreamingTexture(bStringHash(envMapTexture), reinterpret_cast<void (*)(uintptr_t)>(EnvMapTextureLoadedCallback), 0, 0);
}

void eRemoveFEEnvMapPlat() {
    char envMapTexture[64];

    if (FEManager::Get()->GetPreviousGarageType() != 0) {
        bSPrintf(envMapTexture, "%s_%s",
                 FEManager::Get()->GetGaragePrefixFromType(FEManager::Get()->GetPreviousGarageType()), "CARSELECTENVMAP");
        uint32 nameHash = bStringHash(envMapTexture);
        if (GetTextureInfo(nameHash, 0, 0)) {
            eUnloadStreamingTexture(nameHash);
        }
    }
    if (FEManager::Get()->GetGarageType() != 0) {
        bSPrintf(envMapTexture, "%s_%s", FEManager::Get()->GetGaragePrefixFromType(FEManager::Get()->GetGarageType()),
                 "CARSELECTENVMAP");
        uint32 nameHash = bStringHash(envMapTexture);
        if (GetTextureInfo(nameHash, 0, 0)) {
            eUnloadStreamingTexture(nameHash);
        }
    }
    pTextureInfoCarSelectEnvMap = nullptr;
}

void eClearRenderTarget(eRenderTarget *render_target, eDmaChain *dma_chain, int clearR, int clearG, int clearB,
                        int clearA) {
    if (glowstage0) {
        uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(14 * sizeof(u_long128)));

        if (data_start) {
            uint32 *data = data_start;

            *reinterpret_cast<u_long *>(data) = MakeDmaTagA(render_target->GetDrawContextData(0), 14, eDMA_REF);
            data += 2;
            *data++ = SCE_VIF1_SET_FLUSHE(0);
            *data++ = SCE_VIF1_SET_DIRECT(14, 0);

            *reinterpret_cast<u_long *>(data) = MakeDmaTag(11, eDMA_CNT);
            data += 2;
            *data++ = SCE_VIF1_SET_FLUSHE(0);
            *data++ = SCE_VIF1_SET_DIRECT(11, 0);

            reinterpret_cast<u_long *>(data)[0] = MakeGifTag(10, 1, 0, 0, GIF_PACKED, 1);
            reinterpret_cast<u_long *>(data)[1] = SCE_GIF_PACKED_AD;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = 0;
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEXFLUSH;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_ALPHA(cleara, clearb, clearc, cleard, clearf);
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

            reinterpret_cast<u_long *>(data)[0] = 0;
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_DTHE;
            data += 4;

            int dx = render_target->ScissorX;
            int dy = render_target->ScissorY;
            int dw = render_target->ScissorW;
            int dh = render_target->ScissorH;

            if (clearA) {
                reinterpret_cast<u_long *>(data)[0] = PRIM_SPRITE | PRIM_ABE;
            } else {
                reinterpret_cast<u_long *>(data)[0] = PRIM_SPRITE;
            }
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_PRIM;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_RGBAQ(clearR, clearG, clearB, clearA, 0x3F800000);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(dx << 4, dy << 4, 0);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
            data += 4;

            reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ((dx + dw) << 4, (dy + dh) << 4, 0);
            reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;

            dma_chain->Add(data_start, data_start + 4);
        }
    }
}

void eClearVram(uint32 colour) {
    static DATA16 packet_buffer[16];
    DATA16 *pbuf = packet_buffer;
    DATA16 *pbuf_start = pbuf;

    pbuf->q = 0;
    pbuf->w[0] = MakeDmaTag(0, eDMA_END);
    pbuf->w[3] = SCE_VIF1_SET_DIRECT(10, 0);
    pbuf++;

    pbuf->d[0] = MakeGifTag(9, 1, 0, 0, GIF_PACKED, 1);
    pbuf->d[1] = SCE_GIF_PACKED_AD;
    pbuf++;

    pbuf->d[0] = 1;
    pbuf->d[1] = SCE_GS_PRMODECONT;
    pbuf++;

    pbuf->d[0] = SCE_GS_SET_FRAME(0, 16, PSMCT32, 0);
    pbuf->d[1] = SCE_GS_FRAME_1;
    pbuf++;

    pbuf->d[0] = 0;
    pbuf->d[1] = SCE_GS_XYOFFSET_1;
    pbuf++;

    pbuf->d[0] = SCE_GS_SET_SCISSOR(0, 1023, 0, 1023);
    pbuf->d[1] = SCE_GS_SCISSOR_1;
    pbuf++;

    pbuf->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
    pbuf->d[1] = SCE_GS_TEST_1;
    pbuf++;

    pbuf->d[0] = PRIM_SPRITE;
    pbuf->d[1] = SCE_GS_PRIM;
    pbuf++;

    pbuf->w[0] = colour;
    pbuf->w[1] = 0x3F800000;
    pbuf->d[1] = SCE_GS_RGBAQ;
    pbuf++;

    pbuf->d[0] = SCE_GS_SET_XYZ(0, 0, 0);
    pbuf->d[1] = SCE_GS_XYZF2;
    pbuf++;

    pbuf->d[0] = SCE_GS_SET_XYZ(0xFFFF, 0xFFFF, 0);
    pbuf->d[1] = SCE_GS_XYZF2;
    pbuf++;

    pbuf_start->s[0] = pbuf - pbuf_start - 1;

    sceDmaChan *dma_channel = sceDmaGetChan(SCE_DMA_VIF1);
    sceDmaSync(dma_channel, 0, 0);
    FlushCache(WRITEBACK_DCACHE);
    dmaVif1->chcr.TTE = 1;
    sceDmaSend(dma_channel, pbuf_start);
}

void eFillScreenWithColour(uint32 colour) {
    eClearVram(colour);
    SetScreenBuffers();
    ePutDisplayEnvironment();
}

// La mitad del campo de vision de referencia (90 grados), para el FovBias de CreateViewMatricies; en el .data de la A124
// va a cero y la pone el constructor estatico.
bAngle aBaselineFovMip = bDegToAng(90.0f);

float CalculateH(bAngle alpha, eView *view) {
    float tan_half = bTan(alpha >> 1);

    if (tan_half != 0.0f) {
        return (view->GetRenderTarget()->ScissorW >> 1) / tan_half;
    }
    return 10.0f;
}

void eDmaInit() {
    {
        DATA16 *tag0 = reinterpret_cast<DATA16 *>(SentinelDmaTags[eDmaChain::TERMINATE_DEFAULT]);

        tag0->d[0] = MakeDmaTagA(0, 0, eDMA_END);
        tag0->w[2] = SCE_VIF1_SET_MARK(0xABCD, 0);
        tag0->w[3] = SCE_VIF1_SET_NOP(0);
    }
    {
        DATA16 *tag0 = reinterpret_cast<DATA16 *>(SentinelDmaTags[eDmaChain::TERMINATE_INTERUPT]);

        tag0->d[0] = MakeDmaTagA(0, 0, eDMA_END);
        tag0->w[2] = SCE_VIF1_SET_MARK(0xABCD, 0);
        tag0->w[3] = SCE_VIF1_SET_NOP(1);
    }
    {
        DATA16 *tag0 = reinterpret_cast<DATA16 *>(SentinelDmaTags[eDmaChain::TERMINATE_FLUSHA]);

        tag0->d[0] = MakeDmaTagA(0, 0, eDMA_END);
        tag0->w[2] = SCE_VIF1_SET_MARK(0xABCD, 0);
        tag0->w[3] = SCE_VIF1_SET_FLUSHA(0);
    }
    {
        DATA16 *tag0 = reinterpret_cast<DATA16 *>(&SentinelDmaTags[eDmaChain::TERMINATE_FLUSHA_INTERUPT][0]);
        DATA16 *tag1 = reinterpret_cast<DATA16 *>(&SentinelDmaTags[eDmaChain::TERMINATE_FLUSHA_INTERUPT][1]);

        tag0->d[0] = MakeDmaTagA(0, 1, eDMA_END);
        tag0->w[2] = SCE_VIF1_SET_MARK(0xABCD, 0);
        tag0->w[3] = SCE_VIF1_SET_FLUSHA(0);

        tag1->w[0] = SCE_VIF1_SET_NOP(0);
        tag1->w[1] = SCE_VIF1_SET_NOP(0);
        tag1->w[2] = SCE_VIF1_SET_NOP(0);
        tag1->w[3] = SCE_VIF1_SET_NOP(1);
    }
    {
        DATA16 *tag0 = reinterpret_cast<DATA16 *>(&SentinelDmaTags[eDmaChain::TERMINATE_ENABLE_3_FLUSHA_INTERUPT][0]);
        DATA16 *tag1 = reinterpret_cast<DATA16 *>(&SentinelDmaTags[eDmaChain::TERMINATE_ENABLE_3_FLUSHA_INTERUPT][1]);

        tag0->d[0] = MakeDmaTagA(0, 1, eDMA_END);
        tag0->w[2] = SCE_VIF1_SET_MARK(0xABCD, 0);
        tag0->w[3] = SCE_VIF1_SET_FLUSHA(0);

        tag1->w[0] = SCE_VIF1_SET_NOP(0);
        tag1->w[1] = SCE_VIF1_SET_NOP(0);
        tag1->w[2] = SCE_VIF1_SET_NOP(0);
        tag1->w[3] = SCE_VIF1_SET_NOP(1);
    }
}

void eDmaChain::Init() {
    DATA16 *tag = reinterpret_cast<DATA16 *>(&BeginingDmaTag);

    tag->d[0] = MakeDmaTag(0, eDMA_NEXT);
    tag->w[2] = SCE_VIF1_SET_STCYCL(1, 1, 0);
    tag->w[3] = SCE_VIF1_SET_MARK(0xEEEE, 0);

    CurrentDmaTag = &BeginingDmaTag;
}

void eDmaChain::Add(void *packet_start) {
    DATA16 *current = reinterpret_cast<DATA16 *>(CurrentDmaTag);

    current->w[1] = reinterpret_cast<unsigned int>(packet_start);
    current->c[3] = DMA_NEXT;

    CurrentDmaTag = static_cast<u_long128 *>(packet_start);
}

void eDmaChain::Add(void *packet_start, void *packet_end) {
    DATA16 *current = reinterpret_cast<DATA16 *>(CurrentDmaTag);

    current->w[1] = reinterpret_cast<unsigned int>(packet_start);
    current->c[3] = DMA_NEXT;

    CurrentDmaTag = static_cast<u_long128 *>(packet_end);
}

void eDmaChain::Terminate(EDMA_TERMINATE flag) {
    DATA16 *current = reinterpret_cast<DATA16 *>(CurrentDmaTag);
    uint32 sentinel = reinterpret_cast<uint32>(SentinelDmaTags[flag]);

    current->w[1] = sentinel;
    current->c[3] = DMA_NEXT;

    CurrentDmaTag = nullptr;
}

void ResetDmaStats() {}

void eDmaDataList::Init() {
    Reset();
    LightMaterial = nullptr;
    LightContext = nullptr;

    LocalWorldMatrix = nullptr;
    MatrixPack = nullptr;
    BoneMatrixPack = nullptr;
}

inline eDmaData *eDmaDataList::AddData(unsigned short type, void *data, unsigned short qwsize,
                                       unsigned short parameter) {
    eDmaData *dma_data = static_cast<eDmaData *>(eFrameMalloc(sizeof(eDmaData)));

    if (dma_data) {
        dma_data->Type = type;
        dma_data->Parameter = parameter;
        dma_data->QWordSize = qwsize;
        dma_data->pData = static_cast<unsigned char *>(data);
        AddTail(dma_data);
    }
    return dma_data;
}

void eDmaDataList::AddDataHead(unsigned short type, void *data, unsigned short qwsize, unsigned short parameter) {
    eDmaData *dma_data = static_cast<eDmaData *>(eFrameMalloc(sizeof(eDmaData)));

    if (dma_data) {
        dma_data->Type = type;
        dma_data->Parameter = parameter;
        dma_data->QWordSize = qwsize;
        dma_data->pData = static_cast<unsigned char *>(data);
        AddHead(dma_data);
    }
}

void eDmaDataList::SetLocalWorldMatrix(bMatrix4 *matrix) {
    if (LocalWorldMatrix != matrix) {
        LocalWorldMatrix = matrix;
        AddData(EDMA_SET_LOCAL_WORLD, matrix, 4, 845);
    }
}

void eDmaDataList::SetLightMaterial(eLightMaterial *light_material) {
    if (LightMaterial != light_material) {
        LightMaterial = light_material;
        AddData(EDMA_LIGHTMATERIAL, light_material, 6, 859);
    }
}

void eDmaDataList::SetLightContext(eLightContext *light_context) {
    if (light_context->Type != 0) {
        return;
    }
    if (light_context != LightContext) {
        LightContext = light_context;
        AddData(EDMA_LIGHTCONTEXT, &static_cast<eDynamicLightContext *>(light_context)->LocalColourMatrix, 18, 871);
    }
}

void eDmaDataList::SetMatrixPack(eRenderTargetMatrixPack *matrix_pack) {
    if (matrix_pack != MatrixPack) {
        MatrixPack = matrix_pack;
        AddDataHead(EDMA_SET_MATRIX_PACK, matrix_pack, 0, 0);
    }
}

void eDmaDataList::SetBoneMatrixPack(bMatrix4 *bone_matrix_pack) {
    if (bone_matrix_pack != BoneMatrixPack) {
        BoneMatrixPack = bone_matrix_pack;
        AddData(EDMA_SET_BONE_MATRIX_PACK, bone_matrix_pack, 0, 0);
    }
}

inline eActiveTexture::eActiveTexture(TextureInfo *texture_info) {
    pTextureInfo = texture_info;
    texture_info->GetPlatInfo()->pActiveTexture = this;

    pPreviousDmaData = nullptr;

    reinterpret_cast<u_long128 *>(pDmaDataList)[0] = 0;
    reinterpret_cast<u_long128 *>(pDmaDataList)[1] = 0;
    reinterpret_cast<u_long128 *>(pDmaDataList)[2] = 0;
    reinterpret_cast<u_long128 *>(pDmaDataList)[3] = 0;
    reinterpret_cast<u_long128 *>(pDmaDataList)[4] = 0;
    pDmaDataList[20] = nullptr;
}

inline eActiveTexture::~eActiveTexture() {
    pTextureInfo->GetPlatInfo()->pActiveTexture = nullptr;
}

eActiveTexture *eCreateActiveTexture(TextureInfo *texture_info) {
    eActiveTexture *active_texture = static_cast<eActiveTexture *>(texture_info->GetPlatInfo()->pActiveTexture);

    if (!active_texture) {
        int apply_alpha_sorting = texture_info->ApplyAlphaSorting;

        active_texture = new eActiveTexture(texture_info);
        if (apply_alpha_sorting) {
            AlphaTextureList.AddTail(active_texture);
        } else {
            SolidTextureList.AddTail(active_texture);
        }
    }
    return active_texture;
}

// Inline de EcstasyE.hpp en el original (lineas 440-448): la lista de DMA de la textura en un render target; con
// create la crea si aun no existe (AttachAllPolysNoTexUpload la pide sin crear: el codigo de crearla no sale). Va aqui
// porque necesita eFrameMalloc (Ecstasy.hpp).
inline eDmaDataList *eActiveTexture::GetDmaDataList(int render_target_id, int create) {
    eDmaDataList *dma_data_list = pDmaDataList[render_target_id];

    if (!dma_data_list && create) {
        dma_data_list = static_cast<eDmaDataList *>(eFrameMalloc(sizeof(eDmaDataList)));

        if (dma_data_list) {
            dma_data_list->Init();
        }
        pDmaDataList[render_target_id] = dma_data_list;
    }
    return dma_data_list;
}

void TestSort() {}

struct eAlphaSortKey : public eSNode<eAlphaSortKey> {
    // total size: 0x10
    eDmaDataList *DmaDataList;      // offset 0x4, size 0x4
    eActiveTexture *ActiveTexture;  // offset 0x8, size 0x4
    float Z;                        // offset 0xC, size 0x4
};

struct eAlphaList : public eSList<eAlphaSortKey> {
    // total size: 0xC
    int32 NumKeys; // offset 0x8, size 0x4

    void AddKey(eAlphaSortKey *key);
    void Flush();
};

inline void eAlphaList::Flush() {
    Reset();
    NumKeys = 0;
}

inline void eAlphaList::AddKey(eAlphaSortKey *alpha_key) {
    *TailPtr = alpha_key;
    TailPtr = &alpha_key->Next;
    alpha_key->Next = 0;
    NumKeys++;
}

eAlphaList AlphaBucketLists[8];

eAlphaSortKey *eNewAlphaSortKey() {
    eAlphaSortKey *alpha_key = static_cast<eAlphaSortKey *>(eFrameMalloc(sizeof(eAlphaSortKey) + sizeof(eDmaDataList)));
    eDmaDataList *dma_data_list = reinterpret_cast<eDmaDataList *>(alpha_key + 1);

    if (alpha_key && dma_data_list) {
        alpha_key->Z = 0.0f;

        alpha_key->ActiveTexture = nullptr;

        alpha_key->DmaDataList = dma_data_list;
        dma_data_list->Init();

        return alpha_key;
    }
    return nullptr;
}

// Ordena las claves de alfa por Z (radix de 8 bits sobre los bits del float, con las claves negativas al reves) y, en
// la ultima pasada, por el orden de dibujo de la textura. Las claves se copian de 128 en 128 bits.
//
// El `pref` es asm de EA (aprobado el 2026-10-01): el cc1plus de ee-gcc 2.9-ee-991111 no tiene ningun patron que
// emita `pref` (no esta en su tabla de patrones ni en sus plantillas de salida), asi que solo puede salir de un asm.
// El original lo lleva en cuatro sitios, cada uno con su linea de fuente propia (EcstasyE.cpp:2731/2735, 2766/2770
// y 2869/2873 de la A124): 64 bytes por delante antes de cada bucle y 128 dentro.
eAlphaSortKey *RadixSortAlphaKeys(eAlphaSortKey *sorted_buffer0, eAlphaSortKey *sorted_buffer1, eAlphaList *alpha_list) {
    eAlphaSortKey *track_key;
    int32 num_keys = alpha_list->NumKeys;
    uint16 renderorders[8];
    uint16 counters[4][256];
    int32 num_negative_keys;
    int num_positive_passes;

    num_negative_keys = 0;

    bMemSet(renderorders, 0, sizeof(renderorders));
    bMemSet(counters, 0, sizeof(counters));

    track_key = sorted_buffer0;
    for (eAlphaSortKey *alpha_key = alpha_list->GetHead(); alpha_key != alpha_list->EndOfList();
         alpha_key = alpha_key->GetNext()) {
        u_long128 key_data = *reinterpret_cast<u_long128 *>(alpha_key);
        uint32 key = *reinterpret_cast<uint32 *>(&alpha_key->Z);
        uint16 render_order = alpha_key->ActiveTexture->pTextureInfo->RenderingOrder;

        counters[0][key & 0xff]++;
        counters[1][(key >> 8) & 0xff]++;
        counters[2][(key >> 16) & 0xff]++;
        counters[3][key >> 24]++;

        num_negative_keys += key >> 31;

        if (render_order > 7) {
            render_order = 7;
        }
        renderorders[7 - render_order]++;

        *reinterpret_cast<u_long128 *>(track_key) = key_data;
        track_key++;
    }

    num_positive_passes = num_negative_keys ? 3 : 4;

    for (int p = 0; p < num_positive_passes; p++) {
        uint16 prev_offset = 0;

        for (int32 i = 0; i < 256; i++) {
            uint16 count = counters[p][i];
            counters[p][i] = prev_offset;
            prev_offset += count;
        }
    }

    if (num_negative_keys) {
        uint16 prev_offset = num_negative_keys;

        for (int32 i = 0; i < 128; i++) {
            uint16 count = counters[3][i];
            counters[3][i] = prev_offset;
            prev_offset += count;
        }

        prev_offset = 0;
        for (int32 j = 255; j >= 128; j--) {
            uint16 count = counters[3][j];

            counters[3][j] = prev_offset + count;
            prev_offset = counters[3][j];
        }
    }

    {
        uint16 prev_offset = 0;

        for (int32 i = 0; i < 8; i++) {
            uint16 count = renderorders[i];
            renderorders[i] = prev_offset;
            prev_offset += count;
        }
    }

    for (int32 pass = 0; pass < num_positive_passes; pass++) {
        eAlphaSortKey *temp_buffer;

        track_key = sorted_buffer0;
        asm("pref 0, 64(%0)" : : "r"(track_key));

        for (int32 i = 0; i < num_keys; i++) {
            asm("pref 0, 128(%0)" : : "r"(track_key));

            u_long128 sort_key = *reinterpret_cast<u_long128 *>(track_key);
            uint32 key = *reinterpret_cast<uint32 *>(&track_key->Z);
            uint32 index = (key >> (pass * 8)) & 0xff;
            uint32 offset = counters[pass][index];

            counters[pass][index] = offset + 1;

            u_long128 *out_key = reinterpret_cast<u_long128 *>(&sorted_buffer1[offset]);
            *out_key = sort_key;

            track_key++;
        }

        temp_buffer = sorted_buffer0;
        sorted_buffer0 = sorted_buffer1;
        sorted_buffer1 = temp_buffer;
    }

    if (num_negative_keys) {
        eAlphaSortKey *temp_buffer;

        track_key = sorted_buffer0;
        asm("pref 0, 64(%0)" : : "r"(track_key));

        for (int32 i = 0; i < num_keys; i++) {
            asm("pref 0, 128(%0)" : : "r"(track_key));

            u_long128 sort_key = *reinterpret_cast<u_long128 *>(track_key);
            uint32 key = *reinterpret_cast<uint32 *>(&track_key->Z);
            uint32 index = key >> 24;
            uint32 offset = counters[3][index];

            if (static_cast<int32>(key) >= 0) {
                counters[3][index] = offset + 1;
                *reinterpret_cast<u_long128 *>(sorted_buffer1 + offset) = sort_key;
            } else {
                counters[3][index] = offset - 1;
                *reinterpret_cast<u_long128 *>(sorted_buffer1 + offset - 1) = sort_key;
            }

            track_key++;
        }

        temp_buffer = sorted_buffer0;
        sorted_buffer0 = sorted_buffer1;
        sorted_buffer1 = temp_buffer;
    }

    {
        track_key = sorted_buffer0;
        asm("pref 0, 64(%0)" : : "r"(track_key));

        for (int32 i = 0; i < num_keys; i++) {
            asm("pref 0, 128(%0)" : : "r"(track_key));

            u_long128 sort_key = *reinterpret_cast<u_long128 *>(track_key);
            uint32 key = track_key->ActiveTexture->pTextureInfo->RenderingOrder;

            if (key > 7) {
                key = 7;
            }

            uint32 offset = renderorders[7 - key];

            renderorders[7 - key] = offset + 1;
            *reinterpret_cast<u_long128 *>(&sorted_buffer1[offset]) = sort_key;

            track_key++;
        }
    }

    return sorted_buffer1;
}

int SortAlphaKeys(eAlphaList *alpha_list) {
    int32 num_alpha_keys = alpha_list->NumKeys;

    if (!alpha_list || !num_alpha_keys) {
        return 0;
    }

    eAlphaSortKey *sorted_buffer0 =
        static_cast<eAlphaSortKey *>(eFrameMalloc(num_alpha_keys * sizeof(eAlphaSortKey) * 2));
    eAlphaSortKey *sorted_buffer1 = sorted_buffer0 + num_alpha_keys;

    if (!sorted_buffer0) {
        return 0;
    }

    eAlphaSortKey *output_buffer = RadixSortAlphaKeys(sorted_buffer1, sorted_buffer0, alpha_list);

    if (!output_buffer) {
        return -1;
    }

    eAlphaSortKey *head_key = output_buffer + (num_alpha_keys - 1);

    alpha_list->Reset();

    for (int i = 0; i < num_alpha_keys; i++) {
        alpha_list->AddTail(head_key);
        head_key--;
    }

    return 1;
}

inline int eVRAMmanager::GetCurrentLayout() {
    if (TheGameFlowManager.IsInFrontend() && !MovieIsPlaying()) {
        return 2;
    }
    return 0;
}

int32 eVRAMmanager::GetArea(int area_id, int layout_id) {
    return Layouts[layout_id].Areas[area_id].Address;
}

void eVRAMmanager::SetLayout(int layout_id, int max_size, const char *name) {
    eVRAMLayout *layout = &Layouts[layout_id];

    layout->MaxSize = max_size;
    layout->Name = name;
}

void eVRAMmanager::UseArea(int layout_id, int src, int dst, int amount, int align, char *name) {
    eVRAMLayout *layout = &Layouts[layout_id];
    eVRAMArea *src_area = &layout->Areas[src];
    eVRAMArea *dst_area = &layout->Areas[dst];

    src_area->Valid = 1;
    src_area->Address = dst_area->Address;
    src_area->Size = amount;
    src_area->Name = name;
}

void eVRAMmanager::AddArea(int layout_id, int src, int amount, int align, char *name) {
    eVRAMLayout *layout = &Layouts[layout_id];
    eVRAMArea *area = &layout->Areas[src];

    int current_address = layout->CurrentAddress;

    if (align) {
        current_address = (current_address + 8191) & ~8191;
    }

    if (current_address + amount <= layout->MaxSize) {
        area->Valid = 1;
        area->Address = current_address;
        area->Size = amount;
        area->Name = name;

        layout->CurrentAddress = current_address + amount;
    }
}

enum EDISPLAY_MODE {
    EDISP_NONE = 0,
    EDISP_FULLHEIGHT = 1,
    EDISP_HALFHEIGHT_NEW = 2,
};

enum GSReg {
    GSREG_PRIM = 0,
    GSREG_RGBAQ = 1,
    GSREG_ST = 2,
    GSREG_UV = 3,
    GSREG_XYZF2 = 4,
    GSREG_XYZ2 = 5,
    GSREG_TEX0_1 = 6,
    GSREG_TEX0_2 = 7,
    GSREG_CLAMP_1 = 8,
    GSREG_CLAMP_2 = 9,
    GSREG_FOG = 10,
    GSREG_XYZF3 = 12,
    GSREG_XYZ3 = 13,
    GSREG_TEX1_1 = 20,
    GSREG_TEX1_2 = 21,
    GSREG_TEX2_1 = 22,
    GSREG_TEX2_2 = 23,
    GSREG_XYOFFSET_1 = 24,
    GSREG_XYOFFSET_2 = 25,
    GSREG_PRMODECONT = 26,
    GSREG_PRMODE = 27,
    GSREG_TEXCLUT = 28,
    GSREG_SCANMSK = 34,
    GSREG_MIPTBP1_1 = 52,
    GSREG_MIPTBP1_2 = 53,
    GSREG_MIPTBP2_1 = 54,
    GSREG_MIPTBP2_2 = 55,
    GSREG_TEXA = 59,
    GSREG_FOGCOL = 61,
    GSREG_TEXFLUSH = 63,
    GSREG_SCISSOR_1 = 64,
    GSREG_SCISSOR_2 = 65,
    GSREG_ALPHA_1 = 66,
    GSREG_ALPHA_2 = 67,
    GSREG_DIMX = 68,
    GSREG_DTHE = 69,
    GSREG_COLCLAMP = 70,
    GSREG_TEST_1 = 71,
    GSREG_TEST_2 = 72,
    GSREG_PABE = 73,
    GSREG_FBA_1 = 74,
    GSREG_FBA_2 = 75,
    GSREG_FRAME_1 = 76,
    GSREG_FRAME_2 = 77,
    GSREG_ZBUF_1 = 78,
    GSREG_ZBUF_2 = 79,
    GSREG_BITBLTBUF = 80,
    GSREG_TRXPOS = 81,
    GSREG_TRXREG = 82,
    GSREG_TRXDIR = 83,
    GSREG_HWREG = 84,
    GSREG_SIGNAL = 96,
    GSREG_FINISH = 97,
    GSREG_LABEL = 98,
    GSREG_NOP = 127,
};

eVRAMmanager eVRAM;
int DownloadVramStartAddrees;

void InitVRAM() {
    eVRAM.SetLayout(0, 0x400000, "In-Game");

    eVRAM.AddArea(0, VRAM_FRAMEBUFFER0, 0x100000, 1, "FrameBuffer0");
    eVRAM.AddArea(0, VRAM_FRAMEBUFFER1, 0x100000, 1, "FrameBuffer1");

    eVRAM.AddArea(0, VRAM_ZBUFFER, 0x100000, 1, "ZBuffer");

    eVRAM.AddArea(0, VRAM_ENVMAP0_FULL, 0x20000, 1, "Envmap FULL");

#ifdef EA_BUILD_A124
    eVRAM.AddArea(0, VRAM_GLOSSYWINDOW, 0x8000, 1, "Glossy Window");
    eVRAM.AddArea(0, VRAM_GLOSSYWINDOW_PALETTE, 0x400, 1, "Glossy Window Palette");
#endif
    eVRAM.AddArea(0, VRAM_SMALL_ENVMAP, 0x4000, 1, "small envmap");
    eVRAM.AddArea(0, VRAM_SMALL_ENVMAP_PALETTE, 0x400, 1, "small envmap palette");

    eVRAM.AddArea(0, VRAM_DUMMY_8K, 0x2000, 1, "Dummy 8K");

    eVRAM.AddArea(0, VRAM_START_TEXTURES_FE, 0, 0, "TexStart-FE");

    eVRAM.AddArea(0, VRAM_REFLECTION, 0x20000, 1, nullptr);
    eVRAM.AddArea(0, VRAM_SPECULAR, 0x8000, 1, nullptr);
    eVRAM.AddArea(0, VRAM_REFLECTION_ZBUF, 0x20000, 1, nullptr);

    eVRAM.AddArea(0, VRAM_ENVRMAP_FRAME_START, 0, 1, nullptr);
    eVRAM.AddArea(0, VRAM_ENVRMAP0B_FRAME, 0x2000, 1, nullptr);
    eVRAM.AddArea(0, VRAM_ENVRMAP0L_FRAME, 0x2000, 0, nullptr);
    eVRAM.AddArea(0, VRAM_ENVRMAP0R_FRAME, 0x2000, 0, nullptr);
    eVRAM.AddArea(0, VRAM_ENVRMAP0U_FRAME, 0x2000, 0, nullptr);
    eVRAM.AddArea(0, VRAM_ENVRMAP0D_FRAME, 0x2000, 0, nullptr);

    eVRAM.AddArea(0, VRAM_ENVRMAP_ZBUF_START, 0, 1, nullptr);
    eVRAM.AddArea(0, VRAM_ENVRMAP0B_ZBUF, 0x2000, 0, nullptr);
    eVRAM.AddArea(0, VRAM_ENVRMAP0L_ZBUF, 0x2000, 0, nullptr);
    eVRAM.AddArea(0, VRAM_ENVRMAP0R_ZBUF, 0x2000, 0, nullptr);
    eVRAM.AddArea(0, VRAM_ENVRMAP0U_ZBUF, 0x2000, 0, nullptr);
    eVRAM.AddArea(0, VRAM_ENVRMAP0D_ZBUF, 0x2000, 0, nullptr);

    eVRAM.AddArea(0, VRAM_RVM1_ZBUF, 0x10000, 0, nullptr);
    eVRAM.AddArea(0, VRAM_RVM1_FRAME, 0x10000, 0, nullptr);

    eVRAM.AddArea(0, VRAM_FOG_PALETTE_START, 0x800, 0, nullptr);

    eVRAM.AddArea(0, VRAM_START_TEXTURES, 0, 0, "START");

    eVRAM.SetLayout(2, 0x400000, "In-FE");

    eVRAM.AddArea(2, VRAM_FRAMEBUFFER0, 0x100000, 1, "FrameBuffer0");
    eVRAM.AddArea(2, VRAM_FRAMEBUFFER1, 0x100000, 1, "FrameBuffer1");

    eVRAM.AddArea(2, VRAM_ZBUFFER, 0x80000, 1, "ZBuffer");

    eVRAM.AddArea(2, VRAM_ENVMAP0_FULL, 0x20000, 1, "Envmap FULL");
    eVRAM.AddArea(2, VRAM_GLOSSYWINDOW, 0x8000, 1, "Glossy Window");
    eVRAM.AddArea(2, VRAM_GLOSSYWINDOW_PALETTE, 0x400, 1, "Glossy Window Palette");
    eVRAM.AddArea(2, VRAM_SMALL_ENVMAP, 0x4000, 1, "small envmap");
    eVRAM.AddArea(2, VRAM_SMALL_ENVMAP_PALETTE, 0x400, 1, "small envmap palette");

    eVRAM.AddArea(2, VRAM_DUMMY_8K, 0x2000, 1, "VRAM_DUMMY_8K");

    eVRAM.AddArea(2, VRAM_START_TEXTURES_FE, 0, 0, "TexStart-FE");

    eVRAM.AddArea(2, VRAM_REFLECTION, 0x20000, 1, nullptr);
    eVRAM.AddArea(2, VRAM_REFLECTION_ZBUF, 0x20000, 1, nullptr);

    eVRAM.AddArea(2, VRAM_DUMMY, 0xC0000, 1, nullptr);

    eVRAM.UseArea(2, VRAM_FRAMEBUFFER2, VRAM_REFLECTION, 1, 0, nullptr);

    eVRAM.AddArea(2, VRAM_START_TEXTURES, 0, 0, "START");

    eVRAM.SetLayout(1, 0x400000, "Special");

    eVRAM.AddArea(1, VRAM_FRAMEBUFFER0, 0x100000, 1, nullptr);
    eVRAM.AddArea(1, VRAM_FRAMEBUFFER1, 0x100000, 1, nullptr);
    eVRAM.AddArea(1, VRAM_ZBUFFER, 0x100000, 1, nullptr);

    eVRAM.AddArea(1, VRAM_ENVMAP0_FULL, 0x20000, 1, nullptr);

    eVRAM.AddArea(1, VRAM_GLOSSYWINDOW, 0x8000, 1, "Glossy Window");
    eVRAM.AddArea(1, VRAM_GLOSSYWINDOW_PALETTE, 0x400, 1, "Glossy Window Palette");
    eVRAM.AddArea(1, VRAM_SMALL_ENVMAP, 0x4000, 1, "small envmap");
    eVRAM.AddArea(1, VRAM_SMALL_ENVMAP_PALETTE, 0x400, 1, "small envmap palette");

    eVRAM.AddArea(1, VRAM_DUMMY_8K, 0x2000, 1, nullptr);

    eVRAM.AddArea(1, VRAM_128_A, 0x10000, 1, "VRAM_128_A");
    eVRAM.AddArea(1, VRAM_128_B, 0x10000, 1, "VRAM_128_B");
    eVRAM.AddArea(1, VRAM_128_C, 0x10000, 1, "VRAM_128_C");
    eVRAM.AddArea(1, VRAM_128_D, 0x10000, 1, "VRAM_128_D");

    eVRAM.UseArea(1, VRAM_256_B, VRAM_128_A, 0, 1, "VRAM_256_B");
    eVRAM.UseArea(1, VRAM_256_B, VRAM_COMBINER, 0, 1, "VRAM_COMBINER");

    eVRAM.AddArea(1, VRAM_256_A, 0x40000, 1, "VRAM_256_A");

    eVRAM.AddArea(1, VRAM_FOG_PALETTE_START, 0xC00, 0, nullptr);

    eVRAM.AddArea(1, VRAM_START_SPECIAL, 0, 0, "START_SPECIAL");

    eVRAM.UseArea(1, VRAM_RAMP_PALETTE_START, VRAM_128_C, 0, 1, "VRAM_RAMP_PALETTE_START");

    DownloadVramStartAddrees = eVRAM.GetArea(VRAM_START_TEXTURES, 0);
}

const char *EnvmapTargetNames[7] = {"TARGET_ENVRMAP0B", "TARGET_ENVRMAP0L", "TARGET_ENVRMAP0F", "TARGET_ENVRMAP0R",
                                    "TARGET_ENVRMAP0U", "TARGET_ENVRMAP0D", "TARGET_ENVMAP0_FULL"};
eAlphaList RenderTargetAlphaLists[24];
// Volatiles: ePutDisplayEnvironment los lee desde la interrupcion del VBlank (eVblCallback).
eRenderTarget *volatile MainDisp0Context = &RenderTargets[TARGET_FRAMEBUFFER0];
eRenderTarget *volatile MainDisp1Context = &RenderTargets[TARGET_FRAMEBUFFER0];
eRenderTarget *volatile MainDraw0Context = &RenderTargets[TARGET_FRAMEBUFFER1];
eRenderTarget *volatile MainDraw1Context = &RenderTargets[TARGET_FRAMEBUFFER1];
eRenderTarget *CurrentRenderTarget = nullptr;
EDISPLAY_MODE PreviousDisplayBufferMode = EDISP_FULLHEIGHT;
EDISPLAY_MODE DisplayBufferMode = EDISP_FULLHEIGHT;

int eGetScreenWidth() {
    return ScreenWidth;
}

int eGetScreenHeight() {
    return ScreenHeight;
}

int PAL_OFFX = 4;
int PAL_OFFY = 25;

void eSetScreenDisplayOffsets(int offset_x, int offset_y) {
    VIDEO_MODE video_mode = GetVideoMode();

    if (video_mode == MODE_NTSC) {
        ScreenDisplayOffsetsXY[video_mode][0] = offset_x * 8;
        ScreenDisplayOffsetsXY[video_mode][1] = offset_y;
    } else {
        ScreenDisplayOffsetsXY[video_mode][0] = (offset_x + PAL_OFFX) * 8;
        ScreenDisplayOffsetsXY[video_mode][1] = offset_y + PAL_OFFY;
    }
}

void eGetScreenDisplayOffsets(int &offset_x, int &offset_y) {
    VIDEO_MODE video_mode = GetVideoMode();

    offset_x = ScreenDisplayOffsetsXY[video_mode][0];
    offset_y = ScreenDisplayOffsetsXY[video_mode][1];
}

int32 GetImageCompressionFromPSM(int32 psm) {
    switch (psm) {
    case PSMCT32:
    case PSMT8H:
    case PSMT4HL:
    case PSMT4HH:
    case PSMZ32:
        return TEXCOMP_32BIT;
    case PSMCT24:
    case PSMZ24:
        return TEXCOMP_24BIT;
    case PSMCT16:
    case PSMCT16S:
    case PSMZ16:
    case PSMZ16S:
        return TEXCOMP_16BIT;
    case PSMT8:
        return TEXCOMP_8BIT;
    case PSMT4:
        return TEXCOMP_4BIT;
    }
    return TEXCOMP_DEFAULT;
}

TextureInfo *eGetRenderTargetTextureInfo(int32 name_hash) {
    for (int i = 0; i < 24; i++) {
        TextureInfo *info = eGetRenderTarget(i)->GetTextureInfo(0);

        if (name_hash == info->NameHash) {
            return info;
        }
    }
    return nullptr;
}

eRenderTarget *eGetCurrentRenderTarget() {
    return CurrentRenderTarget;
}

void eSetCurrentRenderTarget(eRenderTarget *render_target) {
    CurrentRenderTarget = render_target;
}

eRenderTarget *eGetRenderTarget(int32 render_target) {
    return &RenderTargets[render_target];
}

float ofsetdata[2][3] = {{1824.0f, 1936.0f, 2228.0f}, {1792.0f, 1936.0f, 2176.0f}};


// Los tres paquetes de matrices del render target para el microcodigo de la VU1: clip a pantalla de cada uno, mundo
// a clip, vista a mundo, pantalla a mundo, la distancia de la niebla, la nubosidad, la camara y la direccion del sol.
void eRenderTarget::SetupMatrixPacks() {
    eRenderTargetMatrixPack *matrix_pack0 = static_cast<eRenderTargetMatrixPack *>(eFrameMalloc(sizeof(eRenderTargetMatrixPack)));
    eRenderTargetMatrixPack *matrix_pack1 = static_cast<eRenderTargetMatrixPack *>(eFrameMalloc(sizeof(eRenderTargetMatrixPack)));
    eRenderTargetMatrixPack *matrix_pack2 = static_cast<eRenderTargetMatrixPack *>(eFrameMalloc(sizeof(eRenderTargetMatrixPack)));
    bMatrix4 PackViewWorld;
    bMatrix4 PackScreenWorld;

    eTransposeMatrix(&PackViewWorld, WorldView);

    if (ID == TARGET_PLAYER1) {
        PackScreenWorld = VU1clipToTexMatrix[eFrameCounter & 1][0];
    } else if (ID == TARGET_PLAYER2) {
        PackScreenWorld = VU1clipToTexMatrix[eFrameCounter & 1][1];
    }

    eRenderTargetMatrixPack *matrix_pack = nullptr;

    for (int pack = 0; pack < 3; pack++) {
        if (pack == 0) {
            matrix_pack = matrix_pack0;

            if (matrix_pack) {
                matrix_pack->ClipGBScreen0 = VUClipGBScreen0->v0;
                matrix_pack->ClipGBScreen1 = VUClipGBScreen0->v1;
                matrix_pack->ScreenClipGB0 = VUClipGBScreen0->v2;
                matrix_pack->ScreenClipGB1 = VUClipGBScreen0->v3;

                MatrixPack0 = matrix_pack;
            } else {
                MatrixPack0 = &DefaultMatrixPack;
            }
        } else if (pack == 1) {
            matrix_pack = matrix_pack1;

            if (matrix_pack) {
                matrix_pack->ClipGBScreen0 = VUClipGBScreen1->v0;
                matrix_pack->ClipGBScreen1 = VUClipGBScreen1->v1;
                matrix_pack->ScreenClipGB0 = VUClipGBScreen1->v2;
                matrix_pack->ScreenClipGB1 = VUClipGBScreen1->v3;

                MatrixPack1 = matrix_pack;
            } else {
                MatrixPack1 = &DefaultMatrixPack;
            }
        } else if (pack == 2) {
            matrix_pack = matrix_pack2;

            if (matrix_pack) {
                matrix_pack->ClipGBScreen0 = VUClipGBScreen2->v0;
                matrix_pack->ClipGBScreen1 = VUClipGBScreen2->v1;
                matrix_pack->ScreenClipGB0 = VUClipGBScreen2->v2;
                matrix_pack->ScreenClipGB1 = VUClipGBScreen2->v3;

                MatrixPack2 = matrix_pack;
            } else {
                MatrixPack2 = &DefaultMatrixPack;
            }
        }

        if (matrix_pack) {
            matrix_pack->WorldClipGB = *WorldClipGB;
            matrix_pack->ViewWorld = PackViewWorld;
            matrix_pack->ScreenWorld = PackScreenWorld;

            matrix_pack->GenericData.x = 1792.0f;

            if (eGetCurrentViewMode() == EVIEWMODE_TWOH) {
                if (ScissorY == 0) {
                    if (eCurrentVideoMode == MODE_NTSC) {
                        matrix_pack->GenericData.y = ofsetdata[0][1];
                    } else {
                        matrix_pack->GenericData.y = ofsetdata[1][1];
                    }
                } else {
                    if (eCurrentVideoMode == MODE_NTSC) {
                        matrix_pack->GenericData.y = ofsetdata[0][2];
                    } else {
                        matrix_pack->GenericData.y = ofsetdata[1][2];
                    }
                }
            } else {
                if (eCurrentVideoMode == MODE_NTSC) {
                    matrix_pack->GenericData.y = ofsetdata[0][0];
                } else {
                    matrix_pack->GenericData.y = ofsetdata[1][0];
                }
            }

            eView *view;

            if (ID == TARGET_PLAYER1) {
                view = eGetView(EVIEW_PLAYER1, false);
            } else if (ID == TARGET_PLAYER2) {
                view = eGetView(EVIEW_PLAYER2, false);
            } else {
                matrix_pack->GenericData.w = 0.0f;
                continue;
            }

            if (view->Precipitation) {
                matrix_pack->GenericData.w = view->Precipitation->GetCloudIntensity() * 128.0f;
                matrix_pack->GenericData.z = 255.0f - matrix_pack->GenericData.w;
            }

            bVector3 *camera_position = view->GetCamera()->GetPosition();

            matrix_pack->CameraPos.x = camera_position->x;
            matrix_pack->CameraPos.y = camera_position->y;
            matrix_pack->CameraPos.z = camera_position->z;

            bVector4 SunPosition;

            GetSunPos(view, &SunPosition.x, &SunPosition.y, &SunPosition.z);
            bSub(&matrix_pack->SunDirection, &SunPosition, &matrix_pack->CameraPos);
            matrix_pack->SunDirection.w = 1.0f;
            bNormalize(&matrix_pack->SunDirection, &matrix_pack->SunDirection);
        }
    }
}

TextureInfo *eRenderTarget::GetTextureInfo(int index) {
    if (index < 0 || index > 1) {
        index = 0;
    }
    return &RenderTargetTextureInfos[GetID()][index];
}

// Rellena los paquetes del GS del render target para este fotograma: el de borrar el z-buffer y los cinco contextos de
// dibujo (solido y alfa, con y sin escritura de z, y el especial), y deja preparadas las dos TextureInfo que permiten
// usar el render target como textura. Los paquetes van en uno de los dos bloques UgiftagP (doble buffer por fotograma).
void eRenderTarget::UpdateGifTags(int half_offset_x, int half_offset_y, int quarter_offset_x, int quarter_offset_y,
                                  eRenderStageQueue *render_stage_queue, int fast) {
    u_long128 *data_start = UgiftagP[eFrameCounter & 1];

    ClearZBufferData = reinterpret_cast<unsigned int *>(data_start);
    DrawContextData[DRAW_CONTEXT_SOLID_NOZ] = reinterpret_cast<unsigned int *>(data_start + 8);
    DrawContextData[DRAW_CONTEXT_ALPHA_NOZ] = reinterpret_cast<unsigned int *>(data_start + 22);
    DrawContextData[DRAW_CONTEXT_SOLID_Z] = reinterpret_cast<unsigned int *>(data_start + 36);
    DrawContextData[DRAW_CONTEXT_ALPHA_Z] = reinterpret_cast<unsigned int *>(data_start + 50);
    DrawContextData[DRAW_CONTEXT_SPECIAL] = reinterpret_cast<unsigned int *>(data_start + 64);

    if (fast) {
        return;
    }

    int32 scissor_x = ScissorX;
    int32 scissor_y = ScissorY;
    int32 scissor_w = ScissorW;
    int32 scissor_h = ScissorH;
    int32 back_colour = GetBackgroundColour();
    int32 frame_address = FrameAddress;
    int32 frame_mode = FrameMode;
    int32 frame_write_mask = FrameWriteMask;
    int32 frame_width = FrameWidth;
    int32 frame_height = FrameHeight;
    int32 z_address = ZBufferAddress;
    int32 z_mode = ZBufferMode;
    int32 z_mask = ZBufferMask;
    int32 z_test = ZTest;
    int32 alpha_test = AlphaTest;
    int32 fba = FBA;
    float aspect_ratio_3dx = AspectRatioX;
    float aspect_ratio_3dy = AspectRatioY;
    float aspect_ratio_2dx = AspectRatio2DX;
    float aspect_ratio_2dy = AspectRatio2DY;
    int pixel_offset_type = PixelOffsetType;
    int32 pixel_offset_x;
    int32 pixel_offset_y;
    int32 offset_x;
    int32 offset_y;
    int32 shift_width_amount;
    int32 shift_height_amount;

    // Los render targets que dependen del frame buffer copian su direccion, formato y tamano del que se esta dibujando.
    if (RelativeToFrameBuffer) {
        eRenderTarget *frame_buffer;

        if (RelativeToFrameBuffer == 2) {
            frame_buffer = MainDraw1Context;
        } else {
            frame_buffer = MainDraw0Context;
        }

        frame_address = frame_buffer->FrameAddress;
        frame_mode = frame_buffer->FrameMode;
        frame_width = frame_buffer->FrameWidth;
        frame_height = frame_buffer->FrameHeight;
        z_address = frame_buffer->ZBufferAddress;
        z_mode = frame_buffer->ZBufferMode;
        aspect_ratio_3dx = frame_buffer->AspectRatioX;
        aspect_ratio_3dy = frame_buffer->AspectRatioY;
        aspect_ratio_2dx = frame_buffer->AspectRatio2DX;
        aspect_ratio_2dy = frame_buffer->AspectRatio2DY;
        pixel_offset_type = frame_buffer->PixelOffsetType;
    }

    FrameAddress = frame_address;
    FrameMode = frame_mode;
    FrameWidth = frame_width;
    FrameHeight = frame_height;
    ZBufferAddress = z_address;
    ZBufferMode = z_mode;
    AspectRatioX = aspect_ratio_3dx;
    AspectRatioY = aspect_ratio_3dy;
    AspectRatio2DX = aspect_ratio_2dx;
    AspectRatio2DY = aspect_ratio_2dy;
    PixelOffsetType = pixel_offset_type;

    ZMax = z_mode == PSMZ24 ? 16777215.0f : 65535.0f;

    pixel_offset_x = 0;
    pixel_offset_y = 0;
    if (pixel_offset_type == 1) {
        pixel_offset_x = quarter_offset_x;
        pixel_offset_y = quarter_offset_y;
    } else if (pixel_offset_type == 2) {
        pixel_offset_x = half_offset_x;
        pixel_offset_y = half_offset_y;
    }

    // El centro del espacio de coordenadas del GS (2048, 2048) cae en el centro del render target, o del scissor.
    if (CenterRelativeToFrame) {
        offset_x = (((4096 - frame_width) >> 1) << 4) + pixel_offset_x;
        offset_y = (((4096 - frame_height) >> 1) << 4) + pixel_offset_y;
    } else {
        offset_x = (((4096 - scissor_w) >> 1) << 4) + pixel_offset_x;
        offset_y = (((4096 - scissor_h) >> 1) << 4) + pixel_offset_y;
        offset_x -= scissor_x << 4;
        offset_y -= scissor_y << 4;
    }

    for (shift_width_amount = 0; (1 << shift_width_amount) < frame_width; shift_width_amount++) {
    }
    for (shift_height_amount = 0; (1 << shift_height_amount) < frame_height; shift_height_amount++) {
    }

    for (int tex_index = 0; tex_index < 2; tex_index++) {
        int alpha_bits = 10;
        int alpha_fix = 128;

        if (tex_index == 0) {
            if (GetID() == TARGET_PLAYER1_RVM) {
                alpha_bits = 68;
            } else if (GetID() >= TARGET_ENVRMAP0B && GetID() <= TARGET_ENVRMAP0D) {
                alpha_bits = 68;
                alpha_fix = 64;
            }
        } else {
            alpha_bits = 84;
        }

        TextureInfo *texture_info = GetTextureInfo(tex_index);
        TextureInfoPlatInfo *plat_info = texture_info->GetPlatInfo();

        const char *name = GetName();

        bMemCpy(texture_info->DebugName, name, sizeof(texture_info->DebugName));
        texture_info->DebugName[sizeof(texture_info->DebugName) - 1] = 0;
        texture_info->NameHash = bStringHash(name);
        texture_info->Width = frame_width;
        texture_info->Height = frame_height;
        texture_info->ImagePlacement = 0;
        texture_info->PalettePlacement = 0;
        texture_info->BaseImageSize = 0;
        texture_info->ImageSize = 0;
        texture_info->PaletteSize = 0;
        texture_info->ImageCompressionType = GetImageCompressionFromPSM(frame_mode);
        texture_info->PaletteCompressionType = 0;
        texture_info->NumPaletteEntries = 256;
        texture_info->NumMipMapLevels = 1;
        texture_info->ShiftWidth = shift_width_amount;
        texture_info->ShiftHeight = shift_height_amount;
        texture_info->ApplyAlphaSorting = 0;
        texture_info->AlphaUsageType = 2;
        texture_info->AlphaBlendType = 0;
        texture_info->RenderingOrder = 0;
        texture_info->TilableUV = 0;
        texture_info->Flags = 0;
        texture_info->ScrollType = 0;
        texture_info->SetOffsetS(0.0f);
        texture_info->SetOffsetT(0.0f);
        texture_info->SetScaleS(1.0f);
        texture_info->SetScaleT(1.0f);
        texture_info->pTexturePack = 0;
        texture_info->pImageParent = texture_info;
        texture_info->ImageData = 0;
        texture_info->PaletteData = 0;
        texture_info->ReferenceCount = 1;

        plat_info->PixelStorageMode = frame_mode;
        plat_info->ClutPixelStorageMode = PaletteMode;
        plat_info->TextureFx = 0;
        plat_info->MMag = 1;
        plat_info->MMin = 1;
        plat_info->MipMapK = 0;
        plat_info->MipMapL = 0;
        plat_info->ImageInfos[0].Width = frame_width;
        plat_info->ImageInfos[0].Height = frame_height;
        plat_info->ImageInfos[0].TBP = 0;
        plat_info->ImageInfos[0].TBW = frame_width >> 6;
        plat_info->ImageInfos[0].IsSwizzled = 0;
        plat_info->ImageVramBlocksRequired = 0;
        plat_info->PaletteVramBlocksRequired = 0;
        plat_info->AlphaBits = alpha_bits;
        plat_info->AlphaFix = alpha_fix;
        plat_info->pActiveTexture = 0;
        plat_info->PaletteVramAddress = PaletteAddress / 256;
        plat_info->ImageVramAddress = frame_address << 5;

        if (GetID() == TARGET_PLAYER1_RVM) {
            plat_info->Special = 15;
            texture_info->Flags |= 1;
        }
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(GetClearZBufferData());

        data->d[0] = MakeGifTag(7, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        if (ClearBackground) {
            data->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
        } else {
            data->d[0] = SCE_GS_SET_TEST(1, 0, 0, 2, 0, 0, 1, 1);
        }
        data->d[1] = SCE_GS_TEST_1;
        data++;

        data->d[0] = SCE_GS_SET_SCISSOR(0, frame_width - 1, 0, frame_height - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = PRIM_SPRITE;
        data->d[1] = SCE_GS_PRIM;
        data++;
        data->w[0] = back_colour;
        data->w[1] = 0x3F800000; // Q = 1.0f
        data->d[1] = SCE_GS_RGBAQ;
        data++;
        data->d[0] = SCE_GS_SET_XYZ(offset_x, offset_y, 0);
        data->d[1] = SCE_GS_XYZ2;
        data++;
        data->d[0] = SCE_GS_SET_XYZ(offset_x + (frame_width << 4), offset_y + (frame_height << 4), 0);
        data->d[1] = SCE_GS_XYZ2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, alpha_test, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(DrawContextData[DRAW_CONTEXT_SOLID_Z]);

        data->d[0] = MakeGifTag(13, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        data->d[0] = 0;
        data->d[1] = SCE_GS_TEXFLUSH;
        data++;
        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask);
        data->d[1] = SCE_GS_FRAME_1;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, z_mask);
        data->d[1] = SCE_GS_ZBUF_1;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_1;
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, alpha_test, 0x30, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_1;
        data++;

        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask | 0xFF000000);
        data->d[1] = SCE_GS_FRAME_2;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_2;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_2;
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_2;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_2;
        data++;
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(DrawContextData[DRAW_CONTEXT_ALPHA_Z]);

        data->d[0] = MakeGifTag(13, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        data->d[0] = 0;
        data->d[1] = SCE_GS_TEXFLUSH;
        data++;
        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask);
        data->d[1] = SCE_GS_FRAME_1;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, z_mask);
        data->d[1] = SCE_GS_ZBUF_1;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_1;
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, ATST_GEQUAL, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_1;
        data++;

        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask | 0xFF000000);
        data->d[1] = SCE_GS_FRAME_2;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_2;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_2;
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_2;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_2;
        data++;
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(DrawContextData[DRAW_CONTEXT_SOLID_NOZ]);

        data->d[0] = MakeGifTag(13, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        data->d[0] = 0;
        data->d[1] = SCE_GS_TEXFLUSH;
        data++;
        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask);
        data->d[1] = SCE_GS_FRAME_1;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_1;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_1;
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, alpha_test, 0x30, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_1;
        data++;

        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask | 0xFF000000);
        data->d[1] = SCE_GS_FRAME_2;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_2;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_2;
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_2;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_2;
        data++;
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(DrawContextData[DRAW_CONTEXT_ALPHA_NOZ]);

        data->d[0] = MakeGifTag(13, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        data->d[0] = 0;
        data->d[1] = SCE_GS_TEXFLUSH;
        data++;
        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask);
        data->d[1] = SCE_GS_FRAME_1;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_1;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_1;
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, ATST_GEQUAL, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_1;
        data++;

        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask | 0xFF000000);
        data->d[1] = SCE_GS_FRAME_2;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_2;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_2;
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_2;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_2;
        data++;
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(DrawContextData[DRAW_CONTEXT_SPECIAL]);

        data->d[0] = MakeGifTag(13, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        data->d[0] = 0;
        data->d[1] = SCE_GS_TEXFLUSH;
        data++;
        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask);
        data->d[1] = SCE_GS_FRAME_1;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, z_mask);
        data->d[1] = SCE_GS_ZBUF_1;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_1;
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, ATST_GEQUAL, 1, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_1;
        data++;

        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask | 0xFF000000);
        data->d[1] = SCE_GS_FRAME_2;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, z_mask);
        data->d[1] = SCE_GS_ZBUF_2;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_2;
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, ATST_GEQUAL, 1, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_2;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_2;
        data++;
    }
}

// La version anterior de UpdateGifTags, con los paquetes en el frame malloc: los XYOFFSET se apuntan en la cola de la
// etapa para parchearlos con el medio pixel del fotograma.
void eRenderTarget::UpdateGifTagsOLD(int half_offset_x, int half_offset_y, int quarter_offset_x, int quarter_offset_y,
                                     eRenderStageQueue *render_stage_queue, int fast) {
    u_long128 *data_start = static_cast<u_long128 *>(eFrameMalloc(78 * sizeof(u_long128)));

    ClearZBufferData = reinterpret_cast<unsigned int *>(data_start);
    DrawContextData[DRAW_CONTEXT_SOLID_NOZ] = reinterpret_cast<unsigned int *>(data_start + 8);
    DrawContextData[DRAW_CONTEXT_ALPHA_NOZ] = reinterpret_cast<unsigned int *>(data_start + 22);
    DrawContextData[DRAW_CONTEXT_SOLID_Z] = reinterpret_cast<unsigned int *>(data_start + 36);
    DrawContextData[DRAW_CONTEXT_ALPHA_Z] = reinterpret_cast<unsigned int *>(data_start + 50);
    DrawContextData[DRAW_CONTEXT_SPECIAL] = reinterpret_cast<unsigned int *>(data_start + 50);

    if (fast) {
        return;
    }

    // Solo el jugador y los frame buffers parchean su XYOFFSET al cambiar de medio pixel.
    switch (ID) {
    case TARGET_FLAYER:
    case TARGET_PLAYER1:
    case TARGET_PLAYER2:
    case TARGET_FRAMEBUFFER0:
    case TARGET_FRAMEBUFFER1:
        break;
    default:
        render_stage_queue = 0;
        break;
    }

    int32 scissor_x = ScissorX;
    int32 scissor_y = ScissorY;
    int32 scissor_w = ScissorW;
    int32 scissor_h = ScissorH;
    int32 back_colour = GetBackgroundColour();
    int32 frame_address = FrameAddress;
    int32 frame_mode = FrameMode;
    int32 frame_write_mask = FrameWriteMask;
    int32 frame_width = FrameWidth;
    int32 frame_height = FrameHeight;
    int32 z_address = ZBufferAddress;
    int32 z_mode = ZBufferMode;
    int32 z_mask = ZBufferMask;
    int32 z_test = ZTest;
    int32 alpha_test = AlphaTest;
    int32 fba = FBA;
    float aspect_ratio_3dx = AspectRatioX;
    float aspect_ratio_3dy = AspectRatioY;
    float aspect_ratio_2dx = AspectRatio2DX;
    float aspect_ratio_2dy = AspectRatio2DY;
    int pixel_offset_type = PixelOffsetType;
    int32 pixel_offset_x;
    int32 pixel_offset_y;
    int32 offset_x;
    int32 offset_y;
    int32 shift_width_amount;
    int32 shift_height_amount;

    // Los render targets que dependen del frame buffer copian su direccion, formato y tamano del que se esta dibujando.
    if (RelativeToFrameBuffer) {
        eRenderTarget *frame_buffer;

        if (RelativeToFrameBuffer == 2) {
            frame_buffer = MainDraw1Context;
        } else {
            frame_buffer = MainDraw0Context;
        }

        frame_address = frame_buffer->FrameAddress;
        frame_mode = frame_buffer->FrameMode;
        frame_width = frame_buffer->FrameWidth;
        frame_height = frame_buffer->FrameHeight;
        z_address = frame_buffer->ZBufferAddress;
        z_mode = frame_buffer->ZBufferMode;
        aspect_ratio_3dx = frame_buffer->AspectRatioX;
        aspect_ratio_3dy = frame_buffer->AspectRatioY;
        aspect_ratio_2dx = frame_buffer->AspectRatio2DX;
        aspect_ratio_2dy = frame_buffer->AspectRatio2DY;
        pixel_offset_type = frame_buffer->PixelOffsetType;
    }

    FrameAddress = frame_address;
    FrameMode = frame_mode;
    FrameWidth = frame_width;
    FrameHeight = frame_height;
    ZBufferAddress = z_address;
    ZBufferMode = z_mode;
    AspectRatioX = aspect_ratio_3dx;
    AspectRatioY = aspect_ratio_3dy;
    AspectRatio2DX = aspect_ratio_2dx;
    AspectRatio2DY = aspect_ratio_2dy;
    PixelOffsetType = pixel_offset_type;

    ZMax = z_mode == PSMZ24 ? 16777215.0f : 65535.0f;

    pixel_offset_x = 0;
    pixel_offset_y = 0;
    if (pixel_offset_type == 1) {
        pixel_offset_x = quarter_offset_x;
        pixel_offset_y = quarter_offset_y;
    } else if (pixel_offset_type == 2) {
        pixel_offset_x = half_offset_x;
        pixel_offset_y = half_offset_y;
    }

    // El centro del espacio de coordenadas del GS (2048, 2048) cae en el centro del render target, o del scissor.
    if (CenterRelativeToFrame) {
        offset_x = (((4096 - frame_width) >> 1) << 4) + pixel_offset_x;
        offset_y = (((4096 - frame_height) >> 1) << 4) + pixel_offset_y;
    } else {
        offset_x = (((4096 - scissor_w) >> 1) << 4) + pixel_offset_x;
        offset_y = (((4096 - scissor_h) >> 1) << 4) + pixel_offset_y;
        offset_x -= scissor_x << 4;
        offset_y -= scissor_y << 4;
    }

    for (shift_width_amount = 0; (1 << shift_width_amount) < frame_width; shift_width_amount++) {
    }
    for (shift_height_amount = 0; (1 << shift_height_amount) < frame_height; shift_height_amount++) {
    }

    for (int tex_index = 0; tex_index < 2; tex_index++) {
        int alpha_bits = 34;
        int alpha_fix = 128;

        if (tex_index == 0) {
            if (GetID() == TARGET_PLAYER1_RVM) {
                alpha_bits = 68;
            } else if (GetID() >= TARGET_ENVRMAP0B && GetID() <= TARGET_ENVRMAP0D) {
                alpha_bits = 68;
                alpha_fix = 64;
            }
        } else {
            alpha_bits = 84;
        }

        TextureInfo *texture_info = GetTextureInfo(tex_index);
        TextureInfoPlatInfo *plat_info = texture_info->GetPlatInfo();

        const char *name = GetName();

        bMemCpy(texture_info->DebugName, name, sizeof(texture_info->DebugName));
        texture_info->DebugName[sizeof(texture_info->DebugName) - 1] = 0;
        texture_info->NameHash = bStringHash(name);
        texture_info->Width = frame_width;
        texture_info->Height = frame_height;
        texture_info->ImagePlacement = 0;
        texture_info->PalettePlacement = 0;
        texture_info->BaseImageSize = 0;
        texture_info->ImageSize = 0;
        texture_info->PaletteSize = 0;
        texture_info->ImageCompressionType = GetImageCompressionFromPSM(frame_mode);
        texture_info->PaletteCompressionType = 0;
        texture_info->NumPaletteEntries = 256;
        texture_info->NumMipMapLevels = 1;
        texture_info->ShiftWidth = shift_width_amount;
        texture_info->ShiftHeight = shift_height_amount;
        texture_info->ApplyAlphaSorting = 0;
        texture_info->AlphaUsageType = 2;
        texture_info->AlphaBlendType = 0;
        texture_info->RenderingOrder = 0;
        texture_info->TilableUV = 0;
        texture_info->Flags = 0;
        texture_info->ScrollType = 0;
        texture_info->SetOffsetS(0.0f);
        texture_info->SetOffsetT(0.0f);
        texture_info->SetScaleS(1.0f);
        texture_info->SetScaleT(1.0f);
        texture_info->pTexturePack = 0;
        texture_info->pImageParent = texture_info;
        texture_info->ImageData = 0;
        texture_info->PaletteData = 0;
        texture_info->ReferenceCount = 1;

        plat_info->PixelStorageMode = frame_mode;
        plat_info->ClutPixelStorageMode = PaletteMode;
        plat_info->TextureFx = 0;
        plat_info->MMag = 1;
        plat_info->MMin = 1;
        plat_info->MipMapK = 0;
        plat_info->MipMapL = 0;
        plat_info->ImageInfos[0].Width = frame_width;
        plat_info->ImageInfos[0].Height = frame_height;
        plat_info->ImageInfos[0].TBP = 0;
        plat_info->ImageInfos[0].TBW = frame_width >> 6;
        plat_info->ImageInfos[0].IsSwizzled = 0;
        plat_info->ImageVramBlocksRequired = 0;
        plat_info->PaletteVramBlocksRequired = 0;
        plat_info->AlphaBits = alpha_bits;
        plat_info->AlphaFix = alpha_fix;
        plat_info->pActiveTexture = 0;
        plat_info->PaletteVramAddress = PaletteAddress / 256;
        plat_info->ImageVramAddress = frame_address << 5;
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(GetClearZBufferData());

        data->d[0] = MakeGifTag(7, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        if (ClearBackground) {
            data->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
        } else {
            data->d[0] = SCE_GS_SET_TEST(1, 0, 0, 2, 0, 0, 1, 1);
        }
        data->d[1] = SCE_GS_TEST_1;
        data++;

        data->d[0] = SCE_GS_SET_SCISSOR(0, frame_width - 1, 0, frame_height - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = PRIM_SPRITE;
        data->d[1] = SCE_GS_PRIM;
        data++;
        data->w[0] = back_colour;
        data->w[1] = 0x3F800000; // Q = 1.0f
        data->d[1] = SCE_GS_RGBAQ;
        data++;
        data->d[0] = SCE_GS_SET_XYZ(offset_x, offset_y, 0);
        data->d[1] = SCE_GS_XYZ2;
        data++;
        data->d[0] = SCE_GS_SET_XYZ(offset_x + (frame_width << 4), offset_y + (frame_height << 4), 0);
        data->d[1] = SCE_GS_XYZ2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, alpha_test, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(DrawContextData[DRAW_CONTEXT_SOLID_Z]);

        data->d[0] = MakeGifTag(13, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        data->d[0] = 0;
        data->d[1] = SCE_GS_TEXFLUSH;
        data++;
        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask);
        data->d[1] = SCE_GS_FRAME_1;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, z_mask);
        data->d[1] = SCE_GS_ZBUF_1;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_1;
        if (render_stage_queue) {
            render_stage_queue->AddPatchXYOffset(&data->d[0]);
        }
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, alpha_test, 0x30, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_1;
        data++;

        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask | 0xFF000000);
        data->d[1] = SCE_GS_FRAME_2;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_2;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_2;
        if (render_stage_queue) {
            render_stage_queue->AddPatchXYOffset(&data->d[0]);
        }
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_2;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_2;
        data++;
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(DrawContextData[DRAW_CONTEXT_ALPHA_Z]);

        data->d[0] = MakeGifTag(13, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        data->d[0] = 0;
        data->d[1] = SCE_GS_TEXFLUSH;
        data++;
        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask);
        data->d[1] = SCE_GS_FRAME_1;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, z_mask);
        data->d[1] = SCE_GS_ZBUF_1;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_1;
        if (render_stage_queue) {
            render_stage_queue->AddPatchXYOffset(&data->d[0]);
        }
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, ATST_GEQUAL, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_1;
        data++;

        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask | 0xFF000000);
        data->d[1] = SCE_GS_FRAME_2;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_2;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_2;
        if (render_stage_queue) {
            render_stage_queue->AddPatchXYOffset(&data->d[0]);
        }
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_2;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_2;
        data++;
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(DrawContextData[DRAW_CONTEXT_SOLID_NOZ]);

        data->d[0] = MakeGifTag(13, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        data->d[0] = 0;
        data->d[1] = SCE_GS_TEXFLUSH;
        data++;
        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask);
        data->d[1] = SCE_GS_FRAME_1;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_1;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_1;
        if (render_stage_queue) {
            render_stage_queue->AddPatchXYOffset(&data->d[0]);
        }
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, alpha_test, 0x30, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_1;
        data++;

        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask | 0xFF000000);
        data->d[1] = SCE_GS_FRAME_2;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_2;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_2;
        if (render_stage_queue) {
            render_stage_queue->AddPatchXYOffset(&data->d[0]);
        }
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_2;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_2;
        data++;
    }

    {
        DATA16 *data = reinterpret_cast<DATA16 *>(DrawContextData[DRAW_CONTEXT_ALPHA_NOZ]);

        data->d[0] = MakeGifTag(13, 1, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        data->d[0] = 0;
        data->d[1] = SCE_GS_TEXFLUSH;
        data++;
        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask);
        data->d[1] = SCE_GS_FRAME_1;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_1;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_1;
        if (render_stage_queue) {
            render_stage_queue->AddPatchXYOffset(&data->d[0]);
        }
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_1;
        data++;
        data->d[0] = SCE_GS_SET_TEST(1, ATST_GEQUAL, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_1;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_1;
        data++;

        data->d[0] = SCE_GS_SET_FRAME(frame_address, frame_width >> 6, frame_mode, frame_write_mask | 0xFF000000);
        data->d[1] = SCE_GS_FRAME_2;
        data++;
        data->d[0] = SCE_GS_SET_ZBUF(z_address, z_mode, 1);
        data->d[1] = SCE_GS_ZBUF_2;
        data++;
        data->d[0] = SCE_GS_SET_XYOFFSET(offset_x, offset_y);
        data->d[1] = SCE_GS_XYOFFSET_2;
        if (render_stage_queue) {
            render_stage_queue->AddPatchXYOffset(&data->d[0]);
        }
        data++;
        data->d[0] = SCE_GS_SET_SCISSOR(scissor_x, scissor_x + scissor_w - 1, scissor_y, scissor_y + scissor_h - 1);
        data->d[1] = SCE_GS_SCISSOR_2;
        data++;
        data->d[0] = SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, z_test);
        data->d[1] = SCE_GS_TEST_2;
        data++;
        data->d[0] = SCE_GS_SET_FBA(fba);
        data->d[1] = SCE_GS_FBA_2;
        data++;
    }
}

TextureInfoPlatInfo RenderTargetTextureInfoPlatInfos[24][2];

void SetScreenBuffers() {
    bMatrix4 *identity = eGetIdentityMatrix();
    int32 frame_width = ScreenWidth;
    int32 frame_height = ScreenHeight;
    float aspect_ratio_3dx;
    float aspect_ratio_3dy;
    float aspect_ratio_2dx;
    float aspect_ratio_2dy;

    bMemSet(RenderTargetTextureInfos, 0, sizeof(RenderTargetTextureInfos));
    bMemSet(RenderTargetTextureInfoPlatInfos, 0, sizeof(RenderTargetTextureInfoPlatInfos));

    for (int i = 0; i < 24; i++) {
        RenderTargetTextureInfos[i][0].SetPlatInfo(&RenderTargetTextureInfoPlatInfos[i][0]);
        RenderTargetTextureInfos[i][1].SetPlatInfo(&RenderTargetTextureInfoPlatInfos[i][1]);
    }

    if (GetVideoMode() == MODE_NTSC) {
        aspect_ratio_3dx = NTSCAspectRatio3dx;
        aspect_ratio_3dy = NTSCAspectRatio3dy;
        aspect_ratio_2dx = NTSCAspectRatio2dx;
        aspect_ratio_2dy = NTSCAspectRatio2dy;
    } else {
        aspect_ratio_3dx = PALAspectRatio3dx;
        aspect_ratio_3dy = PALAspectRatio3dy;
        aspect_ratio_2dx = PALAspectRatio2dx;
        aspect_ratio_2dy = PALAspectRatio2dy;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_FRAMEBUFFER0);

        rc->SetID(TARGET_FRAMEBUFFER0);
        rc->SetName("TARGET_FRAMEBUFFER0");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width;
        rc->ScissorH = frame_height;
        rc->FrameWidth = frame_width;
        rc->FrameHeight = frame_height;
        rc->FrameMode = PSMCT32;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_FRAMEBUFFER0, 0) / 8192;
        rc->ZBufferMode = PSMZ24;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_ZBUFFER, 0) / 8192;
        rc->ZBufferMask = 0;
        rc->ZTest = 2;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_FRAMEBUFFER1);

        rc->SetID(TARGET_FRAMEBUFFER1);
        rc->SetName("TARGET_FRAMEBUFFER1");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width;
        rc->ScissorH = frame_height;
        rc->FrameWidth = frame_width;
        rc->FrameHeight = frame_height;
        rc->FrameMode = PSMCT32;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_FRAMEBUFFER1, 0) / 8192;
        rc->ZBufferMode = PSMZ24;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_ZBUFFER, 0) / 8192;
        rc->ZBufferMask = 0;
        rc->ZTest = 2;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_FRAMEBUFFER2);

        rc->SetID(TARGET_FRAMEBUFFER2);
        rc->SetName("TARGET_FRAMEBUFFER2");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 1;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width;
        rc->ScissorH = frame_height;
        rc->FrameWidth = frame_width;
        rc->FrameHeight = frame_height;
        rc->FrameMode = PSMCT32;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_FRAMEBUFFER2, 2) / 8192;
        rc->ZBufferMode = PSMZ16S;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_ZBUFFER, 2) / 8192;
        rc->ZBufferMask = 0;
        rc->ZTest = 2;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_ZBUFFER);

        rc->SetID(TARGET_ZBUFFER);
        rc->SetName("TARGET_ZBUFFER");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width;
        rc->ScissorH = frame_height;
        rc->FrameWidth = frame_width;
        rc->FrameHeight = frame_height;
        rc->FrameMode = PSMCT32;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_ZBUFFER, 0) / 8192;
        rc->ZBufferMode = PSMZ24;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_ZBUFFER, 0) / 8192;
        rc->ZBufferMask = 0;
        rc->ZTest = 1;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_FLAYER);

        rc->SetID(TARGET_FLAYER);
        rc->SetName("TARGET_FLAYER");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 1;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width;
        rc->ScissorH = frame_height;
        rc->FrameWriteMask = 0;
        rc->ZBufferMask = 0;
        rc->ZTest = 2;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_PLAYER1);

        rc->SetID(TARGET_PLAYER1);
        rc->SetName("TARGET_PLAYER1");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 1;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width;
        rc->ScissorH = frame_height;
        rc->FrameWriteMask = 0;
        rc->ZBufferMask = 0;
        rc->ZTest = 2;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_PLAYER2);

        rc->SetID(TARGET_PLAYER2);
        rc->SetName("TARGET_PLAYER2");
        rc->SetActive(0);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 1;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 64;
        rc->ScissorY = frame_height - 128;
        rc->ScissorW = frame_width - 128;
        rc->ScissorH = 128;
        rc->FrameWriteMask = 0;
        rc->ZBufferMask = 0;
        rc->ZTest = 2;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_PLAYER1_RVM);

        rc->SetID(TARGET_PLAYER1_RVM);
        rc->SetName("TARGET_PLAYER1_RVM");
        rc->SetActive(0);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = 1.5f;
        rc->AspectRatioY = 1.08f;
        rc->AspectRatio2DX = 0.8f;
        rc->AspectRatio2DY = 0.94f;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = 256;
        rc->ScissorH = 64;
        rc->FrameWidth = 256;
        rc->FrameHeight = 64;
        rc->FrameMode = PSMCT32;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_RVM1_FRAME, 0) / 8192;
        rc->ZBufferMode = PSMZ24;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_RVM1_ZBUF, 0) / 8192;
        rc->ZBufferMask = 0;
        rc->ZTest = 2;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_PLAYER1_REFLECTION);

        rc->SetID(TARGET_PLAYER1_REFLECTION);
        rc->SetName("TARGET_PLAYER1_REFLECTION");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width / 2;
        rc->ScissorH = frame_height / 2;
        rc->FrameWidth = frame_width / 2;
        rc->FrameHeight = frame_height / 2;
        rc->FrameMode = PSMCT16S;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_REFLECTION, eVRAM.GetCurrentLayout()) / 8192;
        rc->ZBufferMode = PSMZ16S;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_REFLECTION_ZBUF, eVRAM.GetCurrentLayout()) / 8192;
        rc->ZBufferMask = 0;
        rc->ZTest = 2;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_PLAYER2_REFLECTION);

        rc->SetID(TARGET_PLAYER2_REFLECTION);
        rc->SetName("TARGET_PLAYER2_REFLECTION");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width / 2;
        rc->ScissorH = frame_height / 2;
        rc->FrameWidth = frame_width / 2;
        rc->FrameHeight = frame_height / 2;
        rc->FrameMode = PSMCT16S;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_REFLECTION, eVRAM.GetCurrentLayout()) / 8192;
        rc->ZBufferMode = PSMZ16S;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_REFLECTION_ZBUF, eVRAM.GetCurrentLayout()) / 8192;
        rc->ZBufferMask = 0;
        rc->ZTest = 2;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_PLAYER1_SPECULAR);

        rc->SetID(TARGET_PLAYER1_SPECULAR);
        rc->SetName("TARGET_PLAYER1_SPECULAR");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width / 4;
        rc->ScissorH = frame_height / 4;
        rc->FrameWidth = frame_width / 4;
        rc->FrameHeight = frame_height / 4;
        rc->FrameMode = PSMCT16S;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_SPECULAR, eVRAM.GetCurrentLayout()) / 8192;
        rc->ZBufferMode = PSMZ16S;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_REFLECTION_ZBUF, eVRAM.GetCurrentLayout()) / 8192;
        rc->ZBufferMask = 0;
        rc->ZTest = 1;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_PLAYER2_SPECULAR);

        rc->SetID(TARGET_PLAYER2_SPECULAR);
        rc->SetName("TARGET_PLAYER2_SPECULAR");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width / 4;
        rc->ScissorH = frame_height / 4;
        rc->FrameWidth = frame_width / 4;
        rc->FrameHeight = frame_height / 4;
        rc->FrameMode = PSMCT16S;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_SPECULAR, eVRAM.GetCurrentLayout()) / 8192;
        rc->ZBufferMode = PSMZ16S;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_REFLECTION_ZBUF, eVRAM.GetCurrentLayout()) / 8192;
        rc->ZBufferMask = 0;
        rc->ZTest = 1;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_QUARTER_24);

        rc->SetID(TARGET_QUARTER_24);
        rc->SetName("TARGET_QUARTER_24");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width / 2;
        rc->ScissorH = frame_height / 2;
        rc->FrameWidth = 256;
        rc->FrameHeight = 256;
        rc->FrameMode = PSMCT24;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_256_A, 1) / 8192;
        rc->ZBufferMode = PSMZ24;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_256_A, 1) / 8192;
        rc->ZBufferMask = 1;
        rc->ZTest = 1;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_QUARTER_32);

        rc->SetID(TARGET_QUARTER_32);
        rc->SetName("TARGET_QUARTER_32");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = 1.0f;
        rc->AspectRatio2DY = 1.0f;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = frame_width / 2;
        rc->ScissorH = frame_height / 2;
        rc->FrameWidth = 256;
        rc->FrameHeight = 256;
        rc->FrameMode = PSMCT32;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_256_A, 1) / 8192;
        rc->ZBufferMode = PSMZ24;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_256_A, 1) / 8192;
        rc->ZBufferMask = 1;
        rc->ZTest = 1;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_EIGTH_32);

        rc->SetID(TARGET_EIGTH_32);
        rc->SetName("TARGET_EIGTH_32");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = 128;
        rc->ScissorH = 128;
        rc->FrameWidth = 128;
        rc->FrameHeight = 128;
        rc->FrameMode = PSMCT32;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_128_A, 1) / 8192;
        rc->ZBufferMode = PSMZ24;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_128_A, 1) / 8192;
        rc->ZBufferMask = 1;
        rc->ZTest = 1;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_SIXTEENTH_32);

        rc->SetID(TARGET_SIXTEENTH_32);
        rc->SetName("TARGET_SIXTEENTH_32");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = 64;
        rc->ScissorH = 64;
        rc->FrameWidth = 64;
        rc->FrameHeight = 64;
        rc->FrameMode = PSMCT32;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_128_B, 1) / 8192;
        rc->ZBufferMode = PSMZ24;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_128_B, 1) / 8192;
        rc->ZBufferMask = 1;
        rc->ZTest = 1;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_THIRTYSECOND_32);

        rc->SetID(TARGET_THIRTYSECOND_32);
        rc->SetName("TARGET_THIRTYSECOND_32");
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 2;
        rc->AspectRatioX = aspect_ratio_3dx;
        rc->AspectRatioY = aspect_ratio_3dy;
        rc->AspectRatio2DX = aspect_ratio_2dx;
        rc->AspectRatio2DY = aspect_ratio_2dy;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = 32;
        rc->ScissorH = 32;
        rc->FrameWidth = 32;
        rc->FrameHeight = 32;
        rc->FrameMode = PSMCT32;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_128_C, 1) / 8192;
        rc->ZBufferMode = PSMZ24;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_128_C, 1) / 8192;
        rc->ZBufferMask = 1;
        rc->ZTest = 1;
        rc->dthe = 0;
        rc->AlphaTest = 5;
        rc->FBA = 0;
    }

    int zbuf_address = eVRAM.GetArea(VRAM_ENVRMAP_ZBUF_START, eVRAM.GetCurrentLayout());
    int frame_address = eVRAM.GetArea(VRAM_ENVRMAP_FRAME_START, eVRAM.GetCurrentLayout());

    for (int i = 0; i < 6; i++) {
        eRenderTarget *rc = eGetRenderTarget(i + TARGET_ENVRMAP0B);

        rc->SetID(i + TARGET_ENVRMAP0B);
        rc->SetName(EnvmapTargetNames[i]);
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 1;
        rc->PixelOffsetType = 0;
        rc->AspectRatioX = 1.0f;
        rc->AspectRatioY = 1.0f;
        rc->AspectRatio2DX = 0.8f;
        rc->AspectRatio2DY = 0.94f;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = 64;
        rc->ScissorH = 64;
        rc->FrameWidth = 64;
        rc->FrameHeight = 64;
        rc->FrameMode = PSMCT16;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = frame_address / 8192;
        rc->ZBufferMode = PSMZ16;
        rc->ZBufferAddress = zbuf_address / 8192;
        rc->ZBufferMask = 0;
        rc->ZTest = 2;
        rc->dthe = 0;
        rc->AlphaTest = 6;
        rc->FBA = 1;

        zbuf_address += 8192;
        frame_address += 8192;
    }

    {
        eRenderTarget *rc = eGetRenderTarget(TARGET_ENVMAP0_FULL);

        rc->SetID(TARGET_ENVMAP0_FULL);
        rc->SetName(EnvmapTargetNames[6]);
        rc->SetActive(1);
        rc->SetBackgroundColour(0x80000000);
        rc->RelativeToFrameBuffer = 0;
        rc->CenterRelativeToFrame = 0;
        rc->ClearBackground = 0;
        rc->PixelOffsetType = 0;
        rc->AspectRatioX = 1.0f;
        rc->AspectRatioY = 1.0f;
        rc->AspectRatio2DX = 0.8f;
        rc->AspectRatio2DY = 0.94f;
        rc->BackgroundColour[0] = 128;
        rc->BackgroundColour[1] = 0;
        rc->BackgroundColour[2] = 0;
        rc->BackgroundColour[3] = 128;
        rc->ScissorX = 0;
        rc->ScissorY = 0;
        rc->ScissorW = 256;
        rc->ScissorH = 256;
        rc->FrameWidth = 256;
        rc->FrameHeight = 256;
        rc->FrameMode = PSMCT16;
        rc->FrameWriteMask = 0;
        rc->FrameAddress = eVRAM.GetArea(VRAM_ENVMAP0_FULL, eVRAM.GetCurrentLayout()) / 8192;
        rc->ZBufferMode = PSMZ16;
        rc->ZBufferAddress = eVRAM.GetArea(VRAM_ENVRMAP_ZBUF_START, eVRAM.GetCurrentLayout()) / 8192;
        rc->ZBufferMask = 1;
        rc->ZTest = 1;
        rc->dthe = 0;
        rc->AlphaTest = 1;
        rc->FBA = 1;
    }

    for (int i = 0; i < 24; i++) {
        eRenderTarget *rc = eGetRenderTarget(i);

        rc->FovBias = 1.0f;
        rc->MatrixPack0 = &DefaultMatrixPack;
        rc->MatrixPack1 = &DefaultMatrixPack;
        rc->WorldScreen = identity;
        rc->WorldView = identity;
        rc->WorldClipGB = identity;
        rc->ClipGBScreen = identity;
        rc->VUClipGBScreen0 = identity;
        rc->VUClipGBScreen1 = identity;
        rc->VUClipGBScreen2 = identity;
    }
}

EVIEWMODE CurrentViewMode;
EVIEWMODE RenderingViewMode;
EVIEWMODE TweakerViewMode;

EVIEWMODE eGetCurrentViewMode() {
    return CurrentViewMode;
}

void eUpdateViewMode() {
    eView *eview_player1 = eGetView(1, false);
    eView *eview_player2 = eGetView(2, false);
    eView *eview_player1_rvm = eGetView(3, false);

    if (eview_player1->GetCameraMover() && eview_player2->GetCameraMover()) {
        CurrentViewMode = EVIEWMODE_TWOH;
    } else if (eview_player1->GetCameraMover() && eview_player1_rvm->GetCameraMover() &&
               !eview_player1->GetCameraMover()->OutsidePOV()) {
        CurrentViewMode = EVIEWMODE_ONE;
    } else if (eview_player1->GetCameraMover()) {
        CurrentViewMode = EVIEWMODE_ONE;
    } else {
        CurrentViewMode = EVIEWMODE_NONE;
    }
}

CameraMover *cmp;

void MaybeChangeViewMode() {
    float rat3dX = GetVideoMode() == MODE_NTSC ? NTSCAspectRatio3dx : PALAspectRatio3dx;
    float rat2dX = GetVideoMode() == MODE_NTSC ? NTSCAspectRatio2dx : PALAspectRatio2dx;

    if (FEDatabase) {
        if (FEDatabase->GetUserProfile(0)->GetOptions()->TheVideoSettings.WideScreen) {
            rat2dX = GetVideoMode() == MODE_NTSC ? NTSCAspectRatio2dxWide : PALAspectRatio2dxWide;
            rat3dX = GetVideoMode() == MODE_NTSC ? NTSCAspectRatio3dxWide : PALAspectRatio3dxWide;
            ClipGBVolumeScaleXDisp = ClipGBVolumeScaleXNWIDE;
        } else {
            rat2dX = GetVideoMode() == MODE_NTSC ? NTSCAspectRatio2dx : PALAspectRatio2dx;
            rat3dX = GetVideoMode() == MODE_NTSC ? NTSCAspectRatio3dx : PALAspectRatio3dx;
            ClipGBVolumeScaleXDisp = ClipGBVolumeScaleXNORM;
        }
    }

    eRenderTarget *rc = eGetRenderTarget(TARGET_FRAMEBUFFER0);

    rc->AspectRatio2DX = rat2dX;
    rc->AspectRatioX = rat3dX;
    rc->AspectRatioY = GetVideoMode() == MODE_NTSC ? NTSCAspectRatio3dy : PALAspectRatio3dy;

    rc = eGetRenderTarget(TARGET_FRAMEBUFFER1);

    rc->AspectRatio2DX = rat2dX;
    rc->AspectRatioX = rat3dX;
    rc->AspectRatioY = GetVideoMode() == MODE_NTSC ? NTSCAspectRatio3dy : PALAspectRatio3dy;

    eGetRenderTarget(TARGET_PLAYER1_REFLECTION)->AspectRatioX = rat3dX;
    eGetRenderTarget(TARGET_PLAYER2_REFLECTION)->AspectRatioX = rat3dX;

    if (TweakerViewMode != EVIEWMODE_NONE) {
        if (RenderingViewMode != TweakerViewMode) {
            eWaitUntilRenderingDone();
            RenderingViewMode = TweakerViewMode;
        }
    } else {
        if (RenderingViewMode != CurrentViewMode) {
            eWaitUntilRenderingDone();
            RenderingViewMode = CurrentViewMode;
        }
    }

    eView *viewp1 = eGetView(1, false);
    eView *viewp2 = eGetView(2, false);
    eView *viewr1 = eGetView(3, false);

    if (CurrentViewMode == EVIEWMODE_ONE) {
        cmp = viewp1->GetCameraMover();

        if (viewp1->GetCameraMover() && viewr1->GetCameraMover() && !viewp1->GetCameraMover()->OutsidePOV()) {
            CurrentViewMode = EVIEWMODE_ONE_RVM;
        }
    } else if (CurrentViewMode == EVIEWMODE_ONE_RVM) {
        if (!viewr1->GetCameraMover() || viewp1->GetCameraMover()->OutsidePOV()) {
            CurrentViewMode = EVIEWMODE_ONE;
        }
    }

    eRenderTarget *draw = MainDraw0Context;
    eView *viewrefp1 = eGetView(4, false);
    eView *viewrefp2 = eGetView(5, false);
    eView *viewspcp1 = eGetView(8, false);
    eView *viewspcp2 = eGetView(9, false);
    eRenderTarget *targetp1 = viewp1->GetRenderTarget0();
    eRenderTarget *targetp2 = viewp2->GetRenderTarget0();
    eRenderTarget *targetr1 = viewr1->GetRenderTarget0();
    eRenderTarget *targetrefp1 = viewrefp1->GetRenderTarget0();
    eRenderTarget *targetrefp2 = viewrefp2->GetRenderTarget0();
    eRenderTarget *targetspcp1 = viewspcp1->GetRenderTarget0();
    eRenderTarget *targetspcp2 = viewspcp2->GetRenderTarget0();
    int screen_width = draw->ScissorW;
    int screen_height = draw->ScissorH;

    switch (RenderingViewMode) {
    case EVIEWMODE_TWOH:
        targetp1->ScissorX = 0; targetp1->ScissorY = 0; targetp1->ScissorW = screen_width; targetp1->ScissorH = screen_height >> 1;
        targetp2->ScissorX = 0; targetp2->ScissorY = screen_height >> 1; targetp2->ScissorW = screen_width; targetp2->ScissorH = screen_height >> 1;
        targetr1->ScissorX = 0; targetr1->ScissorY = 0; targetr1->ScissorW = 0; targetr1->ScissorH = 0;
        targetp1->SetActive(1);
        viewp1->SetActive(1);
        targetp2->SetActive(1);
        viewp2->SetActive(1);
        targetr1->SetActive(0);
        viewr1->SetActive(0);

        targetrefp1->ScissorX = 0; targetrefp1->ScissorY = 0; targetrefp1->ScissorW = screen_width >> 1; targetrefp1->ScissorH = screen_height >> 2;
        targetrefp2->ScissorX = 0; targetrefp2->ScissorY = screen_height >> 2; targetrefp2->ScissorW = screen_width >> 1; targetrefp2->ScissorH = screen_height >> 2;
        targetspcp1->ScissorX = 0; targetspcp1->ScissorY = 0; targetspcp1->ScissorW = screen_width >> 2; targetspcp1->ScissorH = screen_height >> 3;
        targetspcp2->ScissorX = 0; targetspcp2->ScissorY = screen_height >> 3; targetspcp2->ScissorW = screen_width >> 2; targetspcp2->ScissorH = screen_height >> 3;
        targetrefp1->SetActive(1);
        viewrefp1->SetActive(1);
        targetrefp2->SetActive(1);
        viewrefp2->SetActive(1);
        targetspcp1->SetActive(1);
        viewspcp1->SetActive(1);
        targetspcp2->SetActive(1);
        viewspcp2->SetActive(1);
        break;
    case EVIEWMODE_TWOV:
        targetp1->ScissorX = 0; targetp1->ScissorY = 0; targetp1->ScissorW = screen_width >> 1; targetp1->ScissorH = screen_height;
        targetp2->ScissorX = screen_width >> 1; targetp2->ScissorY = 0; targetp2->ScissorW = screen_width >> 1; targetp2->ScissorH = screen_height;
        targetr1->ScissorX = 0; targetr1->ScissorY = 0; targetr1->ScissorW = 0; targetr1->ScissorH = 0;
        targetp1->SetActive(1);
        viewp1->SetActive(1);
        targetp2->SetActive(1);
        viewp2->SetActive(1);
        targetr1->SetActive(0);
        viewr1->SetActive(0);
        break;
    case EVIEWMODE_ONE_RVM:
        targetp1->ScissorX = 0; targetp1->ScissorY = 0; targetp1->ScissorW = screen_width; targetp1->ScissorH = screen_height;
        targetp2->ScissorX = 0; targetp2->ScissorY = 0; targetp2->ScissorW = 0; targetp2->ScissorH = 0;
        targetr1->ScissorX = 0; targetr1->ScissorY = 0; targetr1->ScissorW = 256; targetr1->ScissorH = 64;
        targetp1->SetActive(1);
        viewp1->SetActive(1);
        targetp2->SetActive(0);
        viewp2->SetActive(0);
        targetr1->SetActive(1);
        viewr1->SetActive(1);

        targetrefp1->ScissorX = 0; targetrefp1->ScissorY = 0; targetrefp1->ScissorW = screen_width >> 1; targetrefp1->ScissorH = screen_height >> 1;
        targetrefp2->ScissorX = 0; targetrefp2->ScissorY = screen_height >> 1; targetrefp2->ScissorW = screen_width >> 1; targetrefp2->ScissorH = screen_height >> 1;
        targetspcp1->ScissorX = 0; targetspcp1->ScissorY = 0; targetspcp1->ScissorW = screen_width >> 2; targetspcp1->ScissorH = screen_height >> 2;
        targetspcp2->ScissorX = 0; targetspcp2->ScissorY = screen_height >> 2; targetspcp2->ScissorW = screen_width >> 2; targetspcp2->ScissorH = screen_height >> 2;
        targetrefp1->SetActive(1);
        viewrefp1->SetActive(1);
        targetrefp2->SetActive(0);
        viewrefp2->SetActive(0);
        targetspcp1->SetActive(1);
        viewspcp1->SetActive(1);
        targetspcp2->SetActive(0);
        viewspcp2->SetActive(0);
        break;
    case EVIEWMODE_NONE:
    case EVIEWMODE_ONE:
        targetp1->ScissorX = 0; targetp1->ScissorY = 0; targetp1->ScissorW = screen_width; targetp1->ScissorH = screen_height;
        targetp2->ScissorX = 0; targetp2->ScissorY = 0; targetp2->ScissorW = 0; targetp2->ScissorH = 0;
        targetr1->ScissorX = 0; targetr1->ScissorY = 0; targetr1->ScissorW = 0; targetr1->ScissorH = 0;
        targetp1->SetActive(1);
        viewp1->SetActive(1);
        targetp2->SetActive(0);
        viewp2->SetActive(0);
        targetr1->SetActive(0);
        viewr1->SetActive(0);

        targetrefp1->ScissorX = 0; targetrefp1->ScissorY = 0; targetrefp1->ScissorW = screen_width >> 1; targetrefp1->ScissorH = screen_height >> 1;
        targetrefp2->ScissorX = 0; targetrefp2->ScissorY = screen_height >> 1; targetrefp2->ScissorW = screen_width >> 1; targetrefp2->ScissorH = screen_height >> 1;
        targetspcp1->ScissorX = 0; targetspcp1->ScissorY = 0; targetspcp1->ScissorW = screen_width >> 2; targetspcp1->ScissorH = screen_height >> 2;
        targetspcp2->ScissorX = 0; targetspcp2->ScissorY = screen_height >> 2; targetspcp2->ScissorW = screen_width >> 2; targetspcp2->ScissorH = screen_height >> 2;
        targetrefp1->SetActive(1);
        viewrefp1->SetActive(1);
        targetrefp2->SetActive(0);
        viewrefp2->SetActive(0);
        targetspcp1->SetActive(1);
        viewspcp1->SetActive(1);
        targetspcp2->SetActive(0);
        viewspcp2->SetActive(0);
        break;
    }
}

void eSwapScreenBuffers() {
    eRenderTarget *disp0 = MainDisp0Context;
    eRenderTarget *disp1 = MainDisp1Context;
    eRenderTarget *draw0 = MainDraw0Context;
    eRenderTarget *draw1 = MainDraw1Context;

    switch (DisplayBufferMode) {
    case EDISP_NONE:
        break;
    case EDISP_FULLHEIGHT:
        if (disp0 == &RenderTargets[TARGET_FRAMEBUFFER0]) {
            disp0 = &RenderTargets[TARGET_FRAMEBUFFER1];
            disp1 = &RenderTargets[TARGET_FRAMEBUFFER1];
            draw0 = &RenderTargets[TARGET_FRAMEBUFFER0];
            draw1 = &RenderTargets[TARGET_FRAMEBUFFER0];
        } else {
            disp0 = &RenderTargets[TARGET_FRAMEBUFFER0];
            disp1 = &RenderTargets[TARGET_FRAMEBUFFER0];
            draw0 = &RenderTargets[TARGET_FRAMEBUFFER1];
            draw1 = &RenderTargets[TARGET_FRAMEBUFFER1];
        }
        break;
    case EDISP_HALFHEIGHT_NEW:
        if (disp0 == &RenderTargets[TARGET_FRAMEBUFFER0]) {
            disp0 = &RenderTargets[TARGET_FRAMEBUFFER1];
            disp1 = &RenderTargets[TARGET_FRAMEBUFFER1];
            draw0 = &RenderTargets[TARGET_FRAMEBUFFER0];
            draw1 = &RenderTargets[TARGET_FRAMEBUFFER0];
        } else {
            disp0 = &RenderTargets[TARGET_FRAMEBUFFER0];
            disp1 = &RenderTargets[TARGET_FRAMEBUFFER0];
            draw0 = &RenderTargets[TARGET_FRAMEBUFFER1];
            draw1 = &RenderTargets[TARGET_FRAMEBUFFER1];
        }
        break;
    }

    MainDisp0Context = disp0;
    MainDisp1Context = disp1;
    MainDraw0Context = draw0;
    MainDraw1Context = draw1;
}

void ePutDisplayEnvironment() {
    eRenderTarget *disp0 = MainDisp0Context;
    eRenderTarget *disp1 = MainDisp1Context;

    uint32 fbp1 = disp0->FrameAddress;
    uint32 fbw1 = disp0->FrameWidth >> 6;
    uint32 psm1 = disp0->FrameMode;

    uint32 fbp2 = disp1->FrameAddress;
    uint32 fbw2 = disp1->FrameWidth >> 6;
    uint32 psm2 = disp1->FrameMode;

    int32 fmode = DisplayBufferMode != EDISP_FULLHEIGHT;

    int screen_offset_x;
    int screen_offset_y;

    eGetScreenDisplayOffsets(screen_offset_x, screen_offset_y);

    BufferParams *buffer_params = CurrentBufferParameters;
    int32 enb1 = buffer_params->enable1;
    int32 sdx1 = buffer_params->sdx1 + screen_offset_x;
    int32 sdy1 = buffer_params->sdy1 + screen_offset_y;
    int32 dbx1 = buffer_params->dbx1;
    int32 dby1 = buffer_params->dby1;
    int32 magh1 = buffer_params->magh1;
    int32 magv1 = buffer_params->magv1;
    int32 dw1 = buffer_params->dw1;
    int32 dh1 = ScreenHeight - 5;

    int32 enb2 = buffer_params->enable2;
    int32 sdx2 = buffer_params->sdx2 + screen_offset_x;
    int32 sdy2 = buffer_params->sdy2 + screen_offset_y;
    int32 dbx2 = buffer_params->dbx2;
    int32 dby2 = buffer_params->dby2;
    int32 magh2 = buffer_params->magh2;
    int32 magv2 = buffer_params->magv2;
    int32 dw2 = buffer_params->dw2;
    int32 dh2 = ScreenHeight - 5;

    int32 blend = buffer_params->blend;

    if (DisplayBufferMode == EDISP_NONE) {
        dh1 = 0;
        dh2 = 0;
    }

    u_long dispfb1 = SCE_GS_SET_DISPFB(fbp1, fbw1, psm1, dbx1, dby1);
    u_long dispfb2 = SCE_GS_SET_DISPFB(fbp2, fbw2, psm2, dbx2, dby2);
    u_long display1 = (u_long)sdx1 | ((u_long)sdy1 << 12) | ((u_long)magh1 << 23) | ((u_long)magv1 << 27) |
                      ((u_long)dw1 << 32) | ((u_long)dh1 << 44);
    u_long display2 = (u_long)sdx2 | ((u_long)sdy2 << 12) | ((u_long)magh2 << 23) | ((u_long)magv2 << 27) |
                      ((u_long)dw2 << 32) | ((u_long)dh2 << 44);

    *GS_PMODE = SCE_GS_SET_PMODE(enb1, enb2, 0, 1, 0, 0, blend);
    *GS_SMODE2 = SCE_GS_SET_SMODE2(1, fmode, 0);

    *GS_DISPFB1 = dispfb1;
    *GS_DISPLAY1 = display1;

    *GS_DISPFB2 = dispfb2;
    *GS_DISPLAY2 = display2;

    *GS_BGCOLOR = 0;
    CPU::SyncL();
}

void eForceBackgroundColour(unsigned char r, unsigned char g, unsigned char b, float time, const char *reason) {
    eWaitUntilRenderingDone();

    eFillScreenWithColour(r + (g << 8) + (b << 16) + (0x80 << 24));

    int start_tick = bGetTicker();
    while (bGetTickerDifference(start_tick, bGetTicker()) * 0.001f < time) {
    }
}

int AttachTestingSets;
int AttachCurrentReg;

void AttachTestSet(eDmaChain *dma_chain) {
    if (AttachTestingSets) {
        DATA16 *data_start = static_cast<DATA16 *>(eFrameMalloc(253 * sizeof(DATA16)));

        if (data_start) {
            data_start->d[0] = MakeDmaTagA(0, 252, eDMA_CNT);
            data_start->w[2] = SCE_VIF1_SET_NOP(0);
            data_start->w[3] = SCE_VIF1_SET_DIRECT(252, 0);

            data_start[1].d[0] = MakeGifTag(251, 1, 0, 0, GIF_PACKED, 1);
            data_start[1].d[1] = SCE_GIF_PACKED_AD;

            DATA16 *data = data_start + 2;

            for (int i = 0; i < 256; i++) {
                switch (i) {
                case GSREG_TEXA:
                case GSREG_LABEL:
                case GSREG_NOP:
                    break;
                case GSREG_TRXDIR:
                case GSREG_HWREG:
                case GSREG_SIGNAL:
                case GSREG_FINISH:
                case 224:
                    continue;
                }
                data->d[0] = 0;
                data->d[1] = i;
                data++;
            }

            dma_chain->Add(data_start);
        }
    }
}

// El entorno de dibujo del frame: los registros del GS que no cambian (clamp, dither, niebla, PABE...) y, para cada
// render target activo (mapas de entorno, retrovisor, reflejos y los dos de pantalla), su contexto de dibujo y el
// borrado del z-buffer.
void AttachDrawEnvironment(eDmaChain *dma_chain) {
    DATA16 *data_start = static_cast<DATA16 *>(eFrameMalloc(42 * sizeof(DATA16)));
    DATA16 *data = data_start;

    if (!data) {
        return;
    }

    data->d[0] = MakeDmaTagA(0, 13, eDMA_CNT);
    data->w[2] = SCE_VIF1_SET_NOP(0);
    data->w[3] = SCE_VIF1_SET_DIRECT(13, 0);
    data++;

    data->d[0] = MakeGifTag(12, 1, 0, 0, GIF_PACKED, 1);
    data->d[1] = SCE_GIF_PACKED_AD;
    data++;

    data->d[0] = SCE_GS_SET_CLAMP(1, 1, 0, 0, 0, 0);
    data->d[1] = SCE_GS_CLAMP_1;
    data++;

    data->d[0] = SCE_GS_SET_CLAMP(1, 1, 0, 0, 0, 0);
    data->d[1] = SCE_GS_CLAMP_2;
    data++;

    data->d[0] = SCE_GS_SET_COLCLAMP(1);
    data->d[1] = SCE_GS_COLCLAMP;
    data++;

    data->d[0] = SCE_GS_SET_DTHE(0);
    data->d[1] = SCE_GS_DTHE;
    data++;

    data->d[0] = SCE_GS_SET_FBA(0);
    data->d[1] = SCE_GS_FBA_1;
    data++;

    data->d[0] = SCE_GS_SET_FBA(0);
    data->d[1] = SCE_GS_FBA_2;
    data++;

    data->d[0] = SCE_GS_SET_FOGCOL(0xEB, 0xDF, 0x9D);
    data->d[1] = SCE_GS_FOGCOL;
    data++;

    data->d[0] = SCE_GS_SET_FOG(0xFF);
    data->d[1] = SCE_GS_FOG;
    data++;

    data->d[0] = SCE_GS_SET_PABE(0);
    data->d[1] = SCE_GS_PABE;
    data++;

    data->d[0] = SCE_GS_SET_PRMODECONT(1);
    data->d[1] = SCE_GS_PRMODECONT;
    data++;

    data->d[0] = SCE_GS_SET_SCANMSK(0);
    data->d[1] = SCE_GS_SCANMSK;
    data++;

    data->d[0] = SCE_GS_SET_TEXA(0, 1, 0x80);
    data->d[1] = SCE_GS_TEXA;
    data++;

    if (EnvironmentMappingEnable && TheGameFlowManager.IsInGame()) {
        for (int i = 0; i < 6; i++) {
            if (eGetView(EVIEW_ENVMAP0F + i, false)->IsActive() && eGetView(EVIEW_ENVMAP0F + i, false)->GetRenderTarget()->IsActive()) {
                eRenderTarget *render_target = eGetView(EVIEW_ENVMAP0F + i, false)->GetRenderTarget();

                data->d[0] = MakeDmaTagA(render_target->GetDrawContextData(DRAW_CONTEXT_WRITE_Z), 14, eDMA_REF);
                data->w[2] = SCE_VIF1_SET_NOP(0);
                data->w[3] = SCE_VIF1_SET_DIRECT(14, 0);
                data++;

                data->d[0] = MakeDmaTagA(render_target->GetClearZBufferData(), 8, eDMA_REF);
                data->w[2] = SCE_VIF1_SET_NOP(0);
                data->w[3] = SCE_VIF1_SET_DIRECT(8, 0);
                data++;
            }
        }
    }

    for (int view_id = EVIEW_FIRST_RVM; view_id <= EVIEW_LAST_RVM; view_id++) {
        eView *view = eGetView(view_id, false);

        if (view->IsActive() && view->GetRenderTarget()->IsActive()) {
            eRenderTarget *render_target = view->GetRenderTarget();

            data->d[0] = MakeDmaTagA(render_target->GetDrawContextData(DRAW_CONTEXT_WRITE_Z), 14, eDMA_REF);
            data->w[2] = SCE_VIF1_SET_NOP(0);
            data->w[3] = SCE_VIF1_SET_DIRECT(14, 0);
            data++;

            data->d[0] = MakeDmaTagA(render_target->GetClearZBufferData(), 8, eDMA_REF);
            data->w[2] = SCE_VIF1_SET_NOP(0);
            data->w[3] = SCE_VIF1_SET_DIRECT(8, 0);
            data++;
        }
    }

    for (int view_id = EVIEW_PLAYER1_REFLECTION; view_id <= EVIEW_PLAYER2_REFLECTION; view_id++) {
        eView *view = eGetView(view_id, false);

        if (view->IsActive() && view->GetRenderTarget()->IsActive()) {
            eRenderTarget *render_target = view->GetRenderTarget();

            data->d[0] = MakeDmaTagA(render_target->GetDrawContextData(DRAW_CONTEXT_WRITE_Z), 14, eDMA_REF);
            data->w[2] = SCE_VIF1_SET_NOP(0);
            data->w[3] = SCE_VIF1_SET_DIRECT(14, 0);
            data++;

            data->d[0] = MakeDmaTagA(render_target->GetClearZBufferData(), 8, eDMA_REF);
            data->w[2] = SCE_VIF1_SET_NOP(0);
            data->w[3] = SCE_VIF1_SET_DIRECT(8, 0);
            data++;
        }
    }

    eRenderTarget *draw0 = MainDraw0Context;
    eRenderTarget *draw1 = MainDraw1Context;

    data->d[0] = MakeDmaTagA(draw0->GetDrawContextData(DRAW_CONTEXT_WRITE_Z), 14, eDMA_REF);
    data->w[2] = SCE_VIF1_SET_NOP(0);
    data->w[3] = SCE_VIF1_SET_DIRECT(14, 0);
    data++;

    data->d[0] = MakeDmaTagA(draw0->GetClearZBufferData(), 8, eDMA_REF);
    data->w[2] = SCE_VIF1_SET_NOP(0);
    data->w[3] = SCE_VIF1_SET_DIRECT(8, 0);
    data++;

    if (draw0 != draw1) {
        data->d[0] = MakeDmaTagA(draw1->GetDrawContextData(DRAW_CONTEXT_WRITE_Z), 14, eDMA_REF);
        data->w[2] = SCE_VIF1_SET_NOP(0);
        data->w[3] = SCE_VIF1_SET_DIRECT(14, 0);
        data++;

        data->d[0] = MakeDmaTagA(draw1->GetClearZBufferData(), 8, eDMA_REF);
        data->w[2] = SCE_VIF1_SET_NOP(0);
        data->w[3] = SCE_VIF1_SET_DIRECT(8, 0);
        data++;
    }

    data->d[0] = MakeDmaTagA(0, 0, eDMA_NEXT);
    data->w[2] = SCE_VIF1_SET_NOP(0);
    data->w[3] = SCE_VIF1_SET_NOP(0);

    dma_chain->Add(data_start, data);
}

void AttachVUdata(eDmaChain *dma_chain, uint32 *what, uint32 where, uint32 qwords) {
    DATA16 *data_start = static_cast<DATA16 *>(eFrameMalloc(2 * sizeof(DATA16)));

    if (data_start) {
        data_start->d[0] = MakeDmaTag(qwords, eDMA_REF, what);
        data_start->w[2] = SCE_VIF1_SET_FLUSHE(0);
        data_start->w[3] = VIF_CODE(UNPACK | V4_32, qwords, where);

        DATA16 *data = data_start + 1;

        data->d[0] = MakeDmaTag(0, eDMA_NEXT);
        data->w[2] = SCE_VIF1_SET_NOP(0);
        data->w[3] = SCE_VIF1_SET_NOP(0);

        dma_chain->Add(data_start, data);
    }
}

void AttachZbufClear(eDmaChain *dma_chain, eView *view) {
    DATA16 *data_start = static_cast<DATA16 *>(eFrameMalloc(4 * sizeof(DATA16)));
    eRenderTarget *render_target = view->GetRenderTarget();

    if (data_start) {
        data_start->d[0] = MakeDmaTagA(render_target->GetDrawContextData(2), 14, eDMA_REF);
        data_start->w[2] = SCE_VIF1_SET_FLUSHE(0);
        data_start->w[3] = SCE_VIF1_SET_DIRECT(14, 0);

        DATA16 *data = data_start + 1;

        data->d[0] = MakeDmaTag(8, eDMA_REF, render_target->GetClearZBufferData());
        data->w[2] = SCE_VIF1_SET_NOP(0);
        data->w[3] = SCE_VIF1_SET_DIRECT(8, 0);

        data = data_start + 2;

        data->d[0] = MakeDmaTag(0, eDMA_NEXT);
        data->w[2] = SCE_VIF1_SET_NOP(0);
        data->w[3] = SCE_VIF1_SET_NOP(0);

        dma_chain->Add(data_start, data);
    }
}

void AttachImageDataPacket(eDmaChain *dma_chain, void *image_data, uint32 vram_address, uint32 qwsize, uint32 psm, uint32 tbw,
                           uint32 width, uint32 height, int eop) {
    DATA16 *data_start = static_cast<DATA16 *>(eFrameMalloc(10 * sizeof(DATA16)));
    DATA16 *data = data_start;

    if (data) {
        data->d[0] = MakeDmaTag(6, eDMA_CNT);
        data->w[2] = SCE_VIF1_SET_FLUSHE(0);
        data->w[3] = Path3TextureTransferEnabled ? 0 : SCE_VIF1_SET_DIRECT(6, 0);
        data++;

        data->d[0] = MakeGifTag(4, 0, 0, 0, GIF_PACKED, 1);
        data->d[1] = SCE_GIF_PACKED_AD;
        data++;

        data->d[0] = SCE_GS_SET_BITBLTBUF(0, 0, 0, vram_address, tbw, psm);
        data->d[1] = SCE_GS_BITBLTBUF;
        data++;

        data->d[0] = SCE_GS_SET_TRXPOS(0, 0, 0, 0, 0);
        data->d[1] = SCE_GS_TRXPOS;
        data++;

        data->d[0] = SCE_GS_SET_TRXREG(width, height);
        data->d[1] = SCE_GS_TRXREG;
        data++;

        data->d[0] = SCE_GS_SET_TRXDIR(0);
        data->d[1] = SCE_GS_TRXDIR;
        data++;

        data->d[0] = MakeGifTag(qwsize, eop, 0, 0, GIF_IMAGE, 0);
        data->d[1] = 0;
        data++;

        data->d[0] = MakeDmaTag(qwsize, eDMA_REF, image_data);
        data->w[2] = SCE_VIF1_SET_NOP(0);
        data->w[3] = Path3TextureTransferEnabled ? 0 : SCE_VIF1_SET_DIRECT(qwsize, 0);
        data++;

        data->d[0] = MakeDmaTag(0, eDMA_NEXT);
        data->w[2] = SCE_VIF1_SET_NOP(0);
        data->w[3] = SCE_VIF1_SET_NOP(0);

        dma_chain->Add(data_start, data);
    }
}

void rAttachTextureDownloadPacket(eDmaChain *dma_chain, TextureInfo *texture_info, unsigned short image_download_address,
                                  unsigned short palette_download_address) {
    TextureInfoPlatInfo *plat_info = texture_info->GetPlatInfo();

    if (palette_download_address && texture_info->GetPaletteData()) {
        uint32 qwsize = texture_info->PaletteSize / 16;
        uint32 cpsm = plat_info->ClutPixelStorageMode;
        uint32 width;
        uint32 height;

        if (texture_info->NumPaletteEntries == 256) {
            width = 16;
            height = 16;
        } else {
            width = 8;
            height = 2;
        }
        AttachImageDataPacket(dma_chain, texture_info->GetPaletteData(), palette_download_address, qwsize, cpsm, 1, width,
                              height, 0);
    }

    void *image_data;

    if (image_download_address && (image_data = texture_info->GetImageData())) {
        uint32 qwsize = texture_info->ImageSize / 16;
        uint32 width = texture_info->Width;
        uint32 height = texture_info->Height;
        uint32 psm = plat_info->PixelStorageMode;
        uint32 tbw = plat_info->ImageInfos[0].TBW;
        int dma_swizzled = plat_info->ImageInfos[0].IsSwizzled;

        if (dma_swizzled) {
            if (texture_info->ImageCompressionType == TEXCOMP_4BIT) {
                width >>= 1;
                height >>= 1;
                tbw = GetPS2TextureBufferWidth(width, 16);
                psm = PSMCT16;
            } else {
                width >>= 1;
                height >>= 1;
                tbw = GetPS2TextureBufferWidth(width, 32);
                psm = PSMCT32;
            }
            AttachImageDataPacket(dma_chain, image_data, image_download_address, qwsize, psm, tbw, width, height, 0);
        } else {
            AttachImageDataPacket(dma_chain, image_data, image_download_address, qwsize, psm, tbw, width, height, 0);
        }
    }
}

bool bClampOverRide;
long nClampOverRide;

int eClampTopLeft(bool bOnOff, int nUnused) {
    bClampOverRide = bOnOff;
    nClampOverRide = SCE_GS_SET_CLAMP(2, 2, 0, 1, 0, 1);

    return 0;
}

// El contexto de textura de un TextureInfo: un paquete A+D con TEX0, TEX1, ALPHA, MIPTBP1/2 y CLAMP, el desplazamiento
// y la escala de las UV, el uso del alfa y las llamadas al microcodigo de la VU1. Se guarda en la textura activa para
// no rehacerlo en el mismo frame.
uint32 *CreateTextureContext(TextureInfo *texture_info, int TCC) {
    TextureInfoPlatInfo *plat_info = texture_info->GetPlatInfo();
    eActiveTexture *active_texture = static_cast<eActiveTexture *>(plat_info->pActiveTexture);

    if (active_texture && active_texture->pPreviousDmaData) {
        return active_texture->pPreviousDmaData;
    }

    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(176));

    if (!data_start) {
        return nullptr;
    }

    if (active_texture) {
        active_texture->pPreviousDmaData = data_start;
    }

    int mmin = plat_info->MMin;
    int mmag = plat_info->MMag;
    int l = plat_info->MipMapL;
    int k = plat_info->MipMapK;

    float offset_s = 0.0f;
    float offset_t = 0.0f;

    if (texture_info->ScrollType) {
        float world_time = WorldTimer.GetSeconds();

        if (texture_info->ScrollType == TEXSCROLL_OFFSETSCALE) {
            offset_s = -texture_info->GetOffsetS();
            offset_t = -texture_info->GetOffsetT();
        } else if (TexAnimScrollSpeedS != 0.0f || TexAnimScrollSpeedT != 0.0f) {
            float speed_s = (TexAnimScrollSpeedS / texture_info->Width) * (TexAnimOriginalTextureWidth / texture_info->Width);
            float speed_t = (TexAnimScrollSpeedT / texture_info->Height) * (TexAnimOriginalTextureHeight / texture_info->Height);

            offset_s = texture_info->GetScroll(world_time, speed_s, TexAnimScrollType, TexAnimScrollTimeStep);
            offset_t = texture_info->GetScroll(world_time, speed_t, TexAnimScrollType, TexAnimScrollTimeStep);
        } else {
            offset_s = texture_info->GetScrollS(world_time);
            offset_t = texture_info->GetScrollT(world_time);
        }
    }

    int tbp = texture_info->pImageParent->GetPlatInfo()->ImageVramAddress;
    int tilableu = !(texture_info->TilableUV & 1);
    int tilablev = !(texture_info->TilableUV & 2);
    u_long alpha_bits = (static_cast<u_long>(plat_info->AlphaFix) << 32) | plat_info->AlphaBits;
    u_long tex0 = SCE_GS_SET_TEX0(tbp, plat_info->ImageInfos[0].TBW, plat_info->PixelStorageMode, texture_info->ShiftWidth,
                                  texture_info->ShiftHeight, TCC, plat_info->TextureFx, plat_info->PaletteVramAddress,
                                  plat_info->ClutPixelStorageMode, 0, 0, 2);
    u_long tex1 = SCE_GS_SET_TEX1(0, texture_info->NumMipMapLevels - 1, mmag, mmin, 0, l, k);
    u_long mip1 = SCE_GS_SET_MIPTBP1(tbp + plat_info->ImageInfos[1].TBP, plat_info->ImageInfos[1].TBW,
                                     tbp + plat_info->ImageInfos[2].TBP, plat_info->ImageInfos[2].TBW,
                                     tbp + plat_info->ImageInfos[3].TBP, plat_info->ImageInfos[3].TBW);
    u_long mip2 = SCE_GS_SET_MIPTBP2(tbp + plat_info->ImageInfos[4].TBP, plat_info->ImageInfos[4].TBW,
                                     tbp + plat_info->ImageInfos[5].TBP, plat_info->ImageInfos[5].TBW,
                                     tbp + plat_info->ImageInfos[6].TBP, plat_info->ImageInfos[6].TBW);
    u_long clamp = bClampOverRide ? nClampOverRide : SCE_GS_SET_CLAMP(tilableu, tilablev, 0, 0, 0, 0);

    uint32 scale_s = FloatAsInteger(texture_info->GetScaleS());
    uint32 scale_t = FloatAsInteger(texture_info->GetScaleT());
    int alpha_usage_type = texture_info->AlphaUsageType;

    reinterpret_cast<u_long *>(data_start)[0] = MakeGifTag(6, 1, 0, 0, GIF_PACKED, 1);
    reinterpret_cast<u_long *>(data_start)[1] = SCE_GIF_PACKED_AD;
    reinterpret_cast<u_long *>(data_start)[2] = tex0;
    reinterpret_cast<u_long *>(data_start)[3] = SCE_GS_TEX0_1;
    reinterpret_cast<u_long *>(data_start)[4] = tex1;
    reinterpret_cast<u_long *>(data_start)[5] = SCE_GS_TEX1_1;
    reinterpret_cast<u_long *>(data_start)[6] = alpha_bits;
    reinterpret_cast<u_long *>(data_start)[7] = SCE_GS_ALPHA_1;
    reinterpret_cast<u_long *>(data_start)[8] = mip1;
    reinterpret_cast<u_long *>(data_start)[9] = SCE_GS_MIPTBP1_1;
    reinterpret_cast<u_long *>(data_start)[10] = mip2;
    reinterpret_cast<u_long *>(data_start)[11] = SCE_GS_MIPTBP2_1;
    reinterpret_cast<u_long *>(data_start)[12] = clamp;
    reinterpret_cast<u_long *>(data_start)[13] = SCE_GS_CLAMP_1;

    data_start[28] = FloatAsInteger(offset_s);
    data_start[29] = FloatAsInteger(offset_t);
    data_start[30] = FloatAsInteger(offset_t);
    data_start[31] = FloatAsInteger(offset_s);

    data_start[32] = scale_s;
    data_start[33] = scale_t;
    data_start[34] = scale_t;
    data_start[35] = scale_s;

    data_start[36] = alpha_usage_type;
    data_start[37] = texture_info->ApplyAlphaSorting;
    data_start[38] = texture_info->ApplyAlphaSorting;
    data_start[39] = alpha_usage_type;

    data_start[40] = 0;
    data_start[41] = 0;
    data_start[42] = VIF_CODE(0x14, 0, reinterpret_cast<uint32>(vuSetTextureEnv) >> 3);
    data_start[43] = VIF_CODE(0x14, 0, reinterpret_cast<uint32>(vuDoNothing) >> 3);

    return data_start;
}

int AttachColourModulation(eDmaChain *dma_chain, float r, float g, float b, float a) {
    float *data_start = static_cast<float *>(eFrameMalloc(4 * sizeof(float)));

    data_start[0] = r;
    data_start[1] = g;
    data_start[2] = b;
    data_start[3] = a;

    AttachVUdata(dma_chain, reinterpret_cast<uint32 *>(data_start), 640, 1);
}

int AttachTextureEnvDirect(eDmaChain *dma_chain, TextureInfo *texture_info, int TCC) {
    uint32 *texture_context = CreateTextureContext(texture_info, TCC);

    if (!texture_context) {
        return 0;
    }

    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(2 * sizeof(u_long128)));

    if (!data_start) {
        return 0;
    }

    *reinterpret_cast<u_long *>(data_start) = MakeDmaTag(11, eDMA_REF, texture_context);
    data_start[2] = SCE_VIF1_SET_FLUSHE(0);
    data_start[3] = VIF_CODE(UNPACK | V4_32, 10, 739);

    *reinterpret_cast<u_long *>(data_start + 4) = MakeDmaTag(0, eDMA_CNT);
    data_start[6] = SCE_VIF1_SET_NOP(0);
    data_start[7] = SCE_VIF1_SET_NOP(0);

    dma_chain->Add(data_start, data_start + 4);

    return 1;
}

eTextureDownloader TextureDownloader;

void eInitTextureDownloader() {
    TextureDownloader.GlobalInit(DownloadVramStartAddrees / 256, 16384);
}

u_long128 Path3GateVifCmdBuffer[55];
u_long128 *Path3GateBuffer;

void eTextureDownloader::Print(const char *text) {}

void eTextureDownloader::GlobalInit(int vram_block_start, int vram_block_end) {
    Init(vram_block_start, vram_block_end);

    StaticTextureSlotPool = bNewSlotPool(sizeof(eStaticTexture), 10, "StaticTextureSlotPool", 0);
    StaticTextureSlotPool->ClearFlag(SLOTPOOL_FLAG_ZERO_ALLOCATED_MEMORY);

    uint32 *gate_buffer = static_cast<uint32 *>(AlignPointer(Path3GateVifCmdBuffer, 128));
    Path3GateBuffer = reinterpret_cast<u_long128 *>(gate_buffer);

    bMemSet(Path3GateVifCmdBuffer, 0, sizeof(Path3GateVifCmdBuffer));

    gate_buffer[0] = SCE_VIF1_SET_MSKPATH3(0, 0);
    gate_buffer[163] = SCE_VIF1_SET_MSKPATH3(0x8000, 0);
}

void eTextureDownloader::Init(int vram_block_start, int vram_block_end) {
    LockedVramBlockStart = vram_block_end;
    LockedVramBlockEnd = vram_block_end;

    VramBlockStart = vram_block_start;
    VramBlockTop = vram_block_end;

    oldVramBlockStart = vram_block_start;
    oldVramBlockTop = vram_block_end;

    VramBlockBoundary = vram_block_end;
    DownloadingPhase = 0;

    SyncRequired = 0;
}

void eTextureDownloader::setStartEnd(int vram_block_start, int vram_block_end) {
    if (vram_block_start == 0) {
        VramBlockStart = oldVramBlockStart;
    } else {
        VramBlockStart = vram_block_start;
    }

    if (vram_block_end == 0) {
        VramBlockTop = oldVramBlockTop;
    } else {
        VramBlockTop = vram_block_end;
    }

    VramBlockBoundary = VramBlockTop;
}

void eTextureDownloader::Close() {
    while (!StaticTextureList.IsEmpty()) {
        eStaticTexture *static_texture = StaticTextureList.RemoveHead();

        bFree(StaticTextureSlotPool, static_texture);
    }

    PrevNumTextures = NumTextures;
    PrevNumPalettes = NumPalettes;
    PrevNumTextureBytes = NumTextureBytes;
    PrevNumPaletteBytes = NumPaletteBytes;
    PrevNumUnalignedTextures = NumUnalignedTextures;
    PrevLess8k = Less8k;
    PrevNumSwizzled = NumSwizzled;
    PrevNumSyncs = NumSyncs;
    PrevNumBatches = NumBatches;

    NumTextures = 0;
    NumPalettes = 0;
    NumTextureBytes = 0;
    NumPaletteBytes = 0;
    NumUnalignedTextures = 0;
    Less8k = 0;
    NumSwizzled = 0;
    NumSyncs = 0;
    NumBatches = 0;
}

void eTextureDownloader::AddTexturesUnsorted(bTList<eActiveTexture> *active_texture_list) {
    DownloadingTextureList.AddTail(active_texture_list);
}

// Reparte las texturas en ocho listas por su RenderingOrder y las pasa en ese orden a la lista de bajada.
void eTextureDownloader::AddTexturesSorted(bTList<eActiveTexture> *active_texture_list) {
    bTList<eActiveTexture> sorted_textures_list[8];

    while (!active_texture_list->IsEmpty()) {
        eActiveTexture *active_texture = active_texture_list->RemoveHead();
        TextureInfo *texture_info = active_texture->pTextureInfo;

        sorted_textures_list[texture_info->RenderingOrder].AddTail(active_texture);
    }
    for (int n = 0; n < 8; n++) {
        DownloadingTextureList.AddTail(&sorted_textures_list[n]);
    }
}

void eTextureDownloader::DeleteDownloadedTextures(eActiveTexture *last_active_texture) {
    if (!last_active_texture) {
        last_active_texture = DownloadingTextureList.EndOfList();
    }

    while (GetFirstTexture() != last_active_texture) {
        eActiveTexture *active_texture = DownloadingTextureList.RemoveHead();
        TextureInfo *texture_info = active_texture->pTextureInfo;
        TextureInfo *parent_texture_info = texture_info->pImageParent;
        TextureInfoPlatInfo *plat_info = texture_info->GetPlatInfo();
        TextureInfoPlatInfo *parent_plat_info = parent_texture_info->GetPlatInfo();

        if (plat_info->ImageVramBlocksRequired != 0) {
            parent_plat_info->ImageVramAddress = 0;
        }

        delete active_texture;
    }
}

void eTextureDownloader::AddStaticTextureForDownload(TextureInfo *texture_info, unsigned short image_download_address,
                                                     unsigned short palette_download_address) {
    if (StaticTextureSlotPool && texture_info) {
        eStaticTexture *static_texture = static_cast<eStaticTexture *>(bOMalloc(StaticTextureSlotPool));

        if (static_texture) {
            static_texture->Texture = texture_info;
            static_texture->ImageDownloadAddress = image_download_address;
            static_texture->PaletteDownloadAddress = palette_download_address;
            StaticTextureList.AddTail(static_texture);
        }
    }
}

int eTextureDownloader::AddPath3GateDmaTag(eDmaChain *dma_chain, eDmaChain *tex_chain) {
    if (!Path3TextureTransferEnabled) {
        return 1;
    }

    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(96));

    if (!data_start) {
        return 0;
    }

    *reinterpret_cast<u_long *>(data_start) = MakeDmaTag(2, eDMA_CNT);
    data_start[2] = SCE_VIF1_SET_FLUSHE(0);
    data_start[3] = SCE_VIF1_SET_DIRECT(2, 0);

    *reinterpret_cast<u_long *>(data_start + 4) = SCE_GIF_SET_TAG(1, 1, 0, 0, 0, 1);
    *reinterpret_cast<u_long *>(data_start + 6) = SCE_GIF_PACKED_AD;

    *reinterpret_cast<u_long *>(data_start + 8) = 0;
    *reinterpret_cast<u_long *>(data_start + 10) = SCE_GS_NOP;

    *reinterpret_cast<u_long *>(data_start + 12) = MakeDmaTag(0, eDMA_NEXT);
    data_start[14] = SCE_VIF1_SET_FLUSHA(0);
    data_start[15] = 0;

    uint32 *data = data_start + 16;

    dma_chain->Add(data_start, data_start + 12);

    data_start = data;

    *reinterpret_cast<u_long *>(data_start) = MakeDmaTag(41, eDMA_REF, Path3GateBuffer);
    data_start[2] = 0;
    data_start[3] = 0;

    *reinterpret_cast<u_long *>(data_start + 4) = MakeDmaTag(0, eDMA_NEXT);
    data_start[6] = 0;
    data_start[7] = 0;

    dma_chain->Add(data_start, data_start + 4);

    return 1;
}

eActiveTexture *eTextureDownloader::DownloadSingleBuffered(eDmaChain *dma_chain, eDmaChain *tex_chain) {
    currTex = tex_chain->CurrentDmaTag;
    currDMA = dma_chain->CurrentDmaTag;

    if (Path3TextureTransferEnabled) {
        if (TextureDownloader.AddPath3GateDmaTag(dma_chain, dma_chain)) {
            AttachDummyTexture(tex_chain, 1);
        }
        if (TextureDownloader.AddPath3GateDmaTag(dma_chain, dma_chain)) {
            FillVramWithStaticTextures(tex_chain);
            AttachDummyTexture(tex_chain, 0);
        }
    }

    if (!Path3TextureTransferEnabled) {
        tex_chain = dma_chain;
    }

    NumBatches++;

    VramFillInfo vram_fill_info;

    vram_fill_info.StartAddress = VramBlockStart;
    vram_fill_info.DesiredEndAddress = VramBlockTop;
    vram_fill_info.EndAddress = VramBlockTop;
    vram_fill_info.FillAddress = 0;

    if (VramBlockStart >= LockedVramBlockStart && VramBlockStart < LockedVramBlockEnd) {
        NumSyncs++;
        SyncRequired = 1;
    }

    eActiveTexture *active_texture =
        FillVramWithTextures(tex_chain, dma_chain, DownloadingTextureList.GetHead(), &vram_fill_info, false);

    LockedVramBlockStart = vram_fill_info.StartAddress;
    LockedVramBlockEnd = vram_fill_info.FillAddress;

    return active_texture;
}

eActiveTexture *eTextureDownloader::DownloadDoubleBuffered(eDmaChain *dma_chain, eDmaChain *tex_chain, bool forcePageAlign) {
    currTex = tex_chain->CurrentDmaTag;
    currDMA = dma_chain->CurrentDmaTag;

    if (Path3TextureTransferEnabled) {
        if (TextureDownloader.AddPath3GateDmaTag(dma_chain, dma_chain)) {
            FillVramWithStaticTextures(tex_chain);
            AttachDummyTexture(tex_chain, 0);
        }
    }

    if (!Path3TextureTransferEnabled) {
        tex_chain = dma_chain;
    }

    NumBatches++;

    VramFillInfo vram_fill_info;
    eActiveTexture *active_texture;

    if (DownloadingPhase == 0) {
        vram_fill_info.StartAddress = VramBlockStart;
        vram_fill_info.DesiredEndAddress = (VramBlockTop + VramBlockStart) / 2;
        vram_fill_info.EndAddress = VramBlockBoundary;
        vram_fill_info.FillAddress = 0;

        if (VramBlockStart >= LockedVramBlockStart && VramBlockStart < LockedVramBlockEnd) {
            NumSyncs++;
            SyncRequired = 2;
        }

        active_texture =
            FillVramWithTextures(tex_chain, dma_chain, DownloadingTextureList.GetHead(), &vram_fill_info, forcePageAlign);

        if ((VramBlockTop + VramBlockStart) / 2 < vram_fill_info.FillAddress) {
            VramBlockBoundary = vram_fill_info.FillAddress;
        } else {
            VramBlockBoundary = (VramBlockTop + VramBlockStart) / 2;
        }

        DownloadingPhase = 1;
    } else {
        vram_fill_info.StartAddress = VramBlockBoundary;
        vram_fill_info.DesiredEndAddress = VramBlockTop;
        vram_fill_info.EndAddress = VramBlockTop;
        vram_fill_info.FillAddress = 0;

        if (VramBlockBoundary >= LockedVramBlockStart && VramBlockBoundary < LockedVramBlockEnd) {
            NumSyncs++;
            SyncRequired = 2;
        }

        active_texture =
            FillVramWithTextures(tex_chain, dma_chain, DownloadingTextureList.GetHead(), &vram_fill_info, forcePageAlign);

        if (vram_fill_info.FillAddress == vram_fill_info.StartAddress) {
            VramBlockBoundary = VramBlockTop;
        }

        DownloadingPhase = 0;
    }

    if (vram_fill_info.FillAddress != vram_fill_info.StartAddress) {
        LockedVramBlockStart = vram_fill_info.StartAddress;
        LockedVramBlockEnd = vram_fill_info.FillAddress;
    } else {
        LockedVramBlockStart = VramBlockTop;
        LockedVramBlockEnd = VramBlockTop;
    }

    if (vram_fill_info.EndAddress < vram_fill_info.FillAddress) {
        for (eActiveTexture *t = DownloadingTextureList.GetHead();
             t != DownloadingTextureList.EndOfList() && t != active_texture; t = t->GetNext()) {
        }
    }

    return active_texture;
}

void eTextureDownloader::FillVramWithStaticTextures(eDmaChain *dma_chain) {
    while (!StaticTextureList.IsEmpty()) {
        eStaticTexture *static_texture = StaticTextureList.RemoveHead();

        rAttachTextureDownloadPacket(dma_chain, static_texture->Texture, static_texture->ImageDownloadAddress,
                                     static_texture->PaletteDownloadAddress);

        bFree(StaticTextureSlotPool, static_texture);
    }
}

int least_down = 999999999;
int dmaheight = 255;

PS2VramInfo PS2VramInfoPSMCT16 = {64, 64, 16, 8, 4, 8, {0,  2,  8,  10, 1,  3,  9,  11, 4,  6,  12, 14, 5,  7,  13, 15,
                                                        16, 28, 24, 26, 17, 19, 25, 27, 20, 22, 28, 30, 21, 23, 29, 31}};
PS2VramInfo PS2VramInfoPSMT8 = {128, 64, 16, 16, 8, 4, {0, 1, 4,  5,  16, 17, 20, 21, 2,  3,  6,  7,  18, 19, 22, 23,
                                                        8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31}};
PS2VramInfo PS2VramInfoPSMT4 = {128, 128, 32, 16, 4, 8, {0,  2,  8,  10, 1,  3,  9,  11, 4,  6,  12, 14, 5,  7,  13, 15,
                                                         16, 28, 24, 26, 17, 19, 25, 27, 20, 22, 28, 30, 21, 23, 29, 31}};

int GetPS2TextureBufferWidth(int width, int bit_depth) {
    int aligned_width;

    if (bit_depth < 16) {
        aligned_width = (width + 127) & ~127;
    } else {
        aligned_width = (width + 63) & ~63;
    }

    return aligned_width >> 6;
}

PS2VramInfo *GetPS2VramInfo(int bit_depth) {
    if (bit_depth < 8) return &PS2VramInfoPSMT4;
    if (bit_depth < 16) return &PS2VramInfoPSMT8;
    if (bit_depth < 24) return &PS2VramInfoPSMCT16;
    if (bit_depth < 32) return &PS2VramInfoPSMCT32;
    return &PS2VramInfoPSMCT32;
}

int GetPS2VramSize(int width, int height, int bit_depth) {
    PS2VramInfo *info = GetPS2VramInfo(bit_depth);

    int num_pages_x = (width + info->PixelsPerPageX - 1) / info->PixelsPerPageX;
    int num_pages_y = (height + info->PixelsPerPageY - 1) / info->PixelsPerPageY;
    int total_num_pages = num_pages_x * num_pages_y;
    int total_num_blocks = total_num_pages * 32;

    if (total_num_pages != 0) {
        int page_pixels_x = width - (num_pages_x - 1) * info->PixelsPerPageX;
        int page_pixels_y = height - (num_pages_y - 1) * info->PixelsPerPageY;
        int num_blocks_x = (page_pixels_x + info->PixelsPerBlockX - 1) / info->PixelsPerBlockX;
        int num_blocks_y = (page_pixels_y + info->PixelsPerBlockY - 1) / info->PixelsPerBlockY;
        int block_index = (num_blocks_x - 1) + (num_blocks_y - 1) * info->BlocksPerPageX;

        total_num_blocks = info->BlockNumbers[block_index] + total_num_blocks - 31;
    }

    return total_num_blocks * 256;
}


eActiveTexture *eTextureDownloader::FillVramWithTextures(eDmaChain *tex_chain, eDmaChain *dma_chain2,
                                                         eActiveTexture *current_texture, VramFillInfo *vram_fill_info,
                                                         bool forcePageAlign) {
    if (SyncRequired) {
        if (TextureDownloader.AddPath3GateDmaTag(dma_chain2, tex_chain)) {
            AttachDummyTexture(tex_chain, 1);
        }
        SyncRequired = 0;
    }

    int eop;
    DATA16 *chain_tag;
    void *pbuf_very_start = eFrameMallocGetPos();
    int vram_address = vram_fill_info->StartAddress;

    if (forcePageAlign) {
        vram_address = (vram_address + 31) & 0xFFFFFFE0;
    }

    int total_amount_downloaded = 0;

    while (current_texture != DownloadingTextureList.EndOfList() && vram_address < vram_fill_info->DesiredEndAddress) {
        TextureInfo *texture_info = current_texture->pTextureInfo;
        TextureInfoPlatInfo *plat_info = texture_info->GetPlatInfo();
        TextureInfoPlatInfo *parent_plat_info = texture_info->pImageParent->GetPlatInfo();
        int palette_blocks_required = plat_info->PaletteVramBlocksRequired;
        int image_blocks_required = plat_info->ImageVramBlocksRequired;
        int MipBlocks = 0;

        if (texture_info->NumMipMapLevels > 1) {
            for (int image_index = 1; image_index < texture_info->NumMipMapLevels; image_index++) {
                TexturePlatImageInfo *image_info = &plat_info->ImageInfos[image_index];
                int mp = (GetPS2VramSize(image_info->Width, image_info->Height, texture_info->ImageCompressionType) + 255) / 256;

                MipBlocks += mp;
            }
            image_blocks_required += MipBlocks;
        }

        if (forcePageAlign) {
            palette_blocks_required = (palette_blocks_required + 31) & 0xFFFFFFE0;
            image_blocks_required = (image_blocks_required + 31) & 0xFFFFFFE0;
        }

        if (image_blocks_required != 0 && parent_plat_info->ImageVramAddress != 0) {
            image_blocks_required = 0;
        }

        if (VramBlockTop - VramBlockStart < palette_blocks_required + image_blocks_required) {
            static int prev_frame;

            prev_frame = eFrameCounter;

            return DownloadingTextureList.EndOfList();
        }

        if (vram_address + palette_blocks_required + image_blocks_required > vram_fill_info->EndAddress) {
            break;
        }

        if (palette_blocks_required) {
            DATA16 *pbuf_start = static_cast<DATA16 *>(eFrameMalloc(8 * sizeof(DATA16)));
            DATA16 *pbuf = pbuf_start;

            if (!pbuf_start) {
                goto out_of_memory;
            }

            pbuf->d[0] = MakeDmaTag(6, eDMA_CNT);
            pbuf->w[2] = 0;
            pbuf->w[3] = Path3TextureTransferEnabled ? 0 : SCE_VIF1_SET_DIRECT(6, 0);
            pbuf++;

            pbuf->d[0] = MakeGifTag(4, 0, 0, 0, GIF_PACKED, 1);
            pbuf->d[1] = SCE_GIF_PACKED_AD;
            pbuf++;

            pbuf->d[0] = SCE_GS_SET_BITBLTBUF(0, 0, 0, vram_address, 1, plat_info->ClutPixelStorageMode);
            pbuf->d[1] = SCE_GS_BITBLTBUF;
            pbuf++;

            pbuf->d[0] = SCE_GS_SET_TRXPOS(0, 0, 0, 0, 0);
            pbuf->d[1] = SCE_GS_TRXPOS;
            pbuf++;

            if (texture_info->NumPaletteEntries == 256) {
                pbuf->d[0] = SCE_GS_SET_TRXREG(16, 16);
            } else {
                pbuf->d[0] = SCE_GS_SET_TRXREG(8, 2);
            }
            pbuf->d[1] = SCE_GS_TRXREG;
            pbuf++;

            pbuf->d[0] = SCE_GS_SET_TRXDIR(0);
            pbuf->d[1] = SCE_GS_TRXDIR;
            pbuf++;

            pbuf->d[0] = MakeGifTag(texture_info->PaletteSize / 16, 0, 0, 0, GIF_IMAGE, 0);
            pbuf->d[1] = 0;
            pbuf++;

            pbuf->d[0] = MakeDmaTag(texture_info->PaletteSize / 16, eDMA_REF, texture_info->PaletteData);
            pbuf->w[2] = 0;
            pbuf->w[3] = Path3TextureTransferEnabled ? 0 : SCE_VIF1_SET_DIRECT(texture_info->PaletteSize / 16, 0);

            plat_info->PaletteVramAddress = vram_address;

            vram_address += palette_blocks_required;

            NumPalettes++;
            NumPaletteBytes += texture_info->PaletteSize;
        }

        if (image_blocks_required) {
            uint32 dma_main_memory_address = reinterpret_cast<uint32>(texture_info->ImageData);

            for (int image_index = 0; image_index < texture_info->NumMipMapLevels; image_index++) {
                TexturePlatImageInfo *image_info = &plat_info->ImageInfos[image_index];
                int image_size = texture_info->BaseImageSize >> (image_index * 2);
                int dma_width = image_info->Width;
                int dma_height = image_info->Height;
                int dma_transfer_mode = plat_info->PixelStorageMode;
                int dma_texture_buffer_width = image_info->TBW;
                int dma_vram_address = vram_address + image_info->TBP;
                int dma_swizzled = image_info->IsSwizzled;

                NumTextures++;
                NumTextureBytes += image_size;
                NumUnalignedTextures += (dma_main_memory_address & 0x7F) != 0;
                Less8k += image_size < 8192;
                NumSwizzled += dma_swizzled;

                if (dma_swizzled) {
                    if (texture_info->ImageCompressionType == TEXCOMP_4BIT) {
                        dma_width = dma_width / 2;
                        dma_height = dma_height / 2;
                        dma_texture_buffer_width = GetPS2TextureBufferWidth(dma_width, 16);
                        dma_transfer_mode = PSMCT16;
                    } else {
                        dma_width = dma_width / 2;
                        dma_height = dma_height / 2;
                        dma_texture_buffer_width = GetPS2TextureBufferWidth(dma_width, 32);
                        dma_transfer_mode = PSMCT32;
                    }
                }

                for (int amount_downloaded = 0; amount_downloaded < image_size;) {
                    int amount_to_download = image_size - amount_downloaded;

                    if (amount_to_download > dmaheight << 11) {
                        amount_to_download = dmaheight << 11;
                    }

                    int dma_y = (dma_height * amount_downloaded) / image_size;
                    int dma_rows = (dma_height * amount_to_download) / image_size;

                    DATA16 *pbuf_start = static_cast<DATA16 *>(eFrameMalloc(8 * sizeof(DATA16)));
                    DATA16 *pbuf = pbuf_start;

                    if (!pbuf_start) {
                        goto out_of_memory;
                    }

                    pbuf->d[0] = MakeDmaTag(6, eDMA_CNT);
                    pbuf->w[2] = 0;
                    pbuf->w[3] = Path3TextureTransferEnabled ? 0 : SCE_VIF1_SET_DIRECT(6, 0);
                    pbuf++;

                    pbuf->d[0] = MakeGifTag(4, 0, 0, 0, GIF_PACKED, 1);
                    pbuf->d[1] = SCE_GIF_PACKED_AD;
                    pbuf++;

                    pbuf->d[0] = SCE_GS_SET_BITBLTBUF(0, 0, 0, dma_vram_address, dma_texture_buffer_width, dma_transfer_mode);
                    pbuf->d[1] = SCE_GS_BITBLTBUF;
                    pbuf++;

                    pbuf->d[0] = SCE_GS_SET_TRXPOS(0, 0, 0, dma_y, 0);
                    pbuf->d[1] = SCE_GS_TRXPOS;
                    pbuf++;

                    pbuf->d[0] = SCE_GS_SET_TRXREG(dma_width, dma_rows);
                    pbuf->d[1] = SCE_GS_TRXREG;
                    pbuf++;

                    pbuf->d[0] = SCE_GS_SET_TRXDIR(0);
                    pbuf->d[1] = SCE_GS_TRXDIR;
                    pbuf++;

                    pbuf->d[0] = MakeGifTag(amount_to_download / 16, 0, 0, 0, GIF_IMAGE, 0);
                    pbuf->d[1] = 0;
                    pbuf++;

                    pbuf->d[0] = MakeDmaTag(amount_to_download / 16, eDMA_REF,
                                            reinterpret_cast<void *>(dma_main_memory_address + amount_downloaded));
                    pbuf->w[2] = 0;
                    pbuf->w[3] = Path3TextureTransferEnabled ? 0 : SCE_VIF1_SET_DIRECT(amount_to_download / 16, 0);

                    amount_downloaded += amount_to_download;
                    total_amount_downloaded += amount_downloaded;
                }

                dma_main_memory_address += image_size;
            }

            parent_plat_info->ImageVramAddress = vram_address;

            vram_address += image_blocks_required;
        }

        current_texture = current_texture->GetNext();
    }

    eop = 1;

    if (Path3TextureTransferEnabled && total_amount_downloaded < 8192) {
        eop = 0;
    }

    chain_tag = static_cast<DATA16 *>(eFrameMalloc(3 * sizeof(DATA16)));

    if (!chain_tag) {
    out_of_memory:
        tex_chain->CurrentDmaTag = currTex;
        dma_chain2->CurrentDmaTag = currDMA;
        return 0;
    }

    chain_tag[0].d[0] = MakeDmaTag(2, eDMA_NEXT);
    chain_tag[0].w[2] = 0;
    chain_tag[0].w[3] = Path3TextureTransferEnabled ? 0 : SCE_VIF1_SET_DIRECT(2, 0);

    chain_tag[1].d[0] = MakeGifTag(1, eop, 0, 0, GIF_PACKED, 1);
    chain_tag[1].d[1] = 0xF;

    chain_tag[2].d[0] = 0;
    chain_tag[2].d[1] = 0;

    tex_chain->Add(pbuf_very_start, chain_tag);

    if (!eop) {
        AttachDummyTexture(tex_chain, 1);
    }

    vram_fill_info->FillAddress = vram_address;

    return current_texture;
}

void UpdateDmaStats() {}

struct DMAState {
    // total size: 0x38
    bMatrix4 *LocalWorldMatrix0;             // offset 0x0, size 0x4
    bMatrix4 *LocalWorldMatrix1;             // offset 0x4, size 0x4
    eLightMaterial *LightMaterial0;          // offset 0x8, size 0x4
    eLightMaterial *LightMaterial1;          // offset 0xC, size 0x4
    eLightContext *LightContext;             // offset 0x10, size 0x4
    eRenderTarget *RenderTarget;             // offset 0x14, size 0x4
    unsigned int RenderTargetParameter;      // offset 0x18, size 0x4
    unsigned int pad0;                       // offset 0x1C, size 0x4
    eRenderTargetMatrixPack *MatrixPack0;    // offset 0x20, size 0x4
    eRenderTargetMatrixPack *MatrixPack1;    // offset 0x24, size 0x4
    unsigned int MatrixPackSlot;             // offset 0x28, size 0x4
    unsigned int pad1;                       // offset 0x2C, size 0x4
    unsigned int ToggleLocalWorld;           // offset 0x30, size 0x4
    unsigned int ToggleMatrixPack;           // offset 0x34, size 0x4

    DMAState() {
        LocalWorldMatrix0 = 0; LocalWorldMatrix1 = 0;
        LightMaterial0 = 0; LightMaterial1 = 0;
        LightContext = 0;
        RenderTarget = 0; RenderTargetParameter = 0;
        MatrixPack0 = 0; MatrixPack1 = 0; MatrixPackSlot = 0;

        ToggleLocalWorld = 0; ToggleMatrixPack = 0;
    }
};

// Convierte una lista de datos de DMA en la cadena del frame: cada dato es un tag REF al paquete (tiras, luces,
// matrices, contexto de textura...) con los codigos del VIF que hacen falta, y solo se manda lo que cambia respecto
// a dma_state. Se escribe directamente en el frame malloc, por la vista sin cache acelerada.
void AttachDmaDataListMain(eDmaChain *dma_chain, eDmaDataList *dma_data_list, DMAState *dma_state) {
    uint32 *data_start = static_cast<uint32 *>(UncachedAcceleratedAddress(AlignPointer(eFrameMallocGetPos(), 64)));
    uint32 *data_end = static_cast<uint32 *>(UncachedAcceleratedAddress(eFrameMallocGetEnd()));
    eDmaData *dma_data = dma_data_list->GetHead();
    uint32 *data = data_start;
    uint32 prev_opcode = 0;

    while (dma_data != dma_data_list->EndOfList() &&
           reinterpret_cast<uint8 *>(data_end) - reinterpret_cast<uint8 *>(data) > 128) {
        eDmaData *next_data = dma_data->GetNext();
        uint16 type = dma_data->Type;
        uint8 *data_ptr = dma_data->pData;
        uint16 qwsize = dma_data->QWordSize;
        uint16 parameter = dma_data->Parameter;

        // asm de EA (aprobado el 2026-10-02): el original adelanta el siguiente dato con su propia linea de fuente
        // (EcstasyE.cpp:8309 de la A124) y ee-gcc no tiene patron para `pref`.
        asm("pref 0, 0(%0)" : : "r"(next_data));

        switch (type) {
            case EDMA_STRIP: {
                *reinterpret_cast<u_long *>(data) = MakeDmaTag(qwsize, eDMA_REF, data_ptr);
                data += 2;
                *data++ = prev_opcode;
                *data++ = SCE_VIF1_SET_ITOP(parameter, 0);

                prev_opcode = 0;
                break;
            }

            case EDMA_SET_LOCAL_WORLD: {
                uint32 load_call_address;

                if (data_ptr == reinterpret_cast<uint8 *>(dma_state->LocalWorldMatrix0)) {
                    load_call_address = reinterpret_cast<uint32>(vuLoadMatricies00) >> 3;
                } else if (data_ptr == reinterpret_cast<uint8 *>(dma_state->LocalWorldMatrix1)) {
                    load_call_address = reinterpret_cast<uint32>(vuLoadMatricies01) >> 3;
                } else {
                    if (dma_state->ToggleLocalWorld & 1) {
                        *reinterpret_cast<u_long *>(data) = MakeDmaTag(4, eDMA_REF, data_ptr);
                        data += 2;
                        *data++ = prev_opcode;
                        *data++ = VIF_CODE(UNPACK | V4_32, 4, 0x351);

                        load_call_address = reinterpret_cast<uint32>(vuLoadMatricies01) >> 3;
                        dma_state->LocalWorldMatrix1 = reinterpret_cast<bMatrix4 *>(data_ptr);
                    } else {
                        *reinterpret_cast<u_long *>(data) = MakeDmaTag(4, eDMA_REF, data_ptr);
                        data += 2;
                        *data++ = prev_opcode;
                        *data++ = VIF_CODE(UNPACK | V4_32, 4, 0x34D);

                        load_call_address = reinterpret_cast<uint32>(vuLoadMatricies00) >> 3;
                        dma_state->LocalWorldMatrix0 = reinterpret_cast<bMatrix4 *>(data_ptr);
                    }

                    prev_opcode = 0;
                    dma_state->ToggleLocalWorld++;
                }

                *reinterpret_cast<u_long *>(data) = MakeDmaTag(0, eDMA_CNT);
                data += 2;
                *data++ = prev_opcode;
                *data++ = SCE_VIF1_SET_MSCAL(load_call_address + dma_state->MatrixPackSlot, 0);

                prev_opcode = SCE_VIF1_SET_MSCAL(reinterpret_cast<uint32>(vuDoNothing) >> 3, 0);
                break;
            }

            case EDMA_SET_MATRIX_PACK: {
                uint32 offset_matrix_pack;

                if (data_ptr == reinterpret_cast<uint8 *>(dma_state->MatrixPack0)) {
                    dma_state->MatrixPackSlot = 0;
                    break;
                }

                if (data_ptr == reinterpret_cast<uint8 *>(dma_state->MatrixPack1)) {
                    dma_state->MatrixPackSlot = 6;
                    break;
                }

                if (dma_state->ToggleMatrixPack & 1) {
                    dma_state->MatrixPack1 = reinterpret_cast<eRenderTargetMatrixPack *>(data_ptr);
                    dma_state->MatrixPackSlot = 6;
                    offset_matrix_pack = 0x332;
                } else {
                    dma_state->MatrixPack0 = reinterpret_cast<eRenderTargetMatrixPack *>(data_ptr);
                    dma_state->MatrixPackSlot = 0;
                    offset_matrix_pack = 0x317;
                }

                dma_state->ToggleMatrixPack++;

                *reinterpret_cast<u_long *>(data) = MakeDmaTag(19, eDMA_REF, data_ptr);
                data += 2;
                *data++ = prev_opcode;
                *data++ = VIF_CODE(UNPACK | V4_32, 19, offset_matrix_pack);

                prev_opcode = 0;
                break;
            }

            case EDMA_LIGHTMATERIAL: {
                eLightMaterialPlatInfo *plat_info =
                    static_cast<eLightMaterialPlatInfo *>(reinterpret_cast<eLightMaterial *>(data_ptr)->PlatInfo);

                if (data_ptr == reinterpret_cast<uint8 *>(dma_state->LightMaterial0)) {
                    break;
                }

                dma_state->LightMaterial0 = reinterpret_cast<eLightMaterial *>(data_ptr);

                *reinterpret_cast<u_long *>(data) = MakeDmaTag(0, eDMA_CNT);
                data += 2;
                *data++ = prev_opcode;
                *data++ = SCE_VIF1_SET_FLUSHE(0);

                *reinterpret_cast<u_long *>(data) = MakeDmaTag(6, eDMA_REF, plat_info->feDmaData);
                data += 2;
                *data++ = SCE_VIF1_SET_NOP(0);
                *data++ = VIF_CODE(UNPACK | V4_32, 6, 0x35B);

                prev_opcode = 0;
                break;
            }

            case EDMA_LIGHTCONTEXT: {
                if (data_ptr == reinterpret_cast<uint8 *>(dma_state->LightContext)) {
                    break;
                }

                dma_state->LightContext = reinterpret_cast<eLightContext *>(data_ptr);

                *reinterpret_cast<u_long *>(data) = MakeDmaTag(0, eDMA_CNT);
                data += 2;
                *data++ = prev_opcode;
                *data++ = SCE_VIF1_SET_FLUSHE(0);

                *reinterpret_cast<u_long *>(data) = MakeDmaTag(qwsize, eDMA_REF, data_ptr);
                data += 2;
                *data++ = SCE_VIF1_SET_NOP(0);
                *data++ = VIF_CODE(UNPACK | V4_32, qwsize, parameter);

                prev_opcode = 0;
                break;
            }

            case EDMA_SETTARGET: {
                if (data_ptr == reinterpret_cast<uint8 *>(dma_state->RenderTarget) &&
                    parameter == dma_state->RenderTargetParameter) {
                    break;
                }

                dma_state->RenderTarget = reinterpret_cast<eRenderTarget *>(data_ptr);
                dma_state->RenderTargetParameter = parameter;

                if (prev_opcode) {
                    *reinterpret_cast<u_long *>(data) = MakeDmaTag(0, eDMA_CNT);
                    data += 2;
                    *data++ = prev_opcode;
                    *data++ = SCE_VIF1_SET_NOP(0);

                    prev_opcode = 0;
                }

                int type = parameter;

                *reinterpret_cast<u_long *>(data) =
                    MakeDmaTag(14, eDMA_REF, reinterpret_cast<eRenderTarget *>(data_ptr)->GetDrawContextData(type));
                data += 2;
                *data++ = SCE_VIF1_SET_FLUSHE(0);
                *data++ = SCE_VIF1_SET_DIRECT(14, 0);
                break;
            }

            case EDMA_SET_TEXTURE_CONTEXT: {
                *reinterpret_cast<u_long *>(data) =
                    MakeDmaTag(9, eDMA_REF, reinterpret_cast<eActiveTexture *>(data_ptr)->pPreviousDmaData);
                data += 2;
                *data++ = SCE_VIF1_SET_FLUSHE(0);
                *data++ = VIF_CODE(UNPACK | V4_32, 8, 0x2E3);
                break;
            }

            case EDMA_DIRECT: {
                *reinterpret_cast<u_long *>(data) = MakeDmaTag(qwsize, eDMA_REF, data_ptr);
                data += 2;
                *data++ = prev_opcode;
                *data++ = SCE_VIF1_SET_DIRECT(qwsize, 0);

                prev_opcode = 0;
                break;
            }

            case EDMA_POLY3D: {
                *reinterpret_cast<u_long *>(data) = MakeDmaTag(qwsize, eDMA_REF, data_ptr);
                data += 2;
                *data++ = prev_opcode;
                *data++ = SCE_VIF1_SET_ITOP(parameter, 0);

                prev_opcode = 0;
                break;
            }

            case EDMA_FLUSH: {
                *reinterpret_cast<u_long *>(data) = MakeDmaTag(0, eDMA_CNT);
                data += 2;
                *data++ = prev_opcode;
                *data++ = SCE_VIF1_SET_FLUSHE(0);

                prev_opcode = 0;
                break;
            }

            case EDMA_SET_BONE_MATRIX_PACK: {
                *reinterpret_cast<u_long *>(data) = MakeDmaTag(144, eDMA_REF, data_ptr);
                data += 2;
                *data++ = prev_opcode;
                *data++ = VIF_CODE(UNPACK | V4_32, 144, 0x1B8);

                prev_opcode = 0;
                break;
            }

            case EDMA_VERTEXALPHA: {
                *reinterpret_cast<u_long *>(data) = MakeDmaTag(1, eDMA_REF, data_ptr);
                data += 2;
                *data++ = prev_opcode;
                *data++ = VIF_CODE(UNPACK | V4_32, 1, 0x280);

                prev_opcode = 0;
                break;
            }
        }

        dma_data = next_data;
    }

    if (data != data_start) {
        *reinterpret_cast<u_long *>(data) = MakeDmaTag(0, eDMA_NEXT);
        data[2] = prev_opcode;
        data[3] = SCE_VIF1_SET_NOP(0);

        dma_chain->Add(PhysicalAddress(data_start), PhysicalAddress(data));

        eFrameMallocSetPos(PhysicalAddress(AlignPointer(data + 4, 64)));
    }
}


u_long GetTex0(TextureInfo *texture_info, uint32 vram_block_image, uint32 vram_block_palette) {
    u_long tex0 = 0;

    if (texture_info) {
        TextureInfoPlatInfo *plat_info = texture_info->GetPlatInfo();

        tex0 = SCE_GS_SET_TEX0(vram_block_image, plat_info->ImageInfos[0].TBW, plat_info->PixelStorageMode,
                               texture_info->ShiftWidth, texture_info->ShiftHeight, 1, plat_info->TextureFx,
                               vram_block_palette, plat_info->ClutPixelStorageMode, 0, 0, 2);
    }

    return tex0;
}

// Los registros ALPHA y TEST del contexto 1 que carga el microcodigo, para tocarlos desde el depurador (los valores,
// los del .data de la A124).
uint32 prA = 2;
uint32 prB = 2;
uint32 prC = 2;
uint32 prD = 1;
uint32 prF;
uint32 prt0 = 1;
uint32 prt1;
uint32 prt2 = 48;
uint32 prt3 = 1;
uint32 prt4;
uint32 prt5;
uint32 prt6 = 1;
uint32 prt7 = 2;
float RSPECpow = 4.0f;
int32 headTexFunc;
int32 tfxHead;

// El arranque del microcodigo de la VU1 en cada fotograma: llama a su inicializacion y le sube las constantes (las filas
// de STROW, los GIF tags de cada tipo de primitiva, los registros del GS de los dos contextos con las texturas del
// entorno y las constantes de la raiz cuadrada inversa rapida), en un paquete que se cierra alineado a 16 bytes.
void AttachMicroCode(eDmaChain *dma_chain) {
    uint32 *data_start;
    uint32 *data;
    uint32 qwsize;

    dma_chain->Add(vu1MicroCodeStart);

    data = static_cast<uint32 *>(eFrameMallocGetPos());
    data_start = data;
    data += 2;

    *data++ = SCE_VIF1_SET_MSCAL(reinterpret_cast<uint32>(vuInitMicroCode) >> 3, 0);
    *data++ = SCE_VIF1_SET_NOP(0);

    *data++ = SCE_VIF1_SET_BASE(200, 0);
    *data++ = SCE_VIF1_SET_OFFSET(120, 0);

    *data++ = SCE_VIF1_SET_STROW(0);
    *data++ = 0x47800000; // 65536.0f
    *data++ = 0x47800000;
    *data++ = 0x47800000;
    *data++ = 0x47800000;

    *data++ = SCE_VIF1_SET_STMASK(0);
    *data++ = 0;

    *data++ = SCE_VIF1_SET_STCYCL(1, 1, 0);

    {
        int base_prim = DrawWireframe ? PRIM_LINESTRIP : PRIM_TRISTRIP;
        int clip_prim = DrawWireframe ? PRIM_LINESTRIP : PRIM_TRIFAN;
        int sprite_prim = DrawWireframe ? PRIM_LINESTRIP : PRIM_TRISTRIP;
        int iip;
        int prim_flags;

        prim_flags = EnableTexturing ? PRIM_TME : 0;
        iip = PRIM_IIP;
        prim_flags |= iip | PRIM_ABE;

        *data++ = VIF_CODE(UNPACK | V4_32, 2, 0x268);
        *data++ = MakeGifTagLo(1, 1);
        *data++ = MakeGifTagHi(1, base_prim | iip | PRIM_ABE, GIF_PACKED, 3);
        *data++ = 0x412;
        *data++ = 0;
        *data++ = MakeGifTagLo(1, 1);
        *data++ = MakeGifTagHi(1, clip_prim | iip | PRIM_ABE, GIF_PACKED, 3);
        *data++ = 0x412;
        *data++ = 0;
        *data++ = 0;

        *data++ = VIF_CODE(UNPACK | V4_32, 1, 0x26A);
        *data++ = MakeGifTagLo(1, 1);
        *data++ = MakeGifTagHi(1, PRIM_LINE | PRIM_IIP, GIF_PACKED, 4);
        *data++ = 0x4141;
        *data++ = *reinterpret_cast<uint32 *>(&DrawNormalsScale);
        *data++ = 0;

        *data++ = VIF_CODE(UNPACK | V4_32, 10, 0x26B);

        *data++ = MakeGifTagLo(0, 1);
        *data++ = MakeGifTagHi(1, base_prim | prim_flags, GIF_PACKED, 3);
        *data++ = 0x412;
        *data++ = 0;

        *data++ = MakeGifTagLo(0, 1);
        *data++ = MakeGifTagHi(1, clip_prim | prim_flags, GIF_PACKED, 3);
        *data++ = 0x412;
        *data++ = 0;

        *data++ = MakeGifTagLo(0, 1);
        *data++ = MakeGifTagHi(1, base_prim | prim_flags | PRIM_CTXT2 | PRIM_ABE, GIF_PACKED, 3);
        *data++ = 0x412;
        *data++ = 0;

        *data++ = MakeGifTagLo(0, 1);
        *data++ = MakeGifTagHi(1, clip_prim | prim_flags | PRIM_CTXT2 | PRIM_ABE, GIF_PACKED, 3);
        *data++ = 0x412;
        *data++ = 0;

        *data++ = MakeGifTagLo(0, 1);
        *data++ = MakeGifTagHi(1, base_prim | iip | PRIM_CTXT2 | PRIM_ABE, GIF_PACKED, 3);
        *data++ = 0x412;
        *data++ = 0;

        *data++ = MakeGifTagLo(0, 1);
        *data++ = MakeGifTagHi(1, clip_prim | iip | PRIM_CTXT2 | PRIM_ABE, GIF_PACKED, 3);
        *data++ = 0x412;
        *data++ = 0;

        *data++ = MakeGifTagLo(0, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_REGLIST, 1);
        *data++ = 1;
        *data++ = 0;

        *data++ = MakeGifTagLo(0, 1);
        *data++ = MakeGifTagHi(1, sprite_prim | prim_flags, GIF_PACKED, 2);
        *data++ = 0x42;
        *data++ = 0;

        *data++ = MakeGifTagLo(1, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = 0xF;
        *data++ = 0;

        *data++ = MakeGifTagLo(0, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_REGLIST, 1);
        *data++ = 8;
        *data++ = 0;

        *data++ = VIF_CODE(UNPACK | V4_32, 2, 0x282);

        *data++ = MakeGifTagLo(1, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;
        *data++ = 0;
        *data++ = 0;
        *data++ = SCE_GS_TEXFLUSH;
        *data++ = 0;

        *data++ = SCE_VIF1_SET_NOP(0);
        *data++ = VIF_CODE(UNPACK | V4_32, 3, 0x383);

        *data++ = MakeGifTagLo(2, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;

        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_ALPHA(prA, prB, prC, prD, prF);
        *reinterpret_cast<u_long *>(data + 2) = SCE_GS_ALPHA_1;
        data += 4;
        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_TEST(prt0, prt1, prt2, prt3, prt4, prt5, prt6, prt7);
        *reinterpret_cast<u_long *>(data + 2) = SCE_GS_TEST_1;
        data += 4;

        *data++ = SCE_VIF1_SET_NOP(0);
        *data++ = VIF_CODE(UNPACK | V4_32, 2, 0x38D);

        *data++ = MakeGifTagLo(1, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;

        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_TEST(1, 0, 0, 3, 1, 1, 1, 2);
        *reinterpret_cast<u_long *>(data + 2) = SCE_GS_TEST_1;
        data += 4;

        *data++ = SCE_VIF1_SET_NOP(0);
        *data++ = VIF_CODE(UNPACK | V4_32, 3, 0x388);

        *data++ = MakeGifTagLo(2, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;

        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_TEST(1, 5, 0x30, 0, 0, 0, 1, 2);
        *reinterpret_cast<u_long *>(data + 2) = SCE_GS_TEST_1;
        data += 4;

        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_TEST(1, 5, 0x30, 0, 0, 0, 1, 2);
        *reinterpret_cast<u_long *>(data + 2) = SCE_GS_TEST_1;
        data += 4;

        *data++ = SCE_VIF1_SET_NOP(0);
        *data++ = VIF_CODE(UNPACK | V4_32, 8, 0x2F7);

        *data++ = MakeGifTagLo(3, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;

        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_TEX1_2;
        data += 2;

        if (TheGameFlowManager.IsInFrontend()) {
            *reinterpret_cast<u_long *>(data) =
                SCE_GS_SET_TEX0(eVRAM.GetArea(VRAM_ENVMAP0_FULL, eVRAM.GetCurrentLayout()) / 256, 2, PSMCT32, 7, 7, 1, 0, 0, 0, 0, 0, 0);
            data += 2;
            *reinterpret_cast<u_long *>(data) = SCE_GS_TEX0_2;
            data += 2;
        } else {
            *reinterpret_cast<u_long *>(data) =
                SCE_GS_SET_TEX0(eVRAM.GetArea(VRAM_ENVMAP0_FULL, 0) / 256, 4, PSMCT16, 8, 8, 1, 0, 0, 0, 0, 0, 0);
            data += 2;
            *reinterpret_cast<u_long *>(data) = SCE_GS_TEX0_2;
            data += 2;
        }

        *data++ = 0x58;
        *data++ = 0x10;
        *data++ = SCE_GS_ALPHA_2;
        *data++ = 0;

        *data++ = MakeGifTagLo(3, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;

        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_TEX1_2;
        data += 2;

        if (TheGameFlowManager.IsInFrontend()) {
            *reinterpret_cast<u_long *>(data) =
                SCE_GS_SET_TEX0(eVRAM.GetArea(VRAM_ENVMAP0_FULL, eVRAM.GetCurrentLayout()) / 256, 2, PSMCT32, 7, 7, 1, 0, 0, 0, 0, 0, 0);
            data += 2;
            *reinterpret_cast<u_long *>(data) = SCE_GS_TEX0_2;
            data += 2;
        } else {
            *reinterpret_cast<u_long *>(data) =
                SCE_GS_SET_TEX0(eVRAM.GetArea(VRAM_ENVMAP0_FULL, 0) / 256, 4, PSMCT16, 8, 8, 1, 0, 0, 0, 0, 0, 0);
            data += 2;
            *reinterpret_cast<u_long *>(data) = SCE_GS_TEX0_2;
            data += 2;
        }

        *data++ = (egAlphaD << 6) | (egAlphaC << 4) | (egAlphaB << 2) | egAlphaA;
        *data++ = egAlphaF;
        *data++ = SCE_GS_ALPHA_2;
        *data++ = 0;

        *data++ = VIF_CODE(UNPACK | V4_32, 8, 0x2E3);

        *data++ = MakeGifTagLo(6, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;

        *data++ = 0; *data++ = 0; *data++ = SCE_GS_TEX0_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_TEX1_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_MIPTBP1_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_MIPTBP2_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_CLAMP_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_ALPHA_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_PRIM; *data++ = 0;

        *data++ = VIF_CODE(UNPACK | V4_32, 8, 0x2ED);

        *data++ = MakeGifTagLo(6, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;

        *data++ = 0; *data++ = 0; *data++ = SCE_GS_TEX0_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_TEX1_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_MIPTBP1_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_MIPTBP2_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_CLAMP_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_ALPHA_1; *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = SCE_GS_PRIM; *data++ = 0;

        *data++ = SCE_VIF1_SET_NOP(0);
        *data++ = VIF_CODE(UNPACK | V4_32, 8, 0x2FF);

        *data++ = MakeGifTagLo(6, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;

        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_TEX0(eVRAM.GetArea(VRAM_REFLECTION, eVRAM.GetCurrentLayout()) / 256, 4,
                                                            PSMCT16S, 8, 8, 1, tfxHead, 0, 0, 0, 0, 0);
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_TEX0_1;
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_TEX1_1;
        data += 2;
        *reinterpret_cast<u_long *>(data) = 0;
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_MIPTBP1_1;
        data += 2;
        *reinterpret_cast<u_long *>(data) = 0;
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_MIPTBP2_1;
        data += 2;
        *reinterpret_cast<u_long *>(data) = 0;
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_CLAMP_1;
        data += 2;
        *data++ = 0x58;
        *data++ = 0x40;
        *data++ = SCE_GS_ALPHA_1;
        *data++ = 0;
        *data++ = 0; *data++ = 0; *data++ = 0; *data++ = 0;

        *data++ = SCE_VIF1_SET_NOP(0);
        *data++ = VIF_CODE(UNPACK | V4_32, 8, 0x2DB);

        *data++ = MakeGifTagLo(6, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;

        if (pTextureInfoGlossyWindow) {
            *reinterpret_cast<u_long *>(data) =
                GetTex0(pTextureInfoGlossyWindow, eVRAM.GetArea(VRAM_GLOSSYWINDOW, eVRAM.GetCurrentLayout()) / 256,
                        eVRAM.GetArea(VRAM_GLOSSYWINDOW_PALETTE, eVRAM.GetCurrentLayout()) / 256);
            data += 2;
            *reinterpret_cast<u_long *>(data) = SCE_GS_TEX0_1;
            data += 2;
        } else {
            *reinterpret_cast<u_long *>(data) = 0;
            data += 2;
            *reinterpret_cast<u_long *>(data) = 0;
            data += 2;
        }
        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_TEX1_1;
        data += 2;
        *reinterpret_cast<u_long *>(data) = 0;
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_MIPTBP1_1;
        data += 2;
        *reinterpret_cast<u_long *>(data) = 0;
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_MIPTBP2_1;
        data += 2;
        *reinterpret_cast<u_long *>(data) = 0;
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_CLAMP_1;
        data += 2;
        *data++ = 0x58;
        *data++ = 0x46;
        *data++ = SCE_GS_ALPHA_1;
        *data++ = 0;
        *data++ = 0;
        *data++ = 0;
        *data++ = 0;
        *data++ = 0;

        *data++ = SCE_VIF1_SET_NOP(0);
        *data++ = VIF_CODE(UNPACK | V4_32, 4, 0x307);

        *data++ = MakeGifTagLo(3, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;

        *reinterpret_cast<u_long *>(data) = SCE_GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
        data += 2;
        *reinterpret_cast<u_long *>(data) = SCE_GS_TEX1_2;
        data += 2;

        if (pTextureInfoSmallEnvMap) {
            *reinterpret_cast<u_long *>(data) =
                GetTex0(pTextureInfoSmallEnvMap, eVRAM.GetArea(VRAM_SMALL_ENVMAP, eVRAM.GetCurrentLayout()) / 256,
                        eVRAM.GetArea(VRAM_SMALL_ENVMAP_PALETTE, eVRAM.GetCurrentLayout()) / 256);
            data += 2;
            *reinterpret_cast<u_long *>(data) = SCE_GS_TEX0_2;
            data += 2;
        } else {
            *reinterpret_cast<u_long *>(data) = 0;
            data += 2;
            *reinterpret_cast<u_long *>(data) = 0;
            data += 2;
        }

        *data++ = 0x58;
        *data++ = 0x74;
        *data++ = SCE_GS_ALPHA_2;
        *data++ = 0;

        *data++ = SCE_VIF1_SET_DIRECT(3, 0);

        *data++ = MakeGifTagLo(2, 1);
        *data++ = MakeGifTagHi(0, 0, GIF_PACKED, 1);
        *data++ = SCE_GIF_PACKED_AD;
        *data++ = 0;
        *data++ = 0x2A;
        *data++ = 0x40;
        *data++ = SCE_GS_ALPHA_1;
        *data++ = 0;
        *data++ = 0x2A;
        *data++ = 0x40;
        *data++ = SCE_GS_ALPHA_2;
        *data++ = 0;

        *data++ = VIF_CODE(UNPACK | V4_32, 1, 0x275);

        {
            // Las constantes de la raiz cuadrada inversa rapida (el entero de un float como aproximacion de su
            // logaritmo): la del specular y la de la normalizacion.
            float pow_scale = fpRSQRT_PowScale;
            float max_scale = fpRSQRT_MaxScale;
            float pow = fpRSQRT_Pow * pow_scale;
            float max = FloatAsInteger(fpRSQRT_Max * max_scale);
            float rsqrt_magic = max - pow * 1065353216.0f;
            float spec_magic = FloatAsInteger(1.0f) - RSPECpow * 1065353216.0f;

            *data++ = FloatAsInteger(pow);
            *data++ = FloatAsInteger(rsqrt_magic);
            *data++ = FloatAsInteger(RSPECpow);
            *data++ = FloatAsInteger(spec_magic);
        }
    }

    while (reinterpret_cast<uint32>(data) & 0xF) {
        *data++ = 0;
    }

    qwsize = (data - data_start) / 4u - 1;

    *reinterpret_cast<u_long *>(data_start) = MakeDmaTag(qwsize, eDMA_NEXT);
    dma_chain->Add(data_start);

    eFrameMallocSetPos(data);
}

// Las texturas con alfa: se ordenan las claves de cada render target por z, se bajan todas las texturas de una vez
// (si no caben, un aviso cada 60 frames) y se mandan las claves en orden, cambiando matriz, textura y contexto de
// dibujo solo cuando hace falta.
void AttachAlphaPolys(eDmaChain *dma_chain, eDmaChain *tex_chain) {
    bTList<eActiveTexture> *alpha_texture_list = &AlphaTextureList;

    if (alpha_texture_list->IsEmpty()) {
        return;
    }

    for (int i = 0; i < 21; i++) {
        eAlphaList *alpha_list = &RenderTargetAlphaLists[i];

        if (alpha_list->NumKeys) {
            if (!SortAlphaKeys(alpha_list)) {
                alpha_list->Flush();
            }
        }
    }

    TextureDownloader.AddTexturesUnsorted(alpha_texture_list);

    DMAState dma_state;
    int global_z_write_mask = ~DRAW_CONTEXT_WRITE_Z;

    if (eGetZWriteState()) {
        global_z_write_mask = -1;
    }

    eActiveTexture *last_active_texture = TextureDownloader.DownloadSingleBuffered(dma_chain, tex_chain);

    if (last_active_texture && last_active_texture == TextureDownloader.EndOfList()) {
        eActiveTexture *previous_active_texture = nullptr;

        for (int32 i = 0; i < 21; i++) {
            eAlphaList *alpha_list = &RenderTargetAlphaLists[i];

            if (!alpha_list->IsEmpty()) {
                eRenderTarget *render_target = eGetRenderTarget(i);
                int previous_context_flag = -1;
                int previous_bias_level = -1;
                eRenderTargetMatrixPack *matrix_pack0 = render_target->MatrixPack0;
                eRenderTargetMatrixPack *matrix_pack1 = render_target->MatrixPack1;
                eRenderTargetMatrixPack *matrix_pack2 = render_target->MatrixPack2;
                eAlphaSortKey *alpha_key = alpha_list->GetHead();
                eDmaDataList *dma_data_list = alpha_key->DmaDataList;

                while (alpha_key) {
                    eActiveTexture *active_texture = alpha_key->ActiveTexture;
                    TextureInfo *texture_info = active_texture->pTextureInfo;
                    int alpha_usage_type = texture_info->AlphaUsageType;
                    int write_z = texture_info->IsZWrite();
                    int bias_level = texture_info->BiasLevel;

                    if (bias_level != previous_bias_level) {
                        eRenderTargetMatrixPack *matrix_pack;

                        if (bias_level == 1) {
                            matrix_pack = matrix_pack1;
                        } else {
                            matrix_pack = bias_level == 2 ? matrix_pack2 : matrix_pack0;
                        }
                        dma_data_list->SetMatrixPack(matrix_pack);
                        previous_bias_level = bias_level;
                    }

                    if (active_texture != previous_active_texture) {
                        AttachTextureEnvDirect(dma_chain, texture_info, 1);
                        previous_active_texture = active_texture;
                    }

                    int context_flag = alpha_usage_type != TEXUSAGE_PUNCHTHRU;
                    int writez_context_flag = write_z ? DRAW_CONTEXT_WRITE_Z : 0;

                    context_flag |= writez_context_flag;
                    context_flag &= global_z_write_mask;
                    if (texture_info->GetPlatInfo()->Special == 15) {
                        context_flag = DRAW_CONTEXT_SPECIAL;
                    }

                    if (context_flag != previous_context_flag) {
                        previous_context_flag = context_flag;
                        alpha_key->DmaDataList->AddDataHead(1, render_target, 0, previous_context_flag);
                    }

                    AttachDmaDataListMain(dma_chain, alpha_key->DmaDataList, &dma_state);
                    alpha_key = alpha_key->GetNext();
                }
            }
        }
    } else {
        static int last_warning;

        if (!last_warning || RealTimeFrames - last_warning > 60) {
            last_warning = RealTimeFrames;
        }
    }

    TextureDownloader.DeleteDownloadedTextures(last_active_texture);
    bGetTicker();

    for (int i = 0; i < 21; i++) {
        RenderTargetAlphaLists[i].Flush();
    }
}

// Las texturas solidas: se ordenan y se van bajando a la VRAM por tandas (doble buffer); detras de cada tanda van las
// listas de DMA de sus texturas en cada render target.
void AttachSolidPolys(eDmaChain *dma_chain, eDmaChain *tex_chain) {
    TextureDownloader.AddTexturesSorted(&SolidTextureList);

    DMAState dma_state;
    int global_z_write_mask = ~DRAW_CONTEXT_WRITE_Z;

    if (eGetZWriteState()) {
        global_z_write_mask = -1;
    }

    while (!TextureDownloader.IsFinishedDownloading()) {
        eActiveTexture *active_texture = TextureDownloader.GetFirstTexture();
        eActiveTexture *last_active_texture = TextureDownloader.DownloadDoubleBuffered(dma_chain, tex_chain, false);

        while (last_active_texture && active_texture != last_active_texture) {
            TextureInfo *texture_info = active_texture->pTextureInfo;
            int alpha_usage_type = texture_info->AlphaUsageType != TEXUSAGE_PUNCHTHRU;
            int bias_level = texture_info->BiasLevel;
            int writez_context_flag = texture_info->IsZWrite() ? DRAW_CONTEXT_WRITE_Z : 0;
            int context_flag = alpha_usage_type | writez_context_flag;

            context_flag &= global_z_write_mask;
            if (texture_info->GetPlatInfo()->Special == 15) {
                context_flag = DRAW_CONTEXT_SPECIAL;
            }

            for (int i = 0; i < 21; i++) {
                eDmaDataList *dma_data_list = active_texture->GetDmaDataList(i, 0);

                if (dma_data_list && !dma_data_list->IsEmpty()) {
                    eRenderTarget *render_target = eGetRenderTarget(i);
                    eRenderTargetMatrixPack *matrix_pack;

                    if (bias_level == 1) {
                        matrix_pack = render_target->MatrixPack1;
                    } else if (bias_level == 2) {
                        matrix_pack = render_target->MatrixPack2;
                    } else {
                        matrix_pack = render_target->MatrixPack0;
                    }
                    dma_data_list->SetMatrixPack(matrix_pack);
                    dma_data_list->AddDataHead(1, render_target, 0, context_flag);

                    AttachTextureEnvDirect(dma_chain, texture_info, 1);
                    AttachDmaDataListMain(dma_chain, dma_data_list, &dma_state);
                }
            }
            active_texture = active_texture->GetNext();
        }
        TextureDownloader.DeleteDownloadedTextures(last_active_texture);
    }
}

// Manda las listas de DMA de las texturas [first, last) sin bajar texturas (ya estan en la VRAM): para cada render
// target, la matriz y el contexto de dibujo de la textura y despues sus datos.
void AttachAllPolysNoTexUpload(eActiveTexture *first_active_texture, eActiveTexture *last_active_texture,
                               eDmaChain *dma_chain, eDmaChain *tex_chain) {
    DMAState dma_state;
    int global_z_write_mask = ~DRAW_CONTEXT_WRITE_Z;

    if (eGetZWriteState()) {
        global_z_write_mask = -1;
    }

    for (eActiveTexture *active_texture = first_active_texture; active_texture != last_active_texture;
         active_texture = active_texture->GetNext()) {
        TextureInfo *texture_info = active_texture->pTextureInfo;
        int alpha_usage_type = texture_info->AlphaUsageType != TEXUSAGE_PUNCHTHRU;
        int bias_level = texture_info->BiasLevel;
        int writez_context_flag = texture_info->IsZWrite() ? DRAW_CONTEXT_WRITE_Z : 0;
        int context_flag = alpha_usage_type | writez_context_flag;

        context_flag &= global_z_write_mask;

        for (int i = 0; i < 21; i++) {
            eDmaDataList *dma_data_list = active_texture->GetDmaDataList(i, 0);

            if (dma_data_list && !dma_data_list->IsEmpty()) {
                eRenderTarget *render_target = eGetRenderTarget(i);
                eRenderTargetMatrixPack *matrix_pack;

                if (bias_level == 1) {
                    matrix_pack = render_target->MatrixPack1;
                } else if (bias_level == 2) {
                    matrix_pack = render_target->MatrixPack2;
                } else {
                    matrix_pack = render_target->MatrixPack0;
                }
                dma_data_list->SetMatrixPack(matrix_pack);
                dma_data_list->AddDataHead(1, render_target, 0, context_flag);

                AttachTextureEnvDirect(dma_chain, texture_info, 1);
                AttachDmaDataListMain(dma_chain, dma_data_list, &dma_state);
            }
        }
    }

    TextureDownloader.DeleteDownloadedTextures(last_active_texture);
}

// La profundidad del centro de una caja en espacio de vista (solo z; el resto a cero).
void GetAABBZDepth(bVector4 *zdepths, bVector4 *aabb_min, bVector4 *aabb_max, bMatrix4 *local_world,
                   bMatrix4 *world_view) {
    bVector4 center;
    bVector4 out;

    center.x = (aabb_min->x + aabb_max->x) * 0.5f;
    center.y = (aabb_min->y + aabb_max->y) * 0.5f;
    center.z = (aabb_min->z + aabb_max->z) * 0.5f;
    center.w = 1.0f;

    eMulVector(&center, local_world, &center);
    eMulVector(&out, world_view, &center);

    zdepths->x = out.z;
    zdepths->y = 0.0f;
    zdepths->z = 0.0f;
    zdepths->w = 0.0f;
}

int bAutoK = 1;
float globmod[2] = {1.0f, 1.0f};
float ZRanges[2][6] = {
    {2.00000381f, 3.0f, 2.00001526f, 3.01999998f, 2.00001526f, 3.0207479f},
    {2.00000381f, 4.0f, 2.00001526f, 4.00390625f, 4.5f, 7.5f},
};
int PrintScreenClip = 0;
int zxxyz = 1;
unsigned char nRadialAlphaCenter[2] = {0x00, 0x00};
unsigned char nRadialAlphaEdge[2] = {0xc0, 0xc0};
float FlailerLayerToZConversionTable[29] = {10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f,
                                            20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 5.0f,  6.0f,  7.0f,  8.0f,
                                            5.0f,  4.0f,  3.0f,  2.0f,  1.0f,  1.0f,  2.0f,  3.0f,  4.0f};

// Las matrices de proyeccion de la vista: la de vista a pantalla (con la escala de la pantalla o, con bAutoK, dividida
// por h), las dos de las otras capas de z (vs1 y vs2), la de recorte con banda de guarda y, para las vistas de los
// jugadores, la matriz de clip a textura de la VU1 de este frame.
void CreateViewMatricies(eView *view, bMatrix4 *view_to_screen, bMatrix4 *view_clipgb, bMatrix4 *clip_to_screen0,
                         bMatrix4 *clip_to_screen1, bMatrix4 *clip_to_screen2, float force_near_z, float force_far_z,
                         float force_screen_far_z) {
    Camera *camera = view->GetCamera();
    eRenderTarget *render_target = view->GetRenderTarget();
    int nId = view->GetID();
    float nearz = camera->GetNearZ();
    float farz = camera->GetFarZ();
    float fFovBias = bTan(aBaselineFovMip >> 1) / bTan(camera->GetFieldOfView() >> 1);
    float h;
    float ax;
    float ay;
    float cx;
    float cy;
    float zmin0;
    float zmax0;
    float zmin1;
    float zmax1;
    float zmin2;
    float zmax2;
    float cz0;
    float az0;
    float cz1;
    float az1;
    float cz2;
    float az2;
    bMatrix4 vs1;
    bMatrix4 vs2;
    float screen_x;
    float screen_y;
    float clipgb_volume_x;
    float clipgb_volume_y;

    render_target->FovBias = fFovBias;

    if (force_near_z != 0.0f) {
        nearz = force_near_z;
    }

    if (force_far_z != 0.0f) {
        farz = force_far_z;
    }

    view->NearZ = nearz;
    view->FarZ = farz;

    h = CalculateH(camera->GetFieldOfView(), view);
    view->H = h;

    view->FovDegrees = bAngToDeg(camera->GetFieldOfView());
    view->ViewDirection = *camera->GetDirection();

    ax = render_target->AspectRatioX;
    ay = render_target->AspectRatioY;
    cx = CenterX;
    cy = CenterY;
    int zrange = render_target->ZBufferMode == PSMZ24;

    zmin0 = ZRanges[zrange][0] * globmod[zrange];
    zmax0 = ZRanges[zrange][1] * globmod[zrange];
    zmin1 = ZRanges[zrange][2] * globmod[zrange];
    zmax1 = ZRanges[zrange][3] * globmod[zrange];
    zmin2 = ZRanges[zrange][4] * globmod[zrange];
    zmax2 = ZRanges[zrange][5] * globmod[zrange];

    cz0 = (-zmax0 * nearz + zmin0 * farz) / (-nearz + farz);
    az0 = farz * nearz * (zmax0 - zmin0) / (-nearz + farz);

    cz1 = (-zmax1 * nearz + zmin1 * farz) / (-nearz + farz);
    az1 = farz * nearz * (zmax1 - zmin1) / (-nearz + farz);

    cz2 = (-zmax2 * nearz + zmin2 * farz) / (-nearz + farz);
    az2 = farz * nearz * (zmax2 - zmin2) / (-nearz + farz);

    if (bAutoK) {
        view_to_screen->v0.x = ax; view_to_screen->v0.y = 0.0f; view_to_screen->v0.z = 0.0f; view_to_screen->v0.w = 0.0f;
        view_to_screen->v1.x = 0.0f; view_to_screen->v1.y = ay; view_to_screen->v1.z = 0.0f; view_to_screen->v1.w = 0.0f;
        view_to_screen->v2.x = cx / h; view_to_screen->v2.y = cy / h; view_to_screen->v2.z = cz0 / h; view_to_screen->v2.w = 1.0f / h;
        view_to_screen->v3.x = 0.0f; view_to_screen->v3.y = 0.0f; view_to_screen->v3.z = az0 / h; view_to_screen->v3.w = 0.0f;

        vs1.v0.x = ax; vs1.v0.y = 0.0f; vs1.v0.z = 0.0f; vs1.v0.w = 0.0f;
        vs1.v1.x = 0.0f; vs1.v1.y = ay; vs1.v1.z = 0.0f; vs1.v1.w = 0.0f;
        vs1.v2.x = cx / h; vs1.v2.y = cy / h; vs1.v2.z = cz1 / h; vs1.v2.w = 1.0f / h;
        vs1.v3.x = 0.0f; vs1.v3.y = 0.0f; vs1.v3.z = az1 / h; vs1.v3.w = 0.0f;

        vs2.v0.x = ax; vs2.v0.y = 0.0f; vs2.v0.z = 0.0f; vs2.v0.w = 0.0f;
        vs2.v1.x = 0.0f; vs2.v1.y = ay; vs2.v1.z = 0.0f; vs2.v1.w = 0.0f;
        vs2.v2.x = cx / h; vs2.v2.y = cy / h; vs2.v2.z = cz2 / h; vs2.v2.w = 1.0f / h;
        vs2.v3.x = 0.0f; vs2.v3.y = 0.0f; vs2.v3.z = az2 / h; vs2.v3.w = 0.0f;
    } else {
        view_to_screen->v0.x = ax * h; view_to_screen->v0.y = 0.0f; view_to_screen->v0.z = 0.0f; view_to_screen->v0.w = 0.0f;
        view_to_screen->v1.x = 0.0f; view_to_screen->v1.y = ay * h; view_to_screen->v1.z = 0.0f; view_to_screen->v1.w = 0.0f;
        view_to_screen->v2.x = cx; view_to_screen->v2.y = cy; view_to_screen->v2.z = cz0; view_to_screen->v2.w = 1.0f;
        view_to_screen->v3.x = 0.0f; view_to_screen->v3.y = 0.0f; view_to_screen->v3.z = az0; view_to_screen->v3.w = 0.0f;

        vs1.v0.x = ax * h; vs1.v0.y = 0.0f; vs1.v0.z = 0.0f; vs1.v0.w = 0.0f;
        vs1.v1.x = 0.0f; vs1.v1.y = ay * h; vs1.v1.z = 0.0f; vs1.v1.w = 0.0f;
        vs1.v2.x = cx; vs1.v2.y = cy; vs1.v2.z = cz1; vs1.v2.w = 1.0f;
        vs1.v3.x = 0.0f; vs1.v3.y = 0.0f; vs1.v3.z = az1; vs1.v3.w = 0.0f;

        vs2.v0.x = ax * h; vs2.v0.y = 0.0f; vs2.v0.z = 0.0f; vs2.v0.w = 0.0f;
        vs2.v1.x = 0.0f; vs2.v1.y = ay * h; vs2.v1.z = 0.0f; vs2.v1.w = 0.0f;
        vs2.v2.x = cx; vs2.v2.y = cy; vs2.v2.z = cz2; vs2.v2.w = 1.0f;
        vs2.v3.x = 0.0f; vs2.v3.y = 0.0f; vs2.v3.z = az2; vs2.v3.w = 0.0f;
    }

    screen_x = render_target->ScissorW;
    screen_y = render_target->ScissorH;

    if (nId == EVIEW_PLAYER1 || nId == EVIEW_PLAYER2 || nId == EVIEW_PLAYER1_GLOW || nId == EVIEW_PLAYER2_GLOW ||
        nId == EVIEW_PLAYER1_REFLECTION || nId == EVIEW_PLAYER2_REFLECTION || nId == EVIEW_PLAYER1_SPECULAR ||
        nId == EVIEW_PLAYER2_SPECULAR) {
        clipgb_volume_x = screen_x * ClipGBVolumeScaleXDisp;
    } else {
        clipgb_volume_x = screen_x * ClipGBVolumeScaleX;
    }

    clipgb_volume_y = screen_y * ClipGBVolumeScaleY;

    if (view_clipgb) {
        bMatrix4 c;
        float gsx = nearz * clipgb_volume_x / h;
        float gsy = nearz * clipgb_volume_y / h;

        bIdentity(&c);

        c.v0.x = (nearz + nearz) / (gsx + gsx);
        c.v1.y = (nearz + nearz) / (gsy + gsy);
        c.v2.z = (farz + nearz) / (farz - nearz);
        c.v2.w = 1.0f;
        c.v3.z = farz * nearz * -2.0f / (farz - nearz);
        c.v3.w = 0.0f;

        *view_clipgb = c;

        if (clip_to_screen0 || clip_to_screen1) {
            bMatrix4 ic;

            bIdentity(&ic);

            ic.v0.x = 1.0f / c.v0.x;
            ic.v1.y = 1.0f / c.v1.y;
            ic.v2.z = 0.0f;
            ic.v2.w = 1.0f / c.v3.z;
            ic.v3.z = 1.0f;
            ic.v3.w = -c.v2.z / c.v3.z;

            if (view->GetID() == EVIEW_PLAYER1) {
                int buffer = eFrameCounter & 1;
                bMatrix4 clip_to_world;
                bMatrix4 view_to_world;
                bMatrix4 world_view = *camera->GetCameraMatrix();

                fInvertMatrix(&view_to_world, &world_view);
                eMulMatrix(&clip_to_world, &ic, &view_to_world);
                eMulMatrix(&VU1clipToTexMatrix[buffer][0], &clip_to_world, &VU1textureModMatrix[buffer][1]);
            }

            if (view->GetID() == EVIEW_PLAYER2) {
                int buffer = eFrameCounter & 1;
                bMatrix4 clip_to_world;
                bMatrix4 view_to_world;
                bMatrix4 world_view = *camera->GetCameraMatrix();

                fInvertMatrix(&view_to_world, &world_view);
                eMulMatrix(&clip_to_world, &ic, &view_to_world);
                eMulMatrix(&VU1clipToTexMatrix[buffer][1], &clip_to_world, &VU1textureModMatrix[buffer][3]);
            }

            if (clip_to_screen0) {
                eMulMatrix(clip_to_screen0, &ic, view_to_screen);
            }

            if (clip_to_screen1) {
                eMulMatrix(clip_to_screen1, &ic, &vs1);
            }

            if (clip_to_screen2) {
                eMulMatrix(clip_to_screen2, &ic, &vs2);
            }
        }
    }

    eMulMatrix(view_to_screen, view_clipgb, clip_to_screen0);
}

// La imagen del retrovisor y su mascara, encima de la vista del espejo.
void OverlayRVMImages() {
    TextureInfo *player1_rvm_info = pTextureInfoRVMInfo;
    TextureInfo *player1_rvm_mask_info = pTextureInfoRVMMask;

    if (!player1_rvm_info || !player1_rvm_mask_info) {
        return;
    }

    player1_rvm_mask_info->AlphaBlendType = TEXBLEND_BLEND;

    ePoly poly;

    poly.SetFlags(poly.GetFlags() ^ 1);

    poly.UVs[0][0] = 1.0f; poly.UVs[0][1] = 0.0f;
    poly.UVs[1][0] = 0.0f; poly.UVs[1][1] = 0.0f;
    poly.UVs[2][0] = 0.0f; poly.UVs[2][1] = 1.0f;
    poly.UVs[3][0] = 1.0f; poly.UVs[3][1] = 1.0f;

    *reinterpret_cast<uint32 *>(poly.Colours[0]) = 0x80808080;
    *reinterpret_cast<uint32 *>(poly.Colours[1]) = 0x80808080;
    *reinterpret_cast<uint32 *>(poly.Colours[2]) = 0x80808080;
    *reinterpret_cast<uint32 *>(poly.Colours[3]) = 0x80808080;

    for (int id = EVIEW_PLAYER1_RVM; id <= EVIEW_PLAYER1_RVM; id++) {
        eView *view = &eViews[id];
        eRenderTarget *render_target = view->GetRenderTarget();

        if (view->IsActive() && render_target->IsActive()) {
            float zmax = render_target->ZMax;

            poly.Vertices[0].x = 0.0f; poly.Vertices[0].y = 0.0f;
            poly.Vertices[1].x = 256.0f; poly.Vertices[1].y = 0.0f;
            poly.Vertices[2].x = 256.0f; poly.Vertices[2].y = 64.0f;
            poly.Vertices[3].x = 0.0f; poly.Vertices[3].y = 64.0f;

            poly.Vertices[0].z = zmax;
            poly.Vertices[1].z = zmax;
            poly.Vertices[2].z = zmax;
            poly.Vertices[3].z = zmax;

            view->Render(&poly, player1_rvm_info, 0);

            poly.Vertices[0].z = zmax;
            poly.Vertices[1].z = zmax;
            poly.Vertices[2].z = zmax;
            poly.Vertices[3].z = zmax;

            view->Render(&poly, player1_rvm_mask_info, 0);
        }
    }
}

// El retrovisor: la imagen de la vista del espejo en un rectangulo arriba de la pantalla (mas ancho en panoramico y
// mas alto en PAL).
void DisplayRVMs(eView *view) {
    float zmax = view->GetRenderTarget()->ZMax - zxxyz;
    ePoly poly;

    poly.SetFlags(poly.GetFlags() ^ 1);

    poly.UVs[0][0] = 1.0f; poly.UVs[0][1] = 0.0f;
    poly.UVs[1][0] = 0.0f; poly.UVs[1][1] = 0.0f;
    poly.UVs[2][0] = 0.0f; poly.UVs[2][1] = 1.0f;
    poly.UVs[3][0] = 1.0f; poly.UVs[3][1] = 1.0f;

    uint32 rgba = 0x80808080;

    *reinterpret_cast<uint32 *>(poly.Colours[0]) = rgba;
    *reinterpret_cast<uint32 *>(poly.Colours[1]) = rgba;
    *reinterpret_cast<uint32 *>(poly.Colours[2]) = rgba;
    *reinterpret_cast<uint32 *>(poly.Colours[3]) = rgba;

    int32 tRVMsw;
    int32 tRVMsx;

    if (FEDatabase->GetUserProfile(0)->GetOptions()->TheVideoSettings.WideScreen) {
        tRVMsw = 154;
        tRVMsx = 180;
    } else {
        tRVMsx = 154;
        tRVMsw = 206;
    }

    int32 tRVMsy = 20;
    int32 tRVMsh = 63;

    if (GetVideoMode() == MODE_PAL) {
        tRVMsy = 23;
        tRVMsh = 72;
    }

    for (int id = EVIEW_PLAYER1_RVM; id <= EVIEW_PLAYER1_RVM; id++) {
        eView *rvm_view = &eViews[id];
        eRenderTarget *rvm_target = rvm_view->GetRenderTarget();

        if (rvm_view->IsActive() && rvm_target->IsActive()) {
            TextureInfo *texture_info = rvm_target->GetTextureInfo(0);

            poly.Vertices[0].x = tRVMsx; poly.Vertices[0].y = tRVMsy; poly.Vertices[0].z = zmax;
            poly.Vertices[1].x = tRVMsx + tRVMsw; poly.Vertices[1].y = tRVMsy; poly.Vertices[1].z = zmax;
            poly.Vertices[2].x = tRVMsx + tRVMsw; poly.Vertices[2].y = tRVMsy + tRVMsh; poly.Vertices[2].z = zmax;
            poly.Vertices[3].x = tRVMsx; poly.Vertices[3].y = tRVMsy + tRVMsh; poly.Vertices[3].z = zmax;

            view->Render(&poly, texture_info, 0);
        }
    }
}

// Las bandas negras de arriba y abajo de las vistas de los jugadores cuando la camara pide letterbox.
void DisplayLetterBoxes() {
    TextureInfo *texture_info = pTextureInfoSolidGrey;

    if (!texture_info) {
        return;
    }

    ePoly poly;

    poly.SetFlags(poly.GetFlags() ^ 1);
    poly.SetFlailer(1);

    *reinterpret_cast<uint32 *>(poly.Colours[0]) = 0x80000000;
    *reinterpret_cast<uint32 *>(poly.Colours[1]) = 0x80000000;
    *reinterpret_cast<uint32 *>(poly.Colours[2]) = 0x80000000;
    *reinterpret_cast<uint32 *>(poly.Colours[3]) = 0x80000000;

    for (int id = EVIEW_FIRST_PLAYER; id <= EVIEW_LAST_PLAYER; id++) {
        eView *view = &eViews[id];
        eRenderTarget *render_target = view->GetRenderTarget();
        if (view->GetCamera()->GetLetterBox() > 0.0f && view->IsActive() && render_target->IsActive()) {
            int x = render_target->ScissorX;
            int y = render_target->ScissorY;
            int w = render_target->ScissorW + 2;
            int h = render_target->ScissorH;
            int bar = bFloatToInt(h * view->GetCamera()->GetLetterBox());
            float zmax = 1.0f;

            poly.Vertices[0].x = x; poly.Vertices[0].y = y; poly.Vertices[0].z = zmax;
            poly.Vertices[1].x = x + w; poly.Vertices[1].y = y; poly.Vertices[1].z = zmax;
            poly.Vertices[2].x = x + w; poly.Vertices[2].y = y + bar; poly.Vertices[2].z = zmax;
            poly.Vertices[3].x = x; poly.Vertices[3].y = y + bar; poly.Vertices[3].z = zmax;
            view->Render(&poly, texture_info, 0);

            poly.Vertices[0].x = x; poly.Vertices[0].y = y + h - bar; poly.Vertices[0].z = zmax;
            poly.Vertices[1].x = x + w; poly.Vertices[1].y = y + h - bar; poly.Vertices[1].z = zmax;
            poly.Vertices[2].x = x + w; poly.Vertices[2].y = y + h; poly.Vertices[2].z = zmax;
            poly.Vertices[3].x = x; poly.Vertices[3].y = y + h; poly.Vertices[3].z = zmax;
            view->Render(&poly, texture_info, 0);
        }
    }
}

void SetupSceneryCullInfoPlat(eView *view, SceneryCullInfo &info) {
    bCopy(&info.ClipMatrix, view->GetPlatInfo()->GetWorldClipGBMatrix());
    info.pView = view;
}

ePoly::ePoly() {
    UVs[0][0] = 0.0f; UVs[0][1] = 0.0f;
    UVs[1][0] = 1.0f; UVs[1][1] = 0.0f;
    UVs[2][0] = 1.0f; UVs[2][1] = 1.0f;
    UVs[3][0] = 0.0f; UVs[3][1] = 1.0f;

    SetFlags(1);
    SetFlailer(0);
}

// Cuelga un paquete ya hecho de la lista de DMA de su textura en el render target actual, o de una clave de la lista
// de alfa con la z que se le pasa.
void eAttachDirectData(TextureInfo *texture_info, float sort_z, void *data, unsigned short qwsize) {
    if (texture_info && data && qwsize) {
        int render_target_id = eGetCurrentRenderTarget()->GetID();
        eActiveTexture *active_texture = eGetActiveTexture(texture_info);
        eDmaDataList *dma_data_list = active_texture->GetDmaDataList(render_target_id, 1);

        if (texture_info->ApplyAlphaSorting) {
            eAlphaSortKey *sort_key = eNewAlphaSortKey();

            if (sort_key) {
                sort_key->Z = sort_z;
                sort_key->ActiveTexture = active_texture;
                dma_data_list = sort_key->DmaDataList;
                RenderTargetAlphaLists[render_target_id].AddKey(sort_key);
            } else {
                dma_data_list = nullptr;
            }
        }

        if (dma_data_list) {
            dma_data_list->AddData(4, data, qwsize, 0);
        }
    }
}

// Abre un strip inmediato: reserva el paquete de la VU1 (cabecera, vertices, colores y UV, cada bloque con su UNPACK)
// y deja los punteros de escritura de eAddVertex, eAddColour y eAddUV al principio de cada bloque.
bool eBeginStrip(TextureInfo *texture_info, int nVert, bMatrix4 *local_to_world) {
    int adjVert = (nVert + 3) & ~3;
    int uv_size_32 = adjVert * 2;
    int vertex_size_32 = adjVert * 3;
    int header_size_32 = 11;
    int total_size_32 = uv_size_32 + vertex_size_32 + adjVert;
    int num_loops = 0;
    int light_mat = -3;

    total_size_32 = (total_size_32 + header_size_32 + 4 + 3) & ~3;
    es_qwsize = total_size_32 >> 2;
    es_data_start = static_cast<uint32 *>(eFrameMalloc(es_qwsize << 4));
    if (!es_data_start) {
        return false;
    }
    es_data_end = es_data_start + total_size_32;


    es_data_start[0] = VIF_CODE(UNPACK | V4_8, 1, VUNPACK_TOPS);
    es_data_start[1] = (light_mat << 24) | (num_loops << 8) | nVert;
    es_data_start[2] = VIF_CODE(UNPACK, 1, VUNPACK_TOPS | VUNPACK_UNSIGNED | 1);
    es_data_start[3] = 0;
    es_data_start[4] = VIF_CODE(UNPACK | V3_32, 2, VUNPACK_TOPS | VUNPACK_UNSIGNED | 2);
    es_data_start[5] = 0;
    es_data_start[6] = 0;
    es_data_start[7] = 0;
    es_data_start[8] = 0;
    es_data_start[9] = 0;
    es_data_start[10] = 0;

    es_data_vertex = es_data_start + header_size_32;
    *es_data_vertex++ = VIF_CODE(UNPACK | V4_32, (adjVert / 4) * 3, VUNPACK_TOPS | 5);
    es_data_vertex_top = es_data_vertex;

    es_data_colour = es_data_vertex + vertex_size_32;
    *es_data_colour++ = VIF_CODE(UNPACK | V4_8, adjVert, VUNPACK_TOPS | 45);

    es_data_uv = es_data_colour + adjVert;
    *es_data_uv++ = VIF_CODE(UNPACK | V4_32, adjVert / 2, VUNPACK_TOPS | 29);

    es_data_bottom = es_data_uv + uv_size_32;

    es_local_to_world = local_to_world;
    es_texture_info = texture_info;
    es_vertex_count = 0;
    es_uv_flip = 0;

    return true;
}

// Cierra el strip: rellena el paquete hasta el final con NOP de la VIF (detras del MSCAL) y lo cuelga de la lista de
// DMA de su textura, o de una clave de la lista de alfa del render target (ordenada por la z del primer vertice).
bool eEndStrip(eView *view) {
    *es_data_bottom++ = VIF_CODE(MSCAL, 0, 0);

    while (es_data_bottom < es_data_end) {
        *es_data_bottom++ = 0;
    }

    bMatrix4 *world_view = view->GetCamera()->GetCameraMatrix();
    int render_target_id = eGetCurrentRenderTarget()->GetID();
    eActiveTexture *active_texture = eGetActiveTexture(es_texture_info);
    eDmaDataList *dma_data_list = active_texture->GetDmaDataList(render_target_id, 1);

    if (es_texture_info->ApplyAlphaSorting) {
        eAlphaSortKey *sort_key = eNewAlphaSortKey();

        if (sort_key) {
            bVector4 zdepth;

            zdepth.x = reinterpret_cast<float *>(es_data_vertex_top)[0];
            zdepth.y = reinterpret_cast<float *>(es_data_vertex_top)[4];
            zdepth.z = reinterpret_cast<float *>(es_data_vertex_top)[8];
            zdepth.w = 1.0f;
            eMulVector(&zdepth, world_view, &zdepth);

            sort_key->Z = zdepth.z;
            sort_key->ActiveTexture = active_texture;
            dma_data_list = sort_key->DmaDataList;
            RenderTargetAlphaLists[render_target_id].AddKey(sort_key);
        } else {
            dma_data_list = nullptr;
        }
    }

    if (dma_data_list) {
        dma_data_list->SetLocalWorldMatrix(es_local_to_world);
        dma_data_list->AddData(5, es_data_start, es_qwsize, 0);
        return true;
    }
    return false;
}

// Los vertices van de cuatro en cuatro por componentes (x0..x3, y0..y3, z0..z3), y la caja del strip (minimos y
// maximos, a partir del sexto float de la cabecera del paquete) se va ampliando.
void eAddVertex(float x, float y, float z) {
    float *aabb_base = reinterpret_cast<float *>(es_data_start);
    float min_x = aabb_base[5], min_y = aabb_base[6], min_z = aabb_base[7];
    float max_x = aabb_base[8], max_y = aabb_base[9], max_z = aabb_base[10];

    if (x < min_x) {
        min_x = x;
    }
    if (y < min_y) {
        min_y = y;
    }
    if (z < min_z) {
        min_z = z;
    }
    if (x > max_x) {
        max_x = x;
    }
    if (y > max_y) {
        max_y = y;
    }
    if (z > max_z) {
        max_z = z;
    }

    aabb_base[5] = min_x;
    aabb_base[6] = min_y;
    aabb_base[7] = min_z;
    aabb_base[8] = max_x;
    aabb_base[9] = max_y;
    aabb_base[10] = max_z;

    float *p_x = reinterpret_cast<float *>(es_data_vertex + es_vertex_count);
    float *p_y = p_x + 4;
    float *p_z = p_x + 8;

    *p_x = x;
    *p_y = y;
    *p_z = z;

    if (++es_vertex_count >= 4) {
        es_vertex_count = 0;
        es_data_vertex += 12;
    }
}

void eAddVertex(const bVector3 &v) {
    eAddVertex(v.x, v.y, v.z);
}

void eAddColour(unsigned int c) {
    *es_data_colour++ = c;
}

void eAddUV(float u, float v) {
    if (!es_uv_flip) {
        *reinterpret_cast<float *>(es_data_uv++) = u;
        *reinterpret_cast<float *>(es_data_uv++) = v;
    } else {
        *reinterpret_cast<float *>(es_data_uv++) = v;
        *reinterpret_cast<float *>(es_data_uv++) = u;
    }
    es_uv_flip ^= 1;
}

typedef struct {
    // total size: 0x18
    float position[3]; // offset 0x0, size 0xC
    uint32 colour;     // offset 0xC, size 0x4
    float uv[2];       // offset 0x10, size 0x8
} exVertexData;

exVertexData ex_vertices[96];
TextureInfo *ex_texture_info;
int ex_num_verts;
bMatrix4 *ex_matrix;
int ex_vnum;
int ex_cnum;
int ex_tnum;

// Las versiones de puntero de eAddColour y eAddUV que usa exEndStrip (inline; el original no las emite aparte).
inline void _eAddColour(uint32 c) {
    *es_data_colour++ = c;
}

inline void _eAddUV(const float *uv) {
    *reinterpret_cast<float *>(es_data_uv++) = uv[es_uv_flip];
    es_uv_flip ^= 1;
    *reinterpret_cast<float *>(es_data_uv++) = uv[es_uv_flip];
}

bool exBeginStrip(TextureInfo *texture_info, int nVert, bMatrix4 *local_to_world) {
    if (nVert > 96) {
        return false;
    }

    ex_texture_info = texture_info;
    ex_num_verts = nVert;
    ex_matrix = local_to_world;

    ex_vnum = ex_cnum = ex_tnum = 0;

    return true;
}

int ex_batch_max = 5;

// Parte el strip guardado en tandas de como mucho ex_batch_max vertices; cada tanda empieza dos vertices antes de
// donde acabo la anterior, para que el strip siga sin cortes.
bool exEndStrip(eView *view) {
    int i;
    int n;
    exVertexData *pVert;
    int num_verts = ex_vnum;
    int next_vert = 0;
    int diff;
    bool rval = true;

    while (true) {
        bool bDone;

        diff = num_verts - next_vert;
        n = diff > ex_batch_max ? ex_batch_max : diff;
        bDone = false;
        if (next_vert + n > num_verts) {
            bDone = true;
            n -= next_vert + n - num_verts;
        }
        if (n < 3) {
            break;
        }

        if (eBeginStrip(ex_texture_info, n, ex_matrix)) {
            pVert = &ex_vertices[next_vert];
            for (i = 0; i < n; i++) {
                _eAddVertex(pVert->position);
                _eAddColour(pVert->colour);
                _eAddUV(pVert->uv);
                pVert++;
            }
            if (!eEndStrip(view)) {
                rval = false;
            }
        }
        next_vert += n - 2;
        if (bDone || diff == ex_batch_max) {
            break;
        }
    }

    return rval;
}

void exAddVertex(const bVector3 &v) {
    ex_vertices[ex_vnum].position[0] = v.x;
    ex_vertices[ex_vnum].position[1] = v.y;
    ex_vertices[ex_vnum].position[2] = v.z;
    ex_vnum++;
}

void exAddColour(uint32 c) {
    ex_vertices[ex_cnum].colour = c;
    ex_cnum++;
}

void exAddUV(float u, float v) {
    ex_vertices[ex_tnum].uv[0] = u;
    ex_vertices[ex_tnum].uv[1] = v;
    ex_tnum++;
}

// Un poligono de pruebas en pantalla (layout de los stabs de la A124).
class DemoPoly : public ePoly {
  public:
    // total size: 0xB0
    unsigned int RenderMe;             // offset 0x80, size 0x4
    int EnableCBV;                     // offset 0x84, size 0x4
    unsigned int TextureHash;          // offset 0x88, size 0x4
    int TextureNumber;                 // offset 0x8C, size 0x4
    int PrintTexture;                  // offset 0x90, size 0x4
    TextureInfo *NewTex;               // offset 0x94, size 0x4
    float X;                           // offset 0x98, size 0x4
    float Y;                           // offset 0x9C, size 0x4
    float W;                           // offset 0xA0, size 0x4
    float H;                           // offset 0xA4, size 0x4
    float Z;                           // offset 0xA8, size 0x4
    TextureInfo *pRenderedTextureInfo; // offset 0xAC, size 0x4

    DemoPoly();
    void Render(eView *view);
};

DemoPoly DemoPolyA;
DemoPoly DemoPolyB;

DemoPoly::DemoPoly() {
    RenderMe = 0;
    EnableCBV = 1;

    TextureHash = 0;
    TextureNumber = 0;
    PrintTexture = 0;

    NewTex = nullptr;

    X = 0.0f;
    Y = 0.0f;
    W = 640.0f;
    H = 480.0f;
    Z = 16777215.0f;

    if (this != &DemoPolyA) {
        char name[64];

        bSPrintf(name, "A_DATEPALMXQUAD");
        RenderMe = 0;
        TextureHash = bStringHash(name);
    } else {
        TextureHash = 0xbdd3acce; // bStringHash("TARGET_ENVMAP0_FULL")
    }
}

eTransferRectQueue::eTransferRectQueue() {
    for (int i = 0; i < 8; i++) {
        IdleList.AddTail(&TransferRects[i]);
    }
}

bFix MyGetFixTickerDifference(unsigned int start_ticks, unsigned int end_ticks) {
    unsigned int ticks = end_ticks - start_ticks;

    return (ticks << 6) / 293;
}

void ServiceRenderStageQueue() {
    eRenderStageQueue *queue = ActiveRenderStageQueue;
    uint32 end_time = bGetTicker();

    RenderQueueExecuting = 0;

    WaitingForBufferSwap = 1;

    queue->EndTime = end_time;
    FrameTimingStage1.TotalDrawTime = MyGetFixTickerDifference(queue->StartTime, queue->EndTime);
    VifTime = FrameTimingStage1.TotalDrawTime;

    FrameTimingStage2 = FrameTimingStage1;
}

void DumpDmaChainsToTerminal() {
    if (DumpDMAChains) {
        eRenderStageQueue *active_queue = ActiveRenderStageQueue;

        DumpDmaChain(1, &active_queue->DataDmaChain);
        DumpDmaChain(2, &active_queue->TextureDmaChain);

        DumpDMAChains = 0;
    }
}

void ExecuteRenderStageQueue(eRenderStageQueue *render_stage_queue, int swap_queues) {
    eRenderStageQueue *active_queue;

    if (swap_queues) {
        eRenderStageQueue *active_queue = ActiveRenderStageQueue;
        eRenderStageQueue *idle_queue = IdleRenderStageQueue;

        ActiveRenderStageQueue = idle_queue;
        IdleRenderStageQueue = active_queue;

        active_queue->PatchXYOffsetList.InitList();
    }

    active_queue = ActiveRenderStageQueue;

    dmaGif->chcr.TTE = 0;
    dmaVif1->chcr.TTE = 1;
    *GIF_MODE = 4;

    DumpDmaChainsToTerminal();

    RenderQueueExecuting = 1;

    active_queue->StartTime = bGetTicker();

    sceDmaSend(dmaGif, &active_queue->TextureDmaChain);
    sceDmaSend(dmaVif1, &active_queue->DataDmaChain);
}

// Espera activa al siguiente VBlank; los nop del bucle son asm de EA (el binario los tiene en esa linea). El contador lo
// sube eVblCallback desde la interrupcion y es volatil: el binario lo vuelve a leer en la condicion aunque acaba de
// leerlo (dos lw seguidos). Los stabs de PS2 no guardan el volatile (int32 en los stabs).
void WaitUntilVSync() {
    EnableInterrupts();

    int32 counter = eVBlankCounter;

    while (counter == eVBlankCounter) {
        asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
    }
}

// Espera a que la cola de render termine y a que se cambien los buffers; si pasan mas de 120 cuadros, vuelca el estado
// de las DMA de la VIF1 y del GIF.
void eWaitUntilRenderingDone() {
    if (IsWaitUntilRenderingDoneDisabled()) {
        return;
    }

    EnableInterrupts();

    bGetTicker();

    int start_frame_counter = FrameCounter;

    while (RenderQueueExecuting || WaitingForBufferSwap) {
        if (WaitingForBufferSwap && !RenderQueueExecuting && IsWaitForFrameBufferSwapDisabled()) {
            break;
        }

        if (FrameCounter - start_frame_counter > 120) {
            uint32 vif1_madr = *D1_MADR;
            uint32 vif1_tadr = *D1_TADR;
            uint32 vif1_qwc = *D1_QWC;
            uint32 gif_madr = *D2_MADR;
            uint32 gif_tadr = *D2_TADR;
            uint32 gif_qwc = *D2_QWC;

            DumpDmaChainsToTerminal();

            start_frame_counter = FrameCounter;
        }

        // asm de EA (aprobado): el binario tiene ocho lineas de fuente seguidas (EcstasyE.cpp:13174-13181) con 8 nop
        // cada una, como WaitUntilVSync; el compilador no emite nop por linea
        asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
        asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
        asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
        asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
        asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
        asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
        asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
        asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
    }
}

float GetWaitForBufferSwapTime() {
    return WaitBufferSwapTime * (1.0f / 65536.0f);
}

float GetVifTime() {
    return VifTime * (1.0f / 65536.0f);
}

volatile int32 FrameGSInterrupts = 0;
volatile int32 CurrentGSInterrupts = 0;
volatile int32 FrameVifInterrupts = 0;
volatile int32 CurrentVifInterrupts = 0;
volatile int32 NumBadVifInterupts = 0;

void eVblCallback() {
    eVBlankCounter++;

    LastVBlankTime = bGetTicker();

    CurrentHalfOffset = (*GS_CSR >> 13) & 1;

    if (WaitingForBufferSwap) {
        ePutDisplayEnvironment();

        WaitingForBufferSwap = 0;
    }

    FrameGSInterrupts = CurrentGSInterrupts; CurrentGSInterrupts = 0;
    FrameVifInterrupts = CurrentVifInterrupts; CurrentVifInterrupts = 0;
}

// La interrupcion de la VIF1 al final de la cola de render: la marca 0xabcd la pone la propia cola. El nop suelto antes
// de ExitHandler() es asm de EA (el binario lo tiene en su propia linea).
int Vif1InteruptHandler(int) {
    if (RenderQueueExecuting) {
        unsigned int vif_mark = *VIF1_MARK;

        if ((vif_mark & 0xffff) == 0xabcd) {
            ServiceRenderStageQueue();
        } else {
            NumBadVifInterupts++;
        }
    }

    CurrentVifInterrupts++;

    *VIF1_FBRST = VIF1_FBRST_STC_M;
    CPU::SyncL();
    CPU::SyncP();

    asm("nop");

    ExitHandler();
    return 0;
}

void DmaTest() {}

// Los ajustes del visor de VRAM se tocan desde el depurador: son volatiles. Lo prueba el binario: SetVRAMView lee
// vrWidth dos veces y la segunda lectura no se usa (dw), y VRAMView vuelve a leer HoldViewVRAM justo despues de
// probarlo. Los stabs de PS2 no guardan el volatile (FrameCounter es volatile en el DWARF de GameCube y sale como int32).
volatile int32 HoldViewVRAM = 0;
volatile int32 ViewVRAM = 0;
volatile int32 vrFBP = 0;
volatile GS_PSM vrPixelMode = PSMCT16;
volatile int32 vrWidth = 512;
volatile int32 vrHeight = 448;
volatile int32 vrMagV = 0;
volatile int32 vrMagH = 4;
volatile int32 vrX = 0;
volatile int32 vrY = 0;

// Ensena un trozo de la VRAM en la pantalla (depuracion): los dos circuitos leen el mismo marco, el segundo una linea
// mas abajo.
void SetVRAMView() {
    uint32 fbp = vrFBP;
    uint32 fbw = vrWidth >> 6;
    uint32 psm = vrPixelMode;
    uint32 dw = vrWidth;
    uint32 dh = ((vrHeight >> 1) << 1) - 1;

    BufferParams *buffer_params = CurrentBufferParameters;

    u_long dispfb1 = SCE_GS_SET_DISPFB(fbp, fbw, psm, vrX, vrY);
    u_long dispfb2 = SCE_GS_SET_DISPFB(fbp, fbw, psm, vrX, vrY + 1);
    u_long display1 = (u_long)buffer_params->sdx1 | ((u_long)buffer_params->sdy1 << 12) | ((u_long)vrMagH << 23) |
                      ((u_long)vrMagV << 27) | ((u_long)buffer_params->dw1 << 32) | ((u_long)dh << 44);
    u_long display2 = (u_long)buffer_params->sdx2 | ((u_long)buffer_params->sdy2 << 12) | ((u_long)vrMagH << 23) |
                      ((u_long)vrMagV << 27) | ((u_long)buffer_params->dw2 << 32) | ((u_long)(dh - 1) << 44);

    *GS_PMODE = SCE_GS_SET_PMODE(1, 1, 0, 1, 0, 0, buffer_params->blend);
    *GS_SMODE2 = SCE_GS_SET_SMODE2(1, 0, 0);

    *GS_DISPFB1 = dispfb1;
    *GS_DISPLAY1 = display1;

    *GS_DISPFB2 = dispfb2;
    *GS_DISPLAY2 = display2;

    *GS_BGCOLOR = 0;
    CPU::SyncL();
    CPU::SyncP();
}

// Mientras HoldViewVRAM este puesto, se queda aqui ensenando la VRAM y esperando cada VBlank; los nop del bucle son
// asm de EA aprobado (el binario los tiene en esa linea), como en WaitUntilVSync.
void VRAMView() {
    if (HoldViewVRAM) {
        while (HoldViewVRAM) {
            SetVRAMView();

            int32 vbl = eVBlankCounter;

            while (vbl == eVBlankCounter) {
                asm("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
            }
        }
    } else if (ViewVRAM) {
        SetVRAMView();
    }
}

int negx = 40;
int negx0 = 2;

int downsample_ud = 12;
int downsample_vd = 16;
int downsample_toggle = 0;

// Copia un render target en otro con sprites de 32x32 texeles (la altura baja a la mitad hasta que divide la del
// origen): el contexto de dibujo del destino, ALPHA, XYOFFSET, PRIM, RGBAQ, TEST y ZBUF, y dos UV/XYZ2 por sprite.
void eAttachTargetMoveSprites(eDmaChain *dma_chain, eRenderTarget *dest_target, eRenderTarget *src_target, int delta_x,
                              int delta_y, int delta_u, int delta_v, int blend_bits, int alpha, int TCC, int r, int g,
                              int b, int a, int flip, uint32 test_1) {
    int32 src_w = src_target->ScissorW;
    int32 src_h = src_target->ScissorH;

    int32 dest_w = dest_target->ScissorW;
    int32 dest_h = dest_target->ScissorH;

    uint32 SRC_sprite_w = 32;
    uint32 SRC_sprite_h = 32;

    int32 NumColumns = src_w / SRC_sprite_w;
    int32 NumRows = src_h / SRC_sprite_h;

    while (NumRows * SRC_sprite_h != src_h) {
        SRC_sprite_h >>= 1;
        NumRows = src_h / SRC_sprite_h;
    }

    eRenderTarget *Passed_src_target = src_target;

    if (src_target->RelativeToFrameBuffer) {
        src_target = MainDraw0Context;
    }

    uint32 DST_sprite_w = dest_w / NumColumns;
    uint32 DST_sprite_h = dest_h / NumRows;

    int32 QwordsSprites = NumColumns * NumRows * 4;

    AttachTextureEnvDirect(dma_chain, src_target->GetTextureInfo(0), TCC);

    uint32 qwsize = QwordsSprites + 10;
    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(qwsize * sizeof(DATA16)));
    uint32 *data = data_start;

    if (data_start) {
        *reinterpret_cast<u_long *>(data) = MakeDmaTagA(dest_target->GetDrawContextData(1), 14, eDMA_REF);
        data += 2;
        *data++ = SCE_VIF1_SET_FLUSHE(0);
        *data++ = SCE_VIF1_SET_DIRECT(14, 0);

        *reinterpret_cast<u_long *>(data) = MakeDmaTag(QwordsSprites + 7, eDMA_CNT);
        data += 2;
        *data++ = SCE_VIF1_SET_FLUSHE(0);
        *data++ = SCE_VIF1_SET_DIRECT(QwordsSprites + 7, 0);

        reinterpret_cast<u_long *>(data)[0] = MakeGifTag(QwordsSprites + 6, 1, 0, 0, GIF_PACKED, 1);
        reinterpret_cast<u_long *>(data)[1] = SCE_GIF_PACKED_AD;
        data += 4;

        *data++ = blend_bits;
        *data++ = alpha;
        *data++ = SCE_GS_ALPHA_1;
        *data++ = 0;

        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYOFFSET(negx, negx);
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYOFFSET_1;
        data += 4;

        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_PRIM(PRIM_SPRITE, 1, 1, 0, 1, 0, 1, 0, 0);
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_PRIM;
        data += 4;

        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_RGBAQ(r, g, b, a, 0x3F800000);
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_RGBAQ;
        data += 4;

        reinterpret_cast<u_long *>(data)[0] = test_1;
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_TEST_1;
        data += 4;

        reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_ZBUF(0, PSMZ16S, 1);
        reinterpret_cast<u_long *>(data)[1] = SCE_GS_ZBUF_1;
        data += 4;

        uint32 SRC_current_x = 0;
        uint32 DST_current_x = 0;

        for (int i = 0; i < NumColumns; i++) {
            uint32 SRC_current_y = Passed_src_target->ScissorY;
            uint32 DST_current_y = dest_target->ScissorY;

            for (int j = 0; j < NumRows; j++) {
                reinterpret_cast<u_long *>(data)[0] =
                    SCE_GS_SET_UV((SRC_current_x << 4) + delta_u, (SRC_current_y << 4) + delta_v);
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
                data += 4;

                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_XYZ(((DST_current_x + negx0) << 4) + delta_x,
                                                                     ((DST_current_y + negx0) << 4) + delta_y, 0xFFFF0);
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
                data += 4;

                reinterpret_cast<u_long *>(data)[0] = SCE_GS_SET_UV(((SRC_current_x + SRC_sprite_w) << 4) + delta_u,
                                                                    ((SRC_current_y + SRC_sprite_h) << 4) + delta_v);
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_UV;
                data += 4;

                reinterpret_cast<u_long *>(data)[0] =
                    SCE_GS_SET_XYZ(((DST_current_x + DST_sprite_w + negx0) << 4) + delta_x,
                                   ((DST_current_y + DST_sprite_h + negx0) << 4) + delta_y, 0xFFFF0);
                reinterpret_cast<u_long *>(data)[1] = SCE_GS_XYZ2;
                data += 4;

                SRC_current_y += SRC_sprite_h;
                DST_current_y += DST_sprite_h;
            }

            SRC_current_x += SRC_sprite_w;
            DST_current_x += DST_sprite_w;
        }

        dma_chain->Add(data_start, data_start + 4);
    }
}

void eAttachDownsample(eDmaChain *dma_chain, eRenderTarget *dest_target, eRenderTarget *src_target, int TCC) {
    eAttachTargetMoveSprites(dma_chain, dest_target, src_target, 0, 0, downsample_ud, downsample_vd, 100, 128, TCC, 128, 128,
                             128, 128, 0, SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1));
}

// Las tiras de un eSolid sin luz: cada grupo de tiras con la misma textura va como un solo paquete a la lista de DMA
// de su textura, o tira a tira a la lista de alfa (por la z del centro de su caja) si la textura se ordena.
int epRenderStripsUnlit(eView *view, eSolid *solid, bMatrix4 *local_world, eLightContext *light_context, uint32 flags,
                        bMatrix4 *blending_matrices) {
    uint16 disable_mask = 0x1A0;
    int32 render_target_id = eGetCurrentRenderTarget()->GetID();
    int32 ignore_alpha_textures;
    int32 ignore_solid_textures;
    uint16 extra_strip_flags;

    if (flags & 0x40) {
        ignore_alpha_textures = 1;
    } else {
        ignore_alpha_textures = 0;
    }

    if (flags & 0x20000) {
        extra_strip_flags = 2;
    } else {
        extra_strip_flags = 0;
    }

    if (flags & 0x20) {
        ignore_solid_textures = 1;
    } else {
        ignore_solid_textures = 0;
    }

    if (flags & 0x1000) {
        disable_mask = 0x1E0;
    }

    disable_mask = ~disable_mask;

    bMatrix4 *world_view = view->GetPlatInfo()->GetWorldViewMatrix();
    eActiveTexture *previous_active_texture = nullptr;
    eTextureEntry *texture_table = solid->pTextureTable;
    int num_strip_entries = solid->GetPlatInfo()->NumStrips;
    eStripEntry *strip_entry = solid->GetPlatInfo()->StripEntryTable;
    eStripEntry *sentinal_strip_entry = strip_entry + num_strip_entries;
    uint8 *strip_data_start = solid->GetPlatInfo()->StripDataStart;

    if (num_strip_entries == 0) {
        return 0;
    }

    while (strip_entry < sentinal_strip_entry) {
        int current_poly_group = strip_entry->PolyGroupNumber;
        int texture_index = strip_entry->TextureNumber;
        TextureInfo *texture_info = texture_table[texture_index].pTextureInfo;
        int32 apply_alpha_sort = texture_info->ApplyAlphaSorting;
        eStripEntry *last_group_strip_entry = strip_entry;

        while ((last_group_strip_entry + 1)->PolyGroupNumber == current_poly_group &&
               last_group_strip_entry + 1 < sentinal_strip_entry) {
            last_group_strip_entry++;
        }

        int draw_strip = !(ignore_alpha_textures && apply_alpha_sort);
        draw_strip &= !ignore_solid_textures || apply_alpha_sort;
        if (texture_info->NameHash == 0x66994F4A) {
            draw_strip = 0;
        }

        if (draw_strip) {
            uint8 *polygroup_data = strip_data_start + (strip_entry->DataQWordOffset << 4);
            uint32 polygroup_bytes_size =
                ((last_group_strip_entry->DataQWordOffset - strip_entry->DataQWordOffset) + last_group_strip_entry->DataQWordSize) << 4;
            uint32 polygroup_qwsize = polygroup_bytes_size >> 4;
            uint16 polygroup_parameter = strip_entry->Flags;

            polygroup_parameter |= extra_strip_flags;
            polygroup_parameter &= disable_mask;

            eActiveTexture *active_texture = eGetActiveTexture(texture_info);
            eDmaDataList *group_dma_data_list = active_texture->GetDmaDataList(render_target_id, 1);

            while (strip_entry <= last_group_strip_entry) {
                eDmaDataList *dma_data_list = nullptr;
                uint8 *dma_data = polygroup_data;
                uint32 dma_qwsize = polygroup_qwsize;

                if (apply_alpha_sort) {
                    dma_data = strip_data_start + (strip_entry->DataQWordOffset << 4);
                    dma_qwsize = strip_entry->DataQWordSize;

                    eAlphaSortKey *sort_key = eNewAlphaSortKey();

                    if (sort_key) {
                        bVector4 aabb_min;
                        bVector4 aabb_max;
                        bVector4 zdepth;

                        reinterpret_cast<eStripData *>(dma_data)->GetAABB(&aabb_min, &aabb_max);
                        GetAABBZDepth(&zdepth, &aabb_min, &aabb_max, local_world, world_view);

                        sort_key->Z = zdepth.x;
                        sort_key->ActiveTexture = active_texture;
                        RenderTargetAlphaLists[render_target_id].AddKey(sort_key);
                        dma_data_list = sort_key->DmaDataList;
                    }
                } else {
                    dma_data_list = group_dma_data_list;
                }

                if (dma_data_list) {
                    if (active_texture != previous_active_texture || apply_alpha_sort) {
                        dma_data_list->SetLocalWorldMatrix(local_world);
                        previous_active_texture = active_texture;
                    }

                    dma_data_list->AddData(EDMA_STRIP, dma_data, dma_qwsize, polygroup_parameter);
                }

                if (apply_alpha_sort) {
                    strip_entry++;
                } else {
                    strip_entry = last_group_strip_entry + 1;
                }
            }
        } else {
            strip_entry = last_group_strip_entry + 1;
        }
    }

    return 1;
}

// Las tiras de un eSolid con luz: como epRenderStripsUnlit, mas el contexto de luz, la matriz de huesos y el material
// de luz de cada grupo (el de la tabla del solido o el de por defecto).
int epRenderStripsLit(eView *view, eSolid *solid, bMatrix4 *local_world, eLightContext *light_context, uint32 flags,
                      bMatrix4 *blending_matrices) {
    uint16 disable_mask = 0;
    int32 render_target_id = eGetCurrentRenderTarget()->GetID();
    int32 ignore_alpha_textures;
    int32 ignore_solid_textures;
    uint16 extra_strip_flags = 0;

    if (blending_matrices) {
        disable_mask = 0x40;
        extra_strip_flags = 1;
    }

    if (flags & 0x40) {
        ignore_alpha_textures = 1;
    } else {
        ignore_alpha_textures = 0;
    }

    if (flags & 0x20) {
        ignore_solid_textures = 1;
    } else {
        ignore_solid_textures = 0;
    }

    if (flags & 0x80000) {
        extra_strip_flags |= 0x200;
    }

    if (flags & 0x400) {
        disable_mask |= 0x1C0;
        extra_strip_flags |= 0x20;
    } else if (flags & 0x8000) {
        extra_strip_flags |= 4;
    }

    disable_mask = ~disable_mask;

    bMatrix4 *world_view = view->GetPlatInfo()->GetWorldViewMatrix();
    eLightMaterial *default_light_material = elGetDefaultLightMaterial();
    eActiveTexture *previous_active_texture = nullptr;
    eTextureEntry *texture_table = solid->pTextureTable;
    int num_strip_entries = solid->GetPlatInfo()->NumStrips;
    eStripEntry *strip_entry = solid->GetPlatInfo()->StripEntryTable;
    eStripEntry *sentinal_strip_entry = strip_entry + num_strip_entries;
    uint8 *strip_data_start = solid->GetPlatInfo()->StripDataStart;

    if (num_strip_entries == 0) {
        return 0;
    }

    while (strip_entry < sentinal_strip_entry) {
        int current_poly_group = strip_entry->PolyGroupNumber;
        int texture_index = strip_entry->TextureNumber;
        TextureInfo *texture_info = texture_table[texture_index].pTextureInfo;
        int32 apply_alpha_sort = texture_info->ApplyAlphaSorting;

#ifdef EA_BUILD_A124
        // Solo la A124 comprueba que la textura no este corrupta (SLUS no tiene ni la cadena ni la llamada).
        if (texture_info->Width && texture_info->Width != (1 << texture_info->ShiftWidth)) {
            bReleasePrintf("TextureInfo %d (0x%08x) in solid %s has been corrupted.\n", texture_index,
                           texture_table[texture_index].NameHash, solid->Name);
            bBreak();
        }
#endif

        eStripEntry *last_group_strip_entry = strip_entry;

        while ((last_group_strip_entry + 1)->PolyGroupNumber == current_poly_group &&
               last_group_strip_entry + 1 < sentinal_strip_entry) {
            last_group_strip_entry++;
        }

        int draw_strip = !(ignore_alpha_textures && apply_alpha_sort);
        draw_strip &= !ignore_solid_textures || apply_alpha_sort;
        if (texture_info->NameHash == 0x66994F4A) {
            draw_strip = 0;
        }

        if (draw_strip) {
            uint8 *polygroup_data = strip_data_start + (strip_entry->DataQWordOffset << 4);
            uint32 polygroup_bytes_size =
                ((last_group_strip_entry->DataQWordOffset - strip_entry->DataQWordOffset) + last_group_strip_entry->DataQWordSize) << 4;
            uint32 polygroup_qwsize = polygroup_bytes_size >> 4;
            eLightMaterial *light_material = default_light_material;

            if (solid->LightMaterialTable) {
                if (strip_entry->LightMaterialIndex >= 0 && strip_entry->LightMaterialIndex < solid->NumLightMaterials) {
                    light_material = solid->LightMaterialTable[strip_entry->LightMaterialIndex].LightMaterial;
                }
            }

            uint16 polygroup_parameter = strip_entry->Flags;

            polygroup_parameter |= extra_strip_flags;
            polygroup_parameter &= disable_mask;

            eActiveTexture *active_texture = eGetActiveTexture(texture_info);
            eDmaDataList *group_dma_data_list = active_texture->GetDmaDataList(render_target_id, 1);

            while (strip_entry <= last_group_strip_entry) {
                eDmaDataList *dma_data_list = nullptr;
                uint8 *dma_data = polygroup_data;
                uint32 dma_qwsize = polygroup_qwsize;

                if (apply_alpha_sort) {
                    dma_data = strip_data_start + (strip_entry->DataQWordOffset << 4);
                    dma_qwsize = strip_entry->DataQWordSize;

                    eAlphaSortKey *sort_key = eNewAlphaSortKey();

                    if (sort_key) {
                        bVector4 aabb_min;
                        bVector4 aabb_max;
                        bVector4 zdepth;

                        reinterpret_cast<eStripData *>(dma_data)->GetAABB(&aabb_min, &aabb_max);
                        GetAABBZDepth(&zdepth, &aabb_min, &aabb_max, local_world, world_view);

                        sort_key->Z = zdepth.x;
                        sort_key->ActiveTexture = active_texture;
                        RenderTargetAlphaLists[render_target_id].AddKey(sort_key);
                        dma_data_list = sort_key->DmaDataList;
                    }
                } else {
                    dma_data_list = group_dma_data_list;
                }

                if (dma_data_list) {
                    if (active_texture != previous_active_texture || apply_alpha_sort) {
                        dma_data_list->SetLocalWorldMatrix(local_world);
                        dma_data_list->SetLightContext(light_context);

                        if (blending_matrices) {
                            dma_data_list->SetBoneMatrixPack(blending_matrices);
                        }

                        previous_active_texture = active_texture;
                    }

                    if (light_material) {
                        dma_data_list->SetLightMaterial(light_material);
                    }

                    dma_data_list->AddData(EDMA_STRIP, dma_data, dma_qwsize, polygroup_parameter);
                }

                if (apply_alpha_sort) {
                    strip_entry++;
                } else {
                    strip_entry = last_group_strip_entry + 1;
                }
            }
        } else {
            strip_entry = last_group_strip_entry + 1;
        }
    }

    return 1;
}

void eViewPlatInterface::FEBeginBatchRender(int numPolys) {}

void eViewPlatInterface::FEEndBatchRender() {}

void eViewPlatInterface::Render(eModel *model, bMatrix4 *local_to_world, eLightContext *light_context, uint32 flags,
                                bMatrix4 *blending_matricies) {
    eSolid *solid = model->GetSolid();
    bVector4 *blending_mat_3x4_ef = nullptr;

    if (!solid || !local_to_world) {
        return;
    }

    eVisibleState visibility_state = EVISIBLESTATE_PARTIAL;

    if (flags & 4) {
        visibility_state = EVISIBLESTATE_FULL;
    }

    if (visibility_state != EVISIBLESTATE_NOT) {
        if (blending_matricies) {
            int num_bones = solid->NumBones;

            blending_mat_3x4_ef = static_cast<bVector4 *>(eFrameMalloc(num_bones * sizeof(bVector4) * 3));

            if (blending_mat_3x4_ef) {
                bMatrix4 *blendMatPtr = blending_matricies;
                bVector4 *blendMatEFPtr = blending_mat_3x4_ef;

                for (int i = 0; i < num_bones; i++) {
                    bMemCpy(blendMatEFPtr, blendMatPtr, sizeof(bMatrix4));

                    blendMatEFPtr[0].w = blendMatPtr->v3.x;
                    blendMatEFPtr[1].w = blendMatPtr->v3.y;
                    blendMatEFPtr[2].w = blendMatPtr->v3.z;

                    blendMatEFPtr += 3;
                    blendMatPtr++;
                }
            }
        }

        TextureInfo **replaced_textures[128];
        model->ApplyReplacementTextureTable(replaced_textures);

        if (light_context) {
            epRenderStripsLit(reinterpret_cast<eView *>(this), solid, local_to_world, light_context, flags,
                              reinterpret_cast<bMatrix4 *>(blending_mat_3x4_ef));
        } else {
            SimpleModelAnim::Animate(model, solid, local_to_world);
            epRenderStripsUnlit(reinterpret_cast<eView *>(this), solid, local_to_world, nullptr, flags, nullptr);
        }

        model->RestoreReplacementTextureTable(replaced_textures);
    }
}


// La mascara de un poligono con mascara: el mismo paquete que Render(ePoly *, TextureInfo *, int).
void RenderSillyMask(ePoly *poly, TextureInfo *texture_info) {
    if (!poly || !texture_info) {
        return;
    }

    eRenderTarget *render_target = eGetCurrentRenderTarget();
    static eRenderTarget *last_render_target = nullptr;
    static int last_applyaspect = 0;
    static float lastAspectRatio2DX = 1.0f;
    static float aspect_ratiox;
    static float aspect_ratioy;
    static float offset_x;
    static float offset_y;
    float zmax = render_target->ZMax;
    uint32 *data_start = nullptr;
    uint32 qwsize = 13;
    uint32 *data;

    data_start = static_cast<uint32 *>(eFrameMalloc(qwsize * 16));
    data = data_start;

    if (!data_start) {
        return;
    }

    if (render_target != last_render_target || render_target->AspectRatio2DX != lastAspectRatio2DX ||
        last_applyaspect != (poly->GetFlags() & 1)) {
        lastAspectRatio2DX = render_target->AspectRatio2DX;
        last_applyaspect = poly->GetFlags() & 1;

        float screen_width2 = render_target->ScissorW >> 1;

        if (render_target->AspectRatio2DX == NTSCAspectRatio2dxWide && last_applyaspect) {
            screen_width2 *= 0.75f;
        }

        float screen_height2 = render_target->ScissorH >> 1;

        offset_x = (2048.0f - screen_width2) * 16.0f;
        offset_y = (2048.0f - screen_height2) * 16.0f;

        aspect_ratiox = last_applyaspect ? render_target->AspectRatio2DX * 16.0f : 16.0f;
        aspect_ratioy = last_applyaspect ? render_target->AspectRatio2DY * 16.0f : 16.0f;
    }

    bVector2 nn[4];
    for (int n = 0; n < 4; n++) {
        nn[n].x = poly->Vertices[n].x * aspect_ratiox + offset_x;
        nn[n].y = poly->Vertices[n].y * aspect_ratioy + offset_y;
    }

    *data++ = MakeGifTagLo(1, 1);
    *data++ = MakeGifTagHi(1, 0x5c, 0, 12);
    *data++ = 0x12412412;
    *data++ = 0x4124;

    uint32 *data2 = data;

    if (poly->GetFlailer()) {
        if (poly->GetFlags() & 2) {
            data2[10] = FloatAsInteger(poly->Vertices[0].z);
            data2[22] = FloatAsInteger(poly->Vertices[1].z);
            data2[34] = FloatAsInteger(poly->Vertices[3].z);
            data2[46] = FloatAsInteger(poly->Vertices[2].z);
        } else {
            data2[10] = bFloatToInt(zmax) << 4;
            data2[22] = bFloatToInt(zmax) << 4;
            data2[34] = bFloatToInt(zmax) << 4;
            data2[46] = bFloatToInt(zmax) << 4;
        }
    } else {
        data2[10] = bFloatToInt(poly->Vertices[0].z) << 4;
        data2[22] = bFloatToInt(poly->Vertices[1].z) << 4;
        data2[34] = bFloatToInt(poly->Vertices[3].z) << 4;
        data2[46] = bFloatToInt(poly->Vertices[2].z) << 4;
    }

    float u0 = poly->UVsMask[0][0];
    float v0 = poly->UVsMask[0][1];

    u0 *= 1.0f / 16384.0f;
    v0 *= 1.0f / 16384.0f;
    data2[0] = *reinterpret_cast<uint32 *>(&u0);
    data2[1] = *reinterpret_cast<uint32 *>(&v0);
    data2[2] = 0x3f800000;
    data2[3] = 0;
    data2[4] = poly->Colours[0][0];
    data2[5] = poly->Colours[0][1];
    data2[6] = poly->Colours[0][2];
    data2[7] = poly->Colours[0][3];
    data2[8] = bFloatToInt(nn[0].x);
    data2[9] = bFloatToInt(nn[0].y);
    data2[11] = 0x3f800000;

    float u1 = poly->UVsMask[1][0];
    float v1 = poly->UVsMask[1][1];

    u1 *= 1.0f / 16384.0f;
    v1 *= 1.0f / 16384.0f;
    data2[12] = *reinterpret_cast<uint32 *>(&u1);
    data2[13] = *reinterpret_cast<uint32 *>(&v1);
    data2[14] = 0x3f800000;
    data2[15] = 0;
    data2[16] = poly->Colours[1][0];
    data2[17] = poly->Colours[1][1];
    data2[18] = poly->Colours[1][2];
    data2[19] = poly->Colours[1][3];
    data2[20] = bFloatToInt(nn[1].x);
    data2[21] = bFloatToInt(nn[1].y);
    data2[23] = 0x3f800000;

    float u2 = poly->UVsMask[3][0];
    float v2 = poly->UVsMask[3][1];

    u2 *= 1.0f / 16384.0f;
    v2 *= 1.0f / 16384.0f;
    data2[24] = *reinterpret_cast<uint32 *>(&u2);
    data2[25] = *reinterpret_cast<uint32 *>(&v2);
    data2[26] = 0x3f800000;
    data2[27] = 0;
    data2[28] = poly->Colours[3][0];
    data2[29] = poly->Colours[3][1];
    data2[30] = poly->Colours[3][2];
    data2[31] = poly->Colours[3][3];
    data2[32] = bFloatToInt(nn[3].x);
    data2[33] = bFloatToInt(nn[3].y);
    data2[35] = 0x3f800000;

    float u3 = poly->UVsMask[2][0];
    float v3 = poly->UVsMask[2][1];

    u3 *= 1.0f / 16384.0f;
    v3 *= 1.0f / 16384.0f;
    data2[36] = *reinterpret_cast<uint32 *>(&u3);
    data2[37] = *reinterpret_cast<uint32 *>(&v3);
    data2[38] = 0x3f800000;
    data2[39] = 0;
    data2[40] = poly->Colours[2][0];
    data2[41] = poly->Colours[2][1];
    data2[42] = poly->Colours[2][2];
    data2[43] = poly->Colours[2][3];
    data2[44] = bFloatToInt(nn[2].x);
    data2[45] = bFloatToInt(nn[2].y);
    data2[47] = 0x3f800000;

    eActiveTexture *active_texture = eGetActiveTexture(texture_info);
    int32 render_target_id = render_target->GetID();
    eDmaDataList *dma_data_list = active_texture->GetDmaDataList(render_target_id, 1);
    int32 apply_alpha_sort = texture_info->ApplyAlphaSorting;

    if (apply_alpha_sort) {
        eAlphaSortKey *sort_key = eNewAlphaSortKey();

        if (sort_key) {
            float z_depth;

            if (poly->GetFlailer() && !(poly->GetFlags() & 2)) {
                z_depth = FlailerLayerToZConversionTable[bFloatToInt(poly->Vertices[0].z)] - zmax;
            } else {
                z_depth = poly->Vertices[0].z;
            }

            sort_key->Z = z_depth;
            sort_key->ActiveTexture = active_texture;
            dma_data_list = sort_key->DmaDataList;
            RenderTargetAlphaLists[render_target_id].AddKey(sort_key);
        } else {
            dma_data_list = nullptr;
        }
    }

    if (dma_data_list) {
        dma_data_list->AddData(4, data_start, qwsize, 0);
    }

    // Como al final de Render(ePoly *, TextureInfo *, int): cinco sentencias sin codigo (lineas 15316-15324 de la A124)
    // y entre ellas un bucle, que se ve en el `ld ra` de los huecos de los saltos al final.
    for (int i = 0; i < 0; i++) {
    }
}

void eViewPlatInterface::FERender(ePoly *poly, TextureInfo *texture_info, TextureInfo *texture_info_mask, int use_previous_data) {
    Render(poly, texture_info, texture_info_mask, use_previous_data);
}

void eViewPlatInterface::FERender(ePoly *poly, TextureInfo *texture_info, int use_previous_data) {
    Render(poly, texture_info, use_previous_data);
}

// Un poligono con textura: un paquete GIF de 13 quadwords (tira de cuatro vertices ST/RGBAQ/XYZ2) en la lista de DMA de
// la textura en el render target actual, o en la lista de alfa si la textura se ordena.
void eViewPlatInterface::Render(ePoly *poly, TextureInfo *texture_info, int use_previous_data) {
    static uint32 LastFrame = 0;
    static int adtc = 0;

    if (!poly || !texture_info) {
        return;
    }

    eRenderTarget *render_target = eGetCurrentRenderTarget();
    static eRenderTarget *last_render_target = nullptr;
    static int last_applyaspect = 0;
    static float lastAspectRatio2DX = 1.0f;
    static float aspect_ratiox;
    static float aspect_ratioy;
    static float offset_x;
    static float offset_y;
    float zmax = render_target->ZMax;
    uint32 *data_start = nullptr;
    uint32 qwsize = 13;
    uint32 *data;

    data_start = static_cast<uint32 *>(eFrameMalloc(qwsize * 16));
    data = data_start;

    if (!data_start) {
        return;
    }

    if (render_target != last_render_target || render_target->AspectRatio2DX != lastAspectRatio2DX ||
        last_applyaspect != (poly->GetFlags() & 1)) {
        lastAspectRatio2DX = render_target->AspectRatio2DX;
        last_applyaspect = poly->GetFlags() & 1;

        float screen_width2 = render_target->ScissorW >> 1;

        if (render_target->AspectRatio2DX == NTSCAspectRatio2dxWide && last_applyaspect) {
            screen_width2 *= 0.75f;
        }

        float screen_height2 = render_target->ScissorH >> 1;

        offset_x = (2048.0f - screen_width2) * 16.0f;
        offset_y = (2048.0f - screen_height2) * 16.0f;

        aspect_ratiox = last_applyaspect ? render_target->AspectRatio2DX * 16.0f : 16.0f;
        aspect_ratioy = last_applyaspect ? render_target->AspectRatio2DY * 16.0f : 16.0f;
    }

    bVector2 nn[4];
    for (int n = 0; n < 4; n++) {
        nn[n].x = poly->Vertices[n].x * aspect_ratiox + offset_x;
        nn[n].y = poly->Vertices[n].y * aspect_ratioy + offset_y;
    }

    *data++ = MakeGifTagLo(1, 1);
    *data++ = MakeGifTagHi(1, 0x5c, 0, 12);
    *data++ = 0x12412412;
    *data++ = 0x4124;

    uint32 *data2 = data;

    if (poly->GetFlailer()) {
#ifdef EA_BUILD_A124
        // Solo la A124 tiene la capa 2 (z medida desde zmax): SLUS no lleva esta rama.
        if (poly->GetFlailer() == 2) {
            data2[10] = bFloatToInt(zmax - poly->Vertices[0].z) << 4;
            data2[22] = bFloatToInt(zmax - poly->Vertices[1].z) << 4;
            data2[34] = bFloatToInt(zmax - poly->Vertices[3].z) << 4;
            data2[46] = bFloatToInt(zmax - poly->Vertices[2].z) << 4;
        } else
#endif
        if (poly->GetFlags() & 2) {
            data2[10] = FloatAsInteger(poly->Vertices[0].z);
            data2[22] = FloatAsInteger(poly->Vertices[1].z);
            data2[34] = FloatAsInteger(poly->Vertices[3].z);
            data2[46] = FloatAsInteger(poly->Vertices[2].z);
        } else {
            data2[10] = bFloatToInt(zmax) << 4;
            data2[22] = bFloatToInt(zmax) << 4;
            data2[34] = bFloatToInt(zmax) << 4;
            data2[46] = bFloatToInt(zmax) << 4;
        }
    } else {
        data2[10] = bFloatToInt(poly->Vertices[0].z) << 4;
        data2[22] = bFloatToInt(poly->Vertices[1].z) << 4;
        data2[34] = bFloatToInt(poly->Vertices[3].z) << 4;
        data2[46] = bFloatToInt(poly->Vertices[2].z) << 4;
    }

    data2[0] = *reinterpret_cast<uint32 *>(&poly->UVs[0][0]);
    data2[1] = *reinterpret_cast<uint32 *>(&poly->UVs[0][1]);
    data2[2] = 0x3f800000;
    data2[3] = 0;
    data2[4] = poly->Colours[0][0];
    data2[5] = poly->Colours[0][1];
    data2[6] = poly->Colours[0][2];
    data2[7] = poly->Colours[0][3];
    data2[8] = bFloatToInt(nn[0].x);
    data2[9] = bFloatToInt(nn[0].y);
    data2[11] = 0x3f800000;

    data2[12] = *reinterpret_cast<uint32 *>(&poly->UVs[1][0]);
    data2[13] = *reinterpret_cast<uint32 *>(&poly->UVs[1][1]);
    data2[14] = 0x3f800000;
    data2[15] = 0;
    data2[16] = poly->Colours[1][0];
    data2[17] = poly->Colours[1][1];
    data2[18] = poly->Colours[1][2];
    data2[19] = poly->Colours[1][3];
    data2[20] = bFloatToInt(nn[1].x);
    data2[21] = bFloatToInt(nn[1].y);
    data2[23] = 0x3f800000;

    data2[24] = *reinterpret_cast<uint32 *>(&poly->UVs[3][0]);
    data2[25] = *reinterpret_cast<uint32 *>(&poly->UVs[3][1]);
    data2[26] = 0x3f800000;
    data2[27] = 0;
    data2[28] = poly->Colours[3][0];
    data2[29] = poly->Colours[3][1];
    data2[30] = poly->Colours[3][2];
    data2[31] = poly->Colours[3][3];
    data2[32] = bFloatToInt(nn[3].x);
    data2[33] = bFloatToInt(nn[3].y);
    data2[35] = 0x3f800000;

    data2[36] = *reinterpret_cast<uint32 *>(&poly->UVs[2][0]);
    data2[37] = *reinterpret_cast<uint32 *>(&poly->UVs[2][1]);
    data2[38] = 0x3f800000;
    data2[39] = 0;
    data2[40] = poly->Colours[2][0];
    data2[41] = poly->Colours[2][1];
    data2[42] = poly->Colours[2][2];
    data2[43] = poly->Colours[2][3];
    data2[44] = bFloatToInt(nn[2].x);
    data2[45] = bFloatToInt(nn[2].y);
    data2[47] = 0x3f800000;

    eActiveTexture *active_texture = eGetActiveTexture(texture_info);
    int32 render_target_id = render_target->GetID();
    eDmaDataList *dma_data_list = active_texture->GetDmaDataList(render_target_id, 1);
    int32 apply_alpha_sort = texture_info->ApplyAlphaSorting;

    if (apply_alpha_sort) {
        eAlphaSortKey *sort_key = eNewAlphaSortKey();

        if (sort_key) {
            float z_depth;

            if (poly->GetFlailer() && !(poly->GetFlags() & 2)) {
                z_depth = FlailerLayerToZConversionTable[bFloatToInt(poly->Vertices[0].z)] - zmax;
            } else {
                z_depth = poly->Vertices[0].z;
            }

            sort_key->Z = z_depth;
            sort_key->ActiveTexture = active_texture;
            dma_data_list = sort_key->DmaDataList;
            RenderTargetAlphaLists[render_target_id].AddKey(sort_key);
        } else {
            dma_data_list = nullptr;
        }
    }

    if (dma_data_list) {
        dma_data_list->AddData(4, data_start, qwsize, 0);
    }

    // El original tiene aqui cinco sentencias que no generan codigo (lineas 15975-15983 de la A124, todas en la
    // direccion del epilogo), y una de ellas es un bucle: la nota de LOOP_BEG que deja delante de la etiqueta de
    // retorno hace que reorg prediga como tomados los tres saltos al final y llene su hueco con el `ld ra` del
    // epilogo. Su contenido no se conoce; queda el bucle vacio.
    for (int i = 0; i < 0; i++) {
    }
}

TextureAlphaUsageType AUsage = TEXUSAGE_MODULATED;
TextureAlphaBlendType ABlend = TEXBLEND_DEST_BLEND;
int ASort = 1;
TextureAlphaUsageType AUsageM = TEXUSAGE_MODULATED;
TextureAlphaBlendType ABlendM = TEXBLEND_SRCCOPY;
int ASortM = 1;

// Un poligono con mascara: primero la mascara (RenderSillyMask) y despues la textura, mezclada con el alfa del destino.
void eViewPlatInterface::Render(ePoly *poly, TextureInfo *texture_info, TextureInfo *texture_info_mask, int use_previous_data) {
    texture_info->AlphaUsageType = AUsage;
    texture_info->AlphaBlendType = ABlend;
    texture_info->ApplyAlphaSorting = ASort;

    uint32 AB = texture_info->GetPlatInfo()->AlphaBits;
    uint32 AF = texture_info->GetPlatInfo()->AlphaFix;

    uint32 alpha_bits = 84;

    texture_info->GetPlatInfo()->Special = 15;
    texture_info->GetPlatInfo()->AlphaBits = alpha_bits;
    texture_info->GetPlatInfo()->AlphaFix = 0x80;

    FinishedRenderingFEngLayer();

    RenderSillyMask(poly, texture_info_mask);

    FinishedRenderingFEngLayer();

    Render(poly, texture_info, use_previous_data);

    FinishedRenderingFEngLayer();

    texture_info->GetPlatInfo()->AlphaBits = AB;
    texture_info->GetPlatInfo()->AlphaFix = AF;
    texture_info->GetPlatInfo()->Special = 0;
}

void eViewPlatInterface::FERender(ePoly *poly, TextureInfo *texture_info, bMatrix4 *local_to_world, int use_previous_data,
                                  float bbRad) {
    Render(poly, texture_info, local_to_world, use_previous_data, bbRad);
}

// Un poligono en el mundo (lo transforma la VU1 con local_to_world): la caja que lo envuelve (o la de radio bbRad
// alrededor del primer vertice), los cuatro vertices, sus colores y sus UV, y el MSCAL. Va a la lista de DMA de su
// textura o, si se ordena, a la lista de alfa por la z del primer vertice en la vista.
void eViewPlatInterface::Render(ePoly *poly, TextureInfo *texture_info, bMatrix4 *local_to_world, int use_previous_data,
                                float bbRad) {
    if (!poly || !texture_info) {
        return;
    }

    uint32 qwsize = 10;
    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(qwsize * 16));

    if (!data_start) {
        return;
    }

    float x0 = poly->Vertices[0].x, y0 = poly->Vertices[0].y, z0 = poly->Vertices[0].z;
    float x1 = poly->Vertices[1].x, y1 = poly->Vertices[1].y, z1 = poly->Vertices[1].z;
    float x2 = poly->Vertices[2].x, y2 = poly->Vertices[2].y, z2 = poly->Vertices[2].z;
    float x3 = poly->Vertices[3].x, y3 = poly->Vertices[3].y, z3 = poly->Vertices[3].z;

    float minx = x0, miny = y0, minz = z0;
    float maxx = x0, maxy = y0, maxz = z0;

    if (bbRad != 0.0f) {
        minx = x0 - bbRad;
        maxx = x0 + bbRad;

        miny = y0 - bbRad;
        maxy = y0 + bbRad;

        minz = z0 - bbRad;
        maxz = z0 + bbRad;
    } else {
        if (x1 < minx) minx = x1;
        if (x2 < minx) minx = x2;
        if (x3 < minx) minx = x3;
        if (y1 < miny) miny = y1;
        if (y2 < miny) miny = y2;
        if (y3 < miny) miny = y3;
        if (z1 < minz) minz = z1;
        if (z2 < minz) minz = z2;
        if (z3 < minz) minz = z3;

        if (x1 > maxx) maxx = x1;
        if (x2 > maxx) maxx = x2;
        if (x3 > maxx) maxx = x3;
        if (y1 > maxy) maxy = y1;
        if (y2 > maxy) maxy = y2;
        if (y3 > maxy) maxy = y3;
        if (z1 > maxz) maxz = z1;
        if (z2 > maxz) maxz = z2;
        if (z3 > maxz) maxz = z3;
    }

    uint32 c0 = *reinterpret_cast<uint32 *>(poly->Colours[0]);
    uint32 c1 = *reinterpret_cast<uint32 *>(poly->Colours[1]);
    uint32 c2 = *reinterpret_cast<uint32 *>(poly->Colours[2]);
    uint32 c3 = *reinterpret_cast<uint32 *>(poly->Colours[3]);

    float s0 = poly->UVs[0][0], t0 = poly->UVs[0][1];
    float s1 = poly->UVs[1][0], t1 = poly->UVs[1][1];
    float s2 = poly->UVs[2][0], t2 = poly->UVs[2][1];
    float s3 = poly->UVs[3][0], t3 = poly->UVs[3][1];

    data_start[0] = VIF_CODE(UNPACK | V4_8, 1, VUNPACK_TOPS);
    data_start[1] = 0xFD000004;

    data_start[2] = VIF_CODE(UNPACK | S_32, 1, VUNPACK_TOPS | VUNPACK_UNSIGNED | 1);
    data_start[3] = 0;

    data_start[4] = VIF_CODE(UNPACK | V3_32, 2, VUNPACK_TOPS | VUNPACK_UNSIGNED | 2);
    data_start[5] = FloatAsInteger(minx);
    data_start[6] = FloatAsInteger(miny);
    data_start[7] = FloatAsInteger(minz);
    data_start[8] = FloatAsInteger(maxx);
    data_start[9] = FloatAsInteger(maxy);
    data_start[10] = FloatAsInteger(maxz);

    data_start[11] = VIF_CODE(UNPACK | V4_32, 3, VUNPACK_TOPS | VUNPACK_UNSIGNED | 5);

    data_start[12] = FloatAsInteger(x0);
    data_start[13] = FloatAsInteger(x1);
    data_start[14] = FloatAsInteger(x3);
    data_start[15] = FloatAsInteger(x2);

    data_start[16] = FloatAsInteger(y0);
    data_start[17] = FloatAsInteger(y1);
    data_start[18] = FloatAsInteger(y3);
    data_start[19] = FloatAsInteger(y2);

    data_start[20] = FloatAsInteger(z0);
    data_start[21] = FloatAsInteger(z1);
    data_start[22] = FloatAsInteger(z3);
    data_start[23] = FloatAsInteger(z2);

    data_start[24] = VIF_CODE(UNPACK | V4_8, 4, VUNPACK_TOPS | VUNPACK_UNSIGNED | 45);

    data_start[25] = c0;
    data_start[26] = c1;
    data_start[27] = c2;
    data_start[28] = c3;

    data_start[29] = VIF_CODE(UNPACK | V4_32, 2, VUNPACK_TOPS | VUNPACK_UNSIGNED | 29);

    data_start[30] = FloatAsInteger(s0);
    data_start[31] = FloatAsInteger(t0);
    data_start[32] = FloatAsInteger(t1);
    data_start[33] = FloatAsInteger(s1);

    data_start[34] = FloatAsInteger(s3);
    data_start[35] = FloatAsInteger(t3);
    data_start[36] = FloatAsInteger(t2);
    data_start[37] = FloatAsInteger(s2);

    data_start[38] = VIF_CODE(MSCAL, 0, 0);
    data_start[39] = 0;

    int32 render_target_id = eGetCurrentRenderTarget()->GetID();
    eActiveTexture *active_texture = eGetActiveTexture(texture_info);
    eDmaDataList *dma_data_list = active_texture->GetDmaDataList(render_target_id, 1);

    if (texture_info->ApplyAlphaSorting) {
        eAlphaSortKey *sort_key = eNewAlphaSortKey();

        if (sort_key) {
            bVector4 zdepth(poly->Vertices[0].x, poly->Vertices[0].y, poly->Vertices[0].z, 1.0f);

            eMulVector(&zdepth, reinterpret_cast<eView *>(this)->GetCamera()->GetCameraMatrix(), &zdepth);

            sort_key->Z = zdepth.z;
            sort_key->ActiveTexture = active_texture;
            dma_data_list = sort_key->DmaDataList;
            RenderTargetAlphaLists[render_target_id].AddKey(sort_key);
        } else {
            dma_data_list = nullptr;
        }
    }

    if (dma_data_list) {
        dma_data_list->SetLocalWorldMatrix(local_to_world);
        dma_data_list->AddData(5, data_start, qwsize, 0);
    }
}

// Lo que lee y escribe eDisplayFrame (valores iniciales del .data de la A124).
int HeadLightsON = 1;
int PolyNum;
int ModelNum;
int EnableLODZ = 1;
int ONEMinimalEnvironmentMappingEnable;
int AreAnyFrontEndCarsVisible;
int targ_u = 8;
int targ_v = 8;
int targ_x = 8;
int targ_y = 8;
int mv = 1;
int rv = 1;
int rspec = 128;
int gspec = 128;
int bspec = 128;
static const float gRadialBlur = 0.0f;

float ShadC = 0.5f;
float ShadARange = 0.6f;

// El retrovisor del jugador 1: la vista EVIEW_PLAYER1_RVM se pinta en su propio render target y despues se pega en la
// vista del jugador (DisplayRVMs), oscurecida segun la hora del dia y las nubes.
void eRenderRVMirror(eDmaChain *dma_chain, eDmaChain *tex_chain, GrandSceneryCullInfo *grand_scenery_cull_info) {
    if (!FEngHud::ShouldRearViewMirrorBeVisible(EVIEW_PLAYER1)) {
        return;
    }

    eView *view = eGetView(EVIEW_PLAYER1_RVM, false);
    eRenderTarget *render_target = view->GetRenderTarget();
    if (view->IsActive() && render_target && render_target->IsActive()) {
        eSetCurrentRenderTarget(render_target);

        AttachZbufClear(dma_chain, view);

        eClearRenderTarget(render_target, dma_chain, 255, 255, 255, 255);

        AttachColourModulation(dma_chain, 0.5f, 0.5f, 0.5f, 0.5f);

        StuffSkyLayer(view, SKY_LAYER_LOWREZ);
        grand_scenery_cull_info->StuffScenery(view, 0x81);

        AttachSolidPolys(dma_chain, tex_chain);
        AttachAlphaPolys(dma_chain, tex_chain);

        DrawTestCars(view, 0);

        RenderVehicleFlares(view, 0, 0);
#ifndef EA_BUILD_A124
        eRenderLightFlarePool(view);
#endif
        eRenderWorldLightFlares(view, FLARE_NORM);

        AttachSolidPolys(dma_chain, tex_chain);
        AttachAlphaPolys(dma_chain, tex_chain);

        OverlayRVMImages();

        AttachSolidPolys(dma_chain, tex_chain);
        AttachAlphaPolys(dma_chain, tex_chain);

        eSetCurrentRenderTarget(eGetView(EVIEW_PLAYER1, false)->GetRenderTarget());

        DisplayRVMs(eGetView(EVIEW_PLAYER1, false));
        AttachSolidPolys(dma_chain, tex_chain);
        AttachAlphaPolys(dma_chain, tex_chain);

        if (GetCurrentTimeOfDay() == 0) {
            ShadC = 0.5f;
        } else {
#ifdef EA_BUILD_A124
            ShadC = 1.0f;
#else
            ShadC = 0.9f;
#endif
        }

        AttachColourModulation(dma_chain, ShadC, ShadC, ShadC,
                               1.0f - eGetView(EVIEW_PLAYER1, false)->Precipitation->GetCloudIntensity() * ShadARange);

        eSetZWriteState(1);
    }
}

struct ParticleSystemStats {
    // total size: 0x20
    int NumParticles;              // offset 0x0, size 0x4
    int MaxNumParticles;           // offset 0x4, size 0x4
    int NumActiveParticleTextures; // offset 0x8, size 0x4
    int MaxNumParticleTextures;    // offset 0xC, size 0x4
    int NumEmitters;               // offset 0x10, size 0x4
    int MaxNumEmitters;            // offset 0x14, size 0x4
    int NumEmitterGroups;          // offset 0x18, size 0x4
    int MaxNumEmitterGroups;       // offset 0x1C, size 0x4

    ParticleSystemStats() {
        NumParticles = 0;
        MaxNumParticles = 0;
        NumActiveParticleTextures = 0;
        MaxNumParticleTextures = 0;
        NumEmitters = 0;
        MaxNumEmitters = 0;
        NumEmitterGroups = 0;
        MaxNumEmitterGroups = 0;
    }
};

static ParticleSystemStats g_ParticleStats;

void SetParticleSystemStats(int num_particles, int max_num_particles, int num_particle_textures, int max_num_particle_textures,
                            int num_emitters, int max_num_emitters, int num_groups, int max_num_groups) {
    g_ParticleStats.NumParticles = num_particles;
    g_ParticleStats.MaxNumParticles = max_num_particles;
    g_ParticleStats.NumActiveParticleTextures = num_particle_textures;
    g_ParticleStats.MaxNumParticleTextures = max_num_particle_textures;
    g_ParticleStats.NumEmitters = num_emitters;
    g_ParticleStats.MaxNumEmitters = max_num_emitters;
    g_ParticleStats.NumEmitterGroups = num_groups;
    g_ParticleStats.MaxNumEmitterGroups = max_num_groups;
}

// El fotograma de PS2: elige el juego de buffers y las direcciones de VRAM del frontend o del juego, actualiza los
// paquetes del GS de cada render target y las matrices de las vistas, llena las dos cadenas de DMA de la etapa libre
// (cielo, escenario, coches, efectos de pantalla, interfaz) y las manda a dibujar al cambiar de buffer.
void eDisplayFrame() {
    static int inFE;
    static int inGM;
    eRenderTarget *render_targets[2];
    int are_any_frontend_cars_visible;
    eDmaChain *dma_chain;
    eDmaChain *tex_chain;

    if (FEDatabase) {
        VideoSettings *video_settings = FEDatabase->GetVideoSettings();

        if (video_settings) {
            eSetScreenDisplayOffsets(bFloatToInt(video_settings->ScreenOffsetX),
                                     bFloatToInt(video_settings->ScreenOffsetY));
        }
    }

    testCam = Player1Camera;

    if (TheGameFlowManager.IsInFrontend()) {
        if (!inFE) {
            inFE = 1;
            inGM = 0;
            SetScreenBuffers();
            eGetView(EVIEW_ANTI_ALIAS, false)->SetActive(1);
            {
                eRenderTarget *rc = eGetRenderTarget(TARGET_FRAMEBUFFER0);

                rc->FrameAddress = eVRAM.GetArea(VRAM_FRAMEBUFFER0, 2) / 8192;
                rc->ZBufferMode = PSMZ16S;
                rc->ZBufferAddress = eVRAM.GetArea(VRAM_ZBUFFER, 2) / 8192;

                rc = eGetRenderTarget(TARGET_FRAMEBUFFER1);
                rc->FrameAddress = eVRAM.GetArea(VRAM_FRAMEBUFFER1, 2) / 8192;
                rc->ZBufferMode = PSMZ16S;
                rc->ZBufferAddress = eVRAM.GetArea(VRAM_ZBUFFER, 2) / 8192;
            }
        }
    } else if (!inGM) {
        inGM = 1;
        inFE = 0;
        eGetView(EVIEW_ANTI_ALIAS, false)->SetActive(0);
        SetScreenBuffers();
        {
            eRenderTarget *rc = eGetRenderTarget(TARGET_FRAMEBUFFER0);

            rc->FrameAddress = eVRAM.GetArea(VRAM_FRAMEBUFFER0, 0) / 8192;
            rc->ZBufferMode = PSMZ24;
            rc->ZBufferAddress = eVRAM.GetArea(VRAM_ZBUFFER, 0) / 8192;

            rc = eGetRenderTarget(TARGET_FRAMEBUFFER1);
            rc->FrameAddress = eVRAM.GetArea(VRAM_FRAMEBUFFER1, 0) / 8192;
            rc->ZBufferMode = PSMZ24;
            rc->ZBufferAddress = eVRAM.GetArea(VRAM_ZBUFFER, 0) / 8192;
        }
    }

    PolyNum = 0;
    ModelNum = 0;

    if (Attrib::Database::IsInitialized()) {
        ePreDisplay();
    }

    if (eGetCurrentViewMode() == EVIEWMODE_TWOH) {
        if (TheGameFlowManager.IsInFrontend()) {
            HeadLightsON = 1;
        } else {
            HeadLightsON = 0;
        }
        EnableLODZ = 0;
        DrawCarsReflections = 0;
        MinimalEnvironmentMappingEnable = 1;
    } else {
        HeadLightsON = 1;
        EnableLODZ = 1;
        DrawCarsReflections = 1;
        MinimalEnvironmentMappingEnable = ONEMinimalEnvironmentMappingEnable;
    }

    are_any_frontend_cars_visible = IsAnyFrontEndCars();
    AreAnyFrontEndCarsVisible = are_any_frontend_cars_visible;

    // Las texturas que se bajan cada fotograma empiezan detras de lo que ocupa la VRAM fija de cada modo.
    if (TheGameFlowManager.IsInFrontend()) {
        if (are_any_frontend_cars_visible) {
            DownloadVramStartAddrees = eVRAM.GetArea(VRAM_START_TEXTURES, eVRAM.GetCurrentLayout());
        } else {
            DownloadVramStartAddrees = eVRAM.GetArea(VRAM_START_TEXTURES_FE, eVRAM.GetCurrentLayout());
        }
    } else if (!MovieIsPlaying()) {
        DownloadVramStartAddrees = eVRAM.GetArea(VRAM_START_TEXTURES, 0);
    } else {
        DownloadVramStartAddrees = eVRAM.GetArea(VRAM_START_TEXTURES_FE, 0);
    }

    TextureDownloader.Print("-------------------------------------------------\n");
    TextureDownloader.Print("Texture Downloader:\n");
    TextureDownloader.Print("\n");

    TextureDownloader.Init(DownloadVramStartAddrees / 256, 16384);

    {
        int current_video_mode = GetVideoMode();

        if (eCurrentVideoMode != current_video_mode) {
            eSetDisplaySystem(current_video_mode);
        }
    }

    MaybeChangeViewMode();

    if (PreviousDisplayBufferMode != DisplayBufferMode) {
        CurrentBufferParameters = BufferParameters[0];
        ePutDisplayEnvironment();
        eWaitUntilRenderingDone();
        eClearVram(0);
        eSwapScreenBuffers();
        ePutDisplayEnvironment();
        PreviousDisplayBufferMode = DisplayBufferMode;
    }

    CurrentBufferParameters = &BufferParameters[DisplayBufferMode][CurrentHalfOffset];

    uint32 half_offsetx = CurrentBufferParameters->HalfOffsetX;
    uint32 half_offsety = CurrentBufferParameters->HalfOffsetY;
    uint32 quarter_offsetx = CurrentBufferParameters->QuarterOffsetX;
    uint32 quarter_offsety = CurrentBufferParameters->QuarterOffsetY;

    if (EnvironmentMappingEnable && !MinimalEnvironmentMappingEnable) {
        UpdateEnvironmentMapCameras();
    }

    elBeginFrame();

    for (int i = 0; i < NUM_EVIEWS; i++) {
        eView *view = &eViews[i];
        eRenderTarget *render_target = view->GetRenderTarget();

        if (render_target) {
            render_target->SetActive(view->Active);
        }
    }

    static uint32 nextframe = 9999999;

    if (TheGameFlowManager.IsInFrontend()) {
        nextframe = eFrameCounter + 3;
        for (int target_id = 0; target_id < TOTAL_RENDER_TARGETS; target_id++) {
            eRenderTarget *render_target = eGetRenderTarget(target_id);

            if (render_target->IsActive()) {
                render_target->UpdateGifTagsOLD(half_offsetx, half_offsety, quarter_offsetx, quarter_offsety,
                                                IdleRenderStageQueue, 0);
            }
        }
    } else {
        int fast = 1;
        static int oCurrentViewMode = -1;
        static int wideFlip;
        static float ASrat = 1.0f;
        static int ovM = -1;

        // Los paquetes solo se rehacen enteros dos fotogramas despues de cambiar de modo de pantalla.
        if (FEDatabase && FEDatabase->GetVideoSettings()->WideScreen != wideFlip) {
            wideFlip = FEDatabase->GetVideoSettings()->WideScreen;
            nextframe = eFrameCounter + 2;
        }

        if (NTSCAspectRatio3dx != ASrat) {
            ASrat = NTSCAspectRatio3dx;
            nextframe = eFrameCounter + 2;
        }

        if (GetVideoMode() != ovM) {
            ovM = GetVideoMode();
            nextframe = eFrameCounter + 2;
        }

        if (CurrentViewMode != oCurrentViewMode) {
            oCurrentViewMode = CurrentViewMode;
            nextframe = eFrameCounter + 2;
        }

        if (nextframe >= eFrameCounter) {
            fast = 0;
        }

        for (int target_id = 0; target_id < TOTAL_RENDER_TARGETS; target_id++) {
            eRenderTarget *render_target = eGetRenderTarget(target_id);

            if (render_target->IsActive()) {
                render_target->UpdateGifTags(half_offsetx, half_offsety, quarter_offsetx, quarter_offsety,
                                             IdleRenderStageQueue, fast);
            }
        }
    }

    for (int32 view_id = 0; view_id < NUM_EVIEWS; view_id++) {
        eView *view = eGetView(view_id, false);
        eViewPlatInfo *plat_info = view->GetPlatInfo();

        if (view_id >= EVIEW_ENVMAP0F && view_id <= EVIEW_ENVMAP0D) {
            plat_info->CalculateViewMatricies(view, 0.0f, GetFaceFarZ[view_id - EVIEW_ENVMAP0F], 0.0f);
        } else if (view_id >= EVIEW_PLAYER1_REFLECTION && view_id <= EVIEW_PLAYER2_REFLECTION) {
            plat_info->CalculateViewMatricies(view, 0.5f, 700.0f, 0.0f);
        } else if (view_id >= EVIEW_PLAYER1_SPECULAR && view_id <= EVIEW_PLAYER2_SPECULAR) {
            plat_info->CalculateViewMatricies(view, 0.5f, 700.0f, 0.0f);
        } else if (view_id == EVIEW_PLAYER1_RVM) {
            plat_info->CalculateViewMatricies(view, 0.5f, 300.0f, 0.0f);
        } else {
            if (TheGameFlowManager.IsInFrontend()) {
                BackgroundNearZ = 0.21f;
            } else if (UTL::Collections::Singleton<INIS>::Get()) {
                BackgroundNearZ = 0.5f;
            } else {
                BackgroundNearZ = 0.7f;
            }
            plat_info->CalculateViewMatricies(view, BackgroundNearZ, 50000.0f, 0.0f);
        }

        // En el frontend el render target del coche copia las matrices de la vista del jugador.
        if (TheGameFlowManager.IsInFrontend()) {
            eRenderTarget *player_target = eGetView(EVIEW_PLAYER1, false)->GetRenderTarget();
            eRenderTarget *target0 = eGetRenderTarget(TARGET_FRAMEBUFFER2);

            target0->WorldClipGB = player_target->WorldClipGB;
            target0->WorldView = player_target->WorldView;
            target0->ClipGBScreen = player_target->ClipGBScreen;
            target0->VUClipGBScreen0 = player_target->VUClipGBScreen1;
            target0->VUClipGBScreen1 = player_target->VUClipGBScreen1;
            target0->VUClipGBScreen2 = player_target->VUClipGBScreen2;
            target0->SetupMatrixPacks();
        }
    }

    dma_chain = &IdleRenderStageQueue->DataDmaChain;
    tex_chain = &IdleRenderStageQueue->TextureDmaChain;

    dma_chain->Init();
    tex_chain->Init();

    AttachTestSet(dma_chain);
    AttachMicroCode(dma_chain);
    AttachDrawEnvironment(dma_chain);

    TextureDownloader.AddPath3GateDmaTag(dma_chain, tex_chain);

    if (TheGameFlowManager.IsInGame()) {
        eClearRenderTarget(eGetRenderTarget(TARGET_PLAYER1_SPECULAR), dma_chain, 255, 255, 255, 128);
        if (CurrentViewMode == EVIEWMODE_TWOH) {
            eClearRenderTarget(eGetRenderTarget(TARGET_PLAYER2_SPECULAR), dma_chain, 255, 255, 255, 128);
        }
    }

    TextureDownloader.Init(DownloadVramStartAddrees / 256, 16384);

    GrandSceneryCullInfo grand_scenery_cull_info;

    if (pTextureInfoGlossyWindow) {
        int paletteAddr = eVRAM.GetArea(VRAM_GLOSSYWINDOW_PALETTE, eVRAM.GetCurrentLayout()) / 256;

        TextureDownloader.AddStaticTextureForDownload(
            pTextureInfoGlossyWindow, eVRAM.GetArea(VRAM_GLOSSYWINDOW, eVRAM.GetCurrentLayout()) / 256, paletteAddr);
    }

    if (pTextureInfoSmallEnvMap) {
        int paletteAddr = eVRAM.GetArea(VRAM_SMALL_ENVMAP_PALETTE, eVRAM.GetCurrentLayout()) / 256;

        TextureDownloader.AddStaticTextureForDownload(
            pTextureInfoSmallEnvMap, eVRAM.GetArea(VRAM_SMALL_ENVMAP, eVRAM.GetCurrentLayout()) / 256, paletteAddr);
    }

    if (!MovieIsPlaying()) {
        eSetZWriteState(0);

        // El cielo: las capas de cada vista que lo ve, cada una modulada por su color.
        int SkyViewsList[12] = {EVIEW_PLAYER1,          EVIEW_PLAYER2,          EVIEW_PLAYER1_REFLECTION,
                                EVIEW_PLAYER2_REFLECTION, EVIEW_PLAYER1_SPECULAR, EVIEW_PLAYER2_SPECULAR,
                                EVIEW_ENVMAP0F,         EVIEW_ENVMAP0R,         EVIEW_ENVMAP0B,
                                EVIEW_ENVMAP0L,         EVIEW_ENVMAP0U,         EVIEW_ENVMAP0D};

        for (int32 view_index = 0; view_index < 12; view_index++) {
            int32 view_id = SkyViewsList[view_index];
            eView *view = eGetView(view_id, false);

            if (view->IsActive() && view->GetRenderTarget() && view->GetRenderTarget()->IsActive()) {
                eSetCurrentRenderTarget(view->GetRenderTarget());
                if (TheGameFlowManager.IsInGame()) {
                    switch (view_id) {
                    case EVIEW_PLAYER1:
                    case EVIEW_PLAYER2: {
                        float r;
                        float g;
                        float b;
                        float a;

                        GetLayerMod(view, SKY_LAYER_BLUE, &r, &g, &b, &a);
                        AttachColourModulation(dma_chain, r, g, b, a);
                        StuffSkyLayer(view, SKY_LAYER_BLUE);
                        AttachSolidPolys(dma_chain, tex_chain);

                        GetLayerMod(view, SKY_LAYER_CLOUDS, &r, &g, &b, &a);
                        if (a > 0.001f) {
                            AttachColourModulation(dma_chain, r, g, b, a);
                            StuffSkyLayer(view, SKY_LAYER_CLOUDS);
                            AttachSolidPolys(dma_chain, tex_chain);
                        }

                        GetLayerMod(view, SKY_LAYER_OVERCAST, &r, &g, &b, &a);
                        if (a > 0.001f) {
                            AttachColourModulation(dma_chain, r, g, b, a);
                            StuffSkyLayer(view, SKY_LAYER_OVERCAST);
                            AttachSolidPolys(dma_chain, tex_chain);
                        }
                        break;
                    }
                    case EVIEW_PLAYER1_REFLECTION:
                    case EVIEW_PLAYER2_REFLECTION:
                        if (view->Precipitation->GetRoadDampness() > 0.0f) {
                            StuffSkyLayer(view, SKY_LAYER_REFLECTION);
                        }
                        break;
                    case EVIEW_PLAYER1_SPECULAR:
                    case EVIEW_PLAYER2_SPECULAR:
                        StuffSpecular(view);
                        break;
                    case EVIEW_ENVMAP0F:
                    case EVIEW_ENVMAP0R:
                    case EVIEW_ENVMAP0B:
                    case EVIEW_ENVMAP0L:
                    case EVIEW_ENVMAP0U:
                    case EVIEW_ENVMAP0D:
                        if (IsFaceEnabled(view_index - 6) && !MinimalEnvironmentMappingEnable) {
                            float r;
                            float g;
                            float b;
                            float a;

                            GetLayerMod(view, SKY_LAYER_LOWREZ, &r, &g, &b, &a);
                            AttachColourModulation(dma_chain, r, g, b, a);
                            StuffSkyLayer(view, SKY_LAYER_LOWREZ);
                        }
                        break;
                    }
                }
            }
        }

        AttachSolidPolys(dma_chain, tex_chain);
        AttachAlphaPolys(dma_chain, tex_chain);
    }

    eSetZWriteState(1);

    render_targets[0] = eGetRenderTarget(TARGET_PLAYER1);
    render_targets[1] = eGetRenderTarget(TARGET_PLAYER2);

    for (int32 view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER1_RVM; view_id++) {
        eView *view = eGetView(view_id, false);

        if (view->Active) {
            view->SetupWorldLightContext();
            if (TheGameFlowManager.IsInGame() && view_id != EVIEW_PLAYER1_RVM) {
                pVisualTreatmentPlat->Update(view);
            }
        }
    }

    if (!MovieIsPlaying()) {
        if (DrawWorld && TheGameFlowManager.IsInGame()) {
            for (int view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER1_RVM; view_id++) {
                eView *view = eGetView(view_id, false);

                if (view->IsActive() && view->GetRenderTarget()->IsActive()) {
                    SetupSceneryCullInfo(view, *grand_scenery_cull_info.AddCullInfo(),
                                         view_id != EVIEW_PLAYER1_RVM ? 0x4000 : 0);
                }
            }

            if (EnvironmentMappingEnable && !MinimalEnvironmentMappingEnable) {
                for (int view_id = EVIEW_ENVMAP0F; view_id <= EVIEW_ENVMAP0D; view_id++) {
                    eView *view = eGetView(view_id, false);

                    if (view->IsActive() && view->GetRenderTarget()->IsActive() && IsFaceEnabled(view_id - EVIEW_ENVMAP0F)) {
                        SetupSceneryCullInfo(view, *grand_scenery_cull_info.AddCullInfo(), 0x1100);
                    }
                }
            }
        }

        if (TheGameFlowManager.IsInGame()) {
            EVIEW_ID lastRefView = EVIEW_PLAYER1_REFLECTION;

            if (RenderingViewMode == EVIEWMODE_TWOH || RenderingViewMode == EVIEWMODE_TWOV) {
                lastRefView = EVIEW_PLAYER2_REFLECTION;
            }

            for (int view_id = EVIEW_PLAYER1_REFLECTION; view_id <= lastRefView; view_id++) {
                eView *view = eGetView(view_id, false);

                if (view->IsActive() && view->GetRenderTarget()->IsActive() &&
                    view->Precipitation->GetRoadDampness() > 0.0f) {
                    if (DrawWorld && EnableLODZ) {
                        SetupSceneryCullInfo(view, *grand_scenery_cull_info.AddCullInfo(), 0x800);
                    }
                }
            }
        }

        grand_scenery_cull_info.DoCulling();
    }

    if (TheGameFlowManager.IsInFrontend()) {
        eView *view = eGetView(EVIEW_PLAYER1, false);

        eClearRenderTarget(eGetRenderTarget(TARGET_FRAMEBUFFER2), dma_chain, 255, 255, 255, 128);
        eSetCurrentRenderTarget(view->GetRenderTarget());

        SetFeCarRenderFlagsGlob(1, 1, 0);
        RenderFrontEndCars(view, 1);

        SetFeCarRenderFlagsGlob(1, 0, 0);
        RenderFrontEndCars(view, 0);
        AttachSolidPolys(dma_chain, tex_chain);

        {
            eRenderTarget *player_target = view->GetRenderTarget();
            eRenderTarget *target0 = eGetRenderTarget(TARGET_FRAMEBUFFER2);

            target0->WorldClipGB = player_target->WorldClipGB;
            target0->WorldView = player_target->WorldView;
            target0->ClipGBScreen = player_target->ClipGBScreen;
            target0->VUClipGBScreen0 = player_target->VUClipGBScreen1;
            target0->VUClipGBScreen1 = player_target->VUClipGBScreen1;
            target0->VUClipGBScreen2 = player_target->VUClipGBScreen2;
            target0->SetupMatrixPacks();
        }

        eSetCurrentRenderTarget(eGetRenderTarget(TARGET_FRAMEBUFFER2));
        SetFeCarRenderFlagsGlob(1, 0, 0);
        RenderFrontEndCars(eGetView(EVIEW_ANTI_ALIAS, false), 0);
        AttachSolidPolys(dma_chain, tex_chain);
        AttachAlphaPolys(dma_chain, tex_chain);

        eAttachTargetMoveSprites(dma_chain, MainDraw0Context, eGetRenderTarget(TARGET_FRAMEBUFFER2), 0, 0, 1, 1, 100, 64, 1,
                                 128, 128, 128, 128, 0, SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1));
    }

    if (!MovieIsPlaying()) {
        bVector3 *posi = static_cast<bVector3 *>(eFrameMalloc(sizeof(bVector3)));

        if (posi) {
            bCopy(posi, eGetView(EVIEW_PLAYER1, false)->GetCamera()->GetPosition());
            AttachVUdata(dma_chain, reinterpret_cast<uint32 *>(posi), 692, 1);
        }

        if (TheGameFlowManager.IsInGame()) {
            EVIEW_ID lastRefView = EVIEW_PLAYER1_REFLECTION;

            if (RenderingViewMode == EVIEWMODE_TWOH || RenderingViewMode == EVIEWMODE_TWOV) {
                lastRefView = EVIEW_PLAYER2_REFLECTION;
            }

            for (int view_id = EVIEW_PLAYER1_REFLECTION; view_id <= lastRefView; view_id++) {
                eView *view = eGetView(view_id, false);

                if (view->IsActive() && view->GetRenderTarget()->IsActive() &&
                    view->Precipitation->GetRoadDampness() > 0.0f) {
                    eSetCurrentRenderTarget(view->GetRenderTarget());
                    if (EnableLODZ) {
                        grand_scenery_cull_info.StuffScenery(view, 0x88);
                    }
                    RenderWorldModels(view, 0x800);
                    SetFeCarRenderFlagsGlob(1, 1, 0);
                    RenderFrontEndCars(view, 1);
                    RenderVehicleFlares(view, 1, 0);
                    eRenderLightFlarePool(view);
                }
            }
        }

        if (are_any_frontend_cars_visible) {
            AttachSolidPolys(dma_chain, tex_chain);
            AttachAlphaPolys(dma_chain, tex_chain);
        }

        if (EnvironmentMappingEnable && !MinimalEnvironmentMappingEnable && TheGameFlowManager.IsInGame()) {
            for (int i = 0; i < 6; i++) {
                eView *view = eGetView(EVIEW_ENVMAP0F + i, false);

                if (view->IsActive() && view->GetRenderTarget()->IsActive()) {
                    eSetCurrentRenderTarget(view->GetRenderTarget());
                    if (IsFaceEnabled(i)) {
                        eRenderWorldLightFlares(view, FLARE_ENV);
                        RenderVehicleFlares(view, 0, 1);
                        grand_scenery_cull_info.StuffScenery(view, 0x81);
                        eRenderLightFlarePool(view);
                    }
                }
            }
        }

        if (mv) {
            for (int view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER2; view_id++) {
                eView *view = eGetView(view_id, false);

                if (view->IsActive() && view->GetRenderTarget() && view->GetRenderTarget()->IsActive()) {
                    eSetCurrentRenderTarget(view->GetRenderTarget());
                    FrameTimingStage0.efStuffAllScenery.Start();
                    grand_scenery_cull_info.StuffScenery(view, 0x28);
                    FrameTimingStage0.efStuffAllScenery.End();
                    if (!UTL::Collections::Singleton<INIS>::Get()) {
                        RenderWorldModels(view, 0);
                    }
                    if (TheGameFlowManager.IsInFrontend()) {
                        GarageMainScreen *garageMainScreen = GarageMainScreen::GetInstance();

                        if (garageMainScreen) {
                            garageMainScreen->HandleRender(1);
                        }
                    }
                }
            }
        }

        AttachSolidPolys(dma_chain, tex_chain);

        {
            int alpha = static_cast<int>((1.0f - eGetView(EVIEW_PLAYER1_SPECULAR, false)->Precipitation->GetRoadDampness()) *
                                         128.0f);

            eAttachTargetMoveSprites(dma_chain, eGetRenderTarget(TARGET_PLAYER1_REFLECTION),
                                     eGetRenderTarget(TARGET_PLAYER1_SPECULAR), 0, 0, 12, 16, 100, alpha, 1, rspec, gspec,
                                     bspec, 0, 0, SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1));
            if (CurrentViewMode == EVIEWMODE_TWOH) {
                eAttachTargetMoveSprites(dma_chain, eGetRenderTarget(TARGET_PLAYER2_REFLECTION),
                                         eGetRenderTarget(TARGET_PLAYER2_SPECULAR), 0, 0, 12, 16, 100, alpha, 1, rspec,
                                         gspec, bspec, 0, 0, SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1));
            }
        }
    }

    AttachColourModulation(dma_chain, ShadC, ShadC, ShadC,
                           1.0f - eGetView(EVIEW_PLAYER1, false)->Precipitation->GetCloudIntensity() * ShadARange);

    if (!MovieIsPlaying()) {
        if (rv) {
            for (int view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER2; view_id++) {
                eView *view = eGetView(view_id, false);

                if (view->IsActive() && view->GetRenderTarget() && view->GetRenderTarget()->IsActive()) {
                    eSetCurrentRenderTarget(view->GetRenderTarget());
                    grand_scenery_cull_info.StuffScenery(view, 0x10);
                    RenderTrackMarkers(view);
                    RenderTrackMarkers(view);
                    DrawMissionComponents();
                    RenderVisibleZones(view);
                    RenderTrackRoutes(view);
                    RenderCameraMovers(view);
                    FrameTimingStage0.efStuffClans.Start();
                    RenderClans(view);
                    FrameTimingStage0.efStuffClans.End();
                }
            }
        }
    }

    AttachSolidPolys(dma_chain, tex_chain);
    AttachAlphaPolys(dma_chain, tex_chain);

    if (!MovieIsPlaying()) {
        if ((EnvironmentMappingEnable && !MinimalEnvironmentMappingEnable && TheGameFlowManager.IsInGame()) ||
            are_any_frontend_cars_visible) {
            if (TheGameFlowManager.IsInFrontend()) {
                if (pTextureInfoCarSelectEnvMap) {
                    TextureDownloader.AddStaticTextureForDownload(
                        pTextureInfoCarSelectEnvMap, eVRAM.GetArea(VRAM_ENVMAP0_FULL, eVRAM.GetCurrentLayout()) / 256, 0);
                }
            } else {
                eView *view = eGetView(EVIEW_FLAILER, false);

                if (view->IsActive() && view->GetRenderTarget()->IsActive()) {
                    eSetCurrentRenderTarget(view->GetRenderTarget());
                    eCreateEnvMapPrims(eGetRenderTarget(TARGET_ENVMAP0_FULL));
                }
            }
        }
    }

    eSetCurrentRenderTarget(eGetView(EVIEW_PLAYER1, false)->GetRenderTarget());
    eDisplayEnvRenderTargets(eGetView(EVIEW_PLAYER1, false));
    AttachSolidPolys(dma_chain, tex_chain);
    AttachAlphaPolys(dma_chain, tex_chain);

    if (Path3TextureTransferEnabled) {
        if (TextureDownloader.AddPath3GateDmaTag(dma_chain, dma_chain)) {
            AttachDummyTexture(tex_chain, 1);
        }
    }

    if (TheGameFlowManager.IsInGame()) {
        if (eAttachDepthIntoAlpha(dma_chain)) {
            eAttachFog(dma_chain, eVRAM.GetArea(VRAM_FOG_PALETTE_START, 0), DISTANT_FOG);
        }
        if (!QueryFlushAccumulationBuffer()) {
            if (!pVisualTreatmentPlat || bEqual(pVisualTreatmentPlat->GetRadialBlur(), 0.0f, 0.01f) ||
                !bEqual(gRadialBlur, 0.0f, 0.01f)) {
                eAttachSmear2(dma_chain, eVRAM.GetArea(VRAM_FOG_PALETTE_START, 0), 1.0f);
            }
        }
    }

    if (Path3TextureTransferEnabled) {
        if (TextureDownloader.AddPath3GateDmaTag(dma_chain, dma_chain)) {
            AttachDummyTexture(tex_chain, 1);
        }
    }

    if (!MovieIsPlaying()) {
        for (int view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER2; view_id++) {
            eView *view = eGetView(view_id, false);

            if (view->IsActive() && view->GetRenderTarget() && view->GetRenderTarget()->IsActive()) {
                eSetCurrentRenderTarget(view->GetRenderTarget());
                RenderFEFlares(view, 0);
                RenderGpsArrows(view);
                DebugCarRenderTest::Render();
                DrawTestCars(view, 0);
                if (TheGameFlowManager.IsInFrontend()) {
                    eRenderTarget *player_target = eGetView(EVIEW_PLAYER1, false)->GetRenderTarget();

                    player_target->VUClipGBScreen0 = player_target->VUClipGBScreen2;
                    player_target->VUClipGBScreen1 = player_target->VUClipGBScreen2;
                    player_target->SetupMatrixPacks();
                }
                SetFeCarRenderFlagsGlob(0, 1, 1);
                RenderFrontEndCars(view, 0);
                view->Precipitation->UpdateAndRender();
            }
        }
        AttachSolidPolys(dma_chain, tex_chain);
        AttachAlphaPolys(dma_chain, tex_chain);
    }

    if (UTL::Collections::Singleton<INIS>::Get()) {
        for (int view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER2; view_id++) {
            eView *view = eGetView(view_id, false);

            if (view->IsActive() && view->GetRenderTarget() && view->GetRenderTarget()->IsActive()) {
                RenderWorldModels(view, 0);
                AttachSolidPolys(dma_chain, tex_chain);
                AttachAlphaPolys(dma_chain, tex_chain);
            }
        }
    }

    if (!MovieIsPlaying()) {
        for (int view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER2; view_id++) {
            eView *view = eGetView(view_id, false);
            eRenderTarget *render_target = view->GetRenderTarget();

            if (view->IsActive() && render_target && render_target->IsActive()) {
                eSetCurrentRenderTarget(render_target);
                if (TheGameFlowManager.IsInFrontend()) {
                    eRenderTarget *player_target = eGetView(EVIEW_PLAYER1, false)->GetRenderTarget();

                    render_target->WorldClipGB = player_target->WorldClipGB;
                    render_target->WorldView = player_target->WorldView;
                    render_target->ClipGBScreen = player_target->ClipGBScreen;
                    render_target->VUClipGBScreen0 = player_target->VUClipGBScreen2;
                    render_target->VUClipGBScreen1 = player_target->VUClipGBScreen2;
                    render_target->VUClipGBScreen2 = player_target->VUClipGBScreen2;
                    render_target->SetupMatrixPacks();
                }
                eRenderLightFlarePool(view);
                eRenderWorldLightFlares(view, FLARE_NORM);
                RenderVehicleFlares(view, 0, 0);
                gEmitterSystem.Render(view);
                NGSpriteManager.RenderAll(view);
            }
        }
        AttachSolidPolys(dma_chain, tex_chain);
        AttachAlphaPolys(dma_chain, tex_chain);
    }

    if (TheGameFlowManager.IsInGame()) {
        uint32 DoBlur = 0;

        for (int view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER2; view_id++) {
            if (eGetView(view_id, false)->GetRenderTarget()) {
                DoBlur = pVisualTreatmentPlat->Radialblur(eGetView(view_id, false)->GetRenderTarget(), dma_chain);
            }
        }
        if (DoBlur) {
            eAttachSmear2(dma_chain, eVRAM.GetArea(VRAM_FOG_PALETTE_START, 1),
                          pVisualTreatmentPlat->GetNosRadialBlur() == 0.0f ? 1.0f : 10.0f);
        }
    }

    if (Path3TextureTransferEnabled) {
        if (TextureDownloader.AddPath3GateDmaTag(dma_chain, dma_chain)) {
            AttachDummyTexture(tex_chain, 1);
        }
    }

    eRenderRVMirror(dma_chain, tex_chain, &grand_scenery_cull_info);

    FrameTimingStage0.efStuffSun.Start();
    for (int view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER2; view_id++) {
        eView *view = eGetView(view_id, false);

        if (view->IsActive() && view->GetRenderTarget() && view->GetRenderTarget()->IsActive()) {
            eSetCurrentRenderTarget(view->GetRenderTarget());
            StuffSun(view);
            eRenderOccludedFlares(dma_chain, tex_chain);
            AttachSolidPolys(dma_chain, tex_chain);
            AttachAlphaPolys(dma_chain, tex_chain);
            FrameTimingStage0.efStuffSun.End();
        }
    }

    if (!MovieIsPlaying()) {
        if (TheGameFlowManager.IsInGame()) {
            Camera *camera1 = eGetView(EVIEW_PLAYER1, false)->GetCamera();
            Camera *camera2 = eGetView(EVIEW_PLAYER2, false)->GetCamera();
            float camera_fd1 = FocalDistance;
            float camera_dof1 = DepthOfField;

            if (camera_fd1 == 0.0f) {
                camera_fd1 = camera1->GetFocalDistance();
            }
            if (camera_dof1 == 0.0f) {
                camera_dof1 = camera1->GetDepthOfField();
            }

            if (camera_fd1 > 0.0f || camera_dof1 > 0.0f || camera2->GetFocalDistance() > 0.0f ||
                camera2->GetDepthOfField() > 0.0f) {
                if (eAttachDepthIntoAlpha(dma_chain)) {
                    eAttachDepthOfField(dma_chain, eVRAM.GetArea(VRAM_ENVRMAP_FRAME_START, 0), eVRAM.GetArea(VRAM_FOG_PALETTE_START, 0));
                }
            }

            if (Path3TextureTransferEnabled) {
                if (TextureDownloader.AddPath3GateDmaTag(dma_chain, dma_chain)) {
                    AttachDummyTexture(tex_chain, 1);
                }
            }

            AttachSolidPolys(dma_chain, tex_chain);
            AttachAlphaPolys(dma_chain, tex_chain);

            for (int view_id = EVIEW_PLAYER1; view_id < EVIEW_PLAYER1_RVM; view_id++) {
                eView *view = eGetView(view_id, false);
                eRenderTarget *render_target = view->GetRenderTarget();

                eSetCurrentRenderTarget(render_target);
                if (view->IsActive() && render_target && render_target->IsActive()) {
                    for (int i = 0; i < view->Precipitation->OSrain.GetNumOnScreen(); i++) {
                        float x;
                        float y;
                        float decay;
                        float size;

                        view->Precipitation->OSrain.GetData(i, &x, &y, &decay, &size, 0);
                        RainDrop(view, x * 512.0f, render_target->ScissorH * y, decay, size);
                    }
                    AttachAlphaPolys(dma_chain, tex_chain);
                    AttachSolidPolys(dma_chain, tex_chain);
                }
            }
        }

        if (TheGameFlowManager.IsInGame() && !TheGameFlowManager.IsLoading()) {
            if (Path3TextureTransferEnabled) {
                if (TextureDownloader.AddPath3GateDmaTag(dma_chain, dma_chain)) {
                    AttachDummyTexture(tex_chain, 1);
                }
            }

            TextureDownloader.setStartEnd(eVRAM.GetArea(VRAM_START_SPECIAL, 1) / 256, 0);

            for (int view_id = EVIEW_PLAYER1; view_id < EVIEW_PLAYER1_RVM; view_id++) {
                eView *view = eGetView(view_id, false);

                if (view->IsActive() && view->GetRenderTarget() && view->GetRenderTarget()->IsActive()) {
                    if (view->ScreenEffects->IsActive(SE_GLARE)) {
                        float intensity =
                            view->ScreenEffects->GetIntensity(SE_GLARE) * (1.0f - view->Precipitation->GetRainIntensity());

                        if (intensity > 0.02f) {
                            eAttachBloom4(view, dma_chain, tex_chain, intensity * 0.5f,
                                          view->ScreenEffects->GetDATA(SE_GLARE, 0), 1);
                        }
                    } else {
                        GetSunIntensity(view);

                        bVector3 campos = *view->GetCamera()->GetPosition();
                        bVector3 camdir = *view->GetCamera()->GetDirection();
                        bVector3 sunpos;

                        GetSunPos(view, &sunpos.x, &sunpos.y, &sunpos.z);
                        sunpos = sunpos - campos;
                        bNormalize(&sunpos, &sunpos);

                        float intensity = indep_fpow(bDot(&sunpos, &camdir), 12.0f) * 0.2f *
                                          (1.0f - view->Precipitation->GetCloudIntensity());

                        if (intensity > 0.0f) {
                            eAttachBloom4(view, dma_chain, tex_chain, intensity, 0.0f, 0);
                        }
                    }
                }
            }

            eGetView(EVIEW_PLAYER1, false)->facePixelation->Render();
            eAttachPIXELATE(eGetView(EVIEW_PLAYER1, false), dma_chain, tex_chain);

            for (int view_id = EVIEW_PLAYER1; view_id <= EVIEW_PLAYER2; view_id++) {
                eView *view = eGetView(view_id, false);
                eRenderTarget *render_target = view->GetRenderTarget();

                if (view->IsActive() && render_target && render_target->IsActive()) {
                    ScreenEffectDB *screen_effects = view->ScreenEffects;

                    if (screen_effects->IsActive(SE_TINT)) {
                        int alpha = static_cast<int>(screen_effects->GetIntensity(SE_TINT) * 128.0f);

                        if (alpha > 0) {
                            unsigned int r;
                            unsigned int g;
                            unsigned int b;
                            unsigned int a;

                            screen_effects->GetRGBA(SE_TINT, &r, &g, &b, &a);
                            eAttachTargetMoveSprites(dma_chain, render_target, render_target, targ_x, targ_y, targ_u,
                                                     targ_v, 100, alpha, 1, r, g, b, 0, 0,
                                                     SCE_GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1));
                        }
                    }
                    pVisualTreatmentPlat->RenderMWVisualLook(view, dma_chain, tex_chain);
                }
            }
        }
    }

    {
        float avg_cpu = 0.0f;
        float avg_gpu = avg_cpu;

        for (int i = 0; i < 16; i++) {
            avg_cpu += PrevCpuTime[i];
            avg_gpu += PrevGpuTime[i];
        }
        avg_cpu /= 16.0f;
        avg_gpu /= 16.0f;
        AvgCpuTime = avg_cpu;
        AvgGpuTime = avg_gpu;
    }

    if (Path3TextureTransferEnabled) {
        if (TextureDownloader.AddPath3GateDmaTag(dma_chain, dma_chain)) {
            AttachDummyTexture(tex_chain, 1);
        }
    }

    TextureDownloader.setStartEnd(0, 0);
    Path3TextureTransferEnabled = 0;

    {
        eView *view = eGetView(EVIEW_PLAYER1, false);

        eSetCurrentRenderTarget(view->GetRenderTarget());
        DebugMenu::renderBackground();
        DebugMenu::render();
        eDebugRender(view);
        RenderMWDebugTopology(view);
        AttachSolidPolys(dma_chain, tex_chain);
        AttachAlphaPolys(dma_chain, tex_chain);
    }

    SuspendRenderEggs();

    if (TheGameFlowManager.IsInFrontend()) {
        if (Path3TextureTransferEnabled) {
            if (TextureDownloader.AddPath3GateDmaTag(dma_chain, dma_chain)) {
                AttachDummyTexture(tex_chain, 1);
            }
        }
        TextureDownloader.setStartEnd(eVRAM.GetArea(VRAM_START_TEXTURES_FE, eVRAM.GetCurrentLayout()) / 256, 0);
    }

    if (eGetView(EVIEW_PLAYER1, false)->ScreenEffects->IsActive(SE_FE_BLUR)) {
        eAttachBlur(eGetView(EVIEW_FLAILER, false), dma_chain, tex_chain,
                    eGetView(EVIEW_PLAYER1, false)->ScreenEffects->GetIntensity(SE_FE_BLUR), 1.0f, 0);
    }

    {
        eView *view = eGetView(EVIEW_FLAILER, false);

        if (view->IsActive() && view->GetRenderTarget() && view->GetRenderTarget()->IsActive()) {
            eSetCurrentRenderTarget(view->GetRenderTarget());
            DisplayLetterBoxes();
            GiganticPSX2Hack(dma_chain, tex_chain);
            FEManager::Get()->Render();
            AttachSolidPolys(dma_chain, tex_chain);
            AttachAlphaPolys(dma_chain, tex_chain);
            GiganticPSX2Hack(0, 0);
            AttachSolidPolys(dma_chain, tex_chain);
            AttachAlphaPolys(dma_chain, tex_chain);
        }
    }

    ResumeRenderEggs();

    Path3TextureTransferEnabled = 1;

    dma_chain->Terminate(eDmaChain::TERMINATE_ENABLE_3_FLUSHA_INTERUPT);
    tex_chain->Terminate(eDmaChain::TERMINATE_DEFAULT);

    eSetCurrentRenderTarget(0);

    FrameTimingStage0.TotalStuffTime = bGetFixTickerDifference(FrameTimingStage0.TotalStuffTimeStart, bGetTicker());

    {
        int start_time = bGetTicker();

        eWaitUntilRenderingDone();
        WaitBufferSwapTime = bGetFixTickerDifference(start_time, bGetTicker());
    }

    if ((!RenderQueueExecuting || WaitingForBufferSwap) && !RenderQueueExecuting && WaitingForBufferSwap) {
    }

    if (TheGameFlowManager.IsInGame() && LockTo30FPS) {
        while (eVBlankCounter - PreviousVBLCount < 2) {
            WaitUntilVSync();
        }
    }
    PreviousVBLCount = eVBlankCounter;

    eSwapScreenBuffers();

    VRAMView();

    FrameTimingStage0.FrameMallocEnd = static_cast<uint8 *>(eFrameMalloc(0));
    FrameTimingStage0.TotalFrameMallocSize = FrameTimingStage0.FrameMallocEnd - FrameTimingStage0.FrameMallocStart;
    FrameTimingStage1 = FrameTimingStage0;

    FlushCache(0);

    ExecuteRenderStageQueue(IdleRenderStageQueue, 1);

    PatchVUCode();

    ePostDisplay();

    if (DisplayParticleSystemStats) {
        ScreenPrintf(-290, -162, 0xFFFFFFFF, "Particles: %d/%d\n", g_ParticleStats.NumParticles,
                     g_ParticleStats.MaxNumParticles);
        ScreenPrintf(-290, -144, 0xFFFFFFFF, "ParticleTextures: %d/%d\n", g_ParticleStats.NumActiveParticleTextures,
                     g_ParticleStats.MaxNumParticleTextures);
        ScreenPrintf(-290, -126, 0xFFFFFFFF, "Emitters: %d/%d\n", g_ParticleStats.NumEmitters,
                     g_ParticleStats.MaxNumEmitters);
        ScreenPrintf(-290, -108, 0xFFFFFFFF, "EmitterGroups: %d/%d\n", g_ParticleStats.NumEmitterGroups,
                     g_ParticleStats.MaxNumEmitterGroups);
    }

    ResetDmaStats();
    UpdateDmaStats();

    for (int target_id = 0; target_id < TOTAL_RENDER_TARGETS; target_id++) {
        eRenderTarget *render_target = eGetRenderTarget(target_id);

        render_target->ClearZBufferData = 0;
        render_target->DrawContextData[DRAW_CONTEXT_SOLID_NOZ] = 0;
        render_target->DrawContextData[DRAW_CONTEXT_ALPHA_NOZ] = 0;
        render_target->DrawContextData[DRAW_CONTEXT_SOLID_Z] = 0;
        render_target->DrawContextData[DRAW_CONTEXT_ALPHA_Z] = 0;
    }

    TextureDownloader.Close();

    eSwapFrameMallocBuffers();
    eSwapDynamicFrameMemory();

    eFrameCounter++;
}

void GiganticPSX2Hack(void *dma_ptr, void *tex_ptr) {
    GiganticPSX2HackDMAPtr = dma_ptr;
    GiganticPSX2HackTEXPtr = tex_ptr;
}

void FinishedRenderingFEngLayer() {
    eDmaChain *dma_chain = static_cast<eDmaChain *>(GiganticPSX2HackDMAPtr);
    eDmaChain *tex_chain = static_cast<eDmaChain *>(GiganticPSX2HackTEXPtr);

    AttachSolidPolys(dma_chain, tex_chain);
    AttachAlphaPolys(dma_chain, tex_chain);
}

// El mapa de indices de las paletas de 8 bits: el GS guarda las entradas 8-15 y 16-23 de cada bloque de 32 cambiadas.
struct PaletteIndexMap {
    // total size: 0x100
    unsigned char map[256]; // offset 0x0, size 0x100

    PaletteIndexMap() {
        for (int i = 0; i < 256; i++) {
            if ((i & 0x18) == 0) {
                map[i] = i;
            } else if ((i & 0x18) == 8) {
                map[i] = i + 8;
            } else if ((i & 0x18) == 16) {
                map[i] = i - 8;
            } else if ((i & 0x18) == 24) {
                map[i] = i;
            }
        }
    }
    int GetIndex(int i) {
        return map[i];
    }
};

static PaletteIndexMap palette_index_map;

inline uint32 GetUnswizzledColour(uint32 *palette, int index) {
    return palette[palette_index_map.GetIndex(index)];
}

inline void SetSwizzledColour(uint32 *new_palette, uint32 *palette, int index) {
    uint32 colour = palette[index];

    new_palette[palette_index_map.GetIndex(index)] = colour;
}

void eUnSwizzle8bitPalette(uint32 *palette) {
    unsigned int new_palette[256];

    for (int i = 0; i < 256; i++) {
        new_palette[i] = GetUnswizzledColour(palette, i);
    }

    bMemCpy(palette, new_palette, sizeof(new_palette));
}

void eSwizzle8bitPalette(uint32 *palette) {
    unsigned int new_palette[256];

    for (int i = 0; i < 256; i++) {
        SetSwizzledColour(new_palette, palette, i);
    }

    bMemCpy(palette, new_palette, sizeof(new_palette));
}

u_long128 OverFlow0[15];
u_long128 OverFlow1[15];

void AttachDummyTexture(eDmaChain *dma_chain, int eop) {
    DATA16 *pbuf_start;
    DATA16 *pbuf;

    if (Path3TextureTransferEnabled) {
        pbuf_start = static_cast<DATA16 *>(eFrameMalloc(9 * sizeof(DATA16)));

        if (pbuf_start || eop) {
            if (!pbuf_start) {
                if (eFrameCounter & 1) {
                    pbuf_start = reinterpret_cast<DATA16 *>(OverFlow0);
                } else {
                    pbuf_start = reinterpret_cast<DATA16 *>(OverFlow1);
                }
            }

            pbuf_start[0].d[0] = MakeDmaTag(6, eDMA_CNT);
            pbuf_start[0].w[2] = SCE_VIF1_SET_FLUSHE(0);
            pbuf_start[0].w[3] = Path3TextureTransferEnabled ? 0 : SCE_VIF1_SET_DIRECT(6, 0);

            pbuf_start[1].d[0] = MakeGifTag(4, 0, 0, 0, GIF_PACKED, 1);
            pbuf_start[1].d[1] = SCE_GIF_PACKED_AD;

            pbuf = pbuf_start + 2;

            pbuf->d[0] = SCE_GS_SET_BITBLTBUF(0, 0, 0, eVRAM.GetArea(VRAM_DUMMY_8K, eVRAM.GetCurrentLayout()) / 256, 1, 0);
            pbuf->d[1] = SCE_GS_BITBLTBUF;
            pbuf++;

            pbuf->d[0] = SCE_GS_SET_TRXPOS(0, 0, 0, 0, 0);
            pbuf->d[1] = SCE_GS_TRXPOS;
            pbuf++;

            pbuf->d[0] = SCE_GS_SET_TRXREG(64, 32);
            pbuf->d[1] = SCE_GS_TRXREG;
            pbuf++;

            pbuf->d[0] = SCE_GS_SET_TRXDIR(0);
            pbuf->d[1] = SCE_GS_TRXDIR;
            pbuf++;

            pbuf->d[0] = MakeGifTag(512, eop, 0, 0, GIF_IMAGE, 0);
            pbuf->d[1] = 0;
            pbuf++;

            pbuf->d[0] = MakeDmaTag(512, eDMA_REF, vu1MicroCodeStart);
            pbuf->w[2] = 0;
            pbuf->w[3] = Path3TextureTransferEnabled ? 0 : SCE_VIF1_SET_DIRECT(512, 0);
            pbuf++;

            pbuf->d[0] = MakeDmaTag(0, eDMA_RET);
            pbuf->w[2] = 0;
            pbuf->w[3] = 0;

            dma_chain->Add(pbuf_start, pbuf);
        }
    }
}

void _eAddVertex(const float *v) {
    eAddVertex(v[0], v[1], v[2]);
}

#ifdef EA_BUILD_A124
// Una tira de lineas de depuracion: cada segmento es un quad de 1,5 pixeles de grueso con la textura blanca. SLUS no la
// tiene.
void eDrawKLine(const ePolyVertex *verts, unsigned int numLines) {
    eView *view = eGetView(EVIEW_PLAYER1, false);
    TextureInfo *texture_info = GetTextureInfo(bStringHash("WHITE16X16"), 1, 0);
    ePoly poly;

    poly.Vertices[0].z = poly.Vertices[1].z = poly.Vertices[2].z = poly.Vertices[3].z = 524288.0f;
    poly.UVs[0][0] = verts->u; poly.UVs[0][1] = verts->v;
    poly.UVs[1][0] = verts->u; poly.UVs[1][1] = verts->v;
    poly.UVs[2][0] = verts->u; poly.UVs[2][1] = verts->v;
    poly.UVs[3][0] = verts->u; poly.UVs[3][1] = verts->v;

    for (unsigned int i = 0; i < numLines; i++) {
        poly.Vertices[0].x = verts[0].x; poly.Vertices[0].y = verts[0].y;
        poly.Vertices[1].x = verts[1].x; poly.Vertices[1].y = verts[1].y;
        poly.Vertices[2].x = verts[1].x + 1.5f; poly.Vertices[2].y = verts[1].y + 1.5f;
        poly.Vertices[3].x = verts[0].x + 1.5f; poly.Vertices[3].y = verts[0].y + 1.5f;

        *reinterpret_cast<uint32 *>(poly.Colours[0]) = *reinterpret_cast<uint32 *>(poly.Colours[1]) =
            *reinterpret_cast<uint32 *>(poly.Colours[2]) = *reinterpret_cast<uint32 *>(poly.Colours[3]) = verts->colour;

        view->Render(&poly, texture_info, 0);
        verts++;
    }
}
#endif
