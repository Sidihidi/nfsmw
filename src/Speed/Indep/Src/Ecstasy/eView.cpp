#include "./Ecstasy.hpp"
#include "EcstasyData.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Ecstasy/eLight.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

eView eViews[NUM_EVIEWS];

eView::eView() {
    bMemSet(this, 0, sizeof(*this));
    this->CameraMoverList.InitList();
    this->FovBias = 1.0f;
    this->SetID(this - eViews);
    this->SetPlatInfo(eViewPlatInterface::GimmeMyViewPlatInfo(this->ID));
    this->PixelMinSize = 4;
    this->ScreenEffects = nullptr;
    this->Precipitation = nullptr;
    this->facePixelation = nullptr;
}

#ifdef EA_PLATFORM_PLAYSTATION2
// Solo el ejecutable de PS2 tiene el destructor (en GameCube y PC no queda).
eView::~eView() {}
#endif

eVisibleState eView::GetVisibleState(const bVector3 *aabb_min, const bVector3 *aabb_max, bMatrix4 *local_world) {
    eVisibleState vis_state = this->GetVisibleStateSB(aabb_min, aabb_max, local_world);

    return vis_state;
}

eVisibleState eView::GetVisibleState(eModel *model, bMatrix4 *local_world) {
    bVector3 min;
    bVector3 max;
    model->GetBoundingBox(&min, &max);
    return this->GetVisibleState(&min, &max, local_world);
}

int eView::GetPixelSize(float radius, float distance) {
    float pixel_size;

    if (distance > radius / 2.0f) {
        pixel_size = (radius * this->H) / distance;
    } else {
        pixel_size = 2.0f * this->H;
    }
    return bFloatToInt(pixel_size);
}

int eView::GetPixelSize(const bVector3 *position, float radius) {
    Camera *camera = this->GetCamera();
    bVector3 *cam_position = camera->GetPosition();
    bVector3 *cam_direction = camera->GetDirection();
    float dir_x = position->x - cam_position->x;
    float dir_y = position->y - cam_position->y;
    float dir_z = position->z - cam_position->z;
    float pixel_size;

    if (dir_x * cam_direction->x + dir_y * cam_direction->y + dir_z * cam_direction->z < -radius) {
        return 0;
    } else {
        float distance_ahead = bSqrt(dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);
        float distance_away = distance_ahead - radius;
        if (distance_away > radius / 2) {
#ifdef EA_PLATFORM_WIN32 // speed.exe divide antes de multiplicar ((H/d)*r: fdivr + fmul); GameCube casa con (r*H)/d
            pixel_size = (this->H / distance_away) * radius;
#else
            pixel_size = (radius * this->H) / distance_away;
#endif
        } else {
            pixel_size = 2 * this->H;
        }
    }
    return bFloatToInt(pixel_size);
}

int eView::GetPixelSize(const bVector3 *bbox_min, const bVector3 *bbox_max) {
    Camera *camera = this->GetCamera();
    bVector3 *cam_position = camera->GetPosition();
    bVector3 *cam_direction = camera->GetDirection();

    float rad_x = (bbox_min->x + bbox_max->x) * 0.5f;
    float rad_y = (bbox_min->y + bbox_max->y) * 0.5f;
    float rad_z = (bbox_min->z + bbox_max->z) * 0.5f;
    float pos_x = bbox_min->x - bbox_max->x;
    float pos_y = bbox_min->y - bbox_max->y;
    float pos_z = bbox_min->z - bbox_max->z;

    float radius = bSqrt(pos_x * pos_x + pos_y * pos_y + pos_z * pos_z) * 0.5f;

    float dir_x = rad_x - cam_position->x;
    float dir_y = rad_y - cam_position->y;
    float dir_z = rad_z - cam_position->z;
    float pixel_size;

    if (dir_x * cam_direction->x + dir_y * cam_direction->y + dir_z * cam_direction->z < -radius) {
        return 0;
    } else {
        float distance_ahead = bSqrt(dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);
        float distance_away = distance_ahead - radius;
        pixel_size = this->H;
        if (distance_away > radius) {
#ifdef EA_PLATFORM_WIN32 // como en el GetPixelSize de (posicion, radio): speed.exe divide antes de multiplicar
            pixel_size = (this->H / distance_away) * radius;
#else
            pixel_size = (radius * this->H) / distance_away;
#endif
        }
    }
    return bFloatToInt(pixel_size);
}

