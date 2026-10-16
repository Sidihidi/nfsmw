#include "Speed/PSX2/Src/Logitech/LogiWheels.h"

#include <string.h>

// Que direcciones marca cada posicion de la cruceta (POV0 de 0 a 7, en el sentido de las agujas del reloj desde
// arriba), por columnas en el orden de DPadDirection.
static const int dpadPosition[8][4] = {
    {1, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 1}, {0, 0, 0, 1}, {1, 0, 0, 1},
};

LogiWheels::LogiWheels() {
    InitVars();
    InitlgDevLibrary();
}

void LogiWheels::InitVars() {
    for (int channel = 0; channel < 2; channel++) {
        WheelHandles[channel] = -1;
        validController[channel] = false;
        wheelClass[channel] = 0x80000000;
        multiturnCapable[channel] = false;
    }

    enumhint = 1;
    numDevices = 0;
    rebootedIOP = false;

    memset(Position, 0, sizeof(Position));
    memset(PositionLast, 0, sizeof(PositionLast));
    memset(NonLinearWheel, 0, sizeof(NonLinearWheel));
}

void LogiWheels::InitlgDevLibrary() {
    lgDevInit();

    for (int ii = 0; ii < 2; ii++) {
        validController[ii] = false;
        WheelHandles[ii] = -1;
    }

    numDevices = 0;
}

// El primer byte de la topologia es el puerto, con el 1 y el 2 cambiados; los otros siete, un nibble cada uno.
unsigned int LogiWheels::GetDeviceNumber(unsigned char *Topology) {
    unsigned int deviceNumber;
    int i;

    deviceNumber = Topology[0];
    deviceNumber = (deviceNumber == 2) ? 1 : ((deviceNumber == 1) ? 2 : deviceNumber);
    for (i = 1; i < 8; i++) {
        deviceNumber <<= 4;
        deviceNumber |= Topology[i] & 0xF;
    }

    return deviceNumber;
}

bool LogiWheels::EnumIsNeeded() {
    lgDevEnumHint(&enumhint);

    if (enumhint == 1 || rebootedIOP == true) {
        rebootedIOP = false;
        return true;
    }

    return false;
}

void LogiWheels::ReadPort(int channel) {
    int ret;
    lgDevPosition *readbuff = &Position[channel];

    if (validController[channel]) {
        PositionLast[channel] = Position[channel];
        lgDevASync(0, &ret);
        lgDevARead(WheelHandles[channel], 1, readbuff);
    }
}

// Abre los dos primeros dispositivos, se queda con los volantes (clase 0x11 y 0x12) y cierra los 0x21 y 0x22.
// Luego los ordena por su numero de dispositivo, para que el canal no dependa del orden de enumeracion.
void LogiWheels::EnumerateDevices() {
    int openeddevs = 0;
    int index = 0;
    unsigned int deviceNumber[2];
    unsigned int deviceIndex[2];
    lgDevDeviceDesc description;

    memset(deviceNumber, 0, sizeof(deviceNumber));
    memset(deviceIndex, 0, sizeof(deviceIndex));

    while (lgDevEnumerate(index, &description) >= 0) {
        if (index < 2) {
            lgDevOpen(index, &WheelHandles[openeddevs]);
            lgDevSetDeviceProperty(WheelHandles[openeddevs], 2, 0);

            if (description.DevClass == 0x11) {
                wheelClass[openeddevs] = description.DevClass;
                validController[openeddevs] = true;
                deviceNumber[openeddevs] = GetDeviceNumber(description.Topology);
                deviceIndex[openeddevs] = openeddevs;
                openeddevs++;
            } else if (description.DevClass == 0x12) {
                wheelClass[openeddevs] = description.DevClass;
                validController[openeddevs] = true;
                deviceNumber[openeddevs] = GetDeviceNumber(description.Topology);
                deviceIndex[openeddevs] = openeddevs;
                if (description.Flags & 4) {
                    multiturnCapable[openeddevs] = true;
                }
                openeddevs++;
            } else if (description.DevClass == 0x21) {
                lgDevClose(WheelHandles[openeddevs]);
                WheelHandles[openeddevs] = -1;
            } else if (description.DevClass == 0x22) {
                lgDevClose(WheelHandles[openeddevs]);
                WheelHandles[openeddevs] = -1;
            }
        }
        index++;
    }

    numDevices = openeddevs;

    if (numDevices > 0) {
        for (int i = 0; i < numDevices - 1; i++) {
            for (int j = i + 1; j < numDevices; j++) {
                if (deviceNumber[j] < deviceNumber[i]) {
                    unsigned int tempindex;
                    unsigned int tempHandle;
                    unsigned int tempClass;
                    unsigned int tempController;
                    bool tempMultiturnCapable;

                    tempindex = deviceNumber[i];
                    deviceNumber[i] = deviceNumber[j];
                    deviceNumber[j] = tempindex;
                    tempindex = deviceIndex[i];
                    deviceIndex[i] = deviceIndex[j];
                    deviceIndex[j] = tempindex;
                    tempHandle = WheelHandles[i];
                    WheelHandles[i] = WheelHandles[j];
                    WheelHandles[j] = tempHandle;
                    tempClass = wheelClass[i];
                    wheelClass[i] = wheelClass[j];
                    wheelClass[j] = tempClass;
                    tempController = validController[i];
                    validController[i] = validController[j];
                    validController[j] = tempController;
                    tempMultiturnCapable = multiturnCapable[i];
                    multiturnCapable[i] = multiturnCapable[j];
                    multiturnCapable[j] = tempMultiturnCapable;
                }
            }
        }
    }

    for (int i = numDevices; i < 2; i++) {
        validController[i] = false;
        WheelHandles[i] = -1;
    }
}

