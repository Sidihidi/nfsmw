#include <string.h>

#include "Speed/Indep/Src/FEng/FEngStandard.h"
#include "Speed/Indep/Src/FEng/FEString.h"
#include "Speed/Indep/Src/FEng/FEWideString.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngFont.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_MU_Keyboard.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/NetworkCore.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

// total size: 0x1390
class UIOnlineChatDialog : public MenuScreen {
  public:
    UIOnlineChatDialog(ScreenConstructorData *sd);
    ~UIOnlineChatDialog() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup();
    static void _DoWithStackBuddyMessage(UIOnlineChatDialog *pThis, const char *message) {
        pThis->_SendBuddyMessage(message);
    }
    void _SendBuddyMessage(const char *message);
    void SendBuddyMessage(const char *message) {
        DoWithMainMemoryStack((void *)_DoWithStackBuddyMessage, (int)this, (int)message);
    }
    void SendBuddyMessageCB(int op, int status);
    void ReadBuddyMessage();
    BuddySettings getThisBuddysSettings(int);
    void AddNewBuddyMsgs();
    void Receive(char *message);
    int WordWrap(char *message);
    void AddLine(char *message, int size);
    void Display();
    void ScrollChatBodyUp();
    void ScrollChatBodyDown();

    dialog_handle msgHandle;           // offset 0x2C, size 0x4
    bool m_complete;                   // offset 0x30, size 0x4
    bool m_exitToParent;               // offset 0x34, size 0x4
    int m_buddyCount;                  // offset 0x38, size 0x4
    HLBBudT *m_pBuddyData;             // offset 0x3C, size 0x4
    int m_currentBuddyIndex;           // offset 0x40, size 0x4
    bool m_inKeyboardMode;             // offset 0x44, size 0x4
    Scrollerina m_msgBodyScroller;     // offset 0x48, size 0xC8
    FEScrollBar m_ScrollBar;           // offset 0x110, size 0x64
    char m_strBuf[4096];               // offset 0x174, size 0x1000
    FEString *m_pChatBody;             // offset 0x1174, size 0x4
    FEngFont *m_pFont;                 // offset 0x1178, size 0x4
    int m_Width;                       // offset 0x117C, size 0x4
    int m_Flags;                       // offset 0x1180, size 0x4
    int m_Scroll;                      // offset 0x1184, size 0x4
    int m_Lines;                       // offset 0x1188, size 0x4
    Utf8EncodeTblT m_ASCIIToUTF8Table; // offset 0x118C, size 0x200
    bool m_waitingForCB;               // offset 0x138C, size 0x4
};

char gScreenCallingFeedback[32] = "";

template <void (UIOnlineChatDialog::*Callback)(int, int)> struct hlb_onlinechat_op_callback {
    static void call(HLBApiRefT *, int op, int status, void *context) {
        (static_cast<UIOnlineChatDialog *>(context)->*Callback)(op, status);
    }
};

MenuScreen *CreateUIOnlineChatDialog(ScreenConstructorData *sd) {
    return new ("UIOnlineChatDialog", 0) UIOnlineChatDialog(sd);
}

UIOnlineChatDialog::UIOnlineChatDialog(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , msgHandle(0) //
    , m_complete(false) //
    , m_exitToParent(false) //
    , m_inKeyboardMode(false) //
    , m_msgBodyScroller(GetPackageName(), "scroll_area", "junk", true, true, false, true) //
    , m_ScrollBar(GetPackageName(), "scrollbar", true, false, false) //
    , m_pChatBody(FEngFindString(GetPackageName(), 0x6d851451)) //
    , m_pFont(FindFont(m_pChatBody->Handle)) //
    , m_Width(m_pChatBody->MaxWidth) //
    , m_Flags(m_pChatBody->Flags) //
    , m_Scroll(12) //
    , m_Lines(0) //
    , m_waitingForCB(false) {
    bMemSet(m_strBuf, 0, 4096);

    Setup();

    HLBBudT *pBud = BuddyCore::instance()->getBuddyByName(gBuddySettings.buddyName);
    if (pBud) {
        gBuddySettings = EA_Messenger_GetBuddySettings(pBud);
    } else {
        HLBListFlagTempBuddy(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, 1);
        pBud = BuddyCore::instance()->getBuddyByName(gBuddySettings.buddyName);
        gBuddySettings = EA_Messenger_GetBuddySettings(pBud);
    }

    if (HLBMsgListGetUnreadCount(gBuddySettings.pBuddy)) {
        ReadBuddyMessage();
    } else {
        m_inKeyboardMode = true;
        FEngBeginTextInput(FEngHashString("Message_body"), 0, "", GetLocalizedString(0x9a2e5f7b), 116);
    }

    FEngPrintf(GetPackageName(), "Header", GetLocalizedString(0x53f9fd5c));

    for (int i = 0; i < 256; i++) {
        m_ASCIIToUTF8Table.uCodeTbl[i] = i;
    }
}

UIOnlineChatDialog::~UIOnlineChatDialog() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
}

