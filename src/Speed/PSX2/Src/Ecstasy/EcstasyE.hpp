#ifndef PSX2_ECSTASY_ECSTASYE_H
#define PSX2_ECSTASY_ECSTASYE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Ecstasy/EcstasyData.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bList.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"

struct eRenderTarget;
struct eDmaChain;
struct eRenderStageQueue;
struct IVisualTreatmentPlat;
struct TextureInfo;
class eView;
class eLightMaterial;
class eLightContext;
struct SlotPool;

// GameCube incluye esta cabecera (FEPkg_GarageMain.cpp) y no tiene las de Sony: los tipos, que usan los de 128 bits
// de DmaE.hpp, solo existen en PS2. Los prototipos de abajo bastan con las declaraciones adelantadas.
#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/PSX2/Inc/DmaE.hpp"

// psx2/src/ecstasy/EcstasyE.hpp. Los tipos, en el orden de los stabs de la A124 (TARGET_ID va en EcstasyData.hpp):
// RenderTargetDrawContext, eRenderTargetMatrixPack, eRenderTarget, EDMA_TERMINATE, eDmaChain, eDmaType, eSNode,
// eDmaData, eSList, eDmaDataList, eActiveTexture, VramFillInfo, eStaticTexture y eTextureDownloader.

enum RenderTargetDrawContext {
    DRAW_CONTEXT_SOLID = 0,
    DRAW_CONTEXT_ALPHA = 1,
    DRAW_CONTEXT_WRITE_Z = 2,
    DRAW_CONTEXT_NOWRITE_Z = 0,
    DRAW_CONTEXT_SPECIAL = 4,
    DRAW_CONTEXT_SOLID_MASK = 1,
    DRAW_CONTEXT_ZWRITE_MASK = 2,
    DRAW_CONTEXT_SOLID_Z = 2,
    DRAW_CONTEXT_ALPHA_Z = 3,
    DRAW_CONTEXT_SOLID_NOZ = 0,
    DRAW_CONTEXT_ALPHA_NOZ = 1,
};

struct eRenderTargetMatrixPack {
    // total size: 0x130
    bMatrix4 WorldClipGB;   // offset 0x0, size 0x40
    bVector4 ClipGBScreen0; // offset 0x40, size 0x10
    bVector4 ClipGBScreen1; // offset 0x50, size 0x10
    bVector4 ScreenClipGB0; // offset 0x60, size 0x10
    bVector4 ScreenClipGB1; // offset 0x70, size 0x10
    bMatrix4 ViewWorld;     // offset 0x80, size 0x40
    bMatrix4 ScreenWorld;   // offset 0xC0, size 0x40
    bVector4 GenericData;   // offset 0x100, size 0x10
    bVector4 CameraPos;     // offset 0x110, size 0x10
    bVector4 SunDirection;  // offset 0x120, size 0x10
};

// El layout entero sale de los stabs de la A124 (scripts/clase_stabs.py): tamano, nombre y offset de cada campo
// y la lista de accesores.
struct eRenderTarget {
    // total size: 0xA90
    TARGET_ID ID;                          // offset 0x0, size 0x4
    const char *Name;                      // offset 0x4, size 0x4
    int Active;                            // offset 0x8, size 0x4
    int RelativeToFrameBuffer;             // offset 0xC, size 0x4
    int CenterRelativeToFrame;             // offset 0x10, size 0x4
    int ScissorX;                          // offset 0x14, size 0x4
    int ScissorY;                          // offset 0x18, size 0x4
    int ScissorW;                          // offset 0x1C, size 0x4
    int ScissorH;                          // offset 0x20, size 0x4
    int FrameMode;                         // offset 0x24, size 0x4
    int FrameAddress;                      // offset 0x28, size 0x4
    int FrameWriteMask;                    // offset 0x2C, size 0x4
    int FrameWidth;                        // offset 0x30, size 0x4
    int FrameHeight;                       // offset 0x34, size 0x4
    int ZBufferMode;                       // offset 0x38, size 0x4
    int ZBufferAddress;                    // offset 0x3C, size 0x4
    int ZBufferMask;                       // offset 0x40, size 0x4
    int ZTest;                             // offset 0x44, size 0x4
    int dthe;                              // offset 0x48, size 0x4
    int PaletteAddress;                    // offset 0x4C, size 0x4
    int PaletteMode;                       // offset 0x50, size 0x4
    int AlphaTest;                         // offset 0x54, size 0x4
    int FBA;                               // offset 0x58, size 0x4
    int PixelOffsetType;                   // offset 0x5C, size 0x4
    float AspectRatioX;                    // offset 0x60, size 0x4
    float AspectRatioY;                    // offset 0x64, size 0x4
    float AspectRatio2DX;                  // offset 0x68, size 0x4
    float AspectRatio2DY;                  // offset 0x6C, size 0x4
    float FovBias;                         // offset 0x70, size 0x4
    unsigned char BackgroundColour[4];     // offset 0x74, size 0x4
    int ClearBackground;                   // offset 0x78, size 0x4
    float ZMax;                            // offset 0x7C, size 0x4
    unsigned int *ClearZBufferData;        // offset 0x80, size 0x4
    unsigned int *DrawContextData[5];      // offset 0x84, size 0x14
    u_long128 UgiftagP[2][78];             // offset 0xA0, size 0x9C0
    eRenderTargetMatrixPack *MatrixPack0;  // offset 0xA60, size 0x4
    eRenderTargetMatrixPack *MatrixPack1;  // offset 0xA64, size 0x4
    eRenderTargetMatrixPack *MatrixPack2;  // offset 0xA68, size 0x4
    bMatrix4 *WorldScreen;                 // offset 0xA6C, size 0x4
    bMatrix4 *WorldView;                   // offset 0xA70, size 0x4
    bMatrix4 *WorldClipGB;                 // offset 0xA74, size 0x4
    bMatrix4 *ClipGBScreen;                // offset 0xA78, size 0x4
    bMatrix4 *VUClipGBScreen0;             // offset 0xA7C, size 0x4
    bMatrix4 *VUClipGBScreen1;             // offset 0xA80, size 0x4
    bMatrix4 *VUClipGBScreen2;             // offset 0xA84, size 0x4

