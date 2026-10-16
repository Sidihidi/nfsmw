#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Online/LobbyRanks.hpp"
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"

enum SortField {
    SORT_RANK = 0,
    SORT_NAME = 1,
    SORT_WIN_PERCENT = 2,
    SORT_TOP_SPEED = 3,
    SORT_N20 = 4,
    SORT_TN20 = 5,
    SORT_DISCONNS = 6,
    SORT_GP = 7,
    SORT_WINS = 8,
    SORT_LOSSES = 9,
    SORT_ASCENDING = 0x80000000,
};

// total size: 0x41C
class UIOnlineRankingsLeaderboard : public MenuScreen {
  public:
    UIOnlineRankingsLeaderboard(ScreenConstructorData *sd);
    ~UIOnlineRankingsLeaderboard() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup();
    void ClearRanks(int first, int last);
    void GetRanks();
    int32 PopulateRankingsList(bool keepHighlight);
    void ScrollList(int direction);
    void SetViewableColumnHeadings();
    void UpdateViewableColumns(RankingDataT *rankData, int index);
    void SetWinPercentage(unsigned int objHash, RankingDataT *rankData);
    void SetTopSpeed(unsigned int objHash, int value);
    void SetFastestFinish(unsigned int, RankingDataT *);
    void SetAvgNOSUsed(unsigned int objHash, RankingDataT *rankData);
    void SetGenericValue(unsigned int objHash, int value);
    void SetDisconnectPercentage(unsigned int objHash, RankingDataT *rankData);
    void SetSortPos(int index);
    static void GetRankCB(LobbyApiMsgT *msg, void *context);
    static void UserStatsCB(LobbyApiMsgT *msg, void *context);
    void SortCurrentColumn();
    static int SortFunc(void *sortref, int sortcon, void *recptr1, void *recptr2);

    static const int m_ranksPerPage;
    static const int m_NumberOfRankingSlots;

    int raceMode;                     // offset 0x2C, size 0x4
    RankListSubcategoryE subCategory; // offset 0x30, size 0x4
    RankListE currentRankList;        // offset 0x34, size 0x4
    int m_columnIndex;                // offset 0x38, size 0x4
    int m_highlightIndex;             // offset 0x3C, size 0x4
    int32 commandID;                  // offset 0x40, size 0x4
    PlayerDataT myUserStats;          // offset 0x44, size 0x200
    dialog_handle msgHandle;          // offset 0x244, size 0x4
    bool m_complete;                  // offset 0x248, size 0x4
    int m_eventHash[3];               // offset 0x24C, size 0xC
    int m_currentEvent;               // offset 0x258, size 0x4
    FEScrollBar m_vertScroller;       // offset 0x25C, size 0x64
    int m_PositionHandleIndex;        // offset 0x2C0, size 0x4
    FEScrollBar m_statScroller;       // offset 0x2C4, size 0x64
    Scrollerina m_rankScroller;       // offset 0x328, size 0xC8
    bool m_rankScrollerBuilt;         // offset 0x3F0, size 0x4
    ScrollerSlot *m_firstSlot;        // offset 0x3F4, size 0x4
    ScrollerSlot *m_lastSlot;         // offset 0x3F8, size 0x4
    int m_usedCount;                  // offset 0x3FC, size 0x4
    int m_listStart;                  // offset 0x400, size 0x4
    int m_msgCode;                    // offset 0x404, size 0x4
    int m_selectedColumn;             // offset 0x408, size 0x4
    ScrollerSlot *m_currentSlot;      // offset 0x40C, size 0x4
    int m_weekOffset;                 // offset 0x410, size 0x4
    int m_lastSortPosition;           // offset 0x414, size 0x4
    int m_ascendingFlag;              // offset 0x418, size 0x4
};

// Definidos en UIOnlineMainMenu.cpp y en uiOnlinePersonalRankings.cpp (gOtherPlayerName es static alli: la unidad zFe
// los ve porque los incluye antes).
extern bool EnterRankingsMenu;
extern bool bShowPersonalStatsForOtherPlayer;
extern bool bShowPersonalStatsForOtherPlayerOverlay;
extern char gOtherPlayerName[16];

