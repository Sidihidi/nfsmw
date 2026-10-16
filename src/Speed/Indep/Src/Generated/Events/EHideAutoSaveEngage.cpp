#include "EHideAutoSaveEngage.hpp"

#include "Speed/Indep/Src/Interfaces/IFengHud.h"
#include "Speed/Indep/Src/Interfaces/IMenuZoneTrigger.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Lua/source/lua.h"

EHideAutoSaveEngage::EHideAutoSaveEngage() : Event(0x10) {
}

EHideAutoSaveEngage::~EHideAutoSaveEngage() {
    IPlayer *player = IPlayer::First(PLAYER_LOCAL);

    if (player) {
        IMenuZoneTrigger *izone;

        if (player->GetHud()->QueryInterface(&izone)) {
            izone->ExitTriggerForAutoSave();
        }
    }
}

const char *EHideAutoSaveEngage::GetEventName() const {
    return "EHideAutoSaveEngage";
}

void EHideAutoSaveEngage_MakeEvent_Callback(const void *staticData) {
    new EHideAutoSaveEngage();
}

int EHideAutoSaveEngage_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 0) {
        new EHideAutoSaveEngage();
    }
    return 0;
}

void EHideAutoSaveEngage_ResolveEvent_Callback(void *event, const UGroup *group) {
}
