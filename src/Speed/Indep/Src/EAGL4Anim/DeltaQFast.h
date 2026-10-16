#ifndef EAGL4ANIM_DELTAQFAST_H
#define EAGL4ANIM_DELTAQFAST_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "AnimMemoryMap.h"
#include "AnimUtil.h"
#include "Speed/Indep/Libs/Support/Utility/UTypes.h"

#ifdef EA_PLATFORM_PLAYSTATION2
// Las VU0_ en linea de UVectorMath.hpp, como en DeltaQ.h, y su Coord4NegOneXYZ (DeltaQFastPhysical::UnQuantize).
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "DeltaQ.h"
#endif

namespace EAGL4Anim {

// Las constantes de cuantizacion de este fichero, en el orden del DWARF de GameCube (sin direccion: el compilador
// las pliega). Las UnQuantize de los deltas las usan tal cual; el original no tiene funciones auxiliares.
static const unsigned short TwoPower6Minus1 = 63;
static const float OneOverTwoPower6Minus1 = 1.0f / TwoPower6Minus1;
static const unsigned short TwoPower12Minus1 = 4095;
static const float TwoOverTwoPower12Minus1 = 2.0f / TwoPower12Minus1;
// En el DWARF de GameCube (sin direccion) y en la A124 (.rodata 0x544360, los cuatro a -1): solo la
// UnQuantize de PS2 lo lee.
static const COORD4 Coord4NegOne = {-1.0f, -1.0f, -1.0f, -1.0f};

// total size: 0x20
struct DeltaQFastMinRangef {
    UMath::Vector4 mMin;   // offset 0x0, size 0x10
    UMath::Vector4 mRange; // offset 0x10, size 0x10
};

// total size: 0x10
struct DeltaQFastMinRange {
    void UnQuantize(DeltaQFastMinRangef &minRangef) const {
        const float RangeScale16Bit = 2.0f / 65535.0f;

#ifdef EA_PLATFORM_PLAYSTATION2
        // Como DeltaQMinRange::UnQuantize: la A124 copia mMin en una temporal (`temp' en los stabs), la rellena
        // y la pasa por la VU0 (vmulx + vadd con Coord4NegOne, y vmulx solo para mRange).
        UMath::Vector4 temp = minRangef.mMin;

        temp.x = mMin[0];
        temp.y = mMin[1];
        temp.z = mMin[2];
        temp.w = mMin[3];
        VU0_v4scaleadd(temp, RangeScale16Bit, Coord4NegOne, temp);
        minRangef.mMin = temp;

        temp.x = mRange[0];
        temp.y = mRange[1];
        temp.z = mRange[2];
        temp.w = mRange[3];
        VU0_v4scale(temp, RangeScale16Bit, temp);
        minRangef.mRange = temp;
#else
        minRangef.mMin.x = mMin[0] * RangeScale16Bit - 1.0f;
        minRangef.mMin.y = mMin[1] * RangeScale16Bit - 1.0f;
        minRangef.mMin.z = mMin[2] * RangeScale16Bit - 1.0f;
        minRangef.mMin.w = mMin[3] * RangeScale16Bit - 1.0f;

        minRangef.mRange.x = mRange[0] * RangeScale16Bit;
        minRangef.mRange.y = mRange[1] * RangeScale16Bit;
        minRangef.mRange.z = mRange[2] * RangeScale16Bit;
        minRangef.mRange.w = mRange[3] * RangeScale16Bit;
#endif
    }

    unsigned short mMin[4];   // offset 0x0, size 0x8
    unsigned short mRange[4]; // offset 0x8, size 0x8
};

// total size: 0x6
struct DeltaQFastPhysical {
    void UnQuantize(UMath::Vector4 &q) const {
#ifdef EA_PLATFORM_PLAYSTATION2
        // Como DeltaQPhysical::UnQuantize: la A124 escala las cuatro componentes y resta el uno con la VU0
        // (vadd.xyzw con el vector de -1 de .rodata 0x544350, Coord4NegOneXYZ en los stabs).
        q.x = mX * TwoOverTwoPower12Minus1;
        q.y = mY * TwoOverTwoPower12Minus1;
        q.z = mZ * TwoOverTwoPower12Minus1;

        unsigned short w = (mW0 << 8) + (mW1 << 4) | mW2;

        q.w = w * TwoOverTwoPower12Minus1;
        VU0_v4add(q, Coord4NegOneXYZ, q);
#else
        q.x = mX * TwoOverTwoPower12Minus1 - 1.0f;
        q.y = mY * TwoOverTwoPower12Minus1 - 1.0f;
        q.z = mZ * TwoOverTwoPower12Minus1 - 1.0f;

        unsigned short w = (mW0 << 8) + (mW1 << 4) | mW2;

        q.w = w * TwoOverTwoPower12Minus1 - 1.0f;
#endif
    }

