#ifndef WORLD_WCOLLISION_H
#define WORLD_WCOLLISION_H

#include <types.h>

#include "Speed/Indep/Src/Physics/Dynamics/Collision.h"
#include "WCollisionTri.h"
#include "WSurfaceTypes.h"

// total size: 0x8
struct WCollisionPackedVert {
    short x;                        // offset 0x0, size 0x2
    short y;                        // offset 0x2, size 0x2
    short z;                        // offset 0x4, size 0x2
    CARP::CollisionSurface surface; // offset 0x6, size 0x2
};

struct WCollisionBarrier;

// total size: 0x10
class WCollisionStripSphere {
  public:
    unsigned int Offset() const {
        return static_cast<unsigned int>(fOffset) + 0x10;
    }

    UMath::Vector3 fPos;    // offset 0x0, size 0xC
    unsigned short fRadius; // offset 0xC, size 0x2

  private:
    unsigned short fOffset; // offset 0xE, size 0x2
};

// total size: 0x1
struct WCollisionStrip {
    enum Flags {
        kUpFacing = 1,
        kFacingUnknown = 2,
    };

    const WCollisionPackedVert *Verts() const {
        return reinterpret_cast<const WCollisionPackedVert *>(this);
    }

    WCollisionPackedVert *Verts() {
        return reinterpret_cast<WCollisionPackedVert *>(this);
    }

    unsigned int NumVerts() const {
        return *reinterpret_cast<const unsigned short *>(&Verts()[0].surface);
    }

    unsigned int NumTris() const {
        return NumVerts() - 2;
    }

    unsigned int TotalSize() const {
        return NumVerts() * sizeof(WCollisionPackedVert);
    }

    const WCollisionStrip *NextStrip() const {
        return reinterpret_cast<const WCollisionStrip *>(reinterpret_cast<const char *>(this) + TotalSize());
    }

    unsigned int Flags() const {
        return *reinterpret_cast<const unsigned short *>(&Verts()[1].surface);
    }

    inline void MakeFace(unsigned int ind, const UMath::Vector3 &cp, WCollisionTri &retFace) const {
        const WCollisionPackedVert *v = &Verts()[ind];
        retFace.fPt0.x = cp.x + static_cast<float>(v->x) / 128.0f;
        retFace.fPt0.y = cp.y + static_cast<float>(v->y) / 128.0f;
        retFace.fPt0.z = cp.z + static_cast<float>(v->z) / 128.0f;
#ifdef EA_BUILD_A124
        retFace.fSurfaceHash = nullptr;
#else
        retFace.fSurfaceRef = nullptr;
#endif
        v++;
        retFace.fPt1.x = cp.x + static_cast<float>(v->x) / 128.0f;
        retFace.fPt1.y = cp.y + static_cast<float>(v->y) / 128.0f;
        retFace.fPt1.z = cp.z + static_cast<float>(v->z) / 128.0f;
        retFace.fFlags = 0;
        v++;
        retFace.fPt2.x = cp.x + static_cast<float>(v->x) / 128.0f;
        retFace.fPt2.y = cp.y + static_cast<float>(v->y) / 128.0f;
        retFace.fPt2.z = cp.z + static_cast<float>(v->z) / 128.0f;
        retFace.fSurface = WSurface(v->surface);
    }

    inline void MakeNextFace(unsigned int ind, const UMath::Vector3 &cp, WCollisionTri &retFace) const {
        const WCollisionPackedVert *v = Verts() + ind + 2;
        retFace.fPt0 = retFace.fPt1;
        retFace.fPt1 = retFace.fPt2;
        retFace.fPt2.x = cp.x + static_cast<float>(v->x) * (1.0f / 128.0f);
        retFace.fPt2.y = cp.y + static_cast<float>(v->y) * (1.0f / 128.0f);
        retFace.fPt2.z = cp.z + static_cast<float>(v->z) * (1.0f / 128.0f);
        retFace.fSurface = WSurface(v->surface);
    }
};

// total size: 0x10
struct WCollisionArticle {
    void Resolve();

