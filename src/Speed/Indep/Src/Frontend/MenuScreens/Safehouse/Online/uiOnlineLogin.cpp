#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineLogin.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/FEng/FEList.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feUIWidgetMenu.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feWidget.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineDisconnect.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineMain.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/uiMain.hpp"
#include "Speed/Indep/Src/Misc/BuildRegion.hpp"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/ResourceLoader.hpp"
#include "Speed/Indep/Src/Online/BuddyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyAccount.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyLogin.hpp"
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#include "Speed/Indep/Src/Online/NetworkCore.hpp"
#include "Speed/Indep/Src/Online/OnlineCfg.hpp"
#include "Speed/Indep/Src/Online/WebOfferUG2.hpp"
#include "Speed/Indep/Src/World/CarLoader.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/PSX2/Src/Online/PS2Isp.hpp"

#include <string.h>

OnlineLoginScreenManager *OnlineLoginScreenManager::pInstance = nullptr;

const char *gOL_NET_SELECT = "UI_OLNetSelect.fng";
const char *gOL_LOGIN = "UI_OLISPConnect.fng";
const char *gOL_AUTH_DNAS = "UI_OLAuthDNAS.fng";
const char *gOL_EA_LOGIN = "OLEALogin.fng";
const char *gOL_EA_TERMS = "UI_OL_WebOffer2.fng";
const char *gOL_SELECT_PERSONA = "OL_SelectPersona.fng";
const char *gOL_CREATE_USER_A = "OL_Create_Account.fng";
const char *gOL_CREATE_USER_B = "UI_OLX_Info_Sharing.fng";
const char *gOL_CREATE_ACCOUNT_COUNTRY = "OL_Create_Account_Country.fng";
const char *gOL_PARENT_EMAIL = "UI_OLAgeVerif.fng";
const char *gOL_USE_EXISTING = "OL_UseExisting.fng";
const char *g_ONLINE_MAIN_MENU = "OL_MAIN.fng";
const char *g_MAIN_MENU = "MainMenu.fng";
bool ResendData = false;
bool ComingBack = false;
bool gbParentalEmailHasBeenEntered = false;
bool gMemCardScreenShowing = false;

OnlineLoginScreenManager *OnlineLoginScreenManager::Instance() {
    return pInstance;
}

void OnlineLoginScreenManager::CommitSuicide() {
    if (pInstance) {
        delete pInstance;
    }
    pInstance = nullptr;
}

OnlineLoginScreenManager::ScreenNode *OnlineLoginScreenManager::FindScreen(const char *pkg) {
    for (ScreenNode *n = ScreenList.GetHead(); n != ScreenList.EndOfList(); n = n->GetNext()) {
        if (!bStrICmp(n->Package, pkg)) {
            return n;
        }
    }
    return nullptr;
}

void OnlineLoginScreenManager::AddScreen(const char *pkg, const char *(OnlineLoginScreenManager::*fwd)(),
                                         const char *(OnlineLoginScreenManager::*back)()) {
    if (!FindScreen(pkg)) {
        ScreenList.AddTail(new ("ScreenNode", 0) ScreenNode(pkg, fwd, back));
    }
}

bool OnlineLoginScreenManager::IsFrontdoor(const char *name) {
    return !bStrICmp(name, Frontdoor);
}

bool OnlineLoginScreenManager::IsBackdoor(const char *name) {
    return !bStrICmp(name, Backdoor);
}

void OnlineLoginScreenManager::EnterFront(const char *from_pkg) {
    NetworkUseLobbies = 1;

    const char *front_door = g_MAIN_MENU;
    const char *back_door = g_ONLINE_MAIN_MENU;
    pInstance = new ("OnlineLoginScreenManager", 0) OnlineLoginScreenManager(from_pkg, front_door, back_door);
}

void OnlineLoginScreenManager::Start(const char *from_pkg, const char *first_screen) {
    CurrentScreen = FindScreen(first_screen);
    cFEng::Get()->QueuePackageSwitch(CurrentScreen->Package, 0, 0, false);

    MemoryCard::GetInstance()->ShowTRCCheckCardMessage();
    gMemCardScreenShowing = true;
}

OnlineLoginScreenManager::OnlineLoginScreenManager(const char *from_pkg, const char *front_door, const char *back_door)
    : EALoginOutcome(OUTCOME_UNSET) //
    , EATermsAnswered(false) //
    , EATermsAccepted(false) //
    , NeedToAcceptEATerms(false) //
    , TOSFromCreateAccount(false) //
    , NeedToChoosePersona(false) //
    , HasChosenPersona(false) //
    , m_boDoContextLogin(false) //
    , hasShownParentEmail(false) //
    , NeedToChooseNetworkConfig(true) //
    , NeedToDNASAuthenticate(true) //
    , NoNetworkConfig(false) {
    Frontdoor = front_door;
    Backdoor = back_door;

    AddScreen(gOL_LOGIN, &OnlineLoginScreenManager::CalcNextScreenFromOnlineLogin,
              &OnlineLoginScreenManager::CalcPrevScreenFromOnlineLogin);
    AddScreen(gOL_NET_SELECT, &OnlineLoginScreenManager::CalcNextScreenFromNetSelect,
              &OnlineLoginScreenManager::CalcPrevScreenFromNetSelect);
    AddScreen(gOL_AUTH_DNAS, &OnlineLoginScreenManager::CalcNextScreenFromAuthDNAS,
              &OnlineLoginScreenManager::CalcPrevScreenFromAuthDNAS);
    AddScreen(gOL_EA_LOGIN, &OnlineLoginScreenManager::CalcNextScreenFromEALogin,
              &OnlineLoginScreenManager::CalcPrevScreenFromEALogin);
    AddScreen(gOL_EA_TERMS, &OnlineLoginScreenManager::CalcNextScreenFromEATerms,
              &OnlineLoginScreenManager::CalcPrevScreenFromEATerms);
    AddScreen(gOL_CREATE_USER_A, &OnlineLoginScreenManager::CalcNextScreenFromCreateUserA,
              &OnlineLoginScreenManager::CalcPrevScreenFromCreateUserA);
    AddScreen(gOL_CREATE_USER_B, &OnlineLoginScreenManager::CalcNextScreenFromCreateUserB,
              &OnlineLoginScreenManager::CalcPrevScreenFromCreateUserB);
    AddScreen(gOL_USE_EXISTING, &OnlineLoginScreenManager::CalcNextScreenFromUseExisting,
              &OnlineLoginScreenManager::CalcPrevScreenFromUseExisting);
    AddScreen(gOL_SELECT_PERSONA, &OnlineLoginScreenManager::CalcNextScreenFromSelectPersona,
              &OnlineLoginScreenManager::CalcPrevScreenFromSelectPersona);

    Start(from_pkg, gOL_LOGIN);

    bStrNCpy(ValidExistingAccountName, FEDatabase->GetOnlineSettings()->GetLobbyAccountName(), 16);
    bStrNCpy(ValidExistingPassword, FEDatabase->GetOnlineSettings()->GetLobbyPassword(), 16);
}

void OnlineLoginScreenManager::SetDefaultFlags() {
    EALoginOutcome = OUTCOME_UNSET;
    EATermsAnswered = false;
    EATermsAccepted = false;
    NeedToAcceptEATerms = false;
    NeedToChoosePersona = false;
    HasChosenPersona = false;
    m_boDoContextLogin = false;

    NeedToChooseNetworkConfig = true;
    NeedToDNASAuthenticate = true;
    NoNetworkConfig = false;
}

OnlineLoginScreenManager::~OnlineLoginScreenManager() {}

void OnlineLoginScreenManager::ChangeToNextScreen() {
    const char *new_pkg = (this->*CurrentScreen->ForwardFunction)();

    CurrentScreen = FindScreen(new_pkg);
    cFEng::Get()->QueuePackageSwitch(new_pkg, 0, 0, false);

    if (IsBackdoor(new_pkg)) {
        CommitSuicide();
    }
}

void OnlineLoginScreenManager::ChangeToPrevScreen() {
    const char *new_pkg = (this->*CurrentScreen->BackwardFunction)();

    CurrentScreen = FindScreen(new_pkg);
    cFEng::Get()->QueuePackageSwitch(new_pkg, 0, 0, false);

    if (IsFrontdoor(new_pkg)) {
        ExitOnlineMode(nullptr);
    }
}

const char *OnlineLoginScreenManager::CalcNextScreenFromNetSelect() {
    return gOL_LOGIN;
}

const char *OnlineLoginScreenManager::CalcNextScreenFromOnlineLogin() {
    if (NoNetworkConfig) {
        return gOL_NET_SELECT;
    }

    PS2Isp &ps2isp = PS2Isp::Instance();
    if (ps2isp.IsStarted() == true) {
        if (ps2isp.GetContext() == PS2ISP_CONTEXT_SELECT) {
            return gOL_NET_SELECT;
        }
        if (ps2isp.GetContext() == PS2ISP_CONTEXT_DNAS) {
            return gOL_AUTH_DNAS;
        }
    }

    {
        LobbyLogin &lobbyLogin = LobbyLogin::Instance();
        if (lobbyLogin.IsStarted() == true) {
            LobbyLoginContextE context = lobbyLogin.GetContext();

            if (context == LOBBYLOGIN_CONTEXT_CONNECT) {
                return gOL_EA_LOGIN;
            } else if (context == LOBBYLOGIN_CONTEXT_LOGIN) {
                if (NeedToChoosePersona == true) {
                    bStrNCpy(Instance()->ValidExistingAccountName, FEDatabase->GetOnlineSettings()->GetLobbyAccountName(), 16);
                    bStrNCpy(Instance()->ValidExistingPassword, FEDatabase->GetOnlineSettings()->GetLobbyPassword(), 16);
                    return gOL_SELECT_PERSONA;
                }
                if (EALoginOutcome == WANTS_NEW_ACCOUNT) {
                    return gOL_CREATE_USER_A;
                } else if (EALoginOutcome == WANTS_EXISTING_ACCOUNT) {
                    return gOL_USE_EXISTING;
                }
            } else if (context == LOBBYLOGIN_CONTEXT_TOS) {
                ConfigureWebOfferForTOS();
                return gOL_EA_TERMS;
            } else if (context == LOBBYLOGIN_CONTEXT_SELPERS) {
                bStrNCpy(Instance()->ValidExistingAccountName, FEDatabase->GetOnlineSettings()->GetLobbyAccountName(), 16);
                bStrNCpy(Instance()->ValidExistingPassword, FEDatabase->GetOnlineSettings()->GetLobbyPassword(), 16);
                return gOL_SELECT_PERSONA;
            }
        }
    }

    return g_ONLINE_MAIN_MENU;
}

const char *OnlineLoginScreenManager::CalcNextScreenFromAuthDNAS() {
    return gOL_LOGIN;
}

const char *OnlineLoginScreenManager::CalcNextScreenFromEALogin() {
    if (EALoginOutcome == WANTS_NEW_ACCOUNT) {
        NeedToAcceptEATerms = true;
    }
    return gOL_LOGIN;
}

const char *OnlineLoginScreenManager::CalcNextScreenFromEATerms() {
    if (TOSFromCreateAccount) {
        TOSFromCreateAccount = false;
        return gOL_CREATE_USER_B;
    }
    return gOL_LOGIN;
}

const char *OnlineLoginScreenManager::CalcNextScreenFromSelectPersona() {
    return gOL_LOGIN;
}

const char *OnlineLoginScreenManager::CalcNextScreenFromCreateUserA() {
    if (NeedToAcceptEATerms || !EATermsAccepted) {
        TOSFromCreateAccount = true;
        return gOL_EA_TERMS;
    }
    return gOL_CREATE_USER_B;
}

const char *OnlineLoginScreenManager::CalcNextScreenFromCreateUserB() {
    return gOL_SELECT_PERSONA;
}

const char *OnlineLoginScreenManager::CalcNextScreenFromUseExisting() {
    return gOL_LOGIN;
}

const char *OnlineLoginScreenManager::CalcPrevScreenFromNetSelect() {
    return g_MAIN_MENU;
}

const char *OnlineLoginScreenManager::CalcPrevScreenFromOnlineLogin() {
    LobbyLogin &lobbyLogin = LobbyLogin::Instance();

    if (lobbyLogin.IsStarted() == true) {
        LobbyLoginContextE context = lobbyLogin.GetContext();

        if (context == LOBBYLOGIN_CONTEXT_CONNECT) {
            return gOL_EA_LOGIN;
        } else if (context == LOBBYLOGIN_CONTEXT_LOGIN) {
            if (EALoginOutcome == WANTS_EXISTING_ACCOUNT) {
                return gOL_USE_EXISTING;
            }
            if (EALoginOutcome == WANTS_NEW_ACCOUNT) {
                return gOL_CREATE_USER_A;
            }
            return gOL_EA_LOGIN;
        } else if (context == LOBBYLOGIN_CONTEXT_TOS) {
            return gOL_CREATE_USER_A;
        } else if (context == LOBBYLOGIN_CONTEXT_SELPERS || context == LOBBYLOGIN_CONTEXT_DONE) {
            HasChosenPersona = false;
            return gOL_SELECT_PERSONA;
        }
    }

    return g_MAIN_MENU;
}

const char *OnlineLoginScreenManager::CalcPrevScreenFromAuthDNAS() {
    return g_MAIN_MENU;
}

const char *OnlineLoginScreenManager::CalcPrevScreenFromEALogin() {
    return g_MAIN_MENU;
}

const char *OnlineLoginScreenManager::CalcPrevScreenFromEATerms() {
    return gOL_EA_LOGIN;
}

const char *OnlineLoginScreenManager::CalcPrevScreenFromSelectPersona() {
    return gOL_EA_LOGIN;
}

const char *OnlineLoginScreenManager::CalcPrevScreenFromCreateUserA() {
    return gOL_EA_LOGIN;
}

const char *OnlineLoginScreenManager::CalcPrevScreenFromCreateUserB() {
    return gOL_CREATE_USER_A;
}

const char *OnlineLoginScreenManager::CalcPrevScreenFromUseExisting() {
    return gOL_EA_LOGIN;
}

static int sIndexOfCurrentConfig = 0;

