#ifndef BMATH_HPP
#define BMATH_HPP

#include <math.h>
#include <types.h>

#include "Speed/Indep/bWare/Inc/Strings.hpp"

#ifdef EA_PLATFORM_GAMECUBE
#include <dolphin.h>
#elif defined(EA_PLATFORM_XENON)
#include <ppcintrinsics.h>
#elif defined(EA_PLATFORM_PLAYSTATION2)
#include "Speed/PSX2/bWare/Src/ee/include/eetypes.h"
#elif defined(EA_PLATFORM_WIN32)
// TODO
#else
#error Choose a platform
#endif

static const float PI = M_PI; // Decl: 60

typedef int32 bFix;            // Decl: 71

// Lineas 72-78 del bMath.hpp de la A124 (nombre provisional: una funcion libre en linea no deja nombre en los
// stabs). En PS2 es asm de EA: el cvt.w.s va en el sitio en 232 de sus 265 expansiones (el del compilador, en 12
// de 739) y cada expansion tiene un bloque con `temp' (float) y `result' (int32). Recibe un float: en las dos
// Recalculate de Table el original convierte int -> float -> este inline -> float. GameCube y PC no convierten
// alli (casan con la forma simple) y su DWARF no tiene este inline: fuera de PS2 es un cast que deja los int.
#ifdef EA_PLATFORM_PLAYSTATION2
inline int32 bFloatToInt(float a) {
    float temp;
    int32 result;

    asm("cvt.w.s %0, %1" : "=f"(temp) : "f"(a));
    asm("mfc1 %0, %1" : "=r"(result) : "f"(temp));
    return result;
}
#elif defined(EA_PLATFORM_GAMECUBE)
// En GameCube es una macro: su DWARF no tiene ningun inline bFloatToInt, y como funcion en linea deja copias que el
// original no tiene (AverageAir de CameraAI.cpp, donde las dos PS2 SI expanden el asm: `temp' y `result' en sus stabs).
#define bFloatToInt(a) static_cast<int32>(a)
#else
// Xbox 360 si necesita la funcion: con la macro, MSVC junta el cast a short de SceneryInstance::SetMatrix con este y
// AssignOverrides deja de casar.
template <typename T> inline int32 bFloatToInt(T a) {
    return static_cast<int32>(a);
}
#endif

typedef unsigned short bAngle; // Decl: 143
typedef short bSignedAngle;    // Decl: 144

struct bPolar {
    float r;
    bAngle a;
};

extern unsigned int bDefaultSeed; // Decl: 179

// Los dos tipos de registro de la VU0 de PS2 (definidos abajo). El DWARF de GameCube tambien los tiene, declarados
// y sin cuerpo, justo aqui.
struct bVector3Reg;
struct bVU0Float;

unsigned int bRandom(int range, unsigned int *seed);
float bRandom(float range, unsigned int *seed);
unsigned int bRandom(int range);
float bRandom(float range);
void bSetRandomSeed(unsigned int value, unsigned int *seed);
float bFMod(float a, float b);
float bSin(unsigned short angle);
float bSin(float angle);
float bCos(unsigned short angle);
void bSinCos(float *presult_sin, float *presult_cos, bAngle angle);
bAngle bASin(float x);
bAngle bATan(float x, float y);

void bMathTimingTest();

inline bAngle bACos(float x) {
    return 16384 - bASin(x);
}

inline float bTan(bAngle angle) {
    return bSin(angle) / bCos(angle);
}

inline float bSqrt(float x) {
    const float bSqrtEPS = 5e-11f;

    float y0
#ifdef _MSC_VER
        = 0.0f
#endif
        ;
    float y1;
    float t0;
    float t1;
    float t2;

    float half = 0.5f;
    float one = 1.0f;

#ifdef EA_PLATFORM_GAMECUBE
    // frsqrte/fres/fmuls/fnmsubs son asm de EA: el oficial (SN v1.76) no tiene patron
    // para frsqrte/fres, y con -ffast-math no genera fnmsubs (docs/asm-de-ea.md tiene
    // la forma en C, que son las lineas comentadas, y lo que cuesta). Con el compilador
    // construido desde la fuente (Fase 2: builtins, patron fnmsubs-minus y scheduling
    // t1-antes-t2) la forma en C+builtins da los mismos bytes (familia bSqrt al 100 %);
    // se usa solo ahi para no romper el oficial.
#ifdef SN_SOURCE_TOOLCHAIN
    if (x > bSqrtEPS) {
        y0 = __builtin_frsqrte(x);
        t0 = y0 * y0;
        t1 = y0 * half;
        t2 = one - x * t0;
        y0 += (t2 * t1);

        t0 = y0 * y0;
        t1 = y0 * half;
        t2 = one - x * t0;
        y0 += (t2 * t1);

        y0 *= x;
    } else {
        y0 = 0.0f;
    }
#else
    if (x > bSqrtEPS) {
        asm("frsqrte %0, %1" : "=f"(y0) : "f"(x));
        t0 = y0 * y0;
        // t1 = y0 * half;
        asm("fmuls %0, %1, %2" : "=f"(t1) : "f"(y0), "f"(half));
        // t2 = -(x * t0 - one);
        asm("fnmsubs %0, %1, %2, %3" : "=f"(t2) : "f"(x), "f"(t0), "f"(one));
        y0 += (t2 * t1);
        // asm("fmadds %0, %1, %2, %3" : "=f"(y0) : "f"(t2), "f"(t1), "f"(y0));

        t0 = y0 * y0;
        // t1 = y0 * half;
        asm("fmuls %0, %1, %2" : "=f"(t1) : "f"(y0), "f"(half));
        // t2 = -(x * t0 - one);
        asm("fnmsubs %0, %1, %2, %3" : "=f"(t2) : "f"(x), "f"(t0), "f"(one));
        y0 += (t2 * t1);
        // asm("fmadds %0, %1, %2, %3" : "=f"(y0) : "f"(t2), "f"(t1), "f"(y0));

        y0 *= x;
    } else {
        y0 = 0.0f;
    }
#endif
#elif defined(EA_PLATFORM_XENON)
    y0 = __fsqrts(x); // NFS.exe: fsqrts en linea (HeliRenderConn::OnRender), y el compilador pliega la raiz de 0
#elif defined(EA_PLATFORM_PLAYSTATION2)
    // sqrt.s es asm VOLATIL de EA: UpdateCameraMovers del original conserva un sqrt.s cuyo resultado nadie usa,
    // que sqrtf() borraria (docs/asm-de-ea.md tiene la prueba y lo que cuesta en C: `y0 = sqrtf(x);`).
    asm volatile("sqrt.s %0, %1" : "=f"(y0) : "f"(x));
#elif defined(EA_PLATFORM_WIN32)
    return sqrt(x);
#else
#error Choose a platform
#endif

    return y0;
}

inline int bMin(int a, int b) {
#ifdef EA_PLATFORM_PLAYSTATION2
    int c;
    // b primero, como en `b < a ? b : a`: el original da `pminw c, b, a` (SaveRaceData, los bClamp).
    asm("pminw %0, %1, %2" : "=r"(c) : "r"(b), "r"(a));
    return c;
#else
    if (b < a) {
        return b;
    } else {
        return a;
    }
#endif
}

inline float bMin(float a, float b) {
#ifdef EA_PLATFORM_GAMECUBE
    float c = a - b;
    float d;
    asm("fsel %0, %1, %2, %3" : "=f"(d) : "f"(c), "f"(b), "f"(a));
    return d;
#elif defined(EA_PLATFORM_PLAYSTATION2)
    float f;

    asm("min.s %0, %1, %2" : "=f"(f) : "f"(a), "f"(b));
    return f;
#else
    return a < b ? a : b;
#endif
}

inline int bMax(int a, int b) {
#ifdef EA_PLATFORM_PLAYSTATION2
    int c;

    asm("pmaxw %0, %1, %2" : "=r"(c) : "r"(a), "r"(b));
    return c;
#else
    if (a > b) {
        return a;
    } else {
        return b;
    }
#endif
}

inline float bMax(float a, float b) {
#ifdef EA_PLATFORM_GAMECUBE
    float c = a - b;
    float d;
    asm("fsel %0, %1, %2, %3" : "=f"(d) : "f"(c), "f"(a), "f"(b));
    return d;
#elif defined(EA_PLATFORM_PLAYSTATION2)
    float f;

    asm("max.s %0, %1, %2" : "=f"(f) : "f"(a), "f"(b));
    return f;
#else
    if (a > b) {
        return a;
    } else {
        return b;
    }
#endif
}