    // The article, its strip spheres, strips, edges and surfaces, rounded up to 64 bytes.
    unsigned int TotalSize() const {
        unsigned int size = sizeof(WCollisionArticle) + fStripsSize;
        size += fEdgesSize;
        size += fSurfacesSize;
        return (size + 63) & 0x0FFFFFC0;
    }

#ifdef EA_BUILD_A124
    // La A124 guarda en el articulo el HASH de la superficie y su GetSurface devuelve una referencia a el (stabs:
    // `unsigned int &GetSurface()`, con dataStart como unica local): es la GetSurfaceIndRef de GameCube con el nombre
    // viejo. Quien la llama busca la superficie con SimSurface::Lookup.
    const unsigned int &GetSurface(unsigned int ind) const {
        const char *dataStart = reinterpret_cast<const char *>(&this[1]) + fStripsSize + fEdgesSize;
        return reinterpret_cast<const unsigned int *>(dataStart)[ind];
    }
#else
    const Attrib::Collection *GetSurface(unsigned int ind) const {
        // TODO 64 bit
        const char *dataStart = reinterpret_cast<const char *>(&this[1]) + fStripsSize + fEdgesSize;
        unsigned int ref = reinterpret_cast<const unsigned int *>(dataStart)[ind];
        return reinterpret_cast<const Attrib::Collection *>(ref);
    }
#endif

    const WCollisionBarrier *GetBarrier(unsigned int ind) const {
        const char *dataStart = reinterpret_cast<const char *>(&this[1]) + fStripsSize;
        return reinterpret_cast<const WCollisionBarrier *>(dataStart + ind * 0x10); // sizeof(WCollisionBarrier)
    }

    const WCollisionStripSphere *GetStripSphere(unsigned int ind) const {
        const char *dataStart = reinterpret_cast<const char *>(&this[1]);
        const WCollisionStripSphere *stripSp = reinterpret_cast<const WCollisionStripSphere *>(dataStart);
        return &stripSp[ind];
    }

#ifdef EA_BUILD_A124
    // In the prototype the strips follow the strip spheres one after the other (WDbgCollisionDraw walks them with
    // NextStrip); the offset of each sphere came later.
    const WCollisionStrip *GetStrip() const {
        return reinterpret_cast<const WCollisionStrip *>(GetStripSphere(fNumStrips));
    }
#endif

    unsigned short fNumStrips;         // offset 0x0, size 0x2
    unsigned short fStripsSize;        // offset 0x2, size 0x2
    unsigned short fNumEdges;          // offset 0x4, size 0x2
    unsigned short fEdgesSize;         // offset 0x6, size 0x2
    unsigned char fResolvedFlag;       // offset 0x8, size 0x1
    unsigned char fNumSurfaces;        // offset 0x9, size 0x1
    unsigned short fSurfacesSize;      // offset 0xA, size 0x2
    unsigned short fIntermediatObjInd; // offset 0xC, size 0x2
    short fFlags;                      // offset 0xE, size 0x2
};

// total size: 0x20
struct WCollisionBarrier {
    const WSurface &GetWSurface() const {
        // TODO why 0xC?
        return *reinterpret_cast<const WSurface *>(reinterpret_cast<const char *>(this) + 0xC);
    }

    const WCollisionBarrier *Next() const {
        return &this[1];
    }

    const UMath::Vector4 *GetPts() const {
        return fPts;
    }

    const UMath::Vector4 *GetPt(int ptInd) const {
        return &fPts[ptInd];
    }

    const float YBot() const {
        return fPts[0].y < fPts[1].y ? fPts[0].y : fPts[1].y;
    }

    const float YTop() const {
        return fPts[1].y < fPts[0].y ? fPts[0].y : fPts[1].y;
    }

    float GetInvXZLength() const {
        return fPts[1].w;
    }

    void GetNormal(UMath::Vector3 &norm) const {
        float invLen = GetInvXZLength();
        norm.x = (fPts[1].z - fPts[0].z) * invLen;
        norm.y = 0.0f;
        norm.z = -(fPts[1].x - fPts[0].x) * invLen;
    }

