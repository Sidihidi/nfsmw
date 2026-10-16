

#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#ifdef EA_PLATFORM_XENON
#include <xtl.h>
#include <d3dx9math.h>
#endif

void bInvertMatrix(bMatrix4 *dest, const bMatrix4 *src) {
    eInvertMatrix(dest, const_cast<bMatrix4 *>(src));
}

// UNSOLVED
float fDeterminant(bMatrix4 *m) {
    float value = m->v0.w * m->v1.z * m->v2.y * m->v3.x - m->v0.z * m->v1.w * m->v2.y * m->v3.x - m->v0.w * m->v1.y * m->v2.z * m->v3.x +
                  m->v0.y * m->v1.w * m->v2.z * m->v3.x + m->v0.z * m->v1.y * m->v2.w * m->v3.x - m->v0.y * m->v1.z * m->v2.w * m->v3.x -
                  m->v0.w * m->v1.z * m->v2.x * m->v3.y + m->v0.z * m->v1.w * m->v2.x * m->v3.y + m->v0.w * m->v1.x * m->v2.z * m->v3.y -
                  m->v0.x * m->v1.w * m->v2.z * m->v3.y - m->v0.z * m->v1.x * m->v2.w * m->v3.y + m->v0.x * m->v1.z * m->v2.w * m->v3.y +
                  m->v0.w * m->v1.y * m->v2.x * m->v3.z - m->v0.y * m->v1.w * m->v2.x * m->v3.z - m->v0.w * m->v1.x * m->v2.y * m->v3.z +
                  m->v0.x * m->v1.w * m->v2.y * m->v3.z + m->v0.y * m->v1.x * m->v2.w * m->v3.z - m->v0.x * m->v1.y * m->v2.w * m->v3.z -
                  m->v0.z * m->v1.y * m->v2.x * m->v3.w + m->v0.y * m->v1.z * m->v2.x * m->v3.w + m->v0.z * m->v1.x * m->v2.y * m->v3.w -
                  m->v0.x * m->v1.z * m->v2.y * m->v3.w - m->v0.y * m->v1.x * m->v2.z * m->v3.w + m->v0.x * m->v1.y * m->v2.z * m->v3.w;

    return value;
}

