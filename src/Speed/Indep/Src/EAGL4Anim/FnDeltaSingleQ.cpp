#include "FnDeltaSingleQ.h"
#include "Speed/Indep/Src/EAGL4Anim/AnimTypeId.h"
#include "Speed/Indep/Src/EAGL4Anim/AnimUtil.h"
#include "Speed/Indep/Src/EAGL4Anim/MemoryPoolManager.h"

#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#endif

namespace EAGL4Anim {

FnDeltaSingleQ::FnDeltaSingleQ()
    : mPrevKey(-1),         //
      mPostMultQs(nullptr), //
      mMinRanges(nullptr),  //
      mBins(nullptr),       //
      mBinSize(-1),         //
      mPrevQBlock(nullptr), //
      mPrevQs(nullptr),     //
      mPreMultQs(nullptr) {
    mType = AnimTypeId::ANIM_DELTASINGLEQ;
}

inline void FnDeltaSingleQ::InitBuffersAsRequired() {

    if (!mPrevQs) {

        DeltaSingleQ *deltaQ = reinterpret_cast<DeltaSingleQ *>(mpAnim);
        DeltaSingleQMinRange *minRanges;

        deltaQ->GetArrays(minRanges, mBins);

        mBinSize = deltaQ->GetBinSize();

        int ibone;
        float eul[3];

        // PS2: lqc2/sqc2 exigen mPrevQs (solo el: los otros dos bloques no pasan por VU0) alineado a 16. La A124 pide 15 bytes de mas
        // y alinea el puntero (`ori 0xf' al pedir; `addiu 15; sra 4; sll 4' despues).
#ifdef EA_PLATFORM_PLAYSTATION2
        mPrevQBlock = MemoryPoolManager::NewBlock(deltaQ->mNumBones * sizeof(COORD4) + 15);
        mPrevQs = reinterpret_cast<COORD4 *>(AlignSize16(reinterpret_cast<intptr_t>(mPrevQBlock)));
#else
        mPrevQBlock = MemoryPoolManager::NewBlock(deltaQ->mNumBones * sizeof(COORD4));
        mPrevQs = reinterpret_cast<COORD4 *>(mPrevQBlock);
#endif

        mMinRanges = minRanges;

        mPreMultQs = reinterpret_cast<COORD4 *>(MemoryPoolManager::NewBlock(deltaQ->mNumBones * sizeof(COORD4)));

        mPostMultQs = reinterpret_cast<COORD4 *>(MemoryPoolManager::NewBlock(deltaQ->mNumBones * sizeof(COORD4)));

        for (ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

            DeltaSingleQMinRangef minRangef;

            mMinRanges[ibone].UnQuantize(minRangef);

            if (minRangef.mIndex == 0) {

                mPreMultQs[ibone].x = 0.0f;
                mPreMultQs[ibone].y = 0.0f;
                mPreMultQs[ibone].z = 0.0f;
                mPreMultQs[ibone].w = 1.0f;

                eul[0] = 0.0f;
                eul[1] = minRangef.mConst0;
                eul[2] = minRangef.mConst1;
                EulToQuat(eul, &mPostMultQs[ibone].x);

            } else if (minRangef.mIndex == 1) {

                eul[0] = minRangef.mConst0;
                eul[1] = eul[2] = 0.0f;
                EulToQuat(eul, &mPreMultQs[ibone].x);

                eul[0] = eul[1] = 0.0f;
                eul[2] = minRangef.mConst1;
                EulToQuat(eul, &mPostMultQs[ibone].x);

            } else {

                eul[0] = minRangef.mConst0;
                eul[1] = minRangef.mConst1;
                eul[2] = 0.0f;
                EulToQuat(eul, &mPreMultQs[ibone].x);

                mPostMultQs[ibone].x = 0.0f;
                mPostMultQs[ibone].y = 0.0f;
                mPostMultQs[ibone].z = 0.0f;
                mPostMultQs[ibone].w = 1.0f;
            }
        }
    }
}

FnDeltaSingleQ::~FnDeltaSingleQ() {
    if (mPrevQBlock) {
        MemoryPoolManager::DeleteBlock(mPrevQBlock);
        MemoryPoolManager::DeleteBlock(mPreMultQs);
        MemoryPoolManager::DeleteBlock(mPostMultQs);
    }
}

void FnDeltaSingleQ::SetAnimMemoryMap(AnimMemoryMap *anim) {
    mpAnim = anim;
}

bool FnDeltaSingleQ::GetLength(float &timeLength) const {
    DeltaSingleQ *deltaQ = reinterpret_cast<DeltaSingleQ *>(mpAnim);

    timeLength = static_cast<float>(deltaQ->GetNumFrames());
    return true;
}

void FnDeltaSingleQ::Eval(float prevTime, float currTime, float *sqt) {
    EvalSQTMasked(currTime, nullptr, sqt);
}

}; // namespace EAGL4Anim

