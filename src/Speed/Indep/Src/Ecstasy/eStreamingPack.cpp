// clang-format off
// Maquetado: LoadStreamingEntry va a las lineas del original de la A124 (solo lineas en blanco de relleno delante y dentro):
// la A124 es una build milestone y su bMalloc pasa __LINE__.
#include "./eStreamingPack.hpp"
#include "./EcstasyE.hpp"
#include "Speed/Indep/Src/Misc/LZCompress.hpp"
#include "Speed/Indep/Src/Misc/Platform.h"
#include "Speed/Indep/Src/Misc/Profiler.hpp"
#include "Speed/Indep/Src/Misc/QueuedFile.hpp"
#include "Speed/Indep/Src/Misc/ResourceLoader.hpp"
#include "Speed/Indep/Src/Misc/bFile.hpp"
#include "Speed/Indep/Src/World/CarLoader.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bChunk.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

#include <types.h>

int AllowCompressedStreamingTexturesInThisPoolNum = 0; // size: 0x4, address: 0x8041A5E4
SlotPool *eStreamingPackSlotPool = 0;
eStreamingPackLoadTable QueuedLoadingTables[64]; // size: 0x400, address: 0x8046A770

void InitStreamingPacks() {
    bMemSet(QueuedLoadingTables, 0x0, 0x400);

    const char *test = "eStreamingPackSlotPool";
    eStreamingPackSlotPool = bNewSlotPool(0x44, 0x40, test, GetVirtualMemoryAllocParams());
}

// STRIPPED
void CloseStreamingPacks() {}

void PrintStreamingPackMemoryWarning(const char *malloc_name, int malloc_size, int memory_pool_num) {
    int language_pool_num; // probably used for EU release

    bGetMemoryPoolNum("LanguageMemoryPool");
}

void *ScanHashTableKey32(uint32 key_value, void *table_start, int table_length, int entry_key_offset, int entry_size) {
    uint8 *table_base;
    int low_index;
    int high_index;

    if (table_start == 0 || table_length <= 0 || entry_key_offset + 4U > entry_size) {
        return nullptr;
    }

    high_index = table_length - 1;
    low_index = 0;

    while (low_index <= high_index) {
        int mid_index;
        uint32 mid_key_value;
        uint8 *mid_entry;

        mid_key_value = low_index + high_index;
        mid_index = (int)(mid_key_value + (mid_key_value >> 0x1FU)) >> 1;
        table_base = &((uint8 *)table_start)[mid_index * entry_size];
        if (key_value < *(uint32 *)&table_base[entry_key_offset]) {
            high_index = mid_index - 1;
            continue;
        }
        if (key_value > *(uint32 *)&table_base[entry_key_offset]) {
            low_index = mid_index + 1;
            continue;
        }
        if (mid_index > 0) {
            mid_entry = table_base - entry_size;
            if (*(uint32 *)(mid_entry + entry_key_offset) == key_value) {
                while (true) {
                    table_base = mid_entry;
                    if (--mid_index > 0) {
                        mid_entry = table_base - entry_size;
                        if (*(uint32 *)(mid_entry + entry_key_offset) != key_value) {
                            break;
                        }
                        continue;
                    }
                    break;
                }
            }
        }
        return table_base;
    }
    return nullptr;
}

// STRIPPED
void *ScanHashTableKey16(uint32 key_value /* r8 */, void *table_start /* r4 */, int table_length /* r5 */, int entry_key_offset /* r6 */,
                         int entry_size /* r7 */);

void *ScanHashTableKey8(uint8 key_value, void *table_start, int table_length, int entry_key_offset, int entry_size) {
    uint8 *table_base;
    int low_index;
    int high_index;

    if (table_start == 0 || table_length <= 0 || entry_key_offset + 1U > entry_size) {
        return nullptr;
    }

    high_index = table_length - 1;
    low_index = 0;

    while (low_index <= high_index) {
        int mid_index;
        unsigned int mid_key_value; // TODO: should be uint8
        uint8 *mid_entry;

        mid_key_value = low_index + high_index;
        mid_index = (int)(mid_key_value + (mid_key_value >> 0x1FU)) >> 1;
        table_base = &((uint8 *)table_start)[mid_index * entry_size];
        if (key_value < table_base[entry_key_offset]) {
            high_index = mid_index - 1;
            continue;
        }
        if (key_value > table_base[entry_key_offset]) {
            low_index = mid_index + 1;
            continue;
        }
        if (mid_index > 0) {
            mid_entry = table_base - entry_size;
            if (mid_entry[entry_key_offset] == key_value) {
                while (true) {
                    table_base = mid_entry;
                    if (--mid_index > 0) {
                        mid_entry = table_base - entry_size;
                        if (mid_entry[entry_key_offset] != key_value) {
                            break;
                        }
                        continue;
                    }
                    break;
                }
            }
        }
        return table_base;
    }
    return nullptr;
}

int eStreamingPack::GetHeaderMemoryEntries(void **memory_entries, int num_memory_entries) {
    if (this->HeaderChunks) {
        memory_entries[num_memory_entries++] = (void *)this->HeaderChunks;
    }
    if (this->pResourceFile) {
        memory_entries[num_memory_entries++] = (void *)this->pResourceFile->GetMemory();
    }

    return num_memory_entries;
}

