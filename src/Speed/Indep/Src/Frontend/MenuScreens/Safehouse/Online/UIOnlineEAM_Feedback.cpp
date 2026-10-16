#include "Speed/Indep/Src/FEng/FEngStandard.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

// Definido en UIOnlineChatDialog.cpp.
extern char gScreenCallingFeedback[32];

// total size: 0x104
class UIOnlineEAM_Feedback : public MenuScreen {
  public:
    UIOnlineEAM_Feedback(ScreenConstructorData *sd);
    ~UIOnlineEAM_Feedback() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup();
    void ScrollOptions(int direction);
    void OptionAction();
    void SubmitComplaintCB(LobbyApiMsgT *msg);

    dialog_handle msgHandle;      // offset 0x2C, size 0x4
    bool m_waitcomplete;          // offset 0x30, size 0x4
    Scrollerina m_optionScroller; // offset 0x34, size 0xC8
    bool m_exitToParent;          // offset 0xFC, size 0x4
    int m_returnCode;             // offset 0x100, size 0x4
};

int kFEEDBACK_DESCRIPTIONS[8] = {0, 0xc33d7ae7, 0xdc764d91, 0xa403c822, 0xc4c7edbe, 0xe3602354, 0x10811591, 0x99d8bae0};
int kFEEDBACK_LABELS[8] = {0, 0xc91d620f, 0xa5b53f2b, 0x0a276384, 0xa0c5d893, 0xdbff7776, 0x1c775494, 0x017b2522};

MenuScreen *CreateUIOnlineEAM_Feedback(ScreenConstructorData *sd) {
    return new ("UIOnlineEAM_Feedback", 0) UIOnlineEAM_Feedback(sd);
}

template <void (UIOnlineEAM_Feedback::*Callback)(LobbyApiMsgT *)> struct EAMLobbyCmd_callback {
    static void call(LobbyApiMsgT *msg, void *context) {
        (static_cast<UIOnlineEAM_Feedback *>(context)->*Callback)(msg);
    }
};

UIOnlineEAM_Feedback::UIOnlineEAM_Feedback(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , msgHandle(0) //
    , m_waitcomplete(false) //
    , m_optionScroller(GetPackageName(), "ScrollBar", "ScrollBar", true, false, false, true) //
    , m_exitToParent(false) //
    , m_returnCode(0) {
    FEngSetLanguageHash(GetPackageName(), 0xb2980bb0, 0x3e10eab2);
    Setup();
    FEPrintf(GetPackageName(), 0x42aa5788, "%s", gBuddySettings.buddyName);
    ScrollOptions(0);
    m_optionScroller.GetScrollBarPointer()->SetArrow1Visibility(true);
    m_optionScroller.GetScrollBarPointer()->SetArrow2Visibility(true);
}

UIOnlineEAM_Feedback::~UIOnlineEAM_Feedback() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
}

void UIOnlineEAM_Feedback::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (m_waitcomplete) {
        switch (msg) {
            case 0x35f8620b:
            case 0x34dc1bec:
            case 0xc98356ba:
            case 0xe1fde1d1:
                break;
            default:
                return;
        }
    }

    switch (msg) {
        case 0x34dc1bec:
            FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
            break;
        case 0xc98356ba:
            if (m_exitToParent) {
                cFEng::Get()->QueuePackagePop(1);
                m_exitToParent = false;
                if (bStrCmp(gScreenCallingFeedback, "") != 0) {
                    bStrCpy(gScreenCallingFeedback, "");
                }
                cFEng::Get()->QueueGameMessage(0x7c42d6db, "OL_FriendDialogue.fng", 0xff);
            }
            break;
        case 0xe1fde1d1:
            m_exitToParent = true;
            break;
        case 0x911ab364:
            m_waitcomplete = true;
            FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
            break;
        case 0x911c0a4b:
            ScrollOptions(1);
            break;
        case 0x72619778:
            ScrollOptions(-1);
            break;
        case 0x0c407210:
        case 0x406415e3:
            OptionAction();
            break;
        case 0x4b52b8a1:
            cFEng::Get()->QueueGameMessage(0x0226ec1f, GetPackageName(), 0xff);
            break;
        case 0x0226ec1f:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }
            if (m_returnCode) {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                               GetLocalizedString(0x04984589));
            } else {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                               GetLocalizedString(0xa19617bd));
            }
            break;
        case 0x35f8620b:
        case 0x9120409e:
        case 0xb5971bf1:
            break;
    }
}

