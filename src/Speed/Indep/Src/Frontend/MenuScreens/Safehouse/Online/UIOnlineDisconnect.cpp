#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineDisconnect.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineLogin.hpp"
#include "Speed/Indep/Src/Generated/Events/EQuitToFE.hpp"
#include "Speed/Indep/Src/Online/NetworkCore.hpp"
#include "Speed/Indep/Src/Online/WebOfferUG2.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"

static const char *pDisconnectDialogName = "GenericDialog_MED.fng";

// total size: 0x3C
class CUIOnlineDisconnectDialog : public MenuScreen {
  public:
    CUIOnlineDisconnectDialog(ScreenConstructorData *sd);
    ~CUIOnlineDisconnectDialog() override {}

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

    static void SetParams(LobbyCoreN::OnlineDisconnectReason reason, uint32 code, uint32 label) {
        m_DisconnectReason = reason;
        m_DisconnectCode = code;
        m_DisconnectSuppliedStringHash = label;
    }

  private:
    void SetMessage(uint32 MessageHash);
    void SetButtons(bool bCanDoReconnect);
    void TakeAction(int ButtonIndex);

    static LobbyCoreN::OnlineDisconnectReason m_DisconnectReason;
    static uint32 m_DisconnectCode;
    static uint32 m_DisconnectSuppliedStringHash;

    uint32 m_ButtonActions[3];                // offset 0x2C, size 0xC
    CUIOnlineDisconnect *m_pDisconnectParent; // offset 0x38, size 0x4
};

LobbyCoreN::OnlineDisconnectReason CUIOnlineDisconnectDialog::m_DisconnectReason = LobbyCoreN::DISC_DISCONNECTED;
uint32 CUIOnlineDisconnectDialog::m_DisconnectCode = 0;
uint32 CUIOnlineDisconnectDialog::m_DisconnectSuppliedStringHash = 0;

CUIOnlineDisconnect *CUIOnlineDisconnect::m_pUIOnlineDisconnect_Instance = nullptr;
cOnlineSettings::eOnlineState CUIOnlineDisconnect::m_FinalReconnectState = cOnlineSettings::OLS_LOBBY_IDLE;
bool CUIOnlineDisconnect::m_bNotificationPending = false;
bool CUIOnlineDisconnect::mIsHostInGameDisconnect = false;

MenuScreen *CreateOnlineDisconnectDialog(ScreenConstructorData *sd) {
    return new ("UIOnlineDisconnect", 0) CUIOnlineDisconnectDialog(sd);
}

MenuScreen *CreateOnlineDisconnectBG(ScreenConstructorData *sd) {
    return new ("UIOnlineDisconnectBG", 0) CUIOnlineDisconnect(sd);
}

CUIOnlineDisconnect::CUIOnlineDisconnect(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , m_bReconnectInProgress(false) //
    , m_bReconnectSuccessful(false) //
    , m_bDisconnectDialogPending(true) //
    , m_bDisconnectDialogUp(false) {
    if (mIsHostInGameDisconnect) {
        if (TheOnlineManager.IsServer()) {
            DialogInterface::ShowOneButton(GetPackageName(), "InGameDialog.fng", dialog_alert, 0x417b2601, 0x215f55d6, 0x215f55d6,
                                           0x17aaef8b);
            return;
        }
        DialogInterface::ShowOneButton(GetPackageName(), "InGameDialog.fng", dialog_alert, 0x417b2601, 0x215f55d6, 0x215f55d6,
                                       0x1754f68a);
        return;
    }

    DebugPrintf("OnlineDisconnect Constructed.");
    ClearDisconnectNotificationPending();
    m_pUIOnlineDisconnect_Instance = this;
    cFEng::Get()->QueuePackagePush("UI_OL_Disconnect.fng", reinterpret_cast<int>(this), 0, false);
}

CUIOnlineDisconnect::~CUIOnlineDisconnect() {
    DebugPrintf("OnlineDisconnect Destroyed.");
    m_pUIOnlineDisconnect_Instance = nullptr;
}

void CUIOnlineDisconnect::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x215f55d6:
            cFEng::Get()->QueuePackagePop(1);
            new EQuitToFE(GARAGETYPE_MAIN_FE, nullptr);
            break;
        case 0xc98356ba:
            if (m_bReconnectSuccessful) {
                DebugPrintf("Reconnect Success - disconnect system go bye byes.");
                cFEng::Get()->QueuePackagePop(1);
            }
            break;
    }
}

