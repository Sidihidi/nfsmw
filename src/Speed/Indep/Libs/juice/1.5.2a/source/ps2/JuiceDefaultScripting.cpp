// Juice 1.5.2a: JuiceDefaultScripting.cpp, los comandos de scripting por
// defecto: Controller (botones del mando y el mono), Commands (pantalla,
// delta de botones y reboot en crash) y Debug (la bateria de Test*).
// Escrito del asm de la A124 (JuiceDefaultScripting.o: .text 0x4B3C20, 49
// etiquetas). Unidad solo enlaza en A124. Casan 48 de 52 (ronda p2j3:
// los destructores son implicitos y los Test* anidan las comprobaciones
// en positivo); quedan los 3 constructores (62-75 %, la conversion de
// PMF del ABI viejo con un temporal de mas por rama) e InstantCrash
// (47 %, el original recarga el puntero nulo tras escribirlo).

#include <string.h>

namespace Juice {

enum eJuiceButtonNames {
};
enum eMonkeyState {
};

class GameHook {
public:
    static GameHook *(*Instance)();
    void GameEvent(const char *p1, const char *p2, const char *p3, int iValue, float fValue, const char *p4,
                   const char *p5, const char *p6);
    void LogStat(int iLevel, const char *pName, const char *pValue, int iValue);
    void AdvanceCycle(int iLevel);
    void ResetCycle(int iLevel);
    void AssertLog(int iLine, const char *pFunction, const char *pFile, const char *pCondition,
                   const char *pMessage);
};

namespace Util {
const char *GetXboxDebugIP();
}

class JuicePad {
public:
    static JuicePad *Instance();
    void SetScriptButton(eJuiceButtonNames eButton, float fValue);
    void SetScriptButtonMask(unsigned int iMask);
    void SetScriptButtonDelta(int iDelta);
    void StopScriptButtonDelta();
};

class JuiceMonkey {
public:
    static JuiceMonkey *Instance();
    void SetMonkeyState(eMonkeyState eState);
    void ActivatePadMonkey(bool bActivate);
};

class SystemManager {
public:
    static SystemManager *Instance();
    void SetAutomationCrashOption(bool bOption);
};

class ThreadManager {
public:
    static ThreadManager *Instance();
    void InflictHang();
};

struct tJuicePad;

class IExtension {
public:
    IExtension();
    virtual char *GetTitleName() = 0;
    virtual char *GetBuildName() = 0;
    virtual char *GetChangeList() = 0;
    virtual char *GetConsoleTarget() = 0;
    virtual char *GetXboxPath() = 0;
    virtual char *GetXboxExeName() = 0;
    virtual char *GetServerIP() = 0;
    virtual char *GetConsoleName() = 0;
    virtual int GetServerPort() = 0;
    virtual int GetReadyToReset() = 0;
    virtual void FileSyncUpdate() = 0;
    virtual void GamePadToJuicePad(void *pGamePad, tJuicePad *pJuicePad) = 0;
    virtual void JuicePadToGamePad(tJuicePad *pJuicePad, void *pGamePad) = 0;
    virtual int HasFileLoaded(const void *pFile) = 0;
    virtual char *GetBuildDate();
    virtual int GetInitializationTimeOut();
    virtual int GetHeartBeatTimeOut();
    virtual char *GetBackupServerIP();
    virtual int GetBackupServerPort();
    virtual int NetworkInitRetries();
    virtual int InitialConnectionRetries();
    virtual int WaitBetweenInitialConnectionRretries();
    virtual int WaitBetweenNetworkInitRetries();
    virtual void ThreadYield(int iTime);
    virtual char *GetScreenName();
    virtual char *GetPopupName();
    virtual char *GetPopupCheck();
    virtual char *GetCursorPos();
    virtual float GetGameTime();
    virtual int IsOkToConnect();
    virtual void InstantiateScriptExt();
    virtual void ResetGamePad();
    virtual int HangTimeoutLength();
    virtual void UpdatePad(void *pPad);
    virtual void JuiceScreenshot(char *pFileName);
    virtual int NumberOfRepeatedReplayEntries(int iEntry);
    virtual int GetJuiceThreadPriority();
    virtual void *GetJuiceUpdateProcessor();
    virtual void PauseGame();
    virtual void UnPauseGame();
    virtual ~IExtension();

