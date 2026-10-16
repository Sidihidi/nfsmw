#ifndef REALCORE_SOURCE_INPUT_PS2_DEVICE_H
#define REALCORE_SOURCE_INPUT_PS2_DEVICE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#pragma interface

#include "../cmn/deviceimp.h"

namespace RealInput {

struct Ps2Interface;

// Un dispositivo de PS2 en su puerto y ranura del multitap (stabs de la A124: Ps2Device mide 0x14C).
struct Ps2Device : DeviceImp {
    static inline void operator delete(void *ptr, unsigned int size) {
        FreeMemSize(ptr, static_cast<int>(size));
    }

    Ps2Device(Type type, Ps2Interface *pInterface);
    virtual ~Ps2Device();

    void GetPortSlotNum(int &port, int &slot);

  protected:
    void SetPortSlotNum(int port, int slot);

    Ps2Interface *mpInterface; // offset 0x140
    int mPortNum;              // offset 0x144
    int mSlotNum;              // offset 0x148
};

} // namespace RealInput

#endif
