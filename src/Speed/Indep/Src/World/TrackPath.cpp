#include "Speed/Indep/Src/World/TrackPath.hpp"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Misc/SpeedChunks.hpp"
#include "Scenery.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

// gcdatos2: inline en el original (DWARF), sin copia fuera de linea ni referencias a las
// cadenas en el .text de zTrack: existen porque SN interna los literales al parsear el
// cuerpo. El orden de casos es el orden 0..14 del enum, que es el de la .rodata objetivo.
inline char *GetTrackPathZoneName(eTrackPathZoneType zone_type) {
    switch (zone_type) {
        case TRACK_PATH_ZONE_RESET:
            return "RESET";
        case TRACK_PATH_ZONE_RESET_TO_POINT:
            return "RESET_TO_POINT";
        case TRACK_PATH_ZONE_GUIDED_RESET:
            return "GUIDED_RESET";
        case TRACK_PATH_ZONE_TUNNEL:
            return "TUNNEL";
        case TRACK_PATH_ZONE_OVERPASS:
            return "OVERPASS";
        case TRACK_PATH_ZONE_OVERPASS_SMALL:
            return "OVERPASS_SMALL";
        case TRACK_PATH_ZONE_STREAMER_PREDICTION:
            return "STREAMER_PREDICTION";
        case TRACK_PATH_ZONE_GARAGE:
            return "GARAGE";
        case TRACK_PATH_ZONE_HIDDEN:
            return "HIDDEN";
        case TRACK_PATH_ZONE_TRAFFIC_PATTERN:
            return "TRAFFIC_PATTERN";
        case TRACK_PATH_ZONE_DYNAMIC:
            return "DYNAMIC";
        case TRACK_PATH_ZONE_NEIGHBOURHOOD:
            return "NEIGHBOURHOOD";
        case TRACK_PATH_ZONE_JUMP_CAM:
            return "JUMP_CAM";
        case TRACK_PACH_ZONE_NO_COP_SPAWN:
            return "NO_COP_SPAWN";
        case TRACK_PATH_ZONE_PURSUIT_START:
            return "PURSUIT_START";
        default:
            return "RESET";
    }
}

TrackPathZone *zoneB[2] = {};
#ifdef EA_BUILD_A124
// gcdatos13: la tabla que el switch de arriba sustituye en retail; entera solo en la
// A124 (0x62f8d8, 60 B, PS2_globals.hpp:5576), con las mismas cadenas en el orden del
// enum, leidas de su .data/.rodata objetivo. En GC queda solo el puntero [14] cadaver
// (gcdatos9) y en SLUS/X360 no hay simbolo. En PC si existe (symbols.json, addr 9445608).
char *TrackPathZoneTypeNames[15] = {"RESET", "RESET_TO_POINT", "GUIDED_RESET", "TUNNEL",
                                    "OVERPASS", "OVERPASS_SMALL", "STREAMER_PREDICTION", "GARAGE",
                                    "HIDDEN", "TRAFFIC_PATTERN", "DYNAMIC", "NEIGHBOURHOOD",
                                    "JUMP_CAM", "NO_COP_SPAWN", "PURSUIT_START"};
#endif
TrackPathManager TheTrackPathManager;
bChunkLoader bChunkLoaderTrackPath(BCHUNK_TRACK_PATH_MANAGER, LoaderTrackPath, UnloaderTrackPath);
bChunkLoader bChunkLoaderTrackPathBarriers(BCHUNK_TTRACK_PATH_BARRIERS, LoaderTrackPath, UnloaderTrackPath);

bool DoLinesIntersect(const bVector2 &a, const bVector2 &b, const bVector2 &c, const bVector2 &d) {
    float den = (b.x - a.x) * (d.y - c.y) - (b.y - a.y) * (d.x - c.x);

    if (den == 0.0f) {
        return false;
    }

    float r = ((a.y - c.y) * (d.x - c.x) - (a.x - c.x) * (d.y - c.y)) / den;

    if (r < 0.0f || r > 1.0f) {
        return false;
    }

    float s = ((a.y - c.y) * (b.x - a.x) - (a.x - c.x) * (b.y - a.y)) / den;

    if (s < 0.0f || s > 1.0f) {
        return false;
    }

    return true;
}

