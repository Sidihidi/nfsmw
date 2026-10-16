#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_PostRace.hpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/PhotoFinish.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/MemCard/uiMemcardInterface.hpp"
#include "Speed/Indep/Src/Generated/Events/EQuitToFE.hpp"
#include "Speed/Indep/Src/Generated/Events/ERestartRace.hpp"
#include "Speed/Indep/Src/Generated/Events/EShowResults.hpp"
#include "Speed/Indep/Src/Generated/Events/EUnPause.hpp"
#include "Speed/Indep/Src/Generated/Messages/MEnterSafeHouse.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyRaceAbandoned.h"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Gameplay/GInfractionManager.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Gameplay/GTimer.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Tools/Inc/ConversionUtil.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyGames.hpp"
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineDisconnect.hpp"
#endif

PursuitData PostRacePursuitScreen::mPursuitData;

RaceStat::RaceStat(FEString *title, FEString *data) : FEStatWidget(true) {
    SetTitleObject(title);
    SetDataObject(data);
}



inline RaceResultStat::~RaceResultStat() {}

inline void RaceResultStat::Draw() {
    char text[0x20];

    FEPrintf(GetTitleObject(), "%s", RacerInfo->GetName());

    if (RacerInfo->GetIsEngineBlown()) {
        FEngSetLanguageHash(GetDataObject(), 0x01E66364);
    } else if (RacerInfo->GetIsTotalled()) {
        FEngSetLanguageHash(GetDataObject(), 0xB7B75185);
    } else if (RacerInfo->GetIsKnockedOut()) {
        FEngSetLanguageHash(GetDataObject(), 0x5D82DBA2);
#ifdef EA_BUILD_A124 // la A124 no marca el abandono en las partidas en linea ni en LAN
    } else if (!RacerInfo->IsFinishedRacing() && !FEDatabase->IsOnlineMode() && !FEDatabase->IsLanMode()) {
#else
    } else if (!RacerInfo->IsFinishedRacing()) {
#endif
        FEngSetLanguageHash(GetDataObject(), 0x0FC1BF40);
    } else {
        Timer t(RacerInfo->GetRaceTimer().GetTime());
        t.PrintToString(text, 0);
        FEPrintf(GetDataObject(), "%s", text);
    }

    FEPrintf(Position, "%d", RacerInfo->GetRanking());
}

StatsPanel::StatsPanel() : TheStats() {
    iWidgetToAdd = 1;
    RacerName = "";
    ParentPkg = "";
}

void StatsPanel::Reset() {
    TheStats.DeleteAllElements();
    iWidgetToAdd = 1;
}

void StatsPanel::Draw(unsigned int numPlayers) {
    if (numPlayers > 1 && RacerName != nullptr && bStrCmp(RacerName, "") != 0) {
#ifdef EA_BUILD_A124
        // La A124 muestra y oculta los tres objetos uno a uno, sin el script del grupo.
        FEngSetVisible(ParentPkg, 0x5BC);
        FEngSetVisible(ParentPkg, 0x682);
        FEngSetVisible(ParentPkg, 0xEB43CCB0);
#else
        if (!FEngIsScriptSet(ParentPkg, 0x8A41F5B9, FEHASH_APPEAR)) {
            FEngSetScript(ParentPkg, 0x8A41F5B9, FEHASH_APPEAR, true);
        }
#endif

        FEngSetButtonTexture(FEngFindImage(ParentPkg, 0x5BC), 0x5BC);
        FEngSetButtonTexture(FEngFindImage(ParentPkg, 0x682), 0x682);
        FEPrintf(ParentPkg, 0xEB43CCB0, "%s", RacerName);
#ifdef EA_BUILD_A124
    } else {
        FEngSetInvisible(ParentPkg, 0x5BC);
        FEngSetInvisible(ParentPkg, 0x682);
        FEngSetInvisible(ParentPkg, 0xEB43CCB0);
    }
#else
    } else if (!FEngIsScriptSet(ParentPkg, 0x8A41F5B9, FEHASH_HIDE)) {
        FEngSetScript(ParentPkg, 0x8A41F5B9, FEHASH_HIDE, true);
    }
#endif

    {
        FEWidget *widgey = TheStats.GetHead();
        while (widgey != TheStats.EndOfList()) {
            widgey->Draw();
            widgey = widgey->GetNext();
        }
    }
}

void StatsPanel::AddStat(RaceStat *stat) {
    FEngSetScript(ParentPkg, FEngHashString("LINE%d_GROUP", iWidgetToAdd), FEHASH_INIT, true);
    TheStats.AddTail(stat);
    ++iWidgetToAdd;
}

void StatsPanel::AddInfoStat(unsigned int title, unsigned int info) {
    FEngSetScript(ParentPkg, FEngHashString("LINE%d_GROUP", iWidgetToAdd), FEHASH_INIT, true);
    InfoStat *stat = new ("", 0) InfoStat(GetCurrentString("COLUMN1_DATA"), GetCurrentString("COLUMN3_DATA"), title, info);
    TheStats.AddTail(stat);
    ++iWidgetToAdd;
}

void StatsPanel::AddGenericStat(float stat_data, unsigned int title_hash, unsigned int units_hash, const char *format) {
    FEngSetScript(ParentPkg, FEngHashString("LINE%d_GROUP", iWidgetToAdd), FEHASH_INIT, true);
    GenericStat *stat =
        new ("", 0) GenericStat(GetCurrentString("COLUMN1_DATA"), GetCurrentString("COLUMN3_DATA"), stat_data, title_hash, units_hash, format);
    TheStats.AddTail(stat);
    ++iWidgetToAdd;
}

void StatsPanel::AddTimerStat(float seconds, unsigned int title_hash) {
    FEngSetScript(ParentPkg, FEngHashString("LINE%d_GROUP", iWidgetToAdd), FEHASH_INIT, true);
    TimerStat *stat = new ("", 0) TimerStat(GetCurrentString("COLUMN1_DATA"), GetCurrentString("COLUMN3_DATA"), seconds, title_hash);
    TheStats.AddTail(stat);
    ++iWidgetToAdd;
}

PostRaceResultsScreen::PostRaceResultsScreen(ScreenConstructorData *sd)
    : MenuScreen(sd), RacerStats(), RaceResults(),
#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32)
      msgHandle(0),
#endif
#ifdef EA_PLATFORM_WIN32
      mPCOnlineFlag(false),
#endif
      mNumberOfRacers(GRaceStatus::Get().GetRacerCount()), mIndexOfWinner(-1),
      mIndexOfCurrentRacer(-1), mNumberOfLaps(GRaceStatus::Get().GetRaceParameters()->GetNumLaps()), mNumberOfStats(0),
      mRaceType(GRaceStatus::Get().GetRaceType()), mPostRaceScreenMode(POSTRACESCREENMODE_RESULTS), mPlayerRacerInfo(nullptr),
      mMaxSlotsLeftSide(11), m_raceResultsUploaded(false) {
#ifndef EA_BUILD_A124
    m_RaceButtonHash = 0x5CED1D04;

    bEnableEAMessenger = false;
#endif

    if (mRaceType == GRace::kRaceType_Tollbooth) {
        mPostRaceScreenMode = POSTRACESCREENMODE_LAPSTATS;
    }

    for (int i = 0; i < mNumberOfRacers; ++i) {
        GRacerInfo *info = &GRaceStatus::Get().GetRacerInfo(i);

        if (info->GetSimable() != nullptr && mIndexOfCurrentRacer == -1 && info->GetSimable()->IsPlayer()) {
            mPlayerRacerInfo = info;
            mIndexOfCurrentRacer = i;
            break;
        }
    }

    for (int i = 0; i < 16; ++i) {
        RacerStats[i].SetParentPkg(GetPackageName());
    }
    RaceResults.SetParentPkg(GetPackageName());

#ifdef EA_BUILD_A124
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
        FEngSetScript(GetPackageName(), 0x445A862B, 0x0016A259, true);
        LobbyCore::Instance().Resume(false);
        LobbyGames::Instance().SetResultUpdateCB(ResultUpdateCB, this);
        msgHandle = DialogInterface::ShowMessage(GetPackageName(), "", static_cast<eDialogTitle>(2),
                                                 GetLocalizedString(0x144849C1));
    }
#endif
    Setup();
}

PostRaceResultsScreen::~PostRaceResultsScreen() {
#ifdef EA_BUILD_A124
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
        LobbyGames::Instance().SetResultUpdateCB(nullptr, nullptr);
        LobbyCore::Instance().Suspend();
        if (msgHandle) {
            DialogInterface::DismissDialog(msgHandle);
            msgHandle = 0;
        }
    }
#endif
}

void PostRaceResultsScreen::Setup() {
    for (int i = 0; i < mNumberOfRacers; ++i) {
        GRacerInfo *info = &GRaceStatus::Get().GetRacerInfo(i);

        if (info->IsFinishedRacing() && info->GetRanking() == 1) {
            mIndexOfWinner = i;
            break;
        }
    }

    for (int i = 0; i < mMaxSlotsLeftSide;) {
        FEngSetScript(GetPackageName(), FEngHashString("LINE%d_GROUP", i + 1), FEHASH_HIDE, true);

        if (mPostRaceScreenMode == POSTRACESCREENMODE_STATS ||
            (mPostRaceScreenMode == POSTRACESCREENMODE_LAPSTATS && mRaceType == GRace::kRaceType_Tollbooth)) {
            FEngSetInvisible(GetPackageName(), FEngHashString("COLUMN2_DATA%d", i + 1));
        } else {
            FEngSetVisible(GetPackageName(), FEngHashString("COLUMN2_DATA%d", i + 1));
        }

        i = i + 1;
    }

    FEngSetInvisible(GetPackageName(), 0x586AB4A6);
    FEngSetInvisible(GetPackageName(), 0x44AC8987);
    FEngSetInvisible(GetPackageName(), 0x30EE5E68);

    switch (mPostRaceScreenMode) {
        case POSTRACESCREENMODE_RESULTS:
            RaceResults.Reset();
            SetupResults();
            RaceResults.Draw(mNumberOfRacers);
            break;
        case POSTRACESCREENMODE_STATS:
            RacerStats[mIndexOfCurrentRacer].Reset();
            SetupRacerStats(mIndexOfCurrentRacer, &GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer));
            RacerStats[mIndexOfCurrentRacer].Draw(mNumberOfRacers);
            break;
        case POSTRACESCREENMODE_LAPSTATS:
            RacerStats[mIndexOfCurrentRacer].Reset();
            SetupLapStats(mIndexOfCurrentRacer, &GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer));
            RacerStats[mIndexOfCurrentRacer].Draw(mNumberOfRacers);
            break;
        default:
            break;
    }

    // A124 (oraculo 0x294da8): el alpha encola el mensaje de paquete 0x5e5b26a1 en online o LAN; la final fija el
    // script APPEAR de 0x445A862B cuando no es ninguno de los dos. El sra+andi de las dos versiones es el bool de los
    // accesores IsOnlineMode/IsLanMode.
