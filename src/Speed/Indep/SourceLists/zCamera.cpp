// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// ITrafficCenter.h es `#pragma interface`: el original define su vtable y su
// destructor, globales, en esta unidad.
#pragma implementation "ITrafficCenter.h"
// Igual con IListener.h: su vtable, global, en esta unidad.
#pragma implementation "IListener.h"

#include "Speed/Indep/Src/Camera/Camera.cpp"

#include "Speed/Indep/Src/Camera/CameraMover.cpp"

#include "Speed/Indep/Src/Camera/CameraAI.cpp"

#include "Speed/Indep/Src/Camera/ChaseCamAI.cpp"

#include "Speed/Indep/Src/Camera/Actions/CDActionDrive.cpp"

#include "Speed/Indep/Src/Camera/Actions/CDActionTrackCar.cpp"

#include "Speed/Indep/Src/Camera/Actions/CDActionTrackCop.cpp"

#include "Speed/Indep/Src/Camera/Actions/CDActionShowcase.cpp"

#include "Speed/Indep/Src/Camera/Actions/CDActionDebug.cpp"

#include "Speed/Indep/Src/Camera/Actions/CDActionIce.cpp"

#include "Speed/Indep/Src/Camera/Actions/CDActionDebugWatchCar.cpp"

#include "Speed/Indep/Src/Camera/Movers/Cubic.cpp"

#include "Speed/Indep/Src/Camera/Movers/TrackCar.cpp"

#include "Speed/Indep/Src/Camera/Movers/TrackCop.cpp"

#include "Speed/Indep/Src/Camera/Movers/CopView.cpp"

#include "Speed/Indep/Src/Camera/Movers/DebugWorld.cpp"

#include "Speed/Indep/Src/Camera/Movers/Rearview.cpp"

#include "Speed/Indep/Src/Camera/Movers/SelectCar.cpp"

#include "Speed/Indep/Src/Camera/Movers/Showcase.cpp"

#include "Speed/Indep/Src/Camera/Movers/Still.cpp"

#include "Speed/Indep/Src/Camera/ICE/ICEPoint.cpp"

#include "Speed/Indep/Src/Camera/ICE/ICEData.cpp"

#include "Speed/Indep/Src/Camera/ICE/ICEAnchor.cpp"

#include "Speed/Indep/Src/Camera/ICE/ICEMover.cpp"

#include "Speed/Indep/Src/Camera/ICE/ICEManager.cpp"

#include "Speed/Indep/Src/Camera/ICE/ICEAnimScene.cpp"

#include "Speed/Indep/Src/Camera/ICE/ICEOverlays.cpp"

#include "Speed/Indep/Src/Camera/ICE/ICEReplay.cpp"

// El EGM trae los primitivos de DebugDraw tras el codigo de Camera (AllocLists en 0x821965C8,
// DeAllocLists en 0x82196780, hasta Box en 0x82199418), pero compilar DebugDraw.cpp en Xenon
// hoy desplaza el enlace y baja ?InitTimers@Internal@Sim (100 -> 99,93) y los anclas de
// ?Init@Sim/?initnet (medido en x36099, en zCamera y en zDebug): queda fuera hasta que el
// coordinador lo entre con su remapeo de unidad y la lib UMath.
