#ifndef __ECSTASY_ENGINE__XSPARK_H
#define __ECSTASY_ENGINE__XSPARK_H

#include "Speed/Indep/Src/Ecstasy/EmitterSystem.h"

#ifdef EA_PLATFORM_PLAYSTATION2
// Las chispas de PS2 (psx2/src/xsparks.cpp): los mismos tipos que el xSparks.h de GameCube, con el layout de los
// stabs de la A124.
struct TextureInfo;

// total size: 0x30
struct NGParticle {
    UMath::Vector3 initialPos; // offset 0x0, size 0xC
    unsigned int color;        // offset 0xC, size 0x4
    UMath::Vector3 vel;        // offset 0x10, size 0xC
    float gravity;             // offset 0x1C, size 0x4
    unsigned short life;       // offset 0x20, size 0x2
    unsigned char length;      // offset 0x22, size 0x1
    unsigned char width;       // offset 0x23, size 0x1
    unsigned char uv[1];       // offset 0x24, size 0x1
    float padAlign;            // offset 0x28, size 0x4
    float age;                 // offset 0x2C, size 0x4
};

// total size: 0x3850
struct ParticleList {
  public:
    unsigned int GetNumParticles() {
        return mNumParticles;
    }

    NGParticle *GetNextParticle();
    void GeneratePolys();
    void AgeParticles(float dt);

  private:
    NGParticle mParticles[300];     // offset 0x0, size 0x3840
    unsigned int mNumParticles;     // offset 0x3840, size 0x4
    TextureInfo *mContrail_tex;     // offset 0x3844, size 0x4
    TextureInfo *mSparks_tex;       // offset 0x3848, size 0x4
    TextureInfo *mCurrentTexture;   // offset 0x384C, size 0x4
};

// total size: 0x58
struct XenonEffectDef {
    UMath::Vector4 vel;                 // offset 0x0, size 0x10
    UMath::Matrix4 mat;                 // offset 0x10, size 0x40
    const Attrib::Collection *spec;     // offset 0x50, size 0x4
    EmitterGroup *piggyback_effect;     // offset 0x54, size 0x4
};

extern ParticleList gParticleList;
#endif

void AddXenonEffect(EmitterGroup *piggyback_fx, const Attrib::Collection *spec, const UMath::Matrix4 *mat, const UMath::Vector4 *vel);

#endif
