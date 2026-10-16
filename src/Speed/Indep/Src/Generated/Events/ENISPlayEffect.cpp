#include "ENISPlayEffect.hpp"

#include <new>

#include "Speed/Indep/Src/Generated/AttribSys/Classes/effects.h"
#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Sim/SimEffect.h"

ENISPlayEffect::ENISPlayEffect(const char *pEffect, float pPositionX, float pPositionY, float pPositionZ, float pMagnitudeX, float pMagnitudeY,
                               float pMagnitudeZ)
    : Event(0x30), fEffect(pEffect), fPositionX(pPositionX), fPositionY(pPositionY), fPositionZ(pPositionZ), fMagnitudeX(pMagnitudeX),
      fMagnitudeY(pMagnitudeY), fMagnitudeZ(pMagnitudeZ) {
}

ENISPlayEffect::~ENISPlayEffect() {
    if (fEffect) {
        const Attrib::Collection *effect = Attrib::FindCollection(Attrib::Gen::effects::ClassKey(), Attrib::StringToKey(fEffect));

        if (effect) {
            UMath::Vector3 magnitude = {-fMagnitudeY, fMagnitudeZ, fMagnitudeX};
            UMath::Vector3 position = {-fPositionY, fPositionZ, fPositionX};

            Sim::Effect::Fire(effect, position, magnitude, 0, NULL, NULL, 0);
        }
    }
}

const char *ENISPlayEffect::GetEventName() const {
    return "ENISPlayEffect";
}

void ENISPlayEffect_MakeEvent_Callback(const void *staticData) {
    new ENISPlayEffect(((ENISPlayEffect::StaticData *) staticData)->fEffect, ((ENISPlayEffect::StaticData *) staticData)->fPositionX,
                       ((ENISPlayEffect::StaticData *) staticData)->fPositionY, ((ENISPlayEffect::StaticData *) staticData)->fPositionZ,
                       ((ENISPlayEffect::StaticData *) staticData)->fMagnitudeX, ((ENISPlayEffect::StaticData *) staticData)->fMagnitudeY,
                       ((ENISPlayEffect::StaticData *) staticData)->fMagnitudeZ);
}

int ENISPlayEffect_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 7) {
        new ENISPlayEffect(lua_tostring(luaState, 1), lua_tonumber(luaState, 2), lua_tonumber(luaState, 3), lua_tonumber(luaState, 4), lua_tonumber(luaState, 5), lua_tonumber(luaState, 6),
                           lua_tonumber(luaState, 7));
    }
    return 0;
}

void ENISPlayEffect_ResolveEvent_Callback(void *event, const UGroup *group) {
    new (&((ENISPlayEffect::StaticData *) event)->fEffect) CARP::TagReference(group);
}