    const char *GetName() {
        return Name;
    }
    void SetName(const char *name) {
        Name = name;
    }
    int IsActive() {
        return Active;
    }
    void SetActive(int active) {
        Active = active;
    }
    int GetID() {
        return ID;
    }
    void SetID(int id) {
        ID = static_cast<TARGET_ID>(id);
    }
    void SetBackgroundColour(unsigned int colour) {
        *reinterpret_cast<unsigned int *>(BackgroundColour) = colour;
    }
    int GetBackgroundColour() {
        return *reinterpret_cast<int *>(BackgroundColour);
    }
    TextureInfo *GetTextureInfo(int index);
    void UpdateGifTags(int half_offset_x, int half_offset_y, int quarter_offset_x, int quarter_offset_y,
                       eRenderStageQueue *render_stage_queue, int fast);
    void UpdateGifTagsOLD(int half_offset_x, int half_offset_y, int quarter_offset_x, int quarter_offset_y,
                          eRenderStageQueue *render_stage_queue, int fast);
    float GetFovBias() {
        return FovBias;
    }
    void SetFovBias(float bias) {
        FovBias = bias;
    }
    void SetupMatrixPacks();
    unsigned int *GetDrawContextData(int draw_context) {
        return DrawContextData[draw_context];
    }
    unsigned int *GetClearZBufferData() {
        return ClearZBufferData;
    }
};

// Una cadena de DMA: el tag de cabeza y el ultimo tag enlazado, cuyo campo de direccion rellena el siguiente Add.
// EDMA_TERMINATE es un tipo anidado: el nombre decorado de Terminate lo dice (Q29eDmaChain14EDMA_TERMINATE).
struct eDmaChain {
    // total size: 0x20
    enum EDMA_TERMINATE {
        TERMINATE_DEFAULT = 0,
        TERMINATE_INTERUPT = 1,
        TERMINATE_FLUSHA = 2,
        TERMINATE_FLUSHA_INTERUPT = 3,
        TERMINATE_ENABLE_3_FLUSHA_INTERUPT = 4,
    };

    u_long128 BeginingDmaTag;  // offset 0x0, size 0x10
    u_long128 *CurrentDmaTag;  // offset 0x10, size 0x4

    void Init();
    void Terminate(EDMA_TERMINATE flag);
    void Add(void *packet_start);
    void Add(void *packet_start, void *packet_end);
    void AddHead(void *packet_start);
    void AddHead(void *packet_start, void *packet_end);
};

enum eDmaType {
    EDMA_STRIP = 0,
    EDMA_SETTARGET = 1,
    EDMA_LIGHTMATERIAL = 2,
    EDMA_LIGHTCONTEXT = 3,
    EDMA_DIRECT = 4,
    EDMA_POLY3D = 5,
    EDMA_FLUSH = 6,
    EDMA_VERTEXALPHA = 7,
    EDMA_SET_MATRIX_PACK = 8,
    EDMA_SET_BONE_MATRIX_PACK = 9,
    EDMA_SET_TEXTURE_CONTEXT = 10,
    EDMA_SET_RENDER_TARGET = 11,
    EDMA_SET_LOCAL_WORLD = 12,
    EDMA_SET_LIGHT_CONTEXT = 13,
};

