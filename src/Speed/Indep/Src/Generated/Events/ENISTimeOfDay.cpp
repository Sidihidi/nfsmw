#include "ENISTimeOfDay.hpp"

#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Main/EventDynamicData.h"

ENISTimeOfDay::ENISTimeOfDay(float pTimeOfDay, float pUpdateRate, float pSunAzimuth, float pTimeOfDayOvercast) : Event(0x20), fTimeOfDay(pTimeOfDay), fUpdateRate(pUpdateRate), fSunAzimuth(pSunAzimuth), fTimeOfDayOvercast(pTimeOfDayOvercast) {}

ENISTimeOfDay::~ENISTimeOfDay() {}

const char *ENISTimeOfDay::GetEventName() const {
    return "ENISTimeOfDay";
}

void ENISTimeOfDay_MakeEvent_Callback(const void *staticData) {
    new ENISTimeOfDay(((ENISTimeOfDay::StaticData *)staticData)->fTimeOfDay, ((ENISTimeOfDay::StaticData *)staticData)->fUpdateRate, ((ENISTimeOfDay::StaticData *)staticData)->fSunAzimuth, ((ENISTimeOfDay::StaticData *)staticData)->fTimeOfDayOvercast);
}

int ENISTimeOfDay_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 4) {
        new ENISTimeOfDay(lua_tonumber(luaState, 1), lua_tonumber(luaState, 2), lua_tonumber(luaState, 3), lua_tonumber(luaState, 4));
    }
    return 0;
}

void ENISTimeOfDay_ResolveEvent_Callback(void *event, const UGroup *group) {}
