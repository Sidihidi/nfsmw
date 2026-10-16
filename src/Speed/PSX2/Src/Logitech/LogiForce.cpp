#include "Speed/PSX2/Src/Logitech/LogiForce.h"
#include "Speed/PSX2/Src/Logitech/liblgdev.h"

LogiForce::LogiForce() {
    InitVars();
}

void LogiForce::InitVars() {
    for (int channel = 0; channel < 2; channel++) {
        for (int forceNumber = 0; forceNumber < 8; forceNumber++) {
            Playing[channel][forceNumber] = false;
            EffectID[channel][forceNumber] = -1;
        }
    }
}

int LogiForce::Start(int channel, int forceNumber) {
    int ret = 0;

    if (EffectID[channel][forceNumber] != -1) {
        lgDevASync(0, &ret);
        lgDevAStartForceEffect(EffectID[channel][forceNumber]);

        if (ret >= 0) {
            Playing[channel][forceNumber] = true;
        }
    } else {
        ret = 0;
    }

    return ret;
}

int LogiForce::Stop(int channel, int forceNumber) {
    int ret = 0;

    if (EffectID[channel][forceNumber] != -1) {
        lgDevASync(0, &ret);
        lgDevAStopForceEffect(EffectID[channel][forceNumber]);

        if (ret >= 0) {
            Playing[channel][forceNumber] = false;
        }
    } else {
        ret = 0;
    }

    return ret;
}

int LogiForce::Destroy(int channel, int forceNumber) {
    int ret = 0;

    if (EffectID[channel][forceNumber] != -1) {
        ret = lgDevDestroyForceEffect(EffectID[channel][forceNumber]);

        if (ret >= 0) {
            EffectID[channel][forceNumber] = -1;
            Playing[channel][forceNumber] = false;
        }
    }

    return ret;
}
