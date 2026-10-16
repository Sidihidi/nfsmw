#ifndef EAGL4ANIM_IK_H
#define EAGL4ANIM_IK_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "eagl4runtimetransform.h"

namespace EAGL4Anim {

class Skeleton;

// Declaracion del DWARF de GameCube. Init y Solve (IK.cpp) estan estripadas en las tres versiones y las dos en
// linea solo las usan ellas: se quedan sin cuerpo.
// total size: 0x28
struct IK {
    inline float Solve(float len0, float len1, const UMath::Vector4 &localGoalPos);

    inline void ComputeDir(const UMath::Vector4 &p0, const UMath::Vector4 &p1, const UMath::Vector4 &p2,
                           UMath::Vector4 &dir);

    void Init(Skeleton *skel, int IKRootBoneIdx, const UMath::Vector4 &rotAxis);

    void Init(Skeleton *skel, int IKRootBoneIdx, int IKMiddleBoneIdx, int IKEndBoneIdx, const UMath::Vector4 &rotAxis,
              float *sqtBuffer, EAGL4::Transform *globalMatricesBuffer);

    void Solve(const UMath::Vector4 &goalWorldPos, const EAGL4::Transform *skelWorld, EAGL4::Transform *skelLocal,
               bool computeDir, const UMath::Vector4 *desireDir);

    void Solve(const EAGL4::Transform &t0World, const UMath::Vector4 &goalPos, EAGL4::Transform &t0,
               EAGL4::Transform &t1, EAGL4::Transform &t2, const UMath::Vector4 *dir);

    Skeleton *mpSkel;      // offset 0x0, size 0x4
    int mIKRootBoneIdx;    // offset 0x4, size 0x4
    int mIKMiddleBoneIdx;  // offset 0x8, size 0x4
    int mIKEndBoneIdx;     // offset 0xC, size 0x4
    float mBoneLen[2];     // offset 0x10, size 0x8
    UMath::Vector4 mAxis;  // offset 0x18, size 0x10
};

}; // namespace EAGL4Anim

#endif
