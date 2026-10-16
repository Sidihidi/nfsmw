#include "EReloadHud.hpp"

#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Sim/Simulation.h"

EReloadHud::EReloadHud() : Event(0x10) {
}

EReloadHud::~EReloadHud() {
    for (IPlayer::List::const_iterator iter = IPlayer::GetList(PLAYER_LOCAL).begin(); iter != IPlayer::GetList(PLAYER_LOCAL).end(); ++iter) {

        IPlayer *player = const_cast<IPlayer *>(*iter);

        if (Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) {

            bool isDrag = false;
#ifndef EA_BUILD_A124
            bool isChallenge = false;
#endif

            if (GRaceStatus::Exists()) {
                isDrag = GRaceStatus::IsDragRace();
#ifndef EA_BUILD_A124
                isChallenge = GRaceStatus::IsChallengeRace();
#endif
            }

            if (player == IPlayer::First(PLAYER_LOCAL)) {

                if (isDrag) {
                    player->SetHud(PHT_DRAG_SPLIT1);
                } else {
                    player->SetHud(PHT_SPLIT1);
                }
            } else {

                if (isDrag) {
                    player->SetHud(PHT_DRAG_SPLIT2);
                } else {
                    player->SetHud(PHT_SPLIT2);
                }
            }

#ifndef EA_BUILD_A124
            // La A124 aun no pone el sonido de la intro ni el del reinicio.
            if (!isChallenge) {
                g_pEAXSound->QueueNISButtonThrough(bStringHash("IntroNisDDEnd"), 0);
            }
#endif
        } else if (GRaceStatus::Exists()) {
#ifndef EA_BUILD_A124
            bool isChallenge = GRaceStatus::IsChallengeRace();
#endif

            if (GRaceStatus::Get().GetRaceType() == GRace::kRaceType_Drag) {
                player->SetHud(PHT_DRAG);
            } else {
                player->SetHud(PHT_STANDARD);
            }

#ifndef EA_BUILD_A124
            if (!isChallenge && GRaceStatus::Get().GetRaceType() != GRace::kRaceType_None) {
                g_pEAXSound->QueueNISButtonThrough(bStringHash("RESTART_FAKE"), -1);
            }
#endif
        } else {
            player->SetHud(PHT_STANDARD);
        }
    }
}

const char *EReloadHud::GetEventName() const {
    return "EReloadHud";
}

void EReloadHud_MakeEvent_Callback(const void *staticData) {
    new EReloadHud();
}

int EReloadHud_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 0) {
        new EReloadHud();
    }
    return 0;
}

void EReloadHud_ResolveEvent_Callback(void *event, const UGroup *group) {
}
