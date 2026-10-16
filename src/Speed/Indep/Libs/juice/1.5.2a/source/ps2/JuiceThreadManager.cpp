// Juice 1.5.2a: JuiceThreadManager.cpp, registro de hilos del juego y
// vigilancia de cuelgues (en esta version CheckForHang, CheckPoint,
// SendCallStacks y RegisterThreadWithServer estan vacios). Escrito del
// asm de la A124 (JuiceThreadManager.o: .text 0x4BB1F8, 16 etiquetas).
// Unidad solo enlaza en A124. Hereda de Processor (vptr a +0x0C, ABI
// viejo); casi todo el estado es estatico de clase (la lista de 24 hilos
// de 32 B, la informacion del mono de hilos y el mutex que construye el
// _GLOBAL_$I$). Los casos del switch van en el orden en que los emite el
// original. Casan 15 de 19 en banco (con los pads): el constructor queda
// al 89,9 % por la asignacion de registros de las constantes.

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
    int GetInt(Channels channel);
};

class JuiceMutex {
public:
    JuiceMutex();
    void Create();
    void Lock();
    void Unlock();

private:
    int m_bUncreated;
    int m_iOwner;
    int m_iLockCount;
    int m_iSema;
    int m_bHeld;
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

class SystemManager {
public:
    static unsigned int GetCurrentSystemTime();
    static unsigned int GetTimeElapsed(unsigned int &iLastTime);
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

struct RegisteredThread {
    int m_iThreadID;          // +0x00
    unsigned int m_iField4;   // +0x04
    int m_iUnused8;    // +0x08
    int m_iUnused12;   // +0x0C
    int m_iUnused16;   // +0x10
    int m_iField20;    // +0x14
    int m_iHangTimeout; // +0x18
    int m_iField28;    // +0x1C
};

struct ThreadMonkeyInfo {
    int m_iField0;
    int m_iField4;
    int m_iField8;
    int m_iField12;
    int m_iField16;
};

class ThreadManager : public Processor {
public:
    ThreadManager();
    static ThreadManager *Instance();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();
    void SetJuiceCycle(unsigned int iCycle);
    void CheckThreadsForHangs();
    void CheckForHang(unsigned int iThread);
    void CheckPoint(char *pName);
    void SendCallStacks();
    void RegisterThreadWithServer(unsigned int iThread);
    void RegisterThreadsWithServer();
    void InflictHang();

