// Juice 1.5.2a: JuiceLogging.cpp, el gestor de registro: reenvia la salida
// de depuracion al servidor (comando 202) y recorta a 768 B las cadenas
// mas largas avisando con el texto JUICE CROP. Escrito del asm de la A124
// (JuiceLogging.o: .text 0x4B4B10, 10 etiquetas). Unidad solo enlaza en
// A124. Hereda de Processor (vptr a +0x0C, ABI viejo) y no anade miembros;
// las banderas heredadas de +4/+8 son los interruptores de 251/252 y la
// de +8 es la que consulta SendLogDebugOutput.

#include <stdio.h>
#include <string.h>

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
    void PutString(const char *pString, Channels channel);
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

class LoggingManager : public Processor {
public:
    LoggingManager();
    static LoggingManager *Instance();
    virtual void Initialize(cJuiceCom *pCom);
    virtual void Disable();
    virtual void ProcessMessage(int iMsg);
    virtual void Update();
    void SendLogCommand(Command cmd);
    void SendLogDebugOutput(const char *pString);
    void SendLogOutput(const char *pString, Command cmd);
};

LoggingManager::LoggingManager() {
    m_bActive = 0;
    m_bEnabled = 0;
}

LoggingManager *LoggingManager::Instance() {
    static LoggingManager loggingManagerInstance;
    return &loggingManagerInstance;
}

void LoggingManager::ProcessMessage(int iMsg) {
    switch (iMsg) {
    case 251:
        m_bActive = 1;
        m_bEnabled = 1;
        break;
    case 252:
        m_bEnabled = 1;
        m_bActive = 0;
        break;
    default:
        DebugSystem::WarningOnMsg(65, "void Juice::LoggingManager::ProcessMessage(int)",
                                   "C:\\work\\Juice_Clean\\packages\\Juice\\1.5.X\\src\\JuiceLogging.cpp", iMsg,
                                   "Invalid command %d was sent to the Logging Manager.");
        break;
    }
}

void LoggingManager::Initialize(cJuiceCom *pCom) {
    Processor::Initialize(pCom);
    SendLogCommand((Command)201);
}

void LoggingManager::Disable() {
}

void LoggingManager::SendLogCommand(Command cmd) {
    m_pCom->NewMsg(cmd, (Channels)0);
    m_pCom->EndMsg((Channels)0);
    m_pCom->Send((Channels)0);
}

void LoggingManager::SendLogDebugOutput(const char *pString) {
    char buffer[768];
    int iLength;

    if (m_bActive) {
        iLength = strlen(pString);
        if (iLength > 768) {
            memcpy(buffer, pString, 768);
            sprintf(&buffer[700], " *** JUICE CROP - %d byte string.\n\n", iLength);
            SendLogOutput(buffer, (Command)202);
        } else {
            SendLogOutput(pString, (Command)202);
        }
    }
}

void LoggingManager::SendLogOutput(const char *pString, Command cmd) {
    m_pCom->NewMsg(cmd, (Channels)0);
    m_pCom->PutString(pString, (Channels)0);
    m_pCom->EndMsg((Channels)0);
}

void LoggingManager::Update() {
}

}
