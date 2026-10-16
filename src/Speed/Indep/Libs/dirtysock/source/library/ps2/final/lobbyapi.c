// DirtySock: lobbyapi.c, la API de lobby sobre ProtoAries (el transporte de
// sesion). Escrito del asm de la A124 (libdirtysock.a(lobbyapi.o): .text
// 0x384140, 33 etiquetas + 2 pads; la unidad solo enlaza en el prototipo).
// El .mdebug del NFS.ELF no trae esta lib (venia precompilada): el unico
// oraculo es el asm + los datos (.rodata 0x489A70, .sdata 0x654170).
//
// Medido del asm (r48):
// - LobbyApiRefT mide 0x2078 B (LobbyApiCreate reserva e inicializa 0x2078).
// - La cola de peticiones: 32 entradas de 0x14 B en +0x1A20 (head +0x1CA0,
//   tail +0x1CA4, secuencia +0x1CB4); peticiones diferibles por 'idle'.
// - La tabla de callbacks: 32 entradas de 0xC B en +0x18A0 {iKind, pCallback,
//   pRef}; iKind = uKind|0x1000 o -1 (libre).
// - Los estados (+0x08) van en MAYUSCULAS de 4 letras ('init', 'conn',
//   'idle', 'offl', 'rdir', 'term', 'keyx'...).
// - Los 4-caracteres de mensaje van en orden de bytes grande ('auth' ==
//   0x61757468) via lui+ori.
#include <string.h>
#include <stdio.h>

typedef struct LobbyApiRefT LobbyApiRefT;
typedef struct LobbyApiRequestT LobbyApiRequestT;

typedef void (*LobbyApiCallbackT)(LobbyApiRefT *, void *, void *);
typedef void (*LobbyApiPingServerCbT)(void *, int, void *);

// --- otras unidades de dirtysock (aun asmtu)
extern void *NetAlloc(int iSize);
extern void NetFree(void *pMem);
extern void *LobbyAlloc(int iSize);
extern void LobbyFree(void *pMem);
extern int NetTick(void);
extern int TagFieldGetEpoch(const char *pField, int iDefault);
extern void *TagFieldGetAddress(const char *pField, int iDefault);
extern int TagFieldSetAddress(char *pBuffer, int iBufferSize, const char *pName, void *pValue);
extern int TagFieldSetBinary(char *pBuffer, int iBufferSize, const char *pName, void *pValue, int iLen);
extern int TagFieldGetBinary(const char *pField, void *pBuffer, int iBufferSize);
extern int TagFieldMerge(char *pDest, int iDestSize, const char *pSrc);
extern int TagFieldPrintf(char *pBuffer, int iBufferSize, const char *pFormat, ...);
extern int CryptSSC2StringEncrypt(const char *pSrc, int iSrcSize, char *pDst, void *pKey, int iDstSize, int iKeyLen);
extern void *ProtoAriesCreate(int iBufSize);
extern void ProtoAriesDestroy(void *pAries);
extern int ProtoAriesConnect(void *pAries, const char *pHost, void *pX, int iPort);
extern void ProtoAriesUnconnect(void *pAries);
extern int ProtoAriesSend(void *pAries, void *pKind, void *pCode, void *pData, int iLen);
extern void ProtoAriesRecv(void *pAries, void *pX, void *pY, void *pZ, void *pW);
extern int ProtoAriesPeek(void *pAries, unsigned int *puKind, unsigned int *puCode, int *piExtra);
extern int ProtoAriesTick(void *pAries);
extern void ProtoAriesUpdate(void *pAries);
extern int ProtoAriesStatus(void *pAries, unsigned int uKind);
extern void ProtoAriesSetKey(void *pAries, void *pKey, int iKeyLen);
extern void ProtoAriesSecure(void *pAries, void *pX);
extern void *PingManagerCreate(int iX);
extern void PingManagerDestroy(void *pPing);
extern void PingManagerUpdate(void *pPing);
extern int PingManagerPingAddress(void *pPing, void *pAddr, void *pX, void *pY);
extern void PingManagerPingServer(void *pPing, int iAddr, LobbyApiPingServerCbT pCallback, void *pRef);
extern int DispListCount(void *pList);
extern void DispListDirty(void *pList, int iDirty);
extern void *DispListGet(void *pList, int iIndex);
extern void *DispListIndex(void *pList, int iIndex);
extern int LobbyApiListCreate(LobbyApiRefT *pApi);
extern void LobbyApiListDestroy(LobbyApiRefT *pApi);
extern void LobbyApiListUpdate(LobbyApiRefT *pApi);
extern void LobbyApiListFlush(LobbyApiRefT *pApi, int iX);
extern void *LobbyApiListGetHash(LobbyApiRefT *pApi, int iIndex);
extern void *LobbyApiListGetDisp(LobbyApiRefT *pApi, int iIndex);
extern int LobbyApiListFindByName(LobbyApiRefT *pApi, int iIndex, const char *pName);
extern void LobbyApiExtractUserAddr(void *pUser, const char *pSrc);
extern int LobbyApiAsyncProcess(LobbyApiRefT *pApi, const char *pSrc, void *pX, void *pY);
extern int LobbyApiAsyncMoveRoom(LobbyApiRefT *pApi, const char *pSrc, void *pX, void *pY);
extern char *TagFieldFind(const char *pRecord, const char *pName);
extern int TagFieldGetNumber(const char *pField, int iDefault);
extern int TagFieldGetString(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault);
extern char *TagFieldDupl(char *pDest, int iDestSize, const char *pSrc);
extern int TagFieldSetNumber(char *pBuffer, int iBufferSize, const char *pName, int iValue);
extern int TagFieldSetString(char *pBuffer, int iBufferSize, const char *pName, const char *pValue);

