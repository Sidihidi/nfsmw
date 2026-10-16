// DirtySock: lobbytagfield.c, los campos con nombre ("etiqueta=valor") de los mensajes.
// Escrito del asm de la A124 (libdirtysock.a(lobbytagfield.o), .text 0x48B900, 0x2BC4):
// la biblioteca solo enlaza en el prototipo; SLUS no la tiene. Un registro es una
// cadena de campos "nombre=valor" separados por '\n' (TagFieldSep); el valor admite
// comillas ('"..."') y escapes %XX. Los nombres casan sin mayusculas (TagFieldUpper).
#include "lobbytagfield.h"

#include <string.h>
#include <stdio.h>

int _SecsToTime(int *pTm, unsigned int uSecs);
unsigned int _TimeToSecs(const int *pTm);

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
const unsigned char TagFieldB32Val[256] = "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x1f\xff\xff\x1b\x1c\x1d\x1e\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\xff\xff\xff\xff\xff\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

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
const char *_ParseNumber(const char *pStr, int *pValue, int iMaxDigits)
{
    int iLeft = (iMaxDigits > 0) ? iMaxDigits : 256;
    unsigned char c = *pStr;
    int iValue;
    *pValue = 0;
    if ((unsigned int)(c - '0') >= 10)
        return pStr;
    if (iLeft <= 0)
        return pStr;
    iValue = *pValue;
    do {
        iValue = iValue * 10 + (c & 0x0F);
        *pValue = iValue;
        c = *++pStr;
        if ((unsigned int)(c - '0') >= 10)
            break;
        if (--iLeft <= 0)
            break;
        iValue = *pValue;
    } while (1);
    return pStr;
}

// prepara pBuffer para anadir pName: borra el campo si ya existe y devuelve
// donde escribir el valor (o NULL si no cabe: hacen falta el separador, el
// nombre, iReserve bytes de valor, el '=' y el cierre). Con pName nulo vacia.
char *_TagFieldSetupAppend(char *pBuffer, int iBufferSize, const char *pName, int iReserve)
{
    char *pEnd = pBuffer;
    int iNameLen = 0;
    int iEquals = 1;
    int iSep;
    if (pBuffer == NULL)
        return NULL;
    if (pName == NULL) {
        *pBuffer = '\0';
        pName = TagFieldNull;
        iEquals = 0;
    }
    if (*pName != '\0') {
        iNameLen = 0;
        do {
            iNameLen += 1;
            if (*pEnd != '\0')
                pEnd += 1;
        } while (pName[iNameLen] != '\0');
    }
    while (*pEnd != '\0') {
        // busca el '=' del campo que ya existe
        if (*pEnd != '=') {
            pEnd++;
            continue;
        }
        if (*(pEnd - 1) < 0x21)
            return NULL;
        {
            char *pCand = pEnd - iNameLen;
            const char *pN = pName;
            const char *pC = pCand;
            if (pCand != pBuffer && *(pCand - 1) >= 0x21) {
                pEnd++;
                continue;
            }
            while (TagFieldUpper[(unsigned char)*pC] == TagFieldUpper[(unsigned char)*pN]) {
                pC++;
                pN++;
            }
            pEnd++;
            if (*pN != '\0')
                continue;
            // coincide: borra "nombre=valor" compactando encima del inicio
            {
                char *pScan = pEnd - 1;
                char *pSpace = NULL;
                char *pDst = pCand;
                unsigned char c;
                for (;;) {
                    c = *pScan;
                    if (c < 0x20)
                        break;
                    if (c == ' ')
                        pSpace = pScan;
                    pScan++;
                    if (c != '=')
                        continue;
                    if (pSpace == NULL)
                        continue;
                    pScan = pSpace;
                    break;
                }
                while (*pScan != '\0' && *pScan < 0x20)
                    pScan++;
                for (;;) {
                    c = *pScan++;
                    pDst++;
                    if (c == '\0')
                        break;
                    pDst[-1] = c;
                }
            }
            break;
        }
    }
    if (pEnd == pBuffer)
        iSep = 0;
    else if (*(pEnd - 1) < 0x20)
        iSep = 0;
    else
        iSep = (*(pEnd - 1) != TagFieldSep);
    if ((pBuffer + iBufferSize) - pEnd + TagFieldDoTerm < iSep + iNameLen + (iReserve + 1) + 1 + TagFieldDoTerm)
        return NULL;
    if (iSep != 0)
        *pEnd++ = TagFieldSep;
    while (*pName != '\0')
        *pEnd++ = *pName++;
    if (iEquals != 0)
        *pEnd++ = '=';
    *pEnd = '\0';
    return pEnd;
}

