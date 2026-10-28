#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/InGameTutorialScreen.hpp"

#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageData.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_Chyron.hpp"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOff.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/Src/Frontend/SubTitle.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEMovies.hpp"
#include "Speed/Indep/Src/Generated/Messages/MNotifyMovieFinished.h"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOn.hpp"
#ifdef EA_PLATFORM_WIN32
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Misc/Main.hpp"
#include "Speed/Indep/Src/World/CarLoader.hpp"
#include "Speed/Indep/Src/World/TrackStreamer.hpp"
#endif

extern bool gInGameMoviePlaying;

char InGameAnyTutorialScreen::MovieFilename[64] = "unknown";
char InGameAnyTutorialScreen::PackageFilename[64] = "unknown";
bool InGameAnyTutorialScreen::PackageSet = false;
static const char *InGameTutorialScreenName = "InGameAnyTutorial.fng";

InGameAnyTutorialScreen::InGameAnyTutorialScreen(ScreenConstructorData *sd) : MenuScreen(sd) {
#ifndef EA_BUILD_A124
    unsigned int str_hash = 0;
    bool mSkipable = true;

#ifdef EA_PLATFORM_WIN32
    // speed.exe 0x58F320: no chyron; wait for the loads and take EA Trax off the screen
    bAllowingControllerErrors = FEManager::Get()->IsAllowingControllerError();
    FEManager::Get()->AllowControllerError(false);
    while (TheCarLoader.IsLoadingInProgress() || TheTrackStreamer.IsLoadingInProgress() ||
           g_pEAXSound->AreResourceLoadsPending()) {
        MiniMainLoop();
    }
    if (cFEng::Get()->IsPackagePushed("EA_Trax.fng")) {
        cFEng::Get()->PopNoControlPackage("EA_Trax.fng");
    }
#else
    DismissChyron();
#endif
    FEngSetMovieName(GetPackageName(), 0x348ff9f, MovieFilename);

    if (eIsWidescreen()) {
        cFEng::Get()->QueuePackageMessage(0x70d2183b, GetPackageName(), 0);
    }
#ifdef EA_PLATFORM_PLAYSTATION2
    if (FEDatabase->GetVideoSettings()->WideScreen) {
        cFEng::Get()->QueuePackageMessage(0x70d2183b, GetPackageName(), 0);
    }
#endif

    CareerSettings *career = FEDatabase->GetCareerSettings();

    if (bStrCmp(MovieFilename, "drag_tutorial") == 0) {
        if (career && !career->HasDoneDragTutorial()) {
            mSkipable = false;
            career->SetHasDoneDragTutorial();
        }
        str_hash = FEngHashString("TUTORIAL_DRAG");
    } else if (bStrCmp(MovieFilename, "speedtrap_tutorial") == 0) {
        if (career && !career->HasDoneSpeedTrapTutorial()) {
            mSkipable = false;
            career->SetHasDoneSpeedTrapTutorial();
        }
        str_hash = FEngHashString("TUTORIAL_SPEEDTRAPRACE");
    } else if (bStrCmp(MovieFilename, "tollbooth_tutorial") == 0) {
        if (career && !career->HasDoneTollBoothTutorial()) {
            mSkipable = false;
            career->SetHasDoneTollBoothTutorial();
        }
        str_hash = FEngHashString("TUTORIAL_TOLLBOOTH");
    } else if (bStrCmp(MovieFilename, "bounty_tutorial") == 0) {
        if (career && !career->HasDoneBountyTutorial()) {
            mSkipable = false;
            career->SetHasDoneBountyTutorial();
        }
        str_hash = FEngHashString("TUTORIAL_BOUNTY");
    } else if (bStrCmp(MovieFilename, "pursuit_tutorial") == 0) {
        if (career && !career->HasDonePursuitTutorial()) {
            mSkipable = false;
            career->SetHasDonePursuitTutorial();
        }
        str_hash = FEngHashString("TUTORIAL_PURSUIT");
    }

    if (mSkipable) {
        cFEng::Get()->QueuePackageMessage(0x59291f95, GetPackageName(), 0);
    }

    unsigned int label_hash = bStringHash("_LABEL", str_hash);
    FEngSetLanguageHash(GetPackageName(), 0x5a0ee0d9, label_hash);
    FEngSetLanguageHash(GetPackageName(), 0xf414bf3e, label_hash);
    FEngSetLanguageHash(GetPackageName(), 0x5a0ee0d8, label_hash);
    FEngSetLanguageHash(GetPackageName(), 0x07d2ea5d, label_hash);

    mSubtitler.BeginningMovie(MovieFilename, GetPackageName());
#endif
    new EFadeScreenOff(0x14035fb);
}

MenuScreen *InGameAnyTutorialScreen::Create(ScreenConstructorData *sd) {
    return new ("", 0) InGameAnyTutorialScreen(sd);
}

void InGameAnyTutorialScreen::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
#ifndef EA_BUILD_A124
    mSubtitler.Update(msg);
#endif
    switch (msg) {
        case 0xc3960eb9:
            DismissMovie();
            break;
        case 0xb5af2461:
        case 0x406415e3:
            DismissMovie();
#ifndef EA_BUILD_A124
            mSubtitler.Update(0xc3960eb9);
#endif
            break;
    }
}

InGameAnyTutorialScreen::~InGameAnyTutorialScreen() {
#ifndef EA_PLATFORM_PLAYSTATION2
    gInGameMoviePlaying = false;
#endif
#ifdef EA_PLATFORM_WIN32
    FEManager::Get()->AllowControllerError(bAllowingControllerErrors);
#endif
}

void InGameAnyTutorialScreen::LaunchMovie(const char *filename
#ifndef EA_BUILD_A124
                                          ,
                                          const char *packageName
#endif
) {
#ifndef EA_PLATFORM_PLAYSTATION2
    gInGameMoviePlaying = true;
#endif
#ifndef EA_BUILD_A124
    PackageSet = false;
#endif
    SetMovieName(filename);
#ifndef EA_BUILD_A124
    if (packageName) {
        SetPackageName(packageName);
    }
#endif
    if (cFEng::Get()->IsPackageInControl(GetLoadingScreenPackageName())) {
        cFEng::Get()->QueuePackageSwitch(InGameTutorialScreenName, 0, 0, false);
    } else {
        cFEng::Get()->QueuePackagePush(InGameTutorialScreenName, 0, 0, false);
    }
}

void InGameAnyTutorialScreen::DismissMovie() {
#ifndef EA_PLATFORM_PLAYSTATION2
    gInGameMoviePlaying = false;
#endif
    MNotifyMovieFinished().Post(UCrc32(0x20d60dbf));
    cFEng::Get()->QueuePackagePop(0);
#ifndef EA_BUILD_A124
    cFEng::Get()->QueueGameMessage(0xc3960eb9, PackageFilename, 0xff);
#endif
    new EFadeScreenOn(
#ifndef EA_BUILD_A124
        false
#endif
    );
}

void InGameAnyTutorialScreen::SetMovieName(const char *filename) {
    bStrNCpy(MovieFilename, filename, 0x40);
}

void InGameAnyTutorialScreen::SetPackageName(const char *packageName) {
    PackageSet = true;
    bStrNCpy(PackageFilename, packageName, 0x40);
}