// los tipos de mensaje y estado (constantes de cuatro letras, big endian)
#define KIND_IDLE 0x69646C65
#define KIND_AUTH 0x61757468
#define KIND_ACCT 0x61636374
#define KIND_SKEY 0x736B6579
#define KIND_PASS 0x70617373
#define KIND_SNAP 0x736E6170
#define KIND_MOVE 0x6D6F7665
#define KIND_NEWS 0x6E657773
#define KIND_CONN 0x636F6E6E
#define KIND_INIT 0x696E6974
#define KIND_OFFL 0x6F66666C
#define KIND_RDIR 0x72646972
#define KIND_TERM 0x7465726D
#define KIND_SELE 0x73656C65
#define KIND_PERS 0x70657273
#define KIND_ROOM 0x726F6F6D
#define KIND_DQUE 0x44515545
#define KIND_CRYT 0x63727970
#define KIND_PNG 0x7E706E67
#define KIND_LPTR 0x6C707274
#define KIND_ADDR 0x61646472
#define KIND_SERV 0x73657276

// una peticion diferida: 0x14 B (uKind +0, iType +4, iTick +8, pCallback +0xC, pRef +0x10)
struct LobbyApiRequestT
{
    unsigned int uKind;             // +0x00 (la secuencia de +0x1CB4 & 0x7FFFFFFF)
    int iType;                      // +0x04
    int iTick;                      // +0x08 (tick de alta)
    LobbyApiCallbackT pCallback;    // +0x0C
    void *pRef;                     // +0x10
};

// un hueco de callback: 0xC B (iKind +0, pCallback +4, pRef +8; -1 = libre)
typedef struct LobbyApiCbSlotT
{
    int iKind;                      // +0x00 (uKind | 0x1000, o -1)
    LobbyApiCallbackT pCallback;    // +0x04
    void *pRef;                     // +0x08
} LobbyApiCbSlotT;

// el mensaje que reciben los callbacks (por valor, en el stack del llamador)
typedef struct LobbyApiMsgT
{
    int iCode;                      // +0x00
    unsigned int uKind;             // +0x04
    void *pData;                    // +0x08
    void *pRef;                     // +0x0C
    void *p1;                       // +0x10
    void *p2;                       // +0x14 (0 en GlobalCallback)
} LobbyApiMsgT;

struct LobbyApiRefT
{
    void *pAries;                               // +0x00
    void *pPing;                                // +0x04 (PingManager)
    unsigned int uKind;                         // +0x08 (el estado, 4 letras)
    int iSuspend;                               // +0x0C
    int iData;                                  // +0x10 (Control 'dobj')
    int iTimestamp;                             // +0x14 (tick de la ultima actividad)
    int iLastActivity;                          // +0x18 (tick del ultimo paquete)
    int iPingServerTick;                        // +0x1C (vencimiento del ping al servidor)
    unsigned int iIcmpPing;                    // +0x20 (resultado del ICMP, 0xFFFFFFFF = sin dato)
    int iIcmpState;                             // +0x24 (0..3)
    int iPingIndex;                             // +0x28 (el indexeado del DispList)
    int iTimeout;                               // +0x2C (defecto 0x1D4C0)
    int iMaxCps;                                // +0x30 (Control 'itim', 0xEA60)
    int iMinCps;                                // +0x34 (Control 'rtim', 0x7530)
    char Key[0x10];                             // +0x38 (la llave SSC2)
    char Account[0x100];                        // +0x48 (el TagField de cuenta)
    char X148[0x200];                           // +0x148 ('pers')
    char X348[0x200];                           // +0x348 ('perd')
    char Party[0x40];                           // +0x548 (el party buffer)
    char Config[0x1004];                        // +0x588 (la config descargada)
    char User158C[0x14];                         // +0x158C ('acct'/'user')
    unsigned char Pad15A0[0x15A0 - 0x15A0];
    char Serv[0x244];                           // +0x15A0 (datos de sesion del servidor)
    int iRooms;                                  // +0x17E4 ('room')
    int iRflg;                                   // +0x17E8 ('rflg')
    char Self17EC[0x48];                        // +0x17EC ('self')
    unsigned char Pad1834[0x1834 - 0x1834];
    int iListCreated;                           // +0x1834
    int iSessPort;                              // +0x1838
    int iSessAddr;                              // +0x183C
    int i1840;                                  // +0x1840 (puerto alternativo de sesion)
    int i1844;                                  // +0x1844 (MASK de UpdateRdir)
    char X1848[0x40];                           // +0x1848 (DP del login)
    unsigned char Pad1888[0x1888 - 0x1888];
    int iLoggedIn;                              // +0x1888 (el ADDR de sesion)
    int iLoginFail;                             // +0x188C ('rdrl' del UpdateRdir)
    int iMore;                                  // +0x1890
    int i1894;                                  // +0x1894
    int i1898;                                  // +0x1898 (peticiones 'snap' pendientes)
    int i189C;                                  // +0x189C (slots)
    LobbyApiCbSlotT CbSlots[32];                // +0x18A0
    LobbyApiRequestT Queue[32];                 // +0x1A20
    int iQueueHead;                             // +0x1CA0
    int iQueueTail;                             // +0x1CA4
    int i1CA8;                                  // +0x1CA8
    void *pUser1CAC;                            // +0x1CAC (arg de LobbyApiCreate)
    void *pUser1CB0;                            // +0x1CB0
    unsigned int uSeq;                          // +0x1CB4 (& 0x7FFFFFFF)
    int i1CB8;                                  // +0x1CB8 (tiempo acumulado, ms)
    int iEpoch;                                 // +0x1CBC (TagFieldGetEpoch)
    int i1CC0;                                  // +0x1CC0 (tick del ultimo Update)
    unsigned int i1CC4;                         // +0x1CC4 (resto de la division)
    unsigned char Pad1CC8[0x2018 - 0x1CC8];
    int i2018;                                  // +0x2018 ('vers': flag de listas)
    int i201C;                                  // +0x201C
    int i2020;                                  // +0x2020 ('lcnt' cache)
    int i2024;                                  // +0x2024 ('lcnt' cache)
    unsigned char Pad2028[0x2038 - 0x2028];
    char Sess[0x40];                            // +0x2038 (host de sesion, '*' = cifrado)
    unsigned char Pad2078[0x2078 - 0x2078];
};

