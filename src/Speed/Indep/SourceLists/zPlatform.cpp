#ifdef EA_PLATFORM_PLAYSTATION2
// Prefijo comun de las unidades de juego del original (como zEcstasy.cpp y zFEng.cpp). Lo prueban los stabs de la
// A124: InitMonitor, las Seek/ReadAsync de bCodeineFile y DemoDiscManager::SetEndReason expanden stl_relops.h.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// El orden es el que da el .mdebug de la A124 (scripts/ficheros_mdebug.py): superbenderpsx2, platform,
// rebootiop, TextureInfoPlat, sune, acidfxe, logitech/, demodisc_ps2, movie_ps2, fastpacketbuffer,
// bcodeinefile, xsprites, xsparks y memorycardimp.
#include "Speed/Indep/bWare/Src/PSX2/SuperBenderPSX2.cpp"

#include "Speed/PSX2/Src/Platform.cpp"

#include "Speed/PSX2/Src/RebootIOP.cpp"

#include "Speed/PSX2/Src/Ecstasy/TextureInfoPlat.cpp"

#include "Speed/PSX2/Src/SunE.cpp"

#include "Speed/PSX2/Src/AcidFXE.cpp"

// logitech/ va en el orden de emision del original, que es el alfabetico al reves.
#include "Speed/PSX2/Src/Logitech/LogiWheels.cpp"

#include "Speed/PSX2/Src/Logitech/LogiRamp.cpp"

#include "Speed/PSX2/Src/Logitech/LogiPeriodic.cpp"

#include "Speed/PSX2/Src/Logitech/LogiForce.cpp"

#include "Speed/PSX2/Src/Logitech/LogiConstant.cpp"

#include "Speed/PSX2/Src/Logitech/LogiCondition.cpp"

#include "Speed/PSX2/Src/Logitech/LGWheels.cpp"

#include "Speed/PSX2/Src/DemoDisc_PS2.cpp"

#include "Speed/PSX2/Src/MOVIE_PS2.cpp"

#include "Speed/Indep/bWare/Src/PSX2/FastPacketBuffer.cpp"

#include "Speed/Indep/bWare/Src/bCodeineFile.cpp"

#include "Speed/PSX2/Src/Ecstasy/xSprites.cpp"

#include "Speed/PSX2/Src/xSparks.cpp"

#include "Speed/PSX2/Src/MemoryCard/MemoryCardImp.cpp"
#endif

#ifdef EA_PLATFORM_GAMECUBE
#include "Speed/GameCube/Src/Platform_G.cpp"

#include "Speed/GameCube/Src/Ecstasy/TextureInfoPlat.cpp"

#include "Speed/GameCube/Src/JoyE.cpp"

#include "Speed/GameCube/Src/Render/SunE.cpp"

#include "Speed/GameCube/Src/globala_bun.cpp"

#include "Speed/GameCube/Src/Render/AcidFX_G.cpp"

#include "Speed/GameCube/Src/DemoDisc_G.cpp"

#include "Speed/GameCube/Src/Movie_GC.cpp"

#include "Speed/GameCube/Src/Logitech/Wheels.cpp"

#include "Speed/GameCube/Src/Logitech/Ramp.cpp"

#include "Speed/GameCube/Src/Logitech/Periodic.cpp"

#include "Speed/GameCube/Src/Logitech/LGWheels.cpp"

#include "Speed/GameCube/Src/Logitech/Force.cpp"

#include "Speed/GameCube/Src/Logitech/Constant.cpp"

#include "Speed/GameCube/Src/Logitech/Condition.cpp"

#include "Speed/GameCube/Src/Ecstasy/xSprites.cpp"

#include "Speed/GameCube/Src/xSparks.cpp"

#include "Speed/GameCube/Src/MemoryCard/MemoryCardImp.cpp"

#endif

#ifdef EA_PLATFORM_WIN32
// PC: la capa de plataforma va entre zPhysicsBehaviors y zSim en el exe (0x6BC750-0x6E79D0), con la parte de PC de
// Ecstasy (TextureInfoPlatInterface, eSolidPlatInterface...), como TextureInfoPlat en GameCube y PS2.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h" // el prefijo comun entero: sin UCollections.h, cl 7.1 hace
#include "Speed/Indep/Libs/Support/Utility/UCollections.h" // dinamicos UMath::PI y compania (el exe no los tiene)

#include "Speed/PC/Src/Ecstasy/TextureInfoPlat.cpp"

#include "Speed/PC/Src/Ecstasy/eEffect.cpp"

#include "Speed/PC/Src/Ecstasy/eSolidPlat.cpp"

#include "Speed/PC/Src/Ecstasy/eMathE.cpp"

#include "Speed/PC/Src/Ecstasy/eLightE.cpp"

#include "Speed/PC/Src/Ecstasy/eViewPlat.cpp"

#include "Speed/PC/Src/Ecstasy/eStrip.cpp"

#include "Speed/PC/Src/Ecstasy/EcstasyE.cpp"

// El cielo de PC (SkyInitModel, InitSkyHash, NotifySkyUnloader, ReplaceSkyTextures...) esta en este rango del exe, no
// en el de zWorld: el codigo de SkyRender.cpp es identico byte a byte al de 0x6C0AB0, 0x6C0D50 y 0x6C0DC0.
#include "Speed/Indep/Src/World/SkyRender.cpp"

#include "Speed/PC/Src/Platform_PC.cpp"

#include "Speed/PC/Src/DemoDisc_PC.cpp"

#include "Speed/PC/Src/MemoryCard/MemoryCardImp.cpp"
#endif
