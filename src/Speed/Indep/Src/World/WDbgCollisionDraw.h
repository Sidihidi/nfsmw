#ifndef WORLD_WDBGCOLLISIONDRAW_H
#define WORLD_WDBGCOLLISIONDRAW_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/UTLVector.h"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"

#include <set>

// The collision debug draw of the PS2 prototype (SLES-53558-A124): its .mdebug keeps this header (src/World) and
// WDbgCollisionDraw.cpp, with the layout of the class and the methods that the linker did not strip.

struct WCollisionTri;
struct WCollisionBarrier;
struct WCollisionStripSphere;
struct WCollisionStrip;
struct WCollisionArticle;
struct WCollisionInstance;
class WTrigger;

// total size: 0xC
class WVoidSet : public std::set<const void *> {};

// total size: 0x4
class WDbgCollisionDraw {
  public:
    static void Init() {
        fgDbgCollisionDraw = new ("WDbgCollisionDraw", 0) WDbgCollisionDraw;
    }

    static void Shutdown();

    static WDbgCollisionDraw &Get() {
        if (!fgDbgCollisionDraw) {
            Init();
        }
        return *fgDbgCollisionDraw;
    }

    void StartFrame();
    void EndFrame();
    void CollisionTri(const WCollisionTri &tri, unsigned int c);
    void CollisionTri(const WCollisionTri &tri, const UMath::Matrix4 &mat, unsigned int c);
    void CollisionTriStrip(const WCollisionArticle &cArt, const WCollisionStripSphere &sp, const WCollisionStrip &strip);
    void CollisionTriStrip(const WCollisionArticle &cArt, const WCollisionStripSphere &sp, const WCollisionStrip &strip,
                           const UMath::Matrix4 &mat);
    void CollisionBarrier(const WCollisionBarrier &barrier, unsigned int c);
    void CollisionBarrier(const WCollisionBarrier &barrier, const UMath::Matrix4 &mat, unsigned int c);
    void CollisionNode(unsigned short nodeInd, unsigned int c, unsigned int flags);
    void CollisionNodeList(UTL::Vector<unsigned int, 16> &nodeIndList, unsigned int flags);
    void CollisionInst(const WCollisionInstance &inst, unsigned int flags);
    void CollisionInstBounds(const WCollisionInstance &inst, unsigned int c);
    void CollisionArticle(const WCollisionArticle &ca, const WCollisionInstance &inst, const UMath::Matrix4 &mat,
                          unsigned int flags);
    void Trigger(const WTrigger &trig);
    bool HasBeenDrawnThisFrame(const void *test);

  private:
    WDbgCollisionDraw();

    static WDbgCollisionDraw *fgDbgCollisionDraw;

    WVoidSet *fVoidSet; // offset 0x0, size 0x4
};

#endif
