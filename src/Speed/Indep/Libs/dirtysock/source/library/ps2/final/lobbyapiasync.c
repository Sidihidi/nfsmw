// DirtySock: lobbyapiasync.c, los procesadores de mensajes asincronos del
// lobby ('+ses', '+usr', '+rom'...). Escrito del asm de la A124
// (libdirtysock.a(lobbyapiasync.o): .text 0x484FC0, 20 etiquetas; la tabla
// de despacho en .data 0x64B660 y los nombres de campo en .sdata 0x654240).
// Unidad que solo enlaza en A124.
//
// Medido del asm (r52):
// - Cada handler recibe (pApi, pSrc, pRef): pRef es el iCode que el
//   despachador desplaza de a3 a a2 al llamar.
// - Los registros: PlayRecord 0x674 (listas 3/4), RoomRecord 0x124 (lista 0,
//   nombre +0x20, flags +0x8), UserRecord 0x244 (lista 1, flags +0x4),
//   UserSet 0xD4 (lista 0x17, nombre +0x28), UserSetMember 0xA0 (listas
//   0x18..0x1C), RankRecord 0xA0 (lista pApi->i1894, llave +0xC).
// - La copia sobre el registro ya listado es un memcpy del sizeof entero.
// - pApi->iIdent (+0x1CB8) es el contador decreciente de identificadores
//   de sala sinteticos (salas sin IDENT o marcadas con el flag 0x2).
// - Los 4 UserSet propios viven en +0x1CC8 y las estadisticas en +0x2018.
#include <string.h>

typedef struct LobbyApiRefT LobbyApiRefT;
typedef struct LobbyApiPlayRecordT LobbyApiPlayRecordT;
typedef struct LobbyApiRoomRecordT LobbyApiRoomRecordT;
typedef struct LobbyApiUserRecordT LobbyApiUserRecordT;
typedef struct LobbyApiUserSetT LobbyApiUserSetT;
typedef struct LobbyApiUserSetMemberT LobbyApiUserSetMemberT;
typedef struct LobbyApiRankRecordT LobbyApiRankRecordT;
typedef struct LobbyApiStatsT LobbyApiStatsT;
typedef void (*LobbyApiAsyncProcT)(LobbyApiRefT *pApi, const char *pSrc, void *pRef);

// --- otras unidades de dirtysock
extern void *LobbyAlloc(int iSize);
extern void LobbyFree(void *pMem);
extern char *TagFieldFind(const char *pRecord, const char *pName);
extern int TagFieldGetNumber(const char *pField, int iDefault);
extern int TagFieldGetFlags(const char *pField, int iDefault);
extern int TagFieldGetString(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault);
extern int TagFieldGetStructure(const char *pField, void *pBuffer, int iBufferSize, const char *pFormat);
extern void *LobbyApiListGetHash(LobbyApiRefT *pApi, int iIndex);
extern void *LobbyApiListGetDisp(LobbyApiRefT *pApi, int iIndex);
extern void LobbyApiListFlush(LobbyApiRefT *pApi, int iIndex);
extern int LobbyApiListFindByName(LobbyApiRefT *pApi, int iIndex, const char *pName);
extern void LobbyApiListObjectUpdate(LobbyApiRefT *pApi, int iIndex, void *pObj, int iArg);
extern void LobbyApiPopul(LobbyApiRefT *pApi, int iKey, int iNum, int iVal);
extern void LobbyApiExtractPlayRecord(LobbyApiPlayRecordT *pRec, const char *pSrc);
extern void LobbyApiExtractRoomRecord(LobbyApiRoomRecordT *pRec, const char *pSrc);
extern void LobbyApiExtractUserRecord(LobbyApiUserRecordT *pRec, const char *pSrc);
extern void LobbyApiExtractUserSet(LobbyApiUserSetT *pRec, const char *pSrc);
extern void LobbyApiExtractUserSetMember(LobbyApiUserSetMemberT *pRec, const char *pSrc);
extern void LobbyApiExtractRankRecord(LobbyApiRankRecordT *pRec, const char *pSrc, const char *pName);
extern int HashNumFind(void *pHasher, int iNum);
extern void HashNumAdd(void *pHasher, int iNum, int iIndex);
extern void HashNumDel(void *pHasher, int iNum);
extern int HashStrFind(void *pHasher, const char *pKey);
extern void HashStrAdd(void *pHasher, const char *pKey, int iIndex);
extern void *DispListGet(void *pList, int iIndex);
extern void DispListSet(void *pList, int iIndex, void *pItem);
extern int DispListAdd(void *pList, void *pItem);
extern void *DispListDel(void *pList, int iIndex);
extern void LobbyApiGlobalCallback();