void UIOnlineChatDialog::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {
        case 0xc98356ba:
            if (m_exitToParent && cFEng::Get()->IsPackageInControl(GetPackageName())) {
                cFEng::Get()->QueuePackagePop(1);
                m_exitToParent = false;
            }
            if (BuddyCore::instance()->GetConnectedState() == connected) {
                HLBBudT *pBud = BuddyCore::instance()->getBuddyByName(gBuddySettings.buddyName);
                if (pBud) {
                    if (HLBMsgListGetUnreadCount(pBud)) {
                        AddNewBuddyMsgs();
                    }
                }
            }
            break;
        case 0xe1fde1d1:
            if (BuddyCore::instance()->GetConnectedState() == connected) {
                HLBBudT *pBud = BuddyCore::instance()->getBuddyByName(gBuddySettings.buddyName);
                if (pBud) {
                    gBuddySettings = EA_Messenger_GetBuddySettings(pBud);
                }
                HLBMsgListDeleteAll(gBuddySettings.pBuddy, 1);
                UpdateStatusIcons(0, false);
                gBuddyListHasChanged = true;
            }
            m_exitToParent = true;
            break;
        case 0x911ab364:
            if (m_waitingForCB) {
                HLBApiCancelOp(BuddyCore::instance()->getApiRef());
                m_waitingForCB = false;
            }
            FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
            break;
        case 0x911c0a4b:
            ScrollChatBodyDown();
            break;
        case 0x72619778:
            ScrollChatBodyUp();
            break;
        case 0x406415e3:
            if ((RealTimer - KBCreationTimer).GetSeconds() < g_KBDelaySeconds) {
                return;
            }
            m_inKeyboardMode = true;
            FEngBeginTextInput(FEngHashString("Message_body"), 0, "", GetLocalizedString(0x9a2e5f7b), 116);
            break;
        case 0xc519bfc3:
            bStrCpy(gScreenCallingFeedback, "OLX_Message.fng");
            cFEng::Get()->QueuePackagePush("OL_Feedback.fng", 0, 0, false);
            break;
        case 0xc519bfc4:
            if (gBuddySettings.IsWannaBeMyBuddy) {
                cFEng::Get()->QueueGameMessagePkg(10, cFEng::Get()->FindPackage("OL_FriendDialogue.fng"));
            } else {
                char msg[256] = "";
                FEngSNPrintf(msg, 256, GetLocalizedString(0x8d0a8371), gBuddySettings.buddyName);
                DialogInterface::ShowTwoButtons("OL_FriendDialogue.fng", "", dialog_alert, 0x70e01038, 0x417b25e4, 8,
                                                0xffffffff, 0xffffffff, first_dialog_button2, msg);
            }
            break;
        case 0xc9d30688:
            if (m_strBuf[0] != '\0') {
                break;
            }
            FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
            break;
        case 0xda5b8712: {
            char strBuf[256] = "";
            bStrCat(strBuf, strBuf, FEngGetEditedString());
            bMemSet(strBuf, 0, 256);
            bStrCat(strBuf, strBuf, FEDatabase->GetOnlineSettings()->GetLobbyPersona());
            bStrCat(strBuf, strBuf, "> ");
            bStrCat(strBuf, strBuf, FEngGetEditedString());
            Receive(strBuf);
            SendBuddyMessage(FEngGetEditedString());
            m_inKeyboardMode = false;
            break;
        }
        case 0x5073ef13:
            break;
        case 0x35f8620b:
            Display();
            break;
    }
}