#ifdef EA_BUILD_A124
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
        cFEng::Get()->QueuePackageMessage(0x5e5b26a1, GetPackageName(), nullptr);
    }
#else
    if (!FEDatabase->IsOnlineMode() && !FEDatabase->IsLanMode() && !FEngIsScriptSet(GetPackageName(), 0x445A862B, FEHASH_APPEAR)) {
        FEngSetScript(GetPackageName(), 0x445A862B, FEHASH_APPEAR, true);
    }
#endif
}

void PostRaceResultsScreen::SetupResults() {
    FEngSetVisible(GetPackageName(), 0x586AB4A6);
    FEngSetVisible(GetPackageName(), 0x44AC8987);
    FEngSetVisible(GetPackageName(), 0x30EE5E68);

    switch (mRaceType) {
        case GRace::kRaceType_P2P:
        case GRace::kRaceType_Drag:
        case GRace::kRaceType_Circuit:
        case GRace::kRaceType_Knockout:
            FEngSetLanguageHash(GetPackageName(), 0x586AB4A6, 0x96B05F47);
            FEngSetLanguageHash(GetPackageName(), 0x44AC8987, 0xCE678AD3);
            FEngSetLanguageHash(GetPackageName(), 0x30EE5E68, 0xB67DA102);
            break;
        case GRace::kRaceType_SpeedTrap:
            FEngSetLanguageHash(GetPackageName(), 0x586AB4A6, 0x96B05F47);
            FEngSetLanguageHash(GetPackageName(), 0x44AC8987, 0xCE678AD3);
            FEngSetLanguageHash(GetPackageName(), 0x30EE5E68, 0x7540FB04);
            break;
        default:
            break;
    }

    FEngSetLanguageHash(GetPackageName(), 0x2D691760, 0xFF115FFF);
    FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                        m_RaceButtonHash,
#else
                        0x5CED1D04,
#endif
                        0xD0B8AA33);

    unsigned int speed_units = 0x8569AB44;
    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        speed_units = 0x8569A25F;
    }

    switch (mRaceType) {
        case GRace::kRaceType_P2P:
        case GRace::kRaceType_Drag:
        case GRace::kRaceType_Circuit:
        case GRace::kRaceType_Knockout:
        case GRace::kRaceType_Tollbooth: {
            int place = 0;
            if (place < mNumberOfRacers) {
                do {
                    int rank = place + 1;
                    int i = 0;
                    GRacerInfo *racer_info = &GRaceStatus::Get().GetRacerInfo(i);

                    while (racer_info->GetRanking() != rank) {
                        racer_info = &GRaceStatus::Get().GetRacerInfo(i++);
                    }
#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124)
                    // Solo SLUS espera a que las estadisticas del corredor esten listas (su asm llama a AreStatsReady).
                    if (!racer_info->AreStatsReady()) {
                        break;
                    }
#endif
                    place = rank;

                    RaceResults.AddStat(new ("", 0)
                                            RaceResultStat(RaceResults.GetCurrentString("COLUMN2_DATA"), RaceResults.GetCurrentString("COLUMN3_DATA"),
                                                           RaceResults.GetCurrentString("COLUMN1_DATA"), racer_info));
                } while (place < mNumberOfRacers);
            }
            break;
        }
        case GRace::kRaceType_SpeedTrap: {
            int place = 0;
            if (place < mNumberOfRacers) {
                do {
                    int rank = place + 1;
                    int i = 0;
                    GRacerInfo *racer_info = &GRaceStatus::Get().GetRacerInfo(i);

                    while (racer_info->GetRanking() != rank) {
                        racer_info = &GRaceStatus::Get().GetRacerInfo(i++);
                    }

#ifdef EA_BUILD_A124 // la A124 muestra la velocidad sin convertir
                    place = rank;
                    RaceResults.AddStat(new ("", 0)
                                            GenericResult(RaceResults.GetCurrentString("COLUMN2_DATA"), RaceResults.GetCurrentString("COLUMN3_DATA"),
                                                          RaceResults.GetCurrentString("COLUMN1_DATA"), speed_units, racer_info->GetPointTotal(), "%$0.0f", racer_info));
#else
                    float speed = racer_info->GetPointTotal();
                    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 0) {
                        speed = MPS2MPH(speed * 0.27778f);
                    }
                    place = rank;

                    RaceResults.AddStat(new ("", 0)
                                            GenericResult(RaceResults.GetCurrentString("COLUMN2_DATA"), RaceResults.GetCurrentString("COLUMN3_DATA"),
                                                          RaceResults.GetCurrentString("COLUMN1_DATA"), speed_units, speed, "%$0.0f", racer_info));
#endif
                } while (place < mNumberOfRacers);
            }
            break;
        }
        default:
            break;
    }
}

