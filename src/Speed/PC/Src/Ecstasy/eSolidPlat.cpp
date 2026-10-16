#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyData.hpp"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/lightmaterials.h"
#include "Speed/Indep/Src/Misc/SpeedChunks.hpp"
#include "Speed/Indep/bWare/Inc/bChunk.hpp"
#include "Speed/PC/Src/Ecstasy/D3D9Min.hpp"
#include "Speed/PC/Src/Ecstasy/eEffect.hpp"
#include "Speed/PC/Src/Ecstasy/eSolidPlat.hpp"

// Los nombres de las variables de este fichero son nuestros: el exe de PC no los trae.
extern IDirect3DDevice9 *g_D3DDevice;
extern bool eDeferVertexBuffers;

// PC42: un std::map con el asignador de la STL de EA (xmemory de stlpc: gFastMem con "STL"), no un UTL::Std::map
// (ese pasa NULL): los cuatro _Buynode/_Erase/erase del exe empujan la cadena "STL" (0x890F10).
typedef std::map<unsigned int, Attrib::Gen::lightmaterials> eLightMaterialMap;

bTList<eSolidPlatInfo> eSolidPlatInfoList;
bPList<IDirect3DVertexBuffer9> eVertexBufferReleaseList;
eLightMaterialMap eLightMaterials;

int eSolidPlatInfo::CreateVertexBuffers() {
    IDirect3DVertexBuffer9 *vertex_buffer = nullptr;
    unsigned int num_verts = 0;
    int last_effect = -1;
    int num_created = 0;

    for (int i = 0; i < this->NumMeshes; i++) {
        eMeshEntry *mesh = &this->MeshEntryTable[i];

        if (last_effect != mesh->EffectIndex) {
            void *data;

            g_D3DDevice->CreateVertexBuffer(mesh->VertexDataSize, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &mesh->VertexBuffer, nullptr);
            mesh->NumVertexBufferVerts = mesh->VertexDataSize / mesh->pEffect->VertexSize;
            num_verts = mesh->NumVertexBufferVerts;
            mesh->VertexBuffer->Lock(0, 0, &data, 0);
            bMemCpy(data, mesh->VertexData, mesh->VertexDataSize);
            mesh->VertexBuffer->Unlock();
            vertex_buffer = mesh->VertexBuffer;
            num_created++;
        } else {
            mesh->VertexBuffer = vertex_buffer;
            mesh->NumVertexBufferVerts = num_verts;
        }
        last_effect = mesh->EffectIndex;
    }
    this->VertexBufferState = 2;
    return num_created;
}

