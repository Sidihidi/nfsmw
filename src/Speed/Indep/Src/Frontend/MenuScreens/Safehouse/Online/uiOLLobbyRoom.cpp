#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOLLobbyRoom.hpp"

#include "Speed/Indep/Src/FEng/FEList.h"
#include "Speed/Indep/Src/FEng/FEngStandard.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/Database/RaceDB.hpp"
#include "Speed/Indep/Src/Frontend/FECarViewer.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineDisconnect.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

bool UIOLLobbyRoom::sExittingOnline = false;
const char *UIOLLobbyRoom::m_password = nullptr;

MenuScreen *CreateUIOLLobbyRoomScreen(ScreenConstructorData *sd) {
    UIOLLobbyRoom::sExittingOnline = false;
    return new ("UIOLLobbyRoom", 0) UIOLLobbyRoom(sd);
}

UIOLLobbyRoom::UIOLLobbyRoom(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , GameScroller(GetPackageName(), "scroll_area", "sidescrollbar", true, false, false, true) //
    , TitleScroller(GetPackageName(), "option_button", "scrollbar", false, false, false, true) //
    , bAllowScroll(true) //
    , haveRunFindSessions(false) //
    , numFailedJoinAttempts(0) //
    , msgHandle(0) //
    , curOption(JOIN_BUTTON) //
    , doBadConnectionTest(FEDatabase->GetOnlineSettings()->ShowBadConnectionWarning) //
    , redrawSessionsList(false) //
    , doSwitchToGameRoomScreen(false) //
    , doDismissDialog(false) //
    , bNameAscending(true) //
    , bTypeAscending(true) //
    , bRacersAscending(true) //
    , bPingAscending(true) //
    , bCourseAscending(true) //
    , m_bDisableMouseInput(false) //
    , m_LobbyApiMsgT_code(0) //
    , m_bPerformJoinSession(false) //
    , m_currentSortColumn(3) //
    , m_PlayerDisconnectMinThreshold(OLS_DISCONNECT_PERC_ANY) //
#ifdef EA_PLATFORM_XENON
    , xenon_unknown_0x1d4(0) //
#endif
{
    m_password = nullptr;
    if (FEDatabase->IsLANMode()) {
        FEPrintf(GetPackageName(), 0x42adb44c, GetTranslatedString(0xe50258b1));
    }
    m_PlayerDisconnectMinThreshold = FEDatabase->GetOnlineSettings()->CurrPlayerDisconnectPerc;
    LobbyGameSessions::Instance().SetSessionUpdateCB(SessionUpdateCB, this);
    FEngSetScript(GetPackageName(), 0x9321399c, 0x1b20c4, true);
    FEngPrintf(GetPackageName(), 0x04059a9c, "0");
    FEngPrintf(GetPackageName(), 0x154d771c, "0");
    quickjoin = FEDatabase->GetOnlineSettings()->QuickJoinRace;
    FEDatabase->GetOnlineSettings()->QuickJoinRace = false;
    Setup();
    FEngSetInvisible(GetPackageName(), 0x444969fd);
    FEngSetInvisible(GetPackageName(), 0x3fbb7a4e);
    FEngSetInvisible(GetPackageName(), 0x688a1551);
    if (FEDatabase->IsLANMode()) {
        FEngSetInvisible(GetPackageName(), 0x1e617415);
    }
    PopulateSessionsList();
    bMemSet(&delayedDialogDetails, 0, sizeof(delayedDialogDetails));
    LobbyGameSessions::Instance().SetSortField(LobbyGameSessionsN::SORT_PING, bNameAscending);
}

UIOLLobbyRoom::~UIOLLobbyRoom() {
    LobbyCore::Instance().AbortCommand(FindSessionsCB, true);
    LobbyGameSessions::Instance().SetSessionUpdateCB(nullptr, nullptr);
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
}

void UIOLLobbyRoom::ShowCarSwitchConfirmDialog() {
    if (!GameScroller.GetActiveSelection() || bStrCmp(GameScroller.GetActiveSelection(), "") == 0) {
        return;
    }
    if (FEDatabase->GetOnlineSettings()->RankedGame == true) {
        msgHandle = DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x1a294dad, 0x417b2601, 0xffffffff,
                                                    0x59337816, 0, first_dialog_button2, GetLocalizedString(0x5ca73434));
        return;
    }
    DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x1a294dad, 0x417b2601, 0xffffffff, 0x59337816, 0,
                                    first_dialog_button2, GetLocalizedString(0x5ca73434));
}

