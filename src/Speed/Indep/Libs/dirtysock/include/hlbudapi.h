#ifndef _hlbudapi_h
#define _hlbudapi_h

// DirtySock: el API de nivel alto de buddies (libdirtysock.a(hlbudapi.o) en el mapa de la A124).
// Solo la A124 enlaza esta biblioteca; las firmas publicas son las que declara BuddyCore.cpp
// (extern "C") y los tipos opacos, los forward del mdebug. El estado de la conexion y las
// constantes que HLBApiOverrideConstants cambia salen del asm de la A124.

typedef struct HLBApiRefT HLBApiRefT;
typedef struct HLBBudT HLBBudT;
typedef struct DispListT DispListT;

// estado de presencia (HLBApiPresenceSend); los valores son los de BuddyCore.hpp
typedef enum HLBStatE {
    HLB_STAT_OFFLINE = 0,
    HLB_STAT_ONLINE = 1,
    HLB_STAT_INGAME = 2,
    HLB_STAT_DISCONNECTED = 3
} HLBStatE;

#ifdef __cplusplus
extern "C" {
#endif

HLBApiRefT *HLBApiCreate2(void *(*pMemAlloc)(int), void (*pMemFree)(void *), const char *pProduct,
                          const char *pVersion);
void HLBApiSetDebugFunction(HLBApiRefT *pApi, void *pDebugRef, void (*pDebugFn)(void *, const char *));
void HLBApiOverrideConstants(HLBApiRefT *pApi, int iNoText, int iSuspendPort, int iRosterTimeout,
                             int iMaxIgnoreCount, int iMaxTempBuddyCount, int iMaxTotalMsgCount,
                             int iGameInviteTimeout, int iMaxPreConnBuddyCount, int iPad10,
                             int iGameInviteTimeout2);
int HLBApiInitialize(HLBApiRefT *pApi, const char *pServerPort, int iProduct, const char *pLanguage);
int HLBApiConnect(HLBApiRefT *pApi, const char *pServer, int iPort, const char *pPassword, void *pContext);
int HLBApiRegisterConnectCallback(HLBApiRefT *pApi, void (*pCallback)(HLBApiRefT *, int, int, void *),
                                  void *pRef);
int HLBApiGetConnectState(HLBApiRefT *pApi);
void HLBApiDisconnect(HLBApiRefT *pApi);
void HLBApiDestroy(HLBApiRefT *pApi);
void HLBApiUpdate(HLBApiRefT *pApi);
int HLBApiSuspend(HLBApiRefT *pApi);
void HLBApiResume(HLBApiRefT *pApi);
void HLBApiCancelOp(HLBApiRefT *pApi);
int HLBApiFindUsers(HLBApiRefT *pApi, const char *pSearch);
int HLBApiUserFound(HLBApiRefT *pApi, int iIndex);
int HLBApiRegisterNewMsgCallback(HLBApiRefT *pApi, void (*pCallback)(HLBApiRefT *, void *, void *),
                                 void *pRef);
int HLBApiRegisterBuddyChangeCallback(HLBApiRefT *pApi, void (*pCallback)(HLBApiRefT *, int, int, void *),
                                      void *pRef);
int HLBApiRegisterGameInviteCallback(HLBApiRefT *pApi,
                                     void (*pCallback)(HLBApiRefT *, HLBBudT *, int, void *), void *pRef);
int HLBApiPresenceSend(HLBApiRefT *pApi, HLBStatE eState, int iVoipState, const char *pPresence,
                      int iWaitForCompletion);
void HLBApiPresenceDiff(HLBApiRefT *pApi, const char *pDomain, const char *pLanguage);
int HLBApiPresenceVOIPSend(HLBApiRefT *pApi, int iVoipState);

const char *HLBBudGetName(HLBBudT *pBud);
int HLBBudIsTemporary(HLBBudT *pBud);
int HLBBudIsBlocked(HLBBudT *pBud);
int HLBBudIsWannaBeMyBuddy(HLBBudT *pBud);
int HLBBudIsIWannaBeHisBuddy(HLBBudT *pBud);
const char *HLBBudGetTitle(HLBBudT *pBud);
int HLBBudTempBuddyIs(HLBBudT *pBud, int iFlags);
int HLBBudIsRealBuddy(HLBBudT *pBud);
int HLBBudIsInGroup(HLBBudT *pBud);
int HLBBudGetState(HLBBudT *pBud);
int HLBBudIsPassive(HLBBudT *pBud);
int HLBBudIsAvailableForChat(HLBBudT *pBud);
int HLBBudIsSameProduct(HLBBudT *pBud);
int HLBBudGetPresence(HLBBudT *pBud, char *pBuffer);
int HLBBudGetPresenceExtra(HLBBudT *pBud, char *pBuffer, int iBufferSize);
int HLBBudIsNoReplyBud(HLBBudT *pBud);
int HLBBudIsJoinable(HLBBudT *pBud);
int HLBBudGetVOIPState(HLBBudT *pBud);
int HLBBudCanVoiceChat(HLBBudT *pBud);
unsigned int HLBBudGetGameInviteFlags(HLBBudT *pBud);

void *HLBMsgListGetFirstUnreadMsg(HLBBudT *pBud, int iIndex, int iMarkRead);
int HLBMsgListGetUnreadCount(HLBBudT *pBud);
int HLBMsgListGetTotalCount(HLBBudT *pBud);
void HLBMsgListDelete(HLBBudT *pBud, int iIndex);
void HLBMsgListDeleteAll(HLBBudT *pBud, int iUnreadOnly);

HLBBudT *HLBListGetBuddyByName(HLBApiRefT *pApi, const char *pName);
HLBBudT *HLBListGetBuddyByIndex(HLBApiRefT *pApi, int iIndex);
int HLBListGetBuddyCount(HLBApiRefT *pApi);
int HLBListBlockBuddy(HLBApiRefT *pApi, const char *pName, void (*pCallback)(HLBApiRefT *, int, int, void *),
                      void *pRef);
void HLBListUnBlockBuddy(HLBApiRefT *pApi, const char *pName, void (*pCallback)(HLBApiRefT *, int, int, void *),
                         void *pRef);
int HLBListUnMakeBuddy(HLBApiRefT *pApi, const char *pName, void (*pCallback)(HLBApiRefT *, int, int, void *),
                       void *pRef);
int HLBListInviteBuddy(HLBApiRefT *pApi, const char *pName, void (*pCallback)(HLBApiRefT *, int, int, void *),
                       void *pRef);
int HLBListAnswerInvite(HLBApiRefT *pApi, const char *pName, int iAnswer,
                        void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef);
int HLBListAnswerGameInvite(HLBApiRefT *pApi, const char *pName, int iAnswer,
                            void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef);
int HLBListCancelGameInvite(HLBApiRefT *pApi, void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef);
int HLBListFlagTempBuddy(HLBApiRefT *pApi, const char *pName, int iFlags);
int HLBListUnFlagTempBuddy(HLBApiRefT *pApi, const char *pName, int iFlags);
int HLBListDeleteTempBuddy(HLBApiRefT *pApi, const char *pName);
int HLBListSort(HLBApiRefT *pApi);
int HLBListAddToGroup(HLBApiRefT *pApi, const char *pName, int iGroup);
int HLBListRemoveFromGroup(HLBApiRefT *pApi, const char *pName, int iGroup);
int HLBListClearGroup(HLBApiRefT *pApi, int iGroup);
int HLBListSendMsgToGroup(HLBApiRefT *pApi, int iGroup, const char *pText,
                          void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef);
int HLBListSendChatMsg(HLBApiRefT *pApi, const char *pName, const char *pText, int iKind,
                       void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef);

#ifdef __cplusplus
}
#endif

#endif
