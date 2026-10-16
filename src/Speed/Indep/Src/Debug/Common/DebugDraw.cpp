#include "Speed/Indep/Src/Debug/Common/DebugDraw.h"
#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/World/WCollisionMgr.h"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Libs/Support/Utility/FastMem.h"

#include <math.h>
#include <string.h>

extern float Tweak_drawRange;

int sDebugDrawMaxPrims = 2000;
int sDebugDrawMaxLinePrims = 0;

DebugDraw *DebugDraw::fgDbgDraw;

void DebugDraw::AllocLists() {
    if (fTextureInfo == NULL) {
        fTextureInfo = GetTextureInfo(bStringHash("GREYMAP"), 1, 0);
        fTriPrimList = new ("DrawPrimTri[]", 0) DrawPrimTri[sDebugDrawMaxPrims];
        fTriColourList = static_cast<unsigned int *>(bMalloc(sDebugDrawMaxPrims * 3 * sizeof(unsigned int), "DebugDraw TriCols", 0, 0));
        fTriVertList = static_cast<UMath::Vector4 *>(bMalloc(sDebugDrawMaxPrims * 3 * sizeof(UMath::Vector4), "DebugDraw TriVerts", 0, 0));
        fLinPrimList = new ("DrawPrimLin[]", 0) DrawPrimLin[sDebugDrawMaxLinePrims];
        fLinColourList = static_cast<unsigned int *>(bMalloc(sDebugDrawMaxLinePrims * 2 * sizeof(unsigned int), "DebugDraw LinCols", 0, 0));
        fLinVertList = static_cast<UMath::Vector4 *>(bMalloc(sDebugDrawMaxLinePrims * 2 * sizeof(UMath::Vector4), "DebugDraw LinVerts", 0, 0));
    }
}

void DebugDraw::DeAllocLists() {
    if (fTextureInfo != NULL) {
        delete[] fTriPrimList;
        delete[] fLinPrimList;

        UMemory::Free(fTriColourList);
        UMemory::Free(fTriVertList);
        UMemory::Free(fLinColourList);
        UMemory::Free(fLinVertList);
    }

    fTextureInfo = NULL;

    fNumTriPrims = 0;
    fNumLinPrims = 0;
    fEnabled = false;
}

DebugDraw::DebugDraw()
    : fZTest(true), fEnabled(false), fCheckDrawOverlap(true), fNumTriPrims(0), fNumLinPrims(0), fTextureInfo(NULL) {}

void DebugDraw::CacheCameraInfo() {
    eView *view = eGetView(EVIEW_PLAYER1, false);

    if (!view->IsActive()) {
        fCachedCameraPos = UMath::Vector4::kIdentity;
        fCachedCameraFwdVec = UMath::Vector4::kIdentity;
        fCachedCameraUpVec = UMath::Vector4::kIdentity;
    }

    {
        bVector3 cPos = *view->GetCamera()->GetPosition();
        bVector3 posV3;

        eUnSwizzleWorldVector(cPos, posV3);
        fCachedCameraPos = UMath::Vector4Make(*reinterpret_cast<UMath::Vector3 *>(&posV3), 1.0f);
    }

    {
        bVector3 cDir = *view->GetCamera()->GetDirection();
        bVector3 dirV3;

        eUnSwizzleWorldVector(cDir, dirV3);
        fCachedCameraFwdVec = UMath::Vector4Make(*reinterpret_cast<UMath::Vector3 *>(&dirV3), 1.0f);
    }

    {
        bVector3 cUp = *view->GetCamera()->GetUpVec();
        bVector3 upV3;

        eUnSwizzleWorldVector(cUp, upV3);
        upV3 = -upV3;
        fCachedCameraUpVec = UMath::Vector4Make(*reinterpret_cast<UMath::Vector3 *>(&upV3), 1.0f);
    }
}

bool DebugDraw::InView(const UMath::Vector3 &pt, float radius) {
    UMath::Vector3 camPos;
    UMath::Vector3 camVec;
    UMath::Vector3 objVec;

    camPos = UMath::Vector4To3(Get().GetCameraPos());
    camVec = UMath::Vector4To3(Get().GetCameraFwdVec());

    UMath::Sub(pt, camPos, objVec);
    if (UMath::Length(objVec) < radius) {
        return true;
    }

    if (0.707f > UMath::Abs(camVec.y)) {
        objVec.y = camVec.y = 0.0f;

        if (UMath::Dot(objVec, camVec) < 0.0f) {
            return false;
        }
    }

    return true;
}

