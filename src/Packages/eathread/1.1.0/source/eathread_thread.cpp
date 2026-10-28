#ifdef EA_PLATFORM_GAMECUBE
#include "gc/eathread_thread_gc.cpp"
#else
#ifdef EA_PLATFORM_PLAYSTATION2
#include "ps2/eathread_thread_ps2.cpp"
#endif
#ifdef EA_PLATFORM_XENON
// Xbox 360: el backend de PC de EAThread sobre xapilib (EGM 0x82654D18-0x82656070, nombres del PDB de ProStreet).
#define NOMINMAX
#include <xtl.h>
#include <ppcintrinsics.h>
#include <new>
#include <string.h>

#include "eathread/eathread_thread.h"

// La barrera de memoria de EAThread en Xenon: el `sync` con que empiezan RunnableFunctionInternal, WaitForEnd y
// GetStatus en el EGM (la intrinseca del XDK).
#define EAReadWriteBarrier() __sync()

#include "xenon/eathread_xenon.cpp"
#include "xenon/eathread_thread_xenon.cpp"

namespace EA {
namespace Thread {

Thread *ThreadFactory::CreateThread() {
    return new Thread;
}

void ThreadFactory::DestroyThread(Thread *pThread) {
    delete pThread;
}

unsigned int ThreadFactory::GetThreadSize() {
    return sizeof(Thread);
}

Thread *ThreadFactory::ConstructThread(void *pMemory) {
    return new (pMemory) Thread;
}

void ThreadFactory::DestructThread(Thread *pThread) {
    pThread->~Thread();
}

} // namespace Thread
} // namespace EA
#endif
#endif
