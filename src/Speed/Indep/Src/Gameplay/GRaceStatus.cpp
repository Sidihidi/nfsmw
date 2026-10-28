#include "GRaceStatus.h"

#include "GManager.h"
#include "GRuntimeInstance.h"
#include "GTrigger.h"
#include "GVault.h"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/Database/VehicleDB.hpp"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pvehicle.h"
#include "Speed/Indep/Src/Generated/Events/EAutoSave.hpp"
#include "Speed/Indep/Src/Generated/Hash.hpp"
#include "Speed/Indep/Src/Generated/Messages/MLoadingComplete.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyRaceTime.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyRaceTimeExpired.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyRaceTimeSecTick.h"
#include "Speed/Indep/Src/Generated/Messages/MTriggerEnter.h"
#include "Speed/Indep/Src/World/TrackStreamer.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOn.hpp"
#include "Speed/Indep/Src/Generated/Events/EReloadHud.hpp"
#include "Speed/Indep/Src/Interfaces/SimActivities/ICopMgr.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/ITrafficMgr.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAudible.h"
#include "Speed/Indep/Src/Interfaces/Simables/IRenderable.h"
#include "Speed/Indep/Src/Interfaces/Simables/IEngine.h"
#include "Speed/Indep/Src/Interfaces/Simables/IRigidBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Misc/Table.hpp"
#include "Speed/Indep/Src/Physics/PhysicsInfo.hpp"
#include "Speed/Indep/Src/Physics/PVehicle.h"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/World/CarInfo.hpp"
#include "Speed/Indep/Src/World/Rain.hpp"
#include "Speed/Indep/Src/World/TimeOfDay.hpp"
#include "Speed/Indep/Src/World/WCollisionAssets.h"
#include "Speed/Indep/Src/World/WorldModel.hpp"
#include "Speed/Indep/Src/World/WRoadNetwork.h"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

#include <algorithm>

extern const char *SkipFEOpponentPresetRide;
PresetCar *FindFEPresetCar(unsigned int hash);

static const float Tweak_GlueSpreadData_Low[5] = {1000.0f, 900.0f, 750.0f, 600.0f, 800.0f};
static const float Tweak_GlueSpreadData_High[5] = {300.0f, 275.0f, 250.0f, 150.0f, 250.0f};
static const float Tweak_GlueStrengthData_Low[5] = {0.75f, 0.75f, 0.75f, 0.5f, 0.25f};
static const float Tweak_GlueStrengthData_High[5] = {0.75f, 0.75f, 0.75f, 0.75f, 0.25f};
static const float Tweak_QuickRaceGlue[3] = {0.0f, 0.5f, 1.0f};

Table Tweak_GlueSpreadTable_Low(Tweak_GlueSpreadData_Low, 5, 0.0f, 100.0f);
Table Tweak_GlueSpreadTable_High(Tweak_GlueSpreadData_High, 5, 0.0f, 100.0f);
Table Tweak_GlueStrengthTable_Low(Tweak_GlueStrengthData_Low, 5, 0.0f, 100.0f);
Table Tweak_GlueStrengthTable_High(Tweak_GlueStrengthData_High, 5, 0.0f, 100.0f);

DECLARE_CONTAINER_TYPE(ID_ROAD_SET);
DECLARE_CONTAINER_TYPE(ID_PATH_SET);

struct PathSegment {
    float mDistance;
    UTL::Std::set<short, _type_ID_ROAD_SET> mRoads;

    bool operator<(const PathSegment &other) const {
        return mDistance < other.mDistance;
    }
};

GRaceStatus *GRaceStatus::fObj = NULL;

GRaceStatus::GRaceStatus() : UTL::COM::Object(1), IVehicleCache(this) {
    int binNumber = FEDatabase->GetCareerSettings()->GetCurrentBin();

    mNextCheckpoint = nullptr;
    mCheckpointModel = nullptr;
    mCheckpointEmitter = nullptr;
    mIsLoading = false;
    mActivelyRacing = false;
    mRaceParms = nullptr;
    mRacerCount = 0;
    nSpeedTraps = 0;
    mRaceContext = GRace::kRaceContext_Career;
    mRaceBin = GRaceDatabase::Get().GetBinNumber(binNumber);
    mQueueBinChange = false;
    mNumTollbooths = 0;
    fRaceLength = 0.0f;
    fFirstLapLength = 0.0f;
    fSubsequentLapLength = 0.0f;
    mVehicleCacheLocked = false;
    bRaceRouteError = false;
    mTrafficDensity = 0;
    mTrafficPattern = 0;
    mScriptWaitingForLoad = false;
#ifndef EA_BUILD_A124
    mHasBeenWon = false;
    mPlayMode = kPlayMode_Racing;
    mRefreshBinAfterRace = false;
    mWarpWhenInFreeRoam = 0;

    mCaluclatedAdaptiveGain = false;
#endif

    ClearTimes();
    SyncronizeAdaptiveBonus();
    MakeDefaultCatchUpData();

    if (GRaceDatabase::Get().GetStartupRace() == nullptr) {
        EnterBin(binNumber);
        SetRoaming();
    }
}

GRaceStatus::~GRaceStatus() {
    if (mCheckpointModel != nullptr) {
        delete mCheckpointModel;
        mCheckpointModel = nullptr;
    }
    if (mCheckpointEmitter != nullptr) {
        mCheckpointEmitter->UnSubscribe();
        delete mCheckpointEmitter;
        mCheckpointEmitter = nullptr;
    }
}

void GRaceStatus::Init() {
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_XENON) // X360: bMalloc(0x4620, "GRaceStatus", 0, 0) en el EGM (0x82357B40)
    fObj = new ("GRaceStatus", 0) GRaceStatus();
#else
    fObj = new (GetVirtualMemoryAllocParams()) GRaceStatus();
#endif
}

void GRaceStatus::Shutdown() {
    if (fObj != nullptr) {
        delete fObj;
    }
    fObj = nullptr;
}

void GRaceStatus::EnableBinBarriers() {
    if (mRaceBin != nullptr) {
        mRaceBin->EnableBarriers();
    }
}

void GRaceStatus::RefreshBinWhileInGame() {
#ifdef EA_BUILD_A124
    // La A124 solo pide el cambio si el bin de la carrera no es el de la carrera profesional.
    int newBin = FEDatabase->GetCareerSettings()->GetCurrentBin();
    if (mRaceBin == nullptr || mRaceBin->GetBinNumber() != newBin) {
        mQueueBinChange = true;
    }
#else
    mQueueBinChange = true;
#endif
}

void GRaceStatus::EnterBin(unsigned int binNumber) {
    if (mRaceBin != nullptr) {
        mRaceBin->DisableBarriers();
    }
    mRaceBin = GRaceDatabase::Get().GetBinNumber(binNumber);
    if (mRaceBin != nullptr) {
        mRaceBin->EnableBarriers();
#ifdef EA_BUILD_A124 // la A124 pide la boveda hija sin comprobar el bin
    }
    {
#endif
        GVault *childVault = mRaceBin->GetChildVault();
        if (childVault != nullptr && !childVault->IsLoaded()) {
            childVault->LoadSyncTransient();
        }
    }
    if (GManager::Get().GetInGameplay()) {
#ifdef EA_BUILD_A124
        for (GObjectIterator<GActivity> activityIter(0xFFFFFFFF); activityIter.IsValid(); activityIter.Advance()) {
            GActivity *activity = activityIter.GetInstance();
            if (activity->AutoStart() && !activity->GetIsRunning()) {
                activity->Run();
            }
        }

        GActivity *binActivity = static_cast<GActivity *>(GManager::Get().FindInstance(mRaceBin->GetCollectionKey()));
        if (!binActivity->GetIsRunning()) {
            binActivity->Run();
        }
#else
        GManager::Get().StartWorldActivities(true);
        GManager::Get().StartBinActivity(mRaceBin);
        GManager::Get().RefreshEngageTriggerIcons();
        GManager::Get().RefreshSpeedTrapIcons();
#endif
    }
}

void GRaceStatus::CalculateRankings() {
    GRacerInfo *ranked[16];
    for (int i = 0; i < mRacerCount; i++) {
        ranked[i] = &mRacerInfo[i];
    }

    bool byPoints = mRaceParms->GetRankPlayersByPoints();
    for (int n = mRacerCount - 1; n > 0; n--) {
        for (int i = 0; i < n; i++) {
            bool swap;
            if (byPoints) {
                GRacerInfo *b = ranked[n];
                GRacerInfo *a = ranked[i];
                if (b->GetPointTotal() + a->GetPointTotal() > 1.0f) {
                    swap = a->GetPointTotal() < b->GetPointTotal();
                } else {
                    swap = a->IsBehind(*b);
                }
            } else {
                swap = ranked[i]->IsBehind(*ranked[n]);
            }
            if (swap) {
                GRacerInfo *tmp = ranked[n];
                ranked[n] = ranked[i];
                ranked[i] = tmp;
            }
        }
    }

    int humans = 0;
    for (int i = 0; i < mRacerCount; i++) {
        int rank = i + 1;
        ranked[i]->SetRanking(rank);
        if (ranked[i]->GetGameCharacter() == nullptr) {
            ranked[i]->mAiRanking = 0;
            humans++;
        } else {
            ranked[i]->mAiRanking = rank - humans;
        }
    }
}

void GRaceStatus::SortCheckPointRankings() {
    for (int i = 0; i < 16; i++) {
        GRacerInfo *sortedRacers[16];
        int players_encountered;

        for (int idx = 0; idx < mRacerCount; idx++) {
            sortedRacers[idx] = &mRacerInfo[idx];
        }

        for (int sort1 = mRacerCount - 1; sort1 > 0; sort1--) {
            for (int sort2 = 0; sort2 < sort1; sort2++) {
                bool swap;
                float spd1 = sortedRacers[sort1]->mSpeedTrapSpeed[i];
                float spd2 = sortedRacers[sort2]->mSpeedTrapSpeed[i];

                swap = spd2 < spd1;
                if (swap) {
                    GRacerInfo *temp = sortedRacers[sort1];
                    sortedRacers[sort1] = sortedRacers[sort2];
                    sortedRacers[sort2] = temp;
                }
            }
        }

        for (int rankIndex = 0; rankIndex < mRacerCount; rankIndex++) {
            sortedRacers[rankIndex]->mSpeedTrapPosition[i] = rankIndex + 1;
        }
    }
}

