#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_MU_Keyboard.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineViewTrack.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyChat.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"
#include "Speed/Indep/Src/Online/LobbyGames.hpp"
#include "Speed/Indep/Src/Online/VoiceCore.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"

// Vecinas de la unidad zFe: uiOnlinePersonalRankings.cpp (gOtherPlayerName es static alli) va antes y
// uiOnlineChallengeInvite.cpp despues. InvitationList y g_inviteList son de uiOnlineMain.cpp.
extern bool bShowPersonalStatsForOtherPlayerOverlay;
extern char gOtherPlayerName[16];
extern ChallengeInfo *gSelfChallengeInfo;
extern bool gSupressSelfChallengeUIHack;

// total size: 0x124
class UIOnlineEAM_Options : public MenuScreen {
  public:
    // total size: 0x1C
    struct OptionScrollerData : public ScrollerDatum {
        int m_index; // offset 0x18, size 0x4

        OptionScrollerData() : m_index(-1) {}
        ~OptionScrollerData() override {}
    };

    enum eCHALLENGE_STATE {
        eCHALLENGE_STATE_NONE = 0,
        eCHALLENGE_STATE_CAN_SEND = 1,
        eCHALLENGE_STATE_SENT = 2,
        eCHALLENGE_STATE_RECIEVED = 3,
    };

    UIOnlineEAM_Options(ScreenConstructorData *sd);
    ~UIOnlineEAM_Options() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup();
    void ScrollOptions(int direction);
    void OptionAction();
    void PopulateOptions();
    void BlockCB(int op, int status);
    void UnBlockCB(int op, int status);
    void InviteBuddy(char *pName);
    void InviteBuddyCB(int op, int status);
    void AnswerFriendRequest();
    void RemoveFriend();
    void RemoveFriendCB(int op, int status);
    void CancelFriendRequest();
    void AnswerInviteCB(int op, int status);
    void EvaluateGameInviteStatus();
    bool EvaluateGameInviteDisplayStatus(char *sztemp);
    void SendInviteCB(int op, int status);
    void CancelInviteCB(int op, int status);
    void AnswerChallenge();
    void ShowWaitingForCallbackDialog(uint32 StringHash, char *pString);
    void CloseWaitingDialog(bool bCancelBuddyOp);
    void DisplayError(int ErrorCode);

    dialog_handle msgHandle;               // offset 0x2C, size 0x4
    bool m_complete;                       // offset 0x30, size 0x4
    Scrollerina m_optionScroller;          // offset 0x34, size 0xC8
    bool m_exitToParent;                   // offset 0xFC, size 0x4
    eCHALLENGE_STATE m_challengeState;     // offset 0x100, size 0x4
    HLBApiRefT *m_HLB_ApiRef;              // offset 0x104, size 0x4
    bool m_enteringChallenge;              // offset 0x108, size 0x4
    bool m_viewingModalDialog;             // offset 0x10C, size 0x4
    bool m_bDontDisplayCancelledCallbacks; // offset 0x110, size 0x4
    bool m_removingFriend;                 // offset 0x114, size 0x4
    bool m_showingXTitleInvite;            // offset 0x118, size 0x4
    bool m_acceptedInvite;                 // offset 0x11C, size 0x4
    bool m_declinedInvite;                 // offset 0x120, size 0x4
};

MenuScreen *CreateUIOnlineEAM_Options(ScreenConstructorData *sd) {
    return new ("UIOnlineEAM_Options", 0) UIOnlineEAM_Options(sd);
}

template <void (UIOnlineEAM_Options::*Callback)(int, int)> struct options_hlb_op_callback {
    static void call(HLBApiRefT *, int op, int status, void *context) {
        (static_cast<UIOnlineEAM_Options *>(context)->*Callback)(op, status);
    }
};

UIOnlineEAM_Options::UIOnlineEAM_Options(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , msgHandle(0) //
    , m_complete(false) //
    , m_optionScroller(GetPackageName(), nullptr, nullptr, true, true, false, false) //
    , m_exitToParent(false) //
    , m_challengeState(eCHALLENGE_STATE_NONE) //
    , m_enteringChallenge(false) //
    , m_viewingModalDialog(false) //
    , m_bDontDisplayCancelledCallbacks(false) //
    , m_removingFriend(false) //
    , m_showingXTitleInvite(false) //
    , m_acceptedInvite(false) //
    , m_declinedInvite(false) {
    m_HLB_ApiRef = BuddyCore::instance()->getApiRef();
    Setup();
    FEPrintf(GetPackageName(), FEngHashString("OPTION_NAME_1"), "%s", gBuddySettings.buddyName);
    m_optionScroller.Update(true);
}

UIOnlineEAM_Options::~UIOnlineEAM_Options() {
    m_bDontDisplayCancelledCallbacks = true;
    CloseWaitingDialog(true);
}

void UIOnlineEAM_Options::ShowWaitingForCallbackDialog(uint32 StringHash, char *pString) {
    char Msg[256];

    bSPrintf(Msg, GetLocalizedString(StringHash), pString);
    msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x1a294dad, 0xca4cb0dd, 0xca4cb0dd, Msg);
}

