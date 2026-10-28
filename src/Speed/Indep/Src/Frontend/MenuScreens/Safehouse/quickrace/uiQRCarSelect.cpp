#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRCarSelect.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/Database/VehicleDB.hpp"
#include "Speed/Indep/Src/Frontend/Careers/UnlockSystem.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feWidget.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/MemCard/uiMemcardInterface.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/FEPkg_GarageMain.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRBrief.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiShowcase.hpp"
#include "Speed/Indep/Src/Physics/PhysicsInfo.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/frontend.h"
#include "Speed/Indep/Src/Frontend/RaceStarter.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#include "Speed/Indep/Src/Generated/FEngHash/FEHash_FeBusted.hpp"
#include "Speed/Indep/Src/Generated/FEngHash/FEHash_FeWorldMapQuickList.hpp"
#include "Speed/Indep/Src/Generated/FEngHash/FEHash_UI_DebugCarCustomize.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Online/LobbyUsers.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/uiMain.hpp"
#endif

extern int g_MaximumMaximumTimesBusted;







uint32 UIQRCarSelect::ForceCar = (int)0xFFFFFFFF;
bool QRCarSelectBustedManager::bPlayerJustGotBusted = false;











bool QRCarSelectBustedManager::bIsCross = false;
//

int CheatBustedCount = 0;
int CheatMaxBusted = 0;
bool CheatImpounded = false;
bool CheatCanAddImpoundBox = false;
bool CheatReleaseFromImpoundMarker = false;
bool CheatReleasable = false;
int gPlayerNum;
// gPlayerNum (.bss 0x804AB5AC) solo la usa este fichero. El bloque ocupa las
// mismas cinco lineas que la declaracion extern de antes: los FNEW de abajo
// llevan __LINE__ y sus numeros de linea son parte del codigo que se compara.
//

QRCarSelectBustedManager::QRCarSelectBustedManager(const char *pkg_name, int flags) {
    Flags = static_cast<eBustedAnimationTypes>(flags);
    ImpoundStampHash = 0;
    ParentPkg = pkg_name;
    bWantsImpound = false;
    WorkingCareerRecord = nullptr;
    WorkingCarRecord = nullptr;
}

QRCarSelectBustedManager::~QRCarSelectBustedManager() {
    if (ImpoundStampHash) {
        eUnloadStreamingTexture(ImpoundStampHash);
        ImpoundStampHash = 0;
    }
}

bool QRCarSelectBustedManager::IsImpoundInfoVisible() {
    return FEDatabase->IsCareerMode() && !FEDatabase->IsCarLotMode();
}

bool QRCarSelectBustedManager::ShowImpoundedTexture() {
    return WorkingCareerRecord->TheImpoundData.IsImpounded();
}

void QRCarSelectBustedManager::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    switch (msg) {
        case 0x8defa48b:
            TheFEMarkerManager.UtilizeMarker(FEMarkerManager::MARKER_ADD_IMPOUND_BOX, 0);
            WorkingCareerRecord->TheImpoundData.AddMaxBusted();
            RefreshHeader();
            return;
        case 0xa0fc39f9:
            WorkingCareerRecord->TheImpoundData.NotifyPlayerPaidToRelease();
            WorkingCareerRecord->SetVehicleHeat(0.0f);
            {
                float cost = WorkingCarRecord->GetReleaseFromImpoundCost();
                FEDatabase->GetCareerSettings()->SpendCash(bFloatToInt(cost));
            }
            RefreshHeader();
            return;
        case 0xe845bc1c:
            WorkingCareerRecord->TheImpoundData.NotifyPlayerUsedMarkerToRelease();
            WorkingCareerRecord->SetVehicleHeat(0.0f);
            TheFEMarkerManager.UtilizeMarker(FEMarkerManager::MARKER_IMPOUND_RELEASE, 0);
            RefreshHeader();
            return;
        case 0x406415e3:
            return;
        case 0xe0b38195:
            if (!CalcGameOver())
                return;
            FEDatabase->GetCareerSettings()->SetGameOver();
            DialogInterface::ShowOneButton(GetPackageName(), "GameOver.fng", dialog_alert, LANGUAGE_COMMON_OK, 0x3fdc64c1, 0x164bed94);
            return;
        case 0x3fdc64c1:
            FEManager::Get()->SetGarageType(static_cast<eGarageType>(1));
            FEDatabase->ClearGameMode(static_cast<eFEGameModes>(1));
            FEDatabase->SetGameMode(static_cast<eFEGameModes>(0x100));
            cFEng::Get()->QueuePackageSwitch("MainMenu_Sub.fng", 0, 0, false);
            return;
        default:
            return;
    }
}

void QRCarSelectBustedManager::TextureLoadedCallback() {
    if (ShowImpoundedTexture()) {
        FEngSetTextureHash(ParentPkg, 0xce18427d, ImpoundStampHash);
#ifdef EA_BUILD_A124
        FEngSetTextureHash(ParentPkg, 0xbc7b91f, ImpoundStampHash);
#else
        FEngSetTextureHash(ParentPkg, 0x5b8f2a45, ImpoundStampHash);
#endif
        bool animate = Flags == BUSTED_ANIM_SHOW_IMPOUNDED;
        if (animate) {
            FEngSetScript(ParentPkg, 0xbc7b91f, FEHASH_ANIMATE, true);
            Flags = BUSTED_ANIM_NOTHING;
        } else {
#ifdef EA_BUILD_A124
            FEngSetScript(ParentPkg, 0xbc7b91f, FEHASH_SHOW, true);
#else
            FEngSetScript(ParentPkg, 0xbc7b91f, FEHASH_NORMAL, true);
#endif
        }
    } else {
        FEngSetScript(ParentPkg, 0xbc7b91f, 0x16a259, true);
    }
}

void QRCarSelectBustedManager::LoadImpoundTexture() {
    switch (GetCurrentLanguage()) {
        case 1:
            ImpoundStampHash = 0xce184740;
            break;
        case 2:
            ImpoundStampHash = 0xce1849e1;
            break;
        case 3:
            ImpoundStampHash = 0xce185441;
            break;
        case 4:
            ImpoundStampHash = 0xce187e47;
            break;
        case 5:
            ImpoundStampHash = 0xce183f30;
            break;
        case 6:
            ImpoundStampHash = 0xce187f32;
            break;
        case 7:
            ImpoundStampHash = 0xce183c96;
            break;
        case 12:
            ImpoundStampHash = 0xce18716e;
            break;
        case 13:
            ImpoundStampHash = 0xce184620;
            break;
#ifndef EA_BUILD_A124
        case 8:
            ImpoundStampHash = 0xce185c2f;
            break;
        case 9:
            ImpoundStampHash = 0xce183937;
            break;
        case 10:
            ImpoundStampHash = 0xce18561e;
            break;
        case 11:
            ImpoundStampHash = 0xce188180;
            break;
#endif
        default:
            ImpoundStampHash = 0xce18427d;
            break;
    }

#ifdef EA_BUILD_A124
    eLoadStreamingTexture(ImpoundStampHash, TextureLoadedCallbackAccessor, reinterpret_cast<uint32>(this), 0);
#else
    uint32 hash = ImpoundStampHash;
    eLoadStreamingTexture(&hash, 1, TextureLoadedCallbackAccessor, reinterpret_cast<uintptr_t>(this), 0);
#endif
}

void QRCarSelectBustedManager::SetSelectedCar(FECarRecord *record) {
    WorkingCarRecord = record;
    WorkingCareerRecord = FEDatabase->GetPlayerCarStable(0)->GetCareerRecordByHandle(record->CareerHandle);
    if (CheatImpounded != 0) {
        WorkingCareerRecord->TheImpoundData.MaxBusted = static_cast<unsigned char>(CheatMaxBusted);
        WorkingCareerRecord->TheImpoundData.TimesBusted = static_cast<char>(CheatMaxBusted);
        WorkingCareerRecord->TheImpoundData.BecomeImpounded(FEImpoundData::IMPOUND_REASON_STRIKE_LIMIT_REACHED);
        Flags = BUSTED_ANIM_SHOW_IMPOUNDED;
    } else if (CheatBustedCount != 0) {
        WorkingCareerRecord->TheImpoundData.TimesBusted = static_cast<char>(CheatBustedCount);
        WorkingCareerRecord->TheImpoundData.MaxBusted = static_cast<unsigned char>(CheatMaxBusted);
        Flags = BUSTED_ANIM_SHOW_STRIKE;
    }
    if (CheatReleasable != 0) {
        bool released;
        do {
            released = WorkingCareerRecord->TheImpoundData.NotifyWin();
        } while (!released);
    }
    RefreshHeader();
}