bool DebugDraw::OutOfRange(const UMath::Vector3 &pt, float radius) {
    UMath::Vector3 pos = UMath::Vector4To3(Get().GetCameraPos());
    UMath::Vector3 diff;

    UMath::Sub(pos, pt, diff);

    float distSq = diff.x * diff.x + diff.z * diff.z;

    float range = Tweak_drawRange + radius;

    bool outOfRange = range * range < distSq;

    return outOfRange;
}

bool DebugDraw::OutOfRange(const UMath::Vector4 &pt, float radius) {
    return OutOfRange(UMath::Vector4To3(pt), radius);
}

static inline bVector3 Coord4ToSwizzledbVec(const UMath::Vector4 *c) {
    bVector3 vec;
    bVector3 temp;

    temp.x = c->x;
    temp.y = c->y;
    temp.z = c->z;
    eSwizzleWorldVector(temp, vec);
    return vec;
}

void DebugDraw::DrawAll() {
    eView *view = eGetView(EVIEW_PLAYER1, false);
    static int lastSimFrame;
    int simDecayFrames;

    if (view == NULL) {
        return;
    }

    bMatrix4 *matrix = static_cast<bMatrix4 *>(eFrameMalloc(sizeof(bMatrix4)));

    if (matrix == NULL) {
        return;
    }

    bIdentity(matrix);
    view->BiasMatrixForZSorting(matrix, 0.96f);

    if (fNumTriPrims > 0) {
        ePoly poly;
        int iCurVert = 0;
        int iNumVertsLeft = fNumTriPrims * 3;

        while (iNumVertsLeft > 0) {
            poly.Vertices[0] = Coord4ToSwizzledbVec(&fTriVertList[iCurVert]);
            poly.Vertices[1] = Coord4ToSwizzledbVec(&fTriVertList[iCurVert + 1]);
            poly.Vertices[2] = Coord4ToSwizzledbVec(&fTriVertList[iCurVert + 2]);
            poly.Vertices[3] = poly.Vertices[0];

            unsigned int c = fTriColourList[iCurVert];
            *reinterpret_cast<unsigned int *>(poly.Colours[0]) = *reinterpret_cast<unsigned int *>(poly.Colours[1]) =
                *reinterpret_cast<unsigned int *>(poly.Colours[2]) = *reinterpret_cast<unsigned int *>(poly.Colours[3]) = c;

            view->Render(&poly, fTextureInfo, matrix, 0, 0.0f);

            iNumVertsLeft -= 3;
            iCurVert += 3;
        }
    }

    if (fNumLinPrims > 0) {
        ePolyVertex verts[2];
        int iCurVert = 0;
        int iNumVertsLeft = fNumLinPrims * 2;

        while (iNumVertsLeft > 2) {
            verts[0].x = fLinVertList[iCurVert].x;
            verts[0].y = fLinVertList[iCurVert].y;
            verts[0].z = fLinVertList[iCurVert].z;
            verts[0].colour = ((fLinColourList[iCurVert] & 0xFF) << 16) | (fLinColourList[iCurVert] & 0xFF00) | ((fLinColourList[iCurVert] & 0xFF0000) >> 16) | (fLinColourList[iCurVert] & 0xFF000000);
            verts[0].u = 0.5f;
            verts[0].v = 0.5f;

            verts[1].x = fLinVertList[iCurVert + 1].x;
            verts[1].y = fLinVertList[iCurVert + 1].y;
            verts[1].z = fLinVertList[iCurVert + 1].z;
            verts[1].colour = ((fLinColourList[iCurVert] & 0xFF) << 16) | (fLinColourList[iCurVert] & 0xFF00) | ((fLinColourList[iCurVert] & 0xFF0000) >> 16) | (fLinColourList[iCurVert] & 0xFF000000);
            verts[1].u = 0.5f;
            verts[1].v = 0.5f;

            eSwizzleWorldVector(*reinterpret_cast<bVector3 *>(&verts[0]), *reinterpret_cast<bVector3 *>(&verts[0]));
            eSwizzleWorldVector(*reinterpret_cast<bVector3 *>(&verts[1]), *reinterpret_cast<bVector3 *>(&verts[1]));

            eDrawKLine(verts, 1);

            iNumVertsLeft -= 2;
            iCurVert += 2;
        }
    }

    simDecayFrames = eGetFrameCounter() - lastSimFrame;
    lastSimFrame = eGetFrameCounter();

    for (int iCurVert = 0; iCurVert < fNumTriPrims; iCurVert++) {
        if (fTriPrimList[iCurVert].fTimeType == 1) {
            if (fTriPrimList[iCurVert].fLifeSpan > 0) {
                fTriPrimList[iCurVert].fLifeSpan -= simDecayFrames;
            }
            if (fTriPrimList[iCurVert].fLifeSpan < 0) {
                fTriPrimList[iCurVert].fLifeSpan = 0;
            }
        } else if (fTriPrimList[iCurVert].fLifeSpan > 0) {
            fTriPrimList[iCurVert].fLifeSpan--;
        }
    }

    {
        int endIndex = fNumTriPrims - 1;

        for (int s = 0; s < fNumTriPrims; s++) {
            if (fTriPrimList[s].fLifeSpan == 0) {
                while (s <= endIndex && fTriPrimList[endIndex].fLifeSpan == 0) {
                    endIndex--;
                }
                if (s < endIndex) {
                    fTriPrimList[s] = fTriPrimList[endIndex];

                    fTriColourList[s * 3] = fTriColourList[endIndex * 3];
                    fTriColourList[s * 3 + 1] = fTriColourList[endIndex * 3 + 1];
                    fTriColourList[s * 3 + 2] = fTriColourList[endIndex * 3 + 2];

                    fTriVertList[s * 3] = fTriVertList[endIndex * 3];
                    fTriVertList[s * 3 + 1] = fTriVertList[endIndex * 3 + 1];
                    fTriVertList[s * 3 + 2] = fTriVertList[endIndex * 3 + 2];

                    endIndex--;
                }
            }
        }
        fNumTriPrims = endIndex + 1;
    }

    for (int iCurVert = 0; iCurVert < fNumLinPrims; iCurVert++) {
        if (fLinPrimList[iCurVert].fTimeType == 1) {
            if (fLinPrimList[iCurVert].fLifeSpan > 0) {
                fLinPrimList[iCurVert].fLifeSpan -= simDecayFrames;
            }
            if (fLinPrimList[iCurVert].fLifeSpan < 0) {
                fLinPrimList[iCurVert].fLifeSpan = 0;
            }
        } else if (fLinPrimList[iCurVert].fLifeSpan > 0) {
            fLinPrimList[iCurVert].fLifeSpan--;
        }
    }

    {
        int endIndex = fNumLinPrims - 1;

        for (int s = 0; s < fNumLinPrims; s++) {
            if (fLinPrimList[s].fLifeSpan == 0) {
                while (s <= endIndex && fLinPrimList[endIndex].fLifeSpan == 0) {
                    endIndex--;
                }
                if (s < endIndex) {
                    fLinPrimList[s] = fLinPrimList[endIndex];

                    fLinColourList[s * 3] = fLinColourList[endIndex * 3];
                    fLinColourList[s * 3 + 1] = fLinColourList[endIndex * 3 + 1];

                    fLinVertList[s * 3] = fLinVertList[endIndex * 3];
                    fLinVertList[s * 3 + 1] = fLinVertList[endIndex * 3 + 1];

                    endIndex--;
                }
            }
        }
        fNumLinPrims = endIndex + 1;
    }

    CacheCameraInfo();
}