void UIOnlineEAM_Options::CloseWaitingDialog(bool bCancelBuddyOp) {
    if (msgHandle) {
        feDialogScreen *pDialogScreen = DialogInterface::FindDialogBox(msgHandle);
        if (pDialogScreen && cFEng::Get()->IsPackageInControl(pDialogScreen->GetPackageName())) {
            DialogInterface::DismissDialog(msgHandle);
        } else {
            DialogInterface::DismissDialog(msgHandle);
            cFEng::Get()->QueuePackagePop(1);
        }
        msgHandle = 0;
        cFEng::Get()->EnablePackageControl(cFEng::Get()->FindPackage(GetPackageName()), true);
    }
    if (bCancelBuddyOp) {
        if (BuddyCore::instance()->GetConnectedState() == connected && BuddyCore::instance()->getApiRef()) {
            HLBApiCancelOp(BuddyCore::instance()->getApiRef());
        }
    }
}

void UIOnlineEAM_Options::DisplayError(int ErrorCode) {
    if (!m_bDontDisplayCancelledCallbacks || ErrorCode != -108) {
        DisplayHLBerrorType(GetPackageName(), ErrorCode);
    }
}

void UIOnlineEAM_Options::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (msg == 0xd15c04ec) {
        cFEng::Get()->QueuePackagePop(1);
        return;
    }

    if (EnteringChallenge) {
        m_viewingModalDialog = true;
    }

    if (msg != 0xc98356ba && msg != 0xe1fde1d1 && msg != 0x4fbee42a && msg != 0x7c42d6db) {
        if (m_viewingModalDialog) {
            return;
        }
    }

    switch (msg) {
        case 0xca4cb0dd:
            CloseWaitingDialog(true);
            break;
        case 10: {
            char msg[512] = "";
            FEngSNPrintf(msg, 512, GetLocalizedString(0x8d0a8371), gBuddySettings.buddyName);
            DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 4, 0xffffffff, 0xffffffff,
                                            first_dialog_button2, msg);
            break;
        }
        case 0x1cad26e2:
            if (m_showingXTitleInvite && !m_acceptedInvite && !m_declinedInvite) {
                if (msgHandle) {
                    DialogInterface::DismissDialog(msgHandle);
                    msgHandle = 0;
                }
                m_showingXTitleInvite = false;
                BuddyCore::instance()->DisplayRevokedInvite(gBuddySettings.buddyName);
            }
            break;
        case 0x97c1b72f:
            m_declinedInvite = true;
            HLBListAnswerGameInvite(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, 1, 0, 0);
            FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
            break;
        case 0x4fbee42a:
            if (!gSelfChallengeInfo) {
                for (int i = 0; i < 5; i++) {
                    Invite *info = LobbyChat::Instance().GetReceivedInvite(i);
                    if (!info) {
                        break;
                    }
                    if (!bStrCmp(info->player, gBuddySettings.buddyName)) {
                    }
                }
            }
            m_exitToParent = true;
            m_enteringChallenge = true;
            break;
        case 0xc98356ba:
            BuddyCore::instance()->doprocessing();
            if (gBuddyListHasChanged) {
                HLBBudT *pBud = BuddyCore::instance()->getBuddyByName(gBuddySettings.buddyName);
                if (pBud) {
                    gBuddySettings = EA_Messenger_GetBuddySettings(pBud);
                } else if (!m_removingFriend) {
                    HLBListFlagTempBuddy(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, HLB_TEMPBUDDYTYPE_MESG);
                    gBuddySettings = EA_Messenger_GetBuddySettings(BuddyCore::instance()->getBuddyByName(gBuddySettings.buddyName));
                }
                PopulateOptions();
            }
            if (m_exitToParent) {
                cFEng::Get()->QueuePackagePop(1);
                m_exitToParent = false;
                if (m_enteringChallenge) {
                    cFEng::Get()->QueueGameMessage(0x4fbee42a, "OL_EAMessenger.fng", 0xff);
                }
            }
            break;
        case 0xe1fde1d1:
            cFEng::Get()->QueueGameMessage(0x7a548b3f, "OL_EAMessenger.fng", 0xff);
            m_exitToParent = true;
            break;
        case 0x35f8620b:
            PopulateOptions();
            break;
        case 0x911ab364:
        case 0x34dc1bec:
            FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
            break;
        case 0x911c0a4b:
            ScrollOptions(1);
            break;
        case 0x72619778:
            ScrollOptions(-1);
            break;
        case 0x406415e3:
        case 0x0c407210:
            OptionAction();
            break;
        case 11:
            if (!gVOIP_InviteState) {
                gVOIP_InviteState = 1;
            } else {
                gVOIP_InviteState = 2;
            }
        case 7: {
            if (BuddyCore::instance()->InVoiceChat()) {
                ConnectionCore::Instance().LeaveSession();
                BuddyCore::instance()->SetVoiceChatState(false);
            }
            int port = FEngMapJoyParamToJoyport(param1);
            cFEng::Get()->QueuePackagePush("OL_VoiceChat.fng", 0, port, false);
            break;
        }
        case 6:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }
            if (VoiceCore::instance().IsInVOIPChat(gBuddySettings.buddyName, nullptr)) {
                ConnectionCore::Instance().LeaveSession();
                BuddyCore::instance()->SetVoiceChatState(false);
                FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
                PopulateOptions();
            }
            break;
        case 2:
            if (gBuddySettings.IsWannaBeMyBuddy) {
                if (!HLBListAnswerInvite(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, 0,
                                         options_hlb_op_callback<&UIOnlineEAM_Options::AnswerInviteCB>::call, this)) {
                    ShowWaitingForCallbackDialog(0x91070ff2, gBuddySettings.buddyName);
                }
            } else {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                               GetLocalizedString(0x2106e2fe));
            }
            break;
        case 3:
            if (gBuddySettings.IsWannaBeMyBuddy) {
                if (!HLBListAnswerInvite(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, 1,
                                         options_hlb_op_callback<&UIOnlineEAM_Options::AnswerInviteCB>::call, this)) {
                    ShowWaitingForCallbackDialog(0xc744b7ab, gBuddySettings.buddyName);
                }
            } else {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                               GetLocalizedString(0x2106e2fe));
            }
            break;
        case 4:
            if (gBuddySettings.IsWannaBeMyBuddy) {
                if (!HLBListAnswerInvite(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, 3,
                                         options_hlb_op_callback<&UIOnlineEAM_Options::BlockCB>::call, this)) {
                    ShowWaitingForCallbackDialog(0x7f1025e7, gBuddySettings.buddyName);
                }
            } else {
                cFEng::Get()->QueueGameMessagePkg(8, cFEng::Get()->FindPackage(GetPackageName()));
            }
            break;
        case 5:
            RemoveFriend();
            break;
        case 8: {
            int result = HLBListBlockBuddy(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName,
                                           options_hlb_op_callback<&UIOnlineEAM_Options::BlockCB>::call, this);
            if (result == 0) {
                ShowWaitingForCallbackDialog(0x78300ebf, gBuddySettings.buddyName);
            } else if (result == -5) {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                               GetLocalizedString(0xfbe5ad60));
            }
            break;
        }
        case 9:
            if (!HLBListUnBlockBuddy(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName,
                                     options_hlb_op_callback<&UIOnlineEAM_Options::UnBlockCB>::call, this)) {
                ShowWaitingForCallbackDialog(0xa33aa702, gBuddySettings.buddyName);
            }
            break;
        case 0x7c42d6db:
            if (m_viewingModalDialog) {
                m_viewingModalDialog = false;
                m_optionScroller.HighlightSelected();
            }
            break;
        case 0x9120409e:
        case 0xb5971bf1:
        case 0xc9d30688:
        case 0xda5b8712:
        // El arbol de comparaciones del switch de la A124 pide un caso mas entre 0x4fbee42a y 0x72619778 que salga
        // por el final; el valor no queda en el binario (0x5073ef13 es el que otras pantallas online dejan vacio).
        case 0x5073ef13:
            break;
    }
}