// las que define este fichero mas abajo
extern void _LobbyApiDisconnect(LobbyApiRefT *pApi, unsigned int uKind, void *pX);
extern void LobbyApiDisconnect();
extern int _LobbyApiDequeueRequestCallback(LobbyApiRefT *pApi, LobbyApiRequestT *pReq, unsigned int uKind);
extern int LobbyApiSetCallback(LobbyApiRefT *pApi, int iSlot, LobbyApiCallbackT pCallback, void *pRef);
extern int LobbyApiClearCallback(LobbyApiRefT *pApi, int iKind);
extern int LobbyApiRequestCB(LobbyApiRefT *pApi, int iCode, const char *pData, void *pCallback, void *pRef);
extern void LobbyApiGlobalCallback();

// prepara una peticion: la limpia, guarda tipo/callback/ref y la sella con el tick
void _LobbyApiInitRequest(LobbyApiRefT *pApi, LobbyApiRequestT *pReq, int iType,
                          LobbyApiCallbackT pCallback, void *pRef)
{
    memset(pReq, 0, 0x14);
    pReq->iType = iType;
    pReq->pCallback = pCallback;
    pReq->pRef = pRef;
    pReq->iTick = ProtoAriesTick(pApi->pAries);
}

// alta en la cola circular de 32; devuelve el hueco o NULL si esta llena
LobbyApiRequestT *_LobbyApiEnqueueRequestCallback(LobbyApiRefT *pApi, int iType,
                                                  LobbyApiCallbackT pCallback, void *pRef)
{
    int iTail = pApi->iQueueTail;
    int iNext = (iTail + 1) % 32;

    LobbyApiRequestT *pRq;

    if (iNext == pApi->iQueueHead)
        return NULL;
    pApi->iQueueTail = iNext;
    pRq = &pApi->Queue[iTail];
    _LobbyApiInitRequest(pApi, pRq, iType, pCallback, pRef);
    pRq->uKind = pApi->uSeq;
    pApi->uSeq = (pApi->uSeq + 1) & 0x7FFFFFFF;
    return pRq;
}

// saca una peticion de la cola (por uKind o tipo DQUE); devuelve 1 si quedan
int _LobbyApiDequeueRequestCallback(LobbyApiRefT *pApi, LobbyApiRequestT *pReq, unsigned int uKind)
{
    LobbyApiRequestT *pSlot;
    int iHead;
    int iTail;

    memset(pReq, 0, 0x14);
    iHead = pApi->iQueueHead;
    iTail = pApi->iQueueTail;
    if (iHead != iTail)
    {
        if (pApi->Queue[iHead].iType == uKind || uKind == KIND_DQUE)
        {
            pSlot = &pApi->Queue[iHead];
            *pReq = *pSlot;
            pApi->iQueueHead = (pApi->iQueueHead + 1) % 32;
        }
    }
    return pApi->iQueueHead != pApi->iQueueTail;
}

// vacia la cola sacando todo lo diferible
void _LobbyApiClearQueue(LobbyApiRefT *pApi)
{
    LobbyApiRequestT Req;

    do
        ;
    while (_LobbyApiDequeueRequestCallback(pApi, &Req, KIND_DQUE) != 0);
}

// el callback de un disparo: llama, se desarma y se libera
void _LobbyApiOneShotCB(LobbyApiRefT *pApi, void *pX, void *pRef)
{
    ((LobbyApiRequestT *)pRef)->pCallback(pApi, pX, ((LobbyApiRequestT *)pRef)->pRef);
    LobbyApiClearCallback(pApi, ((LobbyApiRequestT *)pRef)->uKind);
    LobbyFree(pRef);
}

// registra un disparo (iSlot 0..31); devuelve el uKind o -1 sin memoria
int _LobbyApiSetOneShotCB(LobbyApiRefT *pApi, int iSlot, LobbyApiCallbackT pCallback, void *pRef)
{
    LobbyApiRequestT *pReq = LobbyAlloc(0x14);

    if (pReq == NULL)
        return -1;
    _LobbyApiInitRequest(pApi, pReq, 0x6F6E6365, pCallback, pRef);
    pReq->uKind = LobbyApiSetCallback(pApi, iSlot, _LobbyApiOneShotCB, pReq);
    return pReq->uKind;
}

// cancela un disparo por su uKind (sin el bit 0x1000); 1 si estaba
int _LobbyApiCancelOneShotCB(LobbyApiRefT *pApi, int iKind)
{
    void *pRef;

    iKind &= ~0x1000;
    if ((unsigned int)iKind >= 0x20)
        return 0;
    pRef = pApi->CbSlots[iKind].pRef;
    LobbyApiClearCallback(pApi, iKind);
    LobbyFree(pRef);
    return 1;
}

// refresca el ping del jugador indexado en la lista de usuarios
void _LobbyApiUpdatePing(LobbyApiRefT *pApi)
{
    void *pList;
    void *pUser;
    char Buf[0x18];
    int iPing;

    if (pApi->pPing == NULL)
        return;
    PingManagerUpdate(pApi->pPing);
    if (LobbyApiListGetHash(pApi, 1) == NULL)
        return;
    pList = LobbyApiListGetDisp(pApi, 1);
    pUser = DispListIndex(pList, pApi->iPingIndex);
    if (pUser == NULL)
    {
        pApi->iPingIndex = 0;
        pUser = DispListIndex(pList, 0);
        if (pUser == NULL)
            return;
    }
    iPing = PingManagerPingAddress(pApi->pPing, (char *)pUser + 0x1E0, NULL, NULL);
    if (iPing > 0)
    {
        sprintf(Buf, "%d", iPing);
        if (strcmp((char *)pUser + 0x18, Buf) != 0)
        {
            strcpy((char *)pUser + 0x18, Buf);
            DispListDirty(pList, 1);
        }
    }
    pApi->iPingIndex = pApi->iPingIndex + 1;
}

