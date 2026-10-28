#include "FEManager.hpp"

#ifdef EA_PLATFORM_PLAYSTATION2 // el volante de PS2 trabaja con int (IsConnected__8LGWheelsi en SLUS)
#include "Speed/PSX2/Src/Logitech/LGWheels.h"
#else
#include "Speed/GameCube/Src/Logitech/LGWheels.h"
#endif
#include "Speed/Indep/Src/Camera/ICE/ICEManager.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/FEng/FEGameInterface.h"
#include "Speed/Indep/Src/FEng/FEPackage.h"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEGameInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEJoyInput.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FEBootFlowManager.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingControllerScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingTips.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetRivalFlow.hpp"
#include "Speed/Indep/Src/Frontend/MoviePlayer/MoviePlayer.hpp"
#include "Speed/Indep/Src/Frontend/FEngRender.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Generated/Messages/MControlPathfinder.h"
#include "Speed/Indep/Src/Input/IOModule.h"
#include "Speed/Indep/Src/Input/ISteeringWheel.h"
#include "Speed/Indep/Src/Input/SteeringWheelDevice.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/INIS.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/IFengHud.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Misc/EasterEggs.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/World/CarInfo.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FeFadeScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_Chyron.hpp"




void DismissChyron();
#include "Speed/Indep/Src/Frontend/HUD/FeMinimapStreamer.hpp"
#include "Speed/Indep/Src/Frontend/FECarLoader.hpp"
#include "Speed/Indep/Src/Frontend/FECarViewer.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#endif

extern int DoScreenPrintf;

FEManager *FEManager::mInstance = nullptr;
int FEManager::mPauseRequest = 0;
const char *FEManager::mPauseReason[8] = {nullptr};

int DrawFEng = 1;
int SummonChyronNow = 0;

FEManager::FEManager()
    : bSuppressControllerError(false), bAllowControllerError(false), mFirstScreen(nullptr), mFirstScreenArg(0),
#ifndef EA_BUILD_A124
      mFirstScreenMask(0xFF),
#endif
      mGarageType(GARAGETYPE_NONE), mPreviousGarageType(GARAGETYPE_NONE), mGarageBackground(nullptr), mFirstBoot(true), mEATraxDelay(0),
      mEATraxFirstButton(false) {
    for (int port = 0; port < 8; port++) {
        bWantControllerError[port] = false;
    }
}

void FEManager::Init() {
    if (!mInstance) {
        mInstance = new ("FEManager", 0) FEManager;
    }
    InitFEngMemoryPool();
    LoadingScreen::InitLoadingScreen();
    LoadingTips::InitLoadingTipsScreen();
    LoadingControllerScreen::InitLoadingControllerScreen();
    InitChyron();
    cFEngGameInterface::pInstance = new ("cFEngGameInterface", 0) cFEngGameInterface;
    ChoppedMiniMapManager::Init();
    cFEng::Init();
    cFEngRender::mInstance = new ("cFEngRender", 0) cFEngRender;
#ifdef EA_PLATFORM_PLAYSTATION2
    InitMemoryCard();
#endif
    FEManager::Get()->SetGarageType(GARAGETYPE_MAIN_FE);
    uiRepSheetRivalFlow::Init();
}

void FEManager::InitInput() {
    cFEngJoyInput::mInstance = new ("cFEngJoyInput", 0) cFEngJoyInput();
}

void FEManager::Destroy() {}

FEManager *FEManager::Get() {
    return mInstance;
}

eGarageType FEManager::GetGarageType() {
    return mGarageType;
}

void FEManager::SetGarageType(eGarageType pGarageType) {
    mPreviousGarageType = GetGarageType();
    mGarageType = pGarageType;
}

