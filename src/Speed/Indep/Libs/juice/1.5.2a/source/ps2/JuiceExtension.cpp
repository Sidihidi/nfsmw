// Juice 1.5.2a: JuiceExtension.cpp, los valores por defecto de IExtension
// (la interfaz que implementa el juego en MWExtension) y su UpdatePad, que
// cruza el mando del juego con el tJuicePad, aplica el replay a los
// botones y captura teclas para la navegacion FE como mucho cada 15
// frames. Escrito del asm de la A124 (JuiceExtension.o: .text 0x4B4760,
// 30 etiquetas). Unidad solo enlaza en A124. La clase se declara con sus
// 41 virtuales en orden (los 14 primeros puros) porque su tabla y las
// llamadas por slot de otros modulos dependen de el; GetBuildDate
// devuelve __DATE__.

int ShouldWeUseThisButton(int iButton, int iMask);

namespace Juice {

enum Command {
};
enum Channels {
};

struct tJuicePad {
    int m_iButtonMask;
    float m_fButtons[32];
};

class cJuiceCom;

class GameHook {
public:
    static GameHook *(*Instance)();
    unsigned int ReplayData(unsigned int &iData, int iSize, const char *pName, int iChannel);
    float ReplayData(float &fData, int iSize, const char *pName, int iChannel);

    int m_bConnected;
    int m_iFrameCount;
    int m_iUnused8;
    int m_iUnused12;
    int m_iUnused16;
    cJuiceCom *m_pJuiceCom;
};

class JuicePad {
public:
    static JuicePad *Instance();
    tJuicePad *GetMasterPad();
    void UpdateJuicePad();
    int DidInputHappen();
    void ResetButtons();
};

class ReplayManager {
public:
    static ReplayManager *Instance();
    int IsReplayEnabled();
};

class PadConfigManager {
public:
    static PadConfigManager *Instance();
    int IsFENavCapturing();
    void CaptureKeyAndState(unsigned int iKey);
};

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

IExtension *IExtension::sCurrentExtension;

IExtension::IExtension() {
    sCurrentExtension = this;
}

int IExtension::InitialConnectionRetries() {
    return 100;
}

void IExtension::UpdatePad(void *pGamePad) {
    static int iLastFrame;
    tJuicePad *pPad = JuicePad::Instance()->GetMasterPad();

    sCurrentExtension->GamePadToJuicePad(pGamePad, pPad);
    JuicePad::Instance()->UpdateJuicePad();
    if (ReplayManager::Instance()->IsReplayEnabled()) {
        unsigned int iMask = pPad->m_iButtonMask;
        unsigned int iReplayMask = GameHook::Instance()->ReplayData((unsigned int &)pPad->m_iButtonMask, 3, "", 0);
        int iButton;

        pPad->m_iButtonMask = iReplayMask | iMask;
        for (iButton = 1; iButton < 21; iButton++) {
            if (ShouldWeUseThisButton(iButton, (int)iReplayMask) == 1) {
                pPad->m_fButtons[iButton] =
                    GameHook::Instance()->ReplayData(pPad->m_fButtons[iButton], 3, "", 0);
            }
        }
    }
    sCurrentExtension->JuicePadToGamePad(pPad, pGamePad);
    if (PadConfigManager::Instance()->IsFENavCapturing()) {
        if (JuicePad::Instance()->DidInputHappen() == 1) {
            int iFrame = GameHook::Instance()->m_iFrameCount;
            int iDiff = iFrame - iLastFrame;

            if (pPad->m_iButtonMask != 0 && iDiff >= 15) {
                iLastFrame = iFrame;
                PadConfigManager::Instance()->CaptureKeyAndState(pPad->m_iButtonMask);
            }
        }
    }
    JuicePad::Instance()->ResetButtons();
}

IExtension::~IExtension() {
    sCurrentExtension = 0;
}

void IExtension::UnPauseGame() {
}

void IExtension::PauseGame() {
}

void *IExtension::GetJuiceUpdateProcessor() {
    return 0;
}

int IExtension::GetJuiceThreadPriority() {
    return 8;
}

int IExtension::NumberOfRepeatedReplayEntries(int iEntry) {
    return 0;
}

void IExtension::JuiceScreenshot(char *pFileName) {
}

int IExtension::HangTimeoutLength() {
    return 20000;
}

void IExtension::ResetGamePad() {
}

void IExtension::InstantiateScriptExt() {
}

int IExtension::IsOkToConnect() {
    return 1;
}

float IExtension::GetGameTime() {
    return 0.0f;
}

char *IExtension::GetCursorPos() {
    return "null";
}

char *IExtension::GetPopupCheck() {
    return "false";
}

char *IExtension::GetPopupName() {
    return "null";
}

char *IExtension::GetScreenName() {
    return "null";
}

void IExtension::ThreadYield(int iTime) {
}

int IExtension::WaitBetweenNetworkInitRetries() {
    return 100;
}

int IExtension::WaitBetweenInitialConnectionRretries() {
    return 100;
}

int IExtension::NetworkInitRetries() {
    return 17000;
}

int IExtension::GetBackupServerPort() {
    return 0;
}

char *IExtension::GetBackupServerIP() {
    return "127.0.0.1";
}

int IExtension::GetHeartBeatTimeOut() {
    return 20000;
}

int IExtension::GetInitializationTimeOut() {
    return 30000;
}

char *IExtension::GetBuildDate() {
    return __DATE__;
}

}

int ShouldWeUseThisButton(int iButton, int iMask) {
    return (iMask >> (iButton - 1)) & 1;
}
