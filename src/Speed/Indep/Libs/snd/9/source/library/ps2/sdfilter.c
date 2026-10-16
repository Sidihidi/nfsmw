// Los filtros de voz en el driver de PS2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/sdfilter.c): solo las voces
// de la CPU principal los tienen (el mezclador).

#include "sndps2.h"

int SNDPLATFORM_filteradd(int voice, SNDFILTERDEF *psfd) {
    CHANPUB *ppubv;
    int i;

    ppubv = &sndgs.chan[voice];

    if (ppubv->rendermode & SND_PLAYLOC_MAINCPU) {
        for (i = 0; i < ppubv->numchan; i++) {
            MIX_filteradd(ppubv->voices[i] - (sndgs.sso.set.voicesspu + sndgs.sso.set.voicesiopcpu), i, psfd);
        }
        return 0;
    }
    return SNDERR_FORMAT;
}
