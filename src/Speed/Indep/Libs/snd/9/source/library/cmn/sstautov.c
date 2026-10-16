#include "./sndcmn.h"

namespace Snd {

void StreamFader(void *pClientData) {
    StreamSourceChannelState *pSourceChannelState = (StreamSourceChannelState *)pClientData;
    SNDSTREAMCHANNEL *pSndStreamChannel;
    int offsetToStart;
    int voice;

    pSourceChannelState->vol += pSourceChannelState->fader.incrementPerUpdate;
    if (pSourceChannelState->fader.incrementPerUpdate < 0.0f) {
        if (pSourceChannelState->vol <= pSourceChannelState->fader.targetVol) {
            pSourceChannelState->vol = pSourceChannelState->fader.targetVol;
            pSourceChannelState->fader.incrementPerUpdate = 0.0f;
        }
    } else {
        if (pSourceChannelState->vol >= pSourceChannelState->fader.targetVol) {
            pSourceChannelState->vol = pSourceChannelState->fader.targetVol;
            pSourceChannelState->fader.incrementPerUpdate = 0.0f;
        }
    }

    // El estado de cada canal fuente vive dentro de su SNDSTREAMCHANNEL: se vuelve al principio del canal.
    offsetToStart = pSourceChannelState->sourceChannel * sizeof(StreamSourceChannelState);
    offsetToStart += offsetof(SNDSTREAMCHANNEL, sourceChannelState);
    pSndStreamChannel = (SNDSTREAMCHANNEL *)((char *)pSourceChannelState - offsetToStart);

    voice = SNDVOICEI_get(pSndStreamChannel->shandle);
    if (voice >= 0) {
        CHANPUB *pVoice = &sndgs.chan[voice];
        int channelIndex = Snd::gChannelToVoiceIndexLut[pVoice->numchan - 1][pSourceChannelState->sourceChannel];

        if (channelIndex != -128) {
            int sourceVoice = pVoice->voices[channelIndex];
            CHANPUB *pSourceVoice = &sndgs.chan[sourceVoice];

            pSourceVoice->programmedVol = pSourceChannelState->vol;
            Snd::Util::FastVol(pSourceVoice);
            Snd::Hal::SetVol(sourceVoice);
        }
    }

    if (pSourceChannelState->fader.incrementPerUpdate == 0.0f) {
        Snd::Util::RemoveVariableTimerClient(&pSourceChannelState->fader.variableTimerClient);
    }
}

} // namespace Snd

int SNDSTRM_autovol(int sndStrmHandle, int time, int targetvol) {
    SNDI_CHECKINSTALLED();
    SNDSTREAMCHANNEL *pssc = SNDSTRMI_getstreamptr(sndStrmHandle);
    if (pssc == NULL) {
        return -8;
    }

    float targetVol = (float)targetvol * 0.007874016f;
    if (time <= 0) {
        return SNDSTRM_setvol(sndStrmHandle, (Snd::Channel)-1, targetVol);
    }

    int i = 0;
    do {
        Snd::StreamSourceChannelState *pState = &pssc->sourceChannelState[i];
        if (pState->fader.incrementPerUpdate != 0.0f) {
            Snd::Util::RemoveVariableTimerClient(&pState->fader.variableTimerClient);
        }
        pState->fader.targetVol = targetVol;
        pState->fader.incrementPerUpdate =
            (targetVol - pState->vol) * Snd::gVariableTimerPeriod / (float)time;
        if (pState->fader.incrementPerUpdate != 0.0f) {
            pState->fader.variableTimerClient.pClientFunc = Snd::StreamFader;
            pState->fader.variableTimerClient.pClientData = pState;
            Snd::Util::AddVariableTimerClient(&pState->fader.variableTimerClient);
        }
        i++;
    } while (i <= 5);

    return 0;
}
