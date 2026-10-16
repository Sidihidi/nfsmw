// DirtySock: lobbyapiutil.c, utilidades de extraccion de registros del lobby.
// Escrito del asm de la A124 (libdirtysock.a(lobbyapiutil.o): .text 0x487040,
// 9 etiquetas). Unidad solo enlaza en A124. Los Extract* leen campos TagField
// de un registro fuente y los guardan en un struct destino (offsets del asm).
#include <stdio.h>
#include <string.h>

typedef struct LobbyApiRefT LobbyApiRefT;
typedef struct LobbyApiPlayRecordT LobbyApiPlayRecordT;
typedef struct LobbyApiRoomRecordT LobbyApiRoomRecordT;
typedef struct LobbyApiUserRecordT LobbyApiUserRecordT;
typedef struct LobbyApiUserSetT LobbyApiUserSetT;
typedef struct LobbyApiUserSetMemberT LobbyApiUserSetMemberT;
typedef struct LobbyApiRankRecordT LobbyApiRankRecordT;

// --- tagfield / dirtyaddr (otras unidades dirtysock)
extern char *TagFieldFind(const char *pRecord, const char *pName);
extern int TagFieldGetNumber(const char *pField, int iDefault);
extern int TagFieldGetFlags(const char *pField, int iDefault);
extern int TagFieldGetString(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault);
extern int TagFieldGetRaw(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault);
extern int TagFieldGetAddress(const char *pField, int iDefault);
extern int TagFieldGetEpoch(const char *pField, int iDefault);
extern int TagFieldGetToken(const char *pField, int iDefault);
extern int TagFieldGetStructure(const char *pField, void *pBuffer, int iBufferSize, const char *pFormat);
extern int TagFieldGetDelim(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault, int iIndex, int cDelim);
extern void DirtyAddrFromHostAddr(void *pHost, void *pAddr);
extern int LobbyNameCmp(const char *pA, const char *pB);

void LobbyApiColor(unsigned char *pColor, int iValue);
void LobbyApiExtractUserAddr(LobbyApiUserRecordT *pRec, const char *pSrc);

// un oponente de la partida: 0x8C B
typedef struct LobbyApiOpponentT
{
    int iIdent;                     // +0x00 'OPID%d'
    char szName[0x10];              // +0x04 'OPPO%d'
    int iAddr;                      // +0x14 'ADDR%d'
    int iLAddr;                     // +0x18 'LADDR%d'
    char szMAddr[0x40];             // +0x1C 'MADDR%d'
    int iPart;                      // +0x5C 'OPPART%d'
    int iFlag;                      // +0x60 'OPFLAG%d'
    int iPres;                      // +0x64 'PRES%d'
    char szParam[0x24];             // +0x68 'OPPARAM%d'
} LobbyApiOpponentT;

// una parte de la partida: 0x28 B
typedef struct LobbyApiPartT
{
    int iSize;                      // +0x00 'PARTSIZE%d'
    char szParams[0x24];            // +0x04 'PARTPARAMS%d'
} LobbyApiPartT;

// el registro de partida: 0x674 B
struct LobbyApiPlayRecordT
{
    int iIdent;                     // +0x000 'IDENT'
    char szName[0x24];              // +0x004 'NAME'
    char szSelf[0x10];              // +0x028 'SELF'
    char szHost[0x10];              // +0x038 'HOST'
    int iRoom;                      // +0x048 'ROOM'
    char szSess[0x80];              // +0x04C 'SESS'
    char szParams[0x84];            // +0x0CC 'PARAMS'
    char szPlatParams[0x10];        // +0x150 'PLATPARAMS'
    int iCustFlags;                 // +0x160 'CUSTFLAGS'
    int iSysFlags;                  // +0x164 'SYSFLAGS'
    int iMinSize;                   // +0x168 'MINSIZE'
    int iMaxSize;                   // +0x16C 'MAXSIZE'
    int iSeed;                      // +0x170 'SEED'
    int iWhen;                      // +0x174 'WHEN'
    char szAuth[0x40];              // +0x178 'AUTH'
    int iNumPart;                   // +0x1B8 'NUMPART'
    LobbyApiPartT Parts[2];         // +0x1BC
    int iCount;                     // +0x20C 'COUNT'
    int iPriv;                      // +0x210 'PRIV'
    LobbyApiOpponentT Opponents[8]; // +0x214
};

