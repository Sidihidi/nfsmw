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

EPause::~EPause() {
    const bool useControllerErrors = false;

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