void GRaceStatus::Update(float dT) {
#ifndef EA_BUILD_A124
    if (mPlayMode == kPlayMode_Racing && mRefreshBinAfterRace) {
        RefreshBinWhileInGame();
        mRefreshBinAfterRace = false;
    }
#endif

    if (mPlayMode == kPlayMode_Roaming) {
        if (mQueueBinChange) {
            EnterBin(FEDatabase->GetCareerSettings()->GetCurrentBin());
            mQueueBinChange = false;
        }
#ifndef EA_BUILD_A124
        if (mPlayMode == kPlayMode_Roaming && mWarpWhenInFreeRoam != 0) {
            if (GManager::Get().WarpToMarker(mWarpWhenInFreeRoam, false)) {
                mWarpWhenInFreeRoam = 0;
            }
        }
#endif
    }

    int num_racers = mRacerCount;
    if (mPlayMode == kPlayMode_Racing && num_racers > 0) {
        int numCharacters = 0;
        for (int i = 0; i < num_racers; i++) {
            if (GetRacerInfo(i).GetGameCharacter() != nullptr) {
                numCharacters++;
            }
        }

        float totalPct = 0.0f;
        float totalWeight = 0.0f;
        for (int i = 0; i < num_racers; i++) {
            GRacerInfo &info = GetRacerInfo(i);
            info.Update(dT);
            if (info.mhSimable != 0) {
                float weight;
                if (info.GetGameCharacter() == nullptr) {
                    weight = bMax(1.0f, (float)numCharacters);
                } else {
                    weight = 1.0f;
                }
                totalPct += weight * info.GetPctRaceComplete();
                totalWeight += weight;
            }
        }
        fAveragePercentComplete = totalPct / bMax(1.0f, totalWeight);

        CalculateRankings();

#ifndef EA_BUILD_A124
        for (int i = 0; i < num_racers; i++) {
            GetRacerInfo(i).UpdateSplits();
        }
#endif

        (MNotifyRaceTime(GetRaceTimeElapsed(), GetIsTimeLimited(), GetRaceTimeRemaining())).Post(UCrc32(0x20d60dbf));

        float elapsed = GetRaceTimeElapsed();
        int seconds = (int)elapsed;
        if (seconds > mLastSecondTickSent) {
            mLastSecondTickSent = seconds;
            (MNotifyRaceTimeSecTick(elapsed)).Post(UCrc32(0x20d60dbf));
        }

        if (GetIsTimeLimited()) {
#ifndef EA_BUILD_A124
            bool isChallenge = GRaceStatus::IsChallengeRace();
            if (isChallenge) {
                if (mPlayerPursuitInCooldown == true) {
                    if (mRaceMasterTimer.IsRunning()) {
                        mRaceMasterTimer.Stop();
                    }
                } else {
                    if (!mRaceMasterTimer.IsRunning()) {
                        mRaceMasterTimer.Start();
                    }
                }
            }
#endif

            if (!mTimeExpiredMsgSent) {
                if (GetRaceTimeRemaining() <= 0.0f) {
                    MNotifyRaceTimeExpired().Post(UCrc32(0x20d60dbf));
                    mTimeExpiredMsgSent = true;
                    for (int i = 0; i < mRacerCount; i++) {
                        GRacerInfo &info = mRacerInfo[i];
                        if (info.GetGameCharacter() == nullptr && !info.IsFinishedRacing()) {
                            info.ForceStop();
                        }
                    }
                }
            }
        }
    }

    if (mScriptWaitingForLoad) {
        bool loading = IsLoading();
        bool streaming = TheTrackStreamer.IsLoadingInProgress();
        bool copLoading = false;
        if (ICopMgr::Get() != nullptr && ICopMgr::Get()->IsCopSpawnPending()) {
            copLoading = true;
        }
        if (!loading && !streaming && !copLoading) {
            MLoadingComplete().Post(UCrc32(0x20d60dbf));
            mScriptWaitingForLoad = false;
        }
    }
}

bool GRaceStatus::CanUnspawnRoamer(const IVehicle *roamer) const {
    if (!roamer->IsActive()) {
        return true;
    }
    const GRacerInfo *info = nullptr;

    for (int onRacer = 0; onRacer < GetRacerCount(); onRacer++) {
        const GRacerInfo &racerInfo = mRacerInfo[onRacer];

        if (UTL::COM::ComparePtr(racerInfo.GetSimable(), roamer)) {
            info = &racerInfo;
            break;
        }
    }
    if (info == nullptr) {
        return true;
    }
    if (roamer->GetOffscreenTime() < 4.0f) {
        return false;
    }
    return Sim::DistanceToCamera(roamer->GetPosition()) >= 100.0f;
}

enum eVehicleCacheResult GRaceStatus::OnQueryVehicleCache(const IVehicle *removethis, const IVehicleCache *whosasking) const {
    if (mPlayMode != kPlayMode_Racing && !mVehicleCacheLocked) {
#ifndef EA_BUILD_A124
        if (UTL::COM::ComparePtr(whosasking, ICopMgr::Get()) || UTL::COM::ComparePtr(whosasking, ITrafficMgr::Get())) {
            if (!CanUnspawnRoamer(removethis)) {
                return VCR_WANT;
            }
        }
#endif
        return VCR_DONTCARE;
    }
    for (int onRacer = 0; onRacer < GetRacerCount(); onRacer++) {
        const GRacerInfo &racerInfo = mRacerInfo[onRacer];

        if (UTL::COM::ComparePtr(racerInfo.GetSimable(), removethis)) {
            return VCR_WANT;
        }
    }
    return VCR_DONTCARE;
}

void GRaceStatus::OnRemovedVehicleCache(IVehicle *ivehicle) {}

void GRaceStatus::SetRaceContext(GRace::Context context) {
    mRaceContext = context;
}

GRacerInfo &GRaceStatus::GetRacerInfo(int index) {
    return mRacerInfo[index];
}

GRacerInfo *GRaceStatus::GetRacerInfo(ISimable *isim) {
    for (int idx = 0; idx < mRacerCount; idx++) {
        GRacerInfo &info = mRacerInfo[idx];

        if (info.GetSimable() == isim) {
            return &info;
        }
    }

    return NULL;
}

GRacerInfo *GRaceStatus::GetWinningPlayerInfo() {
    IPlayer *player = IPlayer::First(PLAYER_ALL);

    while (player != NULL) {
        ISimable *sim = player->GetSimable();
        GRacerInfo *info = GetRacerInfo(sim);

        if (info != NULL && info->IsFinishedRacing() && info->GetRanking() == 1) {
            return info;
        }

        player = player->Next(PLAYER_ALL);
    }

    return NULL;
}

int GRaceStatus::GetRacerCount() const {
    return mRacerCount;
}

void GRaceStatus::StartMasterTimer() {
    float startTime = mRaceParms ? mRaceParms->GetStartTime() : 0.0f;
    mRaceMasterTimer.Reset(startTime);
    mRaceMasterTimer.Start();
}

void GRaceStatus::StopMasterTimer() {
    mRaceMasterTimer.Stop();
}

float GRaceStatus::GetRaceTimeElapsed() const {
    if (mRaceParms == nullptr) {
        return 0.0f;
    }
    return mRaceMasterTimer.GetTime();
}

float GRaceStatus::GetRaceTimeRemaining() const {
    if (mRaceParms != nullptr) {
        float total = mRaceParms->GetTimeLimit() + mBonusTime;
        if (total > 0.0f) {
            float remaining = total - mRaceMasterTimer.GetTime();
            if (remaining < 0.0f) {
                remaining = 0.0f;
            }
            return remaining;
        }
    }
    return 0.0f;
}

// STRIPPED en GameCube (Range 0xFFFFFFFF en el DWARF); la A124 la tiene y SLUS no
void GRaceStatus::SkipToEndOfRaceForRacer(ISimable *thisPlayer, int index, float time) {
    if (GetPlayMode() == kPlayMode_Racing) {
        GRaceParameters *parms = GetRaceParameters();
        float lapTime = time / parms->GetNumLaps();
        for (int onLap = 0; onLap < parms->GetNumLaps(); onLap++) {
            SetLapTime(onLap, index, lapTime);
            for (int onCheckpoint = 0; onCheckpoint < GetCheckpointCount(); onCheckpoint++) {
                (MTriggerEnter(GCollectionKey(this->GetCheckpoint(onCheckpoint)), thisPlayer->GetInstanceHandle())).Post(UCrc32(0x20d60dbf));
            }
        }
    }
}

// STRIPPED en GameCube (Range 0xFFFFFFFF en el DWARF); la A124 la tiene y SLUS no
void GRaceStatus::SkipToEndOfRaceInPlace(int place, float finishTime) {
    int playerIndex;

    for (int onRacer = 0; onRacer < mRacerCount; onRacer++) {
        if (onRacer == place - 1) {
            playerIndex = onRacer;
            break;
        }
        GRacerInfo &info = mRacerInfo[onRacer];
        ISimable *simable = info.GetSimable();
        if (simable) {
            if (simable->IsPlayer()) {
                place++;
            } else {
                SkipToEndOfRaceForRacer(simable, onRacer, finishTime - 0.9f);
            }
        }
    }
    SkipToEndOfRaceForRacer(IPlayer::First(PLAYER_LOCAL)->GetSimable(), playerIndex, finishTime);
}

void GRaceStatus::ClearRacers() {
    const IVehicle::List &list = IVehicle::GetList(VEHICLE_RACERS);

    for (IVehicle::List::const_iterator iter = list.begin(); iter != list.end(); ++iter) {
        IVehicle *vehicle = *iter;
        ISimable *simable = vehicle->GetSimable();

        if (simable && !simable->IsPlayer()) {
            simable->Kill();
        }
    }

    mRacerCount = 0;
}

GRacerInfo &GRaceStatus::AddSimablePlayer(ISimable *isim) {
    GRacerInfo &info = mRacerInfo[mRacerCount++];

    info.ClearAll();

    info.SetSimable(isim);
    info.SetIndex(mRacerCount - 1);

#ifndef EA_BUILD_A124 // la A124 no tiene pantalla partida aqui
    if (Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) {
        if (mRacerCount == 1) {
            info.SetName(GetLocalizedString(0x7B070984));
        } else {
            info.SetName(GetLocalizedString(0x7B070985));
        }
    } else
#endif
    {
        UserProfile *userProfile = FEDatabase->GetUserProfile(isim->GetPlayer()->GetSettingsIndex());

        if (userProfile != NULL) {
            info.SetName(userProfile->GetProfileName());
        } else {
            info.SetName(GetLocalizedString(0xF760EABE));
        }
    }

    return info;
}

void GRaceStatus::AddRacer(GRuntimeInstance *racer) {
    GRacerInfo *info = &mRacerInfo[mRacerCount++];

    info->ClearAll();

    info->mGameCharacter = (GCharacter *)racer;

    info->SetIndex(mRacerCount - 1);

    if (!info->ChooseBossName() && !info->ChooseRacerName()) {
        info->ChooseRandomName();
    }
}

void GRaceStatus::SetRaceActivity(GActivity *activity) {
    mRaceParms = GRaceDatabase::Get().GetRaceFromActivity(activity);
}