// el registro de partida ('+agm'/'+gam'): 0x674 B
struct LobbyApiPlayRecordT
{
    int iIdent;                     // +0x00
    char X04[0x674 - 0x04];
};

// el registro de sala ('+rom'): 0x124 B
struct LobbyApiRoomRecordT
{
    int iIdent;                     // +0x00
    int X04;                        // +0x04
    int iFlags;                     // +0x08 (0x2 = conservar al borrar)
    char X0C[0x20 - 0x0C];
    char szName[0x124 - 0x20];      // +0x20
};

// el registro de usuario ('+usr'): 0x244 B; tambien es el Serv del ref
struct LobbyApiUserRecordT
{
    int iIdent;                     // +0x00
    int iFlags;                     // +0x04 (0x200000 = el usuario propio)
    char szName[0x144 - 0x08];      // +0x08
    char Sets[4][0x24];             // +0x144 (los nombres de sus usersets)
    char X1D4[0x244 - 0x1D4];
};

// un userset ('+uss'/'+ust'): 0xD4 B
struct LobbyApiUserSetT
{
    int iIdent;                     // +0x00
    char X04[0x28 - 0x04];
    char szName[0xD4 - 0x28];       // +0x28
};

// un miembro de userset ('+usm'): 0xA0 B
struct LobbyApiUserSetMemberT
{
    int iIdent;                     // +0x00
    char X04[0xA0 - 0x04];
};

// un puesto del ranking ('+snp'): 0xA0 B
struct LobbyApiRankRecordT
{
    char X00[0x0C];
    char szKey[0xA0 - 0x0C];        // +0x0C (la llave del hash de cadenas)
};

// las estadisticas del servidor ('+sst'), en +0x2018 del ref
struct LobbyApiStatsT
{
    int iX00;                       // +0x00
    int iValid;                     // +0x04 (se pone a 1 al recibir)
    int iUIL;                       // +0x08 'UIL'
    int iUIR;                       // +0x0C 'UIR'
    int iUIG;                       // +0x10 'UIG'
    int iGIP;                       // +0x14 'GIP'
    int iGCR;                       // +0x18 'GCR'
    int iGCM;                       // +0x1C 'GCM'
};

// En PC (speed.exe: DirtySDK 3.9.3.0, "Oct 19 2005") la cola de peticiones de lobbyapi.c (+0x1A20) tiene
// 32 entradas de 0x18 B en vez de 0x14: lo que va detras corre 0x80 B (iIdent +0x1D38, UserSets +0x1D48,
// Stats +0x2098; _LobbyApiCreate de PC reserva 0x20F8). Los offsets de abajo son los de la A124.
#ifdef EA_PLATFORM_WIN32
#define LOBBYAPI_QUEUE_EXTRA 0x80
#else
#define LOBBYAPI_QUEUE_EXTRA 0
#endif

// el ref de lobby: solo los campos que toca esta unidad (los offsets mandan)
struct LobbyApiRefT
{
    char X0000[0x15A0];
    LobbyApiUserRecordT Serv;       // +0x15A0 (el usuario propio)
    int iRooms;                     // +0x17E4 (la sala actual)
    int iRflg;                      // +0x17E8 (sus flags)
    char Self[0x24];                // +0x17EC (su nombre)
    char Structure[0x24];           // +0x1810 (el 'C' del '+who', "l*")
    char X1834[0x1894 - 0x1834];
    int i1894;                      // +0x1894 (la lista del ranking en curso)
    char X1898[0x1CB8 - 0x1898 + LOBBYAPI_QUEUE_EXTRA];
    int iIdent;                     // +0x1CB8 (identificadores sinteticos, --)
    char X1CBC[0x1CC8 - 0x1CBC];
    LobbyApiUserSetT UserSets[4];   // +0x1CC8
    LobbyApiStatsT Stats;           // +0x2018
};

