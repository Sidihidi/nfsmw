#include "GRaceDatabase.h"

#include "GRaceStatus.h"

#include "GActivity.h"
#include "GCharacter.h"
#include "GManager.h"
#include "GMarker.h"
#include "GMilestone.h"
#include "GObjectBlock.h"
#include "GSpeedTrap.h"
#include "GVault.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/FixedPoint.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Misc/Platform.h"
#include "Speed/Indep/Src/Misc/PackedDecimal.h"
#include "Speed/Indep/Src/World/TimeOfDay.hpp"
#include "Speed/Indep/Src/World/TrackInfo.hpp"
#include "Speed/Indep/Src/World/WCollisionAssets.h"
#include "Speed/Indep/Src/World/WRoadNetwork.h"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/gameplay_hash.h"
#include "Speed/Indep/Tools/AttribSys/Runtime/AttribLoadAndGo.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

// r75-medianas: el DWARF del original (bloques inline de GenerateIndex) declara
// neg, man, exp --en ese orden-- con neg en r0, man en r31 y exp en r27.
inline PackedDecimal::PackedDecimal(float value) {
    bool neg;
    int man;
    int exp;

    exp = 0;
    man = 0;
    neg = false;

    if (value < 0.0f) {
        neg = true;
        value = -value;
    }
    if (value > 0.0f) {
        while (value < (float)FloatingPoint<short, 10, 3, 5, 11>::GetNormalizedLower()) {
            value *= 10.0f;
            exp--;
        }
        man = (int)value;
        while (man >= FloatingPoint<short, 10, 3, 5, 11>::GetNormalizedUpper()) {
            man /= 10;
            exp++;
        }
    }
    if (neg) {
        man = -man;
    }

    mExp = exp;
    mMan = man;
}

class Minimap {
  public:
    static void ConvertPos(bVector2 &worldPos, bVector2 &minimapPos, TrackInfo *track);
};

GRaceDatabase *GRaceDatabase::mObj = NULL;

const char GRaceDatabase::sDDayRaces[5][8] = {
    "16.1.0",
    "16.2.2",
    "16.2.3",
    "16.1.1",
    "16.2.1",
};

GRaceDatabase::GRaceDatabase() {
    mRaceCountStatic = 0;
    mRaceCountDynamic = 0;
    mRaceIndex = NULL;
    mRaceParameters = NULL;
    mBinCount = 0;
    mBins = NULL;
    mGameplayClass = Attrib::Database::Get().GetClass(Attrib::ClassName::gameplay);
    mStartupRace = NULL;
    mStartupRaceContext = kRaceContext_QuickRace;
    mRaceScoreInfo = NULL;

#ifndef EA_PLATFORM_PLAYSTATION2
    DVDErrorTask(NULL, 0);
#endif
    BuildBinList();
#ifndef EA_PLATFORM_PLAYSTATION2
    DVDErrorTask(NULL, 0);
#endif
    BuildRaceList();
#ifndef EA_PLATFORM_PLAYSTATION2
    DVDErrorTask(NULL, 0);
#endif
    BuildScoreList();
#ifndef EA_PLATFORM_PLAYSTATION2
    DVDErrorTask(NULL, 0);
#endif
}

void GRaceDatabase::Init() {
    mObj = new ("GRaceDatabase", 0) GRaceDatabase;
}

void GRaceDatabase::BuildBinList() {
    mBinCount = StoreBinList(NULL);
    mBins = (GRaceBin *)::operator new[](mBinCount * sizeof(GRaceBin), "Gameplay race bin list", 0);
    StoreBinList(mBins);
}

unsigned int GRaceDatabase::StoreBinList(GRaceBin *list) {
    unsigned int count = 0;

    Attrib::Key collectionKey = mGameplayClass->GetFirstCollection();

    while (collectionKey != 0) {
        Attrib::Gen::gameplay element(collectionKey, 0, NULL);

        if (CollectionIsRaceBin(element)) {
            if (list != NULL) {
                new (&list[count]) GRaceBin(collectionKey);
            }
            count++;
        }

        collectionKey = mGameplayClass->GetNextCollection(collectionKey);
    }

    return count;
}

unsigned int GRaceDatabase::SerializeBins(unsigned char *dest) {
    unsigned char *destStart = dest;

    *(unsigned int *)dest = mBinCount;
    dest += 4;

    for (unsigned int onBin = 0; onBin < mBinCount; onBin++) {
        *(unsigned short *)dest = (unsigned short)mBins[onBin].GetBinNumber();
        dest += 2;

        unsigned short *serializedSize = (unsigned short *)dest;
        dest += 2;

        *serializedSize = mBins[onBin].Serialize(dest);
        dest += *serializedSize;
    }

    return dest - destStart;
}

unsigned int GRaceDatabase::DeserializeBins(unsigned char *src) {
    unsigned char *start = src;
    unsigned int count = *(unsigned int *)src;

    src += 4;

    for (unsigned int i = 0; i < count; i++) {
        int binNumber = *(unsigned short *)src;
        src += 2;

        unsigned short size = *(unsigned short *)src;
        src += 2;

        GRaceBin *bin = GetBinNumber(binNumber);
        if (bin != NULL) {
            bin->Deserialize(src);
        }

        src += size;
    }

    return src - start;
}

void GRaceDatabase::RefreshBinProgress() {
    for (unsigned int onBin = 0; onBin < mBinCount; onBin++) {
        mBins[onBin].RefreshProgress();
    }
}

void GRaceDatabase::BuildRaceList() {
    unsigned int raceCount = StoreRaceList(NULL);
    mRaceCountStatic = raceCount;
    mRaceCountDynamic = 0;

    unsigned int indexAllocParams = GetVirtualMemoryAllocParams();
    mRaceIndex = (GRaceIndexData *)bMalloc(raceCount * sizeof(GRaceIndexData), "Gameplay race index", 0, indexAllocParams);

    unsigned int parmsCount = mRaceCountStatic;
    unsigned int parmsAllocParams = GetVirtualMemoryAllocParams();
    mRaceParameters = (GRaceParameters *)bMalloc(parmsCount * sizeof(GRaceParameters), "Gameplay race tracking", 0, parmsAllocParams);

    for (unsigned int i = 0; i < 4; i++) {
        mRaceCustom[i] = NULL;
    }

    StoreRaceList(mRaceParameters);
}

unsigned int GRaceDatabase::StoreRaceList(GRaceParameters *list) {
    unsigned int count = 0;

    Attrib::Key collectionKey = mGameplayClass->GetFirstCollection();

    while (collectionKey != 0) {
#ifndef EA_PLATFORM_PLAYSTATION2
        DVDErrorTask(NULL, 0);
#endif

        Attrib::Gen::gameplay element(collectionKey, 0, NULL);

        if (CollectionIsRaceActivity(element)) {
            if (list != NULL) {
                new (&list[count]) GRaceParameters(collectionKey, &mRaceIndex[count]);
            }
            count++;
        }

        collectionKey = mGameplayClass->GetNextCollection(collectionKey);
    }

    return count;
}

bool GRaceDatabase::CollectionIsRaceActivity(Attrib::Gen::gameplay &instanceObj) {
    Attrib::Gen::gameplay templateObj(Attrib::Hash::gameplay::key_race, 0, NULL);

    if (instanceObj.Template()) {
        return false;
    }

    unsigned int parentKey = instanceObj.GetParent();

    while (parentKey != 0) {
        Attrib::Gen::gameplay parentObj(parentKey, 0, 0);

        if (parentObj.GetCollection() == templateObj.GetCollection()) {
            return true;
        }

        parentKey = parentObj.GetParent();
    }

    return false;
}

bool GRaceDatabase::CollectionIsRaceBin(Attrib::Gen::gameplay &instanceObj) {
    unsigned int parentKey = instanceObj.GetParent();

    Attrib::Gen::gameplay templateObj(Attrib::Hash::gameplay::key_race_bin, 0, 0);
    Attrib::Gen::gameplay parentObj(parentKey, 0, 0);

    if (parentObj.GetCollection() == templateObj.GetCollection()) {
        return true;
    }

    return false;
}

