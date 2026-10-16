#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyAccount.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#include "Speed/Indep/Src/Online/NetworkCore.hpp"
#include "Speed/Indep/Src/Online/VoiceCore.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"

// total size: 0x4C
class uiOnlineVoiceInvite : public MenuScreen {
  public:
    uiOnlineVoiceInvite(ScreenConstructorData *sd);
    ~uiOnlineVoiceInvite() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;
    eMenuSoundTriggers NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) override;
    void Setup();

  private:
    dialog_handle msgHandle;              // offset 0x2C, size 0x4
    bool m_complete;                      // offset 0x30, size 0x4
    bool m_exitToParent;                  // offset 0x34, size 0x4
    VOIP_ChallengeInfo *m_pChallengeInfo; // offset 0x38, size 0x4
    bool m_isVoiceChatInviter;            // offset 0x3C, size 0x4
    bool m_connecting;                    // offset 0x40, size 0x4
    Timer m_connectTimer;                 // offset 0x44, size 0x4
    u32 mLastButtonHash;                  // offset 0x48, size 0x4
};

MenuScreen *CreateOnlineVoiceInvite(ScreenConstructorData *sd) {
    return new ("uiOnlineVoiceInvite", 0) uiOnlineVoiceInvite(sd);
}

static uiOnlineVoiceInvite *gVoiceInviteScreen = nullptr;

void SetupVoiceInvite() {
    gVoiceInviteScreen->Setup();
}

uiOnlineVoiceInvite::uiOnlineVoiceInvite(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , msgHandle(0) //
    , m_complete(false) //
    , m_exitToParent(false) //
    , m_isVoiceChatInviter(false) //
    , m_connecting(false) {
    m_pChallengeInfo = BuddyCore::instance()->GetVOIPChallengeInfo(gBuddySettings.buddyName);
    m_connectTimer.ResetLow();
    LobbyUsers::Instance().GetUserOnlineRecord(gBuddySettings.buddyName, nullptr, this);
    Setup();
    gVoiceInviteScreen = this;
}

uiOnlineVoiceInvite::~uiOnlineVoiceInvite() {}

eMenuSoundTriggers uiOnlineVoiceInvite::NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) {
    if (maybe == UISND_COMMON_LEFT) {
        if (FEngGetCurrentButton(GetPackageName())->NameHash != mLastButtonHash) {
            mLastButtonHash = FEngGetCurrentButton(GetPackageName())->NameHash;
            return UISND_COMMON_LEFT;
        } else {
            mLastButtonHash = FEngGetCurrentButton(GetPackageName())->NameHash;
            return UISND_NONE;
        }
    }
    return maybe;
}

