#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Online/LobbyRanks.hpp"
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#include "Speed/Indep/Tools/Inc/ConversionUtil.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#include "Speed/Indep/bWare/Inc/bList.hpp"

// total size: 0x18
class OnlineRankingsManager {
  public:
    // total size: 0x1C
    struct RankScreenNode : public bTNode<RankScreenNode> {
        const char *Package;                                        // offset 0x8, size 0x4
        const char *(OnlineRankingsManager::*ForwardFunction)();  // offset 0xC, size 0x8
        const char *(OnlineRankingsManager::*BackwardFunction)(); // offset 0x14, size 0x8

        RankScreenNode(const char *, const char *(OnlineRankingsManager::*)(), const char *(OnlineRankingsManager::*)());
    };

    static OnlineRankingsManager *Instance();
    static void ManagerDestroy();
    OnlineRankingsManager(const char *, const char *, const char *);
    virtual ~OnlineRankingsManager();
    static void CreateManager(const char *);
    static void ExitToScreen(const char *);
    void ChangeToNextScreen();
    void ChangeToPrevScreen();
    const char *GetCurrentScreen();

  private:
    RankScreenNode *FindScreen(const char *);
    void AddScreen(const char *, const char *(OnlineRankingsManager::*)(), const char *(OnlineRankingsManager::*)());
    bool IsFrontdoor(const char *);
    bool IsBackdoor(const char *);
    void Start(const char *, const char *);
    const char *CalcNextScreenFromPersonal();
    const char *CalcNextScreenFromOverall();
    const char *CalcNextScreenFromMonthly();
    const char *CalcPrevScreenFromPersonal();
    const char *CalcPrevScreenFromOverall();
    const char *CalcPrevScreenFromMonthly();

    bTList<RankScreenNode> m_screenList; // offset 0x0, size 0x8
    RankScreenNode *m_currentScreen;     // offset 0x8, size 0x4
    const char *m_frontdoor;             // offset 0xC, size 0x4
    const char *m_backdoor;              // offset 0x10, size 0x4

    static OnlineRankingsManager *m_pInstance;
};

bool bShowPersonalStatsForOtherPlayer = false;
bool bShowPersonalStatsForOtherPlayerOverlay = false;
static char gOtherPlayerName[16] = "otherplayer";

// total size: 0x264
class UIOnlinePersonalRankings : public MenuScreen {
  public:
    UIOnlinePersonalRankings(ScreenConstructorData *sd);
    ~UIOnlinePersonalRankings() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    int32 PopulatePersonalRankings();
    void GetRanks();
    static void GetExtendedStatsCB(LobbyApiMsgT *msg, void *context);
    static void GetRankCB(LobbyApiMsgT *msg, void *context);

    dialog_handle msgHandle;          // offset 0x2C, size 0x4
    bool m_complete;                  // offset 0x30, size 0x4
    int m_eventHash[3];               // offset 0x34, size 0xC
    int m_currentEvent;               // offset 0x40, size 0x4
    int m_code;                       // offset 0x44, size 0x4
    PlayerDataT m_stats;              // offset 0x48, size 0x200
    LobbyApiMsgT *m_pMsg;             // offset 0x248, size 0x4
    int raceMode;                     // offset 0x24C, size 0x4
    RankListSubcategoryE subCategory; // offset 0x250, size 0x4
    RankListE currentRankList;        // offset 0x254, size 0x4
    int32 commandID;                  // offset 0x258, size 0x4
    int m_weekOffset;                 // offset 0x25C, size 0x4
    bool m_originalFlag;              // offset 0x260, size 0x4
};

extern bool EnterRankingsMenu;

MenuScreen *CreateOnlinePersonalRankings(ScreenConstructorData *sd) {
    return new ("UIOnlinePersonalRankings", 0) UIOnlinePersonalRankings(sd);
}

UIOnlinePersonalRankings::UIOnlinePersonalRankings(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , msgHandle(0) //
    , m_complete(false) //
    , m_currentEvent(0) //
    , raceMode(0) //
    , subCategory(RANKLIST_BY_WIN_PERCENTAGE) //
    , currentRankList(RL_INVALID) //
    , commandID(-1) //
    , m_weekOffset(0) {
    LobbyUsers::Instance().ClearUserStats();

    m_eventHash[0] = 0x034fa2c1;
    m_eventHash[1] = 0xb94fd70e;
    m_eventHash[2] = 0x6f547e4c;

    if (bShowPersonalStatsForOtherPlayer || bShowPersonalStatsForOtherPlayerOverlay) {
        FEPrintf(GetPackageName(), 0x7326e44c, "%s", gOtherPlayerName);
    } else {
        FEPrintf(GetPackageName(), 0x7326e44c, "%s", FEDatabase->GetOnlineSettings()->GetLobbyPersona());
    }
    GetRanks();
}

UIOnlinePersonalRankings::~UIOnlinePersonalRankings() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }

    if (GetPackageName()) {
        if (!FEngFindScreen("OL_Rankings_Personal_Overlay.fng")) {
            bShowPersonalStatsForOtherPlayer = false;
        } else {
            bShowPersonalStatsForOtherPlayerOverlay = false;
        }
    }
}