// Lista enlazada simple de EcstasyE.hpp (lineas 259-282 de la A124): Reset 271, IsEmpty 273, GetHead 275,
// AddTail 278-279 y AddHead 281-282.
template <typename T> struct eSNode {
    // total size: 0x4
    T *Next; // offset 0x0, size 0x4

    void Init() {
        Next = 0;
    }
    T *GetNext() {
        return Next;
    }
};

template <typename T> struct eSList {
    // total size: 0x8
    T *Head;     // offset 0x0, size 0x4
    T **TailPtr; // offset 0x4, size 0x4

    void Reset() {
        Head = 0;
        TailPtr = &Head;
    }
    int IsEmpty() {
        return Head == 0;
    }
    T *GetHead() {
        return Head;
    }
    T *EndOfList() {
        return 0;
    }
    T *AddTail(T *node) {
        eSNode<T> *snode = node;
        *TailPtr = node;
        snode->Next = 0;
        TailPtr = &snode->Next;
        return node;
    }
    T *AddHead(T *node) {
        if (IsEmpty()) {
            return AddTail(node);
        }
        eSNode<T> *snode = node;
        snode->Next = Head;
        Head = node;
        return node;
    }
};

struct eDmaData : public eSNode<eDmaData> {
    // total size: 0x10
    unsigned short Type;      // offset 0x4, size 0x2
    unsigned short Parameter; // offset 0x6, size 0x2
    unsigned short QWordSize; // offset 0x8, size 0x2
    unsigned short pad;       // offset 0xA, size 0x2
    unsigned char *pData;     // offset 0xC, size 0x4
};

struct eDmaDataList : public eSList<eDmaData> {
    // total size: 0x20
    eLightMaterial *LightMaterial;        // offset 0x8, size 0x4
    eLightContext *LightContext;          // offset 0xC, size 0x4
    bMatrix4 *LocalWorldMatrix;           // offset 0x10, size 0x4
    eRenderTargetMatrixPack *MatrixPack;  // offset 0x14, size 0x4
    bMatrix4 *BoneMatrixPack;             // offset 0x18, size 0x4
    unsigned int pad0;                    // offset 0x1C, size 0x4

    void Init();
    eDmaData *AddData(unsigned short type, void *data, unsigned short qwsize, unsigned short parameter);
    void AddDataHead(unsigned short type, void *data, unsigned short qwsize, unsigned short parameter);
    void SetLocalWorldMatrix(bMatrix4 *matrix);
    void SetLightContext(eLightContext *light_context);
    void SetLightMaterial(eLightMaterial *light_material);
    void SetMatrixPack(eRenderTargetMatrixPack *matrix_pack);
    void SetBoneMatrixPack(bMatrix4 *bone_matrix_pack);
};

extern SlotPool *ActiveTextureSlotPool;

struct eActiveTexture : public bTNode<eActiveTexture> {
    // total size: 0x70
    TextureInfo *pTextureInfo;        // offset 0x8, size 0x4
    unsigned int *pPreviousDmaData;   // offset 0xC, size 0x4
    eDmaDataList *pDmaDataList[21];   // offset 0x10, size 0x54
    int Padding[3];                   // offset 0x64, size 0xC

    void *operator new(size_t size) {
        return bOMalloc(ActiveTextureSlotPool);
    }
    void operator delete(void *ptr) {
        bFree(ActiveTextureSlotPool, ptr);
    }
    eActiveTexture(TextureInfo *texture_info);
    ~eActiveTexture();
    eDmaDataList *GetDmaDataList(int render_target_id, int create);
};

eActiveTexture *eCreateActiveTexture(TextureInfo *texture_info);

// La textura activa de un TextureInfo (lineas 467-475 del EcstasyE.hpp de la A124); se crea si no la tiene. El nombre
// no queda en los stabs.
inline eActiveTexture *eGetActiveTexture(TextureInfo *texture_info) {
    eActiveTexture *active_texture = static_cast<eActiveTexture *>(texture_info->GetPlatInfo()->pActiveTexture);

    if (active_texture) {
        return active_texture;
    }
    return eCreateActiveTexture(texture_info);
}

struct VramFillInfo {
    // total size: 0x10
    int StartAddress;      // offset 0x0, size 0x4
    int DesiredEndAddress; // offset 0x4, size 0x4
    int EndAddress;        // offset 0x8, size 0x4
    int FillAddress;       // offset 0xC, size 0x4
};

struct eStaticTexture : public bTNode<eStaticTexture> {
    // total size: 0x10
    TextureInfo *Texture;                 // offset 0x8, size 0x4
    unsigned short ImageDownloadAddress;  // offset 0xC, size 0x2
    unsigned short PaletteDownloadAddress; // offset 0xE, size 0x2
};

