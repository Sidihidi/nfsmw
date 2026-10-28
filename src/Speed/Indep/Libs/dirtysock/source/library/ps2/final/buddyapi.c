// DirtySock: buddyapi.c, el nivel bajo de buddies (el transporte sobre ProtoAries).
// Escrito del asm de la A124 (libdirtysock.a(buddyapi.o): .text 0x397C48, 61
// etiquetas; la unidad solo enlaza en el prototipo, SLUS no la tiene). El
// .mdebug del NFS.ELF no trae ni una funcion de esta lib (venia precompilada),
// asi que el unico oraculo es el asm + los datos (.rodata 0x48A8E0, .data
// 0x54C770, .sdata 0x5558D0, .sbss 0x654F10).
//
// Medido del asm (r45):
// - BuddyApiRefT mide 0xF80 B (BuddyApiCreate2 reserva e inicializa 0xF80).
// - El registro de buddy mide 0x11C B (igual que en hlbudapi: nombre +0x00,
//   presencia +0x35, titulo +0x85, extra +0xAE, voip +0xF0, estado +0xF7,
//   flags +0xF8, vuelta +0x100, indice +0x104).
// - Los mensajes de cuatro letras van en el orden de bytes grande
//   ('send' == 0x73656E64) via lui+ori.
// - _BuddyApiDebug es un stub vacio (jr; nop) pero se le llama con 3
//   argumentos desde _BuddyApiTransact: la declaracion sin prototipo es la
//   unica forma (igual que _setUpOp en hlbudapi, r43).
// - En el EE los argumentos 5-8 van en $t0-$t3 (BuddyApiSend/Broadcast
//   reenvian a0-a3/t0-t1 a _BuddyApiSendMsg).
#include <string.h>
#include <stdio.h>

typedef struct BuddyApiRefT BuddyApiRefT;
typedef struct BuddyApiBuddyT BuddyApiBuddyT;
typedef struct BuddyApiMsgT BuddyApiMsgT;
typedef struct DispListT DispListT;
typedef int (*BuddyApiSortFnT)(BuddyApiBuddyT *, BuddyApiBuddyT *);
typedef void (*BuddyApiCallbackT)(BuddyApiRefT *, BuddyApiMsgT *, void *);
typedef void (*BuddyApiChangeCallbackT)(BuddyApiRefT *, BuddyApiBuddyT *, unsigned int, int, void *);

// --- dirtynet / hasher / lobbytagfield / protoaries (otras unidades, aun asmtu)
extern DispListT *DispListCreate(int iSortMode, int iSortDir, int iSortKeySize);
extern void *DispListGet(DispListT *pList, int iIndex);
extern void *DispListAdd(DispListT *pList, void *pItem);
extern void *DispListDel();
extern void DispListDirty(DispListT *pList, int iDirty);
extern void *DispListIndex(DispListT *pList, int iIndex);
extern int DispListOrder(DispListT *pList);
extern void HashStrAdd(void *pHasher, const void *pKey, void *pValue);
extern int HashStrDel(void *pHasher, const void *pKey);
extern int ds_stricmp(const char *pA, const char *pB);
extern void *HasherCreate(int iX, int iY);
extern void *HasherFlush(void *pHasher);
extern void HasherDestroy(void *pHasher);
extern int HashStrFind(void *pHasher, const void *pKey);
extern char *TagFieldFind(const char *pRecord, const char *pName);
extern int TagFieldGetFlags(const char *pField, int iDefault);
extern int TagFieldGetNumber(const char *pField, int iDefault);
extern int TagFieldGetString(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault);
extern int TagFieldGetBinary(const char *pField, void *pBuffer, int iBufferSize);
extern int TagFieldGetToken(const char *pField, int iIndex);
extern char *TagFieldDupl(char *pDest, int iDestSize, const char *pSrc);
extern void TagFieldDelete(char *pRecord, const char *pName);
extern int TagFieldSetNumber(char *pBuffer, int iBufferSize, const char *pName, int iValue);
extern int TagFieldSetString(char *pBuffer, int iBufferSize, const char *pName, const char *pValue);
extern void TagFieldSetToken(char *pBuffer, int iBufferSize, const char *pName, int iValue);
extern void *ProtoAriesCreate(int iX);
extern void ProtoAriesSecure(void *pAries, void *pX);
extern void ProtoAriesUnconnect(void *pAries);
extern int ProtoAriesTick(void *pAries);
extern void ProtoAriesDestroy(void *pAries);
extern int ProtoAriesSend(void *pAries, void *pBuf, void *pX, void *pY, int iZ);
extern void ProtoAriesUpdate(void *pAries);
extern int ProtoAriesConnect(void *pAries, const char *pHost, void *pX, int iPort);
extern int ProtoAriesPeek(void *pAries, unsigned int *puKind, unsigned int *puCode, int *piExtra);
extern void ProtoAriesRecv(void *pAries, void *pX, void *pY, void *pZ, void *pW);
extern void *ProtoNameAsync(const char *pHost, int iTimeout);
extern void *NetAlloc(int iSize);
extern void NetFree(void *pMem);
extern void *LobbyAlloc(int iSize);
extern void LobbyFree(void *pMem);

// Chamullos de aridad mixta (K&R, como _setUpOp en hlbudapi): _BuddyApiDebug se
// llama con 3 argumentos y no hace nada.
extern void _BuddyApiDebug();

// Las que define este fichero mas abajo (tandas siguientes).
extern int BuddyApiAdd(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, BuddyApiCallbackT pCallback,
                       void *pRef, int iTimeout);
extern int BuddyApiFlush(BuddyApiRefT *pApi);
extern int BuddyApiDisconnect();
extern int _BuddyApiSendMsg(BuddyApiRefT *pApi, unsigned int uType, BuddyApiMsgT *pMsg, int iKind,
                            BuddyApiCallbackT pCallback, void *pRef);
extern int _BuddyApiSetCallback();
extern void _BuddyApiTransact(BuddyApiRefT *pApi, void *pBuf, void *pExtra);
extern int _BuddyApiSendResponse(BuddyApiRefT *pApi, const char *pUser, unsigned int uType,
                                 const char *pAnswer, int iFlag, int iSeq);

// tipos de mensaje (constantes de cuatro letras, orden de bytes grande, EN
// MAYUSCULAS en el cable: 'SEND' == 0x53454E44, medido; hlbudapi usa las
// minusculas para sus propios mensajes)
#define MSGTYPE_CONN 0x636F6E6E
#define MSGTYPE_SEND 0x53454E44
#define MSGTYPE_BRDC 0x42524443
#define MSGTYPE_GSRP 0x47525350
#define MSGTYPE_DISC 0x44495343
#define MSGTYPE_ROST 0x726F7374
#define MSGTYPE_AUTH 0x41555448
#define MSGTYPE_PING 0x50494E47
#define MSGTYPE_RCVD 0x52454356
#define MSGTYPE_EPST 0x45505354
#define MSGTYPE_EPGT 0x45504754
#define MSGTYPE_RGET 0x52474554
#define MSGTYPE_RNOT 0x524E4F54
#define MSGTYPE_PGET 0x50474554
#define MSGTYPE_RADD 0x52414444
#define MSGTYPE_RADM 0x5241444D
#define MSGTYPE_RDEL 0x5244454C
#define MSGTYPE_RDEM 0x5244454D
#define MSGTYPE_PADD 0x50414444
#define MSGTYPE_PDEL 0x5044454C
#define MSGTYPE_RRSP 0x52525350
#define MSGTYPE_GRSP 0x47525350
#define MSGTYPE_GRVK 0x4752564B
#define MSGTYPE_GINV 0x47494E56
#define MSGTYPE_GNOT 0x474E4F54
#define MSGTYPE_USCH 0x55534348
#define MSGTYPE_USER 0x55534552
#define MSGTYPE_PSET 0x50534554

// el registro de buddy: 0x11C B (medido en hlbudapi r43/r44, verificado aqui
// por BuddyApiBuddyInvite: lw/or/sw en +0xF8 con 0x80000)
struct BuddyApiBuddyT
{
    char Name[0x21];            // +0x00
    unsigned char Pad21[0x35 - 0x21];
    char Presence[0x2A];        // +0x35
    unsigned char Pad5F[0x85 - 0x5F];
    char Title[0x29];           // +0x85
    char PresenceExtra[0xF0 - 0xAE]; // +0xAE
    int iVoipState;             // +0xF0
    unsigned char F4, F5, F6;   // +0xF4 (codigo de producto partido)
    signed char eState;         // +0xF7
    int uFlags;                // +0xF8
    int iSeq;                   // +0xFC (la serie de BuddyApiAdd)
    void *pHLBud;               // +0x100
    int iIndex;                 // +0x104
    char GameServer[0x11];      // +0x108 (HOST/GSTR de GNOT)
    unsigned char Pad119[0x11C - 0x119];
};

// el mensaje de buddy
struct BuddyApiMsgT
{
    unsigned int uType;         // +0x00
    unsigned int uKind;         // +0x04
    int uCode;                  // +0x08 (con signo: -4 se materializa addiu)
    unsigned int uTimeStamp;    // +0x0C
    unsigned int uFlags;        // +0x10
    char *pData;                // +0x14
};

// lo que cuelga de +0x08: BuddyApiDestroy le llama a +0x0C y lo pone a NULL
typedef struct BuddyApiX08T
{
    unsigned char Pad0[0x08];     // +0x00
    int (*pPoll)(void *);         // +0x08 (el sondeo de ProtoName en Update)
    void (*pDestroy)();           // +0x0C (K&R: Destroy le pasa 1, Disconnect 3)
} BuddyApiX08T;

// un hueco de callback de _BuddyApiSetCallback: 0x14 B (tipo +0x00, serie
// +0x04, callback +0x0C, ref +0x10; base +0x910, 4 entradas por _Buddy_CBType)
typedef struct BuddyApiCbSlotT
{
    unsigned int uType;         // +0x00 (de _Buddy_CBType[iOp])
    int iSeq;                   // +0x04 (serie de pApi->iCbSeq)
    int iDeadline;              // +0x08 (tick + 1000*iTimeout, o 0)
    void *pRef;                 // +0x0C (el DATO: se pasa como 3.er arg)
    BuddyApiCallbackT pCallback; // +0x10 (la FUNCION: la que se llama)
} BuddyApiCbSlotT;

// un usuario buscado: 0x27 B (el byte +0x00 dice si esta en uso;
// BuddyApiUserFound lo indexa con mult MMI por no ser potencia de dos)
typedef struct BuddyApiSearchedT
{
    char Data[0x27];            // +0x00
} BuddyApiSearchedT;

// una entrada de productos: id, codigo y cadena (0x0C B)
typedef struct BuddyApiProductT
{
    int iId;                // +0x00
    short sCode;            // +0x04
    short sPad06;           // +0x06
    char *pStr;             // +0x08
} BuddyApiProductT;