void PostRaceResultsScreen::SetupStat_NosUsed() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);
    unsigned int mass_units = 0xC173E1BB;
    float nos = racerInfo.GetPoundsNOSUsed();

    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        mass_units = 0xC173DE1F;
        nos *= 0.45359f;
    }

    if (GRaceStatus::Get().GetRaceTimeElapsed() > 0.0f
#ifndef EA_BUILD_A124 // la A124 no comprueba si las estadisticas estan listas
        && racerInfo.AreStatsReady()
#endif
    ) {
        RacerStats[mIndexOfCurrentRacer].AddGenericStat(nos, 0x114E759F, mass_units, "%$0.0f");
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(0x114E759F, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_TopSpeed() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);
    unsigned int speedUnits = 0x8569AB44;
    float speed = racerInfo.GetTopSpeed() * 2.23699f;

    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        speedUnits = 0x8569A25F;
        speed *= 1.60931f;
    }

    if (GRaceStatus::Get().GetRaceTimeElapsed() > 0.0f
#ifndef EA_BUILD_A124 // la A124 no comprueba si las estadisticas estan listas
        && racerInfo.AreStatsReady()
#endif
    ) {
        RacerStats[mIndexOfCurrentRacer].AddGenericStat(speed, 0x0EF34382, speedUnits, "%$0.0f");
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(0x0EF34382, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_AverageSpeed() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);
    unsigned int speedUnits = 0x8569AB44;
    float speed = racerInfo.CalcAverageSpeed() * 2.23699f;

    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        speedUnits = 0x8569A25F;
        speed *= 1.60931f;
    }

    if (GRaceStatus::Get().GetRaceTimeElapsed() > 0.0f
#ifndef EA_BUILD_A124 // la A124 no comprueba si las estadisticas estan listas
        && racerInfo.AreStatsReady()
#endif
    ) {
        RacerStats[mIndexOfCurrentRacer].AddGenericStat(speed, 0x57F4140A, speedUnits, "%$0.0f");
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(0x57F4140A, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_TimeBehind() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);

#ifdef EA_BUILD_A124
    if (mIndexOfWinner >= 0) {
#else
    if (mIndexOfWinner >= 0 && mIndexOfWinner != mIndexOfCurrentRacer) {
#endif
        float timeDiff = bAbs(GRaceStatus::Get().GetRacerInfo(mIndexOfWinner).GetRaceTime() - racerInfo.GetRaceTime());

        RacerStats[mIndexOfCurrentRacer].AddTimerStat(timeDiff, 0xAB44ED8B);
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(0xAB44ED8B, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_LapVariance() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);

    if (racerInfo.GetLapsCompleted() >= 2) {
        float bestLapTime = GRaceStatus::Get().GetBestLapTime(mIndexOfCurrentRacer);
        float worstLapTime = GRaceStatus::Get().GetWorstLapTime(mIndexOfCurrentRacer);

        RacerStats[mIndexOfCurrentRacer].AddTimerStat(worstLapTime - bestLapTime, 0x4121E8C4);
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(0x4121E8C4, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_StageVariance() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);
}

void PostRaceResultsScreen::SetupStat_TrafficCollisions() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);

    if (GRaceStatus::Get().GetRaceTimeElapsed() > 0.0f) {
        RacerStats[mIndexOfCurrentRacer].AddGenericStat(
            static_cast<float>(racerInfo.GetTrafficCarsHit()), 0x094BFDFC, 0, "%$0.0f");
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(0x094BFDFC, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_ZeroToSixty() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);
    unsigned int speedUnits = 0xCCBC22B3;

    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        speedUnits = 0xB8CF16FC;
    }

    if (racerInfo.GetZeroToSixtyTime() > 0.0f
#ifndef EA_BUILD_A124
        && racerInfo.AreStatsReady()
#endif

    ) {
        RacerStats[mIndexOfCurrentRacer].AddTimerStat(racerInfo.GetZeroToSixtyTime(),
                                                      speedUnits);
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(speedUnits, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_QuarterMile() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);
    unsigned int timeUnits = 0x49FD5DCB;

    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        timeUnits = 0x1C6F2A82;
    }

    if (racerInfo.GetQuarterMileTime() > 0.0f
#ifndef EA_BUILD_A124
        && racerInfo.AreStatsReady()
#endif
    ) {
        RacerStats[mIndexOfCurrentRacer].AddTimerStat(racerInfo.GetQuarterMileTime(),
                                                      timeUnits);
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(timeUnits, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_PerfectShifts() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);

    if (GRaceStatus::Get().GetRaceTimeElapsed() > 0.0f
#ifndef EA_BUILD_A124
        && racerInfo.AreStatsReady()
#endif
    ) {
        RacerStats[mIndexOfCurrentRacer].AddGenericStat(
            static_cast<float>(racerInfo.GetPerfectShifts()), 0x680AC597, 0, "%$0.0f");
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(0x680AC597, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_AccumulatedSpeed() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);
    unsigned int speedUnits = 0x8569A25F;
    float speed = racerInfo.GetPointTotal();

    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 0) {
        speedUnits = 0x8569AB44;
        speed = MPS2MPH(speed * 0.27778f);
    }

    if (GRaceStatus::Get().GetRaceTimeElapsed() > 0.0f) {
        RacerStats[mIndexOfCurrentRacer].AddGenericStat(speed, 0xD57E02AB, speedUnits, "%$0.0f");
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(0xD57E02AB, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_SpeedVariance() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);
    float bestSpeed = GRaceStatus::Get().GetBestSpeedTrapSpeed(mIndexOfCurrentRacer);
    float worstSpeed = GRaceStatus::Get().GetWorstSpeedTrapSpeed(mIndexOfCurrentRacer);

    if (bestSpeed > 0.0f && worstSpeed > 0.0f) {
        unsigned int speed_units = 0x8569A25F;
        float speed = worstSpeed - bestSpeed;

        if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 0) {
            speed_units = 0x8569AB44;
            speed = MPS2MPH(speed * 0.27778f);
        }

        RacerStats[mIndexOfCurrentRacer].AddGenericStat(speed, 0x6EEABE8C, speed_units, "%$0.0f");
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(0x6EEABE8C, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupStat_SpeedBehind() {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfCurrentRacer);

    if (mIndexOfWinner >= 0) {
        unsigned int speed_units = 0x8569A25F;
        GRacerInfo &winnerInfo = GRaceStatus::Get().GetRacerInfo(mIndexOfWinner);
        float speed = bAbs(winnerInfo.GetPointTotal() - racerInfo.GetPointTotal());

        if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 0) {
            speed_units = 0x8569AB44;
            speed = MPS2MPH(speed * 0.27778f);
        }

        RacerStats[mIndexOfCurrentRacer].AddGenericStat(speed, 0x2E54B7ED, speed_units, "%$0.0f");
        return;
    }

    RacerStats[mIndexOfCurrentRacer].AddInfoStat(0x2E54B7ED, 0x0FC1BF40);
}

void PostRaceResultsScreen::SetupRacerStats(int index, GRacerInfo *racer_info) {
    FEngSetLanguageHash(GetPackageName(), 0x2D691760, 0x4E706980);
    switch (mRaceType) {
        case GRace::kRaceType_P2P:
        case GRace::kRaceType_Drag:
            FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                                m_RaceButtonHash,
#else
                                0x5CED1D04,
#endif
                                0x1135F776);
            break;
        case GRace::kRaceType_Circuit:
        case GRace::kRaceType_Knockout:
            FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                                m_RaceButtonHash,
#else
                                0x5CED1D04,
#endif
                                0x7B8F45DF);
            break;
        case GRace::kRaceType_SpeedTrap:
            FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                                m_RaceButtonHash,
#else
                                0x5CED1D04,
#endif
                                0xAEF51E9D);
            break;
        case GRace::kRaceType_Tollbooth:
            FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                                m_RaceButtonHash,
#else
                                0x5CED1D04,
#endif
                                0xAC23368C);
            break;
        default:
            break;
    }

    RacerStats[index].SetRacerName(racer_info->GetName());

    switch (mRaceType) {
        case GRace::kRaceType_P2P:
            SetupStat_TopSpeed();
            SetupStat_AverageSpeed();
            if (FEDatabase->IsLANMode() || FEDatabase->IsOnlineMode()) {
                SetupStat_NosUsed();
            } else {
                SetupStat_TimeBehind();
                SetupStat_TrafficCollisions();
            }
            SetupStat_StageVariance();
            break;
        case GRace::kRaceType_Circuit:
        case GRace::kRaceType_Knockout:
            SetupStat_TopSpeed();
            SetupStat_AverageSpeed();
            if (FEDatabase->IsLANMode() || FEDatabase->IsOnlineMode()) {
                SetupStat_NosUsed();
            } else {
                SetupStat_TimeBehind();
                SetupStat_LapVariance();
                SetupStat_TrafficCollisions();
            }
            break;
        case GRace::kRaceType_Drag:
            SetupStat_ZeroToSixty();
            SetupStat_QuarterMile();
            SetupStat_PerfectShifts();
            if (FEDatabase->IsLANMode() || FEDatabase->IsOnlineMode()) {
                SetupStat_NosUsed();
            } else {
                SetupStat_TimeBehind();
                SetupStat_TrafficCollisions();
            }
            break;
        case GRace::kRaceType_SpeedTrap:
            SetupStat_AccumulatedSpeed();
            SetupStat_SpeedVariance();
            SetupStat_SpeedBehind();
            break;
        case GRace::kRaceType_Tollbooth:
            SetupStat_TopSpeed();
            SetupStat_AverageSpeed();
            SetupStat_StageVariance();
            SetupStat_TrafficCollisions();
            break;
        default:
            break;
    }
}

