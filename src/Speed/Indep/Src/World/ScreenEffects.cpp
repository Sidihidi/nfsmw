#include "ScreenEffects.hpp"

#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/World/ParameterMaps.hpp"
#include "Speed/Indep/Src/World/TrackPath.hpp"
#include "Speed/Indep/Src/World/Rain.hpp"
#include "Speed/Indep/Src/World/WCollisionMgr.h"
#include "Speed/Indep/Src/World/WWorldPos.h"
#include "Speed/Indep/Src/World/WeatherMan.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

ScreenEffectPaletteDef SE_PaletteFile[EFX_NUMBER];
#ifdef EA_BUILD_A124
// gcdatos13: solo en la A124, tras SE_PaletteFile (0x6439b4 y 0x6439b8,
// PS2_globals.hpp:5687-5688), a cero en su .data objetivo. Sin simbolo en las demas.
ScreenEffectType TestScreenEffectType;
ScreenEffectDef TestScreenEffectDef;
#endif

// TODO move these
extern eModel *pVisibleZoneBoundaryModel;
extern unsigned int FrameMallocFailed;
extern unsigned int FrameMallocFailAmount;
extern TrackPathZone *zoneB[2];
#ifdef EA_BUILD_A124
extern float DarkFalloff;
extern float DarkFallon;
extern int tunnelDark;
#endif

void InitScreenEFX();

#ifdef EA_BUILD_A124
void Unique(ScreenEffectType, ScreenEffectDB *);
#endif

ScreenEffectDB::ScreenEffectDB() {
    this->SE_time = 0.0f;
    for (int i = 0; i < SE_NUM_TYPES; i++) {
        this->SE_inf[i].active = 0;
        this->SE_data[i].r = 0.0f;
        this->SE_data[i].g = 0.0f;
        this->SE_data[i].b = 0.0f;
        this->SE_data[i].a = 0.0f;
        for (int j = 0; j < 14; j++) {
            this->SE_data[i].data[j] = 0.0f;
        }
        this->SE_data[i].intensity = 0.0f;
        this->SE_data[i].UpdateFnc = nullptr;
        this->numType[i] = 0;
    }
    InitScreenEFX();
}
void ScreenEffectDB::Update(float deltatime) {
    this->SE_time += deltatime;

    for (int i = 0; i < SE_NUM_TYPES; i++) {
        if (this->IsActive(static_cast<ScreenEffectType>(i)) == 1) {
            this->SE_inf[i].frameNum++;
            switch (this->GetController(static_cast<ScreenEffectType>(i))) {
                case SEC_FRAME:
                case SEC_FUNCTION:
                    this->RemoveScreenEffect(static_cast<ScreenEffectType>(i));
                    break;
                default:
                    break;
            }
        }
    }
}

void ScreenEffectDB::AddScreenEffect(ScreenEffectType type, float intensity, float r, float g, float b) {
    ScreenEffectDef info;

    info.intensity = intensity;
    info.r = r;
    info.g = g;
    info.b = b;
    info.UpdateFnc = nullptr;
    this->AddScreenEffect(type, &info, 1, SEC_FRAME);
}

void ScreenEffectDB::AddScreenEffect(ScreenEffectType type, ScreenEffectDef *info, unsigned int lock, ScreenEffectControl controller) {
    if (lock != 0) {
        if (info != nullptr) {
            this->SE_data[type] = *info;
        }
        this->numType[type] = 1;
    } else {
        this->numType[type]++;
        float influence = static_cast<float>(this->numType[type]) / static_cast<float>(this->numType[type] + 1);
        float invFluence = 1.0f - influence;

        this->SE_data[type].r = influence * this->SE_data[type].r + invFluence * info->r;
        this->SE_data[type].g = influence * this->SE_data[type].g + invFluence * info->g;
        this->SE_data[type].b = influence * this->SE_data[type].b + invFluence * info->b;
        this->SE_data[type].a = influence * this->SE_data[type].a + invFluence * info->a;
        this->SE_data[type].intensity = influence * this->SE_data[type].intensity + invFluence * info->intensity;
    }

    this->SE_inf[type].active = 1;
    if (this->SE_data[type].UpdateFnc != nullptr) {
        this->SE_data[type].UpdateFnc(type, this);
    } else {
        this->SetController(type, controller);
    }

    if (this->SE_data[type].intensity < 0.01f) {
        this->SE_inf[type].active = 0;
    }
}

