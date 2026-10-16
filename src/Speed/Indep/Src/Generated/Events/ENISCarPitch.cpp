#include "ENISCarPitch.hpp"

#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Interfaces/Simables/INISCarControl.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Main/EventDynamicData.h"

ENISCarPitch::ENISCarPitch(float pPitch, float pTime, unsigned int phSimable) : Event(0x20), fPitch(pPitch), fTime(pTime), fhSimable(phSimable) {
    ISimable *isimable = ISimable::FindInstance((HSIMABLE) fhSimable);

    if (isimable) {
        INISCarControl *inis;

        if (isimable->QueryInterface(&inis)) {
            inis->SetAnimPitch(fPitch, fTime);
        }
    }
}

ENISCarPitch::~ENISCarPitch() {
}

const char *ENISCarPitch::GetEventName() const {
    return "ENISCarPitch";
}

void ENISCarPitch_MakeEvent_Callback(const void *staticData) {
    new ENISCarPitch(((ENISCarPitch::StaticData *) staticData)->fPitch, ((ENISCarPitch::StaticData *) staticData)->fTime, gEventDynamicData.fhSimable);
}

int ENISCarPitch_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 2) {
        new ENISCarPitch(lua_tonumber(luaState, 1), lua_tonumber(luaState, 2), gEventDynamicData.fhSimable);
    }
    return 0;
}

void ENISCarPitch_ResolveEvent_Callback(void *event, const UGroup *group) {
}