void PostRaceResultsScreen::SetupLapStats(int racerIndex, GRacerInfo *racer_info) {
    FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                        m_RaceButtonHash,
#else
                        0x5CED1D04,
#endif
                        0x8159A0B2);
    switch (mRaceType) {
        case GRace::kRaceType_P2P:
        case GRace::kRaceType_Drag:
            FEngSetLanguageHash(GetPackageName(), 0x2D691760, 0x34BA50FF);
            FEngSetVisible(GetPackageName(), 0x586AB4A6);
            FEngSetVisible(GetPackageName(), 0x44AC8987);
            FEngSetVisible(GetPackageName(), 0x30EE5E68);
            FEngSetLanguageHash(GetPackageName(), 0x586AB4A6, 0xE8B7D527);
            FEngSetLanguageHash(GetPackageName(), 0x44AC8987, 0x96B05F47);
            FEngSetLanguageHash(GetPackageName(), 0x30EE5E68, 0xB67DA102);
            break;
        case GRace::kRaceType_Circuit:
        case GRace::kRaceType_Knockout:
            FEngSetLanguageHash(GetPackageName(), 0x2D691760, 0x9C8D7FE8);
            FEngSetVisible(GetPackageName(), 0x586AB4A6);
            FEngSetVisible(GetPackageName(), 0x44AC8987);
            FEngSetVisible(GetPackageName(), 0x30EE5E68);
            FEngSetLanguageHash(GetPackageName(), 0x586AB4A6, 0x0000BF9C);
            FEngSetLanguageHash(GetPackageName(), 0x44AC8987, 0x96B05F47);
            FEngSetLanguageHash(GetPackageName(), 0x30EE5E68, 0xB67DA102);
            break;
        case GRace::kRaceType_SpeedTrap:
            if (GRaceStatus::Exists()) {
                GRaceStatus::Get().SortCheckPointRankings();
            }
            FEngSetLanguageHash(GetPackageName(), 0x2D691760, 0xECD0E6A6);
            FEngSetVisible(GetPackageName(), 0x586AB4A6);
            FEngSetVisible(GetPackageName(), 0x44AC8987);
            FEngSetVisible(GetPackageName(), 0x30EE5E68);
            FEngSetLanguageHash(GetPackageName(), 0x586AB4A6, 0xEE1EDC76);
            FEngSetLanguageHash(GetPackageName(), 0x44AC8987, 0x96B05F47);
            FEngSetLanguageHash(GetPackageName(), 0x30EE5E68, 0x7540FB04);
            break;
        case GRace::kRaceType_Tollbooth:
            FEngSetLanguageHash(GetPackageName(), 0x2D691760, 0xD10A8EA2);
            FEngSetVisible(GetPackageName(), 0x586AB4A6);
            FEngSetVisible(GetPackageName(), 0x30EE5E68);
            FEngSetLanguageHash(GetPackageName(), 0x586AB4A6, 0xA15E4505);
            FEngSetLanguageHash(GetPackageName(), 0x30EE5E68, 0xB67DA102);
            FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                                m_RaceButtonHash,
#else
                                0x5CED1D04,
#endif
                                0xD0B8AA33);
            break;
        default:
            break;
    }

    RacerStats[racerIndex].SetRacerName(racer_info->GetName());

    switch (mRaceType) {
        case GRace::kRaceType_P2P:
        case GRace::kRaceType_Drag: {
#ifdef EA_BUILD_A124
            // La A124 aun no tiene StageStat: pone los tiempos de cada punto de control con LapStat.
            unsigned int numCheckpoints = GRaceStatus::Get().GetRaceParameters()->GetNumCheckpoints();
            unsigned int i;
            for (i = 1; i < numCheckpoints; ++i) {
                float checkTime = GRaceStatus::Get().GetCheckpointTime(0, i, racerIndex, true);
                LapStat *stat = new ("", 0) LapStat(RacerStats[racerIndex].GetCurrentString("COLUMN1_DATA"), RacerStats[racerIndex].GetCurrentString("COLUMN3_DATA"),
                                                    RacerStats[racerIndex].GetCurrentString("COLUMN2_DATA"), i, checkTime,
                                                    GRaceStatus::Get().GetCheckpointPosition(0, i, racerIndex));
                RacerStats[racerIndex].AddStat(stat);
            }

            float checkTime = GRaceStatus::Get().GetCheckpointTime(0, 0, racerIndex, true);
            LapStat *stat = new ("", 0) LapStat(RacerStats[racerIndex].GetCurrentString("COLUMN1_DATA"), RacerStats[racerIndex].GetCurrentString("COLUMN3_DATA"),
                                                RacerStats[racerIndex].GetCurrentString("COLUMN2_DATA"), numCheckpoints, checkTime,
                                                GRaceStatus::Get().GetCheckpointPosition(0, 0, racerIndex));
            RacerStats[racerIndex].AddStat(stat);
            break;
#else
            const unsigned int numStages = 4;
            unsigned int stageIndex;
            for (stageIndex = 0; stageIndex < numStages; ++stageIndex) {
                StageStat *stat = new ("", 0) StageStat(RacerStats[racerIndex].GetCurrentString("COLUMN1_DATA"), RacerStats[racerIndex].GetCurrentString("COLUMN3_DATA"),
                                                        RacerStats[racerIndex].GetCurrentString("COLUMN2_DATA"), stageIndex,
                                                        racer_info->GetSplitTime(stageIndex), racer_info->GetSplitRanking(stageIndex));
                RacerStats[racerIndex].AddStat(stat);
            }

            float final_time = 0.0f;
            if (racer_info->IsFinishedRacing()) {
                final_time = racer_info->GetRaceTime();
            }

            StageStat *stat = new ("", 0) StageStat(RacerStats[racerIndex].GetCurrentString("COLUMN1_DATA"), RacerStats[racerIndex].GetCurrentString("COLUMN3_DATA"),
                                                        RacerStats[racerIndex].GetCurrentString("COLUMN2_DATA"), numStages, final_time, racer_info->GetRanking());
            RacerStats[racerIndex].AddStat(stat);
            break;
#endif
        }
        case GRace::kRaceType_Circuit:
        case GRace::kRaceType_Knockout: {
#ifndef EA_BUILD_A124
            bool bCumulativeTime = false;
#endif
            for (int lapIndex = 0; lapIndex < GRaceStatus::Get().GetRaceParameters()->GetNumLaps(); ++lapIndex) {
                int lapPosition = GRaceStatus::Get().GetLapPosition(lapIndex, racerIndex
#ifndef EA_BUILD_A124
                                                                    ,
                                                                    true
#endif
                );

                if (racer_info->GetIsKnockedOut() && lapPosition <= 1) {
                    lapPosition = -1;
                }

                LapStat *stat = new ("", 0) LapStat(RacerStats[racerIndex].GetCurrentString("COLUMN1_DATA"), RacerStats[racerIndex].GetCurrentString("COLUMN3_DATA"),
                                                        RacerStats[racerIndex].GetCurrentString("COLUMN2_DATA"), lapIndex + 1,
                                                    GRaceStatus::Get().GetLapTime(lapIndex, racerIndex
#ifndef EA_BUILD_A124
                                                                                  ,
                                                                                  bCumulativeTime
#endif
                                                                                  ),
                                                    lapPosition);
                RacerStats[racerIndex].AddStat(stat);
            }
            break;
        }
        case GRace::kRaceType_SpeedTrap: {
            for (int trapIndex = 0; trapIndex < GRaceStatus::Get().GetNumRaceSpeedTraps(); ++trapIndex) {
                SpeedStat *stat = new ("", 0) SpeedStat(RacerStats[racerIndex].GetCurrentString("COLUMN1_DATA"), RacerStats[racerIndex].GetCurrentString("COLUMN3_DATA"),
                                                        RacerStats[racerIndex].GetCurrentString("COLUMN2_DATA"), trapIndex + 1,
                                                        GRaceStatus::Get().GetRaceSpeedTrapSpeed(trapIndex, racerIndex),
                                                        GRaceStatus::Get().GetRaceSpeedTrapPosition(trapIndex, racerIndex));
                RacerStats[racerIndex].AddStat(stat);
            }
#ifndef EA_BUILD_A124
            break; // la A124 no lo tiene: sigue con los peajes (su asm cae en el caso siguiente)
#endif
        }
        case GRace::kRaceType_Tollbooth: {
            for (int boothIndex = 0; boothIndex < GRaceStatus::Get().GetNumRaceTollbooths(); ++boothIndex) {
#ifdef EA_BUILD_A124
                // la A124 aun no tiene TollboothStat: los peajes van con LapStat
                LapStat *stat = new ("", 0) LapStat(RacerStats[racerIndex].GetCurrentString("COLUMN1_DATA"), RacerStats[racerIndex].GetCurrentString("COLUMN3_DATA"),
                                                    RacerStats[racerIndex].GetCurrentString("COLUMN2_DATA"), boothIndex + 1,
                                                    GRaceStatus::Get().GetRaceTollboothTime(boothIndex, racerIndex), 1);
#else
                TollboothStat *stat = new ("", 0) TollboothStat(RacerStats[racerIndex].GetCurrentString("COLUMN1_DATA"), RacerStats[racerIndex].GetCurrentString("COLUMN3_DATA"),
                                                        RacerStats[racerIndex].GetCurrentString("COLUMN2_DATA"), boothIndex + 1,
                                                                GRaceStatus::Get().GetRaceTollboothTime(boothIndex, racerIndex), 1);
#endif
                RacerStats[racerIndex].AddStat(stat);
            }
#ifndef EA_BUILD_A124

            float remaining_time = 0.0f;
            if (racer_info->IsFinishedRacing()) {
                remaining_time = GRaceStatus::Get().GetRaceTimeRemaining();
            }

            TollboothStat *stat = new ("", 0) TollboothStat(RacerStats[racerIndex].GetCurrentString("COLUMN1_DATA"), RacerStats[racerIndex].GetCurrentString("COLUMN3_DATA"),
                                                        RacerStats[racerIndex].GetCurrentString("COLUMN2_DATA"), GRaceStatus::Get().GetNumRaceTollbooths() + 1, remaining_time, 1);
            RacerStats[racerIndex].AddStat(stat);
#endif
            break;
        }
        default:
            break;
    }
}

void PostRaceResultsScreen::NotificationMessage(u32 msg, FEObject *pObject, u32 Param1, u32 Param2) {
    switch (msg) {
        case 0x35F8620B: {
            if (!FEDatabase->IsLANMode()) {
                if (!FEDatabase->IsOnlineMode()) {
                    return;
                }
            }

            FEngSetScript(GetPackageName(), 0x812A09D4, FEHASH_HIDE, true);
            FEngSetScript(GetPackageName(), 0x05D85A9F, FEHASH_APPEAR, true);
            return;
        }
        case 0x5073EF13:
            if (mPostRaceScreenMode == POSTRACESCREENMODE_RESULTS) {
                return;
            }

            --mIndexOfCurrentRacer;
            if (mIndexOfCurrentRacer < 0) {
                mIndexOfCurrentRacer = mNumberOfRacers - 1;
            }

            Setup();
            return;
        case 0xD9FEEC59:
            if (mPostRaceScreenMode == POSTRACESCREENMODE_RESULTS) {
                return;
            }

            ++mIndexOfCurrentRacer;
            if (mIndexOfCurrentRacer >= mNumberOfRacers) {
                mIndexOfCurrentRacer = 0;
            }

            Setup();
            return;
        case 0xC519BFC3:
            if (mRaceType == GRace::kRaceType_Tollbooth) {
                if (mPostRaceScreenMode == POSTRACESCREENMODE_LAPSTATS) {
                    mPostRaceScreenMode = POSTRACESCREENMODE_STATS;
                } else {
                    mPostRaceScreenMode = POSTRACESCREENMODE_LAPSTATS;
                }
            } else {
                mPostRaceScreenMode = static_cast<PostRaceScreenMode>(mPostRaceScreenMode + 1);
                if (mPostRaceScreenMode == POSTRACESCREENMODE_NUMMODES) {
                    mPostRaceScreenMode = POSTRACESCREENMODE_RESULTS;
                }
            }

            Setup();
            return;
        case 0xC519BFC4: {
            if (FEDatabase->IsLANMode()) {
                return;
            }

            if (FEDatabase->IsOnlineMode()) {
                return;
            }

            DialogInterface::ShowTwoButtons(GetPackageName(), "InGameDialog.fng", dialog_alert, LANGUAGE_COMMON_OK, LANGUAGE_COMMON_CANCEL, 0xE1A57D51, 0xB4623F67,
                                            0xB4623F67, first_dialog_button2, static_cast<unsigned int>(0x4D3399A8));
            return;
        }
        case 0xE1A57D51:
#ifdef EA_BUILD_A124
            new ERestartRace();
            new EUnPause();
            return;
#else
            new EUnPause();
            if (cFEng::Get()->IsPackagePushed("InGamePhotoMaster.fng")) {
                PhotoFinishScreen::mRestartSelected = true;
                return;
            }

            new ERestartRace();
            return;
#endif
        case 0xB4623F67:
            cFEng::Get()->QueuePackageMessage(FEHASH_ENABLE_INPUT, GetPackageName(), nullptr);
            return;
#ifndef EA_BUILD_A124
        case 0x406415E3: {
            if (FEngIsScriptSet(GetPackageName(), 0x57EFB2FB, FEHASH_HIDE)) {
                return;
            }

            if (GRaceStatus::Get().GetRaceContext() == GRace::kRaceContext_Career && GRaceStatus::Get().GetRaceParameters()->GetIsBossRace()) {
                bool playerDone = false;

                if (mPlayerRacerInfo != nullptr &&
                    (mPlayerRacerInfo->IsFinishedRacing() || mPlayerRacerInfo->GetIsTotalled() || mPlayerRacerInfo->GetIsEngineBlown() ||
                     mPlayerRacerInfo->GetIsKnockedOut() || mPlayerRacerInfo->GetIsBusted())) {
                    playerDone = true;
                }

                if (playerDone && mPlayerRacerInfo->GetRanking() != 1) {
                    DialogInterface::ShowTwoButtons(GetPackageName(), "InGameDialog.fng", dialog_alert, LANGUAGE_COMMON_OK, LANGUAGE_COMMON_CANCEL, LANGUAGE_CONFIRM_QUIT_GAME,
                                                    0xB4623F67, 0xB4623F67, first_dialog_button2, static_cast<unsigned int>(0x9887EB98));
                    return;
                }
            }

            goto set_continue_script;
        }
        case LANGUAGE_CONFIRM_QUIT_GAME:
        set_continue_script:
            if (!FEngIsScriptSet(GetPackageName(), 0x47FF4E7C, 0x001335F0)) {
                FEngSetScript(GetPackageName(), 0x47FF4E7C, 0x001335F0, true);
            }
            return;
        case 0xE1FDE1D1: {
#else
        // La A124 sale de la carrera con este mensaje, y en linea vuelve al frontend sin preguntar.
        case 0x406415E3: {
            if (FEDatabase->IsLANMode() || FEDatabase->IsOnlineMode()) {
                FEManager::RequestUnPauseSimulation(nullptr);
                new EQuitToFE(GARAGETYPE_MAIN_FE, nullptr);
            }
#endif
            if (FEDatabase->IsLANMode()) {
                return;
            }

            if (FEDatabase->IsOnlineMode()) {
                return;
            }

            if (cFEng::Get()->IsPackagePushed("InGamePhotoMaster.fng")) {
                cFEng::Get()->QueuePackagePop(1);
                if (cFEng::Get()->IsPackagePushed("InGameBackground.fng")) {
                    cFEng::Get()->QueuePackagePop(1);
                }
                return;
            }

            {
                bool playerDone = false;
                if (mPlayerRacerInfo != nullptr &&
                    (mPlayerRacerInfo->IsFinishedRacing() || mPlayerRacerInfo->GetIsTotalled() || mPlayerRacerInfo->GetIsEngineBlown() ||
                     mPlayerRacerInfo->GetIsKnockedOut() || mPlayerRacerInfo->GetIsBusted())) {
                    playerDone = true;
                }

                GRaceParameters *parms = GRaceStatus::Get().GetRaceParameters();
                bool ddayRace = false;
                if (parms != nullptr && parms->GetIsDDayRace()) {
                    ddayRace = true;
                }

                if (GRaceStatus::Exists() && GRaceStatus::Get().GetRaceContext() == GRace::kRaceContext_Career && !ddayRace) {
                    if (playerDone) {
                        GRaceStatus::Get().RaceAbandoned();

                        MNotifyRaceAbandoned().Post(UCrc32(0x20d60dbf));
                    }

                    new EUnPause();
                    return;
                }

                if (playerDone) {
                    new EQuitToFE(GARAGETYPE_MAIN_FE, nullptr);
                } else {
                    new EUnPause();
                }
            }
            return;
        }
#ifdef EA_BUILD_A124
        case 0xC98356BA: {
            if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
                float countdown = -1.0f;
                if (TheOnlineManager.GetLocalRacer()) {
                    countdown = TheOnlineManager.GetLocalRacer()->GetEndRaceCountdown();
                }
                if (countdown >= 0.0f) {
                    char countdownStr[32];

                    FEngSetScript(GetPackageName(), 0x7D913050, 0x001CA7C0, false);
                    FEngSetScript(GetPackageName(), 0x77CE0364, 0x03826A28, false);
                    if (countdown >= 10.0f) {
                        bSPrintf(countdownStr, "00:%1.1f", countdown);
                    } else {
                        bSPrintf(countdownStr, "00:0%1.1f", countdown);
                    }
                    FEPrintf(GetPackageName(), 0x77CE0364, countdownStr);
                }
                if (FEDatabase->GetOnlineSettings()->GetState() < cOnlineSettings::OLS_LOBBY_SENT_RACE_RESULTS) {
                    LobbyCore::Instance().Resume(false);
                    LobbyGames::Instance().SetResultUpdateCB(ResultUpdateCB, this);
                    msgHandle = DialogInterface::ShowMessage(GetPackageName(), "", static_cast<eDialogTitle>(2),
                                                             GetLocalizedString(0x144849C1));
                } else if (msgHandle > 0) {
                    DialogInterface::DismissDialog(msgHandle);
                    msgHandle = 0;
                }
                if (TheOnlineManager.GetState() == OLS_RACE_END && !m_raceResultsUploaded) {
                    UploadSavedRaceResults();
                    m_raceResultsUploaded = true;
                }
                if (TheOnlineManager.GetState() != OLS_RACE_END) {
                    Setup();
                }
            }
            return;
        }
        case 0x2C0E485A:
            ExitOnlineMode(GetPackageName());
            return;
#endif
        default:
            return;
    }
}

