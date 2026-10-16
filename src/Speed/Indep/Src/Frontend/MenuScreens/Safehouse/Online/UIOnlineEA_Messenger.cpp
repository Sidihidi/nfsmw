#include "Speed/Indep/Src/FEng/FEString.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_MU_Keyboard.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineViewTrack.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"
#include "Speed/Indep/Src/Online/NetworkCore.hpp"
#include "Speed/Indep/Src/Online/VoiceCore.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"

enum eBuddyListView {
    eBUDDY_LIST_VIEW_ALL = 0,
    eBUDDY_LIST_VIEW_FRIENDS = 1,
    eBUDDY_LIST_VIEW_REQUESTS = 2,
    eBUDDY_LIST_VIEW_BLOCKED = 3,
    eBUDDY_LIST_VIEW_OPPONENTS = 4,
};

// total size: 0x1B4
class UIOnlineEA_Messenger : public MenuScreen {
  public:
    // total size: 0x30
    struct BuddySlot : public ScrollerSlot {
        int m_slotIndex; // offset 0x2C, size 0x4

        BuddySlot() {}
        ~BuddySlot() override {}
    };

    // total size: 0x40
    struct BuddyScrollerData : public ScrollerDatum {
        char buddyName[32]; // offset 0x18, size 0x20
        int m_count;        // offset 0x38, size 0x4
        bool m_isOnline;    // offset 0x3C, size 0x4

        BuddyScrollerData() : m_count(-1), m_isOnline(true) {}
        ~BuddyScrollerData() override {}
    };

    UIOnlineEA_Messenger(ScreenConstructorData *sd);
    ~UIOnlineEA_Messenger() override;

    eMenuSoundTriggers NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) override;
    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup();
    uint32 PopulateFriendList(bool init);
    void InititalizeBuddyList();
    void SearchForUser(const char *name);
    void SearchForUserCB(int op, int status);
    void UpdateBuddyCount();
    void ScrollBuddys(int direction);
    BuddySettings getThisBuddysSettings(int index);
    BuddySettings getThisBuddysSettings(const char *name);
    void ShowBuddyOptions(bool isNewBuddy);
    void AddBuddyToGroup();
    void RemoveBuddyFromGroup();
    void BuddyListChangeCB(int op, int status);
    void SetBuddyIconState(int index, BuddySettings buddySettings);
    BuddySettings GetNextBuddyFromList();
    void SetHeading();
    bool TestSearchStringLength();
    void UpdateAllBuddyIcons();
    void UpdateBuddyPresenceDisplay(BuddySettings buddySettings);

    dialog_handle msgHandle;      // offset 0x2C, size 0x4
    bool m_complete;              // offset 0x30, size 0x4
    bool m_exitToParent;          // offset 0x34, size 0x4
    int m_buddyCount;             // offset 0x38, size 0x4
    HLBBudT *m_pBuddyData;        // offset 0x3C, size 0x4
    int m_currentBuddyIndex;      // offset 0x40, size 0x4
    bool m_inKeyboardMode;        // offset 0x44, size 0x4
    Scrollerina m_buddyScroller;  // offset 0x48, size 0xC8
    bool m_haveUnreadMsgs;        // offset 0x110, size 0x4
    int m_currentBuddyListView;   // offset 0x114, size 0x4
    int m_buddyListIndex;         // offset 0x118, size 0x4
    int m_usedCount;              // offset 0x11C, size 0x4
    int m_buddyListStartIndex;    // offset 0x120, size 0x4
    bool m_enteringChallenge;     // offset 0x124, size 0x4
    bool m_haveGameInvites;       // offset 0x128, size 0x4
    FEString m_dummyString;       // offset 0x12C, size 0x78
    bool bWaitOneFrame;           // offset 0x1A4, size 0x4
    Timer m_iconUpdateTimer;      // offset 0x1A8, size 0x4
    int m_totalUnreadMsgs;        // offset 0x1AC, size 0x4
    u32 m_ControllerControlMask;  // offset 0x1B0, size 0x4
};