// el registro de sala: 0x124 B
struct LobbyApiRoomRecordT
{
    int iIdent;                     // +0x000 'I'
    int X04;                        // +0x004
    int iFlags;                     // +0x008 'F'
    int iL;                         // +0x00C 'L'
    int iT;                         // +0x010 'T'
    int iGT;                        // +0x014 'GT'
    char szT[0x4];                  // +0x018 "%d" de iT
    char szPriv[0x4];               // +0x01C "Yes"/"No" (flag 0x10000)
    char szName[0x24];              // +0x020 'N'
    char szDN[0x24];                // +0x044 'DN'
    char szHost[0x10];              // +0x068 'H'
    char szDesc[0x84];              // +0x078 'D'
    int iLO;                        // +0x0FC 'LO'
    char Structure[0x24];           // +0x100 'C'
};

// el registro de usuario: 0x244 B
struct LobbyApiUserRecordT
{
    int iIdent;                     // +0x000 'I'
    int iFlags;                     // +0x004 'F'
    char szName[0x10];              // +0x008 'N'
    char szP[0x8];                  // +0x018 'P'
    int iAddr;                      // +0x020 'A'
    int iR;                         // +0x024 'R'
    char szS[0x84];                 // +0x028 'S'
    char szX[0x84];                 // +0x0AC 'X'
    int iG;                         // +0x130 'G'
    int iAT;                        // +0x134 'AT'
    unsigned char Color[0x4];       // +0x138 'CL'
    int iLV;                        // +0x13C 'LV'
    int iMD;                        // +0x140 'MD'
    char Sets[4][0x24];             // +0x144 'US' por comas
    int iHW;                        // +0x1D4 'HW'
    int iRP;                        // +0x1D8 'RP'
    char szRP[0x4];                 // +0x1DC "%03d" de iRP
    char szMAddr[0x40];             // +0x1E0 'MA'
    int iLAddr;                     // +0x220 'LA'
    int iLO;                        // +0x224 'LO'
    char szCI[0x14];                // +0x228 'CI'
    char szCT[0x8];                 // +0x23C 'CT'
};

// un userset: 0xD4 B (sin memset al extraer)
struct LobbyApiUserSetT
{
    int iIdent;                     // +0x00 'I'
    int iT;                         // +0x04 'T'
    int iSF;                        // +0x08 'SF'
    int iCF;                        // +0x0C 'CF'
    char szO[0x10];                 // +0x10 'O'
    int iS;                         // +0x20 'S'
    int iC;                         // +0x24 'C'
    char szName[0x24];              // +0x28 'N'
    char szD[0x44];                 // +0x4C 'D'
    char szP[0x44];                 // +0x90 'P'
};

// un miembro de userset: 0xA0 B
struct LobbyApiUserSetMemberT
{
    int iIdent;                     // +0x00 'I'
    char szName[0x10];              // +0x04 'N'
    int iFlags;                     // +0x14 'F'
    int iG;                         // +0x18 'G'
    char szX[0x84];                 // +0x1C 'X'
};

// un puesto del ranking: 0xA0 B
struct LobbyApiRankRecordT
{
    int iP;                         // +0x00 'P'
    int iR;                         // +0x04 'R'
    int iFlags;                     // +0x08 (0x4 si 'O', 0x1 si es el propio)
    char szName[0x10];              // +0x0C 'N'
    char szS[0x84];                 // +0x1C 'S'
};

// sintetiza la direccion de maquina a partir del host (vacio) y la addr;
// los llamadores pasan tambien la addr local (a2 en su asm), que no se usa
void _LobbyApiSynthesizeMachineAddr(void *pHost, int iAddr, int iLAddr)
{
    if (*(char *)pHost == 0)
    {
        if (iAddr != 0)
            DirtyAddrFromHostAddr(pHost, &iAddr);
    }
}

