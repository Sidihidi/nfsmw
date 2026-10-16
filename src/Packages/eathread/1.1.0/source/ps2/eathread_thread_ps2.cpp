// eathread 1.1.0 — backend PlayStation 2 del hilo (SLES-53558-A124/SLUS-21351).
// El original solo trae las tablas estaticas del bss (0x80 B de 32 int + 0x480 B
// de 32 x 0x24 B) y su static-init; AllocateThreadDynamicData no llega al ELF.
// Sin el include de GameCube (eathread_thread.h tira de dolphin/os/OSThread.h):
// el tamano del dato dinamico de PS2 (0x24) sale del bss del original.

#include <stddef.h>

static const unsigned int kMaxThreadDynamicDataCount = 32;

// total size: 0x24 (bss 0x480 / 32)
struct EAThreadDynamicDataPs2 {
    int mStatus;          // +0x00
    void *mpStack;        // +0x04
    int mnRefCount;       // +0x08
    char mPad[0x24 - 0xC];

    EAThreadDynamicDataPs2() {}
};

namespace EA {
namespace Thread {

static EAThreadDynamicDataPs2 gThreadDynamicData[kMaxThreadDynamicDataCount];
static int gThreadDynamicDataAllocated[kMaxThreadDynamicDataCount];

}
}