void TrackPathManager::Clear() {
    this->NumZones = 0;
    this->SizeofZones = 0;
    this->pZones = nullptr;
    bMemSet(this->ZoneInfoTable, 0, sizeof(this->ZoneInfoTable));
    this->MostCachedZones = 0;
    this->pBarriers = nullptr;
    this->NumBarriers = 0;
#ifndef EA_BUILD_A124
    zoneB[0] = nullptr;
    zoneB[1] = nullptr;
#endif
}

int TrackPathManager::Loader(bChunk *chunk) {
    if (chunk->GetID() == BCHUNK_TRACK_PATH_MANAGER) {
        bChunk *last_chunk = chunk->GetLastChunk();

        for (chunk = chunk->GetFirstChunk(); chunk != last_chunk; chunk = chunk->GetNext()) {
            if (chunk->GetID() == BCHUNK_TTRACK_PATH_ZONES) {
                this->pZones = reinterpret_cast<TrackPathZone *>(chunk->GetData());
                this->SizeofZones = chunk->GetSize();
                this->NumZones = 0;

                for (TrackPathZone *zone = this->pZones; zone < this->GetLastZone(); zone = zone->GetMemoryImageNext()) {
                    bPlatEndianSwap(reinterpret_cast<int32 *>(&zone->Type));
                    bPlatEndianSwap(&zone->Position);
                    bPlatEndianSwap(&zone->Direction);
                    bPlatEndianSwap(&zone->Elevation);
                    bPlatEndianSwap(&zone->VisitInfo);
                    bPlatEndianSwap(&zone->NumPoints);
                    bPlatEndianSwap(&zone->MemoryImageSize);
                    bPlatEndianSwap(&zone->BBoxMin);
                    bPlatEndianSwap(&zone->BBoxMax);

                    int n;
                    for (n = 0; n < zone->NumPoints; n++) {
                        bPlatEndianSwap(&zone->Points[n]);
                    }
                    for (n = 0; n < 4; n++) {
                        bPlatEndianSwap(&zone->Data[n]);
                    }
                    this->NumZones++;
                }
            }
        }
        this->BuildZoneInfoTable();
        return 1;
    }

    if (chunk->GetID() == BCHUNK_TTRACK_PATH_BARRIERS) {
        this->pBarriers = reinterpret_cast<TrackPathBarrier *>(chunk->GetData());
        this->NumBarriers = chunk->GetSize() / sizeof(*this->pBarriers);
        for (int i = 0; i < this->NumBarriers; i++) {
            this->pBarriers[i].EndianSwap();
        }
        return 1;
    }

    return 0;
}

int TrackPathManager::Unloader(bChunk *chunk) {
    if (chunk->GetID() == BCHUNK_TRACK_PATH_MANAGER) {
        this->Clear();
        void NotifyGameZonesChanged();
        NotifyGameZonesChanged();
        return 1;
    }

    if (chunk->GetID() == BCHUNK_TTRACK_PATH_BARRIERS) {
        this->pBarriers = nullptr;
        this->NumBarriers = 0;
        return 1;
    }

    return 0;
}

void TrackPathManager::DisableAllBarriers() {
    for (int n = 0; n < this->NumBarriers; n++) {
        TrackPathBarrier *barrier = this->GetBarrier(n);
        barrier->Enabled = 0;
    }
}

void TrackPathManager::EnableBarriers(const char *group_name) {
    unsigned int group_name_hash = bStringHash(group_name);
    for (int n = 0; n < this->NumBarriers; n++) {
        TrackPathBarrier *barrier = this->GetBarrier(n);
        if (barrier->HasGroup(group_name_hash)) {
            barrier->Enabled = 1;

            SceneryGroup *scenery_group = FindSceneryGroup(group_name_hash);
            barrier->PlayerBarrier = static_cast<int8>((scenery_group != nullptr) && (scenery_group->DriveThroughBarrierFlag != 0));
        }
    }
}

