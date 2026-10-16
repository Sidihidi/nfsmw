#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineMain.hpp"

#include "Speed/Indep/Src/Frontend/Careers/UnlockSystem.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FECarViewer.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_MU_Keyboard.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/GameRoomSubMenus.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOLLobbyRoom.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineDisconnect.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyChat.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"
#include "Speed/Indep/Src/Online/LobbyGames.hpp"
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#include "Speed/Indep/Src/Online/VoiceCore.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"

const char *g_GameRoomName = "OL_GameRoom.fng";
const char *g_LOBBY_ROOM_SCREEN = "OL_LobbyRoom.fng";
const char *GAME_ROOM_INFO_GROUP = "Group(72)";

struct SDisqRatingHighlights {
    int MaxDisqualifications;
    const char *pHighlightName;
    const char *pUnHighlightName;
};

static SDisqRatingHighlights gsDisqRatings[5] = {
    {25, "Highlight", "UnHighlight"},       {75, "D1_Highlight", "D1_UnHighlight"},  {150, "D2_Highlight", "D2_UnHighlight"},
    {300, "D3_Highlight", "D3_UnHighlight"}, {0, "D4_Highlight", "D4_UnHighlight"},
};

static SDisqRatingHighlights *gGetDisqRatingHighlightsFor(int NumDisqs) {
    SDisqRatingHighlights *pRet = nullptr;

    for (int i = 0; i < 4 && !pRet; i++) {
        if (NumDisqs <= gsDisqRatings[i].MaxDisqualifications) {
            pRet = &gsDisqRatings[i];
        }
    }

    if (!pRet) {
        pRet = &gsDisqRatings[4];
    }
    return pRet;
}

struct InvitationList {
    int numberOfInvitedPlayers;
    char *pInvitedPlayerNames[4];
    bool isXGameInvite[4];
    bool isInRoom[4];
};

InvitationList g_inviteList;

MenuScreen *CreateUIOLGameRoomScreen(ScreenConstructorData *sd) {
    return new ("UIOLGameRoom", 0) UIOLGameRoom(sd);
}

template <void (UIOLGameRoom::*Callback)(LobbyApiMsgT *)> struct LobbyCmd_callback {
    static void call(LobbyApiMsgT *msg, void *context) {
        (static_cast<UIOLGameRoom *>(context)->*Callback)(msg);
    }
};

const int UIOLGameRoom::countdownSeconds = 10;
static UIOLGameRoom *g_pGameRoom = nullptr;
static bool gNeedToReCreateGame = false;
PlayerScrollerDatum *UIOLGameRoom::mSelectedPlayerDatum = nullptr;

UIOLGameRoom::UIOLGameRoom(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , playerScroller(GetPackageName(), "scroll_area", "scrollbar", true, true, false, true) //
    , mChat(GetPackageName()) //
    , msgHandle(0) //
    , m_viewingModalDisplay(false) //
    , m_setDimState(false) //
    , m_startGameTimer(RealTimer.GetSeconds()) //
    , m_bTestStartGameTimeOut(false) //
    , m_sendPrivateMsg(false) //
    , m_startReadySlot(nullptr) //
    , m_isPrivateRoom(false) //
    , m_bWaitingToViewCar(false) //
    , m_slotIndex(0) //
    , m_bShowingHostModal(false) //
    , m_bShowingClientModal(false) //
    , m_bSetSelectionToUser(false) //
    , m_bLocallyReady(true) //
    , m_bReadyButtonPressed(false) //
    , doUpdateSession(false) //
    , doDismissDialog(false) //
    , doSwitchToLobby(false) //
    , m_bChangingSettings(false) //
    , m_hostStartUpdateTimer(RealTimer.GetSeconds()) //
    , m_isRaceStarted(false) {
    bMemSet(&delayedDialogDetails, 0, sizeof(delayedDialogDetails));

    if (TheOnlineManager.GetState() != OLS_LOBBY_IN_LOBBY) {
        if (LobbyCore::Instance().IsSuspended()) {
            LobbyCore::Instance().Resume(true);
        }
        TheOnlineManager.StartLobby();
    }

    m_isPrivateRoom = FEDatabase->GetOnlineSettings()->IsPrivateRoom;

    for (int i = 0; i < 4; i++) {
        g_inviteList.pInvitedPlayerNames[i] = new ("invited player names", 0) char[32];
        bMemSet(g_inviteList.pInvitedPlayerNames[i], 0, 32);
        g_inviteList.isXGameInvite[i] = false;
        g_inviteList.isInRoom[i] = false;
    }

    g_inviteList.isInRoom[0] = true;
    g_inviteList.numberOfInvitedPlayers = 1;
    bStrCpy(g_inviteList.pInvitedPlayerNames[0], FEDatabase->GetOnlineSettings()->GetLobbyPersona());

    m_carHandle = FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->GetSelectedCar(0);
    m_rideInfo = CarViewer::GetRideInfo(eCARVIEWER_PLAYER1_CAR);

    the_type_area = FEngFindObject(GetPackageName(), 0x759898cb);

    Setup();

    char sztemp[32];
    for (int i = 0; i < 4; i++) {
        FEngSNPrintf(sztemp, 32, "STATUS_%d", i + 1);
        FEngSetScript(GetPackageName(), FEHashUpper(sztemp), FEHASH_HIDE, true);
    }

    InitializePlayerList();

    LobbyGameSessions &lgs = LobbyGameSessions::Instance();
    lgs.SetSessionUpdateCB(SessionUpdateCB, this);
    lgs.FinishSessionChanges();

    cOnlineSettings *onlineSettings = FEDatabase->GetOnlineSettings();
    if (onlineSettings->GameDetails == eHOSTING) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;

        if (!lgs.GetMySession()) {
            if (lgs.CreateSession(FEDatabase->GetOnlineSettings()->GetLobbyPersona(), CreateSessionCB, this) > 0) {
                onlineSettings->SetState(cOnlineSettings::OLS_LOBBY_REQUEST_PENDING);
                msgHandle = DialogInterface::ShowMessage(GetPackageName(), "", dialog_alert, GetLocalizedString(0xcc6c959f));
            } else {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x87f32b2a, 0x87f32b2a,
                                               GetLocalizedString(0xab13cae4));
            }
        } else {
            lgs.CreateGameInSession();
            lgs.SettingsHaveChanged();
        }
    } else {
        FEngSetInvisible(GetPackageName(), 0x69593a1d);
        DisplayGameDetails(this);
    }

    LobbyChat::Instance().SetChatCallback(&mChat, ReceiveChatCB);
    FEngPrintf(GetPackageName(), 0x4162454c, "");
    Redraw(false);

    if (m_isPrivateRoom) {
        gPrivateRoomFlag = 1;
    } else {
        gPrivateRoomFlag = 0;
    }

    for (int i = LobbyChat::Instance().GetNumReceivedInvites() - 1; i >= 0; i--) {
        Invite *pInvite = LobbyChat::Instance().GetReceivedInvite(i);
        LobbyChat::Instance().RespondToInvite(pInvite->player, pInvite->gameIdent, LobbyChatN::RESPONSE_DECLINE);
    }

    if (gVOIP_InviteState == 2) {
        LobbyChalDeclineChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), 0);
        UpdateStatusIcons(2, false);
        gVOIP_InviteState = 0;
    }
    UpdateStatusIcons(1, false);

    g_pGameRoom = this;
    CUIOnlineDisconnect::mIsHostInGameDisconnect = false;
    VerifyMyCar();
}

void UIOLGameRoom::VerifyMyCar() {
    int l_validCarHandle = -1;
    int l_currentCarHandle = FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->GetSelectedCar(0);
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);

    for (int i = 0; i < 200; i++) {
        FECarRecord *fe_car = stable->GetCarByIndex(i);

        if (fe_car->IsValid() && fe_car->MatchesFilter(0xf0007)) {
            if (UnlockSystem::IsCarUnlocked(static_cast<eUnlockFilters>(UNLOCK_QUICK_RACE | UNLOCK_CAREER_MODE | UNLOCK_ONLINE),
                                            fe_car->Handle, 0)) {
                if (l_validCarHandle == 0xffffffff) {
                    l_validCarHandle = fe_car->Handle;
                }
                if (fe_car->Handle == l_currentCarHandle) {
                    return;
                }
            }
        }
    }

    FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->SetSelectedCar(l_validCarHandle, 0);
    RideInfo ride;
    stable->BuildRideForPlayer(l_validCarHandle, 0, &ride);
    CarViewer::SetRideInfo(&ride, SET_RIDE_INFO_REASON_LOAD_CAR, eCARVIEWER_PLAYER1_CAR);
}