void GRaceDatabase::NotifyVaultUnloading(GVault *vault) {
    for (unsigned int onRace = 0; onRace < GetRaceCount(); onRace++) {
        GRaceParameters *parms = GetRaceParameters(onRace);

        if (parms->GetParentVault() == vault) {
            parms->NotifyParentVaultUnloading();
        }
    }

    if (mStartupRace != NULL && vault == mStartupRace->GetParentVault()) {
        ClearStartupRace();
    }
}

void GRaceDatabase::NotifyVaultLoaded(GVault *vault) {
    for (unsigned int onRace = 0; onRace < GetRaceCount(); onRace++) {
        GRaceParameters *parms = GetRaceParameters(onRace);

        if (parms->GetParentVault() == vault) {
            parms->NotifyParentVaultLoaded();
        }
    }
}

unsigned int GRaceDatabase::GetRaceCount() {
    return mRaceCountStatic + mRaceCountDynamic;
}

GRaceParameters *GRaceDatabase::GetRaceParameters(unsigned int index) {
    if (index < mRaceCountStatic) {
        return &mRaceParameters[index];
    }

    index -= mRaceCountStatic;

    return mRaceCustom[index];
}

GRaceParameters *GRaceDatabase::GetRaceFromActivity(GActivity *activity) {
    return GetRaceFromKey(activity->GetCollection());
}

GRaceParameters *GRaceDatabase::GetRaceFromHash(unsigned int hash) {
    for (unsigned int onRace = 0; onRace < GetRaceCount(); onRace++) {
        GRaceParameters *parms = GetRaceParameters(onRace);

        if (parms->GetEventHash() == hash) {
            return parms;
        }
    }

    return nullptr;
}

GRaceParameters *GRaceDatabase::GetRaceFromKey(unsigned int key) {
    for (unsigned int onRace = 0; onRace < GetRaceCount(); onRace++) {
        GRaceParameters *parms = GetRaceParameters(onRace);

        if (parms->GetCollectionKey() == key) {
            return parms;
        }
    }

    return nullptr;
}

unsigned int GRaceDatabase::GetBinCount() {
    return mBinCount;
}

GRaceBin *GRaceDatabase::GetBin(unsigned int index) {
    return &mBins[index];
}

GRaceBin *GRaceDatabase::GetBinNumber(int number) {
    for (unsigned int onBin = 0; onBin < GetBinCount(); onBin++) {
        if (mBins[onBin].GetBinNumber() == number) {
            return &mBins[onBin];
        }
    }

    return nullptr;
}

GRaceCustom *GRaceDatabase::AllocCustomRace(GRaceParameters *parms) {
    if (parms == NULL) {
        return NULL;
    }

    parms->BlockUntilLoaded();

    int index = mRaceCountDynamic++;

    return mRaceCustom[index] = new ("GRaceCustom") GRaceCustom(*parms);
}

void GRaceDatabase::FreeCustomRace(GRaceCustom *custom) {
    if (custom == NULL) {
        return;
    }

    if (custom == mStartupRace) {
        custom->SetFreedByOwner();
    } else {
        DestroyCustomRace(custom);
    }
}

void GRaceDatabase::DestroyCustomRace(GRaceCustom *custom) {
    if (custom != NULL) {
        delete custom;
    }

    for (unsigned int onCustom = 0; onCustom < 4; onCustom++) {
        if (mRaceCustom[onCustom] == custom) {
            if (onCustom < mRaceCountDynamic - 1) {
                mRaceCustom[onCustom] = mRaceCustom[mRaceCountDynamic - 1];
            }
            mRaceCountDynamic--;
            mRaceCustom[mRaceCountDynamic] = NULL;
            break;
        }
    }
}

void GRaceDatabase::ClearStartupRace() {
    if (mStartupRace != NULL && mStartupRace->GetFreedByOwner()) {
        DestroyCustomRace(mStartupRace);
    }

    mStartupRace = NULL;
    mStartupRaceContext = kRaceContext_QuickRace;
}

void GRaceDatabase::SetStartupRace(GRaceCustom *custom, GRace::Context context) {
    if (mStartupRace != NULL) {
        ClearStartupRace();
    }

    mStartupRace = custom;
    mStartupRaceContext = (Context)context;

#ifndef EA_BUILD_A124
    if (custom != NULL && context == GRace::kRaceContext_Career) {
        custom->SetupTimeOfDay();
    }
#endif
}

GRaceCustom *GRaceDatabase::GetStartupRace() {
    return mStartupRace;
}

Context GRaceDatabase::GetStartupRaceContext() {
    return mStartupRaceContext;
}

void GRaceDatabase::BuildScoreList() {
    mRaceScoreInfo = (GRaceSaveInfo *)bMalloc(mRaceCountStatic * sizeof(GRaceSaveInfo), "Gameplay score index", 0, 0x800);
    ClearRaceScores();
}

void GRaceDatabase::UpdateRaceScore(bool setComplete) {
    GRaceParameters *raceParms = GRaceStatus::Get().GetRaceParameters();
    unsigned int eventHash = raceParms->GetEventHash();
    GRacerInfo *racer = GRaceStatus::Get().GetWinningPlayerInfo();

    if (racer == NULL) {
        return;
    }

    GRaceSaveInfo *info = GetScoreInfo(eventHash);

    float topSpeed = racer->mTopSpeed;

    info->mTopSpeed = UMath::Max(topSpeed, (float)info->mTopSpeed);
    info->mAverageSpeed = UMath::Max(racer->CalcAverageSpeed(), (float)info->mAverageSpeed);

    switch (raceParms->GetRaceType()) {
    case GRace::kRaceType_Circuit:
    case GRace::kRaceType_Knockout:
        if ((info->mFlags & kCompleted_ContextAny) == 0
            || GRaceStatus::Get().GetBestLapTime(racer->mIndex) < info->mHighScores.mBestTime) {
            info->mHighScores.mBestTime = GRaceStatus::Get().GetBestLapTime(racer->mIndex);
        }
        break;

    case GRace::kRaceType_P2P:
    case GRace::kRaceType_Drag:
    case GRace::kRaceType_Tollbooth:
    case GRace::kRaceType_JumpToSpeedTrap:
    case GRace::kRaceType_JumpToMilestone:
        if ((info->mFlags & kCompleted_ContextAny) == 0 || racer->GetRaceTime() < info->mHighScores.mBestTime) {
            info->mHighScores.mBestTime = racer->GetRaceTime();
        }
        break;

    case GRace::kRaceType_Checkpoint: {
        unsigned int points = (unsigned int)racer->mPointTotal;

        if ((info->mFlags & kCompleted_ContextAny) == 0 || points > info->mHighScores.mBestPoints) {
            info->mHighScores.mBestPoints = points;
        }
        break;
    }

    case GRace::kRaceType_SpeedTrap:
    case GRace::kRaceType_CashGrab:
        if ((info->mFlags & kCompleted_ContextAny) == 0 || racer->mPointTotal > info->mHighScores.mBestCash) {
            info->mHighScores.mBestCash = racer->mPointTotal;
        }
        break;

    default:
        break;
    }

    if (setComplete) {
        if (GRaceStatus::Get().GetRaceContext() == GRace::kRaceContext_Career) {
            info->mFlags |= kCompleted_ContextCareer;
        } else {
            info->mFlags |= kCompleted_ContextQuickRace;
        }

        RefreshBinProgress();
    }
}

void GRaceDatabase::ClearRaceScores() {
    if (mRaceScoreInfo != NULL) {
        bMemSet(mRaceScoreInfo, 0, mRaceCountStatic * sizeof(GRaceSaveInfo));

        for (unsigned int i = 0; i < mRaceCountStatic; i++) {
            GRaceParameters *parms = &mRaceParameters[i];

            if (parms->GetIsDDayRace()) {
                continue;
            }

            bool quickUnlocked = parms->GetInitiallyUnlockedQuickRace();
            bool onlineUnlocked = parms->GetInitiallyUnlockedOnline();
            bool challengeUnlocked = parms->GetInitiallyUnlockedChallenge();

            if (parms->GetIsBossRace()) {
#ifdef EA_BUILD_A124
                quickUnlocked = false;
                onlineUnlocked = false;
#endif
                challengeUnlocked = false;
            }

            if (quickUnlocked || onlineUnlocked || challengeUnlocked) {
                GRaceSaveInfo *info = GetScoreInfo(parms->GetEventHash());

                if (quickUnlocked || challengeUnlocked) {
                    info->mFlags |= kUnlocked_QuickRace;
                }
                if (onlineUnlocked) {
                    info->mFlags |= kUnlocked_Online;
                }
            }
        }
    }
}

