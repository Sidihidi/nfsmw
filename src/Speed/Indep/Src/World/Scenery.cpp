#include "Scenery.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Tools/Inc/ConversionUtil.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "VisibleSection.hpp"
#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Ecstasy/eLight.hpp"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Misc/Profiler.hpp"
#include "Speed/Indep/Src/Misc/ResourceLoader.hpp"
#include "Speed/Indep/Src/Misc/SpeedChunks.hpp"
#include "Speed/Indep/Src/World/TrackInfo.hpp"
#include "Speed/Indep/Src/World/TrackStreamer.hpp"
#include "Speed/Indep/Src/World/Track.hpp"
#include "Speed/Indep/Src/World/Rain.hpp"
#include "Speed/Indep/bWare/Inc/SpeedScript.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/bWare/Inc/bFunk.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/bFile.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#endif

// El ELF carga aqui 1/2.237f (0x3EE4E0C2), no el 0.44703001f de
// ConversionUtil.hpp: el volcado DWARF nombra un segundo inline MPH2MPS(float x)
// -sin const- que divide entre 2.237f. Local a esta TU para no tocar el resto
// del juego, que si usa 0.44703001f.
static inline float SceneryMPH2MPS(float mph) {
    return mph / 2.237f;
}

// gcdatos2: inline en el original (DWARF), sin copia fuera de linea; los literales del
// objetivo van en orden del enum (High/Medium/Low/Reflection).
inline char *GetSceneryDetailName(int detail_level) {
    switch (detail_level) {
        case SCENERY_DETAIL_HIGH:
            return "High";
        case SCENERY_DETAIL_MEDIUM:
            return "Medium";
        case SCENERY_DETAIL_LOW:
            return "Low";
        case SCENERY_DETAIL_REFLECTION:
            return "Reflection";
        default:
            return "High";
    }
}

int LoaderSceneryGroup(bChunk *chunk);
int UnloaderSceneryGroup(bChunk *chunk);
int LoaderScenery(bChunk *chunk);
int UnloaderScenery(bChunk *chunk);
ScenerySectionHeader *GetScenerySectionHeader(int section_number);

// total size: 0x10
class HashToHeirarchyMap : public UTL::Std::map<unsigned int, ModelHeirarchy *, _type_map> {};

class PrecullerBooBooManager {
  public:
    void Reset() {
        bMemSet(this, 0, 0x800);
    }

    void Set(bVector3 &pos) {
        int n = GetSectionNumber(pos);
        unsigned char *p = GetByte(n);
        *p |= GetBit(n);
    }

    void Clr(bVector3 &pos) {
        int n = GetSectionNumber(pos);
        unsigned char *p = GetByte(n);
        *p &= ~0 - GetBit(n);
    }

    bool IsSet(bVector3 &pos) {
        int n = GetSectionNumber(pos);
        unsigned char *p = GetByte(n);
        return (*p & GetBit(n)) != 0;
    }

    int GetSectionNumber(bVector3 &pos) {
        int nx = (bFloatToInt(pos.x) >> 5) & 0x7F;
        int ny = (bFloatToInt(pos.y) >> 5) & 0x7F;
        return (ny << 7) + nx;
    }

  private:
    unsigned char *GetByte(int section_number) {
        return &BitField[section_number >> 3];
    }

    unsigned char GetBit(int section_number) {
        return static_cast<unsigned char>(1 << (section_number & 7));
    }

    uint8 BitField[0x800];
};

// TODO move these?
// r61-world: el objetivo lo emite en 0x80436000, DELANTE del hueco
// gap_06_80436004_data y de EnvMapShadowExtraHeight.
SceneryDetailLevel ForceAllSceneryDetailLevels = SCENERY_DETAIL_NONE;
int DropTopSceneryLOD = 0;
float EnvMapShadowExtraHeight = 2.0f;
int CurrentZoneNumber = (int)0xFFFFFFFF;

void (*ModelConnectionCallback)(ScenerySectionHeader *, int, eModel *) = nullptr;
void (*ModelDisconnectionCallback)(ScenerySectionHeader *, int, eModel *) = nullptr;
void (*SectionConnectionCallback)(ScenerySectionHeader *) = nullptr;
void (*SectionDisconnectionCallback)(ScenerySectionHeader *) = nullptr;

bTList<ScenerySectionHeader> ScenerySectionHeaderList;
HashToHeirarchyMap HeirarchyMap;

int SeeulatorToolActive = 0;
int ScenerySectionToBlink = 0;
int ShowSectionBoarder = 0;
int SeeulatorRefreshTrackStreamer = 0;
#ifdef EA_PLATFORM_PLAYSTATION2
// gcdatos13: definido entero solo en las dos PS2, entre SeeulatorRefreshTrackStreamer
// y LightTable (A124 0x630738, PS2_globals.hpp:5603; SLUS en el mismo hueco), valores
// de su .data objetivo. En GC queda solo la palabra cadaver 99999.0f de 0x80436030
// (gcdatos9); la declaracion esta en Scenery.hpp:296.
float ScenerySectionHeader::mLodLevelDistance[3] = {80.0f, 160.0f, 99999.0f};
#endif
eLight *LightTable = nullptr;
int MaxSceneryLightContexts = 0;
eSceneryLightContext **SceneryLightContextTable = nullptr;
SceneryOverrideInfo *SceneryOverrideInfoTable = nullptr;
int NumSceneryOverrideInfos = 0;

bTList<SceneryGroup> SceneryGroupList;
signed char SceneryGroupEnabledTable[4096] = {1};

bChunkLoader bChunkLoaderSceneryGroup(BCHUNK_SCENERY_BARRIER_GROUPS, LoaderSceneryGroup, UnloaderSceneryGroup);
bChunkLoader bChunkLoaderScenerySection(BCHUNK_SCENERY_SECTION, LoaderScenery, UnloaderScenery);
bChunkLoader bChunkLoaderOverrideInfos(BCHUNK_SCENERY_OVERRIDE_INFOS, LoaderScenery, UnloaderScenery);
bChunkLoader bChunkLoaderSceneryHeirarchy(BCHUNK_MODEL_HIERARCHY_TREE, LoaderScenery, UnloaderScenery);
bChunkLoader bChunkLoaderSceneryLighting(BCHUNK_SCENERY_LIGHT_CONTEXTS, LoaderScenery, UnloaderScenery);

static const float EnablePrecullingSpeed = SceneryMPH2MPS(40.0f);
eModel *pVisibleZoneBoundaryModel = nullptr;
ePrecullerMode PrecullerMode = PRECULLER_MODE_ON;
int DisablePrecullerCounter = 0;
// gcdatos10: int muerto sin direccion en el DWARF (globals: int MostDrawInfo,
// address 0xFFFFFFFF); en la A124 en 0x6317C8, entre el estatico bPrintName
// de DrawAScenery (0x6317C4) y pDebugModel (0x6317CC), con valor 0 en su
// .data. En GC es la segunda palabra del gap_06_80437058_data (8 B) entre
// DisablePrecullerCounter (0x80437054) y pDebugModel (0x80437060).
int MostDrawInfo = 0;
eModel *pDebugModel = nullptr;
int RunPrecullingTool = 0; // el LoadGlobalChunks de PS2 agranda el pool del streamer si esta puesta

RegionQuery RegionInfo;
#ifdef EA_BUILD_A124
// gcdatos11: SceneryCullingStats (CullingStats, 0x8) sobrevive solo en el
// prototipo: A124 la tiene en .bss 0x642C80 (symbols/PS2/PS2_globals.hpp,
// entre RegionInfo y SceneryOverrideHashTable); en GC su DIE va sin direccion
// (0xFFFFFFFF, symbols/mw_dwarfdump.nothpp:2202000, entre el const
// LodUseDistance y LightTable) = eliminada en retail. Solo-A124, como los
// demas datos de prototipo de esta cabecera.
CullingStats SceneryCullingStats;
#endif
short SceneryOverrideHashTable[257];
PrecullerBooBooManager gPrecullerBooBooManager;
SceneryDrawInfo GrandSceneryCullInfo::SceneryDrawInfoTable[3500];

void BuildSceneryOverrideHashTable() {
    int current_offset = 0;
    for (int hash = 0; hash < 256; hash++) {
        SceneryOverrideHashTable[hash] = static_cast<short>(current_offset);
        while (current_offset < NumSceneryOverrideInfos && (SceneryOverrideInfoTable[current_offset].GetHashIndex() & 0xFF) == hash) {
            current_offset++;
        }
    }
    SceneryOverrideHashTable[256] = static_cast<short>(current_offset);
}