void UIOLLobbyRoom::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0xabc08912:
            bAllowScroll = pobj->NameHash == 0x980822b3;
            break;
        case 0x00ca4ce1:
            DialogInterface::DismissDialog(msgHandle);
            msgHandle = 0;
            LobbyCore::Instance().AbortCommand(FindSessionsCB, true);
            haveRunFindSessions = true;
            if (FEDatabase->GetOnlineSettings()->GetState() == cOnlineSettings::OLS_LOBBY_REQUEST_PENDING) {
                FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_IDLE);
                return;
            }
            break;
        case 0x0b1fb0b6:
        case 0xb5af2461:
            RefreshButton();
            break;
        case 0xc519bfc4:
            switch (m_currentSortColumn) {
                case 1:
                    LobbyGameSessions::Instance().SetSortField(LobbyGameSessionsN::SORT_SESSION_NAME, bNameAscending);
                    break;
                case 2:
                    LobbyGameSessions::Instance().SetSortField(LobbyGameSessionsN::SORT_PLAYER_COUNT, bNameAscending);
                    break;
                case 3:
                    LobbyGameSessions::Instance().SetSortField(LobbyGameSessionsN::SORT_PING, bNameAscending);
                    break;
            }
            break;
        case 0xc519bfc3:
            FEDatabase->GetOnlineSettings()->ePrevPage = ePrevPage_LobbyRoom;
            cFEng::Get()->QueuePackageSwitch("UI_OLX_OptiMatch_Filters.fng", 0, 0, false);
            break;
        case 0x406415e3:
        case 0x0c407210:
            JoinButton();
            break;
        case 0x9120409e:
            m_currentSortColumn--;
            SwitchSortColumn();
            break;
        case 0xb5971bf1:
            m_currentSortColumn++;
            SwitchSortColumn();
            break;
        case 0x911ab364:
            cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
            break;
        case 0x72619778:
            if (bAllowScroll) {
                GameScroller.ScrollPrev();
                DrawSessionsList();
            }
            break;
        case 0x911c0a4b:
            if (bAllowScroll) {
                GameScroller.ScrollNext();
                DrawSessionsList();
            }
            break;
        case 0xc98356ba: {
            MaybeDismissDialog();
            MaybeShowDelayedDialog();
            if (doSwitchToGameRoomScreen == true) {
                doSwitchToGameRoomScreen = false;
                if (msgHandle) {
                    DialogInterface::DismissDialog(msgHandle);
                    msgHandle = 0;
                }
                FEManager::Get()->AllowControllerError(true);
                u32 controlMask = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(0));
                cFEng::Get()->QueuePackageSwitch("OL_GameRoom.fng", 0, controlMask, true);
                break;
            }
            if (!cFEng::Get()->IsPackageInControl("UI_OL_Disconnect.fng")) {
                MaybeFindSessions();
                MaybeDrawSessionsList();
                MaybeUpdateServerUsageStats();
                if (m_bPerformJoinSession) {
                    PerformJoinSession();
                }
            }
            break;
        }
        case 0xda5b8712: {
            m_password = FEngGetEditedString();
            JoinSession();
            bEnableEAMessenger = !FEDatabase->IsLANMode();
            break;
        }
        case 0xbd6234af:
            doBadConnectionTest = false;
        case 0x59337816:
            JoinSession();
            break;
        case 0x904aca43:
            ExitOnlineMode(GetPackageName());
            break;
        case 0x35f8620b:
            PopulateSessionsList();
            break;
        case 0x023f1bfa:
            if (numFailedJoinAttempts >= 3) {
                numFailedJoinAttempts = 0;
                haveRunFindSessions = false;
            }
            break;
    }
}

