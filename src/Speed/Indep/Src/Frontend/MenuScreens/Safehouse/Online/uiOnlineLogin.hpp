#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINELOGIN_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINELOGIN_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Online/LobbyLogin.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bList.hpp"
#include "Speed/PSX2/Src/Online/PS2Isp.hpp"

// total size: 0x14
struct LobbyAlertData {
    uint32 check;   // offset 0x0, size 0x4
    uint32 msg1;    // offset 0x4, size 0x4
    uint32 button1; // offset 0x8, size 0x4
    uint32 msg2;    // offset 0xC, size 0x4
    uint32 button2; // offset 0x10, size 0x4
};

// total size: 0x14
struct PS2IspAlertData {
    uint32 check;   // offset 0x0, size 0x4
    uint32 msg1;    // offset 0x4, size 0x4
    uint32 button1; // offset 0x8, size 0x4
    uint32 msg2;    // offset 0xC, size 0x4
    uint32 button2; // offset 0x10, size 0x4
};

enum eEALoginOutcomes {
    OUTCOME_UNSET = -1,
    WANTS_NEW_ACCOUNT = 0,
    WANTS_EXISTING_ACCOUNT = 1,
    USE_FOUND_ACCOUNT = 2,
};

enum eEACreateUserOutcomes {
    CREATE_USER_ALL_IS_GOOD = 0,
    CREATE_USER_PASSWORDS_DONT_MATCH = 1,
    CREATE_USER_TOO_YOUNG = 2,
    CREATE_USER_PARENTAL_CONSENT_REQUIRED = 3,
    CREATE_USER_INVALID_BIRTHDAY = 4,
};

// total size: 0x11C
struct CreateAccountSettings {
    char AccountName[64];     // offset 0x0, size 0x40
    char Password[32];        // offset 0x40, size 0x20
    char ConfirmPassword[32]; // offset 0x60, size 0x20
    char Email[64];           // offset 0x80, size 0x40
    char ParentEmail[64];     // offset 0xC0, size 0x40
    int CountryIndex;         // offset 0x100, size 0x4
    int birthDay;             // offset 0x104, size 0x4
    int birthMonth;           // offset 0x108, size 0x4
    int birthYear;            // offset 0x10C, size 0x4
    bool IsFemale;            // offset 0x110, size 0x4
    bool ShareInfo;           // offset 0x114, size 0x4
    bool InformEA;            // offset 0x118, size 0x4

    eEACreateUserOutcomes Validate();
    bool AllDataHasBeenGathered() {
        if (AccountName[0] != '\0' && Password[0] != '\0' && ConfirmPassword[0] != '\0' && Email[0] != '\0') {
            return true;
        }
        return false;
    }
    void Reset() {
        bMemSet(AccountName, 0, sizeof(AccountName));
        bMemSet(Password, 0, sizeof(Password));
        bMemSet(ConfirmPassword, 0, sizeof(ConfirmPassword));
        bMemSet(Email, 0, sizeof(Email));
        bMemSet(ParentEmail, 0, sizeof(ParentEmail));
        CountryIndex = -1;
        birthDay = 0;
        birthMonth = 0;
        birthYear = 0;
        IsFemale = false;
        InformEA = false;
        ShareInfo = false;
    }
};

// total size: 0x18C
class OnlineLoginScreenManager {
  public:
    // total size: 0x1C
    struct ScreenNode : public bTNode<ScreenNode> {
        const char *Package;                                        // offset 0x8, size 0x4
        const char *(OnlineLoginScreenManager::*ForwardFunction)();  // offset 0xC, size 0x8
        const char *(OnlineLoginScreenManager::*BackwardFunction)(); // offset 0x14, size 0x8

        ScreenNode(const char *pkg, const char *(OnlineLoginScreenManager::*fwd)(),
                   const char *(OnlineLoginScreenManager::*back)()) {
            Package = pkg;
            ForwardFunction = fwd;
            BackwardFunction = back;
        }
    };

    eEALoginOutcomes EALoginOutcome;   // offset 0x0, size 0x4
    bool EATermsAnswered;              // offset 0x4, size 0x4
    bool EATermsAccepted;              // offset 0x8, size 0x4
    bool NeedToAcceptEATerms;          // offset 0xC, size 0x4
    bool TOSFromCreateAccount;         // offset 0x10, size 0x4
    bool NeedToChoosePersona;          // offset 0x14, size 0x4
    bool HasChosenPersona;             // offset 0x18, size 0x4
    bool m_boDoContextLogin;           // offset 0x1C, size 0x4
    bool hasShownParentEmail;          // offset 0x20, size 0x4
    bool NeedToChooseNetworkConfig;    // offset 0x24, size 0x4
    bool NeedToDNASAuthenticate;       // offset 0x28, size 0x4
    bool NoNetworkConfig;              // offset 0x2C, size 0x4
    char ValidExistingAccountName[20]; // offset 0x30, size 0x14
    char ValidExistingPassword[20];    // offset 0x44, size 0x14

