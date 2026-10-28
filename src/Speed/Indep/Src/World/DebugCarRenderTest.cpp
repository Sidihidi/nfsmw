// Prueba de render de coches: crea coches de policia en fila delante del jugador. Solo existe en las dos PS2 (la
// A124 y SLUS): el .mdebug de la A124 da este fichero entre OnlineManager.cpp y DebugVehicleSelection.cpp, y los
// stabs dan sus funciones, sus estaticos y las variables de ajuste. GameCube, PC y Xbox 360 no lo tienen.
#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/ICollisionBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/IRigidBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Physics/PVehicle.h"

bool CarRenderTestEnable = false;
unsigned int CarRenderTestNum = 5;
float CarRenderTestDistance = 5.0f;

// Un espacio de nombres y no una clase: sus funciones sin argumentos llevan la `v' del mangling de funcion libre
// (Render__18DebugCarRenderTestv) y los stabs no le dan ni tipo ni `this'.
namespace DebugCarRenderTest {

void Init();
void CreateAllCars();
ISimable *CreateACop(const UMath::Vector3 &position, float degrees);
void KillAllCars();
void Render();

static unsigned int lastCarRenderTestNum = 0;
static ISimable *testCars[20];

}; // namespace DebugCarRenderTest

void DebugCarRenderTest::Init() {
    for (unsigned int ix = 0; ix < 20; ix++) {
        testCars[ix] = nullptr;
    }
}

void DebugCarRenderTest::CreateAllCars() {
    UMath::Vector3 playerPosition = IPlayer::First(PLAYER_LOCAL)->GetSimable()->GetPosition();
    unsigned int carNum = CarRenderTestNum;

    if (carNum > 20) {
        carNum = 20;
    }

    for (unsigned int ix = 0; ix < carNum; ix++) {
        testCars[ix] = CreateACop(playerPosition, 0.0f);
        if (testCars[ix]) {
            ICollisionBody *body;

            if (testCars[ix]->QueryInterface(&body)) {
                body->DisableModeling();
            }
        }
    }
}

ISimable *DebugCarRenderTest::CreateACop(const UMath::Vector3 &position, float degrees) {
    UMath::Vector3 angle;

    angle.x = UMath::Cosa(DEG2ANGLE(degrees));
    angle.z = -UMath::Sina(DEG2ANGLE(degrees));
    angle.y = 0.0f;
    VehicleParams parameters(nullptr, DRIVER_NONE, Attrib::StringToKey("copmidsize"), angle, position, VPF_SNAP_TO_GROUND, nullptr, nullptr);
    return ISimable::CreateInstance("PVehicle", parameters);
}

void DebugCarRenderTest::KillAllCars() {
    for (unsigned int ix = 0; ix < 20; ix++) {
        ISimable *car = testCars[ix];

        if (car) {
            car->Kill();
            testCars[ix] = nullptr;
        }
    }
}

void DebugCarRenderTest::Render() {
    if (!CarRenderTestEnable && lastCarRenderTestNum) {
        KillAllCars();
        lastCarRenderTestNum = 0;
        return;
    }
    if (!CarRenderTestEnable) {
        return;
    }

    UMath::Vector3 playerPosition = IPlayer::First(PLAYER_LOCAL)->GetSimable()->GetPosition();
    UMath::Vector3 playerForwardVec;
    IPlayer::First(PLAYER_LOCAL)->GetSimable()->GetRigidBody()->GetForwardVector(playerForwardVec);

    if (CarRenderTestNum != lastCarRenderTestNum) {
        KillAllCars();
        CreateAllCars();
        lastCarRenderTestNum = CarRenderTestNum;
        return;
    }

    UMath::Vector3 carPosition = playerPosition;
    float spreadDistance = CarRenderTestDistance;

    for (unsigned int ix = 0; ix < 20; ix++) {
        ISimable *car = testCars[ix];

        if (car) {
            IRigidBody *rigidBody = car->GetRigidBody();

            if (rigidBody) {
                carPosition.x = playerPosition.x + playerForwardVec.x * spreadDistance;
                carPosition.y = playerPosition.y + playerForwardVec.y * spreadDistance;
                carPosition.z = playerPosition.z + playerForwardVec.z * spreadDistance;
                spreadDistance += CarRenderTestDistance;
                rigidBody->SetPosition(carPosition);
            }
        }
    }
}
#endif