void DebugDraw::LineSeg(const UMath::Vector3 &pt0, const UMath::Vector3 &pt1, unsigned int c, short int lifeSpan) {
    UMath::Vector4 pt14 = UMath::Vector4Make(pt0, 1.0f);
    UMath::Vector4 pt24 = UMath::Vector4Make(pt1, 1.0f);

    LineSeg(UMath::Matrix4::kIdentity, pt14, pt24, c, lifeSpan);
}

void DebugDraw::LineSeg(const UMath::Matrix4 &mat, const UMath::Vector4 &pt0, const UMath::Vector4 &pt1, unsigned int c,
                        short int lifeSpan) {
    if (fEnabled) {
        if (fNumLinPrims < sDebugDrawMaxLinePrims) {
            UMath::Vector4 tpt0;
            UMath::Vector4 tpt1;

            VU0_MATRIX4_vect4mult(pt0, mat, tpt0);
            VU0_MATRIX4_vect4mult(pt1, mat, tpt1);
            fLinPrimList[fNumLinPrims].InitLineSeg(&fLinColourList[fNumLinPrims * 2], &fLinVertList[fNumLinPrims * 2], tpt0,
                                                   tpt1, c, lifeSpan);
            fNumLinPrims++;
        }
    }
}

void DebugDraw::Triangle(const UMath::Matrix4 &mat, const UMath::Vector3 *pt0, const UMath::Vector3 *pt1,
                         const UMath::Vector3 *pt2, unsigned int c, short int lifeSpan, int texture) {
    if (!fEnabled) {
        return;
    }

    UMath::Vector2 uv0 = {0.0f, 0.0f};
    UMath::Vector2 uv1 = {0.0f, 0.0f};
    UMath::Vector2 uv2 = {0.0f, 0.0f};

    UMath::Vector4 pt04 = UMath::Vector4Make(*pt0, 1.0f);
    UMath::Vector4 pt14 = UMath::Vector4Make(*pt1, 1.0f);
    UMath::Vector4 pt24 = UMath::Vector4Make(*pt2, 1.0f);

    Triangle(mat, &pt04, &pt14, &pt24, &uv0, &uv1, &uv2, c, lifeSpan, texture);
}

