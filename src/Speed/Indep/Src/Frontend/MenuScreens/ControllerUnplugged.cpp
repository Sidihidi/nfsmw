#include "ControllerUnplugged.hpp"

#include "Speed/Indep/Src/Frontend/Event.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Loading/FEBootFlowManager.hpp"
#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/Src/Input/IOModule.h"
#include "Speed/Indep/Src/Input/SteeringWheelDevice.h"
#endif
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#endif

#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124)
bool bControllerUnpluggedScreenUp = false;
#endif

ControllerUnplugged::ControllerUnplugged(ScreenConstructorData *sd) : MenuScreen(sd) {
    port = (JoystickPort)sd->Arg;
    Setup();
}

ControllerUnplugged::~ControllerUnplugged() {
#ifdef EA_BUILD_A124
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
        IPlayer *iplayer = IPlayer::First(PLAYER_LOCAL);
        if (!iplayer) {
            return;
        }
        ISimable *isimable = iplayer->GetSimable();
        if (!isimable) {
            return;
        }
        IVehicle *vehicle;
        if (isimable->QueryInterface(&vehicle)) {
            vehicle->ForceStopOff(11);
        }
    }
#elif defined(EA_PLATFORM_PLAYSTATION2)
    bControllerUnpluggedScreenUp = false;
#endif
}

void ControllerUnplugged::NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) {
    switch (msg) {
        case 0xEBFCDA65: {
        if (port == JOYSTICK_PORT_NONE) {
            BootFlowManager::Get()->JumpToHead();
        } else {
#ifdef EA_PLATFORM_GAMECUBE
            int joyPort = FEngMapJoyParamToJoyport(param1);
            cFEng::Get()->PopErrorPackage(joyPort);
            if (FEManager::IsOkayToRequestPauseSimulation(GetPortsPlayer(joyPort), false, false)) {
                new EPause(GetPortsPlayer(joyPort), 0, 0);
            }
#else
            int joyPort = FEngMapJoyParamToJoyport(param1);
            cFEng::Get()->PopErrorPackage(joyPort);
#endif
        }
        break;
        }
    }
}

void ControllerUnplugged::Setup() {
#ifdef EA_PLATFORM_PLAYSTATION2
    if (port == JOYSTICK_PORT_NONE) {
        FEngSetLanguageHash(GetPackageName(), 0xB244CF71, 0x8E4901EF);
    } else {
#ifndef EA_BUILD_A124
        if (port == JOYSTICK_PORT1) {
            bControllerUnpluggedScreenUp = true;
        }
#endif
        bool WasDeviceWheel = false;
        InputDevice *device = IOModule::GetIOModule().GetDevice(port);
        if (device) {
            UTL::COM::IUnknown *pUnk = device->GetSecondaryDevice();
            if (pUnk) {
                SteeringWheelDevice *wheel;
                if (pUnk->QueryInterface(&wheel)) {
                    if (wheel->IsActivated()) {
                        wheel->Deactivated();
                        WasDeviceWheel = true;
                    }
                }
            }
        }
        if (WasDeviceWheel) {
            FEPrintf(GetPackageName(), 0xB244CF71, GetLocalizedString(0x1E2A1C4B));
        } else {
            FEPrintf(GetPackageName(), 0xB244CF71, GetLocalizedString(0x54EEFEC2), port + 1);
        }
    }
#ifdef EA_BUILD_A124
    if (FEDatabase->IsOnlineMode() || FEDatabase->IsLanMode()) {
        IPlayer *iplayer = IPlayer::First(PLAYER_LOCAL);
        if (!iplayer) {
            return;
        }
        ISimable *isimable = iplayer->GetSimable();
        if (!isimable) {
            return;
        }
        IVehicle *vehicle;
        if (isimable->QueryInterface(&vehicle)) {
            vehicle->ForceStopOn(1);
        }
    }
#endif
#elif defined(EA_PLATFORM_GAMECUBE)
    FEPrintf(GetPackageName(), 0xB244CF71, GetLocalizedString(0x54EEF4C5), port + 1);
#elif defined(EA_PLATFORM_XENON)
    FEPrintf(GetPackageName(), 0xB244CF71, GetLocalizedString(0x54EE7FA6), port + 1); // X360: NFS.exe 0x82271CE4
#else
    FEPrintf(GetPackageName(), 0xB244CF71, GetLocalizedString(0x54EEFEC2), port + 1);
#endif
}