void QRCarSelectBustedManager::RefreshHeader() {
    if (!IsImpoundInfoVisible())
        return;

    bool bNotImpounded = false;
    if (ShowImpoundedTexture()) {
        TextureInfo *texInfo = GetTextureInfo(ImpoundStampHash, 0, 0);
        if (texInfo) {
#ifdef EA_BUILD_A124
            FEngSetScript(GetPackageName(), 0xbc7b91f, FEHASH_ANIMATE, true);
#else
            FEngSetScript(GetPackageName(), 0xbc7b91f, FEHASH_NORMAL, true);
            FEngSetScript(GetPackageName(), 0x64f3a49c, FEHASH_APPEAR, true);
            FEngSetTextureHash(GetPackageName(), 0xce18427d, ImpoundStampHash);
            FEngSetTextureHash(GetPackageName(), 0x5b8f2a45, ImpoundStampHash);
#endif
        }
        float cost = WorkingCarRecord->GetReleaseFromImpoundCost();
        bool has_cash = FEDatabase->GetCareerSettings()->GetCash() >= bFloatToInt(cost);
        bool has_marker = TheFEMarkerManager.HasMarker(FEMarkerManager::MARKER_IMPOUND_RELEASE, 0);
        if (WorkingCareerRecord->TheImpoundData.IsReleasable() && has_cash) {
            FEngSetLanguageHash(GetPackageName(), 0xb94139f4, 0x281dee8a);
        } else if (has_marker) {
            FEngSetLanguageHash(GetPackageName(), 0xb94139f4, 0xf9c73cc2);
        } else {
            FEngSetLanguageHash(GetPackageName(), 0xb94139f4, 0x2b65a216);
        }
    } else {
#ifdef EA_BUILD_A124
        FEngSetScript(GetPackageName(), 0xbc7b91f, 0x16a259, true);
#else
        FEngSetLanguageHash(GetPackageName(), 0xb94139f4, 0x2b65a216);
        FEngSetScript(GetPackageName(), 0x64f3a49c, 0x16a259, true);
#endif
    }
    if (WorkingCareerRecord->GetTimesBusted() >= 0) {
        FEngSetVisible(GetPackageName(), 0x75721326);
        int posIndex = 1;
        unsigned int script2 = 0x16a259;
        unsigned int script1 = 0x16a259;
        switch (WorkingCareerRecord->TheImpoundData.MaxBusted) {
            case 4:
                posIndex = 2;
                script2 = 0x1ca7c0;
                break;
            case 5:
                posIndex = 3;
                script2 = 0x1ca7c0;
                script1 = 0x1ca7c0;
                break;
        }
        FEngSetScript(GetPackageName(), 0x5bc78037, script2, true);
        FEngSetScript(GetPackageName(), 0x48095518, script1, true);
        FEngSetScript(GetPackageName(), 0xf9a5ce86, FEngHashString("POS%d", posIndex), true);
        FEngSetScript(GetPackageName(), 0xebf0016e, FEngHashString("POS%d", posIndex), true);
        if (ShowNewStrikeAnimation()) {
            FEngSetScript(GetPackageName(), FEngHashString("IMPOUND_STATE_%d", WorkingCareerRecord->GetTimesBusted()), FEHASH_ANIMATE, true);
            Flags = BUSTED_ANIM_NOTHING;
        }
        for (int i = 1; i <= static_cast<int>(static_cast<unsigned char>(WorkingCareerRecord->TheImpoundData.MaxBusted)); i++) {
            if (i <= WorkingCareerRecord->GetTimesBusted()) {
                if (!FEngIsScriptSet(GetPackageName(), FEngHashString("IMPOUND_STATE_%d", i), FEHASH_ANIMATE)) {
                    FEngSetScript(GetPackageName(), FEngHashString("IMPOUND_STATE_%d", i), 0x1ca7c0, true);
                }
            } else {
                FEngSetScript(GetPackageName(), FEngHashString("IMPOUND_STATE_%d", i), 0x16a259, true);
            }
        }
    } else {
        if (WorkingCareerRecord->TheImpoundData.IsReleasable()) {
            FEngSetInvisible(GetPackageName(), 0x75721326);
        } else if (WorkingCareerRecord->TheImpoundData.IsImpounded()) {
            FEngSetInvisible(GetPackageName(), 0x75721326);
        } else {
            bNotImpounded = true;
        }
    }
    if (bNotImpounded) {
        FEngSetScript(GetPackageName(), 0xbc7b91f, 0x16a259, true);
        FEngSetInvisible(GetPackageName(), 0x75721326);
        FEngSetLanguageHash(GetPackageName(), 0xb94139f4, 0x2b65a216);
    }
}

bool QRCarSelectBustedManager::CalcGameOver() {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    bool game_over = false;
    if (stable->GetNumAvailableCareerCars() < 1) {
        if (!TheFEMarkerManager.HasMarker(FEMarkerManager::MARKER_IMPOUND_RELEASE, 0)) {
            game_over = true;
        }
    }
    return game_over;
}

void QRCarSelectBustedManager::MaybeReleaseCar() {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    FECareerRecord *record = stable->GetCareerRecordByHandle(WorkingCarRecord->CareerHandle);
    float costf = WorkingCarRecord->GetReleaseFromImpoundCost();
    int cost = bFloatToInt(costf);
    bool has_cash = FEDatabase->GetCareerSettings()->GetCash() >= cost;
    bool has_marker = TheFEMarkerManager.GetNumMarkers(FEMarkerManager::MARKER_IMPOUND_RELEASE, 0) > 0;

    if (record->TheImpoundData.IsReleasable() && has_cash) {
        if (has_marker) {
#ifdef EA_BUILD_A124 // la A124 abre el dialogo en el primer boton
            DialogInterface::ShowThreeButtons(GetPackageName(), "", dialog_confirmation, 0xf9c73cc2, 0x4eb9591f, LANGUAGE_COMMON_CANCEL, 0xe845bc1c, 0xa0fc39f9,
                                              0x5ee58948, 0x5ee58948, first_dialog_button1, 0xb715ae8f, cost);
#else
            DialogInterface::ShowThreeButtons(GetPackageName(), "", dialog_confirmation, 0xf9c73cc2, 0x4eb9591f, LANGUAGE_COMMON_CANCEL, 0xe845bc1c, 0xa0fc39f9,
                                              0x5ee58948, 0x5ee58948, first_dialog_button2, 0xb715ae8f, cost);
#endif
        } else {
            DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_confirmation, LANGUAGE_COMMON_OK, LANGUAGE_COMMON_CANCEL, 0xa0fc39f9, 0x5ee58948, first_dialog_button1,
                                            0xcad5722e, cost);
        }
    } else if (has_marker) {
        DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_confirmation, LANGUAGE_COMMON_OK, LANGUAGE_COMMON_CANCEL, 0xe845bc1c, 0x5ee58948, first_dialog_button1,
                                        0xed4dd591, bFloatToInt(costf));
    } else {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x5ee58948, 0xe96fa0c5);
    }





}

void QRCarSelectBustedManager::MaybeAddImpoundBox() {
    bool can_add_impound_box = false;
    if (WorkingCareerRecord->TheImpoundData.CanAddMaxBusted()) {
        can_add_impound_box = true;
    }
    can_add_impound_box = can_add_impound_box && TheFEMarkerManager.HasMarker(FEMarkerManager::MARKER_ADD_IMPOUND_BOX, 0);
    // La forma del original: la variable can_add_impound_box absorbe el cheat (el original no tiene la local
    // showDialog), la alpha usa el hash 0x1fab5998 y no tiene la rama ShowOneButton (maibB, medidas r90/r94).
    can_add_impound_box = can_add_impound_box || CheatCanAddImpoundBox != 0;
    if (can_add_impound_box) {
        DialogInterface::ShowTwoButtons(GetPackageName(), "", dialog_confirmation, LANGUAGE_COMMON_OK, LANGUAGE_COMMON_CANCEL, 0x8defa48b,
#ifdef EA_BUILD_A124
                                        0x1fab5998,
#else
                                        0xb4edeb6d,
#endif
                                        first_dialog_button2, 0xcebda20);
#ifndef EA_BUILD_A124
    } else if (static_cast<int>(WorkingCareerRecord->TheImpoundData.MaxBusted) >= g_MaximumMaximumTimesBusted) {
        DialogInterface::ShowOneButton(GetPackageName(), "", dialog_info, LANGUAGE_COMMON_OK, 0xb4edeb6d, 0xbcae8539);
#endif
    }
}

UIQRCarSelect::UIQRCarSelect(ScreenConstructorData *sd)
    : MenuScreen(sd), pManuLogo(nullptr), pCarBadge(nullptr), pCarName(nullptr), pCarNameShadow(nullptr),
      pFilter(nullptr), originalCar(0xFFFFFFFF),
      TheBustedManager(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), (sd->Arg >> 8) & 0xFF),
      bLoadingBarActive(false), bShowcaseMode(false) {
    iPlayerNum = sd->Arg & 0xFF;
    filter = 0;
    iPrevButtonMsg = 0;
    tLastEventTimer.ResetLow();

    const u32 FEObj_ManufacturerLogo = 0x3e01ad1d;
    pManuLogo = FEngFindImage(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), FEObj_ManufacturerLogo);
    const u32 FEObj_CarBadge = 0xb05dd708;
    pCarBadge = FEngFindImage(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), FEObj_CarBadge);
    const u32 FEObj_CARNAME = __CARNAME__;
    pCarName = FEngFindString(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), FEObj_CARNAME);
    const u32 FEObj_CARNAME_SHADOW = 0x79d6e45c;
    pCarNameShadow = FEngFindString(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), FEObj_CARNAME_SHADOW);
    const u32 FEObj_FILTER = __FILTER__;
    pFilter = FEngFindString(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), FEObj_FILTER);

    if (FEDatabase->IsSplitScreenMode()) {
        const u32 FEObj_PLAYER_TEXT_GROUP = 0xe3fe27fe;
        if (iPlayerNum == 0) {
#ifdef EA_BUILD_A124 // la A124 no lleva gPlayerNum y borra el perfil del segundo jugador en el destructor
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), FEObj_PLAYER_TEXT_GROUP, LANGUAGE_COMMON_PLAYER_1);
#else
            gPlayerNum = iPlayerNum;
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), FEObj_PLAYER_TEXT_GROUP, LANGUAGE_COMMON_PLAYER_1);
            FEDatabase->DeleteMultiplayerProfile(1);