void DebugDraw::Triangle(const UMath::Matrix4 &mat, const UMath::Vector4 *pt0, const UMath::Vector4 *pt1,
                         const UMath::Vector4 *pt2, unsigned int c, short int lifeSpan, int texture) {
    UMath::Vector2 uv0;
    UMath::Vector2 uv1;
    UMath::Vector2 uv2;

    if (fEnabled) {
        memset(&uv0, 0, sizeof(uv0));
        memset(&uv1, 0, sizeof(uv1));
        memset(&uv2, 0, sizeof(uv2));

        if (fNumTriPrims < sDebugDrawMaxPrims) {
            UMath::Vector4 tpt0;
            UMath::Vector4 tpt1;
            UMath::Vector4 tpt2;

            UMath::RotateTranslate(*pt0, mat, tpt0);
            UMath::RotateTranslate(*pt1, mat, tpt1);
            UMath::RotateTranslate(*pt2, mat, tpt2);
            fTriPrimList[fNumTriPrims].InitTriangle(&fTriColourList[fNumTriPrims * 3], &fTriVertList[fNumTriPrims * 3], tpt0,
                                                    tpt1, tpt2, c, lifeSpan);
            fNumTriPrims++;
        }
    }
}

void DebugDraw::Triangle(const UMath::Vector4 *pt0, const UMath::Vector4 *pt1, const UMath::Vector4 *pt2,
                         const UMath::Vector2 *uv0, const UMath::Vector2 *uv1, const UMath::Vector2 *uv2, unsigned int c,
                         short int lifeSpan, int texture) {
    Triangle(UMath::Matrix4::kIdentity, pt0, pt1, pt2, uv0, uv1, uv2, c, lifeSpan, texture);
}

void DebugDraw::Triangle(const UMath::Matrix4 &pmat, const UMath::Vector4 *pt0, const UMath::Vector4 *pt1,
                         const UMath::Vector4 *pt2, const UMath::Vector2 *uv0, const UMath::Vector2 *uv1,
                         const UMath::Vector2 *uv2, unsigned int c, short int lifeSpan, int texture) {
    if (fEnabled) {
        if (fNumTriPrims < sDebugDrawMaxPrims) {
            UMath::Matrix4 mat = pmat;
            UMath::Vector4 tpt0;
            UMath::Vector4 tpt1;
            UMath::Vector4 tpt2;

            UMath::RotateTranslate(*pt0, mat, tpt0);
            UMath::RotateTranslate(*pt1, mat, tpt1);
            UMath::RotateTranslate(*pt2, mat, tpt2);
            fTriPrimList[fNumTriPrims].InitTriangle(&fTriColourList[fNumTriPrims * 3], &fTriVertList[fNumTriPrims * 3], tpt0,
                                                    tpt1, tpt2, c, lifeSpan);
            fNumTriPrims++;
        }
    }
}

void DebugDraw::Quad(const UMath::Vector4 *pt0, const UMath::Vector4 *pt1, const UMath::Vector4 *pt2,
                     const UMath::Vector4 *pt3, unsigned int c, short int lifeSpan, int texture) {
    UMath::Vector2 uv0 = {0.0f, 1.0f};
    UMath::Vector2 uv1 = {1.0f, 1.0f};
    UMath::Vector2 uv2 = {1.0f, 0.0f};
    UMath::Vector2 uv3 = {0.0f, 0.0f};

    Triangle(pt0, pt1, pt2, &uv0, &uv1, &uv2, c, lifeSpan, texture);
    Triangle(pt2, pt3, pt0, &uv2, &uv3, &uv0, c, lifeSpan, texture);
}

void DebugDraw::Quad(const UMath::Matrix4 &mat, const UMath::Vector4 *pt0, const UMath::Vector4 *pt1,
                     const UMath::Vector4 *pt2, const UMath::Vector4 *pt3, unsigned int c, short int lifeSpan,
                     int texture) {
    UMath::Vector2 uv0 = {0.0f, 1.0f};
    UMath::Vector2 uv1 = {1.0f, 1.0f};
    UMath::Vector2 uv2 = {1.0f, 0.0f};
    UMath::Vector2 uv3 = {0.0f, 0.0f};

    Triangle(mat, pt0, pt1, pt2, &uv0, &uv1, &uv2, c, lifeSpan, texture);
    Triangle(mat, pt2, pt3, pt0, &uv2, &uv3, &uv0, c, lifeSpan, texture);
}

