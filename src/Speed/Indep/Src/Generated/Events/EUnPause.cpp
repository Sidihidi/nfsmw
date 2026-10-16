#include "EUnPause.hpp"

#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/HUD/FEPkg_Hud.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Sim/Simulation.h"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Generated/Events/ESndGameState.hpp"
#endif

EUnPause::EUnPause() : Event(0x10) {
}

EUnPause::~EUnPause() {
    ePlayerHudType ht;

    cFEng::Get()->QueuePackagePop(0);
    FEManager::Get()->AllowControllerError(false);

    if (Sim::GetUserMode() != Sim::USER_SPLIT_SCREEN) {
        ht = GRaceStatus::Exists() && GRaceStatus::IsDragRace() ? PHT_DRAG : PHT_STANDARD;
    }

#ifdef EA_BUILD_A124
    // La A124 reanuda aqui el sonido (EPause lo pausa en su destructor).
    SoundPause(false, eSNDPAUSE_PAUSEMENU);
    new ESndGameState(8, false);
    SetSoundControlState(false, SNDSTATE_PAUSE, "EUnPause");
#endif
}

const char *EUnPause::GetEventName() const {
    return "EUnPause";
}

void EUnPause_MakeEvent_Callback(const void *staticData) {
    new EUnPause();
}