// callback del ping ICMP al servidor: guarda resultado y programa el siguiente
void _LobbyApiPingServerCb(void *pUnused, int iResult, void *pRef)
{
    LobbyApiRefT *pApi = (LobbyApiRefT *)pRef;

    if (iResult == 0)
    {
        pApi->iIcmpState = pApi->iIcmpState + 1;
        if (pApi->iIcmpState == 3)
            pApi->iIcmpState = 0;
        else
        {
            pApi->iPingServerTick = NetTick();
            return;
        }
    }
    else
    {
        pApi->iIcmpPing = iResult;
        pApi->iPingServerTick = NetTick() + 0x7530;
        pApi->iIcmpState = 0;
    }
}

// lanza el ping ICMP al servidor de sesion
void _LobbyApiPingServer(LobbyApiRefT *pApi)
{
    if (pApi->pPing == NULL)
        return;
    PingManagerPingServer(pApi->pPing, pApi->iSessAddr, _LobbyApiPingServerCb, pApi);
}

// deja el estado ICMP del servidor como recien creado
void _LobbyApiResetServerIcmpPingState(LobbyApiRefT *pApi)
{
    pApi->iIcmpState = 0;
    pApi->iPingServerTick = 0;
    pApi->iIcmpPing = 0xFFFFFFFF;
}

// la config que baja del servidor: guarda 'new7' y avisa 'conn'
void _LobbyApiConfigCallback(LobbyApiRefT *pApi, LobbyApiMsgT *pMsg, void *pRef)
{
    if (pMsg->pRef == (void *)0x6E657737)
        TagFieldDupl(pApi->Config, 0x1004, pMsg->p1);
    LobbyApiGlobalCallback(pApi, 0, 3, KIND_CONN, 0, 0);
}

// el arranque de sesion: conecta (cifrado si el host lleva '*')
int _LobbyApiUpdateInit(LobbyApiRefT *pApi)
{
    char *pSess = &pApi->Sess[0];
    int iStar = pApi->Sess[0] == '*';
    int iTick;

    if (iStar != 0)
        pSess = &pApi->Sess[1];
    ProtoAriesUnconnect(pApi->pAries);
    if (iStar == 0 || ProtoAriesStatus(pApi->pAries, KIND_CRYT) != 0)
        ProtoAriesConnect(pApi->pAries, pSess, NULL, pApi->i1840);
    else
    {
        ProtoAriesConnect(pApi->pAries, pSess, NULL, pApi->i1840 + 1);
        ProtoAriesSecure(pApi->pAries, (void *)1);
    }
    pApi->uKind = KIND_RDIR;
    pApi->iTimestamp = ProtoAriesTick(pApi->pAries);
    pApi->iLastActivity = 0;
    return pApi->iTimestamp += pApi->iTimeout;
}

// vence la sesion por inactividad (5000 ms) o cola parada
void _LobbyApiCheckTimeout(LobbyApiRefT *pApi, int iNow)
{
    int iDelta;

    if (pApi->uKind == KIND_OFFL)
        return;
    if (pApi->iLastActivity != 0)
    {
        iDelta = iNow - pApi->iLastActivity;
        if (iDelta >= 0x1389)
            pApi->iTimestamp = pApi->iTimestamp + iDelta;
    }
    if (pApi->iTimestamp - iNow < 0)
        LobbyApiDisconnect(pApi, 0x74696D65);
    if (pApi->iQueueHead != pApi->iQueueTail)
    {
        if (pApi->iMinCps < iNow - pApi->Queue[pApi->iQueueHead].iTick)
            LobbyApiDisconnect(pApi, 0x74696D65);
    }
    pApi->iLastActivity = iNow;
}

// la desconexion completa: avisa una sola vez, baja todo y limpia la sesion
void _LobbyApiDisconnect(LobbyApiRefT *pApi, unsigned int uKind, void *pX)
{
    if (pApi->uKind != KIND_OFFL)
    {
        pApi->uKind = KIND_OFFL;
        LobbyApiGlobalCallback(pApi, 0, 3, (void *)0x64697363, (void *)uKind, pX);
    }
    ProtoAriesUnconnect(pApi->pAries);
    LobbyApiListFlush(pApi, -1);
    _LobbyApiClearQueue(pApi);
    _LobbyApiResetServerIcmpPingState(pApi);
    pApi->i1CA8 = 0;
    pApi->uSeq = 0;
    memset(&pApi->Serv[0], 0, 0x244);
}

