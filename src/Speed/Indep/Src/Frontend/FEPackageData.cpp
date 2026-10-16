#include "Speed/Indep/Src/Frontend/FEPackageData.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Misc/TestHooks/JuiceHooks/JuiceHooks.h"
#endif
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEIconScrollerMenu.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/uiMain.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsMain.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsTrailers.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsController.hpp"
#ifdef EA_PLATFORM_WIN32
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsPCController.hpp"
#endif
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiCredits.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiEATraxJukebox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiCareerMain.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiCareerManager.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetRaceEvents.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetMain.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetRival.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetRivalBio.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetMilestones.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiSafehouseRegionUnlock.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetBounty.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiMarkerSelect.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetLogin.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetMain.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetRS.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetUS.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetVD.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetCTS.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetTEP.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetPD.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetRankings.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetRankingsDetail.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRModeSelect.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRMainMenu.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRBrief.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRTrackSelect.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRTrackOptions.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRCarSelect.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRPressStart.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRChallengeSeries.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiShowcase.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/customize/MyCarsManager.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/customize/CarCustomize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/uiPause.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/uiSMS.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/uiSMSMessage.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/uiSixDaysLater.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/uiWorldMap.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FeFadeScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FeBustedOverlay.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_PostRace.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/InGameMovieScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/InGameTutorialScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/CustomTuning.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/PhotoFinish.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Career/FEPkg_EngageEventDialog.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Career/FEGameWonScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/ControllerUnplugged.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEAnyMovieScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEAnyTutorialScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FEMovieScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FESplashScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingTips.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELanguageSelect.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingControllerScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiInfractions.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/Database/VehicleDB.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/customize/DebugCarCustomize.hpp"
#include "Speed/Indep/Src/Frontend/FEngRender.hpp"
#include "Speed/Indep/Src/Frontend/FEObjectCallbacks.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/FEng/FEList.h"
#include "Speed/Indep/Src/FEng/FEPackageChunks.h"
#include "Speed/Indep/Src/Misc/SpeedChunks.hpp"
#include "Speed/Indep/Src/Misc/LZCompress.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/Src/Frontend/Database/uiProfileManager.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineLogin.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOnlineMainMenu.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/StatsMainMenu.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineQuickJoin.hpp"
#include "Speed/Indep/Src/Online/WebOfferUG2.hpp"
#endif

// Local declaration of the world map screen class (implemented in
// SourceLists/zFe, uiWorldMap.cpp). The gameplay-side WorldMap stub in
// Gameplay/GManager.cpp prevents this from living in uiWorldMap.hpp.
class UITrackMapStreamer;
struct MapItem;

class WorldMap : public UIWidgetMenu {
  public:
    static GIcon *GetGPSingIcon();

    WorldMap(ScreenConstructorData *sd);
    ~WorldMap() override;

    FEObject *Cursor;                // offset 0x138, size 0x4
#ifndef EA_PLATFORM_WIN32 // PC no tiene la cola del mando (como uiWorldMap.cpp)
    ActionQueue *mActionQ;           // offset 0x13C, size 0x4
#endif
    bVector2 CurrentVelocity;        // offset 0x140, size 0x8
    Timer TimeSinceLastMove;         // offset 0x148, size 0x4
    bVector2 CursorMoveFrom;         // offset 0x14C, size 0x8
    TrackInfo *pCurrentTrack;        // offset 0x154, size 0x4
    FEMultiImage *TrackMap;          // offset 0x158, size 0x4
    bVector2 MapTopLeft;             // offset 0x15C, size 0x8
    bVector2 MapSize;                // offset 0x164, size 0x8
    bTList<MapItem> TheMapItems;     // offset 0x16C, size 0x8
    MapItem *SelectedItem;           // offset 0x174, size 0x4
    UITrackMapStreamer *MapStreamer; // offset 0x178, size 0x4
    uint32 CurrentView;              // offset 0x17C, size 0x4
    int CurrentZoom;                 // offset 0x180, size 0x4
    int CurrentRaceType;             // offset 0x184, size 0x4
    bool bInToggleMode;              // offset 0x188, size 0x1
    bool bCursorOn;                  // offset 0x18C, size 0x1
    bool bCursorMoving;              // offset 0x190, size 0x1
#ifndef EA_BUILD_A124
    // La A124 no lo tiene: sus stabs dan WorldMap de 0x198 con fSnapDist en 0x194 (new de 0x198 en CreateWorldMap).
    bool bLeftHeldOnMap;             // offset 0x194, size 0x1
#endif
    float fSnapDist;                 // offset 0x198, size 0x4

    static GIcon *mGPSingIcon; // size: 0x4, address: 0x8041B640
};

MenuScreen *CreateChyronScreen(ScreenConstructorData *sd);
MenuScreen *CreateFEKeyboard(ScreenConstructorData *sd);

const char *gLoadinScreenPackageName = "";

void SetLoadingScreenPackageName(const char *name) {
    gLoadinScreenPackageName = name;
}

const char *GetLoadingScreenPackageName() {
    return gLoadinScreenPackageName;
}

static MenuScreen *CreateMainMenu(ScreenConstructorData *sd) {
    return new ("UIMain", 0) UIMain(sd);
}

