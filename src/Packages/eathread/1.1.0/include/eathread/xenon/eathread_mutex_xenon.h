#ifndef EATHREAD_EATHREAD_MUTEX_XENON_H
#define EATHREAD_EATHREAD_MUTEX_XENON_H

// Xbox 360: EA::Thread::Mutex sobre una seccion critica del kernel (RtlInitializeCriticalSection, Rtl(Try)Enter y
// RtlLeaveCriticalSection, importadas de xboxkrnl: ordinales 0x12E, 0x125/0x13x y 0x130 en los thunks del EGM).
// Codigo del EGM (NfsMWEuropeGerMilestone, 0x82656070-0x82656370). Los nombres de clases, miembros y funciones son los
// del PDB del prototipo de ProStreet para Xbox 360 (ProStreet08EuropeEngMilestone.pdb, la misma EAThread compilada
// con el backend de PC, eathread_mutex_pc.cpp); el tamano NO: alli EAMutexData mide 0x30 (mData de unsigned __int64[4],
// alineado a 8) y en MW 0x2C (MutexFactory::GetMutexSize devuelve 0x2C y CreateMutex pide 0x2C a bMalloc), asi que
// mData va en palabras de 4 bytes.

// ThreadTime y GetThreadTime de Xenon (GetTickCount() + 10000) estan en la cabecera del semaforo.
#include "eathread/eathread_semaphore.h"

// total size: 0x2C
struct EAMutexData {
    unsigned int mData[8]; // offset 0x0, size 0x20 (el RTL_CRITICAL_SECTION de Xenon, 0x1C)
    int mnLockCount;       // offset 0x20, size 0x4
    bool mbIntraProcess;   // offset 0x24, size 0x1
    void *mThreadId;       // offset 0x28, size 0x4

    EAMutexData();
};

namespace EA {
namespace Thread {

// total size: 0x11
struct MutexParameters {
    MutexParameters(bool bIntraProcess = true, const char *pName = 0);

    bool mbIntraProcess; // offset 0x0, size 0x1
    char mName[16];      // offset 0x1, size 0x10
};

// total size: 0x2C
class Mutex {
  public:
    enum Result {
        kResultError = -1,
        kResultTimeout = -2,
    };

    Mutex(const MutexParameters *pMutexParameters = 0, bool bDefaultParameters = true);

    ~Mutex();

    bool Init(const MutexParameters *pMutexParameters);

    int Lock(const ThreadTime &timeoutAbsolute);

    int Unlock();

    int GetLockCount() const;

  protected:
    EAMutexData mMutexData; // offset 0x0, size 0x2C
};

class MutexFactory {
  public:
    static Mutex *CreateMutex();
    static void DestroyMutex(Mutex *pMutex);
    static unsigned int GetMutexSize();
    static Mutex *ConstructMutex(void *pMemory);
    static void DestructMutex(Mutex *pMutex);
};

} // namespace Thread
} // namespace EA

#endif
