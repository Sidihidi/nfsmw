// DirtySock: lobbytagfield.c, los campos con nombre ("etiqueta=valor") de los mensajes.
// Escrito del asm de la A124 (libdirtysock.a(lobbytagfield.o), .text 0x48B900, 0x2BC4):
// la biblioteca solo enlaza en el prototipo; SLUS no la tiene. Un registro es una
// cadena de campos "nombre=valor" separados por '\n' (TagFieldSep); el valor admite
// comillas ('"..."') y escapes %XX. Los nombres casan sin mayusculas (TagFieldUpper).
#include "lobbytagfield.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static int *_SecsToTime(int *pTm, unsigned int uSecs);
static unsigned int _TimeToSecs(const int *pTm);

// clases de caracteres (rodata 0x588D00 de la A124)
const unsigned char TagFieldCharClass[256] = "\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x0e\x04\x07\x04\x04\x07\x04\x04\x04\x04\x04\x04\x0c\x04\x04\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02";

// valor de cada digito hexadecimal, 0x80 si no es digito (rodata 0x588E10)
const unsigned char TagFieldHexVal[256] = "\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x80\x80\x80\x80\x80\x80\x80\x0a\x0b\x0c\x0d\x0e\x0f\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x0a\x0b\x0c\x0d\x0e\x0f\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80";

// mayusculas: la comparacion de nombres no distingue 'a'-'z' (rodata 0x588F10)
const unsigned char TagFieldUpper[256] = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x7b\x7c\x7d\x7e\x7f\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff";

// digitos hexadecimales en minuscula, para codificar (rodata 0x588E00)
const char TagFieldHexDigits[16] = "0123456789abcdef";

// alfabeto de 32 simbolos de los binarios codificados (rodata 0x589010)
const char TagFieldB32Digits[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ0123-";

// valor de cada simbolo codificado, 0xFF si no es simbolo (rodata 0x589038)
const signed char TagFieldB32Val[256] = "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x1f\xff\xff\x1b\x1c\x1d\x1e\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\xff\xff\xff\xff\xff\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

// dias de cada mes, ano bisiesto y ano normal (rodata 0x589138)
const int TagFieldMonthDays[24] = {
    31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

// separador de campos y terminador: variables en sdata (0x654670) porque el
// codigo original las lee por gp; con el -G0 de configure.py caen en .data
char TagFieldSep = '\n';
int TagFieldDoTerm = 1;

// cadena vacia de 8 B en sdata (0x654678), el nombre nulo de _TagFieldSetupAppend
char TagFieldNull[8] = "";

// escala de la parte fraccionaria de los flotantes, sdata 0x654080
float TagFieldFloatScale = 10000.0f;

// lee un entero decimal de pStr (maximo iMaxDigits digitos, 256 si no es
// positivo); devuelve donde paro y el valor en *pValue (0 si no hay digitos)
static const char *_ParseNumber(const char *pStr, int *pValue, int iMaxDigits)
{
    if (iMaxDigits <= 0)
        iMaxDigits = 256;
    *pValue = 0;
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x83A0F0): el digito se prueba sobre el byte con signo (cmp cl / jl, jg);
    // la A124 (otra version del DirtySDK) resta '0' y compara sin signo.
    {
        char c;
        for (c = *pStr; c >= '0' && c <= '9' && iMaxDigits > 0; c = *++pStr, iMaxDigits--)
            *pValue = *pValue * 10 + (c & 0x0F);
    }
#else
    {
        unsigned char c;
        for (c = *pStr; (unsigned int)(c - '0') < 10 && iMaxDigits > 0; c = *++pStr, iMaxDigits--)
            *pValue = *pValue * 10 + (c & 0x0F);
    }
#endif
    return pStr;
}

// prepara pBuffer para anadir pName: borra el campo si ya existe y devuelve
// donde escribir el valor (o NULL si no cabe: hacen falta el separador, el
// nombre, iReserve bytes de valor, el '=' y el cierre). Con pName nulo vacia.
char *_TagFieldSetupAppend(char *pBuffer, int iBufferSize, const char *pName, int iReserve)
{
    int iSep;
    int iEquals = 1;
    int iNameLen;
    unsigned char *pEnd = (unsigned char *)pBuffer;
    if (pBuffer == NULL)
        return NULL;
    if (*pEnd == '=')
        return NULL;
    if (pName == NULL) {
        *pBuffer = '\0';
        pName = TagFieldNull;
        iEquals = 0;
    }
    iNameLen = 0;
    if (*pName != '\0') {
        do {
            if (*pEnd != '\0')
                pEnd += 1;
            iNameLen += 1;
        } while (pName[iNameLen] != '\0');
    }
    while (*pEnd != '\0') {
        // busca el '=' del campo que ya existe
        if (*pEnd == '=') {
            unsigned char *pCand;
            if (*(pEnd - 1) <= 0x20)
                return NULL;
            pCand = pEnd - iNameLen;
            if (pCand == (unsigned char *)pBuffer || *(pCand - 1) <= 0x20) {
                const unsigned char *pN = (const unsigned char *)pName;
                unsigned char *pC = pCand;
                while (TagFieldUpper[*pC] == TagFieldUpper[*pN]) {
                    pC++;
                    pN++;
                }
                if (*pN == '\0')
                    break;
            }
        }
        pEnd++;
    }
    if (*pEnd != '\0') {
        // coincide: borra "nombre=valor" compactando encima del inicio
        unsigned char *pScan = pEnd;
        unsigned char *pSpace = NULL;
        for (;;) {
            if (*pScan < 0x20)
                break;
            if (*pScan == ' ')
                pSpace = pScan;
            if (*pScan == '=' && pSpace != NULL) {
                pScan = pSpace;
                break;
            }
            pScan++;
        }
        while (*pScan < 0x20 && *pScan != '\0')
            pScan++;
        pEnd = pEnd - iNameLen;
        while ((*pEnd = *pScan) != '\0') {
            pScan++;
            pEnd++;
        }
    }
    iSep = (pEnd != (unsigned char *)pBuffer && *(pEnd - 1) >= 0x20 && *(pEnd - 1) != TagFieldSep);
    if ((pBuffer + iBufferSize) - (char *)pEnd < iSep + iNameLen + (iReserve + 1) + TagFieldDoTerm + 1)
        return NULL;
    if (iSep != 0)
        *pEnd++ = TagFieldSep;
    while (*pName != '\0')
        *pEnd++ = *pName++;
    if (iEquals != 0)
        *pEnd++ = '=';
    *pEnd = '\0';
    return (char *)pEnd;
}

// deshace un SetupAppend fallido: recorta pBuffer hasta el campo anterior
static int _TagFieldSetupCancel(char *pBuffer, char *pPos)
{
    // retrocede hasta el control anterior; un '\n' (el separador) se conserva
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x83A500): el '\n' se relee de memoria (cmp byte ptr [eax], 0xa);
    // la A124 fusiona las dos lecturas en un solo registro (lbu + xori).
    while (pPos != pBuffer) {
        pPos -= 1;
        if ((unsigned char)*pPos < 0x20) {
            if (*pPos == '\n')
                pPos += 1;
            break;
        }
    }
#else
    while (pPos != pBuffer) {
        unsigned char c = *--pPos;
        if (c < 0x20) {
            if (c == '\n')
                pPos += 1;
            break;
        }
    }
#endif
    *pPos = '\0';
    return -1;
}

