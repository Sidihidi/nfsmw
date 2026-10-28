#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndenum.h"

struct GlobalFxProcessorData {
    int fxBusTag; // offset 0x0, size 0x4
    int busId; // offset 0x4, size 0x4
    float fxBusOutputLevel[6]; // offset 0x8, size 0x18
#ifndef EA_PLATFORM_XENON
    // Xenon no tiene `device`: el EGM pide 0x28 B (`li r4, 0x28` en
    // 0x82615714, fn_826156B8), limpia 0xa palabras (0x826155CC y
    // 0x82615768) y pone isReset a 1 en 0x20 (0x826157C4), sin tienda a
    // 0x24 entre los niveles y el isReset. En GC el campo si esta
    // (`stw r29, 0x20(r31)` en 0x80360660, `li r5, 0x2c` en 0x80360598).
    Snd::Device device; // offset 0x20, size 0x4
#endif
    int isReset; // offset 0x24, size 0x4 (Xenon: 0x20)
    unsigned int instanceHandle; // offset 0x28, size 0x4 (Xenon: 0x24)
};
