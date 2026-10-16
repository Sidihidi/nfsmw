#ifndef SPEED_INDEP_DEBUG_COMMON_DEBUGDRAW_H
#define SPEED_INDEP_DEBUG_COMMON_DEBUGDRAW_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"

#include "Speed/Indep/Libs/Support/Utility/FastMem.h"

// The debug draw primitives only exist in the PS2 prototype (SLES-53558-A124): its .mdebug keeps this header
// (src/Debug/DebugDraw.h) with the layout of each struct and the lines of its inline functions.

struct ePolyVertex {
    float x;
    float y;
    float z;
    unsigned int colour;
    float u;
    float v;
    float q;
    float t;
};

struct DebugDraw;

// total size: 0x4
struct DrawPrimTri {
    short int fLifeSpan; // offset 0x0, size 0x2
    short int fTimeType; // offset 0x2, size 0x2

    DrawPrimTri() {
        fLifeSpan = 0;
        fTimeType = 0;
    }

    void InitTriangle(unsigned int *pColours, UMath::Vector4 *pVertices, const UMath::Vector4 &pt0, const UMath::Vector4 &pt1,
                      const UMath::Vector4 &pt2, unsigned int c, short int lifeSpan);
};

// total size: 0x4
struct DrawPrimLin {
    short int fLifeSpan; // offset 0x0, size 0x2
    short int fTimeType; // offset 0x2, size 0x2

    DrawPrimLin() {
        fLifeSpan = 0;
        fTimeType = 0;
    }

    void InitLineSeg(unsigned int *pColours, UMath::Vector4 *pVertices, const UMath::Vector4 &pt0, const UMath::Vector4 &pt1,
                     unsigned int c, short int lifeSpan);
};

// total size: 0x64
struct DebugDraw {
    friend struct DrawPrimTri;
    friend struct DrawPrimLin;

  private:
    static DebugDraw *fgDbgDraw;
    bool fZTest;                      // offset 0x0, size 0x4
    bool fEnabled;                    // offset 0x4, size 0x4
    bool fCheckDrawOverlap;           // offset 0x8, size 0x4
    short int fTimeType;              // offset 0xC, size 0x2
    int fNumTriPrims;                 // offset 0x10, size 0x4
    int fNumLinPrims;                 // offset 0x14, size 0x4
    DrawPrimTri *fTriPrimList;        // offset 0x18, size 0x4
    unsigned int *fTriColourList;     // offset 0x1C, size 0x4
    UMath::Vector4 *fTriVertList;     // offset 0x20, size 0x4
    TextureInfo *fTextureInfo;        // offset 0x24, size 0x4
    DrawPrimLin *fLinPrimList;        // offset 0x28, size 0x4
    unsigned int *fLinColourList;     // offset 0x2C, size 0x4
    UMath::Vector4 *fLinVertList;     // offset 0x30, size 0x4
    UMath::Vector4 fCachedCameraPos;   // offset 0x34, size 0x10
    UMath::Vector4 fCachedCameraUpVec; // offset 0x44, size 0x10
    UMath::Vector4 fCachedCameraFwdVec; // offset 0x54, size 0x10

  public:
    static void Init() {
        fgDbgDraw = new ("DebugDraw", 0) DebugDraw;
    }

    static void Shutdown();

    DebugDraw();

    static DebugDraw &Get() {
        if (!fgDbgDraw) {
            Init();
        }
        return *fgDbgDraw;
    }

    void AllocLists();
    void DeAllocLists();

    const UMath::Vector4 &GetCameraPos() {
        return fCachedCameraPos;
    }

    const UMath::Vector4 &GetCameraFwdVec() {
        return fCachedCameraFwdVec;
    }

    const UMath::Vector4 &GetCameraUpVec() {
        return fCachedCameraUpVec;
    }

    void Enable() {
        fEnabled = true;
    }

    void Disable() {
        fEnabled = false;
    }

    bool IsEnabled() {
        return fEnabled;
    }

    void SetEnable(bool b) {
        if (b) {
            AllocLists();
        }
        fEnabled = b;
    }

    void SetZTest(bool b) {
        fZTest = b;
    }

    void DoDrawOverlapCheck(bool b) {
        fCheckDrawOverlap = b;
    }

    bool IsDoingDrawCheck() {
        return fCheckDrawOverlap;
    }

    void DrawAll();