// atiende las redirecciones y llaves del protocolo de login
int _LobbyApiUpdateRdir(LobbyApiRefT *pApi, int iCode, int uKind, int iExtra, void *pData)
{
    char Buf[0x100];

    if (uKind == -1 && iExtra == -1)
    {
        TagFieldDupl(Buf, 0x100, pApi->Account);
        ProtoAriesSend(pApi->pAries, (void *)0x40746963, NULL, "RC4+MD5-V2", -1);
        ProtoAriesSend(pApi->pAries, (void *)0x40646972, NULL, Buf, -1);
    }
    if (uKind == 0x40746963)
        ProtoAriesSetKey(pApi->pAries, pData, iCode);
    if (uKind == 0x40646972)
    {
        pApi->iLoginFail = ProtoAriesStatus(pApi->pAries, 0x6C616472);
        if (TagFieldFind(pData, "DIRECT") == NULL)
        {
            pApi->iSessAddr = (int)TagFieldGetAddress(TagFieldFind(pData, "ADDR"), 0);
            pApi->iSessPort = TagFieldGetNumber(TagFieldFind(pData, "PORT"), 0);
        }
        TagFieldGetNumber(TagFieldFind(pData, "SESS"), 0);
        TagFieldGetString(TagFieldFind(pData, "MASK"), &pApi->Key[0], 0x40, "");
        ProtoAriesUnconnect(pApi->pAries);
        if (pApi->iSessAddr != 0)
        {
            _LobbyApiPingServer(pApi);
            pApi->uKind = KIND_CONN;
            ProtoAriesConnect(pApi->pAries, NULL, pApi->iSessAddr, pApi->iSessPort);
        }
        else
        {
            TagFieldGetString(TagFieldFind(pData, "DOWN"), Buf, 0x100, "");
            _LobbyApiDisconnect(pApi, 0x646F776E, Buf);
        }
        return -1;
    }
    if (uKind == 0xFFFFFFFF && iExtra == 0xFEFEFEFE)
    {
        if (ProtoAriesStatus(pApi->pAries, KIND_CRYT) == 0)
        {
            ProtoAriesSetKey(pApi->pAries, NULL, 0);
            pApi->uKind = KIND_INIT;
        }
        else
        {
            LobbyApiDisconnect(pApi, KIND_RDIR);
            return -1;
        }
        return -1;
    }
    return 0;
}

// crea la referencia: 0x2078 B, el ProtoAries, el ping y las tablas
LobbyApiRefT *LobbyApiCreate(const char *pAccount, void *pUser1, void *pUser2)
{
    LobbyApiRefT *pApi = NetAlloc(0x2078);
    char Buf[0x20];
    int i;

    if (pApi != NULL)
    {
    memset(pApi, 0, 0x2078);
    if (pAccount != NULL)
    {
        if (strchr(pAccount, '=') != NULL)
            TagFieldDupl(&pApi->Account[0], 0x100, pAccount);
        else
            TagFieldSetString(&pApi->Account[0], 0x100, "VERS", pAccount);
        sprintf(Buf, "%c.%c.%c.%c", 0x33, 0x38, 0x34, 0x30);
        TagFieldSetString(&pApi->Account[0], 0x100, "SDKVERS", Buf);
        TagFieldSetString(&pApi->Account[0], 0x100, "BUILDDATE", "Sep 12 2005");
    }
    pApi->pAries = ProtoAriesCreate(0x2000);
    if (pApi->pAries == NULL)
    {
        LobbyFree(pApi);
        return NULL;
    }
    pApi->pPing = PingManagerCreate(0x80);
    pApi->pUser1CAC = pUser1;
    pApi->pUser1CB0 = pUser2;
    pApi->iTimeout = 0x1D4C0;
    pApi->iMaxCps = 0xEA60;
    pApi->iMinCps = 0x7530;
    for (i = 31; i >= 0; i--)
        pApi->CbSlots[i].iKind = -1;
    LobbyApiListCreate(pApi);
    LobbyApiDisconnect(pApi, 0);
    pApi->iEpoch = TagFieldGetEpoch(NULL, 0);
    }
    return pApi;
}

// destruye todo lo creado
void LobbyApiDestroy(LobbyApiRefT *pApi)
{
    LobbyApiDisconnect(pApi, 0x7465726D);
    LobbyApiListDestroy(pApi);
    if (pApi->pPing != NULL)
        PingManagerDestroy(pApi->pPing);
    ProtoAriesDestroy(pApi->pAries);
    NetFree(pApi);
}

// arranca la conexion de sesion (con opcional callback de un disparo)
int LobbyApiConnect(LobbyApiRefT *pApi, const char *pHost, int iPort, void *pCallback, void *pRef, unsigned int iTimeout)
{
    int iKind = 0;

    pApi->iSessAddr = 0;
    pApi->iSessPort = iPort;
    pApi->i1840 = iPort;
    pApi->iTimeout = (0x1387 < iTimeout) ? iTimeout : 0x1388;
    _LobbyApiClearQueue(pApi);
    pApi->i1898 = 0;
    _LobbyApiResetServerIcmpPingState(pApi);
    if (pCallback != NULL)
    {
        iKind = _LobbyApiSetOneShotCB(pApi, 3, pCallback, pRef);
        if (iKind < 0)
            return iKind;
    }
    strncpy(&pApi->Sess[0], pHost, 0x40);
    pApi->Sess[0x3F] = 0;
    pApi->uKind = KIND_INIT;
    return iKind;
}

// desconexion publica (K&R: el uKind de a1 no se usa, a2 = NULL)
void LobbyApiDisconnect(pApi, uKind)
LobbyApiRefT *pApi;
unsigned int uKind;
{
    _LobbyApiDisconnect(pApi, uKind, 0);
}

// suspende la sesion (baja las listas y el ping, avisa 'sele')
void LobbyApiSuspend(LobbyApiRefT *pApi)
{
    char Buf[0x100];
    int i;

    if (pApi->iSuspend == 1)
        return;
    TagFieldPrintf(Buf, 0x100, "INGAME=%d MESGS=%d MESGTYPES=%f USERS=%d GAMES=%d MYGAME=%d ROOMS=%d ASYNC=%d USERSETS=%d STATS=%d", 1, 1, 0x2010080, 0, 0, 0, 0, 0, 0, 0);
    for (i = 0; i < 4; i++)
        TagFieldPrintf(Buf, 0x100, "~USERSET%d=", i);
    LobbyApiRequestCB(pApi, KIND_SELE, Buf, 0, 0);
    if (pApi->pPing != NULL)
    {
        PingManagerDestroy(pApi->pPing);
        pApi->pPing = NULL;
    }
    if (pApi->iListCreated != 0)
        LobbyApiListDestroy(pApi);
    _LobbyApiResetServerIcmpPingState(pApi);
    pApi->iSuspend = 1;
}