void GRaceStatus::EnableBarriers() {
    if (mRaceParms != nullptr) {

        for (unsigned int onBarrier = 0; onBarrier < mRaceParms->GetBarrierCount(); onBarrier++) {

            const char *barrierName = mRaceParms->GetBarrierName(onBarrier);
            bool flipFlag = mRaceParms->GetBarrierIsFlipped(onBarrier);

            EnableBarrierSceneryGroup(barrierName, flipFlag);
        }

        WCollisionAssets::Get().SetExclusionFlags();
        WRoadNetwork::Get().ResolveBarriers();
    }
}

void GRaceStatus::DisableBarriers() {
    RedoTopologyAndSceneryGroups();
    WRoadNetwork::Get().ResetBarriers();
    WRoadNetwork::Get().ResetRaceSegments();
}

void GRaceStatus::SetRoaming() {
    bool isDDayRace = false;
    if (mRaceParms != nullptr) {
        isDDayRace = bStrCmp(mRaceParms->GetEventID(), "16.2.1") == 0;
    }

    if (mRaceParms == nullptr || mRaceParms->GetGameplayObj()->PostRaceActivity() == 0) {
        g_pEAXSound->StartNewGamePlay();
    }

    mPlayMode = kPlayMode_Roaming;
    SetRaceContext(GRace::kRaceContext_Career);
    mRaceParms = nullptr;
    mIsLoading = false;

#ifndef EA_PLATFORM_PLAYSTATION2
    WRoadNetwork::Get().ResetShortcuts();
#endif

    for (IPlayer *player = IPlayer::First(PLAYER_ALL); player != nullptr; player = player->Next(PLAYER_ALL)) {
        if (player->InGameBreaker()) {
            player->ToggleGameBreaker();
        }
        ISimable *simable = player->GetSimable();
        if (simable != nullptr) {
            IVehicle *ivehicle = nullptr;
            if (simable->QueryInterface(&ivehicle)) {
                ivehicle->ForceStopOff(ivehicle->GetForceStop());
#ifndef EA_BUILD_A124
                IVehicleAI *vehicleAI = nullptr;
                if (ivehicle->QueryInterface(&vehicleAI)) {
                    if (vehicleAI->GetPursuit() == nullptr) {
                        ICopMgr::Get()->LockoutCops(true);
                    }
                }
#endif
            }
        }
    }

    if (!isDDayRace) {
#ifdef EA_BUILD_A124
        new EReloadHud();
#else
        bool restartPending = GManager::Get().GetHasPendingRestartEvent();
        if (!restartPending) {
            new EReloadHud();
        }
#endif
    }
    new EAutoSave();

    bool dDay = false;
    if (mRaceParms != nullptr && mRaceParms->GetIsDDayRace()) {
        dDay = true;
    }
    if (!dDay) {
        SetOverRideRainIntensity(0.0f);
#ifdef EA_BUILD_A124
        // La A124 vuelve tambien a la hora del dia por defecto al salir de la carrera.
        SetCurrentTimeOfDay(0.5f);
#endif
    }

#ifndef EA_BUILD_A124
    GManager::Get().SpawnAllLoadedSectionIcons();
    ICopMgr::mDisableCops = 0;
#endif
}

void GRaceStatus::SetRacing() {
    mPlayMode = kPlayMode_Racing;
    ClearTimes();

    for (IPlayer *player = IPlayer::First(PLAYER_ALL); player != NULL; player = player->Next(PLAYER_ALL)) {
        if (player->InGameBreaker()) {
            player->ToggleGameBreaker();
        }
    }

    mNumTollbooths = 0;
    GObjectIterator<GTrigger> iter(0x800);
    while (iter.IsValid()) {
        GTrigger *trigger = iter.GetInstance();
        if (trigger->IsWorldTriggerEnabled()) {
            mNumTollbooths = mNumTollbooths + 1;
        }
        iter.Advance();
    }

#ifndef EA_BUILD_A124
    if ((mRaceParms != NULL && mRaceParms->GetIsDDayRace() && bStrCmp(mRaceParms->GetEventID(), "16.1.0") != 0) || FEDatabase->IsFinalEpicChase()) {
        new EAutoSave();
    }
#endif

    new EReloadHud();

#ifndef EA_BUILD_A124
    mCaluclatedAdaptiveGain = false;
#endif
}

void GRaceStatus::NotifyScriptWhenLoaded() {
    bool racersLoading = IsLoading();
    bool trackLoading = TheTrackStreamer.IsLoadingInProgress();
    bool copsSpawning = ICopMgr::Get() != nullptr && ICopMgr::Get()->IsCopSpawnPending();

    if (trackLoading && !racersLoading && !copsSpawning) {
#ifndef EA_BUILD_A124
    // la alpha 124 no tiene esta funcion: su declaracion ya va bajo la misma guarda.
        if (!TheTrackStreamer.IsFarLoadingInProgress()) {
            trackLoading = false;
        }
#endif
    }

    if (racersLoading || trackLoading || copsSpawning) {
        new EFadeScreenOn(
#ifndef EA_BUILD_A124
            false
#endif
        );
    }

    mScriptWaitingForLoad = true;
}

void GRaceStatus::AddAvailableEventToMap(GRuntimeInstance *trigger, GRuntimeInstance *event) {}

void GRaceStatus::AddSpeedTrapToMap(GRuntimeInstance *trigger) {}

void GRaceStatus::AwardBonusTime(float seconds) {
#ifdef EA_BUILD_A124 // la A124 suma el tiempo antes de avisar a los corredores
    mBonusTime += seconds;
#endif
    for (int onRacer = 0; onRacer < mRacerCount; onRacer++) {

        GRacerInfo &info = mRacerInfo[onRacer];

        info.NotifyTollboothCrossed(GRaceStatus::Get().GetRaceTimeRemaining());
    }

#ifndef EA_BUILD_A124
    mBonusTime += seconds;
#endif
}

void GRaceStatus::ClearCheckpoints() {
    mCheckpoints.clear();
}

void GRaceStatus::AddCheckpoint(GRuntimeInstance *trigger) {
    GTrigger *gtrigger = (GTrigger *) trigger;

    mCheckpoints.push_back(gtrigger);
}

void GRaceStatus::SetNextCheckpointPos(GRuntimeInstance *trigger) {
    mNextCheckpoint = static_cast<GTrigger *>(trigger);

    bool visible = false;
    if (mRaceParms != NULL) {
        visible = mRaceParms->GetCheckpointsVisible();
    }

    if (trigger != NULL && visible) {
        if (mCheckpointModel == NULL) {
            mCheckpointModel = new WorldModel(STRINGHASH_CHECKPOINT_BEAM, NULL, false);
        }

        UMath::Vector3 pos;
        static_cast<GTrigger *>(trigger)->GetPosition(pos);

        bVector3 translation;
        bFill(&translation, pos.z, -pos.x, pos.y);
        bMatrix4 mat;
        bIdentity(&mat);
        bCopy(&mat.v3, &translation, 1.0f);
        mat.v0.x = 1.0f;
        mat.v1.y = 1.0f;
        mCheckpointModel->SetMatrix(&mat);

        if (mCheckpointEmitter != NULL) {
            mCheckpointEmitter->SetLocalWorld(&mat);
        }
    } else {
        if (mCheckpointModel != NULL) {
            delete mCheckpointModel;
        }
        EmitterGroup *emitter = mCheckpointEmitter;
        mCheckpointModel = NULL;
        if (emitter != NULL) {
            emitter->UnSubscribe();
            if (mCheckpointEmitter != NULL) {
                delete mCheckpointEmitter;
            }
            mCheckpointEmitter = NULL;
        }
    }
}

float GRaceStatus::DetermineRaceSegmentLength(const UMath::Vector4 *positions, const UMath::Vector4 *directions, int start, int end) {
    WRoadNav nav;

    nav.SetDecisionFilter(true);
    nav.SetNavType(WRoadNav::kTypeDirection);
    const bool force_centre_lane = true;
    const float direction_weight = 1.0f;

    nav.SetPathType(WRoadNav::kPathRaceRoute);

    mRaceParms->GetNumCheckpoints();

    float length = UMath::Distance(UMath::Vector4To3(positions[start]), UMath::Vector4To3(positions[end]));

    nav.InitAtPoint(UMath::Vector4To3(positions[start]), UMath::Vector4To3(directions[start]), force_centre_lane, direction_weight);

    if (nav.IsValid()) {
        float travelled = 0.0f;

        if (start == end) {
            short first_segment = nav.GetSegmentInd();
            float segment_length = WRoadNetwork::Get().GetSegment(first_segment)->GetLength();

            do {
                float step = bMax(0.01f, segment_length * (1.0f - nav.GetSegmentTime()));
                nav.IncNavPosition(step, UMath::Vector4To3(directions[start]), 0.0f);
                travelled += step;
            } while (first_segment == nav.GetSegmentInd());
        }

        UTL::Std::set<PathSegment, _type_ID_PATH_SET> path_set;
        char shortcut_allowed[32];
        WRoadNetwork &road_network = WRoadNetwork::Get();
        bMemSet(shortcut_allowed, 1, sizeof(shortcut_allowed));

        bool no_more_shortcuts = true;
        bool found;

        do {
            nav.SetNumPathSegments(0);
            nav.SetNavType(WRoadNav::kTypeDirection);
            nav.FindPathNow(&UMath::Vector4To3(positions[end]), &UMath::Vector4To3(directions[end]), shortcut_allowed);
            found = nav.GetNavType() == WRoadNav::kTypePath;
            if (found) {
                road_network.AddRaceSegments(&nav);
                length = nav.GetPathDistanceRemaining();
                unsigned char shortcut = nav.FirstShortcutInPath();
                no_more_shortcuts = shortcut == 0xFF;
                if (!no_more_shortcuts) {
                    shortcut_allowed[shortcut] = 0;
                }

                PathSegment path_segment;
                path_segment.mDistance = length;
                int num_path_segments = nav.GetNumPathSegments();
                for (int i = 0; i < num_path_segments; i++) {
                    WRoadSegment *segment = WRoadNetwork::Get().GetSegmentNonConst(nav.GetPathSegment(i));
                    if (!segment->IsDecision()) {
                        path_segment.mRoads.insert(segment->fRoadID);
                    }
                }
                path_set.insert(path_segment);
            }
        } while (found && !no_more_shortcuts);

        int num_paths = no_more_shortcuts ? path_set.size() : 0;
        length += travelled;
        if (num_paths == 0) {
            bRaceRouteError = true;
        }

        if (num_paths > 1) {
            PathSegment *segments[32];
            int i = 0;
            UTL::Std::set<PathSegment, _type_ID_PATH_SET>::iterator it = path_set.begin();
            while (it != path_set.end()) {
                segments[i++] = const_cast<PathSegment *>(&*it);
                ++it;
            }

            for (i = 1; i < num_paths; i++) {
                PathSegment *previous = segments[i - 1];
                PathSegment *current = segments[i];
                const UTL::Std::set<short, _type_ID_ROAD_SET> &previous_roads = previous->mRoads;
                const UTL::Std::set<short, _type_ID_ROAD_SET> &current_roads = current->mRoads;
                UTL::Std::set<short, _type_ID_ROAD_SET> unique_roads;

                std::set_difference(previous_roads.begin(), previous_roads.end(), current_roads.begin(), current_roads.end(),
                                     std::inserter(unique_roads, unique_roads.begin()));

                float road_length = 0.0f;
                float distance_delta = current->mDistance - previous->mDistance;
                UTL::Std::set<short, _type_ID_ROAD_SET>::iterator road_it = unique_roads.begin();
                while (road_it != unique_roads.end()) {
                    short road_number = *road_it;
                    const WRoad *road = WRoadNetwork::Get().GetRoad(road_number);
                    road_length += road->GetLength();
                    ++road_it;
                }

                if (road_length > 0.0f) {
                    float scale = (distance_delta + road_length) / road_length;
                    road_it = unique_roads.begin();
                    while (road_it != unique_roads.end()) {
                        WRoad *road = WRoadNetwork::Get().GetRoadNonConst(*road_it);
                        road->nScale = static_cast<unsigned short>(bFloatToInt(scale * 65536.0f) >> 8);
                        ++road_it;
                    }
                }
            }
        }
    } else {
        bRaceRouteError = true;
        mRaceParms->GetNumCheckpoints();
    }

    return length;
}

