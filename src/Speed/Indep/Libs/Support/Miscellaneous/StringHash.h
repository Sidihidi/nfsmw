#ifndef SUPPORT_MISC_STRINGHASH_H
#define SUPPORT_MISC_STRINGHASH_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

unsigned int stringhash32(const char *k);
#ifdef EA_BUILD_A124
// La A124 la usa para la suma de comprobacion de los asignadores de atributos (GVault.cpp).
unsigned int stringhash32(const void *k, unsigned int bytes, unsigned int seed);
#endif

inline unsigned int stringhash(const char *k) {
    return stringhash32(k);
}

#endif