void CUIOnlineDisconnect::ShowDialog(LobbyCoreN::OnlineDisconnectReason Reason, uint32 Code, uint32 Label) {
    DebugPrintf("ShowDialog, Reason(%d), Code('%c%c%c%c'), Label(0x%x)", Reason, Code >> 24, Code >> 16, Code >> 8, Code, Label);

    if (!cFEng::Get()->FindPackage("UI_OL_Disconnect_BG.fng")) {
        CUIOnlineDisconnectDialog::SetParams(Reason, Code, Label);
        cFEng::Get()->QueuePackagePush("UI_OL_Disconnect_BG.fng", 0, 0, false);
    } else {
        DebugPrintf("  |_ Disconnect_BG already up .. checking dialog status.\n");
        if (m_pUIOnlineDisconnect_Instance->m_bDisconnectDialogPending || m_pUIOnlineDisconnect_Instance->m_bDisconnectDialogUp) {
            DebugPrintf("    |_ Dialog is UP - ignoring new disconnect error.  This may have issues if the new one is a "
                        "non-reconnectable ?\n");
        } else {
            DebugPrintf("    |_ No Dialog - Reconnect was in progress.  Showing dialog.\n");
            m_pUIOnlineDisconnect_Instance->m_bDisconnectDialogPending = true;
            CUIOnlineDisconnectDialog::SetParams(Reason, Code, Label);
            cFEng::Get()->QueuePackagePush("UI_OL_Disconnect.fng", reinterpret_cast<int>(m_pUIOnlineDisconnect_Instance), 0, false);
        }
    }
    DebugPrintf("ShowDialog Done.");
}

void CUIOnlineDisconnect::SetReconnectFinalState() {
    DebugPrintf("Set Reconnect final state (%d).", FEDatabase->GetOnlineSettings()->GetState());
    m_FinalReconnectState = FEDatabase->GetOnlineSettings()->GetState();
}

bool CUIOnlineDisconnect::IsReconnectable(uint32 DisconnectErrorMsgHash) {
    bool reconnectable = false;
    if (DisconnectErrorMsgHash == 0xa3f55e1f || DisconnectErrorMsgHash == 0x5a6f817f || DisconnectErrorMsgHash == 0x2177c024) {
        reconnectable = true;
    }
    return reconnectable;
}

void CUIOnlineDisconnect::DoDisconnectBackToMain() {
    if (!TheOnlineManager.IsOnlineRace()) {
        DebugPrintf("Disconnecting - going back to main menu.");
        MenuScreen *pWebOfferScreen = FEPackageManager::Get()->FindScreen(gOL_EA_TERMS);
        if (pWebOfferScreen) {
            static_cast<CUIWebOfferStart *>(pWebOfferScreen)->Shutdown();
        }
        EnteringChallenge = false;
        cFEng::Get()->QueuePackagePop(0);
        cFEng::Get()->PopErrorPackage();
        ExitOnlineMode(GetPackageName());
    } else {
        DebugPrintf("Post Race - pass on FEMSG_DISCONNECT.");
    }
}

void CUIOnlineDisconnect::DoReconnect() {
    DebugPrintf("Reconnect : Wind back to OL Main menu");
    if (!cFEng::Get()->IsPackagePushed("OL_MAIN.fng")) {
        cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
    }
    DebugPrintf("Reconnecting.");
    DebugPrintf("  |_ Sending speculative restart uiautolobbylogin message.");
    FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_NETWORK_UNINITIALIZED);
    cFEng::Get()->QueuePackagePop(1);
    bBreak();
}

void CUIOnlineDisconnect::DisconnectDialogConstructed() {
    DebugPrintf("Pending Dialog Done");
    m_bDisconnectDialogPending = false;
    m_bDisconnectDialogUp = true;
}