UIOLGameRoom::~UIOLGameRoom() {
    for (int i = 0; i < 4; i++) {
        delete g_inviteList.pInvitedPlayerNames[i];
    }

    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }

    if (FEDatabase->GetOnlineSettings()->GetState() >= cOnlineSettings::OLS_LOBBY_IDLE) {
        LobbyChat::Instance().SetChatCallback(nullptr, nullptr);
        LobbyGameSessions::Instance().SetSessionUpdateCB(nullptr, nullptr);
        LobbyCore::Instance().AbortAllCommands();
    }

    gPrivateRoomFlag = 0;
    FEDatabase->GetOnlineSettings()->QuickJoinRace = false;
    g_pGameRoom = nullptr;
    FEManager::Get()->AllowControllerError(false);
}

void UIOLGameRoom::MaybeCreateGameInSession() {
    LobbyApiPlayT *myGame = LobbyGames::Instance().GetMyGame();
    LobbyGameSessions &lgs = LobbyGameSessions::Instance();
    GameSession *mySession = lgs.GetMySession();

    if (gNeedToReCreateGame && mySession && (!myGame || myGame->iIdent < 0) && FEDatabase->GetOnlineSettings()->GameDetails == eHOSTING) {
        bool gameAlreadyExists = false;

        for (int i = 0; i < mySession->iCount; i++) {
            GameSessionMember *thisPlayer = lgs.GetMemberByIndex(i);
            if (thisPlayer && thisPlayer->iGame > 0) {
                gameAlreadyExists = true;
            }
        }

        if (!gameAlreadyExists) {
            gNeedToReCreateGame = false;
            lgs.CreateGameInSession();
            lgs.SettingsHaveChanged();
        }
    }
}

void UIOLGameRoom::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    if (msg == 0x87f32b2a) {
        FEngKillAllChildren(cFEng::Get()->FindPackage(GetPackageName()));
        if (m_isPrivateRoom && !FEDatabase->IsLANMode()) {
            HLBListCancelGameInvite(BuddyCore::instance()->getApiRef(), 0, 0);
            LobbyChalCancelChallenge(BuddyCore::instance()->GetLobbyChallengeApiRef(), 1);
        }
        LeaveSession();
        return;
    }

    if (m_viewingModalDisplay) {
        switch (msg) {
            case 0x5d1a570f:
            case 0x8d770e25:
                m_viewingModalDisplay = false;
            default:
                return;
            case 0xc98356ba:
                if (m_setDimState) {
                    Redraw(true);
                    m_setDimState = false;
                }
                break;
            case 0x97c1b72f:
            case 0x427304a6:
            case 0x39480621:
            case 0x67bfa959:
            case 0xd9656bec:
            case 0xc9d30688:
            case 0xcac72e3d:
            case 0xf9e94d58:
            case 0xda5b8712:
            case 0xfa94652c:
                if (!LobbyGames::Instance().GetMyGame()) {
                    cFEng::Get()->QueueGameMessage(0x87f32b2a, GetPackageName(), 0xff);
                } else {
                    PopulatePlayerList();
                }
                break;
            case 0x37c873af:
                break;
        }
    }

    switch (msg) {
        case 0xeced0b24:
            DisplayGameDetails(this);
            break;
        case 0x35f8620b:
            if (LobbyGameSessions::Instance().GetMySession()) {
                DisplayGameDetails(this);
            }
            UpdateEAMIcons();
            break;
        case 0x0c407210:
        case 0x406415e3:
            if (bStrCmp(static_cast<PlayerScrollerDatum *>(playerScroller.GetSelectedDatum())->name, GetLocalizedString(0x0f636a00))) {
                if (bStrCmp(static_cast<PlayerScrollerDatum *>(playerScroller.GetSelectedDatum())->name,
                            FEDatabase->GetOnlineSettings()->GetLobbyPersona())) {
                    mSelectedPlayerDatum = static_cast<PlayerScrollerDatum *>(playerScroller.GetSelectedDatum());
                    cFEng::Get()->QueuePackagePush("OL_GameRoom_Player_Details.fng", 0, 0, false);
                    if (FEDatabase->GetOnlineSettings()->GameDetails == eHOSTING) {
                        m_bShowingHostModal = true;
                    } else {
                        m_bShowingClientModal = true;
                    }
                    FEngSetScript(GetPackageName(), 0xfc8c5406, FEHASH_HIDE, true);
                    FEngSetScript(GetPackageName(), 0xcd3f1d2f, FEHASH_HIDE, true);
                    FEngSetScript(GetPackageName(), 0x5d974277, FEHASH_HIDE, true);
                } else {
                    cFEng::Get()->QueuePackagePush("OL_GameRoom_Car_Select.fng", 0, 0, false);
                    m_bShowingHostModal = true;
                    FEngSetScript(GetPackageName(), 0xfc8c5406, FEHASH_HIDE, true);
                    FEngSetScript(GetPackageName(), 0xcd3f1d2f, FEHASH_HIDE, true);
                    FEngSetScript(GetPackageName(), 0x5d974277, FEHASH_HIDE, true);
                }
            }
            break;
        case 0x911ab364:
            m_viewingModalDisplay = true;
            if (FEDatabase->GetOnlineSettings()->GameDetails == eHOSTING) {
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, LANGUAGE_COMMON_CANCEL, 0x87f32b2a,
                                                0x8d770e25, 0x8d770e25, first_dialog_button1, 0xfe84c4d1);
            } else {
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, LANGUAGE_COMMON_CANCEL, 0x87f32b2a,
                                                0x8d770e25, 0x8d770e25, first_dialog_button1, 0xbcf16979);
            }
            break;
        case 0xc519bfc4:
            if ((RealTimer - KBCreationTimer).GetSeconds() < g_KBDelaySeconds) {
                break;
            }
            m_setDimState = true;
            m_viewingModalDisplay = true;
            LobbyGameSessions::Instance().StartSessionChanges();
            FEngBeginTextInput(0x6208c3f7, 0, "", GetLocalizedString(0x53f9fd5c), 115);
            break;
        case 0xc519bfc3:
            if (FEDatabase->GetOnlineSettings()->GameDetails == eHOSTING) {
                cFEng::Get()->QueuePackagePush("OL_GameRoom_Edit_Race.fng", 0, 0, false);
                m_bShowingHostModal = true;
                FEngSetScript(GetPackageName(), 0xfc8c5406, FEHASH_HIDE, true);
                FEngSetScript(GetPackageName(), 0xcd3f1d2f, FEHASH_HIDE, true);
                FEngSetScript(GetPackageName(), 0x5d974277, FEHASH_HIDE, true);
            }
            break;
        case 0xc519bfc0:
            if (bEnableEAMessenger) {
                m_setDimState = true;
                m_viewingModalDisplay = true;
            }
            break;
        case 0x72619778:
            ScrollPlayerList(-1);
            m_slotIndex = playerScroller.GetSelectedNodeIndex();
            break;
        case 0x911c0a4b:
            ScrollPlayerList(1);
            m_slotIndex = playerScroller.GetSelectedNodeIndex();
            break;
        case 0xd9feec59:
            mChat.ScrollChatBodyDown();
            break;
        case 0x5073ef13:
            mChat.ScrollChatBodyUp();
            break;
        case 0xc98356ba:
            MaybeCreateGameInSession();
            MaybeDismissDialog();
            MaybeShowDelayedDialog();
            MaybeSwitchToLobby();
            MaybeUpdateSession();
            UpdateInfoBox();
            CheckDisplayOfHostStartCountdown();

            if (m_bWaitingToViewCar) {
                if (playerScroller.GetSelectedDatum()) {
                    if (static_cast<PlayerScrollerDatum *>(playerScroller.GetSelectedDatum())->m_bValidData) {
                        ViewPlayersCar(static_cast<PlayerScrollerDatum *>(playerScroller.GetSelectedDatum()));
                        if (msgHandle) {
                            DialogInterface::DismissDialog(msgHandle);
                        }
                    }
                } else {
                    m_bWaitingToViewCar = false;
                    if (msgHandle) {
                        DialogInterface::DismissDialog(msgHandle);
                    }
                }
            }

            CheckForBuddyOnline();
            ShowVOIPStatus();
            LobbyUsers::Instance().UpdateCarName();

            if (FEDatabase->GetOnlineSettings()->GameDetails == eHOSTING) {
                LobbyGameSessions::Instance().SetSessionLatency(0);
            } else if (!ConnectionCore::Instance().GetNumPlayers() || !ConnectionCore::Instance().GetPlayer(0) ||
                       ConnectionCore::Instance().GetPlayer(0)->GameInfo.eStatus != CONNAPI_STATUS_ACTV) {
                LobbyGameSessions::Instance().SetSessionLatency(-1);
            } else {
                LobbyGameSessions::Instance().SetSessionLatency(
                    NetGameLinkStatus(ConnectionCore::Instance().GetPlayer(0)->pGameLinkRef)->late);
            }

            if (LobbyGameSessions::Instance().GetMySession() && LobbyGameSessions::Instance().GetMySession()->iIdent > 0) {
                char raceinfo[256] = "";
                int i = 0;

                for (; i < LobbyGameSessions::Instance().GetMySession()->iCount; i++) {
                    if (!LobbyGameSessions::Instance().GetMemberByIndex(i)) {
                        TurnOffPingCircles(i + 1);
                    } else {
                        int pingInMsec = LobbyGameSessions::Instance().GetMemberLatency(i);
                        if (pingInMsec < 0) {
                            SetConnectionQualityUnknown(i + 1);
                        } else if (pingInMsec < 51) {
                            SetConnectionQualityGood(i + 1);
                        } else if (pingInMsec < 151) {
                            SetConnectionQualityAverage(i + 1);
                        } else {
                            SetConnectionQualityBad(i + 1);
                        }
                    }
                }
                for (; i < 4; i++) {
                    TurnOffPingCircles(i + 1);
                }
            }

            if (m_bTestStartGameTimeOut) {
                if (m_startGameTimer.GetSeconds() < RealTimer.GetSeconds()) {
                    m_bTestStartGameTimeOut = false;
                    if (msgHandle) {
                        DialogInterface::DismissDialog(msgHandle);
                        msgHandle = 0;
                    }
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x2c03fba3, 0x2c03fba3,
                                                   GetLocalizedString(TheOnlineManager.IsServer() ? 0x9b90975b : 0xb62e143c));
                    TheOnlineManager.Disconnect(true);
                    BuddyCore::instance()->resume();
                }
            }
            break;
        case 0x2c0e485a:
            ExitOnlineMode(GetPackageName());
            break;
        case 0x2c03fba3:
            ExitToOnlineMain();
            break;
        case 0x37c873af: {
            LobbyApiPlayT *mygame = LobbyGames::Instance().GetMyGame();
            int num_players = mygame ? mygame->iCount : 0;

            TheOnlineManager.InitAntiCheating(num_players);
            LobbyCore::Instance().Suspend();
            TheOnlineManager.StartOnlineRace();
            if (FEDatabase->GetOnlineSettings()->GameDetails != eHOSTING) {
                if (msgHandle) {
                    DialogInterface::DismissDialog(msgHandle);
                    msgHandle = 0;
                }
                msgHandle = 0;
                DialogInterface::ShowMessage(GetPackageName(), "", dialog_alert, 0x9c6e60a4);
                m_startGameTimer.SetTime(RealTimer.GetSeconds() + 60.0f + 10.0f);
                m_bTestStartGameTimeOut = true;
            }
            break;
        }
        case 0xda5b8712:
            LobbyGameSessions::Instance().FinishSessionChanges();
            if (FEDatabase->IsLANMode()) {
                bEnableEAMessenger = false;
            } else {
                bEnableEAMessenger = true;
            }
            m_viewingModalDisplay = false;
            m_bShowingClientModal = false;
            m_bShowingHostModal = false;
            if (m_sendPrivateMsg) {
                char buf[256] = "";
                FEngSNPrintf(buf, 256, "%s", FEngGetEditedString());
                LobbyChat::Instance().SendChatMessage(buf, static_cast<PlayerScrollerDatum *>(playerScroller.GetSelectedDatum())->name,
                                                      nullptr, this);
            } else {
                LobbyChat::Instance().SendChatMessage(FEngGetEditedString(), nullptr, nullptr, this);
            }
            m_sendPrivateMsg = false;
            FEngSetLanguageHash(GetPackageName(), 0x759898cb, 0x7d485221);
            PopulatePlayerList();
            ScrollPlayerList(0);
            break;
        case 0xc9d30688:
            LobbyGameSessions::Instance().FinishSessionChanges();
            if (FEDatabase->IsLANMode()) {
                bEnableEAMessenger = false;
            } else {
                bEnableEAMessenger = true;
            }
            m_sendPrivateMsg = false;
            m_viewingModalDisplay = false;
            m_bShowingClientModal = false;
            m_bShowingHostModal = false;
            FEngSetLanguageHash(GetPackageName(), 0x759898cb, 0x7d485221);
            PopulatePlayerList();
            ScrollPlayerList(0);
            break;
        case 0xfdac2d2b:
            m_setDimState = true;
            m_viewingModalDisplay = true;
            playerScroller.UnHighlightSelected();
            bEnableEAMessenger = false;
            FEngSetAllObjectsInPackageVisibility(GetPackageName(), false);
            cFEng::Get()->QueuePackagePush("UI_OLViewTrack.fng", 0, 0, false);
            break;
        case 0xcac72e3d:
            FEngSetAllObjectsInPackageVisibility(GetPackageName(), true);
            m_viewingModalDisplay = false;
            PopulatePlayerList();
            cFEng::Get()->QueueGameMessage(0x7b3f16cd, GetPackageName(), 0xff);
            bEnableEAMessenger = true;
            break;
        case 0xc3dad747:
            if ((RealTimer - KBCreationTimer).GetSeconds() < g_KBDelaySeconds) {
                break;
            }
            m_sendPrivateMsg = true;
            m_setDimState = true;
            m_viewingModalDisplay = true;
            ScrollPlayerList(0);
            FEngBeginTextInput(0x759898cb, 0, "", "", 115);
            bEnableEAMessenger = false;
            break;
        case 0x613697d7:
            VoiceCore::instance().ToggleMuted(mSelectedPlayerDatum->name);
            break;
        case 0x0f918baa:
            DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x6a87dded, LANGUAGE_COMMON_CANCEL, 0xd05fc3a3, 0x1fab5998,
                                            0x1fab5998, first_dialog_button2, 0x99d8bae0);
            break;
        case 0xd05fc3a3:
            LobbyUsers::Instance().SubmitFeedback(mSelectedPlayerDatum->name, mChat.GetChatBuffer(),
                                                  LobbyCmd_callback<&UIOLGameRoom::SubmitComplaintCB>::call, this);
            msgHandle = DialogInterface::ShowMessage(GetPackageName(), "animating", dialog_none, 0xff12b5c5);
            break;
        case 0xfa94652c:
            SendChallengeInvite(nullptr);
            break;
        case 0xd9656bec:
            m_bShowingHostModal = false;
            m_bShowingClientModal = false;
            FEngSetScript(GetPackageName(), 0xfc8c5406, FEHASH_SHOW, true);
            FEngSetScript(GetPackageName(), 0xcd3f1d2f, FEHASH_SHOW, true);
            ScrollPlayerList(0);
            break;
        case 0x97c1b72f:
        case 0x4fbee42a:
        case 0x67bfa959:
        case 0x7b3f16cd:
        case 0xbb6aa4b5:
        case 0xe1fde1d1:
        case 0xf9e94d58:
            break;
    }
}

