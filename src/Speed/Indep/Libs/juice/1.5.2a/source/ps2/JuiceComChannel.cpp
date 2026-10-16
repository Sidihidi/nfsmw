// Juice 1.5.2a: JuiceComChannel.cpp, un canal del cJuiceCom: dos buffers
// de 2048 B (entrada a +0x38, salida a +0x838) con sus cursores y el
// mutex a +0x1038. Los mensajes salientes llevan la longitud en los 4
// bytes que NewMsg reserva con PutInt(0) y EndMsg rellena despues; al
// llegar la salida a 1024 B se manda sola, y en modo stream (tipo de
// conexion 1) se piden mas datos con el comando 1301 cuando quedan menos
// de 409 B por leer. Escrito del asm de la A124 (JuiceComChannel.o: .text
// 0x4B0B18, 24 etiquetas). Unidad solo enlaza en A124. Casan 23 de 24:
// GetFloat queda al 54 % solo por el orden de planificacion del lwc1 y
// los dos incrementos.

#include <string.h>

namespace Juice {

enum Command {
};
enum Channels {
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
    void PutIntToBuffer(int iValue) {
        m_pOutBuffer[m_iOutPos++] = (char)iValue;
        m_pOutBuffer[m_iOutPos++] = (char)(iValue >> 8);
        m_pOutBuffer[m_iOutPos++] = (char)(iValue >> 16);
        m_pOutBuffer[m_iOutPos++] = (char)(iValue >> 24);
        m_iMsgLength += 4;
    }

