#ifndef WORLD_WWORLDMATH_H
#define WORLD_WWORLDMATH_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

namespace WWorldMath {

inline float pow2(float a) {
    return a * a;
}

inline float wmin(const float &a, const float &b) {
    return a < b ? a : b;
}

inline float wmax(const float &a, const float &b) {
    return a > b ? a : b;
}

inline bool InCircle(float x, float y, float cx, float cy, float r) {
    return (cx - x) * (cx - x) + (cy - y) * (cy - y) < r * r;
}

inline float InvSqrt(const float f) {
    return VU0_rsqrt(f);
}

inline bool PtsEqual(const UMath::Vector3 &p0, const UMath::Vector3 &p1, float tolerance) {
    const float kTolerance = tolerance;
    return fabsf(p0.x - p1.x) < kTolerance && fabsf(p0.y - p1.y) < kTolerance && fabsf(p0.z - p1.z) < kTolerance;
}

inline float PtDir4(const UMath::Vector4 &p1, const UMath::Vector4 &p2, const UMath::Vector3 &tp) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // Lineas 145-157 del original, con la VU0 y los vf repartidos por el compilador. Las tres cargas (146-148) son
    // de C y no asm: en InTri el compilador reaprovecha las de la llamada anterior aunque vengan de otra linea, cosa
    // que con un asm no haria. Despues, las dos restas en xz (153 y 154), el producto cruzado en x (156) y la salida
    // (157). Los stabs dan va..vf como u_long128 en registros de la VU0 y result como float.
    // La salida es la de bVU0Float::operator float (bMath.hpp, 379): el asm es el mtc1 y el qmfc2 lo pone el
    // compilador, asi que el GPR lleva un valor de 128 bits. Lo prueba SLUS: FindFaceInTriStrip deja ese valor en s0,
    // lo guarda con sq, y desde ahi todo zWorld2 guarda s0 con sq (el sq pegajoso, +195 funciones en SLUS).
    u_long128 va;
    u_long128 vb;
    u_long128 vc;
    u_long128 vd;
    u_long128 ve;
    u_long128 vf;
    float result;

    va = *reinterpret_cast<const u_long128 *>(&p1);
    vb = *reinterpret_cast<const u_long128 *>(&p2);
    vc = *reinterpret_cast<const u_long128 *>(&tp);
    asm("vsub.xz %0, %1, %2" : "=j"(vd) : "j"(va), "j"(vb));
    asm("vsub.xz %0, %1, %2" : "=j"(ve) : "j"(vc), "j"(vb));
    asm("vmulaz.x ACC, %1, %2z\n\tvmsubz.x %0, %2, %1z" : "=j"(vf) : "j"(vd), "j"(ve));
    asm("mtc1 %1, %0" : "=f"(result) : "r"(vf));
    return result;
#else
    return (p1.x - p2.x) * (tp.z - p2.z) - (tp.x - p2.x) * (p1.z - p2.z);
#endif
}

inline bool InTri(const UMath::Vector3 &pt, const UMath::Vector4 *pts) {
    if (PtDir4(pts[0], pts[1], pt) <= 0.0f) {
        if (PtDir4(pts[1], pts[2], pt) <= 0.0f && PtDir4(pts[2], pts[0], pt) <= 0.0f) {
            return true;
        } else {
            return false;
        }
    } else {
        if (PtDir4(pts[1], pts[2], pt) >= 0.0f && PtDir4(pts[2], pts[0], pt) >= 0.0f) {
            return true;
        } else {
            return false;
        }
    }
}

inline bool InTriR(const UMath::Vector3 &pt, const UMath::Vector4 *pts) {
    return PtDir4(pts[0], pts[1], pt) <= 0.0f && PtDir4(pts[1], pts[2], pt) <= 0.0f && PtDir4(pts[2], pts[0], pt) <= 0.0f;
}

inline bool InTriL(const UMath::Vector3 &pt, const UMath::Vector4 *pts) {
    return 0.0f <= PtDir4(pts[0], pts[1], pt) && 0.0f <= PtDir4(pts[1], pts[2], pt) && 0.0f <= PtDir4(pts[2], pts[0], pt);
}

float GetPlaneY(const UMath::Vector3 &normal, const UMath::Vector3 &pointOnPlane, const UMath::Vector3 &pt);
bool IntersectCircle(float x1, float y1, float x2, float y2, float cx, float cy, float r, float &u1, float &u2);
bool MakeSegSpaceMatrix(const UMath::Vector3 &startPt, const UMath::Vector3 &endPt, UMath::Matrix4 &mat);
float GetPlaneY(const UMath::Vector3 &normal, const UMath::Vector3 &pointOnPlane, const UMath::Vector3 &testPoint);
void NearestPointLine2D(const UMath::Vector4 &pt, const UMath::Vector4 *line, UMath::Vector4 &nearPt);
void NearestPointLine2D3(const UMath::Vector3 &pt, const UMath::Vector3 &p0, const UMath::Vector3 &p1, UMath::Vector3 &nearPt);
bool IntersectSegPlane(const UMath::Vector3 &P1, const UMath::Vector3 &P2, const UMath::Vector3 &PtOnPlane, const UMath::Vector3 &Normal,
                       UMath::Vector3 &intersectionPt, float &t);
bool SegmentIntersect(const UMath::Vector4 *line1, const UMath::Vector4 *line2, UMath::Vector4 *intersectPt);

}; // namespace WWorldMath

#endif