ModelHeirarchy *FindSceneryHeirarchyByName(unsigned int name_hash) {
    HashToHeirarchyMap::iterator iter = HeirarchyMap.find(name_hash);
    if (iter != HeirarchyMap.end()) {
        return (*iter).second;
    }
    return nullptr;
}

SceneryOverrideInfo *GetSceneryOverrideInfo(int override_info_number) {
    return &SceneryOverrideInfoTable[override_info_number];
}

// gcdatos6: sin llamantes en ninguna version (DIE de GC .debug+0x40B71DE con low_pc=-1,
// antes de FindSceneryHeirarchy; la A124 la trae igual, 144 B de codigo PS2 eliminado,
// lineas 43-46 de su Scenery.cpp). Su DIE fija locales hash_index/info/last_info e
// inline SceneryOverrideInfo::GetHashIndex(section_number), sin llamadas (info y
// last_info en registros de paso): recorre el cubo de SceneryOverrideHashTable que
// BuildSceneryOverrideHashTable ordena por (GetHashIndex & 0xFF). La comparacion
// exacta no esta en la evidencia y la funcion no emite en ningun retail.
SceneryOverrideInfo *FindSceneryOverrideInfo(short section_number, short instance_number) {
    int hash_index = SceneryOverrideInfo::GetHashIndex(section_number) & 0xFF;
    SceneryOverrideInfo *info = &SceneryOverrideInfoTable[SceneryOverrideHashTable[hash_index]];
    SceneryOverrideInfo *last_info = &SceneryOverrideInfoTable[SceneryOverrideHashTable[hash_index + 1]];
    for (; info < last_info; info++) {
        if (info->SectionNumber == section_number && info->InstanceNumber == instance_number) {
            return info;
        }
    }
    return nullptr;
}

void SceneryOverrideInfo::AssignOverrides(ScenerySectionHeader *section_header) {
    SceneryInstance *scenery_instance = section_header->GetSceneryInstance(InstanceNumber);

    if ((scenery_instance->ExcludeFlags & 0x800000) != 0 && ((scenery_instance->ExcludeFlags ^ ExcludeFlags) & 0x400) != 0) {
        bMatrix4 matrix;

        scenery_instance->GetRotation(&matrix);
        bFill(&matrix.v3, 0.0f, 0.0f, 0.0f, 1.0f);
        // TODO what did they actually do with that bDegToAng?
        if (bDegToAng(1.0f) > 0.0f) {
            bMatrix4 flip_matrix;
            bIdentity(&flip_matrix);
            flip_matrix.v0.x = -1.0f;
            bMulMatrix(&matrix, &matrix, &flip_matrix);
        }
        scenery_instance->GetPosition(&matrix.v3);
        scenery_instance->SetMatrix(&matrix);
    }

    scenery_instance->ExcludeFlags = ExcludeFlags + (scenery_instance->ExcludeFlags & 0xFFFF0000);
}

void SceneryOverrideInfo::AssignOverrides() {
    ScenerySectionHeader *section_header = GetScenerySectionHeader(SectionNumber);
    if (section_header != nullptr) {
        AssignOverrides(section_header);
    }
}

void LoadPrecullerBooBooScript(const char *filename, bool reset) {
    if (reset) {
        gPrecullerBooBooManager.Reset();
    }

    SpeedScript script(filename, 1);
    while (script.GetNextCommand("BOOBOO:") != nullptr) {
        char *region;
        if (bStrICmp(script.GetNextArgumentString(), TrackInfo::GetLoadedTrackInfo()->RegionName) == 0) {
            char *section = script.GetNextArgumentString();
            char *option = script.GetNextArgumentString();
            bool set_booboo = bStrICmp(option, "SET") == 0;
            bool clr_booboo = bStrICmp(option, "CLR") == 0;
            script.GetNextArgumentString();
            bVector3 pos = script.GetNextArgumentVector3();
            if (set_booboo) {
                gPrecullerBooBooManager.Set(pos);
            } else if (clr_booboo) {
                gPrecullerBooBooManager.Clr(pos);
            }
        }
    }
}

// gcdatos2: el DWARF-1 de la CU zTrack de GC trae la variable (global_variable char*,
// sin direccion asignada) y su cadena esta en la .rodata objetivo de GC y de SLUS,
// aunque Modify/Set/Clr solo existen en la A124: la definicion se parseaba tambien
// fuera de la A124 (en X360/PC la cadena no esta en el objetivo).
// gcdatos10 (MEDIDO y REVERTIDO): su posicion probada en .data es el hueco
// lbl_80436048 entre NumSceneryOverrideInfos y SceneryGroupEnabledTable
// (GC: puntero 0xA68 a la cadena en .rodata+0xA68; A124: 0x630758 entre
// 0x630754 y 0x630768), pero subir la definicion a ese bloque interno la
// cadena delante del pool de conversion que en el objetivo va delante de
// ella: GC .rodata 87,8572->87,6488 (falta 536->543) sin ganar .data
// (97,8409 igual; el puntero apunta a nuestra cadena desplazada). Solo gano
// la A124 (.data 99,2708->99,2757). No reintentar sin cerrar antes el
// desplazamiento del bloque High/Medium/Low/Reflection.
#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_GAMECUBE) || defined(EA_PLATFORM_PLAYSTATION2)
const char *PrecullerNetworkBooBooFilename = "..\\..\\World\\Global\\Scripts\\PrecullerBooBooScript.hoo";
#endif

#ifdef EA_BUILD_A124

// La herramienta de la A124 que anade a mano una linea BOOBOO al guion desde la posicion de la camara.
void ModifyPrecullerBooBooScript(const char *filename, bool set) {
    DoScreenPrintf = 1;
    if (!TrackInfo::GetLoadedTrackInfo()) {
        ScreenPrintf(-250, 100, 3.0f, "Need to be in game!", filename);
        return;
    }
    if (!bFileExists(filename)) {
        ScreenPrintf(-250, 100, 3.0f, "Cannot open %s", filename);
        return;
    }

    char computer_name[80];

    memset(computer_name, 0, sizeof(computer_name));
    bGetComputerName(computer_name, sizeof(computer_name));
    Camera *camera = eViews[EVIEW_PLAYER1].GetCamera();
    const char *option;

    if (set) {
        option = "SET";
        gPrecullerBooBooManager.Set(*camera->GetPosition());
    } else {
        option = "CLR";
        gPrecullerBooBooManager.Clr(*camera->GetPosition());
    }

    char text[512];

    bSPrintf(text, "BOOBOO:  %s %-3s  %s  Pos =%8.2f,%8.2f,%7.2f   Angle=0x%04x   User=%s\n",
             TrackInfo::GetLoadedTrackInfo()->RegionName,
             GetScenerySectionName(TheVisibleSectionManager.GetDrivableSectionNumber(
                 reinterpret_cast<bVector2 *>(camera->GetPosition()))),
             option,
             camera->GetPosition()->x, camera->GetPosition()->y, camera->GetPosition()->z,
             bATan(camera->GetDirection()->x, camera->GetDirection()->y), computer_name);
    bAppendToFile(filename, text, bStrLen(text));
    ScreenPrintf(-250, 100, 3.0f, "Updated %s", filename);
}

void SetPrecullerBooBoo() {
    ModifyPrecullerBooBooScript(PrecullerNetworkBooBooFilename, true);
}

void ClrPrecullerBooBoo() {
    ModifyPrecullerBooBooScript(PrecullerNetworkBooBooFilename, false);
}
#endif

void LoadPrecullerBooBooScripts() {
    LoadPrecullerBooBooScript("TRACKS\\PrecullerBooBooScript.hoo", true);
}

int LoaderSceneryGroup(bChunk *chunk) {
    if (chunk->GetID() == BCHUNK_SCENERY_BARRIER_GROUPS) {
        char *table = chunk->GetData();
        int size = chunk->GetSize();
        int pos = 0;

        while (pos < size) {
            SceneryGroup *group = reinterpret_cast<SceneryGroup *>(&table[pos]);
            SceneryGroupList.AddTail(group);

            group->EndianSwap();

            pos += group->GetMemoryImageSize();
        }
        return 1;
    }

    return 0;
}