// la tabla de _BuddyApiSetCallback: en el orden de _Buddy_CBType
static unsigned int _Buddy_CBType[4] =
{
    0x61646475, // 'addu'
    0x64656C75, // 'delu'
    0x73656E64, // 'send'
    0x75736572, // 'user'
};

// los estados de _BuddyApiStatus (el indice 7 no tiene cadena: retorna antes)
static const char *_BuddyApi_Status[7] =
{
    "DISC",
    "CHAT",
    "AWAY",
    "XA",
    "DND",
    "PASS",
    "GAME",
};

// la tabla de productos (la rellena otro; a cero es el fin)
static BuddyApiProductT _BuddyApi_Products[1] =
{
    { 0, 0, 0, NULL },
};

// las respuestas por codigo (D_0064B7A0 en el original)
static const char *_BuddyApi_Answer[6] =
{
    "Y",
    "N",
    "",
    "B",
    "R",
    "z",
};

struct BuddyApiRefT
{
    void *(*pMemAlloc)(int);                             // +0x00
    void (*pMemFree)(void *);                            // +0x04
    BuddyApiX08T *pX08;                                  // +0x08
    void *pAries;                                        // +0x0C
    int iStatus;                                         // +0x10
    int iX14;                                            // +0x14 (Reconnect guarda tick+1000*x)
    int iX18;                                            // +0x18 (puerto que Reconnect pasa a Connect)
    int iCbSeq;                                          // +0x1C (empieza en 100)
    int iX20;                                            // +0x20
    char X24[0x80];                                      // +0x24 (servidor para Connect; llega a +0xA4)
    int iXA4;                                            // +0xA4 (Reconnect lo parte por 2 para Connect)
    char XA8[0x80];                                      // +0xA8 (clave: B4-B7 van al sprintf de la semilla)
    char Resource[0x1A8 - 0x128];                        // +0x128 (lo devuelve BuddyApiResource)
    int iX1A8;                                           // +0x1A8 (si 0, Disconnect anula el callback)
    int iX1AC;                                           // +0x1AC (semilla que agita Reconnect)
    int iX1B0;                                           // +0x1B0
    unsigned char Pad1B4[0x1D4 - 0x1B4];
    char Presence[0x100];                                // +0x1D4
    char Language[0x100];                                // +0x2D4
    char X3D4[0x80];                                     // +0x3D4 (EXTR de PresSend)
    char X454[0x80];                                     // +0x454 (RSRC/SHOW de PresSend)
    int iX4D4;                                           // +0x4D4 (ATTR="J" si != 0)
    int iNoText;                                         // +0x4D8
    int iExFlags;                                        // +0x4DC
    int iX4E0;                                           // +0x4E0 (deadline de presencia)
    int iX4E4;                                           // +0x4E4 (estado enviado)
    char Domain[0x40];                                   // +0x4E8 (BuddyApiPresSame lo comprueba/duplica)
    int iX528;                                           // +0x528 (BuddyApiConfig guarda el contador)
    short X52C[4];                                       // +0x52C (BuddyApiConfig guarda la clave)
    char Product[0x40];                                  // +0x534
    char Version[0x40];                                  // +0x574
    char ConnBuf[0x100];                                 // +0x5B4 (el registro de Connect)
    char X6B4[0x10];                                     // +0x6B4 (valor de "LRSC")
    char X6C4[0x70];                                     // +0x6C4 (ADDR de EPGT)
    unsigned char Pad734[0x8C4 - 0x734];
    int iX8C4;                                           // +0x8C4 (Create2 le pone el bit 0x100)
    void *pHasher;                                       // +0x8C8
    DispListT *pRosterList;                              // +0x8CC
    void *pRecvRef;                                      // +0x8D0
    BuddyApiCallbackT pRecvCallback;                     // +0x8D4
    void *pFlushRef;                                     // +0x8D8
    void (*pFlushCallback)(BuddyApiRefT *, void *, void *); // +0x8DC
    int iX8E0;                                           // +0x8E0 (quedan por llegar del roster)
    void *pStatusRef;                                    // +0x8E4
    void (*pStatusCallback)(BuddyApiRefT *, void *, void *); // +0x8E8
    BuddyApiCallbackT pDelCallback;                      // +0x8EC
    void *pDelRef;                                       // +0x8F0
    BuddyApiChangeCallbackT pChangeCallback;             // +0x8F4 (5 args: 'pget'/'ginv')
    void *pChangeRef;                                    // +0x8F8
    char X8FC[0x14];                                     // +0x8FC (usuario de invitacion pendiente)
    BuddyApiCbSlotT CbSlots[9];                          // +0x910 (Update recorre 9)
    void *pDebugRef;                                     // +0x9C4
    void *pDebugFn;                                      // +0x9C8
    int iX9CC;                                           // +0x9CC (busquedas vivas)
    BuddyApiSearchedT Searched[21];                      // +0x9D0 (0x333 B, FindUsers los limpia)
    unsigned char PadD03;                                // +0xD03
    int iRosterSeq;                                      // +0xD04 (Roster/Update)
    char GameTitle[0x64];                                // +0xD08 (TITL de AUTH/GRVK)
    char X_D6C[0x200];
    unsigned char PadF6C[0xF80 - 0xF6C];
};

// el stub de depuracion: existe pero no hace nada (el original es jr; nop)
void _BuddyApiDebug()
{
}

// los flags del campo ATTR del mensaje (el mensaje va por valor: 0x18 B que
// GCC copia con ldl/ldr; el bit 0x100000 se mapea a 0x1000000)
int _BuddyApiFlagsFromAttr(void *pApi, BuddyApiMsgT Msg)
{
    int iFlags = TagFieldGetFlags(TagFieldFind(Msg.pData, "ATTR"), 0);
    int iResult = iFlags & 0x80000;

    iResult |= iFlags & 0x40000;
    iResult |= ((iFlags & 0x100000) != 0) ? 0x1000000 : 0;
    return iResult;
}

// arma el envio: el BODY ha de ser corto y el SUBJ vacio (-3 si no); registra
// el callback segun el tipo ('SEND'->2, 'BRDC'->4), lo manda y devuelve 0
int _BuddyApiSendMsg(BuddyApiRefT *pApi, unsigned int uType, BuddyApiMsgT *pMsg, int iKind,
                     BuddyApiCallbackT pCallback, void *pRef)
{
    if (TagFieldGetString(TagFieldFind((const char *)pMsg, "BODY"), NULL, 0, "") >= 0x100)
        return -3;
    if (TagFieldGetString(TagFieldFind((const char *)pMsg, "SUBJ"), NULL, 0, "") > 0)
        return -3;
    if (uType == MSGTYPE_SEND)
        _BuddyApiSetCallback(pApi, 2, iKind, pCallback, pRef);
    if (uType == MSGTYPE_BRDC)
        _BuddyApiSetCallback(pApi, 4, iKind, pCallback, pRef);
    _BuddyApiTransact(pApi, (void *)uType, pMsg);
    return 0;
}

// arma una respuesta ('LRSC' + "RSRC" si es 'GSRP' + "ID" + "USER" + "ANSW" si
// hay + "PRES"="Y" si iFlag==1), la manda y devuelve 0
int _BuddyApiSendResponse(BuddyApiRefT *pApi, const char *pUser, unsigned int uType,
                          const char *pAnswer, int iFlag, int iSeq)
{
    char Buffer[0xFA];

    Buffer[0] = 0;
    TagFieldSetString(Buffer, 0xFA, "LRSC", pApi->X6B4);
    if (uType == MSGTYPE_GSRP)
        TagFieldSetString(Buffer, 0xFA, "RSRC", pApi->Resource);
    TagFieldSetNumber(Buffer, 0xFA, "ID", iSeq);
    TagFieldSetString(Buffer, 0xFA, "USER", pUser);
    if (strlen(pAnswer) != 0)
        TagFieldSetString(Buffer, 0xFA, "ANSW", pAnswer);
    if (iFlag == 1)
        TagFieldSetString(Buffer, 0xFA, "PRES", "Y");
    _BuddyApiTransact(pApi, (void *)uType, Buffer);
    return 0;
}

// reserva un hueco de callback (tipo de _Buddy_CBType, serie, deadline,
// callback, ref) y devuelve la serie. K&R sin prototipo: los llamantes le
// pasan int o puntero en el 3.º y 5.º (iKind/pRef, pRef/iTimeout).
int _BuddyApiSetCallback(pApi, iOp, pRef, pCallback, iTimeout)
BuddyApiRefT *pApi;
int iOp;
void *pRef;
BuddyApiCallbackT pCallback;
int iTimeout;
{
    int iDeadline = 0;

    if (iTimeout > 0)
        iDeadline = ProtoAriesTick(pApi->pAries) + 1000 * iTimeout;
    pApi->CbSlots[iOp].uType = _Buddy_CBType[iOp];
    pApi->CbSlots[iOp].iSeq = ++pApi->iCbSeq;
    pApi->CbSlots[iOp].pRef = pCallback;
    pApi->CbSlots[iOp].pCallback = pRef;
    pApi->CbSlots[iOp].iDeadline = iDeadline;
    return pApi->CbSlots[iOp].iSeq;
}

// atiende un ack: si el hueco trae funcion y la ID casa, la llama (con el
// dato del hueco) y lo desarma. El mensaje va por valor (copia de 0x18).
void _processAck(BuddyApiRefT *pApi, BuddyApiMsgT Msg, int iSlot)
{
    if (pApi->CbSlots[iSlot].pCallback == NULL)
        return;
    if (TagFieldGetNumber(TagFieldFind(Msg.pData, "ID"), 0) != pApi->CbSlots[iSlot].iSeq)
        return;
    pApi->CbSlots[iSlot].pCallback(pApi, &Msg, pApi->CbSlots[iSlot].pRef);
    pApi->CbSlots[iSlot].pCallback = NULL;
    pApi->CbSlots[iSlot].iDeadline = 0;
}

// quita los bits iMask del buddy; si aun le quedan de los 0x1C0204 se queda,
// si no lo borra (hasher+lista, aviso y free). Devuelve los bits quitados.
int _BuddyApiDeleteIfNeeded(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, int iMask)
{
    int iRemoved = 0;
    int iIndex;

    iIndex = HashStrFind(pApi->pHasher, pBuddy);
    if (iIndex != 0)
    {
        BuddyApiBuddyT *pRec = (BuddyApiBuddyT *)DispListGet(pApi->pRosterList, iIndex);

        iRemoved = pRec->uFlags & iMask;
        pRec->uFlags ^= iRemoved;
        if ((pRec->uFlags & 0x1C0204) != 0)
            return iRemoved;
        HashStrDel(pApi->pHasher, pBuddy);
        pRec = (BuddyApiBuddyT *)DispListDel(pApi->pRosterList, iIndex);
        if (pApi->pDelCallback != NULL)
            pApi->pDelCallback(pApi, (BuddyApiMsgT *)pRec, pApi->pDelRef);
        pApi->pMemFree(pRec);
    }
    return iRemoved;
}

