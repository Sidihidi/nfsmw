#include "UVectorMath.h"

#ifdef EA_PLATFORM_XENON
#include <ppcintrinsics.h>
#else
#include "UEALibs.hpp"
#endif

// Decl: 44
inline float VU0_v3dotprod(const UMath::Vector3 &a, const UMath::Vector3 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Decl: 59
inline void VU0_v3crossprod(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &dest) {
    // Admite alias (dest igual a a o b), como GameCube y PS2, que leen todo antes de escribir:
    // SuspensionRacer llama Cross(c, forwardNormal, c).
    UMath::Vector3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    dest = result;
}

// En PC va en linea: la UMath::Unit del original (speed.exe 0x40FE20) llama a la raiz y compara con cero en el
// propio cuerpo. En Xbox 360 sigue fuera de linea (UVectorMath.cpp): con LTCG el binario no lo distingue.
#ifndef EA_PLATFORM_XENON
inline float VU0_rsqrt(const float a) {
    float temp = VU0_sqrt(a);
    float zero = 0.0f;
    if (temp == zero)
        return zero;
    return 1.0f / temp;
}
#endif

// Decl: Carbon: UVectorMathCPU.hpp: 94, GC MW: UVectorMathGC.hpp: 26, PS2 MW: UVectorMath.hpp: 354
inline void VU0_v3unitcrossprod(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &dest) {
    VU0_v3crossprod(a, b, dest);
    VU0_v3unit(dest, dest);
}

// Decl: Carbon: UVectorMathCPU.hpp: 129, GC MW: UVectorMathGC.hpp: 220, PS2 MW: UVectorMath.hpp: 621
// Las longitudes de hasta 1e-6 dan cero sin raiz: asi lo hacen la Unit de PC (speed.exe 0x40FE20) y la
// VU0_v3unit de Xbox 360 (0x820F0410).
inline void VU0_v3unit(const UMath::Vector3 &a, UMath::Vector3 &result) {
    float rlen = 0.0f;
    float lensq = VU0_v3lengthsquare(a);
    if (lensq > 0.000001f) {
        rlen = VU0_rsqrt(lensq);
    }
    VU0_v3scale(a, rlen, result);
}

// Decl: Carbon: UVectorMathCPU.hpp: 139, GC MW: UVectorMathGC.hpp: 228, PS2 MW: UVectorMath.hpp: 1203
inline void VU0_v4unit(const UMath::Vector4 &a, UMath::Vector4 &result) {
    float rlen = VU0_rsqrt(VU0_v4lengthsquare(a));
    VU0_v4scale(a, rlen, result);
}

// Decl: Carbon: UVectorMathCPU.hpp: 146, GC MW: UVectorMathGC.hpp: 236, PS2 MW: UVectorMath.hpp: 677
inline void VU0_v4unitxyz(const UMath::Vector4 &a, UMath::Vector4 &result) {
    float rlen = VU0_rsqrt(VU0_v4lengthsquarexyz(a));
    VU0_v4scalexyz(a, rlen, result);
}

// Decl: 155
inline void VU0_v3add(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
}

// Decl: 177
inline void VU0_v3sub(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
}

// Decl: 199
inline void VU0_v3scale(const UMath::Vector3 &a, const float scaleby, UMath::Vector3 &result) {
    result.x = a.x * scaleby;
    result.y = a.y * scaleby;
    result.z = a.z * scaleby;
}

// Decl: 206
inline void VU0_v3scale(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
}

// Decl: 269
inline void VU0_v3scaleadd(const UMath::Vector3 &a, const float scaleby, const UMath::Vector3 &b, UMath::Vector3 &result) {
    // a * scaleby + b, como en GameCube (ps_muls0 sobre a y ps_add con b) y en PS2 (vmulx sobre a y vadd con b)
    result.x = a.x * scaleby + b.x;
    result.y = a.y * scaleby + b.y;
    result.z = a.z * scaleby + b.z;
}

// Decl: Carbon: UVectorMathCPU.hpp: 293, GC MW: UVectorMathGC.hpp: 62, PS2 MW: UVectorMath.hpp: 1223
inline void VU0_v3negate(UMath::Vector3 &result) {
    result.x = -result.x;
    result.y = -result.y;
    result.z = -result.z;
}

// Decl: Carbon: UVectorMathCPU.hpp: 300, GC MW: UVectorMathGC.hpp: 69, PS2 MW: UVectorMath.hpp: 1243
inline void VU0_v4negatexyz(UMath::Vector4 &result) {
    result.x = -result.x;
    result.y = -result.y;
    result.z = -result.z;
}

// Decl: Carbon: UVectorMathCPU.hpp: 324, GC MW: UVectorMathGC.hpp: 243, PS2 MW: UVectorMath.hpp: 1323
inline float VU0_v3distance(const UMath::Vector3 &p1, const UMath::Vector3 &p2) {
    UMath::Vector3 temp;
    VU0_v3sub(p1, p2, temp);
    return VU0_sqrt(VU0_v3lengthsquare(temp));
}

// Decl: Carbon: UVectorMathCPU.hpp: 333, GC MW: UVectorMathGC.hpp: 255, PS2 MW: UVectorMath.hpp: 842
inline float VU0_v4distancexyz(const UMath::Vector4 &p1, const UMath::Vector4 &p2) {
    UMath::Vector4 temp;
    VU0_v4subxyz(p1, p2, temp);
    return VU0_sqrt(VU0_v4lengthsquarexyz(temp));
}

// Decl: Carbon: UVectorMathCPU.hpp: 342, GC MW: UVectorMathGC.hpp: 263, PS2 MW: UVectorMath.hpp: 1437
inline float VU0_v3distancexz(const UMath::Vector3 &p1, const UMath::Vector3 &p2) {
    return VU0_sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.z - p1.z) * (p2.z - p1.z));
}

