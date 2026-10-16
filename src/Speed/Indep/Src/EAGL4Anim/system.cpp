#include "eagl4runtimetransform.h"
#include "System.h"

#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"

static void MatrixMultiplyDefault(EAGL4::Transform *result, const EAGL4::Transform *m1, const EAGL4::Transform *m2) {
    VU0_MATRIX4_mult(*reinterpret_cast<const UMath::Matrix4 *>(m2), *reinterpret_cast<const UMath::Matrix4 *>(m1),
                     *reinterpret_cast<UMath::Matrix4 *>(result));
}

namespace EAGL4Anim {

MatrixMultiplier MatrixMultiply = MatrixMultiplyDefault;

}; // namespace EAGL4Anim
#else
#include "Speed/Indep/bWare/Inc/bMath.hpp"

void MtxMult(EAGL4::Transform *result, const EAGL4::Transform *A, const EAGL4::Transform *B) {
    EAGL4::Transform newMtx;

    bMulMatrix(reinterpret_cast<bMatrix4 *>(&newMtx), reinterpret_cast<const bMatrix4 *>(A), reinterpret_cast<const bMatrix4 *>(B));
    bCopy(reinterpret_cast<bMatrix4 *>(result), reinterpret_cast<const bMatrix4 *>(&newMtx));
}
#endif