// cierra un valor anadido con SetupAppend: '\n' de separacion y NUL final;
// devuelve la longitud del registro
static int _TagFieldSetupTerm(char *pBuffer, char *pEnd, const char *pName)
{
    if (TagFieldDoTerm != 0) {
        if (pName != NULL) {
            *pEnd = TagFieldSep;
            pEnd += 1;
        }
    }
    *pEnd = '\0';
    return pEnd - pBuffer;
}

// borra el campo pName de pBuffer compactando el resto encima
int TagFieldDelete(char *pBuffer, const char *pName)
{
    char *pValue = TagFieldFind(pBuffer, pName);
    unsigned char *pDst;
    unsigned char *pScan;
    unsigned char *pCopy;
    if (pValue == NULL)
        return -1;
    // inicio del campo: retrocede hasta el blanco anterior
    pDst = (unsigned char *)pValue;
    while (pDst != (unsigned char *)pBuffer && *(pDst - 1) > ' ')
        pDst -= 1;
    // fin del valor: hasta el proximo '=' (el siguiente campo) o un control;
    // con '=' se copia desde el ultimo espacio (el valor no trae espacios)
    pScan = (unsigned char *)pValue;
    pCopy = (unsigned char *)pValue;
    while (*pScan >= ' ' && *pScan != '=') {
        if (*pScan == ' ')
            pCopy = pScan;
        pScan += 1;
    }
    if (*pScan != '=')
        pCopy = pScan;
    while (*pCopy > 0 && *pCopy <= ' ')
        pCopy += 1;
    while (*pCopy != '\0')
        *pDst++ = *pCopy++;
    while (pDst != (unsigned char *)pBuffer && *(pDst - 1) <= ' ')
        pDst -= 1;
    *pDst = '\0';
    return 0;
}

// cambia el nombre de un campo ajustando el hueco (el nuevo puede ser mas
// largo o mas corto)
int TagFieldRename(char *pBuffer, int iBufferSize, const char *pOldName, const char *pNewName)
{
    char *pValue = TagFieldFind(pBuffer, pOldName);
    unsigned char *pEnd = (unsigned char *)pValue;
    unsigned char *pStart;
    int iDiff;
    if (pValue == NULL)
        return -1;
    if (*(pEnd - 1) == ' ')
        pEnd -= 1;
    pStart = pEnd -= 1;
    while (pStart != (unsigned char *)pBuffer && *(pStart - 1) > 0x20)
        pStart -= 1;
    iDiff = strlen(pNewName) - (pEnd - pStart);
    if (iDiff < 0) {
        while (*pEnd != '\0') {
            *(pEnd + iDiff) = *pEnd;
            pEnd += 1;
        }
        *(pEnd + iDiff) = '\0';
    }
    if (iDiff > 0) {
        unsigned char *pTail = pEnd;
        if (*pEnd != '\0') {
            do {
                pTail += 1;
            } while (*pTail != '\0');
        }
        if (pTail - (unsigned char *)pBuffer >= iBufferSize - iDiff)
            return -1;
        pTail += 1;
        if (pTail >= pEnd) {
            do {
                *(pTail + iDiff) = *pTail;
                pTail -= 1;
            } while (pTail >= pEnd);
        }
    }
    while (*pNewName != '\0') {
        *pStart++ = *pNewName++;
    }
    return 0;
}

// copia la cadena pField en pBuffer (truncando a iBufferSize); devuelve lo copiado
int TagFieldDupl(char *pBuffer, int iBufferSize, const char *pField)
{
    int iLeft = iBufferSize;
    while (iLeft > 1 && *pField != '\0') {
        *pBuffer++ = (unsigned char)*pField++;
        iLeft -= 1;
    }
    if (iLeft > 0)
        *pBuffer = '\0';
    return iBufferSize - iLeft;
}

// busca "pName=valor" en el registro (sin mayusculas) y devuelve el valor
char *TagFieldFind(const char *pRecord, const char *pName)
{
    const unsigned char *pBase = (const unsigned char *)pName;
    const unsigned char *pScan = (const unsigned char *)pRecord;
    const unsigned char *pCur;
    unsigned char iDiff = 0;
    int iLen;
    if (pScan == NULL || pBase == NULL || *pBase == '\0')
        return NULL;
    pCur = pBase;
    // el primer campo se prueba directo
    do {
        unsigned char cRec;
        if (*pScan == '\0')
            return NULL;
        cRec = *pScan;
        iDiff |= TagFieldUpper[cRec] ^ TagFieldUpper[*pCur];
        pScan += 1;
        pCur += 1;
    } while (*pCur != '\0');
    iLen = pCur - pBase;
    if (iDiff == 0) {
        if (*pScan == '=')
            return (char *)pScan + 1;
    }
    if (*pScan != '\0') {
        pScan += 1;
        while (*pScan != '\0') {
            // en cada '=' se prueba el nombre que acaba ahi
            if (*pScan == '=') {
                const unsigned char *pCand;
                if (*(pScan - 1) <= 0x20)
                    return NULL;
                pCand = pScan - iLen;
                {
                    const unsigned char *pN = pBase;
                    const unsigned char *pC = pCand;
                    if (*(pCand - 1) <= 0x20) {
                        while (TagFieldUpper[*pC] == TagFieldUpper[*pN]) {
                            pC += 1;
                            pN += 1;
                        }
                        if (pC == pScan)
                            return (char *)pScan + 1;
                    }
                }
            }
            pScan += 1;
        }
    }
    return NULL;
}

// busca "pName<iIndex>=valor" (el nombre con el indice pegado)
char *TagFieldFindIdx(const char *pRecord, const char *pName, int iIndex)
{
    char szName[0x100];
    sprintf(szName, "%s%d", pName, iIndex);
    return TagFieldFind(pRecord, szName);
}

// guarda una direccion IPv4 como "a.b.c.d"
int TagFieldSetAddress(char *pBuffer, int iBufferSize, const char *pName, unsigned int uAddr)
{
    unsigned char oct[4];
    unsigned char szAddr[16];
    unsigned char *pDst;
    char *pPos;
    int i;
    oct[3] = (unsigned char)uAddr;
    uAddr >>= 8;
    oct[2] = (unsigned char)uAddr;
    uAddr >>= 8;
    oct[1] = (unsigned char)uAddr;
    uAddr >>= 8;
    oct[0] = (unsigned char)uAddr;
    pDst = szAddr;
    for (i = 0; i < 4; i += 1) {
        unsigned char u = oct[i];
        if (i > 0)
            *pDst++ = '.';
        // con centenas las decenas van siempre (105 -> "105")
        if (u > 9) {
            if (u > 99) {
                *pDst++ = u / 100 + '0';
                u %= 100;
            }
            *pDst++ = u / 10 + '0';
            u %= 10;
        }
        *pDst++ = u + '0';
    }
    *pDst = '\0';
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, (pDst - szAddr));
    if (pPos == NULL)
        return -1;
    for (pDst = szAddr; *pDst != '\0'; )
        *pPos++ = *pDst++;
    return _TagFieldSetupTerm(pBuffer, pPos, pName);
}

