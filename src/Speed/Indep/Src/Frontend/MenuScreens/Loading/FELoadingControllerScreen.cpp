#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FELoadingControllerScreen.hpp"

#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/FEng/FEList.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Misc/Joystick.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/FEHash_FeBonusCards.hpp"
#endif

void FinishLoadingControllerTextureCallbackBridge(unsigned int p);
extern bool bSawLoadingScreen;

void *LoadingControllerScreen::mLoadingControllerScreenPtr = nullptr;

LoadingControllerScreen::LoadingControllerScreen(ScreenConstructorData *sd) : MenuScreen(sd), LoadingFinished(0) {
    if (eIsWidescreen()) {
        cFEng::Get()->QueuePackageMessage(bStringHash("CURRENT_GEN_WIDESCREEN"), GetPackageName(), nullptr);
    }

    GameTipToShow = nullptr;

    HideControllerConfig();
#ifndef EA_BUILD_A124
    SetupControllerConfig();
#endif
    PrepToShowControllerConfig();
}

LoadingControllerScreen::~LoadingControllerScreen() {
    ClearLoadedControllerTexture();
}

void LoadingControllerScreen::SetupControllerConfig() {
    if (!FEDatabase->IsCareerMode()) {
        cFEng::Get()->QueuePackageMessage(0xDE511657, GetPackageName(), nullptr);
    }

    JoystickPort port = static_cast<JoystickPort>(FEDatabase->GetPlayersJoystickPort(0));
    int config = FEDatabase->GetPlayerSettings(0)->Config;
    char sztemp[32];

    for (int i = 0; i <= 16; i++) {
        FEngSNPrintf(sztemp, 0x20, "CButton_%d", i + 1);
        unsigned int obj_hash = FEHashUpper(sztemp);
        FEngSNPrintf(sztemp, 0x20, "BUTTON_%d", i + 1);
        unsigned int img_hash = FEHashUpper(sztemp);
        unsigned int button_hash = FindButtonNameHashForFEString(config, i, port);
        if (button_hash != 0) {
            FEngSetVisible(GetPackageName(), obj_hash);
            FEngSetLanguageHash(GetPackageName(), obj_hash, button_hash);
            FEngSetVisible(GetPackageName(), img_hash);
        } else {
            FEngSetInvisible(GetPackageName(), obj_hash);
            FEngSetInvisible(GetPackageName(), img_hash);
        }
    }

    if (FEDatabase->GetPlayerSettings(0)->DriveWithAnalog) {
        FEngSetTextureHash(GetPackageName(), 0x4592229C, 0x00148E38);
    } else {
#ifdef EA_PLATFORM_PLAYSTATION2
        FEngSetTextureHash(GetPackageName(), 0x4592229C, 0x0B30961B);
#else
        FEngSetButtonTexture(FEngFindImage(GetPackageName(), 0x4592229C), 0x0B30961B);
#endif
    }

#ifndef EA_BUILD_A124
    FEngSetInvisible(GetPackageName(), 0x0F274B86);
#endif
    FEngSetInvisible(GetPackageName(), 0x673D77BC);
    FEngSetInvisible(GetPackageName(), 0x351AE442);
#ifndef EA_PLATFORM_WIN32 // PC no pone las texturas de los botones del mando (speed.exe 0x57E180)
#ifdef EA_PLATFORM_PLAYSTATION2
#ifndef EA_BUILD_A124 // SLUS vuelve a poner la palanca analogica y los botones del mando de PS2
    if (FEDatabase->GetPlayerSettings(0)->DriveWithAnalog) {
        FEngSetTextureHash(GetPackageName(), 0x4592229C, 0x00148E38);
    } else {
        FEngSetButtonTexture(FEngFindImage(GetPackageName(), 0x4592229C), 0x0B30961B);
    }
    FEngSetButtonTexture(FEngFindImage(GetPackageName(), 0x81B57400), 0x0258D432);
    FEngSetButtonTexture(FEngFindImage(GetPackageName(), 0x81B57401), 0xFBD55D85);
#endif
#else
    FEngSetTextureHash(GetPackageName(), 0x81B57400, 0x02959349);
    FEngSetTextureHash(GetPackageName(), 0x81B57401, 0x6851AAF5);
#endif
    FEngSetTextureHash(GetPackageName(), 0x81B57402, 0x03B7F86D);
#endif
}

