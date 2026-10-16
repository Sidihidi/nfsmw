#include "ETips.hpp"

#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Main/EventDynamicData.h"

ETips::ETips(int pPlayerIndex, int pTipNum) : Event(0x10), fPlayerIndex(pPlayerIndex), fTipNum(pTipNum) {}

ETips::~ETips() {}

const char *ETips::GetEventName() const {
    return "ETips";
}

void ETips_MakeEvent_Callback(const void *staticData) {
    new ETips(((ETips::StaticData *)staticData)->fPlayerIndex, ((ETips::StaticData *)staticData)->fTipNum);
}

int ETips_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 2) {
        new ETips((int)lua_tonumber(luaState, 1), (int)lua_tonumber(luaState, 2));
    }
    return 0;
}

void ETips_ResolveEvent_Callback(void *event, const UGroup *group) {}