void UIOLLobbyRoom::Setup() {
    char sztemp[36];

    for (int i = 0; i < 9; i++) {
        ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;
        FEngSNPrintf(sztemp, 36, "scroll_text_%d", i + 1);
        FEString *str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pslot->AddData(str);
        str = FEngFindString(GetPackageName(), 0x116d2d5c);
        pslot->AddData(str);
        FEngSNPrintf(sztemp, 36, "scroll_racers_%d", i + 1);
        str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pslot->AddData(str);
        FEngSNPrintf(sztemp, 36, "High_Light_%d", i + 1);
        pslot->SetBacking(FEngFindObject(GetPackageName(), FEHashUpper(sztemp)));
        GameScroller.AddSlot(pslot);
    }

    ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;
    pslot->AddData(FEngFindString(GetPackageName(), 0x980822b3));
    TitleScroller.AddSlot(pslot);

    ScrollerDatum *datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0x063d57a0), 0);
    TitleScroller.AddData(datum);
    TitleScroller.ScrollPrev();
    TitleScroller.Update(true);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0x7ad18f04), 0);
    TitleScroller.AddData(datum);
    TitleScroller.ScrollPrev();
    TitleScroller.Update(true);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0x39751bdf), 0);
    TitleScroller.AddData(datum);
    TitleScroller.ScrollPrev();
    TitleScroller.Update(true);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0x813251f6), 0);
    TitleScroller.AddData(datum);
    TitleScroller.ScrollPrev();
    TitleScroller.Update(true);

    PopulateSessionsList();
}

void UIOLLobbyRoom::PopulateSessionsList() {
    char lastSelectedSession[64] = "";

    const char *tmp = GameScroller.GetActiveSelection();
    if (tmp) {
        bStrCpy(lastSelectedSession, tmp);
        GameScroller.UnHighlightSelected();
    }
    GameScroller.DeleteScrollerData();

    LobbyGameSessions &lgs = LobbyGameSessions::Instance();
    int sessionCount = lgs.GetNumSessions();

    for (int i = 0; i < sessionCount; i++) {
        GameSession *session = lgs.GetSessionByIndex(i);
        lgs.GetExtraSessionDataByIdent(session->iIdent);

        LobbyScrollerDatum *datum = new ("LobbyScrollerDatum", 0) LobbyScrollerDatum;
        datum->AddData(lgs.GetSessionDisplayName(session), 0);
        if (session->uCustFlags & 0x80000000) {
            datum->AddData(GetLocalizedString(0x034fa2c1), 0);
        } else if (session->uCustFlags & 0x40000000) {
            datum->AddData(GetLocalizedString(0xb94fd70e), 0);
        } else if (session->uCustFlags & 0x20000000) {
            datum->AddData(GetLocalizedString(0x6f547e4c), 0);
        }

        char temp[8] = "";
        FEngSNPrintf(temp, 8, "%$d/%$d", session->iCount, session->iSize);
        datum->AddData(temp, 0);
        datum->sessionIdent = session->iIdent;
        bStrCpy(datum->sessionName, session->strName);
        GameScroller.AddData(datum);
        GameScroller.Enable(datum);
    }

    for (int i = sessionCount; i < 9; i++) {
        LobbyScrollerDatum *datum = new ("ScrollerDatum", 0) LobbyScrollerDatum;
        datum->AddData("", 0);
        datum->AddData("", 0);
        datum->AddData("", 0);
        TurnOffPingCircles(i + 1);
        GameScroller.AddData(datum);
        GameScroller.Disable(datum);
    }

    if (lastSelectedSession[0] == '\0' || !GameScroller.FindDatum(lastSelectedSession)) {
        if (sessionCount) {
            GameScroller.SetSelectedDatum(0u);
            GameScroller.HighlightSelected();
        }
    } else {
        GameScroller.SetSelectedDatum(GameScroller.FindDatum(lastSelectedSession));
        GameScroller.HighlightSelected();
    }
    DrawSessionsList();
}

