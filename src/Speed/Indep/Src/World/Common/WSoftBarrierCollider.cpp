#include "Speed/Indep/Src/World/WSoftBarrierCollider.h"

#ifdef EA_BUILD_A124

#include "Speed/Indep/Libs/Support/Utility/UGroup.hpp"

// gcdatos21: inicializado para que en la A124 caiga entre bAiRandomTurns y
// fgTriggerManager (0x649d20), no en el bloque tentativo del final.
WSoftBarrierCollider *WSoftBarrierCollider::fgSoftBarrierCollider = nullptr;

// In3dQuad, PosInSoftZone y ApplyForce estan en los stabs de la A124 pero no en su binario (estripadas); quedan sin
// escribir.

WSoftBarrierCollider::WSoftBarrierCollider(const WSoftBarrier *sb, unsigned int count)
    : mSoftBarriers(sb), //
      mSoftBarrierCount(count) {}

void WSoftBarrierCollider::Init(const UData *sbUData) {
    if (sbUData) {
        fgSoftBarrierCollider =
            new WSoftBarrierCollider(static_cast<const WSoftBarrier *>(sbUData->GetDataConst()), sbUData->DataCount());
    } else {
        fgSoftBarrierCollider = new WSoftBarrierCollider(nullptr, 0);
    }
}

#endif
