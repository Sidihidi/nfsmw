// DirtySock: weboffer.c, las ofertas web (la tienda in-game sobre ProtoHttp).
// Escrito del asm de la A124 (libdirtysock.a(weboffer.o): .text 0x395410, 30
// etiquetas; la unidad solo enlaza en el prototipo). El .mdebug no trae esta
// lib: el unico oraculo es el asm + los datos (.rodata 0x489A..., .sdata
// 0x6546F8..0x6548C0).
//
// Medido del asm (r48):
// - WebOfferRefT mide 0x1240 B mas el buffer de descarga (Create2 lo zeroa y
//   reparte: +0 pBuffer, +4 iBufSize, +8 Result[0x100], +0x108 pAlloc, +0x210
//   la pagina TagField de ofertas (0x1000), +0x1210 iCursor, +0x1224 pHttp).
// - Los colores son 0xBBGGRR little-endian en el TagField, con el alpha (byte
//   alto) forzado a 0xFF si el tag no existe.
// - Los botones llevan prefijo: 'X' + nombre ('$next' etc.) o ' ' + texto.
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct WebOfferRefT WebOfferRefT;

extern void *NetAlloc(int iSize);
extern void NetFree(void *pMem);
extern int NetTick(void);
extern int ds_stricmp(const char *pA, const char *pB);
extern const char *ds_stristr(const char *pStr, const char *pFind);
extern void *TagFieldFind(const char *pRecord, const char *pName);
extern void *TagFieldFindIdx(const char *pRecord, const char *pName, int iIndex);
extern void *TagFieldGetAddress(const char *pField, int iDefault);
extern int TagFieldGetString(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault);
extern int TagFieldGetNumber(const char *pField, int iDefault);
extern char *TagFieldDupl(char *pDest, int iDestSize, const char *pSrc);
extern int TagFieldMerge(char *pDest, int iDestSize, const char *pSrc);
extern int TagFieldSetString(char *pBuffer, int iBufferSize, const char *pName, const char *pValue);
extern int TagFieldSetNumber(char *pBuffer, int iBufferSize, const char *pName, int iValue);
extern int TagFieldGetToken(const char *pField, const char *pTokens);
extern int TagFieldRename(char *pRecord, int iRecordSize, const char *pName, const char *pNewName);
extern void *ProtoHttpCreate(int iBufSize);
extern void ProtoHttpDestroy(void *pHttp);
extern void ProtoHttpUpdate(void *pHttp);
extern int ProtoHttpGet(void *pHttp, const char *pUrl, void *pX);
extern int ProtoHttpPost(void *pHttp, const char *pUrl, const char *pData, int iDataLen, void *pX);
extern int ProtoHttpRecv(void *pHttp, char *pBuffer, int iX, int iLen);
extern int ProtoHttpStatus(void *pHttp, unsigned int uKind);
extern int atoi(const char *pStr);

// las que define este fichero mas abajo
extern const char *_WebOfferFind(WebOfferRefT *pOffer, const char *pStart, const char *pName);
extern void _WebOfferUrl(char *pUrl, int iSize, const char *pBase);
extern void _WebOfferGoto(WebOfferRefT *pOffer, const char *pName, const char *pBase, const char *pX);
extern int WebOfferDestroy(WebOfferRefT *pOffer);

struct WebOfferRefT
{
    char *pBuffer;                  // +0x0000 (el buffer de descarga)
    int iBufSize;                   // +0x0004
    char Result[0x100];             // +0x0008 (el TagField de resultado)
    unsigned char Pad108[0x108 - 0x108];
    void *pAlloc;                   // +0x0108 (el NetAlloc del Create)
    char Parm[0x100];               // +0x010C (los $param de la pagina)
    int iCmd;                       // +0x020C (el comando en curso, 4 letras)
    char Page[0x1000];              // +0x0210 (la pagina de ofertas)
    unsigned char Pad1210[0x1210 - 0x1210];
    char *iCursor;                  // +0x1210 (posicion de la oferta actual)
    char *iHead;                    // +0x1214 (el $head del comando en curso)
    char *iTail;                    // +0x1218 (el $tail del comando en curso)
    char *i121C;                    // +0x121C
    char *i1220;                    // +0x1220
    void *pHttp;                    // +0x1224 (ProtoHttp)
    char *i1228;                    // +0x1228 (inicio del historial $home)
    int i122C;                      // +0x122C (espacio libre del buffer)
    char *i1230;                    // +0x1230 (modo rsrc: prefijo de chunk)
    int i1234;                      // +0x1234 (vencimiento del TIME)
    int i1238;                      // +0x1238 (vencimiento del poll de 1 s)
    int i123C;                      // +0x123C (el estado de la descarga)
    unsigned char Pad1240[0x1240 - 0x1240];
};

