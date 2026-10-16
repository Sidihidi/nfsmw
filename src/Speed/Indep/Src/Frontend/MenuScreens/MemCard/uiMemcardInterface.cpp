#include "Speed/Indep/Src/Frontend/MenuScreens/MemCard/uiMemcardInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"

MemoryCardSetup gMemcardSetup;

void MemcardEnter(const char *from, const char *to, uint32 op, MemCardOpType termFunc, void *termParam, uint32 successMsg, uint32 failedMsg) {
    gMemcardSetup.mOp = op;
    gMemcardSetup.mFromScreen = from;
    gMemcardSetup.mToScreen = to;
    gMemcardSetup.mMemScreen = nullptr;
    gMemcardSetup.mTermFunc = termFunc;
    gMemcardSetup.mTermFuncParam = termParam;
    gMemcardSetup.mSuccessMsg = successMsg;
    gMemcardSetup.mFailedMsg = failedMsg;
    MemoryCard::GetInstance()->ShowMessages(true);
#ifndef EA_BUILD_A124
    MemoryCard::GetInstance()->SetPlayerNum((op & 0x20000) != 0);
#endif
#ifdef EA_PLATFORM_PLAYSTATION2
    gMemcardSetup.mMemScreen = TheGameFlowManager.IsInFrontend() ? "MC_Main.fng" : "InGame_MC_Main.fng";
#else
    gMemcardSetup.mMemScreen = TheGameFlowManager.IsInFrontend() ? "MC_Main_GC.fng" : "InGame_MC_Main_GC.fng";
#endif
#ifdef EA_BUILD_A124
    // La A124 elige la pantalla antes y lee el jugador de las opciones extra (GetExtraOptions, uiMemcardInterface.hpp:192).
    MemoryCard::GetInstance()->SetPlayerNum((gMemcardSetup.GetExtraOptions() & 0x20000) != 0);
#endif
    uint32 cmd = gMemcardSetup.mOp & 0xf;
    switch (cmd) {
        case 1:
        case 3:
            cFEng::Get()->QueuePackagePush(gMemcardSetup.mMemScreen, 0, 0, false);
            break;
        case 2:
            cFEng::Get()->QueuePackageSwitch(gMemcardSetup.mMemScreen, 0, 0, false);
            break;
    }
    MemoryCard::GetInstance()->SetMemcardScreenShowing(true);
}

void MemcardExit(unsigned int msg) {
    gMemcardSetup.mLastMessage = msg;
    if (!MemoryCard::GetInstance()->IsMemcardScreenInitialized()) {
        cFEng *feng = cFEng::Get();
        u32 hash = FEHashUpper("EXIT_COMPLETE");
        feng->QueueGameMessage(hash, gMemcardSetup.mMemScreen, 0xff);
    } else {
        cFEng *feng = cFEng::Get();
        u32 hash = FEHashUpper("LEAVE_SCREEN");
        feng->QueuePackageMessage(hash, gMemcardSetup.mMemScreen, nullptr);
    }
    MemoryCard::GetInstance()->SetMemcardScreenInitialized(false);
#ifdef EA_PLATFORM_GAMECUBE
    MemoryCard::GetInstance()->SetMemcardScreenExiting(true);
#endif
}

uint32 MemcardGetCurrentUIOperation() {
    return gMemcardSetup.mOp & 0xf0;
}

