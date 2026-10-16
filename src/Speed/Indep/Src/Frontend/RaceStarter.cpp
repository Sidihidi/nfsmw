#include "Speed/Indep/Src/Frontend/RaceStarter.hpp"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
// Las funciones online de RaceStarter solo existen en los builds que las traen:
// A124 (PS2 alpha) y PC. El X360 milestone no tiene RaceStarter en absoluto y
// GC/SLUS van sin online (ONLINE_SUPPORT 0).
#if ONLINE_SUPPORT && (defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32))
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/NetworkCore.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/Src/Misc/Main.hpp"
#include "Speed/PSX2/Src/Online/PS2Isp.hpp"
#include <libmc.h>
#endif
extern "C" {
void TagFieldSetString(char *buffer, int bufferSize, const char *name, const char *value);
int32 DirtyAddrFromHostAddr(DirtyAddrT *dirtyAddress, const void *hostAddress);
}
void THREAD_yield(int a);
extern int OnlineIsServer;
extern int SkipFEOnlineNumExpectedRemotePlayers;
extern uint32 SkipFEOnlineHostAddr;
extern uint32 SkipFEOnlineClientAddr[3];
#endif

extern int OnlineEnabled;
extern int SkipFEPovType1;
extern int SkipFEPoint2Point;
extern int SkipFEDamageEnabled;
extern int SkipFEControllerConfig1;
extern int SkipFEControllerConfig2;
extern ePlayerSettingsCameras GetPlayerCameraFromPOVType(POVTypes pov);

// Lineas 11-14 del RaceStarter.cpp original (mapa $LM A124; 108 B en A124 y
// SLUS identicos salvo la direccion del jtbl). El caso Circuit cae SIN break
// al caso Knockout: el original compila exactamente asi (bnez sobre IsLapKO
// con los dos brazos devolviendo 3; el ternario con brazos iguales se plegaria
// y no saldria la rama — verificado con el ee-gcc real).
GRace::Type FETypeToRaceType(GRace::Type fe_type) {
    switch (fe_type) {
        case GRace::kRaceType_Circuit:
            if (FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->IsLapKO)
                return GRace::kRaceType_Knockout;
        case GRace::kRaceType_Knockout:
            return GRace::kRaceType_Knockout;
        case GRace::kRaceType_None:
        case GRace::kRaceType_P2P:
        case GRace::kRaceType_Drag:
            return GRace::kRaceType_Circuit;
    }
    return GRace::kRaceType_P2P;
}

void RaceStarter::AddTrafficCars() {}

void RaceStarter::AddCopDriverInfos(int ncops) {}


void RaceStarter::StartRace() {
    TheRaceParameters.InitWithDefaults();
    TheRaceParameters.bOnlineRace = false;
    TheRaceParameters.TrackNumber = 2000;
    for (int i = 0; i < TheRaceParameters.NumPlayerCars; i++) {
        SetControllerConfig(FEDatabase->GetPlayerSettings(i)->Config,
                            (JoystickPort)FEDatabase->GetPlayersJoystickPort(i));
    }
    TheGameFlowManager.UnloadFrontend();
}

