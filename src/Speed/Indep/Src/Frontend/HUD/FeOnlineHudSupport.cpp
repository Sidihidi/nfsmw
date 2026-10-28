// The online part of the hud. PS2 and Xbox 360 have it: the prototype (SLES-53558-A124) puts
// frontend/hud/feonlinehudsupport.cpp in zFe2, between FeReputation.cpp and FeMinimapStreamer.cpp, and so does the
// Xbox 360 EGM (0x822C09B8-0x822C0D28).
#include "Speed/Indep/Src/Frontend/HUD/FeOnlineHudSupport.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#include "Speed/Indep/Src/Frontend/HUD/FEPkg_Hud.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#if ONLINE_SUPPORT
#include "Speed/Indep/Src/Online/InGame/OnlineRacer.h"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#endif
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#ifdef EA_PLATFORM_XENON
#include "Speed/Indep/Src/Misc/Timer.hpp"
#endif

OnlineHUDSupport::OnlineHUDSupport(const char *pkg_name)
    : HudElement(pkg_name, 0x20), //
      mIGenericMessage(nullptr) {
    pPackageName = pkg_name;
}

OnlineHUDSupport::~OnlineHUDSupport() {}

// The countdown to the end of the race once the first racer has finished.
#ifdef EA_PLATFORM_XENON
// EGM (0x822C0A88): la revision final lee la cuenta una vez y la escribe con Timer::PrintToString, sin icono.
void OnlineHUDSupport::Update(IPlayer *player) {
    if (!mIGenericMessage) {
        IPlayer::First(PLAYER_LOCAL)->GetHud()->QueryInterface(&mIGenericMessage);
    }

    OnlineRacer *racer = TheOnlineManager.GetLocalRacer();
    float countdown = racer->GetEndRaceCountdown();
    if (countdown >= 0.0f) {
        char genericMessageStr[128] = "";

        if (!racer->IsFinishedRacing()) {
            Timer timer(countdown);
            char timeStr[32];
            timer.PrintToString(timeStr, 0);
            bSPrintf(genericMessageStr, "%s!\n%s", GetTranslatedString(0x862a0519), timeStr);
            if (countdown > 10.0f) {
                mIGenericMessage->RequestGenericMessage(genericMessageStr, true, 0x821e6378, 0, 0, GenericMessage_Priority_3);
            } else {
                mIGenericMessage->RequestGenericMessage(genericMessageStr, true, 0x4f79cba2, 0, 0, GenericMessage_Priority_3);
            }
        }
    }
}
#else
void OnlineHUDSupport::Update(IPlayer *player) {
    if (!mIGenericMessage) {
        IPlayer::First(PLAYER_LOCAL)->GetHud()->QueryInterface(&mIGenericMessage);
    }

    // SLUS (sin online) solo conserva lo de arriba, pero con el marco de 0xC0 de la A124: el original dejaba aqui
    // codigo muerto con sus locales; la forma exacta no se conoce.
#if ONLINE_SUPPORT
    OnlineRacer *racer = TheOnlineManager.GetLocalRacer();
    if (racer->GetEndRaceCountdown() >= 0.0f) {
        float countdown = racer->GetEndRaceCountdown();
        char genericMessageStr[128] = "";

        if (!racer->IsFinishedRacing()) {
            bSPrintf(genericMessageStr, "%s\n%1.1f", GetTranslatedString(0x862a0519), countdown);
            if (countdown > 10.0f) {
                mIGenericMessage->RequestGenericMessage(genericMessageStr, true, 0x821e6378, bStringHash("TIMER_ICON"), FEHASH_TIMEBONUS,
                                                        GenericMessage_Priority_3);
            } else {
                mIGenericMessage->RequestGenericMessage(genericMessageStr, true, 0x4f79cba2, bStringHash("TIMER_ICON"), FEHASH_TIMEBONUS,
                                                        GenericMessage_Priority_3);
            }
        }
    }
#endif
}
#endif

void OnlineHUDSupport::DisplayGenericMessage(const char *message) {
    mIGenericMessage->RequestGenericMessage(message, false, FEHASH_FLASHER2, 0, 0, GenericMessage_Priority_1);
}

#if ONLINE_SUPPORT
// Tells the local player that another racer has dropped out.
void FEOnlineDisconnectMsg(OnlineRacer *racer) {
    OnlineHUDSupport *support = static_cast<FEngHud *>(IPlayer::First(PLAYER_LOCAL)->GetHud())->GetOnlineHUDSupport();
    if (support) {
        char genericMessageStr[128] = "";
        bSPrintf(genericMessageStr, GetTranslatedString(0x44abac78), racer->GetPersona());
        support->DisplayGenericMessage(genericMessageStr);
    }
}
#endif
