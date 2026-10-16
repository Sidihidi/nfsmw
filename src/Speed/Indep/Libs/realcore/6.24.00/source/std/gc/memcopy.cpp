#include "../../../include/common/realcore/std.h"

void MEM_copy(void *dest, const void *source, int count) {
    char *dst = (char *)dest;
    const char *src = (const char *)source;
    int diff = (int)src - (int)dst;

    if (count == 0) {
        return;
    }
    if (diff == 0) {
        return;
    }

    if ((diff & 1) != 0) {
        while (count > 3) {
            char b0 = src[0];
            char b1 = src[1];
            char b2 = src[2];
            char b3 = src[3];
            dst[0] = b0;
            dst[1] = b1;
            dst[2] = b2;
            dst[3] = b3;
            src += 4;
            dst += 4;
            count -= 4;
        }
        while (count > 0) {
            *dst = *src;
            dst++;
            src++;
            count--;
        }
        return;
    }

    if ((diff & 2) != 0) {
        const short *s = (const short *)src;
        short *d = (short *)dst;

        if (((int)src & 1) != 0 && count > 0) {
            *(char *)d = *(const char *)s;
            d = (short *)((char *)d + 1);
            s = (const short *)((const char *)s + 1);
            count -= 1;
        }
        while (count > 7) {
            int v0 = s[0];
            int v1 = s[1];
            int v2 = s[2];
            int v3 = s[3];
            d[0] = v0;
            d[1] = v1;
            d[2] = v2;
            d[3] = v3;
            d += 4;
            s += 4;
            count -= 8;
        }
        while (count > 1) {
            *(unsigned short *)d = *(unsigned short *)s;
            d++;
            s++;
            count -= 2;
        }
        if (count > 0) {
            *(char *)d = *(const char *)s;
        }
        return;
    }

    if (((int)src & 1) != 0 && count > 0) {
        *dst = *src;
        dst++;
        src++;
        count--;
    }
    if (((int)src & 2) != 0 && count > 1) {
        *(unsigned short *)dst = *(unsigned short *)src;
        dst += 2;
        src += 2;
        count -= 2;
    }
    while (count > 15) {
        int w0 = ((const int *)src)[0];
        int w1 = ((const int *)src)[1];
        int w2 = ((const int *)src)[2];
        int w3 = ((const int *)src)[3];
        ((int *)dst)[0] = w0;
        ((int *)dst)[1] = w1;
        ((int *)dst)[2] = w2;
        ((int *)dst)[3] = w3;
        dst += 16;
        src += 16;
        count -= 16;
    }
    while (count > 3) {
        *(int *)dst = *(const int *)src;
        dst += 4;
        src += 4;
        count -= 4;
    }
    if (count > 1) {
        *(unsigned short *)dst = *(unsigned short *)src;
        dst += 2;
        src += 2;
        count -= 2;
    }
    if (count > 0) {
        *dst = *src;
    }
}
