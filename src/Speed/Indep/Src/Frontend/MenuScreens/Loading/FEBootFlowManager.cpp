#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FEBootFlowManager.hpp"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Misc/BuildRegion.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FECarViewer.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEJoyInput.hpp"

#ifdef EA_PLATFORM_PLAYSTATION2
// Las dos PS2 (stabs de la A124: sBootFlowNTSC[7], sBootFlowPAL[8], sBootFlowWideScreen[1]): sin los paquetes _GC, el
// selector de idioma al principio del flujo PAL y en panoramico solo el menu principal.
static char *sBootFlowNTSC[] = {"MC_Bootup.fng",    "LS_EAlogo.fng", "LS_PSA.fng",  "MW_LS_AttractFMV.fng",
                                "MW_LS_Splash.fng", "MC_Main.fng",   "MainMenu.fng"};
static char *sBootFlowPAL[] = {"LS_LangSelect.fng",    "MC_Bootup.fng",    "LS_EAlogo.fng", "LS_PSA.fng",
                               "MW_LS_AttractFMV.fng", "MW_LS_Splash.fng", "MC_Main.fng",   "MainMenu.fng"};
static char *sBootFlowWideScreen[] = {"MainMenu.fng"};
static char *sBootFlowPALWidescreen[] = {"MainMenu.fng"};
#elif defined(EA_PLATFORM_WIN32)
// PC (speed.exe 1.3, 0x8F3C58-0x8F3CCF): los paquetes de PC sin _GC, y todas menos la NTSC empiezan en DiscErrorPC.
const char *sBootFlowNTSC[] = {"MC_Bootup.fng",    "LS_EAlogo.fng", "LS_PSA.fng",  "MW_LS_AttractFMV.fng",
                               "MW_LS_Splash.fng", "MC_Main.fng",   "MainMenu.fng"};
const char *sBootFlowPAL[] = {"DiscErrorPC.fng",      "MC_Bootup.fng",    "LS_EAlogo.fng", "LS_PSA.fng",
                              "MW_LS_AttractFMV.fng", "MW_LS_Splash.fng", "MC_Main.fng",   "MainMenu.fng"};
const char *sBootFlowWideScreen[] = {"DiscErrorPC.fng",  "MC_Bootup.fng", "LS_EAlogo.fng", "LS_PSA.fng",
                                     "MW_LS_Splash.fng", "MC_Main.fng",   "MainMenu.fng"};
const char *sBootFlowPALWidescreen[] = {"DiscErrorPC.fng",      "MC_Bootup.fng",    "LS_EAlogo.fng", "LS_PSA.fng",
                                        "MW_LS_AttractFMV.fng", "MW_LS_Splash.fng", "MC_Main.fng",   "MainMenu.fng"};
#else
const char *sBootFlowNTSC[] = {"MC_Bootup_GC.fng", "LS_EAlogo.fng",  "LS_PSA.fng",  "MW_LS_AttractFMV.fng",
                               "MW_LS_Splash.fng", "MC_Main_GC.fng", "MainMenu.fng"};
const char *sBootFlowPAL[] = {"MC_Bootup_GC.fng", "LS_EAlogo.fng",  "LS_PSA.fng",  "MW_LS_AttractFMV.fng",
                              "MW_LS_Splash.fng", "MC_Main_GC.fng", "MainMenu.fng"};
const char *sBootFlowWideScreen[] = {"MC_Bootup_GC.fng", "LS_EAlogo.fng",  "LS_PSA.fng",  "MW_LS_AttractFMV.fng",
                                     "MW_LS_Splash.fng", "MC_Main_GC.fng", "MainMenu.fng"};
const char *sBootFlowPALWidescreen[] = {"MC_Bootup_GC.fng", "LS_EAlogo.fng",  "LS_PSA.fng",  "MW_LS_AttractFMV.fng",
                                        "MW_LS_Splash.fng", "MC_Main_GC.fng", "MainMenu.fng"};
#endif

#ifdef EA_PLATFORM_WIN32
// La comprobacion del disco de PC (speed.exe 0x6C2700, zPlatform: GetLogicalDrives/GetDriveTypeA). El nombre es nuestro.
bool IsDiscInDrive();
#endif

BootFlowManager *BootFlowManager::mInstance = nullptr;

void BootFlowManager::Init() {
    if (mInstance == nullptr) {
        mInstance = new ("BootFlowManager", 0) BootFlowManager();
    }
}

void BootFlowManager::Destroy() {
    if (mInstance != nullptr) {
        delete mInstance;
        mInstance = nullptr;
        CarViewer::ShowAllCars();
    }
    g_pEAXSound->PlayFEMusic(-1);
}

BootFlowManager *BootFlowManager::Get() {
    return mInstance;
}