void _LobbyApiAsyncProcessRoomChange(LobbyApiRefT *pApi, int iRoom, const char *pName,
                                     int iNum, int iVal, int iFlags)
{
    pApi->iRooms = iRoom;
    LobbyApiPopul(pApi, iRoom, iNum, iVal);
    strncpy(pApi->Self, pName, 0x24);
    pApi->iRflg = iFlags;
    LobbyApiGlobalCallback(pApi, 0, 4, 0x6D6F7665, 0, pApi->Self);
    LobbyApiListFlush(pApi, 1);
    LobbyApiListFlush(pApi, 4);
}

void _LobbyApiAsyncProcess_agm(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    void *pHash = LobbyApiListGetHash(pApi, 4);
    void *pDisp;
    LobbyApiPlayRecordT *pRec;
    int iIdent;
    int iFind;

    if (!pHash)
        return;
    iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "IDENT"), -1);
    if (iIdent < 0)
        return;
    pDisp = LobbyApiListGetDisp(pApi, 4);
    if (TagFieldFind(pSrc, "NAME"))
    {
        int iArg;

        pRec = LobbyAlloc(sizeof(LobbyApiPlayRecordT));
        LobbyApiExtractPlayRecord(pRec, pSrc);
        iFind = HashNumFind(pHash, iIdent);
        if (iFind)
        {
            LobbyApiPlayRecordT *pOld = DispListGet(pDisp, iFind);
            memcpy(pOld, pRec, sizeof(LobbyApiPlayRecordT));
            DispListSet(pDisp, iFind, pOld);
            LobbyFree(pRec);
            pRec = pOld;
            iArg = 0;
        }
        else
        {
            iFind = DispListAdd(pDisp, pRec);
            HashNumAdd(pHash, iIdent, iFind);
            iArg = 1;
        }
        LobbyApiListObjectUpdate(pApi, 4, pRec, iArg);
    }
    else
    {
        iFind = HashNumFind(pHash, iIdent);
        if (iFind)
        {
            HashNumDel(pHash, iIdent);
            pRec = DispListDel(pDisp, iFind);
            LobbyApiListObjectUpdate(pApi, 4, pRec, -1);
            LobbyFree(pRec);
        }
    }
}

void _LobbyApiAsyncProcess_bud(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    LobbyApiGlobalCallback(pApi, 0, 4, 0x62756464, pRef, pSrc);
}

void _LobbyApiAsyncProcess_gam(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    void *pHash = LobbyApiListGetHash(pApi, 3);
    void *pDisp;
    LobbyApiPlayRecordT *pRec;
    int iFind;

    if (!pHash)
        return;
    pDisp = LobbyApiListGetDisp(pApi, 3);
    if (TagFieldFind(pSrc, "NAME"))
    {
        int iArg;

        pRec = LobbyAlloc(sizeof(LobbyApiPlayRecordT));
        LobbyApiExtractPlayRecord(pRec, pSrc);
        iFind = HashNumFind(pHash, pRec->iIdent);
        if (iFind)
        {
            LobbyApiPlayRecordT *pOld = DispListGet(pDisp, iFind);
            memcpy(pOld, pRec, sizeof(LobbyApiPlayRecordT));
            DispListSet(pDisp, iFind, pOld);
            LobbyFree(pRec);
            pRec = pOld;
            iArg = 0;
        }
        else
        {
            iFind = DispListAdd(pDisp, pRec);
            HashNumAdd(pHash, pRec->iIdent, iFind);
            iArg = 1;
        }
        LobbyApiListObjectUpdate(pApi, 3, pRec, iArg);
    }
    else
    {
        int iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "IDENT"), -1);
        iFind = HashNumFind(pHash, iIdent);
        if (iFind)
        {
            HashNumDel(pHash, iIdent);
            pRec = DispListDel(pDisp, iFind);
            LobbyApiListObjectUpdate(pApi, 3, pRec, -1);
            LobbyFree(pRec);
        }
    }
}

void _LobbyApiAsyncProcess_koh(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    LobbyApiGlobalCallback(pApi, 0, 4, 0x6B6F7468, pRef, pSrc);
}

void _LobbyApiAsyncProcess_mgm(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    LobbyApiGlobalCallback(pApi, 0, 4, 0x67616D65, pRef, pSrc);
}

