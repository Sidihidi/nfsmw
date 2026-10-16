#include "EFadeScreenOff.hpp"

#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FEJoyInput.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Generated/Events/ESndGameState.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Input/IFeedBack.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#endif


EFadeScreenOff::EFadeScreenOff(int pDurationScript) : Event(0x10), fDurationScript(pDurationScript) {
#ifndef EA_BUILD_A124 // como EFadeScreenOn: la A124 no se salta fotogramas al crear el evento
    Main_SkipFrame(2);
#endif
}

EFadeScreenOff::~EFadeScreenOff() {
    if (cFEng::Get()->IsPackagePushed("FadeScreen.fng")) {
#ifndef EA_BUILD_A124 // la A124 no tiene el fundido de solo barra de carga (APPEARINVISIBLE)
        if (FEngIsScriptSet("FadeScreen.fng", 0x027ff2dc, FEHASH_APPEARINVISIBLE) && fDurationScript == FEHASH_HIDE) {
            FEngSetScript("FadeScreen.fng", 0x027ff2dc, FEHASH_HIDE, true);
        } else
#endif
        if (fDurationScript == FEHASH_15_IN || fDurationScript == FEHASH_30_IN || fDurationScript == FEHASH_45_IN
#ifdef EA_BUILD_A124
            || fDurationScript == FEHASH_HIDE
#endif
        ) {
            if (FEngIsScriptSet("FadeScreen.fng", 0x027ff2dc, FEHASH_APPEAR)) {
                FEngSetScript("FadeScreen.fng", 0x027ff2dc, fDurationScript, true);
            }
        }
    }

#ifdef EA_BUILD_A124
    for (IPlayer *player = IPlayer::First(PLAYER_ALL); player; player = player->Next(PLAYER_ALL)) {
        IFeedback *ffb = player->GetFFB();
        if (ffb) {
            ffb->ResumeEffects();
        }
    }
#else
    if (TheGameFlowManager.IsInFrontend() && !cFEngJoyInput::Get()->IsJoyEnabled(JOYSTICK_PORT_ALL)) {
        cFEngJoyInput::Get()->JoyEnable(JOYSTICK_PORT_ALL, true);
    }
#endif

    new ESndGameState(11, false);
    SetSoundControlState(false, SNDSTATE_MINILOAD, "EFadeScreenOff");
}

const char *EFadeScreenOff::GetEventName() const {
    return "EFadeScreenOff";
}

void EFadeScreenOff_MakeEvent_Callback(const void *staticData) {
    new EFadeScreenOff(((EFadeScreenOff::StaticData *) staticData)->fDurationScript);
}