// total size: 0x7C
class NCSelector : public FEToggleWidget {
  public:
    int currentConfig; // offset 0x64, size 0x4

  private:
    int displayIndex;        // offset 0x68, size 0x4
    dialog_handle msgHandle; // offset 0x6C, size 0x4
    bool allConfigsInvalid;  // offset 0x70, size 0x4
    NetConfigRecT *configs;  // offset 0x74, size 0x4
    bool mShouldDraw;        // offset 0x78, size 0x4

  public:
    NCSelector(const char *parent_pkg, bool bMemcardExist, bool)
        : FEToggleWidget(true) //
        , msgHandle(0) //
        , mShouldDraw(true) {
        PS2Isp &ps2isp = PS2Isp::Instance();

        if (ps2isp.GetNumNetworkConfigs() > 0) {
            configs = ps2isp.GetNetworkConfigs();
        }
        RefreshNetworkConfigs(parent_pkg, bMemcardExist);
    }

    ~NCSelector() override {
        sIndexOfCurrentConfig = currentConfig;
    }

    void Act(const char *parent_pkg, uint32 data) override {
        if (allConfigsInvalid && data != 0x0c407210) {
            return;
        }

        if (data == 0x9120409e) {
            GetPrevValidConfig();
        } else if (data == 0xb5971bf1) {
            GetNextValidConfig();
        } else if (data == 0x0c407210) {
            if (currentConfig == -1) {
                if (msgHandle) {
                    DialogInterface::DismissDialog(msgHandle);
                    msgHandle = 0;
                } else if (!bIsPlatSuperBender()) {
                    NetworkCore::Instance().ExecNetGui();
                }
            } else {
                OnlineLoginScreenManager::Instance()->NeedToChooseNetworkConfig = false;
                cFEng::Get()->QueueGameMessage(0x96c32e55, parent_pkg, 0xff);
            }
        }

        FEString *obj = static_cast<FEString *>(FEngFindObject(parent_pkg, 0x37c85b06));
        if (currentConfig == -1) {
            if (FEngIsVisible(obj) == true) {
                FEngSetInvisible(obj);
            }
        } else {
            if (FEngIsVisible(obj) == false) {
                FEngSetVisible(obj);
            }
        }

        BlinkArrows(data);
        Draw();
    }

    void Draw() override {
        if (!mShouldDraw) {
            FEngSetInvisible(GetDataObject());
            return;
        }

        FEngSetLanguageHash(GetTitleObject(), 0x8ef89614);

        if (currentConfig == -1) {
            FEngSetLanguageHash(GetDataObject(), 0xb6f38223);
        } else {
            char config[42] = "";

            bSNPrintf(config, 41, "%d - %s", displayIndex + 1, configs[currentConfig].strIsp);
            if (bStrLen(config) >= 27) {
                config[24] = '.';
                config[25] = '.';
                config[26] = '.';
                config[27] = '\0';
            }

            FEString *str = GetDataObject();
            if (str) {
                str->Flags |= 0x20;
                str->SetString(config);
                str->Flags |= 2;
            }
        }
    }

    void RefreshNetworkConfigs(const char *parent_pkg, bool memcardExists) {
        if (!memcardExists) {
            allConfigsInvalid = true;
            currentConfig = -1;
        } else if (PS2Isp::Instance().GetNumNetworkConfigs() == 0) {
            currentConfig = -1;
            displayIndex = -1;
            allConfigsInvalid = true;
        } else {
            currentConfig = -1;
            GetNextValidConfig();

            if (currentConfig >= 0) {
                allConfigsInvalid = false;
                displayIndex = 0;
            } else {
                allConfigsInvalid = true;
                displayIndex = -1;
            }
        }

        FEString *obj = static_cast<FEString *>(FEngFindObject(parent_pkg, 0x37c85b06));
        if (currentConfig == -1) {
            FEngSetInvisible(obj);
        }
        Draw();
    }

    bool IsConfigValid(int realConfigIndex) {
        if (realConfigIndex < 0 || realConfigIndex >= PS2Isp::Instance().GetNumNetworkConfigs()) {
            return false;
        }

        return configs[realConfigIndex].iType == 3;
    }

    void GetPrevValidConfig() {
        if (currentConfig != -1) {
            for (int x = currentConfig - 1; x >= -1; x--) {
                if (x == -1 || IsConfigValid(x)) {
                    currentConfig = x;
                    break;
                }
            }
            displayIndex--;
        }
    }

    void GetNextValidConfig() {
        for (int x = currentConfig + 1; x < PS2Isp::Instance().GetNumNetworkConfigs(); x++) {
            if (IsConfigValid(x)) {
                currentConfig = x;
                displayIndex++;
                return;
            }

            if (x == PS2Isp::Instance().GetNumNetworkConfigs()) {
                return;
            }
        }
    }

    void StopDrawing() {
        mShouldDraw = false;
        FEngSetInvisible(GetDataObject());
    }
};

// total size: 0x148
class UISelectNetConfig : public UIWidgetMenu {
  public:
    UISelectNetConfig(ScreenConstructorData *sd);
    ~UISelectNetConfig() override {}

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;
    eMenuSoundTriggers NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) override;

  private:
    void Setup() override;
    void GetNetworkConfigs(bool bMemoryCardExists);
    void LoadNetworkConfig(bool bCardExists);

    bool gotNetworkConfigs;  // offset 0x138, size 0x4
    uint32 ncWidgetID;       // offset 0x13C, size 0x4
    dialog_handle msgHandle; // offset 0x140, size 0x4
    bool m_boAlertInActive;  // offset 0x144, size 0x4
};

MenuScreen *CreateUIOLNetSelectScreen(ScreenConstructorData *sd) {
    return new ("UISelectNetConfig", 0) UISelectNetConfig(sd);
}

UISelectNetConfig::UISelectNetConfig(ScreenConstructorData *sd)
    : UIWidgetMenu(sd) //
    , gotNetworkConfigs(false) //
    , ncWidgetID(0) //
    , msgHandle(0) //
    , m_boAlertInActive(false) {
    Setup();

    GetNetworkConfigs(true);
}

eMenuSoundTriggers UISelectNetConfig::NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) {
    switch (msg) {
        case 0x48122792:
            return UISND_COMMON_LEFT;
        case 0x4ac5e165:
            return UISND_COMMON_LEFT;
    }
    return maybe;
}

void UISelectNetConfig::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    UIWidgetMenu::NotificationMessage(msg, pobj, param1, param2);

    switch (msg) {
        case 0xc98356ba: {
            PS2Isp &ps2isp = PS2Isp::Instance();

            if (!m_boAlertInActive && ps2isp.IsStarted() && ps2isp.IsAlertSet()) {
                PS2IspAlertT *alert = ps2isp.GetAlert(PS2ISP_NUMCONTEXTS);

                if (ps2isp.GetAlertEnum(PS2ISP_NUMCONTEXTS) == PS2ISP_ALERT_MCREMOVED) {
                    FEngSetScript(GetPackageName(), 0xed03d1d3, 0xbcbfcc87, true);
                    uint32 message = reinterpret_cast<uint32>(alert->pBody);

                    msgHandle = DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 0x96c32e55,
                                                                0x911ab364, 0x911ab364, first_dialog_button2, message);
                    m_boAlertInActive = true;
                    static_cast<NCSelector *>(GetWidget(ncWidgetID))->RefreshNetworkConfigs(GetPackageName(), false);
                }
            }
            break;
        }
        case 0x7a8ce677:
            msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x2c0e485a, 0x2c0e485a,
                                                       GetLocalizedString(0x02b00aa1));
            break;
        case 0x96c32e55:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }

            if (m_boAlertInActive) {
                PS2Isp::Instance().Finish();
                OnlineLoginScreenManager::Instance()->Start(GetPackageName(), gOL_LOGIN);
            } else {
                OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
                MemoryCard::GetInstance()->ShowTRCCheckCardMessage();
                gMemCardScreenShowing = true;
            }
            break;
        case 0x911ab364:
        case 0x2c0e485a:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }

            PS2Isp::Instance().Finish();
            FreeOnlineMemoryPool();

            if (!m_boAlertInActive && FEDatabase->GetGameplaySettings()->AutoSaveOn) {
                MemcardEnter(GetPackageName(), "MainMenu.fng", 0x80a3, nullptr, nullptr, 0, 0);
            } else {
                cFEng::Get()->QueuePackageSwitch("MainMenu.fng", 0, 0, false);
            }

            static_cast<NCSelector *>(GetWidget(ncWidgetID))->StopDrawing();
            m_boAlertInActive = false;
            break;
    }
}

void UISelectNetConfig::GetNetworkConfigs(bool bMemoryCardExists) {
    const char *package_name = GetPackageName();

    ncWidgetID = AddToggleOption(new ("NCSelector", 0) NCSelector(package_name, bMemoryCardExists, true), true);

    SetInitialOption(0);
}

void UISelectNetConfig::Setup() {}

UIOnlineLogin::UIOnlineLogin(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , m_isBuddyCBcomplete(0) //
    , msgHandle(0) //
    , m_Initialized(false) //
    , exitingScreen(false) //
    , alertActive(false) //
    , m_MemCardError(false) //
    , m_UpdateMemcardCountdown(-1) {
    LobbyLogin &lobbyLogin = LobbyLogin::Instance();

    PS2Isp &ps2isp = PS2Isp::Instance();

    if (ps2isp.IsStarted()) {
        if (ps2isp.GetContext() <= PS2ISP_CONTEXT_SELECT) {
            FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0xd5344356);
        } else {
            FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0x1db81db6);
        }
    } else if (lobbyLogin.IsStarted()) {
        FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0x1b9c916e);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0xd5344356);
    }

    m_NetConnType = 1;

    if (!gMemCardScreenShowing) {
        m_Initialized = true;
    }
}

UIOnlineLogin::~UIOnlineLogin() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
    gMemCardScreenShowing = false;
}

void UIOnlineLogin::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0xc98356ba:
            if (m_UpdateMemcardCountdown > 0) {
                m_UpdateMemcardCountdown--;
            }
            if (m_UpdateMemcardCountdown == 10) {
                MemoryCard::GetInstance()->ShowTRCLoadWarning();
            }
            if (m_UpdateMemcardCountdown == 0) {
                m_Initialized = true;
            }

            if (m_Initialized && !exitingScreen) {
                DoISPProcessing();
            }
            DoLobbyLoginProcessing();

            if (m_isBuddyCBcomplete == 2) {
                OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
                OnlineLoginScreenManager::CommitSuicide();
            }
            break;
        case 0x97584925:
            OnlineLoginScreenManager::Instance()->NoNetworkConfig = true;
            OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
            break;
        case 0x5afe12f4:
            m_UpdateMemcardCountdown = 60;
            break;
        case 0x406415e3:
        case 0x0c407210:
            if (!exitingScreen && !gMemCardScreenShowing) {
                if (!OnlineLoginScreenManager::Instance()->HasChosenPersona || LobbyLogin::Instance().IsStarted()) {
                    exitingScreen = true;
                    OnlineLoginScreenManager::Instance()->ChangeToPrevScreen();
                }
            }
            break;
        case 0x5577a88b:
            if (gMemCardScreenShowing) {
                cFEng::Get()->QueueGameMessage(0x461a18ee, nullptr, 0xff);
                m_MemCardError = true;
            } else {
                OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
            }
            break;
        case 0x7e998e5e:
            gMemCardScreenShowing = false;
            if (m_MemCardError) {
                OnlineLoginScreenManager::Instance()->ChangeToPrevScreen();
            } else {
                OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
            }
            break;
    }
}

const LobbyLoginAlertT UIOnlineLogin::lobbyAlertTable[16] = {
    {LOBBYLOGIN_ALERT_ERROR, nullptr, reinterpret_cast<char *>(0xccc6b264), nullptr},
    {LOBBYLOGIN_ALERT_DB_ERROR, nullptr, reinterpret_cast<char *>(0x4a69f7aa), nullptr},
    {LOBBYLOGIN_ALERT_FILTER, nullptr, reinterpret_cast<char *>(0xfe641264), nullptr},
    {LOBBYLOGIN_ALERT_INV_MASTER, nullptr, reinterpret_cast<char *>(0x4a69f7aa), nullptr},
    {LOBBYLOGIN_ALERT_MISSING_PARAM, nullptr, reinterpret_cast<char *>(0x9bb9ccc3), nullptr},
    {LOBBYLOGIN_ALERT_ALREADY_LOGGED_IN, nullptr, reinterpret_cast<char *>(0x6cacf1ec), nullptr},
    {LOBBYLOGIN_ALERT_LOCKED, nullptr, reinterpret_cast<char *>(0x3b33c4fc), nullptr},
    {LOBBYLOGIN_ALERT_RESERVED, nullptr, reinterpret_cast<char *>(0x2eb94b88), nullptr},
    {LOBBYLOGIN_ALERT_CONNTIMEOUT, nullptr, reinterpret_cast<char *>(0xe156e946), nullptr},
    {LOBBYLOGIN_ALERT_SERVERDOWN, nullptr, reinterpret_cast<char *>(0xccc6b264), nullptr},
    {LOBBYLOGIN_ALERT_DISC, nullptr, reinterpret_cast<char *>(0xe156e946), nullptr},
    {LOBBYLOGIN_ALERT_CHEAT_DEVICE, nullptr, reinterpret_cast<char *>(0xf5ad272b), nullptr},
    {LOBBYLOGIN_ALERT_DASHBOARD, nullptr, reinterpret_cast<char *>(0x9bb9ccc3), nullptr},
    {LOBBYLOGIN_ALERT_PERSONA_SET, nullptr, reinterpret_cast<char *>(0x6cacf1ec), nullptr},
    {LOBBYLOGIN_ALERT_INV_PASS, nullptr, reinterpret_cast<char *>(0xc042c11c), nullptr},
};

