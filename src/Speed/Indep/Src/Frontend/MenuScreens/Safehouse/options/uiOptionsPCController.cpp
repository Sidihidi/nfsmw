// La pantalla de controles de PC (speed.exe 1.3). Ver uiOptionsPCController.hpp; el mapa de la pantalla y del remapeo,
// en el GameDevice de PC (Input/PC/InputDevicePC.cpp).
#include "Speed/Indep/Src/Input/IOModule.h"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsPCController.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Generated/Events/EUnPause.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"

void KeyConfigSlot::Update(ArrayDatum *datum, bool isSelected) {
    ArraySlot::Update(datum, isSelected);
    if (datum != nullptr) {
        KeyConfigDatum *d = static_cast<KeyConfigDatum *>(datum);
        FEngSetLanguageHash(pActionName, d->ActionNameHash);
        FEPrintf(pMapping, "%s", d->MappingName);
    }
}

UIOptionsPCController::UIOptionsPCController(ScreenConstructorData *sd)
    : ArrayScrollerMenu(sd, 2, 8, true) //
    , mCalledFromPauseMenu(sd->Arg != 0) //
    , mWaitingForRemap(false) //
    , DeviceType(1) {
    SetClickToSelectMode(true);
    SetMouseDownMsg(0x936A6A7F);
    Setup();
    SetupMappings();
    RefreshHeader();
}

void UIOptionsPCController::Setup() {
    for (int i = 0; i < GetHeight(); i++) {
        FEString *action_name = FEngFindString(GetPackageName(), FEngHashString("KEY_TEXT_%d", i + 1));
        FEObject *box = FEngFindObject(GetPackageName(), FEngHashString("KEY_BOX_P%d", i + 1));
        FEString *mapping = FEngFindString(GetPackageName(), FEngHashString("KEY_TEXT_P%d", i + 1));
        AddSlot(new ("KeyConfigSlot", 0) KeyConfigSlot(box, action_name, mapping));
        box = FEngFindObject(GetPackageName(), FEngHashString("KEY_BOX_S%d", i + 1));
        mapping = FEngFindString(GetPackageName(), FEngHashString("KEY_TEXT_S%d", i + 1));
        AddSlot(new ("KeyConfigSlot", 0) KeyConfigSlot(box, action_name, mapping));
    }
}

// Rellena la tabla con las asignaciones de la pestana y guarda una copia para poder cancelar.
void UIOptionsPCController::SetupMappings() {
    OriginalRumble = FEDatabase->GetPlayerSettings(0)->Rumble;
    ClearData();
    int slot = DeviceType == 1 ? 0 : 2;
    GameDevice *device = GetGameDevice();
    if (DeviceType == 1 || DeviceType == 3) {
        for (int i = 0; i < GetNumActions(); i++) {
            OriginalMappings[i][0] = device->GetMappings(i)[slot];
            OriginalMappings[i][1] = device->GetMappings(i)[slot + 1];
            KeyConfigDatum *datum = new ("KeyConfigDatum", 0) KeyConfigDatum(GetGameActionNameHash(i));
            GetGameActionMappingName(datum->MappingName, DeviceType, i, true);
            AddDatum(datum);
            datum = new ("KeyConfigDatum", 0) KeyConfigDatum(GetGameActionNameHash(i));
            GetGameActionMappingName(datum->MappingName, DeviceType, i, false);
            AddDatum(datum);
        }
    }
    if (DeviceType == 1) {
        FEngSetInvisible(FEngFindObject(GetPackageName(), 0x9CCF9099));
    } else {
        FEngSetVisible(FEngFindObject(GetPackageName(), 0x9CCF9099));
    }
    SetInitialPosition(0);
    RefreshHeader();
}