// saca un color 0xBBGGRR del tag pName (alpha 0xFF si falta)
void _WebOfferGetColor(WebOfferRefT *pOffer, unsigned char *pColor, const char *pName)
{
    unsigned int uColor = (unsigned int)TagFieldGetAddress(TagFieldFind(&pOffer->Page[0], pName), 0);

    pColor[0] = (unsigned char)(uColor >> 16);
    pColor[1] = (unsigned char)(uColor >> 8);
    pColor[3] = (unsigned char)(uColor >> 24);
    pColor[2] = (unsigned char)uColor;
    if (uColor == 0)
        return;
    if (pColor[3] == 0)
        pColor[3] = 0xFF;
}

// saca el boton iButton: 'X'+nombre si es accion, ' '+texto si no
void _WebOfferGetButton(WebOfferRefT *pOffer, char *pButton, int iButton)
{
    char Name[0x20];
    char Text[0x40];

    char c;

    sprintf(Name, "BTN%d", iButton);
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], Name), Text, 0x40, "");
    c = Text[0];
    if (c == 0)
        return;
    if (c == '^')
    {
        pButton[0] = c;
        TagFieldDupl(&pButton[1], 0x28, &Text[1]);
    }
    else
    {
        pButton[0] = ' ';
        TagFieldDupl(&pButton[1], 0x28, Text);
    }
}

// busca un tag por la pagina desde pStart; los nombres $head/$incl/$next/
// $tail/$last/$rsrc son las pseudotags del formato de pagina
const char *_WebOfferFind(WebOfferRefT *pOffer, const char *pStart, const char *pName)
{
    const char *pScan = pStart;
    const char *pResult = NULL;
    const char *p;
    char *pEnd;
    char c;
    int iIncl = 0;

    for (;;)
    {
        if (pScan == NULL)
            break;
        p = pScan;
        pScan = strstr(p, "%{");
        if (pScan == NULL)
            pScan = p + strlen(p);
        if (iIncl != 0 && ds_stricmp(pName, "$next") == 0)
        {
            pResult = pScan;
            break;
        }
        if (*pScan == 0)
            break;
        if (ds_stricmp(pName, "$head") == 0)
        {
            pResult = pScan;
            break;
        }
        if (ds_stricmp(pName, "$incl") == 0 && pScan[2] == '*')
        {
            pResult = pScan;
            break;
        }
        p = pScan;
        if (ds_stristr(pScan, pName) == pScan + 2 && pScan[2 + strlen(pName)] < 0x21)
        {
            pResult = pScan;
            break;
        }
        p = pScan;
        pScan = strstr(p, "%}");
        if (pScan == NULL)
        {
            pResult = pScan;
            break;
        }
        p = pScan;
        pScan = strchr(p + 1, '%');
        if (pScan != NULL)
            c = pScan[1];
        else
        {
            pScan = p + strlen(p);
            c = pScan[1];
        }
        if (c != '$')
        {
            iIncl = 1;
            if (ds_stricmp(pName, "$rsrc") == 0)
                break;
            continue;
        }
        if (pScan[10] != ':')
            break;
        p = pScan + (int)strtol(pScan + 2, &pEnd, 16) + 12;
        if (p >= pOffer->pBuffer + pOffer->iBufSize)
            break;
        if (ds_stricmp(pName, "$rsrc") == 0)
        {
            pResult = pScan;
            break;
        }
        pScan = p;
        iIncl = 1;
        if (ds_stricmp(pName, "$rsrc") == 0)
            break;
    }
    if (ds_stricmp(pName, "$last") != 0)
        pScan = pResult;
    return pScan;
}

// ejecuta un 'goto' del servidor
void _WebOfferActionGoto(WebOfferRefT *pOffer, const char *pName)
{
    if (ds_stricmp(pName, "$http") == 0)
    {
        pOffer->iCursor = pOffer->i1228;
        return;
    }
    if (ds_stricmp(pName, "$home") == 0)
    {
        if (pOffer->i1228 != 0)
        {
            const char *pPos = _WebOfferFind(pOffer, pOffer->i1228, "$last");

            memmove(pOffer->pBuffer, pOffer->i1228, pPos - pOffer->i1228 + 1);
            pOffer->i122C = pPos - pOffer->i1228;
        }
        pOffer->i1228 = 0;
        pOffer->iCursor = (char *)_WebOfferFind(pOffer, pOffer->pBuffer, "startup");
        if (pOffer->iCursor == 0)
            pOffer->iCursor = pOffer->pBuffer;
        return;
    }
    if (ds_stricmp(pName, "$quit") == 0)
    {
        pOffer->iCursor = 0;
        return;
    }
    if (strncmp(pName, "$exit=", 6) == 0)
    {
        pOffer->iCursor = 0;
        TagFieldDupl(&pOffer->Result[0], 0x100, pName + 6);
        return;
    }
    if (ds_stricmp(pName, "$next") == 0)
        return;
    if (pName[0] != 0)
        pOffer->iCursor = (char *)_WebOfferFind(pOffer, pOffer->pBuffer, pName);
}