GRaceSaveInfo *GRaceDatabase::GetScoreInfo(unsigned int eventHash) {
    unsigned int index = 0;

    GRaceSaveInfo *current = mRaceScoreInfo;

    for (index = 0; index < mRaceCountStatic; index++) {
        if (current->mRaceHash == 0) {
            break;
        }
        if (current->mRaceHash == eventHash) {
            return current;
        }
        current++;
    }

    current->mRaceHash = eventHash;
    return current;
}

bool GRaceDatabase::CheckRaceScoreFlags(unsigned int eventHash, ScoreFlags mask) {
    GRaceSaveInfo *info = GetScoreInfo(eventHash);

    if (info != nullptr) {
        return (info->mFlags & mask) != 0;
    }

    return false;
}

void GRaceDatabase::ResetCareerCompleteFlag(unsigned int eventHash) {
    GRaceSaveInfo *info = GetScoreInfo(eventHash);

    info->mFlags &= ~kCompleted_ContextCareer;
}

void GRaceDatabase::LoadBestScores(GRaceSaveInfo *saveInfo, unsigned int numEntries) {
    bMemSet(mRaceScoreInfo, 0, mRaceCountStatic * sizeof(GRaceSaveInfo));

    unsigned int numLoaded = 0;
    GRaceSaveInfo *dst = mRaceScoreInfo;

    for (unsigned int i = 0; i < numEntries && numLoaded != mRaceCountStatic; i++) {
        if (saveInfo->mRaceHash != 0 && GetRaceFromHash(saveInfo->mRaceHash) != NULL) {
            bMemCpy(dst, saveInfo, sizeof(GRaceSaveInfo));
            numLoaded++;
            dst++;
        }

        saveInfo++;
    }
}

const char *GRaceDatabase::GetNextDDayRace() {
    for (int i = 0; i < 5; i++) {
        const char *raceName = sDDayRaces[i];
        GRaceParameters *race = GetRaceFromHash(Attrib::StringHash32(raceName));

        if (!CheckRaceScoreFlags(race->GetEventHash(), kCompleted_ContextCareer)) {
            return raceName;
        }
    }

    return NULL;
}

unsigned int GRaceParameters::GetCollectionKey() const {
    if (mIndex != nullptr) {
        return mIndex->mKey;
    }
    EnsureLoaded();
    return mRaceRecord->GetCollection();
}

float GRaceParameters::GetRaceLengthMeters() const {
    if (mIndex != nullptr) {
        return mIndex->mLength;
    }
    EnsureLoaded();
    return mRaceRecord->RaceLength();
}

int GRaceParameters::GetReputation() const {
    if (mIndex != nullptr) {
        return (int)(float)mIndex->mReputation;
    }
    EnsureLoaded();
    return mRaceRecord->Reputation();
}

float GRaceParameters::GetCashValue() const {
    if (mIndex != nullptr) {
        return (float)mIndex->mCash;
    }
    EnsureLoaded();
    return mRaceRecord->CashValue();
}

int GRaceParameters::GetLocalizationTag() const {
    if (mIndex != nullptr) {
        return mIndex->mLocalizationTag;
    }
    EnsureLoaded();
    return mRaceRecord->LocalizationTag();
}

int GRaceParameters::GetNumLaps() const {
    if (mIndex != nullptr) {
        return mIndex->mNumLaps;
    }
    EnsureLoaded();
    return mRaceRecord->NumLaps();
}

const char *GRaceParameters::GetEventID() const {
    if (mIndex != nullptr) {
        return mIndex->mEventID;
    }
    EnsureLoaded();
    return mRaceRecord->EventID();
}

float GRaceParameters::GetRivalBestTime() const {
    if (mIndex != nullptr) {
        return (float)mIndex->mRivalBest;
    }
    EnsureLoaded();
    return mRaceRecord->RivalBestTime();
}

float GRaceParameters::GetChallengeGoal() const {
    if (mIndex != nullptr) {
        return (float)mIndex->mChallengeGoal;
    }
    EnsureLoaded();
    return mRaceRecord->ThreshholdValue();
}

bool GRaceParameters::GetInitiallyUnlockedQuickRace() const {
    if (mIndex != nullptr) {
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_AvailableQR) != 0;
    }
    EnsureLoaded();
    return mRaceRecord->AvailableQR();
}

bool GRaceParameters::GetInitiallyUnlockedOnline() const {
    if (mIndex != nullptr) {
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_AvailableOnline) != 0;
    }
    EnsureLoaded();
    return mRaceRecord->AvailableOnline();
}

bool GRaceParameters::GetInitiallyUnlockedChallenge() const {
    if (mIndex != nullptr) {
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_InitiallyUnlocked) != 0;
    }
    EnsureLoaded();
    return mRaceRecord->InitiallyUnlocked();
}

bool GRaceParameters::GetIsDDayRace() const {
    if (mIndex != nullptr) {
#ifdef EA_PLATFORM_WIN32
        bool flagSet = (mIndex->mFlags & GRaceIndexData::kRaceFlag_DDayRace) != 0;
        return flagSet;
#else
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_DDayRace) != 0;
#endif
    }
    EnsureLoaded();
    return mRaceRecord->DDayRace();
}

bool GRaceParameters::GetIsBossRace() const {
    if (mIndex != nullptr) {
#ifdef EA_PLATFORM_WIN32
        bool flagSet = (mIndex->mFlags & GRaceIndexData::kRaceFlag_BossRace) != 0;
        return flagSet;
#else
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_BossRace) != 0;
#endif
    }
    EnsureLoaded();
    return mRaceRecord->BossRace();
}

bool GRaceParameters::GetIsMarkerRace() const {
    if (mIndex != nullptr) {
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_MarkerRace) != 0;
    }
    EnsureLoaded();
    return mRaceRecord->IsMarkerRace();
}

bool GRaceParameters::GetIsPursuitRace() const {
    if (mIndex != nullptr) {
#ifdef EA_PLATFORM_WIN32
        bool flagSet = (mIndex->mFlags & GRaceIndexData::kRaceFlag_PursuitRace) != 0;
        return flagSet;
#else
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_PursuitRace) != 0;
#endif
    }
    EnsureLoaded();
    return mRaceRecord->PursuitRace();
}

bool GRaceParameters::GetIsLoopingRace() const {
    if (mIndex != nullptr) {
#ifdef EA_PLATFORM_WIN32
        bool flagSet = (mIndex->mFlags & GRaceIndexData::kRaceFlag_LoopingRace) != 0;
        return flagSet;
#else
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_LoopingRace) != 0;
#endif
    }
    EnsureLoaded();
    return mRaceRecord->IsLoopingRace();
}

bool GRaceParameters::GetRankPlayersByPoints() const {
    if (mIndex != nullptr) {
#ifdef EA_PLATFORM_WIN32
        bool flagSet = (mIndex->mFlags & GRaceIndexData::kRaceFlag_RankPlayersByPoints) != 0;
        return flagSet;
#else
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_RankPlayersByPoints) != 0;
#endif
    }
    EnsureLoaded();
    return mRaceRecord->RankPlayersByPoints();
}

bool GRaceParameters::GetRankPlayersByDistance() const {
    if (mIndex != nullptr) {
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_RankPlayersByDistance) != 0;
    }
    EnsureLoaded();
    return mRaceRecord->RankPlayersByDistance();
}

