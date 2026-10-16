#include "./sndcmn.h"

Snd::SNDSTREAMREQUEST *SNDSTRMI_getrequestptr(int sndrequesthandle) {
    if (sndrequesthandle < 0) {
        return NULL;
    }

    SNDSTREAMCHANNEL *pssc = SNDSTRMI_getstreamptr(sndrequesthandle & 0xFF);

    if (pssc == NULL) {
        return NULL;
    }

    Snd::SNDSTREAMREQUEST *pssr = (Snd::SNDSTREAMREQUEST *)pssc->allocatedrequests.phead;

    while (pssr != NULL && pssr->sndrequesthandle != sndrequesthandle) {
        pssr = (Snd::SNDSTREAMREQUEST *)pssr->node.pnext;
    }

    return pssr;
}
