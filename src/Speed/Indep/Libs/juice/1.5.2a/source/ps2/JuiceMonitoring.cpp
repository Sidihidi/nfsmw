// Juice 1.5.2a: JuiceMonitoring.cpp, el gestor de variables monitorizadas
// y la base MonitoredVariable (las concretas son las plantillas tMonitor<T>
// de la cabecera, que en la A124 no dejan funciones). Escrito del asm de la
// A124 (JuiceMonitoring.o: .text 0x4B4E08, 9 etiquetas). Unidad solo enlaza
// en A124. MonitorManager hereda de Processor (vptr a +0x0C, ABI viejo) y
// guarda hasta 50 punteros a MonitoredVariable; los comandos 151-155
// reciben la direccion de la variable como entero del cJuiceCom. De
// MonitoredVariable solo se conocen los dos slots virtuales (a +0x08 y
// +0x10 de su tabla, puros) y los campos de +4 (interruptor) y +8 (marco
// que fija ModifyFrame); sus nombres no estan en el binario. El __tf de
// MonitoredVariable del original no sale de esta unidad sin inventar un
// constructor fuera de linea que el .text original no tiene: queda sin
// casar (8 de 9 funciones al 100 %).

namespace Juice {

enum Command {
};
enum Channels {
};

class cJuiceCom {
public:
    int GetInt(Channels channel);
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

class MonitoredVariable {
public:
    virtual void SendValue() = 0;
    virtual void SetValue() = 0;
    void ModifyFrame();

    int m_iUnused0;  // +0x00
    int m_bEnabled;  // +0x04
    int m_iFrame;    // +0x08
    // vptr a +0x0C (ABI viejo)
};

class MonitorManager : public Processor {
public:
    MonitorManager();
    static MonitorManager *Instance();
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();
    int GetInt();

private:
    int m_iNumVariables;                    // +0x10
    MonitoredVariable *m_pVariables[50];    // +0x14
};

MonitorManager *MonitorManager::Instance() {
    static MonitorManager monitoringManagerInstance;
    return &monitoringManagerInstance;
}

int MonitorManager::GetInt() {
    return m_pCom->GetInt((Channels)0);
}

void MonitorManager::ProcessMessage(int iMsg) {
    switch (iMsg) {
    case 151:
        ((MonitoredVariable *)m_pCom->GetInt((Channels)0))->m_bEnabled = 1;
        break;
    case 152:
        ((MonitoredVariable *)m_pCom->GetInt((Channels)0))->m_bEnabled = 0;
        break;
    case 153:
        ((MonitoredVariable *)m_pCom->GetInt((Channels)0))->SendValue();
        break;
    case 154:
        ((MonitoredVariable *)m_pCom->GetInt((Channels)0))->SetValue();
        break;
    case 155:
        ((MonitoredVariable *)m_pCom->GetInt((Channels)0))->ModifyFrame();
        break;
    default:
        DebugSystem::WarningOnMsg(115, "void Juice::MonitorManager::ProcessMessage(int)",
                                   "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceMonitoring.cpp", iMsg,
                                   "Invalid command %d was sent to the Monitor Manager.");
        break;
    }
}

void MonitorManager::Update() {
}

void MonitorManager::Disable() {
}

MonitorManager::MonitorManager() {
    int i;

    m_iNumVariables = 0;
    for (i = 49; i >= 0; i--) {
        m_pVariables[i] = 0;
    }
}

void MonitoredVariable::ModifyFrame() {
    m_iFrame = MonitorManager::Instance()->GetInt();
}

}
