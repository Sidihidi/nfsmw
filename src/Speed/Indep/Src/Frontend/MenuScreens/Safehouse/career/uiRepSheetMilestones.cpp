#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiTrackMapStreamer.hpp"
#include "uiRepSheetMilestones.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"

#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEAnyTutorialScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/InGameTutorialScreen.hpp"
#include "Speed/Indep/Src/Frontend/RaceStarter.hpp"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Generated/Events/ERaceSheetOff.hpp"
#include "Speed/Indep/Tools/Inc/ConversionUtil.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/Src/Generated/FEngHash/FEHash_FeWorldMapQuickList.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"

struct FEObject;
struct FEMultiImage;

int FEPrintf(const char *pkg_name, int hash, const char *fmt, ...);
unsigned int FEngHashString(const char *format, ...);
// [fe3] const char *GetLocalizedString(unsigned int hash);
void FEngSetScript(const char *pkg_name, unsigned int obj_hash, unsigned int script_hash, bool);
void FEngSetRotationZ(FEObject *obj, float angle);
void FEngSetTextureHash(FEImage *image, unsigned int hash);
int FEngMapJoyParamToJoyport(int feng_param);

extern unsigned int iCurrentViewBin;
extern const char *gTUTORIAL_MOVIE_PURSUIT;

// FEngSetInvisible/FEngSetVisible/FEngSetTextureHash inlines defined in uiMain.cpp

MilestoneDatum *theMilestone = nullptr;
const char *gTUTORIAL_MOVIE_PURSUIT = "pursuit_tutorial";

void MilestoneDatum::NotificationMessage(u32 msg, FEObject *pObj, u32 param1, u32 param2) {
    if (msg != 0xc407210) {
        return;
    }
#ifdef EA_BUILD_A124 // la A124 no mira si ya esta marcado
    theMilestone = this;
#else
    if (!IsChecked()) {
        theMilestone = this;
    } else {
        theMilestone = nullptr;
    }
#endif
}

uiRepSheetMilestones::uiRepSheetMilestones(ScreenConstructorData *sd) : ArrayScrollerMenu(sd, 3, 3, true), bIsInGame(sd->Arg != 0) {
#ifndef EA_BUILD_A124
    TrackMapStreamer = nullptr;
    theMilestone = nullptr;
    TrackMapStreamer = new ("UITrackMapStreamer", 0) UITrackMapStreamer();
#endif
    for (int i = 0; i < GetWidth() * GetHeight(); i++) {
        FEImage *img = FEngFindImage(GetPackageName(), FEngHashString("EVENT_ICON_%d", i + 1));
        if (img) {
            AddSlot(new ("ImageArraySlot", 0) ImageArraySlot(img));
        }
    }
    TrackMap = reinterpret_cast<FEMultiImage *>(FEngFindObject(GetPackageName(), FEngHashString("TRACK_MAP")));
#ifdef EA_BUILD_A124 // la A124 tiene un solo titulo, dentro y fuera de la partida
    FEngSetLanguageHash(GetPackageName(), 0xbde82fcc, 0x216f1b81);
#else
    if (bIsInGame) {
        FEngSetLanguageHash(GetPackageName(), 0xbde82fcc, 0x578b767b);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0xbde82fcc, 0x216f1b81);
    }
#endif
    Setup();
}

eMenuSoundTriggers uiRepSheetMilestones::NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) {
    eMenuSoundTriggers result = ArrayScrollerMenu::NotifySoundMessage(msg, maybe);
    if (msg == 0x7b6b89d7 && bIsInGame) {
        return static_cast<eMenuSoundTriggers>(-1);
    }
    return result;
}

void uiRepSheetMilestones::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    int currentIndex = data.IsInList(currentDatum) - 1;
    ArrayScrollerMenu::NotificationMessage(msg, obj, param1, param2);
    if (msg == 0x911c0a4b) {
        goto refresh;
    }
    if (msg <= 0x911c0a4b) {
        if (msg == 0x34dc1bcf) {
            return;
        }
        if (msg <= 0x34dc1bcf) {
            if (msg == 0xc407210) {
                goto handleActivate;
            }
            return;
        }
        if (msg == 0x72619778) {
            goto refresh;
        }
        if (msg == 0x911ab364) {
            goto handlePackageSwitch;
        }
        return;
    }
    if (msg == 0xc3960eb9) {
        goto handleWarp;
    }
    if (msg <= 0xc3960eb9) {
        if (msg == 0x9120409e || msg == 0xb5971bf1) {
            goto refresh;
        }
        return;
    }
    if (msg == 0xc98356ba) {
        goto handleUpdateAnimation;
    }
    if (msg <= 0xc98356ba) {
        if (msg == 0xc519bfc3) {
            goto handleTutorial;
        }
        return;
    }
    if (msg == 0xd05fc3a3) {
        goto handleTutorialAccept;
    }
    return;

