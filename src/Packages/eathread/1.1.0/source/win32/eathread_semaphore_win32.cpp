// PC (Win32): EA::Thread::Semaphore sobre un semaforo del kernel. speed.exe 0x834D82-0x834FE9.
#include "eathread/eathread_semaphore.h"

#include <new>
#include <string.h>

// kernel32: el exe llama a CreateSemaphoreA, WaitForSingleObject, ReleaseSemaphore y CloseHandle por la tabla de
// importaciones.
extern "C" {
__declspec(dllimport) void *__stdcall CreateSemaphoreA(void *lpSemaphoreAttributes, long lInitialCount, long lMaximumCount,
                                                       const char *lpName);
__declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void *hHandle, unsigned long dwMilliseconds);
__declspec(dllimport) int __stdcall ReleaseSemaphore(void *hSemaphore, long lReleaseCount, long *lpPreviousCount);
__declspec(dllimport) int __stdcall CloseHandle(void *hObject);
}

EASemaphoreData::EASemaphoreData() : mhSemaphore(0), mnCount(0) {}

namespace EA {
namespace Thread {

SemaphoreParameters::SemaphoreParameters(int initialCount, bool bIntraProcess, const char *pName)
    : mInitialCount(initialCount), mbIntraProcess(bIntraProcess) {
    if (pName) {
        strncpy(mName, pName, sizeof(mName) - 1);
    } else {
        mName[0] = 0;
    }
}

Semaphore::Semaphore(const SemaphoreParameters *pSemaphoreParameters, bool bDefaultParameters) {
    if (!pSemaphoreParameters && bDefaultParameters) {
        SemaphoreParameters parameters;
        Init(&parameters);
    } else {
        Init(pSemaphoreParameters);
    }
}

Semaphore::Semaphore(int initialCount) {
    SemaphoreParameters parameters(initialCount);
    Init(&parameters);
}

Semaphore::~Semaphore() {
    if (mSemaphoreData.mhSemaphore) {
        CloseHandle(mSemaphoreData.mhSemaphore);
    }
}

bool Semaphore::Init(const SemaphoreParameters *pSemaphoreParameters) {
    if (pSemaphoreParameters && !mSemaphoreData.mhSemaphore) {
        mSemaphoreData.mnCount.SetValue(pSemaphoreParameters->mInitialCount);
        if (mSemaphoreData.mnCount.GetValue() < 0) {
            mSemaphoreData.mnCount.SetValue(0);
        }
        mSemaphoreData.mhSemaphore = CreateSemaphoreA(0, mSemaphoreData.mnCount.GetValue(), 0x3FFFFFFF,
                                                      pSemaphoreParameters->mName[0] ? pSemaphoreParameters->mName : 0);
        return mSemaphoreData.mhSemaphore != 0;
    }
    return false;
}

static unsigned long RelativeTimeoutFromAbsoluteTimeout(const ThreadTime &timeoutAbsolute) {
    if (timeoutAbsolute == 0xFFFFFFFF || timeoutAbsolute == 0) {
        return timeoutAbsolute;
    }
    const ThreadTime timeCurrent = GetThreadTime();
    if (timeoutAbsolute > timeCurrent) {
        return timeoutAbsolute - timeCurrent;
    }
    return 0;
}

int Semaphore::Wait(const ThreadTime &timeoutAbsolute) {
    const unsigned long result = WaitForSingleObject(mSemaphoreData.mhSemaphore, RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));

    if (result == 0) {
        mSemaphoreData.mnCount.Decrement();
        return mSemaphoreData.mnCount.GetValue();
    }
    return result == 0x102 ? kResultTimeout : kResultError;
}

int Semaphore::Post(int count) {
    if (count > 0) {
        mSemaphoreData.mnCount.Add(count);
        const int nNewCount = mSemaphoreData.mnCount.GetValue();

        if (ReleaseSemaphore(mSemaphoreData.mhSemaphore, count, 0)) {
            return nNewCount;
        }
        mSemaphoreData.mnCount.Add(-count);
        return kResultError;
    }
    return mSemaphoreData.mnCount.GetValue();
}

int Semaphore::GetCount() const {
    return mSemaphoreData.mnCount.GetValue();
}

Semaphore *SemaphoreFactory::CreateSemaphore() {
    return new Semaphore;
}

void SemaphoreFactory::DestroySemaphore(Semaphore *pSemaphore) {
    delete pSemaphore;
}

unsigned int SemaphoreFactory::GetSemaphoreSize() {
    return sizeof(Semaphore);
}

Semaphore *SemaphoreFactory::ConstructSemaphore(void *pMemory) {
    return new (pMemory) Semaphore;
}

void SemaphoreFactory::DestructSemaphore(Semaphore *pSemaphore) {
    pSemaphore->~Semaphore();
}

} // namespace Thread
} // namespace EA