// limpia la pagina actual (todo o hasta un tag)
void _WebOfferActionClear(WebOfferRefT *pOffer, const char *pName)
{
    char *iPos;
    char *iLast;
    char *iNext;

    if (ds_stricmp(pName, "$all") == 0)
    {
        if (pOffer->i1228 == 0)
            return;
        memmove(pOffer->pBuffer, pOffer->i1228, pOffer->i122C + 1);
        pOffer->iCursor = pOffer->pBuffer;
        pOffer->i1228 = 0;
    }
    else
    {
        iPos = (char *)_WebOfferFind(pOffer, pOffer->pBuffer, pName);
        iLast = (char *)_WebOfferFind(pOffer, iPos, "$next");
        iNext = (char *)_WebOfferFind(pOffer, iLast, "$last");
        if (iPos == 0 || iLast == 0 || iNext == 0)
            return;
        memmove(iPos, iLast, iNext - iLast + 1);
        pOffer->i1228 = 0;
        if (iPos < pOffer->iCursor)
        {
            if (pOffer->iCursor < iLast)
                pOffer->iCursor = iPos;
            else
                pOffer->iCursor = pOffer->iCursor - (iLast - iPos);
        }
    }
}

// salida con dato
void _WebOfferActionExit(WebOfferRefT *pOffer, const char *pData)
{
    TagFieldDupl(&pOffer->Result[0], 0x100, pData);
    pOffer->iCursor = 0;
}

// interpreta la accion de un boton: ';comando;nombre;...' separado por ';'
void _WebOfferAction(WebOfferRefT *pOffer, char *pAction)
{
    char *pWord;
    char *pName;

    while (*pAction != 0)
    {
        if (*pAction == ';')
        {
            *pAction = 0;
            pWord = pAction;
            while (*pWord <= ' ')
                pWord++;
            pName = pWord + 1;
            while (pName != pAction && *pName > ' ')
                pName++;
            *pName = 0;
            pName++;
            while (pName != pAction && *pName <= ' ')
                pName++;
            if (ds_stricmp(pWord, "goto") == 0)
                _WebOfferActionGoto(pOffer, pName);
            else if (ds_stricmp(pWord, "clear") == 0)
                _WebOfferActionClear(pOffer, pName);
            else if (ds_stricmp(pWord, "exit") == 0)
                _WebOfferActionExit(pOffer, pName);
        }
        pAction++;
    }
}

// resuelve el nombre de una pagina por tags encadenados y ejecuta la accion
void _WebOfferGoto(WebOfferRefT *pOffer, const char *pName, const char *pBase, const char *pX)
{
    char Buf[0x400];

    TagFieldGetString(TagFieldFind(&pOffer->Page[0], pX), Buf, 0x400, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], pBase), Buf, 0x400, Buf);
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], pName), Buf, 0x400, Buf);
    if (strchr(Buf, ';') != 0)
        _WebOfferAction(pOffer, Buf);
    else
        _WebOfferActionGoto(pOffer, Buf);
}

// sustituye los $tag$ de una URL in situ (limite iSize, tags de pBase)
void _WebOfferUrl(char *pUrl, int iSize, const char *pBase)
{
    char Name[0x80];
    char Value[0x80];
    char *pDollar;
    char *pEnd;
    char *pPrev;
    char *pScan;
    int iLen;

    while ((pDollar = strchr(pUrl, '$')) != 0 && (pEnd = strchr(pDollar + 1, '$')) != 0
           && (pPrev = pEnd - 1, iLen = pPrev - pDollar) < 0x80)
    {
        memcpy(Name, pDollar + 1, iLen);
        Name[iLen] = 0;
        TagFieldGetString(TagFieldFind(pBase, Name), Value, 0x80, "");
        TagFieldSetString(Name, 0x80, 0, Value);
        iLen = strlen(Name);
        while (iLen > 0 && Name[iLen - 1] <= ' ')
            Name[--iLen] = 0;
        pScan = pEnd;
        if (*pEnd != 0)
            while (*++pScan != 0)
                ;
        if ((pScan - pUrl) + iLen < iSize)
        {
            memmove(pDollar + iLen, pEnd + 1, pScan - pEnd);
            memcpy(pDollar, Name, iLen);
        }
        else
            return;
    }
}

// crea la referencia con buffer propio
WebOfferRefT *WebOfferCreate2(WebOfferRefT *pOffer, int iSize)
{
    int iUse = (0x989680 < iSize) ? 0x989680 : iSize;
    WebOfferRefT *pResult = NULL;

    if (iUse >= 0x1341)
    {
        pResult = pOffer;
        memset(pResult, 0, 0x1240);
        pResult->i123C = 0;
        pResult->iBufSize = iUse - 0x1240;
        pResult->pBuffer = (char *)pResult + 0x1240;
        pResult->pHttp = ProtoHttpCreate(0x1000);
        if (pResult->pHttp == NULL)
        {
            WebOfferDestroy(pResult);
            pResult = NULL;
        }
    }
    return pResult;
}

