#include "./smixi.h"

#ifdef __cplusplus
extern "C" {
#endif

void MIXI_initunpack16() {
    sndmix.unpackinit[0] = SFILTER_unpackfinit;
#ifdef EA_PLATFORM_XENON
    // Xenon (EGM: MIX_create 0x82617818 y la copia suelta de 0x826381D8): los estados
    // de unpack no tienen `bits16`, 4 bytes menos — UNPACKFSTATE 0x2C -> 0x28 y
    // UNPACKLFSTATE 0x30 -> 0x2C —; UNPACKPFSTATE sigue en 0x34.
    sndmix.unpackstatesize[0] = 0x28;
#else
    sndmix.unpackstatesize[0] = 0x2C;
#endif
    sndmix.unpackinit[1] = SFILTER_unpacklfinit;
#ifdef EA_PLATFORM_XENON
    sndmix.unpackstatesize[1] = 0x2C;
#else
    sndmix.unpackstatesize[1] = 0x30;
#endif
    sndmix.unpackinit[2] = SFILTER_unpackpfinit;
    sndmix.unpackstatesize[2] = 0x34;
}

#ifdef __cplusplus
}
#endif