void UIOLGameRoom::ExitToOnlineMain() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }

    if (FEDatabase->IsLANMode()) {
        FEDatabase->ResetGameMode();
        FEDatabase->SetGameMode(eFE_GAME_MODE_LAN);
    } else {
        FEDatabase->ResetGameMode();
        FEDatabase->SetGameMode(eFE_GAME_MODE_ONLINE);
    }

    cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
}

void UIOLGameRoom::SendChallengeInvite(char *name) {
    bool newChallenge = false;

    if (!name) {
        name = gBuddySettings.buddyName;
        newChallenge = true;
    }

    BuddySettings buddySettings = EA_Messenger_GetBuddySettingsByName(name);
    if (buddySettings.isPassive) {
        DialogInterface::ShowMessage(GetPackageName(), "", dialog_alert, 0xa424c082, name);
        return;
    }

    bool bAlreadyInvited = false;
    for (int i = 0; i < 4; i++) {
        if (!bStrCmp(g_inviteList.pInvitedPlayerNames[i], name)) {
            if (newChallenge) {
                if (gBuddySettings.IsSameProduct) {
                    g_inviteList.isXGameInvite[i] = false;
                } else {
                    g_inviteList.isXGameInvite[i] = true;
                }
            }
            bAlreadyInvited = true;
            break;
        }
    }

    if (g_inviteList.numberOfInvitedPlayers < cOnlineSettings::MaxOnlinePlayers && !bAlreadyInvited) {
        bStrCpy(g_inviteList.pInvitedPlayerNames[g_inviteList.numberOfInvitedPlayers++], name);
        if (gBuddySettings.IsSameProduct) {
            g_inviteList.isXGameInvite[g_inviteList.numberOfInvitedPlayers - 1] = false;
        } else {
            g_inviteList.isXGameInvite[g_inviteList.numberOfInvitedPlayers - 1] = true;
        }
    }
}

