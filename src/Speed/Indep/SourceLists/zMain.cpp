// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// datos-gc 2026-10-13: orden de parseo de la CU original (DWARF, guardas _0.._42):
// MGeneric, WorldConn, MAIEngineRev, MAudioReflection, VehicleParams + los 6 de
// VehicleBehaviors, MBreakerStopCops, SmackableParams, SoundConn, MNotifyEngineBlown,
// MEnterRaceOverFlow, MSetTrafficSpeed, MGamePlayMoment, MNotifyMovieFinished,
// ExplosionParams, MMiscSound, MRestartRace, MSetCopAutoSpawnMode, MEnterFreeRoam y
// MFlowReadyForOutro, antes de la lista alfabetica de mensajes de EventDefs.cpp.
// Solo GameCube (es donde el contador _.tmp_ del slice manda): en bloque al principio
// de la unity el prefijo no reproduce el intercalado original de los E*.cpp y en SLUS
// voltea la asignacion de registros del destructor de EventSequencer::Engine
// (100 -> 99,545 %, bisectado); en las demas plataformas el parseo queda como estaba.
#ifdef EA_PLATFORM_GAMECUBE
#include "Speed/Indep/Src/Generated/Messages/MGeneric.h"
#include "Speed/Indep/Src/World/WorldConn.h"
#include "Speed/Indep/Src/Generated/Messages/MAIEngineRev.h"
#include "Speed/Indep/Src/Generated/Messages/MAudioReflection.h"
#include "Speed/Indep/Src/Physics/PVehicle.h"
#include "Speed/Indep/Src/Physics/VehicleBehaviors.h"
#include "Speed/Indep/Src/Generated/Messages/MBreakerStopCops.h"
#include "Speed/Indep/Src/Physics/Smackable.h"
#include "Speed/Indep/Src/EAXSound/SoundConn.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyEngineBlown.h"
#include "Speed/Indep/Src/Generated/Messages/MEnterRaceOverFlow.h"
#include "Speed/Indep/Src/Generated/Messages/MSetTrafficSpeed.h"
#include "Speed/Indep/Src/Generated/Messages/MGamePlayMoment.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyMovieFinished.h"
#include "Speed/Indep/Src/Physics/Explosion.h"
#include "Speed/Indep/Src/Generated/Messages/MMiscSound.h"
#include "Speed/Indep/Src/Generated/Messages/MRestartRace.h"
#include "Speed/Indep/Src/Generated/Messages/MSetCopAutoSpawnMode.h"
#include "Speed/Indep/Src/Generated/Messages/MEnterFreeRoam.h"
#include "Speed/Indep/Src/Generated/Messages/MFlowReadyForOutro.h"
#endif

#include "Speed/Indep/Src/Generated/Events/E911Call.cpp"

#include "Speed/Indep/Src/Generated/Events/EAccelerate.cpp"

#include "Speed/Indep/Src/Generated/Events/EAddSMS.cpp"

#include "Speed/Indep/Src/Generated/Events/EAIEngineRev.cpp"

#include "Speed/Indep/Src/Generated/Events/EAudioRigidBodyTest.cpp"

#include "Speed/Indep/Src/Generated/Events/EAudioSmackableTest.cpp"

#include "Speed/Indep/Src/Generated/Events/EAudioWorldTest.cpp"

#include "Speed/Indep/Src/Generated/Events/EAutoSave.cpp"

#include "Speed/Indep/Src/Generated/Events/EAwardUpgrade.cpp"

#include "Speed/Indep/Src/Generated/Events/EBailPursuit.cpp"

#include "Speed/Indep/Src/Generated/Events/EBecomeAiCar.cpp"

#include "Speed/Indep/Src/Generated/Events/EBecomePlayerCar.cpp"

#include "Speed/Indep/Src/Generated/Events/EBecomePursuitCar.cpp"

#include "Speed/Indep/Src/Generated/Events/EBreakerStopCops.cpp"

#include "Speed/Indep/Src/Generated/Events/ECameraPhotoFinish.cpp"

#include "Speed/Indep/Src/Generated/Events/ECameraShake.cpp"

#include "Speed/Indep/Src/Generated/Events/ECellCall.cpp"

#include "Speed/Indep/Src/Generated/Events/EChangeState.cpp"