#endif
        } else {
#ifndef EA_BUILD_A124
            gPlayerNum = 1;
#endif
            FEDatabase->CreateMultiplayerProfile(1);
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), FEObj_PLAYER_TEXT_GROUP, LANGUAGE_COMMON_PLAYER_2);
        }
#ifndef EA_BUILD_A124
    } else {
        gPlayerNum = 0;
#endif
    }

    if (!FEDatabase->IsCarLotMode()) {
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x19398802);
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x1930b057);
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x20d113dc);
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x20c83c31);
    }

#ifdef EA_BUILD_A124 // sin LAN y con un solo objeto que ocultar
    if (FEDatabase->IsOnlineMode()) {
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xe9ed0a2);
    }
#else
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xe9ed0a2);
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x18a4384f);
    }
#endif

    if (TheBustedManager.IsImpoundInfoVisible()) {
        TheBustedManager.LoadImpoundTexture();
    }

    InitStatsSliders();
    SetAsGarageScreen();
    Setup();
#ifndef EA_BUILD_A124 // la A124 no tiene el empujon de camara del garaje
    if (!FEDatabase->IsCareerMode() || !FEDatabase->IsCarLotMode()) {
        GarageMainScreen::GetInstance()->CancelCameraPush();
    }
#endif
}

UIQRCarSelect::~UIQRCarSelect() {
#ifdef EA_BUILD_A124 // la A124 borra aqui el perfil del segundo jugador de la pantalla partida
    if (FEDatabase->IsSplitScreenMode() && iPlayerNum == 1) {
        FEDatabase->DeleteMultiplayerProfile(1);
    }
#endif
}

bool UIQRCarSelect::IsCarImpounded(uint32 handle) {
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    FECarRecord *car = stable->GetCarRecordByHandle(handle);
    FECareerRecord *career = stable->GetCareerRecordByHandle(car->CareerHandle);
#ifndef EA_BUILD_A124 // la A124 no comprueba el historial
    if (!career) {
        return false;
    }
#endif
    return career->TheImpoundData.IsImpounded();
}

void UIQRCarSelect::CommitChangeStartRace(bool allowError) {
    FEManager::Get()->AllowControllerError(allowError);
    FEDatabase->DeleteMultiplayerProfile(1);
    RaceStarter::StartRace();
}