void UIOnlineEAM_Options::Setup() {
    char sztemp[32];

    for (int i = 0; i < 7; i++) {
        ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;

        FEngSNPrintf(sztemp, 32, "option_%d", i + 1);
        FEString *str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pslot->AddData(str);

        m_optionScroller.AddSlot(pslot);
    }
    PopulateOptions();
}

void UIOnlineEAM_Options::ScrollOptions(int direction) {
    if (direction < 0) {
        m_optionScroller.ScrollPrev();
    } else if (direction > 0) {
        m_optionScroller.ScrollNext();
    }
    m_optionScroller.HighlightSelected();
    m_optionScroller.Update(true);
}

void UIOnlineEAM_Options::OptionAction() {
    switch (static_cast<OptionScrollerData *>(m_optionScroller.GetSelectedDatum())->m_index) {
        case 1:
            if ((gBuddySettings.gameInviteFlags & 0x400000) || gBuddySettings.BuddyWantsToPlay) {
                AnswerChallenge();
            } else {
                EvaluateGameInviteStatus();
            }
            break;
        case 0:
            if ((RealTimer - KBCreationTimer).GetSeconds() < g_KBDelaySeconds) {
                break;
            }
            cFEng::Get()->QueuePackagePush("OLX_Message.fng", 0, 0xff, false);
            break;
        case 2: {
            if (cFEng::Get()->FindPackage(g_GameRoomName)) {
                VoiceCore::instance().ToggleMuted(gBuddySettings.buddyName);
                gBuddyListHasChanged = 1;
                break;
            }
            if (gBuddySettings.buddyState != HLB_STAT_CHAT) {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                               GetLocalizedString(0xa424c082), gBuddySettings.buddyName);
                break;
            }
            int channel = 0;
            if (VoiceCore::instance().IsInVOIPChat(gBuddySettings.buddyName, &channel) &&
                VoiceCore::instance().IsConnected(channel)) {
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x417b2601, 0x1a294dad, 6, 0xffffffff,
                                                0xffffffff, first_dialog_button2, GetLocalizedString(0x05997667));
                break;
            }
            if (BuddyCore::instance()->InVoiceChat()) {
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x417b2601, 0x1a294dad, 11, 0xffffffff,
                                                0xffffffff, first_dialog_button2, GetLocalizedString(0xd9d8184f));
            } else {
                cFEng::Get()->QueueGameMessage(11, GetPackageName(), 0xff);
            }
            break;
        }
        case 3:
            if (gBuddySettings.isRealBuddy) {
                if (BuddyCore::instance()->InVoiceChat() && !cFEng::Get()->FindPackage(g_GameRoomName)) {
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                                   GetLocalizedString(0x713b9c46));
                    break;
                }
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 5, 0xffffffff,
                                                0xffffffff, first_dialog_button2, GetLocalizedString(0xcd787311));
            } else if (gBuddySettings.IsIWannaBeHisBuddy) {
                CancelFriendRequest();
            } else if (gBuddySettings.IsWannaBeMyBuddy) {
                AnswerFriendRequest();
            } else if (!gBuddySettings.isBlocked) {
                InviteBuddy(gBuddySettings.buddyName);
            }
            break;
        case 4:
            bStrCpy(gOtherPlayerName, gBuddySettings.buddyName);
            bShowPersonalStatsForOtherPlayerOverlay = true;
            cFEng::Get()->QueuePackagePush("OL_Rankings_Personal_Overlay.fng", 0, 0, false);
            break;
        case 5: {
            int channel = 0;
            if (VoiceCore::instance().IsInVOIPChat(gBuddySettings.buddyName, &channel) &&
                VoiceCore::instance().IsConnected(channel) && !gBuddySettings.isBlocked && gBuddySettings.canVoiceChat &&
                VoiceCore::instance().IsHeadsetConnected() &&
                (!cFEng::Get()->FindPackage(g_GameRoomName) || !VoiceCore::instance().IsMuted(channel))) {
                if (gBuddySettings.isRealBuddy) {
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                                   0x10199579);
                } else {
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                                   0xba321d6e);
                }
                break;
            }
            if (!gBuddySettings.isBlocked) {
                if (gBuddySettings.IsWannaBeMyBuddy) {
                    cFEng::Get()->QueueGameMessagePkg(10, cFEng::Get()->FindPackage(GetPackageName()));
                } else {
                    char msg[512] = "";
                    FEngSNPrintf(msg, 512, GetLocalizedString(0x8d0a8371), gBuddySettings.buddyName);
                    DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 8, 0xffffffff,
                                                    0xffffffff, first_dialog_button2, msg);
                }
            } else {
                char msg[512] = "";
                FEngSNPrintf(msg, 512, GetLocalizedString(0x121f05d4), gBuddySettings.buddyName);
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 9, 0xffffffff,
                                                0xffffffff, first_dialog_button2, msg);
            }
            break;
        }
        case 6:
            cFEng::Get()->QueuePackagePush("OL_Feedback.fng", 0, 0xff, false);
            m_viewingModalDialog = true;
            break;
    }
}