    void Triangle(const UMath::Vector4 *pt0, const UMath::Vector4 *pt1, const UMath::Vector4 *pt2, const UMath::Vector2 *uv0,
                  const UMath::Vector2 *uv1, const UMath::Vector2 *uv2, unsigned int c, short int lifeSpan, int texture);
    void Triangle(const UMath::Matrix4 &pmat, const UMath::Vector4 *pt0, const UMath::Vector4 *pt1, const UMath::Vector4 *pt2,
                  const UMath::Vector2 *uv0, const UMath::Vector2 *uv1, const UMath::Vector2 *uv2, unsigned int c,
                  short int lifeSpan, int texture);
    void Triangle(const UMath::Matrix4 &mat, const UMath::Vector4 *pt0, const UMath::Vector4 *pt1, const UMath::Vector4 *pt2,
                  unsigned int c, short int lifeSpan, int texture);
    void Triangle(const UMath::Matrix4 &mat, const UMath::Vector3 *pt0, const UMath::Vector3 *pt1, const UMath::Vector3 *pt2,
                  unsigned int c, short int lifeSpan, int texture);
    void Quad(const UMath::Vector4 *pt0, const UMath::Vector4 *pt1, const UMath::Vector4 *pt2, const UMath::Vector4 *pt3,
              unsigned int c, short int lifeSpan, int texture);
    void Quad(const UMath::Matrix4 &mat, const UMath::Vector4 *pt0, const UMath::Vector4 *pt1, const UMath::Vector4 *pt2,
              const UMath::Vector4 *pt3, unsigned int c, short int lifeSpan, int texture);
    void Quad(const UMath::Vector4 &pt, float size, unsigned int c, short int lifeSpan, int texture);
    void Box(const UMath::Vector4 *tpts, unsigned int c, short int lifeSpan, bool bShadow, int texture);
    void Box(const UMath::Matrix4 &mat, float width, float height, float depth, unsigned int c, short int lifeSpan,
             bool bShadow, int texture, bool frombase);
    void Circle(const UMath::Matrix4 &mat, float radius, unsigned int c, short int lifeSpan, int texture);
    void Cylinder(const UMath::Matrix4 &mat, float radius, float height, unsigned int c, short int lifeSpan, bool bShadow,
                  int texture);
    void Cylinder(const UMath::Vector3 &basePt, float radius, float height, unsigned int c, short int lifeSpan,
                  bool bShadow, int texture);
    void CylinderSection(const UMath::Matrix4 &mat, float radiusTop, float radiusBot, float height, unsigned int c,
                         short int lifeSpan, int texture);
    void Sphere(const UMath::Matrix4 &mat, float radius, unsigned int c, short int lifeSpan, bool bShadow, int texture);
    void Sphere(const UMath::Vector3 &basePt, float radius, unsigned int c, short int lifeSpan, bool bShadow, int texture);
    void Cone(const UMath::Matrix4 &mat, float height, float radius, unsigned int c, short int lifeSpan);
    void LineSeg(const UMath::Vector3 &pt0, const UMath::Vector3 &pt1, unsigned int c, short int lifeSpan);
    void LineSeg(const UMath::Matrix4 &mat, const UMath::Vector4 &pt0, const UMath::Vector4 &pt1, unsigned int c,
                 short int lifeSpan);
    void Line(const UMath::Matrix4 &mat, const UMath::Vector3 &pt1, const UMath::Vector3 &pt2, float scale, unsigned int c,
              short int lifeSpan);
    void Line(const UMath::Matrix4 &mat, const UMath::Vector4 &pt1, const UMath::Vector4 &pt2, float scale, unsigned int c,
              short int lifeSpan);
    void Vector(const UMath::Vector3 &pt, const UMath::Vector3 &vec, float scale, unsigned int c, short int lifeSpan,
                int texture);
    void Vector(const UMath::Vector4 &pt, const UMath::Vector4 &vec, float scale, unsigned int c, short int lifeSpan,
                int texture);
    bool OutOfRange(const UMath::Vector4 &pt, float radius);
    bool OutOfRange(const UMath::Vector3 &pt, float radius);
    bool InView(const UMath::Vector3 &pt, float radius);
    float GetDrawRange();

    void SetTimeType(short int timeType) {
        fTimeType = timeType;
    }

    short int GetTimeType();

  private:
    void CacheCameraInfo();
};

// The two Init functions are compiled before GetTimeType has a body, so the prototype calls the out-of-line copy of
// GetTimeType that it emits at the end of the unit.
inline void DrawPrimTri::InitTriangle(unsigned int *pColours, UMath::Vector4 *pVertices, const UMath::Vector4 &pt0,
                                      const UMath::Vector4 &pt1, const UMath::Vector4 &pt2, unsigned int c,
                                      short int lifeSpan) {
    pVertices[0].x = pt0.x; pVertices[0].y = pt0.y; pVertices[0].z = pt0.z; pVertices[0].w = pt0.w;
    pVertices[1].x = pt1.x; pVertices[1].y = pt1.y; pVertices[1].z = pt1.z; pVertices[1].w = pt1.w;
    pVertices[2].x = pt2.x; pVertices[2].y = pt2.y; pVertices[2].z = pt2.z; pVertices[2].w = pt2.w;
    pColours[0] = c;
    pColours[1] = c;
    pColours[2] = c;
    fLifeSpan = lifeSpan;
    fTimeType = DebugDraw::fgDbgDraw->GetTimeType();
}

inline void DrawPrimLin::InitLineSeg(unsigned int *pColours, UMath::Vector4 *pVertices, const UMath::Vector4 &pt0,
                                     const UMath::Vector4 &pt1, unsigned int c, short int lifeSpan) {
    pVertices[0].x = pt0.x; pVertices[0].y = pt0.y; pVertices[0].z = pt0.z; pVertices[0].w = pt0.w;
    pVertices[1].x = pt1.x; pVertices[1].y = pt1.y; pVertices[1].z = pt1.z; pVertices[1].w = pt1.w;
    pColours[0] = pColours[1] = c;
    fLifeSpan = lifeSpan;
    fTimeType = DebugDraw::fgDbgDraw->GetTimeType();
}

inline short int DebugDraw::GetTimeType() {
    return fTimeType;
}

void eDrawKLine(const ePolyVertex *verts, unsigned int numLines);

#endif
