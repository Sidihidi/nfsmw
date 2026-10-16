// In the PS2 prototype (SLES-53558-A124) this unit starts with the debug draw primitives: its .mdebug puts
// debug/common/debugdraw.cpp before debuggraph.cpp. GameCube and the retail PS2 build have none of them.
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Debug/Common/DebugDraw.cpp"
#endif
#include "Speed/Indep/Src/Debug/Common/DebugGraph.cpp"
