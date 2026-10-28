#include "StringHash.h"

#ifdef EA_PLATFORM_PLAYSTATION2
#include <types.h>

#define mix64(a, b, c)                                                                                                                               \
    {                                                                                                                                                \
        a = a - b;                                                                                                                                   \
        a = a - c;                                                                                                                                   \
        a = a ^ (c >> 43);                                                                                                                           \
        b = b - c;                                                                                                                                   \
        b = b - a;                                                                                                                                   \
        b = b ^ (a << 9);                                                                                                                            \
        c = c - a;                                                                                                                                   \
        c = c - b;                                                                                                                                   \
        c = c ^ (b >> 8);                                                                                                                            \
        a = a - b;                                                                                                                                   \
        a = a - c;                                                                                                                                   \
        a = a ^ (c >> 38);                                                                                                                           \
        b = b - c;                                                                                                                                   \
        b = b - a;                                                                                                                                   \
        b = b ^ (a << 23);                                                                                                                           \
        c = c - a;                                                                                                                                   \
        c = c - b;                                                                                                                                   \
        c = c ^ (b >> 5);                                                                                                                            \
        a = a - b;                                                                                                                                   \
        a = a - c;                                                                                                                                   \
        a = a ^ (c >> 35);                                                                                                                           \
        b = b - c;                                                                                                                                   \
        b = b - a;                                                                                                                                   \
        b = b ^ (a << 49);                                                                                                                           \
        c = c - a;                                                                                                                                   \
        c = c - b;                                                                                                                                   \
        c = c ^ (b >> 11);                                                                                                                           \
        a = a - b;                                                                                                                                   \
        a = a - c;                                                                                                                                   \
        a = a ^ (c >> 12);                                                                                                                           \
        b = b - c;                                                                                                                                   \
        b = b - a;                                                                                                                                   \
        b = b ^ (a << 18);                                                                                                                           \
        c = c - a;                                                                                                                                   \
        c = c - b;                                                                                                                                   \
        c = c ^ (b >> 22);                                                                                                                           \
    }

unsigned long hash64(const unsigned char *k, unsigned int length, unsigned long initval) {
    unsigned int len = length;
    unsigned long a = initval;
    unsigned long b = initval;
    unsigned long c = 0x9e3779b97f4a7c13ULL;
    while (len > 23) {
        len -= 24;
        a += (unsigned long)k[0] + ((unsigned long)k[1] << 8) + ((unsigned long)k[2] << 16) + ((unsigned long)k[3] << 24) + ((unsigned long)k[4] << 32) +
             ((unsigned long)k[5] << 40) + ((unsigned long)k[6] << 48) + ((unsigned long)k[7] << 56);

        b += (unsigned long)k[8] + ((unsigned long)k[9] << 8) + ((unsigned long)k[10] << 16) + ((unsigned long)k[11] << 24) + ((unsigned long)k[12] << 32) +
             ((unsigned long)k[13] << 40) + ((unsigned long)k[14] << 48) + ((unsigned long)k[15] << 56);

        c += (unsigned long)k[16] + ((unsigned long)k[17] << 8) + ((unsigned long)k[18] << 16) + ((unsigned long)k[19] << 24) + ((unsigned long)k[20] << 32) +
             ((unsigned long)k[21] << 40) + ((unsigned long)k[22] << 48) + ((unsigned long)k[23] << 56);

        mix64(a, b, c);
        k += 24;
    }
    c += length;
    switch (len) {
        case 23:
            c += (unsigned long)k[22] << 56;
        case 22:
            c += (unsigned long)k[21] << 48;
        case 21:
            c += (unsigned long)k[20] << 40;
        case 20:
            c += (unsigned long)k[19] << 32;
        case 19:
            c += (unsigned long)k[18] << 24;
        case 18:
            c += (unsigned long)k[17] << 16;
        case 17:
            c += (unsigned long)k[16] << 8;
        case 16:
            b += (unsigned long)k[15] << 56;
        case 15:
            b += (unsigned long)k[14] << 48;
        case 14:
            b += (unsigned long)k[13] << 40;
        case 13:
            b += (unsigned long)k[12] << 32;
        case 12:
            b += (unsigned long)k[11] << 24;
        case 11:
            b += (unsigned long)k[10] << 16;
        case 10:
            b += (unsigned long)k[9] << 8;
        case 9:
            b += (unsigned long)k[8];
        case 8:
            a += (unsigned long)k[7] << 56;
        case 7:
            a += (unsigned long)k[6] << 48;
        case 6:
            a += (unsigned long)k[5] << 40;
        case 5:
            a += (unsigned long)k[4] << 32;
        case 4:
            a += (unsigned long)k[3] << 24;
        case 3:
            a += (unsigned long)k[2] << 16;
        case 2:
            a += (unsigned long)k[1] << 8;
        case 1:
            a += (unsigned long)k[0];
    }

    mix64(a, b, c);

    return c;
}
#endif

