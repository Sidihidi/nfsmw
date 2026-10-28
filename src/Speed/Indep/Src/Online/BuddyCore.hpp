#ifndef ONLINE_BUDDYCORE_HPP
#define ONLINE_BUDDYCORE_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>

#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/Online/LobbyChat.hpp"
#include "Speed/Indep/Src/Online/NetworkCore.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"

struct HLBApiRefT;
struct HLBBudT;
struct LobbyChalRefT;
struct LobbyApiMsgT;
struct BuddyCore;

char *GetLocalizedString(uint32 hash);
int FEngSNPrintf(char *buffer, int32 bufferSize, const char *format, ...);

// A124 (tipos 22301 y 22307, antes de BuddyApiMsgT): tablas de utf8 de DirtySock (UIOnlineChatDialog).
#include "Speed/Indep/Libs/dirtysock/include/lobbyutf8.h"

struct BuddyApiMsgT {
    int iType;
    int iKind;
    int iCode;
    unsigned int uTimeStamp;
    unsigned int uUserFlags;
    char *pData;
};

// A124 (tipo 22328, 39 B): lo devuelve HLBApiUserFound (UIOnlineEA_Messenger::SearchForUserCB).
struct BuddyApiUserT {
    char name[17];
    char domain[5];
    char product[17];
};

template <void (BuddyCore::*Callback)(int, int)> struct buddy_op_callback;
template <void (BuddyCore::*Callback)(BuddyApiMsgT *)> struct buddy_msg_callback;
template <void (BuddyCore::*Callback)(LobbyApiMsgT *)> struct challenge_op_callback;
template <void (BuddyCore::*Callback)(const char *)> struct buddy_print_callback;

typedef void (*BuddyConnectCallback)(int status, void *context);

enum state_type {
    disconnected = 0,
    connected = 1
};

struct InvitedPlayers {
    bool hasAccepted;
    char name[16];

    // 4 B en la A124 (bool de 4 B) y en PC (bool de 1 B: speed.exe 0x78C5C0 pone a cero un dword): no es sizeof(bool)
    InvitedPlayers() { bMemSet(&hasAccepted, 0, 4); }
};

// A124 (tipo 22359, 0x110, nace justo antes de VOIP_ChallengeInfo; ctor en L41@BuddyCore.hpp): la usan
// uiOnlineChallengeInvite (miembro m_challengeInfo) y el reto propio gSelfChallengeInfo.
struct ChallengeInfo {
    bool isActive;
    int gameIdent;
    int gameMode;
    int ranked;
    int numPlayers;
    int collisionDetection;
    int performanceMatchingLevel;
    int gameBreaker;
    int numLaps;
    uint32 eventHash;
    int trackDirection;
    char hostName[32];
    char challengeName[32];
    char gameRoomName[128];
    char password[32];
    int privateSession;

    ChallengeInfo() { bMemSet(&isActive, 0, sizeof(bool)); }
};

struct VOIP_ChallengeInfo {
    bool isActive;
    char hostName[192];
    char challengerName[32];
    char gameRoomName[128];

    VOIP_ChallengeInfo() { bMemSet(&isActive, 0, 4); }
};

struct BuddySettings {
    int gameIdent;
    int BuddyWantsToPlay;
    int IWannaPlayBuddy;
    int voipState;
    int canVoiceChat;
    int isTemporary;
    int isRealBuddy;
    int IsWannaBeMyBuddy;
    int IsIWannaBeHisBuddy;
    int isTempBuddy;
    int isInGroup;
    int buddyState;
    int isBlocked;
    int isPassive;
    int IsAvailableForChat;
    int IsSameProduct;
    int IsJoinable;
    int IsNoReplyBud;
    int unreadMsgCount;
    int totalMsgCount;
    unsigned int gameInviteFlags;
    char *gameBuddyIsPlaying;
    HLBBudT *pBuddy;
    char buddyName[32];
    char presence[256];
    char presenceExtra[256];
};

enum HLBStatE {
    HLB_STAT_UNDEFINED = -1,
    HLB_STAT_DISC = 0,
    HLB_STAT_CHAT = 1,
    HLB_STAT_AWAY = 2,
    HLB_STAT_XA = 3,
    HLB_STAT_DND = 4,
    HLB_STAT_PASSIVE = 5
};