void ScreenEffectDB::AddPaletteEffect(ScreenEffectPalette palette) {
    this->AddPaletteEffect(&SE_PaletteFile[palette]);
}

void ScreenEffectDB::AddPaletteEffect(ScreenEffectPaletteDef *palette) {
    for (int i = 0; i < palette->NumEffects; i++) {
        this->AddScreenEffect(palette->SE_type[i], &palette->SE_Def[i], 1, palette->SE_Controller[i]);
    }
}

// gcdatos7: sin llamantes (DIEs .debug+0x40E2990/0x40E2A1E low_pc=-1, tras las
// AddPaletteEffect; A124 24/96 B). Espejos exactos de aquellas: la de enum
// delega en la de puntero (parametro PalPtr en su DIE, con el bloque {i} y el
// inline RemoveScreenEffect).
void ScreenEffectDB::RemovePaletteEffect(ScreenEffectPalette palette) {
    this->RemovePaletteEffect(&SE_PaletteFile[palette]);
}

void ScreenEffectDB::RemovePaletteEffect(ScreenEffectPaletteDef *PalPtr) {
    for (int i = 0; i < PalPtr->NumEffects; i++) {
        this->RemoveScreenEffect(PalPtr->SE_type[i]);
    }
}

void InitScreenEFX() {
#ifdef EA_BUILD_A124
    SE_PaletteFile[EFX_UNIQUE].NumEffects = 1;
    SE_PaletteFile[EFX_UNIQUE].SE_type[0] = SE_VISUAL_SIG;
    SE_PaletteFile[EFX_UNIQUE].SE_Controller[0] = SEC_FUNCTION;
    SE_PaletteFile[EFX_UNIQUE].SE_Def[0].UpdateFnc = Unique;
    SE_PaletteFile[EFX_UNIQUE].SE_Def[0].intensity = 0.5f;
#endif
}

ParameterAccessorBlendByDistance TintSunRiseAccessor[2] = {"Screen Tint SunRise", "Screen Tint SunRise"};
ParameterAccessorBlendByDistance TintMiddayAccessor[2] = {"Screen Tint Midday", "Screen Tint Midday"};

// gcdatos8: el bloque [0,1,2,3] de la .data objetivo de zTrack entre
// distfall y debugflash (obj+0x11D0): los cuatro ColourTintParam* del DWARF
// (sin direccion en GC; en la A124 en 0x6385F8-0x638608 con estos valores,
// tras los dos Tint*Accessor).
int ColourTintParamRed = 0;
int ColourTintParamGreen = 1;
int ColourTintParamBlue = 2;
int ColourTintParamIntensity = 3;
int debugflash = 0;

void TickSFX() {
    static uint32 ticS = 0;
    if (IsGameFlowInGame()) {
        if (ticS != eFrameCounter - 1) {
            UpdateAllScreenEFX();
        }
        ticS = eFrameCounter;
    }
}

// gcdatos8 (receta r68): en la .data objetivo de zTrack entre ticS
// (0x80437128) y FACflush (0x80437134), sin direccion en el DWARF de GC.
float frame_max = 120.0f;
float frame_speed = 0.2f;

