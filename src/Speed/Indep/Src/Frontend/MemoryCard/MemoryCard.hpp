#ifndef FRONTEND_MEMORYCARD_MEMORYCARD_H
#define FRONTEND_MEMORYCARD_MEMORYCARD_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"

void InitMemoryCard();

namespace RealmcIface {

struct SaveReq;
struct TitleInfo;
struct MemcardInterface;
struct GameInfo;

enum MessageChoices {
    CHOICE_NONE = 0,
    CHOICE_OPTION1 = 1,
    CHOICE_OPTION2 = 2,
    CHOICE_OPTION3 = 3,
    CHOICE_OPTION4 = 4,
};

// total size: 0x10
struct BootupCheckParams {
    void Clear();

    char *mEntryNamePattern;    // offset: 0x0, size 0x4
    unsigned int mNumSaveTypes; // offset: 0x4, size 0x4
    struct SaveReq **mSaveReqs; // offset: 0x8, size 0x4
    unsigned int mValidCardIds; // offset: 0xC, size 0x4
};

} // namespace RealmcIface

struct MemoryCardMessage;
struct GCIconDataInfo;
struct GCBannerDataInfo;
struct UIMemcardBase;

// total size: 0x198
class MemoryCard {
  public:
    static int GetEntryCount();

    static bool FoundInBoot();

    static int GetImageIndex(const char *name);

    static int GetImageFileProfileNameLength(const char *name);

    void BuildImageDisplayName();

    bool CheckForOldSaveFile();

    static const unsigned short *GetCheckCardString();

    void RefreshActiveCard();

    static void TickCardRemoval();

    void ShowOnlyCreateListMessages();

    void FakeLoad(int iSlot);

    void LoadYNCF(int iSlot);

    void ListOldSaveFilesPC();

    void ListOldSaveFilesPS2();

    void ListOldSaveFilesXbox();

    enum SaveType {
        ST_PROFILE = 0,
        ST_THUMBNAIL = 1,
        ST_IMAGE = 2,
        ST_MAX = 3,
    };

    enum _MemOp {
        MO_NONE = 0,
        MO_BootUp = 1,
        MO_CheckCard = 2,
        MO_Save = 3,
        MO_AutoSave = 4,
        MO_Load = 5,
        MO_Delete = 6,
        MO_List = 7,
        MO_FakeLoad = 8,
        MO_LoadYNCF = 9,
        MO_SetMonitor = 10,
    };

    MemoryCard();

    static bool IsCardAvailable();
    static void SetMessageMode(unsigned int msg, bool flag);

    static void SetExtraParam(SaveType t, const char *filename, void *buf, unsigned int size);
    void InitCommand(int op);
    void RequestTask(int op, const char *name);
    void ProcessTask();
    void Init();
    void StartBootSequence();
    void EndBootSequence();
    int GetPrefixLength();
    const char *GetPrefix();
    static const char *GetLocaleString(int strID);
    void MessageDone(RealmcIface::MessageChoices nInput);
    void BootupCheck(const char *entry);
    void StartAutoSave(bool bForce);
    void DoAutoSave();
    void EndAutoSave();
    void SetMonitor(bool bEnabled);
    void SetAutoSaveEnabled(bool bEnabled);
    void ShowOnlyAutoSaveMessages();
    void CheckCard(int iSlot);
    void Save(const char *entryName);
    void List(const char *filter, RealmcIface::TitleInfo *titleInfo);
    void Load(const char *filename);
    void Delete(const char *filename);
    void ListOldSaveFilesNGC();
    void ReleasePendingMessage();
    void HandleAutoSaveError();
    void HandleAutoSaveOverwriteMessage();
#ifdef EA_BUILD_A124
    // El prototipo no tiene el icono de autoguardado: avisa con mensajes TRC.
    void ShowTRCLoadWarning();
    void ShowTRCCheckCardMessage();
    void HideTRCMessage();
    void ShowTRCMessage(const wchar_t *msg, unsigned int nOptions, const wchar_t **options);
#else
    void ShowAutoSaveIcon();
    void HideAutoSaveIcon();
    bool IsAutoSaveIconVisible();
#endif
#ifdef EA_PLATFORM_PLAYSTATION2
    unsigned short *GetPortString();
#endif

    bool ShouldDoAutoSave(bool b);

    // Estatica: el original la llama sin preparar this.
    static bool IsCardBusy();
    void Tick(int TickCount);

    static void LoadLocale(eLanguages eLang);

    bool IsListingOldSaveFiles() {
        return m_bListingOldSaveFiles;
    }

    void StartListingOldSaveFiles();
    void EndListingOldSaveFiles();

    bool IsAutoSaving() {
        return m_bInAutoSave;
    }

    static MemoryCard *GetInstance() {
        return s_pThis;
    }

#ifndef EA_BUILD_A124
    void CancelNextAutoSave() {
        m_bCancelNextAutoSave = true;
    }
#endif

    void SetCardRemovedWithAutoSaveEnabled(bool bRemoved) {
        m_bCardRemoved = bRemoved;
    }

