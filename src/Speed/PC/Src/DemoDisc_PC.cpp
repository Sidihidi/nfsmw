// DemoDisc_PC.cpp: el DemoDisc_G.cpp/DemoDisc_PS2.cpp de PC. La demo disc existe en PC: Init fija el motivo de
// fin a NONE, toma el tiempo actual como ultima actividad y los timeouts de la build de tienda (10 de volumen
// maestro); SetEndReason solo guarda el motivo.
#include "Speed/Indep/Src/Misc/DemoDisc.hpp"

// El objeto global lo declara la corriente de datos de zPlatform (extern aqui para no duplicar su definicion).
extern DemoDiscManager TheDemoDiscManager;

void DemoDiscManager::Init(int argc, char **argv) {
    this->nDemoDiscEndReason = DEMO_DISC_ENDREASON_NONE;
    this->LastActivityTime = RealTimer;
    this->InactiveTimeout = 0;
    this->TotalTimeout = 0;
    this->MasterVolumeScale = 10;
}

void DemoDiscManager::SetEndReason(DemoDiscEndReason end_reason) {
    this->nDemoDiscEndReason = end_reason;
}
