// DirtySock: hlbudapi.c, el API de nivel alto de buddies sobre buddyapi.c.
// Escrito del asm de la A124 (libdirtysock.a(hlbudapi.o), .text 0x48E968, 0x3B0C): la
// biblioteca solo enlaza en el prototipo; SLUS no la tiene. Los offsets de HLBApiRefT
// (0xD4 B), HLBBudT (0x14 B), el registro de buddyapi (0x11C B) y el mensaje guardado
// (0x18 B) salen de ese asm.
#include "hlbudapi.h"

#include <string.h>
#include <stdio.h>

// --- buddyapi.c (libdirtysock.a(buddyapi.o), aun sin decompilar): firmas del uso que
// hace hlbudapi; las aridades son las que marcan los registros que lee el asm (en el
// EE los argumentos 5-8 van en $t0-$t3).
typedef struct BuddyApiRefT BuddyApiRefT;
typedef struct BuddyApiBuddyT BuddyApiBuddyT;
typedef struct BuddyApiMsgT BuddyApiMsgT;
typedef int (*BuddyApiSortFnT)(BuddyApiBuddyT *, BuddyApiBuddyT *);
typedef void (*BuddyApiCallbackT)(BuddyApiRefT *, BuddyApiMsgT *, void *);

extern BuddyApiRefT *BuddyApiCreate2(void *(*pMemAlloc)(int), void (*pMemFree)(void *), const char *pProduct,
                                     const char *pVersion, BuddyApiSortFnT pSortFn);
extern void BuddyApiDebug(BuddyApiRefT *pApi, void *pDebugRef);
extern int BuddyApiConfig(BuddyApiRefT *pApi, const char *pSetting, int iValue, const char *pValue,
                          int iValue2, int iValue3);
extern int BuddyApiConnect(BuddyApiRefT *pApi, const char *pServer, int iPort, int iPassword,
                           const char *pPasswordStr, void *pContext, BuddyApiCallbackT pCallback, void *pRef);
extern int BuddyApiDisconnect(BuddyApiRefT *pApi);
extern void BuddyApiDestroy(BuddyApiRefT *pApi);
extern int BuddyApiUpdate(BuddyApiRefT *pApi);
extern int BuddyApiRoster(BuddyApiRefT *pApi, BuddyApiCallbackT pCallback, void *pRef);
extern DispListT *BuddyApiRosterList(BuddyApiRefT *pApi);
extern int BuddyApiFlush(BuddyApiRefT *pApi);
extern BuddyApiBuddyT *BuddyApiFind(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy);
extern int BuddyApiAdd(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, BuddyApiCallbackT pCallback, void *pRef,
                       int iTimeout);
extern int BuddyApiDel(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, BuddyApiCallbackT pCallback, void *pRef,
                       int iTimeout);
extern int BuddyApiSend(BuddyApiRefT *pApi, BuddyApiMsgT *pMsg, int iKind, BuddyApiCallbackT pCallback,
                        void *pRef, int iTimeout);
extern int BuddyApiBroadcast(BuddyApiRefT *pApi, BuddyApiMsgT *pMsg, int iKind, BuddyApiCallbackT pCallback,
                             void *pRef, int iTimeout);
extern int BuddyApiBuddyInvite(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, BuddyApiCallbackT pCallback,
                               void *pRef, int iTimeout);
extern int BuddyApiRespondBuddy(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, int iRespond,
                               BuddyApiCallbackT pCallback, void *pRef, int iTimeout);
extern int BuddyApiRespondGame(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, int iRespond,
                               BuddyApiCallbackT pCallback, void *pRef, void *pContext, int iTimeout);
extern int BuddyApiFindUsers(BuddyApiRefT *pApi, const char *pSearch, BuddyApiCallbackT pCallback, void *pRef,
                             int iTimeout);
extern int BuddyApiUserFound(BuddyApiRefT *pApi, int iIndex);
extern void BuddyApiRecv(BuddyApiRefT *pApi, BuddyApiCallbackT pCallback, void *pRef);
extern int BuddyApiRegisterBuddyChangeCallback(BuddyApiRefT *pApi, BuddyApiCallbackT pCallback, void *pRef);
extern int BuddyApiRegisterBuddyDelCallback(BuddyApiRefT *pApi, BuddyApiCallbackT pCallback, void *pRef);
extern int BuddyApiResource(BuddyApiRefT *pApi, int iResource, char *pBuffer);
extern void BuddyApiPresInit(BuddyApiRefT *pApi);
extern void BuddyApiPresNoText(BuddyApiRefT *pApi);
extern int BuddyApiPresSend(BuddyApiRefT *pApi, const char *pDomain, int iState, const char *pPresence,
                            int iWaitForCompletion);
extern int BuddyApiPresDiff(BuddyApiRefT *pApi, const char *pDomain, const char *pLanguage);
extern int BuddyApiPresSame();
extern int BuddyApiPresExFlags(BuddyApiRefT *pApi, int iFlags);
extern int BuddyApiRefreshTitle(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy);
extern void BuddyApiSuspendXDK(BuddyApiRefT *pApi);
extern void BuddyApiResumeXDK(BuddyApiRefT *pApi);

// --- dirtynet (DispList), lobbytagfield y lobbyutf8
extern DispListT *DispListCreate(int iSortMode, int iSortDir, int iSortKeySize);
extern void DispListDirty(DispListT *pList, int iMode);
extern int DispListCount(DispListT *pList);
extern void DispListDestroy(DispListT *pList);
extern void *DispListAdd(DispListT *pList, void *pItem);
extern void *DispListIndex(DispListT *pList, int iIndex);
extern void *DispListDelByIndex(DispListT *pList, int iIndex);
extern int DispListChange(DispListT *pList, int iMode);
extern void DispListOrder(DispListT *pList);
extern int DispListSort(DispListT *pList, void *pRef, int (*pSortFn)(void *, void *), int iMode);
extern char *TagFieldFind(const char *pRecord, const char *pName);
extern int TagFieldGetNumber(const char *pField, int iDefault);
extern int TagFieldGetString(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault);
extern void TagFieldSetNumber(char *pBuffer, int iBufferSize, const char *pName, int iValue);
extern void TagFieldSetString(char *pBuffer, int iBufferSize, const char *pName, const char *pValue);
extern int TagFieldDupl(char *pBuffer, int iBufferSize, const char *pField);
extern int Utf8TranslateTo8Bit(char *pDest, int iDestSize, const char *pSource, int iSourceSize);
extern int ds_stricmp(const char *pS1, const char *pS2);
extern void *NetAlloc(int iSize);
extern void NetFree(void *pMem);

// tipos de mensaje de buddyapi (constantes de cuatro letras, en el orden de bytes
// grande: 'auth' == 0x61757468)
#define MSGTYPE_AUTH 0x61757468
#define MSGTYPE_CONN 0x636F6E6E
#define MSGTYPE_HACK 0x6861636B
#define MSGTYPE_BSOD 0x62736F64
#define MSGTYPE_MSRC 0x6D737263
#define MSGTYPE_NETW 0x6E657477
#define MSGTYPE_USER 0x75736572
#define MSGTYPE_SEND 0x73656E64
#define MSGTYPE_BRDC 0x62726463
#define MSGTYPE_ADDU 0x61646475
#define MSGTYPE_DELU 0x64656C75
#define MSGTYPE_INVT 0x696E7674
#define MSGTYPE_GINV 0x67696E76
#define MSGTYPE_EPST 0x65707374
#define MSGTYPE_CHAT 0x63686174
#define MSGTYPE_CLUB 0x636C7562
#define MSGTYPE_ADMN 0x61646D6E
#define MSGTYPE_FULL 0x66756C6C
#define MSGTYPE_GETP 0x70676574

// el registro de buddyapi: 0x11C B; hlbudapi solo toca el nombre (0x00), el estado
// (0xF7), las flags (0xF8), el puntero de vuelta (0x100) y el indice (0x104)
typedef struct BuddyApiBuddyRecT
{
    char Name[0x21];            // +0x00
    unsigned char Pad21[0x35 - 0x21];
    char Presence[0x2A];        // +0x35
    unsigned char Pad5F[0x85 - 0x5F];
    char Title[0x29];           // +0x85
    unsigned char PadAE[0xAE - 0xAE];
    char PresenceExtra[0xF0 - 0xAE]; // +0xAE
    int iVoipState;             // +0xF0
    // Hueco medido: el original lee eState con lb en +0xF7 (3 hits) y voip con lw
    // en +0xF0; entre medias no toca nada. Sin estos 3 B el sizeof sale 0x118 y el
    // memset del original es de 0x11C.
    unsigned char PadF4[3];     // +0xF4
    signed char eState;         // +0xF7
    int uFlags;                // +0xF8
    // Hueco medido: pHLBud va en +0x100 (lw/sw) e iIndex en +0x104; el unico acceso
    // en 0xF4-0xFF es el lb de eState en 0xF7. Sin estos 4 B el sizeof sale 0x118.
    unsigned char PadFC[4];     // +0xFC
    HLBBudT *pHLBud;            // +0x100
    int iIndex;                 // +0x104
    unsigned char Pad108[0x11C - 0x108];
} BuddyApiBuddyRecT;

// el mensaje de buddyapi (los primeros 0x18 B se copian al mensaje guardado)
typedef struct BuddyApiMsgRecT
{
    unsigned int uType;         // +0x00
    unsigned int uKind;         // +0x04
    unsigned int uCode;         // +0x08
    unsigned int uTimeStamp;    // +0x0C
    unsigned int uFlags;        // +0x10
    char *pData;                // +0x14
} BuddyApiMsgRecT;

// el mensaje guardado en la lista de un buddy (0x18 B)
typedef struct HLBMsgRecT
{
    unsigned int uType;         // +0x00
    unsigned short uNew;        // +0x04
    unsigned short uPad06;      // +0x06
    unsigned int uCode;         // +0x08
    unsigned int uTimeStamp;    // +0x0C
    unsigned int uFlags;        // +0x10 (bit 0: leido)
    char *pText;                // +0x14
} HLBMsgRecT;