// busca el buddy en la lista; si no esta lo clona (memcpy de 0x11C, que GCC
// expande con dispatch de alineado), lo alta en lista+hasher y le suma los
// flags. Devuelve el de la lista (o NULL sin roster).
BuddyApiBuddyT *_BuddyApiAddOrSetFlags(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy)
{
    BuddyApiBuddyT *pFound = NULL;

    if (pApi->pRosterList != NULL)
    {
        int iIndex = HashStrFind(pApi->pHasher, pBuddy);

        if (iIndex == 0)
        {
            pFound = (BuddyApiBuddyT *)pApi->pMemAlloc(0x11C);
            memcpy(pFound, pBuddy, 0x11C);
            HashStrAdd(pApi->pHasher, pFound, DispListAdd(pApi->pRosterList, pFound));
        }
        else
            pFound = (BuddyApiBuddyT *)DispListGet(pApi->pRosterList, iIndex);
        pFound->uFlags |= pBuddy->uFlags;
    }
    return pFound;
}

// parte un recurso "cabeza/cola": la cabeza a pHead y la cola a pTail (si no
// hay '/', la cola queda vacia). Los tamaños van ANTES que los buffers
// (medido en el llamante HandleSearchedUserMsg: a1=4, a2=buf, a3=0x10,
// t0=buf) y el camino NULL va primero (bnel al outline).
void _BuddyApiParseResource(const char *pSrc, int iHeadSize, char *pHead, int iTailSize, char *pTail)
{
    char Buffer[0x100];
    char *pSlash;

    strncpy(Buffer, pSrc, 0xFE);
    pSlash = strchr(Buffer, '/');
    if (pSlash == NULL)
    {
        strncpy(pHead, Buffer, iHeadSize);
        *pTail = 0;
    }
    else
    {
        *pSlash = 0;
        strncpy(pHead, Buffer, iHeadSize);
        strncpy(pTail, pSlash, iTailSize);
    }
}

// busca el producto iId con el codigo en pCodes: devuelve lo duplicado o NULL
static int _BuddyApiProd(void *pApi, int iId, short *pCodes, int iCount, char *pDst, int iSize)
{
    int i;
    int j;

    if (_BuddyApi_Products[0].iId == 0)
        return 0;
    for (j = 0; _BuddyApi_Products[j].iId != 0; j++)
    {
        if (_BuddyApi_Products[j].iId == iId && iCount > 0)
        {
            for (i = 0; i < iCount; i++)
            {
                if (pCodes[i] == _BuddyApi_Products[j].sCode)
                {
                    TagFieldDupl(pDst, iSize, _BuddyApi_Products[j].pStr);
                    return _BuddyApi_Products[j].sCode;
                }
            }
        }
    }
    return 0;
}

// valida un nombre: -3 si es NULL/vacio o trae '/' o '@', 0 si el API trae el
// flag +0x20 o si no trae '.', -3 si trae '.'
int _BuddyApiValidateName(BuddyApiRefT *pApi, const char *pName)
{
    if ((pName != NULL) && (pName[0] != 0))
    {
        if (strchr(pName, '/') != NULL)
            return -3;
        if (strchr(pName, '@') != NULL)
            return -3;
        if (pApi->iX20 != 0)
            return 0;
        if (strchr(pName, '.') == NULL)
            return 0;
    }
    return -3;
}

// corta el nombre por '@', '/' y '.'
void _BuddyApiTruncUsername(char *pName)
{
    char *pCut;

    pCut = strchr(pName, '@');
    if (pCut != NULL)
        *pCut = 0;
    pCut = strchr(pName, '/');
    if (pCut != NULL)
        *pCut = 0;
    pCut = strchr(pName, '.');
    if (pCut != NULL)
        *pCut = 0;
}

// envia por ProtoAries y lo chiva por el debug (void: el original cae al
// epilogo con lo que dejo ProtoAriesSend en v0; los llamantes lo ignoran)
void _BuddyApiTransact(BuddyApiRefT *pApi, void *pBuf, void *pExtra)
{
    BuddyApiMsgT Msg;

    ProtoAriesSend(pApi->pAries, pBuf, NULL, pExtra, -1);
    if (pApi->pDebugFn == NULL)
        return;
    Msg.uKind = (unsigned int)pBuf;
    Msg.pData = (char *)pExtra;
    Msg.uCode = 0;
    Msg.uType = 0;
    _BuddyApiDebug(pApi, "send: ", &Msg);
}

// crea el hasher y la lista si aun no estan
void _createBuddyDispList(BuddyApiRefT *pApi)
{
    if (pApi->pHasher == NULL)
        pApi->pHasher = HasherCreate(1000, 2000);
    if (pApi->pRosterList == NULL)
        pApi->pRosterList = DispListCreate(100, 100, 0);
}

// el estado: avisa al callback con un mensaje 'conn' (el 7 no avisa)
void _BuddyApiStatus(BuddyApiRefT *pApi, int iStatus)
{
    BuddyApiMsgT Msg;

    if (pApi->iStatus == iStatus)
        return;
    pApi->iStatus = iStatus;
    if (pApi->pStatusCallback == NULL)
        return;
    if (iStatus == 7)
        return;
    Msg.uKind = (unsigned int)iStatus;
    Msg.uCode = 0;
    Msg.uType = MSGTYPE_CONN;
    Msg.pData = NULL;
    pApi->pStatusCallback(pApi, &Msg, pApi->pStatusRef);
}

BuddyApiRefT *BuddyApiCreate2(void *(*pMemAlloc)(int), void (*pMemFree)(void *), const char *pProduct,
                              const char *pVersion, BuddyApiSortFnT pSortFn)
{
    BuddyApiRefT *pApi;

    if (pMemAlloc == NULL || pMemFree == NULL)
    {
        pMemAlloc = LobbyAlloc;
        pMemFree = LobbyFree;
    }
    pApi = (BuddyApiRefT *)NetAlloc(0xF80);
    if (pApi != NULL)
    {
        memset(pApi, 0, 0xF80);
        pApi->pMemAlloc = pMemAlloc;
        pApi->pMemFree = pMemFree;
        TagFieldDupl(pApi->Product, 0x40, pProduct);
        TagFieldDupl(pApi->Version, 0x40, pVersion);
        pApi->pAries = ProtoAriesCreate(0x8000);
        if (pApi->pAries != NULL)
            ProtoAriesSecure(pApi->pAries, NULL);
        pApi->iCbSeq = 100;
        {
            int iFlags = pApi->iX8C4;

            pApi->iX8C4 = iFlags | 0x100;
        }
    }
    return pApi;
}

void BuddyApiDestroy(BuddyApiRefT *pApi)
{
    if (pApi->pX08 != NULL)
    {
        pApi->pX08->pDestroy(pApi->pX08);
        pApi->pX08 = NULL;
    }
    BuddyApiFlush(pApi);
    BuddyApiDisconnect(pApi);
    BuddyApiFlush(pApi);
    ProtoAriesDestroy(pApi->pAries);
    NetFree(pApi);
}

// K&R con reenvio: el 2.º llega en a1 del llamante (basura si llama con 1) y
// pasa tal cual al destructor de +0x08 (2 args: el 3.º que ve el asm es el
// hi de "" del Transact, metido por el scheduler en el delay del beqz).
int BuddyApiDisconnect(pApi, pX)
BuddyApiRefT *pApi;
void *pX;
{
    if (pApi->pX08 != NULL)
    {
        pApi->pX08->pDestroy(pApi->pX08, pX);
        pApi->pX08 = NULL;
    }
    _BuddyApiTransact(pApi, (void *)MSGTYPE_DISC, "");
    ProtoAriesUnconnect(pApi->pAries);
    if (pApi->iStatus == 5)
        _BuddyApiStatus(pApi, 6);
    _BuddyApiStatus(pApi, 0);
    BuddyApiFlush(pApi);
    if (pApi->iX1A8 == NULL)
        pApi->pStatusCallback = NULL;
    return 0;
}

// devuelve el buffer de recurso (+0x128); los argumentos sobran (el original
// los ignora: jr con addiu en el delay)
char *BuddyApiResource(BuddyApiRefT *pApi, int iResource, char *pBuffer)
{
    return pApi->Resource;
}

void BuddyApiDebug(BuddyApiRefT *pApi, void *pDebugRef, void *pDebugFn)
{
    pApi->pDebugRef = pDebugRef;
    pApi->pDebugFn = pDebugFn;
}

void BuddyApiRecv(BuddyApiRefT *pApi, BuddyApiCallbackT pCallback, void *pRef)
{
    pApi->pRecvCallback = pCallback;
    pApi->pRecvRef = pRef;
}

// sin retorno util (el original no toca v0: cae al epilogo)
int BuddyApiRegisterBuddyChangeCallback(BuddyApiRefT *pApi, BuddyApiCallbackT pCallback, void *pRef)
{
    pApi->pChangeCallback = pCallback;
    pApi->pChangeRef = pRef;
}

int BuddyApiRegisterBuddyDelCallback(BuddyApiRefT *pApi, BuddyApiCallbackT pCallback, void *pRef)
{
    pApi->pDelCallback = pCallback;
    pApi->pDelRef = pRef;
}

void BuddyApiPresInit(BuddyApiRefT *pApi)
{
    pApi->Presence[0] = 0;
    pApi->Language[0] = 0;
    pApi->iExFlags = 0;
}

void BuddyApiPresNoText(BuddyApiRefT *pApi)
{
    pApi->iNoText = 1;
}

// guarda los flags; no devuelve nada util (el original no toca v0)
void BuddyApiPresExFlags(BuddyApiRefT *pApi, int iFlags)
{
    pApi->iExFlags = iFlags;
}

// si hay dominio duplica la presencia, si no el propio byte (que es 0)
int BuddyApiPresSame(BuddyApiRefT *pApi, const char *pPresence)
{
    int iSame = pApi->Domain[0];

    if (iSame != 0)
        iSame = (int)TagFieldDupl(pApi->Presence, 0x100, pPresence);
    return iSame;
}

void BuddyApiPresDiff(BuddyApiRefT *pApi, const char *pDomain, const char *pLanguage)
{
    if (pLanguage[0] == 0)
        return;
    if (pLanguage[1] == 0)
        return;
    if (pLanguage[2] != 0)
        return;
    if (pDomain[0] == 0)
        return;
    TagFieldSetString(pApi->Language, 0x100, pLanguage, pDomain);
}