void DebugDraw::Quad(const UMath::Vector4 &pt, float size, unsigned int c, short int lifeSpan, int texture) {
    if (!fEnabled) {
        return;
    }

    UMath::Vector4 inVec = Get().GetCameraFwdVec();
    UMath::Vector4 upVec = Get().GetCameraUpVec();
    UMath::Vector4 rightVec;
    UMath::Vector4 centerPos;
    UMath::Vector4 pt04;
    UMath::Vector4 pt14;
    UMath::Vector4 pt24;
    UMath::Vector4 pt34;

    UMath::Crossxyz(upVec, inVec, rightVec);
    centerPos = pt;

    UMath::ScaleAdd(upVec, size, centerPos, pt04);
    UMath::ScaleAdd(rightVec, -size, pt04, pt04);
    UMath::ScaleAdd(upVec, size, centerPos, pt14);
    UMath::ScaleAdd(rightVec, size, pt14, pt14);
    UMath::ScaleAdd(upVec, -size, centerPos, pt24);
    UMath::ScaleAdd(rightVec, size, pt24, pt24);
    UMath::ScaleAdd(upVec, -size, centerPos, pt34);
    UMath::ScaleAdd(rightVec, -size, pt34, pt34);

    UMath::Vector2 uv0 = {0.0f, 1.0f};
    UMath::Vector2 uv1 = {1.0f, 1.0f};
    UMath::Vector2 uv2 = {1.0f, 0.0f};
    UMath::Vector2 uv3 = {0.0f, 0.0f};

    Triangle(&pt04, &pt14, &pt24, &uv0, &uv1, &uv2, c, lifeSpan, texture);
    Triangle(&pt24, &pt34, &pt04, &uv2, &uv3, &uv0, c, lifeSpan, texture);
}

void DebugDraw::Line(const UMath::Matrix4 &mat, const UMath::Vector3 &pt1, const UMath::Vector3 &pt2, float scale,
                     unsigned int c, short int lifeSpan) {
    if (!fEnabled) {
        return;
    }

    UMath::Vector4 pt14 = UMath::Vector4Make(pt1, 1.0f);
    UMath::Vector4 pt24 = UMath::Vector4Make(pt2, 1.0f);

    Line(mat, pt14, pt24, scale, c, lifeSpan);
}

void DebugDraw::Line(const UMath::Matrix4 &mat, const UMath::Vector4 &pt1, const UMath::Vector4 &pt2, float scale,
                     unsigned int c, short int lifeSpan) {
    if (!fEnabled) {
        return;
    }

    UMath::Vector4 inVec = Get().GetCameraFwdVec();
    UMath::Vector4 widthVec;
    UMath::Vector4 vec;
    UMath::Vector4 pts[4];

    UMath::Clear(vec);
    UMath::Subxyz(pt2, pt1, vec);
    if (UMath::Lengthxyz(vec) < 0.0001f) {
        Quad(pt1, scale, c, lifeSpan, -1);
        return;
    }

    UMath::UnitCrossxyz(inVec, vec, widthVec);

    UMath::ScaleAddxyz(widthVec, -scale, pt1, pts[0]);
    pts[0].w = 1.0f;
    UMath::ScaleAddxyz(widthVec, scale, pt1, pts[1]);
    pts[1].w = 1.0f;
    UMath::ScaleAddxyz(widthVec, scale, pt2, pts[2]);
    pts[2].w = 1.0f;
    UMath::ScaleAddxyz(widthVec, -scale, pt2, pts[3]);
    pts[3].w = 1.0f;

    Quad(mat, &pts[0], &pts[1], &pts[2], &pts[3], c, lifeSpan, -1);
}

void DebugDraw::Vector(const UMath::Vector3 &pt, const UMath::Vector3 &vec, float scale, unsigned int c,
                       short int lifeSpan, int texture) {
    UMath::Vector4 pt4 = UMath::Vector4Make(pt, 1.0f);
    UMath::Vector4 vector4 = UMath::Vector4Make(vec, 1.0f);

    Vector(pt4, vector4, scale, c, lifeSpan, texture);
}