static MenuScreen *CreateSubMenu(ScreenConstructorData *sd) {
    if (FEDatabase->IsOptionsMode()) {
        if (FEDatabase->GetOptionsSettings()->CurrentCategory == OC_TRAILERS) {
            return new ("UIOptionsTrailers", 0) UIOptionsTrailers(sd);
        }
        return new ("UIOptionsMain", 0) UIOptionsMain(sd);
    }
    if (FEDatabase->IsCareerMode()) {
        return new ("uiCareerCrib", 0) uiCareerCrib(sd);
    }
    if (FEDatabase->IsCareerManagerMode()) {
        return new ("uiCareerManager", 0) uiCareerManager(sd);
    }
    if (FEDatabase->IsModeSelectMode()) {
        return new ("UIQRModeSelect", 0) UIQRModeSelect(sd);
    }
    if (FEDatabase->IsQuickRaceMode()) {
        return new ("UIQRMainMenu", 0) UIQRMainMenu(sd);
    }
    return new ("UIOptionsMain", 0) UIOptionsMain(sd);
}

static MenuScreen *CreateCommonPauseMenu(ScreenConstructorData *sd) {
    if (FEDatabase->IsOptionsMode()) {
        return new ("UIOptionsMain", 0) UIOptionsMain(sd);
    }
    return new ("PauseMenu", 0) PauseMenu(sd);
}

#ifdef EA_BUILD_A124
static MenuScreen *CreateOLConnectingScreen(ScreenConstructorData *sd) {
    switch (FEDatabase->GetOnlineSettings()->SelectedConnectingScreen) {
    case eOnlineConnecting_ISP:
        return new ("UIOnlineLogin", 0) UIOnlineLogin(sd);
    case eOnlineConnecting_QuickMatchSearch:
        FEDatabase->GetOnlineSettings()->SelectedConnectingScreen = eOnlineConnecting_ISP;
        return CreateOnlineGameSessionSearch(sd, eOnlineConnecting_QuickMatchSearch);
    case eOnlineConnecting_LobbyMatchSearch:
        FEDatabase->GetOnlineSettings()->SelectedConnectingScreen = eOnlineConnecting_ISP;
        return CreateOnlineGameSessionSearch(sd, eOnlineConnecting_LobbyMatchSearch);
    }
    return 0;
}

static MenuScreen *CreateNewsAndTerms(ScreenConstructorData *sd) {
    if (FEDatabase->GetOnlineSettings()->ShowForeignReg) {
        FEDatabase->GetOnlineSettings()->ShowForeignReg = false;
        return CreateForeignReg(sd);
    }
    return CreateOnlineNews(sd);
}
#endif

static MenuScreen *CreateOptionsScreen(ScreenConstructorData *sd) {
    return new ("UIOptionsScreen", 0) UIOptionsScreen(sd);
}

static MenuScreen *CreateQRBrief(ScreenConstructorData *sd) {
    return new ("UIQRBrief", 0) UIQRBrief(sd);
}

static MenuScreen *CreateQRTrackSelect(ScreenConstructorData *sd) {
    return new ("UIQRTrackSelect", 0) UIQRTrackSelect(sd);
}

static MenuScreen *CreateQRTrackOptions(ScreenConstructorData *sd) {
    return new ("UIQRTrackOptions", 0) UIQRTrackOptions(sd);
}

static MenuScreen *CreateQRCarSelect(ScreenConstructorData *sd) {
    return new ("UIQRCarSelect", 0) UIQRCarSelect(sd);
}

static MenuScreen *CreateQRPressStart(ScreenConstructorData *sd) {
    return new ("uiQRPressStart", 0) uiQRPressStart(sd);
}

static MenuScreen *CreateQRChallengeSeries(ScreenConstructorData *sd) {
    return new ("UIQRChallengeSeries", 0) UIQRChallengeSeries(sd);
}

static MenuScreen *CreateShowcase(ScreenConstructorData *sd) {
    return new ("Showcase", 0) Showcase(sd);
}

static MenuScreen *CreateFadeScreen(ScreenConstructorData *sd) {
    return new ("FadeScreen", 0) FadeScreen(sd);
}

static MenuScreen *CreateWorldMap(ScreenConstructorData *sd) {
    return new ("WorldMap", 0) WorldMap(sd);
}

static MenuScreen *CreateSMS(ScreenConstructorData *sd) {
    return new ("uiSMS", 0) uiSMS(sd);
}

static MenuScreen *CreateSMSMessage(ScreenConstructorData *sd) {
    return new ("uiSMSMessage", 0) uiSMSMessage(sd);
}

static MenuScreen *CreateControllerUnplugged(ScreenConstructorData *sd) {
    return new ("ControllerUnplugged", 0) ControllerUnplugged(sd);
}

static MenuScreen *CreateMovieScreen(ScreenConstructorData *sd) {
    return new ("MovieScreen", 0) MovieScreen(sd);
}

static MenuScreen *CreateSplashScreen(ScreenConstructorData *sd) {
    return new ("SplashScreen", 0) SplashScreen(sd);
}

static MenuScreen *CreateLoadingTipsScreen(ScreenConstructorData *sd) {
    return new LoadingTips(sd);
}

static MenuScreen *CreateLanguageSelectScreen(ScreenConstructorData *sd) {
    return new ("LanguageSelectScreen", 0) LanguageSelectScreen(sd);
}