void UIQRCarSelect::NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) {
    if (TheBustedManager.IsImpoundInfoVisible()) {
        TheBustedManager.NotificationMessage(msg, pobj, param1, param2);
    }

    switch (msg) {
        case 0x1265ece9: {
#ifndef EA_BUILD_A124 // la A124 mueve la camara en todos los modos
            if (!FEDatabase->IsCareerMode())
                return;
            if (!FEDatabase->IsCarLotMode())
                return;
#endif
            GarageMainScreen::GetInstance()->UpdateCurrentCameraView(false);
            return;
        }
        case 0x35f8620b: {
            if (!FEDatabase->IsSplitScreenMode())
                return;
            cFEng::Get()->QueuePackageMessage(0x841d518a, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
            return;
        }
        case 0xc98356ba: {
            if (GarageMainScreen::GetInstance()->IsCarRendering() && bLoadingBarActive) {
                cFEng::Get()->QueuePackageMessage(0x913fa282, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
                bLoadingBarActive = false;
            }
            if (!tLastEventTimer.IsSet())
                return;
            float elapsed = (RealTimer - tLastEventTimer).GetSeconds();
            if (elapsed >= 0.25f) {
#ifdef EA_BUILD_A124 // la A124 cancela la carga si no hay coche elegido
                if (!pSelectedCar) {
                    CarViewer::CancelCarLoad(static_cast<eCarViewerWhichCar>(0));
                    cFEng::Get()->QueuePackageMessage(0x913fa282, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
                    bLoadingBarActive = false;
                    return;
                }
#endif
                RideInfo ride;
                FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(iPlayerNum);
                stable->BuildRideForPlayer(pSelectedCar->mHandle, iPlayerNum, &ride);
                CarViewer::SetRideInfo(&ride, static_cast<eSetRideInfoReasons>(1), static_cast<eCarViewerWhichCar>(0));
                tLastEventTimer.UnSet();
            }
            return;
        }
        case 0x9120409e:
            ScrollCars(eSD_PREV);
            return;
        case 0xb5971bf1:
            ScrollCars(eSD_NEXT);
            return;
        case 0x72619778:
            ScrollLists(eSD_PREV);
            return;
        case 0x911c0a4b:
            ScrollLists(eSD_NEXT);
            return;
        case 0xc519bfbf: {
            if (!pSelectedCar)
                return;
            if (pSelectedCar->bLocked != 0)
                return;
            if (FEDatabase->IsOnlineMode())
                return;
#ifndef EA_BUILD_A124 // la A124 no tiene LAN
            if (FEDatabase->IsLANMode())
                return;
#endif
            if (FEDatabase->IsCareerMode() && !FEDatabase->IsCarLotMode()) {
                FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
                FECarRecord *car = GetSelectedCarRecord();
                FECareerRecord *career = stable->GetCareerRecordByHandle(car->CareerHandle);
                if (career) {
                    bool impounded = career->TheImpoundData.IsImpounded();
                    if (impounded || career->TheImpoundData.IsReleasable())
                        return;
                }
            }
            cFEng::Get()->QueuePackageMessage(0x89d0649c, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
            bShowcaseMode = true;
            return;
        }
        case 0xa46253ba: {
            FECarRecord *car = GetSelectedCarRecord();
            CareerSettings *settings = FEDatabase->GetCareerSettings();
            unsigned int cost = car->GetCost();
            settings->AwardCash(static_cast<int>(cost >> 1));
            FEPlayerCarDB *stable2 = FEDatabase->GetPlayerCarStable(iPlayerNum);
            stable2->DeleteCareerCar(pSelectedCar->mHandle, true);
            unsigned int old_handle = pSelectedCar->mHandle;
            RefreshCarList();
            if (old_handle == originalCar) {
                SetupForPlayer(0);
                originalCar = FEDatabase->GetCareerSettings()->GetCurrentCar();
            }
            RefreshHeader();
            return;
        }
        case 0xc519bfc4: {
            if (!FEDatabase->IsCareerMode())
                return;
            if (FEDatabase->IsCarLotMode())
                return;
            if (FEDatabase->IsDDay())
                return;
            if (!pSelectedCar)
                return;
            FECarRecord *car = GetSelectedCarRecord();
            bool isCareer = car->CareerHandle != 0xff;
            if (isCareer) {
                FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(iPlayerNum);
                FECareerRecord *career = stable->GetCareerRecordByHandle(car->CareerHandle);
                if (career->TheImpoundData.IsImpounded()) {
                    DialogInterface::ShowOneButton(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x34dc1bcf,
                                                   0x80e4f27c);
                    return;
                }
            }
            FEPlayerCarDB *stable2 = FEDatabase->GetPlayerCarStable(iPlayerNum);
            if (stable2->GetNumAvailableCareerCars() < 2) {
                DialogInterface::ShowOneButton(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK, 0x34dc1bcf,
                                               0x9a772bd6);
                return;
            }
            char buf[512];
            char cost_str[16];
            bSNPrintf(cost_str, 0x10, "%$d", car->GetCost() >> 1);
            bSNPrintf(buf, 0x200, GetLocalizedString(0xb4a40135), cost_str);
            DialogInterface::ShowTwoButtons(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO,
                                            0xa46253ba, 0x34dc1bcf, 0x34dc1bcf, first_dialog_button2, buf);
            return;
        }
        case 0xc519bfc3: {
            if (FEDatabase->IsSplitScreenMode()) {
#ifdef EA_BUILD_A124
                unsigned int op = 0x413;
                if (iPlayerNum == 1) {
                    op = 0x20413;
                }
#else
                unsigned int op = 0x411;
                if (iPlayerNum == 1) {
                    op = 0x20411;
                }
#endif
                MemcardEnter(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), reinterpret_cast<MenuScreen *>(this)->GetPackageName(), op,
                             nullptr, nullptr, 0x7e998e5e, 0x8867412d);
                return;
            }
            if (!TheBustedManager.IsImpoundInfoVisible())
                return;
            TheBustedManager.MaybeAddImpoundBox();
            return;
        }
        case 0xe1fde1d1: {
            if (bShowcaseMode) {
                unsigned int handle = pSelectedCar->mHandle;
                FECarRecord *car = FEDatabase->GetPlayerCarStable(iPlayerNum)->GetCarRecordByHandle(handle);
#ifdef EA_BUILD_A124 // la A124 vuelve del showcase por Showcase::FromIndex
                Showcase::FromIndex = pSelectedCar->mHandle;
                Showcase::FromArgs = 0;
                Showcase::FromPackage = reinterpret_cast<MenuScreen *>(this)->GetPackageName();
                Showcase::FromCarHandle = 0xffffffff;
#else
                ForceCar = pSelectedCar->mHandle;
                Showcase::FromArgs = iPlayerNum;
                Showcase::FromPackage = reinterpret_cast<MenuScreen *>(this)->GetPackageName();
#endif
                cFEng::Get()->QueuePackageSwitch("Showcase.fng", reinterpret_cast<int>(car), 0, false);
                return;
            }
#ifndef EA_BUILD_A124
            ForceCar = 0xffffffff;
#endif
            if (iPrevButtonMsg == 0x406415e3) {
                if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
                    cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
                    return;
                }
                if (FEDatabase->IsCareerMode()) {
                    if (!FEDatabase->IsCarLotMode()) {
                        cFEng::Get()->QueuePackageSwitch("MainMenu_Sub.fng", 0, 0, false);
                        return;
                    }
                } else if (FEDatabase->IsCustomizeMode()) {
                    unsigned int handle = pSelectedCar->mHandle;
                FECarRecord *car = FEDatabase->GetPlayerCarStable(iPlayerNum)->GetCarRecordByHandle(handle);
                    if (!car->IsCustomized()) {
                        car = FEDatabase->GetPlayerCarStable(iPlayerNum)->CreateNewCustomCar(car->Handle);
                    }
                    RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
                    settings->SelectedCar[iPlayerNum] = car->Handle;
                    cFEng::Get()->QueuePackageSwitch("MyCarsManager.fng", 0, 0, false);
                    return;
                }
                if (FEDatabase->RaceMode != 2) {
                    if (FEDatabase->GetPlayerSettings(iPlayerNum)->TransmissionPromptOn != 0) {
                        goto choose_transmission;
                    }
                }
                if (FEDatabase->IsSplitScreenMode() && iPlayerNum != 1) {
                    return;
                }
#ifdef EA_BUILD_A124 // la A124 no borra el perfil del segundo jugador al empezar
                FEManager::Get()->AllowControllerError(true);
                RaceStarter::StartRace();
#else
                CommitChangeStartRace(true);
#endif
                return;
            } else if (iPrevButtonMsg == 0x911ab364) {
                unsigned int iVar3 = 0xFFFFFFFF;
                if (FEDatabase->IsCareerMode()) {
#ifdef EA_BUILD_A124 // la A124 devuelve el coche antes de mirar el concesionario
                    if (!IsCarImpounded(originalCar)) {
                        FEDatabase->GetCareerSettings()->SetCurrentCar(originalCar);
                    }
                    if (FEDatabase->IsCarLotMode()) {
                        RaceStarter::StartCareerFreeRoam();
                        return;
                    }
#else
                    if (FEDatabase->IsCarLotMode()) {
                        RaceStarter::StartCareerFreeRoam();
                        return;
                    }
                    if (!IsCarImpounded(originalCar)) {
                        FEDatabase->GetCareerSettings()->SetCurrentCar(originalCar);
                    }
#endif
                    cFEng::Get()->QueuePackageSwitch("MainMenu_Sub.fng", 0, 0, false);
                } else if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
                    RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
                    settings->SelectedCar[iPlayerNum] = originalCar;
                    cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
                } else {
                    bool allowControllerError = FEDatabase->IsCustomizeMode();
                    if (allowControllerError) {
                        cFEng::Get()->QueuePackageSwitch("MyCarsManager.fng", 0, 0, false);
                    } else {
                        bool isSplit = FEDatabase->IsSplitScreenMode();
                        if (isSplit) {
                            if (iPlayerNum == 1) {
                                FEDatabase->SetPlayersJoystickPort(1, -1);
                                cFEng::Get()->QueuePackageSwitch("PressStart.fng", true, 0xff, false);
                            } else {
                                FEManager::Get()->AllowControllerError(allowControllerError);
                                cFEng::Get()->QueuePackageSwitch("PressStart.fng", false, 0xff, false);
                            }
                        } else {
                            RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
                            settings->SelectedCar[iPlayerNum] = originalCar;
                            iVar3 = originalCar;
                            FEManager::Get()->AllowControllerError(isSplit);
                            cFEng::Get()->QueuePackageSwitch("Track_Options.fng", 0, 0, false);
                        }
                    }
                }
                if (iVar3 != 0xFFFFFFFF) {
                    RideInfo ride;
                    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(iPlayerNum);
                    stable->BuildRideForPlayer(iVar3, iPlayerNum, &ride);
                    CarViewer::SetRideInfo(&ride, static_cast<eSetRideInfoReasons>(1), static_cast<eCarViewerWhichCar>(0));
                }
                return;
            }
            return;
        }
        case 0xd05fc3a3: {
            FECarRecord *car = GetSelectedCarRecord();
            FECarRecord *new_car = FEDatabase->GetPlayerCarStable(0)->CreateNewCareerCar(car->Handle);
            if (new_car) {
                FEDatabase->GetCareerSettings()->SpendCash(new_car->GetCost());
                FEDatabase->GetCareerSettings()->SetCurrentCar(new_car->Handle);
            }
            RaceStarter::StartCareerFreeRoam();
            return;
        }
        case 0xb1ee867d: {
            FECarRecord *car = GetSelectedCarRecord();
            FECarRecord *new_car = FEDatabase->GetPlayerCarStable(0)->CreateNewCareerCar(car->Handle);
            if (new_car) {
                FEDatabase->GetCareerSettings()->SpendCash(new_car->GetCost());
            }
            RaceStarter::StartCareerFreeRoam();
            return;
        }
        case 0x0c407210:
        case 0x406415e3: {
            if (!pSelectedCar)
                return;
            if (pSelectedCar->bLocked != 0)
                return;
            float elapsed = (RealTimer - tLastEventTimer).GetSeconds();
            if (elapsed < 0.25f)
                return;
            if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
#ifndef EA_BUILD_A124
                OnlineActOnSelect();
#endif
                iPrevButtonMsg = 0x406415e3;
                ChooseTransmission();
                return;
            }
            if (FEDatabase->IsCareerMode()) {
                if (FEDatabase->IsCarLotMode()) {
#ifndef EA_BUILD_A124 // la A124 no lista las partidas viejas
                    if (MemoryCard::GetInstance()->IsListingOldSaveFiles())
                        return;
#endif
                    unsigned int cost = GetSelectedCarRecord()->GetCost();
                    if (cost > static_cast<unsigned int>(FEDatabase->GetCareerSettings()->GetCash())) {
                        DialogInterface::ShowOneButton(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK,
                                                       0x34dc1bcf, 0x40fa955d);
                        return;
                    }
                    FEPlayerCarDB *stable2 = FEDatabase->GetPlayerCarStable(iPlayerNum);
#ifdef EA_BUILD_A124
                    if (stable2->GetNumCareerCars() > 9) {
#else
                    if (stable2->GetNumPurchasedCars() > 9) {
#endif
                        DialogInterface::ShowOneButton(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_OK,
                                                       0x34dc1bcf, 0x41030a1b);
                        return;
                    }
                    if (FEDatabase->IsDDay()) {
                        DialogInterface::ShowTwoButtons(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "", dialog_alert, LANGUAGE_COMMON_YES,
                                                        LANGUAGE_COMMON_NO, 0xd05fc3a3, 0x34dc1bcf, 0x34dc1bcf, first_dialog_button2, 0x74317cbc);
                        return;
                    }
                    DialogInterface::ShowThreeButtons(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "", dialog_alert, 0x5b9d89d0,
                                                      0x889d822e, LANGUAGE_COMMON_CANCEL, 0xd05fc3a3, 0xb1ee867d, 0x34dc1bcf, 0x34dc1bcf, first_dialog_button3,
                                                      0x8c451eba);
                    return;
                }
                FEPlayerCarDB *stable2 = FEDatabase->GetPlayerCarStable(0);
                FECarRecord *car = stable2->GetCarRecordByHandle(pSelectedCar->mHandle);
                FECareerRecord *career = stable2->GetCareerRecordByHandle(car->CareerHandle);
                bool impounded = career->TheImpoundData.IsImpounded();
                if (impounded || career->TheImpoundData.IsReleasable()) {
                    TheBustedManager.MaybeReleaseCar();
                    return;
                }
                iPrevButtonMsg = 0x406415e3;
                cFEng::Get()->QueuePackageMessage(FEHASH_LEAVE_ACCEPT, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
                return;
            }
            if (FEDatabase->IsCustomizeMode()) {
                iPrevButtonMsg = 0x406415e3;
                cFEng::Get()->QueuePackageMessage(FEHASH_LEAVE_ACCEPT, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
                return;
            }
            if (FEDatabase->RaceMode != 2) {
                if (FEDatabase->GetPlayerSettings(iPlayerNum)->TransmissionPromptOn != 0) {
                    goto choose_transmission;
                }
            }
#ifdef EA_BUILD_A124 // la A124 mira antes la pantalla partida y ata el mando al jugador uno
            if (FEDatabase->IsSplitScreenMode() && iPlayerNum == 0) {
                cFEng::Get()->QueuePackageSwitch("PressStart.fng", true, 0xff, false);
                return;
            }
            char port = FEngMapJoyParamToJoyport(param1);
            FEDatabase->SetPlayersJoystickPort(0, port);
            iPrevButtonMsg = 0x406415e3;
            cFEng::Get()->QueuePackageMessage(FEHASH_LEAVE_ACCEPT, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
            return;
#else
            char port = FEngMapJoyParamToJoyport(param1);
            FEDatabase->SetPlayersJoystickPort(iPlayerNum, port);
            if (!FEDatabase->IsSplitScreenMode() || iPlayerNum != 0) {
                iPrevButtonMsg = 0x406415e3;
                cFEng::Get()->QueuePackageMessage(FEHASH_LEAVE_ACCEPT, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
                return;
            }
            cFEng::Get()->QueuePackageSwitch("PressStart.fng", true, 0xff, false);
            return;
#endif
        }
choose_transmission:
        ChooseTransmission();
        return;
        case 0x911ab364: {
            bool bShouldProceed = true;
            if (FEDatabase->IsCareerMode()) {
                if (FEDatabase->IsCarLotMode()) {
                    if (FEDatabase->IsDDay()) {
                        bShouldProceed = false;
                    }
                } else {
                    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
#ifdef EA_BUILD_A124 // la A124 mira el coche elegido y ofrece sacarlo del deposito
                    FECarRecord *car = stable->GetCarRecordByHandle(pSelectedCar->mHandle);
#else
                    FECarRecord *car = stable->GetCarRecordByHandle(originalCar);
#endif
                    FECareerRecord *career = stable->GetCareerRecordByHandle(car->CareerHandle);
                    bool impounded = career->TheImpoundData.IsImpounded();
                    if (impounded || career->TheImpoundData.IsReleasable()) {
#ifdef EA_BUILD_A124
                        TheBustedManager.MaybeReleaseCar();
#else
                        DialogInterface::ShowOk(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "", dialog_alert, 0x630931b6);
#endif
                        bShouldProceed = false;
                    }
                }
            } else if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
                RideInfo ride;
                FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(iPlayerNum);
                stable->BuildRideForPlayer(originalCar, iPlayerNum, &ride);
                CarViewer::SetRideInfo(&ride, static_cast<eSetRideInfoReasons>(1), static_cast<eCarViewerWhichCar>(0));
            }
            if (!bShouldProceed)
                return;
            iPrevButtonMsg = 0x911ab364;
            cFEng::Get()->QueuePackageMessage(FEHASH_LEAVE_BACK, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
            return;
        }
        case 0x1fab5998: {
            if (FEDatabase->IsCareerMode()) {
                cFEng::Get()->QueuePackageSwitch(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), iPlayerNum, 0, false);
                return;
            }
            if (!FEDatabase->IsQuickRaceMode()) {
                return;
            }
            bool isSplit = FEDatabase->IsSplitScreenMode();
            if (isSplit) {
                return;
            }
            FEManager::Get()->AllowControllerError(isSplit);
            return;
        }
        case 0x34dc1bcf:
            break;
        case 0x1a2826e1: {
            char port = FEngMapJoyParamToJoyport(param1);
            FEDatabase->SetPlayersJoystickPort(iPlayerNum, port);
            FEDatabase->GetPlayerSettings(iPlayerNum)->Transmission = 0;
#ifdef EA_BUILD_A124 // la A124 vuelve al menu online y no borra el perfil del segundo jugador
            if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
                LobbyUsers::Instance().UpdateCarName();
                cFEng::Get()->QueuePackageSwitch(gOnlineMainMenu, 0, 0, false);
                return;
            }
            if (FEDatabase->IsSplitScreenMode() && iPlayerNum == 0) {
                cFEng::Get()->QueuePackageSwitch("PressStart.fng", true, 0xff, false);
                return;
            }
            FEManager::Get()->AllowControllerError(false);
            RaceStarter::StartRace();
#else
            if (FEDatabase->IsSplitScreenMode() && iPlayerNum == 0) {
                cFEng::Get()->QueuePackageSwitch("PressStart.fng", true, 0xff, false);
                return;
            }
            CommitChangeStartRace(false);
#endif
            return;
        }
        case 0x5f5e3886: {
            char port = FEngMapJoyParamToJoyport(param1);
            FEDatabase->SetPlayersJoystickPort(iPlayerNum, port);
            FEDatabase->GetPlayerSettings(iPlayerNum)->Transmission = 1;
#ifdef EA_BUILD_A124 // la A124 vuelve al menu online y no borra el perfil del segundo jugador
            if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
                LobbyUsers::Instance().UpdateCarName();
                cFEng::Get()->QueuePackageSwitch(gOnlineMainMenu, 0, 0, false);
                return;
            }
            if (FEDatabase->IsSplitScreenMode() && iPlayerNum == 0) {
                cFEng::Get()->QueuePackageSwitch("PressStart.fng", true, 0xff, false);
                return;
            }
            FEManager::Get()->AllowControllerError(false);
            RaceStarter::StartRace();
#else
            if (FEDatabase->IsSplitScreenMode() && iPlayerNum == 0) {
                cFEng::Get()->QueuePackageSwitch("PressStart.fng", true, 0xff, false);
                return;
            }
            CommitChangeStartRace(false);
#endif
            return;
        }
        case 0x7e998e5e:
#ifdef EA_BUILD_A124 // la A124 solo refresca la cabecera al volver de la tarjeta
            RefreshHeader();
#else
            filter = 0xf0001;
            RefreshCarList();
            RefreshHeader();
            cFEng::Get()->QueuePackageMessage(FEHashUpper("ENABLE_INPUTS"), reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
#endif
            return;
#ifndef EA_BUILD_A124 // la A124 no refresca la cabecera al liberar el coche
        case 0x8defa48b:
        case 0xa0fc39f9:
        case 0xe845bc1c:
            RefreshHeader();
            return;
#endif
    }
    return;
}

eMenuSoundTriggers UIQRCarSelect::NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) {
    if (msg == 0x72619778 || msg == 0x911c0a4b || msg == 0xb205316c || msg == 0x480df13f) {
        if (FEDatabase->IsCustomizeMode() || FEDatabase->IsCarLotMode() || FEDatabase->IsCareerMode()) {
            return static_cast<eMenuSoundTriggers>(-1);
        }
    }
    return maybe;
}

void UIQRCarSelect::Setup() {
    if (FEDatabase->IsCarLotMode()) {
        if (FEDatabase->IsDDay()) {
            const u32 FEObj_firsttime = 0x3a12d2f5;
            cFEng::Get()->QueuePackageMessage(FEObj_firsttime, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
        } else {
            const u32 FEObj_carlot = 0x5415e304;
            cFEng::Get()->QueuePackageMessage(FEObj_carlot, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
        }
    } else if (FEDatabase->IsQuickRaceMode()) {
        if (FEDatabase->IsSplitScreenMode()) {
            const u32 FEObj_2players = 0x2cf6c390;
            cFEng::Get()->QueuePackageMessage(FEObj_2players, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
        } else {
            const u32 FEObj_quickrace = 0xde511657;
            cFEng::Get()->QueuePackageMessage(FEObj_quickrace, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
        }
#ifdef EA_BUILD_A124 // la A124 no tiene LAN
    } else if (FEDatabase->IsOnlineMode()) {
#else
    } else if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
#endif
        const u32 FEObj_online = 0x70fbb1e4;
        cFEng::Get()->QueuePackageMessage(FEObj_online, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
    } else if (FEDatabase->IsCustomizeMode()) {
        const u32 FEObj_customize = FEHASH_CUSTOMIZE;
        cFEng::Get()->QueuePackageMessage(FEObj_customize, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
    } else if (FEDatabase->IsCareerMode()) {
        const u32 FEObj_career = 0x5415c3f1;
        cFEng::Get()->QueuePackageMessage(FEObj_career, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
    }

    FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x64f6d21f);

    if (FEDatabase->IsCareerMode()) {
        originalCar = FEDatabase->GetCareerSettings()->GetCurrentCar();
        if (FEDatabase->IsCarLotMode()) {
            filter = 0xf0001;
            UserProfile *profile = FEDatabase->GetUserProfile(0);
            if (!profile->GetCareer()->HasCashBonusBeenAwarded()) {
#ifndef EA_BUILD_A124 // la A124 no bloquea la entrada mientras lista las partidas viejas
                cFEng::Get()->QueuePackageMessage(FEHashUpper("DISABLE_INPUTS"), reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
#endif
                MemoryCard::GetInstance()->StartListingOldSaveFiles();
            }
        } else {
            filter = 0xf0002;
        }
    } else {
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb));
        originalCar = settings->GetSelectedCar(iPlayerNum);
#ifdef EA_BUILD_A124 // la A124 recupera el filtro que cada jugador dejo en la carrera rapida
        if (FEDatabase->IsCustomizeMode()) {
            filter = 0xf0001;
        } else {
            filter = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->CarSelectFilterBits[iPlayerNum];
        }
#else
        if (!FEDatabase->IsCustomizeMode() && originalCar != 0x12345678) {
            unsigned int m3gtrHash = FEHashUpper("M3GTRCAREERSTART");
            if (originalCar != m3gtrHash) {
                FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(iPlayerNum);
                FECarRecord *car = stable->GetCarRecordByHandle(originalCar);
                if (car) {
                    filter = (car->FilterBits & 0x3f) | 0xf0000;
                    goto init_list_handles;
                }
            }
        }
        filter = 0xf0001;
#endif
    }

init_list_handles:
    int i = 0;
    do {
        ListHandles[i] = 0xFFFFFFFF;
        i++;
    } while (i < 6);

    RefreshCarList();
    RefreshHeader();
}

void UIQRCarSelect::InitStatsSliders() {
    TheHeatMeter.Init(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "", 1.0f, 5.0f, 1.0f, 1.0f);
    AccelerationSlider.Init(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "ACCELERATION", 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 160.0f);
    TopSpeedSlider.Init(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "TOPSPEED", 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 160.0f);
    HandlingSlider.Init(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "HANDLING", 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 160.0f);
}

void UIQRCarSelect::UpdateSliders() {
    Physics::Info::Performance perf1;
    Physics::Info::Performance perf2;

    if (pSelectedCar != nullptr) {
#ifdef EA_BUILD_A124 // la A124 siempre mira la cuadra del jugador uno
        FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
#else
        FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(iPlayerNum);
#endif
        if (stable != nullptr) {
            FECarRecord *car = stable->GetCarRecordByHandle(pSelectedCar->mHandle);
            if (car != nullptr) {
                Attrib::Gen::pvehicle pveh(car->VehicleKey, 0, nullptr);
                bool hasCustomization = (car->Customization != 0xff);
                if (hasCustomization) {
                    FECustomizationRecord *cust = stable->GetCustomizationRecordByHandle(car->Customization);
                    cust->WriteRecordIntoPhysics(pveh);
                }
                Physics::Info::EstimatePerformance(pveh, perf1);
            }
            if (FEDatabase->IsCareerMode()) {
                car = stable->GetCarRecordByHandle(originalCar);
            }
            if (car != nullptr) {
                Attrib::Gen::pvehicle pveh2(car->VehicleKey, 0, nullptr);
                bool hasCustomization = (car->Customization != 0xff);
                if (hasCustomization) {
                    FECustomizationRecord *cust = stable->GetCustomizationRecordByHandle(car->Customization);
                    cust->WriteRecordIntoPhysics(pveh2);
                }
                Physics::Info::EstimatePerformance(pveh2, perf2);
            }
        }
    }

    if (FEDatabase->IsDDay()) {
        perf2 = perf1;
    }

    AccelerationSlider.SetValue(perf1.Acceleration);
    AccelerationSlider.SetPreviewValue(perf2.Acceleration);
    AccelerationSlider.Draw();

    TopSpeedSlider.SetValue(perf1.TopSpeed);
    TopSpeedSlider.SetPreviewValue(perf2.TopSpeed);
    TopSpeedSlider.Draw();

    HandlingSlider.SetValue(perf1.Handling);
    HandlingSlider.SetPreviewValue(perf2.Handling);
    HandlingSlider.Draw();
}

int UIQRCarSelect::GetFilterType() {
    switch (static_cast<unsigned int>(filter) & 0xFFFF) {
        case 1:
            return 0;
        case 2:
            return 1;
        case 4:
            return 2;
        case 8:
            return 3;
        case 0x10:
            return 4;
        case 0x20:
            return 5;
        default:
            return 0;
    }
}

void UIQRCarSelect::SetupForPlayer(int player) {
    SelectableCar *car = nullptr;
    for (SelectableCar *sc = FilteredCarsList.GetHead(); sc != FilteredCarsList.EndOfList(); sc = static_cast<SelectableCar *>(reinterpret_cast<bTNode<SelectableCar> *>(sc)->GetNext())) {
#ifdef EA_BUILD_A124 // la A124 vuelve del showcase por Showcase::FromIndex y no fuerza coche
        if (sc->mHandle == ListHandles[GetFilterType()]) {
            car = sc;
            break;
        }
        if (Showcase::FromIndex != 0 && sc->mHandle == Showcase::FromIndex) {
            car = sc;
            Showcase::FromIndex = 0;
            break;
        }
        if (FEDatabase->IsCareerMode()) {
            if (sc->mHandle == FEDatabase->GetCareerSettings()->GetCurrentCar()) {
                car = sc;
                break;
            }
        } else {
            if (sc->mHandle == FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->GetSelectedCar(iPlayerNum)) {
                car = sc;
                break;
            }
        }
#else
        if (ForceCar != 0xFFFFFFFF) {
            if (sc->mHandle == ForceCar) {
                car = sc;
                break;
            }
        } else {
            if (sc->mHandle == ListHandles[GetFilterType()]) {
                car = sc;
                break;
            }
            if (FEDatabase->IsCareerMode()) {
                if (sc->mHandle == FEDatabase->GetCareerSettings()->GetCurrentCar()) {
                    car = sc;
                    break;
                }
            } else {
                if (sc->mHandle == FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->GetSelectedCar(iPlayerNum)) {
                    car = sc;
                    break;
                }
            }
        }
#endif
    }
#ifndef EA_BUILD_A124
    if (FEDatabase->IsCarLotMode() && ForceCar == 0xFFFFFFFF) {
        CarViewer::CancelCarLoad(eCARVIEWER_PLAYER1_CAR);
        car = FilteredCarsList.GetHead();
        originalCar = car->mHandle;
    }
    ForceCar = 0xFFFFFFFF;
#endif
    if (car == nullptr && !FilteredCarsList.IsEmpty()) {
        car = FilteredCarsList.GetHead();
    }
    SetSelectedCar(car, iPlayerNum);
    RefreshHeader();
}

int UIQRCarSelect::GetBonusUnlockText(FECarRecord *fe_car) {
    switch (fe_car->Handle) {
        case 0x965fu: case 0x9660u: case 0x9661u: case 0x9662u: case 0x9663u: case 0x9664u: case 0x9665u: case 0x9666u:
        case 0x13624eu: case 0x13624fu: case 0x136250u: case 0x136251u: case 0x136252u: case 0x136253u:
            return 0x4ef2a115;
        case 0x2cf370f0u:
            return 0xbd8bac94;
        case 0x03a94520u:
            return 0xbd8bac93;
        case 0xcb6aaf2fu:
            return 0xbd8bac91;
        case 0x2cf385b2u:
            return 0xbd8bac92;
    }
    return 0;
}

int UIQRCarSelect::GetBonusUnlockBinNumber(FECarRecord *fe_car) {
    unsigned int handle = fe_car->Handle;
    switch (handle) {
        case 0x965F:
            return 2;
        case 0x9660:
            return 3;
        case 0x9661:
            return 4;
        case 0x9662:
            return 5;
        case 0x9663:
            return 6;
        case 0x9664:
            return 7;
        case 0x9665:
            return 8;
        case 0x9666:
            return 9;
        case 0x13624E:
            return 10;
        case 0x13624F:
            return 11;
        case 0x136250:
            return 12;
        case 0x136251:
            return 13;
        case 0x136252:
            return 14;
        case 0x136253:
            return 15;
        default:
            return -1;
    }
}

void UIQRCarSelect::RefreshHeader() {
    UpdateSliders();

    unsigned int langhash;
    unsigned int texhash;
    unsigned int list = filter & 0xffff;
    switch (list) {
        case 1:
            langhash = LANGUAGE_CARLIST_STOCK;
            texhash = 0x3a541f7f;
            break;
        case 2:
            langhash = LANGUAGE_CARLIST_CAREER;
            texhash = 0xf0bddecd;
            break;
        case 4:
            langhash = LANGUAGE_CARLIST_QUICKRACE;
            texhash = 0x9996ca1e;
            break;
        case 8:
            langhash = LANGUAGE_CARLIST_BONUS;
            texhash = 0xbe5ad8a2;
            break;
        case 0x10:
            langhash = LANGUAGE_CARLIST_PRESET;
            texhash = 0x03704f3d;
            break;
        case 0x20:
            langhash = 0x3ec63978;
            texhash = 0x03704f3d;
            break;
        default:
            langhash = 0;
            texhash = 0;
            break;
    }

    if (FEDatabase->IsCarLotMode() || !FEDatabase->IsCareerMode()) {
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x39dc21f9);
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xe998fe99);
    }

    FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xaa9834bc, langhash);
    FEngSetTextureHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xe3b271b8, texhash);
    FEngSetScript(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xd0f7c7cc, 0x16a259, true);

    if (!pSelectedCar) {
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x7379349b);
    } else if (pSelectedCar->bLocked) {
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x7379349b);
    }

    if (!pSelectedCar) {
#ifdef EA_BUILD_A124 // la A124 vacia los textos del coche y pone los logos genericos
        FEPrintf(pCarName, "");
        FEPrintf(pCarNameShadow, "");
        FEngSetTextureHash(pManuLogo, FEHashUpper("GENERIC_LOGO_128"));
        FEngSetTextureHash(pCarBadge, FEHashUpper("GENERIC_LOGO_256"));
        FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), __NUMBER__, "0");
        FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), __NUMBER_OF__, "0");
        FEPrintf(pCarName, "NO CARS");
        FEPrintf(pCarNameShadow, "NO CARS");
        GarageMainScreen::GetInstance()->DisableCarRendering();
        cFEng::Get()->QueuePackageMessage(0x913fa282, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
        FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x2d25b2c4, LANGUAGE_CS_NO_CARS_IN_LIST);
        cFEng::Get()->QueuePackageMessage(0xd9420cd5, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
        if (filter & 4) {
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x36c1e04d, 0x0da87b01);
        } else {
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x36c1e04d, LANGUAGE_CS_NO_CARS_IN_LIST);
        }
        return;
