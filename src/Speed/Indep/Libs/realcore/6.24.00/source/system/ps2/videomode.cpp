// realcore 6.24.00: lineas horizontales por segundo segun el modo de video (source/system/ps2/videomode.cpp en el
// .mdebug de la A124).
enum TimerVideoModes {
    TIMER_VIDEOMODE_NTSC = 1,
    TIMER_VIDEOMODE_PAL = 2,
    TIMER_VIDEOMODE_DTV480P = 3,
};

extern int hblanks_per_second;

void TIMER_setvideomode(int videomode) {
    switch (videomode) {
    case TIMER_VIDEOMODE_NTSC:
        hblanks_per_second = 15750;
        break;
    case TIMER_VIDEOMODE_PAL:
        hblanks_per_second = 15625;
        break;
    case TIMER_VIDEOMODE_DTV480P:
        hblanks_per_second = 28800;
        break;
    }
}