// crea con NetAlloc
WebOfferRefT *WebOfferCreate(int iSize)
{
    WebOfferRefT *pOffer = NetAlloc(0x1240 + iSize);
    WebOfferRefT *pResult = WebOfferCreate2(pOffer, 0x1240 + iSize);

    if (pResult != NULL)
        pResult->pAlloc = pOffer;
    return pResult;
}

// destruye (http + buffer); devuelve 0
int WebOfferDestroy(WebOfferRefT *pOffer)
{
    if (pOffer != NULL)
    {
        if (pOffer->pHttp != NULL)
            ProtoHttpDestroy(pOffer->pHttp);
        if (pOffer->pAlloc != NULL)
            NetFree(pOffer->pAlloc);
    }
    return 0;
}

// vacia la pagina actual
void WebOfferClear(WebOfferRefT *pOffer)
{
    pOffer->pBuffer[0] = 0;
    pOffer->iCursor = 0;
}

// configura la sesion
void WebOfferSetup(WebOfferRefT *pOffer, char *pSetup)
{
    pOffer->Result[0] = 0;
    TagFieldSetString(&pOffer->Result[0], 0x100, "PERS", pSetup);
    TagFieldSetString(&pOffer->Result[0], 0x100, "FAVT", &pSetup[0x14]);
    TagFieldSetString(&pOffer->Result[0], 0x100, "PROD", &pSetup[0x34]);
    TagFieldSetString(&pOffer->Result[0], 0x100, "PLAT", &pSetup[0x54]);
    TagFieldSetString(&pOffer->Result[0], 0x100, "LANG", &pSetup[0x74]);
    TagFieldSetString(&pOffer->Result[0], 0x100, "FROM", &pSetup[0x78]);
    TagFieldSetString(&pOffer->Result[0], 0x100, "SLUS", &pSetup[0x7C]);
    TagFieldSetString(&pOffer->Result[0], 0x100, "LKEY", &pSetup[0x9C]);
    TagFieldSetString(&pOffer->Result[0], 0x100, "", &pSetup[0xC4]);
}

// lanza la descarga: URL, tagfield literal o texto plano
int WebOfferExecute(WebOfferRefT *pOffer, const char *pCmd)
{
    char *pCursor;
    int iLen;

    WebOfferClear(pOffer);
    if (strncmp(pCmd, "http://", 7) == 0 || strncmp(pCmd, "https://", 8) == 0)
        sprintf(pOffer->pBuffer, "%%{ CMD=http SUCCESS-GOTO=\"$home\" URL-GET=%s\n%%}", pCmd);
    else if (ds_stristr(pCmd, "%{*") != NULL)
    {
        iLen = sprintf(pOffer->pBuffer, "%%{ CMD=http SUCCESS-GOTO=\"$home\" URL-FAKE=1\n%%}");
        strncpy(pOffer->pBuffer + iLen + 1, pCmd, pOffer->iBufSize - iLen - 1);
        pOffer->pBuffer[pOffer->iBufSize - 1] = 0;
    }
    else if ((unsigned int)strlen(pCmd) < (unsigned int)pOffer->iBufSize)
        strcpy(pOffer->pBuffer, pCmd);
    pCursor = (char *)_WebOfferFind(pOffer, pOffer->pBuffer, "startup");
    pOffer->iCursor = pCursor;
    if (pCursor == 0)
    {
        pCursor = pOffer->pBuffer;
        pOffer->iCursor = pCursor;
    }
    pOffer->Result[0] = 0;
    return (int)pCursor;
}

// copia el resultado al buffer del que llama y devuelve su atoi
int WebOfferResultData(WebOfferRefT *pOffer, char *pDest, int iSize)
{
    if (pDest != NULL)
        TagFieldDupl(pDest, iSize, &pOffer->Result[0]);
    return atoi(&pOffer->Result[0]);
}