void TrackPathManager::BuildZoneInfoTable() {
    int type;
    for (type = 0; type < NUM_TRACK_PATH_ZONES; type++) {
        ZoneInfo *zone_info = &this->ZoneInfoTable[type];
        zone_info->NumZones = 0;

        TrackPathZone *zone;
        for (zone = this->pZones; zone < this->GetLastZone(); zone = zone->GetMemoryImageNext()) {
            if (zone->GetType() == type) {
                if (zone_info->NumZones == 0) {
                    zone_info->pFirstZone = zone;
                }
                zone_info->pLastZone = zone->GetMemoryImageNext();
                zone_info->NumZones++;
            }
        }
    }

    NotifyGameZonesChanged();
}

TrackPathZone *TrackPathManager::FindZone(const bVector2 *position, eTrackPathZoneType zone_type, TrackPathZone *prev_zone) {
    ZoneInfo *zone_info = &this->ZoneInfoTable[zone_type];
    bool cache_valid;

    if (position == nullptr) {
        cache_valid = false;
    } else if (bBoundingBoxIsInside(&zone_info->CachedBBoxMin, &zone_info->CachedBBoxMax, position, 0.0f)) {
        cache_valid = zone_info->NumCachedZones <= 8;
    } else {
        const float cached_radius = 64.0f;
        TrackPathZone *first_zone = zone_info->pFirstZone;
        TrackPathZone *last_zone = zone_info->pLastZone;

        zone_info->CachedBBoxMin.x = position->x - cached_radius;
        zone_info->CachedBBoxMin.y = position->y - cached_radius;
        zone_info->CachedBBoxMax.x = position->x + cached_radius;
        zone_info->CachedBBoxMax.y = position->y + cached_radius;
        zone_info->NumCachedZones = 0;
        zone_info->NumCacheRebuilds++;

        for (TrackPathZone *zone = zone_info->pFirstZone; zone < last_zone; zone = zone->GetMemoryImageNext()) {
            if (bBoundingBoxOverlapping(&zone_info->CachedBBoxMin, &zone_info->CachedBBoxMax, &zone->BBoxMin, &zone->BBoxMax)) {
                if (zone_info->NumCachedZones < 8) {
                    zone->CachedIndex = static_cast<char>(zone_info->NumCachedZones);
                    zone_info->CachedZones[zone_info->NumCachedZones] = zone;
                }
                zone_info->NumCachedZones++;
            }
        }

        cache_valid = zone_info->NumCachedZones <= 8;
        this->MostCachedZones = bMax(this->MostCachedZones, zone_info->NumCachedZones);
    }

    TrackPathZone *found_zone = nullptr;
    if (!cache_valid) {
        zone_info->NumFullRebuilds++;

        TrackPathZone *first_zone = zone_info->pFirstZone;
        TrackPathZone *last_zone = zone_info->pLastZone;

        if (prev_zone != nullptr) {
            first_zone = prev_zone->GetMemoryImageNext();
        }

        for (TrackPathZone *zone = first_zone; zone < last_zone; zone = zone->GetMemoryImageNext()) {
            if (position == nullptr) {
                found_zone = zone;
                break;
            }

            if (bBoundingBoxIsInside(&zone->BBoxMin, &zone->BBoxMax, position, 0.0f)) {
                if (zone->IsPointInside(position)) {
                    found_zone = zone;
                    break;
                }
            }
        }
    } else {
        zone_info->NumCacheHits++;

        int first_zone_index = 0;
        if (prev_zone != nullptr) {
            first_zone_index = prev_zone->CachedIndex + 1;
        }

        for (int index = first_zone_index; index < zone_info->NumCachedZones; index++) {
            TrackPathZone *zone = zone_info->CachedZones[index];
            if (bBoundingBoxIsInside(&zone->BBoxMin, &zone->BBoxMax, position, 0.0f)) {
                if (zone->IsPointInside(position)) {
                    found_zone = zone;
                    break;
                }
            }
        }
    }

    return found_zone;
}