// deshace un SetupAppend fallido: recorta pBuffer hasta el campo anterior
int _TagFieldSetupCancel(char *pBuffer, char *pPos)
{
    if (pPos == pBuffer)
        *pPos = '\0';
    else {
        do {
            pPos -= 1;
        } while (*pPos >= 0x20);
        if (*pPos != '\n')
            pPos += 1;
        *pPos = '\0';
    }
    return -1;
}

// cierra un valor anadido con SetupAppend: '\n' de separacion y NUL final;
// devuelve la longitud del registro
int _TagFieldSetupTerm(char *pBuffer, char *pEnd, const char *pName)
{
    int iLen = pEnd - pBuffer;
    if (TagFieldDoTerm != 0) {
        if (pName != NULL) {
            *pEnd = TagFieldSep;
            pEnd += 1;
        }
        iLen = pEnd - pBuffer;
    }
    *pEnd = '\0';
    return iLen;
}

// borra el campo pName de pBuffer compactando el resto encima
int TagFieldDelete(char *pBuffer, const char *pName)
{
    char *pValue = TagFieldFind(pBuffer, pName);
    char *pDst;
    char *pScan;
    char *pCopy;
    if (pValue == NULL)
        return -1;
    // inicio del campo: retrocede hasta el blanco anterior
    pDst = pValue;
    while (pDst != pBuffer && *(pDst - 1) >= 0x21)
        pDst -= 1;
    // fin del valor: hasta el proximo '=' (el siguiente campo) o un control;
    // con '=' se copia desde el ultimo espacio (el valor no trae espacios)
    pScan = pValue;
    pCopy = pValue;
    if (*pScan >= 0x20 && *pScan != '=') {
        for (;;) {
            if (*pScan == ' ')
                pCopy = pScan;
            pScan += 1;
            if (*pScan < 0x20 || *pScan == '=')
                break;
        }
    }
    if (*pScan != '=')
        pCopy = pScan;
    while (*pCopy != '\0' && *pCopy < 0x20)
        pCopy += 1;
    while (*pCopy != '\0')
        *pDst++ = *pCopy++;
    for (;;) {
        if (pDst == pBuffer)
            break;
        if (*(pDst - 1) >= 0x21)
            break;
        pDst -= 1;
    }
    *pDst = '\0';
    return 0;
}

// cambia el nombre de un campo ajustando el hueco (el nuevo puede ser mas
// largo o mas corto)
int TagFieldRename(char *pBuffer, int iBufferSize, const char *pOldName, const char *pNewName)
{
    char *pValue = TagFieldFind(pBuffer, pOldName);
    char *pEnd;
    char *pStart;
    int iDiff;
    if (pValue == NULL)
        return -1;
    pEnd = pValue;
    if (*(pEnd - 1) == ' ')
        pEnd -= 1;
    pStart = pEnd - 1;
    pEnd = pStart;
    while (pStart != pBuffer && *(pStart - 1) >= 0x21)
        pStart -= 1;
    iDiff = strlen(pNewName) - (pEnd - pStart);
    if (iDiff < 0) {
        char *pSrc = pEnd;
        while (*pSrc != '\0') {
            *(pSrc + iDiff) = *pSrc;
            pSrc += 1;
        }
        *(pSrc + iDiff) = '\0';
    }
    if (iDiff > 0) {
        char *pTail = pEnd + 1;
        char *pS;
        if (*pEnd != '\0') {
            while (*pTail != '\0')
                pTail += 1;
        }
        if (pTail - pBuffer >= iBufferSize - iDiff)
            return -1;
        if (pTail >= pEnd) {
            pS = pTail;
            do {
                *(pS + iDiff) = *pS;
                pS -= 1;
            } while (pS >= pEnd);
        }
    }
    if (*pNewName == '\0')
        return 0;
    do {
        *pStart++ = *pNewName++;
    } while (*pNewName != '\0');
    return 0;
}

