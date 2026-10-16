#ifndef __ECSTASY_ENGINE__ECSTASYDATA_HPP
#define __ECSTASY_ENGINE__ECSTASYDATA_HPP

#include "Speed/Indep/bWare/Inc/bList.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

struct TextureInfo;
struct eLightContext;
struct eModel;
struct ePoly;

enum EVIEW_ID {
    EVIEW_NONE = -1,
    EVIEW_FLAILER = 0,
    EVIEW_PLAYER1 = 1,
    EVIEW_PLAYER2 = 2,
    EVIEW_PLAYER1_RVM = 3,
#ifdef EA_PLATFORM_PLAYSTATION2
    // PS2 tiene cuatro vistas mas (stabs de la A124; SLUS pone las sombras en 17 y 18 igual).
    EVIEW_PLAYER1_REFLECTION = 4,
    EVIEW_PLAYER2_REFLECTION = 5,
    EVIEW_PLAYER1_GLOW = 6,
    EVIEW_PLAYER2_GLOW = 7,
    EVIEW_PLAYER1_SPECULAR = 8,
    EVIEW_PLAYER2_SPECULAR = 9,
    EVIEW_ANTI_ALIAS = 10,
    EVIEW_COMBINER = 11,
    EVIEW_QUADRANT_TOP_LEFT = 12,
    EVIEW_QUADRANT_TOP_RIGHT = 13,
    EVIEW_QUADRANT_BOTTOM_LEFT = 14,
    EVIEW_QUADRANT_BOTTOM_RIGHT = 15,
    EVIEW_HDR_SCENE = 16,
    EVIEW_SHADOWMAP1 = 17,
    EVIEW_SHADOWMAP2 = 18,
    EVIEW_SHADOWMATTE = 19,
    EVIEW_ENVMAP0F = 20,
    EVIEW_ENVMAP0R = 21,
    EVIEW_ENVMAP0B = 22,
    EVIEW_ENVMAP0L = 23,
    EVIEW_ENVMAP0U = 24,
    EVIEW_ENVMAP0D = 25,
    EVIEW_FIRST = 0,
    EVIEW_LAST = 25,
    EVIEW_FIRST_RENDER = 1,
    EVIEW_LAST_RENDER = 25,
    EVIEW_FIRST_PLAYER = 1,
    EVIEW_LAST_PLAYER = 3,
    EVIEW_FIRST_RVM = 3,
    EVIEW_LAST_RVM = 3,
    EVIEW_FIRST_ENVMAP = 20,
    EVIEW_LAST_ENVMAP = 25,
    NUM_EVIEWS = 26,
    NUM_RENDER_VIEWS = 25,
#elif defined(EA_PLATFORM_WIN32)
    // PC tiene 24 (eViews 0x9195E0 + 24 x 0x70 = StreamingSolidPackLoader): el orden de PS2 sin ANTI_ALIAS ni COMBINER.
    // Probadas en speed.exe: 4/5 (CarRenderConn::OnRender resta 4 y suma 1), 8/9 (CalculateViewMatricies), 15/16
    // (WhatSectionsShouldWeDraw resta 14), 17 (eRenderLightFlarePool: > 17 y < 24) y 18-23 (eDisplayFrame). Los nombres
    // son los de los stabs de la A124; 6/7, 10-13 y 14 van por el orden de PS2 y GameCube.
    EVIEW_PLAYER1_REFLECTION = 4,
    EVIEW_PLAYER2_REFLECTION = 5,
    EVIEW_PLAYER1_GLOW = 6,
    EVIEW_PLAYER2_GLOW = 7,
    EVIEW_PLAYER1_SPECULAR = 8,
    EVIEW_PLAYER2_SPECULAR = 9,
    EVIEW_QUADRANT_TOP_LEFT = 10,
    EVIEW_QUADRANT_TOP_RIGHT = 11,
    EVIEW_QUADRANT_BOTTOM_LEFT = 12,
    EVIEW_QUADRANT_BOTTOM_RIGHT = 13,
    EVIEW_HDR_SCENE = 14,
    EVIEW_SHADOWMAP1 = 15,
    EVIEW_SHADOWMAP2 = 16,
    EVIEW_SHADOWMATTE = 17,
    EVIEW_ENVMAP0F = 18,
    EVIEW_ENVMAP0R = 19,
    EVIEW_ENVMAP0B = 20,
    EVIEW_ENVMAP0L = 21,
    EVIEW_ENVMAP0U = 22,
    EVIEW_ENVMAP0D = 23,
    EVIEW_FIRST = 0,
    EVIEW_LAST = 23,
    EVIEW_FIRST_RENDER = 1,
    EVIEW_LAST_RENDER = 23,
    EVIEW_FIRST_PLAYER = 1,
    EVIEW_LAST_PLAYER = 3,
    EVIEW_FIRST_RVM = 3,
    EVIEW_LAST_RVM = 3,
    EVIEW_FIRST_ENVMAP = 18,
    EVIEW_LAST_ENVMAP = 23,
    NUM_EVIEWS = 24,
    NUM_RENDER_VIEWS = 23,
#elif defined(EA_PLATFORM_XENON)
    // X360 tiene 24 (eViews mide 0xA80 = 24 x 0x70 y el bucle de eDisplayFrame llega a la 23), con otra disposicion.
    // Probadas en NFS.exe: 4/5 reflejo (CarRenderConn::OnRender resta 4; epInitViews les da las camaras de los jugadores
    // 1/2; culling 0x800), 13/14 sombras (WhatSectionsShouldWeDraw compara 13/14 y resta 12; culling 0x4100) y 18-23
    // entorno (culling 0x1000; eRenderLightFlarePool y OnRender: >= 0x12 && <= 0x17). Los nombres EVIEW_X360_* son
    // NUESTROS: esas vistas no tienen nombre ni uso que lo pruebe en el binario (no hay cadenas).
    EVIEW_PLAYER1_REFLECTION = 4,
    EVIEW_PLAYER2_REFLECTION = 5,
    EVIEW_X360_6 = 6,
    EVIEW_X360_7 = 7,
    EVIEW_X360_8 = 8,
    EVIEW_X360_9 = 9,
    EVIEW_X360_10 = 10,
    EVIEW_X360_11 = 11,
    EVIEW_X360_12 = 12,
    EVIEW_SHADOWMAP1 = 13,
    EVIEW_SHADOWMAP2 = 14,
    EVIEW_X360_15 = 15,
    EVIEW_X360_16 = 16,
    EVIEW_X360_17 = 17,
    EVIEW_ENVMAP0F = 18,
    EVIEW_ENVMAP0R = 19,
    EVIEW_ENVMAP0B = 20,
    EVIEW_ENVMAP0L = 21,
    EVIEW_ENVMAP0U = 22,
    EVIEW_ENVMAP0D = 23,
    EVIEW_FIRST = 0,
    EVIEW_LAST = 23,
    EVIEW_FIRST_RENDER = 1,
    EVIEW_LAST_RENDER = 23,
    EVIEW_FIRST_PLAYER = 1,
    EVIEW_LAST_PLAYER = 3,
    EVIEW_FIRST_RVM = 3,
    EVIEW_LAST_RVM = 3,
    EVIEW_FIRST_ENVMAP = 18,
    EVIEW_LAST_ENVMAP = 23,
    NUM_EVIEWS = 24,
    NUM_RENDER_VIEWS = 23,
#else
    EVIEW_PLAYER1_GLOW = 4,
    EVIEW_PLAYER2_GLOW = 5,
    EVIEW_PLAYER1_SPECULAR = 6,
    EVIEW_PLAYER2_SPECULAR = 7,
    EVIEW_QUADRANT_TOP_LEFT = 8,
    EVIEW_QUADRANT_TOP_RIGHT = 9,
    EVIEW_QUADRANT_BOTTOM_LEFT = 10,
    EVIEW_QUADRANT_BOTTOM_RIGHT = 11,
    EVIEW_HDR_SCENE = 12,
    EVIEW_SHADOWMAP1 = 13,
    EVIEW_SHADOWMAP2 = 14,
    EVIEW_SHADOWMATTE = 15,
    EVIEW_ENVMAP0F = 16,
    EVIEW_ENVMAP0R = 17,
    EVIEW_ENVMAP0B = 18,
    EVIEW_ENVMAP0L = 19,
    EVIEW_ENVMAP0U = 20,
    EVIEW_ENVMAP0D = 21,
    EVIEW_FIRST = 0,
    EVIEW_LAST = 21,
    EVIEW_FIRST_RENDER = 1,
    EVIEW_LAST_RENDER = 21,
    EVIEW_FIRST_PLAYER = 1,
    EVIEW_LAST_PLAYER = 3,
    EVIEW_FIRST_RVM = 3,
    EVIEW_LAST_RVM = 3,
    EVIEW_FIRST_ENVMAP = 16,
    EVIEW_LAST_ENVMAP = 21,
    NUM_EVIEWS = 22,
    NUM_RENDER_VIEWS = 21,
#endif
    NUM_PLAYER_VIEWS = 3,
    NUM_RVM_VIEWS = 1,
};