#else
        FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x2d25b2c4, LANGUAGE_CS_NO_CARS_IN_LIST);
        cFEng::Get()->QueuePackageMessage(0xd9420cd5, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
        if (filter & 4) {
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x36c1e04d, 0x0da87b01);
        } else {
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x36c1e04d, LANGUAGE_CS_NO_CARS_IN_LIST);
        }
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x0e9ed0a2);
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x18a4384f);
        CarViewer::CancelCarLoad(static_cast<eCarViewerWhichCar>(0));
        GarageMainScreen::GetInstance()->DisableCarRendering();
        cFEng::Get()->QueuePackageMessage(0x913fa282, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
        bLoadingBarActive = false;
        tLastEventTimer = 0;
        return;
#endif
    }

#ifndef EA_BUILD_A124 // la A124 no tiene modo online ni LAN aqui
    if (!FEDatabase->IsOnlineMode() && !FEDatabase->IsLANMode()) {
        FEngSetVisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x0e9ed0a2);
        FEngSetVisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x18a4384f);
    }
#endif

    FEngSetVisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x7379349b);
    cFEng::Get()->QueuePackageMessage(0x7c4583dc, reinterpret_cast<MenuScreen *>(this)->GetPackageName(), nullptr);
    FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), __NUMBER__, "%d", FilteredCarsList.GetNodeNumber(pSelectedCar));
    FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), __NUMBER_OF__, "%d", FilteredCarsList.CountElements());