void UIOptionsPCController::RefreshMappings() {
    if (DeviceType == 1 || DeviceType == 3) {
        for (int i = 0; i < GetNumActions(); i++) {
            KeyConfigDatum *datum = static_cast<KeyConfigDatum *>(GetDatumAt(GetWidth() * i));
            GetGameActionMappingName(datum->MappingName, DeviceType, i, true);
            datum = static_cast<KeyConfigDatum *>(GetDatumAt(GetWidth() * i + 1));
            GetGameActionMappingName(datum->MappingName, DeviceType, i, false);
        }
    }
    RefreshHeader();
}

void UIOptionsPCController::RefreshHeader() {
    ArrayScrollerMenu::RefreshHeader();
    GameDevice *device = GetGameDevice();
    if (!device->HasJoystick()) {
        FEngSetScript(GetPackageName(), 0x8A41F5B9, 0x0016A259, true);
    } else {
        const char *rumble;
        if (FEDatabase->GetPlayerSettings(0)->Rumble) {
            rumble = GetLocalizedString(LANGUAGE_COMMON_ON);
        } else {
            rumble = GetLocalizedString(LANGUAGE_COMMON_OFF);
        }
        FEPrintf(GetPackageName(), 0x52CC164A, GetLocalizedString(0x3DF9961B), rumble);
    }
    FEPrintf(GetPackageName(), 0x4E11CF90, "%s", device->GetDeviceName(DeviceType));
}

// Cambia de pestana (solo hay la del mando si hay mando).
void UIOptionsPCController::SwitchDevice() {
    GameDevice *device = GetGameDevice();
    int device_type = DeviceType == 1 && device->HasJoystick() ? 3 : 1;
    if (DeviceType != device_type) {
        DeviceType = device_type;
        SetupMappings();
        RefreshHeader();
    }
}

// Aceptar en una celda: espera la siguiente tecla o boton (GameDevice::ProcessRemapEvents); la segunda vez, o cuando el
// GameDevice ya la tiene, vuelve a la tabla.
void UIOptionsPCController::ToggleRemap() {
    int action = GetCurrentDatumNum() / GetWidth();
    bool primary = GetCurrentDatumNum() % GetWidth() == 0;
    if (!mWaitingForRemap) {
        mWaitingForRemap = true;
        GetGameDevice()->BeginRemap(DeviceType, action, primary);
        FEngSetLanguageHash(GetPackageName(), 0xB2D837B0, GetGameActionNameHash(action));
        FEPrintf(GetPackageName(), 0x3BBE5E51, "%s", static_cast<KeyConfigDatum *>(GetCurrentDatum())->MappingName);
        cFEng::Get()->QueuePackageMessage(0xD707AE8D, nullptr, nullptr);
        cFEng::Get()->QueuePackageMessage(0x13FD3296, nullptr, nullptr);
    } else {
        mWaitingForRemap = false;
        RefreshMappings();
        RefreshHeader();
        cFEng::Get()->QueuePackageMessage(0x9B2BA546, nullptr, nullptr);
        cFEng::Get()->QueuePackageMessage(0x8CB81F09, nullptr, nullptr);
    }
}

bool UIOptionsPCController::OptionsDidNotChange() {
    int slot = DeviceType == 1 ? 0 : 2;
    GameDevice *device = GetGameDevice();
    for (int i = 0; i < GetNumActions(); i++) {
        if (memcmp(&OriginalMappings[i][0], &device->GetMappings(i)[slot], sizeof(GameActionMapping)) != 0) {
            return false;
        }
        if (memcmp(&OriginalMappings[i][1], &device->GetMappings(i)[slot + 1], sizeof(GameActionMapping)) != 0) {
            return false;
        }
    }
    return FEDatabase->GetPlayerSettings(0)->Rumble == OriginalRumble;
}

void UIOptionsPCController::RestoreOriginals() {
    int slot = DeviceType == 1 ? 0 : 2;
    GameDevice *device = GetGameDevice();
    for (int i = 0; i < GetNumActions(); i++) {
        device->GetMappings(i)[slot] = OriginalMappings[i][0];
        device->GetMappings(i)[slot + 1] = OriginalMappings[i][1];
    }
    FEDatabase->GetPlayerSettings(0)->Rumble = OriginalRumble;
    RefreshMappings();
}

