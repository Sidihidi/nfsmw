// Juice 1.5.2a: JuiceErrorManager.cpp, el gestor de asserts: reenvia los
// AssertLog al servidor (comando 704) hasta un maximo (50 al nacer, el
// comando 751 lo cambia) y al llegar manda una sola vez el aviso de
// maximo excedido. Escrito del asm de la A124 (JuiceErrorManager.o: .text
// 0x4B3CB8, 8 etiquetas). Unidad solo enlaza en A124. Hereda de Processor
// (vptr a +0x0C, ABI viejo).

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
};

class GameHook {
public:
    static GameHook *(*Instance)();

    int m_bConnected;       // +0x00
    int m_iFrameCount;      // +0x04
    int m_iUnused8;         // +0x08
    int m_iUnused12;        // +0x0C
    int m_iUnused16;        // +0x10
    cJuiceCom *m_pJuiceCom; // +0x14
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
    cJuiceCom *m_pCom; // +0x00
    int m_bEnabled;    // +0x04
    int m_bActive;     // +0x08
};

class ErrorManager : public Processor {
public:
    ErrorManager();
    static ErrorManager *Instance();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();
    void AssertLog(int iLine, const char *pFunction, const char *pFile, const char *pCondition, const char *pMessage);

private:
    int m_iMaxAsserts;   // +0x10
    int m_iAssertCount;  // +0x14
    int m_bMaxExceeded;  // +0x18
    int m_iUnused28;     // +0x1C
};

ErrorManager::ErrorManager() {
    m_iMaxAsserts = 50;
    m_bEnabled = 0;
    m_bMaxExceeded = 0;
    m_iAssertCount = 0;
}

void ErrorManager::Initialize(cJuiceCom *pCom) {
    Processor::Initialize(pCom);
    m_pCom->NewMsg((Command)706, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

ErrorManager *ErrorManager::Instance() {
    static ErrorManager errorManagerInstance;
    return &errorManagerInstance;
}

void ErrorManager::AssertLog(int iLine, const char *pFunction, const char *pFile, const char *pCondition,
                             const char *pMessage) {
    if (m_iAssertCount < m_iMaxAsserts) {
        m_pCom->NewMsg((Command)704, (Channels)0);
        m_pCom->PutInt(GameHook::Instance()->m_iFrameCount, (Channels)0);
        m_pCom->PutInt(iLine, (Channels)0);
        m_pCom->PutString(pFunction, (Channels)0);
        m_pCom->PutString(pFile, (Channels)0);
        m_pCom->PutString(pCondition, (Channels)0);
        m_pCom->PutString(pMessage, (Channels)0);
        m_pCom->EndMsg((Channels)0);
        if (++m_iAssertCount < m_iMaxAsserts) {
            return;
        }
    }
    if (m_bMaxExceeded) {
        return;
    }
    m_bMaxExceeded = 1;
    m_pCom->NewMsg((Command)704, (Channels)0);
    m_pCom->PutInt(0, (Channels)0);
    m_pCom->PutString("AssertLog", (Channels)0);
    m_pCom->PutString("JuiceErrorManager.cpp", (Channels)0);
    m_pCom->PutString("ASSERT MAX excee", (Channels)0);
    m_pCom->PutString("ASSERT MAX exceeded, No more juice asserts will be sent from this console for this session.",
                      (Channels)0);
    m_pCom->EndMsg((Channels)0);
}

void ErrorManager::ProcessMessage(int iMsg) {
    switch (iMsg) {
    case 751:
        m_iMaxAsserts = m_pCom->GetInt((Channels)0);
        m_bEnabled = 1;
        break;
    default:
        DebugSystem::WarningOnMsg(201, "void Juice::ErrorManager::ProcessMessage(int)",
                                   "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceErrorManager.cpp", iMsg,
                                   "Invalid command %d was sent to the Error Manager.");
        break;
    }
}

void ErrorManager::Disable() {
}

void ErrorManager::Update() {
}

}
