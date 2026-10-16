// The game activity of an online race. Only the PS2 prototype (SLES-53558-A124) has it: its .mdebug puts
// sim/activities/onlinegame.cpp in zSim, between CareerGame.cpp and NISActivity.cpp. It is a QuickGame without the
// split screen and the NIS: it waits for the track, creates a player and a car for every OnlineRacer on the grid and
// starts the race when the OnlineManager does.
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FECarViewer.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pvehicle.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/IGameState.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/IVehicleCache.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IEntity.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IOnlinePlayer.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/InGame/OnlineRacer.h"
#include "Speed/Indep/Src/Physics/PVehicle.h"
#include "Speed/Indep/Src/Physics/PhysicsInfo.hpp"
#include "Speed/Indep/Src/Physics/PhysicsUpgrades.hpp"
#include "Speed/Indep/Src/Sim/SimActivity.h"
#include "Speed/Indep/Src/Sim/SimTypes.h"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#include "Speed/Indep/Src/World/TrackStreamer.hpp"
#include "Speed/Indep/Src/World/WCollisionMgr.h"
#include "Speed/Indep/Src/World/WWorld.h"
#include "Speed/Indep/Src/World/World.hpp"

// total size: 0x140
class OnlineGame : public Sim::Activity, public Sim::ITimeManager, Sim::IStateManager, public IVehicleCache, public IGameState {
  public:
    enum eState {
        RACESTATE_LOADING_WORLD = 0,
        RACESTATE_LOADING_PARTICIPANTS = 1,
        RACESTATE_LOADING_DONE = 2,
    };

    OnlineGame(Sim::Param params);
    ~OnlineGame() override;

    static Sim::IActivity *Construct(Sim::Param params);

    // IGameState
    bool InGameBreaker() const override {
        return false;
    }

    void RaceReset() override {}

    // ITimeManager
    float OnManageTime(float real_time_delta, float sim_rate) override;

    // IStateManager
    Sim::State OnManageState(Sim::State state) override;
    bool ShouldPauseInput() override;

    // IVehicleCache
    eVehicleCacheResult OnQueryVehicleCache(const IVehicle *removethis, const IVehicleCache *whosasking) const override;
    void OnRemovedVehicleCache(IVehicle *ivehicle) override;

    void OnWorldLoaded();
    bool ArePlayersLoading() const;

  private:
    void CreatePlayersAndCars(const UMath::Vector3 &startLoc);

    eState mState;                           // offset 0x110, size 0x4
    ALIGNVEC UMath::Vector3 mStartLocation;  // offset 0x120, size 0xC
    ALIGNVEC UMath::Vector3 mStartDirection; // offset 0x130, size 0xC
};

BIND_ACTIVITY_FACTORY(OnlineGame)

OnlineGame::OnlineGame(Sim::Param params)
    : Sim::Activity(4),                      //
      Sim::ITimeManager(this),               //
      Sim::IStateManager(this),              //
      IVehicleCache(this),                   //
      IGameState(this),                      //
      mState(RACESTATE_LOADING_WORLD),       //
      mStartLocation(UMath::Vector3::kZero) {
    unsigned int raceHash = SkipFE ? Attrib::StringHash32("4.1.4") : FEDatabase->GetOnlineSettings()->GetRaceSettings()->EventHash;
    GRaceParameters *parms = GRaceDatabase::Get().GetRaceFromHash(raceHash);

    parms->GetStartPosition(mStartLocation);
    parms->GetStartDirection(mStartDirection);
    World_DEBUGStartLocation(mStartLocation, mStartDirection);
    Sim::SetStream(mStartLocation, false);

    RichPresence::Instance()->SetEventHash(raceHash);
    RichPresence::Instance()->SetIsRacing(true);
}

bool OnlineGame::ShouldPauseInput() {
    if (Sim::GetState() != Sim::STATE_ACTIVE) {
        return true;
    }

    if (FEManager::Get() && FEManager::ShouldPauseSimulation(true)) {
        return true;
    }

    return false;
}

