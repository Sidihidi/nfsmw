#include "uiProfileManager.hpp"

#include "Speed/Indep/Src/FEng/FEList.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"

void FEngSetColor(FEObject* obj, unsigned int color);
unsigned char FEngGetLastButton(const char* pkg_name);
// [tr] const char* GetLocalizedString(unsigned int hash);

#ifdef EA_BUILD_A124
void PMPopDelete::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) {
    if (data == 0x0C407210) {
        FEDatabase->SetPlayersJoystickPort(0, FEngMapJoyParamToJoyport(param1));
        cFEng::Get()->QueuePackageSwitch("MC_DeleteProfile.fng", 0, 0, false);
    }
}

void PMSave::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) {
    if (data == 0x0C407210) {
        FEDatabase->SetPlayersJoystickPort(0, FEngMapJoyParamToJoyport(param1));
        MemcardEnter(pkg_name, pkg_name, 0x2251, 0, 0, 0, 0);
    }
}

void PMLoad::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) {
    if (data == 0x0C407210) {
        FEDatabase->SetPlayersJoystickPort(0, FEngMapJoyParamToJoyport(param1));
        MemcardEnter(pkg_name, pkg_name, 0x411, 0, 0, 0x3A2BE557, 0x8867412D);
    }
}

void PMDelete::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) {
    if (data == 0x0C407210) {
        FEDatabase->SetPlayersJoystickPort(0, FEngMapJoyParamToJoyport(param1));
        MemcardEnter(pkg_name, pkg_name, 0x31, 0, 0, 0, 0);
    }
}

void PMCreateNew::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1, unsigned int param2) {
    if (data == 0x0C407210) {
        FEDatabase->SetPlayersJoystickPort(0, FEngMapJoyParamToJoyport(param1));
        MemcardEnter(pkg_name, pkg_name, 0x61, 0, 0, 0, 0);
    }
}

#endif
MenuScreen* CreateUIProfileManager(ScreenConstructorData* sd) {
    return new ("CreateUIProfileManager", 0) UIProfileManager(sd);
}

UIProfileManager::UIProfileManager(ScreenConstructorData* sd)
    : IconScrollerMenu(sd) {
    Setup();
    FEPrintf(GetPackageName(), 0x42ADB44C, GetLocalizedString(0xBCB18F38));
}

void UIProfileManager::Refresh() {
    if (FEDatabase->bProfileLoaded) {
        mpSave->IsGreyOut = false;
    } else {
        mpSave->IsGreyOut = true;
    }

    if (FEDatabase->bProfileLoaded) {
#ifndef EA_BUILD_A124
        FEngSetVisible(GetPackageName(), FEHashUpper("NAME_GROUP"));
#endif
        FEPrintf(GetPackageName(), 0xEB406FEC, FEDatabase->GetUserProfile(0)->GetProfileName());
    } else {
        FEngSetInvisible(GetPackageName(), FEHashUpper("NAME_GROUP"));
    }

    FEngSetColor(mpSave->FEngObject, mpSave->OriginalColor);
    RefreshHeader();
}

void UIProfileManager::NotificationMessage(u32 msg, FEObject* obj, u32 param1,
                                           u32 param2) {
    IconScrollerMenu::NotificationMessage(msg, obj, param1, param2);

    switch (msg) {
    case 0x911AB364:
        cFEng::Get()->QueuePackageSwitch("MainMenu.fng", 0, 0, false);
        break;
    case 0x35F8620B:
        Refresh();
        break;
    case 0x7E998E5E:
        FEDatabase->RefreshCurrentRide();
        Refresh();
        break;
    }
}