void UIOLGameRoom::CheckForBuddyOnline() {
    if (FEDatabase->IsLANMode()) {
        return;
    }

    for (int i = 0; i < g_inviteList.numberOfInvitedPlayers; i++) {
        if (g_inviteList.isXGameInvite[i]) {
            BuddySettings settings = EA_Messenger_GetBuddySettings(BuddyCore::instance()->getBuddyByName(g_inviteList.pInvitedPlayerNames[i]));
            if (settings.IsSameProduct) {
                HLBListAnswerGameInvite(BuddyCore::instance()->getApiRef(), settings.buddyName, 2, 0, 0);
                LobbyChat::Instance().SendGameInvite(settings.buddyName);
                g_inviteList.isXGameInvite[i] = false;
                SendChallengeInvite(settings.buddyName);
                DialogInterface::ShowOneButton(GetTopPackage(), "", dialog_alert, LANGUAGE_COMMON_OK, 0xffffffff, 0xffffffff,
                                               GetLocalizedString(0x635cb250), settings.buddyName);
            }
        }
    }
}

void UIOLGameRoom::ViewPlayersCar(PlayerScrollerDatum *pPlayerDatum) {}

void UIOLGameRoom::Redraw(bool dim) {
    if (dim) {
        playerScroller.UnHighlightSelected();
    } else {
        if (m_viewingModalDisplay) {
            return;
        }

        if (playerScroller.GetSelectedDatum() &&
            !bStrCmp(static_cast<PlayerScrollerDatum *>(playerScroller.GetSelectedDatum())->name,
                     FEDatabase->GetOnlineSettings()->GetLobbyPersona())) {
            FEngSetLanguageHash(GetPackageName(), 0x55fa05c3, 0x36282214);
        } else {
            FEngSetLanguageHash(GetPackageName(), 0x55fa05c3, 0x552094e0);
        }
    }
}

void UIOLGameRoom::Setup() {
    char sztemp[36];

    FEngSetScript(GetPackageName(), 0xc52b0819, FEHASH_HIDE, true);
    FEngSetScript(GetPackageName(), 0x6e266928, FEHASH_HIDE, true);
    FEngSetScript(GetPackageName(), 0xabf6f060, FEHASH_HIDE, true);
    FEngSetScript(GetPackageName(), 0x6a0831f6, FEHASH_HIDE, true);

    for (int i = 0; i < cOnlineSettings::MaxOnlinePlayers; i++) {
        ScrollerSlot *pScrollerSlot = new ("ScrollerSlot", 0) ScrollerSlot;
        FEngSNPrintf(sztemp, 36, "scroll_text_%d", i + 1);
        FEString *str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pScrollerSlot->AddData(str);
        playerScroller.AddSlot(pScrollerSlot);
    }

    if (FEDatabase->GetOnlineSettings()->GameDetails == eHOSTING) {
        FEPrintf(GetPackageName(), 0xda0a16cc, GetLocalizedString(0xfece4e04));
    } else {
        FEPrintf(GetPackageName(), 0xda0a16cc, GetLocalizedString(0x459ea230));
    }

    FEManager::Get()->AllowControllerError(true);
}

void UIOLGameRoom::StartGame() {
    LobbyGames &lobbyGames = LobbyGames::Instance();
    int rc = lobbyGames.GetMyGame()->iCount > 1;

    if (rc > 0) {
        LobbyChat::Instance().CancelAllInvites();
        rc = lobbyGames.StartGame(StartGameCB, this, false);
        if (rc > 0) {
            if (!FEDatabase->IsLANMode()) {
                for (int i = 0; i < 4; i++) {
                    if (g_inviteList.pInvitedPlayerNames[i]) {
                        HLBListAnswerGameInvite(BuddyCore::instance()->getApiRef(), g_inviteList.pInvitedPlayerNames[i], 2, 0, 0);
                    }
                }
                RichPresence::Instance()->SetBuddyState(HLB_STAT_PASSIVE);
            }
            FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_REQUEST_PENDING);
            msgHandle = 0;
            DialogInterface::ShowMessage(GetPackageName(), "", dialog_alert, 0x9c6e60a4);
            m_startGameTimer.SetTime(RealTimer.GetSeconds() + 60.0f + 5.0f);
            m_bTestStartGameTimeOut = true;
        } else if (rc == 0) {
            lobbyGames.UnlockGame(nullptr, nullptr);
            DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0xd616e55a));
        } else {
            DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0xd99cfe5e));
        }
    } else if (rc == 0) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0xffffffff, 0xffffffff,
                                       "Game failed to start. <UNLOCALIZED TEXT>");
    }
}

void UIOLGameRoom::LeaveSession() {
    LobbyChat::Instance().CancelAllInvites();

    if (LobbyGameSessions::Instance().GetMySession()) {
        if (LobbyGameSessions::Instance().LeaveSession(LeaveSessionCB, this) > 0) {
            FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_REQUEST_PENDING);
        } else {
            DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x2c0e485a, GetLocalizedString(0xacc1f7da));
        }
    } else {
        LobbyGameSessions::Instance().LeaveSession(nullptr, this);
        RichPresence::Instance()->SetBuddyState(HLB_STAT_CHAT);
        if (FEDatabase->GetOnlineSettings()->GetState() == cOnlineSettings::OLS_CONNECTED_TO_ISP) {
            TheOnlineManager.Disconnect(true);
        }
        ExitToOnlineMain();
    }

    RichPresence::Instance()->SetBuddyState(HLB_STAT_CHAT);
}

static bool is_this_guy_voice_chatting(const char *persona, const BuddySettings &buddySettings) {
    int channel;

    if (buddySettings.isBlocked) {
        return false;
    }
    if (!VoiceCore::instance().IsInVOIPChat(persona, &channel)) {
        return false;
    }
    return buddySettings.voipState > 0 || VoiceCore::instance().IsActive(channel);
}

void UIOLGameRoom::ShowVOIPStatus() {
    if (FEDatabase->GetOnlineSettings()->GetState() < cOnlineSettings::OLS_LOBBY_IDLE) {
        return;
    }

    LobbyApiPlayT *myGame = LobbyGames::Instance().GetMyGame();
    if (!myGame || m_viewingModalDisplay) {
        return;
    }

    VoiceCore &voice = VoiceCore::instance();
    for (int i = 0; i < myGame->iCount; i++) {
        uint32 voiphash = FEngHashString("STATUS_%d", i + 1);

        FEngSetScript(GetPackageName(), voiphash, FEHASH_HIDE, true);
        if (m_viewingModalDisplay) {
            continue;
        }

        if (!bStrCmp(myGame->aOpponents[i].strPers, FEDatabase->GetOnlineSettings()->GetLobbyPersona())) {
            if (voice.IsHeadsetConnected()) {
                FEngSetTextureHash(GetPackageName(), voiphash, FEngHashString("OL_HEADSET"));
                FEngSetScript(GetPackageName(), voiphash, FEHASH_SHOW, true);
            }
        } else if (BuddyCore::instance()->getApiRef()) {
            HLBBudT *pBuddy = BuddyCore::instance()->getBuddyByName(myGame->aOpponents[i].strPers);
            if (!pBuddy) {
                HLBListFlagTempBuddy(BuddyCore::instance()->getApiRef(), myGame->aOpponents[i].strPers, 64);
                pBuddy = BuddyCore::instance()->getBuddyByName(myGame->aOpponents[i].strPers);
            }

            BuddySettings buddySettings = EA_Messenger_GetBuddySettingsByName(myGame->aOpponents[i].strPers);
            if (buddySettings.isBlocked) {
                FEngSetTextureHash(GetPackageName(), voiphash, FEngHashString("OL_HEADSET_MUTE"));
                FEngSetScript(GetPackageName(), voiphash, FEHASH_SHOW, true);
                if (!VoiceCore::instance().IsMuted(buddySettings.buddyName)) {
                    VoiceCore::instance().ToggleMuted(buddySettings.buddyName);
                }
            } else if (is_this_guy_voice_chatting(myGame->aOpponents[i].strPers, buddySettings)) {
                if (voice.IsMuted(myGame->aOpponents[i].strPers)) {
                    FEngSetTextureHash(GetPackageName(), voiphash, FEngHashString("OL_HEADSET_MUTE"));
                    FEngSetScript(GetPackageName(), voiphash, FEHASH_SHOW, true);
                } else if (voice.IsConnected(myGame->aOpponents[i].strPers)) {
                    if (voice.IsSpeaking(myGame->aOpponents[i].strPers)) {
                        FEngSetTextureHash(GetPackageName(), voiphash, FEngHashString("OL_VOICE"));
                        FEngSetScript(GetPackageName(), voiphash, FEHASH_SHOW, true);
                    } else if (HLBBudGetVOIPState(pBuddy) == 1) {
                        FEngSetTextureHash(GetPackageName(), voiphash, FEngHashString("OL_HEADSET"));
                        FEngSetScript(GetPackageName(), voiphash, FEHASH_SHOW, true);
                    }
                }
            } else if (HLBBudGetVOIPState(pBuddy) == 1) {
                FEngSetTextureHash(GetPackageName(), voiphash, FEngHashString("OL_HEADSET"));
                FEngSetScript(GetPackageName(), voiphash, FEHASH_SHOW, true);
            }
        }
    }
}