void uiOnlineVoiceInvite::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x519201f5:
            gVOIP_InviteState = 0;
            m_connecting = false;
            if (m_isVoiceChatInviter) {
                cFEng::Get()->QueueGameMessage(0x34dc1bec, GetPackageName(), 0xff);
            } else {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec,
                                               GetLocalizedString(0xc01ebece));
            }
            break;
        case 0x7c1bb77f:
            gVOIP_InviteState = 3;
            DoWithMainMemoryStack((void *)SetupVoiceInvite, 0, 0);
            break;
        case 0x39992a2c:
            if (gVOIP_InviteState == 1) {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec,
                                               GetLocalizedString(0xfe38abbc));
            } else {
                FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
            }
            gVOIP_InviteState = 0;
            break;
        case 0x72170696:
            if (gVOIP_InviteState != 1) {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec,
                                               GetLocalizedString(0xaf3dceee));
            } else {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec,
                                               GetLocalizedString(0x76be33ff));
            }
            gVOIP_InviteState = 0;
            m_connecting = false;
            break;
        case 0xf5717b72:
            DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec,
                                           GetLocalizedString(0x6c02f6e1), gBuddySettings.buddyName);
            break;
        case 0xc98356ba:
            if (m_connecting) {
                if (!m_isVoiceChatInviter) {
                    if ((RealTimer - m_connectTimer).GetSeconds() > 30.0f) {
                        LobbyChalDeclineChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), 0);
                        cFEng::Get()->QueueGameMessage(0x519201f5, GetPackageName(), 0xff);
                        gVOIP_InviteState = 0;
                        m_connecting = false;
                        BuddyCore::instance()->RemoveVOIPChallenge(gBuddySettings.buddyName);
                        ConnectionCore::Instance().LeaveSession();
                    }
                }
                if (VoiceCore::instance().IsConnected(BuddyCore::instance()->GetBuddyChatChannel())) {
                    BuddyCore::instance()->SetVoiceChatState(true);
                    cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), FEngFindObject(GetPackageName(), 0xf015108a));
                    gVOIP_InviteState = 0;
                    m_connecting = false;
                }
            }
            if (m_exitToParent) {
                cFEng::Get()->QueuePackagePop(1);
                gBuddyListHasChanged = true;
                m_exitToParent = false;
                UpdateStatusIcons(2, false);
                FEngSetScript("OL_FriendDialogue.fng", 0x47ff4e7c, 0xde6eff34, true);
            }
            break;
        case 0x35f8620b:
        case 0x5073ef13:
            break;
        case 0xe1fde1d1:
            m_exitToParent = true;
            break;
        case 0x911ab364:
            if (gVOIP_InviteState == 2) {
                HLBListAnswerGameInvite(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, 1, 0, 0);
                LobbyChalDeclineChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), 0);
                cFEng::Get()->QueueGameMessage(0x39992a2c, GetPackageName(), 0xff);
                gVOIP_InviteState = 0;
            } else if (gVOIP_InviteState == 1) {
                HLBListAnswerGameInvite(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, 2, 0, 0);
                LobbyChalCancelChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), 1);
                cFEng::Get()->QueueGameMessage(0x519201f5, GetPackageName(), 0xff);
                gVOIP_InviteState = 0;
            } else {
                FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
            }
            break;
        case 0x0c407210:
            if (obj) {
                switch (obj->NameHash) {
                    case 0xb8a7c6cc:
                        LobbyChalAcceptChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef());
                        cFEng::Get()->QueueGameMessage(0x7c1bb77f, GetPackageName(), 0xff);
                        UpdateStatusIcons(2, false);
                        gVOIP_InviteState = 0;
                        break;
                    case 0xb8a7c6cd:
                        BuddyCore::instance()->RemoveVOIPChallenge(gBuddySettings.buddyName);
                        ConnectionCore::Instance().LeaveSession();
                        LobbyChalDeclineChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), 0);
                        cFEng::Get()->QueueGameMessage(0x39992a2c, GetPackageName(), 0xff);
                        UpdateStatusIcons(2, false);
                        FEngSetScript(GetPackageName(), 0xf015108a, 0x587c018b, true);
                        break;
                    case 0xb8a7c6ce:
                        if (gVOIP_InviteState == 1) {
                            BuddyCore::instance()->RemoveVOIPChallenge(gBuddySettings.buddyName);
                            ConnectionCore::Instance().LeaveSession();
                            LobbyChalCancelChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), 1);
                            cFEng::Get()->QueueGameMessage(0x519201f5, GetPackageName(), 0xff);
                            UpdateStatusIcons(2, false);
                            FEngSetScript(GetPackageName(), 0xf015108a, 0x587c018b, true);
                            break;
                        }

                        if (m_connecting) {
                            ConnectionCore::Instance().LeaveSession();
                            BuddyCore::instance()->SetVoiceChatState(false);
                            if (!m_isVoiceChatInviter) {
                                LobbyChalDeclineChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), 0);
                            }
                            BuddyCore::instance()->RemoveVOIPChallenge(gBuddySettings.buddyName);
                            gVOIP_InviteState = 0;
                            m_connecting = false;
                        }
                        FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
                        m_exitToParent = true;
                        break;
                }
            }
            break;
        case 0x34dc1bec:
            FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
            m_exitToParent = true;
            break;
    }
}