bool GRaceParameters::GetCopsEnabled() const {
    if (mIndex != nullptr) {
#ifdef EA_PLATFORM_WIN32
        bool flagSet = (mIndex->mFlags & GRaceIndexData::kRaceFlag_CopsInRace) != 0;
        return flagSet;
#else
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_CopsInRace) != 0;
#endif
    }
    EnsureLoaded();
    return mRaceRecord->CopsInRace();
}

bool GRaceParameters::GetScriptedCopsInRace() const {
    if (mIndex != nullptr) {
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_ScriptedCopsInRace) != 0;
    }
    EnsureLoaded();
    return mRaceRecord->ScriptedCopsInRace();
}

bool GRaceParameters::GetNeverInQuickRace() const {
    if (mIndex != nullptr) {
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_NeverInQuickRace) != 0;
    }
    EnsureLoaded();
    return mRaceRecord->NeverInQuickRace();
}

bool GRaceParameters::GetIsChallengeSeriesRace() const {
    if (mIndex != nullptr) {
#ifdef EA_PLATFORM_WIN32
        bool flagSet = (mIndex->mFlags & GRaceIndexData::kRaceFlag_ChallengeSeriesRace) != 0;
        return flagSet;
#else
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_ChallengeSeriesRace) != 0;
#endif
    }
    EnsureLoaded();
    return mRaceRecord->ChallengeSeriesRace();
}

bool GRaceParameters::GetIsCollectorsEditionRace() const {
    if (mIndex != nullptr) {
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_CollectorsEditionRace) ? true : false;
    }
    EnsureLoaded();
    return mRaceRecord->CollectorsEditionRace();
}

float GRaceParameters::GetTimeLimit() const {
    EnsureLoaded();
    return mRaceRecord->TimeLimit();
}

float GRaceParameters::GetMaxHeatLevel() const {
    EnsureLoaded();
    return mRaceRecord->MaxHeatLevel();
}

bool GRaceParameters::GetNoPostRaceScreen() const {
    EnsureLoaded();
    return mRaceRecord->NoPostRaceScreen();
}

bool GRaceParameters::GetUseWorldHeatInRace() const {
    EnsureLoaded();
    return mRaceRecord->UseWorldHeat();
}

float GRaceParameters::GetForceHeatLevel() const {
    EnsureLoaded();
    return mRaceRecord->ForceHeatLevel();
}

float GRaceParameters::GetMaxRaceHeatLevel() const {
    EnsureLoaded();
    return mRaceRecord->MaxHeatLevel();
}

float GRaceParameters::GetInitialPlayerSpeed() const {
    EnsureLoaded();
    return mRaceRecord->InitialPlayerSpeed();
}

bool GRaceParameters::GetIsRollingStart() const {
    EnsureLoaded();
    return mRaceRecord->RollingStart();
}

bool GRaceParameters::GetIsEpicPursuitRace() const {
    EnsureLoaded();
    return mRaceRecord->IsEpicPursuitRace();
}

const char *GRaceParameters::GetPlayerCarType() const {
    EnsureLoaded();
    return mRaceRecord->PlayerCarType();
}

float GRaceParameters::GetPlayerCarPerformance() const {
    EnsureLoaded();
    return mRaceRecord->PlayerCarPerformance();
}

int GRaceParameters::GetKnockoutsPerLap() const {
    EnsureLoaded();
    return mRaceRecord->KnockoutsPerLap();
}

bool GRaceParameters::GetCatchUp() const {
    EnsureLoaded();
    return mRaceRecord->CatchUp();
}

bool GRaceParameters::GetCatchUpOverride() const {
    EnsureLoaded();
    return mRaceRecord->CatchUpOverride();
}

const char *GRaceParameters::GetCatchUpSkill() const {
    EnsureLoaded();
    return mRaceRecord->CatchUpSkill();
}

const char *GRaceParameters::GetCatchUpSpread() const {
    EnsureLoaded();
    return mRaceRecord->CatchUpSpread();
}

float GRaceParameters::GetCatchUpIntegral() const {
    EnsureLoaded();
    return mRaceRecord->CatchUpIntegral();
}

float GRaceParameters::GetCatchUpDerivative() const {
    EnsureLoaded();
    return mRaceRecord->CatchUpDerivative();
}

unsigned int GRaceParameters::GetNumCheckpoints() const {
    EnsureLoaded();
    return mRaceRecord->Num_Checkpoint();
}

bool GRaceParameters::GetCheckpointsVisible() const {
    EnsureLoaded();
    return mRaceRecord->CheckpointsVisible();
}

unsigned int GRaceParameters::GetNumShortcuts() const {
    EnsureLoaded();
    return mRaceRecord->Num_Shortcuts();
}

unsigned int GRaceParameters::GetNumBarrierExemptions() const {
    EnsureLoaded();
    return mRaceRecord->Num_BarrierExemptions();
}

unsigned int GRaceParameters::GetBarrierCount() const {
    EnsureLoaded();
    return mRaceRecord->Num_Barriers();
}

const char *GRaceParameters::GetTrafficPattern() const {
    EnsureLoaded();
    return mRaceRecord->TrafficPattern();
}

const char *GRaceParameters::GetPhotoFinishCamera() const {
    EnsureLoaded();
    return mRaceRecord->FinishCamera();
}

const char *GRaceParameters::GetPhotoFinishTexture() const {
    EnsureLoaded();
    return mRaceRecord->PostRaceScreenTexture();
}

float GRaceParameters::GetTimeOfDay() const {
    EnsureLoaded();

    return mRaceRecord->TOD();
}

float GRaceParameters::GetStartTime() const {
    EnsureLoaded();
    return mRaceRecord->StartTime();
}

float GRaceParameters::GetStartPercent() const {
    EnsureLoaded();
    return mRaceRecord->StartPercent();
}

inline void GRaceParameters::EnsureLoaded() const {
    if (mParentVault != nullptr && !mParentVault->IsLoaded()) {
        mParentVault->LoadSyncTransient();
    }
    if (mChildVault != nullptr && !mChildVault->IsLoaded()) {
        mChildVault->LoadSyncTransient();
    }
}

void GRaceParameters::BlockUntilLoaded() {
    if (mParentVault != nullptr && !mParentVault->IsLoaded()) {
        mParentVault->LoadSyncTransient();
    }
    if (mChildVault != nullptr && !mChildVault->IsLoaded()) {
        mChildVault->LoadSyncTransient();
    }
}

bool GRaceParameters::GetIsLoaded() const {
    if (mParentVault != nullptr && !mParentVault->IsLoaded()) {
        return false;
    }
    if (mChildVault != nullptr && !mChildVault->IsLoaded()) {
        return false;
    }
    return true;
}

GRaceParameters::GRaceParameters(unsigned int collectionKey, GRaceIndexData *index)
    : mIndex(NULL), mRaceRecord(new Attrib::Gen::gameplay(collectionKey, 0, NULL)), mParentVault(NULL), mChildVault(NULL) {
    GenerateIndex(index);
    mIndex = index;

    const char *childVaultName = mRaceRecord->gameplayvault();
    if (childVaultName != NULL) {
        mChildVault = GManager::Get().FindVault(childVaultName);
        mRaceRecord->Num_Children();
    }

    mParentVault = GManager::Get().FindVaultContaining(mRaceRecord->GetCollection());
    if (mParentVault != NULL) {
        mParentVault->IsTransient();
    }
}

GRaceParameters::~GRaceParameters() {
    if (mRaceRecord != NULL) {
        delete mRaceRecord;
        mRaceRecord = NULL;
    }

    mIndex = NULL;
}

