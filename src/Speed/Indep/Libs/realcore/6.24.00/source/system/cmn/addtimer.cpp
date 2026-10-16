// realcore 6.24.00: las tareas del temporizador (source/system/cmn/addtimer.cpp en el .mdebug de la A124; la
// unidad de config/ se llama ps2/addtimer.cpp).
extern void (*tmrsub[8])();

void TIMER_addtask(void (*timerroutine)()) {
    int j;

    for (j = 0; j < 8; j++) {
        if (tmrsub[j] == timerroutine) {
            return;
        }
    }
    for (j = 0; j < 8; j++) {
        if (tmrsub[j] == 0) {
            tmrsub[j] = timerroutine;
            return;
        }
    }
}

void TIMER_removetask(void (*timerroutine)()) {
    int j;

    for (j = 0; j < 8; j++) {
        if (tmrsub[j] == timerroutine) {
            tmrsub[j] = 0;
            return;
        }
    }
}