void UIOLLobbyRoom::DrawSessionsList() {
    LobbyScrollerDatum *datum = static_cast<LobbyScrollerDatum *>(GameScroller.GetTopDatum());

    for (int i = 1; i < 10; i++) {
        ExtraSessionData *esd = LobbyGameSessions::Instance().GetExtraSessionDataByIdent(datum->sessionIdent);
        LobbyGameSessions::Instance().GetSessionByName(datum->sessionName);
        if (!esd) {
            TurnOffPingCircles(i);
        } else if (esd->pingToHostInMsec < 0) {
            SetConnectionQualityUnknown(i);
        } else if (esd->pingToHostInMsec < 76) {
            SetConnectionQualityGood(i);
        } else if (esd->pingToHostInMsec < 176) {
            SetConnectionQualityAverage(i);
        } else {
            SetConnectionQualityBad(i);
        }
        datum = static_cast<LobbyScrollerDatum *>(datum->GetNext());
    }
    GameScroller.Update(true);
    DisplaySingleSessionInformation();
}

void UIOLLobbyRoom::DisplaySingleSessionInformation() {
    GameSession *session;
    uint32 EventHash;
    uint8 NumLaps;
    GRaceParameters *parms;
    uint32 hash;
    uint32 texhash;
    const char *distUnits;

    if (!GameScroller.GetActiveSelection() || bStrCmp(GameScroller.GetActiveSelection(), "") == 0) {
        return;
    }
    LobbyScrollerDatum *datum = static_cast<LobbyScrollerDatum *>(GameScroller.FindDatum(GameScroller.GetActiveSelection()));
    session = LobbyGameSessions::Instance().GetSessionByName(datum->sessionName);
    if (!session) {
        return;
    }
    EventHash = TagFieldGetNumber(TagFieldFind(session->strParams, "E"), 0);
    NumLaps = TagFieldGetNumber(TagFieldFind(session->strParams, "L"), 0);
    parms = GRaceDatabase::Get().GetRaceFromHash(EventHash);
    if (!parms) {
        return;
    }
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
    FEngSetTextureHash(GetPackageName(), 0xceb82625, texhash);
    FEngSetLanguageHash(GetPackageName(), 0x116d2d5c, hash);

    hash = CalcLanguageHash("TRACKNAME_", parms);
    if (DoesStringExist(hash)) {
        FEngSetLanguageHash(GetPackageName(), 0x4b4e3888, hash);
    } else {
        FEPrintf(GetPackageName(), 0x4b4e3888, parms->GetEventID());
    }

    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        distUnits = GetLocalizedString(0x8569a26a);
    } else {
        distUnits = GetLocalizedString(0x867dcfd9);
    }
    FEPrintf(GetPackageName(), 0x4b4e3889, "%0.1f %s", parms->GetRaceLengthMeters() / 1000.0f, distUnits);
    FEPrintf(GetPackageName(), 0x4b4e388a, "%d", NumLaps);

    if (session->uCustFlags & 0x1000) {
        FEPrintf(GetPackageName(), 0x4b4e388b, GetLocalizedString(0xa1cd823e));
    } else {
        FEPrintf(GetPackageName(), 0x4b4e388b, GetLocalizedString(0xde6eff34));
    }
    if (session->uCustFlags & 0x800) {
        FEPrintf(GetPackageName(), 0x4b4e388c, GetLocalizedString(0x70e01038));
    } else {
        FEPrintf(GetPackageName(), 0x4b4e388c, GetLocalizedString(0x417b25e4));
    }
    if (session->uCustFlags & 0x200) {
        FEPrintf(GetPackageName(), 0x4b4e388d, GetLocalizedString(0x70e01038));
    } else {
        FEPrintf(GetPackageName(), 0x4b4e388d, GetLocalizedString(0x417b25e4));
    }
    if (session->uCustFlags & 0x2000) {
        FEPrintf(GetPackageName(), 0x4b4e388e, GetLocalizedString(0x417b2604));
    } else {
        FEPrintf(GetPackageName(), 0x4b4e388e, GetLocalizedString(0x70dfe5c2));
    }
    if (session->uCustFlags & 0x400) {
        FEPrintf(GetPackageName(), 0x4b4e388f, GetLocalizedString(0x70e01038));
    } else {
        FEPrintf(GetPackageName(), 0x4b4e388f, GetLocalizedString(0x417b25e4));
    }
}

