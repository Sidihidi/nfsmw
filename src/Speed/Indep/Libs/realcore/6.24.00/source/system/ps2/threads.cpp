// realcore 6.24.00, hilos de PS2 (source/system/ps2/threads.cpp en el .mdebug de la A124). Tipos, locales y
// globales de los stabs de la A124; el orden de las sentencias, de su mapa de lineas.
#include <eekernel.h>

#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/threads.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/timer.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/std.h"

typedef int (*THREADPROC)(void *);

extern int bInTimerSubroutine;
extern int hblanks_per_second;

struct THREAD {
    int reserved[2];
};

struct PS2THREAD {
    int signature; // offset 0x0
    int thId;      // offset 0x4
};

struct THREADSTARTUPPARAM {
    volatile int threadstarted; // offset 0x0
    THREADPROC entry;  // offset 0x4
    void *param;       // offset 0x8
};

static PS2THREAD s_Ps2MainThread;

static void library_thread_entry(void *param) {
    THREADSTARTUPPARAM p = *static_cast<THREADSTARTUPPARAM *>(param);

    static_cast<THREADSTARTUPPARAM *>(param)->threadstarted = 1;
    p.entry(p.param);
    ExitDeleteThread();
}

void THREAD_init() {
    s_Ps2MainThread.thId = GetThreadId();
    s_Ps2MainThread.signature = 'PS2T';
    THREAD_setpriority(reinterpret_cast<THREAD *>(&s_Ps2MainThread), 0);
}

bool THREAD_create(THREAD *hThread, THREADPROC entryproc, void *param, void *stack_addr, int stack_size,
                   int priority) {
    ThreadParam tp;
    int p;
    PS2THREAD ps2thread;
    THREADSTARTUPPARAM tsp;

    switch (priority) {
    case 3: p = 97; break;
    case 2: p = 98; break;
    case 1: p = 99; break;
    case 0: p = 100; break;
    case -1: p = 101; break;
    case -2: p = 102; break;
    case -3: p = 103; break;
    default: p = 100; break;
    }
    tp.entry = reinterpret_cast<void (*)(void *)>(library_thread_entry);
    tp.stack = stack_addr;
    tp.stackSize = stack_size;
    tp.initPriority = p;
    tp.gpReg = &_gp;
    ps2thread.thId = CreateThread(&tp);
    if (ps2thread.thId != 0) {
        ps2thread.signature = 'PS2T';
        if (hThread != 0) {
            MEM_copy(hThread, &ps2thread, sizeof(ps2thread));
        }
        tsp.threadstarted = 0;
        tsp.entry = entryproc;
        tsp.param = param;
        StartThread(ps2thread.thId, &tsp);
        while (!tsp.threadstarted) {
            THREAD_yield(1);
        }
        return true;
    }
    return false;
}

void THREAD_destroy(THREAD *thread) {
    int thId;
    ThreadParam tp;

    if (thread == reinterpret_cast<THREAD *>(0xFFFFFFFF)) {
        thId = GetThreadId();
    } else {
        thId = reinterpret_cast<PS2THREAD *>(thread)->thId;
    }
    if (bInTimerSubroutine) {
        iReferThreadStatus(thId, &tp);
    } else {
        ReferThreadStatus(thId, &tp);
    }
    if (tp.status & (THS_RUN | THS_READY | THS_WAIT | THS_SUSPEND)) {
        if (bInTimerSubroutine) {
            iTerminateThread(thId);
        } else {
            TerminateThread(thId);
        }
        DeleteThread(thId);
    } else if (tp.status & THS_DORMANT) {
        DeleteThread(thId);
    }
}

static void alarmhandler(int id, u_short time, void *arg) {
    iWakeupThread(reinterpret_cast<int>(arg));
}

void THREAD_yieldticks(int numticks) {
    THREAD_yield(numticks * 1000 / TIMER_getfrequency());
}

void THREAD_yield(int dur) {
    dur = dur * hblanks_per_second / 1000;
    if (dur < 2) {
        dur = 2;
    }
    SetAlarm(dur, alarmhandler, reinterpret_cast<void *>(GetThreadId()));
    SleepThread();
}

bool THREAD_iscurrent(THREAD *thread) {
    int thId = GetThreadId();

    if (thread == 0) {
        return thId == s_Ps2MainThread.thId;
    }
    if (thread == reinterpret_cast<THREAD *>(0xFFFFFFFF)) {
        return true;
    }
    return thId == reinterpret_cast<PS2THREAD *>(thread)->thId;
}

int THREAD_getpriority(THREAD *thread) {
    int thId;
    ThreadParam tp;

    if (thread == 0) {
        thId = s_Ps2MainThread.thId;
    } else if (thread == reinterpret_cast<THREAD *>(0xFFFFFFFF)) {
        thId = GetThreadId();
    } else {
        thId = reinterpret_cast<PS2THREAD *>(thread)->thId;
    }
    if (bInTimerSubroutine) {
        iReferThreadStatus(thId, &tp);
    } else {
        ReferThreadStatus(thId, &tp);
    }
    switch (tp.currentPriority) {
    case 97: return 3;
    case 98: return 2;
    case 99: return 1;
    case 100: return 0;
    case 101: return -1;
    case 102: return -2;
    case 103: return -3;
    }
    return tp.currentPriority;
}

bool THREAD_setpriority(THREAD *thread, int priority) {
    int thId;
    int p;

    if (thread == 0) {
        thId = s_Ps2MainThread.thId;
    } else if (thread == reinterpret_cast<THREAD *>(0xFFFFFFFF)) {
        thId = GetThreadId();
    } else {
        thId = reinterpret_cast<PS2THREAD *>(thread)->thId;
    }
    switch (priority) {
    case 3: p = 97; break;
    case 2: p = 98; break;
    case 1: p = 99; break;
    case 0: p = 100; break;
    case -1: p = 101; break;
    case -2: p = 102; break;
    case -3: p = 103; break;
    default: p = priority; break;
    }
    if (bInTimerSubroutine) {
        iChangeThreadPriority(thId, p);
    } else {
        ChangeThreadPriority(thId, p);
    }
    return true;
}

bool THREAD_waitexit(THREAD *thread, int ticks) {
    ThreadParam tp;
    int t = TIMER_gettick() + ticks;
    int tid = reinterpret_cast<PS2THREAD *>(thread)->thId;

    for (;;) {
        ReferThreadStatus(tid, &tp);
        if ((tp.status & THS_DORMANT) || tp.status == 0) {
            break;
        }
        THREAD_yieldticks(0);
        if (ticks != 0 && TIMER_gettick() > t) {
            break;
        }
    }
    return tp.status == THS_DORMANT;
}
