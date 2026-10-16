// El GameDevice de PC (speed.exe 1.3): la configuracion de controles. Solo la parte que la pantalla de opciones de PC
// (UIOptionsPCController) y el perfil (UserProfile::SaveToBuffer/LoadFromBuffer) usan; el sondeo de DirectInput
// (PollDevice, 0x6349B0), la captura del mando (0x634490) y las configuraciones de fabrica de cada modelo de mando
// (0x63CD80) no estan escritos.
// El nombre del fichero y de la clase son los de GameCube y PS2 (Input/GC/InputDeviceGC.cpp, Input/PS2/InputDevicePS2.cpp,
// `class GameDevice : public InputDevice, public IFeedback`): en PC el constructor (0x634B50) pone las mismas dos bases.
#include "Speed/Indep/Src/Input/PC/InputDevicePC.h"
#include "Speed/Indep/Src/Input/IOModule.h"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

// el interfaz de RealInput (los eventos de DirectInput en PC; en GameCube, InputDeviceGC.cpp)
RealInput::Interface *inputsys;

// La de fabrica (0x8F6D80). Los codigos de tecla son los DIK_ de dinput.h.
GameActionInfo GameActionDefaults[NUM_GAME_ACTIONS] = {
    {"GAME_ACTION_GAS", {{1, 0xC8, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_UP
    {"GAME_ACTION_BRAKE", {{1, 0xD0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_DOWN
    {"GAME_ACTION_STEERLEFT", {{1, 0xCB, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_LEFT
    {"GAME_ACTION_STEERRIGHT", {{1, 0xCD, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_RIGHT
    {"GAME_ACTION_HANDBRAKE", {{1, 0x39, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_SPACE
    {"GAME_ACTION_GAMEBREAKER", {{1, 0x9D, 0}, {1, 0x22, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_RCONTROL DIK_G
    {"GAME_ACTION_NOS", {{1, 0x38, 0}, {1, 0x2D, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_LMENU DIK_X
    {"GAME_ACTION_SHIFTDOWN", {{1, 0x1D, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_LCONTROL
    {"GAME_ACTION_SHIFTUP", {{1, 0x2A, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_LSHIFT
    {"GAME_ACTION_RESET", {{1, 0x13, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_R
    {"HUDACTION_ENGAGE_EVENT", {{1, 0x1C, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_RETURN
    {"HUDACTION_PAD_DOWN", {{1, 0x30, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_B
    {"HUDACTION_PAD_LEFT", {{1, 0x32, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_M
    {"HUDACTION_PAD_RIGHT", {{1, 0x0F, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_TAB
    {"FE_ACTION_B2", {{1, 0x14, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_T
    {"CAM_ACTION_CHANGE", {{1, 0x2E, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_C
    {"CAM_ACTION_LOOKBACK", {{1, 0x26, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_L
    {"CAM_ACTION_PULLBACK", {{1, 0x19, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_P
    {"HUDACTION_PAUSEREQUEST", {{1, 0x01, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_ESCAPE
    {"FE_ACTION_LEFT", {{1, 0xCB, 0}, {0, 0, 0}, {3, 0, 3}, {0, 0, 0}}}, // DIK_LEFT
    {"FE_ACTION_RIGHT", {{1, 0xCD, 0}, {0, 0, 0}, {3, 0, 1}, {0, 0, 0}}}, // DIK_RIGHT
    {"FE_ACTION_UP", {{1, 0xC8, 0}, {0, 0, 0}, {3, 0, 0}, {0, 0, 0}}}, // DIK_UP
    {"FE_ACTION_DOWN", {{1, 0xD0, 0}, {0, 0, 0}, {3, 0, 2}, {0, 0, 0}}}, // DIK_DOWN
    {"HUDACTION_SKIPNIS", {{1, 0x1C, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_RETURN
    {"HUDACTION_NEXTSONG", {{1, 0x14, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_T
    {"SC_W", {{1, 0x11, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_W
    {"SC_A", {{1, 0x1E, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_A
    {"SC_S", {{1, 0x1F, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_S
    {"SC_D", {{1, 0x20, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_D
    {"FE_ACTION_RTRIGGER", {{1, 0x3B, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_F1
    {"FE_ACTION_LTRIGGER", {{1, 0x3C, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_F2
    {"SC_BACKSLASH", {{1, 0x2B, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_BACKSLASH
    {"FE_ACTION_ACCEPT", {{1, 0x1C, 0}, {0, 0, 0}, {2, 0, 1}, {0, 0, 0}}}, // DIK_RETURN
    {"FE_ACTION_CANCEL", {{1, 0x01, 0}, {0, 0, 0}, {2, 0, 2}, {0, 0, 0}}}, // DIK_ESCAPE
    {"FE_ACTION_START", {{1, 0x39, 0}, {0, 0, 0}, {2, 0, 9}, {0, 0, 0}}}, // DIK_SPACE
    {"FE_ACTION_LTRIG", {{1, 0x0A, 0}, {0, 0, 0}, {2, 0, 4}, {0, 0, 0}}}, // DIK_9
    {"FE_ACTION_RTRIG", {{1, 0x0B, 0}, {0, 0, 0}, {2, 0, 5}, {0, 0, 0}}}, // DIK_0
    {"FE_ACTION_B0", {{1, 0x04, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_3
    {"FE_ACTION_B1", {{1, 0x32, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_M
    {"FE_ACTION_B3", {{1, 0x05, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_4
    {"FE_ACTION_B4", {{1, 0x03, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_2
    {"FE_ACTION_B5", {{1, 0x02, 0}, {0, 0, 0}, {2, 0, 0}, {0, 0, 0}}}, // DIK_1
    {"DEBUG_ACTION_TOGGLECAM", {{1, 0x18, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_O
    {"DEBUG_ACTION_TOGGLECARCOLOR", {{1, 0x17, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_I
    {"SC_Z", {{1, 0x2C, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_Z
    {"SC_X", {{1, 0x2D, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_X
    {"SC_C", {{1, 0x2E, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_C
    {"SC_V", {{1, 0x2F, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_V
    {"SC_F1", {{1, 0x3B, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_F1
    {"SC_MINUS", {{1, 0x0C, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_MINUS
    {"SC_EQUALS", {{1, 0x0D, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_EQUALS
    {"SC_LBRACKET", {{1, 0x1A, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_LBRACKET
    {"SC_RBRACKET", {{1, 0x1B, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_RBRACKET
    {"SC_LALT", {{1, 0x38, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_LMENU
    {"SC_RALT", {{1, 0xB8, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_RMENU
    {"SC_NUMPADENTER", {{1, 0x9C, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_NUMPADENTER
    {"SC_PAGEUP", {{1, 0xC9, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_PRIOR
    {"SC_PAGEDOWN", {{1, 0xD1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_NEXT
    {"SC_RETURN", {{1, 0x1C, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_RETURN
    {"SC_ESCAPE", {{1, 0x01, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_ESCAPE
    {"SC_SPACE", {{1, 0x39, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_SPACE
    {"SC_INSERT", {{1, 0xD2, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_INSERT
    {"SC_BACK", {{1, 0x0E, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_BACK
    {"SC_NUMPAD1", {{1, 0x4F, 0}, {0, 0, 0}, {2, 0, 6}, {0, 0, 0}}}, // DIK_NUMPAD1
    {"SC_NUMPAD2", {{1, 0x50, 0}, {0, 0, 0}, {5, 0, 1}, {0, 0, 0}}}, // DIK_NUMPAD2
    {"SC_NUMPAD3", {{1, 0x51, 0}, {0, 0, 0}, {7, 0, 2}, {0, 0, 0}}}, // DIK_NUMPAD3
    {"SC_NUMPAD4", {{1, 0x4B, 0}, {0, 0, 0}, {4, 0, 0}, {0, 0, 0}}}, // DIK_NUMPAD4
    {"SC_NUMPAD5", {{1, 0x4C, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_NUMPAD5
    {"SC_NUMPAD6", {{1, 0x4D, 0}, {0, 0, 0}, {5, 0, 0}, {0, 0, 0}}}, // DIK_NUMPAD6
    {"SC_NUMPAD7", {{1, 0x47, 0}, {0, 0, 0}, {2, 0, 4}, {0, 0, 0}}}, // DIK_NUMPAD7
    {"SC_NUMPAD8", {{1, 0x48, 0}, {0, 0, 0}, {4, 0, 1}, {0, 0, 0}}}, // DIK_NUMPAD8
    {"SC_NUMPAD9", {{1, 0x49, 0}, {0, 0, 0}, {6, 0, 2}, {0, 0, 0}}}, // DIK_NUMPAD9
    {"SC_NUMPAD0", {{1, 0x52, 0}, {0, 0, 0}, {4, 0, 2}, {0, 0, 0}}}, // DIK_NUMPAD0
    {"SC_DECIMAL", {{1, 0x53, 0}, {0, 0, 0}, {5, 0, 2}, {0, 0, 0}}}, // DIK_DECIMAL
    {"SC_SUBTRACT", {{1, 0x4A, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_SUBTRACT
    {"SC_ADD", {{1, 0x4E, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}}, // DIK_ADD
};

// La viva (0x91F418): la copia de la de fabrica que el GameDevice lee y la pantalla de opciones cambia.
GameActionInfo GameAction[NUM_GAME_ACTIONS];

unsigned int GetGameActionNameHash(int action) {
    return bStringHash(GameAction[action].Name);
}

// Las asignaciones de teclado de fabrica (las del mando no se tocan).
void GameDevice::RestoreKeyboardDefaults() {
    for (int i = 0; i < NUM_CONFIGURABLE_GAME_ACTIONS; i++) {
        GameAction[i].Mappings[0] = GameActionDefaults[i].Mappings[0];
        GameAction[i].Mappings[1] = GameActionDefaults[i].Mappings[1];
    }
}

// Borra una asignacion (teclado: ranuras 0 y 1; si no, 2 y 3). Si se borra la principal, la secundaria
// pasa a principal.
void GameDevice::ClearMapping(int device_type, int action, bool primary) {
    int slot;
    if (device_type == RealInput::Device::TYPE_KEYBOARD) {
        slot = !primary;
    } else {
        slot = primary ? 2 : 3;
    }
    GameActionMapping *mapping = &GameAction[action].Mappings[slot];
    if (slot == 0 || slot == 2) {
        mapping[0].Code = mapping[1].Code;
        mapping[0].Index = mapping[1].Index;
        mapping[0].Type = mapping[1].Type;
        mapping[1].Code = 0;
        mapping[1].Index = 0;
        mapping[1].Type = 0;
    } else {
        mapping->Code = 0;
        mapping->Index = 0;
        mapping->Type = 0;
    }
}

// La pantalla de opciones pide la siguiente tecla (teclado) o boton (mando) para una ranura de un control.
void GameDevice::BeginRemap(int device_type, int action, bool primary) {
    mRemapping = true;
    mRemapDeviceType = device_type;
    mRemapAction = action;
    if (device_type == RealInput::Device::TYPE_KEYBOARD) {
        mRemapSlot = !primary;
    } else if (device_type == RealInput::Device::TYPE_PAD) {
        mRemapSlot = primary ? 2 : 3;
    }
}

bool GameDevice::IsRemapping() {
    return mRemapping;
}

// La asignacion que llega para la ranura que se espera: si la principal esta vacia va a la principal, y si ya esta en
// la otra ranura del mismo control, se cambia de sitio; despues se quita de los demas controles configurables.
void GameDevice::SetMapping(RealInput::Event *event, GameActionMapping *mapping) {
    GameActionInfo *info = &GameAction[mRemapAction];
    int device_type = event->mpDevice->GetInfo()->mType;
    int primary = device_type == RealInput::Device::TYPE_PAD ? 2 : 0;
    int secondary = device_type == RealInput::Device::TYPE_PAD ? 3 : 1;

    if (mRemapSlot == secondary && info->Mappings[primary].Type == 0) {
        info->Mappings[primary] = *mapping;
    } else if (mRemapSlot == secondary && bMemCmp(&info->Mappings[primary], mapping, sizeof(*mapping)) == 0) {
        info->Mappings[secondary].Type = 0;
        info->Mappings[secondary].Code = 0;
        info->Mappings[secondary].Index = 0;
    } else if (mRemapSlot == primary && bMemCmp(&info->Mappings[secondary], mapping, sizeof(*mapping)) == 0) {
        info->Mappings[primary] = *mapping;
        info->Mappings[secondary].Type = 0;
        info->Mappings[secondary].Code = 0;
        info->Mappings[secondary].Index = 0;
    } else {
        info->Mappings[mRemapSlot] = *mapping;
    }
    for (int i = 0; i < NUM_CONFIGURABLE_GAME_ACTIONS; i++) {
        GameActionInfo *other = &GameAction[i];
        if (i != mRemapAction) {
            if (bMemCmp(&other->Mappings[primary], mapping, sizeof(*mapping)) == 0) {
                other->Mappings[primary] = other->Mappings[secondary];
                other->Mappings[secondary].Type = 0;
                other->Mappings[secondary].Code = 0;
                other->Mappings[secondary].Index = 0;
            } else if (bMemCmp(&other->Mappings[secondary], mapping, sizeof(*mapping)) == 0) {
                other->Mappings[secondary].Type = 0;
                other->Mappings[secondary].Code = 0;
                other->Mappings[secondary].Index = 0;
            }
        }
    }
}

// Mientras la pantalla de opciones espera una asignacion, PollDevice (0x6349B0) no lee los controles: vacia la cola de
// eventos de RealInput y la siguiente tecla (teclado) o boton, cruceta o eje (mando) es la nueva asignacion. Escape
// cancela.
void GameDevice::ProcessRemapEvents() {
    inputsys->Update();
    for (RealInput::Event *event = inputsys->GetEvent(); event != nullptr; event = inputsys->GetEvent()) {
        int device_type = event->mpDevice->GetInfo()->mType;
        if (device_type == RealInput::Device::TYPE_KEYBOARD) {
            if (event->mObjectIndex == 1) { // DIK_ESCAPE
                mRemapping = false;
            } else if (mRemapDeviceType == RealInput::Device::TYPE_KEYBOARD && event->mType == RealInput::TYPE_PRESSED) {
                GameActionMapping mapping;
                mapping.Code = event->mObjectIndex;
                mapping.Type = 1;
                mapping.Index = 0;
                SetMapping(event, &mapping);
                mRemapping = false;
            }
        } else if (device_type == RealInput::Device::TYPE_PAD && mRemapDeviceType == RealInput::Device::TYPE_PAD &&
                   (event->mType == RealInput::TYPE_PRESSED || event->mType == RealInput::TYPE_CHANGE)) {
            if (RemapPadEvent(event)) {
                mRemapping = false;
            }
        }
    }
}

GameActionMapping *GameDevice::GetMappings(int action) {
    return GameAction[action].Mappings;
}

int GameDevice::GetMappingsSize() {
    return sizeof(GameAction[0].Mappings);
}