MenuScreen *CreateOnlineRankingsLeaderboard(ScreenConstructorData *sd) {
    return new ("UIOnlineRankingsLeaderboard", 0) UIOnlineRankingsLeaderboard(sd);
}

int RankColumnHeading[10][3] = {
    {0, 0, 0},
    {0, 0, 0},
    {0x64929379, 0x64929379, 0x64929379},
    {0xdb615b4a, 0xdb615b4a, 0x6490ee55},
    {0x60234f76, 0x60234f76, 0x60234f76},
    {0x649103ea, 0x649103ea, 0x649103ea},
    {0x648828eb, 0x648828eb, 0x648828eb},
    {0x29b37dbd, 0x29b37dbd, 0x29b37dbd},
    {0x649293a7, 0x649293a7, 0x649293a7},
    {0x648ca5a7, 0x648ca5a7, 0x648ca5a7},
};

unsigned int ColumnHeadingExpanded[10] = {
    0xa89d8f41, 0xf760eabe, 0x91d1594e, 0x5912a1e4, 0x0c5dbfaa, 0x7cb591f4, 0x2d676048, 0x178df2f2, 0x649293a7, 0x648ca5a7,
};

unsigned int DragColumnHeadingExpanded[10] = {
    0xa89d8f41, 0xf760eabe, 0x91d1594e, 0x1898b7c1, 0x0c5dbfaa, 0x7cb591f4, 0x2d676048, 0x178df2f2, 0x649293a7, 0x648ca5a7,
};

const int UIOnlineRankingsLeaderboard::m_ranksPerPage = 10;
const int UIOnlineRankingsLeaderboard::m_NumberOfRankingSlots = 10;

UIOnlineRankingsLeaderboard::UIOnlineRankingsLeaderboard(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , raceMode(0) //
    , subCategory(RANKLIST_BY_WIN_PERCENTAGE) //
    , currentRankList(RL_INVALID) //
    , m_columnIndex(0) //
    , m_highlightIndex(2) //
    , commandID(-1) //
    , msgHandle(0) //
    , m_complete(false) //
    , m_currentEvent(0) //
    , m_vertScroller(GetPackageName(), "ScrollBar", true, false, false) //
    , m_PositionHandleIndex(1) //
    , m_statScroller(GetPackageName(), "H_ScrollBar", false, true, false) //
    , m_rankScroller(GetPackageName(), nullptr, nullptr, true, true, false, false) //
    , m_rankScrollerBuilt(false) //
    , m_usedCount(0) //
    , m_listStart(0) //
    , m_selectedColumn(2) //
    , m_currentSlot(nullptr) //
    , m_weekOffset(0) //
    , m_lastSortPosition(2) //
    , m_ascendingFlag(0) {
    int32 rc;

    m_eventHash[0] = 0x034fa2c1;
    m_eventHash[1] = 0xb94fd70e;
    m_eventHash[2] = 0x6f547e4c;

    FEngSetLanguageHash(GetPackageName(), 0xca5bcc10, 0x99602893);

    ClearRanks(1, 10);
    Setup();

    if (raceMode == 2) {
        FEngSetLanguageHash(GetPackageName(), 0x2caec5d0, DragColumnHeadingExpanded[m_selectedColumn]);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x2caec5d0, ColumnHeadingExpanded[m_selectedColumn]);
    }

    rc = LobbyUsers::Instance().GetMyUserStats(myUserStats, UserStatsCB, this);
    if (rc < 0) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364, 0x911ab364,
                                       GetLocalizedString(0x6cf6e689));
        return;
    }

    if (rc == 0) {
        GetRanks();
    }

    if (PopulateRankingsList(false) > 0) {
        msgHandle = DialogInterface::ShowMessage(GetPackageName(), "", dialog_alert, GetLocalizedString(0xec3905c1));
    }

    FEngSetScript(GetPackageName(), 0x9321399c, 0x001b20c4, true);

    FEPrintf(GetPackageName(), 0x502caa6d, " ");
    FEPrintf(GetPackageName(), 0x68be9ca9, " ");
    FEPrintf(GetPackageName(), 0xe7adc641, " ");
    FEPrintf(GetPackageName(), 0xe7adc641, " ");
    FEPrintf(GetPackageName(), 0xe7adc641, " ");
    FEPrintf(GetPackageName(), 0xe7adc641, " ");

    FEngSetLanguageHash(GetPackageName(), 0x001450dd, 0xe9a7da00);
}