template <void (UIOnlineEA_Messenger::*Callback)(int, int)> struct hlb_op_callback {
    static void call(HLBApiRefT *, int op, int status, void *context) {
        (static_cast<UIOnlineEA_Messenger *>(context)->*Callback)(op, status);
    }
};

MenuScreen *CreateOnlineEA_Messenger(ScreenConstructorData *sd) {
    return new ("UIOnlineEA_Messenger", 0) UIOnlineEA_Messenger(sd);
}

UIOnlineEA_Messenger::UIOnlineEA_Messenger(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , msgHandle(0) //
    , m_complete(false) //
    , m_exitToParent(false) //
    , m_buddyCount(0) //
    , m_currentBuddyIndex(0) //
    , m_inKeyboardMode(false) //
    , m_buddyScroller(GetPackageName(), "ScrollBar", "ScrollBar", true, true, false, false) //
    , m_haveUnreadMsgs(false) //
    , m_currentBuddyListView(eBUDDY_LIST_VIEW_ALL) //
    , m_buddyListIndex(0) //
    , m_usedCount(0) //
    , m_buddyListStartIndex(0) //
    , m_enteringChallenge(false) //
    , m_haveGameInvites(false) //
    , bWaitOneFrame(false) //
    , m_totalUnreadMsgs(0) //
    , m_ControllerControlMask(0xff) {
    MessengerCreationTimer = RealTimer;
    InititalizeBuddyList();
    UpdateBuddyCount();
    Setup();
    PopulateFriendList(true);
    m_iconUpdateTimer = RealTimer;
    if (cFEng::Get()->FindPackage(g_GameRoomName)) {
        LobbyGameSessions::Instance().StartSessionChanges();
    }
}

UIOnlineEA_Messenger::~UIOnlineEA_Messenger() {
    if (BuddyCore::instance()->GetConnectedState() == connected) {
        HLBListClearGroup(BuddyCore::instance()->getApiRef());
    }
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
    ShowingEAMessenger = false;
    bEnableEAMessenger = true;
    if (cFEng::Get()->FindPackage(g_GameRoomName)) {
        LobbyGameSessions::Instance().FinishSessionChanges();
    }
}

eMenuSoundTriggers UIOnlineEA_Messenger::NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) {
    if (msg == 0x7b6b89d7) {
        if (m_usedCount > 0) {
            return maybe;
        }
        return static_cast<eMenuSoundTriggers>(-1);
    }
    return maybe;
}