struct HLBBudT
{
    BuddyApiBuddyRecT *pBuddy;  // +0x00
    unsigned short uNewMsg;     // +0x04
    unsigned short uGroup;      // +0x06
    unsigned short uTempFlags;  // +0x08
    unsigned short uFlags2;     // +0x0A
    DispListT *pMsgList;        // +0x0C
    HLBApiRefT *pApi;           // +0x10
};

typedef struct PreConnTempBudT
{
    char Name[0x24];            // +0x00
    unsigned int uFlags;        // +0x24
} PreConnTempBudT;

struct HLBApiRefT
{
    void *(*pMemAlloc)(int);                             // +0x00
    void (*pMemFree)(void *);                            // +0x04
    BuddyApiRefT *pBuddyApi;                             // +0x08
    int iState;                                          // +0x0C
    int iLastOpState;                                    // +0x10
    int iStatus;                                         // +0x14
    void *pNewMsgRef;                                    // +0x18
    void (*pNewMsgCallback)(HLBApiRefT *, void *, void *); // +0x1C
    void *pBuddyChangeRef;                               // +0x20
    void (*pBuddyChangeCallback)(HLBApiRefT *, int, int, void *); // +0x24
    void *pOpRef;                                        // +0x28
    void (*pOpCallback)(HLBApiRefT *, int, int, void *); // +0x2C
    void *pConnectRef;                                   // +0x30
    void (*pConnectCallback)(HLBApiRefT *, int, int, void *); // +0x34
    int iConnectState;                                   // +0x38
    int bListDirty;                                      // +0x3C
    BuddyApiRefT *pBuddyApi2;                            // +0x40
    void *pDebugRef;                                     // +0x44
    int iPad48;                                          // +0x48
    int iTotalMsgs;                                      // +0x4C
    unsigned int uCurTime;                               // +0x50
    int iBuddyCount;                                     // +0x54
    void (*pGameInviteCallback)(HLBApiRefT *, HLBBudT *, int, void *); // +0x58
    void *pGameInviteRef;                                // +0x5C
    void (*pStatusCallback)(HLBApiRefT *, HLBBudT *, int, void *); // +0x60
    void *pStatusRef;                                    // +0x64
    unsigned int uGameInviteTime;                        // +0x68
    char GameInviteName[0x20];                           // +0x6C
    int iGameInviteTimeout;                              // +0x8C
    int iMaxIgnoreCount;                                 // +0x90
    int iMaxTempBuddyCount;                              // +0x94
    int iMaxBuddyCount;                                  // +0x98
    int iMaxTotalMsgCount;                               // +0x9C
    int iMaxMsgCount;                                    // +0xA0
    int iPadA4;                                          // +0xA4
    int iMaxPreConnBuddyCount;                           // +0xA8
    int iSuspendPort;                                    // +0xAC
    int iRosterTimeout;                                  // +0xB0
    int iGameInviteTimeout2;                             // +0xB4
    int bSuppressPresence;                               // +0xB8
    int bPresNoText;                                     // +0xBC
    void *pUtf8Tbl;                                      // +0xC0
    int bOpPending;                                      // +0xC4
    int iOpError;                                        // +0xC8
    DispListT *pPreConnTempBudList;                      // +0xCC
    // Medido en HLBApiCreate2 (li a0/a2, 0xD4 = 212 en NetAlloc+memset): el
    // sizeof es 0xD4, no 0xD0. Ninguna funcion del asm toca +0xD0 (solo hay
    // accesos a +0xCC en toda la unidad): int sobrante sin uso.
    int iPadD0;                                          // +0xD0
};

void _HLBMsgListDestroy(HLBBudT *pBud);
BuddyApiBuddyT *_setUpOp();

int _msgTypeToOp(unsigned int uType)
{
    if (uType == MSGTYPE_SEND || uType == MSGTYPE_BRDC)
        return 7;
    if (uType == MSGTYPE_ADDU)
        return 2;
    if (uType == MSGTYPE_DELU)
        return 3;
    // El 1 sale del asm del original (li v0,1 en el delay slot + beq a 0xac): no hay
    // MSGTYPE_* que valga 1, es el literal.
    if (uType == 1)
        return 1;
    if (uType == MSGTYPE_USER)
        return 6;
    if (uType == MSGTYPE_EPST)
        return 8;
    // El INVT sale por beql al epilogo comun y el GINV por xor+movz: el if+return
    // separaba el jr y el movn.
    if (uType != MSGTYPE_INVT)
        return uType != MSGTYPE_GINV ? 0xA : 9;
    return 9;
}

DispListT *_getRost(HLBApiRefT *pApi)
{
    return BuddyApiRosterList(pApi->pBuddyApi);
}

void _setLastOpState(HLBApiRefT *pApi, int iState)
{
    if (iState == 0)
    {
        pApi->iLastOpState = -1;
        return;
    }
    pApi->iLastOpState = iState;
}

void _signalBuddyListChanged(HLBApiRefT *pApi)
{
    if (pApi == NULL)
        return;
    pApi->bListDirty = 1;
    if (pApi->pBuddyChangeCallback != NULL)
        pApi->pBuddyChangeCallback(pApi, 5, 0, pApi->pBuddyChangeRef);
}

DispListT *_listCreate(void)
{
    DispListT *pList = DispListCreate(5, 5, 0);
    DispListDirty(pList, 1);
    return pList;
}

int _listGetCount(DispListT *pList)
{
    return DispListCount(pList);
}

void _listDestroy(DispListT *pList)
{
    DispListDestroy(pList);
}

void *_listItemAdd(DispListT *pList, void *pItem)
{
    if (pList != NULL)
        return DispListAdd(pList, pItem);
    return NULL;
}

void *_listItemGetByIndex(DispListT *pList, int iIndex)
{
    return DispListIndex(pList, iIndex);
}

void *_listItemGetPreConnTempBuddyByIndex(DispListT *pList, int iIndex)
{
    return DispListIndex(pList, iIndex);
}

void _listRemovePreConnTempBuddy(DispListT *pList, int iIndex)
{
    DispListDelByIndex(pList, iIndex);
}

void _listMsgItemDestroy(HLBApiRefT *pApi, DispListT *pList, int iIndex)
{
    HLBMsgRecT *pMsg = DispListDelByIndex(pList, iIndex);
    if (pMsg != NULL)
    {
        pApi->pMemFree(pMsg->pText);
        pApi->pMemFree(pMsg);
    }
}

int _rosterCount(HLBApiRefT *pApi)
{
    if (_getRost(pApi) == NULL)
        return 0;
    return DispListCount(_getRost(pApi));
}

int _countBuddies(HLBApiRefT *pApi, unsigned int uFlags)
{
    int iCount = 0;
    int iNum = _rosterCount(pApi);
    int i;
    for (i = 0; i < iNum; i++)
    {
        HLBBudT *pBud = HLBListGetBuddyByIndex(pApi, i);
        if (pBud != NULL && pBud->pBuddy != NULL && (pBud->pBuddy->uFlags & uFlags) != 0)
            iCount = i + 1;
    }
    return iCount;
}

void _HLBDestroyBuddy(HLBBudT *pBud)
{
    if (pBud == NULL)
        return;
    _HLBMsgListDestroy(pBud);
    if (pBud->pBuddy != NULL)
        pBud->pBuddy->pHLBud = NULL;
    pBud->pApi->pMemFree(pBud);
}

void _HLBRosterDestroy(HLBApiRefT *pApi)
{
    if (pApi == NULL)
        return;
    {
        int i;
        for (i = HLBListGetBuddyCount(pApi) - 1; i >= 0; i--)
            _HLBDestroyBuddy(HLBListGetBuddyByIndex(pApi, i));
    }
    BuddyApiFlush(pApi->pBuddyApi);
    pApi->iTotalMsgs = 0;
}

void _preConnTempBudListDestroy(HLBApiRefT *pApi)
{
    if (pApi == NULL || pApi->pPreConnTempBudList == NULL)
        return;
    {
        int i;
        for (i = _listGetCount(pApi->pPreConnTempBudList) - 1; i >= 0; i--)
        {
            PreConnTempBudT *pBud = DispListDelByIndex(pApi->pPreConnTempBudList, i);
            if (pBud == NULL)
                continue;
            pApi->pMemFree(pBud);
        }
    }
    _listDestroy(pApi->pPreConnTempBudList);
    pApi->pPreConnTempBudList = NULL;
}

void _preConnTempBudAdd(HLBApiRefT *pApi, const char *pName, unsigned int uFlags)
{
    PreConnTempBudT *pBud = NULL;
    int iNum;
    int i;
    if (pApi == NULL)
        return;
    if (pApi->pPreConnTempBudList == NULL)
        pApi->pPreConnTempBudList = _listCreate();
    iNum = _listGetCount(pApi->pPreConnTempBudList);
    for (i = 0; i < iNum; i++)
    {
        pBud = _listItemGetPreConnTempBuddyByIndex(pApi->pPreConnTempBudList, i);
        if (strcmp(pBud->Name, pName) == 0)
            break;
    }
    if (i == iNum)
    {
        pBud = pApi->pMemAlloc(sizeof(PreConnTempBudT));
        memset(pBud, 0, sizeof(PreConnTempBudT));
        strncpy(pBud->Name, pName, 0x21);
    }
    else
    {
        _listRemovePreConnTempBuddy(pApi->pPreConnTempBudList, i);
    }
    pBud->uFlags |= uFlags;
    _listItemAdd(pApi->pPreConnTempBudList, pBud);
    while (_listGetCount(pApi->pPreConnTempBudList) >= 10)
        pApi->pMemFree(DispListDelByIndex(pApi->pPreConnTempBudList, 0));
}

