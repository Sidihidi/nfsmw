#ifndef EAXSOUND_CSISALLOCATOR_H
#define EAXSOUND_CSISALLOCATOR_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/allocator/1.5.0/Allocator/iallocator.h"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"

// El reparto de memoria de csis. Las lineas de los mapas lo situan aqui: CSISAllocator.h:33-53 (Alloc), 57-58 (Alloc
// con alineacion), 62-65 (Free), 70 (g_CSISCoreAllocator) y 71/72 (CSISAllocatorMemAlloc/Free). En las dos PS2 la
// llamada a Free de CSISAllocatorMemFree no se expande (`j Free__17CSISCoreAllocatorPvUi` con a2 = 0) y en GameCube si.

extern SlotPool *pCsisSlotPools[];
extern char *g_pcsCSISAllocString;

// total size: 0x4
struct CSISCoreAllocator : public EA::Allocator::ICoreAllocator {
    void *Alloc(unsigned int size, const char *name, unsigned int flags) override;
    void *Alloc(unsigned int size, const char *name, unsigned int flags, unsigned int align, unsigned int alignOffset) override;
    void Free(void *block, unsigned int size) override;
};

inline void *CSISCoreAllocator::Alloc(unsigned int size, const char *name, unsigned int flags) {
    void *memptr;

    (void)flags;
    memptr = bOMalloc(pCsisSlotPools[0]);
    return memptr;
}

inline void *CSISCoreAllocator::Alloc(unsigned int size, const char *name, unsigned int flags, unsigned int align, unsigned int alignOffset) {
    (void)align;
    (void)alignOffset;
    return this->Alloc(size, g_pcsCSISAllocString, 0);
}

inline void CSISCoreAllocator::Free(void *block, unsigned int size) {
    (void)size;
    bFree(pCsisSlotPools[0], block);
}

extern CSISCoreAllocator g_CSISCoreAllocator; // Decl: 70
static void *CSISAllocatorMemAlloc(unsigned int numBytes) { return g_CSISCoreAllocator.Alloc(numBytes, g_pcsCSISAllocString, 0); }
static void CSISAllocatorMemFree(void *memPtr) { g_CSISCoreAllocator.Free(memPtr, 0); }

#endif