int UnloaderSceneryGroup(bChunk *chunk) {
    if (chunk->GetID() == BCHUNK_SCENERY_BARRIER_GROUPS) {
        bMemSet(SceneryGroupEnabledTable, 0, sizeof(SceneryGroupEnabledTable));
        SceneryGroupEnabledTable[0] = 1;
        SceneryGroupList.InitList();
        return 1;
    }

    return 0;
}

SceneryGroup *FindSceneryGroup(unsigned int name_hash) {
    for (SceneryGroup *group = SceneryGroupList.GetHead(); group != SceneryGroupList.EndOfList(); group = group->GetNext()) {
        if (group->NameHash == name_hash) {
            return group;
        }
    }
    return nullptr;
}

// gcdatos6: sin llamantes en ninguna version (DIE de GC .debug+0x40BDA6B con low_pc=-1
// y cuerpo vacio en el volcado; la A124 la trae igual, 8 B de codigo PS2 eliminado:
// retorno pelado). Cuerpo vacio tal cual.
void ServiceSceneryGroupTweakables() {}

void EnableSceneryGroup(unsigned int group_name_hash, bool flip_artwork) {
    SceneryGroup *group = static_cast<SceneryGroup *>(FindSceneryGroup(group_name_hash));
    if (group != nullptr) {
        group->EnableRendering(flip_artwork);
    }
}

void DisableSceneryGroup(unsigned int group_name_hash) {
    SceneryGroup *group = FindSceneryGroup(group_name_hash);
    if (group != nullptr) {
        group->DisableRendering();
        SceneryGroupEnabledTable[group->GroupNumber] = 0;
    }
}

void DisableAllSceneryGroups() {
    for (SceneryGroup *group = SceneryGroupList.GetHead(); group != SceneryGroupList.EndOfList(); group = group->GetNext()) {
        if (IsSceneryGroupEnabled(group->GroupNumber)) {
            group->DisableRendering();
            SceneryGroupEnabledTable[group->GroupNumber] = 0;
        }
    }
}

ScenerySectionHeader *GetScenerySectionHeader(int section_number) {
    VisibleSectionUserInfo *user_info = TheVisibleSectionManager.GetUserInfo(section_number);
    if (user_info != nullptr) {
        return user_info->pScenerySectionHeader;
    }
    return nullptr;
}