void fInvertMatrix(bMatrix4 *d, bMatrix4 *s) {
    float scale = 1.0f / fDeterminant(s);

    d->v0.x = scale * (((((s->v1.z * s->v2.w * s->v3.y - s->v1.w * s->v2.z * s->v3.y) + s->v1.w * s->v2.y * s->v3.z) - s->v1.y * s->v2.w * s->v3.z) -
                        s->v1.z * s->v2.y * s->v3.w) +
                       s->v1.y * s->v2.z * s->v3.w);
    d->v0.y = scale * ((((s->v0.w * s->v2.z * s->v3.y - s->v0.z * s->v2.w * s->v3.y) - s->v0.w * s->v2.y * s->v3.z) + s->v0.y * s->v2.w * s->v3.z +
                        s->v0.z * s->v2.y * s->v3.w) -
                       s->v0.y * s->v2.z * s->v3.w);
    d->v0.z = scale * (((((s->v0.z * s->v1.w * s->v3.y - s->v0.w * s->v1.z * s->v3.y) + s->v0.w * s->v1.y * s->v3.z) - s->v0.y * s->v1.w * s->v3.z) -
                        s->v0.z * s->v1.y * s->v3.w) +
                       s->v0.y * s->v1.z * s->v3.w);
    d->v0.w = scale * ((((s->v0.w * s->v1.z * s->v2.y - s->v0.z * s->v1.w * s->v2.y) - s->v0.w * s->v1.y * s->v2.z) + s->v0.y * s->v1.w * s->v2.z +
                        s->v0.z * s->v1.y * s->v2.w) -
                       s->v0.y * s->v1.z * s->v2.w);
    d->v1.x = scale * ((((s->v1.w * s->v2.z * s->v3.x - s->v1.z * s->v2.w * s->v3.x) - s->v1.w * s->v2.x * s->v3.z) + s->v1.x * s->v2.w * s->v3.z +
                        s->v1.z * s->v2.x * s->v3.w) -
                       s->v1.x * s->v2.z * s->v3.w);
    d->v1.y = scale * (((((s->v0.z * s->v2.w * s->v3.x - s->v0.w * s->v2.z * s->v3.x) + s->v0.w * s->v2.x * s->v3.z) - s->v0.x * s->v2.w * s->v3.z) -
                        s->v0.z * s->v2.x * s->v3.w) +
                       s->v0.x * s->v2.z * s->v3.w);
    d->v1.z = scale * ((((s->v0.w * s->v1.z * s->v3.x - s->v0.z * s->v1.w * s->v3.x) - s->v0.w * s->v1.x * s->v3.z) + s->v0.x * s->v1.w * s->v3.z +
                        s->v0.z * s->v1.x * s->v3.w) -
                       s->v0.x * s->v1.z * s->v3.w);
    d->v1.w = scale * (((((s->v0.z * s->v1.w * s->v2.x - s->v0.w * s->v1.z * s->v2.x) + s->v0.w * s->v1.x * s->v2.z) - s->v0.x * s->v1.w * s->v2.z) -
                        s->v0.z * s->v1.x * s->v2.w) +
                       s->v0.x * s->v1.z * s->v2.w);
    d->v2.x = scale * (((((s->v1.y * s->v2.w * s->v3.x - s->v1.w * s->v2.y * s->v3.x) + s->v1.w * s->v2.x * s->v3.y) - s->v1.x * s->v2.w * s->v3.y) -
                        s->v1.y * s->v2.x * s->v3.w) +
                       s->v1.x * s->v2.y * s->v3.w);
    d->v2.y = scale * ((((s->v0.w * s->v2.y * s->v3.x - s->v0.y * s->v2.w * s->v3.x) - s->v0.w * s->v2.x * s->v3.y) + s->v0.x * s->v2.w * s->v3.y +
                        s->v0.y * s->v2.x * s->v3.w) -
                       s->v0.x * s->v2.y * s->v3.w);
    d->v2.z = scale * (((((s->v0.y * s->v1.w * s->v3.x - s->v0.w * s->v1.y * s->v3.x) + s->v0.w * s->v1.x * s->v3.y) - s->v0.x * s->v1.w * s->v3.y) -
                        s->v0.y * s->v1.x * s->v3.w) +
                       s->v0.x * s->v1.y * s->v3.w);
    d->v2.w = scale * ((((s->v0.w * s->v1.y * s->v2.x - s->v0.y * s->v1.w * s->v2.x) - s->v0.w * s->v1.x * s->v2.y) + s->v0.x * s->v1.w * s->v2.y +
                        s->v0.y * s->v1.x * s->v2.w) -
                       s->v0.x * s->v1.y * s->v2.w);
    d->v3.x = scale * ((((s->v1.z * s->v2.y * s->v3.x - s->v1.y * s->v2.z * s->v3.x) - s->v1.z * s->v2.x * s->v3.y) + s->v1.x * s->v2.z * s->v3.y +
                        s->v1.y * s->v2.x * s->v3.z) -
                       s->v1.x * s->v2.y * s->v3.z);
    d->v3.y = scale * (((((s->v0.y * s->v2.z * s->v3.x - s->v0.z * s->v2.y * s->v3.x) + s->v0.z * s->v2.x * s->v3.y) - s->v0.x * s->v2.z * s->v3.y) -
                        s->v0.y * s->v2.x * s->v3.z) +
                       s->v0.x * s->v2.y * s->v3.z);
    d->v3.z = scale * ((((s->v0.z * s->v1.y * s->v3.x - s->v0.y * s->v1.z * s->v3.x) - s->v0.z * s->v1.x * s->v3.y) + s->v0.x * s->v1.z * s->v3.y +
                        s->v0.y * s->v1.x * s->v3.z) -
                       s->v0.x * s->v1.y * s->v3.z);
    d->v3.w = scale * (((((s->v0.y * s->v1.z * s->v2.x - s->v0.z * s->v1.y * s->v2.x) + s->v0.z * s->v1.x * s->v2.y) - s->v0.x * s->v1.z * s->v2.y) -
                        s->v0.y * s->v1.x * s->v2.z) +
                       s->v0.x * s->v1.y * s->v2.z);
}