static MenuScreen *CreateSixDaysLaterScreen(ScreenConstructorData *sd) {
    return new ("SixDaysLater", 0) SixDaysLater(sd);
}

static MenuScreen *CreateEngageEventDialog(ScreenConstructorData *sd) {
    return new ("EngageEventDialog", 0) nsEngageEventDialog::EngageEventDialog(sd);
}

static MenuScreen *CreateUISafeHouseRaceSheet(ScreenConstructorData *sd) {
    return new ("UISafehouseRaceSheet", 0) UISafehouseRaceSheet(sd);
}

static MenuScreen *CreateUIRapSheetLogin(ScreenConstructorData *sd) {
    return new ("uiRapSheetLogin", 0) uiRapSheetLogin(sd);
}

static MenuScreen *CreateUIRapSheetMain(ScreenConstructorData *sd) {
    return new ("uiRapSheetMain", 0) uiRapSheetMain(sd);
}

static MenuScreen *CreateUIRapSheetRS(ScreenConstructorData *sd) {
    return new ("uiRapSheetRS", 0) uiRapSheetRS(sd);
}

static MenuScreen *CreateUIRapSheetUS(ScreenConstructorData *sd) {
    return new ("uiRapSheetUS", 0) uiRapSheetUS(sd);
}

static MenuScreen *CreateUIRapSheetVD(ScreenConstructorData *sd) {
    return new ("uiRapSheetVD", 0) uiRapSheetVD(sd);
}

static MenuScreen *CreateUIRapSheetCTS(ScreenConstructorData *sd) {
    return new ("uiRapSheetCTS", 0) uiRapSheetCTS(sd);
}

static MenuScreen *CreateUIRapSheetTEP(ScreenConstructorData *sd) {
    return new ("uiRapSheetTEP", 0) uiRapSheetTEP(sd);
}

static MenuScreen *CreateUIRapSheetPD(ScreenConstructorData *sd) {
    return new ("uiRapSheetPD", 0) uiRapSheetPD(sd);
}

static MenuScreen *CreateUIRapSheetRankings(ScreenConstructorData *sd) {
    return new ("uiRapSheetRankings", 0) uiRapSheetRankings(sd);
}

static MenuScreen *CreateUIRapSheetRankingsDetail(ScreenConstructorData *sd) {
    return new ("uiRapSheetRankingsDetail", 0) uiRapSheetRankingsDetail(sd);
}

static MenuScreen *CreateUISafeHouseRepSheetMain(ScreenConstructorData *sd) {
    return new ("uiRepSheetMain", 0) uiRepSheetMain(sd);
}

static MenuScreen *CreateUISafeHouseRivalChallenge(ScreenConstructorData *sd) {
    return new ("uiRepSheetRival", 0) uiRepSheetRival(sd);
}

static MenuScreen *CreateUISafeHouseRivalBio(ScreenConstructorData *sd) {
    return new ("uiRepSheetRivalBio", 0) uiRepSheetRivalBio(sd);
}

static MenuScreen *CreateUISafeHouseMilestones(ScreenConstructorData *sd) {
    return new ("uiRepSheetMilestones", 0) uiRepSheetMilestones(sd);
}

static MenuScreen *CreateUISafeHouseRegionUnlock(ScreenConstructorData *sd) {
    return new ("uiSafehouseRegionUnlock", 0) uiSafehouseRegionUnlock(sd);
}

static MenuScreen *CreateUISafeHouseBounty(ScreenConstructorData *sd) {
    return new ("uiRepSheetBounty", 0) uiRepSheetBounty(sd);
}

static MenuScreen *CreateUISafeHouseMarkers(ScreenConstructorData *sd) {
    return new ("FEMarkerSelection", 0) FEMarkerSelection(sd);
}

#ifndef EA_BUILD_A124
static MenuScreen *CreateGameWonScreen(ScreenConstructorData *sd) {
    return new ("FEGameWonScreen", 0) FEGameWonScreen(sd);
}
#endif

static MenuScreen *CreateDebugCarCustomize(ScreenConstructorData *sd) {
    return new ("DebugCarCustomizeScreen", 0) DebugCarCustomizeScreen(sd);
}

static MenuScreen *CreateMyCarsManager(ScreenConstructorData *sd) {
    return new ("MyCarsManager", 0) MyCarsManager(sd);
}

static MenuScreen *CreateCustomizeMainScreen(ScreenConstructorData *sd) {
    return new ("CustomizeMain", 0) CustomizeMain(sd);
}

static MenuScreen *CreateCustomizeSubScreen(ScreenConstructorData *sd) {
    return new ("CustomizeSub", 0) CustomizeSub(sd);
}

static MenuScreen *CreateCustomizeShoppingCartScreen(ScreenConstructorData *sd) {
    return new ("CustomizeShoppingCart", 0) CustomizeShoppingCart(sd);
}

static MenuScreen *CreateCustomizePartsScreen(ScreenConstructorData *sd) {
    return new ("CustomizeParts", 0) CustomizeParts(sd);
}

static MenuScreen *CreateCustomHUDColorScreen(ScreenConstructorData *sd) {
    return new ("CustomizeHUDColor", 0) CustomizeHUDColor(sd);
}

