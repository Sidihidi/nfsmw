#ifndef ONLINE_INGAME_ONLINERACER_H
#define ONLINE_INGAME_ONLINERACER_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// La capa online esta anulada en la version de GameCube: el volcado DWARF da
// `total size: 0x1` para esta clase, sin un solo miembro, y sus accesores
// devuelven constantes.

#include <types.h>

#if ONLINE_SUPPORT
// El prototipo si tiene la capa online: su OnlineRacer es la clase de verdad (0x1430 B, Persona en 0x1318).
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#else
// total size: 0x1
struct OnlineRacer {
    char *GetPersona() {
        return NULL;
    }
};
#endif

#endif
