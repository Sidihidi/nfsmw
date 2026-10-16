#ifndef EAGL4ANIM_DELTAQ_H
#define EAGL4ANIM_DELTAQ_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "AnimMemoryMap.h"
#include "AnimUtil.h"
#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"

#ifdef EA_PLATFORM_PLAYSTATION2
// Las VU0_ en linea de UVectorMath.hpp. El .mdebug de la A124 las expande
// directamente desde este fichero, sin pasar por las envolturas de UMath.
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#endif

namespace EAGL4Anim {

// Declarados en el DWARF de GameCube (sin direccion: alli nadie los usa) y en
// la A124 (.rodata 0x544340 y 0x544350, con estos valores; la w de
// Coord4NegOneXYZ tambien vale -1). Solo las UnQuantize de PS2 los leen.
static const COORD3 Coord3NegOne = {-1.0f, -1.0f, -1.0f};
static const COORD4 Coord4NegOneXYZ = {-1.0f, -1.0f, -1.0f, -1.0f};

// total size: 0x18
struct DeltaQMinRangef {
    COORD3 mMin;   // offset 0x0, size 0xC
    COORD3 mRange; // offset 0xC, size 0xC
};

// total size: 0xC
struct DeltaQMinRange {
    void UnQuantize(DeltaQMinRangef &minRangef) {
        const float RangeScale16Bit = 2.0f / 65535.0f;

#ifdef EA_PLATFORM_PLAYSTATION2
        // lqc2 exige 16 bytes alineados y mRange (0xC) no lo esta: se calcula
        // en una temporal y se copia. La A124 copia mMin en ella antes de
        // rellenarla, en las tres expansiones de EvalSQT y EvalSQTMasked.
        COORD3 temp = minRangef.mMin;

        temp.x = mMin[0];
        temp.y = mMin[1];
        temp.z = mMin[2];
        VU0_v3scaleadd(temp, RangeScale16Bit, Coord3NegOne, temp);
        minRangef.mMin = temp;

        temp.x = mRange[0];
        temp.y = mRange[1];
        temp.z = mRange[2];
        VU0_v3scale(temp, RangeScale16Bit, temp);
        minRangef.mRange = temp;
#else
        minRangef.mMin.x = mMin[0] * RangeScale16Bit - 1.0f;
        minRangef.mMin.y = mMin[1] * RangeScale16Bit - 1.0f;
        minRangef.mMin.z = mMin[2] * RangeScale16Bit - 1.0f;

        minRangef.mRange.x = mRange[0] * RangeScale16Bit;
        minRangef.mRange.y = mRange[1] * RangeScale16Bit;
        minRangef.mRange.z = mRange[2] * RangeScale16Bit;
#endif
    }

    unsigned short mMin[3];   // offset 0x0, size 0x6
    unsigned short mRange[3]; // offset 0x6, size 0x6
};

inline void DeltaQRecoverW(int signBit, COORD4 &q) {
    float ndotn = q.x * q.x + q.y * q.y + q.z * q.z;

    if (ndotn > 1.0f) {
        float len = FastSqrt(ndotn);

        q.x /= len;
        q.y /= len;
        q.z /= len;
        q.w = 0.0f;
    } else {
        q.w = FastSqrt(1.0f - ndotn);

        if (signBit > 0) {
            q.w = -q.w;
        }
    }
}

// total size: 0x6
struct DeltaQPhysical {
    void UnQuantize(COORD4 &q) {
        const float RangeScale15Bit = 2.0f / 32767.0f;
        const float RangeScale16Bit = 2.0f / 65535.0f;

#ifdef EA_PLATFORM_PLAYSTATION2
        q.x = mX * RangeScale15Bit;
        q.y = mY * RangeScale16Bit;
        q.z = mZ * RangeScale16Bit;
        VU0_v4addxyz(q, Coord4NegOneXYZ, q);
#else
        q.x = mX * RangeScale15Bit - 1.0f;
        q.y = mY * RangeScale16Bit - 1.0f;
        q.z = mZ * RangeScale16Bit - 1.0f;
#endif

        DeltaQRecoverW(mW, q);
    }

    // Los datos llevan mW en el bit 0 en las dos plataformas. En big-endian el
    // primer campo es el bit alto; en el EE (little-endian) el bajo, y la A124
    // lo declara primero: mX sale con `srl 1' y mW con `andi 1'.
#ifdef EA_PLATFORM_PLAYSTATION2
    unsigned short mW : 1;  // offset 0x0, size 0x2
    unsigned short mX : 15; // offset 0x0, size 0x2
#else
    unsigned short mX : 15; // offset 0x0, size 0x2
    unsigned short mW : 1;  // offset 0x0, size 0x2
#endif
    unsigned short mY;      // offset 0x2, size 0x2
    unsigned short mZ;      // offset 0x4, size 0x2
};

// total size: 0x3
struct DeltaQDelta {
    static float DeQuantize7Bit(int v) {
        return v * (1.0f / 127.0f);
    }