void _preConnTempBudListLoad(HLBApiRefT *pApi)
{
    if (pApi == NULL || pApi->pPreConnTempBudList == NULL)
        return;
    {
        int i;
        for (i = _listGetCount(pApi->pPreConnTempBudList) - 1; i >= 0; i--)
        {
            PreConnTempBudT *pBud = DispListIndex(pApi->pPreConnTempBudList, i);
            if (pBud != NULL)
            {
                unsigned int uFlag = 1;
                do
                {
                    if (pBud->uFlags & uFlag)
                        HLBListFlagTempBuddy(pApi, pBud->Name, uFlag);
                    uFlag <<= 1;
                } while (uFlag < 0x80);
                DispListDelByIndex(pApi->pPreConnTempBudList, i);
                pApi->pMemFree(pBud);
            }
        }
    }
    _preConnTempBudListDestroy(pApi);
}

void _HLBConnectCallback(BuddyApiRefT *pBuddyApi, BuddyApiMsgT *pMsg, HLBApiRefT *pApi);
void _HLBOpCallback(BuddyApiRefT *pBuddyApi, BuddyApiMsgT *pMsg, HLBApiRefT *pApi);
void _HLBBuddyChangeCallback(BuddyApiRefT *pBuddyApi, BuddyApiBuddyT *pBuddy, unsigned int uChange,
                                    void *pRef, HLBApiRefT *pApi);

HLBBudT *_constructHLBud(HLBApiRefT *pApi, BuddyApiBuddyT *pBuddyIn)
{
    BuddyApiBuddyRecT *pBuddy = (BuddyApiBuddyRecT *)pBuddyIn;
    HLBBudT *pHLBud;
    if (pBuddy == NULL)
        return NULL;
    if (pBuddy->pHLBud != NULL)
        return pBuddy->pHLBud;
    pHLBud = pApi->pMemAlloc(sizeof(HLBBudT));
    memset(pHLBud, 0, sizeof(HLBBudT));
    pHLBud->pMsgList = _listCreate();
    pHLBud->pApi = pApi;
    pHLBud->pBuddy = pBuddy;
    pBuddy->pHLBud = pHLBud;
    pBuddy->iIndex = ++pApi->iBuddyCount;
    return pHLBud;
}

void _HLBRosterCallback(BuddyApiRefT *pBuddyApi, BuddyApiMsgT *pMsg, HLBApiRefT *pApi)
{
    int iNum = _rosterCount(pApi);
    int i;
    for (i = 0; i < iNum; i++)
    {
        BuddyApiBuddyRecT *pBuddy = DispListIndex(_getRost(pApi), i);
        // Medido (beqzl con lw uFlags en el delay): continue pelao, sin goto
        // ni re-test de NULL (el scheduler rellena con el lw tardio).
        if (pBuddy == NULL)
            continue;
        if (pBuddy->pHLBud == NULL)
            _constructHLBud(pApi, (BuddyApiBuddyT *)pBuddy);
        if ((pBuddy->uFlags & 0x8000) != 0)
        {
            if (pApi->pGameInviteCallback != NULL)
                pApi->pGameInviteCallback(pApi, pBuddy->pHLBud, 1, pApi->pGameInviteRef);
        }
    }
    _signalBuddyListChanged(pApi);
}

BuddyApiBuddyT *_setUpOp(pApi, pName, pCallback, pRef)
HLBApiRefT *pApi;
const char *pName;
void (*pCallback)(HLBApiRefT *, int, int, void *);
void *pRef;
{
    HLBBudT *pBud;
    pApi->pOpCallback = pCallback;
    pApi->pOpRef = pRef;
    pBud = HLBListGetBuddyByName(pApi, pName);
    return pBud != NULL ? pBud->pBuddy : NULL;
}

HLBBudT *_addChangeLLBud(HLBApiRefT *pApi, BuddyApiBuddyRecT *pOld, BuddyApiBuddyRecT *pNew)
{
    BuddyApiBuddyRecT Rec;
    BuddyApiBuddyRecT Rec2;
    BuddyApiBuddyT *pFound;
    int iResult;
    Rec = *pOld;
    Rec2 = *pNew;
    pFound = BuddyApiFind(pApi->pBuddyApi, (BuddyApiBuddyT *)&Rec2);
    if (pFound == NULL)
    {
        // Por los locales (medido: el sw va a Rec2+0xF8, no a *pNew): asi pNew
        // muere tras la copia y no se salva en s3.
        if (Rec.uFlags != Rec2.uFlags && (Rec.uFlags & 0x100000) != 0 && (Rec2.uFlags & 0x204) == 0)
            Rec2.uFlags &= ~0x100000;
    }
    iResult = BuddyApiAdd(pApi->pBuddyApi, (BuddyApiBuddyT *)&Rec2, _HLBOpCallback, pApi, pApi->iRosterTimeout);
    _setLastOpState(pApi, iResult);
    // Devuelve puntero (medido: el llamante lo usa como HLBBudT* para uFlags2 y el
    // Msg'addu'; NULL si Add falla o el buddy ya existia).
    if (iResult != 0)
        return NULL;
    if (pFound != NULL)
        return NULL;
    pFound = BuddyApiFind(pApi->pBuddyApi, (BuddyApiBuddyT *)&Rec2);
    return _constructHLBud(pApi, pFound);
}

int _HLBListRemoveBuddy(HLBApiRefT *pApi, const char *pName, unsigned int uFlags,
                               void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    BuddyApiBuddyRecT Rec;
    int iResult = 0;
    int bCanRemove = 0;
    if (pApi->iState == 5)
        return -0x6E;
    if (HLBApiGetConnectState(pApi) != 2)
        return -4;
    memset(&Rec, 0, sizeof(Rec));
    strncpy(Rec.Name, pName, 0x21);
    Rec.uFlags = uFlags;
    {
        HLBBudT *pBud = HLBListGetBuddyByName(pApi, pName);
        if (pBud != NULL && pBud->pBuddy != NULL)
            bCanRemove = (uFlags & pBud->pBuddy->uFlags) == 0;
    }
    if (bCanRemove == 0)
        return iResult;
    if (pApi->iLastOpState == -1)
        return iResult;
    if (pCallback != NULL)
    {
        pApi->pOpCallback = pCallback;
        pApi->pOpRef = pRef;
    }
    strncpy(Rec.Name, pName, 0x10);
    if (uFlags == 0x100000)
        Rec.uFlags |= 0x800000;
    iResult = BuddyApiDel(pApi->pBuddyApi, (BuddyApiBuddyT *)&Rec, _HLBOpCallback, pApi, pApi->iRosterTimeout);
    _setLastOpState(pApi, iResult);
    if (iResult != 0)
        return iResult;
    if (uFlags == 0x100000)
    {
        BuddyApiMsgRecT Msg;
        Msg.uType = MSGTYPE_DELU;
        Msg.uKind = 0;
        Msg.uCode = 0;
        Msg.uTimeStamp = 0;
        Msg.uFlags = 0;
        Msg.pData = 0;
        _HLBOpCallback(pApi->pBuddyApi, (BuddyApiMsgT *)&Msg, pApi);
    }
    _signalBuddyListChanged(pApi);
    return iResult;
}

int _removeOldestTempBuddy(HLBApiRefT *pApi)
{
    HLBBudT *pOldest = NULL;
    unsigned int uOldestTime = 0xFFFFFFFF;
    int iNum = HLBListGetBuddyCount(pApi);
    int iResult = 0;
    int i;
    for (i = 0; i < iNum; i++)
    {
        HLBBudT *pBud = HLBListGetBuddyByIndex(pApi, i);
        if (pBud == NULL)
            continue;
        if (!HLBBudIsTemporary((HLBBudT *)pBud))
            continue;
        if (HLBBudTempBuddyIs((HLBBudT *)pBud, 0x40))
            continue;
        if (uOldestTime < (unsigned int)pBud->pBuddy->iIndex)
            continue;
        uOldestTime = pBud->pBuddy->iIndex;
        pOldest = pBud;
    }
    if (pOldest != NULL)
        iResult = _HLBListRemoveBuddy(pApi, pOldest->pBuddy->Name, 0x100000, NULL, NULL);
    else
        iResult = -5;
    return iResult;
}

int _canAddBuddy(HLBApiRefT *pApi, unsigned int uFlags)
{
    int iRemoved = 0;
    int iCount = 0;
    if (uFlags == 0x100000)
    {
        int iNum = _rosterCount(pApi);
        int i;
        for (i = 0; i < iNum; i++)
        {
            HLBBudT *pBud = HLBListGetBuddyByIndex(pApi, i);
            if (pBud == NULL || pBud->pBuddy == NULL)
                goto next;
            if ((pBud->pBuddy->uFlags & 0x100000) != 0 && (pBud->pBuddy->uFlags & 0xC0204) == 0
                && (pBud->uFlags2 & 0x40) == 0)
                iCount = i + 1;
next:
            if (iCount < pApi->iMaxTempBuddyCount)
                continue;
            iRemoved = _removeOldestTempBuddy(pApi);
        }
    }
    if (iRemoved == 0)
    {
        if (uFlags == 0x200)
        {
            if (_countBuddies(pApi, 0x200) < pApi->iMaxBuddyCount)
                return 0;
            return 4;
        }
        if (uFlags == 4)
        {
            if (_countBuddies(pApi, 0xC0004) < pApi->iMaxIgnoreCount)
                return 1;
        }
    }
    return 0;
}

int _MsgListGetOldestMsgForBuddy(HLBBudT *pBud, int iUnreadOnly, unsigned int *pTimeStamp)
{
    unsigned int uOldest = 0xFFFFFFFF;
    int iOldest = -1;
    if (pBud == NULL || ((HLBBudT *)pBud)->pMsgList == NULL)
        return -1;
    {
        int iNum = _listGetCount(((HLBBudT *)pBud)->pMsgList);
        int i;
        for (i = 0; i < iNum; i++)
        {
            HLBMsgRecT *pMsg = _listItemGetByIndex(((HLBBudT *)pBud)->pMsgList, i);
            if (pMsg == NULL)
                continue;
            if (iUnreadOnly != 0 && (pMsg->uFlags & 1) != 0)
                continue;
            if (uOldest < pMsg->uTimeStamp)
                continue;
            uOldest = pMsg->uTimeStamp;
            iOldest = i;
        }
    }
    *pTimeStamp = uOldest != 0xFFFFFFFE ? uOldest : *pTimeStamp;
    return iOldest;
}

