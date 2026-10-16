#include "Speed/Indep/Src/Frontend/MenuScreens/InGame/FEPkg_MU_Keyboard.hpp"

const FEColor FEKeyboard::ButtonHighlight(0xC8CFE9F2); // size: 0x10, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:87

const FEColor FEKeyboard::LetterHighlight(0xFFFFFFFF); // size: 0x10, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:88

const FEColor
    FEKeyboard::ButtonIdle(0x50549AC0); // size: 0x10, address: 0x80473048, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:89

const FEColor
    FEKeyboard::LetterIdle(0xFF323232); // size: 0x10, address: 0x80473058, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:90

const char FEKeyboard::mLetterMap[8][2][45] = {
    {{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', '_', '-', '.', '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
      'i', 'j', 'k', 'l', 'm', '?', '!', 'n', 'o', 'p', 'q',  'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ',', '&'},
     {'/', '\\', '|', '`', '~', '#', '%', '+', '*', '=', '\"', ':', ';', '(', ')', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
      'I', 'J',  'K', 'L', 'M', '<', '>', 'N', 'O', 'P', 'Q',  'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', ']'}},
    {{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', '_', '-', '.', '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
      'i', 'j', 'k', 'l', 'm', '?', '!', 'n', 'o', 'p', 'q',  'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ',', '&'},
     {'/', '\\', '|', '`', '~', '#', '%', '+', '*', '=', '\"', ':', ';', '(', ')', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
      'I', 'J',  'K', 'L', 'M', '<', '>', 'N', 'O', 'P', 'Q',  'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', ']'}},
    {{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', '_', '-', '.', '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
      'i', 'j', 'k', 'l', 'm', '?', '!', 'n', 'o', 'p', 'q',  'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ',', '&'},
     {'/', '\\', '|', '`', '~', '#', '%', '+', '*', '=', '\"', ':', ';', '(', ')', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
      'I', 'J',  'K', 'L', 'M', '<', '>', 'N', 'O', 'P', 'Q',  'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', ']'}},
    {{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', '_', '-', '.', '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
      'i', 'j', 'k', 'l', 'm', '?', '!', 'n', 'o', 'p', 'q',  'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ',', '&'},
     {'/', '\\', '|', '`', '~', '#', '%', '+', '*', '=', '\"', ':', ';', '(', ')', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
      'I', 'J',  'K', 'L', 'M', '<', '>', 'N', 'O', 'P', 'Q',  'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', ']'}},
    {{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', '_', '-', '.', '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
      'i', 'j', 'k', 'l', 'm', '?', '!', 'n', 'o', 'p', 'q',  'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ',', '&'},
     {'/', '\\', '|', '`', '~', '#', '%', '+', '*', '=', '\"', ':', ';', '(', ')', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
      'I', 'J',  'K', 'L', 'M', '<', '>', 'N', 'O', 'P', 'Q',  'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', ']'}},
    {{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', '_', '-', '.', '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
      'i', 'j', 'k', 'l', 'm', '?', '!', 'n', 'o', 'p', 'q',  'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ',', '&'},
     {'/', '\\', '|', '`', '~', '#', '%', '+', '*', '=', '\"', ':', ';', '(', ')', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
      'I', 'J',  'K', 'L', 'M', '<', '>', 'N', 'O', 'P', 'Q',  'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', ']'}},
    {{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\'', '_', '-', '.', '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
      'i', 'j', 'k', 'l', 'm', '?', '!', 'n', 'o', 'p', 'q',  'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ',', '&'},
     {'/', '\\', '|', '`', '~', '#', '%', '+', '*', '=', '\"', ':', ';', '(', ')', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
      'I', 'J',  'K', 'L', 'M', '<', '>', 'N', 'O', 'P', 'Q',  'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', ']'}},
    {{0xFA, 0xF9, 0xFB, 0xFC, 0xDF, 0xA1, 0xBF, 0,    0,    0,    0,    0,    0,    0,    0,    0xE1, 0xE0, 0xE2, 0xE5, 0xE4, 0xE6, 0xE7, 0xE9,
      0xE8, 0xEA, 0xEB, 0,    0,    0,    0,    0xED, 0xEC, 0xEE, 0xEF, 0xF1, 0xF8, 0xF3, 0xF2, 0xF4, 0xF6, 0x9C, 0,    0,    0,    0},
     {0xDA, 0xD9, 0xDB, 0xDC, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0xC1, 0xC0, 0xC2, 0xC5, 0xC4, 0xC6, 0xC7, 0xC9,
      0xC8, 0xCA, 0xCB, 0,    0, 0, 0, 0xCD, 0xCC, 0xCE, 0xCF, 0xD1, 0xD8, 0xD3, 0xD2, 0xD4, 0xD6, 0,    0,    0,    0,    0}}};
; // size: 0x2D0, address: 0x803E59E5, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:106

const char mEmailSymbols[45] = {}; // size: 0x2D, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:107

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/FEngFont.hpp"
#include "Speed/Indep/Src/Frontend/FEngFrontend.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/Generated/FEngHash/FEHash_UI_DebugCarCustomize.hpp"

Timer KBCreationTimer;
int KeyboardActive = false; // int en el DWARF de GameCube y en los stabs de la A124
FEKeyboard *gFEKeyboard = nullptr;

MenuScreen *CreateFEKeyboard(ScreenConstructorData *sd) {
    return new ("FEKeyboard", 0) FEKeyboard(sd);
}

FEKeyboard::FEKeyboard(ScreenConstructorData *sd) : MenuScreen(sd) {
#ifndef EA_BUILD_A124
    mnWindowStartIdx = 0;
#endif
    mThis = sd->pPackage;
    KBCreationTimer = RealTimer;
    Initialize();
    UpdateVisuals();
}

// STRIPPED
void FEKeyboard::ShowModal(const char *pstrParent, MODE nMode, u32 nAcceptHash, int nDeclineHash, int nMaxLength, u32 nDefaultTextHash) {}

void FEKeyboard::Dispose(bool bBack) {
    if (bBack) {
        bMemSet(mString, 0, 0x9c);
    }
    if (bBack == true) {
        cFEng::Get()->QueueGameMessage(mnDeclineHash, mThis->GetParentPackage()->GetName(), 0xff);
    } else {
        cFEng::Get()->QueueGameMessage(mnAcceptHash, mThis->GetParentPackage()->GetName(), 0xff);
    }
    cFEng::Get()->QueuePackagePop(1);
    gFEKeyboard = nullptr;
    KeyboardActive = false;
}

void FEKeyboard::NotificationMessage(u32 msg, FEObject *pObject, u32 param1, u32 param2) {
    int nButton = -1;
    switch (msg) {
        case 0x9120409E:
            g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(2));
            break;
        case 0xB5971BF1:
            g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(3));
            break;
        case 0x72619778:
            g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(0));
            break;
        case 0x911C0A4B:
            g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(1));
            break;
        case 0xC407210:
            if (pObject) {
                nButton = IsKeyButton(pObject);
                if (nButton > -1 && GetLetterMap(nButton) != 0) {
                    g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(0x2E));
                    AppendLetter(nButton);
                } else {
                    g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(7));
                }
            }
            break;
        case 0xE1FDE1D1:
            if (bStrCmp(mString, "") != 0 || mnMode != MODE_FILENAME) {
                g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(0x2F));
                Dispose(false);
            }
            break;
        case 0xC1A6F000:
            if (mnMode == MODE_PROFILE_ENTRY) {
                g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(7));
            } else {
                AppendSpace();
                g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(0x2E));
            }
            break;
        case 0xDB3D597C:
            g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(0x30));
            AppendBackspace();
            break;
        case 0xD7AD0DD9:
#ifndef EA_PLATFORM_PLAYSTATION2
            if (mnMode == MODE_PROFILE_ENTRY && !(FEDatabase->GetGameMode() & 8) && !(FEDatabase->GetGameMode() & 0x40)) {
                g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(7));
                break;
            }
#endif
            ToggleCapsLock();
            g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(0x30));
            break;
        case 0xB5AF2461:
            if (bStrCmp(mString, "") == 0) {
                cFEng::Get()->QueuePackageMessage(__ENABLE_INPUTS__, GetPackageName(), nullptr);
            } else {
                g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(0x2F));
                Dispose(false);
            }
            break;
        case 0x5073EF13:
            MoveCursor(-1);
            break;
        case 0xD9FEEC59:
            MoveCursor(1);
            break;
        case 0xC519BFC4:
            if (GetCurrentLanguage() != eLANGUAGE_KOREAN) {
                ToggleSpecialCharacters();
#ifdef EA_BUILD_A124 // la A124 suena al cambiar a los caracteres especiales
                g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(0x2E));
#endif
            }
            break;
        case 0x911AB364:
            if (mnDeclineHash != -1U) {
                Dispose(true);
            }
            break;
    }
}

void FEKeyboard::Initialize() {
    mString = FEDatabase->mFEKeyboardSettings.Buffer;
    SetMaxLength(FEDatabase->mFEKeyboardSettings.MaxTextLength);
    mnAcceptHash = FEDatabase->mFEKeyboardSettings.AcceptCallbackHash;
    mnDeclineHash = FEDatabase->mFEKeyboardSettings.DeclineCallbackHash;
    mnMode = static_cast<MODE>(FEDatabase->mFEKeyboardSettings.Mode);

    mnCursorIndex = 0;

    // Un caso por linea (A124 416-426): las dos PS2 guardan cada indice con su constante; GameCube compara en arbol y
    // sustituye las de los `beq' por el propio idioma.
    eLanguages language = GetCurrentLanguage();
    switch (language) {
        case eLANGUAGE_FRENCH: mnLetterMapIndex = 1; break;
        case eLANGUAGE_GERMAN: mnLetterMapIndex = 2; break;
        case eLANGUAGE_ITALIAN: mnLetterMapIndex = 3; break;
        case eLANGUAGE_SPANISH: mnLetterMapIndex = 4; break;
        case eLANGUAGE_DUTCH: mnLetterMapIndex = 5; break;
        case eLANGUAGE_SWEDISH: mnLetterMapIndex = 6; break;
        case eLANGUAGE_ENGLISH:
        default:
            mnLetterMapIndex = 0;
            break;
    }

    FEngSetButtonTexture(FEngFindImage(GetPackageName(), 0x5BC), 0x5BC);
    FEngSetButtonTexture(FEngFindImage(GetPackageName(), 0x682), 0x682);

    mbIsFirstKey = true;

    mbShift = false;
    mbCaps = false;
    mbOnSpecialCharacters = false;
    if (mnMode == MODE_FILENAME || mnMode == MODE_PROFILE_ENTRY) {
        mbCaps = true;
    }

#ifdef EA_BUILD_A124
    mpInputString = static_cast<FEString *>(FEngFindObject(GetPackageName(), 0xADDA7E89));
#else
    mpInputString = FEngFindString(GetPackageName(), 0xADDA7E89);
#endif
    mpCursor = FEngFindObject(GetPackageName(), 0xA2DEEF46);
    mpTextBox = FEngFindImage(GetPackageName(), 0x128FDCB8);
#ifndef EA_BUILD_A124
    mpCursorTestString = reinterpret_cast<FEString *>(FEngFindObject(GetPackageName(), 0x95D7D3D2));
#endif

    char tmp[32];
    for (int i = 0; i < 0x2D; i++) {
        unsigned int letter_hash;

        FEngSNPrintf(tmp, 0x20, "KEYNUM %.2d", i + 1);
        letter_hash = FEHashUpper(tmp);
        mpKeyName[i] = reinterpret_cast<FEString *>(FEngFindObject(GetPackageName(), letter_hash));

        FEngSNPrintf(tmp, 0x20, "KEYNUM SHADOW %.2d", i + 1);
        letter_hash = FEHashUpper(tmp);
        mpKeyNameShadow[i] = reinterpret_cast<FEString *>(FEngFindObject(GetPackageName(), letter_hash));

        FEngSNPrintf(tmp, 0x20, "KEY %.2d", i + 1);
        letter_hash = FEHashUpper(tmp);
        mpKeyButton[i] = FEngFindObject(GetPackageName(), letter_hash);

        FEngSNPrintf(tmp, 0x20, "KEY %.2d DISABLE", i + 1);
        letter_hash = FEHashUpper(tmp);
        mpKeyDisable[i] = FEngFindObject(GetPackageName(), letter_hash);
    }

    // `!=' entre dos eLanguages: en PS2 pasa por el operator!= de stl_relops.h (xori de la A124 y SLUS).
    if (!(mnMode == MODE_ALL_KEYS && GetCurrentLanguage() != eLANGUAGE_KOREAN)) {
        FEngSetInvisible(GetPackageName(), 0x2C99C4E2);
        FEngSetScript(GetPackageName(), 0xDCBC8286, 0x1CA7C0, true);
    }

#ifndef EA_PLATFORM_PLAYSTATION2 // ninguna de las dos PS2 tiene la tecla 61 del perfil
    if (mnMode == MODE_PROFILE_ENTRY) {
        FEngSetScript(GetPackageName(), FEHashUpper("KEY 61 DISABLE"), FEHashUpper("SHOW"), true);
    }
#endif

    FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0x7F042BCD);