// lee una direccion IPv4 en "a.b.c.d" (0 si no hay digitos, uDefault si nulo)
unsigned int TagFieldGetAddress(const char *pField, unsigned int uDefault)
{
    unsigned int uAddr = 0;
    if (pField == NULL)
        return uDefault;
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x83AF60): el digito se prueba sobre el byte con signo (cmp dl / jl, jg) y el
    // punto reutiliza ese mismo byte; la A124 (otra version del DirtySDK) resta '0' y compara sin signo.
    for (;; pField++) {
        char c = *pField;
        if (c >= '0' && c <= '9')
            uAddr = (uAddr & 0xFFFFFF00) | ((uAddr & 0xFF) * 10 + (c & 0x0F));
        else if (c == '.')
            uAddr <<= 8;
        else
            break;
    }
#else
    for (;; pField++) {
        unsigned char c = *pField;
        if ((unsigned int)(c - '0') < 10)
            uAddr = (uAddr & 0xFFFFFF00) | ((uAddr & 0xFF) * 10 + (c & 0x0F));
        else if ((char)c == '.')
            uAddr <<= 8;
        else
            break;
    }
#endif
    return uAddr;
}

// guarda un token de 4 bytes con los raros escapados como %XX
int TagFieldSetToken(char *pBuffer, int iBufferSize, const char *pName, int iToken)
{
    unsigned char szTok[16];
    unsigned char *pW = szTok + 15;
    char *pPos;
    unsigned int uToken = (unsigned int)iToken;
    unsigned char cPct = '%';
    int i;
    *pW = '\0';
    for (i = 3; i >= 0; i -= 1) {
        unsigned char b = (unsigned char)uToken;
        uToken >>= 8;
        if (TagFieldCharClass[b] & 2) {
            pW -= 1;
            *pW = TagFieldHexDigits[b & 0x0F];
            pW -= 1;
            *pW = TagFieldHexDigits[(b >> 4) & 0x0F];
            pW -= 1;
            *pW = cPct;
        } else {
            pW -= 1;
            *pW = b;
        }
    }
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, (szTok + 16) - pW - 1);
    if (pPos == NULL)
        return -1;
    while (*pW != '\0')
        *pPos++ = *pW++;
    return _TagFieldSetupTerm(pBuffer, pPos, pName);
}

// lee un token de 4 bytes con escapes %XX (iDefault si nulo o vacio)
int TagFieldGetToken(const char *pField, int iDefault)
{
    int iTok = 0;
    int i;
    if (pField == NULL || (unsigned char)*pField <= 0x20)
        return iDefault;
    for (i = 0; i < 4; i += 1) {
        // cuatro caracteres; al acabarse el campo se rellena con espacios
        unsigned char c = *pField++;
        if (c < 0x20) {
            pField -= 1;
            c = ' ';
        }
        if (c == '%') {
            c = TagFieldHexVal[(unsigned char)*pField++] << 4;
            c |= TagFieldHexVal[(unsigned char)*pField++];
        }
        iTok = (iTok << 8) | c;
    }
    return iTok;
}

// fusiona el registro pRecord en pBuffer (los campos de pRecord ganan)
int TagFieldMerge(char *pBuffer, int iBufferSize, const char *pRecord)
{
    char *pScan;
    char *pWrite;
    char *pField;
    char *pLimit = pBuffer + iBufferSize;
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x83C650): el compilador guarda una copia del puntero de registro en el
    // prologo (mov edx,ecx + spill) y la usa en el bucle de anexado final (la guarda del
    // while la hace sobre el registro original); la A124 usa un solo puntero.
    const char *pSrc = pRecord;
#endif
    int iLen;
    if (pRecord == NULL || *pRecord == '\0')
        return 0;
    pField = pBuffer;
    pWrite = pBuffer;
    pScan = pBuffer;
    for (;;) {
        unsigned char c = *pScan;
        if (c == '\0')
            break;
        if (c < 0x21) {
            *pWrite++ = c;
            pScan += 1;
            pField = pScan;
            continue;
        }
        if (c != '=') {
            *pWrite++ = c;
            pScan += 1;
            continue;
        }
        if (pScan == pField) {
            *pScan = '\0';
            *pWrite = '\0';
            break;
        }
        {
            char save = *pScan;
            char *pFound;
            *pScan = '\0';
            *pWrite = '\0';
            iLen = pScan - pField;
            pFound = TagFieldFind(pRecord, pField);
            if (pFound == NULL) {
                *pScan = save;
                *pWrite = save;
                pScan += 1;
                pWrite += 1;
                continue;
            }
            pScan += 1;
            pWrite -= iLen;
            if (*pScan == '\0') {
                pField = pScan;
                break;
            }
            if (*pScan != '=') {
                do {
                    pScan += 1;
                } while (*pScan != '\0' && *pScan != '=');
                if (*pScan == '\0') {
                    pField = pScan;
                    break;
                }
            }
            pField = pScan;
            while ((unsigned char)*(pScan - 1) < 0x21)
                pScan -= 1;
            pField = pScan;
        }
    }
    // recorta blancos del final y anexa pRecord entero
    while (pWrite != pBuffer && (unsigned char)*(pWrite - 1) < 0x21)
        pWrite -= 1;
    if (pWrite < pLimit - 2) {
        if (pWrite != pBuffer)
            *pWrite++ = TagFieldSep;
#ifdef EA_PLATFORM_WIN32
        while (*pSrc != '\0') {
            if (pWrite >= pLimit - 1)
                return _TagFieldSetupCancel(pBuffer, pWrite);
            *pWrite++ = *pSrc++;
        }
#else
        while (*pRecord != '\0') {
            if (pWrite >= pLimit - 1)
                return _TagFieldSetupCancel(pBuffer, pWrite);
            *pWrite++ = *pRecord++;
        }
#endif
        *pWrite = '\0';
        return 1;
    }
    if (pWrite + 1 < pLimit) {
        *pWrite = '\0';
        return _TagFieldSetupTerm(pBuffer, pWrite, TagFieldNull);
    }
    return -1;
}

// guarda un flotante como entero.punto.decimales (sin ceros de sobra)
int TagFieldSetFloat(char *pBuffer, int iBufferSize, const char *pName, float fValue)
{
    char szBuf[64];
    char *pInt = szBuf + 32;
    char *pTmp = pInt;
#ifdef EA_PLATFORM_WIN32
    float fAbs;
#else
    // A124: fValue se copia a fs1 y fAbs (fs0) se niega en sitio (bc1f tras la copia)
    float fVal = fValue;
    float fAbs = fVal;
#endif
    unsigned int uInt;
    unsigned int uFrac;
    char *pPos;
    int iLen;
#ifdef EA_PLATFORM_WIN32
    if (fValue < 0.0f)
        fAbs = -fValue;
    else
        fAbs = fValue;
#else
    if (fVal < 0.0f)
        fAbs = -fAbs;
#endif
    uInt = (unsigned int)fAbs;
    if (uInt != 0) {
        do {
            *--pInt = (uInt % 10) + '0';
            uInt /= 10;
        } while (uInt != 0);
    }
    if (pInt == pTmp)
        *--pInt = '0';
#ifdef EA_PLATFORM_WIN32
    if (fValue < 0.0f)
#else
    if (fVal < 0.0f)
#endif
        *--pInt = '-';
    *pTmp = '.';
    pTmp += 1;
#ifdef EA_PLATFORM_WIN32
    // PC: la escala va como literal (fmul contra __real@461c4000 en el exe)
    uFrac = (unsigned int)(fAbs * 10000.0f) % 10000;
#else
    uFrac = (unsigned int)(fAbs * TagFieldFloatScale) % 10000;
#endif
    do {
        unsigned int uQ = (uFrac / 1000) % 10;
        *pTmp++ = uQ + '0';
        uFrac = (uFrac * 10) % 10000;
    } while (uFrac != 0);
    iLen = pTmp - pInt;
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, iLen);
    if (pPos == NULL)
        return -1;
    memcpy(pPos, pInt, iLen);
    return _TagFieldSetupTerm(pBuffer, pPos + iLen, pName);
}