// Decl: Carbon: UVectorMathCPU.hpp: 348, GC MW: UVectorMathGC.hpp: TODO, PS2 MW: UVectorMath.hpp: TODO
inline float VU0_v4distancexz(const UMath::Vector4 &p1, const UMath::Vector4 &p2) {
    return VU0_sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.z - p2.z) * (p1.z - p2.z));
}

// Decl: Carbon: UVectorMathCPU.hpp: 354, GC MW: UVectorMathGC.hpp: 274, PS2 MW: UVectorMath.hpp: 1014
inline float VU0_v3length(const UMath::Vector3 &a) {
    return VU0_sqrt(VU0_v3lengthsquare(a));
}

// Decl: Carbon: UVectorMathCPU.hpp: 359, GC MW: UVectorMathGC.hpp: 279, PS2 MW: UVectorMath.hpp: 1571
inline float VU0_v3lengthxz(const UMath::Vector3 &a) {
    return VU0_sqrt(a.x * a.x + a.z * a.z);
}

// Como la de GameCube (UVectorMathGC.hpp): UMath::LengthXZ la necesita en todas las plataformas.
inline float VU0_v4lengthxz(const UMath::Vector4 &a) {
    return VU0_sqrt(a.x * a.x + a.z * a.z);
}

// Decl: Carbon: UVectorMathCPU.hpp: 364, GC MW: UVectorMathGC.hpp: 284, PS2 MW: UVectorMath.hpp: TODO
inline float VU0_v4length(const UMath::Vector4 &a) {
    return VU0_sqrt(VU0_v4lengthsquare(a));
}

// Decl: Carbon: UVectorMathCPU.hpp: 374, GC MW: UVectorMathGC.hpp: 294, PS2 MW: UVectorMath.hpp: 1653
inline float VU0_v4lengthxyz(const UMath::Vector4 &a) {
    return VU0_sqrt(VU0_v4lengthsquarexyz(a));
}

// Decl: 379
inline float VU0_v3distancesquare(const UMath::Vector3 &p1, const UMath::Vector3 &p2) {
    return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y) + (p1.z - p2.z) * (p1.z - p2.z);
}

// Decl: 393
inline float VU0_v3distancesquarexz(const UMath::Vector3 &p1, const UMath::Vector3 &p2) {
    return (p1.x - p2.x) * (p1.x - p2.x) + (p1.z - p2.z) * (p1.z - p2.z);
}

// Decl: 405
inline float VU0_v3lengthsquare(const UMath::Vector3 &a) {
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

// Decl: Carbon: UVectorMathCPU.hpp: 415
inline float VU0_v4lengthsquarexyz(const UMath::Vector4 &a) {
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

// Decl: 420
inline float VU0_sqrt(const float a) {
#ifdef EA_PLATFORM_XENON
    return __fsqrts(a);
#else
    // La rsqrt de UEALibs (fuera de linea en zFoundation, speed.exe 0x5C5E80): el juego de PC la llama 208 veces y
    // en zFoundation, que la define, sale en linea.
    return rsqrt(a);
#endif
}

// Decl: 487
inline void VU0_MATRIX3x4_vect3mult(const UMath::Vector3 &v, const UMath::Matrix4 &m, UMath::Vector3 &result) {
    UMath::Vector3 v1 = v;

    result.x = m.v0.x * v1.x + m.v1.x * v1.y + m.v2.x * v1.z;
    result.y = m.v0.y * v1.x + m.v1.y * v1.y + m.v2.y * v1.z;
    result.z = m.v0.z * v1.x + m.v1.z * v1.y + m.v2.z * v1.z;
}

// Decl: Carbon: UVectorMathCPU.hpp: 506, GC MW: UVectorMathGC.hpp: 92, PS2 MW: UVectorMath.hpp: 2061
inline void VU0_MATRIX4_transpose(const UMath::Matrix4 &m, UMath::Matrix4 &result) {
    if (&m == &result) {
        UMath::Matrix4 temp;
        int i;
        int j;
        for (i = 0; i < 4 * 4; ++i) {
            temp.GetElements()[i] = m.GetElements()[i];
        }
        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) {
                result[i][j] = temp[j][i];
            }
        }
    } else {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result[i][j] = m[j][i];
            }
        }
    }
}

