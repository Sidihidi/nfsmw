// Juice 1.5.2a: JuiceReplayManager.cpp, grabacion y reproduccion de datos
// de juego por segmentos: dos ReplaySettings estaticos (complete y
// segments) a los que apuntan m_pSettings[1] y [2], entradas con cabecera
// de tipo/longitud y verificacion contra el fichero al reproducir.
// Escrito del asm de la A124 (JuiceReplayManager.o: .text 0x4B68C0, 54
// etiquetas). Unidad solo enlaza en A124. Hereda de Processor (vptr a
// +0x0C, ABI viejo). Casan 42 de 57; el resto queda documentado en el
// informe 2026-10-14_p2j3 (las peores: SetSegmentState, ProcessMessage,
// AddData y AddEntry, cuya logica difiere mas de lo escrito).

#include <stdio.h>
#include <string.h>

namespace Juice {

enum Command {
};
enum Channels {
};
enum GameLevel {
};
enum ReplayChannel {
};
enum SegmentState {
};

class cJuiceCom {
public:
    void NewMsg(Command cmd, Channels channel);
    void EndMsg(Channels channel);
    void Send(Channels channel);
    void PutInt(int i, Channels channel);
    void PutString(const char *pString, Channels channel);
    void NewMsgAsBytes(Command cmd, int iBytes, const char *pBytes, Channels channel);
    int GetInt(Channels channel);
    char GetChar(Channels channel);
    char *GetNBytes(int iBytes, Channels channel);
    int HasNBytes(int iBytes, Channels channel);
    void ResetBuffer(Channels eChannel);
};

class GameHook {
public:
    static GameHook *(*Instance)();
    int IsConnected();
    void JuiceUpdate();
    void DisableJuice();
    void AssertLog(int iLine, const char *pFunction, const char *pFile, const char *pCondition,
                   const char *pMessage);
    void LogText(const char *pText);
    char *Replay_AddOrGetData(char *pData, int iChannel, const char *pName, int iFlag);
    unsigned int CalculateUsedBytes(unsigned int iData);

    int m_bInitialized;
    int m_iFrameCount;
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
    static SystemManager *Instance();
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

struct ReplaySettings {
    int m_iField0;
    int m_iField4;
    int m_iNormalMode;
    int m_iDebugMode;
    int m_iField16;
    int m_iField20;
    int m_iPlaybackMask;
    int m_iRecordMask;
    int m_iField32;
    int m_iPos;
    int m_iField40;
};

struct ReplayEntry {
    int m_iType;
    int m_iLength;
    int m_iData;
};

class ReplayManager : public Processor {
public:
    enum ReplayType {
    };