static MenuScreen *CreateDecalsScreen(ScreenConstructorData *sd) {
    return new ("CustomizeDecals", 0) CustomizeDecals(sd);
}

static MenuScreen *CreateNumbersScreen(ScreenConstructorData *sd) {
    return new ("CustomizeNumbers", 0) CustomizeNumbers(sd);
}

static MenuScreen *CreatePaintScreen(ScreenConstructorData *sd) {
    return new ("CustomizePaint", 0) CustomizePaint(sd);
}

static MenuScreen *CreateRimmingScreen(ScreenConstructorData *sd) {
    return new ("CustomizeRims", 0) CustomizeRims(sd);
}

static MenuScreen *CreateSpoilersScreen(ScreenConstructorData *sd) {
    return new ("CustomizeSpoiler", 0) CustomizeSpoiler(sd);
}

static MenuScreen *CreateCustomizePerformanceScreen(ScreenConstructorData *sd) {
    return new ("CustomizePerformance", 0) CustomizePerformance(sd);
}

static MenuScreen *CreateCustomTuningScreen(ScreenConstructorData *sd) {
    return new ("CustomTuningScreen", 0) CustomTuningScreen(sd);
}

static MenuScreen *CreatePostRaceResultsScreen(ScreenConstructorData *sd) {
    return new ("PostRaceResultsScreen", 0) PostRaceResultsScreen(sd);
}

static MenuScreen *CreateBustedOverlayScreen(ScreenConstructorData *sd) {
    return new ("BustedOverlayScreen", 0) BustedOverlayScreen(sd);
}

static MenuScreen *CreatePostRacePursuitScreen(ScreenConstructorData *sd) {
    return new ("PostRacePursuitScreen", 0) PostRacePursuitScreen(sd);
}

static MenuScreen *CreatePostRaceMilestonesScreen(ScreenConstructorData *sd) {
    return new ("PostRaceMilestonesScreen", 0) PostRaceMilestonesScreen(sd);
}

static MenuScreen *CreateCreditsScreen(ScreenConstructorData *sd) {
    return new ("uiCredits", 0) uiCredits(sd);
}

static MenuScreen *CreateUIEATraxScreen(ScreenConstructorData *sd) {
    return new ("UIEATraxScreen", 0) UIEATraxScreen(sd);
}

#ifdef EA_BUILD_A124
static MenuScreen *CreateOnlineMenu(ScreenConstructorData *sd) {
    if (EnterRankingsMenu) {
        return new ("StatsMainMenu", 0) StatsMainMenu(sd);
    }
    if (FEDatabase->IsOptionsMode()) {
        return new ("UIOptionsMain", 0) UIOptionsMain(sd);
    }
    if (FEDatabase->IsModeSelectMode()) {
        return new ("UIQRModeSelect", 0) UIQRModeSelect(sd);
    }
    return new ("UIOnlineMainMenu", 0) UIOnlineMainMenu(sd);
}
#endif

static MenuScreen *CreateLoadingScreen(ScreenConstructorData *sd) {
    return new LoadingScreen(sd);
}

static MenuScreen *CreateLoadingControllerScreen(ScreenConstructorData *sd) {
    return new LoadingControllerScreen(sd);
}

static MenuScreen *CreateOptionsControllerScreen(ScreenConstructorData *sd) {
    return new ("UIOptionsController", 0) UIOptionsController(sd);
}

#ifdef EA_PLATFORM_WIN32
// PC (speed.exe 0x571D00): la pantalla de controles de PC, desde Opciones y desde la pausa. La cadena del `new` es la
// del exe (su limpieza de excepcion, 0x871D20).
static MenuScreen *CreatePausePCController(ScreenConstructorData *sd) {
    return new ("UIPCOptionsController", 0) UIOptionsPCController(sd);
}
#endif

