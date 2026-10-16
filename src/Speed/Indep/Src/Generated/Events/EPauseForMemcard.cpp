#include "EPauseForMemcard.hpp"

#include "Speed/Indep/Src/EAXSound/SoundPause.h"
#include "Speed/Indep/Src/Generated/Events/ESndGameState.hpp"

#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"

EPauseForMemcard::EPauseForMemcard() : Event(0x10) {
}

EPauseForMemcard::~EPauseForMemcard() {
    SoundPause(true, eSNDPAUSE_MEMCARD);
    new ESndGameState(8, true);
    SetSoundControlState(true, SNDSTATE_OFF, "EPauseForMemcard");
}

const char *EPauseForMemcard::GetEventName() const {
    return "EPauseForMemcard";
}

void EPauseForMemcard_MakeEvent_Callback(const void *staticData) {
    new EPauseForMemcard();
}
