#ifndef PC_ECSTASY_EVIEW_PLAT_H
#define PC_ECSTASY_EVIEW_PLAT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Ecstasy/EcstasyData.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

// La informacion de plataforma de cada vista en PC. El tamano y los desplazamientos salen de speed.exe:
// GimmeMyViewPlatInfo multiplica por 0x1A0, GetScreenPosition transforma con la matriz de 0x80, la funcion de 0x6BEA40
// guarda en 0x80 el producto de las de 0x0 y 0x40, y GetVisibleStateSB recorre seis planos desde 0x140. Los nombres de
// las matrices son los de GameCube (eViewPlat.hpp), donde estan en el mismo orden.
// total size: 0x1A0
class eViewPlatInfo {
  public:
    bMatrix4 WorldViewMatrix;   // offset 0x0, size 0x40
    bMatrix4 ViewScreenMatrix;  // offset 0x40, size 0x40
    bMatrix4 WorldClipMatrix;   // offset 0x80, size 0x40 (WorldViewMatrix * ViewScreenMatrix)
    bMatrix4 WorldScreenMatrix; // offset 0xC0, size 0x40
    bMatrix4 Unknown100;        // offset 0x100, size 0x40
    bVector4 ClippingPlanes[6]; // offset 0x140, size 0x60

    // Las matrices de la vista (0x6CF400; thiscall, cinco argumentos: ret 0x14). Como en GameCube, mas el ultimo (los
    // planos de recorte; force_screen_far_z no se usa en PC).
    void CalculateViewMatricies(class eView *view, float force_near_z, float force_far_z, float force_screen_far_z,
                                bool clipping_planes);
};

struct IDirect3DSurface9;

// El destino de render de PC: una superficie de color y otra de profundidad de D3D9 y el tamano del viewport. El
// tamano sale de eGetRenderTarget (0x6BD3A0: imul 0x1C); la superficie y la de profundidad, de las llamadas a
// SetRenderTarget y SetDepthStencilSurface del dispositivo; Active, FrameWidth y FrameHeight, de MaybeChangeViewMode
// (los nombres de esos tres son los de GameCube).
// total size: 0x1C
struct eRenderTarget {
    TARGET_ID ID;                       // offset 0x0, size 0x4
    int ViewID;                         // offset 0x4, size 0x4 (la vista que se dibuja en el: 0x6C6F30)
    IDirect3DSurface9 *D3DSurface;      // offset 0x8, size 0x4
    IDirect3DSurface9 *D3DDepthSurface; // offset 0xC, size 0x4
    int Active;                         // offset 0x10, size 0x4
    int FrameWidth;                     // offset 0x14, size 0x4
    int FrameHeight;                    // offset 0x18, size 0x4

    void SetActive(int active) {
        Active = active;
    }

    int IsActive() {
        return Active;
    }
};

// La pantalla de PC es la ventana (eViewPlat.cpp: __window_width/__window_height).
int eGetScreenWidth();
int eGetScreenHeight();

#endif