handleActivate: {
    if (theMilestone == nullptr) {
        return;
    }
    if (theMilestone->IsChecked()) {
        g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(7));
        return;
    }
    if (!bIsInGame) {
        signed char joyPort = static_cast<signed char>(FEngMapJoyParamToJoyport(param1));
        FEDatabase->SetPlayersJoystickPort(0, joyPort);
    }
    const char *dialog = "";
    if (bIsInGame) {
        dialog = "InGameDialog.fng";
    }
    unsigned int messageHash = 0xa5a8409a;
    if (theMilestone->GetType() != 0) {
        messageHash = 0xbf1dcd38;
    }
    DialogInterface::ShowTwoButtons(GetPackageName(), dialog, static_cast<eDialogTitle>(1), LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO, 0xd05fc3a3, 0x34dc1bcf,
                                    0x34dc1bcf, static_cast<eDialogFirstButtons>(1), messageHash);
    return;
}

handleTutorial: {
    if (bIsInGame) {
        return;
    }
    FEngSetScript(GetPackageName(), 0x99344537, 0x16a259, true);
    FEAnyTutorialScreen::LaunchMovie(gTUTORIAL_MOVIE_PURSUIT
#ifndef EA_BUILD_A124
                                     ,
                                     GetPackageName()
#endif
    );
    return;
}

handleWarp: {
    if (bIsInGame) {
        FEngSetVisible("InGameBackground.fng", 0x2716cdbf);
    }
    FEngSetScript(GetPackageName(), 0x99344537, 0x1744b3, true);
    if (theMilestone == nullptr) {
        return;
    }
    unsigned int marker;
    bool pursuit = false;
    if (theMilestone->GetType() == 0) {
        GMilestone *pMilestone = theMilestone->my_milestone;
        marker = pMilestone->GetJumpMarkerKey();
        pursuit = true;
    } else {
        SpeedTrapDatum *st = static_cast<SpeedTrapDatum *>(theMilestone);
        GSpeedTrap *pSpeedTrap = st->my_speedtrap;
        marker = pSpeedTrap->GetJumpMarkerKey();
    }
    if (bIsInGame) {
        new ERaceSheetOff();
        GManager::Get().WarpToMarker(marker, pursuit);
        return;
    }
    GManager::Get().OverrideFreeRoamStartMarker(marker);
    if (pursuit) {
        GManager::Get().QueueFreeRoamPursuit(0.0f);
    }
    RaceStarter::StartCareerFreeRoam();
    return;
}

handleUpdateAnimation:
    // La A124 salta a UpdateAnimation con this+0xE8 sin mirar nada: miembro por valor, sin comprobacion.
#ifndef EA_BUILD_A124
    if (TrackMapStreamer != nullptr)
#endif
    {
#ifdef EA_BUILD_A124
        TrackMapStreamer.UpdateAnimation();
#else
        TrackMapStreamer->UpdateAnimation();
#endif
    }
    return;

refresh: {
    int newIndex = data.IsInList(currentDatum) - 1;
    if (currentIndex != newIndex && currentDatum != nullptr) {
        RefreshTrack();
    }
    return;
}

handleTutorialAccept: {
    CareerSettings *career = FEDatabase->GetCareerSettings();
    if (((career->SpecialFlags >> 9) & 1) == 0) {
        if (bIsInGame) {
#ifndef EA_BUILD_A124
            if (TrackMapStreamer != nullptr) {
                delete TrackMapStreamer;
            }
            TrackMapStreamer = nullptr;
#endif
            InGameAnyTutorialScreen::LaunchMovie(gTUTORIAL_MOVIE_PURSUIT
#ifndef EA_BUILD_A124
                                                 ,
                                                 GetPackageName()
#endif
            );
            FEngSetInvisible("InGameBackground.fng", 0x2716cdbf);
        } else {
            FEAnyTutorialScreen::LaunchMovie(gTUTORIAL_MOVIE_PURSUIT
#ifndef EA_BUILD_A124
                                             ,
                                             GetPackageName()
#endif
            );
        }
        FEngSetScript(GetPackageName(), 0x99344537, 0x16a259, true);
        FEngSetInvisible(GetPackageName(), FEngHashString("TRACK_MAP"));
        career->SpecialFlags |= 0x200;
        return;
    }
    cFEng::Get()->QueueGameMessage(0xc3960eb9, GetPackageName(), 0xff);
    return;
}

