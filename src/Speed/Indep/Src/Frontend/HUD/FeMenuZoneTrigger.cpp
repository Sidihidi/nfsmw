#include "Speed/Indep/Src/Frontend/HUD/FeMenuZoneTrigger.hpp"

#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/FEng/FEImage.h"
#include "Speed/Indep/Src/FEng/FEList.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/customize/FECustomize.hpp"
#include "Speed/Indep/Src/Generated/Events/EQuitToFE.hpp"
#include "Speed/Indep/Src/Generated/Events/ERaceSheetOn.hpp"
#include "Speed/Indep/Src/Generated/Events/ERequestEventInfoDialog.hpp"
#include "Speed/Indep/Src/Generated/Messages/MEnterSafeHouse.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#endif

#ifdef EA_BUILD_A124
MenuZoneTrigger::MenuZoneTrigger(UTL::COM::Object *pOuter, const char *pkg_name, int player_number)
    : HudElement(pkg_name, 0x400000), IMenuZoneTrigger(pOuter), mZoneType(nullptr), mpRaceActivity(nullptr), mbShowingForAutoSave(false),
      mbInsideTrigger(false) {
    RegisterGroup(FEHashUpper("Engage_Mechanic"));
    mEventIcon = RegisterImage(FEHashUpper("EventIcon"));
}

void MenuZoneTrigger::Update(IPlayer *player) {}

bool MenuZoneTrigger::ShouldSeeMenuZoneCluster() {
    return GRaceStatus::Get().GetPlayMode() == GRaceStatus::kPlayMode_Roaming || MemoryCard::GetInstance()->IsAutoSaving();
}

bool MenuZoneTrigger::IsPlayerInsideTrigger() {
    return FEngIsScriptSet(reinterpret_cast<FEObject *>(mEventIcon), FEHASH_BLINK);
}

void MenuZoneTrigger::EnterTriggerForAutoSave() {
    mbShowingForAutoSave = true;
    if (mbInsideTrigger) {
        PulseDPadButton(ENGAGE_DPAD_ELEMENT_UP, false);
    } else {
        FEngSetScript(reinterpret_cast<FEObject *>(mEventIcon), FEHASH_BLINK, true);
    }
}

void MenuZoneTrigger::ExitTriggerForAutoSave() {
    mbShowingForAutoSave = false;
    if (mbInsideTrigger) {
        PulseDPadButton(ENGAGE_DPAD_ELEMENT_UP, true);
    } else {
        FEngSetScript(reinterpret_cast<FEObject *>(mEventIcon), FEHASH_LEAVE, true);
    }
}

void MenuZoneTrigger::EnterTrigger(GRuntimeInstance *pRaceActivity) {
    mbInsideTrigger = true;
    mpRaceActivity = pRaceActivity;

    if (!mbShowingForAutoSave) {
        PulseDPadButton(ENGAGE_DPAD_ELEMENT_UP, true);
        FEngSetScript(reinterpret_cast<FEObject *>(mEventIcon), FEHASH_BLINK, true);
    }

    GRaceParameters *race = GRaceDatabase::Get().GetRaceFromActivity(reinterpret_cast<GActivity *>(mpRaceActivity));
    FEngSetTextureHash(mEventIcon, FEDatabase->GetRaceIconHash(race->GetRaceType()));
}

void MenuZoneTrigger::EnterTrigger(const char *zoneType) {
    mbInsideTrigger = true;
    mZoneType = zoneType;

    if (!mbShowingForAutoSave) {
        PulseDPadButton(ENGAGE_DPAD_ELEMENT_UP, true);
        FEngSetScript(reinterpret_cast<FEObject *>(mEventIcon), FEHASH_BLINK, true);
    }

    FEngSetTextureHash(mEventIcon, FEDatabase->GetSafehouseIconHash(zoneType));
}

