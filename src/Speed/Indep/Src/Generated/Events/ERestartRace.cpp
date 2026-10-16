#include "ERestartRace.hpp"

#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Camera/ICE/ICEManager.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/HUD/FEPkg_Hud.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOn.hpp"
#include "Speed/Indep/Src/Generated/Messages/MRestartRace.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/IGameState.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/ITrafficMgr.h"
#include "Speed/Indep/Src/Misc/Rumble.hpp"
#include "Speed/Indep/Src/World/VisualTreatment.h"
#include "Speed/Indep/Src/World/World.hpp"

void CleanParticlesOnRaceRestart();
void KillSkidsOnRaceRestart();

ERestartRace::ERestartRace() : Event(0x10) {
#ifdef EA_BUILD_A124
    // La A124 no mira Exists() ni las carreras de desafio y no ajusta la dificultad adaptativa.
    if (GRaceStatus::Get().GetActivelyRacing()) {
        g_pEAXSound->ReStartRace(true);
    } else {
        g_pEAXSound->ReStartRace(false);
    }
#else
    if (GRaceStatus::Exists() && GRaceStatus::Get().GetActivelyRacing()) {
        if (!GRaceStatus::IsChallengeRace()) {
            g_pEAXSound->ReStartRace(true);
        } else {
            g_pEAXSound->ReStartRace(false);
        }
    } else {
        g_pEAXSound->ReStartRace(false);
    }

    GRaceStatus::Get().UpdateAdaptiveDifficulty(GRaceStatus::kAdaptiveGain_FromRestart, NULL);
#endif

    FEngHud::bIsRestartingRace = true;

    MRestartRace().Send(UCrc32(0x20d60dbf));

    new EFadeScreenOn(
#ifndef EA_BUILD_A124
        false
#endif
    );
}

ERestartRace::~ERestartRace() {
    World_RestoreProps();
    CleanParticlesOnRaceRestart();
    KillSkidsOnRaceRestart();

    if (IVisualTreatment::Get()) {
        IVisualTreatment::Get()->Reset();
    }

    if (IGameState::Exists()) {
        IGameState::Get()->RaceReset();
    }

    if (ITrafficMgr::Exists()) {
#ifdef EA_BUILD_A124
        ITrafficMgr::Get()->FlushAllTraffic(false); // `move a1, zero` en la A124; SLUS y GameCube pasan true
#else
        ITrafficMgr::Get()->FlushAllTraffic(true);
#endif
    }

    ResetCameraShakers();
    CameraMoverRestartRace();

    TheICEManager.SetGenericCameraToPlay("", "");
}

const char *ERestartRace::GetEventName() const {
    return "ERestartRace";
}

void ERestartRace_MakeEvent_Callback(const void *staticData) {
    new ERestartRace();
}
