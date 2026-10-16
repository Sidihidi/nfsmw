#include "EPause.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FeFadeScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsScreen.hpp"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOff.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Generated/Events/ESndGameState.hpp"
#include "Speed/Indep/Src/Sim/Simulation.h"
#endif

EPause::EPause(int pPlayerIndex, int pCalledFromPostRace
#ifndef EA_BUILD_A124
               ,
               int pShowFlatPause
#endif
               )
    : Event(
#ifdef EA_BUILD_A124
          0x10
#else
          0x20
#endif
          ),
      fPlayerIndex(pPlayerIndex), fCalledFromPostRace(pCalledFromPostRace)
#ifndef EA_BUILD_A124
      ,
      fShowFlatPause(pShowFlatPause)
#endif
{
}

#ifdef EA_BUILD_A124
// La A124 es otra version: sin la pausa desde el final de carrera, con la pausa del sonido aqui, la pausa para los dos
// mandos en pantalla partida y el nombre del paquete escrito en cada llamada (sus stabs no tienen la local).
EPause::~EPause() {
    const bool useControllerErrors = true;

    if (fPlayerIndex == -1) {
        fPlayerIndex = 0;
    }

    if (FEManager::IsOkayToRequestPauseSimulation(fPlayerIndex, useControllerErrors)) {
        if (!cFEng::Get()->IsPackagePushed("Pause_Main.fng")) {
            if (FadeScreen::IsFadeScreenOn()) {
                new EFadeScreenOff(FEHASH_15_IN);
            }

            SetPlayerToEditForOptions(fPlayerIndex);

            FEDatabase->SetGameMode(eFE_GAME_MODE_OPTIONS);
            FEngSetLastButton("Pause_Main.fng", 0);

            FEDatabase->ClearGameMode(eFE_GAME_MODE_OPTIONS);
            FEngSetLastButton("Pause_Main.fng", 0);

            SoundPause(true, eSNDPAUSE_PAUSEMENU);
            new ESndGameState(8, true);
            SetSoundControlState(true, SNDSTATE_PAUSE, "EPause");

            int port = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(0));

            if ((Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) | FEDatabase->IsSplitScreenMode()) {
                port |= FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(1));
            }

            cFEng::Get()->QueuePackagePush("Pause_Main.fng", fCalledFromPostRace, port, false);
        }
    }
}
#else
EPause::~EPause() {
#ifdef EA_PLATFORM_PLAYSTATION2
    const bool useControllerErrors = true; // las dos PS2 pasan 1; GameCube, 0
#else
    const bool useControllerErrors = false;
#endif

    if (fPlayerIndex == -1) {
        fPlayerIndex = 0;
    }

    char *packageName = "Pause_Main.fng";

    if (fCalledFromPostRace == 1 || FEManager::IsOkayToRequestPauseSimulation(fPlayerIndex, useControllerErrors
#ifndef EA_BUILD_A124
                                                                              ,
                                                                              false
#endif
                                                                              )) {
        if (!cFEng::Get()->IsPackagePushed(packageName)) {
            if (FadeScreen::IsFadeScreenOn()) {
                new EFadeScreenOff(FEHASH_15_IN);
            }

            SetPlayerToEditForOptions(fPlayerIndex);

            FEDatabase->SetGameMode(eFE_GAME_MODE_OPTIONS);
            FEngSetLastButton(packageName, 0);

            FEDatabase->ClearGameMode(eFE_GAME_MODE_OPTIONS);
            FEngSetLastButton(packageName, 0);

            int port = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(fPlayerIndex));

            if (!port) {
                port = 255;
            }

            cFEng::Get()->QueuePackagePush(packageName, fCalledFromPostRace, port, false);
        }
    }
}
#endif

const char *EPause::GetEventName() const {
    return "EPause";
}

void EPause_MakeEvent_Callback(const void *staticData) {
    new EPause(((EPause::StaticData *) staticData)->fPlayerIndex, ((EPause::StaticData *) staticData)->fCalledFromPostRace
#ifndef EA_BUILD_A124
               ,
               ((EPause::StaticData *) staticData)->fShowFlatPause
#endif
    );
}