void _LobbyApiAsyncProcess_mlg(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    LobbyApiGlobalCallback(pApi, 0, 4, 0x6D6C6F67, pRef, pSrc);
}

void _LobbyApiAsyncProcess_msg(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    int iFlags = TagFieldGetFlags(TagFieldFind(pSrc, "F"), 0);
    LobbyApiGlobalCallback(pApi, 0, 1, 0x63686174, (void *)iFlags, pSrc);
}

void _LobbyApiAsyncProcess_pop(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    char BufZ[0x300];
    char BufG[0x180];
    char *pZ;
    char *pG;
    int iKey;
    int iNum;
    int iVal;

    if (!LobbyApiListGetHash(pApi, 0))
        return;
    TagFieldGetString(TagFieldFind(pSrc, "Z"), BufZ, 0x300, "");
    TagFieldGetString(TagFieldFind(pSrc, "G"), BufG, 0x180, "");
    pZ = BufZ;
    pG = BufG;
    while (*pZ)
    {
        iKey = 0;
        while ((unsigned)(*pZ - '0') < 10)
            iKey = iKey * 10 + (*pZ++ - '0');
        if (!iKey)
            break;
        if (*pZ)
            pZ++;
        iNum = 0;
        while ((unsigned)(*pZ - '0') < 10)
            iNum = iNum * 10 + (*pZ++ - '0');
        if (*pZ)
            pZ++;
        iVal = 0;
        if (*pG >= '0' && *pZ <= '9')
        {
            do
                iVal = iVal * 10 + (*pG++ - '0');
            while (*pG >= '0');
        }
        LobbyApiPopul(pApi, iKey, iNum, iVal);
        if (*pG)
            pG++;
    }
}

void _LobbyApiAsyncProcess_rom(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    void *pHash = LobbyApiListGetHash(pApi, 0);
    void *pDisp;
    LobbyApiRoomRecordT *pRec;
    int iFind;

    if (!pHash)
        return;
    pDisp = LobbyApiListGetDisp(pApi, 0);
    if (TagFieldFind(pSrc, "N"))
    {
        pRec = LobbyAlloc(sizeof(LobbyApiRoomRecordT));
        LobbyApiExtractRoomRecord(pRec, pSrc);
        iFind = LobbyApiListFindByName(pApi, 0, pRec->szName);
        if (iFind)
        {
            LobbyApiRoomRecordT *pOld = DispListGet(pDisp, iFind);
            if (pRec->iIdent != 0 || pOld->iIdent == 0)
            {
                HashNumDel(pHash, pOld->iIdent);
                LobbyFree(pOld);
                DispListSet(pDisp, iFind, pRec);
                HashNumAdd(pHash, pRec->iIdent, iFind);
                LobbyApiListObjectUpdate(pApi, 0, pRec, 0);
            }
            else
            {
                LobbyFree(pRec);
            }
        }
        else
        {
            if (!pRec->iIdent)
                pRec->iIdent = --pApi->iIdent;
            iFind = DispListAdd(pDisp, pRec);
            HashNumAdd(pHash, pRec->iIdent, iFind);
            LobbyApiListObjectUpdate(pApi, 0, pRec, 1);
        }
    }
    else
    {
        int iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "I"), -1);
        LobbyApiRoomRecordT *pObj;

        if (!iIdent)
            return;
        iFind = HashNumFind(pHash, iIdent);
        if (!iFind)
            return;
        pObj = DispListGet(pDisp, iFind);
        if (pObj->iFlags & 2)
        {
            HashNumDel(pHash, iIdent);
            pObj->iIdent = --pApi->iIdent;
            HashNumAdd(pHash, pObj->iIdent, iFind);
        }
        else
        {
            HashNumDel(pHash, iIdent);
            pObj = DispListDel(pDisp, iFind);
            LobbyApiListObjectUpdate(pApi, 0, pObj, -1);
            LobbyFree(pObj);
        }
    }
}

void _LobbyApiAsyncProcess_ses(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    LobbyApiGlobalCallback(pApi, 0, 4, 0x706C6179, pRef, pSrc);
}

