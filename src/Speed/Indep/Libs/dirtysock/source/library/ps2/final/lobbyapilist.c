// DirtySock: lobbyapilist.c, el gestor de listas del lobby (hash + display por
// cada indice). Escrito del asm de la A124 (libdirtysock.a(lobbyapilist.o):
// .text 0x4869C8, 13 etiquetas incl. 1 pad). Unidad solo enlaza en A124.
//
// Medido del asm (r52):
// - pApi->pList (+0x1834) es un array de 0x1D entradas de 0x14 B (LobbyAlloc
//   0x244 = 29 * 20). Entrada: pHash +0x00 (HasherCreate), pDisp +0x04
//   (DispListCreate), pCallback +0x08, pRef +0x0C, iX +0x10.
// - Cada entrada cuelga de un hasher (1000/2000) y un displist (100/100/0).
// - El objeto del displist lleva iNum +0x10, iVal +0x14, szName +0x18.
#include <stdio.h>
#include <string.h>

typedef struct LobbyApiRefT LobbyApiRefT;
typedef struct LobbyApiMsgT LobbyApiMsgT;
typedef struct LobbyApiListT LobbyApiListT;
typedef struct LobbyApiObjT LobbyApiObjT;
typedef struct DispListT DispListT;
typedef void (*LobbyApiCallbackT)(LobbyApiRefT *, LobbyApiMsgT *, void *);

// --- hasher / displist / alloc (otras unidades dirtysock)
extern void *LobbyAlloc(int iSize);
extern void LobbyFree(void *pMem);
extern void *HasherCreate(int iX, int iY);
extern void *HasherFlush(void *pHasher);
extern void HasherDestroy(void *pHasher);
extern int HashStrFind(void *pHasher, const void *pKey);
extern int HashNumFind(void *pHasher, int iNum);
extern int HasherEnumInit(void *pHasher);
extern int HasherEnum(void *pHasher, int iX);
extern DispListT *DispListCreate(int iSortMode, int iSortDir, int iSortKeySize);
extern void *DispListGet(DispListT *pList, int iIndex);
extern void *DispListDel();
extern void DispListDestroy(DispListT *pList);
extern int DispListOrder(DispListT *pList);
extern void DispListSet(DispListT *pList, int iIndex, void *pItem);
extern void DispListDataSet(DispListT *pList, void *pData);
extern void *DispListDataGet(void *pList);
extern int ds_stricmp(const char *pA, const char *pB);

// el mensaje que reciben los callbacks (por valor, en el stack del llamador)
struct LobbyApiMsgT
{
    int iCode;                      // +0x00
    unsigned int uKind;             // +0x04
    void *pData;                    // +0x08
    void *pRef;                     // +0x0C
    void *p1;                       // +0x10
    void *p2;                       // +0x14
};

// el ref de lobby: solo los campos que toca esta unidad (los offsets mandan)
struct LobbyApiRefT
{
    char X00[0x10];
    int iData;                      // +0x10 (Control 'dobj')
    char X14[0x1834 - 0x14];
    LobbyApiListT *pList;           // +0x1834
};

// una entrada de lista: 0x14 B
struct LobbyApiListT
{
    void *pHash;                    // +0x00 (HasherCreate 1000/2000)
    void *pDisp;                    // +0x04 (DispListCreate 100/100/0)
    LobbyApiCallbackT pCallback;    // +0x08
    void *pRef;                     // +0x0C
    int iX;                         // +0x10
};

// el objeto guardado en el displist
struct LobbyApiObjT
{
    char X00[0x10];
    int iNum;                       // +0x10
    int iVal;                       // +0x14
    char szKey[0x8];                // +0x18 (sprintf "%d")
    char szName[0x20];              // +0x20 (ds_stricmp)
};

// 0x1D entradas por lista
#define LOBBYAPI_LIST_COUNT 0x1D