    int m_iConnectionType;  // +0x0000
    Channels m_eChannelID;  // +0x0004
    int m_bCanStream;       // +0x0008
    int m_bStreamRequested; // +0x000C
    int m_bMsgInProgress;   // +0x0010
    int m_bInitialized;     // +0x0014
    int m_iLengthPos;       // +0x0018
    int m_iUnused28;        // +0x001C
    int m_iOutPos;          // +0x0020
    int m_iMsgLength;       // +0x0024
    int m_iReadCount;       // +0x0028
    int m_iStoredMsgLength; // +0x002C
    int m_iInLength;        // +0x0030
    int m_iReadOffset;      // +0x0034
    char m_pInBuffer[2048];  // +0x0038
    char m_pOutBuffer[2048]; // +0x0838
    JuiceMutex m_mutex;      // +0x1038
};

cJuiceComChannel::cJuiceComChannel() {
    m_iConnectionType = 0;
    m_bMsgInProgress = 0;
    m_bInitialized = 0;
    m_bStreamRequested = 0;
    m_iReadOffset = 0;
    m_iInLength = 0;
    m_iMsgLength = 0;
    m_iReadCount = 0;
    m_iOutPos = 12;
    m_bCanStream = 1;
    m_mutex.Create();
}

void cJuiceComChannel::SetChannelID(Channels eChannel) {
    m_eChannelID = eChannel;
}

void cJuiceComChannel::Init(CONNECTION_TYPE eType) {
    m_iConnectionType = (int)eType;
    m_bInitialized = 1;
}

void cJuiceComChannel::NewMsg(Command cmd) {
    m_mutex.Lock();
    m_bMsgInProgress = 1;
    m_iLengthPos = m_iOutPos;
    PutInt(0);
    PutIntToBuffer((int)cmd);
}

void cJuiceComChannel::EndMsg() {
    m_pOutBuffer[m_iLengthPos++] = (char)m_iMsgLength;
    m_pOutBuffer[m_iLengthPos++] = (char)(m_iMsgLength >> 8);
    m_pOutBuffer[m_iLengthPos++] = (char)(m_iMsgLength >> 16);
    m_pOutBuffer[m_iLengthPos++] = (char)(m_iMsgLength >> 24);
    m_bMsgInProgress = 0;
    m_iMsgLength = 0;
    m_mutex.Unlock();
    if (m_iOutPos >= 1024) {
        Send();
    }
}

int cJuiceComChannel::HasUnprocessedMsg() {
    if (m_iConnectionType == 1) {
        StreamRequestData();
        if (m_iReadOffset < m_iInLength) {
            return 1;
        }
        return 0;
    }
    if (m_iConnectionType == 0) {
        if (m_iReadOffset < m_iInLength) {
            return 1;
        }
        return 0;
    }
    return 0;
}

int cJuiceComChannel::HasNBytes(int iBytes) {
    StreamRequestData();
    return (m_iInLength - m_iReadOffset) >= iBytes;
}

int cJuiceComChannel::GetMsgLength() {
    m_iReadCount = 0;
    m_iStoredMsgLength = GetInt();
    m_iReadCount += 4;
    return m_iStoredMsgLength;
}

int cJuiceComChannel::GetInt() {
    int iValue;

    iValue = (unsigned char)m_pInBuffer[m_iReadOffset];
    iValue += (unsigned char)m_pInBuffer[m_iReadOffset + 1] << 8;
    iValue += (m_pInBuffer[m_iReadOffset + 2] << 16) & 0xFF0000;
    iValue += m_pInBuffer[m_iReadOffset + 3] << 24;
    m_iReadOffset += 4;
    m_iReadCount += 4;
    return iValue;
}

char cJuiceComChannel::GetChar() {
    char cValue = m_pInBuffer[m_iReadOffset];

    m_iReadOffset++;
    m_iReadCount++;
    return cValue;
}

float cJuiceComChannel::GetFloat() {
    float fValue = *(float *)&m_pInBuffer[m_iReadOffset];

    m_iReadOffset += 4;
    m_iReadCount += 4;
    return fValue;
}

char *cJuiceComChannel::GetString() {
    char *pString = &m_pInBuffer[m_iReadOffset];
    int iLength = strlen(pString) + 1;

    m_iReadOffset += iLength;
    m_iReadCount += iLength;
    return pString;
}

char *cJuiceComChannel::GetNBytes(int iBytes) {
    m_iReadOffset += iBytes;
    m_iReadCount += iBytes;
    return &m_pInBuffer[m_iReadOffset - iBytes];
}

int cJuiceComChannel::GetMsgID() {
    int iID = GetInt();

    m_iReadCount += 4;
    return iID;
}

void cJuiceComChannel::PutInt(int iValue) {
    PutIntToBuffer(iValue);
}

void cJuiceComChannel::PutChar(char cValue) {
    m_pOutBuffer[m_iOutPos] = cValue;
    m_iOutPos++;
    m_iMsgLength++;
}

void cJuiceComChannel::PutFloat(float fValue) {
    PutIntToBuffer(*(int *)&fValue);
}

void cJuiceComChannel::PutString(const char *pString) {
    int iLength = strlen(pString) + 1;

    memcpy(&m_pOutBuffer[m_iOutPos], pString, iLength);
    m_iOutPos += iLength;
    m_iMsgLength += iLength;
}

void cJuiceComChannel::PutBytes(int iBytes, const char *pBytes) {
    PutInt(iBytes);
    memcpy(&m_pOutBuffer[m_iOutPos], pBytes, iBytes);
    m_iOutPos += iBytes;
    m_iMsgLength += iBytes;
}

void cJuiceComChannel::NewMsgAsBytes(Command cmd, int iBytes, const char *pBytes) {
    NewMsg(cmd);
    memcpy(&m_pOutBuffer[m_iOutPos], pBytes, iBytes);
    m_iOutPos += iBytes;
    m_iMsgLength += iBytes;
    EndMsg();
}

int cJuiceComChannel::Send() {
    m_mutex.Lock();
    INetwork::sCurrentNetExtention->Send(m_iOutPos - 12, (int)m_eChannelID, &m_pOutBuffer[12]);
    memset(m_pOutBuffer, 0, 2048);
    m_iOutPos = 12;
    INetwork::sCurrentNetExtention->MaintainConn();
    m_mutex.Unlock();
    return 1;
}

int cJuiceComChannel::StreamRequestData() {
    if (m_iConnectionType == 1) {
    if (m_bStreamRequested == 0) {
        NewMsg((Command)1301);
        PutInt(2028);
        EndMsg();
        Send();
        m_bStreamRequested = 1;
        m_bCanStream = 0;
        return 2048;
    }
    if ((m_iInLength - m_iReadOffset) < 409 && m_bCanStream == 1) {
        NewMsg((Command)1301);
        PutInt(2028 - (m_iInLength - m_iReadOffset));
        EndMsg();
        Send();
        m_bCanStream = 0;
        return 1639;
    }
    }
    return 0;
}

int cJuiceComChannel::Read(char *pData, int iLength) {
    if (m_iConnectionType == 0) {
        m_iInLength = iLength;
        m_iReadOffset = 0;
        memset(m_pInBuffer, 255, 2048);
        memcpy(m_pInBuffer, pData, iLength);
    } else if (m_iConnectionType == 1) {
        if (m_iInLength + iLength >= 2048) {
            memcpy(m_pInBuffer, &m_pInBuffer[m_iReadOffset], m_iInLength - m_iReadOffset);
            m_iInLength -= m_iReadOffset;
            m_iReadOffset = 0;
        }
        memcpy(&m_pInBuffer[m_iInLength], pData + 8, iLength - 8);
        m_bCanStream = 1;
        m_iInLength += iLength - 8;
    }
    return 0;
}

void cJuiceComChannel::ResetBuffer() {
    m_iInLength = 0;
    m_iReadOffset = 0;
    m_bStreamRequested = 0;
}

}
