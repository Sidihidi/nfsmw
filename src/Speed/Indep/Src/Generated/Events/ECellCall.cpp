#include "Speed/Indep/Src/Generated/Events/ESndGameState.hpp"
#include "ECellCall.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Lua/source/lua.h"
#include "Speed/Indep/Src/Main/EventSequencer.h"
#include "Speed/Indep/Src/Speech/MiscSpeech.h"

ECellCall::ECellCall(int pID) : Event(0x10), fID(pID) {
}

ECellCall::~ECellCall() {
#ifdef EA_BUILD_A124
    // La A124 aun no abre el buzon de SMS en los idiomas sin voz: siempre llama por telefono.
    MiscSpeech::SMSCellCall(fID);
    Speech::Manager::Deduce();
#else
    eLanguages currentLanguage = GetCurrentLanguage();

    if (currentLanguage == eLANGUAGE_DUTCH || currentLanguage == eLANGUAGE_DANISH || currentLanguage == eLANGUAGE_FINNISH ||
        currentLanguage == eLANGUAGE_POLISH || currentLanguage == eLANGUAGE_SWEDISH) {
        if (FEManager::IsOkayToRequestPauseSimulation(0, true
#ifndef EA_BUILD_A124
                                                      ,
                                                      false
#endif
                                                      )) {
            SMSMessage *msg = FEDatabase->GetCareerSettings()->GetSMSMessage(fID);
            int port;

            if (msg) {
                msg->ClearFlags();
                msg->SetFlag(SMS_FLAG_READ);
            }

            new ESndGameState(8, true);

            port = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(0));

            if (fID > -1) {
                SMSMessage *msg = FEDatabase->GetCareerSettings()->GetSMSMessage(fID);

                if (!cFEng::Get()->IsPackagePushed("SMS_Mailboxes.fng")) {
                    cFEng::Get()->QueuePackagePush("SMS_Mailboxes.fng", (int) msg, port, false);
                }
            } else {
                if (!cFEng::Get()->IsPackagePushed("SMS_Mailboxes.fng")) {
                    cFEng::Get()->QueuePackagePush("SMS_Mailboxes.fng", 0, port, false);
                }
            }
        }
    } else {
        MiscSpeech::SMSCellCall(fID);
        Speech::Manager::Deduce();
    }
#endif
}

const char *ECellCall::GetEventName() const {
    return "ECellCall";
}

void ECellCall_MakeEvent_Callback(const void *staticData) {
    new ECellCall(((ECellCall::StaticData *) staticData)->fID);
}

int ECellCall_MakeEvent_LuaBinding(lua_State *luaState) {
    if (lua_gettop(luaState) == 1) {
        new ECellCall((int) lua_tonumber(luaState, 1));
    }
    return 0;
}

void ECellCall_ResolveEvent_Callback(void *event, const UGroup *group) {
}
