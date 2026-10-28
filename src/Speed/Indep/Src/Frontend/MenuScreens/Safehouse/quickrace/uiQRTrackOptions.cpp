#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRTrackOptions.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feWidget.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRMainMenu.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"

#include <types.h>
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32))
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#endif

class NumOpponents : public FEToggleWidget {
  public:
    NumOpponents(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        int numPlayers = FEDatabase->iNumPlayers;
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        int val = settings->NumOpponents;
        if (data == 0x9120409e) {
            val = val - 1;
            if (val < 1) {
                val = 4 - numPlayers;
            }
        } else if (data == 0xb5971bf1) {
            val = val + 1;
            if (val > 4 - numPlayers) {
                val = 1;
            }
        }
        settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        settings->NumOpponents = static_cast<uint8>(val);
        if (FEDatabase->RaceMode == static_cast<GRace::Type>(3)) {
            settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
            settings->NumLaps = static_cast<uint8>(val);
            cFEng::Get()->QueueGameMessage(0x92b703b5, parent_pkg, 0xff);
        }
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    };
    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0x3384a679);
        FEString *data = GetDataObject();
        const char *fmt = "%d";
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        FEPrintf(data, fmt, settings->NumOpponents);
    };
};

class AISkill : public FEToggleWidget {
  public:
    AISkill(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        int val = settings->AISkill;
        if (data == 0x9120409e) {
            val = val - 1;
            if (val < 0) {
                val = 2;
            }
        } else if (data == 0xb5971bf1) {
            val = val + 1;
            if (val > 2) {
                val = 0;
            }
        }
        settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        settings->AISkill = static_cast<uint8>(val);
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    }
    void Draw() override {
        unsigned int hash = 0;
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        unsigned char skill = settings->AISkill;
        switch (skill) {
            case 0:
                hash = 0x61973e01;
                break;
            case 1:
                hash = 0x3747f6d0;
                break;
            case 2:
                hash = 0x6198e2ee;
                break;
        }
        FEngSetLanguageHash(GetTitleObject(), 0x4d156786);
        FEngSetLanguageHash(GetDataObject(), hash);
    };
};

#ifdef EA_BUILD_A124
// Solo la A124: la densidad de policia de la carrera rapida (CopsOn sale de ella).
class CopDensity : public FEToggleWidget {
  public:
    CopDensity(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        int level = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->CopDensity;
        if (data == 0x9120409e) {
            level = level - 1;
            if (level < 0) {
                level = 3;
            }
        } else if (data == 0xb5971bf1) {
            level = level + 1;
            if (level > 3) {
                level = 0;
            }
        }
        FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->CopDensity = static_cast<uint8>(level);
        FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->CopsOn = level > 0;
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
        cFEng::Get()->QueueGameMessage(0x92b703b5, parent_pkg, 0xff);
    };
    void Draw() override {
        int hash = 0;
        switch (FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->CopDensity) {
            case 0:
                hash = LANGUAGE_COMMON_NONE;
                break;
            case 1:
                hash = LANGUAGE_COMMON_MINIMUM;
                break;
            case 2:
                hash = 0xa2cca838;
                break;
            case 3:
                hash = LANGUAGE_COMMON_MAXIMUM;
                break;
        }
        FEngSetLanguageHash(GetDataObject(), hash);
        FEngSetLanguageHash(GetTitleObject(), 0x251a34f8);
    };
};
#endif

class CatchUp : public FEToggleWidget {
  public:
    CatchUp(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x9120409e || data == 0xb5971bf1) {
            FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->CatchUp =
                !FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->CatchUp;
        }
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    };
    void Draw() override {
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        if (settings->CatchUp) {
            FEngSetLanguageHash(GetDataObject(), LANGUAGE_COMMON_ON);
        } else {
            FEngSetLanguageHash(GetDataObject(), LANGUAGE_COMMON_OFF);
        }
        FEngSetLanguageHash(GetTitleObject(), 0x8b8e913a);
    };
};

class TrafficLevel : public FEToggleWidget {
  public:
    TrafficLevel(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        int val = settings->TrafficDensity;
        if (data == 0x9120409e) {
            val = val - 1;
            if (val < 0) {
                val = 3;
            }
        } else if (data == 0xb5971bf1) {
            val = val + 1;
            if (val > 3) {
                val = 0;
            }
        }
        settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        settings->TrafficDensity = static_cast<uint8>(val);
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    };
    void Draw() override {
        unsigned int hash = 0;
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        unsigned char level = settings->TrafficDensity;
        switch (level) {
            case 0:
                hash = LANGUAGE_COMMON_NONE;
                break;
            case 1:
                hash = LANGUAGE_COMMON_MINIMUM;
                break;
            case 2:
                hash = 0xa2cca838;
                break;
            case 3:
                hash = LANGUAGE_COMMON_MAXIMUM;
                break;
        }
        FEngSetLanguageHash(GetDataObject(), hash);
        FEngSetLanguageHash(GetTitleObject(), 0xeb9dfc09);
    };
};

class NumLaps : public FEToggleWidget {
  public:
    NumLaps(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32)) // la A124 tambien edita las vueltas de la partida online
        int laps;
        if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
            laps = FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps;
        } else {
            laps = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->NumLaps;
        }
#else
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        int laps = settings->NumLaps;
#endif
        int max = 8;
        if (data == 0x9120409e) {
            laps = laps - 1;
            if (laps < 1) {
                laps = max;
            }
        } else if (data == 0xb5971bf1) {
            laps = laps + 1;
            if (laps > max) {
                laps = 1;
            }
        }
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32))
        if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
            FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = static_cast<uint8>(laps);
        } else {
            FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->NumLaps = static_cast<uint8>(laps);
        }
