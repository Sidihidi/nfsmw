#include "Speed/Indep/Src/World/WDbgCollisionDraw.h"
#include "Speed/Indep/Src/World/WCollision.h"
#include "Speed/Indep/Src/World/WCollisionAssets.h"
#include "Speed/Indep/Src/World/WTrigger.h"
#include "Speed/Indep/Src/World/WWorldMath.h"
#include "Speed/Indep/Src/World/Scenery.hpp"
#include "Speed/Indep/Src/World/Common/WGrid.h"
#include "Speed/Indep/Src/Debug/Common/DebugDraw.h"
#include "Speed/Indep/Src/Sim/SimSurface.h"
#include "Speed/Indep/Libs/Support/Utility/UEALibs.hpp"
#include "Speed/Indep/Libs/Support/Utility/UTLFastVector.h"

bool Tweak_faceDraw;
float Tweak_drawRange;
bool Tweak_objectDraw;
bool Tweak_barrierDraw;
unsigned char Tweak_typeMaskInclude;
unsigned char Tweak_typeMaskExclude;
bool Tweak_triggerDraw;
bool gVehicleEffectsDraw;
static bool gResetCar;
static bool gSingleStep;
unsigned int gDebugDrawFlags;
bool gDisableWWorldPos;
bool gDisableWWorldPos2;
bool gDisableCheckHitWorld;

WDbgCollisionDraw *WDbgCollisionDraw::fgDbgCollisionDraw;

WDbgCollisionDraw::WDbgCollisionDraw() {
    fVoidSet = new ("WVoidSet", 0) WVoidSet;
}

static void DrawCollisionNodesInRange(float range, unsigned int flags) {
    if (WGrid::Initialized()) {
        UMath::Vector4 pos = DebugDraw::Get().GetCameraPos();
        UTL::FastVector<unsigned int, 16> nodeIndList;

        nodeIndList.reserve(256);
        WGrid::Get().FindNodes(UMath::Vector4To3(pos), range, nodeIndList);

        WDbgCollisionDraw::Get().CollisionNodeList(nodeIndList, flags);
    }
}

static bool InstInView(const WCollisionInstance &inst) {
    UMath::Vector3 instPos;

    inst.CalcPosition(instPos);

    return DebugDraw::Get().InView(instPos, UMath::Max(inst.fInvPosRadius.w, inst.fHeight));
}

static bool SetDebugVars() {
    return gDebugDrawFlags != 0 || Tweak_triggerDraw || Tweak_objectDraw || Tweak_barrierDraw || Tweak_faceDraw;
}

void WDbgCollisionDraw::StartFrame() {
    DebugDraw::Get().DrawAll();

    fVoidSet->clear();

    DebugDraw::Get().SetEnable(SetDebugVars());

    if (Tweak_objectDraw || Tweak_barrierDraw || Tweak_faceDraw || Tweak_triggerDraw) {
        DrawCollisionNodesInRange(DebugDraw::Get().GetDrawRange(), (Tweak_faceDraw ? 1 : 0) | (Tweak_objectDraw ? 8 : 0) |
                                                                       (Tweak_barrierDraw ? 2 : 0) | (Tweak_triggerDraw ? 0x20 : 0));
    }
}

void WDbgCollisionDraw::CollisionTri(const WCollisionTri &tri, const UMath::Matrix4 &mat, unsigned int c) {
    DebugDraw::Get().Triangle(mat, &tri.fPt0, &tri.fPt1, &tri.fPt2, c, 1, -1);
}