// guarda el valor en bruto (con comillas si trae espacios); NULL borra
int TagFieldSetRaw(char *pBuffer, int iBufferSize, const char *pName, const char *pValue)
{
    char *pPos;
    if (pValue == NULL) {
        TagFieldDelete(pBuffer, pName);
        return 0;
    }
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, 0);
    if (pPos == NULL)
        return -1;
    {
        unsigned char term = 0x21;
        if ((unsigned char)*pValue >= term) {
            char *pLim = pBuffer + iBufferSize - 2;
            do {
                if (pPos >= pLim)
                    return _TagFieldSetupCancel(pBuffer, pPos);
#ifdef EA_PLATFORM_WIN32
                // PC: el toggle va como if (la A124 lo compila desde la forma booleana)
                if ((unsigned char)*pValue == '"')
                    term ^= 1;
#else
                term ^= ((unsigned char)*pValue == '"');
#endif
                *pPos++ = *pValue++;
            } while ((unsigned char)*pValue >= term);
        }
    }
    return _TagFieldSetupTerm(pBuffer, pPos, pName);
}

// lee el valor en bruto hasta el blanco (o el fin entre comillas)
int TagFieldGetRaw(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault)
{
    int iLen;
    unsigned char term;
    if (pField == NULL) {
        if (pDefault == NULL)
            return -1;
        iLen = 1;
        for (;;) {
            if (iLen >= iBufferSize)
                break;
            if (*pDefault == '\0')
                break;
            *pBuffer++ = *pDefault++;
            iLen += 1;
        }
        *pBuffer = '\0';
        return iLen - 1;
    }
    iLen = 1;
    term = 0x21;
    if (iLen < iBufferSize) {
#ifdef EA_PLATFORM_WIN32
        // PC (speed.exe): la guarda c < term es la prueba de cabeza del lazo (sin retorno
        // temprano propio) y la comilla alterna el terminador con sete/xor; la A124 (otra
        // version del DirtySDK) mira el primer caracter antes del lazo y relee *pField al guardar.
        unsigned char c;
        do {
            c = *pField;
            if (c < term)
                break;
            term ^= (c == '"');
            *pBuffer++ = c;
            pField++;
            iLen += 1;
        } while (iLen < iBufferSize);
#else
        unsigned char c = *pField;
        if (c >= term) {
            for (;;) {
                term ^= (c == '"');
                iLen += 1;
                *pBuffer = *pField;
                pField += 1;
                pBuffer += 1;
                if (iLen >= iBufferSize)
                    break;
                c = *pField;
                if (c < term)
                    break;
            }
        }
#endif
    }
    *pBuffer = '\0';
    return iLen - 1;
}

// guarda un entero en decimal
int TagFieldSetNumber(char *pBuffer, int iBufferSize, const char *pName, int iValue)
{
    char *pPos;
    unsigned int iAbs = iValue;
    if (iValue < 10) {
        if (iValue < 0)
            iAbs = -iValue;
        else {
            pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, 1);
            if (pPos == NULL)
                return -1;
            *pPos++ = iValue + '0';
            return _TagFieldSetupTerm(pBuffer, pPos, pName);
        }
    }
    {
        unsigned char szNum[12];
        unsigned char *pNum = szNum + 11;
        *pNum = '\0';
        do {
            pNum -= 1;
            *pNum = (iAbs % 10) + '0';
            iAbs /= 10;
        } while (iAbs != 0);
        if (iValue < 0) {
            pNum -= 1;
            *pNum = '-';
        }
        pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, (szNum + 12) - pNum - 1);
        if (pPos == NULL)
            return -1;
        while (*pNum != '\0')
            *pPos++ = *pNum++;
        return _TagFieldSetupTerm(pBuffer, pPos, pName);
    }
}

// lee un entero decimal o hexadecimal ("$" delante); 0 si no hay digitos
int TagFieldGetNumber(const char *pField, int iDefault)
{
    int iSign = 1;
    int iBase = 10;
    int iVal;
    unsigned int uDigit;
    if (pField == NULL)
        return iDefault;
    if (*pField == '-') {
        pField += 1;
        iSign = -1;
    } else if (*pField == '$') {
        pField += 1;
        iBase = 16;
    } else if (*pField == '+') {
        pField += 1;
    }
    // indice char (lb) y comparacion sin signo (sltu): 0x80 corta
    for (iVal = 0; (uDigit = TagFieldHexVal[*pField]) < iBase; pField++)
        iVal = iVal * iBase + uDigit;
    return iSign * iVal;
}

// guarda los bits de iFlags con el alfabeto de 32 simbolos
int TagFieldSetFlags(char *pBuffer, int iBufferSize, const char *pName, int iFlags)
{
    char *pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, 0);
    const char *pDigits = TagFieldB32Digits;
    if (pPos == NULL)
        return -1;
    if (iFlags != 0) {
        unsigned char c = *pDigits;
        if (c != '\0') {
            do {
                if ((iFlags & 1) != 0) {
                    if (pPos >= pBuffer + iBufferSize - 2)
                        return _TagFieldSetupCancel(pBuffer, pPos);
                    *pPos++ = c;
                }
                iFlags >>= 1;
                if (iFlags == 0)
                    break;
                pDigits += 1;
                c = *pDigits;
            } while (c != '\0');
        }
    }
    return _TagFieldSetupTerm(pBuffer, pPos, pName);
}

// lee los bits guardados por TagFieldSetFlags (0 si el primero es raro)
int TagFieldGetFlags(const char *pField, int iDefault)
{
    int iFlags;
    int iVal;
    if (pField == NULL)
        return iDefault;
    // tabla con signo (lb) e indice char (lb): 0xFF termina
    iFlags = 0;
    for (iVal = TagFieldB32Val[*pField]; iVal >= 0; iVal = TagFieldB32Val[*++pField])
        iFlags |= 1 << iVal;
    return iFlags;
}

// guarda una cadena (entrecomillada si trae espacios o comas, con %XX los raros)
int TagFieldSetString(char *pBuffer, int iBufferSize, const char *pName, const char *pValue)
{
    char *pPos;
    // pOrg/pScan unsigned char: las pruebas *pOrg y *pScan salen lbu en la A124
    const unsigned char *pOrg = (const unsigned char *)pValue;
    int iRoom;
    const unsigned char *pScan;
    if (pValue == NULL) {
        TagFieldDelete(pBuffer, pName);
        return 0;
    }
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, 2);
    if (pPos == NULL)
        return -1;
    iRoom = (pBuffer + iBufferSize) - (pPos + 1);