static unsigned int FACflush = 0;
// gcdatos8 (receta r68): hueco de 4 B a 0 entre FACflush y GlareFalloff
// (0x80437138), el InTunnel muerto del DWARF.
unsigned int InTunnel = 0;
float GlareFalloff = 0.0199999996f;
float GlareFallon = 0.0399999991f;
// gcdatos8 (receta r68): definidos aqui en el original (lbl_80437144, entre
// GlareFallon y TUNHEIGHT) con estos valores; solo estaban los extern de
// arriba y el codigo vivo de este fichero los usa.
#ifdef EA_BUILD_A124
// gcdatos16: 0.04f en la A124 (bytes de su .data objetivo en zTrack +0x8e48:
// 0a d7 23 3d); 0.06f en las demas.
float DarkFalloff = 0.04f;
#else
float DarkFalloff = 0.06f;
#endif
float DarkFallon = 0.1f;
int tunnelDark = 70;
float TUNHEIGHT = 10.0f;
#ifdef EA_BUILD_A124
// gcdatos13: solo en la A124, tras TUNHEIGHT (0x638638 y 0x63863c,
// PS2_globals.hpp:5666-5667), a 1.0f en su .data objetivo. Sin simbolo en las demas.
float togbias = 1.0f;
float movbias = 1.0f;
#endif

void UpdateAllScreenEFX() {
    for (int i = 1; i <= 2; i++) {
        eView *view = eGetView(i, false);
        if (view->IsActive()) {
            eGetView(i, false)->ScreenEffects->Update(0.033333335f);
            if (debugflash != 0) {
                debugflash = 0;
                eGetView(i, false)->ScreenEffects->AddPaletteEffect(EFX_CAMERA_FLASH);
            }
#ifdef EA_BUILD_A124
            eGetView(i, false)->ScreenEffects->AddPaletteEffect(EFX_UNIQUE);
#endif
        }
    }
}

#ifdef EA_BUILD_A124
// El efecto EFX_UNIQUE de la A124: tine la pantalla con el color del mapa de parametros "Screen Tint SunRise" en la
// posicion del coche. Lee tambien el de mediodia, pero no lo usa.
void Unique(ScreenEffectType type, ScreenEffectDB *seDB) {
    seDB->SetController(type, SEC_FUNCTION);
    int idIndex = seDB->GetMyView()->GetID() - 1;
    float x = 0.0f;
    float y = 0.0f;

    GetXYviewCar(seDB->GetMyView(), &x, &y);
    float US_r = 0.0f;
    float US_g = 0.0f;
    float US_b = 0.0f;

    if (TintSunRiseAccessor[idIndex].IsValid()) {
        TintSunRiseAccessor[idIndex].CaptureData(x, y, 50.0f);
        US_r = TintSunRiseAccessor[idIndex].GetDataFloat(0);
        US_g = TintSunRiseAccessor[idIndex].GetDataFloat(1);
        US_b = TintSunRiseAccessor[idIndex].GetDataFloat(2);
    }
    if (TintMiddayAccessor[idIndex].IsValid()) {
        TintMiddayAccessor[idIndex].CaptureData(x, y, 50.0f);
        TintMiddayAccessor[idIndex].GetDataFloat(0);
        TintMiddayAccessor[idIndex].GetDataFloat(1);
        TintMiddayAccessor[idIndex].GetDataFloat(2);
    }
    seDB->SetRGBA(type, US_r, US_g, US_b, 128.0f);
}
#endif

void FlushAccumulationBuffer() {
    FACflush = 1;
}

void AccumulationBufferFlushed() {
    FACflush = 0;
}