struct eTextureDownloader {
    // total size: 0x80
  private:
    int VramBlockStart;                     // offset 0x0, size 0x4
    int VramBlockTop;                       // offset 0x4, size 0x4
    int oldVramBlockStart;                  // offset 0x8, size 0x4
    int oldVramBlockTop;                    // offset 0xC, size 0x4
    int VramBlockBoundary;                  // offset 0x10, size 0x4
    int DownloadingPhase;                   // offset 0x14, size 0x4
    int LockedVramBlockStart;               // offset 0x18, size 0x4
    int LockedVramBlockEnd;                 // offset 0x1C, size 0x4
    int SyncRequired;                       // offset 0x20, size 0x4
    bTList<eActiveTexture> DownloadingTextureList; // offset 0x24, size 0x8
    SlotPool *StaticTextureSlotPool;        // offset 0x2C, size 0x4
    bTList<eStaticTexture> StaticTextureList; // offset 0x30, size 0x8

  public:
    int PrevNumTextures;          // offset 0x38, size 0x4
    int NumTextures;              // offset 0x3C, size 0x4
    int PrevNumPalettes;          // offset 0x40, size 0x4
    int NumPalettes;              // offset 0x44, size 0x4
    int PrevNumTextureBytes;      // offset 0x48, size 0x4
    int NumTextureBytes;          // offset 0x4C, size 0x4
    int PrevNumPaletteBytes;      // offset 0x50, size 0x4
    int NumPaletteBytes;          // offset 0x54, size 0x4
    int PrevNumUnalignedTextures; // offset 0x58, size 0x4
    int NumUnalignedTextures;     // offset 0x5C, size 0x4
    int PrevLess8k;               // offset 0x60, size 0x4
    int Less8k;                   // offset 0x64, size 0x4
    int PrevNumSwizzled;          // offset 0x68, size 0x4
    int NumSwizzled;              // offset 0x6C, size 0x4
    int PrevNumSyncs;             // offset 0x70, size 0x4
    int NumSyncs;                 // offset 0x74, size 0x4
    int PrevNumBatches;           // offset 0x78, size 0x4
    int NumBatches;               // offset 0x7C, size 0x4

    void GlobalInit(int vram_block_start, int vram_block_end);
    void GlobalClose();
    void Init(int vram_block_start, int vram_block_end);
    void setStartEnd(int vram_block_start, int vram_block_end);
    void Close();
    eActiveTexture *GetFirstTexture() {
        return DownloadingTextureList.GetHead();
    }
    eActiveTexture *GetLastTexture() {
        return DownloadingTextureList.GetTail();
    }
    eActiveTexture *EndOfList() {
        return DownloadingTextureList.EndOfList();
    }
    int IsFinishedDownloading() {
        return DownloadingTextureList.IsEmpty();
    }
    int IsSyncRequired() {
        return SyncRequired;
    }
    void AddTexturesSorted(bTList<eActiveTexture> *texture_list);
    void AddTexturesUnsorted(bTList<eActiveTexture> *texture_list);
    void AddStaticTextureForDownload(TextureInfo *texture_info, unsigned short image_address,
                                     unsigned short palette_address);
    eActiveTexture *DownloadSingleBuffered(eDmaChain *dma_chain, eDmaChain *texture_dma_chain);
    eActiveTexture *DownloadDoubleBuffered(eDmaChain *dma_chain, eDmaChain *texture_dma_chain, bool first);
    void DeleteDownloadedTextures(eActiveTexture *last_texture);
    int AddPath3GateDmaTag(eDmaChain *dma_chain, eDmaChain *texture_dma_chain);
    void Print(const char *title);

  private:
    eActiveTexture *FillVramWithTextures(eDmaChain *tex_chain, eDmaChain *dma_chain2, eActiveTexture *current_texture,
                                         VramFillInfo *vram_fill_info, bool forcePageAlign);
    void FillVramWithStaticTextures(eDmaChain *dma_chain);
};

// Los tipos que EcstasyE.cpp usa (los stabs de la A124 los dan al principio de su parte de zEcstasy).
struct eProfileTime {
    // total size: 0x8
    bFix Time;      // offset 0x0, size 0x4
    bFix TotalTime; // offset 0x4, size 0x4

    void Start();
    void End();
    void Reset();
};

