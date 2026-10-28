#include "Speed/Indep/Src/FEng/FEList.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"

// total size: 0x100
class UIOnlineSearchResult : public MenuScreen {
    // total size: 0x3C
    class ResultScrollerData : public ScrollerDatum {
      public:
        ResultScrollerData() {
            m_count = 0;
        }
        ~ResultScrollerData() override {}

        char m_name[32]; // offset 0x18, size 0x20
        int m_count;     // offset 0x38, size 0x4
    };

  public:
    UIOnlineSearchResult(ScreenConstructorData *sd);
    ~UIOnlineSearchResult() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup();
    void PopulateList();
    char *GetBuddysFound(int index);
    int GetBuddysNumFound();

    bool m_exitToParent;          // offset 0x2C, size 0x4
    Scrollerina m_resultScroller; // offset 0x30, size 0xC8
    int m_listStart;              // offset 0xF8, size 0x4
    int m_usedCount;              // offset 0xFC, size 0x4
};

MenuScreen *CreateUIOnlineSearchResult(ScreenConstructorData *sd) {
    return new ("UIOnlineSearchResult", 0) UIOnlineSearchResult(sd);
}

UIOnlineSearchResult::UIOnlineSearchResult(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , m_exitToParent(false) //
    , m_resultScroller(GetPackageName(), "Scrollbar", "Scrollbar", true, true, false, true) //
    , m_listStart(0) //
    , m_usedCount(0) {
    gNewBuddyName = nullptr;

    Setup();
    PopulateList();
}

UIOnlineSearchResult::~UIOnlineSearchResult() {}

void UIOnlineSearchResult::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {
        case 0xc98356ba:
            BuddyCore::instance()->doprocessing();
            if (m_exitToParent) {
                cFEng::Get()->QueuePackagePop(1);
                m_exitToParent = false;
            }
            break;
        case 0x35f8620b:
            m_resultScroller.HighlightSelected();
            break;
        case 0xe1fde1d1:
            cFEng::Get()->QueuePackagePop(1);
            if (gNewBuddyName) {
                cFEng::Get()->QueueGameMessage(100, "OL_EAMessenger.fng", 0xff);
            }
            break;
        case 0x911ab364:
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            break;
        case 0x911c0a4b:
            m_resultScroller.ScrollNext();
            break;
        case 0x72619778:
            m_resultScroller.ScrollPrev();
            break;
        case 0x406415e3:
            if (m_usedCount) {
                gNewBuddyName = static_cast<ResultScrollerData *>(m_resultScroller.GetSelectedDatum())->m_name;
            }
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            break;
    }
}

void UIOnlineSearchResult::Setup() {
    char sztemp[32];
    int numSlots;

    numSlots = GetBuddysNumFound();
    if (numSlots > 8) {
        numSlots = 8;
    }

    for (int i = 0; i < numSlots; i++) {
        ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;

        FEngSNPrintf(sztemp, 32, "option_%d", i + 1);
        FEString *str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pslot->AddData(str);

        m_resultScroller.AddSlot(pslot);
    }
}

void UIOnlineSearchResult::PopulateList() {
    char *pName;
    int BuddyIter;
    int numBuddies;
    bool bFinished;
    char sztemp[32];

    m_resultScroller.DeleteScrollerData();

    m_usedCount = 0;
#ifdef EA_PLATFORM_XENON
    BuddyIter = 0;
    numBuddies = GetBuddysNumFound();
    bFinished = false;
#else
    bFinished = false;
    BuddyIter = 0;
    numBuddies = GetBuddysNumFound();
#endif

    while (m_usedCount < numBuddies && !bFinished) {
        pName = GetBuddysFound(BuddyIter + m_listStart);

        if (pName) {
            if (bStrCmp(pName, FEDatabase->GetOnlineSettings()->GetLobbyPersona())) {
                ResultScrollerData *datum = new ("ResultScrollerData", 0) ResultScrollerData;

                FEngSNPrintf(sztemp, 32, "option_%d", m_usedCount + 1);
                FEPrintf(GetPackageName(), FEHashUpper(sztemp), "%s", pName);
                datum->AddData(pName, 0);
                datum->m_count = m_usedCount;
                bStrCpy(datum->m_name, pName);

                m_resultScroller.AddData(datum);
                m_resultScroller.Enable(datum);

                m_usedCount++;
            }
            BuddyIter++;
        } else {
            bFinished = true;
        }
    }

    for (int i = m_usedCount; i < 8; i++) {
        FEngSNPrintf(sztemp, 32, "OPTION_%d", i + 1);
        FEngSetInvisible(GetPackageName(), FEHashUpper(sztemp));
    }

    if (m_usedCount) {
        m_resultScroller.Reset(true);
        m_resultScroller.HighlightSelected();
    }

    m_resultScroller.Update(true);
}

char *UIOnlineSearchResult::GetBuddysFound(int index) {
    return HLBApiUserFound(BuddyCore::instance()->getApiRef(), index)->name;
}

int UIOnlineSearchResult::GetBuddysNumFound() {
    int numBuddies = 0;

    while (GetBuddysFound(numBuddies)) {
        numBuddies++;
    }

    return numBuddies;
}
