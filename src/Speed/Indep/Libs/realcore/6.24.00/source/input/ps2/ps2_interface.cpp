// realcore 6.24.00: la interfaz de entrada de PS2 (source/input/ps2/ps2_interface.cpp en el .mdebug de la A124).
// Globales, locales y el orden de las sentencias, de sus stabs y su mapa de lineas.
#include "ps2_interface.h"

#include "ps2_effect.h"
#include "ps2_pad.h"

#include <libpad.h>

namespace RealInput {

static char *gIOPDefaultPath = "host0:/usr/local/sce/iop/modules/";
int (*gpInitMTap)() = nullptr;
int (*gpMTapPortOpen)(int) = nullptr;
int (*gpMTapPortClose)(int) = nullptr;
Device *(*gpCreatePs2KeyboardDevice)() = nullptr;
Device *(*gpCreatePs2MouseDevice)() = nullptr;
static bool gIOPMtapInitialized = false;
static char *gIOPModulePath = gIOPDefaultPath;

Ps2Interface::Ps2Interface(const ConfigOptions &options)
    : InterfaceImp(options)
    , mpEffectTable(nullptr) {
    if (this->mConfigOptions.mIOPModulePath != nullptr) {
        gIOPModulePath = this->mConfigOptions.mIOPModulePath;
    } else {
        gIOPModulePath = gIOPDefaultPath;
    }
    scePadInit(0);
    if (gpInitMTap != nullptr) {
        gpInitMTap();
        gpMTapPortOpen(0);
        gpMTapPortOpen(1);
        gIOPMtapInitialized = true;
    } else {
        gIOPMtapInitialized = false;
    }
    if (this->mConfigOptions.mMaxNumEffects != 0) {
        this->mpEffectTable = new Ps2Effect[this->mConfigOptions.mMaxNumEffects];
    }
    this->EnumeratePads();
    this->EnumerateKeyboard();
    this->EnumerateMouse();
}

Ps2Interface::~Ps2Interface() {
    if (this->mpEffectTable != nullptr) {
        delete[] this->mpEffectTable;
    }
    if (gIOPMtapInitialized) {
        gpMTapPortClose(0);
        gpMTapPortClose(1);
    }
    scePadEnd();
}

Ps2Effect *Ps2Interface::GetUnusedEffectSlot() {
    Ps2Effect *pE;
    uint32_t i;

    if (this->mpEffectTable == nullptr) {
        return nullptr;
    }
    pE = this->mpEffectTable;
    for (i = 0; i < this->mConfigOptions.mMaxNumEffects; i++, pE++) {
        if (!pE->IsInUse()) {
            return pE;
        }
    }
    return nullptr;
}

void Ps2Interface::EnumeratePads() {
    Device *pDevice;
    int maxSlots;
    bool keepit;
    int port;
    int slot;

    if (gIOPMtapInitialized) {
        maxSlots = PD_NUM_SLOTS;
    } else {
        maxSlots = 1;
    }
    for (port = 0; port < PD_NUM_PORTS; port++) {
        for (slot = 0; slot < maxSlots; slot++) {
            pDevice = new Ps2Pad(this, port, slot);
            if (this->mConfigOptions.mpEnumDevicesCallback != nullptr) {
                keepit = this->mConfigOptions.mpEnumDevicesCallback(
                    pDevice, this->mConfigOptions.mEnumDevicesCallbackUserData, this);
            } else {
                keepit = false;
            }
            if (!keepit) {
                delete pDevice;
            }
        }
    }
}

void Ps2Interface::EnumerateKeyboard() {
    Device *pDevice;
    bool keepit;

    if (gpCreatePs2KeyboardDevice != nullptr) {
        pDevice = gpCreatePs2KeyboardDevice();
        keepit = false;
        if (this->mConfigOptions.mpEnumDevicesCallback != nullptr) {
            keepit = this->mConfigOptions.mpEnumDevicesCallback(
                pDevice, this->mConfigOptions.mEnumDevicesCallbackUserData, this);
        }
        if (!keepit) {
            delete pDevice;
        }
    }
}

void Ps2Interface::EnumerateMouse() {
    Device *pDevice;
    bool keepit;

    if (gpCreatePs2MouseDevice != nullptr) {
        pDevice = gpCreatePs2MouseDevice();
        keepit = false;
        if (this->mConfigOptions.mpEnumDevicesCallback != nullptr) {
            keepit = this->mConfigOptions.mpEnumDevicesCallback(
                pDevice, this->mConfigOptions.mEnumDevicesCallbackUserData, this);
        }
        if (!keepit) {
            delete pDevice;
        }
    }
}

} // namespace RealInput
