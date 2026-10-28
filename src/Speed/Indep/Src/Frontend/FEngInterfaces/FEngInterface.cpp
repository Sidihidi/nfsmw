#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/FEng/FEGameInterface.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEGameInterface.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEJoyInput.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEObjectCallbacks.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#include "Speed/Indep/Src/Frontend/HUD/FEPkg_Hud.hpp"
#include "Speed/Indep/Src/Interfaces/SimActivities/INIS.h"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#ifdef MILESTONE_BUILD
#include "Speed/Indep/Src/Misc/TestHooks/JuiceHooks/JuiceHooks.h"
#endif

Timer MessengerCreationTimer(0);

cFEng *cFEng::mInstance = nullptr;

void cFEng::Init() {
    if (mInstance == nullptr) {
        mInstance = new ("cFEng", 0) cFEng();
        MessengerCreationTimer.ResetLow();
    }
}

cFEng::cFEng() {
    bWasPaused = false;
    mFEng = new ("FEngine", 0) FEngine();
    mFEng->SetInterface(cFEngGameInterface::pInstance);
#ifdef EA_PLATFORM_XENON
    mFEng->SetNumJoyPads(4); // X360: cuatro mandos (NFS.exe 0x8229CD78)
#else
    mFEng->SetNumJoyPads(2);
#endif
    mFEng->SetInitialState();
    mFEng->SetExecution(true);
}

void cFEng::PushErrorPackage(const char *pPackageName, int pArg, u32 ControlMask) {
// PC (0x542530) solo tiene la rama de abajo: ni la prueba de FEDatabase ni la de IsPackagePushed.
#if !defined(EA_BUILD_A124) && !defined(EA_PLATFORM_WIN32)
    if (FEDatabase == nullptr) {
#endif
#ifndef EA_PLATFORM_WIN32
        if (cFEng::Get()->IsPackagePushed(pPackageName)) {
            return;
        }
#endif
        FEPackageManager::Get()->SetPackageDataArg(pPackageName, pArg);
        FEPackage *pPackage = mFEng->PushPackage(pPackageName, FE_PACKAGE_PRIORITY_ERROR, ControlMask);
        pPackage->SetErrorScreen(true);
        mFEng->ToggleErrorScreenMode(true);
        if (FEManager::IsPaused() && !bWasPaused) {
            bWasPaused = false;
        } else {
            bWasPaused = true;
            FEManager::RequestPauseSimulation(pPackageName);
            PauseAllSystems();
        }
#if !defined(EA_BUILD_A124) && !defined(EA_PLATFORM_WIN32)
    } else {
        FEPackageManager::Get()->SetPackageDataArg(pPackageName, pArg);
        FEPackage *pPackage = mFEng->PushPackage(pPackageName, FE_PACKAGE_PRIORITY_ERROR, ControlMask);
        pPackage->SetErrorScreen(true);
        mFEng->ToggleErrorScreenMode(true);
        if (FEManager::IsPaused() && !bWasPaused) {
            bWasPaused = false;
        } else {
            bWasPaused = true;
            FEManager::RequestPauseSimulation(pPackageName);
            PauseAllSystems();
        }
    }
#endif
}

void cFEng::PopErrorPackage() {
#ifdef EA_PLATFORM_PLAYSTATION2
    mFEng->QueuePackagePop();
    mFEng->ToggleErrorScreenMode(false);
    if (bWasPaused) {
        ResumeAllSystems(true);
#ifdef EA_BUILD_A124
        FEManager::RequestUnPauseSimulation(nullptr);
#else
        if (FEManager::IsPaused()) {
            FEManager::RequestUnPauseSimulation(nullptr);
        }
#endif
    }
#else
    if (FEDatabase == nullptr) {
        mFEng->QueuePackagePop();
        mFEng->ToggleErrorScreenMode(false);
        ServiceFengOnly();
        if (bWasPaused) {
            ResumeAllSystems(false);
            FEManager::RequestUnPauseSimulation(nullptr);
        }
    } else {
        mFEng->QueuePackagePop();
        mFEng->ToggleErrorScreenMode(false);
        ServiceFengOnly();
        if (bWasPaused) {
            ResumeAllSystems(true);
            if (FEManager::IsPaused()) {
                FEManager::RequestUnPauseSimulation(nullptr);
            }
        }
    }
#endif
}

void cFEng::PopErrorPackage(int port) {
    mFEng->QueuePackagePop();
    mFEng->ToggleErrorScreenMode(false);
    FEManager *feManager = FEManager::Get();
#ifdef EA_BUILD_A124
    feManager->ClearControllerError(port);
#else
    if (port != -1) {
        feManager->ClearControllerError(port);
    }
#endif
    if (bWasPaused) {
        if (!feManager->WaitingForControllerError()) {
            ResumeAllSystems(true);
        }
        FEManager::RequestUnPauseSimulation(nullptr);
    }
}

void cFEng::PauseAllSystems() {
    if (UTL::Collections::Singleton<INIS>::Get()) {
        UTL::Collections::Singleton<INIS>::Get()->Pause();
    }
    SoundPause(true, static_cast<eSNDPAUSE_REASON>(-1));
    SetSoundControlState(true, SNDSTATE_ERROR, "PauseAllSystems");
}

