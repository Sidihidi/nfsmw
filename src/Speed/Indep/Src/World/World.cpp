#include "World.hpp"
#include "DebugVehicleSelection.h"
#include "DebugWorld.h"
#include "Speed/Indep/Src/World/Car.hpp"
#include "Track.hpp"
#include "TrackStreamer.hpp"
#include "CarRender.hpp"
#include "Clans.hpp"
#include "RaceParameters.hpp"
#include "Rain.hpp"
#include "Scenery.hpp"
#include "Skids.hpp"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Libs/Support/Utility/UTypes.h"
#include "Speed/Indep/Src/AI/AIDebug.h"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Camera/ICE/ICEManager.hpp"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/INIS.h"
#include "Speed/Indep/Src/Interfaces/SimModels/IModel.h"
#include "Speed/Indep/Src/Interfaces/SimModels/ISceneryModel.h"
#include "Speed/Indep/Src/Interfaces/Simables/IExplosion.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Misc/Rumble.hpp"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Physics/SmokeableInfo.hpp"
#include "Speed/Indep/Src/Sim/SimSubSystem.h"
#include "Speed/Indep/Src/Animation/AnimPlayer.hpp"
#include "Speed/Indep/Src/World/World.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "TrackInfo.hpp"
#include "TrackPath.hpp"
// datos-gc 2026-10-13: la CU zWorld original parsea WorldConn (_0.._11 en la CU), los 18
// hash de RenderConn (_19.._36) y los 8 de SoundConn (_37.._44) antes de
// UpdateEnvironmentMapCameras (sCarWorldPosition, _45).
#include "Speed/Indep/Src/World/WorldConn.h"
#include "Speed/Indep/Src/Render/RenderConn.h"
#include "Speed/Indep/Src/EAXSound/SoundConn.h"

// TODO the stripped functions here are leftovers from HP2

int SuperEasyAIMode = 0;
int CarControllerDisableCatchup = 0;
// gcdatos17: extendido a las DOS PS2. En la SLUS el hueco de la zona cero delante de
// ZeroVector (+0x0..+0x1ac del .data de zWorld) solo cierra si el cluster entero
// (bBeltMover + PerfectShiftMaxRPMDiff + los seis) esta: 244 B de ranuras + 184 B de
// cluster = 428 B, el mismo reparto que la A124 (216 + 184 = 400 B); sin el cluster
// harian falta 412 B de ranuras, el doble que la A124. Ademas la fuente es compartida:
// el DWARF de GC los declara a todos (mw_dwarfdump) y los stabs de la A124 los tienen
// en .data (PS2_globals.hpp:5698-5706).
#ifdef EA_PLATFORM_PLAYSTATION2
int bBeltMover = 0;
#endif
int UseHP2SteeringCode = 0;
// gcdatos17: idem (PS2_globals.hpp:5700-5705; el DWARF de GC da [9][4], los stabs
// [4][9]: 0x90 B en los dos).
#ifdef EA_PLATFORM_PLAYSTATION2
float PerfectShiftMaxRPMDiff[4][9] = {};
float fHighDriftAngle = 0.0f;
float DriftModeYawMinimum = 0.0f;
char *PersonalityTypeNames[1] = {nullptr};
struct DifficultyInfo;
DifficultyInfo *pTheDifficultyInfo = nullptr;
float ImpactDamageIntensityScale = 0.0f;
float MinLookaheadDistance = 0.0f;
#endif
bVector3 ZeroVector(0.0f, 0.0f, 0.0f);
// gcdatos17: extendido a las DOS PS2. En la SLUS los 32 B de ceros de +0x1ac
// (0x59f620) son [ZeroVector 16 B][los cuatro PrintBoost 16 B] y el hueco de 4 B de
// +0x1d0 (delante de pCurrentWorld, 0x59f648) es TrafficTeleporter: el mismo reparto
// de la A124 (PS2_globals.hpp:5707-5712).
#ifdef EA_PLATFORM_PLAYSTATION2
int PrintBoostCalculationForAllCars = 0;
int PrintBoostCalculationForThisCar = 0;
int DoPerformanceSim = 0;
int PerformanceSimState = 0;
#endif

float UglyTimestepHack = 1.0f / 60.0f;
#ifdef EA_PLATFORM_PLAYSTATION2
float TrafficTeleporterDistanceAroundBubble = 0.0f;
#endif
World *pCurrentWorld = nullptr;

int TweakerPauseWorld = 0;

World::World() {
    pCurrentWorld = this;
    this->WorldRandomSeed = bRandom(0x7FFFFFFF);
    this->TotalNumCars = 0;
    this->AICarPos = 0;
    this->TrafficCarPos = 0;
    this->ParkedCarPos = 0;
    this->ResetCollisionPredictionID();
    this->ResetPotentialDriveTargetID();
    this->OnlinePauseWorld = 0;

    this->ResetTimeScale();
    this->PreviousTimeToWaste = 0.0f;
    bMemSet(this->PlayerPositions, 0x0, sizeof(this->PlayerPositions));
    bMemSet(this->PlayerDATs, 0x0, sizeof(this->PlayerDATs));
}