void MenuZoneTrigger::ExitTrigger() {
    mbInsideTrigger = false;
    mZoneType = 0;
    mpRaceActivity = 0;

    if (!mbShowingForAutoSave) {
        PulseDPadButton(ENGAGE_DPAD_ELEMENT_UP, false);
        FEngSetScript(reinterpret_cast<FEObject *>(mEventIcon), FEHASH_LEAVE, true);
    }
}
#else
MenuZoneTrigger::MenuZoneTrigger(UTL::COM::Object *pOuter, const char *pkg_name, int player_number)
    : HudElement(pkg_name, 0x400000), IMenuZoneTrigger(pOuter) {
    mZoneType = nullptr;
    mpRaceActivity = nullptr;
    mbCingularQueued = 0;
    mbInsideTrigger = 0;
    mCingularTimer.UnSet();
    mEngageMechanic = RegisterGroup(FEHashUpper("Engage_Mechanic"));
    mEventIcon = RegisterImage(FEHashUpper("EventIcon"));
    mCingularIcon = RegisterGroup(0xDA8141D4);
}

void MenuZoneTrigger::Update(IPlayer *player) {
    if (mbCingularQueued) {
        mbCingularQueued = false;
        mCingularTimer = WorldTimer;
        PulseDPadButton(ENGAGE_DPAD_ELEMENT_RIGHT, reinterpret_cast<FEObject *>(mCingularIcon));
    } else if (mCingularTimer.IsSet()) {
        if ((WorldTimer - mCingularTimer).GetSeconds() >= 6.0f) {
            mCingularTimer.UnSet();
            if (mbInsideTrigger) {
                PulseDPadButton(ENGAGE_DPAD_ELEMENT_UP, mEventIcon);
            } else {
                HideDPadButton();
            }
        }
    }
}

bool MenuZoneTrigger::ShouldSeeMenuZoneCluster() {
    return GRaceStatus::Get().GetPlayMode() == GRaceStatus::kPlayMode_Roaming;
}

bool MenuZoneTrigger::IsPlayerInsideTrigger() {
    return FEngIsScriptSet(reinterpret_cast<FEObject *>(mEventIcon), FEHASH_BLINK);
}

void MenuZoneTrigger::EnterTrigger(GRuntimeInstance *pRaceActivity) {
    mbInsideTrigger = 1;
    mpRaceActivity = pRaceActivity;

    PulseDPadButton(ENGAGE_DPAD_ELEMENT_UP, reinterpret_cast<FEObject *>(mEventIcon));

    GRaceParameters *race = GRaceDatabase::Get().GetRaceFromActivity(reinterpret_cast<GActivity *>(mpRaceActivity));
    FEngSetTextureHash(mEventIcon, FEDatabase->GetRaceIconHash(race->GetRaceType()));
}

void MenuZoneTrigger::EnterTrigger(const char *zoneType) {
    mbInsideTrigger = 1;
    mZoneType = zoneType;

    PulseDPadButton(ENGAGE_DPAD_ELEMENT_UP, reinterpret_cast<FEObject *>(mEventIcon));

    FEngSetTextureHash(mEventIcon, FEDatabase->GetSafehouseIconHash(zoneType));
}

void MenuZoneTrigger::ExitTrigger() {
    mbInsideTrigger = 0;
    mZoneType = 0;
    mpRaceActivity = 0;

    HideDPadButton();
}
#endif

void MenuZoneTrigger::RequestEventInfoDialog(int port) {
    if (mpRaceActivity != nullptr) {
        GRaceParameters *parms = GRaceDatabase::Get().GetRaceFromActivity(reinterpret_cast<GActivity *>(mpRaceActivity));

        if (parms != nullptr && parms->GetIsBossRace()) {
            new ERaceSheetOn(3);
        } else {
            new ERequestEventInfoDialog(port, mpRaceActivity);
        }
    }
}