eMenuSoundTriggers PostRaceResultsScreen::NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) {
#ifdef EA_BUILD_A124
    if (msg == 0x6B283007 || msg == 0xF4B32D4D) {
        if (mNumberOfRacers < 2 || mPostRaceScreenMode == POSTRACESCREENMODE_RESULTS) {
            return static_cast<eMenuSoundTriggers>(-1);
        }
    }
    return maybe;
#else
    if (msg != 0x7B6B89D7) {
        if (msg <= 0x7B6B89D7) {
            if (msg != 0x4A805994) {
                return maybe;
            }
        } else if (msg != 0x9AFA53A7) {
            return maybe;
        }

        if (mNumberOfRacers < 2 || mPostRaceScreenMode == POSTRACESCREENMODE_RESULTS) {
            return static_cast<eMenuSoundTriggers>(-1);
        }
    }

    if (FEngIsScriptSet(GetPackageName(), 0x57EFB2FB, FEHASH_HIDE)) {
        return static_cast<eMenuSoundTriggers>(-1);
    }

    return maybe;
#endif
}

#ifdef EA_BUILD_A124
void PostRaceResultsScreen::ResultUpdateCB(const LobbyGamesN::_GameResultUpdateT *update, void *context) {
    static_cast<PostRaceResultsScreen *>(context)->Setup();
}

void PostRaceResultsScreen::UploadSavedRaceResults() {
    PlayerDataT playerData;

    LobbyUsers::Instance().GetMyUserStats(playerData, nullptr, nullptr);
    if (LobbyCore::Instance().SendSavedRaceResults(SendRaceResultsCB, this) == 0) {
        FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_SENT_RACE_RESULTS);
    }
}

void PostRaceResultsScreen::SendRaceResultsCB(LobbyApiMsgT *msg, void *context) {
    if (msg->code != 0) {
        static_cast<PostRaceResultsScreen *>(context)->m_lastErrorKind = msg->kind;
        static_cast<PostRaceResultsScreen *>(context)->m_lastErrorCode = msg->code;
    }
    FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_SENT_RACE_RESULTS);
}
#endif

// Range: 0x80155F40 -> 0x801560EC
void PursuitData::PopulateData(IPursuit *ipursuit, IPerpetrator *iperpetrator, int exitToSafehouse) {
    mPursuitIsActive = ipursuit && ipursuit->IsPursuitBailed() == false;

    if (ipursuit) {
        mPursuitLength = ipursuit->GetPursuitDuration();
        mNumCopsDamaged = ipursuit->GetNumCopsDamaged();
        mNumCopsDestroyed = ipursuit->GetNumCopsDestroyed();
        mNumSpikeStripsDodged = ipursuit->GetNumSpikeStripsDodged();
        mNumRoadblocksDodged = ipursuit->GetNumRoadblocksDodged();
        mCostToStateAchieved = ipursuit->CalcTotalCostToState();
    }

    if (iperpetrator) {
        if (iperpetrator->GetPendingRepPointsNormal() > 0) {
            mRepAchievedNormal = iperpetrator->GetPendingRepPointsNormal();
        }
        if (iperpetrator->GetPendingRepPointsFromCopDestruction() > 0) {
            mRepAchievedCopDestruction = iperpetrator->GetPendingRepPointsFromCopDestruction();
        }
    }

    if (exitToSafehouse >= 0) {
        mExitToSafehouse = exitToSafehouse;
    }
}

bool PursuitData::AddMilestone(GMilestone *milestone) {
    if (mNumMilestonesThisPursuit >= 32) {
        return false;
    }
    mMilestonesCompleted[mNumMilestonesThisPursuit++] = milestone;
    return true;
}

const GMilestone *const PursuitData::GetMilestone(int index) const {
    if (index < 0x20) {
        return mMilestonesCompleted[index];
    }
    return 0;
}

void PursuitData::ClearData() {
    mPursuitIsActive = false;
    mPursuitLength = 0.0f;
    mNumCopsDamaged = 0;
    mNumCopsDestroyed = 0;
    mNumSpikeStripsDodged = 0;
    mNumRoadblocksDodged = 0;
    mCostToStateAchieved = 0;
    mRepAchievedNormal = 0;
    mRepAchievedCopDestruction = 0;
    mNumMilestonesThisPursuit = 0;
    for (int i = 0; i <= 0x1F; i++) {
        mMilestonesCompleted[i] = nullptr;
    }
}

PursuitResultsDatum::PursuitResultsDatum(PursuitResultsDatumType type, uint32 itemName, float itemNumber, float itemGoal,
#ifdef EA_BUILD_A124
                                         bool itemChecked
#else
                                         PursuitResultsDatumCheckType itemChecked
#endif
                                         )
    : ArrayDatum(0, 0) {
    mType = type;
    mName = itemName;
    mNumber = itemNumber >= 0.0f ? itemNumber : 0.0f;
    mGoal = itemGoal;
    mChecked = itemChecked;
}

void PursuitResultsDatum::NotificationMessage(u32 msg, FEObject *pObj, u32 param1, u32 param2) {}

PursuitResultsArraySlot::PursuitResultsArraySlot(FEObject *obj, FEString *itemName, FEString *itemNumber, FEImage *itemChecked
#ifndef EA_BUILD_A124
                                                 ,
                                                 FEImage *itemEmpty
#endif
                                                 )
    : ArraySlot(obj) {
#ifndef EA_BUILD_A124
    mLine = obj;
#endif
    mItemName = itemName;
    mItemNumber = itemNumber;
    mItemChecked = itemChecked;
#ifndef EA_BUILD_A124
    mItemEmpty = itemEmpty;
#endif
}