eStreamPackLoader::eStreamPackLoader(int required_chunk_alignment,
                                     void (*loaded_streaming_entry_callback)(bChunk *, eStreamingEntry *, eStreamingPack *),
                                     void (*unloaded_streaming_entry_callback)(bChunk *, eStreamingEntry *, eStreamingPack *),
                                     void (*loading_header_phase1_callback)(eStreamingPackHeaderLoadingInfoPhase1 *),
                                     void (*loading_header_phase2_callback)(eStreamingPackHeaderLoadingInfoPhase2 *),
                                     void (*unloading_header_callback)(eStreamingPack *)) {
    this->PrintLoading = 0;
    this->NumLoadedStreamingPacks = 0;
    this->NumLoadedStreamingEntries = 0;
    this->NumLoadedBytes = 0;
    this->RequiredChunkAlignment = required_chunk_alignment;
    this->LoadedStreamingEntryCallback = loaded_streaming_entry_callback;
    this->UnloadedStreamingEntryCallback = unloaded_streaming_entry_callback;
    this->LoadingHeaderPhase1Callback = loading_header_phase1_callback;
    this->LoadingHeaderPhase2Callback = loading_header_phase2_callback;
    this->UnloadingHeaderCallback = unloading_header_callback;
}

int eStreamPackLoader::GetMemoryEntries(uint32 *name_hash_table, int num_hashes, void **memory_entries, int num_memory_entries) {
    for (int n = 0; n < num_hashes; n++) {
        uint32 name_hash = name_hash_table[n];
        if (name_hash != 0) {
            eStreamingEntry *streaming_entry = this->GetStreamingEntry(name_hash);
            if (streaming_entry && streaming_entry->ChunkData) {
                memory_entries[num_memory_entries++] = streaming_entry->ChunkData;
            }
        }
    }

    return num_memory_entries;
}

eStreamingPack *eStreamPackLoader::GetLoadedStreamingPack(const char *filename) {
    eStreamingPack *streaming_pack;

    for (streaming_pack = this->LoadedStreamingPackList.GetHead(); streaming_pack != this->LoadedStreamingPackList.EndOfList();
         streaming_pack = streaming_pack->GetNext()) {
        if (!bStrCmp(filename, streaming_pack->Filename)) {
            streaming_pack->Remove();
            this->LoadedStreamingPackList.AddHead(streaming_pack);
            return streaming_pack;
        }
    }

    return nullptr;
}

eStreamingPack *eStreamPackLoader::GetLoadedStreamingPack(uint32 name_hash) {
    eStreamingPack *streaming_pack;

    for (streaming_pack = this->LoadedStreamingPackList.GetHead(); streaming_pack != this->LoadedStreamingPackList.EndOfList();
         streaming_pack = streaming_pack->GetNext()) {
        if (this->GetStreamingEntry(name_hash, streaming_pack)) {
            streaming_pack->Remove();
            this->LoadedStreamingPackList.AddHead(streaming_pack);
            return streaming_pack;
        }
    }

    return nullptr;
}

eStreamingEntry *eStreamPackLoader::GetStreamingEntry(uint32 name_hash, eStreamingPack *streaming_pack) {
    if (!streaming_pack->DisabledFlag) {
        int num_entries = streaming_pack->StreamingEntryNumEntries;
        if (streaming_pack->StreamingEntryTable) {
            eStreamingEntry *streaming_entry =
                (eStreamingEntry *)ScanHashTableKey32(name_hash, streaming_pack->StreamingEntryTable, num_entries, 0x0,
                                                      E_FILA_STREAMING_ENTRY);
            if (streaming_entry) {
                return streaming_entry;
            }
        }
    }
    return nullptr;
}

eStreamingEntry *eStreamPackLoader::GetStreamingEntry(uint32 name_hash) {
    eStreamingPack *streaming_pack;

    for (streaming_pack = this->LoadedStreamingPackList.GetHead(); streaming_pack != this->LoadedStreamingPackList.EndOfList();
         streaming_pack = streaming_pack->GetNext()) {
        eStreamingEntry *streaming_entry = this->GetStreamingEntry(name_hash, streaming_pack);
        if (streaming_entry) {
            return streaming_entry;
        }
    }

    return nullptr;
}

bChunk *eStreamPackLoader::GetAlignedChunkDataPtr(unsigned char *chunk_data) {
    int byte_alignment = this->RequiredChunkAlignment - 1;
    return (bChunk *)((uintptr_t)(chunk_data + byte_alignment) & ~(uintptr_t)byte_alignment);
}

eStreamingPackLoadTable *eStreamPackLoader::GetStreamPackLoadingTable() {
    for (int i = 0; i < 0x40; i++) {
        if (QueuedLoadingTables[i].Locked != 0)
            continue;

        QueuedLoadingTables[i].StreamPackLoader = this;
        QueuedLoadingTables[i].Locked = 1;
        return &QueuedLoadingTables[i];
    }

    return nullptr;
}

