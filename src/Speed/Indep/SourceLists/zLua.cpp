// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// datos-gc 2026-10-14: en la CU original los tres primeros estaticos dinamicos
// ajenos a los hash de WorldConn son MGeneric (_.tmp_0), MAudioReflection (_13)
// y VehicleParams::TypeName (_14); sin ellos MNISComplete y las demas guardas
// duenyas del slice (_15.._18, _26, _27) caen tres numeros antes. Solo GameCube
// (contador _.tmp_ del slice), como los prefijos de zWorld/zMain.
#ifdef EA_PLATFORM_GAMECUBE
#include "Speed/Indep/Src/Generated/Messages/MGeneric.h"
#include "Speed/Indep/Src/Generated/Messages/MAudioReflection.h"
#include "Speed/Indep/Src/Physics/PVehicle.h"
#endif

#include "Speed/Indep/Src/Lua/LuaRuntime.cpp"

#include "Speed/Indep/Src/Lua/LuaPostOffice.cpp"

#include "Speed/Indep/Src/Lua/LuaBindery.cpp"

#include "Speed/Indep/Src/Lua/LuaAttributes.cpp"

#include "Speed/Indep/Src/Lua/LuaGameHooks.cpp"

extern "C" {
#include "Speed/Indep/Src/Lua/source/lapi.c"

#include "Speed/Indep/Src/Lua/source/ldebug.c"

#include "Speed/Indep/Src/Lua/source/ldo.c"

#include "Speed/Indep/Src/Lua/source/ldump.c"

#include "Speed/Indep/Src/Lua/source/lfunc.c"

#include "Speed/Indep/Src/Lua/source/lmem.c"

#include "Speed/Indep/Src/Lua/source/lobject.c"

#include "Speed/Indep/Src/Lua/source/lopcodes.c"

#include "Speed/Indep/Src/Lua/source/lstate.c"

#include "Speed/Indep/Src/Lua/source/lstring.c"

#include "Speed/Indep/Src/Lua/source/ltable.c"

#include "Speed/Indep/Src/Lua/source/ltm.c"

#include "Speed/Indep/Src/Lua/source/lundump.c"

#include "Speed/Indep/Src/Lua/source/lvm.c"

#include "Speed/Indep/Src/Lua/source/lzio.c"

#include "Speed/Indep/Src/Lua/source/lib/lauxlib.c"

#include "Speed/Indep/Src/Lua/source/llex.c"

#include "Speed/Indep/Src/Lua/source/lparser.c"

#include "Speed/Indep/Src/Lua/source/lgc.c"
}