    static IExtension *sCurrentExtension;
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
    int m_iNextArgType;
    int m_iRemainingArgs;
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
    template <class T, class R> void RegisterMethod(const char *pName, R (T::*method)(VarArgs &)) const {
        R (ICommands::*converted)(VarArgs &) = (R (ICommands::*)(VarArgs &))method;
        SendMethod(0, this, pName, 8, &converted);
    }
    template <class T, class R>
    void RegisterMethodWithID(int iID, const char *pName, R (T::*method)(VarArgs &)) const {
        R (ICommands::*converted)(VarArgs &) = (R (ICommands::*)(VarArgs &))method;
        SendMethod(iID, this, pName, 8, &converted);
    }
};

class IController : public ICommands {
public:
    IController();
    virtual ~IController();
    virtual void AutoRegister() const;
    virtual int DpadDown(VarArgs &args) = 0;
    virtual int DpadUp(VarArgs &args) = 0;
    virtual int DpadLeft(VarArgs &args) = 0;
    virtual int DpadRight(VarArgs &args) = 0;
    virtual int LeftAnalog(VarArgs &args) = 0;
    virtual int RightAnalog(VarArgs &args) = 0;
    virtual int Button1(VarArgs &args) = 0;
    virtual int Button2(VarArgs &args) = 0;
    virtual int Button3(VarArgs &args) = 0;
    virtual int Button4(VarArgs &args) = 0;
    virtual int ButtonL1(VarArgs &args) = 0;
    virtual int ButtonL2(VarArgs &args) = 0;
    virtual int ButtonL3(VarArgs &args) = 0;
    virtual int ButtonR1(VarArgs &args) = 0;
    virtual int ButtonR2(VarArgs &args) = 0;
    virtual int ButtonR3(VarArgs &args) = 0;
    virtual int ButtonStart(VarArgs &args) = 0;
    virtual int ButtonSelect(VarArgs &args) = 0;
    virtual int JuiceButtonPress(VarArgs &args) = 0;
};

class Controller : public IController {
public:
    Controller();
    virtual const char *GetNamespaceName() const;
    virtual int DpadDown(VarArgs &args);
    virtual int DpadUp(VarArgs &args);
    virtual int DpadLeft(VarArgs &args);
    virtual int DpadRight(VarArgs &args);
    virtual int LeftAnalog(VarArgs &args);
    virtual int RightAnalog(VarArgs &args);
    virtual int Button1(VarArgs &args);
    virtual int Button2(VarArgs &args);
    virtual int Button3(VarArgs &args);
    virtual int Button4(VarArgs &args);
    virtual int ButtonL1(VarArgs &args);
    virtual int ButtonL2(VarArgs &args);
    virtual int ButtonL3(VarArgs &args);
    virtual int ButtonR1(VarArgs &args);
    virtual int ButtonR2(VarArgs &args);
    virtual int ButtonR3(VarArgs &args);
    virtual int ButtonStart(VarArgs &args);
    virtual int ButtonSelect(VarArgs &args);
    virtual int JuiceButtonPress(VarArgs &args);
    int JuiceMonkey(VarArgs &args);
};

class Commands : public ICommands {
public:
    Commands();
    virtual const char *GetNamespaceName() const;
    char *GetScreenName(VarArgs &args);
    char *GetXboxDebugIP(VarArgs &args);
    int SetScriptButtonDelta(VarArgs &args);
    int StopScriptButtonDelta(VarArgs &args);
    int EnableAutomationCrashReboot(VarArgs &args);
    int DisableAutomationCrashReboot(VarArgs &args);
};

class Debug : public ICommands {
public:
    Debug();
    virtual const char *GetNamespaceName() const;
    int Hang(VarArgs &args);
    int InstantCrash(VarArgs &args);
    int InstantAssert(VarArgs &args);
    int TestRegisterAsyncEvent(VarArgs &args);
    int TestUnregisterAsyncEvent(VarArgs &args);
    int TestTriggerAsyncEvent(VarArgs &args);
    int TestEvent(VarArgs &args);
    int TestLogStat(VarArgs &args);
    int TestLogGameStat(VarArgs &args);
    int TestLogGlobalStat(VarArgs &args);
    int TestAdvanceGameCycle(VarArgs &args);
    int TestAdvanceGlobalCycle(VarArgs &args);
    int TestResetStatCycle(VarArgs &args);
    int TestAdvanceStatLevel(VarArgs &args);
};

Controller::Controller() {
    IController::AutoRegister();
    RegisterMethod("JuiceMonkey", &Controller::JuiceMonkey);
}

const char *Controller::GetNamespaceName() const {
    return "Ctrl";
}

int Controller::DpadDown(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)2, fValue);
    return 1;
}