enum eVisibleState {
    EVISIBLESTATE_NOT = 0,
    EVISIBLESTATE_PARTIAL = 1,
    EVISIBLESTATE_FULL = 2,
};

#ifdef EA_PLATFORM_PLAYSTATION2
// PS2 tiene sus propios destinos (el enum de los stabs de la A124): los de las pasadas del VU, los cuartos de
// resolucion para los efectos y los dos frame buffers y el z buffer al final.
enum TARGET_ID {
    TARGET_FLAYER = 0,
    TARGET_PLAYER1 = 1,
    TARGET_PLAYER2 = 2,
    TARGET_PLAYER1_RVM = 3,
    TARGET_PLAYER1_REFLECTION = 4,
    TARGET_PLAYER2_REFLECTION = 5,
    TARGET_PLAYER1_SPECULAR = 6,
    TARGET_PLAYER2_SPECULAR = 7,
    TARGET_FRAMEBUFFER2 = 8,
    TARGET_QUARTER_32 = 9,
    TARGET_QUARTER_24 = 10,
    TARGET_EIGTH_32 = 11,
    TARGET_SIXTEENTH_32 = 12,
    TARGET_THIRTYSECOND_32 = 13,
    TARGET_ENVRMAP0B = 14,
    TARGET_ENVRMAP0L = 15,
    TARGET_ENVRMAP0F = 16,
    TARGET_ENVRMAP0R = 17,
    TARGET_ENVRMAP0U = 18,
    TARGET_ENVRMAP0D = 19,
    TARGET_ENVMAP0_FULL = 20,
    TARGET_ZBUFFER = 21,
    TARGET_FRAMEBUFFER0 = 22,
    TARGET_FRAMEBUFFER1 = 23,
    TOTAL_RENDER_TARGETS = 24,
    FIRST_RENDER_TARGET = 0,
    LAST_RENDER_TARGET = 20,
    NUM_RENDER_TARGETS = 21,
    NUM_FRAMEBUFFER_TARGETS = 2,
    NUM_HALFHEIGHT_TARGETS = 4,
};
#else
enum TARGET_ID {
    NUM_RENDER_TARGETS = 17,
    LAST_RENDER_TARGET = 16,
    FIRST_RENDER_TARGET = 0,
    TOTAL_RENDER_TARGETS = 17,
    TARGET_ENVMAP0_FULL = 16,
    TARGET_ENVMAP0D = 15,
    TARGET_ENVMAP0U = 14,
    TARGET_ENVMAP0L = 13,
    TARGET_ENVMAP0B = 12,
    TARGET_ENVMAP0R = 11,
    TARGET_ENVMAP0F = 10,
    TARGET_QUADRANT_BOTTOM_RIGHT = 9,
    TARGET_QUADRANT_BOTTOM_LEFT = 8,
    TARGET_QUADRANT_TOP_RIGHT = 7,
    TARGET_QUADRANT_TOP_LEFT = 6,
    TARGET_PLAYER2_SPECULAR = 5,
    TARGET_PLAYER1_SPECULAR = 4,
    TARGET_PLAYER1_RVM = 3,
    TARGET_PLAYER2 = 2,
    TARGET_PLAYER1 = 1,
    TARGET_FLAYER = 0,
};
#endif