inline int bAbs(int a) {
#ifdef EA_PLATFORM_PLAYSTATION2
    int c;
    // Como bMin y bMax: el original saca `negu` y `pmaxw` (max(a, -a)), que EE-GCC no genera desde C.
    asm("pmaxw %0, %1, %2" : "=r"(c) : "r"(a), "r"(-a));
    return c;
#else
    if (a < 0) {
        return -a;
    }
    return a;
#endif
}

inline int bMult(int a, int b) {
    return (static_cast<long long>(a) * b) >> 16;
}

inline float bAbs(float a) {
#ifdef EA_PLATFORM_GAMECUBE
    float f_abs = 0.0f;
    // We are sure they use asm, other options don't match
    asm("fabs %0, %1" : "=f"(f_abs) : "f"(a));
    return f_abs;
#elif defined(EA_PLATFORM_PLAYSTATION2)
    float d;
    asm __volatile__("abs.s %0, %1" : "=f"(d) : "f"(a));
    return d;
#else
    return fabsf(a);
#endif
}

inline float bTruncate(float a) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // asm de EA: cvt.w.s y cvt.s.w sin pasar por un registro entero (en C, floatsisf2 no acepta la entrada en un
    // FPR), con la local `f` que dan los stabs de las dos PS2 y el DWARF de GameCube no tiene.
    float f;
    asm("cvt.w.s %0, %1" : "=f"(f) : "f"(a));
    asm("cvt.s.w %0, %1" : "=f"(a) : "f"(f));
    return a;
#else
    return static_cast<int>(a);
#endif
}

inline float bFloor(float a) {
    float t = bTruncate(a);
    if (t > a) {
        t -= 1.0f;
    }
    return t;
}

inline float bCeil(float a) {
    float t = bTruncate(a);
    if (t < a) {
        t += 1.0f;
    }
    return t;
}

// Linea 220 del bMath.hpp de la A124 (FastPacketBuffer); el DWARF de GameCube la da con la local `c'.
inline unsigned int bAlignUp(unsigned int a, unsigned int align_amount) {
    unsigned int c = (a + align_amount - 1) / align_amount;
    return c * align_amount;
}

inline int bClamp(int a, int MINIMUM, int MAXIMUM) {
    return bMin(bMax(a, MINIMUM), MAXIMUM);
}

// TODO is this order correct?
inline float bClamp(float a, float MINIMUM, float MAXIMUM) {
    return bMin(MAXIMUM, bMax(a, MINIMUM));
}

inline bAngle bDegToAng(float degrees) {
    return static_cast<int>(degrees * 65536.0f) / 360;
}

inline bAngle bRadToAng(float radians) {
    return static_cast<int>(radians * (65536.0f / (2 * PI)));
}

inline float bAngToRad(bAngle angle) {
    return ((float)angle) * 0.0000958738f;
}

inline float bDegToRad(float degrees) {
    return degrees * (2 * PI / 360.0f);
}

inline float bAngToDeg(bAngle angle) {
    return static_cast<unsigned int>(angle) * (360.0f / 65536.0f);
}

inline float bCos(float angle) {
    return bSin(angle + bDegToRad(90.0f));
}

inline float bRadToDeg(float radians) {
    return radians * (360.0f / (2 * PI));
}

// La de bAngle es la misma cuenta pero sobre el angulo SIN signo; esta
// conserva el signo, que es para lo que existe la sobrecarga.
inline float bAngToRad(short angle) {
    return ((float)angle) * 0.0000958738f;
}

inline float bAngToDeg(short angle) {
    return static_cast<int>(angle) * (360.0f / 65536.0f);
}

inline int bEqual(float a, float b, float epsilon) {
    return static_cast<int>(bAbs(a - b) <= epsilon);
}

inline int bGetTablePos(short *table, int num_elements, short element) {
    for (int n = 0; n < num_elements; n++) {
        if (table[n] == element) {
            return n;
        }
    }

    return -1;
}

inline bool bIsInTable(short *table, int num_elements, short element) {
    return bGetTablePos(table, num_elements, element) >= 0;
}

struct bVector2 {
    // total size: 0x8
    float x; // offset 0x0, size 0x4
    float y; // offset 0x4, size 0x4

    // Orden de declaracion del volcado DWARF del original.
    bVector2() {}

    bVector2 operator+() const;

    bVector2(float _x, float _y);

    bVector2 operator+(const bVector2 &v) const;

    bVector2 operator-(const bVector2 &v) const;

    bVector2 operator*(float f) const;

    bVector2 operator-() const;

    bVector2 &operator-=(const bVector2 &v);

    bVector2 &operator+=(const bVector2 &v);

    bVector2 &operator*=(float scale);

    bVector2 &operator/=(float inv_scale);

    int operator==(const bVector2 &v) const;

    bVector2 &operator=(const bVector2 &v);

    // bVector2(const bVector2 &v) {} // compiler generated
};

bVector2 *bNormalize(bVector2 *dest, const bVector2 *v);
bVector2 *bNormalize(bVector2 *dest, const bVector2 *v, float length);
bVector2 *bScaleAdd(bVector2 *dest, const bVector2 *v1, const bVector2 *v2, float scale);

inline bVector2 bVector2::operator+() const {
    return *this;
}

inline bVector2 *bFill(bVector2 *dest, float x, float y) {
    dest->x = x;
    dest->y = y;
    return dest;
}

inline bVector2 *bAdd(bVector2 *dest, const bVector2 *v1, const bVector2 *v2) {
    float x1 = v1->x;
    float y1 = v1->y;
    float x2 = v2->x;
    float y2 = v2->y;

    bFill(dest, x1 + x2, y1 + y2);
    return dest;
}

inline bVector2 bAdd(const bVector2 &v1, const bVector2 &v2) {
    bVector2 dest;
    bAdd(&dest, &v1, &v2);
    return dest;
}

inline bVector2 *bCopy(bVector2 *dest, const bVector2 *v) {
    float x = v->x;
    float y = v->y;
    bFill(dest, x, y);
    return dest;
}

inline bVector2::bVector2(float _x, float _y) {
    bFill(this, _x, _y);
}

inline bVector2 &bVector2::operator=(const bVector2 &v) {
    bCopy(this, &v);
    return *this;
}

inline bVector2 &bVector2::operator+=(const bVector2 &v) {
    bAdd(this, this, &v);
    return *this;
}

inline bVector2 bVector2::operator+(const bVector2 &v) const {
    bVector2 *pv = const_cast<bVector2 *>(&v);
    float x1 = this->x;
    float y1 = this->y;
    float x2 = pv->x;
    float y2 = pv->y;
    float _x = x1 + x2;
    float _y = y1 + y2;
    return bVector2(_x, _y);
}

inline bVector2 bVector2::operator-(const bVector2 &v) const {
    bVector2 *pv = const_cast<bVector2 *>(&v);
    float x1 = this->x;
    float y1 = this->y;
    float x2 = pv->x;
    float y2 = pv->y;
    float _x = x1 - x2;
    float _y = y1 - y2;
    return bVector2(_x, _y);
}

inline bVector2 *bSub(bVector2 *dest, const bVector2 *v1, const bVector2 *v2) {
    float x1 = v1->x;
    float y1 = v1->y;
    float x2 = v2->x;
    float y2 = v2->y;

    bFill(dest, x1 - x2, y1 - y2);
    return dest;
}

inline bVector2 bSub(const bVector2 &v1, const bVector2 &v2) {
    bVector2 dest;
    bSub(&dest, &v1, &v2);
    return dest;
}

inline bVector2 &bVector2::operator-=(const bVector2 &v) {
    bSub(this, this, &v);
    return *this;
}

inline bVector2 *bScale(bVector2 *dest, const bVector2 *v, float scale) {
    float x = v->x;
    float y = v->y;

    dest->x = x * scale;
    dest->y = y * scale;
    return dest;
}

inline bVector2 bScale(const bVector2 &v, float scale) {
    bVector2 dest;
    bScale(&dest, &v, scale);
    return dest;
}

inline bVector2 bVector2::operator*(float f) const {
    return bScale(*this, f);
}

inline bVector2 &bVector2::operator*=(float scale) {
    bScale(this, this, scale);
    return *this;
}

inline float bLength(const bVector2 *v) {
    float x = v->x;
    float y = v->y;
    return bSqrt(x * x + y * y);
}

inline float bLength(const bVector2 &v) {
    return bLength(&v);
}

inline bVector2 bNormalize(const bVector2 &v) {
    bVector2 dest;
    bNormalize(&dest, &v);
    return dest;
}

