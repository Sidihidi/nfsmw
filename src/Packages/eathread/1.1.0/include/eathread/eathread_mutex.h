// EA::Thread::Mutex. Solo Xbox 360 lo usa en este arbol: el path de Xenon guarda su cerrojo en un PathMutex (EGM
// 0x825FA580, nombre del PDB del prototipo de ProStreet para Xbox 360) en lugar del PathSemaphore de GameCube y PC.
#ifdef EA_PLATFORM_XENON
#include "eathread/xenon/eathread_mutex_xenon.h"
#endif