    void RequestAutoSave() {
        m_bAutoSaveRequested = true;
    }

    bool AutoSaveRequested() {
        return m_bAutoSaveRequested;
    }

    void SetPlayerNum(int player) {
        m_nPlayer = player;
    }

    void SetMemcardScreenShowing(bool bShowing) {
        m_bMemcardScreenShowing = bShowing;
    }

    bool IsMemcardScreenInitialized() {
        return m_bInitialized;
    }

    void SetMemcardScreenInitialized(bool bInit) {
        m_bInitialized = bInit;
    }

#ifdef EA_PLATFORM_GAMECUBE
    void SetMemcardScreenExiting(bool bExiting) {
        m_bMemcardScreenExiting = bExiting;
    }

    bool IsAutoLoadDone() {
        return m_bAutoLoadDone;
    }

    void SetAutoLoadDone(bool bDone) {
        m_bAutoLoadDone = bDone;
    }
#endif

    void ShowMessages(bool bShow);

    // --- accesores en linea (orden del volcado DWARF) ---
    void FEngLinkObjects(UIMemcardBase *pMenuScreen) {
        m_pFEScreen = pMenuScreen;
    }

    bool IsTypeProfile() {
        return m_Type == ST_PROFILE;
    }

    bool IsAutoSave() {
        return m_bAutoSave;
    }

    int GetOp() {
        return m_MemOp;
    }

    unsigned int GetSize() {
        return m_DataSize;
    }

    bool InBootSequence() {
        return m_bInBootSequence;
    }

    int GetPlayerNum() {
        return m_nPlayer;
    }

#ifndef EA_BUILD_A124
    bool PromptForAutoSave() {
        return m_bNonSilentAutoSave;
    }
#endif

    bool IsCheckingCardForAutoSave() {
        return m_bCheckingCardForAutoSave;
    }

    bool IsCheckingCardForOverwrite() {
        return m_bCheckingCardForOverwrite;
    }

    bool IsRetryingAutoSave() {
        return m_bRetryAutoSave;
    }

    void SetRetryAutoSave(bool bRetry) {
        m_bRetryAutoSave = bRetry;
    }

    void SetHUDLoaded() {
        m_bHUDLoaded = true;
    }

#ifndef EA_BUILD_A124
    bool IsMonitorOn() {
        return m_bMonitorOn;
    }
#endif

    bool IsAutoLoading() {
        return m_bAutoLoading;
    }

    bool IsMemcardScreenShowing() {
        return m_bMemcardScreenShowing;
    }

    bool IsListingForCreate() {
        return m_bListingForCreate;
    }

    void SetListingForCreate(bool bListing) {
        m_bListingForCreate = bListing;
    }

    void ResetAutoSaveCardPulled() {
        m_bAutoSaveCardPulled = false;
    }

    bool CardPulledInAutoSave() {
        return m_bAutoSaveCardPulled;
    }

    bool WasCardRemovedWithAutoSaveEnabled() {
        return m_bCardRemoved;
    }

    bool IsManualSave() {
        return m_bManualSave;
    }

#ifdef EA_PLATFORM_GAMECUBE
    bool IsMemcardScreenExiting() {
        return m_bMemcardScreenExiting;
    }
#endif

    MemoryCardMessage *GetPendingMessage() {
        return m_PendingMessage;
    }

    void SetWaitingForResponse(bool bWaiting) {
        m_bWaitingForResponse = bWaiting;
    }

    bool IsWaitingForResponse() {
        return m_bWaitingForResponse;
    }

#ifdef EA_PLATFORM_GAMECUBE
    GCIconDataInfo *GetSaveIcon() {
        return m_pRMIcon;
    }

    GCBannerDataInfo *GetSaveBanner() {
        return m_pRMBanner;
    }
#endif

    static int GetLastError() {
        return s_pThis->m_LastError;
    }

    static int GetSpecialError() {
        return s_pThis->m_SpecialError;
    }

    void SetBootFound(bool b) {
        m_bBootFoundFile = b;
    }

    UIMemcardBase *GetScreen() {
        return m_pFEScreen;
    }

    char *GetHeader() {
        return reinterpret_cast<char *>(m_Header);
    }

    char *GetData() {
        return m_pBuffer;
    }

    static MemoryCard *s_pThis; // size: 0x4

