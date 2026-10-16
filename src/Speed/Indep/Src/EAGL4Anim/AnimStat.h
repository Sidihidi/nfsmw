#ifndef EAGL4ANIM_ANIMSTAT_H
#define EAGL4ANIM_ANIMSTAT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>

#include "eagl4supportprintmessage.h"

namespace EAGL4Anim {

// total size: 0x14
struct AnimStat {
    AnimStat() {}

    ~AnimStat() {}

    // void *operator new(size_t size) {}

    // void *operator new(size_t size, const char *msg) {}

    // void operator delete(void *ptr, size_t size) {}

    // void *operator new[](size_t size) {}

    // void *operator new[](size_t size, const char *msg) {}

    // void operator delete[](void *ptr, size_t size) {}

    // void *operator new(size_t, void *ptr) {}

    int mNumEvals;     // offset 0x0, size 0x4
    int mNumDeltaSums; // offset 0x4, size 0x4
    int mNumFrames;    // offset 0x8, size 0x4
    int mNumFnCreates; // offset 0xC, size 0x4
    long mNumCycles;   // offset 0x10, size 0x4
};

class AnimBank;
class AnimMemoryMap;

// total size: 0x18
class BankStat {
  public:
    BankStat() {}

    // void *operator new(size_t size) {}

    // void *operator new(size_t size, const char *msg) {}

    // void operator delete(void *ptr, size_t size) {}

    // void *operator new[](size_t size) {}

    // void *operator new[](size_t size, const char *msg) {}

    // void operator delete[](void *ptr, size_t size) {}

    // void *operator new(size_t, void *ptr) {}

    // Sin uso en ningun binario (solo las llaman las funciones de AnimStat.cpp, estripadas en las tres versiones).
    inline void IncrementLoads(AnimBank *ab);

    inline void IncrementAnimNameLookUps();

    inline bool InMemory();

    inline void UnLoadBank();

    ~BankStat();

    void Init(AnimBank *ab, const char *name);

    bool IsTracking(AnimBank *ab, const char *name) const;

    AnimStat *GetAnimStat(AnimMemoryMap *a);

    void Dump() const;

    void Dump(PRINTCHANNEL channel) const;

    void Reset();

  private:
    char *mBankFileName;     // offset 0x0, size 0x4
    int mNumLoads;           // offset 0x4, size 0x4
    int mNumAnims;           // offset 0x8, size 0x4
    int mNumAnimNameLookUps; // offset 0xC, size 0x4
    AnimStat *mAnimStats;    // offset 0x10, size 0x4
    AnimBank *mBank;         // offset 0x14, size 0x4
};

// total size: 0x194
class AllStat {
  public:
    // void *operator new(size_t size) {}

    // void *operator new(size_t size, const char *msg) {}

    // void operator delete(void *ptr, size_t size) {}

    // void *operator new[](size_t size) {}

    // void *operator new[](size_t size, const char *msg) {}

    // void operator delete[](void *ptr, size_t size) {}

    // void *operator new(size_t, void *ptr) {}

    AllStat();

    ~AllStat();

    void TrackBank(AnimBank *ab, const char *name);

    void UnLoadBank(AnimBank *ab);

    BankStat *GetBankStat(AnimBank *ab, const char *name);

    AnimStat *GetAnimStat(AnimMemoryMap *a);

    void Dump(PRINTCHANNEL channel) const;

    void Dump() const;

    void Reset();

    static AllStat *mpStat; // size: 0x4

  private:
    int mNumBanks;             // offset 0x0, size 0x4
    BankStat *mBankStats[100]; // offset 0x4, size 0x190
};

}; // namespace EAGL4Anim

#endif
