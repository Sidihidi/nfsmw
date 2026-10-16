#include "./sndcmn.h"

#ifdef __cplusplus
extern "C" {
#endif

int SNDstop(int shandle) {
    SNDI_CHECKINSTALLED();
    int voice = SNDVOICEI_get(shandle);

    if (voice >= 0) {
        int setvoice = -1;
        while (iSNDpatchkey(voice, &setvoice)) {
            SNDPLATFORM_stop(setvoice);
        }
    }

    return voice;
}

#ifdef __cplusplus
}
#endif
