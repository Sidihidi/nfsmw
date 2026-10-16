#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EmitterSystem.h"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/PSX2/Inc/DmaE.hpp"
#include "Speed/PSX2/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/PSX2/Src/Ecstasy/eViewPlat.hpp"

// psx2/src/AcidFXE.cpp de la A124: las particulas del AcidFX en PS2. Cada particula visible se escribe como cinco
// quadwords de un paquete GIF directo en el frame malloc (acidPS2prim), y PlatEndParticleRender cierra el paquete con
// su GIF tag (REGLIST de 10 registros) y lo manda a la cola de render con eAttachDirectData.

unsigned int *acidPS2data;
unsigned int *acidPS2data_end;
unsigned int *acidPS2giftag;
unsigned int *acidPS2prim;
bMatrix4 *acidPS2world_view;
bMatrix4 *acidPS2world_screen;
bMatrix4 *acidPS2world_clip;
int32 acidPS2num_visible_particles;
int32 acidPS2num_visible_particles_axisconstrained;
float acidPS2average_z;
float acidPS2average_z_axisconstrained;
float acidPS2camera_h;
TextureInfo *acidPS2texture_info;
eView *acidPS2view;

// La define EmitterSystem.cpp (0x21dc98 en la A124); ninguna cabecera nuestra la declara todavia.
void AdjustConstrainedParticle(bVector4 &position, bVector4 &local_basis_x, bVector4 &local_basis_y, bVector4 &wp0, bVector4 &wp1,
                               bVector4 &wp2, bVector4 &wp3);

bool PlatStartParticleRender(eView *view, TextureInfo *mTextureInfo, unsigned int mNumParticles) {
    acidPS2data = static_cast<unsigned int *>(eFrameMallocGetPos());
    acidPS2data_end = static_cast<unsigned int *>(eFrameMallocGetEnd());
    acidPS2giftag = acidPS2data;
    acidPS2prim = acidPS2data + 4;
    acidPS2view = view;
    acidPS2average_z = 0.0f;
    acidPS2average_z_axisconstrained = 0.0f;
    acidPS2num_visible_particles = 0;
    acidPS2num_visible_particles_axisconstrained = 0;

    acidPS2world_view = view->GetPlatInfo()->GetWorldViewMatrix();
    acidPS2world_screen = view->GetPlatInfo()->GetWorldScreenMatrix();

    acidPS2world_clip = view->GetPlatInfo()->GetWorldClipGBMatrix();

    acidPS2camera_h = CalculateH(view->GetCamera()->GetFieldOfView(), view);
    acidPS2texture_info = mTextureInfo;

    return true;
}

void PlatEndParticleRender() {
    int32 total_num_visible_particles = acidPS2num_visible_particles + acidPS2num_visible_particles_axisconstrained;

    if (total_num_visible_particles && acidPS2data_end - acidPS2prim > 32) {
        float avg_z = acidPS2average_z / acidPS2num_visible_particles;

        int particle_qwsize = 5;
        int total_qwsize = total_num_visible_particles * particle_qwsize + 1;

        *acidPS2giftag++ = MakeGifTagLo(total_num_visible_particles, 1);
        *acidPS2giftag++ = MakeGifTagHi(1, 0x54, 1, 10);
        *acidPS2giftag++ = 0x42424210;
        *acidPS2giftag++ = 0x42;

        eFrameMallocSetPos(acidPS2prim);

        eAttachDirectData(acidPS2texture_info, avg_z, acidPS2giftag - 4, total_qwsize);
    }
}