#ifdef EA_BUILD_A124 // la A124 no tiene la lista PAL panoramica y arranca en ControllerUnplugged sin mandos
BootFlowManager::BootFlowManager() {
    if (!BuildRegion::IsPal()) {
        if (eIsWidescreen()) {
            for (int i = 0; i < NUM_ELEMENTS(sBootFlowWideScreen); i++) {
                if (*sBootFlowWideScreen[i] != '\0') {
                    BootFlowScreens.AddTail(new ("BootFlowScreen", 0) BootFlowScreen(sBootFlowWideScreen[i]));
                }
            }
        } else {
            for (int i = 0; i < NUM_ELEMENTS(sBootFlowNTSC); i++) {
                if (*sBootFlowNTSC[i] != '\0') {
                    BootFlowScreens.AddTail(new ("BootFlowScreen", 0) BootFlowScreen(sBootFlowNTSC[i]));
                }
            }
        }
    } else {
        for (int i = 0; i < NUM_ELEMENTS(sBootFlowPAL); i++) {
            bool add_screen = true;
            if (bStrICmp(sBootFlowPAL[i], "LS_LangSelect.fng") == 0 && !BuildRegion::ShowLanguageSelect()) {
                add_screen = false;
            }
            if (add_screen && *sBootFlowPAL[i] != '\0') {
                BootFlowScreens.AddTail(new ("BootFlowScreen", 0) BootFlowScreen(sBootFlowPAL[i]));
            }
        }
    }
    CurrentScreen = BootFlowScreens.GetHead();
    if (!cFEngJoyInput::Get()->IsJoyPluggedIn(JOYSTICK_PORT1) && !cFEngJoyInput::Get()->IsJoyPluggedIn(JOYSTICK_PORT2)) {
        FEManager::Get()->SetFirstScreen("ControllerUnplugged.fng", -1, 0);
    } else {
        FEManager::Get()->SetFirstScreen(CurrentScreen->Name, 0, 0);
    }
}
#else
BootFlowManager::BootFlowManager() {
    if (!BuildRegion::IsPal()) {
        if (eIsWidescreen()) {
            for (int i = 0; i < NUM_ELEMENTS(sBootFlowWideScreen); i++) {
                bool add_screen = true;
                if (bStrICmp(sBootFlowWideScreen[i], "WS_LS_EA_hidef.fng") == 0) {
                }
                if (add_screen && *sBootFlowWideScreen[i] != '\0') {
                    BootFlowScreens.AddTail(new ("BootFlowScreen", 0) BootFlowScreen(sBootFlowWideScreen[i]));
                }
            }
        } else {
            for (int i = 0; i < NUM_ELEMENTS(sBootFlowNTSC); i++) {
                if (*sBootFlowNTSC[i] != '\0') {
                    BootFlowScreens.AddTail(new ("BootFlowScreen", 0) BootFlowScreen(sBootFlowNTSC[i]));
                }
            }
        }
    } else {
        if (eIsWidescreen()) {
            for (int i = 0; i < NUM_ELEMENTS(sBootFlowPALWidescreen); i++) {
                bool add_screen = true;
                if (bStrICmp(sBootFlowPALWidescreen[i], "LS_LangSelect.fng") == 0 && !BuildRegion::ShowLanguageSelect()) {
                    add_screen = false;
                }
                if (add_screen && *sBootFlowPALWidescreen[i] != '\0') {
                    BootFlowScreens.AddTail(new ("BootFlowScreen", 0) BootFlowScreen(sBootFlowPALWidescreen[i]));
                }
            }
        } else {
            for (int i = 0; i < NUM_ELEMENTS(sBootFlowPAL); i++) {
                bool add_screen = true;
                if (bStrICmp(sBootFlowPAL[i], "LS_LangSelect.fng") == 0 && !BuildRegion::ShowLanguageSelect()) {
                    add_screen = false;
                }
                if (add_screen && *sBootFlowPAL[i] != '\0') {
                    BootFlowScreens.AddTail(new ("BootFlowScreen", 0) BootFlowScreen(sBootFlowPAL[i]));
                }
            }
        }
    }
    CurrentScreen = BootFlowScreens.GetHead();
#ifdef EA_PLATFORM_WIN32 // PC salta DiscErrorPC si el disco esta en la unidad (o existe "foobar")
    if (bStrICmp("DiscErrorPC.fng", CurrentScreen->Name) == 0) {
        bool no_disc = !IsDiscInDrive();
        if (bFileExists("foobar") || !no_disc) {
            CurrentScreen = CurrentScreen->GetNext();
        }
    }
#endif
    FEManager::Get()->SetFirstScreen(CurrentScreen->Name, 0, 0);
}
#endif

BootFlowScreen *BootFlowManager::FindScreen(const char *name) {
    {
        BootFlowScreen *s = BootFlowScreens.GetHead();
        while (s != BootFlowScreens.EndOfList()) {
            if (bStrICmp(s->Name, name) == 0) {
                return s;
            }
            s = s->GetNext();
        }
    }
    return nullptr;
}

BootFlowScreen *BootFlowManager::FindScreenSubStr(const char *name) {
    {
        BootFlowScreen *s = BootFlowScreens.GetHead();
        while (s != BootFlowScreens.EndOfList()) {
            if (bStrStr(s->Name, name) != nullptr) {
                return s;
            }
            s = s->GetNext();
        }
    }
    return nullptr;
}

void BootFlowManager::JumpToHead() {
    CurrentScreen = BootFlowScreens.GetHead();
    JumpToScreen(CurrentScreen->Name);
}

bool BootFlowManager::JumpToScreen(const char *screen_name) {
    BootFlowScreen *screen = FindScreen(screen_name);
    if (screen == nullptr) {
        return false;
    }
    CurrentScreen = screen;
#ifdef EA_BUILD_A124
    cFEng::Get()->QueuePackageSwitch(CurrentScreen->Name, 0, 0, false);
#else
    cFEng::Get()->QueuePackagePop(0);
    cFEng::Get()->QueuePackagePush(CurrentScreen->Name, 0, 0, false);
#endif
    if (BootFlowScreens.GetTail() == CurrentScreen) {
        Destroy();
    }
    return true;
}

bool BootFlowManager::DoAttract() {
    BootFlowScreen *new_screen = FindScreenSubStr("AttractFMV");
    if (new_screen == nullptr) {
        return false;
    }
    return JumpToScreen(new_screen->Name);
}

void BootFlowManager::ChangeToNextBootFlowScreen(int mask) {
    CurrentScreen = CurrentScreen->GetNext();
    cFEng::Get()->QueuePackageSwitch(CurrentScreen->Name, 0, mask, false);
    if (BootFlowScreens.GetTail() == CurrentScreen) {
        Destroy();
    }
}