int bEqual(const bVector2 *v1, const bVector2 *v2, float epsilon);

inline float bDot(const bVector2 *v1, const bVector2 *v2) {
    return v1->x * v2->x + v1->y * v2->y;
}

inline float bCross(const bVector2 *a, const bVector2 *b) {
    return a->x * b->y - b->x * a->y;
}

inline float bDot(const bVector2 &v1, const bVector2 &v2) {
    return bDot(&v1, &v2);
}

// Sin `static` (el DWARF del original la lista como `inline`, no `static inline`): con -fkeep-inline-functions
// una static inline arrastra copia fuera de linea en las 33 unidades de juego, y su bSqrt inlined mete en el
// pool comun de .rodata los floats 5e-11f/0.5f/1.0f/0.0f que el original no tiene (zOnline/zMission: 96 B exactos).
inline float bDistBetween(const bVector2 *v1, const bVector2 *v2) {
    float x = v1->x - v2->x;
    float y = v1->y - v2->y;
    return bSqrt(x * x + y * y);
}

inline float bDistBetween(const bVector2 &v1, const bVector2 &v2) {
    return bDistBetween(&v1, &v2);
}

#ifdef EA_PLATFORM_PLAYSTATION2
// Los operadores de la VU0 del original trabajan con valores de 128 bits en registro, que el compilador reparte
// (-mvu0-use-vf0-vf15) sin pasar por memoria. Los stabs de la A124 dan los dos tipos, cada uno con un uint128 `reg',
// y sus metodos: los operadores de bVector3 devuelven un bVector3Reg, la carga de un bVector3 es la conversion
// bVector3Reg(const bVector3 &) (linea 783 del original) y el guardado bVector3::operator=(bVector3Reg) (795) o el
// constructor bVector3(bVector3Reg) (794), sin pasar por el constructor copia. El constructor copia de bVector3Reg de
// los stabs es el implicito: uno declarado dejaria el tipo en memoria (BLKmode en GCC 2.95), no en un registro.
// bVector4 no tiene tipo de registro propio: su interfaz de 128 bits va en la misma clase (mas abajo).
struct bVU0Float {
    u_long128 reg;

    bVU0Float(u_long128 r);
    bVU0Float(float f);
    operator float() const;
};

struct bVector3;

struct bVector3Reg {
    u_long128 reg;

    bVector3Reg() {}

    bVector3Reg(u_long128 r) {
        reg = r;
    }

    bVector3Reg(const bVector3 &v);
    bVector3Reg operator+(bVector3Reg v) const;
    bVector3Reg operator-(bVector3Reg v) const;
    bVector3Reg operator*(bVU0Float f) const;
};
#endif

// total size: 0x10
struct ALIGN_16 bVector3 {
    float x;   // offset 0x0, size 0x4
    float y;   // offset 0x4, size 0x4
    float z;   // offset 0x8, size 0x4
    float pad; // offset 0xC, size 0x4

    // Orden de declaracion tomado del volcado DWARF del original. En GCC 2.9
    // los cuerpos se compilan en ese orden, asi que mover un miembro cambia
    // que se expande dentro de que.
    bVector3() {}

    // Mas unario: devuelve el valor sin tocar.  No hay otra forma posible.
    bVector3 operator+() const {
        return *this;
    }

    bVector3(float _x, float _y, float _z);
    bVector3(const bVector3 &v);
#ifdef EA_PLATFORM_PLAYSTATION2
    // En PS2 los operadores trabajan con el registro de la VU0 (los stabs de la A124): devuelven un bVector3Reg y
    // el resultado se guarda directamente en el destino, sin pasar por el constructor copia.
    bVector3(bVector3Reg v);
    bVector3 &operator=(bVector3Reg v);
    bVector3Reg operator+(bVector3Reg v) const;
    bVector3Reg operator-(bVector3Reg v) const;
    bVector3Reg operator*(bVU0Float f) const;
    bVector3Reg operator-() const;
    bVector3 &operator+=(bVector3Reg v);
    bVector3 &operator-=(bVector3Reg v);
#endif
    bVector3 &operator=(const bVector3 &v);
    bVector3 &operator*=(float scale);
    bVector3 &operator/=(float inv_scale);
    int operator==(const bVector3 &v) const;
    float &operator[](int index);
#ifndef EA_PLATFORM_PLAYSTATION2
    bVector3 operator+(const bVector3 &v) const;
    bVector3 operator-(const bVector3 &v) const;
    bVector3 operator-() const;
    bVector3 operator*(float f) const;
    bVector3 &operator-=(const bVector3 &v);
    bVector3 &operator+=(const bVector3 &v);
#endif
};

#ifdef EA_PLATFORM_PLAYSTATION2
inline bVU0Float::bVU0Float(float f) {
    u_long128 t;

    asm("mfc1 %0, %1" : "=r"(t) : "f"(f));
    reg = t;
}

inline bVector3Reg bVector3Reg::operator*(bVU0Float f) const {
    u_long128 t;

    asm("vmulx.xyzw %0, %1, %2x" : "=j"(t) : "j"(reg), "j"(f.reg));
    return bVector3Reg(t);
}

inline bVector3::bVector3(bVector3Reg v) {
    asm("sqc2 %1, %0" : "=m"(*this) : "j"(v.reg));
}

// Todas las cargas salen de esta sentencia: GCC 2.95 guarda fichero y linea en el RTL de un asm, asi que solo junta
// (CSE) dos lqc2 del mismo vector si son la misma sentencia, y el original lo hace (GetElevation, bDistBetween).
inline bVector3Reg::bVector3Reg(const bVector3 &v) {
    asm("lqc2 %0, %1" : "=j"(reg) : "m"(v));
}

// 384
inline bVector3Reg bVector3Reg::operator+(bVector3Reg v) const {
    u_long128 t;

    asm("vadd.xyzw %0, %1, %2" : "=j"(t) : "j"(reg), "j"(v.reg));
    return bVector3Reg(t);
}

// 385
inline bVector3Reg bVector3Reg::operator-(bVector3Reg v) const {
    u_long128 t;

    asm("vsub.xyzw %0, %1, %2" : "=j"(t) : "j"(reg), "j"(v.reg));
    return bVector3Reg(t);
}

// 795
inline bVector3 &bVector3::operator=(bVector3Reg v) {
    asm("sqc2 %1, %0" : "=m"(*this) : "j"(v.reg));
    return *this;
}

inline bVU0Float::bVU0Float(u_long128 r) {
    reg = r;
}

// 379: el asm es el mtc1, que lee el valor de 128 bits en un GPR; el qmfc2 que lo saca de la VU0 lo pone el
// compilador (una recarga de 128 bits). La unica local es f (stabs de la A124). Asi el GPR lleva un valor de 128 bits,
// como en eAttachSmear2: qmfc2 s7 y s7 guardado con sq desde ahi hasta el final de zEcstasy.
inline bVU0Float::operator float() const {
    float f;

    asm("mtc1 %1, %0" : "=f"(f) : "r"(reg));
    return f;
}

// 400-402, con las tres locales de los stabs de la A124 (d, t y o)
inline bVU0Float bDot(bVector3Reg a, bVector3Reg b) {
    u_long128 d;
    u_long128 t;
    u_long128 o;

    asm("vmul.xyz %0, %1, %2" : "=j"(t) : "j"(a.reg), "j"(b.reg));
    asm("vaddy.x %0, %1, %1y" : "=j"(d) : "j"(t));
    asm("vaddz.x %0, %1, %2z" : "=j"(o) : "j"(d), "j"(t));
    return bVU0Float(o);
}

// 381. La raiz de la VU0 sale en el mismo registro que su entrada; cuando esta sigue viva, el compilador la copia
// antes con vmove. La local se llama vfq en los stabs.
inline bVU0Float bSqrt(bVU0Float f) {
    u_long128 vfq;

    asm("vsqrt Q, %1x\n\tvwaitq\n\tvaddq.x %0, vf0, Q\n\tvnop\n\tvnop" : "=j"(vfq) : "0"(f.reg));
    return bVU0Float(vfq);
}

// El operador de escala por valor (la linea 447 del original, con 380, 389 y 794): el vector llega ya en un
// registro, asi que quien lo pasa lo carga con un movimiento del compilador y no con el lqc2 en asm de bScale(bVector3
// *, ...). Por eso el planificador los ordena distinto (JumpToJR2Camera).
inline bVector3Reg bScale(bVector3Reg v, float scale) {
    return v * scale;
}

