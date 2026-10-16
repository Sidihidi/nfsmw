#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyChat.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"

// total size: 0x154
class uiOnlineChallengeInvite : public MenuScreen {
  public:
    uiOnlineChallengeInvite(ScreenConstructorData *sd);
    ~uiOnlineChallengeInvite() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup();
    void ShowWaitingForCallbackDialog(uint32 StringHash, char *pString);
    void CloseWaitingDialog(bool bCancelBuddyOp);
    void BlockPlayerCB(int, int);
    void AnswerGameInviteCB(int op, int status);
    void AnswerGameInviteAcceptCB(int op, int status);
    void SwitchToOnlineCareerCar();
    void BuildSelfInvite();
    void PopulateSettings();

    dialog_handle msgHandle;               // offset 0x2C, size 0x4
    bool m_exitToParent;                   // offset 0x30, size 0x4
    ChallengeInfo m_challengeInfo;         // offset 0x34, size 0x110
    Invite *m_pInvite;                     // offset 0x144, size 0x4
    bool m_enteringChallenge;              // offset 0x148, size 0x4
    bool m_bDontDisplayCancelledCallbacks; // offset 0x14C, size 0x4
    bool m_waitingForChallInfo;            // offset 0x150, size 0x4
};

MenuScreen *CreateuiOnlineChallengeInvite(ScreenConstructorData *sd) {
    return new ("uiOnlineChallengeInvite", 0) uiOnlineChallengeInvite(sd);
}

ChallengeInfo *gSelfChallengeInfo = nullptr;
bool gSupressSelfChallengeUIHack = false;

uiOnlineChallengeInvite::uiOnlineChallengeInvite(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , msgHandle(0) //
    , m_exitToParent(false) //
    , m_enteringChallenge(false) //
    , m_bDontDisplayCancelledCallbacks(false) //
    , m_waitingForChallInfo(false) {
    if (gSelfChallengeInfo) {
        m_waitingForChallInfo = true;

        if (gSupressSelfChallengeUIHack == true) {
            FEngSetLanguageHash(GetPackageName(), 0xfb8b67d1, 0x2c119933);
        } else {
            FEngSetLanguageHash(GetPackageName(), 0xfb8b67d1, 0x1a294dad);
        }
    } else {
        m_pInvite = LobbyChat::Instance().GetReceivedInvite(gBuddySettings.buddyName, gBuddySettings.gameIdent);
        FEngSetLanguageHash(GetPackageName(), 0xfb8b67d1, 0x2c119933);
    }

    FEngSetLanguageHash(GetPackageName(), 0xf9363f30, 0x4f876b6f);
    FEngSetLanguageHash(GetPackageName(), 0xfde09072, 0x308db484);
    Setup();
}

uiOnlineChallengeInvite::~uiOnlineChallengeInvite() {
    m_bDontDisplayCancelledCallbacks = true;
    CloseWaitingDialog(true);
}

void uiOnlineChallengeInvite::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    char sztemp[128];

    switch (msg) {
        case 0x427304a6:
            if (!gSelfChallengeInfo) {
                LobbyChat::Instance().RespondToInvite(m_pInvite->player, m_pInvite->gameIdent, LobbyChatN::RESPONSE_DECLINE_BLOCK);
            } else {
                delete gSelfChallengeInfo;
                gSelfChallengeInfo = nullptr;
            }

            HLBListBlockBuddy(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, 0, 0);

            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            gChallengeRecieved = 0;
            m_exitToParent = true;
            m_enteringChallenge = false;
            EnteringChallenge = false;

            if (!LobbyChat::Instance().GetNumReceivedInvites()) {
                UpdateStatusIcons(1, false);
            }
            break;
        case 0xf9e94d58:
            FEngSNPrintf(sztemp, 128, GetLocalizedString(0xc9e362a2), gBuddySettings.buddyName);
            DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xe1fde1d1, 0xe1fde1d1, sztemp);
            break;
        case 0x4fbee42a:
            SwitchToOnlineCareerCar();
            m_exitToParent = true;
            m_enteringChallenge = true;
            EnteringChallenge = true;
            break;
        case 0x34dc1bec:
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            break;
        case 0x97c1b72f:
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            break;
        case 0xc98356ba:
            if (m_waitingForChallInfo) {
                int numSessions = LobbyGameSessions::Instance().GetNumSessions();

                for (int i = 0; i < numSessions; i++) {
                    GameSession *session = LobbyGameSessions::Instance().GetSessionByIndex(i);
                    if (!bStrCmp(session->strName, gSelfChallengeInfo->challengeName)) {
                        BuildSelfInvite();
                        m_waitingForChallInfo = false;
                    }
                }
            }

            BuddyCore::instance()->doprocessing();
            if (m_exitToParent) {
                cFEng::Get()->QueuePackagePop(1);
                gBuddyListHasChanged = true;
                m_exitToParent = false;
                if (m_enteringChallenge) {
                    cFEng::Get()->QueueGameMessage(0x4fbee42a, "OL_FriendDialogue.fng", 0xff);
                } else {
                    cFEng::Get()->QueueGameMessage(0x34dc1bec, "OL_FriendDialogue.fng", 0xff);
                }
            }
            break;
        case 0xe1fde1d1:
            if (gBuddySettings.BuddyWantsToPlay || gSelfChallengeInfo) {
                FEngSetScript("OL_FriendDialogue.fng", 0x47ff4e7c, 0x6ebbfb68, true);
                m_exitToParent = true;
            }
            break;
        case 0x35f8620b:
            FEngSetScript("OL_EAMessenger.fng", 0x47ff4e7c, 0x9e99, true);
            FEngSetScript("OL_FriendDialogue.fng", 0x47ff4e7c, 0x9e99, true);
            FEngSetCurrentButton(GetPackageName(), 0xb8a7c6cd);
            FEngSetScript(GetPackageName(), 0xb8a7c6cd, 0x249db7b7, true);
            break;
        case 0x911ab364:
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            m_exitToParent = true;
            break;
        case 0x0c407210:
            if (obj) {
                switch (obj->NameHash) {
                    case 0xb8a7c6cc:
                        gChallengeRecieved = 0;
                        UpdateStatusIcons(1, false);

                        if (gBuddySettings.BuddyWantsToPlay || gSelfChallengeInfo) {
                            cFEng::Get()->QueueGameMessage(0x4fbee42a, GetPackageName(), 0xff);
                        } else {
                            DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364,
                                                           0x911ab364, 0x88a7cf05);
                        }
                        break;
                    case 0xb8a7c6cd:
                        if (!gSelfChallengeInfo) {
                            LobbyChat::Instance().RespondToInvite(m_pInvite->player, m_pInvite->gameIdent,
                                                                  LobbyChatN::RESPONSE_DECLINE);
                        } else {
                            delete gSelfChallengeInfo;
                            gSelfChallengeInfo = nullptr;
                        }

                        cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
                        gChallengeRecieved = 0;

                        if (!LobbyChat::Instance().GetNumReceivedInvites()) {
                            UpdateStatusIcons(1, false);
                        }
                        break;
                    case 0xb8a7c6ce: {
                        char msg[512] = "";
                        FEngSNPrintf(msg, 512, GetLocalizedString(0x8d0a8371), gBuddySettings.buddyName);
                        DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 0x427304a6,
                                                        0xffffffff, 0xffffffff, first_dialog_button2, msg);
                        break;
                    }
                }
            }
            break;
    }
}