const LobbyAlertData UIOnlineLogin::lobbyAlertData[16] = {
    {LOBBYLOGIN_ALERT_ERROR, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_DB_ERROR, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_FILTER, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_INV_MASTER, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_MISSING_PARAM, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_ALREADY_LOGGED_IN, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_LOCKED, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_RESERVED, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_CONNTIMEOUT, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_SERVERDOWN, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_DISC, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_CHEAT_DEVICE, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_DASHBOARD, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_PERSONA_SET, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {LOBBYLOGIN_ALERT_INV_PASS, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
};

const PS2IspAlertT UIOnlineLogin::alertTable[30] = {
    {PS2ISP_ALERT_NOANSWER, nullptr, reinterpret_cast<char *>(0x9bb9ccc3), nullptr},
    {PS2ISP_ALERT_USERAUTH, nullptr, reinterpret_cast<char *>(0xe097de5f), nullptr},
    {PS2ISP_ALERT_BUSY, nullptr, reinterpret_cast<char *>(0x9bb9ccc3), nullptr},
    {PS2ISP_ALERT_CARRIER, nullptr, reinterpret_cast<char *>(0x9bb9ccc3), nullptr},
    {PS2ISP_ALERT_CONNECT, nullptr, reinterpret_cast<char *>(0x31d451c9), nullptr},
    {PS2ISP_ALERT_LOCATE, nullptr, reinterpret_cast<char *>(0x8645e0d4), nullptr},
    {PS2ISP_ALERT_DNSFAIL, nullptr, reinterpret_cast<char *>(0x31d451c9), nullptr},
    {PS2ISP_ALERT_HWINIT, nullptr, reinterpret_cast<char *>(0x8645e0d4), nullptr},
    {PS2ISP_ALERT_DIALTONE, nullptr, reinterpret_cast<char *>(0x9bb9ccc3), nullptr},
    {PS2ISP_ALERT_ISPTIME, nullptr, reinterpret_cast<char *>(0x31d451c9), nullptr},
    {PS2ISP_ALERT_HWINVALID, nullptr, reinterpret_cast<char *>(0xe84d4ebd), nullptr},
    {PS2ISP_ALERT_DIFFCFG, nullptr, reinterpret_cast<char *>(0x81273f29), nullptr},
    {PS2ISP_ALERT_MCREMOVED, nullptr, reinterpret_cast<char *>(0xb99f9a4e), nullptr},
    {PS2ISP_ALERT_NETCFG, nullptr, reinterpret_cast<char *>(0x9ab08da6), nullptr},
    {PS2ISP_ALERT_NOCONFIG, nullptr, reinterpret_cast<char *>(0xe84d4ebd), nullptr},
    {PS2ISP_ALERT_SOMECONFIG, nullptr, reinterpret_cast<char *>(0x08a7c977), nullptr},
    {PS2ISP_ALERT_NOMEMCARD, nullptr, reinterpret_cast<char *>(0x7a6f9259), nullptr},
    {PS2ISP_ALERT_NOFORMAT, nullptr, reinterpret_cast<char *>(0xe745d12f), nullptr},
    {PS2ISP_ALERT_NOSLOGIN, nullptr, reinterpret_cast<char *>(0x9bb9ccc3), nullptr},
    {PS2ISP_ALERT_BADSLOGIN, nullptr, reinterpret_cast<char *>(0x9bb9ccc3), nullptr},
    {PS2ISP_ALERT_DIFFCFG_MTAP, nullptr, reinterpret_cast<char *>(0x81273f29), nullptr},
    {PS2ISP_ALERT_MCREMOVED_MTAP, nullptr, reinterpret_cast<char *>(0xb99f9a4e), nullptr},
    {PS2ISP_ALERT_NETCFG_MTAP, nullptr, reinterpret_cast<char *>(0x9ab08da6), nullptr},
    {PS2ISP_ALERT_NOCONFIG_MTAP, nullptr, reinterpret_cast<char *>(0xe84d4ebd), nullptr},
    {PS2ISP_ALERT_SOMECONFIG_MTAP, nullptr, reinterpret_cast<char *>(0x08a7c977), nullptr},
    {PS2ISP_ALERT_NOMEMCARD_MTAP, nullptr, reinterpret_cast<char *>(0x7a6f9259), nullptr},
    {PS2ISP_ALERT_NOFORMAT_MTAP, nullptr, reinterpret_cast<char *>(0xe745d12f), nullptr},
    {PS2ISP_ALERT_NOSLOGIN_MTAP, nullptr, reinterpret_cast<char *>(0x9bb9ccc3), nullptr},
    {PS2ISP_ALERT_BADSLOGIN_MTAP, nullptr, reinterpret_cast<char *>(0x9bb9ccc3), nullptr},
    {PS2ISP_ALERT_ISPFAIL, nullptr, reinterpret_cast<char *>(0x31d451c9), nullptr},
};

const PS2IspAlertData UIOnlineLogin::alertData[30] = {
    {PS2ISP_ALERT_NOANSWER, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_USERAUTH, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_BUSY, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_CARRIER, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_CONNECT, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_LOCATE, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_DNSFAIL, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_HWINIT, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_DIALTONE, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_ISPTIME, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_HWINVALID, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_DIFFCFG, 0x417b2601, 0x97584925, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_MCREMOVED, 0x417b2601, 0xa0ad5efa, 0x1a294dad, 0xc9515b26},
    {PS2ISP_ALERT_NETCFG, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_NOCONFIG, 0x417b2601, 0x97584925, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_SOMECONFIG, 0x417b2601, 0x97584925, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_NOMEMCARD, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_NOFORMAT, 0x417b2601, 0x97584925, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_NOSLOGIN, 0x417b2601, 0xffffffff, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_BADSLOGIN, 0x417b2601, 0xffffffff, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_DIFFCFG_MTAP, 0x417b2601, 0x97584925, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_MCREMOVED_MTAP, 0x417b2601, 0x406415e3, 0x1a294dad, 0x406415e3},
    {PS2ISP_ALERT_NETCFG_MTAP, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_NOCONFIG_MTAP, 0x417b2601, 0x97584925, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_SOMECONFIG_MTAP, 0x417b2601, 0x97584925, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_NOMEMCARD_MTAP, 0x417b2601, 0x5577a88b, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_NOFORMAT_MTAP, 0x417b2601, 0x97584925, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_NOSLOGIN, 0x417b2601, 0xffffffff, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_BADSLOGIN_MTAP, 0x417b2601, 0xffffffff, 0xffffffff, 0xffffffff},
    {PS2ISP_ALERT_ISPFAIL, 0x417b2601, 0x406415e3, 0xffffffff, 0xffffffff},
};

void UIOnlineLogin::DoISPProcessing() {
    if (FEDatabase->GetOnlineSettings()->GetState() >= cOnlineSettings::OLS_CONNECTED_TO_ISP) {
        return;
    }

    PS2Isp &ps2isp = PS2Isp::Instance();
    if (!ps2isp.IsStarted()) {
        DoWithMainMemoryStack((void *)NetworkCore::StaticInit, 0, 0);

        PS2Isp::Instance().Start(alertTable, PS2ISP_CONTEXT_MEMCARD, false, nullptr);
    }

    if (ps2isp.IsAlertSet() || alertActive) {
        if (!alertActive) {
            alertActive = true;

            PS2IspAlertT *alert = ps2isp.GetAlert(PS2ISP_NUMCONTEXTS);
            PS2IspAlertE alertEnum = ps2isp.GetAlertEnum(PS2ISP_NUMCONTEXTS);

            FEngSetScript(GetPackageName(), 0xed03d1d3, 0xbcbfcc87, true);

            uint32 message = reinterpret_cast<uint32>(alert->pBody);
            uint32 msg1 = alertData[alertEnum].msg1;
            uint32 msg2 = alertData[alertEnum].msg2;
            uint32 button1 = alertData[alertEnum].button1;
            uint32 button2 = alertData[alertEnum].button2;

            if (alertEnum == PS2ISP_ALERT_MCREMOVED) {
                if (gMemCardScreenShowing) {
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x5577a88b, 0x5577a88b, 0x7a6f9259);
                } else {
                    alertActive = false;
                }
            } else if (button2 != 0xffffffff) {
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, msg1, msg2, button1, button2, button2,
                                                first_dialog_button1, message);
            } else {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, msg1, button1, 0, message);
            }
        }
    } else if (ps2isp.GetStatus() != PS2ISP_STATUS_IDLE) {
        PS2IspContextE context = ps2isp.GetContext();

        if (context <= PS2ISP_CONTEXT_LOADCFG) {
            FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0xd5344356);
        }
        if (context == PS2ISP_CONTEXT_SELECT) {
            FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0x1db81db6);
        }
        if (context == PS2ISP_CONTEXT_DONE) {
            FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0x1b9c916e);
        }
    } else {
        PS2IspContextE context = ps2isp.GetContext();
        OnlineLoginScreenManager *olsm = OnlineLoginScreenManager::Instance();

        if (context == PS2ISP_CONTEXT_MEMCARD) {
            ps2isp.DoContextMemcard();
        } else if (context == PS2ISP_CONTEXT_LOADCFG) {
            ps2isp.DoContextLoadCfg(false);
        } else if (context == PS2ISP_CONTEXT_SELECT) {
            if (olsm->NeedToChooseNetworkConfig == true) {
                if (gMemCardScreenShowing) {
                    MemoryCard::GetInstance()->HideTRCMessage();
                } else {
                    OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
                }
            } else {
                ps2isp.GetNumNetworkConfigs();
                ps2isp.GetNetworkConfigs();
                ps2isp.DoContextSelect(sIndexOfCurrentConfig);
            }
        } else if (context == PS2ISP_CONTEXT_DNAS) {
            if (olsm->NeedToDNASAuthenticate == true) {
                if (gMemCardScreenShowing) {
                    MemoryCard::GetInstance()->HideTRCMessage();
                } else {
                    OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
                }
            } else {
                ps2isp.DoContextDNAS(true);
            }
        } else if (context == PS2ISP_CONTEXT_DONE) {
            FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_CONNECTED_TO_ISP);
            ps2isp.Finish();
        }
    }
}

void UIOnlineLogin::DoLobbyLoginProcessing() {
    if (FEDatabase->GetOnlineSettings()->GetState() < cOnlineSettings::OLS_CONNECTED_TO_ISP ||
        FEDatabase->GetOnlineSettings()->GetState() >= cOnlineSettings::OLS_LOBBY_LOGGED_IN) {
        return;
    }

    LobbyLogin &lobbylogin = LobbyLogin::Instance();

    if (!lobbylogin.IsStarted()) {
        LobbyInit();

        LobbyLogin::Instance().Start(LobbyCore::Instance().GetLobbyApiRef(), lobbyAlertTable, LOBBYLOGIN_CONTEXT_CONNECT, nullptr,
                                     nullptr);
    }

    if (lobbylogin.IsAlertSet() || alertActive) {
        LobbyLoginAlertT *alert = lobbylogin.GetAlert(LOBBYLOGIN_NUMCONTEXTS);

        if (!alertActive) {
            alertActive = true;
            LobbyLoginAlertE alertEnum = lobbylogin.GetAlertEnum(LOBBYLOGIN_NUMCONTEXTS);
            OnlineLoginScreenManager *olsm = OnlineLoginScreenManager::Instance();

            if (alertEnum == LOBBYLOGIN_ALERT_DASHBOARD) {
                olsm->NeedToChoosePersona = true;
                olsm->ChangeToNextScreen();
            } else {
                FEngSetScript(GetPackageName(), 0xed03d1d3, 0xbcbfcc87, true);

                uint32 message = reinterpret_cast<uint32>(alert->pBody);
                uint32 msg1 = lobbyAlertData[alertEnum].msg1;
                uint32 msg2 = lobbyAlertData[alertEnum].msg2;
                uint32 button1 = lobbyAlertData[alertEnum].button1;
                uint32 button2 = lobbyAlertData[alertEnum].button2;

                if (button2 != 0xffffffff) {
                    DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, msg1, msg2, button1, button2, button2,
                                                    first_dialog_button1, message);
                } else {
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, msg1, button1, 0, message);
                }
            }
        }
    } else if (lobbylogin.GetStatus() == LOBBYLOGIN_STATUS_IDLE) {
        LobbyLoginContextE context = lobbylogin.GetContext();
        OnlineLoginScreenManager *olsm = OnlineLoginScreenManager::Instance();

        if (context == LOBBYLOGIN_CONTEXT_CONNECT) {
            olsm->m_boDoContextLogin = false;
            if (FEDatabase->IsLANMode() == true) {
                cOnlineSettings *settings = FEDatabase->GetOnlineSettings();

                olsm->EALoginOutcome = USE_FOUND_ACCOUNT;
                lobbylogin.DoContextConnect(settings->GetLobbyServerAddr(), settings->GetLobbyServerPort(), 60000);
            } else if (olsm->EALoginOutcome == OUTCOME_UNSET) {
                olsm->ChangeToNextScreen();
            } else {
                cOnlineSettings *settings = FEDatabase->GetOnlineSettings();

                lobbylogin.DoContextConnect(settings->GetLobbyServerAddr(), settings->GetLobbyServerPort(), 60000);
            }
        } else if (context == LOBBYLOGIN_CONTEXT_LOGIN) {
            if ((olsm->EALoginOutcome == USE_FOUND_ACCOUNT && bStrLen(FEDatabase->GetOnlineSettings()->GetLobbyAccountName()) > 0) ||
                (olsm->m_boDoContextLogin && bStrLen(FEDatabase->GetOnlineSettings()->GetLobbyAccountName()) > 0)) {
                lobbylogin.DoContextLogin(FEDatabase->GetOnlineSettings()->GetLobbyAccountName(),
                                          FEDatabase->GetOnlineSettings()->GetLobbyPassword());
            } else {
                olsm->ChangeToNextScreen();
            }
        } else if (context == LOBBYLOGIN_CONTEXT_TOS) {
            if (olsm->EATermsAnswered) {
                lobbylogin.DoContextTOS(olsm->EATermsAccepted);
            } else {
                olsm->ChangeToNextScreen();
            }
        } else if (context == LOBBYLOGIN_CONTEXT_SELPERS) {
            if (FEDatabase->IsLANMode()) {
                FEDatabase->GetOnlineSettings()->SetLobbyPersona(FEDatabase->GetOnlineSettings()->GetLobbyAccountName());
                lobbylogin.DoContextSelPers(FEDatabase->GetOnlineSettings()->GetLobbyAccountName());
            } else if (FEDatabase->GetOnlineSettings()->GetLobbyPersona()[0] == '\0') {
                olsm->ChangeToNextScreen();
            } else {
                lobbylogin.DoContextSelPers(FEDatabase->GetOnlineSettings()->GetLobbyPersona());
            }
        } else if (context == LOBBYLOGIN_CONTEXT_DONE) {
            TheOnlineManager.StartLobby();
            LobbyCore::Instance().SetDisconnectCallback(DisconnectCB, nullptr);
            LobbyUsers::Instance().UpdateCarName();

            if (m_isBuddyCBcomplete == 0) {
                LobbyApiMsgT Msg;
                char strLKey[256];

                lobbylogin.GetLastResponse(Msg);
                TagFieldGetString(TagFieldFind(Msg.pData, "LKEY"), strLKey, sizeof(strLKey), "");
                lobbylogin.Finish();
                FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_LOBBY_LOGGED_IN);
                LobbyCore::Instance().SetAsyncServerMessages();

                if (FEDatabase->IsLANMode()) {
                    m_isBuddyCBcomplete = 2;
                } else if (BuddyCore::instance()->getApiRef()) {
                    BuddyCore::instance()->resume();
                    FEDatabase->GetOnlineSettings()->SetBuddyServerLoggedIn(true);
                    RichPresence::Instance()->SetBuddyState(HLB_STAT_CHAT);
                    m_isBuddyCBcomplete = 2;
                } else {
                    BuddyCore::instance()->startconnect(FEDatabase->GetOnlineSettings()->GetLobbyPersona(), strLKey, BuddyConnectCB, this);
                    m_isBuddyCBcomplete = 1;
                }
            }
        }
    }
}

