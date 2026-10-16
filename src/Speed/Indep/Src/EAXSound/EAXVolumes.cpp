#include "Speed/Indep/Src/EAXSound/EAXSoundEnums.hpp"

int RoadNoiseVolumes[MAX_NUM_FXROADNOISE_LOOP] = {
    15000, // FXROADNOISE_LOOP_GRAVEL00
    24000, // FXROADNOISE_LOOP_SIDEWALK
    32000, // FXROADNOISE_LOOP_COBBLESTONE00
    24000, // FXROADNOISE_LOOP_DEEPWATER
    24000, // FXROADNOISE_LOOP_WETROAD
    12000, // FXROADNOISE_LOOP_ASHPHALT00
    19000, // FXROADNOISE_LOOP_ASHPHALT01
    32500, // FXROADNOISE_LOOP_METAL
    32500, // FXROADNOISE_LOOP_STITCH_LOOP
};

// PS2 mezcla las transiciones con otros volumenes (las constantes de la funcion en SLUS y en la A124).
int GetRoadNoiseTransitionVol(FXROADNOISE_TRANSITION ID) {
    switch (ID) {
        case FXROADNOISE_TRANSITION_CURBON:
#ifdef EA_PLATFORM_PLAYSTATION2
            return 18000;
#elif defined(EA_PLATFORM_WIN32) // speed.exe tambien tiene volumenes propios (mov eax, 0x2EE0 / 0x5FB4)
            return 12000;
#else
            return 15500;
#endif
        case FXROADNOISE_TRANSITION_CURBOFF:
#ifdef EA_PLATFORM_PLAYSTATION2
            return 16000;
#elif defined(EA_PLATFORM_WIN32)
            return 12000;
#else
            return 15500;
#endif
        case FXROADNOISE_TRANSITION_SPIKESTRIP:
#ifdef EA_PLATFORM_PLAYSTATION2
            return 32000;
#elif defined(EA_PLATFORM_WIN32)
            return 24500;
#else
            return 26000;
#endif
        case FXROADNOISE_TRANSITION_BLOWN:
            return 32767;
    }
#ifdef EA_PLATFORM_PLAYSTATION2
    return 24000;
#else
    return 32767;
#endif
}