#else
        settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        settings->NumLaps = static_cast<uint8>(laps);
#endif
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    };
    void Draw() override {
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32))
        if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
            FEPrintf(GetDataObject(), "%d", FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps);
        } else {
            FEPrintf(GetDataObject(), "%d", FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->NumLaps);
        }
#else
        FEString *data = GetDataObject();
        const char *fmt = "%d";
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        FEPrintf(data, fmt, settings->NumLaps);
#endif
        FEngSetLanguageHash(GetTitleObject(), 0x48494e83);
    };
};

class TrackDirection : public FEToggleWidget {
  public:
    TrackDirection(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x9120409e || data == 0xb5971bf1) {
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32))
            if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
                FEDatabase->GetOnlineSettings()->GetRaceSettings()->TrackDirection =
                    (FEDatabase->GetOnlineSettings()->GetRaceSettings()->TrackDirection == 0);
            } else
#endif
            {
                FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->TrackDirection =
                    (FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->TrackDirection == 0);
            }
        }
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    };
    void Draw() override {
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32))
        unsigned char TrackDirection;
        if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
            TrackDirection = FEDatabase->GetOnlineSettings()->GetRaceSettings()->TrackDirection;
        } else {
            TrackDirection = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->TrackDirection;
        }
        if (TrackDirection == 0) {
#else
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        if (settings->TrackDirection == 0) {
#endif
            FEngSetLanguageHash(GetDataObject(), FEHASH_FORWARD);
        } else {
            FEngSetLanguageHash(GetDataObject(), 0xa1cd823e);
        }
        FEngSetLanguageHash(GetTitleObject(), 0xa88ffeb4);
    };
};

#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32)) // las opciones de la partida online del prototipo
class MO_OL_PrivateRoom : public FEToggleWidget {
  public:
    MO_OL_PrivateRoom(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x9120409e || data == 0xb5971bf1) {
            FEDatabase->GetOnlineSettings()->IsPrivateRoom = !FEDatabase->GetOnlineSettings()->IsPrivateRoom;
#ifndef EA_PLATFORM_WIN32
            if (FEDatabase->GetOnlineSettings()->IsPrivateRoom) {
                FEDatabase->GetOnlineSettings()->Passworded = false;
            }
#endif
        }
#ifdef EA_PLATFORM_WIN32 // PC (0x7AB97D): la sala privada quita la contrasena aunque el dato no sea una flecha
        if (FEDatabase->GetOnlineSettings()->IsPrivateRoom) {
            FEDatabase->GetOnlineSettings()->Passworded = false;
        }
#endif
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    }
    void Draw() override {
        FEngSetLanguageHash(GetDataObject(), FEDatabase->GetOnlineSettings()->IsPrivateRoom ? LANGUAGE_COMMON_YES : LANGUAGE_COMMON_NO);
        FEngSetLanguageHash(GetTitleObject(), 0x95684a90);
    }
};

class OL_GameType : public FEToggleWidget {
  public:
    OL_GameType(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x9120409e || data == 0xb5971bf1) {
            FEDatabase->GetOnlineSettings()->RankedGame = !FEDatabase->GetOnlineSettings()->RankedGame;
        }
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    }
    void Draw() override {
        if (FEDatabase->GetOnlineSettings()->RaceMode == -1) {
            FEngSetScript(GetTitleObject(), 0x7ab5521a, true);
            FEngSetScript(GetDataObject(), 0x7ab5521a, true);
            FEngSetScript("UI_QRModeOptions.fng", 0x7bcd6704, 0x16a259, true);
            FEngSetScript("UI_QRModeOptions.fng", 0x7c51b6d7, 0x16a259, true);
            FEDatabase->GetOnlineSettings()->RankedGame = false;
            Disable();
        }
        FEngSetLanguageHash(GetTitleObject(), 0xfd85bcb4);
        if (FEDatabase->GetOnlineSettings()->RankedGame) {
            FEngSetLanguageHash(GetDataObject(), 0x66cf56ae);
        } else {
            FEngSetLanguageHash(GetDataObject(), 0x0e1f1271);
        }
    }
};

class OL_NumPlayers : public FEToggleWidget {
  public:
    OL_NumPlayers(bool enabled) : FEToggleWidget(enabled) {
        nMax = cOnlineSettings::MaxOnlinePlayers;
        nMin = FEDatabase->GetOnlineSettings()->MinOnlinePlayers;
    }
    void Act(const char *parent_pkg, uint32 data) override {
        cOnlineSettings *settings = FEDatabase->GetOnlineSettings();
        int num_players = settings->MinOnlinePlayers;
        int inc = 0;
        if (data == 0x9120409e) {
            inc = -1;
        }
        if (data == 0xb5971bf1) {
            inc = 1;
        }
        num_players += inc;
        if (num_players > nMax) {
            num_players = nMin;
        }
        if (num_players < nMin) {
            num_players = nMax;
        }
        settings->MinOnlinePlayers = num_players;
        cFEng::Get()->QueueGameMessage(0x92b703b5, parent_pkg, 0xff);
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    }
    void Draw() override {
        FEngSetLanguageHash(GetTitleObject(), 0xb755f08b);
        FEPrintf(GetDataObject(), "%d", FEDatabase->GetOnlineSettings()->MinOnlinePlayers);
    }

  private:
    int nMax;
    int nMin;
};