static ScreenFactoryDatum ScreenFactoryData[] = {
    {"MainMenu.fng", CreateMainMenu},
    {"MainMenu_Sub.fng", CreateSubMenu},
    {"Options.fng", CreateOptionsScreen},
    {"OptionsPCDisplay.fng", CreateOptionsScreen},
    {"Quick_Race_Brief.fng", CreateQRBrief},
    {"Track_Select.fng", CreateQRTrackSelect},
    {"Track_Options.fng", CreateQRTrackOptions},
    {"Car_Select.fng", CreateQRCarSelect},
    {"PressStart.fng", CreateQRPressStart},
    {"ChallengeSeries.fng", CreateQRChallengeSeries},
    {"Showcase.fng", CreateShowcase},
    {"Pause_Main.fng", CreateCommonPauseMenu},
    {"Pause_Performance_Tuning.fng", CreateCustomTuningScreen},
    {"FadeScreen.fng", CreateFadeScreen},
    {"WorldMapMain.fng", CreateWorldMap},
    {"Pause_Options.fng", CreateOptionsScreen},
    {"HUD_SingleRace.fng", 0},
    {"HUD_Drag.fng", 0},
    {"InGameAnyMovie.fng", InGameAnyMovieScreen::Create},
#ifndef EA_BUILD_A124
    {"WS_InGameAnyMovie.fng", InGameAnyMovieScreen::Create},
#endif
    {"InGameAnyTutorial.fng", InGameAnyTutorialScreen::Create},
#ifdef EA_PLATFORM_WIN32 // PC: la tabla de speed.exe (0x8F43C8 y 0x8F43D0)
    {"Pause_PCDisplayOptions.fng", CreateOptionsScreen},
    {"Pause_PC_Controller.fng", CreatePausePCController},
#endif
    {"EngageEventDialog.fng", CreateEngageEventDialog},
    {"SafehouseRaceSheet.fng", CreateUISafeHouseRaceSheet},
    {"OPM_SafehouseRaceSheet.fng", CreateUISafeHouseRaceSheet},
    {"SafehouseReputationOverview.fng", CreateUISafeHouseRepSheetMain},
    {"RapSheetLogin.fng", CreateUIRapSheetLogin},
    {"RapSheetLogin2.fng", CreateUIRapSheetLogin},
    {"RapSheetMain.fng", CreateUIRapSheetMain},
    {"RapSheetRS.fng", CreateUIRapSheetRS},
    {"RapSheetUS.fng", CreateUIRapSheetUS},
    {"RapSheetVD.fng", CreateUIRapSheetVD},
    {"RapSheetCTS.fng", CreateUIRapSheetCTS},
    {"RapSheetTEP.fng", CreateUIRapSheetTEP},
    {"RapSheetPD.fng", CreateUIRapSheetPD},
    {"RapSheetRankings.fng", CreateUIRapSheetRankings},
    {"RapSheetRankingsDetail.fng", CreateUIRapSheetRankingsDetail},
    {"SafehouseRivalChallenge.fng", CreateUISafeHouseRivalChallenge},
    {"SafehouseRivalBio.fng", CreateUISafeHouseRivalBio},
    {"SafehouseMilestones.fng", CreateUISafeHouseMilestones},
    {"SafehouseRegionUnlock.fng", CreateUISafeHouseRegionUnlock},
    {"SafehouseBounty.fng", CreateUISafeHouseBounty},
    {"SafehouseMarkers.fng", CreateUISafeHouseMarkers},
    {"SixDaysLater.fng", CreateSixDaysLaterScreen},
    {"InGameRaceSheet.fng", CreateUISafeHouseRaceSheet},
    {"InGameReputationOverview.fng", CreateUISafeHouseRepSheetMain},
    {"InGameMilestones.fng", CreateUISafeHouseMilestones},
    {"InGameRivalChallenge.fng", CreateUISafeHouseRivalChallenge},
    {"InGameRivalBio.fng", CreateUISafeHouseRivalBio},
    {"InGameBounty.fng", CreateUISafeHouseBounty},
    {"SMS_Mailboxes.fng", CreateSMS},
    {"SMS_Message.fng", CreateSMSMessage},
#ifndef EA_BUILD_A124
    {"GameWon.fng", CreateGameWonScreen},
    {"RapSheetLogin_ENDGAME.fng", CreateGameWonScreen},
    {"RapSheetLogin2_ENDGAME.fng", CreateGameWonScreen},
    {"RapSheetMain_ENDGAME.fng", CreateGameWonScreen},
#endif
    {"ControllerUnplugged.fng", CreateControllerUnplugged},
    {"UI_DebugCarCustomize.fng", CreateDebugCarCustomize},
    {"MyCarsManager.fng", CreateMyCarsManager},
    {"CustomizeMain.fng", CreateCustomizeMainScreen},
    {"CustomizeCategory.fng", CreateCustomizeSubScreen},
    {"CustomizeCategory_BACKROOM.fng", CreateCustomizeSubScreen},
    {"CustomizeGenericTop.fng", CreateCustomizeSubScreen},
    {"CustomizeGenericTop_BACKROOM.fng", CreateCustomizeSubScreen},
    {"ShoppingCart.fng", CreateCustomizeShoppingCartScreen},
    {"ShoppingCart_QR.fng", CreateCustomizeShoppingCartScreen},
    {"ShoppingCart_BACKROOM.fng", CreateCustomizeShoppingCartScreen},
    {"CustomizeParts.fng", CreateCustomizePartsScreen},
    {"CustomizeParts_BACKROOM.fng", CreateCustomizePartsScreen},
    {"CustomHUD.fng", CreateCustomizePartsScreen},
    {"CustomHUD_BACKROOM.fng", CreateCustomizePartsScreen},
    {"CustomHUDColor.fng", CreateCustomHUDColorScreen},
    {"CustomHUDColor_BACKROOM.fng", CreateCustomHUDColorScreen},
    {"Decals.fng", CreateDecalsScreen},
    {"Decals_BACKROOM.fng", CreateDecalsScreen},
    {"Numbers.fng", CreateNumbersScreen},
    {"Paint.fng", CreatePaintScreen},
    {"Paint_BACKROOM.fng", CreatePaintScreen},
    {"Rims.fng", CreateRimmingScreen},
    {"Rims_BACKROOM.fng", CreateRimmingScreen},
    {"Spoilers.fng", CreateSpoilersScreen},
    {"Spoilers_BACKROOM.fng", CreateSpoilersScreen},
    {"CustomizePerformance.fng", CreateCustomizePerformanceScreen},
    {"CustomizePerformance_BACKROOM.fng", CreateCustomizePerformanceScreen},
    {"GarageMain.fng", CreateGarageMainScreen},
    {"DiscError.fng", 0},
    {"Dialog.fng", 0},
    {"GenericDialog_ThreeButton.fng", 0},
#ifndef EA_BUILD_A124
    {"GameOver.fng", 0},
#endif
    {"EA_Trax_Jukebox.fng", CreateUIEATraxScreen},
    {"EA_Trax.fng", CreateChyronScreen},
    {"Chyron_IG.fng", CreateChyronScreen},
    {"InGameDialog.fng", 0},
    {"Keyboard.fng", CreateFEKeyboard},
#ifndef EA_BUILD_A124
    {"Keyboard_GC.fng", CreateFEKeyboard},
#endif
    {"ScreenPrintf.fng", 0},
    {"Credits.fng", CreateCreditsScreen},
    {"FEAnyMovie.fng", FEAnyMovieScreen::Create},
#ifndef EA_BUILD_A124
    {"WS_FEAnyMovie.fng", FEAnyMovieScreen::Create},
#endif
    {"FEAnyTutorial.fng", FEAnyTutorialScreen::Create},
    {"LS_EALogo.fng", CreateMovieScreen},
    {"LS_EA_hidef.fng", CreateMovieScreen},
    {"LS_PSA.fng", CreateMovieScreen},
    {"LS_THXMovie.fng", CreateMovieScreen},
    {"MW_LS_IntroFMV.fng", CreateMovieScreen},
    {"MW_LS_AttractFMV.fng", CreateMovieScreen},
    {"MW_LS_Splash.fng", CreateSplashScreen},
    {"WS_LS_EALogo.fng", CreateMovieScreen},
    {"WS_LS_EA_hidef.fng", CreateMovieScreen},
    {"WS_LS_PSA.fng", CreateMovieScreen},
    {"WS_LS_IntroFMV.fng", CreateMovieScreen},
    {"WS_MW_LS_AttractFMV.fng", CreateMovieScreen},
    {"WS_MW_LS_Splash.fng", CreateSplashScreen},
    {"Loading_Tips.fng", CreateLoadingTipsScreen},
    {"loading_boot.fng", 0},
    {"LS_LangSelect.fng", CreateLanguageSelectScreen},
#ifdef EA_PLATFORM_WIN32 // PC: la tabla de speed.exe (0x8F46C8)
    {"Options_PC_Controller.fng", CreatePausePCController},
#endif
    {"Loading.fng", CreateLoadingScreen},
    {"WS_Loading.fng", CreateLoadingScreen},
    {"Loading_Controller.fng", CreateLoadingControllerScreen},
    {"WS_Loading_Controller.fng", CreateLoadingControllerScreen},
    {"UI_OptionsController.fng", CreateOptionsControllerScreen},
    {"Pause_Controller.fng", CreateOptionsControllerScreen},
    {"PostRace_Results.fng", CreatePostRaceResultsScreen},
    {"BUSTED_OVERLAY.fng", CreateBustedOverlayScreen},
    {"PostBusted.fng", CreatePostRacePursuitScreen},
    {"Infractions.fng", PostPursuitInfractionsScreen::Create},
    {"InGamePhotoMaster.fng", PhotoFinishScreen::Create},
    {"PostRace_Pursuit.fng", CreatePostRacePursuitScreen},
    {"PostRace_MilestoneRewards.fng", CreatePostRaceMilestonesScreen},
#ifdef EA_BUILD_A124
    {"UI_OL_Disconnect.fng", CreateOnlineDisconnectDialog},
    {"UI_OL_Disconnect_BG.fng", CreateOnlineDisconnectBG},
    {"UI_OL_WebOffer2.fng", CreateUIOLEATermsScreen},
    {"OL_News_and_Terms.fng", CreateNewsAndTerms},
    {"OL_LobbyRoom.fng", CreateUIOLLobbyRoomScreen},
    {"OL_GameRoom.fng", CreateUIOLGameRoomScreen},
    {"OL_GameRoom_Player_Details.fng", CreateUIOLGameRoomHostScreen},
    {"OL_GameRoom_Edit_Race.fng", CreateEditGameDetailsScreen},
    {"OL_GameRoom_Car_Select.fng", CreateGameRoomCarSelectScreen},
    {"UI_OLViewTrack.fng", CreateOnlineViewTrack},
    {"OL_MAIN.fng", CreateOnlineMenu},
    {"OL_Quickrace_Main.fng", CreateOnlineQuickJoin},
    {"UI_DateEntry.fng", CreateUIDateMenu},
    {"OL_Dialog.fng", 0},
    {"OL_Dialog_Stacked_Buttons.fng", 0},
    {"UI_OLISPConnect.fng", CreateOLConnectingScreen},
    {"UI_OLFilters.fng", CreateOnlineFilters},
    {"UI_OLX_OptiMatch_Available.fng", CreateUIOLLobbyRoomScreen},
    {"UI_OLX_OptiMatch_Filters.fng", CreateOnlineFilters},
    {"OLX_Connecting2XBL.fng", CreateOLConnectingScreen},
    {"OL_Rankings.fng", CreateOnlineOverallRankings},
    {"OL_Rankings_Personal.fng", CreateOnlinePersonalRankings},
    {"OL_Rankings_Personal_Overlay.fng", CreateOnlinePersonalRankings},
    {"OL_Rankings_Leaderboard.fng", CreateOnlineRankingsLeaderboard},
    {"OL_EAMessenger.fng", CreateOnlineEA_Messenger},
    {"OL_FriendDialogue.fng", CreateUIOnlineEAM_Options},
    {"OLX_Message.fng", CreateUIOnlineChatDialog},
    {"OLX_FindResults.fng", CreateUIOnlineSearchResult},
    {"OL_Feedback.fng", CreateUIOnlineEAM_Feedback},
    {"OL_Challenge.fng", CreateuiOnlineChallengeInvite},
    {"OL_VoiceChat.fng", CreateOnlineVoiceInvite},
    {"UI_OLNetSelect.fng", CreateUIOLNetSelectScreen},
    {"UI_OLAuthDNAS.fng", CreateUIOLAuthDNASScreen},
    {"UI_OLISPConnect.fng", CreateOLConnectingScreen},
    {"OL_SelectPersona.fng", CreateUIOLSelectPersonaScreen},
    {"OL_Create_Account.fng", CreateUIOLCreateUserScreenPart1},
    {"UI_OLX_Info_Sharing.fng", CreateUIOLCreateUserScreenPart2},
    {"OL_Create_Account_Country.fng", CreateUIOLCreateUserCountry},
    {"UI_OLAgeVerif.fng", CreateUIOLAgeVerf},
    {"OL_UseExisting.fng", CreateUIOLUseExistingScreen},
    {"OL_ForgotAccountName.fng", CreateUIGetUserEmailAddress},
    {"OLEALogin.fng", CreateUIOLEALoginScreen},
#endif
    {"MC_ProfileManager.fng", CreateUIProfileManager},
    {"MC_Deleteprofile.fng", CreateUIDeleteProfile},
    {"MC_Bootup.fng", CreateMemCardBootScreen},
    {"MC_Bootup_GC.fng", CreateMemCardBootScreen},
    {"MC_List.fng", CreateMemcardListFiles},
    {"InGame_MC_Main.fng", CreateMemcardMainMenu},
    {"InGame_MC_Main_GC.fng", CreateMemcardMainMenu},
    {"MC_Main.fng", CreateMemcardMainMenu},
    {"MC_Main_GC.fng", CreateMemcardMainMenu},
};

