#include "./sndcmn.h"

#ifdef __cplusplus
extern "C" {
#endif

void STREAM_setgreedylevel(int streamhandle, int greedylevel);

#ifdef __cplusplus
}
#endif

// PC (speed.exe 0x81BAE3): la comprobacion de controlador y la vuelta temprana con -8; el DWARF de GameCube solo
// da la local pssc.
int SNDSTRM_setgreedylevel(int sndstreamhandle, int greedylevel) {
    SNDSTREAMCHANNEL *pssc;

    SNDI_CHECKINSTALLED();
    pssc = SNDSTRMI_getstreamptr(sndstreamhandle);
    if (pssc == NULL) {
        return -8;
    }

    STREAM_setgreedylevel(pssc->streamhandle, greedylevel);
    return 0;
}
