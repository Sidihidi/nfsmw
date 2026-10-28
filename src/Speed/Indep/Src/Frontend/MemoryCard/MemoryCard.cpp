// clang-format off
// Maquetado a las lineas del original de la A124 (solo lineas en blanco de relleno): la A124 es una build milestone y
// bMalloc pasa __LINE__, asi que el de LoadLocale, Save y Load tiene que caer en la linea del original.
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCardHelper.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCardCallbacks.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/MenuScreens/../Frontend/MenuScreens/MemCard/uiMemcardInterface.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/MemCard/uiMemcardBase.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Misc/bFile.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"

extern EA::Allocator::IAllocator &gMemoryAllocator;
extern MemcardCallbacks gMemcardCallbacks;

unsigned short gSaveType0[32];
unsigned short gSaveType1[32];
unsigned short gSaveType2[16];

// .data:0x8041BBB8 del original (0xC B), justo delante de _10MemoryCard.s_pThis:
// los tres punteros valen gSaveType0/1/2. El zPlatform extraido lo referencia
// como UND (MemoryCardImp.cpp lo declara con su nombre de ensamblador).
unsigned short *MemoryCardImp::gEntryType[3] = {gSaveType0, gSaveType1, gSaveType2};
unsigned short MemoryCardImp::gContentName[32];
MemoryCard *MemoryCard::s_pThis = nullptr;

#if defined(EA_PLATFORM_GAMECUBE) || defined(EA_PLATFORM_PLAYSTATION2)
// RealMemCard is built with -fshort-wchar on these platforms, so its wide string
// methods are mangled with `Uw`, while this unit (plain wchar_t) calls the `w`
// names. These C shims carry the `w` names and forward to the library ones; the
// original has them with exactly these names and void * parameters. In PS2 the
// library also compresses repeated types differently (N21 and bT3 where this unit
// writes T1T1 and bb), so Save needs one there too (the A124 and SLUS call it).
extern "C" {

#ifdef EA_PLATFORM_PLAYSTATION2
void Save__Q211RealmcIface16MemcardInterfacePCcN21PCQ211RealmcIface8SaveInfoPCQ211RealmcIface9TitleInfo(void *pthis, void *p1, void *p2,
                                                                                                        void *p3, void *p4, void *p5);
#endif
void Delete__Q211RealmcIface16MemcardInterfacePCcPCUw(void *pthis, void *p1, void *p2);
#ifdef EA_PLATFORM_PLAYSTATION2
void *__Q211RealmcIface8GameInfoPCUwPCcbT3(void *pthis, void *p0, void *p1, void *p2);
void Load__Q211RealmcIface16MemcardInterfacePCcPcT2PCUwPCQ211RealmcIface9TitleInfo(void *pthis, void *p0, void *p1, void *p2, void *p3,
                                                                                   void *p4, void *p5);
#else
void *__Q211RealmcIface8GameInfoPCUwUibT3(void *pthis, void *p0, void *p1, void *p2);
void Load__Q211RealmcIface16MemcardInterfacePCcPcT2PCUwPCQ211RealmcIface9TitleInfoT4(void *pthis, void *p0, void *p1, void *p2,
                                                                                     void *p3, void *p4, void *p5);
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
void Save__Q211RealmcIface16MemcardInterfacePCcT1T1PCQ211RealmcIface8SaveInfoPCQ211RealmcIface9TitleInfo(void *pthis, void *p1, void *p2,
                                                                                                         void *p3, void *p4, void *p5) {
    Save__Q211RealmcIface16MemcardInterfacePCcN21PCQ211RealmcIface8SaveInfoPCQ211RealmcIface9TitleInfo(pthis, p1, p2, p3, p4, p5);
}
#endif

void Delete__Q211RealmcIface16MemcardInterfacePCcPCw(void *pthis, void *p1, void *p2) {
    Delete__Q211RealmcIface16MemcardInterfacePCcPCUw(pthis, p1, p2);
}

#ifdef EA_PLATFORM_PLAYSTATION2
void *__Q211RealmcIface8GameInfoPCwPCcbb(void *pthis, void *p0, void *p1, void *p2) {
    return __Q211RealmcIface8GameInfoPCUwPCcbT3(pthis, p0, p1, p2);
}
#else
void *__Q211RealmcIface8GameInfoPCwUibT3(void *pthis, void *p0, void *p1, void *p2) {
    return __Q211RealmcIface8GameInfoPCUwUibT3(pthis, p0, p1, p2);
}
#endif

void Load__Q211RealmcIface16MemcardInterfacePCcPcT2PCwPCQ211RealmcIface9TitleInfo(void *pthis, void *p0, void *p1, void *p2, void *p3,
                                                                                  void *p4, void *p5) {
#ifdef EA_PLATFORM_PLAYSTATION2
    Load__Q211RealmcIface16MemcardInterfacePCcPcT2PCUwPCQ211RealmcIface9TitleInfo(pthis, p0, p1, p2, p3, p4, p5);
#else
    Load__Q211RealmcIface16MemcardInterfacePCcPcT2PCUwPCQ211RealmcIface9TitleInfoT4(pthis, p0, p1, p2, p3, p4, p5);
#endif
}
}
#endif

