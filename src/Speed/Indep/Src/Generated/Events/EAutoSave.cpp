#include "EAutoSave.hpp"

#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Lua/source/lua.h"

EAutoSave::EAutoSave() : Event(0x10) {
}

EAutoSave::~EAutoSave() {
    if (MemoryCard::GetInstance()->ShouldDoAutoSave(false)) {
        MemoryCard::GetInstance()->RequestAutoSave();
    }
}

const char *EAutoSave::GetEventName() const {
    return "EAutoSave";
}

void EAutoSave_MakeEvent_Callback(const void *staticData) {
    new EAutoSave();
}

int EAutoSave_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 0) {
        new EAutoSave();
    }
    return 0;
}

void EAutoSave_ResolveEvent_Callback(void *event, const UGroup *group) {
}