// Producto componente a componente (vmul.xyz, 391)
inline bVector3Reg bScale(bVector3Reg v1, bVector3Reg v2) {
    u_long128 t;

    asm("vmul.xyz %0, %1, %2" : "=j"(t) : "j"(v1.reg), "j"(v2.reg));
    return bVector3Reg(t);
}

// 386
inline bVector3Reg bNeg(bVector3Reg v) {
    u_long128 t;

    asm("vsub.xyzw %0, vf0, %1" : "=j"(t) : "j"(v.reg));
    return bVector3Reg(t);
}

// 411
inline bVector3Reg bCross(bVector3Reg v1, bVector3Reg v2) {
    u_long128 t;

    asm("vopmula.xyz ACC, %1, %2\n\tvopmsub.xyz %0, %2, %1" : "=j"(t) : "j"(v1.reg), "j"(v2.reg));
    return bVector3Reg(t);
}

// 420-423: el producto escalar de los cuatro componentes (bVector4 en registro), que queda en el carril x. Las
// locales son las de los stabs de la A124.
inline bVU0Float bDot(u_long128 a, u_long128 b) {
    u_long128 o;
    u_long128 t;
    u_long128 d;

    asm("vmul.xyzw %0, %1, %2" : "=j"(t) : "j"(a), "j"(b));
    asm("vaddy.x %0, %1, %1y" : "=j"(d) : "j"(t));
    asm("vaddz.x %0, %1, %2z" : "=j"(d) : "j"(d), "j"(t));
    asm("vaddw.x %0, %1, %2w" : "=j"(o) : "j"(d), "j"(t));
    return bVU0Float(o);
}
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
inline bVector3Reg bNormalize(bVector3Reg v) {
    // Como bLength: una sentencia por linea del original (producto escalar, rsqrt y escala), con la rsqrt en el
    // registro de su entrada.
    u_long128 t;
    u_long128 d;
    u_long128 o;

    t = v.reg;
    asm("vmul.xyz %0, %1, %1" : "=j"(o) : "j"(t));
    asm("vaddy.x %0, %1, %1y" : "=j"(d) : "j"(o));
    asm("vaddz.x %0, %1, %2z" : "=j"(d) : "j"(d), "j"(o));
    asm("vrsqrt Q, vf0w, %0x\n\tvwaitq\n\tvaddq.x %0, vf0, Q\n\tvnop\n\tvnop" : "+j"(d));
    asm("vmulx.xyzw %0, %1, %2x" : "=j"(t) : "j"(t), "j"(d));
    return bVector3Reg(t);
}

inline bVector3 *bNormalize(bVector3 *dest, const bVector3 *v) {
    *dest = bNormalize(bVector3Reg(*v));
    return dest;
}

// a / sqrt(b): UNA vrsqrt (Q = fs / sqrt(ft)), como bSqrt pero con el numerador. La salida va atada al
// registro de b (el denominador) y el asm va PARTIDO en dos (vrsqrt sola): asi el planificador puede
// colar instrucciones del llamador entre vrsqrt y vwaitq (MaybeCameraShake: daddu+mul.s). La local se
// llama vfq en los stabs de la A124.
inline bVU0Float bDivSqrt(bVU0Float a, bVU0Float b) {
    u_long128 vfq;

    asm("vrsqrt Q, %1x, %2x" : "=j"(vfq) : "j"(a.reg), "0"(b.reg));
    asm("vwaitq\n\tvaddq.x %0, vf0, Q\n\tvnop\n\tvnop" : "+j"(vfq));
    return bVU0Float(vfq);
}

inline bVector3Reg bNormalize(bVector3Reg v, bVU0Float length) {
    return v * bDivSqrt(length, bDot(v, v));
}

inline bVector3 *bNormalize(bVector3 *dest, const bVector3 *v, float length) {
    *dest = bNormalize(bVector3Reg(*v), length);
    return dest;
}
#else
bVector3 *bNormalize(bVector3 *dest, const bVector3 *v);
bVector3 *bNormalize(bVector3 *dest, const bVector3 *v, float length);
#endif
#ifdef EA_PLATFORM_PLAYSTATION2
inline bVector3 *bScaleAdd(bVector3 *dest, const bVector3 *v1, const bVector3 *v2, float scale) {
    // v2 escalado mas v1, en ese orden de operandos (vadd.xyzw vf2, vf2, vf1 en el original).
    *dest = bVector3Reg(*v2) * scale + bVector3Reg(*v1);
    return dest;
}
#else
bVector3 *bScaleAdd(bVector3 *dest, const bVector3 *v1, const bVector3 *v2, float scale);
#endif
#ifdef EA_PLATFORM_PLAYSTATION2
inline bVector3 *bCross(bVector3 *dest, const bVector3 *v1, const bVector3 *v2) {
    *dest = bCross(bVector3Reg(*v1), bVector3Reg(*v2));
    return dest;
}
#else
bVector3 *bCross(bVector3 *dest, const bVector3 *v1, const bVector3 *v2);
#endif

inline bVector3 *bFill(bVector3 *dest, float x, float y, float z) {
    dest->x = x;
    dest->y = y;
    dest->z = z;
    return dest;
}

inline bVector3 *bCopy(bVector3 *dest, const bVector3 *v) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // La carga es la conversion a bVector3Reg (linea 783 del original) y el guardado su operator= (795): las dos
    // salen de las mismas sentencias que las demas ayudantes, y el compilador reutiliza la carga (CSE).
    *dest = bVector3Reg(*v);
#else
    float x = v->x;
    float y = v->y;
    float z = v->z;
    bFill(dest, x, y, z);
#endif
    return dest;
}

inline bVector3 *bScale(bVector3 *dest, const bVector3 *v, float scale) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // Su linea 813 del original: la carga del vector (783) va delante del mfc1 que pasa el float a un registro de
    // 128 bits de proposito general (380), el compilador lo sube a la VU0 (qmtc2) delante del vmulx (389) y el
    // guardado es el operator= (795).
    *dest = bVector3Reg(*v) * scale;
#else
    float x = v->x;
    float y = v->y;
    float z = v->z;

    dest->x = x * scale;
    dest->y = y * scale;
    dest->z = z * scale;
#endif
    return dest;
}

#ifndef EA_PLATFORM_PLAYSTATION2
inline bVector3 bScale(const bVector3 &v, float scale) {
    bVector3 dest;
    bScale(&dest, &v, scale);
    return dest;
}
#endif

inline bVector3 *bAdd(bVector3 *dest, const bVector3 *v1, const bVector3 *v2) {
#ifdef EA_PLATFORM_PLAYSTATION2
    *dest = bVector3Reg(*v1) + bVector3Reg(*v2);
    return dest;
#else
    float x1 = v1->x;
    float y1 = v1->y;
    float z1 = v1->z;
    float x2 = v2->x;
    float y2 = v2->y;
    float z2 = v2->z;

    bFill(dest, x1 + x2, y1 + y2, z1 + z2);
    return dest;
#endif
}

#ifdef EA_PLATFORM_PLAYSTATION2
inline bVector3Reg bAdd(const bVector3 &v1, const bVector3 &v2) {
    return bVector3Reg(v1) + bVector3Reg(v2);
}
#else
inline bVector3 bAdd(const bVector3 &v1, const bVector3 &v2) {
    bVector3 dest;
    bAdd(&dest, &v1, &v2);
    return dest;
}
#endif

inline bVector3 *bSub(bVector3 *dest, const bVector3 *v1, const bVector3 *v2) {
#ifdef EA_PLATFORM_PLAYSTATION2
    *dest = bVector3Reg(*v1) - bVector3Reg(*v2);
    return dest;
#else
    float x1 = v1->x;
    float y1 = v1->y;
    float z1 = v1->z;
    float x2 = v2->x;
    float y2 = v2->y;
    float z2 = v2->z;

    bFill(dest, x1 - x2, y1 - y2, z1 - z2);
    return dest;
#endif
}

#ifdef EA_PLATFORM_PLAYSTATION2
inline bVector3Reg bSub(const bVector3 &v1, const bVector3 &v2) {
    return bVector3Reg(v1) - bVector3Reg(v2);
}
#else
inline bVector3 bSub(const bVector3 &v1, const bVector3 &v2) {
    bVector3 dest;
    bSub(&dest, &v1, &v2);
    return dest;
}
#endif

inline bVector3 *bNeg(bVector3 *dest, const bVector3 *v) {
#ifdef EA_PLATFORM_PLAYSTATION2
    *dest = bNeg(bVector3Reg(*v));
#else
    float x = v->x;
    float y = v->y;
    float z = v->z;

    bFill(dest, -x, -y, -z);
#endif
    return dest;
}

