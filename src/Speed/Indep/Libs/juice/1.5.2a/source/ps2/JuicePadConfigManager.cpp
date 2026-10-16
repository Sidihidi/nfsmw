// Juice 1.5.2a: JuicePadConfigManager.cpp, configuracion del mando desde
// el servidor (mono, mascara y frecuencia de botones, captura de
// navegacion FE y autoscript). Escrito del asm de la A124
// (JuicePadConfigManager.o: .text 0x4B5CC0, 12 etiquetas). Unidad solo
// enlaza en A124. Hereda de Processor (vptr a +0x0C, ABI viejo); Instance
// deja las banderas heredadas a 1 en cada llamada. Los casos del switch
// van en el orden en que los emite el original (852, 853, 851, ...). La
// IExtension se declara con su tabla completa porque SendFEExtension-
// MessageBody y CaptureKeyAndState llaman a sus virtuales de pantalla.

namespace Juice {

enum Command {
};
enum Channels {
};
enum eJuiceButtonNames {
};
enum eMonkeyState {
};

class cJuiceCom {
public:
    void NewMsg(Command cmd, Channels channel);
    void EndMsg(Channels channel);
    void Send(Channels channel);
    void PutInt(int i, Channels channel);
    void PutString(const char *pString, Channels channel);
    int GetInt(Channels channel);
};

class GameHook {
public:
    static GameHook *(*Instance)();

