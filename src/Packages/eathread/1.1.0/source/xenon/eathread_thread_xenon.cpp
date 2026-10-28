// Xbox 360: EA::Thread::Thread sobre CreateThread de xapilib (el eathread_thread_pc.cpp del PDB de ProStreet).
// EGM 0x82655220-0x82655F08: los datos dinamicos de cada hilo (12 en una tabla estatica y el resto con new), los dos
// Begin y sus funciones de arranque, la espera, el estado, la prioridad, el procesador y el nombre.

namespace EA {
namespace Thread {

static const unsigned int kMaxThreadDynamicDataCount = 12;

char gThreadDynamicData[kMaxThreadDynamicDataCount][sizeof(EAThreadDynamicData)];
AtomicInt32 gThreadDynamicDataAllocated[kMaxThreadDynamicDataCount];

// El procesador de los hilos creados sin uno: los hilos de hardware 4 y 5, alternando.
static int nLastProcessor = 0;

EAThreadDynamicData *AllocateThreadDynamicData() {
    for (unsigned int i = 0; i < kMaxThreadDynamicDataCount; i++) {
        if (gThreadDynamicDataAllocated[i].SetValueConditional(1, 0)) {
            return reinterpret_cast<EAThreadDynamicData *>(gThreadDynamicData[i]);
        }
    }
    // Sin hueco en la tabla: del monton.
    return new EAThreadDynamicData;
}

void FreeThreadDynamicData(EAThreadDynamicData *pEAThreadDynamicData) {
    if (pEAThreadDynamicData >= reinterpret_cast<EAThreadDynamicData *>(gThreadDynamicData) &&
        pEAThreadDynamicData < reinterpret_cast<EAThreadDynamicData *>(gThreadDynamicData) + kMaxThreadDynamicDataCount) {
        gThreadDynamicDataAllocated[pEAThreadDynamicData - reinterpret_cast<EAThreadDynamicData *>(gThreadDynamicData)]
            .SetValue(0);
    } else {
        delete pEAThreadDynamicData;
    }
}

EAThreadDynamicData *FindThreadDynamicData(ThreadId threadId) {
    for (unsigned int i = 0; i < kMaxThreadDynamicDataCount; i++) {
        EAThreadDynamicData *const pTDD = reinterpret_cast<EAThreadDynamicData *>(gThreadDynamicData[i]);
        if (pTDD->mhThread == threadId) {
            return pTDD;
        }
    }
    return 0;
}

} // namespace Thread
} // namespace EA

EAThreadDynamicData::EAThreadDynamicData()
    : mhThread(EA::Thread::kThreadIdInvalid), //
      mnThreadId(0),                          //
      mnStatus(EA::Thread::Thread::kStatusNone), //
      mnReturnValue(0),                       //
      mpBeginThreadUserWrapper(0),            //
      mnRefCount(0) {}

EAThreadDynamicData::~EAThreadDynamicData() {
    if (mhThread) {
        CloseHandle(mhThread);
    }
}

void EAThreadDynamicData::AddRef() {
    mnRefCount.Increment();
}

void EAThreadDynamicData::Release() {
    if (mnRefCount.Decrement() == 0) {
        this->~EAThreadDynamicData();
        EA::Thread::FreeThreadDynamicData(this);
    }
}

