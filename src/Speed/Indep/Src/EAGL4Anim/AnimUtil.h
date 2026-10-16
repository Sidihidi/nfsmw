#ifndef EAGL4ANIM_ANIMUTIL_H
#define EAGL4ANIM_ANIMUTIL_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/UTypes.h"
#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"
#include "types.h"

#include <cmath>

// El orden es el del original (la lista de declaraciones del DWARF de GameCube, que coincide con las lineas de la
// A124 y de GameCube: FastSqrt 74, FloatToInt 89, AlignSize16 131, AlignSize4 149, AlignSize2 158, CopyF4 168,
// CopyF3 182, LinearBlendF3 211, FastQuatBlendF4 232, QuatBlendF4 277, EulToQuat 319, Coord3Add 517, QuatMult 749,
// QuatTransformPoint 831 y 859). Las funciones que solo usan IK.cpp y AnimUtil.cpp (estripadas en las tres
// versiones) no tienen codigo en ningun binario: se escriben solo cuando el nombre, la firma y los inline que el
// DWARF anida dentro fijan el cuerpo, y el resto queda declarado sin cuerpo.

namespace EAGL4Anim {

inline float FastSqrt(float x) {
    return sqrtf(x);
}

inline int FloatToInt(float f) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // En las dos PS2 el original convierte SIEMPRE en $f4: 49 de 49 en zEagl4Anim, desde siete registros
    // de origen distintos. Ningun reparto de EE-GCC hace eso; es un asm de EA con el registro escrito.
    // Su forma en C y lo que cuesta: docs/asm-de-ea.md.
    int result;
    asm volatile("cvt.w.s $f4, %1\n\tmfc1 %0, $f4" : "=r"(result) : "f"(f) : "$f4");
    return result;
#else
    return static_cast<int>(f);
#endif
}

// Sin uso en ningun binario (local `trunc' en el DWARF).
inline int RoundFloat(float f);

// Con desplazamientos, no con mascara: en la A124 todos los redondeos de la
// unidad salen `addiu; sra n; sll n' y nunca `li -2; and', que el mismo
// compilador si emite para un `& ~1' en otras unidades. En GameCube las dos
// formas dan la misma `clrrwi'. AlignSize8 no aparece y se queda como estaba.
inline intptr_t AlignSize16(intptr_t size) {
    return ((size + 15) >> 4) << 4;
}

inline intptr_t AlignSize8(intptr_t size) {
    return (size + 7) & ~7;
}

inline intptr_t AlignSize4(intptr_t size) {
    return ((size + 3) >> 2) << 2;
}

inline intptr_t AlignSize2(intptr_t size) {
    return ((size + 1) >> 1) << 1;
}

inline void CopyF4(const float *src, float *dest) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
}

inline void CopyF3(const float *src, float *dest) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
}

inline float LinearBlendF(float w, float d0, float d1) {
    return d0 + w * (d1 - d0);
}

// `d0 + w * (d1 - d0)': el add.s de la A124 lleva d0 delante (TranF3Interp).
inline void LinearBlendF3(float w, const float *d0, const float *d1, float *out) {
    out[0] = d0[0] + w * (d1[0] - d0[0]);
    out[1] = d0[1] + w * (d1[1] - d0[1]);
    out[2] = d0[2] + w * (d1[2] - d0[2]);
}

// En PS2 es una llamada de cola a VU0_fastqslerp (8 B fuera de linea en las dos PS2): EE-GCC no expande una
// funcion con llamada de cola ("function with sibling calls cannot be inlined"), asi que los llamadores hacen
// `jal FastQuatBlendF4'. GameCube expande la mezcla en C.
inline void FastQuatBlendF4(float w, const float *d0, const float *d1, float *out) {
#ifdef EA_PLATFORM_PLAYSTATION2
    VU0_fastqslerp(*reinterpret_cast<const RQUAT *>(d0), *reinterpret_cast<const RQUAT *>(d1), *reinterpret_cast<RQUAT *>(out), w);
#else
    if (d0[0] * d1[0] + d0[1] * d1[1] + d0[2] * d1[2] + d0[3] * d1[3] > 0.0f) {
        out[0] = w * (d1[0] - d0[0]) + d0[0];
        out[1] = w * (d1[1] - d0[1]) + d0[1];
        out[2] = w * (d1[2] - d0[2]) + d0[2];
        out[3] = w * (d1[3] - d0[3]) + d0[3];
    } else {
        out[0] = d0[0] - w * (d1[0] + d0[0]);
        out[1] = d0[1] - w * (d1[1] + d0[1]);
        out[2] = d0[2] - w * (d1[2] + d0[2]);
        out[3] = d0[3] - w * (d1[3] + d0[3]);
    }

    float s = 1.0f / FastSqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2] + out[3] * out[3]);

    out[0] *= s;
    out[1] *= s;
    out[2] *= s;
    out[3] *= s;