void eStreamPackLoader::InternalLoadedStreamingEntryCallback(
#ifdef EA_BUILD_A124
    int callback_param, int error_status, int callback_param2
#else
    void *callback_param, int error_status, void *callback_param2
#endif
) {
    eStreamingPackLoadTable *loading_table = (eStreamingPackLoadTable *)callback_param2;
    eStreamingEntry *streaming_entry = (eStreamingEntry *)callback_param;
    eStreamPackLoader *stream_pack_loader = loading_table->StreamPackLoader;
    uint32 name_hash = streaming_entry->NameHash;
    eStreamingPack *streaming_pack = stream_pack_loader->GetLoadedStreamingPack(name_hash);

    streaming_entry->Flags &= ~0x10;
    if (error_status == 0) {
        stream_pack_loader->NumLoadedStreamingEntries++;
        stream_pack_loader->NumLoadedBytes += streaming_entry->ChunkByteSize;
        if (streaming_entry->Flags & 0x20) {
            stream_pack_loader->InternalUnloadStreamingEntry(streaming_pack, streaming_entry);
            streaming_entry->Flags &= ~0x20;
        } else {
            bChunk *loaded_chunks = stream_pack_loader->GetAlignedChunkDataPtr(streaming_entry->ChunkData);
            if (streaming_entry->Flags & 1) {
                LZHeader *lz_header = reinterpret_cast<LZHeader *>(loaded_chunks);

                bPlatEndianSwap(&lz_header->ID);
                bPlatEndianSwap(&lz_header->Version);
                bPlatEndianSwap(&lz_header->Flags);
                bPlatEndianSwap(&lz_header->UncompressedSize);
                bPlatEndianSwap(&lz_header->CompressedSize);

                char malloc_name[128];
                bSPrintf(malloc_name, "%s", streaming_pack->Filename);

                int uncompressed_size = streaming_entry->UncompressedSize;
                int malloc_size = uncompressed_size + stream_pack_loader->RequiredChunkAlignment;
                int memory_pool_num = loading_table->MemoryPoolNum;
                int allocation_params = BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT);
                if (memory_pool_num != 0) {
                    if (bLargestMalloc(memory_pool_num) > malloc_size) {
                        allocation_params |= (memory_pool_num & 0xF);
                    } else {
                        PrintStreamingPackMemoryWarning(malloc_name, malloc_size, memory_pool_num);
                    }
                }
                uint8 *uncompressed_data = (uint8 *)bMalloc(malloc_size, malloc_name, 0, allocation_params);
                uint8 *compressed_data = (uint8 *)loaded_chunks;
                uint8 *base_loaded_data = streaming_entry->ChunkData;
                streaming_entry->ChunkData = uncompressed_data;

                loaded_chunks = stream_pack_loader->GetAlignedChunkDataPtr(streaming_entry->ChunkData);
                // what?
                int32 actual_decompressed_size = LZDecompress(compressed_data, (uint8 *)loaded_chunks);
                bFree(base_loaded_data);
            }

            stream_pack_loader->LoadedStreamingEntryCallback(loaded_chunks, streaming_entry, streaming_pack);
        }
    }

    if (streaming_pack) {
        streaming_pack->NumLoadsPending--;
    }
    if (loading_table) {
        loading_table->NumLoadsPending--;
        if (!loading_table->NumLoadsPending) {
            if (loading_table->Callback) {
                loading_table->Callback(loading_table->Param);
            }
            loading_table->StreamPackLoader = 0;
            loading_table->Locked = 0;
            loading_table->NumLoadsPending = 0;
            loading_table->Callback = 0;
            loading_table->Param = 0;
        }
    }
}

void eStreamPackLoader::InternalLoadStreamingEntry(eStreamingPackLoadTable *loading_table, eStreamingPack *streaming_pack,
                                                   eStreamingEntry *streaming_entry) {
    uint32 name_hash = streaming_entry->NameHash;

    if (!loading_table || !streaming_pack || !streaming_entry)
        return;

    if (streaming_entry->RefCount != 0) {
        streaming_entry->RefCount++;
        return;
    }

    if (streaming_entry->Flags & 0x10) {
        streaming_entry->Flags &= ~0x20;
        streaming_entry->RefCount++;
        streaming_pack->RegisterLoadStreamingEntry(streaming_entry);
    } else {
        char malloc_name[128];

        bSPrintf(malloc_name, "%s%s", streaming_pack->Filename, (streaming_entry->Flags & 0x1) ? " - Compressed" : "");

        int malloc_size = streaming_entry->ChunkByteSize + this->RequiredChunkAlignment;
        int allocation_params = BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT);
        int memory_pool_num = loading_table->MemoryPoolNum;

        if (streaming_entry->Flags & 0x1) {
            allocation_params = BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT) | 0x40;
            if (memory_pool_num != AllowCompressedStreamingTexturesInThisPoolNum) {
                memory_pool_num = 0;
            }
        }
        if (memory_pool_num != 0) {
            if (bLargestMalloc(memory_pool_num) > malloc_size) {
                allocation_params |= (memory_pool_num & 0xF);
            } else {
                PrintStreamingPackMemoryWarning(malloc_name, malloc_size, memory_pool_num);
            }
        }

        streaming_entry->ChunkData = (unsigned char *)bMalloc(malloc_size, malloc_name, 0, allocation_params);
        streaming_entry->RefCount++;
        streaming_pack->NumLoadsPending++;
        if (loading_table) {
            loading_table->NumLoadsPending++;
        }
        streaming_entry->Flags |= 0x10;
        bChunk *aligned_chunk_data = this->GetAlignedChunkDataPtr(streaming_entry->ChunkData);
