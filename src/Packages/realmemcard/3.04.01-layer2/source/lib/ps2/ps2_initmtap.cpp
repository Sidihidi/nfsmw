#include "ps2_interface.h"

namespace Realmc {

// Solo guarda que puertos llevan multitap: la interfaz los abre al crearse (PS2Interface::PS2Interface).
void Interface::InitMultitap(bool usePort1, bool usePort2) {
    if (usePort1 || usePort2) {
        gMCMTapUsePort[0] = usePort1;
        gMCMTapUsePort[1] = usePort2;
    }
}

} // namespace Realmc