#ifdef EA_PLATFORM_PLAYSTATION2
// PS2: sin crear perfil nuevo, el borrado en una ventana aparte y sin el fundido de iconos.
void UIProfileManager::Setup() {
    mpSave = new ("PMSave", 0) PMSave(0x228B7E32, 0x1C8ACE, 0);
    mpSave->SetReactImmediately(true);

    PMPopDelete* popDelete = new ("PMPopDelete", 0) PMPopDelete(0x6B303856, 0xE6F55DF0, 0);
    popDelete->SetReactImmediately(true);
    AddOption(popDelete);

    PMLoad* load = new ("PMLoad", 0) PMLoad(0x2287E063, 0x18ECFF, 0);
    load->SetReactImmediately(true);
    AddOption(load);

    AddOption(mpSave);

    SetInitialOption(FEngGetLastButton(GetPackageName()));
    Refresh();
}
#else
void UIProfileManager::Setup() {
    mpSave = new ("PMSave", 0) PMSave(0x228B7E32, 0x1C8ACE, 0);
    mpSave->SetReactImmediately(true);

    PMCreateNew* createNew = new ("PMCreateNew", 0) PMCreateNew(0x43798644, 0x55423473, 0);
    createNew->SetReactImmediately(true);
    AddOption(createNew);

    PMLoad* load = new ("PMLoad", 0) PMLoad(0x2287E063, 0x18ECFF, 0);
    load->SetReactImmediately(true);
    AddOption(load);

    AddOption(mpSave);

    PMDelete* del = new ("PMDelete", 0) PMDelete(0x0D9035CE, 0x56B00632, 0);
    del->SetReactImmediately(true);
    AddOption(del);

    int lastButton = FEngGetLastButton(GetPackageName());
    if (bFadeInIconsImmediately) {
        Options.bDelayUpdate = false;
        Options.bFadingOut = false;
        Options.StartFadeIn();
    }
    Options.SetInitialPos(lastButton);

    Refresh();
}
#endif

MenuScreen* CreateUIDeleteProfile(ScreenConstructorData* sd) {
    return new ("CreateUIDeleteProfile", 0) UIDeleteProfile(sd);
}

UIDeleteProfile::UIDeleteProfile(ScreenConstructorData* sd)
    : IconScrollerMenu(sd) {
    Setup();
    FEPrintf(GetPackageName(), 0x42ADB44C, GetLocalizedString(0xE6F55DF0));
}

void UIDeleteProfile::Setup() {
    PMCreateNew* createNew = new ("PMCreateNew", 0) PMCreateNew(0x43798644, 0x55423473, 0);
    createNew->SetReactImmediately(true);
    AddOption(createNew);

#ifdef EA_BUILD_A124
    PMDelete* del = new ("PMDelete", 0) PMDelete(0x0D9035CE, 0x56B00632, 0);
#else
    PMDelete* del = new ("PMDelete", 0) PMDelete(0x0D9035CE, 0x9F014666, 0);
#endif
    del->SetReactImmediately(true);
    AddOption(del);

    int lastButton = FEngGetLastButton(GetPackageName());
    SetInitialOption(lastButton);

#ifdef EA_BUILD_A124
    RefreshHeader();
}
#else
    Refresh();
}

void UIDeleteProfile::Refresh() {
    if (FEDatabase->bProfileLoaded) {
        FEngSetVisible(GetPackageName(), FEHashUpper("NAME_GROUP"));
        FEPrintf(GetPackageName(), 0xEB406FEC,
                 FEDatabase->GetUserProfile(0)->GetProfileName());
    } else {
        FEngSetInvisible(GetPackageName(), FEHashUpper("NAME_GROUP"));
    }

    RefreshHeader();
    FEDatabase->RefreshCurrentRide();
}
#endif

void UIDeleteProfile::NotificationMessage(u32 msg, FEObject* obj, u32 param1,
                                          u32 param2) {
    IconScrollerMenu::NotificationMessage(msg, obj, param1, param2);

    switch (msg) {
    case 0x911AB364:
        cFEng::Get()->QueuePackageSwitch("MC_ProfileManager.fng", 0, 0, false);
        break;
#ifdef EA_BUILD_A124
    case 0x35F8620B:
        if (FEDatabase->bProfileLoaded) {
            FEPrintf(GetPackageName(), 0xEB406FEC, FEDatabase->GetUserProfile(0)->GetProfileName());
        } else {
            FEngSetInvisible(GetPackageName(), FEHashUpper("NAME_GROUP"));
        }
        break;
#else
    case 0x7E998E5E:
        Refresh();
        break;
#endif
    }
}
