#include "EFadeScreenOn.hpp"

#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FEJoyInput.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Generated/Events/ESndGameState.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Input/IFeedBack.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#endif

EFadeScreenOn::EFadeScreenOn(
#ifndef EA_BUILD_A124
    bool pLoadingBarOnly
#endif
    )
    : Event(0x10)
#ifndef EA_BUILD_A124
      ,
      fLoadingBarOnly(pLoadingBarOnly)
#endif
{
#ifndef EA_BUILD_A124
    Main_SkipFrame(2);
#endif

    if (!cFEng::Get()->IsPackagePushed("FadeScreen.fng")) {
        cFEng::Get()->PushNoControlPackage("FadeScreen.fng", FE_PACKAGE_PRIORITY_SECOND_CLOSEST);
    }

#ifdef EA_BUILD_A124
    for (IPlayer *player = IPlayer::First(PLAYER_ALL); player; player = player->Next(PLAYER_ALL)) {
        IFeedback *ffb = player->GetFFB();
        if (ffb) {
            ffb->ResetEffects();
        }
    }
#else
    if (TheGameFlowManager.IsInFrontend()) {
        if (cFEngJoyInput::Get()->IsJoyEnabled(JOYSTICK_PORT_ALL)) {
            cFEngJoyInput::Get()->JoyDisable(JOYSTICK_PORT_ALL, true);
        }
    }
#endif

    FEManager::Get()->SuppressControllerError(true);

#ifndef EA_BUILD_A124
    if (!FEngIsScriptSet("FadeScreen.fng", 0x027FF2DC, FEHASH_APPEAR)) {
        if (fLoadingBarOnly) {
            FEngSetScript("FadeScreen.fng", 0x027FF2DC, FEHASH_APPEARINVISIBLE, true);
        } else
#endif
        {
            FEngSetScript("FadeScreen.fng", 0x027FF2DC, FEHASH_APPEAR, true);
        }
#ifndef EA_BUILD_A124
    }
#endif

    new ESndGameState(11, true);
    SetSoundControlState(true, SNDSTATE_MINILOAD, "EFadeScreenOn");
}

EFadeScreenOn::~EFadeScreenOn() {
}

const char *EFadeScreenOn::GetEventName() const {
    return "EFadeScreenOn";
}

void EFadeScreenOn_MakeEvent_Callback(const void *staticData) {
    new EFadeScreenOn(
#ifndef EA_BUILD_A124
        ((EFadeScreenOn::StaticData *) staticData)->fLoadingBarOnly
#endif
    );
}
