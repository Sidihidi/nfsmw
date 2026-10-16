// realcore 6.24.00: MEM_fill de PS2 (source/std/cmn/memfill.cpp en el .mdebug de la A124; GameCube compila la suya,
// std/gc/memfill.cpp). Locales de sus stabs: val64 en un registro y val32[2] en la pila.
#include "../../../include/common/realcore/std.h"

#include "types.h"

void MEM_fill(void *dest, unsigned int val, int count) {
    uint64_t val64;
    unsigned int val32[2];

    if (count > 0 && (reinterpret_cast<unsigned int>(dest) & 1) != 0) {
        *static_cast<unsigned char *>(dest) = val;
        dest = static_cast<char *>(dest) + 1;
        count--;
    }
    if (count > 1 && (reinterpret_cast<unsigned int>(dest) & 2) != 0) {
        *static_cast<unsigned short *>(dest) = val;
        dest = static_cast<char *>(dest) + 2;
        count -= 2;
    }
    if (count > 3 && (reinterpret_cast<unsigned int>(dest) & 4) != 0) {
        *static_cast<unsigned int *>(dest) = val;
        dest = static_cast<char *>(dest) + 4;
        count -= 4;
    }
    if (count >= 64) {
        val32[0] = val32[1] = val;
        val64 = *reinterpret_cast<uint64_t *>(val32);
        do {
            static_cast<uint64_t *>(dest)[0] = val64;
            static_cast<uint64_t *>(dest)[1] = val64;
            static_cast<uint64_t *>(dest)[2] = val64;
            static_cast<uint64_t *>(dest)[3] = val64;
            static_cast<uint64_t *>(dest)[4] = val64;
            static_cast<uint64_t *>(dest)[5] = val64;
            static_cast<uint64_t *>(dest)[6] = val64;
            static_cast<uint64_t *>(dest)[7] = val64;
            dest = static_cast<char *>(dest) + 64;
            count -= 64;
        } while (count >= 64);
    }
    if (count >= 16) {
        do {
            static_cast<unsigned int *>(dest)[0] = val;
            static_cast<unsigned int *>(dest)[1] = val;
            static_cast<unsigned int *>(dest)[2] = val;
            static_cast<unsigned int *>(dest)[3] = val;
            dest = static_cast<char *>(dest) + 16;
            count -= 16;
        } while (count >= 16);
    }
    while (count >= 4) {
        *static_cast<unsigned int *>(dest) = val;
        dest = static_cast<char *>(dest) + 4;
        count -= 4;
    }
    if (count >= 2) {
        *static_cast<unsigned short *>(dest) = val;
        dest = static_cast<char *>(dest) + 2;
        count -= 2;
    }
    if (count > 0) {
        *static_cast<unsigned char *>(dest) = val;
    }
}
