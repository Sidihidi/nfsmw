// Juice 1.5.2a: JuiceDebugSystem.cpp, el aviso de mensaje invalido que
// usan los ProcessMessage de los gestores. Escrito del asm de la A124
// (JuiceDebugSystem.o: .text 0x4B1AA0, 1 etiqueta). Unidad solo enlaza en
// A124. Formatea el mensaje con sprintf en un buffer local de 1000 B, lo
// pasa a AssertLog del GameHook con la condicion "false", manda el
// comando 10 con un entero 1 por el canal 0 y desactiva Juice.

#include <stdio.h>

namespace Juice {

enum Command {
};
enum Channels {
};

class cJuiceCom {
public:
    void NewMsg(Command cmd, Channels channel);
    void PutInt(int i, Channels channel);
    void EndMsg(Channels channel);
    void Send(Channels channel);
};

class GameHook {
public:
    static GameHook *(*Instance)();
    void AssertLog(int iLine, const char *pFunction, const char *pFile, const char *pCondition, const char *pMessage);
    void DisableJuice();

    int m_iUnused0;
    int m_iUnused4;
    int m_iUnused8;
    int m_iUnused12;
    int m_iUnused16;
    cJuiceCom *m_pJuiceCom;
};

class DebugSystem {
public:
    static void WarningOnMsg(int iLine, const char *pFunction, const char *pFile, int iMsg, const char *pMessage);
};

void DebugSystem::WarningOnMsg(int iLine, const char *pFunction, const char *pFile, int iMsg, const char *pMessage) {
    char buffer[1000];
    cJuiceCom *pCom;

    sprintf(buffer, pMessage, iMsg);
    GameHook::Instance()->AssertLog(iLine, pFunction, pFile, "false", buffer);
    pCom = GameHook::Instance()->m_pJuiceCom;
    pCom->NewMsg((Command)10, (Channels)0);
    pCom->PutInt(1, (Channels)0);
    pCom->EndMsg((Channels)0);
    pCom->Send((Channels)0);
    GameHook::Instance()->DisableJuice();
}

}