void GRaceStatus::DetermineRaceLength() {
    nSpeedTraps = 0;
    fRaceLength = 0.0f;
    fFirstLapLength = 0.0f;
    fSubsequentLapLength = 0.0f;
    bMemSet(mSegmentLengths, 0, sizeof(mSegmentLengths));
    bRaceRouteError = false;
    WRoadNetwork::Get().ResolveShortcuts();

    GRaceParameters *parms = mRaceParms;
    if (parms == NULL || !parms->HasFinishLine()) {
        return;
    }

    WRoadNetwork::Get().SetRaceFilterValid(true);

    int numCheckpoints = parms->GetNumCheckpoints();
    int numPoints = numCheckpoints + 2;
    UMath::Vector4 positions[18];
    UMath::Vector4 directions[18];

    parms->GetStartPosition(UMath::Vector4To3(positions[0]));
    parms->GetStartDirection(UMath::Vector4To3(directions[0]));
    parms->GetFinishPosition(UMath::Vector4To3(positions[numPoints - 1]));
    parms->GetFinishDirection(UMath::Vector4To3(directions[numPoints - 1]));

    for (int i = 0; i < numCheckpoints; i++) {
        parms->GetCheckpointPosition(i, UMath::Vector4To3(positions[i + 1]));
        parms->GetCheckpointDirection(i, UMath::Vector4To3(directions[i + 1]));
    }

    float total = 0.0f;
    bool looping = parms->GetIsLoopingRace();
    int numSegments = numPoints;
    if (!looping) {
        numSegments--;
    }
    int wrap = numPoints - 1;
    for (int i = 0; i < numSegments; i++) {
        int end = (i % (numPoints - 1)) + 1;
        float segLen = DetermineRaceSegmentLength(positions, directions, i, end);
        total += segLen;
        mSegmentLengths[i] = segLen;
    }

    if (!looping) {
        fRaceLength = total;
        fFirstLapLength = total;
        fSubsequentLapLength = total;
    } else {
        fSubsequentLapLength = total - mSegmentLengths[0];
        fFirstLapLength = total - mSegmentLengths[wrap];
        fRaceLength = fSubsequentLapLength * static_cast<float>(parms->GetNumLaps() - 1) + fFirstLapLength;
    }

    WRoadNav nav;
    const bool force_centre_lane = true;
    const float direction_weight = 1.0f;
    nav.SetDecisionFilter(true);
    nav.SetNavType(WRoadNav::kTypeDirection);
    nav.SetPathType(WRoadNav::kPathRaceRoute);
    nav.InitAtPoint(UMath::Vector4To3(positions[wrap]), UMath::Vector4To3(directions[wrap]), force_centre_lane, direction_weight);
    if (nav.IsValid()) {
        for (int i = 0; i < 100; i++) {
            nav.IncNavPosition(1.0f, UMath::Vector3::kZero, 0.0f);
            WRoadSegment *segment = WRoadNetwork::Get().GetSegmentNonConst(nav.GetSegmentInd());
            unsigned short flags = segment->fFlags;
            segment->fFlags = flags | kRoadSegmentInRace;
            if (nav.GetNodeInd() == 1) {
                segment->fFlags = flags | kRoadSegmentInRace | kRoadSegmentRaceRouteForward;
            } else {
                segment->fFlags = (flags | kRoadSegmentInRace) & ~kRoadSegmentRaceRouteForward;
            }
        }
    }

    GObjectIterator<GTrigger> iter(0x100);
    int trapCount = 0;
    while (iter.IsValid()) {
        GTrigger *trigger = iter.GetInstance();
        if (!trigger->OpenWorldSpeedTrap()) {
            aSpeedTraps[trapCount] = trigger;
            trapCount++;
        }
        iter.Advance();
    }
    nSpeedTraps = trapCount;
}

int NotNumeric(char c) {
    if (c == '-') {
        return 0;
    }
    if (c == '.') {
        return 0;
    }
    if (c == bClamp((int)c, (int)'0', (int)'9')) {
        return 0;
    }
    return 1;
}

int SplitChars(char *in, char ***array, int (*func)(char)) {
    int i;
    int count;
    char *temp;
    char *end;

    while (*in != 0 && func(*in) != 0) {
        in++;
    }

    temp = in;
    count = 0;

    while (*temp != 0) {
        if (func(*temp) == 0) {
            count++;
            while (*temp != 0 && func(*temp) == 0) {
                temp++;
            }
        } else {
            temp++;
        }
    }

#ifdef MILESTONE_BUILD
    // La A124 etiquetaba esta reserva; la version final usa el new[] plano.
    *array = new ("GRaceStatus SplitChars()", 0) char *[count];
#else
    *array = new char *[count];
#endif

    temp = in;

    for (i = 0; i < count; i++) {
        (*array)[i] = temp;

        while (*temp != 0 && func(*temp) == 0) {
            temp++;
        }

        end = temp;

        while (*temp != 0 && func(*temp) != 0) {
            temp++;
        }

        *end = 0;
    }

    return count;
}

float ParseFloat(char *word) {
    float sign = 1.0f;
    int i = (*word == '-') ? 1 : 0;
    float total = 0.0f;
    float decimal = 1.0f;
    bool after_decimal = false;

    for (; word[i] != '\0'; i++) {
        char c = word[i];
        if (c == '.') {
            after_decimal = true;
            continue;
        }
        if (after_decimal) {
            decimal *= 0.1f;
        } else {
            total *= 10.0f;
        }
        total = decimal * static_cast<float>(bClamp(c - '0', 0, 9)) + total;
    }
    return total;
}

int ParseArray(const char *string, float *data, int max) {
    int len = bStrLen(string) + 1;
    char *copy = new ("GRaceStatus ParseArray()", 0) char[len];
    bMemCpy(copy, string, len);

    char **tokens;
    int count = SplitChars(copy, &tokens, NotNumeric);

    if (count > max) {
        float *parsed = new ("GRaceStatus ParseArray()", 0) float[count];
        {
            int i = 0;
            while (i < count) {
                parsed[i] = ParseFloat(tokens[i]);
                i++;
            }
        }
        Table table(parsed, count, 0.0f, static_cast<float>(max - 1));
        {
            int i = 0;
            while (i < max) {
                data[i] = table.GetValue(static_cast<float>(i));
                i++;
            }
        }
        if (parsed != NULL) {
            delete[] parsed;
        }
        count = max;
    } else {
        {
            int i = 0;
            while (i < count) {
                data[i] = ParseFloat(tokens[i]);
                i++;
            }
        }
        if (count == 1) {
            data[1] = data[0];
            count = 2;
        }
    }

    if (tokens != NULL) {
        delete[] tokens;
    }
    if (copy != NULL) {
        delete[] copy;
    }
    return count;
}

void GRaceStatus::ParseCatchUpData(const char *skill, const char *spread) {
    nCatchUpSkillEntries = ParseArray(skill, aCatchUpSkillData, 11);
    nCatchUpSpreadEntries = ParseArray(spread, aCatchUpSpreadData, 11);
}

float GRaceStatus::GetAdaptiveDifficutly() const {
    if (mRaceContext == kRaceContext_Career) {
        return fCatchUpAdaptiveBonus;
    }
    return 0.0f;
}

void GRaceStatus::SyncronizeAdaptiveBonus() {
    fCatchUpAdaptiveBonus = bClamp((float)FEDatabase->GetCareerSettings()->AdaptiveDifficulty * 0.00003051851f, -1.0f, 1.0f);
}

#ifdef EA_BUILD_A124
void GRaceStatus::UpdateAdaptiveBonus(const GRacerInfo *racer_info) {
    if (mRaceContext != GRace::kRaceContext_Career || Sim::GetUserMode() != 0) {
        return;
    }

    int num_racers = GetRacerCount();
    if (num_racers < 2) {
        return;
    }

    if (racer_info->GetRanking() != 1) {
        return;
    }

    if (racer_info->GetGameCharacter() == nullptr) {
        float max_pct_complete = 0.0f;
        for (int i = 0; i < num_racers; i++) {
            GRacerInfo *info = &GetRacerInfo(i);
            if (info != racer_info && !info->IsFinishedRacing()) {
                max_pct_complete = bMax(max_pct_complete, info->GetPctRaceComplete());
            }
        }

        float percent_remaining = (100.0f - max_pct_complete) * 0.01f;
        float win_margin = GetRaceLength() * percent_remaining;
        if (win_margin > 0.0f) {
            float t = UMath::Ramp(win_margin, 0.0f, 300.0f);
            fCatchUpAdaptiveBonus = bClamp(fCatchUpAdaptiveBonus + UMath::Lerp(0.0f, 0.2f, t), -1.0f, 1.0f);
        }
    } else {
        for (int i = 0; i < num_racers; i++) {
            GRacerInfo *info = &GetRacerInfo(i);
            if (info->GetGameCharacter() == nullptr && !info->IsFinishedRacing()) {
                float percent_remaining = (100.0f - info->GetPctRaceComplete()) * 0.01f;
                float lose_margin = GetRaceLength() * percent_remaining;
                if (lose_margin > 0.0f) {
                    float t = UMath::Ramp(lose_margin, 0.0f, 300.0f);
                    fCatchUpAdaptiveBonus = bClamp(fCatchUpAdaptiveBonus + UMath::Lerp(0.0f, -0.2f, t), -1.0f, 1.0f);
                }
            }
        }
    }

    FEDatabase->GetCareerSettings()->SetAdaptiveDifficulty(fCatchUpAdaptiveBonus);
}
#endif

