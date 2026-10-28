#include "../UTil.h"
#include "Speed/Indep/Src/Physics/Dynamics/Inertia.h"

#ifdef EA_PLATFORM_XENON
// Solo el EGM de X360 conserva Util_Step y Util_Ramp (0x8249F0A0-0x8249F118, antes de Util_GenerateMatrix); el PDB de
// ProStreet las pone al principio de util.cpp.
int Util_Step(const int source, const int dest, const int step) {
    int result;
    if (dest < source) {
        result = source - step;
        if (result < dest) {
            result = dest;
        }
    } else {
        result = source + step;
        if (result > dest) {
            result = dest;
        }
    }
    return result;
}

float Util_Step(const float source, const float dest, const float step) {
    float result;
    if (dest < source) {
        result = source - step;
        if (result < dest) {
            result = dest;
        }
    } else {
        result = source + step;
        if (result > dest) {
            result = dest;
        }
    }
    return result;
}

float Util_Ramp(float source, float dest, float ramp) {
    return source * ramp + (1.0f - ramp) * dest;
}
#endif

UMath::Matrix4 Util_GenerateMatrix(const UMath::Vector3 &vec, const UMath::Vector3 *InUp) {
    UMath::Matrix4 result;
    UMath::Vector4 up = {0.0f, 1.0f, 0.0f, 0.0f};

    if (InUp) {
        up.x = InUp->x;
        up.y = InUp->y;
        up.z = InUp->z;
    }
    result[2].x = vec.x;
    result[2].y = vec.y;
    result[2].z = vec.z;
    result[2].w = 0.0f;

    UMath::Unitxyz(result[2], result[2]);
    UMath::UnitCross(UMath::Vector4To3(up), UMath::Vector4To3(result[2]), UMath::Vector4To3(result[0]));
    UMath::Crossxyz(result[2], result[0], result[1]);
    result[3] = UMath::Vector4::kIdentity;

    result[0].w = 0.0f;
    result[2].w = 0.0f;
    result[1].w = 0.0f;

    return result;
}

#ifdef EA_PLATFORM_XENON
// X360 (EGM 0x8249F270): Util_GenerateMatrix sin el `up` opcional.
UMath::Matrix4 Util_GenerateMatrixUnit(const UMath::Vector3 &unitVector) {
    UMath::Matrix4 result;
    UMath::Vector4 up = {0.0f, 1.0f, 0.0f, 0.0f};

    result[2].x = unitVector.x;
    result[2].y = unitVector.y;
    result[2].z = unitVector.z;
    result[2].w = 0.0f;

    UMath::Unitxyz(result[2], result[2]);
    UMath::UnitCross(UMath::Vector4To3(up), UMath::Vector4To3(result[2]), UMath::Vector4To3(result[0]));
    UMath::Crossxyz(result[2], result[0], result[1]);
    result[3] = UMath::Vector4::kIdentity;

    result[0].w = 0.0f;
    result[2].w = 0.0f;
    result[1].w = 0.0f;

    return result;
}
#endif

UMath::Vector3 Util_GenerateCarTensor(const float mass, const float width, const float height, const float length,
                                      const UMath::Vector3 &tensorScale) {
    Dynamics::Inertia::Tensor t = Dynamics::Inertia::Box(mass, 2 * width, 2 * height, 2 * length);
    UMath::Scale(t, tensorScale, t);
    return t;
}

#ifdef EA_PLATFORM_XENON
// Solo el EGM de X360 conserva estas tres (nadie las llama; las demas versiones las estripan): 0x8249F460,
// 0x8249F558 y 0x8249F6A0, tras Util_GenerateCarTensor. Nombres, estaticos y locales del PDB de ProStreet.
#include <math.h>
#include <stdlib.h>
#include "Speed/Indep/Libs/Support/Utility/UEALibs.hpp"

float Util_GaussRandom() {
    static bool valueIsSaved = false;
    static float value;
    if (valueIsSaved) {
        valueIsSaved = false;
        return value;
    }
    float v1;
    float v2;
    float rsq;
    do {
        v1 = static_cast<float>(rand()) * (2.0f / RAND_MAX) - 1.0f;
        v2 = static_cast<float>(rand()) * (2.0f / RAND_MAX) - 1.0f;
        rsq = v1 * v1 + v2 * v2;
    } while (rsq >= 1.0f);
    float fac = UMath::Sqrt(static_cast<float>(log(rsq)) / rsq * -2.0f);
    value = fac * v2;
    valueIsSaved = true;
    return fac * v1;
}