// A124 (tipo 22344): banderas de HLBBudTempBuddyIs / HLBListFlagTempBuddy.
enum HLBTempBuddyTypeE {
    HLB_TEMPBUDDYTYPE_MESG = 1,
    HLB_TEMPBUDDYTYPE_CHAL = 2,
    HLB_TEMPBUDDYTYPE_TOUR = 4,
    HLB_TEMPBUDDYTYPE_VOIP = 8,
    HLB_TEMPBUDDYTYPE_CUST1 = 16,
    HLB_TEMPBUDDYTYPE_CUST2 = 32,
    HLB_TEMPBUDDYTYPE_SESSION = 64,
    HLB_TEMPBUDDYTYPE_MAX = 128
};

struct RichPresenceMsg {
    char IPaddress[32];
    uint32 eventHash;
    uint32 isRacing;
    char session[32];
    uint32 isSessionPassworded;
    char password[32];
    uint32 isSessionPrivate;

    RichPresenceMsg() {}
    inline void Default() {
        bMemSet(this, 0, sizeof(*this));
        bStrCpy(IPaddress, NetworkCore::MyIPAddressText());
    }
};

struct RichPresence {
  private:
    RichPresenceMsg mCurrentRichPresence;
    HLBStatE mBuddyState;

  public:
    RichPresence();
    ~RichPresence();
    static RichPresence *Instance();
    void Init();
    void SetEventHash(uint32 eventHash);
    void SetSession(const char *session);
    void SetBuddyState(HLBStatE buddyState);
    void SetIsRacing(bool isRacing);
    void SetSessionPasswordProtected(bool isPasswordProtected);
    void SetPassword(const char *password);
    void SetSessionPrivate(bool isPrivate);
    void UpdatePresence();
};

struct BuddyCore {
    InvitedPlayers m_invitedPlayers[5];
    char savedNames[10][16];

  private:
    void connectchanged(int op, int status);
    static void gameinvite(HLBApiRefT *api, HLBBudT *bud, int action, void *context);
    void debugoutput(const char *str);
    void buddyListChangedCallback(int op, int opStatus);
    void messageCallback(BuddyApiMsgT *pMsg);
    void LobbyChallengeCB(LobbyApiMsgT *pMesg);
    static void InviteCB(const char *fromPlayer, int gameIdent, LobbyChatN::CBReason reason, void *context);

    template <void (BuddyCore::*Callback)(int, int)> friend struct buddy_op_callback;
    template <void (BuddyCore::*Callback)(BuddyApiMsgT *)> friend struct buddy_msg_callback;
    template <void (BuddyCore::*Callback)(LobbyApiMsgT *)> friend struct challenge_op_callback;
    template <void (BuddyCore::*Callback)(const char *)> friend struct buddy_print_callback;

    HLBApiRefT *HLBud;
    state_type state;
    BuddyConnectCallback connectcallback;
    void *connectcontext;
    LobbyChalRefT *m_lobbyChalRefT;
    int m_lastVOIPstate;
    VOIP_ChallengeInfo m_voipChallenge[6];
    int m_realBuddys;
    int m_tempBuddys;
    int m_blockedBuddys;
    int m_friendRequestBuddys;
    bool m_InVoiceChat;
    int m_myVoiceChatChannel;
    int m_buddyVoiceChatChannel;
    bool m_networkCableUnplugged;
    bool m_paused;

