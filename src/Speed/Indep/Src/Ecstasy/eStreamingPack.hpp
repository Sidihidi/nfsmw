#ifndef ECSTASY_ESTREAMING_PACK_H
#define ECSTASY_ESTREAMING_PACK_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/Misc/ResourceLoader.hpp"
#include "Speed/Indep/bWare/Inc/bChunk.hpp"
#include "Speed/Indep/bWare/Inc/bList.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

#include <types.h>
#include <cstddef>

// total size: 0x18
// r92 (familia ANDROID96): el tamano de fila de las tablas indexadas por hash. En las plataformas
// del original las clases caben en 32 bits y el binario trae el literal; en LP64 crecen por su
// puntero y hay que usar el tamano real, porque si no la busqueda por hash caia entre entradas:
// UnloadStreamingEntry no encontraba el hash, no descontaba el RefCount y lo cargado no se
// liberaba nunca (la retencion de las partidas largas, medida en telefono con 30 carreras). Donde
// el literal es cierto se deja el literal, para que el codigo de las cinco versiones no se mueva.
// OJO (r93, ANDROID97): este bloque tiene que ir ANTES de su primer uso. En #if un macro sin
// definir vale 0, y definido despues E_FILA_STREAMING_ENTRY salia siempre 0x18: la cura era inerte
// en las tres tablas.
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__) || defined(__arm64__) || defined(__powerpc64__) || defined(__ppc64__) || defined(_M_X64) || defined(_M_ARM64)
#define E_PLAT_LP64 1
#else
#define E_PLAT_LP64 0
#endif

class eStreamingEntry {
  public:
    uint32 NameHash;        // offset 0x0, size 0x4
    uint32 ChunkByteOffset; // offset 0x4, size 0x4
    int32 ChunkByteSize;    // offset 0x8, size 0x4
    int32 UncompressedSize; // offset 0xC, size 0x4
    uint8 UserFlags;        // offset 0x10, size 0x1
    uint8 Flags;            // offset 0x11, size 0x1
    uint16 RefCount;        // offset 0x12, size 0x2
    uint8 *ChunkData;       // offset 0x14, size 0x4

    void EndianSwap() {
        bPlatEndianSwap(&this->NameHash);
        bPlatEndianSwap(&this->ChunkByteOffset);
        bPlatEndianSwap(&this->ChunkByteSize);
        bPlatEndianSwap(&this->UncompressedSize);
    }
};

// r92: el tamano de fila de esta tabla. Donde la clase cabe en 32 bits, el literal del binario; en
// LP64 crece por ChunkData y hay que usar el tamano real. Ver la nota de E_PLAT_LP64 en
// eStreamingPack.hpp.
#if E_PLAT_LP64
#define E_FILA_STREAMING_ENTRY ((int)sizeof(eStreamingEntry))
#else
#define E_FILA_STREAMING_ENTRY 0x18
#endif

class eStreamingPackHeaderLoadingInfo {
    // total size: 0x10
  public:
    int32 HeaderChunksSize;              // offset 0x0, size 0x4
    int32 HeaderChunksMemCopied;         // offset 0x4, size 0x4
#ifdef EA_BUILD_A124
    void (*LoadingDoneCallback)(uint32); // offset 0x8, size 0x4
    uint32 LoadingDoneCallbackParam;     // offset 0xC, size 0x4
#else
    void (*LoadingDoneCallback)(void *); // offset 0x8, size 0x4
    void *LoadingDoneCallbackParam;      // offset 0xC, size 0x4
#endif
};

extern SlotPool *eStreamingPackSlotPool;

// total size: 0x44
class eStreamingPack : public bTNode<eStreamingPack> {
  public:
    const char *Filename;                         // offset 0x8, size 0x4
    int16 HeaderMemoryPoolNum;                    // offset 0xC, size 0x2
    int8 DisabledFlag;                            // offset 0xE, size 0x1
    eStreamingPackHeaderLoadingInfo *LoadingInfo; // offset 0x10, size 0x4
    int32 HeaderLoaded;                           // offset 0x14, size 0x4
    int32 HeaderSize;                             // offset 0x18, size 0x4
    bChunk *HeaderChunks;                         // offset 0x1C, size 0x4
    ResourceFile *pResourceFile;                  // offset 0x20, size 0x4
    eStreamingEntry *StreamingEntryTable;         // offset 0x24, size 0x4
    int32 StreamingEntryNumEntries;               // offset 0x28, size 0x4
    int32 NumLoadsPending;                        // offset 0x2C, size 0x4
    int32 NumLoadedStreamingEntries;              // offset 0x30, size 0x4
    int32 NumLoadedBytes;                         // offset 0x34, size 0x4
    int32 RefCount;                               // offset 0x38, size 0x4
    struct eSolidListHeader *SolidListHeader;     // offset 0x3C, size 0x4
    struct TexturePackHeader *pTexturePackHeader; // offset 0x40, size 0x4

    // El bMemSet no es del operator new: el original lo hace en CreateStreamingPack (mapa de lineas de GameCube y de la A124).
    void *operator new(size_t size) {
        return bOMalloc(eStreamingPackSlotPool);
    }
    void operator delete(void *ptr) {
        bFree(eStreamingPackSlotPool, ptr);
    }

