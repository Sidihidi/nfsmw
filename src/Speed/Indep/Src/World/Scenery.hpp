#ifndef WORLD_SCENERY_H
#define WORLD_SCENERY_H

#include "Speed/Indep/bWare/Inc/bWare.hpp"
#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Libs/Support/Utility/UCrc.h"
#include "Speed/Indep/Src/World/WeatherMan.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

enum SceneryDetailLevel {
    SCENERY_DETAIL_NONE = -1,
    SCENERY_DETAIL_HIGH = 0,
    SCENERY_DETAIL_MEDIUM = 1,
    SCENERY_DETAIL_LOW = 2,
    SCENERY_DETAIL_REFLECTION = 3,
    NUM_SCENERY_DETAIL_LEVELS = 4,
};

// gcdatos2: simetrico a GetTrackPathZoneName (DWARF-1: global_subroutine inline, char*,
// un int); la definicion va en Scenery.cpp para no internar High/Medium/Low/Reflection
// en cada TU que parsea esta cabecera.
// gcdatos3 (medido y REVERTIDO): con la definicion en esta cabecera las cadenas entran en
// 20 TU de GC (el objetivo las tiene en 14: sobran zAI/zEAXSound/zFe/zFe2/zFeOverlay/
// zPlatform/zRender, falta zLua) y desplazadas (en zAnim caen en +0x2E8 contra obj+0x880):
// nuestro grafo parsea Scenery.hpp demasiado pronto (WorldModel.hpp via Skids) y en TU
// de mas. Codigo/fn x5 identicos y matched_data GC identico, pero zRender .rodata bajo
// 83,2649->81,9192. La forma fiel es la definicion aqui, pero solo tras restaurar el
// grafo (receta en docs/analisis/2026-10-18_gcdatos3.md).
char *GetSceneryDetailName(int detail_level);

// total size: 0x18
struct SceneryBoundingBox {
#ifdef EA_BUILD_A124
    short BBoxMin[3]; // offset 0x0, size 0x6
    short BBoxMax[3]; // offset 0x6, size 0x6
#else
    float BBoxMin[3]; // offset 0x0, size 0xC
    float BBoxMax[3]; // offset 0xC, size 0xC
#endif

    void GetBBox(bVector3 *bbox_min, bVector3 *bbox_max) {
        bFill(bbox_min, BBoxMin[0], BBoxMin[1], BBoxMin[2]);
        bFill(bbox_max, BBoxMax[0], BBoxMax[1], BBoxMax[2]);
    }
};

// total size: 0x40
class SceneryInstance : public SceneryBoundingBox {
  public:
    void GetRotation(bMatrix4 *matrix) {
        const float rotation_conversion = 1.0f / 8192.0f;
        float x = static_cast<float>(Rotation[0]) * rotation_conversion;
        float y = static_cast<float>(Rotation[1]) * rotation_conversion;
        float z = static_cast<float>(Rotation[2]) * rotation_conversion;
        matrix->v0.x = x;
        matrix->v0.y = y;
        matrix->v0.z = z;
        matrix->v0.w = 0.0f;

        x = static_cast<float>(Rotation[3]) * rotation_conversion;
        y = static_cast<float>(Rotation[4]) * rotation_conversion;
        z = static_cast<float>(Rotation[5]) * rotation_conversion;
        matrix->v1.x = x;
        matrix->v1.y = y;
        matrix->v1.z = z;
        matrix->v1.w = 0.0f;

        x = static_cast<float>(Rotation[6]) * rotation_conversion;
        y = static_cast<float>(Rotation[7]) * rotation_conversion;
        z = static_cast<float>(Rotation[8]) * rotation_conversion;
        matrix->v2.x = x;
        matrix->v2.y = y;
        matrix->v2.z = z;
        matrix->v2.w = 0.0f;
    }

    void GetPosition(bVector4 *pos) {
        *pos = *reinterpret_cast<bVector4 *>(Position);
        pos->w = 1.0f;
    }

