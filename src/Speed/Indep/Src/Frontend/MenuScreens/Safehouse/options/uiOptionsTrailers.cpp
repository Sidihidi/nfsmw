#include "uiOptionsTrailers.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEButtons.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/FEPkg_GarageMain.hpp"
#include "Speed/Indep/Src/Generated/FEngHash/FEHash_UI_DebugCarCustomize.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEAnyMovieScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"

// Las opciones de la pantalla de trailers solo existen en PS2 (GameCube no las tiene en su DWARF de
// funciones, solo los tipos): la A124 y SLUS las anaden en Setup.
// total size: 0x9C
struct VideoIconOption : public IconOption {
    VideoIconOption(const char *movie_name, unsigned int tex_hash, unsigned int name_hash)
        : IconOption(tex_hash, name_hash, 0) {
        bStrNCpy(mMovieName, movie_name, 64);
        SetReactImmediately(true);
    }

    void React(const char *pkg_name, unsigned int data, FEObject *obj, unsigned int param1, unsigned int param2) override {
        if (data == 0x0C407210) {
            FEAnyMovieScreen::SetMovieName(mMovieName);
            cFEng::Get()->QueuePackagePush("FEAnyMovie.fng", 0, 0, false);
        }
    }

    char mMovieName[64]; // offset 0x5C, size 0x40
};

// total size: 0x60
struct DemoIconOption : public IconOption {
    DemoIconOption(void (*demo_function)(), unsigned int tex_hash, unsigned int name_hash)
        : IconOption(tex_hash, name_hash, 0) {
        mDemoFunction = demo_function;
    }

    void React(const char *pkg_name, unsigned int data, FEObject *obj, unsigned int param1, unsigned int param2) override {
        if (data == 0x0C407210) {
            DialogInterface::ShowTwoButtons(pkg_name, "", static_cast<eDialogTitle>(1), LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO, 0xD05FC3A3,
                                            0x34DC1BCF, 0x34DC1BCF, static_cast<eDialogFirstButtons>(1), 0x46C9CC39,
                                            GetLocalizedString(GetName()));
        } else if (data == 0xD05FC3A3) {
            mDemoFunction();
        }
    }

    void (*mDemoFunction)(); // offset 0x5C, size 0x4
};

UIOptionsTrailers::UIOptionsTrailers(ScreenConstructorData *sd) : IconScrollerMenu(sd) {
    Setup();
}

void UIOptionsTrailers::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    if (msg != 0x0C407210) {
        IconScrollerMenu::NotificationMessage(msg, pobj, param1, param2);
    }
    switch (msg) {
    case 0x911AB364:
        StorePrevNotification(0x911AB364, pobj, param1, param2);
        cFEng::Get()->QueuePackageMessage(0x587C018B, GetPackageName(), 0);
        break;
    case 0x0C407210:
        cFEng::Get()->QueuePackageMessage(__ENABLE_INPUTS__, 0, 0);
        Options.GetCurrentOption()->React(GetPackageName(), 0x0C407210, pobj, param1, param2);
        break;
    case 0xD05FC3A3:
        Options.GetCurrentOption()->React(GetPackageName(), 0xD05FC3A3, pobj, param1, param2);
        break;
    case 0xE1FDE1D1:
        if (PrevButtonMessage == 0x911AB364) {
#ifndef EA_BUILD_A124 // la A124 no limpia el modo de trailers
            FEDatabase->ClearGameMode(eFE_GAME_TRAILERS);
#endif
            FEDatabase->GetUserProfile(0)->GetOptions()->CurrentCategory = (eOptionsCategory)-1;
            cFEng::Get()->QueuePackageSwitch("MainMenu_Sub.fng", 0, 0, 0);
        }
        break;
    }
}

void UIOptionsTrailers::Setup() {
#ifdef EA_PLATFORM_PLAYSTATION2
    AddOption(new ("VideoIconOption", 0) VideoIconOption("xsell_ssx", 0x4CA59CA9, 0x570D68EB));
#ifdef EA_BUILD_A124
    AddOption(new ("DemoIconOption", 0) DemoIconOption(LaunchBondDemo, 0xDF2C8D58, 0xA9A10FBE));
    AddOption(new ("DemoIconOption", 0) DemoIconOption(LaunchMarvelDemo, 0xC8B1FDFC, 0xCA2F13E2));
#else
    // SLUS cambia la demo de Marvel por el trailer de Black y pone la de Bond al final.
    AddOption(new ("VideoIconOption", 0) VideoIconOption("xsell_black", 0x4583E5E8, 0xF010854A));
    AddOption(new ("DemoIconOption", 0) DemoIconOption(LaunchBondDemo, 0xDF2C8D58, 0xA9A10FBE));
#endif
#endif
    SetInitialOption(FEngGetLastButton(GetPackageName()));
#ifndef EA_BUILD_A124
    // La A124 no cancela el empuje de camara: su Setup va de SetInitialOption a FEngSetLanguageHash.
    GarageMainScreen::GetInstance()->CancelCameraPush();
#endif
    FEngSetLanguageHash(GetPackageName(), 0xB71B576D, 0xB65A46D8);
    RefreshHeader();
}
