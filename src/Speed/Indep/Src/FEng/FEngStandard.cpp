#include "FEngStandard.h"

#include <string.h>
#ifdef EA_BUILD_A124
#include <stdarg.h>
#include <stdio.h>
#endif

#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

int FEngMemoryPoolNumber = -1;
void *pFEngMemoryPoolMemory = nullptr;
int FEngMemoryPoolSize = 390000;
static int FEngMemoryPoolUnknown = 9999999;
int FEngMemoryPoolTracingEnabled = 0;

void InitFEngMemoryPool() {
    if (FEngMemoryPoolNumber != 0) {
        FEngMemoryPoolNumber = bGetFreeMemoryPoolNum();
        if (pFEngMemoryPoolMemory == nullptr) {
            pFEngMemoryPoolMemory = bMalloc(FEngMemoryPoolSize, "FEngMemoryPool", 0, 0);
        }
        bInitMemoryPool(FEngMemoryPoolNumber, pFEngMemoryPoolMemory, FEngMemoryPoolSize, "FEngMemoryPool");
        bSetMemoryPoolDebugTracing(FEngMemoryPoolNumber, FEngMemoryPoolTracingEnabled != 0);
    }
}

void *FEngMalloc(unsigned int size, const char *pFilename, int Line) {
    int Pool = 0;

    if (FEngMemoryPoolNumber != -1) {
        if (bLargestMalloc(FEngMemoryPoolNumber) > (int) (size + 64)) {
            Pool = FEngMemoryPoolNumber;
        }
    }

// 0x400 tambien en PC (speed.exe 0x5B0E10 y 0x5B7BF0): solo GameCube usa 0x100.
#ifdef EA_PLATFORM_GAMECUBE
    return bMalloc(size, pFilename, Line, (Pool & 0xF) | 0x100);
#else
    return bMalloc(size, pFilename, Line, (Pool & 0xF) | 0x400);
#endif
}

#ifdef EA_BUILD_A124
int FEngPrintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    return vprintf(format, args);
}
#endif

void FEngMemCpy(void *pDest, const void *pSrc, int Size) {
    memcpy(pDest, pSrc, Size);
}

void FEngMemSet(void *pDest, int Value, int Size) {
    memset(pDest, Value, Size);
}

void FEngStrCpy(char *pDest, const char *pSrc) {
    strcpy(pDest, pSrc);
}

int FEngStrLen(const char *pStr) {
    return strlen(pStr);
}

int FEngStrICmp(const char *pStr1, const char *pStr2) {
    return bStrICmp(pStr1, pStr2);
}

float FEngAbs(float x) {
    return bAbs(x);
}

float FEngSqrt(float x) {
    return bSqrt(x);
}

float FEngSin(float Value) {
    return bSin(Value);
}

float FEngACos(float x) {
    return bAngToRad(bACos(x));
}