void _LobbyApiAsyncProcess_snp(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    void *pHash;
    void *pDisp;
    LobbyApiRankRecordT *pRec;
    int iFind;

    if ((pApi->i1894 < 5) || (pApi->i1894 > 0x16))
        return;
    pHash = LobbyApiListGetHash(pApi, pApi->i1894);
    if (!pHash)
        return;
    pRec = LobbyAlloc(sizeof(LobbyApiRankRecordT));
    LobbyApiExtractRankRecord(pRec, pSrc, pApi->Serv.szName);
    {
        int iArg;

        pDisp = LobbyApiListGetDisp(pApi, pApi->i1894);
        iFind = HashStrFind(pHash, pRec->szKey);
        if (!iFind)
        {
            iFind = DispListAdd(pDisp, pRec);
            HashStrAdd(pHash, pRec->szKey, iFind);
            iArg = 1;
        }
        else
        {
            LobbyApiRankRecordT *pOld = DispListGet(pDisp, iFind);
            memcpy(pOld, pRec, sizeof(LobbyApiRankRecordT));
            DispListSet(pDisp, iFind, pOld);
            LobbyFree(pRec);
            pRec = pOld;
            iArg = 0;
        }
        LobbyApiListObjectUpdate(pApi, pApi->i1894, pRec, iArg);
    }
}

void _LobbyApiAsyncProcess_sst(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    LobbyApiStatsT *pStats = &pApi->Stats;

    pStats->iValid = 1;
    pStats->iUIL = TagFieldGetNumber(TagFieldFind(pSrc, "UIL"), pStats->iUIL);
    pStats->iUIR = TagFieldGetNumber(TagFieldFind(pSrc, "UIR"), pStats->iUIR);
    pStats->iUIG = TagFieldGetNumber(TagFieldFind(pSrc, "UIG"), pStats->iUIG);
    pStats->iGIP = TagFieldGetNumber(TagFieldFind(pSrc, "GIP"), pStats->iGIP);
    pStats->iGCR = TagFieldGetNumber(TagFieldFind(pSrc, "GCR"), pStats->iGCR);
    pStats->iGCM = TagFieldGetNumber(TagFieldFind(pSrc, "GCM"), pStats->iGCM);
    LobbyApiGlobalCallback(pApi, 0, 4, 0x73746174, pRef, pSrc);
}

void _LobbyApiAsyncProcess_usm(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    int iSet = TagFieldGetNumber(TagFieldFind(pSrc, "S"), -1);
    void *pHash;
    void *pDisp;
    LobbyApiUserSetMemberT *pRec;
    int iFind;

    if (iSet < 0)
        return;
    iSet += 0x18;
    if ((iSet < 0x18) || (iSet > 0x1C))
        return;
    pHash = LobbyApiListGetHash(pApi, iSet);
    if (!pHash)
        return;
    pDisp = LobbyApiListGetDisp(pApi, iSet);
    if (TagFieldFind(pSrc, "N"))
    {
        int iArg;

        pRec = LobbyAlloc(sizeof(LobbyApiUserSetMemberT));
        LobbyApiExtractUserSetMember(pRec, pSrc);
        iFind = HashNumFind(pHash, pRec->iIdent);
        if (iFind)
        {
            LobbyApiUserSetMemberT *pOld = DispListGet(pDisp, iFind);
            memcpy(pOld, pRec, sizeof(LobbyApiUserSetMemberT));
            DispListSet(pDisp, iFind, pOld);
            LobbyFree(pRec);
            pRec = pOld;
            iArg = 0;
        }
        else
        {
            iFind = DispListAdd(pDisp, pRec);
            HashNumAdd(pHash, pRec->iIdent, iFind);
            iArg = 1;
        }
        LobbyApiListObjectUpdate(pApi, iSet, pRec, iArg);
    }
    else
    {
        int iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "I"), 0);
        iFind = HashNumFind(pHash, iIdent);
        if (iFind)
        {
            HashNumDel(pHash, iIdent);
            pRec = DispListDel(pDisp, iFind);
            LobbyApiListObjectUpdate(pApi, iSet, pRec, -1);
            LobbyFree(pRec);
        }
    }
}