void LogiWheels::DestroyDevices() {
    for (int i = 0; i < 2; i++) {
        if (WheelHandles[i] != -1) {
            lgDevClose(WheelHandles[i]);
        }
    }

    InitVars();
}

// Los cuatro bits altos de la mascara son la cruceta, que el volante da como POV y no como botones.
bool LogiWheels::ButtonIsPressed(int channel, int buttonMask) {
    lgDevASync(0, 0);

    switch (buttonMask) {
    case 0x10000000:
        if (DPadIsPressed(channel, LG_LOCAL_DPAD_UP)) {
            return true;
        }
        break;
    case 0x20000000:
        if (DPadIsPressed(channel, LG_LOCAL_DPAD_DOWN)) {
            return true;
        }
        break;
    case 0x40000000:
        if (DPadIsPressed(channel, LG_LOCAL_DPAD_RIGHT)) {
            return true;
        }
        break;
    case static_cast<int>(0x80000000):
        if (DPadIsPressed(channel, LG_LOCAL_DPAD_LEFT)) {
            return true;
        }
        break;
    }

    return (Position[channel].Buttons & buttonMask) != 0;
}

bool LogiWheels::DPadIsPressed(int channel, int direction) {
    lgDevASync(0, 0);

    if (Position[channel].POV0 != 0xFF) {
        switch (direction) {
        case LG_LOCAL_DPAD_UP:
            if (dpadPosition[Position[channel].POV0][LG_LOCAL_DPAD_UP]) {
                return true;
            }
            break;
        case LG_LOCAL_DPAD_RIGHT:
            if (dpadPosition[Position[channel].POV0][LG_LOCAL_DPAD_RIGHT]) {
                return true;
            }
            break;
        case LG_LOCAL_DPAD_DOWN:
            if (dpadPosition[Position[channel].POV0][LG_LOCAL_DPAD_DOWN]) {
                return true;
            }
            break;
        case LG_LOCAL_DPAD_LEFT:
            if (dpadPosition[Position[channel].POV0][LG_LOCAL_DPAD_LEFT]) {
                return true;
            }
            break;
        }
    }

    return false;
}

bool LogiWheels::IsConnected(int channel) {
    if (validController[channel]) {
        return true;
    }
    return false;
}

bool LogiWheels::IsWheelClassA(int channel) {
    return wheelClass[channel] == 0x11;
}

bool LogiWheels::IsMultiturnCapable(int channel) {
    return multiturnCapable[channel];
}

bool LogiWheels::IsMultiturnEnabled(int channel) {
    int value = 0;

    if ((lgDevGetDeviceProperty(WheelHandles[channel], 4, &value) & 0x80000000) != 0x80000000) {
        if (value == 1) {
            return true;
        }
    }

    return false;
}

// El original mira el resultado con la misma mascara que IsMultiturnEnabled y no hace nada con el: es lo que
// impide que la llamada salga como salto de cola (jal y no j).
void LogiWheels::SetMultiturn(int channel, int value) {
    if ((lgDevSetDeviceProperty(WheelHandles[channel], 4, value) & 0x80000000) == 0x80000000) {
    }
}