void UIOnlineEAM_Options::PopulateOptions() {
    int lastSelectedIndex = -1;
    bool addData;
    uint32 optionHash;
    int numOptions;

    if (m_optionScroller.GetSelectedDatum()) {
        lastSelectedIndex = static_cast<OptionScrollerData *>(m_optionScroller.GetSelectedDatum())->m_index;
    }

    char sztemp[64] = "";
    FEngSNPrintf(sztemp, 64, " ");
    m_optionScroller.UnHighlightSelected();
    m_optionScroller.DeleteScrollerData();
    numOptions = 0;

    for (int i = 0; i < 7; i++) {
        optionHash = FEngHashString("OPTION_%d", i + 1);
        addData = false;
        switch (i) {
            case 1:
                addData = EvaluateGameInviteDisplayStatus(sztemp);
                break;
            case 0:
                if (gBuddySettings.isBlocked || !gBuddySettings.isRealBuddy) {
                    FEPrintf(GetPackageName(), optionHash, "-BLOCKED-");
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0x7087ce45));
                    addData = false;
                    break;
                }
                if (gBuddySettings.unreadMsgCount) {
                    FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0x748bc8b9));
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0x748bc8b9));
                } else if (gBuddySettings.isInGroup) {
                    FEPrintf(GetPackageName(), optionHash, "Send Group Message");
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0xb8ed90f3));
                } else {
                    FEPrintf(GetPackageName(), optionHash, "Send Message");
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0x12107ffe));
                }
                addData = true;
                break;
            case 2: {
                RichPresenceMsg rpMsg = BuddyCore::instance()->GetRichPresence(&gBuddySettings);
                if (rpMsg.session[0] || FEDatabase->GetOnlineSettings()->AppearOffline == true || gBuddySettings.isBlocked) {
                    addData = false;
                    break;
                }
                if (!gBuddySettings.isRealBuddy) {
                    addData = false;
                    break;
                }
                if (cFEng::Get()->FindPackage(g_GameRoomName)) {
                    addData = false;
                    break;
                }
                if (VoiceCore::instance().IsInVOIPChat(gBuddySettings.buddyName, nullptr)) {
                    FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0xb9c852e4));
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0xb9c852e4));
                    addData = true;
                    break;
                }
                if (!BuddyCore::instance()->getLastVOIPState()) {
                    addData = false;
                    break;
                }
                if (!gVOIP_InviteState) {
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0x2c20c5de));
                } else {
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0xc2aefa2b));
                }
                if (gBuddySettings.canVoiceChat && !gVOIP_InviteState) {
                    FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0x2c20c5de));
                    addData = true;
                    break;
                }
                if (gVOIP_InviteState == 2 && BuddyCore::instance()->DoIHaveAVOIPInviteFromThisBuddy(gBuddySettings.buddyName)) {
                    FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0xc2aefa2b));
                    addData = true;
                    break;
                }
                addData = false;
                FEngSNPrintf(sztemp, 64, GetLocalizedString(0x6967656c));
                break;
            }
            case 3:
                if (gBuddySettings.isBlocked) {
                    addData = false;
                    break;
                }
                if (gBuddySettings.isRealBuddy) {
                    FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0xab6c5a1e));
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0xab6c5a1e));
                } else if (gBuddySettings.IsIWannaBeHisBuddy) {
                    FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0xe59bfb1e));
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0xe59bfb1e));
                } else if (gBuddySettings.IsWannaBeMyBuddy) {
                    FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0xf8bcee28));
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0xf8bcee28));
                } else {
                    FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0x333f7902));
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0x333f7902));
                }
                addData = true;
                break;
            case 4:
                FEngSNPrintf(sztemp, 64, GetLocalizedString(0x9633e6c2));
                addData = true;
                break;
            case 5:
                if (gBuddySettings.isBlocked) {
                    FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0x452383e7));
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0x452383e7));
                } else {
                    FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0x308db484));
                    FEngSNPrintf(sztemp, 64, GetLocalizedString(0x308db484));
                }
                addData = true;
                break;
            case 6:
                FEPrintf(GetPackageName(), optionHash, GetLocalizedString(0x6353f07e));
                FEngSNPrintf(sztemp, 64, GetLocalizedString(0x6353f07e));
                addData = true;
                break;
        }

        if (addData) {
            OptionScrollerData *thisDatum = new ("OptionScrollerData", 0) OptionScrollerData;
            thisDatum->AddData(sztemp, 0);
            thisDatum->m_index = i;
            m_optionScroller.AddData(thisDatum);
            m_optionScroller.Enable(thisDatum);
            numOptions++;
        }
    }

    for (int i = numOptions; i < 7; i++) {
        optionHash = FEngHashString("OPTION_%d", i + 1);
        FEPrintf(GetPackageName(), optionHash, " ");
    }

    m_optionScroller.Reset(true);

    if (lastSelectedIndex != -1) {
        OptionScrollerData *thisDatum = static_cast<OptionScrollerData *>(m_optionScroller.GetFirstDatum());
        for (uint32 x = 0; x < m_optionScroller.GetNumData(); x++) {
            if (thisDatum->m_index == lastSelectedIndex) {
                m_optionScroller.SetSelectedDatum(thisDatum);
                break;
            }
            thisDatum = static_cast<OptionScrollerData *>(thisDatum->GetNext());
        }
    }

    m_optionScroller.HighlightSelected();
    m_optionScroller.Update(true);
}