    // Como en DeltaQPhysical: en el EE (little-endian) el primer campo es el bit bajo, y los stabs de la A124
    // declaran mW0 delante de mX (mX sale con `srl 4' y mW0 con `andi 0xf'). Todos `short unsigned int'.
#ifdef EA_PLATFORM_PLAYSTATION2
    unsigned short mW0 : 4; // offset 0x0, size 0x2
    unsigned short mX : 12; // offset 0x0, size 0x2
    unsigned short mW1 : 4; // offset 0x2, size 0x2
    unsigned short mY : 12; // offset 0x2, size 0x2
    unsigned short mW2 : 4; // offset 0x4, size 0x2
    unsigned short mZ : 12; // offset 0x4, size 0x2
#else
    unsigned short mX : 12; // offset 0x0, size 0x2
    unsigned char mW0 : 4;  // offset 0x0, size 0x2
    unsigned short mY : 12; // offset 0x2, size 0x2
    unsigned char mW1 : 4;  // offset 0x2, size 0x2
    unsigned short mZ : 12; // offset 0x4, size 0x2
    unsigned char mW2 : 4;  // offset 0x4, size 0x2
#endif
};

// total size: 0x3
struct DeltaQFastDelta {
    static float DeQuantize(int v) {
        return v * OneOverTwoPower6Minus1;
    }

    void UnQuantize(const DeltaQFastMinRangef &minRangef, UMath::Vector4 &q) const {
        q.x = minRangef.mRange.x * DeQuantize(mX) + minRangef.mMin.x;
        q.y = minRangef.mRange.y * DeQuantize(mY) + minRangef.mMin.y;
        q.z = minRangef.mRange.z * DeQuantize(mZ) + minRangef.mMin.z;

        unsigned char w = (mW0 << 4) + (mW1 << 2) | mW2;

        q.w = minRangef.mRange.w * DeQuantize(w) + minRangef.mMin.w;
    }

    // Igual que en DeltaQFastPhysical: la A124 declara mW0 delante de mX (mX con `srl 2', mW0 con `andi 3').
#ifdef EA_PLATFORM_PLAYSTATION2
    unsigned char mW0 : 2; // offset 0x0, size 0x1
    unsigned char mX : 6;  // offset 0x0, size 0x1
    unsigned char mW1 : 2; // offset 0x1, size 0x1
    unsigned char mY : 6;  // offset 0x1, size 0x1
    unsigned char mW2 : 2; // offset 0x2, size 0x1
    unsigned char mZ : 6;  // offset 0x2, size 0x1
#else
    unsigned char mX : 6;  // offset 0x0, size 0x1
    unsigned char mW0 : 2; // offset 0x0, size 0x1
    unsigned char mY : 6;  // offset 0x1, size 0x1
    unsigned char mW1 : 2; // offset 0x1, size 0x1
    unsigned char mZ : 6;  // offset 0x2, size 0x1
    unsigned char mW2 : 2; // offset 0x2, size 0x1
#endif
};

// total size: 0x14
struct DeltaQFast : public AnimMemoryMap {
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

    void GetArrays(DeltaQFastMinRange *&minRanges, unsigned char *&binStart, unsigned char *&constBoneIndices, DeltaQFastPhysical *&constPhysical) {
        minRanges = reinterpret_cast<DeltaQFastMinRange *>(&mPadding[1]);
        binStart = &reinterpret_cast<unsigned char *>(minRanges)[mNumBones * sizeof(DeltaQFastMinRange)];

        constBoneIndices = GetConstBoneIdx();
        constPhysical = reinterpret_cast<DeltaQFastPhysical *>(GetConstPhysical());
    }

    int GetBinSize() const {
        return AlignSize2(mNumBones * ((GetBinLength() - 1) * sizeof(DeltaQFastDelta) + sizeof(DeltaQFastPhysical)));
    }

    DeltaQFastMinRange *GetMinRange() {
        unsigned char *memBytes = reinterpret_cast<unsigned char *>(&mPadding[1]);
        return reinterpret_cast<DeltaQFastMinRange *>(memBytes);
    }

    unsigned char *GetBin(int binIdx) {
        const int bs = GetBinSize();
        unsigned char *memPos = &reinterpret_cast<unsigned char *>(GetMinRange())[mNumBones * sizeof(DeltaQFastMinRange)];
        return &memPos[binIdx * bs];
    }

    DeltaQFastPhysical *GetPhysical(unsigned char *binData) {
        return reinterpret_cast<DeltaQFastPhysical *>(binData);
    }

    DeltaQFastDelta *GetDelta(unsigned char *binData, int deltaIdx) {}

    unsigned char *GetConstBoneIdx() {
        const int binSize = AlignSize2(mNumBones * ((GetBinLength() - 1) * sizeof(DeltaQFastDelta) + sizeof(DeltaQFastPhysical)));
        unsigned char *s = &GetBin(0)[binSize * (mNumKeys / GetBinLength())];
        int r = mNumKeys % GetBinLength();

        if (r > 0) {
            s = &s[mNumBones * ((r - 1) * sizeof(DeltaQFastDelta) + sizeof(DeltaQFastPhysical))];
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