#ifdef EA_PLATFORM_WIN32
#include "Speed/PC/Src/Win32Min.hpp" // ListOldSaveFilesPC
#endif












void InitMemoryCard() {
    MemoryCard::s_pThis = new ("MemoryCard", 0) MemoryCard();

    bStrCpy(gSaveType0, "");
    bStrCpy(gSaveType1, "");
    bStrCpy(gSaveType2, "");

    bStrCpy(MemoryCardImp::gContentName, "NFSMWSD");
    MemoryCard::s_pThis->Init();
}







MemoryCardMessage::MemoryCardMessage(const wchar_t *msg, unsigned int nOptions, const wchar_t **options) {
    bStrCpy(reinterpret_cast<unsigned short *>(mMsg), reinterpret_cast<const unsigned short *>(msg));
    mnOptions = nOptions;
    for (unsigned int i = 0; i < nOptions; i++) {
        bStrCpy(reinterpret_cast<unsigned short *>(mOptions[i]), reinterpret_cast<const unsigned short *>(options[i]));
    }
}






MemoryCard::MemoryCard() {
    m_MemOp = 0;
    m_bWaitingForResponse = false;
    m_pIMemcard = nullptr;
    m_PendingMessage = nullptr;
    m_BootupParams.Clear();
    m_Type = ST_PROFILE;
    m_bBootFoundFile = false;
    m_bAutoSave = false;
    m_bInAutoSave = false;
    m_bCheckingCardForAutoSave = false;
    m_bCheckingCardForOverwrite = false;
    m_bAutoSaveRequested = false;
    m_bAutoSaveCardPulled = false;
    m_ReqOp = 0;
    m_bInBootSequence = true;
    m_bRetryBootCheck = false;
    m_bManualSave = false;
    m_bAutoSaveCardPulledDuringSave = false;
    m_bOldSaveFileExists = false;
    m_bListingOldSaveFiles = false;
    m_bFoundAutoSaveFile = false;
    m_bMemcardScreenShowing = false;
    m_bCardRemoved = false;
    m_bRetryAutoSave = false;
    m_bInitialized = false;
    m_bDisablingAutoSaveForSave = false;
    m_bAutoLoading = false;
    m_bListingForCreate = false;
    m_bHUDLoaded = false;
#ifndef EA_BUILD_A124
    m_bCancelNextAutoSave = false;
    m_bMonitorOn = false;
    m_bAutoSaveIconShowing = false;
    m_bNeedToAllowControllerErrors = false;
    m_bNonSilentAutoSave = false;
#endif
#ifdef EA_PLATFORM_GAMECUBE
    m_bAutoLoadDone = false;
    m_bMemcardScreenExiting = false;
#elif defined(EA_PLATFORM_XENON)
    x360_unknown_0x10 = 0;
    x360_unknown_0x18 = 0;
    x360_unknown_0x14 = false;
    x360_unknown_0x1C = false;
#endif
    m_nPlayer = 0;

#ifdef EA_PLATFORM_GAMECUBE
    void *pIcon = bGetFile("memcard/icon1.raw", nullptr, 0);

    void *pBanner = bGetFile("memcard/banner.raw", nullptr, 0);

    m_pRMIcon = BNEW GCIconDataInfo();
    m_pRMBanner = BNEW GCBannerDataInfo();

    m_pRMIcon->mNumIcons = 1;
    m_pRMIcon->mIconData = pIcon;
    m_pRMIcon->mSpeed = 0;

    m_pRMBanner->mBannerData = pBanner;
    m_pRMBanner->mFormat = 0;
#endif
}

bool MemoryCard::IsCardAvailable() {
    MemoryCard *pThis = GetInstance();

    if (pThis != nullptr) {
        unsigned short err = pThis->m_LastError;

        if (err == 0 || err == 11) {
            return true;
        }

        return false;
    }

    return false;
}

void MemoryCard::SetExtraParam(SaveType t, const char *filename, void *buf, unsigned int size) {
    // Cinco GetInstance() en el DWARF de GameCube y ninguna local; PC relee s_pThis tras cada store.

    if (GetInstance() != nullptr) {
        GetInstance()->m_ReqFilename = filename;
        GetInstance()->m_Type = t;
        GetInstance()->m_pBuffer = static_cast<char *>(buf);
        GetInstance()->m_DataSize = size;
    }
}

void MemoryCard::InitCommand(int op) {
    m_ReqOp = 0;

    m_bWaitingForResponse = false;

    m_MemOp = op;
    m_LastError = 0;
}






void MemoryCard::RequestTask(int op, const char *name) {
    m_ReqFilename = name;
    m_ReqOp = op;
}