const char *FEManager::GetGarageNameFromType() {
    eGarageType garageTypeToUse = mGarageType;
    switch (garageTypeToUse) {
        case GARAGETYPE_NONE:
            return "";
        case GARAGETYPE_MAIN_FE:
            return "FRONTEND\\PLATFORMS\\PLATFORMCRIB.BIN";
        case GARAGETYPE_CAREER_SAFEHOUSE:
            return "FRONTEND\\PLATFORMS\\CAREER_SAFEHOUSE.BIN";
        case GARAGETYPE_CUSTOMIZATION_SHOP:
            return "FRONTEND\\PLATFORMS\\CUSTOMIZATION_SHOP.BIN";
        case GARAGETYPE_CUSTOMIZATION_SHOP_BACKROOM:
            return "FRONTEND\\PLATFORMS\\CUSTOMIZATION_SHOP_BACKROOM.BIN";
        case GARAGETYPE_CAR_LOT:
            return "FRONTEND\\PLATFORMS\\CAR_LOT.BIN";
        default:
            return "";
    }
}

const char *FEManager::GetGaragePrefixFromType(eGarageType pGarageType) {
    switch (pGarageType) {
        case GARAGETYPE_NONE:
            return "";
        case GARAGETYPE_MAIN_FE:
            return "QRACE";
        case GARAGETYPE_CAREER_SAFEHOUSE:
            return "CAREER";
        case GARAGETYPE_CUSTOMIZATION_SHOP:
        case GARAGETYPE_CUSTOMIZATION_SHOP_BACKROOM:
            return "CSHOP";
        case GARAGETYPE_CAR_LOT:
            return "CARLOT";
        default:
            return "";
    }
}

bool FEManager::IsOkayToRequestPauseSimulation(int playerIndex, bool useControllerErrors
#ifndef EA_BUILD_A124
                                               ,
                                               bool okIfAutoSaveActive
#endif
) {
#ifndef EA_BUILD_A124 // el prototipo no mira el estado del juego, los fundidos ni al jugador detenido
    if (TheGameFlowManager.GetState() != GAMEFLOW_STATE_RACING) {
        return false;
    }
#endif

    if (cFEng::Get()->IsPackagePushed("InGamePhotoMaster.fng")) {
        return false;
    }

    if (MemoryCard::GetInstance()->IsAutoSaving()
#ifndef EA_BUILD_A124
        && !okIfAutoSaveActive
#endif
    ) {
        return false;
    }

#ifndef EA_BUILD_A124
    if (FadeScreen::IsFadeScreenOn()) {
        return false;
    }

    if (cFEng::Get()->IsPackagePushed("FadeScreenNoLoadingBar.fng")) {
        return false;
    }
#endif

    if (GRaceStatus::Exists()) {
#ifndef EA_BUILD_A124
        ISimable *simable = IPlayer::GetList(PLAYER_LOCAL)[static_cast<unsigned int>(playerIndex)]->GetSimable();
        GRacerInfo *racerInfo;
        if (simable) {
            racerInfo = GRaceStatus::Get().GetRacerInfo(simable);
        } else {
            racerInfo = nullptr;
        }
#endif

        if (GRaceStatus::Get().GetPlayMode() == GRaceStatus::kPlayMode_Racing) {
            if (GRaceStatus::Get().GetIsTimeLimited() && GRaceStatus::Get().GetRaceTimeRemaining() <= 0.0f) {
                return false;
            }

#ifdef EA_BUILD_A124
            GRacerInfo *racerInfo = &GRaceStatus::Get().GetRacerInfo(playerIndex);
#endif
            if (racerInfo) {
                if (racerInfo->GetIsEngineBlown() || racerInfo->GetIsTotalled() || racerInfo->GetIsKnockedOut() || racerInfo->IsFinishedRacing()) {
#ifdef EA_BUILD_A124
                    return false;
#else
                    if (Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) {
                        int other_player = 1;
                        if (playerIndex == 1) {
                            other_player = 0;
                        }
                        ISimable *other_simable = IPlayer::GetList(PLAYER_LOCAL)[static_cast<unsigned int>(other_player)]->GetSimable();
                        GRacerInfo *other_racerInfo;
                        if (other_simable) {
                            other_racerInfo = GRaceStatus::Get().GetRacerInfo(other_simable);
                        } else {
                            other_racerInfo = nullptr;
                        }
                        if (other_racerInfo && (other_racerInfo->GetIsEngineBlown() || other_racerInfo->GetIsTotalled() ||
                                                other_racerInfo->GetIsKnockedOut() || other_racerInfo->IsFinishedRacing())) {
                            return false;
                        }
                    } else {
                        return false;
                    }
#endif
                }

#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124) // SLUS no deja pausar durante la cuenta atras online
                                                                    // (ni GameCube ni la A124 lo tienen)
                if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
                    IHud *hud = racerInfo->GetSimable()->GetPlayer()->GetHud();
                    if (hud) {
                        ICountdown *icountdown;
                        if (hud->QueryInterface(&icountdown) && icountdown->IsActive()) {
                            return false;
                        }
                    }
                }
#endif
            }
        }
#ifndef EA_BUILD_A124
        else if (simable) {
            IVehicle *vehicle;
            if (simable->QueryInterface(&vehicle)) {
                IVehicleAI *vehicleai = vehicle->GetAIVehiclePtr();
                if (vehicleai) {
                    IPursuit *ipursuit = vehicleai->GetPursuit();
                    if (ipursuit && ipursuit->IsPerpBusted()) {
                        return false;
                    }
                }
            }
        }
#endif
    }

    return !ShouldPauseSimulation(useControllerErrors);
}

