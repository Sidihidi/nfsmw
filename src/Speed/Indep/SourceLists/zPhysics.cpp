// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// datos-gc 2026-10-13: orden de parseo de la CU original (DWARF): MGeneric (_0),
// Smackable/Vehicle/ExplosionParams (_1.._3), los 6 TypeName de VehicleBehaviors
// (_4.._9, los duenos del slice), WorldConn (_10.._21) y MAudioReflection (_22),
// antes del MJumpCut de PVehicle.cpp (_23) y su temp_record (_24).
#include "Speed/Indep/Src/Generated/Messages/MGeneric.h"
#include "Speed/Indep/Src/Physics/Smackable.h"
#include "Speed/Indep/Src/Physics/PVehicle.h"
#include "Speed/Indep/Src/Physics/Explosion.h"
#include "Speed/Indep/Src/Physics/VehicleBehaviors.h"
#include "Speed/Indep/Src/World/WorldConn.h"
#include "Speed/Indep/Src/Generated/Messages/MAudioReflection.h"

// Las tablas estaticas de las interfaces de simulacion estan en la .bss de esta
// unidad en el original, delante de todo lo demas, pero ningun .cpp con codigo
// las contiene (debug_lines no da su fichero): se definen aqui, en el orden en
// que el original las construye.
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"
#include "Speed/Indep/Libs/Support/Utility/UListable.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IEntity.h"
#include "Speed/Indep/Src/Interfaces/Simables/IExplosion.h"
#include "Speed/Indep/Src/Interfaces/Simables/IINput.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/IVehicleCache.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Interfaces/Simables/ICollisionBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimpleBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/IRigidBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/ITrafficCenter.h"
#include "Speed/Indep/Src/Interfaces/Simables/IDisposable.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IRecordablePlayer.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/SimModels/IModel.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISpikeable.h"
#include "Speed/Indep/Src/Interfaces/IFengHud.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/IActivity.h"
#include "Speed/Indep/Src/Interfaces/Simables/ICause.h"

#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Physics/SmackableTrigger.cpp"
#endif

IMPLEMENT_LISTABLE(IExplosion)
IMPLEMENT_LISTABLE(IDisposable)
// IVehicle: 10 en GameCube y PS2, 20 en PC y X360 (IVehicle.h).
IMPLEMENT_LISTABLESET(IVehicle, eVehicleList, VEHICLE_MAX)
IMPLEMENT_LISTABLE(IRigidBody)
IMPLEMENT_LISTABLE(ICollisionBody)
IMPLEMENT_LISTABLE(ISimpleBody)
template <>
UTL::Collections::ListableSet<Sim::IEntity, 8, eEntityList, ENTITY_MAX>::_ListSet
    UTL::Collections::ListableSet<Sim::IEntity, 8, eEntityList, ENTITY_MAX>::_mLists =
        UTL::Collections::ListableSet<Sim::IEntity, 8, eEntityList, ENTITY_MAX>::_ListSet();
template <>
UTL::Collections::ListableSet<IPlayer, 8, ePlayerList, PLAYER_MAX>::_ListSet
    UTL::Collections::ListableSet<IPlayer, 8, ePlayerList, PLAYER_MAX>::_mLists =
        UTL::Collections::ListableSet<IPlayer, 8, ePlayerList, PLAYER_MAX>::_ListSet();
IMPLEMENT_LISTABLE(IRecordablePlayer)
IMPLEMENT_LISTABLE(IInputPlayer)
IMPLEMENT_LISTABLE(IModel)
IMPLEMENT_LISTABLE(IPursuit)
IMPLEMENT_LISTABLE(IRoadBlock)
IMPLEMENT_LISTABLE(IHud)
IMPLEMENT_LISTABLE(IVehicleCache)
IMPLEMENT_LISTABLE(ITrafficCenter)
IMPLEMENT_LISTABLE(ISpikeable)

template <>
UTL::Collections::Instanceable<HSIMABLE, ISimable, 160>::_List UTL::Collections::Instanceable<HSIMABLE, ISimable, 160>::_mList =
    UTL::Collections::Instanceable<HSIMABLE, ISimable, 160>::_List();
template <>
UTL::Collections::Instanceable<HACTIVITY, Sim::IActivity, 40>::_List UTL::Collections::Instanceable<HACTIVITY, Sim::IActivity, 40>::_mList =
    UTL::Collections::Instanceable<HACTIVITY, Sim::IActivity, 40>::_List();
// IModel: 434 en GameCube y PS2, 2296 en PC y X360 (IModel.h).
template <>
UTL::Collections::Instanceable<HMODEL, IModel, IModel::InstanceLimit>::_List
    UTL::Collections::Instanceable<HMODEL, IModel, IModel::InstanceLimit>::_mList =
        UTL::Collections::Instanceable<HMODEL, IModel, IModel::InstanceLimit>::_List();
template <>
UTL::Collections::Instanceable<HCAUSE, ICause, 10>::_List UTL::Collections::Instanceable<HCAUSE, ICause, 10>::_mList =
    UTL::Collections::Instanceable<HCAUSE, ICause, 10>::_List();

#ifndef EA_BUILD_A124
#include "Speed/Indep/Src/Physics/SmackableTrigger.cpp"
#endif

#include "Speed/Indep/Src/Physics/Common/Explosion.cpp"

#include "Speed/Indep/Src/Physics/Common/PVehicle.cpp"

#include "Speed/Indep/Src/Physics/Common/PhysicsObject.cpp"

#include "Speed/Indep/Src/Physics/Common/Smackable.cpp"

#include "Speed/Indep/Src/Physics/Common/Wheel.cpp"

#include "Speed/Indep/Src/Physics/Common/VehicleBehaviors.cpp"

#include "Speed/Indep/Src/Physics/Common/VehicleSystem.cpp"

#include "Speed/Indep/Src/Physics/Common/Behavior.cpp"

#include "Speed/Indep/Src/Physics/Common/Bounds.cpp"

#include "Speed/Indep/Src/Physics/Common/SmokeableInfo.cpp"

#include "Speed/Indep/Src/Physics/PhysicsUpgrades.cpp"

#include "Speed/Indep/Src/Physics/PhysicsInfo.cpp"

#include "Speed/Indep/Src/Physics/PhysicsTunings.cpp"

// Los tres Singleton de las interfaces de actividad (en el .data de zPhysics en GameCube y en las dos PS2). Al
// final de la unidad: delante de SmackableTrigger.cpp cambian el nombre de su _GLOBAL_.I de GameCube.
#include "Speed/Indep/Src/Interfaces/SimActivities/ICopMgr.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/IGameState.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/ITrafficMgr.h"
IMPLEMENT_SINGLETON(ICopMgr)
IMPLEMENT_SINGLETON(IGameState)
IMPLEMENT_SINGLETON(ITrafficMgr)
