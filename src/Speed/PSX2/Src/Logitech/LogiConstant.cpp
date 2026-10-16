#include "Speed/PSX2/Src/Logitech/LogiForce.h"
#include "Speed/PSX2/Src/Logitech/liblgdev.h"

#include <string.h>

LogiConstant::LogiConstant() {}

int LogiConstant::DownloadForce(int channel, int forceNumber, int &handle, int duration, int startDelay, int magnitude, int direction,
                                int attackTime, int fadeTime, int attackLevel, int fadeLevel) {
    lgDevForceEffect force;
    int ret = 0;

    if (EffectID[channel][forceNumber] != -1) {
        Destroy(channel, forceNumber);
    }

    if (handle != -1) {
        memset(&force, 0, sizeof(force));
        force.Type = 0;
        force.Duration = duration;
        force.StartDelay = startDelay;
        force.p.constant.Magnitude = magnitude;
        force.p.constant.Direction = direction;
        force.p.constant.envelope.AttackTime = attackTime;
        force.p.constant.envelope.AttackLevel = attackLevel;
        force.p.constant.envelope.FadeTime = fadeTime;
        force.p.constant.envelope.FadeLevel = fadeLevel;

        ret = lgDevDownloadForceEffect(handle, &EffectID[channel][forceNumber], &force);
        if (ret < 0) {
            EffectID[channel][forceNumber] = -1;
        }
    }

    return ret;
}

int LogiConstant::UpdateForce(int channel, int forceNumber, int duration, int startDelay, int magnitude, int direction, int attackTime,
                              int fadeTime, int attackLevel, int fadeLevel) {
    lgDevForceEffect force;
    int ret = 0;

    memset(&force, 0, sizeof(force));
    force.Type = 0;
    force.Duration = duration;
    force.StartDelay = startDelay;
    force.p.constant.Magnitude = magnitude;
    force.p.constant.Direction = direction;
    force.p.constant.envelope.AttackTime = attackTime;
    force.p.constant.envelope.AttackLevel = attackLevel;
    force.p.constant.envelope.FadeTime = fadeTime;
    force.p.constant.envelope.FadeLevel = fadeLevel;

    lgDevASync(0, &ret);
    lgDevAUpdateForceEffect(EffectID[channel][forceNumber], &force);
    if (ret < 0) {
        EffectID[channel][forceNumber] = -1;
    }

    return ret;
}