bool FEManager::ShouldPauseSimulation(bool useControllerErrors) {
    if (!mInstance->bSuppressControllerError && mInstance->WaitingForControllerError() && useControllerErrors &&
        !UTL::Collections::Singleton<INIS>::Get() && !gMoviePlayer) {
        return true;
    }
    return mPauseRequest;
}

void FEManager::RequestPauseSimulation(const char *reason) {
    mPauseReason[mPauseRequest++] = reason;
}

void FEManager::RequestUnPauseSimulation(const char *reason) {
    mPauseRequest--;
}

#ifndef EA_BUILD_A124
void FEManager::WantControllerError(int port) {
    if (port == -1) {
        return;
    }

    if (TheGameFlowManager.IsInGame() && (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode())) {
        ISimable *simable = IPlayer::First(PLAYER_LOCAL)->GetSimable();
        GRacerInfo *racerInfo;
        if (simable) {
            racerInfo = GRaceStatus::Get().GetRacerInfo(simable);
        } else {
            racerInfo = nullptr;
        }
        if (racerInfo) {
            IPlayer *player = racerInfo->GetSimable()->GetPlayer();
            if (player->GetHud()) {
                ICountdown *icountdown;
                if (player->GetHud()->QueryInterface(&icountdown) && icountdown->IsActive()) {
                    return;
                }
            }
        }
    }

    bWantControllerError[port] = true;
}
#endif

bool FEManager::WaitingForControllerError() {
    for (int port = 0; port < 8; port++) {
        if (bWantControllerError[port]) {
            return true;
        }
    }
    return false;
}

void FEManager::StartFE() {
    if (mFirstBoot) {
        mFirstBoot = false;
        BootFlowManager::Init();
    } else {
        g_pEAXSound->PlayFEMusic(-1);
        if (!CarViewer::haveLoadedOnce) {
            RideInfo ride;
#ifndef EA_PLATFORM_PLAYSTATION2
            CarViewer::ShowCarScreen();
#endif
            FEDatabase->BuildCurrentRideForPlayer(0, &ride);
            CarViewer::SetRideInfo(&ride, SET_RIDE_INFO_REASON_CATCHALL, eCARVIEWER_PLAYER1_CAR);
#ifdef EA_PLATFORM_PLAYSTATION2
            CarViewer::ShowCarScreen();
#endif
            CarViewer::haveLoadedOnce = true;
        }
        CarViewer::ShowAllCars();
    }

    bSuppressControllerError = false;
    bAllowControllerError = false;
    for (int port = 0; port < 8; port++) {
        bWantControllerError[port] = false;
    }
    mPauseRequest = 0;
    cFEng::Get()->QueuePackagePush(mFirstScreen, mFirstScreenArg,
#ifdef EA_BUILD_A124
                                   0,
#else
                                   mFirstScreenMask,
#endif
                                   false);
}

void FEManager::StopFE() {
    cFEngJoyInput::mInstance->JoyDisable(JOYSTICK_PORT_ALL, true);
    FEPackageManager::Get()->CloseAllPackages(0);
    BootFlowManager::Destroy();
    mEATraxDelay = 0;
}

