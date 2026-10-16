#include "./sndcmn.h"

int SNDPKTPLAYI_voicetopackethandle(int chan) {
    int i;

    for (i = 0; i < sndgs.sso.set.maxstreams; i++) {
        if (sndpps.ppc[i] != NULL) {
            if (SNDVOICEI_get(sndpps.ppc[i]->shandle) == chan) {
                return i;
            }
        }
    }

    return -1;
}