#ifdef EA_PLATFORM_WIN32
    // PC: el barrido es un for con la c cargada en la cabecera
    for (pScan = (const unsigned char *)pValue; *pScan != '\0'; pScan += 1) {
        unsigned char c = *pScan;
        if (c == ' ' || c == ',') {
            *pPos++ = '"';
            iRoom -= 2;
            break;
        }
    }
#else
    pScan = (const unsigned char *)pValue;
    for (;;) {
        unsigned char c = *pScan;
        if (c == '\0')
            break;
        if (c == ' ' || c == ',') {
            *pPos++ = '"';
            iRoom -= 2;
            break;
        }
        pScan += 1;
    }
#endif
    if (*pOrg != '\0') {
#ifdef EA_PLATFORM_WIN32
        // PC: sin c local; cada uso relee *pValue (movzx ecx, [edi] tras guardar el '%')
        // y el iRoom -= 3 va despues de los tres guardados (sub edx, 3 al final en el exe)
        do {
            if (iRoom <= 0)
                break;
            if ((TagFieldCharClass[(unsigned char)*pValue] & 1) != 0) {
                if (iRoom >= 3) {
                    *pPos++ = '%';
                    *pPos++ = TagFieldHexDigits[(unsigned char)*pValue >> 4];
                    *pPos++ = TagFieldHexDigits[(unsigned char)*pValue & 0x0F];
                    iRoom -= 3;
                }
            } else {
                *pPos++ = *pValue;
                iRoom -= 1;
            }
            pValue += 1;
        } while (*pValue != '\0');
#else
        // la A124 tampoco tiene c local: relee *pValue en cada uso (lbu 0(s0) tras el
        // '%' igual que PC) y el lazo es while con el escape anidado; el iRoom -= 3 va
        // antes de los guardados (addiu a2, -3 delante del sb '%' en el asm)
        while (*pValue != '\0') {
            if (iRoom <= 0)
                break;
            if ((TagFieldCharClass[(unsigned char)*pValue] & 1) != 0) {
                if (iRoom >= 3) {
                    iRoom -= 3;
                    *pPos++ = '%';
                    *pPos++ = TagFieldHexDigits[(unsigned char)*pValue >> 4];
                    *pPos++ = TagFieldHexDigits[(unsigned char)*pValue & 0x0F];
                }
            } else {
                *pPos++ = *pValue;
                iRoom -= 1;
            }
            pValue += 1;
        }
#endif
    }
    if (*pScan == ' ' || *pScan == ',')
        *pPos++ = '"';
    if (iRoom < TagFieldDoTerm)
        return _TagFieldSetupCancel(pBuffer, pPos);
    return _TagFieldSetupTerm(pBuffer, pPos, pName);
}

// lee una cadena con escapes %XX y comillas (longitud si pBuffer nulo)
int TagFieldGetString(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault)
{
    int iOut;
    int term = 0x20;
    if (pField == NULL) {
        if (pDefault == NULL)
            return -1;
        if (pBuffer == NULL)
            return strlen(pDefault);
        iOut = 1;
        for (;;) {
            if (iOut >= iBufferSize)
                break;
            if (*pDefault == '\0')
                break;
            *pBuffer++ = *pDefault++;
            iOut += 1;
        }
        *pBuffer = '\0';
        return iOut - 1;
    }
    if (*pField == '"') {
        term = *pField;
        pField += 1;
    }
    if (pBuffer == NULL) {
        // solo mide
        iOut = 0;
        for (;;) {
            unsigned char c = *pField & 0xFF;
            if (c == term)
                break;
            if (c < 0x20)
                break;
            if (c == '%' && (unsigned char)pField[1] >= 0x20 && (unsigned char)pField[2] >= 0x20)
                pField += 2;
            iOut += 1;
            pField += 1;
        }
        return iOut;
    }
    if (iBufferSize < 1)
        return -1;
    iOut = 1;
    while (iOut < iBufferSize) {
        unsigned char c = *pField & 0xFF;
        if (c == term)
            break;
        if (c < 0x20)
            break;
        if (c != '%') {
            *pBuffer++ = (char)c;
            pField += 1;
        } else if (pField[1] == '%') {
            *pBuffer++ = '%';
            pField += 2;
        } else if ((unsigned char)pField[1] < 0x20 || (unsigned char)pField[2] < 0x20) {
            *pBuffer++ = '%';
            pField += 1;
        } else {
            *pBuffer++ = (char)((TagFieldHexVal[(unsigned char)pField[1]] << 4) | TagFieldHexVal[(unsigned char)pField[2]]);
            pField += 3;
        }
        iOut += 1;
    }
    *pBuffer = '\0';
    return iOut - 1;
}

extern unsigned int LobbyTime(void);

// guarda segundos (epoch) como "ano.mes-dia:hora:min" (0 = ahora)
int TagFieldSetEpoch(char *pBuffer, int iBufferSize, const char *pName, unsigned int uEpoch)
{
    char szDate[20];
    int iTm[9];
    int *pTm;
    char *pPos;
    int iLen;
    if (uEpoch == 0)
        uEpoch = LobbyTime();
    pTm = _SecsToTime(iTm, uEpoch);
    if (pTm == NULL)
        return -1;
    iLen = sprintf(szDate, "%d.%d.%d-%d:%02d:%02d", pTm[5] + 1900, pTm[4] + 1, pTm[3], pTm[2], pTm[1], pTm[0]);
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, iLen);
    if (pPos == NULL)
        return -1;
    strcpy(pPos, szDate);
    return _TagFieldSetupTerm(pBuffer, pPos + iLen, pName);
}

// lee segundos de "$" hexadecimal o "ano.mes-dia:hora:min" (defecto/ahora si no)
unsigned int TagFieldGetEpoch(const char *pField, unsigned int uDefault)
{
    unsigned int uEpoch = 0;
    if (pField != NULL) {
        if (*pField == '$') {
            unsigned char v;
            pField += 1;
            // indice signed char (lb en la A124, movsx en PC), sin variable intermedia:
            // la primera carga sale addu v1 en la A124 y movsx en PC, como en los dos asm
            v = TagFieldHexVal[(signed char)*pField];
            while (v < 16) {
                uEpoch = (uEpoch << 4) | v;
                v = TagFieldHexVal[(signed char)*++pField];
            }
        } else {
            int iTm[9];
            int iDummy;
            const char *pEnd;
            if (*pField >= '0' && *pField <= '9') {
                pEnd = _ParseNumber(pField, &iDummy, 0);
                if (*pEnd <= 0x20)
                    uEpoch = iDummy;
                else if (*pField >= '0' && *pField <= '9') {
                    // tras cada numero se salta un separador (lo que no sea digito)
                    memset(iTm, 0, 0x24);
                    iTm[8] = -1;
                    pField = _ParseNumber(pField, &iTm[5], 4);
                    if ((*pField < '0' || *pField > '9') && *pField != '\0')
                        pField += 1;
                    pField = _ParseNumber(pField, &iTm[4], 2);
                    if ((*pField < '0' || *pField > '9') && *pField != '\0')
                        pField += 1;
                    pField = _ParseNumber(pField, &iTm[3], 2);
                    if ((*pField < '0' || *pField > '9') && *pField != '\0')
                        pField += 1;
                    pField = _ParseNumber(pField, &iTm[2], 2);
                    if ((*pField < '0' || *pField > '9') && *pField != '\0')
                        pField += 1;
                    pField = _ParseNumber(pField, &iTm[1], 2);
                    if ((*pField < '0' || *pField > '9') && *pField != '\0')
                        pField += 1;
                    pField = _ParseNumber(pField, &iTm[0], 2);
                    if (iTm[5] < 1970 || iTm[5] > 2107 || iTm[4] < 1 || iTm[4] > 12 || iTm[3] < 1 || iTm[3] > 31)
                        iTm[5] = 0;
                    if (iTm[2] < 0 || iTm[2] > 23 || iTm[1] < 0 || iTm[1] > 59 || iTm[0] < 0 || iTm[0] > 61)
                        iTm[5] = 0;
                    if (iTm[5] != 0) {
                        iTm[5] -= 1900;
                        iTm[4] -= 1;
                        uEpoch = _TimeToSecs(iTm);
                    }
                }
            }
        }
    }
    if (uEpoch != 0)
        return uEpoch;
    uEpoch = uDefault;
    if (uEpoch == 0)
        uEpoch = LobbyTime();
    return uEpoch;
}