enum DnasMode {
    MODE_WAITING_FOR_STREAMING_TO_STOP = 0,
    MODE_WAITING_FOR_DNAS_RESULT = 1,
    MODE_ABORTING = 2,
    MODE_FAILURE = 3,
    MODE_SUCCESS = 4,
};

// total size: 0x34
class UIAuthDNAS : public MenuScreen {
  public:
    UIAuthDNAS(ScreenConstructorData *sd);
    ~UIAuthDNAS() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  private:
    void Setup();

    DnasMode currentMode; // offset 0x2C, size 0x4
    bool m_bAlertActive;  // offset 0x30, size 0x4
};

MenuScreen *CreateUIOLAuthDNASScreen(ScreenConstructorData *sd) {
    return new ("UIAuthDNAS", 0) UIAuthDNAS(sd);
}

UIAuthDNAS::UIAuthDNAS(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , currentMode(MODE_WAITING_FOR_STREAMING_TO_STOP) //
    , m_bAlertActive(false) {}

UIAuthDNAS::~UIAuthDNAS() {}

void UIAuthDNAS::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x911ab364:
        case 0x406415e3:
        case 0x0c407210:
            if (currentMode == MODE_WAITING_FOR_STREAMING_TO_STOP || currentMode == MODE_WAITING_FOR_DNAS_RESULT) {
                currentMode = MODE_ABORTING;
            }
            break;
        case 0xd9feec59:
            if (SkipOLDnas) {
                OnlineLoginScreenManager::Instance()->NeedToDNASAuthenticate = false;
                OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
            }
            break;
        case 0xc98356ba:
            if (SkipOLDnas) {
                if (currentMode == MODE_ABORTING) {
                    PS2Isp::Instance().Finish();
                    cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
                    currentMode = MODE_FAILURE;
                }
                return;
            } else {
                switch (currentMode) {
                    case MODE_WAITING_FOR_STREAMING_TO_STOP:
                        if (TheCarLoader.IsLoadingInProgress()) {
                            ServiceResourceLoading();
                        } else {
                            PS2Isp::Instance().DoContextDNAS(false);
                            currentMode = MODE_WAITING_FOR_DNAS_RESULT;
                        }
                        break;
                    case MODE_WAITING_FOR_DNAS_RESULT: {
                        PS2Isp &ps2isp = PS2Isp::Instance();

                        if (ps2isp.IsAlertSet() || m_bAlertActive) {
                            if (!m_bAlertActive) {
                                m_bAlertActive = true;

                                PS2IspAlertT *alert = ps2isp.GetAlert(PS2ISP_NUMCONTEXTS);

                                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x039df715, 0x41baf149, 0xe0c238fa,
                                                                0x52d22f3b, first_dialog_button2, "%s: %s\n%s", alert->pTitle, alert->pBody,
                                                                alert->pFooter);
                            }
                        } else if (PS2Isp::Instance().GetContext() == PS2ISP_CONTEXT_DONE) {
                            currentMode = MODE_SUCCESS;
                            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
                        }
                        break;
                    }
                    case MODE_ABORTING:
                        PS2Isp::Instance().Finish();
                        cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
                        currentMode = MODE_FAILURE;
                        break;
                    case MODE_FAILURE:
                    case MODE_SUCCESS:
                        break;
                }
            }
            break;
        case 0x52d22f3b:
            currentMode = MODE_ABORTING;
            break;
        case 0xe0c238fa:
            PS2Isp::Instance().ClearAlert(PS2ISP_NUMCONTEXTS);
            cFEng::Get()->QueuePackageSwitch(gOL_LOGIN, 0, 0, false);
            break;
        case 0xe1fde1d1:
            if (currentMode == MODE_SUCCESS) {
                OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
            } else {
                OnlineLoginScreenManager::Instance()->ChangeToPrevScreen();
            }
            break;
    }
}

// total size: 0x198
class OL_CU_EnterEmailForRequest : public FEInputWidget {
  public:
    char *pEmailData; // offset 0x194, size 0x4

    OL_CU_EnterEmailForRequest(char *init_string, int, bool) : FEInputWidget(50, init_string, 4, true) {
        pEmailData = init_string;
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0x05b2b70f);
        if (pEmailData && pEmailData[0] == '\0') {
            FEngSetLanguageHash(GetDataObject(), 0xbb298a80);
        } else {
            FEPrintf(GetDataObject(), "%s", pEmailData);
        }
    }
};

enum EmailResult {
    EMAIL_RESULT_NOTHING = 0,
    EMAIL_RESULT_BAD_ADDRESS = 1,
    EMAIL_DO_REQUEST = 2,
};

// total size: 0x180
class uiGetUserEmailAddress : public UIWidgetMenu {
  public:
    uiGetUserEmailAddress(ScreenConstructorData *sd);
    ~uiGetUserEmailAddress() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  private:
    dialog_handle msgHandle;      // offset 0x138, size 0x4
    uint32 m_emailWidgetID;       // offset 0x13C, size 0x4
    FEWidget *m_emailWidget;      // offset 0x140, size 0x4
    char m_UserEmailAddress[50];  // offset 0x144, size 0x32
    bool m_exit;                  // offset 0x178, size 0x4
    EmailResult m_emailResult;    // offset 0x17C, size 0x4
};

MenuScreen *CreateUIGetUserEmailAddress(ScreenConstructorData *sd) {
    return new ("uiGetUserEmailAddress", 0) uiGetUserEmailAddress(sd);
}

uiGetUserEmailAddress::uiGetUserEmailAddress(ScreenConstructorData *sd)
    : UIWidgetMenu(sd) //
    , msgHandle(0) //
    , m_emailWidgetID(0) //
    , m_exit(false) //
    , m_emailResult(EMAIL_RESULT_NOTHING) {
    bMemSet(m_UserEmailAddress, 0, sizeof(m_UserEmailAddress));

    m_emailWidgetID = AddInputOption(new ("OL_CU_EnterEmailForRequest", 0) OL_CU_EnterEmailForRequest(m_UserEmailAddress, 50, true));
    m_emailWidget = GetWidget(m_emailWidgetID);
    static_cast<FEInputWidget *>(m_emailWidget)->SetTitle(GetLocalizedString(0xbedf2b09));

    SetInitialOption(1);
}

uiGetUserEmailAddress::~uiGetUserEmailAddress() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
}

void uiGetUserEmailAddress::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    UIWidgetMenu::NotificationMessage(msg, pobj, param1, param2);

    bool highlightonexit = false;
    switch (msg) {
        case 0x911ab364:
            cFEng::Get()->QueuePackagePop(1);
            highlightonexit = true;
            break;
        case 0xda5b8712:
            bStrNCpy(m_UserEmailAddress, GetCurrentInputWidget()->GetInputText(), 50);
            if (!IsEmailAddressValid(m_UserEmailAddress)) {
                m_emailResult = EMAIL_RESULT_BAD_ADDRESS;
            } else {
                m_emailResult = EMAIL_DO_REQUEST;
            }
            break;
        case 0xc9d30688:
            RefreshWidgets();
            break;
        case 0xc98356ba:
            if (m_exit) {
                cFEng::Get()->QueuePackagePop(1);
                highlightonexit = true;
            } else if (m_emailResult == EMAIL_RESULT_BAD_ADDRESS) {
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                               GetLocalizedString(0x7e77208c));
                m_emailResult = EMAIL_RESULT_NOTHING;
            } else if (m_emailResult == EMAIL_DO_REQUEST) {
                LobbyAccount::Instance().RequestLostUsername(m_UserEmailAddress, nullptr, nullptr);
                DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x34dc1bec, 0x34dc1bec,
                                               GetLocalizedString(0xd4966f2c));
                RefreshWidgets();
                m_emailResult = EMAIL_RESULT_NOTHING;
            }
            break;
        case 0x406415e3:
        case 0x0c407210:
            if (pobj->NameHash == 0xd79b07a0) {
                m_exit = true;
            }
            break;
        case 0x34dc1bec:
            m_exit = true;
            break;
        case 0x2c0e485a:
            ExitOnlineMode(GetPackageName());
            break;
    }

    if (highlightonexit) {
        FEObject *button = FEngGetCurrentButton("OL_UseExisting.fng");
        if (button && button->NameHash == 0xb2980bab) {
            FEngSetScript(button, 0x249db7b7, true);
        }
    }
}

// total size: 0x100
class UIEALogin : public MenuScreen {
  public:
    // total size: 0x1C
    struct AccountScrollerDatum : public ScrollerDatum {
        eEALoginOutcomes m_eEALoginOutcome; // offset 0x18, size 0x4

        AccountScrollerDatum() : m_eEALoginOutcome(OUTCOME_UNSET) {}
        ~AccountScrollerDatum() override {}
    };

    UIEALogin(ScreenConstructorData *sd);
    ~UIEALogin() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  protected:
    void PopulateAccountSlots();
    void PopulateAccountOptions();

  private:
    void Setup();

    dialog_handle msgHandle;              // offset 0x2C, size 0x4
    bool bLoginComplete;                  // offset 0x30, size 0x4
    bool m_boExitOnline;                  // offset 0x34, size 0x4
    Scrollerina m_AccountOptionScroller;  // offset 0x38, size 0xC8
};

MenuScreen *CreateUIOLEALoginScreen(ScreenConstructorData *sd) {
    return new ("UIEALogin", 0) UIEALogin(sd);
}

UIEALogin::UIEALogin(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , msgHandle(0) //
    , bLoginComplete(false) //
    , m_boExitOnline(false) //
    , m_AccountOptionScroller(GetPackageName(), "", "", true, true, false, true) {
    if (!BuildRegion::ShowESRBNotice()) {
        FEngSetInvisible(GetPackageName(), 0xbe3c17b7);
        FEngSetInvisible(GetPackageName(), 0xca8ca439);
    }

    Setup();

    if (FEDatabase->GetOnlineSettings()->GetState() > cOnlineSettings::OLS_CONNECTED_TO_ISP) {
        LobbyDisconnect();
        TheOnlineManager.Disconnect(false);
        FEDatabase->GetOnlineSettings()->SetState(cOnlineSettings::OLS_CONNECTED_TO_ISP);
    }

    FEDatabase->GetOnlineSettings()->SetLobbyPersona("");

    OnlineLoginScreenManager::Instance()->SetDefaultFlags();
}

UIEALogin::~UIEALogin() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
}

void UIEALogin::Setup() {
    FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0xc033ea98);

    PopulateAccountSlots();
    PopulateAccountOptions();

    LobbyLogin &lobbyLogin = LobbyLogin::Instance();
    if (lobbyLogin.IsStarted() && lobbyLogin.GetContext() > LOBBYLOGIN_CONTEXT_CONNECT) {
        lobbyLogin.Finish();
        lobbyLogin.Start(LobbyCore::Instance().GetLobbyApiRef(), UIOnlineLogin::GetLobbyAlertTable(), LOBBYLOGIN_CONTEXT_CONNECT, nullptr,
                         nullptr);
    }
}

void UIEALogin::PopulateAccountSlots() {
    char szTemp[33];
    FEString *str;

    for (int i = 0; i < 3; i++) {
        ScrollerSlot *slot = new ("ScrollerSlot", 0) ScrollerSlot();

        FEngSNPrintf(szTemp, 33, "option_%d", i + 1);
        str = FEngFindString(GetPackageName(), FEHashUpper(szTemp));
        slot->AddData(str);

        m_AccountOptionScroller.AddSlot(slot);
    }
}