class OL_CollisionDetection : public FEToggleWidget {
  public:
    OL_CollisionDetection(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x9120409e || data == 0xb5971bf1) {
            FEDatabase->GetOnlineSettings()->CollisionDetection = !FEDatabase->GetOnlineSettings()->CollisionDetection;
        }
        cFEng::Get()->QueueGameMessage(0x92b703b5, parent_pkg, 0xff);
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    }
    void Draw() override {
        FEngSetLanguageHash(GetDataObject(), FEDatabase->GetOnlineSettings()->CollisionDetection ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF);
        FEngSetLanguageHash(GetTitleObject(), 0x7712e575);
    }
};

class OL_PerformanceMatching : public FEToggleWidget {
  public:
    OL_PerformanceMatching(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x9120409e || data == 0xb5971bf1) {
            FEDatabase->GetOnlineSettings()->PerformanceMatching = !FEDatabase->GetOnlineSettings()->PerformanceMatching;
        }
        cFEng::Get()->QueueGameMessage(0x92b703b5, parent_pkg, 0xff);
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    }
    void Draw() override {
        FEngSetLanguageHash(GetDataObject(), FEDatabase->GetOnlineSettings()->PerformanceMatching ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF);
        FEngSetLanguageHash(GetTitleObject(), 0x711179f7);
    }
};

class OL_UseNOS : public FEToggleWidget {
  public:
    OL_UseNOS(bool enabled) : FEToggleWidget(enabled) {}
    void Act(const char *parent_pkg, uint32 data) override {
        if (data == 0x9120409e || data == 0xb5971bf1) {
            FEDatabase->GetOnlineSettings()->UseNOS = !FEDatabase->GetOnlineSettings()->UseNOS;
        }
        cFEng::Get()->QueueGameMessage(0x92b703b5, parent_pkg, 0xff);
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    }
    void Draw() override {
        FEngSetLanguageHash(GetDataObject(), FEDatabase->GetOnlineSettings()->UseNOS ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF);
        FEngSetLanguageHash(GetTitleObject(), 0xc62e);
    }
};

class OL_DisconnectPercentage : public FEToggleWidget {
  public:
    OL_DisconnectPercentage(bool enabled) : FEToggleWidget(enabled) {
        m_eMinDiscThreshold = FEDatabase->GetOnlineSettings()->CurrPlayerDisconnectPerc;
        FEDatabase->GetOnlineSettings()->DisconnectPerc = OLS_DISCONNECT_PERC_ANY;
    }
    void Act(const char *parent_pkg, uint32 data) override {
        int perc = FEDatabase->GetOnlineSettings()->DisconnectPerc;
        if (data == 0x9120409e) {
            perc--;
            if (perc < m_eMinDiscThreshold) {
                perc = OLS_DISCONNECT_PERC_ANY;
            }
        } else if (data == 0xb5971bf1) {
            perc++;
            if (perc > OLS_DISCONNECT_PERC_ANY) {
                perc = m_eMinDiscThreshold;
            }
        }
        FEDatabase->GetOnlineSettings()->DisconnectPerc = static_cast<eOnlineDisconnectPerc>(perc);
        cFEng::Get()->QueueGameMessage(0x92b703b5, parent_pkg, 0xff);
        bMovedLastUpdate = true;
        BlinkArrows(data);
        Draw();
    }
    void Draw() override {
        unsigned int hash = 0;
        switch (FEDatabase->GetOnlineSettings()->DisconnectPerc) {
            case OLS_DISCONNECT_PERC_ANY:
                hash = 0x7105fb23;
                break;
            case OLS_DISCONNECT_PERC_5:
                hash = 0xd1a127c5;
                break;
            case OLS_DISCONNECT_PERC_10:
                hash = 0x05c62011;
                break;
            case OLS_DISCONNECT_PERC_15:
                hash = 0x05c62016;
                break;
            case OLS_DISCONNECT_PERC_20:
                hash = 0x05c62032;
                break;
            case OLS_DISCONNECT_PERC_25:
                hash = 0x05c62037;
                break;
            case OLS_DISCONNECT_PERC_50:
                hash = 0x05c62095;
                break;
        }
        FEngSetLanguageHash(GetDataObject(), hash);
        FEngSetLanguageHash(GetTitleObject(), 0xa1e78997);
    }

    eOnlineDisconnectPerc m_eMinDiscThreshold;
};
#endif

UIQRTrackOptions::UIQRTrackOptions(ScreenConstructorData *sd)
    : UIWidgetMenu(sd),
#ifdef EA_PLATFORM_WIN32
      bOnlineTogglesPending(false),
#endif
      m_code(0), msgHandle(0), m_boDisconnectPercAvail(false) {
    race = GRaceDatabase::Get().GetRaceFromHash(FEDatabase->GetQuickRaceSettings(FEDatabase->RaceMode)->EventHash);
    iMaxWidgetsOnScreen = 9;
    Setup();
}

