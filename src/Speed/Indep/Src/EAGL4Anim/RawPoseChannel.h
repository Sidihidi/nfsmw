#ifndef EAGL4ANIM_RAWPOSECHANNEL_H
#define EAGL4ANIM_RAWPOSECHANNEL_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "AnimMemoryMap.h"
#include "AnimUtil.h"
#include "BoneMask.h"
#include "eagl4supportdef.h"

namespace EAGL4Anim {

typedef void (*NonInterpFn)(float *&, float *);
typedef void (*InterpFn)(float, float *&, float *&, float *);

float qt0[7];

// total size: 0x10
class RawPoseChannel : public AnimMemoryMap {
  public:
    enum ChannelType {
        QUAT = 0,
        EUL = 1,
        TRAN = 2,
    };

    int GetSigSize() const {
        return mSigSize;
    }

    void SetSigSize(int s) {
        mSigSize = s;
    }

    int GetFrameSize() const {
        return mFrameSize;
    }

    void SetFrameSize(int s) {
        mFrameSize = s;
    }

    int GetNumFrames() const {
        return mNumFrames;
    }

    void SetNumFrames(int n) {
        mNumFrames = n;
    }

    int *GetNonInterpSig() {
        return reinterpret_cast<int *>(this + 1);
    }

    const int *GetNonInterpSig() const {
        return reinterpret_cast<const int *>(this + 1);
    }

    int *GetInterpSig() {
        return GetNonInterpSig() + mSigSize;
    }

    const int *GetInterpSig() const {
        return GetNonInterpSig() + mSigSize;
    }

    float *GetAnimData() {
        return reinterpret_cast<float *>(GetInterpSig() + mSigSize);
    }

    const float *GetAnimData() const {
        return reinterpret_cast<const float *>(GetInterpSig() + mSigSize);
    }

    float *GetFrame(int i) {
        return &GetAnimData()[i * mFrameSize];
    }

    const float *GetFrame(int i) const {
        return &GetAnimData()[i * mFrameSize];
    }

    int GetSize() const {}

    static int ComputeSize(int sigSize, int poseSize, int numFrames) {}

    inline int GetNumBones() const;

    inline void EvalInterpFrame(float t, int frame0, int frame1, float *outputPose, const BoneMask *boneMask);

    static void InitAnimMemoryMap(AnimMemoryMap *anim);

    void UnInit();

    void Eval(float frameTime, float *outputPose, bool interp, const BoneMask *boneMask);

    void EvalFrame(int frame, float *outputPose, const BoneMask *boneMask);

  private:
    int mSigSize;   // offset 0x4, size 0x4
    int mFrameSize; // offset 0x8, size 0x4
    int mNumFrames; // offset 0xC, size 0x4
};

inline void EulF3(float *&data, float *output) {
    float eulData[3];

    eulData[0] = DegToRad(*data++);
    eulData[1] = DegToRad(*data++);
    eulData[2] = DegToRad(*data++);

    EulToQuat(eulData, output);
}

inline void QuatF4(float *&data, float *output) {
    output[0] = *data++;
    output[1] = *data++;
    output[2] = *data++;
    output[3] = *data++;
}

inline void TranF3(float *&data, float *output) {
    output[4] = *data++;
    output[5] = *data++;
    output[6] = *data++;
}

inline void EulF3Interp(float w, float *&data0, float *&data1, float *output) {
    EulF3(data0, qt0);
    EulF3(data1, output);
    QuatBlendF4(w, qt0, output, output);
}

inline void QuatF4Interp(float w, float *&data0, float *&data1, float *output) {
    QuatF4(data0, qt0);
    QuatF4(data1, output);
#ifdef EA_PLATFORM_PLAYSTATION2
    VU0_fastqslerp(*reinterpret_cast<const RQUAT *>(qt0), *reinterpret_cast<const RQUAT *>(output), *reinterpret_cast<RQUAT *>(output), w);
#else
    FastQuatBlendF4(w, qt0, output, output);
#endif
}

inline void TranF3Interp(float w, float *&data0, float *&data1, float *output) {
    TranF3(data0, qt0);
    TranF3(data1, output);
    LinearBlendF3(w, &qt0[4], &output[4], &output[4]);
}

inline int RawPoseChannel::GetNumBones() const {
    RawPoseChannel *nonConst_this = const_cast<RawPoseChannel *>(this);
    int count;
    int *s = nonConst_this->GetNonInterpSig();
    int *end = nonConst_this->GetInterpSig();
    int numBones = 0;

    while (s < end) {
        count = *s++;
        s += count;
        numBones++;
    }
    return numBones;
}

}; // namespace EAGL4Anim

#endif