void MenuZoneTrigger::RequestZoneInfoDialog(int port) {
    if (bStrCmp(mZoneType, "safehouse") == 0 || bStrCmp(mZoneType, "carlot") == 0 || bStrCmp(mZoneType, "customshop") == 0) {
        MEnterSafeHouse(mZoneType).Post(UCrc32(0x20D60DBF));
    }
}

bool MenuZoneTrigger::IsType(const char *t) {
    return bStrCmp(mZoneType, t) == 0;
}

void MenuZoneTrigger::RequestDoAction() {
    if (bStrCmp(mZoneType, "safehouse") == 0) {
        new EQuitToFE(GARAGETYPE_CAREER_SAFEHOUSE, "MainMenu_Sub.fng");
    } else if (bStrCmp(mZoneType, "carlot") == 0) {
        new EQuitToFE(GARAGETYPE_CAR_LOT, "Car_Select.fng");
    } else if (bStrCmp(mZoneType, "customshop") == 0) {
#ifdef EA_BUILD_A124
        // La A124 le busca un bloque de personalizacion libre al coche que aun no tiene.
        FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
        FECarRecord *record = stable->GetCarRecordByHandle(FEDatabase->GetCareerSettings()->GetCurrentCar());
        if (record->IsCustomized()) {
            BeginCarCustomize(CEP_GAMEPLAY, record);
            new EQuitToFE(GARAGETYPE_CUSTOMIZATION_SHOP, "CustomizeMain.fng");
        } else {
            FECustomizationRecord *custom = nullptr;
            for (int i = 0; i < MAX_CUSTOMIZE_BLOCKS; i++) {
                custom = stable->GetCustomizationRecordByHandle(i);
                if (custom->Handle == 0xFF) {
                    custom->Handle = i;
                    break;
                }
            }
            record->Customization = custom->Handle;
            BeginCarCustomize(CEP_GAMEPLAY, record);
            new EQuitToFE(GARAGETYPE_CUSTOMIZATION_SHOP, "CustomizeMain.fng");
        }
#else
        FECarRecord *record = FEDatabase->GetPlayerCarStable(0)->GetCarRecordByHandle(FEDatabase->GetCareerSettings()->GetCurrentCar());
        if (record->IsCustomized()) {
            BeginCarCustomize(CEP_GAMEPLAY, record);
            new EQuitToFE(GARAGETYPE_CUSTOMIZATION_SHOP, "CustomizeMain.fng");
        }
#endif
    }
}