struct eFrameTiming {
    // total size: 0x70
    eProfileTime ebInitializeTime;    // offset 0x0, size 0x8
    eProfileTime efStaticTextureTime; // offset 0x8, size 0x8
    eProfileTime efStuffAllScenery;   // offset 0x10, size 0x8
    eProfileTime efStuffNIS;          // offset 0x18, size 0x8
    eProfileTime efStuffSun;          // offset 0x20, size 0x8
    eProfileTime efStuffClans;        // offset 0x28, size 0x8
    eProfileTime efStuffAcidFX;       // offset 0x30, size 0x8
    eProfileTime efStuffEnvMaps;      // offset 0x38, size 0x8
    uint8 *FrameMallocStart;          // offset 0x40, size 0x4
    uint8 *FrameMallocEnd;            // offset 0x44, size 0x4
    uint32 TotalFrameMallocSize;      // offset 0x48, size 0x4
    uint32 TotalStripBytes;           // offset 0x4C, size 0x4
    uint32 TotalMatrixBytes;          // offset 0x50, size 0x4
    uint32 TotalDirectDataBytes;      // offset 0x54, size 0x4
    int32 TotalStuffTimeStart;        // offset 0x58, size 0x4
    bFix TotalStuffTime;              // offset 0x5C, size 0x4
    int32 TotalDrawTimeStart;         // offset 0x60, size 0x4
    bFix TotalDrawTime;               // offset 0x64, size 0x4
    int32 TotalTextures;              // offset 0x68, size 0x4
    int32 TotalTextureBytes;          // offset 0x6C, size 0x4
};

enum eTransferRectStages {
    TRANSFER_RECT_STAGE_IDLE = 0,
    TRANSFER_RECT_STAGE_1 = 1,
    TRANSFER_RECT_STAGE_2 = 2,
};

struct eTransferRect : public bTNode<eTransferRect> {
    // total size: 0x1C
    uint8 *DestinationBuffer; // offset 0x8, size 0x4
    uint16 SourceAddress;     // offset 0xC, size 0x2
    uint16 SourceWidth;       // offset 0xE, size 0x2
    uint16 PixelMode;         // offset 0x10, size 0x2
    uint16 QWC;               // offset 0x12, size 0x2
    uint16 PixelX;            // offset 0x14, size 0x2
    uint16 PixelY;            // offset 0x16, size 0x2
    uint16 PixelWidth;        // offset 0x18, size 0x2
    uint16 PixelHeight;       // offset 0x1A, size 0x2
};

struct eTransferRectQueue {
    // total size: 0xFC
    bTList<eTransferRect> ActiveList; // offset 0x0, size 0x8
    bTList<eTransferRect> IdleList;   // offset 0x8, size 0x8
    eTransferRect *CurrentTransfer;   // offset 0x10, size 0x4
    uint32 CurrentTransferStage;      // offset 0x14, size 0x4
    int32 CurrentDMACHandlerID;       // offset 0x18, size 0x4
    eTransferRect TransferRects[8];   // offset 0x1C, size 0xE0

    eTransferRectQueue();
    eTransferRect *GetTransferRect();
};

struct ePatchOffset : public bTNode<ePatchOffset> {
    // total size: 0x18
    u_long OriginalValue; // offset 0x8, size 0x8
    u_long *PatchOffset;  // offset 0x10, size 0x4
    uint32 pad0;          // offset 0x14, size 0x4
};

struct eRenderStageQueue {
    // total size: 0x60
    uint32 StartTime;                        // offset 0x0, size 0x4
    uint32 EndTime;                          // offset 0x4, size 0x4
    eDmaChain DataDmaChain;                  // offset 0x10, size 0x20
    eDmaChain TextureDmaChain;               // offset 0x30, size 0x20
    bTList<ePatchOffset> PatchXYOffsetList;  // offset 0x50, size 0x8

    void AddPatchXYOffset(u_long *xyoffset);
};

struct tagBufferParams {
    // total size: 0x5C
    int32 HalfOffsetX;    // offset 0x0, size 0x4
    int32 HalfOffsetY;    // offset 0x4, size 0x4
    int32 QuarterOffsetX; // offset 0x8, size 0x4
    int32 QuarterOffsetY; // offset 0xC, size 0x4
    int32 enable1;        // offset 0x10, size 0x4
    int32 sdx1;           // offset 0x14, size 0x4
    int32 sdy1;           // offset 0x18, size 0x4
    int32 dbx1;           // offset 0x1C, size 0x4
    int32 dby1;           // offset 0x20, size 0x4
    int32 magh1;          // offset 0x24, size 0x4
    int32 magv1;          // offset 0x28, size 0x4
    int32 dw1;            // offset 0x2C, size 0x4
    int32 dh1;            // offset 0x30, size 0x4
    int32 enable2;        // offset 0x34, size 0x4
    int32 sdx2;           // offset 0x38, size 0x4
    int32 sdy2;           // offset 0x3C, size 0x4
    int32 dbx2;           // offset 0x40, size 0x4
    int32 dby2;           // offset 0x44, size 0x4
    int32 magh2;          // offset 0x48, size 0x4
    int32 magv2;          // offset 0x4C, size 0x4
    int32 dw2;            // offset 0x50, size 0x4
    int32 dh2;            // offset 0x54, size 0x4
    int32 blend;          // offset 0x58, size 0x4
};