    int m_bConnected;
    int m_iFrameCount;
    int m_iUnused8;
    int m_iUnused12;
    int m_iUnused16;
    cJuiceCom *m_pJuiceCom;
};

class SystemManager {
public:
    static unsigned int GetCurrentSystemTime();
    static unsigned int GetTimeElapsed(unsigned int &iLastTime);
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

class JuicePad {
public:
    static JuicePad *Instance();
    void DisableButton(eJuiceButtonNames eButton);
    void EnableButton(eJuiceButtonNames eButton);
    void SetButtonMask(unsigned int iMask);
};

class JuiceMonkey {
public:
    static JuiceMonkey *Instance();
    void SetButtonFrequency(eJuiceButtonNames eButton, unsigned int iFrequency);
    void ActivatePadMonkey(bool bActivate);
    void SetMonkeyState(eMonkeyState eState);
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

class PadConfigManager : public Processor {
public:
    PadConfigManager();
    static PadConfigManager *Instance();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();
    void SendFEExtensionMessageBody();
    void CaptureKeyAndState(unsigned int iKey);
    void SendAutoScriptCommand(int iCommand);
    void ActivateAutoScript(bool bActivate);
    int IsFENavCapturing();

private:
    int m_bAutoScriptActive; // +0x10
    int m_bFENavCapturing;   // +0x14
    int m_iUnused24;         // +0x18
    int m_iUnused28;         // +0x1C
};

PadConfigManager::PadConfigManager() {
    m_bActive = 0;
    m_bEnabled = 0;
    m_bAutoScriptActive = 0;
    m_bFENavCapturing = 0;
    m_iUnused24 = 0;
}

PadConfigManager *PadConfigManager::Instance() {
    static PadConfigManager padConfigManagerInstance;

    padConfigManagerInstance.m_bEnabled = 1;
    padConfigManagerInstance.m_bActive = 1;
    return &padConfigManagerInstance;
}

void PadConfigManager::ProcessMessage(int iMsg) {
    switch (iMsg) {
    case 852: {
        eMonkeyState eState = (eMonkeyState)m_pCom->GetInt((Channels)0);

        JuiceMonkey::Instance()->SetMonkeyState(eState);
        break;
    }
    case 853: {
        eJuiceButtonNames eButton = (eJuiceButtonNames)m_pCom->GetInt((Channels)0);
        int iEnable = m_pCom->GetInt((Channels)0);

        if (iEnable == 0) {
            JuicePad::Instance()->DisableButton(eButton);
        } else if (iEnable == 1) {
            JuicePad::Instance()->EnableButton(eButton);
        }
        break;
    }
    case 851: {
        eJuiceButtonNames eButton = (eJuiceButtonNames)m_pCom->GetInt((Channels)0);
        unsigned int iFrequency = (unsigned int)m_pCom->GetInt((Channels)0);

        JuiceMonkey::Instance()->SetButtonFrequency(eButton, iFrequency);
        break;
    }
    case 857:
        if (m_pCom->GetInt((Channels)0) == 1) {
            JuiceMonkey::Instance()->ActivatePadMonkey(true);
        } else {
            JuiceMonkey::Instance()->ActivatePadMonkey(false);
        }
        break;
    case 858:
        if (m_pCom->GetInt((Channels)0) == 1) {
            ActivateAutoScript(true);
        } else {
            ActivateAutoScript(false);
        }
        break;
    case 859:
        m_bFENavCapturing = 1;
        m_pCom->NewMsg((Command)804, (Channels)0);
        SendFEExtensionMessageBody();
        m_pCom->EndMsg((Channels)0);
        m_pCom->Send((Channels)0);
        break;
    case 860:
        m_bFENavCapturing = 0;
        m_pCom->NewMsg((Command)806, (Channels)0);
        SendFEExtensionMessageBody();
        m_pCom->EndMsg((Channels)0);
        m_pCom->Send((Channels)0);
        break;
    case 861:
        m_iUnused24 = 1;
        m_pCom->NewMsg((Command)807, (Channels)0);
        SendFEExtensionMessageBody();
        m_pCom->EndMsg((Channels)0);
        m_pCom->Send((Channels)0);
        break;
    case 863: {
        unsigned int iMask = (unsigned int)m_pCom->GetInt((Channels)0);

        JuicePad::Instance()->SetButtonMask(iMask);
        m_pCom->NewMsg((Command)864, (Channels)0);
        SendFEExtensionMessageBody();
        m_pCom->EndMsg((Channels)0);
        m_pCom->Send((Channels)0);
        break;
    }
    case 862:
        m_iUnused24 = 0;
        m_pCom->NewMsg((Command)810, (Channels)0);
        m_pCom->PutInt(4, (Channels)0);
        m_pCom->PutString("GetScreenName", (Channels)0);
        m_pCom->PutString("GetCursorPos", (Channels)0);
        m_pCom->PutString("GetPopupName", (Channels)0);
        m_pCom->PutString("GetPopupCheck", (Channels)0);
        m_pCom->EndMsg((Channels)0);
        m_pCom->Send((Channels)0);
        break;
    }
}

void PadConfigManager::SendFEExtensionMessageBody() {
    m_pCom->PutInt(4, (Channels)0);
    m_pCom->PutString("GetScreenName", (Channels)0);
    m_pCom->PutString("GetCursorPos", (Channels)0);
    m_pCom->PutString("GetPopupName", (Channels)0);
    m_pCom->PutString("GetPopupCheck", (Channels)0);
    m_pCom->PutString(IExtension::sCurrentExtension->GetScreenName(), (Channels)0);
    m_pCom->PutString(IExtension::sCurrentExtension->GetCursorPos(), (Channels)0);
    m_pCom->PutString(IExtension::sCurrentExtension->GetPopupName(), (Channels)0);
    m_pCom->PutString(IExtension::sCurrentExtension->GetPopupCheck(), (Channels)0);
}

void PadConfigManager::CaptureKeyAndState(unsigned int iKey) {
    m_pCom->NewMsg((Command)805, (Channels)0);
    m_pCom->PutInt(4, (Channels)0);
    m_pCom->PutString(IExtension::sCurrentExtension->GetScreenName(), (Channels)0);
    m_pCom->PutString(IExtension::sCurrentExtension->GetCursorPos(), (Channels)0);
    m_pCom->PutString(IExtension::sCurrentExtension->GetPopupName(), (Channels)0);
    m_pCom->PutString(IExtension::sCurrentExtension->GetPopupCheck(), (Channels)0);
    m_pCom->PutInt((int)iKey, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

void PadConfigManager::SendAutoScriptCommand(int iCommand) {
    static int iLastFrame;
    static unsigned int iLastTime;
    int iFrame;
    int iFrameDiff;
    unsigned int iElapsed;

    if (m_bAutoScriptActive) {
        iFrame = GameHook::Instance()->m_iFrameCount;
        iFrameDiff = iFrame - iLastFrame;
        if (iCommand > 0 && iFrameDiff >= 15) {
            iElapsed = SystemManager::GetTimeElapsed(iLastTime);
            iLastTime = SystemManager::GetCurrentSystemTime();
            iLastFrame = iFrame;
            m_pCom->NewMsg((Command)854, (Channels)0);
            m_pCom->PutInt(iCommand, (Channels)0);
            m_pCom->PutInt((int)iElapsed, (Channels)0);
            m_pCom->EndMsg((Channels)0);
            m_pCom->Send((Channels)0);
        }
    }
}

void PadConfigManager::Initialize(cJuiceCom *pCom) {
    Processor::Initialize(pCom);
    m_pCom = pCom;
    m_pCom->NewMsg((Command)856, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

void PadConfigManager::ActivateAutoScript(bool bActivate) {
    m_bAutoScriptActive = bActivate;
}

int PadConfigManager::IsFENavCapturing() {
    return m_bFENavCapturing;
}

void PadConfigManager::Update() {
}

void PadConfigManager::Disable() {
}

}