void UIOLLobbyRoom::MaybeDrawSessionsList() {
    static Timer gamesListUpdateTimer(RealTimer.GetPackedTime());

    if (redrawSessionsList == true && (RealTimer - gamesListUpdateTimer).GetSeconds() > 0.75f) {
        gamesListUpdateTimer.SetPackedTime(RealTimer.GetPackedTime());
        redrawSessionsList = false;
        PopulateSessionsList();
    }
}

void UIOLLobbyRoom::JoinSession() {
    if (!GameScroller.GetActiveSelection() || bStrCmp(GameScroller.GetActiveSelection(), "") == 0) {
        return;
    }
    FEDatabase->GetOnlineSettings()->GameDetails = eJOINING;
    LobbyScrollerDatum *datum = static_cast<LobbyScrollerDatum *>(GameScroller.FindDatum(GameScroller.GetActiveSelection()));
    GameSession *session = LobbyGameSessions::Instance().GetSessionByName(datum->sessionName);
    if (!session) {
        return;
    }
    if (doBadConnectionTest == true) {
        ExtraSessionData *esd = LobbyGameSessions::Instance().GetExtraSessionDataByIdent(session->iIdent);
        if (esd) {
            if (esd->pingToHostInMsec == -1 || esd->pingToHostInMsec == -2) {
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 0xbd6234af,
                                                0xffffffff, 0xffffffff, first_dialog_button1,
                                                GetLocalizedString(0x243f8e1e));
                return;
            } else if (esd->pingToHostInMsec >= 176) {
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 0xbd6234af,
                                                0xffffffff, 0xffffffff, first_dialog_button1,
                                                GetLocalizedString(0xa492ae15));
                return;
            }
        }
    }
    SwitchToOnlineCareerCar();
    if ((session->uCustFlags & 0x2) && !m_password) {
        FEngBeginTextInput(0x573689f6, 5, "", GetLocalizedString(0x332c0b6f), 16);
        return;
    } else {
        cOnlineSettings &onlineSettings = *FEDatabase->GetOnlineSettings();
        bStrCpy(onlineSettings.Password, m_password ? m_password : "");
        bStrCpy(onlineSettings.VerifyPassword, m_password ? m_password : "");
        onlineSettings.Passworded = m_password != nullptr;
        if (LobbyGameSessions::Instance().JoinSession(session->strName, m_password, JoinSessionCB, this) < 0) {
            DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0xe3a9ab2a));
            return;
        } else {
            FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_REQUEST_PENDING);
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }
            msgHandle = DialogInterface::ShowMessage(GetPackageName(), "", dialog_alert, GetLocalizedString(0xd48fba40),
                                                     LobbyGameSessions::Instance().GetSessionDisplayName(session));
        }
    }
}

void UIOLLobbyRoom::SwitchToOnlineCareerCar() {}