void WDbgCollisionDraw::CollisionBarrier(const WCollisionBarrier &barrier, const UMath::Matrix4 &mat, unsigned int c) {
    if (DebugDraw::Get().IsEnabled()) {
        if ((Tweak_typeMaskInclude && !barrier.GetWSurface().HasFlag(Tweak_typeMaskInclude)) ||
            barrier.GetWSurface().HasFlag(Tweak_typeMaskExclude)) {
            return;
        }

        UMath::Vector3 camPos = UMath::Vector4To3(DebugDraw::Get().GetCameraPos());
        UMath::Vector4 barCp;
        UMath::Vector3 barVec;
        UMath::Vector3 barNorm;

        barrier.GetCenter(barCp);
        barCp.x += mat.v3.x;
        barCp.z += mat.v3.z;
        UMath::Sub(UMath::Vector4To3(barCp), camPos, barVec);
        barrier.GetNormal(barNorm);

        if (UMath::Dot(barNorm, barVec) > 0.5f) {
            c = 0x7f88ff22;
        }

        UMath::Vector4 apex = {barrier.fPts[0].x, barrier.fPts[1].y, barrier.fPts[0].z, 1.0f};
        UMath::Vector4 pt1 = {barrier.fPts[0].x, barrier.fPts[0].y, barrier.fPts[0].z, 1.0f};
        UMath::Vector4 pt2 = {barrier.fPts[1].x, barrier.fPts[1].y, barrier.fPts[1].z, 1.0f};

        DebugDraw::Get().Triangle(mat, &pt1, &apex, &pt2, c, 1, -1);

        apex = UMath::Vector4Make(barrier.fPts[1].x, barrier.fPts[0].y, barrier.fPts[1].z, 1.0f);
        DebugDraw::Get().Triangle(mat, &pt1, &apex, &pt2, c, 1, -1);

        DebugDraw::Get().Line(mat, barrier.fPts[0], barrier.fPts[1], 0.05f, 0x7f228888, 1);
    }
}

bool WDbgCollisionDraw::HasBeenDrawnThisFrame(const void *test) {
    if (fVoidSet->find(test) != fVoidSet->end()) {
        return true;
    }

    fVoidSet->insert(test);
    return false;
}

void WDbgCollisionDraw::Trigger(const WTrigger &trig) {
    if (HasBeenDrawnThisFrame(&trig)) {
        return;
    }

    UMath::Vector3 base_point;
    UMath::Matrix4 matrix;

    trig.MakeMatrix(matrix, true, false);
    base_point = UMath::Vector4To3(matrix.v3);

    UMath::ScaleAdd(UMath::Vector4To3(matrix.v1), -trig.fHeight * 0.5f, base_point, base_point);

    if (trig.fShape == 3) {
        if (!trig.IsEnabled()) {
            DebugDraw::Get().Cylinder(base_point, trig.fPosRadius.w, trig.fHeight, 0x7f117f44, 1, false, -1);
        } else {
            DebugDraw::Get().Cylinder(base_point, trig.fPosRadius.w, trig.fHeight, 0x7f22ff88, 1, false, -1);
        }
    } else if (trig.fShape == 2) {
        if (!trig.IsEnabled()) {
            DebugDraw::Get().Sphere(base_point, trig.fPosRadius.w, 0x7f337f44, 1, false, -1);
        } else {
            DebugDraw::Get().Sphere(base_point, trig.fPosRadius.w, 0x7f66ff88, 1, false, -1);
        }
    } else if (trig.fShape == 1) {
        if (!trig.ValidateMatrix()) {
            DebugDraw::Get().Box(matrix, trig.fMatRow0Width.w + trig.fMatRow0Width.w, trig.fHeight + trig.fHeight,
                                 trig.fMatRow2Length.w + trig.fMatRow2Length.w, 0x7f0000ff, 1, false, -1, false);
        }
        if (!trig.IsEnabled()) {
            DebugDraw::Get().Box(matrix, trig.fMatRow0Width.w, trig.fHeight, trig.fMatRow2Length.w, 0x7f7f7f44, 1, false, -1,
                                 false);
        } else {
            DebugDraw::Get().Box(matrix, trig.fMatRow0Width.w, trig.fHeight, trig.fMatRow2Length.w, 0x7fffff88, 1, false, -1,
                                 false);
        }
    }

    if (trig.fFlags & 0x800) {
        UMath::Vector3 dirVec = UMath::Vector4To3(trig.fMatRow2Length);

        DebugDraw::Get().Vector(base_point, dirVec, trig.fPosRadius.w + 1.0f, 0x3fff0000, 1, -1);
    }
}