void UIOLGameRoom::InitializePlayerList() {
    playerScroller.DeleteScrollerData();

    for (int i = 0; i < cOnlineSettings::MaxOnlinePlayers + 1; i++) {
        PlayerScrollerDatum *pPlayerDatum = new ("PlayerScrollerDatum", 0) PlayerScrollerDatum;
        pPlayerDatum->AddData("", 0);
        FEngSNPrintf(pPlayerDatum->name, 16, "\0");
        playerScroller.AddData(pPlayerDatum);
        playerScroller.Disable(pPlayerDatum);
        FEngSetScript(GetPackageName(), FEngHashString("STATUS_%d", i + 1), FEHASH_HIDE, true);
    }

    playerScroller.Reset(true);
    ScrollPlayerList(0);
}

void UIOLGameRoom::PopulatePlayerList() {
    GameSession *mySession = LobbyGameSessions::Instance().GetMySession();
    LobbyApiPlayT *myGame = LobbyGames::Instance().GetMyGame();
    int selectedSlot = playerScroller.GetSelectedSlotIndex();
    PlayerScrollerDatum *pSelectedDatum = static_cast<PlayerScrollerDatum *>(playerScroller.GetSelectedDatum());
    uint32 MyNameHash = FEngHashString(FEDatabase->GetOnlineSettings()->GetLobbyPersona());
    ScrollerSlot *pSelectedSlot = nullptr;
    ScrollerSlot *pMySlot = nullptr;
    uint32 SelectedHash = 0;
    ScrollerSlot *pScrollerSlot;

    if (!mySession || LobbyCore::Instance().IsSuspended() == true) {
        InitializePlayerList();
        return;
    }

    if (playerScroller.GetSelectedSlot() != m_startReadySlot && pSelectedDatum) {
        SelectedHash = FEngHashString(pSelectedDatum->name);
    }

    playerScroller.DeleteScrollerData();
    AddSessionPlayers();

    for (int i = mySession->iCount; i < mySession->iSize; i++) {
        AddOpenSlotAtIndex(i);
    }

    for (int i = mySession->iSize; i < cOnlineSettings::MaxOnlinePlayers; i++) {
        PlayerScrollerDatum *pPlayerDatum = new ("PlayerScrollerDatum", 0) PlayerScrollerDatum;
        pPlayerDatum->AddData("", 0);
        FEngSNPrintf(pPlayerDatum->name, 16, "\0");
        playerScroller.AddData(pPlayerDatum);
        playerScroller.Disable(pPlayerDatum);
        FEngSetScript(GetPackageName(), FEngHashString("READY_INDICATOR_%d", i + 1), FEHASH_HIDE, true);
        FEngSetScript(GetPackageName(), FEngHashString("STATUS_%d", i + 1), FEHASH_HIDE, true);
        FEngSetScript(GetPackageName(), FEngHashString("HIGH_LIGHT_%d", i + 1), FEHASH_UNHIGHLIGHT, true);
    }

    pScrollerSlot = playerScroller.GetFirstSlot();
    for (uint32 i = 1; i < playerScroller.GetNumSlots(); i++) {
        pScrollerSlot = pScrollerSlot->GetNext();
        uint32 datumhash = FEngHashString(static_cast<PlayerScrollerDatum *>(playerScroller.FindDatumInSlot(pScrollerSlot))->name);
        if (SelectedHash && SelectedHash == datumhash && datumhash != 0x0f636a00) {
            pSelectedSlot = pScrollerSlot;
        }
        if (MyNameHash == datumhash) {
            pMySlot = pScrollerSlot;
        }
    }

    playerScroller.Reset(true);
    if (!m_bSetSelectionToUser && pMySlot) {
        m_bSetSelectionToUser = true;
        playerScroller.SetSelected(pMySlot);
    } else if (pSelectedSlot) {
        playerScroller.SetSelected(pSelectedSlot);
    } else {
        playerScroller.SetSelected(playerScroller.GetSlot(selectedSlot ? selectedSlot - 1 : 0));
    }
    playerScroller.HighlightSelected();
    playerScroller.Update(true);
}

void UIOLGameRoom::AddOpenSlotAtIndex(int32 index) {
    PlayerScrollerDatum *pPlayerDatum = new ("PlayerScrollerDatum", 0) PlayerScrollerDatum;
    pPlayerDatum->AddData(GetLocalizedString(0x0f636a00), 0);
    FEngSNPrintf(pPlayerDatum->name, 16, GetLocalizedString(0x0f636a00));

    playerScroller.AddData(pPlayerDatum);
    playerScroller.Disable(pPlayerDatum);

    FEngSetScript(GetPackageName(), FEngHashString("READY_INDICATOR_%d", index + 1), FEHASH_HIDE, true);
    FEngSetScript(GetPackageName(), FEngHashString("STATUS_%d", index + 1), FEHASH_HIDE, true);
    FEngSetScript(GetPackageName(), FEngHashString("HIGH_LIGHT_%d", index + 1), FEHASH_UNHIGHLIGHT, true);

    FEngSetScript(GetPackageName(), FEngHashString("SCROLL_TEXT_%d", index + 1), FEHASH_HIDE, true);
}

void UIOLGameRoom::AddSessionPlayers() {
    LobbyGameSessions &lgs = LobbyGameSessions::Instance();
    GameSession *mySession = lgs.GetMySession();
    LobbyApiPlayT *myGame = LobbyGames::Instance().GetMyGame();

    for (int i = 0; i < mySession->iCount; i++) {
        GameSessionMember *member = lgs.GetMemberByIndex(i);
        if (!member) {
            AddOpenSlotAtIndex(i);
            continue;
        }

        uint32 NumDisqualifications;
        PlayerScrollerDatum *pPlayerDatum = new ("PlayerScrollerDatum", 0) PlayerScrollerDatum;

        if (myGame) {
            if (!FEDatabase->IsLANMode()) {
                char name[32] = "";
                if (bStrCmp(member->strPers, FEDatabase->GetOnlineSettings()->GetLobbyPersona())) {
                    bStrCpy(name, member->strPers);
                    HLBListFlagTempBuddy(BuddyCore::instance()->getApiRef(), name, 64);
                }
            }
        }

        pPlayerDatum->makingChanges = lgs.IsMemberMakingChanges(i);
        pPlayerDatum->AddData(member->strPers, 0);
        FEngSNPrintf(pPlayerDatum->name, 16, member->strPers);

        NumDisqualifications = 0;
        LobbyUsers &lobbyUsers = LobbyUsers::Instance();
        OnlineRaceModeE currentRaceMode = lgs.GetRaceMode(nullptr);
        if (!lobbyUsers.GetUserRecord(mySession->strOwner) ||
            !lobbyUsers.GetCarNameFromUserRecord(member->strPers, pPlayerDatum->carNameHash, pPlayerDatum->carPerformance) ||
            !lobbyUsers.GetPointsFromUserRecord(member->strPers, pPlayerDatum->points, currentRaceMode) ||
            !lobbyUsers.GetDisqualificationsFromUserRecord(member->strPers, NumDisqualifications)) {
            lobbyUsers.GetUserOnlineRecord(member->strPers, GetUserOnlineRecordCB, this);
        } else {
            pPlayerDatum->m_bValidData = true;
        }

        if (!FEDatabase->GetOnlineSettings()->RankedGame) {
            NumDisqualifications = 0;
        }
        gGetDisqRatingHighlightsFor(NumDisqualifications);

        playerScroller.AddData(pPlayerDatum);
        playerScroller.Enable(pPlayerDatum);
        FEngSetScript(GetPackageName(), FEngHashString("SCROLL_TEXT_%d", i + 1), FEHASH_SHOW, true);
    }
}

void UIOLGameRoom::ScrollPlayerList(int direction) {
    if (direction < 0) {
        playerScroller.ScrollPrev();
    } else if (direction > 0) {
        playerScroller.ScrollNext();
    }

    playerScroller.HighlightSelected();
    playerScroller.Update(true);
    Redraw(false);
}