int32 UIOLLobbyRoom::FindSessions() {
    if (FEDatabase->GetOnlineSettings()->GetState() == cOnlineSettings::OLS_LOBBY_IDLE) {
        CarViewer::GetRideInfo(eCARVIEWER_PLAYER1_CAR);
        if (LobbyGameSessions::Instance().FindSessions(OnlineSessionFilter, FindSessionsCB, this) >= 0) {
            FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_REQUEST_PENDING);
#ifdef EA_PLATFORM_XENON
            xenon_unknown_0x198 = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x1a294dad, 0x00ca4ce1,
                                                                 GetLocalizedString(0x65fda335));
#else
            msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x1a294dad, 0x00ca4ce1,
                                                       GetLocalizedString(0x65fda335));
#endif
            return 0;
        }
    }
    return -1;
}

static int LobbyConnectionUnknownHash[9] = {
    0x61726c50, 0x61726c51, 0x61726c52, 0x61726c53, 0x61726c54, 0x61726c55, 0x61726c56, 0x61726c57, 0x61726c58,
};

void UIOLLobbyRoom::SetConnectionQualityGood(int row) {
    char temp[32] = "";

    for (int col = 1; col <= 3; col++) {
        FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", row, col);
        FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
        if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, 0xe8ee5b38)) {
            FEngSetScript(GetPackageName(), obj->NameHash, 0xe8ee5b38, true);
        }
    }
    if (!FEngIsScriptRunning(GetPackageName(), LobbyConnectionUnknownHash[row - 1], 0x16a259)) {
        FEngSetScript(GetPackageName(), LobbyConnectionUnknownHash[row - 1], 0x16a259, true);
    }
}

void UIOLLobbyRoom::SetConnectionQualityAverage(int row) {
    char temp[32] = "";

    for (int col = 1; col <= 2; col++) {
        FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", row, col);
        FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
        if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, 0x76d72a2d)) {
            FEngSetScript(GetPackageName(), obj->NameHash, 0x76d72a2d, true);
        }
    }
    FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", row, 3);
    FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
    if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, 0xccfa)) {
        FEngSetScript(GetPackageName(), obj->NameHash, 0xccfa, true);
    }
    if (!FEngIsScriptRunning(GetPackageName(), LobbyConnectionUnknownHash[row - 1], 0x16a259)) {
        FEngSetScript(GetPackageName(), LobbyConnectionUnknownHash[row - 1], 0x16a259, true);
    }
}

void UIOLLobbyRoom::SetConnectionQualityBad(int row) {
    char temp[32] = "";

    FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", row, 1);
    FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
    if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, 0x2d02fcd6)) {
        FEngSetScript(GetPackageName(), obj->NameHash, 0x2d02fcd6, true);
    }
    for (int col = 2; col <= 3; col++) {
        FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", row, col);
        obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
        if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, 0xccfa)) {
            FEngSetScript(GetPackageName(), obj->NameHash, 0xccfa, true);
        }
    }
    if (!FEngIsScriptRunning(GetPackageName(), LobbyConnectionUnknownHash[row - 1], 0x16a259)) {
        FEngSetScript(GetPackageName(), LobbyConnectionUnknownHash[row - 1], 0x16a259, true);
    }
}

void UIOLLobbyRoom::SetConnectionQualityUnknown(int row) {
    char temp[32] = "";

    for (int col = 1; col <= 3; col++) {
        FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", row, col);
        FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
        if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, 0x5e40072f)) {
            FEngSetScript(GetPackageName(), obj->NameHash, 0x5e40072f, true);
        }
    }
    if (!FEngIsScriptRunning(GetPackageName(), LobbyConnectionUnknownHash[row - 1], 0x1ca7c0)) {
        FEngSetScript(GetPackageName(), LobbyConnectionUnknownHash[row - 1], 0x1ca7c0, true);
    }
}

