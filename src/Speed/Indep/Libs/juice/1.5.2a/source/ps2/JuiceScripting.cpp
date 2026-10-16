// Juice 1.5.2a: JuiceScripting.cpp, la infraestructura de comandos de
// scripting: VarArgs (argumentos tipados que se leen del cJuiceCom y se
// drenan en el destructor), ICommands con su InvokeCommand (reconstruye el
// puntero a miembro de los 8 bytes recibidos y despacha devolviendo int,
// float o cadena segun el tipo), y las interfaces IController e
// IMouseAndKeyboard. Escrito del asm de la A124 (JuiceScripting.o: .text
// 0x4B8948, 22 etiquetas). Unidad solo enlaza en A124. Casan 20 de 23: los
// AutoRegister de IController (59,3 %) e IMouseAndKeyboard (63,4 %)
// quedan parciales: RegisterMethod convierte el PMF-parametro (variable,
// no constante) al PMF de ICommands, pero el compilador usa un temporal
// por rama de la conversion y el original uno solo (banco p2j3_pmf).

#include <string.h>

namespace Juice {

enum Command {
};
enum Channels {
};

class cJuiceCom {
public:
    void NewMsg(Command cmd, Channels channel);
    void EndMsg(Channels channel);
    void Send(Channels channel);
    void PutInt(int i, Channels channel);
    void PutFloat(float f, Channels channel);
    void PutString(const char *pString, Channels channel);
    void PutBytes(int iBytes, const char *pBytes, Channels channel);
    int GetInt(Channels channel);
    float GetFloat(Channels channel);
    char *GetString(Channels channel);
    char *GetNBytes(int iBytes, Channels channel);
};

class GameHook {
public:
    static GameHook *(*Instance)();
    int IsConnected();

