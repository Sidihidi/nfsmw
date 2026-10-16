#include "EVehicleDestroyed.hpp"

#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Generated/Events/EShowRaceOverMessage.hpp"
#include "Speed/Indep/Src/Generated/Messages/MNotifyVehicleDestroyed.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/IINput.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Sim/Simulation.h"

EVehicleDestroyed::EVehicleDestroyed(HSIMABLE phSimable) : Event(0x10), fhSimable(phSimable) {
}

#ifdef EA_BUILD_A124
// La A124 avisa del destrozo antes de mirar al jugador, y aun no ajusta la dificultad ni avisa en pantalla partida.
EVehicleDestroyed::~EVehicleDestroyed() {
    ISimable *isimable = ISimable::FindInstance(fhSimable);

    if (isimable) {
        MNotifyVehicleDestroyed(isimable->GetInstanceHandle()).Post(UCrc32(0x20d60dbf));

        IPlayer *iplayer = isimable->GetPlayer();

        if (iplayer) {
            if (iplayer->InGameBreaker()) {
                iplayer->ToggleGameBreaker();
            }
        }

        GRacerInfo *racerInfo = GRaceStatus::Get().GetRacerInfo(isimable);

        if (racerInfo) {
            racerInfo->TotalVehicle();
        }

        IInputPlayer *input;

        if (isimable->QueryInterface(&input)) {
            input->BlockInput(true);
        }
    }
}
#else
EVehicleDestroyed::~EVehicleDestroyed() {
    ISimable *isimable = ISimable::FindInstance(fhSimable);

    if (isimable) {
        IPlayer *iplayer = isimable->GetPlayer();

        if (iplayer) {
#ifndef EA_BUILD_A124 // la alpha 124 no tiene IPlayer::IsLocal (IPlayer.h)
            if (!iplayer->IsLocal()) {
                return;
            }
#endif

            if (iplayer->InGameBreaker()) {
                iplayer->ToggleGameBreaker();
            }
        }

        MNotifyVehicleDestroyed(isimable->GetInstanceHandle()).Post(UCrc32(0x20d60dbf));

        GRacerInfo *racerInfo = GRaceStatus::Get().GetRacerInfo(isimable);

        if (racerInfo) {
            racerInfo->TotalVehicle();
        }

        IInputPlayer *input;

        if (isimable->QueryInterface(&input)) {
            input->BlockInput(true);
        }

        GRaceStatus::Get().UpdateAdaptiveDifficulty(GRaceStatus::kAdaptiveGain_FromVehicleDestroyed, isimable);

        if (Sim::IsSplitScreen() && isimable->IsPlayer()) {
            new EShowRaceOverMessage(isimable->GetPlayer());
        }
    }
}
#endif

const char *EVehicleDestroyed::GetEventName() const {
    return "EVehicleDestroyed";
}

void EVehicleDestroyed_MakeEvent_Callback(const void *staticData) {
    new EVehicleDestroyed(((EVehicleDestroyed::StaticData *) staticData)->fhSimable);
}
