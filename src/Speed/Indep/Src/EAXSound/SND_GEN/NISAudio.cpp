#include "Speed/Indep/Src/EAXSound/SND_GEN/NISAudio.hpp"

namespace Csis {

InterfaceId NIS_Select_StartId = {"NIS_Select_Start", 0x0BE4, 0x67D1};
FunctionHandle gNIS_Select_StartHandle;

#ifdef EA_BUILD_A124
// la A124 declara NIS_Select_End detras de NIS_Select_Start (sus simbolos: 0x5e5f10/0x5e5f18)
InterfaceId NIS_Select_EndId = {"NIS_Select_End", 0x0BE4, 0x4888};
FunctionHandle gNIS_Select_EndHandle;

#endif
InterfaceId SoundFX_SelectId = {"SoundFX_Select", 0x0BE4, 0x2052};
FunctionHandle gSoundFX_SelectHandle;

InterfaceId NIS_Select_BlacklistId = {"NIS_Select_Blacklist", 0x0BE4, 0x3B39};
FunctionHandle gNIS_Select_BlacklistHandle;

#ifndef EA_BUILD_A124
InterfaceId NIS_Select_EndId = {"NIS_Select_End", 0x0BE4, 0x4888};
FunctionHandle gNIS_Select_EndHandle;
#endif

}; // namespace Csis

#ifdef EA_BUILD_A124
// gcdatos15: los globales de LiveLink (PS2_globals:1474-1477), en su hueco exacto de la
// A124 (0x5e5f40-0x5e5f4c = zEAXSound2 .data +0x1690..0x169c, tras las handles NIS de
// Csis y delante de gnTHCDupStateInstanceShift (0x5e5f58)), a 0 en su .data objetivo.
// Solo A124 (otrasELF); el tipo eLiveMessages, de los stabs (NFSLiveLink.hpp). El
// include va aqui, dentro de la guarda, para que las demas versiones no cambien ni de
// parseo.
#include "Speed/Indep/Src/EAXSound/Dynamic_Mixer/NFSLiveLink.hpp"
eLiveMessages InputMsgBuffer = eLiveMsg_Nothing;
int iTestDisplay = 0;
float fTestDisplay = 0.0f;
NFSLiveLink *g_NFSLiveLink = nullptr;
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
// gcdatos14: en las dos PS2, tras las handles NIS de Csis y los globales de LiveLink (A124
// 0x5e5f58, PS2_globals.hpp:1478; SLUS en el mismo sitio), con estos valores leidos de su
// .data objetivo (identicos en las dos).
int gnTHCDupStateInstanceShift[1][5] = {{0, 14, 13, 11, 0}};
#endif