void TrackPathManager::ResetZoneVisitInfos() {
    for (TrackPathZone *zone = this->pZones; zone < this->GetLastZone(); zone = zone->GetMemoryImageNext()) {
        zone->SetVisitInfo(0);
    }
}

// gcdatos5: sin llamantes en ninguna version (DIE de GC .debug+0x3F849F3 con low_pc=-1;
// la A124 la trae igual, 132 B de codigo PS2 eliminado por el enlazador): existen la
// declaracion (TrackPath.hpp:93), su DIE (unico local `n` en un bloque, 0 inlines, this/
// point/n en registros callee-saved r30/r29/r31: hay una llamada por iteracion) y su
// literal en el pool de zTrack (obj+0x450: 0.001f, el unico). La llamada que encaja en
// las tres cosas es bEqual (externa, el mismo `bEqual(..., 0.001f)` de TrackStreamer.cpp).
bool TrackPathZone::IsPointOnBoundary(const bVector2 *point) {
    for (int n = 0; n < NumPoints; n++) {
        if (bEqual(&Points[n], point, 0.001f)) {
            return true;
        }
    }
    return false;
}

#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_GAMECUBE)
// Viva solo en las PS2; en GC la CU la parsea y emite igual (DIE .debug+0x3F84ABF entre
// IsPointOnBoundary y GetIntercept, low_pc=-1: codigo eliminado por el enlazador, su
// 0.0f queda en el pool en obj+0x454) y en PC no hay evidencia de que se parseara.
// gcdatos5: la guarda pasa a incluir GC por eso. El segmento mas paralelo a in0-in1.
void TrackPathZone::GetOpposite(bVector2 *in0, bVector2 *in1, bVector2 *opp0, bVector2 *opp1) {
    float cosMax = 0.0f;
    int pointIndex = 0;
    bVector2 original = *in0 - *in1;

    bNormalize(&original, &original);
    for (int n = 0; n < NumPoints; n++) {
        bVector2 *p0 = &Points[n % NumPoints];
        bVector2 *p1 = &Points[(n + 1) % NumPoints];

        if ((in0->x == p0->x && in0->y == p0->y && in1->x == p1->x && in1->y == p1->y) ||
            (in1->x == p0->x && in1->y == p0->y && in0->x == p1->x && in0->y == p1->y)) {
            continue;
        }

        bVector2 compare = *p0 - *p1;

        bNormalize(&compare, &compare);
        float cosAngle = bAbs(bDot(&compare, &original));

        if (cosMax < cosAngle) {
            cosMax = cosAngle;
            pointIndex = n;
        }
    }
    *opp0 = Points[pointIndex % NumPoints];
    *opp1 = Points[(pointIndex + 1) % NumPoints];
}
#endif

