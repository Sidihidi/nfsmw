// Juice 1.5.2a: JuiceMutex.cpp, mutex reentrante sobre un semaforo del
// kernel de la EE. Escrito del asm de la A124 (JuiceMutex.o: .text 0x4B5598,
// 4 etiquetas). Unidad solo enlaza en A124. El dueno y la cuenta de bloqueos
// van en el objeto (offsets del asm); solo el hilo dueno incrementa la cuenta
// sin pasar por el semaforo.

#include <string.h>

#include "eekernel.h"

namespace Juice {

class JuiceMutex {
public:
    JuiceMutex();
    void Create();
    void Lock();
    void Unlock();

private:
    int m_bUncreated; // +0x00
    int m_iOwner;     // +0x04
    int m_iLockCount; // +0x08
    int m_iSema;      // +0x0C
    int m_bHeld;      // +0x10
};

JuiceMutex::JuiceMutex() {
    memset(this, 0, 0x10);
    m_bUncreated = 1;
    m_bHeld = 0;
}

void JuiceMutex::Create() {
    SemaParam semaParam;

    m_bHeld = 0;
    m_iLockCount = 0;
    m_iOwner = 0;
    semaParam.maxCount = 1;
    semaParam.initCount = 1;
    m_iSema = CreateSema(&semaParam);
    m_bUncreated = 0;
}

void JuiceMutex::Lock() {
    m_bHeld = 1;
    if (m_iOwner == GetThreadId()) {
        m_iLockCount++;
    } else {
        WaitSema(m_iSema);
        m_iOwner = GetThreadId();
        m_iLockCount++;
    }
}

void JuiceMutex::Unlock() {
    m_bHeld = 0;
    if (m_iOwner == GetThreadId()) {
        if (--m_iLockCount != 0)
            return;
        m_iOwner = 0;
    }
    SignalSema(m_iSema);
}

}
