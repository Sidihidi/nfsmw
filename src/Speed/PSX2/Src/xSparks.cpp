#include "Speed/PSX2/Src/xSparks.h"
#include "Speed/PSX2/Src/Ecstasy/XSprites.hpp"
#include "Speed/Indep/Libs/Support/Utility/FastMem.h"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UTLAllocator.h"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/emitteruv.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/fuelcell_effect.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/fuelcell_emitter.h"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/Tools/AttribSys/Runtime/AttribSys.h"

#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

ParticleList gParticleList;

// total size: 0x80
struct CGEmitter {
  private:
    Attrib::Gen::fuelcell_emitter mEmitterDef; // offset 0x0, size 0x14
    Attrib::Gen::emitteruv mTextureUVs;        // offset 0x14, size 0x14
    UMath::Vector4 mVel PS2ALIGN16;            // offset 0x30, size 0x10
    UMath::Matrix4 mLocalWorld;                // offset 0x40, size 0x40

  public:
    CGEmitter(const Attrib::Collection *spec, const XenonEffectDef &eDef);
    void SpawnParticles(float dt, float intensity);
    ~CGEmitter();

    void *operator new(size_t size) {
        return gFastMem.Alloc(size, 0);
    }

    void operator delete(void *mem, size_t size) {
        if (mem != 0) {
            gFastMem.Free(mem, size, 0);
        }
    }
};

CGEmitter::CGEmitter(const Attrib::Collection *spec, const XenonEffectDef &eDef)
    : mEmitterDef(spec, 0, 0), mTextureUVs(mEmitterDef.emitteruv(), 0, 0) {
    this->mLocalWorld = eDef.mat;
    this->mVel = eDef.vel;
}

NGParticle *ParticleList::GetNextParticle() {
    if (this->mNumParticles < 300) {
        return &this->mParticles[this->mNumParticles++];
    }

    return 0;
}

void ParticleList::GeneratePolys() {
    if (this->mNumParticles == 0) {
        return;
    }

    if (this->mContrail_tex == 0) {
        this->mContrail_tex = GetTextureInfo(bStringHash("PS2_CONTRAIL"), 0, 0);
        this->mSparks_tex = GetTextureInfo(bStringHash("PS2_SPARKS"), 0, 0);
    }

    NGParticle *particle = this->mParticles;
    for (unsigned int i = 0; i < this->mNumParticles; i++) {
        if (particle->uv[0] == 0x7F) {
            this->mCurrentTexture = this->mContrail_tex;
        } else {
            this->mCurrentTexture = this->mSparks_tex;
        }
        NGSpriteManager.AddSpark(*particle, this->mCurrentTexture);
        particle++;
    }
}

// total size: 0x14
struct NGEffect {
  private:
    Attrib::Gen::fuelcell_effect mEffectDef; // offset 0x0, size 0x14

  public:
    NGEffect(const XenonEffectDef &eDef);

    void *operator new(size_t size) {
        return gFastMem.Alloc(size, 0);
    }

    void operator delete(void *mem, size_t size) {
        if (mem != 0) {
            gFastMem.Free(mem, size, 0);
        }
    }
};

DECLARE_CONTAINER_TYPE(XenonEffectDef);

typedef UTL::Std::vector<XenonEffectDef, _type_XenonEffectDef> XenonFXVec;

// [0] recibe los efectos pendientes (AddXenonEffect) y [1] los que suenan este frame; UpdateXenonEmitters los
// traspasa.
struct XenonEffectList : public XenonFXVec {
    XenonEffectList() {
        reserve(20);
    }
};

// Semilla propia del aleatorio de las chispas: se guarda y se restaura en cada tanda.
static unsigned int randomSeed = 0xDEADBEEF;

XenonEffectList gNGEffectList[2];

