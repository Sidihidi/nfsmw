#ifndef PSX2_ECSTASY_EVIEWPLAT_H
#define PSX2_ECSTASY_EVIEWPLAT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Ecstasy/EcstasyData.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

class eView;

// A diferencia de GameCube, que las lleva dentro, en PS2 son punteros: las matrices viven en la memoria
// de trabajo del VU.
struct eViewPlatInfo {
    // total size: 0x1C
    bMatrix4 *WorldViewMatrix;       // offset 0x0, size 0x4
    bMatrix4 *WorldScreenMatrix;     // offset 0x4, size 0x4
    bMatrix4 *WorldClipGBMatrix;     // offset 0x8, size 0x4
    bMatrix4 *ViewScreenMatrix;      // offset 0xC, size 0x4
    bMatrix4 *VUClipGBScreenMatrix0; // offset 0x10, size 0x4
    bMatrix4 *VUClipGBScreenMatrix1; // offset 0x14, size 0x4
    bMatrix4 *VUClipGBScreenMatrix2; // offset 0x18, size 0x4

    bMatrix4 *GetWorldViewMatrix() {
        return WorldViewMatrix;
    }
    bMatrix4 *GetWorldScreenMatrix() {
        return WorldScreenMatrix;
    }
    bMatrix4 *GetWorldClipGBMatrix() {
        return WorldClipGBMatrix;
    }
    bMatrix4 *GetViewScreenMatrix() {
        return ViewScreenMatrix;
    }
    void SetWorldViewMatrix(bMatrix4 *m) {
        WorldViewMatrix = m;
    }
    void SetWorldScreenMatrix(bMatrix4 *m) {
        WorldScreenMatrix = m;
    }
    void SetWorldClipGBMatrix(bMatrix4 *m) {
        WorldClipGBMatrix = m;
    }
    void SetViewScreenMatrix(bMatrix4 *m) {
        ViewScreenMatrix = m;
    }
    void SetVUClipGBScreenMatrix0(bMatrix4 *m) {
        VUClipGBScreenMatrix0 = m;
    }
    void SetVUClipGBScreenMatrix1(bMatrix4 *m) {
        VUClipGBScreenMatrix1 = m;
    }
    void SetVUClipGBScreenMatrix2(bMatrix4 *m) {
        VUClipGBScreenMatrix2 = m;
    }

    void CalculateViewMatricies(eView *view, float force_near_z, float force_far_z, float force_screen_far_z);
};

extern eViewPlatInfo ViewPlatInfoTable[26]; // size: 0x2D8

eVisibleState ClassifyAABB(const bVector3 *aabb_min, const bVector3 *aabb_max, bMatrix4 *world_clip);
eVisibleState ClassifyPoint(const bVector3 *point, bMatrix4 *world_clip);

#endif
