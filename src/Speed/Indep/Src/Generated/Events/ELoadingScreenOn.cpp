#include "ELoadingScreenOn.hpp"

#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FeFadeScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingTips.hpp"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOff.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageData.hpp"

extern const char *LoadingBootName;
extern bool bSawLoadingScreen;


#ifdef EA_BUILD_A124
// La A124 no mira la direccion de la carga para elegir el consejo, tiene su version panoramica de la pantalla de consejos
// y no toca el error de los mandos.
ELoadingScreenOn::ELoadingScreenOn(LoadingScreen::LoadingScreenTypes pLoadingType) : Event(0x10), fLoadingType(pLoadingType) {
    const char *load_screen;
    int screen_arg;
    eGameTips tipID = LoadingTips::WhatTipScreenShouldIUseToday();

    if (tipID != GAME_TIP_NONE) {
        if (tipID == GAME_TIP_USE_CONTROLLER_CONFIG) {
            if (eIsWidescreen()) {
                load_screen = "WS_Loading_Controller.fng";
            } else {
                load_screen = "Loading_Controller.fng";
            }
        } else {
            if (eIsWidescreen()) {
                load_screen = "WS_Loading_Tips.fng";
            } else {
                load_screen = "Loading_Tips.fng";
            }
        }
        screen_arg = tipID;
    } else {
        if (eIsWidescreen()) {
            load_screen = "WS_Loading.fng";
        } else {
            load_screen = "Loading.fng";
        }
        screen_arg = fLoadingType;
    }

    SetLoadingScreenPackageName(load_screen);

    if (cFEng::Get()->IsPackageInControl(LoadingBootName)) {
        cFEng::Get()->QueuePackagePop(1);
    }

    int JoyPort = FEDatabase->GetPlayersJoystickPort(0);

    if (!cFEng::Get()->IsPackagePushed(load_screen)) {
        cFEng::Get()->QueuePackagePush(load_screen, screen_arg, FEngMapJoyportToJoyParam(JoyPort), false);
    }

    bSawLoadingScreen = false;

    if (FadeScreen::IsFadeScreenOn()) {
        new EFadeScreenOff(FEHASH_15_IN);
    }
}
#else
ELoadingScreenOn::ELoadingScreenOn(LoadingScreen::LoadingScreenTypes pLoadingType) : Event(0x10), fLoadingType(pLoadingType) {
    const char *load_screen;
    int screen_arg = pLoadingType;
    eGameTips tipID = LoadingTips::WhatTipScreenShouldIUseToday(pLoadingType);

    if (tipID != GAME_TIP_NONE) {
        if (tipID == GAME_TIP_USE_CONTROLLER_CONFIG) {
            if (eIsWidescreen()) {
                load_screen = "WS_Loading_Controller.fng";
            } else {
                load_screen = "Loading_Controller.fng";
            }
        } else {
            load_screen = "Loading_Tips.fng";
            LoadingTips::SetDoneLoading(false);
            screen_arg = tipID;
        }
    } else {
        if (eIsWidescreen()) {
            load_screen = "WS_Loading.fng";
        } else {
            load_screen = "Loading.fng";
        }
    }

    SetLoadingScreenPackageName(load_screen);

    if (cFEng::Get()->IsPackageInControl(LoadingBootName)) {
        cFEng::Get()->QueuePackagePop(1);
    }

    int JoyPort = FEDatabase->GetPlayersJoystickPort(0);

    FEManager::Get()->AllowControllerError(false);

    if (!cFEng::Get()->IsPackagePushed(load_screen)) {
        cFEng::Get()->QueuePackagePush(load_screen, screen_arg, FEngMapJoyportToJoyParam(JoyPort), false);
    }

    bSawLoadingScreen = false;

    if (FadeScreen::IsFadeScreenOn()) {
        new EFadeScreenOff(FEHASH_15_IN);
    }
}
#endif

ELoadingScreenOn::~ELoadingScreenOn() {
}

const char *ELoadingScreenOn::GetEventName() const {
    return "ELoadingScreenOn";
}

void ELoadingScreenOn_MakeEvent_Callback(const void *staticData) {
    new ELoadingScreenOn(((ELoadingScreenOn::StaticData *) staticData)->fLoadingType);
}
