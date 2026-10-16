#include "./EAXTrafficCar.hpp"

STATETYPE_IMPLEMENT(0x00050000, EAXTrafficCar, CSTATE_Base)

EAXTrafficCar::EAXTrafficCar() {}

EAXTrafficCar::~EAXTrafficCar() {}

void EAXTrafficCar::Attach(void *pAttachment) {
    m_pCar = (EAX_CarState *)pAttachment;
    IsLargeTrafficCar = false;
    CSTATE_Base::Attach(pAttachment);
}

bool EAXTrafficCar::Detach() {
    CSTATE_Base::Detach();
    m_pCar = nullptr;
    return true;
}