UIOnlineRankingsLeaderboard::~UIOnlineRankingsLeaderboard() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }

    LobbyCore::Instance().AbortAllCommands();
    LobbyRanks::Instance().ClearCache();
}

void UIOnlineRankingsLeaderboard::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (bShowPersonalStatsForOtherPlayer || bShowPersonalStatsForOtherPlayerOverlay) {
        return;
    }

    switch (msg) {
        case 0x406415e3:
            if (m_usedCount) {
                ScrollerDatum *datum = m_rankScroller.GetSelectedDatum();

                bStrCpy(gOtherPlayerName, datum->Strings.GetHead()->String);
                bShowPersonalStatsForOtherPlayer = true;
                cFEng::Get()->QueuePackagePush("OL_Rankings_Personal.fng", 0, 0, false);
            }
            break;
        case 0xc519bfc3:
            ClearRanks(1, 10);
            switch (m_weekOffset) {
                case 0:
                    FEngSetLanguageHash(GetPackageName(), 0x001450dd, 0xb90429ea);
                    m_weekOffset = 17;
                    break;
                case 17:
                    FEngSetLanguageHash(GetPackageName(), 0x001450dd, 0xefbe5784);
                    m_weekOffset = 30;
                    break;
                case 30:
                    FEngSetLanguageHash(GetPackageName(), 0x001450dd, 0xe9a7da00);
                    m_weekOffset = 0;
                    break;
            }
            m_complete = false;
            GetRanks();
            break;
        case 0x911ab364:
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), FEngFindObject(GetPackageName(), 0xf015108a));
            break;
        case 0xe1fde1d1:
            EnterRankingsMenu = true;
            cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
            break;
        case 0xd9feec59:
            m_ascendingFlag = 0;
            raceMode++;
            if (raceMode > 2) {
                raceMode = 0;
            }
            m_complete = false;
            m_listStart = 0;
            GetRanks();
            break;
        case 0x5073ef13:
            m_ascendingFlag = 0;
            raceMode--;
            if (raceMode < 0) {
                raceMode = 2;
            }
            m_complete = false;
            m_listStart = 0;
            GetRanks();
            break;
        case 0x911c0a4b:
            ScrollList(1);
            break;
        case 0x72619778:
            ScrollList(-1);
            break;
        case 0xc519bfc4: {
            char sztemp[32];

            SetSortPos(m_highlightIndex);
            FEngSNPrintf(sztemp, 32, "POS%d", m_highlightIndex + 1);
            FEngSetScript(GetPackageName(), 0x9321399c, FEHashUpper(sztemp), true);
            SortCurrentColumn();
            break;
        }
        case 0xb5971bf1:
            m_currentSlot = m_rankScroller.GetSelectedSlot();
            m_selectedColumn++;
            if (m_selectedColumn > 9) {
                m_selectedColumn = 9;
            }
            m_highlightIndex++;
            if (m_highlightIndex > 5) {
                m_columnIndex++;
                if (m_columnIndex > 4) {
                    m_columnIndex = 4;
                }
                m_highlightIndex = 5;
            }
            SetSortPos(m_highlightIndex);
            SetViewableColumnHeadings();
            PopulateRankingsList(true);
            break;
        case 0x9120409e:
            m_currentSlot = m_rankScroller.GetSelectedSlot();
            m_selectedColumn--;
            if (m_selectedColumn < 0) {
                m_selectedColumn = 0;
            }
            m_highlightIndex--;
            if (m_selectedColumn < 6) {
                m_columnIndex--;
                if (m_highlightIndex < 2 && m_columnIndex >= 0) {
                    m_highlightIndex = 2;
                }
                if (m_columnIndex < 0) {
                    m_columnIndex = 0;
                }
            }
            if (m_highlightIndex < 0) {
                m_highlightIndex = 0;
            }
            SetSortPos(m_highlightIndex);
            SetViewableColumnHeadings();
            PopulateRankingsList(true);
            break;
        case 0x6b7baa6f:
            if (m_msgCode) {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364, 0x911ab364,
                                               GetLocalizedString(0x6cf6e689));
            } else {
                LobbyUsers::Instance().GetMyUserStats(myUserStats, nullptr, nullptr);
                GetRanks();
            }
            break;
        case 0xf1943ca3:
            ClearRanks(1, 10);
            PopulateRankingsList(false);
            ClearRanks(1, 10);
            PopulateRankingsList(false);
            break;
    }
}

