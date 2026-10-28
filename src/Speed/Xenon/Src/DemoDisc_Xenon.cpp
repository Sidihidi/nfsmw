// DemoDisc_Xenon.cpp: la demo disc de Xbox 360, en zPlatform tras Platform_P.cpp. El EGM expande Init dentro de
// InitializeEverything (0x823DB2A8-0x823DB2D8, sobre TheDemoDiscManager en 0x82C953E8): motivo -1 en +0xC,
// RealTimer en +4 y 0/0/10 en +0x1A/+0x1C/+0x1E, que es exactamente el cuerpo de DemoDisc_PC.cpp. SetEndReason
// solo guarda el motivo en +0xC (la guarda, expandida, p. ej. en 0x824704F0: stw r4, 0xC(r11)).
#include "Speed/Indep/Src/Misc/DemoDisc.hpp"

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
