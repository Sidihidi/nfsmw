#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Misc/Spline.hpp"
#include "Speed/Indep/Src/Misc/Table.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

// El TU original solo aporta datos: ninguna de estas tablas se lee desde
// zCamera (solo aparecen en __static_initialization_and_destruction_0), asi que
// tTable<bVector3>::Blend nunca se instancia.

// Que camara de persecucion usa la vista de policia (data de la A124: 0x5E0AB0 = 2; GameCube la estripa).
enum eChaseCam {
    eCHASE_CAM_NONE = 0,
    eCHASE_CAM_PIVOT = 1,
    eCHASE_CAM_PARAMETRIC = 2,
    eCHASE_CAM_PULLBACK = 3,
};

eChaseCam nWhichChaseCamera = eCHASE_CAM_PARAMETRIC;

static bVector3 vCopViewPivot(1.0f, 0.0f, 2.0f); // .bss: 0x8045AE04

static bVector3 vCopViewDistanceFovBand[2] = {
    bVector3(1.5f, 12000.0f, 0.5f), //
    bVector3(1.2f, 6000.0f, 0.5f),  //
}; // .bss: 0x8045AE14

static tTable<bVector3> tCopViewDistanceFovBand(vCopViewDistanceFovBand, 2, 0.0f, 1.0f); // .bss: 0x8045AE34

static bVector3 vCopViewPoints[5] = {
    bVector3(-2.0f, -3.0f, 1.2f),  //
    bVector3(-2.0f, -2.5f, 1.3f),  //
    bVector3(-2.0f, 0.0f, 1.5f),   //
    bVector3(-2.0f, 2.5f, 1.3f),   //
    bVector3(-2.0f, 3.0f, 1.2f),   //
}; // .bss: 0x8045AE48

static bVector2 vCopViewDistanceFov[2] = {
    bVector2(2.0f, 12000.0f), //
    bVector2(6.0f, 6000.0f),  //
}; // .bss: 0x8045AE98

static tTable<bVector3> tCopViewPosition(vCopViewPoints, 5, 0.0f, 1.0f); // .bss: 0x8045AEA8

static tTable<bVector2> tCopViewDistanceFov(vCopViewDistanceFov, 2, 0.0f, 1.0f); // .bss: 0x8045AEBC

// El constructor no esta en ningun binario (las tres versiones lo estripan al enlazar) y SetDesired y FindParameters
// leen la clase Car vestigial, que no tiene cuerpo en este arbol: van declaradas. El destructor y Update estan en las
// dos PS2 (CopView.cpp:117-119 y 283-312); GameCube las compila y las estripa (Range 0xFFFFFFFF en su DWARF).
class CopViewCameraMover : public CameraMover {
  public:
    CopViewCameraMover(int n_view, Car *p_eye_car, Car *p_look_car);
    ~CopViewCameraMover() override;

  private:
    void FindParameters(float *p_angle_param, float *p_distance_param);
    void SetDesired(bMatrix4 *p_eye_car_to_world, bMatrix4 *p_world_to_eye_car, bMatrix4 *p_look_car_to_world,
                    bMatrix4 *p_world_to_look_car);

  public:
    void Update(float dT) override;

  private:
    Car *pEyeCar;    // offset 0x80, size 0x4
    Car *pLookCar;   // offset 0x84, size 0x4
    tCubic1D *pFov;  // offset 0x88, size 0x4
    tCubic3D *pEye;  // offset 0x8C, size 0x4
    tCubic3D *pLook; // offset 0x90, size 0x4
};

void CopViewCameraMover::FindParameters(float *p_angle_param, float *p_distance_param) {
    bMatrix4 m_look_to_world;
    bMatrix4 m_world_to_look;

    pLookCar->MakeFlatMatrix(&m_look_to_world);
    eInvertTransformationMatrix(&m_world_to_look, &m_look_to_world);

    bVector3 v_ray;

    bMulMatrix(&v_ray, &m_world_to_look, pEyeCar->GetPosition());
}

