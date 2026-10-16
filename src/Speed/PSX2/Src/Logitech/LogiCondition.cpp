#include "Speed/PSX2/Src/Logitech/LogiForce.h"
#include "Speed/PSX2/Src/Logitech/liblgdev.h"

#include <string.h>

LogiCondition::LogiCondition() {}

// Los dos ejes del efecto de condicion llevan los mismos parametros: se rellena el primero y se copia.
int LogiCondition::DownloadForce(int channel, int forceNumber, int &handle, int type, int duration, int startDelay, int offset,
                                 int deadband, int satNeg, int satPos, int coeffNeg, int coeffPos) {
    lgDevForceEffect force;
    int ret = 0;

    if (EffectID[channel][forceNumber] != -1) {
        Destroy(channel, forceNumber);
    }

    if (handle != -1) {
        memset(&force, 0, sizeof(force));
        force.Type = type;
        force.Duration = duration;
        force.StartDelay = startDelay;
        force.p.condition[0].Offset = offset;
        force.p.condition[0].Deadband = deadband;
        force.p.condition[0].SaturationNeg = satNeg;
        force.p.condition[0].SaturationPos = satPos;
        force.p.condition[0].CoefficientNeg = coeffNeg;
        force.p.condition[0].CoefficientPos = coeffPos;
        force.p.condition[1] = force.p.condition[0];

        ret = lgDevDownloadForceEffect(handle, &EffectID[channel][forceNumber], &force);
        if (ret < 0) {
            EffectID[channel][forceNumber] = -1;
        }
    }

    return ret;
}

int LogiCondition::UpdateForce(int channel, int forceNumber, int type, int duration, int startDelay, int offset, int deadband,
                               int satNeg, int satPos, int coeffNeg, int coeffPos) {
    lgDevForceEffect force;
    int ret = 0;

    memset(&force, 0, sizeof(force));
    force.Type = type;
    force.Duration = duration;
    force.StartDelay = startDelay;
    force.p.condition[0].Offset = offset;
    force.p.condition[0].Deadband = deadband;
    force.p.condition[0].SaturationNeg = satNeg;
    force.p.condition[0].SaturationPos = satPos;
    force.p.condition[0].CoefficientNeg = coeffNeg;
    force.p.condition[0].CoefficientPos = coeffPos;
    force.p.condition[1] = force.p.condition[0];

    lgDevASync(0, &ret);
    lgDevAUpdateForceEffect(EffectID[channel][forceNumber], &force);
    if (ret < 0) {
        EffectID[channel][forceNumber] = -1;
    }

    return ret;
}