static void _LobbyApiListFlush(LobbyApiListT *pList, int iIndex);
void LobbyApiListObjectUpdate(LobbyApiRefT *pApi, int iIndex, void *pObj, int iArg);

// vacia el contenido (hasher + displist) de una entrada
static void _LobbyApiListFlush(LobbyApiListT *pList, int iIndex)
{
    void **ppHash = &pList[iIndex].pHash;
    void **ppDisp = &pList[iIndex].pDisp;
    void *pKey;

    if (*ppHash)
    {
        while ((pKey = HasherFlush(*ppHash)) != 0)
        {
            void *pObj = DispListDel(*ppDisp, pKey);
            LobbyFree(pObj);
        }
    }
}

void LobbyApiListCreate(LobbyApiRefT *pApi)
{
    void *pList = LobbyAlloc(0x244);
    pApi->pList = (LobbyApiListT *)pList;
    if (pList)
        memset(pList, 0, 0x244);
}

void LobbyApiListFlush(LobbyApiRefT *pApi, int iX)
{
    LobbyApiListT *pList = pApi->pList;
    int i;

    if (!pList)
        return;
    if (iX == -1)
    {
        for (i = 0; i < LOBBYAPI_LIST_COUNT; i++)
            _LobbyApiListFlush(pList, i);
    }
    else
    {
        _LobbyApiListFlush(pList, iX);
    }
}

void LobbyApiListDestroy(LobbyApiRefT *pApi)
{
    LobbyApiListT *pList = pApi->pList;
    int i;

    if (pList)
    {
        LobbyApiListT *pEntry;
        for (i = 0, pEntry = pList; i < LOBBYAPI_LIST_COUNT; i++, pEntry++)
        {
            if (pEntry->pHash)
            {
                _LobbyApiListFlush(pList, i);
                HasherDestroy(pEntry->pHash);
                DispListDestroy(pEntry->pDisp);
            }
        }
        LobbyFree(pList);
        pApi->pList = 0;
    }
}

void *LobbyApiListGetDisp(LobbyApiRefT *pApi, int iIndex)
{
    LobbyApiListT *pList = pApi->pList;
    if (pList)
        return pList[iIndex].pDisp;
    return 0;
}

void *LobbyApiListGetHash(LobbyApiRefT *pApi, int iIndex)
{
    LobbyApiListT *pList = pApi->pList;
    if (pList)
        return pList[iIndex].pHash;
    return 0;
}

void *LobbyApiListAlloc(LobbyApiRefT *pApi, int iIndex, LobbyApiCallbackT pCallback, void *pRef)
{
    LobbyApiListT *pList = pApi->pList;
    LobbyApiListT *pEntry;
    void *pDisp;

    if (!pList)
        return 0;
    if ((unsigned)iIndex >= LOBBYAPI_LIST_COUNT)
        return 0;

    pEntry = pList + iIndex;
    pEntry->pCallback = pCallback;
    pEntry->pRef = pRef;
    pEntry->pHash = HasherCreate(0x3E8, 0x7D0);
    pDisp = DispListCreate(0x64, 0x64, 0);
    pEntry->pDisp = pDisp;
    DispListDataSet(pDisp, pEntry->pHash);
    return pEntry->pDisp;
}

void LobbyApiListFree(LobbyApiRefT *pApi, int iIndex, void *pObj)
{
    LobbyApiListT *pList = pApi->pList;
    LobbyApiListT *pEntry;

    if (!pList)
        return;
    if (iIndex < 0)
        return;
    if (iIndex >= LOBBYAPI_LIST_COUNT)
        return;
    if (!pObj)
        return;

    {
        void *pData = DispListDataGet(pObj);
        void *pKey;
        while ((pKey = HasherFlush(pData)) != 0)
        {
            void *pDel = DispListDel(pObj, pKey);
            LobbyFree(pDel);
        }
        HasherDestroy(pData);
        DispListDestroy(pObj);
    }

    pEntry = pList + iIndex;
    if (pEntry->pDisp == pObj)
    {
        pEntry->pHash = 0;
        pEntry->pDisp = 0;
    }
    pEntry->pCallback = 0;
    pEntry->pRef = 0;
}

