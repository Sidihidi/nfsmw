#include "Speed/Indep/Src/Generated/Events/EPlayRaceNIS.hpp"

#include "EPlayDDFlyInNIS.hpp"

#include "Speed/Indep/Src/Gameplay/GMarker.h"
#include "Speed/Indep/Src/Lua/source/lua.h"

EPlayDDFlyInNIS::EPlayDDFlyInNIS() : Event(0x10) {
}

EPlayDDFlyInNIS::~EPlayDDFlyInNIS() {
    new EPlayRaceNIS(NULL, "IntroNisFlyInDD", "Intro", 0, 0, "", "");
}

const char *EPlayDDFlyInNIS::GetEventName() const {
    return "EPlayDDFlyInNIS";
}

void EPlayDDFlyInNIS_MakeEvent_Callback(const void *staticData) {
    new EPlayDDFlyInNIS();
}

int EPlayDDFlyInNIS_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 0) {
        new EPlayDDFlyInNIS();
    }
    return 0;
}

void EPlayDDFlyInNIS_ResolveEvent_Callback(void *event, const UGroup *group) {
}
