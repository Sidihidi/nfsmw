// El dispositivo de entrada de PS2: el gemelo de Input/GC/InputDeviceGC.cpp. Orden de funciones, nombres de
// parametros y locales, tipos y lineas del .mdebug de la A124 (input/ps2/InputDevicePS2.cpp en zMain, entre
// iomodule.cpp y SteeringWheelDevice.cpp). SLUS (#ifndef EA_BUILD_A124) es una version posterior, del asm de SLUS: vibra
// por mando (input_buzz, como GameCube), calla los efectos con el volante y tiene StopVibration.
#include "Speed/Indep/Src/Input/IOModule.h"
#include "Speed/Indep/Src/Input/IFeedBack.h"
#include "Speed/Indep/Src/Input/SteeringWheelDevice.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/input.h"
#include "Speed/Indep/Src/Input/Common/FFBTypes.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Sim/SimSurface.h"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_MU_Keyboard.hpp"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"

extern FEKeyboard *gFEKeyboard;
void SteeringWheels_StopAllForces();

class GameDevice : public InputDevice, public IFeedback {
  public:
    USE_FASTALLOC(GameDevice)

    GameDevice(int deviceIndex);
    virtual ~GameDevice();

    static InputDevice *Construct(int port) {
        return new GameDevice(port);
    }

    static void InitWheelSupport() {}

    // InputDevice
    virtual void Initialize();
    virtual void PollDevice();
    virtual int GetNumDeviceScalar();

    virtual bool IsConnected();

    virtual bool IsWheel() {
        if (mWheelDevice) {
            return mWheelDevice->IsConnected();
        }
        return false;
    }

    virtual UTL::COM::IUnknown *GetInterfaces() {
        return static_cast<IFeedback *>(this);
    }

    virtual UTL::COM::IUnknown *GetSecondaryDevice() {
        return mWheelDevice;
    }

    // IFeedback
    virtual void PauseEffects();
    virtual void ResumeEffects();
    virtual void ResetEffects();
    virtual void StartVibration();
#ifndef EA_BUILD_A124
    virtual void StopVibration();
#endif
    virtual void BeginUpdate();
    virtual void EndUpdate();
    virtual void UpdateRoadNoise(bool front, const SimSurface &surface, float speed);
    virtual void UpdateTireSkid(bool front, const SimSurface &surface, float speed);
    virtual void UpdateTireSlip(bool front, const SimSurface &surface, float speed);
    virtual void UpdateShiftPotential(ShiftPotential potential);
    virtual void UpdateNOS(bool engaged, float NOSLevel);
    virtual void UpdateEngineBlown(bool blown);
    virtual void UpdateRPM(float powerband, float overrev, float throttle);
    virtual void UpdateShifting(bool shifting);
    virtual void ReportCollision(const COLLISION_INFO &cinfo, bool iamA);

  private:
    RealInput::Effect *mEffect;              // offset 0x50, size 0x4
    DeviceScalar fPS2DeviceScalars[66];      // offset 0x54, size 0x420
    float fPS2PrevValues[66];                // offset 0x474, size 0x108
    float fPS2CurrentValues[66];             // offset 0x57C, size 0x108
    int mNumScalars;                         // offset 0x684, size 0x4
    static int mCount;
    bool mNOSEngaged;                        // offset 0x688, size 0x4
    bool mEngineBlown;                       // offset 0x68C, size 0x4
    bool mShifting;                          // offset 0x690, size 0x4
    ShiftPotential mShiftPotential;          // offset 0x694, size 0x4
    SteeringWheelDevice *mWheelDevice;       // offset 0x698, size 0x4
};

enum eDeviceType {
    kDTNone = 0,
    kDTKeyboard = 1,
    kDTMouse = 2,
    kDTDevice = 3,
};

// total size: 0x1C
struct DeviceScalarInfo {
    DeviceScalarType type;  // offset 0x0, size 0x4
    const char *name;       // offset 0x4, size 0x4
    int system_index;       // offset 0x8, size 0x4
    float ramp_min;         // offset 0xC, size 0x4
    float ramp_max;         // offset 0x10, size 0x4
    eDeviceType devicetype; // offset 0x14, size 0x4
    bool wheel;             // offset 0x18, size 0x4
};

UTL::COM::Factory<int, InputDevice, UCrc32>::Prototype _GameDevice(UCrc32("GameDevice"), GameDevice::Construct);

RealInput::Interface *inputsys = nullptr;
static RealInput::Device *input_devices[RealInput::PD_NUM_PORTS * RealInput::PD_NUM_SLOTS] = {
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};
static RealInput::Effect *input_effects[RealInput::PD_NUM_PORTS * RealInput::PD_NUM_SLOTS] = {
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};
static bool input_connected[RealInput::PD_NUM_PORTS * RealInput::PD_NUM_SLOTS] = {
    false, false, false, false, false, false, false, false,
};
#ifndef EA_BUILD_A124
// SLUS vibra por mando, como GameCube (input_buzz).
static float input_buzz[RealInput::PD_NUM_PORTS * RealInput::PD_NUM_SLOTS] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};
#endif
int GameDevice::mCount = 0;
#ifdef EA_BUILD_A124
static float sVibrationTime = 0.0f;
#endif
static unsigned int pad_ticker = 0;

static RealInput::Device *input_device_keyboard;
static RealInput::Device *input_device_mouse;
static InputEffectState effect_states[RealInput::PD_NUM_PORTS * RealInput::PD_NUM_SLOTS];
static float PS2Keys[30];
float *fTemp = nullptr;