// reanuda la sesion suspendida
void LobbyApiResume(LobbyApiRefT *pApi)
{
    char Buf[0x20];

    if (pApi->iSuspend == 0)
        return;
    LobbyApiListCreate(pApi);
    pApi->pPing = PingManagerCreate(0x80);
    Buf[0] = 0;
    TagFieldSetNumber(Buf, 0x20, "INGAME", 0);
    LobbyApiRequestCB(pApi, KIND_SELE, Buf, 0, 0);
    pApi->iSuspend = 0;
}

// registra un callback por uKind en el primer hueco libre
int LobbyApiSetCallback(LobbyApiRefT *pApi, int iKind, LobbyApiCallbackT pCallback, void *pRef)
{
    LobbyApiCbSlotT *pSlot = &pApi->CbSlots[0];
    int i;

    for (i = 0; i < 32; i++)
    {
        if (pSlot->iKind == -1)
        {
            pSlot->iKind = iKind;
            pSlot->pCallback = pCallback;
            pSlot->pRef = pRef;
            return i | 0x1000;
        }
        pSlot++;
    }
    return -1;
}

// libera el callback de un uKind
int LobbyApiClearCallback(LobbyApiRefT *pApi, int iKind)
{
    iKind &= ~0x1000;
    if ((unsigned int)iKind < 0x20 && pApi->CbSlots[iKind].iKind != -1)
    {
        pApi->CbSlots[iKind].iKind = -1;
        return 1;
    }
    return 0;
}

// avisa a todos los callbacks de un uKind
void LobbyApiGlobalCallback(pApi, iCode, uKind, pData, pRef, p1)
LobbyApiRefT *pApi;
int iCode;
unsigned int uKind;
void *pData;
void *pRef;
void *p1;
{
    LobbyApiMsgT Msg;
    void **pWalk;
    LobbyApiCbSlotT *pSlot;

    if (uKind >= 6)
        return;
    pWalk = &pApi->CbSlots[0].pRef;
    pSlot = &pApi->CbSlots[0];
    Msg.iCode = iCode;
    Msg.pData = pData;
    Msg.pRef = pRef;
    Msg.p1 = p1;
    Msg.uKind = uKind;
    Msg.p2 = 0;
    do
    {
        if (pSlot->iKind == (int)uKind)
            ((LobbyApiCallbackT)pWalk[-1])(pApi, &Msg, pWalk[0]);
        pSlot += 1;
        pWalk += 3;
    } while ((int)pSlot < (int)&pApi->CbSlots[32]);
}

// manda una peticion: auth/acct/skey/pass/snap via cola, merge de cuenta y
// cifrado de la password si hace falta
int LobbyApiRequestCB(LobbyApiRefT *pApi, int iCode, const char *pData, void *pCallback, void *pRef)
{
    char Buf[0x244];
    LobbyApiRequestT *pReq;
    int iVal;

    if (pApi->uKind != KIND_IDLE && pApi->uKind != KIND_AUTH && pApi->uKind != KIND_ACCT
        && pApi->uKind != KIND_SKEY)
        return -1;
    pReq = _LobbyApiEnqueueRequestCallback(pApi, iCode, pCallback, pRef);
    if (pReq == NULL)
        return -2;
    if (iCode == KIND_AUTH || iCode == KIND_ACCT || iCode == KIND_PASS)
    {
        TagFieldDupl(Buf, 0x200, &pApi->Account[0]);
        TagFieldMerge(Buf, 0x200, pData);
        TagFieldGetString(TagFieldFind(Buf, "PASS"), &Buf[0x201], 0x1F, "");
        if (pApi->Key[0] == 0)
        {
            TagFieldSetString(Buf, 0x200, "PASS", &Buf[0x201]);
            TagFieldSetNumber(Buf, 0x200, "MASK", pApi->i1844);
        }
        else
        {
            Buf[0x200] = 0x7E;
            CryptSSC2StringEncrypt(&Buf[0x201], 0x1F, &Buf[0x201], &pApi->Key[0], 0x10, 0x10);
            TagFieldSetString(Buf, 0x200, "PASS", &Buf[0x200]);
        }
        pData = Buf;
    }
    if (iCode == KIND_SNAP)
    {
        iVal = TagFieldGetNumber(TagFieldFind(pData, "CHAN"), 0);
        if ((unsigned int)(iVal - 5) < 0x12)
            LobbyApiListFlush(pApi, -1);
        pApi->i1894 = 0;
        pApi->i1898 = pApi->i1898 + 1;
    }
    if (iCode == KIND_MOVE && LobbyApiListGetHash(pApi, 0) != NULL)
    {
        TagFieldGetString(TagFieldFind(pData, "NAME"), &Buf[0x220], 0x24, "");
        if (Buf[0x220] != 0)
        {
            void *pList = LobbyApiListGetDisp(pApi, 0);
            int iIndex = LobbyApiListFindByName(pApi, 0, &Buf[0x220]);

            if (iIndex != 0)
            {
                void *pUser = DispListGet(pList, iIndex);

                TagFieldDupl(Buf, 0x200, pData);
                TagFieldSetNumber(Buf, 0x200, "IDENT", *(int *)pUser);
                pData = Buf;
            }
        }
    }
    ProtoAriesSend(pApi->pAries, (void *)iCode, NULL, (void *)pData, -1);
    return pReq->uKind;
}

// cancela peticiones por uKind
int LobbyApiCancelCB(LobbyApiRefT *pApi, int iKind)
{
    LobbyApiRequestT *pReq;
    int iFound = 0;
    int i;

    if ((iKind & 0x1000) != 0)
        iFound = _LobbyApiCancelOneShotCB(pApi, iKind);
    else
    {
        i = pApi->iQueueHead;
        while (i != pApi->iQueueTail)
        {
            pReq = &pApi->Queue[i];
            if (pReq->uKind == (unsigned int)iKind || iKind == -1)
            {
                pReq->pCallback = NULL;
                iFound = 1;
            }
            i = (i + 1) % 32;
        }
    }
    return iFound;
}

