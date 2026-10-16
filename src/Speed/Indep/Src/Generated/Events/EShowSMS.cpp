#include "EShowSMS.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Generated/Events/ESndGameState.hpp"
#include "Speed/Indep/Src/Lua/source/lua.h"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#endif

EShowSMS::EShowSMS(int pShowMsg) : Event(0x10), fShowMsg(pShowMsg) {
    if (FEManager::IsOkayToRequestPauseSimulation(0, true
#ifndef EA_BUILD_A124
                                                  ,
                                                  false
#endif
                                                  )) {
        new ESndGameState(8, true);
#ifdef EA_BUILD_A124
        SetSoundControlState(true, SNDSTATE_FE_SMS_MESSAGE, "EShowSMS");
#endif

        int port = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(0));

        if (fShowMsg > -1) {
            SMSMessage *msg = FEDatabase->GetCareerSettings()->GetSMSMessage(fShowMsg);

            if (!cFEng::Get()->IsPackagePushed("SMS_Mailboxes.fng")) {
                cFEng::Get()->QueuePackagePush("SMS_Mailboxes.fng", (int) msg, port, false);
            }
        } else {
            if (!cFEng::Get()->IsPackagePushed("SMS_Mailboxes.fng")) {
                cFEng::Get()->QueuePackagePush("SMS_Mailboxes.fng", 0, port, false);
            }
        }
    }
}

EShowSMS::~EShowSMS() {
}

const char *EShowSMS::GetEventName() const {
    return "EShowSMS";
}

void EShowSMS_MakeEvent_Callback(const void *staticData) {
    new EShowSMS(((EShowSMS::StaticData *) staticData)->fShowMsg);
}

int EShowSMS_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 1) {
        new EShowSMS((int) lua_tonumber(luaState, 1));
    }
    return 0;
}

void EShowSMS_ResolveEvent_Callback(void *event, const UGroup *group) {
}