void MemoryCard::ProcessTask() {
    if (m_pFEScreen == nullptr) {
        m_ReqOp = 0;
        return;
    }

    {
        switch (m_ReqOp) {
            case 6:
                Delete(m_ReqFilename);
                break;
            case 5:
                Load(m_ReqFilename);
                break;
            case 7:
                List(nullptr, nullptr);
                break;
        }
    }

    m_ReqOp = 0;
}

bool MemoryCard::IsCardBusy() {
    if (GetInstance() != nullptr) {
        if (GetInstance()->m_pIMemcard->IsResettable() == 0 ||
#ifndef EA_BUILD_A124
            GetInstance()->IsAutoSaveIconVisible() ||
#endif
            (GetInstance()->m_bInAutoSave && !GetInstance()->m_bWaitingForResponse)) {
            return true;
        }
    }

    return false;
}




















































































































































void MemoryCard::Init() {
    static Realmc::SystemInterface iSystem;
    static MemoryCardImp sMemcardImp;
    static Realmc::SystemInterface *pSystem = nullptr;

    if (pSystem == nullptr) {
        iSystem.mAllocator = &gMemoryAllocator;
        // Los nombres estan en el .rodata de GameCube y de las dos PS2; la A124 pasa 0 como linea.
        iSystem.mThread = new ("MemcardThread", 0) MyThread;
        iSystem.mMutex = new ("MemcardMutex", 0) MyMutex;
        iSystem.mLocaleGetter = MemoryCard::GetLocaleString;
        pSystem = &iSystem;
    }

    m_pImp = &sMemcardImp;

#ifdef EA_BUILD_A124
    wchar_t wszGameTitle[28];

    bStrCpy(reinterpret_cast<unsigned short *>(wszGameTitle), "Need" "\xA0" "for" "\xA0" "Speed" "\x99" "\xA0" "Most" "\xA0" "Wanted");
#else
    bStrCpy(m_GameTitle, "Need" "\xA0" "for" "\xA0" "Speed" "\x99" "\xA0" "Most" "\xA0" "Wanted");
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
    RealmcIface::TitleId titleId;

    bStrCpy(titleId, m_pImp->GetTitleId());
#else
    unsigned int titleId = 0;
#endif

#ifdef EA_BUILD_A124
    RealmcIface::GameInfo gameInfo(wszGameTitle, titleId, false, false);

    m_pIMemcard = RealmcIface::MemcardInterface::CreateInstance(&iSystem, &gMemcardCallbacks, &gameInfo);
#else
    m_pGameInfo = BNEW RealmcIface::GameInfo(reinterpret_cast<const wchar_t *>(m_GameTitle), titleId, false, false);
    m_pIMemcard = RealmcIface::MemcardInterface::CreateInstance(&iSystem, &gMemcardCallbacks, m_pGameInfo);
#endif

#ifdef EA_PLATFORM_GAMECUBE
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_SHOW, 1);
#endif

    m_TimeOffsetSec = 0;
    m_pLocaleFileHandler = nullptr;
}

void MemoryCard::StartBootSequence() {
    m_bInBootSequence = true;
    gMemcardSetup.mOp = 32;
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x4000);
}


void MemoryCard::EndBootSequence() {
    m_bInBootSequence = false;
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_SHOW, 0x4000);
}







void MemoryCard::LoadLocale(eLanguages eLang) {
    if (GetInstance() == nullptr) {
        return;
    }

    char sPath[64];

    bStrCpy(sPath, "memcard/LOCALE_");

    switch (eLang) {
#ifdef EA_PLATFORM_PLAYSTATION2
        case eLANGUAGE_CHINESE:
            bStrCat(sPath, sPath, "ENGLISH_ASIAN.loc");
            break;
#endif
        case eLANGUAGE_LARGEST:
        case eLANGUAGE_LABELS:
            bStrCat(sPath, sPath, "ENGLISH.loc");
            break;
        default:
            bStrCat(sPath, sPath, GetLanguageName(eLang));
            bStrCat(sPath, sPath, ".loc");
            break;
    }

    // "Locale buffer", "Save Buffer" y "Load Buffer" son los nombres que estan en el .rodata de GameCube
    // y de las dos PS2; la linea no se puede casar (la A124 dice 531, 1032 y 1079).
    if (GetInstance()->m_pLocaleFileHandler == nullptr) {

#ifdef EA_PLATFORM_PLAYSTATION2
        GetInstance()->m_pLocaleFileHandler = bMalloc(0x2400, "Locale buffer", __LINE__, 0);
#else
        GetInstance()->m_pLocaleFileHandler = bMalloc(0x2000, "Locale buffer", __LINE__, 0);
#endif
    }

    unsigned int currentsize = bFileSize(sPath);



    bFile *file = bOpen(sPath, 1, 1);
    bRead(file, GetInstance()->m_pLocaleFileHandler, currentsize);
    bClose(file);

    LOCALE_create(GetInstance()->m_pLocaleFileHandler, 1);
    LOCALE_setstate(GetInstance()->m_pLocaleFileHandler, LOCALE_LANGUAGEID, 0);

#ifndef EA_PLATFORM_PLAYSTATION2
    bStrCpy(gSaveType0, GetLocalizedString(0xe6f55df0));
#endif

}


