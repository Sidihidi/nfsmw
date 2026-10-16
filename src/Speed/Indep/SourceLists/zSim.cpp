// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

#include "Speed/Indep/Src/Sim/Common/OBB.cpp"

#include "Speed/Indep/Src/Sim/Common/Simulation.cpp"

#include "Speed/Indep/Src/Sim/Common/Util.cpp"

#include "Speed/Indep/Src/Sim/Common/SimSurface.cpp"

#include "Speed/Indep/Src/Sim/Entities/LocalPlayer.cpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Sim/Entities/OnlineLocalPlayer.cpp"
#include "Speed/Indep/Src/Sim/Entities/OnlineRemotePlayer.cpp"
#endif

#include "Speed/Indep/Src/Sim/Common/SimActivity.cpp"

#include "Speed/Indep/Src/Sim/Common/SimObject.cpp"

#include "Speed/Indep/Src/Sim/Common/SimEntity.cpp"

#include "Speed/Indep/Src/Sim/Common/SimEffect.cpp"

#include "Speed/Indep/Src/Sim/Common/SimServer.cpp"

#include "Speed/Indep/Src/Sim/Common/SimTypes.cpp"

#include "Speed/Indep/Src/Sim/Common/SimModel.cpp"

#include "Speed/Indep/Src/Sim/Activities/QuickGame.cpp"

#include "Speed/Indep/Src/Sim/Activities/CareerGame.cpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Sim/Activities/OnlineGame.cpp"
#endif

#include "Speed/Indep/Src/Sim/Activities/NISActivity.cpp"

#include "Speed/Indep/Src/Sim/Activities/GameplayActivity.cpp"