void UIOnlineEA_Messenger::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (msg == 0xd15c04ec) {
        cFEng::Get()->QueuePackagePop(1);
        return;
    }

    if (EnteringChallenge) {
        if (msg != 0xc98356ba && msg != 0xe1fde1d1 && msg != 0x4fbee42a) {
            return;
        }
    }

    switch (msg) {
        case 0x4fbee42a:
            m_enteringChallenge = true;
            m_exitToParent = true;
            break;
        case 0xc98356ba:
            UpdateBuddyCount();
            if ((RealTimer - m_iconUpdateTimer).GetSeconds() > 0.5f && BuddyCore::instance()->GetConnectedState() == connected) {
                m_iconUpdateTimer = RealTimer;
                if (!NetConnStatus('plug', 0, nullptr)) {
                    BuddyCore::instance()->setNetworkCableUnpluggedFlag();
                }
                UpdateAllBuddyIcons();
            }
            if (bWaitOneFrame) {
                bWaitOneFrame = false;
                gBuddyListHasChanged = 0;
            }
            if (gBuddyListHasChanged) {
                PopulateFriendList(true);
                bWaitOneFrame = true;
            }
            if (m_exitToParent) {
                cFEng::Get()->QueuePackagePop(1);
                m_exitToParent = false;
                if (m_enteringChallenge) {
                    cFEng::Get()->QueueGameMessage(0xf764cbe7, nullptr, 0xff);
                } else {
                    cFEng::Get()->QueueGameMessage(0x0b1fb0b6, "OL_LobbyRoom.fng", 0xff);
                }
                cFEng::Get()->QueueGameMessage(0x5d1a570f, nullptr, 0xff);
            }
            break;
        case 0x35f8620b:
            PopulateFriendList(true);
            UpdateEAMIcons();
            break;
        case 0x911ab364:
            FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
            m_exitToParent = true;
            break;
        case 0xb5971bf1:
            AddBuddyToGroup();
            break;
        case 0x9120409e:
            RemoveBuddyFromGroup();
            break;
        case 0x911c0a4b:
            ScrollBuddys(1);
            break;
        case 0x72619778:
            ScrollBuddys(-1);
            break;
        case 0x0c407210:
        case 0x406415e3:
            if (!FEngFindScreen("OL_FriendDialogue.fng")) {
                m_ControllerControlMask = param1;
                ShowBuddyOptions(false);
            }
            break;
        case 0xda5b8712:
            if (TestSearchStringLength()) {
                SearchForUser(FEngGetEditedString());
            }
            m_inKeyboardMode = false;
            break;
        case 0x1c6d9c35:
            SearchForUser(PCSearchName);
        case 0xc9d30688:
            m_inKeyboardMode = false;
            break;
        // El arbol de comparaciones del switch de la A124 pide un caso vacio por encima de 0xda5b8712; el valor no queda
        // en el binario (0xe1fde1d1 es el que deja pasar el filtro de EnteringChallenge de arriba).
        case 0xe1fde1d1:
            break;
        case 0x5073ef13:
            m_currentBuddyListView--;
            if (m_currentBuddyListView < eBUDDY_LIST_VIEW_ALL) {
                m_currentBuddyListView = eBUDDY_LIST_VIEW_OPPONENTS;
            }
            PopulateFriendList(true);
            break;
        case 0xd9feec59:
            m_currentBuddyListView++;
            if (m_currentBuddyListView > eBUDDY_LIST_VIEW_OPPONENTS) {
                m_currentBuddyListView = eBUDDY_LIST_VIEW_ALL;
            }
            PopulateFriendList(true);
            break;
        case 0xc519bfc3:
            if (RealTimer.GetSeconds() - KBCreationTimer.GetSeconds() < g_KBDelaySeconds) {
                break;
            }
            if (!m_inKeyboardMode) {
                m_ControllerControlMask = param1;
                m_inKeyboardMode = true;
                FEngBeginTextInput(0, 2, "", GetLocalizedString(0x626e210c), 12);
            }
            break;
        case 100:
            ShowBuddyOptions(true);
            break;
        case 0x7a548b3f:
            if (m_usedCount) {
                UpdateAllBuddyIcons();
            }
            break;
    }
}

void UIOnlineEA_Messenger::SearchForUser(const char *name) {
    char nameToUse[16];
    char searchStr[64] = "";
    int result;

    bMemSet(nameToUse, 0, sizeof(nameToUse));
    bStrCpy(nameToUse, name);
    msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x1a294dad, 0xffffffff, 0xffffffff,
                                               GetLocalizedString(0x2f0cfb04));
    result = HLBApiFindUsers(BuddyCore::instance()->getApiRef(), nameToUse,
                             hlb_op_callback<&UIOnlineEA_Messenger::SearchForUserCB>::call, this);
    if (result) {
        DisplayHLBerrorType(GetPackageName(), result);
    }
}

void UIOnlineEA_Messenger::SearchForUserCB(int op, int status) {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
    if (status == 0) {
        BuddyApiUserT *pUser = HLBApiUserFound(BuddyCore::instance()->getApiRef(), 0);
        BuddyApiUserT *pSecondUser = HLBApiUserFound(BuddyCore::instance()->getApiRef(), 1);
        if (pUser && (pSecondUser || bStrCmp(pUser->name, FEDatabase->GetOnlineSettings()->GetLobbyPersona()))) {
            cFEng::Get()->QueuePackagePush("OLX_FindResults.fng", 0, m_ControllerControlMask, false);
        } else {
            DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0x8b207e1a));
        }
    } else {
        DisplayHLBerrorType(GetPackageName(), status);
    }
}