void UIOnlineChatDialog::_SendBuddyMessage(const char *message) {
    int rc;
    char UTF8Message[201];

    Utf8EncodeFrom8Bit(UTF8Message, 200, message, &m_ASCIIToUTF8Table);

    if (gBuddySettings.isInGroup) {
        rc = HLBListSendMsgToGroup(BuddyCore::instance()->getApiRef(), 'chat', UTF8Message, 1,
                                   hlb_onlinechat_op_callback<&UIOnlineChatDialog::SendBuddyMessageCB>::call, this);
    } else {
        rc = HLBListSendChatMsg(BuddyCore::instance()->getApiRef(), gBuddySettings.buddyName, UTF8Message,
                                hlb_onlinechat_op_callback<&UIOnlineChatDialog::SendBuddyMessageCB>::call, this);
    }

    if (rc) {
        DisplayHLBerrorType(GetPackageName(), rc);
    } else {
        m_waitingForCB = true;
    }
}

void UIOnlineChatDialog::SendBuddyMessageCB(int op, int status) {
    m_waitingForCB = false;

    if (status == -108) {
        return;
    }

    if (status == -5) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                       GetLocalizedString(0x205b2bf8));
        return;
    }

    if (status == 'rate') {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                       GetLocalizedString(0xdf834843));
        return;
    }

    if (status) {
        DisplayHLBerrorType(GetPackageName(), status);
    } else if (gBuddySettings.isInGroup) {
        FEngSetScript(GetPackageName(), 0x47ff4e7c, 0xde6eff34, true);
    }
}

void UIOnlineChatDialog::Setup() {
    ScrollerSlot *pslot = new ("ScrollerSlot", 0) ScrollerSlot;
    pslot->AddData(FEngFindString(GetPackageName(), 0x6d851451));
    m_msgBodyScroller.AddSlot(pslot);
    FEPrintf(GetPackageName(), 0x6d851451, " ");
}

void UIOnlineChatDialog::ReadBuddyMessage() {
    UpdateStatusIcons(0, false);
    gBuddyListHasChanged = true;

    if (!HLBMsgListGetUnreadCount(gBuddySettings.pBuddy)) {
        return;
    }

    BuddyApiMsgT *buddyApiMsg = HLBMsgListGetFirstUnreadMsg(gBuddySettings.pBuddy, 0, 1);

    char buf[256] = " ";
    bStrCat(buf, buf, gBuddySettings.buddyName);
    bStrCat(buf, buf, "> ");
    char msgBuf[256] = " ";

    TagFieldGetString(TagFieldFind(buddyApiMsg->pData, "BODY"), msgBuf, 256, "Not Present");

    Utf8TransTblT UTF8ToASCIITranslate;
    unsigned char UTF8ToASCIITable[256];

    UTF8ToASCIITranslate.uRangeBegin = 0;
    UTF8ToASCIITranslate.uRangeEnd = 255;
    UTF8ToASCIITranslate.pCodeTbl = UTF8ToASCIITable;
    for (unsigned int i = 0; i < 256; i++) {
        UTF8ToASCIITable[i] = i;
    }
    Utf8TranslateTo8Bit(buf + strlen(buf), 255 - strlen(buf), msgBuf, 0, &UTF8ToASCIITranslate);

    Receive(buf);
}

