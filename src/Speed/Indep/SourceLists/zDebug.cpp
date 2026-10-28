// In the PS2 prototype (SLES-53558-A124) this unit starts with the debug draw primitives: its .mdebug puts
// debug/common/debugdraw.cpp before debuggraph.cpp. GameCube and the retail PS2 build have none of them.
// Xenon tambien los trae (cluster EGM 0x821965C8-0x82199670, tras el codigo de Camera), pero
// compilarlo hoy baja ?InitTimers@Internal@Sim por desplazamiento del enlace (x36099): fuera.
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Debug/Common/DebugDraw.cpp"
#endif
#include "Speed/Indep/Src/Debug/Common/DebugGraph.cpp"