uint32 UIOnlineEA_Messenger::PopulateFriendList(bool init) {
    char lastSelectedBuddyName[32] = "";

    if (m_buddyScroller.GetSelectedDatum() &&
        static_cast<BuddyScrollerData *>(m_buddyScroller.GetSelectedDatum())->buddyName) {
        bStrCpy(lastSelectedBuddyName, static_cast<BuddyScrollerData *>(m_buddyScroller.GetSelectedDatum())->buddyName);
    }

    HLBListSort(BuddyCore::instance()->getApiRef());

    if (init) {
        m_buddyScroller.UnHighlightSelected();
        m_buddyListStartIndex = 0;
    }

    SetHeading();
    m_buddyScroller.DeleteScrollerData();
    m_usedCount = 0;
    m_buddyListIndex = 0;

    for (int i = m_buddyListStartIndex; i < 100; i++) {
        BuddySettings buddySettings = GetNextBuddyFromList();
        if (!buddySettings.pBuddy) {
            break;
        }

        BuddyScrollerData *thisDatum = new ("BuddyScrollerData", 0) BuddyScrollerData;
        thisDatum->AddData(buddySettings.buddyName, 0);
        thisDatum->m_count = i;
        bStrCpy(thisDatum->buddyName, buddySettings.buddyName);
        if (!buddySettings.buddyState) {
            thisDatum->m_isOnline = false;
        } else {
            thisDatum->m_isOnline = true;
        }

        if (m_usedCount < 9) {
            SetBuddyIconState(m_usedCount, buddySettings);
        }

        m_buddyScroller.AddData(thisDatum);
        m_buddyScroller.Enable(thisDatum);
        m_usedCount++;
    }

    for (int i = m_usedCount; i < 9; i++) {
        FEngSetScript(GetPackageName(), FEngHashString("STATUS_%dA", i + 1), 0x16a259, true);
        FEngSetScript(GetPackageName(), FEngHashString("STATUS_%d", i + 1), 0x16a259, true);
        FEngSetScript(GetPackageName(), FEngHashString("GROUP_FLAG_%d", i + 1), 0x16a259, true);
    }

    FEPrintf(GetPackageName(), 0xa0ed7306, "");

    if (init) {
        m_buddyScroller.Reset(true);
        if (m_usedCount) {
            m_buddyScroller.HighlightSelected();
            if (static_cast<BuddyScrollerData *>(m_buddyScroller.GetSelectedDatum())->buddyName) {
                BuddySettings buddySettings =
                    getThisBuddysSettings(static_cast<BuddyScrollerData *>(m_buddyScroller.GetSelectedDatum())->buddyName);
                UpdateBuddyPresenceDisplay(buddySettings);
            }
        }
    }

    if (m_usedCount == 0) {
        FEngSetScript(GetPackageName(), 0x06745352, 0x16a259, true);
    }

    if (m_usedCount > 0 && lastSelectedBuddyName[0]) {
        BuddyScrollerData *thisDatum = static_cast<BuddyScrollerData *>(m_buddyScroller.GetFirstDatum());
        for (uint32 x = 0; x < m_buddyScroller.GetNumData(); x++) {
            if (!bStrCmp(thisDatum->buddyName, lastSelectedBuddyName)) {
                m_buddyScroller.SetSelectedDatum(thisDatum);
                break;
            }
            thisDatum = static_cast<BuddyScrollerData *>(thisDatum->GetNext());
        }
    }

    FEPrintf(GetPackageName(), 0xb2037bdc, "%d", m_usedCount);
    ScrollBuddys(0);
    return m_buddyCount;
}

void UIOnlineEA_Messenger::InititalizeBuddyList() {
    for (int i = 1; i < 8; i++) {
        FEPrintf(GetPackageName(), FEngHashString("OPTION_NAME_%d", i), "%s", "");
        FEngSetScript(GetPackageName(), FEngHashString("STATUS_%d", i), 0x16a259, true);
    }
}

