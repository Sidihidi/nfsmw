// Juice 1.5.2a: JuiceSystemManager.cpp, el gestor de sistema: se presenta
// al servidor con los datos de la IExtension, pausa/reanuda, redirige la
// conexion, reinicia la consola leyendo el SYSTEM.CNF y mantiene el
// latido (comando 3) y la hora del sistema desde el Count de la EE.
// Escrito del asm de la A124 (JuiceSystemManager.o: .text 0x4BA668, 20
// etiquetas). Unidad solo enlaza en A124. Hereda de Processor (vptr a
// +0x0C, ABI viejo). Casan 18 de 22: GetCurrentSystemTime queda sin
// definir porque el original lee el Count con mfc0 y no hay forma
// C/intrinseco en el SDK que lo emita (verificado en cabeceras de
// eekernel.h y del compilador); ResetConsole queda al 90,6 % por la
// asignacion de registros de sus bucles de barrido.

#include <stdio.h>
#include <string.h>

#include "eekernel.h"
#include "libcdvd.h"
#include "libdma.h"
#include "libpad.h"
#include "sifdev.h"

extern "C" int sceCdSearchFile(void *pFile, const char *pName);

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
    void PutString(const char *pString, Channels channel);
    int GetInt(Channels channel);
    char *GetString(Channels channel);
    char *GetMacAddress();
    int Connect(const char *pHost, int iPort);
    int Disconnect();
    int IsConnected() const;
};

class GameHook {
public:
    static GameHook *(*Instance)();
    void DisableJuice();

    int m_bConnected;
    int m_iFrameCount;
    int m_iUnused8;
    int m_iUnused12;
    int m_iUnused16;
    cJuiceCom *m_pJuiceCom;
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

class DebugSystem {
public:
    static void WarningOnMsg(int iLine, const char *pFunction, const char *pFile, int iMsg, const char *pMessage);
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

class SystemManager : public Processor {
public:
    SystemManager();
    static SystemManager *Instance();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();
    void RegisterWithServer(int iID, cJuiceCom *pCom);
    int IsRedirecting();
    int IsGamePaused();
    int InAutomationMode();
    int AutomationCrashReboot();
    void SetAutomationCrashOption(bool bOption);
    void FilterCommands(Command &cmd);
    void SendFrameUpdate();
    void ResetConsole();
    void SendPauseAck(bool bPaused);
    static unsigned int GetCurrentSystemTime();
    static unsigned int GetTimeElapsed(unsigned int &iLastTime);
    static void SetFrequency();

