// Xbox 360: EA::Thread::Semaphore sobre un semaforo del kernel. EGM 0x82669258-0x8266976C.
#include "eathread/eathread_semaphore.h"

#include <string.h>

// xapilib: el EGM llama a CreateSemaphoreA (0x828E8DE0), WaitForSingleObject (0x8266D048), ReleaseSemaphore
// (0x828E8EF8) y CloseHandle (0x8266B930); las Interlocked son intrinsecas del compilador (lwarx/stwcx.).
extern "C" {
void *CreateSemaphoreA(void *lpSemaphoreAttributes, long lInitialCount, long lMaximumCount, const char *lpName);
unsigned long WaitForSingleObject(void *hHandle, unsigned long dwMilliseconds);
int ReleaseSemaphore(void *hSemaphore, long lReleaseCount, long *lpPreviousCount);
int CloseHandle(void *hObject);
long _InterlockedDecrement(long volatile *lpAddend);
long _InterlockedExchange(long volatile *Target, long Value);
long _InterlockedExchangeAdd(long volatile *Addend, long Value);
long _InterlockedCompareExchange(long volatile *Destination, long Exchange, long Comperand);
}
#pragma intrinsic(_InterlockedDecrement, _InterlockedExchange, _InterlockedExchangeAdd, _InterlockedCompareExchange)

EASemaphoreData::EASemaphoreData() : mhSemaphore(0), mnCount(0), mnCancelCount(0), mbIntraProcess(true) {}

// Las esperas que vencieron siguen contadas en mnCount: se devuelven a la cuenta mientras sea negativa y lo que sobra
// queda en mnCancelCount para el siguiente Post.
void EASemaphoreData::UpdateCancelCount(int n) {
    long count = mnCount;

    while (count < 0) {
        long newCount = count + n;
        if (newCount > 0) {
            newCount = 0;
        }
        const long oldCount = count;
        count = _InterlockedCompareExchange(&mnCount, newCount, oldCount);
        if (count == oldCount) {
            n += count - newCount;
            break;
        }
    }
    if (n > 0) {
        _InterlockedExchangeAdd(&mnCancelCount, n);
    }
}

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
        mSemaphoreData.mnCount = pSemaphoreParameters->mInitialCount;
        if (mSemaphoreData.mnCount < 0) {
            mSemaphoreData.mnCount = 0;
        }
        mSemaphoreData.mbIntraProcess = pSemaphoreParameters->mbIntraProcess;
        // Un semaforo de un solo proceso empieza a cero en el kernel: la cuenta la lleva mnCount
        if (mSemaphoreData.mbIntraProcess) {
            mSemaphoreData.mhSemaphore = CreateSemaphoreA(0, 0, 0x3FFFFFFF, 0);
        } else {
            mSemaphoreData.mhSemaphore = CreateSemaphoreA(0, mSemaphoreData.mnCount, 0x3FFFFFFF,
                                                          pSemaphoreParameters->mName[0] ? pSemaphoreParameters->mName : 0);
        }
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
    const unsigned long timeoutRelative = RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute);

    if (mSemaphoreData.mbIntraProcess) {
        if (_InterlockedDecrement(&mSemaphoreData.mnCount) < 0) {
            if (WaitForSingleObject(mSemaphoreData.mhSemaphore, timeoutRelative) != 0) {
                mSemaphoreData.UpdateCancelCount(1);
                return kResultTimeout;
            }
        }
        const int count = mSemaphoreData.mnCount;
        return count > 0 ? count : 0;
    }

    const unsigned long result = WaitForSingleObject(mSemaphoreData.mhSemaphore, timeoutRelative);

    if (result == 0) {
        return _InterlockedDecrement(&mSemaphoreData.mnCount);
    }
    return result == 0x102 ? kResultTimeout : kResultError;
}

int Semaphore::Post(int count) {
    if (mSemaphoreData.mbIntraProcess) {
        if (mSemaphoreData.mnCancelCount > 0 && mSemaphoreData.mnCount < 0) {
            const int cancelCount = _InterlockedExchange(&mSemaphoreData.mnCancelCount, 0);
            if (cancelCount > 0) {
                mSemaphoreData.UpdateCancelCount(cancelCount);
            }
        }
        const int oldCount = _InterlockedExchangeAdd(&mSemaphoreData.mnCount, count);
        if (oldCount < 0) {
            ReleaseSemaphore(mSemaphoreData.mhSemaphore, -oldCount, 0);
        }
        const int newCount = mSemaphoreData.mnCount;
        return newCount > 0 ? newCount : 0;
    }

    _InterlockedExchangeAdd(&mSemaphoreData.mnCount, count);
    if (ReleaseSemaphore(mSemaphoreData.mhSemaphore, count, 0)) {
        return mSemaphoreData.mnCount;
    }
    _InterlockedExchangeAdd(&mSemaphoreData.mnCount, -count);
    return kResultError;
}

int Semaphore::GetCount() const {
    return mSemaphoreData.mnCount;
}

} // namespace Thread
} // namespace EA