void UIOnlineEA_Messenger::UpdateBuddyCount() {
    BuddySettings pBuddySettings;

    m_buddyCount = BuddyCore::instance()->getBuddyCount();
    for (int i = 0; i < m_buddyCount; i++) {
        pBuddySettings = getThisBuddysSettings(i);
        if (pBuddySettings.unreadMsgCount) {
            m_totalUnreadMsgs++;
        }
    }
    if (!m_totalUnreadMsgs) {
        UpdateStatusIcons(0, false);
    }
}

void UIOnlineEA_Messenger::Setup() {
    char sztemp[32];

    for (int i = 0; i < 8; i++) {
        BuddySlot *pslot = new ("BuddySlot", 0) BuddySlot;

        FEngSNPrintf(sztemp, 32, "OPTION_NAME_%d", i + 1);
        FEString *str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pslot->AddData(str);

        FEngSNPrintf(sztemp, 32, "option_mouse_%d", i + 1);
        pslot->SetBacking(FEngFindObject(GetPackageName(), FEHashUpper(sztemp)));

        m_buddyScroller.AddSlot(pslot);
        pslot->m_slotIndex = i;
    }
}

void UIOnlineEA_Messenger::ScrollBuddys(int direction) {
    if (m_usedCount) {
        if (direction < 0) {
            m_buddyScroller.ScrollPrev();
        } else if (direction > 0) {
            m_buddyScroller.ScrollNext();
        }
        BuddySettings buddySettings =
            getThisBuddysSettings(static_cast<BuddyScrollerData *>(m_buddyScroller.GetSelectedDatum())->buddyName);
        UpdateBuddyPresenceDisplay(buddySettings);
        UpdateAllBuddyIcons();
    }
}

void UIOnlineEA_Messenger::UpdateAllBuddyIcons() {
    ScrollerDatum *pSelected = m_buddyScroller.GetSelectedDatum();

    for (int i = 0; i < 8; i++) {
        BuddySlot *pSlot = static_cast<BuddySlot *>(m_buddyScroller.GetSlot(i));
        if (pSlot) {
            BuddyScrollerData *pDatum = static_cast<BuddyScrollerData *>(m_buddyScroller.FindDatumInSlot(m_buddyScroller.GetSlot(i)));
            if (pDatum && pDatum->IsEnabled()) {
                SetBuddyIconState(pSlot->m_slotIndex, getThisBuddysSettings(pDatum->buddyName));
                if (pSelected != pDatum) {
                    if (pDatum->m_isOnline) {
                        FEngSetScript(GetPackageName(), FEngHashString("OPTION_NAME_%d", i + 1), 0x1ca7c0, true);
                    } else {
                        FEngSetScript(GetPackageName(), FEngHashString("OPTION_NAME_%d", i + 1), 0x163c76, true);
                    }
                }
            } else {
                FEngSetScript(GetPackageName(), FEngHashString("GROUP_FLAG_%d", i + 1), 0x16a259, true);
                FEngSetScript(GetPackageName(), FEngHashString("STATUS_%d", i + 1), 0x16a259, true);
            }
        }
    }
}

BuddySettings UIOnlineEA_Messenger::getThisBuddysSettings(int index) {
    HLBBudT *pBuddy = BuddyCore::instance()->getBuddyByIndex(index);
    return EA_Messenger_GetBuddySettings(pBuddy);
}

BuddySettings UIOnlineEA_Messenger::getThisBuddysSettings(const char *name) {
    HLBBudT *pBuddy = BuddyCore::instance()->getBuddyByName(name);
    return EA_Messenger_GetBuddySettings(pBuddy);
}