// Los nombres de los escalares (los del teclado son sus codigos de tecla). Centinela con el nombre a NULL.
static const DeviceScalarInfo device_infos[] = {
    {kJoyAxis, "ALX_L", 0, 0.0f, 0.0f, kDTDevice, false},
    {kJoyAxis, "ALX_R", 0, 0.0f, 0.0f, kDTDevice, false},
    {kJoyAxis, "ALY_U", 0, 0.0f, 0.0f, kDTDevice, false},
    {kJoyAxis, "ALY_D", 0, 0.0f, 0.0f, kDTDevice, false},
    {kJoyAxis, "ARX_L", 1, 0.0f, 0.0f, kDTDevice, false},
    {kJoyAxis, "ARX_R", 1, 0.0f, 0.0f, kDTDevice, false},
    {kJoyAxis, "ARY_U", 1, 0.0f, 0.0f, kDTDevice, false},
    {kJoyAxis, "ARY_D", 1, 0.0f, 0.0f, kDTDevice, false},
    {kAnalogButton, "DUp", RealInput::SC_AUP, 0.0f, 0.5f, kDTDevice, false},
    {kAnalogButton, "DDown", RealInput::SC_ADOWN, 0.0f, 0.5f, kDTDevice, false},
    {kAnalogButton, "DLeft", RealInput::SC_ALEFT, 0.0f, 0.5f, kDTDevice, false},
    {kAnalogButton, "DRight", RealInput::SC_ARIGHT, 0.0f, 0.5f, kDTDevice, false},
    {kDigitalButton, "DUp_Digital", RealInput::SC_AUP, 0.0f, 0.0f, kDTDevice, false},
    {kDigitalButton, "DDown_Digital", RealInput::SC_ADOWN, 0.0f, 0.0f, kDTDevice, false},
    {kDigitalButton, "DLeft_Digital", RealInput::SC_ALEFT, 0.0f, 0.0f, kDTDevice, false},
    {kDigitalButton, "DRight_Digital", RealInput::SC_ARIGHT, 0.0f, 0.0f, kDTDevice, false},
    {kDigitalButton, "Select", RealInput::SC_DSELECT, 0.0f, 0.0f, kDTDevice, false},
    {kDigitalButton, "Start", RealInput::SC_DSTART, 0.0f, 0.0f, kDTDevice, false},
    {kAnalogButton, "Triangle", RealInput::SC_ATRI, 0.0f, 0.5f, kDTDevice, false},
    {kAnalogButton, "Cross", RealInput::SC_ACROSS, 0.0f, 0.5f, kDTDevice, false},
    {kAnalogButton, "Square", RealInput::SC_ASQUARE, 0.0f, 0.5f, kDTDevice, false},
    {kAnalogButton, "Circle", RealInput::SC_ACIRCLE, 0.0f, 0.5f, kDTDevice, false},
    {kAnalogButton, "L1", RealInput::SC_AL1, 0.0f, 0.5f, kDTDevice, false},
    {kAnalogButton, "L2", RealInput::SC_AL2, 0.0f, 0.5f, kDTDevice, false},
    {kAnalogButton, "R1", RealInput::SC_AR1, 0.0f, 0.5f, kDTDevice, false},
    {kAnalogButton, "R2", RealInput::SC_AR2, 0.0f, 0.5f, kDTDevice, false},
    {kDigitalButton, "L3", RealInput::SC_DL3, 0.0f, 0.0f, kDTDevice, false},
    {kDigitalButton, "R3", RealInput::SC_DR3, 0.0f, 0.0f, kDTDevice, false},
    {kDigitalButton, "SC_LEFT", 80, 0.0f, 0.0f, kDTKeyboard, false},
    {kDigitalButton, "SC_RIGHT", 79, 0.0f, 0.0f, kDTKeyboard, false},
    {kDigitalButton, "SC_UP", 82, 0.0f, 0.0f, kDTKeyboard, false},
    {kDigitalButton, "SC_DOWN", 81, 0.0f, 0.0f, kDTKeyboard, false},
    {kDigitalButton, "SC_ENTER", 40, 0.0f, 0.0f, kDTKeyboard, false},
    {kDigitalButton, "SC_ESCAPE", 41, 0.0f, 0.0f, kDTKeyboard, false},
    {kDigitalButton, "SC_SPACE", 44, 0.0f, 0.0f, kDTKeyboard, false},
    {kDigitalButton, "SC_B", 5, 0.0f, 0.0f, kDTKeyboard, false},
    {kDigitalButton, "SC_C", 6, 0.0f, 0.0f, kDTKeyboard, false},
    {kDigitalButton, "SC_R", 21, 0.0f, 0.0f, kDTKeyboard, false},
    {kJoyAxis, "W_RIGHT", 0, 0.0f, 0.0f, kDTDevice, true},
    {kJoyAxis, "W_LEFT", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_BRAKE", 10, 0.0f, 0.5f, kDTDevice, true},
    {kAnalogButton, "W_GAS", 11, 0.0f, 0.5f, kDTDevice, true},
    {kAnalogButton, "W_BUTTON0", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_BUTTON1", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_BUTTON5", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_BUTTON4", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_BUTTON3", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_BUTTON2", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_TRIANGLE", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_SQUARE", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_CIRCLE", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_CROSS", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_R2", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_L2", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_SELECT", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_L1", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_R1", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_DLEFT", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_DRIGHT", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_DLEFT_DIGITAL", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_DRIGHT_DIGITAL", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_L3", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_R3", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_DDOWN_DIGITAL", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_DUP_DIGITAL", 0, 0.0f, 0.0f, kDTDevice, true},
    {kAnalogButton, "W_START", 0, 0.0f, 0.0f, kDTDevice, true},
    {kJoyAxis, nullptr, 0, 0.0f, 0.0f, kDTNone, false},
};

void InputEffectState::Run(float ms) {
    if (ms <= 0.0f) {
        return;
    }

    if (!Enabled) {
        return;
    }

    Volume_Left = 0.0f;
    Volume_Right = 0.0f;

    float road_noise_left = RoadNoise[CHANNEL_LEFT_LOW].Run(ms) + RoadNoise[CHANNEL_LEFT_HIGH].Run(ms);
    float road_noise_right = RoadNoise[CHANNEL_RIGHT_LOW].Run(ms) + RoadNoise[CHANNEL_RIGHT_HIGH].Run(ms);

    float skid_noise_left = SkidNoise[CHANNEL_LEFT_LOW].Run(ms) + SkidNoise[CHANNEL_LEFT_HIGH].Run(ms);
    float skid_noise_right = SkidNoise[CHANNEL_RIGHT_LOW].Run(ms) + SkidNoise[CHANNEL_RIGHT_HIGH].Run(ms);

    float slip_noise_left = SlipNoise[CHANNEL_LEFT_LOW].Run(ms) + SlipNoise[CHANNEL_LEFT_HIGH].Run(ms);
    float slip_noise_right = SlipNoise[CHANNEL_RIGHT_LOW].Run(ms) + SlipNoise[CHANNEL_RIGHT_HIGH].Run(ms);

    float engi_noise_left = EngineNoise[CHANNEL_LEFT_LOW].Run(ms) + EngineNoise[CHANNEL_LEFT_HIGH].Run(ms);
    float engi_noise_right = EngineNoise[CHANNEL_RIGHT_LOW].Run(ms) + EngineNoise[CHANNEL_RIGHT_HIGH].Run(ms);

    float coll_noise_left = CollisionNoise[TWO_CHANNEL_LEFT].Run(ms);
    float coll_noise_right = CollisionNoise[TWO_CHANNEL_RIGHT].Run(ms);

    float shifp_noise_left = ShiftPotentialNoise[TWO_CHANNEL_LEFT].Run(ms);
    float shifp_noise_right = ShiftPotentialNoise[TWO_CHANNEL_RIGHT].Run(ms);

    float nos_noise_left = NOSNoise[TWO_CHANNEL_LEFT].Run(ms);
    float nos_noise_right = NOSNoise[TWO_CHANNEL_RIGHT].Run(ms);

    float shif_noise_left = ShiftNoise[TWO_CHANNEL_LEFT].Run(ms);
    float shif_noise_right = ShiftNoise[TWO_CHANNEL_RIGHT].Run(ms);

    float blow_noise_left = EngineBlownNoise[TWO_CHANNEL_LEFT].Run(ms);
    float blow_noise_right = EngineBlownNoise[TWO_CHANNEL_RIGHT].Run(ms);

    Volume_Left = UMath::Max(road_noise_left, Volume_Left);
    Volume_Left = UMath::Max(skid_noise_left, Volume_Left);
    Volume_Left = UMath::Max(slip_noise_left, Volume_Left);
    Volume_Left = UMath::Max(coll_noise_left, Volume_Left);
    Volume_Left = UMath::Max(engi_noise_left, Volume_Left);
    Volume_Left = UMath::Max(shifp_noise_left, Volume_Left);
    Volume_Left = UMath::Max(nos_noise_left, Volume_Left);
    Volume_Left = UMath::Max(shif_noise_left, Volume_Left);
    Volume_Left = UMath::Max(blow_noise_left, Volume_Left);

    Volume_Right = UMath::Max(road_noise_right, Volume_Right);
    Volume_Right = UMath::Max(skid_noise_right, Volume_Right);
    Volume_Right = UMath::Max(slip_noise_right, Volume_Right);
    Volume_Right = UMath::Max(coll_noise_right, Volume_Right);
    Volume_Right = UMath::Max(engi_noise_right, Volume_Right);
    Volume_Right = UMath::Max(shifp_noise_right, Volume_Right);
    Volume_Right = UMath::Max(nos_noise_right, Volume_Right);
    Volume_Right = UMath::Max(shif_noise_right, Volume_Right);
    Volume_Right = UMath::Max(blow_noise_right, Volume_Right);

    Volume_Left = UMath::Clamp(Volume_Left, 0.0f, 1.0f);
    Volume_Right = UMath::Clamp(Volume_Right, 0.0f, 1.0f);
}

void InputEffectState::Push(RealInput::Effect *effect) {
    if (!effect) {
        return;
    }

    RealInput::Effect::Info info;
    effect->GetInfo(&info);
    if (Enabled) {
        info.mMagnitudeLeft = static_cast<int>(Volume_Left * 255.0f);
        info.mMagnitudeRight = 0;
    } else {
        info.mMagnitudeLeft = 0;
        info.mMagnitudeRight = 0;
    }
    effect->SetInfo(&info);

    bool isplaying = Enabled;
    if (isplaying != (effect->GetStatus() == RealInput::Effect::STATUS_PLAYING)) {
        if (isplaying) {
            effect->Start();
        } else {
            effect->Stop();
        }
    }
}

static bool IsPadSupported(RealInput::Device::Info *info) {
    if (!info) {
        return false;
    }
    switch (info->mControllerID) {
    case RealInput::CT_NOTCONNECTED:
    case RealInput::CT_UNSUPPORTED:
    case RealInput::CT_STANDARD:
    case RealInput::CT_DUALSHOCK:
    case RealInput::CT_DUALSHOCKANALOG:
        return false;
    case RealInput::CT_DUALSHOCK2:
        return true;
    }
    return false;
}

static int32_t MyEnumDeviceCallback(RealInput::Device *pDevice, uint32_t userData, RealInput::Interface *pInterface) {
    int port = pDevice->GetInfo()->mPortNum;
    int slot = pDevice->GetInfo()->mSlotNum;
    if (pDevice->IsKeyboard() && port == 0 && slot == 0) {
        input_device_keyboard = pDevice;
    } else if (pDevice->IsMouse() && port == 0 && slot == 0) {
        input_device_mouse = pDevice;
    } else if (pDevice->IsPad() && port < RealInput::PD_NUM_PORTS && slot < RealInput::PD_NUM_SLOTS) {
        int index = port + slot * RealInput::PD_NUM_PORTS;
        input_devices[index] = pDevice;
        input_connected[index] = IsPadSupported(pDevice->GetInfo()) && pDevice->GetCapabilities()->mAttached;
    }
    return 1;
}

static void InitEffects() {
    for (int i = 0; i < RealInput::PD_NUM_PORTS * RealInput::PD_NUM_SLOTS; i++) {
        RealInput::Device *device = input_devices[i];
        if (!device || !device->IsPad() || !input_connected[i]) {
            input_effects[i] = nullptr;
        }
    }
}

static void InitPads() {
#ifdef EA_BUILD_A124
    RealInput::InitPS2MultiTap();
#endif
    RealInput::ConfigOptions opts;
    opts.mAllocator = &gMemoryAllocator;
    opts.mpEnumDevicesCallback = MyEnumDeviceCallback;
    opts.mMaxNumEffects = 8;
    inputsys = RealInput::Interface::CreateInstance(opts);
    inputsys->AddRef();
    SteeringWheelDevice::InitWheelSupport();
}

#ifdef EA_BUILD_A124
static void UpdatePads(float milliseconds, bool noGameEffect) {
    inputsys->Update();
    for (int port = 0; port < RealInput::PD_NUM_PORTS * RealInput::PD_NUM_SLOTS; port++) {
        if (input_devices[port]) {
            if (input_devices[port]->Update() != RealInput::RI_OK) {
                input_connected[port] = false;
            } else if (input_devices[port]->IsPad() && !noGameEffect) {
                effect_states[port].Push(input_effects[port]);
                effect_states[port].Run(milliseconds);
            }
        }
    }

    for (RealInput::Event *event = inputsys->GetEvent(); event; event = inputsys->GetEvent()) {
        RealInput::Device *pDevice = event->mpDevice;
        if (pDevice && pDevice->IsPad()) {
            int index = pDevice->GetInfo()->mPortNum + pDevice->GetInfo()->mSlotNum * RealInput::PD_NUM_PORTS;
            switch (event->mType) {
            case RealInput::TYPE_CONNECTED:
                if (IsPadSupported(pDevice->GetInfo())) {
                    input_connected[index] = true;
                    if (!input_effects[index]) {
                        RealInput::Effect::Info info;
                        info.mMagnitudeLeft = 0;
                        info.mMagnitudeRight = 0;
                        input_effects[index] = pDevice->CreateEffect(&info);
                    }
                    break;
                }
            case RealInput::TYPE_DISCONNECTED:
                input_connected[index] = false;
                break;
            }
        } else if (pDevice->IsKeyboard()) {
            if (gFEKeyboard) {
                if (event->mType == RealInput::TYPE_PRESSED) {
                    switch (event->mObjectIndex) {
                    case 41:
                        gFEKeyboard->Dispose(true);
                        break;
                    case 40:
                        gFEKeyboard->Dispose(false);
                        break;
                    case 80:
                        gFEKeyboard->MoveCursor(-1);
                        break;
                    case 79:
                        gFEKeyboard->MoveCursor(1);
                        break;
                    case 42:
                        gFEKeyboard->AppendBackspace();
                        break;
                    case 32:
                        gFEKeyboard->AppendSpace();
                        break;
                    default:
                        gFEKeyboard->AppendChar(event->mASCIIKey);
                        break;
                    }
                }
                const DeviceScalarInfo *di = &device_infos[8];
                float *value = PS2Keys;
                while (di->name) {
                    *value = 0.0f;
                    di++;
                    value++;
                }
            } else {
                const DeviceScalarInfo *di = &device_infos[8];
                float *value = PS2Keys;
                while (di->name) {
                    if (di->type == kDigitalButton && di->devicetype == kDTKeyboard &&
                        event->mObjectIndex == static_cast<unsigned int>(di->system_index)) {
                        if (event->mType == RealInput::TYPE_PRESSED) {
                            *value = 1.0f;
                        } else if (event->mType == RealInput::TYPE_RELEASED) {
                            *value = 0.0f;
                        }
                    }
                    di++;
                    value++;
                }
            }
        }
    }

    SteeringWheelDevice::PollWheels();
}
#else
static void UpdatePads(float milliseconds) {
    bool wheelActive = false;
    int wheelPort = 0;
    inputsys->Update();
    InputDevice *device = IOModule::GetIOModule().GetDevice(0);
    if (device) {
        UTL::COM::IUnknown *pUnk = device->GetSecondaryDevice();
        if (pUnk) {
            SteeringWheelDevice *wheel;
            if (pUnk->QueryInterface(&wheel) && wheel->IsActivated()) {
                wheelActive = true;
                if (FEDatabase) {
                    wheelPort = FEDatabase->GetPlayersJoystickPort(0);
                }
            }
        }
    }

    for (int port = 0; port < RealInput::PD_NUM_PORTS * RealInput::PD_NUM_SLOTS; port++) {
        if (input_devices[port]) {
            bool padEffects = !wheelActive || port != wheelPort;
            if (input_devices[port]->Update() != RealInput::RI_OK) {
                input_connected[port] = false;
            } else if (input_devices[port]->IsPad() && input_buzz[port] <= 0.0f && padEffects) {
                effect_states[port].Push(input_effects[port]);
                effect_states[port].Run(milliseconds);
            } else if (input_buzz[port] > 0.0f) {
                input_buzz[port] -= milliseconds;
                if (input_buzz[port] <= 0.0f) {
                    RealInput::Effect *effect = input_devices[port]->GetEffect();
                    if (effect) {
                        effect->Stop();
                    }
                }
            } else if (port == 0 && !padEffects && effect_states[port].Enabled) {
                RealInput::Effect::Info info;
                input_devices[port]->GetEffect()->GetInfo(&info);
                info.mMagnitudeLeft = 0;
                info.mMagnitudeRight = 0;
                effect_states[port].Enabled = false;
                effect_states[port].Volume_Left = 0.0f;
                effect_states[port].Volume_Right = 0.0f;
                effect_states[port].CollisionNoise[TWO_CHANNEL_LEFT].Time = 0.0f;
                effect_states[port].CollisionNoise[TWO_CHANNEL_LEFT].MaxTime = 0.0f;
                effect_states[port].CollisionNoise[TWO_CHANNEL_LEFT].Amplitude = 0.0f;
                effect_states[port].CollisionNoise[TWO_CHANNEL_LEFT].MinAmplitude = 0.0f;
                effect_states[port].Push(input_effects[port]);
            }
        }
    }

    for (RealInput::Event *event = inputsys->GetEvent(); event; event = inputsys->GetEvent()) {
        RealInput::Device *pDevice = event->mpDevice;
        if (pDevice && pDevice->IsPad()) {
            int index = pDevice->GetInfo()->mPortNum + pDevice->GetInfo()->mSlotNum * RealInput::PD_NUM_PORTS;
            switch (event->mType) {
            case RealInput::TYPE_CONNECTED:
                if (!IsPadSupported(pDevice->GetInfo())) {
                    input_connected[index] = false;
                    break;
                }
                input_connected[index] = true;
                input_buzz[index] = 0.0f;
                if (!input_effects[index]) {
                    RealInput::Effect::Info info;
                    info.mMagnitudeLeft = 0;
                    info.mMagnitudeRight = 0;
                    input_effects[index] = pDevice->CreateEffect(&info);
                }
                break;
            case RealInput::TYPE_DISCONNECTED:
                input_connected[index] = false;
                input_buzz[index] = 0.0f;
                break;
            }
        } else if (pDevice->IsKeyboard()) {
            if (gFEKeyboard) {
                if (event->mType == RealInput::TYPE_PRESSED) {
                    switch (event->mObjectIndex) {
                    case 41:
                        gFEKeyboard->Dispose(true);
                        break;
                    case 40:
                        gFEKeyboard->Dispose(false);
                        break;
                    case 80:
                        gFEKeyboard->MoveCursor(-1);
                        break;
                    case 79:
                        gFEKeyboard->MoveCursor(1);
                        break;
                    case 42:
                        gFEKeyboard->AppendBackspace();
                        break;
                    case 32:
                        gFEKeyboard->AppendSpace();
                        break;
                    default:
                        gFEKeyboard->AppendChar(event->mASCIIKey);
                        break;
                    }
                }
                const DeviceScalarInfo *di = &device_infos[8];
                float *value = PS2Keys;
                while (di->name) {
                    *value = 0.0f;
                    di++;
                    value++;
                }
            } else {
                const DeviceScalarInfo *di = &device_infos[8];
                float *value = PS2Keys;
                while (di->name) {
                    if (di->type == kDigitalButton && di->devicetype == kDTKeyboard &&
                        event->mObjectIndex == static_cast<unsigned int>(di->system_index)) {
                        if (event->mType == RealInput::TYPE_PRESSED) {
                            *value = 1.0f;
                        } else if (event->mType == RealInput::TYPE_RELEASED) {
                            *value = 0.0f;
                        }
                    }
                    di++;
                    value++;
                }
            }
        }
    }

    SteeringWheelDevice::PollWheels();
}
#endif

static void ReleasePads() {
    inputsys->Release();
}

void GameDevice::Initialize() {
    int i = 0;
    mNumScalars = 0;
    const DeviceScalarInfo *info = device_infos;

    while (info->name != nullptr && i < 66) {
        fDeviceScalar[i].InitializeDeviceScalar(info->type, info->name, &fPrevValues[i], &fCurrentValues[i]);
        i++;
        mNumScalars++;
        info++;
    }

    mNOSEngaged = false;
    mEngineBlown = false;
    mShiftPotential = SHIFT_POTENTIAL_NONE;
    mShifting = false;
}

bool GameDevice::IsConnected() {
    if (mWheelDevice) {
        if (mWheelDevice->IsConnected()) {
            return true;
        }
        if (mWheelDevice->IsActivated()) {
            return mWheelDevice->IsConnected();
        }
    }
    return input_connected[GetDeviceIndex()];
}

void GameDevice::StartVibration() {
#ifndef EA_BUILD_A124
    if (IsWheel()) {
        return;
    }
#endif
    RealInput::Effect::Info effectInfo;
    effectInfo.mMagnitudeLeft = RealInput::Effect::MAX_MAGNITUDE_LEFT;
    effectInfo.mMagnitudeRight = RealInput::Effect::MAX_MAGNITUDE_RIGHT;
    RealInput::Device *device = input_devices[GetDeviceIndex()];
    RealInput::Effect *effect = device->CreateEffect(&effectInfo);
    effect->Start();
#ifdef EA_BUILD_A124
    sVibrationTime = 500.0f;
#else
    input_buzz[GetDeviceIndex()] = 500.0f;
#endif
}

#ifndef EA_BUILD_A124
void GameDevice::StopVibration() {
    if (input_buzz[GetDeviceIndex()] > 0.0f) {
        input_buzz[GetDeviceIndex()] = 0.00001f;
    }
}
#endif

#ifdef EA_BUILD_A124
void GameDevice::PollDevice() {
    float milliseconds = 0.0f;
    if (GetDeviceIndex() == 0) {
        unsigned int tick = bGetTicker();
        if (pad_ticker) {
            milliseconds = bGetTickerDifference(pad_ticker);
        }
        pad_ticker = tick;
        UpdatePads(milliseconds, sVibrationTime > 0.0f);
    }

    RealInput::Device *device = input_devices[GetDeviceIndex()];
    if (!device) {
        return;
    }

    bool wheelConnected = mWheelDevice ? mWheelDevice->IsConnected() : false;
    if (!input_connected[GetDeviceIndex()] && !wheelConnected) {
        return;
    }

    if (sVibrationTime > 0.0f) {
        sVibrationTime -= milliseconds;
        if (sVibrationTime <= 0.0f) {
            RealInput::Effect *effect = input_devices[GetDeviceIndex()]->GetEffect();
            if (effect) {
                effect->Stop();
            }
        }
    }

    if (device->IsPad()) {
        bMemCpy(fPrevValues, fCurrentValues, sizeof(fPS2CurrentValues));

        RealInput::Device::Data *data = device->GetData();

        int axis;
        int axis_min;
        int axis_max;

        axis = data->mPad.mDirection[0].mX;
        axis_min = RealInput::PD_ALSTICKXMIN;
        axis_max = RealInput::PD_ALSTICKXMAX;
        fCurrentValues[0] =
            UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                         0.0f, 1.0f);
        fCurrentValues[1] =
            -UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                          -1.0f, 0.0f);

        axis = data->mPad.mDirection[0].mY;
        axis_min = RealInput::PD_ALSTICKYMIN;
        axis_max = RealInput::PD_ALSTICKYMAX;
        fCurrentValues[2] =
            UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                         0.0f, 1.0f);
        fCurrentValues[3] =
            -UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                          -1.0f, 0.0f);

        axis = data->mPad.mDirection[1].mX;
        axis_min = RealInput::PD_ARSTICKXMIN;
        axis_max = RealInput::PD_ARSTICKXMAX;
        fCurrentValues[4] =
            UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                         0.0f, 1.0f);
        fCurrentValues[5] =
            -UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                          -1.0f, 0.0f);

        axis = data->mPad.mDirection[1].mY;
        axis_min = RealInput::PD_ARSTICKYMIN;
        axis_max = RealInput::PD_ARSTICKYMAX;
        fCurrentValues[6] =
            UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                         0.0f, 1.0f);
        fCurrentValues[7] =
            -UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                          -1.0f, 0.0f);

        if (GetDeviceIndex() <= 0 && mWheelDevice && mWheelDevice->IsActivated()) {
            fCurrentValues[0] = fCurrentValues[1] = fCurrentValues[2] = fCurrentValues[3] = fCurrentValues[4] =
                fCurrentValues[5] = fCurrentValues[6] = fCurrentValues[7] = 0.0f;
        }

        const DeviceScalarInfo *di = &device_infos[8];
        float *value = &fCurrentValues[8];
        float *key = PS2Keys;
        while (!di->wheel) {
            if (di->type == kAnalogButton) {
                bool wheelActivated = mWheelDevice ? mWheelDevice->IsActivated() : false;
                if (TheGameFlowManager.IsInGame() && !FEManager::IsPaused() && wheelActivated) {
                    *value = 0.0f;
                } else {
                    float newval = data->mPad.mButtons[di->system_index] * (1.0f / RealInput::PD_BUTTONMAX);
                    if (di->ramp_max > di->ramp_min) {
                        float range = di->ramp_max - di->ramp_min;
                        newval = UMath::Clamp((newval - di->ramp_min) / range, 0.0f, 1.0f);
                    }
                    *value = newval;
                }
            } else if (di->type == kDigitalButton) {
                if (di->devicetype == kDTKeyboard) {
                    *value = *key;
                } else if (data->mPad.mButtons[di->system_index] != 0) {
                    *value = 1.0f;
                } else {
                    *value = 0.0f;
                }
            }
            di++;
            value++;
            key++;
        }
        fTemp = &fCurrentValues[15];
    }

    if (mWheelDevice) {
        mWheelDevice->ReadInput(fCurrentValues + 38);
    }
}
#else
void GameDevice::PollDevice() {
    float milliseconds = 0.0f;
    if (GetDeviceIndex() == 0) {
        unsigned int tick = bGetTicker();
        if (pad_ticker) {
            milliseconds = bGetTickerDifference(pad_ticker);
        }
        pad_ticker = tick;
        UpdatePads(milliseconds);
    }

    RealInput::Device *device = input_devices[GetDeviceIndex()];
    if (!device) {
        return;
    }

    if (input_connected[GetDeviceIndex()]) {
        bMemCpy(fPrevValues, fCurrentValues, sizeof(fPS2CurrentValues));

        RealInput::Device::Data *data = device->GetData();

        int axis;
        int axis_min;
        int axis_max;

        axis = data->mPad.mDirection[0].mX;
        axis_min = RealInput::PD_ALSTICKXMIN;
        axis_max = RealInput::PD_ALSTICKXMAX;
        fCurrentValues[0] =
            UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                         0.0f, 1.0f);
        fCurrentValues[1] =
            -UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                          -1.0f, 0.0f);

        axis = data->mPad.mDirection[0].mY;
        axis_min = RealInput::PD_ALSTICKYMIN;
        axis_max = RealInput::PD_ALSTICKYMAX;
        fCurrentValues[2] =
            UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                         0.0f, 1.0f);
        fCurrentValues[3] =
            -UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                          -1.0f, 0.0f);

        axis = data->mPad.mDirection[1].mX;
        axis_min = RealInput::PD_ARSTICKXMIN;
        axis_max = RealInput::PD_ARSTICKXMAX;
        fCurrentValues[4] =
            UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                         0.0f, 1.0f);
        fCurrentValues[5] =
            -UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                          -1.0f, 0.0f);

        axis = data->mPad.mDirection[1].mY;
        axis_min = RealInput::PD_ARSTICKYMIN;
        axis_max = RealInput::PD_ARSTICKYMAX;
        fCurrentValues[6] =
            UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                         0.0f, 1.0f);
        fCurrentValues[7] =
            -UMath::Clamp((static_cast<float>(axis - axis_min) / static_cast<float>(axis_max - axis_min) + -0.5f) * -2.0f,
                          -1.0f, 0.0f);

        if (GetDeviceIndex() <= 0 && mWheelDevice && mWheelDevice->IsActivated() && TheGameFlowManager.IsInGame() &&
            !FEManager::IsPaused()) {
            fCurrentValues[0] = fCurrentValues[1] = fCurrentValues[2] = fCurrentValues[3] = fCurrentValues[4] =
                fCurrentValues[5] = fCurrentValues[6] = fCurrentValues[7] = 0.0f;
        }

        const DeviceScalarInfo *di = &device_infos[8];
        float *value = &fCurrentValues[8];
        float *key = PS2Keys;
        while (!di->wheel) {
            if (di->type == kAnalogButton) {
                bool wheelActivated = mWheelDevice ? mWheelDevice->IsActivated() : false;
                bool wheelInGame = wheelActivated && TheGameFlowManager.IsInGame() && !FEManager::IsPaused();
                if (wheelInGame) {
                    *value = 0.0f;
                } else {
                    float newval = data->mPad.mButtons[di->system_index] * (1.0f / RealInput::PD_BUTTONMAX);
                    if (di->ramp_max > di->ramp_min) {
                        float range = di->ramp_max - di->ramp_min;
                        newval = UMath::Clamp((newval - di->ramp_min) / range, 0.0f, 1.0f);
                    }
                    *value = newval;
                }
            } else if (di->type == kDigitalButton) {
                if (di->devicetype == kDTKeyboard) {
                    *value = *key;
                } else if (data->mPad.mButtons[di->system_index] != 0) {
                    *value = 1.0f;
                } else {
                    *value = 0.0f;
                }
            }
            di++;
            value++;
            key++;
        }
        fTemp = &fCurrentValues[15];
    } else {
        bMemCpy(fPrevValues, fCurrentValues, sizeof(fPS2CurrentValues));
        bMemSet(fCurrentValues, 0, sizeof(fPS2CurrentValues));
    }

    if (mWheelDevice) {
        mWheelDevice->ReadInput(fCurrentValues + 38);
    }
}
#endif