void UIOLLobbyRoom::TurnOffPingCircles(int row) {
    char temp[32] = "";

    for (int col = 1; col <= 3; col++) {
        FEngSNPrintf(temp, 32, "Ping_Circle_%d_%d", row, col);
        FEObject *obj = FEngFindObject(GetPackageName(), FEHashUpper(temp));
        if (!FEngIsScriptRunning(GetPackageName(), obj->NameHash, 0xccfa)) {
            FEngSetScript(GetPackageName(), obj->NameHash, 0xccfa, true);
        }
    }
    if (!FEngIsScriptRunning(GetPackageName(), LobbyConnectionUnknownHash[row - 1], 0x16a259)) {
        FEngSetScript(GetPackageName(), LobbyConnectionUnknownHash[row - 1], 0x16a259, true);
    }
}

void UIOLLobbyRoom::MaybeFindSessions() {
    if (!haveRunFindSessions && FEDatabase->GetOnlineSettings()->GetState() == cOnlineSettings::OLS_LOBBY_IDLE) {
        if (FindSessions() < 0) {
            haveRunFindSessions = true;
            msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364,
                                                       GetLocalizedString(0x8173fc6b));
        }
    }
}

void UIOLLobbyRoom::MaybeUpdateServerUsageStats() {
    static Timer statsUpdateTimer(RealTimer.GetPackedTime());

    if ((RealTimer - statsUpdateTimer).GetSeconds() > 5.0f) {
        LobbyApiServerStatT *stats = LobbyCore::Instance().GetServerStats();
        if (stats) {
            FEngPrintf(GetPackageName(), 0x04059a9c, "%$d", stats->uUsersInLobby + stats->uUsersInGames);
            FEngPrintf(GetPackageName(), 0x154d771c, "%$d", stats->uGamesInProgress + stats->uGamesCreated + stats->uGamesCompleted);
        }
        statsUpdateTimer.SetPackedTime(RealTimer.GetPackedTime());
    }
}

void UIOLLobbyRoom::MaybeDismissDialog() {
    if (doDismissDialog == true && LobbyGameSessions::Instance().FoundAllSessions()) {
        doDismissDialog = false;
        if (msgHandle) {
            DialogInterface::DismissDialog(msgHandle);
            msgHandle = 0;
        }
    }
}

void UIOLLobbyRoom::SessionUpdateCB(LobbyGameSessionsN::SessionStatusCode code, const LobbyApiUserSetT *session, void *context) {
    UIOLLobbyRoom *lobbyRoom = static_cast<UIOLLobbyRoom *>(context);

    switch (code) {
        case LobbyGameSessionsN::SESSION_DELETED:
            break;
        case LobbyGameSessionsN::SESSION_CHANGED:
            break;
        case LobbyGameSessionsN::SESSION_LIST_CHANGED:
            lobbyRoom->redrawSessionsList = true;
            break;
        case LobbyGameSessionsN::SESSION_KICKED:
            break;
        case LobbyGameSessionsN::GAME_STARTED:
            break;
    }
}

void UIOLLobbyRoom::JoinSessionCB(LobbyApiMsgT *msg, void *context) {
    static_cast<UIOLLobbyRoom *>(context)->m_LobbyApiMsgT_code = msg->code;
    static_cast<UIOLLobbyRoom *>(context)->m_bPerformJoinSession = true;
}

void UIOLLobbyRoom::PerformJoinSession() {
    m_bPerformJoinSession = false;
    doDismissDialog = true;
    if (m_LobbyApiMsgT_code == 0) {
        doSwitchToGameRoomScreen = true;
    } else if (m_LobbyApiMsgT_code == 'ugam') {
        SetDelayedDialog(0x417b2601, 0x023f1bfa, 0x023f1bfa, GetLocalizedString(0x331f4f58));
        numFailedJoinAttempts++;
    } else if (m_LobbyApiMsgT_code == 'full') {
        SetDelayedDialog(0x417b2601, 0x023f1bfa, 0x023f1bfa, GetLocalizedString(0xd155595a));
        numFailedJoinAttempts++;
    } else if (m_LobbyApiMsgT_code == 'pass') {
        m_password = nullptr;
        SetDelayedDialog(0x417b2601, 0xffffffff, 0xffffffff, GetLocalizedString(0xb5eaa702));
    } else if (m_LobbyApiMsgT_code == 'asta') {
        SetDelayedDialog(0x417b2601, 0x023f1bfa, 0x023f1bfa, GetLocalizedString(0x1c098ddf));
        numFailedJoinAttempts++;
    } else if (m_LobbyApiMsgT_code == 'invp') {
        SetDelayedDialog(0x417b2601, 0xffffffff, 0xffffffff, GetLocalizedString(0x4ad41e75));
    } else {
        SetDelayedDialog(0x417b2601, 0xffffffff, 0xffffffff, GetLocalizedString(0x4b5c9721));
    }
    FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_IDLE);
}