void UIOnlineEAM_Options::BlockCB(int op, int status) {
    CloseWaitingDialog(false);

    char message[256] = "";
    if (status == 0) {
        for (int i = LobbyChat::Instance().GetNumReceivedInvites() - 1; i >= 0; i--) {
            Invite *pInvite = LobbyChat::Instance().GetReceivedInvite(i);
            if (!bStrCmp(pInvite->player, gBuddySettings.buddyName)) {
                LobbyChat::Instance().RespondToInvite(pInvite->player, pInvite->gameIdent, LobbyChatN::RESPONSE_DECLINE);
            }
        }
        if (!LobbyChat::Instance().GetNumReceivedInvites()) {
            UpdateStatusIcons(1, false);
        }
        if (BuddyCore::instance()->DoIHaveAVOIPInviteFromThisBuddy(gBuddySettings.buddyName)) {
            gVOIP_InviteState = 0;
            LobbyChalDeclineChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), 0);
            UpdateStatusIcons(2, false);
        }
        HLBMsgListDeleteAll(gBuddySettings.pBuddy, 0);
        FEngSNPrintf(message, 256, GetLocalizedString(0x050185f4), gBuddySettings.buddyName, gBuddySettings.buddyName);
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec, message);
        UpdateStatusIcons(0, false);
        gBuddyListHasChanged = 1;
    } else if (status == -5) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                       GetLocalizedString(0xd7a26411));
    } else {
        DisplayError(status);
    }

    if (cFEng::Get()->IsPackagePushed("OLX_Message.fng")) {
        cFEng::Get()->QueueGameMessagePkg(0xe1fde1d1, cFEng::Get()->FindPackage("OLX_Message.fng"));
    }
}

