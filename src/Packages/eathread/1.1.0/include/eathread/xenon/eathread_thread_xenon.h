#ifndef EATHREAD_XENON_EATHREAD_THREAD_XENON_H
#define EATHREAD_XENON_EATHREAD_THREAD_XENON_H

// Xbox 360: EA::Thread::Thread y las funciones de hilo de EAThread sobre los hilos de xapilib (CreateThread,
// XSetThreadProcessor...). Codigo del EGM (NfsMWEuropeGerMilestone, 0x82654D18-0x82656070). Los nombres de clases,
// miembros y funciones son los del PDB del prototipo de ProStreet para Xbox 360 (ProStreet08EuropeEngMilestone.pdb: la
// misma EAThread compilada con el backend de PC, eathread_pc.cpp, eathread_thread_pc.cpp y eathread_thread.cpp); los
// tamanos son los de MW: EAThreadDynamicData mide 0x38 (mName de 20; en ProStreet 0x44 con mName de 32), la tabla
// estatica tiene 12 entradas (AllocateThreadDynamicData) y la de manejadores de hilos dinamicos 16.

#include "eathread/eathread_atomic.h"
#include "eathread/eathread_semaphore.h"

namespace EA {
namespace Thread {

typedef void *ThreadId;
typedef int (*RunnableFunction)(void *pContext);

class IRunnable {
  public:
    virtual int Run(void *pContext) = 0;
};

typedef int (*RunnableFunctionUserWrapper)(RunnableFunction pFunction, void *pContext);
typedef int (*RunnableClassUserWrapper)(IRunnable *pRunnable, void *pContext);

static const ThreadId kThreadIdInvalid = 0;
static const int kThreadPriorityUnknown = (int)0x80000000;
static const int kThreadPriorityDefault = 0;
static const int kProcessorDefault = -1;

ThreadId GetThreadId();
int GetThreadPriority();
bool SetThreadPriority(int nPriority);
void SetThreadProcessor(int nProcessor);
void ThreadEnd(int threadReturnValue);

typedef void (*AssertionFailureFunction)(const char *pExpression, void *pContext);
void SetAssertionFailureFunction(AssertionFailureFunction pAssertionFailureFunction, void *pContext);
void AssertionFailure(const char *pExpression);

// total size: 0x18
struct ThreadParameters {
    ThreadParameters();

    void *mpStack;            // offset 0x0, size 0x4
    unsigned int mnStackSize; // offset 0x4, size 0x4
    int mnPriority;           // offset 0x8, size 0x4
    int mnProcessor;          // offset 0xC, size 0x4
    bool mbSuspended;         // offset 0x10, size 0x1
    const char *mpName;       // offset 0x14, size 0x4
};

} // namespace Thread
} // namespace EA

// total size: 0x38
struct EAThreadDynamicData {
    EAThreadDynamicData();
    ~EAThreadDynamicData();
    void AddRef();
    void Release();

    void *mhThread;            // offset 0x0, size 0x4
    unsigned long mnThreadId;  // offset 0x4, size 0x4
    int mnStatus;              // offset 0x8, size 0x4
    int mnReturnValue;         // offset 0xC, size 0x4
    void *mpStartContext[3];   // offset 0x10, size 0xC
    void *mpBeginThreadUserWrapper; // offset 0x1C, size 0x4
    AtomicInt32 mnRefCount;    // offset 0x20, size 0x4
    char mName[20];            // offset 0x24, size 0x14
};

// total size: 0x4
struct EAThreadData {
    EAThreadDynamicData *mpData; // offset 0x0, size 0x4
};

namespace EA {
namespace Thread {

EAThreadDynamicData *AllocateThreadDynamicData();
void FreeThreadDynamicData(EAThreadDynamicData *pEAThreadDynamicData);
EAThreadDynamicData *FindThreadDynamicData(ThreadId threadId);

// total size: 0x4
class Thread {
  public:
    enum Status {
        kStatusNone,
        kStatusRunning,
        kStatusEnded,
    };

    Thread();
    ~Thread();

    static RunnableFunctionUserWrapper GetGlobalRunnableFunctionUserWrapper();
    static void SetGlobalRunnableFunctionUserWrapper(RunnableFunctionUserWrapper pUserWrapper);
    static RunnableClassUserWrapper GetGlobalRunnableClassUserWrapper();
    static void SetGlobalRunnableClassUserWrapper(RunnableClassUserWrapper pUserWrapper);

    ThreadId Begin(RunnableFunction pFunction, void *pContext = 0, const ThreadParameters *pTP = 0,
                   RunnableFunctionUserWrapper pUserWrapper = 0);
    ThreadId Begin(IRunnable *pRunnable, void *pContext = 0, const ThreadParameters *pTP = 0,
                   RunnableClassUserWrapper pUserWrapper = 0);

    Status WaitForEnd(const ThreadTime &timeoutAbsolute, int *pThreadReturnValue = 0);
    Status GetStatus(int *pThreadReturnValue = 0) const;

    ThreadId GetId() const;
    int GetPriority() const;
    bool SetPriority(int nPriority);
    void SetProcessor(int nProcessor);
    void Wake();
    const char *GetName() const;
    void SetName(const char *pName);

  protected:
    static RunnableFunctionUserWrapper sGlobalRunnableFunctionUserWrapper;
    static RunnableClassUserWrapper sGlobalRunnableClassUserWrapper;

    EAThreadData mThreadData; // offset 0x0, size 0x4
};

class ThreadFactory {
  public:
    static Thread *CreateThread();
    static void DestroyThread(Thread *pThread);
    static unsigned int GetThreadSize();
    static Thread *ConstructThread(void *pMemory);
    static void DestructThread(Thread *pThread);
};

} // namespace Thread
} // namespace EA

#endif