typedef tagBufferParams BufferParams;

enum mipmapFliter {
    MIPMAP_DEFAULT = 0,
    MIPMAP_POINT = 1,
    MIPMAP_NEAREST = 2,
    MIPMAP_BILINEAR = 3,
    MIPMAP_TRILINEAR = 4,
};

enum mipmapLOD {
    MIPMAP_DYNAMIC = 0,
    MIPMAP_0 = 1,
    MIPMAP_1 = 2,
    MIPMAP_2 = 3,
    MIPMAP_3 = 4,
    MIPMAP_4 = 5,
    MIPMAP_5 = 6,
};

extern eRenderTarget RenderTargets[24];
extern u_long128 SentinelDmaTags[5][2];
// Los definen EcstasyE.cpp (los usan tambien eEnvEffectsE.cpp y SunE.cpp).
extern eTextureDownloader TextureDownloader;
extern bTList<eActiveTexture> SolidTextureList;
extern bTList<eActiveTexture> AlphaTextureList;
extern eRenderTarget *volatile MainDisp0Context;
extern eRenderTarget *volatile MainDraw0Context;
extern TextureInfo *pTextureInfoRainDropMask;
extern VIDEO_MODE eCurrentVideoMode;
// Las zonas de la VRAM y su gestor: los define EcstasyE.cpp y los usan tambien los efectos (eVisualTreatment.cpp).
enum VRAMbits {
    VRAM_FRAMEBUFFER0 = 0,
    VRAM_FRAMEBUFFER1 = 1,
    VRAM_ZBUFFER = 2,
    VRAM_ENVMAP0_FULL = 3,
    VRAM_FRAMEBUFFER2 = 4,
    VRAM_ENVRMAP_FRAME_START = 5,
    VRAM_ENVRMAP0B_FRAME = 6,
    VRAM_ENVRMAP0L_FRAME = 7,
    VRAM_ENVRMAP0F_FRAME = 8,
    VRAM_ENVRMAP0R_FRAME = 9,
    VRAM_ENVRMAP0U_FRAME = 10,
    VRAM_ENVRMAP0D_FRAME = 11,
    VRAM_ENVRMAP_ZBUF_START = 12,
    VRAM_ENVRMAP0B_ZBUF = 13,
    VRAM_ENVRMAP0L_ZBUF = 14,
    VRAM_ENVRMAP0F_ZBUF = 15,
    VRAM_ENVRMAP0R_ZBUF = 16,
    VRAM_ENVRMAP0U_ZBUF = 17,
    VRAM_ENVRMAP0D_ZBUF = 18,
    VRAM_RVM1_ZBUF = 19,
    VRAM_RVM1_FRAME = 20,
    VRAM_REFLECTION = 21,
    VRAM_SPECULAR = 22,
    VRAM_REFLECTION_ZBUF = 23,
    VRAM_GLOSSYWINDOW = 24,
    VRAM_GLOSSYWINDOW_PALETTE = 25,
    VRAM_SMALL_ENVMAP = 26,
    VRAM_SMALL_ENVMAP_PALETTE = 27,
    VRAM_DUMMY_8K = 28,
    VRAM_COMBINER = 29,
    VRAM_RAMP_PALETTE_START = 30,
    VRAM_FOG_PALETTE_START = 31,
    VRAM_START_TEXTURES_FE_CARSEL = 32,
    VRAM_START_TEXTURES_FE = 33,
    VRAM_START_TEXTURES = 34,
    VRAM_256_A = 35,
    VRAM_256_B = 36,
    VRAM_256_C = 37,
    VRAM_256_D = 38,
    VRAM_128_A = 39,
    VRAM_128_B = 40,
    VRAM_128_C = 41,
    VRAM_128_D = 42,
    VRAM_START_SPECIAL = 43,
    VRAM_DUMMY = 44,
    NUM_VRAM_AREAS = 45,
};

struct eVRAMArea {
    // total size: 0x10
    int32 Valid;   // offset 0x0, size 0x4
    int32 Address; // offset 0x4, size 0x4
    int32 Size;    // offset 0x8, size 0x4
    char *Name;    // offset 0xC, size 0x4
};

struct eVRAMLayout {
    // total size: 0x2DC
    eVRAMArea Areas[45];  // offset 0x0, size 0x2D0
    int32 CurrentAddress; // offset 0x2D0, size 0x4
    int32 MaxSize;        // offset 0x2D4, size 0x4
    const char *Name;     // offset 0x2D8, size 0x4
};