namespace EA {
namespace Thread {

ThreadParameters::ThreadParameters()
    : mpStack(0), mnStackSize(0), mnPriority(kThreadPriorityDefault), mnProcessor(kProcessorDefault), mbSuspended(false),
      mpName("") {}

RunnableFunctionUserWrapper Thread::sGlobalRunnableFunctionUserWrapper = 0;
RunnableClassUserWrapper Thread::sGlobalRunnableClassUserWrapper = 0;

RunnableFunctionUserWrapper Thread::GetGlobalRunnableFunctionUserWrapper() {
    return sGlobalRunnableFunctionUserWrapper;
}

// Solo se puede fijar una vez.
void Thread::SetGlobalRunnableFunctionUserWrapper(RunnableFunctionUserWrapper pUserWrapper) {
    if (!sGlobalRunnableFunctionUserWrapper) {
        sGlobalRunnableFunctionUserWrapper = pUserWrapper;
    }
}

RunnableClassUserWrapper Thread::GetGlobalRunnableClassUserWrapper() {
    return sGlobalRunnableClassUserWrapper;
}

void Thread::SetGlobalRunnableClassUserWrapper(RunnableClassUserWrapper pUserWrapper) {
    if (!sGlobalRunnableClassUserWrapper) {
        sGlobalRunnableClassUserWrapper = pUserWrapper;
    }
}

Thread::Thread() {
    mThreadData.mpData = 0;
}

Thread::~Thread() {
    if (mThreadData.mpData) {
        mThreadData.mpData->Release();
    }
}

} // namespace Thread
} // namespace EA

static DWORD WINAPI RunnableFunctionInternal(void *pContext) {
    EAThreadDynamicData *const pTDD = static_cast<EAThreadDynamicData *>(pContext);
    EAReadWriteBarrier();
    EA::Thread::RunnableFunction pFunction = reinterpret_cast<EA::Thread::RunnableFunction>(pTDD->mpStartContext[0]);
    void *pCallContext = pTDD->mpStartContext[1];

    EA::Thread::SetCurrentThreadHandle(pTDD->mpStartContext[2], false);
    pTDD->mnStatus = EA::Thread::Thread::kStatusRunning;

    if (pTDD->mpBeginThreadUserWrapper) {
        EA::Thread::RunnableFunctionUserWrapper pWrapper =
            reinterpret_cast<EA::Thread::RunnableFunctionUserWrapper>(pTDD->mpBeginThreadUserWrapper);
        pTDD->mnReturnValue = pWrapper(pFunction, pCallContext);
    } else {
        pTDD->mnReturnValue = pFunction(pCallContext);
    }
    void *const pReturnValue = reinterpret_cast<void *>(pTDD->mnReturnValue);

    EA::Thread::SetCurrentThreadHandle(0, false);
    pTDD->mnStatus = EA::Thread::Thread::kStatusEnded;
    pTDD->Release();
    return reinterpret_cast<DWORD>(pReturnValue);
}

EA::Thread::ThreadId EA::Thread::Thread::Begin(RunnableFunction pFunction, void *pContext, const ThreadParameters *pTP,
                                               RunnableFunctionUserWrapper pUserWrapper) {
    if (mThreadData.mpData) {
        mThreadData.mpData->Release();
    }

    mThreadData.mpData = new (AllocateThreadDynamicData()) EAThreadDynamicData;
    mThreadData.mpData->AddRef(); // la de este objeto
    mThreadData.mpData->AddRef(); // la del hilo (la suelta RunnableFunctionInternal)
    mThreadData.mpData->mhThread = 0;
    mThreadData.mpData->mpStartContext[0] = pFunction;
    mThreadData.mpData->mpStartContext[1] = pContext;
    mThreadData.mpData->mpBeginThreadUserWrapper = pUserWrapper;

    const unsigned int nStackSize = pTP ? pTP->mnStackSize : 0;
    mThreadData.mpData->mhThread = CreateThread(0, nStackSize, RunnableFunctionInternal, mThreadData.mpData,
                                                CREATE_SUSPENDED, &mThreadData.mpData->mnThreadId);

    if (mThreadData.mpData->mhThread) {
        if (pTP) {
            SetName(pTP->mpName);
        }
        mThreadData.mpData->mpStartContext[2] = mThreadData.mpData->mhThread;

        if (pTP && pTP->mnPriority != kThreadPriorityDefault) {
            SetPriority(pTP->mnPriority);
        }

        int nProcessor;
        if (pTP && pTP->mnProcessor >= 0) {
            nProcessor = pTP->mnProcessor;
        } else {
            nProcessor = 4 + nLastProcessor;
            nLastProcessor ^= 1;
        }
        XSetThreadProcessor(mThreadData.mpData->mhThread, nProcessor % 6);

        ResumeThread(mThreadData.mpData->mhThread);
        return mThreadData.mpData->mhThread;
    }

    mThreadData.mpData->Release(); // la del hilo, que no llego a arrancar
    mThreadData.mpData->Release();
    mThreadData.mpData = 0;
    return kThreadIdInvalid;
}

