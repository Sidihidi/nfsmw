// realcore 6.24.00, temporizador de PS2 sobre el contador 1 del EE (source/system/ps2/inittmr.cpp en el .mdebug de
// la A124). Locales, registros y globales de sus stabs; el orden, de su mapa de lineas.
#include <eekernel.h>
#include <eeregs.h>

#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/timer.h"

extern volatile int ticks;
extern volatile int libticks;
extern int TIMERhz;
extern int hblanks_per_second;
extern void (*tmrsub[8])();

void SYSTEM_addexit(void (*exitfunc)());

static int hTimerId;
static u_short iCountTicks;
int bInTimerSubroutine;

static int tmrint(int ca);

int TIMER_init(int hz) {
    int mode;
    int count;
    int stoptick;
    int c;

    if (hTimerId) {
        return TIMERhz;
    }
    if (hz == 0) {
        hz = 100;
    }
    if (hz < 3150) {
        if (hz < 196) {
            mode = 2;
            iCountTicks = (hz / 2 + 576000) / hz;
            TIMERhz = 576000 / iCountTicks;
        } else {
            mode = 1;
            iCountTicks = (hz / 2 + 9216000) / hz;
            TIMERhz = 9216000 / iCountTicks;
        }
    } else {
        mode = 0;
        iCountTicks = (hz / 2 + 147456000) / hz;
        TIMERhz = 147456000 / iCountTicks;
    }
    *T1_COUNT = 0;
    *T1_COMP = iCountTicks;
    *T1_MODE = mode | 0x180;
    hTimerId = AddIntcHandler(INTC_TIM1, tmrint, 0);
    EnableIntc(INTC_TIM1);
    if (hblanks_per_second == 0) {
        count = *T0_COUNT;
        mode = *T0_MODE;
        *T0_MODE = 0x83;
        stoptick = ticks + TIMERhz;
        while (ticks < stoptick) {
        }
        c = *T0_COUNT;
        *T0_COUNT = count;
        *T0_COUNT = mode; // asi en las dos PS2 (A124 0x5029E8): el modo se escribe sobre T0_COUNT, no T0_MODE
        c -= count;
        if (c < 0) {
            c += 0x10000;
        }
        if (c > 15700 && c < 15800) {
            TIMER_setvideomode(1);
        } else if (c > 15575 && c < 15675) {
            TIMER_setvideomode(2);
        } else if (c > 28800 && c < 32000) {
            TIMER_setvideomode(3);
        }
    }
    SYSTEM_addexit(TIMER_restore);
    return TIMERhz;
}

void TIMER_restore() {
    if (hTimerId) {
        *T1_MODE = 0;
        DisableIntc(INTC_TIM1);
        RemoveIntcHandler(INTC_TIM1, hTimerId);
        hTimerId = 0;
    }
}

static int tmrint(int ca) {
    int i;

    *T1_COMP = iCountTicks + *T1_COMP - *T1_COUNT;
    *T1_COUNT = 0;
    *T1_MODE |= 0x400;
    ticks++;
    libticks++;
    for (i = 0; i < 8; i++) {
        if (tmrsub[i]) {
            bInTimerSubroutine = 1;
            tmrsub[i]();
            bInTimerSubroutine = 0;
        }
    }
    ExitHandler();
    return 0;
}
