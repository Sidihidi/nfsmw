#include "Speed/PC/Src/Ecstasy/eEffect.hpp"
#include "Speed/Indep/Src/Ecstasy/eLight.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/World/TimeOfDay.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

#include <string.h>

extern eEffect *eCurrentEffect;     // (0x982C80)
extern int EnableEnvMap;            // (0x9017B8) 0 sin mapa de entorno; 1, 2 y 3, la calidad de las opciones

// El indice en eEffectInfoTable del efecto que se llama `name` (el `shader` de un lightmaterials), o -1.
int eFindEffectIndex(Attrib::StringKeyPtr name) {
    for (int i = 0; i < NUM_EFFECTS; i++) {
        if (Attrib::StringKey(eEffectInfoTable[i].Name) == *name) {
            return i;
        }
    }
    return -1;
}

// La luz de los coches de PC. Los nombres de las dos funciones y de sus globales son nuestros (el exe no los trae); las
// llama el dibujo de cada malla (0x6D7660 y 0x6E0200) con el contexto de luz y el material de la malla.
extern float eFixedFunctionLightDiffuseScale;  // (0x8FAE78) 0.5
extern float eFixedFunctionLightSpecularScale; // (0x8FAE7C) 0.4

// El contexto de luz dinamica (las tres luces que calcula elSetupLights) al efecto actual (0x6C7420). Con el CarShader
// en una tecnica sin shaders (Unknown40 a 0), dos luces direccionales del pipeline fijo; si no, las dos matrices tal
// cual a LOCALCOLOURMATRIX (fila i: el color de la luz i) y LOCALDIRECTIONMATRIX (columna i: su direccion local).
void eSetLightContext(eDynamicLightContext *light_context, bMatrix4 *local_to_world) {
    if (!light_context || light_context->Type != 0) {
        return;
    }
    if (eCurrentEffect->Index == 4 && !eCurrentEffect->Unknown40) {
        D3DXVECTOR4 light_directions[3];
        bMatrix4 light_colours;
        bMatrix4 *direction_matrix = &light_context->LocalDirectionMatrix;
        bMatrix4 *colour_matrix = &light_context->LocalColourMatrix;

        light_colours.v0.z = 0.0f;
        light_colours.v0.y = 0.0f;
        light_colours.v0.x = 0.0f;
        light_directions[0].x = direction_matrix->v0.x;
        light_directions[0].y = direction_matrix->v1.x;
        light_directions[0].z = direction_matrix->v2.x;
        light_directions[0].w = 0.0f;
        light_directions[1].x = direction_matrix->v0.y;
        light_directions[1].y = direction_matrix->v1.y;
        light_directions[1].z = direction_matrix->v2.y;
        light_directions[1].w = 0.0f;
        light_directions[2].x = direction_matrix->v0.z;
        light_directions[2].y = direction_matrix->v1.z;
        light_directions[2].z = direction_matrix->v2.z;
        light_directions[2].w = 0.0f;
        light_colours.v1.x = colour_matrix->v0.x;
        light_colours.v1.y = colour_matrix->v0.y;
        light_colours.v1.z = colour_matrix->v0.z;
        light_colours.v2.x = colour_matrix->v1.x;
        light_colours.v2.y = colour_matrix->v1.y;
        light_colours.v2.z = colour_matrix->v1.z;
        light_colours.v3.x = colour_matrix->v2.x;
        light_colours.v3.y = colour_matrix->v2.y;
        light_colours.v3.z = colour_matrix->v2.z;

        for (int i = 0; i < 2; i++) {
            D3DLIGHT9 light;
            bVector4 direction;

            memset(&light, 0, sizeof(light));
            light.Type = 3; // D3DLIGHT_DIRECTIONAL
            light.Diffuse.r = eFixedFunctionLightDiffuseScale * light_colours[i + 1].x;
            light.Diffuse.g = eFixedFunctionLightDiffuseScale * light_colours[i + 1].y;
            light.Diffuse.b = eFixedFunctionLightDiffuseScale * light_colours[i + 1].z;
            light.Ambient = *reinterpret_cast<D3DCOLORVALUE *>(&light_colours.v0);
            light.Specular.r = eFixedFunctionLightSpecularScale * light_colours[i + 1].x;
            light.Specular.g = eFixedFunctionLightSpecularScale * light_colours[i + 1].y;
            light.Specular.b = eFixedFunctionLightSpecularScale * light_colours[i + 1].z;
            bMulMatrix(&direction, local_to_world, reinterpret_cast<bVector4 *>(&light_directions[i]));
            light.Direction.x = -direction.x;
            light.Direction.y = -direction.y;
            light.Direction.z = -direction.z;
            g_D3DDevice->SetLight(i, &light);
            if (!(bAbs(light.Direction.x) < 0.0001f && bAbs(light.Direction.y) < 0.0001f &&
                  bAbs(light.Direction.z) < 0.0001f)) {
                g_D3DDevice->LightEnable(i, TRUE);
            }
        }
        g_D3DDevice->LightEnable(2, TRUE);
    } else {
        if (eCurrentEffect->Parameters[58].Handle) { // LOCALCOLOURMATRIX
            eCurrentEffect->pD3DXEffect->SetMatrix(eCurrentEffect->Parameters[58].Handle,
                                                   reinterpret_cast<D3DXMATRIX *>(&light_context->LocalColourMatrix));
        }
        if (eCurrentEffect->Parameters[59].Handle) { // LOCALDIRECTIONMATRIX
            eCurrentEffect->pD3DXEffect->SetMatrix(eCurrentEffect->Parameters[59].Handle,
                                                   reinterpret_cast<D3DXMATRIX *>(&light_context->LocalDirectionMatrix));
        }
    }
}

