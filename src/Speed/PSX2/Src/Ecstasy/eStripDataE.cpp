#include "Speed/PSX2/Src/Ecstasy/eStripData.hpp"

// psx2/src/ecstasy/eStripDataE.cpp de la A124. De sus diecinueve funciones el enlazador solo conserva cinco
// (GetElementNORM de char, GetCompression, GetAABB, SetMipMapK y SetNORM): las demas salen en el .mdebug sin
// direccion y sus bytes quedan sin reubicar en el ejecutable. Aqui van las cinco que se pueden comprobar.

void eStripDataE::GetElementNORM(int vert, signed char **x, signed char **y, signed char **z) {
    signed char *base_ptr = reinterpret_cast<signed char *>(GetNORMBase());
    int slot = vert & 3;

    base_ptr += (vert >> 2) * 12 + 4;

    *x = &base_ptr[slot];
    *y = &base_ptr[slot + 4];
    *z = &base_ptr[slot + 8];
}

int eStripData::GetCompression() {
    return reinterpret_cast<unsigned char *>(this)[6];
}

void eStripData::GetAABB(bVector4 *min, bVector4 *max) {
    float *aabb_base = reinterpret_cast<float *>(static_cast<eStripDataE *>(this)->GetAABBBase());

    float min_x = aabb_base[0];
    float min_y = aabb_base[1];
    float min_z = aabb_base[2];
    float max_x = aabb_base[3];
    float max_y = aabb_base[4];
    float max_z = aabb_base[5];

    min->x = min_x;
    min->y = min_y;
    min->z = min_z;
    min->w = 0.0f;
    max->x = max_x;
    max->y = max_y;
    max->z = max_z;
    max->w = 0.0f;
}

void eStripData::SetMipMapK(unsigned char mipmap_k) {
    reinterpret_cast<unsigned int *>(this)[4] = mipmap_k;
}

void eStripData::SetNORM(int vert, float x, float y, float z) {
    signed char *px;
    signed char *py;
    signed char *pz;

    static_cast<eStripDataE *>(this)->GetElementNORM(vert, &px, &py, &pz);

    x *= 127.0f;
    y *= 127.0f;
    z *= 127.0f;

    *px = bFloatToInt(x);
    *py = bFloatToInt(y);
    *pz = bFloatToInt(z);
}
