#include "OnlineManager.hpp"

#if ONLINE_SUPPORT
// El gestor online de verdad: solo existe en el prototipo (A124). Las funciones salen del asm de la A124, con las
// locales de sus stabs y las lineas de su .mdebug; GameCube y SLUS solo tienen el gestor vacio de mas abajo.
// PC (speed.exe) tiene otra revision: lo que cambia va bajo EA_PLATFORM_WIN32 con la direccion de la funcion.
#include "Speed/Indep/Src/Animation/AnimPlayer.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineDisconnect.hpp"
#include "Speed/Indep/Src/Frontend/RaceStarter.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyOnlineRaceOver.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IOnlinePlayer.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Main/Scheduler.h"
#include "Speed/Indep/Src/Misc/Base64.hpp"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Online/InGame/Client.hpp"
#include "Speed/Indep/Src/Online/InGame/OnlineTimer.hpp"
#include "Speed/Indep/Src/Online/InGame/Server.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"
#include "Speed/Indep/Src/Online/NetworkCore.hpp"
#include "Speed/Indep/Src/Online/OnlineCfg.hpp"
#include "Speed/Indep/Src/Online/SmartBitstream.hpp"
#include "Speed/Indep/Src/Online/VoiceCore.hpp"
#include "Speed/Indep/Src/World/RaceParameters.hpp"
#include "Speed/Indep/Src/World/World.hpp"
#ifdef EA_PLATFORM_WIN32
#include "Speed/Indep/Src/Camera/CameraAI.hpp"
#include "Speed/Indep/Src/Camera/ICE/ICEManager.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Generated/Events/EEngineBlown.hpp"
#include "Speed/Indep/Src/Generated/Events/EMomentStrm.hpp"
#endif
#include "Speed/Indep/bWare/Inc/SpeedScript.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/PSX2/Src/Online/PlatformNetworkCore.hpp"
#elif defined(EA_PLATFORM_XENON)
#include "Speed/Indep/Src/Online/NetworkCore.hpp"
// XDK (winbase.h, GetCommandLine): <xtl.h> aqui rompe las unidades que siguen en zWorld (CarRender.cpp).
extern "C" char *GetCommandLineA();
char *bStrChr(const char *string, int character); // bWare (Strings.cpp); Strings.hpp no la declara
#endif

extern int OnlineIsServer;
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
extern "C" int LobbyApiInfoInt(LobbyApiRefT *lobbyRef, int selector);
#endif

#ifdef EA_BUILD_A124
// gcdatos21: en la A124 TheOnlineManager va delante del bloque de trampas
// (.data objetivo de zWorld: TheOnlineManager en +0x3d0 y
// ONLINE_SCREENPRINT_CHEAT_REPORT tras el, en +0x1ad4; los stabs dan el mismo
// orden). El bloque movido es POD con inicializador constante;
// TheOnlineManager conserva su orden relativo de ctor.
OnlineManager TheOnlineManager;

int ONLINE_SCREENPRINT_CHEAT_REPORT = 1;
int ONLINE_CHEAT_FALSESTART = 0;
float ONLINE_CHEAT_SLOW_WORLDTIMER = 1.0f;
int ONLINE_CHEAT_NOSEND_CRC = 0;
int ONLINE_CHEAT_IGNORE_TIMEUP = 0;
int ONLINE_CHEAT_IGNORETOOSLOW = 0;
int ONLINE_CHEAT_IGNORE_SELFDISCONNECT = 1;
int ONLINE_CHEATSCORE_THRESHOLD = 51;
float MinCheatFreq[16] = {0.0f, 10.0f, 0.0f, 5.0f, 0.0f, 5.0f, 0.0f, 0.0f, 2.0f, 10.0f, 1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f};
#else
int ONLINE_SCREENPRINT_CHEAT_REPORT = 1;
int ONLINE_CHEAT_FALSESTART = 0;
float ONLINE_CHEAT_SLOW_WORLDTIMER = 1.0f;
int ONLINE_CHEAT_NOSEND_CRC = 0;
int ONLINE_CHEAT_IGNORE_TIMEUP = 0;
int ONLINE_CHEAT_IGNORETOOSLOW = 0;
int ONLINE_CHEAT_IGNORE_SELFDISCONNECT = 1;
int ONLINE_CHEATSCORE_THRESHOLD = 51;
float MinCheatFreq[16] = {0.0f, 10.0f, 0.0f, 5.0f, 0.0f, 5.0f, 0.0f, 0.0f, 2.0f, 10.0f, 1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f};

OnlineManager TheOnlineManager;
#endif

void OnlineManager::InitQuantizers() {
    float min_value;
    float max_value;

    PositionQuantizerX.Init("PositionQuantizerX", -4800.0f, 1792.0f, 0.01f);
    PositionQuantizerY.Init("PositionQuantizerY", -15.0f, 300.0f, 0.01f);
    max_value = 1.0f;
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    PositionQuantizerZ.Init("PositionQuantizerZ", -1664.0f, 4840.0f, 0.01f); // PC (speed.exe 0x734D80)
#else
    PositionQuantizerZ.Init("PositionQuantizerZ", -1664.0f, 4224.0f, 0.01f);
#endif
    min_value = 0.0f;
    VelocityQuantizer.Init("VelocityQuantizer", -125.0f, 125.0f, 0.1f);
    MatrixQuantizer.Init("MatrixQuantizer", -1.0f, max_value, 0.01f);
    AngleQuantizer.Init("AngleQuantizer", -0.5f, 0.5f, 0.0025f);
    AccelerationQuantizer.Init("AccelerationQuantizer", -25.0f, 25.0f, 0.1f);
    AVelocityQuantizer.Init("AVelocityQuantizer", -5.0f, 5.0f, 0.01f);
    ControlQuantizer.Init("ControlQuantizer", min_value, max_value, 0.15f);
    QuantFloatSimTime.Init("QuantFloatSimTime", min_value, 5400.0f, 0.001f);
    QuantCarSlotID.Init("QuantCarSlotID", 0, 0x8b);
    QuantCarPartIndex.Init("QuantPartIndex", 0, 16000);
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    QuantCarType.Init("QuantCarType", 0, 0x54); // PC (speed.exe 0x734D80)
#else
    QuantCarType.Init("QuantCarType", 0, 0x4e);
#endif
    QuantInt2Bit.Init("QuantInt2Bit", 0, 3);
    QuantInt3Bit.Init("QuantInt3Bit", 0, 7);
    QuantInt4Bit.Init("QuantInt4Bit", 0, 0xf);
    QuantFloatTime.Init("QuantFloatTime", min_value, 14400.0f, 0x18);
}

OnlineRacer::OnlineRacer(int8 driver_number, bool is_server, const char *persona)
    : ExtrapolatedCar(0)
    , CarCustomization()
    , FinishedRaceStats() {
    DriverNumber = driver_number;
    PlayerID = -1;
    LastSpamRealTime = -1.0f;
    Reputation = 100;
    RaceScore = 0;
    State = OPS_DISCONNECTED;
    bIsServer = is_server;
    bShouldRestart = false;
    PhysicsDataCRC = 0;
    BadnessReason = 0;
    EndRaceCountdown = -1.0f;
    SyncScoreMsgID = 0;
    BadnessCountdown.ResetLow();
    GraceCountdown.ResetLow();
    ClearCheatInfo();
    DisconnectTime.UnSet();
    bMemSet(&FinishedRaceStats, 0, 0xc0);
    if (SkipFE) {
        SetPersona("SkipFE");
    } else if (persona) {
        SetPersona(persona);
    } else {
        bMemSet(Persona, 0, 0x10);
    }
}

OnlineRacer::~OnlineRacer() {}

void OnlineRacer::SetPersona(const char *persona) {
    bMemCpy(Persona, persona, 0x10);
}

bool OnlineRacer::IsFinishedRacing() {
    if (!IsConnected()) {
        return true;
    }
    if (GetState() == OPS_FINISHED) {
        return true;
    }
    return TheOnlineManager.GetState() >= OLS_RACE_END && GetState() < OPS_RACING;
}