// lee un flotante con signo y decimales (defecto si nulo)
float TagFieldGetFloat(const char *pField, float fDefault)
{
    float fSign = 1.0f;
    int iInt = 0;
    int iFrac = 0;
    int iScale = 1;
    float fVal;
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x83C050): el digito se prueba sobre el byte con signo (cmp cl / jl, jg),
    // el '.' se mira antes de avanzar y el (float)iInt se hace al final (fild/fild/fdivp/fild/faddp);
    // la A124 (otra version del DirtySDK) resta '0' y compara sin signo.
    char c;
    if (pField == NULL)
        return fDefault;
    if (*pField == '+') {
        pField += 1;
    } else if (*pField == '-') {
        fSign = -1.0f;
        pField += 1;
    }
    for (c = *pField; c >= '0' && c <= '9'; c = *++pField)
        iInt = iInt * 10 + (c & 0x0F);
    if (*pField == '.') {
        char d;
        pField += 1;
        d = *pField;
        while (d >= '0' && d <= '9') {
            iFrac = iFrac * 10 + (d & 0x0F);
            d = *++pField;
            iScale *= 10;
        }
    }
    {
        float fInt = (float)iInt;
        float fFrac = (float)iFrac;
        float fScale = (float)iScale;
        return fSign * (fInt + fFrac / fScale);
    }
#else
    unsigned char c;
    if (pField == NULL)
        return fDefault;
    if (*pField == '+') {
        pField += 1;
    } else if (*pField == '-') {
        fSign = -1.0f;
        pField += 1;
    }
    for (c = *pField; (unsigned int)(c - '0') < 10; c = *++pField)
        iInt = iInt * 10 + (c & 0x0F);
    fVal = (float)iInt;
    if (*pField++ == '.') {
        unsigned char d;
        for (d = *pField; (unsigned int)(d - '0') < 10; d = *++pField) {
            iFrac = iFrac * 10 + (d & 0x0F);
            iScale *= 10;
        }
    }
    return fSign * (fVal + (float)iFrac / (float)iScale);
#endif
}

// lee el campo iIndex-esimo separado por cDelim (con %XX y comillas)
int TagFieldGetDelim(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault, int iIndex, int cDelim)
{
    int iOut;
    unsigned char term;
    unsigned char end;
    // salta iIndex campos; si falta un delimitador, pField queda nulo y vale el defecto
    if (pField != NULL && iIndex > 0) {
        for (;;) {
            if (*pField == '"') {
                term = 0x20;
                end = (unsigned char)*pField;
                pField += 1;
            } else {
                term = 0x21;
                end = (unsigned char)cDelim;
            }
            while ((unsigned char)*pField != end && (unsigned char)*pField >= term)
                pField += 1;
            if (*pField == '"' && end == '"')
                pField += 1;
            if (*pField != cDelim) {
                pField = NULL;
                break;
            }
            pField += 1;
            iIndex -= 1;
            if (iIndex <= 0)
                break;
        }
    }
    if (pField == NULL) {
        if (pDefault == NULL)
            return -1;
        if (pBuffer == NULL)
            return strlen(pDefault);
        iOut = 1;
        while (iOut < iBufferSize && *pDefault != '\0') {
            *pBuffer++ = *pDefault++;
            iOut += 1;
        }
        *pBuffer = '\0';
        return iOut - 1;
    }
    // copia el campo hallado (como GetString con cDelim de fin)
    if (*pField == '"') {
        term = 0x20;
        end = (unsigned char)*pField;
        pField += 1;
    } else {
        term = 0x21;
        end = (unsigned char)cDelim;
    }
    if (pBuffer == NULL) {
        iOut = 0;
        for (;;) {
            unsigned char c = *pField & 0xFF;
            if (c == end)
                break;
            if (c < term)
                break;
            if (c == '%' && (unsigned char)pField[1] >= 0x20 && (unsigned char)pField[2] >= 0x20)
                pField += 2;
            iOut += 1;
            pField += 1;
        }
        return iOut;
    }
    if (iBufferSize < 1)
        return -1;
    iOut = 1;
    while (iOut < iBufferSize) {
        unsigned char c = *pField & 0xFF;
        if (c == end)
            break;
        if (c < term)
            break;
        if (c != '%') {
            *pBuffer++ = (char)c;
            pField += 1;
        } else if (pField[1] == '%') {
            *pBuffer++ = '%';
            pField += 2;
        } else if ((unsigned char)pField[1] < 0x20 || (unsigned char)pField[2] < 0x20) {
            *pBuffer++ = '%';
            pField += 1;
        } else {
            *pBuffer++ = (char)((TagFieldHexVal[(unsigned char)pField[1]] << 4) | TagFieldHexVal[(unsigned char)pField[2]]);
            pField += 3;
        }
        iOut += 1;
    }
    *pBuffer = '\0';
    return iOut - 1;
}

// parte los segundos (epoch 1970) en un struct tm de 9 enteros (el wday no);
// devuelve pTm (speed.exe 0x83A28B y la A124 devuelven el puntero y el llamante
// lee los campos por el)
static int *_SecsToTime(int *pTm, unsigned int uSecs)
{
    int iDays = uSecs / 86400;
    int iRem = uSecs % 86400;
    int iQ;
    int iYear = 1970;
    int iYDays;
    pTm[0] = iRem % 60;
    iQ = iRem / 60;
    pTm[1] = iQ % 60;
    pTm[2] = iQ / 60;
    for (;;) {
        int iApprox;
        int iNew;
        int iAdv;
        int iLeapOld;
        int iLeapNew;
        // 365 dias salvo los anhos bisiestos (divisibles por 4 y no por 100, o por 400)
        if ((iYear & 3) != 0 || (iYear % 100 == 0 && iYear % 400 != 0))
            iYDays = 365;
        else
            iYDays = 366;
        if (iDays < iYDays)
            break;
        iApprox = iDays / 366;
        iNew = iYear + iApprox;
        if (iNew == iYear)
            iNew += 1;
        iAdv = iNew - iYear;
        iLeapOld = (iYear - 1) / 4 - (iYear - 1) / 100 + (iYear - 1) / 400;
        iLeapNew = (iNew - 1) / 4 - (iNew - 1) / 100 + (iNew - 1) / 400;
        iDays -= iAdv * 365;
        iDays += iLeapOld;
        iDays -= iLeapNew;
        iYear = iNew;
    }
    {
        // 12 meses de 31/30/29 segun el ano (indice por el ultimo bit de iYDays)
        const int *pMonths;
        pTm[5] = iYear - 1900;
        pMonths = TagFieldMonthDays + ((iYDays & 1) * 12);
        pTm[7] = iDays;
        pTm[4] = 0;
        while (iDays >= *pMonths) {
            iDays -= *pMonths;
            pMonths += 1;
            pTm[4] += 1;
        }
        pTm[8] = 0;
        pTm[3] = iDays + 1;
    }
    return pTm;
}

