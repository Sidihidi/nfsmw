#include "EShowAutoSaveEngage.hpp"

#include "Speed/Indep/Src/Interfaces/IFengHud.h"
#include "Speed/Indep/Src/Interfaces/IMenuZoneTrigger.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Lua/source/lua.h"

EShowAutoSaveEngage::EShowAutoSaveEngage() : Event(0x10) {
}

EShowAutoSaveEngage::~EShowAutoSaveEngage() {
    IPlayer *player = IPlayer::First(PLAYER_LOCAL);

    if (player) {
        IMenuZoneTrigger *izone;

        if (player->GetHud()->QueryInterface(&izone)) {
            izone->EnterTriggerForAutoSave();
        }
    }
}

const char *EShowAutoSaveEngage::GetEventName() const {
    return "EShowAutoSaveEngage";
}

void EShowAutoSaveEngage_MakeEvent_Callback(const void *staticData) {
    new EShowAutoSaveEngage();
}

int EShowAutoSaveEngage_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 0) {
        new EShowAutoSaveEngage();
    }
    return 0;
}

void EShowAutoSaveEngage_ResolveEvent_Callback(void *event, const UGroup *group) {
}