void PursuitResultsArraySlot::Update(ArrayDatum *datum, bool isSelected) {
    ArraySlot::Update(datum, isSelected);
    if (datum) {
        PursuitResultsDatum *result = static_cast<PursuitResultsDatum *>(datum);

#ifndef EA_BUILD_A124
        FEngSetScript(mItemChecked, 0x16A259, true);
        FEngSetScript(mItemEmpty, 0x16A259, true);
        FEngSetScript(mLine, 0x1744B3, true);
#endif

        if (mItemName) {
            FEngSetScript(mItemName, 0x1744B3, true);
            FEngSetLanguageHash(mItemName, result->GetName());
        }

        if (result->GetType() == PursuitResultsDatum::PursuitResultsDatumType_Number) {
            FEPrintf(mItemNumber, "%$d", bFloatToInt(result->GetNumber()));
            FEngSetScript(mItemNumber, 0x1744B3, true);
            if (result->GetChecked()) {
                FEngSetScript(mItemChecked, 0x1CA7C0, true);
#ifndef EA_BUILD_A124
            } else if (result->GetGreyed()) {
                FEngSetScript(mItemChecked, 0x163C76, true);
#endif
            } else {
                FEngSetScript(mItemChecked, 0x16A259, true);
            }
        } else if (result->GetType() == PursuitResultsDatum::PursuitResultsDatumType_Time) {
            char timeString[32];
            Timer timeTimer(result->GetNumber());
            timeTimer.PrintToString(timeString, 0);
            FEPrintf(mItemNumber, "%s", timeString);
            FEngSetScript(mItemNumber, 0x1744B3, true);
            if (result->GetChecked()) {
                FEngSetScript(mItemChecked, 0x1CA7C0, true);
#ifndef EA_BUILD_A124
            } else if (result->GetGreyed()) {
                FEngSetScript(mItemChecked, 0x163C76, true);
#endif
            } else {
                FEngSetScript(mItemChecked, 0x16A259, true);
            }
        } else if (result->GetType() == PursuitResultsDatum::PursuitResultsDatumType_Milestone_Number) {
#ifndef EA_BUILD_A124
            bool showCurrVal = true;
#endif
            char outputStr[32];
#ifndef EA_BUILD_A124
            if (result->GetChecked() || result->GetGreyed()) {
                showCurrVal = false;
            }
#endif
            FEDatabase->SetMilestoneDescriptionString(outputStr, 0x850A64BC, result->GetNumber(), result->GetGoal()
#ifndef EA_BUILD_A124
                                                      ,
                                                      showCurrVal
#endif
            );
            FEPrintf(mItemNumber, "%s", outputStr);
            FEngSetScript(mItemNumber, 0x1744B3, true);
#ifndef EA_BUILD_A124
            if (result->GetGreyed()) {
                FEngSetScript(mLine, 0x163C76, true);
            }
#endif
            if (result->GetChecked()) {
                FEngSetScript(mItemChecked, 0x1CA7C0, true);
#ifndef EA_BUILD_A124
            } else if (result->GetGreyed()) {
                FEngSetScript(mItemChecked, 0x163C76, true);
            } else {
                FEngSetScript(mItemEmpty, 0x1CA7C0, true);
#else
            } else {
                FEngSetScript(mItemChecked, 0x16A259, true);
#endif
            }
#ifdef EA_BUILD_A124 // la A124 no tiene el tiempo de persecucion restante
        } else if (result->GetType() == PursuitResultsDatum::PursuitResultsDatumType_Milestone_Time) {
            char outputStr[32];
            FEDatabase->SetMilestoneDescriptionString(outputStr, 0x033FA23A, result->GetNumber(), result->GetGoal());
            FEPrintf(mItemNumber, "%s", outputStr);
            FEngSetScript(mItemNumber, 0x1744B3, true);
            if (result->GetChecked()) {
                FEngSetScript(mItemChecked, 0x1CA7C0, true);
            } else {
                FEngSetScript(mItemChecked, 0x16A259, true);
            }
#else
        } else if (result->GetType() == PursuitResultsDatum::PursuitResultsDatumType_Milestone_Time ||
                   result->GetType() == PursuitResultsDatum::PursuitResultsDatumType_Milestone_Time_PursuitRemaining) {
#ifndef EA_BUILD_A124
            bool showCurrVal = true;
#endif
            int milestoneType;
            char outputStr[32];
#ifndef EA_BUILD_A124
            if (result->GetChecked() || result->GetGreyed()) {
                showCurrVal = false;
            }
#endif
            milestoneType = 0x033FA23A;
            if (result->GetType() == PursuitResultsDatum::PursuitResultsDatumType_Milestone_Time_PursuitRemaining) {
                milestoneType = 0x5392E4FD;
            }
            FEDatabase->SetMilestoneDescriptionString(outputStr, milestoneType, result->GetNumber(), result->GetGoal()
#ifndef EA_BUILD_A124
                                                      ,
                                                      showCurrVal
#endif
            );
            FEPrintf(mItemNumber, "%s", outputStr);
            FEngSetScript(mItemNumber, 0x1744B3, true);
#ifndef EA_BUILD_A124
            if (result->GetGreyed()) {
                FEngSetScript(mLine, 0x163C76, true);
            }
#endif
            if (result->GetChecked()) {
                FEngSetScript(mItemChecked, 0x1CA7C0, true);
#ifndef EA_BUILD_A124
            } else if (result->GetGreyed()) {
                FEngSetScript(mItemChecked, 0x163C76, true);
            } else {
                FEngSetScript(mItemEmpty, 0x1CA7C0, true);
#endif
            }
#endif
        } else if (result->GetType() == PursuitResultsDatum::PursuitResultsDatumType_Check) {
            FEngSetScript(mItemNumber, 0x16A259, true);
            if (result->GetChecked()) {
                FEngSetScript(mItemChecked, 0x1CA7C0, true);
#ifndef EA_BUILD_A124
            } else if (result->GetGreyed()) {
                FEngSetScript(mItemChecked, 0x163C76, true);
            } else {
                FEngSetScript(mItemEmpty, 0x1CA7C0, true);
#else
            } else {
                FEngSetScript(mItemChecked, 0x163C76, true);
#endif
            }
        }    }
}

PostRacePursuitScreen::PostRacePursuitScreen(ScreenConstructorData *sd)
    : ArrayScrollerMenu(sd, 1, 9, false), mPostPursuitScreenMode(POSTPURSUITSCREENMODE_PURSUIT) {
#ifndef EA_BUILD_A124
#ifdef EA_PLATFORM_WIN32
    // SPEED_EXE_1_3 guarda 0xC6BE1C1F (`mov [esi+0xe4], 0xc6be1c1f`); las demas
    // versiones, 0x5CED1D04 (ctor al 100 % en GC/A124/SLUS con ese valor).
    m_RaceButtonHash = 0xC6BE1C1F;
#else
    m_RaceButtonHash = 0x5CED1D04;
#endif
#endif

    {
        for (int i = 0; i < GetHeight(); i++) {
            char sztemp[32];
            FEngSNPrintf(sztemp, 0x20, "LINE%d_GROUP", i + 1);
            FEObject *wrapperGroup = FEngFindObject(GetPackageName(), FEHashUpper(sztemp));
            FEngSNPrintf(sztemp, 0x20, "COLUMN1_DATA%d", i + 1);
            FEString *itemName = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
            FEngSNPrintf(sztemp, 0x20, "COLUMN2_DATA%d", i + 1);
            FEString *itemValue = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
            FEngSNPrintf(sztemp, 0x20, "CHECK%d", i + 1);
            FEImage *checkMark = FEngFindImage(GetPackageName(), FEHashUpper(sztemp));
#ifndef EA_BUILD_A124
            FEngSNPrintf(sztemp, 0x20, "EMPTY%d", i + 1);
            FEImage *emptyMark = FEngFindImage(GetPackageName(), FEHashUpper(sztemp));
#endif
            AddSlot(new ("", 0) PursuitResultsArraySlot(wrapperGroup, itemName, itemValue, checkMark
#ifndef EA_BUILD_A124
                                                        ,
                                                        emptyMark
#endif
                                                        ));
        }
    }
    Initialize();
}

PostRacePursuitScreen::~PostRacePursuitScreen() {
    if (GetPursuitData().mExitToSafehouse != 0) {
        GetPursuitData().mExitToSafehouse = 0;
        MEnterSafeHouse("safehouse").Post(UCrc32(0x20D60DBF));
    }
}

void PostRacePursuitScreen::Initialize() {
    ClearData();
    if (mPostPursuitScreenMode == POSTPURSUITSCREENMODE_INFRACTIONS) {
        FEngSetLanguageHash(GetPackageName(), 0x2D691760, 0xFB415E78);
        if (TheGameFlowManager.IsInFrontend()) {
            FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                                m_RaceButtonHash,
#else
                                0x5CED1D04,
#endif
                                0x7448870B);
        } else {
            FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                                m_RaceButtonHash,
#else
                                0x5CED1D04,
#endif
                                0x74413352);
            if (GRaceStatus::Exists()) {
                GRaceParameters *raceParams = GRaceStatus::Get().GetRaceParameters();
                if (raceParams && raceParams->GetIsPursuitRace() && !FEDatabase->IsFinalEpicChase()) {
                    FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                                        m_RaceButtonHash,
#else
                                        0x5CED1D04,
#endif
                                        0x9145A5F2);
                }
            }
        }
        SetupInfractions();
    } else if (mPostPursuitScreenMode == POSTPURSUITSCREENMODE_MILESTONES) {
        FEngSetLanguageHash(GetPackageName(), 0x2D691760, 0x578B767B);
        if (GRaceStatus::Exists()) {
            GRaceParameters *raceParams = GRaceStatus::Get().GetRaceParameters();
            if (raceParams && raceParams->GetIsPursuitRace() && !FEDatabase->IsFinalEpicChase()) {
                FEngSetLanguageHash(GetPackageName(), 0x2D691760, 0x334FA7FB);
            }
        }
        FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                            m_RaceButtonHash,
#else
                            0x5CED1D04,
#endif
                            0x7448870B);
        SetupMilestones();
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x2D691760, 0xFEA872D4);
        FEngSetLanguageHash(GetPackageName(),
#ifndef EA_BUILD_A124
                            m_RaceButtonHash,
#else
                            0x5CED1D04,
#endif
                            0xAEAEB62F);
        SetupPursuit();
    }
    SetInitialPosition(0);
    ArrayScroller::RefreshHeader();
}

