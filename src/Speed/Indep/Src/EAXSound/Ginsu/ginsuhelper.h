//
//
#ifndef GINSUHELPER_H
#define GINSUHELPER_H

//
#define GINSU_USE_BMATH

#if defined(EA_PLATFORM_WIN32) && defined(_MSC_VER)
// PC: asm de EA (docs/asm-de-ea.md). fistp/fist redondean con el modo de la FPU y MSVC 7.1 no los emite desde C.
inline int IntRound(float x) {
    int i;
    __asm {
        fld x
        fistp i
    }
    return i;
}

inline int IntCeil(float x) {
    int i;
    float d;
    __asm {
        fld x
        fist i
        fisubr i
        fstp d
    }
    return i - (*reinterpret_cast<int *>(&d) >> 31);
}

inline int IntFloor(float x) {
    int i;
    float d;
    __asm {
        fld x
        fist i
        fisub i
        fstp d
    }
    return i + (*reinterpret_cast<int *>(&d) >> 31);
}
#else
inline int IntRound(float x) {
    if (0.0f < x) {
        return static_cast<int>(x + 0.5f);
    } else {
        return static_cast<int>(x - 0.5f);
    }
}

inline int IntCeil(float x) {
    if (0.0f < x) {
        int i = static_cast<int>(x);
        if (static_cast<float>(i) == x) {
            return i;
        } else {
            return i + 1;
        }
    }

    return static_cast<int>(x);
}

inline int IntFloor(float x) {
    if (0.0f <= x) {
        return static_cast<int>(x);
    } else {
        int i = static_cast<int>(x);
        if (static_cast<float>(i) == x) {
            return i;
        } else {
            return i - 1;
        }
    }
}
#endif

#endif