#ifdef EA_BUILD_A124
    if (mString) {
        SetString(mString);
    } else {
        SetString("");
    }
#else
    FEString *inputString = mpInputString;
    inputString->string = mDisplayString;
    inputString->Flags |= FF_DirtyCode;
#endif

    mnCursorIndex = bStrLen(mString);
    KeyboardActive = true;
    gFEKeyboard = this;
}

int FEKeyboard::GetCase() {
    if (mbShift) {
        return mbCaps ? 0 : 1;
    }
    return mbCaps ? 1 : 0;
}

void FEKeyboard::UpdateVisuals() {
    const u32 SHOW_SCRIPT = 0x16A259;
    const u32 HIDE_SCRIPT = 0x1CA7C0;

    for (int i = 0; i < 0x2D; i++) {
        char ch = GetLetterMap(i);
        if (GetCurrentLanguage() == eLANGUAGE_KOREAN) {
            short bla[16];
            bMemSet(bla, 0, 0x20);
            bla[0] = static_cast<short>(ch);
            mpKeyName[i]->SetString(bla);
            mpKeyNameShadow[i]->SetString(bla);
        } else {
            FEPrintf(mpKeyName[i], "%c", ch);
            FEPrintf(mpKeyNameShadow[i], "%c", ch);
        }

        bool disabled = ch == 0;
        if (!disabled) {
            if (mnMode - 1U < 3) {
                disabled = IsSymbol(ch);
            } else if (mnMode == MODE_EMAIL) {
                disabled = IsNotOkForEmail(ch);
            }
        }

        FEngSetScript(mpKeyDisable[i], disabled ? HIDE_SCRIPT : SHOW_SCRIPT, true);
    }

    if (GetCase() != 0) {
        FEngSetLanguageHash(GetPackageName(), 0xDE045B20, 0x36F274D0);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0xDE045B20, 0x20BC9973);
    }
    UpdateStringVisual();
}

