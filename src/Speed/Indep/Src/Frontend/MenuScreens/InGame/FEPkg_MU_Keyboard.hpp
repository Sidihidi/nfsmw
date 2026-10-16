#ifndef __FEPKG_KEYBOARD_HPP__
#define __FEPKG_KEYBOARD_HPP__

#include "Speed/Indep/Src/FEng/FEImage.h"
#include "Speed/Indep/Src/FEng/FEPackage.h"
#include "Speed/Indep/Src/FEng/FEString.h"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"

// File: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp
// total size: 0x360
// Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:25
class FEKeyboard : public MenuScreen {
  public:
    typedef enum {
        MODE_ALL_KEYS = 0,
        MODE_ALPHANUMERIC = 1,
        MODE_ALPHANUMERIC_PASSWORD = 2,
        MODE_FILENAME = 3,
        MODE_EMAIL = 4,
        MODE_PROFILE_ENTRY = 5
    } MODE;

    FEKeyboard(ScreenConstructorData *sd);
    ~FEKeyboard() override {} // Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:29

    static void ShowModal(const char *pstrParent, MODE nMode, u32 nAcceptHash, int nDeclineHash, int nMaxLength, u32 nDefaultTextHash);

    void NotificationMessage(u32 msg, FEObject *pObject, u32 param1, u32 param2) override;

    void AppendChar(char ch);

    void AppendSpace();

    void AppendBackspace();

    void MoveCursor(int nDelta);

    void Dispose(bool bBack);

  private:
    void SetString(char *pStr);

    void SetMaxLength(int nLength);

    void Initialize();

    void UpdateVisuals();

    void UpdateStringVisual();

    void UpdateCursorPosition();

    int IsKeyButton(FEObject *pObject);

    bool IsSymbol(char character);

    bool IsNumericSymbol(char character);

    bool IsEmailSymbol(char character);

    bool IsNotOkForEmail(char character);

    void AppendLetter(int nButton);

    char GetLetterMap(int nButton);

    void HighlightButton(int nButton, bool bHighlight);

    void ToggleSpecialCharacters();

    void ToggleCapsLock();

    void ToggleShift();

    int GetCase();

  public:
    static const char *KeyboardPackage; // size: 0x4, address: 0xFFFFFFFF

  private:
    static const FEColor
        ButtonHighlight; // size: 0x10, address: 0x80473028, Decl: speed/indep/src/frontend/menuscreens/ingame/FEpkg_MU_Keyboard.cpp:171
    static const FEColor
        LetterHighlight;             // size: 0x10, address: 0x80473038, Decl: speed/indep/src/frontend/menuscreens/ingame/FEpkg_MU_Keyboard.cpp:172
    static const FEColor ButtonIdle; // size: 0x10, address: 0x80473048, Decl: speed/indep/src/frontend/menuscreens/ingame/FEpkg_MU_Keyboard.cpp:173
    static const FEColor LetterIdle; // size: 0x10, address: 0x80473058, Decl: speed/indep/src/frontend/menuscreens/ingame/FEpkg_MU_Keyboard.cpp:174

    int mnLetterMapIndex;       // offset 0x2C, size 0x4, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:100
    int mnCursorIndex;          // offset 0x30, size 0x4
    int mnMaxLength;            // offset 0x34, size 0x4
    bool mbIsFirstKey;          // offset 0x38, size 0x1
    bool mbShift;               // offset 0x3C, size 0x1, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:102
    bool mbCaps;                // offset 0x40, size 0x1, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:103
    bool mbOnSpecialCharacters; // offset 0x44, size 0x1, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:104
    FEString *mpInputString;    // offset 0x48, size 0x4
    FEObject *mpCursor;         // offset 0x4C, size 0x4
    FEImage *mpTextBox;         // offset 0x50, size 0x4

    static const char mLetterMap[8][2]
                                [45]; // size: 0x2D0, address: 0x803E59E5, Decl: speed/indep/src/frontend/menuscreens/ingame/FEpkg_MU_Keyboard.cpp:32
    static const char
        mEmailSymbols[45]; // size: 0x2D, address: 0xFFFFFFFF, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:107

    FEString *mpKeyName[45];       // offset 0x54, size 0xB4, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:109
    FEString *mpKeyNameShadow[45]; // offset 0x108, size 0xB4, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:110
    FEObject *mpKeyButton[45];     // offset 0x1BC, size 0xB4, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:111
    FEObject *mpKeyDisable[45];    // offset 0x270, size 0xB4, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:112
    char *mString;                 // offset 0x324, size 0x4
    FEPackage *mThis;              // offset 0x328, size 0x4, Decl: speed/indep/src/frontend/menuscreens/ingame/FEPkg_MU_Keyboard.hpp:114
    u32 mnAcceptHash;              // offset 0x32C, size 0x4
    u32 mnDeclineHash;             // offset 0x330, size 0x4
    MODE mnMode;                   // offset 0x334, size 0x4

#ifndef EA_BUILD_A124 // la A124 no tiene la ventana de texto: su FEKeyboard mide 0x338 (stabs y new de CreateFEKeyboard)
    static const int WindowSize;     // size: 0x4, address: 0xFFFFFFFF
    static const int WindowSkipSize; // size: 0x4, address: 0xFFFFFFFF
    int mnWindowStartIdx;            // offset 0x338, size 0x4
    FEString *mpCursorTestString;    // offset 0x33C, size 0x4
    char mDisplayString[31];         // offset 0x340, size 0x1F
#endif
};

extern Timer KBCreationTimer;

#endif