void UIEALogin::PopulateAccountOptions() {
    char *szName = FEDatabase->GetOnlineSettings()->GetLobbyAccountName();
    char *szPassword = FEDatabase->GetOnlineSettings()->GetLobbyPassword();
    char szTemp[33];
    char szOption[33];

    m_AccountOptionScroller.DeleteScrollerData();

    bool boAccountNameExist = szName[0] && szPassword[0];

    for (int i = 0; i < 3; i++) {
        FEngSNPrintf(szOption, 33, "option_%d", i + 1);

        AccountScrollerDatum *datum = new ("AccountScrollerDatum", 0) AccountScrollerDatum();

        switch (i) {
            case 0:
                if (boAccountNameExist) {
                    FEngSNPrintf(szTemp, 33, GetLocalizedString(0x3d0447aa), szName);
                    datum->m_eEALoginOutcome = USE_FOUND_ACCOUNT;
                } else {
                    strcpy(szTemp, GetLocalizedString(0xed21b47f));
                    datum->m_eEALoginOutcome = WANTS_NEW_ACCOUNT;
                }
                break;
            case 1:
                if (boAccountNameExist) {
                    strcpy(szTemp, GetLocalizedString(0xed21b47f));
                    datum->m_eEALoginOutcome = WANTS_NEW_ACCOUNT;
                } else {
                    strcpy(szTemp, GetLocalizedString(0x036e79b9));
                    datum->m_eEALoginOutcome = WANTS_EXISTING_ACCOUNT;
                }
                break;
            case 2:
                if (boAccountNameExist) {
                    strcpy(szTemp, GetLocalizedString(0x036e79b9));
                    datum->m_eEALoginOutcome = WANTS_EXISTING_ACCOUNT;
                } else {
                    strcpy(szTemp, "");
                    datum->m_eEALoginOutcome = OUTCOME_UNSET;
                }
                break;
        }

        FEPrintf(GetPackageName(), FEHashUpper(szOption), "%s", szTemp);
        datum->AddData(szTemp, 0);
        m_AccountOptionScroller.AddData(datum);

        if (datum->m_eEALoginOutcome == OUTCOME_UNSET) {
            m_AccountOptionScroller.Disable(datum);
        } else {
            m_AccountOptionScroller.Enable(datum);
        }
    }
}

void UIEALogin::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x0c407210:
        case 0x406415e3:
            if (OnlineLoginScreenManager::Instance()) {
                AccountScrollerDatum *datum = static_cast<AccountScrollerDatum *>(m_AccountOptionScroller.GetSelectedDatum());

                OnlineLoginScreenManager::Instance()->EALoginOutcome = datum->m_eEALoginOutcome;
            }
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            break;
        case 0x911ab364:
            msgHandle = DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 0x2c0e485a, 0x34dc1bcf,
                                                        first_dialog_button2, GetLocalizedString(0xafff7c7a));
            break;
        case 0x34dc1bcf:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }
            break;
        case 0x2c0e485a:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }
            m_boExitOnline = true;
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            break;
        case 0x72619778:
            m_AccountOptionScroller.ScrollPrev();
            break;
        case 0x911c0a4b:
            m_AccountOptionScroller.ScrollNext();
            break;
        case 0xe1fde1d1:
            if (m_boExitOnline) {
                ExitOnlineMode(GetPackageName());
            } else {
                OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
            }
            break;
        case 0xa8de4cd2:
        case 0xc98356ba:
            break;
    }
}

// total size: 0x12C
class ServerSelector : public FEToggleWidget {
  public:
    ~ServerSelector() override;
    ServerSelector(bool);
    void Act(const char *, uint32) override;
    void Draw() override;

  private:
    int currentConfig;        // offset 0x64, size 0x4
    char displayNames[3][32]; // offset 0x68, size 0x60
    char serverAddrs[3][32];  // offset 0xC8, size 0x60
    int numConfigs;           // offset 0x128, size 0x4
};

// total size: 0x774
class UIEATerms : public CUIWebOfferStart {
  public:
    UIEATerms(ScreenConstructorData *sd);
    ~UIEATerms() override {}

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;
};

MenuScreen *CreateUIOLEATermsScreen(ScreenConstructorData *sd) {
    return new ("UIEATerms", 0) UIEATerms(sd);
}

UIEATerms::UIEATerms(ScreenConstructorData *sd) : CUIWebOfferStart(sd) {}

void UIEATerms::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x911ab364:
            OnlineLoginScreenManager::Instance()->ChangeToPrevScreen();
            break;
        case 0xb19ba115: {
            OnlineLoginScreenManager *olsm = OnlineLoginScreenManager::Instance();

            if (olsm) {
                olsm->EATermsAnswered = true;
                if (param1 == 0) {
                    olsm->EATermsAccepted = true;
                    olsm->NeedToAcceptEATerms = false;
                    olsm->ChangeToNextScreen();
                } else {
                    olsm->ChangeToPrevScreen();
                }
            } else {
                cFEng::Get()->QueuePackageSwitch(gOnlineMainMenu, 0, 0, false);
            }
            break;
        }
        default:
            CUIWebOfferStart::NotificationMessage(msg, pobj, param1, param2);
            break;
    }
}

// total size: 0x14C
class UISelectPersona : public UIWidgetMenu {
  public:
    UISelectPersona(ScreenConstructorData *sd);
    ~UISelectPersona() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;
    void EnableDeletePersona(const bool &boEnable);

  private:
    void Setup() override;
    static void CreatePersonaCB(LobbyApiMsgT *msg, void *context);
    static void DeletePersonaCB(LobbyApiMsgT *msg, void *context);

    dialog_handle msgHandle;      // offset 0x138, size 0x4
    bool inDeleteMode;            // offset 0x13C, size 0x4
    bool m_boLeavePersona;        // offset 0x140, size 0x4
    int32 m_nCreatePersonaCmdID;  // offset 0x144, size 0x4
    FEWidget *pPrevOptionWidget;  // offset 0x148, size 0x4
};

MenuScreen *CreateUIOLSelectPersonaScreen(ScreenConstructorData *sd) {
    return new ("UISelectPersona", 0) UISelectPersona(sd);
}

UISelectPersona::UISelectPersona(ScreenConstructorData *sd)
    : UIWidgetMenu(sd) //
    , msgHandle(0) //
    , inDeleteMode(false) //
    , m_boLeavePersona(false) {
    bScrollWrapped = false;
    Setup();
}

UISelectPersona::~UISelectPersona() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
}

// total size: 0x198
class OLSPPersonaThing : public FEInputWidget {
  public:
    OLSPPersonaThing(bool has_persona, bool, const char *persona_name, unsigned int)
        : FEInputWidget(12, persona_name, 2, true) //
        , bHasPersona(has_persona) {
        SetBackingOffsetX(-125.0f);
        SetInputText(persona_name ? persona_name : GetLocalizedString(0x9700a5ad));
    }

    void Act(const char *parent_pkg, uint32 data) override {
        if (!bHasPersona) {
            FEInputWidget::Act(parent_pkg, data);
        }
    }

    void Draw() override {
        FEPrintf(GetTitleObject(), "                                ");
        FEPrintf(GetDataObject(), GetInputText());
    }

    bool bHasPersona; // offset 0x194, size 0x4
};

void UISelectPersona::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    UIWidgetMenu::NotificationMessage(msg, pobj, param1, param2);

    switch (msg) {
        case 0x35f8620b: {
            char temp[32];

            if (OnlineLoginScreenManager::Instance()->EALoginOutcome != USE_FOUND_ACCOUNT) {
                MemcardEnter(GetPackageName(), GetPackageName(), 0x9141, nullptr, nullptr, 0, 0);
            }

            for (int x = 1; x < 4; x++) {
                bSPrintf(temp, "option_backing_%d", x + 1);
                FEngSetScript(GetPackageName(), FEHashUpper(temp), 0x7ab5521a, true);
            }
            break;
        }
        case 0x406415e3:
        case 0x0c407210:
            if (pCurrentOption && static_cast<OLSPPersonaThing *>(pCurrentOption)->bHasPersona) {
                if (!m_boLeavePersona && !inDeleteMode) {
                    m_boLeavePersona = true;
                    FEDatabase->GetOnlineSettings()->SetLobbyPersona(static_cast<OLSPPersonaThing *>(pCurrentOption)->GetInputText());
                    cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
                }
            }
            break;
        case 0xc519bfc3:
        case 0xc519bfc0:
            if (LobbyLogin::Instance().GetPersonaList().iNumNames > 0) {
                inDeleteMode = true;
                FEngSetInvisible(GetPackageName(), 0x95016184);
                FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0x50eac1e3);
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 0x347820ba, 0x34dc1bcf,
                                                0x34dc1bcf, first_dialog_button2, GetLocalizedString(0x46527fd0));
            }
            break;
        case 0x347820ba:
            DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x70e01038, 0x417b25e4, 0x34dc1bec, 0x34dc1bcf, 0x34dc1bcf,
                                            first_dialog_button2, GetLocalizedString(0x10a27a02));
            break;
        case 0x34dc1bec: {
            OLSPPersonaThing *widge = static_cast<OLSPPersonaThing *>(pCurrentOption);

            if (widge->bHasPersona == true) {
                inDeleteMode = false;
                if (LobbyAccount::Instance().DeletePersona(widge->GetInputText(), DeletePersonaCB, this) < 0) {
                    inDeleteMode = true;
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                                   GetLocalizedString(0x167bcf0b));
                } else {
                    msgHandle =
                        DialogInterface::ShowMessage(GetPackageName(), "", dialog_alert, GetLocalizedString(0xb709aca9), widge->GetInputText());
                    EnableDeletePersona(false);
                    inDeleteMode = false;
                    FEngSetVisible(GetPackageName(), 0x95016184);
                    FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0x5dc29751);
                }
            }
            break;
        }
        case 0x34dc1bcf:
            inDeleteMode = false;
            FEngSetVisible(GetPackageName(), 0x95016184);
            FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0x5dc29751);
            break;
        case 0x911ab364:
        case 0xaa54bf29:
            if (inDeleteMode) {
                inDeleteMode = false;
                cFEng::Get()->QueueGameMessage(0xaf0bbd92, GetPackageName(), 0xff);
            } else if (bAllowScroll) {
                OnlineLoginScreenManager::Instance()->ChangeToPrevScreen();
            }
            break;
        case 0xda5b8712: {
            OLSPPersonaThing *widge = static_cast<OLSPPersonaThing *>(pCurrentOption);

            if (bStrLen(widge->GetInputText()) == 0) {
                widge->SetInputText(GetLocalizedString(0x9700a5ad));
                widge->Draw();
            } else {
                msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x1a294dad, 0x1fab5998, 0x1fab5998,
                                                           GetLocalizedString(0x17828bea), widge->GetInputText());
                m_nCreatePersonaCmdID = LobbyAccount::Instance().CreatePersona(widge->GetInputText(), CreatePersonaCB, this);
                if (m_nCreatePersonaCmdID < 0) {
                    DialogInterface::DismissDialog(msgHandle);
                    msgHandle = 0;
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                                   GetLocalizedString(0x150dfd4c));
                    widge->SetInputText(GetLocalizedString(0x9700a5ad));
                    widge->Draw();
                }
            }
            break;
        }
        case 0x1fab5998:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }
            FEDatabase->GetOnlineSettings()->SetLobbyPersona("");
            LobbyAccount::Instance().CancelPersonaCreation(m_nCreatePersonaCmdID);
            cFEng::Get()->QueueGameMessage(0xc9d30688, GetPackageName(), 0xff);
            break;
        case 0xc9d30688: {
            OLSPPersonaThing *widge = static_cast<OLSPPersonaThing *>(pCurrentOption);

            widge->SetInputText(GetLocalizedString(0x9700a5ad));
            widge->Draw();
            FEngSetScript(pobj, 0x249db7b7, true);
            break;
        }
        case 0x2c0e485b:
            ExitOnlineMode(GetPackageName());
            break;
        case 0xe1fde1d1:
            if (m_boLeavePersona && bAllowScroll) {
                OnlineLoginScreenManager::Instance()->NeedToChoosePersona = false;
                OnlineLoginScreenManager::Instance()->HasChosenPersona = true;
                OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
                m_boLeavePersona = false;
            }
            break;
        case 0xc98356ba:
            break;
    }

    if (pCurrentOption) {
        if (!pPrevOptionWidget || pPrevOptionWidget != pCurrentOption) {
            pPrevOptionWidget = pCurrentOption;
            EnableDeletePersona(static_cast<OLSPPersonaThing *>(pCurrentOption)->bHasPersona);
        }
    }
}

void UISelectPersona::EnableDeletePersona(const bool &boEnable) {
    FEObject *deletetext = FEngFindObject(GetPackageName(), 0x7ab80697);
    FEObject *deleteicon = FEngFindObject(GetPackageName(), 0xff871084);

    if (deletetext) {
        FEngSetVisibility(deletetext, boEnable);
    }
    if (deleteicon) {
        FEngSetVisibility(deleteicon, boEnable);
    }
}

void UISelectPersona::Setup() {
    LobbyLoginNameListT &personas = LobbyLogin::Instance().GetPersonaList();

    int emptySlots = 0;
    for (int i = 0; i < 4; i++) {
        if (bStrLen(personas.strNames[i]) == 0) {
            if (emptySlots == 0 && !inDeleteMode) {
                static_cast<FEInputWidget *>(GetWidget(AddInputOption(new ("OLSPPersonaThing", 0) OLSPPersonaThing(false, true, nullptr, 12))))
                    ->SetTitle(GetLocalizedString(0x72bb728e));
            }
            emptySlots++;
        } else {
            AddInputOption(new ("OLSPPersonaThing", 0) OLSPPersonaThing(true, true, personas.strNames[i], 12));
            break;
        }
    }

    SetInitialOption(0);

    uint32 titleHash = 0x5dc29751;
    if (inDeleteMode) {
        titleHash = 0x50eac1e3;
    }

    FEngSetLanguageHash(GetPackageName(), 0x03c458c1, titleHash);
}

void UISelectPersona::CreatePersonaCB(LobbyApiMsgT *msg, void *context) {
    UISelectPersona *selectPersona = static_cast<UISelectPersona *>(context);

    DialogInterface::DismissDialog(selectPersona->msgHandle);
    selectPersona->msgHandle = 0;

    if (msg->code != 0) {
        FEDatabase->GetOnlineSettings()->SetLobbyPersona("");

        if (msg->code == 'dupl') {
            DialogInterface::ShowOneButton(selectPersona->GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0x5498e449));
        } else if (msg->code == 'filt') {
            DialogInterface::ShowOneButton(selectPersona->GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0x346ca432));
        } else if (msg->code == 'iper') {
            DialogInterface::ShowOneButton(selectPersona->GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0xe16c24e3));
        } else if (msg->code == 'maxp') {
            DialogInterface::ShowOneButton(selectPersona->GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0x39b57034));
        } else if (msg->code == 'inam') {
            DialogInterface::ShowOneButton(selectPersona->GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0x4904cdde), 3, 12);
        } else {
            DialogInterface::ShowOneButton(selectPersona->GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                           GetLocalizedString(0x39b57034));
        }
    } else {
        selectPersona->EnableDeletePersona(true);
    }

    cFEng::Get()->QueueGameMessage(0xaf0bbd92, selectPersona->GetPackageName(), 0xff);
}