void uiOnlineVoiceInvite::Setup() {
    if (gVOIP_InviteState == 2) {
        FEPrintf(GetPackageName(), 0x63023aae, GetLocalizedString(0x76be33ff), m_pChallengeInfo->challengerName);
        FEngSetScript(GetPackageName(), 0xea19fbdb, 0x1835d907, true);
        FEngSetScript(GetPackageName(), 0x02ddf58c, 0x0016a259, true);
        FEngSetScript(GetPackageName(), 0x949f4946, 0x001744b3, true);
        FEPrintf(GetPackageName(), 0xf9363f30, GetLocalizedString(0x4f876b6f));
        FEPrintf(GetPackageName(), 0xfb8b67d1, GetLocalizedString(0x2c119933));
        FEngSetScript(GetPackageName(), 0xfde09072, 0x0016a259, true);
        FEngSetButtonState(GetPackageName(), 0xb8a7c6ce, false);
        FEngSetCurrentButton(GetPackageName(), 0xb8a7c6cd);
    }

    if (gVOIP_InviteState == 3) {
        if (!m_isVoiceChatInviter) {
            ConnectionCore::Instance().LeaveSession();
        }
        FEPrintf(GetPackageName(), 0x63023aae, GetLocalizedString(0x50cf1107));
        FEngSetScript(GetPackageName(), 0xea19fbdb, 0x1835d907, true);
        FEngSetScript(GetPackageName(), 0x02ddf58c, 0x001744b3, true);
        FEngSetScript(GetPackageName(), 0x949f4946, 0x0016a259, true);
        FEPrintf(GetPackageName(), 0xfde09072, GetLocalizedString(0x1a294dad));
        FEngSetButtonState(GetPackageName(), 0xb8a7c6cc, false);
        FEngSetInvisible(GetPackageName(), 0xf9363f30);
        FEngSetButtonState(GetPackageName(), 0xb8a7c6cd, false);
        FEngSetInvisible(GetPackageName(), 0xfb8b67d1);
        FEngSetCurrentButton(GetPackageName(), 0xb8a7c6ce);

        int slot = BuddyCore::instance()->GetNextAvailableVOIPSlot();
        if (m_isVoiceChatInviter) {
            LobbyApiUserT *buddy = LobbyUsers::Instance().GetUserRecord(gBuddySettings.buddyName);
            LobbyApiUserT *me = LobbyUsers::Instance().GetMyUserRecord();
            ConnectionCore::Instance().HostSession(me->uLocalAddr, 2);
            ConnectionCore::Instance().AddPlayer(*buddy);
        } else {
            ConnApiUserInfoT hostInfo;
            hostInfo.uAddr = TagFieldGetNumber(TagFieldFind(m_pChallengeInfo->hostName, "EXTHOSTNAME"), 0);
            hostInfo.uLocalAddr = TagFieldGetNumber(TagFieldFind(m_pChallengeInfo->hostName, "INTHOSTNAME"), 0);
            bStrCpy(hostInfo.strName, m_pChallengeInfo->challengerName);
            TagFieldGetString(TagFieldFind(m_pChallengeInfo->hostName, "DIRTYADDR"), hostInfo.DirtyAddr.strMachineAddr, 64,
                              "NO DIRTYADDR");
            ConnectionCore::Instance().JoinSession(hostInfo, hostInfo.uAddr, 2);
        }

        BuddyCore::instance()->SetMyChatChannel(0);
        BuddyCore::instance()->GetBuddyChatChannel(slot);
        m_connecting = true;
        m_connectTimer = RealTimer;
    }

    if (gVOIP_InviteState == 1) {
        char msg[192] = "";
        TagFieldSetString(msg, 192, "MYMSG", "I wanna talk to you!!!");
        LobbyApiUserT *me = LobbyUsers::Instance().GetMyUserRecord();
        TagFieldSetNumber(msg, 192, "INTHOSTNAME", me->uLocalAddr);
        TagFieldSetNumber(msg, 192, "EXTHOSTNAME", me->addr);
        TagFieldSetString(msg, 192, "DIRTYADDR", me->MachineAddr.strMachineAddr);
        LobbyChalInitiateChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), gBuddySettings.buddyName, 'voip', msg, 0);

        FEPrintf(GetPackageName(), 0x63023aae, GetLocalizedString(0x695d0495), gBuddySettings.buddyName);
        FEngSetScript(GetPackageName(), 0xea19fbdb, 0x1835d907, true);
        FEngSetScript(GetPackageName(), 0x02ddf58c, 0x0016a259, true);
        FEPrintf(GetPackageName(), 0xfde09072, GetLocalizedString(0x1a294dad));
        FEngSetButtonState(GetPackageName(), 0xb8a7c6cc, false);
        FEngSetInvisible(GetPackageName(), 0xf9363f30);
        FEngSetButtonState(GetPackageName(), 0xb8a7c6cd, false);
        FEngSetInvisible(GetPackageName(), 0xfb8b67d1);
        FEngSetCurrentButton(GetPackageName(), 0xb8a7c6ce);
        m_isVoiceChatInviter = true;
    }

    mLastButtonHash = FEngGetCurrentButton(GetPackageName())->NameHash;
}