void WDbgCollisionDraw::CollisionTriStrip(const WCollisionArticle &cArt, const WCollisionStripSphere &sp,
                                          const WCollisionStrip &strip, const UMath::Matrix4 &mat) {
    if (DebugDraw::Get().IsEnabled()) {
        int numTris = strip.NumTris();
        WCollisionTri face;
        unsigned int surfaceInd = 0xffffffff;
        unsigned int surfaceColour = 0xeeeeeeee;

        strip.MakeFace(0, sp.fPos, face);

        for (int i = 0; i < numTris; i++) {
            if ((Tweak_typeMaskInclude == 0 || face.fSurface.HasFlag(Tweak_typeMaskInclude)) &&
                !face.fSurface.HasFlag(Tweak_typeMaskExclude)) {
                if (face.fSurface.Surface() != surfaceInd) {
                    surfaceInd = face.fSurface.Surface();

                    unsigned int surfaceCRC = cArt.GetSurface(surfaceInd);
                    UMath::Vector4 cv = Attrib::Gen::simsurface(SimSurface::Lookup(UCrc32(surfaceCRC)), 0, NULL).DEBUG_COLOUR();

                    surfaceColour = (static_cast<int>(cv.w * 127.0f) << 24) | (static_cast<int>(cv.z * 255.0f) << 16) |
                                    (static_cast<int>(cv.y * 255.0f) << 8) | static_cast<int>(cv.x * 255.0f);
                }

                CollisionTri(face, mat, surfaceColour);
            }

            if (i + 1 < numTris) {
                strip.MakeNextFace(i + 1, sp.fPos, face);
            }
        }
    }
}

void WDbgCollisionDraw::CollisionInst(const WCollisionInstance &inst, unsigned int flags) {
    if (HasBeenDrawnThisFrame(&inst + flags)) {
        return;
    }

    UMath::Matrix4 invMat;

    inst.MakeMatrix(invMat, true);

    UTransform mat(invMat);

    UMath::OrthoInverse(mat.fTransform);

    if (!DebugDraw::Get().OutOfRange(UMath::Vector4To3(mat.fTransform.v3), inst.fInvPosRadius.w)) {
        const WCollisionArticle *cArt = inst.fCollisionArticle;

        if (cArt) {
            CollisionArticle(*cArt, inst, mat.fTransform, flags);
        }
    }
}

void WDbgCollisionDraw::CollisionNode(unsigned short nodeInd, unsigned int c, unsigned int flags) {
    if (!DebugDraw::Get().IsEnabled()) {
        return;
    }

    WGridNode *node = WGrid::Get().GetNode(nodeInd);

    if (node) {
        const unsigned int *objIndPtr;
        WGridNode::iterator eIter(node, WGrid_kInstance);

        while ((objIndPtr = eIter.GetIndPtr()) != NULL) {
            UMath::Vector3 sNorm;
            UMath::Vector3 np0;
            const WCollisionInstance *cInst = WCollisionAssets::Get().Instance(*objIndPtr);

            if (cInst) {
                if (IsSceneryGroupEnabled(cInst->fGroupNumber)) {
                    if (cInst->IsDynamic()) {
                        CollisionInst(*cInst, flags);
                    } else {
                        CollisionInst(*cInst, flags);
                    }
                }
            }
        }

        if (flags & 0x20) {
            const unsigned int *trigIndPtr;
            WGridNode::iterator eIter(node, WGrid_kTrigger);

            while ((trigIndPtr = eIter.GetIndPtr()) != NULL) {
                Trigger(WCollisionAssets::Get().Trigger(*trigIndPtr));
            }
        }
    }
}