#if ONLINE_SUPPORT && (defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32))
// SetupOnlineRace__11RaceStarter @ 0x29e820 (A124, 652 B; PS2 alpha y PC 1.3, GC/SLUS no lo traen).
void RaceStarter::SetupOnlineRace() {
    GRaceParameters *race_params = GRaceDatabase::Get().GetRaceFromHash(
        FEDatabase->GetOnlineSettings()->GetRaceSettings()->EventHash);
    TheRaceParameters.bOnlineRace = true;
    TheRaceParameters.TrackNumber = 2000;
    TheRaceParameters.TrackDirection = (eTrackDirection)FEDatabase->GetOnlineSettings()->GetRaceSettings()->TrackDirection;
    TheRaceParameters.TrafficOncoming = 0.5f;
    TheRaceParameters.TrafficDensity = eTRAFFICDENSITY_LOW;
    TheRaceParameters.RaceType = RACE_TYPE_NONE;
    TheRaceParameters.Point2Point = 0;
    TheRaceParameters.NumLapsInRace = FEDatabase->GetOnlineSettings()->RaceMode == GRace::kRaceType_Circuit
                                           ? FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps
                                           : 1;
    TheRaceParameters.NumPlayerCars = 1;
    TheRaceParameters.NumAICars = 0;
    if (FEDatabase->GetOnlineSettings()->GetRaceSettings()->CopDensity != 0) {
        TheRaceParameters.nMaxCops = 9;
    } else {
        TheRaceParameters.nMaxCops = 0;
    }
    TheRaceParameters.DamageEnabled = true;
    TheRaceParameters.OpponentStrength = eOPPONENTSTRENGTH_LOW;
    TheRaceParameters.CopStrength = eOPPONENTSTRENGTH_LOW;
    TheRaceParameters.PlayerStartPosition[0] = TheRaceParameters.NumAICars + 1;
    TheRaceParameters.PlayerStartPosition[1] = TheRaceParameters.NumAICars + 2;
    TheRaceParameters.BoostScale[0] = 1.0f;
    TheRaceParameters.BoostScale[1] = 1.0f;
    TheRaceParameters.NumDriverInfo = 0;
    for (int i = 0; i < TheRaceParameters.NumPlayerCars; i++) {
        TheRaceParameters.PlayerJoyports[i] = FEDatabase->PlayerJoyports[i];
    }
    for (int i = 0; i < TheRaceParameters.NumPlayerCars; i++) {
        RideInfo ride;
        FEDatabase->BuildCurrentRideForPlayer(i, &ride);
        DriverInfo di(i, DRIVER_TYPE_PLAYER, &ride, "Free Roamer", -2, 0,
                      TheRaceParameters.PlayerStartPosition[i]);
        TheRaceParameters.AddDriverInfo(di);
    }
    AddCopDriverInfos(TheRaceParameters.nMaxCops);
    AddTrafficCars();
    GRaceCustom *custom = GRaceDatabase::Get().AllocCustomRace(race_params);
    custom->SetNumOpponents(0);
    custom->SetCopsEnabled(false);
    custom->SetTrafficDensity(0);
    custom->SetNumLaps(FEDatabase->GetOnlineSettings()->RaceMode == GRace::kRaceType_Circuit
                           ? FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps
                           : 1);
    custom->SetReversed(FEDatabase->GetOnlineSettings()->GetRaceSettings()->TrackDirection != 0);
#ifdef EA_BUILD_A124
    GRaceDatabase::Get().SetStartupRace(custom, GRace::kRaceContext_Online);
#else
    GRaceDatabase::Get().SetStartupRace(custom, GRace::kRaceContext_TimeTrial);
#endif
    GRaceDatabase::Get().FreeCustomRace(custom);
}
#endif

void RaceStarter::SetControllerConfig(int config, JoystickPort port) {
}

void RaceStarter::StartSkipFERace() {
    int track_num = SkipFETrackNumber;

    if (OnlineEnabled != 0) {
        TheRaceParameters.bOnlineRace = true;
    }

    TheRaceParameters.NumPlayerCars = SkipFENumPlayerCars;

    TheRaceParameters.TrackNumber = track_num;
    TheRaceParameters.TrackDirection = SkipFETrackDirection;

    TheRaceParameters.TrafficOncoming = SkipFETrafficOncoming;

    TheRaceParameters.RaceType = (RaceTypes)SkipFERaceType;
    TheRaceParameters.Point2Point = SkipFEPoint2Point;
    TheRaceParameters.NumLapsInRace = SkipFENumLaps;
    TheRaceParameters.NumPlayerCars = SkipFENumPlayerCars;
    TheRaceParameters.NumAICars = SkipFENumAICars;
    TheRaceParameters.nMaxCops = SkipFEMaxCops;
    TheRaceParameters.DamageEnabled = SkipFEDamageEnabled != 0;

    TheRaceParameters.OpponentStrength = (eOpponentStrength)SkipFEDifficulty;
    TheRaceParameters.CopStrength = (eOpponentStrength)SkipFEDifficulty;

    TheRaceParameters.PlayerStartPosition[0] = SkipFENumAICars + 1;
    TheRaceParameters.PlayerStartPosition[1] = SkipFENumAICars + 2;

    if (SkipFEControllerConfig1 != -1) {
        FEDatabase->GetOptionsSettings()->ThePlayerSettings[0].Config =
            (eControllerConfig)SkipFEControllerConfig1;
    }
    if (SkipFEControllerConfig2 != -1) {
        FEDatabase->GetOptionsSettings()->ThePlayerSettings[0].Config =
            (eControllerConfig)SkipFEControllerConfig2;
    }
    FEDatabase->GetPlayerSettings(0)->CurCam = GetPlayerCameraFromPOVType((POVTypes)SkipFEPovType1);
    FEDatabase->GetPlayerSettings(1)->CurCam = GetPlayerCameraFromPOVType((POVTypes)SkipFEPovType1);
    for (int i = 0; i < TheRaceParameters.NumPlayerCars; i++) {
        TheRaceParameters.PlayerJoyports[i] = FEDatabase->PlayerJoyports[i];
    }
    TheRaceParameters.BoostScale[0] = 1.0f;
    TheRaceParameters.BoostScale[1] = 1.0f;
    TheRaceParameters.NumDriverInfo = 0;
#if ONLINE_SUPPORT && defined(EA_BUILD_A124) // la A124 arranca la partida en linea desde aqui
    if (OnlineEnabled != 0) {
        InitSkipFEOnline();
    } else
#endif
    if (TheGameFlowManager.IsInFrontend()) {
        TheGameFlowManager.UnloadFrontend();
    } else {
        TheGameFlowManager.LoadTrack();
    }
}