  public:
    BuddyCore();
    static BuddyCore *instance();
    int startconnect(const char *name, const char *passwd, BuddyConnectCallback callback, void *context);
#ifdef EA_PLATFORM_XENON
    // Xbox 360: sin HLB; los llamantes del EGM pasan 0 (UIOnlineChatDialog 0x822923F0: `li r3, 0` a HLBListFlagTempBuddy).
    HLBApiRefT *getApiRef() { return nullptr; }
#else
    HLBApiRefT *getApiRef() { return HLBud; }
#endif
    // A124: inline (L160@BuddyCore.hpp; lw 264(v0) en DisplayHLBerrorType y ~UIOnlineEA_Messenger).
    state_type GetConnectedState() { return state; }
    // A124: inlines de L133/L134/L151@BuddyCore.hpp (UIOnlineEAM_Options: lw/sw 2436(v0) y lw 280(v0)).
    // El nombre del parametro de SetVoiceChatState no sale en el .mdebug (solo se expande con constante).
    bool InVoiceChat() { return m_InVoiceChat; }
    void SetVoiceChatState(bool inVoiceChat) { m_InVoiceChat = inVoiceChat; }
    int getLastVOIPState() { return m_lastVOIPstate; }
    // A124: inlines de L139/L140@BuddyCore.hpp (uiOnlineVoiceInvite: sw 2440(v0), sw 2444(v0), lw 2444(v0)). El
    // setter del canal del amigo se llama GetBuddyChatChannel(int) en los stabs; los parametros no salen.
    int GetBuddyChatChannel() { return m_buddyVoiceChatChannel; }
    void SetMyChatChannel(int channel) { m_myVoiceChatChannel = channel; }
    void GetBuddyChatChannel(int channel) { m_buddyVoiceChatChannel = channel; }
    void disconnect();
    void handledisconnect();
    void doprocessing();
    void pause();
    int resume();
    void initVoiceAndPresence();
    void clearEAMStatusIcons();
    void setNetworkCableUnpluggedFlag();
    void SetPresence(RichPresenceMsg richPresenceMsg, HLBStatE buddyState);
    RichPresenceMsg GetRichPresence(BuddySettings *buddySettings);
    void DisplayVOIPChatEnded();
    void DisplayDeclinedInviteInGame(const char *name);
    void DisplayDeclinedInvite(const char *name);
    void DisplayBlockedInvite(const char *name);
    void DisplayTimedOutInvite(const char *name);
    void DisplayRevokedInvite(const char *name);
    inline void DisplayBusyInvite(const char *name) {
        char sztemp[128];
        FEngSNPrintf(sztemp, sizeof(sztemp), GetLocalizedString(0x6c02f6e1), name);
        DialogInterface::ShowOneButton("", "", dialog_alert, 0x417b2601, 0xffffffff, 0xffffffff, sztemp);
    }
    LobbyChalRefT *GetLobbyChallengeApiRef();
    int GetNextAvailableVOIPSlot();
    bool DoIHaveAVOIPInviteFromThisBuddy(char *buddyName);
    VOIP_ChallengeInfo *GetVOIPChallengeInfo(char *buddyName);
    void RemoveVOIPChallenge(char *buddyName);
    void AddTempBuddy(char *name, int tempType);
    void CountBuddys();
    int GetNumRealBuddys();
    int GetNumberTempBuddys();
    int GetNumBlockedBuddys();
    int GetNumFriendRequests();
    int getBuddyCount();
    HLBBudT *getBuddyByIndex(int index);
    HLBBudT *getBuddyByName(const char *name);
};

template <void (BuddyCore::*Callback)(int, int)> struct buddy_op_callback {
    static void invoke(HLBApiRefT *, int op, int status, void *context) {
        (static_cast<BuddyCore *>(context)->*Callback)(op, status);
    }
};

template <void (BuddyCore::*Callback)(BuddyApiMsgT *)> struct buddy_msg_callback {
    static void invoke(HLBApiRefT *, BuddyApiMsgT *message, void *context) {
        (static_cast<BuddyCore *>(context)->*Callback)(message);
    }
};

template <void (BuddyCore::*Callback)(LobbyApiMsgT *)> struct challenge_op_callback {
    static void invoke(LobbyChalRefT *, LobbyApiMsgT *message, void *context) {
        (static_cast<BuddyCore *>(context)->*Callback)(message);
    }
};

template <void (BuddyCore::*Callback)(const char *)> struct buddy_print_callback {
    static void invoke(void *context, const char *str) {
        (static_cast<BuddyCore *>(context)->*Callback)(str);
    }
};