// consulta un entero por nombre de 4 letras
int LobbyApiInfoInt(LobbyApiRefT *pApi, unsigned int uName)
{
    void *pList;
    int i;
    int iSum;

    if (uName == 0x6578746E)
        return pApi->iLoggedIn;
    if (uName == 0x686F7374)
    {
        if (pApi->pAries != 0)
            return ProtoAriesStatus(pApi->pAries, 0x61646472);
        return 0;
    }
    if (uName == 0x696E6174)
    {
        if (pApi->iLoggedIn != 0 && pApi->iLoginFail != 0)
            return pApi->iLoggedIn != pApi->iLoginFail;
        return 0;
    }
    if (uName == 0x6C636E74)
    {
        if (pApi->i2018 != 0)
            return pApi->i2020 + pApi->i2024;
        pList = LobbyApiListGetDisp(pApi, 0);
        iSum = -1;
        if (pList != NULL)
        {
            iSum = 0;
            for (i = 0; i < DispListCount(pList); i++)
                iSum += *(int *)((char *)DispListIndex(pList, i) + 0x10);
        }
        return iSum;
    }
    if (uName == 0x6C6F636C)
        return pApi->iLoginFail;
    if (uName == 0x6C6F676E)
        return pApi->Serv[8] != 0;
    if (uName == 0x6D6F7265)
        return pApi->iMore;
    if (uName == 0x72666C67)
        return pApi->iRflg;
    if (uName == 0x726F6F6D)
        return pApi->iRooms;
    if (uName == 0x73656375)
    {
        if (pApi->pAries != 0)
            return ProtoAriesStatus(pApi->pAries, 0x73656375);
        return 0;
    }
    if (uName == 0x736C6F74)
        return pApi->i189C;
    if (uName == 0x73746174)
        return pApi->uKind;
    if (uName == 0x7374696D)
        return pApi->iEpoch;
    if (uName == 0x73757370)
        return pApi->iSuspend;
    if (uName == 0x7469636B)
        return ProtoAriesTick(pApi->pAries);
    if (uName == 0x70696E67)
        return pApi->iIcmpPing;
    return -1;
}

// consulta un puntero por nombre de 4 letras
void *LobbyApiInfoPtr(LobbyApiRefT *pApi, unsigned int uName)
{
    if (uName == 0x61636374)                 /* acct */
        return &pApi->User158C;
    if (uName == 0x75736572)                 /* user */
        return &pApi->User158C;
    if (uName == 0x61757464)                 /* autd */
        return &pApi->X148[0];
    if (uName == 0x6C697374)                 /* list */
        return (void *)pApi->iListCreated;
    if (uName == 0x636F6E66)                 /* conf */
        return &pApi->Config[0];
    if (uName == 0x70617274)                 /* part */
        return &pApi->Party[0];
    if (uName == 0x70657264)                 /* perd */
        return &pApi->X348[0];
    if (uName == 0x70657273)                 /* pers */
        return &pApi->Serv[8];
    if (uName == 0x726F6F6D)                 /* room */
        return &pApi->Self17EC[0];
    if (uName == 0x73656C66)                 /* self */
        return &pApi->Serv[0];
    if (uName == 0x73657276)                 /* serv */
        return &pApi->i2018;
    if (uName == 0x76657273)                 /* vers */
        return TagFieldFind(&pApi->Account[0], "VERS");
    if (uName == 0x736C7573)                 /* slus */
        return TagFieldFind(&pApi->Account[0], "SLUS");
    return NULL;
}

// fija parametros de control
int LobbyApiControl(LobbyApiRefT *pApi, unsigned int uName, int iValue)
{
    int iOld = 0;

    if (uName == 0x646F626A)
        pApi->iData = iValue;
    if (uName == 0x6974696D)
    {
        iOld = pApi->iMaxCps;
        pApi->iMaxCps = iValue;
    }
    if (uName == 0x7274696D)
    {
        iOld = pApi->iMinCps;
        pApi->iMinCps = iValue;
    }
    return iOld;
}

// stub vacio
void LobbyApiDebug(void)
{
}

