#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineQuickJoin.hpp"

#include "Speed/Indep/Src/Frontend/Careers/UnlockSystem.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

template <class T, void (T::*Callback)(LobbyApiMsgT *)> struct lobby_callback_functor {
    static void invoke(LobbyApiMsgT *msg, void *context) {
        (static_cast<T *>(context)->*Callback)(msg);
    }
};

YesNoAny g_RankedGame = YES;
bool UIOnlineGameSessionSearch::m_bConnectionStatusOK = false;

// Definido en uiOnlineFilters.cpp, que la unidad zFe incluye despues de este fichero.
extern FilterGameSessionParamsT OnlineSessionFilter;

MenuScreen *CreateOnlineGameSessionSearch(ScreenConstructorData *sd, const eOnlineConnectingType &eSearchSection) {
    return new ("UIOnlineGameSessionSearch", 0) UIOnlineGameSessionSearch(sd, eSearchSection);
}

eOnlineDisconnectPerc GetDiconnectPercentageThreshold(const int32 &nDisconnectPercentage) {
    if (nDisconnectPercentage >= 0 && nDisconnectPercentage < 5) {
        return OLS_DISCONNECT_PERC_5;
    }
    if (nDisconnectPercentage >= 5 && nDisconnectPercentage < 10) {
        return OLS_DISCONNECT_PERC_10;
    }
    if (nDisconnectPercentage >= 10 && nDisconnectPercentage < 15) {
        return OLS_DISCONNECT_PERC_15;
    }
    if (nDisconnectPercentage >= 15 && nDisconnectPercentage < 20) {
        return OLS_DISCONNECT_PERC_20;
    }
    if (nDisconnectPercentage >= 20 && nDisconnectPercentage < 25) {
        return OLS_DISCONNECT_PERC_25;
    }
    if (nDisconnectPercentage >= 25 && nDisconnectPercentage < 50) {
        return OLS_DISCONNECT_PERC_50;
    }
    return OLS_DISCONNECT_PERC_ANY;
}

UIOnlineGameSessionSearch::UIOnlineGameSessionSearch(ScreenConstructorData *sd, const eOnlineConnectingType &eSearchSection)
    : MenuScreen(sd) //
    , dialogactive(false) //
    , sessionstarted(false) //
    , searchactive(false) //
    , searcherror(false) //
    , m_bWaitingToExit(false) //
    , m_bWaitingForJoinResult(false) //
    , m_bJoinSessionOK(false) //
    , m_numSessions(0) //
    , m_dialogHandle(0) //
    , m_eSearchSection(eSearchSection) //
    , m_nDiscPercCBRetCode(0) {
    bEnableEAMessenger = false;

    ConnectionCore::Instance().SetCallback(ConnectionCoreCB, nullptr);

    if (m_eSearchSection == eOnlineConnecting_QuickMatchSearch) {
        FEDatabase->GetOnlineSettings()->QuickJoinRace = true;
    } else {
        FEDatabase->GetOnlineSettings()->QuickJoinRace = false;
    }

    FEPrintf(GetPackageName(), 0x1ab2b613, GetLocalizedString(0xde5ff65f));

    startsearch();
}

UIOnlineGameSessionSearch::~UIOnlineGameSessionSearch() {
    if (FEDatabase->GetOnlineSettings()->GetState() >= cOnlineSettings::OLS_LOBBY_LOGGED_IN) {
        LobbyGameSessions::Instance().SetSessionUpdateCB(nullptr, nullptr);
        ConnectionCore::Instance().SetCallback(nullptr, nullptr);
        LobbyCore::Instance().AbortCommand(FindSessionsCB, true);
        LobbyCore::Instance().AbortAllCommands();
        FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_IDLE);
    }
}

