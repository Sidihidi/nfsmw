#ifndef ONLINE_NETWORKMUTEX_HPP
#define ONLINE_NETWORKMUTEX_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>

#include "Speed/Indep/Libs/Support/Utility/FastMem.h"

class NetworkMutex {
  public:
    // NetworkMutex.hpp:12 de la A124: el new de ExtrapolatedCar va a gFastMem, y su delete tiene copia en zWorld.
    static void *operator new(size_t size) {
        return gFastMem.Alloc(size, nullptr);
    }
    static void operator delete(void *mem, size_t size) {
        if (mem) {
            gFastMem.Free(mem, size, nullptr);
        }
    }
    NetworkMutex() {}
    void Create() {}
    void Lock() {}
    void Unlock() {}
    void Destroy() {}
};

#endif
