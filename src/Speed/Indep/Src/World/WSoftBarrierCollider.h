#ifndef WORLD_WSOFTBARRIERCOLLIDER_H
#define WORLD_WSOFTBARRIERCOLLIDER_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// Solo la A124 tiene las barreras blandas: sus stabs dan este fichero (src/World/WSoftBarrierCollider.h) y
// WWorld::Open llama a Init con el UData 'SBar'. SLUS y GameCube ya no las tienen.
#ifdef EA_BUILD_A124

#include "Speed/Indep/Libs/Support/Utility/FastMem.h"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"

class UData;
class ISimable;

// total size: 0x50
struct SoftBarrier {
    UMath::Vector4 p0;    // offset 0x0, size 0x10
    UMath::Vector4 p1;    // offset 0x10, size 0x10
    UMath::Vector4 p2;    // offset 0x20, size 0x10
    UMath::Vector4 p3;    // offset 0x30, size 0x10
    UMath::Vector3 normal; // offset 0x40, size 0xC
    float softZoneSize;   // offset 0x4C, size 0x4
};

// total size: 0x50
struct WSoftBarrier : public SoftBarrier {};

// total size: 0x8
class WSoftBarrierCollider {
  public:
    USE_FASTALLOC(WSoftBarrierCollider)

    static void Init(const UData *sbUData);
    static void Shutdown();
    bool ApplyForce(ISimable *simable);
    bool PosInSoftZone(const UMath::Vector3 &pos);

  private:
    WSoftBarrierCollider(const WSoftBarrier *sb, unsigned int count);

    static WSoftBarrierCollider *fgSoftBarrierCollider;

    const WSoftBarrier *mSoftBarriers; // offset 0x0, size 0x4
    unsigned int mSoftBarrierCount;    // offset 0x4, size 0x4
};

#endif

#endif