int LoaderScenery(bChunk *chunk) {
    if (chunk->GetID() == BCHUNK_SCENERY_OVERRIDE_INFOS) {
        SceneryOverrideInfoTable = reinterpret_cast<SceneryOverrideInfo *>(chunk->GetData());
        NumSceneryOverrideInfos = chunk->GetSize() / sizeof(SceneryOverrideInfo);
        for (int n = 0; n < NumSceneryOverrideInfos; n++) {
            SceneryOverrideInfo *override_info = &SceneryOverrideInfoTable[n];
            override_info->EndianSwap();
        }
        BuildSceneryOverrideHashTable();
        return 1;
    }

    if (chunk->GetID() == BCHUNK_SCENERY_SECTION) {
        ScenerySectionHeader *section_header = nullptr;
        bChunk *first_chunk = chunk->GetFirstChunk();
        bChunk *last_chunk = chunk->GetLastChunk();

        for (bChunk *chunk = first_chunk; chunk != last_chunk; chunk = chunk->GetNext()) {
            if (chunk->GetID() == BCHUNK_SCENERY_SECTION_HEADER) {
                section_header = reinterpret_cast<ScenerySectionHeader *>(chunk->GetAlignedData(16));
                if (!section_header->ChunksLoaded) {
#ifndef EA_BUILD_A124
                    bPlatEndianSwap(&section_header->SectionNumber);
                    bPlatEndianSwap(&section_header->NumPolygonsInMemory);
                    bPlatEndianSwap(&section_header->NumPolygonsInWorld);
                    bPlatEndianSwap(&section_header->ViewsVisibleThisFrame);
#endif
                }

                VisibleSectionUserInfo *user_info = TheVisibleSectionManager.AllocateUserInfo(section_header->SectionNumber);
                user_info->pScenerySectionHeader = section_header;

                if (GetScenerySectionLetter(section_header->SectionNumber) == 'Z') {
                    ScenerySectionHeaderList.AddHead(section_header);
                } else {
                    ScenerySectionHeaderList.AddTail(section_header);
                }
            } else if (chunk->GetID() == BCHUNK_SCENERY_INFOS) {
                section_header->pSceneryInfo = reinterpret_cast<SceneryInfo *>(chunk->GetData());
                section_header->NumSceneryInfo = chunk->GetSize() / sizeof(SceneryInfo);
                if (!section_header->ChunksLoaded) {
                    for (int i = 0; i < section_header->NumSceneryInfo; i++) {
#ifndef EA_BUILD_A124
                        bPlatEndianSwap(&section_header->pSceneryInfo[i].mHeirarchyNameHash);
#endif
                        for (int j = 0; j < 4; j++) {
#ifndef EA_BUILD_A124
                            bPlatEndianSwap(&section_header->pSceneryInfo[i].NameHash[j]);
#endif
                        }
#ifndef EA_BUILD_A124
                        bPlatEndianSwap(&section_header->pSceneryInfo[i].Radius);
                        bPlatEndianSwap(&section_header->pSceneryInfo[i].MeshChecksum);
#endif
                    }
                }

                for (int i = 0; i < section_header->NumSceneryInfo; i++) {
                    SceneryInfo *scenery_info = &section_header->pSceneryInfo[i];
                    if (scenery_info->mHeirarchyNameHash != 0) {
                        scenery_info->mHeirarchy = FindSceneryHeirarchyByName(scenery_info->mHeirarchyNameHash);
                    }
                }
            } else if (chunk->GetID() == BCHUNK_SCENERY_INSTANCES) {
                section_header->pSceneryInstance = reinterpret_cast<SceneryInstance *>(chunk->GetAlignedData(16));
                section_header->NumSceneryInstances = chunk->GetAlignedSize(16) / sizeof(SceneryInstance);
                if (!section_header->ChunksLoaded) {
                    for (int i = 0; i < section_header->NumSceneryInstances; i++) {
                        SceneryInstance *scenery_instance = &section_header->pSceneryInstance[i];
#ifndef EA_BUILD_A124
                        bPlatEndianSwap(&scenery_instance->SceneryInfoNumber);
                        bPlatEndianSwap(&scenery_instance->ExcludeFlags);
#endif
                        int j;
                        for (j = 0; j < 3; j++) {
#ifndef EA_BUILD_A124
                            bPlatEndianSwap(&scenery_instance->Position[j]);
#endif
                        }
                        for (j = 0; j < 9; j++) {
#ifndef EA_BUILD_A124
                            bPlatEndianSwap(&scenery_instance->Rotation[j]);
#endif
                        }
#ifndef EA_BUILD_A124
                        bPlatEndianSwap(&scenery_instance->BBoxMin[0]);
                        bPlatEndianSwap(&scenery_instance->BBoxMin[1]);
                        bPlatEndianSwap(&scenery_instance->BBoxMin[2]);
                        bPlatEndianSwap(&scenery_instance->BBoxMax[0]);
                        bPlatEndianSwap(&scenery_instance->BBoxMax[1]);
                        bPlatEndianSwap(&scenery_instance->BBoxMax[2]);
                        bPlatEndianSwap(&scenery_instance->PrecullerInfoIndex);
                        bPlatEndianSwap(&scenery_instance->LightingContextNumber);
#endif
                    }
                }
            } else if (chunk->GetID() == BCHUNK_SCENERY_TREE_NODES) {
                section_header->SceneryTreeNodeTable = reinterpret_cast<SceneryTreeNode *>(chunk->GetData());
                section_header->NumSceneryTreeNodes = chunk->GetSize() / sizeof(SceneryTreeNode);
                if (!section_header->ChunksLoaded) {
                    for (int i = 0; i < section_header->NumSceneryTreeNodes; i++) {
#ifndef EA_BUILD_A124
                        bPlatEndianSwap(&section_header->SceneryTreeNodeTable[i].NumChildren);
                        bPlatEndianSwap(&section_header->SceneryTreeNodeTable[i].BBoxMin[0]);
                        bPlatEndianSwap(&section_header->SceneryTreeNodeTable[i].BBoxMin[1]);
                        bPlatEndianSwap(&section_header->SceneryTreeNodeTable[i].BBoxMin[2]);
                        bPlatEndianSwap(&section_header->SceneryTreeNodeTable[i].BBoxMax[0]);
                        bPlatEndianSwap(&section_header->SceneryTreeNodeTable[i].BBoxMax[1]);
                        bPlatEndianSwap(&section_header->SceneryTreeNodeTable[i].BBoxMax[2]);
#endif

                        for (int j = 0; j < 5; j++) {
#ifndef EA_BUILD_A124
                            bPlatEndianSwap(&section_header->SceneryTreeNodeTable[i].ChildCodes[j]);
#endif
                        }
                    }
                }
            } else if (chunk->GetID() == BCHUNK_SCENERY_OVERRIDE_HOOKS) {
                SceneryOverrideInfoHookup *hookup_table = reinterpret_cast<SceneryOverrideInfoHookup *>(chunk->GetData());
                int num_hookups = chunk->GetSize() / sizeof(SceneryOverrideInfoHookup);
                for (int n = 0; n < num_hookups; n++) {
                    SceneryOverrideInfoHookup *hookup = &hookup_table[n];
                    hookup->EndianSwap();

                    SceneryOverrideInfo *override_info = &SceneryOverrideInfoTable[hookup->OverrideInfoNumber];
                    if (override_info->InstanceNumber == hookup->InstanceNumber &&
                        override_info->SectionNumber == section_header->GetSectionNumber()) {
                        override_info->AssignOverrides(section_header);
                    }
                }
            } else if (chunk->GetID() == BCHUNK_PRECULLER_INFOS) {
                section_header->PrecullerInfoTable = reinterpret_cast<tPrecullerInfo *>(chunk->GetData());
                section_header->NumPrecullerInfos = chunk->GetSize() / sizeof(tPrecullerInfo);
                if (!section_header->ChunksLoaded) {
                    for (int n = 0; n < section_header->NumPrecullerInfos; n++) {
                        tPrecullerInfo *preculler_info = section_header->GetPrecullerInfo(n);
                        preculler_info->EndianSwap();
                    }
                }
            }
        }

        if (!AreChunksBeingMoved()) {
            for (int n = 0; n < section_header->NumSceneryInfo; n++) {
                SceneryInfo *scenery_info = &section_header->pSceneryInfo[n];
                for (int detail_level = 0; detail_level < 4; detail_level++) {
                    unsigned int name_hash = scenery_info->NameHash[detail_level];
                    // TODO magic
                    if (name_hash != 0 && name_hash != 0xBE43EDBB && name_hash != 0x90F70174) {
                        eModel *model = nullptr;
                        for (int i = 0; i < detail_level; i++) {
                            if ((scenery_info->pModel[i] != nullptr) && scenery_info->pModel[i]->GetNameHash() == name_hash) {
                                model = scenery_info->pModel[i];
                                break;
                            }
                        }

                        if (model == nullptr) {
                            model = new eModel(scenery_info->NameHash[detail_level]);
                            if (ModelConnectionCallback != nullptr) {
                                ModelConnectionCallback(section_header, n, model);
                            }
                        }
                        scenery_info->pModel[detail_level] = model;
                    }
                }

                if ((scenery_info->pModel[2] == nullptr) && (scenery_info->pModel[1] != nullptr)) {
                    scenery_info->pModel[2] = scenery_info->pModel[1];
                }
                if ((scenery_info->pModel[0] == nullptr) && (scenery_info->pModel[1] != nullptr)) {
                    scenery_info->pModel[0] = scenery_info->pModel[1];
                }
            }

            if (SectionConnectionCallback != nullptr) {
                SectionConnectionCallback(section_header);
            }
        }

        section_header->ChunksLoaded = 1;
        return 1;
    }

    if (chunk->GetID() == BCHUNK_MODEL_HIERARCHY_TREE) {
        bChunk *last_chunk = chunk->GetLastChunk();
        for (chunk = chunk->GetFirstChunk(); chunk < last_chunk; chunk = chunk->GetNext()) {
            ModelHeirarchy *mH = reinterpret_cast<ModelHeirarchy *>(chunk->GetData());
#ifndef EA_BUILD_A124
            bPlatEndianSwap(&mH->mNameHash);
            bPlatEndianSwap(&mH->mNumNodes);
            bPlatEndianSwap(&mH->mFlags);
#endif

            ModelHeirarchy::Node *node = mH->GetNodes();
            for (unsigned int i = 0; i < mH->mNumNodes; i++) {
                bPlatEndianSwap(&node[i].mNodeName);
                bPlatEndianSwap(&node[i].mFlags);
                bPlatEndianSwap(&node[i].mModelHash);
                bPlatEndianSwap(&node[i].mParent);
                bPlatEndianSwap(&node[i].mNumChildren);
                bPlatEndianSwap(&node[i].mChildIndex);
            }

            HeirarchyMap[mH->mNameHash] = mH;
            for (unsigned int i = 0; i < mH->mNumNodes; i++) {
                node[i].mModel = node[i].mModelHash != 0 ? new eModel(node[i].mModelHash) : nullptr;
            }
        }
        return 1;
    }

    // TODO magic
    if (chunk->GetID() == BCHUNK_SCENERY_LIGHT_CONTEXTS) {
        int numSceneryLightContexts;
        bChunk *last_chunk = chunk->GetLastChunk();
        for (chunk = chunk->GetFirstChunk(); chunk < last_chunk; chunk = chunk->GetNext()) {
            if (chunk->GetID() == 0x34116) {
                LightTable = reinterpret_cast<eLight *>(chunk->GetData());
            } else if (chunk->GetID() == 0x34117) {
                SceneryLightContextTable = reinterpret_cast<eSceneryLightContext **>(chunk->GetData());
                MaxSceneryLightContexts = chunk->GetSize() / static_cast<int>(sizeof(eSceneryLightContext *));
            } else if (chunk->GetID() == 0x34118) {
                eSceneryLightContext *sceneryLightContext = reinterpret_cast<eSceneryLightContext *>(chunk->GetAlignedData(16));
                sceneryLightContext->LocalLights = reinterpret_cast<bMatrix4 *>(&sceneryLightContext[1]);
                sceneryLightContext->EndianSwap();
                SceneryLightContextTable[sceneryLightContext->LightingContextNumber] = sceneryLightContext;
            }
        }
        return 1;
    }

    return 0;
}

int UnloaderScenery(bChunk *chunk) {
    if (chunk->GetID() == BCHUNK_SCENERY_OVERRIDE_INFOS) {
        SceneryOverrideInfoTable = nullptr;
        NumSceneryOverrideInfos = 0;
        BuildSceneryOverrideHashTable();
        return 1;
    }

    if (chunk->GetID() == BCHUNK_SCENERY_SECTION) {
        ScenerySectionHeader *section_header = reinterpret_cast<ScenerySectionHeader *>(chunk->GetFirstChunk()->GetAlignedData(16));
        VisibleSectionUserInfo *user_info = TheVisibleSectionManager.GetUserInfo(section_header->SectionNumber);
        user_info->pScenerySectionHeader = nullptr;
        TheVisibleSectionManager.UnallocateUserInfo(section_header->SectionNumber);
        ScenerySectionHeaderList.Remove(section_header);

        if (!AreChunksBeingMoved()) {
            for (int n = 0; n < section_header->NumSceneryInfo; n++) {
                SceneryInfo *scenery_info = &section_header->pSceneryInfo[n];
                for (int detail_level = 0; detail_level < 4; detail_level++) {
                    if (AreChunksBeingMoved()) {
                        break;
                    }

                    if (scenery_info->pModel[detail_level] != nullptr) {
                        for (int i = detail_level + 1; i < 4; i++) {
                            if (scenery_info->pModel[i] == scenery_info->pModel[detail_level]) {
                                scenery_info->pModel[i] = nullptr;
                            }
                        }
                        if (ModelDisconnectionCallback != nullptr) {
                            ModelDisconnectionCallback(section_header, n, scenery_info->pModel[detail_level]);
                        }

                        delete scenery_info->pModel[detail_level];
                        scenery_info->pModel[detail_level] = nullptr;
                    }
                }
            }

            if (SectionDisconnectionCallback != nullptr) {
                SectionDisconnectionCallback(section_header);
            }
        }

        return 1;
    }

    if (chunk->GetID() == BCHUNK_MODEL_HIERARCHY_TREE) {
        bChunk *last_chunk = chunk->GetLastChunk();
        for (chunk = chunk->GetFirstChunk(); chunk < last_chunk; chunk = chunk->GetNext()) {
            ModelHeirarchy *mH = reinterpret_cast<ModelHeirarchy *>(chunk->GetData());
            ModelHeirarchy::Node *node = mH->GetNodes();

            for (unsigned int i = 0; i < mH->mNumNodes; i++) {
                if (node[i].mModel != nullptr) {
                    delete node[i].mModel;
                    node[i].mModel = nullptr;
                }
            }

            HashToHeirarchyMap::iterator iter = HeirarchyMap.find(mH->mNameHash);
            HeirarchyMap.erase(iter);
        }
        return 1;
    }

    if (chunk->GetID() == BCHUNK_SCENERY_LIGHT_CONTEXTS) {
        // El orden de estas tres asignaciones decide el reparto de r9/r10/r11
        // del planificador: con (Max, Light, Scen) los `stw` salen rotados y
        // cada `lis` queda contra el simbolo equivocado. Medido contra el .s
        // del objetivo: (Light, Scen, Max) reproduce lis r11/r10/r9 =
        // LightTable/SceneryLightContextTable/MaxSceneryLightContexts.
        LightTable = nullptr;
        SceneryLightContextTable = nullptr;
        MaxSceneryLightContexts = 0;
        return 1;
    }

    return 0;
}