enum FILTER_ID {
    FILTER_OFF = 0,
    FILTER_DEFAULT = 1,
    FILTER_EFB_XFB = 2,
    FILTER_CUBE_FACES = 3,
    FILTER_SPHERE_MAP = 4,
    FILTER_REFLECTION = 5,
    FILTER_MOTIONBLUR = 6,
    FILTER_GLOWBLOOM = 7,
    FILTER_CONTRAST_INTENSITY = 8,
    FILTER_EFB_XFB_AA = 9,
    FILTER_PIXELATE = 10,
    FILTER_TOTAL = 11,
};

enum EVIEWMODE {
    EVIEWMODE_NONE = 0,
    EVIEWMODE_ONE = 1,
    EVIEWMODE_ONE_RVM = 2,
    EVIEWMODE_TWOH = 3,
    EVIEWMODE_TWOV = 4,
};

class eTextureEntry {
    // total size: 0x8
  public:
    uint32 NameHash;                  // offset 0x0, size 0x4
    struct TextureInfo *pTextureInfo; // offset 0x4, size 0x4

    void EndianSwap() {
        bPlatEndianSwap(&this->NameHash);
    }
};

class eSolidListHeader : public bTNode<eSolidListHeader> {
    // total size: 0x90
  public:
    int32 Version;                                 // offset 0x8, size 0x4
    int32 NumSolids;                               // offset 0xC, size 0x4
    char Filename[56];                             // offset 0x10, size 0x38
    char GroupName[32];                            // offset 0x48, size 0x20
    uint32 PermChunkByteOffset;                    // offset 0x68, size 0x4
    uint32 PermChunkByteSize;                      // offset 0x6C, size 0x4
    int16 MaxSolidChunkByteAlignment;              // offset 0x70, size 0x2
    int16 EndianSwapped;                           // offset 0x72, size 0x2
    struct eSolidIndexEntry *SolidIndexEntryTable; // offset 0x74, size 0x4
    struct eStreamingEntry *SolidStreamEntryTable; // offset 0x78, size 0x4
    int16 NumTexturePacks;                         // offset 0x7C, size 0x2
    int16 NumDefaultTextures;                      // offset 0x7E, size 0x2
    bPList<struct TexturePack> TexturePackList;    // offset 0x80, size 0x8
    bPList<eTextureEntry> DefaultTextureList;      // offset 0x88, size 0x8