void UIOnlineEAM_Options::UnBlockCB(int op, int status) {
    CloseWaitingDialog(false);

    char message[128] = "";
    if (status == 0) {
        FEngSNPrintf(message, 128, GetLocalizedString(0xa4b092ee), gBuddySettings.buddyName);
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec, message);
        gBuddyListHasChanged = 1;
        if (VoiceCore::instance().IsMuted(gBuddySettings.buddyName)) {
            VoiceCore::instance().ToggleMuted(gBuddySettings.buddyName);
        }
    } else {
        DisplayError(status);
    }
}

void UIOnlineEAM_Options::InviteBuddy(char *pName) {
    int numBuddys = BuddyCore::instance()->GetNumRealBuddys();

    numBuddys += BuddyCore::instance()->GetNumFriendRequests();
    if (numBuddys >= 100) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec, 0x6aa4c280);
        return;
    }

    char message[64] = "";
    int result = HLBListInviteBuddy(BuddyCore::instance()->getApiRef(), pName,
                                    options_hlb_op_callback<&UIOnlineEAM_Options::InviteBuddyCB>::call, this);
    if (result) {
        DisplayError(result);
        return;
    }
    ShowWaitingForCallbackDialog(0xa948a646, gBuddySettings.buddyName);
}

void UIOnlineEAM_Options::InviteBuddyCB(int op, int status) {
    CloseWaitingDialog(false);

    if (status == 0) {
        FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
        gBuddyListHasChanged = 1;
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                       GetLocalizedString(0x84706d98), gBuddySettings.buddyName);
    } else {
        DisplayError(status);
    }
    HLBListUnFlagTempBuddy(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, HLB_TEMPBUDDYTYPE_MESG);
}

void UIOnlineEAM_Options::AnswerFriendRequest() {
    DialogInterface::ShowThreeButtons(GetPackageName(), "3button", dialog_none, 0x308db484, 0x2c119933, 0x4f876b6f, 10, 3, 2,
                                      0xffffffff, first_dialog_button3, 0xf8bcee28);
}

void UIOnlineEAM_Options::RemoveFriend() {
    int result;

    m_removingFriend = true;
    LobbyChat::Instance().CancelInvite(gBuddySettings.buddyName);
    for (int i = LobbyChat::Instance().GetNumReceivedInvites() - 1; i >= 0; i--) {
        Invite *pInvite = LobbyChat::Instance().GetReceivedInvite(i);
        if (!bStrCmp(pInvite->player, gBuddySettings.buddyName)) {
            LobbyChat::Instance().RespondToInvite(pInvite->player, pInvite->gameIdent, LobbyChatN::RESPONSE_DECLINE);
        }
    }
    if (!LobbyChat::Instance().GetNumReceivedInvites()) {
        UpdateStatusIcons(1, false);
    }
    if (BuddyCore::instance()->DoIHaveAVOIPInviteFromThisBuddy(gBuddySettings.buddyName)) {
        gVOIP_InviteState = 0;
        LobbyChalDeclineChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), 0);
        UpdateStatusIcons(2, false);
    }
    HLBMsgListDeleteAll(gBuddySettings.pBuddy, 0);
    HLBListDeleteTempBuddy(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName);
    if (!BuddyCore::instance()->getBuddyByName(gBuddySettings.buddyName)) {
        m_removingFriend = false;
        gBuddyListHasChanged = 1;
        FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
    } else {
        result = HLBListUnMakeBuddy(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName,
                                    options_hlb_op_callback<&UIOnlineEAM_Options::RemoveFriendCB>::call, this);
        if (result) {
            DisplayError(result);
        } else {
            ShowWaitingForCallbackDialog(0x0b8066fd, gBuddySettings.buddyName);
        }
        gBuddyListHasChanged = 1;
    }
}