void hermite_basis(bMatrix4 *b, bMatrix4 *p, float u1, float u2, float u3, float u4) {
    bMatrix4 U;
    bMatrix4 iU;
    bMatrix4 Mf;
    bMatrix4 iMf;
    bMatrix4 K;
    bMatrix4 Nf;

    Mf.v0.x = 2.0f;
    Mf.v0.y = -2.0f;
    Mf.v0.z = 1.0f;
    Mf.v0.w = 1.0f;
    Mf.v1.x = -3.0f;
    Mf.v1.y = 3.0f;
    Mf.v1.z = -2.0f;
    Mf.v1.w = -1.0f;
    Mf.v2.x = 0.0f;
    Mf.v2.y = 0.0f;
    Mf.v2.z = 1.0f;
    Mf.v2.w = 0.0f;
    Mf.v3.x = 1.0f;
    Mf.v3.y = 0.0f;
    Mf.v3.z = 0.0f;
    Mf.v3.w = 0.0f;

    iMf.v0.x = 0.0f;
    iMf.v0.y = 0.0f;
    iMf.v0.z = 0.0f;
    iMf.v0.w = 1.0f;
    iMf.v1.x = 1.0f;
    iMf.v1.y = 1.0f;
    iMf.v1.z = 1.0f;
    iMf.v1.w = 1.0f;
    iMf.v2.x = 0.0f;
    iMf.v2.y = 0.0f;
    iMf.v2.z = 1.0f;
    iMf.v2.w = 0.0f;
    iMf.v3.x = 3.0f;
    iMf.v3.y = 2.0f;
    iMf.v3.z = 1.0f;
    iMf.v3.w = 0.0f;

    U.v0.x = u1 * u1 * u1;
    U.v0.y = u1 * u1;
    U.v0.z = u1;
    U.v0.w = 1.0f;
    U.v1.x = u2 * u2 * u2;
    U.v1.y = u2 * u2;
    U.v1.z = u2;
    U.v1.w = 1.0f;
    U.v2.x = u3 * u3 * u3;
    U.v2.y = u3 * u3;
    U.v2.z = u3;
    U.v2.w = 1.0f;
    U.v3.x = u4 * u4 * u4;
    U.v3.y = u4 * u4;
    U.v3.z = u4;
    U.v3.w = 1.0f;

    fInvertMatrix(&iU, &U);
    eMulMatrix(&K, &iMf, &iU);
    eMulMatrix(&Nf, &Mf, &K);
    eMulMatrix(b, &Nf, p);
}

void hermite_parameter(bVector4 *dest, const bMatrix4 *b, float t) {
    bVector4 u;

    u.x = t * t * t;
    u.y = t * t;
    u.z = t;
    u.w = 1.0f;
    eMulVector(dest, b, &u);
}

/* En PS2 las tres son VU0 en linea, no una llamada a Ecstasy: el original
 * carga las cuatro filas de cada matriz con `lqc2' y acumula con la cadena
 * `vmulax'/`vmadday'/`vmaddaz'/`vmaddw'.  Los `vnop' de las dos de vector no
 * son adorno: en modo macro hay que esperar a que la cadena termine antes de
 * leer el registro de destino, y la de matriz no los lleva porque las cuatro
 * columnas siguientes ya hacen esa espera. */
