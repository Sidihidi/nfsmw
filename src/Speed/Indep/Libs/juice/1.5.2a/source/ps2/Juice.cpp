// Juice 1.5.2a: Juice.cpp, el GameHook: dueno del cJuiceCom y de los doce
// Processor, despacha cada mensaje al gestor que toca segun cmd/100 y
// ofrece al juego la fachada de estadisticas, eventos, replay y asserts.
// Escrito del asm de la A124 (Juice.o: .text 0x4AF188, 31 etiquetas).
// Unidad solo enlaza en A124. GameHook::Instance es un puntero a funcion
// estatico que apunta a InternalInstanceFunc, y el cJuiceCom y los
// objetos de scripting Controller/Debug/Commands son estaticos locales de
// Initialize. Casan 30 de 40 (ronda p2j3: IsConnected, DisableJuice,
// JuiceUpdate y el __tcf_0 del estatico local, que exigia declarar el
// cJuiceCom con su tamano real para que el estatico no caiga en .sbss);
// queda parcial Initialize (84,1 %).

// Global, no de Juice: el original llama a JuiceTrapHandlerPS2__Fv (JuiceExceptionPS2.cpp).
void JuiceTrapHandlerPS2();

namespace Juice {

enum Command {
};
enum Channels {
};
enum GameLevel {
};
enum ReplayChannel {
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

class cJuiceComChannel {
public:
    enum CONNECTION_TYPE {
        CONNECTION_NONE = 0,
        CONNECTION_STREAM = 1,
    };

private:
    int m_iConnectionType;
    Channels m_eChannelID;
    int m_bCanStream;
    int m_bStreamRequested;
    int m_bMsgInProgress;
    int m_bInitialized;
    int m_iLengthPos;
    int m_iUnused28;
    int m_iOutPos;
    int m_iMsgLength;
    int m_iReadCount;
    int m_iStoredMsgLength;
    int m_iInLength;
    int m_iReadOffset;
    char m_pInBuffer[2048];
    char m_pOutBuffer[2048];
    JuiceMutex m_mutex;
};

class cJuiceCom {
public:
    cJuiceCom();
    ~cJuiceCom();
    void Init(Channels eChannel, cJuiceComChannel::CONNECTION_TYPE eType);
    int Read();
    int HasUnprocessedMsg(Channels eChannel);
    int GetMsgLength(Channels eChannel);
    int GetMsgID(Channels eChannel);
    int Connect(const char *pHost, int iPort);
    int Disconnect();
    void TearDownComLayer();
    int IsConnected() const;

private:
    int m_iUnused0;
    int m_iCurrentChannel;
    int m_bConnected;
    int m_bComLayerInitialized;
    cJuiceComChannel m_channels[2];
    char m_pReadBuffer[2048];
    char m_pHost[32];
    int m_iPort;
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

class INetwork {
public:
    INetwork();
    virtual int Connect(const char *host, int port) = 0;
    virtual int IsConnected() = 0;
    virtual int Recv(char *buf, int *channel, int size) = 0;
    virtual int PeekHdr(char *data) = 0;
    virtual int Send(int length, int channel, char *sendBuffer) = 0;
    virtual int IsEnabled() = 0;
    virtual int Initialize() = 0;
    virtual int Disconnect() = 0;
    virtual void MaintainConn() = 0;
    virtual void TearDown() = 0;
    virtual void FreeEverything() = 0;
    virtual char *GetMac() = 0;
    virtual char *GetLocalIpAddress() = 0;
    virtual ~INetwork();

    static INetwork *sCurrentNetExtention;
};

class Processor {
public:
    Processor();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable() = 0;
    virtual void ProcessMessage(int iMsg) = 0;
    virtual void Update() = 0;

    cJuiceCom *m_pCom;
    int m_bEnabled;
    int m_bActive;
};

class SystemManager : public Processor {
public:
    static SystemManager *Instance();
    int IsRedirecting();
    int IsGamePaused();
    int InAutomationMode();
    int AutomationCrashReboot();
    void FilterCommands(Command &cmd);
    void SendFrameUpdate();
    void ResetConsole();
    void RegisterWithServer(int iID, cJuiceCom *pCom);
    static unsigned int GetCurrentSystemTime();
    static unsigned int GetTimeElapsed(unsigned int &iLastTime);

