#include "./sndcmn.h"

void iSNDcalcvol(int voice) {
    CHANPUB *pv;

    pv = &sndgs.chan[voice];
    pv->finalvol = (float)pv->builtinvol * pv->programmedVol * (float)(pv->envvol >> 16) * (1.0f / 127.0f / 127.0f);
    if (pv->pvollfo != 0) {
        pv->finalvol = pv->finalvol * (float)pv->pvollfo->xlate[pv->curvollfoentry] * (1.0f / 127.0f);
    }
    if (pv->pvoltable != 0) {
        pv->finalvol = (float)pv->pvoltable->xlate[SNDI_ftoifast(pv->finalvol * 127.0f)];
    }
    pv->finalvol = pv->finalvol * Snd::gMasterVol;
}