void GRaceParameters::GenerateIndex(GRaceIndexData *index) {
    if (index == nullptr) {
        return;
    }

    index->mKey = GetCollectionKey();
    index->mRaceHash = GetEventHash();
    index->mLocalizationTag = GetLocalizationTag();
    index->mLength = GetRaceLengthMeters();
    index->mRivalBest = FixedPoint<unsigned short, 10, 2>(GetRivalBestTime());
    index->mReputation = PackedDecimal(GetReputation());
    index->mCash = PackedDecimal(GetCashValue());
    index->mChallengeType = GetChallengeType();
#ifdef EA_BUILD_A124
    // mChallengeGoal es float en la A124: `swc1 $f0, 0x18` justo detras de la llamada (0x2EB2DC).
    index->mChallengeGoal = GetChallengeGoal();
#else
    index->mChallengeGoal = PackedDecimal(GetChallengeGoal());
#endif
    index->mNumLaps = GetNumLaps();
    index->mCopDensity = GetCopDensity();
    index->mRaceType = GetRaceType();
    index->mRegion = GetRegion();

    index->mFlags = 0;
    index->mFlags |= GetInitiallyUnlockedQuickRace() ? GRaceIndexData::kRaceFlag_AvailableQR : 0;
    index->mFlags |= GetInitiallyUnlockedOnline() ? GRaceIndexData::kRaceFlag_AvailableOnline : 0;
    index->mFlags |= GetInitiallyUnlockedChallenge() ? GRaceIndexData::kRaceFlag_InitiallyUnlocked : 0;
    index->mFlags |= GetCanBeReversed() ? GRaceIndexData::kRaceFlag_CanBeReversed : 0;
    index->mFlags |= GetIsDDayRace() ? GRaceIndexData::kRaceFlag_DDayRace : 0;
    index->mFlags |= GetIsBossRace() ? GRaceIndexData::kRaceFlag_BossRace : 0;
    index->mFlags |= GetIsMarkerRace() ? GRaceIndexData::kRaceFlag_MarkerRace : 0;
    index->mFlags |= GetIsPursuitRace() ? GRaceIndexData::kRaceFlag_PursuitRace : 0;
    index->mFlags |= GetIsLoopingRace() ? GRaceIndexData::kRaceFlag_LoopingRace : 0;
    index->mFlags |= GetRankPlayersByPoints() ? GRaceIndexData::kRaceFlag_RankPlayersByPoints : 0;
    index->mFlags |= GetRankPlayersByDistance() ? GRaceIndexData::kRaceFlag_RankPlayersByDistance : 0;
    index->mFlags |= GetCopsEnabled() ? GRaceIndexData::kRaceFlag_CopsInRace : 0;
    index->mFlags |= GetScriptedCopsInRace() ? GRaceIndexData::kRaceFlag_ScriptedCopsInRace : 0;
    index->mFlags |= GetTimeOfDay() > 0.8f ? GRaceIndexData::kRaceFlag_Unknown13 : 0;
    index->mFlags |= GetNeverInQuickRace() ? GRaceIndexData::kRaceFlag_NeverInQuickRace : 0;
    index->mFlags |= GetIsChallengeSeriesRace() ? GRaceIndexData::kRaceFlag_ChallengeSeriesRace : 0;
    // La A124 no tiene ni la carrera de la Collector's Edition ni el bloque de la hora del dia: tras el `sh` de
    // ChallengeSeries (0x2EB4A4) va directa a bMemSet.
#ifndef EA_BUILD_A124
    index->mFlags |= GetIsCollectorsEditionRace() ? GRaceIndexData::kRaceFlag_CollectorsEditionRace : 0;

    if (GetTimeOfDay() >= 0.8f) {
        index->mFlags |= GRaceIndexData::kRaceFlag_Unknown13;
    } else if (GetTimeOfDay() >= 0.0f) {
        index->mFlags |= 0x20000;
    }
#endif

    bMemSet(index->mEventID, 0, sizeof(index->mEventID));
    bSafeStrCpy(index->mEventID, GetEventID(), sizeof(index->mEventID));

    UMath::Vector2 topLeft;
    UMath::Vector2 botRight;
    GetBoundingBox(topLeft, botRight);

    index->mMapX1 = (unsigned char)(topLeft.x * 255.0f);
    index->mMapY1 = (unsigned char)(topLeft.y * 255.0f);
    index->mMapX2 = (unsigned char)(botRight.x * 255.0f);
    index->mMapY2 = (unsigned char)(botRight.y * 255.0f);
}

void GRaceParameters::NotifyParentVaultUnloading() {
    if (mRaceRecord != NULL) {
        delete mRaceRecord;
        mRaceRecord = NULL;
    }
}

void GRaceParameters::NotifyParentVaultLoaded() {
    if (mRaceRecord == NULL) {
        mRaceRecord = new Attrib::Gen::gameplay(mIndex->mKey, 0, NULL);
    }
}

const Attrib::Gen::gameplay *GRaceParameters::GetGameplayObj() const {
    return mRaceRecord;
}

GActivity *GRaceParameters::GetActivity() const {
    return GRuntimeInstance::FindObject<GActivity>(GetCollectionKey());
}

GVault *GRaceParameters::GetChildVault() const {
    return mChildVault;
}

GVault *GRaceParameters::GetParentVault() const {
    return mParentVault;
}

void GRaceParameters::GetBoundingBox(UMath::Vector2 &topLeft, UMath::Vector2 &botRight) const {
    if (mIndex != nullptr) {
        topLeft.x = mIndex->mMapX1 * (1.0f / 255.0f);
        topLeft.y = mIndex->mMapY1 * (1.0f / 255.0f);
        botRight.x = mIndex->mMapX2 * (1.0f / 255.0f);
        botRight.y = mIndex->mMapY2 * (1.0f / 255.0f);
        return;
    }

    EnsureLoaded();

    UMath::Vector3 pos;

    GetStartPosition(pos);

    float minX = pos.z;
    float maxX = pos.z;
    float minY = -pos.x;
    float maxY = -pos.x;

    if (HasFinishLine()) {
        GetFinishPosition(pos);

        minX = std::min(minX, pos.z);
        maxX = std::max(maxX, pos.z);
        minY = std::min(minY, -pos.x);
        maxY = std::max(maxY, -pos.x);
    }

    for (unsigned int i = 0; i < GetNumCheckpoints(); i++) {
        GetCheckpointPosition(i, pos);

        minX = std::min(minX, pos.z);
        maxX = std::max(maxX, pos.z);
        minY = std::min(minY, -pos.x);
        maxY = std::max(maxY, -pos.x);
    }

    TrackInfo *track = TrackInfo::GetTrackInfo(2000);

    bVector2 worldMin(minX, minY);
    bVector2 worldMax(maxX, maxY);
    bVector2 mapMin;
    bVector2 mapMax;

    Minimap::ConvertPos(worldMin, mapMin, track);
    Minimap::ConvertPos(worldMax, mapMax, track);

    topLeft.x = mapMin.x;
    topLeft.y = mapMin.y;
    botRight.x = mapMax.x;
    botRight.y = mapMax.y;
}

unsigned int GRaceParameters::GetChallengeType() const {
    if (mIndex != nullptr) {
        return mIndex->mChallengeType;
    }

    EnsureLoaded();

    const char *milestoneName = mRaceRecord->MilestoneName();

    if (milestoneName == nullptr || milestoneName[0] == 0) {
        return 0;
    }

    return Attrib::StringHash32(milestoneName);
}

GRace::Type GRaceParameters::GetRaceType() const {
    if (mIndex != nullptr) {
        return (GRace::Type)mIndex->mRaceType;
    }

    EnsureLoaded();

    static struct {
        const char *mTypeName;
        GRace::Type mType;
    } typeTable[] = {
        {"circuit", GRace::kRaceType_Circuit},
        {"p2p", GRace::kRaceType_P2P},
        {"drag", GRace::kRaceType_Drag},
        {"knockout", GRace::kRaceType_Knockout},
        {"tollbooth", GRace::kRaceType_Tollbooth},
        {"speedtrap", GRace::kRaceType_SpeedTrap},
        {"cashgrab", GRace::kRaceType_CashGrab},
        {"checkpointrace", GRace::kRaceType_Checkpoint},
        {"challenge", GRace::kRaceType_Challenge},
        {"speedtrapjump", GRace::kRaceType_JumpToSpeedTrap},
        {"milestonejump", GRace::kRaceType_JumpToMilestone},
    };

    const char *eventType = mRaceRecord->EventIconType();

    for (int onType = 0; onType < 11; onType++) {
        if (bStrCmp(eventType, typeTable[onType].mTypeName) == 0) {
            return typeTable[onType].mType;
        }
    }

    return GRace::kRaceType_None;
}