void UIOnlineGameSessionSearch::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (msg == 0x87f32b2a || msg == 0x406415e3 || msg == 0x911ab364) {
        if (m_bWaitingForJoinResult) {
            m_bWaitingToExit = true;
            m_bWaitingForJoinResult = false;
            return;
        }
        cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
        return;
    }

    if (msg == 0xc98356ba) {
        if (m_bWaitingToExit) {
            if (!m_bWaitingForJoinResult) {
                cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
            }
        } else {
            if (!searcherror) {
                if (dialogactive) {
                    return;
                }

                if (m_eSearchSection == eOnlineConnecting_QuickMatchSearch) {
                    if (sessionstarted && m_bJoinSessionOK && m_numSessions > 0) {
                        if (m_bConnectionStatusOK) {
                            m_bConnectionStatusOK = false;
                            FEManager::Get()->AllowControllerError(true);
                            u32 controlMask = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(0));
                            cFEng::Get()->QueuePackageSwitch("OL_GameRoom.fng", 0, controlMask, true);
                        }
                        return;
                    }
                } else if (m_numSessions > 0) {
                    cFEng::Get()->QueuePackageSwitch("OL_LobbyRoom.fng", 0, 0, false);
                    return;
                }
            }

            if (searchtimeout < RealTimer && !m_bJoinSessionOK) {
                if ((searchactive || searcherror) && !m_numSessions) {
                    if (!m_dialogHandle) {
                        m_dialogHandle = DialogInterface::ShowThreeButtons(
                            GetPackageName(), "3buttons_stacked", dialog_confirmation, 0x5e112210, 0x96d7d398, 0x1a294dad,
                            GS_SEARCH_AGAIN, GS_CREATE_GAME, GS_CANCEL, GS_CANCEL, first_dialog_button2, 0xe7c1621a);
                    }
                }
            }
        }
    }

    if (msg == GS_CANCEL) {
        if (m_dialogHandle) {
            DialogInterface::DismissDialog(m_dialogHandle);
        }
        m_dialogHandle = 0;
        searchactive = false;
        searcherror = false;
        cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
    }

    if (msg == GS_CREATE_GAME) {
        if (m_dialogHandle) {
            DialogInterface::DismissDialog(m_dialogHandle);
        }
        m_dialogHandle = 0;
        searchactive = false;
        searcherror = false;
        createsession();
    }

    if (msg == GS_SEARCH_AGAIN) {
        if (m_dialogHandle) {
            DialogInterface::DismissDialog(m_dialogHandle);
        }
        m_dialogHandle = 0;
        searchactive = false;
        searcherror = false;

        if (m_eSearchSection == eOnlineConnecting_QuickMatchSearch) {
            startsearch();
        } else {
            cFEng::Get()->QueuePackageSwitch("UI_OLX_OptiMatch_Filters.fng", 0, 0, false);
        }
    }

    if (msg == 0x4ebed0a1) {
        if (m_dialogHandle) {
            DialogInterface::DismissDialog(m_dialogHandle);
            m_dialogHandle = 0;
        }

        if (m_nDiscPercCBRetCode) {
            m_dialogHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, GS_CANCEL, GS_CANCEL,
                                                            GetLocalizedString(0xf77b0d99));
        } else {
            startsearch();
        }
    }
}

void UIOnlineGameSessionSearch::UpdateCallback(LobbyGameSessionsN::SessionStatusCode code, const LobbyApiUserSetT *session) {
    if (code == LobbyGameSessionsN::SESSION_LIST_CHANGED) {
        checksessions();
    }
}

void UIOnlineGameSessionSearch::JoinSessionCallback(LobbyApiMsgT *msg) {
    m_bWaitingForJoinResult = false;

    if (msg->code == 0) {
        if (m_bWaitingToExit) {
            LobbyGameSessions::Instance().LeaveSession(nullptr, nullptr);
        } else {
            m_bJoinSessionOK = true;
            FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_IDLE);
        }
    } else if (!m_bWaitingToExit) {
        sessionstarted = false;
        searcherror = true;
    }
}

typedef lobby_callback_functor<UIOnlineGameSessionSearch, &UIOnlineGameSessionSearch::JoinSessionCallback> call_back;

void UIOnlineGameSessionSearch::joinsession(const LobbyApiUserSetT *session) {
    FEDatabase->GetOnlineSettings()->GameDetails = eJOINING;

    if (LobbyGameSessions::Instance().JoinSession(session->strName, "", call_back::invoke, this) < 0) {
        dialogactive = true;
    } else {
        sessionstarted = true;
        m_bWaitingForJoinResult = true;
        FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_REQUEST_PENDING);
    }
}

void UIOnlineGameSessionSearch::checksessions() {
    int bestsession;
    int sessioncount;

    if (sessionstarted || dialogactive || !searchactive) {
        return;
    }

    m_numSessions = LobbyGameSessions::Instance().GetNumSessions();

    if (m_eSearchSection != eOnlineConnecting_QuickMatchSearch) {
        return;
    }

    bestsession = -1;
    sessioncount = 0;

    for (int i = 0; i < m_numSessions; i++) {
        GameSession *session = LobbyGameSessions::Instance().GetSessionByIndex(i);
        if (session->iCount < session->iSize) {
            sessioncount++;
            if (bestsession < 0 || bRandom(sessioncount) == 0) {
                bestsession = i;
            }
        }
    }

    if (bestsession >= 0) {
        joinsession(LobbyGameSessions::Instance().GetSessionByIndex(bestsession));
    }
}