void GRaceStatus::UpdateAdaptiveDifficulty(eAdaptiveGainReason reason, ISimable *who) {
#ifndef EA_BUILD_A124
    if (mCaluclatedAdaptiveGain) {
        return;
    }
#endif

    if (mRaceContext != GRace::kRaceContext_Career) {
        return;
    }

    if (GetRacerCount() <= 1) {
        return;
    }

    if (Sim::GetUserMode() != 0) {
        return;
    }

    if (mPlayMode != kPlayMode_Racing) {
        return;
    }

    if (mRaceParms != nullptr && mRaceParms->GetNoPostRaceScreen()) {
        return;
    }

    if (GetRaceLength() <= 0.0f) {
        return;
    }

    bool update = false;
    GRacerInfo *winning_player = nullptr;
    GRacerInfo *winning_ai = nullptr;
    GRacerInfo *eliminated_player = nullptr;

    float difficulty = fCatchUpAdaptiveBonus;

    const int num_racers = GetRacerCount();
    for (int i = 0; i < num_racers; i++) {
        GRacerInfo &info = GetRacerInfo(i);
        if (info.GetGameCharacter() == nullptr) {
            if (info.GetIsBusted()) {
                return;
            }

            if (info.IsFinishedRacing() && info.GetRanking() == 1) {
                winning_player = &info;
            } else if (GetRaceType() == GRace::kRaceType_Knockout && info.GetIsKnockedOut()) {
                eliminated_player = &info;
            }
        } else {
            if (info.IsFinishedRacing() && info.GetRanking() == 1) {
                winning_ai = &info;
            }
        }
    }

    if (reason == kAdaptiveGain_FromEngineBlown || reason == kAdaptiveGain_FromVehicleDestroyed) {
        if (who == nullptr || !who->IsPlayer()) {
            return;
        }

        float percent_human_complete = 0.0f;
        float percent_ai_complete = 0.0f;
        for (int i = 0; i < num_racers; i++) {
            GRacerInfo &info = GetRacerInfo(i);
            if (info.GetIsKnockedOut()) {
                continue;
            }
            if (info.GetGameCharacter() == nullptr) {
                percent_human_complete = UMath::Max(percent_human_complete, info.GetPctRaceComplete());
            } else {
                percent_ai_complete = UMath::Max(percent_ai_complete, info.GetPctRaceComplete());
            }
        }

        if (percent_ai_complete > percent_human_complete && percent_human_complete > 0.0f) {
            float race_length_percent = GetRaceLength() * 0.01f;
            float lose_margin = (percent_ai_complete - percent_human_complete) * race_length_percent;
            float t = UMath::Ramp(lose_margin, 0.0f, 300.0f);
            float bonus = UMath::Lerp(0.0f, -0.4f, t);
            difficulty = bClamp(difficulty + bonus * percent_human_complete * 0.01f, -1.0f, 1.0f);
        }

        update = true;
    } else if (reason == kAdaptiveGain_FromRestart) {
        float percent_human_complete = 0.0f;
        float percent_ai_complete = 0.0f;
        for (int i = 0; i < num_racers; i++) {
            GRacerInfo &info = GetRacerInfo(i);
            if (info.GetIsKnockedOut() || info.GetIsTotalled() || info.GetIsEngineBlown()) {
                continue;
            }
            if (info.GetGameCharacter() == nullptr) {
                percent_human_complete = UMath::Max(percent_human_complete, info.GetPctRaceComplete());
            } else {
                percent_ai_complete = UMath::Max(percent_ai_complete, info.GetPctRaceComplete());
            }
        }

        if (percent_ai_complete > percent_human_complete && percent_human_complete > 0.0f) {
            float race_length_percent = GetRaceLength() * 0.01f;
            float lose_margin = (percent_ai_complete - percent_human_complete) * race_length_percent;
            float t = UMath::Ramp(lose_margin, 0.0f, 300.0f);
            float bonus = UMath::Lerp(0.0f, -0.4f, t);
            difficulty = bClamp(difficulty + bonus * percent_human_complete * 0.01f, -1.0f, 1.0f);
        }

        update = true;
    } else if (GetRaceType() == GRace::kRaceType_SpeedTrap) {
        float player_points = 0.0f;
        float ai_points = 0.0f;
        for (int i = 0; i < num_racers; i++) {
            if (!GetRacerInfo(i).IsFinishedRacing()) {
                return;
            }

            if (GetRacerInfo(i).GetGameCharacter() != nullptr) {
                ai_points = UMath::Max(ai_points, GetRacerInfo(i).GetPointTotal());
            } else {
                player_points = GetRacerInfo(i).GetPointTotal();
            }
        }

        float total_points = UMath::Max(player_points, ai_points);
        if (total_points > 0.0f && ai_points > 0.0f && player_points > 0.0f) {
            float point_spread_ratio = (player_points - ai_points) / total_points;
            if (point_spread_ratio > 0.0f) {
                float win_margin = point_spread_ratio;
                float t = UMath::Ramp(win_margin, 0.05f, 0.2f);
                float bonus = UMath::Lerp(0.0f, 0.2f, t);
                update = true;
                difficulty = bClamp(difficulty + bonus, -1.0f, 1.0f);
            } else {
                float lose_margin = -point_spread_ratio;
                float t = UMath::Ramp(lose_margin, 0.0f, 0.2f);
                float bonus = UMath::Lerp(0.0f, -0.2f, t);
                update = true;
                difficulty = bClamp(difficulty + bonus, -1.0f, 1.0f);
            }
        }
    } else if (winning_player != nullptr) {
        float max_pct_complete = 0.0f;
        for (int i = 0; i < num_racers; i++) {
            GRacerInfo &info = GetRacerInfo(i);
            if (&info == winning_player || info.IsFinishedRacing() || info.GetIsTotalled() || info.GetIsEngineBlown()) {
                continue;
            }
            max_pct_complete = bMax(max_pct_complete, info.GetPctRaceComplete());
        }

        float percent_remaining = (100.0f - max_pct_complete) * 0.01f;
        float win_margin = GetRaceLength() * percent_remaining;
        if (win_margin > 0.0f && max_pct_complete > 0.0f) {
            float t = UMath::Ramp(win_margin, 200.0f, 750.0f);
            float bonus = UMath::Lerp(0.0f, 0.4f, t);
            difficulty = bClamp(difficulty + bonus, -1.0f, 1.0f);
        }

        update = true;
    } else if (winning_ai != nullptr) {
        for (int i = 0; i < num_racers; i++) {
            GRacerInfo &info = GetRacerInfo(i);
            if (info.GetGameCharacter() != nullptr || info.IsFinishedRacing()) {
                continue;
            }

            float percent_remaining = (100.0f - info.GetPctRaceComplete()) * 0.01f;
            float lose_margin = GetRaceLength() * percent_remaining;
            if (lose_margin > 0.0f) {
                float t = UMath::Ramp(lose_margin, 0.0f, 300.0f);
                float bonus = UMath::Lerp(-0.1f, -0.4f, t);
                difficulty = bClamp(difficulty + bonus * info.GetPctRaceComplete() * 0.01f, -1.0f, 1.0f);
                update = true;
            }
        }
    } else if (eliminated_player != nullptr) {
        const int num_laps = bMax(mRaceParms->GetNumLaps(), 1);
        float distance_remaining = (100.0f - eliminated_player->GetPctRaceComplete()) * 0.01f;
        float lose_margin = UMath::Mod(GetRaceLength() * distance_remaining, GetRaceLength() / static_cast<float>(num_laps));
        if (lose_margin > 0.0f) {
            float t = UMath::Ramp(lose_margin, 0.0f, 300.0f);
            float bonus = UMath::Lerp(-0.1f, -0.4f, t);
            difficulty = bClamp(difficulty + bonus * eliminated_player->GetPctRaceComplete() * 0.01f, -1.0f, 1.0f);
            update = true;
        }
    }

    if (update) {
#ifndef EA_BUILD_A124
        mCaluclatedAdaptiveGain = true;
#endif
        FEDatabase->GetCareerSettings()->SetAdaptiveDifficulty(difficulty);
        fCatchUpAdaptiveBonus = difficulty;
    }
}

bool GRaceStatus::ComputeCatchUpSkill(GRacerInfo *racer_info, PidError *pid, float *output, float *skill
#ifndef EA_BUILD_A124
                                      ,
                                      bool off_world
#endif
) {
    float glue_level = 0.5f;
    bool is_boss = false;
    bool use_race_override = false;
#ifndef EA_BUILD_A124
    if (off_world) {
        glue_level = 1.0f;
    } else
#endif
    {
        switch (GetRaceContext()) {
        case GRace::kRaceContext_Career: {
            float base_level = glue_level;
            glue_level = UMath::Clamp((GetAdaptiveDifficutly() + 1.0f) * base_level, 0.0f, 1.0f);
            if (mRaceParms != NULL) {
                if (mRaceParms->GetCatchUpOverride()) {
                    use_race_override = true;
                }
                if (mRaceParms->GetIsBossRace()) {
                    is_boss = true;
                    glue_level = UMath::Lerp(glue_level, 1.0f, base_level);
                }
            }
            break;
        }
        case GRace::kRaceContext_QuickRace:
            if (mRaceParms == NULL || !mRaceParms->GetCatchUp()) {
                return false;
            }
            glue_level = Tweak_QuickRaceGlue[mRaceParms->GetDifficulty()];
            break;
        default:
            return false;
        }
    }

    float percent_complete = racer_info->GetPctRaceComplete();
    float glue_skill = 0.0f;
    float glue_spread = 0.0f;
    float glue_integral = 0.0f;
    float glue_derivative = 0.0f;

    float error_integral = pid->GetErrorIntegral();
    float error_derivative = pid->GetErrorDerivative();
    glue_integral = error_integral;
    glue_derivative = error_derivative;

    if (!use_race_override) {
        glue_spread = UMath::Lerp(Tweak_GlueSpreadTable_Low.GetValue(percent_complete), Tweak_GlueSpreadTable_High.GetValue(percent_complete), glue_level);
        glue_skill = UMath::Lerp(Tweak_GlueStrengthTable_Low.GetValue(percent_complete), Tweak_GlueStrengthTable_High.GetValue(percent_complete), glue_level);
        glue_integral *= 0.00005f;
        glue_derivative *= 0.01f;
    } else {
        glue_skill = Table(aCatchUpSkillData, nCatchUpSkillEntries, 0.0f, 100.0f).GetValue(percent_complete);
        glue_spread = Table(aCatchUpSpreadData, nCatchUpSpreadEntries, 0.0f, 100.0f).GetValue(percent_complete);
        glue_integral *= fCatchUpIntegral;
        glue_derivative *= fCatchUpDerivative;
    }

    float glue_p = 2.0f / bMax(100.0f, glue_spread);
    float glue_error = pid->GetError();
    float glue_output = bClamp(glue_p * glue_error + glue_integral + glue_derivative, -1.0f, 1.0f);
    *skill = glue_skill * glue_output;
    if (is_boss) {
        glue_output = 1.0f;
    }
    *output = glue_output;
    return true;
}