void UIOnlineEAM_Options::RemoveFriendCB(int op, int status) {
    CloseWaitingDialog(false);

    if (status) {
        DisplayError(status);
    } else {
        gBuddyListHasChanged = 1;
        cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), FEngFindObject(GetPackageName(), 0xf015108a));
    }
}

void UIOnlineEAM_Options::CancelFriendRequest() {
    int result = HLBListAnswerInvite(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, 2,
                                     options_hlb_op_callback<&UIOnlineEAM_Options::AnswerInviteCB>::call, this);
    if (result) {
        DisplayError(result);
    } else {
        gBuddyListHasChanged = 1;
        ShowWaitingForCallbackDialog(0x4b14af4e, gBuddySettings.buddyName);
    }
}

void UIOnlineEAM_Options::AnswerInviteCB(int op, int status) {
    CloseWaitingDialog(false);

    if (status == 0) {
        FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
    } else {
        DisplayError(status);
    }
    gBuddyListHasChanged = 1;
}

void UIOnlineEAM_Options::AnswerChallenge() {
    BuddySettings buddySettings = EA_Messenger_GetBuddySettings(gBuddySettings.pBuddy);
    Invite *info;

    for (int i = 0; i < 5; i++) {
        info = LobbyChat::Instance().GetReceivedInvite(i);
        if (!info) {
            break;
        }
        if (!bStrCmp(info->player, buddySettings.buddyName)) {
        }
    }

    if (!LobbyChat::Instance().GetReceivedInvite(gBuddySettings.buddyName, gBuddySettings.gameIdent)) {
        RichPresenceMsg rpMsg = BuddyCore::instance()->GetRichPresence(&gBuddySettings);
        if (gBuddySettings.isRealBuddy && rpMsg.session[0] && !rpMsg.isSessionPrivate) {
            FilterGameSessionParamsT filterParams(1, rpMsg.session);
            LobbyGameSessions::Instance().FindSessions(filterParams, nullptr, nullptr);
            gSelfChallengeInfo = new ("gSelfChallengeInfo", 0) ChallengeInfo;
            bStrCpy(gSelfChallengeInfo->hostName, gBuddySettings.buddyName);
            bStrCpy(gSelfChallengeInfo->challengeName, rpMsg.session);
            bStrCpy(gSelfChallengeInfo->password, rpMsg.password);
            gSupressSelfChallengeUIHack = true;
        }
    }

    u32 controlMask = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(0));
    cFEng::Get()->QueuePackagePush("OL_Challenge.fng", 0, controlMask, false);
    gChallengeRecieved = 0;
}

void UIOnlineEAM_Options::EvaluateGameInviteStatus() {
    uint32 result = 0;
    char tempName[16] = "";

    bStrNCpy(tempName, gBuddySettings.buddyName, 16);
    gBuddySettings = EA_Messenger_GetBuddySettingsByName(gBuddySettings.buddyName);
    bStrNCpy(gBuddySettings.buddyName, tempName, 16);

    if (FEngFindScreen(g_GameRoomName)) {
        if (!gBuddySettings.IWannaPlayBuddy && !(gBuddySettings.gameInviteFlags & 0x20000)) {
            if (gBuddySettings.IsSameProduct) {
                if (gBuddySettings.isPassive) {
                    char msg[128] = "";
                    FEngSNPrintf(msg, 128, GetLocalizedString(0xa4ca09c8), gBuddySettings.buddyName);
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec, msg);
                    return;
                }
                int rc = LobbyChat::Instance().SendGameInvite(gBuddySettings.buddyName);
                if (rc == LobbyChatN::IERR_PLAYER_IN_GAME) {
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                                   GetLocalizedString(0xe973ea37), gBuddySettings.buddyName);
                    return;
                } else if (rc == LobbyChatN::IERR_TOO_MANY_INVITES) {
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                                   GetLocalizedString(0x89e3b31f));
                    return;
                } else if (rc == LobbyChatN::IERR_ALREADY_PENDING) {
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                                   GetLocalizedString(0x02ad727f));
                    return;
                }
                FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
                result = 0;
            } else {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                               GetLocalizedString(0x97b25a02));
            }
            gBuddyListHasChanged = 1;
        } else if (gBuddySettings.IsSameProduct) {
            result = 0;
            LobbyChat::Instance().CancelInvite(gBuddySettings.buddyName);
            cFEng::Get()->QueueGameMessage(0xf9e94d58, g_GameRoomName, 0xff);
            gBuddyListHasChanged = 1;
            FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
        } else {
            cFEng::Get()->QueueGameMessage(0xf9e94d58, g_GameRoomName, 0xff);
            result = HLBListAnswerGameInvite(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, 2,
                                             options_hlb_op_callback<&UIOnlineEAM_Options::CancelInviteCB>::call, this);
            if (result == 0) {
                ShowWaitingForCallbackDialog(0xed8e446b, gBuddySettings.buddyName);
                for (int i = 0; i < g_inviteList.numberOfInvitedPlayers; i++) {
                    if (g_inviteList.isXGameInvite[i]) {
                        if (!bStrCmp(g_inviteList.pInvitedPlayerNames[i], gBuddySettings.buddyName)) {
                            g_inviteList.isXGameInvite[i] = false;
                        }
                    }
                }
            }
        }
        if (result) {
            DisplayError(result);
        }
    } else {
        RichPresenceMsg rpMsg = BuddyCore::instance()->GetRichPresence(&gBuddySettings);
        if (gBuddySettings.isRealBuddy && rpMsg.session[0] && !rpMsg.isSessionPrivate) {
            FilterGameSessionParamsT filterParams(1, rpMsg.session);
            LobbyGameSessions::Instance().FindSessions(filterParams, nullptr, nullptr);
            gSelfChallengeInfo = new ("gSelfChallengeInfo", 0) ChallengeInfo;
            bStrCpy(gSelfChallengeInfo->hostName, gBuddySettings.buddyName);
            bStrCpy(gSelfChallengeInfo->challengeName, rpMsg.session);
            bStrCpy(gSelfChallengeInfo->password, rpMsg.password);
            u32 controlMask = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(0));
            cFEng::Get()->QueuePackagePush("OL_Challenge.fng", 0, controlMask, false);
        } else {
            DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0xd831eab3));
        }
    }
}