int32 UIOnlineGameSessionSearch::GetDisconnectPercentage(int32 *pDisconnectPercentage) {
    PlayerDataT MyStats;
    int32 rc = LobbyUsers::Instance().GetMyUserStats(MyStats, GetExtendedStatsCB, this);

    if (rc != 0) {
        if (rc < 0) {
            m_dialogHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, GS_CANCEL, GS_CANCEL,
                                                            GetLocalizedString(0xf77b0d99));
        }
        return rc;
    }

    if (MyStats.GetTotalGamesPlayed(3) < 20) {
        *pDisconnectPercentage = 0;
    } else {
        *pDisconnectPercentage = static_cast<int32>(MyStats.GetTotalDisconnectPercentage());
    }
    return 0;
}

void UIOnlineGameSessionSearch::GetExtendedStatsCB(LobbyApiMsgT *msg, void *context) {
    UIOnlineGameSessionSearch *pclSessionSearch = static_cast<UIOnlineGameSessionSearch *>(context);

    pclSessionSearch->m_nDiscPercCBRetCode = msg->code;
    cFEng::Get()->QueueGameMessage(0x4ebed0a1, pclSessionSearch->GetPackageName(), 0xff);
}

void UIOnlineGameSessionSearch::FindSessionsCB(LobbyApiMsgT *msg, void *context) {
    UIOnlineGameSessionSearch *pclSessionSearch = static_cast<UIOnlineGameSessionSearch *>(context);

    if (msg->code == 0) {
        pclSessionSearch->searchactive = true;
        pclSessionSearch->searcherror = false;
        FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_REQUEST_PENDING);
    } else {
        pclSessionSearch->searcherror = true;
    }
}

void UIOnlineGameSessionSearch::startsearch() {
    int32 nDisconnectPercentage;
    int32 rc;

    searchtimeout = RealTimer + Timer(search_timeout());
    nDisconnectPercentage = 0;

    if (!FEDatabase->IsLANMode()) {
        if (GetDisconnectPercentage(&nDisconnectPercentage)) {
            return;
        }
    }

    LobbyGameSessions::Instance().SetSessionUpdateCB(nullptr, nullptr);
    LobbyGameSessions::Instance().SetSessionUpdateCB(FindMatchUpdateCB, this);

    if (!FEDatabase->IsLANMode()) {
        FEDatabase->GetOnlineSettings()->CurrPlayerDisconnectPerc = GetDiconnectPercentageThreshold(nDisconnectPercentage);
    } else {
        FEDatabase->GetOnlineSettings()->CurrPlayerDisconnectPerc = OLS_DISCONNECT_PERC_ANY;
    }

    if (m_eSearchSection == eOnlineConnecting_QuickMatchSearch) {
        FilterGameSessionParamsT filterParams(100, FEDatabase->GetOnlineSettings()->RaceMode, g_RankedGame, ON, ON,
                                              FEDatabase->GetOnlineSettings()->CurrPlayerDisconnectPerc);

        rc = LobbyGameSessions::Instance().FindSessions(filterParams, FindSessionsCB, this);
    } else {
        OnlineSessionFilter.disconnectPerc = FEDatabase->GetOnlineSettings()->CurrPlayerDisconnectPerc;

        rc = LobbyGameSessions::Instance().FindSessions(OnlineSessionFilter, FindSessionsCB, this);
    }

    if (rc < 0) {
        searcherror = true;
    } else {
        searchactive = true;
        FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_REQUEST_PENDING);
    }
}

