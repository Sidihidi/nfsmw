#include "Speed/Indep/Src/Frontend/HUD/FeShiftUpdater.hpp"
#include "Speed/Indep/Src/FEng/FEObject.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"

extern float warningPulseMinRpm;

ShiftUpdater::ShiftUpdater(UTL::COM::Object *pOutter, const char *pkg_name, int player_number)
    : HudElement(pkg_name, 0x20000000), IShiftUpdater(pOutter) //
{
    mGear = G_NEUTRAL;
    mShiftPotential = SHIFT_POTENTIAL_NONE;
    mGearChanged = 0;
    mLastShiftStatus = SHIFT_STATUS_NONE;
    mIsEngineBlown = false;
    mEngineTemp = 0.0f;
    pShiftIndicator = RegisterImage(FEHashUpper("Shift_light"));
    pShiftIndicatorOverheatGroup = RegisterGroup(FEHashUpper("ENGINE_HEAT_SHIFTLIGHT_GROUP"));
    pShiftMsgGroup = RegisterGroup(FEHashUpper("SHIFT_GROUP"));
    pShiftMsg = static_cast<FEString *>(FEngFindObject(pkg_name, FEHashUpper("ShiftMessage")));
    pShiftMsgShadow = static_cast<FEString *>(FEngFindObject(pkg_name, FEHashUpper("ShiftMessage_Shadow")));
}

void ShiftUpdater::Update(IPlayer *player) {
    if (mShiftPotential >= SHIFT_POTENTIAL_GOOD && !mIsEngineBlown) {
        if (mShiftPotential == SHIFT_POTENTIAL_GOOD) {
            if (!FEngIsScriptSet(pShiftIndicator, FEHashUpper("PulseBlue"))) {
                FEngSetScript(pShiftIndicator, FEHashUpper("PulseBlue"), true);
            }
            FEngSetVisible(pShiftIndicator);
            FEngSetInvisible(pShiftIndicatorOverheatGroup);
        } else if (mShiftPotential == SHIFT_POTENTIAL_PERFECT) {
            if (!FEngIsScriptSet(pShiftIndicator, FEHashUpper("PulseGreen"))) {
                FEngSetScript(pShiftIndicator, FEHashUpper("PulseGreen"), true);
            }
            FEngSetVisible(pShiftIndicator);
            FEngSetInvisible(pShiftIndicatorOverheatGroup);
        } else if (mShiftPotential == SHIFT_POTENTIAL_MISS) {
            if (mEngineTemp > warningPulseMinRpm) {
                if (!FEngIsScriptSet(pShiftIndicatorOverheatGroup, FEHashUpper("OVERHEAT_PULSE"))) {
                    FEngSetScript(pShiftIndicatorOverheatGroup, FEHashUpper("OVERHEAT_PULSE"), true);
                }
                FEngSetInvisible(pShiftIndicator);
                FEngSetVisible(pShiftIndicatorOverheatGroup);
            } else {
                if (!FEngIsScriptSet(pShiftIndicator, FEHashUpper("PulseRed"))) {
                    FEngSetScript(pShiftIndicator, FEHashUpper("PulseRed"), true);
                }
                FEngSetVisible(pShiftIndicator);
                FEngSetInvisible(pShiftIndicatorOverheatGroup);
            }
        }
    } else {
        FEngSetInvisible(pShiftIndicator);
        FEngSetInvisible(pShiftIndicatorOverheatGroup);
    }

    if (mGear > G_FIRST && mGearChanged > 0 && mLastShiftStatus >= SHIFT_STATUS_NORMAL && !mIsEngineBlown) {
        if (mLastShiftStatus == SHIFT_STATUS_NORMAL) {
            FEngSetLanguageHash(pShiftMsg, LANGUAGE_HUD_SHIFT_EARLY);
            FEngSetLanguageHash(pShiftMsgShadow, LANGUAGE_HUD_SHIFT_EARLY);
            FEngSetScript(pShiftMsgGroup, FEHashUpper("EARLY"), true);
        } else if (mLastShiftStatus == SHIFT_STATUS_GOOD) {
            FEngSetLanguageHash(pShiftMsg, LANGUAGE_HUD_SHIFT_GOOD);
            FEngSetLanguageHash(pShiftMsgShadow, LANGUAGE_HUD_SHIFT_GOOD);
            FEngSetScript(pShiftMsgGroup, FEHashUpper("GOOD"), true);
        } else if (mLastShiftStatus == SHIFT_STATUS_PERFECT) {
            FEngSetLanguageHash(pShiftMsg, 0x598b065);
            FEngSetLanguageHash(pShiftMsgShadow, 0x598b065);
            FEngSetScript(pShiftMsgGroup, FEHashUpper("PERFECT"), true);
        } else if (mLastShiftStatus == SHIFT_STATUS_MISSED) {
            FEngSetLanguageHash(pShiftMsg, LANGUAGE_HUD_SHIFT_OVERREV);
            FEngSetLanguageHash(pShiftMsgShadow, LANGUAGE_HUD_SHIFT_OVERREV);
            FEngSetScript(pShiftMsgGroup, FEHashUpper("OVERREV"), true);
        }

        FEngSetInvisible(pShiftIndicator);
        mGearChanged = 0;
    }
}
