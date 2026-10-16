#include "Speed/Indep/Src/Frontend/MenuScreens/Career/FEPkg_EngageEventDialog.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Gameplay/GActivity.h"
#include "Speed/Indep/Src/Gameplay/GRuntimeInstance.h"
#include "Speed/Indep/Src/Generated/Messages/MAcceptEnterCareerEvent.h"
#include "Speed/Indep/Src/Generated/Messages/MDeclineEnterCareerEvent.h"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"

namespace nsEngageEventDialog {

EngageEventDialog::EngageEventDialog(ScreenConstructorData *sd)
    : MenuScreen(sd), mpRaceActivity(reinterpret_cast<GRuntimeInstance *>(sd->Arg)), MapStreamer(nullptr) {
    GRaceParameters *parms = GRaceDatabase::Get().GetRaceFromActivity(reinterpret_cast<GActivity *>(mpRaceActivity));

    FEngSetTextureHash(GetPackageName(), 0xad9e232a, FEDatabase->GetRaceIconHash(parms->GetRaceType()));
    FEngSetLanguageHash(GetPackageName(), 0xa01b9361, FEDatabase->GetRaceNameHash(parms->GetRaceType()));
    FEngSetLanguageHash(GetPackageName(), 0xf601f2d4, CalcLanguageHash("TRACKNAME_", parms));

    FEPrintf(GetPackageName(), 0x644ab208, "%d", parms->GetNumLaps());

    bool kph;
    const char *distUnits;
    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        kph = true;
        distUnits = GetLocalizedString(0x8569a26a);
    } else {
        kph = false;
        distUnits = GetLocalizedString(0x867dcfd9);
    }

#ifdef EA_BUILD_A124 // la A124 redondea la milla a 1600 m
    FEPrintf(GetPackageName(), 0xbce13923, "%$0.1f %s", parms->GetRaceLengthMeters() * (kph ? 0.001f : 0.000625f), distUnits);
#else
    FEPrintf(GetPackageName(), 0xbce13923, "%$0.1f %s",
             parms->GetRaceLengthMeters() * (kph ? 0.001f : 0.0006213728f), distUnits);
#endif

    unsigned int hash;
#ifdef EA_BUILD_A124 // la A124 muestra la densidad de policia, no si hay policia
    switch (parms->GetCopDensity()) {
        case 1:
            hash = LANGUAGE_COMMON_MINIMUM;
            break;
        case 2:
            hash = 0xa2cca838;
            break;
        case 3:
            hash = LANGUAGE_COMMON_MAXIMUM;
            break;
        case 0:
            hash = LANGUAGE_COMMON_NONE;
            break;
        default:
            hash = 0;
            break;
    }
#else
    if (parms->GetCopsEnabled()) {
        hash = LANGUAGE_COMMON_MAXIMUM;
    } else {
        hash = LANGUAGE_COMMON_MINIMUM;
    }
#endif
    FEngSetLanguageHash(GetPackageName(), 0x762f1d7a, hash);

    GRaceSaveInfo *info = GRaceDatabase::Get().GetScoreInfo(parms->GetEventHash());

    if (parms->GetRaceType() == GRace::kRaceType_P2P || parms->GetRaceType() == GRace::kRaceType_Circuit ||
        parms->GetRaceType() == GRace::kRaceType_Drag || parms->GetRaceType() == GRace::kRaceType_Knockout ||
        parms->GetRaceType() == GRace::kRaceType_Tollbooth) {
        Timer timer(info->mHighScores.mBestTime);
        char buf[64];
        timer.PrintToString(buf, 0);
        FEPrintf(GetPackageName(), 0x8fd41bb4, "%s", buf);
    } else {
#ifdef EA_BUILD_A124
        FEPrintf(GetPackageName(), 0x8fd41bb4, "--:--:--");
#else
        FEPrintf(GetPackageName(), 0x8fd41bb4, "%s", GetLocalizedString(0x472aa00a));
#endif
    }

