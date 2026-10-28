#ifdef EA_PLATFORM_GAMECUBE
#include "eathread/powerpc/eathread_atomic_powerpc.h"
#elif defined(EA_PLATFORM_WIN32)
#include "eathread/x86/eathread_atomic_x86.h"
#elif defined(EA_PLATFORM_XENON)
#include "eathread/xenon/eathread_atomic_xenon.h"
#endif
