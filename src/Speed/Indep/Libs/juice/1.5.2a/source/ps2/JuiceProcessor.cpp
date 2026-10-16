// Juice 1.5.2a: JuiceProcessor.cpp, la clase base de los gestores de Juice.
// Escrito del asm de la A124 (JuiceProcessor.o: .text 0x4B64B0, 3 etiquetas).
// Unidad solo enlaza en A124. El vptr va a +0x0C porque el ABI viejo de
// ee-gcc lo coloca despues de los miembros de datos, no a +0: a +0 va el
// cJuiceCom que el constructor saca de GameHook::Instance() (+0x14 del
// GameHook), y a +4/+8 dos banderas que nacen a 1. Initialize solo guarda
// el cJuiceCom que le pasan; Disable/ProcessMessage/Update son puros.

namespace Juice {

class cJuiceCom;

class GameHook {
public:
    static GameHook *(*Instance)();

    int m_iUnused0;        // +0x00
    int m_iUnused4;        // +0x04
    int m_iUnused8;        // +0x08
    int m_iUnused12;       // +0x0C
    int m_iUnused16;       // +0x10
    cJuiceCom *m_pJuiceCom; // +0x14
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

Processor::Processor() {
    m_pCom = GameHook::Instance()->m_pJuiceCom;
    m_bEnabled = 1;
    m_bActive = 1;
}

void Processor::Initialize(cJuiceCom *pCom) {
    m_pCom = pCom;
}

}