void UIOnlineRankingsLeaderboard::ClearRanks(int first, int last) {
    if (first < 1 || last > 10 || first > last) {
        return;
    }

    for (int i = first; i <= last; i++) {
        ScrollerDatum *datum = new ("ScrollerDatum", 0) ScrollerDatum;
        datum->AddData("", 0);
        m_rankScroller.AddData(datum);
        m_rankScroller.Disable(datum);

        FEPrintf(GetPackageName(), FEngHashString("RANK_%d", i), " ");
        FEPrintf(GetPackageName(), FEngHashString("GAMERTAG_%d", i), " ");
        FEPrintf(GetPackageName(), FEngHashString("COLUMN3_%d", i), " ");
        FEPrintf(GetPackageName(), FEngHashString("COLUMN4_%d", i), " ");
        FEPrintf(GetPackageName(), FEngHashString("COLUMN5_%d", i), " ");
        FEPrintf(GetPackageName(), FEngHashString("COLUMN6_%d", i), " ");
    }

    FEPrintf(GetPackageName(), 0x502caa6d, " ");
    FEPrintf(GetPackageName(), 0x68be9ca9, " ");
    FEPrintf(GetPackageName(), 0xe7adc641, " ");
    FEPrintf(GetPackageName(), 0xe7ae52a2, " ");
    FEPrintf(GetPackageName(), 0xe7aedf03, " ");
    FEPrintf(GetPackageName(), 0xe7af6b64, " ");
}

void UIOnlineRankingsLeaderboard::GetRanks() {
    LobbyRanks &lobbyRanks = LobbyRanks::Instance();

    currentRankList = LobbyRanks::GetRankListFromMode(static_cast<OnlineRaceModeE>(raceMode), subCategory, m_weekOffset);

    LobbyCore::Instance().AbortCommand(GetRankCB, true);

    commandID = lobbyRanks.GetRanks(currentRankList, GetRankCB, this);
    if (commandID < 0) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364, 0x911ab364,
                                       GetLocalizedString(0x6cf6e689));
    } else if (commandID == 0) {
        PopulateRankingsList(false);
    } else {
        msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "animating", dialog_alert, 0x1a294dad, 0x00ca4ce1,
                                                   GetLocalizedString(0x12df9f4f));
    }
}