unsigned int QueryFlushAccumulationBuffer() {
    return FACflush;
}
void DoTinting(eView *view) {
#ifdef EA_BUILD_A124
    ScreenEffectDef SE_def;
    float intense = view->Precipitation->GetCloudIntensity();

    if (0.0f < intense) {
        unsigned int r;
        unsigned int g;
        unsigned int b;
        view->Precipitation->GetPrecipFogColour(&r, &g, &b);
        SE_def.r = static_cast<float>(r);
        SE_def.g = static_cast<float>(g);
        SE_def.b = static_cast<float>(b);
        SE_def.a = 128.0f;
        SE_def.UpdateFnc = nullptr;
        SE_def.intensity = intense;
        view->ScreenEffects->AddScreenEffect(SE_TINT, &SE_def, 1, SEC_FRAME);
    } else if (AmIinATunnel(view, 0) != 0) {
        SE_def.data[0] = 0.0f;
        SE_def.data[1] = 0.0f;

        if (view->ScreenEffects->GetDATA(SE_TINT, 0) == 0.0f) {
            SE_def.data[0] = 1.0f;
            SE_def.data[1] = 0.01f;
        }
        if (view->ScreenEffects->GetDATA(SE_TINT, 0) == 1.0f) {
            if (view->ScreenEffects->GetDATA(SE_TINT, 1) < 1.0f) {
                SE_def.data[0] = view->ScreenEffects->GetDATA(SE_TINT, 0);
                SE_def.data[1] = view->ScreenEffects->GetDATA(SE_TINT, 1) + DarkFallon;
            } else {
                SE_def.data[0] = 2.0f;
                SE_def.data[1] = 1.0f;
            }
        }
        if (view->ScreenEffects->GetDATA(SE_TINT, 0) == 2.0f) {
            SE_def.data[0] = view->ScreenEffects->GetDATA(SE_TINT, 0);
            if (0.0f < view->ScreenEffects->GetDATA(SE_TINT, 1)) {
                SE_def.data[0] = view->ScreenEffects->GetDATA(SE_TINT, 0);
                SE_def.data[1] = view->ScreenEffects->GetDATA(SE_TINT, 1) - DarkFalloff;
            } else {
                SE_def.data[0] = 2.0f;
                SE_def.data[1] = 0.0f;
            }
        }

        SE_def.r = static_cast<float>(tunnelDark);
        SE_def.g = SE_def.r;
        SE_def.b = SE_def.r;
        SE_def.a = 128.0f;
        SE_def.intensity = SE_def.data[1];
        SE_def.UpdateFnc = nullptr;
        view->ScreenEffects->AddScreenEffect(SE_TINT, &SE_def, 1, SEC_FRAME);
    } else {
        view->ScreenEffects->SetDATA(SE_TINT, 0.0f, 0);
        return;
    }
#else
    ScreenEffectDef SE_def;
    unsigned int r;
    unsigned int g;
    unsigned int b;
    float intense;

    if (IsRainDisabled()) {
        return;
    }

    if (view->Precipitation != nullptr) {
        intense = view->Precipitation->GetCloudIntensity();
    } else {
        intense = 0.0f;
    }

    if (0.0f < intense) {
        if (view->Precipitation != nullptr) {
            view->Precipitation->GetPrecipFogColour(&r, &g, &b);
        }
        SE_def.r = static_cast<float>(r);
        SE_def.g = static_cast<float>(g);
        SE_def.a = 128.0f;
        SE_def.UpdateFnc = nullptr;
        SE_def.intensity = intense;
        SE_def.b = static_cast<float>(b);
        view->ScreenEffects->AddScreenEffect(SE_TINT, &SE_def, 1, SEC_FRAME);
    }
#endif
}