SceneryInfo *FindSceneryInfo(unsigned int name_hash) {
    for (ScenerySectionHeader *section_header = ScenerySectionHeaderList.GetHead(); section_header != ScenerySectionHeaderList.EndOfList();
         section_header = section_header->GetNext()) {
        for (int scenery_number = 0; scenery_number < section_header->NumSceneryInfo; scenery_number++) {
            SceneryInfo *scenery_info = &section_header->pSceneryInfo[scenery_number];
            if ((scenery_info->pModel[0] != nullptr) && scenery_info->pModel[0]->GetNameHash() == name_hash) {
                return scenery_info;
            }
        }
    }
    return nullptr;
}

// gcdatos6: sin llamantes en ninguna version (DIE de GC .debug+0x40C2E64 con low_pc=-1,
// antes de FindSceneryInstance; la A124 la trae igual, 76 B de codigo PS2 eliminado,
// lineas 271-273 de su Scenery.cpp). Su DIE fija un unico local lod_num en un bloque y
// 0 inlines (scenery_info y lod_num en registros de paso: sin llamadas), y su literal
// "Unknown" esta en el pool de zTrack en obj+0xAD8. El esqueleto recorre los 4 LOD
// buscando el primero con modelo y devuelve DebugName (el unico char* alcanzable sin
// llamadas); la prueba exacta (pModel o NameHash) no esta en la evidencia y la funcion
// no emite en ningun retail.
char *GetSceneryInfoName(SceneryInfo *scenery_info) {
    for (int lod_num = 0; lod_num < 4; lod_num++) {
        if (scenery_info->pModel[lod_num] != nullptr) {
            return scenery_info->DebugName;
        }
    }
    return "Unknown";
}

SceneryInstance *FindSceneryInstance(unsigned int name_hash) {
    for (ScenerySectionHeader *section_header = ScenerySectionHeaderList.GetHead(); section_header != ScenerySectionHeaderList.EndOfList();
         section_header = section_header->GetNext()) {
        int scenery_instance_number;
        for (scenery_instance_number = 0; scenery_instance_number < section_header->NumSceneryInstances; scenery_instance_number++) {
            SceneryInstance *scenery_instance = &section_header->pSceneryInstance[scenery_instance_number];
            SceneryInfo *scenery_info = section_header->GetSceneryInfo(scenery_instance->SceneryInfoNumber);

            if ((scenery_info->pModel[0] != nullptr) && scenery_info->pModel[0]->GetNameHash() == name_hash) {
                return scenery_instance;
            }
        }
    }
    return nullptr;
}

// UNSOLVED
void RenderVisibleSectionBoundary(VisibleSectionBoundary *boundary, eView *view) {
    float perimeter;
    for (int n = 0; n < boundary->GetNumPoints(); n++) {
        perimeter = bDistBetween(boundary->GetPoint(n), boundary->GetPoint((n + 1) % boundary->GetNumPoints()));
    }

    bVector3 position;
    position.z = 0.0f;
    TopologyCoordinate topology_coordinate;
    float pos = static_cast<float>((bFloatToInt(WorldTimer.GetSeconds() * 262144.0f) & 0xffff)) * 6.103515625e-05f;

    int point_number;
    for (point_number = 0; point_number < boundary->GetNumPoints(); point_number++) {
        bVector2 normal = *boundary->GetPoint((point_number + 1) % boundary->GetNumPoints()) - *boundary->GetPoint(point_number);
        float length = bLength(&normal);

        bNormalize(&normal, &normal);
        while (pos < length) {
            bScaleAdd(reinterpret_cast<bVector2 *>(&position), boundary->GetPoint(point_number), &normal, pos);

            if (topology_coordinate.HasTopology(reinterpret_cast<bVector2 *>(&position))) {
                position.z = 9999.0f;
                position.z = topology_coordinate.GetElevation(&position, nullptr, nullptr, nullptr);
                int pixel_size = view->GetPixelSize(&position, 1.0f);
                if (pixel_size > 0) {
                    bMatrix4 *matrix = eFrameMallocMatrix(1);
                    if (matrix != nullptr) {
                        bIdentity(matrix);
                        bCopy(&matrix->v3, &position, 1.0f);
                        view->Render(pVisibleZoneBoundaryModel, matrix, nullptr, 0, nullptr);
                    }
                }
            }

            pos += 4.0f;
        }

        pos -= length;
    }
}

void RenderVisibleZones(eView *view) {
    if (ShowSectionBoarder && (pVisibleZoneBoundaryModel != nullptr)) {
        DrivableScenerySection *scenery_section =
            TheVisibleSectionManager.FindDrivableSection(reinterpret_cast<const bVector2 *>(view->GetCamera()->GetPosition()));
        if (scenery_section != nullptr) {
            RenderVisibleSectionBoundary(scenery_section->GetBoundary(), view);
        }
    }
}

void InitVisibleZones() {
    if (pVisibleZoneBoundaryModel == nullptr) {
        pVisibleZoneBoundaryModel = new eModel(bStringHash("MARKER_BOUNDARY"));
    }
}

void CloseVisibleZones() {
    delete pVisibleZoneBoundaryModel;
    pVisibleZoneBoundaryModel = nullptr;
    if (SeeulatorToolActive) {
        unsigned int address = 0;
        bFunkCallASync("Seeulator", 4, &address, 4);
        bFunkCallASync("Seeulator", 5, &address, 4);
        bFunkCallASync("Seeulator", 6, &address, 4);
    }
}

// gcdatos6: sin llamantes en ninguna version (DIE de GC .debug+0x40C454E con low_pc=-1
// y cuerpo vacio en el volcado, antes de IsInTable; la A124 la trae igual, 8 B de
// codigo PS2 eliminado). Cuerpo vacio tal cual.
void HandleJoystickBlinkSection(int param) {}

int IsInTable(short *table, int num_entries, int entry) {
    for (int i = 0; i < num_entries; i++) {
        if (table[i] == entry) {
            return i;
        }
    }
    return -1;
}

int ToggleIsInTable(short *table, int num_entries, int max_entries, int entry) {
    int index = IsInTable(table, num_entries, entry);

    if (index < 0) {
        table[num_entries % max_entries] = static_cast<short>(entry);
        num_entries++;
    } else {
        table[index] = -1;
    }

    return num_entries;
}

