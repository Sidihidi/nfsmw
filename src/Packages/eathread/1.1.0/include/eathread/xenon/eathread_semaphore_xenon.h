#ifndef EATHREAD_EATHREAD_SEMAPHORE_H
#define EATHREAD_EATHREAD_SEMAPHORE_H

// Xbox 360: el semaforo es uno del kernel (CreateSemaphoreA de xapilib) con la cuenta y las esperas canceladas en
// enteros que se tocan con las intrinsecas Interlocked. La disposicion y el codigo salen del EGM (NfsMWEuropeGer
// Milestone, 0x82669258-0x8266976C): EASemaphoreData de 0x10 bytes (manejador, cuenta, canceladas y si el semaforo es
// de un solo proceso), SemaphoreParameters de 0x18 como en PC (el nombre empieza en +5).

struct PathSemaphore;

// xapilib (el EGM: GetTickCount 0x8266B978, que lee KeTimeStampBundle)
extern "C" unsigned long GetTickCount();

// total size: 0x10
struct EASemaphoreData {
    void *mhSemaphore;    // offset 0x0, size 0x4
    long mnCount;         // offset 0x4, size 0x4
    long mnCancelCount;   // offset 0x8, size 0x4
    bool mbIntraProcess;  // offset 0xC, size 0x1

    EASemaphoreData();
    void UpdateCancelCount(int n);
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

// total size: 0x10
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
    EASemaphoreData mSemaphoreData; // offset 0x0, size 0x10

  private:
    friend struct ::PathSemaphore;
    Semaphore(int initialCount);
};

void ThreadSleep(const ThreadTime &timeout);
// GetTickCount() + 10000: el EGM la tiene fuera de linea en 0x826551B8, con el codigo de los hilos
// (source/xenon/eathread_xenon.cpp), y LTCG la expande en Semaphore::Wait, Mutex::Lock y Thread::WaitForEnd.
ThreadTime GetThreadTime();

} // namespace Thread
} // namespace EA

#endif
