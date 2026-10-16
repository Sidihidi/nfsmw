#ifdef EA_PLATFORM_GAMECUBE
#include "eathread/powerpc/eathread_atomic_powerpc.h"
#elif defined(EA_PLATFORM_WIN32)
#include "eathread/x86/eathread_atomic_x86.h"
#endif