void GRaceStatus::MakeDefaultCatchUpData() {
    fCatchUpIntegral = 0.00005f;
    fCatchUpDerivative = 0.001f;
    nCatchUpSkillEntries = 2;
    aCatchUpSkillData[1] = 0.2f;
    aCatchUpSkillData[0] = 0.2f;
    nCatchUpSpreadEntries = 2;
    aCatchUpSpreadData[1] = 1000.0f;
    aCatchUpSpreadData[0] = 1000.0f;
}

void GRaceStatus::MakeCatchUpData() {
    GRaceParameters *race_parameters = GetRaceParameters();

    if (race_parameters != NULL) {
        fCatchUpIntegral = race_parameters->GetCatchUpIntegral();
        fCatchUpDerivative = race_parameters->GetCatchUpDerivative();
        ParseCatchUpData(race_parameters->GetCatchUpSkill(), race_parameters->GetCatchUpSpread());
    } else {
        MakeDefaultCatchUpData();
    }
}

// STRIPPED en GameCube (Range 0xFFFFFFFF en el DWARF); la A124 la tiene y SLUS no
int GRaceStatus::GetCheckpointCount() {
    return mCheckpoints.size();
}

// STRIPPED en GameCube (Range 0xFFFFFFFF en el DWARF); la A124 la tiene y SLUS no
GTrigger *GRaceStatus::GetCheckpoint(int index) {
    return mCheckpoints[index];
}

void GRaceStatus::ClearTimes() {
    bMemSet(mLapTimes, 0, sizeof(mLapTimes));
    bMemSet(mCheckTimes, 0, sizeof(mCheckTimes));

    float startTime = mRaceParms ? mRaceParms->GetStartTime() : 0.0f;
    mRaceMasterTimer.Stop();
    mRaceMasterTimer.Reset(startTime);

    mBonusTime = 0.0f;
    mTaskTime = 0.0f;
    mTimeExpiredMsgSent = false;
    mSuddenDeathMode = false;
    mLastSecondTickSent = 0;
}

void GRaceStatus::SetLapTime(int lapIndex, int racerIndex, float time) {
    mLapTimes[lapIndex][racerIndex] = time;
}

float GRaceStatus::GetLapTime(int lapIndex, int racerIndex
#ifndef EA_BUILD_A124
                              ,
                              bool bCumulativeTimeAtLap
#endif
) {
#ifndef EA_BUILD_A124
    if (bCumulativeTimeAtLap) {

        float totalTime = 0.0f;
        for (int i = 0; i <= lapIndex; i++) {

            float time = mLapTimes[i][racerIndex];
            if (time > 0.0f) {

                totalTime += time;

            } else {

                return 0.0f;
            }
        }
        return totalTime;
    }
#endif

    return mLapTimes[lapIndex][racerIndex];
}

void GRaceStatus::SetCheckpointTime(int lapIndex, int checkIndex, int racerIndex, float time) {
    mCheckTimes[lapIndex][checkIndex][racerIndex] = time;
}

// STRIPPED en GameCube (Range 0xFFFFFFFF en el DWARF); la A124 la tiene y SLUS no
float GRaceStatus::GetCheckpointTime(int lapIndex, int checkIndex, int racerIndex, bool bCumulative) {
    if (bCumulative) {
        const unsigned int numCheckpoints = GRaceStatus::Get().GetRaceParameters()->GetNumCheckpoints();
        float totalTime = 0.0f;
        for (unsigned int i = 1; i < numCheckpoints; i++) {
            float time = mCheckTimes[lapIndex][i][racerIndex];
            if (time > 0.0f) {
                totalTime += time;
            } else {
                totalTime = 0.0f;
                break;
            }
            if (i == checkIndex) {
                break;
            }
        }
        if (checkIndex == numCheckpoints) {
            float time = mCheckTimes[lapIndex][0][racerIndex];
            if (time > 0.0f) {
                totalTime += time;
            } else {
                totalTime = 0.0f;
            }
        }
        return totalTime;
    }
    return mCheckTimes[lapIndex][checkIndex][racerIndex];
}

int GRaceStatus::GetLapPosition(int lapIndex, int racerIndex
#ifndef EA_BUILD_A124
                                ,
                                bool bOverallPosition
#endif
) {
    float lapTime = GetLapTime(lapIndex, racerIndex
#ifndef EA_BUILD_A124
                               ,
                               bOverallPosition
#endif
    );
    int numFaster = 0;

    for (int onRacer = 0; onRacer < mRacerCount; onRacer++) {

        if (onRacer == racerIndex) {
            continue;
        }
        float onRacerLapTime = GetLapTime(lapIndex, onRacer
#ifndef EA_BUILD_A124
                                          ,
                                          bOverallPosition
#endif
        );

        if (onRacerLapTime < lapTime && onRacerLapTime > 0.0f) {
            numFaster++;
        }
    }

    return numFaster + 1;
}

// STRIPPED en GameCube (Range 0xFFFFFFFF en el DWARF); la A124 la tiene y SLUS no
int GRaceStatus::GetCheckpointPosition(int lapIndex, int checkIndex, int racerIndex) {
    float checkTime = GetCheckpointTime(lapIndex, checkIndex, racerIndex, true);
    int numFaster = 0;

    for (int onRacer = 0; onRacer < mRacerCount; onRacer++) {
        if (onRacer == racerIndex) {
            continue;
        }
        float onRacerCheckTime = GetCheckpointTime(lapIndex, checkIndex, onRacer, true);
        if (onRacerCheckTime < checkTime && onRacerCheckTime > 0.0f) {
            numFaster++;
        }
    }

    return numFaster + 1;
}

float GRaceStatus::GetBestLapTime(int racerIndex) {
    float bestLapTime = GetLapTime(0, racerIndex
#ifndef EA_BUILD_A124
                                   ,
                                   false
#endif
    );

    for (int onLap = 1; onLap < mRaceParms->GetNumLaps(); onLap++) {

        float lapTime = GetLapTime(onLap, racerIndex
#ifndef EA_BUILD_A124
                                   ,
                                   false
#endif
        );

        if (lapTime > 0.0f && lapTime < bestLapTime) {
            bestLapTime = lapTime;
        }
    }

    return bestLapTime;
}

float GRaceStatus::GetWorstLapTime(int racerIndex) {
    float worstLapTime = GetLapTime(0, racerIndex
#ifndef EA_BUILD_A124
                                    ,
                                    false
#endif
    );

    for (int onLap = 1; onLap < mRaceParms->GetNumLaps(); onLap++) {

        float lapTime = GetLapTime(onLap, racerIndex
#ifndef EA_BUILD_A124
                                   ,
                                   false
#endif
        );

        if (lapTime > 0.0f && lapTime > worstLapTime) {
            worstLapTime = lapTime;
        }
    }

    return worstLapTime;
}

float GRaceStatus::GetRaceSpeedTrapSpeed(int trapIndex, int racerIndex) {

    GRacerInfo &info = GetRacerInfo(racerIndex);
    return info.mSpeedTrapSpeed[trapIndex];
}

int GRaceStatus::GetRaceSpeedTrapPosition(int trapIndex, int racerIndex) {

    GRacerInfo &info = GetRacerInfo(racerIndex);
    return info.mSpeedTrapPosition[trapIndex];
}

float GRaceStatus::GetBestSpeedTrapSpeed(int racerIndex) {
    float bestSpeedtrapSpeed = GetRaceSpeedTrapSpeed(0, racerIndex);

    for (int onSpeedtrap = 1; onSpeedtrap < GetNumRaceSpeedTraps(); onSpeedtrap++) {

        float speedtrapSpeed = GetRaceSpeedTrapSpeed(onSpeedtrap, racerIndex);

        if (speedtrapSpeed > 0.0f && speedtrapSpeed < bestSpeedtrapSpeed) {
            bestSpeedtrapSpeed = speedtrapSpeed;
        }
    }

    return bestSpeedtrapSpeed;
}

float GRaceStatus::GetWorstSpeedTrapSpeed(int racerIndex) {
    float worstSpeedtrapSpeed = GetRaceSpeedTrapSpeed(0, racerIndex);

    for (int onSpeedtrap = 1; onSpeedtrap < GetNumRaceSpeedTraps(); onSpeedtrap++) {

        float speedtrapSpeed = GetRaceSpeedTrapSpeed(onSpeedtrap, racerIndex);

        if (speedtrapSpeed > 0.0f && speedtrapSpeed > worstSpeedtrapSpeed) {
            worstSpeedtrapSpeed = speedtrapSpeed;
        }
    }

    return worstSpeedtrapSpeed;
}

float GRaceStatus::GetRaceTollboothTime(int boothIndex, int racerIndex) {

    GRacerInfo &info = GetRacerInfo(racerIndex);
    return info.mTimeRemainingToBooth[boothIndex];
}