// Una particula: se recorta contra el frustum y se escribe en el paquete que abre PlatStartParticleRender como PRIM,
// RGBAQ y cuatro vertices (ST y XYZ2). Las restringidas a ejes llevan sus cuatro esquinas del mundo
// (AdjustConstrainedParticle); las demas son un cuadrado en pantalla, girado con mAngle si hace falta. Las ST son flotantes
// y van al paquete tal cual, por eso se copian sus bits.
void PlatAddParticle(const EmitterParticle &particle, const UMath::Vector3 &upVec, const UMath::Vector3 &rightVec, unsigned int hack_flags,
                     bVector4 *x_constrain_basis, bVector4 *y_constrain_basis) {
    bVector4 position(particle.mPosX, particle.mPosY, particle.mPosZ, 1.0f);
    int32 sx0, sy0, sz0, sx1, sy1, sz1, sx2, sy2, sz2, sx3, sy3, sz3;
    uint32 is0, it0, is1, it1;

    if (eGetClipFlags(&position, acidPS2world_clip)) {
        return;
    }

    if (x_constrain_basis && y_constrain_basis && (hack_flags & 2)) {
        bVector4 wp0 = position;
        bVector4 wp1 = position;
        bVector4 wp2 = position;
        bVector4 wp3 = position;
        bVector4 sposition_0;
        bVector4 sposition_1;
        bVector4 sposition_2;
        bVector4 sposition_3;
        bVector4 vposition;

        AdjustConstrainedParticle(position, *x_constrain_basis, *y_constrain_basis, wp0, wp1, wp2, wp3);

        eRotTransPers(&sposition_0, acidPS2world_screen, &wp0);
        eRotTransPers(&sposition_1, acidPS2world_screen, &wp1);
        eRotTransPers(&sposition_2, acidPS2world_screen, &wp2);
        eRotTransPers(&sposition_3, acidPS2world_screen, &wp3);

        sx0 = bFloatToInt(sposition_0.x) << 4; sy0 = bFloatToInt(sposition_0.y) << 4; sz0 = (FloatAsInteger(sposition_0.z) & 0x0FFFFFFF) >> 4;
        sx1 = bFloatToInt(sposition_1.x) << 4; sy1 = bFloatToInt(sposition_1.y) << 4; sz1 = (FloatAsInteger(sposition_1.z) & 0x0FFFFFFF) >> 4;
        sx2 = bFloatToInt(sposition_2.x) << 4; sy2 = bFloatToInt(sposition_2.y) << 4; sz2 = (FloatAsInteger(sposition_2.z) & 0x0FFFFFFF) >> 4;
        sx3 = bFloatToInt(sposition_3.x) << 4; sy3 = bFloatToInt(sposition_3.y) << 4; sz3 = (FloatAsInteger(sposition_3.z) & 0x0FFFFFFF) >> 4;

        eMulVector(&vposition, acidPS2world_view, &wp0);
        acidPS2average_z_axisconstrained += bFloatToInt(vposition.z);

        acidPS2num_visible_particles_axisconstrained++;
    } else {
        int32 sx;
        int32 sy;
        bVector4 sposition;
        bVector4 vposition;

        eMulVector(&vposition, acidPS2world_view, &position);
        eRotTransPers(&sposition, acidPS2world_screen, &position);

        sx = bFloatToInt(sposition.x);
        sy = bFloatToInt(sposition.y);
        sz2 = (FloatAsInteger(sposition.z) & 0x0FFFFFFF) >> 4;
        sz0 = sz2;
        sz1 = sz2;
        sz3 = sz2;

        float pixel_size = particle.mSize * acidPS2camera_h / vposition.z;

        int32 w = bFloatToInt(pixel_size * 8.0f);
        int32 h = bFloatToInt(pixel_size * 7.0f);

        if (particle.mAngle == 0) {
            sx0 = (sx << 4) - w; sy0 = (sy << 4) - h;
            sx1 = (sx << 4) + w; sy1 = (sy << 4) - h;
            sx2 = (sx << 4) - w; sy2 = (sy << 4) + h;
            sx3 = (sx << 4) + w; sy3 = (sy << 4) + h;
        } else {
            bMatrix4 rz;

            eCreateRotationZ(&rz, particle.mAngle);

            bVector3 sxy0(-pixel_size, -pixel_size, 0.0f);
            bVector3 sxy1(pixel_size, -pixel_size, 0.0f);
            bVector3 sxy2(-pixel_size, pixel_size, 0.0f);
            bVector3 sxy3(pixel_size, pixel_size, 0.0f);
            bVector3 aspect(8.0f, 7.0f, 0.0f);
            bVector3 sxy(sx << 4, sy << 4, 0.0f);

            bMulMatrix(&sxy0, &rz, &sxy0);
            bMulMatrix(&sxy1, &rz, &sxy1);
            bMulMatrix(&sxy2, &rz, &sxy2);
            bMulMatrix(&sxy3, &rz, &sxy3);

            sxy0 = sxy + bScale(sxy0, aspect);
            sxy1 = sxy + bScale(sxy1, aspect);
            sxy2 = sxy + bScale(sxy2, aspect);
            sxy3 = sxy + bScale(sxy3, aspect);

            sx0 = bFloatToInt(sxy0.x); sy0 = bFloatToInt(sxy0.y);
            sx1 = bFloatToInt(sxy1.x); sy1 = bFloatToInt(sxy1.y);
            sx2 = bFloatToInt(sxy2.x); sy2 = bFloatToInt(sxy2.y);
            sx3 = bFloatToInt(sxy3.x); sy3 = bFloatToInt(sxy3.y);
        }

        acidPS2num_visible_particles++;

        acidPS2average_z += bFloatToInt(vposition.z);
    }

    float s0 = static_cast<float>((particle.mUVStart & 0xFFFF0000) >> 16) / 65535.0f;
    float t0 = static_cast<float>(particle.mUVStart & 0xFFFF) / 65535.0f;
    float s1 = static_cast<float>((particle.mUVEnd & 0xFFFF0000) >> 16) / 65535.0f;
    float t1 = static_cast<float>(particle.mUVEnd & 0xFFFF) / 65535.0f;

    is0 = *(uint32 *)&s0; it0 = *(uint32 *)&t0;
    is1 = *(uint32 *)&s1; it1 = *(uint32 *)&t1;

    *acidPS2prim++ = 0x54;
    *acidPS2prim++ = 0;
    *acidPS2prim++ = particle.mColour;
    *acidPS2prim++ = 0x3F800000;

    *acidPS2prim++ = is0;
    *acidPS2prim++ = it0;
    *acidPS2prim++ = (sy0 << 16) | (sx0 & 0xFFFF);
    *acidPS2prim++ = sz0;

    *acidPS2prim++ = is1;
    *acidPS2prim++ = it0;
    *acidPS2prim++ = (sy1 << 16) | (sx1 & 0xFFFF);
    *acidPS2prim++ = sz1;

    *acidPS2prim++ = is0;
    *acidPS2prim++ = it1;
    *acidPS2prim++ = (sy2 << 16) | (sx2 & 0xFFFF);
    *acidPS2prim++ = sz2;

    *acidPS2prim++ = is1;
    *acidPS2prim++ = it1;
    *acidPS2prim++ = (sy3 << 16) | (sx3 & 0xFFFF);
    *acidPS2prim++ = sz3;
}

void PlatGetViewVectors(eView *view, UMath::Vector3 &right, UMath::Vector3 &up, UMath::Vector3 &fwd) {
    bMatrix4 m;

    bCopy(&m, view->GetPlatInfo()->GetWorldViewMatrix());
    right.x = m.v0.x; right.y = m.v1.y; right.z = m.v2.z;
    up.x = m.v0.x; up.y = m.v1.y; up.z = m.v2.z;
    fwd.x = m.v0.x; fwd.y = m.v1.y; fwd.z = m.v2.z;
}
