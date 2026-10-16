#include "uiSafehouseRegionUnlock.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "uiRepSheetRivalFlow.hpp"
#ifdef EA_BUILD_A124
#include "uiRepSheetMain.hpp"
#endif

uiSafehouseRegionUnlock::uiSafehouseRegionUnlock(ScreenConstructorData *sd) : MenuScreen(sd), RivalStreamer(sd->PackageFilename, false) {
    Setup();
}

uiSafehouseRegionUnlock::~uiSafehouseRegionUnlock() {}

void uiSafehouseRegionUnlock::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    if (msg == 0x406415E3) {
        uiRepSheetRivalFlow::Get()->Next();
    }
}

void uiSafehouseRegionUnlock::Setup() {
    pRivalImg = FEngFindImage(GetPackageName(), 0xc1f62308);
    pTagImg = FEngFindImage(GetPackageName(), 0xf5a2a087);
    pBGImg = FEngFindImage(GetPackageName(), 0x2cbe1dd0);
#ifdef EA_BUILD_A124
    int cur_stage = FEDatabase->GetCareerSettings()->GetCurrentBin();
#else
    int cur_stage = FEDatabase->GetCareerSettings()->GetCurrentBin() + 1;
#endif
    if (cur_stage == 13) {
        FEngSetLanguageHash(GetPackageName(), 0xd6c0e097, 0x29e4b193);
    } else if (cur_stage == 9) {
        FEngSetLanguageHash(GetPackageName(), 0xd6c0e097, 0x2b0bca2d);
    }
#ifdef EA_BUILD_A124
    RivalStreamer.Init(iCurrentViewBin, pRivalImg, pTagImg, pBGImg);
#else
    RivalStreamer.Init(static_cast<uint32>(FEDatabase->GetCareerSettings()->GetCurrentBin()) + 1, pRivalImg, pTagImg, pBGImg);
#endif
}