void GRaceStatus::RaceAbandoned() {
    if (GetRaceContext() == GRace::kRaceContext_Career && mRaceBin != NULL) {

        GRaceParameters *parms = GetRaceParameters();

#ifdef EA_BUILD_A124
        if (parms != NULL && parms->GetIsBossRace()) {
#else
        if (parms != NULL && parms->GetIsBossRace() && !parms->GetIsEpicPursuitRace()) {
#endif

            unsigned int numBossRaces = mRaceBin->GetBossRaceCount();

            for (unsigned int index = 0; index < numBossRaces; index++) {

                unsigned int raceHash = mRaceBin->GetBossRaceHash(index);
                GRaceDatabase::Get().ResetCareerCompleteFlag(raceHash);
            }

#ifndef EA_BUILD_A124
            GManager::Get().RefreshEngageTriggerIcons();
#endif
        }
    }
}

void GRaceStatus::FinalizeRaceStats() {
#ifdef EA_BUILD_A124
    if (GRaceStatus::Get().GetRaceContext() != GRace::kRaceContext_Online) {
#else
    if (GRaceStatus::Get().GetRaceContext() != GRace::kRaceContext_TimeTrial) {
#endif

        int num_racers = mRacerCount;
        for (int idx = 0; idx < num_racers; idx++) {

            GRacerInfo &info = GetRacerInfo(idx);
            info.FinalizeRaceStats();
        }

        UpdateAdaptiveDifficulty(kAdaptiveGain_FromFinalizeRace, NULL);
    }
}

IVehicle *GRacerInfo::CreateVehicle(unsigned int default_key) {
    GCharacter *character = mGameCharacter;
    if (character == NULL) {
        return NULL;
    }

    const char *carTypeName = character->CarType();
    const char *carNameLowMem = character->CarTypeLowMem();
    const char *presetRide = character->PresetRide();

    FECustomizationRecord record;
    if (SkipFE != 0 && bStrLen(SkipFEOpponentPresetRide) > 0) {
        presetRide = SkipFEOpponentPresetRide;
    }

    unsigned int vehicleKey = 0;
    if (presetRide != NULL) {
        PresetCar *preset = FindFEPresetCar(bStringHashUpper(presetRide));
        if (preset != NULL) {
            record.BecomePreset(preset);
            vehicleKey = *reinterpret_cast<unsigned int *>(reinterpret_cast<char *>(preset) + 0x54);
        }
    }

    if (vehicleKey == 0) {
        if (carTypeName != NULL && carTypeName[0] != 0) {
            vehicleKey = Attrib::StringKey(carTypeName).GetHash32();
        }
    }
    if (vehicleKey == 0) {
        vehicleKey = default_key;
    }

    Attrib::Gen::pvehicle vehicle(vehicleKey, 0, NULL);
    if (!vehicle.IsValid()) {
        return NULL;
    }

    if (!record.IsPreset()) {
        RideInfo ride;
        ride.Init(CarPartDB.GetCarType(bStringHashUpper(vehicle.MODEL().GetString())), CarRenderUsage_AIRacer, 0, 0);
        ride.SetRandomParts();
        record.WriteRideIntoRecord(&ride);
    }

    Physics::Info::Performance performance(1.0f, 1.0f, 1.0f);
    IVehicleCache *cache = &GRaceStatus::Get();
    UMath::Vector3 dir = {0.0f, 0.0f, 1.0f};
    VehicleParams params(cache, DRIVER_RACER, vehicleKey, dir, UMath::Vector3::kZero, 9, &record, &performance);
    ISimable *simable = ISimable::CreateInstance("PVehicle", params);
    if (simable == NULL) {
        return NULL;
    }

    IVehicle *ivehicle = NULL;
    if (!simable->QueryInterface(&ivehicle)) {
        return NULL;
    }
    SetSimable(simable);
    return ivehicle;
}

bool GRacerInfo::IsBehind(const GRacerInfo &other) const {
    if (mFinishedRacing) {
        if (!other.mFinishedRacing) {
            return false;
        }
        return mRaceTimer.GetTime() > other.mRaceTimer.GetTime();
    }
    if (other.mFinishedRacing) {
        return true;
    }
    if ((mKnockedOut && other.mKnockedOut) ||
        (mEngineBlown && other.mEngineBlown) ||
        (mTotalled && other.mTotalled)) {
        return mRaceTimer.GetTime() < other.mRaceTimer.GetTime();
    }
#ifndef EA_BUILD_A124
    if (mDNF && other.mDNF) {
        return mPctRaceComplete > other.mPctRaceComplete;
    }
#endif
    if (mKnockedOut || mEngineBlown || mTotalled) {
        return true;
    }
    if (other.mKnockedOut || other.mEngineBlown || other.mTotalled) {
        return false;
    }
    if (mLapsCompleted != other.mLapsCompleted) {
        return mLapsCompleted < other.mLapsCompleted;
    }
    return mPctRaceComplete < other.mPctRaceComplete;
}

float GRacerInfo::CalcAverageSpeed() const {
    float raceTime = GetRaceTime();

    if (raceTime > 0.0f) {
        return GetDistDriven() / raceTime;
    }

    return 0.0f;
}

void GRacerInfo::SetSimable(ISimable *simable) {
    if (simable != nullptr) {
        mhSimable = simable->GetInstanceHandle();
        return;
    }
    mhSimable = (HSIMABLE)simable;
}

void GRacerInfo::KnockOut() {
#ifdef EA_BUILD_A124 // la A124 solo marca al corredor
    mKnockedOut = true;
#else
    if (!mFinishedRacing) {
        mKnockedOut = true;
        mRaceTimer.Stop();
        mLapTimer.Stop();
        mCheckTimer.Stop();
        GRaceStatus::Get().CalculateRankings();
    }
#endif
}

void GRacerInfo::TotalVehicle() {
#ifdef EA_BUILD_A124 // la A124 solo marca al corredor
    mTotalled = true;
#else
    if (!IsFinishedRacing()) {

        mTotalled = true;

        mRaceTimer.Stop();
        mLapTimer.Stop();
        mCheckTimer.Stop();

        GRaceStatus::Get().CalculateRankings();
    }
#endif
}

void GRacerInfo::Busted() {
#ifdef EA_BUILD_A124 // la A124 solo marca al corredor
    mBusted = true;
#else
    if (!mFinishedRacing) {
        mBusted = true;
        mRaceTimer.Stop();
        mLapTimer.Stop();
        mCheckTimer.Stop();
        GRaceStatus::Get().CalculateRankings();
    }
#endif
}

void GRacerInfo::ChallengeComplete() {
    mChallengeComplete = true;
}

void GRacerInfo::ForceStop() {
    ISimable *simable = GetSimable();
    if (simable == nullptr) {
        return;
    }
    IVehicle *vehicle = nullptr;
    if (simable->QueryInterface(&vehicle)) {
        vehicle->ForceStopOn(1);
    }
}

void GRacerInfo::BlowEngine() {
#ifdef EA_BUILD_A124 // la A124 solo marca al corredor
    mEngineBlown = true;
#else
    if (!mFinishedRacing) {
        mEngineBlown = true;
        mRaceTimer.Stop();
        mLapTimer.Stop();
        mCheckTimer.Stop();
        GRaceStatus::Get().CalculateRankings();
    }
#endif
}

void GRacerInfo::SetName(const char *name) {
    mName = name;
}

void GRacerInfo::SetRanking(int ranking) {
    mRanking = ranking;
}

void GRacerInfo::AddToPointTotal(float points) {
    mPointTotal = mPointTotal + points;
    mPointTotal = UMath::Max(0.0f, mPointTotal);
}

void GRacerInfo::SetIndex(int index) {
    mIndex = index;
}

static inline float GRacerInfoMphToMetersPerSecond(float mph) {
    return mph * 0.44703f;
}

static inline float GRacerInfoMilesToMeters(float miles) {
    return miles * 1609.34f;
}

void GRacerInfo::Update(float dT) {
#ifndef EA_BUILD_A124 // la A124 no sale antes con el corredor fuera de carrera
    if (mFinishedRacing || mEngineBlown || mTotalled || mKnockedOut) {
        return;
    }
#endif
    ISimable *simable = GetSimable();
    if (simable == nullptr) {
        return;
    }
    GRaceStatus &raceStatus = GRaceStatus::Get();

    IEngine *engine = nullptr;
    if (simable->QueryInterface(&engine)) {
        if (engine->IsNOSEngaged()) {
            mPoundsNOSUsed += dT * engine->GetNOSFlowRate();
        }
    }

    IPlayer *player = simable->GetPlayer();
    if (player != nullptr) {
        if (player->InGameBreaker()) {
            mSpeedBreakerTime += dT;
        }
    }

    UMath::Vector3 velocity;
    simable->GetLinearVelocity(velocity);
    float speed = UMath::Length(velocity);
    float distance = speed * dT;

    if (speed > mTopSpeed) {
        mTopSpeed = speed;
    }
    mDistanceDriven += distance;
    mTotalUpdateTime += dT;

    if (mQuarterMileTime == 0.0f) {
        static const float quarterMileInMeters = GRacerInfoMilesToMeters(0.25f);
        if (mDistanceDriven >= quarterMileInMeters) {
            mQuarterMileTime = mRaceTimer.GetTime();
        }
    }

    if (mZeroToSixtyTime == 0.0f) {
        static const float sixtyMphInMetersPerSec = GRacerInfoMphToMetersPerSecond(60.0f);
        if (mTopSpeed >= sixtyMphInMetersPerSec) {
            mZeroToSixtyTime = mRaceTimer.GetTime();
        }
    }

    IVehicleAI *vehicleAI = nullptr;
    float raceLength = raceStatus.GetRaceLength();
    mDistToNextCheckpoint = simable->QueryInterface(&vehicleAI) ? vehicleAI->GetPathDistanceRemaining() : 0.0f;

    if (raceLength > 0.0f) {
        int lapsCompleted = mLapsCompleted;
        float lapLength = 0.0f;
        if (lapsCompleted > 0) {
            lapLength = raceStatus.GetFirstLapLength();
        }
        if (lapsCompleted > 1) {
            lapLength += raceStatus.GetSubsequentLapLength() * (lapsCompleted - 1);
        }

        int checkpointsHit = mCheckpointsHitThisLap;
        float pctLap = 0.0f;
        for (int i = 0; i < checkpointsHit; i++) {
            pctLap += raceStatus.GetSegmentLength(i, lapsCompleted);
        }
        float distToNext = mDistToNextCheckpoint;
        float segmentLength = raceStatus.GetSegmentLength(checkpointsHit, lapsCompleted);

        if (distToNext != 0.0f) {
            pctLap += segmentLength - distToNext;

            lapLength += pctLap;

            float lapFrac = raceStatus.GetLapLength(lapsCompleted);

            if (lapFrac > 0.0f) {
                mPctLapComplete = bClamp(pctLap / lapFrac, 0.0f, 1.0f) * 100.0f;
            } else {
                mPctLapComplete = 100.0f;
            }

            mPctRaceComplete = bClamp(lapLength / raceLength, 0.0f, 1.0f) * 100.0f;
        }
    }
}

float GRacerInfo::GetHudPctRaceComplete() const {

    float start_percent = GRaceStatus::Get().GetRaceParameters()->GetStartPercent();
    return bClamp(start_percent, 0.0f, 1.0f) * 100.0f + GetPctRaceComplete() * (1.0f - bClamp(start_percent, 0.0f, 1.0f));
}

void GRacerInfo::UpdateSplits() {
#ifndef EA_BUILD_A124
    int split = -1;
    if (mPctRaceComplete >= 80.0f && mSplitTimes[3] == 0.0f) {
        split = 3;
    } else if (mPctRaceComplete >= 60.0f && mSplitTimes[2] == 0.0f) {
        split = 2;
    } else if (mPctRaceComplete >= 40.0f && mSplitTimes[1] == 0.0f) {
        split = 1;
    } else if (mPctRaceComplete >= 20.0f && mSplitTimes[0] == 0.0f) {
        split = 0;
    }
    if (split != -1) {
        mSplitTimes[split] = mRaceTimer.GetTime();
        mSplitRankings[split] = mRanking;
    }
#endif
}

bool GRaceStatus::IsAudioLoading() {
    int count = GetRacerCount();
    for (int i = 0; i < count; i++) {
        ISimable *simable = GetRacerInfo(i).GetSimable();
        if (simable != nullptr) {
            IAudible *audible = nullptr;
            if (simable->QueryInterface(&audible)) {
                if (!audible->IsAudible()) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool GRaceStatus::IsModelsLoading() {
    int count = GetRacerCount();
    for (int i = 0; i < count; i++) {
        ISimable *simable = GetRacerInfo(i).GetSimable();
        if (simable != nullptr) {
            IRenderable *renderable = nullptr;
            if (simable->QueryInterface(&renderable)) {
                if (!renderable->IsRenderable()) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool GRaceStatus::IsLoading() {
    bool loading = false;
    if (IsAudioLoading()) {
        loading = true;
    } else if (IsModelsLoading()) {
        loading = true;
    } else if (!GManager::Get().StockCarsLoaded()) {
        loading = true;
    }
    return loading;
}

float GRaceStatus::GetSegmentLength(int segment, int lap) {
    float length = mSegmentLengths[segment];
    if (lap > 0 && segment == 0) {
        length += fSubsequentLapLength - fFirstLapLength;
    }
    return length;
}

void GRacerInfo::SaveStartPosition() {
    mSavedHeatLevel = 0.0f;
    mSavedSpeed = 0.0f;
    mSavedPosition = UMath::Vector3::kZero;
    mSavedDirection = UMath::Vector3::kZero;

    ISimable *simable = GetSimable();
    if (simable == NULL) {
        return;
    }

    IRigidBody *body = simable->GetRigidBody();
    if (body != NULL) {
        mSavedPosition = body->GetPosition();
        mSavedSpeed = body->GetSpeed();
        body->GetForwardVector(mSavedDirection);
        if (mSavedSpeed == 0.0f) {
            mSavedSpeed = GRaceStatus::Get().GetRaceParameters()->GetInitialPlayerSpeed();
        }
    }

    IPerpetrator *perp = NULL;
#ifdef EA_BUILD_A124
    // La A124 limita el calor guardado a 4.0; en la version final se guarda tal cual.
    if (simable->QueryInterface(&perp)) {
        mSavedHeatLevel = perp->GetHeat();
        if (mSavedHeatLevel > 4.0f) {
            mSavedHeatLevel = 4.0f;
        }
    }
#else
    if (simable->QueryInterface(&perp)) {
        mSavedHeatLevel = perp->GetHeat();
    }
#endif
}

void GRacerInfo::RestoreStartPosition() {
    ISimable *simable = GetSimable();

    if (simable != NULL) {

        IRacer *racer;

        if (simable->QueryInterface(&racer)) {

            RacePreparationInfo rpi;

            rpi.Direction = mSavedDirection;
            rpi.HeatLevel = mSavedHeatLevel;
            rpi.Speed = mSavedSpeed;
            rpi.Position = mSavedPosition;
            rpi.Flags = RacePreparationInfo::RESET_DAMAGE;

            racer->PrepareForRace(rpi);
        }
    }
}

void GRacerInfo::ForceStartPosition(const UMath::Vector3 &position, const UMath::Vector3 &direction) {
    mSavedPosition = position;
    mSavedDirection = direction;
}

void GRacerInfo::StartRace() {
    GRaceParameters *race_parameters = GRaceStatus::Get().GetRaceParameters();
    float start_time = race_parameters != NULL ? race_parameters->GetStartTime() : 0.0f;

    mRaceTimer.Reset(start_time);
    mRaceTimer.Start();

    StartLap(1);
}

void GRacerInfo::StartLap(int lapIndexOneBased) {
    mLapsCompleted = lapIndexOneBased - 1;
    mCheckpointsHitThisLap = 0;

    float start_time = 0.0f;

    mLapTimer.Reset(start_time);
    mLapTimer.Start();

    StartCheckpoint(0);
}

void GRacerInfo::StartCheckpoint(int checkIndex) {
    if (checkIndex == mCheckpointsHitThisLap + 1) {
        mCheckpointsHitThisLap = checkIndex;
    }

    float start_time = 0.0f;

    mCheckTimer.Reset(start_time);
    mCheckTimer.Start();

    mTimeCrossedLastCheck = mRaceTimer.GetTime();

    mDistToNextCheckpoint = start_time;
}

void GRacerInfo::NotifySpeedTrapTriggered(float speed) {
    mSpeedTrapSpeed[mSpeedTrapsCrossed] = speed;
    mSpeedTrapPosition[mSpeedTrapsCrossed] = mRanking;
    mSpeedTrapsCrossed++;
}

void GRacerInfo::FinishRace() {
    mRaceTimer.Stop();
    mFinishedRacing = true;

    ISimable *simable = GetSimable();

    if (simable != NULL) {

        UMath::Vector3 linearVelocity;
        simable->GetLinearVelocity(linearVelocity);

        mFinishingSpeed = UMath::Length(linearVelocity);
    }
}

bool GRacerInfo::AreStatsReady() const {
#ifdef EA_BUILD_A124
    return GRaceStatus::Get().GetRaceContext() != GRace::kRaceContext_Online || IsFinishedRacing() || GetIsEngineBlown() || GetIsTotalled();
#else
    return GRaceStatus::Get().GetRaceContext() != GRace::kRaceContext_TimeTrial || IsFinishedRacing() || GetIsEngineBlown() || GetIsTotalled();
#endif
}

void GRacerInfo::ChooseRandomName() {
    if (!DoesStringExist(bStringHash("RACERNAME_000"))) {
        // No hay tabla de nombres localizados en este build.

        mName = "UNKNOWN";
        return;
    }

    const char *name;
    bool dupe;

    do {
        int nameIndex = bRandom(150);
        char nameIdent[32];

        bSPrintf(nameIdent, "RACERNAME_%03d", nameIndex);

        name = GetLocalizedString(bStringHash(nameIdent));

        // Comprobar que ningun otro corredor lleva ya ese nombre.

        dupe = false;

        for (int onRacer = 0; onRacer < GRaceStatus::Get().GetRacerCount(); onRacer++) {

            const char *otherName = GRaceStatus::Get().GetRacerInfo(onRacer).GetName();

            if (otherName != NULL) {

                if (bStrCmp(name, otherName) == 0) {

                    dupe = true;
                    break;
                }
            }
        }
    } while (dupe);

    mName = name;
}

bool GRacerInfo::ChooseBossName() {
    if (GRaceStatus::Exists() && GRaceStatus::Get().GetRaceContext() == GRace::kRaceContext_Career) {

        GRaceParameters *parms = GRaceStatus::Get().GetRaceParameters();

        if (parms != NULL && parms->GetIsBossRace()) {

            char name[32];

            bSNPrintf(name, 32, "BLACKLIST_RIVAL_%02d_LEADERBOARD", GRaceStatus::Get().GetRaceBin()->GetBinNumber());

            mName = GetLocalizedString(bStringHash(name));

            return true;
        }
    }

    return false;
}

bool GRacerInfo::ChooseRacerName() {

    if (GRaceStatus::Exists() && GRaceStatus::Get().GetRaceContext() == GRace::kRaceContext_Career) {

        unsigned int charName = mGameCharacter->GetName();

        if (charName != 0) {

            mName = GetLocalizedString(charName);

            return true;
        }
    }

    return false;
}

void GRacerInfo::FinalizeRaceStats() {
    if (mFinishedRacing) {
        return;
    }

    float raceTime = mRaceTimer.GetTime();
    float adjustedTime = raceTime;
    float pct = mPctRaceComplete;
    if (pct > 0.1f) {
        adjustedTime = raceTime / (pct * 0.01f);
    }

#ifdef EA_BUILD_A124
    // La A124 solo penaliza el tiempo de mas en el radar: no hay abandonos, ni vueltas, ni parciales.
    if (GRaceStatus::Get().GetRaceType() == GRace::kRaceType_SpeedTrap) {
        GRaceParameters *parms = GRaceStatus::Get().GetRaceParameters();
        if (parms != NULL) {
            float overtime = static_cast<float>(parms->GetGameplayObj()->OvertimePenaltyPerSec());
            float extra = adjustedTime - raceTime;
            if (extra > 0.0f) {
                AddToPointTotal(-(extra * overtime));
            }
        }
    }
    mRaceTimer.SetTime(adjustedTime);
    FinishRace();
#else
    GRace::Type raceType = GRaceStatus::Get().GetRaceType();
    if (raceType == GRace::kRaceType_Drag) {
        if (mTotalled || mEngineBlown || pct < 0.4f) {
            adjustedTime = 0.0f;
            mDNF = true;
        }
    }

    raceType = GRaceStatus::Get().GetRaceType();
    if (raceType == GRace::kRaceType_SpeedTrap) {
        if (mGameCharacter != NULL) {
            GRaceParameters *parms = GRaceStatus::Get().GetRaceParameters();
            if (parms != NULL) {
                float overtime = static_cast<float>(parms->GetGameplayObj()->OvertimePenaltyPerSec());
                float extra = adjustedTime - raceTime;
                if (extra > 0.0f) {
                    AddToPointTotal(-(extra * overtime));
                }
            }
        }
    }

    if (GRaceStatus::Get().GetRaceParameters()->GetIsLoopingRace()) {
        if (mGameCharacter != NULL) {
            int numLaps = GRaceStatus::Get().GetRaceParameters()->GetNumLaps();
            float sum = 0.0f;
            int lapIndex = mLapsCompleted;
            int i = 0;
            while (i < numLaps) {
                float t = GRaceStatus::Get().GetLapTime(i, mIndex, false);
                if (t == 0.0f) {
                    lapIndex = i;
                    break;
                }
                i = i + 1;
                sum = sum + t;
            }
            GRaceStatus::Get().SetLapTime(lapIndex, mIndex, adjustedTime - sum);
        }
    }

    if (mGameCharacter != NULL) {
        float deathPercent;
        if (mDNF) {
            deathPercent = pct;
        } else {
            deathPercent = 1.0f;
        }
        const float pct[4] = {0.2f, 0.4f, 0.6f, 0.8f};
        for (int onSplit = 0; onSplit < 4; onSplit++) {
            bool wasAliveAtPct = deathPercent >= pct[onSplit];
            if (mSplitTimes[onSplit] == 0.0f) {
                if (wasAliveAtPct) {
                    mSplitTimes[onSplit] = adjustedTime * pct[onSplit];
                } else {
                    mSplitTimes[onSplit] = 0.0f;
                }
            }
            if (mSplitRankings[onSplit] == 0) {
                mSplitRankings[onSplit] = mRanking;
            }
        }
    }

    if (!mDNF) {
        mRaceTimer.SetTime(adjustedTime);
        FinishRace();
    }
#endif
}
