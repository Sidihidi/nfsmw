#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Online/LobbyRanks.hpp"
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"

// total size: 0x3A0
class UIOnlineOverallRankings : public MenuScreen {
  public:
    UIOnlineOverallRankings(ScreenConstructorData *sd);
    ~UIOnlineOverallRankings() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    int32 PopulateBlackList();
    void BuildBlackListSlots();
    void ScrollBlackList(int direction);
    void ClearRanks(int first, int last);
    void GetRanks();
    int32 PopulateOverallRankings(bool keepHighlight);
    static void GetRankCB(LobbyApiMsgT *msg, void *context);
    static void UserStatsCB(LobbyApiMsgT *msg, void *context);
    void Setup();

    static const int m_ranksPerPage;
    static const int m_NumberOfRankingSlots;

    int raceMode;                     // offset 0x2C, size 0x4
    RankListSubcategoryE subCategory; // offset 0x30, size 0x4
    RankListE currentRankList;        // offset 0x34, size 0x4
    int32 commandID;                  // offset 0x38, size 0x4
    PlayerDataT myUserStats;          // offset 0x3C, size 0x200
    dialog_handle msgHandle;          // offset 0x23C, size 0x4
    bool m_complete;                  // offset 0x240, size 0x4
    int m_eventHash[3];               // offset 0x244, size 0xC
    int m_currentEvent;               // offset 0x250, size 0x4
    Scrollerina m_blackListScroller;  // offset 0x254, size 0xC8
    FEScrollBar m_vertScroller;       // offset 0x31C, size 0x64
    int m_PositionHandleIndex;        // offset 0x380, size 0x4
    bool m_blackListSlotsBuilt;       // offset 0x384, size 0x4
    ScrollerSlot *m_firstSlot;        // offset 0x388, size 0x4
    ScrollerSlot *m_lastSlot;         // offset 0x38C, size 0x4
    int m_usedCount;                  // offset 0x390, size 0x4
    int m_listStart;                  // offset 0x394, size 0x4
    int m_msgCode;                    // offset 0x398, size 0x4
    int m_weekOffset;                 // offset 0x39C, size 0x4
};

// Definidos en UIOnlineMainMenu.cpp y en uiOnlinePersonalRankings.cpp (gOtherPlayerName es static alli: la unidad zFe
// los ve porque los incluye antes).
extern bool EnterRankingsMenu;
extern bool bShowPersonalStatsForOtherPlayer;
extern char gOtherPlayerName[16];

MenuScreen *CreateOnlineOverallRankings(ScreenConstructorData *sd) {
    return new ("UIOnlineOverallRankings", 0) UIOnlineOverallRankings(sd);
}

const int UIOnlineOverallRankings::m_ranksPerPage = 12;
const int UIOnlineOverallRankings::m_NumberOfRankingSlots = 12;

UIOnlineOverallRankings::UIOnlineOverallRankings(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , raceMode(0) //
    , subCategory(RANKLIST_BY_RATING) //
    , currentRankList(RL_INVALID) //
    , commandID(-1) //
    , msgHandle(0) //
    , m_complete(false) //
    , m_currentEvent(0) //
    , m_blackListScroller(GetPackageName(), nullptr, nullptr, true, true, false, true) //
    , m_vertScroller(GetPackageName(), "ScrollBar", true, false, false) //
    , m_PositionHandleIndex(1) //
    , m_blackListSlotsBuilt(false) //
    , m_usedCount(0) //
    , m_listStart(0) //
    , m_weekOffset(0) {
    int32 rc;

    m_eventHash[0] = 0x034fa2c1;
    m_eventHash[1] = 0xb94fd70e;
    m_eventHash[2] = 0x6f547e4c;

    FEngSetLanguageHash(GetPackageName(), 0xca5bcc10, 0xce678ad3);
    FEngSetLanguageHash(GetPackageName(), 0x001450dd, 0xe9a7da00);
    ClearRanks(1, 12);
    Setup();

    rc = LobbyUsers::Instance().GetMyUserStats(myUserStats, UserStatsCB, this);
    if (rc < 0) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364, 0x911ab364,
                                       GetLocalizedString(0x6cf6e689));
        return;
    }

    if (rc == 0) {
        GetRanks();
    }

    if (PopulateOverallRankings(false) > 0) {
        msgHandle = DialogInterface::ShowMessage(GetPackageName(), "", dialog_alert, GetLocalizedString(0xec3905c1));
    }
}

UIOnlineOverallRankings::~UIOnlineOverallRankings() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }

    LobbyCore::Instance().AbortAllCommands();
    LobbyRanks::Instance().ClearCache();
}

