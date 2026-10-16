#include "../UTil.h"
#include "Speed/Indep/Src/Physics/Dynamics/Inertia.h"

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

UMath::Vector3 Util_GenerateCarTensor(const float mass, const float width, const float height, const float length,
                                      const UMath::Vector3 &tensorScale) {
    Dynamics::Inertia::Tensor t = Dynamics::Inertia::Box(mass, 2 * width, 2 * height, 2 * length);
    UMath::Scale(t, tensorScale, t);
    return t;
}