int GameDevice::GetNumDeviceScalar() {
    return mNumScalars;
}

GameDevice::GameDevice(int deviceIndex)
    : InputDevice(deviceIndex), IFeedback(this), mNumScalars(0), mWheelDevice(nullptr) {
    if (mCount == 0) {
        InitPads();
        InitEffects();
    }

    if (deviceIndex <= 0) {
        mWheelDevice = new ("SteeringWheelDevice", 0) SteeringWheelDevice(deviceIndex);
    }

    mCount++;
    fDeviceScalar = fPS2DeviceScalars;
    fPrevValues = fPS2PrevValues;
    fCurrentValues = fPS2CurrentValues;

    bMemSet(fPrevValues, 0, sizeof(fPS2PrevValues));
    bMemSet(fCurrentValues, 0, sizeof(fPS2CurrentValues));
}

GameDevice::~GameDevice() {
    mCount--;
    if (mCount == 0) {
        ReleasePads();
    }
}

void GameDevice::PauseEffects() {
    effect_states[GetDeviceIndex()].Enabled = false;
    effect_states[GetDeviceIndex()].Push(input_effects[GetDeviceIndex()]);
    SteeringWheels_StopAllForces();
}

void GameDevice::ResumeEffects() {
    effect_states[GetDeviceIndex()].Enabled = true;
    effect_states[GetDeviceIndex()].Push(input_effects[GetDeviceIndex()]);
}

