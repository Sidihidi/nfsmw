#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feUIWidgetMenu.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feWidget.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineLogin.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"

// Definido en uiOnlineMain.cpp.
extern const char *g_LOBBY_ROOM_SCREEN;

bool filterButtonPressed = false;
FilterGameSessionParamsT OnlineSessionFilter(1000, GRace::kRaceType_NumTypes, ANY, ANY, ANY, OLS_DISCONNECT_PERC_ANY);

// total size: 0x64
class OL_GameModeWidget : public FEToggleWidget {
  public:
    OL_GameModeWidget(bVector2 &, bVector2 &, bVector2 &, bVector2 &, bool enabled) : FEToggleWidget(enabled) {
        SetBackingOffsetX(-235.0f);
    }

    void Act(const char *parent_pkg, uint32 data) override {
        int mode = OnlineSessionFilter.gameMode;

        if (data == 0x9120409e) {
            if (mode == GRace::kRaceType_P2P) {
                mode = GRace::kRaceType_NumTypes;
            } else {
                mode--;
                if (mode < GRace::kRaceType_P2P) {
                    mode = GRace::kRaceType_P2P;
                }
                if (mode > GRace::kRaceType_Drag) {
                    mode = GRace::kRaceType_Drag;
                }
            }
        } else if (data == 0xb5971bf1) {
            if (mode == GRace::kRaceType_NumTypes) {
                mode = GRace::kRaceType_P2P;
            } else {
                mode++;
                if (mode > GRace::kRaceType_Drag) {
                    mode = GRace::kRaceType_NumTypes;
                }
            }
        }

        OnlineSessionFilter.gameMode = static_cast<GRace::Type>(mode);
        Update(data);

        cFEng::Get()->QueueGameMessage(0x92b703b5, parent_pkg, 0xff);
    }

    void Draw() override {
        int hash = 0;

        switch (OnlineSessionFilter.gameMode) {
            case GRace::kRaceType_Circuit: hash = 0x034fa2c1; break;
            case GRace::kRaceType_P2P: hash = 0xb94fd70e; break;
            case GRace::kRaceType_Drag: hash = 0x6f547e4c; break;
            case GRace::kRaceType_NumTypes: hash = 0x7105fb23; break;
        }

        FEngSetLanguageHash(GetDataObject(), hash);
        FEngSetLanguageHash(GetTitleObject(), 0xfd81b9f7);
    }
};

// total size: 0x64
class OL_RankedGameWidget : public FEToggleWidget {
  public:
    OL_RankedGameWidget(bVector2 &, bVector2 &, bVector2 &, bVector2 &, bool enabled) : FEToggleWidget(enabled) {
        SetBackingOffsetX(-235.0f);
    }

    void Act(const char *parent_pkg, uint32 data) override {
        int value = OnlineSessionFilter.rankedGames;

        if (data == 0x9120409e) {
            value--;
            if (value < YES) {
                value = ANY;
            }
        } else if (data == 0xb5971bf1) {
            value++;
            if (value > ANY) {
                value = YES;
            }
        }

        OnlineSessionFilter.rankedGames = static_cast<YesNoAny>(value);
        Update(data);
    }

    void Draw() override {
        int hash = 0;

        switch (OnlineSessionFilter.rankedGames) {
            case YES:
                hash = 0x66cf56ae;
                break;
            case NO:
                hash = 0x0e1f1271;
                break;
            case ANY:
                hash = 0x7105fb23;
                break;
        }

        FEngSetLanguageHash(GetDataObject(), hash);
        FEngSetLanguageHash(GetTitleObject(), 0xfd85bcb4);
    }
};

// total size: 0x64
class OL_CollisionDetectionWidget : public FEToggleWidget {
  public:
    OL_CollisionDetectionWidget(bVector2 &, bVector2 &, bVector2 &, bVector2 &, bool enabled) : FEToggleWidget(enabled) {
        SetBackingOffsetX(-235.0f);
    }

    void Act(const char *parent_pkg, uint32 data) override {
        int value = OnlineSessionFilter.collisionDetection;

        if (data == 0x9120409e) {
            value--;
            if (value < YES) {
                value = ANY;
            }
        } else if (data == 0xb5971bf1) {
            value++;
            if (value > ANY) {
                value = YES;
            }
        }

        OnlineSessionFilter.collisionDetection = static_cast<YesNoAny>(value);
        Update(data);
    }

    void Draw() override {
        int hash = 0;

        switch (OnlineSessionFilter.collisionDetection) {
            case YES: hash = 0x417b2604; break;
            case NO: hash = 0x70dfe5c2; break;
            case ANY: hash = 0x7105fb23; break;
        }

        FEngSetLanguageHash(GetDataObject(), hash);
        FEngSetLanguageHash(GetTitleObject(), 0x7712e575);
    }
};