#include "Speed/Indep/Src/Generated/Events/ECinematicMoment.cpp"

#include "Speed/Indep/Src/Generated/Events/ECollision.cpp"

#include "Speed/Indep/Src/Generated/Events/ECommitAudioAssets.cpp"

#include "Speed/Indep/Src/Generated/Events/ECommitRenderAssets.cpp"

#include "Speed/Indep/Src/Generated/Events/EDamageLights.cpp"

#include "Speed/Indep/Src/Generated/Events/EDDaySpeech.cpp"

#include "Speed/Indep/Src/Generated/Events/EDebugPrint.cpp"

#include "Speed/Indep/Src/Generated/Events/EDebugScreenMessage.cpp"

#include "Speed/Indep/Src/Generated/Events/EDeliverMessage.cpp"

#include "Speed/Indep/Src/Generated/Events/EDestroyVehicle.cpp"

#include "Speed/Indep/Src/Generated/Events/EDisablePursuitVehicle.cpp"

#include "Speed/Indep/Src/Generated/Events/EDisableTrigger.cpp"

#include "Speed/Indep/Src/Generated/Events/EDispIntroRace.cpp"

#include "Speed/Indep/Src/Generated/Events/EDynamicRegion.cpp"

#include "Speed/Indep/Src/Generated/Events/EEnableAIPhysics.cpp"

#include "Speed/Indep/Src/Generated/Events/EEnableCollisionElement.cpp"

#include "Speed/Indep/Src/Generated/Events/EEnableModeling.cpp"

#include "Speed/Indep/Src/Generated/Events/EEnableTrigger.cpp"

#include "Speed/Indep/Src/Generated/Events/EEndCarStop.cpp"

#include "Speed/Indep/Src/Generated/Events/EEngineBlown.cpp"

#include "Speed/Indep/Src/Generated/Events/EEnterBin.cpp"

#include "Speed/Indep/Src/Generated/Events/EEnterEngagableTrigger.cpp"

#include "Speed/Indep/Src/Generated/Events/EExitEngagableTrigger.cpp"

#include "Speed/Indep/Src/Generated/Events/EFadeScreenNoLoadingBarOff.cpp"

#include "Speed/Indep/Src/Generated/Events/EFadeScreenNoLoadingBarOn.cpp"

#include "Speed/Indep/Src/Generated/Events/EFadeScreenOff.cpp"

#include "Speed/Indep/Src/Generated/Events/EFadeScreenOn.cpp"

#include "Speed/Indep/Src/Generated/Events/EFireEventList.cpp"

#include "Speed/Indep/Src/Generated/Events/EFireRandomTrigger.cpp"

#include "Speed/Indep/Src/Generated/Events/EFireTriggerSpeedCondition.cpp"

#include "Speed/Indep/Src/Generated/Events/EForceCarStop.cpp"

#include "Speed/Indep/Src/Generated/Events/EGPSFinished.cpp"

#include "Speed/Indep/Src/Generated/Events/EGPSLost.cpp"

#include "Speed/Indep/Src/Generated/Events/EGTriggerInternal.cpp"

#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Generated/Events/EHideAutoSaveEngage.cpp"
#endif

#include "Speed/Indep/Src/Generated/Events/EHideObject.cpp"

#include "Speed/Indep/Src/Generated/Events/EHidePart.cpp"

#include "Speed/Indep/Src/Generated/Events/EHideRaceOverMessage.cpp"

#include "Speed/Indep/Src/Generated/Events/EJointDetached.cpp"

#include "Speed/Indep/Src/Generated/Events/EJumpToStrategyFlow.cpp"

#include "Speed/Indep/Src/Generated/Events/EKillJoint.cpp"

#include "Speed/Indep/Src/Generated/Events/EKillObject.cpp"

#include "Speed/Indep/Src/Generated/Events/EKnockoutRacer.cpp"

#include "Speed/Indep/Src/Generated/Events/ELoadingScreenOff.cpp"

#include "Speed/Indep/Src/Generated/Events/ELoadingScreenOn.cpp"

#include "Speed/Indep/Src/Generated/Events/ELoadLost.cpp"

#include "Speed/Indep/Src/Generated/Events/EMissShift.cpp"