struct eVRAMmanager {
    // total size: 0x894
  private:
    eVRAMLayout Layouts[3]; // offset 0x0, size 0x894

  public:
    void Init();
    void SetLayout(int layout_id, int max_size, const char *name);
    void AddArea(int layout_id, int src, int amount, int align, char *name);
    void UseArea(int layout_id, int src, int dst, int amount, int align, char *name);
    int32 GetArea(int area_id, int layout_id);
    int GetCurrentLayout();
    void PrintLayouts();
    void PrintUsage();
};

extern eVRAMmanager eVRAM;
// Las paletas de los efectos: se suben a la VRAM con un paquete del PATH2 y se reordenan al orden de CLUT de 8 bits
// (eEnvEffectsE.cpp).
void AttachImageDataPacketDMA(eDmaChain *dma_chain, void *image_data, unsigned int vram_address, unsigned int qwsize,
                              unsigned int psm, unsigned int tbw, unsigned int width, unsigned int height, int eop);
void RemapLinearIntoClut8(uint32 *output, uint32 *input);
// Usa tipos de PS2 (eActiveTexture): tiene que ir dentro del bloque de PS2, que GameCube incluye esta cabecera.
void AttachAllPolysNoTexUpload(eActiveTexture *first_active_texture, eActiveTexture *last_active_texture,
                               eDmaChain *dma_chain, eDmaChain *tex_chain);
// Los tipos de niebla de eAttachFog (eEnvEffectsE.cpp); eDisplayFrame la llama con la niebla lejana.
enum FogVolumeType {
    DISTANT_FOG = 0,
    HORIZON_FOG = 1,
    VOLUME_FOG = 2,
    HEADLIGHT_FOG = 3,
};
// Lo que llama eDisplayFrame de los efectos de pantalla (eEnvEffectsE.cpp), los mapas de entorno (eEnvMapE.cpp) y el
// sol (SunE.cpp); las firmas son las de sus definiciones.
int eAttachDepthIntoAlpha(eDmaChain *dma_chain);
void RainDrop(eView *view, float x, float y, float amount, float size);
int eAttachFog(eDmaChain *dma_chain, uint32 palette_base_vram_address, FogVolumeType fogType);
int eAttachDepthOfField(eDmaChain *dma_chain, uint32 vram_sample_buffer, uint32 vram_palette_base);
void eAttachPIXELATE(eView *view, eDmaChain *dma_chain, eDmaChain *tex_chain);
void eAttachBlur(eView *view, eDmaChain *dma_chain, eDmaChain *tex_chain, float intensity, float INz, int bloom);
void eAttachBloom4(eView *view, eDmaChain *dma_chain, eDmaChain *tex_chain, float intensity, float INz, int use3D);
int eAttachSmear2(eDmaChain *dma_chain, unsigned int palette_base_vram_address, float multiplier);
void eDisplayEnvRenderTargets(eView *view);
void eCreateEnvMapPrims(eRenderTarget *dest_target);
int32 IsFaceEnabled(int32 i);
void StuffSun(eView *view);
int eRenderOccludedFlares(eDmaChain *dma_chain, eDmaChain *tex_chain);
extern float FocalDistance;
extern float DepthOfField;
// Y lo que llama de otras unidades sin cabecera propia en el arbol: Ecstasy.cpp, eLight.cpp, CarRender.cpp,
// WorldModel.cpp, Scenery.cpp, World.cpp, Gps.cpp, GameFlow.cpp, MissionEdit.cpp, Stomper.cpp, Config.cpp y
// DebugCarRenderTest.cpp (en PS2 DrawMissionComponents es global: la llama eDisplayFrame).
void ePreDisplay();
void ePostDisplay();
void eSwapFrameMallocBuffers();
void eSwapDynamicFrameMemory();
void eDebugRender(eView *view);
void RenderMWDebugTopology(eView *view);
void SetupSceneryCullInfo(eView *view, struct SceneryCullInfo &info, int exclude_flags);
void elBeginFrame();
float indep_fpow(float n, float p);
void UpdateEnvironmentMapCameras();
int IsAnyFrontEndCars();
void SetFeCarRenderFlagsGlob(int draw_solid, int draw_alpha, int draw_shadows);
void RenderFrontEndCars(eView *view, int reflection);
void RenderFEFlares(eView *view, int reflexion);
void RenderWorldModels(eView *view, int exc_flag);
void RenderVisibleZones(eView *view);
void RenderTrackRoutes(eView *view);
void RenderGpsArrows(eView *view);
void RenderTrackMarkers(eView *view);
void DrawMissionComponents();
void SuspendRenderEggs();
void ResumeRenderEggs();
extern int DrawCarsReflections;
extern int DisplayParticleSystemStats;
namespace DebugCarRenderTest {
void Render();
};
#endif