inline float bDot(const bVector3 *v1, const bVector3 *v2) {
#ifdef EA_PLATFORM_PLAYSTATION2
    return bDot(bVector3Reg(*v1), bVector3Reg(*v2));
#else
    return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
#endif
}

inline float bLength(const bVector3 *v) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // Una sola carga: las dos conversiones son la misma sentencia y el compilador las junta (CSE).
    return bSqrt(bDot(bVector3Reg(*v), bVector3Reg(*v)));
#else
    return bSqrt(bDot(v, v));
#endif
}

inline bVector3 *bScale(bVector3 *dest, const bVector3 *v1, const bVector3 *v2) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // En la VU0 (vmul.xyz, linea 391 del original), con la carga y el guardado de bVector3Reg.
    *dest = bScale(bVector3Reg(*v1), bVector3Reg(*v2));
#else
    float x = v1->x * v2->x;
    float y = v1->y * v2->y;
    float z = v1->z * v2->z;

    dest->x = x;
    dest->y = y;
    dest->z = z;
#endif
    return dest;
}

#ifdef EA_PLATFORM_PLAYSTATION2
// vmini/vmax de la VU0 (bExpandBoundingBox de bVector.cpp, A124 0x1608e0/0x1608a0, 40/64 B): el original hace el minimo
// y el maximo componente a componente en la VU0 con .xyzw, sin pasar por la FPU.
inline bVector3Reg bMin(bVector3Reg v1, bVector3Reg v2) {
    u_long128 t;

    asm("vmini.xyzw %0, %1, %2" : "=j"(t) : "j"(v1.reg), "j"(v2.reg));
    return bVector3Reg(t);
}

inline bVector3Reg bMax(bVector3Reg v1, bVector3Reg v2) {
    u_long128 t;

    asm("vmax.xyzw %0, %1, %2" : "=j"(t) : "j"(v1.reg), "j"(v2.reg));
    return bVector3Reg(t);
}
#endif

inline bVector3 *bMin(bVector3 *dest, const bVector3 *v1, const bVector3 *v2) {
#ifdef EA_PLATFORM_PLAYSTATION2
    *dest = bMin(bVector3Reg(*v1), bVector3Reg(*v2));
#else
    float x = v1->x < v2->x ? v1->x : v2->x;
    float y = v1->y < v2->y ? v1->y : v2->y;
    float z = v1->z < v2->z ? v1->z : v2->z;

    bFill(dest, x, y, z);
#endif
    return dest;
}

inline bVector3 *bMax(bVector3 *dest, const bVector3 *v1, const bVector3 *v2) {
#ifdef EA_PLATFORM_PLAYSTATION2
    *dest = bMax(bVector3Reg(*v1), bVector3Reg(*v2));
#else
    float x = v1->x > v2->x ? v1->x : v2->x;
    float y = v1->y > v2->y ? v1->y : v2->y;
    float z = v1->z > v2->z ? v1->z : v2->z;

    bFill(dest, x, y, z);
#endif
    return dest;
}

#ifndef EA_PLATFORM_PLAYSTATION2
inline bVector3 bNeg(const bVector3 &v) {
    bVector3 dest;

    bNeg(&dest, &v);
    return dest;
}
#endif

#ifndef EA_PLATFORM_PLAYSTATION2
inline bVector3 bCross(const bVector3 &v1, const bVector3 &v2) {
    bVector3 dest;

    bCross(&dest, &v1, &v2);
    return dest;
}
#endif

inline float bDot(const bVector3 &v1, const bVector3 &v2) {
    return bDot(&v1, &v2);
}

int bEqual(const bVector3 *v1, const bVector3 *v2, float epsilon);

inline int bEqual(const bVector3 &v1, const bVector3 &v2, float epsilon) {
    return bEqual(&v1, &v2, epsilon);
}

inline float bLength(const bVector3 &v) {
    return bLength(&v);
}

#ifdef EA_PLATFORM_PLAYSTATION2
// En PS2 va en linea (el original no la emite fuera de linea) y en la VU0: resta (385), producto escalar
// (400-402) y raiz (381).
inline float bDistBetween(const bVector3 *v1, const bVector3 *v2) {
    bVector3Reg d = bVector3Reg(*v1) - bVector3Reg(*v2);

    return bSqrt(bDot(d, d));
}
#else
float bDistBetween(const bVector3 *v1, const bVector3 *v2);
#endif

inline float bDistBetween(const bVector3 &v1, const bVector3 &v2) {
    return bDistBetween(&v1, &v2);
}

inline bVector3 bScale(const bVector3 &v1, const bVector3 &v2) {
    bVector3 dest;

    bScale(&dest, &v1, &v2);
    return dest;
}

#ifdef EA_PLATFORM_PLAYSTATION2
inline bVector3Reg bScaleAdd(const bVector3 &v1, const bVector3 &v2, float scale) {
    return bVector3Reg(v2) * scale + bVector3Reg(v1);
}
#else
inline bVector3 bScaleAdd(const bVector3 &v1, const bVector3 &v2, float scale) {
    bVector3 dest;

    bScaleAdd(&dest, &v1, &v2, scale);
    return dest;
}
#endif

#ifndef EA_PLATFORM_PLAYSTATION2
inline bVector3 bNormalize(const bVector3 &v) {
    bVector3 dest;

    bNormalize(&dest, &v);
    return dest;
}
#endif

inline bVector3 bNormalize(const bVector3 &v, float length) {
    bVector3 dest;
}

inline bVector3 bMin(const bVector3 &v1, const bVector3 &v2) {
    bVector3 dest;
}

inline bVector3 bMax(const bVector3 &v1, const bVector3 &v2) {
    bVector3 dest;
}

// Los 13 miembros de bVector3 se definen aqui, detras de los ayudantes libres
// y en el orden exacto en que los lista el volcado DWARF del original.
inline bVector3::bVector3(float _x, float _y, float _z) {
    bFill(this, _x, _y, _z);
}

inline bVector3::bVector3(const bVector3 &v) {
    bCopy(this, &v);
}

inline bVector3 &bVector3::operator=(const bVector3 &v) {
    bCopy(this, &v);
    return *this;
}

inline bVector3 &bVector3::operator*=(float scale) {
    bScale(this, this, scale);
    return *this;
}

inline bVector3 &bVector3::operator/=(float inv_scale) {
    bScale(this, this, 1.0f / inv_scale);
    return *this;
}

inline int bVector3::operator==(const bVector3 &v) const {
    return x == v.x && y == v.y && z == v.z;
}

inline float &bVector3::operator[](int index) {
    return (&x)[index];
}

#ifdef EA_PLATFORM_PLAYSTATION2
inline bVector3Reg bVector3::operator+(bVector3Reg v) const {
    return bVector3Reg(*this) + v;
}

inline bVector3Reg bVector3::operator-(bVector3Reg v) const {
    return bVector3Reg(*this) - v;
}

inline bVector3Reg bVector3::operator-() const {
    return bNeg(bVector3Reg(*this));
}

inline bVector3Reg bVector3::operator*(bVU0Float f) const {
    return bVector3Reg(*this) * f;
}

inline bVector3 &bVector3::operator-=(bVector3Reg v) {
    *this = bVector3Reg(*this) - v;
    return *this;
}

inline bVector3 &bVector3::operator+=(bVector3Reg v) {
    *this = bVector3Reg(*this) + v;
    return *this;
}
#else
inline bVector3 bVector3::operator+(const bVector3 &v) const {
    return bAdd(*this, v);
}

inline bVector3 bVector3::operator-(const bVector3 &v) const {
    return bSub(*this, v);
}

inline bVector3 bVector3::operator-() const {
    return bNeg(*this);
}

inline bVector3 bVector3::operator*(float f) const {
    return bScale(*this, f);
}

inline bVector3 &bVector3::operator-=(const bVector3 &v) {
    bSub(this, this, &v);
    return *this;
}

inline bVector3 &bVector3::operator+=(const bVector3 &v) {
    bAdd(this, this, &v);
    return *this;
}
#endif

// total size: 0x10
struct ALIGN_16 bVector4 {
    float x; // offset 0x0, size 0x4
    float y; // offset 0x4, size 0x4
    float z; // offset 0x8, size 0x4
    float w; // offset 0xC, size 0x4

    // Orden de declaracion del volcado DWARF del original. Las sobrecargas
    // no-const de operator+/operator- eran duplicados nuestros: el original
    // solo tiene las const.
    bVector4() {}