World::~World() {
    while (!this->CarList.IsEmpty()) {
        delete this->CarList.GetHead()->Remove();
    }
    pCurrentWorld = nullptr;
}

// STRIPPED
void World::DoSnapshot(ReplaySnapshot *snapshot) {}

// STRIPPED
void World::SetTimeScale(float t, float d, void (*func)(int), int param) {}

void World::ResetTimeScale() {
    this->fTimeScale = 1.0f;
    this->fTimeScaleTimer = 0.0f;
    this->fTimeToRemainPaused = 0.0f;
    this->TimeScaleCallback = nullptr;
    this->TimeScaleCallbackParam = 0;
}

// STRIPPED
void World::SetWorldPaused(float t, void (*func)(int), int param) {}

// STRIPPED
void World::BeginPause() {}

// STRIPPED
void World::EndPause() {}

// STRIPPED
void World::BeginOnlinePause() {
    TheAnimPlayer.BeginOnlinePause();
}

// STRIPPED
void World::EndOnlinePause() {
    TheAnimPlayer.EndOnlinePause();
}

// STRIPPED
void World::UpdateWorldPaused() {}

// STRIPPED
int World::IsWorldPaused() {}

// STRIPPED
float World::GetTimestep() {}

// STRIPPED
void World::DoTimestepMove(float timestep) {}

int MAX_CONTACT_RESOLUTIONS = 20;
int MAX_CONTACTS_PER_FRAME = 2;

static const int RecreateWitnesses = 1;

void World::DoTimestepCollisions(float timestep) {}

static const int RespawnSmokeables = 0;

ControlMode OverridePlayerCarControlMode = NO_CONTROL;
int PrintWorldObjectDebugInfo = 0;
int WhichCollisionPredictionIterationToRender = -1;
int WhichCarsRegisteredDriveTargetToRender = -1;
#ifdef EA_BUILD_A124
// gcdatos14: solo en la A124, tras WhichCarsRegisteredDriveTargetToRender (0x643ca8-0x643cb0,
// PS2_globals.hpp:5721-5723), a 0 en su .data objetivo.
int RenderChosenPotentialDriveTarget = 0;
int RenderMyCarDriveTarget = 0;
int RenderOtherCarDriveTarget = 0;
#endif

// STRIPPED
void World::DoTimestep(float timestep) {}

// STRIPPED
unsigned short World::CalculateDebugChecksum() {}

static const int DisableReplayChecksumVerification = 0;

// STRIPPED
void World::VerifyDebugChecksums() {}

// STRIPPED
float World::GetDistanceToClosestPlayer(const bVector2 *point) {}

// STRIPPED
float World::GetDistanceToClosestPlayer(const bVector3 *point) {}

// STRIPPED
unsigned int World::Random(int range) {}

// STRIPPED
float World::Random(float range) {}

// STRIPPED
unsigned int World::ConstantRandom(int range, unsigned int seed) {}

// STRIPPED
float World::ConstantRandom(float range, unsigned int seed) {}

// STRIPPED
void World::IncrementCollisionPredictionID() {}

// STRIPPED
bool World::AddTrafficCar(DriverInfo *d) {}

// STRIPPED
void World::DebugDisplay() {}

// STRIPPED
void World::CollisionPredictionSetOBBs(OBB *my_obbs, OBB *other_car_obbs) {}

// STRIPPED
void World::CollisionPredictionSetDriveTargets(DriveTarget *my_drive_target, DriveTarget *other_car_drive_target) {}

// STRIPPED
void World::RecordPotentialDriveTarget(DriveTarget *drive_target) {}

// STRIPPED
void World::CollisionPredictionRecordIteration(int loop_counter, int8 my_index, int8 other_car_index, bVector2 *my_position,
                                               bVector2 *other_car_position) {}

// STRIPPED
void World::UpdateAIDebugRendering() {}

// STRIPPED
void World::UpdateAIDebugStuff() {}

