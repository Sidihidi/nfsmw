// Xbox 360: las funciones de hilo de EAThread (el eathread_pc.cpp del PDB de ProStreet). EGM 0x82654D18-0x82655220:
// la tabla de manejadores de hilos dinamicos, el indice TLS con el manejador del hilo actual, GetThreadId, la
// prioridad y el procesador del hilo actual, ThreadSleep, ThreadEnd y la funcion de asercion.

namespace EA {
namespace Thread {

// El indice TLS donde cada hilo guarda su manejador (el EGM lo tiene en .data a 0xFFFFFFFF, 0x82942C80).
static DWORD dwThreadHandleTLS = TLS_OUT_OF_INDEXES;

// Los manejadores que GetThreadId duplica para los hilos que no creo EAThread: se cierran cuando el hilo termina.
struct DynamicThreadArray {
    DynamicThreadArray();
    ~DynamicThreadArray();
    void CheckDynamicThreadArray(bool bCloseAll);
    void AddDynamicThreadHandle(HANDLE hThread, bool bAdd);

    HANDLE mhDynamicThreadArray[16];   // offset 0x0, size 0x40
    CRITICAL_SECTION mCriticalSection; // offset 0x40, size 0x1C
};

static DynamicThreadArray sDynamicThreadArray;

DynamicThreadArray::DynamicThreadArray() {
    for (int i = 0; i < 16; i++) {
        mhDynamicThreadArray[i] = 0;
    }
    InitializeCriticalSection(&mCriticalSection);
}

DynamicThreadArray::~DynamicThreadArray() {
    CheckDynamicThreadArray(true);
    DeleteCriticalSection(&mCriticalSection);
}

void DynamicThreadArray::CheckDynamicThreadArray(bool bCloseAll) {
    EnterCriticalSection(&mCriticalSection);

    for (unsigned int i = 0; i < 16; i++) {
        if (mhDynamicThreadArray[i]) {
            DWORD dwExitCode = 0;

            if (bCloseAll || !GetExitCodeThread(mhDynamicThreadArray[i], &dwExitCode) || dwExitCode != STILL_ACTIVE) {
                CloseHandle(mhDynamicThreadArray[i]);
                mhDynamicThreadArray[i] = 0;
            }
        }
    }

    LeaveCriticalSection(&mCriticalSection);
}

void DynamicThreadArray::AddDynamicThreadHandle(HANDLE hThread, bool bAdd) {
    if (hThread) {
        EnterCriticalSection(&mCriticalSection);

        if (bAdd) {
            for (int i = 0; i < 16; i++) {
                if (mhDynamicThreadArray[i] == 0) {
                    mhDynamicThreadArray[i] = hThread;
                    hThread = 0;
                    break;
                }
            }
            // La tabla esta llena: el manejador no se puede seguir y se cierra ya.
            if (hThread) {
                CloseHandle(hThread);
            }
        } else {
            for (int i = 0; i < 16; i++) {
                if (mhDynamicThreadArray[i] == hThread) {
                    CloseHandle(hThread);
                    mhDynamicThreadArray[i] = 0;
                    break;
                }
            }
        }

        LeaveCriticalSection(&mCriticalSection);
    }
}

// Reserva el indice TLS al arrancar (inicializador dinamico de sTLSAlloc).
struct TLSAlloc {
    TLSAlloc() {
        if (dwThreadHandleTLS == TLS_OUT_OF_INDEXES) {
            dwThreadHandleTLS = TlsAlloc();
        }
    }
};

static TLSAlloc sTLSAlloc;

void SetCurrentThreadHandle(HANDLE hThread, bool bDynamic) {
    if (dwThreadHandleTLS == TLS_OUT_OF_INDEXES) {
        dwThreadHandleTLS = TlsAlloc();
    }

    if (dwThreadHandleTLS != TLS_OUT_OF_INDEXES) {
        if (bDynamic) {
            if (hThread) {
                sDynamicThreadArray.AddDynamicThreadHandle(hThread, true);
            } else {
                HANDLE hThreadOld = TlsGetValue(dwThreadHandleTLS);
                if (hThreadOld) {
                    sDynamicThreadArray.AddDynamicThreadHandle(hThreadOld, false);
                }
            }
        } else {
            sDynamicThreadArray.CheckDynamicThreadArray(false);
        }
        TlsSetValue(dwThreadHandleTLS, hThread);
    }
}

// Windows no da a un hilo su propio manejador: se duplica el pseudomanejador del hilo actual y se guarda en el TLS.
ThreadId GetThreadId() {
    if (dwThreadHandleTLS != TLS_OUT_OF_INDEXES) {
        HANDLE hThreadCurrent = TlsGetValue(dwThreadHandleTLS);
        if (hThreadCurrent) {
            return hThreadCurrent;
        }
    }

    HANDLE hThread;
    if (DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hThread, 0, true,
                        DUPLICATE_SAME_ACCESS)) {
        SetCurrentThreadHandle(hThread, true);
    }
    return hThread;
}

int GetThreadPriority() {
    return ::GetThreadPriority(GetCurrentThread());
}

bool SetThreadPriority(int nPriority) {
    return ::SetThreadPriority(GetCurrentThread(), nPriority) != 0;
}

// Los seis hilos de hardware de Xenon.
void SetThreadProcessor(int nProcessor) {
    if (nProcessor < 0) {
        nProcessor = 0;
    }
    XSetThreadProcessor(GetCurrentThread(), nProcessor % 6);
}

void ThreadSleep(const ThreadTime &timeRelative) {
    if (timeRelative == 0) {
        SwitchToThread();
    } else {
        SleepEx(timeRelative, true);
    }
}

void ThreadEnd(int threadReturnValue) {
    EAThreadDynamicData *const pTDD = FindThreadDynamicData(GetThreadId());
    if (pTDD) {
        pTDD->mnReturnValue = threadReturnValue;
        pTDD->Release();
    }
    SetCurrentThreadHandle(0, true);
    ExitThread(threadReturnValue);
}

ThreadTime GetThreadTime() {
    return GetTickCount() + 10000;
}

AssertionFailureFunction gpAssertionFailureFunction = 0;
void *gpAssertionFailureContext = 0;

void SetAssertionFailureFunction(AssertionFailureFunction pAssertionFailureFunction, void *pContext) {
    gpAssertionFailureFunction = pAssertionFailureFunction;
    gpAssertionFailureContext = pContext;
}

void AssertionFailure(const char *pExpression) {
    if (gpAssertionFailureFunction) {
        gpAssertionFailureFunction(pExpression, gpAssertionFailureContext);
    }
}

} // namespace Thread
} // namespace EA