void UIQRTrackOptions::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    UIWidgetMenu::NotificationMessage(msg, pobj, param1, param2);
#ifdef EA_BUILD_A124 // el prototipo monta tambien la partida online
    switch (msg) {
        case 0x406415e3:
            if (!FEDatabase->IsOnlineMode() && !FEDatabase->IsLANMode()) {
                GRaceCustom *custom = GRaceDatabase::Get().AllocCustomRace(race);
                FEDatabase->FillCustomRace(custom, FEDatabase->GetQuickRaceSettings(race->GetRaceType()));
                if (FEDatabase->IsSplitScreenMode()) {
                    custom->SetNumOpponents(1);
                    custom->SetCopsEnabled(false);
                    custom->SetCopDensity(static_cast<GRace::CopDensity>(0));
                }
                GRaceDatabase::Get().SetStartupRace(custom, GRace::kRaceContext_QuickRace);
                GRaceDatabase::Get().FreeCustomRace(custom);
            }
            if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
                if (GetCurrentInputWidget() == GetWidget(EnterPasswordId) || GetCurrentInputWidget() == GetWidget(ConfirmPasswordId)) {
                    return;
                }
                if ((bStrLen(FEDatabase->GetOnlineSettings()->Password) < 3 || bStrLen(FEDatabase->GetOnlineSettings()->Password) > 12) &&
                    FEDatabase->GetOnlineSettings()->Passworded == true) {
                    DialogInterface::ShowOk(GetPackageName(), "", dialog_alert, 0xd29a1a18, 3, 12);
                    return;
                }
                if (bStrCmp(FEDatabase->GetOnlineSettings()->VerifyPassword, FEDatabase->GetOnlineSettings()->Password) != 0 &&
                    FEDatabase->GetOnlineSettings()->Passworded == true) {
                    DialogInterface::ShowOk(GetPackageName(), "", dialog_alert, 0xff99d845);
                    return;
                }
                int port = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(0));
                cFEng::Get()->QueuePackageSwitch("OL_GameRoom.fng", 0, port, true);
                return;
            }
            if (FEDatabase->IsSplitScreenMode()) {
                cFEng::Get()->QueuePackageSwitch("PressStart.fng", 0, 0, false);
                return;
            }
            cFEng::Get()->QueuePackageSwitch("Car_Select.fng", 0, 0, false);
            return;
        case 0x911ab364:
            cFEng::Get()->QueuePackageSwitch("Track_Select.fng", 0, 0, false);
            GRaceDatabase::Get().ClearStartupRace();
            return;
        case 0xc519bfc4:
            DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO, 0xd05fc3a3, 0x34dc1bcf, 0x34dc1bcf,
                                            first_dialog_button2, GetLocalizedString(0x8aef5ae8));
            break;
        case 0xd05fc3a3:
            FEDatabase->DefaultRaceSettings();
            if (FEDatabase->IsOnlineMode()) {
                LoadOnlineDefaults();
                SetOnlineTogglesProperties();
                Reset();
            }
            for (int i = 0; i < Options.CountElements(); i++) {
                FEWidget *w = static_cast<FEWidget *>(Options.GetNode(i));
                w->Draw();
            }
            break;
        case 0xda5b8712: {
            FEInputWidget *input = GetCurrentInputWidget();
            FEWidget *enter = GetWidget(EnterPasswordId);
            FEWidget *confirm = GetWidget(ConfirmPasswordId);
            if (input == enter) {
                FEDatabase->GetOnlineSettings()->SetGamePassword(input->GetInputText());
                return;
            }
            if (input == confirm) {
                FEDatabase->GetOnlineSettings()->SetGameVerifyPassword(input->GetInputText());
                return;
            }
            break;
        }
        case 0x1fab5998:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
            }
            m_boDisconnectPercAvail = true;
            Setup();
            break;
        case 0x9120409e:
        case 0xb5971bf1:
            if (FEDatabase->IsOnlineMode() && !FEDatabase->IsLANMode()) {
                if (pCurrentOption && pCurrentOption == GetWidget(1)) {
                    SetOnlineTogglesProperties();
                    return;
                }
            }
            break;
        case 0x4ebed0a1:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }
            if (m_code != 0) {
                msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x1fab5998, 0x1fab5998,
                                                           GetLocalizedString(0xf77b0d99));
            } else {
                Setup();
            }
            break;
        case 0x34dc1bcf:
            break;
    }