// UNSOLVED
void World_DEBUGStartLocation(UMath::Vector3 &startLoc, UMath::Vector3 &initialVec) {
    const char *regionName = TrackInfo::GetLoadedTrackInfo()->RegionName;
    float rotInitialVec = 0.0f;

    if (stricmp(regionName, "L2RA") == 0) {
        startLoc.x = -2510.0f;
        startLoc.y = 151.0f;
        startLoc.z = 1767.0f;
        rotInitialVec = 0.15f;
    } else if (stricmp(regionName, "L2RB") == 0) {
        startLoc.x = 0.0f;
        startLoc.y = 21.0f;
        startLoc.z = 0.0f;
    } else if (stricmp(regionName, "L2RC") == 0) {
        startLoc.x = 0.0f;
        startLoc.y = 21.0f;
        startLoc.z = 0.0f;
    } else if (stricmp(regionName, "L2RD") == 0) {
        startLoc.x = 16.0f;
        startLoc.y = 457.0f;
        startLoc.z = 43.0f;
        rotInitialVec = 0.63f;
    } else if (stricmp(regionName, "L2RE") == 0) {
        startLoc.x = 16.0f;
        startLoc.y = 457.0f;
        startLoc.z = 43.0f;
    } else if (stricmp(regionName, "L2RG") == 0) {
        startLoc.x = 2036.0f;
        startLoc.y = 70.0f;
        startLoc.z = -2010.0f;
    } else if (stricmp(regionName, "L4RA") == 0) {
        startLoc.x = 30.0f;
        startLoc.y = 20.0f;
        startLoc.z = 40.0f;
    } else if (stricmp(regionName, "L5RD") == 0) {
        startLoc.x = -20.0f;
        startLoc.y = 2.0f;
        startLoc.z = 100.0f;
    } else if (stricmp(regionName, "L2RX") == 0) {
        startLoc.x = -2510.0f;
        startLoc.y = 151.0f;
        startLoc.z = 1773.0f;
        rotInitialVec = 0.63f;
    }

    initialVec = UMath::Vector3Make(0.0f, 0.0f, 1.0f);

    if (rotInitialVec != 0.0f) {
        UMath::Matrix4 rotMat = UMath::Matrix4::kIdentity;

        UMath::MultYRot(rotMat, rotInitialVec, rotMat);
        UMath::Rotate(initialVec, rotMat, initialVec);
    }

    GRaceCustom *quickRace = GRaceDatabase::Get().GetStartupRace();

    if (quickRace != nullptr) {
        quickRace->GetStartPosition(startLoc);
        quickRace->GetStartDirection(initialVec);
    } else if (GManager::Get().Exists()) {
        GManager::Get().GetRespawnLocation(startLoc, initialVec);
    }

    if (SkipFEOverrideStartPosition != nullptr) {
        startLoc.x = -SkipFEOverrideStartPosition->y;
        startLoc.y = SkipFEOverrideStartPosition->z;
        startLoc.z = SkipFEOverrideStartPosition->x;
    }
}

static void HideNonRaceSmackable(IModel *model) {
    ISceneryModel *scenery;
    if (model->QueryInterface(&scenery)) {
        if (scenery->IsExcluded(4)) {
            model->ReleaseModel();
        }
    }
}

void World_RestoreProps() {
    for (IModel::List::const_iterator m = IModel::GetList().begin(); m != IModel::GetList().end(); m++) {
        IModel *model = *m;
        ISceneryModel *iscenery = (ISceneryModel *)model;
        if (iscenery->QueryInterface(&iscenery)) {
            iscenery->RestoreScene();
        }
    }

    for (IExplosion::List::const_iterator e = IExplosion::GetList().begin(); e != IExplosion::GetList().end(); e++) {
        IExplosion *explosion = *e;
        ISimable *isimable;
        if (explosion->QueryInterface(&isimable)) {
            isimable->Kill();
        }
    }

    if (GRaceStatus::Exists() && GRaceStatus::Get().GetPlayMode() == GRaceStatus::kPlayMode_Racing) {
        IModel::ForEach(HideNonRaceSmackable);
    }

#ifndef EA_BUILD_A124
    GManager::Get().RestorePursuitBreakerIcons(-1);
#endif
    void ResetWorldAnimations();
    ResetWorldAnimations();
    ResetPropTimers();
}

bool Tweak_World_RestoreScenery = false;

void World_Service() {
    AI::Debug();
    UglyTimestepHack = 0.0f;

    INIS *nis = INIS::Get();
    if (nis != nullptr) {
        nis->ServiceLoads();
    }

    UglyTimestepHack = 0.0f;

    void ServiceSpaceNodes();
    ServiceSpaceNodes();
    TheTrackStreamer.ServiceGameState();

    if (GManager::Get().Exists()) {
        GManager::Get().NotifyWorldService();
    }
    DebugVehicleSelection::Get().Service();
    DebugWorld::Get().Service();
}

static void World_Init() {
    ResetWorldTime();
    TheICEManager.Resolve();

    // Decl: 736
    void EstablishRemoteCaffeineConnection();
    EstablishRemoteCaffeineConnection();
    TrackPathInitRemoteCaffeineConnection();
    InitCarEffects();
    InitClans();

    int max_skids = 128;
    if (TheRaceParameters.IsDriftRace()) {
        max_skids = 512;
    }
    InitSkids(max_skids);
    ResetCameraShakers();
    CameraMoverRestartRace();
    InitVisibleZones();
    TheTrackPathManager.ResetZoneVisitInfos();
    SetRainBase();
}

static void World_Shutdown() {
    bVerifyPoolIntegrity(0);
    CloseVisibleZones();
    CloseClans();
    CloseSkids();
    CloseCarEffects();
    ResetWorldTime();

    WorldLoopCounter = 100000;
}

BIND_SIM_SUBSYSTEM(World, World_Init, World_Shutdown);

// STRIPPED
void RenderTrackRoutes(eView *view) {}
