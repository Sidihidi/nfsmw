#include "./sndcmn.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/systask.h"

void SYSTEM_addexit(void (*pfunc)());
void SNDREAL_exithandler();

namespace Snd {

static void SNDREAL6_systemtask(void *pdata, int unused) {
    SNDSYS_service();
}

void SNDREAL6_abortmsg(char *message) {
}

Result System::VectorToReal6() {
    static int systaskadded = 0;

    sndgs.sso.vec.abortmsg = SNDREAL6_abortmsg;

    if (systaskadded == 0) {
        SYNCTASK_add(SNDREAL6_systemtask, 0, 1, 0);
        systaskadded = 1;
    }

    SYSTEM_addexit(SNDREAL_exithandler);

    return RESULT_OK;
}

} // namespace Snd
