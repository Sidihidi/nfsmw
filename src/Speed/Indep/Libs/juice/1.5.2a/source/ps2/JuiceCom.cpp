// Juice 1.5.2a: JuiceCom.cpp, el cJuiceCom: dos canales cJuiceComChannel
// a los que reenvia cada operacion segun el Channels pedido, la conexion
// con el servidor a traves de la INetwork en curso (con los reintentos y
// esperas que dicta la IExtension) y la lectura de paquetes entrantes
// (PeekHdr/Recv sobre el buffer de +0x209C). Escrito del asm de la A124
// (JuiceCom.o: .text 0x4B09F0, 29 etiquetas). Unidad solo enlaza en A124.
// Casan 29 de 30 (Connect incluido: el clamp de espera es un minimo
// de 100, no un maximo).

#include <string.h>

namespace Juice {

enum Command {
};
enum Channels {
};

class GameHook {
public:
    static GameHook *(*Instance)();
    void DisableJuice();

    int m_bConnected;
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

    cJuiceComChannel();
    void SetChannelID(Channels eChannel);
    void Init(CONNECTION_TYPE eType);
    void NewMsg(Command cmd);
    void EndMsg();
    int HasUnprocessedMsg();
    int HasNBytes(int iBytes);
    int GetMsgLength();
    int GetInt();
    char GetChar();
    float GetFloat();
    char *GetString();
    char *GetNBytes(int iBytes);
    int GetMsgID();
    void PutInt(int iValue);
    void PutChar(char cValue);
    void PutFloat(float fValue);
    void PutString(const char *pString);
    void PutBytes(int iBytes, const char *pBytes);
    void NewMsgAsBytes(Command cmd, int iBytes, const char *pBytes);
    int Send();
    int StreamRequestData();
    int Read(char *pData, int iLength);
    void ResetBuffer();

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
    void InitComLayer();
    void Init(Channels eChannel, cJuiceComChannel::CONNECTION_TYPE eType);
    void NewMsg(Command cmd, Channels eChannel);
    void EndMsg(Channels eChannel);
    int HasUnprocessedMsg(Channels eChannel);
    int HasNBytes(int iBytes, Channels eChannel);
    int GetMsgLength(Channels eChannel);
    int GetInt(Channels eChannel);
    char GetChar(Channels eChannel);
    float GetFloat(Channels eChannel);
    char *GetString(Channels eChannel);
    char *GetNBytes(int iBytes, Channels eChannel);
    int GetMsgID(Channels eChannel);
    void PutInt(int iValue, Channels eChannel);
    void PutChar(char cValue, Channels eChannel);
    void PutFloat(float fValue, Channels eChannel);
    void PutString(const char *pString, Channels eChannel);
    void PutBytes(int iBytes, const char *pBytes, Channels eChannel);
    void NewMsgAsBytes(Command cmd, int iBytes, const char *pBytes, Channels eChannel);
    int Send(Channels eChannel);
    void ResetBuffer(Channels eChannel);
    int Read();
    int Connect(const char *pHost, int iPort);
    int Disconnect();
    void TearDownComLayer();
    int IsConnected() const;
    char *GetMacAddress();

private:
    int m_iUnused0;              // +0x0000
    int m_iCurrentChannel;       // +0x0004
    int m_bConnected;            // +0x0008
    int m_bComLayerInitialized;  // +0x000C
    cJuiceComChannel m_channels[2]; // +0x0010
    char m_pReadBuffer[2048];    // +0x209C
    char m_pHost[32];            // +0x289C
    int m_iPort;                 // +0x28BC
};

cJuiceCom::cJuiceCom() {
    int i;

    m_bComLayerInitialized = 0;
    m_bConnected = 0;
    for (i = 0; i < 2; i++) {
        m_channels[i].SetChannelID((Channels)i);
    }
    InitComLayer();
}

cJuiceCom::~cJuiceCom() {
}

void cJuiceCom::InitComLayer() {
    int iRetries;
    int iWait;

    if (INetwork::sCurrentNetExtention->Initialize()) {
        iRetries = IExtension::sCurrentExtension->NetworkInitRetries();
        iWait = IExtension::sCurrentExtension->WaitBetweenNetworkInitRetries();
        while (!INetwork::sCurrentNetExtention->IsEnabled()) {
            iRetries--;
            if (iRetries == -1) {
                break;
            }
            if (iRetries < 0) {
                m_bConnected = 0;
                GameHook::Instance()->DisableJuice();
                return;
            }
            IExtension::sCurrentExtension->ThreadYield(iWait);
        }
        INetwork::sCurrentNetExtention->MaintainConn();
        m_bComLayerInitialized = 1;
    } else {
        m_bComLayerInitialized = 0;
    }
}

void cJuiceCom::Init(Channels eChannel, cJuiceComChannel::CONNECTION_TYPE eType) {
    m_channels[eChannel].Init(eType);
}

void cJuiceCom::NewMsg(Command cmd, Channels eChannel) {
    m_channels[eChannel].NewMsg(cmd);
}

void cJuiceCom::EndMsg(Channels eChannel) {
    m_channels[eChannel].EndMsg();
}

int cJuiceCom::HasUnprocessedMsg(Channels eChannel) {
    return m_channels[eChannel].HasUnprocessedMsg();
}

int cJuiceCom::HasNBytes(int iBytes, Channels eChannel) {
    return m_channels[eChannel].HasNBytes(iBytes);
}

int cJuiceCom::GetMsgLength(Channels eChannel) {
    return m_channels[eChannel].GetMsgLength();
}

int cJuiceCom::GetInt(Channels eChannel) {
    return m_channels[eChannel].GetInt();
}

char cJuiceCom::GetChar(Channels eChannel) {
    return m_channels[eChannel].GetChar();
}

float cJuiceCom::GetFloat(Channels eChannel) {
    return m_channels[eChannel].GetFloat();
}

char *cJuiceCom::GetString(Channels eChannel) {
    return m_channels[eChannel].GetString();
}

char *cJuiceCom::GetNBytes(int iBytes, Channels eChannel) {
    return m_channels[eChannel].GetNBytes(iBytes);
}

int cJuiceCom::GetMsgID(Channels eChannel) {
    return m_channels[eChannel].GetMsgID();
}

void cJuiceCom::PutInt(int iValue, Channels eChannel) {
    m_channels[eChannel].PutInt(iValue);
}

void cJuiceCom::PutChar(char cValue, Channels eChannel) {
    m_channels[eChannel].PutChar(cValue);
}

void cJuiceCom::PutFloat(float fValue, Channels eChannel) {
    m_channels[eChannel].PutFloat(fValue);
}

void cJuiceCom::PutString(const char *pString, Channels eChannel) {
    m_channels[eChannel].PutString(pString);
}

void cJuiceCom::PutBytes(int iBytes, const char *pBytes, Channels eChannel) {
    m_channels[eChannel].PutBytes(iBytes, pBytes);
}

void cJuiceCom::NewMsgAsBytes(Command cmd, int iBytes, const char *pBytes, Channels eChannel) {
    m_channels[eChannel].NewMsgAsBytes(cmd, iBytes, pBytes);
}

int cJuiceCom::Send(Channels eChannel) {
    return m_channels[eChannel].Send();
}

void cJuiceCom::ResetBuffer(Channels eChannel) {
    m_channels[eChannel].ResetBuffer();
}

int cJuiceCom::Read() {
    int iResult = 0;
    int iLength = INetwork::sCurrentNetExtention->PeekHdr(m_pReadBuffer);

    INetwork::sCurrentNetExtention->MaintainConn();
    if (iLength > 0) {
        iResult = INetwork::sCurrentNetExtention->Recv(m_pReadBuffer, &m_iCurrentChannel, iLength);
        if (iResult > 0) {
            m_channels[m_iCurrentChannel].Read(m_pReadBuffer, iResult);
        }
        INetwork::sCurrentNetExtention->MaintainConn();
    }
    return iResult;
}

int cJuiceCom::Connect(const char *pHost, int iPort) {
    int iResult = -1;
    int iRetries;
    int iWait;

    if (m_bComLayerInitialized) {
        iRetries = IExtension::sCurrentExtension->InitialConnectionRetries();
        iWait = IExtension::sCurrentExtension->WaitBetweenInitialConnectionRretries();
        m_bConnected = 0;
        strcpy(m_pHost, pHost);
        m_iPort = iPort;
        iResult = INetwork::sCurrentNetExtention->Connect(pHost, iPort);
        INetwork::sCurrentNetExtention->MaintainConn();
        if (iWait < 100) {
            iWait = 100;
        }
        for (;;) {
            if (IsConnected()) {
                break;
            }
            iRetries--;
            if (iRetries == -1) {
                break;
            }
            IExtension::sCurrentExtension->ThreadYield(iWait);
            m_bConnected = INetwork::sCurrentNetExtention->IsConnected();
        }
        INetwork::sCurrentNetExtention->MaintainConn();
        m_bConnected = INetwork::sCurrentNetExtention->IsConnected();
        if (iRetries < 0) {
            m_bConnected = 0;
            INetwork::sCurrentNetExtention->Disconnect();
            INetwork::sCurrentNetExtention->FreeEverything();
        }
    }
    return iResult;
}

int cJuiceCom::Disconnect() {
    int iResult = 0;

    if (m_bConnected) {
        m_bConnected = 0;
        iResult = INetwork::sCurrentNetExtention->Disconnect();
        INetwork::sCurrentNetExtention->FreeEverything();
    }
    return iResult;
}

void cJuiceCom::TearDownComLayer() {
    INetwork::sCurrentNetExtention->TearDown();
}

int cJuiceCom::IsConnected() const {
    return m_bConnected;
}

char *cJuiceCom::GetMacAddress() {
    static char *address = INetwork::sCurrentNetExtention->GetMac();

    if (address && address[0] == '$') {
        return address + 1;
    }
    return "0FFFFFFDD";
}

}
