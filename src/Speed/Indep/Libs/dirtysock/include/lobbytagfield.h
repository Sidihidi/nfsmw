#ifndef _lobbytagfield_h
#define _lobbytagfield_h

// DirtySock: los campos con nombre de lobbytagfield.c
// (libdirtysock.a(lobbytagfield.o) en el mapa de la A124). Un registro es una
// cadena de campos "nombre=valor" separados por '\n'; el valor admite comillas
// y escapes %XX. Los setters devuelven la longitud del registro (o -1).

#ifdef __cplusplus
extern "C" {
#endif

char *TagFieldFind(const char *pRecord, const char *pName);
char *TagFieldFindIdx(const char *pRecord, const char *pName, int iIndex);
int TagFieldDelete(char *pBuffer, const char *pName);
int TagFieldRename(char *pBuffer, int iBufferSize, const char *pOldName, const char *pNewName);
int TagFieldDupl(char *pBuffer, int iBufferSize, const char *pField);
int TagFieldMerge(char *pBuffer, int iBufferSize, const char *pRecord);
int TagFieldSetRaw(char *pBuffer, int iBufferSize, const char *pName, const char *pValue);
int TagFieldGetRaw(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault);
int TagFieldSetNumber(char *pBuffer, int iBufferSize, const char *pName, int iValue);
int TagFieldGetNumber(const char *pField, int iDefault);
int TagFieldSetFlags(char *pBuffer, int iBufferSize, const char *pName, int iFlags);
int TagFieldGetFlags(const char *pField, int iDefault);
int TagFieldSetAddress(char *pBuffer, int iBufferSize, const char *pName, unsigned int uAddr);
unsigned int TagFieldGetAddress(const char *pField, unsigned int uDefault);
int TagFieldSetToken(char *pBuffer, int iBufferSize, const char *pName, int iToken);
int TagFieldGetToken(const char *pField, int iDefault);
int TagFieldSetString(char *pBuffer, int iBufferSize, const char *pName, const char *pValue);
int TagFieldGetString(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault);
int TagFieldSetBinary(char *pBuffer, int iBufferSize, const char *pName, const void *pData, int iSize);
int TagFieldGetBinary(const char *pField, void *pBuffer, int iBufferSize);
int TagFieldGetStructure(const char *pField, void *pBuffer, int iBufferSize, const char *pFormat);
int TagFieldSetEpoch(char *pBuffer, int iBufferSize, const char *pName, unsigned int uEpoch);
unsigned int TagFieldGetEpoch(const char *pField, unsigned int uDefault);
int TagFieldSetFloat(char *pBuffer, int iBufferSize, const char *pName, float fValue);
float TagFieldGetFloat(const char *pField, float fDefault);
int TagFieldPrintf(char *pBuffer, int iBufferSize, const char *pName, const char *pFormat, ...);
int TagFieldGetDelim(const char *pField, char *pBuffer, int iBufferSize, const char *pDefault, int iIndex, int cDelim);

#ifdef __cplusplus
}
#endif

#endif
