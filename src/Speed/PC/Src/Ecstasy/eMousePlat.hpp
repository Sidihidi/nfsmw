#ifndef PC_ECSTASY_EMOUSEPLAT_H
#define PC_ECSTASY_EMOUSEPLAT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// El raton de PC (lo define Speed/PC/Src/Ecstasy/EcstasyE.cpp, 0x987F50): la posicion del cursor en la ventana, la
// rueda y tres botones con su flanco. Nombres de miembro nuestros (ningun binario los da). El frontend lee el cursor
// (IconPanel, cFEngGameInterface::GetMouseInfo) y FEPackageManager::Tick pregunta si cambio.

// total size: 0x2C
class eMouse {
  public:
    void UpdateCursorPos(); // (0x6C9B60)
    int Update();           // (0x6C9C80)
    bool HasChanged();      // (0x6C9D00; nombre nuestro)

    int Unknown0;        // offset 0x0, size 0x4
    int X;               // offset 0x4, size 0x4
    int Y;               // offset 0x8, size 0x4
    int PrevX;           // offset 0xC, size 0x4
    int PrevY;           // offset 0x10, size 0x4
    int DeltaX;          // offset 0x14, size 0x4 (X - PrevX, lo deja UpdateCursorPos)
    int DeltaY;          // offset 0x18, size 0x4 (Y - PrevY)
    int Wheel;           // offset 0x1C, size 0x4
    bool LeftDown;       // offset 0x20, size 0x1
    bool LeftPressed;    // offset 0x21, size 0x1
    bool LeftReleased;   // offset 0x22, size 0x1
    bool RightDown;      // offset 0x23, size 0x1
    bool RightPressed;   // offset 0x24, size 0x1
    bool RightReleased;  // offset 0x25, size 0x1
    bool MiddleDown;     // offset 0x26, size 0x1
    bool MiddlePressed;  // offset 0x27, size 0x1
    bool MiddleReleased; // offset 0x28, size 0x1
};
extern eMouse TheMouse; // (0x987F50)

#endif
