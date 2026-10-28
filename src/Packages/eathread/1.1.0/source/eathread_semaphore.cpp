#ifdef EA_PLATFORM_GAMECUBE
#include "gc/eathread_semaphore_gc.cpp"
#else
#ifdef EA_PLATFORM_PLAYSTATION2
#include "ps2/eathread_semaphore_ps2.cpp"
#endif
#ifdef EA_PLATFORM_WIN32
#include "win32/eathread_semaphore_win32.cpp"
#endif
#ifdef EA_PLATFORM_XENON
#include "xenon/eathread_semaphore_xenon.cpp"
#endif
#endif