#if ONLINE_SUPPORT && (defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32))
// InitSkipFEOnline__11RaceStarter @ 0x29ed00 (A124, 604 B; lineas 220-248 del original).
// El arranque PS2 lleva sceMcInit + pila de red en main memory + PS2Isp con CFGPTH=mc:0/SLOGIN=FALSE;
// PC (SPEED_EXE_1_3 @0x56C3E0, 335 B) inicializa NetworkCore y espera a tener IP.
void RaceStarter::InitSkipFEOnline() {
#ifdef EA_PLATFORM_PLAYSTATION2
    sceMcInit();
    DoWithMainMemoryStack((void *)NetworkCore::StaticInit, 0, 0);
    PS2Isp &ps2isp = PS2Isp::Instance();
    char params[64] = "";
    TagFieldSetString(params, 64, "CFGPTH", "mc:0");
    TagFieldSetString(params, 64, "SLOGIN", "FALSE");
    ps2isp.Start(nullptr, PS2ISP_CONTEXT_LOADCFG, true, params);
    while (!ps2isp.IsFinished()) {
        ps2isp.GetStatus();
        NetworkCore::DoNetworkProcessing();
        THREAD_yield(0);
    }
    ps2isp.Finish();
#else
    NetworkCore::Instance().Init();
    while (NetworkCore::MyIPAddress() == 0) {
        THREAD_yield(0);
    }
#endif
    ConnectionCore::Instance().Init(SkipFEOnlineNumExpectedRemotePlayers + 1, nullptr, nullptr);
    if (OnlineIsServer) {
        ConnectionCore::Instance().HostSession(NetworkCore::MyIPAddress(), 3);
    } else {
        ConnApiUserInfoT userInfo;
        bStrCpy(userInfo.strName, "TheHost");
        userInfo.uLocalAddr = SkipFEOnlineHostAddr;
        userInfo.uAddr = SkipFEOnlineHostAddr;
        DirtyAddrFromHostAddr(&userInfo.DirtyAddr, &SkipFEOnlineHostAddr);
        ConnectionCore::Instance().JoinSession(userInfo, SkipFEOnlineHostAddr, 3);
    }
    for (int i = 0; i < 3; i++) {
        if (SkipFEOnlineClientAddr[i] != 0) {
            ConnApiUserInfoT userInfo;
            bSPrintf(userInfo.strName, "Client%d", i);
            userInfo.uLocalAddr = SkipFEOnlineClientAddr[i];
            userInfo.uAddr = SkipFEOnlineClientAddr[i];
            DirtyAddrFromHostAddr(&userInfo.DirtyAddr, &userInfo.uAddr);
            ConnectionCore::Instance().AddPlayer(userInfo);
        }
    }
    while (ConnectionCore::Instance().GetNumConnectedPlayers() < SkipFEOnlineNumExpectedRemotePlayers) {
        NetworkCore::DoNetworkProcessing();
        THREAD_yield(0);
    }
    TheOnlineManager.StartLobby();
}
#endif

void RaceStarter::StartCareerFreeRoam() {
    if (TheGameFlowManager.IsInFrontend()) {
        TheGameFlowManager.UnloadFrontend();
    } else {
        TheGameFlowManager.LoadTrack();
    }
}
