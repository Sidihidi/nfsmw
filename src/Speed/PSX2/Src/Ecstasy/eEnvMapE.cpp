#include "Speed/Indep/Src/Ecstasy/eEnvMap.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Interfaces/SimActivities/INIS.h"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/PSX2/Src/Ecstasy/EcstasyE.hpp"

// psx2/src/ecstasy/eEnvMapE.cpp de la A124: el mapa de entorno de PS2 son seis vistas con sus render targets (las caras
// del cubo) que despues se proyectan en las paletas de reflejo. Los datos, en el orden y con los valores del .data
// original (los tipos, de los stabs).
eEnvMap TheOnlyEnvMap;
bVector3 envmap_fakeup(0.0f, 0.0f, 1.0f);
float EnvMapScreenZ = 64.0f;

float HackDirArray[2][6][4] = {
    {
        {-1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, -1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
    },
    {
        {-1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, -1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
    },
};

float HackDirArrayUp[2][6][4] = {
    {
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f, 0.0f},
    },
    {
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f, 0.0f},
    },
};

int EnableFace[6] = {1, 1, 0, 1, 1, 1};

int EnableEnvMapFaces[5][6] = {
    {1, 1, 0, 1, 0, 0},
    {0, 0, 0, 1, 1, 1},
    {1, 1, 0, 0, 1, 0},
    {0, 1, 0, 1, 0, 1},
    {1, 0, 0, 0, 1, 1},
};

bMatrix4 hack_man_matrix;

#ifndef EA_BUILD_A124
// La retail define ademas este objeto de 16 B con (1.0, 0.0, 1.0), justo detras de hack_man_matrix en su .data
// (0x54FFA0, 16 B) y sin nombre en el objeto troceado (el ELF de SLUS va sin .symtab): su $I$ lo inicializa
// entrelazado con envmap_fakeup y su UpdateCameras lo suma a la posicion del mapa de entorno con la camara de
// capo. El nombre es el del objeto original.
bVector3 D_0054FFA0(1.0f, 0.0f, 1.0f);
#endif

extern uint32 numCopsActiveCherry;

// El color de fondo de cada cara (la de atras, la 2, va aparte).
static const int EnvMapClearRGB[6] = {16, 16, 16, 16, 16, 20};

// Una muestra de la tabla de reflejo: la direccion (Rx, Ry, Rz), su sitio en la cara (Rs, Rt) y en la paleta (Fs, Ft).
struct RefSample {
    float Rx;
    float Ry;
    float Rz;
    float Rs;
    float Rt;
    float Fs;
    float Ft;
};

RefSample ReflectionSamples[6][7][7];
RefSample BackReflectionSamples[4][7][7];

void CreateRefSampleTable();
void CreateEnvMap(int envmap_face, eRenderTarget *source_target, eRenderTarget *dest_target);
void CreateBackEnvMap(int envmap_face, eRenderTarget *source_target, eRenderTarget *dest_target);
int32 IsFaceEnabled(int32 i);

int eInitEnvMap() {
    CreateRefSampleTable();

    eEnvMap *envmap = eGetEnvMap();

    for (int face = 0; face < 6; face++) {
        int32 view_id = EVIEW_ENVMAP0F + face;
        int32 target_id = 0;

        switch (face) {
        case 0:
            target_id = TARGET_ENVRMAP0F;
            break;
        case 1:
            target_id = TARGET_ENVRMAP0R;
            break;
        case 2:
            target_id = TARGET_ENVRMAP0B;
            break;
        case 3:
            target_id = TARGET_ENVRMAP0L;
            break;
        case 4:
            target_id = TARGET_ENVRMAP0U;
            break;
        case 5:
            target_id = TARGET_ENVRMAP0D;
            break;
        }

        eView *view = eGetView(view_id, false);
        eRenderTarget *target = eGetRenderTarget(target_id);

        view->SetCamera(&envmap->Cameras[face]);
        view->SetRenderTarget(target, 0);
        view->SetActive(1);

        target->SetActive(1);
#ifndef EA_BUILD_A124
        view->Precipitation = eGetView(EVIEW_PLAYER1, false)->Precipitation;
#endif

        envmap->Views[face] = view;
    }

    return 1;
}