    // STRIPPED
    void InitForHibernation();
    // STRIPPED
    bool IsHeaderInMemoryPool(void *mem, int mem_size);
    int GetHeaderMemoryEntries(void **memory_entries, int num_memory_entries);

    eStreamingPack() {};

    void RegisterLoadStreamingEntry(eStreamingEntry *entry) {
        this->NumLoadedStreamingEntries++;
        this->NumLoadedBytes += entry->ChunkByteSize;
    }

    void RegisterUnloadStreamingEntry(eStreamingEntry *entry) {
        entry->ChunkData = nullptr;
        this->NumLoadedStreamingEntries--;
        this->NumLoadedBytes -= entry->ChunkByteSize;
    }
};

// total size: 0x10
class eStreamingPackHeaderLoadingInfoPhase1 {
  public:
    const char *Filename;     // offset 0x0, size 0x4
    bChunk *TempHeaderChunks; // offset 0x4, size 0x4
    int32 NextLoadPosition;   // offset 0x8, size 0x4
    int32 NextLoadAmount;     // offset 0xC, size 0x4
};

// total size: 0x28
class eStreamingPackHeaderLoadingInfoPhase2 {
  public:
    const char *Filename;                     // offset 0x0, size 0x4
    bChunk *HeaderChunks;                     // offset 0x4, size 0x4
    int32 HeaderChunksMemCopied;              // offset 0x8, size 0x4
    eStreamingPack *pStreamingPack;           // offset 0xC, size 0x4
    eStreamingEntry *StreamingEntryTable;     // offset 0x10, size 0x4
    int32 StreamingEntryNumEntries;           // offset 0x14, size 0x4
    struct eSolidListHeader *SolidListHeader; // offset 0x18, size 0x4
    TexturePackHeader *pTexturePackHeader;    // offset 0x1C, size 0x4
    int32 LoadResourceFilePosition;           // offset 0x20, size 0x4
    int32 LoadResourceFileAmount;             // offset 0x24, size 0x4
};

// total size: 0x38
class eStreamPackLoader {
  public:
    eStreamPackLoader(int required_chunk_alignment, void (*loaded_streaming_entry_callback)(bChunk *, eStreamingEntry *, eStreamingPack *),
                      void (*unloaded_streaming_entry_callback)(bChunk *, eStreamingEntry *, eStreamingPack *),
                      void (*loading_header_phase1_callback)(eStreamingPackHeaderLoadingInfoPhase1 *),
                      void (*loading_header_phase2_callback)(eStreamingPackHeaderLoadingInfoPhase2 *),
                      void (*unloading_header_callback)(eStreamingPack *));
    int GetMemoryEntries(uint32 *name_hash_table, int num_hashes, void **memory_entries, int num_memory_entries);
    eStreamingPack *GetLoadedStreamingPack(const char *filename);
    eStreamingPack *GetLoadedStreamingPack(uint32 name_hash);
    eStreamingEntry *GetStreamingEntry(uint32 name_hash, eStreamingPack *streaming_pack);
    eStreamingEntry *GetStreamingEntry(uint32 name_hash);
    bChunk *GetAlignedChunkDataPtr(uint8 *chunk_data);
    struct eStreamingPackLoadTable *GetStreamPackLoadingTable();
    // La A124 recibe los parametros de AddQueuedFile2 como int (InternalLoadedStreamingEntryCallback__17eStreamPackLoaderiii
    // y las tres fases de la cabecera); GameCube y las de venta, como void *.
#ifdef EA_BUILD_A124
    static void InternalLoadedStreamingEntryCallback(int callback_param, int error_status, int callback_param2);
#else
    static void InternalLoadedStreamingEntryCallback(void *callback_param, int error_status, void *callback_param2);
#endif
    void InternalLoadStreamingEntry(eStreamingPackLoadTable *loading_table, struct eStreamingPack *streaming_pack,
                                    struct eStreamingEntry *streaming_entry);

