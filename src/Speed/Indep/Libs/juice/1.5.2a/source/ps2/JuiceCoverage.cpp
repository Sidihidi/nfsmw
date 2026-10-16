// Juice 1.5.2a: JuiceCoverage.cpp, cobertura de assets, estadisticas y
// eventos por nivel de juego. Escrito del asm de la A124
// (JuiceCoverage.o: .text 0x4B1428, 13 etiquetas). Unidad solo enlaza en
// A124. Hereda de Processor (vptr a +0x0C, ABI viejo). Cada tipo de envio
// tiene su interruptor a +0x40/+0x44/+0x48 y la tabla de punteros a
// miembro de +0x54 permite encenderlos y apagarlos por indice desde los
// comandos 351/352; el de eventos lleva ademas un presupuesto a +0x50 que
// nace a -1 (sin limite). El objeto mide 224 B y la cola desde +0x60 no
// la toca ninguna funcion de la unidad.

namespace Juice {

enum Command {
};
enum Channels {
};
enum GameLevel {
};

class cJuiceCom {
public:
    void NewMsg(Command cmd, Channels channel);
    void EndMsg(Channels channel);
    void Send(Channels channel);
    void PutInt(int i, Channels channel);
    void PutChar(char c, Channels channel);
    void PutFloat(float f, Channels channel);
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

class CoverageManager : public Processor {
public:
    CoverageManager();
    static CoverageManager *Instance();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();
    void SendAsset(const char *pAsset, const char *pType);
    void SendGameStat(GameLevel eLevel, const char *pName, const char *pValue, int iValue);
    void SendEvent(const char *p1, const char *p2, const char *p3, int iValue, float fValue, const char *p4,
                   const char *p5, const char *p6, float fTime);
    void AdvanceCycle(GameLevel eLevel);
    void ResetCycle(GameLevel eLevel);
    void SendAllCoverage();

private:
    int m_iCycle[10];              // +0x10
    int m_iUnused56;               // +0x38
    int m_iUnused60;               // +0x3C
    int m_bSendAsset;              // +0x40
    int m_bSendGameStat;           // +0x44
    int m_bSendEvent;              // +0x48
    int m_iUnused76;               // +0x4C
    unsigned int m_iEventBudget;   // +0x50
    int CoverageManager::*m_pFlags[3]; // +0x54
    int m_iUnused96[32];           // +0x60
};

CoverageManager *CoverageManager::Instance() {
    static CoverageManager coverageManagerInstance;
    return &coverageManagerInstance;
}

CoverageManager::CoverageManager() {
    int i;

    m_iUnused60 = 0;
    m_bSendAsset = 1;
    m_bSendGameStat = 1;
    m_bSendEvent = 1;
    m_iUnused76 = 1;
    m_iEventBudget = -1;
    for (i = 9; i >= 0; i--) {
        m_iCycle[i] = 1;
    }
    m_pFlags[0] = &CoverageManager::m_bSendAsset;
    m_pFlags[1] = &CoverageManager::m_bSendGameStat;
    m_pFlags[2] = &CoverageManager::m_bSendEvent;
}

void CoverageManager::SendAsset(const char *pAsset, const char *pType) {
    if (m_bSendAsset) {
        m_pCom->NewMsg((Command)304, (Channels)0);
        m_pCom->PutString(pAsset, (Channels)0);
        m_pCom->PutString(pType, (Channels)0);
        m_pCom->EndMsg((Channels)0);
    }
}

void CoverageManager::SendGameStat(GameLevel eLevel, const char *pName, const char *pValue, int iValue) {
    if (m_bSendGameStat && eLevel < 11) {
        m_pCom->NewMsg((Command)306, (Channels)0);
        m_pCom->PutInt((int)eLevel, (Channels)0);
        m_pCom->PutString(pName, (Channels)0);
        m_pCom->PutInt(m_iCycle[eLevel - 1], (Channels)0);
        m_pCom->PutChar('i', (Channels)0);
        m_pCom->PutString(pValue, (Channels)0);
        m_pCom->PutInt(iValue, (Channels)0);
        m_pCom->EndMsg((Channels)0);
    }
}

void CoverageManager::SendEvent(const char *p1, const char *p2, const char *p3, int iValue, float fValue,
                                const char *p4, const char *p5, const char *p6, float fTime) {
    if (m_bSendEvent && m_iEventBudget != 0) {
        m_pCom->NewMsg((Command)307, (Channels)0);
        m_pCom->PutString(p1, (Channels)0);
        m_pCom->PutString(p2, (Channels)0);
        m_pCom->PutString(p3, (Channels)0);
        m_pCom->PutInt(iValue, (Channels)0);
        m_pCom->PutFloat(fValue, (Channels)0);
        m_pCom->PutString(p4, (Channels)0);
        m_pCom->PutString(p5, (Channels)0);
        m_pCom->PutString(p6, (Channels)0);
        m_pCom->PutInt(GameHook::Instance()->m_iFrameCount, (Channels)0);
        m_pCom->PutFloat(fTime, (Channels)0);
        m_pCom->EndMsg((Channels)0);
        m_iEventBudget--;
    }
}

void CoverageManager::AdvanceCycle(GameLevel eLevel) {
    if (m_bSendGameStat && eLevel < 11) {
        if (eLevel < 5) {
            m_pCom->NewMsg((Command)309, (Channels)0);
            m_pCom->PutInt((int)eLevel, (Channels)0);
            m_pCom->EndMsg((Channels)0);
        }
        m_iCycle[eLevel - 1]++;
    }
}

void CoverageManager::ResetCycle(GameLevel eLevel) {
    if (m_bSendGameStat && eLevel < 11 && eLevel > 3) {
        m_iCycle[eLevel - 1] = 1;
    }
}

void CoverageManager::Initialize(cJuiceCom *pCom) {
    m_pCom = pCom;
    m_pCom->NewMsg((Command)301, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

void CoverageManager::ProcessMessage(int iMsg) {
    switch (iMsg) {
    case 351: {
        int iIndex = m_pCom->GetInt((Channels)0);

        this->*m_pFlags[iIndex] = 1;
        if (iIndex == 2) {
            m_iEventBudget = m_pCom->GetInt((Channels)0);
        }
        break;
    }
    case 352:
        this->*m_pFlags[m_pCom->GetInt((Channels)0)] = 0;
        break;
    case 353:
        break;
    case 354:
        break;
    case 355:
        SendAllCoverage();
        break;
    default:
        DebugSystem::WarningOnMsg(269, "void Juice::CoverageManager::ProcessMessage(int)",
                                   "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceCoverage.cpp", iMsg,
                                   "Invalid command %d was sent to the Coverage Manager.");
        break;
    }
}

void CoverageManager::Disable() {
}

void CoverageManager::Update() {
}

void CoverageManager::SendAllCoverage() {
}

}