DispListT *BuddyApiRosterList(BuddyApiRefT *pApi)
{
    return pApi->pRosterList;
}

BuddyApiBuddyT *BuddyApiFind(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy)
{
    BuddyApiBuddyT *pFound = NULL;

    if (pApi->pHasher != NULL)
    {
        if (pBuddy != NULL)
        {
            int iIndex = HashStrFind(pApi->pHasher, pBuddy);

            if (iIndex != 0)
                pFound = (BuddyApiBuddyT *)DispListGet(pApi->pRosterList, iIndex);
        }
    }
    return pFound;
}

// vacia el roster (hasher+lista con avisos), avisa con 'rost' si toca y lo
// desmonta todo. Sin retorno util (cae con lo ultimo en v0).
int BuddyApiFlush(BuddyApiRefT *pApi)
{
    if (pApi->pRosterList != NULL)
    {
        void *pKey;

        while ((pKey = HasherFlush(pApi->pHasher)) != NULL)
        {
            BuddyApiBuddyT *pGone = (BuddyApiBuddyT *)DispListDel(pApi->pRosterList, pKey);

            if (pApi->pDelCallback != NULL)
                pApi->pDelCallback(pApi, (BuddyApiMsgT *)pGone, pApi->pDelRef);
            pApi->pMemFree(pGone);
        }
        if (pApi->pFlushCallback != NULL)
        {
            BuddyApiMsgT Msg;

            Msg.uType = MSGTYPE_ROST;
            Msg.pData = (char *)pApi->pRosterList;
            Msg.uKind = 0;
            Msg.uCode = 0;
            pApi->pFlushCallback(pApi, &Msg, pApi->pFlushRef);
        }
        HasherDestroy(pApi->pHasher);
        pApi->pHasher = NULL;
        DispListDestroy(pApi->pRosterList);
        pApi->pRosterList = NULL;
        pApi->pFlushRef = NULL;
        pApi->pFlushCallback = NULL;
    }
}

// el buscado iIndex (0-19) si esta en uso, si no NULL
void *BuddyApiUserFound(BuddyApiRefT *pApi, int iIndex)
{
    void *pFound = NULL;

    if (iIndex < 20)
    {
        if (pApi->Searched[iIndex].Data[0] != 0)
            pFound = &pApi->Searched[iIndex];
    }
    return pFound;
}

// invita marcando el flag 0x80000 y reenviando los registros de entrada
int BuddyApiBuddyInvite(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, BuddyApiCallbackT pCallback,
                        void *pRef, int iTimeout)
{
    pBuddy->uFlags |= 0x80000;
    return BuddyApiAdd(pApi, pBuddy, pCallback, pRef, iTimeout);
}

int BuddyApiSend(BuddyApiRefT *pApi, BuddyApiMsgT *pMsg, int iKind, BuddyApiCallbackT pCallback,
                 void *pRef, int iTimeout)
{
    return _BuddyApiSendMsg(pApi, MSGTYPE_SEND, pMsg, iKind, pCallback, pRef);
}

int BuddyApiBroadcast(BuddyApiRefT *pApi, BuddyApiMsgT *pMsg, int iKind, BuddyApiCallbackT pCallback,
                      void *pRef, int iTimeout)
{
    return _BuddyApiSendMsg(pApi, MSGTYPE_BRDC, pMsg, iKind, pCallback, pRef);
}

// vacio a proposito (el original es jr; nop)
void BuddyApiRefreshTitle(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy)
{
}

// vacios a proposito (el original es jr; nop en los dos)
void BuddyApiSuspendXDK(BuddyApiRefT *pApi)
{
}

void BuddyApiResumeXDK(BuddyApiRefT *pApi)
{
}

// saca la semilla aleatoria: si hay clave la busca con "$c1c2c3c4"
// (TagFieldGetBinary devuelve los 2 primeros bytes), si no cae al tick.
// Sin puntero intermedio: la direccion de Tmp se calcula tras los tests.
int _BuddyApiExtractRandomSeed(BuddyApiRefT *pApi)
{
    char Rec[0x11];
    char Tmp[0x20];

    memset(Rec, 0, 0x11);
    if (pApi->XA8 != NULL && pApi->XA8[0] != 0 && pApi->XA8[0x0F] != 0) {
        sprintf(Tmp, "$%c%c%c%c", pApi->XA8[0x0C], pApi->XA8[0x0D], pApi->XA8[0x0E], pApi->XA8[0x0F]);
        TagFieldGetBinary(Tmp, Rec, 0x11);
        return *(unsigned short *)Rec;
    }
    return ProtoAriesTick(pApi->pAries) & 0xFFFF;
}

// guarda el dominio y convierte la clave (8 minusculas -> 4 shorts en
// +0x52C); iValue no se usa (el llamante pasa -1 o el producto). Firma de
// hlbudapi.c (los llamantes pasan (dominio, x, clave, 1, 1)).
int BuddyApiConfig(BuddyApiRefT *pApi, const char *pSetting, int iValue, const char *pValue,
                   int iValue2, int iValue3)
{
    char *pKey = (char *)pValue;
    int i;

    TagFieldDupl(pApi->Domain, 0x40, pSetting);
    pApi->iX1B0 = 0;
    i = 0;
    do
    {
        if (pKey[0] < 'a')
            break;
        if (pKey[1] < 'a')
            break;
        pApi->X52C[i] = 0;
        pApi->X52C[i] = (short)(pKey[0] << 8);
        pApi->X52C[i] |= pKey[1];
        i++;
        pKey += 2;
    } while (i < 4);
    pApi->iX20 = iValue2;
    pApi->iX1A8 = iValue3;
    pApi->iX528 = i;
    return 0;
}

// recibe un mensaje de chat: parte el USER en usuario/dominio/recurso, toma
// el TIME y avisa con 'chat' (el registro retocado queda apuntado por el msg)
void _BuddyApiRecv(BuddyApiRefT *pApi, BuddyApiMsgT *pMsg)
{
    char Rec[0x800];
    char Name[0x100];
    char User[0x100];
    char *pAt;
    char *pCut;

    TagFieldDupl(Rec, 0x800, pMsg->pData);
    TagFieldGetString(TagFieldFind(Rec, "USER"), Name, 0x100, "");
    TagFieldDelete(Rec, "USER");
    pAt = strchr(Name, '@');
    if (pAt != NULL)
    {
        *pAt = 0;
        strcpy(User, Name);
        pCut = strchr(User, '.');
        if (pCut != NULL)
            *pCut = 0;
        TagFieldSetString(Rec, 0x800, "USER", User);
        pCut = strchr(pAt + 1, '/');
        if (pCut != NULL)
            *pCut = 0;
        TagFieldSetString(Rec, 0x800, "DOMN", pAt + 1);
        if (pCut != NULL)
            TagFieldSetString(Rec, 0x800, "RSRC", pCut + 1);
    }
    else
        TagFieldSetString(Rec, 0x800, "USER", Name);
    pMsg->pData = Rec;
    pMsg->uTimeStamp = TagFieldGetNumber(TagFieldFind(Rec, "TIME"), 0);
    pMsg->uType = 0x63686174;
    pApi->pRecvCallback(pApi, pMsg, pApi->pRecvRef);
}

// aplica una presencia al registro: estado (SHOW), titulo (TITL), producto
// (PROD/EXTR), presencia extendida (STAT/EX) y codigo de producto en F4/F5
int _BuddyApiPresRecv(BuddyApiRefT *pApi, BuddyApiMsgT *pMsg, BuddyApiBuddyT *pBuddy)
{
    char Key[4];
    char Presence[0x100];
    char Show[0x40];
    short sCode;
    int iChanged;
    int i;
    int iToken;
    char *pProd;
    char *pField;

    pBuddy->eState = 0;
    pBuddy->F4 = 0;
    pBuddy->Presence[0] = 0;
    pBuddy->Pad21[0] = 0;
    TagFieldGetString(TagFieldFind(pMsg->pData, "SHOW"), Show, 0x40, "");
    for (i = 0; i < 7; i++)
    {
        if (strcmp(_BuddyApi_Status[i], Show) == 0)
        {
            pBuddy->eState = i;
            break;
        }
    }
    iChanged = TagFieldGetNumber(TagFieldFind(pMsg->pData, "CHNG"), 0) == 0;
    TagFieldGetString(TagFieldFind(pMsg->pData, "TITL"), pBuddy->Title, 0x29, "");
    pProd = TagFieldFind(pMsg->pData, "PROD");
    if (pProd != NULL)
    {
        pBuddy->uFlags |= 0x2000;
        TagFieldGetString(TagFieldFind(pMsg->pData, "EXTR"), pBuddy->PresenceExtra, 0x40, "");
    }
    else
    {
        pBuddy->uFlags &= 0xFFFFDFFF;
        pBuddy->PresenceExtra[0] = 0;
    }
    TagFieldGetString(TagFieldFind(pMsg->pData, "STAT"), Presence, 0x100, "");
    pBuddy->iVoipState = TagFieldGetNumber(TagFieldFind(Presence, "EX"), 0);
    pBuddy->uFlags &= 0xFFFFEFFF;
    if ((TagFieldGetFlags(TagFieldFind(pMsg->pData, "ATTR"), 0) & 0x400) != 0)
        pBuddy->uFlags |= 0x1000;
    TagFieldGetString(pProd, pBuddy->Presence, 0x50, "");
    if (pBuddy->Presence[0] != 0)
    {
        _BuddyApiProd(pApi, pApi->iX1B0, pApi->X52C, 1, pBuddy->Pad21, 0x14);
        pBuddy->F4 = pApi->X52C[0] >> 8;
        pBuddy->F5 = pApi->X52C[0];
        pBuddy->F6 = 0;
        TagFieldGetString(pProd, pBuddy->Presence, 0x50, "");
        return iChanged;
    }
    iToken = TagFieldGetToken(TagFieldFind(Presence, "P"), 0);
    pField = NULL;
    for (i = 0; i < pApi->iX528; i++)
    {
        Key[0] = pApi->X52C[i] >> 8;
        Key[1] = pApi->X52C[i];
        Key[2] = 0;
        pField = TagFieldFind(Presence, Key);
        if (pField != NULL)
            break;
    }
    if (pField != NULL)
    {
        sCode = (Key[0] << 8) | Key[1];
        TagFieldGetString(pField, pBuddy->Presence, 0x50, "");
        pBuddy->F4 = Key[0];
        pBuddy->F5 = Key[1];
        pBuddy->F6 = 0;
        _BuddyApiProd(pApi, iToken, &sCode, 1, pBuddy->Pad21, 0x14);
    }
    if (pBuddy->F4 == 0)
    {
        sCode = (short)_BuddyApiProd(pApi, iToken, pApi->X52C, pApi->iX528, pBuddy->Pad21, 0x14);
        pBuddy->F6 = 0;
        pBuddy->F4 = sCode >> 8;
        pBuddy->F5 = sCode;
    }
    return iChanged;
}