// Orienta las seis camaras del mapa de entorno: la base del cubo mirando del espectador al coche, y cada cara con su
// direccion de HackDirArray (las del frontend o las de carrera). Solo se actualizan las caras que tocan este frame.
void eEnvMap::UpdateCameras(bVector3 *viewer_world_position, bVector3 *envmap_world_position) {
#ifndef EA_BUILD_A124
    // La retail abre con un bloque que la alpha no tiene (820 B frente a 740): si la camara que mueve la vista
    // del jugador 1 es de capo, la posicion del mapa de entorno se corre con D_0054FFA0. La llamada es la
    // virtual IsHoodCamera del CameraMover (vtable +0x88/+0x8c, con el ajuste de this de la entrada).
    CameraMover *mover = NULL;
    CameraMover *head = (CameraMover *)eViews[EVIEW_PLAYER1].CameraMoverList.HeadNode.Next;

    if (head != (CameraMover *)&eViews[EVIEW_PLAYER1].CameraMoverList.HeadNode) {
        mover = head;
    }

    if (mover && mover->IsHoodCamera()) {
        *envmap_world_position += D_0054FFA0;
    }
#endif
    bMatrix4 camera_basis;
    bVector3 a(0.0f, 0.0f, 0.0f);
    bVector3 b(1.0f, 0.0f, 0.0f);

    eCreateLookAtMatrix(&camera_basis, a, b, envmap_fakeup);

    camera_basis.v0.w = 0.0f;
    camera_basis.v1.w = 0.0f;
    camera_basis.v2.w = 0.0f;
    camera_basis.v3.x = 0.0f;
    camera_basis.v3.y = 0.0f;
    camera_basis.v3.z = 0.0f;
    camera_basis.v3.w = 1.0f;

    bMatrix4 camera_lookat_car;

    eCreateLookAtMatrix(&camera_lookat_car, *viewer_world_position, *envmap_world_position, envmap_fakeup);

    camera_lookat_car.v0.w = 0.0f;
    camera_lookat_car.v1.w = 0.0f;
    camera_lookat_car.v2.w = 0.0f;
    camera_lookat_car.v3.x = 0.0f;
    camera_lookat_car.v3.y = 0.0f;
    camera_lookat_car.v3.z = 0.0f;
    camera_lookat_car.v3.w = 1.0f;

    hack_man_matrix = camera_lookat_car;

    bMatrix4 orientation;

    eTransposeMatrix(&orientation, &camera_basis);

    eMulMatrix(&orientation, &camera_lookat_car, &orientation);

    orientation.v0.w = 0.0f;
    orientation.v1.w = 0.0f;
    orientation.v2.w = 0.0f;
    orientation.v3.x = 0.0f;
    orientation.v3.y = 0.0f;
    orientation.v3.z = 0.0f;
    orientation.v3.w = 1.0f;

    eTransposeMatrix(&orientation, &orientation);

    int direction_array_index;
    int direction_matrix_index;

    if (TheGameFlowManager.IsInFrontend()) {
        direction_array_index = 0;
        direction_matrix_index = 2;
    } else {
        direction_array_index = 1;
        direction_matrix_index = 2;
    }

    float near_z = 0.5f;
    float far_z = 64.0f;

    for (int face = 0; face < 6; face++) {
        eView *view = this->Views[face];

        if (view) {
            int view_active = IsFaceEnabled(face);

            view->SetActive(view_active != 0);

            Camera *camera = view->GetCamera();

            if (view_active && camera) {
                bVector3 look;
                bVector3 up;
                bMatrix4 matrix;

                eMulVector(&up, &orientation, reinterpret_cast<const bVector3 *>(HackDirArrayUp[direction_array_index][face]));
                eMulVector(&look, &orientation, reinterpret_cast<const bVector3 *>(HackDirArray[direction_array_index][face]));

                bAdd(&look, &look, envmap_world_position);

                eCreateLookAtMatrix(&matrix, *envmap_world_position, look, up);

                if (direction_matrix_index == face) {
                    hack_man_matrix = matrix;
                }

                camera->SetCameraMatrix(matrix, RealTimeElapsed);
                camera->SetRenderDash(0);
                camera->SetFocalDistance(0.0f);
                camera->SetNearZ(near_z);
                camera->SetFarZ(far_z);
                camera->SetFieldOfView(0x4000);
            }
        }
    }
}

eEnvMap *eGetEnvMap() {
    return &TheOnlyEnvMap;
}