void GameDevice::ResetEffects() {
    RealInput::Effect *effect = input_effects[GetDeviceIndex()];
    if (!effect) {
        return;
    }

    RealInput::Effect::Info info;
    effect->GetInfo(&info);
    info.mMagnitudeLeft = 0;
    info.mMagnitudeRight = 0;

    effect_states[GetDeviceIndex()].Enabled = false;
#ifndef EA_BUILD_A124
    effect_states[GetDeviceIndex()].Volume_Left = 0.0f;
    effect_states[GetDeviceIndex()].Volume_Right = 0.0f;
    effect_states[GetDeviceIndex()].CollisionNoise[TWO_CHANNEL_LEFT].Time = 0.0f;
    effect_states[GetDeviceIndex()].CollisionNoise[TWO_CHANNEL_LEFT].MaxTime = 0.0f;
    effect_states[GetDeviceIndex()].CollisionNoise[TWO_CHANNEL_LEFT].Amplitude = 0.0f;
    effect_states[GetDeviceIndex()].CollisionNoise[TWO_CHANNEL_LEFT].MinAmplitude = 0.0f;
#endif
    effect_states[GetDeviceIndex()].Push(input_effects[GetDeviceIndex()]);
}

void GameDevice::BeginUpdate() {
    InputEffectState &state = effect_states[GetDeviceIndex()];
    for (int i = 0; i < CHANNEL_MAX; i++) {
        state.RoadNoise[i].Amplitude = 0.0f;
        state.RoadNoise[i].Frequency = 0.0f;
        state.RoadNoise[i].Offset = 0.0f;

        state.SkidNoise[i].Amplitude = 0.0f;
        state.SkidNoise[i].Frequency = 0.0f;
        state.SkidNoise[i].Offset = 0.0f;

        state.SlipNoise[i].Amplitude = 0.0f;
        state.SlipNoise[i].Frequency = 0.0f;
        state.SlipNoise[i].Offset = 0.0f;
    }
}

