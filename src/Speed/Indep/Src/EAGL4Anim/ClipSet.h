#ifndef EAGL4ANIM_CLIPSET_H
#define EAGL4ANIM_CLIPSET_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

namespace EAGL4Anim {

// Las declaraciones son las del DWARF de GameCube. Los accesores en linea no tienen cuerpo: solo los usan
// GetClipSQT y GetWeightedSQT (ClipSet.cpp), estripadas en las tres versiones.

// total size: 0x8
struct IndexValuePair {
    int mIndex;   // offset 0x0, size 0x4
    float mValue; // offset 0x4, size 0x4
};

// total size: 0x4
struct Clip {
    inline int GetNumPairs() const;

    inline const IndexValuePair *GetPairs() const;

    int mNumPairs; // offset 0x0, size 0x4
};

// total size: 0x8
struct ClipSet {
    inline int GetNumClips() const;

    inline const Clip *const *GetClips() const;

    inline const Clip *GetClip(int index) const;

    void GetClipSQT(int index, float *sqtBuffer);

    void GetWeightedSQT(const float *weights, float *sqtBuffer);

    int mNumClips;    // offset 0x0, size 0x4
    int mMaxSqtIndex; // offset 0x4, size 0x4
};

}; // namespace EAGL4Anim

#endif