void UIOptionsPCController::ClearCurrentMapping() {
    GameDevice *device = GetGameDevice();
    int action = GetCurrentDatumNum() / GetWidth();
    bool primary = GetCurrentDatumNum() % GetWidth() == 0;
    device->ClearMapping(DeviceType, action, primary);
    RefreshMappings();
}

void UIOptionsPCController::RestoreDefaults() {
    GameDevice *device = GetGameDevice();
    if (device->HasJoystick() && DeviceType == 3) {
        device->RestoreJoystickDefaults();
    } else {
        device->RestoreKeyboardDefaults();
    }
    FEDatabase->GetPlayerSettings(0)->Rumble = true;
    RefreshMappings();
}

void UIOptionsPCController::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    ArrayScrollerMenu::NotificationMessage(msg, pobj, param1, param2);
    switch (msg) {
    // la vibracion (solo en la pestana del mando)
    case 0x36D64627:
        if (DeviceType == 3) {
            FEDatabase->GetPlayerSettings(0)->Rumble = !FEDatabase->GetPlayerSettings(0)->Rumble;
            RefreshHeader();
        }
        break;
    case 0x2DCC19C8: {
        GameDevice *device = GetGameDevice();
        ClearCurrentMapping();
        break;
    }
    case 0x06DC9631:
        RestoreOriginals();
        SwitchDevice();
        break;
    case 0xB850B019:
    {
        FEDatabase->SetOptionsDirty(FEDatabase->IsOptionsDirty() || !OptionsDidNotChange());
        SwitchDevice();
        break;
    }
    case 0xD9FEEC59:
    case 0x5073EF13:
        if (GetGameDevice()->HasJoystick()) {
            if (!OptionsDidNotChange()) {
                DialogInterface::ShowTwoButtons(GetPackageName(), mCalledFromPauseMenu ? "InGameDialog.fng" : "Dialog.fng",
                                                static_cast<eDialogTitle>(1), LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO,
                                                0xB850B019, 0x06DC9631, 0x34DC1BCF, static_cast<eDialogFirstButtons>(1),
                                                0xF98FA043);
                return;
            }
            SwitchDevice();
        }
        break;
    case 0x911AB364:
        cFEng::Get()->QueuePackageMessage(0x587C018B, GetPackageName(), nullptr);
        break;
    case 0x936A6A7F:
        ToggleRemap();
        break;
    case 0xC98356BA: {
        GameDevice *device = GetGameDevice();
        if (mWaitingForRemap == true && !device->IsRemapping()) {
            ToggleRemap();
        }
        break;
    }
    case 0xC519BFC4:
        DialogInterface::ShowTwoButtons(GetPackageName(), mCalledFromPauseMenu ? "InGameDialog.fng" : "Dialog.fng",
                                        static_cast<eDialogTitle>(1), LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO, 0xD05FC3A3,
                                        0x34DC1BCF, 0x34DC1BCF, static_cast<eDialogFirstButtons>(1), 0x8AEF5AE8);
        break;
    case 0xD05FC3A3:
        RestoreDefaults();
        break;
    case 0xE1FDE1D1: {
        FEDatabase->SetOptionsDirty(FEDatabase->IsOptionsDirty() || !OptionsDidNotChange());
        if (mCalledFromPauseMenu) {
            cFEng::Get()->QueuePackageSwitch("Pause_Main.fng", 1, 0, false);
        } else if (FEDatabase->IsOnlineMode()) {
            cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
        } else {
            cFEng::Get()->QueuePackageSwitch("MainMenu_Sub.fng", 0, 0, false);
        }
        break;
    }
    case 0xB5AF2461:
        if (mCalledFromPauseMenu) {
            new EUnPause();
        }
        break;
    }
}
