#ifndef ONLINE_WEBOFFERUG2_HPP
#define ONLINE_WEBOFFERUG2_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>
#include <string.h>

#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/CTextScroller.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"

struct WebOfferT;

char *GetLocalizedString(uint32 hash);
int bStrToLong(const char *string);

struct WebOfferSetupT {
    char strPersona[20];
    char strFavTeam[32];
    char strProduct[32];
    char strPlatform[32];
    char strLanguage[4];
    char strCountry[4];
    char strSlusCode[32];
    char strLKey[40];
    char strSkuCode[32];
};

struct WebOfferCommandT {
    int iCommand;
    char strParms[4096];
};

struct WebOfferColorT {
    unsigned char uRed;
    unsigned char uGreen;
    unsigned char uBlue;
    unsigned char uAlpha;
};

struct WebOfferButtonT {
    char strType[1];
    char strText[40];
    WebOfferColorT Color;
};

struct WebOfferBusyT {
    char strTitle[64];
    char strMessage[1024];
    WebOfferColorT MsgColor;
    WebOfferButtonT Button[1];
};

struct WebOfferAlertT {
    char strTitle[64];
    char strImage[32];
    char strMessage[1024];
    WebOfferColorT MsgColor;
    WebOfferButtonT Button[4];
};

struct WebOfferCreditT {
    char strTitle[64];
    char strMessage[1024];
    WebOfferColorT MsgColor;
    WebOfferButtonT Button[4];
    char strFirstName[100];
    char strLastName[100];
    char strNameOnCard[200];
    char strAddress[2][100];
    char strCity[100];
    char strState[40];
    char strPostCode[40];
    char *pCountryList;
    char strCountry[50];
    char strEmail[100];
    char *pCardList;
    char strCardType[80];
    char strCardNumber[40];
    int iExpiryMonth;
    int iExpiryYear;
};

struct WebOfferPromoT {
    char strTitle[64];
    char strMessage[1024];
    WebOfferColorT MsgColor;
    WebOfferButtonT Button[4];
    char strPromo[200];
    char strPromoKPopTitle[64];
};

struct WebOfferNewsT {
    char strTitle[64];
    char strImage[32];
    WebOfferButtonT Button[4];
    WebOfferColorT NewsColor;
    char *pNews;
};

enum EProcessAction {
    eProcessAction_Nothing = 10,
    eProcessAction_AbortScript = 0,
    eProcessAction_Button1 = 1,
    eProcessAction_Button2 = 2,
    eProcessAction_Button3 = 3,
    eProcessAction_Button4 = 4
};

class CWebOffer {
  protected:
    WebOfferT *m_pWebOfferAPI;
    WebOfferCommandT *m_pCurrentCommand;
    WebOfferSetupT m_WebOfferSetup;

  private:
    bool m_bProcessingCommand;

  public:
    struct SOfferData {
        char *pPersonaName;
        char *pProductName;
        char *pPlatformName;
        const char *pLanguage;
        const char *pCountry;
        const char *pSLUSCode;
        char *pLobbyKey;
        char *pOfferURL;
    };

    CWebOffer();
    virtual ~CWebOffer();
    bool Initialise(uint32 MaxScriptSize);
    void Shutdown();
    bool Start(SOfferData &OfferData);
    void Tick();

  protected:
    virtual void StartAlert(const WebOfferAlertT &) {}
    virtual void StartPromo(const WebOfferPromoT &) {}
    virtual void StartCredit(const WebOfferCreditT &) {}
    virtual void StartHTTP(const WebOfferBusyT &) {}
    virtual void StartNews(const char *, const WebOfferNewsT &) {}
    virtual EProcessAction ProcessAlert() { return eProcessAction_AbortScript; }
    virtual EProcessAction ProcessPromo() { return eProcessAction_AbortScript; }
    virtual EProcessAction ProcessCredit() { return eProcessAction_AbortScript; }
    virtual EProcessAction ProcessHTTP() { return eProcessAction_AbortScript; }
    virtual EProcessAction ProcessNews() { return eProcessAction_AbortScript; }
    virtual void EndAlert() {}
    virtual void EndPromo() {}
    virtual void EndCredit() {}
    virtual void EndHTTP() {}
    virtual void EndNews() {}
    virtual bool FillInPromoSubmitData(char *, int) { return false; }
    virtual bool FillInCreditCardSubmitData(WebOfferCreditT &) { return false; }
    virtual void Finished(int) {}

