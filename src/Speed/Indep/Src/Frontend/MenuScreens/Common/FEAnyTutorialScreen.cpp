#include "FEAnyTutorialScreen.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEMovies.hpp"
#include "Speed/Indep/Src/FEng/FEMovie.h"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOff.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"

struct FEMovie;
extern void DismissChyron();
// r75: fuera 2 extern duplicados (FEngHashString, bStringHash): ya los declara
// FEngInterfaceFEStrings.hpp (via FEDatabase.hpp) y Strings.hpp (include propio).

// [fe3] FEngSetMovieName(pkg,hash,name) ya es inline en FEngInterfaces/FEngInterfaceFEMovies.hpp

char FEAnyTutorialScreen::MovieFilename[64] = "unknown";
char FEAnyTutorialScreen::PackageFilename[64] = "unknown";
bool FEAnyTutorialScreen::PackageSet = false;

static const char *FEAnyTutorialScreenName = "FEAnyTutorial.fng";

#ifdef EA_BUILD_A124
// La A124 no lleva subtitulos ni marca los tutoriales vistos: escribe las lineas del texto (LINE01..) y sus vinetas.
FEAnyTutorialScreen::FEAnyTutorialScreen(ScreenConstructorData *sd)
    : MenuScreen(sd) {
    unsigned int str_hash = 0;
    int num_tutorial_strings = 4;

    for (int i = 1; i <= num_tutorial_strings; i++) {
        FEPrintf(GetPackageName(), FEngHashString("TUTORIAL_TEXT_%d", i), "");
        FEngSetScript(GetPackageName(), FEngHashString("BULLET_%d", i), 0x16A259, true);
    }

    FEngSetMovieName(GetPackageName(), 0x0348FF9F, MovieFilename);

    if (bStrCmp(MovieFilename, "drag_tutorial") == 0) {
        str_hash = FEngHashString("TUTORIAL_DRAG_");
    } else if (bStrCmp(MovieFilename, "speedtrap_tutorial") == 0) {
        str_hash = FEngHashString("TUTORIAL_SPEEDTRAPRACE_");
    } else if (bStrCmp(MovieFilename, "tollbooth_tutorial") == 0) {
        num_tutorial_strings = 3;
        str_hash = FEngHashString("TUTORIAL_TOLLBOOTH_");
    } else if (bStrCmp(MovieFilename, "bounty_tutorial") == 0) {
        str_hash = FEngHashString("TUTORIAL_BOUNTY_");
    } else if (bStrCmp(MovieFilename, "pursuit_tutorial") == 0) {
        str_hash = FEngHashString("TUTORIAL_PURSUIT_");
    }

    unsigned int label_hash = bStringHash("LABEL", str_hash);
    FEngSetLanguageHash(GetPackageName(), 0x5A0EE0D9, label_hash);
    FEngSetLanguageHash(GetPackageName(), 0xF414BF3E, label_hash);
    FEngSetLanguageHash(GetPackageName(), 0x5A0EE0D8, label_hash);
    FEngSetLanguageHash(GetPackageName(), 0x07D2EA5D, label_hash);

    for (int i = 1; i <= num_tutorial_strings; i++) {
        char num_str[8];
        bSPrintf(num_str, "LINE%2.2d", i);
        unsigned int text_hash = bStringHash(num_str, str_hash);
        FEngSetLanguageHash(GetPackageName(), FEngHashString("TUTORIAL_TEXT_%d", i), text_hash);
        FEngSetScript(GetPackageName(), FEngHashString("BULLET_%d", i), 0x5079C8F8, true);
    }

    new EFadeScreenOff(0x14035FB);
}
#else
FEAnyTutorialScreen::FEAnyTutorialScreen(ScreenConstructorData *sd)
    : MenuScreen(sd)
#ifndef EA_BUILD_A124
      ,
      mTimer(0)