// extrae y ejecuta el siguiente comando de la pagina (los 'goto' encadenan de 5)
int *WebOfferCommand(WebOfferRefT *pOffer)
{
    int iLen;
    char *p;
    int iCount;
    int *pCommand;

    pCommand = &pOffer->iCmd;
    if (pOffer == NULL)
        return NULL;
    iCount = 0;
    do
    {
        if (pOffer->iCursor == NULL)
            break;
        pOffer->iHead = (char *)_WebOfferFind(pOffer, pOffer->iCursor, "$head");
        pOffer->iTail = (char *)_WebOfferFind(pOffer, pOffer->iHead, "$tail");
        if (pOffer->iHead == NULL || pOffer->iTail == NULL)
            break;
        pOffer->iHead += 2;
        iLen = pOffer->iTail - pOffer->iHead;
        if ((unsigned int)(iLen - 1) >= 0xFFF)
            break;
        memcpy(pOffer->Page, pOffer->iHead, iLen);
        pOffer->Page[iLen] = 0;
        *pCommand = TagFieldGetToken(TagFieldFind(&pOffer->Page[0], "CMD"), NULL);
        if (*pCommand == 0)
            break;
        pOffer->iCursor = (char *)_WebOfferFind(pOffer, pOffer->iHead - 2, "$next");
        p = pOffer->iTail + 2;
        while (*p > 0 && *p <= ' ')
            p++;
        pOffer->i121C = p;
        pOffer->i1220 = pOffer->iCursor;
        if (p == pOffer->iCursor)
            goto setnull;
        if (pOffer->iCursor != NULL)
        {
            *pOffer->iCursor = 0;
        }
        else
        {
setnull:
            pOffer->i1220 = NULL;
            pOffer->i121C = NULL;
        }
        if (*pCommand != 'goto')
            return pCommand;
        _WebOfferGoto(pOffer, "GOTO", "", "DEFAULT-GOTO");
        iCount += 1;
    } while (iCount < 5);
    return NULL;
}

// la tabla de goto por boton (el 0 es el nombre vacio)
static char *_WebOffer_ActionGoto1[9] =
{
    "", "BTN1-GOTO", "BTN2-GOTO", "BTN3-GOTO", "BTN4-GOTO",
    "BTN5-GOTO", "BTN6-GOTO", "BTN7-GOTO", "BTN8-GOTO"
};

// accion del boton iAction (1..8): sustituye el tag y ejecuta su goto
void WebOfferAction(WebOfferRefT *pOffer, int iAction)
{
    const char *pLast;
    int iLen;
    int iDel;
    int iVal;

    if ((unsigned int)(iAction - 1) >= 8)
    {
        WebOfferClear(pOffer);
        return;
    }
    TagFieldSetNumber(&pOffer->Page[0], 0x1000, "ACTION", iAction);
    if (pOffer->i121C != pOffer->i1220)
    {
        *pOffer->i1220 = '%';
        pOffer->i121C = NULL;
        pOffer->i1220 = NULL;
    }
    iDel = pOffer->iTail - pOffer->iHead;
    iLen = strlen(&pOffer->Page[0]);
    pLast = _WebOfferFind(pOffer, pOffer->pBuffer, "$last");
    iVal = pLast - pOffer->pBuffer + 1;
    if (pOffer->iBufSize < iVal - iDel + iLen)
    {
        WebOfferClear(pOffer);
        return;
    }
    memmove(pOffer->iHead + iLen, pOffer->iTail, pOffer->pBuffer + iVal - pOffer->iTail);
    memcpy(pOffer->iHead, &pOffer->Page[0], iLen);
    _WebOfferGoto(pOffer, _WebOffer_ActionGoto1[iAction], "", "DEFAULT-GOTO");
}

