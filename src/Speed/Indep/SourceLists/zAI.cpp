// zAI emite las instanciaciones globales de STLport (_Rb_global, etc.) como el original:
// el define va antes de cualquier cabecera STL de la unidad (tambien esta en
// AITrafficManager.cpp, pero los includes de abajo se parsean antes que ese .cpp).
#define _STLP_NO_FORCE_INSTANTIATE 1

#include "Speed/Indep/bWare/Inc/Strings.hpp" // bStringHash, que UCrc.h (via PVehicle.h) necesita declarado

// datos-gc 2026-10-13: orden de parseo de la CU original (DWARF, guardas _0.._33):
// VehicleParams, MGeneric, MSpawnTraffic, MSetTrafficSpeed, los 6 TypeName de
// VehicleBehaviors, los once mensajes de policia (MSpawnCop..MControlPathfinder),
// WorldConn y MAudioReflection, antes del patrolgoal de AICopManager (_34).
#include "Speed/Indep/Src/Physics/PVehicle.h"
#include "Speed/Indep/Src/Generated/Messages/MGeneric.h"
#include "Speed/Indep/Src/Generated/Messages/MSpawnTraffic.h"
#include "Speed/Indep/Src/Generated/Messages/MSetTrafficSpeed.h"
#include "Speed/Indep/Src/Physics/VehicleBehaviors.h"
#include "Speed/Indep/Src/Generated/Messages/MSpawnCop.h"
#include "Speed/Indep/Src/Generated/Messages/MUnspawnCop.h"
#include "Speed/Indep/Src/Generated/Messages/MSetCopAutoSpawnMode.h"
#include "Speed/Indep/Src/Generated/Messages/MSetCopsEnabled.h"
#include "Speed/Indep/Src/Generated/Messages/MForcePursuitStart.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyPlayerRep.h"
#include "Speed/Indep/Src/Generated/Messages/MReqRoadBlock.h"
#include "Speed/Indep/Src/Generated/Messages/MReqBackup.h"
#include "Speed/Indep/Src/Generated/Messages/MBreakerStopCops.h"
#include "Speed/Indep/Src/Generated/Messages/MPerpEscaped.h"
#include "Speed/Indep/Src/Generated/Messages/MControlPathfinder.h"

#include "Speed/Indep/Src/AI/Activities/AITrafficManager.cpp"

// WorldConn (_21.._32) y MAudioReflection (_33) se parsean tras AITrafficManager y antes
// del patrolgoal de AICopManager (_34), como en la CU original.
#include "Speed/Indep/Src/World/WorldConn.h"
#include "Speed/Indep/Src/Generated/Messages/MAudioReflection.h"

#include "Speed/Indep/Src/AI/Activities/AICopManager.cpp"

#include "Speed/Indep/Src/AI/Activities/AvoidableManager.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionNone.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionTooDamaged.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionGetUnstuck.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionTraffic.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionPursuitOffRoad.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionHeliPursuit.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionHeliExit.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionHeadOnRam.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionRam.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionStopShort.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionSpline.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionStrafe.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionAirborne.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionJackKnife.cpp"

// datos-gc 2026-10-13: tras MJackKnife (_35) la CU original parsea MGamePlayMoment (_36)
// y MPerpBusted (_37) antes de los car_hash/heli_hash de AIPerpVehicle (_38/_39).
#include "Speed/Indep/Src/Generated/Messages/MGamePlayMoment.h"
#include "Speed/Indep/Src/Generated/Messages/MPerpBusted.h"

#include "Speed/Indep/Src/AI/Actions/AIActionStaticRoadBlock.cpp"

#include "Speed/Indep/Src/AI/Actions/AIActionRace.cpp"

#include "Speed/Indep/Src/AI/Common/AIVehicle.cpp"

#include "Speed/Indep/Src/AI/Common/AIVehicleCopCar.cpp"

#include "Speed/Indep/Src/AI/Common/AIVehicleRacecar.cpp"

#include "Speed/Indep/Src/AI/Common/AIVehicleTraffic.cpp"

#include "Speed/Indep/Src/AI/Common/AIVehiclePursuit.cpp"

#include "Speed/Indep/Src/AI/Common/AIVehicleHelicopter.cpp"

#include "Speed/Indep/Src/AI/Common/AdaptivePIDController.cpp"

#include "Speed/Indep/Src/AI/Common/AITarget.cpp"

#include "Speed/Indep/Src/AI/Common/AISteer.cpp"

#include "Speed/Indep/Src/AI/Common/AIAction.cpp"

#include "Speed/Indep/Src/AI/Common/AIGoal.cpp"

#include "Speed/Indep/Src/AI/Common/AIPursuit.cpp"

#include "Speed/Indep/Src/AI/Common/AIRoadBlock.cpp"

#include "Speed/Indep/Src/AI/Common/AIRoadBlockSetups.cpp"

#include "Speed/Indep/Src/AI/Common/AISpawnManager.cpp"

#include "Speed/Indep/Src/AI/Common/AIMath.cpp"

#include "Speed/Indep/Src/AI/Gps.cpp"
