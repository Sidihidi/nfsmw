#ifdef EA_PLATFORM_GAMECUBE
#include "eathread/powerpc/eathread_semaphore_powerpc.h"
#elif defined(EA_PLATFORM_PLAYSTATION2)
#include "eathread/ps2/eathread_semaphore_ps2.h"
#elif defined(EA_PLATFORM_WIN32)
#include "eathread/win32/eathread_semaphore_win32.h"
#endif