// arma y envia la presencia propia ('PSET'); guarda lo construido en +0xD6C
void _BuddyApiPresSend(BuddyApiRefT *pApi)
{
    char Rec[0x200];
    char Rec2[0x200];
    char Tmp[0x100];
    char Pres[0x100];

    TagFieldSetNumber(pApi->Language, 0x100, "EX", pApi->iExFlags);
    if (pApi->iX1B0 != 0)
        TagFieldSetToken(pApi->Language, 0x100, "P", pApi->iX1B0);
    TagFieldDupl(Rec, 0x200, pApi->X454);
    if (pApi->Language[0] != 0)
        TagFieldSetString(Rec, 0x200, "STAT", pApi->Language);
    strcpy(Pres, " ");
    if (pApi->Presence[0] != 0)
        strncpy(Pres, pApi->Presence, 0x100);
    TagFieldSetString(Rec, 0x200, "PROD", Pres);
    if (pApi->X3D4[0] != 0)
        TagFieldSetString(Rec, 0x200, "EXTR", pApi->X3D4);
    if (pApi->iNoText == 1)
        TagFieldSetString(Rec, 0x200, "ATTR", "D");
    if (pApi->iX4D4 != 0)
        TagFieldSetString(Rec, 0x200, "ATTR", "J");
    TagFieldDupl(Rec2, 0x200, Rec);
    TagFieldGetString(TagFieldFind(Rec, "STAT"), Tmp, 0x100, "");
    TagFieldDelete(Tmp, "EX");
    TagFieldDelete(Rec2, "STAT");
    TagFieldSetString(Rec2, 0x200, "STAT", Tmp);
    if (strcmp(Rec2, pApi->X_D6C) == 0)
        TagFieldSetNumber(Rec, 0x200, "NXBL", 1);
    TagFieldDupl(pApi->X_D6C, 0x200, Rec2);
    _BuddyApiTransact(pApi, (void *)MSGTYPE_PSET, Rec);
    pApi->iX4E0 = 0;
}

// una entrada de roster: crea el registro si no esta (nombre valido) y le
// suma los flags del tipo de lista y del ATTR
int _BuddyApiHandleROSTMsg(BuddyApiRefT *pApi, BuddyApiMsgT Msg)
{
    char Name[0x20];
    BuddyApiBuddyT *pBuddy;
    int iFlag;
    int iId;
    int iIndex;

    iFlag = 4;
    _createBuddyDispList(pApi);
    iId = TagFieldGetNumber(TagFieldFind(Msg.pData, "ID"), 0);
    if (iId != 1)
        iFlag = 0;
    if (iId == 2)
        iFlag = 0x200;
    TagFieldGetString(TagFieldFind(Msg.pData, "USER"), Name, 0x20, "");
    _BuddyApiTruncUsername(Name);
    if (strlen(Name) == 0)
        return iId;
    if (pApi->iX20 == 0)
    {
        if (strchr(Name, '.') != NULL)
            return iId;
    }
    iIndex = HashStrFind(pApi->pHasher, Name);
    if (iIndex == 0)
    {
        if (strlen(Name) < 0x21)
        {
            pBuddy = (BuddyApiBuddyT *)pApi->pMemAlloc(0x11C);
            memset(pBuddy, 0, 0x11C);
            strcpy((char *)pBuddy, Name);
            iIndex = (int)DispListAdd(pApi->pRosterList, pBuddy);
            HashStrAdd(pApi->pHasher, pBuddy, (void *)iIndex);
        }
    }
    if (iIndex == 0)
        return iId;
    pBuddy = (BuddyApiBuddyT *)DispListGet(pApi->pRosterList, iIndex);
    if (pBuddy == NULL)
        return iId;
    if (iFlag != 0)
        pBuddy->uFlags |= iFlag;
    pBuddy->uFlags |= _BuddyApiFlagsFromAttr(pApi, Msg);
    if ((pBuddy->uFlags & 0xC0000) != 0)
        pBuddy->uFlags &= 0xFFFFFFFB;
    return iId;
}

// presencia de un buddy: copia el registro, aplica la presencia y avisa del
// cambio (y de la invitacion si era el usuario pendiente)
void _BuddyApiHandlePGETMsg(BuddyApiRefT *pApi, BuddyApiMsgT Msg)
{
    char Name[0x20];
    BuddyApiBuddyT Old;
    BuddyApiBuddyT *pBuddy;
    int iChanged;
    int iIndex;

    TagFieldGetString(TagFieldFind(Msg.pData, "USER"), Name, 0x20, "");
    _BuddyApiTruncUsername(Name);
    iIndex = HashStrFind(pApi->pHasher, Name);
    if (iIndex == 0)
        return;
    pBuddy = (BuddyApiBuddyT *)DispListGet(pApi->pRosterList, iIndex);
    Old = *pBuddy;
    if (_BuddyApiPresRecv(pApi, &Msg, pBuddy) == 1)
        iChanged = -1;
    else
        iChanged = Old.eState;
    if (memcmp(&Old, pBuddy, 0x11C) != 0)
    {
        DispListDirty(pApi->pRosterList, 1);
        if (pApi->pChangeCallback != NULL)
            pApi->pChangeCallback(pApi, pBuddy, 0x70676574, iChanged, pApi->pChangeRef);
    }
    if (pApi->X8FC[0] != 0)
    {
        if (strcmp(pApi->X8FC, Name) == 0)
        {
            pBuddy->uFlags |= 0x8000;
            if (pApi->pChangeCallback != NULL)
                pApi->pChangeCallback(pApi, pBuddy, 0x67696E76, 0x8000, pApi->pChangeRef);
            pApi->X8FC[0] = 0;
        }
    }
}

// notificacion de juego (GNOT): I/R leen HOST+GSTR, A/D leen USER; los flags
// van solos y el aviso sale con 'ginv'
int _BuddyApiHandleGNOTMsg(BuddyApiRefT *pApi, BuddyApiMsgT Msg)
{
    char Host[0x41];
    char Type[0x1F];
    char Gstr[0x11];
    BuddyApiBuddyT *pBuddy;
    int iFlag;
    int iType;

    iFlag = 9;
    TagFieldGetString(TagFieldFind(Msg.pData, "TYPE"), Type, 0x1F, "");
    iType = Type[0];
    if (iType == 'I' || iType == 'R')
    {
        TagFieldGetString(TagFieldFind(Msg.pData, "HOST"), Host, 0x41, "");
        TagFieldGetString(TagFieldFind(Msg.pData, "GSTR"), Gstr, 0x11, "");
    }
    else if (iType == 'A' || iType == 'D')
        TagFieldGetString(TagFieldFind(Msg.pData, "USER"), Host, 0x41, "");
    else
        return -2;
    _BuddyApiTruncUsername(Host);
    pBuddy = BuddyApiFind(pApi, (BuddyApiBuddyT *)Host);
    if (pBuddy == NULL)
        return -2;
    if (iType == 'I')
    {
        iFlag = 0x400000;
        pBuddy->uFlags |= 0x400000;
        strncpy(pBuddy->GameServer, Gstr, 0x11);
    }
    else if (iType == 'A')
    {
        iFlag = 8;
        pBuddy->uFlags |= 8;
    }
    else if (iType == 'D')
    {
        iFlag = 0x400;
        pBuddy->uFlags &= 0xFFFDFFFF;
    }
    if (iType == 'R')
    {
        iFlag = 0x800;
        pBuddy->uFlags &= 0xFFBFFFFF;
        pBuddy->GameServer[0] = 0;
    }
    if (iType == 'G')
    {
        iFlag = 0x8000;
        pBuddy->uFlags |= 0x8000;
    }
    if (pApi->pChangeCallback != NULL)
        pApi->pChangeCallback(pApi, pBuddy, 0x67696E76, iFlag, pApi->pChangeRef);
    return 0;
}

// notificacion de roster (RNOT): 'D' borra, 'X'/'A' casan los flags de
// invitacion y el resto se suma via AddOrSetFlags
int _BuddyApiHandleRNOTMsg(BuddyApiRefT *pApi, BuddyApiMsgT Msg)
{
    char Buf[0xFF];
    BuddyApiBuddyT Rec;
    BuddyApiBuddyT *pBuddy;
    int iChg;
    int iRet;

    memset(&Rec, 0, 0x11C);
    TagFieldGetString(TagFieldFind(Msg.pData, "USER"), Buf, 0xFF, "");
    _BuddyApiTruncUsername(Buf);
    strcpy((char *)&Rec, Buf);
    TagFieldGetString(TagFieldFind(Msg.pData, "CHNG"), Buf, 0xFF, "Z");
    iRet = 0;
    iChg = Buf[0];
    if (iChg == 'D')
    {
        pBuddy = BuddyApiFind(pApi, &Rec);
        if (pBuddy == NULL || (pBuddy->uFlags & 0x90000) != 0x90000)
            _BuddyApiDeleteIfNeeded(pApi, &Rec, 0xC0004);
        return iRet;
    }
    Rec.uFlags |= _BuddyApiFlagsFromAttr(pApi, Msg);
    if (iChg == 'X' || iChg == 'A')
    {
        pBuddy = BuddyApiFind(pApi, &Rec);
        if (pBuddy != NULL)
        {
            if ((pBuddy->uFlags & 0x80000) != 0)
            {
                if ((Rec.uFlags & 0x80000) != 0)
                    Rec.uFlags |= 4;
                pBuddy->uFlags &= 0xFFFBFFFF;
                pBuddy->uFlags &= 0xFFF7FFFF;
            }
            else if ((pBuddy->uFlags & 0x40000) != 0 && (Rec.uFlags & 0x40000) == 0)
            {
                Rec.uFlags |= 4;
                pBuddy->uFlags &= 0xFFFBFFFF;
                pBuddy->uFlags &= 0xFFF7FFFF;
            }
        }
    }
    iRet = _BuddyApiAddOrSetFlags(pApi, &Rec) == NULL ? -2 : 0;
    return iRet;
}

// un usuario buscado (ID 3): lo guarda en el primer hueco libre y parte su
// recurso en cabeza (4) y cola (16)
int _BuddyApiHandleSearchedUserMsg(BuddyApiRefT *pApi, BuddyApiMsgT Msg)
{
    char Name[0x64];
    int iSlot;
    int iRet = -1;

    if (TagFieldGetNumber(TagFieldFind(Msg.pData, "ID"), 0) != 3)
        return iRet;
    TagFieldGetString(TagFieldFind(Msg.pData, "USER"), Name, 0x64, "");
    _BuddyApiTruncUsername(Name);
    for (iSlot = 0; iSlot < 21; iSlot++)
    {
        if (pApi->Searched[iSlot].Data[0] == 0)
            break;
    }
    if (iSlot < 20)
    {
        strncpy((char *)&pApi->Searched[iSlot], Name, 0x11);
        TagFieldGetString(TagFieldFind(Msg.pData, "RSRC"), Name, 0x64, "");
        _BuddyApiParseResource(Name, 4, (char *)&pApi->Searched[iSlot] + 0x11, 0x10,
                               (char *)&pApi->Searched[iSlot] + 0x16);
    }
    iRet = 0;
    return iRet;
}