int _HLBMsgListDeleteOldestMsg(HLBApiRefT *pApi, int iUnreadOnly, int iKind)
{
    unsigned int uOldest = 0xFFFFFFFF;
    HLBBudT *pOldestBud = NULL;
    int iOldestMsg = -1;
    int iNum = HLBListGetBuddyCount(pApi);
    int i;
    for (i = 0; i < iNum; i++)
    {
        HLBBudT *pBud = HLBListGetBuddyByIndex(pApi, i);
        unsigned int uTimeStamp = 0;
        int iMsg;
        if (iUnreadOnly != 0 && !HLBBudIsTemporary(pBud))
            continue;
        iMsg = _MsgListGetOldestMsgForBuddy(pBud, iKind, &uTimeStamp);
        if (iMsg < 0)
            continue;
        if (uOldest < uTimeStamp)
            continue;
        uOldest = uTimeStamp;
        pOldestBud = pBud;
        iOldestMsg = iMsg;
    }
    if (pOldestBud != NULL)
    {
        HLBMsgListDelete(pOldestBud, iOldestMsg);
        return 0;
    }
    return -1;
}

int _HLBMsgListDeleteOldestMsgForBud(HLBApiRefT *pApi, HLBBudT *pBud)
{
    unsigned int uTimeStamp = 0;
    int iMsg = _MsgListGetOldestMsgForBuddy(pBud, 1, &uTimeStamp);
    if (iMsg < 0)
    {
        iMsg = _MsgListGetOldestMsgForBuddy(pBud, 0, &uTimeStamp);
        if (iMsg < 0)
            return -1;
    }
    HLBMsgListDelete(pBud, iMsg);
    return 0;
}

int _HLBMsgListJettisonOldMsg(HLBApiRefT *pApi)
{
    if (_HLBMsgListDeleteOldestMsg(pApi, 1, 1) == 0)
        return 0;
    if (_HLBMsgListDeleteOldestMsg(pApi, 1, 0) == 0)
        return 0;
    if (_HLBMsgListDeleteOldestMsg(pApi, 0, 1) == 0)
        return 0;
    return _HLBMsgListDeleteOldestMsg(pApi, 0, 0);
}

void _HLBMsgListAddMsg(HLBApiRefT *pApi, const char *pName, BuddyApiMsgRecT *pMsg, int iNew)
{
    HLBBudT *pBud;
    int bCanAdd = 1;
    HLBMsgRecT *pNewMsg;
    int iLen;
    HLBListFlagTempBuddy(pApi, pName, 1);
    pBud = HLBListGetBuddyByName(pApi, pName);
    if (pBud == NULL || pBud->pMsgList == NULL)
        return;
    if (!HLBBudIsTemporary((HLBBudT *)pBud))
    {
        // Medido (bnez s5->skip): borra si iNew == 0, al reves de lo heredado.
        if (HLBBudIsWannaBeMyBuddy((HLBBudT *)pBud) && iNew == 0)
            HLBMsgListDelete((HLBBudT *)pBud, 0);
    }
    if (HLBMsgListGetTotalCount((HLBBudT *)pBud) < pApi->iMaxMsgCount)
        goto addMsg;
    if (_HLBMsgListDeleteOldestMsgForBud(pApi, pBud) < 0)
        return;
    if (pApi->iTotalMsgs < pApi->iMaxTotalMsgCount)
        goto addMsg;
    // Medido (sltiu s1,v0,1): asignacion desde la comparacion, no if+movz;
    // Jettison==0 es exito (libero hueco) -> se puede anadir.
    bCanAdd = (_HLBMsgListJettisonOldMsg(pApi) == 0);
    if (bCanAdd == 0)
        return;
addMsg:
    pApi->iTotalMsgs += 1;
    iLen = strlen(pMsg->pData);
    pNewMsg = pApi->pMemAlloc(sizeof(HLBMsgRecT));
    *pNewMsg = *(HLBMsgRecT *)pMsg;
    pNewMsg->pText = pApi->pMemAlloc(strlen(pMsg->pData));
    memcpy(pNewMsg->pText, pMsg->pData, iLen);
    _listItemAdd(pBud->pMsgList, pNewMsg);
    if (iNew == 1)
    {
        pBud->uNewMsg = 1;
        pBud->pBuddy->uFlags |= 0x2000000;
        _signalBuddyListChanged(pApi);
    }
    if (pApi->pNewMsgCallback != NULL && pMsg != NULL)
        pApi->pNewMsgCallback(pApi, pMsg, pApi->pNewMsgRef);
}

int _HLBListAddChangeBuddy(HLBApiRefT *pApi, const char *pName, unsigned int uFlags,
                                  unsigned int uTempFlags, void (*pCallback)(HLBApiRefT *, int, int, void *),
                                  void *pRef)
{
    BuddyApiBuddyRecT Rec;
    BuddyApiBuddyRecT Rec2;
    BuddyApiMsgRecT Msg;
    HLBBudT *pBud;
    memset(&Rec, 0, sizeof(Rec));
    if (pApi->iState == 5)
        return -0x6E;
    if (HLBApiGetConnectState(pApi) != 2)
    {
        if (uFlags != 0x100000)
            return -4;
        _preConnTempBudAdd(pApi, pName, uTempFlags);
    }
    pBud = HLBListGetBuddyByName(pApi, pName);
    if (pBud != NULL)
    {
        if (uFlags == 0x100000)
        {
            // Medido (lhu a1 + or/sh, y el and en el delay usa a1): el chequeo
            // de abajo ve el valor VIEJO, asi que va a un temporal con nombre.
            unsigned short uOldFlags2 = pBud->uFlags2;
            pBud->uFlags2 |= uTempFlags;
            if ((pBud->pBuddy->uFlags & uFlags) == 0)
            {
                // Medido (copia a Rec2+Add con NULL/NULL+signal+return, sin
                // _setLastOpState): alta directa aqui, no goto doAdd.
                Rec2 = *pBud->pBuddy;
                Rec2.uFlags = 0x900000;
                BuddyApiAdd(pApi->pBuddyApi, (BuddyApiBuddyT *)&Rec2, NULL, NULL,
                            pApi->iRosterTimeout);
                _signalBuddyListChanged(pApi);
                return 0;
            }
            if ((uOldFlags2 & uTempFlags) != 0)
                return 0;
            _signalBuddyListChanged(pApi);
            return 0;
        }
        else
        {
            // Medido (and v0,s2,v0): uFlags a la izquierda.
            if ((uFlags & pBud->pBuddy->uFlags) != 0)
                return 0;
        }
    }
    if (uFlags == 0x200)
    {
        if (!_canAddBuddy(pApi, 0x200))
            return -5;
    }
    else if (pBud == NULL && uTempFlags != 0x40)
    {
        if (!_canAddBuddy(pApi, uFlags))
            return -5;
    }
    if (pApi->iLastOpState != -1)
    {
        // Medido (bne a 6608/stores; sin chequeo de uFlags ni -1 en esta rama):
        // ocupado solo guarda el callback.
        if (pCallback != NULL)
        {
            pApi->pOpCallback = pCallback;
            pApi->pOpRef = pRef;
        }
    }
    // Medido (bne+beqz encadenados, v0=-1 vivo hasta el return): en reposo,
    // uFlags distinto de 0x100000 o callback no nulo es -1.
    else if (uFlags != 0x100000 || pCallback != NULL)
        return -1;
doAdd:
    if (pBud != NULL)
        Rec = *pBud->pBuddy;
    // Medido (bucle ldl/ldr de v1=sp a a3=s3 con limite a1=sp+0x100): Rec2 = Rec.
    Rec2 = Rec;
    strncpy(Rec2.Name, pName, 0x21);
    // Medido (store incondicional + bne con el primer store en el delay, sin
    // movn/bnel): no es if/else sino store + overwrite condicional.
    Rec2.uFlags = uFlags;
    if (uFlags == 0x100000)
        Rec2.uFlags = 0x900000;
    // Medido en el original (a1=sp, a2=s3=sp+0x120): Rec (a cero) como pOld y
    // Rec2 como pNew; devuelve el HLBBudT* y el llamante lo usa directo, sin
    // segunda busqueda por nombre.
    pBud = _addChangeLLBud(pApi, &Rec, &Rec2);
    if (pBud == NULL)
        return 0;
    if (uFlags == 0x100000)
    {
        // Medido: sin memset; solo uType, uCode y pData (uKind/uTimeStamp/
        // uFlags quedan sin inicializar en el original). El signal va dentro:
        // con otros uFlags se vuelve sin avisar (bne a epilogo).
        pBud->uFlags2 |= uTempFlags;
        Msg.uType = MSGTYPE_ADDU;
        Msg.uCode = 0;
        Msg.pData = NULL;
        _HLBOpCallback(pApi->pBuddyApi, (BuddyApiMsgT *)&Msg, pApi);
        _signalBuddyListChanged(pApi);
    }
    return 0;
}

int _HLBRosterSort(BuddyApiBuddyRecT *pBud1, BuddyApiBuddyRecT *pBud2)
{
    int iState1 = HLBBudGetState((HLBBudT *)pBud1->pHLBud);
    int iState2 = HLBBudGetState((HLBBudT *)pBud2->pHLBud);
    if (iState1 != iState2)
    {
        if (iState1 == 1 || iState2 == 1)
            return (iState1 ^ 1) != 0 ? 1 : -1;
        return iState2 < iState1 ? 1 : -1;
    }
    return ds_stricmp(pBud1->Name, pBud2->Name);
}

void _HLBMsgCallback(BuddyApiRefT *pBuddyApi, BuddyApiMsgT *pMsgIn, HLBApiRefT *pApi)
{
    BuddyApiMsgRecT *pMsg = (BuddyApiMsgRecT *)pMsgIn;
    char Buffer[0x10];
    int iFlags = 0;
    TagFieldGetString(TagFieldFind(pMsg->pData, "USER"), Buffer, 0x10, "unknown");
    if (pMsg->uType != MSGTYPE_CHAT)
        return;
    iFlags = TagFieldGetNumber(TagFieldFind(pMsg->pData, "NOREPLY"), 0);
    pMsg->uKind = MSGTYPE_CHAT;
    if (strstr(pMsg->pData, "Congratulations, you are a recruit for a club") != NULL)
        pMsg->uKind = MSGTYPE_CLUB;
    else if (iFlags != 0)
        pMsg->uKind = MSGTYPE_ADMN;
    // Medido (daddu a3,s2): el 4.º arg es iFlags (el NOREPLY), no el literal 1.
    _HLBMsgListAddMsg(pApi, Buffer, pMsg, iFlags);
}

