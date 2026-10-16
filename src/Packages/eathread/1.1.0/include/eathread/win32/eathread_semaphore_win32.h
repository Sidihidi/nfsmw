#ifndef EATHREAD_EATHREAD_SEMAPHORE_H
#define EATHREAD_EATHREAD_SEMAPHORE_H

// PC (Win32): el semaforo es uno del kernel (CreateSemaphoreA) mas la cuenta en un entero atomico. La disposicion y
// el codigo salen de speed.exe (0x834D82-0x834FE9): EASemaphoreData de 8 bytes (manejador y cuenta),
// SemaphoreParameters de 0x18 (el nombre empieza en +5) y la fabrica de semaforos de EAThread.

#include "eathread/eathread_atomic.h"

// kernel32. El exe la llama por su thunk (`call` a un `jmp [__imp_GetTickCount]`): la declaracion de EAThread no
// lleva dllimport.
extern "C" unsigned long __stdcall GetTickCount();

struct PathSemaphore;

// total size: 0x8
struct EASemaphoreData {
    void *mhSemaphore;      // offset 0x0, size 0x4
    AtomicInt<int> mnCount; // offset 0x4, size 0x4

    EASemaphoreData();
};

namespace EA {
namespace Thread {

// total size: 0x18
struct SemaphoreParameters {
    SemaphoreParameters(int initialCount = 0, bool bIntraProcess = true, const char *pName = 0);

    int mInitialCount;   // offset 0x0, size 0x4
    bool mbIntraProcess; // offset 0x4, size 0x1
    char mName[16];      // offset 0x5, size 0x10
};

typedef unsigned int ThreadTime;

// total size: 0x8
class Semaphore {
  public:
    enum Result {
        kResultError = -1,
        kResultTimeout = -2,
    };

    Semaphore(const SemaphoreParameters *pSemaphoreParameters = 0, bool bDefaultParameters = true);

    ~Semaphore();

    bool Init(const SemaphoreParameters *pSemaphoreParameters);

    int Wait(const ThreadTime &timeoutAbsolute);

    int Post(int count);

    int GetCount() const;

  protected:
    EASemaphoreData mSemaphoreData; // offset 0x0, size 0x8

  private:
    friend struct ::PathSemaphore;
    Semaphore(int initialCount);
};

// La fabrica de semaforos de EAThread: crear con new, construir en memoria propia, destruir y tamano.
class SemaphoreFactory {
  public:
    static Semaphore *CreateSemaphore();
    static void DestroySemaphore(Semaphore *pSemaphore);
    static unsigned int GetSemaphoreSize();
    static Semaphore *ConstructSemaphore(void *pMemory);
    static void DestructSemaphore(Semaphore *pSemaphore);
};

void ThreadSleep(const ThreadTime &timeout);
inline ThreadTime GetThreadTime() {
    return GetTickCount();
}

} // namespace Thread
} // namespace EA

#endif
