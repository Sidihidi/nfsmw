#include "EUnPauseForMemcard.hpp"

#include "Speed/Indep/Src/EAXSound/SoundPause.h"
#include "Speed/Indep/Src/Generated/Events/ESndGameState.hpp"

#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"

EUnPauseForMemcard::EUnPauseForMemcard() : Event(0x10) {
}

EUnPauseForMemcard::~EUnPauseForMemcard() {
    SoundPause(false, eSNDPAUSE_MEMCARD);
    new ESndGameState(8, false);
    SetSoundControlState(false, SNDSTATE_OFF, "EUnPauseForMemcard");
}

const char *EUnPauseForMemcard::GetEventName() const {
    return "EUnPauseForMemcard";
}

void EUnPauseForMemcard_MakeEvent_Callback(const void *staticData) {
    new EUnPauseForMemcard();
}