static ScreenButtonDatum ScreenButtonData[50] = {0};
int FEPackageData::mInScreenConstructor = 0;

static MenuScreen *ScreenFactory(uint32 hash, FEPackage *pkg, int arg) {
    ScreenConstructorData sd;
    for (int i = 0; i < NUM_ELEMENTS(ScreenFactoryData); i++) {
        if (hash == FEHashUpper(ScreenFactoryData[i].FEngPackageFilename) && ScreenFactoryData[i].ConstructorFunction != 0) {
            ScreenConstructorData data = {0};
            data.PackageFilename = ScreenFactoryData[i].FEngPackageFilename;
            data.pPackage = pkg;
            data.Arg = arg;
            sd = data;
            return ScreenFactoryData[i].ConstructorFunction(&sd);
        }
    }
    return 0;
}

static ScreenFactoryDatum *FindScreenCreateData(uint32 screen_filename_hash) {
    for (int i = 0; i < NUM_ELEMENTS(ScreenFactoryData); i++) {
        if (screen_filename_hash == FEHashUpper(ScreenFactoryData[i].FEngPackageFilename)) {
            return &ScreenFactoryData[i];
        }
    }
    return 0;
}

static ScreenButtonDatum *FindScreenButtonDatum(uint32 screen_filename_hash) {
    for (int i = 0; i <= 49; i++) {
        if (screen_filename_hash == ScreenButtonData[i].ScreenHash) {
            if (FEDatabase->MatchesGameMode(ScreenButtonData[i].GameMode)) {
                return &ScreenButtonData[i];
            }
        }
    }
    return 0;
}