#ifdef EA_BUILD_A124 // la A124 siempre mira la cuadra del jugador uno
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
#else
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(iPlayerNum);
#endif

    if (pSelectedCar->bLocked) {
        FEngSetScript(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xd0f7c7cc, 0x1ca7c0, true);
        FECarRecord *car = stable->GetCarRecordByHandle(pSelectedCar->mHandle);
#ifdef EA_BUILD_A124 // el prototipo no tiene coches de bonificacion y compone "%d, %s" en un segundo bufer
        Attrib::Gen::frontend fe_attrib(car->FEKey, 0, nullptr);
        int rival_num = fe_attrib.UnlockedAt() + 1;
        char rival_name_locdb[128];
        char buf[128];
        FEngSNPrintf(rival_name_locdb, 0x80, "blacklist_rival_%02d_aka", rival_num);
        bSNPrintf(buf, 0x80, "%d, %s", rival_num, GetLocalizedString(FEHashUpper(rival_name_locdb)));
        FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x2d25b2c4, GetLocalizedString(0x4ef2a115), buf);
#else
        if (car->MatchesFilter(0xf0008)) {
            int unlockText = GetBonusUnlockText(car);
            if (unlockText == 0x4ef2a115) {
                int binNum = GetBonusUnlockBinNumber(car);
                char rival_name_locdb[128];
                FEngSNPrintf(rival_name_locdb, 0x80, "blacklist_rival_%02d_aka", binNum);
                FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x2d25b2c4, GetLocalizedString(0x4ef2a115),
                         GetLocalizedString(FEHashUpper(rival_name_locdb)), binNum);
            } else {
                FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x2d25b2c4, static_cast<unsigned int>(unlockText));
            }
        } else {
            Attrib::Gen::frontend fe_attrib(car->FEKey, 0, nullptr);
            int rival_num = fe_attrib.UnlockedAt() + 1;
            char rival_name_locdb[128];
            FEngSNPrintf(rival_name_locdb, 0x80, "blacklist_rival_%02d_aka", rival_num);
            FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x2d25b2c4, GetLocalizedString(0x4ef2a115),
                     GetLocalizedString(FEHashUpper(rival_name_locdb)), rival_num);
        }