void bMulMatrix(bMatrix4 *dest, const bMatrix4 *a, const bMatrix4 *b) {
#ifdef EA_PLATFORM_PLAYSTATION2
    asm __volatile__("lqc2 vf4, 0(%2)\n"
                     "lqc2 vf5, 16(%2)\n"
                     "lqc2 vf6, 32(%2)\n"
                     "lqc2 vf7, 48(%2)\n"
                     "lqc2 vf8, 0(%1)\n"
                     "lqc2 vf9, 16(%1)\n"
                     "lqc2 vf10, 32(%1)\n"
                     "lqc2 vf11, 48(%1)\n"
                     "vmulax.xyzw ACC, vf8, vf4x\n"
                     "vmadday.xyzw ACC, vf9, vf4y\n"
                     "vmaddaz.xyzw ACC, vf10, vf4z\n"
                     "vmaddw.xyzw vf12, vf11, vf4w\n"
                     "vmulax.xyzw ACC, vf8, vf5x\n"
                     "vmadday.xyzw ACC, vf9, vf5y\n"
                     "vmaddaz.xyzw ACC, vf10, vf5z\n"
                     "vmaddw.xyzw vf13, vf11, vf5w\n"
                     "vmulax.xyzw ACC, vf8, vf6x\n"
                     "vmadday.xyzw ACC, vf9, vf6y\n"
                     "vmaddaz.xyzw ACC, vf10, vf6z\n"
                     "vmaddw.xyzw vf14, vf11, vf6w\n"
                     "vmulax.xyzw ACC, vf8, vf7x\n"
                     "vmadday.xyzw ACC, vf9, vf7y\n"
                     "vmaddaz.xyzw ACC, vf10, vf7z\n"
                     "vmaddw.xyzw vf15, vf11, vf7w\n"
                     "sqc2 vf12, 0(%0)\n"
                     "sqc2 vf13, 16(%0)\n"
                     "sqc2 vf14, 32(%0)\n"
                     "sqc2 vf15, 48(%0)"
                     :
                     : "r"(dest), "r"(a), "r"(b)
                     : "memory");
#else
    eMulMatrix(dest, const_cast<bMatrix4 *>(b), const_cast<bMatrix4 *>(a));
#endif
}

void bMulMatrix(bVector4 *dest, const bMatrix4 *m, const bVector4 *v) {
#ifdef EA_PLATFORM_PLAYSTATION2
    asm __volatile__("lqc2 vf4, 0(%2)\n"
                     "lqc2 vf8, 0(%1)\n"
                     "lqc2 vf9, 16(%1)\n"
                     "lqc2 vf10, 32(%1)\n"
                     "lqc2 vf11, 48(%1)\n"
                     "vmulax.xyzw ACC, vf8, vf4x\n"
                     "vmadday.xyzw ACC, vf9, vf4y\n"
                     "vmaddaz.xyzw ACC, vf10, vf4z\n"
                     "vmaddw.xyzw vf12, vf11, vf4w\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "sqc2 vf12, 0(%0)"
                     :
                     : "r"(dest), "r"(m), "r"(v)
                     : "memory");
#else
    eMulVector(dest, m, v);
#endif
}

void bMulMatrix(bVector3 *dest, const bMatrix4 *m, const bVector3 *v) {
#ifdef EA_PLATFORM_PLAYSTATION2
    // La cuarta fila entra con vf0w (el 1,0f constante de VU0) en vez de vf4w:
    // un bVector3 no tiene w que leer.
    asm __volatile__("lqc2 vf4, 0(%2)\n"
                     "lqc2 vf8, 0(%1)\n"
                     "lqc2 vf9, 16(%1)\n"
                     "lqc2 vf10, 32(%1)\n"
                     "lqc2 vf11, 48(%1)\n"
                     "vmulax.xyz ACC, vf8, vf4x\n"
                     "vmadday.xyz ACC, vf9, vf4y\n"
                     "vmaddaz.xyz ACC, vf10, vf4z\n"
                     "vmaddw.xyz vf12, vf11, vf0w\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "sqc2 vf12, 0(%0)"
                     :
                     : "r"(dest), "r"(m), "r"(v)
                     : "memory");
#else
    eMulVector(dest, m, v);
#endif
}