static ScreenButtonDatum *FindAvailableButtonDatum() {
    for (int i = 0; i <= 49; i++) {
        if (ScreenButtonData[i].ScreenHash == 0) {
            return &ScreenButtonData[i];
        }
    }
    return 0;
}

uint8 FEngGetLastButton(const char *pkg_name) {
    ScreenButtonDatum *sd = FindScreenButtonDatum(FEHashUpper(pkg_name));
    if (sd != 0) {
        return sd->LastButton;
    }
    return 0;
}

void FEngSetLastButton(const char *pkg_name, uint8 button_hash) {
    ScreenButtonDatum *sd = FindScreenButtonDatum(FEHashUpper(pkg_name));
    if (sd != 0) {
        sd->LastButton = button_hash;
    } else {
        sd = FindAvailableButtonDatum();
        if (sd != 0) {
            sd->ScreenHash = FEHashUpper(pkg_name);
            sd->LastButton = button_hash;
            sd->GameMode = FEDatabase->GetGameMode();
        }
    }
}

void FEngSetCreateCallback(const char *abstract_pkg_name, MenuScreenCreateFunction function) {
    for (int i = 0; i < NUM_ELEMENTS(ScreenFactoryData); i++) {
        ScreenFactoryDatum &sfd = ScreenFactoryData[i];
        if (bStrICmp(abstract_pkg_name, sfd.FEngPackageFilename) == 0) {
            sfd.ConstructorFunction = function;
        }
    }
}

