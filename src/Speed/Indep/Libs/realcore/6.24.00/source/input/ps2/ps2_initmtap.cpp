// realcore 6.24.00: engancha el multitap (source/input/ps2/ps2_initmtap.cpp en el .mdebug de la A124). Solo la A124
// lo enlaza.
#include "ps2_interface.h"

#include <libmtap.h>

namespace RealInput {

void InitPS2MultiTap() {
    gpInitMTap = sceMtapInit;
    gpMTapPortOpen = sceMtapPortOpen;
    gpMTapPortClose = sceMtapPortClose;
}

} // namespace RealInput
