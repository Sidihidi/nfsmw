// Juice 1.5.2a: JuiceRPCManager.cpp, el gestor de llamadas remotas: al
// inicializar se anuncia con el comando 501 y el unico mensaje que
// procesa (551) invoca un comando de scripting. Escrito del asm de la
// A124 (JuiceRPCManager.o: .text 0x4B8338, 7 etiquetas). Unidad solo
// enlaza en A124. Hereda de Processor (vptr a +0x0C, ABI viejo) y no
// anade miembros.

namespace Juice {

enum Command {
};
enum Channels {
};

class cJuiceCom {
public:
    void NewMsg(Command cmd, Channels channel);
    void EndMsg(Channels channel);
};

class DebugSystem {
public:
    static void WarningOnMsg(int iLine, const char *pFunction, const char *pFile, int iMsg, const char *pMessage);
};

namespace Scripting {
class ICommands {
public:
    static void InvokeCommand();
};
}

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

class RemoteProcedureManager : public Processor {
public:
    RemoteProcedureManager();
    static RemoteProcedureManager *Instance();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();
};

RemoteProcedureManager::RemoteProcedureManager() {
}

RemoteProcedureManager *RemoteProcedureManager::Instance() {
    static RemoteProcedureManager remoteProcedureManagerInstance;
    return &remoteProcedureManagerInstance;
}

void RemoteProcedureManager::Initialize(cJuiceCom *pCom) {
    m_pCom = pCom;
    m_pCom->NewMsg((Command)501, (Channels)0);
    m_pCom->EndMsg((Channels)0);
}

void RemoteProcedureManager::ProcessMessage(int iMsg) {
    switch (iMsg) {
    case 551:
        Scripting::ICommands::InvokeCommand();
        break;
    default:
        DebugSystem::WarningOnMsg(44, "void Juice::RemoteProcedureManager::ProcessMessage(int)",
                                   "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceRPCManager.cpp", iMsg,
                                   "Invalid command %d was sent to the Remote Procedure Manager.");
        break;
    }
}

void RemoteProcedureManager::Update() {
}

void RemoteProcedureManager::Disable() {
}

}