    void GetCenter(UMath::Vector4 &cp) const {
        cp.x = (fPts[0].x + fPts[1].x) * 0.5f;
        cp.y = (fPts[0].y + fPts[1].y) * 0.5f;
        cp.z = (fPts[0].z + fPts[1].z) * 0.5f;
    }

    float GetWidth() const {
        float rx = fPts[0].x - fPts[1].x;
        float rz = fPts[0].z - fPts[1].z;
        return UMath::Sqrt(rx * rx + rz * rz);
    }

    float GetHeight() const {
        return UMath::Abs(fPts[0].y - fPts[1].y);
    }

    float DistSq(const UMath::Vector3 &pt) const {
        float x1 = fPts[0].x;
        float z1 = fPts[0].z;
        float x2 = fPts[1].x;
        float z2 = fPts[1].z;
        float px = pt.x;
        float pz = pt.z;
        float u = (px - x1) * (x2 - x1) + (pz - z1) * (z2 - z1);
        float invLen = GetInvXZLength();
        u *= invLen * invLen;

        if (u < 0.0f) {
            return (x1 - px) * (x1 - px) + (z1 - pz) * (z1 - pz);
        } else if (u > 1.0f) {
            return (x2 - px) * (x2 - px) + (z2 - pz) * (z2 - pz);
        } else {
            float nearX = x1 + u * (x2 - x1) - px;
            float nearZ = z1 + u * (z2 - z1) - pz;
            return nearX * nearX + nearZ * nearZ;
        }
    }

    UMath::Vector4 fPts[2]; // offset 0x0, size 0x20
};

// total size: 0x28
struct WCollisionBarrierListEntry {
    WCollisionBarrier fB; // offset 0x0, size 0x20
#ifdef EA_BUILD_A124
    UCrc32 fSurfaceHash;
#else
    const Attrib::Collection *fSurfaceRef; // offset 0x20, size 0x4
#endif
    float fDistanceToSq; // offset 0x24, size 0x4

    WCollisionBarrierListEntry()
        : fB(), //
#ifdef EA_BUILD_A124
          fSurfaceHash(), //
#else
          fSurfaceRef(nullptr), //
#endif
          fDistanceToSq(0.0f) {
    }

#ifdef EA_BUILD_A124
    // La A124 guarda el hash (stabs de GetBarrierList y GetBarrierNormal: surfaceHash es un UCrc32 en la pila).
    WCollisionBarrierListEntry(const WCollisionBarrier &b, const UCrc32 &surfHash, float disttosq)
        : fB(b), //
          fSurfaceHash(surfHash), //
          fDistanceToSq(disttosq) {
    }
#else
    WCollisionBarrierListEntry(const WCollisionBarrier &b, const Attrib::Collection *surfHash, float disttosq)
        : fB(b), //
          fSurfaceRef(surfHash), //
          fDistanceToSq(disttosq) {
    }
#endif

    bool operator<(const WCollisionBarrierListEntry &rhs) const {
        return fDistanceToSq < rhs.fDistanceToSq;
    }
};

struct WCollisionObject : public CARP::CollisionObject {
    // total size: 0x70
    enum Types {
        kBox = 0,
        kCylinder = 1,
    };

    const WSurface GetWSurface() const {
        return WSurface(fSurface.fSurface, fSurface.fFlags);
    }

    bool IsDynamic() const {
        return (fFlags & 1) != 0;
    }

    void MakeMatrix(UMath::Matrix4 &m, bool addXLate) const;
};

// total size: 0x40
struct WCollisionInstance : public CARP::CollisionInstance {
    void MakeMatrix(UMath::Matrix4 &m, bool addXLate) const;
    void CalcPosition(UMath::Vector3 &pos) const;
    const char *GetName() const;

    bool NeedsCrossProduct() const {
        return (fFlags & 3) != 0;
    }

    float CalcSphericalRadius() const;

    bool IsYVecNotUp() const {
        return (fFlags & 1) != 0;
    }

    bool IsDynamic() const {
        return (fFlags & 2) != 0;
    }
};

#endif