handlePackageSwitch:
    if (bIsInGame) {
        cFEng::Get()->QueuePackageSwitch("InGameReputationOverview.fng", 1, 0, false);
    } else {
        cFEng::Get()->QueuePackageSwitch("SafeHouseReputationOverview.fng", 0, 0, false);
    }
    return;
}

void uiRepSheetMilestones::Setup() {
    ClearData();
    GMilestone *ms = GManager::Get().GetFirstMilestone(false, iCurrentViewBin);
    while (ms != nullptr) {
        AddMilestone(ms);
        if (ms->GetIsLocked()) {
            GetDatumAt(GetNumDatum() - 1)->SetLocked(true);
        }
        if (ms->GetIsAwarded()) {
            GetDatumAt(GetNumDatum() - 1)->SetChecked(true);
        }
        ms = GManager::Get().GetNextMilestone(ms, false, iCurrentViewBin);
    }
    GSpeedTrap *st = GManager::Get().GetFirstSpeedTrap(false, iCurrentViewBin);
    while (st != nullptr) {
        AddSpeedtrap(st);
        if (st->IsFlagClear(GSpeedTrap::kFlag_Unlocked)) {
            GetDatumAt(GetNumDatum() - 1)->SetLocked(true);
        }
        if (st->IsFlagSet(GSpeedTrap::kFlag_Completed)) {
            GetDatumAt(GetNumDatum() - 1)->SetChecked(true);
        }
        st = GManager::Get().GetNextSpeedTrap(st, false, iCurrentViewBin);
    }
    SetDescLabel(0xB5117FDE);
    SetInitialPosition(0);
    RefreshTrack();
    RefreshHeader();
}

void uiRepSheetMilestones::RefreshTrack() {
    if (GetCurrentDatum() != nullptr) {
        bVector2 position;
        float rotation = 0.0f;
        // La A124 (0x23B310) llama a Init, ResetZoom, PanTo y ZoomTo con this+0xE8 y sin comprobar nada.
#ifndef EA_BUILD_A124
        if (TrackMapStreamer != nullptr)
#endif
        {
#ifdef EA_BUILD_A124
            TrackMapStreamer.Init(nullptr, TrackMap, 0, 0);
#else
            TrackMapStreamer->Init(nullptr, TrackMap, 0, 0);
#endif
#ifdef EA_BUILD_A124
            TrackMapStreamer.ResetZoom(false);
#else
            TrackMapStreamer->ResetZoom(false);
#endif
        }
        unsigned int key;
        if (static_cast<MilestoneDatum *>(GetCurrentDatum())->GetType() == 0) {
            GMilestone *pMilestone = static_cast<MilestoneDatum *>(GetCurrentDatum())->my_milestone;
            key = pMilestone->GetJumpMarkerKey();
        } else {
            SpeedTrapDatum *sdt = static_cast<SpeedTrapDatum *>(static_cast<MilestoneDatum *>(GetCurrentDatum()));
            GSpeedTrap *pSpeedTrap = sdt->my_speedtrap;
            key = pSpeedTrap->GetJumpMarkerKey();
        }
        GManager::Get().CalcMapCoordsForMarker(key, position, rotation);
#ifndef EA_BUILD_A124
        if (TrackMapStreamer != nullptr)
#endif
        {
#ifdef EA_BUILD_A124
            TrackMapStreamer.PanTo(position);
#else
            TrackMapStreamer->PanTo(position);
#endif
#ifdef EA_BUILD_A124
            TrackMapStreamer.ZoomTo(bVector2(0.27f, 0.27f));
#else
            TrackMapStreamer->ZoomTo(bVector2(0.27f, 0.27f));
#endif
        }
        FEngSetRotationZ(FEngFindObject(GetPackageName(), 0xaf51dd73), rotation);
    }
}

void uiRepSheetMilestones::AddMilestone(GMilestone *milestone) {
    ArrayScroller *scroller = this;
    MilestoneDatum *datum = new ("MilestoneDatum", 0) MilestoneDatum(FEDatabase->GetMilestoneIconHash(milestone->GetTypeKey(), true),
                                               FEDatabase->GetMilestoneHeaderHash(milestone->GetLocalizationTag()), milestone);
    scroller->AddDatum(datum);
}

void uiRepSheetMilestones::AddSpeedtrap(GSpeedTrap *trap) {
    ArrayScroller *scroller = this;
    SpeedTrapDatum *datum = new ("SpeedTrapDatum", 0) SpeedTrapDatum(FEDatabase->GetRaceIconHash(static_cast<GRace::Type>(5)), 0xF3B3D8DC, trap);
    scroller->AddDatum(datum);
}