void eView::BiasMatrixForZSorting(bMatrix4 *pL2W, float zBias) {
    if (this->pCamera) {
        bMatrix4 m;
        bIdentity(&m);

        bVector3 *pObjPosition = reinterpret_cast<bVector3 *>(&pL2W->v3);
        bVector3 *pCamPosition = this->pCamera->GetPosition();
        m.v0.x = zBias;
        m.v1.y = zBias;
        m.v2.z = zBias;

        bVector3 v = *pObjPosition - *pCamPosition;
        v *= zBias;
        v += *pCamPosition;
        *reinterpret_cast<bVector3 *>(&m.v3) = v;
        m.v3.w = 1.0f;
        bFill(&pL2W->v3, 0.0f, 0.0f, 0.0f, 1.0f);
        eMulMatrix(pL2W, pL2W, &m);
    }
}

// El jugador de una vista (0 o 1): la principal, el retrovisor, el reflejo y el brillo de cada uno.
int eView::GetPlayerNumFromViewID() {
    switch (ID) {
    case EVIEW_PLAYER1:
    case EVIEW_PLAYER1_RVM:
#ifdef EA_PLATFORM_PLAYSTATION2
    case EVIEW_PLAYER1_REFLECTION:
    case EVIEW_PLAYER1_GLOW:
#endif
        return 0;
    case EVIEW_PLAYER2:
#ifdef EA_PLATFORM_PLAYSTATION2
    case EVIEW_PLAYER2_REFLECTION:
    case EVIEW_PLAYER2_GLOW:
#endif
        return 1;
    }

    return 0;
}

void eView::AttachCameraMover(CameraMover *camera_mover) {
    this->CameraMoverList.AddHead(camera_mover);
    eUpdateViewMode();
}

void eView::UnattachCameraMover(CameraMover *camera_mover) {
    this->CameraMoverList.Remove(camera_mover);
    eUpdateViewMode();
}

#ifdef EA_PLATFORM_PLAYSTATION2
extern bVector3 SunPosition;

// Solo PS2: el contexto de luces del mundo de la vista, en el bufer del cuadro. Las luces se colocan desde un punto
// algo detras y por encima de la camara, y la matriz del sol mira desde SunPosition al origen.
void eView::SetupWorldLightContext() {
    this->WorldLightContext = nullptr;
    Camera *camera = GetCamera();
    if (!camera) {
        return;
    }

    this->WorldLightContext = static_cast<eDynamicLightContext *>(eFrameMalloc(sizeof(eDynamicLightContext)));
    if (!this->WorldLightContext) {
        return;
    }

    bVector4 camera_world_position;
    bVector3 camera_dir;
    bVector3 camera_up;
    bVector3 camera_backup;
    float backup_distance = 10.0f;
    float backup_z = 4.0f;
    bMatrix4 *camera_matrix = camera->GetCameraMatrix();

    camera_up.x = camera_matrix->v0.y;
    camera_up.y = camera_matrix->v1.y;
    camera_up.z = camera_matrix->v2.y;

    camera_dir.x = camera_matrix->v0.z;
    camera_dir.y = camera_matrix->v1.z;
    camera_dir.z = camera_matrix->v2.z;

    camera_backup.x = camera_dir.x;
    camera_backup.y = camera_dir.y;
    camera_backup.z = camera_dir.z;

    camera_world_position.x = camera->GetPosition()->x - camera_backup.x * backup_distance;
    camera_world_position.y = camera->GetPosition()->y - camera_backup.y * backup_distance;
    camera_world_position.z = camera->GetPosition()->z - camera_backup.z * backup_distance + backup_z;
    camera_world_position.w = 1.0f;

    // El .mdebug de la A124 expande aqui el inline de eMath.hpp (eGetIdentityMatrix): la forma es esta y los dos
    // `lui` permutados (a2-identity antes que a1-shaper) son orden del planificador, sin oraculo para otra forma.
    elSetupLightContext(this->WorldLightContext, &ShaperLightsDefault, eGetIdentityMatrix(), &hack_man_matrix,
                        &camera_world_position, this);

    bMatrix4 sun_matrix;
    bMatrix4 sun_scale;
    bVector3 world_position(0.0f, 0.0f, 0.0f);
    bVector3 world_up(0.0f, 0.0f, 1.0f);

    eCreateLookAtMatrix(&sun_matrix, SunPosition, world_position, world_up);
    bIdentity(&sun_scale);
    sun_scale.v0.x = 1.0f;
    sun_scale.v1.y = 1.0f;
    sun_scale.v2.z = 1.0f;
    eMulMatrix(&sun_matrix, &sun_scale, &sun_matrix);
    bCopy(&this->WorldLightContext->LocalLightPositions, &sun_matrix);
}
#endif