void GameDevice::EndUpdate() {}

void GameDevice::UpdateRoadNoise(bool front, const SimSurface &surface, float speed) {
#ifndef EA_BUILD_A124
    if (IsWheel()) {
        return;
    }
#endif
#ifndef EA_BUILD_A124
    if (!effect_states[GetDeviceIndex()].Enabled) {
        return;
    }
#endif
    InputEffectState &state = effect_states[GetDeviceIndex()];
    for (int channel = 0; channel < CHANNEL_MAX; channel++) {
        float frequency;
        float amplitude;
        float offset;
        if (surface.FFB_ROLL(channel).Sample(speed, frequency, amplitude, offset)) {
            EffectWave &wave = state.RoadNoise[channel];
            wave.Frequency = UMath::Max(frequency, wave.Frequency);
            wave.Amplitude = UMath::Max(amplitude, wave.Amplitude);
            wave.Offset = UMath::Max(offset, wave.Offset);
        }
    }
}

void GameDevice::UpdateTireSkid(bool front, const SimSurface &surface, float speed) {
#ifndef EA_BUILD_A124
    if (IsWheel()) {
        return;
    }
#endif
    InputEffectState &state = effect_states[GetDeviceIndex()];
#ifndef EA_BUILD_A124
    if (!state.Enabled) {
        return;
    }
#endif
    if (front) {
        speed *= 0.25f;
    }
    for (int channel = 0; channel < CHANNEL_MAX; channel++) {
        float frequency;
        float amplitude;
        float offset;
        if (surface.FFB_SKID(channel).Sample(speed, frequency, amplitude, offset)) {
            EffectWave &wave = state.SkidNoise[channel];
            wave.Frequency = UMath::Max(frequency, wave.Frequency);
            wave.Amplitude = UMath::Max(amplitude, wave.Amplitude);
            wave.Offset = UMath::Max(offset, wave.Offset);
        }
    }
}