void CUIOnlineDisconnect::DisconnectDialogGone() {
    DebugPrintf("Disconnect Dialog Done and Gone");
    m_bDisconnectDialogPending = false;
    m_bDisconnectDialogUp = false;
}

CUIOnlineDisconnectDialog::CUIOnlineDisconnectDialog(ScreenConstructorData *sd) : MenuScreen(sd) {
    uint32 Message;
    bool bCanDoRetry;

    FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0xdbe419d4);
    m_pDisconnectParent = reinterpret_cast<CUIOnlineDisconnect *>(sd->Arg);

    Message = CUIOnlineDisconnect::GetDisconnectMessage(m_DisconnectReason, m_DisconnectCode, m_DisconnectSuppliedStringHash);
    bCanDoRetry = CUIOnlineDisconnect::IsReconnectable(Message);
    if (TheOnlineManager.IsOnlineRace() || OnlineLoginScreenManager::Instance()) {
        Message = 0xef6e7cdb;
        bCanDoRetry = false;
    }
    SetMessage(Message);
    SetButtons(bCanDoRetry);

    CUIOnlineDisconnect::DebugPrintf("OnlineDisconnectDialog .. doing disconnect.");
    if (FEDatabase->GetOnlineSettings()->GetState() > cOnlineSettings::OLS_NETWORK_UNINITIALIZED) {
        LobbyDisconnect();
        TheOnlineManager.Disconnect(true);
    }
    CUIOnlineDisconnect::DebugPrintf("OnlineDisconnectDialog .. disconnect done.");
    m_pDisconnectParent->DisconnectDialogConstructed();
}

void CUIOnlineDisconnectDialog::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x9120409e:
            FEngSetCurrentButton(GetPackageName(), 0xb8a7c6cc);
            break;
        case 0xb5971bf1:
            FEngSetCurrentButton(GetPackageName(), 0xb8a7c6cd);
            break;
        case 0x0c407210:
            switch (obj->NameHash) {
                case 0xb8a7c6cc:
                    TakeAction(0);
                    break;
                case 0xb8a7c6cd:
                    TakeAction(1);
                    break;
                case 0xb8a7c6ce:
                    TakeAction(2);
                    break;
            }
            break;
    }
}

uint32 CUIOnlineDisconnect::GetDisconnectMessage(LobbyCoreN::OnlineDisconnectReason Reason, uint32 Code, uint32 Label) {
    uint32 MessageHash = 0;

    switch (Reason) {
        case LobbyCoreN::DISC_DISCONNECTED:
            if (Code) {
                DebugPrintf("MSG : Server disconnected us!  Reason:  DISC_DISCONNECTED\n");
                bool bonln = NetConnStatus('onln', 0, nullptr) != 0;
                uint32 conn = NetConnStatus('conn', 0, nullptr);
                DebugPrintf("  |_ Online status check (onln=%d), (plug=%d), (conn=%c%c%c%c).", bonln,
                            NetConnStatus('plug', 0, nullptr) != 0, conn >> 24, conn >> 16, conn >> 8, conn);
                if (Label) {
                    DebugPrintf("  |_ String message hash (0x%x) supplied.", Label);
                    NetConnStatus('onln', 0, nullptr);
                    NetConnStatus('conn', 0, nullptr);
                    MessageHash = Label;
                } else {
                    DebugPrintf("  |_ No supplied message .. ISP disconnect or Server disconnect ?.");
                    if (bonln) {
                        DebugPrintf("    |_  Still connected to ISP - allow reconnect and display server down msg.");
                        if (TheOnlineManager.IsOnlineRace()) {
                            MessageHash = 0xef6e7cdb;
                        } else {
                            MessageHash = 0x5a6f817f;
                        }
                    } else {
                        DebugPrintf("    |_  ISP Connection gone.");
                        MessageHash = 0xef6e7cdb;
                    }
                }
            }
            break;
        case LobbyCoreN::DISC_SERVER_DOWN:
            DebugPrintf("MSG : Server disconnected us!  Reason:  DISC_SERVER_DOWN\n");
            if (TheOnlineManager.IsOnlineRace()) {
                MessageHash = 0xef6e7cdb;
            } else {
                MessageHash = 0x5a6f817f;
            }
            break;
        case LobbyCoreN::DISC_NEED_UPDATE:
            DebugPrintf("MSG : Server disconnected us!  Reason:  DISC_NEED_UPDATE\n");
            MessageHash = 0x10073c4f;
            break;
    }
    return MessageHash;
}