    static float mSystemFrequencyScaler;
    static int mHasSetFreq;

private:
    bool m_bPaused;           // +0x10
    int m_iField20;           // +0x14
    int m_bInAutomationMode;  // +0x18
    int m_bAutomationCrash;   // +0x1C
    int m_iRedirecting;       // +0x20
    unsigned int m_iLastTime; // +0x24
    unsigned int m_iAccumulated; // +0x28
    int m_iHeartbeatReset;    // +0x2C
    int m_iHeartbeatCounter;  // +0x30
    int m_iField52;           // +0x34
    unsigned int m_iAccum2;   // +0x38
    unsigned int m_iTimeout;  // +0x3C
    unsigned int m_iTime2;    // +0x40
    int m_iUnused68;          // +0x44
};

float SystemManager::mSystemFrequencyScaler;
int SystemManager::mHasSetFreq;

SystemManager::SystemManager() {
    m_bPaused = 0;
    m_iField20 = 0;
    m_bInAutomationMode = 0;
    m_bAutomationCrash = 1;
    m_iRedirecting = 0;
    m_iHeartbeatReset = 30;
    m_iHeartbeatCounter = 30;
    m_iField52 = -1;
    m_iAccum2 = 0;
    m_iTimeout = 60000;
    m_iTime2 = GetCurrentSystemTime();
    m_bEnabled = 0;
}

SystemManager *SystemManager::Instance() {
    static SystemManager systemManagerInstance;
    return &systemManagerInstance;
}

void SystemManager::Initialize(cJuiceCom *pCom) {
    char *pTitleName;
    char *pBuildName;
    char *pBuildDate;
    char *pChangeList;
    char *pConsoleName;
    char *pMacAddress;
    char *pPlatform;

    m_iLastTime = GetCurrentSystemTime();
    m_pCom = pCom;
    m_iRedirecting = 0;
    pTitleName = IExtension::sCurrentExtension->GetTitleName();
    pBuildName = IExtension::sCurrentExtension->GetBuildName();
    pBuildDate = IExtension::sCurrentExtension->GetBuildDate();
    pChangeList = IExtension::sCurrentExtension->GetChangeList();
    pConsoleName = IExtension::sCurrentExtension->GetConsoleName();
    pMacAddress = m_pCom->GetMacAddress();
    pPlatform = "PS2";
    m_pCom->NewMsg((Command)6, (Channels)0);
    m_pCom->PutString("1.5.2", (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->NewMsg((Command)2, (Channels)0);
    m_pCom->PutString(pTitleName, (Channels)0);
    m_pCom->PutString(pBuildName, (Channels)0);
    m_pCom->PutString(pBuildDate, (Channels)0);
    m_pCom->PutString(pChangeList, (Channels)0);
    m_pCom->PutString(pMacAddress, (Channels)0);
    m_pCom->PutString(pConsoleName, (Channels)0);
    m_pCom->PutString(pPlatform, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

void SystemManager::ProcessMessage(int iMsg) {
    switch (iMsg) {
    case 52:
        m_iAccum2 = 0;
        m_bPaused = !m_bPaused;
        SendPauseAck(m_bPaused);
        if (m_bPaused) {
            IExtension::sCurrentExtension->PauseGame();
        } else {
            IExtension::sCurrentExtension->UnPauseGame();
        }
        break;
    case 3:
        m_iField20 = 1;
        break;
    case 54:
        GameHook::Instance()->DisableJuice();
        m_pCom->Disconnect();
        break;
    case 55:
        m_bInAutomationMode = 1;
        break;
    case 60: {
        int iValue = m_pCom->GetInt((Channels)0);

        m_iField52 = m_pCom->GetInt((Channels)0);
        m_iHeartbeatReset = iValue;
        m_iHeartbeatCounter = iValue;
        m_bEnabled = 1;
        break;
    }
    case 56:
        m_bInAutomationMode = 0;
        break;
    case 57:
        m_pCom->Disconnect();
        ResetConsole();
        break;
    case 8: {
        char buffer[256];
        int iPort;

        strcpy(buffer, m_pCom->GetString((Channels)0));
        iPort = m_pCom->GetInt((Channels)0);
        m_pCom->Disconnect();
        m_pCom->Connect(buffer, iPort);
        m_iRedirecting = 1;
        break;
    }
    case 1:
    case 53:
        break;
    default:
        DebugSystem::WarningOnMsg(180, "void Juice::SystemManager::ProcessMessage(int)",
                                   "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceSystemManager.cpp", iMsg,
                                   "Invalid command %d was sent to the System Manager.");
        break;
    }
}

void SystemManager::RegisterWithServer(int iID, cJuiceCom *pCom) {
    if (pCom->IsConnected()) {
        pCom->NewMsg((Command)1, (Channels)0);
        pCom->PutString(pCom->GetMacAddress(), (Channels)0);
        pCom->PutInt(iID, (Channels)0);
        pCom->EndMsg((Channels)0);
        pCom->Send((Channels)0);
    }
}

int SystemManager::IsRedirecting() {
    return m_iRedirecting;
}

int SystemManager::IsGamePaused() {
    return m_bPaused;
}

int SystemManager::InAutomationMode() {
    return m_bInAutomationMode;
}

int SystemManager::AutomationCrashReboot() {
    return m_bAutomationCrash;
}

void SystemManager::SetAutomationCrashOption(bool bOption) {
    m_bAutomationCrash = bOption;
}

void SystemManager::FilterCommands(Command &cmd) {
    if (m_bPaused) {
        switch ((int)cmd) {
        case 3:
        case 52:
        case 551:
        case 851:
        case 852:
        case 853:
        case 855:
        case 856:
        case 857:
        case 858:
        case 1104:
            break;
        default:
            cmd = (Command)53;
            break;
        }
    }
}

void SystemManager::SendFrameUpdate() {
    Update();
}

void SystemManager::ResetConsole() {
    char cnfFile[48];
    char fileBuffer[4096];
    char nameBuffer[112];
    char execBuffer[112];
    int iDiskType = sceCdGetDiskType();
    int iCount = 0;
    char *pData;

    if ((unsigned int)(iDiskType - 16) < 5) {
        int fd;
        int iSize;

        sceCdDiskReady(0);
        sceCdSearchFile(cnfFile, "\\SYSTEM.CNF;1");
        fd = sceOpen("cdrom0:\\SYSTEM.CNF;1", 1);
        iSize = sceLseek(fd, 0, 2);
        if (iSize >= 4097) {
            sceClose(fd);
        }
        sceLseek(fd, 0, 0);
        sceRead(fd, fileBuffer, iSize);
        sceClose(fd);
        pData = fileBuffer;
        {
            int i = 0;

            while (*pData != '\n' && i < iSize) {
                pData++;
                i++;
            }
        }
        *pData = 0;
        pData = fileBuffer;
        {
            int i = 0;

            while (*pData != ':' && i < iSize) {
                pData++;
                i++;
            }
        }
        pData += 2;
        do {
            nameBuffer[iCount++] = *pData++;
        } while (pData[-2] != ';' || pData[-1] != '1');
        sprintf(execBuffer, "cdrom0:\\%s", nameBuffer);
    }
    IExtension::sCurrentExtension->GetReadyToReset();
    sceDmaReset(1);
    scePadEnd();
    if (sceSifSearchModuleByName("dev9") >= 0) {
        while (sceDevctl("dev9x:", 17410, 0, 0, 0, 0) < 0) {
        }
    }
    if ((unsigned int)(iDiskType - 16) < 5) {
        LoadExecPS2(execBuffer, 0, 0);
    } else {
        LoadExecPS2(IExtension::sCurrentExtension->GetConsoleTarget(), 0, 0);
    }
}

void SystemManager::Update() {
    m_iAccumulated += GetTimeElapsed(m_iLastTime);
    m_iHeartbeatCounter--;
    if (m_iAccumulated > 1000) {
        m_iHeartbeatCounter = m_iHeartbeatReset;
        m_iAccumulated = 0;
        m_pCom->NewMsg((Command)3, (Channels)0);
        m_pCom->PutInt(GameHook::Instance()->m_iFrameCount, (Channels)0);
        m_pCom->EndMsg((Channels)0);
        m_pCom->Send((Channels)0);
    }
    if (m_iField20 != 0) {
        m_iAccum2 = 0;
        m_iTime2 = GetCurrentSystemTime();
        m_iField20 = 0;
        m_iTimeout = IExtension::sCurrentExtension->GetHeartBeatTimeOut();
    } else {
        m_iAccum2 += GetTimeElapsed(m_iTime2);
        if (m_iTimeout < m_iAccum2) {
            m_pCom->Disconnect();
        }
    }
}

void SystemManager::Disable() {
}

void SystemManager::SetFrequency() {
    mSystemFrequencyScaler = 1.0f / 300000.0f;
    mHasSetFreq = 1;
}

unsigned int SystemManager::GetTimeElapsed(unsigned int &iLastTime) {
    unsigned int iNow = GetCurrentSystemTime();
    unsigned int iElapsed = iNow - iLastTime;

    if (iNow < iLastTime) {
        iElapsed = 0;
    }
    iLastTime = iNow;
    return iElapsed;
}

void SystemManager::SendPauseAck(bool bPaused) {
    m_pCom->NewMsg((Command)11, (Channels)0);
    if (bPaused) {
        m_pCom->PutInt(1, (Channels)0);
    } else {
        m_pCom->PutInt(0, (Channels)0);
    }
    m_pCom->EndMsg((Channels)0);
}

}