  private:
    void StartNextCommand();
    void ProcessCommand();
    void _StartAlert();
    void _StartPromo();
    void _StartCredit();
    void _StartHTTP();
    void _StartNews();
    void _ProcessAlert();
    void _ProcessPromo();
    void _ProcessCredit();
    void _ProcessHTTP();
    void _ProcessNews();
    void _Finished();
};

typedef int dialog_handle;

struct SFEngNewsScreenData {
    MenuScreen *pParentScreen;
    char *pNewsText;
    WebOfferNewsT WebOfferNews;
};

struct SButtonInit {
    char *pButtonText;
};

struct SButtons {
    uint32 NameHash;
    uint32 HighlightNameHash;
    EProcessAction Action;
};

struct CUIWebOfferScreen : MenuScreen {
  protected:
    SButtons m_Buttons[2];
    uint8 m_NumButtons;

  public:
    CUIWebOfferScreen(ScreenConstructorData *sd)
        : MenuScreen(sd) //
        , m_NumButtons(0) {}
    virtual ~CUIWebOfferScreen() {}
    void InitialiseButtons();
    EProcessAction TestButtonMessage(uint32 msg);
};

struct CUIWebOfferNews : CUIWebOfferScreen {
  private:
    MenuScreen *m_pOwner;
    FEScrollBar m_ScrollBar;
    CTextScroller m_TextScroller;
    int m_iLastTopLine;

  public:
    CUIWebOfferNews(ScreenConstructorData *sd)
        : CUIWebOfferScreen(sd) //
        , m_ScrollBar(sd->PackageFilename, "Scrollbar", true, false, false) {}
    ~CUIWebOfferNews() override;
    void NotificationMessage(uint32 msg, FEObject *obj, uint32 param1, uint32 param2) override;
    eMenuSoundTriggers NotifySoundMessage(uint32 msg, eMenuSoundTriggers maybe) override;

    friend MenuScreen *CreateOnlineNews(ScreenConstructorData *sd);
};

struct CUIWebOfferForeignReg : CUIWebOfferScreen {
  private:
    FEScrollBar m_ScrollBar;
    CTextScroller m_TextScroller;

  public:
    CUIWebOfferForeignReg(ScreenConstructorData *sd)
        : CUIWebOfferScreen(sd) //
        , m_ScrollBar(sd->PackageFilename, "Scrollbar", true, false, false) {}
    ~CUIWebOfferForeignReg() override {}
    void NotificationMessage(uint32 msg, FEObject *obj, uint32 param1, uint32 param2) override;

    friend MenuScreen *CreateForeignReg(ScreenConstructorData *sd);
};

class CWebOfferUG2 : public CWebOffer {
    SFEngNewsScreenData m_NewsData;
    WebOfferAlertT m_AlertData;

  private:
    EProcessAction m_PendingAction;
    char *m_pCurrentFEngPackage;
    MenuScreen *m_pOwner;
    dialog_handle m_Dialog;
    bool m_bAlertDialogPopulated;

  public:
    CWebOfferUG2()
        : m_PendingAction(eProcessAction_Nothing) //
        , m_pCurrentFEngPackage(nullptr) //
        , m_pOwner(nullptr) //
        , m_Dialog(0) //
        , m_bAlertDialogPopulated(false) {}
    void SetOwner(MenuScreen *pOwner) { m_pOwner = pOwner; }
    void SetPendingAction(EProcessAction action) { m_PendingAction = action; }
    void DismissDialog();
    char *DecodeString(const char *pString, bool *pbWasLabel) {
        if (pString && pString[0] == '0' && pString[1] == 'x') {
            int StringLabel = bStrToLong(pString);
            *pbWasLabel = true;
            return GetLocalizedString(StringLabel);
        }
        *pbWasLabel = false;
        return const_cast<char *>(pString);
    }

  protected:
    void StartAlert(const WebOfferAlertT &AlertData) override;
    void StartHTTP(const WebOfferBusyT &BusyData) override;
    void StartNews(const char *pNewsText, const WebOfferNewsT &NewsData) override;
    EProcessAction ProcessAlert() override;
    EProcessAction ProcessHTTP() override;
    EProcessAction ProcessNews() override;
    void EndAlert() override;
    void EndHTTP() override;
    void EndNews() override;
    void Finished(int ResultCode) override;
    void PopulateAlertDialog();
};

class CUIWebOfferStart : public MenuScreen {
  private:
    CWebOfferUG2 m_WebOffer;
    MenuScreen *m_pCurrentWebOfferScreen;
    static char m_NextPackageSuccess[32];
    static char m_NextPackageFail[32];
    static char m_WebOfferScript[512];

