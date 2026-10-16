// Juice 1.5.2a: JuiceOnlinePlayMgr.cpp, gestor de juego en linea (sin
// trabajo en esta version: Update y Disable estan vacios y ProcessMessage
// solo avisa de que el comando no es valido). Escrito del asm de la A124
// (JuiceOnlinePlayMgr.o: .text 0x4B5768, 6 etiquetas). Unidad solo enlaza
// en A124. Hereda de Processor (vptr a +0x0C, ABI viejo); el objeto mide
// 0x20 B y las cuatro palabras propias a +0x10..+0x1C no las toca ninguna
// funcion de la unidad.

namespace Juice {

class cJuiceCom;

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
    // vptr a +0x0C (ABI viejo)
};

class JuiceOnlinePlayMgr : public Processor {
public:
    JuiceOnlinePlayMgr();
    static JuiceOnlinePlayMgr *Instance();
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();

private:
    int m_iUnused16; // +0x10
    int m_iUnused20; // +0x14
    int m_iUnused24; // +0x18
    int m_iUnused28; // +0x1C
};

JuiceOnlinePlayMgr::JuiceOnlinePlayMgr() {
}

JuiceOnlinePlayMgr *JuiceOnlinePlayMgr::Instance() {
    static JuiceOnlinePlayMgr onlinePlayMgrInstance;
    return &onlinePlayMgrInstance;
}

void JuiceOnlinePlayMgr::ProcessMessage(int iMsg) {
    DebugSystem::WarningOnMsg(24, "void Juice::JuiceOnlinePlayMgr::ProcessMessage(int)",
                               "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceOnlinePlayMgr.cpp", iMsg,
                               "Invalid command %d was sent to the Online Play Manager.");
}

void JuiceOnlinePlayMgr::Update() {
}

void JuiceOnlinePlayMgr::Disable() {
}

}
