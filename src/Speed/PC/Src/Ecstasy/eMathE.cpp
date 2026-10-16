// eMathE de PC: la misma math de Ecstasy que en GameCube (eMathE.cpp), con D3DX (d3dx9_26.dll) en lugar de MTX44.
// Los manglings y los cuerpos salen del exe: bCos/bSin directos en rotaciones (sin los eSin(bAngToRad(...)) de
// GameCube), D3DXMatrixMultiply/D3DXVec4Transform/D3DXVec3Transform en los productos.
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/PC/Src/Ecstasy/D3D9Min.hpp"

bMatrix4 eMathIdentityMatrix;
bMatrix4 eMathZeroMatrix;

void eMathInit(void) {
    bMatrix4 *identity = eGetIdentityMatrix();

    bIdentity(identity);
    bMemSet(eGetZeroMatrix(), 0, sizeof(bMatrix4));
}

void eMulMatrix(bMatrix4 *ab, bMatrix4 *a, bMatrix4 *b) {
    D3DXMatrixMultiply(reinterpret_cast<D3DXMATRIX *>(ab), reinterpret_cast<const D3DXMATRIX *>(a),
                       reinterpret_cast<const D3DXMATRIX *>(b));
}

void eMulVector(bVector4 *vm, const bMatrix4 *m, const bVector4 *v) {
    D3DXVec4Transform(reinterpret_cast<D3DXVECTOR4 *>(vm), reinterpret_cast<const D3DXVECTOR4 *>(v),
                       reinterpret_cast<const D3DXMATRIX *>(m));
}

void eMulVector(bVector3 *vm, const bMatrix4 *m, const bVector3 *v) {
    D3DXVec3Transform(reinterpret_cast<D3DXVECTOR4 *>(vm), reinterpret_cast<const D3DXVECTOR3 *>(v),
                       reinterpret_cast<const D3DXMATRIX *>(m));
}

void eCreateAxisRotationMatrix(bMatrix4 *dest, bVector3 &axis, bAngle angle) {
    float c = bCos(angle);
    float s = bSin(angle);
    float t = 1.0f - c;
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;
    float tx = t * x;
    float ty = t * y;

    dest->v0.x = tx * x + c;
    dest->v0.y = tx * y - s * z;
    dest->v0.z = tx * z + s * y;
    dest->v0.w = 0.0f;
    dest->v1.x = tx * y + s * z;
    dest->v1.y = ty * y + c;
    dest->v1.z = ty * z - x * s;
    dest->v1.w = 0.0f;
    dest->v2.x = tx * z - s * y;
    dest->v2.y = ty * z + x * s;
    dest->v2.z = t * z * z + c;
    dest->v2.w = 0.0f;
    dest->v3.x = 0.0f;
    dest->v3.y = 0.0f;
    dest->v3.z = 0.0f;
    dest->v3.w = 1.0f;
}

void eCreateRotationZ(bMatrix4 *dest, bAngle angle) {
    float cn = bCos(angle);
    float sn = bSin(angle);

    dest->v0.x = cn;
    dest->v0.y = sn;
    dest->v0.z = 0.0f;
    dest->v0.w = 0.0f;
    dest->v1.x = -sn;
    dest->v1.y = cn;
    dest->v1.z = 0.0f;
    dest->v1.w = 0.0f;
    dest->v2.x = 0.0f;
    dest->v2.y = 0.0f;
    dest->v2.z = 1.0f;
    dest->v2.w = 0.0f;
    dest->v3.x = 0.0f;
    dest->v3.y = 0.0f;
    dest->v3.z = 0.0f;
    dest->v3.w = 1.0f;
}

void eRotateX(bMatrix4 *dest, bMatrix4 *a, bAngle angle) {
    bMatrix4 r;
    float cn = bCos(angle);
    float sn = bSin(angle);

    r.v0.x = 1.0f;
    r.v0.y = 0.0f;
    r.v0.z = 0.0f;
    r.v0.w = 0.0f;
    r.v1.x = 0.0f;
    r.v1.y = cn;
    r.v1.z = sn;
    r.v1.w = 0.0f;
    r.v2.x = 0.0f;
    r.v2.y = -sn;
    r.v2.z = cn;
    r.v2.w = 0.0f;
    r.v3.x = 0.0f;
    r.v3.y = 0.0f;
    r.v3.z = 0.0f;
    r.v3.w = 1.0f;
    eMulMatrix(dest, a, &r);
}

void eRotateY(bMatrix4 *dest, bMatrix4 *a, bAngle angle) {
    bMatrix4 r;
    float cn = bCos(angle);
    float sn = bSin(angle);

    r.v0.x = cn;
    r.v0.y = 0.0f;
    r.v0.z = -sn;
    r.v0.w = 0.0f;
    r.v1.x = 0.0f;
    r.v1.y = 1.0f;
    r.v1.z = 0.0f;
    r.v1.w = 0.0f;
    r.v2.x = sn;
    r.v2.y = 0.0f;
    r.v2.z = cn;
    r.v2.w = 0.0f;
    r.v3.x = 0.0f;
    r.v3.y = 0.0f;
    r.v3.z = 0.0f;
    r.v3.w = 1.0f;
    eMulMatrix(dest, a, &r);
}

void eRotateZ(bMatrix4 *dest, bMatrix4 *a, bAngle angle) {
    bMatrix4 rot;

    eCreateRotationZ(&rot, angle);
    eMulMatrix(dest, a, &rot);
}

