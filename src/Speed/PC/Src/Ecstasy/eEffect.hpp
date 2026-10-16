#ifndef PC_ECSTASY_EEFFECT_HPP
#define PC_ECSTASY_EEFFECT_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Misc/attribuserinclude.h"
#include "Speed/PC/Src/Ecstasy/D3D9Min.hpp"

// Los efectos (ID3DXEffect) de la capa grafica de PC. El exe no trae nombres de estas clases ni de estas tablas:
// los nombres son nuestros; las disposiciones, del exe (la tabla de 31 descripciones de 0x90 B con "WorldShader",
// "fx\\world.efx", "IDI_WORLD_FX"... y los 31 objetos de 0x50 B que se crean al arrancar).

extern IDirect3DDevice9 *g_D3DDevice;

#define NUM_EFFECTS 31

// Descripcion fija de un efecto.
// total size: 0x90
struct eEffectInfo {
    const char *Name;         // offset 0x0, size 0x4 ("WorldShader", "CarShader"...)
    int VertexElements[32];   // offset 0x4, size 0x80 (los elementos de la declaracion de vertices)
    const char *FileName;     // offset 0x84, size 0x4 ("fx\\world.efx")
    const char *ResourceName; // offset 0x88, size 0x4 ("IDI_WORLD_FX")
    int Unknown8C;            // offset 0x8C, size 0x4
};

// Un parametro de un efecto: el hash de su semantica y su manejador en el efecto de D3DX. Cada efecto apunta a una tabla
// de 102 (eEffect::GetParameters las rellena por el hash de la semantica).
// total size: 0x28
struct eEffectParameter {
    char Unknown0[0x20];   // offset 0x0, size 0x20
    unsigned int NameHash; // offset 0x20, size 0x4
    D3DXHANDLE Handle;     // offset 0x24, size 0x4
};

#define NUM_EFFECT_PARAMETERS 102

// Un efecto cargado. Solo se declara lo que leen las fuentes de PC escritas.
// total size: 0x50
class eEffect {
  public:
    virtual ~eEffect();
    // Las dos entradas siguientes de la vtable (+4 y +8): las llama el juego justo antes de empezar y despues de
    // terminar el efecto de D3DX (0x6D1930, 0x6E6E40). Nombres nuestros.
    virtual void Start();
    virtual void End();

    // Empezar y terminar el efecto alrededor de un dibujo: las dos secuencias salen expandidas, iguales, en cada
    // sitio que cambia de efecto (eViewPlatInterface::Render 0x6D1930, 0x6E6E40...).
    void BeginEffect() {
        UINT passes;
        Start();
        g_D3DDevice->SetVertexDeclaration(pVertexDecl);
        pD3DXEffect->Begin(&passes, 0);
    }
    void EndEffect() {
        End();
        pD3DXEffect->End();
    }

    void GetParameters(); // (0x6C5500) los manejadores de los parametros del efecto (nombre nuestro)
    void Load();          // (0x6C6080) crea el efecto de D3DX desde su recurso (nombre nuestro)
    void SetDefaults();   // (0x6C5E60) (nombre nuestro)

    int Index;                                // offset 0x4, size 0x4 (en eEffectInfoTable)
    unsigned int VertexSize;                  // offset 0x8, size 0x4 (bytes por vertice: 36 o 60 en los datos de PC)
    int TechniqueIndex;                       // offset 0xC, size 0x4 (la tecnica puesta, por numero)
    D3DXHANDLE Technique;                     // offset 0x10, size 0x4 (cero: el efecto no se puede usar; Render(eModel))
    int NumPasses;                            // offset 0x14, size 0x4 (de la tecnica de los efectos con LOD)
    int LodTechniques[4];                     // offset 0x18, size 0x10 (la tecnica de cada LOD; -1 si no la hay)
    int LodFlags[4];                          // offset 0x28, size 0x10 (> 0: el LOD lleva coordenadas de 4 componentes)
    int NumLods;                              // offset 0x38, size 0x4 (el LOD mas alto de las mallas del efecto)
    int CurrentLod;                           // offset 0x3C, size 0x4 (el de la tecnica puesta: eLoadEffects)
    bool Unknown40;                           // offset 0x40, size 0x1 (coordenadas de textura de 4 componentes)
    eEffectParameter *Parameters;             // offset 0x44, size 0x4
    ID3DXEffect *pD3DXEffect;                 // offset 0x48, size 0x4
    IDirect3DVertexDeclaration9 *pVertexDecl; // offset 0x4C, size 0x4
};

extern eEffectInfo eEffectInfoTable[NUM_EFFECTS];
extern eEffect *eEffects[NUM_EFFECTS];

int eFindEffectIndex(Attrib::StringKeyPtr name);

#endif
