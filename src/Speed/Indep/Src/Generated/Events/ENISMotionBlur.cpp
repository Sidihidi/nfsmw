#include "ENISMotionBlur.hpp"

#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Main/EventDynamicData.h"

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
extern bool NISMotionBlurEnabled; // ICEManager.cpp
#endif

ENISMotionBlur::ENISMotionBlur(int pEnableMotionBlur) : Event(0x10), fEnableMotionBlur(pEnableMotionBlur) {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    NISMotionBlurEnabled = pEnableMotionBlur != 0; // speed.exe 0x622EFB
#endif
}

ENISMotionBlur::~ENISMotionBlur() {}

const char *ENISMotionBlur::GetEventName() const {
    return "ENISMotionBlur";
}

void ENISMotionBlur_MakeEvent_Callback(const void *staticData) {
    new ENISMotionBlur(((ENISMotionBlur::StaticData *)staticData)->fEnableMotionBlur);
}

int ENISMotionBlur_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 1) {
        new ENISMotionBlur((int)lua_tonumber(luaState, 1));
    }
    return 0;
}

void ENISMotionBlur_ResolveEvent_Callback(void *event, const UGroup *group) {}
