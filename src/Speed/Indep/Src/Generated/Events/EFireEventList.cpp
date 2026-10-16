#include "EFireEventList.hpp"

#include <new>

#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Main/EventDynamicData.h"

EFireEventList::EFireEventList(CARP::EventList *pEventList, int pDelayed, int pVerbose)
    : Event(0x20), fEventList(pEventList), fDelayed(pDelayed), fVerbose(pVerbose) {
    if (fEventList && !fDelayed) {
        EventManager::FireEventList(fEventList, fVerbose);
    }
}

EFireEventList::~EFireEventList() {
    if (fEventList && fDelayed) {
        gEventDynamicData.Clear();

        EventManager::FireEventList(fEventList, fVerbose);
    }
}

const char *EFireEventList::GetEventName() const {
    return "EFireEventList";
}

void EFireEventList_MakeEvent_Callback(const void *staticData) {
    new EFireEventList(((EFireEventList::StaticData *) staticData)->fEventList, ((EFireEventList::StaticData *) staticData)->fDelayed, ((EFireEventList::StaticData *) staticData)->fVerbose);
}

int EFireEventList_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 3) {
        new EFireEventList((CARP::EventList *) lua_tostring(luaState, 1), (int) lua_tonumber(luaState, 2), (int) lua_tonumber(luaState, 3));
    }
    return 0;
}

void EFireEventList_ResolveEvent_Callback(void *event, const UGroup *group) {
    new (&((EFireEventList::StaticData *) event)->fEventList) CARP::TagReference(group);
}