// la conexion: guarda todo, prepara el registro de login (USER/LKEY o el
// despiece user:pass), resuelve el nombre y arranca el timer
int BuddyApiConnect(BuddyApiRefT *pApi, const char *pHost, int iPort, int iTimeout, const char *pKey,
                    const char *pResource, void *pStatusCallback, void *pStatusRef)
{
    char Key[0x100];
    char User[0x100];
    char *pColon;

    BuddyApiDisconnect(pApi);
    pApi->pStatusRef = pStatusRef;
    pApi->pStatusCallback = pStatusCallback;
    pApi->iX18 = iPort;
    strncpy(pApi->X24, pHost, 0x80);
    strncpy(pApi->XA8, pKey, 0x80);
    strncpy(pApi->Resource, pResource, 0x80);
    if (pApi->iX1AC == 0)
        pApi->iX1AC = _BuddyApiExtractRandomSeed(pApi);
    pApi->ConnBuf[0] = 0;
    TagFieldSetString(pApi->ConnBuf, 0x100, "PROD", pApi->Product);
    TagFieldSetString(pApi->ConnBuf, 0x100, "VERS", pApi->Version);
    TagFieldSetString(pApi->ConnBuf, 0x100, "PRES", pApi->Domain);
    if (strchr(pKey, ':') == NULL)
    {
        TagFieldSetString(pApi->ConnBuf, 0x100, "USER", pResource);
        TagFieldSetString(pApi->ConnBuf, 0x100, "LKEY", pKey);
    }
    else
    {
        strncpy(User, pKey, 0xFF);
        User[0xFF] = 0;
        pColon = strchr(User, ':');
        if (pColon != NULL)
        {
            *pColon = 0;
            TagFieldDupl(Key, 0x100, User);
            if (strlen(Key) + strlen(pResource) < 0x100)
                strcat(Key, pResource);
            TagFieldSetString(pApi->ConnBuf, 0x100, "USER", Key);
            if (pColon[1] == '$')
                TagFieldSetString(pApi->ConnBuf, 0x100, "LKEY", pColon + 2);
            else
                TagFieldSetString(pApi->ConnBuf, 0x100, "PASS", pColon + 1);
        }
    }
    TagFieldDupl(pApi->X6B4, 0x10, pResource);
    if (pApi->X6B4[0] == '/')
        strcpy(pApi->X6B4, pApi->X6B4 + 1);
    pColon = strchr(pApi->X6B4, '/');
    if (pColon != NULL)
        *pColon = 0;
    pApi->pX08 = ProtoNameAsync(pHost[0] == '*' ? pHost + 1 : pHost, iTimeout * 1000);
    _BuddyApiStatus(pApi, 1);
    if (iTimeout > 0)
        pApi->iX14 = ProtoAriesTick(pApi->pAries) + iTimeout * 1000;
    else
        pApi->iX14 = 0;
    pApi->iX4E4 = -1;
    return 0;
}

// la reconexion automatica: agita la semilla y vuelve a conectar (o pospone)
int _BuddyApiReconnect(BuddyApiRefT *pApi, int iConnect)
{
    int iSeed;

    if (pApi->iX1A8 != 0)
    {
        iSeed = ((unsigned int)pApi->iX1AC & 0xFFFF) % 0x55 + 0xF;
        pApi->iX1AC = pApi->iX1AC * 0x19660D + 0x3C6EF35F;
        if (iConnect == 0)
        {
            pApi->iX14 = ProtoAriesTick(pApi->pAries) + iSeed * 1000;
            _BuddyApiStatus(pApi, 7);
            return 0;
        }
        return BuddyApiConnect(pApi, pApi->X24, pApi->iX18, pApi->iXA4 / 2 + iSeed,
                               pApi->XA8, pApi->Resource, pApi->pStatusCallback,
                               pApi->pStatusRef);
    }
    return 0;
}

// pide el roster entero (lo basico + la lista B + la lista I de pendientes)
DispListT *BuddyApiRoster(BuddyApiRefT *pApi, void (*pFlushCallback)(BuddyApiRefT *, void *, void *),
                          void *pFlushRef)
{
    char Rec[0x100];

    pApi->pFlushCallback = pFlushCallback;
    pApi->pFlushRef = pFlushRef;
    _createBuddyDispList(pApi);
    Rec[0] = 0;
    TagFieldSetString(Rec, 0x100, "LRSC", pApi->X6B4);
    TagFieldSetNumber(Rec, 0x100, "ID", 4);
    _BuddyApiTransact(pApi, (void *)MSGTYPE_EPGT, Rec);
    pApi->iRosterSeq = 0;
    Rec[0] = 0;
    TagFieldSetString(Rec, 0x100, "LRSC", pApi->X6B4);
    TagFieldSetString(Rec, 0x100, "LIST", "B");
    TagFieldSetString(Rec, 0x100, "PRES", "Y");
    TagFieldSetString(Rec, 0x100, "MAXR", "Y");
    TagFieldSetNumber(Rec, 0x100, "ID", 1);
    _BuddyApiTransact(pApi, (void *)MSGTYPE_RGET, Rec);
    Rec[0] = 0;
    TagFieldSetString(Rec, 0x100, "LRSC", pApi->X6B4);
    TagFieldSetString(Rec, 0x100, "LIST", "I");
    TagFieldSetString(Rec, 0x100, "PRES", "Y");
    TagFieldSetString(Rec, 0x100, "MAXR", "Y");
    TagFieldSetNumber(Rec, 0x100, "ID", 2);
    _BuddyApiTransact(pApi, (void *)MSGTYPE_RGET, Rec);
    DispListDirty(pApi->pRosterList, 1);
    return pApi->pRosterList;
}

// da de alta un buddy (validado) y manda RADD/RADM/PADD segun sus flags
int BuddyApiAdd(pApi, pBuddy, pCallback, pRef, iTimeout)
BuddyApiRefT *pApi;
BuddyApiBuddyT *pBuddy;
BuddyApiCallbackT pCallback;
void *pRef;
int iTimeout;
{
    char Rec[0x100];
    BuddyApiBuddyT *pFound;
    int iSeq;

    if (pBuddy == NULL)
        return -3;
    iSeq = 0;
    if (pBuddy->uFlags == 0)
        return -3;
    if (_BuddyApiValidateName(pApi, (char *)pBuddy) != 0)
        return -3;
    if ((pBuddy->uFlags & 0x100000) == 0)
        iSeq = _BuddyApiSetCallback(pApi, 0, pCallback, pRef, iTimeout);
    pFound = _BuddyApiAddOrSetFlags(pApi, pBuddy);
    if (pFound == NULL)
        return 0;
    Rec[0] = 0;
    TagFieldSetString(Rec, 0x100, "LRSC", pApi->X6B4);
    TagFieldSetString(Rec, 0x100, "USER", (const char *)pBuddy);
    if ((pBuddy->uFlags & 0x80204) != 0)
    {
        TagFieldSetNumber(Rec, 0x100, "ID", iSeq);
        pFound->iSeq = iSeq;
        pFound->uFlags |= 0x10000;
    }
    if ((pBuddy->uFlags & 4) != 0)
    {
        TagFieldSetString(Rec, 0x100, "LIST", "B");
        TagFieldSetString(Rec, 0x100, "PRES", "Y");
        _BuddyApiTransact(pApi, (void *)MSGTYPE_RADD, Rec);
    }
    if ((pBuddy->uFlags & 0x200) != 0)
    {
        TagFieldSetString(Rec, 0x100, "LIST", "I");
        _BuddyApiTransact(pApi, (void *)MSGTYPE_RADD, Rec);
    }
    if ((pBuddy->uFlags & 0x80000) != 0)
    {
        TagFieldSetString(Rec, 0x100, "PRES", "Y");
        _BuddyApiTransact(pApi, (void *)MSGTYPE_RADM, Rec);
    }
    if ((pBuddy->uFlags & 0x800000) != 0)
        _BuddyApiTransact(pApi, (void *)MSGTYPE_PADD, Rec);
    return 0;
}

// da de baja un buddy y manda RDEM/RDEL/PDEL segun los flags quitados
int BuddyApiDel(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, BuddyApiCallbackT pCallback,
                void *pRef, int iTimeout)
{
    char Rec[0x100];
    int iSeq;
    int iRemoved;
    int iRet;

    iRet = -2;
    if (pBuddy == NULL)
        return iRet;
    if (pApi->pRosterList == NULL)
        return iRet;
    if (_BuddyApiValidateName(pApi, (const char *)pBuddy) != 0)
        return -3;
    iSeq = _BuddyApiSetCallback(pApi, 1, pCallback, pRef, iTimeout);
    iRemoved = _BuddyApiDeleteIfNeeded(pApi, pBuddy, pBuddy->uFlags);
    if (iRemoved != 0)
    {
        if ((iRemoved & 4) != 0)
        {
            Rec[0] = 0;
            TagFieldSetNumber(Rec, 0x100, "ID", iSeq);
            TagFieldSetString(Rec, 0x100, "LRSC", pApi->X6B4);
            TagFieldSetString(Rec, 0x100, "LIST", "B");
            TagFieldSetString(Rec, 0x100, "PRES", "Y");
            TagFieldSetString(Rec, 0x100, "USER", (const char *)pBuddy);
            _BuddyApiTransact(pApi, (void *)MSGTYPE_RDEM, Rec);
            TagFieldSetNumber(Rec, 0x100, "ID", -1);
            _BuddyApiTransact(pApi, (void *)MSGTYPE_RDEL, Rec);
        }
        if ((iRemoved & 0x200) != 0)
        {
            Rec[0] = 0;
            TagFieldSetNumber(Rec, 0x100, "ID", iSeq);
            TagFieldSetString(Rec, 0x100, "LRSC", pApi->X6B4);
            TagFieldSetString(Rec, 0x100, "LIST", "I");
            TagFieldSetString(Rec, 0x100, "USER", (const char *)pBuddy);
            _BuddyApiTransact(pApi, (void *)MSGTYPE_RDEL, Rec);
        }
        if ((iRemoved & 0x800000) != 0)
        {
            Rec[0] = 0;
            TagFieldSetString(Rec, 0x100, "LRSC", pApi->X6B4);
            TagFieldSetString(Rec, 0x100, "USER", (const char *)pBuddy);
            _BuddyApiTransact(pApi, (void *)MSGTYPE_PDEL, Rec);
        }
        iRet = 0;
    }
    return iRet;
}