static inline uint32 ReadLEu32(void *ptr) {
    uint32 w = *(uint32 *)ptr;
#ifdef NATIVE_ENDIAN_BIG
    return (w >> 24) | (w << 24) | ((w << 8) & 0xFF0000) | ((w >> 8) & 0xFF00);
#else
    return w;
#endif
}

FEPackageData::FEPackageData(bChunk *chunk) {
    IsVisible = true;
    MyChunk = chunk;
    pScreen = 0;
    pPackage = 0;
    LastKnownControlMask = 0;
    bWasSetupForHotchunk = 0;
    IsPermanent = 0;
    CreateData = 0;
    RenderInfo.EpolySlotPool = 0;
    RenderInfo.AllowOverflows = 0;
    mArg = 0;
    if (chunk->GetID() == BCHUNK_FENG_PACKAGE) {
        DataChunk = chunk->GetData();
    } else if (chunk->GetID() == BCHUNK_FENG_COMPRESSED_PACKAGE) {
        DataChunk = 0;
    }
    CreateData = FindScreenCreateData(GetNameHash());
}

FEPackageData::~FEPackageData() {
    if (MyChunk->GetID() != BCHUNK_FENG_PACKAGE && MyChunk->GetID() == BCHUNK_FENG_COMPRESSED_PACKAGE) {
        bFree(DataChunk);
    }
    DataChunk = 0;
}

void FEPackageData::Activate(FEPackage *pkg, int arg) {
#ifdef EA_BUILD_A124
    if (pScreen) {
        char oldPackageName[1000] = "(unknown)";
        if (pScreen->GetPackageName()) {
            bStrCpy(oldPackageName, pScreen->GetPackageName());
        }
        char newPackageName[1000] = "(unknown)";
        if (pkg && pkg->GetFilename()) {
            bStrCpy(newPackageName, pkg->GetFilename());
        }
    }
#endif
    pPackage = pkg;
    pkg->SetUserParam((u32)this);
    mInScreenConstructor = mInScreenConstructor + 1;
    pScreen = ScreenFactory(GetNameHash(), pkg, arg);
    LastKnownControlMask = pkg->GetControlMask();
    mInScreenConstructor = mInScreenConstructor - 1;
#ifdef EA_BUILD_A124
    Juice::MWCommands::LoadingNewFEngPackage(pkg->GetNameHash());
#endif
}

void FEPackageData::UnActivate() {
    if (pScreen != 0) {
        delete pScreen;
    }
    pScreen = 0;
    if (pPackage != 0) {
        pPackage->SetUserParam(0);
    }
    pPackage = 0;
    if (IsCompressedChunk()) {
        bFree(DataChunk);
        DataChunk = 0;
    }
}

void FEPackageData::Close() {
    if (pPackage != 0) {
        HackClearCache(pPackage);
        RenderObjectDisconnect disconnect;
        disconnect.pFEngRenderer = cFEngRender::mInstance;
        disconnect.PkgRenderInfo = HACK_FEPkgMgr_GetPackageRenderInfo(pPackage);
        pPackage->ForAllObjects(disconnect);
        cFEng::Get()->UnloadPackage(pPackage);
        UnActivate();
    }
}

void *FEPackageData::GetDataChunk() {
    if (MyChunk->GetID() == BCHUNK_FENG_PACKAGE) {
        return DataChunk;
    } else if (MyChunk->GetID() == BCHUNK_FENG_COMPRESSED_PACKAGE) {
        unsigned int *chunkdata = reinterpret_cast<unsigned int *>(MyChunk->GetData());
        LZHeader *header = reinterpret_cast<LZHeader *>(&chunkdata[1]);
        int data_bytes = header->UncompressedSize;
        DataChunk = bMalloc(data_bytes, "DECOMPRESSED FENG PACKAGE", 0, GetVirtualMemoryAllocParams());
        LZDecompress(reinterpret_cast<uint8 *>(header), reinterpret_cast<uint8 *>(DataChunk));
        return DataChunk;
    }
    return 0;
}

uint32 FEPackageData::GetNameHash() {
    if (MyChunk->GetID() == BCHUNK_FENG_PACKAGE) {
        uint8 *data = (uint8 *)GetDataChunk();
        if (ReadLEu32(data) != Chunk_FEPackage) {
            return 0;
        }
        if (ReadLEu32(data + 8) != Chunk_FEPackageHeader) {
            return 0;
        }
        uint8 *header = data + 8;
        if (ReadLEu32(header + 8) < FENG_PACKAGEWRITER_VERSION) {
            return 0;
        }
        return FEHashUpper((char *)(data + 0x28));
    } else if (MyChunk->GetID() == BCHUNK_FENG_COMPRESSED_PACKAGE) {
        return *(uint32 *)MyChunk->GetData();
    }
    return 0;
}

void FEPackageData::NotificationMessage(u32 Message, FEObject *pObject, u32 Param1, u32 Param2) {
    if (pScreen != 0) {
        pScreen->BaseNotify(Message, pObject, Param1, Param2);
    }
}

void FEPackageData::NotifySoundMessage(u32 msg, FEObject *obj, u32 control_mask, u32 pkg_ptr) {
    if (pScreen != 0) {
        pScreen->BaseNotifySound(msg, obj, control_mask, pkg_ptr);
    }
}