    bVector4 operator+() const {
        return *this;
    }

    bVector4(float _x, float _y, float _z, float _w);

    bVector4(const bVector4 &v);
#ifdef EA_PLATFORM_PLAYSTATION2
    // En PS2 los stabs de la A124 dan a bVector4 la interfaz de 128 bits de la VU0 (sin tipo de registro propio,
    // a diferencia de bVector3): el constructor y el operator= desde uint128, la conversion a uint128 y los
    // operadores +, - y * por escalar que devuelven uint128. Por eso `a + b` guarda con operator=(uint128) y
    // `V4Mult(a - b, t)` construye un temporal (lineas 1044-1046 del original).
    bVector4(u_long128 v);
    operator u_long128() const;
    bVector4 &operator=(u_long128 v);
    u_long128 operator+(u_long128 v) const;
    u_long128 operator-(u_long128 v) const;
    u_long128 operator*(float f) const;
#endif

    bVector4 &operator=(const bVector4 &v);

    bVector4 &operator-=(const bVector4 &v) {}

    bVector4 &operator+=(const bVector4 &v);

    bVector4 &operator*=(float scale);

    bVector4 &operator/=(float inv_scale) {}

    int operator==(const bVector4 &v) const {}

    float &operator[](int index) {
        return reinterpret_cast<float *>(this)[index];
    }

    const float &operator[](int index) const {
        return reinterpret_cast<const float *>(this)[index];
    }

#ifndef EA_PLATFORM_PLAYSTATION2
    bVector4 operator+(const bVector4 &v) const;

    bVector4 operator-(const bVector4 &v) const;
#endif

    bVector4 operator-() const {
        float x1;
        float y1;
        float z1;
        float w1;
    }

#ifndef EA_PLATFORM_PLAYSTATION2
    bVector4 operator*(const float f) const {
        bVector4 t;
    }
#endif
};

#ifdef EA_PLATFORM_PLAYSTATION2
// 1044
inline bVector4::bVector4(u_long128 v) {
    asm("sqc2 %1, %0" : "=m"(*this) : "j"(v));
}

// 1045: todas las cargas de un bVector4, con la local t de los stabs
inline bVector4::operator u_long128() const {
    u_long128 t;

    asm("lqc2 %0, %1" : "=j"(t) : "m"(*this));
    return t;
}

// 1046
inline bVector4 &bVector4::operator=(u_long128 v) {
    asm("sqc2 %1, %0" : "=m"(*this) : "j"(v));
    return *this;
}

// Las operaciones son las mismas lineas de la VU0 que las de bVector3Reg (384, 385 y 389).
inline u_long128 bVector4::operator+(u_long128 v) const {
    return (bVector3Reg(*this) + bVector3Reg(v)).reg;
}

inline u_long128 bVector4::operator-(u_long128 v) const {
    return (bVector3Reg(*this) - bVector3Reg(v)).reg;
}

inline u_long128 bVector4::operator*(float f) const {
    return (bVector3Reg(*this) * f).reg;
}
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
// En PS2 va en linea y en la VU0 (el original no la emite fuera de linea): carga, producto escalar de cuatro
// (420-423), inversa de la raiz (382) y escala (389).
inline bVector4 *bNormalize(bVector4 *dest, const bVector4 *v) {
    u_long128 t = *v;
    u_long128 d = bDot(t, t).reg;

    asm("vrsqrt Q, vf0w, %0x\n\tvwaitq\n\tvaddq.x %0, vf0, Q\n\tvnop\n\tvnop" : "+j"(d));
    asm("vmulx.xyzw %0, %1, %2x" : "=j"(t) : "j"(t), "j"(d));
    *dest = t;
    return dest;
}
#else
bVector4 *bNormalize(bVector4 *dest, const bVector4 *v);
bVector4 *bScaleAdd(bVector4 *dest, const bVector4 *v1, const bVector4 *v2, float scale);
#endif

// clang-format off
inline bVector4 *bFill(bVector4 *dest, float x, float y, float z, float w) {
    dest->x = x; dest->y = y; dest->z = z; dest->w = w;

    return dest;
}

inline bVector4 *bCopy(bVector4 *dest, const bVector4 *v) {
#ifdef EA_PLATFORM_PLAYSTATION2
    *dest = static_cast<u_long128>(*v);
#else
    float x = v->x; float y = v->y; float z = v->z; float w = v->w;

    bFill(dest, x, y, z, w);
#endif
    return dest;
}
// clang-format on

inline bVector4 *bCopy(bVector4 *dest, const bVector3 *v) {
    float x = v->x;
    float y = v->y;
    float z = v->z;

    bFill(dest, x, y, z, 0.0f);
    return dest;
}

inline bVector4 *bCopy(bVector4 *dest, const bVector3 *v, float w) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // Carga del bVector3 (783), guardado de bVector4 (1046) y la w aparte (1049 del original, Gps::Render).
    *dest = bVector3Reg(*v).reg;
    dest->w = w;
#else
    float x = v->x;
    float y = v->y;
    float z = v->z;

    dest->x = x;
    dest->y = y;
    dest->z = z;
    dest->w = w;
#endif

    return dest;
}

inline bVector4 *bAdd(bVector4 *dest, const bVector4 *v1, const bVector4 *v2) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // En la VU0, cargando v1 antes que v2 (InitEmitterPositions)
    *dest = (bVector3Reg(*v1) + bVector3Reg(*v2)).reg;
    return dest;
#else
    float x1 = v1->x;
    float y1 = v1->y;
    float z1 = v1->z;
    float w1 = v1->w;
    float x2 = v2->x;
    float y2 = v2->y;
    float z2 = v2->z;
    float w2 = v2->w;

    bFill(dest, x1 + x2, y1 + y2, z1 + z2, w1 + w2);
    return dest;
#endif
}

inline bVector4 *bSub(bVector4 *dest, const bVector4 *v1, const bVector4 *v2) {
#ifdef EA_PLATFORM_PLAYSTATION2
    *dest = (bVector3Reg(*v1) - bVector3Reg(*v2)).reg;
    return dest;
#else
    float x1 = v1->x;
    float y1 = v1->y;
    float z1 = v1->z;
    float w1 = v1->w;
    float x2 = v2->x;
    float y2 = v2->y;
    float z2 = v2->z;
    float w2 = v2->w;

    bFill(dest, x1 - x2, y1 - y2, z1 - z2, w1 - w2);
    return dest;
#endif
}

inline bVector4 *bNeg(bVector4 *dest, const bVector4 *v) {
    float x;
    float y;
    float z;
    float w;
}

inline float bDot(const bVector4 *v1, const bVector4 *v2) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // Producto escalar de cuatro en la VU0 (420-423 del original, Slerp)
    return bDot(static_cast<u_long128>(*v1), static_cast<u_long128>(*v2));
#else
    return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z + v1->w * v2->w;
#endif
}

inline float bLength(const bVector4 *v) {
    return bSqrt(bDot(v, v));
}

inline bVector4 *bScale(bVector4 *dest, const bVector4 *v, float scale) {
#ifdef EA_PLATFORM_PLAYSTATION2
    *dest = *v * scale;
#else
    float x = v->x;
    float y = v->y;
    float z = v->z;
    float w = v->w;

    dest->x = x * scale;
    dest->y = y * scale;
    dest->z = z * scale;
    dest->w = w * scale;
#endif
    return dest;
}

#ifdef EA_PLATFORM_PLAYSTATION2
// En PS2 va en linea y en la VU0 (su linea 1058 del original, en FixupEndpoints; no hay copia fuera de linea): v2
// escalado mas v1, en ese orden de operandos, como la de bVector3.
inline bVector4 *bScaleAdd(bVector4 *dest, const bVector4 *v1, const bVector4 *v2, float scale) {
    *dest = (bVector3Reg(*v2) * scale + bVector3Reg(*v1)).reg;
    return dest;
}
#endif

inline bVector4 *bScale(bVector4 *dest, const bVector4 *v1, const bVector4 *v2) {
    float x = v1->x;
    float y = v1->y;
    float z = v1->z;
    float w = v1->w;

    dest->x = x * v2->x;
    dest->y = y * v2->y;
    dest->z = z * v2->z;
    dest->w = w * v2->w;

    return dest;
}

inline bVector4 *bMin(bVector4 *dest, const bVector4 *v1, const bVector4 *v2) {}

inline bVector4 *bMax(bVector4 *dest, const bVector4 *v1, const bVector4 *v2) {}