#elif ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32)
    switch (msg) {
        case 0x406415e3:
            if (!FEDatabase->IsOnlineMode() && !FEDatabase->IsLANMode()) {
                GRaceCustom *custom = GRaceDatabase::Get().AllocCustomRace(race);
                custom->SetCopsEnabled(false);
                FEDatabase->FillCustomRace(custom, FEDatabase->GetQuickRaceSettings(race->GetRaceType()));
                if (FEDatabase->IsSplitScreenMode()) {
                    custom->SetNumOpponents(1);
                }
                GRaceDatabase::Get().SetStartupRace(custom, GRace::kRaceContext_QuickRace);
                GRaceDatabase::Get().FreeCustomRace(custom);
            }
            if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
                if (GetCurrentInputWidget() == GetWidget(EnterPasswordId) || GetCurrentInputWidget() == GetWidget(ConfirmPasswordId)) {
                    return;
                }
                if ((bStrLen(FEDatabase->GetOnlineSettings()->Password) < 3 || bStrLen(FEDatabase->GetOnlineSettings()->Password) > 12) &&
                    FEDatabase->GetOnlineSettings()->Passworded == true) {
                    DialogInterface::ShowOk(GetPackageName(), "", dialog_alert, 0xd29a1a18, 3, 12);
                    return;
                }
                if (bStrCmp(FEDatabase->GetOnlineSettings()->VerifyPassword, FEDatabase->GetOnlineSettings()->Password) != 0 &&
                    FEDatabase->GetOnlineSettings()->Passworded == true) {
                    DialogInterface::ShowOk(GetPackageName(), "", dialog_alert, 0xff99d845);
                    return;
                }
                cFEng::Get()->QueuePackageSwitch("OL_GameRoom.fng", 0, 0, false);
                return;
            }
            if (FEDatabase->IsSplitScreenMode()) {
                cFEng::Get()->QueuePackageSwitch("PressStart.fng", 0, 0, false);
                return;
            }
            cFEng::Get()->QueuePackageSwitch("Car_Select.fng", 0, 0, false);
            return;
        case 0x911ab364:
            cFEng::Get()->QueuePackageSwitch("Track_Select.fng", 0, 0, false);
            return;
        case 0xc519bfc4:
            DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO, 0xd05fc3a3, 0x34dc1bcf, 0x34dc1bcf,
                                            first_dialog_button2, GetLocalizedString(0x8aef5ae8));
            break;
        case 0xd05fc3a3:
            FEDatabase->DefaultRaceSettings();
            if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
                LoadOnlineDefaults();
                Reset();
                bOnlineTogglesPending = true;
            }
            for (int i = 0; i < Options.CountElements(); i++) {
                FEWidget *w = static_cast<FEWidget *>(Options.GetNode(i));
                w->Draw();
            }
            break;
        case 0xc98356ba:
            if (bOnlineTogglesPending) {
                bOnlineTogglesPending = false;
                SetOnlineTogglesProperties();
                return;
            }
            break;
        case 0xda5b8712: {
            FEInputWidget *input = GetCurrentInputWidget();
            FEWidget *enter = GetWidget(EnterPasswordId);
            FEWidget *confirm = GetWidget(ConfirmPasswordId);
            if (input == enter) {
                FEDatabase->GetOnlineSettings()->SetGamePassword(input->GetInputText());
                return;
            }
            if (input == confirm) {
                FEDatabase->GetOnlineSettings()->SetGameVerifyPassword(input->GetInputText());
                return;
            }
            break;
        }
        case 0x1fab5998:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
            }
            m_boDisconnectPercAvail = true;
            Setup();
            break;
        case 0x9120409e:
        case 0xb5971bf1:
            if (FEDatabase->IsOnlineMode() && !FEDatabase->IsLANMode()) {
                if (pCurrentOption && pCurrentOption == GetWidget(1)) {
                    SetOnlineTogglesProperties();
                    return;
                }
            }
            break;
        case 0x4ebed0a1:
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
                msgHandle = 0;
            }
            if (m_code != 0) {
                msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x1fab5998, 0x1fab5998,
                                                           GetLocalizedString(0xf77b0d99));
            } else {
                Setup();
            }
            break;
    }
#else
    switch (msg) {
        case 0x406415e3:
            if (!(FEDatabase->IsOnlineMode()) && !(FEDatabase->IsLANMode())) {
                GRaceCustom *custom = GRaceDatabase::Get().AllocCustomRace(race);
                custom->SetCopsEnabled(false);
                FEDatabase->FillCustomRace(custom, FEDatabase->GetQuickRaceSettings(race->GetRaceType()));
                bool isSplitScreen = FEDatabase->IsSplitScreenMode();
                if (isSplitScreen) {
                    custom->SetNumOpponents(1);
                }
                GRaceDatabase::Get().SetStartupRace(custom, GRace::kRaceContext_QuickRace);
                GRaceDatabase::Get().FreeCustomRace(custom);
            }
            {
                bool isSplitScreen = FEDatabase->IsSplitScreenMode();
                if (isSplitScreen) {
                    cFEng::Get()->QueuePackageSwitch("PressStart.fng", 0, 0, false);
                } else {
                    cFEng::Get()->QueuePackageSwitch("Car_Select.fng", 0, 0, false);
                }
            }
            break;
        case 0x911ab364:
            cFEng::Get()->QueuePackageSwitch("Track_Select.fng", 0, 0, false);
            break;
        case 0xc519bfc4:
            DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO, 0xd05fc3a3, 0x34dc1bcf, 0x34dc1bcf,
                                            first_dialog_button2, GetLocalizedString(0x8aef5ae8));
            break;
        case 0xd05fc3a3:
            FEDatabase->DefaultRaceSettings();
            for (int i = 0; i < Options.CountElements(); i++) {
                FEWidget *w = static_cast<FEWidget *>(Options.GetNode(i));
                w->Draw();
            }
            break;
        case 0x34dc1bcf:
            break;
        case 0xe1fde1d1:
            break;
    }
#endif
}

void UIQRTrackOptions::Setup() {
#if ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32)
    if (!FEDatabase->IsOnlineMode() && !FEDatabase->IsLANMode()) {
        unsigned int titleHash = 0;
        GRace::Type type = race->GetRaceType();
        switch (type) {
            case GRace::kRaceType_Circuit:
                titleHash = 0xa6ed015d;
                break;
            case GRace::kRaceType_Drag:
                titleHash = 0xec86e188;
                break;
            case GRace::kRaceType_Knockout:
                titleHash = 0xcc959b8;
                break;
            case GRace::kRaceType_P2P:
                titleHash = 0xb80bfc8a;
                break;
            case GRace::kRaceType_Tollbooth:
                titleHash = 0x141edfe1;
                break;
            case GRace::kRaceType_SpeedTrap:
                titleHash = 0xf2745852;
                break;
            default:
                titleHash = 0;
                break;
        }
        FEngSetLanguageHash(GetPackageName(), 0x42adb44c, titleHash);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0x7dadee33);
    }