    void EndianSwap() {
        bPlatEndianSwap(&this->Version);
        bPlatEndianSwap(&this->NumSolids);
        bPlatEndianSwap(&this->PermChunkByteOffset);
        bPlatEndianSwap(&this->PermChunkByteSize);
        bPlatEndianSwap(&this->MaxSolidChunkByteAlignment);
    }
};

class eSolidIndexEntry {
    // total size: 0x8
  public:
    uint32 NameHash;      // offset 0x0, size 0x4
    struct eSolid *Solid; // offset 0x4, size 0x4

    void EndianSwap() {
        bPlatEndianSwap(&this->NameHash);
    }
};

class eLightMaterialEntry {
    // total size: 0x8
  public:
    uint32 NameHash;                      // offset 0x0, size 0x4
    struct eLightMaterial *LightMaterial; // offset 0x4, size 0x4

    void EndianSwap() {
        bPlatEndianSwap(&this->NameHash);
    }
};

class eSmoothVertex {
    // total size: 0x8
  public:
    uint32 VertexHash;          // offset 0x0, size 0x4
    uint8 SmoothingGroupNumber; // offset 0x4, size 0x1
    int8 NX;                    // offset 0x5, size 0x1
    int8 NY;                    // offset 0x6, size 0x1
    int8 NZ;                    // offset 0x7, size 0x1

    void EndianSwap() {
        bPlatEndianSwap(&this->VertexHash);
    }
};

class eSmoothVertexPlat {
    // total size: 0xC
  public:
    uint32 VertexHash;     // offset 0x0, size 0x4
    uint32 SmoothingGroup; // offset 0x4, size 0x4
    uint32 VertexOffset;   // offset 0x8, size 0x4

    void EndianSwap() {
        bPlatEndianSwap(&this->VertexHash);
        bPlatEndianSwap(&this->SmoothingGroup);
        bPlatEndianSwap(&this->VertexOffset);
    }
};