void WDbgCollisionDraw::CollisionArticle(const WCollisionArticle &ca, const WCollisionInstance &inst, const UMath::Matrix4 &mat,
                                         unsigned int flags) {
    if (!DebugDraw::Get().IsEnabled()) {
        return;
    }
    if (!InstInView(inst)) {
        return;
    }

    if (flags & 3) {
        const WCollisionStrip *strip = ca.GetStrip();
        const WCollisionStripSphere *sp = ca.GetStripSphere(0);

        for (int i = 0; i < ca.fNumStrips; i++) {
            WCollisionTri firstFace;

            strip->MakeFace(0, sp->fPos, firstFace);

            if ((flags & 1) && !firstFace.fSurface.HasFlag(2)) {
                if (inst.IsDynamic()) {
                    CollisionTriStrip(ca, *sp, *strip, mat);
                } else {
                    CollisionTriStrip(ca, *sp, *strip, mat);
                }
            }
            if ((flags & 2) && firstFace.fSurface.HasFlag(2)) {
                CollisionTriStrip(ca, *sp, *strip, mat);
            }

            sp++;
            strip = strip->NextStrip();
        }
    }

    if (flags & 0x10) {
        const WCollisionStripSphere *stripSp = ca.GetStripSphere(0);

        for (int i = 0; i < ca.fNumStrips; i++) {
            UMath::Vector3 pos;

            MATRIX4_vect3mult3(&stripSp->fPos, &mat, &pos);

            float radius = static_cast<float>(stripSp->fRadius) * 0.0625f;

            DebugDraw::Get().Sphere(pos, radius, 0x7faaddaa, 1, false, -1);
            stripSp++;
        }
    }

    if (flags & 0x40) {
        unsigned int colour = 0x7f00ff00;

        if (inst.fInvPosRadius.w > 0.01f) {
            unsigned int densityFactor = static_cast<unsigned int>(static_cast<float>(ca.TotalSize()) * 4.0f / inst.fInvPosRadius.w);
            unsigned int g;
            unsigned int r;

            if (densityFactor > 511) {
                densityFactor = 511;
            }
            r = static_cast<unsigned int>(WWorldMath::wmin(static_cast<float>(densityFactor), 255.0f));
            g = 255 - static_cast<unsigned int>(WWorldMath::wmax(static_cast<float>(densityFactor) - 255.0f, 0.0f));
            colour += (g << 8) + r;
        }

        {
            const WCollisionStripSphere *stripSp = ca.GetStripSphere(0);
            const WCollisionStrip *strip = ca.GetStrip();

            for (int i = 0; i < ca.fNumStrips; i++) {
                UMath::Vector3 pos;

                MATRIX4_vect3mult3(&stripSp->fPos, &mat, &pos);
                CollisionTriStrip(ca, *stripSp, *strip, mat);
                stripSp++;
                strip = strip->NextStrip();
            }
        }

        if (inst.fGroupNumber == 0 || IsSceneryGroupEnabled(inst.fGroupNumber)) {
            const WCollisionBarrier *barrier = ca.GetBarrier(0);

            for (int i = 0; i < ca.fNumEdges; i++) {
                CollisionBarrier(*barrier, mat, colour);
                barrier = barrier->Next();
            }
        }
    }

    if (flags & 2) {
        if (inst.fGroupNumber == 0 || IsSceneryGroupEnabled(inst.fGroupNumber)) {
            const WCollisionBarrier *barrier = ca.GetBarrier(0);

            for (int i = 0; i < ca.fNumEdges; i++) {
                if ((Tweak_typeMaskInclude == 0 || barrier->GetWSurface().HasFlag(Tweak_typeMaskInclude)) &&
                    !barrier->GetWSurface().HasFlag(Tweak_typeMaskExclude)) {
                    CollisionBarrier(*barrier, mat, 0x7f0088ff);
                }
                barrier = barrier->Next();
            }
        }
    }
}

void WDbgCollisionDraw::CollisionNodeList(UTL::Vector<unsigned int, 16> &nodeIndList, unsigned int flags) {
    for (const unsigned int *nIter = nodeIndList.begin(); nIter != nodeIndList.end(); ++nIter) {
        WDbgCollisionDraw::Get().CollisionNode(*nIter, 0x7f888888, flags);
    }
}