#ifdef EA_BUILD_A124
        AddQueuedFile2(aligned_chunk_data, streaming_pack->Filename, streaming_entry->ChunkByteOffset, streaming_entry->ChunkByteSize,
                       eStreamPackLoader::InternalLoadedStreamingEntryCallback, (int)streaming_entry, (int)loading_table, nullptr);
#else
        AddQueuedFile2(aligned_chunk_data, streaming_pack->Filename, streaming_entry->ChunkByteOffset, streaming_entry->ChunkByteSize,
                       eStreamPackLoader::InternalLoadedStreamingEntryCallback, streaming_entry, loading_table, nullptr);
#endif
        streaming_pack->RegisterLoadStreamingEntry(streaming_entry);
    }
}

// FIXME
int eStreamPackLoaderSortLoadEntries(eStreamPackLoadEntryInfo *a, eStreamPackLoadEntryInfo *b /* r4 */) {
    if (b->StreamingPack < a->StreamingPack) {
        return 0;
    }

    if (b->StreamingPack == a->StreamingPack) {
        if (b->FileOffset < a->FileOffset) {
            return 0;
        }
    }

    return 1;
}

// r75: fuera extern SetDelayedResourceCallback duplicado; ResourceLoader.hpp (include propio) ya lo declara.



















































































































































































































































































































