void cFEng::ResumeAllSystems(bool flushActions) {
    SoundPause(false, static_cast<eSNDPAUSE_REASON>(-1));
    SetSoundControlState(false, SNDSTATE_ERROR, "PauseAllSystems");
    if (UTL::Collections::Singleton<INIS>::Get()) {
        UTL::Collections::Singleton<INIS>::Get()->UnPause();
    }
    if (flushActions) {
        cFEngJoyInput::Get()->FlushActions();
    }
}

void cFEng::QueuePackagePush(const char *pPackageName, int pArg, u32 ControlMask, bool pSuppressSimPause) {
    PrintLoadedPackages();
    if (TheGameFlowManager.IsInGame() && !pSuppressSimPause && !FEManager::IsPaused()) {
        FEManager::RequestPauseSimulation(pPackageName);
        HideEverySingleHud();
        bool push_bkg = IsPackagePushed("InGameBackground.fng");
        if (!push_bkg) {
            mFEng->QueuePackagePush("InGameBackground.fng", 0);
        }
    }
    FEPackageManager::Get()->SetPackageDataArg(pPackageName, pArg);
    mFEng->QueuePackagePush(pPackageName, ControlMask);
    if (!cFEngJoyInput::Get()->IsJoyEnabled(JOYSTICK_PORT_ALL)) {
        cFEngJoyInput::Get()->JoyEnable(JOYSTICK_PORT_ALL, true);
    }
#ifdef MILESTONE_BUILD
    Juice::GameHook::Instance()->AssetHit("MENU_POP-UP", pPackageName);
#endif
}

void cFEng::QueuePackagePop(int numPackagesToPop) {
    const int numPackagesPushed = mFEng->GetNumPackagesBelowPriority(FE_PACKAGE_PRIORITY_FIFTH_CLOSEST);
    if (numPackagesToPop <= 0 || numPackagesToPop > numPackagesPushed) {
        numPackagesToPop = numPackagesPushed;
    }
    PrintLoadedPackages();
    for (int i = 0; i < numPackagesToPop; i++) {
        mFEng->QueuePackagePop();
    }
    if (mFEng->GetNumPackagesBelowPriority(FE_PACKAGE_PRIORITY_FIFTH_CLOSEST) <= 0) {
        if (TheGameFlowManager.IsInGame() && FEManager::IsPaused()) {
            FEManager::RequestUnPauseSimulation(nullptr);
        }
        if (cFEngJoyInput::Get()->IsJoyEnabled(JOYSTICK_PORT_ALL)) {
            cFEngJoyInput::Get()->JoyDisable(JOYSTICK_PORT_ALL, true);
        }
    }
}

void cFEng::QueuePackageSwitch(const char *pPackageName, int pArg, u32 ControlMask, bool pSuppressSimPause) {
    PrintLoadedPackages();
    if (TheGameFlowManager.IsInGame() && !pSuppressSimPause && !FEManager::IsPaused()) {
        FEManager::RequestPauseSimulation(pPackageName);
        HideEverySingleHud();
        bool push_bkg = IsPackagePushed("InGameBackground.fng");
        if (!push_bkg) {
            mFEng->QueuePackagePush("InGameBackground.fng", 0);
        }
    }
    FEPackageManager::Get()->SetPackageDataArg(pPackageName, pArg);
    mFEng->QueuePackageSwitch(pPackageName, ControlMask);
    if (cFEngJoyInput::Get() && !cFEngJoyInput::Get()->IsJoyEnabled(JOYSTICK_PORT_ALL)) {
        cFEngJoyInput::Get()->JoyEnable(JOYSTICK_PORT_ALL, true);
    }
#ifdef MILESTONE_BUILD
    Juice::GameHook::Instance()->AssetHit("MENU_SCREENS", pPackageName);
#endif
}

void cFEng::PushNoControlPackage(const char *pPackageName, FE_PACKAGE_PRIORITY pPriority) {
    mFEng->PushPackage(pPackageName, pPriority, 0);
}

void cFEng::PopNoControlPackage(const char *pPackageName) {
    FEPackage *packageToPop = FEPackageManager::Get()->FindPackage(pPackageName);
    mFEng->UnloadPackage(packageToPop);
}

void cFEng::Service() {
    mFEng->Update(static_cast<int>(RealTimeElapsed * 1200.0f), IsGameFlowInGame() && WorldTimeElapsed > 0.0f);
}

void cFEng::ServiceFengOnly() {
    mFEng->Update(0, 0);
}

void cFEng::DrawForeground() {
    mFEng->Render();
}

FEPackage *cFEng::FindPackageWithControl() {
    FEPackageList *packageList = mFEng->GetPackageList();
    if (packageList != nullptr) {
        for (FEPackage *package = packageList->GetLastPackage(); package != nullptr; package = package->GetPrev()) {
            if (package->GetControlMask() != 0) {
                return package;
            }
        }
    }
    return nullptr;
}

