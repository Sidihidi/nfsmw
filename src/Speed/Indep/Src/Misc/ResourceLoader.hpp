#ifndef MISC_RESOURCE_LOADER_H
#define MISC_RESOURCE_LOADER_H

#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file/driver.h"
#include "Speed/Indep/bWare/Inc/bChunk.hpp"
#include "Speed/Indep/bWare/Inc/bList.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"

enum ResourceFileType {
    RESOURCE_FILE_NONE = 0,
    RESOURCE_FILE_GLOBAL = 1,
    RESOURCE_FILE_FRONTEND = 2,
    RESOURCE_FILE_INGAME = 3,
    RESOURCE_FILE_TRACK = 4,
    RESOURCE_FILE_NIS = 5,
    RESOURCE_FILE_CAR = 6,
    RESOURCE_FILE_LANGUAGE = 7,
    RESOURCE_FILE_REPLAY = 8,
};

enum ResourceFileFlags {
    RESOURCE_FILE_FLAG_HOTCHUNKABLE = 1,
    RESOURCE_FILE_FLAG_USE_TEMPORARY_MEMORY = 2,
    RESOURCE_FILE_FLAG_FREE_AFTER_LOADING = 4,
    RESOURCE_FILE_FLAG_LZCOMPRESSED = 8,
};

// total size: 0x10
struct LoadedHotFileEntry {
    bChunk *pChunk;        // offset 0x0, size 0x4
    int OriginalSize;      // offset 0x4, size 0x4
    int MaxSize;           // offset 0x8, size 0x4
    unsigned int Checksum; // offset 0xC, size 0x4
};

extern SlotPool *ResourceFileSlotPool;

// total size: 0x50
class ResourceFile : public bTNode<ResourceFile> {
  public:
    ResourceFile(const char *filename, ResourceFileType type, int flags, int file_offset, int file_size);
    ~ResourceFile();

    USE_SLOTALLOC(ResourceFileSlotPool);

    void SetAllocationParams(int allocation_params, const char *debug_name);
    void AllocateMemory(bool loading_compressed_file);
    bool IsFreeMemoryEnabled();
    void SetFreeMemoryEnabled(bool enable);
    void FreeMemory();
    // La A124 recibe la llamada de vuelta y su parametro como int (BeginLoading__12ResourceFilePFi_vi; Callback y
    // CallbackParam son int32 en sus stabs), como el resto de su API de colas; GameCube y las de venta, como void *.
#ifdef EA_BUILD_A124
    void BeginLoading(void (*callback)(int), int callback_param);
#else
    void BeginLoading(void (*callback)(void *), void *callback_param);
#endif
    void ManualUnload();
    void ManualReload(bChunk *new_chunks);
    void LoadResourceIfFileTransferFinished();
    int GetSize(int chunk_id, int *pnum_chunks);

    void *GetMemory() {
        return pFirstChunk;
    }

    void AssignMemory(void *mem, int allocation_params, const char *debug_name) {
        pFirstChunk = reinterpret_cast<bChunk *>(mem);
        SetAllocationParams(allocation_params, debug_name);
    }

    // int GetAddress() {}

#ifdef EA_BUILD_A124
    void BeginLoading(void (*callback)(void *), void *callback_param) {
        BeginLoading(reinterpret_cast<void (*)(int)>(callback), reinterpret_cast<int>(callback_param));
    }
#else
    void BeginLoading(ASYNCFILE_CALLBACK *callback, intptr_t callback_param) {
        BeginLoading(reinterpret_cast<void (*)(void *)>(callback), reinterpret_cast<void *>(callback_param));
    }
#endif

    void BeginLoading() {
        BeginLoading(nullptr, 0);
    }

    int IsFinishedLoading() {
        return LoadingFinishedFlag;
    }

    void CallCallback() {
        if (Callback != nullptr) {
            Callback(CallbackParam);
        }
    }

    const char *GetFilename() {
        return Filename;
    }

    const char *GetHotFilename() {
        return HotFilename;
    }

    ResourceFileType GetType() {
        return Type;
    }

    void ChangeFilenameForHotChunking(const char *filename) {
        bFreeSharedString(Filename);
        Filename = bAllocateSharedString(filename);
    }

    void SetHotFileNumber(int number) {
        HotFileNumber = number;
    }

    int GetHotFileNumber() {
        return HotFileNumber;
    }

    // int IsHotChunkable() {}

    bChunk *GetFirstChunk() {
        return pFirstChunk;
    }

    int GetSizeofChunks() {
        return SizeofChunks;
    }

  private:
    // La A124 recibe el parametro como int (FileTransferCallback__12ResourceFileii), como toda su API de colas.
#ifdef EA_BUILD_A124
    static void FileTransferCallback(int param, int error_status);
#else
    static void FileTransferCallback(void *param, int error_status);
#endif