    bVector3 *GetPosition() {
        return reinterpret_cast<bVector3 *>(Position);
    }

    void GetMatrix(bMatrix4 *matrix) {
        GetRotation(matrix);
        GetPosition(&matrix->v3);
        matrix->v3.w = 1.0f;
    }

    void SetMatrix(const bMatrix4 *matrix) {
        const float rotation_conversion = 8192.0f;
        Rotation[0] = static_cast<short>(bFloatToInt(matrix->v0.x * rotation_conversion));
        Rotation[1] = static_cast<short>(bFloatToInt(matrix->v0.y * rotation_conversion));
        Rotation[2] = static_cast<short>(bFloatToInt(matrix->v0.z * rotation_conversion));
        Rotation[3] = static_cast<short>(bFloatToInt(matrix->v1.x * rotation_conversion));
        Rotation[4] = static_cast<short>(bFloatToInt(matrix->v1.y * rotation_conversion));
        Rotation[5] = static_cast<short>(bFloatToInt(matrix->v1.z * rotation_conversion));
        Rotation[6] = static_cast<short>(bFloatToInt(matrix->v2.x * rotation_conversion));
        Rotation[7] = static_cast<short>(bFloatToInt(matrix->v2.y * rotation_conversion));
        Rotation[8] = static_cast<short>(bFloatToInt(matrix->v2.z * rotation_conversion));
        Position[0] = matrix->v3.x;
        Position[1] = matrix->v3.y;
        Position[2] = matrix->v3.z;
    }

    uint32 ExcludeFlags; // offset 0x18, size 0x4
#ifdef EA_BUILD_A124
    int32 PrecullerInfoIndex;
    int16 LightingContextNumber;
    int16 Pad1;
    int Pad[2];
#else
    int16 PrecullerInfoIndex;    // offset 0x1C, size 0x2
    int16 LightingContextNumber; // offset 0x1E, size 0x2
#endif
    float Position[3];       // offset 0x20, size 0xC
    short Rotation[9];       // offset 0x2C, size 0x12
    int16 SceneryInfoNumber; // offset 0x3E, size 0x2
};

// total size: 0xC
struct SceneryDrawInfo {
    eModel *pModel;               // offset 0x0, size 0x4
    bMatrix4 *pMatrix;            // offset 0x4, size 0x4
    SceneryInstance *SceneryInst; // offset 0x8, size 0x4
};

// gcdatos11: typedef y tipos de culling/log ausentes del arbol, reconstruidos del
// DWARF GC (symbols/mw_dwarfdump.nothpp:2198484 SceneryLogInfo 0x184 tras el
// typedef SceneryCallback, :2201993 CullingStats 0x8) confirmados campo a campo
// en stabs PS2 (PS2_types.nothpp; SceneryLogInfo es 0x190 alli por el alineado
// a 16 de bVector3, que los tipos bVector3/bMatrix4 ya reproducen sin relleno
// manual). Solo declaraciones: sin cuerpos ni variables no emiten nada.
typedef void (*SceneryCallback)(SceneryDrawInfo *);