int Controller::DpadUp(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)1, fValue);
    return 1;
}

int Controller::DpadLeft(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)3, fValue);
    return 1;
}

int Controller::DpadRight(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)4, fValue);
    return 1;
}

int Controller::LeftAnalog(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)11, fValue);
    return 0;
}

int Controller::RightAnalog(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)14, fValue);
    return 0;
}

int Controller::Button1(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)5, fValue);
    return 1;
}

int Controller::Button2(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)6, fValue);
    return 1;
}

int Controller::Button3(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)7, fValue);
    return 1;
}

int Controller::Button4(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)8, fValue);
    return 1;
}

int Controller::ButtonL1(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)9, fValue);
    return 1;
}

int Controller::ButtonR1(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)12, fValue);
    return 1;
}

int Controller::ButtonL2(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)10, fValue);
    return 1;
}

int Controller::ButtonR2(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)13, fValue);
    return 1;
}

int Controller::ButtonL3(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)11, fValue);
    return 1;
}

int Controller::ButtonR3(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)14, fValue);
    return 1;
}

int Controller::ButtonStart(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)15, fValue);
    return 1;
}

int Controller::ButtonSelect(VarArgs &args) {
    float fValue = 255.0f;

    args.GetFloat(fValue);
    JuicePad::Instance()->SetScriptButton((eJuiceButtonNames)16, fValue);
    return 1;
}

int Controller::JuiceButtonPress(VarArgs &args) {
    int iMask = 0;

    args.GetInt(iMask);
    JuicePad::Instance()->SetScriptButtonMask(iMask);
    return 1;
}

int Controller::JuiceMonkey(VarArgs &args) {
    char buffer[256];
    int iResult = 1;

    if (args.GetString(buffer)) {
    if (!strcasecmp(buffer, "random")) {
        Juice::JuiceMonkey::Instance()->SetMonkeyState((eMonkeyState)0);
    } else if (!strcasecmp(buffer, "sequential")) {
        Juice::JuiceMonkey::Instance()->SetMonkeyState((eMonkeyState)1);
    } else if (!strcasecmp(buffer, "disable")) {
        Juice::JuiceMonkey::Instance()->ActivatePadMonkey(false);
    } else if (!strcasecmp(buffer, "enable")) {
        Juice::JuiceMonkey::Instance()->ActivatePadMonkey(true);
    } else {
        iResult = 0;
    }
    }
    return iResult;
}

#define JUICE_REGISTER_COMMANDS_METHOD(name) RegisterMethod(#name, &Commands::name);

Commands::Commands() {
    RegisterMethodWithID(2, "GetScreenName", &Commands::GetScreenName);
    RegisterMethodWithID(2, "GetXboxDebugIP", &Commands::GetXboxDebugIP);
    JUICE_REGISTER_COMMANDS_METHOD(SetScriptButtonDelta)
    JUICE_REGISTER_COMMANDS_METHOD(StopScriptButtonDelta)
    JUICE_REGISTER_COMMANDS_METHOD(EnableAutomationCrashReboot)
    JUICE_REGISTER_COMMANDS_METHOD(DisableAutomationCrashReboot)
}