void LobbyApiExtractUserSet(LobbyApiUserSetT *pRec, const char *pSrc)
{
    pRec->iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "I"), 0);
    pRec->iT = TagFieldGetNumber(TagFieldFind(pSrc, "T"), 0);
    pRec->iSF = TagFieldGetFlags(TagFieldFind(pSrc, "SF"), 0);
    pRec->iCF = TagFieldGetFlags(TagFieldFind(pSrc, "CF"), 0);
    TagFieldGetString(TagFieldFind(pSrc, "O"), pRec->szO, 0x10, "");
    pRec->iS = TagFieldGetNumber(TagFieldFind(pSrc, "S"), 0);
    pRec->iC = TagFieldGetNumber(TagFieldFind(pSrc, "C"), 0);
    TagFieldGetString(TagFieldFind(pSrc, "N"), pRec->szName, 0x24, "");
    TagFieldGetString(TagFieldFind(pSrc, "D"), pRec->szD, 0x44, "");
    TagFieldGetString(TagFieldFind(pSrc, "P"), pRec->szP, 0x44, "");
}

void LobbyApiExtractPlayRecord(LobbyApiPlayRecordT *pRec, const char *pSrc)
{
    LobbyApiOpponentT *pOpp;
    int i;
    char Buf[0x20];

    memset(pRec, 0, sizeof(LobbyApiPlayRecordT));
    pRec->iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "IDENT"), -1);
    TagFieldGetString(TagFieldFind(pSrc, "NAME"), pRec->szName, 0x24, "");
    TagFieldGetString(TagFieldFind(pSrc, "SELF"), pRec->szSelf, 0x10, "");
    TagFieldGetString(TagFieldFind(pSrc, "HOST"), pRec->szHost, 0x10, "");
    TagFieldGetString(TagFieldFind(pSrc, "PARAMS"), pRec->szParams, 0x84, "");
    TagFieldGetString(TagFieldFind(pSrc, "PLATPARAMS"), pRec->szPlatParams, 0x10, "");
    pRec->iRoom = TagFieldGetNumber(TagFieldFind(pSrc, "ROOM"), 0);
    pRec->iCustFlags = TagFieldGetNumber(TagFieldFind(pSrc, "CUSTFLAGS"), 0);
    pRec->iSysFlags = TagFieldGetNumber(TagFieldFind(pSrc, "SYSFLAGS"), 0);
    pRec->iCount = TagFieldGetNumber(TagFieldFind(pSrc, "COUNT"), 0);
    pRec->iPriv = TagFieldGetNumber(TagFieldFind(pSrc, "PRIV"), 0);
    pRec->iMinSize = TagFieldGetNumber(TagFieldFind(pSrc, "MINSIZE"), 0);
    pRec->iMaxSize = TagFieldGetNumber(TagFieldFind(pSrc, "MAXSIZE"), 0);
    pRec->iNumPart = TagFieldGetNumber(TagFieldFind(pSrc, "NUMPART"), 0);
    pRec->iSeed = TagFieldGetNumber(TagFieldFind(pSrc, "SEED"), 0);
    pRec->iWhen = TagFieldGetEpoch(TagFieldFind(pSrc, "WHEN"), 0);
    TagFieldGetString(TagFieldFind(pSrc, "AUTH"), pRec->szAuth, 0x40, "");

    for (i = 0; i < pRec->iCount; i++)
    {
        pOpp = pRec->Opponents + i;

        sprintf(Buf, "OPID%d", i);
        pOpp->iIdent = TagFieldGetNumber(TagFieldFind(pSrc, Buf), 0);
        sprintf(Buf, "OPPO%d", i);
        TagFieldGetString(TagFieldFind(pSrc, Buf), pOpp->szName, 0x10, "");
        sprintf(Buf, "ADDR%d", i);
        pOpp->iAddr = TagFieldGetAddress(TagFieldFind(pSrc, Buf), 0);
        sprintf(Buf, "LADDR%d", i);
        pOpp->iLAddr = TagFieldGetAddress(TagFieldFind(pSrc, Buf), 0);
        sprintf(Buf, "MADDR%d", i);
        TagFieldGetString(TagFieldFind(pSrc, Buf), pOpp->szMAddr, 0x40, "");
        _LobbyApiSynthesizeMachineAddr(pOpp->szMAddr, pOpp->iAddr, pOpp->iLAddr);
        sprintf(Buf, "OPPART%d", i);
        pOpp->iPart = TagFieldGetNumber(TagFieldFind(pSrc, Buf), 0);
        sprintf(Buf, "OPPARAM%d", i);
        TagFieldGetString(TagFieldFind(pSrc, Buf), pOpp->szParam, 0x24, "");
        sprintf(Buf, "OPFLAG%d", i);
        pOpp->iFlag = TagFieldGetNumber(TagFieldFind(pSrc, Buf), 0);
        sprintf(Buf, "PRES%d", i);
        pOpp->iPres = TagFieldGetNumber(TagFieldFind(pSrc, Buf), 0);
    }

    for (i = 0; i < pRec->iNumPart; i++)
    {
        sprintf(Buf, "PARTSIZE%d", i);
        pRec->Parts[i].iSize = TagFieldGetNumber(TagFieldFind(pSrc, Buf), 0);
        sprintf(Buf, "PARTPARAMS%d", i);
        TagFieldGetString(TagFieldFind(pSrc, Buf), pRec->Parts[i].szParams, 0x24, 0);
    }

    {
        char *pField = TagFieldFind(pSrc, "SESS");
        if (pField)
            TagFieldGetString(pField, pRec->szSess, 0x80, "");
        else
            sprintf(pRec->szSess, "%s-%s-%08x", pRec->Opponents[0].szName, pRec->Opponents[1].szName, pRec->iWhen);
    }
}