// Las lineas de esta funcion son las del original de la A124 (en GameCube, dos mas): su bMalloc pasa __LINE__.
#ifdef EA_BUILD_A124
void eStreamPackLoader::LoadStreamingEntry(unsigned int *name_hash_table, int num_hashes, void (*callback)(uint32), uint32 param0, int memory_pool_num) {
#else
void eStreamPackLoader::LoadStreamingEntry(unsigned int *name_hash_table, int num_hashes, void (*callback)(void *), void *param0, int memory_pool_num) {
#endif
    ProfileNode profile_node;

    eStreamingPackLoadTable *loading_table = this->GetStreamPackLoadingTable();

    if (!loading_table)
        return;

    eStreamPackLoadEntryInfo *load_info_table;
    int num_load_entries;
    unsigned int smallest_file_offset;
    int mem_required;
    loading_table->MemoryPoolNum = memory_pool_num;
    loading_table->Callback = callback;
    loading_table->Param = param0;



    bTList<eStreamPackLoadEntryInfo> load_info_list;
    load_info_table = (eStreamPackLoadEntryInfo *)bMalloc(num_hashes * sizeof(eStreamPackLoadEntryInfo), "eStreamPackLoadEntryInfo", __LINE__, 0x40);
    num_load_entries = 0;
    smallest_file_offset = 0x7FFFFFFF;



    for (int i = 0; i < num_hashes; i++) {


        uint32 name_hash = name_hash_table[i];
        eStreamingPack *streaming_pack = this->GetLoadedStreamingPack(name_hash);
        eStreamingEntry *streaming_entry = nullptr;

        if (streaming_pack) {


            streaming_entry = this->GetStreamingEntry(name_hash, streaming_pack);
        }











        if (streaming_entry) {

            eStreamPackLoadEntryInfo *load_info = &load_info_table[num_load_entries];
            unsigned int entry_file_offset = streaming_entry->ChunkByteOffset;

            load_info->FileOffset = entry_file_offset;
            load_info->StreamingPack = streaming_pack;
            load_info->StreamingEntry = streaming_entry;

            if (entry_file_offset < smallest_file_offset) {

                smallest_file_offset = entry_file_offset;
            }

            load_info_list.AddTail(load_info);



            num_load_entries++;
        }
    }

    if (memory_pool_num == 0) bLargestMalloc(0);






    load_info_list.Sort(eStreamPackLoaderSortLoadEntries);


    for (eStreamPackLoadEntryInfo *load_info = load_info_list.GetHead(); load_info != load_info_list.EndOfList(); load_info = load_info->GetNext()) {

        this->InternalLoadStreamingEntry(loading_table, load_info->StreamingPack, load_info->StreamingEntry);
    }



    load_info_list.InitList();
    bFree(load_info_table);


    if (!loading_table->NumLoadsPending && callback) {


        loading_table->Locked = 0;








#ifdef EA_BUILD_A124
        SetDelayedResourceCallback(reinterpret_cast<void (*)(int)>(callback), param0);
#else
        SetDelayedResourceCallback(callback, param0);
#endif
    }


}

#ifdef EA_BUILD_A124
void eStreamPackLoader::LoadStreamingEntry(uint32 name_hash, void (*callback)(uint32), uint32 param0, int memory_pool_num) {
#else
void eStreamPackLoader::LoadStreamingEntry(uint32 name_hash, void (*callback)(void *), void *param0, int memory_pool_num) {
#endif
    LoadStreamingEntry(&name_hash, 1, callback, param0, memory_pool_num);
}

void eStreamPackLoader::InternalUnloadStreamingEntry(eStreamingPack *streaming_pack, eStreamingEntry *streaming_entry) {
    if ((streaming_entry->Flags & 0x20) == 0) {
        bChunk *aligned_chunk = this->GetAlignedChunkDataPtr(streaming_entry->ChunkData);
        this->UnloadedStreamingEntryCallback(aligned_chunk, streaming_entry, streaming_pack);
    }
    bFree(streaming_entry->ChunkData);

    streaming_pack->RegisterUnloadStreamingEntry(streaming_entry);
    this->RegisterUnloadStreamingEntry(streaming_entry);
}

// STRIPPED
int eStreamPackLoader::IsLoaded(uint32 name_hash) {}

void eStreamPackLoader::UnloadStreamingEntry(uint32 name_hash, eStreamingPack *streaming_pack) {

    if (name_hash == 0)
        return;

    if (streaming_pack == nullptr) {
        streaming_pack = this->GetLoadedStreamingPack(name_hash);
        if (streaming_pack == nullptr)
            return;
    }

    eStreamingEntry *streaming_entry = this->GetStreamingEntry(name_hash, streaming_pack);
    if (streaming_entry == nullptr)
        return;

    streaming_entry->RefCount--;
    if (streaming_entry->RefCount == 0) {
        eWaitUntilRenderingDone();
        if ((streaming_entry->Flags & 0x10) != 0) {
            streaming_entry->Flags |= 0x20;
        } else {
            this->InternalUnloadStreamingEntry(streaming_pack, streaming_entry);
        }
    }
}

void eStreamPackLoader::UnloadStreamingEntry(unsigned int *name_hash_table, int num_hashes) {
    for (int i = 0; i < num_hashes; i++) {
        this->UnloadStreamingEntry(name_hash_table[i], nullptr);
    }

    ProfileNode profile_node;
}

void eStreamPackLoader::UnloadAllStreamingEntries(const char *filename) {
    eStreamingPack *streaming_pack = this->GetLoadedStreamingPack(filename);

    if (!streaming_pack)
        return;

    eStreamingEntry *streaming_entry = streaming_pack->StreamingEntryTable;
    int num_entries = streaming_pack->StreamingEntryNumEntries;

    for (int i = 0; i < num_entries; i++, streaming_entry++) {
        if (streaming_entry->ChunkData == nullptr)
            continue;

        streaming_entry->RefCount = 1;
        this->UnloadStreamingEntry(streaming_entry->NameHash, streaming_pack);
    }
}

int eStreamPackLoader::IsLoading(const char *filename) {
    eStreamingPack *streaming_pack = this->LoadedStreamingPackList.GetHead();

    for (; streaming_pack != this->LoadedStreamingPackList.EndOfList(); streaming_pack = streaming_pack->GetNext()) {
        if (filename == nullptr || bStrCmp(filename, streaming_pack->Filename) == 0) {
            if (streaming_pack->HeaderLoaded == 0 || streaming_pack->NumLoadsPending > 0) {
                return 1;
            }
        }
    }

    return 0;
}

int eStreamPackLoader::TestLoadStreamingEntry(unsigned int *name_hash_table, int num_hashes, int memory_pool_num, bool error_if_out_in_main_pool) {
    ProfileNode profile_node;
    int amount_not_alloc = 0;
    int amount_not_alloc_main_mem = 0;
    bTList<eStreamPackLoadEntryInfo> load_info_list;
    eStreamPackLoadEntryInfo *load_info_table =
        (eStreamPackLoadEntryInfo *)bMalloc(num_hashes * sizeof(eStreamPackLoadEntryInfo), "eStreamPackLoadEntryInfo", 0, 0x40);
    int num_load_entries = 0;

    for (int i = 0; i < num_hashes; i++) {
        uint32 name_hash = name_hash_table[i];
        eStreamingPack *streaming_pack = this->GetLoadedStreamingPack(name_hash_table[i]);
        if (streaming_pack) {
            eStreamingEntry *streaming_entry = this->GetStreamingEntry(name_hash, streaming_pack);
            if (streaming_entry) {
                eStreamPackLoadEntryInfo *load_info = &load_info_table[num_load_entries++];
                load_info->FileOffset = streaming_entry->ChunkByteOffset;
                load_info->StreamingPack = streaming_pack;
                load_info->StreamingEntry = streaming_entry;
                load_info_list.AddTail(load_info);
            }
        }
    }

    load_info_list.Sort(eStreamPackLoaderSortLoadEntries);

    for (int pass = 0; pass < 3; pass++) {
        unsigned int prev_name_hash = 0;
        for (eStreamPackLoadEntryInfo *load_info = load_info_list.GetHead(); load_info != load_info_list.EndOfList();
             load_info = load_info->GetNext()) {
            eStreamingEntry *streaming_entry = load_info->StreamingEntry;
            uint32 name_hash = streaming_entry->NameHash;

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // speed.exe y X360 comprueban antes el RefCount que el bit 0x10 de Flags; GameCube casa al reves
            if (streaming_entry && streaming_entry->RefCount == 0 && (streaming_entry->Flags & 0x10) == 0) {
#else
            if (streaming_entry && (streaming_entry->Flags & 0x10) == 0 && streaming_entry->RefCount == 0) {
#endif
                eStreamingPack *streaming_pack = load_info->StreamingPack;

                if (name_hash != prev_name_hash) {
                    prev_name_hash = name_hash;
#ifdef EA_PLATFORM_XENON
                    char malloc_name_uncompressed[128];
#endif
                    if (pass == 0) {
                        char malloc_name[128];
                        int malloc_size;
                        int allocation_params;

                        bSPrintf(malloc_name, "%s%s", streaming_pack->Filename, (streaming_entry->Flags & 1) ? " - Compressed" : "");

                        allocation_params = BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT);
                        malloc_size = streaming_entry->ChunkByteSize + this->RequiredChunkAlignment;

                        if (streaming_entry->Flags & 1) {
                            allocation_params = BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT) | 0x40;
                        } else if (memory_pool_num != 0) {
                            allocation_params = (memory_pool_num & 0xF) | BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT);
                        }

                        if (bLargestMalloc(allocation_params) > malloc_size) {
                            streaming_entry->ChunkData = (unsigned char *)bMalloc(malloc_size, malloc_name, 0, allocation_params);
                        } else {
                            if ((allocation_params & 0xF) == 0) {
                                amount_not_alloc_main_mem += malloc_size;
                            }
                            amount_not_alloc += malloc_size;
                        }
                    } else if (pass == 1) {
#ifdef EA_PLATFORM_XENON
                        char (&malloc_name)[128] = malloc_name_uncompressed;
#else
                        char malloc_name[128];
#endif
                        int malloc_size;
                        int allocation_params;
                        unsigned char *compressed_data;

                        if (streaming_entry->Flags & 1) {
                            bSPrintf(malloc_name, "%s", streaming_pack->Filename);

                            allocation_params = BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT);
                            malloc_size = streaming_entry->UncompressedSize + this->RequiredChunkAlignment;

                            if (memory_pool_num != 0) {
                                allocation_params = (memory_pool_num & 0xF) | BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT);
                            }

                            compressed_data = streaming_entry->ChunkData;
                            streaming_entry->ChunkData = nullptr;

                            if (bLargestMalloc(allocation_params) > malloc_size) {
                                streaming_entry->ChunkData = (unsigned char *)bMalloc(malloc_size, malloc_name, 0, allocation_params);
                            } else {
                                amount_not_alloc += malloc_size;
                                if ((allocation_params & 0xF) == 0) {
                                    amount_not_alloc_main_mem += malloc_size;
                                }
                            }

                            if (compressed_data) {
                                bFree(compressed_data);
                            }
                        }
                    } else if (pass == 2) {
                        if (streaming_entry->ChunkData) {
                            bFree(streaming_entry->ChunkData);
                            streaming_entry->ChunkData = nullptr;
                        }
                    }
                }
            }
        }
    }

    load_info_list.InitList();
    bFree(load_info_table);

    if (error_if_out_in_main_pool && amount_not_alloc_main_mem != 0) {
        bBreak();
    }

    return amount_not_alloc;
}

