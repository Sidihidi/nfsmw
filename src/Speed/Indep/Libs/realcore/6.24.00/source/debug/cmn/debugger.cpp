#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system.h"

#ifdef EA_PLATFORM_GAMECUBE
#include <dolphin/os.h>
#endif

void DEBUG_break() {
#ifdef EA_PLATFORM_GAMECUBE
    OSPanic("", 0, "");
#elif defined(EA_PLATFORM_PLAYSTATION2)
    // Asm de EA: las dos PS2 tienen `break 0, 6` (0x0000018D), una trampa con codigo que C no genera.
    asm("break 0, 6");
#endif
}