// copia la cadena pField en pBuffer (truncando a iBufferSize); devuelve lo copiado
int TagFieldDupl(char *pBuffer, int iBufferSize, const char *pField)
{
    int iLeft = iBufferSize;
    while (iLeft >= 2 && *pField != '\0') {
        *pBuffer = (unsigned char)*pField;
        iLeft -= 1;
        pField += 1;
        pBuffer += 1;
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
    int iDiff = 0;
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
        pCur += 1;
        pScan += 1;
    } while (*pCur != '\0');
    iLen = pCur - pBase;
    if (iDiff == 0) {
        if (*pScan == '=')
            return (char *)pScan + 1;
    }
    if (*pScan != '\0') {
        pScan += 1;
        for (;;) {
            // salta hasta el proximo '='
            for (;;) {
            if (*pScan == '\0')
                return NULL;
            if (*pScan == '=')
                break;
            pScan += 1;
            }
            if (*(pScan - 1) < 0x21)
                return NULL;
            {
                const unsigned char *pCand = pScan - iLen;
                const unsigned char *pN = pBase;
                const unsigned char *pC = pCand;
                if (*(pCand - 1) < 0x21) {
                    while (TagFieldUpper[*pC] == TagFieldUpper[*pN]) {
                        pC += 1;
                        pN += 1;
                    }
                    if (pC == pScan)
                        return (char *)pScan + 1;
                    pScan += 1;
                } else {
                    pScan += 1;
                }
            }
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
    char szAddr[16];
    char *pDst = szAddr;
    char *pPos;
    int i;
    oct[3] = (unsigned char)uAddr;
    uAddr >>= 8;
    oct[2] = (unsigned char)uAddr;
    uAddr >>= 8;
    oct[1] = (unsigned char)uAddr;
    uAddr >>= 8;
    oct[0] = (unsigned char)uAddr;
    for (i = 0; i < 4; i += 1) {
        unsigned char u = oct[i];
        if (i > 0)
            *pDst++ = '.';
        if (u >= 100) {
            *pDst++ = u / 100 + '0';
            u %= 100;
        }
        if (u >= 10) {
            *pDst++ = u / 10 + '0';
            u %= 10;
        }
        *pDst++ = u + '0';
    }
    *pDst = '\0';
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, (pDst - szAddr));
    if (pPos == NULL)
        return -1;
    {
        char *pSrc = szAddr;
        while (*pSrc != '\0') {
            *pPos++ = *pSrc++;
        }
    }
    return _TagFieldSetupTerm(pBuffer, pPos, pName);
}

// lee una direccion IPv4 en "a.b.c.d" (0 si no hay digitos, uDefault si nulo)
unsigned int TagFieldGetAddress(const char *pField, unsigned int uDefault)
{
    unsigned int uAddr = 0;
    if (pField == NULL)
        return uDefault;
    for (;;) {
        unsigned char c = *pField;
        if (c >= '0' && c <= '9') {
            uAddr = (uAddr & 0xFFFFFF00) | ((uAddr & 0xFF) * 10 + (c & 0x0F));
            pField += 1;
        } else if (c == '.') {
            uAddr <<= 8;
            pField += 1;
        } else {
            return uAddr;
        }
    }
}

// guarda un token de 4 bytes con los raros escapados como %XX
int TagFieldSetToken(char *pBuffer, int iBufferSize, const char *pName, int iToken)
{
    char szTok[16];
    char *pW = szTok + 15;
    char *pPos;
    int i;
    *pW = '\0';
    for (i = 3; i >= 0; i -= 1) {
        unsigned char b = (unsigned char)iToken;
        iToken >>= 8;
        if (TagFieldCharClass[b] & 2) {
            *pW = TagFieldHexDigits[b & 0x0F];
            pW -= 1;
            *pW = TagFieldHexDigits[(b >> 4) & 0x0F];
            pW -= 1;
            *pW = '%';
        } else {
            pW -= 1;
            *pW = (char)b;
        }
    }
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, (szTok + 15) - pW);
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
    if (pField == NULL)
        return iDefault;
    if (*pField < 0x21)
        return iDefault;
    for (i = 3; i >= 0; i -= 1) {
        unsigned char c = *pField;
        if (c < 0x20) {
            c = 0x20;
        } else {
            pField += 1;
        }
        if (c == '%') {
            unsigned char hi = *pField++;
            unsigned char lo = *pField++;
            c = ((TagFieldHexVal[hi] << 4) & 0xFF) | TagFieldHexVal[lo];
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
    int iLen;
    if (pRecord == NULL || *pRecord == '\0')
        return 0;
    pScan = pBuffer;
    pWrite = pBuffer;
    pField = pBuffer;
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
            while (*(pScan - 1) < 0x21)
                pScan -= 1;
            pField = pScan;
        }
    }
    // recorta blancos del final y anexa pRecord entero
    while (pWrite != pBuffer && *(pWrite - 1) < 0x21)
        pWrite -= 1;
    if (pWrite < pLimit - 2) {
        if (pWrite != pBuffer)
            *pWrite++ = TagFieldSep;
        while (*pRecord != '\0') {
            if (pWrite >= pLimit - 1)
                return _TagFieldSetupCancel(pBuffer, pWrite);
            *pWrite++ = *pRecord++;
        }
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
    char szBuf[32];
    char *pInt = szBuf + 32;
    char *pTmp;
    float fAbs = fValue;
    unsigned int uInt;
    unsigned int uFrac;
    char *pPos;
    int iLen;
    if (fValue < 0.0f) {
        fAbs = -fValue;
    } else {
        pTmp = pInt;
    }
    uInt = fptoui(fAbs);
    if (uInt != 0) {
        do {
            *--pInt = (uInt % 10) + '0';
            uInt /= 10;
        } while (uInt != 0);
    }
    if (pInt == pTmp)
        *--pInt = '0';
    if (fValue < 0.0f)
        *--pInt = '-';
    *pTmp = '.';
    pTmp += 1;
    uFrac = fptoui(fAbs * TagFieldFloatScale) % 10000;
    do {
        unsigned int uQ = uFrac / 1000;
        uFrac = (uFrac * 10) % 10000;
        uQ = (uQ / 10) % 10;
        *pTmp++ = uQ + '0';
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
    if (pValue == NULL)
        return TagFieldDelete(pBuffer, pName);
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, 0);
    if (pPos == NULL)
        return -1;
    if (*pValue >= 0x21) {
        char *pLim = pBuffer + iBufferSize - 2;
        unsigned char term = 0x21;
        unsigned char c = *pValue;
        for (;;) {
            if (pPos >= pLim)
                return _TagFieldSetupCancel(pBuffer, pPos);
            if (c == '"')
                term ^= 0x01;
            *pPos = c;
            pValue += 1;
            c = *pValue;
            pPos += 1;
            if (c >= term)
                continue;
            break;
        }
    }
    return _TagFieldSetupTerm(pBuffer, pPos, pName);
}

// lee el valor en bruto hasta el blanco (o el fin entre comillas)
int TagFieldGetRaw(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault)
{
    int iLen = 1;
    if (pField == NULL) {
        if (pDefault == NULL)
            return -1;
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
    if (iLen < iBufferSize) {
        unsigned char c = *pField;
        unsigned char term;
        if (c < 0x21) {
            *pBuffer = '\0';
            return 0;
        }
        term = 0x21;
        for (;;) {
            term = 0x21;
            if (c == '"')
                term = 0x20;
            c = *pField;
            iLen += 1;
            pField += 1;
            *pBuffer = c;
            pBuffer += 1;
            if (iLen >= iBufferSize)
                break;
            c = *pField;
            if (c >= term)
                continue;
            break;
        }
    }
    *pBuffer = '\0';
    return iLen - 1;
}

// guarda un entero en decimal
int TagFieldSetNumber(char *pBuffer, int iBufferSize, const char *pName, int iValue)
{
    char *pPos;
    if (iValue < 10 && iValue >= 0) {
        pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, 1);
        if (pPos == NULL)
            return -1;
        *pPos++ = iValue + '0';
        return _TagFieldSetupTerm(pBuffer, pPos, pName);
    } else {
        char szNum[12];
        char *pNum = szNum + 11;
        int iAbs = (iValue < 0) ? -iValue : iValue;
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
        pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, (szNum + 11) - pNum - 1);
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
    int iVal = 0;
    unsigned char c;
    if (pField == NULL)
        return iDefault;
    c = *pField;
    if (c == '-') {
        pField += 1;
        iSign = -1;
    } else if (c == '$') {
        pField += 1;
        iBase = 16;
    } else if (c == '+') {
        pField += 1;
    }
    for (;;) {
        unsigned char d = TagFieldHexVal[(unsigned char)*pField];
        if (d >= iBase)
            break;
        iVal = iVal * iBase + d;
        pField += 1;
    }
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
    int iFlags = iDefault;
    const signed char *pTbl;
    int iVal;
    if (pField == NULL)
        return iFlags;
    iFlags = 0;
    pTbl = (const signed char *)TagFieldB32Val;
    iVal = pTbl[(unsigned char)*pField];
    if (iVal < 0)
        return iFlags;
    do {
        iFlags |= 1 << iVal;
        pField += 1;
        iVal = pTbl[(unsigned char)*pField];
    } while (iVal >= 0);
    return iFlags;
}

// guarda una cadena (entrecomillada si trae espacios o comas, con %XX los raros)
int TagFieldSetString(char *pBuffer, int iBufferSize, const char *pName, const char *pValue)
{
    char *pPos;
    const char *pOrg = pValue;
    int iRoom;
    const char *pScan;
    if (pValue == NULL)
        return TagFieldDelete(pBuffer, pName);
    pPos = _TagFieldSetupAppend(pBuffer, iBufferSize, pName, 2);
    if (pPos == NULL)
        return -1;
    iRoom = (pBuffer + iBufferSize) - (pPos + 1);
    pScan = pValue;
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
    if (*pOrg != '\0') {
        for (;;) {
            unsigned char c;
            if (iRoom <= 0)
                break;
            c = *pValue;
            if (c == '\0')
                break;
            if ((TagFieldCharClass[c] & 1) == 0) {
                *pPos++ = c;
                iRoom -= 1;
            } else {
                if (iRoom < 3) {
                    pValue += 1;
                    continue;
                }
                iRoom -= 3;
                *pPos++ = '%';
                *pPos++ = TagFieldHexDigits[c >> 4];
                *pPos++ = TagFieldHexDigits[c & 0x0F];
            }
            pValue += 1;
        }
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
            if (c != '%') {
                pField += 1;
            } else if (pField[1] < 0x20) {
                pField += 1;
            } else if (pField[2] < 0x20) {
                pField += 1;
            } else {
                pField += 3;
            }
            iOut += 1;
        }
        return iOut;
    }
    if (iBufferSize <= 0)
        return -1;
    if (iBufferSize <= 1) {
        *pBuffer = '\0';
        return 0;
    }
    iOut = 0;
    for (;;) {
        unsigned char c = *pField & 0xFF;
        int iVal;
        if (c == term)
            break;
        if (c < 0x20)
            break;
        if (c != '%') {
            iVal = c;
            pField += 1;
        } else if (pField[1] == '%') {
            iVal = '%';
            pField += 2;
        } else if (pField[1] < 0x20 || pField[2] < 0x20) {
            iVal = '%';
            pField += 1;
        } else {
            iVal = ((TagFieldHexVal[(unsigned char)pField[1]] << 4)) | TagFieldHexVal[(unsigned char)pField[2]];
            pField += 3;
        }
        *pBuffer++ = (char)iVal;
        iOut += 1;
        if (iOut >= iBufferSize - 1)
            break;
    }
    *pBuffer = '\0';
    return iOut;
}