void FEKeyboard::UpdateStringVisual() {
    mpInputString->Flags |= 2;
    if (mnMode == MODE_ALPHANUMERIC_PASSWORD) {
        char aHidden[64];
        FEngSNMakeHidden(aHidden, 0x40, mString);
        SetString(aHidden);
    } else {
#ifdef EA_BUILD_A124
        // La A124 recorta la cadena por delante hasta que cabe en la caja.
        SetString(mString);
        while (FEngGetSizeX(mpInputString) > FEngGetSizeX(mpTextBox) - 20.0f) {
            int len = bStrLen(mString);
            char *fitstring = mString;
            SetString(fitstring + 1);
        }
#else
        unsigned short widestring[156];
        FEngFont *font;
        int width;
        u32 flags;
        short *fitstring;

        mnWindowStartIdx = 0;
#ifdef EA_PLATFORM_PLAYSTATION2
        bStrCpy(widestring, mString);
#else
        PackedStringToWideString(widestring, 0x9C, mString);
#endif
        font = FindFont(mpInputString->Handle);
        width = mpInputString->MaxWidth;
        flags = mpInputString->Flags;
        fitstring = reinterpret_cast<short *>(widestring);
        while (*fitstring != 0 && font->GetLineWidth(fitstring, flags, 0, false) > static_cast<float>(width)) {
            fitstring += 4;
            mnWindowStartIdx = mnWindowStartIdx + 4;
        }

        while (mnCursorIndex < mnWindowStartIdx) {
            if (mnWindowStartIdx - 4 < 0) {
                fitstring = reinterpret_cast<short *>(widestring);
                mnWindowStartIdx = 0;
            } else {
                mnWindowStartIdx = mnWindowStartIdx - 4;
                fitstring = fitstring - 4;
            }
        }

        while (*fitstring != 0 && font->GetLineWidth(fitstring, flags, 0, false) > static_cast<float>(width)) {
            widestring[bStrLen(widestring) - 1] = 0;
        }

        FESetString(mpInputString, fitstring);
#endif
    }
    UpdateCursorPosition();
}