void UISelectPersona::DeletePersonaCB(LobbyApiMsgT *msg, void *context) {
    UISelectPersona *selectPersona = static_cast<UISelectPersona *>(context);

    DialogInterface::DismissDialog(selectPersona->msgHandle);
    selectPersona->msgHandle = 0;

    if (msg->code != 0) {
        DialogInterface::ShowOneButton(selectPersona->GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                       GetLocalizedString(0x6a4ec4ab));
    }

    cFEng::Get()->QueueGameMessage(0xaf0bbd92, selectPersona->GetPackageName(), 0xff);
}

// total size: 0x64
class OL_CU_ShareInfo : public FEToggleWidget {
  public:
    OL_CU_ShareInfo(bool) : FEToggleWidget(true) {
        SetBackingOffset(-145.0f, -65.0f);
    }

    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x9120409e || data == 0xb5971bf1) {
            OnlineLoginScreenManager::GetCreateAccountSettings()->ShareInfo = !OnlineLoginScreenManager::GetCreateAccountSettings()->ShareInfo;
        }
        Update(data);
    }

    void Draw() override {
        Position();
        uint32 hash = OnlineLoginScreenManager::GetCreateAccountSettings()->ShareInfo ? 0x70e01038 : 0x417b25e4;
        FEngSetLanguageHash(GetDataObject(), hash);
        FEngSetLanguageHash(GetTitleObject(), 0xda3cddbb);
    }
};

// total size: 0x64
class OL_CU_InformEA : public FEToggleWidget {
  public:
    OL_CU_InformEA(bool) : FEToggleWidget(true) {
        SetBackingOffset(-145.0f, -65.0f);
    }

    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x9120409e || data == 0xb5971bf1) {
            OnlineLoginScreenManager::GetCreateAccountSettings()->InformEA = !OnlineLoginScreenManager::GetCreateAccountSettings()->InformEA;
        }
        cFEng::Get()->QueueGameMessage(0x92b703b5, parent_pkg, 0xff);
        BlinkArrows(data);
        Draw();
    }

    void Draw() override {
        Position();
        uint32 hash = OnlineLoginScreenManager::GetCreateAccountSettings()->InformEA ? 0x70e01038 : 0x417b25e4;
        FEngSetLanguageHash(GetDataObject(), hash);
        FEngSetLanguageHash(GetTitleObject(), 0x9b985275);
    }
};

// total size: 0xF8
class UICreateUserCountry : public MenuScreen {
  public:
    UICreateUserCountry(ScreenConstructorData *sd);
    ~UICreateUserCountry() override {}

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  private:
    void SetupSlots();
    void Populate();

    Scrollerina m_scroller; // offset 0x2C, size 0xC8
    int m_listStartIndex;   // offset 0xF4, size 0x4
};

// total size: 0x1C
class CountryScrollerDatum : public ScrollerDatum {
  public:
    CountryScrollerDatum(char *string, int idx, unsigned int hash)
        : ScrollerDatum(string, hash) //
        , index(idx) {}
    ~CountryScrollerDatum() override {}

    int index; // offset 0x18, size 0x4
};

static char gCOUNTRY_CODES[][3] = {
    "CA", "CL", "FR", "FI", "GR", "DE", "KR", "ES", "GB", "US", "  ", "AF", "AL", "DZ", "AS", "AD", "AO", "AI", "AQ", "AG", "AR",
    "AM", "AW", "AU", "AT", "AZ", "BS", "BH", "BD", "BB", "BY", "BE", "BZ", "BJ", "BM", "BT", "BO", "BA", "BW", "BV", "BR", "IO",
    "VG", "BN", "BG", "BF", "BI", "KH", "CM", "CA", "CV", "KY", "CF", "TD", "CL", "CN", "CX", "CC", "CO", "KM", "CD", "CG", "CK",
    "CR", "HR", "CY", "CZ", "DK", "DJ", "DM", "DO", "TL", "EC", "EG", "SV", "GQ", "ER", "EE", "ET", "FO", "FK", "FJ", "FI", "FR",
    "GF", "PF", "TF", "GA", "GM", "GE", "DE", "GH", "GI", "GR", "GL", "GD", "GP", "GU", "GT", "GN", "GW", "GY", "HT", "HM", "HN",
    "HU", "IS", "IN", "ID", "IE", "IL", "IT", "CI", "JM", "JP", "JO", "KZ", "KE", "KI", "KR", "KW", "KG", "LA", "LV", "LB", "LS",
    "LR", "LI", "LT", "LU", "MK", "MG", "MW", "MY", "MV", "ML", "MT", "MH", "MQ", "MR", "MU", "YT", "MX", "FM", "MD", "MC", "MN",
    "MS", "MA", "MZ", "MM", "NA", "NR", "NP", "NL", "AN", "NC", "NZ", "NI", "NE", "NG", "NU", "NF", "MP", "NO", "OM", "PK", "PW",
    "PS", "PA", "PG", "PY", "PE", "PH", "PN", "PL", "PT", "PR", "QA", "RE", "RO", "RU", "RW", "WS", "SM", "ST", "SA", "SN", "CS",
    "SC", "SL", "SG", "SK", "SI", "SB", "SO", "ZA", "GS", "ES", "LK", "SH", "KN", "LC", "PM", "VC", "SR", "SJ", "SZ", "SE", "CH",
    "TW", "TJ", "TZ", "TH", "TG", "TK", "TO", "TT", "TN", "TR", "TM", "TC", "TV", "UG", "UA", "AE", "GB", "UM", "US", "UY", "VI",
    "UZ", "VU", "VA", "VE", "VN", "WF", "EH", "YE", "ZM", "ZW",
};

eEACreateUserOutcomes CreateAccountSettings::Validate() {
    if (bStrCmp(Password, ConfirmPassword) != 0) {
        return CREATE_USER_PASSWORDS_DONT_MATCH;
    }

    if (birthDay < 1 || birthDay > 31 || birthMonth < 1 || birthMonth > 12) {
        return CREATE_USER_INVALID_BIRTHDAY;
    }

    return CREATE_USER_ALL_IS_GOOD;
}

// total size: 0x14C
class UICreateUserPart2 : public UIWidgetMenu {
  public:
    UICreateUserPart2(ScreenConstructorData *sd);
    ~UICreateUserPart2() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  private:
    void CreateAccount();
    static void CreateAccountCB(LobbyApiMsgT *msg, void *context);
    void Setup() override;

  public:
    bool showParentEmail; // offset 0x138, size 0x4

  private:
    uint32 shareInfoID;      // offset 0x13C, size 0x4
    uint32 informEAID;       // offset 0x140, size 0x4
    bool creatingAccount;    // offset 0x144, size 0x4
    dialog_handle msgHandle; // offset 0x148, size 0x4
};

UICreateUserPart2::UICreateUserPart2(ScreenConstructorData *sd)
    : UIWidgetMenu(sd) //
    , showParentEmail(false) {
    Setup();

    ComingBack = false;

    shareInfoID = AddToggleOption(new ("OL_CU_ShareInfo", 0) OL_CU_ShareInfo(true), true);
    informEAID = AddToggleOption(new ("OL_CU_InformEA", 0) OL_CU_InformEA(true), true);

    creatingAccount = false;
    msgHandle = 0;

    SetInitialOption(1);
}

void UICreateUserPart2::Setup() {
    FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0xe33dc0e1);
    FEngSetLanguageHash(GetPackageName(), 0x03312a84, 0x8e7f3eee);
}

UICreateUserCountry::UICreateUserCountry(ScreenConstructorData *sd)
    : MenuScreen(sd) //
    , m_scroller(GetPackageName(), "Scrollbar", "Scrollbar", true, true, false, false) {
    SetupSlots();
    Populate();

    m_scroller.Update(true);
}

void UICreateUserCountry::SetupSlots() {
    char sztemp[32];
    FEString *str;

    m_scroller.DeleteScrollerData();
    m_scroller.UnHighlightSelected();

    for (int i = 0; i < 9; i++) {
        ScrollerSlot *slot = new ScrollerSlot();

        FEngSNPrintf(sztemp, 32, "OPTION_NAME_%d", i + 1);
        str = FEngFindString(GetPackageName(), FEHashUpper(sztemp));
        slot->AddData(str);

        m_scroller.AddSlot(slot);
    }
}

void UICreateUserCountry::Populate() {
    for (int i = 0; i < 241; i++) {
        CountryScrollerDatum *datum;

        if (i == 10) {
            datum = new CountryScrollerDatum("", i, 0);
            datum->Disable();
        } else {
            datum = new CountryScrollerDatum(GetLocalizedString(FEngHashString("ON_CNTRY_%s", gCOUNTRY_CODES[i])), i, 0);
        }

        m_scroller.AddData(datum);
    }

    if (OnlineLoginScreenManager::GetCreateAccountSettings()->CountryIndex > 0) {
        m_scroller.SetSelectedDatum(OnlineLoginScreenManager::GetCreateAccountSettings()->CountryIndex);
    } else {
        m_scroller.Reset(true);
    }

    m_scroller.HighlightSelected();
}

void UICreateUserCountry::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0xe1fde1d1:
            cFEng::Get()->QueuePackagePop(1);
            break;
        case 0x72619778:
            m_scroller.ScrollPrev();
            break;
        case 0x911c0a4b:
            m_scroller.ScrollNext();
            break;
        case 0x406415e3: {
            int selIdx = static_cast<CountryScrollerDatum *>(m_scroller.GetSelectedDatum())->index;

            OnlineLoginScreenManager::GetCreateAccountSettings()->CountryIndex = selIdx;
            FEngStrCpy(FEDatabase->GetOnlineSettings()->CountryCode, gCOUNTRY_CODES[selIdx]);
            cFEng::Get()->QueueGameMessage(0x3bf5b345, GetPackage()->GetParentPackage()->GetName(), 0xff);
            cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            break;
        }
        case 0xd9feec59:
            for (int i = 0; i < 9; i++) {
                m_scroller.ScrollNext();
            }
            break;
        case 0x5073ef13:
            for (int i = 0; i < 9; i++) {
                m_scroller.ScrollPrev();
            }
            break;
    }
}

UICreateUserPart2::~UICreateUserPart2() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
}

bool IsEmailAddressValid(char *pEmailAddress) {
    int8 atIndex = -1;
    int8 dotIndex = -1;

    if (bStrLen(pEmailAddress) < 7 || bStrLen(pEmailAddress) > 50) {
        return false;
    }

    for (int c = 0; c < bStrLen(pEmailAddress); c++) {
        if (pEmailAddress[c] == '@') {
            if (atIndex != -1) {
                return false;
            }

            atIndex = c;
            if (atIndex == 0 || atIndex == bStrLen(pEmailAddress) - 1) {
                return false;
            }
        } else if (pEmailAddress[c] == '.') {
            dotIndex = c;
            if (dotIndex == 0 || dotIndex == bStrLen(pEmailAddress) - 1) {
                return false;
            }
        }
    }

    if (dotIndex < atIndex || dotIndex == atIndex + 1 || atIndex == -1 || dotIndex == -1) {
        return false;
    }

    return true;
}

void UICreateUserPart2::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    UIWidgetMenu::NotificationMessage(msg, pobj, param1, param2);

    switch (msg) {
        case 0x24c7be5c:
            cFEng::Get()->QueuePackageSwitch(gOL_LOGIN, 0, 0, false);
            break;
        case 0xa365b3d5:
            CreateAccount();
            break;
        case 0x0c407210:
            if (pobj->NameHash == 0xd79b07a0) {
                DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, 0x417b2601, 0x1a294dad, 0xa365b3d5, 0xb4623f67, 0xb4623f67,
                                                first_dialog_button2, 0x7e630fac);
            }
            break;
        case 0xb4623f67:
            cFEng::Get()->QueuePackageMessage(0xabc08912, GetPackageName(), FEngFindObject(GetPackageName(), 0xd79b07a0));
            break;
        case 0x911ab364:
            ComingBack = true;
            OnlineLoginScreenManager::Instance()->ChangeToPrevScreen();
            break;
        case 0x904aca43:
            ComingBack = true;
            OnlineLoginScreenManager::Instance()->ChangeToPrevScreen();
            break;
        case 0xc9d30688:
            RefreshWidgets();
            break;
        case 0xc98356ba:
            if (ResendData == true) {
                CreateAccount();
                ResendData = false;
            } else if (showParentEmail == true) {
                if (OnlineLoginScreenManager::Instance()->hasShownParentEmail) {
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x11f8be10, 0x34d83742);
                } else {
                    cFEng::Get()->QueueGameMessage(0x11f8be10, GetPackageName(), 0xff);
                }
                showParentEmail = false;
            }
            break;
        case 0x11f8be10:
            OnlineLoginScreenManager::Instance()->hasShownParentEmail = true;
            cFEng::Get()->QueuePackageSwitch(gOL_PARENT_EMAIL, 0, 0, false);
            break;
        case 0xb34a760f: {
            CreateAccountSettings *pInfo = OnlineLoginScreenManager::GetCreateAccountSettings();

            FEDatabase->GetOnlineSettings()->SetLobbyAccountName(pInfo->AccountName);
            FEDatabase->GetOnlineSettings()->SetLobbyPassword(pInfo->Password);

            if (OnlineLoginScreenManager::GetCreateAccountSettings()->CountryIndex != 9 &&
                OnlineLoginScreenManager::GetCreateAccountSettings()->CountryIndex != 228) {
                FEDatabase->GetOnlineSettings()->ShowForeignReg = true;
                cFEng::Get()->QueuePackagePush("OL_News_and_Terms.fng", 0, 0, false);
            } else {
                cFEng::Get()->QueuePackageSwitch(gOL_LOGIN, 0, 0, false);
            }
            break;
        }
        case 0x3098bf25:
            creatingAccount = false;
            ComingBack = true;
            OnlineLoginScreenManager::Instance()->ChangeToPrevScreen();
            break;
        case 0x406415e3:
            break;
    }
}

