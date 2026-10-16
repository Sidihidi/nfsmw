// eathread 1.1.0 — backend PlayStation 2 del semaforo (SLES-53558-A124/SLUS-21351).
// El original del asm troceado: los contadores van protegidos con DI()/EI() del
// eekernel.h (el bucle di/sync.p/mfc0 $12 & 0x10000 es la macro DI tal cual), el
// semaforo del kernel via CreateSema/DeleteSema/WaitSema/SignalSema con
// SemaParam {maxCount=0x2710, initCount=0}.

#include "eathread/eathread_semaphore.h"
#include "eekernel.h"
#include <stddef.h>

EASemaphoreData::EASemaphoreData() : mSemaphore(-1), mnCount(0), mnWaitCount(0) {}

namespace EA {
namespace Thread {

SemaphoreParameters::SemaphoreParameters(int initialCount, bool, const char *) {
    this->mInitialCount = initialCount;
}

Semaphore::Semaphore(int initialCount) : mSemaphoreData() {
    SemaphoreParameters parameters(initialCount, true, NULL);
    this->Init(&parameters);
}

Semaphore::~Semaphore() {
    if (mSemaphoreData.mSemaphore >= 0)
        DeleteSema(mSemaphoreData.mSemaphore);
}

bool Semaphore::Init(const SemaphoreParameters *parameters) {
    SemaParam param;

    if (parameters != NULL) {
        if (mSemaphoreData.mSemaphore < 0) {
            mSemaphoreData.mnCount = parameters->mInitialCount;
            if (mSemaphoreData.mnCount < 0)
                mSemaphoreData.mnCount = 0;
            param.initCount = 0;
            param.maxCount = 0x2710;
            mSemaphoreData.mSemaphore = CreateSema(&param);
            return mSemaphoreData.mSemaphore >= 0;
        }
    }
    return false;
}

int Semaphore::Wait(const ThreadTime &timeoutAbsolute) {
    DI();
    if (timeoutAbsolute == (ThreadTime)-1) {
        while (mSemaphoreData.mnCount == 0) {
            mSemaphoreData.mnWaitCount = mSemaphoreData.mnWaitCount + 1;
            EI();
            WaitSema(mSemaphoreData.mSemaphore);
            DI();
        }
    } else if (timeoutAbsolute != 0) {
        while (mSemaphoreData.mnCount == 0 && GetThreadTime() < timeoutAbsolute) {
            EI();
            ThreadSleep(1);
            DI();
        }
    }
    if (mSemaphoreData.mnCount == 0) {
        EI();
        return -2;
    }
    mSemaphoreData.mnCount = mSemaphoreData.mnCount - 1;
    EI();
    return mSemaphoreData.mnCount;
}

int Semaphore::Post(int count) {
    if (count > 0) {
        DI();
        mSemaphoreData.mnCount += count;
        if (mSemaphoreData.mnWaitCount != 0) {
            if (mSemaphoreData.mnWaitCount < count)
                count = mSemaphoreData.mnWaitCount;
            mSemaphoreData.mnWaitCount -= count;
            EI();
            while (count-- > 0)
                SignalSema(mSemaphoreData.mSemaphore);
        } else {
            EI();
        }
    }
    return mSemaphoreData.mnCount;
}

}
}