#endif
    }

    FECarRecord *car = stable->GetCarRecordByHandle(pSelectedCar->mHandle);
#ifdef EA_BUILD_A124 // la A124 escribe el nombre del coche en sus dos textos
    if (car->GetNameHash() != 0x9bb9ccc3 && DoesStringExist(car->GetNameHash())) {
        FEngSetLanguageHash(pCarName, car->GetNameHash());
        FEngSetLanguageHash(pCarNameShadow, car->GetNameHash());
    } else {
        FEPrintf(pCarName, car->GetDebugName());
        FEPrintf(pCarNameShadow, car->GetDebugName());
    }
#else
    FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), __CARNAME__);
    FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x79d6e45c);
#endif
    FEngSetTextureHash(pManuLogo, car->GetManuLogoHash());
    FEngSetTextureHash(pCarBadge, car->GetLogoHash());

    if (FEDatabase->IsCarLotMode()) {
        FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x1930b057, "%$d", FEDatabase->GetCareerSettings()->GetCash());
        FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x20c83c31, "%$d", car->GetCost());
        FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xdc18c4d4, 0xa9950b93);
        FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xb94139f4, 0x7010bbf2);
    }

    if (FEDatabase->IsCareerMode() && !FEDatabase->IsCarLotMode()) {
        TheHeatMeter.SetVisibility(true);
    } else {
        TheHeatMeter.SetVisibility(false);
    }

    if (car->IsCareer()) {
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x39dc21f9);
        FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xe998fe99);

        FECareerRecord *career = stable->GetCareerRecordByHandle(car->CareerHandle);

        if (TheFEMarkerManager.HasMarker(static_cast<FEMarkerManager::ePossibleMarker>(0x14), 0) || (CheatCanAddImpoundBox && !career->TheImpoundData.IsImpounded())) {
            int num_markers = TheFEMarkerManager.GetNumMarkers(static_cast<FEMarkerManager::ePossibleMarker>(0x14), 0);
            FEngSetVisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x39dc21f9);
            FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x5b875870, "%2d", num_markers);
            FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xea8aecd9, "%2d", num_markers);
        }
#ifndef EA_BUILD_A124
        else {
            FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x39dc21f9);
        }
#endif

        if (career->TheImpoundData.IsReleasable()) {
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x72e7ea88, 0x9db4df7d);
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x9d974df3, 0x073b79e0);
            float costf = static_cast<float>(car->GetReleaseFromImpoundCost());
            FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x322b18f9, "%$0.0f", costf);
            FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x7044a5a4, "%$d", FEDatabase->GetCareerSettings()->GetCash());
            FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x0e9ed0a2);
        } else if (career->TheImpoundData.IsImpounded()) {
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x72e7ea88, 0x9db4df7d);
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x9d974df3, 0x073b79e0);
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x322b18f9, 0xaefedad9);
            FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x7044a5a4, "%$d", FEDatabase->GetCareerSettings()->GetCash());
            FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x0e9ed0a2);

            if (TheFEMarkerManager.HasMarker(static_cast<FEMarkerManager::ePossibleMarker>(0x15), 0) || CheatReleaseFromImpoundMarker) {
                int num_markers = TheFEMarkerManager.GetNumMarkers(static_cast<FEMarkerManager::ePossibleMarker>(0x15), 0);
                FEngSetVisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xe998fe99);
#ifdef EA_BUILD_A124 // la A124 escribe la cuenta en los mismos textos que la caja de deposito
                FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x5b875870, "%2d", num_markers);
                FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xea8aecd9, "%2d", num_markers);
#else
                FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xcc59b910, "%2d", num_markers);
                FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0xb8f9938a, "%2d", num_markers);
#endif
                FEngSetInvisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x39dc21f9);
            }
        } else {
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x72e7ea88, 0x17574b0e);
            FEngSetLanguageHash(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x9d974df3, 0x915f4d26);

#ifdef EA_BUILD_A124
            FEngSetVisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x0e9ed0a2);
#else
            if (!FEDatabase->IsOnlineMode() && !FEDatabase->IsLANMode()) {
                FEngSetVisible(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x0e9ed0a2);
            }
#endif

            FECareerRecord *record = stable->GetCareerRecordByHandle(car->CareerHandle);
            if (record) {
                FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x322b18f9, "%$d", record->GetBounty());
                FEPrintf(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), 0x7044a5a4, "%$d", record->GetInfractions(true).GetFineValue());
            }
        }

        TheHeatMeter.SetCurrent(career->GetVehicleHeat());
        TheHeatMeter.SetPreview(career->GetVehicleHeat());
        TheHeatMeter.Draw();
    } else {
        TheHeatMeter.SetVisibility(false);
    }
}

void UIQRCarSelect::ChooseTransmission() {
#ifdef EA_BUILD_A124 // la A124 monta el dialogo a mano y lo ata al mando del jugador en pantalla partida
    eDialogFirstButtons first_button = first_dialog_button1;
    if (!FEDatabase->GetPlayerSettings(iPlayerNum)->Transmission) {
        first_button = first_dialog_button2;
    }
    feDialogConfig conf;
    GetLocalizedString(conf.BlurbString, 0x200, LANGUAGE_COMMON_TRANSMISSION);
    conf.Title = dialog_confirmation;
    conf.NumButtons = 2;
    conf.Button1TextHash = LANGUAGE_COMMON_MANUAL;
    conf.Button1PressedMessage = 0x5f5e3886;
    conf.Button2TextHash = LANGUAGE_COMMON_AUTO;
    conf.Button2PressedMessage = 0x1a2826e1;
    conf.DialogCancelledMessage = 0x1fab5998;
    conf.FirstButton = first_button;
    conf.ParentPackage = reinterpret_cast<MenuScreen *>(this)->GetPackageName();
    conf.DialogPackage = "";
    conf.bIsDismissable = true;
    if (FEDatabase->IsSplitScreenMode()) {
        conf.ControlMask = FEngMapJoyportToJoyParam(FEDatabase->GetPlayersJoystickPort(iPlayerNum));
    } else {
        conf.ControlMask = 0xff;
    }
    DialogInterface::ShowDialog(&conf);
#else
    DialogInterface::ShowTwoButtons(reinterpret_cast<MenuScreen *>(this)->GetPackageName(), "", dialog_confirmation, LANGUAGE_COMMON_MANUAL, LANGUAGE_COMMON_AUTO,
                                    0x5f5e3886, 0x1a2826e1, 0x34dc1bcf,
                                    (eDialogFirstButtons)(FEDatabase->GetPlayerSettings(iPlayerNum)->Transmission == 0), LANGUAGE_COMMON_TRANSMISSION);
#endif
}