void CopViewCameraMover::SetDesired(bMatrix4 *p_eye_car_to_world, bMatrix4 *p_world_to_eye_car, bMatrix4 *p_look_car_to_world,
                                    bMatrix4 *p_world_to_look_car) {
    bMatrix4 m_eye_car_to_world;
    bMatrix4 m_world_to_eye_car;
    bMatrix4 m_world_to_look_car;
    bMatrix4 m_look_car_to_world;
    bVector3 v_eye;
    bVector3 v_look;

    bCopy(&m_look_car_to_world, pLookCar->GetBodyMatrix(), pLookCar->GetPosition());
    eInvertTransformationMatrix(&m_world_to_look_car, &m_look_car_to_world);

    float a_fov = 12000.0f;

    switch (nWhichChaseCamera) {
        case eCHASE_CAM_PIVOT: {
            float f_width_param;
            float f_distance_param;

            pEyeCar->MakeFlatMatrix(&m_eye_car_to_world);
            eInvertTransformationMatrix(&m_world_to_eye_car, &m_eye_car_to_world);

            FindParameters(&f_width_param, &f_distance_param);

            bVector3 v_dist_fov_band;
            tCopViewDistanceFovBand.GetValue(&v_dist_fov_band, f_distance_param);
            float f_look_to_eye = v_dist_fov_band.x * bDistBetween(pEyeCar->GetPosition(), pLookCar->GetPosition());

            v_look = bVector3(0.0f, 0.0f, 0.0f);
            bMulMatrix(&v_look, &m_look_car_to_world, &v_look);

            bVector3 v_chaser;
            bMulMatrix(&v_chaser, &m_eye_car_to_world, &vCopViewPivot);

            bNormalize(&v_eye, pEyeCar->GetVelocity(), -f_look_to_eye);
            v_eye += v_look;
            v_eye.z = bMax(v_eye.z, v_chaser.z);

            bVector3 v_eye_to_look = v_look - v_eye;
            bVector3 v_eye_to_chaser = v_chaser - v_eye;
            bNormalize(&v_eye_to_look, &v_eye_to_look);
            bNormalize(&v_eye_to_chaser, &v_eye_to_chaser);

            unsigned short a_look_eye_car = bACos(bDot(&v_eye_to_look, &v_eye_to_chaser));
            unsigned short a_fov_band = bFloatToInt(v_dist_fov_band.y * v_dist_fov_band.z);

            if (a_look_eye_car > a_fov_band) {
                bVector3 v_look_to_eye = -v_eye_to_look;
                bVector3 v_look_to_chaser = v_chaser - v_look;
                float f_look_to_chaser = bLength(&v_look_to_chaser);
                float f_look_to_chaser_recip = 1.0f / f_look_to_chaser;
                bScale(&v_look_to_chaser, &v_look_to_chaser, f_look_to_chaser_recip);
                float f_dot_eye_look_car = bDot(&v_look_to_eye, &v_look_to_chaser);
                unsigned short a_eye_look_car = bACos(f_dot_eye_look_car);
                unsigned short a_off_center =
                    bASin(f_look_to_eye * bSin(a_fov_band) * f_look_to_chaser_recip) - a_fov_band;
                unsigned short a_to_rotate = a_eye_look_car - a_off_center;

                f_look_to_eye = f_look_to_chaser * f_dot_eye_look_car;
                bScaleAdd(&v_eye, &v_look, &v_look_to_eye, f_look_to_eye);
                v_eye_to_chaser = v_chaser - v_eye;
                bNormalize(&v_eye_to_chaser, &v_eye_to_chaser, f_look_to_eye * bTan(a_to_rotate));
                v_eye += v_eye_to_chaser;
                v_look_to_eye = v_eye - v_look;
                bNormalize(&v_look_to_eye, &v_look_to_eye, f_look_to_eye);
                v_eye = v_look + v_look_to_eye;
            }

            bMulMatrix(&v_eye, &m_world_to_eye_car, &v_eye);
            bMulMatrix(&v_look, &m_world_to_look_car, &v_look);
            a_fov = v_dist_fov_band.y;
            break;
        }

        case eCHASE_CAM_PARAMETRIC: {
            bVector3 v_up(0.0f, 0.0f, 1.0f);
            bVector3 v_left;
            bVector3 v_forward = *pLookCar->GetPosition() - *pEyeCar->GetPosition();

            bCross(&v_left, &v_up, &v_forward);
            bCross(&v_up, &v_forward, &v_left);
            bNormalize(&v_up, &v_up);
            bNormalize(&v_left, &v_left);
            bNormalize(&v_forward, &v_forward);

            bCopy(&m_eye_car_to_world.v0, &v_forward, 0.0f);
            bCopy(&m_eye_car_to_world.v1, &v_left, 0.0f);
            bCopy(&m_eye_car_to_world.v2, &v_up, 0.0f);
            bCopy(&m_eye_car_to_world.v3, pEyeCar->GetPosition(), 1.0f);
            eInvertTransformationMatrix(&m_world_to_eye_car, &m_eye_car_to_world);

            bVector2 v_dist;
            float f_width_param;
            float f_distance_param;

            FindParameters(&f_width_param, &f_distance_param);
            tCopViewPosition.GetValue(&v_eye, f_width_param);
            tCopViewDistanceFov.GetValue(&v_dist, f_distance_param);
            bMulMatrix(&v_look, &m_world_to_eye_car, pLookCar->GetPosition());

            bVector3 v_aim = v_eye - v_look;
            bNormalize(&v_aim, &v_aim, v_dist.x);
            v_look = bVector3(0.0f, 0.0f, 0.0f);
            v_eye += v_aim;

            bMatrix4 m_eye_to_look;
            bVector3 v_eye_in_look_space;
            bVector3 v_eye_car_in_look_space;

            bMulMatrix(&m_eye_to_look, &m_world_to_look_car, &m_eye_car_to_world);
            bMulMatrix(&v_eye_in_look_space, &m_eye_to_look, &v_eye);
            bMulMatrix(&v_eye_car_in_look_space, &m_eye_to_look, pEyeCar->GetPosition());

            if (v_eye_in_look_space.y * v_eye_car_in_look_space.y < 0.0f) {
                bMatrix4 m_look_to_eye;

                bMulMatrix(&m_look_to_eye, &m_world_to_eye_car, &m_look_car_to_world);
                v_eye_in_look_space.y = 0.0f;
                bMulMatrix(&v_eye, &m_look_to_eye, &v_eye_in_look_space);
            }
            a_fov = v_dist.y;
            break;
        }
    }

    pFov->SetValDesired(a_fov);
    pEye->SetValDesired(&v_eye);
    pLook->SetValDesired(&v_look);

    if (p_eye_car_to_world) {
        bCopy(p_eye_car_to_world, &m_eye_car_to_world);
    }
    if (p_world_to_eye_car) {
        bCopy(p_world_to_eye_car, &m_world_to_eye_car);
    }
    if (p_look_car_to_world) {
        bCopy(p_look_car_to_world, &m_look_car_to_world);
    }
    if (p_world_to_look_car) {
        bCopy(p_world_to_look_car, &m_world_to_look_car);
    }
}