bool eStreamPackLoader::DefragmentAllocation(void *allocation) {
    for (eStreamingPack *streaming_pack = this->LoadedStreamingPackList.GetHead(); streaming_pack != this->LoadedStreamingPackList.EndOfList();
         streaming_pack = streaming_pack->GetNext()) {
        for (int i = 0; i < streaming_pack->StreamingEntryNumEntries; i++) {
            eStreamingEntry *streaming_entry = &streaming_pack->StreamingEntryTable[i];
            if (streaming_entry->ChunkData == allocation) {
                if (streaming_pack->NumLoadsPending != 0) {
                    bBreak();
                }
                bChunk *chunks = this->GetAlignedChunkDataPtr(streaming_entry->ChunkData);
                this->UnloadedStreamingEntryCallback(chunks, streaming_entry, streaming_pack);
                streaming_entry->ChunkData = (unsigned char *)MoveDefragmentAllocation(allocation);
                chunks = this->GetAlignedChunkDataPtr(streaming_entry->ChunkData);
                this->LoadedStreamingEntryCallback(chunks, streaming_entry, streaming_pack);
                this->LoadedStreamingPackList.Remove(streaming_pack);
                this->LoadedStreamingPackList.AddHead(streaming_pack);
                return 1;
            }
        }
    }

    return 0;
}

void eStreamPackLoader::WaitForLoadingToFinish(const char *filename) {
    while (this->IsLoading(filename)) {
#ifdef EA_PLATFORM_GAMECUBE
        DVDErrorTask(nullptr, 0);
#endif
        bThreadYield(8);
        ServiceResourceLoading();
    }
}