  public:
    CUIWebOfferStart(ScreenConstructorData *screenConstructorData)
        : MenuScreen(screenConstructorData) //
        , m_pCurrentWebOfferScreen(nullptr) {
        m_WebOffer.SetOwner(this);
    }
    ~CUIWebOfferStart() override {
        m_WebOffer.DismissDialog();
        m_WebOffer.Shutdown();
    }
    void Shutdown() {
        m_WebOffer.Shutdown();
    }
    static void SetNextPackages(const char *pSuccessPackage, const char *pFailPackage) {
        bStrNCpy(m_NextPackageSuccess, pSuccessPackage, 31);
        bStrNCpy(m_NextPackageFail, pFailPackage, 31);
    }
    static void MakeURLScript2(const char *url, uint32 titleHash, uint32 messageHash);
#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_XENON) // EGM: 0x822720D0 (MMOnlineNews/TOS::React)
    // A124: stab MakeURLScript::f22:PCciiii y cuerpo inline (WebOfferUG2.hpp:261-283) que expanden
    // MMOnlineNews::React y MMOnlineTOS::React (uionlinemainmenu.cpp). pURL, TotalStrLen, FudgedURL y
    // pURLScriptFormat salen de los stabs del bloque inline; los cuatro int llegan como constantes y no dejan
    // nombre: son los cuatro campos 0x%x del guion (TIME=30 y $exit=-1 van fijos; 36 = 4*8 + 2 + 2).
    static void MakeURLScript(const char *pURL, int busyMessage, int failTitle, int failMessage, int failButton) {
        int TotalStrLen;
        char FudgedURL[32];
        const char *pURLScriptFormat =
            "%%{ CMD=http\nTIME=%d\nMESG=0x%x\nURL-GET=%s\nSUCCESS-GOTO=$http\nBTN1-GOTO=$exit=-1\n%%}\n"
            "%%{FAILURE CMD=alrt\nTITLE=0x%x\nMESG=0x%x\nBTN1=0x%x\nBTN1-GOTO=$exit=%d\n%%}";

        if (strlen(pURL) == 0) {
            strcpy(FudgedURL, "http://1mdi71");
            pURL = FudgedURL;
        }

        TotalStrLen = strlen(pURLScriptFormat) + 36;
        if (TotalStrLen < 512) {
            bSPrintf(m_WebOfferScript, pURLScriptFormat, 30, busyMessage, pURL, failTitle, failMessage, failButton, -1);
        }
    }
#else
    static void MakeURLScript(const char *url);
#endif
    static int MakeDialogButtonHash(int buttonIndex);
    void NotificationMessage(uint32 Message, FEObject *pObject, uint32 Param1, uint32 Param2) override {
        switch (Message) {
            case 0xd15504ec:
            case 0xd15554ec:
            case 0xd25504ec:
                break;
            case 0xd15c04ec:
                m_WebOffer.DismissDialog();
                m_WebOffer.Shutdown();
                break;
            case 0xc98356ba:
                m_WebOffer.Tick();
                break;
            case 0xb19b00b5:
                m_pCurrentWebOfferScreen = reinterpret_cast<MenuScreen *>(Param1);
                break;
            case 0xdead900d: {
                EProcessAction Action = static_cast<EProcessAction>(Param1);
                m_WebOffer.SetPendingAction(Action);
                break;
            }
            case 0xd09f00d5:
                if (m_pCurrentWebOfferScreen) {
                    m_pCurrentWebOfferScreen->NotificationMessage(0xb19ba115, nullptr, 0, 0);
                }
                break;
            case 0xb19ba115:
                if (Param1 == 0) {
                    cFEng::Get()->QueuePackageSwitch(m_NextPackageSuccess, 0, 0, false);
                } else {
                    cFEng::Get()->QueuePackageSwitch(m_NextPackageFail, 0, 0, false);
                }
                break;
            default: {
                int Button = 0x0eb0ffe0;
                if ((Message & Button) == Button) {
                    EProcessAction Action = static_cast<EProcessAction>(Message & ~Button);
                    m_WebOffer.SetPendingAction(Action);
                }
                break;
            }
        }
    }

    friend void ConfigureWebOfferForTOS();
    friend MenuScreen *CreateOnlineWebOfferScreen(ScreenConstructorData *sd);
};

void ConfigureWebOfferForTOS();
MenuScreen *CreateOnlineNews(ScreenConstructorData *sd);
MenuScreen *CreateOnlineWebOfferScreen(ScreenConstructorData *sd);
MenuScreen *CreateForeignReg(ScreenConstructorData *sd);

#endif
