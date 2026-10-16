// Juice 1.5.2a: JuiceRemoteFileManager.cpp, gestor de ficheros remotos
// (sin trabajo en esta version: ProcessMessage, Update y Disable estan
// vacios e Initialize solo llama al de Processor). Escrito del asm de la
// A124 (JuiceRemoteFileManager.o: .text 0x4B6720, 7 etiquetas). Unidad
// solo enlaza en A124. Hereda de Processor (vptr a +0x0C, ABI viejo); el
// objeto mide 0x18 B, el constructor pone a cero la palabra propia de
// +0x10 y vuelve a dejar a 1 las dos banderas heredadas de +4/+8.

namespace Juice {

class cJuiceCom;

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

class RemoteFileManager : public Processor {
public:
    RemoteFileManager();
    static RemoteFileManager *Instance();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();

private:
    int m_iUnused16; // +0x10
    int m_iUnused20; // +0x14
};

RemoteFileManager::RemoteFileManager() : m_iUnused16(0) {
    m_bActive = 1;
    m_bEnabled = 1;
}

RemoteFileManager *RemoteFileManager::Instance() {
    static RemoteFileManager remoteFileManagerInstance;
    return &remoteFileManagerInstance;
}

void RemoteFileManager::ProcessMessage(int iMsg) {
}

void RemoteFileManager::Initialize(cJuiceCom *pCom) {
    Processor::Initialize(pCom);
}

void RemoteFileManager::Disable() {
}

void RemoteFileManager::Update() {
}

}