#endif
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32)) // el prototipo carga los valores online y la tasa de desconexion del jugador
    int percentage;
    GRace::Type raceType;
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        raceType = FEDatabase->GetOnlineSettings()->RaceMode;
        LoadOnlineDefaults();
        percentage = 0;
        FEDatabase->GetOnlineSettings()->CurrPlayerDisconnectPerc = OLS_DISCONNECT_PERC_ANY;
        if (FEDatabase->IsLANMode()) {
            m_boDisconnectPercAvail = true;
        } else if (!m_boDisconnectPercAvail) {
            if (GetDisconnectPercentage(&percentage) == 0) {
                m_boDisconnectPercAvail = true;
                if (msgHandle) {
                    DialogInterface::DismissDialog(msgHandle);
                }
                FEDatabase->GetOnlineSettings()->CurrPlayerDisconnectPerc = GetDiconnectPercentageThreshold(percentage);
            } else {
                if (!msgHandle) {
                    msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_info, LANGUAGE_COMMON_CANCEL, 0x1fab5998, 0x1fab5998,
                                                               GetLocalizedString(0x59b40454));
                }
                return;
            }
        }
    } else {
        raceType = FEDatabase->RaceMode;
    }
#else
    GRace::Type raceType = FEDatabase->RaceMode;
#endif
    switch (raceType) {
        case GRace::kRaceType_Circuit:
            SetupCircuit();
            break;
        case GRace::kRaceType_P2P:
            SetupSprint();
            break;
        case GRace::kRaceType_Drag:
            SetupDrag();
            break;
        case GRace::kRaceType_Knockout:
            SetupKnockout();
            break;
        case GRace::kRaceType_SpeedTrap:
            SetupSpeedTrap();
            break;
        case GRace::kRaceType_Tollbooth:
            SetupTollbooth();
            break;
        default:
            SetupCircuit();
            break;
    }
#if ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32)
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        SetOnlineTogglesProperties();
    }
    SetInitialOption(0);
#else
    SetInitialOption(0);
#ifdef EA_BUILD_A124
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        SetOnlineTogglesProperties();
    }
#endif
    if ((FEDatabase->IsOnlineMode()) || (FEDatabase->IsLANMode())) {
        FEngSetLanguageHash(GetPackageName(), 0x42adb44c, 0x7dadee33);
        return;
    }
    unsigned int titleHash = 0;
    GRace::Type type = race->GetRaceType();
    switch (type) {
        case GRace::kRaceType_Circuit:
            titleHash = 0xa6ed015d;
            break;
        case GRace::kRaceType_Drag:
            titleHash = 0xec86e188;
            break;
        case GRace::kRaceType_Knockout:
            titleHash = 0xcc959b8;
            break;
        case GRace::kRaceType_P2P:
            titleHash = 0xb80bfc8a;
            break;
        case GRace::kRaceType_Tollbooth:
            titleHash = 0x141edfe1;
            break;
        case GRace::kRaceType_SpeedTrap:
            titleHash = 0xf2745852;
            break;
        default:
            titleHash = 0;
            break;
    }
    FEngSetLanguageHash(GetPackageName(), 0x42adb44c, titleHash);
#endif
}

void UIQRTrackOptions::BoilerPlateOnline(const bool &boAddLaps) {
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32)) // las opciones de la partida online; en LAN no hay partida puntuada, tasa de desconexion ni sala privada
    if (FEDatabase->IsLANMode()) {
        AddToggleOption(new ("OL_GameType", 0) OL_GameType(false), true);
    } else {
        AddToggleOption(new ("OL_GameType", 0) OL_GameType(true), true);
    }
#ifndef EA_PLATFORM_WIN32 // PC (0x7AC5F0) no pone el sentido de la pista en la partida online
    AddToggleOption(new ("TrackDirection", 0) TrackDirection(true), true);
#endif
    if (boAddLaps) {
        AddToggleOption(new ("NumLaps", 0) NumLaps(true), true);
    }
    AddToggleOption(new ("OL_UseNOS", 0) OL_UseNOS(true), true);
    AddToggleOption(new ("OL_CollisionDetection", 0) OL_CollisionDetection(true), true);
    AddToggleOption(new ("OL_PerformanceMatching", 0) OL_PerformanceMatching(true), true);
    AddToggleOption(new ("OL_NumPlayers", 0) OL_NumPlayers(true), true);
    if (!FEDatabase->IsLANMode()) {
        AddToggleOption(new ("OL_DisconnectPercentage", 0) OL_DisconnectPercentage(true), true);
        AddToggleOption(new ("MO_OL_PrivateRoom", 0) MO_OL_PrivateRoom(true), true);
    }
#endif
}

void UIQRTrackOptions::SetupCircuit() {
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32))
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        BoilerPlateOnline(true);
        FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = 3;
        return;
    }
#endif
    if (race->GetCanBeReversed()) {
        AddToggleOption(new ("TrackDirection", 0) TrackDirection(true), true);
    }
    AddToggleOption(new ("NumLaps", 0) NumLaps(true), true);
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // las dos PS2 y PC ponen siempre el trafico
    if (!FEDatabase->IsSplitScreenMode())
#endif
    {
        AddToggleOption(new ("TrafficLevel", 0) TrafficLevel(true), true);
    }
    if (!FEDatabase->IsSplitScreenMode()) {
        AddToggleOption(new ("NumOpponents", 0) NumOpponents(true), true);
#ifndef EA_BUILD_A124
        AddToggleOption(new ("AISkill", 0) AISkill(true), true);
        AddToggleOption(new ("CatchUp", 0) CatchUp(true), true);
#endif
    }
#ifdef EA_BUILD_A124 // la A124 pone la habilidad y el rubber band tambien en pantalla partida
    AddToggleOption(new ("AISkill", 0) AISkill(true), true);
    AddToggleOption(new ("CatchUp", 0) CatchUp(true), true);
    if (!FEDatabase->IsSplitScreenMode()) {
        AddToggleOption(new ("CopDensity", 0) CopDensity(true), true);
    }
