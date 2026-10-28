#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndcmn.h"
#include "snd/sndo.h"

int SNDBANK_play(int bhandle, int patnum, SNDPLAYOPTS *pspo) {
    BANKVER5 *pb;
    SNDI_CHECKINSTALLED();
    SNDI_checkplayopts(pspo);
    if (SNDBANKI_valid(bhandle) < 0) {
        return -8;
    }

    pb = sndgs.banklist[bhandle].phdr;
    if (patnum < 0 || patnum >= pb->numpatches) {
        return -8;
    }
    // r76: el return lo recupero del PORT (bug cazado en Android): el binario
    // GC original tiene epilogo completo tras el bl playpatch (con el camino
    // del -8 convergiendo en el), asi que el fuente original lo llevaba. Sin
    // el, GCC cae por el final (UB que tolera devolviendo r3) pero clang -O2
    // optimiza el epilogo fuera y la ejecucion continua dentro de la funcion
    // siguiente. Es UB, no codigo original.
    return SNDBANKI_playpatch(pb, SNDBANKI_getppatch(pb, patnum), bhandle, patnum, pspo);
}
