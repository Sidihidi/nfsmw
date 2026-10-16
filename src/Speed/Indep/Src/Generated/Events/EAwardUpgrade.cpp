#include "EAwardUpgrade.hpp"

#include "Speed/Indep/Src/Frontend/Careers/UnlockSystem.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"

#ifdef EA_BUILD_A124
// En la A124 el evento hace su trabajo al destruirse (el constructor solo guarda la clave).
EAwardUpgrade::EAwardUpgrade(unsigned int pUpgradeKey) : Event(0x10), fUpgradeKey(pUpgradeKey) {}

EAwardUpgrade::~EAwardUpgrade() {
    Attrib::Gen::gameplay upgradeInst(fUpgradeKey, 0, NULL);
    const char *upgradeType = upgradeInst.UpgradeType();
    const char *markerType = upgradeInst.RewardMarkerType();

    if (upgradeType) {
        AwardUnlockUpgrade(upgradeInst);
    } else if (markerType) {
        bool immediate_reward = false;

        if (!GRaceStatus::Exists() || !GRaceStatus::Get().GetRaceParameters()->GetIsMarkerRace()) {
            immediate_reward = true;
        }

        TheFEMarkerManager.AwardMarker(upgradeInst, immediate_reward);
    }
}
#else
EAwardUpgrade::EAwardUpgrade(unsigned int pUpgradeKey) : Event(0x10), fUpgradeKey(pUpgradeKey) {
    Attrib::Gen::gameplay upgradeInst(fUpgradeKey, 0, NULL);
    const char *upgradeType = upgradeInst.UpgradeType();
    const char *markerType = upgradeInst.RewardMarkerType();

    if (upgradeType) {
        AwardUnlockUpgrade(upgradeInst);
    } else if (markerType) {
        bool immediate_reward = false;

        if (!GRaceStatus::Exists() || !GRaceStatus::Get().GetRaceParameters()->GetIsMarkerRace()) {
            immediate_reward = true;
        }

        TheFEMarkerManager.AwardMarker(upgradeInst, immediate_reward);
    }
}

EAwardUpgrade::~EAwardUpgrade() {
}
#endif

const char *EAwardUpgrade::GetEventName() const {
    return "EAwardUpgrade";
}

void EAwardUpgrade_MakeEvent_Callback(const void *staticData) {
    new EAwardUpgrade(((EAwardUpgrade::StaticData *) staticData)->fUpgradeKey);
}