void UIOnlineChatDialog::AddNewBuddyMsgs() {
    BuddyApiMsgT *buddyApiMsg = HLBMsgListGetFirstUnreadMsg(gBuddySettings.pBuddy, 0, 1);

    char buf[256] = " ";
    bStrCat(buf, buf, gBuddySettings.buddyName);
    bStrCat(buf, buf, "> ");
    char msgBuf[256] = " ";

    TagFieldGetString(TagFieldFind(buddyApiMsg->pData, "BODY"), msgBuf, 256, "Not Present");

    Utf8TransTblT UTF8ToASCIITranslate;
    unsigned char UTF8ToASCIITable[256];

    UTF8ToASCIITranslate.uRangeBegin = 0;
    UTF8ToASCIITranslate.uRangeEnd = 255;
    UTF8ToASCIITranslate.pCodeTbl = UTF8ToASCIITable;
    for (unsigned int i = 0; i < 256; i++) {
        UTF8ToASCIITable[i] = i;
    }
    Utf8TranslateTo8Bit(buf + strlen(buf), 255 - strlen(buf), msgBuf, 0, &UTF8ToASCIITranslate);

    Receive(buf);
}

int UIOnlineChatDialog::WordWrap(char *message) {
    short widestring[152];
    int size = strlen(message);
    int messagesize = size;

    CopyString(widestring, message);
    while (m_pFont->GetLineWidth(widestring, m_Flags, 0, false) >= m_Width - 16) {
        size--;
        if (size < 2) {
            break;
        }
        CopyString(widestring, message, size);
    }

    if (size < messagesize) {
        int wordbreak = size - 1;

        while (wordbreak > 0 && message[wordbreak] != ' ') {
            wordbreak--;
        }
        if (wordbreak > 0) {
            size = wordbreak;
            while (wordbreak > 0 && message[wordbreak] == ' ') {
                wordbreak--;
            }
            if (wordbreak > 0) {
                size = wordbreak;
            }
            size++;
        }
    }

    return size;
}

void UIOnlineChatDialog::AddLine(char *message, int size) {
    int buffersize = strlen(m_strBuf);
    char *bufferptr = m_strBuf;

    while (size + buffersize + 1 >= 4096) {
        while (*bufferptr++ != '\n') {
            buffersize--;
        }
        buffersize--;

        m_Lines--;
    }

    if (bufferptr > m_strBuf) {
        bStrCpy(m_strBuf, bufferptr);
    }

    bufferptr = &m_strBuf[buffersize];
    bStrNCpy(bufferptr, message, size);

    bufferptr += size;
    *bufferptr = '\n';
    bufferptr[1] = '\0';

    m_Lines++;

    if (m_Scroll > 12) {
        if (++m_Scroll > m_Lines) {
            m_Scroll = m_Lines;
        }
    }
}

void UIOnlineChatDialog::Display() {
    if (m_pChatBody) {
        int linesback = 0;
        char *bufferend = &m_strBuf[bStrLen(m_strBuf)];
        char *displayptr = bufferend;

        while (displayptr > m_strBuf) {
            if (displayptr[-1] == '\n') {
                if (++linesback > m_Scroll) {
                    break;
                }
            }
            displayptr--;
        }

        char *displayend = displayptr;
        int linesdisplayed = 0;

        while (displayend < bufferend) {
            if (*displayend++ == '\n') {
                if (++linesdisplayed >= 12) {
                    break;
                }
            }
        }

        int length = displayend - displayptr;
        CopyString(m_pChatBody->string.AllocateString(length + 1), displayptr, length);

        m_pChatBody->Flags |= 0x400802;

        if (m_Lines < 13) {
            m_ScrollBar.SetBackingVisibility(false);
        } else {
            m_ScrollBar.SetBackingVisibility(true);
            m_ScrollBar.SetArrow1Visibility(true);
            m_ScrollBar.SetArrow2Visibility(true);
        }

        m_ScrollBar.Update(12, m_Lines, bMax(1, m_Lines - m_Scroll + 1), -1);
    }
}

void UIOnlineChatDialog::Receive(char *message) {
    char *tempMsg = message;

    while (*tempMsg) {
        if (*tempMsg == '^') {
            *tempMsg = '*';
        }
        tempMsg++;
    }

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

void UIOnlineChatDialog::ScrollChatBodyUp() {
    if (m_Scroll < m_Lines) {
        m_Scroll++;
    }
    Display();
}

void UIOnlineChatDialog::ScrollChatBodyDown() {
    if (m_Scroll > 12) {
        m_Scroll--;
    }
    Display();
}
