#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/FEng/FEPackage.h"
#include "Speed/Indep/Src/FEng/FEngine.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Input/Action.h"
#include "Speed/Indep/Src/Input/ActionData.h"
#include "Speed/Indep/Src/Input/ActionRef.h"
#include "Speed/Indep/Libs/Support/Utility/UStringToNumber.h"
#include "Speed/Indep/Src/Misc/bFile.hpp"

#include <stdio.h>

// misc/testhooks/inputsimulation.cpp de la A124 (zMisc, entre debugtrap.cpp y Hermes.cpp): el "joystick monkey" que
// pulsa acciones al azar en el frontend y en el HUD. Solo la A124 lo conserva; las cadenas y los valores son los de
// su .rodata y su .data, y los nombres de las locales los de sus stabs.

extern StringToNumberEntry GameActionToStringTable[];

bool JoyMonkeyInitialized = false;
int EnableJoystickMonkey = 0;
ActionData *monkeyAction = nullptr;
unsigned int JoystickMonkeyRandomSeed = 0x11111111;
static const char monkeyLogFile[] = "joy_monkey_log.txt";
float gMonkeyActionFrequency[MAX_ACTIONID];
int gLastMonkeyAction[1];
unsigned int gLastActiveScreen = 0;
unsigned int gLastFrameMonkeyed = 0;

void InitJoystickMonkey() {
    JoystickMonkeyRandomSeed = bRandom(static_cast<int>(0x80000000));

    for (int counter = MAX_ACTIONID - 1; counter >= 0; counter--) {
        gMonkeyActionFrequency[counter] = 0.0f;
    }
    for (int counter = 0; counter >= 0; counter--) {
        gLastMonkeyAction[counter] = 0;
    }

    bFile *monkeyFileHandle = bOpen(monkeyLogFile, 6, 1);
    if (monkeyFileHandle) {
        bFPrintf(monkeyFileHandle, "JoyMonkey Log Started:\r\r\n");
        bClose(monkeyFileHandle);
    }

    gMonkeyActionFrequency[FRONTENDACTION_UP] = 0.17f;
    gMonkeyActionFrequency[FRONTENDACTION_DOWN] = 0.17f;
    gMonkeyActionFrequency[FRONTENDACTION_LEFT] = 0.17f;
    gMonkeyActionFrequency[FRONTENDACTION_RIGHT] = 0.17f;
    gMonkeyActionFrequency[FRONTENDACTION_ACCEPT] = 0.17f;
    gMonkeyActionFrequency[FRONTENDACTION_CANCEL] = 0.17f;
    gMonkeyActionFrequency[FRONTENDACTION_CANCEL_ALT] = 0.0f;
    gMonkeyActionFrequency[FRONTENDACTION_START] = 0.0f;
    gMonkeyActionFrequency[FRONTENDACTION_BUTTON0] = 0.0f;
    gMonkeyActionFrequency[FRONTENDACTION_BUTTON1] = 0.0f;
    gMonkeyActionFrequency[FRONTENDACTION_BUTTON2] = 0.0f;
    gMonkeyActionFrequency[FRONTENDACTION_BUTTON3] = 0.0f;
    gMonkeyActionFrequency[FRONTENDACTION_BUTTON4] = 0.0f;
    gMonkeyActionFrequency[FRONTENDACTION_LTRIGGER] = 0.0f;
    gMonkeyActionFrequency[FRONTENDACTION_RTRIGGER] = 0.0f;

    gMonkeyActionFrequency[HUDACTION_PAUSEREQUEST] = 1.0f;

    gLastActiveScreen = 0;
    gLastFrameMonkeyed = 0;
}

void DoMonkeyLog(ActionData monkeyAction) {
    char monkeyLogLine[256];

    if (cFEng::Get()->FindPackageWithControl() &&
        cFEng::Get()->FindPackageWithControl()->GetNameHash() != gLastActiveScreen) {
        sprintf(monkeyLogLine, "Frame %d: %s - [%s]\r\n", eGetFrameCounter(),
                GameActionToStringTable[monkeyAction.ID()].string, cFEng::Get()->FindPackageWithControl()->GetFilename());
        gLastActiveScreen = cFEng::Get()->FindPackageWithControl()->GetNameHash();
    } else {
        sprintf(monkeyLogLine, "Frame %d: %s\r\n", eGetFrameCounter(), GameActionToStringTable[monkeyAction.ID()].string);
    }

    bAppendToFile(monkeyLogFile, monkeyLogLine, bStrLen(monkeyLogLine));
}

ActionRef GetFEJoystickMonkeyAction(int port) {
    if (!JoyMonkeyInitialized) {
        InitJoystickMonkey();
        JoyMonkeyInitialized = true;
    }

    if (!monkeyAction) {
        monkeyAction = new ActionData(0, 0.0f, 0);
    }

    if (gLastMonkeyAction[port] == 0) {
        float randNum = bRandom(1.0f, &JoystickMonkeyRandomSeed);
        float freqTotal = 0.0f;
        int monkeyActionID = FRONTENDACTION_UP;

        while (monkeyActionID < HUDACTION_PAUSEREQUEST) {
            freqTotal += gMonkeyActionFrequency[monkeyActionID];
            if (randNum <= freqTotal) {
                break;
            }
            monkeyActionID++;
        }

        monkeyAction->SetID(monkeyActionID);
        monkeyAction->SetSlot(port);
        monkeyAction->SetData(1.0f);
        gLastMonkeyAction[port] = monkeyActionID;

        DoMonkeyLog(*monkeyAction);
    } else {
        monkeyAction->SetSlot(port);
        monkeyAction->SetData(0.0f);
        monkeyAction->SetID(gLastMonkeyAction[port]);
        gLastMonkeyAction[port] = 0;
    }

    return ActionRef(monkeyAction);
}

ActionRef GetHUDJoystickMonkeyAction(int port) {
    if (!monkeyAction) {
        monkeyAction = new ("JoyMonkey::ActionData", 0) ActionData(0, 0.0f, 0);
    }

    if (gLastMonkeyAction[port] == 0) {
        float randNum = bRandom(1.0f, &JoystickMonkeyRandomSeed);
        float freqTotal = 0.0f;
        int monkeyActionID = HUDACTION_PAUSEREQUEST;

        while (monkeyActionID < HUDACTION_PAUSEREQUEST + 1) {
            freqTotal += gMonkeyActionFrequency[monkeyActionID];
            if (randNum <= freqTotal) {
                break;
            }
            monkeyActionID++;
        }

        monkeyAction->SetID(monkeyActionID);
        monkeyAction->SetSlot(port);
        gLastMonkeyAction[port] = monkeyActionID;
        monkeyAction->SetData(1.0f);
    } else {
        monkeyAction->SetSlot(port);
        monkeyAction->SetData(0.0f);
        monkeyAction->SetID(gLastMonkeyAction[port]);
        gLastMonkeyAction[port] = 0;
    }

    return ActionRef(monkeyAction);
}