void UIOnlineOverallRankings::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {
        case 0xc519bfc3:
            ClearRanks(1, 12);
            switch (m_weekOffset) {
                case 0:
                    FEngSetLanguageHash(GetPackageName(), 0x001450dd, 0xb90429ea);
                    m_weekOffset = 1;
                    break;
                case 1:
                    FEngSetLanguageHash(GetPackageName(), 0x001450dd, 0xefbe5784);
                    m_weekOffset = 2;
                    break;
                case 2:
                    FEngSetLanguageHash(GetPackageName(), 0x001450dd, 0xe9a7da00);
                    m_weekOffset = 0;
                    break;
            }
            m_complete = false;
            m_listStart = 0;
            GetRanks();
            break;
        case 0x406415e3:
            if (m_usedCount) {
                ScrollerDatum *datum = m_blackListScroller.GetSelectedDatum();

                bStrCpy(gOtherPlayerName, datum->Strings.GetHead()->String);
                bShowPersonalStatsForOtherPlayer = true;
                cFEng::Get()->QueuePackagePush("OL_Rankings_Personal.fng", 0, 0, false);
            }
            break;
        case 0x911ab364:
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), FEngFindObject(GetPackageName(), 0xf015108a));
            break;
        case 0xe1fde1d1:
            EnterRankingsMenu = true;
            cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
            break;
        case 0xd9feec59:
            raceMode++;
            if (raceMode > 2) {
                raceMode = 0;
            }
            m_complete = false;
            GetRanks();
            break;
        case 0x5073ef13:
            raceMode--;
            if (raceMode < 0) {
                raceMode = 2;
            }
            m_complete = false;
            GetRanks();
            break;
        case 0x911c0a4b:
            ScrollBlackList(1);
            break;
        case 0x72619778:
            ScrollBlackList(-1);
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
    }
}

void UIOnlineOverallRankings::ClearRanks(int first, int last) {
    if (first < 1 || last > 12 || first > last) {
        return;
    }

    for (int i = first; i <= last; i++) {
        ScrollerDatum *datum = new ("ScrollerDatum", 0) ScrollerDatum;
        datum->AddData("", 0);
        m_blackListScroller.AddData(datum);
        m_blackListScroller.Disable(datum);

        FEPrintf(GetPackageName(), FEngHashString("RANK_%d", i), " ");
        FEPrintf(GetPackageName(), FEngHashString("GAMERTAG_%d", i), " ");
        FEPrintf(GetPackageName(), FEngHashString("COLUMN3_%d", i), " ");
    }
}

void UIOnlineOverallRankings::GetRanks() {
    LobbyRanks &lobbyRanks = LobbyRanks::Instance();

    currentRankList = LobbyRanks::GetRankListFromMode(static_cast<OnlineRaceModeE>(raceMode), subCategory, m_weekOffset);

    FEngSetLanguageHash(GetPackageName(), 0x3d2937d5, m_eventHash[raceMode]);
    switch (raceMode) {
        case 0:
            FEngSetTextureHash(GetPackageName(), 0x1166c783, 0xe9638d3e);
            break;
        case 1:
            FEngSetTextureHash(GetPackageName(), 0x1166c783, 0x2521e5eb);
            break;
        case 2:
            FEngSetTextureHash(GetPackageName(), 0x1166c783, 0xaaab31e9);
            break;
    }

    LobbyCore::Instance().AbortCommand(GetRankCB, true);

    commandID = lobbyRanks.GetRanks(currentRankList, GetRankCB, this);
    if (commandID < 0) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364, 0x911ab364,
                                       GetLocalizedString(0x6cf6e689));
    } else if (commandID == 0) {
        PopulateOverallRankings(false);
    } else {
        msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "animating", dialog_alert, 0x1a294dad, 0x00ca4ce1,
                                                   GetLocalizedString(0x12df9f4f));
    }
}

void UIOnlineOverallRankings::ScrollBlackList(int direction) {
    ScrollerSlot *pslot;
    int currentIndex;
    int numRanks;
    char sztemp[32];

    if (m_usedCount) {
        pslot = m_blackListScroller.GetSelectedSlot();
        currentIndex = m_blackListScroller.GetSelectedNodeIndex();

        LobbyRanks &lobbyRanks = LobbyRanks::Instance();
        numRanks = lobbyRanks.GetRankListSize(currentRankList, RLT_TOP100);

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
                PopulateOverallRankings(false);
                m_blackListScroller.SetSelected(pslot);
                return;
            } else {
                m_blackListScroller.ScrollPrev();
            }
        } else if (direction > 0) {
            m_PositionHandleIndex++;
            if (m_PositionHandleIndex >= numRanks) {
                m_PositionHandleIndex = numRanks;
            }
            currentIndex++;
            if (currentIndex > 12) {
                if (m_usedCount > 12) {
                    m_listStart++;
                    PopulateOverallRankings(false);
                    m_blackListScroller.SetSelected(pslot);
                }
            } else {
                m_blackListScroller.ScrollNext();
            }
        }

        m_blackListScroller.Update(true);
        m_blackListScroller.HighlightSelected();

        currentIndex = m_blackListScroller.GetSelectedNodeIndex();
        FEngSNPrintf(sztemp, 32, "POS%d", currentIndex);
        FEngSetScript(GetPackageName(), 0x06745352, FEHashUpper(sztemp), true);
        m_vertScroller.Update(1, numRanks, m_PositionHandleIndex, -1);
    }
}