void _LobbyApiAsyncProcess_usr(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    void *pHash = LobbyApiListGetHash(pApi, 1);
    void *pDisp;
    LobbyApiUserRecordT *pRec;
    int iIdent;
    int iFind;

    if (!pHash)
        return;
    iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "I"), -1);
    if (iIdent < 0)
        return;
    pDisp = LobbyApiListGetDisp(pApi, 1);
    if (TagFieldFind(pSrc, "N"))
    {
        int iArg;

        pRec = LobbyAlloc(sizeof(LobbyApiUserRecordT));
        LobbyApiExtractUserRecord(pRec, pSrc);
        iFind = HashNumFind(pHash, pRec->iIdent);
        if (iFind)
        {
            LobbyApiUserRecordT *pOld = DispListGet(pDisp, iFind);
            memcpy(pOld, pRec, sizeof(LobbyApiUserRecordT));
            DispListSet(pDisp, iFind, pOld);
            LobbyFree(pRec);
            pRec = pOld;
            iArg = 0;
        }
        else
        {
            iFind = DispListAdd(pDisp, pRec);
            HashNumAdd(pHash, pRec->iIdent, iFind);
            iArg = 1;
        }
        if (pRec->iFlags & 0x200000)
            memcpy(&pApi->Serv, pRec, sizeof(LobbyApiUserRecordT));
        LobbyApiListObjectUpdate(pApi, 1, pRec, iArg);
    }
    else
    {
        iFind = HashNumFind(pHash, iIdent);
        if (iFind)
        {
            HashNumDel(pHash, iIdent);
            pRec = DispListDel(pDisp, iFind);
            LobbyApiListObjectUpdate(pApi, 1, pRec, -1);
            LobbyFree(pRec);
        }
    }
    LobbyApiPopul(pApi, pApi->iRooms, TagFieldGetNumber(TagFieldFind(pSrc, "T"), 0),
                  TagFieldGetNumber(TagFieldFind(pSrc, "RG"), -1));
}

void _LobbyApiAsyncProcess_uss(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    void *pHash = LobbyApiListGetHash(pApi, 0x17);
    void *pDisp;
    LobbyApiUserSetT *pRec;
    int iFind;

    if (!pHash)
        return;
    pRec = LobbyAlloc(sizeof(LobbyApiUserSetT));
    LobbyApiExtractUserSet(pRec, pSrc);
    {
        int iArg;

        pDisp = LobbyApiListGetDisp(pApi, 0x17);
        iFind = HashStrFind(pHash, pRec->szName);
        if (!iFind)
        {
            iFind = DispListAdd(pDisp, pRec);
            HashStrAdd(pHash, pRec->szName, iFind);
            iArg = 1;
        }
        else
        {
            LobbyApiUserSetT *pOld = DispListGet(pDisp, iFind);
            memcpy(pOld, pRec, sizeof(LobbyApiUserSetT));
            DispListSet(pDisp, iFind, pOld);
            LobbyFree(pRec);
            pRec = pOld;
            iArg = 0;
        }
        LobbyApiListObjectUpdate(pApi, 0x17, pRec, iArg);
    }
}

void _LobbyApiAsyncProcess_ust(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    int iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "I"), 0);
    int i;

    if (TagFieldFind(pSrc, "N"))
    {
        int iFound = -1;

        for (i = 0; i < 4; i++)
        {
            if (pApi->UserSets[i].iIdent == iIdent)
            {
                iFound = i;
                break;
            }
        }
        if (iFound != -1)
            LobbyApiExtractUserSet(&pApi->UserSets[iFound], pSrc);
    }
    else
    {
        LobbyApiUserSetT *pSet = pApi->UserSets;

        for (i = 0; i < 4; i++, pSet++)
        {
            if (pSet->iIdent == iIdent)
            {
                memset(pSet, 0, sizeof(LobbyApiUserSetT));
                break;
            }
        }
    }
    LobbyApiGlobalCallback(pApi, 0, 4, 0x75736574, pRef, pSrc);
}

