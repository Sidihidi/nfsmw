// La reproduccion por paquetes en el driver de PS2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/sdpacket.c): solo la
// CPU principal la hace (el mezclador); el SPU y el IOP no tienen canales de paquetes.

#include "sndps2.h"

int SNDDRV_getmastervoice(int voice) {
    CHANPUB *pv;

    voice = voice + sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu;
    pv = &sndgs.chan[voice];
    if (pv->masterchan == -1) {
        return voice;
    }
    return pv->masterchan;
}

int SNDDRV_getsamplechan(int voice) {
    CHANPUB *pv;
    int i;

    voice = voice + sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu;
    pv = &sndgs.chan[voice];
    if (pv->masterchan == -1) {
        return 0;
    }
    pv = &sndgs.chan[pv->masterchan];
    for (i = 1; i < 6; i++) {
        if (pv->voices[i] == voice) {
            return i;
        }
    }
    return 0;
}

int SNDPLATFORM_packetoverhead() {
    return 0;
}

int SNDPLATFORM_packetplaycreate(int packetchan, void *pmem) {
    return 0;
}

int SNDPLATFORM_packetplaydestroy(int packetchan) {
    return 0;
}

int SNDPLATFORM_packetplay(int pktchan, int voice, int timemult, int lowpasscutoff, int highpasscutoff,
                           SNDSAMPLEFORMAT *pssf, SNDSAMPLEATTR *pssa) {
    CHANPUB *ppubv;
    int i;
    int sourceVoice;
    SNDDRVVOICE *pHalVoice;

    ppubv = &sndgs.chan[voice];

    if (!(ppubv->rendermode & SND_PLAYLOC_SPU)) {
        if (ppubv->rendermode & SND_PLAYLOC_MAINCPU) {
        for (i = 0; i < ppubv->numchan; i++) {
            sourceVoice = ppubv->voices[i];
            pHalVoice = &snddrv.voice[sourceVoice];

            MIX_playinit(sourceVoice - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu), pssf->samplerep, 0, 0,
                         (void *)-1, pssa->ptsdata[i], ppubv->numchan, ppubv->frames, -1, -1, 0, 0,
                         pssa->platformver, i);

            SNDI_aztospkrvol(sndgs.chan[sourceVoice].azimuth, pHalVoice->balances);

            pHalVoice->prevlvol = SNDI_ftoitruncpositive(pHalVoice->balances[1] * 32767.0f);
            pHalVoice->prevrvol = SNDI_ftoitruncpositive(pHalVoice->balances[0] * 32767.0f);

            Snd::Hal::SetPan(sourceVoice);
        }

        SNDPLATFORM_setpitch(ppubv->voices[0]);
        SNDPLATFORM_timemult(ppubv->voices[0], timemult);
        SNDPLATFORM_lowpass(ppubv->voices[0], lowpasscutoff);
        SNDPLATFORM_highpass(ppubv->voices[0], highpasscutoff);

        for (i = 0; i < ppubv->numchan; i++) {
            MIX_play(ppubv->voices[i] - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu));
        }
        }
    }
    return 0;
}