void OnlineRacer::SetRaceScore(int score) {
    if (RaceScore != score) {
        RaceScore = score;
    }
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (speed.exe 0x735040): dos if anidados (la coma de la A124 da otro codigo con MSVC).
void OnlineRacer::ChangeState(eOnlineRacerState new_state) {
    if (new_state != State) {
        State = new_state;
        if (new_state == OPS_RACING) {
            BadnessCountdown.SetTime(20.0f);
            GraceCountdown.SetTime(6.0f);
            BadnessReason = 0;
        }
    }
}
#else
void OnlineRacer::ChangeState(eOnlineRacerState new_state) {
    if ((new_state != State) && (State = new_state, new_state == OPS_RACING)) {
        BadnessCountdown.SetTime(20.0f);
        GraceCountdown.SetTime(6.0f);
        BadnessReason = 0;
    }
}
#endif

#ifdef EA_PLATFORM_WIN32
// PC (speed.exe 0x754C30): sin nombre en ningun binario. Busca el GRacerInfo de este corredor: el IPlayer cuyo
// IOnlinePlayer lleva este OnlineRacer; devuelve tambien su simable.
GRacerInfo *OnlineRacer::Win32Unknown754C30(ISimable **simable) {
    if (!GRaceStatus::Exists()) {
        return nullptr;
    }
    for (IPlayer *player = IPlayer::First(PLAYER_ALL); player; player = player->Next(PLAYER_ALL)) {
        IOnlinePlayer *online_player;
        if (player->QueryInterface(&online_player) && this == online_player->GetOnlineRacer()) {
            *simable = player->GetSimable();
            if (*simable) {
                return GRaceStatus::Get().GetRacerInfo(*simable);
            }
        }
    }
    return nullptr;
}

// PC (speed.exe 0x754CF0): en vez de las estadisticas de la A124 manda por la red el GRacerInfo del corredor (tiempos,
// vueltas o parciales) y lanza la camara lenta, el estado de sonido y el momento de la llegada.
void OnlineRacer::Finish(int nRank, bool bBlinkBlinkPoof, int raceFinishReason) {
    SmartBitStream data;
    data.AddByte(DriverNumber);
    data.AddByte(static_cast<uint8>(nRank));
    bool value = bBlinkBlinkPoof || raceFinishReason == 0xb;
    data.AddByte(value);
    data.AddByte(static_cast<uint8>(raceFinishReason));
    data.AddRawData(reinterpret_cast<const char *>(CheatTally), 0x20);
    ISimable *simable;
    GRacerInfo *racer_info = Win32Unknown754C30(&simable);
    if (racer_info) {
        data.AddBool(true);
        data.AddFloat(racer_info->GetRaceTime());
        data.AddFloat(static_cast<float>(racer_info->GetLapsCompleted()));
        data.AddFloat(racer_info->mPctRaceComplete);
        data.AddFloat(racer_info->mDistanceDriven);
        data.AddFloat(racer_info->mTopSpeed);
        data.AddFloat(racer_info->mPoundsNOSUsed);
        data.AddFloat(racer_info->mZeroToSixtyTime);
        data.AddFloat(racer_info->mQuarterMileTime);
        data.AddInt(racer_info->mNumPerfectShifts);
        if (GRaceStatus::Get().GetRaceType() == GRace::kRaceType_Circuit) {
            for (int i = 0; i < GRaceStatus::Get().GetRaceParameters()->GetNumLaps(); i++) {
                data.AddFloat(GRaceStatus::Get().GetLapTime(i, racer_info->GetIndex(), false));
            }
        } else {
            for (int i = 0; i < 4; i++) {
                data.AddFloat(racer_info->GetSplitTime(i));
                data.AddInt(racer_info->GetSplitRanking(i));
            }
        }
    } else {
        data.AddBool(false);
    }
    if (Online::IsInitialized()) {
        Online::SignalDriverFinish(data);
    }
    ChangeState(OPS_FINISHED);
    CameraAI::StartCinematicSlowdown(EVIEW_PLAYER1, 0.75f);
    SetSoundControlState(true, SNDSTATE_FADEOUT, "CinemSlow");
    new EMomentStrm(UMath::Vector4::kZero, UMath::Vector4::kZero, UMath::Vector4::kZero, 0, nullptr, 0x9FE1EE17);
    TheICEManager.SetUseRealTime(true);
}

// PC (speed.exe 0x758B30): lee lo que manda Finish y lo aplica al GRacerInfo del corredor; el rango y el parpadeo que
// llegan no se usan (FinishDriver recibe el rango del GRacerInfo y false).
void OnlineRacer::SignalFinish(SmartBitStream &data) {
    uint8 rank;
    uint8 blinkPoof;
    uint8 finishReason;

    data.GetByte(rank);
    data.GetByte(blinkPoof);
    data.GetByte(finishReason);

    uint16 remote_cheat_tally[16];

    data.GetRawData(reinterpret_cast<char *>(remote_cheat_tally), sizeof(remote_cheat_tally));

    ISimable *simable;
    GRacerInfo *racer_info = Win32Unknown754C30(&simable);
    if (racer_info) {
        switch (finishReason) {
        case 8:
            if (simable) {
                new EEngineBlown(simable->GetInstanceHandle());
            }
            break;
        case 9:
            racer_info->TotalVehicle();
            break;
        case 0xb:
            break;
        default:
            racer_info->FinishRace();
            break;
        }
    }

    if (data.GetBool()) {
        if (racer_info) {
            float race_time = data.GetFloat();
            racer_info->GetRaceTimer().SetTime(race_time);
            racer_info->mLapsCompleted = static_cast<int>(data.GetFloat());
            racer_info->mPctRaceComplete = data.GetFloat();
            racer_info->mDistanceDriven = data.GetFloat();
            racer_info->mTopSpeed = data.GetFloat();
            racer_info->mPoundsNOSUsed = data.GetFloat();
            racer_info->mZeroToSixtyTime = data.GetFloat();
            racer_info->mQuarterMileTime = data.GetFloat();
            racer_info->mNumPerfectShifts = data.GetInt();
            if (GRaceStatus::Get().GetRaceType() == GRace::kRaceType_Circuit) {
                for (int i = 0; i < GRaceStatus::Get().GetRaceParameters()->GetNumLaps(); i++) {
                    GRaceStatus::Get().SetLapTime(i, racer_info->GetIndex(), data.GetFloat());
                }
            } else {
                for (int i = 0; i < 4; i++) {
                    racer_info->mSplitTimes[i] = data.GetFloat();
                    racer_info->mSplitRankings[i] = data.GetInt();
                }
            }
        } else {
            data.GetFloat();
            data.GetFloat();
            data.GetFloat();
            data.GetFloat();
            data.GetFloat();
            data.GetFloat();
            data.GetFloat();
            data.GetInt();
            for (int i = 0; i < 4; i++) {
                data.GetFloat();
                data.GetInt();
            }
        }
    }

    for (int i = 0; i < 16; i++) {
        CheatTally[i] = bMax(remote_cheat_tally[i], CheatTally[i]);
    }

    ChangeState(OPS_FINISHED);
    TheOnlineManager.FinishDriver(racer_info->GetIndex(), racer_info->GetRanking(), false, finishReason);
}
#else
void OnlineRacer::Finish(int nRank, bool bBlinkBlinkPoof, int raceFinishReason) {
    SmartBitStream data;
    data.AddByte(DriverNumber);
    data.AddByte(static_cast<uint8>(nRank));
    uint8 value = 0;
    if (bBlinkBlinkPoof || raceFinishReason == 0xb) {
        value = 1;
    }
    data.AddByte(value);
    data.AddByte(static_cast<uint8>(raceFinishReason));
    data.AddRawData(reinterpret_cast<const char *>(CheatTally), 0x20);
    UpdateEndRaceStats();
    data.AddRawData(reinterpret_cast<const char *>(&FinishedRaceStats), 0xc0);
    if (Online::IsInitialized()) {
        Online::SignalDriverFinish(data);
    }
    ChangeState(OPS_FINISHED);
}

void OnlineRacer::SignalFinish(SmartBitStream &data) {
    uint8 rank;
    uint8 blinkPoof;
    uint8 finishReason;

    data.GetByte(rank);
    data.GetByte(blinkPoof);
    data.GetByte(finishReason);

    uint16 remote_cheat_tally[16];

    data.GetRawData(reinterpret_cast<char *>(remote_cheat_tally), sizeof(remote_cheat_tally));
    data.GetRawData(reinterpret_cast<char *>(&FinishedRaceStats), sizeof(FinishedRaceStats));

    for (int i = 0; i < 16; i++) {
        CheatTally[i] = bMax(remote_cheat_tally[i], CheatTally[i]);
    }

    ChangeState(OPS_FINISHED);
}
#endif

void OnlineRacer::UpdateEndRaceStats() {}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (speed.exe 0x7350A0): con IsConnected() en vez de la local de la A124 (mini banco: es la unica forma
// que da las dos colas de la funcion del exe).
void OnlineRacer::DriverDisconnect(eOnlineRacerState new_state, int finish_reason) {
    if (IsConnected()) {
        ChangeState(new_state);
    }
    FinishedRaceStats.FinishReason = finish_reason;
}
#else
void OnlineRacer::DriverDisconnect(eOnlineRacerState new_state, int finish_reason) {
    bool connected = false;
    if (State == OPS_LOST_CONNECTION || State == OPS_QUIT || State == OPS_DISCONNECTED) {
    } else {
        connected = State != OPS_DISCERROR;
    }

    if (connected) {
        ChangeState(new_state);
        FinishedRaceStats.FinishReason = finish_reason;
    } else {
        FinishedRaceStats.FinishReason = finish_reason;
    }
}
#endif

void OnlineRacer::UpdateLocal(float t) {
    if (!GRaceStatus::Exists()) {
        return;
    }
    BadnessReason = 0;
    EndRaceCountdown = -1.0f;

    IPlayer *p = IPlayer::First(PLAYER_LOCAL);
    GRacerInfo *localRacerInfo = GRaceStatus::Get().GetRacerInfo(p->GetSimable());

    if (TheOnlineManager.AreAllPlayersFinishedRacing()) {
        return;
    }
    if (TheOnlineManager.GetTimeupStartTime().IsSet()) {
        EndRaceCountdown =
            TheOnlineManager.GetTimeupLength() - (WorldTimer - TheOnlineManager.GetTimeupStartTime()).GetSeconds();
        if (EndRaceCountdown <= 0.0f && !localRacerInfo->IsFinishedRacing()) {
            TheOnlineManager.SetRaceTimeup();
            GRaceStatus::Get().SkipToEndOfRaceForRacer(IPlayer::First(PLAYER_LOCAL)->GetSimable(),
                                                       localRacerInfo->mIndex, 3600.0f);
        }
    }
}

// El bucle hacia delante: PC (expandida en ??0OnlineRacer, 0x760B20) lo junta en ocho dwords a cero; la A124 da el
// mismo codigo que con el bucle hacia atras.
void OnlineRacer::ClearCheatInfo() {
    for (int i = 0; i < 16; i++) {
        CheatTally[i] = 0;
    }
}

inline void OnlineRacer::DetectedCheat(int reason) {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    if (TheOnlineManager.IsAntiCheatingEnabled() && CheatTally[reason] < 0xFE01) { // PC (speed.exe 0x740FA0)
#else
    if (TheOnlineManager.IsAntiCheatingEnabled() && CheatTally[reason] <= 0xFE00) {
#endif
        CheatTally[reason]++;
    }
}

uint8 OnlineRacer::GetCheatScore() { return 0; }

uint32 OnlineRacer::GetDataCRC(bool recalc) { return PhysicsDataCRC; }

float OnlineRacer::GetEndRaceCountdown() { return EndRaceCountdown; }

OnlineManager::OnlineManager()
    : RestartingRace(false) //
    , RestartRequested(false) {
    State = OLS_DISCONNECTED;
    bMemSet(PersonaMap, 0, sizeof(PersonaMap));
    InitQuantizers();

    pLocalRacer = nullptr;
    NumRacers = 0;
    bOnlineRace = false;
    mMasterTime = 0;
    mServerTime = 0;
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    mAnimFinishAndLoadHudTime = 0.0f;
#endif
    CountdownSyncAnims.UnSet();
    CountdownSendDataCRC.UnSet();
    LastSyncedWTTimeStamp.UnSet();
    LastAntiCheatWorldTimer.UnSet();
    LastAntiCheatRealTimer.UnSet();
    TimeRaceFinished.UnSet();
    RaceTimeup = false;
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    win32_unknown_0x118 = false;
    win32_unknown_0x119 = false;
#endif

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    // PC (speed.exe 0x740FF0): en orden creciente.
    for (int i = 0; i < 4; ++i) {
        pRacers[i] = nullptr;
    }
#else
    for (int i = 3; i >= 0; --i) {
        pRacers[i] = nullptr;
    }
#endif
    ClearAnimWorldObjects();
}

void OnlineManager::PrintQuantizersUsageReport() {
    PositionQuantizerX.PrintUsageReport();
    PositionQuantizerY.PrintUsageReport();
    PositionQuantizerZ.PrintUsageReport();
    VelocityQuantizer.PrintUsageReport();
    AccelerationQuantizer.PrintUsageReport();
    AngleQuantizer.PrintUsageReport();
    AVelocityQuantizer.PrintUsageReport();
    MatrixQuantizer.PrintUsageReport();
    ControlQuantizer.PrintUsageReport();
}

void OnlineManager::PrintCheatTallies(bool) {}

OnlineManager::~OnlineManager() {}

void OnlineManager::Initialize(int argc, char **argv) {
#ifdef EA_PLATFORM_XENON
    // EGM 0x8250A0E8: sin argc/argv; el .cfg es lo que sigue al primer espacio de la linea de ordenes.
    char *filename = bStrChr(GetCommandLineA(), ' ');
    if (filename) {
        filename++;
    }
    OnlineCfg::ReadConfigFile(filename);
    VoiceCore::Construct();
#elif !defined(EA_PLATFORM_WIN32)
#ifdef EA_PLATFORM_PLAYSTATION2
    if (bIsPlatSuperBender()) {
        argc = SuperBenderGetCommandLineArgc();
        argv = SuperBenderGetCommandLineArgv();
    }
#endif

    const char *filename = nullptr;
    for (int i = 1; i < argc; i++) {
        int len = bStrLen(argv[i]);

        if (len > 4 && bStrICmp(argv[i] + len - 4, ".cfg") == 0) {
            filename = argv[i];
        }
    }
    OnlineCfg::ReadConfigFile(filename);
    VoiceCore::Construct();
#endif
    if (!SkipFE) {
        OnlineEnabled = 1;
    }
}

// En linea (el original la define aqui, tras DetectedCheat, que por eso la llama en vez de expandirla).
inline bool OnlineManager::IsAntiCheatingEnabled() {
    return (FEDatabase->GetOnlineSettings()->RankedGame || SkipFE) && !FrameRateIsTooLow;
}

void OnlineManager::InitAntiCheating(int num_players) {
    if (NetworkUseLobbies != 0) {
        char *server_config = LobbyCore::Instance().GetServerConfig();
        if (server_config) {
            char buff1[128];
            char buff2[255];

            TagFieldGetString(TagFieldFind(server_config, "RESPONSE_GRAPH"), buff1, 0x80, "");
            bSPrintf(buff2, "%s: %s", "GRAPH", buff1);
            SpeedScript parse("GRAPH_SCRIPT", buff2, 0);
            if (parse.GetNextCommand()) {
                int i = 0;
                while (i < 16) {
                    if (!parse.IsAnotherArgument()) {
                        break;
                    }
                    MinCheatFreq[i] = parse.GetNextArgumentFloat();
                    i++;
                }
            }

            TagFieldGetString(TagFieldFind(server_config, "MIN_CHEAT_THRESHOLD"), buff1,
                              0x80, "51");
            ONLINE_CHEATSCORE_THRESHOLD = bStrToLong(buff1);
            bSPrintf(buff2, "RESPONSE_REMAP%d", num_players);
            TagFieldGetString(TagFieldFind(server_config, buff2), buff1, 0x80, "");
            if (bStrLen(buff1) > 0) {
                bSPrintf(buff2, "%s: %s", "REMAP", buff1);
                SpeedScript parse("REMAP_SCRIPT", buff2, 0);
                if (parse.GetNextCommand()) {
                    int i = 0;
                    while (i < 4) {
                        if (!parse.IsAnotherArgument()) {
                            break;
                        }
                        Client::m_sendCarFrequencyHz[i] = static_cast<float>(parse.GetNextArgumentInt());
                        i++;
                    }
                }
            }
        }
    }
}

void OnlineManager::Disconnect(bool force_disconnect) {
    if (Online::IsInitialized()) {
        Online::Close();
    }

    int driver_number = 0;
    ConnectionCore::Instance().LeaveSession();
    bOnlineRace = false;

    for (OnlineRacer **racer = pRacers; driver_number < 4; ++driver_number, ++racer) {
        if (*racer) {
            GetOnlineRacer(driver_number)->ChangeState(OPS_DISCONNECTED);
        }
    }

    if (State == OLS_RACE_DATA_SYNC && NetworkUseLobbies != 0 && !force_disconnect) {
        ChangeState(OLS_LOBBY_IN_LOBBY);
    } else {
        ChangeState(OLS_DISCONNECTED);
    }
    PurgeDisconnectedRacers();
}

void OnlineManager::ChangeState(eOnlineState newState) {
    if (newState != State) {
        eOnlineState oldState = State;

        State = newState;
        if (newState == OLS_RACING || oldState == OLS_RACING) {
            Timer to;

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
            to.SetTime(TheOnlineManager.GetMasterTime() * 0.002f); // PC (speed.exe 0x741230)
#else
            to.SetTime(TheOnlineManager.GetMasterTime() * 0.001f);
#endif
            Scheduler::Get().Synchronize(to);
        }
    }
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (speed.exe 0x735620): sin la local de la A124.
bool OnlineManager::IsOnlineRace() {
    return bOnlineRace || Online::IsInitialized();
}
#else
bool OnlineManager::IsOnlineRace() {
    bool online_race = false;
    if (bOnlineRace || Online::IsInitialized()) {
        online_race = true;
    }
    return online_race;
}
#endif

bool OnlineManager::IsServer() {
    return OnlineIsServer != 0;
}

bool OnlineManager::RaceStartAborted() {
    return GetNumConnectedRacers() == 0;
}

void OnlineManager::Update(bool receive) {
    Online::ShowDiagnostics();

    uint32 time = NetworkCore::Instance().GetTime();
    uint32 elapsed = time - mLastUpdateTime;
    if (elapsed > 100) {
        GetMasterTime();
    }
    mLastUpdateTime = time;

    bool host_disconnected = IsOnlineRace() && State != OLS_RACE_END && !IsServer() &&
                             !CUIOnlineDisconnect::mIsHostInGameDisconnect &&
                             (!ConnectionCore::Instance().GetHost() ||
                              ConnectionCore::Instance().GetHost()->GameInfo.eStatus != CONNAPI_STATUS_ACTV);
    if (host_disconnected && TheGameFlowManager.GetState() == GAMEFLOW_STATE_RACING) {
        CUIOnlineDisconnect::mIsHostInGameDisconnect = true;
        cFEng::Get()->QueuePackagePush("UI_OL_Disconnect_BG.fng", 0, 0, false);
    }

    if (State == OLS_RACING && !IsServer()) {
        uint32 master = GetMasterTime();
        uint32 server = GetServerTime();
        int delta = server - master;
        if (delta > 100) {
            mMasterTime += master - server;
        } else if (delta > 3) {
            mMasterTime -= 3;
        } else if (delta < -1) {
            mMasterTime++;
        } else {
            mMasterTime -= delta;
        }
    }

    switch (State) {
    case OLS_DISCONNECTED:
    case OLS_RACE_DATA_SYNC:
    case OLS_RACE_LOAD_TRACK:
        break;

    case OLS_LOBBY_IN_LOBBY:
        if (NetworkUseLobbies == 0) {
            if (SkipFE) {
                if (static_cast<PlatformNetworkCore &>(NetworkCore::Instance()).IsOnline() == true) {
                    FEDatabase->GetOnlineSettings()->RankedGame = false;
                    StartOnlineRace();
                }
            }
        }
        break;

    case OLS_RACE_START_LINE:
        if (!IsServer()) {
            uint32 ticker = bGetTicker();
            if (bGetTickerDifference(m_ticker, ticker) > 125.0f) {
                m_ticker = ticker;
                Client::SendStartRaceSyncMessage();
            }
        }

        if (TimeTrackLoaded.IsSet() && !mStartRaceIsSet && !RaceStartAborted()) {
            if (IsServer()) {
                if ((RealTimer - TimeTrackLoaded).GetSeconds() > 120.0f) {
                    TimeTrackLoaded.UnSet();
                    Server::DisconnectLaggers();
                }
            } else if ((RealTimer - TimeTrackLoaded).GetSeconds() > 140.0f) {
                TimeTrackLoaded.UnSet();
                Online::Close();
            }
        }

        if (ONLINE_CHEAT_FALSESTART || (GetStartRaceTime(bGetTicker()) <= 0.0f && !RaceStartAborted())) {
            StartRace();
        }
        break;

    case OLS_RACING: {
        if (TheGameFlowManager.GetState() != GAMEFLOW_STATE_RACING) {
            break;
        }

        if (GetNumConnectedRacers() == 0 && !pLocalRacer->IsFinishedRacing()) {
            pLocalRacer->DriverDisconnect(OPS_DISCONNECTED, 0xc);
        }

        bool all_finished = true;
        bool all_disconnected = true;
        for (IPlayer::List::const_iterator iter = IPlayer::GetList(PLAYER_ALL).begin();
             iter != IPlayer::GetList(PLAYER_ALL).end(); ++iter) {
            IPlayer *player = *iter;
            IOnlinePlayer *online_player;

            if (!player->QueryInterface(&online_player)) {
                break;
            }

            OnlineRacer *racer = online_player->GetOnlineRacer();
            GRacerInfo *racer_info = GRaceStatus::Get().GetRacerInfo(player->GetSimable());
            if (racer) {
                if (racer_info->IsFinishedRacing() || racer->IsFinishedRacing()) {
                    if (racer->IsConnected()) {
                        all_disconnected = false;
                    }
                } else {
                    all_disconnected = false;
                    all_finished = false;
                    break;
                }
            }
        }

        if (all_finished) {
            if (all_disconnected && RaceEndReason == OEND_RACE_FINISHED) {
                RaceEndReason = OEND_NO_PLAYERS;
            }
            EndOnlineRace(false);
            if (all_disconnected && !CUIOnlineDisconnect::mIsHostInGameDisconnect) {
                CUIOnlineDisconnect::mIsHostInGameDisconnect = true;
                cFEng::Get()->QueuePackagePush("UI_OL_Disconnect_BG.fng", 0, 0, false);
            }
        } else {
            if (IsServer() && !CountdownSyncAnims.CountDown(RealTimeElapsed)) {
                SendSyncAnimations();
                CountdownSyncAnims.SetTime(1.0f);
            }
            if (!CountdownSendDataCRC.CountDown(RealTimeElapsed)) {
                SendLocalPlayerDataCRC();
                CountdownSendDataCRC.SetTime(5.0f);
            }
            if ((RealTimer - LastAntiCheatRealTimer).GetSeconds() > 2.0f) {
                CheckWorldTimerHacking();
                LastAntiCheatRealTimer = RealTimer;
            }
            if (IsServer() && TheRaceParameters.IsGetAway()) {
                CheckGetAwayLeaderChange();
            }
        }

        if (pLocalRacer->IsConnected()) {
            pLocalRacer->UpdateLocal(WorldTimeElapsed);
        }

        if (TheOnlineManager.IsAntiCheatingEnabled() && pLocalRacer->GetEndRaceCountdown() < -3.0f) {
            for (int i = 0; i < 4; i++) {
                OnlineRacer *racer = GetOnlineRacer(i);

                if (racer && !racer->IsFinishedRacing()) {
                    racer->DetectedCheat(CHEAT_IGNORED_TIMEUP);
                    if (pLocalRacer != racer || ONLINE_CHEAT_IGNORE_TIMEUP == 0) {
                        racer->DriverDisconnect(OPS_FINISHED, 0xb);
                    }
                }
            }
        }

        if (ONLINE_SCREENPRINT_CHEAT_REPORT) {
            if (HasAnyoneCheated()) {
                DoScreenPrintf = 1;
                PrintCheatTallies(true);
            }
        }
        break;
    }

    case OLS_RACE_END:
        RaceTimeup = false;
        if (!TimeRaceFinished.IsSet()) {
            TimeRaceFinished = WorldTimer;
        }
        PrintCheatTallies(true);
        break;

    default:
        break;
    }

    if (receive) {
        UpdateIncoming();
    }
}

void OnlineManager::UpdateIncoming() {
    if (Online::IsInitialized()) {
        Online::ReadIncomingPackets();
    }
}

void OnlineManager::UpdateOutgoing() {
    if (Online::IsInitialized()) {
        Online::SendUpdates();
    }
}

void OnlineManager::StartLobby() {
    if (State == OLS_DISCONNECTED) {
        ChangeState(OLS_LOBBY_IN_LOBBY);
    }
}

void OnlineManager::StartOnlineRace() {
    if (NetworkUseLobbies != 0 && State != OLS_LOBBY_IN_LOBBY) {
        return;
    }

    ChangeState(OLS_LOBBY_IN_LOBBY);
    CreateOnlineRacer(0, nullptr, IsServer(), FEDatabase->GetOnlineSettings()->GetLobbyPersona());
    if (SkipFE == 0) {
        if (IsServer()) {
            SetupRaceParams();
        }
    }

    mStartRaceIsSet = false;
    mStartRaceTick = bGetTicker();
    mStartRaceTime = 0.0f;
    mMasterTime += GetMasterTime();
    uint32 master_time = GetMasterTime();
    SetServerTime(master_time);
    mLastUpdateTime = NetworkCore::Instance().GetTime();
    TimeupStartTime.UnSet();
    Online::Init();
    bOnlineRace = true;
    ChangeState(OLS_RACE_DATA_SYNC);
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (speed.exe 0x761240): sin RemoveDriverInfo.
void OnlineManager::SetupRestartRace() {
    TheRaceParameters.RaceType = RACE_TYPE_SINGLE_RACE;
    for (int driver_number = 0; driver_number < 4; ++driver_number) {
        OnlineRacer *racer = pRacers[driver_number];
        if (racer && racer != pLocalRacer) {
            if (!racer->bShouldRestart) {
                delete racer;
                pRacers[driver_number] = nullptr;
                --NumRacers;
            } else {
                racer->bShouldRestart = false;
                racer->SetRaceScore(0);
            }
        }
    }
}
#else
void OnlineManager::SetupRestartRace() {
    TheRaceParameters.RaceType = RACE_TYPE_SINGLE_RACE;

    int driver_number = 0;
    OnlineRacer **racer = pRacers;
    do {
        OnlineRacer *oracer = GetOnlineRacer(driver_number);
        ++driver_number;
        if (*racer && oracer != pLocalRacer) {
            if (!oracer->bShouldRestart) {
                TheRaceParameters.RemoveDriverInfo(oracer->GetDriverNumber());
                delete *racer;
                *racer = nullptr;
                --NumRacers;
            } else {
                oracer->bShouldRestart = false;
                oracer->SetRaceScore(0);
            }
        }
        ++racer;
    } while (driver_number < 4);
}
#endif

void OnlineManager::EndOnlineRace(bool bForced) {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    bool players_left = true;
    if (RaceEndReason == OEND_NO_PLAYERS) {
        players_left = false;
    }
    MNotifyOnlineRaceOver(players_left).Post(UCrc32(0x20d60dbf));
#else
    MNotifyOnlineRaceOver().Post(UCrc32(0x20d60dbf));
#endif

    if (pCurrentWorld) {
        pCurrentWorld->EndOnlinePause();
    }

#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x761313): el anfitrion (GameDetails == eHOSTING) baja el bool de la sesion (bit 0x4000) al acabar la carrera.
    if (FEDatabase->OnlineSettings.GameDetails == eHOSTING) {
        LobbyGameSessions::Instance().Win32Unknown79A1D0(false);
    }
#endif

    if (State == OLS_RACING || State == OLS_RACE_LOAD_TRACK || State == OLS_RACE_START_LINE) {
        if (bForced) {
            if (pLocalRacer) {
                pLocalRacer->DriverDisconnect(OPS_QUIT, 0xc);
            }
            for (int i = 0; i < 4; i++) {
                if (IsValidRacer(i) && GetOnlineRacer(i) != pLocalRacer && !GetOnlineRacer(i)->IsFinishedRacing()) {
                    GetOnlineRacer(i)->DriverDisconnect(OPS_DISCONNECTED, 0xc);
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            if (IsValidRacer(i)) {
                GetOnlineRacer(i)->bShouldRestart = GetOnlineRacer(i)->IsConnected();
            }
        }

        if (IsServer() == true) {
            float current_time = WorldTimer.GetSeconds();
            int numDiscAtCurrentTime = 0;
            int index = -1;

            for (int i = 0; i < 4; i++) {
                OnlineRacer *r;

                if (IsValidRacer(i) && (r = GetOnlineRacer(i)) && r != pLocalRacer &&
                    bEqual(r->GetDisconnectTime(), current_time, 0.3f)) {
                    numDiscAtCurrentTime++;
                    if (numDiscAtCurrentTime > 1) {
                        r->ChangeState(OPS_DISCONNECTED);
                        if (index >= 0) {
                            GetOnlineRacer(index)->ChangeState(OPS_DISCONNECTED);
                            index = -1;
                        }
                    } else {
                        index = i;
                    }
                }
            }
            if (numDiscAtCurrentTime > 1) {
                pLocalRacer->ChangeState(OPS_LOST_CONNECTION);
            }
        }

        CalculateFinishOrder();
        SendEndOfRaceResults();
        ClearAnimWorldObjects();
        PrintCheatTallies(false);
        PrintQuantizersUsageReport();
    }

    if (bForced || State == OLS_RACE_END) {
        if (GetRestartingRace()) {
            bRandom(2, &GameSeed);
            SetupRestartRace();
            ChangeState(OLS_RACE_LOAD_TRACK);
            Online::SignalRestart();
        } else {
            Disconnect(false);
        }
    } else {
        ChangeState(OLS_RACE_END);
    }
}

void OnlineManager::RequestRestart() {}

bool OnlineManager::GetRestartingRace() {
    return RestartingRace;
}

void OnlineManager::SignalLoad() {
    if (NetworkUseLobbies != 0) {
        MapRacers2PlayerIDs();
    }
    ChangeState(OLS_RACE_LOAD_TRACK);
    if (TheGameFlowManager.IsInFrontend()) {
        TheGameFlowManager.UnloadFrontend();
        return;
    }
    TheGameFlowManager.LoadTrack();
}

void OnlineManager::SetupStartingPositions() {}

void OnlineManager::TrackLoaded() {
    if (State == OLS_RACE_LOAD_TRACK) {
        CountdownSyncAnims.UnSet();
        CountdownSendDataCRC.UnSet();
        LastSyncedWTTimeStamp.UnSet();
        LastAntiCheatRealTimer.UnSet();
        LastAntiCheatWorldTimer.UnSet();
        TimeRaceFinished.UnSet();
        FrameRateIsTooLow = false;
        OnlineTimer::SetTimestamp(WorldTimer);
        TimeTrackLoaded = RealTimer;
        if (Online::IsInitialized()) {
            Online::SignalStartClockSync();
            SetupStartingPositions();
        }
    }
}

void OnlineManager::ReadyStartLine() {
    if (State == OLS_RACE_LOAD_TRACK) {
        m_ticker = bGetTicker();
        ChangeState(OLS_RACE_START_LINE);
        TheAnimPlayer.PauseAll();
        if (pCurrentWorld) {
            pCurrentWorld->BeginOnlinePause();
            OnlineTimer::SetTimestamp(WorldTimer);
            BuildAnimWorldObjects();
        }
        if (Online::IsInitialized()) {
            Online::SignalReady();
        }
    }
}

uint32 OnlineManager::GetMasterTime() {
    return NetworkCore::Instance().GetTime() - mMasterTime;
}

void OnlineManager::SetServerTime(uint32 time) {
    mServerTime = time - NetworkCore::Instance().GetTime();
}

uint32 OnlineManager::GetServerTime() {
    return mServerTime + NetworkCore::Instance().GetTime();
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (speed.exe 0x7358A0): vacia.
void OnlineManager::StartSimFrame() {}
#else
void OnlineManager::StartSimFrame() {
    NetworkCore::DoNetworkProcessing();
}
#endif

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (speed.exe 0x7358B0): `> 100` y la latencia de la sesion en una sola cadena de if.
void OnlineManager::EndSimFrame() {
    uint32 time = NetworkCore::Instance().GetTime();
    if (time - mLastUpdateTime > 100) {
        if (!bHasStalled) {
            GetMasterTime();
        }
        bHasStalled = true;
    } else {
        bHasStalled = false;
    }

    if (IsServer()) {
        LobbyGameSessions::Instance().SetSessionLatency(0);
    } else if (ConnectionCore::Instance().GetHost() &&
               ConnectionCore::Instance().GetHost()->GameInfo.eStatus == CONNAPI_STATUS_ACTV) {
        LobbyGameSessions::Instance().SetSessionLatency(
            NetGameLinkStatus(ConnectionCore::Instance().GetHost()->pGameLinkRef)->late);
    } else {
        LobbyGameSessions::Instance().SetSessionLatency(-1);
    }
}
#else
void OnlineManager::EndSimFrame() {
    uint32 time = NetworkCore::Instance().GetTime();
    if (time - mLastUpdateTime >= 0x65) {
        if (!bHasStalled) {
            GetMasterTime();
        }
        bHasStalled = true;
    } else {
        bHasStalled = false;
    }

    if (IsServer()) {
        LobbyGameSessions::Instance().SetSessionLatency(0);
    } else {
        ConnApiClientT *player = ConnectionCore::Instance().GetHost();
        if (!player) {
            LobbyGameSessions::Instance().SetSessionLatency(-1);
        } else {
            player = ConnectionCore::Instance().GetHost();
            if (player->GameInfo.eStatus != CONNAPI_STATUS_ACTV) {
                LobbyGameSessions::Instance().SetSessionLatency(-1);
            } else {
                LobbyGameSessions *sessions = &LobbyGameSessions::Instance();
                player = ConnectionCore::Instance().GetHost();
                sessions->SetSessionLatency(NetGameLinkStatus(player->pGameLinkRef)->late);
            }
        }
    }
}
#endif

void OnlineManager::SetStartRaceTime(uint32 from_tick, float time, float ping) {
    if (!mStartRaceIsSet || ping < mStartRacePing) {
        RestartingRace = false;
        RestartRequested = false;
        mStartRaceTime = time;
        mStartRaceTick = from_tick;
        mStartRacePing = ping;

        uint32 master_time = GetMasterTime();
        uint32 time_milliseconds = static_cast<uint32>(time * 1000.0f);
        mMasterTime += master_time + time_milliseconds;
        SetServerTime(GetMasterTime());
        mStartRaceIsSet = true;
    }
}

float OnlineManager::GetStartRaceTime(uint32 from_tick) {
    if (mStartRaceIsSet) {
        return mStartRaceTime - bGetTickerDifference(mStartRaceTick, from_tick) * 0.001f;
    }
    return 2.0f;
}

void OnlineManager::StartRace() {
    TheAnimPlayer.UnPauseAll();
    RaceEndReason = OEND_RACE_FINISHED;

    for (int i = 0; i < 4; i++) {
        if (IsValidRacer(i) && GetOnlineRacer(i)->IsConnected()) {
            GetOnlineRacer(i)->ChangeState(OPS_RACING);
        }
    }

    mPostCountdownStartRaceTime = 0.0f;
    mStartTime = mMasterTime;
    ChangeState(OLS_RACING);
}

void OnlineManager::ImportRaceParams(int local_driver_number, int server_driver_number, SmartBitStream &data) {
    cOnlineSettings *settings = FEDatabase->GetOnlineSettings();

    settings->RaceMode = static_cast<GRace::Type>(static_cast<uint8>(data.GetBits(8)));
    settings->CollisionDetection = data.GetBool();
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    settings->PerformanceMatching = data.GetBool();
    settings->UseNOS = data.GetBool();
#endif
    settings->GetRaceSettings()->TrackDirection = data.GetBits(8);
    settings->GetRaceSettings()->NumLaps = data.GetBits(16);
    settings->GetRaceSettings()->CopDensity = data.GetByte();
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    settings->GetRaceSettings()->TimeOfDay = data.GetFloat();
#endif
    SetupRaceParams();
    SetupLocalDriver(local_driver_number);
    CreateOnlineRacer(server_driver_number, nullptr, true, nullptr);
    data.GetBits(8);
}

void OnlineManager::ExportRaceParams(int server_driver_number, SmartBitStream &data) {
    if (!pLocalRacer) {
        SetupLocalDriver(server_driver_number);
    }

    cOnlineSettings *settings = FEDatabase->GetOnlineSettings();
    OnlineRaceParameters *race_settings = settings->GetRaceSettings();
    data.AddByte(settings->RaceMode);
    data.AddBool(settings->CollisionDetection);
    data.AddByte(race_settings->TrackDirection);
    data.AddShort(race_settings->NumLaps);
    data.AddByte(race_settings->CopDensity);
    data.AddByte(0);
}

void OnlineManager::ImportDriverInfo(int driver_number, SmartBitStream &data) {
    char persona[16];
    OnlineRacer *racer;
    uint32 v;

    data.GetTerminatedString(persona, 0x10);
    CreateOnlineRacer(driver_number, nullptr, false, persona);
    racer = GetOnlineRacer(driver_number);
    data.GetRawData(reinterpret_cast<char *>(&racer->CarTypeKey), 4);
    v = 0;
    data.GetBits(v, 1);
    racer->IsCustomCar = v != 0;
    if (racer->IsCustomCar) {
        data.GetRawData(reinterpret_cast<char *>(&racer->CarCustomization), 0x198);
    }

    Attrib::Gen::pvehicle vehicle(racer->CarTypeKey, 0, nullptr);
}

void OnlineManager::ExportDriverInfo(int driver_number, SmartBitStream &data) {
    OnlineRacer *racer = GetOnlineRacer(driver_number);
    const char *carType;

    data.AddTerminatedString(racer->Persona);
    racer->IsCustomCar = false;
    carType = SkipFEPlayerCar;
    if (SkipFE == 0 || carType == nullptr || *carType == '\0') {
        RaceSettings *race_settings =
            FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes);
        FEPlayerCarDB *stable = &FEDatabase->CurrentUserProfiles[0]->PlayersCarStable;
        FECarRecord *record = stable->GetCarRecordByHandle(race_settings->SelectedCar[0]);
        carType = record->GetDebugName();
        FECustomizationRecord *customization =
            stable->GetCustomizationRecordByHandle(record->Customization);
        if (customization) {
            racer->IsCustomCar = true;
            racer->CarCustomization = *customization;
        }
    }

    racer->CarTypeKey = Attrib::StringToKey(carType);
    data.AddRawData(reinterpret_cast<char *>(&racer->CarTypeKey), 4);
    data.AddBool(racer->IsCustomCar);
    if (racer->IsCustomCar) {
        data.AddRawData(reinterpret_cast<char *>(&racer->CarCustomization), 0x198);
    }
}

void OnlineManager::CreateOnlineRacer(int driver_num, SmartBitStream *pdata, bool is_server,
                                      const char *persona) {
    PurgeDisconnectedRacers();
    if (pRacers[driver_num] == nullptr) {
        OnlineRacer *racer = new OnlineRacer(static_cast<int8>(driver_num), is_server, persona);
        pRacers[driver_num] = racer;
        racer->ChangeState(OPS_CONNECTED);
        ++NumRacers;
    } else {
        if (GetOnlineRacer(driver_num)->IsServer()) {
            GetOnlineRacer(driver_num)->SetPersona(persona);
            return;
        }
    }
}

void OnlineManager::PurgeDisconnectedRacers() {
    if (pLocalRacer && !pLocalRacer->IsConnected()) {
        pLocalRacer = nullptr;
    }

    for (int i = 0; i < 4; i++) {
        if (IsValidRacer(i) && !GetOnlineRacer(i)->IsConnected()) {
            delete pRacers[i];
            pRacers[i] = nullptr;
            NumRacers--;
        }
    }
}

int OnlineManager::GetNumConnectedRacers() {
    int num_connected = 0;

    for (int i = 0; i < 4; i++) {
        if (IsValidRacer(i) && GetOnlineRacer(i) != pLocalRacer) {
            num_connected += GetOnlineRacer(i)->IsConnected();
        }
    }
    return num_connected;
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (SendEndOfRaceResults, speed.exe 0x741780, la expande en cuatro pruebas en orden creciente).
int OnlineManager::GetNumRacers() {
    int num_racers = 0;
    for (int i = 0; i < 4; ++i) {
        if (IsValidRacer(i)) {
            ++num_racers;
        }
    }
    return num_racers;
}
#else
int OnlineManager::GetNumRacers() {
    int num_racers = 0;
    OnlineRacer **racer = pRacers;
    int i = 3;
    do {
        if (*racer) {
            ++num_racers;
        }
        ++racer;
        --i;
    } while (i >= 0);
    return num_racers;
}
#endif

OnlineRacer *OnlineManager::GetOnlineRacer(int driver_number) {
    return pRacers[driver_number];
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (speed.exe 0x735AE0): devuelve GetOnlineRacer(x) (la de la A124, el puntero que recorre). Nombres del
// parametro y de la local de Carbon PS3 (DWARF de la Alpha 56).
OnlineRacer *OnlineManager::GetOnlineRacer(const char *racerName) {
    for (int x = 0; x < 4; ++x) {
        if (IsValidRacer(x) && bStrCmp(GetOnlineRacer(x)->GetPersona(), racerName) == 0) {
            return GetOnlineRacer(x);
        }
    }
    return nullptr;
}
#else
OnlineRacer *OnlineManager::GetOnlineRacer(const char *racer_name) {
    int driver_number = 0;
    OnlineRacer **racer = pRacers;

    for (; driver_number < 4; ++driver_number, ++racer) {
        if (*racer && bStrCmp((*racer)->GetPersona(), racer_name) == 0) {
            return *racer;
        }
    }
    return nullptr;
}
#endif

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (speed.exe 0x735B30): como GetOnlineRacer(const char *) de PC, devuelve GetOnlineRacer(driver_number).
OnlineRacer *OnlineManager::GetServerRacer() {
    for (int driver_number = 0; driver_number < 4; ++driver_number) {
        if (IsValidRacer(driver_number) && GetOnlineRacer(driver_number)->IsServer()) {
            return GetOnlineRacer(driver_number);
        }
    }
    return nullptr;
}
#else
OnlineRacer *OnlineManager::GetServerRacer() {
    int driver_number = 0;
    OnlineRacer **racer = pRacers;
    do {
        if (*racer && GetOnlineRacer(driver_number)->IsServer()) {
            return GetOnlineRacer(driver_number);
        }
        ++driver_number;
        ++racer;
    } while (driver_number < 4);
    return nullptr;
}
#endif

void OnlineManager::SetupRaceParams() {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    OnlineRaceParameters *race_settings = FEDatabase->GetOnlineSettings()->GetRaceSettings();
    unsigned int seconds = LobbyApiInfoInt(LobbyCore::Instance().GetLobbyApiRef(), 'stim');
    float time_of_day = ((seconds % 3600) * 1000) * (1.0f / 1800000.0f);

    race_settings->TimeOfDay = time_of_day < 1.0f ? time_of_day : 2.0f - time_of_day;
#endif
    RaceStarter::SetupOnlineRace();
    g_tweakDriftPhysics = 0;
}

void OnlineManager::SetupLocalDriver(int driver_num) {
    if (driver_num != 0) {
        pRacers[driver_num] = pRacers[0];
        pRacers[0] = nullptr;
    }
    pLocalRacer = pRacers[driver_num];
    pLocalRacer->DriverNumber = driver_num;
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (speed.exe 0x735C10): otra revision, sin la rama de pCurrentRace ni RemoveDriverInfo.
void OnlineManager::DriverLeft(int driver_number, bool he_quit) {
    if (!IsValidRacer(driver_number)) {
        return;
    }

    OnlineRacer *racer = GetOnlineRacer(driver_number);

    racer->SetDisconnectTime(WorldTimer.GetSeconds());
    if (State == OLS_RACE_DATA_SYNC) {
        racer->ChangeState(OPS_DISCONNECTED);
    } else {
        racer->ChangeState(he_quit ? OPS_QUIT : OPS_LOST_CONNECTION);
    }
    if (racer->IsServer()) {
        for (int i = 0; i < 4; i++) {
            if (IsValidRacer(i) && GetOnlineRacer(i) != racer) {
                GetOnlineRacer(i)->ChangeState(OPS_DISCONNECTED);
            }
        }
    }
}
#else
void OnlineManager::DriverLeft(int driver_number, bool he_quit) {
    if (!IsValidRacer(driver_number)) {
        return;
    }

    OnlineRacer *racer = GetOnlineRacer(driver_number);

    racer->SetDisconnectTime(WorldTimer.GetSeconds());
    if (State == OLS_RACE_DATA_SYNC) {
        racer->ChangeState(OPS_DISCONNECTED);
        TheRaceParameters.RemoveDriverInfo(racer->GetDriverNumber());
        if (racer->IsServer()) {
            for (int i = 0; i < 4; i++) {
                if (IsValidRacer(i) && GetOnlineRacer(i) != racer) {
                    GetOnlineRacer(i)->ChangeState(OPS_DISCONNECTED);
                    TheRaceParameters.RemoveDriverInfo(GetOnlineRacer(i)->GetDriverNumber());
                }
            }
        }
    } else if (pCurrentRace) {
        FEOnlineDisconnectMsg(racer);
        if (IsRaceOver()) {
            racer->DriverDisconnect(OPS_DISCONNECTED, 0xc);
        } else {
            racer->DriverDisconnect(he_quit ? OPS_QUIT : OPS_LOST_CONNECTION, 0xc);
        }
        if (racer->IsServer()) {
            for (int i = 0; i < 4; i++) {
                if (IsValidRacer(i) && GetOnlineRacer(i) != racer) {
                    GetOnlineRacer(i)->DriverDisconnect(OPS_DISCONNECTED, 0xc);
                }
            }
        }
    } else {
        racer->ChangeState(he_quit ? OPS_QUIT : OPS_LOST_CONNECTION);
        if (racer->IsServer()) {
            for (int i = 0; i < 4; i++) {
                if (IsValidRacer(i) && GetOnlineRacer(i) != racer) {
                    GetOnlineRacer(i)->ChangeState(OPS_DISCONNECTED);
                }
            }
        }
    }
}
#endif

void OnlineManager::SignalDriverFinish(SmartBitStream &data) {
    uint8 drivernum;

    data.GetByte(drivernum);
    if (IsValidRacer(drivernum)) {
        GetOnlineRacer(drivernum)->SignalFinish(data);
    }
}

void OnlineManager::SignalSyncAnimationMessage(SmartBitStream &) {}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (speed.exe 0x755140): sin las comprobaciones de trampas de la A124, `> 0` en CheatTally, y el tiempo de
// cierre (TimeupLength) se pone con la primera llegada; en los modos 8 y 9 sale del porcentaje de carrera.
bool OnlineManager::FinishDriver(int driver_number, int nRank, bool bBlinkBlinkPoof,
                                 int raceFinishReason) {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(driver_number);
    OnlineRacer *racer = GetOnlineRacer(racerInfo.GetName());
    bool finished = racer->IsFinishedRacing();

    if (!finished && racer == pLocalRacer) {
        racer->Finish(nRank, bBlinkBlinkPoof, raceFinishReason);
        finished = true;

        if (raceFinishReason == 0xb && racer->CheatTally[2] > 0) {
            for (int i = 0; i < 4; ++i) {
                OnlineRacer *other_racer = GetOnlineRacer(i);
                if (other_racer && other_racer != racer) {
                    other_racer->DriverDisconnect(OPS_DISCONNECTED, 0xc);
                }
            }
        }
    }

    if (!TimeupStartTime.IsSet()) {
        TimeupStartTime.SetTime(racerInfo.GetRaceTime() + mPostCountdownStartRaceTime);
        if (raceFinishReason == 8 || raceFinishReason == 9) {
            TimeupLength =
                (static_cast<float>(static_cast<int>(6.0f - racerInfo.GetPctRaceComplete() * 0.06f)) + 1.0f) * 10.0f;
        } else {
            TimeupLength = 30.0f;
        }
    }
    return finished;
}
#else
bool OnlineManager::FinishDriver(int driver_number, int nRank, bool bBlinkBlinkPoof,
                                 int raceFinishReason) {
    GRacerInfo &racerInfo = GRaceStatus::Get().GetRacerInfo(driver_number);
    OnlineRacer *racer = GetOnlineRacer(racerInfo.GetName());
    bool finished = racer->IsFinishedRacing();

    if (!finished && racer == pLocalRacer) {
        if (TheOnlineManager.IsAntiCheatingEnabled() && raceFinishReason == 1 &&
            (TheRaceParameters.IsDriftRace() || TheRaceParameters.IsDragRace())) {
        }

        if (raceFinishReason != 0xb || ONLINE_CHEAT_IGNORE_TIMEUP == 0) {
            racer->Finish(nRank, bBlinkBlinkPoof, raceFinishReason);
            finished = true;

            if (raceFinishReason == 0xb && racer->CheatTally[2] != 0) {
                for (int i = 0; i < 4; ++i) {
                    OnlineRacer *other_racer = GetOnlineRacer(i);
                    if (other_racer && other_racer != racer) {
                        other_racer->DriverDisconnect(OPS_DISCONNECTED, 0xc);
                    }
                }
            }
        }
    }

    if (raceFinishReason == 1 && nRank == 1 && !TimeupStartTime.IsSet()) {
        TimeupStartTime.SetTime(mPostCountdownStartRaceTime + racerInfo.GetRaceTime());
        TimeupLength = 30.0f;
    }
    return finished;
}
#endif

void OnlineManager::CalculateFinishOrder() {
    int num_racers = 0;
    OnlineRacer *racers[4];
    bool swap;

    for (int i = 0; i < 4; i++) {
        if (IsValidRacer(i)) {
            racers[num_racers] = GetOnlineRacer(i);
            num_racers++;
        }
    }

    RaceParameters &race_parameters = TheRaceParameters;
    do {
        swap = false;
        for (int j = 0; j < num_racers - 1; j++) {
            OnlineRacer *racer[2];
            int sort_score[2];
            FinishedRaceStatsEntry *frs[2];

            racer[0] = racers[j];
            racer[1] = racers[j + 1];

#ifdef EA_BUILD_A124
            memset(sort_score, 0, sizeof(sort_score)); // la A124 llama a memset
#else
            bMemSet(sort_score, 0, sizeof(sort_score));
#endif

            frs[0] = racer[0]->GetFinishedRaceStats();
            frs[1] = racer[1]->GetFinishedRaceStats();

            for (int i = 0; i < 2; i++) {
                if (frs[i]->FinishReason == 12) {
                    sort_score[i] += 2;
                } else if (frs[i]->FinishReason != 1) {
                    sort_score[i]++;
                }
            }

            if (sort_score[0] == sort_score[1]) {
                int behind_index = -1;

                if (!race_parameters.IsDriftRace()) {
                    if (sort_score[0] == 0) {
                        if (frs[0]->RaceTime.GetPackedTime() != frs[1]->RaceTime.GetPackedTime()) {
                            if (frs[0]->RaceTime.GetPackedTime() < frs[1]->RaceTime.GetPackedTime()) {
                                behind_index = 1;
                            } else {
                                behind_index = 0;
                            }
                        }
                    } else if (frs[0]->NumLapsCompletedExact != frs[1]->NumLapsCompletedExact) {
                        behind_index = frs[1]->NumLapsCompletedExact < frs[0]->NumLapsCompletedExact ? 1 : 0;
                    }
                }

                if (behind_index >= 0) {
                    sort_score[behind_index]++;
                }
            }

            if (sort_score[0] > sort_score[1]) {
                racers[j] = racer[1];
                racers[j + 1] = racer[0];
                swap = true;
            }
        }
    } while (swap);

    for (int i = 0; i < num_racers; i++) {
        OnlineRacer *racer = racers[i];

        racer->FinishedRaceStats.FinishPosition = i + 1;
    }
}

// OnlineManager.cpp:2960-3125 de la A124: los resultados de cada corredor en un SmartBitStream, en base64 al lobby.
void OnlineManager::SendEndOfRaceResults() {
    GRace::Type ugmMode = FEDatabase->GetOnlineSettings()->RaceMode;
    int8 race_mode = -1;

    if (ugmMode == GRace::kRaceType_Circuit) {
        race_mode = 0;
    } else if (ugmMode == GRace::kRaceType_P2P) {
        race_mode = 1;
    } else if (ugmMode == GRace::kRaceType_Drag) {
        race_mode = 2;
    }

    SmartBitStream data;

    data.AddByte(3);

    data.AddByte(GetNumRacers());
    data.AddByte(race_mode);
    data.AddByte(GetLocalRacer()->GetPlayerID());
    data.AddShort(TheRaceParameters.TrackNumber);
    data.AddByte(TheRaceParameters.TrackDirection);
    data.AddByte(TheRaceParameters.NumLapsInRace);

    for (IPlayer::List::const_iterator iter = IPlayer::GetList(PLAYER_ALL).begin();
         iter != IPlayer::GetList(PLAYER_ALL).end(); ++iter) {
        IPlayer *player = *iter;
        IOnlinePlayer *online_player;

        if (!player->QueryInterface(&online_player)) {
            break;
        }

        OnlineRacer *racer = online_player->GetOnlineRacer();
        GRacerInfo *racerInfo = GRaceStatus::Get().GetRacerInfo(player->GetSimable());
        FinishedRaceStatsEntry local_frs;

        data.AddByte(racer->GetPlayerID());
        data.AddByte(racerInfo->GetRanking());
        data.AddByte(racer->GetFinishedRaceStats()->FinishReason);

        data.AddByte(0);

        data.AddFloat(racerInfo->GetLapsCompleted());
        data.AddFloat(racerInfo->GetRaceTime());
        if (racerInfo->GetLapsCompleted()) {
            data.AddFloat(GRaceStatus::Get().GetBestLapTime(racerInfo->GetIndex()));
        } else {
            data.AddFloat(0.0f);
        }
        data.AddFloat(racerInfo->CalcAverageSpeed());
        data.AddFloat(racerInfo->GetTopSpeed());
        data.AddFloat(racerInfo->GetZeroToSixtyTime());
        data.AddFloat(racerInfo->GetQuarterMileTime());
        data.AddFloat(racer->GetFinishedRaceStats()->QuarterMileSpeed);
        data.AddFloat(racer->GetFinishedRaceStats()->LongestJump);
        data.AddFloat(0.0f);
        data.AddFloat(racerInfo->GetPoundsNOSUsed());

        uint8 lostconnection;
        switch (racer->GetState()) {
            case OPS_LOST_CONNECTION: lostconnection = 1; break;
            case OPS_QUIT: lostconnection = 2; break;
            case OPS_DISCERROR: lostconnection = 3; break;
            default: lostconnection = 0; break;
        }
        data.AddByte(lostconnection);

        uint16 cheat_bitmask = 0;
        for (int i = 0; i < NUM_CHEAT_REASONS; i++) {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
            if (racer->GetCheatTally(static_cast<eOnlineCheatReason>(i)) > 0) { // PC: `cmp; jbe` (0x741780)
#else
            if (racer->GetCheatTally(static_cast<eOnlineCheatReason>(i))) {
#endif
                cheat_bitmask |= 1 << i;
            }
        }
        data.AddShort(cheat_bitmask);

        for (int i = 0; i < NUM_CHEAT_REASONS; i++) {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
            if (racer->GetCheatTally(static_cast<eOnlineCheatReason>(i)) > 0) { // PC: `test; jbe` (0x741780)
#else
            if (racer->GetCheatTally(static_cast<eOnlineCheatReason>(i))) {
#endif
                data.AddShort(racer->GetCheatTally(static_cast<eOnlineCheatReason>(i)));
            }
        }
    }

    int length = data.GetByteLength();
    if (NetworkUseLobbies) {
        char b64encoded[1024];

        Base64::Encode(data.GetBuffer(), length, b64encoded, sizeof(b64encoded), true);

        LobbyCore::Instance().SaveRaceResults(b64encoded);
    }
}

void OnlineManager::SendSyncAnimations() {
    if (NumWorldObjects) {
        SmartBitStream data;
        data.AddByte(NumWorldObjects);
        data.AddQuantizedFloat(WorldTimer.GetSeconds(), QuantFloatTime);
        if (Online::IsInitialized()) {
            Online::SignalSyncAnimationMessage(data);
        }
    }
}

void OnlineManager::SignalScoreMessage(SmartBitStream &) {}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
// PC (el ctor 0x740FF0 la expande en ocho escrituras en orden creciente).
void OnlineManager::ClearAnimWorldObjects() {
    NumWorldObjects = 0;
    for (int i = 0; i < 8; ++i) {
        pAnimWorldObjects[i] = nullptr;
    }
}
#else
void OnlineManager::ClearAnimWorldObjects() {
    NumWorldObjects = 0;
    int i = 7;
    WorldObject **object = pAnimWorldObjects + 7;
    do {
        *object = nullptr;
        --i;
        --object;
    } while (i > -1);
}
#endif

void OnlineManager::BuildAnimWorldObjects() {
    ClearAnimWorldObjects();
}

void OnlineManager::SetPlayerIDs(const char (*personas)[16]) {
    bMemCpy(PersonaMap, personas, 0x40);
    RestartingRace = false;
    RestartRequested = false;
}

void OnlineManager::MapRacers2PlayerIDs() {
    int playerID = 0;
    do {
        if (PersonaMap[playerID][0] != '\0') {
            int driver_num = 0;
            do {
                OnlineRacer *racer = GetOnlineRacer(driver_num);
                if (racer && bStrICmp(PersonaMap[playerID], racer->GetPersona()) == 0) {
                    racer->SetPlayerID(playerID);
                }
                ++driver_num;
            } while (driver_num < 4);
        }
        ++playerID;
    } while (playerID < 4);
    bMemSet(PersonaMap, 0, 0x40);
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
void OnlineManager::ImportPositionData(int driver_number, SmartBitStream &data, ePosDataPriorityMask priority_mask) {
#else
void OnlineManager::ImportPositionData(int driver_number, SmartBitStream &data, float timestamp,
                                       ePosDataPriorityMask priority_mask) {
#endif
    if (pRacers[driver_number]) {
        if (data.GetByteLengthRemaining() > 1) {
            GetOnlineRacer(driver_number)->ImportStream(data, priority_mask);
        }
    }
}

void OnlineManager::ExportPositionData(int driver_number, SmartBitStream &data,
                                       ePosDataPriorityMask priority_mask) {
    if (pRacers[driver_number]) {
        OnlineRacer *racer = GetOnlineRacer(driver_number);
        if (pCurrentWorld) {
            racer->ExportStream(data, priority_mask);
        }
    }
}

void OnlineManager::CheckWorldTimerHacking() {
    if (TheOnlineManager.IsAntiCheatingEnabled()) {
        if (LastAntiCheatRealTimer.IsSet()) {
            float result = (WorldTimer - LastAntiCheatWorldTimer).GetSeconds() -
                           (RealTimer - LastAntiCheatRealTimer).GetSeconds();

            if (bAbs(result) > 0.5f) {
                OnlineRacer *server = GetServerRacer();

                if (server) {
                    server->DetectedCheat(CHEAT_TIMER_FUDGE);
                }
            }
        }
        LastAntiCheatRealTimer = RealTimer;
        LastAntiCheatWorldTimer = WorldTimer;
    }
}

void OnlineManager::SendLocalPlayerDataCRC() {
    if (pLocalRacer && ONLINE_CHEAT_NOSEND_CRC == 0) {
        SmartBitStream data;

        data.AddByte(pLocalRacer->GetDriverNumber());
        data.AddInt(pLocalRacer->GetDataCRC(true));
        if (Online::IsInitialized()) {
            Online::SignalDataCRCMessage(data);
        }
        if (TheOnlineManager.IsAntiCheatingEnabled() && pLocalRacer->GetDataCRC(false) != SavedLocalPhysicsCRC) {
            pLocalRacer->DetectedCheat(CHEAT_DATA_TAMPER);
        }
    }
}

void OnlineManager::SignalDataCRCMessage(SmartBitStream &data) {
    char driver_number;
    uint32 expected_crc;
    {
        uint32 temp = 0;
        data.GetBits(temp, 8);
        driver_number = static_cast<char>(temp);
    }
    {
        uint32 temp = 0;
        data.GetBits(temp, 32);
        expected_crc = temp;
    }

    if (IsValidRacer(driver_number)) {
        OnlineRacer *racer = GetOnlineRacer(driver_number);
        uint32 actual_crc = racer->GetDataCRC(false);

        if (TheOnlineManager.IsAntiCheatingEnabled() && expected_crc != actual_crc) {
            racer->DetectedCheat(CHEAT_DATA_TAMPER);
        }
    }
}

void OnlineManager::SignalRestartLoad(int driver_num) {
    if (pRacers[driver_num]) {
        GetOnlineRacer(driver_num)->ChangeState(OPS_CONNECTED);
    }
}

void OnlineManager::CheckGetAwayLeaderChange() {}

bool OnlineManager::HasAnyoneCheated() {
    bool has_cheated = false;
    int driver_number = 0;
    OnlineRacer **racer = pRacers;
    for (; driver_number < 4; ++racer, ++driver_number) {
        if (*racer && GetOnlineRacer(driver_number)->GetCheatScore()) {
            has_cheated = true;
            break;
        }
    }
    return has_cheated;
}

int OnlineManager::AreAllPlayersFinishedRacing() {
    if (State == OLS_RACE_END) {
        if (pLocalRacer->IsConnected() || pLocalRacer->GetState() == OPS_QUIT) {
            return 1;
        }

        int time_finished = TimeRaceFinished.GetPackedTime();
        int finished = 0;
        if (TimeRaceFinished.IsSet() &&
            static_cast<float>(WorldTimer.GetPackedTime() - time_finished) * 0.00025f > 3.0f) {
            finished = 1;
        }
        return finished;
    }
    return State == OLS_DISCONNECTED;
}

#if !defined(EA_PLATFORM_WIN32) && !defined(EA_PLATFORM_XENON)
void OnlineManager::NotifyDiscEjected() {
    if (pLocalRacer && pCurrentRace) {
        pLocalRacer->DriverDisconnect(OPS_DISCERROR, 0xc);
        int driver_number = 0;
        do {
            OnlineRacer *racer = GetOnlineRacer(driver_number);
            if (racer && racer != pLocalRacer) {
                racer->DriverDisconnect(OPS_DISCONNECTED, 0xc);
            }
            ++driver_number;
        } while (driver_number < 4);
        EndOnlineRace(false);
        Online::Close();
    }
}
#endif

#else
OnlineManager TheOnlineManager;

#ifdef EA_PLATFORM_PLAYSTATION2 // declarado en la cabecera solo en PS2 (SLUS 0x411400)
OnlineManager::~OnlineManager() {}
#endif

#if !ONLINE_SUPPORT || !defined(EA_PLATFORM_WIN32)
inline void OnlineManager::InitQuantizers() {}
#endif
#endif