void _HLBBuddyDelMemCallback(BuddyApiRefT *pBuddyApi, BuddyApiBuddyT *pBuddyIn, HLBApiRefT *pApi)
{
    BuddyApiBuddyRecT *pBuddy = (BuddyApiBuddyRecT *)pBuddyIn;
    HLBBudT *pHLBud = pBuddy != NULL ? pBuddy->pHLBud : NULL;
    if (pHLBud != NULL)
        _HLBDestroyBuddy((HLBBudT *)pHLBud);
}

void _HLBBuddyChangeCallback(BuddyApiRefT *pBuddyApi, BuddyApiBuddyT *pBuddyIn, unsigned int uChange,
                                    void *pRef, HLBApiRefT *pApi)
{
    BuddyApiBuddyRecT *pBuddy = (BuddyApiBuddyRecT *)pBuddyIn;
    HLBBudT *pHLBud = pBuddy->pHLBud;
    if (pHLBud == NULL)
        return;
    if (uChange == MSGTYPE_GETP)
    {
        if (pApi->pStatusCallback != NULL)
            pApi->pStatusCallback(pApi, (HLBBudT *)pHLBud, uChange, pApi->pStatusRef);
        return;
    }
    if (uChange != MSGTYPE_GINV)
        return;
    if (uChange == 0x4000000)
    {
        pHLBud->pBuddy->uFlags |= uChange;
        uChange = 8;
    }
    if (uChange == 8)
    {
        if (HLBBudIsSameProduct((HLBBudT *)pHLBud))
        {
            pHLBud->pBuddy->uFlags |= 0x8000;
            goto checkFlags;
        }
        uChange = 0x8000;
    }
    if (pApi->iGameInviteTimeout2 > 0)
        pApi->uGameInviteTime = pApi->uCurTime + pApi->iGameInviteTimeout2 * 1000;
    strncpy(pApi->GameInviteName, HLBBudGetName((HLBBudT *)pHLBud), 0x20);
checkFlags:
    if (uChange == 0x8000 || uChange == 0x800 || uChange == 0x400)
    {
        if (uChange == 0x400)
        {
            BuddyApiRespondGame(pApi->pBuddyApi, (BuddyApiBuddyT *)pHLBud->pBuddy, 2, NULL, NULL, NULL,
                                pApi->iGameInviteTimeout);
            return;
        }
        pApi->uGameInviteTime = 0;
    }
    if (uChange == 0x400000 && HLBBudIsBlocked((HLBBudT *)pHLBud))
    {
        BuddyApiRespondGame(pApi->pBuddyApi, (BuddyApiBuddyT *)pHLBud->pBuddy, 1, NULL, NULL, NULL,
                            pApi->iGameInviteTimeout);
        return;
    }
    if (pApi->pGameInviteCallback != NULL)
    {
        pApi->pGameInviteCallback(pApi, (HLBBudT *)pHLBud, uChange, pApi->pGameInviteRef);
        if (uChange == 0x4000000)
            HLBListAnswerGameInvite(pApi, HLBBudGetName((HLBBudT *)pHLBud), 2, NULL, NULL);
    }
    _signalBuddyListChanged(pApi);
}

HLBApiRefT *_HLBApiInit(HLBApiRefT *pApi, const char *pProduct, const char *pVersion)
{
    pApi->pBuddyApi2 = BuddyApiCreate2(pApi->pMemAlloc, pApi->pMemFree, pProduct, pVersion,
                                       (BuddyApiSortFnT)_HLBRosterSort);
    pApi->pBuddyApi = pApi->pBuddyApi2;
    if (pApi->pBuddyApi == NULL)
    {
        pApi->iLastOpState = -0x6D;
        pApi->pMemFree(pApi);
        return NULL;
    }
    pApi->pPreConnTempBudList = _listCreate();
    BuddyApiRecv(pApi->pBuddyApi, _HLBMsgCallback, pApi);
    BuddyApiRegisterBuddyChangeCallback(pApi->pBuddyApi, _HLBBuddyChangeCallback, pApi);
    BuddyApiRegisterBuddyDelCallback(pApi->pBuddyApi, _HLBBuddyDelMemCallback, pApi);
    HLBApiOverrideConstants(pApi, 0, 60, 90, 100, 10, 100, -1, 200, 0, 600);
    pApi->iMaxMsgCount = 100;
    pApi->iMaxBuddyCount = 50;
    BuddyApiConfig(pApi->pBuddyApi, "unknown", -1, "en", 1, 1);
    BuddyApiPresInit(pApi->pBuddyApi);
    return pApi;
}

HLBApiRefT *HLBApiCreate2(void *(*pMemAlloc)(int), void (*pMemFree)(void *), const char *pProduct,
                          const char *pVersion)
{
    HLBApiRefT *pApi;
    if (pMemAlloc == NULL || pMemFree == NULL)
        return NULL;
    pApi = NetAlloc(sizeof(HLBApiRefT));
    if (pApi != NULL)
    {
        memset(pApi, 0, sizeof(HLBApiRefT));
        pApi->pMemAlloc = pMemAlloc;
        pApi->pMemFree = pMemFree;
    }
    return _HLBApiInit(pApi, pProduct, pVersion);
}

void HLBApiSetDebugFunction(HLBApiRefT *pApi, void *pDebugRef, void (*pDebugFn)(void *, const char *))
{
    if (pDebugFn == NULL)
        return;
    if (pApi == NULL)
        return;
    // Medido (sw a2,0x44; BuddyApiDebug recibe a1 intacto): el campo
    // guarda la funcion, no el ref (quirk original de EA).
    pApi->pDebugRef = (void *)pDebugFn;
    BuddyApiDebug(pApi->pBuddyApi, pDebugRef);
}

void HLBApiOverrideConstants(HLBApiRefT *pApi, int iNoText, int iSuspendPort, int iRosterTimeout,
                             int iMaxIgnoreCount, int iMaxTempBuddyCount, int iMaxTotalMsgCount,
                             int iGameInviteTimeout, int iMaxPreConnBuddyCount, int iPad10, int iGameInviteTimeout2)
{
    if (iSuspendPort != -1)
        pApi->iSuspendPort = iSuspendPort;
    if (iRosterTimeout != -1)
        pApi->iRosterTimeout = iRosterTimeout;
    if (iMaxIgnoreCount != -1)
        pApi->iMaxIgnoreCount = iMaxIgnoreCount;
    if (iMaxTempBuddyCount != -1)
        pApi->iMaxTempBuddyCount = iMaxTempBuddyCount;
    if (iMaxTotalMsgCount != -1)
        pApi->iMaxTotalMsgCount = iMaxTotalMsgCount;
    if (iMaxPreConnBuddyCount != -1 && iMaxPreConnBuddyCount < 201)
        pApi->iMaxPreConnBuddyCount = iMaxPreConnBuddyCount;
    if (iNoText == 1)
        BuddyApiPresNoText(pApi->pBuddyApi);
    if (iPad10 == 1)
        pApi->bPresNoText = iPad10;
    if (iGameInviteTimeout2 != -1)
        pApi->iGameInviteTimeout2 = iGameInviteTimeout2;
}

int HLBApiInitialize(HLBApiRefT *pApi, const char *pServerPort, int iProduct, const char *pLanguage)
{
    if (pApi == NULL)
        return -8;
    return BuddyApiConfig(pApi->pBuddyApi, pServerPort, iProduct, pLanguage, 1, 1);
}

int HLBApiConnect(HLBApiRefT *pApi, const char *pServer, int iPort, const char *pPassword, void *pContext)
{
    if (pApi == NULL || pApi->pBuddyApi == NULL)
        return -3;
    _HLBRosterDestroy(pApi);
    pApi->iState = 0;
    pApi->iTotalMsgs = 0;
    pApi->iConnectState = 1;
    return BuddyApiConnect(pApi->pBuddyApi, pServer, iPort, pApi->iSuspendPort, pPassword, pContext,
                           _HLBConnectCallback, pApi);
}

int HLBApiRegisterConnectCallback(HLBApiRefT *pApi,
                                  void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    if (pApi == NULL)
        return -3;
    pApi->pConnectCallback = pCallback;
    pApi->pConnectRef = pRef;
    return 0;
}

int HLBApiGetConnectState(HLBApiRefT *pApi)
{
    return pApi == NULL ? 0 : pApi->iConnectState;
}

void HLBApiDisconnect(HLBApiRefT *pApi)
{
    if (pApi == NULL)
        return;
    _HLBRosterDestroy(pApi);
    if (pApi->pBuddyApi != NULL)
    {
        BuddyApiDisconnect(pApi->pBuddyApi);
        // Medido (sw 0x0C antes que sw 0x38 en el original; el scheduler los
        // invierte: van en orden inverso en fuente).
        pApi->iConnectState = 0;
        pApi->iState = 0;
    }
}

void HLBApiDestroy(HLBApiRefT *pApi)
{
    if (pApi == NULL)
        return;
    if (pApi->pBuddyApi != NULL)
    {
        _HLBRosterDestroy(pApi);
        if (pApi->pBuddyApi != NULL)
            BuddyApiDestroy(pApi->pBuddyApi);
    }
    _preConnTempBudListDestroy(pApi);
    NetFree(pApi);
}

