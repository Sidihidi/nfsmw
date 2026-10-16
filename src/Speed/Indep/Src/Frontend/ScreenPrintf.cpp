// Debug text on screen through the strings of ScreenPrintf.fng. Only the PS2 prototype (SLES-53558-A124) has it: its
// .mdebug puts frontend/screenprintf.cpp in zFe2, between WideCharHistogram.cpp and RaceStarter.cpp. The source also
// had ScreenDrawSlider and a ScreenShadowPrintf with a colour, but the prototype's linker dropped them.
#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#include "Speed/Indep/Src/Camera/ICE/ICEManager.hpp"
#include "Speed/Indep/Src/FEng/FEString.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEPackageManager.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"

#include <stdarg.h>

ScreenPrintItem ScreenPrintItemTable[64];

ScreenPrintItem *GetUnusedScreenPrintItem() {
    for (int i = 0; i < 64; i++) {
        ScreenPrintItem *item = &ScreenPrintItemTable[i];
        if (item->Lifetime < 0.0f) {
            return item;
        }
    }
    return nullptr;
}

ScreenPrintItem *GetScreenPrintItemAtLocation(int x, int y) {
    for (int i = 0; i < 64; i++) {
        if (ScreenPrintItemTable[i].PosX == x && ScreenPrintItemTable[i].PosY == y) {
            return &ScreenPrintItemTable[i];
        }
    }
    return nullptr;
}

void InitScreenPrintf() {
    cFEng::Get()->PushNoControlPackage("ScreenPrintf.fng", FE_PACKAGE_PRIORITY_CLOSEST);
    FEPackageManager::Get()->SetUseIdleList("ScreenPrintf.fng", true);
    FEPackageManager::Get()->SetPermanent("ScreenPrintf.fng", 1);

    for (int n = 0; n < 64; n++) {
        char text[30];

        bSPrintf(text, "SCREENPRINTF%d", n);
        FEString *string = FEngFindString("ScreenPrintf.fng", FEHashUpper(text));

        if (string) {
            FEngSetInvisible(string);
        }

        ScreenPrintItemTable[n].pFEString = string;
        ScreenPrintItemTable[n].PosX = 0;
        ScreenPrintItemTable[n].PosY = 0;
        ScreenPrintItemTable[n].Lifetime = -1.0f;
    }
}

// The 4:3 positions, pushed out to the sides of a wide screen (not while the camera editor is on).
static void FixWidescreen(int &x) {
    if (!TheICEManager.IsEditorOn() && FEDatabase && FEDatabase->GetVideoSettings()->WideScreen) {
        if (x < -10) {
            x -= 120;
        } else if (x > 10) {
            x += 120;
        }
    }
}

void ScreenPrintf(int x, int y, float duration, unsigned int color, const char *fmt, va_list &argList) {
    FixWidescreen(x);

    ScreenPrintItem *item;
    if (duration != 0.0f) {
        item = GetScreenPrintItemAtLocation(x, y);
        if (!item) {
            item = GetUnusedScreenPrintItem();
        }
    } else {
        item = GetUnusedScreenPrintItem();
    }

    if (item && DoScreenPrintf) {
        item->Lifetime = duration;
        item->PosX = x;
        item->PosY = y;

        char flailer_print_buffer[128];
        bVSPrintf(flailer_print_buffer, fmt, argList);

        FEString *text_chunk = item->pFEString;
        if (text_chunk) {
            text_chunk->SetColor(FEColor(color), false);
            FEPrintf(text_chunk, flailer_print_buffer);
            FEngSetTopLeft(text_chunk, static_cast<float>(x), static_cast<float>(y));
            FEngSetVisible(text_chunk);
        }
    }
}

void CancelScreenPrintf(int x, int y) {
    ScreenPrintf(x, y, -1.0f, "");
}

void ScreenPrintf(int x, int y, const char *fmt, ...) {
    va_list argList;
    va_start(argList, fmt);
    ScreenPrintf(x, y, 0.0f, 0xFFFFFFFF, fmt, argList);
}

void ScreenPrintf(int x, int y, float duration, const char *fmt, ...) {
    va_list argList;
    va_start(argList, fmt);
    ScreenPrintf(x, y, duration, 0xFFFFFFFF, fmt, argList);
}

void ScreenPrintf(int x, int y, unsigned int color, const char *fmt, ...) {
    va_list argList;
    va_start(argList, fmt);
    ScreenPrintf(x, y, 0.0f, color, fmt, argList);
}

void ScreenPrintf(int x, int y, float duration, unsigned int color, const char *fmt, ...) {
    va_list argList;
    va_start(argList, fmt);
    ScreenPrintf(x, y, duration, color, fmt, argList);
}

// A black copy one pixel down and to the right, then the text in white.
void ScreenShadowPrintf(int x, int y, const char *fmt, ...) {
    va_list argList;
    va_start(argList, fmt);

    float duration = 0.0f;
    ScreenPrintf(x + 1, y + 1, duration, 0x80000000, fmt, argList);
    ScreenPrintf(x, y, duration, 0xFFFFFFFF, fmt, argList);
}

// Once a frame: ages the items and hides the ones that run out.
void ResetScreenPrintf() {
    for (int i = 0; i < 64; i++) {
        ScreenPrintItem *item = &ScreenPrintItemTable[i];
        if (item->Lifetime >= 0.0f) {
            item->Lifetime -= RealTimeElapsed;
            if (item->Lifetime < 0.0f) {
                FEString *text_chunk = item->pFEString;
                if (text_chunk) {
                    FEngSetInvisible(text_chunk);
                }
            }
        }
    }
}