int eSolidPlatInterface::LoaderPlatChunks(bChunk *chunk) {
    bChunk *current_chunk = chunk->GetFirstChunk();
    bChunk *last_chunk = chunk->GetLastChunk();
    eSolidPlatInfo *plat_info = nullptr;
    int vertex_mesh = 0;
    int material_mesh = 0;

    while (current_chunk < last_chunk) {
        unsigned int chunk_id = current_chunk->GetID();

        if (chunk_id == BCHUNK_SPEED_ESOLID_PC_PLATINFO) {
            plat_info = reinterpret_cast<eSolidPlatInfo *>(current_chunk->GetAlignedData(16));
            plat_info->VertexBufferState = 0;
            SetPlatInfo(plat_info);
            eSolidPlatInfoList.AddTail(plat_info);
        } else if (chunk_id == BCHUNK_SPEED_ESOLID_XBOX_MESH_ENTRY_TABLE) {
            plat_info = GetPlatInfo();
            plat_info->MeshEntryTable = reinterpret_cast<eMeshEntry *>(current_chunk->GetAlignedData(16));
            for (int i = 0; i < plat_info->NumMeshes; i++) {
                eMeshEntry *mesh = &plat_info->MeshEntryTable[i];

                mesh->pEffect = eEffects[mesh->EffectIndex];
            }
        } else if (chunk_id == BCHUNK_SPEED_ESOLID_XBOX_MESH_ENTRY_DATA) {
            plat_info = GetPlatInfo();
            unsigned short *index_data = reinterpret_cast<unsigned short *>(current_chunk->GetAlignedData(16));

            plat_info->IndexData = index_data;
            if (plat_info->NumIndices > 0) {
                void *data;

                g_D3DDevice->CreateIndexBuffer(plat_info->NumIndices * sizeof(unsigned short), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED,
                                               &plat_info->IndexBuffer, nullptr);
                plat_info->IndexBuffer->Lock(0, 0, &data, 0);
                bMemCpy(data, index_data, plat_info->NumIndices * sizeof(unsigned short));
                plat_info->IndexBuffer->Unlock();
            } else {
                plat_info->IndexBuffer = nullptr;
                plat_info->IndexData = nullptr;
            }
        } else if (chunk_id == BCHUNK_SPEED_ESOLID_XBOX_VERTEX_DATA) {
            plat_info = GetPlatInfo();
            unsigned int size = current_chunk->GetAlignedSize(128);

            if (size > 0) {
                eMeshEntry *mesh = &plat_info->MeshEntryTable[vertex_mesh];

                mesh->VertexData = reinterpret_cast<unsigned char *>(current_chunk->GetAlignedData(128));
                mesh->VertexDataSize = size;
                if (eDeferVertexBuffers && !(plat_info->Flags & 0x80)) {
                    if (plat_info->VertexBufferState == 0) {
                        plat_info->VertexBufferState = 1;
                    }
                    for (vertex_mesh++; vertex_mesh < plat_info->NumMeshes; vertex_mesh++) {
                        eMeshEntry *next_mesh = &plat_info->MeshEntryTable[vertex_mesh];

                        if (next_mesh->EffectIndex != plat_info->MeshEntryTable[vertex_mesh - 1].EffectIndex) {
                            break;
                        }
                        next_mesh->VertexData = nullptr;
                        next_mesh->VertexDataSize = 0;
                        next_mesh->VertexBuffer = nullptr;
                        next_mesh->NumVertexBufferVerts = mesh->NumVertexBufferVerts;
                    }
                } else {
                    void *data;

                    g_D3DDevice->CreateVertexBuffer(size, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &mesh->VertexBuffer, nullptr);
                    mesh->NumVertexBufferVerts = mesh->VertexDataSize / mesh->pEffect->VertexSize;
                    mesh->VertexBuffer->Lock(0, 0, &data, 0);
                    bMemCpy(data, mesh->VertexData, mesh->VertexDataSize);
                    mesh->VertexBuffer->Unlock();
                    for (vertex_mesh++; vertex_mesh < plat_info->NumMeshes; vertex_mesh++) {
                        eMeshEntry *next_mesh = &plat_info->MeshEntryTable[vertex_mesh];

                        if (next_mesh->EffectIndex != plat_info->MeshEntryTable[vertex_mesh - 1].EffectIndex) {
                            break;
                        }
                        next_mesh->VertexData = nullptr;
                        next_mesh->VertexDataSize = 0;
                        next_mesh->VertexBuffer = mesh->VertexBuffer;
                        next_mesh->NumVertexBufferVerts = mesh->NumVertexBufferVerts;
                    }
                }
            }
        } else if (chunk_id == BCHUNK_SPEED_ESOLID_MATERIAL_NAME) {
            eMeshEntry *mesh = &plat_info->MeshEntryTable[material_mesh++];
            const char *name = current_chunk->GetData();

            mesh->MaterialName = name;
            int key = Attrib::StringToKey(name);
            eLightMaterialMap::iterator it = eLightMaterials.find(key);
            if (it == eLightMaterials.end()) {
                it = eLightMaterials.insert(eLightMaterialMap::value_type(key, Attrib::Gen::lightmaterials(key, 0, nullptr))).first;
            }
            Attrib::Gen::lightmaterials *light_material = &it->second;

            mesh->LightMaterial = light_material;
            if (light_material->IsValid()) {
                eFindEffectIndex(&light_material->shader());
            }
        }
        current_chunk = current_chunk->GetNext();
    }
    return 1;
}

int eSolidPlatInterface::UnloaderPlatChunks(bChunk *chunk) {
    eSolidPlatInfo *plat_info = GetPlatInfo();

    if (plat_info->IndexBuffer) {
        plat_info->IndexBuffer->Release();
        plat_info->IndexBuffer = nullptr;
    }
    for (int i = 0; i < plat_info->NumMeshes; i++) {
        if (plat_info->MeshEntryTable[i].VertexBuffer) {
            if (plat_info->MeshEntryTable[i].VertexDataSize > 0) {
                eVertexBufferReleaseList.AddTail(plat_info->MeshEntryTable[i].VertexBuffer);
            }
            plat_info->MeshEntryTable[i].VertexBuffer = nullptr;
        }
    }
    plat_info->Remove();
    SetPlatInfo(nullptr);
    return 1;
}

int eSolidPlatInterface::FixPlatInfo() {
    return 1;
}

void eSolidPlatInterface::SetSmoothVertex(uint32 vertex_offset, float nx, float ny, float nz) {}

int eLoadSolidListPlatChunks(bChunk *chunk) {
    for (bChunk *current_chunk = chunk->GetFirstChunk(); current_chunk < chunk->GetLastChunk(); current_chunk = current_chunk->GetNext()) {
    }
    return 1;
}

int eUnloadSolidListPlatChunks(bChunk *chunk) {
    for (bChunk *current_chunk = chunk->GetFirstChunk(); current_chunk < chunk->GetLastChunk(); current_chunk = current_chunk->GetNext()) {
    }
    return 1;
}