void HLBApiUpdate(HLBApiRefT *pApi)
{
    unsigned int uCurTime;
    if (pApi == NULL)
        return;
    uCurTime = BuddyApiUpdate(pApi->pBuddyApi);
    if (pApi->uGameInviteTime != 0)
    {
        pApi->uCurTime = uCurTime;
        if (pApi->uGameInviteTime >= uCurTime)
            goto checkOp;
        {
            HLBBudT *pBud = HLBListGetBuddyByName(pApi, pApi->GameInviteName);
            pApi->GameInviteName[0] = '\0';
            if (pBud != NULL)
            {
                pApi->uGameInviteTime = 0;
                _HLBBuddyChangeCallback(pApi->pBuddyApi, (BuddyApiBuddyT *)pBud->pBuddy, MSGTYPE_GINV,
                                        0x4000000, pApi);
                _signalBuddyListChanged(pApi);
            }
        }
    }
checkOp:
    if (pApi->bOpPending != 0)
    {
        if (pApi->pOpCallback != NULL)
        {
            pApi->iLastOpState = 0;
            pApi->pOpCallback(pApi, 0xA, pApi->iOpError, pApi->pOpRef);
            pApi->pOpCallback = NULL;
        }
        pApi->bOpPending = 0;
    }
}

int HLBApiSuspend(HLBApiRefT *pApi)
{
    pApi->iStatus = 5;
    pApi->uGameInviteTime = 0;
    if (pApi->bSuppressPresence == 0)
        BuddyApiPresSend(pApi->pBuddyApi, "", 5, NULL, 0);
    BuddyApiSuspendXDK(pApi->pBuddyApi);
    _HLBRosterDestroy(pApi);
    _preConnTempBudListDestroy(pApi);
    pApi->iState = 5;
    // Medido (daddu v0,zero,zero final): devuelve int (0), no void.
    return 0;
}

void HLBApiResume(HLBApiRefT *pApi)
{
    pApi->iLastOpState = 0;
    pApi->iStatus = 1;
    BuddyApiRoster(pApi->pBuddyApi, _HLBRosterCallback, pApi);
    if (pApi->bSuppressPresence == 0)
        BuddyApiPresSend(pApi->pBuddyApi, NULL, pApi->iStatus, NULL, 1);
    BuddyApiResumeXDK(pApi->pBuddyApi);
    pApi->iState = 1;
    _getRost(pApi);
}

void HLBApiCancelOp(HLBApiRefT *pApi)
{
    if (pApi->pOpCallback != NULL)
    {
        pApi->pOpCallback(pApi, 0xA, -0x6C, pApi->pOpRef);
        pApi->pOpCallback = NULL;
    }
    pApi->iLastOpState = -0x6C;
}

int HLBApiFindUsers(HLBApiRefT *pApi, const char *pSearch)
{
    int iResult;
    _setUpOp(pApi, pSearch);
    iResult = BuddyApiFindUsers(pApi->pBuddyApi, pSearch, _HLBOpCallback, pApi, pApi->iRosterTimeout);
    _setLastOpState(pApi, iResult);
    return iResult;
}

int HLBApiUserFound(HLBApiRefT *pApi, int iIndex)
{
    return BuddyApiUserFound(pApi->pBuddyApi, iIndex);
}

int HLBApiRegisterNewMsgCallback(HLBApiRefT *pApi, void (*pCallback)(HLBApiRefT *, void *, void *), void *pRef)
{
    if (pApi->pNewMsgCallback != NULL && pCallback != NULL)
        return -5;
    // Medido (sw Ref antes que Callback; el scheduler los invierte): orden
    // inverso en fuente. Igual en los otros tres Register*.
    pApi->pNewMsgCallback = pCallback;
    pApi->pNewMsgRef = pRef;
    return 0;
}

int HLBApiRegisterBuddyChangeCallback(HLBApiRefT *pApi,
                                      void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    if (pApi->pBuddyChangeCallback != NULL && pCallback != NULL)
        return -5;
    pApi->pBuddyChangeCallback = pCallback;
    pApi->pBuddyChangeRef = pRef;
    return 0;
}

int HLBApiRegisterGameInviteCallback(HLBApiRefT *pApi,
                                     void (*pCallback)(HLBApiRefT *, HLBBudT *, int, void *), void *pRef)
{
    if (pApi->pGameInviteCallback != NULL && pCallback != NULL)
        return -5;
    pApi->pGameInviteCallback = pCallback;
    pApi->pGameInviteRef = pRef;
    return 0;
}

const char *HLBBudGetName(HLBBudT *pBud)
{
    return ((HLBBudT *)pBud)->pBuddy->Name;
}

int HLBBudIsTemporary(HLBBudT *pBud)
{
    int bTemp = 0;
    int bOther = 0;
    if (HLBBudIsIWannaBeHisBuddy(pBud))
        bTemp = 1;
    else if (HLBBudIsWannaBeMyBuddy(pBud))
        bTemp = 1;
    if (HLBBudIsRealBuddy(pBud))
        bOther = 1;
    else if (HLBBudIsBlocked(pBud))
        bOther = 1;
    return (bTemp | bOther) != 0 ? 0 : 1;
}

int HLBBudIsBlocked(HLBBudT *pBud)
{
    return (((HLBBudT *)pBud)->pBuddy->uFlags >> 9) & 1;
}

int HLBBudIsWannaBeMyBuddy(HLBBudT *pBud)
{
    return (((HLBBudT *)pBud)->pBuddy->uFlags >> 18) & 1;
}

int HLBBudIsIWannaBeHisBuddy(HLBBudT *pBud)
{
    return (((HLBBudT *)pBud)->pBuddy->uFlags >> 19) & 1;
}

const char *HLBBudGetTitle(HLBBudT *pBud)
{
    BuddyApiRefreshTitle(((HLBBudT *)pBud)->pApi->pBuddyApi,
                         (BuddyApiBuddyT *)((HLBBudT *)pBud)->pBuddy);
    return ((HLBBudT *)pBud)->pBuddy->Title;
}

int HLBBudTempBuddyIs(HLBBudT *pBud, int iFlags)
{
    return pBud == NULL ? 0 : (((HLBBudT *)pBud)->uFlags2 & iFlags) != 0;
}

int HLBBudIsRealBuddy(HLBBudT *pBud)
{
    if (pBud == NULL)
        return 0;
    return (((HLBBudT *)pBud)->pBuddy->uFlags >> 2) & 1;
}

int HLBBudIsInGroup(HLBBudT *pBud)
{
    return ((HLBBudT *)pBud)->uGroup != 1;
}

int HLBBudGetState(HLBBudT *pBud)
{
    return pBud == NULL ? 0 : ((HLBBudT *)pBud)->pBuddy->eState;
}

int HLBBudIsPassive(HLBBudT *pBud)
{
    return ((HLBBudT *)pBud)->pBuddy->eState == 5;
}

int HLBBudIsAvailableForChat(HLBBudT *pBud)
{
    return ((HLBBudT *)pBud)->pBuddy->eState == 1;
}

int HLBBudIsSameProduct(HLBBudT *pBud)
{
    int bSame = 0;
    // Medido (beqz v0->jr con el SET caido): la condicion es el int
    // enmascarado, sin == 0 (el bit a 1 da 1, al reves de lo heredado).
    if (pBud == NULL || (((HLBBudT *)pBud)->pBuddy->uFlags & 0x2000))
        bSame = 1;
    return bSame;
}

int HLBBudGetPresence(HLBBudT *pBud, char *pBuffer)
{
    HLBBudT *pRec = (HLBBudT *)pBud;
    if (pBud == NULL)
        return 0;
    if (pRec->pApi != NULL && pRec->pApi->pUtf8Tbl != NULL)
        Utf8TranslateTo8Bit(pBuffer, 100, pRec->pBuddy->Presence, 0x2A);
    else
        strncpy(pBuffer, pRec->pBuddy->Presence, 100);
    return 0;
}

int HLBBudGetPresenceExtra(HLBBudT *pBud, char *pBuffer, int iBufferSize)
{
    return TagFieldDupl(((HLBBudT *)pBud)->pBuddy->PresenceExtra, pBuffer, iBufferSize) != 0;
}

int HLBBudIsNoReplyBud(HLBBudT *pBud)
{
    return (((HLBBudT *)pBud)->pBuddy->uFlags >> 25) & 1;
}

int HLBBudIsJoinable(HLBBudT *pBud)
{
    return (((HLBBudT *)pBud)->pBuddy->uFlags >> 12) & 1;
}

int HLBBudGetVOIPState(HLBBudT *pBud)
{
    return ((HLBBudT *)pBud)->pBuddy->iVoipState;
}

int HLBBudCanVoiceChat(HLBBudT *pBud)
{
    int iVoipState = HLBBudGetVOIPState(pBud);
    int bCanChat = 0;
    if (iVoipState != 1)
        return 0;
    if (HLBBudIsSameProduct(pBud) != 0)
        bCanChat = iVoipState;
    return bCanChat;
}

void *HLBMsgListGetFirstUnreadMsg(HLBBudT *pBud, int iIndex, int iMarkRead)
{
    int iNum = _listGetCount(((HLBBudT *)pBud)->pMsgList);
    int i;
    for (i = iIndex; i < iNum; i++)
    {
        HLBMsgRecT *pMsg = _listItemGetByIndex(((HLBBudT *)pBud)->pMsgList, i);
        if (pMsg == NULL)
            continue;
        if ((pMsg->uFlags & 1) != 0)
            continue;
        if (iMarkRead != 0)
            pMsg->uFlags |= 1;
        return pMsg;
    }
    return NULL;
}

int HLBMsgListGetUnreadCount(HLBBudT *pBud)
{
    int iCount = 0;
    int iNum = _listGetCount(((HLBBudT *)pBud)->pMsgList);
    int i;
    for (i = 0; i < iNum; i++)
    {
        HLBMsgRecT *pMsg = _listItemGetByIndex(((HLBBudT *)pBud)->pMsgList, i);
        if (pMsg == NULL)
            continue;
        if ((pMsg->uFlags & 1) == 0)
            iCount = i + 1;
    }
    return iCount;
}

int HLBMsgListGetTotalCount(HLBBudT *pBud)
{
    return _listGetCount(((HLBBudT *)pBud)->pMsgList);
}

