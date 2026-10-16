#ifndef TYPE_ALIASES_H
#define TYPE_ALIASES_H

/* Tipos basicos del codec, tal como los listan los stabs de todas las unidades
   de vp6 en el .mdebug de la A124 (PS2, vp6 1.0.3). */
typedef signed char INT8;
typedef short INT16;
typedef int INT32;
typedef unsigned char UINT8;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef int BOOL;
typedef unsigned char BOOLEAN;
#ifdef EA_PLATFORM_PLAYSTATION2
typedef long INT64; /* en el EE `long` ya es de 64 bits (y `long long`, de 128) */
#else
typedef long long INT64;
#endif
typedef double FLOAT64;
typedef float FLOAT32;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif
