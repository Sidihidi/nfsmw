#ifndef REALCORE_COMMON_INPUT_H
#define REALCORE_COMMON_INPUT_H

#ifndef REALINPUT_INPUT_IMPLEMENTATION
#pragma interface "input.h"
#endif

#include "Allocator/iallocator.h"
#include "types.h"

namespace RealInput {

// Los valores de RealInput (DWARF de GameCube y stabs de la A124); el enum de 8 plataformas era el de AEMS.
enum Platform {
    PLATFORM_PC = 0,
    PLATFORM_PS2 = 1,
    PLATFORM_XBOX = 2,
    PLATFORM_GC = 3,
};

enum RiResult {
    RI_OK = 0,
    RI_EVENT_QUEUE_FULL = 1,
    RI_EVENT_NOT_FOUND = 2,
    RI_FUNCTION_NOT_IMPLEMENTED = 3,
    RI_ACQUIRE_FAILED = 4,
    RI_NO_DIRECTINPUT_DEVICE = 5,
    RI_FAILED_GETTING_DEVICE_STATE = 6,
    RI_NO_EFFECT = 7,
};

struct AXES {
    int mX;
    int mY;
    int mZ;
};

struct Device;

void FreeMemSize(void *pBlock, int size);

struct Effect {
#ifdef EA_PLATFORM_PLAYSTATION2
    // PS2: los dos motores del DualShock (stabs de la A124: Info mide 0x8 y el enum Constants da sus maximos).
    enum Constants {
        MAX_MAGNITUDE_LEFT = 255,
        MAX_MAGNITUDE_RIGHT = 1,
    };

    struct Info {
        int32_t mMagnitudeLeft;
        int32_t mMagnitudeRight;

        Info();
    };
#else
    struct Info {
        unsigned int mFullStop;

        Info();
    };
#endif

    enum Status {
        STATUS_STOPPED = 0,
        STATUS_PLAYING = 1,
    };

    static inline void operator delete(void *ptr, unsigned int size) {
        FreeMemSize(ptr, size);
    }

    Effect();
    virtual ~Effect();
    virtual void Start();
    virtual void Stop();
    virtual Status GetStatus();
    virtual Device *GetDevice();
    virtual void GetInfo(Info *info);
    virtual void SetInfo(Info *info);
};

struct Device {
    enum Type {
        TYPE_UNKNOWN = 0,
        TYPE_KEYBOARD = 1,
        TYPE_MOUSE = 2,
        TYPE_PAD = 3,
    };

    struct Info {
        Platform mPlatform;
        Type mType;
        unsigned int mJoypadID;
        unsigned int mControllerID;
        unsigned int mPortNum;
#ifdef EA_PLATFORM_PLAYSTATION2
        unsigned int mSlotNum; // la ranura del multitap (stabs de la A124: Info mide 0x18)
#endif
    };

    struct Capabilities {
        unsigned int mNumDigitalButtons;
        unsigned int mNumAnalogButtons;
        unsigned int mAttached : 1;
        unsigned int mForceFeedback : 1;
        unsigned int mUnused : 30;
    };

    struct Pad {
        AXES mDirection[2];
        int mButtons[64];
    };

    struct Keyboard {
        unsigned char mMap[256];
    };

    struct Mouse {
        int mX;
        int mY;
        int mZ;
        unsigned char mButtons[8];
    };

    union Data {
        Pad mPad;
        Keyboard mKeyboard;
        Mouse mMouse;
    };

    Device();
    Device(Platform platform, Type type);
    virtual ~Device();

    virtual Data *GetData() {
        return &this->mData;
    }

    int32_t IsKeyboard() {
        return mInfo.mType == TYPE_KEYBOARD;
    }

    int32_t IsMouse() {
        return mInfo.mType == TYPE_MOUSE;
    }

    int32_t IsPad() {
        return mInfo.mType == TYPE_PAD;
    }

    Capabilities *GetCapabilities() {
        return &this->mCapabilities;
    }

    Info *GetInfo() {
        return &this->mInfo;
    }

    virtual RiResult Acquire() {
        return RI_OK;
    }

    virtual RiResult Release() {
        return RI_OK;
    }

    virtual RiResult Update() {
        return RI_OK;
    }

    virtual Effect *CreateEffect(Effect::Info *) {
        return nullptr;
    }

    virtual Effect *GetEffect() {
        return nullptr;
    }

    virtual unsigned int GetKeyState(unsigned int) {
        return 0;
    }

    static void operator delete(void *ptr, unsigned int size) {
        FreeMemSize(ptr, size);
    }

  private:
    void InitData();