void ScenerySectionHeader::DrawAScenery(int scenery_instance_number, SceneryCullInfo *scenery_cull_info, int visibility_state) {
    SceneryInstance *scenery_instance = &pSceneryInstance[scenery_instance_number];
    tPrecullerInfo *preculler_info = GetPrecullerInfo(scenery_instance->PrecullerInfoIndex);
    if (scenery_cull_info->PrecullerSectionNumber >= 0) {
        if (!preculler_info->IsVisible(scenery_cull_info->PrecullerSectionNumber)) {
            return;
        }
    }

    SceneryInfo *my_scenery_info = &pSceneryInfo[scenery_instance->SceneryInfoNumber];
    // gcdatos10: estatico local del DIE de GC (bool bPrintName @ 0x80437058,
    // entre my_scenery_info y exclude_bits; en la A124 static bool en
    // 0x6317C4, entre DisablePrecullerCounter y MostDrawInfo). Sin uso en el
    // codigo retail (DrawAScenery casa al 100 % en GC sin tocarlo): solo
    // ocupa su palabra en .data, a cero en GC y en la A124.
    static bool bPrintName = false;
    int exclude_bits = scenery_instance->ExcludeFlags & 0xFF;
    int exclude_mask = 0x60;
    int exclude_flagxor = exclude_bits ^ exclude_mask;
    if (exclude_flagxor & scenery_cull_info->ExcludeFlags) {
        return;
    }
    SceneryInfo *scenery_info = &pSceneryInfo[scenery_instance->SceneryInfoNumber];

    if (visibility_state == EVISIBLESTATE_PARTIAL) {
        bVector3 bbox_min;
        bVector3 bbox_max;
        scenery_instance->GetBBox(&bbox_min, &bbox_max);
        visibility_state = scenery_cull_info->pView->GetVisibleState(&bbox_min, &bbox_max, nullptr);
        if (visibility_state == EVISIBLESTATE_NOT) {
            return;
        }
    }

    bVector3 *position = scenery_instance->GetPosition();
    int pixel_size = InlinedViewGetPixelSize(scenery_cull_info, position, scenery_info->Radius + 6.0f);

    if (pixel_size < 2) {
        return;
    }
    if (scenery_instance->ExcludeFlags & 0x2000000) {
        pixel_size += 10;
    }

    eModel *model = nullptr;
    if (scenery_cull_info->ExcludeFlags & 0x1800) {
        int far_clip_size = 0x20;
        if (scenery_instance->ExcludeFlags & 0x80) {
            if (pixel_size >= far_clip_size) {
                model = scenery_info->pModel[2];
            }
        } else {
            if (pixel_size >= far_clip_size) {
                if (scenery_instance->ExcludeFlags & 0x1000100) {
                    model = scenery_info->pModel[0];
                } else {
                    model = scenery_info->pModel[3];
                }
            }
        }
    } else if (scenery_cull_info->ExcludeFlags & 0x20) {
        int far_clip_size = 0x20;
        if (pixel_size >= far_clip_size) {
            model = scenery_info->pModel[2];
        }
    } else if (eGetCurrentViewMode() >= EVIEWMODE_TWOH) {
        if (pixel_size > 0x16) {
            model = scenery_info->pModel[2];
        }
    } else if (pixel_size > 0x11) {
        float density = 8.7f;

        if (scenery_info->pModel[0] != nullptr) {
            eSolid *solid = (scenery_info->pModel[0] != nullptr) ? scenery_info->pModel[0]->GetSolid() : nullptr;
            if ((solid != nullptr) && solid->NumPolys >= 40) {
                float ratio = solid->Density;
                if (ratio < 6.0f) {
                    ratio = 6.0f;
                }
                density = static_cast<float>(pixel_size) / ratio;
            }
        }
        if (density >= 8.7f) {
            model = scenery_info->pModel[0];
        } else {
            model = scenery_info->pModel[2];
        }
    }

    if (model == nullptr) {
        return;
    }

    if (scenery_instance->ExcludeFlags & 0x200) {
        SceneryDrawInfo *draw_info = scenery_cull_info->pCurrentDrawInfo;
        if (draw_info >= scenery_cull_info->pTopDrawInfo) {
            return;
        }

        scenery_cull_info->pCurrentDrawInfo = draw_info + 1;
        draw_info->pModel = reinterpret_cast<eModel *>(reinterpret_cast<uintptr_t>(model) + visibility_state);
        draw_info->pMatrix = nullptr;
        draw_info->SceneryInst = scenery_instance;
        return;
    }

    {
        bMatrix4 *matrix = eFrameMallocMatrix(1);
        if (matrix == nullptr) {
            return;
        }

        scenery_instance->GetRotation(matrix);
        scenery_instance->GetPosition(&matrix->v3);

        if (scenery_instance->ExcludeFlags & scenery_cull_info->ExcludeFlags & 0x100) {
            matrix->v3.z += EnvMapShadowExtraHeight;
        }
        if (scenery_cull_info->ExcludeFlags & 0x800) {
            matrix->v2.z = -matrix->v2.z;
        }

        {
            SceneryDrawInfo *draw_info = scenery_cull_info->pCurrentDrawInfo;
            if (draw_info >= scenery_cull_info->pTopDrawInfo) {
                return;
            }

            scenery_cull_info->pCurrentDrawInfo = draw_info + 1;
            draw_info->pModel = reinterpret_cast<eModel *>(reinterpret_cast<uintptr_t>(model) + visibility_state);
            if ((scenery_cull_info->ExcludeFlags & 0x4000) && (model->GetSolid() != nullptr) && (model->GetSolid()->Flags & 0x80)) {
                bMatrix4 windrot;
                int offset = static_cast<int>(matrix->v3.x * 60.0f) % 0x168; // TODO magic
                CreateWindRotMatrix(scenery_cull_info->pView, &windrot, offset, matrix);
                bMulMatrix(matrix, matrix, &windrot);
            }

            draw_info->pMatrix = matrix;
            draw_info->SceneryInst = scenery_instance;

            if (scenery_cull_info->pView == eGetView(1, false) || scenery_cull_info->pView == eGetView(2, false)) {
                ePositionMarker *position_marker = nullptr;
                while ((position_marker = model->GetPostionMarker(position_marker)) != nullptr) {
                    if (model->GetSolid() != nullptr) {
                        eLightFlare *light_flare = eGetNextLightFlareInPool(scenery_cull_info->pView == eGetView(1, false) ? 1 : 2);
                        if (light_flare != nullptr) {
                            bVector4 ps;
                            ps.x = position_marker->Matrix.v3.x - model->GetSolid()->PivotMatrix.v3.x;
                            ps.y = position_marker->Matrix.v3.y - model->GetSolid()->PivotMatrix.v3.y;
                            ps.z = position_marker->Matrix.v3.z - model->GetSolid()->PivotMatrix.v3.z;
                            ps.w = 1.0f;
                            eMulVector(&ps, draw_info->pMatrix, &ps);

                            if ((scenery_cull_info->pView->Precipitation != nullptr) &&
                                0.0f < scenery_cull_info->pView->Precipitation->GetRoadDampness() &&
                                (light_flare->PositionX != ps.x || light_flare->PositionY != ps.y || light_flare->PositionZ != ps.z)) {
                                light_flare->ReflectPosZ = 999.0f;
                            }

                            light_flare->PositionX = ps.x;
                            light_flare->PositionY = ps.y;
                            light_flare->PositionZ = ps.z;
                            light_flare->Type = position_marker->iParam0 + ELF_CATSEYE_ORANGE;
                            if (position_marker->iParam0 >= ELF_CAR_REVERSELIGHT && position_marker->iParam0 <= ELF_CAR_COPLIGHTRED) {
                                light_flare->Flags = 4;
                                bVector2 dr(ps.x - draw_info->pMatrix->v3.x, ps.y - draw_info->pMatrix->v3.y);
                                bNormalize(&dr, &dr);
                                light_flare->DirectionX = dr.x;
                                light_flare->DirectionY = dr.y;
                                light_flare->DirectionZ = 0.0f;
                            } else {
                                light_flare->Flags = 2;
                            }
                        }
                    }
                }
            }
        }
    }
}

// gcdatos7: sin llamantes en ninguna version (DIE .debug+0x40C61BE low_pc=-1, tras
// DrawAScenery; A124 164 B, lineas 490-494 de su Scenery.cpp). Unico bloque con
// node/bbox_min/bbox_max/visibility_state y el inline GetBBox: prueba la caja del
// nodo raiz contra la vista. La forma exacta del retorno no esta en la evidencia.
int ScenerySectionHeader::IsVisible(SceneryCullInfo *scenery_cull_info) {
    SceneryTreeNode *node = SceneryTreeNodeTable;
    bVector3 bbox_min;
    bVector3 bbox_max;
    node->GetBBox(&bbox_min, &bbox_max);
    unsigned int visibility_state = scenery_cull_info->pView->GetVisibleState(&bbox_min, &bbox_max, nullptr);
    return visibility_state;
}

