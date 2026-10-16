#include "ENISCopCarDoors.hpp"

#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Animation/AnimPlayer.hpp"

ENISCopCarDoors::ENISCopCarDoors(int pCarDoor, float pCarDoorPos, float pCarDoorAnimLength, float pCarDoorEndPos) : Event(0x20), fCarDoor(pCarDoor), fCarDoorPos(pCarDoorPos), fCarDoorAnimLength(pCarDoorAnimLength), fCarDoorEndPos(pCarDoorEndPos) {
    if (fCarDoor >= 0 && fCarDoor < 4) {
        StartCopDoorAnim(fCarDoor, fCarDoorPos, fCarDoorAnimLength, fCarDoorEndPos);
    }
}

ENISCopCarDoors::~ENISCopCarDoors() {
}

const char *ENISCopCarDoors::GetEventName() const {
    return "ENISCopCarDoors";
}

void ENISCopCarDoors_MakeEvent_Callback(const void *staticData) {
    new ENISCopCarDoors(((ENISCopCarDoors::StaticData *) staticData)->fCarDoor, ((ENISCopCarDoors::StaticData *) staticData)->fCarDoorPos, ((ENISCopCarDoors::StaticData *) staticData)->fCarDoorAnimLength, ((ENISCopCarDoors::StaticData *) staticData)->fCarDoorEndPos);
}

int ENISCopCarDoors_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 4) {
        new ENISCopCarDoors((int) lua_tonumber(luaState, 1), lua_tonumber(luaState, 2), lua_tonumber(luaState, 3), lua_tonumber(luaState, 4));
    }
    return 0;
}

void ENISCopCarDoors_ResolveEvent_Callback(void *event, const UGroup *group) {
}