#endif
}

// En PS2 tambien sale fuera de linea: `j FastQuatBlendF4' (EulF3Interp la llama con jal).
inline void QuatBlendF4(float w, const float *d0, const float *d1, float *out) {
    FastQuatBlendF4(w, d0, d1, out);
}

// Sin uso en ningun binario (local `diff' en el DWARF).
inline bool FloatEqual(float a, float b, float tol);

inline void EulToQuat(const float *eulData, float *quatData) {
    float ti = eulData[0] * 0.5f;
    float tj = eulData[1] * 0.5f;
    float th = eulData[2] * 0.5f;
    float ci = cosf(ti);
    float cj = cosf(tj);
    float ch = cosf(th);
    float si = sinf(ti);
    float sj = sinf(tj);
    float sh = sinf(th);
    float cc = ci * ch;
    float cs = ci * sh;
    float sc = si * ch;
    float ss = si * sh;

    quatData[0] = cj * sc - sj * cs;
    quatData[1] = cj * ss + sj * cc;
    quatData[2] = cj * cs - sj * sc;
    quatData[3] = cj * cc + sj * ss;
}

// Sin uso en ningun binario (local `float eulRads[3]' en el DWARF).
inline void EulDegToQuat(const float *eulData, float *quatData);

inline float Coord2Length(const UMath::Vector2 &v) {
    return FastSqrt(v.x * v.x + v.y * v.y);
}

// Sin uso en ningun binario (local `len' en el DWARF).
inline void Coord2Normalize(const UMath::Vector2 &v, UMath::Vector2 &result);

inline void Coord2Subtract(const UMath::Vector2 &a, const UMath::Vector2 &b, UMath::Vector2 &result) {
    result.x = a.x - b.x;
    result.y = a.y - b.y;
}