// Una tanda de particulas: la cantidad sale de NumParticles*intensidad menos su varianza, y cada una nace en un
// punto aleatorio del volumen, hereda velocidad del emisor y del evento, y vive Life menos su varianza. En PS2 el
// color va de 0 a 128, que es la intensidad plena del GS.
void CGEmitter::SpawnParticles(float dt, float intensity) {
    UMath::Matrix4 local_world;
    UMath::Matrix4 local_orientation;
    unsigned int random_seed;
    float life_variance;
    float life;
    int r;
    int g;
    int b;
    int a;
    unsigned int particleColor;
    float num_particles_variance;
    float num_particles;
    float particle_age_factor;
    float current_particle_age;

    if (intensity <= 0.0f) {
        return;
    }

    {
        local_world = this->mLocalWorld;
        local_orientation = local_world;
        local_orientation.v3.x = 0.0f;
        local_orientation.v3.y = 0.0f;
        local_orientation.v3.z = 0.0f;
        local_orientation.v3.w = 1.0f;

        random_seed = randomSeed;

        life_variance = this->mEmitterDef.Life() * this->mEmitterDef.LifeVariance();
        life = this->mEmitterDef.Life() - life_variance;

        r = (int)(this->mEmitterDef.Colour1().x * 128.0f);
        g = (int)(this->mEmitterDef.Colour1().y * 128.0f);
        b = (int)(this->mEmitterDef.Colour1().z * 128.0f);
        a = (int)(this->mEmitterDef.Colour1().w * 128.0f);
        particleColor = (a << 24) | (b << 16) | (g << 8) | r;

        num_particles_variance = intensity * this->mEmitterDef.NumParticles() * this->mEmitterDef.NumParticlesVariance() * 100.0f;
        num_particles = intensity * this->mEmitterDef.NumParticles() - num_particles_variance;

        particle_age_factor = dt / num_particles;
        current_particle_age = 0.0f;

        while (num_particles-- != 0.0f) {
            NGParticle *particle;
            float sparkLength;
            UMath::Vector4 pvel;
            UMath::Vector4 rand;
            UMath::Vector4 rotatedVel;
            float gravity;
            UMath::Vector4 ppos;

            particle = gParticleList.GetNextParticle();
            if (particle == 0) {
                break;
            }

            sparkLength = this->mEmitterDef.LengthStart();
            sparkLength += bRandom(this->mEmitterDef.LengthDelta(), &random_seed);
            if (sparkLength < 0.0f) {
                break;
            }

            float f = bMin(sparkLength, 255.0f);

            rand.x = 1.0f - (this->mEmitterDef.VelocityDelta().x - 2.0f * bRandom(this->mEmitterDef.VelocityDelta().x, &random_seed));
            rand.y = 1.0f - (this->mEmitterDef.VelocityDelta().y - 2.0f * bRandom(this->mEmitterDef.VelocityDelta().y, &random_seed));
            rand.z = 1.0f - (this->mEmitterDef.VelocityDelta().z - 2.0f * bRandom(this->mEmitterDef.VelocityDelta().z, &random_seed));
            UMath::Scalexyz(this->mEmitterDef.VelocityInherit(), this->mVel, pvel);
            UMath::Rotate(this->mEmitterDef.VelocityStart(), this->mLocalWorld, rotatedVel);
            UMath::Add(pvel, rotatedVel, pvel);
            UMath::Scalexyz(pvel, rand, pvel);

            gravity = this->mEmitterDef.GravityStart() - this->mEmitterDef.GravityDelta() + 2.0f * bRandom(this->mEmitterDef.GravityDelta(), &random_seed);

            ppos.x = this->mEmitterDef.VolumeCenter().x +
                     (bRandom(this->mEmitterDef.VolumeExtent().x, &random_seed) - this->mEmitterDef.VolumeExtent().x * 0.5f);
            ppos.y = this->mEmitterDef.VolumeCenter().y +
                     (bRandom(this->mEmitterDef.VolumeExtent().y, &random_seed) - this->mEmitterDef.VolumeExtent().y * 0.5f);
            ppos.z = this->mEmitterDef.VolumeCenter().z +
                     (bRandom(this->mEmitterDef.VolumeExtent().z, &random_seed) - this->mEmitterDef.VolumeExtent().z * 0.5f);
            ppos.w = 1.0f;
            UMath::RotateTranslate(ppos, local_world, ppos);

            UMath::ScaleAdd((const UMath::Vector3 &)pvel, current_particle_age, (const UMath::Vector3 &)ppos, particle->initialPos);
            particle->initialPos.z = particle->initialPos.z + gravity * current_particle_age * current_particle_age;
            particle->vel.x = pvel.x;
            particle->vel.y = pvel.y;
            particle->vel.z = pvel.z;
            particle->life = (unsigned short)(int)(life * 8191.0f);
            particle->age = current_particle_age;
            particle->gravity = gravity;
            particle->uv[0] = (unsigned char)(int)(this->mTextureUVs.StartU() * 255.0f);
            particle->length = (unsigned char)(int)f;
            particle->width = (unsigned char)(int)this->mEmitterDef.HeightStart();
            particle->color = particleColor;

            current_particle_age += particle_age_factor;
        }

        randomSeed = random_seed;
    }
}

