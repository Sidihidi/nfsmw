#ifndef WORLD_WSIMPLEZONE_H
#define WORLD_WSIMPLEZONE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/UMath.h"

// total size: 0xC
struct WSimpleZone {
    WSimpleZone(int x, int y, int z);
    WSimpleZone(const UMath::Vector3 &Pos);
    ~WSimpleZone();

    void SetPosition(const UMath::Vector3 &Pos);
    bool IsZoneInRange(const WSimpleZone &Zone, int RangeBits);
    bool IsPosInRange(const UMath::Vector3 &Pos, int RangeBits);

    int fZoneX; // offset 0x0, size 0x4
    int fZoneY; // offset 0x4, size 0x4
    int fZoneZ; // offset 0x8, size 0x4
};

#endif