extern float eFixedFunctionSpecularPowerScale; // (0x8FAE84) 4
extern float eCarSpecularPowerScale;           // (0x9018E0) 1
extern float eCarEnvmapPowerScale;             // (0x9018DC) 1

// El material de luz de la malla al efecto actual (0x6C76E0): minimo y rango (maximo - minimo) del difuso, el especular
// (por el CarSpecScale de la hora del dia) y el entorno (a la mitad si la tecnica no lleva shaders). Con el bit 0x100 de
// la malla, el mapa de entorno puesto y sin el bit 0x80, solo queda el entorno. El WorldBoneShader (2) solo recibe el
// difuso; el CarShader (4), todo, o en el pipeline fijo un D3DMATERIAL9 y el TEXTUREFACTOR.
void eSetLightMaterial(eLightMaterial *light_material, unsigned int flags) {
    eLightMaterialData *data = &light_material->feData;
    bVector4 diffuse_min;
    bVector4 specular_min;
    bVector4 diffuse_range;
    bVector4 specular_range;
    bVector4 envmap_min;
    bVector4 envmap_range;

    diffuse_min.x = data->DiffuseMinR * data->DiffuseMinScale;
    diffuse_min.y = data->DiffuseMinG * data->DiffuseMinScale;
    diffuse_min.z = data->DiffuseMinB * data->DiffuseMinScale;
    diffuse_min.w = data->DiffuseMinA;
    diffuse_range.x = data->DiffuseMaxR * data->DiffuseMaxScale - diffuse_min.x;
    diffuse_range.y = data->DiffuseMaxG * data->DiffuseMaxScale - diffuse_min.y;
    diffuse_range.z = data->DiffuseMaxB * data->DiffuseMaxScale - diffuse_min.z;
    diffuse_range.w = (data->DiffuseMaxA - diffuse_min.w) * 0.25f;

    float specular_scale = TimeOfDayPointer->CurrentLighting.CarSpecScale;
    float specular_min_scale = specular_scale * data->SpecularMinScale;
    float specular_max_scale = specular_scale * data->SpecularMaxScale;

    specular_min.x = specular_min_scale * data->SpecularMinR;
    specular_min.y = specular_min_scale * data->SpecularMinG;
    specular_min.z = specular_min_scale * data->SpecularMinB;
    specular_min.w = 0.0f;
    specular_range.x = specular_max_scale * data->SpecularMaxR - specular_min.x;
    specular_range.y = specular_max_scale * data->SpecularMaxG - specular_min.y;
    specular_range.z = specular_max_scale * data->SpecularMaxB - specular_min.z;
    specular_range.w = 0.0f;

    float envmap_scale = !eCurrentEffect->Unknown40 ? 0.5f : 1.0f;

    envmap_min.x = data->EnvmapMinR * data->EnvmapMinScale * envmap_scale;
    envmap_min.y = data->EnvmapMinG * data->EnvmapMinScale * envmap_scale;
    envmap_min.z = data->EnvmapMinB * data->EnvmapMinScale * envmap_scale;
    envmap_min.w = 0.0f;
    envmap_range.x = data->EnvmapMaxR * data->EnvmapMaxScale * envmap_scale - envmap_min.x;
    envmap_range.y = data->EnvmapMaxG * data->EnvmapMaxScale * envmap_scale - envmap_min.y;
    envmap_range.z = data->EnvmapMaxB * data->EnvmapMaxScale * envmap_scale - envmap_min.z;
    envmap_range.w = 0.0f;

    if ((flags & 0x100) && EnableEnvMap && !(flags & 0x80)) {
        diffuse_min.x = 0.0f;
        diffuse_min.y = 0.0f;
        diffuse_min.z = 0.0f;
        diffuse_min.w = 0.0f;
        diffuse_range.x = 0.0f;
        diffuse_range.y = 0.0f;
        diffuse_range.z = 0.0f;
        diffuse_range.w = 0.0f;
        specular_min.x = 0.0f;
        specular_min.y = 0.0f;
        specular_min.z = 0.0f;
        specular_min.w = 0.0f;
        specular_range.x = 0.0f;
        specular_range.y = 0.0f;
        specular_range.z = 0.0f;
        specular_range.w = 0.0f;
    }

    if (eCurrentEffect->Index == 4 && !eCurrentEffect->Unknown40) {
        D3DCOLORVALUE black = {0.0f, 0.0f, 0.0f, 1.0f};
        D3DCOLORVALUE white = {1.0f, 1.0f, 1.0f, 1.0f};
        D3DMATERIAL9 material;
        bVector3 diffuse;
        bVector3 specular;
        bVector3 texture_factor;

        diffuse.x = diffuse_min.x + diffuse_range.x * 0.5f;
        diffuse.y = diffuse_min.y + diffuse_range.y * 0.5f;
        diffuse.z = diffuse_min.z + diffuse_range.z * 0.5f;
        specular.x = specular_min.x + specular_range.x * 0.5f;
        specular.y = specular_min.y + specular_range.y * 0.5f;
        specular.z = specular_min.z + specular_range.z * 0.5f;
        texture_factor.x = envmap_min.x + envmap_range.x * 0.25f;
        texture_factor.y = envmap_min.y + envmap_range.y * 0.25f;
        texture_factor.z = envmap_min.z + envmap_range.z * 0.25f;
        material.Diffuse.r = diffuse.x;
        material.Diffuse.g = diffuse.y;
        material.Diffuse.b = diffuse.z;
        material.Diffuse.a = 1.0f;
        material.Specular.r = specular.x;
        material.Specular.g = specular.y;
        material.Specular.b = specular.z;
        material.Specular.a = 1.0f;
        material.Ambient = white;
        material.Emissive = black;
        material.Power = eFixedFunctionSpecularPowerScale * data->SpecularPower;
        if (material.Power < 0.1f) {
            material.Power = 0.1f;
        } else if (material.Power > 20.0f) {
            material.Power = 20.0f;
        }
        g_D3DDevice->SetMaterial(&material);
        g_D3DDevice->SetRenderState(static_cast<D3DRENDERSTATETYPE>(60), // D3DRS_TEXTUREFACTOR
                                    (static_cast<int>((diffuse_min.w + diffuse_range.w) * 128.0f) << 24) |
                                        (static_cast<int>(texture_factor.z * 255.0f) << 16) |
                                        (static_cast<int>(texture_factor.y * 255.0f) << 8) |
                                        static_cast<int>(texture_factor.x * 255.0f));
        return;
    }
    if (eCurrentEffect->Index == 4 || eCurrentEffect->Index == 2) {
        eCurrentEffect->pD3DXEffect->SetVector(eCurrentEffect->Parameters[14].Handle, // DIFFUSEMIN
                                               reinterpret_cast<D3DXVECTOR4 *>(&diffuse_min));
        eCurrentEffect->pD3DXEffect->SetVector(eCurrentEffect->Parameters[15].Handle, // DIFFUSERANGE
                                               reinterpret_cast<D3DXVECTOR4 *>(&diffuse_range));
        if (eCurrentEffect->Index == 4) {
            eCurrentEffect->pD3DXEffect->SetVector(eCurrentEffect->Parameters[81].Handle, // SPECULARMIN
                                                   reinterpret_cast<D3DXVECTOR4 *>(&specular_min));
            eCurrentEffect->pD3DXEffect->SetVector(eCurrentEffect->Parameters[83].Handle, // SPECULARRANGE
                                                   reinterpret_cast<D3DXVECTOR4 *>(&specular_range));
            eCurrentEffect->pD3DXEffect->SetVector(eCurrentEffect->Parameters[22].Handle, // ENVMAPMIN
                                                   reinterpret_cast<D3DXVECTOR4 *>(&envmap_min));
            eCurrentEffect->pD3DXEffect->SetVector(eCurrentEffect->Parameters[21].Handle, // ENVMAPANGE (asi en el exe)
                                                   reinterpret_cast<D3DXVECTOR4 *>(&envmap_range));
            eCurrentEffect->pD3DXEffect->SetFloat(eCurrentEffect->Parameters[82].Handle, // SPECULARPOWER
                                                  eCarSpecularPowerScale * data->SpecularPower);
            eCurrentEffect->pD3DXEffect->SetFloat(eCurrentEffect->Parameters[23].Handle, // ENVMAPPOWER
                                                  eCarEnvmapPowerScale * data->EnvmapPower);
        }
    }
}