static DWORD WINAPI RunnableObjectInternal(void *pContext) {
    EAThreadDynamicData *const pTDD = static_cast<EAThreadDynamicData *>(pContext);
    EAReadWriteBarrier();
    EA::Thread::IRunnable *pRunnable = static_cast<EA::Thread::IRunnable *>(pTDD->mpStartContext[0]);
    void *pCallContext = pTDD->mpStartContext[1];

    EA::Thread::SetCurrentThreadHandle(pTDD->mpStartContext[2], false);
    pTDD->mnStatus = EA::Thread::Thread::kStatusRunning;

    if (pTDD->mpBeginThreadUserWrapper) {
        EA::Thread::RunnableClassUserWrapper pWrapper =
            reinterpret_cast<EA::Thread::RunnableClassUserWrapper>(pTDD->mpBeginThreadUserWrapper);
        pTDD->mnReturnValue = pWrapper(pRunnable, pCallContext);
    } else {
        pTDD->mnReturnValue = pRunnable->Run(pCallContext);
    }
    void *const pReturnValue = reinterpret_cast<void *>(pTDD->mnReturnValue);

    EA::Thread::SetCurrentThreadHandle(0, false);
    pTDD->mnStatus = EA::Thread::Thread::kStatusEnded;
    pTDD->Release();
    return reinterpret_cast<DWORD>(pReturnValue);
}

EA::Thread::ThreadId EA::Thread::Thread::Begin(IRunnable *pRunnable, void *pContext, const ThreadParameters *pTP,
                                               RunnableClassUserWrapper pUserWrapper) {
    if (mThreadData.mpData) {
        mThreadData.mpData->Release();
    }

    mThreadData.mpData = new (AllocateThreadDynamicData()) EAThreadDynamicData;
    mThreadData.mpData->AddRef();
    mThreadData.mpData->AddRef();
    mThreadData.mpData->mhThread = 0;
    mThreadData.mpData->mpStartContext[0] = pRunnable;
    mThreadData.mpData->mpStartContext[1] = pContext;
    mThreadData.mpData->mpBeginThreadUserWrapper = pUserWrapper;

    const unsigned int nStackSize = pTP ? pTP->mnStackSize : 0;
    mThreadData.mpData->mhThread = CreateThread(0, nStackSize, RunnableObjectInternal, mThreadData.mpData,
                                                CREATE_SUSPENDED, &mThreadData.mpData->mnThreadId);

    if (mThreadData.mpData->mhThread) {
        if (pTP) {
            SetName(pTP->mpName);
        }
        mThreadData.mpData->mpStartContext[2] = mThreadData.mpData->mhThread;

        if (pTP && pTP->mnPriority != kThreadPriorityDefault) {
            SetPriority(pTP->mnPriority);
        }

        ResumeThread(mThreadData.mpData->mhThread);
        return mThreadData.mpData->mhThread;
    }

    mThreadData.mpData->Release();
    mThreadData.mpData->Release();
    mThreadData.mpData = 0;
    return kThreadIdInvalid;
}

static unsigned long RelativeTimeoutFromAbsoluteTimeout(const EA::Thread::ThreadTime &timeoutAbsolute) {
    if (timeoutAbsolute == 0xFFFFFFFF || timeoutAbsolute == 0) {
        return timeoutAbsolute;
    }
    const EA::Thread::ThreadTime timeCurrent = EA::Thread::GetThreadTime();
    if (timeoutAbsolute > timeCurrent) {
        return timeoutAbsolute - timeCurrent;
    }
    return 0;
}

