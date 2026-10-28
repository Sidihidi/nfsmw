#include "WeatherMan.hpp"

#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Misc/SpeedChunks.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

int RegionCount[NUM_REGION_TYPES] = {};
bTList<GenericRegion> RegionLists[NUM_REGION_TYPES];
int WeatherUseCoolFogValues = 0;
float BaseFogFalloff = 0.62f;
float BaseFogFalloffX = 0.31f;
float BaseFogFalloffY = 1.3f;
float BaseWeatherFog = 0.74f;
float BaseWeatherFogStart = 212.0f;
int BaseWeatherFogColourR = 0x4B;
int BaseWeatherFogColourG = 0x69;
int BaseWeatherFogColourB = 0x6E;
bVector3 cPos;
// gcdatos8 (receta r68): la pareja muerta de los Base*, en la .data objetivo de
// zTrack tras BaseWeatherFogColourB (lbl_804370EC) con estos valores; en el
// DWARF de GC no tienen direccion (eliminadas del ELF) pero la A124 las trae
// en 0x638510-0x638524 y PC las nombra en su PDB.
float HorizFogFalloff = 0.62f;
float HorizFogFalloffX = 0.0f;
float HorizFogFalloffY = 1.0f;
float HorizWeatherFog = 0.58f;
float HorizWeatherFogStart = 229.0f;

// TODO move
extern int FogControlOverRide;

// gcdatos7: vacias en el original (DIEs .debug+0x40D7236/0x40D726F low_pc=-1 antes
// de LoaderWeatherMan; la A124 las trae igual, 8 B y una linea cada una).
void WeatherManInit() {}

void WeatherManClose() {}

int LoaderWeatherMan(bChunk *bchunk) {
    if (bchunk->GetID() != BCHUNK_GENERIC_REGIONS) {
        return 0;
    }

    WeatherRegionPack *region_pack = reinterpret_cast<WeatherRegionPack *>(bchunk->GetAlignedData(16));
    bPlatEndianSwap(&region_pack->Version);
    bPlatEndianSwap(&region_pack->NumRegions);
    if (region_pack->Version == 2) {
        GenericRegion *region = reinterpret_cast<GenericRegion *>(&region_pack[1]);
        int num_regions = region_pack->NumRegions;
        for (int i = 0; i < num_regions; i++) {
            bPlatEndianSwap(&region->NameHash);
            bPlatEndianSwap(&region->GroupNameHash);
            bPlatEndianSwap(&region->Type);
            bPlatEndianSwap(&region->FogColour);
            bPlatEndianSwap(&region->FogStart);
            bPlatEndianSwap(&region->Parameter1);
            bPlatEndianSwap(&region->PositionX);
            bPlatEndianSwap(&region->PositionY);
            bPlatEndianSwap(&region->PositionZ);
            bPlatEndianSwap(&region->Radius);
            bPlatEndianSwap(&region->Intensity);
            bPlatEndianSwap(&region->FarFalloffStart);
            bPlatEndianSwap(&region->FogFalloff);
            bPlatEndianSwap(&region->FogFalloffX);
            bPlatEndianSwap(&region->FogFalloffY);
            bPlatEndianSwap(&region->Shape);
            bPlatEndianSwap(&region->Blend);
            bPlatEndianSwap(&region->modifier);

            AddRegion(region);
            region++;
        }
    }

    return 1;
}

int UnloaderWeatherMan(bChunk *bchunk) {
    if (bchunk->GetID() != BCHUNK_GENERIC_REGIONS) {
        return 0;
    }

    WeatherRegionPack *region_pack = reinterpret_cast<WeatherRegionPack *>(bchunk->GetAlignedData(16));
    if (region_pack->Version == 2) {
        GenericRegion *region = reinterpret_cast<GenericRegion *>(&region_pack[1]);
        int num_regions = region_pack->NumRegions;
        for (int i = 0; i < num_regions; i++) {
            RemoveRegion(region);
            region++;
        }
    }

    return 1;
}

void AddRegion(GenericRegion *region) {
    RegionType type = region->GetType();
    if (type == REGION_RAIN && region->Intensity == 0.0f) {
        region->SetType(REGION_TUNNEL);
        type = REGION_TUNNEL;
    }

    // TODO why is that cast needed?
    if ((unsigned int)type < NUM_REGION_TYPES) {
        RegionLists[type].AddTail(region);
        RegionCount[type]++;
    }
}

