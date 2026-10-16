// realcore 6.24.00: dispositivo de entrada de PS2 (source/input/ps2/ps2_device.cpp en el .mdebug de la A124).
#pragma implementation "deviceimp.h"
#include "ps2_device.h"

namespace RealInput {

Ps2Device::Ps2Device(Type type, Ps2Interface *pInterface)
    : DeviceImp(PLATFORM_PS2, type) {
    this->mpInterface = pInterface;
}

Ps2Device::~Ps2Device() {}

void Ps2Device::GetPortSlotNum(int &port, int &slot) {
    port = this->mPortNum;
    slot = this->mSlotNum;
}

void Ps2Device::SetPortSlotNum(int port, int slot) {
    this->mPortNum = port;
    this->mSlotNum = slot;
}

} // namespace RealInput
