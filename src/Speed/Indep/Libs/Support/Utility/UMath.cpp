#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Libs/Support/Utility/UTypes.h"
#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include <cmath>
#if defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_WIN32)
#include <float.h> // _isnan
#endif

// El objetivo guarda 0x7FF00000 en .rodata bajo el nombre `_Infinity_` (local,
// sin manglar: esta FUERA del namespace), lo carga con `lfs` y lo almacena en
// UMath::Infinity. El front-end no puede plegar el type-pun, y por eso esta
// inicializacion es DINAMICA y aparece en el static-init.
static const unsigned int _Infinity_ = 0x7FF00000;

namespace UMath {

const float Infinity = *reinterpret_cast<const float *>(&_Infinity_);

float Ceil(const float x) {
    return bCeil(x);
}

// STRIPPED

float Mod(const float x, const float e) {
    return bFMod(x, e);
}

bool IsNaN(const float f) {
#if defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_WIN32)
    // La CRT de MSVC (Xenon y PC) no tiene isnanf.
    return _isnan(f) ? true : false;
#else
    return isnanf(f) ? true : false;
#endif
}

// STRIPPED
bool IsNaN(const UMath::Vector3 &v) {}
bool IsNaN(const UMath::Vector4 &v) {}
bool IsNaN(const UMath::Matrix3 &v) {}
bool IsNaN(const UMath::Matrix4 &v) {}
//...

#ifndef EA_PLATFORM_GAMECUBE
// Fuera de GameCube es una funcion de verdad (UMath.h): en SLUS, `j VU0_quattom4`.
void QuaternionToMatrix4(const Vector4 &q, Matrix4 &m) {
    VU0_quattom4(q, m);
}
#endif

// Estaticos de clase que el objetivo emite en el .rodata de zFoundation
// (zFoundation.s: 0x803EB374..0x803EB3F0, 116 B). Sin definicion quedaban
// UNDEF en nuestro .o y el enlazador les daba basura.
const Vector2 Vector2::kZero = {0.0f, 0.0f};
const Vector3 Vector3::kZero = {0.0f, 0.0f, 0.0f};
const Vector4 Vector4::kZero = {0.0f, 0.0f, 0.0f, 0.0f};
const Vector4 Vector4::kIdentity = {0.0f, 0.0f, 0.0f, 1.0f};

const Matrix4 Matrix4::kIdentity = {{1.0f, 0.0f, 0.0f, 0.0f},
                                    {0.0f, 1.0f, 0.0f, 0.0f},
                                    {0.0f, 0.0f, 1.0f, 0.0f},
                                    {0.0f, 0.0f, 0.0f, 1.0f}};

} // namespace UMath

#ifdef EA_BUILD_A124
// Lineas 34-39 del UMath.cpp de la alpha, tras IsNaN: comprueba que la matriz manda el vector de prueba
// (1,1,1,0) a otro de longitud parecida y que su columna w es (0,0,0,1). Solo la usa CARP::Trigger.
bool ValidateMatrix(const UMath::Matrix4 &matrix) {
    static const UMath::Vector4 test1 = {1.0f, 1.0f, 1.0f, 0.0f};
    UMath::Vector4 test2;
    VU0_MATRIX4_vect4mult(test1, matrix, test2);
    float ratio = VU0_v4lengthsquare(test1) / VU0_v4lengthsquare(test2);
    if (ratio < 0.9f || ratio > 1.1f) {
        return false;
    }
    if (fabsf(matrix[0][3]) > 0.001 || fabsf(matrix[1][3]) > 0.001 || fabsf(matrix[2][3]) > 0.001) {
        return false;
    }
    if (fabsf(matrix[3][3]) > 1.1f || fabsf(matrix[3][3]) < 0.9f) {
        return false;
    }
    return true;
}
#endif

#ifdef EA_BUILD_A124
// Lineas 65-67 del UMath.cpp de la alpha: vuelca la rotacion 3x3 y la traslacion en la 4x4. Es la
// unica copia que queda en los cinco informes (en las demas el enlazador la quita con su llamante).
void BuildRotTrans(UMath::Matrix4 &quatTrans, const UMath::Matrix3 &orientation, const UMath::Vector3 &t) {
    for (int mx = 0; mx < 3; mx++) {
        for (int my = 0; my < 3; my++) {
            quatTrans[mx][my] = orientation[mx][my];
        }
    }
    quatTrans[0][3] = quatTrans[1][3] = quatTrans[2][3] = 0.0f;
    quatTrans[3][0] = t.x;
    quatTrans[3][1] = t.y;
    quatTrans[3][2] = t.z;
    quatTrans[3][3] = 1.0f;
}
#endif

void BuildRotate(UMath::Matrix4 &m, float r, float x, float y, float z) {
    float angle;
    float fSin;
    float fCos;
    float invllen;
    float fT;
    float fTX;
    float fTY;
    float fTZ;
    float fSX;
    float fSY;
    float fSZ;
    float *p;

    angle = r * 0.0027777778f;
    if (angle == 0.0f) {
        VU0_MATRIX4Init(m);
        return;
    }

    fSin = VU0_Sin(angle);
    fCos = VU0_Cos(angle);

    invllen = VU0_rsqrt(x * x + y * y + z * z);
    x *= invllen;
    y *= invllen;
    z *= invllen;

    fT = 1.0f - fCos;
    fTX = fT * x;
    fTY = fT * y;
    fTZ = fT * z;
    fSX = fSin * x;
    fSY = fSin * y;
    fSZ = fSin * z;

    p = &m[0][0];

    p[0] = fTX * x + fCos;
    p[1] = fTX * y + fSZ;
    p[2] = fTX * z - fSY;
    p[3] = 0.0f;
    p[4] = fTY * x - fSZ;
    p[5] = fTY * y + fCos;
    p[6] = fTY * z + fSX;
    p[7] = 0.0f;
    p[8] = fTZ * x + fSY;
    p[9] = fTZ * y - fSX;
    p[10] = fTZ * z + fCos;
    p[11] = 0.0f;
    p[12] = 0.0f;
    p[13] = 0.0f;
    p[14] = 0.0f;
    p[15] = 1.0f;
}

void OrthoInverse(UMath::Matrix4 &m) {
    float temp;
    temp = m[0][1];
    m[0][1] = m[1][0];
    m[1][0] = temp;
    temp = m[0][2];
    m[0][2] = m[2][0];
    m[2][0] = temp;
    temp = m[1][2];
    m[1][2] = m[2][1];
    m[2][1] = temp;
    float ntx = -m[3][0];
    float ty = m[3][1];
    float tz = m[3][2];
    m[3][0] = (ntx * m[0][0] - ty * m[1][0]) - tz * m[2][0];
    m[3][1] = (ntx * m[0][1] - ty * m[1][1]) - tz * m[2][1];
    m[3][2] = (ntx * m[0][2] - ty * m[1][2]) - tz * m[2][2];
}
