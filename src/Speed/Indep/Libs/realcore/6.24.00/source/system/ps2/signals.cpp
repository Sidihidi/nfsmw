// realcore 6.24.00, senales de PS2 sobre semaforos del kernel (source/system/ps2/signals.cpp en el .mdebug de la
// A124). Tipos y locales de sus stabs.
#include <eekernel.h>

extern int bInTimerSubroutine;

struct SIGNAL {
    int reserved[3];
};

struct PS2SIGNAL {
    int signature; // offset 0x0
    int alarmset;  // offset 0x4
    int s;         // offset 0x8
};

bool SIGNAL_create(SIGNAL *hSignal) {
    PS2SIGNAL *ps2sig = reinterpret_cast<PS2SIGNAL *>(hSignal);
    SemaParam sp;
    int s;

    sp.maxCount = 1;
    sp.initCount = 0;
    s = CreateSema(&sp);
    if (s != 0) {
        ps2sig->s = s;
        ps2sig->alarmset = 0;
        return true;
    }
    return false;
}

void SIGNAL_destroy(SIGNAL *hSignal) {
    DeleteSema(reinterpret_cast<PS2SIGNAL *>(hSignal)->s);
}

void SIGNAL_set(SIGNAL *hSignal) {
    SemaParam sp;
    PS2SIGNAL *ps2sig = reinterpret_cast<PS2SIGNAL *>(hSignal);

    if (bInTimerSubroutine) {
        iReferSemaStatus(ps2sig->s, &sp);
        if (sp.currentCount < sp.maxCount) {
            iSignalSema(ps2sig->s);
        }
    } else {
        ReferSemaStatus(ps2sig->s, &sp);
        if (sp.currentCount < sp.maxCount) {
            SignalSema(ps2sig->s);
        }
    }
}

void SIGNAL_wait(SIGNAL *hSignal) {
    WaitSema(reinterpret_cast<PS2SIGNAL *>(hSignal)->s);
}
