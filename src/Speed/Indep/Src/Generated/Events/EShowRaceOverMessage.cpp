#include "EShowRaceOverMessage.hpp"

#include <new>

#include "Speed/Indep/Src/Camera/CameraAI.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Interfaces/IFengHud.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Sim/Simulation.h"

EShowRaceOverMessage::EShowRaceOverMessage(IPlayer *pPlayer) : Event(0x10), fPlayer(pPlayer) {
}

EShowRaceOverMessage::~EShowRaceOverMessage() {
#ifndef EA_BUILD_A124 // la A124 no comprueba que exista la simulacion
    if (Sim::Exists())
#endif
    {
        GRacerInfo *racerInfo = GRaceStatus::Get().GetRacerInfo(fPlayer->GetSimable());

        if (racerInfo->GetIsTotalled()) {
            CameraAI::MaybeDoTotaledCam(fPlayer);
        }

        IRaceOverMessage *message;

        if (fPlayer->GetHud()->QueryInterface(&message)) {
            message->RequestRaceOverMessage(fPlayer);
        }
    }
}

const char *EShowRaceOverMessage::GetEventName() const {
    return "EShowRaceOverMessage";
}

void EShowRaceOverMessage_MakeEvent_Callback(const void *staticData) {
    new EShowRaceOverMessage(((EShowRaceOverMessage::StaticData *) staticData)->fPlayer);
}

int EShowRaceOverMessage_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 1) {
        new EShowRaceOverMessage((IPlayer *) lua_tostring(luaState, 1));
    }
    return 0;
}

void EShowRaceOverMessage_ResolveEvent_Callback(void *event, const UGroup *group) {
    new (&((EShowRaceOverMessage::StaticData *) event)->fPlayer) CARP::TagReference(group);
}
