#include "Speed/Indep/Src/Misc/DemoDisc.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"

DemoDiscManager TheDemoDiscManager;

// Los tres Timer los pone a cero su propio constructor por defecto.
DemoDiscManager::DemoDiscManager() {
    nDemoDiscEndReason = static_cast<DemoDiscEndReason>(-1);
    MasterVolumeScale = 10;
    NumDemoLoops = 0;
    bCalledSCEADemoStart = false;
    Language = 0;
    Aspect = 0;
    PlayMode = 0;
    InactiveTimeout = 0;
    TotalTimeout = 0;
}

void DemoDiscManager::Init(int argc, char **argv) {
    LastActivityTime = RealTimer;
}

void DemoDiscManager::SetEndReason(DemoDiscEndReason end_reason) {
    nDemoDiscEndReason = end_reason;
}