int32 UIOnlineOverallRankings::PopulateOverallRankings(bool keepHighlight) {
    LobbyRanks &lobbyRanks = LobbyRanks::Instance();
    LobbyApiRankT *myrank;
    int numRanks;

    m_blackListScroller.DeleteScrollerData();
    m_usedCount = 0;

    if (currentRankList == RL_INVALID) {
        return 0;
    }

    int i = 0;

    for (; i < 13; i++) {
        LobbyApiRankT *rank = lobbyRanks.GetRankData(currentRankList, RLT_TOP100, i + m_listStart);
        if (!rank) {
            break;
        }

        ScrollerDatum *datum = new ("ScrollerDatum", 0) ScrollerDatum;
        RankingDataT rankData;

        TagFieldGetStructure(rank->stat, &rankData, sizeof(RankingDataT), "l*");

        if (rank->rank > 15) {
            break;
        }

        FEPrintf(GetPackageName(), FEngHashString("RANK_%d", i + 1), "%d", rank->rank);
        FEPrintf(GetPackageName(), FEngHashString("GAMERTAG_%d", i + 1), "%s", rank->name);
        FEPrintf(GetPackageName(), FEngHashString("COLUMN3_%d", i + 1), "%d", rank->pts);
        datum->AddData(rank->name, 0);

        m_blackListScroller.AddData(datum);
        m_blackListScroller.Enable(datum);
        m_usedCount++;
    }

    ClearRanks(i + 1, 12);

    myrank = lobbyRanks.GetRankData(currentRankList, RLT_PERSONAL, 0);

    RankingDataT myrankData;

    if (myrank) {
        TagFieldGetStructure(myrank->stat, &myrankData, sizeof(RankingDataT), "l*");

        FEPrintf(GetPackageName(), 0x0e01061f, "%d", myrank->rank);
        FEPrintf(GetPackageName(), 0x92d8775b, myrank->name);
        FEPrintf(GetPackageName(), 0x72346254, "%d", myrank->pts);
    } else {
        FEPrintf(GetPackageName(), 0x0e01061f, "--");
        FEPrintf(GetPackageName(), 0x92d8775b, FEDatabase->GetOnlineSettings()->GetLobbyPersona());
        FEPrintf(GetPackageName(), 0x72346254, "%$d", 0);
    }

    if (m_usedCount) {
        if (!keepHighlight) {
            m_blackListScroller.UnHighlightSelected();
            m_blackListScroller.Reset(true);
            ScrollBlackList(0);
        }
        m_blackListScroller.HighlightSelected();
    }

    numRanks = lobbyRanks.GetRankListSize(currentRankList, RLT_TOP100);
    m_blackListScroller.Update(true);
    m_vertScroller.Update(1, numRanks, m_PositionHandleIndex, -1);

    return 0;
}

void UIOnlineOverallRankings::GetRankCB(LobbyApiMsgT *msg, void *context) {
    UIOnlineOverallRankings *overallRankings = static_cast<UIOnlineOverallRankings *>(context);

    DialogInterface::DismissDialog(overallRankings->msgHandle);
    overallRankings->msgHandle = 0;

    overallRankings->commandID = -1;

    if (msg->code == 0) {
        overallRankings->PopulateOverallRankings(false);
    } else {
        DialogInterface::ShowOneButton(overallRankings->GetPackageName(), "", dialog_alert, 0x417b2601, 0x911ab364,
                                       0x911ab364, GetLocalizedString(0x6cf6e689));
    }
}

void UIOnlineOverallRankings::UserStatsCB(LobbyApiMsgT *msg, void *context) {
    UIOnlineOverallRankings *overallRankings = static_cast<UIOnlineOverallRankings *>(context);

    overallRankings->m_msgCode = msg->code;
    cFEng::Get()->QueueGameMessage(0x6b7baa6f, overallRankings->GetPackageName(), 0xff);
}

void UIOnlineOverallRankings::Setup() {
    char sztemp[32];

    FEngSetLanguageHash(GetPackageName(), 0x7326e44c, 0xb90429ea);

    for (int i = 0; i < 12; i++) {
        ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;

        FEngSNPrintf(sztemp, 32, "GAMERTAG_%d", i + 1);
        FEString *str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pslot->AddData(str);

        m_blackListScroller.AddSlot(pslot);
    }
}