// Las coordenadas de la paleta de reflejo de un vector (proyeccion esferica).
void ComputeReflectionST(bVector3 *reflection_vector, bVector2 *st) {
    bNormalize(reflection_vector, reflection_vector);

    float rx = reflection_vector->x;
    float ry = reflection_vector->y;
    float rz = reflection_vector->z;

    float m = bSqrt(rx * rx + ry * ry + (rz + 1.0f) * (rz + 1.0f));

    float s = rx / (m + m) + 0.5f;
    float t = ry / (m + m) + 0.5f;

    st->x = s;
    st->y = t;
}

// La tabla de reflejo: para cada cara del cubo, una rejilla de 6x6 direcciones con su sitio en la paleta (Rs, Rt) y en
// la cara (Fs, Ft); despues, las cuatro tiras de la cara de atras, que salen del borde de las caras 2, 3, 5 y 1 hacia
// fuera (eEnvMapE.cpp:536-733 en la A124).
void CreateRefSampleTable() {
    int num_loops = 6;
    float delta = 2.0f / (num_loops - 1);

    for (int face = 0; face < 6; face++) {
        for (int i = 0; i < num_loops; i++) {
            for (int j = 0; j < num_loops; j++) {
                float di = delta * i;
                float dj = delta * j;
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;

                switch (face) {
                case 0: x = -1.0f + di; y = -1.0f + dj; z = 1.0f; break;
                case 1: x = 1.0f; y = -1.0f + dj; z = 1.0f - di; break;
                case 2: x = 1.0f - di; y = -1.0f + dj; z = -1.0f; break;
                case 3: x = -1.0f; y = -1.0f + dj; z = -1.0f + di; break;
                case 4: x = -1.0f + di; y = -1.0f; z = -1.0f + dj; break;
                case 5: x = -1.0f + di; y = 1.0f; z = 1.0f - dj; break;
                }

                bVector3 ref(x, y, z);
                bVector2 st;

                ComputeReflectionST(&ref, &st);

                RefSample *sample = &ReflectionSamples[face][i][j];

                sample->Rx = x;
                sample->Ry = y;
                sample->Rz = z;
                sample->Rs = st.x;
                sample->Rt = st.y;
                sample->Fs = di * 0.5f;
                sample->Ft = dj * 0.5f;
            }
        }
    }

    float back_scale = 0.16f;

    {
        for (int i = 0; i < num_loops; i++) {
            RefSample *sample = &ReflectionSamples[2][i][0];
            float irs = sample->Rs - 0.5f;
            float irt = sample->Rt - 0.5f;
            bVector2 rdelta(irs, irt);

            bNormalize(&rdelta, &rdelta);

            irs += 0.5f;
            irt += 0.5f;
            rdelta *= 0.5f;

            float pi = i / static_cast<float>(num_loops - 1);
            float ifs = 1.0f - pi;
            float ift = 0.0f;
            float dfs = 0.5f - ifs;
            float dft = 0.5f;

            for (int j = 0; j < num_loops; j++) {
                RefSample *back_sample = &BackReflectionSamples[0][i][j];
                float pj = j / static_cast<float>(num_loops - 1);

                back_sample->Rs = irs + rdelta.x * pj * back_scale;
                back_sample->Rt = irt + rdelta.y * pj * back_scale;
                back_sample->Fs = ifs + dfs * pj;
                back_sample->Ft = ift + dft * pj;
            }
        }
    }

    {
        for (int i = 0; i < num_loops; i++) {
            RefSample *sample = &ReflectionSamples[3][num_loops - 1][i];
            float irs = sample->Rs - 0.5f;
            float irt = sample->Rt - 0.5f;
            bVector2 rdelta(irs, irt);

            bNormalize(&rdelta, &rdelta);

            irs += 0.5f;
            irt += 0.5f;
            rdelta *= 0.5f;

            float pi = i / static_cast<float>(num_loops - 1);
            float ifs = 0.0f;
            float ift = pi;
            float dfs = 0.5f;
            float dft = 0.5f - pi;

            for (int j = 0; j < num_loops; j++) {
                RefSample *back_sample = &BackReflectionSamples[1][i][j];
                float pj = j / static_cast<float>(num_loops - 1);

                back_sample->Rs = irs + rdelta.x * pj * back_scale;
                back_sample->Rt = irt + rdelta.y * pj * back_scale;
                back_sample->Fs = ifs + dfs * pj;
                back_sample->Ft = ift + dft * pj;
            }
        }
    }

    {
        for (int i = 0; i < num_loops; i++) {
            RefSample *sample = &ReflectionSamples[5][i][num_loops - 1];
            float irs = sample->Rs - 0.5f;
            float irt = sample->Rt - 0.5f;
            bVector2 rdelta(irs, irt);

            bNormalize(&rdelta, &rdelta);

            irs += 0.5f;
            irt += 0.5f;
            rdelta *= 0.5f;

            float pi = i / static_cast<float>(num_loops - 1);
            float ifs = 1.0f - pi;
            float ift = 1.0f;
            float dfs = 0.5f - ifs;
            float dft = -0.5f;

            for (int j = 0; j < num_loops; j++) {
                RefSample *back_sample = &BackReflectionSamples[2][i][j];
                float pj = j / static_cast<float>(num_loops - 1);

                back_sample->Rs = irs + rdelta.x * pj * back_scale;
                back_sample->Rt = irt + rdelta.y * pj * back_scale;
                back_sample->Fs = ifs + dfs * pj;
                back_sample->Ft = ift + dft * pj;
            }
        }
    }

    {
        for (int i = 0; i < num_loops; i++) {
            RefSample *sample = &ReflectionSamples[1][0][i];
            float irs = sample->Rs - 0.5f;
            float irt = sample->Rt - 0.5f;
            bVector2 rdelta(irs, irt);

            bNormalize(&rdelta, &rdelta);

            irs += 0.5f;
            irt += 0.5f;
            rdelta *= 0.5f;

            float pi = i / static_cast<float>(num_loops - 1);
            float ifs = 1.0f;
            float ift = pi;
            float dfs = -0.5f;
            float dft = 0.5f - pi;

            for (int j = 0; j < num_loops; j++) {
                RefSample *back_sample = &BackReflectionSamples[3][i][j];
                float pj = j / static_cast<float>(num_loops - 1);

                back_sample->Rs = irs + rdelta.x * pj * back_scale;
                back_sample->Rt = irt + rdelta.y * pj * back_scale;
                back_sample->Fs = ifs + dfs * pj;
                back_sample->Ft = ift + dft * pj;
            }
        }
    }
}