class eNormalSmoother {
    // total size: 0xC
  public:
    eSmoothVertex *SmoothVertexTable;         // offset 0x0, size 0x4
    eSmoothVertexPlat *SmoothVertexPlatTable; // offset 0x4, size 0x4
    int16 NumSmoothVertex;                    // offset 0x8, size 0x2
    int16 NumSmoothVertexPlat;                // offset 0xA, size 0x2

    void EndianSwap() {
        bPlatEndianSwap(&this->NumSmoothVertex);
        bPlatEndianSwap(&this->NumSmoothVertexPlat);
    }
};

// total size: 0x2
struct eDamageVertex {
    uint16 Data; // offset 0x0, size 0x2

    void EndianSwap() {
        bPlatEndianSwap(&this->Data);
    }

    uint32 GetDirection();
    uint32 GetDistance();
    uint32 GetCell();
};

// total size: 0x50
class ePositionMarker {
  public:
    uint32 NameHash; // offset 0x0, size 0x4
    int32 iParam0;   // offset 0x4, size 0x4
    float fParam0;   // offset 0x8, size 0x4
    float fParam1;   // offset 0xC, size 0x4
    bMatrix4 Matrix; // offset 0x10, size 0x40

    void EndianSwap() {
        bPlatEndianSwap(&this->NameHash);
        bPlatEndianSwap(&this->iParam0);
        bPlatEndianSwap(&this->fParam0);
        bPlatEndianSwap(&this->fParam1);
        bPlatEndianSwap(&this->Matrix);
    }
};

class eModel;
class ePoly;
class eLightContext;

class eViewPlatInfo;

// total size: 0x4
class eViewPlatInterface {
    eViewPlatInfo *PlatInfo; // offset 0x0, size 0x4

  public:
    eViewPlatInfo *GetPlatInfo() {
        return PlatInfo;
    }
    void SetPlatInfo(eViewPlatInfo *info) {
        PlatInfo = info;
    }

    void FEBeginBatchRender(int numPolys);
    void FEEndBatchRender();
    eVisibleState GetVisibleStateGB(const bVector3 *aabb_min, const bVector3 *aabb_max, bMatrix4 *local_world);
    eVisibleState GetVisibleStateSB(const bVector3 *aabb_min, const bVector3 *aabb_max, bMatrix4 *local_world);
    eVisibleState GetVisibleStateSB(const bVector3 *position, bMatrix4 *local_world);
    void GetScreenPosition(bVector3 *screen_position, const bVector3 *world_position);
    int GetPixelWidth();
    int GetPixelHeight();

    void Render(eModel *model, bMatrix4 *local_to_world, eLightContext *light_context, uint32 flags, bMatrix4 *blending_matricies);
    void FERender(ePoly *poly, TextureInfo *texture_info, bMatrix4 *local_to_world, int use_previous_data, float bbRad);
    void Render(ePoly *poly, TextureInfo *texture_info, bMatrix4 *local_to_world, int use_previous_data, float bbRad);
    void FERender(ePoly *poly, TextureInfo *texture_info, TextureInfo *texture_info_mask, int use_previous_data);
    void FERender(ePoly *poly, TextureInfo *texture_info, int use_previous_data);
#ifdef EA_PLATFORM_WIN32
    // PC: estas dos sobrecargas son MIEMBROS ESTATICOS en el original (los FERender que las llaman destruyen
    // ECX con un argumento y solo empujan tres/cuatro argumentos: sin this; y el `add esp` tras la llamada las
    // declara cdecl, que en un miembro estatico es la convencion por defecto).
    static void Render(ePoly *poly, TextureInfo *texture_info, TextureInfo *texture_info_mask, int use_previous_data);
    static void Render(ePoly *poly, TextureInfo *texture_info, int use_previous_data);
    // La tercera (0x6D1A40): el Render(poly, textura, matriz, ...) miembro y ___eDisplayFrame la llaman igual, sin this.
    static void Render(ePoly *poly, TextureInfo *texture_info, bMatrix4 *local_to_world, int use_previous_data);
#else
    void Render(ePoly *poly, TextureInfo *texture_info, TextureInfo *texture_info_mask, int use_previous_data);
    void Render(ePoly *poly, TextureInfo *texture_info, int use_previous_data);
#endif
    void Render(struct ePointSprite3D *sprite, TextureInfo *texture_info, bMatrix4 *local_world, int num_sprites);

#ifdef EA_PLATFORM_WIN32
    // PC: protegida (el mangling de speed.exe es ?GimmeMyViewPlatInfo@eViewPlatInterface@@KAPAVeViewPlatInfo@@H@Z;
    // KA = static protected). Solo la llama el constructor de eView. Ultima, como en el DWARF de GameCube.
  protected:
#endif
    static eViewPlatInfo *GimmeMyViewPlatInfo(int view_id);
};