#ifdef EA_BUILD_A124
eStreamingPack *eStreamPackLoader::CreateStreamingPack(const char *filename, void (*callback_function)(uint32), uint32 callback_param,
                                                       int memory_pool_num) {
#else
eStreamingPack *eStreamPackLoader::CreateStreamingPack(const char *filename, void (*callback_function)(void *), void *callback_param,
                                                       int memory_pool_num) {
#endif
    eStreamingPack *streaming_pack = this->GetLoadedStreamingPack(filename);

    if (streaming_pack) {
        streaming_pack->RefCount++;
        return streaming_pack;
    }

    bFile *file = bOpen(filename, 1, 1);
    if (!file)
        return nullptr;

    int file_size = bFileSize(file);
    bClose(file);
    if (file_size <= 0x40)
        return nullptr;

    eStreamingPackHeaderLoadingInfo *loading_info =
        (eStreamingPackHeaderLoadingInfo *)bMalloc(sizeof(eStreamingPackHeaderLoadingInfo), "eStreamingPackLoadingInfo", 0, 0x40);
    bMemSet(loading_info, 0x0, sizeof(eStreamingPackHeaderLoadingInfo));

    loading_info->LoadingDoneCallback = callback_function;
    loading_info->LoadingDoneCallbackParam = callback_param;

    streaming_pack = new eStreamingPack();
    bMemSet(streaming_pack, 0, sizeof(eStreamingPack));
    streaming_pack->Filename = bAllocateSharedString(filename);
    streaming_pack->HeaderMemoryPoolNum = memory_pool_num;
    streaming_pack->HeaderSize = 0;
    streaming_pack->RefCount = 1;
    streaming_pack->LoadingInfo = loading_info;
    this->LoadedStreamingPackList.AddTail(streaming_pack);
    this->NumLoadedStreamingPacks++;

    int amount_to_load = file_size;
#ifdef EA_PLATFORM_WIN32 // speed.exe salta con jl (forma >=, idempotente en el borde: con 0x8000 asigna 0x8000);
                         // GameCube casa con >
    if (file_size >= 0x8000) {
#else
    if (file_size > 0x8000) {
#endif
        amount_to_load = 0x8000;
    }
    streaming_pack->HeaderChunks = (bChunk *)bMalloc(amount_to_load, "eStreamingPack_TempMem", 0, BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT) | 0x40);
    loading_info->HeaderChunksSize = amount_to_load;
#ifdef EA_BUILD_A124
    AddQueuedFile2(streaming_pack->HeaderChunks, streaming_pack->Filename, 0, amount_to_load,
                   eStreamPackLoader::InternalLoadingHeaderPhase1Callback, (int)this, (int)streaming_pack, nullptr);
#else
    AddQueuedFile2(streaming_pack->HeaderChunks, streaming_pack->Filename, 0, amount_to_load,
                   eStreamPackLoader::InternalLoadingHeaderPhase1Callback, (void *)this, (void *)streaming_pack, nullptr);
#endif

    return streaming_pack;
}

void eStreamPackLoader::InternalLoadingHeaderPhase1Callback(
#ifdef EA_BUILD_A124
    int callback_param, int error_status, int callback_param2
#else
    void *callback_param, int error_status, void *callback_param2
#endif
) {
    eStreamPackLoader *stream_pack_loader = (eStreamPackLoader *)callback_param;
    eStreamingPack *streaming_pack = (eStreamingPack *)callback_param2;
    eStreamingPackHeaderLoadingInfo *loading_info = streaming_pack->LoadingInfo;
    eStreamingPackHeaderLoadingInfoPhase1 user_load_info;
    bMemSet(&user_load_info, 0, sizeof(eStreamingPackHeaderLoadingInfoPhase1));

    user_load_info.Filename = streaming_pack->Filename;
    user_load_info.TempHeaderChunks = streaming_pack->HeaderChunks;

    stream_pack_loader->LoadingHeaderPhase1Callback(&user_load_info);

    char malloc_name[256];
    bSPrintf(malloc_name, "%s: StrmHdrChks", streaming_pack->Filename);

    int next_load_position = user_load_info.NextLoadPosition;
    int next_load_amount = user_load_info.NextLoadAmount;
    streaming_pack->HeaderSize = next_load_amount;

    int allocation_params = BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT);
    if (streaming_pack->HeaderMemoryPoolNum != 0 && bLargestMalloc(streaming_pack->HeaderMemoryPoolNum) > next_load_amount) {
        allocation_params = (streaming_pack->HeaderMemoryPoolNum & 0xF) | BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT) | 0x40;
    }

    bChunk *perm_header_chunks = (bChunk *)bMalloc(next_load_amount, malloc_name, 0, allocation_params);
    bChunk *temp_header_chunks = streaming_pack->HeaderChunks;

    if (next_load_position + next_load_amount <= loading_info->HeaderChunksSize) {
        bMemCpy(perm_header_chunks, reinterpret_cast<void *>((uintptr_t)temp_header_chunks + next_load_position), next_load_amount);
        loading_info->HeaderChunksMemCopied = 1;
    }
    bFree(temp_header_chunks);

    streaming_pack->HeaderChunks = perm_header_chunks;
    if (loading_info->HeaderChunksMemCopied == 0) {
#ifdef EA_BUILD_A124
        AddQueuedFile2(perm_header_chunks, streaming_pack->Filename, next_load_position, next_load_amount, InternalLoadingHeaderPhase2Callback,
                       (int)stream_pack_loader, (int)streaming_pack, nullptr);
#else
        AddQueuedFile2(perm_header_chunks, streaming_pack->Filename, next_load_position, next_load_amount, InternalLoadingHeaderPhase2Callback,
                       stream_pack_loader, streaming_pack, nullptr);
#endif
    }

    if (loading_info->HeaderChunksMemCopied != 0) {
#ifdef EA_BUILD_A124
        InternalLoadingHeaderPhase2Callback((int)stream_pack_loader, 0, (int)streaming_pack);
#else
        InternalLoadingHeaderPhase2Callback(stream_pack_loader, 0, streaming_pack);
#endif
    }
}