// http de la oferta: prepara la peticion con INCL/PARM/TIME/DISP y la lanza
void WebOfferHttp(WebOfferRefT *pOffer)
{
    char Url[0x100];
    char Name[0x40];
    char Value[0x100];
    char *pIncl;
    char *pTail;
    char *pDot;
    char *pField;
    int iIdx;
    int iTime;
    int iDisp;
    int iStatus;
    int iVal;
    char *pPage;

    if (pOffer->iCmd != 'http')
    {
        WebOfferClear(pOffer);
        return;
    }
    pOffer->i123C = 5;
    pOffer->i1238 = NetTick() + 0xBB8;
    pOffer->i1234 = NetTick() + 0x3E8;
    pOffer->i1228 = (char *)_WebOfferFind(pOffer, pOffer->pBuffer, "$last") + 1;
    pOffer->i1230 = NULL;
    pOffer->i122C = pOffer->pBuffer + pOffer->iBufSize - pOffer->i1228 - 1;
    if (pOffer->i122C < 0x100)
    {
        WebOfferClear(pOffer);
        return;
    }
    pPage = &pOffer->Page[0];
    if (TagFieldGetNumber(TagFieldFind(pPage, "URL-FAKE"), 0) > 0)
    {
        pOffer->i123C = 1;
        return;
    }
    *pOffer->i1228 = 0;
    TagFieldMerge(pPage, 0x1000, &pOffer->Parm[0]);
    {
        char *pInclPage = pPage;
        TagFieldSetNumber(pInclPage, 0x1000, "MAXLEN", pOffer->i122C);
        if (TagFieldGetString(TagFieldFind(pInclPage, "INCL"), Name, 0x40, "") > 0)
        {
            pIncl = (char *)_WebOfferFind(pOffer, pOffer->pBuffer, Name);
            pTail = (char *)_WebOfferFind(pOffer, pIncl, "$tail");
            if (pIncl != NULL && pTail != NULL)
            {
                *pTail = 0;
                TagFieldMerge(pInclPage, 0x1000, pIncl + strlen(Name) + 2);
                *pTail = '%';
            }
        }
    }
    {
        char *pParmPage = &pOffer->Page[0];
        for (iIdx = 1;; iIdx += 1)
        {
            if (TagFieldGetString(TagFieldFindIdx(pParmPage, "PARM", iIdx), Name, 0x40, "") <= 0)
                break;
            pDot = strchr(Name, '.');
            if (pDot == NULL)
                break;
            *pDot = 0;
            pField = (char *)_WebOfferFind(pOffer, pOffer->pBuffer, Name);
            if (pField != NULL)
            {
                TagFieldGetString(TagFieldFind(pField + strlen(Name) + 2, pDot + 1), Value, 0x100, Name);
                *pDot = '.';
                TagFieldSetString(pParmPage, 0x1000, Name, Value);
            }
        }
        TagFieldGetString(TagFieldFind(pParmPage, "URL"), Url, 0x100, "");
        TagFieldGetString(TagFieldFind(pParmPage, "URL-GET"), Url, 0x100, Url);
        TagFieldGetString(TagFieldFind(pParmPage, "URL-POST"), Url, 0x100, Url);
        if (Url[0] == 0)
        {
            WebOfferClear(pOffer);
            return;
        }
        iVal = TagFieldGetNumber(TagFieldFind(pParmPage, "TIME"), 10);
        iTime = 2;
        if (iVal > 1)
            iTime = iVal;
        if (iTime > 60)
            iTime = 60;
        pOffer->i1234 = NetTick() + iTime * 1000;
        iVal = TagFieldGetNumber(TagFieldFind(pParmPage, "DISP"), 3);
        iDisp = 0;
        if (iVal >= 0)
            iDisp = iVal;
        if (iDisp > 60)
            iDisp = 60;
        pOffer->i1238 = NetTick() + iDisp * 1000;
        _WebOfferUrl(Url, 0x100, pParmPage);
        if (TagFieldFind(pParmPage, "URL-GET") == NULL)
            iStatus = ProtoHttpPost(pOffer->pHttp, Url, pParmPage, -1, NULL);
        else
            iStatus = ProtoHttpGet(pOffer->pHttp, Url, NULL);
    }
    if (iStatus < 0)
    {
        WebOfferClear(pOffer);
        return;
    }
    pOffer->i123C = 2;
}

// completion del http: exitos/fallos/timeout y el poll del cursor
int WebOfferHttpComplete(WebOfferRefT *pOffer)
{
    int iResult;

    WebOfferUpdate(pOffer);
    iResult = 0;
    if (pOffer->i123C == 4)
    {
        pOffer->i1228 = (char *)_WebOfferFind(pOffer, pOffer->pBuffer, "$last");
        *pOffer->i1228 = '\n';
        _WebOfferGoto(pOffer, "SUCCESS-GOTO", "", "DEFAULT-GOTO");
        pOffer->i1228 = NULL;
        pOffer->i123C = 0;
    }
    if (pOffer->i123C == 5)
    {
        _WebOfferGoto(pOffer, "FAILURE-GOTO", "", "DEFAULT-GOTO");
        pOffer->i123C = 0;
    }
    if (pOffer->i123C != 0)
    {
        if (pOffer->i1234 < (unsigned int)NetTick())
        {
            _WebOfferGoto(pOffer, "TIMEOUT-GOTO", "FAILURE-GOTO", "DEFAULT-GOTO");
            pOffer->i123C = 0;
        }
    }
    if (pOffer->i123C == 0 && pOffer->i1238 < (unsigned int)NetTick())
    {
        iResult = 1;
        if (pOffer->iCursor == NULL || *pOffer->iCursor == 0)
        {
            if (pOffer->Result[0] == 0)
                iResult = -1;
        }
    }
    return iResult;
}