#endif
{
    unsigned int str_hash = 0;
    bool mSkipable = true;

    DismissChyron();

    FEngSetMovieName(GetPackageName(), 0x0348FF9F, MovieFilename);

    if (eIsWidescreen()) {
        cFEng::Get()->QueuePackageMessage(0x70D2183B, GetPackageName(), nullptr);
    }

    CareerSettings *career = FEDatabase->GetCareerSettings();

    if (bStrCmp(MovieFilename, "drag_tutorial") == 0) {
        if (career != nullptr && !career->HasDoneDragTutorial()) {
            mSkipable = false;
            career->SetHasDoneDragTutorial();
        }
        str_hash = FEngHashString("TUTORIAL_DRAG");
    } else if (bStrCmp(MovieFilename, "speedtrap_tutorial") == 0) {
        if (career != nullptr && !career->HasDoneSpeedTrapTutorial()) {
            mSkipable = false;
            career->SetHasDoneSpeedTrapTutorial();
        }
        str_hash = FEngHashString("TUTORIAL_SPEEDTRAPRACE");
    } else if (bStrCmp(MovieFilename, "tollbooth_tutorial") == 0) {
        if (career != nullptr && !career->HasDoneTollBoothTutorial()) {
            mSkipable = false;
            career->SetHasDoneTollBoothTutorial();
        }
        str_hash = FEngHashString("TUTORIAL_TOLLBOOTH");
    } else if (bStrCmp(MovieFilename, "bounty_tutorial") == 0) {
        if (career != nullptr && !career->HasDoneBountyTutorial()) {
            mSkipable = false;
            career->SetHasDoneBountyTutorial();
        }
        str_hash = FEngHashString("TUTORIAL_BOUNTY");
    } else if (bStrCmp(MovieFilename, "pursuit_tutorial") == 0) {
        if (career != nullptr && !career->HasDonePursuitTutorial()) {
            mSkipable = false;
            career->SetHasDonePursuitTutorial();
        }
        str_hash = FEngHashString("TUTORIAL_PURSUIT");
    }

    if (mSkipable) {
        cFEng::Get()->QueuePackageMessage(0x59291F95, GetPackageName(), nullptr);
    }

    unsigned int label_hash = bStringHash("_LABEL", str_hash);
    FEngSetLanguageHash(GetPackageName(), 0x5A0EE0D9, label_hash);
    FEngSetLanguageHash(GetPackageName(), 0xF414BF3E, label_hash);
    FEngSetLanguageHash(GetPackageName(), 0x5A0EE0D8, label_hash);
    FEngSetLanguageHash(GetPackageName(), 0x07D2EA5D, label_hash);

#ifndef EA_BUILD_A124
    mSubtitler.BeginningMovie(MovieFilename, GetPackageName());
#endif

    new EFadeScreenOff(0x14035FB);
}
#endif

MenuScreen *FEAnyTutorialScreen::Create(ScreenConstructorData *sd) {
    return new ("FEAnyTutorialScreen", 0) FEAnyTutorialScreen(sd);
}

FEAnyTutorialScreen::~FEAnyTutorialScreen() {
#ifndef EA_BUILD_A124
    FEManager::Get()->SetEATraxSecondButton();
#endif
}

void FEAnyTutorialScreen::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
#ifndef EA_BUILD_A124
    mSubtitler.Update(msg);
#endif

    switch (msg) {
        case 0xC3960EB9:
            DismissMovie(
#ifndef EA_BUILD_A124
                false
#endif
            );
            break;
        case 0xB5AF2461:
        case 0x406415E3:
            DismissMovie(
#ifndef EA_BUILD_A124
                true
#endif
            );
#ifndef EA_BUILD_A124
            mSubtitler.Update(0xC3960EB9);
#endif
            break;
    }
}

void FEAnyTutorialScreen::LaunchMovie(const char *filename
#ifndef EA_BUILD_A124
                                      ,
                                      const char *packageName
#endif
) {
#ifndef EA_BUILD_A124
    PackageSet = false;
#endif
    SetMovieName(filename);
#ifndef EA_BUILD_A124
    if (packageName != nullptr) {
        SetPackageName(packageName);
    }
#endif
    cFEng::Get()->QueuePackagePush(FEAnyTutorialScreenName, 0, 0, false);
}

void FEAnyTutorialScreen::DismissMovie(
#ifndef EA_BUILD_A124
    bool send_message
#endif
) {
    cFEng::Get()->QueuePackagePop(1);
#ifndef EA_BUILD_A124
    if (send_message) {
        cFEng::Get()->QueueGameMessage(0xC3960EB9, PackageFilename, 0xFF);
    }
#endif
}

void FEAnyTutorialScreen::SetMovieName(const char *filename) {
    bStrNCpy(MovieFilename, filename, 64);
}

void FEAnyTutorialScreen::SetPackageName(const char *packageName) {
    PackageSet = true;
    bStrNCpy(PackageFilename, packageName, 64);
}