int MemoryCard::GetPrefixLength() {
    return bStrLen(m_pImp->GetPrefix());
}

const char *MemoryCard::GetPrefix() {
    return m_pImp->GetPrefix();
}



const char *MemoryCard::GetLocaleString(int strID) {
#ifdef EA_PLATFORM_PLAYSTATION2
    if (GetInstance()->m_MemOp == MO_LoadYNCF && strID > 6) {
        return LOCALE_getstrA(GetInstance()->m_pLocaleFileHandler, 17);
    }
#endif
    return LOCALE_getstrA(GetInstance()->m_pLocaleFileHandler, strID);
}




































void MemoryCard::SetMessageMode(unsigned int msg, bool flag) {
    if (GetInstance() != nullptr) {
        GetInstance()->m_pIMemcard->SetMessage((RealmcIface::MessageState)(flag == 0), msg);
    }
}

void MemoryCard::Tick(int TickCount) {
    if (m_MemOp == 0 && m_ReqOp != 0) {
        ProcessTask();
    }

    if (m_bAutoSaveRequested && m_bHUDLoaded && GManager::Exists() && !GManager::Get().GetHasPendingSMS()) {
        m_bHUDLoaded = false;
        m_bAutoSaveRequested = false;
        StartAutoSave(false);
    }

    if (Joylog::IsReplaying()) {
        MemoryCardJoyLoggableEvents l_JoyOp;
        do {
            l_JoyOp = static_cast<MemoryCardJoyLoggableEvents>(ReplayJoyOp());
        } while (l_JoyOp != MJ_None);
    } else {
        m_pIMemcard->Update(TickCount);
        if (Joylog::IsCapturing()) {
            CaptureJoyOp(MJ_None);
        }
    }

    // El prototipo no tiene el icono de autoguardado ni este control de los errores del mando.
#ifndef EA_BUILD_A124
    if (FEDatabase != nullptr && !FEDatabase->IsOptionsMode()) {
        if (cFEng::Get()->IsPackagePushed("MC_Main_GC.fng") || cFEng::Get()->IsPackagePushed("InGame_MC_Main_GC.fng") ||
            IsAutoSaveIconVisible()) {
            if (FEManager::Get()->IsAllowingControllerError() || TheGameFlowManager.IsInGame()) {
                if (cFEng::Get()->IsPackagePushed("InGamePhotoMaster.fng") || cFEng::Get()->IsPackagePushed("PostRace_Pursuit.fng")) { m_bNonSilentAutoSave = true; }
                m_bNeedToAllowControllerErrors = true;
                FEManager::Get()->AllowControllerError(false);
                FEManager::Get()->SuppressControllerError(true);
            }
        } else if (m_bNeedToAllowControllerErrors) {
            m_bNeedToAllowControllerErrors = false;
            if (!FEManager::Get()->IsAllowingControllerError()) {
                if (m_bNonSilentAutoSave) {
                    m_bNonSilentAutoSave = false;
                } else {
                    FEManager::Get()->AllowControllerError(true);
                    FEManager::Get()->SuppressControllerError(false);
                }
            }
        }
    }
#endif
}


void MemoryCard::MessageDone(RealmcIface::MessageChoices nInput) {
    if (m_bWaitingForResponse) {
        m_pIMemcard->MessageDone(nInput);
        m_bWaitingForResponse = false;
    }
}

void MemoryCard::BootupCheck(const char *entry) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // El reloj de la PS2 va en hora de Japon.
    m_TimeOffsetSec = sceScfGetTimeZone() * 60 - 32400;
    m_TimeOffsetSec += sceScfGetSummerTime() * 3600;
#endif
    // PS2 y PC buscan con el prefijo y "*" en m_Filename; la A124 lo prepara despues de ConstructSaveInfo.
#if !defined(EA_PLATFORM_GAMECUBE) && !defined(EA_BUILD_A124)
    bStrCat(m_Filename, m_pImp->GetPrefix(), "*");
#endif
#ifdef EA_PLATFORM_GAMECUBE
    bStrCpy(m_BootupFilename, "");
#endif

    m_pImp->ConstructSaveInfo(ST_PROFILE, "", FEDatabase->GetUserProfileSaveSize(false));

#ifdef EA_BUILD_A124
    bStrCat(m_Filename, m_pImp->GetPrefix(), "*");
#endif

#ifndef EA_PLATFORM_GAMECUBE
    m_BootupParams.mEntryNamePattern = m_Filename;
#else
    m_BootupParams.mEntryNamePattern = m_BootupFilename;