// el corazon: atiende el protocolo completo por frame
void LobbyApiUpdate(LobbyApiRefT *pApi)
{
    char Buf[0x100];
    LobbyApiRequestT DQ;
    LobbyApiMsgT Msg;
    int iKind;
    int iCode;
    char *pMsg;
    int iResult;
    int iStatus;
    int iTick;

    if (pApi->pAries == 0)
        return;
    if (pApi->uKind == KIND_INIT)
        _LobbyApiUpdateInit(pApi);
    iTick = ProtoAriesTick(pApi->pAries);
    _LobbyApiCheckTimeout(pApi, iTick);
    if (pApi->i1CC0 == 0)
        pApi->i1CC0 = iTick;
    pApi->i1CC4 += iTick - pApi->i1CC0;
    pApi->i1CC0 = iTick;
    pApi->iEpoch += pApi->i1CC4 / 1000;
    pApi->i1CC4 = pApi->i1CC4 % 1000;
    if (pApi->uKind == KIND_TERM)
        return;
    if (pApi->uKind == KIND_OFFL)
        return;
    _LobbyApiUpdatePing(pApi);
    for (;;)
    {
        iResult = ProtoAriesPeek(pApi->pAries, &iKind, &iCode, &pMsg);
        if (iResult < 0)
            break;
        pApi->iTimestamp = iTick + pApi->iMaxCps;
        if (iKind == KIND_PNG)
        {
            if (pApi->iIcmpPing != 0xFFFFFFFF)
            {
                TagFieldDupl(Buf, 0x100, pMsg);
                iResult = -1;
                TagFieldSetNumber(Buf, 0x100, "TIME", pApi->iIcmpPing);
                pMsg = Buf;
            }
            pApi->iEpoch = TagFieldGetEpoch(TagFieldFind(pMsg, "REF"), 0);
            pApi->i1CC0 = 0;
            pApi->i1CC4 = 0;
            iCode = 0;
            ProtoAriesSend(pApi->pAries, iKind, 0, pMsg, iResult);
        }
        else
        {
            _LobbyApiDequeueRequestCallback(pApi, &DQ, iKind);
            LobbyApiGlobalCallback(pApi, DQ.uKind, 0, iKind, iCode, pMsg);
            if (DQ.pCallback != 0)
            {
                Msg.iCode = DQ.uKind;
                Msg.uKind = 0;
                Msg.pData = (void *)iKind;
                Msg.pRef = (void *)iCode;
                Msg.p1 = pMsg;
                Msg.p2 = 0;
                DQ.pCallback(pApi, &Msg, DQ.pRef);
            }
            if (pApi->uKind == KIND_RDIR)
            {
                iKind = _LobbyApiUpdateRdir(pApi, iResult, iKind, iCode, pMsg);
                if (iKind < 0)
                    return;
            }
            if (iKind == -1 && iCode == -1)
            {
                iStatus = ProtoAriesStatus(pApi->pAries, KIND_LPTR);
                Buf[0] = 0;
                TagFieldSetAddress(Buf, 0x100, "ADDR", (void *)pApi->iLoginFail);
                TagFieldSetNumber(Buf, 0x100, "PORT", iStatus);
                ProtoAriesSend(pApi->pAries, KIND_ADDR, 0, Buf, -1);
                Buf[0] = 0;
                TagFieldSetBinary(Buf, 0x100, "SKEY", "Public Key", 10);
                ProtoAriesSend(pApi->pAries, KIND_SKEY, 0, Buf, -1);
                pApi->iMore = 1;
                pApi->uKind = KIND_SKEY;
                iKind = KIND_CONN;
                iCode = 0;
                LobbyApiRequestCB(pApi, KIND_NEWS, "NAME=7", _LobbyApiConfigCallback, 0);
            }
            if (pApi->uKind == KIND_SKEY && (iKind == pApi->uKind || iKind == KIND_SELE) && iCode == 0)
            {
                if (iKind == pApi->uKind)
                {
                    TagFieldGetBinary(TagFieldFind(pMsg, "SKEY"), pApi->Key, 0x10);
                    TagFieldGetString(TagFieldFind(pMsg, "DP"), pApi->Party, 0x40, "");
                }
                pApi->uKind = KIND_IDLE;
                iKind = KIND_CONN;
                iCode = 0;
                pApi->iPingServerTick = iTick;
            }
            if (iKind == -1 && iCode == 0xFEFEFEFE && pApi->uKind != KIND_CONN)
                LobbyApiDisconnect(pApi, KIND_SERV);
            if (iKind == KIND_AUTH && iCode == 0)
            {
                pApi->uKind = KIND_AUTH;
                TagFieldGetString(TagFieldFind(pMsg, "NAME"), pApi->User158C, 0x14, "");
                pApi->iLoggedIn = (int)TagFieldGetAddress(TagFieldFind(pMsg, "ADDR"), 0);
            }
            if (iKind == KIND_ACCT && iCode == 0)
                pApi->uKind = KIND_ACCT;
            if (iKind == KIND_SNAP)
            {
                pApi->i1898 -= 1;
                if (pApi->i1898 == 0 && iCode == 0)
                    pApi->i1894 = TagFieldGetNumber(TagFieldFind(pMsg, "CHAN"), 0);
            }
            if (iKind == KIND_AUTH)
                TagFieldDupl(pApi->X148, 0x200, pMsg);
            if (iKind == KIND_PERS)
            {
                TagFieldDupl(pApi->X348, 0x200, pMsg);
                if (iCode == 0)
                {
                    memset(pApi->Serv, 0, 0x244);
                    TagFieldGetString(TagFieldFind(pMsg, "PERS"), &pApi->Serv[8], 0x10, "");
                    LobbyApiExtractUserAddr(pApi->Serv, pMsg);
                    pApi->iPingServerTick = iTick;
                }
            }
            if (iKind == KIND_SELE && iCode == 0)
            {
                pApi->iMore = TagFieldGetNumber(TagFieldFind(pMsg, "MORE"), 0);
                pApi->i189C = TagFieldGetNumber(TagFieldFind(pMsg, "SLOTS"), 0);
                iStatus = TagFieldGetNumber(TagFieldFind(pMsg, "STATS"), 0);
                if (iStatus == 0)
                {
                    pApi->i201C = 0;
                    pApi->i2018 = 0;
                }
                else
                {
                    if (pApi->i2018 == 0)
                        pApi->i201C = 0;
                    pApi->i2018 = 1;
                }
            }
            if ((iKind >> 24) == '+')
                LobbyApiAsyncProcess(pApi, pMsg, iKind, iCode);
            if ((iKind == KIND_MOVE || iKind == KIND_ROOM) && iCode == 0)
                LobbyApiAsyncMoveRoom(pApi, pMsg, iKind, iKind);
            if ((iKind >> 24) == '$')
                LobbyApiGlobalCallback(pApi, 0, 4, iKind, iCode, pMsg);
        }
        ProtoAriesRecv(pApi->pAries, 0, 0, 0, 0);
    }
    LobbyApiListUpdate(pApi);
    if (pApi->iPingServerTick != 0 && (unsigned int)iTick >= (unsigned int)pApi->iPingServerTick)
    {
        _LobbyApiPingServer(pApi);
        pApi->iPingServerTick = iTick + 0x7530;
    }
    LobbyApiGlobalCallback(pApi, 0, 5, 0, 0, 0);
}