OnlineGame::~OnlineGame() {
    RichPresence::Instance()->SetIsRacing(false);
}

bool OnlineGame::ArePlayersLoading() const {
    const IVehicle::List &vehicles = IVehicle::GetList(VEHICLE_PLAYERS);
    for (IVehicle::List::const_iterator iter = vehicles.begin(); iter != vehicles.end(); iter++) {
        IVehicle *vehicle = *iter;
        if (vehicle->IsLoading()) {
            return true;
        }
    }
    return false;
}

Sim::IActivity *OnlineGame::Construct(Sim::Param params) {
    return new OnlineGame(Sim::Param(params));
}

float OnlineGame::OnManageTime(float real_time_delta, float sim_rate) {
    return 1.0f;
}

eVehicleCacheResult OnlineGame::OnQueryVehicleCache(const IVehicle *removethis, const IVehicleCache *whosasking) const {
    for (IPlayer::List::const_iterator iter = IPlayer::GetList(PLAYER_ALL).begin(); iter != IPlayer::GetList(PLAYER_ALL).end(); iter++) {
        IPlayer *player = *iter;
        if (UTL::COM::ComparePtr(player->GetSimable(), removethis)) {
            return VCR_WANT;
        }
    }
    return VCR_DONTCARE;
}

void OnlineGame::OnRemovedVehicleCache(IVehicle *ivehicle) {}

void OnlineGame::CreatePlayersAndCars(const UMath::Vector3 &startLoc) {
    // Without nitrous in the game options, no custom car keeps its nitrous upgrade.
    if (!FEDatabase->GetOnlineSettings()->UseNOS) {
        for (int i = 0; i < 4; i++) {
            OnlineRacer *racer = TheOnlineManager.GetOnlineRacer(i);
            if (racer && racer->IsCustomCar) {
                racer->CarCustomization.SetInstalledPhysics(Physics::Upgrades::PUT_NOS, 0);
            }
        }
    }

    // With performance matching every car is matched to the best of each rating on the grid.
    Physics::Info::Performance *match = nullptr;
    Physics::Info::Performance best(0.0f, 0.0f, 0.0f);
    if (FEDatabase->GetOnlineSettings()->PerformanceMatching) {
        for (int i = 0; i < 4; i++) {
            OnlineRacer *racer = TheOnlineManager.GetOnlineRacer(i);
            if (racer) {
                Attrib::Gen::pvehicle vehicle(racer->CarTypeKey, 0, nullptr);
                if (racer->IsCustomCar) {
                    racer->CarCustomization.WriteRecordIntoPhysics(vehicle);
                }

                Physics::Info::Performance performance;
                if (Physics::Info::ComputePerformance(vehicle, performance)) {
                    if (best.Acceleration < performance.Acceleration) {
                        best.Acceleration = performance.Acceleration;
                    }
                    if (best.TopSpeed < performance.TopSpeed) {
                        best.TopSpeed = performance.TopSpeed;
                    }
                    if (best.Handling < performance.Handling) {
                        best.Handling = performance.Handling;
                    }
                }
            }
        }
        match = &best;
    }

    UMath::Vector3 initialVec = mStartDirection;
    UMath::Vector3 rightVec = {1.0f, 0.0f, 0.0f};
    UMath::Vector3 initialPos = startLoc;

    if (!WWorld::Get().IsValid()) {
        initialPos = UMath::Vector3::kZero;
    }

    float height = 0.0f;
    if (WCollisionMgr(0, 3).GetWorldHeightAtPoint(initialPos, height, nullptr)) {
        initialPos.y = height + 1.0f;
    }

    // The grid: three metres between drivers, from the left.
    float gridLeft = -6.0f;

    for (int i = 0; i < 4; i++) {
        OnlineRacer *racer = TheOnlineManager.GetOnlineRacer(i);
        if (racer) {
            bool isLocal = racer == TheOnlineManager.GetLocalRacer();
            Sim::IEntity *entity =
                Sim::IEntity::CreateInstance(isLocal ? "OnlineLocalPlayer" : "OnlineRemotePlayer", Sim::Param());

            IPlayer *player;
            if (entity->QueryInterface(&player)) {
                int driverNum = racer->GetDriverNumber();
                float gridPos = gridLeft + driverNum * 3.0f;
                UMath::Vector3 playerPos;
                UMath::ScaleAdd(rightVec, gridPos, initialPos, playerPos);

                Attrib::Gen::pvehicle vehicle(racer->CarTypeKey, 0, nullptr);
                const char *carType = vehicle.CollectionName();
                FECustomizationRecord *custom = racer->IsCustomCar ? &racer->CarCustomization : nullptr;
                unsigned int vp_flags = 2;
                if (racer->IsCustomCar && !FEDatabase->GetOnlineSettings()->UseNOS) {
                    vp_flags |= 4;
                }

                ISimable *icar = ISimable::CreateInstance(
                    "PVehicle", VehicleParams(this, isLocal ? DRIVER_HUMAN : DRIVER_REMOTE, Attrib::StringToKey(carType), initialVec,
                                              playerPos, vp_flags, custom, match));
                if (icar) {
                    icar->Attach(player);

                    IOnlinePlayer *online;
                    if (entity->QueryInterface(&online)) {
                        online->SetOnlineRacer(racer);
                    }

                    if (isLocal) {
                        player->SetControllerPort(FEDatabase->PlayerJoyports[0]);
                        player->SetSettings(0);
                        player->SetRenderPort(1);
                    }
                }
            }
        }
    }

    CarViewer::haveLoadedOnce = false;
}