#endif

    m_BootupParams.mNumSaveTypes = 1;
    m_BootupParams.mSaveReqs = m_pImp->GetSaveReqArray();
    m_BootupParams.mValidCardIds = 1;

    InitCommand(MO_BootUp);
    if (!Joylog::IsReplaying()) {
        m_pIMemcard->BootupCheck(&m_BootupParams, 0, nullptr, nullptr);
    }
}

// r56-fe: ShouldDoAutoSave va detras de BootupCheck (zFe.o 0x020130), no
// detras de Tick.
bool MemoryCard::ShouldDoAutoSave(bool bForce) {
    if (bForce) {
        return true;
    }
#ifdef EA_BUILD_A124
    // En el prototipo la persecucion final guarda siempre, antes de mirar nada mas.
    if (FEDatabase->IsFinalEpicChase()) {
        return true;
    }
#else
    if (m_bCancelNextAutoSave) {
        m_bCancelNextAutoSave = false;
        return false;
    }

    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
        return false;
    }
#endif

    if (!IsMemcardEnabled || !IsAutoSaveEnabled) {
        return false;
    }
    if (!FEDatabase->GetUserProfile(0)->GetOptions()->TheGameplaySettings.AutoSaveOn && !m_bCardRemoved) {
        return false;
    }
#ifndef EA_BUILD_A124
    if (FEDatabase->IsFinalEpicChase())
        return true;
#endif
    {
        if (GRaceStatus::Exists()) {
            if (GRaceStatus::Get().GetRaceParameters() != nullptr && GRaceStatus::Get().GetRaceParameters()->GetIsBossRace()) {
                return false;
            }
        }
    }

    return true;
}
void MemoryCard::StartAutoSave(bool bForce) {
    if (!ShouldDoAutoSave(bForce)) { // con return explicito ee-gcc no salta a HandleAutoSaveError ni a CheckCard
        return;
    }
#ifdef EA_BUILD_A124
    // Sin icono de autoguardado, y la orden se limpia antes de mirar si hay perfil cargado.
    if (gMemcardSetup.GetCommand() != MCO_AutoSave) {
        gMemcardSetup.mOp = 0;
    }
#endif
#if !defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_BUILD_A124) // SLUS autoguarda sin mirar si hay perfil
    if (FEDatabase->bProfileLoaded)
#endif
    {
#ifndef EA_BUILD_A124
        if (gMemcardSetup.GetCommand() != MCO_AutoSave) {
            ShowAutoSaveIcon();
            gMemcardSetup.mOp = 0;
        }
#endif

        if (m_bCardRemoved) {
            HandleAutoSaveError();
        } else {
            m_bInAutoSave = true;
            m_bCheckingCardForAutoSave = true;
#ifndef EA_BUILD_A124
            FEManager::Get()->SuppressControllerError(true);
#endif

#ifdef EA_PLATFORM_WIN32 // PC no comprueba la tarjeta: autoguarda en seguida (speed.exe 0x562AB0)
            DoAutoSave();
#else
            ShowMessages(false);
            CheckCard(0);
#endif
        }
    }
}

void MemoryCard::DoAutoSave() {
    m_bCheckingCardForAutoSave = false;

    if (gMemcardSetup.GetCommand() == MCO_AutoSave) {
        ShowMessages(true);
#ifndef EA_BUILD_A124
        m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x100);
#endif
    } else {
        ShowOnlyAutoSaveMessages();
    }

    Save(FEDatabase->GetUserProfile(0)->GetProfileName());
}

void MemoryCard::EndAutoSave() {
    if (!m_bRetryAutoSave) {
        m_MemOp = 0;
    }
    m_bCheckingCardForAutoSave = false;
    m_bCheckingCardForOverwrite = false;
    m_bInAutoSave = false;
#ifndef EA_BUILD_A124
    FEManager::Get()->SuppressControllerError(false);
#endif
    ShowMessages(true);
#ifdef EA_BUILD_A124
    cFEng::Get()->QueuePackageMessage(FEHashUpper("TRAX_POS_1"), nullptr, nullptr);
#else
    HideAutoSaveIcon();
#endif
}

















void MemoryCard::StartListingOldSaveFiles() {
    m_bListingOldSaveFiles = true;

#ifdef EA_PLATFORM_PLAYSTATION2
    ListOldSaveFilesPS2();
#elif defined(EA_PLATFORM_WIN32) // speed.exe 0x5268A0: mov [ecx+0x1D], 1 y jmp a ListOldSaveFilesPC
    ListOldSaveFilesPC();
#else
    ListOldSaveFilesNGC();
#endif
}







void MemoryCard::EndListingOldSaveFiles() {
    m_bListingOldSaveFiles = false;
    if (m_bOldSaveFileExists) {
        cFEng::Get()->QueueGameMessage(0x7e998e5e, nullptr, 0xff);
        DialogInterface::ShowOneButton("", "", static_cast<eDialogTitle>(2), LANGUAGE_COMMON_OK, 0x34dc1bec, 0xc5e2beac);
    }

    FEDatabase->GetCareerSettings()->AwardOneTimeCashBonus(m_bOldSaveFileExists);
}