    const char *speedUnits;
    float avg_speed;
    float top_speed;
    if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
        speedUnits = GetLocalizedString(0x8569a25f);
        avg_speed = MPS2KPH(info->mAverageSpeed);
        top_speed = MPS2KPH(info->mTopSpeed);
    } else {
        speedUnits = GetLocalizedString(0x8569ab44);
        avg_speed = MPS2MPH(info->mAverageSpeed);
        top_speed = MPS2MPH(info->mTopSpeed);
    }
    FEPrintf(GetPackageName(), 0x35d1ab83, "%$0.0f %s", avg_speed, speedUnits);
    FEPrintf(GetPackageName(), 0xde9145fb, "%$0.0f %s", top_speed, speedUnits);

    FEPrintf(GetPackageName(), 0x45276f1f, "%$0.0f", parms->GetCashValue());

    TrackMap = reinterpret_cast<FEMultiImage *>(FEngFindObject(GetPackageName(), FEngHashString("TRACK_MAP")));
    MapStreamer = new ("MapStreamer", 0) UITrackMapStreamer();
    MapStreamer->Init(parms, TrackMap, 0, 0);
}

EngageEventDialog::~EngageEventDialog() {
    delete MapStreamer;
    MapStreamer = nullptr;
}

void EngageEventDialog::NotifyTheGameAcceptEvent() {
    MAcceptEnterCareerEvent().Post(UCrc32(0x20d60dbf));
}

void EngageEventDialog::NotifyTheGameDeclineEvent() {
    MDeclineEnterCareerEvent().Post(UCrc32(0x20d60dbf));
}

// STRIPPED
void EngageEventDialog::NotifyTheGameButton3() {}

#ifdef EA_BUILD_A124 // la A124 tiene tres botones y navega entre aceptar y rechazar
void EngageEventDialog::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {
        case 0xc98356ba:
            MapStreamer->UpdateAnimation();
            break;
        case 0x35f8620b:
            FEngSetCurrentButton(GetPackageName(), 0x63857de0);
            break;
        case 0x911ab364:
            NotifyTheGameDeclineEvent();
            cFEng::Get()->QueuePackagePop(0);
            break;
        case 0x9120409e:
            if (FEngGetCurrentButton(GetPackageName())->NameHash == 0x63857de0) {
                FEngSetCurrentButton(GetPackageName(), 0xd72f002a);
            }
            break;
        case 0xb5971bf1:
            if (FEngGetCurrentButton(GetPackageName())->NameHash == 0xd72f002a) {
                FEngSetCurrentButton(GetPackageName(), 0x63857de0);
            }
            break;
        case 0x0c407210:
            switch (obj->NameHash) {
                case 0xd72f002a:
                    NotifyTheGameAcceptEvent();
                    cFEng::Get()->QueuePackagePop(0);
                    break;
                case FEHASH_BUTTON3:
                    NotifyTheGameButton3();
                    cFEng::Get()->QueuePackagePop(0);
                    break;
                case 0x63857de0:
                    NotifyTheGameDeclineEvent();
                    cFEng::Get()->QueuePackagePop(0);
                    break;
            }
            break;
    }
}
#else
void EngageEventDialog::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {
        case 0xc98356ba:
            MapStreamer->UpdateAnimation();
            break;
        case 0x911ab364:
            NotifyTheGameDeclineEvent();
            cFEng::Get()->QueuePackagePop(0);
            break;
        case 0x0c407210: {
            unsigned int objHash = obj->NameHash;
            if (objHash != 0x694b896e) {
                if (objHash == 0xd72f002a) {
                    NotifyTheGameAcceptEvent();
                    cFEng::Get()->QueuePackagePop(0);
                }
            } else {
                NotifyTheGameDeclineEvent();
                cFEng::Get()->QueuePackagePop(0);
            }
            break;
        }
    }
}
#endif

} // namespace nsEngageEventDialog