int LobbyApiListFindByName(LobbyApiRefT *pApi, int iIndex, const char *pName)
{
    LobbyApiListT *pList = pApi->pList;
    void *pHash;

    if ((iIndex < 0) || (iIndex >= LOBBYAPI_LIST_COUNT))
        return 0;
    if ((pList != NULL) && ((pHash = pList[iIndex].pHash) != NULL))
    {
        if (iIndex == 0)
        {
            int iEnum;
            HasherEnumInit(pHash);
            while ((iEnum = HasherEnum(pHash, 0)) != 0)
            {
                LobbyApiObjT *pObj = (LobbyApiObjT *)DispListGet(pList[iIndex].pDisp, iEnum);
                if (ds_stricmp(pObj->szName, pName) == 0)
                    return iEnum;
            }
            return 0;
        }
        return HashStrFind(pHash, pName);
    }
    return 0;
}

void LobbyApiPopul(LobbyApiRefT *pApi, int iKey, int iNum, int iVal)
{
    LobbyApiListT *pList = pApi->pList;
    LobbyApiListT *pEntry;
    void *pHash;
    int iFind;

    if (!pList)
        return;
    pEntry = pList;
    pHash = pEntry->pHash;
    if (!pHash)
        return;
    iFind = HashNumFind(pHash, iKey);
    if (!iFind)
        return;
    {
        LobbyApiObjT *pObj = (LobbyApiObjT *)DispListGet(pEntry->pDisp, iFind);
        if (!pObj)
            return;
        if (iVal == -1)
            iVal = pObj->iVal;
        if (iNum != pObj->iNum)
        {
            pObj->iVal = iVal;
        }
        else
        {
            if (iVal == pObj->iVal)
                return;
            pObj->iVal = iVal;
        }
        sprintf(pObj->szKey, "%d", pObj->iNum = iNum);
        DispListSet(pEntry->pDisp, iFind, pObj);
        LobbyApiListObjectUpdate(pApi, 0, pObj, 0);
    }
}

void LobbyApiListObjectUpdate(LobbyApiRefT *pApi, int iIndex, void *pObj, int iArg)
{
    LobbyApiListT *pList = pApi->pList;
    LobbyApiListT *pEntry;
    LobbyApiMsgT Msg;

    if (!pList)
        return;
    pEntry = pList + iIndex;
    if (!pApi->iData)
        return;
    if (!pEntry->pCallback)
        return;

    Msg.iCode = 0;
    Msg.uKind = 2;
    Msg.pData = (void *)iIndex;
    Msg.pRef = (void *)iArg;
    Msg.p1 = pEntry->pDisp;
    Msg.p2 = pObj;
    pEntry->pCallback(pApi, &Msg, pEntry->pRef);
}

void LobbyApiListUpdate(LobbyApiRefT *pApi)
{
    LobbyApiListT *pList = pApi->pList;
    LobbyApiListT *pEntry;
    int i;

    if (!pList)
        return;
    if (pApi->iData == 1)
        return;

    for (i = 0, pEntry = pList; i < LOBBYAPI_LIST_COUNT; i++, pEntry++)
    {
        if (pEntry->pHash && pEntry->pCallback)
        {
            if (DispListOrder(pEntry->pDisp) > 0)
            {
                LobbyApiMsgT Msg;
                Msg.iCode = 0;
                Msg.uKind = 2;
                Msg.pData = (void *)i;
                Msg.pRef = 0;
                Msg.p1 = pEntry->pDisp;
                Msg.p2 = 0;
                pEntry->pCallback(pApi, &Msg, pEntry->pRef);
            }
        }
    }
}
