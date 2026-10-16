#include "FnStatelessQ.h"
#include "AnimTypeId.h"
#include "FnStatelessF3.h"
#include "Speed/Indep/Src/EAGL4Anim/AnimUtil.h"
#include "StatelessQ.h"

namespace EAGL4Anim {

FnStatelessQ::FnStatelessQ() {
    mType = AnimTypeId::ANIM_STATELESSQ;
    mPrevKey = 0;
}

FnStatelessQ::~FnStatelessQ() {
}

void FnStatelessQ::SetAnimMemoryMap(AnimMemoryMap *anim) {
    mpAnim = anim;
}

bool FnStatelessQ::GetLength(float &timeLength) const {
    StatelessQ *statelessQ = reinterpret_cast<StatelessQ *>(mpAnim);

    timeLength = static_cast<float>(statelessQ->GetNumFrames());

    return true;
}

inline const AttributeBlock *FnStatelessQ::GetAttributes() const {
    StatelessQ *statelessQ = reinterpret_cast<StatelessQ *>(mpAnim);

    return statelessQ->mAttributeBlock;
}

void FnStatelessQ::Eval(float, float currTime, float *sqt) {
    EvalSQT(currTime, sqt, nullptr);
}

bool FnStatelessQ::EvalSQT(float currTime, float *sqt, const BoneMask *boneMask) {

    if (mUseFPS) {
        currTime = currTime * mFPS;
    }

    StatelessQ *statelessQ = reinterpret_cast<StatelessQ *>(mpAnim);
    int floorTime = FloatToInt(currTime);
    int floorKey;
    float scale = 0.0f;
    bool slerpReqd;

    if (!statelessQ->mTimes) {

        if (floorTime < 0) {

            floorKey = 0;
        } else if (floorTime >= statelessQ->mNumKeys) {

            floorKey = statelessQ->mNumKeys - 1;
        } else {

            floorKey = floorTime;
        }

        slerpReqd = currTime != floorTime;
        scale = currTime - floorTime;
        if (floorKey >= statelessQ->mNumKeys - 1) {
            slerpReqd = false;
        }
    } else {

        if (floorTime < statelessQ->mTimes[0]) {

            floorKey = 0;
        } else {

            int timeIndex = 0;

            if (mPrevKey) {

                timeIndex = mPrevKey - 1;
            } else {

                timeIndex = 0;
            }

            if (statelessQ->mTimes[timeIndex] <= floorTime) {

                while (timeIndex < statelessQ->mNumKeys - 2 && statelessQ->mTimes[timeIndex + 1] <= floorTime) {

                    timeIndex++;
                }
            } else {

                while (timeIndex > 0 && statelessQ->mTimes[timeIndex] > floorTime) {

                    timeIndex--;
                }
            }

            floorKey = timeIndex + 1;
        }

        if (floorKey == 0) {

            slerpReqd = currTime != 0.0f;

            if (slerpReqd) {

                float ceilKeyTime = statelessQ->mTimes[floorKey];
                scale = currTime / ceilKeyTime;
            }
        } else {

            float floorKeyTime = statelessQ->mTimes[floorKey - 1];
            slerpReqd = currTime != floorKeyTime;

            if (slerpReqd) {

                float ceilKeyTime = statelessQ->mTimes[floorKey];
                scale = (currTime - floorKeyTime) / (ceilKeyTime - floorKeyTime);
            }
        }

        if (floorKey >= statelessQ->mNumKeys - 1) {
            slerpReqd = false;
        }
    }

    mPrevKey = floorKey;

    if (boneMask == nullptr) {

        if (mBoneMask) {

            mBoneMask = boneMask;
        }

        float *q = sqt + 4;
        unsigned short *dataBuf = statelessQ->GetData();
        unsigned short *frameData = statelessQ->GetFrameData(dataBuf, floorKey);
        unsigned char *boneIdxs = statelessQ->mBoneIdxs;
        int nBones = statelessQ->mNumBones;
#ifndef EA_PLATFORM_PLAYSTATION2
        int ibone;
        int index;
#endif
        int numConsts;

        if (slerpReqd && floorKey < statelessQ->mNumKeys - 1) {

            int nextKey = floorKey + 1;
            unsigned short *nextFrameData = statelessQ->GetFrameData(dataBuf, nextKey);
            UMath::Vector4 prevQ;
            UMath::Vector4 nextQ;

            // asm de EA en PS2 (VU0 autorizado en la ronda 17; docs/asm-de-ea.md): en las dos PS2 los bucles de huesos
            // son bloques de asm con addi (el compilador da addiu), j a etiqueta local con nop propio, $8-$12 fijos en
            // todos los bloques y lqc2/qmtc2.ni/vmulx sin patron en ee-gcc. Los registros de los operandos los reparte
            // el compilador.
#ifdef EA_PLATFORM_PLAYSTATION2
            asm __volatile__(".set noreorder\n"
                             "li $10, 0xbfff\n"
                             "sll $10, $10, 16\n"
                             "ori $10, $10, 0x8000\n"
                             "ori $11, $0, 0x30\n"
                             "1:\n"
                             "blez %0, 2f\n"
                             "nop\n"
                             "lbu $12, 0(%1)\n"
                             "mult $12, $12, $11\n"
                             "addu $12, %2, $12\n"
                             "lw $8, 0(%3)\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 4(%5)\n"
                             "sll $8, $8, 16\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 0(%5)\n"
                             "lw $8, 4(%3)\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 12(%5)\n"
                             "sll $8, $8, 16\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 8(%5)\n"
                             "lw $8, 0(%4)\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 4(%6)\n"
                             "sll $8, $8, 16\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 0(%6)\n"
                             "lw $8, 4(%4)\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 12(%6)\n"
                             "sll $8, $8, 16\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 8(%6)\n"
                             "lqc2 vf1, 0(%5)\n"
                             "lqc2 vf2, 0(%6)\n"
                             "vsub.xyzw vf2, vf2, vf1\n"
                             "qmtc2.ni %7, vf3\n"
                             "vmulx.xyzw vf2, vf2, vf3x\n"
                             "vadd.xyzw vf4, vf2, vf1\n"
                             "sqc2 vf4, 0($12)\n"
                             "addi %0, %0, -1\n"
                             "addiu %3, %3, 8\n"
                             "addiu %4, %4, 8\n"
                             "addiu %1, %1, 1\n"
                             "j 1b\n"
                             "nop\n"
                             "2:\n"
                             "nop\n"
                             ".set reorder"
                             :
                             : "r"(nBones), "r"(boneIdxs), "r"(q), "r"(frameData), "r"(nextFrameData), "r"(&prevQ),
                               "r"(&nextQ), "r"(scale)
                             : "$8", "$9", "$10", "$11", "$12");
#else
            for (ibone = 0; ibone < nBones; ibone++) {

                *reinterpret_cast<unsigned int *>(&prevQ.x) = StatelessQ::Uncompress2Bits(*frameData++);
                *reinterpret_cast<unsigned int *>(&prevQ.y) = StatelessQ::Uncompress2Bits(*frameData++);
                *reinterpret_cast<unsigned int *>(&prevQ.z) = StatelessQ::Uncompress2Bits(*frameData++);
                *reinterpret_cast<unsigned int *>(&prevQ.w) = StatelessQ::Uncompress2Bits(*frameData++);
                *reinterpret_cast<unsigned int *>(&nextQ.x) = StatelessQ::Uncompress2Bits(*nextFrameData++);
                *reinterpret_cast<unsigned int *>(&nextQ.y) = StatelessQ::Uncompress2Bits(*nextFrameData++);
                *reinterpret_cast<unsigned int *>(&nextQ.z) = StatelessQ::Uncompress2Bits(*nextFrameData++);
                *reinterpret_cast<unsigned int *>(&nextQ.w) = StatelessQ::Uncompress2Bits(*nextFrameData++);

                index = boneIdxs[ibone] * 12;
                q[index] = prevQ.x + scale * (nextQ.x - prevQ.x);
                q[index + 1] = prevQ.y + scale * (nextQ.y - prevQ.y);
                q[index + 2] = prevQ.z + scale * (nextQ.z - prevQ.z);
                q[index + 3] = prevQ.w + scale * (nextQ.w - prevQ.w);
            }
#endif
        } else {

#ifdef EA_PLATFORM_PLAYSTATION2
            asm __volatile__(".set noreorder\n"
                             "li $10, 0xbfff\n"
                             "sll $10, $10, 16\n"
                             "ori $10, $10, 0x8000\n"
                             "ori $11, $0, 0x30\n"
                             "1:\n"
                             "blez %0, 2f\n"
                             "nop\n"
                             "lbu $12, 0(%1)\n"
                             "mult $12, $12, $11\n"
                             "addu $12, %2, $12\n"
                             "lw $8, 0(%3)\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 4($12)\n"
                             "sll $8, $8, 16\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 0($12)\n"
                             "lw $8, 4(%3)\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 12($12)\n"
                             "sll $8, $8, 16\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 8($12)\n"
                             "addi %0, %0, -1\n"
                             "addiu %3, %3, 8\n"
                             "addiu %1, %1, 1\n"
                             "j 1b\n"
                             "nop\n"
                             "2:\n"
                             "nop\n"
                             ".set reorder"
                             :
                             : "r"(nBones), "r"(boneIdxs), "r"(q), "r"(frameData)
                             : "$8", "$9", "$10", "$11", "$12");
#else
            for (ibone = 0; ibone < nBones; ibone++) {

                index = boneIdxs[ibone] * 12;
                *reinterpret_cast<unsigned int *>(&q[index++]) = StatelessQ::Uncompress2Bits(*frameData++);
                *reinterpret_cast<unsigned int *>(&q[index++]) = StatelessQ::Uncompress2Bits(*frameData++);
                *reinterpret_cast<unsigned int *>(&q[index++]) = StatelessQ::Uncompress2Bits(*frameData++);
                *reinterpret_cast<unsigned int *>(&q[index++]) = StatelessQ::Uncompress2Bits(*frameData++);
            }
#endif
        }

        numConsts = statelessQ->mNumConstBones;

        if (numConsts > 0) {

            unsigned char *constIdxs = statelessQ->GetConstBoneIdx();
            unsigned short *constBuf = statelessQ->GetConstData(dataBuf);

#ifdef EA_PLATFORM_PLAYSTATION2
            asm __volatile__(".set noreorder\n"
                             "li $10, 0xbfff\n"
                             "sll $10, $10, 16\n"
                             "ori $10, $10, 0x8000\n"
                             "ori $11, $0, 0x30\n"
                             "1:\n"
                             "lbu $12, 0(%1)\n"
                             "mult $12, $12, $11\n"
                             "addu $12, %2, $12\n"
                             "lw $8, 0(%3)\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 4($12)\n"
                             "sll $8, $8, 16\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 0($12)\n"
                             "lw $8, 4(%3)\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 12($12)\n"
                             "sll $8, $8, 16\n"
                             "sra $9, $8, 1\n"
                             "and $9, $9, $10\n"
                             "sw $9, 8($12)\n"
                             "addi %0, %0, -1\n"
                             "addiu %3, %3, 8\n"
                             "addiu %1, %1, 1\n"
                             "bgtz %0, 1b\n"
                             "nop\n"
                             "nop\n"
                             ".set reorder"
                             :
                             : "r"(numConsts), "r"(constIdxs), "r"(q), "r"(constBuf)
                             : "$8", "$9", "$10", "$11", "$12");
#else
            for (ibone = 0; ibone < numConsts; ibone++) {

                index = constIdxs[ibone] * 12;
                *reinterpret_cast<unsigned int *>(&q[index++]) = StatelessQ::Uncompress2Bits(*constBuf++);
                *reinterpret_cast<unsigned int *>(&q[index++]) = StatelessQ::Uncompress2Bits(*constBuf++);
                *reinterpret_cast<unsigned int *>(&q[index++]) = StatelessQ::Uncompress2Bits(*constBuf++);
                *reinterpret_cast<unsigned int *>(&q[index++]) = StatelessQ::Uncompress2Bits(*constBuf++);
            }
#endif
        }
    } else {

        EvalSQTMask(currTime, sqt, boneMask, slerpReqd, floorKey, scale);
    }

    if (statelessQ->mF3Ptr) {

        reinterpret_cast<FnStatelessF3 *>(statelessQ->mF3Ptr)
            ->EvalSQTfast(currTime, sqt, boneMask, slerpReqd, floorKey, scale);
    }

    return true;
}

bool FnStatelessQ::EvalSQTMask(float, float *sqt, const BoneMask *boneMask, bool slerpReqd, int floorKey, float scale) {

    if (boneMask != mBoneMask) {

        mBoneMask = boneMask;
    }

    StatelessQ *statelessQ = reinterpret_cast<StatelessQ *>(mpAnim);
    float *q = sqt + 4;
    unsigned short *dataBuf = statelessQ->GetData();
    unsigned short *frameData = statelessQ->GetFrameData(dataBuf, floorKey);
    unsigned short *prevData;
    unsigned short *nextData;
    unsigned char *boneIdxs = statelessQ->mBoneIdxs;
    int nBones = statelessQ->mNumBones;
    int ibone;
    int index;
    int numConsts;

    if (slerpReqd && floorKey < statelessQ->mNumKeys - 1) {

        int nextKey = floorKey + 1;
        unsigned short *nextFrameData = statelessQ->GetFrameData(dataBuf, nextKey);
        UMath::Vector4 prevQ;
        UMath::Vector4 nextQ;

        for (ibone = 0; ibone < nBones; ibone++) {

            if (boneMask->GetBone(boneIdxs[ibone])) {

                prevData = &frameData[ibone * 4];
                nextData = &nextFrameData[ibone * 4];

                *reinterpret_cast<unsigned int *>(&prevQ.x) = ((*prevData & 0x8000) << 16) | ((*prevData & 0x7FFF) << 15);
                prevData++;
                *reinterpret_cast<unsigned int *>(&prevQ.y) = ((*prevData & 0x8000) << 16) | ((*prevData & 0x7FFF) << 15);
                prevData++;
                *reinterpret_cast<unsigned int *>(&prevQ.z) = ((*prevData & 0x8000) << 16) | ((*prevData & 0x7FFF) << 15);
                prevData++;
                *reinterpret_cast<unsigned int *>(&prevQ.w) = ((*prevData & 0x8000) << 16) | ((*prevData & 0x7FFF) << 15);
                prevData++;
                *reinterpret_cast<unsigned int *>(&nextQ.x) = ((*nextData & 0x8000) << 16) | ((*nextData & 0x7FFF) << 15);
                nextData++;
                *reinterpret_cast<unsigned int *>(&nextQ.y) = ((*nextData & 0x8000) << 16) | ((*nextData & 0x7FFF) << 15);
                nextData++;
                *reinterpret_cast<unsigned int *>(&nextQ.z) = ((*nextData & 0x8000) << 16) | ((*nextData & 0x7FFF) << 15);
                nextData++;
                *reinterpret_cast<unsigned int *>(&nextQ.w) = ((*nextData & 0x8000) << 16) | ((*nextData & 0x7FFF) << 15);
                nextData++;

                index = boneIdxs[ibone] * 12;
                q[index] = prevQ.x + scale * (nextQ.x - prevQ.x);
                q[index + 1] = prevQ.y + scale * (nextQ.y - prevQ.y);
                q[index + 2] = prevQ.z + scale * (nextQ.z - prevQ.z);
                q[index + 3] = prevQ.w + scale * (nextQ.w - prevQ.w);
            }
        }
    } else {

        for (ibone = 0; ibone < nBones; ibone++) {

            if (boneMask->GetBone(boneIdxs[ibone])) {

                index = boneIdxs[ibone] * 12;
                prevData = &frameData[ibone * 4];
                *reinterpret_cast<unsigned int *>(&q[index++]) = ((*prevData & 0x8000) << 16) | ((*prevData & 0x7FFF) << 15);
                prevData++;
                *reinterpret_cast<unsigned int *>(&q[index++]) = ((*prevData & 0x8000) << 16) | ((*prevData & 0x7FFF) << 15);
                prevData++;
                *reinterpret_cast<unsigned int *>(&q[index++]) = ((*prevData & 0x8000) << 16) | ((*prevData & 0x7FFF) << 15);
                prevData++;
                *reinterpret_cast<unsigned int *>(&q[index++]) = ((*prevData & 0x8000) << 16) | ((*prevData & 0x7FFF) << 15);
                prevData++;
            }
        }
    }

    numConsts = statelessQ->mNumConstBones;

    if (numConsts > 0) {

        unsigned char *constIdxs = statelessQ->GetConstBoneIdx();
        unsigned short *constBuf = statelessQ->GetConstData(dataBuf);

        for (ibone = 0; ibone < numConsts; ibone++) {

            if (boneMask->GetBone(constIdxs[ibone])) {

                index = constIdxs[ibone] * 12;
                *reinterpret_cast<unsigned int *>(&q[index++]) = ((*constBuf & 0x8000) << 16) | ((*constBuf & 0x7FFF) << 15);
                constBuf++;
                *reinterpret_cast<unsigned int *>(&q[index++]) = ((*constBuf & 0x8000) << 16) | ((*constBuf & 0x7FFF) << 15);
                constBuf++;
                *reinterpret_cast<unsigned int *>(&q[index++]) = ((*constBuf & 0x8000) << 16) | ((*constBuf & 0x7FFF) << 15);
                constBuf++;
                *reinterpret_cast<unsigned int *>(&q[index++]) = ((*constBuf & 0x8000) << 16) | ((*constBuf & 0x7FFF) << 15);
                constBuf++;
            }
        }
    }

    return true;
}

void FnStatelessQ::UseFPS(bool u) {
    mUseFPS = u;
    if (mFPS != 0 || !mUseFPS) {
        return;
    }
    GetAttribute(AttributeId(AttributeId::ID_FPS), mFPS);
}

}; // namespace EAGL4Anim
