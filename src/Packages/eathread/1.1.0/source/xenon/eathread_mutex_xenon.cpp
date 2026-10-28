// Xbox 360: EA::Thread::Mutex sobre una seccion critica. EGM 0x82656070-0x82656370 (ver la cabecera).
#include "eathread/eathread_mutex.h"

#include <new>
#include <string.h>

// xboxkrnl (thunks de importacion del EGM: RtlEnterCriticalSection 0x828F4F04, RtlLeaveCriticalSection 0x828F4F14,
// RtlInitializeCriticalSection 0x828F4F24, RtlTryEnterCriticalSection 0x828F5BD4) y xapilib (Sleep, 0x8266ACD0). Las
// EnterCriticalSection... de las cabeceras del XDK de Xenon son estas mismas funciones del kernel.
extern "C" {
void RtlInitializeCriticalSection(void *lpCriticalSection);
void RtlEnterCriticalSection(void *lpCriticalSection);
int RtlTryEnterCriticalSection(void *lpCriticalSection);
void RtlLeaveCriticalSection(void *lpCriticalSection);
void Sleep(unsigned long dwMilliseconds);
}

static const EA::Thread::ThreadTime kTimeoutNone = 0xFFFFFFFF;

EAMutexData::EAMutexData() : mnLockCount(0), mbIntraProcess(true), mThreadId(0) {
    memset(mData, 0, sizeof(mData));
}

namespace EA {
namespace Thread {

MutexParameters::MutexParameters(bool bIntraProcess, const char *pName) : mbIntraProcess(bIntraProcess) {
    if (pName) {
        strncpy(mName, pName, sizeof(mName) - 1);
    } else {
        mName[0] = 0;
    }
}

Mutex::Mutex(const MutexParameters *pMutexParameters, bool bDefaultParameters) {
    if (!pMutexParameters && bDefaultParameters) {
        MutexParameters parameters;
        Init(&parameters);
    } else {
        Init(pMutexParameters);
    }
}

// La seccion critica de Xenon no se libera: el EGM deja el destructor vacio (un blr).
Mutex::~Mutex() {}

bool Mutex::Init(const MutexParameters *pMutexParameters) {
    if (pMutexParameters) {
        mMutexData.mnLockCount = 0;
        RtlInitializeCriticalSection(mMutexData.mData);
        return true;
    }
    return false;
}

int Mutex::Lock(const ThreadTime &timeoutAbsolute) {
    if (timeoutAbsolute == kTimeoutNone) {
        RtlEnterCriticalSection(mMutexData.mData);
    } else {
        while (!RtlTryEnterCriticalSection(mMutexData.mData)) {
            Sleep(1);
            if (GetThreadTime() >= timeoutAbsolute) {
                return kResultTimeout;
            }
        }
    }
    return ++mMutexData.mnLockCount;
}

int Mutex::Unlock() {
    const int nReturnValue = --mMutexData.mnLockCount;
    RtlLeaveCriticalSection(mMutexData.mData);
    return nReturnValue;
}

int Mutex::GetLockCount() const {
    return mMutexData.mnLockCount;
}

Mutex *MutexFactory::CreateMutex() {
    return new Mutex;
}

void MutexFactory::DestroyMutex(Mutex *pMutex) {
    delete pMutex;
}

unsigned int MutexFactory::GetMutexSize() {
    return sizeof(Mutex);
}

Mutex *MutexFactory::ConstructMutex(void *pMemory) {
    return new (pMemory) Mutex;
}

void MutexFactory::DestructMutex(Mutex *pMutex) {
    pMutex->~Mutex();
}

} // namespace Thread
} // namespace EA
