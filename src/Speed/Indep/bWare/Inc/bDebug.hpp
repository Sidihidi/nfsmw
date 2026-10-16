#ifndef BWARE_BDEBUG_H
#define BWARE_BDEBUG_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>

#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system.h"

class bMutex {
    // total size: 0x20
  private:
    unsigned int MutexMemory[8]; // offset 0x0, size 0x20

  public:
    void Create();
    void Destroy();
    void Lock();
    void Unlock();
};

int bIsValidPointer(void *p, int size);
float bGetTickerDifference(unsigned int start_ticks);
int bGetFixTickerDifference(unsigned int start_ticks, unsigned int end_ticks);
float bGetTickerDifference(unsigned int start_ticks, unsigned int end_ticks);
#ifdef EA_PLATFORM_PLAYSTATION2
inline unsigned int bGetTicker() {
    unsigned int ticks;

    // volatile: el registro Count de COP0 cambia en cada lectura, asi que
    // GCC no puede ni borrar una lectura cuyo resultado no se usa ni fundir
    // dos. El original lo conserva: UnloadChunks lee el contador aunque
    // start_time no se use despues, y sin volatile el mfc0 desaparecia.
    asm volatile("mfc0 %0, $9" : "=r"(ticks));
    return ticks;
}
#else
unsigned int bGetTicker();
#endif
void bInitTicker(float min_wraparound_time);
void bBreak();
int bDisableInterrupts();
void bRestoreInterrupts(int previous_state);
void bSetUserPutStringFunction(bool (*function)(int, const char *));
// bware/src/bDebug.cpp: el nombre del ordenador del depurador (Codeine).
int bGetComputerName(char *computer_name, int max_chars);

extern int EnableReleasePrintf;

#endif