// Pinta una cara del mapa de entorno: cinco tiras de triangulos que llevan la imagen de la cara (source_target) a su
// sitio en dest_target, con las coordenadas de ReflectionSamples. El paquete va a la lista de DMA de la textura.
void CreateEnvMap(int envmap_face, eRenderTarget *source_target, eRenderTarget *dest_target) {
    TextureInfo *texture_info = source_target->GetTextureInfo(0);

    if (!texture_info) {
        return;
    }

    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(78 * sizeof(u_long128)));
    uint32 *data;

    if (!data_start) {
        return;
    }

    data_start[0] = MakeGifTagLo(1, 0);
    data_start[1] = MakeGifTagHi(1, PRIM_TRISTRIP | PRIM_TME | PRIM_ABE, GIF_PACKED, 2);
    data_start[2] = SCE_GS_ST | (SCE_GS_RGBAQ << 4);
    data_start[3] = 0;

    data_start[4] = 0;
    data_start[5] = 0;
    data_start[6] = 0x3F800000;
    data_start[7] = 0;

    data_start[8] = 128;
    data_start[9] = 128;
    data_start[10] = 128;
    data_start[11] = 96;
    data = &data_start[12];

    float envmap_width = dest_target->ScissorW;
    float envmap_height = dest_target->ScissorH;
    float half_width = envmap_width * 0.5f;
    float half_height = envmap_height * 0.5f;
    uint32 envmap_z = 0xFFFFFFFF;

    for (int i = 0; i < 5; i++) {
        *data++ = MakeGifTagLo(1, 0);
        *data++ = MakeGifTagHi(1, PRIM_TRISTRIP | PRIM_TME | PRIM_ABE, GIF_PACKED, 1);
        *data++ = SCE_GS_ST;
        *data++ = 0;

        *data++ = 0;
        *data++ = 0;
        *data++ = 0x3F800000;
        *data++ = 0;

        *data++ = MakeGifTagLo(12, 1);
        *data++ = MakeGifTagHi(1, PRIM_TRISTRIP | PRIM_TME | PRIM_ABE, GIF_REGLIST, 2);
        *data++ = SCE_GS_ST | (SCE_GS_XYZ2 << 4);
        *data++ = 0;

        for (int j = 0; j < 6; j++) {
            RefSample *left = &ReflectionSamples[envmap_face][i][j];
            RefSample *right = &ReflectionSamples[envmap_face][i + 1][j];

            int32 x0 = bFloatToInt(left->Rs * envmap_width - half_width + 2048.0f) << 4;
            int32 y0 = bFloatToInt(left->Rt * envmap_height - half_height + 2048.0f) << 4;
            int32 x1 = bFloatToInt(right->Rs * envmap_width - half_width + 2048.0f) << 4;
            int32 y1 = bFloatToInt(right->Rt * envmap_height - half_height + 2048.0f) << 4;

            uint32 xy0 = (x0 & 0xFFFF) | (y0 << 16);
            uint32 xy1 = (x1 & 0xFFFF) | (y1 << 16);

            *data++ = *reinterpret_cast<uint32 *>(&left->Fs);
            *data++ = *reinterpret_cast<uint32 *>(&left->Ft);
            *data++ = xy0;
            *data++ = envmap_z;

            *data++ = *reinterpret_cast<uint32 *>(&right->Fs);
            *data++ = *reinterpret_cast<uint32 *>(&right->Ft);
            *data++ = xy1;
            *data++ = envmap_z;
        }
    }

    eActiveTexture *active_texture = eGetActiveTexture(texture_info);
    eDmaDataList *dma_data_list = active_texture->GetDmaDataList(dest_target->GetID(), 1);

    if (dma_data_list) {
        int32 qwsize = (data - data_start) / 4;

        dma_data_list->AddData(4, data_start, qwsize, 0);
    }
}