// busca por biseccion los segundos cuyo desglose coincide con el tm (0 si no)
static unsigned int _TimeToSecs(const int *pTm)
{
    unsigned int uLo = 0;
    unsigned int uHi = 0xFFFFFFFF;
    unsigned int uMid;
    int iTm[9];
    int iDiff = 0;
    while (uLo <= uHi) {
        uMid = (uLo >> 1) + (uHi >> 1) + ((uLo & uHi) & 1);
        _SecsToTime(iTm, uMid);
        // ano, mes, dia, hora, minuto y segundo: el primero que difiere decide
        if ((iDiff = iTm[5] - pTm[5]) == 0 && (iDiff = iTm[4] - pTm[4]) == 0 && (iDiff = iTm[3] - pTm[3]) == 0
            && (iDiff = iTm[2] - pTm[2]) == 0 && (iDiff = iTm[1] - pTm[1]) == 0 && (iDiff = iTm[0] - pTm[0]) == 0)
            break;
        if (uLo == uHi)
            break;
        if (iDiff > 0)
            uHi = uMid - 1;
        else
            uLo = uMid + 1;
    }
    return iDiff == 0 ? uMid : 0;
}

// guarda bytes como "$" + hexadecimal
int TagFieldSetBinary(char *pBuffer, int iBufferSize, const char *pName, const void *pData, int iSize)
{
    const unsigned char *pSrc = (const unsigned char *)pData;
    char *pPos;
    if (iSize < 0)
        return -1;
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, iSize * 2 + 1);
    if (pPos == NULL)
        return -1;
    *pPos++ = '$';
    while (iSize > 0) {
        *pPos++ = TagFieldHexDigits[*pSrc >> 4];
        *pPos++ = TagFieldHexDigits[*pSrc & 0x0F];
        pSrc += 1;
        iSize -= 1;
    }
    return _TagFieldSetupTerm(pBuffer, pPos, pName);
}

// lee bytes en "$" hexadecimal o "^" de 7 bits (longitud si pBuffer nulo)
int TagFieldGetBinary(const char *pField, void *pBuffer, int iBufferSize)
{
    unsigned char *pDst = (unsigned char *)pBuffer;
    unsigned char *pEnd = pDst + iBufferSize;
    if (pField == NULL)
        return -1;
#ifdef EA_PLATFORM_WIN32
    // PC: un solo bloque de salida -1 al final (los tres saltos van al mismo epilogo)
    if (*pField == '$' || *pField == '^') {
#else
    if (*pField != '$' && *pField != '^')
        return -1;
#endif
    if (pDst == NULL) {
        // solo mide: "$" cuenta parejas hex, "^" cuenta simbolos de 7 bits
        if (*pField == '$') {
            const char *p = pField + 1;
            while ((unsigned char)*p >= '0' && (unsigned char)p[1] >= '0')
                p += 2;
            return (p - pField - 1) / 2;
        } else {
            const char *p = pField + 1;
            if (*(const signed char *)p < 0) {
                do {
                    p += 1;
                } while (*(const signed char *)p < 0);
            }
            return ((p - pField - 1) * 7) / 8;
        }
    }
#ifdef EA_PLATFORM_WIN32
    if (iBufferSize >= 1) {
#else
    if (iBufferSize < 1)
        return -1;
#endif
    if (*pField == '$') {
        const char *p = pField + 1;
        while ((unsigned char)*p >= '0' && (unsigned char)p[1] >= '0' && pDst < pEnd) {
            *pDst++ = (TagFieldHexVal[(unsigned char)p[0]] << 4) | TagFieldHexVal[(unsigned char)p[1]];
            p += 2;
        }
    } else {
        const signed char *p = (const signed char *)pField + 1;
        unsigned int uAcc = 0;
        int iBits = 0;
        while ((unsigned char)*p >= 0x80 && pDst < pEnd) {
            unsigned int uVal = ((unsigned char)*p & 0x7F) << iBits;
            iBits += 7;
            uAcc |= uVal;
            if (iBits >= 8) {
                *pDst++ = (unsigned char)uAcc;
                uAcc >>= 8;
                iBits -= 8;
            }
            p += 1;
        }
    }
    return pDst - (unsigned char *)pBuffer;
#ifdef EA_PLATFORM_WIN32
    }
    }
    return -1;
#endif
}