    static float DeQuantize8Bit(int v) {
        return v * (1.0f / 255.0f);
    }

    void UnQuantize(const DeltaQMinRangef &minRangef, COORD4 &q) {
        q.x = minRangef.mRange.x * DeQuantize7Bit(mX) + minRangef.mMin.x;
        q.y = minRangef.mRange.y * DeQuantize8Bit(mY) + minRangef.mMin.y;
        q.z = minRangef.mRange.z * DeQuantize8Bit(mZ) + minRangef.mMin.z;
    }

    // Como en DeltaQPhysical: la A124 saca mX con `srl 1' y mW con `andi 1'.
#ifdef EA_PLATFORM_PLAYSTATION2
    unsigned char mW : 1; // offset 0x0, size 0x1
    unsigned char mX : 7; // offset 0x0, size 0x1
#else
    unsigned char mX : 7; // offset 0x0, size 0x1
    unsigned char mW : 1; // offset 0x0, size 0x1
#endif
    unsigned char mY;     // offset 0x1, size 0x1
    unsigned char mZ;     // offset 0x2, size 0x1
};

// total size: 0x14
struct DeltaQ : public AnimMemoryMap {
    static int ComputeSize(int numBones, int numConst, int numFrames, int binLen, bool useKeyFrames) {
        // const int binSize;
        // int s;
        // int r;
    }

    int GetNumFrames() const {
        if (!mTimes) {
            return mNumKeys;
        } else {
            return mTimes[mNumKeys - 2] + 1;
        }
    }

    unsigned int GetBinLength() const {
        return 1 << mBinLengthPower;
    }

    unsigned int GetBinLengthPower() const {
        return mBinLengthPower;
    }

    unsigned int GetBinLengthModMask() const {
        unsigned int result = 0x7FFFFFFFU >> (31 - mBinLengthPower);
        return result;
    }

    void GetArrays(DeltaQMinRange *&minRanges, unsigned char *&binStart, unsigned char *&constBoneIndices, DeltaQPhysical *&constPhysical) {
        minRanges = reinterpret_cast<DeltaQMinRange *>(&mPadding[1]);
        binStart = &reinterpret_cast<unsigned char *>(minRanges)[mNumBones * sizeof(DeltaQMinRange)];

        constBoneIndices = GetConstBoneIdx();
        constPhysical = reinterpret_cast<DeltaQPhysical *>(GetConstPhysical());
    }

    int GetBinSize() const {
        return AlignSize2(mNumBones * ((GetBinLength() - 1) * sizeof(DeltaQDelta) + sizeof(DeltaQPhysical)));
    }

    DeltaQMinRange *GetMinRange() {
        unsigned char *memBytes = reinterpret_cast<unsigned char *>(&mPadding[1]);
        return reinterpret_cast<DeltaQMinRange *>(memBytes);
    }

    unsigned char *GetBin(int binIdx) {
        const int bs = GetBinSize();
        unsigned char *memPos = &reinterpret_cast<unsigned char *>(GetMinRange())[mNumBones * sizeof(DeltaQMinRange)];
        return &memPos[binIdx * bs];
    }

    DeltaQPhysical *GetPhysical(unsigned char *binData) {
        return reinterpret_cast<DeltaQPhysical *>(binData);
    }

    DeltaQDelta *GetDelta(unsigned char *binData, int deltaIdx) {
        return &reinterpret_cast<DeltaQDelta *>(&GetPhysical(binData)[mNumBones])[deltaIdx * mNumBones];
    }

    unsigned char *GetConstBoneIdx() {
        const int binSize = AlignSize2(mNumBones * ((GetBinLength() - 1) * sizeof(DeltaQDelta) + sizeof(DeltaQPhysical)));
        unsigned char *s = &GetBin(0)[binSize * (mNumKeys / GetBinLength())];
        int r = mNumKeys % GetBinLength();

        if (r > 0) {
            s = &s[mNumBones * ((r - 1) * sizeof(DeltaQDelta) + sizeof(DeltaQPhysical))];
        }

        return s;
    }

    float *GetConstPhysical() {
        return reinterpret_cast<float *>(AlignSize2(reinterpret_cast<intptr_t>(&GetConstBoneIdx()[mNumConstBones])));
    }

    unsigned short mNumKeys;       // offset 0x4, size 0x2
    unsigned char mNumBones;       // offset 0x6, size 0x1
    unsigned char mNumConstBones;  // offset 0x7, size 0x1
    unsigned short *mTimes;        // offset 0x8, size 0x4
    unsigned char *mBoneIdxs;      // offset 0xC, size 0x4
    unsigned char mBinLengthPower; // offset 0x10, size 0x1
    unsigned char mPadding[1];     // offset 0x11, size 0x1
};

}; // namespace EAGL4Anim

#endif
