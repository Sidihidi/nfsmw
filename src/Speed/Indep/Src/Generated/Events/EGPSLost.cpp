#include "EGPSLost.hpp"

#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Interfaces/IFengHud.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"

EGPSLost::EGPSLost() : Event(0x10) {
}

EGPSLost::~EGPSLost() {
#ifndef EA_BUILD_A124 // la A124 aun no avisa de la perdida del GPS (su destructor no llama a nada)
    if (IPlayer::First(PLAYER_LOCAL)->GetHud()) {
        IGenericMessage *igenericmessage;

        if (IPlayer::First(PLAYER_LOCAL)->GetHud()->QueryInterface(&igenericmessage)) {
            // TODO hash
            igenericmessage->RequestGenericMessage(GetTranslatedString(0xee236ed9), false, FEHASH_ZOOMINRED, 0, 0, GenericMessage_Priority_1);
        }
    }
#endif
}

const char *EGPSLost::GetEventName() const {
    return "EGPSLost";
}

void EGPSLost_MakeEvent_Callback(const void *staticData) {
    new EGPSLost();
}