void UIOnlineEA_Messenger::ShowBuddyOptions(bool isNewBuddy) {
    if (!isNewBuddy) {
        if (!m_usedCount) {
            return;
        }
        BuddyScrollerData *pSelected = static_cast<BuddyScrollerData *>(m_buddyScroller.GetSelectedDatum());
        HLBBudT *pBuddy = BuddyCore::instance()->getBuddyByName(pSelected->buddyName);
        gBuddySettings = EA_Messenger_GetBuddySettings(pBuddy);
        HLBListFlagTempBuddy(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, HLB_TEMPBUDDYTYPE_MESG);
    } else {
        int rc = 0;

        if (FEngFindScreen(g_GameRoomName)) {
            rc = HLBListFlagTempBuddy(BuddyCore::instance()->getApiRef(), gNewBuddyName, HLB_TEMPBUDDYTYPE_MESG);
        }
        if (!rc) {
            gBuddySettings = getThisBuddysSettings(gNewBuddyName);
            bStrCpy(gBuddySettings.buddyName, gNewBuddyName);
        } else {
            DisplayHLBerrorType(GetPackageName(), rc);
            return;
        }
    }
    cFEng::Get()->QueuePackagePush("OL_FriendDialogue.fng", 0, m_ControllerControlMask, false);
}

void UIOnlineEA_Messenger::AddBuddyToGroup() {
    if (!m_usedCount) {
        return;
    }

    BuddySettings buddySettings =
        getThisBuddysSettings(static_cast<BuddyScrollerData *>(m_buddyScroller.GetSelectedDatum())->buddyName);

    if (!buddySettings.pBuddy || !buddySettings.isRealBuddy || buddySettings.isBlocked) {
        return;
    }
    if (!buddySettings.isInGroup) {
        int result = HLBListAddToGroup(BuddyCore::instance()->getApiRef(), buddySettings.buddyName);
        if (result) {
            DisplayHLBerrorType(GetPackageName(), result);
        }
        UpdateAllBuddyIcons();
    }
}

void UIOnlineEA_Messenger::RemoveBuddyFromGroup() {
    if (!m_usedCount) {
        return;
    }

    BuddySettings buddySettings =
        getThisBuddysSettings(static_cast<BuddyScrollerData *>(m_buddyScroller.GetSelectedDatum())->buddyName);

    if (!buddySettings.pBuddy || !buddySettings.isRealBuddy) {
        return;
    }
    if (buddySettings.isInGroup) {
        int result = HLBListRemoveFromGroup(BuddyCore::instance()->getApiRef(), buddySettings.buddyName);
        if (result) {
            DisplayHLBerrorType(GetPackageName(), result);
        }
        UpdateAllBuddyIcons();
    }
}

BuddySettings UIOnlineEA_Messenger::GetNextBuddyFromList() {
    BuddySettings pBuddySettings;

    switch (m_currentBuddyListView) {
        case eBUDDY_LIST_VIEW_FRIENDS:
            while (m_buddyListIndex < m_buddyCount) {
                pBuddySettings = getThisBuddysSettings(m_buddyListIndex);
                m_buddyListIndex++;
                if (pBuddySettings.isRealBuddy) {
                    return pBuddySettings;
                }
            }
            break;
        case eBUDDY_LIST_VIEW_ALL:
            if (m_buddyListIndex < m_buddyCount) {
                pBuddySettings = getThisBuddysSettings(m_buddyListIndex);
                m_buddyListIndex++;
                return pBuddySettings;
            }
            break;
        case eBUDDY_LIST_VIEW_BLOCKED:
            while (m_buddyListIndex < m_buddyCount) {
                pBuddySettings = getThisBuddysSettings(m_buddyListIndex);
                m_buddyListIndex++;
                if (pBuddySettings.isBlocked) {
                    return pBuddySettings;
                }
            }
            break;
        case eBUDDY_LIST_VIEW_REQUESTS:
            while (m_buddyListIndex < m_buddyCount) {
                pBuddySettings = getThisBuddysSettings(m_buddyListIndex);
                m_buddyListIndex++;
                if (pBuddySettings.IsIWannaBeHisBuddy || pBuddySettings.IsWannaBeMyBuddy) {
                    return pBuddySettings;
                }
            }
            break;
        case eBUDDY_LIST_VIEW_OPPONENTS:
            while (m_buddyListIndex < m_buddyCount) {
                pBuddySettings = getThisBuddysSettings(m_buddyListIndex);
                m_buddyListIndex++;
                if (pBuddySettings.isTemporary) {
                    return pBuddySettings;
                }
            }
            break;
    }

    pBuddySettings.pBuddy = nullptr;
    return pBuddySettings;
}