void UIOLGameRoom::SetDelayedDialog(uint32 titleHash, uint32 pressMessage, uint32 cancelMessage, const char *text) {
    delayedDialogDetails.dialogTitle = titleHash;
    delayedDialogDetails.buttonPressedMessage = pressMessage;
    delayedDialogDetails.buttonCancelMessage = cancelMessage;
    delayedDialogDetails.dialogText = text;

    delayedDialogDetails.showDialog = true;
}

void UIOLGameRoom::MaybeShowDelayedDialog() {
    if (delayedDialogDetails.showDialog == true) {
        delayedDialogDetails.showDialog = false;
        if (msgHandle) {
            DialogInterface::DismissDialog(msgHandle);
            msgHandle = 0;
        }
        msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, delayedDialogDetails.dialogTitle,
                                                   delayedDialogDetails.buttonPressedMessage, delayedDialogDetails.buttonCancelMessage,
                                                   delayedDialogDetails.dialogText);
    }
}

void UIOLGameRoom::MaybeDismissDialog() {
    if (doDismissDialog == true) {
        doDismissDialog = false;
        if (msgHandle) {
            DialogInterface::DismissDialog(msgHandle);
            msgHandle = 0;
        }
    }
}

void UIOLGameRoom::MaybeSwitchToLobby() {
    if (doSwitchToLobby == true) {
        doSwitchToLobby = false;
        if (FEDatabase->GetOnlineSettings()->QuickJoinRace || FEDatabase->GetOnlineSettings()->GameDetails == eHOSTING) {
            ExitToOnlineMain();
        } else {
            cFEng::Get()->QueuePackageSwitch(g_LOBBY_ROOM_SCREEN, 0, 0, false);
        }
    }
}

static int GameRoomConnectionUnknownHash[4] = {0x61726c50, 0x61726c51, 0x61726c52, 0x61726c53};

void UIOLGameRoom::SetConnectionQualityGood(uint32 index) {
    char temp[32] = "";

    for (int32 col = 1; col < 4; col++) {
        FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", index, col);
        FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
        if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, FEHASH_GOODCONNECTION)) {
            FEngSetScript(GetPackageName(), obj->NameHash, FEHASH_GOODCONNECTION, true);
        }
    }
    if (!FEngIsScriptRunning(GetPackageName(), GameRoomConnectionUnknownHash[index - 1], FEHASH_HIDE)) {
        FEngSetScript(GetPackageName(), GameRoomConnectionUnknownHash[index - 1], FEHASH_HIDE, true);
    }
}

void UIOLGameRoom::SetConnectionQualityAverage(uint32 index) {
    char temp[32] = "";

    for (int32 col = 1; col < 3; col++) {
        FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", index, col);
        FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
        if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, FEHASH_REGCONNECTION)) {
            FEngSetScript(GetPackageName(), obj->NameHash, FEHASH_REGCONNECTION, true);
        }
    }

    FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", index, 3);
    FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
    if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, FEHASH_OFF)) {
        FEngSetScript(GetPackageName(), obj->NameHash, FEHASH_OFF, true);
    }
    if (!FEngIsScriptRunning(GetPackageName(), GameRoomConnectionUnknownHash[index - 1], FEHASH_HIDE)) {
        FEngSetScript(GetPackageName(), GameRoomConnectionUnknownHash[index - 1], FEHASH_HIDE, true);
    }
}

void UIOLGameRoom::SetConnectionQualityBad(uint32 index) {
    char temp[32] = "";

    FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", index, 1);
    FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
    if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, FEHASH_BADCONNECTION)) {
        FEngSetScript(GetPackageName(), obj->NameHash, FEHASH_BADCONNECTION, true);
    }

    for (int32 col = 2; col < 4; col++) {
        FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", index, col);
        obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
        if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, FEHASH_OFF)) {
            FEngSetScript(GetPackageName(), obj->NameHash, FEHASH_OFF, true);
        }
    }
    if (!FEngIsScriptRunning(GetPackageName(), GameRoomConnectionUnknownHash[index - 1], FEHASH_HIDE)) {
        FEngSetScript(GetPackageName(), GameRoomConnectionUnknownHash[index - 1], FEHASH_HIDE, true);
    }
}

void UIOLGameRoom::SetConnectionQualityUnknown(uint32 index) {
    char temp[32] = "";

    for (int32 col = 1; col < 4; col++) {
        FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", index, col);
        FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
        if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, FEHASH_UNKNOWN)) {
            FEngSetScript(GetPackageName(), obj->NameHash, FEHASH_UNKNOWN, true);
        }
    }
    if (!FEngIsScriptRunning(GetPackageName(), GameRoomConnectionUnknownHash[index - 1], FEHASH_SHOW)) {
        FEngSetScript(GetPackageName(), GameRoomConnectionUnknownHash[index - 1], FEHASH_SHOW, true);
    }
}

void UIOLGameRoom::TurnOffPingCircles(uint32 index) {
    char temp[32] = "";

    for (int32 col = 1; col < 4; col++) {
        FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", index, col);
        FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
        if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, FEHASH_OFF)) {
            FEngSetScript(GetPackageName(), obj->NameHash, FEHASH_OFF, true);
        }
    }
    if (!FEngIsScriptRunning(GetPackageName(), GameRoomConnectionUnknownHash[index - 1], FEHASH_HIDE)) {
        FEngSetScript(GetPackageName(), GameRoomConnectionUnknownHash[index - 1], FEHASH_HIDE, true);
    }
}

void UIOLGameRoom::MaybeUpdateSession() {
    if (doUpdateSession) {
        doUpdateSession = false;

        GameSession *session = LobbyGameSessions::Instance().GetMySession();
        if (session) {
            RichPresence::Instance()->SetSession(session->strName);
            RichPresence::Instance()->SetEventHash(TagFieldGetNumber(TagFieldFind(session->strParams, "E"), 0));
            RichPresence::Instance()->SetSessionPasswordProtected(FEDatabase->GetOnlineSettings()->Passworded);
            RichPresence::Instance()->SetPassword(FEDatabase->GetOnlineSettings()->Password);
            RichPresence::Instance()->SetSessionPrivate(FEDatabase->GetOnlineSettings()->IsPrivateRoom);

            PopulatePlayerList();

            if (LobbyGames::Instance().GetMyGame()) {
                DisplayGameDetails(this);
            }
        }
    }
}

void UIOLGameRoom::SessionUpdateCB(LobbyGameSessionsN::SessionStatusCode code, const LobbyApiUserSetT *session, void *context) {
    UIOLGameRoom *gameRoom = static_cast<UIOLGameRoom *>(context);

    switch (code) {
        case LobbyGameSessionsN::SESSION_DELETED:
            DialogInterface::ShowOneButton(gameRoom->GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x87f32b2a, 0x87f32b2a,
                                           GetLocalizedString(0x4b5c9721));
            gameRoom->FEngAbortTextInput();
            RichPresence::Instance()->SetSession("");
            break;
        case LobbyGameSessionsN::GAME_STARTED: {
            LobbyApiPlayT *game = LobbyGames::Instance().GetMyGame();
            char personas[4][16];

            bMemSet(personas, 0, sizeof(personas));
            for (int i = 0; i < game->iCount; i++) {
                bStrCpy(personas[i], game->aOpponents[i].strPers);
            }
            TheOnlineManager.SetPlayerIDs(personas);
            TheOnlineManager.SetGameSeed(game->uSeed);
            cFEng::Get()->QueueGameMessage(0x37c873af, gameRoom->GetPackageName(), 0xff);
            break;
        }
        case LobbyGameSessionsN::SESSION_CHANGED:
            gameRoom->doUpdateSession = true;
            break;
        case LobbyGameSessionsN::SESSION_KICKED:
            ConnectionCore::Instance().LeaveSession();
            DialogInterface::ShowOneButton(gameRoom->GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x87f32b2a, 0x87f32b2a,
                                           GetLocalizedString(0x7d162708));
            DialogInterface::SetDismissedByAnotherDialogMsg(0x87f32b2a);
            gameRoom->FEngAbortTextInput();
            break;
        case LobbyGameSessionsN::SESSION_LIST_CHANGED:
            break;
    }
}

