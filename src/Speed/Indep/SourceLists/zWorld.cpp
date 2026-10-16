// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// datos-gc 2026-10-13: en la CU original, entre los 12 hash de WorldConn y los 18 de
// RenderConn van seis _GetKind (guardas _12.._17) y VehicleParams (_18); sin ellos los
// paquetes *_Service de RenderConn caian 6 guardas antes que en el slice (_21/_22...).
// Solo GameCube (contador _.tmp_ del slice): en bloque al principio de la unity, en PC
// voltea un registro de VehicleFragmentConn::UpdateModel (100 -> 99,947 %, bisectado:
// ni los includes Conn de World.cpp ni RenderConn.h son la causa, solo este prefijo).
#ifdef EA_PLATFORM_GAMECUBE
#include "Speed/Indep/Src/Generated/Messages/MGeneric.h"
#include "Speed/Indep/Src/Generated/Messages/MAudioReflection.h"
#include "Speed/Indep/Src/Generated/Messages/MGamePlayMoment.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyEngineBlown.h"
#include "Speed/Indep/Src/Generated/Messages/MEnterRaceOverFlow.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyOnlineRaceOver.h"
#endif

#include "Speed/Indep/Src/World/World.cpp"

#include "Speed/Indep/Src/World/Player.cpp"

#include "Speed/Indep/Src/World/RaceParameters.cpp"

#include "Speed/Indep/Src/World/ExtrapolatedCar.cpp"

#include "Speed/Indep/Src/World/OnlineManager.cpp"

#include "Speed/Indep/Src/World/DebugCarRenderTest.cpp"

#include "Speed/Indep/Src/World/DebugVehicleSelection.cpp"

#include "Speed/Indep/Src/World/DebugWorld.cpp"

#include "Speed/Indep/Src/World/SmackableRender.cpp"

#include "Speed/Indep/Src/World/CarRender.cpp"

#include "Speed/Indep/Src/World/VehicleRenderConn.cpp"

#include "Speed/Indep/Src/World/CarRenderConn.cpp"

#include "Speed/Indep/Src/World/HeliRenderConn.cpp"

#include "Speed/Indep/Src/World/VehicleFragmentConn.cpp"

#ifndef EA_PLATFORM_WIN32 // PC: el cielo va en zPlatform (speed.exe 0x6C0AB0-0x6C0E7F, entre las funciones de EcstasyE)
#include "Speed/Indep/Src/World/SkyRender.cpp"
#endif

#include "Speed/Indep/Src/World/SpaceNode.cpp"

#include "Speed/Indep/Src/World/CarInfo.cpp"

#include "Speed/Indep/Src/World/CarPartNames.cpp"

#include "Speed/Indep/Src/World/CarLoader.cpp"

#include "Speed/Indep/Src/World/CarSkin.cpp"

#include "Speed/Indep/Src/World/WorldModel.cpp"

#include "Speed/Indep/Src/World/NeuQuant.cpp"

#include "Speed/Indep/Src/World/rain.cpp"

#include "Speed/Indep/Src/World/FacePixelate.cpp"

#include "Speed/Indep/Src/World/HeliSheet.cpp"

#include "Speed/Indep/Src/World/SimpleModelAnim.cpp"

#include "Speed/Indep/Src/World/ParameterMaps.cpp"

#include "Speed/Indep/Src/World/VisualTreatment.cpp"

#include "Speed/Indep/Src/World/VehicleFX.cpp"

#include "Speed/Indep/Src/World/VehiclePartDamage.cpp"
