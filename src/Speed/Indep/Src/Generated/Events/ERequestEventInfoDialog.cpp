#include "ERequestEventInfoDialog.hpp"

#include <new>

#include "Speed/Indep/Libs/Support/Miscellaneous/CARP.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Lua/source/lua.h"

ERequestEventInfoDialog::ERequestEventInfoDialog(int pJoyPort, GRuntimeInstance *pRaceActivity) : Event(0x10), fJoyPort(pJoyPort), fRaceActivity(pRaceActivity) {
}

ERequestEventInfoDialog::~ERequestEventInfoDialog() {
#ifdef EA_BUILD_A124
    cFEng::Get()->QueuePackageSwitch("EngageEventDialog.fng", (int) fRaceActivity, 0, false); // la A124 cambia de paquete
#else
    cFEng::Get()->QueuePackagePush("EngageEventDialog.fng", (int) fRaceActivity, 0, false);
#endif
}

const char *ERequestEventInfoDialog::GetEventName() const {
    return "ERequestEventInfoDialog";
}

void ERequestEventInfoDialog_MakeEvent_Callback(const void *staticData) {
    new ERequestEventInfoDialog(((ERequestEventInfoDialog::StaticData *) staticData)->fJoyPort, ((ERequestEventInfoDialog::StaticData *) staticData)->fRaceActivity);
}

int ERequestEventInfoDialog_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 2) {
        new ERequestEventInfoDialog((int) lua_tonumber(luaState, 1), (GRuntimeInstance *) lua_tostring(luaState, 2));
    }
    return 0;
}

void ERequestEventInfoDialog_ResolveEvent_Callback(void *event, const UGroup *group) {
    new (&((ERequestEventInfoDialog::StaticData *) event)->fRaceActivity) CARP::TagReference(group);
}