void FEKeyboard::UpdateCursorPosition() {
#ifdef EA_BUILD_A124
    short *pSrc = mpInputString->GetString();
    char pBuf[500];
    bMemSet(pBuf, 0, 500);
    WideToCharString(pBuf, mnCursorIndex + 1, pSrc);
    FEPrintf(mpInputString, "%s", pBuf);
    float width = FEngGetSizeX(mpInputString);
    int maxLineWidth = mpInputString->MaxWidth;
    float string_x = mpInputString->GetObjData()->Pos.x;
    float cursor_x = string_x + width;
    FEngSetTopLeftX(mpCursor, cursor_x);
    FEPrintf(mpInputString, "%s", mString);
#else
    mpCursorTestString->Flags |= 2;
    short *pSrc = mpInputString->string;
    char pBuf[150];
    bMemSet(pBuf, 0, 0x96);
    WideToCharString(pBuf, static_cast<unsigned int>(mnCursorIndex - mnWindowStartIdx + 1), pSrc);
    FEPrintf(GetPackageName(), mpCursorTestString, "%s", pBuf);
    float width = FEngGetSizeX(mpCursorTestString);
    float string_x = reinterpret_cast<float *>(mpInputString->pData)[7];
    float cursor_x = string_x + width;
    FEngSetTopLeftX(mpCursor, cursor_x);
#endif
}

void FEKeyboard::MoveCursor(int nDelta) {
    mbIsFirstKey = false;
    mnCursorIndex = mnCursorIndex + nDelta;
    if (mnCursorIndex < 0) {
        mnCursorIndex = 0;
    }
    int stringLength = strlen(mString);
    if (mnCursorIndex >= stringLength) {
        mnCursorIndex = stringLength;
    }
    UpdateVisuals();
}

// STRIPPED
void FEKeyboard::HighlightButton(int nButton, bool bHighlight) {}

void FEKeyboard::SetString(char *pStr) {
#ifdef EA_BUILD_A124
    bStrNCpy(mString, pStr, 0x9C);
    mString[0x9C] = 0;
    mpInputString->SetString(mString);
#else
    FEPrintf(mpInputString, pStr);
#endif
}

void FEKeyboard::SetMaxLength(int nLength) {
    if (nLength > 0x9C) {
        nLength = 0x9C;
    }
    mnMaxLength = nLength;
}

int FEKeyboard::IsKeyButton(FEObject *pObject) {
    for (int i = 0; i < 0x2D; i++) {
        if (mpKeyButton[i] == pObject) {
            return i;
        }
    }
    return -1;
}

bool FEKeyboard::IsSymbol(char character) {
    char symbols[28] = {'-', '=', '/', '?', '\\', ';', '\'', ',', '.', '!', '@', '#',  '~', '%',
                        '`', '&', '*', '(', ')',  '_', '+',  '[', ']', '|', ':', '\"', '<', '>'};
    for (int i = 0; i < sizeof(symbols); i++) {
        if (character == symbols[i]) {
            return true;
        }
    }
    return false;
}

bool FEKeyboard::IsNotOkForEmail(char character) {
    char symbols[24] = {'=', '/', '?', '\\', ';', '\'', ',', '!', '#', '~', '%', '`', '&', '*', '(', ')', '+', '[', ']', '|', ':', '\"', '<', '>'};
    for (int i = 0; i < sizeof(symbols); i++) {
        if (character == symbols[i]) {
            return true;
        }
    }
    return false;
}

bool FEKeyboard::IsNumericSymbol(char character) {
    char symbols[10] = {'/', '\\', '|', '`', '~', '#', '%', '+', '*', '='};
    for (int i = 0; i < sizeof(symbols); i++) {
        if (character == symbols[i]) {
            return true;
        }
    }
    return false;
}

bool FEKeyboard::IsEmailSymbol(char character) {
    char symbols[4] = {':', ';', '(', ')'};
    for (int i = 0; i < sizeof(symbols); i++) {
        if (character == symbols[i]) {
            return true;
        }
    }
    return false;
}

void FEKeyboard::AppendLetter(int nButton) {
    AppendChar(GetLetterMap(nButton));
}