// busca usuarios por mascara ('USCH') y limpia los 21 huecos de 0x27 B
int BuddyApiFindUsers(BuddyApiRefT *pApi, const char *pMask, BuddyApiCallbackT pCallback,
                      void *pRef, int iTimeout)
{
    char Rec[0xFF];

    if (pMask[0] == 0)
        return -3;
    memset(pApi->Searched, 0, 0x333);
    _BuddyApiSetCallback(pApi, 3, pCallback, pRef, iTimeout);
    Rec[0] = 0;
    TagFieldSetNumber(Rec, 0xFF, "ID", 3);
    TagFieldSetString(Rec, 0xFF, "RSRC", pApi->X6B4);
    TagFieldSetString(Rec, 0xFF, "USER", pMask);
    TagFieldSetNumber(Rec, 0xFF, "MAXR", 0x14);
    pApi->iX9CC = 9999;
    _BuddyApiTransact(pApi, (void *)MSGTYPE_USCH, Rec);
    return 0;
}

// el estado propio: RSRC/SHOW van a +0x454; si cambia y venimos de PASS/DND
// con estado CHAT, repide el roster; al vencer el plazo envia la presencia
int BuddyApiPresSend(BuddyApiRefT *pApi, const char *pText, int iStatus, int iX, void *pY)
{
    const char *pStat;
    int iNow;

    iNow = ProtoAriesTick(pApi->pAries);
    pStat = (unsigned int)iStatus < 7 ? _BuddyApi_Status[iStatus] : _BuddyApi_Status[0];
    pApi->X454[0] = 0;
    TagFieldSetString(pApi->X454, 0x80, "RSRC", pText);
    TagFieldSetString(pApi->X454, 0x80, "SHOW", pStat);
    if (pApi->iX4E0 == 0)
        pApi->iX4E0 = iNow + iX;
    if (pApi->iX4E4 != iStatus)
    {
        if (pApi->iX4E4 == 5 || pApi->iX4E4 == 6)
        {
            if (iStatus == 1)
            {
                if (pY == NULL)
                    BuddyApiRoster(pApi, pApi->pFlushCallback, pApi->pFlushRef);
            }
        }
        pApi->iX4E4 = iStatus;
        pApi->iX4E0 = iNow;
    }
    if ((unsigned int)iNow >= (unsigned int)pApi->iX4E0)
        _BuddyApiPresSend(pApi);
    return 0;
}

// responde a una invitacion de buddy: 0 acepta, 1 ignora, 2 rechaza, 3 veta
int BuddyApiRespondBuddy(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, int iAnswer,
                         BuddyApiCallbackT pCallback, void *pRef, int iTimeout)
{
    const char *pAnswer = _BuddyApi_Answer[iAnswer];
    unsigned int uType;
    int iFlag;
    int iRet;

    uType = MSGTYPE_RRSP;
    iFlag = 0;
    if (iAnswer == 2)
        iRet = _BuddyApiSetCallback(pApi, 1, pCallback, pRef, iTimeout);
    else
        iRet = _BuddyApiSetCallback(pApi, 6, pCallback, pRef, iTimeout);
    switch (iAnswer)
    {
    case 1:
        break;
    case 0:
        iFlag = 1;
        break;
    case 2:
        uType = MSGTYPE_RDEM;
        break;
    case 3:
        break;
    default:
        return -3;
    }
    iRet = _BuddyApiSendResponse(pApi, (const char *)pBuddy, uType, pAnswer, iFlag, iRet);
    if (iRet != 0)
        return iRet;
    switch (iAnswer)
    {
    case 1:
        _BuddyApiDeleteIfNeeded(pApi, pBuddy, 0x40000);
        break;
    case 0:
        if ((pBuddy->uFlags & 0x40000) != 0)
            pBuddy->uFlags = (pBuddy->uFlags & 0xFFFBFFFF & 0xFFF7FFFF) | 4;
        break;
    case 2:
        _BuddyApiDeleteIfNeeded(pApi, pBuddy, 0x80000);
        break;
    case 3:
        if ((pBuddy->uFlags & 0x40000) != 0)
            pBuddy->uFlags = (pBuddy->uFlags & 0xFFFBFFFF) | 0x200;
        break;
    }
    return iRet;
}

// responde a una invitacion de juego: 2 monta la respuesta GRVK a mano
int BuddyApiRespondGame(BuddyApiRefT *pApi, BuddyApiBuddyT *pBuddy, int iAnswer,
                        BuddyApiCallbackT pCallback, void *pRef, int iTimeout, int iSess)
{
    char Rec[0xFA];
    const char *pAnswer = _BuddyApi_Answer[iAnswer];
    unsigned int uType;
    int iSeq;

    uType = MSGTYPE_GRSP;
    switch (iAnswer)
    {
    case 1:
        pBuddy->uFlags &= 0xFBBF73F7;
        break;
    case 0:
        pBuddy->uFlags |= 8;
        break;
    case 2:
        uType = MSGTYPE_GRVK;
        if (pBuddy != NULL)
            pBuddy->uFlags &= 0xFBFD73F7;
        break;
    case 4:
        pBuddy->uFlags &= 0xFBBD73F7;
        break;
    default:
        return -3;
    }
    iSeq = _BuddyApiSetCallback(pApi, 7, pCallback, pRef, iTimeout);
    if (uType != MSGTYPE_GRVK)
        return _BuddyApiSendResponse(pApi, (const char *)pBuddy, uType, pAnswer, 0, iSeq);
    if (pBuddy != NULL)
    {
        Rec[0] = 0;
        TagFieldSetString(Rec, 0xFA, "USER", (const char *)pBuddy);
    }
    TagFieldSetString(Rec, 0xFA, "RSRC", pApi->Resource);
    TagFieldSetNumber(Rec, 0xFA, "ID", iSeq);
    TagFieldSetString(Rec, 0xFA, "TITL", pApi->GameTitle);
    TagFieldSetNumber(Rec, 0xFA, "SESS", iSess);
    _BuddyApiTransact(pApi, (void *)MSGTYPE_GRVK, Rec);
    return 0;
}