class eLoadedSolidStats {
    // total size: 0x14
  public:
    uint32 NumLoadedLists;           // offset 0x0, size 0x4
    uint32 NumLoadedSolids;          // offset 0x4, size 0x4
    uint32 TotalSolidsByteSize;      // offset 0x8, size 0x4
    uint32 TotalNormalSmootherBytes; // offset 0xC, size 0x4
    uint32 TotalDamageBytes;         // offset 0x10, size 0x4
};

#ifdef EA_PLATFORM_PLAYSTATION2
// total size: 0x10 (los stabs de la A124: en PS2 la entrada describe el bloque de quadwords del VU)
struct eStripEntry {
    unsigned short Flags;          // offset 0x0, size 0x2
    unsigned char NumVerts;        // offset 0x2, size 0x1
    unsigned char NumPolys;        // offset 0x3, size 0x1
    float MaterialDensity;         // offset 0x4, size 0x4
    unsigned short DataQWordOffset; // offset 0x8, size 0x2
    signed char Nz;                // offset 0xA, size 0x1
    signed char Nt;                // offset 0xB, size 0x1
    unsigned char DataQWordSize;   // offset 0xC, size 0x1
    unsigned char PolyGroupNumber; // offset 0xD, size 0x1
    signed char TextureNumber;     // offset 0xE, size 0x1
    signed char LightMaterialIndex; // offset 0xF, size 0x1
};
#else
// total size: 0x10
struct eStripEntry {
    unsigned int DataOffset;            // offset 0x0, size 0x4
    unsigned short DataSize;            // offset 0x4, size 0x2
    unsigned short Flags;               // offset 0x6, size 0x2
    unsigned char NumVerts;             // offset 0x8, size 0x1
    unsigned char PolyGroupNumber;      // offset 0x9, size 0x1
    char TextureNumber;                 // offset 0xA, size 0x1
    char LightMaterialIndex;            // offset 0xB, size 0x1
    unsigned char VertexDescription;    // offset 0xC, size 0x1
    unsigned char VertexFormat;         // offset 0xD, size 0x1
    unsigned short DataDisplayListSize; // offset 0xE, size 0x2
};
#endif

#ifdef EA_PLATFORM_WIN32
// PC: eSolidPlatInfo con su tabla de mallas (BCHUNK_SPEED_ESOLID_PC_PLATINFO, 0x134900).
#include "Speed/PC/Src/Ecstasy/eSolidPlat.hpp"
#else
class eSolidPlatInfo {
    // total size: 0x24
  public:
    uint16 Version;    // offset 0x0, size 0x2
    uint16 StripFlags; // offset 0x2, size 0x2
    uint16 NumStrips;  // offset 0x4, size 0x2
#ifndef EA_BUILD_A124
    uint16 NumIdxClrTable; // offset 0x6, size 0x2
#else
    uint16 pad0;
#endif
    uint32 SizeofStripData; // offset 0x8, size 0x4
#ifndef EA_PLATFORM_PLAYSTATION2
    uint32 DataOffset0; // offset 0xC, size 0x4
    uint32 DataOffset1; // offset 0x10, size 0x4
    uint32 DataOffset2; // offset 0x14, size 0x4
    uint32 DataOffset3; // offset 0x18, size 0x4
#endif
    struct eStripEntry *StripEntryTable; // offset 0x1C, size 0x4
    uint8 *StripDataStart;               // offset 0x20, size 0x4

    void FixStripEntryTable(struct eSolid *solid, uint8 *strip_entry_data, uint8 *previous_strip_entry_data);
#ifdef EA_PLATFORM_PLAYSTATION2
    void CalcAutoK(struct eSolid *solid);
#endif
};
#endif

#endif