uint32 UIOnlineGameSessionSearch::pick_random_event(GRace::Type &gamemode) {
    GRaceDatabase &db = GRaceDatabase::Get();
    uint32 racecount = db.GetRaceCount();
    GRaceParameters **matches = new GRaceParameters *[racecount];
    int matchcount = 0;
    uint32 eventhash;

    for (uint32 i = 0; i < racecount; i++) {
        GRaceParameters *param = db.GetRaceParameters(i);

        if (param->GetIsAvailable(GRace::kRaceContext_Online)) {
            if (UnlockSystem::IsTrackUnlocked(UNLOCK_ONLINE, param->GetEventHash(), 0)) {
                if (gamemode == GRace::kRaceType_NumTypes) {
                    if (param->GetRaceType() == GRace::kRaceType_Circuit || param->GetRaceType() == GRace::kRaceType_P2P ||
                        param->GetRaceType() == GRace::kRaceType_Drag) {
                        matches[matchcount++] = param;
                    }
                } else if (gamemode == param->GetRaceType()) {
                    matches[matchcount++] = param;
                }
            }
        }
    }

    int index = bRandom(matchcount);
    eventhash = matches[index]->GetEventHash();
    gamemode = matches[index]->GetRaceType();

    delete[] matches;

    return eventhash;
}

void UIOnlineGameSessionSearch::createsession() {
    cOnlineSettings *onlineSettings = FEDatabase->GetOnlineSettings();
    GRace::Type gamemode;

    sessionstarted = true;

    if (m_eSearchSection == eOnlineConnecting_QuickMatchSearch) {
        gamemode = onlineSettings->RaceMode;

        switch (g_RankedGame) {
            case NO:
                onlineSettings->RankedGame = false;
                break;
            case YES:
                onlineSettings->RankedGame = true;
                break;
            default:
                onlineSettings->RankedGame = true;
                break;
        }

        onlineSettings->CollisionDetection = true;
        onlineSettings->PerformanceMatching = true;
    } else {
        gamemode = OnlineSessionFilter.gameMode;

        switch (OnlineSessionFilter.rankedGames) {
            case NO:
                onlineSettings->RankedGame = false;
                break;
            case YES:
                onlineSettings->RankedGame = true;
                break;
            default:
                onlineSettings->RankedGame = true;
                break;
        }

        if (OnlineSessionFilter.collisionDetection == ON) {
            onlineSettings->CollisionDetection = true;
        } else if (OnlineSessionFilter.collisionDetection == OFF) {
            onlineSettings->CollisionDetection = false;
        } else {
            onlineSettings->CollisionDetection = true;
        }

        if (OnlineSessionFilter.performanceMatching == ON) {
            onlineSettings->PerformanceMatching = true;
        } else if (OnlineSessionFilter.performanceMatching == OFF) {
            onlineSettings->PerformanceMatching = false;
        } else {
            onlineSettings->PerformanceMatching = true;
        }
    }

    if (gamemode == GRace::kRaceType_Circuit) {
        onlineSettings->GetRaceSettings()->NumLaps = 3;
    } else {
        onlineSettings->GetRaceSettings()->NumLaps = 1;
    }
    onlineSettings->GetRaceSettings()->TrackDirection = 0;
    onlineSettings->GetRaceSettings()->EventHash = pick_random_event(gamemode);

    onlineSettings->RaceMode = gamemode;
    onlineSettings->UseNOS = true;
    onlineSettings->AnyCarClassAllowed = true;
    onlineSettings->MinOnlinePlayers = 2;
    onlineSettings->Passworded = false;
    onlineSettings->IsPrivateRoom = false;

    if (onlineSettings->CurrPlayerDisconnectPerc > OLS_DISCONNECT_PERC_50) {
        onlineSettings->DisconnectPerc = OLS_DISCONNECT_PERC_ANY;
    } else {
        onlineSettings->DisconnectPerc = OLS_DISCONNECT_PERC_50;
    }

    onlineSettings->GameDetails = eHOSTING;
    FEManager::Get()->AllowControllerError(true);
    u32 controlMask = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(0));
    cFEng::Get()->QueuePackageSwitch("OL_GameRoom.fng", 0, controlMask, true);
}

void UIOnlineGameSessionSearch::ConnectionCoreCB(ConnApiRefT *connapi, ConnApiCbInfoT *cbinfo, void *context) {
    if (cbinfo->eNewStatus == CONNAPI_STATUS_MNGL || cbinfo->eNewStatus == CONNAPI_STATUS_ACTV) {
        m_bConnectionStatusOK = true;
    }

    if (cbinfo->eNewStatus == CONNAPI_STATUS_DISC) {
        ConnectionCore::Instance().GetPlayer(cbinfo->iClientId);
    } else {
        ConnectionCore::Instance().GetPlayer(cbinfo->iClientId);
    }
}