void MemoryCard::SetMonitor(bool bEnabled) {
    InitCommand(MO_SetMonitor);
    if (!Joylog::IsReplaying()) {
        m_pIMemcard->SetMonitor(bEnabled ? RealmcIface::MONITOR_ON : RealmcIface::MONITOR_OFF);
    }

#ifndef EA_BUILD_A124
    if (!bEnabled && Joylog::IsReplaying()) {
        ReplayJoyOp();
    }
#endif
}

void MemoryCard::SetAutoSaveEnabled(bool bEnabled) {
    char entryname[16];

    bStrCpy(entryname, FEDatabase->GetMultiplayerProfile(0)->GetProfileName());

    SetExtraParam(ST_PROFILE, entryname, nullptr, FEDatabase->GetUserProfileSaveSize(false));

    bStrCat(m_Filename, m_pImp->GetPrefix(), entryname);

    bStrNCpy(MemoryCardImp::gContentName, entryname, 16);

#ifndef EA_BUILD_A124
    if (GetScreen() != nullptr && gMemcardSetup.GetCommand() == MCO_EnableAutoSave) {
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32)
        GetScreen()->SetStringCheckingCard();
#endif

        ShowMessages(true);
    } else {
        ShowMessages(false);
    }

    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_SHOW, 1);
#endif

    if (bEnabled) {
        gMemcardSetup.ClearCommand();
        gMemcardSetup.SetCommand(MCO_EnableAutoSave);
    } else {
        m_bDisablingAutoSaveForSave = true;
    }

#ifdef EA_BUILD_A124
    // El prototipo no mira la pantalla y oculta los mensajes despues de fijar la orden.
    ShowMessages(false);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_SHOW, 1);
#endif

    InitCommand(MO_AutoSave);
    if (!Joylog::IsReplaying()) {
#ifdef EA_BUILD_A124
        GetInstance()->m_pIMemcard->SetAutosave(bEnabled ? RealmcIface::AUTOSAVE_ENABLE : RealmcIface::AUTOSAVE_DISABLE, 0, nullptr,
                                                entryname, RealmcIface::CARD_UNKNOWN);
#else
        m_pIMemcard->SetAutosave(bEnabled ? RealmcIface::AUTOSAVE_ENABLE : RealmcIface::AUTOSAVE_DISABLE, 0, nullptr, entryname,
                                 RealmcIface::CARD_UNKNOWN);
#endif
    }

#ifndef EA_BUILD_A124
    if (!bEnabled && Joylog::IsReplaying()) {
        ReplayJoyOp();
    }
#endif
}

void MemoryCard::ShowOnlyAutoSaveMessages() {
    m_bManualSave = false;

    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_SHOW, 2);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_SHOW, 4);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_SHOW, 0x800);

    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 1);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x100);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x200);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x400);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x1000);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x2000);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x4000);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x8000);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x10000);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x20000);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x40000);
    m_pIMemcard->SetMessage(RealmcIface::MESSAGE_HIDE, 0x80000);
}








void MemoryCard::ShowMessages(bool bShow) {
    m_bManualSave = bShow;
    m_pIMemcard->SetMessage((RealmcIface::MessageState)(bShow == 0), 0xffffffff);
}


void MemoryCard::CheckCard(int iSlot) {
    InitCommand(MO_CheckCard);
    if (!Joylog::IsReplaying()) {
        m_pIMemcard->CheckCard(RealmcIface::CARD_UNKNOWN);
    }
}































#ifdef EA_PLATFORM_PLAYSTATION2
void MemoryCard::LoadYNCF(int iSlot) {
    if (m_MemOp != 0) {
        return;
    }

    InitCommand(MO_LoadYNCF);
    if (!Joylog::IsReplaying()) {
        m_pIMemcard->CheckCard(static_cast<RealmcIface::CardId>(1));
    }
}
#endif

void MemoryCard::Save(const char *entryName) {
    SetExtraParam(ST_PROFILE, entryName, nullptr, FEDatabase->GetUserProfileSaveSize(false));


    if (m_pImp->GetSaveInfo() == nullptr) {
        m_pImp->ConstructSaveInfo(ST_PROFILE, entryName, m_DataSize);
        bStrCat(m_Filename, m_pImp->GetPrefix(), entryName);
    }

    bStrNCpy(MemoryCardImp::gContentName, entryName, 16);

    m_pBuffer = static_cast<char *>(bMalloc(m_DataSize, "Save Buffer", __LINE__, 0x40));
    FEDatabase->SaveUserProfileToBuffer(m_pBuffer, m_DataSize);



    m_Header[0] = 0x10d;
    m_Header[1] = m_DataSize;

    InitCommand(MO_Save);
    if (!Joylog::IsReplaying()) {
        m_pIMemcard->Save(m_Filename, reinterpret_cast<const char *>(m_Header), m_pBuffer, m_pImp->GetSaveInfo(), nullptr);
    }
}