void FEManager::Render() {
#ifdef EA_PLATFORM_XENON
    // Xenon: sin la comprobacion de DrawFEng (EGM 0x8229EE50, 0x10 B, con el menu de depuracion escribiendo DrawFEng
    // en 0x822AE208); la hacen los llamantes del render de Xenon (0x82302DB0 y 0x82476208).
    cFEng::Get()->DrawForeground();
#else
    if (DrawFEng) {
        cFEng::Get()->DrawForeground();
    }
#endif
}

// STRIPPED
void FEManager::UpdateJoyInput() {}

void SteeringWheels_StopAllForces() {
    if (SteeringWheelDevice::lgwheels != nullptr) {
        for (int mDeviceIndex = 0; mDeviceIndex < 2; mDeviceIndex++) {
            if (SteeringWheelDevice::lgwheels->IsConnected(mDeviceIndex)) {
                SteeringWheelDevice::lgwheels->StopConstantForce(mDeviceIndex);
                SteeringWheelDevice::lgwheels->StopSurfaceEffect(mDeviceIndex);
                SteeringWheelDevice::lgwheels->StopDamperForce(mDeviceIndex);
                SteeringWheelDevice::lgwheels->StopCarAirborne(mDeviceIndex);
                SteeringWheelDevice::lgwheels->StopSlipperyRoadEffect(mDeviceIndex);
                SteeringWheelDevice::lgwheels->PlaySpringForce(mDeviceIndex, 0, 200, 200);
            }
        }
    }
}

int GetPortsPlayer(int port) {
    if (FEDatabase->GetPlayersJoystickPort(0) != -1 && FEDatabase->GetPlayersJoystickPort(0) == port) {
        return 0;
    }
    if (FEDatabase->GetPlayersJoystickPort(1) != -1 && FEDatabase->GetPlayersJoystickPort(1) == port) {
        return 1;
    }
    return -1;
}