void UICreateUserPart2::CreateAccount() {
    LobbyAccountT acct(OnlineLoginScreenManager::GetCreateAccountSettings()->AccountName,
                       OnlineLoginScreenManager::GetCreateAccountSettings()->Password,
                       OnlineLoginScreenManager::GetCreateAccountSettings()->Email,
                       OnlineLoginScreenManager::GetCreateAccountSettings()->birthDay,
                       OnlineLoginScreenManager::GetCreateAccountSettings()->birthMonth,
                       OnlineLoginScreenManager::GetCreateAccountSettings()->birthYear,
                       !OnlineLoginScreenManager::GetCreateAccountSettings()->IsFemale,
                       OnlineLoginScreenManager::GetCreateAccountSettings()->InformEA,
                       OnlineLoginScreenManager::GetCreateAccountSettings()->ShareInfo,
                       OnlineLoginScreenManager::GetCreateAccountSettings()->ParentEmail);

    if (LobbyAccount::Instance().CreateAccount(acct, CreateAccountCB, this) < 0) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff, GetLocalizedString(0x906b132a));
    } else {
        creatingAccount = true;
        msgHandle = DialogInterface::ShowMessage(GetPackageName(), "", dialog_alert, GetLocalizedString(0x44345f7f));
        OnlineLoginScreenManager::Instance()->m_boDoContextLogin = true;
    }
}

void UICreateUserPart2::CreateAccountCB(LobbyApiMsgT *msg, void *context) {
    UICreateUserPart2 *menuScreen = static_cast<UICreateUserPart2 *>(context);

    DialogInterface::DismissDialog(menuScreen->msgHandle);
    menuScreen->msgHandle = 0;

    if (msg->code == 0) {
        cFEng::Get()->QueueGameMessage(0xb34a760f, menuScreen->GetPackageName(), 0xff);
        return;
    }

    if (msg->code == 'dupl') {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                       GetLocalizedString(0x67f1d911));
    } else if (msg->code == 'dber') {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                       GetLocalizedString(0x91c92a94));
    } else if (msg->code == 'mail') {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                       GetLocalizedString(0x7e77208c));
    } else if (msg->code == 'pmal') {
        if (bStrCmp(menuScreen->GetPackageName(), gOL_PARENT_EMAIL) != 0) {
            menuScreen->showParentEmail = true;
        }
    } else if (msg->code == 'ipas') {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                       GetLocalizedString(0xd29a1a18), 4, 16);
    } else if (msg->code == 'inam') {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                       GetLocalizedString(0x4da10f66), 4, 16);
    } else if (msg->code == 'elen') {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                       GetLocalizedString(0x6864bfed), 50);
    } else if (msg->code == 'filt') {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                       GetLocalizedString(0xfe641264));
    } else if (msg->code == 'tooy') {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, GetLocalizedString(0x87983521));
    } else if (msg->code == 'izip') {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                       "I18N: Your postal code is incorrect.");
    } else if (msg->code == 'imst') {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                       "I18N: Your account name contains invalid characters");
    } else {
        DialogInterface::ShowOneButton(menuScreen->GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                       GetLocalizedString(0x2eb94b88));
    }
}

// total size: 0x194
class UIOLParentalEmail : public FEInputWidget {
  public:
    UIOLParentalEmail(const char *init_string, int, bool) : FEInputWidget(50, init_string, 4, true) {}

    void Act(const char *parent_pkg, uint32 data) override {
        FEInputWidget::Act(parent_pkg, data);
        if (data == 0xda5b8712) {
            bStrNCpy(OnlineLoginScreenManager::GetCreateAccountSettings()->ParentEmail, GetInputText(), 64);
        }
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0x0020747c);
        if (OnlineLoginScreenManager::GetCreateAccountSettings()->ParentEmail[0] == '\0') {
            FEngSetLanguageHash(GetDataObject(), 0xbb298a80);
        } else {
            FEPrintf(GetDataObject(), "%s", OnlineLoginScreenManager::GetCreateAccountSettings()->ParentEmail);
        }
    }
};

// total size: 0x138
class UIOLAgeVerf : public UIWidgetMenu {
  public:
    UIOLAgeVerf(ScreenConstructorData *sd);
    ~UIOLAgeVerf() override {}

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  private:
    void Setup() override;
};

MenuScreen *CreateUIOLAgeVerf(ScreenConstructorData *sd) {
    return new ("UIOLAgeVerif.fng", 0) UIOLAgeVerf(sd);
}

UIOLAgeVerf::UIOLAgeVerf(ScreenConstructorData *sd) : UIWidgetMenu(sd) {
    Setup();
}

void UIOLAgeVerf::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    UIWidgetMenu::NotificationMessage(msg, pobj, param1, param2);

    switch (msg) {
        case 0x406415e3:
        case 0x0c407210:
            if (pobj && pobj->NameHash == 0xd79b07a0) {
                if (IsEmailAddressValid(OnlineLoginScreenManager::GetCreateAccountSettings()->ParentEmail)) {
                    gbParentalEmailHasBeenEntered = true;
                    ResendData = true;
                    cFEng::Get()->QueuePackageMessage(0x587c018b, nullptr, nullptr);
                } else {
                    DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0x3098bf25, 0x3098bf25,
                                                   GetLocalizedString(0x34d83742));
                }
            }
            break;
        case 0x911ab364:
            ComingBack = true;
            bMemSet(OnlineLoginScreenManager::GetCreateAccountSettings()->ParentEmail, 0, 64);
            cFEng::Get()->QueuePackageMessage(0x587c018b, nullptr, nullptr);
            break;
        case 0xe1fde1d1:
            cFEng::Get()->QueuePackageSwitch(gOL_CREATE_USER_B, 0, 0, false);
            break;
        case 0xc9d30688:
        case 0xda5b8712:
            RefreshWidgets();
            break;
        case 0x81017864:
            bStrCpy(FEDatabase->GetFEKeyboardSettings()->Buffer, OnlineLoginScreenManager::GetCreateAccountSettings()->ParentEmail);
            break;
        case 0x3098bf25:
            cFEng::Get()->QueuePackageMessage(0xabc08912, GetPackageName(), FEngFindObject(GetPackageName(), 0xd79b07a0));
            break;
    }
}

void UIOLAgeVerf::Setup() {
    const char *pParentEmail = OnlineLoginScreenManager::GetCreateAccountSettings()->ParentEmail;

    AddInputOption(new ("UIOLParentalEmail", 0) UIOLParentalEmail(pParentEmail, 50, true));
    SetInitialOption(1);

    FEPrintf(GetPackageName(), FEHashUpper("info_text"), GetLocalizedString(0x371b340f));
}

// total size: 0x194
class OLUEAAccountName : public FEInputWidget {
  public:
    OLUEAAccountName(const char *init_string, unsigned int, bool) : FEInputWidget(16, init_string, 2, true) {
        SetInputText(init_string ? init_string : "");
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0x6b6538a1);
        if (bStrLen(FEDatabase->GetOnlineSettings()->GetLobbyAccountName()) == 0) {
            FEngSetLanguageHash(GetDataObject(), 0xbb298a80);
        } else {
            FEPrintf(GetDataObject(), FEDatabase->GetOnlineSettings()->GetLobbyAccountName());
        }
    }
};

// total size: 0x194
class OLUEAAccountPassword : public FEInputWidget {
  public:
    OLUEAAccountPassword(const char *init_string, unsigned int, bool) : FEInputWidget(16, init_string, 2, true) {
        SetInputText(init_string ? init_string : "");
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0xb34ab032);
        if (bStrLen(FEDatabase->GetOnlineSettings()->GetLobbyPassword()) == 0) {
            FEngSetLanguageHash(GetDataObject(), 0xbb298a80);
        } else {
            FEPrintf(GetDataObject(), "*******");
        }
    }
};

// total size: 0x40
class OLUEForgotPassword : public FEButtonWidget {
  public:
    OLUEForgotPassword(bool) : FEButtonWidget(true) {}

    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x0c407210) {
            const char *name = FEDatabase->GetOnlineSettings()->GetLobbyAccountName();

            if (name[0] == '\0') {
                DialogInterface::ShowOneButton(parent_pkg, "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                               GetLocalizedString(0x5dafd047));
            } else {
                LobbyAccount::Instance().RequestLostPassword(name, nullptr, nullptr);
                DialogInterface::ShowOneButton(parent_pkg, "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff,
                                               GetLocalizedString(0x8aa2aa13));
            }
        }
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0xadbc52f2);
    }
};

// total size: 0x40
class OLUEForgotAccountName : public FEButtonWidget {
  public:
    OLUEForgotAccountName(bool) : FEButtonWidget(true) {}

    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x0c407210) {
            cFEng::Get()->QueuePackagePush("OL_ForgotAccountName.fng", 0, 0, false);
        }
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0x03cd7c4c);
    }
};

// total size: 0x174
class UIUseExisting : public UIWidgetMenu {
  public:
    UIUseExisting(ScreenConstructorData *sd);
    ~UIUseExisting() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;
    void ContinueLoggingin();
    bool boAccountInfoFilled();

  private:
    void Setup() override;

    uint32 accountNameWidgetID;        // offset 0x138, size 0x4
    uint32 accountPasswordWidgetID;    // offset 0x13C, size 0x4
    char TrialExistingAccountName[20]; // offset 0x140, size 0x14
    char TrialExistingPassword[20];    // offset 0x154, size 0x14
    FEWidget *AccountNameWidget;       // offset 0x168, size 0x4
    FEWidget *PasswordWidget;          // offset 0x16C, size 0x4
    bool m_boTryLogin;                 // offset 0x170, size 0x4
};

MenuScreen *CreateUIOLUseExistingScreen(ScreenConstructorData *sd) {
    return new ("UIUseExisting", 0) UIUseExisting(sd);
}

UIUseExisting::UIUseExisting(ScreenConstructorData *sd)
    : UIWidgetMenu(sd) //
    , accountNameWidgetID(0) //
    , accountPasswordWidgetID(0) //
    , AccountNameWidget(nullptr) //
    , PasswordWidget(nullptr) //
    , m_boTryLogin(false) {
    bMemSet(TrialExistingAccountName, 0, sizeof(TrialExistingAccountName));
    bMemSet(TrialExistingPassword, 0, sizeof(TrialExistingPassword));

    bStrNCpy(TrialExistingAccountName, FEDatabase->GetOnlineSettings()->GetLobbyAccountName(), 16);
    bStrNCpy(TrialExistingPassword, FEDatabase->GetOnlineSettings()->GetLobbyPassword(), 16);

    Setup();
}

UIUseExisting::~UIUseExisting() {}

bool UIUseExisting::boAccountInfoFilled() {
    if (FEDatabase->GetOnlineSettings()->GetLobbyAccountName()[0] == '\0' || FEDatabase->GetOnlineSettings()->GetLobbyPassword()[0] == '\0') {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff, GetLocalizedString(0x9da1f76d));
        return false;
    }

    return true;
}

void UIUseExisting::ContinueLoggingin() {
    OnlineLoginScreenManager::Instance()->m_boDoContextLogin = true;
    OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
}

void UIUseExisting::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    UIWidgetMenu::NotificationMessage(msg, pobj, param1, param2);

    switch (msg) {
        case 0x0c407210:
            if (pobj && pobj->NameHash == 0xd79b07a0) {
                if (boAccountInfoFilled()) {
                    m_boTryLogin = true;
                    cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
                }
            }
            break;
        case 0x406415e3:
            if (boAccountInfoFilled()) {
                m_boTryLogin = true;
                cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            }
            break;
        case 0x911ab364:
            m_boTryLogin = false;
            FEDatabase->GetOnlineSettings()->SetLobbyAccountName(OnlineLoginScreenManager::Instance()->ValidExistingAccountName);
            FEDatabase->GetOnlineSettings()->SetLobbyPassword(OnlineLoginScreenManager::Instance()->ValidExistingPassword);
            OnlineLoginScreenManager::Instance()->ChangeToPrevScreen();
            break;
        case 0xda5b8712: {
            FEInputWidget *widget = GetCurrentInputWidget();

            if (widget == AccountNameWidget) {
                if (bStrLen(widget->GetInputText()) == 0) {
                    FEDatabase->GetOnlineSettings()->SetLobbyAccountName("");
                    widget->SetInputText(GetLocalizedString(0xbb298a80));
                } else {
                    FEDatabase->GetOnlineSettings()->SetLobbyAccountName(widget->GetInputText());
                }
            } else if (widget == PasswordWidget) {
                if (bStrLen(widget->GetInputText()) == 0) {
                    FEDatabase->GetOnlineSettings()->SetLobbyPassword("");
                    widget->SetInputText(GetLocalizedString(0xbb298a80));
                } else {
                    FEDatabase->GetOnlineSettings()->SetLobbyPassword(widget->GetInputText());
                }
            }

            RefreshWidgets();
            break;
        }
        case 0xc9d30688:
            RefreshWidgets();
            break;
        case 0x81017864:
            if (GetCurrentInputWidget() == AccountNameWidget) {
                bStrCpy(FEDatabase->GetFEKeyboardSettings()->Buffer, FEDatabase->GetOnlineSettings()->GetLobbyAccountName());
            }
            break;
        case 0x35f8620b:
            cFEng::Get()->FindPackage(GetPackageName())->SetControlMask(0xff);
            break;
        case 0x72619778:
            FEngSetScript(GetPackageName(), 0x444969fd, 0x0280164f, true);
            break;
        case 0x911c0a4b:
            FEngSetScript(GetPackageName(), 0x444969fe, 0x0280164f, true);
            break;
        case 0xe1fde1d1:
            if (m_boTryLogin) {
                ContinueLoggingin();
                m_boTryLogin = false;
            } else if (boAccountInfoFilled()) {
                m_boTryLogin = true;
                cFEng::Get()->QueuePackageMessage(0x587c018b, GetPackageName(), nullptr);
            }
            break;
        case 0x904aca43:
        case 0xc98356ba:
            break;
    }
}