#endif
}

void UIQRTrackOptions::SetupSprint() {
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32))
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        BoilerPlateOnline(false);
        FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = 1;
        return;
    }
#endif
    if (race->GetCanBeReversed()) {
        AddToggleOption(new ("TrackDirection", 0) TrackDirection(true), true);
    }
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // las dos PS2 y PC ponen siempre el trafico
    if (!FEDatabase->IsSplitScreenMode())
#endif
    {
        AddToggleOption(new ("TrafficLevel", 0) TrafficLevel(true), true);
    }
    if (!FEDatabase->IsSplitScreenMode()) {
        AddToggleOption(new ("NumOpponents", 0) NumOpponents(true), true);
#ifndef EA_BUILD_A124
        AddToggleOption(new ("AISkill", 0) AISkill(true), true);
        AddToggleOption(new ("CatchUp", 0) CatchUp(true), true);
#endif
    }
#ifdef EA_BUILD_A124 // la A124 pone la habilidad y el rubber band tambien en pantalla partida
    AddToggleOption(new ("AISkill", 0) AISkill(true), true);
    AddToggleOption(new ("CatchUp", 0) CatchUp(true), true);
    if (!FEDatabase->IsSplitScreenMode()) {
        AddToggleOption(new ("CopDensity", 0) CopDensity(true), true);
    }
#endif
}

void UIQRTrackOptions::SetupDrag() {
#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32))
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        BoilerPlateOnline(false);
        FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = 1;
        return;
    }
#endif
    if (race->GetCanBeReversed()) {
        AddToggleOption(new ("TrackDirection", 0) TrackDirection(true), true);
    }
    if (!FEDatabase->IsSplitScreenMode()) {
        AddToggleOption(new ("NumOpponents", 0) NumOpponents(true), true);
#ifndef EA_BUILD_A124
        AddToggleOption(new ("AISkill", 0) AISkill(true), true);
        AddToggleOption(new ("CatchUp", 0) CatchUp(true), true);
#endif
    }
#ifdef EA_BUILD_A124
    AddToggleOption(new ("AISkill", 0) AISkill(true), true);
    AddToggleOption(new ("CatchUp", 0) CatchUp(true), true);
#endif
}

void UIQRTrackOptions::SetupKnockout() {
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        bool boAddLaps = false;
        BoilerPlateOnline(boAddLaps);
        if (race->GetCanBeReversed()) {
            TrackDirection *td = new ("TrackDirection", 0) TrackDirection(true);
            AddToggleOption(td, true);
        }
        NumLaps *nl = new ("NumLaps", 0) NumLaps(true);
        AddToggleOption(nl, true);
    } else {
        if (race->GetCanBeReversed()) {
            TrackDirection *td = new ("TrackDirection", 0) TrackDirection(true);
            AddToggleOption(td, true);
        }
        NumOpponents *no = new ("NumOpponents", 0) NumOpponents(true);
        AddToggleOption(no, true);
        NumLaps *nl = new ("NumLaps", 0) NumLaps(true);
        unsigned int idx = AddToggleOption(nl, true);
        static_cast<FEToggleWidget *>(GetWidget(idx))->SetDisableScript(FEHashUpper("GREY"));
        static_cast<FEToggleWidget *>(GetWidget(idx))->Disable();
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32)
        bool isSplitScreen = FEDatabase->IsSplitScreenMode();
        if (!isSplitScreen)
#endif
        {
            TrafficLevel *tl = new ("TrafficLevel", 0) TrafficLevel(true);
            AddToggleOption(tl, true);
        }
        AISkill *ai = new ("AISkill", 0) AISkill(true);
        AddToggleOption(ai, true);
        CatchUp *cu = new ("CatchUp", 0) CatchUp(true);
        AddToggleOption(cu, true);
#ifdef EA_BUILD_A124
        CopDensity *cd = new ("CopDensity", 0) CopDensity(true);
        AddToggleOption(cd, true);
#endif
    }
}

void UIQRTrackOptions::SetupSpeedTrap() {
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        BoilerPlateOnline(false);
        if (race->GetCanBeReversed()) {
            AddToggleOption(new ("TrackDirection", 0) TrackDirection(true), true);
        }
    } else {
        if (race->GetCanBeReversed()) {
            AddToggleOption(new ("TrackDirection", 0) TrackDirection(true), true);
        }
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // las dos PS2 y PC ponen siempre el trafico
        if (!FEDatabase->IsSplitScreenMode())
#endif
        {
            AddToggleOption(new ("TrafficLevel", 0) TrafficLevel(true), true);
        }
        if (!FEDatabase->IsSplitScreenMode()) {
            AddToggleOption(new ("NumOpponents", 0) NumOpponents(true), true);
#ifndef EA_BUILD_A124
            AddToggleOption(new ("AISkill", 0) AISkill(true), true);
            AddToggleOption(new ("CatchUp", 0) CatchUp(true), true);
#endif
        }
#ifdef EA_BUILD_A124 // la A124 pone la habilidad y el rubber band tambien en pantalla partida
        AddToggleOption(new ("AISkill", 0) AISkill(true), true);
        AddToggleOption(new ("CatchUp", 0) CatchUp(true), true);
        if (!FEDatabase->IsSplitScreenMode()) {
            AddToggleOption(new ("CopDensity", 0) CopDensity(true), true);
        }
#endif
    }
}