    int m_iPad24;
    int m_iPad28;
    int m_iRedirecting;
};

class MonitorManager : public Processor {
public:
    static MonitorManager *Instance();
};

class LoggingManager : public Processor {
public:
    static LoggingManager *Instance();
    void SendLogDebugOutput(const char *pString);
};

class CoverageManager : public Processor {
public:
    static CoverageManager *Instance();
    void SendAsset(const char *pAsset, const char *pType);
    void SendGameStat(GameLevel eLevel, const char *pName, const char *pValue, int iValue);
    void SendEvent(const char *p1, const char *p2, const char *p3, int iValue, float fValue, const char *p4,
                   const char *p5, const char *p6, float fTime);
    void AdvanceCycle(GameLevel eLevel);
    void ResetCycle(GameLevel eLevel);
};

class ReplayManager : public Processor {
public:
    static ReplayManager *Instance();
    void Begin();
    void DisableReplayMgr();
    int IsCapturing();
    int IsReplaying();
    int IsPlayingBackEnabled();
    unsigned int AddOrGetData(unsigned int iData, int iSize, ReplayChannel eChannel, const char *pName, int iFlag);
    float AddOrGetData(float fData, ReplayChannel eChannel, const char *pName, int iFlag);
    char *AddOrGetData(char *pData, ReplayChannel eChannel, const char *pName, int iFlag);
};

class RemoteProcedureManager : public Processor {
public:
    static RemoteProcedureManager *Instance();
};

class JuiceOnlinePlayMgr : public Processor {
public:
    static JuiceOnlinePlayMgr *Instance();
};

class ErrorManager : public Processor {
public:
    static ErrorManager *Instance();
    void AssertLog(int iLine, const char *pFunction, const char *pFile, const char *pCondition,
                   const char *pMessage);
};

class PadConfigManager : public Processor {
public:
    static PadConfigManager *Instance();
};

class ThreadManager : public Processor {
public:
    static ThreadManager *Instance();
    void CheckThreadsForHangs();
};

class RemoteFileManager : public Processor {
public:
    static RemoteFileManager *Instance();
};

class JuiceScreenshotMgr : public Processor {
public:
    static JuiceScreenshotMgr *Instance();
};

namespace Scripting {

class VarArgs;

class ICommands {
public:
    ICommands();
    virtual ~ICommands();
    virtual void AutoRegister() const;
    virtual const char *GetNamespaceName() const = 0;
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

class Controller : public IController {
public:
    Controller();
    virtual ~Controller();
    virtual const char *GetNamespaceName() const;
    virtual void DpadDown(VarArgs &args);
    virtual void DpadUp(VarArgs &args);
    virtual void DpadLeft(VarArgs &args);
    virtual void DpadRight(VarArgs &args);
    virtual void LeftAnalog(VarArgs &args);
    virtual void RightAnalog(VarArgs &args);
    virtual void Button1(VarArgs &args);
    virtual void Button2(VarArgs &args);
    virtual void Button3(VarArgs &args);
    virtual void Button4(VarArgs &args);
    virtual void ButtonL1(VarArgs &args);
    virtual void ButtonL2(VarArgs &args);
    virtual void ButtonL3(VarArgs &args);
    virtual void ButtonR1(VarArgs &args);
    virtual void ButtonR2(VarArgs &args);
    virtual void ButtonR3(VarArgs &args);
    virtual void ButtonStart(VarArgs &args);
    virtual void ButtonSelect(VarArgs &args);
    virtual void JuiceButtonPress(VarArgs &args);
};

class Debug : public ICommands {
public:
    Debug();
    virtual ~Debug();
    virtual const char *GetNamespaceName() const;
};

class Commands : public ICommands {
public:
    Commands();
    virtual ~Commands();
    virtual const char *GetNamespaceName() const;
};

}

class GameHook {
public:
    GameHook();
    static GameHook *InternalInstanceFunc();
    void Initialize();
    void DisableJuice();
    int IsConnected();
    void FrameUpdate();
    void JuiceUpdate();
    void AssetHit(const char *pAsset, const char *pType);
    void LogStat(int iLevel, const char *pName, const char *pValue, int iValue);
    void AdvanceCycle(int iLevel);
    void ResetCycle(int iLevel);
    void GameEvent(const char *p1, const char *p2, const char *p3, int iValue, float fValue, const char *p4,
                   const char *p5, const char *p6);
    void LogText(const char *pText);
    void UpdatePad(void *pPad);
    int ReplayData(int &iData, int iSize, const char *pName, int iFlag);
    unsigned int ReplayData(unsigned int &iData, int iSize, const char *pName, int iFlag);
    float ReplayData(float &fData, int iSize, const char *pName, int iFlag);
    unsigned int CalculateUsedBytes(unsigned int iData);
    void InitTrapHandler();
    void AssertLog(int iLine, const char *pFunction, const char *pFile, const char *pCondition,
                   const char *pMessage);
    int AllInitialized();
    int Replay_IsCapturing();
    int Replay_IsReplaying();
    char *Replay_AddOrGetData(char *pData, int iChannel, const char *pName, int iFlag);
    void RebootWhenAutomating();
    unsigned int GetCurrentSystemTime();
    unsigned int GetTimeElapsed(unsigned int &iLastTime);