char FEKeyboard::GetLetterMap(int nButton) {
    char returnChar = mLetterMap[mnLetterMapIndex][GetCase()][nButton];
    if (mnMode == MODE_ALL_KEYS && mbOnSpecialCharacters) {
        returnChar = mLetterMap[7][GetCase()][nButton];
    } else if (mnMode == MODE_ALPHANUMERIC || mnMode == MODE_ALPHANUMERIC_PASSWORD || mnMode == MODE_FILENAME) {
        if (IsSymbol(returnChar)) {
            return 0;
        }
    } else if (mnMode == MODE_EMAIL) {
        if (IsEmailSymbol(returnChar) || IsNumericSymbol(returnChar)) {
            returnChar = mLetterMap[mnLetterMapIndex][0][nButton];
        }
        if (IsNotOkForEmail(returnChar)) {
            return 0;
        }
    } else if (mnMode == MODE_PROFILE_ENTRY) {
        if (IsNumericSymbol(returnChar)) {
            returnChar = mLetterMap[mnLetterMapIndex][0][nButton];
        } else if (IsSymbol(returnChar)) {
            return 0;
        }
    }
    return returnChar;
}

void FEKeyboard::AppendSpace() {
    if (mnMode == MODE_ALL_KEYS) {
        AppendChar(0x20);
    }
}

void FEKeyboard::AppendBackspace() {
    if (mbIsFirstKey) {
        mbIsFirstKey = false;
#ifdef EA_BUILD_A124
        SetString("");
#else
        mString[0] = 0;
#endif
        mnCursorIndex = 0;
    }
    int len = bStrLen(mString);
    if (len > 0 && mnCursorIndex > 0) {
        int i = mnCursorIndex - 1;
        for (; i < len; i++) {
            mString[i] = mString[i + 1];
        }
        mString[len - 1] = 0;
        mnCursorIndex = mnCursorIndex - 1;
    }
    UpdateStringVisual();
}

void FEKeyboard::AppendChar(char ch) {
    if (!ch) {
        return;
    }
    if (mbIsFirstKey) {
        mbIsFirstKey = false;
#ifdef EA_BUILD_A124
        SetString("");
#else
        mString[0] = 0;
#endif
        mnCursorIndex = 0;
    }
    int len = bStrLen(mString);
    if (len < mnMaxLength) {
        int i = 0x9A;
        for (; i > mnCursorIndex; i--) {
            mString[i] = mString[i - 1];
        }
        mString[mnCursorIndex] = ch;
        mString[0x9B] = 0;
        mnCursorIndex = mnCursorIndex + 1;
        if (mnCursorIndex > mnMaxLength) {
            mnCursorIndex = mnMaxLength;
        }
        UpdateStringVisual();
    }
    if (mbShift && ch != 0x20) {
        ToggleShift();
    }
}

void FEKeyboard::ToggleCapsLock() {
    // GameCube y PC no cambian de mayusculas al escribir el perfil; las dos PS2 (A124 y SLUS) no lo comprueban.
#ifndef EA_PLATFORM_PLAYSTATION2
    if (mnMode != MODE_PROFILE_ENTRY) {
#endif
        mbCaps = mbCaps != 1;
        mbShift = false;
        if (mnMode == MODE_FILENAME) {
            mbCaps = true;
            mbShift = false;
        }
        UpdateVisuals();
#ifndef EA_PLATFORM_PLAYSTATION2
    }
#endif
}

void FEKeyboard::ToggleShift() {
    mbShift = mbShift != 1;
    if (mnMode == MODE_FILENAME) {
        mbShift = false;
    }
    UpdateVisuals();
}

void FEKeyboard::ToggleSpecialCharacters() {
#ifdef EA_BUILD_A124 // la A124 suena desde NotificationMessage
    mbOnSpecialCharacters = mbOnSpecialCharacters != 1;
    if (mnMode != MODE_ALL_KEYS) {
        mbOnSpecialCharacters = false;
    }
    UpdateVisuals();
#else
    bool newVal = true;
    if (mbOnSpecialCharacters == true) {
        newVal = false;
    }
    mbOnSpecialCharacters = newVal;
    if (mnMode == MODE_ALL_KEYS) {
        g_pEAXSound->PlayUISoundFX(static_cast<eMenuSoundTriggers>(0x2E));
    } else {
        mbOnSpecialCharacters = false;
    }
    UpdateVisuals();
#endif
}