// PS2: MMI de EA. Las ocho locales de 128 bits (t0..t7, en GPR) salen de los stabs de la A124, y pextlw/pextuw/
// pcpyld/pcpyud no los genera el compilador. Cada instruccion es una sentencia (lineas 378-397 del bMatrix.cpp de
// la A124, reordenadas por el planificador). lq y sq tambien son asm: el ultimo sq no va a la ranura del jr (un
// store de C si) y con lq/sq de C cambian el orden y los registros. PC llama a D3DXMatrixTranspose.
bMatrix4 *bTransposeMatrix(bMatrix4 *dest, const bMatrix4 *m) {
#if defined(EA_PLATFORM_PLAYSTATION2)
    u_long128 t0;
    u_long128 t1;
    u_long128 t2;
    u_long128 t3;
    u_long128 t4;
    u_long128 t5;
    u_long128 t6;
    u_long128 t7;

    asm("lq %0, %1" : "=r"(t0) : "m"(m->v0));
    asm("lq %0, %1" : "=r"(t1) : "m"(m->v1));
    asm("lq %0, %1" : "=r"(t2) : "m"(m->v2));
    asm("lq %0, %1" : "=r"(t3) : "m"(m->v3));

    asm("pextlw %0, %1, %2" : "=r"(t4) : "r"(t1), "r"(t0));
    asm("pextuw %0, %1, %2" : "=r"(t5) : "r"(t1), "r"(t0));
    asm("pextlw %0, %1, %2" : "=r"(t6) : "r"(t3), "r"(t2));
    asm("pextuw %0, %1, %2" : "=r"(t7) : "r"(t3), "r"(t2));

    asm("pcpyld %0, %1, %2" : "=r"(t0) : "r"(t6), "r"(t4));
    asm("pcpyud %0, %1, %2" : "=r"(t1) : "r"(t4), "r"(t6));
    asm("pcpyld %0, %1, %2" : "=r"(t2) : "r"(t7), "r"(t5));
    asm("pcpyud %0, %1, %2" : "=r"(t3) : "r"(t5), "r"(t7));

    asm("sq %1, %0" : "=m"(dest->v0) : "r"(t0));
    asm("sq %1, %0" : "=m"(dest->v1) : "r"(t1));
    asm("sq %1, %0" : "=m"(dest->v2) : "r"(t2));
    asm("sq %1, %0" : "=m"(dest->v3) : "r"(t3));
#elif defined(EA_PLATFORM_GAMECUBE)
    MTX44Transpose(*reinterpret_cast<const Mtx44 *>(m), *reinterpret_cast<Mtx44 *>(dest));
#elif defined(EA_PLATFORM_XENON)
    // NFS.exe 0x82168890: D3DXMatrixTranspose (0x82679F20) y devuelve dest.
    D3DXMatrixTranspose(reinterpret_cast<D3DXMATRIX *>(dest), reinterpret_cast<const D3DXMATRIX *>(m));
#else
    // Por copia: dest puede ser la propia m.
    bMatrix4 t;
    t.v0.x = m->v0.x;
    t.v0.y = m->v1.x;
    t.v0.z = m->v2.x;
    t.v0.w = m->v3.x;
    t.v1.x = m->v0.y;
    t.v1.y = m->v1.y;
    t.v1.z = m->v2.y;
    t.v1.w = m->v3.y;
    t.v2.x = m->v0.z;
    t.v2.y = m->v1.z;
    t.v2.z = m->v2.z;
    t.v2.w = m->v3.z;
    t.v3.x = m->v0.w;
    t.v3.y = m->v1.w;
    t.v3.z = m->v2.w;
    t.v3.w = m->v3.w;
    *dest = t;
#endif
    return dest;
}
