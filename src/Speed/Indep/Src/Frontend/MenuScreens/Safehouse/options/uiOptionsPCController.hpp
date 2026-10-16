#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_OPTIONS_UIOPTIONSPCCONTROLLER_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_OPTIONS_UIOPTIONSPCCONTROLLER_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// La pantalla de controles de PC (Options_PC_Controller.fng y Pause_PC_Controller.fng; speed.exe 1.3): una tabla de
// 2 columnas (asignacion principal y secundaria) por 8 filas visibles con los controles configurables del GameDevice de
// PC (Input/PC/InputDevicePC.h) y dos pestanas: teclado (tipo 1) y mando (tipo 3). Solo existe en PC: GameCube y PS2
// usan UIOptionsController. El nombre de la pantalla viene de la tabla de simbolos de PC (Setup__21UIOptionsPCController);
// los de la celda y del dato, de las cadenas de su `new` (las limpiezas de excepcion del exe: "KeyConfigSlot",
// "KeyConfigDatum"); los de los metodos y los campos son nuestros.

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feArrayScrollerMenu.hpp"
#include "Speed/Indep/Src/Input/IOModule.h"
#include "Speed/Indep/Src/Input/PC/InputDevicePC.h"

// Una celda: el recuadro (para el raton), el nombre del control (el mismo en las dos columnas) y el texto de la
// asignacion. vtable 0x89C23C.
// total size: 0x1C
struct KeyConfigSlot : public ArraySlot {
    FEString *pActionName; // offset 0x14, size 0x4
    FEString *pMapping;    // offset 0x18, size 0x4

    KeyConfigSlot(FEObject *box, FEString *action_name, FEString *mapping)
        : ArraySlot(box) //
        , pActionName(action_name) //
        , pMapping(mapping) {}
    ~KeyConfigSlot() override {}
    void Update(ArrayDatum *datum, bool isSelected) override;
};

// El dato de una celda: el texto de la asignacion (el nombre que da DirectInput, o "---") y el nombre del control.
// vtable 0x89C244.
// total size: 0x120
struct KeyConfigDatum : public ArrayDatum {
    char MappingName[260];       // offset 0x18, size 0x104
    unsigned int ActionNameHash; // offset 0x11C, size 0x4

    KeyConfigDatum(unsigned int action_name_hash) : ArrayDatum(0, 0) {
        ActionNameHash = action_name_hash;
        bStrNCpy(MappingName, "", 260);
    }
    ~KeyConfigDatum() override {}
    void NotificationMessage(u32 msg, FEObject *pObj, u32 param1, u32 param2) override {}
};

// total size: 0x2B4
class UIOptionsPCController : public ArrayScrollerMenu {
  public:
    UIOptionsPCController(ScreenConstructorData *sd);
    ~UIOptionsPCController() override {}

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;
    void RefreshHeader() override;

  private:
    // las filas de la pestana: en la de teclado la ultima (HUDACTION_PAUSEREQUEST, Escape) no se cambia
    int GetNumActions() {
        return DeviceType == 1 ? NUM_CONFIGURABLE_GAME_ACTIONS - 1 : NUM_CONFIGURABLE_GAME_ACTIONS;
    }

    static GameDevice *GetGameDevice() {
        return static_cast<GameDevice *>(IOModule::GetIOModule().GetDevice(0));
    }

    void Setup();
    void SetupMappings();
    void RefreshMappings();
    void SwitchDevice();
    void ToggleRemap();
    bool OptionsDidNotChange();
    void RestoreOriginals();
    void ClearCurrentMapping();
    void RestoreDefaults();

    bool mCalledFromPauseMenu;                                         // offset 0xE0, size 0x1
    bool mWaitingForRemap;                                             // offset 0xE1, size 0x1
    int DeviceType;                                                    // offset 0xE4, size 0x4
    GameActionMapping OriginalMappings[NUM_CONFIGURABLE_GAME_ACTIONS][2]; // offset 0xE8, size 0x1C8
    bool OriginalRumble;                                               // offset 0x2B0, size 0x1
};

#endif