void UIOnlineEA_Messenger::SetHeading() {
    switch (m_currentBuddyListView) {
        case eBUDDY_LIST_VIEW_FRIENDS:
            FEPrintf(GetPackageName(), FEngHashString("OPTION_HEADER"), "%s", GetTranslatedString(0xb204477c));
            break;
        case eBUDDY_LIST_VIEW_ALL:
            FEPrintf(GetPackageName(), FEngHashString("OPTION_HEADER"), "%s", GetTranslatedString(0x8cac9d6a));
            break;
        case eBUDDY_LIST_VIEW_BLOCKED:
            FEPrintf(GetPackageName(), FEngHashString("OPTION_HEADER"), "%s", GetTranslatedString(0x7087ce45));
            break;
        case eBUDDY_LIST_VIEW_REQUESTS:
            FEPrintf(GetPackageName(), FEngHashString("OPTION_HEADER"), "%s", GetTranslatedString(0x0b1b1fc4));
            break;
        case eBUDDY_LIST_VIEW_OPPONENTS:
            FEPrintf(GetPackageName(), FEngHashString("OPTION_HEADER"), "%s", GetTranslatedString(0xf155ab34));
            break;
    }
}

void UIOnlineEA_Messenger::SetBuddyIconState(int index, BuddySettings buddySettings) {
    uint32 voipImageHash;
    uint32 statusImageHash;
    uint32 flagHash;
    bool showVoipImage = false;
    bool showStatusImage;
    bool bBuddySpeaking;

    voipImageHash = FEngHashString("STATUS_%dA", index + 1);
    statusImageHash = FEngHashString("STATUS_%d", index + 1);
    flagHash = FEngHashString("GROUP_FLAG_%d", index + 1);

    FEngSetScript(GetPackageName(), voipImageHash, 0x16a259, true);
    FEngSetScript(GetPackageName(), statusImageHash, 0x16a259, true);
    FEngSetScript(GetPackageName(), flagHash, 0x16a259, true);

    showStatusImage = false;

    if (buddySettings.buddyName[0]) {
        FEngSetTextureHash(GetPackageName(), voipImageHash, FEHashUpper("OL_HEADSET"));
        bBuddySpeaking = false;
        if (buddySettings.voipState) {
            int channel = 0;
            if (VoiceCore::instance().IsInVOIPChat(buddySettings.buddyName, &channel)) {
                if (VoiceCore::instance().IsSpeaking(channel)) {
                    bBuddySpeaking = true;
                }
            }
            showVoipImage = true;
        }
        if (VoiceCore::instance().IsMuted(buddySettings.buddyName)) {
            FEngSetTextureHash(GetPackageName(), voipImageHash, FEHashUpper("OL_HEADSET_MUTE"));
            showVoipImage = true;
        } else if (bBuddySpeaking) {
            FEngSetTextureHash(GetPackageName(), voipImageHash, FEHashUpper("OL_VOICE"));
            showVoipImage = true;
        }

        if (buddySettings.isInGroup) {
            FEngSetScript(GetPackageName(), flagHash, 0x1ca7c0, true);
        }

        if (buddySettings.IsIWannaBeHisBuddy) {
            FEngSetTextureHash(GetPackageName(), statusImageHash, FEHashUpper("OL_FRIEND_SENT"));
            showStatusImage = true;
        }
        if (buddySettings.unreadMsgCount) {
            FEngSetTextureHash(GetPackageName(), statusImageHash, FEHashUpper("OL_MAIL"));
            showStatusImage = true;
            m_haveUnreadMsgs = true;
        }
        if (buddySettings.IsWannaBeMyBuddy) {
            FEngSetTextureHash(GetPackageName(), statusImageHash, FEHashUpper("OL_FRIEND"));
            showStatusImage = true;
        }
        if (buddySettings.IWannaPlayBuddy || (buddySettings.gameInviteFlags & 0x20000)) {
            FEngSetTextureHash(GetPackageName(), statusImageHash, FEHashUpper("OL_CHALLENGE_SENT"));
            showStatusImage = true;
        }

        if (!FEDatabase->GetOnlineSettings()->AppearOffline) {
            if (buddySettings.BuddyWantsToPlay) {
                FEngSetTextureHash(GetPackageName(), statusImageHash, FEHashUpper("OL_CHALLENGE"));
                showStatusImage = true;
                m_haveGameInvites = true;
            }
            if ((buddySettings.gameInviteFlags & 0x400000) && !buddySettings.IsSameProduct) {
                FEngSetTextureHash(GetPackageName(), statusImageHash, FEHashUpper("OL_CHALLENGE"));
                showStatusImage = true;
                m_haveGameInvites = true;
            }
            if (gVOIP_InviteState == 2 && BuddyCore::instance()->DoIHaveAVOIPInviteFromThisBuddy(buddySettings.buddyName)) {
                FEngSetTextureHash(GetPackageName(), voipImageHash, FEHashUpper("OL_VOICE_REQUEST"));
                showVoipImage = true;
                m_haveGameInvites = true;
            }
        } else {
            if (buddySettings.gameInviteFlags & 0x400000) {
                FEngSetTextureHash(GetPackageName(), statusImageHash, FEHashUpper("OL_CHALLENGE"));
                showStatusImage = true;
                m_haveGameInvites = true;
            }
        }

        if (buddySettings.isBlocked) {
            FEngSetTextureHash(GetPackageName(), statusImageHash, FEHashUpper("OL_BLOCKED"));
            showStatusImage = true;
        }

        if (showVoipImage && buddySettings.IsSameProduct) {
            FEngSetScript(GetPackageName(), voipImageHash, 0x1ca7c0, true);
        }
        if (showStatusImage) {
            FEngSetScript(GetPackageName(), statusImageHash, 0x1ca7c0, true);
        }
    }
}

