#include "Speed/Indep/Src/Frontend/HUD/feMinimap.hpp"

#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/Src/FEng/FEImage.h"
#include "Speed/Indep/Src/FEng/FEMultiImage.h"
#include "Speed/Indep/Src/FEng/FEObject.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/HUD/FeMinimapStreamer.hpp"
#include "Speed/Indep/Src/Gameplay/GIcon.h"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Interfaces/Simables/ICollisionBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Physics/PVehicle.h"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#include "Speed/Indep/Src/World/RaceParameters.hpp"

bool GPS_IsEngaged();

bool MinimapShowNonPursuitCops = false;
bool MinimapShowPursuitCops = true;
float MinimapPivotX = 0.0f;
float MinimapPivotY = 0.0f;
float MinimapDispX = 0.0f;
float MinimapMaxSpeed = 100.0f;

Minimap::GameplayIconInfo Minimap::kGameplayIconInfo[] = {
    { GII_NONE, WMIT_NONE, "", 0x0, 0x0 },
    { GII_SPRINT, WMIT_SPRINT_RACE, "MMICON_SPRINT_%d", 0xB94FD70E, 0x2521E5EB },
    { GII_CIRCUIT, WMIT_CIRCUIT_RACE, "MMICON_CIRCUIT_%d", 0x034FA2C1, 0xE9638D3E },
    { GII_DRAG, WMIT_DRAG_RACE, "MMICON_DRAG_%d", 0x6F547E4C, 0xAAAB31E9 },
    { GII_KNOCKOUT, WMIT_LAP_KO_RACE, "MMICON_KNOCKOUT_%d", 0x4930F5FC, 0x3A015595 },
    { GII_TOLLBOOTH, WMIT_TOLLBOOTH_RACE, "MMICON_TOLLBOOTH_%d", 0xA15E4505, 0x1A091045 },
    { GII_SPEEDTRAP, WMIT_SPEED_TRAP_RACE, "MMICON_SPEEDTRAP_%d", 0xEE7386EA, 0x66C9A7B6 },
#ifdef EA_BUILD_A124 // sin textura en el mapa del mundo (.data de la A124)
    { GII_RIVAL, WMIT_RIVAL_RACE, "MINIMAP_ICON_EVENT_RIVAL", 0x213CC8D1, 0x0 },
#else
    { GII_RIVAL, WMIT_RIVAL_RACE, "MINIMAP_ICON_EVENT_RIVAL", 0x213CC8D1, 0x9F562D57 },
#endif
    { GII_SAFEHOUSE, WMIT_SAFEHOUSE, "SAFEHOUSEINDICATOR_%d", 0xDED7B298, 0x0ED39F69 },
    { GII_CARLOT, WMIT_CAR_LOT, "CAR_LOT_%d", 0xB9D71959, 0x4EAEE18B },
    { GII_SHOP, WMIT_SHOP, "SHOP_%d", 0xE0C8090F, 0x0CF07089 },
    { GII_HIDINGSPOT, WMIT_HIDING_SPOT, "HIDING_SPOT_%d", 0xA31E817A, 0xF172B456 },
    { GII_PURSUITSBREAKER, WMIT_PURSUIT_BREAKER, "PURSUIT_BREAKER_%d", 0xDAC9D619, 0xD01A771E },
#ifdef EA_BUILD_A124 // el radar usa el titulo del de las carreras, y no hay mas iconos (.data de la A124)
    { GII_SPEEDTRAPCAM, WMIT_SPEED_TRAP, "SPEED_TRAP_CAM_%d", 0xEE7386EA, 0x66C9A7B6 },
#else
    { GII_SPEEDTRAPCAM, WMIT_SPEED_TRAP, "SPEED_TRAP_CAM_%d", 0x5BDCF050, 0x66C9A7B6 },
    { GII_SPEEDTRAPRACECAM, WMIT_SPEED_TRAP, "SPEED_TRAP_RACE_CAM_%d", 0xEE7386EA, 0x66C9A7B6 },
    { GII_CHECKPOINT, WMIT_NONE, "MMICON_CHECKPOINT", 0x0, 0x0 },
#endif
    { GII_MAX, WMIT_NONE, "", 0x0, 0x0 },
    { static_cast<Type>(GII_MAX + 1), WMIT_NONE, "", 0x0, 0x0 },
};

