#ifndef REALCORE_SOURCE_INPUT_PS2_INTERFACE_H
#define REALCORE_SOURCE_INPUT_PS2_INTERFACE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#pragma interface

#include "../cmn/interfaceimp.h"

namespace RealInput {

struct Ps2Effect;

// Los enganches opcionales del multitap y de los teclados y ratones USB (globales de ps2_interface.cpp en los stabs).
extern int (*gpInitMTap)();
extern int (*gpMTapPortOpen)(int);
extern int (*gpMTapPortClose)(int);
extern Device *(*gpCreatePs2KeyboardDevice)();
extern Device *(*gpCreatePs2MouseDevice)();

// La interfaz de entrada de PS2 (stabs de la A124: Ps2Interface mide 0xB8).
struct Ps2Interface : InterfaceImp {
    Ps2Interface(const ConfigOptions &options);
    virtual ~Ps2Interface();

    static inline void operator delete(void *ptr, unsigned int size) {
        FreeMemSize(ptr, static_cast<int>(size));
    }

    static inline void *operator new(unsigned int size) {
        return AllocateMemSize(nullptr, static_cast<int>(size), 0, 0, 0);
    }

    Ps2Effect *GetUnusedEffectSlot();

  private:
    void EnumeratePads();
    void EnumerateKeyboard();
    void EnumerateMouse();

    Ps2Effect *mpEffectTable; // offset 0xB4
};

} // namespace RealInput

#endif