// gcdatos5: sin llamantes en ninguna version (DIE de GC .debug+0x3F85429 con low_pc=-1,
// subarbol de 8715 B; la A124 la trae igual, 928 B de codigo PS2 eliminado por el
// enlazador): existen la declaracion (TrackPath.hpp:96), su DIE (locales
// TempInterceptPoint/P1/V1/P2/V2/t2/Numer0/Denom0/InterceptOccured en la funcion, `n`
// en el bloque del bucle y TmpVec1/TmpVec2 dentro de el, con su secuencia de inlines
// =, bNormalize= | =, bSub+bNormalize=, bDot, bSub+bNormalize+==, =, bScale++=,
// -=, -=, bDot, bDistBetween x2, =) y sus literales en el pool de zTrack
// (obj+0x458..0x468: 1.0f, 0.001f, -0.001f, 0.0f, y el 5e-11f del bSqrt de
// bDistBetween). El cuerpo sigue ese esqueleto y ese orden de primera aparicion de
// constantes; la aritmetica exacta no esta en la evidencia y la funcion no emite en
// ningun retail. El `==` de vectores usa el bVector2::operator== inline original, que
// nuestro bMath.hpp solo declara (pedido al coordinador, ver gcdatos5.md).
bool TrackPathZone::GetIntercept(bVector2 &InterceptPoint, const bVector2 *Start, const bVector2 *Direction) {
    bVector2 TempInterceptPoint;
    bVector2 P1;
    bVector2 V1;
    bVector2 P2;
    bVector2 V2;
    float t2;
    float Numer0;
    float Denom0;
    bool InterceptOccured = false;

    // Los huecos de pila del DIE (P2 en r1+0x8 y V2 en r1+0x18, delante de P1/V1) dicen
    // que P2/V2 son el rayo (Start/Direction) y P1/V1 el segmento de cada iteracion.
    P2 = *Start;
    V2 = bNormalize(*Direction);

    for (int n = 0; n < NumPoints; n++) {
        P1 = Points[n];
        V1 = bNormalize(bSub(Points[(n + 1) % NumPoints], P1));
        Denom0 = bDot(V1, V2);
        Denom0 = 1.0f - Denom0 * Denom0;

        if (Denom0 < 0.001f && Denom0 > -0.001f) {
            continue;
        }

        Numer0 = (P1.x - P2.x) * V1.y - (P1.y - P2.y) * V1.x;
        t2 = Numer0 / (V2.x * V1.y - V2.y * V1.x);

        if (t2 < 0.0f || bNormalize(bSub(P1, P2)) == V2) {
            continue;
        }

        InterceptPoint = *Start;
        TempInterceptPoint = P2 + bScale(V2, t2);

        bVector2 TmpVec1;
        bVector2 TmpVec2;

        TmpVec1 = Points[n] - TempInterceptPoint;
        TmpVec2 = Points[(n + 1) % NumPoints] - TempInterceptPoint;

        if (bDot(TmpVec1, TmpVec2) < 0.0f) {
            if (!InterceptOccured || bDistBetween(InterceptPoint, *Start) > bDistBetween(TempInterceptPoint, *Start)) {
                InterceptPoint = TempInterceptPoint;
                InterceptOccured = true;
            }
        }
    }

    return InterceptOccured;
}

bool TrackPathZone::IsPointInside(const bVector2 *point) {
    return bIsPointInPoly(point, this->Points, this->NumPoints);
}

void TrackPathInitRemoteCaffeineConnection() {}

int LoaderTrackPath(bChunk *chunk) {
    return TheTrackPathManager.Loader(chunk);
}

int UnloaderTrackPath(bChunk *chunk) {
    return TheTrackPathManager.Unloader(chunk);
}
float TrackPathZone::GetSegmentNextTo(bVector2 *point, bVector2 *s0, bVector2 *s1) {
    int Closest0 = -1;
    int Closest1 = -1;
    float d0 = 99999.0f;
    float len = -1.0f;

    for (int n = 0; n < this->NumPoints; n++) {
        bVector2 *p0 = &this->Points[n % this->NumPoints];
        bVector2 *p1 = &this->Points[(n + 1) % this->NumPoints];
        bVector2 r = *p0 - *point;
#ifdef EA_BUILD_A124
        bVector2 v(p0->y - p1->y, p1->x - p0->x); // la A124 toma la normal del otro lado
#else
        bVector2 v(p1->y - p0->y, p0->x - p1->x);
#endif

        bNormalize(&v, &v);
        len = bDot(&v, &r);
        bVector2 InPoint = v * (len * -1.1f) + *point;
        bVector2 InPoint2 = v * (len * 1.1f) + *point;
        len = bAbs(len);

        if (len < d0 && (this->IsPointInside(&InPoint) || this->IsPointInside(&InPoint2))) {
            Closest0 = n % this->NumPoints;
            Closest1 = (n + 1) % this->NumPoints;
            d0 = len;
        }
    }

    if (Closest0 == -1 || Closest1 == -1) {
        return -1.0f;
    }

    *s0 = this->Points[Closest0];
    *s1 = this->Points[Closest1];
#ifdef EA_BUILD_A124
    return len; // la A124 devuelve la ultima distancia calculada, no la del segmento elegido
#else
    return d0;
#endif
}
