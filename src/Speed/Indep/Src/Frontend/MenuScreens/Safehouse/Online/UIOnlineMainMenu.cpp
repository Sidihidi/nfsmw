#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOnlineMainMenu.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineDisconnect.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/uiMain.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/WebOfferUG2.hpp"

UIOnlineMainMenu::UIOnlineMainMenu(ScreenConstructorData *sd)
    : IconScrollerMenu(sd) //
    , m_exitingOnline(false) //
    , m_dialogHandle(0) {
    FEPrintf(GetPackageName(), 0x5e7b09c9, " ");
    FEngSetLanguageHash(GetPackageName(), 0xb71b576d, 0xca38d55f);
    Setup();
}

void UIOnlineMainMenu::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (!m_dialogHandle && !cFEng::Get()->IsPackagePushed("OL_EAMessenger.fng")) {
        IconScrollerMenu::NotificationMessage(msg, obj, param1, param2);
    }

    switch (msg) {
        case 0x35f8620b:
        case 0x97c1b72f:
        case 0xbb6aa4b5:
        case 0xe1fde1d1:
        case 0xf9e94d58:
            break;
        case 0x911ab364:
            if (FEDatabase->IsLANMode()) {
                if (FEDatabase->GetOnlineSettings()->IsUserHostingLANServer) {
                    m_dialogHandle = DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_YES,
                                                                     LANGUAGE_COMMON_NO, 0xd05fc3a3, 0x34dc1bcf, 0x34dc1bcf,
                                                                     first_dialog_button1, 0x309b91ba);
                } else {
                    m_dialogHandle = DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_YES,
                                                                     LANGUAGE_COMMON_NO, 0xd05fc3a3, 0x34dc1bcf, 0x34dc1bcf,
                                                                     first_dialog_button1, 0xc2e59520);
                }
            } else {
                m_dialogHandle = DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_YES,
                                                                 LANGUAGE_COMMON_NO, 0xd05fc3a3, 0x34dc1bcf, 0x34dc1bcf,
                                                                 first_dialog_button2, 0xee46bda4);
            }
            DialogInterface::SetDismissedByAnotherDialogMsg(0x34dc1bcf);
            break;
        case 0x6481273e:
            StartInput();
            break;
        case 0xd05fc3a3:
            m_dialogHandle = 0;
            ExitOnlineMode(GetPackageName());
            m_exitingOnline = true;
            break;
        case 0x34dc1bcf:
            m_dialogHandle = 0;
            StartInput();
            break;
        case 0x904aca43:
            if (FEDatabase->IsLANMode()) {
                cFEng::Get()->QueuePackageMessage(0xd05fc3a3, GetPackageName(), nullptr);
            }
            break;
    }
}

class MMOnlineNews : public IconOption {
  public:
    MMOnlineNews(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {}

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        FEngFindScreen(gOnlineMainMenu);
        if (data == 0x0c407210) {
            char *config = LobbyCore::Instance().GetServerConfig();
            char NEWSURLFromServerConfig[256];

            NEWSURLFromServerConfig[0] = '\0';
            if (config) {
                TagFieldGetString(TagFieldFind(config, "NEWS_URL"), NEWSURLFromServerConfig, 255, "");
            }
            CUIWebOfferStart::MakeURLScript(NEWSURLFromServerConfig, 0x783f8d11, 0, 0x3efe5317, LANGUAGE_COMMON_OK);
            CUIWebOfferStart::SetNextPackages(gOnlineMainMenu, gOnlineMainMenu);
            cFEng::Get()->QueuePackageSwitch("UI_OL_WebOffer2.fng", 0, 0, false);
        }
    }
};

class MMOnlineTOS : public IconOption {
  public:
    MMOnlineTOS(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {}

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        FEngFindScreen(gOnlineMainMenu);
        if (data == 0x0c407210) {
            char *config = LobbyCore::Instance().GetServerConfig();
            char TOSURLFromServerConfig[256];

            TOSURLFromServerConfig[0] = '\0';
            if (config) {
                TagFieldGetString(TagFieldFind(config, "TOSA_URL"), TOSURLFromServerConfig, 255, "");
            }
            CUIWebOfferStart::MakeURLScript(TOSURLFromServerConfig, 0x780207ca, 0, 0x9016a670, LANGUAGE_COMMON_OK);
            CUIWebOfferStart::SetNextPackages(gOnlineMainMenu, gOnlineMainMenu);
            cFEng::Get()->QueuePackageSwitch("UI_OL_WebOffer2.fng", 0, 0, false);
        }
    }
};

class MMOnlineQuickRace : public IconOption {
  public:
    MMOnlineQuickRace(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {}

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            cFEng::Get()->QueuePackageSwitch("OL_Quickrace_Main.fng", 0, 0, false);
        }
    }
};

class MMOnlineOptiMatch : public IconOption {
  public:
    MMOnlineOptiMatch(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {}

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            cFEng::Get()->QueuePackageSwitch("UI_OLX_OptiMatch_Filters.fng", 0, 0, false);
        }
    }
};

class MMOnlineCreateGame : public IconOption {
  public:
    MMOnlineCreateGame(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {}

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            FEDatabase->GetOnlineSettings()->GameDetails = eHOSTING;
            FEDatabase->SetGameMode(eFE_GAME_MODE_MODE_SELECT);
            cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
        }
    }
};

class MMOnlineSelectCar : public IconOption {
  public:
    MMOnlineSelectCar(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {}

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            cFEng::Get()->QueuePackageSwitch("Car_Select.fng", 0, 0, false);
        }
    }
};

bool EnterRankingsMenu = false;

class MMOnlineRankingSubMenu : public IconOption {
  public:
    MMOnlineRankingSubMenu(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {}

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            EnterRankingsMenu = true;
            cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
        }
    }
};

void UIOnlineMainMenu::Setup() {
    AddOption(new ("MMOnlineQuickRace", 0) MMOnlineQuickRace(0x23d40757, 0x54020a7a, 0));
    AddOption(new ("MMOnlineOptiMatch", 0) MMOnlineOptiMatch(0x6b0c3634, 0x98dce70c, 0));
    AddOption(new ("MMOnlineCreateGame", 0) MMOnlineCreateGame(0x56026748, 0x972dd7ed, 0));
    AddOption(new ("MMOnlineSelectCar", 0) MMOnlineSelectCar(0x9c0a19e0, 0xc0bedf15, 0));
    if (!FEDatabase->IsLANMode()) {
        AddOption(new ("MMOnlineRankingSubMenu", 0) MMOnlineRankingSubMenu(0xda665928, 0x8106d26a, 0));
    }
    if (!FEDatabase->IsLANMode()) {
        AddOption(new ("MMOnlineNews", 0) MMOnlineNews(0x114f5968, 0x4815fdbd, 0));
        AddOption(new ("MMOnlineTOS", 0) MMOnlineTOS(0x3ba703b6, 0x25c9afe9, 0));
    }

    SetInitialOption(FEngGetLastButton(GetPackageName()));
    RefreshHeader();
    if (Options.GetCurrentIndex() == 1) {
        FEngSetScript(GetPackageName(), 0x444969fe, 0x7ab70d67, true);
    }
}