bool UIOnlineEAM_Options::EvaluateGameInviteDisplayStatus(char *sztemp) {
    bool bAlreadyInRoom;
    bool inGameRoom;

    if (gBuddySettings.isBlocked || !gBuddySettings.isRealBuddy) {
        return false;
    }
    if (gBuddySettings.isInGroup) {
        return false;
    }

    if (FEngFindScreen(g_GameRoomName)) {
        inGameRoom = true;
    } else {
        inGameRoom = false;
    }
    RichPresenceMsg rpMsg = BuddyCore::instance()->GetRichPresence(&gBuddySettings);
    if (rpMsg.session[0]) {
        bAlreadyInRoom = true;
    } else {
        bAlreadyInRoom = false;
    }
    bool bOpenSlots = true;
    if (LobbyGames::Instance().GetMyGame()) {
        LobbyApiPlayT *myGame = LobbyGames::Instance().GetMyGame();
        if (myGame->iCount == myGame->iMaxsize) {
            bOpenSlots = false;
        }
    }

    FEngSetScript(GetPackageName(), 0x4b4e3889, 0x7ab5521a, true);

    if (inGameRoom && !bAlreadyInRoom) {
        if ((gBuddySettings.gameInviteFlags & 0x20000) || gBuddySettings.IWannaPlayBuddy) {
            FEngSetLanguageHash(GetPackageName(), 0x4b4e3889, 0x5632d96d);
            FEngSNPrintf(sztemp, 32, GetLocalizedString(0x5632d96d));
            return true;
        }
        if (!gBuddySettings.IsSameProduct || gBuddySettings.BuddyWantsToPlay || !bOpenSlots) {
            return false;
        }
        FEngSetLanguageHash(GetPackageName(), 0x4b4e3889, 0x3438b351);
        FEngSNPrintf(sztemp, 32, GetLocalizedString(0x3438b351));
        return true;
    }

    if ((gBuddySettings.gameInviteFlags & 0x400000) || gBuddySettings.BuddyWantsToPlay) {
        FEngSetLanguageHash(GetPackageName(), 0x4b4e3889, 0xc48a4103);
        FEngSNPrintf(sztemp, 32, GetLocalizedString(0xc48a4103));
        return true;
    }
    if (gBuddySettings.isRealBuddy && bAlreadyInRoom && !rpMsg.isSessionPrivate && !inGameRoom) {
        FEngSetLanguageHash(GetPackageName(), 0x4b4e3889, 0x23c37b11);
        FEngSNPrintf(sztemp, 32, GetLocalizedString(0x23c37b11));
        return true;
    }
    FEngSNPrintf(sztemp, 32, GetLocalizedString(0x6729a51e));
    return false;
}

void UIOnlineEAM_Options::CancelInviteCB(int op, int status) {
    CloseWaitingDialog(false);

    cFEng::Get()->QueueGameMessage(0xf9e94d58, g_GameRoomName, 0xff);
    m_challengeState = eCHALLENGE_STATE_CAN_SEND;
    gBuddyListHasChanged = 1;
    FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
}