void uiOnlineChallengeInvite::Setup() {
    if (gSelfChallengeInfo) {
        if (gSupressSelfChallengeUIHack == true) {
            FEPrintf(GetPackageName(), 0x42adb44c, GetLocalizedString(0xd4abf03d));
            gSupressSelfChallengeUIHack = false;
        } else {
            FEPrintf(GetPackageName(), 0x42adb44c, GetLocalizedString(0x79dfa0b9));
        }
        FEPrintf(GetPackageName(), 0xcc8d0942, gSelfChallengeInfo->hostName);
    } else {
        FEPrintf(GetPackageName(), 0x42adb44c, GetLocalizedString(0xd4abf03d));

        m_challengeInfo.gameMode = TagFieldGetNumber(TagFieldFind(m_pInvite->gameSettings, "GAMEMODE"), 0);
        m_challengeInfo.ranked = TagFieldGetNumber(TagFieldFind(m_pInvite->gameSettings, "RANKED"), 0);
        m_challengeInfo.numPlayers = TagFieldGetNumber(TagFieldFind(m_pInvite->gameSettings, "NUMPLYS"), 0);
        m_challengeInfo.collisionDetection = TagFieldGetNumber(TagFieldFind(m_pInvite->gameSettings, "COLLON"), 0);
        m_challengeInfo.performanceMatchingLevel = TagFieldGetNumber(TagFieldFind(m_pInvite->gameSettings, "CCLASS"), 1);
        m_challengeInfo.gameBreaker = TagFieldGetNumber(TagFieldFind(m_pInvite->gameSettings, "NOS"), 0);
        m_challengeInfo.eventHash = TagFieldGetNumber(TagFieldFind(m_pInvite->gameSettings, "EVHASH"), 0);
        m_challengeInfo.trackDirection = TagFieldGetNumber(TagFieldFind(m_pInvite->gameSettings, "TRKDIR"), 0);
        m_challengeInfo.numLaps = TagFieldGetNumber(TagFieldFind(m_pInvite->gameSettings, "NUMLAPS"), 0);
        TagFieldGetString(TagFieldFind(m_pInvite->gameSettings, "PASSWORD"), m_challengeInfo.password, 32, "crusoe");
        m_challengeInfo.privateSession = TagFieldGetNumber(TagFieldFind(m_pInvite->gameSettings, "PRIV"), 0);

        FEPrintf(GetPackageName(), 0xcc8d0942, m_pInvite->player);
    }

    FEngSetLanguageHash(GetPackageName(), 0x7248670a, 0xfd143578);
    FEngSetLanguageHash(GetPackageName(), 0x7248670b, 0x145890ee);
    FEngSetLanguageHash(GetPackageName(), 0x7248670c, 0xc419592e);
    FEngSetLanguageHash(GetPackageName(), 0x7248670d, 0x9c302c85);
    FEngSetLanguageHash(GetPackageName(), 0x7248670e, 0xcfb4caa6);
    FEngSetLanguageHash(GetPackageName(), 0x7248670f, 0x932538c3);
    FEngSetLanguageHash(GetPackageName(), 0x72486710, 0x911d6309);
    FEngSetLanguageHash(GetPackageName(), 0x72486711, 0x12f6a0b7);
    FEngSetLanguageHash(GetPackageName(), 0x72486712, 0x883d13ca);

    if (!gSelfChallengeInfo) {
        PopulateSettings();
    }
}