// FIXME
void eStreamPackLoader::InternalLoadingHeaderPhase2Callback(
#ifdef EA_BUILD_A124
    int callback_param, int error_status, int callback_param2
#else
    void *callback_param, int error_status, void *callback_param2
#endif
) {
    eStreamPackLoader *stream_pack_loader = (eStreamPackLoader *)callback_param;
    eStreamingPack *streaming_pack = (eStreamingPack *)callback_param2;
    eStreamingPackHeaderLoadingInfo *loading_info = streaming_pack->LoadingInfo;
    eStreamingPackHeaderLoadingInfoPhase2 user_load_info;
    bMemSet(&user_load_info, 0, sizeof(eStreamingPackHeaderLoadingInfoPhase2));

    user_load_info.Filename = streaming_pack->Filename;
    user_load_info.HeaderChunks = streaming_pack->HeaderChunks;
    user_load_info.HeaderChunksMemCopied = loading_info->HeaderChunksMemCopied;
    user_load_info.pStreamingPack = streaming_pack;

    stream_pack_loader->LoadingHeaderPhase2Callback(&user_load_info);

    // El volcado DWARF trae aqui un bloque { int n; { eStreamingEntry *entry;
    // eStreamingPack *other_pack; } } con rango VACIO (0x800F61F4 -> 0x800F61F4):
    // un bucle de traza cero. Emite cero instrucciones pero reordena el reparto
    // de registros de las cuatro asignaciones de abajo (r0/r9/r11/r10 en vez de
    // r11/r10/r0/r9) y la colocacion del mtlr. Con el, 0 diffs.
    // La A124 no tiene este bloque (sus stabs no traen n, entry ni other_pack) y sin el casa.
#ifndef EA_BUILD_A124
    {
        int n;
        for (n = 0; n < 0; n++) {
            eStreamingEntry *entry = &user_load_info.StreamingEntryTable[n];
            eStreamingPack *other_pack = streaming_pack;
        }
    }
#endif

    streaming_pack->StreamingEntryTable = user_load_info.StreamingEntryTable;
    streaming_pack->StreamingEntryNumEntries = user_load_info.StreamingEntryNumEntries;
    streaming_pack->SolidListHeader = user_load_info.SolidListHeader;
    streaming_pack->pTexturePackHeader = user_load_info.pTexturePackHeader;

    if (user_load_info.LoadResourceFileAmount != 0) {
        streaming_pack->pResourceFile = CreateResourceFile(streaming_pack->Filename, RESOURCE_FILE_CAR, 0x0, user_load_info.LoadResourceFilePosition,
                                                           user_load_info.LoadResourceFileAmount);

        char malloc_name[1024];
        bSPrintf(malloc_name, "%s: StrmHdrRes", streaming_pack->Filename);

        int allocation_params = BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT);
        if (streaming_pack->HeaderMemoryPoolNum != 0 && bLargestMalloc(streaming_pack->HeaderMemoryPoolNum) > user_load_info.LoadResourceFileAmount) {
            allocation_params = (streaming_pack->HeaderMemoryPoolNum & 0xF) | BMEMORY_ALIGNMENT(BCHUNK_ALIGNMENT) | 0x40;
        }

        streaming_pack->pResourceFile->SetAllocationParams(allocation_params, malloc_name);
#ifdef EA_BUILD_A124
        streaming_pack->pResourceFile->BeginLoading(InternalLoadingHeaderPhase3Callback, (int)streaming_pack);
#else
        streaming_pack->pResourceFile->BeginLoading(InternalLoadingHeaderPhase3Callback, streaming_pack);
#endif
    }

    if (streaming_pack->pResourceFile == nullptr) {
#ifdef EA_BUILD_A124
        InternalLoadingHeaderPhase3Callback((int)streaming_pack);
#else
        InternalLoadingHeaderPhase3Callback(streaming_pack);
#endif
    }
}

void eStreamPackLoader::InternalLoadingHeaderPhase3Callback(
#ifdef EA_BUILD_A124
    int callback_param
#else
    void *callback_param
#endif
) {
    eStreamingPack *streaming_pack = (eStreamingPack *)callback_param;
    eStreamingPackHeaderLoadingInfo *loading_info = streaming_pack->LoadingInfo;
#ifdef EA_BUILD_A124
    void (*user_callback_function)(uint32) = loading_info->LoadingDoneCallback;
    uint32 user_callback_param = loading_info->LoadingDoneCallbackParam;
#else
    void (*user_callback_function)(void *) = loading_info->LoadingDoneCallback;
    void *user_callback_param = loading_info->LoadingDoneCallbackParam;
#endif

    streaming_pack->HeaderLoaded = 1;
    bFree(loading_info);

    streaming_pack->LoadingInfo = 0;
    if (user_callback_function) {
        user_callback_function(user_callback_param);
    }
}

#ifdef EA_BUILD_A124
int eStreamPackLoader::DeleteStreamingPack(const char *filename, void (*callback_function)(uint32), uint32 callback_param) {
#else
int eStreamPackLoader::DeleteStreamingPack(const char *filename) {
#endif
    eStreamingPack *streaming_pack = GetLoadedStreamingPack(filename);

    if (!streaming_pack)
        return 0;
    if (--streaming_pack->RefCount != 0)
        return 1;

    streaming_pack->DisabledFlag = 0;

    this->UnloadAllStreamingEntries(filename);
    this->UnloadingHeaderCallback(streaming_pack);

    streaming_pack->Remove();

    if (streaming_pack->HeaderChunks) {
        bFree(streaming_pack->HeaderChunks);
    }
    if (streaming_pack->pResourceFile) {
        UnloadResourceFile(streaming_pack->pResourceFile);
    }

    bFreeSharedString(streaming_pack->Filename);
    delete streaming_pack;

    this->NumLoadedStreamingPacks--;
#ifdef EA_BUILD_A124
    if (callback_function) {
        callback_function(callback_param);
    }
#endif

    return 1;
}