bool UIOnlineEA_Messenger::TestSearchStringLength() {
    if (bStrLen(FEngGetEditedString()) < 3 || *FEngGetEditedString() == ' ') {
        DialogInterface::ShowOneButton("", "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff, GetLocalizedString(0xd1f8f281));
        return false;
    }
    return true;
}

void UIOnlineEA_Messenger::UpdateBuddyPresenceDisplay(BuddySettings buddySettings) {
    const char *pTitle = HLBBudGetTitle(buddySettings.pBuddy);

    if (!buddySettings.buddyState) {
        FEPrintf(GetPackageName(), 0xa0ed7306, GetLocalizedString(0x221e85b4));
        return;
    }

    RichPresenceMsg rpMsg;
    rpMsg.Default();
    rpMsg = BuddyCore::instance()->GetRichPresence(&buddySettings);

    char presStr[128];
    char richPresStr[128];

    if (buddySettings.isRealBuddy && rpMsg.session[0] && !rpMsg.isSessionPrivate) {
        if (rpMsg.isRacing) {
            GRaceParameters *buddysRace = GRaceDatabase::Get().GetRaceFromHash(rpMsg.eventHash);
            if (buddysRace) {
                bSPrintf(richPresStr, GetLocalizedString(0x36915b3e), buddysRace->GetEventID());
            } else {
                bSPrintf(richPresStr, " ");
            }
        } else {
            bSPrintf(richPresStr, GetLocalizedString(0x56a46b33));
        }
    } else {
        bSPrintf(richPresStr, " ");
    }

    if (buddySettings.isPassive) {
        bSPrintf(presStr, GetLocalizedString(0x679fd4e9), pTitle);
    } else if (pTitle) {
        bSPrintf(presStr, GetLocalizedString(0x8d3ee1af), pTitle);
    } else {
        bSPrintf(presStr, GetLocalizedString(0x02d32a79));
    }

    FEPrintf(GetPackageName(), 0xa0ed7306, "%s %s", presStr, richPresStr);
}