void HLBMsgListDelete(HLBBudT *pBud, int iIndex)
{
    if (_listItemGetByIndex(((HLBBudT *)pBud)->pMsgList, iIndex) != NULL
        && ((HLBBudT *)pBud)->pApi != NULL)
        ((HLBBudT *)pBud)->pApi->iTotalMsgs -= 1;
    // Medido (sin daddu v0 final): es void, no int (los llamantes lo usan
    // como sentencia).
    _listMsgItemDestroy(((HLBBudT *)pBud)->pApi, ((HLBBudT *)pBud)->pMsgList, iIndex);
}

void HLBMsgListDeleteAll(HLBBudT *pBud, int iUnreadOnly)
{
    if (pBud == NULL || ((HLBBudT *)pBud)->pMsgList == NULL)
        return;
    {
        int i;
        for (i = _listGetCount(((HLBBudT *)pBud)->pMsgList) - 1; i >= 0; i--)
        {
            HLBMsgRecT *pMsg = _listItemGetByIndex(((HLBBudT *)pBud)->pMsgList, i);
            if (pMsg == NULL)
                continue;
            // Medido (beqzl s2->destroy; beqzl flag->latch): destruye si
            // iUnreadOnly == 0 o si el bit 0 esta a 1 (al reves del nombre;
            // el unico llamante pasa 0). Sin && combinado: ramas separadas.
            if (iUnreadOnly == 0)
                goto destroy;
            if ((pMsg->uFlags & 1) == 0)
                continue;
destroy:
            _listMsgItemDestroy(((HLBBudT *)pBud)->pApi, ((HLBBudT *)pBud)->pMsgList, i);
            ((HLBBudT *)pBud)->pApi->iTotalMsgs -= 1;
        }
    }
}

void _HLBMsgListDestroy(HLBBudT *pBud)
{
    HLBMsgListDeleteAll(pBud, 0);
    _listDestroy(((HLBBudT *)pBud)->pMsgList);
    ((HLBBudT *)pBud)->pMsgList = NULL;
}

HLBBudT *HLBListGetBuddyByName(HLBApiRefT *pApi, const char *pName)
{
    HLBBudT *pBud = NULL;
    if (_getRost(pApi) == NULL)
        return NULL;
    {
        BuddyApiBuddyRecT *pBuddy = (BuddyApiBuddyRecT *)BuddyApiFind(pApi->pBuddyApi, (BuddyApiBuddyT *)pName);
        if (pBuddy != NULL)
            pBud = pBuddy->pHLBud;
    }
    return (HLBBudT *)pBud;
}

HLBBudT *HLBListGetBuddyByIndex(HLBApiRefT *pApi, int iIndex)
{
    HLBBudT *pBud = NULL;
    if (_getRost(pApi) == NULL)
        return pBud;
    {
        BuddyApiBuddyRecT *pBuddy = DispListIndex(_getRost(pApi), iIndex);
        if (pBuddy != NULL)
            pBud = pBuddy->pHLBud;
    }
    return (HLBBudT *)pBud;
}

int HLBListGetBuddyCount(HLBApiRefT *pApi)
{
    return _countBuddies(pApi, 0xFFFFFFFF);
}

int HLBListBlockBuddy(HLBApiRefT *pApi, const char *pName,
                      void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    HLBBudT *pBud = HLBListGetBuddyByName(pApi, pName);
    if (pBud != NULL && HLBBudIsNoReplyBud(pBud) != 0)
        return -2;
    return _HLBListAddChangeBuddy(pApi, pName, 0x200, 0, pCallback, pRef);
}

void HLBListUnBlockBuddy(HLBApiRefT *pApi, const char *pName,
                         void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    _HLBListRemoveBuddy(pApi, pName, 0x200, pCallback, pRef);
}

int HLBListUnMakeBuddy(HLBApiRefT *pApi, const char *pName,
                       void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    int iResult;
    if (pApi->iState == 5)
        return -0x6E;
    HLBListDeleteTempBuddy(pApi, pName);
    iResult = _HLBListRemoveBuddy(pApi, pName, 4, pCallback, pRef);
    return iResult;
}

int HLBListInviteBuddy(HLBApiRefT *pApi, const char *pName,
                       void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    BuddyApiBuddyRecT Rec;
    int iResult;
    if (pApi->iState == 5)
        return -0x6E;
    {
        HLBBudT *pBud = HLBListGetBuddyByName(pApi, pName);
        if (pBud == NULL)
        {
            if (!_canAddBuddy(pApi, 4))
                return -5;
        }
        else if (HLBBudIsNoReplyBud(pBud) != 0)
            return -2;
        else if (HLBBudIsTemporary(pBud) != 0 || HLBBudIsBlocked(pBud) != 0)
        {
            if (!_canAddBuddy(pApi, 4))
                return -5;
        }
    }
    memset(&Rec, 0, sizeof(Rec));
    _setUpOp(pApi, pName, pCallback, pRef);
    strncpy(Rec.Name, pName, 0x21);
    iResult = BuddyApiBuddyInvite(pApi->pBuddyApi, (BuddyApiBuddyT *)&Rec, _HLBOpCallback, pApi,
                                  pApi->iRosterTimeout);
    _setLastOpState(pApi, iResult);
    return iResult;
}

int HLBListAnswerInvite(HLBApiRefT *pApi, const char *pName, int iAnswer,
                        void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    BuddyApiBuddyT *pBuddy;
    int iResult;
    if (iAnswer == 3 && _countBuddies(pApi, 0x200) >= pApi->iMaxBuddyCount)
        return -5;
    pBuddy = _setUpOp(pApi, pName, pCallback, pRef);
    if (pBuddy == NULL)
        return -8;
    // Medido (lw tmp,0xB0 para el 6.º arg en pila): RespondBuddy lleva
    // iTimeout como RespondGame (buddyapi.c aun no lo define; misma aridad).
    iResult = BuddyApiRespondBuddy(pApi->pBuddyApi, pBuddy, iAnswer, _HLBOpCallback, pApi,
                                   pApi->iRosterTimeout);
    _setLastOpState(pApi, iResult);
    return iResult;
}

int HLBListAnswerGameInvite(HLBApiRefT *pApi, const char *pName, int iAnswer,
                            void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    BuddyApiBuddyT *pBuddy;
    int iResult;
    if (iAnswer == 2)
    {
        pApi->uGameInviteTime = 0;
        pApi->GameInviteName[0] = '\0';
    }
    pBuddy = _setUpOp(pApi, pName, pCallback, pRef);
    if (pBuddy == NULL)
        return -8;
    // Medido EABI (t0=pApi, t1=[0xB0], t2=[0x8C], sin pila): el 6.º es
    // iRosterTimeout (como RespondBuddy) y el 7.º iGameInviteTimeout.
    iResult = BuddyApiRespondGame(pApi->pBuddyApi, pBuddy, iAnswer, _HLBOpCallback, pApi,
                                  (void *)pApi->iRosterTimeout, pApi->iGameInviteTimeout);
    _setLastOpState(pApi, iResult);
    _signalBuddyListChanged(pApi);
    return iResult;
}

int HLBListCancelGameInvite(HLBApiRefT *pApi, void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    int iResult = 0;
    int iNum = HLBListGetBuddyCount(pApi);
    int i;
    for (i = 0; i < iNum; i++)
    {
        HLBBudT *pBud = HLBListGetBuddyByIndex(pApi, i);
        if (pBud == NULL || pBud->pBuddy == NULL || (pBud->pBuddy->uFlags & 0x20000) == 0)
            continue;
        iResult = HLBListAnswerGameInvite(pApi, pBud->pBuddy->Name, 2, pCallback, pRef);
    }
    return iResult;
}

int HLBListFlagTempBuddy(HLBApiRefT *pApi, const char *pName, int iFlags)
{
    if (pApi->iState == 5)
        return -0x6E;
    return _HLBListAddChangeBuddy(pApi, pName, 0x100000, iFlags, NULL, NULL);
}

int HLBListUnFlagTempBuddy(HLBApiRefT *pApi, const char *pName, int iFlags)
{
    if (pApi->iState == 5)
        return -0x6E;
    {
        HLBBudT *pBud = HLBListGetBuddyByName(pApi, pName);
        if (pBud != NULL && (pBud->uFlags2 & iFlags) != 0)
        {
            pBud->uFlags2 &= ~iFlags;
            _signalBuddyListChanged(pApi);
        }
    }
    return 0;
}

int HLBListDeleteTempBuddy(HLBApiRefT *pApi, const char *pName)
{
    if (pApi->iState == 5)
        return -0x6E;
    return _HLBListRemoveBuddy(pApi, pName, 0x100000, NULL, NULL);
}

int HLBListSort(HLBApiRefT *pApi)
{
    if (_getRost(pApi) == NULL)
        return 0;
    DispListSort(_getRost(pApi), pApi, (int (*)(void *, void *))_HLBRosterSort, 0);
    DispListChange(_getRost(pApi), 1);
    DispListOrder(_getRost(pApi));
    return 0;
}

int HLBListAddToGroup(HLBApiRefT *pApi, const char *pName, int iGroup)
{
    HLBBudT *pBud = HLBListGetBuddyByName(pApi, pName);
    if (pBud == NULL)
        return -8;
    pBud->uGroup = 1;
    return 0;
}

int HLBListRemoveFromGroup(HLBApiRefT *pApi, const char *pName, int iGroup)
{
    HLBBudT *pBud = HLBListGetBuddyByName(pApi, pName);
    if (pBud == NULL)
        return -8;
    pBud->uGroup = 0;
    return 0;
}

int HLBListClearGroup(HLBApiRefT *pApi, int iGroup)
{
    int iNum = HLBListGetBuddyCount(pApi);
    int i;
    for (i = 0; i < iNum; i++)
    {
        HLBBudT *pBud = HLBListGetBuddyByIndex(pApi, i);
        if (pBud != NULL)
            pBud->uGroup = 0;
    }
    return 0;
}

