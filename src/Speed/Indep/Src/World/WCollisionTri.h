#ifndef WORLD_WCOLISSION_TRI_H
#define WORLD_WCOLISSION_TRI_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "WSurfaceTypes.h"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Libs/Support/Utility/UTypes.h"

// total size: 0x30
// SLUS copia el triangulo con ldl/ldr (vector<WCollisionTri>, FindClosestFace): sus vertices no van alineados; la
// alineacion a 16 la pone WWorldPos::fFace (los WWorldPos siguen en multiplos de 16).
#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124)
#define WCOLLISIONTRI_ALIGN
#else
#define WCOLLISIONTRI_ALIGN ALIGNVEC
#endif

struct WCollisionTri {
    WCollisionTri() {}

    float MinY() const {
        float minY = UMath::Min(fPt0.y, fPt1.y);
        return UMath::Min(minY, fPt2.y);
    }

    void GetNormal(UMath::Vector3 *norm) const {
        UMath::Vector3 vecX;
        UMath::Vector3 vecZ;
        UMath::Vector3 normal;

        vecZ.x = fPt1.x - fPt0.x;
        vecZ.y = fPt1.y - fPt0.y;
        vecZ.z = fPt1.z - fPt0.z;

        vecX.x = fPt0.x - fPt2.x;
        vecX.y = fPt0.y - fPt2.y;
        vecX.z = fPt0.z - fPt2.z;
        UMath::Cross(vecZ, vecX, normal);

        if (normal.x == 0.0f && normal.y == 0.0f && normal.z == 0.0f) {
            norm->x = 0.0f;
            norm->y = 1.0f;
            norm->z = 0.0f;
        } else {
#ifdef EA_PLATFORM_PLAYSTATION2
            // PS2 normaliza en linea con el VU0_rsqrt de EA (lineas 61-64 de la A124); GameCube, PC y X360 llaman a
            // v3unit. El giro hacia arriba y el tope de 0.9999 tambien son de GetNormal (lineas 72-81 de GameCube),
            // no de quien la llama.
            float invlen = VU0_rsqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            norm->x = normal.x * invlen;
            norm->y = normal.y * invlen;
            norm->z = normal.z * invlen;
#else
            v3unit(&normal, norm);
#endif
        }
        if (norm->y < 0.0f) {
            norm->x = -norm->x;
            norm->y = -norm->y;
            norm->z = -norm->z;
        }
        if (norm->y >= 0.9999f) {
            norm->y = 0.9999f;
        }
    }

    // Los tres vertices van alineados a 16 en la A124 y X360, y con ellos WCollisionTri y WWorldPos: en la A124 todo
    // miembro WWorldPos cae en un multiplo de 16 (CameraMover::mWPos en 0x30 y CarRenderInfo::mWorldPos en 0x50,
    // con 8 bytes vacios delante), y X360 construye el de CameraMover en 0x30; GameCube en 0x28.
    WCOLLISIONTRI_ALIGN UMath::Vector3 fPt0;         // offset 0x0, size 0xC
#ifdef EA_BUILD_A124
    // La A124 guarda el hash de la superficie y no el puntero ya resuelto (stabs: UCrc32 fSurfaceHash en 0xC), como
    // WCollisionBarrierListEntry; el constructor lo deja a cero.
    UCrc32 fSurfaceHash;                  // offset 0xC, size 0x4
#else
    const Attrib::Collection *fSurfaceRef; // offset 0xC, size 0x4 (DWARF de GameCube: const Collection *)
#endif
    WCOLLISIONTRI_ALIGN UMath::Vector3 fPt1;         // offset 0x10, size 0xC
    unsigned int fFlags;                  // offset 0x1C, size 0x4
    WCOLLISIONTRI_ALIGN UMath::Vector3 fPt2;         // offset 0x20, size 0xC
    WSurface fSurface;                    // offset 0x2C, size 0x2
    unsigned short PAD;                   // offset 0x2E, size 0x2
};

#endif
