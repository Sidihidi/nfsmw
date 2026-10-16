#ifndef _lobbyutf8_h
#define _lobbyutf8_h

// DirtySock: las conversiones UTF-8 de lobbyutf8.c (libdirtysock.a(lobbyutf8.o) en el mapa de la A124).
// Los dos tipos son los de los stabs de la A124, que los definen en zFEng justo antes de FEString.cpp: campos con
// tipos de C (unsigned int, unsigned char *, unsigned short), no con typedefs de anchura. Las funciones tienen enlace
// de C; sus argumentos salen del codigo de la A124 (Utf8DecodeToUCS2 recibe destino, tamano y origen).

// tabla de traduccion de UTF-8 a 8 bits
typedef struct Utf8TransTblT {
    unsigned int uRangeBegin;
    unsigned int uRangeEnd;
    unsigned char *pCodeTbl;
} Utf8TransTblT;

// tabla de codificacion de 8 bits a UTF-8
typedef struct Utf8EncodeTblT {
    unsigned short uCodeTbl[256];
} Utf8EncodeTblT;

#ifdef __cplusplus
extern "C" {
#endif

// numero de caracteres de una cadena UTF-8
int Utf8StrLen(const char *pStr);

// codifica un caracter UCS-2 en UTF-8; devuelve los bytes escritos
int Utf8EncodeFromUCS2CodePt(char *pOutPtr, unsigned short uCodePt);

// decodifica una cadena UTF-8 a UCS-2
int Utf8DecodeToUCS2(unsigned short *pOutStr, int iBufLen, const char *pInStr);

// codifica una cadena de 8 bits en UTF-8 con una tabla
int Utf8EncodeFrom8Bit(char *pOutStr, int iBufLen, const char *pInStr, const Utf8EncodeTblT *pEncodeTbl);

// traduce una cadena UTF-8 a 8 bits con una tabla
int Utf8TranslateTo8Bit(char *pOutStr, int iBufLen, const char *pInStr, char cReplace, const Utf8TransTblT *pTransTbl);

#ifdef __cplusplus
}
#endif

#endif