void UIOLLobbyRoom::FindSessionsCB(LobbyApiMsgT *msg, void *context) {
    static_cast<UIOLLobbyRoom *>(context)->doDismissDialog = true;
    static_cast<UIOLLobbyRoom *>(context)->haveRunFindSessions = true;
    if (msg->code) {
        static_cast<UIOLLobbyRoom *>(context)->SetDelayedDialog(0x417b2601, 0xffffffff, 0xffffffff, GetLocalizedString(0xef94affd));
    }
    FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_IDLE);
}

void UIOLLobbyRoom::JoinButton() {
    if (!GameScroller.GetActiveSelection()) {
        return;
    }
    LobbyScrollerDatum *datum = static_cast<LobbyScrollerDatum *>(GameScroller.FindDatum(GameScroller.GetActiveSelection()));
    if (datum->sessionIdent < 0) {
        return;
    }
    GameSession *session = LobbyGameSessions::Instance().GetSessionByName(datum->sessionName);
    if (session->uCustFlags & 0x200) {
        FEDatabase->GetOnlineSettings()->RankedGame = true;
        ShowCarSwitchConfirmDialog();
    } else {
        FEDatabase->GetOnlineSettings()->RankedGame = false;
        m_password = nullptr;
        doBadConnectionTest = FEDatabase->GetOnlineSettings()->ShowBadConnectionWarning;
        ShowCarSwitchConfirmDialog();
    }
}

void UIOLLobbyRoom::RefreshButton() {
    if (FindSessions() < 0) {
        if (msgHandle) {
            DialogInterface::DismissDialog(msgHandle);
        }
        msgHandle = 0;
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                       GetLocalizedString(0x8173fc6b));
    }
}

void UIOLLobbyRoom::SetDelayedDialog(uint32 titleHash, uint32 pressMessage, uint32 cancelMessage, const char *text) {
    delayedDialogDetails.dialogTitle = titleHash;
    delayedDialogDetails.buttonPressedMessage = pressMessage;
    delayedDialogDetails.buttonCancelMessage = cancelMessage;
    delayedDialogDetails.dialogText = text;
    delayedDialogDetails.showDialog = true;
}

void UIOLLobbyRoom::MaybeShowDelayedDialog() {
    if (delayedDialogDetails.showDialog == true) {
        delayedDialogDetails.showDialog = false;
        if (msgHandle) {
            DialogInterface::DismissDialog(msgHandle);
            msgHandle = 0;
        }
        msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, delayedDialogDetails.dialogTitle,
                                                   delayedDialogDetails.buttonPressedMessage,
                                                   delayedDialogDetails.buttonCancelMessage, delayedDialogDetails.dialogText);
    }
}

void UIOLLobbyRoom::SwitchSortColumn() {
    char scriptName[8];

    if (m_currentSortColumn < 1) {
        m_currentSortColumn = 1;
    }
    if (m_currentSortColumn > 3) {
        m_currentSortColumn = 3;
    }
    FEngSNPrintf(scriptName, 8, "POS%d", m_currentSortColumn);
    FEngSetScript(GetPackageName(), 0x9321399c, FEHashUpper(scriptName), true);
}
