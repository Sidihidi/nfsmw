// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// datos-gc 2026-10-13: la CU original parsea WorldConn (guardas _0.._11), MGeneric (_12) y
// MAudioReflection (_13) antes del estatico de DoTunnelBloom (_14, en ScreenEffects.cpp).
#include "Speed/Indep/Src/World/WorldConn.h"
#include "Speed/Indep/Src/Generated/Messages/MGeneric.h"
#include "Speed/Indep/Src/Generated/Messages/MAudioReflection.h"

#include "Speed/Indep/Src/World/Skids.cpp"

#include "Speed/Indep/Src/World/Clans.cpp"

#include "Speed/Indep/Src/World/Track.cpp"

#include "Speed/Indep/Src/World/TrackPositionMarker.cpp"

#include "Speed/Indep/Src/World/TrackInfo.cpp"

#include "Speed/Indep/Src/World/TrackPath.cpp"

#include "Speed/Indep/Src/World/TrackStreamer.cpp"

#include "Speed/Indep/Src/World/Scenery.cpp"

#include "Speed/Indep/Src/World/VisibleSection.cpp"

#include "Speed/Indep/Src/World/WeatherMan.cpp"

#include "Speed/Indep/Src/World/ScreenEffects.cpp"

#include "Speed/Indep/Src/World/EventManager.cpp"