// el latido: resuelve el nombre, conecta, vence plazos, despacha todos los
// mensajes entrantes y refresca el roster. Devuelve el tick de entrada
int BuddyApiUpdate(BuddyApiRefT *pApi)
{
    char Name[0x20];
    BuddyApiMsgT Msg;
    char Buf[0x64];
    BuddyApiBuddyT *pBuddy;
    int iExtra;
    int iNow;
    int iOldRoster;
    int iRes;
    int i;

    iOldRoster = pApi->iRosterSeq;
    iNow = ProtoAriesTick(pApi->pAries);
    ProtoAriesUpdate(pApi->pAries);
    memset(&Msg, 0, 0x18);
    if (pApi->pStatusCallback != NULL && pApi->pX08 != NULL)
    {
        iRes = pApi->pX08->pPoll(pApi->pX08);
        if (iRes != 0)
        {
            if (iRes > 0)
            {
                const char *pHost = pApi->X24;

                if (pApi->X24[0] == '*')
                    pHost = pApi->X24 + 1;
                ProtoAriesConnect(pApi->pAries, pHost, ((void **)pApi->pX08)[1], pApi->iX18);
                if (pHost != pApi->X24)
                    ProtoAriesSecure(pApi->pAries, (void *)1);
                _BuddyApiTransact(pApi, (void *)MSGTYPE_AUTH, pApi->ConnBuf);
            }
            if (iRes < 0)
            {
                _BuddyApiStatus(pApi, 2);
                if (pApi->iX1A8 == 0)
                    pApi->pStatusCallback = NULL;
            }
            pApi->pX08->pDestroy(pApi->pX08);
            pApi->pX08 = NULL;
        }
    }
    if (pApi->iX14 != 0 && (unsigned int)pApi->iX14 < (unsigned int)iNow)
    {
        if (pApi->iStatus == 7)
            _BuddyApiReconnect(pApi, 1);
        else
        {
            _BuddyApiStatus(pApi, 3);
            if (pApi->iX1A8 == 0)
                pApi->pStatusCallback = NULL;
        }
    }
    if (pApi->iStatus == 5)
    {
        if (pApi->iX4E0 != 0 && (unsigned int)iNow >= (unsigned int)pApi->iX4E0)
            _BuddyApiPresSend(pApi);
    }
    if (pApi->iStatus == 5)
    {
        for (i = 0; i < 9; i++)
        {
            if (pApi->CbSlots[i].pCallback != NULL && pApi->CbSlots[i].iDeadline != 0
                && (unsigned int)pApi->CbSlots[i].iDeadline < (unsigned int)iNow)
            {
                pApi->CbSlots[i].iDeadline = 0;
                ProtoAriesUnconnect(pApi->pAries);
                _BuddyApiStatus(pApi, 3);
                break;
            }
        }
    }
    if (pApi->iStatus == 6 || pApi->iStatus == 3)
    {
        for (i = 0; i < 9; i++)
        {
            if (pApi->CbSlots[i].pCallback != NULL)
            {
                Msg.uType = pApi->CbSlots[i].uType;
                pApi->CbSlots[i].pCallback(pApi, &Msg, pApi->CbSlots[i].pRef);
                Msg.uCode = -4;
                pApi->CbSlots[i].pCallback = NULL;
            }
        }
        if (pApi->iStatus == 3)
            _BuddyApiReconnect(pApi, 1);
    }
    while (ProtoAriesPeek(pApi->pAries, &Msg.uKind, &Msg.uCode, &iExtra) >= 0)
    {
        Msg.pData = (char *)iExtra;
        if (pApi->pDebugFn != NULL)
            _BuddyApiDebug(pApi, "recv: ", &Msg);
        if (Msg.uKind == MSGTYPE_RCVD)
        {
            if (pApi->pRecvCallback != NULL)
                _BuddyApiRecv(pApi, &Msg);
        }
        if (Msg.uKind == MSGTYPE_SEND)
        {
            if (pApi->CbSlots[2].pCallback != NULL)
            {
                Msg.uType = 0x73656E64;
                pApi->CbSlots[2].pCallback(pApi, &Msg, pApi->CbSlots[2].pRef);
                pApi->CbSlots[2].pCallback = NULL;
            }
        }
        if (Msg.uKind == MSGTYPE_BRDC)
        {
            if (pApi->CbSlots[4].pCallback != NULL)
            {
                Msg.uType = 0x62726463;
                pApi->CbSlots[4].pCallback(pApi, &Msg, pApi->CbSlots[4].pRef);
                pApi->CbSlots[4].pCallback = NULL;
            }
        }
        if (Msg.uKind == -1 && Msg.uCode == Msg.uKind)
            _BuddyApiStatus(pApi, 5);
        if (Msg.uKind == MSGTYPE_PING)
            ProtoAriesSend(pApi->pAries, (void *)MSGTYPE_PING, (void *)Msg.uCode, Msg.pData, -1);
        if (Msg.uKind == MSGTYPE_AUTH)
        {
            if (Msg.uCode != 0)
            {
                pApi->iX14 = 0;
                if (pApi->pStatusCallback != NULL)
                {
                    Msg.uType = 0x61757468;
                    pApi->pStatusCallback(pApi, &Msg, pApi->pStatusRef);
                }
                if (Msg.uCode == 0x6E657477 || Msg.uCode == 0x62736F64)
                {
                    BuddyApiDisconnect(pApi);
                    _BuddyApiReconnect(pApi, 0);
                }
            }
            else
            {
                TagFieldGetString(TagFieldFind(Msg.pData, "TITL"), pApi->GameTitle, 0x64, "");
                if (pApi->pStatusCallback != NULL)
                {
                    Msg.uType = 0x61757468;
                    pApi->pStatusCallback(pApi, &Msg, pApi->pStatusRef);
                }
            }
        }
        if (Msg.uKind == 0x41444D4E)
        {
            TagFieldGetString(TagFieldFind(Msg.pData, "TYPE"), Buf, 0x1F, "");
            if (ds_stricmp(Buf, "DUPL") != 0)
                _BuddyApiStatus(pApi, 6);
            if (pApi->pRecvCallback != NULL)
            {
                Msg.uType = 0x61646D6E;
                pApi->pRecvCallback(pApi, &Msg, pApi->pRecvRef);
            }
        }
        if (Msg.uKind == MSGTYPE_RGET)
        {
            if (TagFieldGetNumber(TagFieldFind(Msg.pData, "ID"), 0) == 2)
            {
                if (TagFieldGetNumber(TagFieldFind(Msg.pData, "SIZE"), 0) == 0)
                {
                    pApi->iX8E0 = 0;
                    pApi->iRosterSeq = 1;
                }
            }
        }
        if (Msg.uKind == MSGTYPE_ROST)
        {
            if (pApi->pRosterList != NULL)
            {
                if (_BuddyApiHandleROSTMsg(pApi, Msg) == 2)
                {
                    pApi->iX8E0--;
                    if (pApi->iX8E0 == 0)
                        pApi->iRosterSeq = 1;
                }
            }
        }
        if (Msg.uKind == MSGTYPE_RNOT)
        {
            if (pApi->pRosterList != NULL)
            {
                if (_BuddyApiHandleRNOTMsg(pApi, Msg) == 0)
                    DispListDirty(pApi->pRosterList, 1);
            }
        }
        if (Msg.uKind == MSGTYPE_PGET)
        {
            if (pApi->pRosterList != NULL)
                _BuddyApiHandlePGETMsg(pApi, Msg);
        }
        if (Msg.uKind == MSGTYPE_RADD || Msg.uKind == MSGTYPE_RADM)
        {
            if (pApi->pRosterList != NULL)
            {
                int iSeq;
                int iRemove;

                Msg.uType = 0x61646475;
                iSeq = TagFieldGetNumber(TagFieldFind(Msg.pData, "ID"), 0);
                pApi->CbSlots[0].iSeq = 0;
                iRemove = 1;
                for (i = 0; ; i++)
                {
                    pBuddy = (BuddyApiBuddyT *)DispListIndex(pApi->pRosterList, i);
                    if (pBuddy == NULL)
                        break;
                    if ((pBuddy->uFlags & 0x10000) == 0)
                        continue;
                    if (pBuddy->iSeq != iSeq)
                        continue;
                    pBuddy->uFlags &= 0xFFFEFFFF;
                    if (Msg.uCode == 0)
                    {
                        TagFieldGetString(TagFieldFind(Msg.pData, "FUSR"), Name, 0x20, "");
                        _BuddyApiTruncUsername(Name);
                        if (Name[0] != 0 && strlen(Name) < 0x21)
                        {
                            iExtra = HashStrDel(pApi->pHasher, pBuddy);
                            TagFieldDupl((char *)pBuddy, 0x21, Name);
                            HashStrAdd(pApi->pHasher, pBuddy, (void *)iExtra);
                        }
                        if (pApi->CbSlots[0].pCallback != NULL)
                        {
                            Msg.uCode = 0;
                            Msg.pData = (char *)pBuddy;
                            pApi->CbSlots[0].pCallback(pApi, &Msg, pApi->CbSlots[0].pRef);
                        }
                        pBuddy->iSeq = 0;
                        pBuddy->uFlags &= 0xFFFEFFFF;
                        continue;
                    }
                    if (Msg.uCode == 0x626C636B)
                        Msg.uCode = 0;
                    else if (Msg.uCode == 0x66756C6C)
                        Msg.uCode = -5;
                    else
                        Msg.uCode = -2;
                    if (Msg.uKind == MSGTYPE_RADD && (pBuddy->uFlags & 0x8C0004) != 0)
                    {
                        pBuddy->uFlags &= 0xFFFFFDFF;
                        iRemove = 0;
                    }
                    if (Msg.uKind == MSGTYPE_RADM && (pBuddy->uFlags & 0x840200) != 0)
                    {
                        pBuddy->uFlags &= 0xFFF7FFFF;
                        iRemove = 0;
                    }
                    if (pApi->CbSlots[0].pCallback != NULL)
                    {
                        Msg.pData = (char *)pBuddy;
                        pApi->CbSlots[0].pCallback(pApi, &Msg, pApi->CbSlots[0].pRef);
                    }
                    if (iRemove != 0)
                    {
                        DispListDel(pApi->pRosterList, (void *)HashStrDel(pApi->pHasher, pBuddy));
                        if (pApi->pDelCallback != NULL)
                            pApi->pDelCallback(pApi, (BuddyApiMsgT *)pBuddy, pApi->pDelRef);
                        pApi->pMemFree(pBuddy);
                        break;
                    }
                    pBuddy->iSeq = 0;
                    pBuddy->uFlags &= 0xFFFEFFFF;
                }
                DispListDirty(pApi->pRosterList, 1);
            }
        }
        if (Msg.uKind == MSGTYPE_PADD)
        {
            if (pApi->CbSlots[0].pCallback != NULL)
            {
                if (Msg.pData == NULL || Msg.pData[0] == 0)
                    pApi->CbSlots[0].iSeq = 0;
            }
        }
        if (Msg.uKind == MSGTYPE_PDEL)
        {
            if (pApi->CbSlots[1].pCallback != NULL)
            {
                if (Msg.pData == NULL || Msg.pData[0] == 0)
                    pApi->CbSlots[1].iSeq = 0;
            }
        }
        if (Msg.uKind == MSGTYPE_RDEL || Msg.uKind == MSGTYPE_RDEM)
        {
            Msg.uType = 0x64656C75;
            _processAck(pApi, Msg, 1);
        }
        if (Msg.uKind == MSGTYPE_RRSP)
        {
            Msg.uType = 0x696E7674;
            _processAck(pApi, Msg, 6);
        }
        if (Msg.uKind == MSGTYPE_GRSP || Msg.uKind == MSGTYPE_GRVK)
        {
            Msg.uType = 0x67696E76;
            _processAck(pApi, Msg, 7);
        }
        if (Msg.uKind == MSGTYPE_GINV)
        {
            Msg.uType = 0x67696E76;
            _processAck(pApi, Msg, 8);
        }
        if (Msg.uKind == MSGTYPE_GNOT)
        {
            if (_BuddyApiHandleGNOTMsg(pApi, Msg) == 0)
            {
                if (pApi->pRosterList != NULL)
                    DispListDirty(pApi->pRosterList, 1);
            }
        }
        if (Msg.uKind == MSGTYPE_EPST)
        {
            Msg.uType = 0x65707374;
            _processAck(pApi, Msg, 5);
        }
        if (Msg.uKind == MSGTYPE_EPGT)
        {
            if (TagFieldGetNumber(TagFieldFind(Msg.pData, "ID"), 0) == 4)
            {
                if (Msg.uCode == 0)
                {
                    pApi->iX8C4 &= 0xFFFFFEFF;
                    TagFieldGetString(TagFieldFind(Msg.pData, "ADDR"), pApi->X6C4, 0xFF, "");
                    TagFieldGetString(TagFieldFind(Msg.pData, "ENAB"), Buf, 0x64, "");
                    if (ds_stricmp(Buf, "T") == 0)
                        pApi->iX8C4 |= 1;
                }
            }
        }
        if (Msg.uKind == MSGTYPE_USCH)
        {
            if (TagFieldGetNumber(TagFieldFind(Msg.pData, "ID"), 0) == 3)
            {
                if (Msg.uCode == 0)
                    pApi->iX9CC = TagFieldGetNumber(TagFieldFind(Msg.pData, "SIZE"), 0);
                else
                {
                    if (Msg.uCode == 0x73757372)
                        pApi->iX9CC = 0;
                    else
                    {
                        if (pApi->CbSlots[3].pCallback != NULL)
                        {
                            Msg.uType = 0x75736572;
                            pApi->CbSlots[3].pCallback(pApi, &Msg, pApi->CbSlots[3].pRef);
                        }
                        pApi->CbSlots[3].pCallback = NULL;
                    }
                }
            }
        }
        if (Msg.uKind == MSGTYPE_USER)
        {
            if (_BuddyApiHandleSearchedUserMsg(pApi, Msg) == 0)
                pApi->iX9CC--;
        }
        if (pApi->iX9CC == 0)
        {
            if (pApi->CbSlots[3].pCallback != NULL)
            {
                Msg.uKind = 0;
                Msg.uType = 0x75736572;
                Msg.uCode = 0;
                Msg.pData = NULL;
                pApi->CbSlots[3].pCallback(pApi, &Msg, pApi->CbSlots[3].pRef);
                pApi->CbSlots[3].pCallback = NULL;
            }
        }
        ProtoAriesRecv(pApi->pAries, NULL, NULL, NULL, NULL);
    }
    if (pApi->iRosterSeq != 0)
    {
        if (iOldRoster == 0 && pApi->pRosterList != NULL)
            DispListDirty(pApi->pRosterList, 1);
    }
    if (pApi->pFlushCallback != NULL)
    {
        if (DispListOrder(pApi->pRosterList) > 0)
        {
            Msg.uKind = 0;
            Msg.uType = MSGTYPE_ROST;
            Msg.uCode = 0;
            Msg.pData = (char *)pApi->pRosterList;
            pApi->pFlushCallback(pApi, &Msg, pApi->pFlushRef);
        }
    }
    return iNow;
}