void ParticleList::AgeParticles(float dt) {
    NGParticle *inParticle = this->mParticles;
    NGParticle *outParticle = this->mParticles;
    int numOutParticles = 0;

    for (int i = 0; i < static_cast<int>(this->mNumParticles); i++) {
        if (static_cast<float>(static_cast<int>(inParticle->life)) < dt * 8191.0f) {
            inParticle++;
        } else {
            *outParticle = *inParticle;
            outParticle->life = static_cast<unsigned short>(static_cast<float>(static_cast<int>(inParticle->life)) - dt * 8191.0f);
            outParticle->age += dt;
            inParticle++;
            outParticle++;
            numOutParticles++;
        }
    }

    this->mNumParticles = numOutParticles;
}

NGEffect::NGEffect(const XenonEffectDef &eDef) : mEffectDef(eDef.spec, 0, 0) {
    if (this->mEffectDef.IsValid()) {
        int numEmitters = this->mEffectDef.Num_NGEmitter();

        for (int i = 0; i < numEmitters; i++) {
            CGEmitter anEmitter(this->mEffectDef.NGEmitter(i).GetCollection(), eDef);
            anEmitter.SpawnParticles(1.0f / 30.0f, 1.0f);
        }
    }
}

// Envejece las chispas vivas, ejecuta los efectos pendientes (los que siguen enganchados a un EmitterGroup activo)
// y genera los quads.
void UpdateXenonEmitters(float dt) {
    gParticleList.AgeParticles(dt);

    for (XenonEffectDef *iter = gNGEffectList[0].begin(); iter != gNGEffectList[0].end(); iter++) {
        XenonEffectDef eDef = *iter;
        gNGEffectList[1].push_back(eDef);
    }

    gNGEffectList[0].clear();

    for (XenonEffectDef *iter = gNGEffectList[1].begin(); iter != gNGEffectList[1].end(); iter++) {
        XenonEffectDef eDef = *iter;

        if (eDef.piggyback_effect == 0 || eDef.piggyback_effect->IsEnabled()) {
            NGEffect anEffect(eDef);
        }
    }

    gNGEffectList[1].clear();

    gParticleList.GeneratePolys();
}

void DrawXenonEmitters(eView *view) {}

// Encola un efecto: matriz identidad con solo la fila de traslacion del parametro, la velocidad tal cual y el grupo
// al que se engancha. Como mucho 19 pendientes (las dos listas reservan 20).
void AddXenonEffect(EmitterGroup *piggyback_fx, const Attrib::Collection *spec, const UMath::Matrix4 *mat, const UMath::Vector4 *vel) {
    XenonEffectDef eDef;

    if (gNGEffectList[0].size() <= 19) {
        if (gNGEffectList[0].capacity() <= 19) {
            gNGEffectList[0].reserve(20);
        }

        if (gNGEffectList[1].capacity() <= 19) {
            gNGEffectList[1].reserve(20);
        }

        eDef.mat = UMath::Matrix4::kIdentity;
        eDef.mat.v3 = mat->v3;
        eDef.piggyback_effect = piggyback_fx;
        eDef.spec = spec;
        eDef.vel = *vel;

        gNGEffectList[0].push_back(eDef);
    }
}

inline CGEmitter::~CGEmitter() {}