extern unsigned int LobbyTime(void);

// guarda segundos (epoch) como "ano.mes-dia:hora:min" (0 = ahora)
int TagFieldSetEpoch(char *pBuffer, int iBufferSize, const char *pName, unsigned int uEpoch)
{
    int iTm[9];
    char szDate[32];
    char *pPos;
    int iLen;
    if (uEpoch == 0)
        uEpoch = LobbyTime();
    if (_SecsToTime(iTm, uEpoch) == 0)
        return -1;
    iLen = sprintf(szDate, "%d.%d-%d:%02d:%02d", iTm[5] + 1900, iTm[4] + 1, iTm[3], iTm[2], iTm[1], iTm[0]);
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
            signed char c;
            unsigned char v;
            pField += 1;
            c = *pField;
            v = TagFieldHexVal[c];
            if (v < 16) {
                pField += 1;
                for (;;) {
                    uEpoch = (uEpoch << 4) | v;
                    c = *pField++;
                    v = TagFieldHexVal[c];
                    if (v >= 16)
                        break;
                }
            }
        } else {
            int iTm[9];
            int iDummy;
            const char *pEnd;
            if ((*pField - '0') >= 10)
                goto haveEpoch;
            pEnd = _ParseNumber(pField, &iDummy, 0);
            if (*pEnd < 0x21)
                goto haveEpoch;
            if ((*pField - '0') >= 10)
                goto haveEpoch;
            memset(iTm, 0, 0x24);
            iTm[8] = -1;
            pField = _ParseNumber(pField, &iTm[5], 4);
            if ((*pField - '0') >= 10)
                goto parsemore;
            if (*pField != '\0')
                pField += 1;
        parsemore:
            pField = _ParseNumber(pField, &iTm[4], 2);
            if ((*pField - '0') >= 10)
                goto parsemore2;
            if (*pField != '\0')
                pField += 1;
        parsemore2:
            pField = _ParseNumber(pField, &iTm[3], 2);
            if ((*pField - '0') >= 10)
                goto parsemore3;
            if (*pField != '\0')
                pField += 1;
        parsemore3:
            pField = _ParseNumber(pField, &iTm[2], 2);
            if ((*pField - '0') >= 10)
                goto parsemore4;
            if (*pField != '\0')
                pField += 1;
        parsemore4:
            pField = _ParseNumber(pField, &iTm[1], 2);
            if ((*pField - '0') >= 10)
                goto parsemore5;
            if (*pField != '\0')
                pField += 1;
        parsemore5:
            pField = _ParseNumber(pField, &iTm[0], 2);
            if ((unsigned)(iTm[5] - 1970) >= 138 || iTm[4] <= 0 || iTm[4] >= 13 || iTm[3] <= 0 || iTm[3] >= 32)
                iTm[5] = 0;
            if ((unsigned)iTm[2] >= 24 || iTm[1] < 0 || iTm[1] >= 60 || iTm[0] < 0 || iTm[0] >= 62)
                iTm[5] = 0;
            if (iTm[5] == 0)
                goto haveEpoch;
            iTm[5] -= 1900;
            iTm[4] -= 1;
            uEpoch = _TimeToSecs(iTm);
        }
    }
