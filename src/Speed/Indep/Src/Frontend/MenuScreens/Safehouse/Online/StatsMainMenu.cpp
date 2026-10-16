#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/StatsMainMenu.hpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOnlineMainMenu.hpp"

StatsMainMenu::StatsMainMenu(ScreenConstructorData *sd) : IconScrollerMenu(sd) {
    FEngSetLanguageHash(GetPackageName(), 0xb71b576d, 0xc5e9f93a);
    Setup();
    EnterRankingsMenu = false;
}

void StatsMainMenu::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (!cFEng::Get()->IsPackagePushed("OL_EAMessenger.fng")) {
        IconScrollerMenu::NotificationMessage(msg, obj, param1, param2);
    }

    switch (msg) {
        case 0x911ab364:
            cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
            break;
        case 0x6481273e:
            StartInput();
            break;
        case 0x35f8620b:
            break;
    }
}

class MMOnlineRanking : public IconOption {
  public:
    MMOnlineRanking(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {}

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            cFEng::Get()->QueuePackageSwitch("OL_Rankings.fng", 0, 0, false);
        }
    }
};

// Definido en uiOnlinePersonalRankings.cpp, que no tiene cabecera propia.
extern bool bShowPersonalStatsForOtherPlayer;

class MMOnlineRankingPersonal : public IconOption {
  public:
    MMOnlineRankingPersonal(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {}

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            bShowPersonalStatsForOtherPlayer = false;
            cFEng::Get()->QueuePackageSwitch("OL_Rankings_Personal.fng", 0, 0, false);
        }
    }
};

class MMOnlineRankingsLeaderboard : public IconOption {
  public:
    MMOnlineRankingsLeaderboard(uint32 tex_hash, uint32 name_hash, uint32 desc_hash) : IconOption(tex_hash, name_hash, desc_hash) {}

    void React(const char *pkg_name, uint32 data, FEObject *obj, uint32 param1, uint32 param2) override {
        if (data == 0x0c407210) {
            cFEng::Get()->QueuePackageSwitch("OL_Rankings_Leaderboard.fng", 0, 0, false);
        }
    }
};

void StatsMainMenu::Setup() {
    AddOption(new ("MMOnlineRanking", 0) MMOnlineRanking(0x2c14ac23, 0xd314729f, 0));
    AddOption(new ("MMOnlineRankingPersonal", 0) MMOnlineRankingPersonal(0xa2515ebd, 0xe5f757cb, 0));
    AddOption(new ("MMOnlineRankingsLeaderboard", 0) MMOnlineRankingsLeaderboard(0x9dd4bb38, 0xc93fbfb5, 0));

    SetInitialOption(1);
    RefreshHeader();
    if (Options.GetCurrentIndex() == 1) {
        FEngSetScript(GetPackageName(), 0x444969fe, 0x7ab70d67, true);
    }
}
