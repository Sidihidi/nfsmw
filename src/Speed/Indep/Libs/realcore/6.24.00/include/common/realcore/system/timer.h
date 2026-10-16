#ifndef REALCORE_COMMON_SYSTEM_TIMER_H
#define REALCORE_COMMON_SYSTEM_TIMER_H

// El temporizador de realcore (firmas de los stabs de la A124).
int TIMER_init(int hz);
void TIMER_restore();
void TIMER_setvideomode(int videomode);
int TIMER_getfrequency();
int TIMER_gettick();

#endif