#include "Speed/Indep/Src/Generated/Events/EMomentStrm.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISAeroDynamics.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISBrakelock.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISBurnout.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISCarDamageReset.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISCarPitch.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISCarRoll.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISCarShake.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISConstraint.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISCopCarDoors.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISCopLights.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISDetach.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISDetail.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISFakeFar.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISFreeze.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISHideCharacter.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISLights.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISMotionBlur.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISNeutralRev.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISNitro.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISNukeSmack.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISOverlayMessage.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISPixelate.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISPlayEffect.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISRain.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISReattach.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISRoadNoise.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISScreenFlash.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISSteering.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISStopEffects.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISTimeOfDay.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISVisualLook.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISWolrdGeometry.cpp"

#include "Speed/Indep/Src/Generated/Events/ENISWorldAnimTrigger.cpp"

#include "Speed/Indep/Src/Generated/Events/EPause.cpp"

#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Generated/Events/EPauseForMemcard.cpp"
#endif

#include "Speed/Indep/Src/Generated/Events/EPerfectLaunch.cpp"

#include "Speed/Indep/Src/Generated/Events/EPerfectShift.cpp"

#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Generated/Events/EPlayDDFlyInNIS.cpp"
#endif

#include "Speed/Indep/Src/Generated/Events/EPlayEndNIS.cpp"

#include "Speed/Indep/Src/Generated/Events/EPlayerAirborne.cpp"

#include "Speed/Indep/Src/Generated/Events/EPlayerShift.cpp"

#include "Speed/Indep/Src/Generated/Events/EPlayerTriggeredNOS.cpp"

#include "Speed/Indep/Src/Generated/Events/EPlayObjectEffect.cpp"

#include "Speed/Indep/Src/Generated/Events/EPlayRaceMovie.cpp"

#include "Speed/Indep/Src/Generated/Events/EPlayRaceNIS.cpp"

#include "Speed/Indep/Src/Generated/Events/EProcessAreaStimulus.cpp"

#include "Speed/Indep/Src/Generated/Events/EProcessStimulus.cpp"

#include "Speed/Indep/Src/Generated/Events/EPursuitBreaker.cpp"

#include "Speed/Indep/Src/Generated/Events/EQuitDemo.cpp"

#include "Speed/Indep/Src/Generated/Events/EQuitToFE.cpp"

#include "Speed/Indep/Src/Generated/Events/ERaceSheetOff.cpp"

#include "Speed/Indep/Src/Generated/Events/ERaceSheetOn.cpp"

#include "Speed/Indep/Src/Generated/Events/ERandomEventList.cpp"

#include "Speed/Indep/Src/Generated/Events/ERandomExplosion.cpp"

// El prototipo A124 no tiene EReloadGame: ni la clase ni su cadena estan en su ELF.
#ifndef EA_BUILD_A124
#include "Speed/Indep/Src/Generated/Events/EReloadGame.cpp"
#endif

#include "Speed/Indep/Src/Generated/Events/EReloadHud.cpp"

#include "Speed/Indep/Src/Generated/Events/EReportInfraction.cpp"

#include "Speed/Indep/Src/Generated/Events/EReportMilestoneAtStake.cpp"

#include "Speed/Indep/Src/Generated/Events/ERequestEventInfoDialog.cpp"

#include "Speed/Indep/Src/Generated/Events/EResetPlayerCar.cpp"

#include "Speed/Indep/Src/Generated/Events/EResetProps.cpp"

#include "Speed/Indep/Src/Generated/Events/EResetSequencer.cpp"

#include "Speed/Indep/Src/Generated/Events/EResetSystem.cpp"

#include "Speed/Indep/Src/Generated/Events/ERestartRace.cpp"

#include "Speed/Indep/Src/Generated/Events/EScheduleEvent.cpp"

#include "Speed/Indep/Src/Generated/Events/EScheduleEventUpdate.cpp"

#include "Speed/Indep/Src/Generated/Events/ESetCopAutoSpawnMode.cpp"

#include "Speed/Indep/Src/Generated/Events/ESetPlayerCarReset.cpp"

#include "Speed/Indep/Src/Generated/Events/ESetPlayerCollisionCache.cpp"

