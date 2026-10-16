// El arranque de un timbre en el driver de PS2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/sdplapat.c): las voces
// del SPU y del IOP se le mandan al IOP con la direccion de su muestra en la memoria de cada uno (la del SPU tambien a
// la mitad alta del SPU2 si la salida digital va en modo 2); las de la CPU principal las arranca el mezclador.

#include "sndps2.h"

int SNDPLATFORM_playtimbre(SNDIPATCHHEADER *pph, void *psampledata, int voice, int timemult, int lowpasscutoff,
                           int highpasscutoff) {
    CHANPUB *ppubv;
    int i;

    ppubv = &sndgs.chan[voice];

    if (ppubv->rendermode & SND_PLAYLOC_SPU) {
        SNDPLATFORM_setpitch(voice);

        for (i = 0; i < ppubv->numchan; i++) {
            int sourceVoice = ppubv->voices[i];
            SNDDRVVOICE *pHalVoice = &snddrv.voice[sourceVoice];

            SNDPLATFORM_setfxlevel(sourceVoice, 0);
            Snd::Hal::SetDry(sourceVoice);

            SNDI_aztospkrvol(sndgs.chan[sourceVoice].azimuth, pHalVoice->balances);

            pHalVoice->prevlvol = SNDI_ftoitruncpositive(pHalVoice->balances[1] * 32767.0f);
            pHalVoice->prevrvol = SNDI_ftoitruncpositive(pHalVoice->balances[0] * 32767.0f);

            Snd::Hal::SetPan(sourceVoice);

            pHalVoice->sampleaddr = *pph->paudioramaddr + (pph->sampleoffset[i] - pph->sampleoffset[0]);

            snddrv.startaddrsync += 0x800000;

            snddrv.peetoiop->sv[sourceVoice].startaddr = pHalVoice->sampleaddr | snddrv.startaddrsync;
            snddrv.ioptoee.sv[sourceVoice].curaddr = pHalVoice->sampleaddr;

            if (sndgs.sso.set.outputspdifmode == 2) {
                snddrv.peetoiop->sv[sourceVoice + 24].startaddr = pHalVoice->sampleaddr | snddrv.startaddrsync;
                snddrv.ioptoee.sv[sourceVoice + 24].curaddr = pHalVoice->sampleaddr;
            }
        }
    } else if (ppubv->rendermode & SND_PLAYLOC_IOPCPU) {
        SNDPLATFORM_setpitch(voice);

        for (i = 0; i < ppubv->numchan; i++) {
            int sourceVoice = ppubv->voices[i];
            SNDDRVVOICE *pHalVoice = &snddrv.voice[sourceVoice];
            int iopVoice;

            SNDPLATFORM_setfxlevel(sourceVoice, 0);

            SNDI_aztospkrvol(sndgs.chan[sourceVoice].azimuth, pHalVoice->balances);

            pHalVoice->prevlvol = SNDI_ftoitruncpositive(pHalVoice->balances[1] * 32767.0f);
            pHalVoice->prevrvol = SNDI_ftoitruncpositive(pHalVoice->balances[0] * 32767.0f);

            Snd::Hal::SetPan(sourceVoice);

            pHalVoice->sampleaddr = *pph->paudioramaddr + (pph->sampleoffset[i] - pph->sampleoffset[0]);

            iopVoice = sourceVoice - sndgs.sso.set.voicesspu;

            snddrv.startaddrsync += 0x800000;

            snddrv.peetoiop->iv[iopVoice].startaddr = pHalVoice->sampleaddr | snddrv.startaddrsync;
            snddrv.ioptoee.iv[iopVoice].curframe = 0;
        }
    } else {
        for (i = 0; i < ppubv->numchan; i++) {
            int sourceVoice = ppubv->voices[i];
            SNDDRVVOICE *pHalVoice = &snddrv.voice[sourceVoice];

            MIX_playinit(sourceVoice - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu), pph->samplerep, 1,
                         (char *)psampledata + pph->sampleoffset[i],
                         (char *)psampledata + pph->sampleoffset[0] + pph->loopoffset[i], pph->ptimestretchdata[i],
                         ppubv->numchan, ppubv->frames, pph->sustainstart, pph->sustainend, 0, 0, pph->platformver,
                         i);

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
    return 0;
}