// gcdatos7: sin llamantes (DIE .debug+0x40C6505 low_pc=-1; A124 116 B, lineas
// 495-498). Unico bloque {scenery_instance_number}, sin inlines: dibuja todas las
// instancias con prueba por instancia (EVISIBLESTATE_PARTIAL en DrawAScenery).
void ScenerySectionHeader::CullBruteForce(SceneryCullInfo *scenery_cull_info) {
    for (int scenery_instance_number = 0; scenery_instance_number < NumSceneryInstances; scenery_instance_number++) {
        DrawAScenery(scenery_instance_number, scenery_cull_info, EVISIBLESTATE_PARTIAL);
    }
}

void ScenerySectionHeader::TreeCull(SceneryCullInfo *scenery_cull_info) {
    const int max_depth = 64;
    SceneryTreeNode *node_stack[max_depth];
    unsigned char visibility_state_stack[max_depth];
    SceneryTreeNode **pnode = node_stack + 1;
    unsigned char *pvisibility_state = visibility_state_stack + 1;

    node_stack[0] = SceneryTreeNodeTable;
    visibility_state_stack[0] = 1;
    while (pnode != node_stack) {
        pnode--;
        SceneryTreeNode *node = *pnode;
        pvisibility_state--;
        unsigned char visibility_state = *pvisibility_state;
        if (visibility_state == 1) {
            bVector3 bbox_min;
            bVector3 bbox_max;

            node->GetBBox(&bbox_min, &bbox_max);
            visibility_state = scenery_cull_info->pView->GetVisibleState(&bbox_min, &bbox_max, nullptr);
        }

        if (visibility_state != 0) {
            for (int child_number = 0; child_number < node->NumChildren; child_number++) {
                short child_code = node->ChildCodes[child_number];
                if (child_code >= 0) {
                    DrawAScenery(child_code, scenery_cull_info, visibility_state);
                } else {
                    child_code = -child_code;
                    SceneryTreeNode *child_node = &SceneryTreeNodeTable[child_code];
                    *pnode = child_node;
                    *pvisibility_state = visibility_state;
                    pnode++;
                    pvisibility_state++;
                }
            }
        }
    }
}

// gcdatos7: sin llamantes (DIE .debug+0x40C6B72 low_pc=-1, tras TreeCull; A124
// 344 B, lineas 515-523). La version recursiva del nucleo de TreeCull: bloque
// {bbox_min/max + GetBBox} y bloque {child_number {child_code
// {scenery_instance_number} {child_node}}} (las dos ramas del if/else).
void ScenerySectionHeader::CullNodeRecursive(SceneryTreeNode *node, SceneryCullInfo *scenery_cull_info, uint32 visibility_state) {
    if (visibility_state == 1) {
        bVector3 bbox_min;
        bVector3 bbox_max;

        node->GetBBox(&bbox_min, &bbox_max);
        visibility_state = scenery_cull_info->pView->GetVisibleState(&bbox_min, &bbox_max, nullptr);
    }

    if (visibility_state != 0) {
        for (int child_number = 0; child_number < node->NumChildren; child_number++) {
            short child_code = node->ChildCodes[child_number];
            if (child_code >= 0) {
                int scenery_instance_number = child_code;
                DrawAScenery(scenery_instance_number, scenery_cull_info, visibility_state);
            } else {
                child_code = -child_code;
                SceneryTreeNode *child_node = &SceneryTreeNodeTable[child_code];
                CullNodeRecursive(child_node, scenery_cull_info, visibility_state);
            }
        }
    }
}

