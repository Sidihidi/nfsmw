#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiTrackMapStreamer.hpp"
#include "uiRepSheetBounty.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEAnyTutorialScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/InGameTutorialScreen.hpp"
#include "Speed/Indep/Src/Frontend/RaceStarter.hpp"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Generated/Events/ERaceSheetOff.hpp"
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
// .bss:0x80472B18 del original (4 B, global). Nadie mas lo define: el objeto
// de zFe lo referencia como UND y sin esto la unidad no enlaza.
unsigned int theMarker;
const char *gTUTORIAL_MOVIE_BOUNTY = "bounty_tutorial";

// FEngSetInvisible/FEngSetVisible inlines defined in uiMain.cpp
// FEngSetTextureHash inline already defined in uiOptionsScreen.cpp



void BountyDatum::NotificationMessage(u32 msg, FEObject *pObj, u32 param1, u32 param2) {
    if (msg == 0xc407210) {
        theMarker = GManager::Get().GetBountySpawnMarker(static_cast<unsigned int>(index));
    }
}

uiRepSheetBounty::uiRepSheetBounty(ScreenConstructorData *sd) : ArrayScrollerMenu(sd, 3, 3, true), bIsInGame(sd->Arg != 0) {
#ifndef EA_BUILD_A124
    TrackMapStreamer = nullptr;
#endif
    TrackMap = nullptr;
#ifndef EA_BUILD_A124
    tutorialPlaying = false;
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
    FEngSetLanguageHash(GetPackageName(), 0xbde82fcc, 0xe451941e);
#else
    if (bIsInGame) {
        FEngSetLanguageHash(GetPackageName(), 0xbde82fcc, 0x6ddfa694);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0xbde82fcc, 0xe451941e);
    }
#endif
    Setup();
}

eMenuSoundTriggers uiRepSheetBounty::NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) {
    if (msg == 0x7b6b89d7 && bIsInGame)
        return static_cast<eMenuSoundTriggers>(-1);
#ifndef EA_BUILD_A124 // la A124 no mira si la recompensa esta bloqueada
    if (currentDatum->IsLocked())
        return static_cast<eMenuSoundTriggers>(7);
#endif
    return maybe;
}

void uiRepSheetBounty::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    int currentIndex = data.IsInList(currentDatum) - 1;
    ArrayScrollerMenu::NotificationMessage(msg, obj, param1, param2);
    switch (msg) {
        case 0xc407210: {
            BountyDatum *d = static_cast<BountyDatum *>(GetCurrentDatum());
            if (GetNumDatum() < 1) {
                return;
            }
            if (d->IsLocked()) {
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
            DialogInterface::ShowTwoButtons(GetPackageName(), dialog, static_cast<eDialogTitle>(1), LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO, 0xd05fc3a3, 0x34dc1bcf,
                                            0x34dc1bcf, static_cast<eDialogFirstButtons>(1), 0xcd195d0b);
            return;
        }
        case 0xc519bfc3:
            if (bIsInGame) {
                return;
            }
#ifndef EA_BUILD_A124
            tutorialPlaying = true;
#endif
            FEngSetScript(GetPackageName(), 0x99344537, 0x16a259, true);
            FEAnyTutorialScreen::LaunchMovie(gTUTORIAL_MOVIE_BOUNTY
#ifndef EA_BUILD_A124
                                             ,
                                             GetPackageName()
#endif
            );
            return;
        case 0xd05fc3a3: {
            CareerSettings *career = FEDatabase->GetCareerSettings();
            if (((career->SpecialFlags >> 10) & 1) == 0) {
                if (bIsInGame) {
#ifndef EA_BUILD_A124
                    if (TrackMapStreamer != nullptr) {
                        delete TrackMapStreamer;
                    }
                    TrackMapStreamer = nullptr;
#endif
                    InGameAnyTutorialScreen::LaunchMovie(gTUTORIAL_MOVIE_BOUNTY
#ifndef EA_BUILD_A124
                                                         ,
                                                         GetPackageName()
#endif
                    );
                    FEngSetInvisible("InGameBackground.fng", 0x2716cdbf);
                } else {
                    FEAnyTutorialScreen::LaunchMovie(gTUTORIAL_MOVIE_BOUNTY
#ifndef EA_BUILD_A124
                                                     ,
                                                     GetPackageName()
#endif
                    );
                }
                FEngSetScript(GetPackageName(), 0x99344537, 0x16a259, true);
                FEngSetInvisible(GetPackageName(), FEngHashString("TRACK_MAP"));
                career->SpecialFlags |= 0x400;
                return;
            }
            cFEng::Get()->QueueGameMessage(0xc3960eb9, GetPackageName(), 0xff);
            return;
        }
        case 0xc3960eb9:
#ifndef EA_BUILD_A124
            if (tutorialPlaying) {
                tutorialPlaying = false;
                FEngSetScript(GetPackageName(), 0x99344537, 0x1744b3, true);
                return;
            }
#endif
            if (bIsInGame) {
                FEngSetVisible("InGameBackground.fng", 0x2716cdbf);
                GManager::Get().WarpToMarker(theMarker, true);
                new ERaceSheetOff();
                return;
            }
            GManager::Get().OverrideFreeRoamStartMarker(theMarker);
            GManager::Get().QueueFreeRoamPursuit(0.0f);
            GManager::Get().QueueFreeRoamPursuit(0.0f);
            RaceStarter::StartCareerFreeRoam();
            return;
        case 0x911ab364:
            if (bIsInGame) {
                cFEng::Get()->QueuePackageSwitch("InGameReputationOverview.fng", 1, 0, false);
            } else {
                cFEng::Get()->QueuePackageSwitch("SafeHouseReputationOverview.fng", 0, 0, false);
            }
            return;
        case 0xc98356ba:
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
        case 0x1265ece9:
            return;
        case 0x34dc1bcf:
            return;
        case 0x72619778:
        case 0x911c0a4b:
        case 0x9120409e:
        case 0xb5971bf1:
            break;
        default:
            return;
    }
    int newIndex = data.IsInList(currentDatum) - 1;
    if (currentIndex != newIndex && currentDatum != nullptr) {
        RefreshTrack();
    }
}