    static GameHook *(*Instance)();

    int m_bInitialized;       // +0x00
    int m_iFrameCount;        // +0x04
    int m_bConnected;         // +0x08
    int m_iUnused12;          // +0x0C
    int m_bTrapInitialized;   // +0x10
    cJuiceCom *m_pJuiceCom;   // +0x14
    Processor *m_pProcessors[12]; // +0x18
    JuiceMutex *m_pMutex;     // +0x48
};

GameHook *(*GameHook::Instance)() = &GameHook::InternalInstanceFunc;

GameHook::GameHook() {
    m_bInitialized = 0;
    m_iFrameCount = 0;
    m_bConnected = 0;
    m_iUnused12 = 0;
    m_bTrapInitialized = 0;
    static JuiceMutex mutex;

    m_pMutex = &mutex;
    mutex.Create();
}

GameHook *GameHook::InternalInstanceFunc() {
    static GameHook game;
    return &game;
}

void GameHook::DisableJuice() {
    int i;

    cJuiceCom *pCom = m_pJuiceCom;

    if (IsConnected()) {
        pCom->Disconnect();
    }
    for (i = 0; i < 12; i++) {
        if (m_pProcessors[i]) {
            m_pProcessors[i]->Disable();
        }
    }
    m_bInitialized = 0;
}

int GameHook::IsConnected() {
    if (!m_bInitialized) {
        return 0;
    }
    if (!m_pJuiceCom) {
        return 0;
    }
    int iConnected = m_pJuiceCom->IsConnected();

    if (m_bConnected != 0) {
        m_bInitialized = iConnected;
    }
    return iConnected;
}

void GameHook::Initialize() {
    int iWasConnected = 0;
    int i;

    m_bInitialized = 1;
    if (m_pJuiceCom) {
        iWasConnected = m_pJuiceCom->IsConnected();
    }
    if (!IExtension::sCurrentExtension) {
        return;
    }
    if (!INetwork::sCurrentNetExtention) {
        return;
    }
    if (iWasConnected) {
        if (!SystemManager::Instance()->IsRedirecting()) {
            return;
        }
    }
    {
        static cJuiceCom juiceCom;

        m_pJuiceCom = &juiceCom;
        juiceCom.Init((Channels)0, (cJuiceComChannel::CONNECTION_TYPE)0);
        juiceCom.Init((Channels)1, (cJuiceComChannel::CONNECTION_TYPE)1);
        if (IExtension::sCurrentExtension->IsOkToConnect() != 1) {
            m_bInitialized = 0;
            m_bConnected = 0;
        }
        if (m_bInitialized) {
            int iConnectAttempted = 0;

            if (!SystemManager::Instance()->IsRedirecting()) {
                const char *pHost;
                int iPort;

                iConnectAttempted = 1;
                pHost = IExtension::sCurrentExtension->GetServerIP();
                iPort = IExtension::sCurrentExtension->GetServerPort();
                juiceCom.Connect(pHost, iPort);
                if (!juiceCom.IsConnected()) {
                    pHost = IExtension::sCurrentExtension->GetBackupServerIP();
                    iPort = IExtension::sCurrentExtension->GetBackupServerPort();
                    juiceCom.Connect(pHost, iPort);
                }
            }
            SystemManager::Instance()->m_iRedirecting = 0;
            m_bConnected = juiceCom.IsConnected();
            if (juiceCom.IsConnected()) {
                unsigned int iStartTime;
                unsigned int iElapsed;
                unsigned int iTimeout;

                SystemManager::Instance()->RegisterWithServer(-2, &juiceCom);
                m_pProcessors[0] = SystemManager::Instance();
                m_pProcessors[1] = MonitorManager::Instance();
                m_pProcessors[2] = LoggingManager::Instance();
                m_pProcessors[3] = CoverageManager::Instance();
                m_pProcessors[4] = ReplayManager::Instance();
                m_pProcessors[5] = RemoteProcedureManager::Instance();
                m_pProcessors[6] = JuiceOnlinePlayMgr::Instance();
                m_pProcessors[7] = ErrorManager::Instance();
                m_pProcessors[8] = PadConfigManager::Instance();
                m_pProcessors[9] = ThreadManager::Instance();
                m_pProcessors[10] = RemoteFileManager::Instance();
                m_pProcessors[11] = JuiceScreenshotMgr::Instance();
                if (m_iFrameCount > 0) {
                    ((ReplayManager *)m_pProcessors[5])->DisableReplayMgr();
                }
                for (i = 0; i < 12; i++) {
                    m_pProcessors[i]->Initialize(&juiceCom);
                }
                iTimeout = IExtension::sCurrentExtension->GetInitializationTimeOut();
                iStartTime = SystemManager::GetCurrentSystemTime();
                iElapsed = 0;
                for (;;) {
                    juiceCom.Read();
                    while (juiceCom.HasUnprocessedMsg((Channels)0)) {
                        int iMsgID;

                        juiceCom.GetMsgLength((Channels)0);
                        iMsgID = juiceCom.GetMsgID((Channels)0);
                        m_pProcessors[iMsgID / 100]->ProcessMessage(iMsgID);
                    }
                    iElapsed += SystemManager::GetTimeElapsed(iStartTime);
                    if (!juiceCom.IsConnected()) {
                        if (SystemManager::Instance()->IsRedirecting()) {
                            break;
                        }
                        m_bInitialized = 0;
                        m_bConnected = 0;
                    }
                    if (!m_bInitialized) {
                        break;
                    }
                    if (AllInitialized()) {
                        break;
                    }
                    if (SystemManager::Instance()->IsRedirecting()) {
                        break;
                    }
                    if (iElapsed >= iTimeout) {
                        break;
                    }
                }
                if (!AllInitialized() && iElapsed >= iTimeout) {
                    juiceCom.Disconnect();
                    m_bInitialized = 0;
                    m_bConnected = 0;
                }
                if (SystemManager::Instance()->IsRedirecting()) {
                    Initialize();
                }
                ReplayManager::Instance()->Begin();
            }
            if (!iConnectAttempted) {
                juiceCom.TearDownComLayer();
                m_bInitialized = 0;
            }
        }
    }
    {
        static Scripting::Controller controller;
        static Scripting::Debug debug;
        static Scripting::Commands commands;

        IExtension::sCurrentExtension->InstantiateScriptExt();
    }
}

void GameHook::FrameUpdate() {
    JuiceUpdate();
    m_iFrameCount++;
}

void GameHook::JuiceUpdate() {
    if (IsConnected()) {
        int i;

        m_pMutex->Lock();
        INetwork::sCurrentNetExtention->MaintainConn();
        cJuiceCom *pCom = m_pJuiceCom;

        do {
            pCom->Read();
            while (pCom->HasUnprocessedMsg((Channels)0)) {
                Command cmd;

                pCom->GetMsgLength((Channels)0);
                cmd = (Command)pCom->GetMsgID((Channels)0);
                ((SystemManager *)m_pProcessors[0])->FilterCommands(cmd);
                m_pProcessors[(int)cmd / 100]->ProcessMessage((int)cmd);
            }
            ((SystemManager *)m_pProcessors[0])->SendFrameUpdate();
            if (((SystemManager *)m_pProcessors[0])->IsGamePaused()) {
                ThreadManager::Instance()->CheckThreadsForHangs();
            }
        } while (((SystemManager *)m_pProcessors[0])->IsGamePaused());
        for (i = 0; i < 12; i++) {
            m_pProcessors[i]->Update();
        }
        INetwork::sCurrentNetExtention->MaintainConn();
        m_pMutex->Unlock();
    }
}

void GameHook::AssetHit(const char *pAsset, const char *pType) {
    if (IsConnected() && !((ReplayManager *)m_pProcessors[4])->IsPlayingBackEnabled()) {
        ((CoverageManager *)m_pProcessors[3])->SendAsset(pAsset, pType);
    }
}

void GameHook::LogStat(int iLevel, const char *pName, const char *pValue, int iValue) {
    if (IsConnected() && !ReplayManager::Instance()->IsPlayingBackEnabled()) {
        ((CoverageManager *)m_pProcessors[3])->SendGameStat((GameLevel)iLevel, pName, pValue, iValue);
    }
}

void GameHook::AdvanceCycle(int iLevel) {
    if (IsConnected() && !((ReplayManager *)m_pProcessors[4])->IsPlayingBackEnabled()) {
        ((CoverageManager *)m_pProcessors[3])->AdvanceCycle((GameLevel)iLevel);
    }
}

void GameHook::ResetCycle(int iLevel) {
    if (IsConnected() && !((ReplayManager *)m_pProcessors[4])->IsPlayingBackEnabled()) {
        ((CoverageManager *)m_pProcessors[3])->ResetCycle((GameLevel)iLevel);
    }
}

void GameHook::GameEvent(const char *p1, const char *p2, const char *p3, int iValue, float fValue, const char *p4,
                         const char *p5, const char *p6) {
    if (IsConnected() && !((ReplayManager *)m_pProcessors[4])->IsPlayingBackEnabled()) {
        ((CoverageManager *)m_pProcessors[3])
            ->SendEvent(p1, p2, p3, iValue, fValue, p4, p5, p6,
                        IExtension::sCurrentExtension->GetGameTime());
    }
}

void GameHook::LogText(const char *pText) {
    if (IsConnected()) {
        ((LoggingManager *)m_pProcessors[2])->SendLogDebugOutput(pText);
    }
}

void GameHook::UpdatePad(void *pPad) {
    if (IsConnected()) {
        IExtension::sCurrentExtension->UpdatePad(pPad);
    }
}

int GameHook::ReplayData(int &iData, int iSize, const char *pName, int iFlag) {
    if (IsConnected()) {
        iData = ((ReplayManager *)m_pProcessors[4])
                    ->AddOrGetData((unsigned int)iData, CalculateUsedBytes(iData) * 8, (ReplayChannel)iSize, pName,
                                   iFlag);
    }
    return iData;
}

unsigned int GameHook::ReplayData(unsigned int &iData, int iSize, const char *pName, int iFlag) {
    if (IsConnected()) {
        iData = ((ReplayManager *)m_pProcessors[4])
                    ->AddOrGetData(iData, CalculateUsedBytes(iData) * 8, (ReplayChannel)iSize, pName, iFlag);
    }
    return iData;
}

float GameHook::ReplayData(float &fData, int iSize, const char *pName, int iFlag) {
    if (IsConnected()) {
        fData = ((ReplayManager *)m_pProcessors[4])->AddOrGetData(fData, (ReplayChannel)iSize, pName, iFlag);
    }
    return fData;
}

unsigned int GameHook::CalculateUsedBytes(unsigned int iData) {
    if (Replay_IsCapturing()) {
        if ((iData & 0xFFFFFF00) == 0) {
            return 1;
        }
        if ((iData & 0xFFFF0000) == 0) {
            return 2;
        }
        if ((iData & 0xFF000000) == 0) {
            return 3;
        }
    }
    return 4;
}

void GameHook::InitTrapHandler() {
    if (!m_bTrapInitialized) {
        JuiceTrapHandlerPS2();
        m_bTrapInitialized = 1;
    }
}

void GameHook::AssertLog(int iLine, const char *pFunction, const char *pFile, const char *pCondition,
                         const char *pMessage) {
    if (IsConnected()) {
        ((ErrorManager *)m_pProcessors[7])->AssertLog(iLine, pFunction, pFile, pCondition, pMessage);
    }
}

int GameHook::AllInitialized() {
    int i;

    for (i = 0; i < 12; i++) {
        if (!m_pProcessors[i]->m_bEnabled) {
            return 0;
        }
    }
    return 1;
}

int GameHook::Replay_IsCapturing() {
    if (IsConnected()) {
        return ((ReplayManager *)m_pProcessors[4])->IsCapturing();
    }
    return 0;
}

int GameHook::Replay_IsReplaying() {
    if (IsConnected()) {
        return ((ReplayManager *)m_pProcessors[4])->IsReplaying();
    }
    return 0;
}

char *GameHook::Replay_AddOrGetData(char *pData, int iChannel, const char *pName, int iFlag) {
    if (IsConnected()) {
        ((ReplayManager *)m_pProcessors[4])->AddOrGetData(pData, (ReplayChannel)iChannel, pName, iFlag);
    }
    return pData;
}

void GameHook::RebootWhenAutomating() {
    if (((SystemManager *)m_pProcessors[0])->InAutomationMode() &&
        ((SystemManager *)m_pProcessors[0])->AutomationCrashReboot()) {
        ((SystemManager *)m_pProcessors[0])->ResetConsole();
    }
}

unsigned int GameHook::GetCurrentSystemTime() {
    return SystemManager::GetCurrentSystemTime();
}

unsigned int GameHook::GetTimeElapsed(unsigned int &iLastTime) {
    unsigned int iElapsed = SystemManager::GetTimeElapsed(iLastTime);

    return iElapsed;
}

}
