#include "Speed/PSX2/Src/Logitech/LogiForce.h"
#include "Speed/PSX2/Src/Logitech/liblgdev.h"

#include <string.h>

LogiPeriodic::LogiPeriodic() {}

int LogiPeriodic::DownloadForce(int channel, int forceNumber, int &handle, int type, int duration, int startDelay, int magnitude,
                                int direction, int period, int phase, int offset, int attackTime, int fadeTime, int attackLevel,
                                int fadeLevel) {
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
        force.p.periodic.Magnitude = magnitude;
        force.p.periodic.Direction = direction;
        force.p.periodic.Period = period;
        force.p.periodic.Phase = phase;
        force.p.periodic.Offset = offset;
        force.p.periodic.envelope.AttackTime = attackTime;
        force.p.periodic.envelope.FadeTime = fadeTime;
        force.p.periodic.envelope.AttackLevel = attackLevel;
        force.p.periodic.envelope.FadeLevel = fadeLevel;

        ret = lgDevDownloadForceEffect(handle, &EffectID[channel][forceNumber], &force);
        if (ret < 0) {
            EffectID[channel][forceNumber] = -1;
        }
    }

    return ret;
}

int LogiPeriodic::UpdateForce(int channel, int forceNumber, int type, int duration, int startDelay, int magnitude, int direction,
                              int period, int phase, int offset, int attackTime, int fadeTime, int attackLevel, int fadeLevel) {
    lgDevForceEffect force;
    int ret = 0;

    memset(&force, 0, sizeof(force));
    force.Type = type;
    force.Duration = duration;
    force.StartDelay = startDelay;
    force.p.periodic.Magnitude = magnitude;
    force.p.periodic.Direction = direction;
    force.p.periodic.Period = period;
    force.p.periodic.Phase = phase;
    force.p.periodic.Offset = offset;
    force.p.periodic.envelope.AttackTime = attackTime;
    force.p.periodic.envelope.FadeTime = fadeTime;
    force.p.periodic.envelope.AttackLevel = attackLevel;
    force.p.periodic.envelope.FadeLevel = fadeLevel;

    lgDevASync(0, &ret);
    lgDevAUpdateForceEffect(EffectID[channel][forceNumber], &force);
    if (ret < 0) {
        EffectID[channel][forceNumber] = -1;
    }

    return ret;
}
