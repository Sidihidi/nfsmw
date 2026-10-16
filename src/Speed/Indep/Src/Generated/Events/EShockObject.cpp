#include "EShockObject.hpp"

#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Interfaces/Simables/IDamageable.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Main/EventDynamicData.h"

EShockObject::EShockObject(float pAmount, unsigned int phSimable) : Event(0x10), fAmount(pAmount), fhSimable(phSimable) {
}

EShockObject::~EShockObject() {
    ISimable *iowner = ISimable::FindInstance((HSIMABLE) fhSimable);

    if (iowner) {
        IDamageable *idamage;

        if (iowner->QueryInterface(&idamage)) {
            idamage->SetInShock(fAmount);
        }
    }
}

const char *EShockObject::GetEventName() const {
    return "EShockObject";
}

void EShockObject_MakeEvent_Callback(const void *staticData) {
    new EShockObject(((EShockObject::StaticData *) staticData)->fAmount, gEventDynamicData.fhSimable);
}

int EShockObject_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 1) {
        new EShockObject(lua_tonumber(luaState, 1), gEventDynamicData.fhSimable);
    }
    return 0;
}

void EShockObject_ResolveEvent_Callback(void *event, const UGroup *group) {
}