    ReplayManager();
    static ReplayManager *Instance();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();
    void InitReplaySettings(ReplaySettings *pSettings);
    void DisableReplayMgr();
    int IsPlayingBackEnabled();
    int IsReplayEnabled();
    int RequestToSaveGame();
    void IncrementPos(int iAmount, ReplayType eType);
    void CloseFile();
    void NotifyDesync();
    int IsPlayBackComplete();
    void SetSegmentState(SegmentState eState, int iValue1, int iValue2);
    void Begin();
    void SendReplayCommand(Command cmd);
    void SendReplayCommand(Command cmd, int iValue);
    void SendReplayEntry(unsigned char *pData, int iLength, ReplayChannel eChannel);
    int GetReplayHeader();
    char *GetReplayBody(int iLength);
    int IsNormalMode();
    int IsDebugMode();
    void SendVerifyResult(const char *pFile, int iLine, int iResult, int iChannel);
    unsigned int AddOrGetData(unsigned int iData, int iSize, ReplayChannel eChannel, const char *pName, int iFlag);
    float AddOrGetData(float fData, ReplayChannel eChannel, const char *pName, int iFlag);
    char *AddOrGetData(char *pData, ReplayChannel eChannel, const char *pName, int iFlag);
    void Init();
    void Save();
    void StopReplaying();
    void StopCapturing();
    int CheckForData(int iBytes);
    void DecodeReplayEntry(ReplayEntry &entry);
    int GetEntry(int iType);
    int GetData(int iSize, ReplayChannel eChannel);
    int GetSignedData(int iSize, ReplayChannel eChannel);
    float GetData(ReplayChannel eChannel);
    void AddEntry(int iData, int iSize, int iType);
    void AddData(int iData, int iSize, ReplayChannel eChannel);
    void AddData(float fData, ReplayChannel eChannel);
    int CheckPlaybackMask(ReplayChannel eChannel);
    int CheckRecordMask(ReplayChannel eChannel);
    int CheckVerifyMask(ReplayChannel eChannel);
    int CheckOverWriteMask(ReplayChannel eChannel);
    int CheckMask(int iMask, ReplayChannel eChannel);
    void ProcessHeader();
    void HandleVerifyResult(const char *pFile, int iLine, int iResult, ReplayChannel eChannel, char cGame,
                            char cReplay);
    void HandleVerifyResult(const char *pFile, int iLine, int iResult, ReplayChannel eChannel, int iGame,
                            int iReplay);
    void HandleVerifyResult(const char *pFile, int iLine, int iResult, ReplayChannel eChannel,
                            unsigned int iGame, unsigned int iReplay);
    void HandleVerifyResult(const char *pFile, int iLine, int iResult, ReplayChannel eChannel, float fGame,
                            float fReplay);
    int IsCapturing();
    int IsReplaying();

private:
    int m_iField16;        // +0x10
    int m_iField20;        // +0x14
    int m_iSaveRequested;  // +0x18
    int m_iField28;        // +0x1C
    int m_iSegmentState;   // +0x20
    int m_iField36;        // +0x24
    int m_iField40;        // +0x28
    int m_iField44;        // +0x2C
    int m_iField48;        // +0x30
    int m_iCheckForData;   // +0x34
    unsigned int m_iAccum56; // +0x38
    unsigned int m_iTime60;  // +0x3C
    unsigned int m_iAccum64; // +0x40
    ReplaySettings *m_pSettings[3]; // +0x44
};

ReplayManager::ReplayManager() {
    static ReplaySettings complete;
    static ReplaySettings segments;

    m_iField16 = 0;
    m_iField20 = 0;
    m_iSaveRequested = 0;
    m_iField28 = 0;
    m_iSegmentState = 0;
    m_iField36 = 0;
    m_iField40 = 0;
    m_iField44 = 0;
    m_iField48 = 0;
    m_iCheckForData = 0;
    m_iAccum56 = 0;
    m_bActive = 0;
    m_bEnabled = 0;
    InitReplaySettings(&complete);
    InitReplaySettings(&segments);
    m_pSettings[1] = &complete;
    m_pSettings[2] = &segments;
    m_iTime60 = 0;
    m_iAccum64 = 0;
}

void ReplayManager::InitReplaySettings(ReplaySettings *pSettings) {
    pSettings->m_iPos = 0;
    pSettings->m_iField32 = 0;
    pSettings->m_iField4 = 0;
    pSettings->m_iField0 = 0;
    pSettings->m_iField40 = 0;
    pSettings->m_iDebugMode = 0;
    pSettings->m_iField16 = 0;
    pSettings->m_iNormalMode = 0;
    pSettings->m_iRecordMask = 0;
    pSettings->m_iField20 = 0;
    pSettings->m_iPlaybackMask = 0;
}

ReplayManager *ReplayManager::Instance() {
    static ReplayManager replayManagerInstance;
    return &replayManagerInstance;
}

void ReplayManager::ProcessMessage(int iMsg) {
    switch (iMsg) {
    case 453:
        m_iField28 = 1;
        break;
    case 454:
        m_iSaveRequested = 1;
        break;
    case 451: {
        int iValue1 = m_pCom->GetInt((Channels)0);
        int iValue2;
        ReplaySettings *pSettings = m_pSettings[2];

        m_iField20 = (unsigned int)iValue1 > 0;
        iValue2 = m_pCom->GetInt((Channels)0);
        m_iField16 = (unsigned int)iValue2 > 0;
        pSettings->m_iField4 = m_iField20;
        pSettings->m_iField0 = m_pCom->GetInt((Channels)0);
        pSettings->m_iField32 = m_pCom->GetInt((Channels)0);
        pSettings->m_iField20 = m_pCom->GetInt((Channels)0);
        pSettings->m_iPlaybackMask = m_pCom->GetInt((Channels)0);
        pSettings->m_iRecordMask = m_pCom->GetInt((Channels)0);
        pSettings->m_iNormalMode = (unsigned int)m_pCom->GetInt((Channels)0) > 0;
        pSettings->m_iDebugMode = (unsigned int)m_pCom->GetInt((Channels)0) > 0;
        pSettings->m_iField16 = (unsigned int)m_pCom->GetInt((Channels)0) > 0;
        m_iField36 = (unsigned int)m_pCom->GetInt((Channels)0) > 0;
        m_bActive = 1;
        m_bEnabled = 1;
        pSettings->m_iPos = 0;
        break;
    }
    case 459:
        SetSegmentState((SegmentState)m_pCom->GetInt((Channels)0), m_pCom->GetInt((Channels)0),
                        m_pCom->GetInt((Channels)0));
        break;
    case 452:
        break;
    case 455:
        SendReplayCommand((Command)1204);
        break;
    case 456:
    case 457:
    case 458:
        break;
    default:
        DebugSystem::WarningOnMsg(175, "void Juice::ReplayManager::ProcessMessage(int)",
                                   "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceReplayManager.cpp", iMsg,
                                   "Invalid command %d was sent to the Replay Manager.");
        break;
    }
}

void ReplayManager::Initialize(cJuiceCom *pCom) {
    Processor::Initialize(pCom);
    if (!m_iField44) {
        SendReplayCommand((Command)401);
    }
}

void ReplayManager::DisableReplayMgr() {
    m_bEnabled = 1;
    m_iField44 = 1;
    m_bActive = 1;
    SendReplayCommand((Command)460);
}

int ReplayManager::IsPlayingBackEnabled() {
    return m_iField16;
}

int ReplayManager::IsReplayEnabled() {
    return m_bActive;
}

int ReplayManager::RequestToSaveGame() {
    if (m_iSaveRequested == 1) {
        m_iSaveRequested = 0;
        return 1;
    }
    return 0;
}

void ReplayManager::IncrementPos(int iAmount, ReplayType eType) {
    m_pSettings[eType]->m_iPos += iAmount;
}

void ReplayManager::CloseFile() {
    if (m_iSegmentState == 1) {
        SendReplayCommand((Command)405);
    } else if (m_iSegmentState == 2) {
        SendReplayCommand((Command)1203);
    }
}

void ReplayManager::NotifyDesync() {
    if (m_iSegmentState == 1) {
        SendReplayCommand((Command)407);
    } else if (m_iSegmentState == 2) {
        SendReplayCommand((Command)1206);
    }
}

int ReplayManager::IsPlayBackComplete() {
    if (m_iField28) {
        return 1;
    }
    if (m_pSettings[m_iSegmentState]->m_iPos < m_pSettings[m_iSegmentState]->m_iField32) {
        return 0;
    }
    return 1;
}

void ReplayManager::SetSegmentState(SegmentState eState, int iValue1, int iValue2) {
    int iMode = 0;

    switch ((int)eState) {
    case 1:
        iMode = 1;
        m_pSettings[2]->m_iPos = 0;
        m_pSettings[2]->m_iField20 = iValue1;
        m_pSettings[2]->m_iField4 = (int)eState;
        m_pSettings[2]->m_iField0 = 0;
        m_iField40 = m_pSettings[1]->m_iField20 | iValue1;
        break;
    case 2:
        if (m_iSegmentState == 0) {
            m_pCom->ResetBuffer((Channels)1);
            iMode = 2;
            m_iSegmentState = (int)eState;
            m_pSettings[2]->m_iField32 = iValue2;
            m_pSettings[2]->m_iRecordMask = iValue1;
            m_pSettings[2]->m_iPos = 0;
            m_iField36 = iValue1;
            m_pSettings[2]->m_iField0 = 1;
            m_iTime60 = SystemManager::GetCurrentSystemTime();
            m_pSettings[2]->m_iField4 = 0;
        }
        break;
    case 0:
        if (m_iSegmentState == 0) {
            StopReplaying();
            m_iField36 = 0;
            IExtension::sCurrentExtension->InstantiateScriptExt();
        }
        m_pSettings[2]->m_iField4 = 0;
        m_iField40 = m_pSettings[1]->m_iField20;
        m_pSettings[2]->m_iField0 = 0;
        break;
    }
    m_pCom->NewMsg((Command)1201, (Channels)0);
    m_pCom->PutInt(iMode, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

void ReplayManager::Begin() {
    if (GameHook::Instance()->IsConnected()) {
        Init();
        ProcessHeader();
    }
}

void ReplayManager::SendReplayCommand(Command cmd) {
    m_pCom->NewMsg(cmd, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

void ReplayManager::SendReplayCommand(Command cmd, int iValue) {
    m_pCom->NewMsg(cmd, (Channels)0);
    m_pCom->PutInt(iValue, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

void ReplayManager::SendReplayEntry(unsigned char *pData, int iLength, ReplayChannel eChannel) {
    if (m_pSettings[1]->m_iField4) {
        if (CheckMask(m_pSettings[1]->m_iField20, eChannel) || (int)eChannel == 1) {
            m_pCom->NewMsgAsBytes((Command)402, iLength, (char *)pData, (Channels)0);
            IncrementPos(iLength, (ReplayType)1);
        }
    }
    if (m_pSettings[2]->m_iField4) {
        if (CheckMask(m_pSettings[2]->m_iField20, eChannel)) {
            m_pCom->NewMsgAsBytes((Command)1202, iLength, (char *)pData, (Channels)0);
            IncrementPos(iLength, (ReplayType)2);
        }
    }
}

int ReplayManager::GetReplayHeader() {
    return m_pCom->GetChar((Channels)1) & 0xFF;
}

char *ReplayManager::GetReplayBody(int iLength) {
    return m_pCom->GetNBytes(iLength, (Channels)1);
}

void ReplayManager::Update() {
    if (!m_iCheckForData) {
        if (RequestToSaveGame()) {
            Save();
        }
        if (m_pSettings[2]->m_iField0) {
            m_iAccum64 += SystemManager::GetTimeElapsed(m_iTime60);
            if (m_iAccum64 > 500) {
                SendReplayCommand((Command)1205, m_pSettings[2]->m_iPos);
                m_iAccum64 = 0;
                m_iTime60 = SystemManager::GetCurrentSystemTime();
            }
        }
        if (m_iField48) {
            m_iField48 = 0;
            m_iSegmentState = 0;
        }
    }
}

void ReplayManager::Disable() {
}

int ReplayManager::IsNormalMode() {
    return m_pSettings[m_iSegmentState]->m_iNormalMode;
}

int ReplayManager::IsDebugMode() {
    return m_pSettings[m_iSegmentState]->m_iDebugMode;
}

void ReplayManager::SendVerifyResult(const char *pFile, int iLine, int iResult, int iChannel) {
    m_pCom->NewMsg((Command)408, (Channels)0);
    m_pCom->PutString(pFile, (Channels)0);
    m_pCom->PutInt(iLine, (Channels)0);
    m_pCom->PutInt(iResult, (Channels)0);
    m_pCom->PutInt(iChannel, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

unsigned int ReplayManager::AddOrGetData(unsigned int iData, int iSize, ReplayChannel eChannel, const char *pName,
                                         int iFlag) {
    if (IsReplaying()) {
        if (CheckPlaybackMask(eChannel)) {
            unsigned int iReplayData = GetData(iSize, eChannel);

            if (CheckVerifyMask(eChannel)) {
                HandleVerifyResult(pName, iFlag, iData == iReplayData, eChannel, iData, iReplayData);
            }
            if (CheckOverWriteMask(eChannel)) {
                iData = iReplayData;
            }
        }
    }
    if (IsCapturing()) {
        if (CheckRecordMask(eChannel)) {
            AddData((int)iData, GameHook::Instance()->CalculateUsedBytes(iData) * 8, eChannel);
        }
    }
    return iData;
}

float ReplayManager::AddOrGetData(float fData, ReplayChannel eChannel, const char *pName, int iFlag) {
    if (IsReplaying()) {
        if (CheckPlaybackMask(eChannel)) {
            float fReplayData = GetData(eChannel);

            if (CheckVerifyMask(eChannel)) {
                HandleVerifyResult(pName, iFlag, fData == fReplayData, eChannel, fData, fReplayData);
            }
            if (CheckOverWriteMask(eChannel)) {
                fData = fReplayData;
            }
        }
    }
    if (IsCapturing()) {
        if (CheckRecordMask(eChannel)) {
            AddData(fData, eChannel);
        }
    }
    return fData;
}

char *ReplayManager::AddOrGetData(char *pData, ReplayChannel eChannel, const char *pName, int iFlag) {
    if (IsReplaying()) {
        if (CheckPlaybackMask(eChannel)) {
            int iLength = GetData(16, eChannel);
            int iStringLength = strlen(pData);
            int i;

            if (CheckVerifyMask(eChannel)) {
                HandleVerifyResult(pName, iFlag, iStringLength == iLength, eChannel, iStringLength, iLength);
            }
            if (IsReplaying()) {
                for (i = 0; i < iLength; i++) {
                    char cData = (char)GetData(8, eChannel);

                    if (CheckVerifyMask(eChannel)) {
                        HandleVerifyResult(pName, iFlag, pData[i] == cData, eChannel, pData[i], cData);
                    }
                    if (CheckOverWriteMask(eChannel)) {
                        pData[i] = cData;
                    }
                }
                pData[iLength] = 0;
            }
        }
    }
    if (IsCapturing()) {
        if (CheckRecordMask(eChannel)) {
            int iLength = strlen(pData);
            int i;

            AddData(iLength, GameHook::Instance()->CalculateUsedBytes(iLength) * 8, eChannel);
            for (i = 0; i < iLength; i++) {
                AddData((int)pData[i], 8, eChannel);
            }
        }
    }
    return pData;
}

void ReplayManager::Init() {
    if (m_bActive) {
        m_pSettings[1]->m_iField4 = 0;
        m_pSettings[1]->m_iField0 = 0;
        GameHook::Instance()->LogText("JOYLOG: REPLAY ENABLED!\n");
        if (m_iField16) {
            GameHook::Instance()->LogText("JOYLOG: PLAYINGBACK!\n");
            SendReplayCommand((Command)404);
            m_iSegmentState = 1;
            m_pSettings[1]->m_iField0 = 1;
        }
        if (m_iField20) {
            SendReplayCommand((Command)403);
        }
    }
}

void ReplayManager::Save() {
    StopCapturing();
    SendReplayCommand((Command)405);
}

void ReplayManager::StopReplaying() {
    if (IsReplaying()) {
        m_iField16 = 0;
        m_pSettings[m_iSegmentState]->m_iField0 = 0;
        CloseFile();
        m_iField48 = 1;
        GameHook::Instance()->LogText("JOYLOG: STOP REPLAY!\n");
    }
}

void ReplayManager::StopCapturing() {
    m_pSettings[1]->m_iField4 = 0;
}

int ReplayManager::CheckForData(int iBytes) {
    unsigned int iStartTime = SystemManager::GetCurrentSystemTime();
    unsigned int iElapsed = 0;
    int iResult = 1;

    m_iCheckForData = 1;
    while (!m_pCom->HasNBytes(iBytes, (Channels)1)) {
        GameHook::Instance()->JuiceUpdate();
        iElapsed += SystemManager::GetTimeElapsed(iStartTime);
        if (IsPlayBackComplete()) {
            break;
        }
        if (iElapsed > 5000) {
            iResult = 0;
            break;
        }
    }
    if (IsPlayBackComplete()) {
        GameHook::Instance()->DisableJuice();
        iResult = 0;
    }
    m_iCheckForData = 0;
    return iResult;
}

void ReplayManager::DecodeReplayEntry(ReplayEntry &entry) {
    if (CheckForData(1)) {
        int iHeader = GetReplayHeader();
        int iLength;
        int i;
        int iValue = 0;
        char *pBody;

        IncrementPos(1, (ReplayType)m_iSegmentState);
        iLength = (iHeader >> 4) & 0xFF;
        entry.m_iLength = iLength;
        entry.m_iType = iHeader & 0xF;
        if (!CheckForData(iLength)) {
            return;
        }
        pBody = GetReplayBody(iLength);
        IncrementPos(iLength, (ReplayType)m_iSegmentState);
        for (i = iLength; i > 0; i--) {
            iValue = (iValue << 8) | (unsigned char)pBody[i - 1];
        }
        entry.m_iData = iValue;
    }
}

int ReplayManager::GetEntry(int iType) {
    ReplayEntry entry;

    DecodeReplayEntry(entry);
    if (entry.m_iType != iType) {
        GameHook::Instance()->LogText("JOYLOG: DESYNC!\n");
        NotifyDesync();
        StopReplaying();
    } else if (IsPlayBackComplete()) {
        GameHook::Instance()->LogText("JOYLOG: END OF FILE!\n");
        StopReplaying();
    }
    return entry.m_iData;
}

int ReplayManager::GetData(int iSize, ReplayChannel eChannel) {
    return GetEntry(eChannel);
}

int ReplayManager::GetSignedData(int iSize, ReplayChannel eChannel) {
    return (int)(((unsigned int)GetEntry(eChannel) << (32 - iSize)) >> (32 - iSize));
}

float ReplayManager::GetData(ReplayChannel eChannel) {
    int iData = GetSignedData(32, eChannel);

    return *(float *)&iData;
}

void ReplayManager::AddEntry(int iData, int iSize, int iType) {
    unsigned char buffer[8];
    int iBytes = (iSize + 7) >> 3;
    int i;

    buffer[0] = (unsigned char)(iType | (iBytes << 4));
    for (i = 0; i < iBytes; i++) {
        buffer[i + 1] = (unsigned char)iData;
        iData >>= 8;
    }
    SendReplayEntry(buffer, iBytes + 1, (ReplayChannel)0);
}

void ReplayManager::AddData(int iData, int iSize, ReplayChannel eChannel) {
    if (IExtension::sCurrentExtension->NumberOfRepeatedReplayEntries((int)eChannel) > 0) {
        if (!(m_iAccum56 + 1 > (unsigned int)IExtension::sCurrentExtension->NumberOfRepeatedReplayEntries((int)eChannel))) {
            m_iAccum56++;
            return;
        }
    }
    m_iAccum56 = 0;
    AddEntry(iData, iSize, (int)eChannel);
}

void ReplayManager::AddData(float fData, ReplayChannel eChannel) {
    AddData(*(int *)&fData, 32, eChannel);
}

int ReplayManager::CheckPlaybackMask(ReplayChannel eChannel) {
    return CheckMask(m_iField36, eChannel);
}

int ReplayManager::CheckRecordMask(ReplayChannel eChannel) {
    return CheckMask(m_iField40, eChannel);
}

int ReplayManager::CheckVerifyMask(ReplayChannel eChannel) {
    if (!m_iSegmentState) {
        return 0;
    }
    return CheckMask(m_pSettings[m_iSegmentState]->m_iPlaybackMask, eChannel);
}

int ReplayManager::CheckOverWriteMask(ReplayChannel eChannel) {
    if (!m_iSegmentState) {
        return 0;
    }
    return CheckMask(m_pSettings[m_iSegmentState]->m_iRecordMask, eChannel);
}

int ReplayManager::CheckMask(int iMask, ReplayChannel eChannel) {
    return ((1 << (int)eChannel) & iMask) > 0;
}

void ReplayManager::ProcessHeader() {
    char *pVersion = (char *)0x58BD40;
    char buffer1[256];
    char buffer2[256];
    char buffer3[256];

    strcpy(buffer1, IExtension::sCurrentExtension->GetTitleName());
    strcpy(buffer2, IExtension::sCurrentExtension->GetBuildName());
    strcpy(buffer3, IExtension::sCurrentExtension->GetChangeList());
    if (m_pSettings[m_iSegmentState]->m_iField0) {
        int iData = GetData(32, (ReplayChannel)1);
        ReplaySettings *pSettings = m_pSettings[m_iSegmentState];

        pSettings->m_iField20 = iData;
        m_iField36 = iData;
        pSettings->m_iPlaybackMask &= pSettings->m_iField20;
        pSettings->m_iRecordMask &= pSettings->m_iField20;
    }
    if (m_pSettings[1]->m_iField4) {
        AddData(m_pSettings[1]->m_iField20, 32, (ReplayChannel)1);
    }
    GameHook::Instance()->Replay_AddOrGetData((char *)pVersion, 1, "PS2", 0);
    GameHook::Instance()->Replay_AddOrGetData(buffer1, 1, "PS2", 0);
    GameHook::Instance()->Replay_AddOrGetData(buffer2, 1, "PS2", 0);
    GameHook::Instance()->Replay_AddOrGetData(buffer3, 1, "PS2", 0);
}

void ReplayManager::HandleVerifyResult(const char *pFile, int iLine, int iResult, ReplayChannel eChannel,
                                       char cGame, char cReplay) {
    if (IsDebugMode()) {
        SendVerifyResult(pFile, iLine, iResult, (int)eChannel);
    } else if (IsNormalMode()) {
        if (!iResult) {
            char buffer[256];

            sprintf(buffer,
                    "Verification Fail - Mis-match between data has been found! %c game value does not equal %c "
                    "replay file value",
                    cGame, cReplay);
            GameHook::Instance()->AssertLog(iLine, "HandleVerifyResult", pFile, "false", buffer);
            CloseFile();
            StopReplaying();
        }
    }
}

void ReplayManager::HandleVerifyResult(const char *pFile, int iLine, int iResult, ReplayChannel eChannel,
                                       int iGame, int iReplay) {
    if (IsDebugMode()) {
        SendVerifyResult(pFile, iLine, iResult, (int)eChannel);
    } else if (IsNormalMode()) {
        if (!iResult) {
            char buffer[256];

            sprintf(buffer,
                    "Verification Fail - Mis-match between data has been found! %d game value does not equal %d "
                    "replay file value",
                    iGame, iReplay);
            GameHook::Instance()->AssertLog(iLine, "HandleVerifyResult", pFile, "false", buffer);
            CloseFile();
            StopReplaying();
        }
    }
}

void ReplayManager::HandleVerifyResult(const char *pFile, int iLine, int iResult, ReplayChannel eChannel,
                                       unsigned int iGame, unsigned int iReplay) {
    if (IsDebugMode()) {
        SendVerifyResult(pFile, iLine, iResult, (int)eChannel);
    } else if (IsNormalMode()) {
        if (!iResult) {
            char buffer[256];

            sprintf(buffer,
                    "Verification Fail - Mis-match between data has been found! %u game value does not equal %u "
                    "replay file value",
                    iGame, iReplay);
            GameHook::Instance()->AssertLog(iLine, "HandleVerifyResult", pFile, "false", buffer);
            CloseFile();
            StopReplaying();
        }
    }
}

void ReplayManager::HandleVerifyResult(const char *pFile, int iLine, int iResult, ReplayChannel eChannel,
                                       float fGame, float fReplay) {
    if (IsDebugMode()) {
        SendVerifyResult(pFile, iLine, iResult, (int)eChannel);
    } else if (IsNormalMode()) {
        if (!iResult) {
            char buffer[256];

            sprintf(buffer,
                    "Verification Fail - Mis-match between data has been found! %0.7e game value does not equal "
                    "%0.7e replay file value",
                    fGame, fReplay);
            GameHook::Instance()->AssertLog(iLine, "HandleVerifyResult", pFile, "false", buffer);
            CloseFile();
            StopReplaying();
        }
    }
}

int ReplayManager::IsCapturing() {
    int iCapturing = 0;

    if (m_pSettings[1]->m_iField4 || m_pSettings[2]->m_iField4) {
        iCapturing = 1;
    }
    return iCapturing;
}

int ReplayManager::IsReplaying() {
    if (!m_iSegmentState) {
        return 0;
    }
    return m_pSettings[m_iSegmentState]->m_iField0;
}

}