void MemoryCard::List(const char *filter, RealmcIface::TitleInfo *titleInfo) {
    SetExtraParam(ST_PROFILE, nullptr, nullptr, 0);

    m_EntryCount = 0;
    bStrCat(m_Filename, m_pImp->GetPrefix(), "*");

    InitCommand(MO_List);

    if (!Joylog::IsReplaying()) {
        m_pIMemcard->FindEntries(filter != nullptr ? filter : m_Filename, titleInfo);
    } else {
        ReplayJoyOp();
    }
}








void MemoryCard::Load(const char *filename) {
    SetExtraParam(ST_PROFILE, filename, nullptr, FEDatabase->GetUserProfileSaveSize(false));



    FEDatabase->AllocBackupDB(true);


    m_pBuffer = static_cast<char *>(bMalloc(m_DataSize, "Load Buffer", __LINE__, 0x40));

    // El prototipo no comprueba el nombre.
#ifndef EA_BUILD_A124
    if (filename != nullptr)
#endif
    {
        bStrNCpy(MemoryCardImp::gContentName, filename, 16);
        bStrCat(m_Filename, m_pImp->GetPrefix(), filename);
    }

    InitCommand(MO_Load);

    if (!Joylog::IsReplaying()) {
        // En PS2 la carga no pasa por BootupCheck durante el arranque (A124 y SLUS); en PC tampoco.
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32)
        if (m_bInBootSequence) {
            m_bAutoLoading = true;
            BootupCheck(filename);
            return;
        }
#endif
        m_pIMemcard->Load(m_Filename, nullptr, nullptr, reinterpret_cast<const wchar_t *>(MemoryCardImp::gContentName), nullptr);
    }
}

void MemoryCard::Delete(const char *filename) {
    InitCommand(MO_Delete);
    // El prototipo borra por el nombre que recibe, sin prefijo ni copia.
#ifndef EA_BUILD_A124
    if (filename != nullptr) {
        bStrNCpy(MemoryCardImp::gContentName, filename, 16);
        bStrCat(m_Filename, m_pImp->GetPrefix(), filename);
    }
#endif

    if (!Joylog::IsReplaying()) {
#ifdef EA_BUILD_A124
        m_pIMemcard->Delete(filename, reinterpret_cast<const wchar_t *>(MemoryCardImp::gContentName));
#else
        m_pIMemcard->Delete(m_Filename, reinterpret_cast<const wchar_t *>(MemoryCardImp::gContentName));
#endif
    }
}

void MemoryCard::ListOldSaveFilesNGC() {
    // En PS2 esta vacia (8 bytes en la A124): alli se buscan las partidas antiguas en ListOldSaveFilesPS2.
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // PC: vacia (speed.exe 0x518B30, un ret)
    RealmcIface::TitleInfo titleInfo;

    titleInfo.Init(RealmcIface::TITLE_ALTERNATE, 0, RealmcIface::NAME_ENTRY, RealmcIface::FORMAT_LAYER2);

    GetInstance()->ShowMessages(false);
    List("NFSU2*", &titleInfo);
#endif
}

// Orden del original (DWARF de GameCube): NGC, PC, PS2 y Xbox, cada una con cuerpo solo en su plataforma.
void MemoryCard::ListOldSaveFilesPC() {
#ifdef EA_PLATFORM_WIN32 // speed.exe 0x518B40 (200 B): las partidas de Underground 2 en la carpeta local del usuario
    char path[256];
    WIN32_FIND_DATAA findData;
    SHGetFolderPathA(0, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, 0, 0, path);
    bStrCat(path, path, "\\");
    bStrCat(path, path, "NFS Underground 2");
    bStrCat(path, path, "\\*");
    HANDLE hFind = FindFirstFileA(path, &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        unsigned int numFiles = 1;
        while (FindNextFileA(hFind, &findData)) {
            numFiles++;
        }
        if (numFiles > 2) { // "." y ".." no cuentan
            GetInstance()->m_bOldSaveFileExists = true;
        }
        FindClose(hFind);
    }
    EndListingOldSaveFiles();
#endif
}
void MemoryCard::ListOldSaveFilesPS2() {
#ifdef EA_PLATFORM_PLAYSTATION2
    char path[64];
    char *oldPrefix = GetOldPrefix();
    RealmcIface::TitleInfo titleInfo;

    bSPrintf(path, "%s*/%s*", oldPrefix, oldPrefix);
    titleInfo.Init(RealmcIface::TITLE_ALTERNATE, oldPrefix, RealmcIface::NAME_PATH, RealmcIface::FORMAT_LAYER2);

    GetInstance()->ShowMessages(false);
    List(path, &titleInfo);
#endif
}

void MemoryCard::ListOldSaveFilesXbox() {}













void MemoryCard::ReleasePendingMessage() {
    if (m_PendingMessage != nullptr) {
        delete m_PendingMessage;
        m_PendingMessage = nullptr;
    }
}