void uiOnlineChallengeInvite::PopulateSettings() {
    uint32 hash = 0;

    switch (m_challengeInfo.gameMode) {
        case GRace::kRaceType_Circuit:
            hash = 0x034fa2c1;
            break;
        case GRace::kRaceType_P2P:
            hash = 0xb94fd70e;
            break;
        case GRace::kRaceType_Drag:
            hash = 0x6f547e4c;
            break;
    }

    FEngSetLanguageHash(GetPackageName(), 0x0ca49d8b, hash);
    FEngSetLanguageHash(GetPackageName(), 0x0ca49d8c, m_challengeInfo.ranked ? 0x66cf56ae : 0x0e1f1271);

    GRaceParameters *raceParams = GRaceDatabase::Get().GetRaceFromHash(m_challengeInfo.eventHash);
    if (raceParams) {
        FEPrintf(GetPackageName(), 0x0ca49d8d, raceParams->GetEventID());
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x0ca49d8d, 0x7818f85e);
    }

    if (m_challengeInfo.gameMode == GRace::kRaceType_Circuit) {
        FEPrintf(GetPackageName(), 0x0ca49d8e, "%d", m_challengeInfo.numLaps);
    } else {
        FEPrintf(GetPackageName(), 0x0ca49d8e, GetLocalizedString(0x7f2f7ad6));
    }

    FEngSetLanguageHash(GetPackageName(), 0x0ca49d8f, m_challengeInfo.gameBreaker ? 0x417b2604 : 0x70dfe5c2);
    FEngSetLanguageHash(GetPackageName(), 0x0ca49d90, m_challengeInfo.collisionDetection ? 0x417b2604 : 0x70dfe5c2);
    FEngSetLanguageHash(GetPackageName(), 0x0ca49d91, m_challengeInfo.performanceMatchingLevel ? 0x417b2604 : 0x70dfe5c2);
    FEPrintf(GetPackageName(), 0x0ca49d92, "%d / 4", m_challengeInfo.numPlayers);
    FEngSetLanguageHash(GetPackageName(), 0x0ca49d93, m_challengeInfo.privateSession ? 0x70e01038 : 0x417b25e4);

    FEngSetLanguageHash(GetPackageName(), 0x57ad6797, 0x5ca73434);
}

void uiOnlineChallengeInvite::CloseWaitingDialog(bool bCancelBuddyOp) {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }

    if (bCancelBuddyOp) {
        if (BuddyCore::instance()->GetConnectedState() == connected && BuddyCore::instance()->getApiRef()) {
            HLBApiCancelOp(BuddyCore::instance()->getApiRef());
        }
    }
}

void uiOnlineChallengeInvite::SwitchToOnlineCareerCar() {}

void uiOnlineChallengeInvite::BuildSelfInvite() {
    GameSession *thisSession;
    uint32 eventHash;

    thisSession = LobbyGameSessions::Instance().GetSessionByName(gSelfChallengeInfo->challengeName);
    eventHash = TagFieldGetNumber(TagFieldFind(thisSession->strParams, "E"), 0);

    GRaceParameters *raceParams = GRaceDatabase::Get().GetRaceFromHash(eventHash);

    m_challengeInfo.eventHash = eventHash;
    m_challengeInfo.gameMode = raceParams->GetRaceType();
    m_challengeInfo.collisionDetection = thisSession->uCustFlags & 0x400;
    m_challengeInfo.gameBreaker = thisSession->uCustFlags & 0x800;
    m_challengeInfo.ranked = thisSession->uCustFlags & 0x200;
    m_challengeInfo.trackDirection = (thisSession->uCustFlags & 0x1000) != 0;
    m_challengeInfo.numLaps = TagFieldGetNumber(TagFieldFind(thisSession->strParams, "L"), 0);
    m_challengeInfo.numPlayers = thisSession->iCount;
    m_challengeInfo.privateSession = thisSession->uCustFlags & 0x2;
    m_challengeInfo.performanceMatchingLevel = thisSession->uCustFlags & 0x2000;

    PopulateSettings();
}
