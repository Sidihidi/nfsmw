// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

#include "Speed/Indep/Src/Frontend/HUD/FeHudElement.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeGetawayMeter.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeRadarDetector.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeHeatMeter.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeCostToState.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeReputation.cpp"
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_XENON)
#include "Speed/Indep/Src/Frontend/HUD/FeOnlineHudSupport.cpp"
#endif
#ifdef EA_PLATFORM_XENON
// Solo el EGM conserva su codigo (/OPT:NOREF; GameCube y la A124 lo tiran al enlazar): 0x822C0D28-0x822C0FF8.
#include "Speed/Indep/Src/Frontend/HUD/FeQueuedMessageSwapper.cpp"
#endif

#include "Speed/Indep/Src/Frontend/HUD/FeMinimapStreamer.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeMinimap.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeCountdown.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeGenericMessage.cpp"

#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/HUD/FeAutoSaveIcon.cpp"
#endif

#include "Speed/Indep/Src/Frontend/HUD/FeRaceOverMessage.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeNitrousGauge.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FESpeedBreakerMeter.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeEngineTempGauge.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeSpeedometer.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeTachometer.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeWrongWIndi.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeRaceInformation.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeLeaderBoard.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeMilestoneBoard.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FePursuitBoard.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeTimeExtension.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeBustedMeter.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeTurboMeter.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeMenuZoneTrigger.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FEPkg_Hud.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeShiftUpdater.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FETachometerDrag.cpp"

#include "Speed/Indep/Src/Frontend/HUD/FeInfractions.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiInfractions.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FeFadeScreen.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FeBustedOverlay.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_Chyron.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_ConfirmDialog.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEpkg_MU_Keyboard.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPKg_PostRace.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/PhotoFinish.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/uiSixDaysLater.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_OKDialog.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FEBootFlowManager.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FESplashScreen.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FEMovieScreen.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingTips.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingScreen.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELanguageSelect.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingControllerScreen.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Career/FEPkg_EngageEventDialog.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/InGameMovieScreen.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/InGameTutorialScreen.cpp"

#include "Speed/Indep/Src/Frontend/Localization/Localize.cpp"

#include "Speed/Indep/Src/Frontend/Localization/WideCharHistogram.cpp"
// Las milestone (la A124 y el EGM de X360, donde ScreenPrintf y GetUnusedScreenPrintItem estan entre
// WideCharHistogram y RaceStarter, 0x822E4BC0-0x822E5898).
#ifdef MILESTONE_BUILD
#include "Speed/Indep/Src/Frontend/ScreenPrintf.cpp"
#endif

#include "Speed/Indep/Src/Frontend/RaceStarter.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feKeyboardInput.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/Slider.cpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.cpp"
#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // PC y el EGM: cOnlineSettings::Default/SetState viven en zFe2
#include "Speed/Indep/Src/Frontend/Database/OnlineDB.cpp"
#endif

#include "Speed/Indep/Src/Frontend/Database/RaceDB.cpp"

#include "Speed/Indep/Src/Frontend/Database/VehicleDB.cpp"

#include "Speed/Indep/Src/Frontend/FERenderObject.cpp"

#include "Speed/Indep/Src/Frontend/FEngRender.cpp"

#include "Speed/Indep/Src/Frontend/FEPackageData.cpp"

#include "Speed/Indep/Src/Frontend/FEPackageManager.cpp"

#include "Speed/Indep/Src/Frontend/FEngFont.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.cpp"

#include "Speed/Indep/Src/Frontend/FEngFrontend.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feIconScrollerMenu.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feArrayScrollerMenu.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feUIWidgetMenu.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/uiDateMenu.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feWidget.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/customize/FECustomize.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/CustomTuning.cpp"

#include "Speed/Indep/Src/Frontend/Careers/UnlockSystem.cpp"