// update por frame: la maquina de estados de la descarga
void WebOfferUpdate(WebOfferRefT *pOffer)
{
    char Url[0x100];
    char *pIncl;
    char *pNext;
    char *pLast;
    char *pTag;
    int iStatus;
    int iLen;
    int iCount;

    if (pOffer->i123C == 2)
    {
        int iData;

        ProtoHttpUpdate(pOffer->pHttp);
        iData = ProtoHttpStatus(pOffer->pHttp, 'data');
        if (iData < 0)
            pOffer->i123C = 5;
        else if (iData > 0)
            pOffer->i123C = (ProtoHttpStatus(pOffer->pHttp, 'code') == 200) ? 3 : 5;
    }
    if (pOffer->i123C == 3)
    {
        do
        {
            ProtoHttpUpdate(pOffer->pHttp);
            iLen = ProtoHttpRecv(pOffer->pHttp, pOffer->i1228, 0, pOffer->i122C);
            if (iLen > 0)
            {
                if (pOffer->i1230 == NULL)
                {
                    int i, j;

                    if ((int)pOffer->i1230 < iLen)
                    {
                        for (i = 0, j = 0; i < iLen; i += 1)
                        {
                            if (pOffer->i1228[i] != '\r')
                                pOffer->i1228[j++] = pOffer->i1228[i];
                        }
                        iLen = j;
                    }
                }
                else
                    sprintf(pOffer->i1230 - 0xA, "%08x:", pOffer->i1228 - pOffer->i1230 + iLen);
                pOffer->i1228 += iLen;
                pOffer->i122C -= iLen;
                *pOffer->i1228 = 0;
                if (pOffer->i122C <= 0)
                    pOffer->i123C = 5;
            }
        } while (iLen > 0 && pOffer->i123C == 3);
        if (iLen == -1)
            pOffer->i123C = 4;
        else if (iLen < 0)
            pOffer->i123C = 5;
    }
    if (pOffer->i123C == 1)
    {
        iLen = strlen(pOffer->i1228);
        pOffer->i123C = 4;
        pOffer->i1228 += iLen;
        pOffer->i122C -= iLen;
    }
    if (pOffer->i123C != 4)
        return;
    pLast = (char *)_WebOfferFind(pOffer, pOffer->pBuffer, "$last");
    pIncl = (char *)_WebOfferFind(pOffer, pLast + 1, "$incl");
    pNext = (char *)_WebOfferFind(pOffer, pIncl, "$next");
    if (pIncl == NULL || pNext == NULL)
        return;
    iLen = pNext - pIncl;
    if (iLen >= pOffer->i122C)
        return;
    if (ds_stristr(pIncl, "URL=") == NULL)
        return;
    memmove(pOffer->i1228, pIncl, iLen);
    memmove(pIncl, pNext, pOffer->i1228 - pIncl);
    *pOffer->i1228 = 0;
    pTag = pOffer->i1228 - iLen;
    TagFieldGetString(TagFieldFind(pTag, "URL"), Url, 0x100, "");
    iStatus = TagFieldGetNumber(TagFieldFind(pTag, "TIME"), 0);
    TagFieldRename(pTag, pNext - pTag, "URL", "UUU");
    pOffer->i123C = 5;
    if (Url[0] == 0)
        return;
    if (pOffer->i122C < 0x101)
        return;
    iLen = sprintf(pOffer->i1228, "%%$00000000:") + 1;
    pOffer->i1230 = pOffer->i1228 + iLen;
    pOffer->i122C -= iLen;
    pOffer->i1228 += iLen;
    _WebOfferUrl(Url, 0x100, &pOffer->Parm[0]);
    if (ProtoHttpGet(pOffer->pHttp, Url, NULL) < 0)
        return;
    pOffer->i1234 += iStatus * 1000;
    pOffer->i123C = 2;
}

// rellena la estructura de ocupado (0x471 B)
void WebOfferGetBusy(WebOfferRefT *pOffer, char *pBusy)
{
    memset(pBusy, 0, 0x471);
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "IDENT"), pBusy, 0x40, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "NAME"), &pBusy[0x40], 0x400, "PLEASE WAIT");
    _WebOfferGetColor(pOffer, (unsigned char *)&pBusy[0x440], "MESG-RGB");
    _WebOfferGetButton(pOffer, &pBusy[0x444], 1);
}

static char DefaultButton[] = "OK";

// rellena la estructura de alerta (0x518 B; boton por defecto "OK")
void WebOfferGetAlert(WebOfferRefT *pOffer, char *pAlert)
{
    memset(pAlert, 0, 0x518);
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "TITLE"), pAlert, 0x40, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "IMAGE"), &pAlert[0x40], 0x20, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "MESG"), &pAlert[0x60], 0x400, "");
    _WebOfferGetColor(pOffer, (unsigned char *)&pAlert[0x460], "MESG-RGB");
    _WebOfferGetButton(pOffer, &pAlert[0x464], 1);
    _WebOfferGetButton(pOffer, &pAlert[0x491], 2);
    _WebOfferGetButton(pOffer, &pAlert[0x4BE], 3);
    _WebOfferGetButton(pOffer, &pAlert[0x4EB], 4);
    if (pAlert[0x465] == 0)
    {
        char c0 = DefaultButton[0];
        char c1 = DefaultButton[1];
        char c2 = DefaultButton[2];

        pAlert[0x465] = c0;
        pAlert[0x466] = c1;
        pAlert[0x467] = c2;
        pAlert[0x464] = 0x20;
    }
}

