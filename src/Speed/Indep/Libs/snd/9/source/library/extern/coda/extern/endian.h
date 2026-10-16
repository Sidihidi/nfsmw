#ifndef SND_CODA_EXTERN_ENDIAN_H
#define SND_CODA_EXTERN_ENDIAN_H

// Lectura de datos de 16 bits sin alinear y con orden de bytes conocido (C:/packages/snd/9.06.00-ps2/source/library/
// extern/coda/extern/endian.h en el .mdebug de la A124). La usan los decodificadores de coda (eaxadecf.cpp, mtdecf.cpp).

inline void CopyU(short &dst, short &src)
{
    ((unsigned char *)&dst)[0] = ((unsigned char *)&src)[0];
    ((unsigned char *)&dst)[1] = ((unsigned char *)&src)[1];
}

// PutUB/PutUL escriben un dato que viene en big/little-endian: en GameCube y Xbox 360 (big-endian) el primero es una
// copia y el segundo da la vuelta a los bytes; en PS2 y PC (little-endian) al reves.
inline void Reverse(short &dst, short &src)
{
    ((unsigned char *)&dst)[0] = ((unsigned char *)&src)[1];
    ((unsigned char *)&dst)[1] = ((unsigned char *)&src)[0];
}

inline void PutUB(short &dst, short &src)
{
#if defined(EA_PLATFORM_GAMECUBE) || defined(EA_PLATFORM_XENON)
    CopyU(dst, src);
#else
    Reverse(dst, src);
#endif
}

inline void PutUL(short &dst, short &src)
{
#if defined(EA_PLATFORM_GAMECUBE) || defined(EA_PLATFORM_XENON)
    Reverse(dst, src);
#else
    CopyU(dst, src);
#endif
}

inline void PutU(bool bigendian, short &dst, short &src)
{
    if (bigendian)
        PutUB(dst, src);
    else
        PutUL(dst, src);
}

#endif