void UIUseExisting::Setup() {
    accountNameWidgetID =
        AddInputOption(new ("OLUEAAccountName", 0) OLUEAAccountName(FEDatabase->GetOnlineSettings()->GetLobbyAccountName(), 16, true));
    accountPasswordWidgetID =
        AddInputOption(new ("OLUEAAccountPassword", 0) OLUEAAccountPassword(FEDatabase->GetOnlineSettings()->GetLobbyPassword(), 16, true));
    AddButtonOption(new ("OLUEForgotPassword", 0) OLUEForgotPassword(true));
    AddButtonOption(new ("OLUEForgotAccountName", 0) OLUEForgotAccountName(true));

    AccountNameWidget = GetWidget(accountNameWidgetID);
    static_cast<FEInputWidget *>(AccountNameWidget)->SetTitle(GetLocalizedString(0xb32f2b49));

    PasswordWidget = GetWidget(accountPasswordWidgetID);
    static_cast<FEInputWidget *>(PasswordWidget)->SetTitle(GetLocalizedString(0x332c0b6f));

    SetInitialOption(1);
}

void UIOnlineLogin::BuddyConnectCB(int status, void *context) {
    UIOnlineLogin *onlineLogin = static_cast<UIOnlineLogin *>(context);

    if (FEDatabase->GetOnlineSettings()->IsInErrorState()) {
        return;
    }

    if (status != 2) {
        if (!FEDatabase->GetOnlineSettings()->IsLoggedInToBuddyServer()) {
            DialogInterface::ShowOneButton(onlineLogin->GetPackageName(), "", dialog_alert, 0x417b2601, 0x2c0e485a, 0x13138f54);
        } else {
            HandleBuddyDisconnect();
        }
    } else {
        FEDatabase->GetOnlineSettings()->SetBuddyServerLoggedIn(true);
        BuddyCore::instance()->initVoiceAndPresence();
        onlineLogin->m_isBuddyCBcomplete = status;
    }
}

// total size: 0x194
class OL_CU_AccountName : public FEInputWidget {
  public:
    OL_CU_AccountName(char *init_string, int, bool) : FEInputWidget(16, init_string, 2, true) {}

    void Act(const char *parent_pkg, uint32 data) override {
        FEInputWidget::Act(parent_pkg, data);
        if (data == 0xda5b8712) {
            bStrNCpy(OnlineLoginScreenManager::GetCreateAccountSettings()->AccountName, GetInputText(), 64);
        }
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0x65929d97);
        if (OnlineLoginScreenManager::GetCreateAccountSettings()->AccountName[0] == '\0') {
            FEngSetLanguageHash(GetDataObject(), 0xbb298a80);
        } else {
            FEPrintf(GetDataObject(), "%s", OnlineLoginScreenManager::GetCreateAccountSettings()->AccountName);
        }
    }
};

// total size: 0x194
class OL_CU_EnterPassword : public FEInputWidget {
  public:
    OL_CU_EnterPassword(char *init_string, int, bool) : FEInputWidget(16, init_string, 2, true) {}

    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0xda5b8712) {
            bStrNCpy(OnlineLoginScreenManager::GetCreateAccountSettings()->Password, GetInputText(), 32);
        }
        FEInputWidget::Act(parent_pkg, data);
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0x0903e809);
        if (OnlineLoginScreenManager::GetCreateAccountSettings()->Password[0] == '\0') {
            FEngSetLanguageHash(GetDataObject(), 0xbb298a80);
        } else {
            FEPrintf(GetDataObject(), "*******");
        }
    }
};

// total size: 0x194
class OL_CU_ConfirmPassword : public FEInputWidget {
  public:
    OL_CU_ConfirmPassword(char *init_string, int, bool) : FEInputWidget(16, init_string, 2, true) {}

    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0xda5b8712) {
            bStrNCpy(OnlineLoginScreenManager::GetCreateAccountSettings()->ConfirmPassword, GetInputText(), 32);
        }
        FEInputWidget::Act(parent_pkg, data);
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0x99659496);
        if (OnlineLoginScreenManager::GetCreateAccountSettings()->ConfirmPassword[0] == '\0') {
            FEngSetLanguageHash(GetDataObject(), 0xbb298a80);
        } else {
            FEPrintf(GetDataObject(), "*******");
        }
    }
};

// total size: 0x194
class OL_CU_EnterEmail : public FEInputWidget {
  public:
    OL_CU_EnterEmail(char *init_string, int, bool) : FEInputWidget(50, init_string, 4, true) {}

    void Act(const char *parent_pkg, uint32 data) override {
        FEInputWidget::Act(parent_pkg, data);
        if (data == 0xda5b8712) {
            bStrNCpy(OnlineLoginScreenManager::GetCreateAccountSettings()->Email, GetInputText(), 64);
        }
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0x05b2b70f);
        if (OnlineLoginScreenManager::GetCreateAccountSettings()->Email[0] == '\0') {
            FEngSetLanguageHash(GetDataObject(), 0xbb298a80);
        } else {
            if (bStrLen(OnlineLoginScreenManager::GetCreateAccountSettings()->Email) > 18) {
                char tempbuf1[64];

                bStrNCpy(tempbuf1, OnlineLoginScreenManager::GetCreateAccountSettings()->Email, 18);
                tempbuf1[18] = '\0';
                FEPrintf(GetDataObject(), "%s...", tempbuf1);
            } else {
                FEPrintf(GetDataObject(), "%s", OnlineLoginScreenManager::GetCreateAccountSettings()->Email);
            }
        }
    }
};

// total size: 0x64
class OL_CU_EnterBirthday : public FEDateWidget {
  public:
    OL_CU_EnterBirthday(bool) : FEDateWidget(true) {
        if (OnlineLoginScreenManager::GetCreateAccountSettings()->birthDay == 0) {
            SetDate(1, 1, 1980);
        } else {
            SetDate(OnlineLoginScreenManager::GetCreateAccountSettings()->birthDay, OnlineLoginScreenManager::GetCreateAccountSettings()->birthMonth,
                    OnlineLoginScreenManager::GetCreateAccountSettings()->birthYear);
        }
    }

    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x0c407210) {
            SetPackageName(parent_pkg);
            cFEng::Get()->QueuePackagePush("UI_DateEntry.fng", reinterpret_cast<int>(this), 0, false);
        } else if (data == 0xda5b8712) {
            GetDate(&OnlineLoginScreenManager::GetCreateAccountSettings()->birthDay, &OnlineLoginScreenManager::GetCreateAccountSettings()->birthMonth,
                    &OnlineLoginScreenManager::GetCreateAccountSettings()->birthYear);
        }
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0x1492d7be);
        if (OnlineLoginScreenManager::GetCreateAccountSettings()->birthDay == 0) {
            FEngSetLanguageHash(GetDataObject(), 0xbb298a80);
        } else {
            FEDateWidget::Draw();
        }
    }

    int GetMaxYear() override {
        return 2000;
    }
};

// total size: 0x194
class OL_CU_Country : public FEInputWidget {
  public:
    OL_CU_Country(bool) : FEInputWidget(2, "", 0, true) {}

    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x0c407210) {
            cFEng::Get()->QueuePackagePush("OL_Create_Account_Country.fng", reinterpret_cast<int>(this), 0, false);
        } else if (data == 0xda5b8712) {
            OnlineLoginScreenManager::Instance()->EATermsAccepted = false;
        }
    }

    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0xf7b1f3f3);
        int countryIdx = OnlineLoginScreenManager::GetCreateAccountSettings()->CountryIndex;
        if (countryIdx == -1) {
            FEngSetLanguageHash(GetDataObject(), 0xbb298a80);
        } else {
            FEngSetLanguageHash(GetDataObject(), FEngHashString("ON_CNTRY_%s", gCOUNTRY_CODES[countryIdx]));
        }
    }
};

// total size: 0x64
class OL_CU_Gender : public FEToggleWidget {
  public:
    OL_CU_Gender(bool) : FEToggleWidget(true) {}

    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x9120409e || data == 0xb5971bf1) {
            OnlineLoginScreenManager::GetCreateAccountSettings()->IsFemale = !OnlineLoginScreenManager::GetCreateAccountSettings()->IsFemale;
        }
        Update(data);
    }

    void Draw() override {
        uint32 hash = OnlineLoginScreenManager::GetCreateAccountSettings()->IsFemale ? 0x5b5ad1c9 : 0x00193f3e;
        FEngSetLanguageHash(GetDataObject(), hash);
        FEngSetLanguageHash(GetTitleObject(), 0xcb6b7cff);
    }
};

// total size: 0x140
class UICreateUserPart1 : public UIWidgetMenu {
  public:
    UICreateUserPart1(ScreenConstructorData *sd);
    ~UICreateUserPart1() override {}

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  private:
    void Setup(CreateAccountSettings *pInfo);
    bool VerifyInformation(CreateAccountSettings *pInfo);

    uint32 m_uAccountNameWidgetId; // offset 0x138, size 0x4
    uint32 m_uEmailWidgetId;       // offset 0x13C, size 0x4
};

UICreateUserPart1::UICreateUserPart1(ScreenConstructorData *sd)
    : UIWidgetMenu(sd) //
    , m_uAccountNameWidgetId(0) //
    , m_uEmailWidgetId(0) {
    if (!ResendData && !ComingBack) {
        OnlineLoginScreenManager::GetCreateAccountSettings()->Reset();
    }
    ComingBack = false;

    Setup(OnlineLoginScreenManager::GetCreateAccountSettings());
}

void UICreateUserPart1::Setup(CreateAccountSettings *pInfo) {
    m_uAccountNameWidgetId = AddInputOption(new ("OL_CU_AccountName", 0) OL_CU_AccountName(pInfo->AccountName, 16, true));
    AddInputOption(new ("OL_CU_EnterPassword", 0) OL_CU_EnterPassword(pInfo->Password, 16, true));
    AddInputOption(new ("OL_CU_ConfirmPassword", 0) OL_CU_ConfirmPassword(pInfo->ConfirmPassword, 16, true));

    m_uEmailWidgetId = AddInputOption(new ("OL_CU_EnterEmail", 0) OL_CU_EnterEmail(pInfo->Email, 50, true));
    AddDateOption(new ("OL_CU_EnterBirthday", 0) OL_CU_EnterBirthday(true));
    AddInputOption(new ("OL_CU_Country", 0) OL_CU_Country(true));
    AddToggleOption(new ("OL_CU_Gender", 0) OL_CU_Gender(true), true);

    SetInitialOption(1);
}

bool UICreateUserPart1::VerifyInformation(CreateAccountSettings *pInfo) {
    if (!OnlineLoginScreenManager::GetCreateAccountSettings()->AllDataHasBeenGathered()) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, GetTranslatedString(0xa5cf4291));
        return false;
    }

    eEACreateUserOutcomes result = OnlineLoginScreenManager::GetCreateAccountSettings()->Validate();
    if (result == CREATE_USER_PASSWORDS_DONT_MATCH) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff, GetLocalizedString(0xff99d845));
        return false;
    } else if (result == CREATE_USER_INVALID_BIRTHDAY) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff, GetLocalizedString(0xa8da51cf));
        return false;
    }

    if (bStrLen(pInfo->AccountName) < 4 || bStrLen(pInfo->AccountName) > 16) {
        DialogInterface::ShowOk(GetPackageName(), "", dialog_alert, 0x4da10f66, 4, 16);
        return false;
    }

    if (IsDigit(pInfo->AccountName[0])) {
        DialogInterface::ShowOk(GetPackageName(), "", dialog_alert, 0x536c452d);
        return false;
    }

    if (bStrLen(pInfo->Password) < 4 || bStrLen(pInfo->Password) > 16) {
        DialogInterface::ShowOk(GetPackageName(), "", dialog_alert, 0xd29a1a18, 4, 16);
        return false;
    }

    if (!IsEmailAddressValid(pInfo->Email)) {
        DialogInterface::ShowOk(GetPackageName(), "", dialog_alert, 0x7e77208c);
        return false;
    }

    return true;
}

void UICreateUserPart1::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    UIWidgetMenu::NotificationMessage(msg, pobj, param1, param2);

    switch (msg) {
        case 0x0c407210:
            if (pobj->NameHash == 0xd79b07a0) {
                if (VerifyInformation(OnlineLoginScreenManager::GetCreateAccountSettings())) {
                    ConfigureWebOfferForTOS();
                    gbParentalEmailHasBeenEntered = false;
                    cFEng::Get()->QueuePackageMessage(0x587c018b, nullptr, nullptr);
                }
            }
            break;
        case 0x911ab364:
            if (bAllowScroll) {
                cFEng::Get()->QueuePackageMessage(0x587c018b, nullptr, nullptr);
            }
        case 0xe1fde1d1:
            if (iPrevButtonMessage == 0x911ab364) {
                OnlineLoginScreenManager::Instance()->ChangeToPrevScreen();
            } else {
                OnlineLoginScreenManager::Instance()->ChangeToNextScreen();
            }
            break;
        case 0xc9d30688:
        case 0xda5b8712:
            RefreshWidgets();
            break;
        case 0x3bf5b345:
        case 0xda52af6b:
            GetCurrentInputWidget()->Act(GetPackageName(), 0xda5b8712);
            RefreshWidgets();
            break;
        case 0x81017864: {
            FEInputWidget *currWidget = GetCurrentInputWidget();

            if (currWidget == GetWidget(m_uAccountNameWidgetId)) {
                bStrCpy(FEDatabase->GetFEKeyboardSettings()->Buffer, OnlineLoginScreenManager::GetCreateAccountSettings()->AccountName);
            } else if (currWidget == GetWidget(m_uEmailWidgetId)) {
                bStrCpy(FEDatabase->GetFEKeyboardSettings()->Buffer, OnlineLoginScreenManager::GetCreateAccountSettings()->Email);
            }
            break;
        }
    }
}

MenuScreen *CreateUIOLCreateUserScreenPart1(ScreenConstructorData *sd) {
    return new ("UICreateUserPart1", 0) UICreateUserPart1(sd);
}

MenuScreen *CreateUIOLCreateUserScreenPart2(ScreenConstructorData *sd) {
    return new ("UICreateUserPart2", 0) UICreateUserPart2(sd);
}

MenuScreen *CreateUIOLCreateUserCountry(ScreenConstructorData *sd) {
    return new ("UICreateUserCountry", 0) UICreateUserCountry(sd);
}
