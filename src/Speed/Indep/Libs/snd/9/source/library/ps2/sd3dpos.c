// El panorama de las voces en el driver de PS2 (C:/packages/snd/9.06.00-ps2/source/library/ps2/sd3dpos.c): las
// ganancias de los altavoces por el acimut y, para el Dolby Pro Logic, el signo de las dos de delante segun el cuadrante
// (el del cuadrante anterior se deduce del signo de los volumenes que se mandaron la ultima vez).

#include "sndps2.h"

void SNDI_dolby(int azimuth, float *plvol, float *prvol, int prevlvol, int prevrvol) {
    int newquad;
    int prevquad;
    int flip;

    if (prevlvol < 0 && prevrvol < 0) {
        if (prevlvol < prevrvol) {
            prevquad = 1;
        } else {
            prevquad = 2;
        }
    } else if (prevlvol >= 0 && prevrvol >= 0) {
        if (prevlvol < prevrvol) {
            prevquad = 2;
        } else {
            prevquad = 1;
        }
    } else {
        if ((prevlvol > -1 ? prevlvol : -prevlvol) > (prevrvol > -1 ? prevrvol : -prevrvol)) {
            prevquad = 4;
        } else {
            prevquad = 3;
        }
    }

    flip = 0;
    if (azimuth > 0x2000 && azimuth < 0xE000) {
        if (*plvol < *prvol) {
            newquad = 3;
        } else {
            newquad = 4;
        }
        if (prevquad < 3) {
            flip = 2;
            if (newquad == 3 && prevquad == 2) {
                flip = 1;
            }
        }
    } else {
        if (*plvol < *prvol) {
            newquad = 2;
        } else {
            newquad = 1;
        }
        if (prevquad >= 3) {
            flip = 2;
            if (newquad == 2 && prevquad == 3) {
                flip = 1;
            }
        }
    }

    if (prevlvol < 0) {
        *plvol = -*plvol;
    }
    if (prevrvol < 0) {
        *prvol = -*prvol;
    }

    if (flip == 1) {
        *plvol = -*plvol;
    } else if (flip == 2) {
        *prvol = -*prvol;
    }
}

void Snd::Hal::SetPan(int voice) {
    CHANPUB *pVoice;
    SNDDRVVOICE *pHalVoice;
    int i;

    pVoice = &sndgs.chan[voice];
    pHalVoice = &snddrv.voice[voice];

    if (pVoice->isLfe) {
        for (i = 0; i < Snd::gTotalOutputChannels; i++) {
            pHalVoice->balances[i] = 0.0f;
        }
        if (Snd::gFoldDownTarget == FOLDDOWNTARGET_5POINT1) {
            pHalVoice->balances[5] = 1.0f;
        }
    } else if (Snd::gFoldDownTarget == FOLDDOWNTARGET_5POINT1 && (pVoice->rendermode & SND_PLAYLOC_SPU) &&
               (pVoice->azimuth < 0x2000 || pVoice->azimuth > 0xE000)) {
        SNDI_aztospkrvol((unsigned short)(pVoice->azimuth + 0x8000), pHalVoice->balances);
        pHalVoice->balances[4] = pHalVoice->balances[2];
        pHalVoice->balances[1] = pHalVoice->balances[3];
        pHalVoice->balances[0] = 0.0f;
        pHalVoice->balances[2] = 0.0f;
        pHalVoice->balances[3] = 0.0f;
    } else {
        SNDI_aztospkrvol(pVoice->azimuth, pHalVoice->balances);
    }

    if (Snd::gFoldDownTarget == FOLDDOWNTARGET_MONO && (pVoice->rendermode & SND_PLAYLOC_SPU)) {
        pHalVoice->balances[1] = pHalVoice->balances[0];
    }

    if (pVoice->rendermode & 0x40) {
        SNDI_dolby(pVoice->azimuth, &pHalVoice->balances[1], &pHalVoice->balances[0], pHalVoice->prevlvol,
                   pHalVoice->prevrvol);
        pHalVoice->prevlvol = SNDI_ftoitruncpositive(pHalVoice->balances[1] * 32767.0f);
        pHalVoice->prevrvol = SNDI_ftoitruncpositive(pHalVoice->balances[0] * 32767.0f);
    }

    SetVol(voice);
}