void FindMatchUpdateCB(LobbyGameSessionsN::SessionStatusCode code, const LobbyApiUserSetT *session, void *context) {
    static_cast<UIOnlineGameSessionSearch *>(context)->UpdateCallback(code, session);
}

MenuScreen *CreateOnlineQuickJoin(ScreenConstructorData *sd) {
    return new ("UIOnlineQuickJoin", 0) UIOnlineQuickJoin(sd);
}

UIOnlineQuickJoin::UIOnlineQuickJoin(ScreenConstructorData *sd)
    : IconScrollerMenu(sd) //
    , m_boLaunchMatchList(false) //
    , m_dialogHandle(0) {
    FEngSetLanguageHash(GetPackageName(), 0xb71b576d, 0xcdf0cc03);
    Setup();
}

void UIOnlineQuickJoin::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (!m_dialogHandle && !cFEng::Get()->IsPackagePushed("OL_EAMessenger.fng")) {
        IconScrollerMenu::NotificationMessage(msg, obj, param1, param2);
    }

    switch (msg) {
        case 0x0c407210:
        case 0x406415e3:
            if (FEngMapJoyParamToJoyport(param1) >= 0) {
                FEDatabase->SetPlayersJoystickPort(0, FEngMapJoyParamToJoyport(param1));
            }
            if (!m_dialogHandle) {
                bEnableEAMessenger = false;
                m_dialogHandle = DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x417b2601, 0x1a294dad, 0xbb737c09,
                                                                 0x34dc1bcf, 0x34dc1bcf, first_dialog_button1,
                                                                 GetLocalizedString(0x5ca73434));
            }
            break;
        case 0xbb737c09:
            m_boLaunchMatchList = true;
            FEDatabase->GetOnlineSettings()->SelectedConnectingScreen = eOnlineConnecting_QuickMatchSearch;
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            break;
        case 0x34dc1bcf:
            bEnableEAMessenger = true;
            if (m_dialogHandle) {
                DialogInterface::DismissDialog(m_dialogHandle);
            }
            m_dialogHandle = 0;
            m_boLaunchMatchList = false;
            break;
        case 0xe1fde1d1:
            m_dialogHandle = 0;
            if (m_boLaunchMatchList) {
                m_boLaunchMatchList = false;
                cFEng::Get()->QueuePackageSwitch("UI_OLISPConnect.fng", 0, 0, false);
            } else {
                cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
            }
            break;
        case 0x911ab364:
            m_boLaunchMatchList = false;
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            break;
        case 0xc98356ba:
            break;
    }
}

// total size: 0x5C
struct CSprintMenu : public IconOption {
    CSprintMenu(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {
        SetReactImmediately(true);
    }

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            FEDatabase->GetOnlineSettings()->RaceMode = GRace::kRaceType_P2P;
            if (FEDatabase->IsLANMode()) {
                g_RankedGame = NO;
            } else {
                g_RankedGame = YES;
            }
        }
    }
};

// total size: 0x5C
struct CDragMenu : public IconOption {
    CDragMenu(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {
        SetReactImmediately(true);
    }

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            FEDatabase->GetOnlineSettings()->RaceMode = GRace::kRaceType_Drag;
            if (FEDatabase->IsLANMode()) {
                g_RankedGame = NO;
            } else {
                g_RankedGame = YES;
            }
        }
    }
};

// total size: 0x5C
struct CCircuitMenu : public IconOption {
    CCircuitMenu(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {
        SetReactImmediately(true);
    }

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            FEDatabase->GetOnlineSettings()->RaceMode = GRace::kRaceType_Circuit;
            if (FEDatabase->IsLANMode()) {
                g_RankedGame = NO;
            } else {
                g_RankedGame = YES;
            }
        }
    }
};

void UIOnlineQuickJoin::Setup() {
    if (TheOnlineManager.GetState() != OLS_LOBBY_IN_LOBBY) {
        LobbyCore::Instance().Resume(true);
        TheOnlineManager.StartLobby();
    }

    AddOption(new ("CCircuitMenu", 0) CCircuitMenu(0xe9638d3e, 0x034fa2c1, 0));
    AddOption(new ("CSprintMenu", 0) CSprintMenu(0x2521e5eb, 0xb94fd70e, 0));
    AddOption(new ("CDragMenu", 0) CDragMenu(0xaaab31e9, 0x6f547e4c, 0));

    SetInitialOption(FEngGetLastButton(GetPackageName()));
    RefreshHeader();
}