CopViewCameraMover::~CopViewCameraMover() {
    delete pFov;
    delete pEye;
    delete pLook;
}

void CopViewCameraMover::Update(float dT) {
    if (IsGameFlowPaused()) {
        return;
    }

    bMatrix4 m_eye_car_to_world;
    bMatrix4 m_look_car_to_world;

    pFov->SetDuration(0.7f);
    pEye->SetDuration(0.7f);
    pLook->SetDuration(0.7f);

    SetDesired(&m_eye_car_to_world, nullptr, &m_look_car_to_world, nullptr);

    pFov->Update(dT, 0.0f, 0.0f);
    pEye->Update(dT, 0.0f, 0.0f);
    pLook->Update(dT, 0.0f, 0.0f);

    bVector3 v_up(0.0f, 0.0f, 1.0f);
    bVector3 v_eye(pEye->x.Val, pEye->y.Val, pEye->z.Val);
    bVector3 v_look(pLook->x.Val, pLook->y.Val, pLook->z.Val);

    bMulMatrix(&v_eye, &m_eye_car_to_world, &v_eye);
    bMulMatrix(&v_look, &m_look_car_to_world, &v_look);

    bMatrix4 m_world_to_camera;
    eCreateLookAtMatrix(&m_world_to_camera, v_eye, v_look, v_up);

    pCamera->SetFieldOfView(bFloatToInt(pFov->Val));
    pCamera->SetCameraMatrix(m_world_to_camera, dT);
}