int HLBListSendMsgToGroup(HLBApiRefT *pApi, int iGroup, const char *pText,
                          void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    char Buffer[0x400];
    int iResult = 0;
    if (HLBApiGetConnectState(pApi) != 2)
        return iResult;
    BuddyApiResource(pApi->pBuddyApi, 0, Buffer);
    if (strchr(Buffer, '/') != NULL)
        *strchr(Buffer, '/') = '\0';
    {
        int iNum = HLBListGetBuddyCount(pApi);
        int i;
        for (i = 0; i < iNum; i++)
        {
            HLBBudT *pBud = HLBListGetBuddyByIndex(pApi, i);
            if (pBud == NULL || !HLBBudIsInGroup((HLBBudT *)pBud))
                continue;
            if (strlen(pText) == 0)
                continue;
            iResult = HLBListSendChatMsg(pApi, pBud->pBuddy->Name, pText, 0, pCallback, pRef);
            strcpy(Buffer, Buffer);
        }
    }
    return iResult;
}

void HLBApiPresenceDiff(HLBApiRefT *pApi, const char *pDomain, const char *pLanguage)
{
    BuddyApiPresDiff(pApi->pBuddyApi, pDomain, pLanguage);
}

int HLBApiPresenceSend(HLBApiRefT *pApi, HLBStatE eState, int iVoipState, const char *pPresence,
                      int iWaitForCompletion)
{
    if (HLBApiGetConnectState(pApi) != 2)
        return -4;
    BuddyApiPresSame(pApi->pBuddyApi, pPresence);
    if (iVoipState != -1)
        BuddyApiPresExFlags(pApi->pBuddyApi, iVoipState);
    if (pApi->bSuppressPresence != 0)
        return 0;
    pApi->iStatus = eState;
    // Medido en el original (move a3,s4 + move t0,zero): el 4.º argumento es
    // iWaitForCompletion y el 5.º es 0, no (pPresence, iWaitForCompletion).
    // VEDADO esta ronda: queda al 88 % por un colorido v0/v1 (el -1 va a v0 en el
    // original y a v1 aqui) que ninguna forma (if, goto, bloque unico) mueve.
    BuddyApiPresSend(pApi->pBuddyApi, NULL, eState, iWaitForCompletion, 0);
    return 0;
}

// Devuelve int (medido: el original carga -4 en v0 en la salida temprana y 0 al
// final), no void como se reconstruyo en la ronda 43.
int HLBApiPresenceVOIPSend(HLBApiRefT *pApi, int iVoipState)
{
    BuddyApiPresExFlags(pApi->pBuddyApi, iVoipState);
    if (HLBApiGetConnectState(pApi) != 2)
        return -4;
    if (pApi->bSuppressPresence != 0)
        return 0;
    BuddyApiPresSend(pApi->pBuddyApi, NULL, pApi->iStatus, NULL, 0);
    return 0;
}

int HLBListSendChatMsg(HLBApiRefT *pApi, const char *pName, const char *pText, int iKind,
                       void (*pCallback)(HLBApiRefT *, int, int, void *), void *pRef)
{
    char Buffer[0x100];
    char Resource[0x100];
    BuddyApiMsgRecT Msg;
    int iResult;
    HLBBudT *pBud = HLBListGetBuddyByName(pApi, pName);
    if (pBud != NULL && HLBBudIsNoReplyBud(pBud) != 0)
        return -2;
    if (HLBApiGetConnectState(pApi) != 2)
        return -4;
    memset(&Msg, 0, sizeof(Msg));
    TagFieldSetString(Buffer, sizeof(Buffer), "TYPE", "C");
    if (strchr(pName, '@') != NULL)
    {
        BuddyApiResource(pApi->pBuddyApi, 1, Resource);
        strcpy(Buffer, Resource);
    }
    else
    {
        BuddyApiResource(pApi->pBuddyApi, 0, Resource);
        strcpy(Buffer, Resource);
        if (strchr(Buffer, '/') != NULL)
            *strchr(Buffer, '/') = '\0';
        sprintf(Buffer + strlen(Buffer), "%s/%s", pName, Resource);
    }
    TagFieldSetString(Buffer, sizeof(Buffer), "USER", pName);
    TagFieldSetNumber(Buffer, sizeof(Buffer), "SECS", 0);
    Msg.pData = Buffer;
    _setUpOp(pApi, pName, pCallback, pRef);
    if (iKind == 0)
        iResult = BuddyApiSend(pApi->pBuddyApi, (BuddyApiMsgT *)&Msg, 0, _HLBOpCallback, pApi,
                               pApi->iRosterTimeout);
    else
        iResult = BuddyApiBroadcast(pApi->pBuddyApi, (BuddyApiMsgT *)&Msg, 0, _HLBOpCallback, pApi,
                                    pApi->iRosterTimeout);
    _setLastOpState(pApi, iResult);
    return iResult;
}

unsigned int HLBBudGetGameInviteFlags(HLBBudT *pBud)
{
    unsigned int uFlags = ((HLBBudT *)pBud)->pBuddy->uFlags & 0x04428C08;
    if (uFlags & 0x4000000)
        ((HLBBudT *)pBud)->pBuddy->uFlags &= 0xFBBD73F7;
    return uFlags;
}

void _HLBConnectCallback(BuddyApiRefT *pBuddyApi, BuddyApiMsgT *pMsgIn, HLBApiRefT *pApi)
{
    BuddyApiMsgRecT *pMsg = (BuddyApiMsgRecT *)pMsgIn;
    unsigned int uType = pMsg->uType;
    // Ifs anidados medidos (el arbol usa slt con signo y prueba == antes que > en
    // cada nivel; un switch bajaria por tabla/mediana y daria otra forma).
    // El externo va en != (el beq salta lejos al bloque auth).
    if (uType != MSGTYPE_AUTH)
    {
        if (uType == MSGTYPE_CONN)
        {
            // Medido (lw v1,4(a1)): el switch de conn va sobre uKind, no sobre uCode.
            unsigned int uKind = pMsg->uKind;
            switch (uKind)
            {
            case 0:
            case 5:
                break;
            case 1:
                pApi->iConnectState = 1;
                break;
            case 3:
                pApi->iConnectState = 7;
                break;
            case 2:
            case 4:
                pApi->iConnectState = 5;
                break;
            case 6:
                if (pApi->iConnectState == 2)
                    pApi->iConnectState = 0;
                break;
            default:
                pApi->iConnectState = 6;
                break;
            }
        }
        else
        {
            // Medido: el marco es de 0xA0 (el nuestro sale de 0x60): el Buffer es de
            // 0x80, no de 0x40. Entre sp+0x40 y sp+0x7F no hay ni un acceso.
            char Buffer[0x80];
            sprintf(Buffer, "Received unknown response '%c%c%c%c'", (char)(uType >> 24),
                    (char)(uType >> 16), (char)(uType >> 8), (char)uType);
        }
    }
    else
    {
        int iCode = pMsg->uCode;
        // Medido: el arbol prueba == antes que > en cada nivel (beq y luego
        // slt+bnez) y el leaf del 0 no re-testea iCode (beqz al camino del 0;
        // el != 0 cae directo al store del 6 sin segunda rama).
        if (iCode == MSGTYPE_HACK)
            pApi->iConnectState = 6;
        else if (iCode > MSGTYPE_HACK)
        {
            if (iCode == MSGTYPE_MSRC)
                pApi->iConnectState = 6;
            else if (iCode > MSGTYPE_MSRC)
            {
                if (iCode == MSGTYPE_NETW)
                    pApi->iConnectState = 5;
                else if (iCode == MSGTYPE_USER)
                    pApi->iConnectState = 4;
                else
                    pApi->iConnectState = 6;
            }
            else
                pApi->iConnectState = 6;
        }
        else if (iCode == MSGTYPE_AUTH)
            pApi->iConnectState = 3;
        else if (iCode > MSGTYPE_AUTH)
            pApi->iConnectState = 6;
        else if (iCode == 0)
        {
            if (pApi->iConnectState == 1)
                pApi->iConnectState = 2;
        }
        else
            pApi->iConnectState = 6;
    }
    if (pApi->iConnectState == 1)
        return;
    if (pApi->iConnectState == 2)
    {
        // Medido: los stores van DENTRO del if (el original comprueba el iState
        // anterior, no el 1 recien guardado). Y el beq va a roster (==2, no !=2).
        pApi->iLastOpState = 0;
        if (pApi->iState != 5)
        {
            pApi->iStatus = 1;
            pApi->iState = 1;
            BuddyApiRoster(pApi->pBuddyApi, _HLBRosterCallback, pApi);
            _preConnTempBudListLoad(pApi);
        }
    }
    else
    {
        _HLBRosterDestroy(pApi);
    }
    if (pApi->pConnectCallback != NULL)
        pApi->pConnectCallback(pApi, 1, pApi->iConnectState, pApi->pConnectRef);
}

void _HLBOpCallback(BuddyApiRefT *pBuddyApi, BuddyApiMsgT *pMsgIn, HLBApiRefT *pApi)
{
    BuddyApiMsgRecT *pMsg = (BuddyApiMsgRecT *)pMsgIn;
    int bCallUser = 0;
    unsigned int uType = pMsg->uType;
    if (uType == MSGTYPE_SEND || uType == MSGTYPE_BRDC || uType == MSGTYPE_ADDU || uType == MSGTYPE_DELU)
    {
        if (pMsg->uCode == MSGTYPE_FULL)
            pMsg->uCode = -5;
        bCallUser = (unsigned)pApi->bPresNoText < 1;
    }
    else if (uType != MSGTYPE_INVT && uType != MSGTYPE_USER)
    {
        if (pMsg->uCode == MSGTYPE_FULL)
            pMsg->uCode = -5;
        bCallUser = (unsigned)pApi->bPresNoText < 1;
    }
    else
    {
        if (pMsg->uCode == MSGTYPE_FULL)
            pMsg->uCode = -5;
    }
    if (bCallUser != 0)
        pApi->iLastOpState = ~pMsg->uCode != 0 ? pMsg->uCode : -0x3E7;
    if (pApi != NULL && pApi->pOpCallback != NULL)
    {
        pApi->pOpCallback(pApi, _msgTypeToOp(pMsg->uType), pMsg->uCode, pApi->pOpRef);
        pApi->pOpCallback = NULL;
    }
}