UMath::Vector3 Util_PerturbVector(float yRot, float zRot, const UMath::Vector3 &vec) {
    UMath::Matrix4 mat;
    UMath::Vector3 result;
    UMath::SetYRot(mat, yRot);
    UMath::Rotate(vec, mat, result);
    UMath::SetZRot(mat, zRot);
    UMath::Rotate(result, mat, result);
    return result;
}

UMath::Vector3 Util_PerturbVector(const UMath::Vector3 &vec, float arcPct) {
    float yRot = Util_GaussRandom();
    float zRot = Util_GaussRandom();
    UMath::Matrix4 mat;
    UMath::Vector3 result;
    UMath::SetYRot(mat, yRot * arcPct);
    UMath::Rotate(vec, mat, result);
    UMath::SetZRot(mat, zRot * arcPct);
    UMath::Rotate(result, mat, result);
    return result;
}

void Util_EulerToQuat(const UMath::Vector3 &euler, UMath::Vector4 &quat) {
    float x = euler.x * 0.5f;
    float y = euler.y * 0.5f;
    float z = euler.z * 0.5f;
    float sr;
    float cr;
    float sp;
    float cp;
    float sy;
    float cy;

    rsincos(&x, &sr, &cr);
    rsincos(&y, &sp, &cp);
    rsincos(&z, &sy, &cy);

    quat.x = cy * cp * sr - sy * sp * cr;
    quat.y = cy * sp * cr + sy * cp * sr;
    quat.z = sy * cp * cr - cy * sp * sr;
    quat.w = sy * sp * sr + cy * cp * cr;
}

void Util_VecToPoleDepthAndLong(const UMath::Vector3 &vec, float &poleDepth, float &longitude) {
    UMath::Vector3 vecNorm = vec;

    poleDepth = 1.0f - (vecNorm.y + 1.0f) * 0.5f;
    longitude = 1.0f - UMath::Atan2a(vecNorm.x, vecNorm.z);
    while (longitude < 0.0f) {
        longitude += 1.0f;
    }
    while (longitude > 1.0f) {
        longitude -= 1.0f;
    }
}

float Util_InvSin(float value);

void Util_PoleDepthAndLongToVec(float poleDepth, float longitude, UMath::Vector3 &vec) {
    UMath::Vector3 rotEuler;
    UMath::Vector3 forward;
    UMath::Vector4 rotQuat;
    UMath::Matrix4 rotMat;

    forward.x = 0.0f;
    forward.y = 0.0f;
    forward.z = 1.0f;
    rotEuler.x = -Util_InvSin(1.0f - poleDepth * 2.0f);
    rotEuler.y = -longitude;
    rotEuler.z = 0.0f;
    Util_EulerToQuat(rotEuler, rotQuat);
    UMath::QuaternionToMatrix4(rotQuat, rotMat);
    UMath::RotateTranslate(forward, rotMat, vec);
}

char Util_CharMapLookup(float x, float y, char *pMap, int width, int height) {
    return pMap[static_cast<int>((height - 1) * y) * width + static_cast<int>((width - 1) * x)];
}

// Las mismas que NearPtLinePerSeg y NearPtLine de WCollisionMgr.cpp (BradsNearPtLine tiene el mismo codigo); la de
// cada segmento comprueba ademas la longitud cero.
void BradsNearPtLinePerSeg(const UMath::Vector3 &p0, const UMath::Vector3 &p1, float &invDen, UMath::Vector3 &diffVec) {
    UMath::Sub(p1, p0, diffVec);
    float ud = diffVec.x * diffVec.x + diffVec.y * diffVec.y + diffVec.z * diffVec.z;
    if (ud == 0.0f) {
        invDen = 0.0f;
    } else {
        invDen = 1.0f / ud;
    }
}

void BradsNearPtLine(const UMath::Vector3 &pt, const UMath::Vector3 &p0, float den, const UMath::Vector3 &diffVec,
                     UMath::Vector3 &nearPt) {
    float u = (pt.x - p0.x) * diffVec.x + (pt.y - p0.y) * diffVec.y + (pt.z - p0.z) * diffVec.z;
    u *= den;
    u = UMath::Max(UMath::Min(u, 1.0f), 0.0f);
    nearPt.x = u * diffVec.x + p0.x;
    nearPt.y = u * diffVec.y + p0.y;
    nearPt.z = u * diffVec.z + p0.z;
}