const char *Commands::GetNamespaceName() const {
    return "Juice";
}

char *Commands::GetScreenName(VarArgs &args) {
    return IExtension::sCurrentExtension->GetScreenName();
}

char *Commands::GetXboxDebugIP(VarArgs &args) {
    return (char *)Util::GetXboxDebugIP();
}

int Commands::SetScriptButtonDelta(VarArgs &args) {
    int iDelta;

    if (args.GetInt(iDelta)) {
        JuicePad::Instance()->SetScriptButtonDelta(iDelta);
        return 1;
    }
    return 0;
}

int Commands::StopScriptButtonDelta(VarArgs &args) {
    JuicePad::Instance()->StopScriptButtonDelta();
    return 1;
}

int Commands::EnableAutomationCrashReboot(VarArgs &args) {
    SystemManager::Instance()->SetAutomationCrashOption(true);
    return 1;
}

int Commands::DisableAutomationCrashReboot(VarArgs &args) {
    SystemManager::Instance()->SetAutomationCrashOption(false);
    return 1;
}

#define JUICE_REGISTER_DEBUG_METHOD(name) RegisterMethod(#name, &Debug::name);

Debug::Debug() {
    JUICE_REGISTER_DEBUG_METHOD(Hang)
    JUICE_REGISTER_DEBUG_METHOD(InstantCrash)
    JUICE_REGISTER_DEBUG_METHOD(InstantAssert)
    JUICE_REGISTER_DEBUG_METHOD(TestRegisterAsyncEvent)
    JUICE_REGISTER_DEBUG_METHOD(TestUnregisterAsyncEvent)
    JUICE_REGISTER_DEBUG_METHOD(TestTriggerAsyncEvent)
    JUICE_REGISTER_DEBUG_METHOD(TestEvent)
    JUICE_REGISTER_DEBUG_METHOD(TestLogStat)
    JUICE_REGISTER_DEBUG_METHOD(TestLogGameStat)
    JUICE_REGISTER_DEBUG_METHOD(TestLogGlobalStat)
    JUICE_REGISTER_DEBUG_METHOD(TestAdvanceGameCycle)
    JUICE_REGISTER_DEBUG_METHOD(TestAdvanceGlobalCycle)
    JUICE_REGISTER_DEBUG_METHOD(TestResetStatCycle)
    JUICE_REGISTER_DEBUG_METHOD(TestAdvanceStatLevel)
}

const char *Debug::GetNamespaceName() const {
    return "Debug";
}

int Debug::InstantCrash(VarArgs &args) {
    int *pNull = 0;

    *pNull = 0;
    return *pNull;
}

int Debug::Hang(VarArgs &args) {
    ThreadManager::Instance()->InflictHang();
    return 0;
}

int Debug::InstantAssert(VarArgs &args) {
    GameHook::Instance()->AssertLog(1, "InstantAssert", "JuiceDefaultScripting.cpp", "false", "triggered by rpc");
    return 0;
}

int Debug::TestRegisterAsyncEvent(VarArgs &args) {
    char buffer1[256];
    char buffer2[256];

    if (args.GetString(buffer1)) {
        if (args.GetString(buffer2)) {
            RegisterAsyncEvent(buffer1, buffer2);
            return 1;
        }
    }
    return 0;
}

int Debug::TestUnregisterAsyncEvent(VarArgs &args) {
    char buffer[256];

    if (!args.GetString(buffer)) {
        return 0;
    }
    UnregisterAsyncEvent(buffer);
    return 1;
}

int Debug::TestTriggerAsyncEvent(VarArgs &args) {
    char buffer1[256];
    char buffer2[256];

    if (args.GetString(buffer1)) {
        if (args.GetString(buffer2)) {
            SendAsyncEvent(buffer1, buffer2);
            return 1;
        }
    }
    return 0;
}

