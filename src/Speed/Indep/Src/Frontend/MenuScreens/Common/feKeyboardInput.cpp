#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feKeyboardInput.hpp"

#include "Speed/Indep/Src/Frontend/FEJoyInput.hpp"
#include "Speed/Indep/Src/Frontend/FEngFont.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

KeyboardEditString gKeyboardManager;

KeyboardEditString::KeyboardEditString() {
    TextInputObject = nullptr;
    MaxTextLength = 0;
#ifdef EA_BUILD_A124 // la A124 edita la cadena de 8 bits, sin la copia UCS2
    bMemSet(EditString, 0, 0x100);
#else
    bMemSet(EditStringUCS2, 0, 0x200);
    CursorPosUCS2 = 0;
    bMemSet(EditStringPacked, 0, 0x100);
#endif
    bMemSet(InitialString, 0, 0x100);
    mEnabled = false;
}

#ifndef EA_BUILD_A124
void KeyboardEditString::SyncEditIntoPacked() {
    WideStringToPackedString(EditStringPacked, 0x100, EditStringUCS2);
}

char *KeyboardEditString::GetEditedString() {
    SyncEditIntoPacked();
    return EditStringPacked;
}
#endif

// Solo la llama el constructor de FEngTextInputObject, que solo usa PC (speed.exe 0x57F1D0, 164 B): GameCube y las PS2
// la definen y su enlazador la quita (stabs de la A124: StartCapture 0x84 B con la local `a`, direccion 0xFFFFFFFF).
void KeyboardEditString::StartCapture(FEngTextInputObject *txt, uint32 mode, const char *start_string, uint32 max_text_length) {
    TextInputObject = txt;
    ModeFlags = mode;
    int a = max_text_length;
    if (a > 0x9B) {
        a = 0x9B;
    }
    MaxTextLength = a;
#ifdef EA_BUILD_A124
    bStrNCpy(EditString, start_string, 0x100);
#else
    PackedStringToWideString(EditStringUCS2, 0x200, start_string);
    SyncEditIntoPacked();
#endif
    bStrNCpy(InitialString, start_string, 0x100);
#ifdef EA_BUILD_A124
    CursorPos = bStrLen(EditString);
#else
    CursorPosUCS2 = bStrLen(EditStringUCS2);
#endif
    KeysProcessed = 0;
    mEnabled = true;
}

void KeyboardEditString::EndCapture() {
    mEnabled = false;
    TextInputObject = nullptr;
#ifdef EA_BUILD_A124
    bMemSet(EditString, 0, 0x100);
#else
    bMemSet(EditStringUCS2, 0, 0x200);
    bMemSet(EditStringPacked, 0, 0x100);
#endif
    bMemSet(InitialString, 0, 0x100);
}

void KeyboardEditString::GetStringForDisplay(char *buffer, int size) {
#ifdef EA_BUILD_A124
    if (ModeFlags == 5) {
        FEngSNMakeHidden(buffer, size, EditString);
    } else {
        bStrNCpy(buffer, EditString, size);
    }
#else
    SyncEditIntoPacked();
    if (ModeFlags == 5) {
        FEngSNMakeHidden(buffer, size, EditStringUCS2);
    } else {
        bStrNCpy(buffer, EditStringPacked, size);
    }
#endif
}

void KeyboardEditString::RevertToOriginalString() {
#ifdef EA_BUILD_A124
    bStrNCpy(EditString, InitialString, 0x100);
#else
    PackedStringToWideString(EditStringUCS2, 0x200, InitialString);
    SyncEditIntoPacked();
#endif
}

// DWARF de GameCube y stabs de la A124: definido aqui y quitado por el enlazador; en PC va en linea en FEngBeginTextInput.
FEngTextInputObject::FEngTextInputObject(MenuScreen *pkg, FEString *obj, uint32 mode, const char *start_string, uint32 max_text_length) {
    DisplayString = obj;
    ParentPackage = pkg;
    mBlinkTime = 0;
    gKeyboardManager.StartCapture(this, mode, start_string, max_text_length);
}

FEngTextInputObject::~FEngTextInputObject() {
    gKeyboardManager.EndCapture();
}

void FEngTextInputObject::ReturnPressed() {
    if (gKeyboardManager.GetModeFlags() == 6 && bStrLen(gKeyboardManager.GetEditedString()) == 0) {
        return;
    }

    cFEngJoyInput::Get()->FlushActions();
    RedrawString(false);
    ParentPackage->NotificationMessage(0xDA5B8712, reinterpret_cast<FEObject *>(DisplayString), 0, 0);
    gKeyboardManager.EndCapture();
    ParentPackage->FEngEndTextInput();
}

void FEngTextInputObject::EscapePressed() {
    gKeyboardManager.RevertToOriginalString();
    RedrawString(false);
    ParentPackage->NotificationMessage(0xC9D30688, reinterpret_cast<FEObject *>(DisplayString), 0, 0);
    gKeyboardManager.EndCapture();
    ParentPackage->FEngEndTextInput();
}

void FEngTextInputObject::RedrawString(bool pIncludeCursor) {
    if (DisplayString == nullptr) {
        return;
    }

    char buffer[156];
    unsigned short widestring[156];
    gKeyboardManager.GetStringForDisplay(buffer, 0x9C);

    if (pIncludeCursor) {
        mBlinkTime++;
        if (mBlinkTime >= 0x5a) {
            mBlinkTime = 0;
        }
        if (mBlinkTime <= 0x2C) {
            bStrCat(buffer, buffer, "|");
        } else {
            bStrCat(buffer, buffer, " ");
        }
    }

    bStrCpy(widestring, buffer);
    short *fitstring = reinterpret_cast<short *>(widestring);
    FEngFont *font = FindFont(DisplayString->Handle);
    int width = DisplayString->MaxWidth;
    int flags = DisplayString->Flags;
    while (fitstring[0] != 0 && font->GetLineWidth(fitstring, flags, 0, false) > static_cast<float>(width)) {
        fitstring++;
    }
    FESetString(DisplayString, fitstring);
}

void FEngTextInputObject::Notify(uint32 msg) {
    if (msg == 0xC98356BA) {
        RedrawString(true);
    } else if (msg == 0x0C407210) {
        ReturnPressed();
    }
}