// total size: 0x64
class OL_PerformanceMatchingWidget : public FEToggleWidget {
  public:
    OL_PerformanceMatchingWidget(bVector2 &, bVector2 &, bVector2 &, bVector2 &, bool enabled) : FEToggleWidget(enabled) {
        SetBackingOffsetX(-235.0f);
    }

    void Act(const char *parent_pkg, uint32 data) override {
        int value = OnlineSessionFilter.performanceMatching;

        if (data == 0x9120409e) {
            value--;
            if (value < YES) {
                value = ANY;
            }
        } else if (data == 0xb5971bf1) {
            value++;
            if (value > ANY) {
                value = YES;
            }
        }

        OnlineSessionFilter.performanceMatching = static_cast<YesNoAny>(value);
        Update(data);
    }

    void Draw() override {
        int hash = 0;

        switch (OnlineSessionFilter.performanceMatching) {
            case YES: hash = 0x417b2604; break;
            case NO: hash = 0x70dfe5c2; break;
            case ANY: hash = 0x7105fb23; break;
        }

        FEngSetLanguageHash(GetDataObject(), hash);
        FEngSetLanguageHash(GetTitleObject(), 0x711179f7);
    }
};

// total size: 0x64
class OL_PasswordedWidget : public FEToggleWidget {
  public:
    OL_PasswordedWidget(bVector2 &, bVector2 &, bVector2 &, bVector2 &, bool enabled) : FEToggleWidget(enabled) {}

    void Act(const char *parent_pkg, uint32 data) override;
    void Draw() override;
};

// total size: 0x13C
class UIOnlineFilters : public UIWidgetMenu {
  public:
    UIOnlineFilters(ScreenConstructorData *sd);
    ~UIOnlineFilters() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup() override;

    dialog_handle msgHandle; // offset 0x138, size 0x4
};

MenuScreen *CreateOnlineFilters(ScreenConstructorData *sd) {
    return new ("UIOnlineFilters", 0) UIOnlineFilters(sd);
}

UIOnlineFilters::UIOnlineFilters(ScreenConstructorData *sd) : UIWidgetMenu(sd), msgHandle(0) {
    if (TheOnlineManager.GetState() != OLS_LOBBY_IN_LOBBY) {
        LobbyCore::Instance().Resume(true);
        TheOnlineManager.StartLobby();
    }

    Setup();

    if (!FEDatabase->IsLANMode()) {
        AddToggleOption(new ("OL_RankedGameWidget", 0) OL_RankedGameWidget(vLastWidgetPos, vMaxTitleSize, vMaxDataSize, vDataPos, true), true);
    }
    AddToggleOption(new ("OL_GameModeWidget", 0) OL_GameModeWidget(vLastWidgetPos, vMaxTitleSize, vMaxDataSize, vDataPos, true), true);
    AddToggleOption(new ("OL_CollisionDetectionWidget", 0)
                        OL_CollisionDetectionWidget(vLastWidgetPos, vMaxTitleSize, vMaxDataSize, vDataPos, true),
                    true);
    AddToggleOption(new ("OL_PerformanceMatchingWidget", 0)
                        OL_PerformanceMatchingWidget(vLastWidgetPos, vMaxTitleSize, vMaxDataSize, vDataPos, true),
                    true);

    SetInitialOption(0);
}

UIOnlineFilters::~UIOnlineFilters() {
    if (msgHandle) {
        DialogInterface::DismissDialog(msgHandle);
        msgHandle = 0;
    }
}

void UIOnlineFilters::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    UIWidgetMenu::NotificationMessage(msg, obj, param1, param2);

    switch (msg) {
        case 0x911ab364:
            if (FEDatabase->GetOnlineSettings()->ePrevPage == ePrevPage_LobbyRoom) {
                cFEng::Get()->QueuePackageSwitch(g_LOBBY_ROOM_SCREEN, 0, 0, false);
            } else {
                cFEng::Get()->QueuePackageSwitch(g_ONLINE_MAIN_MENU, 0, 0, false);
            }
            FEDatabase->GetOnlineSettings()->ePrevPage = ePrevPage_None;
            break;
        case 0x406415e3:
            FEDatabase->GetOnlineSettings()->ePrevPage = ePrevPage_None;
            FEDatabase->GetOnlineSettings()->SelectedConnectingScreen = eOnlineConnecting_LobbyMatchSearch;
            cFEng::Get()->QueuePackageSwitch("UI_OLISPConnect.fng", 0, 0, false);
            break;
    }
}

void UIOnlineFilters::Setup() {
    OnlineSessionFilter.gameMode = GRace::kRaceType_NumTypes;
    OnlineSessionFilter.collisionDetection = ANY;
    OnlineSessionFilter.performanceMatching = ANY;
    if (FEDatabase->IsLANMode()) {
        OnlineSessionFilter.rankedGames = NO;
    } else {
        OnlineSessionFilter.rankedGames = ANY;
    }
}