void GameDevice::UpdateTireSlip(bool front, const SimSurface &surface, float speed) {
#ifndef EA_BUILD_A124
    if (IsWheel()) {
        return;
    }
#endif
    InputEffectState &state = effect_states[GetDeviceIndex()];
#ifndef EA_BUILD_A124
    if (!state.Enabled) {
        return;
    }
#endif
    for (int channel = 0; channel < CHANNEL_MAX; channel++) {
        float frequency;
        float amplitude;
        float offset;
        if (surface.FFB_SLIP(channel).Sample(speed, frequency, amplitude, offset)) {
            EffectWave &wave = state.SlipNoise[channel];
            wave.Frequency = UMath::Max(frequency, wave.Frequency);
            wave.Amplitude = UMath::Max(amplitude, wave.Amplitude);
            wave.Offset = UMath::Max(offset, wave.Offset);
        }
    }
}

void GameDevice::UpdateRPM(float powerband, float overrev, float throttle) {
#ifndef EA_BUILD_A124
    if (IsWheel()) {
        return;
    }
#endif
}

void GameDevice::UpdateShiftPotential(ShiftPotential potential) {
#ifndef EA_BUILD_A124
    if (IsWheel()) {
        return;
    }
#endif
    InputEffectState &state = effect_states[GetDeviceIndex()];
#ifndef EA_BUILD_A124
    if (!state.Enabled) {
        return;
    }
#endif
    float magnitudeRight = 0.0f;
    EffectRamp &rampLeft = state.ShiftPotentialNoise[TWO_CHANNEL_LEFT];
    EffectRamp &rampRight = state.ShiftPotentialNoise[TWO_CHANNEL_RIGHT];
    bool changed = false;
    bool clear;

    switch (potential) {
    case SHIFT_POTENTIAL_GOOD:
        magnitudeRight = 0.5f;
        clear = mShiftPotential != SHIFT_POTENTIAL_NONE;
        changed = mShiftPotential != SHIFT_POTENTIAL_GOOD;
        break;
    case SHIFT_POTENTIAL_PERFECT:
        magnitudeRight = 0.75f;
        clear = mShiftPotential != SHIFT_POTENTIAL_NONE;
        changed = mShiftPotential != SHIFT_POTENTIAL_PERFECT;
        break;
    case SHIFT_POTENTIAL_MISS:
        magnitudeRight = 1.0f;
        clear = mShiftPotential != SHIFT_POTENTIAL_NONE;
        changed = mShiftPotential != SHIFT_POTENTIAL_MISS;
        break;
    default:
        clear = mShiftPotential != SHIFT_POTENTIAL_NONE;
        break;
    }

    if (clear) {
        rampLeft.Clear();
        rampRight.Clear();
    }

    ShiftPotential previous = mShiftPotential;
    mShiftPotential = potential;
    if (magnitudeRight <= 0.0f) {
        return;
    }

    if (changed && previous < potential) {
        rampLeft.Time = 3.0f;
        rampLeft.MaxTime = 3.0f;
        rampLeft.Amplitude = UMath::Min(magnitudeRight, 1.0f);
        rampLeft.MinAmplitude = UMath::Min(magnitudeRight, magnitudeRight);

        rampRight.Time = 3.0f;
        rampRight.MaxTime = 3.0f;
        rampRight.Amplitude = UMath::Min(magnitudeRight, 1.0f);
        rampRight.MinAmplitude = UMath::Min(magnitudeRight, magnitudeRight);
    }
}