    static JuiceMutex mThreadMgrMutex;
    static int mJuiceThreadIDCount;
    static int mThreadMonitorEnabled;
    static RegisteredThread mJuiceRegisteredThreadList[24];
    static int mHasCreatedUpdateThread;
    static int mNewRegisteredThread;
    static int mThreadMonkeyEnabled;
    static int mHangInflicted;
    static unsigned int mThreadUpdateInterval;
    static ThreadMonkeyInfo mThreadMonkeyInfo;

private:
    unsigned int m_iJuiceCycle; // +0x10
    unsigned int m_iLastTime;   // +0x14
    unsigned int m_iAccumulated; // +0x18
    int m_iUnused28;            // +0x1C
    int m_iUnused32;            // +0x20
    int m_iUnused36;            // +0x24
};

int ThreadManager::mJuiceThreadIDCount = 1;
JuiceMutex ThreadManager::mThreadMgrMutex;
int ThreadManager::mThreadMonitorEnabled = 1;
RegisteredThread ThreadManager::mJuiceRegisteredThreadList[24];
int ThreadManager::mHasCreatedUpdateThread;
int ThreadManager::mNewRegisteredThread;
int ThreadManager::mThreadMonkeyEnabled;
int ThreadManager::mHangInflicted;
unsigned int ThreadManager::mThreadUpdateInterval;
ThreadMonkeyInfo ThreadManager::mThreadMonkeyInfo;

ThreadManager::ThreadManager() {
    int iHangTimeout;
    int i;

    m_iUnused32 = 10000;
    m_iLastTime = 0;
    m_iJuiceCycle = 5;
    m_iAccumulated = 0;
    m_iUnused28 = 0;
    mThreadMonkeyInfo.m_iField0 = 200;
    mThreadMonkeyInfo.m_iField4 = 1;
    mThreadMonkeyInfo.m_iField8 = 200;
    mThreadMonkeyInfo.m_iField16 = 1;
    mThreadMonkeyInfo.m_iField12 = 50;
    m_bEnabled = 0;
    m_bActive = 1;
    mHasCreatedUpdateThread = 0;
    mHangInflicted = 0;
    mThreadUpdateInterval = 500;
    mThreadMonitorEnabled = 1;
    iHangTimeout = IExtension::sCurrentExtension->HangTimeoutLength();
    for (i = 1; i < 24; i++) {
        if (mJuiceRegisteredThreadList[i].m_iHangTimeout == 0) {
            mJuiceRegisteredThreadList[i].m_iHangTimeout = iHangTimeout;
        }
        mJuiceRegisteredThreadList[i].m_iField4 = -1;
        mJuiceRegisteredThreadList[i].m_iField28 = 0;
    }
}

void ThreadManager::SetJuiceCycle(unsigned int iCycle) {
    m_iJuiceCycle = iCycle;
}

void ThreadManager::CheckThreadsForHangs() {
    int i;

    for (i = 1; i < mJuiceThreadIDCount; i++) {
        if (mJuiceRegisteredThreadList[i].m_iThreadID == 0) {
            break;
        }
        CheckForHang(i);
    }
}

void ThreadManager::CheckForHang(unsigned int iThread) {
}

ThreadManager *ThreadManager::Instance() {
    static ThreadManager threadManagerInstance;
    return &threadManagerInstance;
}

void ThreadManager::ProcessMessage(int iMsg) {
    switch (iMsg) {
    case 952:
        m_pCom->GetInt((Channels)0);
        break;
    case 953:
        m_pCom->GetInt((Channels)0);
        break;
    case 954:
        SendCallStacks();
        break;
    case 955:
        mThreadUpdateInterval = m_pCom->GetInt((Channels)0);
        break;
    case 957:
        SetJuiceCycle(m_pCom->GetInt((Channels)0));
        break;
    case 960: {
        int iEnable = m_pCom->GetInt((Channels)0);

        if (iEnable == 0) {
            mThreadMonkeyEnabled = 0;
        } else if (iEnable == 1) {
            mThreadMonkeyEnabled = 1;
        }
        break;
    }
    case 961: {
        int iMode = m_pCom->GetInt((Channels)0);

        mThreadMonkeyInfo.m_iField16 = iMode;
        if (iMode == 0) {
            int iValue1 = m_pCom->GetInt((Channels)0);
            int iValue2 = m_pCom->GetInt((Channels)0);
            int iValue3 = m_pCom->GetInt((Channels)0);

            mThreadMonkeyInfo.m_iField4 = iValue2;
            mThreadMonkeyInfo.m_iField8 = iValue3;
            mThreadMonkeyInfo.m_iField12 = iValue1;
        }
        if (iMode == 1) {
            mThreadMonkeyInfo.m_iField0 = m_pCom->GetInt((Channels)0);
        }
        break;
    }
    case 958: {
        int iThreadID = m_pCom->GetInt((Channels)0);
        int iValue = m_pCom->GetInt((Channels)0);
        int i;

        for (i = 1; i < 24; i++) {
            if (mJuiceRegisteredThreadList[i].m_iThreadID == iThreadID) {
                mJuiceRegisteredThreadList[i].m_iField20 = iValue;
            }
        }
        break;
    }
    case 959: {
        int iThreadID = m_pCom->GetInt((Channels)0);
        int iValue = m_pCom->GetInt((Channels)0);
        int i;

        for (i = 1; i < 24; i++) {
            if (mJuiceRegisteredThreadList[i].m_iThreadID == iThreadID) {
                mJuiceRegisteredThreadList[i].m_iHangTimeout = iValue;
            }
        }
        break;
    }
    case 951: {
        int iEnable = m_pCom->GetInt((Channels)0);
        int iValue = m_pCom->GetInt((Channels)0);
        int iFlag = 0;
        int i;

        if (iEnable == 1) {
            mThreadMonitorEnabled = 1;
            if (iValue == 1) {
                iFlag = 1;
            }
        } else {
            mThreadMonitorEnabled = 0;
        }
        for (i = 1; i < 24; i++) {
            mJuiceRegisteredThreadList[i].m_iField28 = iFlag;
        }
        m_bEnabled = 1;
        break;
    }
    default:
        DebugSystem::WarningOnMsg(345, "void Juice::ThreadManager::ProcessMessage(int)",
                                   "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceThreadManager.cpp", iMsg,
                                   "Invalid command %d was sent to the Thread Manager.");
        break;
    }
}

void ThreadManager::Initialize(cJuiceCom *pCom) {
    Processor::Initialize(pCom);
    m_pCom = pCom;
    m_pCom->NewMsg((Command)905, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
    RegisterThreadsWithServer();
}

void ThreadManager::Update() {
    if (mThreadMonitorEnabled) {
        m_iAccumulated += SystemManager::GetTimeElapsed(m_iLastTime);
        if (mThreadUpdateInterval < m_iAccumulated) {
            m_iAccumulated = 0;
            m_iLastTime = SystemManager::GetCurrentSystemTime();
            CheckPoint("Checkpoint");
        }
    }
}

void ThreadManager::Disable() {
}

void ThreadManager::CheckPoint(char *pName) {
}

void ThreadManager::SendCallStacks() {
}

void ThreadManager::RegisterThreadWithServer(unsigned int iThread) {
}

void ThreadManager::RegisterThreadsWithServer() {
    int i;

    for (i = 1; i < mJuiceThreadIDCount; i++) {
        RegisterThreadWithServer(i);
    }
}

void ThreadManager::InflictHang() {
    mHangInflicted = 1;
}

}