void UIOnlineRankingsLeaderboard::ScrollList(int direction) {
    ScrollerSlot *pslot;
    int currentIndex;
    int numRanks;
    char sztemp[32];

    if (m_usedCount) {
        pslot = m_rankScroller.GetSelectedSlot();
        currentIndex = m_rankScroller.GetSelectedNodeIndex();

        LobbyRanks &lobbyRanks = LobbyRanks::Instance();
        numRanks = lobbyRanks.GetRankListSize(currentRankList, RLT_TOP100);

        m_rankScroller.UnHighlightSelected();

        if (direction < 0) {
            m_PositionHandleIndex--;
            if (m_PositionHandleIndex < 1) {
                m_PositionHandleIndex = 1;
            }
            currentIndex--;
            if (currentIndex < 1) {
                m_listStart--;
                if (m_listStart < 0) {
                    m_listStart = 0;
                }
                PopulateRankingsList(false);
                m_rankScroller.SetSelected(pslot);
                return;
            } else {
                m_rankScroller.ScrollPrev();
            }
        } else if (direction > 0) {
            m_PositionHandleIndex++;
            if (m_PositionHandleIndex >= numRanks) {
                m_PositionHandleIndex = numRanks;
            }
            currentIndex++;
            if (currentIndex > 10) {
                if (m_usedCount > 10) {
                    m_listStart++;
                    PopulateRankingsList(false);
                    m_rankScroller.SetSelected(pslot);
                }
            } else {
                m_rankScroller.ScrollNext();
            }
        }

        m_rankScroller.Update(true);

        currentIndex = m_rankScroller.GetSelectedNodeIndex();
        FEngSNPrintf(sztemp, 32, "POS%d", currentIndex);
        FEngSetScript(GetPackageName(), 0xdaf7, FEHashUpper(sztemp), true);
        m_vertScroller.Update(1, numRanks, m_PositionHandleIndex, -1);
        m_rankScroller.HighlightSelected();
    }
}

int32 UIOnlineRankingsLeaderboard::PopulateRankingsList(bool keepHighlight) {
    LobbyRanks &lobbyRanks = LobbyRanks::Instance();
    int i;
    LobbyApiRankT *myrank;
    int numRanks;

    m_rankScroller.DeleteScrollerData();
    m_usedCount = 0;

    switch (raceMode) {
        case 0:
            FEngSetLanguageHash(GetPackageName(), 0x116d2d5c, 0x034fa2c1);
            break;
        case 1:
            FEngSetLanguageHash(GetPackageName(), 0x116d2d5c, 0xb94fd70e);
            break;
        case 2:
            FEngSetLanguageHash(GetPackageName(), 0x116d2d5c, 0x6f547e4c);
            break;
    }

    if (raceMode == 2) {
        FEngSetLanguageHash(GetPackageName(), 0x2caec5d0, DragColumnHeadingExpanded[m_selectedColumn]);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x2caec5d0, ColumnHeadingExpanded[m_selectedColumn]);
    }

    SetViewableColumnHeadings();

    if (currentRankList == RL_INVALID) {
        return 0;
    }

    i = 0;

    for (; i < 11; i++) {
        LobbyApiRankT *rank = lobbyRanks.GetRankData(currentRankList, RLT_TOP100, i + m_listStart);
        if (!rank) {
            break;
        }

        ScrollerDatum *datum = new ("ScrollerDatum", 0) ScrollerDatum;
        RankingDataT rankData;

        TagFieldGetStructure(rank->stat + 1, &rankData, sizeof(RankingDataT), "l*");

        FEPrintf(GetPackageName(), FEngHashString("RANK_%d", i + 1), "%d", rank->rank);
        FEPrintf(GetPackageName(), FEngHashString("GAMERTAG_%d", i + 1), "%s", rank->name);
        datum->AddData(rank->name, 0);

        UpdateViewableColumns(&rankData, i + 1);

        m_rankScroller.AddData(datum);
        m_rankScroller.Enable(datum);
        m_usedCount++;
    }

    m_statScroller.Update(1, 10, m_selectedColumn + 1, -1);

    ClearRanks(i + 1, 10);
    m_rankScroller.Update(true);

    myrank = lobbyRanks.GetRankData(currentRankList, RLT_PERSONAL, 0);

    RankingDataT myrankData;

    if (myrank) {
        TagFieldGetStructure(myrank->stat, &myrankData, sizeof(RankingDataT), "l*");

        FEPrintf(GetPackageName(), 0x502caa6d, "%d", myrank->rank);
        FEPrintf(GetPackageName(), 0x68be9ca9, myrank->name);

        UpdateViewableColumns(&myrankData, 11);
    } else {
        FEPrintf(GetPackageName(), 0x502caa6d, "--");
        FEPrintf(GetPackageName(), 0x68be9ca9, FEDatabase->GetOnlineSettings()->GetLobbyPersona());

        UpdateViewableColumns(&myrankData, 11);
    }

    numRanks = lobbyRanks.GetRankListSize(currentRankList, RLT_TOP100);
    if (m_usedCount) {
        if (keepHighlight) {
            m_rankScroller.SetSelected(m_currentSlot);
        } else {
            m_rankScroller.UnHighlightSelected();
            m_rankScroller.Reset(true);
            ScrollList(0);
        }
        m_rankScroller.HighlightSelected();
    }

    m_vertScroller.Update(1, numRanks, m_PositionHandleIndex, -1);

    if (lobbyRanks.GetRankData(currentRankList, RLT_TOP100, i + m_listStart)) {
        m_usedCount++;
    }

    m_rankScroller.Update(true);

    return 0;
}

