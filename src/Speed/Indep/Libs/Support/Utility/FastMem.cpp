#include "FastMem.h"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

void FastMemEmergencyInitialization(unsigned int &bytes, const char *&name, unsigned int &expansion, unsigned int &track);
void *bMalloc(int size, int flags);
void bFree(void *ptr);
void bBreak();

#ifdef EA_PLATFORM_PLAYSTATION2
class ConsoleDumpRecorder : public FastMem::DumpRecorder {
  public:
    void ReportBlockKind(const char *name, unsigned int bytes, unsigned int count, unsigned int highwater);
    void ReportBlockSize(unsigned int size, unsigned int freeblocks, unsigned int usedblocks);
};

void ConsoleDumpRecorder::ReportBlockKind(const char *name, unsigned int bytes, unsigned int count, unsigned int highwater) {}
void ConsoleDumpRecorder::ReportBlockSize(unsigned int size, unsigned int freeblocks, unsigned int usedblocks) {}
#endif

// total size: 0x10
class AllocDesc {
    unsigned int mIndex; // offset 0x0, size 0x4
    const char *mName;   // offset 0x4, size 0x4
    unsigned int mCount; // offset 0x8, size 0x4
    unsigned int mHigh;  // offset 0xC, size 0x4
};

FastMem::FastMem() {}

// STRIPPED
// FastMem::FastMem(EA::Allocator::IAllocator *allocator, unsigned int bytes, const char *name, unsigned int expansionsize, unsigned int trackingsize)
// {}

void FastMem::Init() {
    if (!mInited) {
        FastMemEmergencyInitialization(mBytes, mName, mExpansionSize, mTrackMax);
        for (unsigned int i = 0; i < 64; i++) {
            mFreeLists[i] = 0;
            mAlloc[i] = 0;
            mAvail[i] = 0;
        }
        mBlock = CoreAlloc(mBytes, mName);
        // speed.exe cierra el if con `jbe` (mExpansionSize > 0 sin signo), no con `je` (msvc 7.1 no pliega uno al otro).
        if (mExpansionSize > 0) {
            *(unsigned int *)mBlock = 0;
            ((unsigned int *)mBlock)[1] = mBytes;
            mUsed = 16;
        }
        mInited = true;
    }
}

#ifdef EA_PLATFORM_PLAYSTATION2
void FastMem::Deinit() {
    if (mInited) {
        void *block = mBlock;
        while (block) {
            void *next = 0;
            if (mExpansionSize) {
                next = ((void **)block)[0];
            }
            CoreFree(block);
            block = next;
        }
        mAllocOver = 0;
        mName = 0;
        mBlock = 0;
        mBytes = 0;
        mUsed = 0;
        mExpansionSize = 0;
        for (unsigned int i = 0; i < 64; i++) {
            mFreeLists[i] = 0;
            mAlloc[i] = 0;
            mAvail[i] = 0;
        }
    }
}
#endif

void *FastMem::Alloc(std::size_t bytes, const char *kind) {
    void *result;
    if (bytes > 0x400) {
        result = CoreAlloc(bytes, kind);
#ifdef EA_PLATFORM_WIN32
        bMemSet(result, 0, bytes); // PC llama a bMemSet; las PS2, a bMemZero (bMemZero__FPvUi)
#else
        bMemZero(result, bytes);
#endif
    } else {
        std::size_t listIndex = (bytes - 1) >> 4;
        if (mFreeLists[listIndex] == 0) {
            if (CreateBlock(listIndex) == 0) {
                DumpRecord();
                bReleasePrintf("Out of Fastmem memory\n");
                bBreak();
                return 0;
            }
        }
        // Sin local para la cabeza (el DWARF de GC solo da result y listIndex): releerla de la lista hace que X360
        // conserve el null check del `new` tras expandir Alloc, como el EGM.
        result = mFreeLists[listIndex];
        mFreeLists[listIndex] = mFreeLists[listIndex]->mNext;
#ifdef EA_PLATFORM_WIN32
        bMemSet(result, 0, bytes);
#else
        bMemZero(result, bytes);
#endif
    }
    return result;
}

void FastMem::Free(void *ptr, std::size_t bytes, const char *kind) {
    if (bytes > 0x400) {
        CoreFree(ptr);
    } else {
        size_t listIndex = (bytes - 1) / 16;
        FreeBlock *freeBlk = reinterpret_cast<FreeBlock *>(ptr);
        freeBlk->mNext = this->mFreeLists[listIndex]; // TODO bug? shouldn't this be FastMem::FreeBlock::mNext?
        this->mFreeLists[listIndex] = freeBlk;
    }
}

void *FastMem::CoreAlloc(std::size_t bytes, const char *kind) {
#ifdef MILESTONE_BUILD // A124 y X360 (NFS.exe 0x82341238)
    return bMalloc(bytes, kind, 0, 0);
#else
    return bMalloc(bytes, 0);
#endif
}

void FastMem::CoreFree(void *ptr) {
    bFree(ptr);
}

bool FastMem::SplitOrExpand(std::size_t bytes) {
    void *block;
    if (mExpansionSize == 0 || mLocks != 0) {
        return false;
    }
    block = CoreAlloc(mExpansionSize, mName);
    if (block == 0) {
        return false;
    }
    ((void **)block)[0] = mBlock;
    ((unsigned int *)block)[1] = mExpansionSize;
    mBlock = block;
    mBytes = mExpansionSize;
    mUsed = 16;
    return true;
}

bool FastMem::AssignToFree(std::size_t bytes) {
    if (mBytes < mUsed + bytes) {
        unsigned int remaining = mBytes - mUsed;
        if (remaining > 16 && (mUsed & 0x1F)) {
            AssignToFree(16);
        }
        while ((remaining = mBytes - mUsed) > 32) {
            AssignToFree(32);
        }
        if (SplitOrExpand(bytes) == 0) {
            return false;
        }
    }
    unsigned int offset = mUsed;
    mUsed = offset + bytes;
    Free((char *)mBlock + offset, bytes, 0);
    return true;
}

bool FastMem::CreateBlock(unsigned int listIndex) {
    unsigned int bytes;
    unsigned int mask;

    if (!mInited) {
        Init();
    }
    bytes = (listIndex + 1) * 16;
    mask = bytes - 1;
    if ((bytes & mask) == 0) {
        if (listIndex != 0) {
            if (mUsed & 0x1F) {
                if (AssignToFree(16) == 0) {
                    return false;
                }
            }
        }
        if (listIndex > 1) {
            if (mUsed & 0x3F) {
                unsigned int num2xBlocks = 2;
                do {
                    if (AssignToFree(32) == 0) {
                        return false;
                    }
                    if ((mUsed & 0x3F) == 0) {
                        break;
                    }
                } while (num2xBlocks-- > 0);
            }
        }
        if (listIndex > 3) {
            mask &= 0x7F;
            if (mUsed & mask) {
                do {
                    if (AssignToFree(64) == 0) {
                        return false;
                    }
                } while (mUsed & mask);
            }
        }
    }
    return AssignToFree(bytes);
}

// STRIPPED
void FastMem::DumpRecord() {}