// Lo mismo con las muestras de la cara de atras (BackReflectionSamples). Pinta una cara del mapa de entorno: cinco tiras de triangulos que llevan la imagen de la cara (source_target) a su
// sitio en dest_target, con las coordenadas de ReflectionSamples. El paquete va a la lista de DMA de la textura.
void CreateBackEnvMap(int envmap_face, eRenderTarget *source_target, eRenderTarget *dest_target) {
    TextureInfo *texture_info = source_target->GetTextureInfo(0);

    if (!texture_info) {
        return;
    }

    uint32 *data_start = static_cast<uint32 *>(eFrameMalloc(78 * sizeof(u_long128)));
    uint32 *data;

    if (!data_start) {
        return;
    }

    data_start[0] = MakeGifTagLo(1, 0);
    data_start[1] = MakeGifTagHi(1, PRIM_TRISTRIP | PRIM_TME | PRIM_ABE, GIF_PACKED, 2);
    data_start[2] = SCE_GS_ST | (SCE_GS_RGBAQ << 4);
    data_start[3] = 0;

    data_start[4] = 0;
    data_start[5] = 0;
    data_start[6] = 0x3F800000;
    data_start[7] = 0;

    data_start[8] = 128;
    data_start[9] = 128;
    data_start[10] = 128;
    data_start[11] = 96;
    data = &data_start[12];

    float envmap_width = dest_target->ScissorW;
    float envmap_height = dest_target->ScissorH;
    float half_width = envmap_width * 0.5f;
    float half_height = envmap_height * 0.5f;
    uint32 envmap_z = 0xFFFFFFFF;

    for (int i = 0; i < 5; i++) {
        *data++ = MakeGifTagLo(1, 0);
        *data++ = MakeGifTagHi(1, PRIM_TRISTRIP | PRIM_TME | PRIM_ABE, GIF_PACKED, 1);
        *data++ = SCE_GS_ST;
        *data++ = 0;

        *data++ = 0;
        *data++ = 0;
        *data++ = 0x3F800000;
        *data++ = 0;

        *data++ = MakeGifTagLo(12, 1);
        *data++ = MakeGifTagHi(1, PRIM_TRISTRIP | PRIM_TME | PRIM_ABE, GIF_REGLIST, 2);
        *data++ = SCE_GS_ST | (SCE_GS_XYZ2 << 4);
        *data++ = 0;

        for (int j = 0; j < 6; j++) {
            RefSample *left = &BackReflectionSamples[envmap_face][i][j];
            RefSample *right = &BackReflectionSamples[envmap_face][i + 1][j];

            int32 x0 = bFloatToInt(left->Rs * envmap_width - half_width + 2048.0f) << 4;
            int32 y0 = bFloatToInt(left->Rt * envmap_height - half_height + 2048.0f) << 4;
            int32 x1 = bFloatToInt(right->Rs * envmap_width - half_width + 2048.0f) << 4;
            int32 y1 = bFloatToInt(right->Rt * envmap_height - half_height + 2048.0f) << 4;

            uint32 xy0 = (x0 & 0xFFFF) | (y0 << 16);
            uint32 xy1 = (x1 & 0xFFFF) | (y1 << 16);

            *data++ = *reinterpret_cast<uint32 *>(&left->Fs);
            *data++ = *reinterpret_cast<uint32 *>(&left->Ft);
            *data++ = xy0;
            *data++ = envmap_z;

            *data++ = *reinterpret_cast<uint32 *>(&right->Fs);
            *data++ = *reinterpret_cast<uint32 *>(&right->Ft);
            *data++ = xy1;
            *data++ = envmap_z;
        }
    }

    eActiveTexture *active_texture = eGetActiveTexture(texture_info);
    eDmaDataList *dma_data_list = active_texture->GetDmaDataList(dest_target->GetID(), 1);

    if (dma_data_list) {
        int32 qwsize = (data - data_start) / 4;

        dma_data_list->AddData(4, data_start, qwsize, 0);
    }
}