void UIOnlineRankingsLeaderboard::GetRankCB(LobbyApiMsgT *msg, void *context) {
    UIOnlineRankingsLeaderboard *overallRankings = static_cast<UIOnlineRankingsLeaderboard *>(context);

    DialogInterface::DismissDialog(overallRankings->msgHandle);
    overallRankings->msgHandle = 0;

    overallRankings->commandID = -1;

    if (msg->code == 0) {
        overallRankings->PopulateRankingsList(false);
    } else {
        DialogInterface::ShowOneButton(overallRankings->GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364,
                                       0x911ab364, GetLocalizedString(0x6cf6e689));
    }
}

void UIOnlineRankingsLeaderboard::UserStatsCB(LobbyApiMsgT *msg, void *context) {
    UIOnlineRankingsLeaderboard *overallRankings = static_cast<UIOnlineRankingsLeaderboard *>(context);

    overallRankings->m_msgCode = msg->code;
    cFEng::Get()->QueueGameMessage(0x6b7baa6f, overallRankings->GetPackageName(), 0xff);
}

void UIOnlineRankingsLeaderboard::SetViewableColumnHeadings() {
    FEngSetLanguageHash(GetPackageName(), 0x30ee5e68, RankColumnHeading[m_columnIndex + 2][raceMode]);
    FEngSetLanguageHash(GetPackageName(), 0x1d303349, RankColumnHeading[m_columnIndex + 3][raceMode]);
    FEngSetLanguageHash(GetPackageName(), 0x0972082a, RankColumnHeading[m_columnIndex + 4][raceMode]);
    FEngSetLanguageHash(GetPackageName(), 0xf5b3dd0b, RankColumnHeading[m_columnIndex + 5][raceMode]);
}

void UIOnlineRankingsLeaderboard::UpdateViewableColumns(RankingDataT *rankData, int index) {
    char sztemp[32];
    uint32 column3;
    uint32 column4;
    uint32 column5;
    uint32 column6;

    FEngSNPrintf(sztemp, 32, "COLUMN3_%d", index);
    column3 = FEHashUpper(sztemp);
    FEngSNPrintf(sztemp, 32, "COLUMN4_%d", index);
    column4 = FEHashUpper(sztemp);
    FEngSNPrintf(sztemp, 32, "COLUMN5_%d", index);
    column5 = FEHashUpper(sztemp);
    FEngSNPrintf(sztemp, 32, "COLUMN6_%d", index);
    column6 = FEHashUpper(sztemp);

    switch (m_columnIndex) {
        case 0:
            SetWinPercentage(column3, rankData);
            if (raceMode == 2) {
                SetTopSpeed(column4, rankData->fastestFinishTime);
            } else {
                SetTopSpeed(column4, rankData->topspeed);
            }
            SetAvgNOSUsed(column5, rankData);
            SetGenericValue(column6, rankData->totalNOS);
            break;
        case 1:
            if (raceMode == 2) {
                SetTopSpeed(column3, rankData->fastestFinishTime);
            } else {
                SetTopSpeed(column3, rankData->topspeed);
            }
            SetAvgNOSUsed(column4, rankData);
            SetGenericValue(column5, rankData->totalNOS);
            SetDisconnectPercentage(column6, rankData);
            break;
        case 2:
            SetAvgNOSUsed(column3, rankData);
            SetGenericValue(column4, rankData->totalNOS);
            SetDisconnectPercentage(column5, rankData);
            SetGenericValue(column6, rankData->wins + rankData->losses);
            break;
        case 3:
            SetGenericValue(column3, rankData->totalNOS);
            SetDisconnectPercentage(column4, rankData);
            SetGenericValue(column5, rankData->wins + rankData->losses);
            SetGenericValue(column6, rankData->wins);
            break;
        case 4:
            SetDisconnectPercentage(column3, rankData);
            SetGenericValue(column4, rankData->wins + rankData->losses);
            SetGenericValue(column5, rankData->wins);
            SetGenericValue(column6, rankData->losses);
            break;
    }
}