// Decl: Carbon: UVectorMathCPU.hpp: 530, GC MW: UVectorMathGC.hpp: 115, PS2 MW: UVectorMath.hpp: 2086
inline void VU0_qtranspose(const RQUAT &a, RQUAT &result) {
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = a.w;
}

// Decl: Carbon: UVectorMathCPU.hpp: 538, GC MW: UVectorMathGC.hpp: 125, PS2 MW: UVectorMath.hpp: 1136
inline void VU0_qmul(const RQUAT &b, const RQUAT &a, RQUAT &dest) {
    UMath::Vector4 result;
    result.x = a.y * b.z - a.z * b.y + a.w * b.x + a.x * b.w;
    result.y = a.z * b.x - a.x * b.z + a.w * b.y + a.y * b.w;
    result.z = a.x * b.y - a.y * b.x + a.w * b.z + a.z * b.w;
    result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;

    dest = result;
}

// Decl: Carbon: UVectorMathCPU.hpp: 584, GC MW: UVectorMathGC.hpp: 169, PS2 MW: UVectorMath.hpp: 2188
inline void VU0_MATRIX4Init(UMath::Matrix4 &dest, const float xx, const float yy, const float zz) {
    dest[0][0] = xx;
    dest[1][1] = yy;
    dest[2][2] = zz;
    dest[3][3] = 1.0f;

    dest[3][2] = 0.0f;
    dest[3][1] = 0.0f;
    dest[3][0] = 0.0f;

    dest[2][3] = 0.0f;
    dest[2][1] = 0.0f;
    dest[2][0] = 0.0f;

    dest[1][3] = 0.0f;
    dest[1][2] = 0.0f;
    dest[1][0] = 0.0f;

    dest[0][3] = 0.0f;
    dest[0][2] = 0.0f;
    dest[0][1] = 0.0f;
}

// Las tres siguientes existian solo en UVectorMathGC.hpp. UMath.h las llama desde
// codigo comun --Init(Matrix4&), Init(Vector4&) y UnitCrossxyz--, asi que sin
// ellas la ruta de CPU (Xbox 360, PC y cualquier host que no sea GC ni PS2) no
// compila. Este fichero NO entra en el build de GameCube, comprobado contra
// build/GOWE69/dep: aqui no se puede mover el DOL.

// Decl: GC MW: UVectorMathGC.hpp: 96
inline void VU0_v4Init(UMath::Vector4 &a) {
    a.x = a.y = a.z = 0.0f;
    a.w = 1.0f;
}

// Decl: GC MW: UVectorMathGC.hpp: 102
inline void VU0_MATRIX4Init(UMath::Matrix4 &dest) {
    dest[0][0] =
    dest[1][1] =
    dest[2][2] =
    dest[3][3] = 1.0f;

    dest[0][1] =
    dest[0][2] =
    dest[0][3] =
    dest[1][0] =
    dest[1][2] =
    dest[1][3] =
    dest[2][0] =
    dest[2][1] =
    dest[2][3] =
    dest[3][0] =
    dest[3][1] =
    dest[3][2] = 0.0f;
}

// Decl: GC MW: UVectorMathGC.hpp: 10
inline void VU0_v4unitcrossprodxyz(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &dest) {
    VU0_v3crossprod(reinterpret_cast<const UMath::Vector3 &>(a), reinterpret_cast<const UMath::Vector3 &>(b), reinterpret_cast<UMath::Vector3 &>(dest));
    VU0_v3unit(reinterpret_cast<const UMath::Vector3 &>(dest), reinterpret_cast<UMath::Vector3 &>(dest));
}

// Decl: Carbon: UVectorMathCPU.hpp: 607, GC MW: UVectorMathGC.hpp: 193, PS2 MW: UVectorMath.hpp: 2234
inline void VU0_v4Copy(const UMath::Vector4 &a, UMath::Vector4 &b) {
    b = a;
}

// Decl: Carbon: UVectorMathCPU.hpp: 612, GC MW: UVectorMathGC.hpp: 198, PS2 MW: UVectorMath.hpp: 2268
inline void VU0_MATRIX4Copy(const UMath::Matrix4 &a, UMath::Matrix4 &b) {
    b = a;
}

// Decl: Carbon: UVectorMathCPU.hpp: 644, GC MW: UVectorMathGC.hpp: 311, PS2 MW: UVectorMath.hpp: 2466
inline void VU0_v3lerp(const UMath::Vector3 &v1, const UMath::Vector3 &v2, const float t, UMath::Vector3 &target) {
    target.x = v1.x + (v2.x - v1.x) * t;
    target.y = v1.y + (v2.y - v1.y) * t;
    target.z = v1.z + (v2.z - v1.z) * t;
}