// Del PDB de ProStreet (util.h no esta en el arbol).
enum eIntersection {
    INTERSECT_NONE = 0,
    INTERSECT_INSIDE = 1,
    INTERSECT_HULL = 2,
};

eIntersection Util_CollideRayWithSphere(const UMath::Vector3 &rayStart, const UMath::Vector3 &rayVec,
                                        const UMath::Vector3 &targetCenter, float targetRadius,
                                        UMath::Vector3 &collidePos) {
    float radiusSq = targetRadius * targetRadius;
    UMath::Vector3 diffVec;
    UMath::Sub(targetCenter, rayStart, diffVec);
    if (UMath::LengthSquare(diffVec) < radiusSq) {
        collidePos = rayStart;
        return INTERSECT_INSIDE;
    }
    UMath::Vector3 rayVecUnit;
    UMath::Unit(rayVec, rayVecUnit);
    UMath::Vector3 segEnd;
    UMath::ScaleAdd(rayVecUnit, 1000.0f, rayStart, segEnd);
    float invDen;
    BradsNearPtLinePerSeg(rayStart, segEnd, invDen, diffVec);
    UMath::Vector3 nearPt;
    BradsNearPtLine(targetCenter, rayStart, invDen, diffVec, nearPt);
    if (UMath::DistanceSquare(nearPt, targetCenter) <= radiusSq) {
        collidePos = nearPt;
        return INTERSECT_HULL;
    }
    return INTERSECT_NONE;
}

eIntersection Util_CollideRayWithStretchedSphere(const UMath::Vector3 &rayStart, const UMath::Vector3 &rayVec,
                                                 const UMath::Vector3 &targetCenter, float targetRadius,
                                                 float targetStretchY, UMath::Vector3 &collidePos) {
    float invStretchY = 1.0f / targetStretchY;
    UMath::Vector3 targetCenterFixed = targetCenter;
    UMath::Vector3 rayStartFixed = rayStart;
    UMath::Vector3 rayVecFixed;

    UMath::Unit(rayVec, rayVecFixed);
    rayVecFixed.y *= invStretchY;
    rayStartFixed.y *= invStretchY;
    targetCenterFixed.y *= invStretchY;
    eIntersection result =
        Util_CollideRayWithSphere(rayStartFixed, rayVecFixed, targetCenterFixed, targetRadius, collidePos);
    if (result != INTERSECT_NONE) {
        collidePos.y = (collidePos.y - targetCenterFixed.y) * targetStretchY;
        collidePos.y = targetCenter.y + collidePos.y;
    }
    return result;
}

static const float kInvUtilTrigTableEntries = 1.0f / 2048.0f;
static float TrigTable[512];
static bool TrigTableReady = false;

void Util_InitTrigTable() {
    if (!TrigTableReady) {
        for (int i = 0; i < 512; i++) {
            TrigTable[i] = sin(static_cast<float>(i) * kInvUtilTrigTableEntries * UMath::TWOPI);
        }
        TrigTableReady = true;
    }
}

float Util_InvSin(float value) {
    if (value == 1.0f) {
        return 0.25f;
    }
    if (value == -1.0f) {
        return -0.25f;
    }
    if (!TrigTableReady) {
        Util_InitTrigTable();
    }
    bool negative = false;
    if (value < 0.0f) {
        value *= -1.0f;
        negative = true;
    }
    int slotHigh = static_cast<int>(value * 512.0f);
    int slotLow = 0;
    while (slotHigh - slotLow > 1) {
        int slotMid = (slotHigh + slotLow) >> 1;
        if (TrigTable[slotMid] > value) {
            slotHigh = slotMid;
        } else {
            slotLow = slotMid;
        }
    }
    float result = (static_cast<float>(slotLow) +
                    (value - TrigTable[slotLow]) / (TrigTable[slotHigh] - TrigTable[slotLow])) *
                   kInvUtilTrigTableEntries;
    if (negative) {
        result *= -1.0f;
    }
    return result;
}

float Util_ApplyVariance(float value, float variance) {
    // the original keeps the random value apart (fmadds f1, f1, f0, f31 in NFS.exe 0x824A0370); the local's name is ours
    float gauss = Util_GaussRandom();
    return value + gauss * (value * variance);
}
#endif
