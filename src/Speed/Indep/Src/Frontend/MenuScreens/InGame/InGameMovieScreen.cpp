
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEMovies.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_Chyron.hpp"
#include "Speed/Indep/Src/World/CarLoader.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/Src/Frontend/SubTitle.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOff.hpp"
#include "Speed/Indep/Src/Generated/Messages/MNotifyMovieFinished.h"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOn.hpp"
#include "Speed/Indep/Src/Frontend/MoviePlayer/MoviePlayer.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/InGameMovieScreen.hpp"
#include "Speed/Indep/Src/World/TrackStreamer.hpp"
#include "Speed/Indep/Src/Misc/Main.hpp"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Frontend/FEPackageData.hpp"

static bool gInGameMoviePlaying = false;
char InGameAnyMovieScreen::MovieFilename[64] = "unknown";
#ifdef EA_BUILD_A124
static const char *InGameMovieScreenName = "InGameAnyMovie.fng";
#endif

InGameAnyMovieScreen::InGameAnyMovieScreen(ScreenConstructorData *sd) : MenuScreen(sd) {
#ifndef EA_BUILD_A124
    bAllowingControllerErrors = FEManager::Get()->IsAllowingControllerError();
    FEManager::Get()->AllowControllerError(false);
    while (TheCarLoader.IsLoadingInProgress() || TheTrackStreamer.IsLoadingInProgress() || g_pEAXSound->AreResourceLoadsPending()) {
        MiniMainLoop();
    }
    DismissChyron();
#endif
    FEngSetMovieName(GetPackageName(), 0x348ff9f, MovieFilename);
    mSubtitler.BeginningMovie(MovieFilename, GetPackageName());
    new EFadeScreenOff(0x14035fb);
}

InGameAnyMovieScreen::~InGameAnyMovieScreen() {
#ifndef EA_BUILD_A124
    FEManager::Get()->AllowControllerError(bAllowingControllerErrors);
#endif
#ifndef EA_PLATFORM_PLAYSTATION2
    gInGameMoviePlaying = false;
#endif
}

MenuScreen *InGameAnyMovieScreen::Create(ScreenConstructorData *sd) {
    return new ("InGameAnyMovieScreen", 0) InGameAnyMovieScreen(sd);
}

void InGameAnyMovieScreen::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    mSubtitler.Update(msg);
    switch (msg) {
        case 0xc3960eb9:
            DismissMovie();
            break;
        case 0xb5af2461:
        case 0x406415e3:
#ifndef EA_BUILD_A124
            if (!FEDatabase->IsDDay() || SkipMovies || MoviePlayer_Bypass())
#endif
            {
                mSubtitler.Update(0xc3960eb9);
                DismissMovie();
            }
            break;
    }
}

bool InGameAnyMovieScreen::IsPlaying() {
    return gInGameMoviePlaying;
}

void InGameAnyMovieScreen::LaunchMovie(const char *filename) {
    InGameAnyMovieScreen::SetMovieName(filename);
#ifndef EA_PLATFORM_PLAYSTATION2
    gInGameMoviePlaying = true;
#endif
    if (cFEng::Get()->IsPackageInControl(GetLoadingScreenPackageName())) {
#ifdef EA_BUILD_A124
        cFEng::Get()->QueuePackageSwitch(InGameMovieScreenName, 0, 0, false);
#else
        cFEng::Get()->QueuePackageSwitch(GetFEngPackageName(), 0, 0, false);
#endif
    } else {
#ifdef EA_BUILD_A124
        cFEng::Get()->QueuePackagePush(InGameMovieScreenName, 0, 0, false);
#else
        cFEng::Get()->QueuePackagePush(GetFEngPackageName(), 0, 0, false);
#endif
    }
}

void InGameAnyMovieScreen::DismissMovie() {
#ifndef EA_PLATFORM_PLAYSTATION2
    gInGameMoviePlaying = false;
#endif
    MNotifyMovieFinished().Post(UCrc32(0x20d60dbf));
    cFEng::Get()->QueuePackagePop(0);
    new EFadeScreenOn(
#ifndef EA_BUILD_A124
        false
#endif
    );
}

void InGameAnyMovieScreen::SetMovieName(const char *filename) {
    bStrNCpy(MovieFilename, filename, 0x40);
}

const char *InGameAnyMovieScreen::GetFEngPackageName() {
    bool ps2_widescreen = eIsWidescreen();
    if (!ps2_widescreen) {
        return "InGameAnyMovie.fng";
    }
    return "WS_InGameAnyMovie.fng";
}
