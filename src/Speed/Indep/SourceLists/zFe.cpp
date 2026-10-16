// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// datos-gc 2026-10-13: en la CU original van seis _GetKind (MGeneric, MAudioReflection,
// MControlPathfinder, MRestartRace, MEnterFreeRoam, MQuitToFrontEnd) entre los hash de
// WorldConn y el k de MNotifyRaceAbandoned (guarda _18, duena en este slice).
#include "Speed/Indep/Src/Generated/Messages/MGeneric.h"
#include "Speed/Indep/Src/Generated/Messages/MAudioReflection.h"
#include "Speed/Indep/Src/Generated/Messages/MControlPathfinder.h"
#include "Speed/Indep/Src/Generated/Messages/MRestartRace.h"
#include "Speed/Indep/Src/Generated/Messages/MEnterFreeRoam.h"
#include "Speed/Indep/Src/Generated/Messages/MQuitToFrontEnd.h"

// FEObjectCallback.h es `#pragma interface`: el original define su vtable y su
// destructor, globales, en esta unidad.
#pragma implementation "FEObjectCallback.h"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/uiMain.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionWidgets.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsMain.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsScreen.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsTrailers.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsController.cpp"

#ifdef EA_PLATFORM_WIN32 // la pantalla de controles de PC (Options_PC_Controller.fng)
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsPCController.cpp"
#endif

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/uiPause.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiTrackMapStreamer.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiEATraxJukebox.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetLogin.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetMain.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetRS.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetUS.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetVD.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetCTS.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetTEP.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetPD.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetRankings.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRapSheetRankingsDetail.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetMain.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetRaceEvents.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetRival.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetRivalBio.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetMilestones.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetBounty.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetRivalStreamer.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiSafehouseRegionUnlock.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiRepSheetRivalFlow.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/uiWorldMap.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/uiSMS.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/uiSMSMessage.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiCareerMain.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/career/uiCareerManager.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Career/FEGameWonScreen.cpp"

#include "Speed/Indep/Src/Frontend/UnicodeFile.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiCredits.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/ControllerUnplugged.cpp"

// Solo el prototipo: las pantallas online, en el orden del .mdebug de la A124.
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOnlineDisconnect.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineLogin.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOnlineQuickJoin.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOLLobbyRoom.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineMain.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOnlineMainMenu.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/GameRoomSubMenus.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/StatsMainMenu.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlinePersonalRankings.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineOverallRankings.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineFilters.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineViewTrack.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/EAMessenger.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOnlineEA_Messenger.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineEAM_Options.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineVoiceInvite.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineSearchResult.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOnlineChatDialog.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOnlineEAM_Feedback.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/uiOnlineChallengeInvite.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/Online/UIOnlineRankingsLeaderboard.cpp"
#endif

#include "Speed/Indep/Src/Frontend/FEJoyInput.cpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.cpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.cpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.cpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEMovies.cpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.cpp"

#include "Speed/Indep/Src/Frontend/FEObjectCallbacks.cpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEGameInterface.cpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.cpp"

#include "Speed/Indep/Src/Frontend/FEManager.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEAnyMovieScreen.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEAnyTutorialScreen.cpp"

// Solo el prototipo: el menu de depuracion, en el orden del .mdebug de la A124.
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/DebugMenu.cpp"

#include "Speed/Indep/Src/Frontend/DebugMenuWidgets.cpp"

#include "Speed/Indep/Src/Frontend/NFSDebugMenu.cpp"
#endif

#include "Speed/Indep/Src/Frontend/MoviePlayer/MoviePlayer.cpp"

#include "Speed/Indep/Src/Frontend/SubTitle.cpp"

#include "Speed/Indep/Src/Frontend/Database/uiProfileManager.cpp"

// datos-gc 2026-10-13: SmackableParams (_21 en la CU original, con VehicleParams) y
// MNotifyMovieFinished (_23) se parsean antes de los dos estaticos de MemoryCard::Init
// (_24/_25, duenos en este slice).
#include "Speed/Indep/Src/Physics/Smackable.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyMovieFinished.h"

#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.cpp"

#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCardCallbacks.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/MemCard/uiMemcard.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/MemCard/uiMemcardBase.cpp"

#include "Speed/Indep/Src/Frontend/MenuScreens/MemCard/uiMemcardInterface.cpp"
