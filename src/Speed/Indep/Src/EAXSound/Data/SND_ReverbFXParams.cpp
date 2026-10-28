#include "Speed/Indep/Src/EAXSound/Data/SND_REVERBFXPARAMS.hpp"

// size: 0x120, Decl: 82
stREVERB_PARAMS g_REVERBFXMODULES[12] = {
#ifdef EA_PLATFORM_PLAYSTATION2
    // Las dos PS2 (la A124 y SLUS) llevan su propia mezcla: entradas 5 y 6 con el seco a -200 y 7, 9 y 10 a -900.
    stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),   stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),   stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),
    stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),   stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),   stREVERB_PARAMS(0, -200, 0, -200, 300, 120),
    stREVERB_PARAMS(0, -200, 0, -200, 120, 120),   stREVERB_PARAMS(-900, 0, -900, 0, 120, 120),   stREVERB_PARAMS(-1200, 0, -1200, 0, 120, 120),
    stREVERB_PARAMS(-900, 0, -900, 0, 120, 120),   stREVERB_PARAMS(-900, 0, -900, 0, 120, 120),   stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),
#else
    stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),   stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),   stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),
    stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),   stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),   stREVERB_PARAMS(-100, 0, -300, 0, 300, 120),
    stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),   stREVERB_PARAMS(-1200, 0, -1200, 0, 120, 120), stREVERB_PARAMS(-1200, 0, -1200, 0, 120, 120),
    stREVERB_PARAMS(-1200, 0, -1200, 0, 120, 120), stREVERB_PARAMS(-1200, 0, -1200, 0, 120, 120), stREVERB_PARAMS(-300, 0, -300, 0, 120, 120),
#endif
};

int SIZE_OF_g_REVERBFXMODULES = sizeof(g_REVERBFXMODULES); // size: 0x4, address: 0xFFFFFFFF, Decl: 353
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_GAMECUBE)
// gcdatos14: en la A124, tras SIZE_OF_g_REVERBFXMODULES (0x5e5dfc, PS2_globals.hpp:1387):
// apunta a la cadena vacia de su .rodata+0x80 (la reubicacion de su .data objetivo).
// gcdatos19: no es solo-A124: la palabra-puntero tras SIZE_OF_g_REVERBFXMODULES
// y delante de xafilterf esta en SLUS (zEAXSound2 .data +0xff0, a 0 sin
// resolver en el recon) y en GC (+0x634, con el addend 0x5c de su .rodata);
// el DWARF de GC lo declara const char*. PC/X360 sin definir, como hasta ahora.
const char *cstringname = "";
#endif