// total size: 0x184
struct SceneryLogInfo {
    char MeshName[32];              // offset 0x0, size 0x20
    char InstanceMeshName[32];      // offset 0x20, size 0x20
    char BBGFilename[48];           // offset 0x40, size 0x30
    char SmokeableTypeName[32];     // offset 0x70, size 0x20
    char SmokeableCollisionName[32]; // offset 0x90, size 0x20
    unsigned int NameHash[4];       // offset 0xB0, size 0x10
    bVector3 Position;              // offset 0xC0, size 0x10
    bMatrix4 Matrix;                // offset 0xD0, size 0x40
    bVector3 BBoxMin;               // offset 0x110, size 0x10
    bVector3 BBoxMax;               // offset 0x120, size 0x10
    int SectionNumber;              // offset 0x130, size 0x4
    int SceneryInfoNumber;          // offset 0x134, size 0x4
    int SceneryInstanceNumber;      // offset 0x138, size 0x4
    int OverrideInfoNumber;         // offset 0x13C, size 0x4
    int ExcludeFlags;               // offset 0x140, size 0x4
    float Radius;                   // offset 0x144, size 0x4
    int SmokeableFlag;              // offset 0x148, size 0x4
    unsigned int SmokeableHash;     // offset 0x14C, size 0x4
    int FlattenIfDrivenOver;        // offset 0x150, size 0x4
    int IncludedAsTopology;         // offset 0x154, size 0x4
    int IsTrafficLight;             // offset 0x158, size 0x4
    char SoundEventType[32];        // offset 0x15C, size 0x20
    int NumPolysWorld;              // offset 0x17C, size 0x4
    int NumPolysMemory;             // offset 0x180, size 0x4
};

// total size: 0x8
struct CullingStats {
    int TotalNumObjects; // offset 0x0, size 0x4
    int TotalNumPolys;   // offset 0x4, size 0x4
};

// total size: 0xBC
struct SceneryCullInfo {
    bMatrix4 FacadeMatrix;             // offset 0x0, size 0x40
    bMatrix4 ClipMatrix;               // offset 0x40, size 0x40
    struct eView *pView;               // offset 0x80, size 0x4
    int ExcludeFlags;                  // offset 0x84, size 0x4
    SceneryDrawInfo *pFirstDrawInfo;   // offset 0x88, size 0x4
    SceneryDrawInfo *pCurrentDrawInfo; // offset 0x8C, size 0x4
    SceneryDrawInfo *pTopDrawInfo;     // offset 0x90, size 0x4
    bVector3 Position;                 // offset 0x94, size 0x10
    bVector3 Direction;                // offset 0xA4, size 0x10
    float H;                           // offset 0xB4, size 0x4
    int PrecullerSectionNumber;        // offset 0xB8, size 0x4
};

// total size: 0x8
struct ModelHeirarchy {
    enum Flags {
        F_INTERNAL = 1,
    };
    // total size: 0x10
    struct Node {
        UCrc32 mNodeName;   // offset 0x0, size 0x4
        uint32 mModelHash;  // offset 0x4, size 0x4
        eModel *mModel;     // offset 0x8, size 0x4
        uint8 mFlags;       // offset 0xC, size 0x1
        uint8 mParent;      // offset 0xD, size 0x1
        uint8 mNumChildren; // offset 0xE, size 0x1
        uint8 mChildIndex;  // offset 0xF, size 0x1
    };

    const Node *GetNodes() const {
        return reinterpret_cast<const Node *>(&this[1]);
    }

    Node *GetNodes() {
        return reinterpret_cast<Node *>(&this[1]);
    }

    // unsigned int GetSize() const {}

    uint32 mNameHash; // offset 0x0, size 0x4
    uint8 mNumNodes;  // offset 0x4, size 0x1
    uint8 mFlags;     // offset 0x5, size 0x1
    uint16 pad;       // offset 0x6, size 0x2
};

// total size: 0x48
struct SceneryInfo {
    char DebugName[24];         // offset 0x0, size 0x18
    uint32 NameHash[4];         // offset 0x18, size 0x10
    eModel *pModel[4];          // offset 0x28, size 0x10
    float Radius;               // offset 0x38, size 0x4
    uint32 MeshChecksum;        // offset 0x3C, size 0x4
    uint32 mHeirarchyNameHash;  // offset 0x40, size 0x4
    ModelHeirarchy *mHeirarchy; // offset 0x44, size 0x4
};

class tPrecullerInfo {
  public:
    int IsNotVisible(int preculler_section_number) {
        return (VisibilityBits[preculler_section_number >> 3] & (1 << (preculler_section_number & 7)));
    }

