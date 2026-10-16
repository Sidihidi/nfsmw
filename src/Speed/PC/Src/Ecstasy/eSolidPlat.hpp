#ifndef PC_ECSTASY_ESOLIDPLAT_HPP
#define PC_ECSTASY_ESOLIDPLAT_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/bWare/Inc/bList.hpp"

// La parte de PC de los solidos (eSolidPlatInfo y su tabla de mallas). Las disposiciones salen del exe de PC
// (eSolidPlatInterface::LoaderPlatChunks y ::UnloaderPlatChunks, el render de mallas y la creacion de los vertex
// buffers) y se comprueban contra los datos de la instalacion de PC (scratch ecspc/py/valida_pc.py en la ronda 17).
// Los nombres de los campos que el exe no deja ver son nuestros.

struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;
class eEffect;
namespace Attrib {
namespace Gen {
struct lightmaterials;
}
} // namespace Attrib

// Una malla del solido: un registro de BCHUNK_SPEED_ESOLID_XBOX_MESH_ENTRY_TABLE (0x134B02).
// total size: 0x68
struct eMeshEntry {
    float BBoxMin[3];                     // offset 0x0, size 0xC
    float BBoxMax[3];                     // offset 0xC, size 0xC
    unsigned char TextureIndex[5];        // offset 0x18, size 0x5 (en la tabla de texturas del solido)
    unsigned char LightMaterialIndex;     // offset 0x1D, size 0x1 (0xFF: el material de luz por defecto)
    unsigned char BonePalette[16];        // offset 0x1E, size 0x10 (matrices de hueso de la malla: speed.exe 0x6C7F50)
    char Unknown2E[2];                    // offset 0x2E, size 0x2
    int EffectIndex;                      // offset 0x30, size 0x4 (indice en eEffects[]: WorldShader, CarShader...)
    eEffect *pEffect;                     // offset 0x34, size 0x4 (lo pone el cargador)
    unsigned int Flags;                   // offset 0x38, size 0x4
    unsigned int NumVerts;                // offset 0x3C, size 0x4
    unsigned int NumTris;                 // offset 0x40, size 0x4
    unsigned int IndexStart;              // offset 0x44, size 0x4
    unsigned int Unknown48;               // offset 0x48, size 0x4
    unsigned char *VertexData;            // offset 0x4C, size 0x4 (BCHUNK_SPEED_ESOLID_XBOX_VERTEX_DATA, 0x134B01)
    unsigned int VertexDataSize;          // offset 0x50, size 0x4
    IDirect3DVertexBuffer9 *VertexBuffer; // offset 0x54, size 0x4
    unsigned int NumVertexBufferVerts;    // offset 0x58, size 0x4 (VertexDataSize / tamano de vertice del efecto)
    unsigned int NumIndices;              // offset 0x5C, size 0x4
    const char *MaterialName;             // offset 0x60, size 0x4 (BCHUNK_SPEED_ESOLID_MATERIAL_NAME, 0x134C02)
    Attrib::Gen::lightmaterials *LightMaterial; // offset 0x64, size 0x4
};

// Un registro de BCHUNK_SPEED_ESOLID_PC_PLATINFO (0x134900).
// total size: 0x30
class eSolidPlatInfo : public bTNode<eSolidPlatInfo> {
  public:
    unsigned int Version;                 // offset 0x8, size 0x4 (0x12 en los datos de PC)
    unsigned int Flags;                   // offset 0xC, size 0x4 (0x80: los vertex buffers se crean al cargar)
    int NumMeshes;                        // offset 0x10, size 0x4
    eMeshEntry *MeshEntryTable;           // offset 0x14, size 0x4 (BCHUNK_SPEED_ESOLID_XBOX_MESH_ENTRY_TABLE, 0x134B02)
    int NumVertexBuffers;                 // offset 0x18, size 0x4 (cuantos 0x134B01 trae el solido)
    int Unknown1C;                        // offset 0x1C, size 0x4
    int VertexBufferState;                // offset 0x20, size 0x4 (0 nada, 1 pendientes, 2 creados)
    unsigned short *IndexData;            // offset 0x24, size 0x4 (BCHUNK_SPEED_ESOLID_XBOX_MESH_ENTRY_DATA, 0x134B03)
    IDirect3DIndexBuffer9 *IndexBuffer;   // offset 0x28, size 0x4
    unsigned int NumIndices;              // offset 0x2C, size 0x4

    int CreateVertexBuffers();
};

#endif