GRace::Region GRaceParameters::GetRegion() const {
    if (mIndex != nullptr) {
        return (GRace::Region)mIndex->mRegion;
    }

    EnsureLoaded();

    static struct {
        const char *mRegionName;
        GRace::Region mRegion;
    } regionTable[] = {
        {"college", GRace::kRaceRegion_College},
        {"coastal", GRace::kRaceRegion_Coastal},
        {"city", GRace::kRaceRegion_City},
    };

    const char *regionName = mRaceRecord->Region();

    for (int onRegion = 0; onRegion < 3; onRegion++) {
        if (bStrCmp(regionName, regionTable[onRegion].mRegionName) == 0) {
            return regionTable[onRegion].mRegion;
        }
    }

    return GRace::kRaceRegion_None;
}

void GRaceParameters::ExtractPosition(Attrib::Gen::gameplay &collection, UMath::Vector3 &pos) const {
    const UMath::Vector3 &position = collection.Position();

    UMath::Vector3 posSwizzled = UMath::Vector3Make(-position.y, position.z, position.x);

    pos = posSwizzled;
}

void GRaceParameters::ExtractDirection(Attrib::Gen::gameplay &collection, UMath::Vector3 &dir, float rotate) const {
    UMath::Matrix4 rotMat = UMath::Matrix4::kIdentity;
    UMath::Vector3 initialVec = {0.0f, 0.0f, 1.0f};

    UMath::Init(rotMat, 1.0f, 1.0f, 1.0f);
    UMath::MultYRot(rotMat, -(collection.Rotation() + rotate) / 360.0f, rotMat);
    UMath::Rotate(initialVec, rotMat, initialVec);

    dir = initialVec;
}

unsigned int GRaceParameters::GetEventHash() const {
    if (mIndex != nullptr) {
        return mIndex->mRaceHash;
    }

    EnsureLoaded();

    return Attrib::StringHash32(mRaceRecord->EventID());
}

bool GRaceParameters::GetIsAvailable(GRace::Context context) const {
    if (UnlockAllThings) {
        return true;
    }

    switch (context) {
    case kRaceContext_Career:
        return GRaceDatabase::Get().IsCareerRaceUnlocked(GetEventHash()) && !GRaceDatabase::Get().IsCareerRaceComplete(GetEventHash());
    case kRaceContext_QuickRace:
        if (GetNeverInQuickRace()) {
            return false;
        }
        return GRaceDatabase::Get().IsQuickRaceUnlocked(GetEventHash());
    case kRaceContext_Online:
        return GRaceDatabase::Get().IsOnlineRaceUnlocked(GetEventHash());
    default:
        break;
    }

    return false;
}

bool GRaceParameters::GetIsSunsetRace() const {
    if (mIndex != NULL) {
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_Unknown13) != 0;
    }

    if (mParentVault != NULL && !mParentVault->IsLoaded()) {
        mParentVault->LoadSyncTransient();
    }

    if (mChildVault != NULL && !mChildVault->IsLoaded()) {
        mChildVault->LoadSyncTransient();
    }

    return GetTimeOfDay() >= 0.8f;
}

bool GRaceParameters::GetIsMiddayRace() const {
    if (mIndex != NULL) {
        return (mIndex->mFlags & 0x20000) != 0;
    }

    if (mParentVault != NULL && !mParentVault->IsLoaded()) {
        mParentVault->LoadSyncTransient();
    }

    if (mChildVault != NULL && !mChildVault->IsLoaded()) {
        mChildVault->LoadSyncTransient();
    }

    return GetTimeOfDay() >= 0.0f && GetTimeOfDay() < 0.8f;
}

void GRaceParameters::SetupTimeOfDay() {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    // speed.exe 0x5FD210 and X360 0x82363AD0: the TimeOfDay of PC/360 takes the race's own time
    if (GetTimeOfDay() >= 0.0f) {
        SetCurrentTimeOfDay(GetTimeOfDay());
    }
#else
    if (GetIsSunsetRace()) {
        SetCurrentTimeOfDay(0.9f);
    } else if (GetIsMiddayRace()) {
        SetCurrentTimeOfDay(0.0f);
    }
#endif
}

int GRaceParameters::GetTrafficDensity() const {
    EnsureLoaded();

    return mRaceRecord->TrafficLevel();
}

GRace::Difficulty GRaceParameters::GetDifficulty() const {
    EnsureLoaded();

    int difficulty = mRaceRecord->DifficultyLevel();

    if (difficulty < 34) {
        return GRace::kRaceDifficulty_Easy;
    } else if (difficulty < 67) {
        return GRace::kRaceDifficulty_Medium;
    }

    return GRace::kRaceDifficulty_Hard;
}

int GRaceParameters::GetCopDensity() const {
    if (mIndex != nullptr) {
        return mIndex->mCopDensity;
    }

    EnsureLoaded();

    int density = mRaceRecord->CopDensity();

    if (density == 0 && !GetCopsEnabled()) {
        return 0;
    }

    if (density <= 33) {
        return 1;
    }

    if (density <= 66) {
        return 2;
    }

    return 3;
}

bool GRaceParameters::GetCanBeReversed() const {
    if (mIndex != nullptr) {
        return (mIndex->mFlags & GRaceIndexData::kRaceFlag_CanBeReversed) != 0;
    }

#ifndef EA_BUILD_A124
    EnsureLoaded();
#endif

    return false;
}

GCharacter *GRaceParameters::GetOpponentChar(unsigned int index) const {
    EnsureLoaded();

    if (GetIsBossRace() && GRaceStatus::Get().GetRaceContext() != GRace::kRaceContext_Career) {
        return GRuntimeInstance::FindObject<GCharacter>(Attrib::StringToLowerCaseKey("character_smart"));
    }

    return GRuntimeInstance::FindObject<GCharacter>(mRaceRecord->Opponents(index));
}

int GRaceParameters::GetNumOpponents() const {
    EnsureLoaded();

    return mRaceRecord->Num_Opponents();
}

void GRaceParameters::GetStartPosition(UMath::Vector3 &pos) const {
    EnsureLoaded();

    UMath::Vector3 startPos = UMath::Vector3::kZero;
    const GCollectionKey &raceStartSpec = mRaceRecord->racestart();

    if (raceStartSpec.GetCollectionKey() != 0) {
        Attrib::Gen::gameplay startMarkerCollection(raceStartSpec.GetCollectionKey(), 0, NULL);

        ExtractPosition(startMarkerCollection, startPos);
    }

    pos = startPos;
}

void GRaceParameters::GetStartDirection(UMath::Vector3 &dir) const {
    EnsureLoaded();

    UMath::Vector3 startDir = UMath::Vector3::kZero;
    const GCollectionKey &raceStartSpec = mRaceRecord->racestart();

    if (raceStartSpec.GetCollectionKey() != 0) {
        Attrib::Gen::gameplay startMarkerCollection(raceStartSpec.GetCollectionKey(), 0, NULL);

        ExtractDirection(startMarkerCollection, startDir, 0.0f);
    }

    dir = startDir;
}

bool GRaceParameters::HasFinishLine() const {
    EnsureLoaded();

    const GCollectionKey &raceFinishSpec = mRaceRecord->racefinish();

    return raceFinishSpec.GetCollectionKey() != 0;
}

void GRaceParameters::GetFinishPosition(UMath::Vector3 &pos) const {
    EnsureLoaded();

    UMath::Vector3 finishPos = UMath::Vector3::kZero;
    const GCollectionKey &raceFinishSpec = mRaceRecord->racefinish();

    if (raceFinishSpec.GetCollectionKey() != 0) {
        Attrib::Gen::gameplay finishTriggerCollection(raceFinishSpec.GetCollectionKey(), 0, NULL);

        ExtractPosition(finishTriggerCollection, finishPos);
    }

    pos = finishPos;
}

void GRaceParameters::GetFinishDirection(UMath::Vector3 &dir) const {
    EnsureLoaded();

    UMath::Vector3 finishDir = UMath::Vector3::kZero;
    const GCollectionKey &raceFinishSpec = mRaceRecord->racefinish();

    if (raceFinishSpec.GetCollectionKey() != 0) {
        Attrib::Gen::gameplay finishTriggerCollection(raceFinishSpec.GetCollectionKey(), 0, NULL);

        ExtractDirection(finishTriggerCollection, finishDir, 0.0f);
    }

    dir = finishDir;
}