    int IsVisible(int preculler_section_number) {
        return static_cast<int>(IsNotVisible(preculler_section_number) == 0);
    }

    uint8 *GetBits() {
        return VisibilityBits;
    }

    void EndianSwap() {}

  private:
    uint8 VisibilityBits[0x80];
};

inline int GetPrecullerSectionNumber(float x, float y) {
    int nx = (bFloatToInt(x) >> 5) & 0x1F;
    int ny = bFloatToInt(y) & 0x3E0;
    return ny + nx;
}

// total size: 0x24
class SceneryTreeNode : public SceneryBoundingBox {
  public:
    int16 NumChildren;   // offset 0x18, size 0x2
    int16 ChildCodes[5]; // offset 0x1A, size 0xA
};

// TODO
class ScenerySectionHeader : public bTNode<ScenerySectionHeader> {
  public:
    void DrawAScenery(int scenery_instance_number, SceneryCullInfo *scenery_cull_info, int visibility_state);

    int IsVisible(SceneryCullInfo *scenery_cull_info);

    void CullBruteForce(SceneryCullInfo *scenery_cull_info);

    void TreeCull(SceneryCullInfo *scenery_cull_info);

    void CullNodeRecursive(SceneryTreeNode *node, SceneryCullInfo *scenery_cull_info, uint32 visibility_state);

    SceneryInstance *GetSceneryInstance(int scenery_instance_number) {
        return &pSceneryInstance[scenery_instance_number];
    }

    SceneryInfo *GetSceneryInfo(int scenery_info_number) {
        return &pSceneryInfo[scenery_info_number];
    }

    int GetSectionNumber() {
        return this->SectionNumber;
    }

    tPrecullerInfo *GetPrecullerInfo(int preculler_info_index) {
        return &PrecullerInfoTable[preculler_info_index];
    }

    static float mLodLevelDistance[3]; // size: 0xC, address: 0xFFFFFFFF

    int ChunksLoaded;                      // offset 0x8, size 0x4
    int SectionNumber;                     // offset 0xC, size 0x4
    int NumPolygonsInMemory;               // offset 0x10, size 0x4
    int NumPolygonsInWorld;                // offset 0x14, size 0x4
    SceneryInfo *pSceneryInfo;             // offset 0x18, size 0x4
    int NumSceneryInfo;                    // offset 0x1C, size 0x4
    SceneryInstance *pSceneryInstance;     // offset 0x20, size 0x4
    int NumSceneryInstances;               // offset 0x24, size 0x4
    SceneryTreeNode *SceneryTreeNodeTable; // offset 0x28, size 0x4
    int NumSceneryTreeNodes;               // offset 0x2C, size 0x4
    tPrecullerInfo *PrecullerInfoTable;    // offset 0x30, size 0x4
    int NumPrecullerInfos;                 // offset 0x34, size 0x4
    int32 ViewsVisibleThisFrame;           // offset 0x38, size 0x4
};

// total size: 0x6
struct SceneryOverrideInfo {
    void EndianSwap() {
#ifndef EA_BUILD_A124
        bPlatEndianSwap(&SectionNumber);
        bPlatEndianSwap(&InstanceNumber);
        bPlatEndianSwap(&ExcludeFlags);
#endif
    }

    static int GetHashIndex(short section_number) {
        return section_number;
    }

    int GetHashIndex() {
        return GetHashIndex(SectionNumber);
    }

    void SetExcludeFlags(unsigned short exclude_flag_mask, unsigned short exclude_flag_override) {
        ExcludeFlags = (ExcludeFlags & exclude_flag_mask) | exclude_flag_override;
        AssignOverrides();
    }

    void EnableRendering() {
        SetExcludeFlags(0xFFEF, 0);
    }