FEPackage *cFEng::FindPackageAtBase() {
    FEPackageList *packageList = mFEng->GetPackageList();
    if (packageList != nullptr) {
        return packageList->GetFirstPackage();
    }
    return nullptr;
}

FEPackage *cFEng::FindPackageActive(const char *pPackageName) {
    FEPackageList *packageList = mFEng->GetPackageList();
    return packageList->FindPackage(pPackageName);
}

FEPackage *cFEng::FindPackageIdle(const char *pPackageName) {
    return mFEng->FindIdlePackage(pPackageName);
}

FEPackage *cFEng::FindPackage(const char *pPackageName) {
    if (pPackageName == nullptr || strlen(pPackageName) == 0) {
        return nullptr;
    }
    if (FEPackageData::IsInScreenConstructor()) {
        return FEPackageManager::Get()->FindPackage(pPackageName);
    }
    FEPackage *packagePtr = FindPackageActive(pPackageName);
    if (packagePtr != nullptr) {
        return packagePtr;
    }
    packagePtr = FindPackageIdle(pPackageName);
    if (packagePtr != nullptr) {
        return packagePtr;
    }
    return nullptr;
}

bool cFEng::IsPackagePushed(const char *pPackageName) {
    if (FEPackageData::IsInScreenConstructor()) {
        return FEPackageManager::Get()->FindPackage(pPackageName) != nullptr;
    } else {
        return FindPackageActive(pPackageName) != nullptr;
    }
}

bool cFEng::IsPackageInControl(const char *pPackageName) {
    FEPackage *packageWithCtrl = FindPackageWithControl();
    if (packageWithCtrl != nullptr) {
        return bStrCmp(pPackageName, packageWithCtrl->GetName()) == 0;
    }
    return false;
}

void cFEng::PrintLoadedPackages() {
#ifdef EA_BUILD_A124 // la A124 recorre los paquetes; lo que imprimia de cada uno no deja codigo
    if (mFEng->GetPackageList() != nullptr) {
        for (FEPackage *pkg = mFEng->GetPackageList()->GetFirstPackage(); pkg != nullptr; pkg = pkg->GetNext()) {
        }
    }
#endif
}

void cFEng::QueueMessage(uint32 pMessage, const char *pPackageName, FEObject *to, uint32 controlMask) {
    if (pPackageName != nullptr) {
        FEPackage *package = FindPackage(pPackageName);
        if (package != nullptr) {
            mFEng->QueueMessage(pMessage, nullptr, package, to, controlMask);
        }
    } else {
        FEPackageList *pkg_list = mFEng->GetPackageList();
        if (pkg_list != nullptr) {
            for (FEPackage *pkg = pkg_list->GetFirstPackage(); pkg != nullptr; pkg = pkg->GetNext()) {
                mFEng->QueueMessage(pMessage, nullptr, pkg, to, 0);
            }
        }
    }
}

void cFEng::QueueGameMessage(uint32 pMessage, const char *pPackageName, uint32 controlMask) {
    QueueMessage(pMessage, pPackageName, reinterpret_cast<FEObject *>(-1), controlMask);
}

void cFEng::QueueGameMessagePkg(uint32 pMessage, FEPackage *topkg) {
    mFEng->QueueMessage(pMessage, nullptr, topkg, reinterpret_cast<FEObject *>(-1), 0);
}

void cFEng::QueuePackageMessage(uint32 pMessage, const char *pPackageName, FEObject *obj) {
    if (obj != nullptr) {
        QueueMessage(pMessage, pPackageName, obj, 0xFF);
    } else {
        QueueMessage(pMessage, pPackageName, reinterpret_cast<FEObject *>(-4), 0xFF);
    }
}

void cFEng::QueueSoundMessage(uint32 pMessage, const char *pPackageName) {
    FEPackage *package = FindPackage(pPackageName);
    if (package != nullptr) {
        mFEng->QueueMessage(pMessage, nullptr, package, reinterpret_cast<FEObject *>(-5), 0);
    }
}

void cFEng::EnablePackageControl(FEPackage *pkg, bool bProcess) {
    mFEng->SetProcessInput(pkg, bProcess);
}
#ifdef EA_BUILD_A124
void FEngKillAllChildren(FEPackage *pParent) {
    FEPackage *pPackage = cFEng::Get()->GetEngine()->GetPackageList()->GetFirstPackage();
    while (pPackage) {
        FEPackage *pNextPackage = pPackage->GetNext();
        if (pPackage->GetParentPackage() == pParent) {
            FEngKillAllChildren(pPackage);
            cFEng::Get()->QueuePackagePop(1);
        }
        pPackage = pNextPackage;
    }
}
#endif

void cFEng::MakeLoadedPackagesDirty() {
    FEPackageList *pkg_list = mFEng->GetPackageList();
    if (pkg_list != nullptr) {
        ObjectDirtySetter dirt;
        for (FEPackage *pkg = pkg_list->GetFirstPackage(); pkg != nullptr; pkg = pkg->GetNext()) {
            dirt.pRenderInfo = HACK_FEPkgMgr_GetPackageRenderInfo(pkg);
            pkg->ForAllObjects(dirt);
        }
    }
}
