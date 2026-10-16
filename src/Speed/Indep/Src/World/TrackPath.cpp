#include "Speed/Indep/Src/World/TrackPath.hpp"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Misc/SpeedChunks.hpp"
#include "Scenery.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

TrackPathZone *zoneB[2] = {};
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

#ifdef EA_PLATFORM_PLAYSTATION2
// Solo en las PS2 (GameCube y PC no la tienen): el segmento de la zona mas paralelo a in0-in1, sin contar el mismo.
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