void UIQRTrackOptions::SetupTollbooth() {
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        BoilerPlateOnline(false);
        if (race->GetCanBeReversed()) {
            AddToggleOption(new ("TrackDirection", 0) TrackDirection(true), true);
        }
    } else {
        if (race->GetCanBeReversed()) {
            AddToggleOption(new ("TrackDirection", 0) TrackDirection(true), true);
        }
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // las dos PS2 y PC ponen siempre el trafico
        if (!FEDatabase->IsSplitScreenMode())
#endif
        {
            AddToggleOption(new ("TrafficLevel", 0) TrafficLevel(true), true);
        }
#ifdef EA_BUILD_A124
        AddToggleOption(new ("CopDensity", 0) CopDensity(true), true);
#endif
    }
}

#if defined(EA_BUILD_A124) || (ONLINE_SUPPORT && defined(EA_PLATFORM_WIN32))
void UIQRTrackOptions::LoadOnlineDefaults() {
    if (FEDatabase->IsLANMode()) {
        FEDatabase->GetOnlineSettings()->RankedGame = false;
    } else {
        FEDatabase->GetOnlineSettings()->RankedGame = true;
    }
    FEDatabase->GetOnlineSettings()->GetRaceSettings()->TrackDirection = 0;
    if (FEDatabase->GetOnlineSettings()->RaceMode == GRace::kRaceType_Circuit) {
        FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = 3;
    } else {
        FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = 1;
    }
    FEDatabase->GetOnlineSettings()->UseNOS = true;
    FEDatabase->GetOnlineSettings()->CollisionDetection = true;
    FEDatabase->GetOnlineSettings()->PerformanceMatching = true;
    FEDatabase->GetOnlineSettings()->DisconnectPerc = OLS_DISCONNECT_PERC_ANY;
    FEDatabase->GetOnlineSettings()->IsPrivateRoom = false;
    FEDatabase->GetOnlineSettings()->MinOnlinePlayers = 2;
}

void UIQRTrackOptions::SetOnlineTogglesProperties() {
#ifdef EA_PLATFORM_WIN32
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        int last_widget = FEDatabase->GetOnlineSettings()->RaceMode == GRace::kRaceType_Circuit ? 6 : 5;

        if (FEDatabase->GetOnlineSettings()->RankedGame) {
            FEDatabase->GetOnlineSettings()->GetRaceSettings()->TrackDirection = 0;
            FEDatabase->GetOnlineSettings()->UseNOS = true;
            FEDatabase->GetOnlineSettings()->CollisionDetection = true;
            FEDatabase->GetOnlineSettings()->PerformanceMatching = true;
            if (FEDatabase->GetOnlineSettings()->RaceMode == GRace::kRaceType_Circuit) {
                FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = 3;
            }
            FEDatabase->GetOnlineSettings()->MinOnlinePlayers = 2;
            for (int i = 2; i <= last_widget; i++) {
                FEWidget *w = GetWidget(i);
                if (w) {
                    w->Draw();
                    w->Disable();
                }
            }
        } else {
            if (FEDatabase->IsOnlineMode()) {
                last_widget += 2;
            }
            for (int i = 2; i <= last_widget; i++) {
                FEWidget *w = GetWidget(i);
                if (w) {
                    w->Enable();
                }
            }
        }
    }
#else
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        if (FEDatabase->GetOnlineSettings()->RankedGame) {
            FEDatabase->GetOnlineSettings()->GetRaceSettings()->TrackDirection = 0;
            FEDatabase->GetOnlineSettings()->UseNOS = true;
            FEDatabase->GetOnlineSettings()->CollisionDetection = true;
            FEDatabase->GetOnlineSettings()->PerformanceMatching = true;
            if (FEDatabase->GetOnlineSettings()->RaceMode == GRace::kRaceType_Circuit) {
                FEDatabase->GetOnlineSettings()->GetRaceSettings()->NumLaps = 3;
            }
            FEDatabase->GetOnlineSettings()->MinOnlinePlayers = 2;
            for (int i = 2; i < 8; i++) {
                FEWidget *w = GetWidget(i);
                if (w) {
                    w->Draw();
                    w->Disable();
                }
                if (i == 6 && FEDatabase->GetOnlineSettings()->RaceMode != GRace::kRaceType_Circuit) {
                    break;
                }
            }
        } else {
            for (int i = 2; i < 8; i++) {
                FEWidget *w = GetWidget(i);
                if (w) {
                    w->Enable();
                }
                if (i == 6 && FEDatabase->GetOnlineSettings()->RaceMode != GRace::kRaceType_Circuit) {
                    break;
                }
            }
        }
    }
#endif
}

int32 UIQRTrackOptions::GetDisconnectPercentage(int *percentage) {
    PlayerDataT playerData;
    int32 result = LobbyUsers::Instance().GetMyUserStats(playerData, GetExtendedStatsCB, this);
    if (result != 0) {
        if (result < 0) {
            if (msgHandle) {
                DialogInterface::DismissDialog(msgHandle);
            }
            msgHandle = DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x1fab5998, 0x1fab5998,
                                                       GetLocalizedString(0xf77b0d99));
        }
        return result;
    }
    if (playerData.GetTotalGamesPlayed(3) < 20) {
        *percentage = 0;
    } else {
        *percentage = static_cast<int>(playerData.GetTotalDisconnectPercentage());
    }
    return 0;
}

void UIQRTrackOptions::GetExtendedStatsCB(LobbyApiMsgT *msg, void *context) {
    UIQRTrackOptions *options = static_cast<UIQRTrackOptions *>(context);
    if (!options->m_boDisconnectPercAvail) {
        options->m_code = msg->code;
        cFEng::Get()->QueueGameMessage(0x4ebed0a1, options->GetPackageName(), 0xff);
    }
}
#endif