// El DWARF de IK::Init anida FastSqrt (y nada mas) dentro de Coord3Length.
inline float Coord3Length(const UMath::Vector3 &v) {
    return FastSqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float Coord4Length(const UMath::Vector4 &v) {
    return FastSqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

// Sin uso en ningun binario.
inline float Coord4Length2(const UMath::Vector4 &v);

inline float Coord3Dot(const UMath::Vector3 &a, const UMath::Vector3 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline void Coord3Cross(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
}

// Solo la usan IK::Solve y FnPoseBlender::XZProjectAlign (estripadas): el DWARF anida Coord3Cross, Coord3Length
// y Coord3Dot con la local `cross', pero no dice que funcion de la libreria da el angulo.
inline float Coord3Angle(const UMath::Vector3 &a, const UMath::Vector3 &b);

inline void Coord3Scale(const UMath::Vector3 &v, float s, UMath::Vector3 &result) {
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
}

// El DWARF de QuatFromTo anida Coord3Dot, FastSqrt y Coord3Scale dentro de Coord3Normalize, sin locales.
inline void Coord3Normalize(const UMath::Vector3 &v, UMath::Vector3 &result) {
    Coord3Scale(v, 1.0f / FastSqrt(Coord3Dot(v, v)), result);
}

inline void Coord3Add(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
}

inline void Coord4Add(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
}

inline void Coord3Subtract(const UMath::Vector3 &a, const UMath::Vector3 &b, UMath::Vector3 &result) {
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
}

inline void Coord4Subtract(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
}

inline void Coord3Negate(const UMath::Vector3 &p, UMath::Vector3 &negp) {
    negp.x = -p.x;
    negp.y = -p.y;
    negp.z = -p.z;
}

inline void Coord4Negate(const UMath::Vector4 &p, UMath::Vector4 &negp) {
    negp.x = -p.x;
    negp.y = -p.y;
    negp.z = -p.z;
    negp.w = -p.w;
}

inline void Coord4Scale(const UMath::Vector4 &v, float s, UMath::Vector4 &result) {
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    result.w = v.w * s;
}

inline float Coord4Dot(const UMath::Vector4 &a, const UMath::Vector4 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Sin uso en ningun binario.
inline void Coord4Normalize(const UMath::Vector4 &v, UMath::Vector4 &result);

// Sin uso en ningun binario.
inline bool Coord3Equal(const UMath::Vector3 &a, const UMath::Vector3 &b, float tol);

// Sin uso en ningun binario.
inline float Coord3DistToPlane(const UMath::Vector3 &p, float a, float b, float c, float d);

// Solo la usa IK::Solve (estripada); locales `x', `y' y `z' en el DWARF.
inline void Coord3Perpendicular(const UMath::Vector3 &v, UMath::Vector3 &perp);

// Solo la usa IK::Solve (estripada); local `s' y un Coord3Length anidado en el DWARF.
inline void QuatAxisRot(const UMath::Vector3 &axis, float angle, UMath::Vector4 &quat);

// Solo la usa IK::Solve (estripada); locales `f', `t', `d' y `axis', y anida dos Coord3Normalize, Coord3Dot,
// Coord3Perpendicular, dos QuatAxisRot, Coord3Cross y Coord3Angle.
inline void QuatFromTo(const UMath::Vector3 &from, const UMath::Vector3 &to, UMath::Vector4 &quat);

inline void QuatMult(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    result.x = a.x * b.w - a.y * b.z + a.z * b.y + a.w * b.x;
    result.y = a.x * b.z + a.y * b.w - a.z * b.x + a.w * b.y;
    result.z = -a.x * b.y + a.y * b.x + a.z * b.w + a.w * b.z;
    result.w = -a.x * b.x - a.y * b.y - a.z * b.z + a.w * b.w;
}

// El DWARF de IK::Solve anida QuatMult dentro, con la local `temp'.
inline void QuatCompose(UMath::Vector4 &a, const UMath::Vector4 &b) {
    UMath::Vector4 temp;

    QuatMult(a, b, temp);
    a = temp;
}

inline void QuatMultAlias(const UMath::Vector4 &a, const UMath::Vector4 &b, UMath::Vector4 &result) {
    UMath::Vector4 temp;

    QuatMult(a, b, temp);
    result = temp;
}

// Sin uso en ningun binario (local `dot' en el DWARF).
inline void QuatInvert(UMath::Vector4 &q);

// Sin uso en ningun binario (un bloque con `int i' en el DWARF).
inline void PoseBlend(float w, int numBones, const float *sqt0, const float *sqt1, float *result);

// `q.x * s' con `s = 2.0f' sale `add.s x, x, x' (el 2.0 se propaga y GCC lo cambia por una suma), igual que el
// original en AlignVel, BlendFacing y los BlendXXXFacing.
inline void QuatTransformPoint(const UMath::Vector4 &q, const UMath::Vector4 &p, UMath::Vector4 &result) {
    float s = 2.0f;
    float xs = q.x * s, ys = q.y * s, zs = q.z * s;
    float wx = q.w * xs, wy = q.w * ys, wz = q.w * zs;
    float xx = q.x * xs, xy = q.x * ys, xz = q.x * zs;
    float yy = q.y * ys, yz = q.y * zs, zz = q.z * zs;

    result.x = p.x * (1.0f - (yy + zz)) + p.y * (xy - wz) + p.z * (xz + wy);
    result.y = p.x * (xy + wz) + p.y * (1.0f - (xx + zz)) + p.z * (yz - wx);
    result.z = p.x * (xz - wy) + p.y * (yz + wx) + p.z * (1.0f - (xx + yy));
    result.w = 1.0f;
}

inline void QuatTransformPoint(const UMath::Vector4 &q, const UMath::Vector3 &p, UMath::Vector3 &result) {
    float s = 2.0f;
    float xs = q.x * s, ys = q.y * s, zs = q.z * s;
    float wx = q.w * xs, wy = q.w * ys, wz = q.w * zs;
    float xx = q.x * xs, xy = q.x * ys, xz = q.x * zs;
    float yy = q.y * ys, yz = q.y * zs, zz = q.z * zs;

    result.x = p.x * (1.0f - (yy + zz)) + p.y * (xy - wz) + p.z * (xz + wy);
    result.y = p.x * (xy + wz) + p.y * (1.0f - (xx + zz)) + p.z * (yz - wx);
    result.z = p.x * (xz - wy) + p.y * (yz + wx) + p.z * (1.0f - (xx + yy));
}

inline void QuatTransformPointAlias(const UMath::Vector4 &q, const UMath::Vector3 &p, UMath::Vector3 &result) {
    UMath::Vector3 temp;

    QuatTransformPoint(q, p, temp);
    result = temp;
}

}; // namespace EAGL4Anim

#endif