void CUIOnlineDisconnectDialog::SetMessage(uint32 MessageHash) {
    FEngSetLanguageHash(GetPackageName(), 0x1e2640fa, MessageHash);
}

void CUIOnlineDisconnectDialog::SetButtons(bool bCanDoReconnect) {
    uint32 ButtonTextHashes[2];
    int NumButtons;

    if (!bCanDoReconnect) {
        NumButtons = 1;
        ButtonTextHashes[0] = 0x417b2601;
        m_ButtonActions[0] = 3;
    } else if (FEDatabase->IsLANMode()) {
        NumButtons = 2;
        ButtonTextHashes[0] = 0x417b2601;
        ButtonTextHashes[1] = 0x1a294dad;
        m_ButtonActions[0] = 0;
        m_ButtonActions[1] = 3;
    } else {
        NumButtons = 2;
        ButtonTextHashes[0] = 0x039df715;
        ButtonTextHashes[1] = 0x1a294dad;
        m_ButtonActions[0] = 2;
        m_ButtonActions[1] = 3;
    }

    if (NumButtons == 1) {
        FEngSetInvisible(GetPackageName(), 0xb8a7c6cd);
        FEngSetInvisible(GetPackageName(), 0xb8a7c6ce);
        FEngDisableButton(GetPackageName(), 0xb8a7c6cd);
        FEngDisableButton(GetPackageName(), 0xb8a7c6ce);
        FEPrintf(GetPackageName(), 0xfb8b67d1, "");
        FEPrintf(GetPackageName(), 0xfde09072, "");
        FEngSetVisible(GetPackageName(), 0xb8a7c6cc);
        FEngEnableButton(GetPackageName(), 0xb8a7c6cc);
        FEngSetCurrentButton(GetPackageName(), 0xb8a7c6cc);
        FEngSetLanguageHash(GetPackageName(), 0xf9363f30, ButtonTextHashes[0]);
    } else if (NumButtons == 2) {
        FEngSetInvisible(GetPackageName(), 0xb8a7c6ce);
        FEngDisableButton(GetPackageName(), 0xb8a7c6ce);
        FEPrintf(GetPackageName(), 0xfde09072, "");
        FEngSetLanguageHash(GetPackageName(), 0xf9363f30, ButtonTextHashes[0]);
        FEngSetLanguageHash(GetPackageName(), 0xfb8b67d1, ButtonTextHashes[1]);
        FEngSetCurrentButton(GetPackageName(), 0xb8a7c6cd);
        FEngDisableButton(GetPackageName(), 0xb8a7c6cc);
    }
}

void CUIOnlineDisconnectDialog::TakeAction(int ButtonIndex) {
    CUIOnlineDisconnect::DebugPrintf("Taking action on button(%d)", ButtonIndex);
    m_pDisconnectParent->DisconnectDialogGone();
    if (m_ButtonActions[ButtonIndex] == 3) {
        m_pDisconnectParent->DoDisconnectBackToMain();
    } else if (m_ButtonActions[ButtonIndex] == 2) {
        m_pDisconnectParent->DoReconnect();
    }
}

void ExitOnlineMode(const char *currentPackageName) {
    PS2Isp::Instance().Finish();
    if (LobbyLogin::Instance().IsStarted()) {
        LobbyLogin::Instance().Finish();
    }
    NetworkCore::Instance().Destroy();
    FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_NETWORK_UNINITIALIZED);
    OnlineLoginScreenManager::CommitSuicide();
    FreeOnlineMemoryPool();
    if (currentPackageName) {
        cFEng::Get()->QueuePackageSwitch("MainMenu.fng", 0, 0, false);
    }
}
