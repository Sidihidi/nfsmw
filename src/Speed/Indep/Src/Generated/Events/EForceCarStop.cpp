#include "EForceCarStop.hpp"

#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Main/EventDynamicData.h"

EForceCarStop::EForceCarStop(int pStopInstantly,
#ifdef EA_BUILD_A124
                             int pStopInNeutral,
#endif
                             unsigned int phSimable)
    : Event(
#ifdef EA_BUILD_A124
          0x20
#else
          0x10
#endif
          ),
      fStopInstantly(pStopInstantly),
#ifdef EA_BUILD_A124
      fStopInNeutral(pStopInNeutral),
#endif
      fhSimable(phSimable) {
}

EForceCarStop::~EForceCarStop() {
    ISimable *isimable = ISimable::FindInstance((HSIMABLE) fhSimable);

    if (isimable) {
        IVehicle *ivehicle;

        if (isimable->QueryInterface(&ivehicle)) {
            char bits = fStopInstantly ? 3 : 1;
#ifdef EA_BUILD_A124
            if (fStopInNeutral) {
                bits |= 8;
            }
#endif

            if (ivehicle) {
                ivehicle->ForceStopOn(bits);
            }
        }
    }
}

const char *EForceCarStop::GetEventName() const {
    return "EForceCarStop";
}

void EForceCarStop_MakeEvent_Callback(const void *staticData) {
    new EForceCarStop(((EForceCarStop::StaticData *) staticData)->fStopInstantly,
#ifdef EA_BUILD_A124
                      ((EForceCarStop::StaticData *) staticData)->fStopInNeutral,
#endif
                      gEventDynamicData.fhSimable);
}

int EForceCarStop_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) ==
#ifdef EA_BUILD_A124
        2
#else
        1
#endif
    ) {
        new EForceCarStop((int) lua_tonumber(luaState, 1),
#ifdef EA_BUILD_A124
                          (int) lua_tonumber(luaState, 2),
#endif
                          gEventDynamicData.fhSimable);
    }
    return 0;
}

void EForceCarStop_ResolveEvent_Callback(void *event, const UGroup *group) {
}