void UIOnlinePersonalRankings::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x911ab364:
            if (bShowPersonalStatsForOtherPlayer || bShowPersonalStatsForOtherPlayerOverlay) {
                cFEng::Get()->QueuePackagePop(1);
            } else {
                EnterRankingsMenu = true;
                cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
            }
            break;
        case 0x5073ef13:
            m_currentEvent--;
            if (m_currentEvent < 0) {
                m_currentEvent = 2;
            }
            m_complete = false;
            GetRanks();
            break;
        case 0xd9feec59:
            m_currentEvent++;
            if (m_currentEvent > 2) {
                m_currentEvent = 0;
            }
            m_complete = false;
            GetRanks();
            break;
        case 0x4ebed0a1:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }

            if (m_code) {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364, 0x911ab364,
                                               GetLocalizedString(0xb6aae6a6));
            } else {
                GetRanks();
                m_complete = true;
            }
            break;
    }
}

void UIOnlinePersonalRankings::GetRanks() {
    LobbyRanks &lobbyRanks = LobbyRanks::Instance();

    currentRankList = LobbyRanks::GetRankListFromMode(static_cast<OnlineRaceModeE>(m_currentEvent), subCategory, m_weekOffset);

    LobbyCore::Instance().AbortCommand(GetRankCB, true);

    commandID = lobbyRanks.GetRanks(currentRankList, GetRankCB, this);
    if (commandID < 0) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364, 0x911ab364,
                                       GetLocalizedString(0x6cf6e689));
    } else if (commandID == 0) {
        PopulatePersonalRankings();
    } else {
        msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "animating", dialog_alert, 0x1a294dad, 0x00ca4ce1,
                                                   GetLocalizedString(0x12df9f4f));
    }
}

