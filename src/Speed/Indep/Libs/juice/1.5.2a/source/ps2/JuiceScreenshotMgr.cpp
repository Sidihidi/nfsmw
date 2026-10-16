// Juice 1.5.2a: JuiceScreenshotMgr.cpp, el gestor de capturas de pantalla.
// Escrito del asm de la A124 (JuiceScreenshotMgr.o: .text 0x4B84B0, 8
// etiquetas). Unidad solo enlaza en A124. Hereda de Processor (vptr a
// +0x0C, ABI viejo) y no anade miembros: las dos banderas heredadas de
// +4/+8 hacen de interruptores de los comandos 1101/1102, y el 1105 lee
// el nombre de fichero del cJuiceCom y pide la captura a la IExtension en
// curso (declarada con su tabla virtual completa, 41 slots, porque la
// llamada a JuiceScreenshot sale por el slot de +0x11C).

#include <stdio.h>

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
    char *GetString(Channels channel);
};

class DebugSystem {
public:
    static void WarningOnMsg(int iLine, const char *pFunction, const char *pFile, int iMsg, const char *pMessage);
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

class Processor {
public:
    Processor();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable() = 0;
    virtual void ProcessMessage(int iMsg) = 0;
    virtual void Update() = 0;

protected:
    cJuiceCom *m_pCom;
    int m_bEnabled;
    int m_bActive;
};

class JuiceScreenshotMgr : public Processor {
public:
    JuiceScreenshotMgr();
    static JuiceScreenshotMgr *Instance();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();
    void SendScreenshotCommand(Command cmd);
};

JuiceScreenshotMgr::JuiceScreenshotMgr() {
    m_bActive = 0;
    m_bEnabled = 0;
}

JuiceScreenshotMgr *JuiceScreenshotMgr::Instance() {
    static JuiceScreenshotMgr juiceScreenshotMgrInstance;
    return &juiceScreenshotMgrInstance;
}

void JuiceScreenshotMgr::ProcessMessage(int iMsg) {
    char buffer[64];

    switch (iMsg) {
    case 1101:
        m_bActive = 1;
        m_bEnabled = 1;
        break;
    case 1102:
        m_bEnabled = 1;
        m_bActive = 0;
        break;
    case 1105:
        sprintf(buffer, m_pCom->GetString((Channels)0));
        IExtension::sCurrentExtension->JuiceScreenshot(buffer);
        break;
    default:
        DebugSystem::WarningOnMsg(82, "void Juice::JuiceScreenshotMgr::ProcessMessage(int)",
                                   "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceScreenshotMgr.cpp", iMsg,
                                   "Invalid command %d was sent to the Screenshot Manager.");
        break;
    }
}

void JuiceScreenshotMgr::Initialize(cJuiceCom *pCom) {
    Processor::Initialize(pCom);
    SendScreenshotCommand((Command)1103);
}

void JuiceScreenshotMgr::Disable() {
}

void JuiceScreenshotMgr::SendScreenshotCommand(Command cmd) {
    m_pCom->NewMsg(cmd, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

void JuiceScreenshotMgr::Update() {
}

}
