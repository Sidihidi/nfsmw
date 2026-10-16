#include "Speed/Indep/Src/Frontend/HUD/FeAutoSaveIcon.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Generated/Events/EHideAutoSaveEngage.hpp"
#include "Speed/Indep/Src/Generated/Events/EShowAutoSaveEngage.hpp"

// Solo la A124 (zFe2.cpp lo incluye con EA_BUILD_A124): el icono de autoguardado del HUD. Leido en
// orig/SLES-53558-A124/NFS.ELF con el mapa de lineas del .mdebug.
AutoSaveIcon::AutoSaveIcon(UTL::COM::Object *pOutter, const char *pkg_name, int player_number)
    : HudElement(pkg_name, HUD_FEATURE_AUTO_SAVE_ICON), IAutoSaveIcon(pOutter), mShowingIcon(false), mIconRequested(false) {
    mpIcon = RegisterObject(0x3DF00E93);
}

void AutoSaveIcon::Update(IPlayer *player) {
    if (mIconRequested) {
        if (!FEngIsScriptSet(mpIcon, 0x5079C8F8)) {
            FEngSetScript(mpIcon, 0x5079C8F8, true);
            FEManager::Get()->SuppressControllerError(true);
            new EShowAutoSaveEngage();
        }
        mShowingIcon = true;
        mIconRequested = false;
    } else if (mShowingIcon) {
        if (!MemoryCard::GetInstance()->IsAutoSaving()) {
            if (!FEngIsScriptSet(mpIcon, 0x033113AC)) {
                FEngSetScript(mpIcon, 0x033113AC, true);
                FEManager::Get()->SuppressControllerError(false);
                new EHideAutoSaveEngage();
            }
            mShowingIcon = false;
        }
    }
}

void AutoSaveIcon::RequestAutoSaveIcon() {
    mIconRequested = true;
}