int32 UIOnlinePersonalRankings::PopulatePersonalRankings() {
    int32 rc;
    const char *szSpeedUnit;
    const char *szWeightUnit;
    const char *szSeconds;
    bool bMetricUnit;
    uint32 uTotalGamesPlayed;
    LobbyApiRankT *myrank;
    float fTotalNOS;

    if (m_complete) {
        return 0;
    }

    if (bShowPersonalStatsForOtherPlayer || bShowPersonalStatsForOtherPlayerOverlay) {
        rc = LobbyUsers::Instance().GetOtherUserStats(m_stats, gOtherPlayerName, GetExtendedStatsCB, this);
    } else {
        rc = LobbyUsers::Instance().GetMyUserStats(m_stats, GetExtendedStatsCB, this);
    }

    if (rc < 0) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364, 0x911ab364,
                                       GetLocalizedString(0xb6aae6a6));
        return rc;
    }

    szSeconds = GetLocalizedString(0x58968929);

    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        bMetricUnit = true;
        szSpeedUnit = GetLocalizedString(0x8569a25f);
        szWeightUnit = GetLocalizedString(0xc173de1f);
    } else {
        bMetricUnit = false;
        szSpeedUnit = GetLocalizedString(0x8569ab44);
        szWeightUnit = GetLocalizedString(0xc173e1bb);
    }

    FEngSetLanguageHash(GetPackageName(), 0x116d2d5c, m_eventHash[m_currentEvent]);

    FEPrintf(GetPackageName(), 0xc354e4cf, "%d", m_stats.GetTotalGamesPlayed(3));
    FEPrintf(GetPackageName(), 0x89d52d43, "%d", m_stats.GetTotalWins(3));
    FEPrintf(GetPackageName(), 0x3a9c253b, "%d", m_stats.GetTotalLosses(3));
    FEPrintf(GetPackageName(), 0xab46130a, "%d%%", static_cast<uint32>(m_stats.GetWinPercentage(3)));
    FEPrintf(GetPackageName(), 0x134dac12, "%d%%", static_cast<uint32>(m_stats.GetTotalDisconnectPercentage()));

    uTotalGamesPlayed = m_stats.GetTotalGamesPlayed(m_currentEvent);

    myrank = nullptr;
    if (bShowPersonalStatsForOtherPlayer || bShowPersonalStatsForOtherPlayerOverlay) {
        int numRanks = LobbyRanks::Instance().GetRankListSize(currentRankList, RLT_TOP100);
        for (int i = 0; i < numRanks; i++) {
            myrank = LobbyRanks::Instance().GetRankData(currentRankList, RLT_TOP100, i);
            if (bStrCmp(myrank->name, gOtherPlayerName) == 0) {
                break;
            }
            myrank = nullptr;
        }
    } else {
        myrank = LobbyRanks::Instance().GetRankData(currentRankList, RLT_PERSONAL, 0);
    }

    RankingDataT myrankData;

    if (myrank) {
        TagFieldGetStructure(myrank->stat, &myrankData, sizeof(RankingDataT), "l*");
        FEPrintf(GetPackageName(), 0x7ab7102b, "%d", myrank->rank);
    } else {
        FEPrintf(GetPackageName(), 0x7ab7102b, "%s", "---");
    }

    FEPrintf(GetPackageName(), 0xd19915bd, "%d", m_stats.stats.raceModeStats[m_currentEvent].points);
    FEPrintf(GetPackageName(), 0x7ab7102c, "%d", uTotalGamesPlayed);
    FEPrintf(GetPackageName(), 0x7ab7102d, "%d", m_stats.GetTotalWins(m_currentEvent));
    FEPrintf(GetPackageName(), 0x7ab7102e, "%d", m_stats.GetTotalLosses(m_currentEvent));
    FEPrintf(GetPackageName(), 0x7ab7102f, "%d%%", static_cast<uint32>(m_stats.GetWinPercentage(m_currentEvent)));

    // La A124 convierte con dos inline de CarBasics.hpp (lineas 13 y 9: el juego "float x" de MPS2KPH/MPS2MPH que
    // el DWARF de GC tambien tiene), que multiplican por 3.6f y 2.237f; el MPS2MPH de ConversionUtil.hpp usa 2.23699f
    // y los dos juegos no pueden convivir con el mismo nombre, asi que la cuenta va escrita tal cual.
    FEPrintf(GetPackageName(), 0x7ab71030, "%d %s",
             bMetricUnit ? static_cast<uint32>(m_stats.GetTopSpeedInMPS(m_currentEvent) * 3.6f)
                         : static_cast<uint32>(m_stats.GetTopSpeedInMPS(m_currentEvent) * 2.237f),
             szSpeedUnit);
    FEPrintf(GetPackageName(), 0x7ab71031, "%f %s", m_stats.GetLongestJumpDuration(m_currentEvent), szSeconds);

    fTotalNOS = m_stats.GetTotalNOS(m_currentEvent);
    if (bMetricUnit) {
        fTotalNOS = LB2KG(fTotalNOS);
    }

    FEPrintf(GetPackageName(), 0x7ab71032, "%d %s", uTotalGamesPlayed ? static_cast<uint32>(fTotalNOS / uTotalGamesPlayed) : 0,
             szWeightUnit);
    FEPrintf(GetPackageName(), 0x7ab71033, "%d %s", static_cast<uint32>(fTotalNOS), szWeightUnit);

    FEPrintf(GetPackageName(), 0xd19915bb, "%d%%", static_cast<uint32>(m_stats.GetTotalDisconnectPercentage()));
    FEPrintf(GetPackageName(), 0xd19915bc, "%d", m_stats.GetMonthlyPointTotals(m_currentEvent));

    return rc;
}

void UIOnlinePersonalRankings::GetExtendedStatsCB(LobbyApiMsgT *msg, void *context) {
    UIOnlinePersonalRankings *personalRankings = static_cast<UIOnlinePersonalRankings *>(context);

    personalRankings->m_code = msg->code;
    if (bShowPersonalStatsForOtherPlayer || bShowPersonalStatsForOtherPlayerOverlay) {
        personalRankings->m_stats.ExtractRecord(msg->pData);
    }
    if (bIsValidPointer(const_cast<char *>(personalRankings->GetPackageName()), 1)) {
        cFEng::Get()->QueueGameMessage(0x4ebed0a1, personalRankings->GetPackageName(), 0xff);
    }
}

void UIOnlinePersonalRankings::GetRankCB(LobbyApiMsgT *msg, void *context) {
    UIOnlinePersonalRankings *personalRankings = static_cast<UIOnlinePersonalRankings *>(context);

    DialogInterface::DismissDialog(personalRankings->msgHandle);
    personalRankings->msgHandle = 0;

    personalRankings->commandID = -1;

    if (msg->code == 0) {
        personalRankings->PopulatePersonalRankings();
    } else {
        DialogInterface::ShowOneButton(personalRankings->GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364,
                                       0x911ab364, GetLocalizedString(0x6cf6e689));
    }
}