void PostRacePursuitScreen::SetupInfractions() {
#ifdef EA_BUILD_A124
    // La A124 es la revision alpha (oraculo A124 0x297f58, 684 B): Speeding antes que Racing, Check=4 (falta
    // Milestone_Time_PursuitRemaining, luego Check no es 5) y el ctor toma bool, asi que el DidInfractionOccur va
    // directo como argumento del new (banco A124 100%; sin header guard por ahora: el enum compartido sigue con
    // Check=5, aqui va 4 literal; propuesta: guardia en FEPKg_PostRace.hpp para Check=4 sin PursuitRemaining).
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(static_cast<PursuitResultsDatum::PursuitResultsDatumType>(4), 0x8ec1a6ec, 0.0f, 0.0f,
                                                               GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Speeding)));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(static_cast<PursuitResultsDatum::PursuitResultsDatumType>(4), 0x4b0ff103, 0.0f, 0.0f,
                                                               GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Racing)));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(static_cast<PursuitResultsDatum::PursuitResultsDatumType>(4), 0x370f331d, 0.0f, 0.0f,
                                                               GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Reckless)));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(static_cast<PursuitResultsDatum::PursuitResultsDatumType>(4), 0x8462a784, 0.0f, 0.0f,
                                                               GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Assault)));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(static_cast<PursuitResultsDatum::PursuitResultsDatumType>(4), 0xdff254ba, 0.0f, 0.0f,
                                                               GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_HitAndRun)));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(static_cast<PursuitResultsDatum::PursuitResultsDatumType>(4), 0x7dbd5b34, 0.0f, 0.0f,
                                                               GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Damage)));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(static_cast<PursuitResultsDatum::PursuitResultsDatumType>(4), 0x2b1de2a9, 0.0f, 0.0f,
                                                               GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Resist)));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(static_cast<PursuitResultsDatum::PursuitResultsDatumType>(4), 0xb0ef5c12, 0.0f, 0.0f,
                                                               GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_OffRoad)));
#else
    PursuitResultsDatum::PursuitResultsDatumCheckType checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_Off;

    if (GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Racing)) {
        checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_On;
    }
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Check, 0x4b0ff103, 0.0f, 0.0f, checkType));

    checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_Off;
    if (GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Speeding)) {
        checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_On;
    }
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Check, 0x8ec1a6ec, 0.0f, 0.0f, checkType));

    checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_Off;
    if (GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Reckless)) {
        checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_On;
    }
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Check, 0x370f331d, 0.0f, 0.0f, checkType));

    checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_Off;
    if (GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Assault)) {
        checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_On;
    }
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Check, 0x8462a784, 0.0f, 0.0f, checkType));

    checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_Off;
    if (GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_HitAndRun)) {
        checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_On;
    }
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Check, 0xdff254ba, 0.0f, 0.0f, checkType));

    checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_Off;
    if (GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Damage)) {
        checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_On;
    }
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Check, 0x7dbd5b34, 0.0f, 0.0f, checkType));

    checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_Off;
    if (GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_Resist)) {
        checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_On;
    }
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Check, 0x2b1de2a9, 0.0f, 0.0f, checkType));

    checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_Off;
    if (GInfractionManager::Get().DidInfractionOccur(GInfractionManager::kInfraction_OffRoad)) {
        checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_On;
    }
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Check, 0xb0ef5c12, 0.0f, 0.0f, checkType));
#endif
}

void PostRacePursuitScreen::SetupMilestones() {
#ifdef EA_BUILD_A124
    // La A124 es la revision alpha (oraculo A124 0x298208, 720 B): el primer bloque pursuit no mira
    // PursuitRemaining (0x5392e4fd) y pasa bestVal >= goalVal directo como bool; el bucle tampoco trae
    // PursuitRemaining y el check es DonePendingEscape || Awarded como bool.
    if (GRaceStatus::Exists()) {
        GRaceParameters *raceParams = GRaceStatus::Get().GetRaceParameters();
        if (raceParams && raceParams->GetIsPursuitRace() && !FEDatabase->IsFinalEpicChase()) {
            PursuitResultsDatum::PursuitResultsDatumType type = PursuitResultsDatum::PursuitResultsDatumType_Milestone_Number;
            if (FEDatabase->IsMilestoneTimeFormat(raceParams->GetChallengeType())) {
                type = PursuitResultsDatum::PursuitResultsDatumType_Milestone_Time;
            }
            float bestVal = GManager::Get().GetBestValue(raceParams->GetChallengeType());
            float goalVal = raceParams->GetChallengeGoal();
            AddDatum(
                new ("PursuitResultsDatum", 0) PursuitResultsDatum(type, FEDatabase->GetChallengeHeaderHash(raceParams->GetLocalizationTag()), bestVal, goalVal, bestVal >= goalVal));
        } else {
            unsigned int binNumber = FEDatabase->GetCareerSettings()->GetCurrentBin();
            GMilestone *milestone = GManager::Get().GetFirstMilestone(false, binNumber);
            while (milestone) {
                PursuitResultsDatum::PursuitResultsDatumType type = PursuitResultsDatum::PursuitResultsDatumType_Milestone_Number;
                if (FEDatabase->IsMilestoneTimeFormat(milestone->GetTypeKey())) {
                    type = PursuitResultsDatum::PursuitResultsDatumType_Milestone_Time;
                }
                AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(type, FEDatabase->GetMilestoneHeaderHash(milestone->GetLocalizationTag()),
                                                 milestone->GetCurrentValue() >= 0.0f ? milestone->GetCurrentValue() : 0.0f,
                                                 milestone->GetRequiredValue(),
                                                 milestone->GetIsDonePendingEscape() || milestone->GetIsAwarded()));
                milestone = GManager::Get().GetNextMilestone(milestone, false, binNumber);
            }
        }
    }
#else
    if (GRaceStatus::Exists()) {
        GRaceParameters *raceParams = GRaceStatus::Get().GetRaceParameters();
        if (raceParams && raceParams->GetIsPursuitRace() && !FEDatabase->IsFinalEpicChase()) {
            PursuitResultsDatum::PursuitResultsDatumType type = PursuitResultsDatum::PursuitResultsDatumType_Milestone_Number;
            if (FEDatabase->IsMilestoneTimeFormat(raceParams->GetChallengeType())) {
                type = PursuitResultsDatum::PursuitResultsDatumType_Milestone_Time;
            }
            float bestVal = GManager::Get().GetBestValue(raceParams->GetChallengeType());
            float goalVal = raceParams->GetChallengeGoal();
            if (raceParams->GetChallengeType() == 0x5392e4fd) {
                type = PursuitResultsDatum::PursuitResultsDatumType_Milestone_Time_PursuitRemaining;
            }
            PursuitResultsDatum::PursuitResultsDatumCheckType checkType;
            if (bestVal >= goalVal) {
                checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_On;
            } else {
                checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_Off;
            }
            AddDatum(
                new ("PursuitResultsDatum", 0) PursuitResultsDatum(type, FEDatabase->GetChallengeHeaderHash(raceParams->GetLocalizationTag()), bestVal, goalVal, checkType));
        } else {
            unsigned int binNumber = FEDatabase->GetCareerSettings()->GetCurrentBin();
            GMilestone *milestone = GManager::Get().GetFirstMilestone(false, binNumber);
            while (milestone) {
                PursuitResultsDatum::PursuitResultsDatumType type = PursuitResultsDatum::PursuitResultsDatumType_Milestone_Number;
                if (FEDatabase->IsMilestoneTimeFormat(milestone->GetTypeKey())) {
                    type = PursuitResultsDatum::PursuitResultsDatumType_Milestone_Time;
                }
                if (milestone->GetTypeKey() == 0x5392e4fd) {
                    type = PursuitResultsDatum::PursuitResultsDatumType_Milestone_Time_PursuitRemaining;
                }
                PursuitResultsDatum::PursuitResultsDatumCheckType checkType;
                if (milestone->GetIsDonePendingEscape()) {
                    checkType = PursuitResultsDatum::PursuitResultsDatumCheckType_On;
                } else {
                    checkType = static_cast<PursuitResultsDatum::PursuitResultsDatumCheckType>(milestone->GetIsAwarded() ? 2 : 0);
                }
                AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(type, FEDatabase->GetMilestoneHeaderHash(milestone->GetLocalizationTag()),
                                                 milestone->GetCurrentValue(), milestone->GetRequiredValue(), checkType));
                milestone = GManager::Get().GetNextMilestone(milestone, false, binNumber);
            }
        }
    }
#endif
}

void PostRacePursuitScreen::SetupPursuit() {
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Time, 0x4d64888d, mPursuitData.mPursuitLength,
                                                          0.0f, PursuitResultsDatum::PursuitResultsDatumCheckType_Off));
    AddDatum(new ("PursuitResultsDatum", 0)
                 PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Number, 0xa999f6e2,
                                     static_cast<float>(mPursuitData.mNumCopsDamaged), 0.0f, PursuitResultsDatum::PursuitResultsDatumCheckType_Off));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Number, 0x23f6e732,
                                                          static_cast<float>(mPursuitData.mNumCopsDestroyed), 0.0f,
                                                          PursuitResultsDatum::PursuitResultsDatumCheckType_Off));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Number, 0x0291c816,
                                                          static_cast<float>(mPursuitData.mNumSpikeStripsDodged), 0.0f,
                                                          PursuitResultsDatum::PursuitResultsDatumCheckType_Off));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Number, 0x29daba15,
                                                          static_cast<float>(mPursuitData.mNumRoadblocksDodged), 0.0f,
                                                          PursuitResultsDatum::PursuitResultsDatumCheckType_Off));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Number, 0xd9bb7d2d,
                                                          static_cast<float>(mPursuitData.mCostToStateAchieved), 0.0f,
                                                          PursuitResultsDatum::PursuitResultsDatumCheckType_Off));
    AddDatum(new ("PursuitResultsDatum", 0) PursuitResultsDatum(PursuitResultsDatum::PursuitResultsDatumType_Number, 0xb7dfff96,
                                                          static_cast<float>(GInfractionManager::Get().GetNumInfractions()), 0.0f,
                                                          PursuitResultsDatum::PursuitResultsDatumCheckType_Off));
}