void GameDevice::UpdateEngineBlown(bool blown) {
#ifndef EA_BUILD_A124
    if (IsWheel()) {
        return;
    }
#endif
    if (blown ^ mEngineBlown) {
        InputEffectState &state = effect_states[GetDeviceIndex()];
#ifndef EA_BUILD_A124
        if (!state.Enabled) {
            return;
        }
#endif
        EffectRamp &rampLeft = state.EngineBlownNoise[TWO_CHANNEL_LEFT];
        EffectRamp &rampRight = state.EngineBlownNoise[TWO_CHANNEL_RIGHT];

        if (blown) {
            rampLeft.Time = 1.0f;
            rampLeft.MaxTime = 1.0f;
            rampLeft.Amplitude = UMath::Min(1.0f, 1.0f);
            rampLeft.MinAmplitude = UMath::Min(0.6f, rampLeft.Amplitude);

            rampRight.Time = 1.0f;
            rampRight.MaxTime = 1.0f;
            rampRight.Amplitude = UMath::Min(1.0f, rampRight.Amplitude);
            rampRight.MinAmplitude = UMath::Min(0.6f, 1.0f);
        } else {
            rampLeft.Clear();
            rampRight.Clear();
        }

        mEngineBlown = blown;
    }
}

void GameDevice::UpdateNOS(bool engaged, float NOSLevel) {
#ifndef EA_BUILD_A124
    if (IsWheel()) {
        return;
    }
#endif
    if (engaged ^ mNOSEngaged) {
        InputEffectState &state = effect_states[GetDeviceIndex()];
#ifndef EA_BUILD_A124
        if (!state.Enabled) {
            return;
        }
#endif
        EffectRamp &rampLeft = state.NOSNoise[TWO_CHANNEL_LEFT];

        if (engaged) {
            rampLeft.Time = 2.0f;
            rampLeft.MaxTime = 2.0f;
            rampLeft.Amplitude = UMath::Min(1.0f, 1.0f);
            rampLeft.MinAmplitude = UMath::Min(0.5f, rampLeft.Amplitude);

            rampLeft.Time = 2.0f;
            rampLeft.MaxTime = 2.0f;
            rampLeft.Amplitude = UMath::Min(1.0f, rampLeft.Amplitude);
            rampLeft.MinAmplitude = UMath::Min(0.5f, 1.0f);
        } else {
            rampLeft.Clear();
            rampLeft.Clear();
        }

        mNOSEngaged = engaged;
    }
}

