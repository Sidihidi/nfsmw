#ifndef REALCORE_SOURCE_INPUT_PS2_EFFECT_H
#define REALCORE_SOURCE_INPUT_PS2_EFFECT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#pragma interface

#include "../cmn/effectimp.h"

namespace RealInput {

// Vibracion del DualShock (stabs de la A124: Ps2Effect mide 0x34).
struct Ps2Effect : EffectImp {
    static inline void *operator new[](unsigned int size) {
        return AllocateMemSize(nullptr, static_cast<int>(size), 0, 4, 0);
    }

    static inline void operator delete[](void *ptr, unsigned int size) {
        FreeMemSize(ptr, static_cast<int>(size));
    }

    static inline void operator delete(void *ptr, unsigned int size) {
        FreeMemSize(ptr, static_cast<int>(size));
    }

    Ps2Effect();
    virtual ~Ps2Effect();

    virtual void Init(Device *pDevice, Info *pInfo);
    virtual void Release();
    virtual void Start();
    virtual void Stop();
    virtual Status GetStatus();
    virtual void SetInfo(Info *pInfo);

  private:
    bool mPlaying;               // offset 0x18
    uint32_t mMagLeft;           // offset 0x1C
    uint32_t mMagRight;          // offset 0x20
    int mPort;                   // offset 0x24
    int mSlot;                   // offset 0x28
    unsigned char mAct_Val[6];   // offset 0x2C
};

} // namespace RealInput

#endif
