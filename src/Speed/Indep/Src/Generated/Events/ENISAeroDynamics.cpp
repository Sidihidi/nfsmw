#include "ENISAeroDynamics.hpp"

#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Main/EventDynamicData.h"

extern float gNIS_AeroDynamics;

ENISAeroDynamics::ENISAeroDynamics(float pAerodynamics) : Event(0x10), fAerodynamics(pAerodynamics) {
    gNIS_AeroDynamics = pAerodynamics;
}

ENISAeroDynamics::~ENISAeroDynamics() {}

const char *ENISAeroDynamics::GetEventName() const {
    return "ENISAeroDynamics";
}

void ENISAeroDynamics_MakeEvent_Callback(const void *staticData) {
    new ENISAeroDynamics(((ENISAeroDynamics::StaticData *)staticData)->fAerodynamics);
}

int ENISAeroDynamics_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 1) {
        new ENISAeroDynamics(lua_tonumber(luaState, 1));
    }
    return 0;
}

void ENISAeroDynamics_ResolveEvent_Callback(void *event, const UGroup *group) {}
