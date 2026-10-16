#include "./sndcmn.h"

#ifdef __cplusplus
extern "C" {
#endif

int SNDSTRM_overheadtap(int maxrequests, int maxchunks) {
    int overhead = Snd::gMaxFxBuses * sizeof(Snd::SNDSTREAMFXVOLUME) + sizeof(SNDSTREAMCHANNEL);
    overhead += maxrequests * sizeof(Snd::SNDSTREAMREQUEST);

    return overhead + SNDPKTPLAY_overhead(maxchunks);
}

int SNDSTRM_overhead(int maxrequests, int maxchunks) {
    int overhead = SNDSTRM_overheadtap(maxrequests, maxchunks);

    overhead += STREAM_overhead(maxrequests + 2, 1, 1);

    return overhead;
}

#ifdef __cplusplus
}
#endif
