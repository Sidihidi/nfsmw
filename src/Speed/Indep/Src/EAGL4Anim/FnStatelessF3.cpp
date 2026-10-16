#include "FnStatelessF3.h"
#include "AnimTypeId.h"
#include "Speed/Indep/Src/EAGL4Anim/AnimUtil.h"
#include "StatelessF3.h"

namespace EAGL4Anim {

FnStatelessF3::FnStatelessF3() {
    mType = AnimTypeId::ANIM_STATELESSF3;
}

FnStatelessF3::~FnStatelessF3() {
}

void FnStatelessF3::SetAnimMemoryMap(AnimMemoryMap *anim) {
    mpAnim = anim;
}

bool FnStatelessF3::GetLength(float &timeLength) const {
    StatelessF3 *statelessF3 = reinterpret_cast<StatelessF3 *>(mpAnim);

    timeLength = static_cast<float>(statelessF3->GetNumFrames());

    return true;
}

inline const AttributeBlock *FnStatelessF3::GetAttributes() const {
    StatelessF3 *statelessF3 = reinterpret_cast<StatelessF3 *>(mpAnim);

    return statelessF3->mAttributeBlock;
}

void FnStatelessF3::Eval(float, float currTime, float *sqt) {
    EvalSQT(currTime, sqt, nullptr);
}

bool FnStatelessF3::EvalSQT(float currTime, float *sqt, const BoneMask *boneMask) {

    if (mUseFPS) {
        currTime = currTime * mFPS;
    }

    StatelessF3 *statelessF3 = reinterpret_cast<StatelessF3 *>(mpAnim);
    int floorTime = FloatToInt(currTime);
    int floorKey;
    float scale = 0.0f;
    bool slerpReqd;

    if (!statelessF3->mTimes) {

        if (floorTime < 0) {

            floorKey = 0;
        } else if (floorTime >= statelessF3->mNumKeys) {

            floorKey = statelessF3->mNumKeys - 1;
        } else {

            floorKey = floorTime;
        }

        slerpReqd = currTime != floorTime;
        scale = currTime - floorTime;
        if (floorKey >= statelessF3->mNumKeys - 1) {
            slerpReqd = false;
        }
    } else {

        if (floorTime < statelessF3->mTimes[0]) {

            floorKey = 0;
        } else {

            int timeIndex = 0;

            if (mPrevKey) {

                timeIndex = mPrevKey - 1;
            } else {

                timeIndex = 0;
            }

            if (statelessF3->mTimes[timeIndex] <= floorTime) {

                while (timeIndex < statelessF3->mNumKeys - 2 && statelessF3->mTimes[timeIndex + 1] <= floorTime) {

                    timeIndex++;
                }
            } else {

                while (timeIndex > 0 && statelessF3->mTimes[timeIndex] > floorTime) {

                    timeIndex--;
                }
            }

            floorKey = timeIndex + 1;
        }

        if (floorKey == 0) {

            slerpReqd = currTime != 0.0f;

            if (slerpReqd) {

                float ceilKeyTime = statelessF3->mTimes[floorKey];
                scale = currTime / ceilKeyTime;
            }
        } else {

            float floorKeyTime = statelessF3->mTimes[floorKey - 1];
            slerpReqd = currTime != floorKeyTime;

            if (slerpReqd) {

                float ceilKeyTime = statelessF3->mTimes[floorKey];
                scale = (currTime - floorKeyTime) / (ceilKeyTime - floorKeyTime);
            }
        }

        if (floorKey >= statelessF3->mNumKeys - 1) {
            slerpReqd = false;
        }
    }

    mPrevKey = floorKey;

    if (boneMask == nullptr) {

        short *dataBuf = statelessF3->GetData();
        StatelessF3::DofInfo *dofInfos = statelessF3->GetDofInfo();
        unsigned short *dofIdxs = statelessF3->mDofIdxs;
        short *frameData = statelessF3->GetFrameData(dataBuf, floorKey);
        int nBones = statelessF3->mNumBones;
#ifndef EA_PLATFORM_PLAYSTATION2
        int ibone;
        int index;
#endif

        if (slerpReqd) {

            int nextKey = floorKey + 1;
            short *nextFrameData = statelessF3->GetFrameData(dataBuf, nextKey);

            // asm de EA en PS2 (VU0 autorizado en la ronda 17; docs/asm-de-ea.md): en las dos PS2 los bucles de huesos
            // son bloques de asm con addi (el compilador da addiu), j a etiqueta local con nop propio, $8-$12 fijos en
            // todos los bloques y lqc2/qmtc2.ni/vmulx/ctc2/vmfir/vitof0 sin patron en ee-gcc. Los registros de los
            // operandos los reparte el compilador.
#ifdef EA_PLATFORM_PLAYSTATION2
            asm __volatile__(".set noreorder\n"
                             "ori $9, $0, 4\n"
                             "lui $11, 0x3f80\n"
                             "mtc1 $11, $f4\n"
                             "1:\n"
                             "blez %0, 2f\n"
                             "nop\n"
                             "lh $10, 0(%1)\n"
                             "mult $10, $10, $9\n"
                             "addu $10, %2, $10\n"
                             "lh $8, 0(%3)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.x vf1, vi1\n"
                             "lh $8, 2(%3)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.y vf1, vi1\n"
                             "lh $8, 4(%3)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.z vf1, vi1\n"
                             "lh $8, 0(%4)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.x vf2, vi1\n"
                             "lh $8, 2(%4)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.y vf2, vi1\n"
                             "lh $8, 4(%4)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.z vf2, vi1\n"
                             "lqc2 vf4, 16(%5)\n"
                             "vitof0.xyzw vf1, vf1\n"
                             "vmul.xyz vf1, vf4, vf1\n"
                             "vitof0.xyzw vf2, vf2\n"
                             "vmul.xyz vf2, vf4, vf2\n"
                             "qmtc2.ni %6, vf3\n"
                             "vsub.xyz vf2, vf2, vf1\n"
                             "vmulx.xyzw vf2, vf2, vf3x\n"
                             "vadd.xyz vf2, vf2, vf1\n"
                             "sqc2 vf2, 0($10)\n"
                             "swc1 $f4, 12($10)\n"
                             "addiu %3, %3, 6\n"
                             "addiu %4, %4, 6\n"
                             "addiu %1, %1, 2\n"
                             "addiu %5, %5, 32\n"
                             "addi %0, %0, -1\n"
                             "j 1b\n"
                             "nop\n"
                             "2:\n"
                             "nop\n"
                             ".set reorder"
                             :
                             : "r"(nBones), "r"(dofIdxs), "r"(sqt), "r"(frameData), "r"(nextFrameData), "r"(dofInfos),
                               "r"(scale)
                             : "$8", "$9", "$10", "$11", "$f4");
#else
            UMath::Vector3 prev;
            UMath::Vector3 next;

            for (ibone = 0; ibone < nBones; ibone++) {

                statelessF3->UnQuantize(dofInfos[ibone], frameData, prev);
                statelessF3->UnQuantize(dofInfos[ibone], nextFrameData, next);

                frameData += 3;
                nextFrameData += 3;

                index = dofIdxs[ibone];
                sqt[index] = prev.x + scale * (next.x - prev.x);
                sqt[index + 1] = prev.y + scale * (next.y - prev.y);
                sqt[index + 2] = prev.z + scale * (next.z - prev.z);
            }
#endif
        } else {

#ifdef EA_PLATFORM_PLAYSTATION2
            asm __volatile__(".set noreorder\n"
                             "ori $9, $0, 4\n"
                             "1:\n"
                             "blez %0, 2f\n"
                             "nop\n"
                             "lh $10, 0(%1)\n"
                             "mult $10, $10, $9\n"
                             "addu $10, %2, $10\n"
                             "lh $8, 0(%3)\n"
                             "mtc1 $8, $f4\n"
                             "cvt.s.w $f4, $f4\n"
                             "lwc1 $f5, 16(%4)\n"
                             "mul.s $f5, $f5, $f4\n"
                             "lh $8, 2(%3)\n"
                             "mtc1 $8, $f4\n"
                             "cvt.s.w $f4, $f4\n"
                             "lwc1 $f6, 20(%4)\n"
                             "mul.s $f6, $f6, $f4\n"
                             "lh $8, 4(%3)\n"
                             "mtc1 $8, $f4\n"
                             "cvt.s.w $f4, $f4\n"
                             "lwc1 $f7, 24(%4)\n"
                             "mul.s $f7, $f7, $f4\n"
                             "swc1 $f5, 0($10)\n"
                             "swc1 $f6, 4($10)\n"
                             "swc1 $f7, 8($10)\n"
                             "addiu %3, %3, 6\n"
                             "addiu %1, %1, 2\n"
                             "addiu %4, %4, 32\n"
                             "addi %0, %0, -1\n"
                             "j 1b\n"
                             "nop\n"
                             "2:\n"
                             "nop\n"
                             ".set reorder"
                             :
                             : "r"(nBones), "r"(dofIdxs), "r"(sqt), "r"(frameData), "r"(dofInfos)
                             : "$8", "$9", "$10", "$f4", "$f5", "$f6", "$f7");
#else
            for (ibone = 0; ibone < nBones; ibone++) {

                index = dofIdxs[ibone];
                float *rangePtr = dofInfos[ibone].mRange;
                sqt[index] = rangePtr[0] * frameData[0];
                sqt[index + 1] = rangePtr[1] * frameData[1];
                sqt[index + 2] = rangePtr[2] * frameData[2];
                frameData += 3;
            }
#endif
        }

        if (statelessF3->mNumConstBones) {

            unsigned short *constIdxs = statelessF3->GetConstBoneIdx();
            float *constBuf = statelessF3->GetConstData(dataBuf);

#ifdef EA_PLATFORM_PLAYSTATION2
            asm __volatile__(".set noreorder\n"
                             "ori $9, $0, 4\n"
                             "1:\n"
                             "blez %0, 2f\n"
                             "nop\n"
                             "lh $10, 0(%1)\n"
                             "mult $10, $10, $9\n"
                             "addu $10, %2, $10\n"
                             "lw $8, 0(%3)\n"
                             "sw $8, 0($10)\n"
                             "lw $8, 4(%3)\n"
                             "sw $8, 4($10)\n"
                             "lw $8, 8(%3)\n"
                             "sw $8, 8($10)\n"
                             "addiu %3, %3, 12\n"
                             "addiu %1, %1, 2\n"
                             "addi %0, %0, -1\n"
                             "j 1b\n"
                             "nop\n"
                             "2:\n"
                             "nop\n"
                             ".set reorder"
                             :
                             : "r"(statelessF3->mNumConstBones), "r"(constIdxs), "r"(sqt), "r"(constBuf)
                             : "$8", "$9", "$10");
#else
            for (ibone = 0; ibone < statelessF3->mNumConstBones; ibone++) {

                index = constIdxs[ibone];
                sqt[index] = constBuf[ibone * 3];
                sqt[index + 1] = constBuf[ibone * 3 + 1];
                sqt[index + 2] = constBuf[ibone * 3 + 2];
            }
#endif
        }
    } else {

        return EvalSQTMask(currTime, sqt, boneMask, slerpReqd, floorKey, scale);
    }

    return true;
}

bool FnStatelessF3::EvalSQTMask(float currTime, float *sqt, const BoneMask *boneMask, bool slerpReqd, int floorKey,
                                float scale) {

    if (boneMask != mBoneMask) {

        mBoneMask = boneMask;
    }

    StatelessF3 *statelessF3 = reinterpret_cast<StatelessF3 *>(mpAnim);
    short *dataBuf = statelessF3->GetData();
    StatelessF3::DofInfo *dofInfos = statelessF3->GetDofInfo();
    unsigned short *dofIdxs = statelessF3->mDofIdxs;
    short *frameData = statelessF3->GetFrameData(dataBuf, floorKey);
    int nBones = statelessF3->mNumBones;
    unsigned char boneIdxs[120];
    int ibone;
    int idof;
    int index;

    idof = 0;
    if (statelessF3->mNumBones != 0) {
        do {
            boneIdxs[idof] = dofIdxs[idof] / 12;
            idof++;
        } while (idof < statelessF3->mNumBones);
    }

    if (slerpReqd) {

        int nextKey = floorKey + 1;
        short *nextFrameData = statelessF3->GetFrameData(dataBuf, nextKey);
        UMath::Vector3 prev;
        UMath::Vector3 next;

        for (ibone = 0; ibone < nBones; ibone++) {

            if (boneMask->GetBone(boneIdxs[ibone])) {

                statelessF3->UnQuantize(dofInfos[ibone], frameData, prev);
                statelessF3->UnQuantize(dofInfos[ibone], nextFrameData, next);

                frameData += 3;
                nextFrameData += 3;

                index = dofIdxs[ibone];
                sqt[index] = scale * (next.x - prev.x) + prev.x;
                sqt[index + 1] = scale * (next.y - prev.y) + prev.y;
                sqt[index + 2] = scale * (next.z - prev.z) + prev.z;
            }
        }
    } else {

        for (ibone = 0; ibone < nBones; ibone++) {

            if (boneMask->GetBone(boneIdxs[ibone])) {

                index = dofIdxs[ibone];
                float *rangePtr = dofInfos[ibone].mRange;
                sqt[index] = rangePtr[0] * frameData[0];
                sqt[index + 1] = rangePtr[1] * frameData[1];
                sqt[index + 2] = rangePtr[2] * frameData[2];
                frameData += 3;
            }
        }
    }

    if (statelessF3->mNumConstBones) {

        unsigned short *constIdxs = statelessF3->GetConstBoneIdx();
        float *constBuf = statelessF3->GetConstData(dataBuf);

        for (ibone = 0; ibone < statelessF3->mNumConstBones; ibone++) {

            if (boneMask->GetBone(boneIdxs[ibone])) {

                index = constIdxs[ibone];
                sqt[index] = constBuf[ibone * 3];
                sqt[index + 1] = constBuf[ibone * 3 + 1];
                sqt[index + 2] = constBuf[ibone * 3 + 2];
            }
        }
    }

    return true;
}

bool FnStatelessF3::EvalSQTfast(float currTime, float *sqt, const BoneMask *boneMask, bool slerpReqd, int floorKey,
                                float scale) {

    mPrevKey = floorKey;

    if (boneMask == nullptr) {

        StatelessF3 *statelessF3 = reinterpret_cast<StatelessF3 *>(mpAnim);
        short *dataBuf = statelessF3->GetData();
        StatelessF3::DofInfo *dofInfos = statelessF3->GetDofInfo();
        unsigned short *dofIdxs = statelessF3->mDofIdxs;
        short *frameData = statelessF3->GetFrameData(dataBuf, floorKey);
        int nBones = statelessF3->mNumBones;
#ifndef EA_PLATFORM_PLAYSTATION2
        int ibone;
        int index;
#endif

        if (slerpReqd) {

            int nextKey = floorKey + 1;
            short *nextFrameData = statelessF3->GetFrameData(dataBuf, nextKey);

#ifdef EA_PLATFORM_PLAYSTATION2
            asm __volatile__(".set noreorder\n"
                             "ori $9, $0, 4\n"
                             "lui $11, 0x3f80\n"
                             "mtc1 $11, $f4\n"
                             "1:\n"
                             "blez %0, 2f\n"
                             "nop\n"
                             "lh $10, 0(%1)\n"
                             "mult $10, $10, $9\n"
                             "addu $10, %2, $10\n"
                             "lh $8, 0(%3)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.x vf1, vi1\n"
                             "lh $8, 2(%3)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.y vf1, vi1\n"
                             "lh $8, 4(%3)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.z vf1, vi1\n"
                             "lh $8, 0(%4)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.x vf2, vi1\n"
                             "lh $8, 2(%4)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.y vf2, vi1\n"
                             "lh $8, 4(%4)\n"
                             "ctc2 $8, $vi1\n"
                             "vmfir.z vf2, vi1\n"
                             "lqc2 vf4, 16(%5)\n"
                             "vitof0.xyzw vf1, vf1\n"
                             "vmul.xyz vf1, vf4, vf1\n"
                             "vitof0.xyzw vf2, vf2\n"
                             "vmul.xyz vf2, vf4, vf2\n"
                             "qmtc2.ni %6, vf3\n"
                             "vsub.xyz vf2, vf2, vf1\n"
                             "vmulx.xyzw vf2, vf2, vf3x\n"
                             "vadd.xyz vf2, vf2, vf1\n"
                             "sqc2 vf2, 0($10)\n"
                             "swc1 $f4, 12($10)\n"
                             "addiu %3, %3, 6\n"
                             "addiu %4, %4, 6\n"
                             "addiu %1, %1, 2\n"
                             "addiu %5, %5, 32\n"
                             "addi %0, %0, -1\n"
                             "j 1b\n"
                             "nop\n"
                             "2:\n"
                             "nop\n"
                             ".set reorder"
                             :
                             : "r"(nBones), "r"(dofIdxs), "r"(sqt), "r"(frameData), "r"(nextFrameData), "r"(dofInfos),
                               "r"(scale)
                             : "$8", "$9", "$10", "$11", "$f4");
#else
            UMath::Vector3 prev;
            UMath::Vector3 next;

            for (ibone = 0; ibone < nBones; ibone++) {

                statelessF3->UnQuantize(dofInfos[ibone], frameData, prev);
                statelessF3->UnQuantize(dofInfos[ibone], nextFrameData, next);

                frameData += 3;
                nextFrameData += 3;

                index = dofIdxs[ibone];
                sqt[index] = prev.x + scale * (next.x - prev.x);
                sqt[index + 1] = prev.y + scale * (next.y - prev.y);
                sqt[index + 2] = prev.z + scale * (next.z - prev.z);
            }
#endif
        } else {

#ifdef EA_PLATFORM_PLAYSTATION2
            asm __volatile__(".set noreorder\n"
                             "ori $9, $0, 4\n"
                             "1:\n"
                             "blez %0, 2f\n"
                             "nop\n"
                             "lh $10, 0(%1)\n"
                             "mult $10, $10, $9\n"
                             "addu $10, %2, $10\n"
                             "lh $8, 0(%3)\n"
                             "mtc1 $8, $f4\n"
                             "cvt.s.w $f4, $f4\n"
                             "lwc1 $f5, 16(%4)\n"
                             "mul.s $f5, $f5, $f4\n"
                             "lh $8, 2(%3)\n"
                             "mtc1 $8, $f4\n"
                             "cvt.s.w $f4, $f4\n"
                             "lwc1 $f6, 20(%4)\n"
                             "mul.s $f6, $f6, $f4\n"
                             "lh $8, 4(%3)\n"
                             "mtc1 $8, $f4\n"
                             "cvt.s.w $f4, $f4\n"
                             "lwc1 $f7, 24(%4)\n"
                             "mul.s $f7, $f7, $f4\n"
                             "swc1 $f5, 0($10)\n"
                             "swc1 $f6, 4($10)\n"
                             "swc1 $f7, 8($10)\n"
                             "addiu %3, %3, 6\n"
                             "addiu %1, %1, 2\n"
                             "addiu %4, %4, 32\n"
                             "addi %0, %0, -1\n"
                             "j 1b\n"
                             "nop\n"
                             "2:\n"
                             "nop\n"
                             ".set reorder"
                             :
                             : "r"(nBones), "r"(dofIdxs), "r"(sqt), "r"(frameData), "r"(dofInfos)
                             : "$8", "$9", "$10", "$f4", "$f5", "$f6", "$f7");
#else
            for (ibone = 0; ibone < nBones; ibone++) {

                index = dofIdxs[ibone];
                const StatelessF3::DofInfo &dofInfo = dofInfos[ibone];
                sqt[index] = dofInfo.mRange[0] * frameData[0];
                sqt[index + 1] = dofInfo.mRange[1] * frameData[1];
                sqt[index + 2] = dofInfo.mRange[2] * frameData[2];
                frameData += 3;
            }
#endif
        }

        if (statelessF3->mNumConstBones) {

            unsigned short *constIdxs = statelessF3->GetConstBoneIdx();
            float *constBuf = statelessF3->GetConstData(dataBuf);

#ifdef EA_PLATFORM_PLAYSTATION2
            asm __volatile__(".set noreorder\n"
                             "ori $9, $0, 4\n"
                             "1:\n"
                             "blez %0, 2f\n"
                             "nop\n"
                             "lh $10, 0(%1)\n"
                             "mult $10, $10, $9\n"
                             "addu $10, %2, $10\n"
                             "lw $8, 0(%3)\n"
                             "sw $8, 0($10)\n"
                             "lw $8, 4(%3)\n"
                             "sw $8, 4($10)\n"
                             "lw $8, 8(%3)\n"
                             "sw $8, 8($10)\n"
                             "addiu %3, %3, 12\n"
                             "addiu %1, %1, 2\n"
                             "addi %0, %0, -1\n"
                             "j 1b\n"
                             "nop\n"
                             "2:\n"
                             "nop\n"
                             ".set reorder"
                             :
                             : "r"(statelessF3->mNumConstBones), "r"(constIdxs), "r"(sqt), "r"(constBuf)
                             : "$8", "$9", "$10");
#else
            for (ibone = 0; ibone < statelessF3->mNumConstBones; ibone++) {

                index = constIdxs[ibone];
                sqt[index] = constBuf[ibone * 3];
                sqt[index + 1] = constBuf[ibone * 3 + 1];
                sqt[index + 2] = constBuf[ibone * 3 + 2];
            }
#endif
        }
    } else {

        return EvalSQTMask(currTime, sqt, boneMask, slerpReqd, floorKey, scale);
    }

    return true;
}

void FnStatelessF3::UseFPS(bool u) {
    mUseFPS = u;
    if (mFPS != 0 || !mUseFPS) {
        return;
    }
    GetAttribute(AttributeId(AttributeId::ID_FPS), mFPS);
}

}; // namespace EAGL4Anim
