// La muestra por la que va una voz (C:/packages/snd/9.06.00-ps2/source/library/ps2/sdgetcf.c): las del SPU por la
// direccion que devuelve el IOP (ADPCM: 28 muestras cada 16 bytes), las del IOP por su cuenta de muestras y las de la
// CPU principal por el mezclador.

#include "sndps2.h"

int SNDPLATFORM_getcurframe(int voice) {
    CHANPUB *ppubv;
    SNDDRVVOICE *pv;

    ppubv = &sndgs.chan[voice];
    pv = &snddrv.voice[voice];

    if (ppubv->rendermode & SND_PLAYLOC_SPU) {
        return (snddrv.ioptoee.sv[voice].curaddr - pv->sampleaddr) * 28 / 16;
    }
    if (ppubv->rendermode & SND_PLAYLOC_IOPCPU) {
        return snddrv.ioptoee.iv[voice - sndgs.sso.set.voicesspu].curframe;
    }
    return MIX_getframe(voice - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu));
}