int GrandSceneryCullInfo::WhatSectionsShouldWeDraw(short *sections_to_draw, int max_sections_to_draw, SceneryCullInfo *scenery_cull_info) {
    DrivableScenerySection *drivable_scenery_section;
    int iViewID = scenery_cull_info->pView->GetID();
#ifdef EA_BUILD_A124
    // El prototipo busca siempre desde la camara del jugador 1.
    if (iViewID == EVIEW_SHADOWMAP1 || iViewID == EVIEW_SHADOWMAP2) {
        drivable_scenery_section = TheVisibleSectionManager.FindDrivableSection(
            reinterpret_cast<const bVector2 *>(eGetView(EVIEW_PLAYER1, false)->GetCamera()->GetPosition()));
#else
    if (iViewID == EVIEW_SHADOWMAP1 || iViewID == EVIEW_SHADOWMAP2) {
        int iViewPlayer = iViewID - EVIEW_SHADOWMAP1 + EVIEW_PLAYER1;
        drivable_scenery_section = TheVisibleSectionManager.FindDrivableSection(
            reinterpret_cast<const bVector2 *>(eGetView(iViewPlayer, false)->GetCamera()->GetPosition()));
#endif
    } else {
        drivable_scenery_section =
            TheVisibleSectionManager.FindDrivableSection(reinterpret_cast<const bVector2 *>(scenery_cull_info->pView->GetCamera()->GetPosition()));
    }

    int num_sections_to_draw = 0;
    if (drivable_scenery_section == nullptr) {
        for (ScenerySectionHeader *section_header = ScenerySectionHeaderList.GetHead(); section_header != ScenerySectionHeaderList.EndOfList();
             section_header = section_header->GetNext()) {
            int section_number = section_header->GetSectionNumber();
            int subsection_number = GetScenerySubsectionNumber(section_number);
            if (subsection_number < 10 && num_sections_to_draw < max_sections_to_draw) {
                sections_to_draw[num_sections_to_draw] = static_cast<short>(section_number);
                num_sections_to_draw++;
            }
        }
    } else {
        for (int section_number = MakeScenerySectionNumber('Z', 0); section_number < MakeScenerySectionNumber('Z', 100); section_number++) {
            if ((GetScenerySectionHeader(section_number) != nullptr) && num_sections_to_draw < max_sections_to_draw) {
                ScenerySectionHeader *section_header; // ?
                sections_to_draw[num_sections_to_draw] = static_cast<short>(section_number);
                num_sections_to_draw++;
            }
        }

        for (int i = 0; i < drivable_scenery_section->GetNumVisibleSections(); i++) {
            int section_number = drivable_scenery_section->GetVisibleSection(i);
            if (section_number >= 0 && (GetScenerySectionHeader(section_number) != nullptr) && num_sections_to_draw < max_sections_to_draw) {
                ScenerySectionHeader *section_header; // ?
                sections_to_draw[num_sections_to_draw] = static_cast<short>(section_number);
                num_sections_to_draw++;
            }
        }
    }

    if (scenery_cull_info->pView->GetID() == EVIEW_PLAYER1) {
        int current_zone_number = -1;
        if (drivable_scenery_section != nullptr) {
            current_zone_number = drivable_scenery_section->GetSectionNumber();
        }

        if (current_zone_number != CurrentZoneNumber) {
            CurrentZoneNumber = current_zone_number;
            if (!SeeulatorToolActive) {
                return num_sections_to_draw;
            }
            if (drivable_scenery_section != nullptr) {
                bFunkCallASync("Seeulator", 1, &CurrentZoneNumber, 4);
            }
        }

        if (SeeulatorToolActive) {
            if (ScenerySectionToBlink != 0 && ((RealTimeFrames / 5) & 1U) != 0) {
                num_sections_to_draw = ToggleIsInTable(sections_to_draw, num_sections_to_draw, max_sections_to_draw, ScenerySectionToBlink);
            }
            if (SeeulatorToolActive && SeeulatorRefreshTrackStreamer != 0) {
                void RefreshTrackStreamer();
                RefreshTrackStreamer();
                SeeulatorRefreshTrackStreamer = 0;
            }
        }
    }

    return num_sections_to_draw;
}

void GrandSceneryCullInfo::CullView(SceneryCullInfo *scenery_cull_info) {
    ProfileNode profile_node;
    const int max_sections_to_draw = 128;
    short sections_to_draw[max_sections_to_draw];
    int num_sections_to_draw = WhatSectionsShouldWeDraw(sections_to_draw, max_sections_to_draw, scenery_cull_info);

    for (int i = 0; i < num_sections_to_draw; i++) {
        int section_number = sections_to_draw[i];
        if (section_number >= 0) {
            ScenerySectionHeader *section_header = GetScenerySectionHeader(section_number);
            if ((section_header != nullptr) && (section_header->SceneryTreeNodeTable != nullptr)) {
                section_header->TreeCull(scenery_cull_info);
            }
        }
    }
}

void GrandSceneryCullInfo::DoCulling() {
    ProfileNode profile_node;
    pFirstDrawInfo = SceneryDrawInfoTable;
    pCurrentDrawInfo = SceneryDrawInfoTable;
    pTopDrawInfo = SceneryDrawInfoTable + 3500;

    int n;
    for (n = 0; n < NumCullInfos; n++) {
        SceneryCullInfo *scenery_cull_info = &SceneryCullInfos[n];
        bool enable_preculling = true;

        scenery_cull_info->Position = *scenery_cull_info->pView->GetCamera()->GetPosition();
        scenery_cull_info->Direction = *scenery_cull_info->pView->GetCamera()->GetDirection();
        scenery_cull_info->H = scenery_cull_info->pView->H;

        if (PrecullerMode == PRECULLER_MODE_OFF) {
            enable_preculling = false;
        } else if (PrecullerMode == PRECULLER_MODE_BLINKING) {
            int time = RealTimeFrames % 0x3D;
            if (time < 0xF) {
                enable_preculling = false;
            }
        } else if (PrecullerMode == PRECULLER_MODE_HIGH_SPEED) {
            Camera *camera = scenery_cull_info->pView->GetCamera();
            if (bLength(camera->GetVelocityPosition()) < EnablePrecullingSpeed) {
                enable_preculling = false;
            }
        }

        if (DisablePrecullerCounter > 0 && PrecullerMode != PRECULLER_MODE_BLINKING) {
            enable_preculling = false;
        }

        if (gPrecullerBooBooManager.IsSet(scenery_cull_info->Position)) {
            enable_preculling = false;
        }
        scenery_cull_info->PrecullerSectionNumber = -1;
        if (enable_preculling) {
            scenery_cull_info->PrecullerSectionNumber = GetPrecullerSectionNumber(scenery_cull_info->Position.x, scenery_cull_info->Position.y);
        }
    }

    for (n = 0; n < NumCullInfos; n++) {
        SceneryCullInfo *scenery_cull_info = &SceneryCullInfos[n];
        scenery_cull_info->pFirstDrawInfo = pCurrentDrawInfo;
        scenery_cull_info->pCurrentDrawInfo = pCurrentDrawInfo;
        scenery_cull_info->pTopDrawInfo = pTopDrawInfo;
        CullView(scenery_cull_info);
        pCurrentDrawInfo = scenery_cull_info->pCurrentDrawInfo;
    }

    // TODO
    {
        SceneryCullInfo *scenery_cull_info;
    }
    {
        SceneryCullInfo *scenery_cull_info;
    }

    for (n = 0; n < NumCullInfos; n++) {
        SceneryCullInfo *scenery_cull_info = &SceneryCullInfos[n];
        // TODO
        if (false) {
            for (SceneryDrawInfo *info = scenery_cull_info->pFirstDrawInfo; info < scenery_cull_info->pCurrentDrawInfo; info++) {
                eModel *model = reinterpret_cast<eModel *>(reinterpret_cast<uintptr_t>(info->pModel) & ~3);
                if (model->HasSolid()) {
                    model->GetSolid();
                }
            }
        }
    }
}

void GrandSceneryCullInfo::StuffScenery(eView *view, int stuff_flags) {
    ProfileNode profile_node;
    Camera *camera = view->GetCamera();
    bVector4 *cpos = reinterpret_cast<bVector4 *>(camera->GetPosition());
    unsigned int extra_flags = 0;
    unsigned int exclude_flags = 0;
    unsigned int exclusive_flags = 0;

    if (stuff_flags & 1) {
        extra_flags = 0x1000;
    }
    if (stuff_flags & 0x400) {
        exclusive_flags = 0x100000;
    }
    if (stuff_flags & 0x80) {
        extra_flags |= 0x100;
    }
    if (stuff_flags & 0x10) {
        exclusive_flags = 0x2000;
    } else if (stuff_flags & 8) {
        exclude_flags = 0x2000;
    }
    if (stuff_flags & 0x300) {
        exclusive_flags = 0x10000;
    }
    if (stuff_flags & 0x800) {
        exclude_flags |= 0x400000;
    }
#ifndef EA_BUILD_A124
    if (stuff_flags & 0x1000) {
        exclusive_flags = 0x1000000;
    }
#endif

    for (int n = 0; n < NumCullInfos; n++) {
        SceneryCullInfo *scenery_cull_info = &SceneryCullInfos[n];
        if (scenery_cull_info->pView != view) {
            continue;
        }

        for (SceneryDrawInfo *info = scenery_cull_info->pFirstDrawInfo; info < scenery_cull_info->pCurrentDrawInfo; info++) {
            // They store data in the lower bits of the pointer
            // *free real estate*
            eVisibleState vis_state = static_cast<eVisibleState>(reinterpret_cast<uintptr_t>(info->pModel) & 3);
            unsigned int flags = extra_flags;
            bMatrix4 *positMarkMatrix;

            pDebugModel = reinterpret_cast<eModel *>(reinterpret_cast<uintptr_t>(info->pModel) & ~3);
            eSceneryLightContext *sceneryLightContext = nullptr;

            if (info->SceneryInst->ExcludeFlags & 0x80) {
                flags |= 0x2000;
            }
            if (info->SceneryInst->ExcludeFlags & 0x1000000) {
                flags |= 0x100000;
            }
#ifdef EA_BUILD_A124
            if (info->SceneryInst->ExcludeFlags & 0x100) {
                flags |= 0x20000;
            }
#else
            if (info->SceneryInst->ExcludeFlags & 0x100) {
                flags |= 0x1020000;
            }
#endif
            if (info->SceneryInst->ExcludeFlags & 0x400000) {
                flags |= 0x40000;
            }
            if (info->SceneryInst->ExcludeFlags & 0x20000) {
                flags |= 0x800000;
            }
            if (info->SceneryInst->ExcludeFlags & 0x80000) {
                flags |= 0x400000;
            }
            if (stuff_flags & 0x200) {
                bool isShadowMapMesh = (info->SceneryInst->ExcludeFlags & 0x4000000) != 0;
                if ((info->SceneryInst->ExcludeFlags & 0x200000) != 0 && !isShadowMapMesh) {
                    flags |= 0x10000;
                }
                if ((info->SceneryInst->ExcludeFlags & 0x40000000) != 0) {
                    flags |= 0x10000;
                }
            }
            if (stuff_flags & 0x100) {
                if ((info->SceneryInst->ExcludeFlags & 0x200000) != 0) {
                    flags |= 0x10000;
                }
#ifndef EA_BUILD_A124
                if ((info->SceneryInst->ExcludeFlags & 0x40000000) != 0) {
                    flags |= 0x10000;
                }
#endif
            }
            if (vis_state == EVISIBLESTATE_FULL) {
                flags |= 4;
            }

#ifdef EA_BUILD_A124
            bool A = exclusive_flags == 0;
#else
            bool A = false;
            if (exclusive_flags == 0) {
                A = exclude_flags == 0;
            }
#endif

            bool B = false;
            if (exclusive_flags != 0) {
                if (flags & exclusive_flags) {
                    B = true;
                }
            }

            bool C = false;
            if (exclude_flags != 0) {
                C = (flags & exclude_flags) == 0;
            }
            if (A || B || C) {
                if (info->pMatrix == nullptr) {
                    bMatrix4 *local_world = eGetIdentityMatrix();
                    view->Render(pDebugModel, local_world, nullptr, flags, nullptr);
                } else {
                    view->Render(pDebugModel, info->pMatrix, nullptr, flags, nullptr);
                }
                pDebugModel = reinterpret_cast<eModel *>(sceneryLightContext);
            }
        }
        return;
    }
}

// gcdatos7: sin llamantes (DIE .debug+0x40C8BF8 low_pc=-1, tras StuffScenery; A124
// 124 B, lineas 699-705). Locales profile_node (ctor de 2 args: la forma
// (nullptr, 0) de la ronda 33, sin literal) y camera, como el prologo de
// StuffScenery; bucle {n {scenery_cull_info {info}}} como el de DoCulling.
void GrandSceneryCullInfo::CallbackScenery(eView *view, void (*callback)(SceneryDrawInfo *)) {
    ProfileNode profile_node(nullptr, 0);
    Camera *camera = view->GetCamera();

    for (int n = 0; n < NumCullInfos; n++) {
        SceneryCullInfo *scenery_cull_info = &SceneryCullInfos[n];
        for (SceneryDrawInfo *info = scenery_cull_info->pFirstDrawInfo; info < scenery_cull_info->pCurrentDrawInfo; info++) {
            callback(info);
        }
    }
}

void ServicePreculler() {}