#ifdef EA_BUILD_A124
// La A124 no para la vibracion ni limpia el error de los mandos al desenchufar uno, con el error del motor no hace nada
// mas (no hay ErrorTick), escribe en pantalla el paquete con el control y hace avanzar la tarjeta de memoria al final.
void FEManager::Update() {
    if (!Sim::Exists() || (Sim::Exists() && Sim::GetState() != Sim::STATE_ACTIVE) || UTL::Collections::Singleton<INIS>::Get()) {
        SteeringWheels_StopAllForces();
    }

    if (cFEngJoyInput::mInstance) {
        cFEngJoyInput::mInstance->HandleJoy();
    }

    for (int port = 0; port < 8; port++) {
        if (bWantControllerError[port]) {
            if (((!UTL::Collections::Singleton<INIS>::Get() && !gMoviePlayer) || bAllowControllerError) && !bSuppressControllerError) {
                cFEng::Get()->PushErrorPackage("ControllerUnplugged.fng", port, FEngMapJoyportToJoyParam(port));
                break;
            }
        }
    }

    cFEng::Get()->Service();

    if (cFEng::Get()->IsErrorState()) {
        return;
    }

    FEPackageManager::Get()->Tick();

    if (TheGameFlowManager.IsInFrontend()) {
        UpdateGarageCarLoaders();
    }

    if (DoScreenPrintf && !TheICEManager.IsEditorOn()) {
        FEPackage *pCurrentPkgWithControl = cFEng::Get()->FindPackageWithControl();
        if (pCurrentPkgWithControl) {
            ScreenPrintf(-300, 75, "%s
", pCurrentPkgWithControl->GetName());
        }
    }

    gEasterEggs.HandleJoy();

    if (MemoryCard::GetInstance()) {
        MemoryCard::GetInstance()->Tick(static_cast<int>(RealTimeElapsed * 1000.0f));
    }

    if (gMoviePlayer) {
        gMoviePlayer->Update();
    }

    if (SummonChyronNow) {
        SummonChyron(0, 0, 0);
        SummonChyronNow = 0;
    } else if (mEATraxDelay >= 0) {
        mEATraxDelay--;
        if (mEATraxDelay == 0) {
            SummonChyron(0, 0, 0);
        }
    }
}
#else
void FEManager::Update() {
    if (MemoryCard::GetInstance()) {
        MemoryCard::GetInstance()->Tick(static_cast<int>(RealTimeElapsed * 1000.0f));
    }

    if (!Sim::Exists() || (Sim::Exists() && Sim::GetState() != Sim::STATE_ACTIVE) || UTL::Collections::Singleton<INIS>::Get()) {
        SteeringWheels_StopAllForces();
    }

    if (cFEngJoyInput::mInstance) {
        cFEngJoyInput::mInstance->HandleJoy();
    }

    int port;
    for (port = 0; port < 8; port++) {
        if (bWantControllerError[port]) {
            if ((!UTL::Collections::Singleton<INIS>::Get() && !gMoviePlayer) || bAllowControllerError) {
                if (!bSuppressControllerError) {
#ifndef EA_PLATFORM_PLAYSTATION2
                    if (TheGameFlowManager.IsInGame() && FEManager::IsPaused()) {
                        FEManager *feManager = FEManager::Get();
                        JoystickPort player_port1 = static_cast<JoystickPort>(FEDatabase->GetPlayersJoystickPort(0));
                        feManager->ClearControllerError(static_cast<int>(player_port1));
                        JoystickPort player_port2 = static_cast<JoystickPort>(FEDatabase->GetPlayersJoystickPort(1));
                        feManager->ClearControllerError(static_cast<int>(player_port2));
                    }
#endif

                    int maxPort = IOModule::GetIOModule().GetNumDevices();
                    for (int p = 0; p < maxPort; p++) {
                        InputDevice *device = IOModule::GetIOModule().GetDevice(p);
                        if (device) {
                            device->StopVibration();
                        }
                    }

                    if (!cFEng::Get()->IsPackagePushed("ControllerUnplugged.fng")) {
                        cFEng *eng = cFEng::Get();
                        u32 joyParam = FEngMapJoyportToJoyParam(port);
                        eng->PushErrorPackage("ControllerUnplugged.fng", port, joyParam);
                    }
                }
            }
            break;
        }
    }

    cFEng::Get()->Service();

    if (cFEng::Get()->IsErrorState()) {
        FEPackageManager::Get()->ErrorTick();
    } else {
        FEPackageManager::Get()->Tick();

        if (TheGameFlowManager.IsInFrontend()) {
            UpdateGarageCarLoaders();
        }

        if (DoScreenPrintf && !TheICEManager.IsEditorOn()) {
            FEPackage *pCurrentPkgWithControl = cFEng::Get()->FindPackageWithControl();
            if (pCurrentPkgWithControl) {
                pCurrentPkgWithControl->GetName();
            }
        }

        gEasterEggs.HandleJoy();

        if (gMoviePlayer) {
            gMoviePlayer->Update();
        }

        if (SummonChyronNow) {
            SummonChyron(0, 0, 0);
            SummonChyronNow = 0;
        } else {
            if (mEATraxDelay >= 0) {
                mEATraxDelay--;
                if (mEATraxDelay == 0) {
                    SummonChyron(0, 0, 0);
                }
            }
        }
    }
}
#endif

void FEManager::SetEATraxSecondButton() {
    if (gMoviePlayer && gMoviePlayer->IsMoviePlaying()) {
        return;
    }

    if (!cFEng::Get()->IsPackagePushed("EA_Trax_Jukebox.fng") && TheGameFlowManager.IsInFrontend()) {
        // Sobre un temporal: PC calcula el hash antes de construir el mensaje.
        MControlPathfinder(true, 0xffffffff, 0, 0).Send(UCrc32("Pathfinder5"));
    }
}

#ifdef EA_BUILD_A124
void FEManager::ExitOnlineGameplayBasedOnConnection() {
    if (NetworkCore::Instance().IsInitialized()) {
        if (FEDatabase->IsLANMode()) {
            FEDatabase->SetGameMode(eFE_GAME_MODE_LAN);
        }
        FEDatabase->SetGameMode(eFE_GAME_MODE_ONLINE);
        FEManager::Get()->SetFirstScreen(g_GameRoomName, 0, 0);
    } else {
        FEManager::Get()->SetFirstScreen("MainMenu.fng", 0, 0);
    }
}
#else
void FEManager::ExitOnlineGameplayBasedOnConnection() {}
#endif
