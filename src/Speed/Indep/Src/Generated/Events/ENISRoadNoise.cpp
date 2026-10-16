#include "ENISRoadNoise.hpp"

#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Main/EventDynamicData.h"

extern int Tweak_DisableRoadNoise;

ENISRoadNoise::ENISRoadNoise(int pDisableRoadNoise) : Event(0x10), fDisableRoadNoise(pDisableRoadNoise) {
    Tweak_DisableRoadNoise = pDisableRoadNoise;
}

ENISRoadNoise::~ENISRoadNoise() {}

const char *ENISRoadNoise::GetEventName() const {
    return "ENISRoadNoise";
}

void ENISRoadNoise_MakeEvent_Callback(const void *staticData) {
    new ENISRoadNoise(((ENISRoadNoise::StaticData *)staticData)->fDisableRoadNoise);
}

int ENISRoadNoise_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 1) {
        new ENISRoadNoise((int)lua_tonumber(luaState, 1));
    }
    return 0;
}

void ENISRoadNoise_ResolveEvent_Callback(void *event, const UGroup *group) {}