void _LobbyApiAsyncProcess_who(LobbyApiRefT *pApi, const char *pSrc, void *pRef)
{
    char Buf[0x30];
    int iRI;
    int iRT;
    int iRG;
    int i;
    int j;

    LobbyApiExtractUserRecord(&pApi->Serv, pSrc);
    TagFieldGetStructure(TagFieldFind(pSrc, "C"), pApi->Structure, 0x24, "l*");
    iRI = TagFieldGetNumber(TagFieldFind(pSrc, "RI"), 0);
    iRT = TagFieldGetNumber(TagFieldFind(pSrc, "RT"), 0);
    iRG = TagFieldGetNumber(TagFieldFind(pSrc, "RG"), -1);
    for (i = 0; i < 4; i++)
    {
        int iFound = 0;

        for (j = 0; j < 4; j++)
        {
            if (strcmp(pApi->UserSets[i].szName, pApi->Serv.Sets[j]) == 0)
            {
                iFound = 1;
                break;
            }
        }
        if (!iFound)
            memset(&pApi->UserSets[i], 0, sizeof(LobbyApiUserSetT));
    }
    LobbyApiGlobalCallback(pApi, 0, 4, 0x75736572, pRef, pSrc);
    if (iRI != pApi->iRooms)
    {
        int iFlags;

        TagFieldGetString(TagFieldFind(pSrc, "RM"), Buf, 0x24, "");
        iFlags = TagFieldGetFlags(TagFieldFind(pSrc, "RF"), 0);
        _LobbyApiAsyncProcessRoomChange(pApi, iRI, Buf, iRT, iRG, iFlags);
    }
}

// la tabla de despacho por tipo de mensaje ('+ses', '+mgm'...), cerrada en 0
typedef struct LobbyApiAsyncTableT
{
    int iKind;
    LobbyApiAsyncProcT pProc;
} LobbyApiAsyncTableT;

LobbyApiAsyncTableT LobbyApiAsyncTable[] =
{
    { 0x2B736573, _LobbyApiAsyncProcess_ses },
    { 0x2B6D676D, _LobbyApiAsyncProcess_mgm },
    { 0x2B6D7367, _LobbyApiAsyncProcess_msg },
    { 0x2B77686F, _LobbyApiAsyncProcess_who },
    { 0x2B726F6D, _LobbyApiAsyncProcess_rom },
    { 0x2B706F70, _LobbyApiAsyncProcess_pop },
    { 0x2B757372, _LobbyApiAsyncProcess_usr },
    { 0x2B757373, _LobbyApiAsyncProcess_uss },
    { 0x2B757374, _LobbyApiAsyncProcess_ust },
    { 0x2B75736D, _LobbyApiAsyncProcess_usm },
    { 0x2B736E70, _LobbyApiAsyncProcess_snp },
    { 0x2B61676D, _LobbyApiAsyncProcess_agm },
    { 0x2B67616D, _LobbyApiAsyncProcess_gam },
    { 0x2B737374, _LobbyApiAsyncProcess_sst },
    { 0x2B627564, _LobbyApiAsyncProcess_bud },
    { 0x2B6D6C67, _LobbyApiAsyncProcess_mlg },
    { 0x2B6B6F68, _LobbyApiAsyncProcess_koh },
    { 0, 0 }
};

void LobbyApiAsyncProcess(LobbyApiRefT *pApi, const char *pSrc, int iKind, void *pRef)
{
    int i;

    for (i = 0; LobbyApiAsyncTable[i].iKind; i++)
    {
        if (LobbyApiAsyncTable[i].iKind == iKind)
        {
            LobbyApiAsyncTable[i].pProc(pApi, pSrc, pRef);
            break;
        }
    }
}

void LobbyApiAsyncMoveRoom(LobbyApiRefT *pApi, const char *pSrc, void *pX, void *pY)
{
    char Buf[0x30];
    int iIdent;
    int iCount;
    int iGCount;

    iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "LIDENT"), -1);
    if (iIdent >= 0)
    {
        iCount = TagFieldGetNumber(TagFieldFind(pSrc, "LCOUNT"), 0);
        iGCount = TagFieldGetNumber(TagFieldFind(pSrc, "LGCOUNT"), -1);
        LobbyApiPopul(pApi, iIdent, iCount, iGCount);
    }
    iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "IDENT"), 0);
    iCount = TagFieldGetNumber(TagFieldFind(pSrc, "COUNT"), 0);
    iGCount = TagFieldGetNumber(TagFieldFind(pSrc, "GCOUNT"), -1);
    if (iIdent != pApi->iRooms)
    {
        int iFlags;

        TagFieldGetString(TagFieldFind(pSrc, "NAME"), Buf, 0x24, "");
        iFlags = TagFieldGetFlags(TagFieldFind(pSrc, "FLAGS"), 0);
        _LobbyApiAsyncProcessRoomChange(pApi, iIdent, Buf, iCount, iGCount, iFlags);
    }
}