    bool mEnableFreeMemory;                    // offset 0x8, size 0x1
    ResourceFileType Type;                     // offset 0xC, size 0x4
    int Flags;                                 // offset 0x10, size 0x4
    int FileOffset;                            // offset 0x14, size 0x4
    int FileSize;                              // offset 0x18, size 0x4
    const char *Filename;                      // offset 0x1C, size 0x4
    const char *HotFilename;                   // offset 0x20, size 0x4
    const char *AllocationName;                // offset 0x24, size 0x4
    int AllocationParams;                      // offset 0x28, size 0x4
    int FileTransfersInProgress;               // offset 0x2C, size 0x4
    int LoadingFinishedFlag;                   // offset 0x30, size 0x4
#ifdef EA_BUILD_A124
    void (*Callback)(int);                     // offset 0x34, size 0x4
    int CallbackParam;                         // offset 0x38, size 0x4
#else
    void (*Callback)(void *);                  // offset 0x34, size 0x4
    void *CallbackParam;                       // offset 0x38, size 0x4
#endif
    bChunk *pFirstChunk;                       // offset 0x3C, size 0x4
    int SizeofChunks;                          // offset 0x40, size 0x4
    LoadedHotFileEntry *pLoadedHotFileEntries; // offset 0x44, size 0x4
    int NumLoadedHotFileEntries;               // offset 0x48, size 0x4
    int HotFileNumber;                         // offset 0x4C, size 0x4
};

// total size: 0x4C
struct VMFile {
    VMFile();

    bool mInit;          // offset 0x0, size 0x1
    char mFilename[48];  // offset 0x4, size 0x30
    bool mCompressed;    // offset 0x34, size 0x1
    int mSize;           // offset 0x38, size 0x4
    int mSizeOfChunks;   // offset 0x3C, size 0x4
    void *mMainMemAddr;  // offset 0x40, size 0x4
    void *mVirtMemAddr;  // offset 0x44, size 0x4
    bool mUsedTrackPool; // offset 0x48, size 0x1
};

void InitResourceLoader();
void LoadEmbeddedChunks(struct bChunk *chunk, int32 sizeof_chunks, const char *debug_name);
void WaitForResourceLoadingComplete();
int ServiceResourceLoading();

ResourceFile *FindResourceFile(ResourceFileType type);
#ifdef EA_BUILD_A124
ResourceFile *LoadResourceFile(const char *filename, ResourceFileType type, int flags, void (*callback)(int), int callback_param,
                               int file_offset, int file_size);
#else
ResourceFile *LoadResourceFile(const char *filename, ResourceFileType type, int flags, void (*callback)(void *), void *callback_param,
                               int file_offset, int file_size);
#endif
ResourceFile *CreateResourceFile(const char *filename, ResourceFileType type, int flags, int flag_offset, int file_size);
void UnloadResourceFile(ResourceFile *resource_file);
VMFile *LoadFileIntoVirtualMemory(const char *filename, bool compressed, bool use_trackstreampool_as_temp);
void UnloadFileFromVirtualMemory(VMFile *vm_file);

void EndianSwapChunkHeader(bChunk *chunk);
void EndianSwapChunkHeadersRecursive(bChunk *chunks, int32 sizeof_chunks);
void EndianSwapChunkHeadersRecursive(bChunk *first_chunk, bChunk *last_chunk);

int ServiceResourceLoading();
ResourceFile *CreateResourceFile(const char *filename, ResourceFileType type, int flags, int flag_offset, int file_size);
void UnloadResourceFile(ResourceFile *resource_file);
#ifdef EA_BUILD_A124
void SetDelayedResourceCallback(void (*callback)(int), int param);
#else
void SetDelayedResourceCallback(void (*callback)(void *), void *param);
#endif

bool LoadTempPermChunks(bChunk **ppchunks, int *psizeof_chunks, int allocation_params, const char *debug_name);
void MoveChunks(bChunk *dest_chunks, bChunk *source_chunks, int32 sizeof_chunks, const char *debug_name);
void UnloadChunks(bChunk *chunks, int sizeof_chunks, const char *debug_name);
void PostLoadFixup();

void ScratchPadMemCpy(void *dest, const void *src, unsigned int numbytes);

extern int ChunkMovementOffset; // size: 0x4

inline ResourceFile *LoadResourceFile(const char *filename, ResourceFileType type, int flags) {
    return LoadResourceFile(filename, type, flags, nullptr, nullptr, 0, 0);
}

#ifdef EA_BUILD_A124
inline ResourceFile *LoadResourceFile(const char *filename, ResourceFileType type, int flags, void (*callback)(void *), void *callback_param,
                                      int file_offset, int file_size) {
    return LoadResourceFile(filename, type, flags, reinterpret_cast<void (*)(int)>(callback), reinterpret_cast<int>(callback_param),
                            file_offset, file_size);
}
#else
inline ResourceFile *LoadResourceFile(const char *filename, ResourceFileType type, int flags, void (*callback)(intptr_t), intptr_t callback_param,
                                      int file_offset, int file_size) {
    return LoadResourceFile(filename, type, flags, reinterpret_cast<void (*)(void *)>(callback), reinterpret_cast<void *>(callback_param),
                            file_offset, file_size);
}
#endif

inline bool AreChunksBeingMoved() {
    return ChunkMovementOffset != 0;
}

inline int GetChunkMovementOffset() {
    return ChunkMovementOffset;
}

#ifdef EA_BUILD_A124
inline void SetDelayedResourceCallback(void (*callback)(void *), void *param) {
    SetDelayedResourceCallback(reinterpret_cast<void (*)(int)>(callback), reinterpret_cast<int>(param));
}
#else
inline void SetDelayedResourceCallback(void (*callback)(intptr_t), intptr_t param) {
    SetDelayedResourceCallback(reinterpret_cast<void (*)(void *)>(callback), reinterpret_cast<void *>(param));
}
#endif

#endif
