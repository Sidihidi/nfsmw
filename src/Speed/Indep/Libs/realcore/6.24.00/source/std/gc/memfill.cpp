#include "../../../include/common/realcore/std.h"

void MEM_fill(void *dest, unsigned int val, int count) {
    char *d = (char *)dest;

    if (count > 0 && ((unsigned int)d & 1) != 0) {
        *d = (char)val;
        ++d;
        --count;
    }
    if (count > 1 && ((unsigned int)d & 2) != 0) {
        *(unsigned short *)d = (unsigned short)val;
        d += 2;
        count -= 2;
    }
    if (count > 3 && ((unsigned int)d & 4) != 0) {
        *(unsigned int *)d = val;
        d += 4;
        count -= 4;
    }
    if (count > 7 && ((unsigned int)d & 8) != 0) {
        *(unsigned int *)d = val;
        ((unsigned int *)d)[1] = val;
        d += 8;
        count -= 8;
    }
    if (count > 15 && ((unsigned int)d & 16) != 0) {
        *(unsigned int *)d = val;
        ((unsigned int *)d)[1] = val;
        ((unsigned int *)d)[2] = val;
        ((unsigned int *)d)[3] = val;
        d += 16;
        count -= 16;
    }
    while (count > 31) {
        ((unsigned int *)d)[0] = val;
        ((unsigned int *)d)[1] = val;
        ((unsigned int *)d)[2] = val;
        ((unsigned int *)d)[3] = val;
        ((unsigned int *)d)[4] = val;
        ((unsigned int *)d)[5] = val;
        ((unsigned int *)d)[6] = val;
        ((unsigned int *)d)[7] = val;
        d += 32;
        count -= 32;
    }
    if (count > 15) {
        ((unsigned int *)d)[0] = val;
        ((unsigned int *)d)[1] = val;
        ((unsigned int *)d)[2] = val;
        ((unsigned int *)d)[3] = val;
        d += 16;
        count -= 16;
    }
    if (count > 7) {
        ((unsigned int *)d)[0] = val;
        ((unsigned int *)d)[1] = val;
        d += 8;
        count -= 8;
    }
    if (count > 3) {
        *(unsigned int *)d = val;
        d += 4;
        count -= 4;
    }
    if (count > 1) {
        *(unsigned short *)d = (unsigned short)val;
        d += 2;
        count -= 2;
    }
    if (count > 0) {
        *d = (char)val;
    }
}
