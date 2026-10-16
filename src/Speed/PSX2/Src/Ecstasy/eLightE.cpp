#include "Speed/Indep/Src/Ecstasy/eLight.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/PSX2/Inc/DmaE.hpp"
#include "Speed/PSX2/Src/Ecstasy/eLightPlat.hpp"

SlotPool *eLightMaterialPlatInfoSlotPool;

void eLightMaterialPlatInterface::CreatePlatInfo() {
    PlatInfo = bOMalloc(eLightMaterialPlatInfoSlotPool);
}

// Los datos de DMA del material para el microcodigo: el difuso (minimo y rango), el especular (minimo, rango, potencia
// y su "numero magico" para la exponenciacion de la VU1) y lo mismo del mapa de entorno.
void eLightMaterialPlatInterface::UpdatePlatInfo() {
    eLightMaterialPlatInfo *plat_info = static_cast<eLightMaterialPlatInfo *>(PlatInfo);
    {
        eLightMaterialData *material_data = &static_cast<eLightMaterial *>(this)->feData;

        float diffuse_min_scale = material_data->DiffuseMinScale;
        float diffuse_min_r = diffuse_min_scale * material_data->DiffuseMinR;
        float diffuse_min_g = diffuse_min_scale * material_data->DiffuseMinG;
        float diffuse_min_b = diffuse_min_scale * material_data->DiffuseMinB;
        float diffuse_min_a = material_data->DiffuseMinA;
        float diffuse_max_scale = material_data->DiffuseMaxScale;
        float diffuse_max_r = diffuse_max_scale * material_data->DiffuseMaxR;
        float diffuse_max_g = diffuse_max_scale * material_data->DiffuseMaxG;
        float diffuse_max_b = diffuse_max_scale * material_data->DiffuseMaxB;
        float diffuse_max_a = material_data->DiffuseMaxA;

        float specular_power = material_data->SpecularPower;
        float one = FloatAsInteger(1.0f), specular_k = specular_power * 1065353216.0f;
        float specular_min_scale = material_data->SpecularMinScale * 2.0f;
        float specular_min_r = specular_min_scale * material_data->SpecularMinR;
        float specular_min_g = specular_min_scale * material_data->SpecularMinG;
        float specular_min_b = specular_min_scale * material_data->SpecularMinB;
        float specular_max_scale = material_data->SpecularMaxScale * 2.0f;
        float specular_max_r = specular_max_scale * material_data->SpecularMaxR;
        float specular_max_g = specular_max_scale * material_data->SpecularMaxG;
        float specular_max_b = specular_max_scale * material_data->SpecularMaxB;

        float envmap_power = material_data->EnvmapPower;
        float envmap_k = envmap_power * 1065353216.0f;
        float envmap_min_scale = material_data->EnvmapMinScale * 0.5f;
        float envmap_min_r = envmap_min_scale * material_data->EnvmapMinR;
        float envmap_min_g = envmap_min_scale * material_data->EnvmapMinG;
        float envmap_min_b = envmap_min_scale * material_data->EnvmapMinB;
        float envmap_max_scale = material_data->EnvmapMaxScale * 0.5f;
        float envmap_max_r = envmap_max_scale * material_data->EnvmapMaxR;
        float envmap_max_g = envmap_max_scale * material_data->EnvmapMaxG;
        float envmap_max_b = envmap_max_scale * material_data->EnvmapMaxB;

        plat_info->feDmaData[0] = FloatAsInteger(diffuse_min_r);
        plat_info->feDmaData[1] = FloatAsInteger(diffuse_min_g);
        plat_info->feDmaData[2] = FloatAsInteger(diffuse_min_b);
        plat_info->feDmaData[3] = FloatAsInteger(diffuse_min_a);

        plat_info->feDmaData[4] = FloatAsInteger(diffuse_max_r - diffuse_min_r);
        plat_info->feDmaData[5] = FloatAsInteger(diffuse_max_g - diffuse_min_g);
        plat_info->feDmaData[6] = FloatAsInteger(diffuse_max_b - diffuse_min_b);
        plat_info->feDmaData[7] = FloatAsInteger(diffuse_max_a - diffuse_min_a);

        plat_info->feDmaData[8] = FloatAsInteger(specular_min_r);
        plat_info->feDmaData[9] = FloatAsInteger(specular_min_g);
        plat_info->feDmaData[10] = FloatAsInteger(specular_min_b);
        plat_info->feDmaData[11] = FloatAsInteger(specular_power);

        plat_info->feDmaData[12] = FloatAsInteger(specular_max_r - specular_min_r);
        plat_info->feDmaData[13] = FloatAsInteger(specular_max_g - specular_min_g);
        plat_info->feDmaData[14] = FloatAsInteger(specular_max_b - specular_min_b);
        plat_info->feDmaData[15] = FloatAsInteger(one - specular_k);

        plat_info->feDmaData[16] = FloatAsInteger(envmap_min_r);
        plat_info->feDmaData[17] = FloatAsInteger(envmap_min_g);
        plat_info->feDmaData[18] = FloatAsInteger(envmap_min_b);
        plat_info->feDmaData[19] = FloatAsInteger(envmap_power);

        plat_info->feDmaData[20] = FloatAsInteger(envmap_max_r - envmap_min_r);
        plat_info->feDmaData[21] = FloatAsInteger(envmap_max_g - envmap_min_g);
        plat_info->feDmaData[22] = FloatAsInteger(envmap_max_b - envmap_min_b);
        plat_info->feDmaData[23] = FloatAsInteger(one - envmap_k);
    }
}

void elInitPlat() {
    eLightMaterialPlatInfoSlotPool = bNewSlotPool(96, 220, "eLightMaterialPlatInfoSlotPool", 0);
}
