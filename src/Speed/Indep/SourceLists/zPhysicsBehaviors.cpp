// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// datos-gc 2026-10-13: orden de parseo de la CU original (DWARF): VehicleParams + los 6
// TypeName de VehicleBehaviors + ExplosionParams + MGeneric + MSetTrafficSpeed (guardas
// _0.._9), los 18 hash de RenderConn (_10.._27), 4 _GetKind + MAudioReflection +
// SmackableParams, los 12 de WorldConn y los 8 de SoundConn (_46.._53). Se parsean aqui
// las cabeceras de los dos primeros tramos para reproducir el contador _.tmp_ de la TU.
#include "Speed/Indep/Src/Physics/PVehicle.h"
#include "Speed/Indep/Src/Physics/VehicleBehaviors.h"
#include "Speed/Indep/Src/Physics/Explosion.h"
#include "Speed/Indep/Src/Generated/Messages/MGeneric.h"
#include "Speed/Indep/Src/Generated/Messages/MSetTrafficSpeed.h"
#include "Speed/Indep/Src/Render/RenderConn.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyVehicleDestroyed.h"
#include "Speed/Indep/Src/Generated/Messages/MEnterRaceOverFlow.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyEngineBlown.h"
#include "Speed/Indep/Src/Generated/Messages/MGamePlayMoment.h"
#include "Speed/Indep/Src/Generated/Messages/MAudioReflection.h"
#include "Speed/Indep/Src/Physics/Smackable.h"

#include "Speed/Indep/Src/Physics/Behaviors/BehaviorSpecs.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/RigidBody.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/SimpleRigidBody.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/RBVehicle.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/RBTractor.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/RBCop.cpp"

#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Physics/Behaviors/RBRemote.cpp"
#endif

#include "Speed/Indep/Src/Physics/Behaviors/Effects.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/DamageVehicle.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/DamageRacer.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/DamageDragster.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/DamageHeli.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/DamageCopCar.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/PInput.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/Chassis.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/SuspensionRacer.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/SuspensionTraffic.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/SuspensionSimple.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/SuspensionTrailer.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/SuspensionSpline.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/EngineRacer.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/EngineSpline.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/SimpleChopper.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/EngineTraffic.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/DrawVehicle.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/DrawHeli.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/DrawCar.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/SoundCar.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/ResetCar.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/SoundHeli.cpp"

#include "Speed/Indep/Src/Physics/Behaviors/SpikeStrip.cpp"
