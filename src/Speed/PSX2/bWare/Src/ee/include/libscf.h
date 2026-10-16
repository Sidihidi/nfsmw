#ifndef _LIBSCF_H
#define _LIBSCF_H

// Solo lo que usa el juego de libscf (psx2/src/platform.cpp): el idioma de la consola.

#define SCE_JAPANESE_LANGUAGE 0
#define SCE_ENGLISH_LANGUAGE 1
#define SCE_FRENCH_LANGUAGE 2
#define SCE_SPANISH_LANGUAGE 3
#define SCE_GERMAN_LANGUAGE 4
#define SCE_ITALIAN_LANGUAGE 5
#define SCE_DUTCH_LANGUAGE 6
#define SCE_PORTUGUESE_LANGUAGE 7

#ifdef __cplusplus
extern "C" {
#endif

int sceScfGetLanguage(void);

#ifdef __cplusplus
}
#endif

#endif