void PostRacePursuitScreen::NotificationMessage(u32 msg, FEObject *pObject, u32 Param1, u32 Param2) {
    ArrayScrollerMenu::NotificationMessage(msg, pObject, Param1, Param2);
    switch (msg) {
        case 0x406415E3:
            if (TheGameFlowManager.IsInFrontend()) {
                if (FEDatabase->IsQuickRaceMode()) {
                    cFEng::Get()->QueuePackageSwitch("ChallengeSeries.fng", 0, 0, false);
                } else {
                    cFEng::Get()->QueuePackageSwitch("Car_Select.fng", 0, 0, false);
                }
            } else {
                if (GRaceStatus::Exists() && GRaceStatus::Get().GetPlayMode() == GRaceStatus::kPlayMode_Racing) {
                    GRacerInfo &info = GRaceStatus::Get().GetRacerInfo(0);
                    if (info.IsFinishedRacing() && GRaceStatus::Get().GetRaceParameters()->GetIsPursuitRace()) {
                        if (FEDatabase->IsChallengeMode() && MemoryCard::GetInstance()->ShouldDoAutoSave(false)) {
                            MemcardEnter(nullptr, nullptr, 0x100B1, nullptr, nullptr, 0, 0);
                        } else {
                            new EQuitToFE(static_cast<eGarageType>(1), nullptr);
                        }
                        break;
                    }
                }
                new EUnPause();
            }
            break;
        case 0xC519BFC3:
            if (TheGameFlowManager.IsInFrontend()) {
                if (mPostPursuitScreenMode == POSTPURSUITSCREENMODE_INFRACTIONS) {
                    mPostPursuitScreenMode = POSTPURSUITSCREENMODE_PURSUIT;
                } else {
                    mPostPursuitScreenMode = POSTPURSUITSCREENMODE_INFRACTIONS;
                }
            } else {
                if (mPostPursuitScreenMode == POSTPURSUITSCREENMODE_INFRACTIONS) {
                    mPostPursuitScreenMode = POSTPURSUITSCREENMODE_MILESTONES;
                } else if (mPostPursuitScreenMode == POSTPURSUITSCREENMODE_MILESTONES) {
                    mPostPursuitScreenMode = POSTPURSUITSCREENMODE_PURSUIT;
                } else {
                    mPostPursuitScreenMode = POSTPURSUITSCREENMODE_INFRACTIONS;
                }
            }
            Initialize();
            break;
    }
}

PostRaceMilestonesScreen::PostRaceMilestonesScreen(ScreenConstructorData *sd)
    : MenuScreen(sd), mBountyEarned(0.0f), mCurrMilestoneIndex(-1), mCurrMilestoneScriptHash(0), mCopDestructionBountyShown(false) {
    mpDataBigIcon = FEngFindImage(GetPackageName(), 0x14564FB9);
}

PostRaceMilestonesScreen::~PostRaceMilestonesScreen() {}

void PostRaceMilestonesScreen::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x35f8620b:
            StartBountyAnimations(false);
            break;
        case 0xd3c3de7:
            if (!mCopDestructionBountyShown) {
                mCopDestructionBountyShown = true;
                if (PostRacePursuitScreen::GetPursuitData().mRepAchievedCopDestruction > 0) {
                    StartBountyAnimations(true);
                    break;
                }
            }

            if (GRaceStatus::Exists() && GRaceStatus::Get().GetRaceParameters() && GRaceStatus::Get().GetRaceParameters()->GetIsPursuitRace()
#ifndef EA_BUILD_A124 // la A124 no tiene la persecucion final
                && !FEDatabase->IsFinalEpicChase()
#endif
            ) {
                StartChallengeAnimations();
            } else {
                StartMilestoneAnimations();
            }
            break;
        case 0xc98356ba:
            if (FEngIsScriptSet(mpDataBigIcon, FEHASH_APPEAR) && !FEngIsScriptRunning(mpDataBigIcon, FEHASH_APPEAR)) {
                FEngSetScript(mpDataBigIcon, mCurrMilestoneScriptHash, true);
            }
            break;
        case 0x406415e3:
            cFEng::Get()->QueuePackagePop(1);
            new EShowResults(FERESULTTYPE_PURSUIT
#ifndef EA_BUILD_A124
                ,
                false
#endif
            );
            break;
    }
}

bool PostRaceMilestonesScreen::StartMilestoneAnimations() {
    mCurrMilestoneIndex++;
    const GMilestone *const milestone = PostRacePursuitScreen::GetPursuitData().GetMilestone(mCurrMilestoneIndex);
    if (milestone) {
        char descStr[32];
        char outputStr[64];
        FEDatabase->SetMilestoneDescriptionString(descStr, milestone->GetTypeKey(), milestone->GetCurrentValue(), milestone->GetRequiredValue()
#ifndef EA_BUILD_A124
                                                  ,
                                                  false
#endif
        );
        bSNPrintf(outputStr, 64, "%s: %s", GetTranslatedString(FEDatabase->GetMilestoneHeaderHash(milestone->GetLocalizationTag())), descStr);
        StartAnimations(true, milestone->GetTypeKey(), milestone->GetBounty(), outputStr);
        return true;
    }

    StartMilestoneDoneAnimations();
    return false;
}

bool PostRaceMilestonesScreen::StartChallengeAnimations() {
    mCurrMilestoneIndex++;
    if (mCurrMilestoneIndex < 1 && GRaceStatus::Exists()) {
        GRaceParameters *raceParams = GRaceStatus::Get().GetRaceParameters();
        if (raceParams && raceParams->GetIsPursuitRace()
#ifndef EA_BUILD_A124
            && !FEDatabase->IsFinalEpicChase()
#endif
        ) {
            float currVal = GManager::Get().GetBestValue(raceParams->GetChallengeType());
            float goalVal = raceParams->GetChallengeGoal();
            char descStr[32];
            char outputStr[64];
            FEDatabase->SetMilestoneDescriptionString(descStr, raceParams->GetChallengeType(), currVal, goalVal
#ifndef EA_BUILD_A124
                                                      ,
                                                      false
#endif
            );
            bSNPrintf(outputStr, 64, "%s: %s", GetTranslatedString(FEDatabase->GetChallengeHeaderHash(raceParams->GetLocalizationTag())), descStr);
            StartAnimations(true, raceParams->GetChallengeType(), static_cast<float>(raceParams->GetReputation()), outputStr);
            return true;
        }
    }
    StartMilestoneDoneAnimations();
    return false;
}

bool PostRaceMilestonesScreen::StartBountyAnimations(bool copDestruction) {
    unsigned int key;
    float bountyEarned;
    char outputStr[64];
    if (!copDestruction) {
        key = 0x33fa23a;
        bountyEarned = static_cast<float>(PostRacePursuitScreen::GetPursuitData().mRepAchievedNormal);
        bSNPrintf(outputStr, 64, "%s", GetTranslatedString(0x4d64888d));
    } else {
        key = 0x4fc942ca;
        bountyEarned = static_cast<float>(PostRacePursuitScreen::GetPursuitData().mRepAchievedCopDestruction);
        bSNPrintf(outputStr, 64, "%s: %$d", GetTranslatedString(0x23f6e732), PostRacePursuitScreen::GetPursuitData().mNumCopsDestroyed);
    }
    StartAnimations(false, key, bountyEarned, outputStr);
    return true;
}

void PostRaceMilestonesScreen::StartAnimations(bool isMilestone, int typeKey, float bountyEarned, const char *const descriptionStr) {
    mBountyEarned += bountyEarned;
    SetMilestoneAnimationScriptHash(isMilestone, typeKey);
    FEngSetTextureHash(mpDataBigIcon, FEDatabase->GetMilestoneIconHash(typeKey, isMilestone));
    FEPrintf(GetPackageName(), 0xe526d0d2, "%s", descriptionStr);
    if (bountyEarned > 0.0f) {
        FEngSetVisible(GetPackageName(), 0xe1045a4f);
    } else {
        FEngSetInvisible(GetPackageName(), 0xe1045a4f);
    }
    FEPrintf(GetPackageName(), 0xe1045a4f, "%s: %$0.0f", GetTranslatedString(0x29b1b96a), bountyEarned);
    FEPrintf(GetPackageName(), 0x324f7792, "%s: %$0.0f", GetTranslatedString(0x5ccf949a), mBountyEarned);
    FEngSetScript(mpDataBigIcon, FEHASH_APPEAR, true);
}

void PostRaceMilestonesScreen::StartMilestoneDoneAnimations() {
    FEngSetScript(mpDataBigIcon, 0x16a259, true);
    FEngSetScript(GetPackageName(), 0xe526d0d2, 0x33113ac, true);
    FEngSetScript(GetPackageName(), 0xe1045a4f, 0x33113ac, true);
    FEngSetScript(GetPackageName(), 0x962b9c62, FEHashUpper("POS2"), true);
    FEngSetScript(GetPackageName(), 0xec85c7e4, FEHashUpper("POS2"), true);
}

bool PostRaceMilestonesScreen::SetMilestoneAnimationScriptHash(bool isMilestone, int type) {
    switch (type) {
        case 0x2377e50d:
            mCurrMilestoneScriptHash = FEHashUpper("POS1");
            break;
        case 0x33fa23a:
            if (isMilestone) {
                mCurrMilestoneScriptHash = FEHashUpper("POS7");
            } else {
                mCurrMilestoneScriptHash = FEHashUpper("POS0");
            }
            break;
        case static_cast<int>(0xA61CAC24):
            mCurrMilestoneScriptHash = FEHashUpper("POS2");
            break;
        case static_cast<int>(0xFD989A3A):
            mCurrMilestoneScriptHash = FEHashUpper("POS3");
            break;
        case static_cast<int>(0xEB45F99D):
            mCurrMilestoneScriptHash = FEHashUpper("POS4");
            break;
        case static_cast<int>(0xCDF36FC2):
            mCurrMilestoneScriptHash = FEHashUpper("POS5");
            break;
        case static_cast<int>(0x850A64BC):
            mCurrMilestoneScriptHash = FEHashUpper("POS6");
            break;
        case 0x4fc942ca:
            mCurrMilestoneScriptHash = FEHashUpper("POS00");
            break;
        case 0x5392e4fd:
            mCurrMilestoneScriptHash = FEHashUpper("POS8");
            break;
        default:
            mCurrMilestoneScriptHash = 0;
            break;
    }
    return mCurrMilestoneScriptHash != 0;
}