void GameDevice::UpdateShifting(bool shifting) {
#ifndef EA_BUILD_A124
    if (IsWheel()) {
        return;
    }
#endif
    if (mShifting ^ shifting) {
        InputEffectState &state = effect_states[GetDeviceIndex()];
#ifndef EA_BUILD_A124
        if (!state.Enabled) {
            return;
        }
#endif
        EffectRamp &rampLeft = state.ShiftNoise[TWO_CHANNEL_LEFT];
        EffectRamp &rampRight = state.ShiftNoise[TWO_CHANNEL_RIGHT];

        if (shifting) {
            rampLeft.MaxTime = 0.5f;
            rampLeft.Time = 0.5f;
            rampLeft.Amplitude = UMath::Clamp(0.4f, 0.0f, 1.0f);
            rampLeft.MinAmplitude = UMath::Clamp(0.4f, 0.0f, 1.0f);

            rampRight.MaxTime = 0.5f;
            rampRight.Time = 0.5f;
            rampRight.Amplitude = UMath::Clamp(0.4f, 0.0f, 1.0f);
            rampRight.MinAmplitude = UMath::Clamp(0.4f, 0.0f, 1.0f);
        }

        mShifting = shifting;
    }
}

void GameDevice::ReportCollision(const COLLISION_INFO &cinfo, bool iamA) {
#ifndef EA_BUILD_A124
    if (IsWheel()) {
        return;
    }
#endif
    InputEffectState &state = effect_states[GetDeviceIndex()];
#ifndef EA_BUILD_A124
    if (!state.Enabled) {
        return;
    }
#endif
    float magnitude = 0.0f;

    ISimable *me = ISimable::FindInstance(iamA ? cinfo.objA : cinfo.objB);
    ISimable *them = ISimable::FindInstance(iamA ? cinfo.objB : cinfo.objA);

    if (me == nullptr) {
        return;
    }

    UMath::Vector3 current_velocity;
    me->GetLinearVelocity(current_velocity);

    float myspeed = UMath::Length(current_velocity);
    float speedchange = UMath::Distance(current_velocity, iamA ? cinfo.objAVel : cinfo.objBVel);
    float their_speed_change = 0.0f;

    if (them != nullptr) {
        UMath::Vector3 tmp;
        them->GetLinearVelocity(tmp);
        their_speed_change = UMath::Distance(tmp, iamA ? cinfo.objAVel : cinfo.objBVel);
    }

    switch (cinfo.type) {
    case 2:
        magnitude = UMath::Min(speedchange * 0.033333335f, 1.0f);
        break;

    case 3:
        magnitude = UMath::Min((speedchange - 1.0f) * 0.33333334f, 1.0f);
        break;

    case 1:
        if (speedchange < 5.0f) {
            bool backEnder = false;

            if (them != nullptr) {
                IVehicle *theirVehicle;

                if (them->QueryInterface(&theirVehicle)) {
                    UMath::Vector3 myVelocity = iamA ? cinfo.objAVel : cinfo.objBVel;
                    UMath::Vector3 theirVelocity = iamA ? cinfo.objBVel : cinfo.objAVel;

                    UMath::Normalize(myVelocity);
                    UMath::Normalize(theirVelocity);

                    if (UMath::Dot(myVelocity, theirVelocity) > 0.1f) {
                        speedchange = UMath::Max(UMath::Length(cinfo.objAVel), UMath::Length(cinfo.objAVel)) * 0.5f;
                        backEnder = true;
                    }
                }
            }

            if (backEnder && speedchange > 5.0f) {
                magnitude = UMath::Min((speedchange - 5.0f) * 0.05f, 1.0f);
#ifdef EA_BUILD_A124
            } else if (myspeed > 2.0f && their_speed_change > 0.0f) {
#else
            } else if (myspeed > 2.0f && their_speed_change > 2.0f) {
#endif
                magnitude = UMath::Ramp(myspeed, 20.0f, 50.0f);
            }
        } else {
            magnitude = UMath::Min((speedchange - 5.0f) * 0.05f, 1.0f);
        }
        break;
    }

    if (magnitude <= 0.0f) {
        return;
    }

    EffectRamp &ramp = state.CollisionNoise[TWO_CHANNEL_LEFT];
    if (ramp.Time > 0.0f) {
        ramp.MaxTime = UMath::Max(magnitude, ramp.MaxTime);
        ramp.Time = UMath::Max(magnitude, ramp.Time);
        ramp.Amplitude = UMath::Max(magnitude, ramp.Amplitude);
        ramp.MinAmplitude = 0.0f;
    } else {
        ramp.MaxTime = magnitude;
        ramp.Time = magnitude;
        ramp.Amplitude = magnitude;
        ramp.MinAmplitude = 0.0f;
    }
}