int eGetScreenWidth();
int eGetScreenHeight();
int eGetZWriteState();
int eSetZWriteState(int state);
eRenderTarget *eGetCurrentRenderTarget();
void CreateViewMatricies(eView *view, bMatrix4 *view_to_screen, bMatrix4 *view_clipgb, bMatrix4 *clip_to_screen0,
                         bMatrix4 *clip_to_screen1, bMatrix4 *clip_to_screen2, float force_near_z, float force_far_z,
                         float force_screen_far_z);
// De bWare/Src/bMatrix.cpp.
void fInvertMatrix(bMatrix4 *d, bMatrix4 *s);
void eSetCurrentRenderTarget(eRenderTarget *render_target);
void PatchVUCode();
void ResetDmaStats();
void TestSort();
void UpdateDmaStats();
void DmaTest();
float GetWaitForBufferSwapTime();
float GetVifTime();
void GiganticPSX2Hack(void *dma_ptr, void *tex_ptr);
void _eAddVertex(const float *v);
// La de tres flotantes es la que llaman las otras dos; en PS2 es una funcion aparte.
void eAddVertex(float x, float y, float z);
// Tipos de retorno y nombres de los parametros, de los stabs de la A124 (vide_mode es asi en el original); las usan
// Platform.cpp (eVblCallback, eSetDisplaySystem), SunE.cpp y AcidFXE.cpp.
int eSetDisplaySystem(int vide_mode);
float CalculateH(unsigned short alpha, eView *view);
void InitVRAM();
void eAttachDirectData(TextureInfo *texture_info, float sort_z, void *data, unsigned short qwsize);
void eVblCallback();
void eDmaInit();
void eClearRenderTarget(eRenderTarget *render_target, eDmaChain *dma_chain, int clearR, int clearG, int clearB,
                        int clearA);
void eClearVram(unsigned int colour);
void eFillScreenWithColour(unsigned int colour);
unsigned int *CreateTextureContext(TextureInfo *texture_info, int TCC);
// Etiquetas del microcodigo de la VU1 (el overlay .DVP.overlay..0x0...208.0 del ELF): su direccion es la del programa
// en la memoria de la VU1, y el MSCAL del VIF la quiere en dobles palabras (>> 3).
extern unsigned int vuInitMicroCode[];
extern unsigned int vuDoNothing[];
extern unsigned int vuLoadMatricies00[];
extern unsigned int vuLoadMatricies01[];
extern unsigned int vuSetTextureEnv[];
int GetPS2TextureBufferWidth(int width, int psm_bits);
void DownloadVUCode();
void SetScreenBuffers();
void eInitTextureDownloader();
void AttachDummyTexture(eDmaChain *dma_chain, int eop);
void AttachSolidPolys(eDmaChain *dma_chain, eDmaChain *tex_chain);
void AttachAlphaPolys(eDmaChain *dma_chain, eDmaChain *tex_chain);
// De DmaDump.cpp (va detras de EcstasyE.cpp en zEcstasy).
void DumpDmaChain(int chain_number, void *dma_chain);
// Las usa eVisualTreatment.cpp (plataforma 3); firmas y nombres de parametro de los stabs de la A124.
void eAttachTargetMoveSprites(eDmaChain *dma_chain, eRenderTarget *dest_target, eRenderTarget *src_target, int delta_x,
                              int delta_y, int delta_u, int delta_v, int blend_bits, int alpha, int TCC, int r, int g,
                              int b, int a, int flip, unsigned int test_1);
void eAttachDownsample(eDmaChain *dma_chain, eRenderTarget *dest_target, eRenderTarget *src_target, int TCC);
void VRAMView();
void RenderSillyMask(ePoly *poly, TextureInfo *texture_info);
// Las tiras de un eSolid, con luz o sin ella (eViewPlatInterface::Render(eModel *...)); nombres de los stabs.
int epRenderStripsUnlit(eView *view, eSolid *solid, bMatrix4 *local_world, eLightContext *light_context, uint32 flags,
                        bMatrix4 *blending_matrices);
int epRenderStripsLit(eView *view, eSolid *solid, bMatrix4 *local_world, eLightContext *light_context, uint32 flags,
                      bMatrix4 *blending_matrices);
void ePutDisplayEnvironment();
// Los rangos de z de pantalla de los dos juegos de matrices (CreateViewMatricies); EcstasyE.cpp.
extern float ZRanges[2][6];
void eSwapScreenBuffers();
int Vif1InteruptHandler(int cause);
// De psx2/src/Platform.cpp: eSetDisplaySystem la registra al cambiar de modo de video.
void RegisterPSX2VBlankHandler();

#endif