    // La API publica de la A124 pasa la llamada de vuelta y su parametro como uint32 (LoadStreamingEntry__...PFUi_vUii,
    // CreateStreamingPack__...PFUi_vUii y DeleteStreamingPack__...PCcPFUi_vUi, que ademas avisa al terminar); GameCube y
    // las de venta, como void *.
#ifdef EA_BUILD_A124
    void LoadStreamingEntry(uint32 *name_hash_table, int num_hashes, void (*callback)(uint32), uint32 param0, int memory_pool_num);
    void LoadStreamingEntry(uint32 name_hash, void (*callback)(uint32), uint32 param0, int memory_pool_num);
#else
    void LoadStreamingEntry(uint32 *name_hash_table, int num_hashes, void (*callback)(void *), void *param0, int memory_pool_num);
    void LoadStreamingEntry(uint32 name_hash, void (*callback)(void *), void *param0, int memory_pool_num);
#endif
    int IsLoaded(uint32 name_hash);
    void UnloadStreamingEntry(uint32 name_hash, eStreamingPack *streaming_pack);
    void UnloadStreamingEntry(uint32 *name_hash_table, int num_hashes);
    void UnloadAllStreamingEntries(const char *filename);
    int IsLoading(const char *filename);
    int TestLoadStreamingEntry(uint32 *name_hash_table, int num_hashes, int memory_pool_num, bool error_if_out_in_main_pool);
    bool DefragmentAllocation(void *allocation);
    void WaitForLoadingToFinish(const char *filename);
#ifdef EA_BUILD_A124
    static void InternalLoadingHeaderPhase1Callback(int callback_param, int error_status, int callback_param2);
    static void InternalLoadingHeaderPhase2Callback(int callback_param, int error_status, int callback_param2);
    static void InternalLoadingHeaderPhase3Callback(int callback_param);
#else
    static void InternalLoadingHeaderPhase1Callback(void *callback_param, int error_status, void *callback_param2);
    static void InternalLoadingHeaderPhase2Callback(void *callback_param, int error_status, void *callback_param2);
    static void InternalLoadingHeaderPhase3Callback(void *callback_param);
#endif
#ifdef EA_BUILD_A124
    eStreamingPack *CreateStreamingPack(const char *filename, void (*callback_function)(uint32), uint32 callback_param, int memory_pool_num);

    int DeleteStreamingPack(const char *filename, void (*callback_function)(uint32) = nullptr, uint32 callback_param = 0);
#else
    eStreamingPack *CreateStreamingPack(const char *filename, void (*callback_function)(void *), void *callback_param, int memory_pool_num);

    int DeleteStreamingPack(const char *filename);
#endif

    void DisableStreamingPack(const char *filename) {}

    void EnableStreamingPack(const char *filename) {}

    void RegisterLoadStreamingEntry(eStreamingEntry *entry) {}

    void RegisterUnloadStreamingEntry(eStreamingEntry *entry) {
        this->NumLoadedStreamingEntries--;
        this->NumLoadedBytes -= entry->ChunkByteSize;
    }

    void InternalUnloadStreamingEntry(eStreamingPack *streaming_pack, eStreamingEntry *streaming_entry);

    bTList<eStreamingPack> *GetLoadedStreamingPackList() {
        return &this->LoadedStreamingPackList;
    }

    int32 RequiredChunkAlignment;                                                          // offset 0x0, size 0x4
    void (*LoadedStreamingEntryCallback)(bChunk *, eStreamingEntry *, eStreamingPack *);   // offset 0x4, size 0x4
    void (*UnloadedStreamingEntryCallback)(bChunk *, eStreamingEntry *, eStreamingPack *); // offset 0x8, size 0x4
    void (*LoadingHeaderPhase1Callback)(eStreamingPackHeaderLoadingInfoPhase1 *);          // offset 0xC, size 0x4
    void (*LoadingHeaderPhase2Callback)(eStreamingPackHeaderLoadingInfoPhase2 *);          // offset 0x10, size 0x4
    void (*UnloadingHeaderCallback)(eStreamingPack *);                                     // offset 0x14, size 0x4
    bTList<eStreamingPack> LoadedStreamingPackList;                                        // offset 0x18, size 0x8

  public:
    int32 PrintLoading;                      // offset 0x20, size 0x4
    int32 PrintInventory;                    // offset 0x24, size 0x4
    const char *(*DebugGetNameFunc)(uint32); // offset 0x28, size 0x4
    int32 NumLoadedStreamingPacks;           // offset 0x2C, size 0x4
    int32 NumLoadedStreamingEntries;         // offset 0x30, size 0x4
    int32 NumLoadedBytes;                    // offset 0x34, size 0x4
};

// total size: 0x10
class eStreamingPackLoadTable {
  public:
    int8 MemoryPoolNum;                  // offset 0x0, size 0x1
    int8 Locked;                         // offset 0x1, size 0x1
    int16 NumLoadsPending;               // offset 0x2, size 0x2
#ifdef EA_BUILD_A124
    void (*Callback)(uint32);            // offset 0x4, size 0x4
    uint32 Param;                        // offset 0x8, size 0x4
#else
    void (*Callback)(void *);            // offset 0x4, size 0x4
    void *Param;                         // offset 0x8, size 0x4
#endif
    eStreamPackLoader *StreamPackLoader; // offset 0xC, size 0x4
};

// total size: 0x14
class eStreamPackLoadEntryInfo : public bTNode<eStreamPackLoadEntryInfo> {
  public:
    eStreamingPack *StreamingPack;   // offset 0x8, size 0x4
    eStreamingEntry *StreamingEntry; // offset 0xC, size 0x4
    uint32 FileOffset;               // offset 0x10, size 0x4
};

extern eStreamPackLoader StreamingTexturePackLoader;
extern eStreamPackLoader StreamingSolidPackLoader;

void InitStreamingPacks();
void *ScanHashTableKey8(unsigned char key_value, void *table_start, int table_length, int entry_key_offset, int entry_size);
void *ScanHashTableKey32(uint32 key_value, void *table_start, int table_length, int entry_key_offset, int entry_size);


#endif
