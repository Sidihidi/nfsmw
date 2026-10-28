#ifndef INPUT_PC_INPUTDEVICEPC_H
#define INPUT_PC_INPUTDEVICEPC_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// La configuracion de controles de PC (speed.exe 1.3): la tabla de controles y lo que el GameDevice de PC ofrece a la
// pantalla de opciones (UIOptionsPCController) y al perfil. Ver InputDevicePC.cpp.
#include "Speed/Indep/Src/Input/InputDevice.h"
#include "Speed/Indep/Src/Input/IFeedBack.h"
#include "Speed/Indep/Src/Input/SteeringWheelDevice.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/input.h"

// Una asignacion de un control: tipo 1 = tecla (codigo DIK de DirectInput), 2 = boton del mando (indice), 3 = la cruceta
// (indice: 0 arriba, 1 derecha, 2 abajo, 3 izquierda), 4-7 = los ejes del mando (indice).
struct GameActionMapping {
    int Type;  // offset 0x0, size 0x4
    int Code;  // offset 0x4, size 0x4
    int Index; // offset 0x8, size 0x4
};

// Cada control: su nombre (el de la coleccion de controles) y cuatro asignaciones: teclado principal y secundaria, mando
// principal y secundaria.
// total size: 0x34
struct GameActionInfo {
    const char *Name;               // offset 0x0, size 0x4
    GameActionMapping Mappings[4];  // offset 0x4, size 0x30
};

enum {
    NUM_GAME_ACTIONS = 76,
    // los que la pantalla de opciones deja cambiar y el perfil guarda: los 19 primeros (de GAS a HUDACTION_PAUSEREQUEST)
    NUM_CONFIGURABLE_GAME_ACTIONS = 19,
};

extern GameActionInfo GameActionDefaults[NUM_GAME_ACTIONS];
extern GameActionInfo GameAction[NUM_GAME_ACTIONS];

unsigned int GetGameActionNameHash(int action);
// El texto de una celda de la pantalla de opciones (0x628230): "---" si la ranura esta vacia; si no, el nombre que da
// DirectInput para esa tecla o boton. Nombre nuestro.
void GetGameActionMappingName(char *buf, int device_type, int action, bool primary);

// El GameDevice de PC entero. La disposicion es la del constructor de PC (0x634B50): las mismas tablas que el GameDevice
// de GameCube y PS2, con 256 controles, y el volante al final. Las vtables: 0x8A7B9C (InputDevice) y 0x8A7BD4
// (IFeedback). Las virtuales de IFeedback de PC estan vacias salvo PauseEffects (0x628800, para los efectos del mando
// en el gestor de force feedback) y ResetEffects (0x628880, llama a PauseEffects); StartVibration/StopVibration
// tambien estan vacias (0x6287E0/0x6287F0).
// total size: 0x1850
class GameDevice : public InputDevice, public IFeedback {
    DeviceScalar fDeviceScalars[256]; // offset 0x34, size 0x1000
    float fPrevValues[256];           // offset 0x1034, size 0x400
    float fCurrentValues[256];        // offset 0x1434, size 0x400
    int mNumScalars;                  // offset 0x1834, size 0x4
    int mUnknown1838;                 // offset 0x1838, size 0x4 (lo que devuelve GetNumDeviceScalar, 0x628BF0)
    // la asignacion que la pantalla de opciones espera (BeginRemap): la siguiente tecla o boton va a esa ranura
    bool mRemapping;                  // offset 0x183C, size 0x1
    int mRemapDeviceType;             // offset 0x1840, size 0x4
    int mRemapSlot;                   // offset 0x1844, size 0x4
    int mRemapAction;                 // offset 0x1848, size 0x4
    SteeringWheelDevice *mWheelDevice; // offset 0x184C, size 0x4 (el constructor lo crea para su puerto)

  public:
    GameDevice(int deviceIndex);      // 0x634B50
    static InputDevice *Construct(int port);
    virtual ~GameDevice();            // 0x634D00
    // InputDevice
    virtual void Initialize();        // 0x63F850
    virtual bool IsConnected();       // 0x6287D0
    virtual void PollDevice();        // 0x6349B0
    virtual int GetNumDeviceScalar(); // 0x628BF0
    virtual void StartVibration();    // 0x6287E0
    virtual void StopVibration();     // 0x6287F0
    virtual UTL::COM::IUnknown *GetInterfaces(); // 0x634CB0
    // 0x634CC0 y 0x634CE0 (en linea, como en GameCube: el exe las saca junto al constructor)
    virtual bool IsWheel() {
        if (this->mWheelDevice != 0) {
            return this->mWheelDevice->IsConnected();
        }
        return false;
    }
    virtual UTL::COM::IUnknown *GetSecondaryDevice() {
        return this->mWheelDevice;
    }
    // IFeedback
    virtual void PauseEffects();      // 0x628800
    virtual void ResumeEffects();
    virtual void ResetEffects();      // 0x628880
    virtual void BeginUpdate();
    virtual void EndUpdate();
    virtual void UpdateRoadNoise(bool front, const struct SimSurface &surface, float speed);
    virtual void UpdateTireSkid(bool front, const struct SimSurface &surface, float speed);
    virtual void UpdateTireSlip(bool front, const struct SimSurface &surface, float speed);
    virtual void UpdateRPM(float powerband, float overrev, float throttle);
    virtual void UpdateShiftPotential(ShiftPotential potential);
    virtual void UpdateNOS(bool engaged, float NOSLevel);
    virtual void UpdateEngineBlown(bool blown);
    virtual void UpdateShifting(bool shifting);
    virtual void ReportCollision(const COLLISION_INFO &cinfo, bool iamA);

    // Lo que usan la pantalla de opciones (UIOptionsPCController) y el perfil.
    void RestoreKeyboardDefaults();
    void ClearMapping(int device_type, int action, bool primary);
    void BeginRemap(int device_type, int action, bool primary);
    bool IsRemapping();
    void SetMapping(RealInput::Event *event, GameActionMapping *mapping);
    bool RemapPadEvent(RealInput::Event *event);
    void ProcessRemapEvents();
    GameActionMapping *GetMappings(int action);
    int GetMappingsSize();
    // Lo que usa la pantalla de opciones y no esta escrito: leen los dispositivos de DirectInput. Nombres nuestros.
    bool HasJoystick();                        // 0x628C50: hay mando
    const char *GetDeviceName(int device_type); // 0x628C60: el nombre del teclado (1) o del mando (3)
    void RestoreJoystickDefaults();            // 0x63CD80: las de fabrica segun el modelo de mando
    // 0x634DA0: con 3 (el mando) o 1 (el teclado), el bStringHashUpper del nombre del dispositivo; el perfil lo guarda
    // con los mapeos (pc_fedb_56E180). Nombre neutro: ningun binario lo da.
    unsigned int GetWin32Unknown634DA0(int device_type);

    static int mCount; // 0x9203C0: el primero crea las tablas de controles (el constructor)
};

#endif