FECarRecord *UIQRCarSelect::GetSelectedCarRecord() {
#ifdef EA_BUILD_A124 // la A124 siempre mira la cuadra del jugador uno
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
#else
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(iPlayerNum);
#endif
    FECarRecord *selected_car = stable->GetCarRecordByHandle(pSelectedCar->mHandle);
    return selected_car;
}

void UIQRCarSelect::SetSelectedCar(SelectableCar *newCar, int player_num) {
    pSelectedCar = newCar;
    if (TheBustedManager.IsImpoundInfoVisible() && pSelectedCar != nullptr) {
        TheBustedManager.SetSelectedCar(GetSelectedCarRecord());
    }
    if (newCar != nullptr) {
        ListHandles[GetFilterType()] = newCar->mHandle;
        GarageMainScreen::GetInstance()->DisableCarRendering();

        const u32 FEObj_LOADERAPPEAR = 0xa05a328e;
        const u32 FEObj_FADE_IN_TEXT = 0x9c0a27eb;
        cFEng::Get()->QueuePackageMessage(FEObj_LOADERAPPEAR, nullptr, nullptr);
        cFEng::Get()->QueuePackageMessage(FEObj_FADE_IN_TEXT, this->GetPackageName(), nullptr);

        bLoadingBarActive = true;
        if (FEDatabase->IsCareerMode()) {
            if (!FEDatabase->IsCarLotMode()) {
                FEDatabase->GetCareerSettings()->SetCurrentCar(newCar->mHandle);
            }
        } else if (!FEDatabase->IsCustomizeMode()) {
            FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->SetSelectedCar(newCar->mHandle, player_num);
        }
        tLastEventTimer = RealTimer;
    }
}

int SortCarsByUnlock(SelectableCar *a, SelectableCar *b) {
#ifdef EA_BUILD_A124
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
#else
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(gPlayerNum);
#endif
    FECarRecord *carA = stable->GetCarRecordByHandle(a->mHandle);
    Attrib::Gen::frontend fe_a(carA->FEKey, 0, nullptr);
    FECarRecord *carB = stable->GetCarRecordByHandle(b->mHandle);
    Attrib::Gen::frontend fe_b(carB->FEKey, 0, nullptr);
    int binA = fe_a.UnlockedAt();
    int binB = fe_b.UnlockedAt();
    return static_cast<int>(binA > binB);
}

bool IsValidMikeMannCar(FECarRecord *fe_car, unsigned int filter) {
    if (GetMikeMannBuild() == 1) {
        return fe_car->GetType() != CARTYPE_CAYMANS;
    }
    if (GetMikeMannBuild() == 2) {
        if (static_cast<unsigned short>(filter) == 1) {
            switch (fe_car->GetType()) {
#ifdef EA_BUILD_A124
                case CARTYPE_MUSTANGGT:
                case CARTYPE_SLR:
                case CARTYPE_BMWM3GTR:
                case CARTYPE_IS300:
                case CARTYPE_GALLARDO:
#else
                case CARTYPE_RX8:
                case CARTYPE_SLR:
                case CARTYPE_BMWM3GTR:
                case CARTYPE_CAYMANS:
                case CARTYPE_GALLARDO:
                case CARTYPE_PUNTO:
#endif
                    return true;
                default:
                    return false;
            }
        }
        if (static_cast<unsigned short>(filter) == 0x10) {
            return fe_car->Handle == bStringHash("M3GTRCAREERSTART");
        }
    }
    // Un solo return true al final (linea 1870 del original, detras del de la 1867): GameCube junta ahi los de
    // GetMikeMannBuild() != 2 y los del switch.
    return true;
}

void UIQRCarSelect::RefreshBonusCarList() {
    bool bCarUnlocked;
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x136253, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x136253, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x136252, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x136252, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x136251, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x136251, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x136250, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x136250, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x13624f, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x13624f, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x13624e, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x13624e, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x9666, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x9666, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x9665, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x9665, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x9664, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x9664, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x9663, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x9663, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x9662, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x9662, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x9661, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x9661, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x9660, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x9660, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x965f, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x965f, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x3a94520, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x3a94520, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0xcb6aaf2f, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0xcb6aaf2f, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x2cf370f0, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x2cf370f0, !bCarUnlocked));
    bCarUnlocked = UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x2cf385b2, iPlayerNum);
    FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x2cf385b2, !bCarUnlocked));
    if (UnlockSystem::IsCarUnlocked(UNLOCK_QUICK_RACE, 0x34498eb2, iPlayerNum)) {
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x34498eb2, false));
    }
    if (GetIsCollectorsEdition()) {
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x634d1bd2, false));
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0xe1075862, false));
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x02d642b8, false));
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x03d3401a, false));
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x03d8a6d1, false));
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x54653c71, false));
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0xe115ead0, false));
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x54655133, false));
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x582f21d9, false));
        FilteredCarsList.AddTail(new ("SelectableCar", 0) SelectableCar(0x363a1fea, false));
    }
    SetupForPlayer(iPlayerNum);
}

void UIQRCarSelect::RefreshCarList() {
    ClearCarList();
#ifndef EA_BUILD_A124 // la A124 no tiene la lista de coches de bonificacion
    if ((filter & 8) != 0) {
        RefreshBonusCarList();
        return;
    }
#endif
    FEPlayerCarDB *carDB = FEDatabase->GetPlayerCarStable(iPlayerNum);
    unsigned int unlockFilter;
    if (FEDatabase->IsCareerMode()) {
        unlockFilter = 2;
    } else if (FEDatabase->IsQuickRaceMode()) {
        unlockFilter = 1;
#ifdef EA_BUILD_A124 // sin LAN, y online desbloquea con su propio filtro
    } else if (FEDatabase->IsOnlineMode()) {
        unlockFilter = UNLOCK_ONLINE;
#else
    } else if (FEDatabase->IsOnlineMode() || FEDatabase->IsLANMode()) {
        unlockFilter = 7;
#endif
    } else {
        unlockFilter = 1;
    }
    int i = 0;
    do {
        FECarRecord *car = carDB->GetCarByIndex(i);
        if (car->IsValid() && car->MatchesFilter(filter)) {
            bool unlocked = UnlockSystem::IsCarUnlocked(static_cast<eUnlockFilters>(unlockFilter), car->Handle, iPlayerNum);
            if (!GetMikeMannBuild() || IsValidMikeMannCar(car, filter)) {
                SelectableCar *newCar = new ("SelectableCar", 0) SelectableCar(car->Handle, !unlocked);
                FilteredCarsList.AddTail(newCar);
            }
        }
        i++;
    } while (i < 200);
    unsigned int f = static_cast<unsigned short>(filter);
    switch (f) {
        case 1:
        case 2:
        case 8:
        case 0x10:
            FilteredCarsList.Sort(SortCarsByUnlock);
            break;
    }
    SetupForPlayer(iPlayerNum);
}

void UIQRCarSelect::ClearCarList() {
    FilteredCarsList.DeleteAllElements();
}

void UIQRCarSelect::ScrollCars(eScrollDir dir) {
    SelectableCar *old_car = pSelectedCar;
    if (!old_car)
        return;
    SelectableCar *new_car = FilteredCarsList.GetPrevCircular(old_car);
    if (dir == eSD_PREV) {
    } else if (dir == eSD_NEXT) {
        new_car = FilteredCarsList.GetNextCircular(old_car);
    }
    if (new_car != old_car) {
        SetSelectedCar(new_car, iPlayerNum);
        RefreshHeader();
    }
}

// UNSOLVED
void UIQRCarSelect::ScrollLists(eScrollDir dir) {
    if (FEDatabase->IsCareerMode()) {
        return;
    }
    if (FEDatabase->IsCustomizeMode()) {
        return;
    }

    uint32 region = this->filter & 0xffff0000;
    uint32 list = this->filter & 0xffff;
#ifdef EA_BUILD_A124 // la A124 tiene seis listas de coches (la ultima, 0x20)
    const uint32 last_list = 0x20;
#else
    const uint32 last_list = 8;
#endif

    if (GetMikeMannBuild() == 2) {
        if (list == 1) {
            list = 4;
        } else if (list == 4) {
            list = 16;
        } else if (list == 16) {
            list = 1;
        }
    } else if (dir == eSD_PREV) {
        if (list == 1) {
            list = last_list;
        } else {
            list = list / 2;
        }
    } else if (dir == eSD_NEXT) {
        if (list == last_list) {
            list = 1;
        } else {
            list = list << 1;
        }
    }

    filter = region | list;
#ifdef EA_BUILD_A124 // y guarda el filtro de cada jugador en la carrera rapida
    FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0xb))->CarSelectFilterBits[iPlayerNum] = filter;
#endif

    RefreshCarList();
    RefreshHeader();
}

void UIQRCarSelect::OnlineActOnSelect() {
    FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(1))->SetSelectedCar(pSelectedCar->mHandle, 0);
    FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(0))->SetSelectedCar(pSelectedCar->mHandle, 0);
    FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(2))->SetSelectedCar(pSelectedCar->mHandle, 0);
}