  protected:
    Info mInfo;
    Capabilities mCapabilities;
    Data mData;
};

enum Type {
    TYPE_CONNECTED = 0,
    TYPE_DISCONNECTED = 1,
    TYPE_PRESSED = 2,
    TYPE_RELEASED = 3,
    TYPE_CHANGE = 4,
};

enum Object {
    OBJECT_UNKNOWN = 0,
    OBJECT_BUTTON = 1,
    OBJECT_MDIRECTION = 2,
    OBJECT_DIRECTIONLEFT = 3,
    OBJECT_DIRECTIONRIGHT = 4,
};

struct Event {
    Event();
    virtual ~Event();

    static inline void operator delete(void *ptr, unsigned int size) {
        FreeMemSize(ptr, size);
    }

    Device *mpDevice;
    Type mType;
    Object mObject;
    unsigned int mObjectIndex;
    unsigned int mTimestamp;
    unsigned int mVirtKey;
    unsigned int mASCIIKey;
    unsigned int mKeyState;
};

struct ConfigOptions;
struct DeviceImp;
struct EventQueue;
struct GcEffect;

struct Interface;

void SetAllocator(EA::Allocator::IAllocator *allocator);
void ReleaseAllocator();
void *AllocateMemSize(const char *name, int size, int align, int alignoffset, int flag);

struct ConfigOptions {
    ConfigOptions()
        : mAllocator(nullptr)
#ifdef EA_PLATFORM_PLAYSTATION2
        , mIOPModulePath(nullptr)
#elif defined(EA_PLATFORM_WIN32)
        , win32_unknown_0x4(nullptr)
#endif
        , mpEnumDevicesCallback(nullptr)
        , mEnumDevicesCallbackUserData(0)
        , mEventQueueSize(0x20)
        , mMaxNumEffects(0) {
    }

    EA::Allocator::IAllocator *mAllocator;
#ifdef EA_PLATFORM_PLAYSTATION2
    char *mIOPModulePath; // los modulos de la IOP (stabs de la A124: ConfigOptions mide 0x18)
#elif defined(EA_PLATFORM_WIN32)
    // PC: ConfigOptions tambien mide 0x18 y este campo va en +0x4 (speed.exe: el ctor de InterfaceImp en 0x7F1E60
    // copia 6 palabras y lee mEventQueueSize en +0x10; el llamante del juego, 0x628410, lo deja a 0). Ningun binario
    // da su nombre ni su tipo; probablemente es el dato de plataforma que ocupa el sitio del mIOPModulePath de PS2.
    void *win32_unknown_0x4;
#endif
    int (*mpEnumDevicesCallback)(Device *, unsigned int, Interface *);
    unsigned int mEnumDevicesCallbackUserData;
    unsigned int mEventQueueSize;
    unsigned int mMaxNumEffects;
};

struct Interface {
    static Interface *CreateInstance(const ConfigOptions &options);

    virtual int AddRef();
    virtual int Release();
    virtual void Update();
    virtual Device *GetPad();
    virtual Device *GetMouse();
    virtual Device *GetKeyboard();
    virtual Event *GetEvent();
    virtual ~Interface() {}
};

#ifdef EA_PLATFORM_PLAYSTATION2
// Los mandos de PS2 (los enum salen de los stabs de la A124): botones, tipo de mando conectado y los limites del pad.
enum Ps2DigitalPresses {
    SC_DSELECT = 0,
    SC_DL3 = 1,
    SC_DR3 = 2,
    SC_DSTART = 3,
};

enum Ps2AnalogPresses {
    SC_ARIGHT = 4,
    SC_ALEFT = 5,
    SC_AUP = 6,
    SC_ADOWN = 7,
    SC_ATRI = 8,
    SC_ACIRCLE = 9,
    SC_ACROSS = 10,
    SC_ASQUARE = 11,
    SC_AL1 = 12,
    SC_AR1 = 13,
    SC_AL2 = 14,
    SC_AR2 = 15,
};

enum Ps2ConnectTypes {
    CT_NOTCONNECTED = 0,
    CT_UNSUPPORTED = 1,
    CT_STANDARD = 2,
    CT_DUALSHOCK = 3,
    CT_DUALSHOCKANALOG = 4,
    CT_DUALSHOCK2 = 5,
};

enum Ps2PadDefines {
    PD_NUM_PORTS = 2,
    PD_NUM_SLOTS = 4,
    PD_ALSTICKXMIN = 0,
    PD_ALSTICKXMAX = 255,
    PD_ALSTICKYMIN = 0,
    PD_ALSTICKYMAX = 255,
    PD_ARSTICKXMIN = 0,
    PD_ARSTICKXMAX = 255,
    PD_ARSTICKYMIN = 0,
    PD_ARSTICKYMAX = 255,
    PD_TRIGGERMAX = 255,
    PD_BUTTONMAX = 255,
};

enum Ps2KeyStates {
    KS_SHIFT = 1,
    KS_CTRL = 2,
    KS_ALT = 4,
};

void InitPS2MultiTap();
#endif

struct ITimer {
    static unsigned int GetTime();
};

} // namespace RealInput

#endif