  private:
    CreateAccountSettings mCreateAccountSettings; // offset 0x58, size 0x11C
    bTList<ScreenNode> ScreenList;                // offset 0x174, size 0x8
    ScreenNode *CurrentScreen;                    // offset 0x17C, size 0x4
    const char *Frontdoor;                        // offset 0x180, size 0x4
    const char *Backdoor;                         // offset 0x184, size 0x4

    static OnlineLoginScreenManager *pInstance;

  public:
    static OnlineLoginScreenManager *Instance();
    static void CommitSuicide();
    OnlineLoginScreenManager(const char *from_pkg, const char *front_door, const char *back_door);
    virtual ~OnlineLoginScreenManager();
    void Start(const char *from_pkg, const char *first_screen);
    static void EnterFront(const char *from_pkg);
    static void ExitToScreen(const char *);
    static CreateAccountSettings *GetCreateAccountSettings() {
        return &pInstance->mCreateAccountSettings;
    }
    void ChangeToNextScreen();
    void ChangeToPrevScreen();
    const char *GetCurrentScreen();
    void SetDefaultFlags();

  private:
    ScreenNode *FindScreen(const char *pkg);
    void AddScreen(const char *pkg, const char *(OnlineLoginScreenManager::*fwd)(),
                   const char *(OnlineLoginScreenManager::*back)());
    bool IsFrontdoor(const char *name);
    bool IsBackdoor(const char *name);
    const char *CalcNextScreenFromNetSelect();
    const char *CalcNextScreenFromOnlineLogin();
    const char *CalcNextScreenFromAuthDNAS();
    const char *CalcNextScreenFromEALogin();
    const char *CalcNextScreenFromEATerms();
    const char *CalcNextScreenFromSelectPersona();
    const char *CalcNextScreenFromCreateUserA();
    const char *CalcNextScreenFromCreateUserB();
    const char *CalcNextScreenFromUseExisting();
    const char *CalcPrevScreenFromNetSelect();
    const char *CalcPrevScreenFromOnlineLogin();
    const char *CalcPrevScreenFromAuthDNAS();
    const char *CalcPrevScreenFromEALogin();
    const char *CalcPrevScreenFromEATerms();
    const char *CalcPrevScreenFromSelectPersona();
    const char *CalcPrevScreenFromCreateUserA();
    const char *CalcPrevScreenFromCreateUserB();
    const char *CalcPrevScreenFromUseExisting();
};

// total size: 0x4C
class UIOnlineLogin : public MenuScreen {
  public:
    int m_isBuddyCBcomplete; // offset 0x2C, size 0x4

  private:
    dialog_handle msgHandle;      // offset 0x30, size 0x4
    bool m_Initialized;           // offset 0x34, size 0x4
    bool exitingScreen;           // offset 0x38, size 0x4
    int m_NetConnType;            // offset 0x3C, size 0x4
    bool alertActive;             // offset 0x40, size 0x4
    bool m_MemCardError;          // offset 0x44, size 0x4
    int m_UpdateMemcardCountdown; // offset 0x48, size 0x4

    static const PS2IspAlertT alertTable[30];
    static const PS2IspAlertData alertData[30];
    static const LobbyAlertData lobbyAlertData[16];
    static const LobbyLoginAlertT lobbyAlertTable[16];

  public:
    UIOnlineLogin(ScreenConstructorData *sd);
    ~UIOnlineLogin() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

    static const LobbyLoginAlertT *GetLobbyAlertTable() {
        return lobbyAlertTable;
    }

  private:
    void DoISPProcessing();
    void DoLobbyLoginProcessing();
    static void BuddyConnectCB(int status, void *context);
};

bool IsEmailAddressValid(char *pEmailAddress);

extern const char *gOL_NET_SELECT;
extern const char *gOL_LOGIN;
extern const char *gOL_AUTH_DNAS;
extern const char *gOL_EA_LOGIN;
extern const char *gOL_EA_TERMS;
extern const char *gOL_SELECT_PERSONA;
extern const char *gOL_CREATE_USER_A;
extern const char *gOL_CREATE_USER_B;
extern const char *gOL_CREATE_ACCOUNT_COUNTRY;
extern const char *gOL_PARENT_EMAIL;
extern const char *gOL_USE_EXISTING;
extern const char *g_ONLINE_MAIN_MENU;
extern const char *g_MAIN_MENU;

#endif
