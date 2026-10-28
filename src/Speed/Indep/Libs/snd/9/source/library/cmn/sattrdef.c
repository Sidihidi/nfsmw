#include "snd/sndo.h"
#include <cstddef>

int SND_attrsetdef(SNDSAMPLEATTR *pssa) {
    pssa->priority = 0;
    pssa->detune = 0;
    pssa->vol = 0x7F;
    pssa->pan = 0x40;
    pssa->fxlevel0 = 0;
#ifndef EA_PLATFORM_XENON
    pssa->bendrange = 0;
#endif
#ifdef EA_PLATFORM_PLAYSTATION2
    pssa->platformver = 0;
    pssa->rendermode = 8;
#elif defined(EA_PLATFORM_WIN32)
    pssa->platformver = 0; // speed.exe 0x81E2CB
    pssa->rendermode = 0;
#elif defined(EA_PLATFORM_XENON)
    pssa->platformver = 0; // EGM 0x82614850: stb 0 en el byte 0x6 (ni bendrange ni rendermode en su struct)
#else
    pssa->platformver = 2;
    pssa->rendermode = 0x200;
#endif

    int i;
    for (i = 0; i < 4; i++) {
        pssa->puserdata[i] = NULL;
        pssa->userdatasize[i] = 0;
    }

    for (i = 0; i < 6; i++) {
        pssa->azimuth[i] = 0;
        pssa->ptsdata[i] = NULL;
    }

    return 0;
}