int Debug::TestEvent(VarArgs &args) {
    char buffer1[256];
    char buffer2[256];
    char buffer3[256];
    char buffer4[256];
    char buffer5[256];
    char buffer6[256];
    int iValue;
    float fValue;

    if (args.GetString(buffer1)) {
        if (args.GetString(buffer2)) {
            if (args.GetString(buffer3)) {
                if (args.GetInt(iValue)) {
                    if (args.GetFloat(fValue)) {
                        if (args.GetString(buffer4)) {
                            if (args.GetString(buffer5)) {
                                if (args.GetString(buffer6)) {
                                    GameHook::Instance()->GameEvent(buffer1, buffer2, buffer3, iValue, fValue,
                                                                      buffer4, buffer5, buffer6);
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int Debug::TestLogStat(VarArgs &args) {
    char buffer[256];
    int iValue;

    if (args.GetString(buffer)) {
        if (args.GetInt(iValue)) {
            GameHook::Instance()->LogStat(4, "Unknown", buffer, iValue);
            return 1;
        }
    }
    return 0;
}

int Debug::TestLogGameStat(VarArgs &args) {
    char buffer1[256];
    char buffer2[256];
    int iLevel;
    int iValue;

    if (args.GetInt(iLevel)) {
        if (args.GetString(buffer1)) {
            if (args.GetString(buffer2)) {
                if (args.GetInt(iValue)) {
    switch (iLevel) {
    case 1:
        GameHook::Instance()->LogStat(1, buffer1, buffer2, iValue);
        break;
    case 2:
        GameHook::Instance()->LogStat(2, buffer1, buffer2, iValue);
        break;
    case 3:
        GameHook::Instance()->LogStat(3, buffer1, buffer2, iValue);
        break;
    case 4:
        GameHook::Instance()->LogStat(4, buffer1, buffer2, iValue);
        break;
    case 5:
        GameHook::Instance()->LogStat(5, buffer1, buffer2, iValue);
        break;
    case 6:
        GameHook::Instance()->LogStat(6, buffer1, buffer2, iValue);
        break;
    case 7:
        GameHook::Instance()->LogStat(7, buffer1, buffer2, iValue);
        break;
    case 8:
        GameHook::Instance()->LogStat(8, buffer1, buffer2, iValue);
        break;
    case 9:
        GameHook::Instance()->LogStat(9, buffer1, buffer2, iValue);
        break;
    default:
        GameHook::Instance()->AssertLog(432, "TestLogGameStat",
                                    "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceDefaultScripting.cpp",
                                    "Invalid Level", "Error");
        break;
    }
    return 1;
                }
            }
        }
    }
    return 0;
}

int Debug::TestLogGlobalStat(VarArgs &args) {
    char buffer1[256];
    char buffer2[256];
    int iLevel;
    int iValue;

    if (args.GetInt(iLevel)) {
        if (args.GetString(buffer1)) {
            if (args.GetString(buffer2)) {
                if (args.GetInt(iValue)) {
    switch (iLevel) {
    case 1:
        GameHook::Instance()->LogStat(1, buffer1, buffer2, iValue);
        break;
    case 2:
        GameHook::Instance()->LogStat(2, buffer1, buffer2, iValue);
        break;
    case 3:
        GameHook::Instance()->LogStat(3, buffer1, buffer2, iValue);
        break;
    case 4:
        GameHook::Instance()->LogStat(4, buffer1, buffer2, iValue);
        break;
    case 5:
        GameHook::Instance()->LogStat(5, buffer1, buffer2, iValue);
        break;
    case 6:
        GameHook::Instance()->LogStat(6, buffer1, buffer2, iValue);
        break;
    case 7:
        GameHook::Instance()->LogStat(7, buffer1, buffer2, iValue);
        break;
    case 8:
        GameHook::Instance()->LogStat(8, buffer1, buffer2, iValue);
        break;
    case 9:
        GameHook::Instance()->LogStat(9, buffer1, buffer2, iValue);
        break;
    default:
        GameHook::Instance()->AssertLog(480, "TestLogGlobalStat",
                                    "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceDefaultScripting.cpp",
                                    "Invalid Level", "Error");
        break;
    }
    return 1;
                }
            }
        }
    }
    return 0;
}

int Debug::TestAdvanceGameCycle(VarArgs &args) {
    int iLevel;

    if (args.GetInt(iLevel)) {
    switch (iLevel) {
    case 1:
        GameHook::Instance()->AdvanceCycle(1);
        break;
    case 2:
        GameHook::Instance()->AdvanceCycle(2);
        break;
    case 3:
        GameHook::Instance()->AdvanceCycle(3);
        break;
    case 4:
        GameHook::Instance()->AdvanceCycle(4);
        break;
    case 5:
        GameHook::Instance()->AdvanceCycle(5);
        break;
    case 6:
        GameHook::Instance()->AdvanceCycle(6);
        break;
    case 7:
        GameHook::Instance()->AdvanceCycle(7);
        break;
    case 8:
        GameHook::Instance()->AdvanceCycle(8);
        break;
    case 9:
        GameHook::Instance()->AdvanceCycle(9);
        break;
    default:
        GameHook::Instance()->AssertLog(525, "TestAdvanceGameCycle",
                                        "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceDefaultScripting.cpp",
                                        "Invalid Level", "Error");
        break;
    }
    return 1;
    }
    return 0;
}

int Debug::TestAdvanceGlobalCycle(VarArgs &args) {
    int iLevel;

    if (args.GetInt(iLevel)) {
    switch (iLevel) {
    case 1:
        GameHook::Instance()->AdvanceCycle(1);
        break;
    case 2:
        GameHook::Instance()->AdvanceCycle(2);
        break;
    case 3:
        GameHook::Instance()->AdvanceCycle(3);
        break;
    case 4:
        GameHook::Instance()->AdvanceCycle(4);
        break;
    case 5:
        GameHook::Instance()->AdvanceCycle(5);
        break;
    case 6:
        GameHook::Instance()->AdvanceCycle(6);
        break;
    case 7:
        GameHook::Instance()->AdvanceCycle(7);
        break;
    case 8:
        GameHook::Instance()->AdvanceCycle(8);
        break;
    case 9:
        GameHook::Instance()->AdvanceCycle(9);
        break;
    default:
        GameHook::Instance()->AssertLog(577, "TestAdvanceGlobalCycle",
                                    "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceDefaultScripting.cpp",
                                    "Invalid Level", "Error");
        break;
    }
    return 1;
    }
    return 0;
}

int Debug::TestResetStatCycle(VarArgs &args) {
    int iLevel;

    if (args.GetInt(iLevel)) {
    switch (iLevel) {
    case 1:
        GameHook::Instance()->ResetCycle(1);
        break;
    case 2:
        GameHook::Instance()->ResetCycle(2);
        break;
    case 3:
        GameHook::Instance()->ResetCycle(3);
        break;
    case 4:
        GameHook::Instance()->ResetCycle(4);
        break;
    case 5:
        GameHook::Instance()->ResetCycle(5);
        break;
    case 6:
        GameHook::Instance()->ResetCycle(6);
        break;
    case 7:
        GameHook::Instance()->ResetCycle(7);
        break;
    case 8:
        GameHook::Instance()->ResetCycle(8);
        break;
    case 9:
        GameHook::Instance()->ResetCycle(9);
        break;
    default:
        GameHook::Instance()->AssertLog(623, "TestResetStatCycle",
                                    "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceDefaultScripting.cpp",
                                    "Invalid Level", "Error");
        break;
    }
    return 1;
    }
    return 0;
}

int Debug::TestAdvanceStatLevel(VarArgs &args) {
    GameHook::Instance()->AdvanceCycle(4);
    return 1;
}

}
}
