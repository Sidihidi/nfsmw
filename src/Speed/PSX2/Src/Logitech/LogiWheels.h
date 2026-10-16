#ifndef PSX2_LOGITECH_LOGIWHEELS_H
#define PSX2_LOGITECH_LOGIWHEELS_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/PSX2/Src/Logitech/liblgdev.h"

enum DPadDirection {
    LG_LOCAL_DPAD_UP = 0,
    LG_LOCAL_DPAD_RIGHT = 1,
    LG_LOCAL_DPAD_DOWN = 2,
    LG_LOCAL_DPAD_LEFT = 3,
};

enum MenuDirection {
    LG_MENU_UP = 0,
    LG_MENU_DOWN = 1,
    LG_MENU_RIGHT = 2,
    LG_MENU_LEFT = 3,
};

// total size: 0x900
struct LogiWheels {
    lgDevPosition Position[2];    // offset 0x0, size 0x30
    int NonLinearWheel[2][256];   // offset 0x30, size 0x800
    int WheelHandles[2];          // offset 0x830, size 0x8
    bool validController[2];      // offset 0x838, size 0x8
    int wheelClass[2];            // offset 0x840, size 0x8

  private:
    // Los stabs lo ponen en 0x880 y no en 0x848, y la clase mide 0x900: el miembro va alineado a 64.
    lgDevPosition PositionLast[2] __attribute__((aligned(64))); // offset 0x880, size 0x30
    bool multiturnCapable[2];     // offset 0x8B0, size 0x8
    int numDevices;               // offset 0x8B8, size 0x4
    int enumhint;                 // offset 0x8BC, size 0x4
    bool rebootedIOP;             // offset 0x8C0, size 0x4

  public:
    LogiWheels();
    bool EnumIsNeeded();
    void EnumerateDevices();
    void ReadPort(int channel);
    bool ButtonIsPressed(int channel, int buttonMask);
    bool ButtonTriggered(int channel, int buttonMask);
    bool ButtonReleased(int channel, int buttonMask);
    bool WheelAMenuIsPressed(int channel, int direction);
    bool WheelAMenuTriggered(int channel, int direction);
    bool IsConnected(int channel);
    bool IsWheelClassA(int channel);
    bool IsWheelClassB(int channel);
    bool IsMultiturnCapable(int channel);
    bool IsMultiturnEnabled(int channel);
    void SetMultiturn(int channel, int value);
    void DestroyDevices();
    void GenerateNonLinValues(int channel, int nonLinCoeff);
    void PrepareForIOPReboot();
    void InitAfterIOPReboot();

  private:
    void InitVars();
    void InitlgDevLibrary();
    unsigned int GetDeviceNumber(unsigned char *Topology);
    bool DPadIsPressed(int channel, int direction);
    bool DPadTriggered(int channel, int direction);
    bool DPadReleased(int channel, int direction);
    float CalculateNonLinValue(int inputValue, int nonLinearCoeff, float physicsMinInput, float physicsMaxInput);
};

#endif