void eTranslate(bMatrix4 *dest, bMatrix4 *a, bVector3 *tran) {
    bMatrix4 t;

    t.v0.x = 1.0f;
    t.v0.y = 0.0f;
    t.v0.z = 0.0f;
    t.v0.w = 0.0f;
    t.v1.x = 0.0f;
    t.v1.y = 1.0f;
    t.v1.z = 0.0f;
    t.v1.w = 0.0f;
    t.v2.x = 0.0f;
    t.v2.y = 0.0f;
    t.v2.z = 1.0f;
    t.v2.w = 0.0f;
    t.v3.x = tran->x;
    t.v3.y = tran->y;
    t.v3.z = tran->z;
    t.v3.w = 1.0f;
    eMulMatrix(dest, a, &t);
}

void eCreateTranslationMatrix(bMatrix4 *dest, bVector3 &tran) {
    dest->v0.x = 1.0f;
    dest->v0.y = 0.0f;
    dest->v0.z = 0.0f;
    dest->v0.w = 0.0f;
    dest->v1.x = 0.0f;
    dest->v1.y = 1.0f;
    dest->v1.z = 0.0f;
    dest->v1.w = 0.0f;
    dest->v2.x = 0.0f;
    dest->v2.y = 0.0f;
    dest->v2.z = 1.0f;
    dest->v2.w = 0.0f;
    dest->v3.x = tran.x;
    dest->v3.y = tran.y;
    dest->v3.z = tran.z;
    dest->v3.w = 1.0f;
}

bMatrix4 *eInvertTransformationMatrix(bMatrix4 *dest, const bMatrix4 *tm) {
    bMatrix4 m1;
    bMatrix4 m2;
    float x0 = tm->v0.x;
    float y0 = tm->v0.y;
    float z0 = tm->v0.z;
    float x1 = tm->v1.x;
    float y1 = tm->v1.y;
    float z1 = tm->v1.z;
    float x2 = tm->v2.x;
    float y2 = tm->v2.y;
    float z2 = tm->v2.z;
    float x3 = tm->v3.x;
    float y3 = tm->v3.y;
    float z3 = tm->v3.z;

    m1.v0.x = x0;
    m1.v0.y = x1;
    m1.v0.z = x2;
    m1.v0.w = 0.0f;
    m1.v1.x = y0;
    m1.v1.y = y1;
    m1.v1.z = y2;
    m1.v1.w = 0.0f;
    m1.v2.x = z0;
    m1.v2.y = z1;
    m1.v2.z = z2;
    m1.v2.w = 0.0f;
    m1.v3.x = 0.0f;
    m1.v3.y = 0.0f;
    m1.v3.z = 0.0f;
    m1.v3.w = 1.0f;

    m2.v0.x = 1.0f;
    m2.v0.y = 0.0f;
    m2.v0.z = 0.0f;
    m2.v0.w = 0.0f;
    m2.v1.x = 0.0f;
    m2.v1.y = 1.0f;
    m2.v1.z = 0.0f;
    m2.v1.w = 0.0f;
    m2.v2.x = 0.0f;
    m2.v2.y = 0.0f;
    m2.v2.z = 1.0f;
    m2.v2.w = 0.0f;
    m2.v3.x = -x3;
    m2.v3.y = -y3;
    m2.v3.z = -z3;
    m2.v3.w = 1.0f;

    eMulMatrix(dest, &m2, &m1);
    return dest;
}

bMatrix4 *eInvertRotationMatrix(bMatrix4 *dest, bMatrix4 *src) {
    bTransposeMatrix(dest, src);
    dest->v0.w = 0.0f;
    dest->v1.w = 0.0f;
    dest->v2.w = 0.0f;
    dest->v3.w = 0.0f;
    dest->v3.x = 0.0f;
    dest->v3.y = 0.0f;
    dest->v3.z = 0.0f;
    return dest;
}

void eCreateLookAtMatrix(bMatrix4 *mat, bVector3 &eye, bVector3 &center, bVector3 &up) {
    bVector3 c = center - eye;
    bNormalize(&c, &c);
    bVector3 b = -up;
    bVector3 a;
    bMatrix4 tl;

    bCross(&a, &b, &c);
    bCross(&b, &c, &a);
    bNormalize(&a, &a);
    bNormalize(&b, &b);

    mat->v0.x = a.x;
    mat->v0.y = b.x;
    mat->v0.z = c.x;
    mat->v0.w = 0.0f;
    mat->v1.x = a.y;
    mat->v1.y = b.y;
    mat->v1.z = c.y;
    mat->v1.w = 0.0f;
    mat->v2.x = a.z;
    mat->v2.y = b.z;
    mat->v2.z = c.z;
    mat->v2.w = 0.0f;
    mat->v3.x = 0.0f;
    mat->v3.y = 0.0f;
    mat->v3.z = 0.0f;
    mat->v3.w = 1.0f;

    tl.v0.x = 1.0f;
    tl.v0.y = 0.0f;
    tl.v0.z = 0.0f;
    tl.v0.w = 0.0f;
    tl.v1.x = 0.0f;
    tl.v1.y = 1.0f;
    tl.v1.z = 0.0f;
    tl.v1.w = 0.0f;
    tl.v2.x = 0.0f;
    tl.v2.y = 0.0f;
    tl.v2.z = 1.0f;
    tl.v2.w = 0.0f;
    tl.v3.x = -eye.x;
    tl.v3.y = -eye.y;
    tl.v3.z = -eye.z;
    tl.v3.w = 1.0f;
    eMulMatrix(mat, &tl, mat);
}