void GRaceParameters::GetCheckpointPosition(unsigned int index, UMath::Vector3 &pos) const {
    EnsureLoaded();

    UMath::Vector3 checkPos = UMath::Vector3::kZero;
    const GCollectionKey &raceCheckSpec = mRaceRecord->Checkpoint(index);

    if (raceCheckSpec.GetCollectionKey() != 0) {
        Attrib::Gen::gameplay checkTriggerCollection(raceCheckSpec.GetCollectionKey(), 0, NULL);

        ExtractPosition(checkTriggerCollection, checkPos);
    }

    pos = checkPos;
}

void GRaceParameters::GetCheckpointDirection(unsigned int index, UMath::Vector3 &dir) const {
    EnsureLoaded();

    UMath::Vector3 checkDir = UMath::Vector3::kZero;
    const GCollectionKey &raceCheckSpec = mRaceRecord->Checkpoint(index);

    if (raceCheckSpec.GetCollectionKey() != 0) {
        Attrib::Gen::gameplay checkTriggerCollection(raceCheckSpec.GetCollectionKey(), 0, NULL);

        ExtractDirection(checkTriggerCollection, checkDir, 0.0f);
    }

    dir = checkDir;
}

GMarker *GRaceParameters::GetShortcut(unsigned int index) const {
    EnsureLoaded();

    const GCollectionKey &shortcut = mRaceRecord->Shortcuts(index);

    return GRuntimeInstance::FindObject<GMarker>(shortcut.GetCollectionKey());
}

GMarker *GRaceParameters::GetBarrierExemption(unsigned int index) const {
    EnsureLoaded();

    const GCollectionKey &exemption = mRaceRecord->BarrierExemptions(index);

    return GRuntimeInstance::FindObject<GMarker>(exemption.GetCollectionKey());
}

const char *GRaceParameters::GetBarrierName(unsigned int index) const {
    EnsureLoaded();

    const char *barrierName = mRaceRecord->Barriers(index);

    if (barrierName != NULL && *barrierName == '*') {
        barrierName++;
    }

    return barrierName;
}

bool GRaceParameters::GetBarrierIsFlipped(unsigned int index) const {
    EnsureLoaded();

    const char *barrierName = mRaceRecord->Barriers(index);

    if (barrierName != NULL) {
        return *barrierName == '*';
    }

    return false;
}

GRaceCustom::GRaceCustom(const GRaceParameters &other) : GRaceParameters(other.GetCollectionKey(), NULL) {
    mRaceActivity = NULL;
    mNumOpponents = 0;
    mReversed = false;
    mFreedByOwner = false;
#ifndef EA_BUILD_A124
    mHeatLevel = -1;
#endif

    unsigned int uniqueKey = mRaceRecord->GenerateUniqueKey("GRaceCustom", true);
    // `gameplay::Modify` ya suma LocalAttribCount() dentro del inline, igual
    // que `Instance::Modify`, asi que aqui va el 0 y GCC lo pliega.
    mRaceRecord->Modify(uniqueKey, 0);

    mRaceRecord->SetParent(other.GetGameplayObj()->GetCollection());

    mNumOpponents = mRaceRecord->Num_Opponents();
}

GRaceCustom::~GRaceCustom() {
    if (mRaceActivity != NULL) {
        delete mRaceActivity;
        mRaceActivity = NULL;
    }
}

void GRaceCustom::CreateRaceActivity() {
    if (mReversed) {
        if (GetCanBeReversed()) {
            GCollectionKey startReverse = mRaceRecord->racestartReverse();
            GCollectionKey finishReverse = mRaceRecord->racefinishReverse();
            mRaceRecord->SET_racestart(startReverse);
            mRaceRecord->SET_racefinish(finishReverse);
        } else {
            mReversed = false;
        }
    }

    int existingOpponents = GetNumOpponents();
    bool bossINQuickRace = false;
    if (GRaceStatus::Get().GetRaceContext() != GRace::kRaceContext_Career && GetIsBossRace()) {
        bossINQuickRace = true;
        existingOpponents = 0;
    }

    if (mNumOpponents != existingOpponents) {
        unsigned int opponentKey[16];
        unsigned int currOpponents = 0;

        if (!bossINQuickRace) {
            currOpponents = mRaceRecord->Num_Opponents();
            for (unsigned int onOpp = 0; onOpp < currOpponents; onOpp++) {
                opponentKey[onOpp] = mRaceRecord->Opponents(onOpp).GetCollectionKey();
            }
        }

        if ((unsigned int)mNumOpponents > currOpponents) {
            if (currOpponents == 0) {
                for (unsigned int onSet = 0; onSet < (unsigned int)mNumOpponents; onSet++) {
                    opponentKey[onSet] = 0x9f3b88c5;
                }
            } else {
                for (unsigned int onCopy = currOpponents; onCopy < (unsigned int)mNumOpponents; onCopy++) {
                    // El original calcula tambien el cociente (muerto): el doble beql+break
                    // viene de / y % sobre el mismo divisor (un solo divu, sin mflo).
                    unsigned int opponentSet = onCopy / currOpponents;
                    (void)opponentSet;
                    opponentKey[onCopy] = opponentKey[onCopy % currOpponents];
                }
            }
        }

        mRaceRecord->Add(Attrib::Hash::gameplay::Opponents, mNumOpponents);
        Attrib::Attribute attribute = mRaceRecord->Get(Attrib::Hash::gameplay::Opponents);
        for (unsigned int onSet = 0; onSet < (unsigned int)mNumOpponents; onSet++) {
            attribute.Set(onSet, GCollectionKey(opponentKey[onSet]));
        }
    }

#ifndef EA_BUILD_A124
    if (mHeatLevel != -1) {
        if (mHeatLevel == 0) {
            SetCopsEnabled(false);
        } else {
            SetForceHeatLevel(mHeatLevel);
        }
    }
#endif

    mRaceActivity = new GActivity(mRaceRecord->GetCollection());
    mRaceActivity->AllocateConnectionBuffer(GObjectBlock::CalcNumConnections(mRaceRecord->GetCollection()));
}

GActivity *GRaceCustom::GetRaceActivity() const {
    return mRaceActivity;
}

void GRaceCustom::GetCheckpointPosition(unsigned int index, UMath::Vector3 &pos) const {
    if (mReversed) {
        unsigned int numCheckpoints = GetNumCheckpoints();
        unsigned int reverseIndex = index + 1;
        index = numCheckpoints - reverseIndex;
    }

    GRaceParameters::GetCheckpointPosition(index, pos);
}

void GRaceCustom::GetCheckpointDirection(unsigned int index, UMath::Vector3 &dir) const {
    if (mReversed) {
        unsigned int numCheckpoints = GetNumCheckpoints();
        unsigned int reverseIndex = index + 1;
        index = numCheckpoints - reverseIndex;
    }

    UMath::Vector3 checkDir = UMath::Vector3::kZero;
    const GCollectionKey &raceCheckSpec = mRaceRecord->Checkpoint(index);

    if (raceCheckSpec.GetCollectionKey() != 0) {

        Attrib::Gen::gameplay checkTriggerCollection(raceCheckSpec.GetCollectionKey(), 0, NULL);

        ExtractDirection(checkTriggerCollection, checkDir, mReversed ? 180.0f : 0.0f);
    }

    dir = checkDir;
}

void GRaceCustom::SetReversed(bool isReverseDir) {
    mReversed = isReverseDir;
}

template <typename T>
void GRaceCustom::SetAttribute(unsigned int key, const T &value, unsigned int index) {
    Attrib::Attribute attribute = mRaceRecord->Get(key);

    if (!attribute.IsValid()) {
        mRaceRecord->Add(key, 1);
        attribute = mRaceRecord->Get(key);
    }

    T *data = (T *)attribute.GetElementPointer(index);
    if (data != nullptr) {
        *data = value;
    }
}

