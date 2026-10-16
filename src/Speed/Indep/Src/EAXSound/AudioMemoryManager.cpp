#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"
#include "./AudioMemoryManager.hpp"
#include "Speed/Indep/Src/EAXSound/EAXCarState.hpp" // needed for symbol order
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#endif

#ifndef EA_PLATFORM_PLAYSTATION2 // no esta en los stabs de la A124: el _GLOBAL_ de PS2 lleva gAudioMemoryManager
int DEBUG_360MEM = 0; // size: 0x4, address: 0xFFFFFFFF, Decl: 15
#endif

AudioMemoryManager gAudioMemoryManager; // Decl: 128

int AudioMemoryPool = -1; // Decl: 147
int SND11MemoryPoolNum;   // size: 0x4, address: 0xFFFFFFFF, Decl: 148

AudioMemoryManager::AudioMemoryManager()
    : m_pMemoryPoolMem(nullptr), //
#ifndef EA_BUILD_A124
      m_pSnd11PoolMem(nullptr),  //
#endif
      m_memoryPoolSize(0),       //
#ifndef EA_BUILD_A124
      m_Snd11MemPoolSize(0),     //
#endif
      m_numMemoryAllocations(0) {}

AudioMemoryManager::~AudioMemoryManager() {}

void AudioMemoryManager::InitMemoryPool(
#ifndef EA_BUILD_A124
    eAUDMEMPOOLTYPE etype,
#endif
    int size) {
#ifndef EA_BUILD_A124
    if (etype == AUD_MAIN_MEM_POOL)
#endif
    {
        m_memoryPoolSize = size;
        m_pMemoryPoolMem = bMalloc(size, "Audio Memory Pool", 0, 0x2000);
        AudioMemoryPool = bGetFreeMemoryPoolNum();
        bInitMemoryPool(AudioMemoryPool, m_pMemoryPoolMem, m_memoryPoolSize, "Audio Memory Pool");
    }
}

void *AudioMemoryManager::AllocateMemory(int size, const char *debug_name, bool FromTop) {
    void *memptr;

    if (size <= bLargestMalloc(AudioMemoryPool)) {
        if (!FromTop) {
            memptr = bMalloc(size, debug_name, 0, (AudioMemoryPool & 0xF) + 0x1000);
        } else {
            memptr = bMalloc(size, debug_name, 0, (AudioMemoryPool & 0xF) + 0x1040);
        }
    } else {
        bMemoryPrintAllocationsByAddress(AudioMemoryPool, 0, 0x7FFFFFFF);
#ifdef EA_BUILD_A124
        bReleasePrintf("ERROR:  AudioMemoryPool is out of memory allocating %d for %s.  Largest = %d, Free = %d\n", size, debug_name,
                       bLargestMalloc(AudioMemoryPool), bCountFreeMemory(AudioMemoryPool));
#endif
        if (!FromTop) {
            memptr = bMalloc(size, debug_name, 0, 0x1000);
        } else {
            memptr = bMalloc(size, debug_name, 0, 0x1040);
        }
    }

    return memptr;
}

void AudioMemoryManager::FreeMemory(void *mem) {
    if (mem != nullptr) {
        bFree(mem);
    }
}

char *AudioMemoryManager::AllocateMemoryChar(int size, const char *debug_name, bool FromTop) {
    return static_cast<char *>(AllocateMemory(size, debug_name, FromTop));
}