// gcdatos7: sin llamantes (DIE .debug+0x40D9014 low_pc=-1, entre AddRegion y
// RemoveRegion; A124 260 B). Unico local region e inlines ctor+SetType: crea la
// region, le pasa los 11 parametros a sus campos homonimos y la registra. El
// orden de las asignaciones no esta en la evidencia.
GenericRegion *BuildWeatherRegion(RegionType type, bVector3 *position, float Intensity, unsigned int colour, float radius, float falloffstart,
                                  float FogFalloff, float FogFalloffX, float FogFalloffY, float fogstart, float modifier) {
    GenericRegion *region = new GenericRegion();
    region->SetType(type);
    region->PositionX = position->x;
    region->PositionY = position->y;
    region->PositionZ = position->z;
    region->Intensity = Intensity;
    region->FogColour = colour;
    region->Radius = radius;
    region->FarFalloffStart = falloffstart;
    region->FogFalloff = FogFalloff;
    region->FogFalloffX = FogFalloffX;
    region->FogFalloffY = FogFalloffY;
    region->FogStart = fogstart;
    region->modifier = modifier;
    AddRegion(region);
    return region;
}

void RemoveRegion(GenericRegion *region) {
    region->Remove();
}

// gcdatos7: sin llamantes (DIE .debug+0x40D9639 low_pc=-1, tras RemoveRegion;
// A124 136 B). Locales region_list/px/py/pz y bloque {region {radius2, dx, dy,
// dz, distance2 + bAbs x3}}: primera region del tipo cuyo radio alcanza el
// punto, como la prueba de emTriggerEventsInSection. El operador exacto de la
// comparacion no esta en la evidencia.
GenericRegion *GenericRegionQuery(bVector3 *position, RegionType Rtype) {
    bTList<GenericRegion> *region_list = &RegionLists[Rtype];
    float px = position->x;
    float py = position->y;
    float pz = position->z;

    for (GenericRegion *region = region_list->GetHead(); region != region_list->EndOfList(); region = region->GetNext()) {
        float radius2 = region->Radius * region->Radius;
        float dx = bAbs(px - region->PositionX);
        float dy = bAbs(py - region->PositionY);
        float dz = bAbs(pz - region->PositionZ);
        float distance2 = dx * dx + dy * dy + dz * dz;
        if (distance2 < radius2) {
            return region;
        }
    }

    return nullptr;
}

// gcdatos8: sin llamantes (DIE .debug+0x40D9ADD low_pc=-1, entre
// GenericRegionQuery y CalculateHorizonInfo; A124 104 B en 4 lineas 93-96).
// Locales radius2/dx/dy/dz/distance2 con bAbs x3: la misma prueba de alcance
// que el bloque de GenericRegionQuery. El operador exacto de la comparacion
// no esta en la evidencia.
unsigned int InRegion(bVector3 *position, GenericRegion *region) {
    float radius2 = region->Radius * region->Radius;
    float dx = bAbs(position->x - region->PositionX);
    float dy = bAbs(position->y - region->PositionY);
    float dz = bAbs(position->z - region->PositionZ);
    float distance2 = dx * dx + dy * dy + dz * dz;
    return distance2 < radius2;
}

int DepthRegion(GenericRegion *before, GenericRegion *after) {
    bVector3 Position(before->PositionX, before->PositionY, before->PositionZ);
    bVector3 Delta = Position - cPos;

    float distB = bLength(Delta);
    Position = bVector3(after->PositionX, after->PositionY, after->PositionZ);
    Delta = Position - cPos;

    float distA = bLength(Delta);
    return distB <= distA;
}

