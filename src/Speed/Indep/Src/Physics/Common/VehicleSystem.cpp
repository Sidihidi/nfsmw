#include "VehicleSystem.h"
#include "Speed/Indep/Src/Sim/SimSubSystem.h"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Libs/Support/Utility/FastMem.h"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#endif

namespace VehicleSystem {

// La A124 es una foto anterior: VehicleSystem aun guardaba los nombres de las fisicas de coche y muchos ajustes que
// las versiones finales ya no tienen (stabs de la A124 en 0x61C754..0x61C7C0; Shutdown los libera).
#ifdef EA_BUILD_A124
typedef char *StringPtr;

struct NameToIndexMap : public UTL::Std::map<unsigned int, unsigned int, _type_map> {
    USE_FASTALLOC(NameToIndexMap)
};

static NameToIndexMap *gCarPhysicsNameMap = nullptr;
static StringPtr *gCarPhysicsNames = nullptr;
static unsigned int gCarPhysicsNameCount = 0;

int CAR_LASER_LIFESPAN = 120;
float CRASH_RESET_COUNT = 2.0f;
float CRASH_RESET_COUNT_BIKE = 1.25f;
float TRAFFIC_OPT_DISTANCE_SQUARE = 2500.0f;
unsigned int COLLISION_LANDING_DELAY = 50;
int LANDING_ASSIST_ENABLE_COUNT = 10;
#endif

float ENABLE_ROLL_STOPS_THRESHOLD = 0.2f;

#ifdef EA_BUILD_A124
float BASE_FRICTION_MASS = 8.0f;
int ENABLE_TWO_WHEEL_TESTS = 0;
int ENABLE_BOOST_TESTS = 0;
float SHRED_DRAG = 2.0f;
int ENABLE_COLOR_CHECK = 0;
#endif

float PAD_DEAD_ZONE = 0.05f;

#ifdef EA_BUILD_A124
float SKID_AUDIO_SCALE = 0.15f;
float ROLLING_RESISTANCE = 0.1f;
float MIN_BUTTON_VALUE = 0.1f;
float WHEEL_SPIN_EXTRA_RPM = 4000.0f;
float MAXSPEED_REVERSELIGHTS = 0.3f;
float DAMAGE_SCALE_COLLISION = 1.0f;
int EMP_LIFETIME = 120;
float TYRE_DAMAGE_RADIUS = 1.25f;
float MAX_WHEEL_SPIN_RATE_AI = 0.05f;
int POST_BRAKE_ACCEL_COUNT = 160;
float POST_BRAKE_ACCEL_SCALE = 0.2f;
float POST_BRAKE_ACCEL_MIN = 0.1f;
#endif

static void InitializeVehicleGlobals() {}

static void InitializeGlobals() {}

static void Init() {
    InitializeGlobals();
    InitializeVehicleGlobals();
}

static void Shutdown() {
#ifdef EA_BUILD_A124
    if (gCarPhysicsNames) {
        delete[] gCarPhysicsNames;
    }
    if (gCarPhysicsNameMap) {
        delete gCarPhysicsNameMap;
    }
#endif
}

}; // namespace VehicleSystem

BIND_SIM_SUBSYSTEM(VehicleSystem, VehicleSystem::Init, VehicleSystem::Shutdown);