void DebugDraw::Vector(const UMath::Vector4 &pt, const UMath::Vector4 &vec, float scale, unsigned int c,
                       short int lifeSpan, int texture) {
    if (!fEnabled) {
        return;
    }

    UMath::Vector4 pts[3];
    UMath::Vector4 inVec = Get().GetCameraFwdVec();
    UMath::Vector4 widthVec;

    UMath::Crossxyz(inVec, vec, widthVec);
    UMath::Scalexyz(widthVec, scale * 0.05f, widthVec);

    pts[0] = pt;
    pts[0].w = 1.0f;

    UMath::ScaleAddxyz(vec, scale, pt, pts[1]);
    UMath::Subxyz(pts[1], widthVec, pts[1]);
    pts[1].w = 1.0f;

    UMath::ScaleAddxyz(vec, scale, pt, pts[2]);
    UMath::Addxyz(pts[2], widthVec, pts[2]);
    pts[2].w = 1.0f;

    UMath::Vector2 uv0 = {0.0f, 0.0f};
    UMath::Vector2 uv1 = {0.5f, 1.0f};
    UMath::Vector2 uv2 = {1.0f, 0.0f};

    Triangle(&pts[0], &pts[1], &pts[2], &uv0, &uv1, &uv2, c, lifeSpan, texture);
}

void DebugDraw::Box(const UMath::Vector4 *tpts, unsigned int c, short int lifeSpan, bool bShadow, int texture) {
    if (!fEnabled) {
        return;
    }

    Quad(&tpts[0], &tpts[1], &tpts[2], &tpts[3], c, lifeSpan, texture);
    Quad(&tpts[4], &tpts[5], &tpts[6], &tpts[7], c, lifeSpan, texture);
    Quad(&tpts[0], &tpts[4], &tpts[7], &tpts[3], c, lifeSpan, texture);
    Quad(&tpts[1], &tpts[5], &tpts[4], &tpts[0], c, lifeSpan, texture);
    Quad(&tpts[2], &tpts[6], &tpts[5], &tpts[1], c, lifeSpan, texture);
    Quad(&tpts[3], &tpts[7], &tpts[6], &tpts[2], c, lifeSpan, texture);

    if (bShadow) {
        UMath::Vector4 trans1;
        UMath::Vector4 trans2;

        trans1.x = (tpts[0].x + tpts[2].x) * 0.5f;
        trans1.y = tpts[0].y;
        trans1.z = (tpts[0].z + tpts[2].z) * 0.5f;
        trans1.w = 1.0f;

        trans2 = trans1;

        if (WCollisionMgr(0, 3).GetWorldHeightAtPoint(UMath::Vector4To3(trans1), trans2.y, NULL)) {
            UTransform t(UMath::Matrix3::kIdentity, UMath::Vector4To3(trans2));

            Box(t.fTransform, 1.0f, 0.01f, 1.0f, 0x1f888888, 1, false, -1, true);
            UMath::Subxyz(trans2, trans1, trans2);
            trans2.w = 1.0f;
            Vector(trans1, trans2, 1.0f, 0x1f888888, 1, -1);
        }
    }
}

void DebugDraw::Box(const UMath::Matrix4 &mat, float width, float height, float depth, unsigned int c,
                    short int lifeSpan, bool bShadow, int texture, bool frombase) {
    UMath::Vector4 tpts[8];

    if (fEnabled) {
        if (frombase) {
            UMath::Vector4 pts[8] = {
                {-width * 0.5f, 0.0f, -depth * 0.5f, 1.0f},  {-width * 0.5f, 0.0f, depth * 0.5f, 1.0f},
                {width * 0.5f, 0.0f, depth * 0.5f, 1.0f},    {width * 0.5f, 0.0f, -depth * 0.5f, 1.0f},
                {-width * 0.5f, height, -depth * 0.5f, 1.0f}, {-width * 0.5f, height, depth * 0.5f, 1.0f},
                {width * 0.5f, height, depth * 0.5f, 1.0f},   {width * 0.5f, height, -depth * 0.5f, 1.0f},
            };

            UMath::RotateTranslate(pts, mat, tpts, 8);
        } else {
            UMath::Vector4 pts[8] = {
                {-width * 0.5f, -height * 0.5f, -depth * 0.5f, 1.0f}, {-width * 0.5f, -height * 0.5f, depth * 0.5f, 1.0f},
                {width * 0.5f, -height * 0.5f, depth * 0.5f, 1.0f},   {width * 0.5f, -height * 0.5f, -depth * 0.5f, 1.0f},
                {-width * 0.5f, height * 0.5f, -depth * 0.5f, 1.0f},  {-width * 0.5f, height * 0.5f, depth * 0.5f, 1.0f},
                {width * 0.5f, height * 0.5f, depth * 0.5f, 1.0f},    {width * 0.5f, height * 0.5f, -depth * 0.5f, 1.0f},
            };

            UMath::RotateTranslate(pts, mat, tpts, 8);
        }

        Box(tpts, c, lifeSpan, bShadow, texture);
    }
}

