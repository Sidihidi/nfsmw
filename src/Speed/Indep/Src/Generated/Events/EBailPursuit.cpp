#include "EBailPursuit.hpp"

#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Speech/SoundAI.h"

EBailPursuit::EBailPursuit(int pPlayAudio, int pDisperseCops) : Event(0x10), fPlayAudio(pPlayAudio), fDisperseCops(pDisperseCops) {
}

EBailPursuit::~EBailPursuit() {
    if (fPlayAudio) {
        SoundAI *ai = SoundAI::Get();

        if (ai) {
            ai->TerminatePursuit(SoundAI::kForcedBail);
        }
    }

    if (fDisperseCops) {
        for (IPursuit::List::const_iterator i = IPursuit::GetList().begin(); i != IPursuit::GetList().end(); ++i) {
            (*i)->BailPursuit();
        }
    }
}

const char *EBailPursuit::GetEventName() const {
    return "EBailPursuit";
}

void EBailPursuit_MakeEvent_Callback(const void *staticData) {
    new EBailPursuit(((EBailPursuit::StaticData *) staticData)->fPlayAudio, ((EBailPursuit::StaticData *) staticData)->fDisperseCops);
}

int EBailPursuit_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 2) {
        new EBailPursuit((int) lua_tonumber(luaState, 1), (int) lua_tonumber(luaState, 2));
    }
    return 0;
}

void EBailPursuit_ResolveEvent_Callback(void *event, const UGroup *group) {
}