namespace EA {
namespace Thread {

Thread::Status Thread::WaitForEnd(const ThreadTime &timeoutAbsolute, int *pThreadReturnValue) {
    EAReadWriteBarrier();

    // Begin puede no haber creado el hilo todavia: se espera a que lo tenga o a que venza el plazo.
    if (!mThreadData.mpData) {
        while (!mThreadData.mpData || !mThreadData.mpData->mhThread) {
            if (GetThreadTime() >= timeoutAbsolute) {
                break;
            }
            ThreadSleep(1);
            EAReadWriteBarrier();
        }
        if (!mThreadData.mpData) {
            return kStatusNone;
        }
        EAReadWriteBarrier();
    }

    if (mThreadData.mpData->mhThread) {
        const unsigned long result =
            WaitForSingleObject(mThreadData.mpData->mhThread, RelativeTimeoutFromAbsoluteTimeout(timeoutAbsolute));
        if (result == WAIT_TIMEOUT) {
            return kStatusRunning;
        }
        CloseHandle(mThreadData.mpData->mhThread);
        mThreadData.mpData->mhThread = 0;
        mThreadData.mpData->mnStatus = kStatusEnded;
    }
    if (pThreadReturnValue) {
        *pThreadReturnValue = mThreadData.mpData->mnReturnValue;
    }
    return kStatusEnded;
}

Thread::Status Thread::GetStatus(int *pThreadReturnValue) const {
    EAReadWriteBarrier();

    if (mThreadData.mpData) {
        if (mThreadData.mpData->mhThread) {
            DWORD dwExitStatus;
            if (GetExitCodeThread(mThreadData.mpData->mhThread, &dwExitStatus)) {
                if (dwExitStatus == STILL_ACTIVE) {
                    return kStatusRunning;
                }
                CloseHandle(mThreadData.mpData->mhThread);
                mThreadData.mpData->mhThread = 0;
            }
        }
        if (pThreadReturnValue) {
            *pThreadReturnValue = mThreadData.mpData->mnReturnValue;
        }
        mThreadData.mpData->mnStatus = kStatusEnded;
        return kStatusEnded;
    }
    return kStatusNone;
}

ThreadId Thread::GetId() const {
    if (mThreadData.mpData) {
        return mThreadData.mpData->mhThread;
    }
    return kThreadIdInvalid;
}

int Thread::GetPriority() const {
    if (mThreadData.mpData) {
        return ::GetThreadPriority(mThreadData.mpData->mhThread);
    }
    return kThreadPriorityUnknown;
}

bool Thread::SetPriority(int nPriority) {
    if (mThreadData.mpData) {
        return ::SetThreadPriority(mThreadData.mpData->mhThread, nPriority) != 0;
    }
    return false;
}

void Thread::SetProcessor(int nProcessor) {
    if (mThreadData.mpData) {
        XSetThreadProcessor(mThreadData.mpData->mhThread, nProcessor % 6);
    }
}

void Thread::Wake() {
    // Una APC vacia saca al hilo de un SleepEx o de una espera con aviso.
    struct ThreadWake {
        static void WINAPI Empty(ULONG_PTR) {}
    };

    if (mThreadData.mpData && mThreadData.mpData->mhThread) {
        QueueUserAPC(ThreadWake::Empty, mThreadData.mpData->mhThread, 0);
    }
}

const char *Thread::GetName() const {
    if (mThreadData.mpData) {
        return mThreadData.mpData->mName;
    }
    return "";
}

void Thread::SetName(const char *pName) {
    if (mThreadData.mpData && pName) {
        strncpy(mThreadData.mpData->mName, pName, sizeof(mThreadData.mpData->mName));
        mThreadData.mpData->mName[sizeof(mThreadData.mpData->mName) - 1] = 0;
    }
}

} // namespace Thread
} // namespace EA