void uiRepSheetMilestones::RefreshHeader() {
    ArrayScrollerMenu::RefreshHeader();
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    FEPrintf(GetPackageName(), __NUMBER_GROUP__, "%d", GetCurrentDatumNum() + 1);
#else
    FEPrintf(GetPackageName(), __NUMBER_GROUP__, "%d", GetCurrentDatumNum());
#endif
    FEPrintf(GetPackageName(), __NUMBER_OF_GROUP__, "%d", GetNumDatum());
#ifdef EA_BUILD_A124 // la A124 pone el nombre del perfil y el dinero sin rotulos
    FEPrintf(GetPackageName(), 0xb514e2d8, "%s", FEDatabase->GetMultiplayerProfile(0)->GetProfileName());
    FEPrintf(GetPackageName(), 0xf91a59f6, "%$d", FEDatabase->GetCareerSettings()->GetCash());
#else
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    FEPrintf(GetPackageName(), 0xb514e2d8, "%s %$d", GetLocalizedString(0xce6b99b1), stable->GetTotalBounty());
    FEPrintf(GetPackageName(), 0xf91a59f6, "%s %$d", GetLocalizedString(0x73b79e0), FEDatabase->GetCareerSettings()->GetCash());
#endif
    ArrayDatum *currentDatum = GetCurrentDatum();
    if (currentDatum != nullptr) {
        MilestoneDatum *d = static_cast<MilestoneDatum *>(currentDatum);
        if (d->GetType() == 0) {
            GMilestone *pMilestone = d->my_milestone;
            FEngSetTextureHash(GetPackageName(), 0xf97ec5d5, FEDatabase->GetMilestoneIconHash(pMilestone->GetTypeKey(), true));
            FEPrintf(GetPackageName(), 0xb21d69bd, "%$0.0f", pMilestone->GetBounty());
            float goal = pMilestone->GetRequiredValue();
            if (FEDatabase->IsMilestoneTimeFormat(pMilestone->GetTypeKey())) {
                goal = goal * 0.016666668f;
            }
            char buf[32];
            bSNPrintf(buf, 32, "%$0.0f", goal);
            FEPrintf(GetPackageName(), 0x28049d6, GetLocalizedString(FEDatabase->GetMilestoneDescHash(pMilestone->GetLocalizationTag())),
                     buf, buf);
        } else {
            SpeedTrapDatum *p = static_cast<SpeedTrapDatum *>(d);
            GSpeedTrap *pSpeedTrap = p->my_speedtrap;
            FEngSetTextureHash(GetPackageName(), 0xf97ec5d5, FEDatabase->GetRaceIconHash(GRace::kRaceType_SpeedTrap));
            FEPrintf(GetPackageName(), 0xb21d69bd, "%$0.0f", pSpeedTrap->GetBounty());
            float value;
            const char *distUnits;
            if (FEDatabase->GetGameplaySettings()->SpeedoUnits == 1) {
                value = static_cast<float>(bFloatToInt(MPS2KPH(pSpeedTrap->GetTriggerSpeed())));
                distUnits = GetLocalizedString(0x8569a25f);
            } else {
                value = static_cast<float>(bFloatToInt(MPS2MPH(pSpeedTrap->GetTriggerSpeed())));
                distUnits = GetLocalizedString(0x8569ab44);
            }
            char buf[32];
            bSNPrintf(buf, 32, "%$0.0f %s", value, distUnits);
            FEPrintf(GetPackageName(), 0x28049d6, GetLocalizedString(0xb14018bd), buf);
        }
        for (int i = 0; i < GetNumSlots(); i++) {
            ArrayDatum *datum = GetDatumAt(i + GetStartDatumNum());
            unsigned int check_hash = FEngHashString("MEDAL_THUMB_%d", i + 1);
            FEngSetInvisible(GetPackageName(), check_hash);
            if (datum) {
                if (datum->IsLocked()) {
                    FEngSetVisible(GetPackageName(), check_hash);
                    FEngSetTextureHash(GetPackageName(), check_hash, 0x18ed48);
                } else if (datum->IsChecked()) {
                    FEngSetVisible(GetPackageName(), check_hash);
                    FEngSetTextureHash(GetPackageName(), check_hash, 0x28feadd);
                } else {
                    FEngSetInvisible(GetPackageName(), check_hash);
                }
            } else {
                FEngSetInvisible(GetPackageName(), check_hash);
            }
        }
    }
}

inline bool GSpeedTrap::IsFlagSet(unsigned int mask) const {
    return (mFlags & mask) != 0;
}

inline bool GSpeedTrap::IsFlagClear(unsigned int mask) const {
    return (mFlags & mask) == 0;
}
