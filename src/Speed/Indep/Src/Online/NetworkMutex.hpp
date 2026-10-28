#ifndef ONLINE_NETWORKMUTEX_HPP
#define ONLINE_NETWORKMUTEX_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>

#include "Speed/Indep/Libs/Support/Utility/FastMem.h"

#if (defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)) && ONLINE_SUPPORT
#include "realcore/system.h"

// PC (speed.exe): un mutex de verdad. El ctor de ExtrapolatedCar (0x74D510) pide 0x1C bytes a gFastMem, llama a
// RealSystem::Mutex::Mutex y despues a RealSystem::Mutex::Create sobre el mismo objeto. Xbox 360 igual: el ctor de OnlineMutex del EGM (0x82573F18) llama a
// RealSystem::Mutex::Mutex (0x8284FDF8) y su Lock (0x82573F98) a RealSystem::Mutex::Lock.
class NetworkMutex : public RealSystem::Mutex {
#else
class NetworkMutex {
#endif
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
#if !(defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)) || !ONLINE_SUPPORT
    void Create() {}
    void Lock() {}
    void Unlock() {}
    void Destroy() {}
#endif
};

#endif