void LoadingControllerScreen::ShowControllerConfig() {
    FEngSetScript(GetPackageName(), 0x3248E720, FEHASH_SHOW, true);
}

void LoadingControllerScreen::HideControllerConfig() {
    FEngSetScript(GetPackageName(), 0x3248E720, FEHASH_HIDE, true);

    WhichControllerTexture = 0;
}

void LoadingControllerScreen::FinishLoadingControllerTextureCallback(uint32 p) {
#ifdef EA_BUILD_A124
    SetupControllerConfig();
#endif
    ShowControllerConfig();
}

void FinishLoadingControllerTextureCallbackBridge(unsigned int p) {
    if (p) {
        reinterpret_cast<LoadingControllerScreen *>(p)->FinishLoadingControllerTextureCallback(0);
    }
}

void LoadingControllerScreen::PrepToShowControllerConfig() {
    unsigned int texture_hash = 0;

#ifdef EA_PLATFORM_PLAYSTATION2 // las texturas del mando de PS2: el volante de Logitech y el mando coreano
    if (IsJoystickTypeWheel(static_cast<JoystickPort>(FEDatabase->GetPlayersJoystickPort(0)))) {
#ifndef EA_BUILD_A124 // la A124 no cambia la configuracion
        FEDatabase->GetPlayerSettings(0)->Config = CC_CONFIG_1;
#endif
        texture_hash = bStringHash("LOGITECH_WHEEL");
        FEngSetInvisible(GetPackageName(), 0x0F274B7F);
        FEngSetInvisible(GetPackageName(), 0x0F274B80);
    } else if (GetCurrentLanguage() == eLANGUAGE_KOREAN) {
        texture_hash = 0x07FBF7A5;
    } else if (FEDatabase->GetPlayerSettings(0)->DriveWithAnalog) {
        texture_hash = 0xDF7B27A7;
    } else {
        texture_hash = 0xDF7B27A8;
    }
#elif defined(EA_PLATFORM_WIN32) // PC no tiene imagen del mando: textura 0 (speed.exe 0x56B190)
#else
    if (IsJoystickTypeWheel(static_cast<JoystickPort>(FEDatabase->GetPlayersJoystickPort(0)))) {
        FEDatabase->GetPlayerSettings(0)->Config = CC_CONFIG_1;
        texture_hash = 0xB511476B;
    } else {
        texture_hash = 0xED543BAC;
        if (FEDatabase->GetPlayerSettings(0)->DriveWithAnalog) {
            texture_hash = 0xED543BAB;
        }
    }
#endif

    WhichControllerTexture = texture_hash;
    FEngSetTextureHash(GetPackageName(), 0x922A39C4, texture_hash);
    eLoadStreamingTexture(WhichControllerTexture, FinishLoadingControllerTextureCallbackBridge, reinterpret_cast<uintptr_t>(this), 0);
}

void LoadingControllerScreen::ClearLoadedControllerTexture() {
    if (WhichControllerTexture != 0) {
        eUnloadStreamingTexture(WhichControllerTexture);
    }
}

// Estripada en GameCube y PS2 (Range 0xFFFFFFFF); PC la tiene.
void LoadingControllerScreen::NotifyLoadingFinished() {
    LoadingFinished = 1;
    bSawLoadingScreen = true;
}

void LoadingControllerScreen::NotificationMessage(u32 msg, FEObject *obj, u32 p1, u32 p2) {
#ifdef EA_BUILD_A124
    if (msg == __PAD_ACCEPT__) {
        g_pEAXSound->PlayUISoundFX(UISND_COMMON_SELECT);
        bSawLoadingScreen = true;
    }
#endif
}

void LoadingControllerScreen::InitLoadingControllerScreen() {
    mLoadingControllerScreenPtr = bMalloc(sizeof(LoadingControllerScreen), "LoadingControllerScreen", 0, 0);
}

// Estripada en GameCube y PS2 (Range 0xFFFFFFFF); PC la tiene.
void LoadingControllerScreen::CloseLoadingControllerScreen() {
    bFree(mLoadingControllerScreenPtr);
    mLoadingControllerScreenPtr = nullptr;
}