    int m_bConnected;
    int m_iFrameCount;
    int m_iUnused8;
    int m_iUnused12;
    int m_iUnused16;
    cJuiceCom *m_pJuiceCom;
};

namespace Scripting {

class VarArgs {
public:
    VarArgs();
    ~VarArgs();
    int GetNumberOfRemainingArgs() const;
    int GetInt(int &iValue);
    int GetFloat(float &fValue);
    int GetString(char *pString);

private:
    int m_iNextArgType;    // +0x00
    int m_iRemainingArgs;  // +0x04
};

class ICommands {
public:
    ICommands();
    virtual ~ICommands();
    virtual void AutoRegister() const;
    virtual const char *GetNamespaceName() const = 0;
    static void InvokeCommand();
    static void SendAsyncEvent(const char *pName, const char *pData);
    static void RegisterAsyncEvent(const char *pName, const char *pData);
    static void UnregisterAsyncEvent(const char *pName);
    static void SendMethod(int iID, const ICommands *pCommands, const char *pMethod, int iSize,
                           const void *pMethodData);
    template <class T> void RegisterMethod(const char *pName, void (T::*method)(VarArgs &)) const {
        void (ICommands::*converted)(VarArgs &) = (void (ICommands::*)(VarArgs &))method;
        SendMethod(0, this, pName, 8, &converted);
    }
};

class IController : public ICommands {
public:
    IController();
    virtual ~IController();
    virtual void AutoRegister() const;
    virtual void DpadDown(VarArgs &args) = 0;
    virtual void DpadUp(VarArgs &args) = 0;
    virtual void DpadLeft(VarArgs &args) = 0;
    virtual void DpadRight(VarArgs &args) = 0;
    virtual void LeftAnalog(VarArgs &args) = 0;
    virtual void RightAnalog(VarArgs &args) = 0;
    virtual void Button1(VarArgs &args) = 0;
    virtual void Button2(VarArgs &args) = 0;
    virtual void Button3(VarArgs &args) = 0;
    virtual void Button4(VarArgs &args) = 0;
    virtual void ButtonL1(VarArgs &args) = 0;
    virtual void ButtonL2(VarArgs &args) = 0;
    virtual void ButtonL3(VarArgs &args) = 0;
    virtual void ButtonR1(VarArgs &args) = 0;
    virtual void ButtonR2(VarArgs &args) = 0;
    virtual void ButtonR3(VarArgs &args) = 0;
    virtual void ButtonStart(VarArgs &args) = 0;
    virtual void ButtonSelect(VarArgs &args) = 0;
    virtual void JuiceButtonPress(VarArgs &args) = 0;
};

class IMouseAndKeyboard : public ICommands {
public:
    virtual ~IMouseAndKeyboard();
    virtual void AutoRegister() const;
    virtual void MouseLeftClick(VarArgs &args) = 0;
    virtual void MouseRightClick(VarArgs &args) = 0;
    virtual void MouseMiddleClick(VarArgs &args) = 0;
    virtual void MouseScrollUp(VarArgs &args) = 0;
    virtual void MouseScrollDown(VarArgs &args) = 0;
    virtual void MouseMove(VarArgs &args) = 0;
    virtual void KeyboardPress(VarArgs &args) = 0;
};

VarArgs::VarArgs() {
    cJuiceCom *pCom = GameHook::Instance()->m_pJuiceCom;

    m_iRemainingArgs = pCom->GetInt((Channels)0);
    m_iNextArgType = pCom->GetInt((Channels)0);
}

int VarArgs::GetNumberOfRemainingArgs() const {
    return m_iRemainingArgs;
}

int VarArgs::GetInt(int &iValue) {
    if (m_iRemainingArgs == 0) {
        return 0;
    }
    if (m_iNextArgType != 0) {
        return 0;
    }
    {
        cJuiceCom *pCom = GameHook::Instance()->m_pJuiceCom;

        iValue = pCom->GetInt((Channels)0);
        m_iNextArgType = pCom->GetInt((Channels)0);
    }
    m_iRemainingArgs--;
    return 1;
}

int VarArgs::GetFloat(float &fValue) {
    if (m_iRemainingArgs == 0) {
        return 0;
    }
    if (m_iNextArgType != 1) {
        return 0;
    }
    {
        cJuiceCom *pCom = GameHook::Instance()->m_pJuiceCom;

        fValue = pCom->GetFloat((Channels)0);
        m_iNextArgType = pCom->GetInt((Channels)0);
    }
    m_iRemainingArgs--;
    return 1;
}

int VarArgs::GetString(char *pString) {
    if (m_iRemainingArgs == 0) {
        return 0;
    }
    if (m_iNextArgType != 2) {
        return 0;
    }
    {
        cJuiceCom *pCom = GameHook::Instance()->m_pJuiceCom;

        strcpy(pString, pCom->GetString((Channels)0));
        m_iNextArgType = pCom->GetInt((Channels)0);
    }
    m_iRemainingArgs--;
    return 1;
}

VarArgs::~VarArgs() {
    char buffer[512];
    int iValue;
    float fValue;

    if (m_iRemainingArgs != 0) {
        for (;;) {
            if (GetInt(iValue)) {
                continue;
            }
            if (GetFloat(fValue)) {
                continue;
            }
            if (GetString(buffer)) {
                continue;
            }
            break;
        }
    }
}

ICommands::ICommands() {
}

ICommands::~ICommands() {
}

void ICommands::AutoRegister() const {
}

void ICommands::InvokeCommand() {
    cJuiceCom *pCom = GameHook::Instance()->m_pJuiceCom;
    int iReturnType = pCom->GetInt((Channels)0);
    int iObject = pCom->GetInt((Channels)0);

    switch (iReturnType) {
    case 0: {
        int (ICommands::*method)(VarArgs &);
        int iResult;

        memcpy(&method, pCom->GetNBytes(8, (Channels)0), 8);
        VarArgs args;
        iResult = (((ICommands *)iObject)->*method)(args);
        pCom->NewMsg((Command)503, (Channels)0);
        pCom->PutInt(0, (Channels)0);
        pCom->PutInt(iResult, (Channels)0);
        break;
    }
    case 1: {
        float (ICommands::*method)(VarArgs &);
        float fResult;

        memcpy(&method, pCom->GetNBytes(8, (Channels)0), 8);
        VarArgs args;
        fResult = (((ICommands *)iObject)->*method)(args);
        pCom->NewMsg((Command)503, (Channels)0);
        pCom->PutInt(1, (Channels)0);
        pCom->PutFloat(fResult, (Channels)0);
        break;
    }
    case 2: {
        char *(ICommands::*method)(VarArgs &);
        char *pResult;

        memcpy(&method, pCom->GetNBytes(8, (Channels)0), 8);
        VarArgs args;
        pResult = (((ICommands *)iObject)->*method)(args);
        pCom->NewMsg((Command)503, (Channels)0);
        pCom->PutInt(2, (Channels)0);
        pCom->PutString(pResult, (Channels)0);
        break;
    }
    }
    pCom->EndMsg((Channels)0);
    pCom->Send((Channels)0);
}

void ICommands::SendAsyncEvent(const char *pName, const char *pData) {
    if (GameHook::Instance()->IsConnected()) {
        cJuiceCom *pCom = GameHook::Instance()->m_pJuiceCom;

        pCom->NewMsg((Command)504, (Channels)0);
        pCom->PutString(pName, (Channels)0);
        pCom->PutString(pData, (Channels)0);
        pCom->EndMsg((Channels)0);
        pCom->Send((Channels)0);
    }
}

void ICommands::RegisterAsyncEvent(const char *pName, const char *pData) {
    if (GameHook::Instance()->IsConnected()) {
        cJuiceCom *pCom = GameHook::Instance()->m_pJuiceCom;

        pCom->NewMsg((Command)505, (Channels)0);
        pCom->PutString(pName, (Channels)0);
        pCom->PutString(pData, (Channels)0);
        pCom->EndMsg((Channels)0);
        pCom->Send((Channels)0);
    }
}

void ICommands::UnregisterAsyncEvent(const char *pName) {
    if (GameHook::Instance()->IsConnected()) {
        cJuiceCom *pCom = GameHook::Instance()->m_pJuiceCom;

        pCom->NewMsg((Command)505, (Channels)0);
        pCom->PutString(pName, (Channels)0);
        pCom->PutString("remove", (Channels)0);
        pCom->EndMsg((Channels)0);
        pCom->Send((Channels)0);
    }
}

void ICommands::SendMethod(int iID, const ICommands *pCommands, const char *pMethod, int iSize,
                           const void *pMethodData) {
    if (GameHook::Instance()->IsConnected()) {
        cJuiceCom *pCom = GameHook::Instance()->m_pJuiceCom;

        pCom->NewMsg((Command)502, (Channels)0);
        pCom->PutString(pCommands->GetNamespaceName(), (Channels)0);
        pCom->PutInt((int)pCommands, (Channels)0);
        pCom->PutInt(iID, (Channels)0);
        pCom->PutString(pMethod, (Channels)0);
        pCom->PutBytes(iSize, (const char *)pMethodData, (Channels)0);
        pCom->EndMsg((Channels)0);
        pCom->Send((Channels)0);
    }
}

#define JUICE_REGISTER_CONTROLLER_METHOD(name) RegisterMethod(#name, &IController::name);

#define JUICE_REGISTER_MOUSE_METHOD(name) RegisterMethod(#name, &IMouseAndKeyboard::name);

IController::IController() {
}

IController::~IController() {
}

IMouseAndKeyboard::~IMouseAndKeyboard() {
}

void IController::AutoRegister() const {
    JUICE_REGISTER_CONTROLLER_METHOD(DpadDown)
    JUICE_REGISTER_CONTROLLER_METHOD(DpadUp)
    JUICE_REGISTER_CONTROLLER_METHOD(DpadLeft)
    JUICE_REGISTER_CONTROLLER_METHOD(DpadRight)
    JUICE_REGISTER_CONTROLLER_METHOD(LeftAnalog)
    JUICE_REGISTER_CONTROLLER_METHOD(RightAnalog)
    JUICE_REGISTER_CONTROLLER_METHOD(Button1)
    JUICE_REGISTER_CONTROLLER_METHOD(Button2)
    JUICE_REGISTER_CONTROLLER_METHOD(Button3)
    JUICE_REGISTER_CONTROLLER_METHOD(Button4)
    JUICE_REGISTER_CONTROLLER_METHOD(ButtonL1)
    JUICE_REGISTER_CONTROLLER_METHOD(ButtonL2)
    JUICE_REGISTER_CONTROLLER_METHOD(ButtonL3)
    JUICE_REGISTER_CONTROLLER_METHOD(ButtonR1)
    JUICE_REGISTER_CONTROLLER_METHOD(ButtonR2)
    JUICE_REGISTER_CONTROLLER_METHOD(ButtonR3)
    JUICE_REGISTER_CONTROLLER_METHOD(ButtonStart)
    JUICE_REGISTER_CONTROLLER_METHOD(ButtonSelect)
    JUICE_REGISTER_CONTROLLER_METHOD(JuiceButtonPress)
    RegisterMethod("PS2ButtonCross", &IController::Button1);
    RegisterMethod("PS2ButtonCircle", &IController::Button2);
    RegisterMethod("PS2ButtonTriangle", &IController::Button3);
    RegisterMethod("PS2ButtonSquare", &IController::Button4);
    RegisterMethod("PS2ButtonL1", &IController::ButtonL1);
    RegisterMethod("PS2ButtonL2", &IController::ButtonL2);
    RegisterMethod("PS2LeftAnalogDown", &IController::LeftAnalog);
    RegisterMethod("PS2ButtonR1", &IController::ButtonR1);
    RegisterMethod("PS2ButtonR2", &IController::ButtonR2);
    RegisterMethod("PS2RightAnalogDown", &IController::RightAnalog);
    RegisterMethod("PS2ButtonStart", &IController::ButtonStart);
    RegisterMethod("PS2ButtonSelect", &IController::ButtonSelect);
    RegisterMethod("ButtonGoBack", &IController::ButtonSelect);
}

void IMouseAndKeyboard::AutoRegister() const {
    JUICE_REGISTER_MOUSE_METHOD(MouseLeftClick)
    JUICE_REGISTER_MOUSE_METHOD(MouseRightClick)
    JUICE_REGISTER_MOUSE_METHOD(MouseMiddleClick)
    JUICE_REGISTER_MOUSE_METHOD(MouseScrollUp)
    JUICE_REGISTER_MOUSE_METHOD(MouseScrollDown)
    JUICE_REGISTER_MOUSE_METHOD(MouseMove)
    JUICE_REGISTER_MOUSE_METHOD(KeyboardPress)
}

}
}