// lee una estructura de campos hexadiminales de pField segun pFormat: "[n]l" (entero
// de 32 bits en hexadecimal big-endian textual), "[n]w" (16), "[n]b" (8), "ns" (cadena
// de n bytes con escapes %XX y relleno a 0), "na" (salta n bytes, 1 si n es 0);
// un '*' tras la letra repite lo anterior hasta llenar pBuffer (o el hueco medido con
// pBuffer nulo). El inicial '-' niega el valor.
int TagFieldGetStructure(const char *pField, void *pBuffer, int iBufferSize, const char *pFormat)
{
    char szBuf[1024];
    unsigned char *pDst = (unsigned char *)pBuffer;
    unsigned char *pEnd;
    int iLen;
#ifndef EA_PLATFORM_WIN32
    unsigned char c;
#endif
    if (iBufferSize < 0)
        pEnd = pDst + 65535;
    else
        pEnd = pDst + iBufferSize;
    if (pField == NULL)
        pField = TagFieldNull;
    if (pBuffer == NULL) {
        pBuffer = szBuf;
        pDst = (unsigned char *)szBuf;
    }
#ifdef EA_PLATFORM_WIN32
    // PC: la guarda es > 0x20 (cmp dl, 0x20 / jbe en el exe)
    while (*pFormat != '\0' && (unsigned char)*pField > 0x20) {
#else
    // la A124 se queda el caracter probado en c (es el inicial de la rama 's');
    // en PC la rama 's' relee *pField en cada vuelta (otra version del DirtySDK)
    while (*pFormat != '\0' && (c = *pField) >= 0x21) {
#endif
        int iCount = 0;
        int iDigits;
#ifdef EA_PLATFORM_WIN32
        // PC: cf con signo (cmp al, '0' / jl y cmp al, '9' / jg en el exe)
        char cf;
        for (cf = *pFormat; cf >= '0' && cf <= '9'; cf = *++pFormat)
#else
        unsigned char cf;
        for (cf = *pFormat; (unsigned int)(cf - '0') < 10; cf = *++pFormat)
#endif
            iCount = iCount * 10 + (cf & 0x0F);
        iDigits = 0;
        if (*pFormat == 'l')
            iDigits = 8;
        else if (*pFormat == 'b')
            iDigits = 2;
        else if (*pFormat == 'w')
            iDigits = 4;
        else if (*pFormat == 's' && iCount > 0) {
#ifdef EA_PLATFORM_WIN32
            // PC: la rama 's' cuenta con iOut propio (el exe toma esi prestado para el
            // contador y restaura pFormat de su home; iDigits queda para l/b/w)
            int iOut = 0;
            unsigned char c;
            // PC: el lazo relee *pField arriba en cada vuelta (mov al, [ebp] en la cabecera)
            for (;;) {
                c = *pField;
                if ((c < '0' && c != '%') || iOut + 1 >= iCount)
                    break;
                if (c == '%') {
                    pDst[iOut] = (TagFieldHexVal[(unsigned char)pField[1]] << 4) | TagFieldHexVal[(unsigned char)pField[2]];
                    pField += 3;
                } else {
                    pDst[iOut] = c;
                    pField++;
                }
                iOut++;
            }
            while (iOut < iCount)
                pDst[iOut++] = '\0';
            pDst += iOut;
#else
            for (; (c >= '0' || c == '%') && iDigits + 1 < iCount; c = *pField) {
                if (c == '%') {
                    pDst[iDigits] = (TagFieldHexVal[(unsigned char)pField[1]] << 4) | TagFieldHexVal[(unsigned char)pField[2]];
                    pField += 3;
                } else {
                    pDst[iDigits] = c;
                    pField++;
                }
                iDigits++;
            }
            while (iDigits < iCount)
                pDst[iDigits++] = '\0';
            pDst += iDigits;
            iDigits = 0;
#endif
            if ((unsigned char)*pField == ',')
                pField++;
        } else if (*pFormat == 'a') {
#ifdef EA_PLATFORM_WIN32
            // PC: un solo add edi, ecx con ecx puesto a 1 si la cuenta es 0
            if (iCount == 0)
                iCount = 1;
            pDst += iCount;
#else
            if (iCount != 0)
                pDst += iCount;
            else
                pDst += 1;
#endif
        }
        if (iDigits > 0) {
            int iVal;
            unsigned int uDigit;
            char cFirst = *pField;
            if (cFirst == '-')
                pField++;
            iVal = 0;
            while (iDigits > 0 && (uDigit = TagFieldHexVal[(unsigned char)*pField]) < 16) {
                iVal = (iVal << 4) | uDigit;
                iDigits--;
                pField++;
            }
            if (iDigits > 0 && (unsigned char)*pField == ',')
                pField++;
            if (cFirst == '-')
                iVal = -iVal;
            if (*pFormat == 'l') {
                *(int *)pDst = iVal;
                pDst += 4;
            } else if (*pFormat == 'b') {
                *pDst = (unsigned char)iVal;
                pDst += 1;
            } else if (*pFormat == 'w') {
                *(short *)pDst = (short)iVal;
                pDst += 2;
            }
        }
        if (pDst >= pEnd)
            break;
        pFormat++;
        if (*pFormat == '*')
            pFormat--;
    }
    iLen = pDst - (unsigned char *)pBuffer;
    if (iLen < iBufferSize)
        memset(pDst, 0, iBufferSize - iLen);
    return iLen;
}

// escribe campos en pBuffer con un formato tipo printf: "%s %d %t %a %f %e" llaman al
// setter del tipo con pName nulo, '#' es snprintf("%d"), "%i" guarda el registro para
// "%x" (= SetRaw(Find(...))), "%r" copia la cadena tal cual; un blanco del formato vale
// TagFieldSep, '~' anexa con separador y ',' sigue sin separador (tragandose la coma si
// el registro acaba en '='). El parametro de tamano es el contador de lo que
// queda (iLeft, que se decrementa al escribir); iSize guarda el inicial.
// Devuelve los bytes escritos.
int TagFieldPrintf(char *pBuffer, int iLeft, const char *pFormat, ...)
{
    va_list ap;
    int iSize = iLeft;
    const char *pRecord = "";
    char *pOut = pBuffer;
    unsigned char c;
    va_start(ap, pFormat);
    if (*pFormat == '~') {
        pFormat++;
        while (*pOut != '\0' && iLeft > 1) {
            pOut++;
            iLeft--;
        }
        if (iLeft > 1 && iLeft < iSize && pOut[-1] >= 0x20) {
            *pOut++ = TagFieldSep;
            iLeft--;
        }
    } else if (*pFormat == ',') {
        while (*pOut != '\0' && iLeft > 1) {
            pOut++;
            iLeft--;
        }
        while (iLeft < iSize && pOut[-1] <= 0x20) {
            pOut--;
            iLeft++;
        }
        if (iLeft < iSize && pOut[-1] == '=')
            pFormat++;
    }
    while ((c = *pFormat++) != '\0' && iLeft > 1) {
        int iRes;
        if (c <= 0x20) {
            *pOut++ = TagFieldSep;
            iLeft--;
            while (*pFormat != '\0' && *pFormat <= 0x20)
                pFormat++;
            continue;
        }
        if (c == '#') {
            iRes = snprintf(pOut, iLeft, "%d", va_arg(ap, int));
            pOut += iRes;
            iLeft -= iRes;
            continue;
        }
        if (c != '%') {
            *pOut++ = c;
            iLeft--;
            continue;
        }
        if (*pFormat == '\0')
            continue;
        iRes = 0;
        c = *pFormat++;
        *pOut = '\0';
        if (c == 's')
            iRes = TagFieldSetString(pOut, iLeft, NULL, va_arg(ap, const char *));
        else if (c == 'd')
            iRes = TagFieldSetNumber(pOut, iLeft, NULL, va_arg(ap, int));
        else if (c == 't')
            iRes = TagFieldSetToken(pOut, iLeft, NULL, va_arg(ap, int));
        else if (c == 'a')
            iRes = TagFieldSetAddress(pOut, iLeft, NULL, va_arg(ap, unsigned int));
        else if (c == 'f')
            iRes = TagFieldSetFlags(pOut, iLeft, NULL, va_arg(ap, int));
        else if (c == 'e')
            iRes = TagFieldSetEpoch(pOut, iLeft, NULL, va_arg(ap, unsigned int));
        else if (c == 'i')
            pRecord = va_arg(ap, const char *);
        else if (c == 'x')
            iRes = TagFieldSetRaw(pOut, iLeft, NULL, TagFieldFind(pRecord, va_arg(ap, const char *)));
        else if (c == 'r') {
            const char *pStr = va_arg(ap, const char *);
            while (*pStr != '\0' && iLeft > 0) {
                *pOut++ = *pStr++;
                iLeft--;
            }
        }
        if (iRes > 0) {
            pOut += iRes;
            iLeft -= iRes;
        }
    }
    va_end(ap);
    if (TagFieldSep == '\n' && iSize != iLeft && iLeft > 1) {
        *pOut++ = TagFieldSep;
        iLeft--;
    }
    if (iLeft > 0)
        *pOut = '\0';
    return iSize - iLeft;
}