inline bVector4 bAdd(const bVector4 &v1, const bVector4 &v2) {
    bVector4 dest;
    bAdd(&dest, &v1, &v2);
    return dest;
}

inline bVector4 bSub(const bVector4 &v1, const bVector4 &v2) {
    bVector4 dest;
    bSub(&dest, &v1, &v2);
    return dest;
}

inline bVector4 bNeg(const bVector4 &v) {
    bVector4 dest;
    bNeg(&dest, &v);
    return dest;
}

// No hay a quien reenviar: bCross(bVector4 *, ...) NO existe en el original (ni
// simbolo, ni DIE en el DWARF de GameCube, ni en el .mdebug de PS2), y nadie
// expande esta funcion en ninguna version, asi que no hay asm del que deducir
// el cuerpo. Se queda como esta hasta que aparezca un llamante.
inline bVector4 bCross(const bVector4 &v1, const bVector4 &v2) {
    bVector4 dest;
}

inline float bDot(const bVector4 &v1, const bVector4 &v2) {
    return bDot(&v1, &v2);
}

int bEqual(const bVector4 *v1, const bVector4 *v2, float epsilon);

inline int bEqual(const bVector4 &v1, const bVector4 &v2, float epsilon) {
    return bEqual(&v1, &v2, epsilon);
}

inline float bLength(const bVector4 &v) {
    return bLength(&v);
}

float bDistBetween(const bVector4 *v1, const bVector4 *v2);
inline float bDistBetween(const bVector4 &v1, const bVector4 &v2) {
    return bDistBetween(&v1, &v2);
}

inline bVector4 bScale(const bVector4 &v, float scale) {
    bVector4 dest;
    bScale(&dest, &v, scale);
    return dest;
}

inline bVector4 bScale(const bVector4 &v1, const bVector4 &v2) {
    bVector4 dest;
    bScale(&dest, &v1, &v2);
    return dest;
}

inline bVector4 bScaleAdd(const bVector4 &v1, const bVector4 &v2, float scale) {
    bVector4 dest;
}

inline bVector4 bNormalize(const bVector4 &v) {
    bVector4 dest;
}

inline bVector4 bMin(const bVector4 &v1, const bVector4 &v2) {
    bVector4 dest;
}

inline bVector4 bMax(const bVector4 &v1, const bVector4 &v2) {
    bVector4 dest;
}

inline bVector4::bVector4(const bVector4 &v) {
    bCopy(this, &v);
}

inline bVector4::bVector4(float _x, float _y, float _z, float _w) {
    bFill(this, _x, _y, _z, _w);
}

inline bVector4 &bVector4::operator+=(const bVector4 &v) {
    bAdd(this, this, &v);

    return *this;
}

inline bVector4 &bVector4::operator=(const bVector4 &v) {
    bCopy(this, &v);
    return *this;
}

inline bVector4 &bVector4::operator*=(float scale) {
    bScale(this, this, scale);
    return *this;
}

#ifndef EA_PLATFORM_PLAYSTATION2
inline bVector4 bVector4::operator+(const bVector4 &v) const {
    bVector4 *pv = const_cast<bVector4 *>(&v);
    float x1 = this->x;
    float y1 = this->y;
    float z1 = this->z;
    float w1 = this->w;

    float x2 = pv->x;
    float y2 = pv->y;
    float z2 = pv->z;
    float w2 = pv->w;

    float _x = x1 + x2;
    float _y = y1 + y2;
    float _z = z1 + z2;
    float _w = w1 + w2;

    return bVector4(_x, _y, _z, _w);
}

inline bVector4 bVector4::operator-(const bVector4 &v) const {
    bVector4 *pv = const_cast<bVector4 *>(&v);
    float x1 = this->x;
    float y1 = this->y;
    float z1 = this->z;
    float w1 = this->w;

    float x2 = pv->x;
    float y2 = pv->y;
    float z2 = pv->z;
    float w2 = pv->w;

    float _x = x1 - x2;
    float _y = y1 - y2;
    float _z = z1 - z2;
    float _w = w1 - w2;

    return bVector4(_x, _y, _z, _w);
}
#endif

inline bVector4 &bConvertToBond(bVector4 &dest, const bVector4 &v) {
    float x = v.y;
    float y = v.z;
    float z = v.x;
    float w = v.w;
    dest.x = -x;
    dest.y = y;
    dest.z = z;
    dest.w = w;
    return dest;
}

inline bVector4 &bConvertFromBond(bVector4 &dest, const bVector4 &v) {
    float x = v.z;
    float y = v.x;
    float z = v.y;
    float w = v.w;
    dest.x = x;
    dest.y = -y;
    dest.z = z;
    dest.w = w;
    return dest;
}

inline bVector3 &bConvertFromBond(bVector3 &dest, const bVector3 &v) {
    float x = v.z;
    float y = -v.x;
    float z = v.y;
    dest.x = x;
    dest.y = y;
    dest.z = z;

    return dest;
}

inline bVector3 &bConvertToBond(bVector3 &dest, const bVector3 &v) {
    float x = -v.y;
    float y = v.z;
    float z = v.x;
    dest.x = x;
    dest.y = y;
    dest.z = z;

    return dest;
}

// total size: 0x40
struct bMatrix4 {
    bVector4 v0; // offset 0x0, size 0x10
    bVector4 v1; // offset 0x10, size 0x10
    bVector4 v2; // offset 0x20, size 0x10
    bVector4 v3; // offset 0x30, size 0x10

    bMatrix4() {}

    bMatrix4(const bMatrix4 &m);
    bMatrix4 &operator=(const bMatrix4 &m);

    bVector4 &operator[](int index) {
        return reinterpret_cast<bVector4 *>(this)[index];
    }

    const bVector4 &operator[](int index) const {
        return reinterpret_cast<const bVector4 *>(this)[index];
    }
};

inline bMatrix4 *bCopy(bMatrix4 *dest, const bMatrix4 *v) {
#ifdef EA_PLATFORM_GAMECUBE
    MTX44Copy(*reinterpret_cast<const Mtx44 *>(v), *reinterpret_cast<Mtx44 *>(dest));
#elif defined(EA_PLATFORM_XENON)
    dest->v0 = v->v0; dest->v1 = v->v1; dest->v2 = v->v2; dest->v3 = v->v3; // NFS.exe 0x82145008: fila a fila
#elif defined(EA_PLATFORM_PLAYSTATION2)
    // Las cuatro filas en sentencias separadas, primero las cargas y luego los guardados (los stabs dan cuatro
    // locales de 128 bits en la VU0). Cada carga y cada guardado es la conversion de bVector4 a 128 bits y su
    // operator= (lineas 1045 y 1046 del original, las mismas que en el resto de copias de bVector4). El nop de en
    // medio tambien es del original: en la A124 sale en una linea propia de su bMath.hpp (la 1199, entre las
    // cargas y los guardados) en las 76 funciones que copian una matriz, y el compilador no pone linea a los nop
    // que mete el planificador, asi que es un asm que EA escribio dentro de su bloque de VU0. Sin operandos es
    // volatil: el planificador no lo mueve ni pasa nada por encima (docs/asm-de-ea.md).
    u_long128 t0 = v->v0;
    u_long128 t1 = v->v1;
    u_long128 t2 = v->v2;
    u_long128 t3 = v->v3;

    asm("nop");
    dest->v0 = t0;
    dest->v1 = t1;
    dest->v2 = t2;
    dest->v3 = t3;
#elif defined(EA_PLATFORM_WIN32)
    // Portable copy, without platform intrinsics.
    const float *src = reinterpret_cast<const float *>(v);
    float *dst = reinterpret_cast<float *>(dest);
    for (int i = 0; i < 16; i++)
        dst[i] = src[i];
#else
#error Choose a platform
#endif
    return dest;
}