template void GRaceCustom::SetAttribute<int>(unsigned int, const int &, unsigned int);
template void GRaceCustom::SetAttribute<bool>(unsigned int, const bool &, unsigned int);

void GRaceCustom::SetNumLaps(int numLaps) {
    SetAttribute<int>(Attrib::Hash::gameplay::NumLaps, numLaps, 0);
}

void GRaceCustom::SetTrafficDensity(int density) {
    SetAttribute<int>(Attrib::Hash::gameplay::TrafficLevel, UMath::Clamp(density, 0, 100), 0);
}

void GRaceCustom::SetNumOpponents(int numOpponents) {
    mNumOpponents = numOpponents;
}

void GRaceCustom::SetDifficulty(GRace::Difficulty difficulty) {
    int difficultyPct;
    switch (difficulty) {
    case GRace::kRaceDifficulty_Easy:
        difficultyPct = 33;
        break;
    case GRace::kRaceDifficulty_Medium:
        difficultyPct = 66;
        break;
    case GRace::kRaceDifficulty_Hard:
        difficultyPct = 100;
        break;
    default:
        difficultyPct = 0;
        break;
    }
    SetAttribute<int>(Attrib::Hash::gameplay::DifficultyLevel, (int) difficultyPct, 0);
}

void GRaceCustom::SetCatchUp(bool catchUpEnabled) {
    SetAttribute<bool>(Attrib::Hash::gameplay::CatchUp, catchUpEnabled, 0);
}

void GRaceCustom::SetCopsEnabled(bool copsEnabled) {
    SetAttribute<bool>(Attrib::Hash::gameplay::CopsInRace, copsEnabled, 0);
}

#ifdef EA_BUILD_A124
// Alpha 124 (PS2, 0x2ED580): `jr ra; nop`.
void GRaceCustom::SetCopDensity(GRace::CopDensity copDensity) {}
#endif

void GRaceCustom::SetForceHeatLevel(int level) {
    SetAttribute<int>(Attrib::Hash::gameplay::ForceHeatLevel, level, 0);
}

GRaceBin::GRaceBin(unsigned int collectionKey) : mBinRecord(collectionKey, 0, 0), mChildVault(NULL) {
    bMemSet(&mStats, 0, sizeof(BinStats));

    const char *childVaultName = mBinRecord.gameplayvault();
    if (childVaultName != NULL) {
        mChildVault = GManager::Get().FindVault(childVaultName);
        mChildVault->SetRaceBin();
    }
}

unsigned int GRaceBin::GetCollectionKey() const {
    return mBinRecord.GetCollection();
}

GVault *GRaceBin::GetChildVault() const {
    return mChildVault;
}

int GRaceBin::GetBinNumber() const {
    return mBinRecord.BinIndex();
}

float GRaceBin::GetBaseOpenWorldHeat() const {
    return mBinRecord.BaseOpenWorldHeat();
}

float GRaceBin::GetMaxOpenWorldHeat() const {
    return mBinRecord.MaxOpenWorldHeat();
}

float GRaceBin::GetScaleOpenWorldHeat() const {
    return mBinRecord.ScaleOpenWorldHeat();
}

unsigned int GRaceBin::GetBossRaceCount() const {
    return mBinRecord.Num_BossRaces();
}

unsigned int GRaceBin::GetBossRaceHash(unsigned int index) const {
    const GCollectionKey &bossRace = mBinRecord.BossRaces(index);

    GRaceParameters *raceParms = GRaceDatabase::Get().GetRaceFromKey(bossRace.GetCollectionKey());

    return raceParms != nullptr ? raceParms->GetEventHash() : 0;
}

unsigned int GRaceBin::GetWorldRaceCount() const {
    return mBinRecord.Num_WorldRaces();
}

unsigned int GRaceBin::GetWorldRaceHash(unsigned int index) const {
    const GCollectionKey &worldRace = mBinRecord.WorldRaces(index);

    GRaceParameters *raceParms = GRaceDatabase::Get().GetRaceFromKey(worldRace.GetCollectionKey());

    return raceParms != nullptr ? raceParms->GetEventHash() : 0;
}

unsigned int GRaceBin::GetBaselineUnlockCount() const {
    return mBinRecord.Num_BaselineUnlocks();
}

unsigned int GRaceBin::GetBaselineUnlock(unsigned int index) const {
    return mBinRecord.BaselineUnlocks(index);
}

unsigned int GRaceBin::GetBarrierCount() const {
    return mBinRecord.Num_Barriers();
}

const char *GRaceBin::GetBarrierName(unsigned int index) const {
    const char *barrierName = mBinRecord.Barriers(index);

    if (barrierName != nullptr && barrierName[0] == '*') {
        barrierName++;
    }

    return barrierName;
}

bool GRaceBin::GetBarrierIsFlipped(unsigned int index) const {
    const char *barrierName = mBinRecord.Barriers(index);

    if (barrierName != nullptr) {
        return barrierName[0] == '*';
    }

    return false;
}

void GRaceBin::EnableBarriers() {
#ifdef EA_BUILD_A124 // la A124 no pone barreras con todo desbloqueado
    if (UnlockAllThings) {
        return;
    }
#endif
    for (unsigned int i = 0; i < GetBarrierCount(); i++) {
        const char *barrierName = GetBarrierName(i);
        EnableBarrierSceneryGroup(barrierName, GetBarrierIsFlipped(i));
    }
    WCollisionAssets::Get().SetExclusionFlags();
    WRoadNetwork::Get().ResolveBarriers();
}

void GRaceBin::DisableBarriers() {
    RedoTopologyAndSceneryGroups();
    WRoadNetwork::Get().ResetBarriers();
    WRoadNetwork::Get().ResetRaceSegments();
}

int GRaceBin::GetRequiredBounty() const {
    return mBinRecord.RequiredBounty();
}

int GRaceBin::GetRequiredChallenges() const {
    return mBinRecord.RequiredChallenges();
}

int GRaceBin::GetRequiredRaceWins() const {
    return mBinRecord.RequiredRacesWon();
}

int GRaceBin::GetCompletedChallenges() const {
    return mStats.mChallengesCompleted;
}

int GRaceBin::GetAwardedRaceWins() const {
    return mStats.mRacesWon;
}

unsigned int GRaceBin::Serialize(unsigned char *dest) {
    bMemCpy(dest, &mStats, sizeof(BinStats));
    return sizeof(BinStats);
}

unsigned int GRaceBin::Deserialize(unsigned char *src) {
    bMemCpy(&mStats, src, sizeof(BinStats));
    return sizeof(BinStats);
}

void GRaceBin::SetCompletedChallenges(int numChallenges) {
    mStats.mChallengesCompleted = numChallenges;
}

void GRaceBin::SetRacesWon(int numRaces) {
    mStats.mRacesWon = numRaces;
}

void GRaceBin::RefreshProgress() {
    int racesWon = 0;

    for (unsigned int i = 0; i < GetWorldRaceCount(); i++) {
        unsigned int raceHash = GetWorldRaceHash(i);
        if (GRaceDatabase::Get().CheckRaceScoreFlags(raceHash, GRaceDatabase::kCompleted_ContextCareer)) {
            racesWon++;
        }
    }

    int challengesCompleted = 0;

    for (GMilestone *milestone = GManager::Get().GetFirstMilestone(false, GetBinNumber()); milestone != NULL;
         milestone = GManager::Get().GetNextMilestone(milestone, false, GetBinNumber())) {
        if (milestone->GetIsAwarded()) {
            challengesCompleted++;
        }
    }

    for (GSpeedTrap *speedTrap = GManager::Get().GetFirstSpeedTrap(false, GetBinNumber()); speedTrap != NULL;
         speedTrap = GManager::Get().GetNextSpeedTrap(speedTrap, false, GetBinNumber())) {
        if (speedTrap->IsFlagSet(GSpeedTrap::kFlag_Completed)) {
            challengesCompleted++;
        }
    }

    SetRacesWon(racesWon);
    SetCompletedChallenges(challengesCompleted);
}

void GRaceDatabase::SimulateDDayComplete() {
}