void UIOLGameRoom::CreateSessionCB(LobbyApiMsgT *msg, void *context) {
    UIOLGameRoom *gameRoomUI = static_cast<UIOLGameRoom *>(context);

    gameRoomUI->doDismissDialog = true;
    if (msg->code == 'invp') {
        gameRoomUI->SetDelayedDialog(LANGUAGE_COMMON_OK, 0x87f32b2a, 0x87f32b2a, GetLocalizedString(0xf2aab6b5));
    } else if (msg->code) {
        gameRoomUI->SetDelayedDialog(LANGUAGE_COMMON_OK, 0x2c0e485a, 0x2c0e485a, GetLocalizedString(0x3f920248));
    } else {
        cFEng::Get()->QueueGameMessage(0xeced0b24, gameRoomUI->GetPackageName(), 0xff);
    }
    FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_IDLE);
}

void UIOLGameRoom::LeaveSessionCB(LobbyApiMsgT *msg, void *context) {
    UIOLGameRoom *gameRoom = static_cast<UIOLGameRoom *>(context);

    if (msg->code == 0 || msg->code == 'nfnd' || msg->code == 'join') {
        ConnectionCore::Instance().LeaveSession();
        gameRoom->doSwitchToLobby = true;
    } else if (msg->code != 'asta' && msg->code != 'lock') {
        gameRoom->SetDelayedDialog(LANGUAGE_COMMON_OK, 0x2c0e485a, 0x2c0e485a, GetLocalizedString(0xacc1f7da));
    }
    FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_IDLE);
    RichPresence::Instance()->SetSession("");
}

void UIOLGameRoom::StartGameCB(LobbyApiMsgT *msg, void *context) {
    if (msg->code) {
        static_cast<UIOLGameRoom *>(context)->SetDelayedDialog(LANGUAGE_COMMON_OK, 0xffffffff, 0xffffffff, GetLocalizedString(0xd99cfe5e));
    }
    FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_IDLE);
}

void UIOLGameRoom::GetUserOnlineRecordCB(LobbyApiMsgT *msg, void *context) {
    static_cast<UIOLGameRoom *>(context)->doUpdateSession = true;
}

void UIOLGameRoom::SubmitComplaintCB(LobbyApiMsgT *msg) {
    doDismissDialog = true;

    if (msg->code) {
        SetDelayedDialog(LANGUAGE_COMMON_OK, 0xffffffff, 0xffffffff, "Unable to submit complaint");
    } else {
        SetDelayedDialog(LANGUAGE_COMMON_OK, 0xffffffff, 0xffffffff, GetLocalizedString(0xa19617bd));
    }
}

void UIOLGameRoom::DisplayGameDetails(MenuScreen *pGameRoom) {
    GameSession *session = LobbyGameSessions::Instance().GetMySession();
    GRaceParameters *parms;
    uint32 hash;
    uint32 texhash;
    const char *distUnits;

    hash = TagFieldGetNumber(TagFieldFind(session->strParams, "E"), 0);
    parms = GRaceDatabase::Get().GetRaceFromHash(hash);

    switch (parms->GetRaceType()) {
        case GRace::kRaceType_Circuit:
            hash = 0x034fa2c1;
            texhash = 0xe9638d3e;
            break;
        case GRace::kRaceType_P2P:
            hash = 0xb94fd70e;
            texhash = 0x2521e5eb;
            break;
        case GRace::kRaceType_Drag:
            hash = 0x6f547e4c;
            texhash = 0xaaab31e9;
            break;
        case GRace::kRaceType_Knockout:
            hash = 0x4930f5fc;
            texhash = 0x3a015595;
            break;
        default:
            hash = 0x001b01cc;
            texhash = 0x3a015595;
            break;
    }

    FEngSetTextureHash(pGameRoom->GetPackageName(), 0xceb82625, texhash);
    FEngSetLanguageHash(pGameRoom->GetPackageName(), 0x4b4e3888, hash);

    hash = CalcLanguageHash("TRACKNAME_", parms);
    if (DoesStringExist(hash)) {
        FEngSetLanguageHash(pGameRoom->GetPackageName(), 0x4b4e3889, hash);
    } else {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e3889, parms->GetEventID());
    }

    hash = static_cast<uint8>(TagFieldGetNumber(TagFieldFind(session->strParams, "M"), 0));
    FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388a, "%d", hash);

    if (session->uCustFlags & 0x200) {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388b, GetLocalizedString(LANGUAGE_COMMON_YES));
    } else {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388b, GetLocalizedString(LANGUAGE_COMMON_NO));
    }

    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        distUnits = GetLocalizedString(0x8569a26a);
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388c, "%0.1f %s", parms->GetRaceLengthMeters() / 1000.0f, distUnits);
    } else {
        distUnits = GetLocalizedString(0x867dcfd9);
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388c, "%0.1f %s", parms->GetRaceLengthMeters() / 1609.34f, distUnits);
    }

    hash = static_cast<uint8>(TagFieldGetNumber(TagFieldFind(session->strParams, "L"), 0));
    if (parms->GetRaceType() == GRace::kRaceType_Circuit) {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388d, "%d", hash);
    } else {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388d, GetLocalizedString(0x7f2f7ad6));
    }

    if (session->uCustFlags & 0x1000) {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388e, GetLocalizedString(0xa1cd823e));
    } else {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388e, GetLocalizedString(0xde6eff34));
    }

    if (session->uCustFlags & 0x800) {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388f, GetLocalizedString(LANGUAGE_COMMON_ON));
    } else {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e388f, GetLocalizedString(LANGUAGE_COMMON_OFF));
    }

    if (FEDatabase->GetOnlineSettings()->PerformanceMatching) {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e3890, GetLocalizedString(LANGUAGE_COMMON_ON));
    } else {
        FEPrintf(pGameRoom->GetPackageName(), 0x4b4e3890, GetLocalizedString(LANGUAGE_COMMON_OFF));
    }

    if (session->uCustFlags & 0x400) {
        FEPrintf(pGameRoom->GetPackageName(), 0xb51549b8, GetLocalizedString(LANGUAGE_COMMON_ON));
    } else {
        FEPrintf(pGameRoom->GetPackageName(), 0xb51549b8, GetLocalizedString(LANGUAGE_COMMON_OFF));
    }
}

int UIOLGameRoom::mGameCountdown = -1;

void UIOLGameRoom::CheckDisplayOfHostStartCountdown() {
    if (m_hostStartUpdateTimer.GetSeconds() < RealTimer.GetSeconds() && !m_isRaceStarted) {
        m_hostStartUpdateTimer.SetTime(RealTimer.GetSeconds() + 0.1f);

        if (LobbyGameSessions::Instance().GetMySession()) {
            mGameCountdown = LobbyGameSessions::Instance().GetSecsBeforeHostCanStart();

            if (mGameCountdown > 0) {
                FEngSetScript(GetPackageName(), 0xc52b0819, FEHASH_SHOW, true);
                FEngSetScript(GetPackageName(), 0x6e266928, FEHASH_SHOW, true);
                FEngSetLanguageHash(GetPackageName(), 0x6e266928, 0x250d0152);
                FEngSetScript(GetPackageName(), 0xabf6f060, FEHASH_SHOW, true);
                FEPrintf(GetPackageName(), 0xabf6f060, "%d", LobbyGameSessions::Instance().GetSecsBeforeHostCanStart());
            } else if (mGameCountdown == 0) {
                FEPrintf(GetPackageName(), 0xabf6f060, "0");
                m_isRaceStarted = true;
                if (FEDatabase->GetOnlineSettings()->GameDetails == eHOSTING) {
                    StartGame();
                }
            } else {
                FEngSetScript(GetPackageName(), 0xabf6f060, FEHASH_HIDE, true);
                FEngSetScript(GetPackageName(), 0xc52b0819, FEHASH_HIDE, true);
            }

            cFEng::Get()->QueueGameMessage(0x971391a2, nullptr, 0xff);
        }
    }
}

UIOLGameRoom::ChatBuffer::ChatBuffer(const char *packagename)
    : mpArrow1(FEngFindObject(packagename, FEHashUpper("Chat_ScrollBar_Arrow_1"))) //
    , mpArrow2(FEngFindObject(packagename, FEHashUpper("Chat_ScrollBar_Arrow_2"))) //
    , mScrollBar(packagename, "Chat_ScrollBar", true, false, false) //
    , mpChatBody(FEngFindString(packagename, 0x4162454c)) //
    , mpFont(FindFont(mpChatBody->Handle)) //
    , mWidth(mpChatBody->MaxWidth) //
    , mFlags(0) //
    , mScroll(2) //
    , mLines(0) {
    mBuffer[0] = '\0';

    mFlags = mpChatBody->Flags |= 0x200;

    FEPrintf(mpChatBody, "");

    Display();
}