void MemoryCard::HandleAutoSaveError() {
    UIMemcardBase *pScreen = m_pFEScreen;

    if (gMemcardSetup.GetCommand() == MCO_AutoSave || pScreen != nullptr) {
        pScreen->HandleAutoSaveError();
    } else {
        MemcardEnter(nullptr, nullptr, 0x91, nullptr, nullptr, 0, 0);
    }
}





void MemoryCard::HandleAutoSaveOverwriteMessage() {
    UIMemcardBase *pScreen = m_pFEScreen;

    if (gMemcardSetup.GetCommand() == MCO_AutoSave || pScreen != nullptr) {
        pScreen->HandleAutoSaveOverwriteMessage();
    } else {
        MemcardEnter(nullptr, nullptr, 0xd1, nullptr, nullptr, 0, 0);
    }
}





#ifdef EA_BUILD_A124
void MemoryCard::ShowTRCCheckCardMessage() {
    ShowTRCMessage(Realmc::Locale::GetString(6, "s", GetPortString()), 0, nullptr);
}


void MemoryCard::ShowTRCLoadWarning() {
    ShowTRCMessage(Realmc::Locale::GetString(15, "s", GetPortString()), 0, nullptr);
}


void MemoryCard::HideTRCMessage() {
    if (GetScreen() != nullptr) {
        cFEng::Get()->QueueGameMessage(0x461A18EE, nullptr, 0xFF);
    }
}

void MemoryCard::ShowTRCMessage(const wchar_t *msg, unsigned int nOptions, const wchar_t **options) {
    UIMemcardBase *pScreen = GetScreen();

    if (pScreen != nullptr) {
        if (options != nullptr) {
            pScreen->ShowMessage(msg, nOptions, options[0], options[1], options[2]);
        } else {
            pScreen->ShowMessage(msg, nOptions, nullptr, nullptr, nullptr);
        }
    } else {
        m_PendingMessage = new ("Memcard Msg", 0) MemoryCardMessage(msg, nOptions, options);
        MemcardEnter(nullptr, nullptr, 0x40E1, nullptr, nullptr, 0, 0);
    }
}
#else
void MemoryCard::ShowAutoSaveIcon() {
    if (!m_bAutoSaveIconShowing) {
        m_bAutoSaveIconShowing = true;

        if (!cFEng::Get()->IsPackagePushed("Autosave_Overlay.fng")) {
            cFEng::Get()->PushNoControlPackage("Autosave_Overlay.fng", static_cast<FE_PACKAGE_PRIORITY>(0x68));
        }

        cFEng::Get()->QueuePackageMessage(FEHashUpper("SAVE"), "Autosave_Overlay.fng", nullptr);

        unsigned int msg;
        bool bWidescreen = FEDatabase->GetVideoSettings()->WideScreen;

        if (GRaceStatus::Exists() && GRaceStatus::Get().GetRaceParameters() != nullptr &&
            GRaceStatus::Get().GetRaceParameters()->GetIsDDayRace()) {
            msg = FEHashUpper(bWidescreen ? "LEFT_WIDE" : "LEFT_NORM");
        } else if (cFEng::Get()->IsPackagePushed("SMS_MailBoxes.fng") || GManager::Get().GetHasPendingSMS()) {
            msg = FEHashUpper("SMS_MAILBOX");
            cFEng::Get()->QueuePackageMessage(FEHashUpper("HIDE_INCOMING"), nullptr, nullptr);
        } else {
            msg = FEHashUpper(bWidescreen ? "RIGHT_WIDE" : "RIGHT_NORM");
        }

        cFEng::Get()->QueuePackageMessage(msg, "Autosave_Overlay.fng", nullptr);
    }
}

void MemoryCard::HideAutoSaveIcon() {
    if (m_bAutoSaveIconShowing) {
        m_bAutoSaveIconShowing = false;
        cFEng::Get()->QueuePackageMessage(FEHashUpper("SAVE_DONE"), "Autosave_Overlay.fng", nullptr);
        cFEng::Get()->QueuePackageMessage(FEHashUpper("SHOW_INCOMING"), nullptr, nullptr);
    }
}

bool MemoryCard::IsAutoSaveIconVisible() {
    if (m_bAutoSaveIconShowing ||
        FEngIsScriptSet("Autosave_Overlay.fng", FEHashUpper("AUTOSAVE_ICON_GROUP"), FEHashUpper("LEAVE")) ||
        FEngIsScriptSet("Autosave_Overlay.fng", FEHashUpper("AUTOSAVE_ICON_GROUP"), FEHashUpper("APPEAR"))) {
        return true;
    }

    return false;
}
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
unsigned short *MemoryCard::GetPortString() {
    static unsigned short sBuf[4];

    sBuf[0] = '1';
    sBuf[1] = 0;
    if (GetCurrentLanguage() == eLANGUAGE_FRENCH) {
        sBuf[0] = 'N';
        sBuf[1] = '\xb0';
        sBuf[2] = '1';
        sBuf[3] = 0;
    }

    return sBuf;
}
#endif
