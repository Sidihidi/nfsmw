#include "Speed/Indep/Src/AI/AIAction.h"

IMPLEMENT_FACTORY(AIAction);

AIAction::AIAction(AIActionParams *params, float score)
    : Sim::Object(0),                //
      mActionParams(*params), //
      mScore(score) {
    mActionParams.mOwner->QueryInterface(&mVehicle);
    mActionParams.mOwner->QueryInterface(&mAI);
}

void performance_limiter::init(float speed) {
    speed_limit = speed;
}

void performance_limiter::update(float speed, float maxspeed, float maxaccel, float dt) {
#ifndef EA_BUILD_A124 // la A124 solo limita hacia delante (AIAction.cpp:36-42 de sus stabs, sin la rama de marcha atras)
    if (speed > 0.0f) {
#endif

        if (speed < speed_limit) {

#ifdef EA_BUILD_A124
            float t = (speed_limit - speed) / KPH2MPS(16.0f); // 0x408E38E4 en la A124
#else
            float t = (speed_limit - speed) / KPH2MPS(5.0f);
#endif
            speed_limit -= bMin(1.0f, dt * t * t) * (speed_limit - speed);
        }

        speed_limit = bMax(speed, speed_limit) + maxaccel * dt;
        speed_limit = bMin(maxspeed, speed_limit);
#ifndef EA_BUILD_A124

    } else {


        if (speed > speed_limit) {

            float t = (speed_limit - speed) / KPH2MPS(5.0f);
            speed_limit += bMax(-1.0f, dt * t * t) * (speed_limit - speed);
        }

        speed_limit = bMin(speed, speed_limit) - maxaccel * dt;
        speed_limit = bMin(maxspeed, -speed_limit);
    }
#endif
}