    // Los offsets de los comentarios son los de GameCube.  En PS2, PC y 360 faltan el icono y el banner
    // de GC, m_bMemcardScreenExiting, m_bAutoLoadDone y m_BootupFilename (SLUS: m_pIMemcard en 0x15C,
    // m_bInBootSequence en 0x24); el prototipo A124 no tiene tampoco m_bCancelNextAutoSave, los
    // indicadores de autoguardado, m_pGameInfo ni m_GameTitle (0xD0 en su volcado de tipos).
    RealmcIface::BootupCheckParams m_BootupParams; // offset: 0x0, size 0x10
#ifdef EA_PLATFORM_GAMECUBE
    struct GCIconDataInfo *m_pRMIcon;           // offset: 0x10, size 0x4
    struct GCBannerDataInfo *m_pRMBanner;       // offset: 0x14, size 0x4
#endif
    void *m_pLocaleFileHandler;                 // offset: 0x18, size 0x4
    bool m_bWaitingForResponse;                 // offset: 0x1C, size 0x1
    bool m_bBootFoundFile;                      // offset: 0x20, size 0x1
    bool m_bAutoSave;                           // offset: 0x24, size 0x1
    bool m_bAutoSaveCardPulled;                 // offset: 0x28, size 0x1
    bool m_bInBootSequence;                     // offset: 0x2C, size 0x1
    bool m_bRetryBootCheck;                     // offset: 0x30, size 0x1
    bool m_bManualSave;                         // offset: 0x34, size 0x1
    bool m_bAutoSaveCardPulledDuringSave;       // offset: 0x38, size 0x1
    bool m_bOldSaveFileExists;                  // offset: 0x3C, size 0x1
    bool m_bListingOldSaveFiles;                // offset: 0x40, size 0x1
    bool m_bInAutoSave;                         // offset: 0x44, size 0x1
    bool m_bAutoSaveRequested;                  // offset: 0x48, size 0x1
    bool m_bCheckingCardForAutoSave;            // offset: 0x4C, size 0x1
    bool m_bFoundAutoSaveFile;                  // offset: 0x50, size 0x1
    bool m_bCheckingCardForOverwrite;           // offset: 0x54, size 0x1
    bool m_bMemcardScreenShowing;               // offset: 0x58, size 0x1
    bool m_bCardRemoved;                        // offset: 0x5C, size 0x1
    bool m_bRetryAutoSave;                      // offset: 0x60, size 0x1
    bool m_bInitialized;                        // offset: 0x64, size 0x1
    bool m_bDisablingAutoSaveForSave;           // offset: 0x68, size 0x1
    bool m_bAutoLoading;                        // offset: 0x6C, size 0x1
    bool m_bListingForCreate;                   // offset: 0x70, size 0x1
    bool m_bHUDLoaded;                          // offset: 0x74, size 0x1
#ifndef EA_BUILD_A124
    bool m_bCancelNextAutoSave;                 // offset: 0x78, size 0x1
    bool m_bMonitorOn;                          // offset: 0x7C, size 0x1
    bool m_bAutoSaveIconShowing;                // offset: 0x80, size 0x1
    bool m_bNeedToAllowControllerErrors;        // offset: 0x84, size 0x1
    bool m_bNonSilentAutoSave;                  // offset: 0x88, size 0x1
#endif
#ifdef EA_PLATFORM_GAMECUBE
    bool m_bAutoLoadDone;                       // offset: 0x8C, size 0x1
    bool m_bMemcardScreenExiting;               // offset: 0x90, size 0x1
#endif
    struct MemoryCardMessage *m_PendingMessage; // offset: 0x94, size 0x4
#ifndef EA_BUILD_A124
    RealmcIface::GameInfo *m_pGameInfo;         // offset: 0x98, size 0x4
#endif
    int m_ReqOp;                                // offset: 0x9C, size 0x4
    const char *m_ReqFilename;                  // offset: 0xA0, size 0x4
    int m_MemOp;                                // offset: 0xA4, size 0x4
    char *m_pBuffer;                            // offset: 0xA8, size 0x4
    unsigned short m_LastError;                 // offset: 0xAC, size 0x2
    unsigned short m_SpecialError;              // offset: 0xAE, size 0x2
    int m_EntryCount;                           // offset: 0xB0, size 0x4
    int m_nPlayer;                              // offset: 0xB4, size 0x4
    unsigned int m_Header[2];                   // offset: 0xB8, size 0x8
    char m_Filename[32];                        // offset: 0xC0, size 0x20
#ifdef EA_PLATFORM_GAMECUBE
    char m_BootupFilename[32];                  // offset: 0xE0, size 0x20
#endif
#ifndef EA_BUILD_A124
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    unsigned short m_GameTitle[32];             // PC: offset 0x7C, size 0x40 (el wchar_t de MSVC es de 2 bytes)
#else
    unsigned short m_GameTitle[64];             // offset: 0x100, size 0x80
#endif
#endif
    SaveType m_Type;                            // offset: 0x180, size 0x4
    unsigned int m_DataSize;                    // offset: 0x184, size 0x4
    int m_TimeOffsetSec;                        // offset: 0x188, size 0x4
    RealmcIface::MemcardInterface *m_pIMemcard; // offset: 0x18C, size 0x4
    struct UIMemcardBase *m_pFEScreen;          // offset: 0x190, size 0x4
    struct MemoryCardImp *m_pImp;               // offset: 0x194, size 0x4
};

void MemcardEnter(const char *pkg_name, const char *screen_name, unsigned int arg2, void (*callback)(void *), void *param, unsigned int arg5, unsigned int arg6);

#endif