// UNSOLVED, functionally matching, just regswaps: https://decomp.me/scratch/Ar2tQ
void DoTunnelBloom(eView *view) {
    int vIndex = 1;
    float BaseGlare; // TODO
    if (view->GetID() == EVIEW_PLAYER1) {
        vIndex = 0;
    }

    if (!view->IsActive()) {
        return;
    }

    CameraMover *cameraMover = view->GetCameraMover();
    if (cameraMover == nullptr) {
        return;
    }

    CameraAnchor *cameraAnchor = cameraMover->GetAnchor();
    if (cameraAnchor == nullptr) {
        return;
    }

    bVector3 *MyCarPos = cameraAnchor->GetGeometryPosition();
    Camera *view_camera = view->GetCamera();
    bVector3 *CameraPosition = view_camera->GetPosition();
    bVector3 *CameraDirection = view_camera->GetDirection();
    bVector2 twoDpos(CameraPosition->x, CameraPosition->y);

    bVector3 endVector;

    static bVector3 lcamPosInside[2];
    static float dataBackup[2][12];
    static GenericRegion *regionB[2] = {};

    bVector3 posScreen;
    TrackPathZone *zone = nullptr;
    TrackPathZone *zoneBP = zoneB[vIndex];
    if ((zoneBP != nullptr) && zoneBP->IsPointInside(&twoDpos)) {
        zone = zoneB[vIndex];
    } else {
        zone = TheTrackPathManager.FindZone(&twoDpos, TRACK_PATH_ZONE_TUNNEL, nullptr);
    }

    if ((zone != nullptr) && zone->GetElevation() > MyCarPos->z) {
        bVector2 p0;
        bVector2 p1;
        lcamPosInside[vIndex] = *CameraPosition;
        float angleCos = 0.0f;
        GenericRegion *EndTunnelP = GetClosestRegionInView(view, &endVector, &angleCos);
        if (EndTunnelP != nullptr) {
            ScreenEffectDef SE_def;
            bVector2 endP(endVector.x, endVector.y);
            float len = zone->GetSegmentNextTo(&endP, &p0, &p1);
            if (len != -1.0f && len < 40.0f) {
                {
                    bVector3 p3(endVector);
                    UMath::Vector3 usPoint;
                    eUnSwizzleWorldVector(p3, *reinterpret_cast<bVector3 *>(&usPoint));

                    float height;
                    WCollisionMgr(0, 3).GetWorldHeightAtPointRigorous(usPoint, height, nullptr);

                    SE_def.data[2] = p0.x + CameraDirection->x;
                    dataBackup[vIndex][0] = SE_def.data[2];

                    SE_def.data[3] = p0.y + CameraDirection->y;
                    dataBackup[vIndex][1] = SE_def.data[3];

                    SE_def.data[4] = height + CameraDirection->z;
                    dataBackup[vIndex][2] = SE_def.data[4];

                    SE_def.data[5] = p1.x + CameraDirection->x;
                    dataBackup[vIndex][3] = SE_def.data[5];

                    SE_def.data[6] = p1.y + CameraDirection->y;
                    dataBackup[vIndex][4] = SE_def.data[6];

                    SE_def.data[7] = height + CameraDirection->z;
                    dataBackup[vIndex][5] = SE_def.data[7];

                    SE_def.data[8] = p0.x + CameraDirection->x;
                    dataBackup[vIndex][6] = SE_def.data[8];

                    SE_def.data[9] = p0.y + CameraDirection->y;
                    dataBackup[vIndex][7] = SE_def.data[9];

                    SE_def.data[10] = height + TUNHEIGHT + CameraDirection->z;
                    dataBackup[vIndex][8] = SE_def.data[10];

                    SE_def.data[11] = p1.x + CameraDirection->x;
                    dataBackup[vIndex][9] = SE_def.data[11];

                    SE_def.data[12] = p1.y + CameraDirection->y;
                    dataBackup[vIndex][10] = SE_def.data[12];

                    SE_def.data[13] = height + TUNHEIGHT + CameraDirection->z;
                    dataBackup[vIndex][11] = SE_def.data[13];

                    SE_def.r = 128.0f;
                    SE_def.g = 128.0f;
                    SE_def.b = 128.0f;
                    SE_def.a = 128.0f;
                    SE_def.UpdateFnc = nullptr;
                }

                if (regionB[vIndex] != EndTunnelP) {
                    view->ScreenEffects->SetDATA(SE_GLARE, 0.0f, 1);
                }
                regionB[vIndex] = EndTunnelP;
                if (zoneB[vIndex] != zone) {
                    view->ScreenEffects->SetDATA(SE_GLARE, 0.0f, 1);
                }

                zoneB[vIndex] = zone;
                {
                    bVector2 r = p0 - twoDpos;
                    bVector2 v(p1.y - p0.y, -(p1.x - p0.x));
                    bNormalize(&v, &v);
                    len = bDot(&v, &r);
                    len = bAbs(len);
                }

                if (len < 17.0f) {
                    SE_def.data[1] = view->ScreenEffects->GetDATA(SE_GLARE, 1) * (len / 17.0f);
                } else {
                    if (view->ScreenEffects->GetDATA(SE_GLARE, 1) < 1.0f) {
                        SE_def.data[1] = view->ScreenEffects->GetDATA(SE_GLARE, 1) + GlareFallon;
                    } else {
                        SE_def.data[1] = 1.0f;
                    }
                }

                SE_def.intensity = SE_def.data[1];
                view->ScreenEffects->AddScreenEffect(SE_GLARE, &SE_def, 1, SEC_FRAME);

                if ((view->Precipitation != nullptr) && view->Precipitation->GetRainIntensity() > 0.0f) {
                    view->Precipitation->IsValidRainCurtainPos = CT_OVERIDE;
                    view->Precipitation->AttachRainCurtain(SE_def.data[8], SE_def.data[9], SE_def.data[10], SE_def.data[11], SE_def.data[12],
                                                           SE_def.data[13], SE_def.data[2], SE_def.data[3], SE_def.data[4], SE_def.data[5],
                                                           SE_def.data[6], SE_def.data[7]);
                }
            }
        }
        return;
    }

    if (view->ScreenEffects->GetIntensity(SE_GLARE) > 0.0f) {
        ScreenEffectDef SE_def;
        SE_def.r = 128.0f;
        SE_def.g = 128.0f;
        SE_def.b = 128.0f;
        SE_def.a = 128.0f;
        SE_def.UpdateFnc = nullptr;

        SE_def.intensity = view->ScreenEffects->GetIntensity(SE_GLARE) - GlareFalloff;
        SE_def.data[1] = SE_def.intensity;

        {
            bVector3 midpoint((dataBackup[vIndex][0] + dataBackup[vIndex][3]) * 0.5f, (dataBackup[vIndex][1] + dataBackup[vIndex][4]) * 0.5f,
                              (dataBackup[vIndex][2] + dataBackup[vIndex][5]) * 0.5f);
            bVector3 ToGlare = *CameraPosition - lcamPosInside[vIndex];
            ToGlare += *CameraDirection * 1.0f;

            if (SE_def.intensity > 0.0f) {
                SE_def.data[2] = ToGlare.x + dataBackup[vIndex][0];
                SE_def.data[3] = ToGlare.y + dataBackup[vIndex][1];
                SE_def.data[4] = ToGlare.z + dataBackup[vIndex][2];

                SE_def.data[5] = ToGlare.x + dataBackup[vIndex][3];
                SE_def.data[6] = ToGlare.y + dataBackup[vIndex][4];
                SE_def.data[7] = ToGlare.z + dataBackup[vIndex][5];

                SE_def.data[8] = ToGlare.x + dataBackup[vIndex][6];
                SE_def.data[9] = ToGlare.y + dataBackup[vIndex][7];
                SE_def.data[10] = ToGlare.z + dataBackup[vIndex][8];

                SE_def.data[11] = ToGlare.x + dataBackup[vIndex][9];
                SE_def.data[12] = ToGlare.y + dataBackup[vIndex][10];
                SE_def.data[13] = ToGlare.z + dataBackup[vIndex][11];
                view->ScreenEffects->AddScreenEffect(SE_GLARE, &SE_def, 1, SEC_FRAME);
            }
        }
    }
}