void eDisplayEnvRenderTargets(eView *view) {
    ePoly poly;

    poly.SetFlags(poly.GetFlags() ^ 1);
}

// Pinta las caras activas del mapa de entorno en el destino (la de atras con sus cuatro cuartos).
void eCreateEnvMapPrims(eRenderTarget *dest_target) {
    eEnvMap *envmap = eGetEnvMap();

    for (int i = 0; i < 6; i++) {
#ifdef EA_BUILD_A124
        if (i == 2) {
            continue;
        }
#else
        // La version final pinta cada cara solo en su turno (IsFaceEnabled), y la de atras va con las demas.
        if (!IsFaceEnabled(i)) {
            continue;
        }
#endif

        eView *view = envmap->Views[i];
        eRenderTarget *target = view->GetRenderTarget();

        if (view->IsActive() && target->IsActive()) {
            int colour_rgb = EnvMapClearRGB[i];

            if (TheGameFlowManager.IsInFrontend()) {
                colour_rgb = 0;
            }

            int colour_a = 128;

            target->BackgroundColour[0] = colour_rgb;
            target->BackgroundColour[1] = colour_rgb;
            target->BackgroundColour[2] = colour_rgb;
            target->BackgroundColour[3] = colour_a;

            CreateEnvMap(i, target, dest_target);
        }
    }

#ifdef EA_BUILD_A124
    eView *view = envmap->Views[2];
    eRenderTarget *target = view->GetRenderTarget();

    if (view->IsActive() && target->IsActive()) {
        int colour_rgb = EnvMapClearRGB[2];
        int colour_a = 128;

        target->BackgroundColour[0] = colour_rgb;
        target->BackgroundColour[1] = colour_rgb;
        target->BackgroundColour[2] = colour_rgb;
        target->BackgroundColour[3] = colour_a;

        for (int j = 0; j < 4; j++) {
            CreateBackEnvMap(j, target, dest_target);
        }
    }
#endif
}

// La cara j del mapa de entorno se pinta este frame: siempre en el frontend; en carrera, con la camara del jugador 1
// rapida (mas de 30), solo las del turno de la tabla EnableEnvMapFaces.
int32 IsFaceEnabled(int32 i) {
    if (!TheGameFlowManager.IsInFrontend()) {
        eView *view = eGetView(EVIEW_PLAYER1, false);

        if (view && view->GetCameraMover()) {
            CameraAnchor *anchor = view->GetCameraMover()->GetAnchor();

            if (anchor) {
#ifdef EA_BUILD_A124
                if (anchor->GetVelocityMagnitude() > 30.0f) {
#else
                // La version final turna las caras tambien a poca velocidad con tres o mas policias a la vista (y
                // sin cinematica).
                if (anchor->GetVelocityMagnitude() > 5.0f || (numCopsActiveCherry >= 3 && !INIS::Exists())) {
#endif
                    return EnableEnvMapFaces[eFrameCounter % 5][i];
                }
            }
        }
    }

    return EnableFace[i];
}
