// volatile int ticks: el DWARF de GameCube (systemvars.cpp) y la A124, que no lo mete en la ranura del jr.
extern volatile int ticks;
// int en el DWARF de GameCube (systemvars.cpp); TIMER_getfrequency esta estripada alli (Range 0xFFFFFFFF).
extern int TIMERhz;

unsigned int TIMER_gettick() {
    return ticks;
}

int TIMER_getfrequency() {
    return TIMERhz;
}