void LobbyApiExtractRankRecord(LobbyApiRankRecordT *pRec, const char *pSrc, const char *pName)
{
    memset(pRec, 0, sizeof(LobbyApiRankRecordT));
    pRec->iR = TagFieldGetNumber(TagFieldFind(pSrc, "R"), 0);
    pRec->iP = TagFieldGetNumber(TagFieldFind(pSrc, "P"), 0);
    pRec->iFlags = 0;
    if (TagFieldGetNumber(TagFieldFind(pSrc, "O"), 0))
        pRec->iFlags |= 0x4;
    TagFieldGetString(TagFieldFind(pSrc, "N"), pRec->szName, 0x10, "");
    TagFieldGetRaw(TagFieldFind(pSrc, "S"), pRec->szS, 0x84, "");
    if (LobbyNameCmp(pRec->szName, pName) == 0)
        pRec->iFlags |= 0x1;
}

void LobbyApiExtractRoomRecord(LobbyApiRoomRecordT *pRec, const char *pSrc)
{
    memset(pRec, 0, sizeof(LobbyApiRoomRecordT));
    pRec->iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "I"), -1);
    TagFieldGetString(TagFieldFind(pSrc, "N"), pRec->szName, 0x24, "");
    TagFieldGetString(TagFieldFind(pSrc, "DN"), pRec->szDN, 0x24, "");
    TagFieldGetString(TagFieldFind(pSrc, "H"), pRec->szHost, 0x10, "");
    TagFieldGetString(TagFieldFind(pSrc, "D"), pRec->szDesc, 0x84, "");
    pRec->iFlags = TagFieldGetFlags(TagFieldFind(pSrc, "F"), -1);
    pRec->iT = TagFieldGetNumber(TagFieldFind(pSrc, "T"), 0);
    pRec->iGT = TagFieldGetNumber(TagFieldFind(pSrc, "GT"), 0);
    pRec->iL = TagFieldGetNumber(TagFieldFind(pSrc, "L"), 0);
    pRec->iLO = TagFieldGetToken(TagFieldFind(pSrc, "LO"), 0);
    sprintf(pRec->szT, "%d", pRec->iT);
    strcpy(pRec->szPriv, (pRec->iFlags & 0x10000) ? "No" : "Yes");
    TagFieldGetStructure(TagFieldFind(pSrc, "C"), pRec->Structure, 0x24, "l*");
}