void DebugDraw::Cone(const UMath::Matrix4 &mat, float height, float radius, unsigned int c, short int lifeSpan) {
    int numsteps = 20;

    for (int i = 0; i < numsteps; i++) {
        float angle = static_cast<float>(i) / numsteps;
        float nextangle = static_cast<float>(i + 1) / numsteps;
        UMath::Vector3 p0 = {0.0f, 0.0f, 0.0f};
        UMath::Vector3 p1;
        UMath::Vector3 p2;

        p1.x = UMath::Sina(angle) * radius;
        p1.y = -UMath::Cosa(angle) * radius;
        p1.z = height;

        p2.x = UMath::Sina(nextangle) * radius;
        p2.y = -UMath::Cosa(nextangle) * radius;
        p2.z = height;

        Triangle(mat, &p0, &p1, &p2, c, lifeSpan, -1);
    }
}

void DebugDraw::Sphere(const UMath::Vector3 &basePt, float radius, unsigned int c, short int lifeSpan, bool bShadow,
                       int texture) {
    UMath::Matrix4 mat;

    if (fEnabled) {
        mat = UMath::Matrix4::kIdentity;
        mat.v3 = UMath::Vector4Make(basePt, 1.0f);
        Sphere(mat, radius, c, lifeSpan, bShadow, texture);
    }
}

void DebugDraw::Sphere(const UMath::Matrix4 &mat, float radius, unsigned int c, short int lifeSpan, bool bShadow,
                       int texture) {
    const int kMaxNumFacets = 12;
    const int kMinNumFacets = 2;
    int numFacets;
    UMath::Matrix4 tmpMat;
    float angle;
    float angleStep;

    if (!fEnabled) {
        return;
    }
    if (OutOfRange(mat.v3, radius)) {
        return;
    }

    numFacets = static_cast<int>(static_cast<float>(log(radius)) + 9.0f);
    tmpMat = mat;
    angle = 0.0f;
    tmpMat.v3.y -= radius;
    numFacets = UMath::Min(kMaxNumFacets, numFacets);
        numFacets = UMath::Max(numFacets, kMinNumFacets);
    angleStep = 0.5f / numFacets;

    for (int i = 0; i < numFacets; i++) {
        float localRadTop = radius * UMath::Cosa(angle - 0.25 + angleStep);
        float localRadBot = radius * UMath::Cosa(angle - 0.25);
        float sectionHeight = UMath::Abs(radius * (UMath::Sina(angle - 0.25) - UMath::Sina(angle - 0.25 + angleStep)));

        CylinderSection(tmpMat, localRadTop, localRadBot, sectionHeight, c, lifeSpan, texture);
        angle += angleStep;
        tmpMat.v3.y += sectionHeight;
    }

    if (bShadow == true) {
        UMath::Vector4 trans1;
        UMath::Vector4 trans2;

        trans1 = mat.v3;
        trans1.w = 1.0f;
        trans2 = trans1;

        if (WCollisionMgr(0, 3).GetWorldHeightAtPoint(UMath::Vector4To3(trans1), trans2.y, NULL)) {
            UTransform t(UMath::Matrix3::kIdentity, UMath::Vector4To3(trans2));

            Box(t.fTransform, 1.0f, 0.01f, 1.0f, 0x1f888888, 1, false, -1, true);
            UMath::Subxyz(trans2, trans1, trans2);
            trans2.w = 1.0f;
            Vector(trans1, trans2, 1.0f, 0x1f888888, 1, -1);
        }
    }
}

void DebugDraw::Cylinder(const UMath::Vector3 &basePt, float radius, float height, unsigned int c, short int lifeSpan,
                         bool bShadow, int texture) {
    UMath::Matrix4 mat;

    if (fEnabled) {
        mat = UMath::Matrix4::kIdentity;
        mat.v3 = UMath::Vector4Make(basePt, 1.0f);
        Cylinder(mat, radius, height, c, lifeSpan, bShadow, texture);
    }
}