// Bob Jenkins lookup3-style hash (EA variant), Decl: stringhash.cpp: 69
#ifdef EA_PLATFORM_PLAYSTATION2
static unsigned int hash32(const unsigned char *k, unsigned int length, unsigned int initval) {
    unsigned int len = length;
    unsigned int b = 0x9e3779b9;
    unsigned int a = 0x9e3779b9;
    unsigned int c = initval;

    while (len >= 12) {
        len -= 12;
        a += k[0] + ((unsigned int)k[1] << 8) + ((unsigned int)k[2] << 16) + ((unsigned int)k[3] << 24);
        b += k[4] + ((unsigned int)k[5] << 8) + ((unsigned int)k[6] << 16) + ((unsigned int)k[7] << 24);
        c += k[8] + ((unsigned int)k[9] << 8) + ((unsigned int)k[10] << 16) + ((unsigned int)k[11] << 24);

        a -= b; a -= c; a ^= (c >> 13);
        b -= c; b -= a; b ^= (a << 8);
        c -= a; c -= b; c ^= (b >> 13);
        a -= b; a -= c; a ^= (c >> 12);
        b -= c; b -= a; b ^= (a << 16);
        c -= a; c -= b; c ^= (b >> 5);
        a -= b; a -= c; a ^= (c >> 3);
        b -= c; b -= a; b ^= (a << 10);
        c -= a; c -= b; c ^= (b >> 15);

        k += 12;
    }
    c += length;
    switch (len) {
        case 11:
            c += (unsigned int)k[10] << 24;
        case 10:
            c += (unsigned int)k[9] << 16;
        case 9:
            c += (unsigned int)k[8] << 8;
        case 8:
            b += (unsigned int)k[7] << 24;
        case 7:
            b += (unsigned int)k[6] << 16;
        case 6:
            b += (unsigned int)k[5] << 8;
        case 5:
            b += (unsigned int)k[4];
        case 4:
            a += (unsigned int)k[3] << 24;
        case 3:
            a += (unsigned int)k[2] << 16;
        case 2:
            a += (unsigned int)k[1] << 8;
        case 1:
            a += (unsigned int)k[0];
    }

    a -= b; a -= c; a ^= (c >> 13);
    b -= c; b -= a; b ^= (a << 8);
    c -= a; c -= b; c ^= (b >> 13);
    a -= b; a -= c; a ^= (c >> 12);
    b -= c; b -= a; b ^= (a << 16);
    c -= a; c -= b; c ^= (b >> 5);
    a -= b; a -= c; a ^= (c >> 3);
    b -= c; b -= a; b ^= (a << 10);
    c -= a; c -= b; c ^= (b >> 15);

    return c;
}
#else
static unsigned int hash32(const unsigned char *k, unsigned int length, unsigned int initval) {
    unsigned int a, b, c;
    unsigned int remaining = length;

    a = b = 0x9e3779b9;
    c = initval;

    while (remaining > 11) {
        a += k[0] + (k[1] << 8) + (k[2] << 16) + (k[3] << 24);
        b += k[4] + (k[5] << 8) + (k[6] << 16) + (k[7] << 24);
        c += k[8] + (k[9] << 8) + (k[10] << 16) + (k[11] << 24);

        a -= b; a -= c; a ^= (c >> 13);
        b -= c; b -= a; b ^= (a << 8);
        c -= a; c -= b; c ^= (b >> 13);
        a -= b; a -= c; a ^= (c >> 12);
        b -= c; b -= a; b ^= (a << 16);
        c -= a; c -= b; c ^= (b >> 5);
        a -= b; a -= c; a ^= (c >> 3);
        b -= c; b -= a; b ^= (a << 10);
        c -= a; c -= b; c ^= (b >> 15);

        remaining -= 12;
        k += 12;
    }

    c += length;

    switch (remaining) {
    case 11:
        c += k[10] << 24;
    case 10:
        c += k[9] << 16;
    case 9:
        c += k[8] << 8;
    case 8:
        b += k[7] << 24;
    case 7:
        b += k[6] << 16;
    case 6:
        b += k[5] << 8;
    case 5:
        b += k[4];
    case 4:
        a += k[3] << 24;
    case 3:
        a += k[2] << 16;
    case 2:
        a += k[1] << 8;
    case 1:
        a += k[0];
    }

    a -= b; a -= c; a ^= (c >> 13);
    b -= c; b -= a; b ^= (a << 8);
    c -= a; c -= b; c ^= (b >> 13);
    a -= b; a -= c; a ^= (c >> 12);
    b -= c; b -= a; b ^= (a << 16);
    c -= a; c -= b; c ^= (b >> 5);
    a -= b; a -= c; a ^= (c >> 3);
    b -= c; b -= a; b ^= (a << 10);
    c -= a; c -= b; c ^= (b >> 15);

    return c;
}
#endif

// Decl: stringhash.cpp: 126
unsigned int stringhash32(const char *string) {
    if (string != 0 && string[0] != 0) {
        return hash32(reinterpret_cast<const unsigned char *>(string), strlen(string), 0xABCDEF00);
    }
    return 0;
}

#ifdef EA_BUILD_A124
unsigned int stringhash32(const void *k, unsigned int bytes, unsigned int seed) {
    if (k != 0 && bytes != 0) {
        return hash32(reinterpret_cast<const unsigned char *>(k), bytes, seed);
    }
    return 0;
}
#endif