bChunkLoader bChunkLoaderMiniMap(0x3A100, LoaderMiniMap, UnloaderMiniMap);

#include "Speed/Indep/Src/Animation/AnimWorldTypes.hpp"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"

// GLOBAL, no `static`: el objetivo lo tiene en .text:0x80144198 con scope:global
// y lo llama uiWorldMap.cpp, que vive en zFe (otra unidad de enlace).
void GetVehicleVectors(bVector2 *pos, bVector2 *dir, ISimable *simable) {
    UMath::Vector3 position = simable->GetPosition();

    pos->y = -position.x;
    pos->x = position.z;

    ICollisionBody *body;
    if (simable->QueryInterface(&body)) {
        UMath::Vector3 velocity = body->GetForwardVector();

        dir->y = -velocity.x;
        dir->x = velocity.z;
    }
}

int LoaderMiniMap(bChunk *chunk) {
    return gChoppedMiniMapManager->Loader(chunk);
}

int UnloaderMiniMap(bChunk *chunk) {
    return gChoppedMiniMapManager->Unloader(chunk);
}

Minimap::Minimap(const char *pkg_name, int player_number)
    : HudElement(pkg_name, HUD_FEATURE_MINIMAP | HUD_FEATURE_CANYON_METER), mSpeedZoomScale(0.0f), mPolyRotation(0.0f),
      mCopFlashCounter(-1) {
#ifndef EA_BUILD_A124 // la A124 no reinicia el pivote al crear el radar
    MinimapPivotX = 0.0f;
#endif
    mTrackTargetNormalized.x = 0.0f;
    mTrackTargetNormalized.y = 0.0f;
    mTrackMapCentre.x = 0.0f;
    mTrackMapCentre.y = 0.0f;

    for (unsigned int i = 0; i < 4; i++) {
        TrackmapArt[i] = RegisterMultiImage(FEngHashString("TRACK_MAP%d", i + 1));
        if (TrackmapArt[i] != nullptr) {
            TrackmapArt[i]->GetUVs(0, TrackmapArtUVs[i][0], TrackmapArtUVs[i][1]);
        }
    }

    TrackmapLayout = RegisterObject(FEngHashString("TRACK_MAP"));
    TrackmapNorth = RegisterImage(FEngHashString("MINIMAP_NORTH_INDICATOR"));
    mPlayerCarIndicator = RegisterImage(FEngHashString("PLAYERCARINDICATOR"));
    mPlayerCarIndicator2 = RegisterImage(FEngHashString("PLAYERCARINDICATOR2"));
    RegisterObject(FEngHashString("TRACKMAPTARGETRING"));
    RegisterObject(FEngHashString("MAP_COLOR_TINT"));

    bMemSet(mGameplayIcons, 0, sizeof(mGameplayIcons));

    mHeliElementArt = RegisterGroup(FEngHashString("HELICOPTER_ICON_GROUP"));
    mHeliLineOfSiteArt = RegisterImage(FEngHashString("HELICOPTER_LINE_OF_SIGHT"));

    for (unsigned int p = 0; p < 8; p++) {
        mCopElementArt[p] = RegisterImage(FEngHashString("MMICON_COPCAR_%d", p));
        mRacerElementArt[p] = RegisterImage(FEngHashString("MMICON_AIRACER_%d", p));

        for (int g = 0; g < GII_MAX + 1; g++) {
            int onType = kGameplayIconInfo[g].mItemType;
            if (onType != WMIT_NONE) {
#ifdef EA_PLATFORM_XENON // X360: cmpwi con signo en 0x822C1F68 (el bucle p<8 es cmplwi en 0x822C20EC)
                if ((int)p == 0 || bStrStr(kGameplayIconInfo[g].mElementString, "%") != nullptr) {
#else
                if (p == 0 || bStrStr(kGameplayIconInfo[g].mElementString, "%") != nullptr) {
#endif
                    mGameplayIcons[g][p] = RegisterImage(FEngHashString(kGameplayIconInfo[g].mElementString, p));
                    if (mGameplayIcons[g][p] != nullptr) {
                        FEngSetInvisible(mGameplayIcons[g][p]);
                    }
                } else {
                    mGameplayIcons[g][p] = nullptr;
                }
            }
        }
    }

    mCheckpointElementArt = RegisterImage(FEngHashString("MMICON_CHECKPOINT"));
    mGPSSelectionElementArt = RegisterImage(0xE8741681);

    if (TrackmapLayout != nullptr) {
        mMapDefaultPos = TrackmapLayout->GetObjData()->Pos;
        mTrackMapCentre.x = FEngGetCenterX(TrackmapLayout);
        mTrackMapCentre.y = FEngGetCenterY(TrackmapLayout);
    }

    InitStaticMiniMapItems();
}

Minimap::~Minimap() {
    gChoppedMiniMapManager->RemoveUncompressedMaps();
}

void Minimap::SetupMinimap(IPlayer *player) {
    CurrentTrack = TrackInfo::GetTrackInfo(TheRaceParameters.TrackNumber);

    const int num_chops = 4;
    short chop_nums[4] = { 0, 0, 0, 0 };
    // En este orden en los stabs de la A124 y en el DWARF: map_pos antes que target_pos y target_dir.
    bVector2 map_pos;
    bVector2 target_pos;
    bVector2 target_dir;
    ISimable *isimable = player->GetSimable();
    GetVehicleVectors(&target_pos, &target_dir, isimable);

    ConvertPos(target_pos, map_pos, CurrentTrack);
    map_pos.x *= 8.0f;
    map_pos.y *= 8.0f;

    int XSection = static_cast<int>(map_pos.x);
    float XSection_decimal = map_pos.x - static_cast<float>(XSection);
    int YSection = static_cast<int>(map_pos.y);
    float YSection_decimal = map_pos.y - static_cast<float>(YSection);
    float SectionSize = 128.0f;

    float xDisp;
    float yDisp;
    if (XSection_decimal < 0.5f) {
        if (YSection_decimal < 0.5f) {
            chop_nums[0] = (YSection - 1) * 8 + XSection - 1;
            chop_nums[1] = (YSection - 1) * 8 + XSection;
            chop_nums[2] = YSection * 8 + XSection - 1;
            chop_nums[3] = YSection * 8 + XSection;
            xDisp = XSection_decimal;
            yDisp = YSection_decimal;
        } else {
            chop_nums[0] = YSection * 8 + XSection - 1;
            chop_nums[1] = YSection * 8 + XSection;
            chop_nums[2] = (YSection + 1) * 8 + XSection - 1;
            chop_nums[3] = (YSection + 1) * 8 + XSection;
            xDisp = XSection_decimal;
            yDisp = YSection_decimal - 1.0f;
        }
    } else {
        if (YSection_decimal < 0.5f) {
            chop_nums[0] = (YSection - 1) * 8 + XSection;
            chop_nums[1] = (YSection - 1) * 8 + XSection + 1;
            chop_nums[2] = YSection * 8 + XSection;
            chop_nums[3] = YSection * 8 + XSection + 1;
            xDisp = XSection_decimal - 1.0f;
            yDisp = YSection_decimal;
        } else {
            chop_nums[0] = YSection * 8 + XSection;
            chop_nums[1] = YSection * 8 + XSection + 1;
            chop_nums[2] = (YSection + 1) * 8 + XSection;
            chop_nums[3] = (YSection + 1) * 8 + XSection + 1;
            xDisp = XSection_decimal - 1.0f;
            yDisp = YSection_decimal - 1.0f;
        }
    }

    gChoppedMiniMapManager->UncompressMaps(chop_nums, 4);

    char texture_name[128];
    for (unsigned int i = 0; i < 4; i++) {
        gChoppedMiniMapManager->GetTextureName(texture_name, 128, chop_nums[i]);
        unsigned int hash = FEngHashString(texture_name);
        FEngSetTextureHash(TrackmapArt[i], hash);
    }

    xDisp *= mSpeedZoomScale;
    yDisp *= mSpeedZoomScale;

    float uvScale = mSpeedZoomScale - 1.0f;
    FEVector2 top_left(uvScale, uvScale);
    FEVector2 bottom_right(1.0f, 1.0f);
    TrackmapArt[0]->SetTopLeft(top_left, false);
    TrackmapArt[0]->SetBottomRight(bottom_right, false);
    top_left = FEVector2(0.0f, uvScale);
    bottom_right = FEVector2(1.0f - uvScale, 1.0f);
    TrackmapArt[1]->SetTopLeft(top_left, false);
    TrackmapArt[1]->SetBottomRight(bottom_right, false);
    top_left = FEVector2(uvScale, 0.0f);
    bottom_right = FEVector2(1.0f, 1.0f - uvScale);
    TrackmapArt[2]->SetTopLeft(top_left, false);
    TrackmapArt[2]->SetBottomRight(bottom_right, false);
    top_left = FEVector2(0.0f, 0.0f);
    bottom_right = FEVector2(1.0f - uvScale, 1.0f - uvScale);
    TrackmapArt[3]->SetTopLeft(top_left, false);
    TrackmapArt[3]->SetBottomRight(bottom_right, false);

    top_left = FEVector2(-xDisp, -yDisp);
    bottom_right = FEVector2(-xDisp, -yDisp);
    TrackmapArt[0]->SetUVs(0, TrackmapArtUVs[0][0] + top_left, TrackmapArtUVs[0][1] + bottom_right);
    TrackmapArt[1]->SetUVs(0, TrackmapArtUVs[1][0] + top_left, TrackmapArtUVs[1][1] + bottom_right);
    TrackmapArt[2]->SetUVs(0, TrackmapArtUVs[2][0] + top_left, TrackmapArtUVs[2][1] + bottom_right);
    TrackmapArt[3]->SetUVs(0, TrackmapArtUVs[3][0] + top_left, TrackmapArtUVs[3][1] + bottom_right);

    xDisp *= SectionSize;
    yDisp *= SectionSize;

    TrackmapLayout->GetObjData()->Pos = FEVector3(mMapDefaultPos.x - xDisp, mMapDefaultPos.y - yDisp, mMapDefaultPos.z);

    TrackmapLayout->GetObjData()->Pivot = FEVector3(xDisp + MinimapPivotX, yDisp + MinimapPivotY, 0.0f);
}

void Minimap::RefreshMapItems() {
    for (MiniMapItem *item = StaticMiniMapItems.GetHead(); item != StaticMiniMapItems.EndOfList(); item = item->GetNext()) {
        FEngSetInvisible(item->pIcon);
    }
    StaticMiniMapItems.DeleteAllElements();
    InitStaticMiniMapItems();
}

void Minimap::ConvertPos(bVector2 &worldPos, bVector2 &minimapPos, TrackInfo *track) {
    minimapPos.x = (worldPos.x - track->TrackMapCalibrationUpperLeft.x) / track->TrackMapCalibrationMapWidthMetres;
    minimapPos.y = (track->TrackMapCalibrationUpperLeft.y - worldPos.y) / track->TrackMapCalibrationMapWidthMetres + 1.0f;
}

void Minimap::Update(IPlayer *player) {
    if (!IsElementVisible() || player == nullptr || player->GetSimable() == nullptr) {
        return;
    }

    MinimapRotateWithPlayer = 1;

    if (Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) {
        MinimapRotateWithPlayer = 0;
    } else {
        if (GRaceStatus::Get().GetRaceParameters() == nullptr) {
            if (FEDatabase->GetGameplaySettings()->ExploringMiniMapMode == 0) {
                MinimapRotateWithPlayer = 0;
            }
        } else {
            if (FEDatabase->GetGameplaySettings()->RacingMiniMapMode == 0) {
                MinimapRotateWithPlayer = 0;
            }
        }
    }

    SetupMinimap(player);

    float speed = 0.0f;
    ISimable *simable = player->GetSimable();
    bVector2 carPos;
    bVector2 carDir;
    GetVehicleVectors(&carPos, &carDir, simable);

    IVehicle *vehicle;
    if (simable->QueryInterface(&vehicle)) {
        speed = bAbs(vehicle->GetSpeed());
    }

    mPolyRotation = static_cast<unsigned int>(bATan(carDir.y, carDir.x)) * 0.0054931640625f;

    ConvertPos(carPos, mTrackTargetNormalized, CurrentTrack);

    if (speed > MinimapMaxSpeed) {
        speed = MinimapMaxSpeed;
    } else if (speed < 0.0f) {
        speed = 0.0f;
    }

    mSpeedZoomScale = 1.0f - speed / MinimapMaxSpeed;
    if (mSpeedZoomScale < 1.0f) {
        mSpeedZoomScale = 1.0f;
    }

    UpdateTrackMapArt();

    if (MinimapRotateWithPlayer == 0) {
        mPolyRotation = 0.0f;
    }

    UpdateCopElements(vehicle);
    UpdateAiRacerElements();
    UpdatePlayer2Element();
    UpdateRaceElements();
    UpdateGameplayIcons(player);
}

void Minimap::UpdateTrackMapArt() {
    if (MinimapRotateWithPlayer == 0) {
        FEngSetRotationZ(mPlayerCarIndicator, mPolyRotation);
        FEngSetRotationZ(TrackmapLayout, 0.0f);
        FEngSetRotationZ(TrackmapNorth, 0.0f);
    } else {
        FEngSetRotationZ(mPlayerCarIndicator, 0.0f);
        FEngSetRotationZ(TrackmapLayout, -mPolyRotation);
        FEngSetRotationZ(TrackmapNorth, -mPolyRotation);
    }
}

void Minimap::UpdateElementArt(bVector2 *elementPos, bVector2 *elementDir, FEObject *elementArt, bool pulse) {
    bVector2 mapPos;
    ConvertPos(*elementPos, mapPos, CurrentTrack);

    float epoly_x = (mapPos.x - mTrackTargetNormalized.x) * mSpeedZoomScale;
    float epoly_y = (mapPos.y - mTrackTargetNormalized.y) * mSpeedZoomScale;
    const float sa = bSin(bDegToRad(mPolyRotation));
    const float ca = bCos(bDegToRad(mPolyRotation));
    float rot_epoly_x = epoly_x * ca + epoly_y * sa;
    float rot_epoly_y = epoly_y * ca - epoly_x * sa;
    float distance = bSqrt(rot_epoly_x * rot_epoly_x + rot_epoly_y * rot_epoly_y);
    float alpha = 1.0f;

#ifdef EA_BUILD_A124
    if (distance > 0.0f && distance > 0.06f && distance < 0.23f) {
        float scale = 0.06f / distance;
        rot_epoly_x *= scale;
        rot_epoly_y *= scale;

        if (distance > 0.125f) {
            alpha = 1.0f - (distance - 0.125f) * 9.5238094f;
        }
        distance = 0.06f;
    }
#else
    if (distance > 0.0f && distance > 0.06f) {
        float scale = 0.06f / distance;
        rot_epoly_x *= scale;
        rot_epoly_y *= scale;

        if (distance > 0.125f) {
            alpha = 1.0f - (distance - 0.125f) * 9.5238094f;
        }
        if (distance > 0.23f) {
            alpha = 0.0f;
        }
        if (pulse) {
            alpha = 1.0f;
        }
        distance = 0.06f;
    }
#endif

    if (distance <= 0.06f) {
        float centerX = mTrackMapCentre.x + rot_epoly_x * 1024.0f;
        float centerY = mTrackMapCentre.y + rot_epoly_y * 1024.0f;
        FEngSetCenter(elementArt, centerX, centerY);
        FEngSetVisible(elementArt);
        float element_angle = bAngToDeg(bATan(elementDir->y, elementDir->x));
        FEngSetRotationZ(elementArt, element_angle - mPolyRotation);

        uint32 color = FEngGetColor(elementArt);
        color = (color & 0xFFFFFF) | (static_cast<uint32>(static_cast<int>(alpha * 255.0f)) << 24);
        FEngSetColor(elementArt, color);

        if (pulse) {
            FEngSetVisible(mGPSSelectionElementArt);
            FEngSetCenter(mGPSSelectionElementArt, centerX, centerY);
        }
    } else {
        FEngSetInvisible(elementArt);
    }
}

void Minimap::UpdateCopElements(IVehicle *ivehicle) {
    unsigned int artIter = 0;
    bool helicopterFound = false;

    if (mCopFlashCounter++ >= 8) {
        mCopFlashCounter = 0;
    }

    eVehicleList list_id = VEHICLE_AICOPS;
    IPursuit *ipursuit = ivehicle->GetAIVehiclePtr()->GetPursuit();

    if (MinimapShowNonPursuitCops || (ipursuit != nullptr && !ipursuit->IsPursuitBailed())) {
        const IVehicle::List &vehicles = IVehicle::GetList(list_id);
        for (IVehicle::List::const_iterator iter = vehicles.begin(); iter != vehicles.end(); ++iter) {
            if (!(*iter)->IsActive()) {
                continue;
            }
            if (artIter >= 8) {
                break;
            }

            bVector2 target_pos;
            bVector2 target_dir;
            ISimable *isimable = (*iter)->GetSimable();
            IPursuitAI *ipursuitai;
            FEObject *copArtToUse;

            GetVehicleVectors(&target_pos, &target_dir, isimable);
            (*iter)->QueryInterface(&ipursuitai);

            if ((*iter)->GetVehicleClass() == VehicleClass::CHOPPER) {
                if (MinimapShowNonPursuitCops || (ipursuitai != nullptr && ipursuitai->WasWithinEngagementRadius())) {
                    AITarget *target = ipursuitai->GetPursuitTarget();
                    if (target != nullptr && ipursuitai->GetTimeSinceTargetSeen() <= 0.25f) {
                        if (!FEngIsScriptSet(mHeliLineOfSiteArt, FEHashUpper("TRACKING"))) {
                            FEngSetScript(mHeliLineOfSiteArt, FEHashUpper("TRACKING"), true);
                        }
                    } else {
                        if (!FEngIsScriptSet(mHeliLineOfSiteArt, FEHASH_INIT)) {
                            FEngSetScript(mHeliLineOfSiteArt, FEHASH_INIT, true);
                        }
                    }
                }
                UpdateElementArt(&target_pos, &target_dir, mHeliElementArt, false);
                UpdateElementArt(&target_pos, &target_dir, mHeliLineOfSiteArt, false);
                helicopterFound = true;
            } else {
                if (MinimapShowNonPursuitCops || (ipursuitai != nullptr && ipursuitai->WasWithinEngagementRadius() && MinimapShowPursuitCops)) {
                    UpdateElementArt(&target_pos, &target_dir, mCopElementArt[artIter], false);
                } else {
                    FEngSetInvisible(mCopElementArt[artIter]);
                }
            }

            if ((*iter)->GetVehicleClass() != VehicleClass::CHOPPER) {
                uint32 copFlasherColour = 0xFFCCCCCC;
                if (ipursuitai != nullptr && ipursuitai->GetInPursuit()) {
                    if (mCopFlashCounter <= 2) {
                        copFlasherColour = 0xFF0000FF;
                    } else if (static_cast<unsigned int>(mCopFlashCounter - 4) <= 2) {
                        copFlasherColour = 0xFFA00000;
                    }
                }
                FEngSetColor(mCopElementArt[artIter], copFlasherColour);
            }

            if ((*iter)->GetVehicleClass() != VehicleClass::CHOPPER) {
                artIter++;
            }
        }
    }

    for (unsigned int i = artIter; i < 8; i++) {
        FEngSetInvisible(mCopElementArt[i]);
    }

    if (!helicopterFound) {
        FEngSetInvisible(mHeliElementArt);
        FEngSetInvisible(mHeliLineOfSiteArt);
    }
}

void Minimap::UpdateAiRacerElements() {
    unsigned int artIter = 0;
#ifdef EA_BUILD_A124 // la A124 pinta tambien los rivales remotos en las carreras en linea
    eVehicleList listid = TheOnlineManager.IsOnlineRace() ? VEHICLE_REMOTE : VEHICLE_AIRACERS;
#else
    eVehicleList listid = VEHICLE_AIRACERS;
#endif
    const IVehicle::List &vehicles = IVehicle::GetList(listid);

    for (IVehicle::List::const_iterator iter = vehicles.begin(); iter != vehicles.end(); ++iter) {
        if (!(*iter)->IsActive()) {
            continue;
        }

        bVector2 target_pos;
        bVector2 target_dir;
        ISimable *isimable = (*iter)->GetSimable();
        GetVehicleVectors(&target_pos, &target_dir, isimable);
        UpdateElementArt(&target_pos, &target_dir, mRacerElementArt[artIter], false);
        artIter++;
    }

    for (unsigned int i = artIter; i < 8; i++) {
        FEngSetInvisible(mRacerElementArt[i]);
    }
}

void Minimap::UpdatePlayer2Element() {
    if (Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) {
        IPlayer *player2 = IPlayer::Last(PLAYER_LOCAL);
        bVector2 target_pos;
        bVector2 target_dir;
        ISimable *isimable = player2->GetSimable();
        GetVehicleVectors(&target_pos, &target_dir, isimable);
        UpdateElementArt(&target_pos, &target_dir, mPlayerCarIndicator2, false);
    }
}

void Minimap::UpdateIconElement(FEImage *image, GIcon *icon) {
    bVector2 pos2D;
    bVector2 dir2D;
    icon->GetPosition2D(pos2D);
    dir2D.x = 1.0f;
    dir2D.y = 0.0f;

#ifndef EA_BUILD_A124 // la A124 no apaga el GPS del icono
    if (icon->GetType() != GIcon::kType_AreaUnlock && !GPS_IsEngaged() && icon->GetIsGPSing()) {

        icon->ClearGPSing();
    }
#endif

    UpdateElementArt(&pos2D, &dir2D, image, icon->GetIsGPSing());
    FEngSetRotationZ(image, 0.0f);
}

void Minimap::UpdateRaceElements() {
    UpdateMiniMapItems();
}

void Minimap::AdjustForWidescreen(bool moveOutwards) {
    float adjustment;

    if (moveOutwards) {
        adjustment = -120.0f;
        MinimapPivotX = -120.0f;
        MinimapDispX = -0.9375f;
    } else {
        adjustment = 120.0f;
        MinimapPivotX = 0.0f;
        MinimapDispX = 0.9375f;
    }

    mTrackMapCentre.x += adjustment;

    for (unsigned int i = 0; i < 4; i++) {
        TrackmapArt[i]->GetObjData()->Pos.x += adjustment;
    }

    mPlayerCarIndicator->GetObjData()->Pos.x += adjustment;
    mPlayerCarIndicator->GetObjData()->Pos.y = mTrackMapCentre.y;
}

void Minimap::UpdateMiniMapItems() {
    for (MiniMapItem *item = StaticMiniMapItems.GetHead(); item != StaticMiniMapItems.EndOfList(); item = item->GetNext()) {
        if (item->bHidden) {
            FEngSetInvisible(item->pIcon);
        } else {
            FEngSetVisible(item->pIcon);
            bVector2 dir(0.0f, 1.0f);
            UpdateElementArt(&item->ItemPosition, &dir, item->pIcon, false);
            FEngSetRotationZ(item->pIcon, 0.0f);
        }
    }
}

void Minimap::InitStaticMiniMapItems() {}

void Minimap::UpdateGameplayIcons(IPlayer *player) {
    int iconsPlaced[GIcon::kType_Count];
    GIcon *sortedIcons[200];

    FEngSetInvisible(mGPSSelectionElementArt);
    bMemSet(iconsPlaced, 0, sizeof(iconsPlaced));

    int numIcons = GManager::Get().GatherVisibleIcons(sortedIcons, player);
    for (int onIcon = 0; onIcon < numIcons; onIcon++) {
        GIcon *icon = sortedIcons[onIcon];
        GIcon::Type iconType = icon->GetType();
        GameplayIconInfo &iconInfo = kGameplayIconInfo[iconType];
        FEImage *image;

        if (iconInfo.mItemType != 0 && static_cast<unsigned int>(iconsPlaced[iconType]) < 8) {
            if (FEDatabase->GetGameplaySettings()->IsMapItemEnabled(static_cast<eWorldMapItemType>(iconInfo.mItemType))) {
                image = mGameplayIcons[iconType][static_cast<unsigned int>(iconsPlaced[iconType]++)];
                if (image) {
                    UpdateIconElement(image, icon);
                }
            }
        }
    }

    for (int onType = 0; onType < GIcon::kType_Count; onType++) {
        for (int onHideIcon = iconsPlaced[onType]; static_cast<unsigned int>(onHideIcon) < 8; onHideIcon++) {
            if (mGameplayIcons[onType][onHideIcon]) {
                FEngSetInvisible(mGameplayIcons[onType][onHideIcon]);
            }
        }
    }
}

