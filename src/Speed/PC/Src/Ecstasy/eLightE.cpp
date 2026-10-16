// eLightE.cpp de PC: la parte de plataforma de los materiales de luz (speed.exe 0x6BE180-0x6BE31F).
#include "Speed/Indep/Src/Ecstasy/eLight.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"

SlotPool *eLightMaterialPlatInfoSlotPool = 0;

// El material de PC, sin nombre en el exe: 0x80 bytes que se crean a cero (elInitPlat pide un pool de 0x80 x 220).
// total size: 0x80
struct eLightMaterialPlatInfo {
    unsigned int Data[32]; // offset 0x0, size 0x80

    void *operator new(size_t size) {
        return bOMalloc(eLightMaterialPlatInfoSlotPool);
    }
};

void eLightMaterialPlatInterface::CreatePlatInfo() {
    PlatInfo = new eLightMaterialPlatInfo();
}

// La alfa difusa de PC por familia de material (0x8F91E4-0x8F9208): los mismos grupos de NameHash que el switch de
// GameCube, cada uno con su minimo y su maximo. Los nombres son nuestros.
float LightMaterialMinA[4] = {0.3f, 0.3f, 0.4f, 0.5f};
float LightMaterialMaxA[4] = {1.0f, 1.0f, 1.0f, 1.0f};
float LightMaterialWindowMinA = 0.4f;
float LightMaterialWindowMaxA = 0.8f;

void eLightMaterialPlatInterface::UpdatePlatInfo() {
    eLightMaterial *light_material = static_cast<eLightMaterial *>(this);
    eLightMaterialData *material_data = &light_material->feData;

    switch (light_material->NameHash) {
    case 0x33A26CB6:
    case 0xA6348EE3:
        material_data->DiffuseMinA = LightMaterialWindowMinA;
        material_data->DiffuseMaxA = LightMaterialWindowMaxA;
        break;

    case 0x12C9453C:
    case 0x3ED70C43:
    case 0x471A1DCA:
        material_data->DiffuseMinA = LightMaterialMinA[0];
        material_data->DiffuseMaxA = LightMaterialMaxA[0];
        break;

    case 0x89946400:
    case 0x89F23A14:
        material_data->DiffuseMinA = LightMaterialMinA[1];
        material_data->DiffuseMaxA = LightMaterialMaxA[1];
        break;

    case 0x8050CA3F:
    case 0x8A616C4C:
    case 0xD68EA0C1:
    case 0xD6EC76D5:
        material_data->DiffuseMinA = LightMaterialMinA[2];
        material_data->DiffuseMaxA = LightMaterialMaxA[2];
        break;

    case 0x2388DD82:
    case 0x23E6A3F4:
    case 0x23E6B396:
    case 0x6CF41096:
    case 0x8062E2C0:
    case 0x8CB61DCD:
    case 0x8CB694ED:
    case 0x8CB95E6B:
    case 0x8CBC02EC:
    case 0x8CBE3497:
    case 0xB3645F81:
    case 0xC9D2B801:
        material_data->DiffuseMinA = LightMaterialMinA[3];
        material_data->DiffuseMaxA = LightMaterialMaxA[3];
        break;
    }
}

void elInitPlat() {
    eLightMaterialPlatInfoSlotPool = bNewSlotPool(sizeof(eLightMaterialPlatInfo), 220, "eLightMaterialPlatInfoSlotPool", 0);
}