    void DisableRendering() {
#ifdef EA_PLATFORM_PLAYSTATION2
        // Las dos PS2 materializan 0xFFEF (`ori a1,zero,0xffef`), no 0xFFFF: el resultado es el mismo
        // ((x & 0xFFEF) | 0x10 == (x & 0xFFFF) | 0x10). Fuera de PS2 se queda como estaba (PC al 100 %).
        SetExcludeFlags(0xFFEF, 0x10);
#else
        SetExcludeFlags(0xFFFF, 0x10);
#endif
    }

    void AssignOverrides();
    void AssignOverrides(ScenerySectionHeader *section_header);

    int16 SectionNumber;  // offset 0x0, size 0x2
    int16 InstanceNumber; // offset 0x2, size 0x2
    uint16 ExcludeFlags;  // offset 0x4, size 0x2
};

extern signed char SceneryGroupEnabledTable[0x1000];

SceneryOverrideInfo *GetSceneryOverrideInfo(int override_info_number);

// total size: 0x4
struct SceneryOverrideInfoHookup {
    void EndianSwap() {
#ifndef EA_BUILD_A124
        bPlatEndianSwap(&OverrideInfoNumber);
        bPlatEndianSwap(&InstanceNumber);
#endif
    }

    uint16 OverrideInfoNumber; // offset 0x0, size 0x2
    uint16 InstanceNumber;     // offset 0x2, size 0x2
};

// total size: 0x8014
struct SceneryGroup : public bTNode<SceneryGroup> {
    // SceneryGroup(unsigned int name_hash) {}

    int GetMemoryImageSize() {
        int size = (NumObjects * static_cast<int>(sizeof(*OverrideInfoNumbers)) + static_cast<int>(offsetof(SceneryGroup, OverrideInfoNumbers)) + 3) & ~3;
        return size;
    }

    int GetNumObjects() {
        return NumObjects;
    }

    // int GetOverrideInfoNumber(int index) {}

    SceneryOverrideInfo *GetOverrideInfo(int index) {
        return GetSceneryOverrideInfo(OverrideInfoNumbers[index]);
    }

    void EndianSwap() {
#ifndef EA_BUILD_A124
        bPlatEndianSwap(&NameHash);
        bPlatEndianSwap(&GroupNumber);
        bPlatEndianSwap(&NumObjects);
#endif
        for (int n = 0; n < NumObjects; n++) {
#ifndef EA_BUILD_A124
            bPlatEndianSwap(&OverrideInfoNumbers[n]);
#endif
        }
    }

    void EnableRendering(bool flip_artwork) {
        int exclude_flag_mask = 0xFBEF;
        int exclude_flag_override = 0;
        if (flip_artwork) {
            exclude_flag_override = 0x400;
        }

        for (int n = 0; n < GetNumObjects(); n++) {
            GetOverrideInfo(n)->SetExcludeFlags(exclude_flag_mask, exclude_flag_override);
        }

        SceneryGroupEnabledTable[this->GroupNumber] = 1;
        if (flip_artwork) {
            SceneryGroupEnabledTable[this->GroupNumber] |= 2;
        }
        if (this->DriveThroughBarrierFlag) {
            SceneryGroupEnabledTable[this->GroupNumber] |= 4;
        }
    }

    void DisableRendering() {
        for (int n = 0; n < GetNumObjects(); n++) {
            // Las dos PS2 dejan el bit 0x10 fuera de la mascara; GameCube la pone entera.
#ifdef EA_PLATFORM_PLAYSTATION2
            GetOverrideInfo(n)->SetExcludeFlags(0xFFEF, 0x10);
#else
            GetOverrideInfo(n)->SetExcludeFlags(0xFFFF, 0x10);
#endif
        }
    }

    uint32 NameHash;                   // offset 0x8, size 0x4
    int16 GroupNumber;                 // offset 0xC, size 0x2
    int16 NumObjects;                  // offset 0xE, size 0x2
    int8 BarrierFlag;                  // offset 0x10, size 0x1
    int8 DriveThroughBarrierFlag;      // offset 0x11, size 0x1
    int16 Pad2;                        // offset 0x12, size 0x2
    uint16 OverrideInfoNumbers[16384]; // offset 0x14, size 0x8000
};

