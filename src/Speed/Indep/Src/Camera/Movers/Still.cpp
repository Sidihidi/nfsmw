#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

// TU de solo datos: los tweaks de la camara Still. Sus tres globales viven en
// .bss entre gPhoto_CarPosBias (Showcase.cpp) y vIceAccelLagMin (ICEMover.cpp).

static bVector3 StillEyeTweak(0.0f, -80.0f, 0.0f); // .bss: 0x8045AF78
static bVector3 StillLookTweak(0.0f, 0.0f, 0.0f);  // .bss: 0x8045AF88
static bVector3 StillUpTweak(0.0f, 0.0f, 1.0f);    // .bss: 0x8045AF98

// El constructor no esta en ningun binario (GameCube lo compila y lo estripa; las PS2 no lo enlazan): va declarado.
class StillCameraMover : public CameraMover {
  public:
    StillCameraMover(int view_id);
    ~StillCameraMover() override {}

    void SetEye(const bVector3 *eye) {
        Eye = *eye;
    }

    void SetLook(const bVector3 *look) {
        Look = *look;
    }

    void SetUp(bVector3 *up) {
        Up = *up;
    }

    void SetFOV(unsigned short fov) {
        FOV = fov;
    }

    void Update(float dT) override;

  private:
    bVector3 Eye;       // offset 0x80, size 0x10
    bVector3 Look;      // offset 0x90, size 0x10
    bVector3 Up;        // offset 0xA0, size 0x10
    unsigned short FOV; // offset 0xB0, size 0x2
};

void StillCameraMover::Update(float dT) {
    bMatrix4 m;
    bVector3 eye = Eye;
    bVector3 look = Look;
    bVector3 up = Up;
    unsigned short fov = FOV;

    eCreateLookAtMatrix(&m, eye, look, up);

    pCamera->SetFieldOfView(fov);
    pCamera->SetCameraMatrix(m, dT);
}