int RegionQuery::CalculateRegionInfo(eView *view, RegionType regionKind, int InFE) {
    static unsigned int oldDistFogColour = 0x000003E7;
    static float oldDistFogPower = 999.0f;
    static float oldDistFogStart = 999.0f;

    unsigned int colr_r = 0;
    unsigned int colr_g = 0;
    unsigned int colr_b = 0;
    bVector3 cPos(*view->GetCamera()->GetPosition());

    if (FogControlOverRide) {
        FogFalloff = BaseFogFalloff;
        FogFalloffX = BaseFogFalloffX;
        FogFalloffY = BaseFogFalloffY;
        DistFogStart = BaseWeatherFogStart;
        DistFogPower = BaseWeatherFog;

        unsigned int fog_colour = (BaseWeatherFogColourB << 16) | (BaseWeatherFogColourG << 8) | BaseWeatherFogColourR;
        colr_r = fog_colour & 0xFF;
        colr_g = (fog_colour >> 8) & 0xFF;
        colr_b = (fog_colour >> 16) & 0xFF;

        DistFogStart = BaseWeatherFogStart; // repetido, como en rain.cpp: el mapa de lineas lo pide (545)

        unsigned int retcol = colr_r | (colr_g << 8) | (colr_b << 16) | 0x80000000;
        DistFogColour = retcol;

        if (oldDistFogColour == retcol && oldDistFogPower == DistFogPower && oldDistFogStart == DistFogStart) {
            return 0;
        }

        oldDistFogColour = DistFogColour;
        oldDistFogPower = DistFogPower;
        oldDistFogStart = DistFogStart;
        return 1;
    }

    DistFogStart = 0.0f;
    DistFogPower = 0.0f;
    FogFalloff = 0.0f;
    FogFalloffX = 0.0f;
    FogFalloffY = 0.0f;
    float smallest = 9999999.0f;
    bTList<GenericRegion> *region_list = &RegionLists[regionKind];

    for (GenericRegion *region = region_list->GetHead(); region != region_list->EndOfList(); region = region->GetNext()) {
        bVector4 direction;
        direction.x = region->PositionX - cPos.x;
        direction.y = region->PositionY - cPos.y;
        float distanceSq = direction.x * direction.x + direction.y * direction.y;

        if (distanceSq < region->Radius * region->Radius) {
            float distance = bSqrt(distanceSq);

            float ratio;
            if (distance >= region->FarFalloffStart) {
                ratio = (distance - region->FarFalloffStart) / (region->Radius - region->FarFalloffStart);
            } else {
                ratio = 0.0f;
            }

            region->effect = 1.0f - ratio;
            if (ratio == 0.0f) {
                region->inFlags = 1;
                if (region->Radius < smallest) {
                    smallest = region->Radius;
                }
            } else {
                region->inFlags = 2;
            }
        } else {
            region->inFlags = 4;
            region->effect = 0.0f;
        }
    }

    float totaleffex = 0.0f;
    for (GenericRegion *region = region_list->GetHead(); region != region_list->EndOfList(); region = region->GetNext()) {
        region->effect = region->effect * region->modifier;
        totaleffex += region->effect;
    }

    for (GenericRegion *region = region_list->GetHead(); region != region_list->EndOfList(); region = region->GetNext()) {
        if (totaleffex == 0.0f) {
            region->effect = 0.0f;
        } else {
            region->effect = region->effect / totaleffex;
        }

        if (region->effect != 0.0f) {
            FogFalloff += region->FogFalloff * region->effect;
            FogFalloffX += region->FogFalloffX * region->effect;
            FogFalloffY += region->FogFalloffY * region->effect;
            DistFogStart += region->FogStart * region->effect;
            DistFogPower += region->Intensity * region->effect;

            unsigned int tcolr_r = region->FogColour & 0xFF;
            unsigned int tcolr_g = (region->FogColour & 0xFF00) >> 8;
            unsigned int tcolr_b = (region->FogColour & 0xFF0000) >> 16;

            colr_r += static_cast<unsigned int>(tcolr_r * region->effect);
            colr_g += static_cast<unsigned int>(tcolr_g * region->effect);
            colr_b += static_cast<unsigned int>(tcolr_b * region->effect);
        }
    }

    unsigned int retcol = colr_r | (colr_g << 8) | (colr_b << 16) | 0x80000000;
    DistFogColour = retcol;
    if (oldDistFogColour == retcol && oldDistFogPower == DistFogPower && oldDistFogStart == DistFogStart) {
        return 0;
    }

    oldDistFogColour = DistFogColour;
    oldDistFogPower = DistFogPower;
    oldDistFogStart = DistFogStart;
    return 1;
}

// gcdatos7: tambien en GameCube. La .data objetivo de zTrack GC trae los dos
// floats (3FE66666/3EE66666 en obj+0x11C4) y el DIE de getHorizonColour existe
// en la CU de GC (low_pc=-1): la guarda #ifndef EA_PLATFORM_GAMECUBE que los
// rodeaba a los tres no es del original. gcdatos8: vivian junto a
// getHorizonColour, no con los Base*: en la .data objetivo van tras los
// oldDistFog* de CalculateRegionInfo (0x8043710C/10).
float distColAff = 1.8f;
float distfall = 0.45f;

// La distancia aproximada en pantalla (la mayor de las dos mas un cuarto de la menor), en las lineas 742-751 del
// WeatherMan.cpp de la A124. En el DWARF (GC y PS2) se llama qHdis.
static inline int ApproxScreenDistance(int x0, int y0, int x1, int y1) {
    int dx = bAbs(x0 - x1);
    int dy = bAbs(y0 - y1);

    if (dx > dy) {
        return dx + (dy >> 2);
    }
    return dy + (dx >> 2);
}