#include "Speed/Indep/Src/Generated/Events/ESetSimRate.cpp"

#include "Speed/Indep/Src/Generated/Events/EShockObject.cpp"

#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Generated/Events/EShowAutoSaveEngage.cpp"
#endif

#include "Speed/Indep/Src/Generated/Events/EShowMarketingScreen.cpp"

#include "Speed/Indep/Src/Generated/Events/EShowMessageScreen.cpp"

#include "Speed/Indep/Src/Generated/Events/EShowMilestones.cpp"

#include "Speed/Indep/Src/Generated/Events/EShowRaceCountdown.cpp"

#include "Speed/Indep/Src/Generated/Events/EShowRaceOverMessage.cpp"

#include "Speed/Indep/Src/Generated/Events/EShowResults.cpp"

#include "Speed/Indep/Src/Generated/Events/EShowSMS.cpp"

#include "Speed/Indep/Src/Generated/Events/EShowTimeExtension.cpp"

#include "Speed/Indep/Src/Generated/Events/ESimulate.cpp"

#include "Speed/Indep/Src/Generated/Events/ESndGameState.cpp"

#include "Speed/Indep/Src/Generated/Events/ESpawnExplosion.cpp"

#include "Speed/Indep/Src/Generated/Events/ESpawnFragment.cpp"

#include "Speed/Indep/Src/Generated/Events/ESpawnSmackable.cpp"

#include "Speed/Indep/Src/Generated/Events/EStopObjectEffect.cpp"

#include "Speed/Indep/Src/Generated/Events/EStopObjectEffects.cpp"

#include "Speed/Indep/Src/Generated/Events/ETerminateMusic.cpp"

#include "Speed/Indep/Src/Generated/Events/ETips.cpp"

#include "Speed/Indep/Src/Generated/Events/ETireBlown.cpp"

#include "Speed/Indep/Src/Generated/Events/ETirePunctured.cpp"

#include "Speed/Indep/Src/Generated/Events/ETriggerMomentNIS.cpp"

#include "Speed/Indep/Src/Generated/Events/ETuneVehicle.cpp"

#include "Speed/Indep/Src/Generated/Events/EUnPause.cpp"

#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Generated/Events/EUnPauseForMemcard.cpp"
#endif

#include "Speed/Indep/Src/Generated/Events/EVehicleDestroyed.cpp"

#include "Speed/Indep/Src/Generated/Events/EVehicleReset.cpp"

#include "Speed/Indep/Src/Generated/Events/EWakeObject.cpp"

#include "Speed/Indep/Src/Generated/Events/EWorldMapOff.cpp"

#include "Speed/Indep/Src/Generated/Events/EWorldMapOn.cpp"

#include "Speed/Indep/Src/Generated/Events/EventDefs.cpp"

#include "Speed/Indep/Src/Generated/QueryDefs.cpp"

#include "Speed/Indep/Src/Generated/AttribSys/codegen.cpp"

#include "Speed/Indep/Src/Input/Common/action.cpp"

#include "Speed/Indep/Src/Input/Common/actionqueue.cpp"

#include "Speed/Indep/Src/Input/Common/InputDefParser.cpp"

#include "Speed/Indep/Src/Input/Common/InputDevice.cpp"

#include "Speed/Indep/Src/Math/Common/SimRandom.cpp"

#include "Speed/Indep/Src/Main/Common/Event.cpp"

#include "Speed/Indep/Src/Main/Common/EventSequencer.cpp"

#include "Speed/Indep/Src/Main/Common/Scheduler.cpp"

#ifdef EA_PLATFORM_GAMECUBE
#include "Speed/Indep/Src/Main/GC/VirtualMemory.cpp"
#endif

#include "Speed/Indep/Src/Input/Common/iomodule.cpp"

#ifdef EA_PLATFORM_GAMECUBE
#include "Speed/Indep/Src/Input/GC/InputDeviceGC.cpp"
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/Src/Input/PS2/InputDevicePS2.cpp"
#endif

#ifdef EA_PLATFORM_WIN32
#include "Speed/Indep/Src/Input/PC/InputDevicePC.cpp"
#endif

#include "Speed/Indep/Src/Input/Common/SteeringWheelDevice.cpp"