int UIOLGameRoom::ChatBuffer::WordWrap(char *message) {
    float width = 0.0f;
    int size = 0;
    int spacesize = 0;
    char *messageptr = message;
    i16 prevChar = 0;
    i16 currChar;

    while (true) {
        currChar = *messageptr++;
        if (!currChar) {
            break;
        }
        if (currChar == ' ') {
            spacesize = size;
        }
        width += mpFont->GetCharacterWidth(currChar, prevChar, mFlags);
        if (width >= mWidth) {
            if (spacesize > 0) {
                size = spacesize;
            }
            break;
        }
        size++;
        prevChar = currChar;
    }
    return size;
}

void UIOLGameRoom::ChatBuffer::AddLine(char *message, int size) {
    int buffersize = strlen(mBuffer);
    char *bufferptr = mBuffer;

    while (size + buffersize + 1 >= 4096) {
        while (*bufferptr++ != '\n') {
            buffersize--;
        }
        buffersize--;
        mLines--;
    }

    if (bufferptr > mBuffer) {
        bStrCpy(mBuffer, bufferptr);
    }

    bufferptr = &mBuffer[buffersize];
    bStrNCpy(bufferptr, message, size);

    bufferptr += size;
    *bufferptr = '\n';
    bufferptr[1] = '\0';

    mLines++;

    if (mScroll > 2) {
        if (++mScroll > mLines) {
            mScroll = mLines;
        }
    }
}

void UIOLGameRoom::ChatBuffer::Display() {
    if (mpChatBody) {
        int linesback = 0;
        char *bufferend = &mBuffer[bStrLen(mBuffer)];
        char *displayptr = bufferend;

        while (displayptr > mBuffer) {
            if (displayptr[-1] == '\n') {
                if (++linesback > mScroll) {
                    break;
                }
            }
            displayptr--;
        }

        char *displayend = displayptr;
        int linesdisplayed = 0;

        while (displayend < bufferend) {
            if (*displayend++ == '\n') {
                if (++linesdisplayed >= 2) {
                    break;
                }
            }
        }

        int length = displayend - displayptr;
        CopyString(mpChatBody->string.AllocateString(length + 1), displayptr, length);
        mpChatBody->Flags |= 0x400002;

        mScrollBar.Update(2, mLines, bMax(1, mLines - mScroll + 1), -1);
    }
}

void UIOLGameRoom::ChatBuffer::Receive(char *message) {
    while (*message) {
        int size = WordWrap(message);

        if (size + 1 >= 4096) {
            return;
        }

        AddLine(message, size);
        message += size;

        while (*message == ' ') {
            message++;
        }
    }

    Display();
}

void UIOLGameRoom::ReceiveChatCB(void *online_chat_panel, char *message, bool is_private, bool is_special, bool is_local) {
    static_cast<ChatBuffer *>(online_chat_panel)->Receive(message);
}

void UIOLGameRoom::ChatBuffer::ScrollChatBodyUp() {
    if (mScroll < mLines) {
        FEngSetScript(mpArrow1, FEHASH_BLINK, true);
        mScroll++;
        Display();
    }
}

void UIOLGameRoom::ChatBuffer::ScrollChatBodyDown() {
    if (mScroll > 2) {
        FEngSetScript(mpArrow2, FEHASH_BLINK, true);
        mScroll--;
        Display();
    }
}

void UIOLGameRoom::UpdateInfoBox() {
    LobbyApiPlayT *myGame = LobbyGames::Instance().GetMyGame();
    GameSession *mySession = LobbyGameSessions::Instance().GetMySession();
    int maxCurrLap = -1;

    m_bChangingSettings = false;
    for (uint32 i = 0; i < playerScroller.GetNumSlots(); i++) {
        PlayerScrollerDatum *datum =
            static_cast<PlayerScrollerDatum *>(playerScroller.FindDatumInSlot(playerScroller.GetSlot(i)));
        FEObject *readyIndicator = FEngFindObject(GetPackageName(), FEngHashString("READY %d", i + 1));

        if (datum && datum->IsEnabled()) {
            if (datum->makingChanges) {
                FEngSetScript(readyIndicator, FEHASH_NOT_READY, true);
                m_bChangingSettings = true;
                break;
            }

            GameSessionMember *thisPlayer = LobbyGameSessions::Instance().GetMemberByIndex(i);
            if (!myGame && thisPlayer && thisPlayer->iGame > 0) {
                int lap;
                int mapx;
                int mapy;

                LobbyGameSessions::Instance().GetMemberRaceStatus(i, lap, mapx, mapy);
                if (lap > maxCurrLap) {
                    maxCurrLap = lap;
                }
                FEngSetScript(readyIndicator, FEHASH_NOT_READY, true);
            } else {
                FEngSetScript(readyIndicator, FEHASH_READY, true);
            }
        } else {
            FEngSetScript(readyIndicator, FEHASH_GREY, true);
        }
    }

    if (m_bChangingSettings) {
        FEngSetScript(GetPackageName(), 0x6a0831f6, FEHASH_SHOW, true);
    } else {
        FEngSetScript(GetPackageName(), 0x6a0831f6, FEHASH_HIDE, true);
    }

    if ((m_InfoBoxTimer - RealTimer).GetSeconds() > 0.0f) {
        return;
    }

    FEngSetScript(GetPackageName(), 0x07792134, FEHASH_HIDE, true);
    if (!myGame) {
        FEngSetScript(GetPackageName(), 0x07792134, FEHASH_SHOW, true);
        if (maxCurrLap > 0 && maxCurrLap <= FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps) {
            FEPrintf(GetPackageName(), 0x2d6970c7, "%s - %s %d / %d", GetLocalizedString(0x1a9afbfe), GetLocalizedString(0xbf9c),
                     maxCurrLap, FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps);
        } else {
            FEPrintf(GetPackageName(), 0x2d6970c7, "%s - %s", GetLocalizedString(0x1a9afbfe), GetLocalizedString(LANGUAGE_HUD_COMPLETE));
        }
        return;
    }

    if (mySession && mySession->iCount < FEDatabase->GetOnlineSettings()->MinOnlinePlayers) {
        FEngSetScript(GetPackageName(), 0x07792134, FEHASH_SHOW, true);
        FEngSetLanguageHash(GetPackageName(), 0x2d6970c7, 0x8c68dfb4);
        return;
    }

    if (LobbyGameSessions::mHurryTimerStopped) {
        FEngSetScript(GetPackageName(), 0x07792134, FEHASH_SHOW, true);
        FEngSetLanguageHash(GetPackageName(), 0x2d6970c7, 0x148f8752);
        m_InfoBoxTimer.SetTime(RealTimer.GetSeconds() + 3.0f);
        LobbyGameSessions::mHurryTimerStopped = false;
    } else if (LobbyGameSessions::mHurryTimerStarted) {
        FEngSetScript(GetPackageName(), 0x07792134, FEHASH_SHOW, true);
        FEngSetLanguageHash(GetPackageName(), 0x2d6970c7, 0x3cd25d49);
        LobbyGameSessions::mHurryTimerStarted = false;
        m_InfoBoxTimer.SetTime(RealTimer.GetSeconds() + 3.0f);
    }
}

const char *GetTopPackage() {
    return nullptr;
}

void ShowDisconnectDialog(const char *pkg, LobbyCoreN::OnlineDisconnectReason reason, int code, uint32 label) {
    if (!TheOnlineManager.IsOnlineRace()) {
        CUIOnlineDisconnect::ShowDialog(reason, code, label);
    } else {
        DialogInterface::ShowOneButton(pkg, "", dialog_alert, LANGUAGE_COMMON_OK, 0x2c0e485a, 0x2c0e485a,
                                       CUIOnlineDisconnect::GetDisconnectMessage(reason, code, label));
    }
}

struct DisconnectMemcardParam {
    LobbyCoreN::OnlineDisconnectReason reason;
    int code;
    uint32 label;
};

void DisconnectCB(LobbyCoreN::OnlineDisconnectReason reason, LobbyApiMsgT *msg, void *context) {
    BuddyCore::instance()->handledisconnect();

    uint32 label = msg->pData ? bStrToLong(msg->pData) : 0;
    static DisconnectMemcardParam param = {reason, msg->code, label};

    CUIOnlineDisconnect::SetDisconnectNotificationPending();
    CUIOnlineDisconnect::SetReconnectFinalState();
    ShowDisconnectDialog(GetTopPackage(), reason, msg->code, label);
    FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_CONNECTED_TO_ISP);
}

void HandleBuddyDisconnect() {
    BuddyCore::instance()->handledisconnect();
    CUIOnlineDisconnect::SetDisconnectNotificationPending();
    CUIOnlineDisconnect::SetReconnectFinalState();
    ShowDisconnectDialog(GetTopPackage(), LobbyCoreN::DISC_DISCONNECTED, 'xxxx', 0x5a6f817f);
    FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_CONNECTED_TO_ISP);
}