void LobbyApiExtractUserAddr(LobbyApiUserRecordT *pRec, const char *pSrc)
{
    pRec->iAddr = TagFieldGetAddress(TagFieldFind(pSrc, "A"), 0);
    pRec->iLAddr = TagFieldGetAddress(TagFieldFind(pSrc, "LA"), 0);
    TagFieldGetString(TagFieldFind(pSrc, "MA"), pRec->szMAddr, 0x40, "");
    _LobbyApiSynthesizeMachineAddr(pRec->szMAddr, pRec->iAddr, pRec->iLAddr);
}

void LobbyApiExtractUserRecord(LobbyApiUserRecordT *pRec, const char *pSrc)
{
    int i;

    memset(pRec, 0, sizeof(LobbyApiUserRecordT));
    pRec->iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "I"), 0);
    TagFieldGetString(TagFieldFind(pSrc, "N"), pRec->szName, 0x10, "");
    pRec->iFlags = TagFieldGetFlags(TagFieldFind(pSrc, "F"), 0);
    TagFieldGetString(TagFieldFind(pSrc, "P"), pRec->szP, 0x8, "");
    TagFieldGetString(TagFieldFind(pSrc, "S"), pRec->szS, 0x84, "");
    TagFieldGetString(TagFieldFind(pSrc, "X"), pRec->szX, 0x84, "");
    pRec->iG = TagFieldGetNumber(TagFieldFind(pSrc, "G"), 0);
    pRec->iAT = TagFieldGetFlags(TagFieldFind(pSrc, "AT"), 0);
    LobbyApiColor(pRec->Color, TagFieldGetNumber(TagFieldFind(pSrc, "CL"), 0));
    pRec->iLV = TagFieldGetNumber(TagFieldFind(pSrc, "LV"), 0);
    pRec->iMD = TagFieldGetNumber(TagFieldFind(pSrc, "MD"), 0);
    pRec->iR = TagFieldGetNumber(TagFieldFind(pSrc, "R"), 0);

    for (i = 0; i < 4; i++)
    {
        TagFieldGetDelim(TagFieldFind(pSrc, "US"), pRec->Sets[i], 0x24, "", i, ',');
        if (pRec->Sets[i][0] == 0)
            break;
    }

    pRec->iHW = TagFieldGetNumber(TagFieldFind(pSrc, "HW"), 0);
    pRec->iRP = TagFieldGetNumber(TagFieldFind(pSrc, "RP"), 0);
    sprintf(pRec->szRP, "%03d", pRec->iRP);
    pRec->iLO = TagFieldGetToken(TagFieldFind(pSrc, "LO"), 0);
    TagFieldGetString(TagFieldFind(pSrc, "CI"), pRec->szCI, 0x14, "");
    TagFieldGetString(TagFieldFind(pSrc, "CT"), pRec->szCT, 0x8, "");
    LobbyApiExtractUserAddr(pRec, pSrc);
}

void LobbyApiExtractUserSetMember(LobbyApiUserSetMemberT *pRec, const char *pSrc)
{
    memset(pRec, 0, sizeof(LobbyApiUserSetMemberT));
    pRec->iIdent = TagFieldGetNumber(TagFieldFind(pSrc, "I"), 0);
    TagFieldGetString(TagFieldFind(pSrc, "N"), pRec->szName, 0x10, "");
    pRec->iFlags = TagFieldGetFlags(TagFieldFind(pSrc, "F"), 0);
    pRec->iG = TagFieldGetNumber(TagFieldFind(pSrc, "G"), 0);
    TagFieldGetString(TagFieldFind(pSrc, "X"), pRec->szX, 0x84, "");
}

// expande 3 bits de color a 8 (duplicado de bits + bit alto) y fija alpha
void LobbyApiColor(unsigned char *pColor, int iValue)
{
    int v = iValue ? iValue : 0x1FF;
    unsigned r = (v >> 6) & 7;
    unsigned g = (v >> 3) & 7;
    unsigned b = v & 7;

    pColor[0] = (unsigned char)((r << 5) | (r << 2) | (r >> 1));
    pColor[1] = (unsigned char)((g << 5) | (g << 2) | (g >> 1));
    pColor[2] = (unsigned char)((b << 5) | (b << 2) | (b >> 1));
    pColor[3] = 0xFF;
}
