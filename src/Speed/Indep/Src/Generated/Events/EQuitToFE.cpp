#include "EQuitToFE.hpp"

#include "EFadeScreenOn.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Career/FEGameWonScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEAnyMovieScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetMain.hpp"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Input/IFeedBack.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Sim/Simulation.h"
#endif

EQuitToFE::EQuitToFE(eGarageType pGarageType, const char *pFirstScreenName)
    : Event(0x10), fGarageType(pGarageType), fFirstScreenName(pFirstScreenName) {
    // La PS2 de venta (SLUS) no funde la pantalla al crear el evento: su constructor (0x2f6b28, 36 B) solo guarda los
    // campos. La A124, GameCube y PC si crean el EFadeScreenOn.
#if !defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_BUILD_A124)
    new EFadeScreenOn(
#ifndef EA_BUILD_A124
        false
#endif
    );
#endif
}

#ifdef EA_BUILD_A124
// La A124 es otra version: sale tambien de una partida en linea, apaga la vibracion de cada jugador, cierra la pausa y
// para el sonido aqui, y no conoce el final del juego ni la vuelta al piso franco desde la ciudad.
EQuitToFE::~EQuitToFE() {
    if (Sim::GetState() != Sim::STATE_NONE) {
        FEManager::Get()->SetGarageType(fGarageType);

        if (fGarageType == GARAGETYPE_CUSTOMIZATION_SHOP) {
            FEDatabase->SetGameMode(eFE_GAME_MODE_CUSTOMIZE);
        }

        switch (GRaceStatus::Get().GetRaceContext()) {
            case kRaceContext_Online:
                FEDatabase->ResetGameMode();
                FEManager::Get()->ExitOnlineGameplayBasedOnConnection();
                break;

            case kRaceContext_Career:
                FEDatabase->ResetGameMode();
                FEDatabase->SetGameMode(eFE_GAME_MODE_CAREER);

                if (!bStrICmp(fFirstScreenName, "SafeHouseRivalChallenge.fng") || !bStrICmp(fFirstScreenName, "FEAnyMovie.fng")) {
                    FEDatabase->SetGameMode(eFE_GAME_MODE_POST_RIVAL);
                } else if (fGarageType == GARAGETYPE_CAR_LOT) {
                    FEDatabase->SetGameMode(eFE_GAME_MODE_CAR_LOT);
                    FEManager::Get()->SetFirstScreen("Car_Select.fng", 0, 0);
                } else if (GRaceStatus::Get().GetRaceParameters()) {
                    if (GRaceStatus::Get().GetRaceParameters()->GetIsDDayRace() || FEDatabase->IsFinalEpicChase()) {
                        FEDatabase->ClearGameMode(eFE_GAME_MODE_CAREER);
                        FEDatabase->SetGameMode(eFE_GAME_MODE_CAREER_MANAGER);
                        FEManager::Get()->SetGarageType(GARAGETYPE_MAIN_FE);
                        FEManager::Get()->SetFirstScreen("MainMenu_Sub.fng", 0, 0);
                    } else {
                        FEManager::Get()->SetFirstScreen("SafeHouseRaceSheet.fng", 0, 0);
                    }
                } else if (FEDatabase->GetCareerSettings()->GetCurrentBin() == 16) {
                    FEDatabase->ClearGameMode(eFE_GAME_MODE_CAREER);
                    FEDatabase->SetGameMode(eFE_GAME_MODE_CAREER_MANAGER);
                    FEManager::Get()->SetGarageType(GARAGETYPE_MAIN_FE);
                    FEManager::Get()->SetFirstScreen("MainMenu_Sub.fng", 0, 0);
                } else {
                    FEManager::Get()->SetFirstScreen("MainMenu_Sub.fng", 0, 0);
                }
                break;

            case kRaceContext_QuickRace:
                if (FEDatabase->IsChallengeMode()) {
                    FEManager::Get()->SetFirstScreen("MainMenu.fng", 0, 0);
                } else {
                    FEDatabase->ResetGameMode();
                    FEDatabase->SetGameMode(eFE_GAME_MODE_QUICK_RACE);
                    FEManager::Get()->SetFirstScreen("MainMenu_Sub.fng", 0, 0);
                }
                break;

            default:
                FEManager::Get()->SetFirstScreen("MainMenu.fng", 0, 0);
                break;
        }

        if (fFirstScreenName) {
            FEManager::Get()->SetFirstScreen(fFirstScreenName, 0, 0);
        }

        DismissChyron();

        for (IPlayer *player = IPlayer::First(PLAYER_ALL); player; player = player->Next(PLAYER_ALL)) {
            IFeedback *ffb = player->GetFFB();
            if (ffb) {
                ffb->ResetEffects();
            }
        }

        if (cFEng::Get()->IsPackagePushed("FadeScreen.fng")) {
            cFEng::Get()->PopNoControlPackage("FadeScreen.fng");
        }

        if (cFEng::Get()->IsPackagePushed("Pause_Main.fng")) {
            FEDatabase->NotifyExitRaceToFrontend(EXIT_RACE_FROM_PAUSE);
        }

        cFEng::Get()->QueuePackagePop(0);

        SoundPause(false, eSNDPAUSE_QUITTOFE);
        SetSoundControlState(false, SNDSTATE_OFF, "EQuitToFE");

        TheGameFlowManager.UnloadTrack();
        FEDatabase->SetPlayersJoystickPort(1, -1);
    }
}
#else
EQuitToFE::~EQuitToFE() {
    if (TheGameFlowManager.GetState() == GAMEFLOW_STATE_RACING) {
        bool isLanMode = FEDatabase->IsLANMode();

        FEManager::Get()->SetGarageType(fGarageType);

        if (fGarageType == GARAGETYPE_CUSTOMIZATION_SHOP) {
            FEDatabase->SetGameMode(eFE_GAME_MODE_CUSTOMIZE);
        }

        switch (GRaceStatus::Get().GetRaceContext()) {
            case kRaceContext_Online:
                if (isLanMode) {
                    FEDatabase->SetGameMode(eFE_GAME_MODE_LAN);
                } else {
                    FEDatabase->SetGameMode(eFE_GAME_MODE_ONLINE);
                }
                break;

            case kRaceContext_Career:
                FEDatabase->ResetGameMode();
                FEDatabase->SetGameMode(eFE_GAME_MODE_CAREER);

                if (!bStrICmp(fFirstScreenName, "SafeHouseRivalChallenge.fng") || !bStrICmp(fFirstScreenName, FEAnyMovieScreen::GetFEngPackageName())) {
                    FEDatabase->SetGameMode(eFE_GAME_MODE_POST_RIVAL);
                } else if (fGarageType == GARAGETYPE_CAR_LOT) {
                    FEDatabase->SetGameMode(eFE_GAME_MODE_CAR_LOT);
                    FEManager::Get()->SetFirstScreen("Car_Select.fng", 0, 0);
                } else if (GRaceStatus::Get().GetRaceParameters()) {
                    if (GRaceStatus::Get().GetRaceParameters()->GetIsDDayRace()) {
                        FEDatabase->ClearGameMode(eFE_GAME_MODE_CAREER);
                        FEDatabase->SetGameMode(eFE_GAME_MODE_CAREER_MANAGER);
                        FEManager::Get()->SetGarageType(GARAGETYPE_MAIN_FE);
                        FEManager::Get()->SetFirstScreen("MainMenu_Sub.fng", 0, 0);
                    } else if (FEDatabase->IsFinalEpicChase()) {
                        if (GRaceDatabase::Get().IsCareerRaceComplete(GRaceStatus::Get().GetRaceParameters()->GetEventHash())) {
                            FEDatabase->GetCareerSettings()->SetPlayerHasBeatenTheGame();
                            FEDatabase->SetGameMode(eFE_GAME_MODE_BEAT_GAME);
                            FEManager::Get()->SetGarageType(GARAGETYPE_CAREER_SAFEHOUSE);
                            FEGameWonScreen::Initialize();
                            FEManager::Get()->SetFirstScreen("GameWon.fng", 0, 0);
                        } else {
                            FEDatabase->ClearGameMode(eFE_GAME_MODE_CAREER);
                            FEDatabase->SetGameMode(eFE_GAME_MODE_CAREER_MANAGER);
                            FEManager::Get()->SetGarageType(GARAGETYPE_MAIN_FE);
                            FEManager::Get()->SetFirstScreen("MainMenu_Sub.fng", 0, 0);
                        }
                    } else {
                        iCurrentViewBin = FEDatabase->GetCareerSettings()->GetCurrentBin();
                        FEManager::Get()->SetFirstScreen("SafeHouseRaceSheet.fng", 0, 0);
                    }
                } else if (FEDatabase->GetCareerSettings()->GetCurrentBin() == 16) {
                    FEDatabase->ClearGameMode(eFE_GAME_MODE_CAREER);
                    FEDatabase->SetGameMode(eFE_GAME_MODE_CAREER_MANAGER);
                    FEManager::Get()->SetGarageType(GARAGETYPE_MAIN_FE);
                    FEManager::Get()->SetFirstScreen("MainMenu_Sub.fng", 0, 0);
                } else {
                    FEManager::Get()->SetFirstScreen("MainMenu_Sub.fng", 0, 0);
                }

                if (fGarageType == GARAGETYPE_CAREER_SAFEHOUSE && !GRaceStatus::Get().GetRaceParameters()) {
                    GManager::Get().SetStartingFreeRoamFromSafeHouse();
                }
                break;

            case kRaceContext_QuickRace:
                if (FEDatabase->IsChallengeMode()) {
                    FEManager::Get()->SetFirstScreen("MainMenu.fng", 0, 0);
                } else {
                    FEDatabase->ResetGameMode();
                    FEDatabase->SetGameMode(eFE_GAME_MODE_QUICK_RACE);
                    FEManager::Get()->SetFirstScreen("MainMenu_Sub.fng", 0, 0);
                }
                break;

            default:
                FEManager::Get()->SetFirstScreen("MainMenu.fng", 0, 0);
                break;
        }

        if (fFirstScreenName) {
            FEManager::Get()->SetFirstScreen(fFirstScreenName, 0, 0);
        }

        DismissChyron();

        if (cFEng::Get()->IsPackagePushed("FadeScreen.fng")) {
            cFEng::Get()->PopNoControlPackage("FadeScreen.fng");
        }

        if (cFEng::Get()->IsPackagePushed("Pause_Main.fng")) {
            FEDatabase->NotifyExitRaceToFrontend(EXIT_RACE_FROM_PAUSE);
        }

        TheGameFlowManager.UnloadTrack();
        FEDatabase->SetPlayersJoystickPort(1, -1);
    }
}
#endif

const char *EQuitToFE::GetEventName() const {
    return "EQuitToFE";
}

void EQuitToFE_MakeEvent_Callback(const void *staticData) {
    new EQuitToFE(((EQuitToFE::StaticData *) staticData)->fGarageType, ((EQuitToFE::StaticData *) staticData)->fFirstScreenName);
}