inline void bIdentity(bMatrix4 *a) {
#ifdef EA_PLATFORM_GAMECUBE
    MTX44Identity(*reinterpret_cast<Mtx44 *>(a));
// X360: la forma de PC (la rama de WIN32 de abajo)
//
#elif defined(EA_PLATFORM_PLAYSTATION2)
    // La semilla (1.0f en la palabra baja) es una constante de 128 bits: el compilador la carga con por + lui
    // en cada identidad y la puede adelantar entre los guardados, como el original.
    u_long128 t = 0x3f800000;

    asm("sq   %1, %0" : "=m"(a->v0) : "r"(t));
    asm("pexew %0, %0" : "+r"(t));
    asm("sq   %1, %0" : "=m"(a->v2) : "r"(t));
    asm("pexcw %0, %0" : "+r"(t));
    asm("sq   %1, %0" : "=m"(a->v1) : "r"(t));
    asm("pextlw %0, %0, $0" : "+r"(t));
    asm("sq   %1, %0" : "=m"(a->v3) : "r"(t));
#elif defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    a->v0.x = 1.0f;
    a->v0.y = 0.0f;
    a->v0.z = 0.0f;
    a->v0.w = 0.0f;

    a->v1.x = 0.0f;
    a->v1.y = 1.0f;
    a->v1.z = 0.0f;
    a->v1.w = 0.0f;

    a->v2.x = 0.0f;
    a->v2.y = 0.0f;
    a->v2.z = 1.0f;
    a->v2.w = 0.0f;

    a->v3.x = 0.0f;
    a->v3.y = 0.0f;
    a->v3.z = 0.0f;
    a->v3.w = 1.0f;
#else
#error Choose a platform
#endif
}

void bConvertFromBond(bMatrix4 &dest, const bMatrix4 &m);
void bConvertToBond(bMatrix4 &dest, const bMatrix4 &m);

inline void eIdentity(bMatrix4 *a) {
    bIdentity(a);
}

inline bMatrix4::bMatrix4(const bMatrix4 &m) {
    bCopy(this, &m);
}

inline bMatrix4 &bMatrix4::operator=(const bMatrix4 &m) {
    bCopy(this, &m);
    return *this;
}

// UNUSED
inline bMatrix4 *bCopy(bMatrix4 *dest, const bMatrix4 *v, const bVector4 *position) {
    dest->v0 = v->v0;
    dest->v1 = v->v1;
    dest->v2 = v->v2;
    dest->v3 = *position;
    return dest;
}

inline bMatrix4 *bCopy(bMatrix4 *dest, const bMatrix4 *v, const bVector3 *position) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // Como la copia de matrices: cuatro cargas (la posicion tambien con la conversion de bVector4, la 1045 del
    // original), el nop de EA (su linea 1206) y los cuatro guardados; la w de la posicion se pone despues (1210).
    u_long128 t0 = v->v0;
    u_long128 t1 = v->v1;
    u_long128 t2 = v->v2;
    u_long128 t3 = *reinterpret_cast<const bVector4 *>(position);

    asm("nop");
    dest->v0 = t0;
    dest->v1 = t1;
    dest->v2 = t2;
    dest->v3 = t3;
    dest->v3.w = 1.0f;
#else
    dest->v0 = v->v0;
    dest->v1 = v->v1;
    dest->v2 = v->v2;
    bCopy(&dest->v3, position, 1.0f);
#endif
    return dest;
}

void bMulMatrix(bMatrix4 *dest, const bMatrix4 *a, const bMatrix4 *b);
void bMulMatrix(bVector3 *dest, const bMatrix4 *a, const bVector3 *b);
void bMulMatrix(bVector4 *dest, const bMatrix4 *a, const bVector4 *b);

bMatrix4 *bTransposeMatrix(bMatrix4 *dest, const bMatrix4 *m);
void bInvertMatrix(bMatrix4 *dest, const bMatrix4 *src);

void bConvertToBond(bMatrix4 &dest, const bMatrix4 &m);
void bConvertFromBond(bMatrix4 &dest, const bMatrix4 &m);

// total size: 0x10
#ifdef EA_PLATFORM_PLAYSTATION2
// En PS2 va alineado como bVector4, con el que se mezcla (Slerp): el original copia el cuaternion con ld/sd
// (`r = qtemp'), que exige una alineacion de 8 o mas, y bIdentityQuaternion esta en una direccion multiplo de 16.
struct ALIGN_16 bQuaternion {
#else
struct bQuaternion {
#endif

    bQuaternion() {}

    bQuaternion(float _x, float _y, float _z, float _w) {
        this->x = _x;
        this->y = _y;
        this->z = _z;
        this->w = _w;
    }

    bQuaternion(const bMatrix4 &m);

    bQuaternion &Slerp(bQuaternion &r, const bQuaternion &target, float t) const;

    void GetMatrix(bMatrix4 *mat) const {
        return this->GetMatrix(*mat);
    }

    void GetMatrix(bMatrix4 &mat) const;

    float x; // offset 0x0, size 0x4
    float y; // offset 0x4, size 0x4
    float z; // offset 0x8, size 0x4
    float w; // offset 0xC, size 0x4
};

inline void bQuaternion::GetMatrix(bMatrix4 &mat) const {
    float x2 = x + x;
    float y2 = y + y;
    float z2 = z + z;

    float xx2 = x * x2;
    float yy2 = y * y2;
    float xy2 = x * y2;
    float xz2 = x * z2;
    float yz2 = y * z2;
    float zz2 = z * z2;
    float wx2 = w * x2;
    float wy2 = w * y2;
    float wz2 = w * z2;

    mat.v0.x = 1.0f - yy2 - zz2;
    mat.v1.x = xy2 - wz2;
    mat.v2.x = xz2 + wy2;
    mat.v3.x = 0.0f;

    mat.v0.y = xy2 + wz2;
    mat.v1.y = 1.0f - xx2 - zz2;
    mat.v2.y = yz2 - wx2;
    mat.v3.y = 0.0f;

    mat.v0.z = xz2 - wy2;
    mat.v1.z = yz2 + wx2;
    mat.v2.z = 1.0f - xx2 - yy2;
    mat.v3.z = 0.0f;

    mat.v0.w = 0.0f;
    mat.v1.w = 0.0f;
    mat.v2.w = 0.0f;
    mat.v3.w = 1.0f;
}

void bMatrixToQuaternion(bQuaternion &quat, const bMatrix4 &m);

inline bQuaternion::bQuaternion(const bMatrix4 &m) {
    bMatrixToQuaternion(*this, m);
}

inline void bMemZero(void *dest, unsigned int size) {
    bMemSet(dest, 0, size);
}

class bBitTable {
  public:
    bBitTable() {
        Bits = nullptr;
        NumBits = 0;
    }

    bBitTable(void *mem, int num_bits) {
        Init(mem, num_bits);
    }

    void Init(void *mem, int num_bits) {
        Bits = reinterpret_cast<uint8 *>(mem);
        NumBits = num_bits;
        ClearTable();
    }

    void ClearTable();

    void Set(int bit) {
        unsigned char *p = &Bits[bit >> 3];
        *p |= static_cast<uint8>(1 << (bit & 7));
    }

    // void Clear(int bit) {}

    int IsSet(int bit) {
        unsigned char *p = &Bits[bit >> 3];
        return *p & (1 << (bit & 7));
    }

  private:
    int NumBits; // offset 0x0, size 0x4
    uint8 *Bits; // offset 0x4, size 0x4
};

void hermite_basis(bMatrix4 *b, bMatrix4 *p, float u1, float u2, float u3, float u4);
void hermite_parameter(bVector4 *dest, const bMatrix4 *b, float t);

void bExpandBoundingBox(bVector2 *bbox_min, bVector2 *bbox_max, const bVector2 *point);
void bExpandBoundingBox(bVector3 *bbox_min, bVector3 *bbox_max, const bVector3 *bbox2_min, const bVector3 *bbox2_max);
void bExpandBoundingBox(bVector3 *bbox_min, bVector3 *bbox_max, const bVector3 *point, float extra_width);
void bInitializeBoundingBox(bVector2 *bbox_min, bVector2 *bbox_max);
void bInitializeBoundingBox(bVector2 *bbox_min, bVector2 *bbox_max, const bVector2 *point);
void bInitializeBoundingBox(bVector3 *bbox_min, bVector3 *bbox_max);
void bInitializeBoundingBox(bVector3 *bbox_min, bVector3 *bbox_max, const bVector3 *point);
int bBoundingBoxIsInside(const bVector2 *bbox_min, const bVector2 *bbox_max, const bVector2 *point, float extra_width);
int bBoundingBoxIsInside(const bVector3 *bbox_min, const bVector3 *bbox_max, const bVector3 *point, float extra_width);
int bBoundingBoxOverlapping(const bVector2 *bbox_min, const bVector2 *bbox_max, const bVector2 *bbox2_min, const bVector2 *bbox2_max);
bool bIsPointInPoly(const bVector2 *point, const bVector2 *points, int num_points);
bool bIsPointInPoly(const bVector2 *point, const bVector3 *points, int num_points);
float bDistToLine(const bVector2 *point, const bVector2 *line_p1, const bVector2 *line_p2);

extern bVector3 ZeroVector;

#endif