#ifdef EA_BUILD_A124 // la A124 no limita las recompensas por bin ni marca las conseguidas al empezar
void uiRepSheetBounty::Setup() {
    ClearData();
    for (unsigned int i = 0; i < GManager::Get().GetNumBountySpawnMarkers(); i++) {
        int index = GManager::Get().GetBountySpawnMarkerTag(i);
        orderedList[index - 1] = static_cast<char>(index);
    }
    for (unsigned int i = 0; i < GManager::Get().GetNumBountySpawnMarkers(); i++) {
        AddDatum(new ("BountyDatum", 0) BountyDatum(FEDatabase->GetBountyIconHash(orderedList[i]),
                                                    FEDatabase->GetBountyHeaderHash(orderedList[i]), i));
    }
    SetDescLabel(0xb5117fde);
    SetInitialPosition(0);
    RefreshTrack();
    RefreshHeader();
}
#else
void uiRepSheetBounty::Setup() {
    ClearData();
    unsigned int bin = FEDatabase->GetCareerSettings()->GetCurrentBin();
    for (int i = 0; i < 9; i++) {
        unsigned int check_hash = FEngHashString("CHECK_%d", i + 1);
        FEngSetInvisible(GetPackageName(), check_hash);
    }
    for (unsigned int i = 0; i < GManager::Get().GetNumBountySpawnMarkers(); i++) {
        int index = GManager::Get().GetBountySpawnMarkerTag(i);
        orderedList[index - 1] = static_cast<char>(i);
    }
    for (unsigned int i = 0; i < GManager::Get().GetNumBountySpawnMarkers(); i++) {
        if (i < 4 || (i < 8 && bin <= 12) || bin <= 8) {
            AddDatum(new ("BountyDatum", 0) BountyDatum(FEDatabase->GetBountyIconHash(i + 1),
                                                        FEDatabase->GetBountyHeaderHash(i + 1),
                                                        static_cast<int>(static_cast<signed char>(orderedList[i]))));
        }
    }
    SetDescLabel(0xb5117fde);
    SetInitialPosition(0);
    RefreshTrack();
    RefreshHeader();
}
#endif

void uiRepSheetBounty::RefreshTrack() {
    if (GetCurrentDatum() != nullptr) {
        bVector2 position;
        float rotation = 0.0f;
        // La A124 (0x23C0D8) llama a Init, ResetZoom, PanTo y ZoomTo con this+0xE8 y sin comprobar nada.
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
        BountyDatum *d = static_cast<BountyDatum *>(GetCurrentDatum());
        unsigned int key = GManager::Get().GetBountySpawnMarker(d->index);
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

void uiRepSheetBounty::RefreshHeader() {
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
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    int loc_tag = GManager::Get().GetBountySpawnMarkerTag(GetCurrentDatumNum());
#else
    int loc_tag = GManager::Get().GetBountySpawnMarkerTag(GetCurrentDatumNum() - 1);
#endif
    FEngSetTextureHash(GetPackageName(), 0xf97ec5d5, FEDatabase->GetBountyIconHash(loc_tag));
    BountyDatum *d = static_cast<BountyDatum *>(GetCurrentDatum());
#ifdef EA_BUILD_A124 // la A124 marca con medallas las recompensas bloqueadas y conseguidas
    if (d != nullptr) {
        FEngSetLanguageHash(GetPackageName(), 0x28049d6, FEDatabase->GetBountyDescHash(orderedList[d->index]));
        for (int i = 0; i < GetNumSlots(); i++) {
            ArrayDatum *datum = GetDatumAt(i + GetStartDatumNum());
            unsigned int check_hash = FEngHashString("MEDAL_THUMB_%d", i + 1);
            if (datum != nullptr) {
                if (datum->IsLocked()) {
                    FEngSetVisible(GetPackageName(), check_hash);
                    FEngSetTextureHash(GetPackageName(), check_hash, 0x18ed48);
                } else if (datum->IsChecked()) {
                    FEngSetVisible(GetPackageName(), check_hash);
                    FEngSetTextureHash(GetPackageName(), check_hash, 0x28feadd);
                } else {
                    FEngSetInvisible(GetPackageName(), check_hash);
                }
            }
        }
    }
#else
    if (d != nullptr) {
        if (d->IsLocked()) {
            cFEng::Get()->QueuePackageMessage(0xc5dd9d68, GetPackageName(), nullptr);
        } else {
            cFEng::Get()->QueuePackageMessage(0x38091fa1, GetPackageName(), nullptr);
        }
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
        FEngSetLanguageHash(GetPackageName(), 0x28049d6, FEDatabase->GetBountyDescHash(GetCurrentDatumNum() + 1));
#else
        FEngSetLanguageHash(GetPackageName(), 0x28049d6, FEDatabase->GetBountyDescHash(GetCurrentDatumNum()));
#endif
        for (int i = 0; i < GetNumSlots(); i++) {
            ArrayDatum *datum = GetDatumAt(i + GetStartDatumNum());
            unsigned int check_hash = FEngHashString("CHECK_%d", i + 1);
            FEngSetTextureHash(GetPackageName(), check_hash, 0x18ed48);
            if (datum != nullptr) {
                FEngSetInvisible(GetPackageName(), check_hash);
            }
        }
    }
#endif
}
