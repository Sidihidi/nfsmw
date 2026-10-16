#include "EDebugPrint.hpp"

#include <new>

#include "Speed/Indep/Src/Lua/source/lua.h"

EDebugPrint::EDebugPrint(const char *pDebugMsg) : Event(0x10), fDebugMsg(EventManager::EmbedField(this, pDebugMsg)) {
}

EDebugPrint::~EDebugPrint() {
}

const char *EDebugPrint::GetEventName() const {
    return "EDebugPrint";
}

void EDebugPrint_MakeEvent_Callback(const void *staticData) {
    new EDebugPrint(((EDebugPrint::StaticData *) staticData)->fDebugMsg);
}

int EDebugPrint_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 1) {
        new EDebugPrint(lua_tostring(luaState, 1));
    }
    return 0;
}

void EDebugPrint_ResolveEvent_Callback(void *event, const UGroup *group) {
    new (&((EDebugPrint::StaticData *) event)->fDebugMsg) CARP::TagReference(group);
}