extern "C" {
// Devuelve el codigo HLB (UIOnlineEA_Messenger::ShowBuddyOptions lo pasa a DisplayHLBerrorType).
int HLBListFlagTempBuddy(HLBApiRefT *api, const char *name, int flags);
// A124: devuelve el codigo HLB y los dos ultimos son callback y contexto (UIOnlineEAM_Options::EvaluateGameInviteStatus
// pasa options_hlb_op_callback<CancelInviteCB>::call y this, y usa el resultado); los demas pasan 0, 0.
int HLBListAnswerGameInvite(HLBApiRefT *api, const char *name, int answer, void (*)(HLBApiRefT *, int, int, void *), void *);
void HLBListCancelGameInvite(HLBApiRefT *, int, int);
int HLBBudGetVOIPState(HLBBudT *);
void LobbyChalDeclineChallenge(LobbyChalRefT *challenge, int reason);
// Los tres llamantes de zFe (EAM_Options y uiOnlineVoiceInvite) pasan li a1,1: dos argumentos.
void LobbyChalCancelChallenge(LobbyChalRefT *, int);
void LobbyChalAcceptChallenge(LobbyChalRefT *);
void LobbyChalInitiateChallenge(LobbyChalRefT *, const char *, int, const char *, int);
// EA Messenger (EAMessenger.cpp, UIOnlineEA_Messenger.cpp): tipos segun el uso en la A124.
const char *HLBBudGetName(HLBBudT *);
unsigned int HLBBudGetGameInviteFlags(HLBBudT *);
int HLBBudCanVoiceChat(HLBBudT *);
int HLBBudIsTemporary(HLBBudT *);
int HLBBudIsWannaBeMyBuddy(HLBBudT *);
int HLBBudIsIWannaBeHisBuddy(HLBBudT *);
int HLBBudTempBuddyIs(HLBBudT *, int);
int HLBBudIsBlocked(HLBBudT *);
int HLBBudIsRealBuddy(HLBBudT *);
char *HLBBudGetTitle(HLBBudT *);
int HLBBudIsInGroup(HLBBudT *);
int HLBBudGetState(HLBBudT *);
int HLBBudIsPassive(HLBBudT *);
int HLBBudIsAvailableForChat(HLBBudT *);
int HLBBudIsSameProduct(HLBBudT *);
int HLBBudIsJoinable(HLBBudT *);
int HLBBudIsNoReplyBud(HLBBudT *);
void HLBBudGetPresence(HLBBudT *, char *);
void HLBBudGetPresenceExtra(HLBBudT *, char *, int);
int HLBMsgListGetUnreadCount(HLBBudT *);
int HLBMsgListGetTotalCount(HLBBudT *);
void HLBListClearGroup(HLBApiRefT *);
void HLBListSort(HLBApiRefT *);
int HLBListAddToGroup(HLBApiRefT *, const char *);
int HLBListRemoveFromGroup(HLBApiRefT *, const char *);
int HLBApiFindUsers(HLBApiRefT *, const char *, void (*)(HLBApiRefT *, int, int, void *), void *);
BuddyApiUserT *HLBApiUserFound(HLBApiRefT *, int);
// UIOnlineChatDialog: tipos segun el uso en la A124.
BuddyApiMsgT *HLBMsgListGetFirstUnreadMsg(HLBBudT *, int, int);
void HLBMsgListDeleteAll(HLBBudT *, int);
void HLBApiCancelOp(HLBApiRefT *);
int HLBListSendChatMsg(HLBApiRefT *, const char *, const char *, void (*)(HLBApiRefT *, int, int, void *), void *);
int HLBListSendMsgToGroup(HLBApiRefT *, int, const char *, int, void (*)(HLBApiRefT *, int, int, void *), void *);
// uiOnlineChallengeInvite (y UIOnlineEAM_Options, que pasa un callback).
int HLBListBlockBuddy(HLBApiRefT *, const char *, void (*)(HLBApiRefT *, int, int, void *), void *);
// UIOnlineEAM_Options: tipos segun el uso en la A124.
int HLBListAnswerInvite(HLBApiRefT *, const char *, int, void (*)(HLBApiRefT *, int, int, void *), void *);
int HLBListUnBlockBuddy(HLBApiRefT *, const char *, void (*)(HLBApiRefT *, int, int, void *), void *);
int HLBListInviteBuddy(HLBApiRefT *, const char *, void (*)(HLBApiRefT *, int, int, void *), void *);
int HLBListUnMakeBuddy(HLBApiRefT *, const char *, void (*)(HLBApiRefT *, int, int, void *), void *);
void HLBListDeleteTempBuddy(HLBApiRefT *, const char *);
void HLBListUnFlagTempBuddy(HLBApiRefT *, const char *, int);
}

// Definido en la unidad zOnline (0x64a5b8); BuddyCore.cpp lo declara a mano. Lo limpia uiOnlineChallengeInvite.
extern int gChallengeRecieved;

// EA Messenger (frontend): los usan BuddyCore.cpp y LobbyChat.cpp con declaraciones locales.
extern BuddySettings gBuddySettings;
// La A124 los declara juntos antes de g_inviteList (orden de salida de los globales diferidos).
extern const char *gNewBuddyName;
extern int gVOIP_InviteState;
extern int gPrivateRoomFlag;
extern int gBuddyListHasChanged;
extern char PCSearchName[16];
BuddySettings EA_Messenger_GetBuddySettings(HLBBudT *pBuddy);
BuddySettings EA_Messenger_GetBuddySettingsByName(const char *name);
void DisplayHLBerrorType(const char *pkgName, int errorNum);

#endif