#ifdef EA_BUILD_A124
void MenuZoneTrigger::PulseDPadButton(ENGAGE_DPAD_ELEMENT_DIRECTION direction, bool enable) {
    unsigned int objectHash = 0;
    switch (direction) {
    case ENGAGE_DPAD_ELEMENT_UP:
        objectHash = 0x0A729B1B;
        break;
    case ENGAGE_DPAD_ELEMENT_DOWN:
        objectHash = 0x717C82AE;
        break;
    case ENGAGE_DPAD_ELEMENT_RIGHT:
        objectHash = 0xA206A0B4;
        break;
    case ENGAGE_DPAD_ELEMENT_LEFT:
        objectHash = 0x7180B901;
        break;
    }

    if (enable) {
        FEngSetScript(GetPackageName(), objectHash, FEHASH_APPEAR, true);
        g_pEAXSound->PlayUISoundFX(UISND_ENTER_TRIGGER);
    } else {
        FEngSetScript(GetPackageName(), objectHash, FEHASH_LEAVE, true);
    }
}
#else
void MenuZoneTrigger::HideDPadButton() {
    FEObject *objectPtr;

    if (!FEngIsScriptSet(reinterpret_cast<FEObject *>(mEventIcon), FEHASH_LEAVE) &&
        !FEngIsScriptSet(reinterpret_cast<FEObject *>(mEventIcon), FEHASH_HIDE)) {
        FEngSetScript(reinterpret_cast<FEObject *>(mEventIcon), FEHASH_LEAVE, true);
    }

    if (!FEngIsScriptSet(reinterpret_cast<FEObject *>(mCingularIcon), FEHASH_LEAVE) &&
        !FEngIsScriptSet(reinterpret_cast<FEObject *>(mCingularIcon), FEHASH_HIDE)) {
        FEngSetScript(reinterpret_cast<FEObject *>(mCingularIcon), FEHASH_LEAVE, true);
    }

    objectPtr = FEngFindObject(GetPackageName(), 0x0A729B1B);
    if (objectPtr != nullptr && !FEngIsScriptSet(objectPtr, FEHASH_LEAVE) && !FEngIsScriptSet(objectPtr, FEHASH_INIT)) {
        FEngSetScript(objectPtr, FEHASH_LEAVE, true);
    }

    objectPtr = FEngFindObject(GetPackageName(), 0x717C82AE);
    if (objectPtr != nullptr && !FEngIsScriptSet(objectPtr, FEHASH_LEAVE) && !FEngIsScriptSet(objectPtr, FEHASH_INIT)) {
        FEngSetScript(objectPtr, FEHASH_LEAVE, true);
    }

    objectPtr = FEngFindObject(GetPackageName(), 0xA206A0B4);
    if (objectPtr != nullptr && !FEngIsScriptSet(objectPtr, FEHASH_LEAVE) && !FEngIsScriptSet(objectPtr, FEHASH_INIT)) {
        FEngSetScript(objectPtr, FEHASH_LEAVE, true);
    }

    objectPtr = FEngFindObject(GetPackageName(), 0x7180B901);
    if (objectPtr != nullptr && !FEngIsScriptSet(objectPtr, FEHASH_LEAVE) && !FEngIsScriptSet(objectPtr, FEHASH_INIT)) {
        FEngSetScript(objectPtr, FEHASH_LEAVE, true);
    }

    if (FEngIsScriptSet(reinterpret_cast<FEObject *>(mEngageMechanic), FEHASH_APPEAR)) {
        FEngSetScript(reinterpret_cast<FEObject *>(mEngageMechanic), FEHASH_LEAVE, true);
    }
}

void MenuZoneTrigger::PulseDPadButton(ENGAGE_DPAD_ELEMENT_DIRECTION direction, FEObject *iconToShow) {
    HideDPadButton();

    if (iconToShow != nullptr && !FEngIsScriptSet(iconToShow, FEHASH_APPEAR) && !FEngIsScriptSet(iconToShow, FEHASH_BLINK)) {
        FEngSetScript(iconToShow, FEHASH_APPEAR, true);
    }

    if (direction != ENGAGE_DPAD_ELEMENT_NONE) {
        unsigned int objectHash = 0;
        switch (direction) {
        case ENGAGE_DPAD_ELEMENT_UP:
            objectHash = 0x0A729B1B;
            break;
        case ENGAGE_DPAD_ELEMENT_DOWN:
            objectHash = 0x717C82AE;
            break;
        case ENGAGE_DPAD_ELEMENT_RIGHT:
            objectHash = 0xA206A0B4;
            break;
        case ENGAGE_DPAD_ELEMENT_LEFT:
            objectHash = 0x7180B901;
            break;
        default:
            break;
        }

        FEObject *objectPtr = FEngFindObject(GetPackageName(), objectHash);
        if (objectPtr != nullptr && !FEngIsScriptSet(objectPtr, FEHASH_APPEAR)) {
            FEngSetScript(objectPtr, FEHASH_APPEAR, true);
            g_pEAXSound->PlayUISoundFX(UISND_ENTER_TRIGGER);
        }
    } else if (!FEngIsScriptSet(reinterpret_cast<FEObject *>(mEngageMechanic), FEHASH_APPEAR)) {
        FEngSetScript(reinterpret_cast<FEObject *>(mEngageMechanic), FEHASH_APPEAR, true);
        g_pEAXSound->PlayUISoundFX(UISND_ENTER_TRIGGER);
    }
}
#endif
