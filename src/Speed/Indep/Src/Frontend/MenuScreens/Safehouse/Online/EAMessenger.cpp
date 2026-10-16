#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyChat.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"

int gVOIP_InviteState = 0;

void DisplayHLBerrorType(const char *pkgName, int errorNum) {
    char *msg = nullptr;

    switch (errorNum) {
        case 0:
            break;
        case -1: msg = "command is still pending"; break;
        case -2: msg = "not a user"; break;
        case -3: msg = "bad parameter"; break;
        case -4: msg = "server is offline"; break;
        case -8: msg = "item not found etc."; break;
        case -5: msg = GetLocalizedString(0x9a58e01c); break;
        case -6: msg = "unsupported function."; break;
        case -7: msg = "cannot add yourself as buddy."; break;
        case -11: msg = "cannot invite-he has blocked you."; break;
        case -101: msg = "password error"; break;
        case -102: msg = "hack ??? error"; break;
        case -103: msg = "internal error"; break;
        case -104: msg = "bad resource"; break;
        case -105: msg = "network error"; break;
        case -106: msg = "authorization error"; break;
        case -107: msg = "timeout error"; break;
        case -108:
            if (BuddyCore::instance()->GetConnectedState() == disconnected)
                return;
            msg = GetLocalizedString(0x501cdf6f);
            break;
        case -109: msg = "there is no operation --"; break;
        case -110: msg = "some invite answers need right wannabe state."; break;
        case -999: msg = "unknown HLB error.."; break;
        default:
            bSPrintf(msg, "HLB interface returned code :%d", errorNum);
            break;
    }

    if (!msg) {
        msg = GetLocalizedString(0x906b132a);
    }
    DialogInterface::ShowOneButton(pkgName, "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff, msg);
}

int gBuddyListHasChanged = 0;
BuddyApiUserT *gBuddyApiUser = nullptr;
int gPrivateRoomFlag = 0;
BuddySettings gBuddySettings;
const char *gNewBuddyName;

BuddySettings EA_Messenger_GetBuddySettings(HLBBudT *pBuddy) {
    BuddySettings settings;

    bMemSet(&settings, 0, sizeof(settings));

    if (pBuddy) {
        settings.pBuddy = pBuddy;
        bStrCpy(settings.buddyName, HLBBudGetName(pBuddy));
        settings.gameInviteFlags = HLBBudGetGameInviteFlags(pBuddy);
        settings.voipState = HLBBudGetVOIPState(pBuddy);
        settings.canVoiceChat = HLBBudCanVoiceChat(pBuddy);
        settings.isTemporary = HLBBudIsTemporary(pBuddy) | HLBBudTempBuddyIs(pBuddy, HLB_TEMPBUDDYTYPE_SESSION) |
                               HLBBudIsIWannaBeHisBuddy(pBuddy) | HLBBudIsWannaBeMyBuddy(pBuddy);
        settings.isBlocked = HLBBudIsBlocked(pBuddy);
        settings.isRealBuddy = HLBBudIsRealBuddy(pBuddy);
        settings.IsWannaBeMyBuddy = HLBBudIsWannaBeMyBuddy(pBuddy);
        settings.IsIWannaBeHisBuddy = HLBBudIsIWannaBeHisBuddy(pBuddy);

        if (settings.isRealBuddy && (settings.IsWannaBeMyBuddy || settings.IsIWannaBeHisBuddy)) {
            settings.IsWannaBeMyBuddy = 0;
            settings.IsIWannaBeHisBuddy = 0;
        }

        settings.gameBuddyIsPlaying = HLBBudGetTitle(pBuddy);
        settings.isInGroup = HLBBudIsInGroup(pBuddy);
        settings.buddyState = HLBBudGetState(pBuddy);
        settings.isPassive = HLBBudIsPassive(pBuddy);
        settings.IsAvailableForChat = HLBBudIsAvailableForChat(pBuddy);
        settings.IsSameProduct = HLBBudIsSameProduct(pBuddy);
        settings.IsJoinable = HLBBudIsJoinable(pBuddy);
        settings.IsNoReplyBud = HLBBudIsNoReplyBud(pBuddy);
        HLBBudGetPresence(pBuddy, settings.presence);
        HLBBudGetPresenceExtra(pBuddy, settings.presenceExtra, 256);
        settings.unreadMsgCount = HLBMsgListGetUnreadCount(pBuddy);
        settings.totalMsgCount = HLBMsgListGetTotalCount(pBuddy);

        for (int i = 0; i < 5; i++) {
            Invite *pInvite = LobbyChat::Instance().GetReceivedInvite(i);
            if (pInvite) {
                if (!bStrCmp(pInvite->player, settings.buddyName)) {
                    settings.BuddyWantsToPlay = 1;
                    settings.gameIdent = pInvite->gameIdent;
                    break;
                }
            }
        }

        if (LobbyChat::Instance().GetSentInvite(settings.buddyName)) {
            settings.IWannaPlayBuddy = 1;
        }
    }

    return settings;
}

BuddySettings EA_Messenger_GetBuddySettingsByName(const char *name) {
    return EA_Messenger_GetBuddySettings(BuddyCore::instance()->getBuddyByName(name));
}

char PCSearchName[16] = "";