void UIOnlineEAM_Feedback::ScrollOptions(int direction) {
    if (direction < 0) {
        m_optionScroller.ScrollPrev();
    } else if (direction > 0) {
        m_optionScroller.ScrollNext();
    }

    int index = m_optionScroller.GetSelectedNodeIndex();
    FEPrintf(GetPackageName(), 0xb2980baf, GetLocalizedString(kFEEDBACK_DESCRIPTIONS[index]));

    m_optionScroller.HighlightSelected();
    m_optionScroller.Update(true);
    m_optionScroller.GetScrollBarPointer()->Update(7, 7, index, -1);
}

void UIOnlineEAM_Feedback::Setup() {
    char sztemp[32];

    for (int i = 0; i < 7; i++) {
        ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;

        FEngSNPrintf(sztemp, 32, "option_name_%d", i + 1);
        FEString *str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        pslot->AddData(str);

        FEngSNPrintf(sztemp, 32, "High_Light_%d", i + 1);
        pslot->SetBacking(FEngFindObject(GetPackageName(), FEHashUpper(sztemp)));

        m_optionScroller.AddSlot(pslot);
    }

    ScrollerDatum *datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0xc91d620f), 0);
    FEngPrintf(GetPackageName(), FEngHashString("option_name_1"), GetLocalizedString(0xc91d620f));
    m_optionScroller.AddData(datum);
    m_optionScroller.Enable(datum);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0xa5b53f2b), 0);
    FEngPrintf(GetPackageName(), FEngHashString("option_name_2"), GetLocalizedString(0xa5b53f2b));
    m_optionScroller.AddData(datum);
    m_optionScroller.Enable(datum);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0x0a276384), 0);
    FEngPrintf(GetPackageName(), FEngHashString("option_name_3"), GetLocalizedString(0x0a276384));
    m_optionScroller.AddData(datum);
    m_optionScroller.Enable(datum);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0xa0c5d893), 0);
    FEngPrintf(GetPackageName(), FEngHashString("option_name_4"), GetLocalizedString(0xa0c5d893));
    m_optionScroller.AddData(datum);
    m_optionScroller.Enable(datum);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0xdbff7776), 0);
    FEngPrintf(GetPackageName(), FEngHashString("option_name_5"), "%s", GetLocalizedString(0xdbff7776));
    m_optionScroller.AddData(datum);
    m_optionScroller.Enable(datum);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0x1c775494), 0);
    FEngPrintf(GetPackageName(), FEngHashString("option_name_6"), GetLocalizedString(0x1c775494));
    m_optionScroller.AddData(datum);
    m_optionScroller.Enable(datum);

    datum = new ("ScrollerDatum", 0) ScrollerDatum;
    datum->AddData(GetLocalizedString(0x017b2522), 0);
    FEngPrintf(GetPackageName(), FEngHashString("option_name_6"), GetLocalizedString(0x017b2522));
    m_optionScroller.AddData(datum);
    m_optionScroller.Enable(datum);

    m_optionScroller.HighlightSelected();
    m_optionScroller.Update(true);
    m_optionScroller.GetScrollBarPointer()->Update(7, 7, 1, -1);
}

void UIOnlineEAM_Feedback::OptionAction() {
    int index = m_optionScroller.GetSelectedNodeIndex();
    LobbyUsers::Instance().SubmitFeedback(gBuddySettings.buddyName, GetLocalizedString(kFEEDBACK_LABELS[index]),
                                          EAMLobbyCmd_callback<&UIOnlineEAM_Feedback::SubmitComplaintCB>::call, this);

    char msg[128] = "";
    FEngSNPrintf(msg, 128, GetLocalizedString(0x5566d903), gBuddySettings.buddyName);
}

void UIOnlineEAM_Feedback::SubmitComplaintCB(LobbyApiMsgT *msg) {
    m_returnCode = msg->code;
    cFEng::Get()->QueueGameMessage(0x4b52b8a1, GetPackageName(), 0xff);
}