// total size: 0x8E0
class GrandSceneryCullInfo {
  public:
    // El ctor va ANTES de Init(): los cuerpos en clase se compilan en orden de
    // declaracion, asi que aqui Init() todavia no tiene cuerpo y GCC 2.9 emite
    // una llamada real mas la copia fuera de linea. Misma forma que DefragFixer.
    GrandSceneryCullInfo() {
        Init();
    }

    void Init() {
        NumCullInfos = 0;
    }

    SceneryCullInfo *AddCullInfo() {
        return &SceneryCullInfos[NumCullInfos++];
    }

    bool IsFrozen() {
        return false;
    }

    int WhatSectionsShouldWeDraw(int16 *sections_to_draw, int max_sections_to_draw, SceneryCullInfo *scenery_cull_info);
    void CullView(SceneryCullInfo *scenery_cull_info);
    void DoCulling();
    void StuffScenery(eView *view, int stuff_flags);
    void CallbackScenery(eView *view, void (*callback)(SceneryDrawInfo *));

    SceneryCullInfo SceneryCullInfos[12]; // offset 0x0, size 0x8D0
    int NumCullInfos;                     // offset 0x8D0, size 0x4
    SceneryDrawInfo *pFirstDrawInfo;      // offset 0x8D4, size 0x4
    SceneryDrawInfo *pCurrentDrawInfo;    // offset 0x8D8, size 0x4
    SceneryDrawInfo *pTopDrawInfo;        // offset 0x8DC, size 0x4

    static SceneryDrawInfo SceneryDrawInfoTable[3500];
};

// TODO making this extern messes with the order
// extern RegionQuery RegionInfo;
extern SceneryDetailLevel ForceAllSceneryDetailLevels;
extern bTList<SceneryGroup> SceneryGroupList;
enum ePrecullerMode {
    PRECULLER_MODE_OFF = 0,
    PRECULLER_MODE_ON = 1,
    PRECULLER_MODE_BLINKING = 2,
    PRECULLER_MODE_HIGH_SPEED = 3,
};

extern ePrecullerMode PrecullerMode;
extern int DisablePrecullerCounter;

void InitVisibleZones();
void CloseVisibleZones();
void ServicePreculler();
void LoadPrecullerBooBooScripts();
void SetPrecullerBooBoo();
void ClrPrecullerBooBoo();
void EnableSceneryGroup(unsigned int group_name_hash, bool flip_artwork);
void DisableSceneryGroup(unsigned int name_hash);
SceneryGroup *FindSceneryGroup(unsigned int name_hash);
SceneryInstance *FindSceneryInstance(unsigned int name_hash);
SceneryInfo *FindSceneryInfo(unsigned int name_hash);

inline int IsSceneryGroupEnabled(int group_number) {
    return SceneryGroupEnabledTable[group_number];
}

inline int InlinedViewGetPixelSize(SceneryCullInfo *scenery_cull_info, const bVector3 *position, float radius) {
    bVector3 dir = *position - scenery_cull_info->Position;
    float distance_ahead = bDot(&dir, &scenery_cull_info->Direction);
    if (distance_ahead < -radius) {
        return 0;
    }

    float distance_away = bLength(&dir) - radius;
    float pixel_size = scenery_cull_info->H;
    if (distance_away > radius) {
        pixel_size = (radius * pixel_size) / distance_away;
    }
    return bFloatToInt(pixel_size);
}

inline void SetPrecullerMode(ePrecullerMode mode) {
    PrecullerMode = mode;
}

inline void EnablePreculler() {
    DisablePrecullerCounter--;
}

inline void DisablePreculler() {
    DisablePrecullerCounter++;
}

void DisableSceneryGroup(unsigned int group_name_hash);

#endif