haveEpoch:
    if (uEpoch != 0)
        return uEpoch;
    uEpoch = uDefault;
    if (uEpoch != 0)
        return uEpoch;
    return LobbyTime();
}

// lee un flotante con signo y decimales (defecto si nulo)
float TagFieldGetFloat(const char *pField, float fDefault)
{
    float fSign = 1.0f;
    int iInt = 0;
    int iFrac = 0;
    int iScale = 1;
    float fVal;
    if (pField == NULL)
        return fDefault;
    if (*pField == '+') {
        pField += 1;
    } else if (*pField == '-') {
        fSign = -1.0f;
        pField += 1;
    }
    while (((unsigned char)*pField - '0') < 10) {
        iInt = iInt * 10 + (*pField & 0x0F);
        pField += 1;
    }
    fVal = (float)iInt;
    if (*pField++ == '.') {
        while (((unsigned char)*pField - '0') < 10) {
            iFrac = iFrac * 10 + (*pField & 0x0F);
            iScale *= 10;
            pField += 1;
        }
    }
    return fSign * (fVal + (float)iFrac / (float)iScale);
}

// lee el campo iIndex-esimo separado por cDelim (con %XX y comillas)
int TagFieldGetDelim(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault, int iIndex, int cDelim)
{
    int iOut;
    if (pField == NULL)
        goto usedefault;
    if (iIndex > 0) {
        for (;;) {
            unsigned char c = *pField;
            int term;
            if (c == '"') {
                term = 0x20;
                pField += 1;
            } else {
                term = 0x21;
            }
            while (*pField != '\0' && *pField != (cDelim & 0xFF) && (unsigned char)*pField >= term)
                pField += 1;
            if (*pField == '"') {
                if (cDelim != '"')
                    pField += 1;
            }
            if (*pField != (cDelim & 0xFF)) {
                pField = NULL;
                goto usedefault;
            }
            pField += 1;
            iIndex -= 1;
            if (iIndex <= 0)
                break;
        }
        if (pField == NULL)
            goto usedefault;
    }
    // copia el campo hallado (como GetString con cDelim de fin)
    {
        int term;
        if (*pField == '"') {
            term = 0x20;
            pField += 1;
        } else {
            term = 0x21;
        }
        if (pBuffer == NULL) {
            iOut = 0;
            for (;;) {
                unsigned char c = *pField & 0xFF;
                if (c == (cDelim & 0xFF))
                    break;
                if (c < term)
                    break;
                if (c != '%') {
                    pField += 1;
                } else if (pField[1] < 0x20) {
                    pField += 1;
                } else if (pField[2] < 0x20) {
                    pField += 1;
                } else {
                    pField += 3;
                }
                iOut += 1;
            }
            return iOut;
        }
        if (iBufferSize <= 0)
            return -1;
        iOut = 1;
        if (iOut >= iBufferSize) {
            *pBuffer = '\0';
            return 0;
        }
        for (;;) {
            unsigned char c = *pField & 0xFF;
            int iVal;
            if (c == (cDelim & 0xFF))
                break;
            if (c < term)
                break;
            if (c != '%') {
                iVal = c;
                pField += 1;
            } else if (pField[1] == '%') {
                iVal = '%';
                pField += 2;
            } else if (pField[1] < 0x20 || pField[2] < 0x20) {
                iVal = '%';
                pField += 1;
            } else {
                iVal = (TagFieldHexVal[(unsigned char)pField[1]] << 4) | TagFieldHexVal[(unsigned char)pField[2]];
                pField += 3;
            }
            *pBuffer++ = (char)iVal;
            iOut += 1;
            if (iOut >= iBufferSize)
                break;
        }
        *pBuffer = '\0';
        return iOut - 1;
    }
usedefault:
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

// parte los segundos (epoch 1970) en un struct tm de 9 enteros (el wday no)
int _SecsToTime(int *pTm, unsigned int uSecs)
{
    int iDays = uSecs / 86400;
    unsigned int uRem = uSecs % 86400;
    int iSec = uRem % 60;
    int iQ = uRem / 60;
    int iMin = iQ % 60;
    int iHour = iQ / 60;
    int iYear = 1970;
    int iYDays;
    pTm[0] = iSec;
    pTm[1] = iMin;
    pTm[2] = iHour;
    for (;;) {
        int iApprox = iDays / 366;
        int iNew = iYear + iApprox;
        int iAdv;
        int iLeapOld;
        int iLeapNew;
        if (iNew == iYear)
            iNew = iYear + 1;
        iAdv = iNew - iYear;
        iLeapOld = (iYear - 1) / 4 - (iYear - 1) / 100 + (iYear - 1) / 400;
        iLeapNew = (iNew - 1) / 4 - (iNew - 1) / 100 + (iNew - 1) / 400;
        iDays = iDays - iAdv * 365 + iLeapOld - iLeapNew;
        iYear = iNew;
        if ((iYear & 3) != 0)
            iYDays = 365;
        else if (iYear % 100 != 0)
            iYDays = 366;
        else if (iYear % 400 != 0)
            iYDays = 365;
        else
            iYDays = 366;
        if (iDays < iYDays)
            break;
    }
    {
        const int *pMonths = TagFieldMonthDays + ((iYDays & 1) ? 12 : 0);
        int iMon = 0;
        pTm[5] = iYear - 1900;
        pTm[7] = iDays;
        pTm[4] = 0;
        while (iDays >= pMonths[iMon]) {
            iDays -= pMonths[iMon];
            iMon += 1;
            pTm[4] = iMon;
        }
        pTm[3] = iDays + 1;
        pTm[8] = 0;
    }
}

// busca por biseccion los segundos cuyo desglose coincide con el tm (0 si no)
unsigned int _TimeToSecs(const int *pTm)
{
    unsigned int uLo = 0;
    unsigned int uHi = 0xFFFFFFFF;
    unsigned int uAve = 0x7FFFFFFF;
    unsigned int uMid;
    int iTm[9];
    int iDiff = 0;
    for (;;) {
        uMid = (uLo >> 1) + uAve + ((uLo & uHi) & 1);
        _SecsToTime(iTm, uMid);
        iDiff = iTm[5] - pTm[5];
        if (iDiff != 0)
            goto adjust;
        iDiff = iTm[4] - pTm[4];
        if (iDiff != 0)
            goto adjust;
        iDiff = iTm[3] - pTm[3];
        if (iDiff != 0)
            goto adjust;
        iDiff = iTm[2] - pTm[2];
        if (iDiff != 0)
            goto adjust;
        iDiff = iTm[1] - pTm[1];
        if (iDiff != 0)
            goto adjust;
        iDiff = iTm[0] - pTm[0];
        if (iDiff == 0)
            return uMid;
    adjust:
        if (uLo == uHi)
            return 0;
        if (iDiff <= 0)
            uLo = uMid + 1;
        else
            uHi = uMid - 1;
        if (uHi < uLo)
            return 0;
        uAve = uHi >> 1;
    }
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
    if (iSize > 0) {
        do {
            unsigned char b = *pSrc++;
            iSize -= 1;
            *pPos++ = TagFieldHexDigits[b >> 4];
            *pPos++ = TagFieldHexDigits[b & 0x0F];
        } while (iSize > 0);
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
    if (*pField == '$') {
        if (pDst == NULL) {
            const char *p = pField + 1;
            while (*p >= '0' && p[1] >= '0')
                p += 2;
            return (p - pField - 1) / 2;
        } else {
            const char *p = pField + 1;
            if ((unsigned char)p[0] < '0')
                return 0;
            if ((unsigned char)p[1] < '0')
                return 0;
            if (pDst >= pEnd)
                return 0;
            for (;;) {
                *pDst = (TagFieldHexVal[(unsigned char)p[0]] << 4) | TagFieldHexVal[(unsigned char)p[1]];
                p += 2;
                pDst += 1;
                if ((unsigned char)p[0] < '0')
                    break;
                if ((unsigned char)p[1] < '0')
                    break;
                if (pDst >= pEnd)
                    break;
            }
            return pDst - (unsigned char *)pBuffer;
        }
    } else if (*pField == '^') {
        if (pDst == NULL) {
            const char *p = pField + 1;
            int iLen;
            int iBits;
            int iRet;
            if (*(const signed char *)p < 0) {
                p += 1;
                while (*(const signed char *)p < 0)
                    p += 1;
            }
            iLen = p - pField - 1;
            iBits = (iLen << 3) - iLen;
            iRet = iBits + 7;
            if (iBits > -1)
                iRet = iBits;
            return iRet >> 3;
        } else {
            const signed char *p = (const signed char *)pField + 1;
            unsigned int uAcc = 0;
            int iBits = 0;
            if (*p >= 0)
                return 0;
            if (pDst >= pEnd)
                return 0;
            for (;;) {
                unsigned int uVal = ((unsigned char)*p) & 0x7F;
                uVal <<= iBits;
                iBits += 7;
                if (iBits >= 8) {
                    uAcc |= uVal;
                    *pDst = (unsigned char)uAcc;
                    iBits -= 8;
                    pDst += 1;
                    uAcc >>= 8;
                }
                p += 1;
                if (*p >= 0)
                    break;
                if (pDst >= pEnd)
                    break;
            }
            return pDst - (unsigned char *)pBuffer;
        }
    }
    return -1;
}