void OnlineGame::OnWorldLoaded() {
    CreatePlayersAndCars(mStartLocation);

    if (TheOnlineManager.IsServer()) {
        Sim::IActivity::CreateInstance("AvoidableManager", Sim::Param());
        Sim::IActivity::CreateInstance("AICopManager", Sim::Param());
    }

    Sim::IActivity::CreateInstance("GameplayActivity", Sim::Param());
    Sim::IActivity::CreateInstance("PathFinder", Sim::Param());
    Sim::IActivity::CreateInstance("Gps", Sim::Param());
    Sim::IActivity::CreateInstance("NISListenerActivity", Sim::Param());
}

Sim::State OnlineGame::OnManageState(Sim::State state) {
    Sim::State newstate = state;

    if (state == Sim::STATE_NONE) {
        if (mState != RACESTATE_LOADING_DONE) {
            return Sim::STATE_INITIALIZING;
        }
    } else if (state == Sim::STATE_INITIALIZING) {
        switch (mState) {
            case RACESTATE_LOADING_WORLD:
                if (!TheTrackStreamer.IsLoadingInProgress()) {
                    OnWorldLoaded();
                    mState = RACESTATE_LOADING_PARTICIPANTS;
                    return Sim::STATE_INITIALIZING;
                }
                break;

            case RACESTATE_LOADING_PARTICIPANTS:
                if (!ArePlayersLoading()) {
                    mState = RACESTATE_LOADING_DONE;
                    TheOnlineManager.ReadyStartLine();
                    return Sim::STATE_INITIALIZING;
                }
                break;

            case RACESTATE_LOADING_DONE:
                return TheOnlineManager.GetState() == OLS_RACING ? Sim::STATE_ACTIVE : newstate;
        }
    } else if (state == Sim::STATE_ACTIVE) {
        // Losing the connection ends the race.
        if (!TheOnlineManager.GetLocalRacer() || !TheOnlineManager.GetLocalRacer()->IsConnected()) {
            return Sim::STATE_IDLE;
        }
    }

    return newstate;
}