void UIOnlineRankingsLeaderboard::SetWinPercentage(unsigned int objHash, RankingDataT *rankData) {
    uint32 games = rankData->wins + rankData->losses;
    float winpercentage = games ? (static_cast<float>(rankData->wins) / static_cast<float>(games)) * 100.0f : 0.0f;
    FEPrintf(GetPackageName(), objHash, "%d%%", static_cast<uint32>(winpercentage));
}

void UIOnlineRankingsLeaderboard::SetGenericValue(unsigned int objHash, int value) {
    FEPrintf(GetPackageName(), objHash, "%d", value);
}

void UIOnlineRankingsLeaderboard::SetAvgNOSUsed(unsigned int objHash, RankingDataT *rankData) {
    uint32 games = rankData->wins + rankData->losses;
    float avgNOS = games ? static_cast<float>(rankData->totalNOS) / static_cast<float>(games) : 0.0f;
    FEPrintf(GetPackageName(), objHash, "%d", static_cast<uint32>(avgNOS));
}

void UIOnlineRankingsLeaderboard::SetDisconnectPercentage(unsigned int objHash, RankingDataT *rankData) {
    uint32 games = rankData->wins + rankData->losses;
    float disconnectpercentage =
        games ? (static_cast<float>(rankData->disconnects) / static_cast<float>(games)) * 100.0f : 0.0f;
    FEPrintf(GetPackageName(), objHash, "%d", static_cast<uint32>(disconnectpercentage));
}

void UIOnlineRankingsLeaderboard::SetSortPos(int index) {
    char sztemp[32];

    FEngSNPrintf(sztemp, 32, "POS%d", index + 1);
    FEngSetScript(GetPackageName(), 0x551015cd, FEHashUpper(sztemp), true);
}

void UIOnlineRankingsLeaderboard::SetTopSpeed(unsigned int objHash, int value) {
    bool bMetricUnit = FEDatabase->GetGameplaySettings()->SpeedoUnits == 1;
    float iMetersPerSecond = static_cast<float>(value) * 0.001f;

    // Como en uiOnlinePersonalRankings.cpp: la A124 convierte con MPS2KPH/MPS2MPH de CarBasics.hpp (3.6f y 2.237f).
    FEPrintf(GetPackageName(), objHash, "%d",
             bMetricUnit ? static_cast<uint32>(iMetersPerSecond * 3.6f) : static_cast<uint32>(iMetersPerSecond * 2.237f));
}

void UIOnlineRankingsLeaderboard::Setup() {
    char sztemp[32];

    FEngSetLanguageHash(GetPackageName(), 0x7326e44c, 0xb90429ea);

    for (int i = 0; i < 10; i++) {
        ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;

        FEngSNPrintf(sztemp, 32, "GAMERTAG_%d", i + 1);
        FEString *str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pslot->AddData(str);

        m_rankScroller.AddSlot(pslot);
    }
}