void DebugDraw::Cylinder(const UMath::Matrix4 &mat, float radius, float height, unsigned int c, short int lifeSpan,
                         bool bShadow, int texture) {
    UMath::Matrix4 matTop;

    if (fEnabled) {
        CylinderSection(mat, radius, radius, height, c, lifeSpan, texture);
        Circle(mat, radius, c, lifeSpan, texture);
        matTop = mat;
        matTop.v3.y += height;
        Circle(matTop, radius, c, lifeSpan, texture);

        if (bShadow == true) {
            UMath::Vector4 trans1;
            UMath::Vector4 trans2;

            trans1 = mat.v3;
            trans1.w = 1.0f;
            trans2 = trans1;

            if (WCollisionMgr(0, 3).GetWorldHeightAtPoint(UMath::Vector4To3(trans1), trans2.y, NULL)) {
                UTransform t(UMath::Matrix3::kIdentity, UMath::Vector4To3(trans2));

                Box(t.fTransform, 1.0f, 0.01f, 1.0f, 0x1f888888, 1, false, -1, true);
                UMath::Subxyz(trans2, trans1, trans2);
                trans2.w = 1.0f;
                Vector(trans1, trans2, 1.0f, 0x1f888888, 1, -1);
            }
        }
    }
}

void DebugDraw::CylinderSection(const UMath::Matrix4 &mat, float radiusTop, float radiusBot, float height, unsigned int c,
                                short int lifeSpan, int texture) {
    const int kMaxNumFacets = 12;
    const int kMinNumFacets = 4;
    int numFacets;
    UMath::Vector4 pts[12];
    UMath::Vector4 topTpts[12];
    UMath::Vector4 botTpts[12];

    if (fEnabled) {
        numFacets = static_cast<int>(static_cast<float>(log(radiusTop + radiusBot)) * 5.0f + 6.0f);
        numFacets = UMath::Min(kMaxNumFacets, numFacets);
        numFacets = UMath::Max(numFacets, kMinNumFacets);

        {
            float angle = 0.0f;

            for (int i = 0; i < numFacets; i++) {
                pts[i].x = radiusBot * UMath::Cosa(angle);
                pts[i].y = 0.0f;
                pts[i].z = radiusBot * UMath::Sina(angle);
                pts[i].w = 1.0f;
                angle += 1.0f / numFacets;
            }
        }
        UMath::RotateTranslate(pts, mat, botTpts, numFacets);

        {
            float angle = 0.0f;

            for (int i = 0; i < numFacets; i++) {
                pts[i].x = radiusTop * UMath::Cosa(angle);
                pts[i].y = height;
                pts[i].z = radiusTop * UMath::Sina(angle);
                pts[i].w = 1.0f;
                angle += 1.0f / numFacets;
            }
        }
        UMath::RotateTranslate(pts, mat, topTpts, numFacets);

        for (int i = 0; i < numFacets - 1; i++) {
            Quad(&topTpts[i], &topTpts[i + 1], &botTpts[i + 1], &botTpts[i], c, lifeSpan, texture);
        }
        Quad(&topTpts[numFacets - 1], &topTpts[0], &botTpts[0], &botTpts[numFacets - 1], c, lifeSpan, texture);
    }
}

void DebugDraw::Circle(const UMath::Matrix4 &mat, float radius, unsigned int c, short int lifeSpan, int texture) {
    if (fEnabled) {
        const int kNumFacets = 12;
        UMath::Vector4 pts[kNumFacets];
        UMath::Vector2 txtOrigin = {0.5f, 0.5f};
        UMath::Vector2 txtXY[kNumFacets];
        float txtRadius = 0.5f;
        float angle = 0.0f;
        UMath::Vector4 tpts[kNumFacets];

        for (int i = 0; i < kNumFacets; i++) {
            txtXY[i].x = txtRadius * UMath::Cosa(angle) + 0.5;
            txtXY[i].y = txtRadius * UMath::Sina(angle) + 0.5;
            pts[i].x = radius * UMath::Cosa(angle);
            pts[i].y = 0.0f;
            pts[i].z = radius * UMath::Sina(angle);
            pts[i].w = 1.0f;
            angle += 1.0f / kNumFacets;
        }

        UMath::RotateTranslate(pts, mat, tpts, kNumFacets);

        UMath::Vector4 cp = {0.0f, 0.0f, 0.0f, 1.0f};

        UMath::RotateTranslate(cp, mat, cp);

        for (int i = 0; i < kNumFacets - 1; i++) {
            Triangle(&cp, &tpts[i], &tpts[i + 1], &txtOrigin, &txtXY[i], &txtXY[i + 1], c, lifeSpan, texture);
        }
        Triangle(&cp, &tpts[kNumFacets - 1], &tpts[0], &txtOrigin, &txtXY[kNumFacets - 1], &txtXY[0], c, lifeSpan,
                 texture);
    }
}

float DebugDraw::GetDrawRange() {
    return Tweak_drawRange;
}
