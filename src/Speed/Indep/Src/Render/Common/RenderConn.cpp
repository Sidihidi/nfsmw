// RenderConn.cpp - top level render service entry points.
//
// These four functions are the render side pump called from the game loop
// (see GameFlow.cpp): they forward to the individual render connections and
// keep the global emitter system and animation player ticking.

#include "Speed/Indep/Src/Render/RenderConn.h"
#include "Speed/Indep/Src/World/VehicleRenderConn.h" // datos-gc: antes llegaba via RenderConn.h->CarRenderConn.h

#include "Speed/Indep/Src/Animation/AnimPlayer.hpp"

// Free hooks implemented by the render connections (SmackableRender.cpp,
// CarRender.cpp, EmitterSystem.cpp).
void SmackableRender_Init();
void SmackableRender_Shutdown();
void SmackableRender_Service(float dT);
void CarRender_Service(float dT);
void ClearXenonEmitters();

namespace RenderConn {

void InitServices() {
    SmackableRender_Init();
}

void RestoreServices() {
    SmackableRender_Shutdown();
    // En PC no existe ClearXenonEmitters (sin simbolo ni definicion en el exe;
    // solo la define el xSparks de GameCube): el original es el tail-call unico
    // a SmackableRender_Shutdown (jmp de 5 B en 0x6E79E0, banco zRender 4/4).
#if !defined(EA_PLATFORM_WIN32) && !defined(EA_BUILD_A124)
    ClearXenonEmitters();
#endif
}

void UpdateLoading() {
    VehicleRenderConn::UpdateLoading();
}

void UpdateServices(float dT) {
    SmackableRender_Service(dT);
    CarRender_Service(dT);
    gEmitterSystem.Update(dT);
    TheAnimPlayer.UpdateTime(dT);
}

} // namespace RenderConn