static void QuatMultXxYxZ(const COORD4 &a, const COORD4 &b, const COORD4 &c, COORD4 &result) {

    COORD4 q;

    q.x = a.x * b.w;
    q.y = a.w * b.y;
    q.z = -a.x * b.y;
    q.w = a.w * b.w;

    result.x = q.x * c.w - q.y * c.z;
    result.y = q.x * c.z + q.y * c.w;
    result.z = q.z * c.w + q.w * c.z;
    result.w = -q.z * c.z + q.w * c.w;
}

static void QuatMultXxQ(const COORD4 &a, const COORD4 &b, COORD4 &result) {

    result.x = a.x * b.w + a.w * b.x;
    result.y = a.x * b.z + a.w * b.y;
    result.z = -a.x * b.y + a.w * b.z;
    result.w = -a.x * b.x + a.w * b.w;
}

static void QuatMultQxZ(const COORD4 &a, const COORD4 &b, COORD4 &result) {

    result.x = a.x * b.w - a.y * b.z;
    result.y = a.x * b.z + a.y * b.w;
    result.z = a.z * b.w + a.w * b.z;
    result.w = -a.z * b.z + a.w * b.w;
}

namespace EAGL4Anim {

bool FnDeltaSingleQ::EvalSQT(float currTime, float *sqt, const BoneMask *boneMask) {

    if (boneMask) {

        return EvalSQTMasked(currTime, boneMask, sqt);
    }

    InitBuffersAsRequired();

    DeltaSingleQ *deltaQ = reinterpret_cast<DeltaSingleQ *>(mpAnim);

    int floorTime = FloatToInt(currTime);

    int floorKey;

    if (!deltaQ->mTimes) {

        if (floorTime < 0) {

            floorKey = 0;

        } else {

            if (floorTime >= deltaQ->mNumKeys) {

                floorKey = deltaQ->mNumKeys - 1;

            } else {

                floorKey = floorTime;
            }
        }
    } else if (floorTime < deltaQ->mTimes[0]) {

        floorKey = 0;

    } else {
        int timeIndex;

        if (mPrevKey <= 0) {

            timeIndex = 0;

        } else {

            timeIndex = mPrevKey - 1;
        }

        if (deltaQ->mTimes[timeIndex] <= floorTime) {

            while (timeIndex < deltaQ->mNumKeys - 2 && deltaQ->mTimes[timeIndex + 1] <= floorTime) {

                timeIndex++;
            }
        } else {

            while (timeIndex > 0 && deltaQ->mTimes[timeIndex] > floorTime) {

                timeIndex--;
            }
        }

        floorKey = timeIndex + 1;
    }

    const unsigned int binLenPower = deltaQ->GetBinLengthPower();
    unsigned int binLenModMask = deltaQ->GetBinLengthModMask();

    int floorBinIdx = floorKey >> binLenPower;
    int floorDeltaIdx = floorKey & binLenModMask;
    int prevBinIdx = mPrevKey >> binLenPower;
    int prevDeltaIdx;

    DeltaSingleQPhysical *floorPhys = deltaQ->GetPhysical(&mBins[floorBinIdx * mBinSize]);
    unsigned char *boneIdxs = deltaQ->mBoneIdxs;

    if (mPrevKey == -1 || floorBinIdx != prevBinIdx || floorKey < mPrevKey) {

        for (int ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

            floorPhys[ibone].UnQuantize(mMinRanges[ibone].mIndex, mPrevQs[ibone]);
        }

        prevDeltaIdx = 0;

    } else {

        prevDeltaIdx = mPrevKey & binLenModMask;
    }

    if (prevDeltaIdx < floorDeltaIdx) {

        DeltaSingleQDelta *floorDelta = &reinterpret_cast<DeltaSingleQDelta *>(&floorPhys[deltaQ->mNumBones])[prevDeltaIdx * deltaQ->mNumBones];

        for (int iframe = prevDeltaIdx; iframe < floorDeltaIdx; iframe++) {

            for (int ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

                COORD4 deltaf;
                DeltaSingleQMinRangef minRangef;

                mMinRanges[ibone].UnQuantize(minRangef);
                floorDelta->UnQuantize(minRangef, deltaf);

#ifdef EA_PLATFORM_PLAYSTATION2
                VU0_v4add(mPrevQs[ibone], deltaf, mPrevQs[ibone]);
#else
                mPrevQs[ibone].x += deltaf.x;
                mPrevQs[ibone].y += deltaf.y;
                mPrevQs[ibone].z += deltaf.z;
                mPrevQs[ibone].w += deltaf.w;
#endif

                floorDelta++;
            }
        }
    }

    mPrevKey = floorKey;

    int ceilKey = floorKey + 1;

    float scale;
    bool slerpReqd;

    if (!deltaQ->mTimes) {
        slerpReqd = currTime != floorTime;
        if (slerpReqd) {
            scale = currTime - floorTime;
        }
    } else if (floorKey == 0) {
        slerpReqd = currTime != 0.0f;
        if (slerpReqd) {
            float ceilKeyTime = deltaQ->mTimes[floorKey];
            scale = currTime / ceilKeyTime;
        }
    } else {
        float floorKeyTime = deltaQ->mTimes[floorKey - 1];
        slerpReqd = currTime != floorKeyTime;
        if (slerpReqd) {
            float ceilKeyTime = deltaQ->mTimes[floorKey];
            scale = (currTime - floorKeyTime) / (ceilKeyTime - floorKeyTime);
        }
    }

    if (slerpReqd && floorKey < deltaQ->mNumKeys - 1) {

        int ceilBinIdx = ceilKey >> binLenPower;
        DeltaSingleQPhysical *ceilPhys = deltaQ->GetPhysical(&mBins[ceilBinIdx * mBinSize]);

        if (ceilBinIdx != floorBinIdx) {

            COORD4 ceilq;
            COORD4 interpq;

            for (int ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

                ceilPhys[ibone].UnQuantize(mMinRanges[ibone].mIndex, ceilq);

                FastPolarizedQuatBlend(scale, mPrevQs[ibone], ceilq, interpq);

                switch (mMinRanges[ibone].mIndex) {

                case 0:

                    QuatMultXxQ(interpq, mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                    break;

                case 1:

                    QuatMultXxYxZ(mPreMultQs[ibone], interpq, mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                    break;

                case 2:
                default:

                    QuatMultQxZ(mPreMultQs[ibone], interpq, reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                    break;
                }
            }
        } else {

            int ceilDeltaIdx = floorDeltaIdx;

            DeltaSingleQDelta *ceilDelta = &reinterpret_cast<DeltaSingleQDelta *>(&ceilPhys[deltaQ->mNumBones])[ceilDeltaIdx * deltaQ->mNumBones];

            COORD4 ceilq;
            COORD4 interpq;

            for (int ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

                DeltaSingleQMinRangef minRangef;

                mMinRanges[ibone].UnQuantize(minRangef);
                ceilDelta->UnQuantize(minRangef, ceilq);

#ifdef EA_PLATFORM_PLAYSTATION2
                VU0_v4add(ceilq, mPrevQs[ibone], ceilq);
#else
                ceilq.x += mPrevQs[ibone].x;
                ceilq.y += mPrevQs[ibone].y;
                ceilq.z += mPrevQs[ibone].z;
                ceilq.w += mPrevQs[ibone].w;
#endif

                FastPolarizedQuatBlend(scale, mPrevQs[ibone], ceilq, interpq);

                switch (mMinRanges[ibone].mIndex) {

                case 0:

                    QuatMultXxQ(interpq, mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                    break;

                case 1:

                    QuatMultXxYxZ(mPreMultQs[ibone], interpq, mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                    break;

                case 2:
                default:

                    QuatMultQxZ(mPreMultQs[ibone], interpq, reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                    break;
                }

                ceilDelta++;
            }
        }
    } else {

        for (int ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

            switch (mMinRanges[ibone].mIndex) {

            case 0:

                QuatMultXxQ(mPrevQs[ibone], mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                break;

            case 1:

                QuatMultXxYxZ(mPreMultQs[ibone], mPrevQs[ibone], mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                break;

            case 2:
            default:

                QuatMultQxZ(mPreMultQs[ibone], mPrevQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                break;
            }
        }
    }

    return true;
}

bool FnDeltaSingleQ::EvalSQTMasked(float currTime, const BoneMask *boneMask, float *sqt) {

    InitBuffersAsRequired();

    DeltaSingleQ *deltaQ = reinterpret_cast<DeltaSingleQ *>(mpAnim);

    int floorTime = FloatToInt(currTime);

    int floorKey;

    if (!deltaQ->mTimes) {

        if (floorTime < 0) {

            floorKey = 0;

        } else {

            if (floorTime >= deltaQ->mNumKeys) {

                floorKey = deltaQ->mNumKeys - 1;

            } else {

                floorKey = floorTime;
            }
        }
    } else if (floorTime < deltaQ->mTimes[0]) {

        floorKey = 0;

    } else {
        int timeIndex;

        if (mPrevKey <= 0) {

            timeIndex = 0;

        } else {

            timeIndex = mPrevKey - 1;
        }

        if (deltaQ->mTimes[timeIndex] <= floorTime) {

            while (timeIndex < deltaQ->mNumKeys - 2 && deltaQ->mTimes[timeIndex + 1] <= floorTime) {

                timeIndex++;
            }
        } else {

            while (timeIndex > 0 && deltaQ->mTimes[timeIndex] > floorTime) {

                timeIndex--;
            }
        }

        floorKey = timeIndex + 1;
    }

    const unsigned int binLenPower = deltaQ->GetBinLengthPower();
    unsigned int binLenModMask = deltaQ->GetBinLengthModMask();

    int floorBinIdx = floorKey >> binLenPower;
    int floorDeltaIdx = floorKey & binLenModMask;
    int prevBinIdx = mPrevKey >> binLenPower;
    int prevDeltaIdx;

    DeltaSingleQPhysical *floorPhys = deltaQ->GetPhysical(&mBins[floorBinIdx * mBinSize]);
    unsigned char *boneIdxs = deltaQ->mBoneIdxs;

    if (mPrevKey == -1 || floorBinIdx != prevBinIdx || floorKey < mPrevKey) {

        for (int ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

            if (boneMask->GetBone(boneIdxs[ibone])) {

                floorPhys[ibone].UnQuantize(mMinRanges[ibone].mIndex, mPrevQs[ibone]);
            }
        }

        prevDeltaIdx = 0;

    } else {

        prevDeltaIdx = mPrevKey & binLenModMask;
    }

    if (prevDeltaIdx < floorDeltaIdx) {

        DeltaSingleQDelta *floorDelta = &reinterpret_cast<DeltaSingleQDelta *>(&floorPhys[deltaQ->mNumBones])[prevDeltaIdx * deltaQ->mNumBones];

        for (int iframe = prevDeltaIdx; iframe < floorDeltaIdx; iframe++) {

            for (int ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

                if (boneMask->GetBone(boneIdxs[ibone])) {

                    COORD4 deltaf;
                    DeltaSingleQMinRangef minRangef;

                    mMinRanges[ibone].UnQuantize(minRangef);
                    floorDelta->UnQuantize(minRangef, deltaf);

#ifdef EA_PLATFORM_PLAYSTATION2
                    VU0_v4add(mPrevQs[ibone], deltaf, mPrevQs[ibone]);
#else
                    mPrevQs[ibone].x += deltaf.x;
                    mPrevQs[ibone].y += deltaf.y;
                    mPrevQs[ibone].z += deltaf.z;
                    mPrevQs[ibone].w += deltaf.w;
#endif
                }

                floorDelta++;
            }
        }
    }

    mPrevKey = floorKey;

    int ceilKey = floorKey + 1;

    float scale;
    bool slerpReqd;

    if (!deltaQ->mTimes) {
        slerpReqd = currTime != floorTime;
        if (slerpReqd) {
            scale = currTime - floorTime;
        }
    } else if (floorKey == 0) {
        slerpReqd = currTime != 0.0f;
        if (slerpReqd) {
            float ceilKeyTime = deltaQ->mTimes[floorKey];
            scale = currTime / ceilKeyTime;
        }
    } else {
        float floorKeyTime = deltaQ->mTimes[floorKey - 1];
        slerpReqd = currTime != floorKeyTime;
        if (slerpReqd) {
            float ceilKeyTime = deltaQ->mTimes[floorKey];
            scale = (currTime - floorKeyTime) / (ceilKeyTime - floorKeyTime);
        }
    }

    if (slerpReqd && floorKey < deltaQ->mNumKeys - 1) {

        int ceilBinIdx = ceilKey >> binLenPower;
        DeltaSingleQPhysical *ceilPhys = deltaQ->GetPhysical(&mBins[ceilBinIdx * mBinSize]);

        if (ceilBinIdx != floorBinIdx) {

            COORD4 ceilq;
            COORD4 interpq;

            for (int ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

                if (boneMask->GetBone(boneIdxs[ibone])) {

                    ceilPhys[ibone].UnQuantize(mMinRanges[ibone].mIndex, ceilq);

                    FastPolarizedQuatBlend(scale, mPrevQs[ibone], ceilq, interpq);

                    switch (mMinRanges[ibone].mIndex) {

                    case 0:

                        QuatMultXxQ(interpq, mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                        break;

                    case 1:

                        QuatMultXxYxZ(mPreMultQs[ibone], interpq, mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                        break;

                    case 2:
                    default:

                        QuatMultQxZ(mPreMultQs[ibone], interpq, reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                        break;
                    }
                }
            }
        } else {

            int ceilDeltaIdx = floorDeltaIdx;

            DeltaSingleQDelta *ceilDelta = &reinterpret_cast<DeltaSingleQDelta *>(&ceilPhys[deltaQ->mNumBones])[ceilDeltaIdx * deltaQ->mNumBones];

            COORD4 ceilq;
            COORD4 interpq;

            for (int ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

                if (boneMask->GetBone(boneIdxs[ibone])) {

                    DeltaSingleQMinRangef minRangef;

                    mMinRanges[ibone].UnQuantize(minRangef);
                    ceilDelta->UnQuantize(minRangef, ceilq);

#ifdef EA_PLATFORM_PLAYSTATION2
                    VU0_v4add(ceilq, mPrevQs[ibone], ceilq);
#else
                    ceilq.x += mPrevQs[ibone].x;
                    ceilq.y += mPrevQs[ibone].y;
                    ceilq.z += mPrevQs[ibone].z;
                    ceilq.w += mPrevQs[ibone].w;
#endif

                    FastPolarizedQuatBlend(scale, mPrevQs[ibone], ceilq, interpq);

                    switch (mMinRanges[ibone].mIndex) {

                    case 0:

                        QuatMultXxQ(interpq, mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                        break;

                    case 1:

                        QuatMultXxYxZ(mPreMultQs[ibone], interpq, mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                        break;

                    case 2:
                    default:

                        QuatMultQxZ(mPreMultQs[ibone], interpq, reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                        break;
                    }
                }

                ceilDelta++;
            }
        }
    } else {

        for (int ibone = 0; ibone < deltaQ->mNumBones; ibone++) {

            if (boneMask->GetBone(boneIdxs[ibone])) {

                switch (mMinRanges[ibone].mIndex) {

                case 0:

                    QuatMultXxQ(mPrevQs[ibone], mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                    break;

                case 1:

                    QuatMultXxYxZ(mPreMultQs[ibone], mPrevQs[ibone], mPostMultQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                    break;

                case 2:
                default:

                    QuatMultQxZ(mPreMultQs[ibone], mPrevQs[ibone], reinterpret_cast<UMath::Vector4 *>(sqt)[boneIdxs[ibone] * 3 + 1]);
                    break;
                }
            }
        }
    }

    return true;
}

}; // namespace EAGL4Anim