// rellena la estructura de credito (0x924 B)
void WebOfferGetCredit(WebOfferRefT *pOffer, char *pCredit)
{
    char Buf[0x20];

    memset(pCredit, 0, 0x924);
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "TITLE"), pCredit, 0x40, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "MESG"), &pCredit[0x40], 0x400, "");
    _WebOfferGetColor(pOffer, (unsigned char *)&pCredit[0x440], "MESG-RGB");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "FIRSTNAME"), &pCredit[0x4F8], 0x64, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "LASTNAME"), &pCredit[0x55C], 0x64, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "NAMEONCARD"), &pCredit[0x5C0], 0xC8, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "ADDRESS1"), &pCredit[0x688], 0x64, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "ADDRESS2"), &pCredit[0x6EC], 0x64, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "CITY"), &pCredit[0x750], 0x64, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "STATE"), &pCredit[0x7B4], 0x28, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "POSTCODE"), &pCredit[0x7DC], 0x28, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "COUNTRY"), &pCredit[0x808], 0x32, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "EMAIL"), &pCredit[0x83A], 0x64, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "CARDTYPE"), &pCredit[0x8A4], 0x50, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "CARDNUMBER"), &pCredit[0x8F4], 0x28, "");
    *(char **)&pCredit[0x8A0] = TagFieldFind(&pOffer->Page[0], "CARDLIST");
    if (*(char **)&pCredit[0x8A0] == 0)
        *(char **)&pCredit[0x8A0] = "UNKNOWN";
    *(char **)&pCredit[0x804] = TagFieldFind(&pOffer->Page[0], "COUNTRYL");
    if (*(char **)&pCredit[0x804] == 0)
        *(char **)&pCredit[0x804] = "UNKNOWN";
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "EXPIRYMONTH"), Buf, 0x20, "1");
    *(int *)&pCredit[0x91C] = atoi(Buf);
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "EXPIRYYEAR"), Buf, 0x20, "2004");
    *(int *)&pCredit[0x920] = atoi(Buf);
    _WebOfferGetButton(pOffer, &pCredit[0x444], 1);
    _WebOfferGetButton(pOffer, &pCredit[0x471], 2);
    _WebOfferGetButton(pOffer, &pCredit[0x49E], 3);
    _WebOfferGetButton(pOffer, &pCredit[0x4CB], 4);
}

// fija el credito
void WebOfferSetCredit(WebOfferRefT *pOffer, char *pCredit)
{
    TagFieldSetString(&pOffer->Page[0], 0x1000, "FIRSTNAME", &pCredit[0x4F8]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "LASTNAME", &pCredit[0x55C]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "NAMEONCARD", &pCredit[0x5C0]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "ADDRESS1", &pCredit[0x688]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "ADDRESS2", &pCredit[0x6EC]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "CITY", &pCredit[0x750]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "STATE", &pCredit[0x7B4]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "POSTCODE", &pCredit[0x7DC]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "COUNTRY", &pCredit[0x808]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "EMAIL", &pCredit[0x83A]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "CARDTYPE", &pCredit[0x8A4]);
    TagFieldSetString(&pOffer->Page[0], 0x1000, "CARDNUMBER", &pCredit[0x8F4]);
    TagFieldSetNumber(&pOffer->Page[0], 0x1000, "EXPIRYMONTH", *(int *)&pCredit[0x91C]);
    TagFieldSetNumber(&pOffer->Page[0], 0x1000, "EXPIRYYEAR", *(int *)&pCredit[0x920]);
}

// rellena la estructura de promo (0x600 B)
void WebOfferGetPromo(WebOfferRefT *pOffer, char *pPromo)
{
    memset(pPromo, 0, 0x600);
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "IDENT"), pPromo, 0x40, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "NAME"), &pPromo[0x40], 0x400, "");
    _WebOfferGetColor(pOffer, (unsigned char *)&pPromo[0x440], "BGC");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "PROMO"), &pPromo[0x4F8], 0xC8, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "K-TITLE"), &pPromo[0x5C0], 0x40, "");
    _WebOfferGetButton(pOffer, &pPromo[0x444], 1);
    _WebOfferGetButton(pOffer, &pPromo[0x471], 2);
    _WebOfferGetButton(pOffer, &pPromo[0x49E], 3);
    _WebOfferGetButton(pOffer, &pPromo[0x4CB], 4);
}

// fija la promo
void WebOfferSetPromo(WebOfferRefT *pOffer, char *pPromo)
{
    TagFieldSetString(&pOffer->Page[0], 0x1000, "PROM", &pPromo[0x4F8]);
}

// rellena la estructura de noticias (0x11C B)
int WebOfferGetNews(WebOfferRefT *pOffer, char *pNews)
{
    memset(pNews, 0, 0x11C);
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "TITLE"), pNews, 0x40, "");
    TagFieldGetString(TagFieldFind(&pOffer->Page[0], "IMAGE"), &pNews[0x40], 0x20, "");
    _WebOfferGetColor(pOffer, (unsigned char *)&pNews[0x114], "NEWS-RGB");
    _WebOfferGetButton(pOffer, &pNews[0x60], 1);
    _WebOfferGetButton(pOffer, &pNews[0x8D], 2);
    _WebOfferGetButton(pOffer, &pNews[0xBA], 3);
    _WebOfferGetButton(pOffer, &pNews[0xE7], 4);
    return (int)pOffer->i121C;
}