void UIOnlineRankingsLeaderboard::SortCurrentColumn() {
    int column = m_selectedColumn;
    column |= m_ascendingFlag;

    if (column < 0) {
        FEngSetScript(GetPackageName(), 0x4f7e6ae9, FEHashUpper("UP"), true);
    } else {
        FEngSetScript(GetPackageName(), 0x4f7e6ae9, FEHashUpper("DOWN"), true);
    }

    DispListSort(LobbyRanks::Instance().GetDisplayListReference(currentRankList), this, column, SortFunc);

    if (m_lastSortPosition == m_selectedColumn) {
        m_ascendingFlag ^= SORT_ASCENDING;
    } else {
        m_ascendingFlag = 0;
    }

    m_lastSortPosition = m_selectedColumn;
    cFEng::Get()->QueueGameMessage(0xf1943ca3, GetPackageName(), 0xff);
}

int UIOnlineRankingsLeaderboard::SortFunc(void *sortref, int sortcon, void *recptr1, void *recptr2) {
    bool ascending = (sortcon & SORT_ASCENDING) != 0;
    SortField sortField = static_cast<SortField>(sortcon & 0x0fffffff);
    RankingDataT rankData1;
    RankingDataT rankData2;
    uint32 games1;
    uint32 games2;
    int rc;

    TagFieldGetStructure(static_cast<LobbyApiRankT *>(recptr1)->stat + 1, &rankData1, sizeof(RankingDataT), "l*");
    TagFieldGetStructure(static_cast<LobbyApiRankT *>(recptr2)->stat + 1, &rankData2, sizeof(RankingDataT), "l*");
    games1 = rankData1.wins + rankData1.losses;
    games2 = rankData2.wins + rankData2.losses;

    switch (sortField) {
        case SORT_RANK:
            rc = static_cast<LobbyApiRankT *>(recptr1)->rank - static_cast<LobbyApiRankT *>(recptr2)->rank;
            return ascending ? rc : -rc;
        case SORT_NAME:
            rc = bStrICmp(static_cast<LobbyApiRankT *>(recptr1)->name, static_cast<LobbyApiRankT *>(recptr2)->name);
            return ascending ? rc : -rc;
        case SORT_WIN_PERCENT: {
            float winpercentage1 =
                games1 ? (static_cast<float>(rankData1.wins) / static_cast<float>(games1)) * 100.0f : 0.0f;
            float winpercentage2 =
                games2 ? (static_cast<float>(rankData2.wins) / static_cast<float>(games2)) * 100.0f : 0.0f;
            if (winpercentage1 > winpercentage2) {
                rc = 1;
            } else if (winpercentage1 == winpercentage2) {
                rc = 0;
            } else {
                rc = -1;
            }
            return ascending ? rc : -rc;
        }
        case SORT_TOP_SPEED:
            rc = rankData1.topspeed - rankData2.topspeed;
            return ascending ? rc : -rc;
        case SORT_N20: {
            float avgNOS1 = games1 ? static_cast<float>(rankData1.totalNOS) / static_cast<float>(games1) : 0.0f;
            float avgNOS2 = games2 ? static_cast<float>(rankData2.totalNOS) / static_cast<float>(games2) : 0.0f;
            if (avgNOS1 > avgNOS2) {
                rc = 1;
            } else if (avgNOS1 == avgNOS2) {
                rc = 0;
            } else {
                rc = -1;
            }
            return ascending ? rc : -rc;
        }
        case SORT_TN20:
            rc = rankData1.totalNOS - rankData2.totalNOS;
            return ascending ? rc : -rc;
        case SORT_DISCONNS: {
            float disconnectpercentage1 =
                games1 ? (static_cast<float>(rankData1.disconnects) / static_cast<float>(games1)) * 100.0f : 0.0f;
            float disconnectpercentage2 =
                games2 ? (static_cast<float>(rankData2.disconnects) / static_cast<float>(games2)) * 100.0f : 0.0f;
            rc = static_cast<int>(disconnectpercentage1 - disconnectpercentage2);
            return ascending ? rc : -rc;
        }
        case SORT_GP:
            rc = games1 - games2;
            return ascending ? rc : -rc;
        case SORT_WINS:
            rc = rankData1.wins - rankData2.wins;
            return ascending ? rc : -rc;
        case SORT_LOSSES:
            rc = rankData1.losses - rankData2.losses;
            return ascending ? rc : -rc;
    }

    return 0;
}