// El color del horizonte en un punto de la pantalla, sumando las regiones de color de horizonte cercanas. Sin llamantes
// en ninguna version; en GC su DIE (.debug+0x40DCF3C) trae low_pc=-1 y su pool (128.0f, 1/128, 0.5f, 255.0f, 1/384)
// esta en la .rodata objetivo de zTrack (obj+0xCA0..0xCE4): GameCube si la parseaba.
void getHorizonColour(eView *view, int x, int y, uint32 *r, uint32 *g, uint32 *b, uint32 *a) {
    bVector3 cPos = *view->GetCamera()->GetPosition();
    float rf = 0.0f;
    float gf = 0.0f;
    float bf = 0.0f;
    bTList<GenericRegion> *region_list = &RegionLists[REGION_HORIZON_COLOUR];
    float lasteffect = 0.0f;
    float Amod;

    for (GenericRegion *region = region_list->GetHead(); region != region_list->EndOfList(); region = region->GetNext()) {
        if (region->modifier > 0.0f) {
            float colr_r = static_cast<float>(region->FogColour & 0xff);
            float colr_g = static_cast<float>((region->FogColour >> 8) & 0xff);
            float colr_b = static_cast<float>((region->FogColour >> 16) & 0xff);
            float dist = static_cast<float>(
                ApproxScreenDistance(x, y, bFloatToInt(region->ScreenPosX), bFloatToInt(region->ScreenPosY)));
            float effect = (128.0f - dist * distfall) * (1.0f / 128.0f);

            effect *= region->modifier;
            if (effect < 0.0f) {
                continue;
            }

            Amod = 1.0f - lasteffect;

            lasteffect += effect;
            if (Amod < 0.0f) {
                continue;
            }
            rf += effect * (Amod * (colr_r * 0.5f));
            gf += effect * (Amod * (colr_g * 0.5f));
            bf += effect * (Amod * (colr_b * 0.5f));
        }
    }

    if (lasteffect > 0.0f) {
        rf *= distColAff;
        gf *= distColAff;
        bf *= distColAff;
        if (rf > 255.0f) {
            rf = 255.0f;
        }
        if (gf > 255.0f) {
            gf = 255.0f;
        }
        if (bf > 255.0f) {
            bf = 255.0f;
        }
    }

    *r = static_cast<uint32>(rf);
    *g = static_cast<uint32>(gf);
    *b = static_cast<uint32>(bf);

    float atten = static_cast<float>(*r + *g + *b) * (1.0f / 384.0f);

    if (atten > 1.0f) {
        atten = 1.0f;
    }
    *a = static_cast<uint32>(static_cast<float>(*a) * atten);
}

GenericRegion *GetClosestRegionInView(eView *view, bVector3 *endVector, float *angleCos) {
    cPos = *view->GetCamera()->GetPosition();
    bVector3 cDir(*view->GetCamera()->GetDirection());
    bTList<GenericRegion> *region_list = &RegionLists[REGION_BLOOM];
    region_list->Sort(DepthRegion);
    bVector3 posScreen;

    CameraMover *cameraMover = view->GetCameraMover();
    if (cameraMover == nullptr) {
        return nullptr;
    }

    CameraAnchor *cameraAnchor = cameraMover->GetAnchor();
    if (cameraAnchor == nullptr) {
        return nullptr;
    }

    bVector3 MyCarPos(*cameraAnchor->GetGeometryPosition());
    float maxDist = 99999.0f;
    float angleCOS = 0.0f;
    GenericRegion *ClosestRegion = nullptr;

    for (GenericRegion *region = region_list->GetHead(); region != region_list->EndOfList(); region = region->GetNext()) {
        bVector3 Position(region->PositionX, region->PositionY, region->PositionZ);
        bVector3 Delta = Position - cPos;
        bVector3 Delta2;
        bNormalize(&Delta2, &Delta);

        angleCOS = bDot(Delta2, cDir);
        if (angleCOS > 0.0f) {
            float dist = bLength(Delta);
            if (dist < maxDist) {
                *angleCos = angleCOS;
                ClosestRegion = region;
                maxDist = dist;
            }
        }
    }

    if (ClosestRegion != nullptr) {
        endVector->x = ClosestRegion->PositionX;
        endVector->y = ClosestRegion->PositionY;
        endVector->z = ClosestRegion->PositionZ;
        return ClosestRegion;
    }
    return 0;
}
